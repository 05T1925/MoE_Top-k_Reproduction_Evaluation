#include <moe_topk/protocol_i_chosen_ot.h>

#include <emp-ot/emp-ot.h>

#include <array>
#include <chrono>
#include <cerrno>
#include <climits>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include <unistd.h>

namespace moe_topk {
namespace {

constexpr std::uint32_t kMagic = UINT32_C(0x4d324f54);  // M2OT
constexpr std::uint16_t kVersion = 1;
constexpr std::uint8_t kSenderRole = 1;
constexpr std::uint8_t kReceiverRole = 2;
constexpr std::size_t kPreambleBytes = 48;

using Clock = std::chrono::steady_clock;

[[noreturn]] void fail(const char* message) { throw std::runtime_error(message); }

void put_u16(std::array<std::uint8_t, kPreambleBytes>& bytes, std::size_t offset,
             std::uint16_t value) {
  bytes[offset] = static_cast<std::uint8_t>(value >> 8);
  bytes[offset + 1] = static_cast<std::uint8_t>(value);
}
void put_u32(std::array<std::uint8_t, kPreambleBytes>& bytes, std::size_t offset,
             std::uint32_t value) {
  for (int shift = 24; shift >= 0; shift -= 8) bytes[offset++] = static_cast<std::uint8_t>(value >> shift);
}
void put_u64(std::array<std::uint8_t, kPreambleBytes>& bytes, std::size_t offset,
             std::uint64_t value) {
  for (int shift = 56; shift >= 0; shift -= 8) bytes[offset++] = static_cast<std::uint8_t>(value >> shift);
}
std::uint16_t get_u16(const std::array<std::uint8_t, kPreambleBytes>& bytes, std::size_t offset) {
  return (static_cast<std::uint16_t>(bytes[offset]) << 8) | bytes[offset + 1];
}
std::uint32_t get_u32(const std::array<std::uint8_t, kPreambleBytes>& bytes, std::size_t offset) {
  std::uint32_t value = 0;
  for (int i = 0; i != 4; ++i) value = (value << 8) | bytes[offset + i];
  return value;
}
std::uint64_t get_u64(const std::array<std::uint8_t, kPreambleBytes>& bytes, std::size_t offset) {
  std::uint64_t value = 0;
  for (int i = 0; i != 8; ++i) value = (value << 8) | bytes[offset + i];
  return value;
}

class EmpBoundedFdIO final : public emp::IOChannel {
 public:
  EmpBoundedFdIO(int fd, Clock::time_point deadline) : fd_(fd), deadline_(deadline) {
    if (fd_ < 0) fail("chosen OT invalid connected fd");
  }

  void send_data_internal(const void* data, int64_t count) override {
    if (count < 0) fail("chosen OT negative send");
    exact(const_cast<void*>(data), static_cast<std::size_t>(count), true);
  }
  void recv_data_internal(void* data, int64_t count) override {
    if (count < 0) fail("chosen OT negative receive");
    exact(data, static_cast<std::size_t>(count), false);
  }
  void raw_send(const void* data, std::size_t count) { exact(const_cast<void*>(data), count, true); }
  void raw_receive(void* data, std::size_t count) { exact(data, count, false); }

 private:
  int fd_;
  Clock::time_point deadline_;

  void wait(short requested) {
    pollfd descriptor{fd_, requested, 0};
    for (;;) {
      // poll() may be interrupted arbitrarily often.  Recompute against the
      // absolute deadline before every retry rather than restarting a timeout.
      const auto now = Clock::now();
      if (now >= deadline_) fail("chosen OT deadline exceeded");
      const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline_ - now).count();
      const auto timeout = static_cast<int>(std::min<std::int64_t>(remaining + 1, INT_MAX));
      const int result = ::poll(&descriptor, 1, timeout);
      if (result < 0 && errno == EINTR) continue;
      if (result == 0) fail("chosen OT deadline exceeded");
      if (result < 0) fail("chosen OT poll failure");
      if ((descriptor.revents & (POLLERR | POLLNVAL)) != 0) fail("chosen OT peer failure");
      if ((descriptor.revents & requested) != 0) return;
      if ((descriptor.revents & POLLHUP) != 0) fail("chosen OT peer failure");
      fail("chosen OT poll event");
    }
  }
  void exact(void* data, std::size_t count, bool sending) {
    auto* cursor = static_cast<std::uint8_t*>(data);
    while (count != 0) {
      wait(sending ? POLLOUT : POLLIN);
      const auto result = sending ? ::write(fd_, cursor, count) : ::read(fd_, cursor, count);
      if (result < 0 && errno == EINTR) continue;
      if (result < 0) fail("chosen OT fd I/O failure");
      if (result == 0) fail("chosen OT unexpected EOF");
      cursor += result;
      count -= static_cast<std::size_t>(result);
    }
  }
};

void validate_config(const ProtocolIChosenOtConfig& config) {
  if (config.item_count == 0 || config.timeout_ms <= 0) fail("chosen OT invalid config");
}

std::array<std::uint8_t, kPreambleBytes> preamble(const ProtocolIChosenOtConfig& config,
                                                   std::uint8_t role) {
  std::array<std::uint8_t, kPreambleBytes> bytes{};
  put_u32(bytes, 0, kMagic);
  put_u16(bytes, 4, kVersion);
  bytes[6] = role;
  put_u64(bytes, 8, config.session);
  put_u64(bytes, 16, config.fingerprint);
  put_u64(bytes, 24, config.material_id);
  put_u32(bytes, 32, config.item_count);
  put_u32(bytes, 36, config.protocol_id);
  return bytes;
}

void exchange_preamble(EmpBoundedFdIO& io, const ProtocolIChosenOtConfig& config,
                       std::uint8_t local_role) {
  const auto local = preamble(config, local_role);
  std::array<std::uint8_t, kPreambleBytes> peer{};
  io.raw_send(local.data(), local.size());
  io.raw_receive(peer.data(), peer.size());
  const auto peer_role = local_role == kSenderRole ? kReceiverRole : kSenderRole;
  if (get_u32(peer, 0) != kMagic || get_u16(peer, 4) != kVersion || peer[6] != peer_role ||
      get_u64(peer, 8) != config.session || get_u64(peer, 16) != config.fingerprint ||
      get_u64(peer, 24) != config.material_id || get_u32(peer, 32) != config.item_count ||
      get_u32(peer, 36) != config.protocol_id) {
    fail("chosen OT preamble mismatch");
  }
}

void consume_material(const ProtocolIChosenOtConfig& config, std::uint8_t role) {
  static std::mutex mutex;
  static std::unordered_set<std::string> consumed;
  const auto key = std::to_string(config.session) + ":" + std::to_string(config.fingerprint) +
                   ":" + std::to_string(config.material_id) + ":" + std::to_string(role);
  std::lock_guard<std::mutex> lock(mutex);
  // This is deliberately only a process-local duplicate invocation guard;
  // it is neither persistent nor a cross-process replay defence.
  if (!consumed.insert(key).second) fail("chosen OT material replay");
}

ProtocolIChosenOtCounters counters(const EmpBoundedFdIO& io) {
  return {io.send_counter, io.recv_counter, io.rounds};
}

std::vector<emp::block> to_emp_blocks(const std::vector<ProtocolIBlock128>& source) {
  std::vector<emp::block> result(source.size());
  for (std::size_t i = 0; i < source.size(); ++i) std::memcpy(&result[i], source[i].data(), 16);
  return result;
}

std::vector<ProtocolIBlock128> from_emp_blocks(const std::vector<emp::block>& source) {
  std::vector<ProtocolIBlock128> result(source.size());
  for (std::size_t i = 0; i < source.size(); ++i) std::memcpy(result[i].data(), &source[i], 16);
  return result;
}

}  // namespace

namespace testing {

void protocol_i_chosen_ot_test_raw_receive(
    int connected_fd, void* data, std::size_t count, int timeout_ms) {
  if (timeout_ms <= 0) fail("chosen OT test raw receive invalid timeout");
  const auto deadline = Clock::now() + std::chrono::milliseconds(timeout_ms);
  EmpBoundedFdIO io(connected_fd, deadline);
  io.raw_receive(data, count);
}

}  // namespace testing

ProtocolIChosenOtCounters protocol_i_chosen_ot_sender(
    const ProtocolIChosenOtConfig& config, int connected_fd,
    const std::vector<ProtocolIBlock128>& message0,
    const std::vector<ProtocolIBlock128>& message1) {
  validate_config(config);
  if (message0.size() != config.item_count || message1.size() != config.item_count)
    fail("chosen OT sender input length");
  const auto deadline = Clock::now() + std::chrono::milliseconds(config.timeout_ms);
  EmpBoundedFdIO io(connected_fd, deadline);
  exchange_preamble(io, config, kSenderRole);
  consume_material(config, kSenderRole);
  auto left = to_emp_blocks(message0);
  auto right = to_emp_blocks(message1);
  emp::IKNP ot(emp::ALICE, &io, false);
  ot.send(left.data(), right.data(), static_cast<int64_t>(left.size()));
  return counters(io);
}

ProtocolIChosenOtReceiverResult protocol_i_chosen_ot_receiver(
    const ProtocolIChosenOtConfig& config, int connected_fd,
    const std::vector<std::uint8_t>& choices) {
  validate_config(config);
  if (choices.size() != config.item_count) fail("chosen OT receiver input length");
  std::unique_ptr<bool[]> bits(new bool[choices.size()]);
  for (std::size_t i = 0; i < choices.size(); ++i) {
    if (choices[i] > 1) fail("chosen OT choice must be zero or one");
    bits[i] = choices[i] != 0;
  }
  const auto deadline = Clock::now() + std::chrono::milliseconds(config.timeout_ms);
  EmpBoundedFdIO io(connected_fd, deadline);
  exchange_preamble(io, config, kReceiverRole);
  consume_material(config, kReceiverRole);
  std::vector<emp::block> selected(choices.size());
  emp::IKNP ot(emp::BOB, &io, false);
  ot.recv(selected.data(), bits.get(), static_cast<int64_t>(selected.size()));
  return {from_emp_blocks(selected), counters(io)};
}

}  // namespace moe_topk
