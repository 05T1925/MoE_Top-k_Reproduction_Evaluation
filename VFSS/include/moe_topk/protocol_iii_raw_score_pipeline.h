#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <moe_topk/protocol_i_party_package.h>
#include <moe_topk/protocol_i_score_input.h>
#include <moe_topk/protocol_iii_dpf_routing.h>
#include <moe_topk/protocol_iii_grank.h>
#include <moe_topk/protocol_iii_secure_combine.h>

namespace moe_topk {

// Unified project entry point:
//
// Q20.12 raw-score arithmetic shares
//        |
//        | two-round secure raw-score adapter
//        v
// padded priority-key additive shares
//        |
//        | three-round Protocol III modular core
//        v
// original-order XOR Top-K mask shares
struct ProtocolIIIRawScorePipelineConfig {
  ProtocolIScoreInputConfig score_input;
  ProtocolIIIGrankConfig grank;
  ProtocolIIIDpfRoutingConfig routing;
  ProtocolIIISecureCombineConfig combine;
};

struct ProtocolIIIRawScorePipelineMaterial {
  // Kept separate because the raw-score adapter consumes carry/sign
  // materials, while GRank consumes node-mask/edge materials.
  ProtocolIPartyPackage score_input_package;
  ProtocolIPartyPackage grank_package;

  ProtocolIIIDpfRoutingPartyMaterial routing_material;
  ProtocolIIISecureCombinePartyMaterial combine_material;

  // One additive share of u_i=1 for every logical input position.
  std::vector<std::uint64_t> unit_payload_shares;

  ProtocolIIIRawScorePipelineMaterial() = default;

  ProtocolIIIRawScorePipelineMaterial(
      const ProtocolIIIRawScorePipelineMaterial&) = delete;

  ProtocolIIIRawScorePipelineMaterial& operator=(
      const ProtocolIIIRawScorePipelineMaterial&) = delete;

  ProtocolIIIRawScorePipelineMaterial(
      ProtocolIIIRawScorePipelineMaterial&&) noexcept = default;

  ProtocolIIIRawScorePipelineMaterial& operator=(
      ProtocolIIIRawScorePipelineMaterial&&) noexcept = default;
};

struct ProtocolIIIRawScorePipelineFds {
  // carry, sign
  std::array<int, 2> score_input_fds{{-1, -1}};

  int grank_fd = -1;
  int routing_fd = -1;
  int combine_fd = -1;
};

struct ProtocolIIIRawScorePipelineMetrics {
  ProtocolIScoreInputMetrics score_input;
  ProtocolIIIGrankMetrics grank;
  ProtocolIIIDpfRoutingMetrics routing;
  ProtocolIIISecureCombineMetrics combine;

  std::uint64_t input_adapter_rounds = 0;
  std::uint64_t core_rounds = 0;
  std::uint64_t total_online_rounds = 0;

  std::uint64_t sent_bytes = 0;
  std::uint64_t received_bytes = 0;
};

struct ProtocolIIIRawScorePipelineOutput {
  std::vector<std::uint8_t> xor_mask_shares;
  ProtocolIIIRawScorePipelineMetrics metrics;
};

// The caller supplies only its own raw-score shares and its own party
// materials. The runtime never reconstructs raw scores, priority keys,
// ranks, indicators, products, selected indices, or the final mask.
[[nodiscard]] ProtocolIIIRawScorePipelineOutput
protocol_iii_raw_score_pipeline_party(
    const ProtocolIIIRawScorePipelineConfig& config,
    ProtocolIIIRawScorePipelineMaterial& material,
    const std::vector<std::uint32_t>& raw_score_shares,
    const ProtocolIIIRawScorePipelineFds& fds);

}  // namespace moe_topk
