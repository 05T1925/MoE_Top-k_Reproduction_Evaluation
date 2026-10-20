#pragma once
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

struct ssl_st;
struct ssl_ctx_st;
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;

namespace moe_topk {

struct ProtocolIFrameConfig {
  std::uint64_t session, fingerprint;
  std::uint32_t n, k;
  std::uint8_t bits, sender, receiver, phase, type;
  // Nonzero selects the version-2 header, which binds the frame to the
  // one-shot material package. Legacy protocols retain version 1.
  std::uint64_t material_id = 0;
};

struct ProtocolIFramedEndpoint {
  int fd = -1;
  SSL* ssl = nullptr;
  bool tls_server = false;
  std::string expected_peer_identity;

  ProtocolIFramedEndpoint() = default;
  ProtocolIFramedEndpoint(int raw_fd) : fd(raw_fd) {}
  ProtocolIFramedEndpoint(int raw_fd, SSL* tls, bool is_server,
                          std::string expected_peer)
      : fd(raw_fd), ssl(tls), tls_server(is_server),
        expected_peer_identity(std::move(expected_peer)) {}
  ProtocolIFramedEndpoint& operator=(int raw_fd) {
    fd = raw_fd;
    ssl = nullptr;
    tls_server = false;
    expected_peer_identity.clear();
    return *this;
  }
};

struct ProtocolITls13Credentials {
  std::string ca_certificate_file;
  std::string certificate_file;
  std::string private_key_file;
};

SSL_CTX* protocol_i_tls13_context_create(const ProtocolITls13Credentials&);
void protocol_i_tls13_context_free(SSL_CTX*) noexcept;
ProtocolIFramedEndpoint protocol_i_tls13_wrap_fd(
    int connected_fd, SSL_CTX*, bool server,
    const std::string& expected_peer_identity);

struct ProtocolIFramedChannelOptions {
  int timeout_ms = 2000;
  std::size_t max_io_chunk = std::numeric_limits<std::size_t>::max();
};

class ProtocolIFramedChannel {
 public:
  ProtocolIFramedChannel(int, ProtocolIFrameConfig, ProtocolIFramedChannelOptions = {});
  ProtocolIFramedChannel(ProtocolIFramedEndpoint, ProtocolIFrameConfig,
                         ProtocolIFramedChannelOptions = {});
  ProtocolIFramedChannel(int, ProtocolIFrameConfig, int);
  ProtocolIFramedChannel(const ProtocolIFramedChannel&) = delete;
  ProtocolIFramedChannel& operator=(const ProtocolIFramedChannel&) = delete;
  ~ProtocolIFramedChannel();
  // Advance the bound message phase while retaining this channel's sequence,
  // TLS session, and descriptor ownership. This is required for multi-step
  // barriers on one authenticated stream.
  void set_phase(std::uint8_t phase);
  const ProtocolIFrameConfig& configuration() const noexcept { return c_; }
  void complete_authenticated_handshake();
  std::array<std::uint8_t, 32> peer_certificate_sha256() const;
  void send(const std::vector<std::uint8_t>&);
  std::vector<std::uint8_t> receive();
  void reset_counters() {
    sent_ = received_ = 0;
    sent_frames_ = received_frames_ = 0;
    max_payload_sent_ = max_payload_received_ = 0;
  }
  std::uint64_t sent_bytes() const { return sent_; }
  std::uint64_t received_bytes() const { return received_; }
  std::uint64_t sent_frames() const { return sent_frames_; }
  std::uint64_t received_frames() const { return received_frames_; }
  std::size_t max_payload_sent() const { return max_payload_sent_; }
  std::size_t max_payload_received() const { return max_payload_received_; }

 private:
  using Clock = std::chrono::steady_clock;
  int fd_, timeout_;
  SSL* ssl_ = nullptr;
  bool tls_server_ = false;
  bool tls_handshake_complete_ = false;
  std::string expected_peer_identity_;
  std::size_t max_io_chunk_;
  ProtocolIFrameConfig c_;
  std::uint64_t out_ = 0, in_ = 0, sent_ = 0, received_ = 0;
  std::uint64_t sent_frames_ = 0, received_frames_ = 0;
  std::size_t max_payload_sent_ = 0, max_payload_received_ = 0;
  void exact(void*, std::size_t, bool, Clock::time_point);
  void ensure_tls_handshake(Clock::time_point);
};

// Bounded package transport for the offline P2 -> P0/P1 handoff.  Each chunk
// remains a normal authenticated ProtocolIFramedChannel frame; the small
// envelope only binds the whole-message size and its ordered offset.
void protocol_i_send_framed_chunks(ProtocolIFramedChannel& channel,
                                   const std::vector<std::uint8_t>& message);
std::vector<std::uint8_t> protocol_i_receive_framed_chunks(ProtocolIFramedChannel& channel,
                                                            std::size_t max_message_bytes);
}  // namespace moe_topk
