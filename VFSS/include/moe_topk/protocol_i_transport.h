#pragma once
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace moe_topk {

// Stable classification for failures at the framed I/O boundary. Callers may
// treat this as a local communication abort; malformed local configuration
// remains an ordinary argument/programming error.
class ProtocolITransportError : public std::runtime_error {
 public:
  explicit ProtocolITransportError(const std::string& message)
      : std::runtime_error(message) {}
};

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

// Bounded package transport for the offline P2 -> P0/P1 handoff.  Each chunk
// remains a normal authenticated ProtocolIFramedChannel frame; the small
// envelope only binds the whole-message size and its ordered offset.
void protocol_i_send_framed_chunks(ProtocolIFramedChannel& channel,
                                   const std::vector<std::uint8_t>& message);
std::vector<std::uint8_t> protocol_i_receive_framed_chunks(ProtocolIFramedChannel& channel,
                                                            std::size_t max_message_bytes);
}  // namespace moe_topk
