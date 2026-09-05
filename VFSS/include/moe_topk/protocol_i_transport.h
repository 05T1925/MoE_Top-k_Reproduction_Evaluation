#pragma once
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace moe_topk {

struct ProtocolIFrameConfig {
  std::uint64_t session, fingerprint;
  std::uint32_t n, k;
  std::uint8_t bits, sender, receiver, phase, type;
};

struct ProtocolIFramedChannelOptions {
  int timeout_ms = 2000;
  std::size_t max_io_chunk = std::numeric_limits<std::size_t>::max();
};

class ProtocolIFramedChannel {
 public:
  ProtocolIFramedChannel(int, ProtocolIFrameConfig, ProtocolIFramedChannelOptions = {});
  ProtocolIFramedChannel(int, ProtocolIFrameConfig, int);
  ProtocolIFramedChannel(const ProtocolIFramedChannel&) = delete;
  ProtocolIFramedChannel& operator=(const ProtocolIFramedChannel&) = delete;
  ~ProtocolIFramedChannel();
  void send(const std::vector<std::uint8_t>&);
  std::vector<std::uint8_t> receive();
  void reset_counters() { sent_ = received_ = 0; }
  std::uint64_t sent_bytes() const { return sent_; }
  std::uint64_t received_bytes() const { return received_; }

 private:
  using Clock = std::chrono::steady_clock;
  int fd_, timeout_;
  std::size_t max_io_chunk_;
  ProtocolIFrameConfig c_;
  std::uint64_t out_ = 0, in_ = 0, sent_ = 0, received_ = 0;
  void exact(void*, std::size_t, bool, Clock::time_point);
};
}  // namespace moe_topk
