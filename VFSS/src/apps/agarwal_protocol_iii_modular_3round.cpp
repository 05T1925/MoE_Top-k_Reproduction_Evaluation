#include <moe_topk/masked_mul_adapter.h>
#include <moe_topk/protocol_i_party_package.h>
#include <moe_topk/protocol_iii_secure_core.h>

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
    "agarwal_protocol_iii_modular_3round";

constexpr int kOnlineTimeoutMs = 15000;
constexpr int kOfflineTimeoutMs = 300000;
constexpr int kDpfPayloadBits = 64;

constexpr std::size_t kMaximumMessageBytes =
    64U * 1024U * 1024U;

constexpr std::size_t kReportMetricCount = 14U;

void require(
    bool condition,
    const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::uint8_t bit_width(
    std::uint32_t value) {
  std::uint8_t bits = 0;

  while (value != 0U) {
    ++bits;
    value >>= 1U;
  }

  return bits;
}

std::uint32_t padded_size(
    std::uint32_t logical_n) {
  require(
      logical_n != 0U,
      "logical_n must be positive");

  std::uint32_t padded_n = 2U;

  while (padded_n < logical_n) {
    require(
        padded_n <=
            std::numeric_limits<std::uint32_t>::max() / 2U,
        "padded_n overflow");

    padded_n <<= 1U;
  }

  return padded_n;
}

std::uint8_t rank_bits(
    std::uint32_t logical_n) {
  return logical_n <= 1U
             ? 1U
             : bit_width(logical_n - 1U);
}

std::uint8_t comparison_bits(
    std::uint32_t padded_n) {
  const auto bits =
      static_cast<std::uint32_t>(
          33U + bit_width(padded_n - 1U));

  require(
      bits < 64U,
      "comparison width is unsupported");

  return static_cast<std::uint8_t>(bits);
}

std::size_t cell_count(
    std::uint32_t logical_n,
    std::uint32_t k) {
  require(
      logical_n != 0U &&
          k != 0U &&
          k <= logical_n,
      "invalid secure-combine dimensions");

  require(
      static_cast<std::size_t>(logical_n) <=
          std::numeric_limits<std::size_t>::max() /
              static_cast<std::size_t>(k),
      "secure-combine cell count overflow");

  return static_cast<std::size_t>(logical_n) *
         static_cast<std::size_t>(k);
}

void put_u64(
    Bytes& bytes,
    std::uint64_t value) {
  for (int shift = 56;
       shift >= 0;
       shift -= 8) {
    bytes.push_back(
        static_cast<std::uint8_t>(value >> shift));
  }
}

std::uint64_t get_u64(
    const Bytes& bytes,
    std::size_t& offset,
    const char* message) {
  require(
      offset <= bytes.size() &&
          bytes.size() - offset >=
              sizeof(std::uint64_t),
      message);

  std::uint64_t value = 0;

  for (int index = 0;
       index < 8;
       ++index) {
    value =
        (value << 8U) |
        static_cast<std::uint64_t>(bytes[offset++]);
  }

  return value;
}

void wait_for_fd(
    int fd,
    short event,
    int timeout_ms) {
  require(fd >= 0, "invalid file descriptor");
  require(timeout_ms > 0, "invalid I/O timeout");

  pollfd descriptor{fd, event, 0};

  int result = -1;

  do {
    result = ::poll(
        &descriptor,
        1,
        timeout_ms);
  } while (result < 0 && errno == EINTR);

  require(result > 0, "protocol role I/O timeout");

  require(
      (descriptor.revents &
       (POLLERR | POLLNVAL)) == 0,
      "protocol role poll error");

  require(
      (descriptor.revents & event) != 0,
      "protocol role peer closed");
}

void write_all(
    int fd,
    const std::uint8_t* data,
    std::size_t size,
    int timeout_ms) {
  require(
      data != nullptr || size == 0U,
      "invalid write buffer");

  while (size != 0U) {
    wait_for_fd(
        fd,
        POLLOUT,
        timeout_ms);

    const auto written =
        ::write(fd, data, size);

    if (written < 0 && errno == EINTR) {
      continue;
    }

    require(
        written > 0,
        "protocol role write failed");

    data += written;
    size -=
        static_cast<std::size_t>(written);
  }
}

void read_exact(
    int fd,
    std::uint8_t* data,
    std::size_t size,
    int timeout_ms) {
  require(
      data != nullptr || size == 0U,
      "invalid read buffer");

  while (size != 0U) {
    wait_for_fd(
        fd,
        POLLIN,
        timeout_ms);

    const auto count =
        ::read(fd, data, size);

    if (count < 0 && errno == EINTR) {
      continue;
    }

    require(
        count > 0,
        "protocol role read failed");

    data += count;
    size -=
        static_cast<std::size_t>(count);
  }
}

void send_message(
    int fd,
    const Bytes& message,
    int timeout_ms) {
  require(
      !message.empty() &&
          message.size() <= kMaximumMessageBytes,
      "invalid protocol output message");

  Bytes header;
  header.reserve(sizeof(std::uint64_t));
  put_u64(header, message.size());

  write_all(
      fd,
      header.data(),
      header.size(),
      timeout_ms);

  write_all(
      fd,
      message.data(),
      message.size(),
      timeout_ms);
}

Bytes receive_message(
    int fd,
    int timeout_ms) {
  Bytes header(sizeof(std::uint64_t));

  read_exact(
      fd,
      header.data(),
      header.size(),
      timeout_ms);

  std::size_t offset = 0;

  const auto length =
      get_u64(
          header,
          offset,
          "truncated message length");

  require(
      length != 0U &&
          length <= kMaximumMessageBytes &&
          length <=
              std::numeric_limits<std::size_t>::max(),
      "invalid protocol message length");

  Bytes message(
      static_cast<std::size_t>(length));

  read_exact(
      fd,
      message.data(),
      message.size(),
      timeout_ms);

  return message;
}

std::vector<std::uint64_t> decode_words(
    const Bytes& bytes,
    std::size_t expected_words) {
  require(
      expected_words <=
          std::numeric_limits<std::size_t>::max() /
              sizeof(std::uint64_t),
      "priority-key input size overflow");

  require(
      bytes.size() ==
          expected_words *
              sizeof(std::uint64_t),
      "priority-key input length");

  std::vector<std::uint64_t> words(
      expected_words);

  std::size_t offset = 0;

  for (auto& word : words) {
    word =
        get_u64(
            bytes,
            offset,
            "truncated priority-key input");
  }

  return words;
}

class ScopedFd final {
 public:
  ScopedFd() = default;

  explicit ScopedFd(int fd)
      : fd_(fd) {}

  ~ScopedFd() {
    reset();
  }

  ScopedFd(const ScopedFd&) = delete;
  ScopedFd& operator=(const ScopedFd&) = delete;

  ScopedFd(ScopedFd&& other) noexcept
      : fd_(other.release()) {}

  ScopedFd& operator=(
      ScopedFd&& other) noexcept {
    if (this != &other) {
      reset();
      fd_ = other.release();
    }

    return *this;
  }

  int get() const noexcept {
    return fd_;
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

ScopedFd duplicate_lifetime_fd(
    int fd,
    const char* message) {
  require(fd >= 0, message);

  int duplicated = -1;

  do {
    duplicated = ::dup(fd);
  } while (duplicated < 0 &&
           errno == EINTR);

  require(duplicated >= 0, message);

  return ScopedFd(duplicated);
}

ProtocolIIISecureCoreConfig make_config(
    std::uint8_t party,
    std::uint32_t logical_n,
    std::uint32_t k,
    std::uint64_t session,
    std::uint64_t fingerprint) {
  require(
      party < 2U,
      "party must be 0 or 1");

  require(
      logical_n != 0U &&
          k != 0U &&
          k <= logical_n,
      "invalid public dimensions");

  require(
      session != 0U &&
          fingerprint != 0U,
      "invalid public binding");

  const auto padded_n =
      padded_size(logical_n);

  const auto cmp_bits =
      comparison_bits(padded_n);

  const auto r_bits =
      rank_bits(logical_n);

  ProtocolIIISecureCoreConfig config;

  config.grank.session = session;
  config.grank.fingerprint = fingerprint;
  config.grank.logical_n = logical_n;
  config.grank.padded_n = padded_n;
  config.grank.k = k;
  config.grank.comparison_bits = cmp_bits;
  config.grank.rank_bits = r_bits;
  config.grank.party = party;
  config.grank.timeout_ms =
      kOnlineTimeoutMs;

  config.routing.session = session;
  config.routing.fingerprint = fingerprint;
  config.routing.logical_n = logical_n;
  config.routing.k = k;
  config.routing.rank_bits = r_bits;
  config.routing.comparison_bits = cmp_bits;
  config.routing.party = party;
  config.routing.timeout_ms =
      kOnlineTimeoutMs;

  config.combine.session = session;
  config.combine.fingerprint = fingerprint;
  config.combine.logical_n = logical_n;
  config.combine.k = k;
  config.combine.comparison_bits = cmp_bits;
  config.combine.party = party;
  config.combine.timeout_ms =
      kOnlineTimeoutMs;

  return config;
}

ProtocolIIISecureCoreMaterial
deserialize_offline_material(
    const ProtocolIIISecureCoreConfig& config,
    const Bytes& bytes) {
  const auto party = config.grank.party;

  require(
      bytes.size() >= 56U &&
          bytes[0] == 'M' &&
          bytes[1] == '3' &&
          bytes[2] == 'O' &&
          bytes[3] == 'F' &&
          bytes[4] == 1U &&
          bytes[5] == party &&
          bytes[6] ==
              config.grank.rank_bits &&
          bytes[7] ==
              config.grank.comparison_bits,
      "offline material header");

  std::size_t offset = 8U;

  require(
      get_u64(
          bytes,
          offset,
          "offline session") ==
              config.grank.session &&
          get_u64(
              bytes,
              offset,
              "offline fingerprint") ==
              config.grank.fingerprint &&
          get_u64(
              bytes,
              offset,
              "offline logical_n") ==
              config.grank.logical_n &&
          get_u64(
              bytes,
              offset,
              "offline padded_n") ==
              config.grank.padded_n &&
          get_u64(
              bytes,
              offset,
              "offline k") ==
              config.grank.k,
      "offline material binding");

  const auto package_length =
      get_u64(
          bytes,
          offset,
          "offline package length");

  require(
      package_length <=
          bytes.size() - offset,
      "offline party package truncated");

  const auto package_size =
      static_cast<std::size_t>(
          package_length);

  Bytes package_bytes(
      bytes.begin() +
          static_cast<std::ptrdiff_t>(offset),
      bytes.begin() +
          static_cast<std::ptrdiff_t>(
              offset + package_size));

  offset += package_size;

  ProtocolIIISecureCoreMaterial material;

  material.grank_package =
      deserialize_party_package(
          package_bytes,
          static_cast<int>(party));

  material.routing_material.session =
      config.grank.session;

  material.routing_material.fingerprint =
      config.grank.fingerprint;

  material.routing_material.logical_n =
      config.grank.logical_n;

  material.routing_material.k =
      config.grank.k;

  material.routing_material.rank_bits =
      config.grank.rank_bits;

  material.routing_material.party = party;

  material.routing_material.rank_mask_shares.resize(
      config.grank.logical_n);

  for (auto& share :
       material.routing_material.rank_mask_shares) {
    share =
        get_u64(
            bytes,
            offset,
            "offline rank-mask share");
  }

  material.unit_payload_shares.resize(
      config.grank.logical_n);

  for (auto& share :
       material.unit_payload_shares) {
    share =
        get_u64(
            bytes,
            offset,
            "offline unit-payload share");
  }

  material.combine_material.session =
      config.grank.session;

  material.combine_material.fingerprint =
      config.grank.fingerprint;

  material.combine_material.logical_n =
      config.grank.logical_n;

  material.combine_material.k =
      config.grank.k;

  material.combine_material.party = party;

  char* cursor =
      reinterpret_cast<char*>(
          const_cast<std::uint8_t*>(
              bytes.data() + offset));

  Dealer receiver(&cursor);

  try {
    material.routing_material.dpf_keys.reserve(
        config.grank.logical_n);

    for (std::uint32_t index = 0;
         index < config.grank.logical_n;
         ++index) {
      auto wire_key =
          receiver.recv_dpf_keypack(
              config.grank.rank_bits,
              kDpfPayloadBits);

      require(
          wire_key.s != nullptr,
          "offline DPF key has no seed blocks");

      DPFKeyPack owned_key(
          config.grank.rank_bits,
          kDpfPayloadBits);

      std::memcpy(
          owned_key.s,
          wire_key.s,
          static_cast<std::size_t>(
              config.grank.rank_bits + 1U) *
              sizeof(osuCrypto::block));

      owned_key.tLcw = wire_key.tLcw;
      owned_key.tRcw = wire_key.tRcw;
      owned_key.payload = wire_key.payload;

      material.routing_material.dpf_keys.emplace_back(
          std::move(owned_key));

      // The received seed pointer refers to the framed offline buffer.
      wire_key.s = nullptr;
    }

    const auto cells =
        cell_count(
            config.grank.logical_n,
            config.grank.k);

    material.combine_material
        .multiplication_materials.reserve(cells);

    for (std::size_t cell = 0;
         cell < cells;
         ++cell) {
      material.combine_material
          .multiplication_materials.push_back(
              receive_masked_mul_material(
                  receiver));
    }

    require(
        cursor ==
            reinterpret_cast<char*>(
                const_cast<std::uint8_t*>(
                    bytes.data() +
                    bytes.size())),
        "offline material trailing bytes");
  } catch (...) {
    delete static_cast<MemBuf*>(
        receiver.keyBuf);

    throw;
  }

  delete static_cast<MemBuf*>(
      receiver.keyBuf);

  return material;
}

Bytes encode_report(
    const ProtocolIIISecureCoreOutput& output,
    std::uint64_t offline_bytes) {
  Bytes bytes(
      output.xor_mask_shares.begin(),
      output.xor_mask_shares.end());

  const std::array<
      std::uint64_t,
      kReportMetricCount>
      metrics{{
          offline_bytes,

          output.metrics.grank.sent_bytes,
          output.metrics.grank.received_bytes,
          output.metrics.grank.comparison_edges,
          output.metrics.grank.raw_dcf_calls,

          output.metrics.routing.sent_bytes,
          output.metrics.routing.received_bytes,
          output.metrics.routing.dpf_keys,
          output.metrics.routing.eval_calls,

          output.metrics.combine.sent_bytes,
          output.metrics.combine.received_bytes,
          output.metrics.combine
              .multiplication_calls,
          output.metrics.combine
              .opened_masked_values,

          output.metrics.online_rounds,
      }};

  for (const auto metric : metrics) {
    put_u64(bytes, metric);
  }

  return bytes;
}

std::uint64_t parse_u64(
    const char* text,
    const char* message) {
  require(text != nullptr, message);

  std::size_t consumed = 0;

  const std::string value(text);
  const auto parsed =
      std::stoull(
          value,
          &consumed,
          10);

  require(
      consumed == value.size(),
      message);

  return parsed;
}

std::uint32_t parse_u32(
    const char* text,
    const char* message) {
  const auto value =
      parse_u64(text, message);

  require(
      value <=
          std::numeric_limits<std::uint32_t>::max(),
      message);

  return static_cast<std::uint32_t>(value);
}

int parse_fd(
    const char* text,
    const char* message) {
  const auto value =
      parse_u64(text, message);

  require(
      value <=
          static_cast<std::uint64_t>(
              std::numeric_limits<int>::max()),
      message);

  return static_cast<int>(value);
}

int run_party(
    int party,
    int offline_fd,
    int input_fd,
    int grank_fd,
    int routing_fd,
    int combine_fd,
    int result_fd,
    std::uint32_t logical_n,
    std::uint32_t k,
    std::uint64_t session,
    std::uint64_t fingerprint) {
  require(
      party == 0 || party == 1,
      "party must be 0 or 1");

  // Keep independent endpoint references alive until the final report has
  // been delivered. These guards do not exchange protocol messages and do
  // not add an online round.
  auto grank_lifetime =
      duplicate_lifetime_fd(
          grank_fd,
          "GRank lifetime dup failed");

  auto routing_lifetime =
      duplicate_lifetime_fd(
          routing_fd,
          "routing lifetime dup failed");

  auto combine_lifetime =
      duplicate_lifetime_fd(
          combine_fd,
          "combine lifetime dup failed");

  const auto config =
      make_config(
          static_cast<std::uint8_t>(party),
          logical_n,
          k,
          session,
          fingerprint);

  const auto offline_message =
      receive_message(
          offline_fd,
          kOfflineTimeoutMs);

  auto material =
      deserialize_offline_material(
          config,
          offline_message);

  const auto input_message =
      receive_message(
          input_fd,
          kOnlineTimeoutMs);

  const auto priority_key_shares =
      decode_words(
          input_message,
          config.grank.padded_n);

  const ProtocolIIISecureCoreFds fds{
      grank_fd,
      routing_fd,
      combine_fd,
  };

  const auto output =
      protocol_iii_secure_core_party(
          config,
          material,
          priority_key_shares,
          fds);

  require(
      output.metrics.online_rounds == 3U,
      "formal Protocol III round count");

  require(
      output.xor_mask_shares.size() ==
          logical_n,
      "formal Protocol III output length");

  const auto report =
      encode_report(
          output,
          static_cast<std::uint64_t>(
              offline_message.size() +
              sizeof(std::uint64_t)));

  send_message(
      result_fd,
      report,
      kOnlineTimeoutMs);

  // Process-lifecycle acknowledgement.
  //
  // result_fd is a controller channel, not a GRank, routing, or combine
  // protocol channel. Keeping the three duplicated online endpoints alive
  // until the controller has received both reports prevents one Party from
  // causing POLLHUP while its peer is consuming the final R3 frame.
  //
  // This acknowledgement is excluded from protocol communication metrics
  // and does not add a Protocol III online round.
  const auto acknowledgement =
      receive_message(
          result_fd,
          kOnlineTimeoutMs);

  require(

      acknowledgement.size() == 2U &&
          acknowledgement[0] ==
              static_cast<std::uint8_t>('O') &&
          acknowledgement[1] ==
              static_cast<std::uint8_t>('K'),
      "invalid controller acknowledgement");

  grank_lifetime.reset();
  routing_lifetime.reset();
  combine_lifetime.reset();

  return 0;}

void print_usage(
    const char* program) {
  std::cerr
      << "Usage:\n  "
      << program
      << " party PARTY OFFLINE_FD INPUT_FD"
      << " GRANK_FD ROUTING_FD COMBINE_FD RESULT_FD"
      << " LOGICAL_N K SESSION FINGERPRINT\n";
}

}  // namespace

int main(
    int argc,
    char** argv) {
  std::signal(SIGPIPE, SIG_IGN);

  try {
    if (argc == 2 &&
        std::string(argv[1]) == "--label") {
      std::cout
          << kProtocolLabel
          << '\n';

      return 0;
    }

    if (argc == 2 &&
        std::string(argv[1]) == "--help") {
      print_usage(argv[0]);
      return 0;
    }

    require(
        argc == 13 &&
            std::string(argv[1]) == "party",
        "invalid Protocol III role arguments");

    return run_party(
        static_cast<int>(
            parse_u32(
                argv[2],
                "invalid party")),
        parse_fd(
            argv[3],
            "invalid offline fd"),
        parse_fd(
            argv[4],
            "invalid input fd"),
        parse_fd(
            argv[5],
            "invalid GRank fd"),
        parse_fd(
            argv[6],
            "invalid routing fd"),
        parse_fd(
            argv[7],
            "invalid combine fd"),
        parse_fd(
            argv[8],
            "invalid result fd"),
        parse_u32(
            argv[9],
            "invalid logical_n"),
        parse_u32(
            argv[10],
            "invalid k"),
        parse_u64(
            argv[11],
            "invalid session"),
        parse_u64(
            argv[12],
            "invalid fingerprint"));
  } catch (const std::exception& error) {
    std::cerr
        << kProtocolLabel
        << ": "
        << error.what()
        << '\n';

    print_usage(
        argc > 0
            ? argv[0]
            : kProtocolLabel);

    return 1;
  }
}
