#pragma once

#include <moe_topk/experimental_bmw16_material_bundle.h>

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace moe_topk {

class ProtocolIBmw16MaterialDeliveryError : public std::runtime_error {
 public:
  enum class Kind { Transport, Material };
  ProtocolIBmw16MaterialDeliveryError(Kind kind, const std::string& what)
      : std::runtime_error(what), kind_(kind) {}
  Kind kind() const noexcept { return kind_; }
 private:
  Kind kind_;
};

// EXPERIMENTAL mutual-TLS transport for the offline BMW16-derived material
// package. Credentials are provisioned out of band; this API never derives
// identity keys from the input, session seed, or algorithm sampler.
struct ProtocolIBmw16TlsCredentials {
  std::string certificate_pem;
  std::string private_key_pem;
  std::string trust_bundle_pem;
  std::string peer_dns_identity;
  int io_timeout_ms = 30000;
};

struct ProtocolIBmw16TlsEndpoint {
  std::string host;
  std::uint16_t port = 0;
  std::string server_dns_identity;
};

struct ProtocolIBmw16TlsDeliveryManifest {
  ProtocolIBmw16ExperimentalPartyConfig config{};
  std::array<std::uint8_t, 32> stream_id{};
  std::array<std::uint8_t, 32> own_shell_manifest_sha256{};
  std::array<std::uint8_t, 32> peer_shell_manifest_sha256{};
  std::array<std::uint8_t, 32> shell_sha256{};
  std::array<std::uint8_t, 32> sidecar_sha256{};
  std::uint64_t slot_count = 0;
  std::uint64_t shell_bytes = 0;
  std::uint64_t sidecar_bytes = 0;
};

struct ProtocolIBmw16TlsDeliveryFiles {
  std::string shell_path;
  std::string sidecar_path;
  std::string pair_ready_path;
};

struct ProtocolIBmw16TlsDeliveryStats {
  std::uint64_t bytes_sent = 0;
  std::uint64_t bytes_received = 0;
  std::uint64_t elapsed_us = 0;
};

// T connects to both already-listening parties. It stages and validates both
// deliveries first, then commits each package. The caller must launch raw
// input creation/online parties only if this function returns after both
// durable commit acknowledgements and T subsequently exits zero.
void protocol_i_bmw16_tls_deliver_pair(
    const ProtocolIBmw16TlsCredentials& dealer_credentials,
    const std::array<ProtocolIBmw16TlsEndpoint, 2>& endpoints,
    const std::array<ProtocolIBmw16TlsDeliveryManifest, 2>& manifests,
    const std::array<ProtocolIBmw16TlsDeliveryFiles, 2>& files,
    const std::array<std::uint8_t, 132>& pair_ready_marker,
    ProtocolIBmw16TlsDeliveryStats* stats = nullptr);

// Party receives exactly one package on an already-bound/listening socket.
// The listener owner and certificate config are supplied by the party service
// manager; the function accepts no path from an unauthenticated peer.
ProtocolIBmw16TlsDeliveryStats protocol_i_bmw16_tls_receive_once(
    int listening_socket,
    const ProtocolIBmw16TlsCredentials& party_credentials,
    const ProtocolIBmw16ExperimentalPartyConfig& expected_config,
    const std::string& claim_root,
    const ProtocolIBmw16TlsDeliveryFiles& destination,
    const std::array<std::uint8_t, 32>& wrapping_key);

}  // namespace moe_topk
