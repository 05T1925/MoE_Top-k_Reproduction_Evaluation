#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include <moe_topk/protocol_i_transport.h>

#if !defined(MOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER)
#error "BMW16-derived Select online TLS is opt-in and EXPERIMENTAL"
#endif

namespace moe_topk {

inline constexpr std::size_t kProtocolIBmw16OnlineChannelCount = 12;

struct ProtocolIBmw16OnlineTlsConfig {
  std::uint64_t session = 0;
  std::uint64_t fingerprint = 0;
  std::uint32_t n = 0;
  std::uint32_t k = 0;
  std::uint8_t comparison_bits = 0;
  std::uint8_t party = 0;
  int timeout_ms = 0;
  std::string bind_address;
  std::uint16_t local_port_base = 0;
  std::string peer_address;
  std::uint16_t peer_port_base = 0;
  std::string certificate_pem;
  std::string private_key_pem;
  std::string trust_bundle_pem;
  std::string expected_peer_identity;
  // Optional inherited pipe owned by the trusted startup supervisor. P1 writes
  // a binding-specific listening receipt only after all twelve listeners bind.
  int listener_ready_fd = -1;
};

// Opens twelve separate TLS 1.3 mutual-authenticated TCP channels. P0 connects
// to P1. Each application FD is registered so ProtocolIFramedChannel sends
// and receives its full frame bytes through SSL_read/SSL_write. The set must
// outlive the party API call; party API channels consume/close the descriptors.
class ProtocolIBmw16OnlineTlsChannels {
 public:
  explicit ProtocolIBmw16OnlineTlsChannels(const ProtocolIBmw16OnlineTlsConfig&);
  ProtocolIBmw16OnlineTlsChannels(const ProtocolIBmw16OnlineTlsChannels&) = delete;
  ProtocolIBmw16OnlineTlsChannels& operator=(const ProtocolIBmw16OnlineTlsChannels&) = delete;
  ~ProtocolIBmw16OnlineTlsChannels();

  std::array<int, kProtocolIBmw16OnlineChannelCount> party_fds() const;
  void verify_all_registered() const;

 private:
  std::array<int, kProtocolIBmw16OnlineChannelCount> fds_{};
  std::array<bool, kProtocolIBmw16OnlineChannelCount> registered_{};
};

}  // namespace moe_topk
