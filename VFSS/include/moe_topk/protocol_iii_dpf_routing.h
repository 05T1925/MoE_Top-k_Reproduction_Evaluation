#pragma once

#include <FSS/dpf.h>

#include <cstdint>
#include <vector>

namespace moe_topk {

// Move-only owner for one received DPF party key.
class ProtocolIIIDpfRoutingKey {
 public:
  explicit ProtocolIIIDpfRoutingKey(DPFKeyPack&& key);
  ~ProtocolIIIDpfRoutingKey();

  ProtocolIIIDpfRoutingKey(
      const ProtocolIIIDpfRoutingKey&) = delete;

  ProtocolIIIDpfRoutingKey& operator=(
      const ProtocolIIIDpfRoutingKey&) = delete;

  ProtocolIIIDpfRoutingKey(
      ProtocolIIIDpfRoutingKey&& other) noexcept;

  ProtocolIIIDpfRoutingKey& operator=(
      ProtocolIIIDpfRoutingKey&& other) noexcept;

  DPFKeyPack& native_key() noexcept {
    return key_;
  }

  const DPFKeyPack& native_key() const noexcept {
    return key_;
  }

 private:
  DPFKeyPack key_;

  void release() noexcept;
  void take(DPFKeyPack& source) noexcept;
};

struct ProtocolIIIDpfRoutingConfig {
  std::uint64_t session = 0;
  std::uint64_t fingerprint = 0;

  std::uint32_t logical_n = 0;
  std::uint32_t k = 0;

  // Mathematical rank ring: Z_(2^rank_bits).
  std::uint8_t rank_bits = 0;

  // Retained in the frozen framed-channel header. This is not the DPF
  // domain width.
  std::uint8_t comparison_bits = 0;

  std::uint8_t party = 0;
  int timeout_ms = 0;
};

struct ProtocolIIIDpfRoutingPartyMaterial {
  std::uint64_t session = 0;
  std::uint64_t fingerprint = 0;

  std::uint32_t logical_n = 0;
  std::uint32_t k = 0;

  std::uint8_t rank_bits = 0;
  std::uint8_t party = 0;

  // Additive shares of the DPF points r_i in Z_(2^rank_bits).
  std::vector<std::uint64_t> rank_mask_shares;

  // One DPF party key for f_(r_i,1) per logical input position.
  std::vector<ProtocolIIIDpfRoutingKey> dpf_keys;

  ProtocolIIIDpfRoutingPartyMaterial() = default;

  ProtocolIIIDpfRoutingPartyMaterial(
      const ProtocolIIIDpfRoutingPartyMaterial&) = delete;

  ProtocolIIIDpfRoutingPartyMaterial& operator=(
      const ProtocolIIIDpfRoutingPartyMaterial&) = delete;

  ProtocolIIIDpfRoutingPartyMaterial(
      ProtocolIIIDpfRoutingPartyMaterial&&) noexcept = default;

  ProtocolIIIDpfRoutingPartyMaterial& operator=(
      ProtocolIIIDpfRoutingPartyMaterial&&) noexcept = default;
};

struct ProtocolIIIDpfRoutingMetrics {
  std::uint64_t sent_bytes = 0;
  std::uint64_t received_bytes = 0;

  std::uint64_t dpf_keys = 0;
  std::uint64_t eval_calls = 0;

  std::uint64_t online_rounds = 1;
};

struct ProtocolIIIDpfRoutingOutput {
  // Row-major logical_n x k matrix:
  //
  // indicator_shares[input_index * k + target_rank]
  //
  // reconstructs to 1 iff rank[input_index] == target_rank.
  std::vector<std::uint64_t> indicator_shares;

  ProtocolIIIDpfRoutingMetrics metrics;
};

[[nodiscard]] ProtocolIIIDpfRoutingOutput
protocol_iii_dpf_routing_party(
    const ProtocolIIIDpfRoutingConfig& config,
    ProtocolIIIDpfRoutingPartyMaterial& material,
    const std::vector<std::uint64_t>& rank_shares,
    int routing_fd);

}  // namespace moe_topk
