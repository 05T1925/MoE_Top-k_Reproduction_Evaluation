#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <moe_topk/protocol_i_score_input.h>
#include <moe_topk/protocol_iii_dpf_routing.h>
#include <moe_topk/protocol_iii_grank.h>

namespace moe_topk {

// F1 standard-function specialization: existing two-round score adapter,
// shared CmpAgg, and masked-rank DPF. No field record is required here.
struct ProtocolIIIRawScoreMaskConfig {
  std::uint64_t material_id = 0;
  ProtocolIScoreInputConfig score_input;
  ProtocolIIIGrankConfig grank;
  ProtocolIIIDpfRoutingConfig routing;
};

struct ProtocolIIIRawScoreMaskMaterial {
  std::uint64_t material_id = 0;
  ProtocolIPartyPackage score_input_package;
  ProtocolIPartyPackage grank_package;
  ProtocolIIIDpfRoutingPartyMaterial routing_material;
  bool started = false;

  ProtocolIIIRawScoreMaskMaterial() = default;
  ProtocolIIIRawScoreMaskMaterial(const ProtocolIIIRawScoreMaskMaterial&) = delete;
  ProtocolIIIRawScoreMaskMaterial& operator=(const ProtocolIIIRawScoreMaskMaterial&) = delete;
  ProtocolIIIRawScoreMaskMaterial(ProtocolIIIRawScoreMaskMaterial&&) noexcept = default;
  ProtocolIIIRawScoreMaskMaterial& operator=(ProtocolIIIRawScoreMaskMaterial&&) noexcept = default;
};

struct ProtocolIIIRawScoreMaskFds {
  std::array<int, 2> score_input_fds{{-1, -1}};
  int grank_fd = -1;
  int routing_fd = -1;
};

struct ProtocolIIIRawScoreMaskMetrics {
  ProtocolIScoreInputMetrics score_input;
  ProtocolIIIGrankMetrics grank;
  ProtocolIIIDpfRoutingMetrics routing;
  std::uint64_t input_adapter_rounds = 0;
  std::uint64_t mask_core_rounds = 0;
  std::uint64_t output_adapter_rounds = 0;
  std::uint64_t total_online_rounds = 0;
  std::uint64_t sent_bytes = 0;
  std::uint64_t received_bytes = 0;
  // Across both parties, excluding wire framing.
  std::uint64_t input_logical_bits = 0;
  std::uint64_t ranking_logical_bits = 0;
  std::uint64_t routing_logical_bits = 0;
  std::uint64_t total_logical_bits = 0;
  std::uint64_t output_local_ring_additions = 0;
};

struct ProtocolIIIRawScoreMaskOutput {
  std::vector<std::uint8_t> xor_mask_shares;
  ProtocolIIIRawScoreMaskMetrics metrics;
};

// Local homomorphism Z_(2^64) -> Z_2. This does not hold for the odd-prime
// field shares in the general field-valued Fselect/Fsort core.
[[nodiscard]] std::vector<std::uint8_t>
protocol_iii_ring_indicators_to_xor_mask(
    std::uint32_t logical_n, std::uint32_t k,
    const std::vector<std::uint64_t>& indicator_shares);

// Each online party supplies only its own Q20.12 score shares and one-shot
// material. P2 distributes input-independent material before these four rounds.
[[nodiscard]] ProtocolIIIRawScoreMaskOutput protocol_iii_raw_score_mask_party(
    const ProtocolIIIRawScoreMaskConfig& config,
    ProtocolIIIRawScoreMaskMaterial& material,
    const std::vector<std::uint32_t>& raw_score_shares,
    const ProtocolIIIRawScoreMaskFds& fds);

}  // namespace moe_topk
