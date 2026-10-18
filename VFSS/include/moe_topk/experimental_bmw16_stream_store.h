#pragma once

#include <moe_topk/experimental_bmw16_material_bundle.h>

#include <memory>
#include <mutex>
#include <unordered_set>

namespace moe_topk {

struct ProtocolIBmw16StreamStoreStats {
  std::uint64_t ucmp_slots = 0;
  std::uint64_t plaintext_key_bytes = 0;
  std::uint64_t sealed_file_bytes = 0;
  std::array<std::uint8_t, 32> stream_id{};
  std::array<std::uint8_t, 32> file_sha256{};
};

// EXPERIMENTAL sequential writer. A trusted-offline generator appends each
// freshly generated pair-key slot and immediately seals each party copy. The
// final path appears only after the exact pool is fsynced and renamed.
class ProtocolIBmw16StreamedUcmpSlotWriter {
 public:
  ProtocolIBmw16StreamedUcmpSlotWriter(
      const std::string& path, const ProtocolIBmw16ExperimentalPartyConfig& config,
      std::uint8_t party, const std::array<std::uint8_t, 32>& recipient_key,
      const std::array<std::uint8_t, 32>& stream_id);
  ~ProtocolIBmw16StreamedUcmpSlotWriter();
  ProtocolIBmw16StreamedUcmpSlotWriter(const ProtocolIBmw16StreamedUcmpSlotWriter&) = delete;
  ProtocolIBmw16StreamedUcmpSlotWriter& operator=(const ProtocolIBmw16StreamedUcmpSlotWriter&) = delete;

  void append(const ProtocolIBmw16MaterialSlot& slot,
              const ProtocolIUcmpPartyMaterial& key);
  ProtocolIBmw16StreamStoreStats finalize();
  // Keep the no-clobber published sidecar only after the pair-ready marker is
  // durably published. Until then the writer removes only the inode it made.
  void commit_published() noexcept { committed_ = true; }

 private:
  int fd_ = -1;
  std::string final_path_, temporary_path_;
  std::array<std::uint8_t, 32> data_key_{}, stream_id_{};
  std::array<std::uint8_t, 12> base_nonce_{};
  std::vector<std::uint8_t> prefix_;
  ProtocolIBmw16ExperimentalPartyConfig config_{};
  std::uint8_t party_ = 0;
  std::uint32_t key_bytes_ = 0, record_bytes_ = 0;
  std::uint64_t expected_slots_ = 0, written_slots_ = 0, plaintext_bytes_ = 0;
  std::uint64_t temporary_device_ = 0, temporary_inode_ = 0;
  std::uint64_t published_device_ = 0, published_inode_ = 0;
  void* digest_context_ = nullptr;
  bool finalized_ = false, published_ = false, committed_ = false;
  void rollback_owned_files() noexcept;
};

// Legacy adapter retained for differential tests. It serializes an already
// materialized pool and is not used by the T/party-node full path.
void protocol_i_bmw16_stream_store_seal_party(
    const std::string& path, const ProtocolIBmw16ExperimentalPartyConfig& config,
    std::uint8_t party, const ProtocolIBmw16ExperimentalPartyMaterial& material,
    const std::array<std::uint8_t, 32>& recipient_key,
    ProtocolIBmw16StreamStoreStats* stats = nullptr);

class ProtocolIBmw16StreamedUcmpSlotStore {
 public:
  static std::shared_ptr<ProtocolIBmw16StreamedUcmpSlotStore> open_party(
      const std::string& path, const ProtocolIBmw16ExperimentalPartyConfig& expected,
      std::uint8_t party, const std::array<std::uint8_t, 32>& recipient_key,
      const std::array<std::uint8_t, 32>& expected_stream_id = {},
      const std::array<std::uint8_t, 32>& expected_file_sha256 = {});
  ~ProtocolIBmw16StreamedUcmpSlotStore();
  ProtocolIBmw16StreamedUcmpSlotStore(const ProtocolIBmw16StreamedUcmpSlotStore&) = delete;
  ProtocolIBmw16StreamedUcmpSlotStore& operator=(const ProtocolIBmw16StreamedUcmpSlotStore&) = delete;

  ProtocolIUcmpPartyMaterial load_once(const ProtocolIBmw16MaterialSlot& slot);
  std::uint64_t file_bytes() const noexcept { return file_bytes_; }
  std::uint64_t slots() const noexcept { return slot_count_; }

 private:
  ProtocolIBmw16StreamedUcmpSlotStore() = default;
  int fd_ = -1;
  std::array<std::uint8_t, 32> data_key_{};
  ProtocolIBmw16ExperimentalPartyConfig config_{};
  std::uint8_t party_ = 0;
  std::uint64_t slot_count_ = 0, file_bytes_ = 0;
  std::uint32_t key_bytes_ = 0, record_bytes_ = 0;
  std::array<std::uint8_t, 32> stream_id_{};
  std::array<std::uint8_t, 12> base_nonce_{};
  std::mutex mutex_;
  std::unordered_set<std::uint64_t> loaded_;
};

}  // namespace moe_topk
