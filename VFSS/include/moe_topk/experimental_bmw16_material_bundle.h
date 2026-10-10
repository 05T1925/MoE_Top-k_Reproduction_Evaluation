#pragma once

#include <moe_topk/experimental_bmw16_select_party.h>

#include <array>
#include <cstdint>
#include <mutex>
#include <unordered_set>
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

struct ProtocolIBmw16PublishedFileIdentity {
  std::string path;
  std::uint64_t device = 0;
  std::uint64_t inode = 0;
};

struct ProtocolIBmw16StreamBundleBinding {
  std::array<std::uint8_t, 32> stream_id{};
  std::array<std::uint8_t, 32> sidecar_sha256{};
  std::uint64_t ucmp_slot_count = 0;
  std::uint64_t sealed_sidecar_bytes = 0;
};

struct ProtocolIBmw16StreamBundleOpenResult {
  ProtocolIBmw16ExperimentalPartyMaterial material;
  ProtocolIBmw16StreamBundleBinding stream;
  ProtocolIBmw16BundleStats stats;
};

struct ProtocolIBmw16ClaimRootIdentity {
  std::uint64_t device = 0;
  std::uint64_t inode = 0;
};

// Resolve an already provisioned local claim root without following a final
// symlink. The identity is public metadata and is authenticated in bundle v3.
ProtocolIBmw16ClaimRootIdentity protocol_i_bmw16_claim_root_identity(
    const std::string& directory);

// EXPERIMENTAL bundle v2 (S14 shuffle-only profile): whole-party payload sealed with AES-256-GCM.
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

// EXPERIMENTAL v4 shell profile: encrypts raw-score and forward/inverse
// shuffle material plus a compact implicit manifest that binds a streamed
// Select/membership sidecar. No quadratic uCMP keys are included in memory.
std::vector<std::uint8_t> protocol_i_bmw16_bundle_seal_stream_shell_party(
    const ProtocolIBmw16ExperimentalPartyConfig& config, std::uint8_t party,
    const ProtocolIBmw16ExperimentalPartyMaterial& material,
    const ProtocolIBmw16StreamBundleBinding& stream,
    const std::array<std::uint8_t, 32>& recipient_key,
    ProtocolIBmw16BundleStats* stats = nullptr);

ProtocolIBmw16StreamBundleOpenResult protocol_i_bmw16_bundle_open_stream_shell_party(
    const ProtocolIBmw16ExperimentalPartyConfig& expected, std::uint8_t party,
    const std::vector<std::uint8_t>& envelope,
    const std::array<std::uint8_t, 32>& recipient_key);

// Publish one file atomically without replacing an existing path. The return
// value identifies the exact inode created by this call; rollback must compare
// device+inode before unlinking so a retry never deletes someone else's file.
ProtocolIBmw16PublishedFileIdentity protocol_i_bmw16_bundle_write_atomic(
    const std::string& path, const std::vector<std::uint8_t>& bytes);
bool protocol_i_bmw16_remove_published_if_owned(
    const ProtocolIBmw16PublishedFileIdentity& file) noexcept;
std::vector<std::uint8_t> protocol_i_bmw16_bundle_read(const std::string& path);

class ProtocolIBmw16PersistentClaimStore {
 public:
  ProtocolIBmw16PersistentClaimStore(const std::string& directory,
                                     std::uint64_t session,
                                     std::uint8_t party,
                                     ProtocolIBmw16ClaimRootIdentity expected_root = {});
  ProtocolIBmw16PersistentClaimStore(const ProtocolIBmw16PersistentClaimStore&) = delete;
  ProtocolIBmw16PersistentClaimStore& operator=(const ProtocolIBmw16PersistentClaimStore&) = delete;
  ProtocolIBmw16PersistentClaimStore(ProtocolIBmw16PersistentClaimStore&&) noexcept;
  ProtocolIBmw16PersistentClaimStore& operator=(ProtocolIBmw16PersistentClaimStore&&) noexcept;
  ~ProtocolIBmw16PersistentClaimStore();

  // Claiming the bundle invalidates every contained slot, including after a crash.
  void claim_bundle(const std::array<std::uint8_t, 32>& manifest_digest);

 private:
  int directory_fd_ = -1;
  std::uint64_t session_ = 0;
  std::uint8_t party_ = 0;
  ProtocolIBmw16ClaimRootIdentity root_{};
};

// EXPERIMENTAL runtime guard. It detects duplicate slot use while this process
// is alive. Crash/restart safety comes from the durable bundle claim above:
// after claiming any bundle, the complete bundle is invalidated permanently.
class ProtocolIBmw16ProcessSlotClaimSet {
 public:
  ProtocolIBmw16ProcessSlotClaimSet();
  ProtocolIBmw16ProcessSlotClaimSet(const ProtocolIBmw16ProcessSlotClaimSet&) = delete;
  ProtocolIBmw16ProcessSlotClaimSet& operator=(const ProtocolIBmw16ProcessSlotClaimSet&) = delete;

  void claim_once(std::uint64_t slot_id);
  std::vector<std::uint64_t> snapshot() const;

 private:
  mutable std::mutex mutex_;
  std::uint64_t owner_process_id_ = 0;
  std::unordered_set<std::uint64_t> claimed_;
};

}  // namespace moe_topk
