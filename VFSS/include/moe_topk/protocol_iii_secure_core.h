#pragma once

#include <cstdint>
#include <vector>

#include <moe_topk/protocol_i_party_package.h>
#include <moe_topk/protocol_iii_dpf_routing.h>
#include <moe_topk/protocol_iii_grank.h>
#include <moe_topk/protocol_iii_secure_combine.h>

namespace moe_topk {

// Production boundary for the priority-key-input Protocol III modular
// three-round core.
//
// This type deliberately excludes the two-round raw-score adapter.
struct ProtocolIIISecureCoreConfig {
  ProtocolIIIGrankConfig grank;
  ProtocolIIIDpfRoutingConfig routing;
  ProtocolIIISecureCombineConfig combine;
};

struct ProtocolIIISecureCoreMaterial {
  ProtocolIPartyPackage grank_package;
  ProtocolIIIDpfRoutingPartyMaterial routing_material;
  ProtocolIIISecureCombinePartyMaterial combine_material;

  // One additive share of u_i=1 for every logical input position.
  std::vector<std::uint64_t> unit_payload_shares;

  ProtocolIIISecureCoreMaterial() = default;

  ProtocolIIISecureCoreMaterial(
      const ProtocolIIISecureCoreMaterial&) = delete;

  ProtocolIIISecureCoreMaterial& operator=(
      const ProtocolIIISecureCoreMaterial&) = delete;

  ProtocolIIISecureCoreMaterial(
      ProtocolIIISecureCoreMaterial&&) noexcept = default;

  ProtocolIIISecureCoreMaterial& operator=(
      ProtocolIIISecureCoreMaterial&&) noexcept = default;
};

struct ProtocolIIISecureCoreFds {
  int grank_fd = -1;
  int routing_fd = -1;
  int combine_fd = -1;
};

struct ProtocolIIISecureCoreMetrics {
  ProtocolIIIGrankMetrics grank;
  ProtocolIIIDpfRoutingMetrics routing;
  ProtocolIIISecureCombineMetrics combine;

  std::uint64_t sent_bytes = 0;
  std::uint64_t received_bytes = 0;
  std::uint64_t online_rounds = 0;
};

struct ProtocolIIISecureCoreOutput {
  // Original-input-order XOR shares. The vector has logical_n entries and
  // every local share is represented as 0 or 1.
  std::vector<std::uint8_t> xor_mask_shares;

  ProtocolIIISecureCoreMetrics metrics;
};

// Secure production entry.
//
// Input:
//   padded_n priority-key additive shares.
//
// Online execution:
//   R1 GRank -> R2 DPF routing -> R3 secure combine.
//
// Output:
//   logical_n original-order XOR Top-K mask shares.
//
// This function never reconstructs priority keys, ranks, indicators,
// selected indices, products, or the final mask.
[[nodiscard]] ProtocolIIISecureCoreOutput
protocol_iii_secure_core_party(
    const ProtocolIIISecureCoreConfig& config,
    ProtocolIIISecureCoreMaterial& material,
    const std::vector<std::uint64_t>& priority_key_shares,
    const ProtocolIIISecureCoreFds& fds);

}  // namespace moe_topk
