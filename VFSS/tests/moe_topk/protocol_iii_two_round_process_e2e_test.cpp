// TEST_ONLY independent fork+exec harness for the M5-E two-round core.
// P2, P0 and P1 are distinct exec'd address spaces. Clear input/oracle and
// result reconstruction remain exclusively in the controller role.
#include <moe_topk/protocol_i_priority_key.h>
#include <moe_topk/protocol_i_transport.h>
#include <moe_topk/protocol_iii_field_payload.h>
#include <moe_topk/protocol_iii_two_round_package.h>
#include <moe_topk/topk_oracle.h>

#include <FSS/prng.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <limits>
#include <poll.h>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
using namespace moe_topk;
using Bytes = std::vector<std::uint8_t>;
constexpr int kIoTimeoutMs = 6000;
constexpr std::size_t kMaxMessageBytes = 64U * 1024U * 1024U;

enum class Event : std::uint8_t {
  bundle_ready = 1, r1_prepared, r1_sent, r1_received, r1_immutable,
  r2_prepared, r2_sent, r2_received, r2_immutable, complete
};

void check(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}

void put_word(Bytes& out, std::uint64_t value, std::size_t count) {
  for (std::size_t i = count; i != 0U; --i)
    out.push_back(static_cast<std::uint8_t>(value >> (8U * (i - 1U))));
}

std::uint64_t get_word(const Bytes& bytes, std::size_t& offset,
                       std::size_t count) {
  check(offset <= bytes.size() && bytes.size() - offset >= count,
        "process message truncated");
  std::uint64_t result = 0;
  for (std::size_t i = 0; i < count; ++i)
    result = (result << 8U) | bytes[offset++];
  return result;
}

void put_field(Bytes& out, ProtocolIIIField value) {
  const auto part = value.serialize();
  out.insert(out.end(), part.begin(), part.end());
}

ProtocolIIIField get_field(const Bytes& bytes, std::size_t& offset) {
  check(offset <= bytes.size() && bytes.size() - offset >= 16U,
        "process field truncated");
  const Bytes part(bytes.begin() + offset, bytes.begin() + offset + 16U);
  offset += 16U;
  return ProtocolIIIField::deserialize(part);
}

void exact_io(int fd, void* data, std::size_t length, bool writing,
              int timeout_ms = kIoTimeoutMs) {
  auto* cursor = static_cast<std::uint8_t*>(data);
  const auto deadline = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(timeout_ms);
  while (length != 0U) {
    const auto now = std::chrono::steady_clock::now();
    check(now < deadline, "process IPC timeout");
    const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
        deadline - now).count();
    pollfd pfd{fd, static_cast<short>(writing ? POLLOUT : POLLIN), 0};
    const auto ready = ::poll(&pfd, 1, static_cast<int>(std::max<std::int64_t>(1, remaining)));
    if (ready < 0 && errno == EINTR) continue;
    check(ready > 0 && (pfd.revents & (POLLERR | POLLNVAL)) == 0,
          "process IPC poll/closed peer");
    const auto count = writing
        ? ::send(fd, cursor, length, MSG_NOSIGNAL)
        : ::read(fd, cursor, length);
    if (count < 0 && errno == EINTR) continue;
    check(count > 0, "process IPC EOF/I-O error");
    cursor += count;
    length -= static_cast<std::size_t>(count);
  }
}

void send_bytes(int fd, const Bytes& bytes) {
  check(bytes.size() <= kMaxMessageBytes, "process IPC message too large");
  Bytes length;
  put_word(length, bytes.size(), 8U);
  exact_io(fd, length.data(), length.size(), true);
  if (!bytes.empty()) exact_io(fd, const_cast<std::uint8_t*>(bytes.data()),
                                bytes.size(), true);
}

Bytes receive_bytes(int fd, std::size_t maximum = kMaxMessageBytes) {
  Bytes header(8U);
  exact_io(fd, header.data(), header.size(), false);
  std::size_t cursor = 0;
  const auto length = get_word(header, cursor, 8U);
  check(length <= maximum, "process IPC declared length too large");
  Bytes result(static_cast<std::size_t>(length));
  if (!result.empty()) exact_io(fd, result.data(), result.size(), false);
  return result;
}

class FdPool {
 public:
  ~FdPool() { close_except({}); }
  void make_pair(std::array<int, 2>& endpoints) {
    endpoints = {{-1, -1}};
    check(::socketpair(AF_UNIX, SOCK_STREAM, 0, endpoints.data()) == 0,
          "process socketpair");
    owned_.push_back(&endpoints[0]);
    owned_.push_back(&endpoints[1]);
  }
  void close_except(const std::vector<int>& keep) {
    for (auto* fd : owned_) {
      if (*fd >= 0 && std::find(keep.begin(), keep.end(), *fd) == keep.end()) {
        ::close(*fd);
        *fd = -1;
      }
    }
  }
 private:
  std::vector<int*> owned_;
};

class ChildSet {
 public:
  ~ChildSet() {
    for (auto child : children_) if (child > 0) ::kill(child, SIGTERM);
    for (auto child : children_) if (child > 0) {
      while (::waitpid(child, nullptr, 0) < 0 && errno == EINTR) {}
    }
  }
  void add(pid_t pid) { children_.push_back(pid); }
  int wait_status(pid_t pid, int timeout_ms = 10000) {
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);
    for (;;) {
      int status = 0;
      const auto result = ::waitpid(pid, &status, WNOHANG);
      if (result == pid) {
        for (auto& tracked : children_) if (tracked == pid) tracked = -1;
        return WIFEXITED(status) ? WEXITSTATUS(status) : 128;
      }
      if (result < 0 && errno == EINTR) continue;
      check(result == 0 && std::chrono::steady_clock::now() < deadline,
            "process child timeout/waitpid");
      (void)::poll(nullptr, 0, 10);
    }
  }
 private:
  std::vector<pid_t> children_;
};

std::string executable_path(const char* argv0) {
  std::array<char, 4096> path{};
  const auto count = ::readlink("/proc/self/exe", path.data(), path.size() - 1U);
  return count > 0 ? std::string(path.data(), static_cast<std::size_t>(count))
                   : std::string(argv0);
}

pid_t launch_role(const std::string& exe, const std::vector<std::string>& args,
                  FdPool& fds, ChildSet& children,
                  const std::vector<int>& keep) {
  const auto child = ::fork();
  check(child >= 0, "process fork");
  if (child == 0) {
    fds.close_except(keep);
    std::vector<char*> words;
    words.push_back(const_cast<char*>(exe.c_str()));
    for (const auto& arg : args) words.push_back(const_cast<char*>(arg.c_str()));
    words.push_back(nullptr);
    ::execv(exe.c_str(), words.data());
    ::_exit(127);
  }
  children.add(child);
  return child;
}

struct EventRecord {
  Event event;
  std::uint8_t party;
  std::uint32_t sequence;
  std::uint8_t ok;
};

void emit_event(int fd, std::uint8_t party, Event event,
                std::uint32_t& sequence, bool ok = true) {
  Bytes data{static_cast<std::uint8_t>(event), party,
             static_cast<std::uint8_t>(ok), 0U};
  put_word(data, sequence++, 4U);
  exact_io(fd, data.data(), data.size(), true);
}

EventRecord read_event(int fd) {
  Bytes data(8U);
  exact_io(fd, data.data(), data.size(), false);
  std::size_t offset = 4U;
  check(data[3] == 0U && data[0] >= 1U && data[0] <= 10U &&
            data[1] <= 1U && data[2] <= 1U,
        "process event frame");
  return {static_cast<Event>(data[0]), data[1],
          static_cast<std::uint32_t>(get_word(data, offset, 4U)), data[2]};
}

std::uint64_t digest(const Bytes& bytes) {
  std::uint64_t value = UINT64_C(1469598103934665603);
  for (const auto byte : bytes) {
    value ^= byte;
    value *= UINT64_C(1099511628211);
  }
  return value;
}

std::uint8_t width(std::uint32_t value) {
  std::uint8_t bits = 0;
  for (; value != 0U; value >>= 1U) ++bits;
  return bits;
}

std::uint32_t padded(std::uint32_t n) {
  std::uint32_t result = 2U;
  while (result < n) result <<= 1U;
  return result;
}

ProtocolIIITwoRoundConfig config(std::uint32_t n, std::uint32_t k,
                                 std::uint32_t target, std::uint64_t session,
                                 std::uint64_t fingerprint, std::uint8_t party) {
  ProtocolIIITwoRoundConfig c;
  c.session = session;
  c.fingerprint = fingerprint;
  c.logical_n = n;
  c.padded_n = padded(n);
  c.k = k;
  c.target_rank = target;
  c.comparison_bits = static_cast<std::uint8_t>(33U + width(c.padded_n - 1U));
  c.rank_bits = width(n - 1U);
  c.party = party;
  return c;
}

struct PublicRun {
  std::uint32_t n, k, target;
  std::uint64_t session, fingerprint, material_id;
};

PublicRun parse_public(int argc, char** argv, int offset) {
  check(argc >= offset + 6, "process public argument count");
  auto parse = [](const char* word) -> std::uint64_t {
    std::size_t consumed = 0;
    const auto value = std::stoull(word, &consumed);
    check(word[consumed] == '\0', "process numeric argument");
    return value;
  };
  const auto n = parse(argv[offset]);
  const auto k = parse(argv[offset + 1]);
  const auto target = parse(argv[offset + 2]);
  check(n >= 2U && n <= 1024U && k >= 1U && k <= n && target < k,
        "process public shape");
  return {static_cast<std::uint32_t>(n), static_cast<std::uint32_t>(k),
          static_cast<std::uint32_t>(target), parse(argv[offset + 3]),
          parse(argv[offset + 4]), parse(argv[offset + 5])};
}

std::vector<std::string> public_args(const PublicRun& p) {
  return {std::to_string(p.n), std::to_string(p.k),
          std::to_string(p.target), std::to_string(p.session),
          std::to_string(p.fingerprint), std::to_string(p.material_id)};
}

int parse_fd(const char* value) {
  std::size_t consumed = 0;
  const auto fd = std::stoi(value, &consumed);
  check(value[consumed] == '\0' && fd >= 0, "process descriptor argument");
  return fd;
}

void seed_fss(std::uint64_t seed) {
  for (int i = 0; i < 256; ++i)
    FSSConfig::prngs[i].SetSeed(osuCrypto::toBlock(
        seed, static_cast<std::uint64_t>(i)));
}

Bytes dealer_receipt(std::uint64_t material_id,
                     std::size_t bytes0, std::size_t bytes1) {
  Bytes out{'M','5','D','R',1U,0U,0U,0U};
  put_word(out, material_id, 8U);
  put_word(out, bytes0, 8U);
  put_word(out, bytes1, 8U);
  return out;
}

std::pair<std::uint64_t, std::uint64_t> decode_receipt(
    const Bytes& in, std::uint64_t expected_id) {
  check(in.size() == 32U && in[0] == 'M' && in[1] == '5' &&
            in[2] == 'D' && in[3] == 'R' && in[4] == 1U &&
            in[5] == 0U && in[6] == 0U && in[7] == 0U,
        "dealer receipt header");
  std::size_t offset = 8U;
  check(get_word(in, offset, 8U) == expected_id,
        "dealer material identity");
  const auto size0 = get_word(in, offset, 8U);
  const auto size1 = get_word(in, offset, 8U);
  return {size0, size1};
}

Bytes encode_input(const ProtocolIIITwoRoundConfig& c,
                   const std::vector<std::uint64_t>& keys,
                   const std::vector<ProtocolIIIField>& payloads) {
  check(keys.size() == c.padded_n && payloads.size() == c.logical_n,
        "test input share count");
  Bytes out{'M','5','I','N',1U,c.party,0U,0U};
  put_word(out, c.session, 8U);
  put_word(out, c.fingerprint, 8U);
  put_word(out, c.logical_n, 4U);
  put_word(out, c.padded_n, 4U);
  put_word(out, c.k, 4U);
  put_word(out, c.target_rank, 4U);
  put_word(out, c.comparison_bits, 1U);
  put_word(out, c.rank_bits, 1U);
  put_word(out, 0U, 2U);
  for (const auto value : keys) put_word(out, value, 8U);
  for (const auto value : payloads) put_field(out, value);
  return out;
}

struct InputShares {
  std::vector<std::uint64_t> keys;
  std::vector<ProtocolIIIField> payloads;
};

InputShares decode_input(const ProtocolIIITwoRoundConfig& c, const Bytes& in) {
  check(in.size() == 44U + static_cast<std::size_t>(c.padded_n) * 8U +
                         static_cast<std::size_t>(c.logical_n) * 16U &&
            in[0] == 'M' && in[1] == '5' && in[2] == 'I' && in[3] == 'N' &&
            in[4] == 1U && in[5] == c.party && in[6] == 0U && in[7] == 0U,
        "online input frame header/length");
  std::size_t offset = 8U;
  check(get_word(in, offset, 8U) == c.session &&
            get_word(in, offset, 8U) == c.fingerprint &&
            get_word(in, offset, 4U) == c.logical_n &&
            get_word(in, offset, 4U) == c.padded_n &&
            get_word(in, offset, 4U) == c.k &&
            get_word(in, offset, 4U) == c.target_rank &&
            get_word(in, offset, 1U) == c.comparison_bits &&
            get_word(in, offset, 1U) == c.rank_bits &&
            get_word(in, offset, 2U) == 0U,
        "online input frame binding");
  InputShares result;
  result.keys.reserve(c.padded_n);
  result.payloads.reserve(c.logical_n);
  const auto ring_mask = (UINT64_C(1) << c.comparison_bits) - 1U;
  for (std::uint32_t i = 0; i < c.padded_n; ++i) {
    const auto share = get_word(in, offset, 8U);
    check((share & ~ring_mask) == 0U, "online key share outside ring");
    result.keys.push_back(share);
  }
  for (std::uint32_t i = 0; i < c.logical_n; ++i)
    result.payloads.push_back(get_field(in, offset));
  check(offset == in.size(), "online input trailing bytes");
  return result;
}

Bytes encode_report(const ProtocolIIITwoRoundConfig& c, std::uint64_t id,
                    ProtocolIIIField selected,
                    const ProtocolIIITwoRoundMetrics& m,
                    std::uint64_t offline_bytes) {
  Bytes out{'M','5','R','P',1U,c.party,0U,0U};
  put_word(out, c.session, 8U);
  put_word(out, c.fingerprint, 8U);
  put_word(out, id, 8U);
  put_field(out, selected);
  for (const auto value : {m.round1_sent_bytes, m.round1_received_bytes,
                           m.round2_sent_bytes, m.round2_received_bytes,
                           m.round1_logical_bits, m.round2_logical_bits,
                           offline_bytes})
    put_word(out, value, 8U);
  return out;
}

struct PartyReport {
  ProtocolIIIField selected;
  ProtocolIIITwoRoundMetrics metrics;
  std::uint64_t offline_bytes = 0;
};

PartyReport decode_report(const ProtocolIIITwoRoundConfig& c,
                          std::uint64_t id, const Bytes& in) {
  check(in.size() == 104U && in[0] == 'M' && in[1] == '5' &&
            in[2] == 'R' && in[3] == 'P' && in[4] == 1U &&
            in[5] == c.party && in[6] == 0U && in[7] == 0U,
        "party report header");
  std::size_t offset = 8U;
  check(get_word(in, offset, 8U) == c.session &&
            get_word(in, offset, 8U) == c.fingerprint &&
            get_word(in, offset, 8U) == id,
        "party report binding");
  PartyReport result;
  result.selected = get_field(in, offset);
  result.metrics.round1_sent_bytes = get_word(in, offset, 8U);
  result.metrics.round1_received_bytes = get_word(in, offset, 8U);
  result.metrics.round2_sent_bytes = get_word(in, offset, 8U);
  result.metrics.round2_received_bytes = get_word(in, offset, 8U);
  result.metrics.round1_logical_bits = get_word(in, offset, 8U);
  result.metrics.round2_logical_bits = get_word(in, offset, 8U);
  result.offline_bytes = get_word(in, offset, 8U);
  check(offset == in.size(), "party report trailing bytes");
  return result;
}

class OwnedFd {
 public:
  explicit OwnedFd(int fd) : fd_(fd) { check(fd_ >= 0, "dup online descriptor"); }
  ~OwnedFd() { if (fd_ >= 0) ::close(fd_); }
  OwnedFd(const OwnedFd&) = delete;
  OwnedFd& operator=(const OwnedFd&) = delete;
 private:
  int fd_;
};

int dealer_main(const PublicRun& p, std::uint64_t seed, int offline0,
                int offline1, int receipt_fd, const std::string& fault) {
  try {
    seed_fss(seed ^ UINT64_C(0x4445414c4552));
    osuCrypto::PRNG generator(osuCrypto::toBlock(
        seed ^ UINT64_C(0x515250365151), p.fingerprint));
    auto c = config(p.n, p.k, p.target, p.session, p.fingerprint, 0U);
    auto pair = protocol_iii_two_round_preprocess(c, generator);
    auto bytes0 = protocol_iii_two_round_serialize_bundle(
        c, pair.first, p.material_id);
    c.party = 1U;
    auto bytes1 = protocol_iii_two_round_serialize_bundle(
        c, pair.second, p.material_id);
    if (fault == "bundle_swap") {
      std::swap(bytes0, bytes1);
    } else if (fault == "bundle_session") {
      bytes0[15] ^= 1U;
    } else if (fault == "bundle_fingerprint") {
      bytes0[23] ^= 1U;
    } else if (fault == "bundle_target") {
      bytes0[47] ^= 1U;
    } else if (fault == "bundle_field") {
      bytes0[5] ^= 1U;
    } else if (fault == "bundle_truncated") {
      bytes0.pop_back();
    } else if (fault == "bundle_extra") {
      bytes0.push_back(0U);
    } else if (fault == "bundle_material_id") {
      bytes0[31] ^= 1U;
    } else if (fault == "bundle_n") {
      bytes0[35] ^= 1U;
    } else if (fault == "bundle_padded") {
      bytes0[39] ^= 1U;
    } else if (fault == "bundle_comparison_bits") {
      bytes0[48] ^= 1U;
    } else if (fault == "bundle_rank_bits") {
      bytes0[49] ^= 1U;
    } else if (fault == "bundle_count" || fault == "bundle_dpf_count" ||
               fault == "bundle_mul_count") {
      const auto package_size =
          (std::uint32_t{bytes0[52]} << 24U) |
          (std::uint32_t{bytes0[53]} << 16U) |
          (std::uint32_t{bytes0[54]} << 8U) | bytes0[55];
      std::size_t offset = 56U + package_size;
      if (fault == "bundle_count") {
        bytes0[offset + 3U] ^= 1U;
      } else {
        offset += 4U + 8U * p.n;
        if (fault == "bundle_dpf_count") {
          bytes0[offset + 3U] ^= 1U;
        } else {
          offset += 4U;
          for (std::uint32_t i = 0; i < p.n; ++i) {
            offset += 16U;
            const auto key_length =
                (std::uint32_t{bytes0[offset]} << 24U) |
                (std::uint32_t{bytes0[offset + 1U]} << 16U) |
                (std::uint32_t{bytes0[offset + 2U]} << 8U) |
                bytes0[offset + 3U];
            offset += 4U + key_length;
          }
          bytes0[offset + 3U] ^= 1U;
        }
      }
    }
    send_bytes(offline0, bytes0);
    send_bytes(offline1, bytes1);
    send_bytes(receipt_fd,
               dealer_receipt(p.material_id, bytes0.size(), bytes1.size()));
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "M5-F P2: " << e.what() << '\n';
    return 1;
  }
}

void send_partial(int fd) {
  const std::array<std::uint8_t, 9> bytes{{'M','2','T','F',1U,0U,0U,0U,0U}};
  check(::send(fd, bytes.data(), bytes.size(), MSG_NOSIGNAL) ==
            static_cast<ssize_t>(bytes.size()), "fault partial send");
  ::shutdown(fd, SHUT_WR);
  throw std::runtime_error("fault injected partial protocol frame");
}

int party_main(const PublicRun& p, std::uint8_t party, int offline_fd,
               int input_fd, int r1_fd, int r2_fd, int result_fd,
               int event_fd, const std::string& fault) {
  try {
    const auto c = config(p.n, p.k, p.target, p.session, p.fingerprint, party);
    std::uint32_t event_sequence = 0;
    const auto offline_bytes = receive_bytes(offline_fd);
    auto bundle = protocol_iii_two_round_deserialize_bundle(
        c, p.material_id, offline_bytes);
    emit_event(event_fd, party, Event::bundle_ready, event_sequence);
    // No online input is read until P2 has exited and the controller releases
    // the party-local input frame on this separate socket.
    auto input = decode_input(c, receive_bytes(input_fd, 44U +
        static_cast<std::size_t>(c.padded_n) * 8U +
        static_cast<std::size_t>(c.logical_n) * 16U));
    ProtocolIIITwoRoundParty state(c, std::move(bundle.material),
                                   std::move(input.keys),
                                   std::move(input.payloads));
    // A duplicate keeps a finished party's endpoint alive while its peer
    // drains the final frame. Controller result ACK releases both only after
    // both protocol outputs already exist; this is TEST_ONLY, not R3.
    OwnedFd hold_r1(::dup(r1_fd));
    OwnedFd hold_r2(::dup(r2_fd));
    ProtocolIFramedChannel r1(r1_fd,
        {c.session, c.fingerprint, c.logical_n, c.k, c.comparison_bits,
         c.party, static_cast<std::uint8_t>(1U - c.party), 6U, 1U},
        kIoTimeoutMs);
    ProtocolIFramedChannel r2(r2_fd,
        {c.session, c.fingerprint, c.logical_n, c.k, c.comparison_bits,
         c.party, static_cast<std::uint8_t>(1U - c.party), 6U, 2U},
        kIoTimeoutMs);
    ProtocolIIITwoRoundMetrics metrics;
    metrics.round1_logical_bits = static_cast<std::uint64_t>(c.logical_n) *
        (c.comparison_bits + 254U);
    metrics.round2_logical_bits = static_cast<std::uint64_t>(c.logical_n) *
        (c.rank_bits + 127U);
    auto outbound1 = state.prepare_round1();
    const auto r1_digest = digest(outbound1);
    emit_event(event_fd, party, Event::r1_prepared, event_sequence);
    if (fault == "r1_close" && party == 0U) {
      ::shutdown(r1_fd, SHUT_RDWR);
      throw std::runtime_error("fault injected early R1 close");
    }
    if (fault == "r1_truncated" && party == 0U) send_partial(r1_fd);
    if (fault == "r1_wrong_round" && party == 0U) outbound1[5] ^= 1U;
    Bytes inbound1;
    if (party == 0U) {
      r1.send(outbound1);
      emit_event(event_fd, party, Event::r1_sent, event_sequence);
      inbound1 = r1.receive();
      emit_event(event_fd, party, Event::r1_received, event_sequence);
    } else {
      inbound1 = r1.receive();
      emit_event(event_fd, party, Event::r1_received, event_sequence);
      check(digest(outbound1) == r1_digest, "R1 outbound mutated after receive");
      emit_event(event_fd, party, Event::r1_immutable, event_sequence);
      r1.send(outbound1);
      emit_event(event_fd, party, Event::r1_sent, event_sequence);
    }
    if (party == 0U) {
      check(digest(outbound1) == r1_digest, "R1 outbound mutated");
      emit_event(event_fd, party, Event::r1_immutable, event_sequence);
    }
    state.consume_round1(inbound1);
    metrics.round1_sent_bytes = r1.sent_bytes();
    metrics.round1_received_bytes = r1.received_bytes();

    auto outbound2 = state.prepare_round2();
    const auto r2_digest = digest(outbound2);
    emit_event(event_fd, party, Event::r2_prepared, event_sequence);
    if (fault == "r2_close" && party == 1U) {
      ::shutdown(r2_fd, SHUT_RDWR);
      throw std::runtime_error("fault injected early R2 close");
    }
    if (fault == "r2_truncated" && party == 1U) send_partial(r2_fd);
    Bytes inbound2;
    if (party == 0U) {
      r2.send(outbound2);
      emit_event(event_fd, party, Event::r2_sent, event_sequence);
      inbound2 = r2.receive();
      emit_event(event_fd, party, Event::r2_received, event_sequence);
    } else {
      inbound2 = r2.receive();
      emit_event(event_fd, party, Event::r2_received, event_sequence);
      check(digest(outbound2) == r2_digest, "R2 outbound mutated after receive");
      emit_event(event_fd, party, Event::r2_immutable, event_sequence);
      r2.send(outbound2);
      emit_event(event_fd, party, Event::r2_sent, event_sequence);
    }
    if (party == 0U) {
      check(digest(outbound2) == r2_digest, "R2 outbound mutated");
      emit_event(event_fd, party, Event::r2_immutable, event_sequence);
    }
    const auto selected = state.consume_round2(inbound2);
    metrics.round2_sent_bytes = r2.sent_bytes();
    metrics.round2_received_bytes = r2.received_bytes();
    emit_event(event_fd, party, Event::complete, event_sequence);
    send_bytes(result_fd, encode_report(c, p.material_id, selected, metrics,
                                        offline_bytes.size() + 8U));
    std::uint8_t release = 0;
    exact_io(result_fd, &release, 1U, false);
    check(release == 1U, "result-channel lifetime release");
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "M5-F P" << int(party) << ": " << e.what() << '\n';
    return 1;
  }
}


struct CaseData {
  std::vector<std::uint32_t> scores;
  std::vector<std::uint64_t> payloads;
};

CaseData make_case(std::uint32_t n, std::uint32_t variant,
                   std::mt19937_64& random) {
  CaseData out;
  out.scores.resize(n);
  out.payloads.resize(n);
  for (std::uint32_t i = 0; i < n; ++i) {
    switch (variant) {
      case 0: out.scores[i] = (n - i) * 4096U; break;
      case 1: out.scores[i] = i * 4096U; break;
      case 2: out.scores[i] = 7U; break;
      case 3: out.scores[i] = (i % 3U == 0U) ? 10U : 20U; break;
      case 4: out.scores[i] = (i % 2U == 0U) ? 0U : UINT32_MAX; break;
      case 5: out.scores[i] = (i % 3U == 0U) ? UINT32_C(0x80000000) :
          ((i % 3U == 1U) ? UINT32_C(0x7fffffff) : 0U); break;
      default: out.scores[i] = static_cast<std::uint32_t>(random()); break;
    }
    switch ((variant + i) % 5U) {
      case 0: out.payloads[i] = 0U; break;
      case 1: out.payloads[i] = 1U; break;
      case 2: out.payloads[i] = UINT64_MAX; break;
      case 3: out.payloads[i] = 7U; break;
      default: out.payloads[i] = random(); break;
    }
  }
  return out;
}

struct Sockets {
  std::array<int, 2> offline0{}, offline1{}, input0{}, input1{};
  std::array<int, 2> r1{}, r2{}, result0{}, result1{};
  std::array<int, 2> event0{}, event1{}, receipt{};
  // Destroy the pool first, while descriptor storage still has lifetime.
  FdPool pool;
  Sockets() {
    pool.make_pair(offline0); pool.make_pair(offline1);
    pool.make_pair(input0); pool.make_pair(input1);
    pool.make_pair(r1); pool.make_pair(r2);
    pool.make_pair(result0); pool.make_pair(result1);
    pool.make_pair(event0); pool.make_pair(event1);
    pool.make_pair(receipt);
  }
};

struct Running {
  Sockets sockets;
  ChildSet children;
  pid_t p0 = -1, p1 = -1, p2 = -1;
};

std::vector<std::string> role_args(const char* role, const PublicRun& p,
                                   std::initializer_list<std::uint64_t> suffix,
                                   const std::string& fault) {
  std::vector<std::string> out{role};
  auto common = public_args(p);
  out.insert(out.end(), common.begin(), common.end());
  for (const auto value : suffix) out.push_back(std::to_string(value));
  out.push_back(fault);
  return out;
}

void start_roles(const std::string& exe, const PublicRun& p,
                 std::uint64_t dealer_seed, const std::string& fault,
                 Running& run) {
  auto& s = run.sockets;
  run.p2 = launch_role(exe,
      role_args("--dealer", p, {dealer_seed,
          static_cast<std::uint64_t>(s.offline0[0]),
          static_cast<std::uint64_t>(s.offline1[0]),
          static_cast<std::uint64_t>(s.receipt[1])}, fault),
      s.pool, run.children, {s.offline0[0],s.offline1[0],s.receipt[1]});
  run.p0 = launch_role(exe,
      role_args("--party", p, {0U, static_cast<std::uint64_t>(s.offline0[1]),
          static_cast<std::uint64_t>(s.input0[1]),
          static_cast<std::uint64_t>(s.r1[0]),
          static_cast<std::uint64_t>(s.r2[0]),
          static_cast<std::uint64_t>(s.result0[1]),
          static_cast<std::uint64_t>(s.event0[1])}, fault),
      s.pool, run.children,
      {s.offline0[1],s.input0[1],s.r1[0],s.r2[0],s.result0[1],s.event0[1]});
  run.p1 = launch_role(exe,
      role_args("--party", p, {1U, static_cast<std::uint64_t>(s.offline1[1]),
          static_cast<std::uint64_t>(s.input1[1]),
          static_cast<std::uint64_t>(s.r1[1]),
          static_cast<std::uint64_t>(s.r2[1]),
          static_cast<std::uint64_t>(s.result1[1]),
          static_cast<std::uint64_t>(s.event1[1])}, fault),
      s.pool, run.children,
      {s.offline1[1],s.input1[1],s.r1[1],s.r2[1],s.result1[1],s.event1[1]});
  s.pool.close_except({s.input0[0],s.input1[0],s.result0[0],s.result1[0],
                       s.event0[0],s.event1[0],s.receipt[0]});
}

void check_event(const EventRecord& record, Event event, std::uint8_t party,
                 std::uint32_t sequence) {
  check(record.event == event && record.party == party &&
        record.sequence == sequence && record.ok == 1U,
        "process causal event order/immutability");
}

// Each party has one complete, fixed event sequence. P1 intentionally reads
// before its send syscall but sends precisely its pre-receive frozen buffer.
void check_events(int fd, std::uint8_t party) {
  const std::array<Event, 10> p0{{
      Event::bundle_ready, Event::r1_prepared, Event::r1_sent,
      Event::r1_received, Event::r1_immutable, Event::r2_prepared,
      Event::r2_sent, Event::r2_received, Event::r2_immutable,
      Event::complete}};
  const std::array<Event, 10> p1{{
      Event::bundle_ready, Event::r1_prepared, Event::r1_received,
      Event::r1_immutable, Event::r1_sent, Event::r2_prepared,
      Event::r2_received, Event::r2_immutable, Event::r2_sent,
      Event::complete}};
  const auto& expected = party == 0U ? p0 : p1;
  // bundle_ready was consumed before online input release.
  for (std::uint32_t i = 1; i < expected.size(); ++i)
    check_event(read_event(fd), expected[i], party, i);
  std::uint8_t trailing = 0;
  check(::read(fd, &trailing, 1U) == 0,
        "unexpected post-completion process event");
}

struct InputPair {
  Bytes p0, p1;
};

InputPair split_test_input(const PublicRun& p, const CaseData& data,
                           std::uint64_t seed) {
  auto c0 = config(p.n,p.k,p.target,p.session,p.fingerprint,0U);
  auto c1 = c0; c1.party = 1U;
  std::mt19937_64 random(seed);
  osuCrypto::PRNG field_random(osuCrypto::toBlock(seed, p.session));
  const auto mask = (UINT64_C(1) << c0.comparison_bits) - 1U;
  std::vector<std::uint64_t> keys0(c0.padded_n), keys1(c0.padded_n);
  std::vector<ProtocolIIIField> payloads0(p.n), payloads1(p.n);
  for (std::uint32_t i = 0; i < c0.padded_n; ++i) {
    const auto score = i < p.n ? data.scores[i] : UINT32_C(0x80000000);
    const auto key = protocol_i_priority_key(score,i,c0.padded_n).value;
    keys0[i] = random() & mask;
    keys1[i] = (key - keys0[i]) & mask;
    if (i < p.n) {
      const auto encoded = protocol_iii_pack_key_payload_nonzero(
          key,c0.comparison_bits,data.payloads[i]);
      const auto shares = protocol_iii_split_field_element(encoded,field_random);
      payloads0[i] = shares.first;
      payloads1[i] = shares.second;
    }
  }
  return {encode_input(c0,keys0,payloads0),
          encode_input(c1,keys1,payloads1)};
}

struct Sample {
  PublicRun public_run;
  PartyReport p0, p1;
  std::uint64_t dealer_p0 = 0, dealer_p1 = 0;
};

Sample run_success(const std::string& exe, const PublicRun& p,
                   const CaseData& data, std::uint64_t seed) {
  Running run;
  start_roles(exe,p,seed,"none",run);
  const auto dealer_sizes = decode_receipt(
      receive_bytes(run.sockets.receipt[0],32U),p.material_id);
  check(run.children.wait_status(run.p2) == 0,
        "P2 must exit successfully before online input release");
  check_event(read_event(run.sockets.event0[0]),Event::bundle_ready,0U,0U);
  check_event(read_event(run.sockets.event1[0]),Event::bundle_ready,1U,0U);
  // TEST_ONLY input generation and release. P2 is already reaped; its
  // process had no input socket and never received clear or shared inputs.
  const auto inputs = split_test_input(p,data,seed ^ UINT64_C(0x91571591));
  send_bytes(run.sockets.input0[0],inputs.p0);
  send_bytes(run.sockets.input1[0],inputs.p1);
  const auto c0 = config(p.n,p.k,p.target,p.session,p.fingerprint,0U);
  const auto c1 = config(p.n,p.k,p.target,p.session,p.fingerprint,1U);
  const auto report0 = decode_report(c0,p.material_id,
                                    receive_bytes(run.sockets.result0[0],104U));
  const auto report1 = decode_report(c1,p.material_id,
                                    receive_bytes(run.sockets.result1[0],104U));
  std::uint8_t release = 1U;
  exact_io(run.sockets.result0[0],&release,1U,true);
  exact_io(run.sockets.result1[0],&release,1U,true);
  check(run.children.wait_status(run.p0) == 0 &&
        run.children.wait_status(run.p1) == 0,
        "online party exit status");
  check_events(run.sockets.event0[0],0U);
  check_events(run.sockets.event1[0],1U);
  check(report0.offline_bytes == dealer_sizes.first + 8U &&
        report1.offline_bytes == dealer_sizes.second + 8U,
        "dealer/party offline byte accounting");
  const auto ranks = stable_ranks_cmpagg(data.scores);
  auto index = std::uint32_t{0};
  while (index < p.n && ranks[index] != p.target) ++index;
  check(index < p.n, "clear oracle selected rank");
  const auto actual = ProtocolIIIField::add(report0.selected,report1.selected);
  const auto decoded = protocol_iii_unpack_key_payload_nonzero(
      actual,c0.comparison_bits);
  check(decoded.first == protocol_i_priority_key(
            data.scores[index],index,c0.padded_n).value &&
        decoded.second == data.payloads[index],
        "process Fselect result differs from clear oracle");
  const auto expected_r1 = 92U + 40U * p.n;
  const auto expected_r2 = 92U + 24U * p.n;
  const auto expected_r1_bits = static_cast<std::uint64_t>(p.n) *
      (c0.comparison_bits + 254U);
  const auto expected_r2_bits = static_cast<std::uint64_t>(p.n) *
      (c0.rank_bits + 127U);
  check(report0.metrics.round1_sent_bytes == expected_r1 &&
        report1.metrics.round1_sent_bytes == expected_r1 &&
        report0.metrics.round1_received_bytes == expected_r1 &&
        report1.metrics.round1_received_bytes == expected_r1 &&
        report0.metrics.round2_sent_bytes == expected_r2 &&
        report1.metrics.round2_sent_bytes == expected_r2 &&
        report0.metrics.round2_received_bytes == expected_r2 &&
        report1.metrics.round2_received_bytes == expected_r2 &&
        report0.metrics.round1_logical_bits == expected_r1_bits &&
        report1.metrics.round1_logical_bits == expected_r1_bits &&
        report0.metrics.round2_logical_bits == expected_r2_bits &&
        report1.metrics.round2_logical_bits == expected_r2_bits,
        "process online communication accounting");
  return {p,report0,report1,dealer_sizes.first,dealer_sizes.second};
}

// Faults are executed in the same fork+exec architecture. They must lead to
// a peer-visible decode/transport failure; the controller never retries the
// consumed material or releases online input for a malformed offline bundle.
void run_failure(const std::string& exe, PublicRun p, const CaseData& data,
                 std::uint64_t seed, const std::string& fault) {
  Running run;
  start_roles(exe,p,seed,fault,run);
  (void)decode_receipt(receive_bytes(run.sockets.receipt[0],32U),p.material_id);
  check(run.children.wait_status(run.p2) == 0,"fault dealer exit");
  const bool offline = fault.rfind("bundle_",0) == 0;
  if (offline) {
    // P0 must reject before the controller releases either input share.
    bool p0_rejected = false;
    try { (void)read_event(run.sockets.event0[0]); }
    catch (const std::exception&) { p0_rejected = true; }
    check(p0_rejected && run.children.wait_status(run.p0) != 0,
          "malformed bundle accepted before online release");
    return;
  }
  check_event(read_event(run.sockets.event0[0]),Event::bundle_ready,0U,0U);
  check_event(read_event(run.sockets.event1[0]),Event::bundle_ready,1U,0U);
  const auto input = split_test_input(p,data,seed ^ UINT64_C(0x91571591));
  send_bytes(run.sockets.input0[0],input.p0);
  send_bytes(run.sockets.input1[0],input.p1);
  bool reported0 = false;
  bool reported1 = false;
  try {
    (void)receive_bytes(run.sockets.result0[0],104U);
    reported0 = true;
  } catch (const std::exception&) {}
  try {
    (void)receive_bytes(run.sockets.result1[0],104U);
    reported1 = true;
  } catch (const std::exception&) {}
  check(!(reported0 && reported1),
        "faulted online exchange produced both outputs");
  // A party that already finished may be waiting on the test-only lifetime
  // ACK. Release it before collecting exit statuses even in a failure case.
  std::uint8_t release = 1U;
  if (reported0) exact_io(run.sockets.result0[0],&release,1U,true);
  if (reported1) exact_io(run.sockets.result1[0],&release,1U,true);
  const auto status0 = run.children.wait_status(run.p0);
  const auto status1 = run.children.wait_status(run.p1);
  check(status0 != 0 || status1 != 0,"faulted online parties both succeeded");
}

int controller_main(const std::string& exe) {
  std::mt19937_64 random(UINT64_C(0x5f555345c0ffee));
  std::set<std::uint64_t> issued_material;
  std::size_t completed = 0;
  std::uint64_t case_number = 1U;
  Sample sample;
  for (const auto n : {2U,3U,4U,5U,8U}) {
    for (const auto variant : {0U,2U,3U,4U}) {
      const auto data = make_case(n,variant,random);
      const auto target = variant == 0U ? 0U :
          (variant == 2U ? n-1U : n/2U);
      const auto k = variant == 0U ? 1U :
          (variant == 3U ? n/2U + 1U : n);
      const auto p = PublicRun{n,k,target,
          UINT64_C(0x5f51000000000000) + case_number,
          UINT64_C(0x5f52000000000000) + case_number,
          UINT64_C(0x5f53000000000000) + case_number};
      check(issued_material.insert(p.material_id).second,
            "controller replayed material identity");
      sample = run_success(exe,p,data,random());
      ++completed;
      ++case_number;
    }
  }
  for (const auto n : {3U,5U}) {
    const auto data = make_case(n,6U,random);
    const auto p = PublicRun{n,n,n/2U,
        UINT64_C(0x5f51000000000000) + case_number,
        UINT64_C(0x5f52000000000000) + case_number,
        UINT64_C(0x5f53000000000000) + case_number};
    check(issued_material.insert(p.material_id).second,
          "controller replayed material identity");
    sample = run_success(exe,p,data,random());
    ++completed;
    ++case_number;
  }
  // All-equal n=5 explicitly exercises original-index tie break at 0/2/4.
  for (const auto target : {0U,2U,4U}) {
    const auto data = make_case(5U,2U,random);
    const auto p = PublicRun{5U,5U,target,
        UINT64_C(0x5f51000000000000) + case_number,
        UINT64_C(0x5f52000000000000) + case_number,
        UINT64_C(0x5f53000000000000) + case_number};
    check(issued_material.insert(p.material_id).second,
          "controller replayed material identity");
    sample = run_success(exe,p,data,random());
    ++completed;
    ++case_number;
  }
  const auto failure_data = make_case(3U,3U,random);
  std::size_t failures = 0;
  for (const std::string fault : {"bundle_swap","bundle_session",
       "bundle_fingerprint","bundle_target","bundle_field",
       "bundle_truncated","bundle_extra","bundle_count",
       "bundle_material_id","bundle_n","bundle_padded",
       "bundle_comparison_bits","bundle_rank_bits",
       "bundle_dpf_count","bundle_mul_count",
       "r1_close","r1_truncated","r1_wrong_round",
       "r2_close","r2_truncated"}) {
    const auto p = PublicRun{3U,3U,1U,
        UINT64_C(0x5f51000000000000) + case_number,
        UINT64_C(0x5f52000000000000) + case_number,
        UINT64_C(0x5f53000000000000) + case_number};
    check(issued_material.insert(p.material_id).second,
          "controller replayed failure material identity");
    run_failure(exe,p,failure_data,random(),fault);
    ++failures;
    ++case_number;
  }
  // The controller refuses a second launch with any already issued identity.
  // This is process-local uniqueness, not persistent restart protection.
  check(!issued_material.insert(sample.public_run.material_id).second,
        "controller accepted replayed offline material identity");
  std::cout << "M5-F PROCESS_E2E_PASS=" << completed
            << " FAILURE_INJECTION_PASS=" << failures << '\n';
  std::cout << "SAMPLE_N=" << sample.public_run.n
            << " P0_OFFLINE_BUNDLE_BYTES=" << sample.dealer_p0
            << " P1_OFFLINE_BUNDLE_BYTES=" << sample.dealer_p1
            << " P0_TO_P1_R1_BYTES=" << sample.p0.metrics.round1_sent_bytes
            << " P1_TO_P0_R1_BYTES=" << sample.p1.metrics.round1_sent_bytes
            << " P0_TO_P1_R2_BYTES=" << sample.p0.metrics.round2_sent_bytes
            << " P1_TO_P0_R2_BYTES=" << sample.p1.metrics.round2_sent_bytes
            << " ROUND1_LOGICAL_BITS=" <<
                 sample.p0.metrics.round1_logical_bits +
                 sample.p1.metrics.round1_logical_bits
            << " ROUND2_LOGICAL_BITS=" <<
                 sample.p0.metrics.round2_logical_bits +
                 sample.p1.metrics.round2_logical_bits << '\n';
  return 0;
}

} // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1) return controller_main(executable_path(argv[0]));
    check(argc >= 2,"process role");
    const std::string role(argv[1]);
    const auto p = parse_public(argc,argv,2);
    if (role == "--dealer") {
      check(argc == 13,"dealer argument count");
      return dealer_main(p,std::stoull(argv[8]),
          parse_fd(argv[9]),parse_fd(argv[10]),parse_fd(argv[11]),argv[12]);
    }
    if (role == "--party") {
      check(argc == 16,"party argument count");
      const auto party = static_cast<std::uint8_t>(std::stoul(argv[8]));
      check(party <= 1U,"party id");
      return party_main(p,party,parse_fd(argv[9]),parse_fd(argv[10]),
          parse_fd(argv[11]),parse_fd(argv[12]),parse_fd(argv[13]),
          parse_fd(argv[14]),argv[15]);
    }
    throw std::runtime_error("unknown process role");
  } catch (const std::exception& e) {
    std::cerr << "M5-F controller/role: " << e.what() << '\n';
    return 1;
  }
}
