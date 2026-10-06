#pragma once

#include <moe_topk/experimental_bmw16_select_party.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace moe_topk {

struct ProtocolIBmw16BundleStats {
  std::uint64_t plaintext_bytes = 0;
  std::uint64_t manifest_bytes = 0;
  std::uint64_t ciphertext_bytes = 0;
  std::uint64_t envelope_bytes = 0;
  std::uint64_t authenticated_metadata_bytes = 0;
  std::uint64_t slot_count = 0;
  std::array<std::uint8_t, 32> manifest_sha256{};
};

// EXPERIMENTAL bundle v1: whole-party payload sealed with AES-256-GCM.
// Recipient keys must be provisioned out of band; this API does not implement PKI.
std::vector<std::uint8_t> protocol_i_bmw16_bundle_seal_party(
    const ProtocolIBmw16ExperimentalPartyConfig& config, std::uint8_t party,
    const ProtocolIBmw16ExperimentalPartyMaterial& material,
    const std::array<std::uint8_t, 32>& recipient_key,
    ProtocolIBmw16BundleStats* stats = nullptr);

ProtocolIBmw16ExperimentalPartyMaterial protocol_i_bmw16_bundle_open_party(
    const ProtocolIBmw16ExperimentalPartyConfig& expected, std::uint8_t party,
    const std::vector<std::uint8_t>& envelope,
    const std::array<std::uint8_t, 32>& recipient_key,
    ProtocolIBmw16BundleStats* stats = nullptr);

void protocol_i_bmw16_bundle_write_atomic(const std::string& path,
                                         const std::vector<std::uint8_t>& bytes);
std::vector<std::uint8_t> protocol_i_bmw16_bundle_read(const std::string& path);

class ProtocolIBmw16PersistentClaimStore {
 public:
  ProtocolIBmw16PersistentClaimStore(const std::string& directory,
                                     std::uint64_t session,
                                     std::uint8_t party);
  ProtocolIBmw16PersistentClaimStore(const ProtocolIBmw16PersistentClaimStore&) = delete;
  ProtocolIBmw16PersistentClaimStore& operator=(const ProtocolIBmw16PersistentClaimStore&) = delete;
  ProtocolIBmw16PersistentClaimStore(ProtocolIBmw16PersistentClaimStore&&) noexcept;
  ProtocolIBmw16PersistentClaimStore& operator=(ProtocolIBmw16PersistentClaimStore&&) noexcept;
  ~ProtocolIBmw16PersistentClaimStore();

  // Claiming the bundle invalidates every contained slot, including after a crash.
  void claim_bundle(const std::array<std::uint8_t, 32>& manifest_digest);
  // Per-slot O_EXCL + fsync claims are durable before the caller evaluates a key.
  void claim_slot(std::uint64_t slot_id);

 private:
  int directory_fd_ = -1;
  std::uint64_t session_ = 0;
  std::uint8_t party_ = 0;
};

}  // namespace moe_topk
