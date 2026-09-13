#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <moe_topk/protocol_i_dealer_candidate_core.h>
#include <moe_topk/protocol_i_pipeline.h>

namespace moe_topk {

inline constexpr char kProtocolIDealerCandidateRouteAPriorityLabel[] =
    "m2_protocol_i_dealer_preprocessed_rank_reveal_6round_mask_output";

struct ProtocolIDealerCandidateRouteAOutput {
  std::vector<std::uint8_t> xor_mask_share;
  ProtocolIDealerCandidateMetrics core_metrics;
  std::uint64_t rank_reveal_sent_bytes = 0, rank_reveal_received_bytes = 0;
  std::uint64_t reverse_sent_bytes = 0, reverse_received_bytes = 0;
  std::uint64_t online_rounds = 6;
};

ProtocolIDealerCandidateRouteAOutput protocol_i_dealer_candidate_route_a_party(
    const ProtocolIDealerCandidateCoreConfig& config,
    ProtocolIDealerCandidatePackage&& package,
    ProtocolIShufflePartyMaterial& shuffle_material,
    const std::vector<std::uint64_t>& priority_key_share,
    const std::array<int, 2>& forward_fds,
    int masked_open_fd,
    int rank_reveal_fd,
    const std::array<int, 2>& reverse_fds);

ProtocolIPriorityPipelineOutput protocol_i_dealer_candidate_route_a_priority_party(
    const ProtocolIPriorityPipelineConfig& config,
    ProtocolIPartyPackage&& package,
    ProtocolIShufflePartyMaterial& shuffle_material,
    const std::vector<std::uint64_t>& priority_key_share,
    const std::array<int, 2>& forward_fds,
    int masked_open_fd,
    int rank_reveal_fd,
    const std::array<int, 2>& reverse_fds);

}  // namespace moe_topk
