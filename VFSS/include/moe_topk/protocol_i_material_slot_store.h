#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <moe_topk/protocol_i_ucmp.h>
#include <moe_topk/protocol_i_score_share_abi.h>

namespace moe_topk {

// Staged, input-independent material binding for streamed M6B5/v2 slots.
// This storage ABI is deliberately separate from the current M6B5/v1 runtime.
struct ProtocolIMaterialSlotBinding {
  std::uint64_t session = 0;
  std::uint64_t fingerprint = 0;
  std::uint64_t material_id = 0;
  std::uint32_t logical_n = 0;
  std::uint32_t padded_n = 0;
  std::uint32_t k = 0;
  std::uint8_t comparison_bits = 0;
  std::uint8_t party = 0;
  ProtocolIDcfAbi dcf_abi = ProtocolIDcfAbi::LegacyM2UcV1;
  std::uint64_t expected_slot_count = 0;
  ProtocolIScoreShareAbi score_share_abi =
      ProtocolIScoreShareAbi::S15InducedV1;
};

struct ProtocolIMaterialSlotAddress {
  std::uint32_t purpose = 0;
  std::uint32_t iteration = 0;
  std::uint32_t substage = 0;
  std::uint32_t direction = 0;
  std::uint32_t left = 0;
  std::uint32_t right = 0;

  friend bool operator<(const ProtocolIMaterialSlotAddress& a,
                        const ProtocolIMaterialSlotAddress& b) noexcept {
    if (a.purpose != b.purpose) return a.purpose < b.purpose;
    if (a.iteration != b.iteration) return a.iteration < b.iteration;
    if (a.substage != b.substage) return a.substage < b.substage;
    if (a.direction != b.direction) return a.direction < b.direction;
    if (a.left != b.left) return a.left < b.left;
    return a.right < b.right;
  }
  friend bool operator==(const ProtocolIMaterialSlotAddress& a,
                         const ProtocolIMaterialSlotAddress& b) noexcept {
    return a.purpose == b.purpose && a.iteration == b.iteration &&
           a.substage == b.substage && a.direction == b.direction &&
           a.left == b.left && a.right == b.right;
  }
};

class ProtocolIStreamedMaterialWriter final {
 public:
  ProtocolIStreamedMaterialWriter(
      std::string store_directory, ProtocolIMaterialSlotBinding binding,
      std::uint64_t segment_bytes = 16U * 1024U * 1024U);
  ~ProtocolIStreamedMaterialWriter();
  ProtocolIStreamedMaterialWriter(const ProtocolIStreamedMaterialWriter&) = delete;
  ProtocolIStreamedMaterialWriter& operator=(const ProtocolIStreamedMaterialWriter&) = delete;
  ProtocolIStreamedMaterialWriter(ProtocolIStreamedMaterialWriter&&) noexcept;
  ProtocolIStreamedMaterialWriter& operator=(ProtocolIStreamedMaterialWriter&&) noexcept;

  // Appends one encoded DCF/adapter slot and retains only its descriptor in
  // memory. Slot addresses must be unique within the bound material pool.
  void append(const ProtocolIMaterialSlotAddress&,
              const std::vector<std::uint8_t>& encoded_slot);
  // Closes/fsyncs bounded data segments and an index. It does not make the
  // staging data claimable; the supervisor publishes ready only after T exits.
  void finalize_staging();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

// Party-side reader validates and retains only the small descriptor manifest.
// It claims each payload independently, creating a durable one-shot lease
// before releasing the slot bytes. Reuse this object for large pools so a
// party does not re-read/hash/scan the full manifest for every DCF key.
class ProtocolIStreamedMaterialReader final {
 public:
  ProtocolIStreamedMaterialReader(
      std::string store_directory, ProtocolIMaterialSlotBinding binding);
  ~ProtocolIStreamedMaterialReader();
  ProtocolIStreamedMaterialReader(const ProtocolIStreamedMaterialReader&) = delete;
  ProtocolIStreamedMaterialReader& operator=(
      const ProtocolIStreamedMaterialReader&) = delete;
  ProtocolIStreamedMaterialReader(ProtocolIStreamedMaterialReader&&) noexcept;
  ProtocolIStreamedMaterialReader& operator=(
      ProtocolIStreamedMaterialReader&&) noexcept;

  std::vector<std::uint8_t> claim_slot(
      const ProtocolIMaterialSlotAddress& address);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

void protocol_i_streamed_material_publish_ready(
    const std::string& store_directory,
    const ProtocolIMaterialSlotBinding& binding);

// Atomically burns the addressed slot before releasing its bytes. A crash
// after lease fsync leaves the slot permanently unavailable.
std::vector<std::uint8_t> protocol_i_streamed_material_claim_slot(
    const std::string& store_directory,
    const ProtocolIMaterialSlotBinding& binding,
    const ProtocolIMaterialSlotAddress& address);

}  // namespace moe_topk
