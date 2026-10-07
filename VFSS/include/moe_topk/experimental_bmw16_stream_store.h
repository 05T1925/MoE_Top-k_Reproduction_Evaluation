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
};

// EXPERIMENTAL random-access sidecar for the quadratic Select + membership
// key pool. The current in-memory KeyGen remains the source of records; this
// adapter demonstrates authenticated sequential sealing and lazy party reads.
// It does not stream KeyGen, raw-score/shuffle material, or reduce peak KeyGen
// memory. The n<=256 bound remains in force.
void protocol_i_bmw16_stream_store_seal_party(
    const std::string& path, const ProtocolIBmw16ExperimentalPartyConfig& config,
    std::uint8_t party, const ProtocolIBmw16ExperimentalPartyMaterial& material,
    const std::array<std::uint8_t, 32>& recipient_key,
    ProtocolIBmw16StreamStoreStats* stats = nullptr);

class ProtocolIBmw16StreamedUcmpSlotStore {
 public:
  static std::shared_ptr<ProtocolIBmw16StreamedUcmpSlotStore> open_party(
      const std::string& path, const ProtocolIBmw16ExperimentalPartyConfig& expected,
      std::uint8_t party, const std::array<std::uint8_t, 32>& recipient_key);
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
  std::mutex mutex_;
  std::unordered_set<std::uint64_t> loaded_;
};

}  // namespace moe_topk
