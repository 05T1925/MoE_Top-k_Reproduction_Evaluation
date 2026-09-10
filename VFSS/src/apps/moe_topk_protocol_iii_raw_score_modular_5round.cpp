#include <moe_topk/masked_mul_adapter.h>
#include <moe_topk/protocol_i_party_package.h>
#include <moe_topk/protocol_i_ucmp.h>
#include <moe_topk/protocol_iii_raw_score_pipeline.h>

#include <FSS/comms.h>
#include <FSS/dpf.h>

#include <array>
#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <limits>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <unistd.h>

namespace {

using namespace moe_topk;
using Bytes = std::vector<std::uint8_t>;

constexpr const char* kProtocolLabel =
    "moe_topk_protocol_iii_raw_score_modular_5round";
constexpr int kOnlineTimeoutMs = 15000;
constexpr int kOfflineTimeoutMs = 300000;
constexpr int kDpfPayloadBits = 64;
constexpr std::size_t kMaximumMessageBytes = 64U * 1024U * 1024U;
constexpr std::size_t kReportMetricCount = 22U;
constexpr std::uint64_t kScoreMask = (UINT64_C(1) << 34U) - 1U;

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::uint8_t bit_width(std::uint32_t value) {
  std::uint8_t bits = 0;
  while (value != 0U) {
    ++bits;
    value >>= 1U;
  }
  return bits;
}

std::uint32_t padded_size(std::uint32_t logical_n) {
  require(logical_n != 0U, "logical_n must be positive");
  std::uint32_t padded_n = 2U;
  while (padded_n < logical_n) {
    require(
        padded_n <= std::numeric_limits<std::uint32_t>::max() / 2U,
        "padded_n overflow");
    padded_n <<= 1U;
  }
  return padded_n;
}

std::uint8_t rank_bits(std::uint32_t logical_n) {
  return logical_n <= 1U ? 1U : bit_width(logical_n - 1U);
}

std::uint8_t comparison_bits(std::uint32_t padded_n) {
  const auto value =
      static_cast<std::uint32_t>(33U + bit_width(padded_n - 1U));
  require(value < 64U, "comparison width is unsupported");
  return static_cast<std::uint8_t>(value);
}

std::size_t cell_count(std::uint32_t logical_n, std::uint32_t k) {
  require(logical_n != 0U && k != 0U && k <= logical_n,
          "invalid secure-combine dimensions");
  require(
      static_cast<std::size_t>(logical_n) <=
          std::numeric_limits<std::size_t>::max() /
              static_cast<std::size_t>(k),
      "secure-combine cell count overflow");
  return static_cast<std::size_t>(logical_n) * k;
}

void put_u64(Bytes& bytes, std::uint64_t value) {
  for (int shift = 56; shift >= 0; shift -= 8) {
    bytes.push_back(static_cast<std::uint8_t>(value >> shift));
  }
}

std::uint64_t get_u64(
    const Bytes& bytes,
    std::size_t& offset,
    const char* message) {
  require(offset <= bytes.size() &&
              bytes.size() - offset >= sizeof(std::uint64_t),
          message);
  std::uint64_t value = 0;
  for (int index = 0; index < 8; ++index) {
    value = (value << 8U) | bytes[offset++];
  }
  return value;
}

void wait_for_fd(int fd, short event, int timeout_ms) {
  require(fd >= 0 && timeout_ms > 0, "invalid role I/O parameters");
  pollfd descriptor{fd, event, 0};
  int result = -1;
  do {
    result = ::poll(&descriptor, 1, timeout_ms);
  } while (result < 0 && errno == EINTR);
  require(result > 0, "protocol role I/O timeout");
  require((descriptor.revents & (POLLERR | POLLNVAL)) == 0,
          "protocol role poll error");
  require((descriptor.revents & event) != 0, "protocol role peer closed");
}

void write_all(
    int fd,
    const std::uint8_t* data,
    std::size_t size,
    int timeout_ms) {
  while (size != 0U) {
    wait_for_fd(fd, POLLOUT, timeout_ms);
    const auto written = ::write(fd, data, size);
    if (written < 0 && errno == EINTR) {
      continue;
    }
    require(written > 0, "protocol role write failed");
    data += written;
    size -= static_cast<std::size_t>(written);
  }
}

void read_exact(
    int fd,
    std::uint8_t* data,
    std::size_t size,
    int timeout_ms) {
  while (size != 0U) {
    wait_for_fd(fd, POLLIN, timeout_ms);
    const auto count = ::read(fd, data, size);
    if (count < 0 && errno == EINTR) {
      continue;
    }
    require(count > 0, "protocol role read failed");
    data += count;
    size -= static_cast<std::size_t>(count);
  }
}

void send_message(int fd, const Bytes& message, int timeout_ms) {
  require(!message.empty() && message.size() <= kMaximumMessageBytes,
          "invalid protocol output message");
  Bytes header;
  header.reserve(sizeof(std::uint64_t));
  put_u64(header, message.size());
  write_all(fd, header.data(), header.size(), timeout_ms);
  write_all(fd, message.data(), message.size(), timeout_ms);
}

Bytes receive_message(int fd, int timeout_ms) {
  Bytes header(sizeof(std::uint64_t));
  read_exact(fd, header.data(), header.size(), timeout_ms);
  std::size_t offset = 0;
  const auto length = get_u64(header, offset, "truncated message length");
  require(length != 0U && length <= kMaximumMessageBytes &&
              length <= std::numeric_limits<std::size_t>::max(),
          "invalid protocol message length");
  Bytes message(static_cast<std::size_t>(length));
  read_exact(fd, message.data(), message.size(), timeout_ms);
  return message;
}

std::vector<std::uint32_t> decode_raw_score_shares(
    const Bytes& bytes,
    std::size_t expected_words) {
  require(expected_words <=
              std::numeric_limits<std::size_t>::max() /
                  sizeof(std::uint32_t),
          "raw-score input size overflow");
  require(bytes.size() == expected_words * sizeof(std::uint32_t),
          "raw-score share payload length");
  std::vector<std::uint32_t> shares(expected_words);
  for (std::size_t index = 0; index < expected_words; ++index) {
    for (std::size_t byte = 0; byte < sizeof(std::uint32_t); ++byte) {
      shares[index] =
          (shares[index] << 8U) |
          bytes[index * sizeof(std::uint32_t) + byte];
    }
  }
  return shares;
}

class ScopedFd final {
 public:
  explicit ScopedFd(int fd = -1) : fd_(fd) {}
  ~ScopedFd() { reset(); }
  ScopedFd(const ScopedFd&) = delete;
  ScopedFd& operator=(const ScopedFd&) = delete;
  ScopedFd(ScopedFd&& other) noexcept : fd_(other.release()) {}
  ScopedFd& operator=(ScopedFd&& other) noexcept {
    if (this != &other) {
      reset();
      fd_ = other.release();
    }
    return *this;
  }
  int release() noexcept {
    const int result = fd_;
    fd_ = -1;
    return result;
  }
  void reset() noexcept {
    if (fd_ >= 0) {
      ::close(fd_);
      fd_ = -1;
    }
  }

 private:
  int fd_ = -1;
};

ScopedFd duplicate_lifetime_fd(int fd, const char* message) {
  require(fd >= 0, message);
  int duplicated = -1;
  do {
    duplicated = ::dup(fd);
  } while (duplicated < 0 && errno == EINTR);
  require(duplicated >= 0, message);
  return ScopedFd(duplicated);
}

ProtocolIIIRawScorePipelineConfig make_config(
    std::uint8_t party,
    std::uint32_t logical_n,
    std::uint32_t k,
    std::uint64_t session,
    std::uint64_t fingerprint) {
  require(party < 2U, "party must be 0 or 1");
  require(logical_n != 0U && k != 0U && k <= logical_n,
          "invalid public dimensions");
  require(session != 0U && fingerprint != 0U, "invalid public binding");

  const auto padded_n = padded_size(logical_n);
  const auto index_bits = bit_width(padded_n - 1U);
  const auto key_bits = comparison_bits(padded_n);
  const auto r_bits = rank_bits(logical_n);

  ProtocolIIIRawScorePipelineConfig config;
  config.score_input.session = session;
  config.score_input.fingerprint = fingerprint;
  config.score_input.logical_n = logical_n;
  config.score_input.padded_n = padded_n;
  config.score_input.k = k;
  config.score_input.index_bits = index_bits;
  config.score_input.comparison_bits = key_bits;
  config.score_input.party = party;
  config.score_input.timeout_ms = kOnlineTimeoutMs;

  config.grank.session = session;
  config.grank.fingerprint = fingerprint;
  config.grank.logical_n = logical_n;
  config.grank.padded_n = padded_n;
  config.grank.k = k;
  config.grank.comparison_bits = key_bits;
  config.grank.rank_bits = r_bits;
  config.grank.party = party;
  config.grank.timeout_ms = kOnlineTimeoutMs;

  config.routing.session = session;
  config.routing.fingerprint = fingerprint;
  config.routing.logical_n = logical_n;
  config.routing.k = k;
  config.routing.rank_bits = r_bits;
  config.routing.comparison_bits = key_bits;
  config.routing.party = party;
  config.routing.timeout_ms = kOnlineTimeoutMs;

  config.combine.session = session;
  config.combine.fingerprint = fingerprint;
  config.combine.logical_n = logical_n;
  config.combine.k = k;
  config.combine.comparison_bits = key_bits;
  config.combine.party = party;
  config.combine.timeout_ms = kOnlineTimeoutMs;
  return config;
}

ProtocolIPartyPackage deserialize_score_input_package(
    const ProtocolIIIRawScorePipelineConfig& config,
    int expected_party,
    const Bytes& bytes) {
  require(bytes.size() >= 40U && bytes[0] == 'M' && bytes[1] == '3' &&
              bytes[2] == 'S' && bytes[3] == 'I' && bytes[4] == 1U &&
              bytes[5] == expected_party &&
              bytes[6] == config.score_input.comparison_bits &&
              bytes[7] == 0U,
          "score-input package header");

  std::size_t offset = 8U;
  ProtocolIPartyPackage package;
  package.party = expected_party;
  package.comparison_bits = bytes[6];
  package.session = get_u64(bytes, offset, "score-input session");
  package.fingerprint = get_u64(bytes, offset, "score-input fingerprint");
  const auto n = get_u64(bytes, offset, "score-input n");
  const auto k = get_u64(bytes, offset, "score-input k");

  require(package.session == config.score_input.session &&
              package.fingerprint == config.score_input.fingerprint &&
              n == config.score_input.padded_n && k == config.score_input.k,
          "score-input package public binding");
  package.n = static_cast<std::uint32_t>(n);
  package.k = static_cast<std::uint32_t>(k);

  const auto read_stage =
      [&](std::vector<ProtocolIScoreInputPartyMaterial>& items,
          std::uint8_t expected_stage) {
        items.reserve(package.n);
        for (std::uint32_t slot = 0; slot < package.n; ++slot) {
          const auto encoded_slot = get_u64(bytes, offset, "score-input slot");
          const auto encoded_stage =
              get_u64(bytes, offset, "score-input stage");
          const auto left = get_u64(bytes, offset, "score-input left share");
          const auto right = get_u64(bytes, offset, "score-input right share");
          const auto length =
              get_u64(bytes, offset, "score-input material length");
          require(encoded_slot == slot && encoded_stage == expected_stage &&
                      (left & ~kScoreMask) == 0U &&
                      (right & ~kScoreMask) == 0U &&
                      length <= bytes.size() - offset,
                  "score-input material identity");
          const auto material_size = static_cast<std::size_t>(length);
          Bytes encoded(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                        bytes.begin() + static_cast<std::ptrdiff_t>(
                                            offset + material_size));
          offset += material_size;
          auto material = ProtocolIUcmpPartyMaterial::deserialize(encoded);
          require(material.party_id() == expected_party &&
                      material.comparison_bits() == 34,
                  "score-input material party binding");
          items.emplace_back(slot, expected_stage, left, right,
                             std::move(material));
        }
      };

  read_stage(package.carry_materials, 1U);
  read_stage(package.sign_materials, 2U);
  require(offset == bytes.size(), "score-input package trailing bytes");
  return package;
}

ProtocolIIIRawScorePipelineMaterial deserialize_offline_material(
    const ProtocolIIIRawScorePipelineConfig& config,
    const Bytes& bytes) {
  const int party = static_cast<int>(config.score_input.party);
  require(bytes.size() >= 64U && bytes[0] == 'M' && bytes[1] == '3' &&
              bytes[2] == 'R' && bytes[3] == 'F' && bytes[4] == 1U &&
              bytes[5] == party && bytes[6] == config.grank.rank_bits &&
              bytes[7] == config.grank.comparison_bits,
          "offline material header");

  std::size_t offset = 8U;
  require(get_u64(bytes, offset, "offline session") == config.grank.session &&
              get_u64(bytes, offset, "offline fingerprint") ==
                  config.grank.fingerprint &&
              get_u64(bytes, offset, "offline logical_n") ==
                  config.grank.logical_n &&
              get_u64(bytes, offset, "offline padded_n") ==
                  config.grank.padded_n &&
              get_u64(bytes, offset, "offline k") == config.grank.k,
          "offline material binding");

  const auto score_length =
      get_u64(bytes, offset, "offline score package length");
  require(score_length <= bytes.size() - offset,
          "offline score package truncated");
  const auto grank_length =
      get_u64(bytes, offset, "offline GRank package length");
  require(score_length <= bytes.size() - offset &&
              grank_length <= bytes.size() - offset - score_length,
          "offline packages truncated");

  const auto score_size = static_cast<std::size_t>(score_length);
  Bytes score_bytes(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                    bytes.begin() + static_cast<std::ptrdiff_t>(offset + score_size));
  offset += score_size;
  const auto grank_size = static_cast<std::size_t>(grank_length);
  Bytes grank_bytes(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                    bytes.begin() + static_cast<std::ptrdiff_t>(offset + grank_size));
  offset += grank_size;

  ProtocolIIIRawScorePipelineMaterial material;
  material.score_input_package =
      deserialize_score_input_package(config, party, score_bytes);
  material.grank_package = deserialize_party_package(grank_bytes, party);

  material.routing_material.session = config.grank.session;
  material.routing_material.fingerprint = config.grank.fingerprint;
  material.routing_material.logical_n = config.grank.logical_n;
  material.routing_material.k = config.grank.k;
  material.routing_material.rank_bits = config.grank.rank_bits;
  material.routing_material.party = config.grank.party;
  material.routing_material.rank_mask_shares.resize(config.grank.logical_n);
  for (auto& share : material.routing_material.rank_mask_shares) {
    share = get_u64(bytes, offset, "offline rank-mask share");
  }

  material.unit_payload_shares.resize(config.grank.logical_n);
  for (auto& share : material.unit_payload_shares) {
    share = get_u64(bytes, offset, "offline unit-payload share");
  }

  material.combine_material.session = config.grank.session;
  material.combine_material.fingerprint = config.grank.fingerprint;
  material.combine_material.logical_n = config.grank.logical_n;
  material.combine_material.k = config.grank.k;
  material.combine_material.party = config.grank.party;

  char* cursor = reinterpret_cast<char*>(
      const_cast<std::uint8_t*>(bytes.data() + offset));
  Dealer receiver(&cursor);
  try {
    material.routing_material.dpf_keys.reserve(config.grank.logical_n);
    for (std::uint32_t index = 0; index < config.grank.logical_n; ++index) {
      auto wire_key = receiver.recv_dpf_keypack(config.grank.rank_bits,
                                                kDpfPayloadBits);
      require(wire_key.s != nullptr, "offline DPF key has no seed blocks");
      DPFKeyPack owned_key(config.grank.rank_bits, kDpfPayloadBits);
      std::memcpy(owned_key.s, wire_key.s,
                  static_cast<std::size_t>(config.grank.rank_bits + 1U) *
                      sizeof(osuCrypto::block));
      owned_key.tLcw = wire_key.tLcw;
      owned_key.tRcw = wire_key.tRcw;
      owned_key.payload = wire_key.payload;
      material.routing_material.dpf_keys.emplace_back(std::move(owned_key));
      wire_key.s = nullptr;
    }

    const auto cells = cell_count(config.grank.logical_n, config.grank.k);
    material.combine_material.multiplication_materials.reserve(cells);
    for (std::size_t cell = 0; cell < cells; ++cell) {
      material.combine_material.multiplication_materials.push_back(
          receive_masked_mul_material(receiver));
    }
    require(cursor == reinterpret_cast<char*>(const_cast<std::uint8_t*>(
                          bytes.data() + bytes.size())),
            "offline material trailing bytes");
  } catch (...) {
    delete static_cast<MemBuf*>(receiver.keyBuf);
    throw;
  }
  delete static_cast<MemBuf*>(receiver.keyBuf);
  return material;
}

Bytes encode_report(
    const ProtocolIIIRawScorePipelineOutput& output,
    std::uint64_t offline_bytes) {
  Bytes bytes(output.xor_mask_shares.begin(), output.xor_mask_shares.end());
  const auto& metrics = output.metrics;
  const std::array<std::uint64_t, kReportMetricCount> record{{
      offline_bytes,
      metrics.score_input.carry_sent_bytes,
      metrics.score_input.carry_received_bytes,
      metrics.score_input.sign_sent_bytes,
      metrics.score_input.sign_received_bytes,
      metrics.grank.sent_bytes,
      metrics.grank.received_bytes,
      metrics.grank.comparison_edges,
      metrics.grank.raw_dcf_calls,
      metrics.routing.sent_bytes,
      metrics.routing.received_bytes,
      metrics.routing.dpf_keys,
      metrics.routing.eval_calls,
      metrics.combine.sent_bytes,
      metrics.combine.received_bytes,
      metrics.combine.multiplication_calls,
      metrics.combine.opened_masked_values,
      metrics.input_adapter_rounds,
      metrics.core_rounds,
      metrics.total_online_rounds,
      metrics.sent_bytes,
      metrics.received_bytes,
  }};
  for (const auto value : record) {
    put_u64(bytes, value);
  }
  return bytes;
}

std::uint64_t parse_u64(const char* text, const char* message) {
  require(text != nullptr, message);
  std::size_t consumed = 0;
  const std::string value(text);
  const auto parsed = std::stoull(value, &consumed, 10);
  require(consumed == value.size(), message);
  return parsed;
}

std::uint32_t parse_u32(const char* text, const char* message) {
  const auto value = parse_u64(text, message);
  require(value <= std::numeric_limits<std::uint32_t>::max(), message);
  return static_cast<std::uint32_t>(value);
}

int parse_fd(const char* text, const char* message) {
  const auto value = parse_u64(text, message);
  require(value <= static_cast<std::uint64_t>(
                       std::numeric_limits<int>::max()),
          message);
  return static_cast<int>(value);
}

int run_party(
    int party,
    int offline_fd,
    int input_fd,
    int carry_fd,
    int sign_fd,
    int grank_fd,
    int routing_fd,
    int combine_fd,
    int result_fd,
    std::uint32_t logical_n,
    std::uint32_t k,
    std::uint64_t session,
    std::uint64_t fingerprint) {
  require(party == 0 || party == 1, "party must be 0 or 1");

  auto carry_lifetime =
      duplicate_lifetime_fd(carry_fd, "carry lifetime dup failed");
  auto sign_lifetime =
      duplicate_lifetime_fd(sign_fd, "sign lifetime dup failed");
  auto grank_lifetime =
      duplicate_lifetime_fd(grank_fd, "GRank lifetime dup failed");
  auto routing_lifetime =
      duplicate_lifetime_fd(routing_fd, "routing lifetime dup failed");
  auto combine_lifetime =
      duplicate_lifetime_fd(combine_fd, "combine lifetime dup failed");

  const auto config = make_config(
      static_cast<std::uint8_t>(party), logical_n, k, session, fingerprint);
  const auto offline_message = receive_message(offline_fd, kOfflineTimeoutMs);
  auto material = deserialize_offline_material(config, offline_message);
  const auto raw_score_shares = decode_raw_score_shares(
      receive_message(input_fd, kOnlineTimeoutMs), logical_n);

  ProtocolIIIRawScorePipelineFds fds;
  fds.score_input_fds = {{carry_fd, sign_fd}};
  fds.grank_fd = grank_fd;
  fds.routing_fd = routing_fd;
  fds.combine_fd = combine_fd;

  const auto output = protocol_iii_raw_score_pipeline_party(
      config, material, raw_score_shares, fds);
  require(output.metrics.input_adapter_rounds == 2U,
          "formal raw-score adapter round count");
  require(output.metrics.core_rounds == 3U,
          "formal raw-score core round count");
  require(output.metrics.total_online_rounds == 5U,
          "formal raw-score total round count");
  require(output.xor_mask_shares.size() == logical_n,
          "formal raw-score output length");

  require(material.score_input_package.carry_materials.empty() &&
              material.score_input_package.sign_materials.empty(),
          "raw-score material not consumed");
  require(material.grank_package.node_mask_shares.empty() &&
              material.grank_package.edge_materials.empty(),
          "GRank material not consumed");
  require(material.routing_material.rank_mask_shares.empty() &&
              material.routing_material.dpf_keys.empty(),
          "routing material not consumed");
  require(material.combine_material.multiplication_materials.empty(),
          "combine material not consumed");

  const auto report = encode_report(
      output,
      static_cast<std::uint64_t>(offline_message.size() +
                                 sizeof(std::uint64_t)));
  send_message(result_fd, report, kOnlineTimeoutMs);

  const auto acknowledgement = receive_message(result_fd, kOnlineTimeoutMs);
  require(acknowledgement.size() == 2U && acknowledgement[0] == 'O' &&
              acknowledgement[1] == 'K',
          "invalid controller acknowledgement");

  carry_lifetime.reset();
  sign_lifetime.reset();
  grank_lifetime.reset();
  routing_lifetime.reset();
  combine_lifetime.reset();
  return 0;
}

void print_usage(const char* program) {
  std::cerr
      << "Usage:\n  " << program
      << " party PARTY OFFLINE_FD INPUT_FD CARRY_FD SIGN_FD"
      << " GRANK_FD ROUTING_FD COMBINE_FD RESULT_FD"
      << " LOGICAL_N K SESSION FINGERPRINT\n";
}

}  // namespace

int main(int argc, char** argv) {
  std::signal(SIGPIPE, SIG_IGN);
  try {
    if (argc == 2 && std::string(argv[1]) == "--label") {
      std::cout << kProtocolLabel << '\n';
      return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "--help") {
      print_usage(argv[0]);
      return 0;
    }
    require(argc == 15 && std::string(argv[1]) == "party",
            "invalid raw-score Protocol III role arguments");
    return run_party(
        static_cast<int>(parse_u32(argv[2], "invalid party")),
        parse_fd(argv[3], "invalid offline fd"),
        parse_fd(argv[4], "invalid input fd"),
        parse_fd(argv[5], "invalid carry fd"),
        parse_fd(argv[6], "invalid sign fd"),
        parse_fd(argv[7], "invalid GRank fd"),
        parse_fd(argv[8], "invalid routing fd"),
        parse_fd(argv[9], "invalid combine fd"),
        parse_fd(argv[10], "invalid result fd"),
        parse_u32(argv[11], "invalid logical_n"),
        parse_u32(argv[12], "invalid k"),
        parse_u64(argv[13], "invalid session"),
        parse_u64(argv[14], "invalid fingerprint"));
  } catch (const std::exception& error) {
    std::cerr << kProtocolLabel << ": " << error.what() << '\n';
    print_usage(argc > 0 ? argv[0] : kProtocolLabel);
    return 1;
  }
}
