#pragma once

#include <moe_topk/protocol_i_dealer_candidate_package.h>
#include <moe_topk/protocol_i_party_package.h>

namespace moe_topk {

inline constexpr char kProtocolIRawScoreRouteALabel[] =
    "m2_protocol_i_raw_score_dealer_preprocessed_rank_reveal_8round_mask_output";

// C: input-independent Dealer material for the raw-score Route A extension.
// The returned package is the existing M2 party-package format with the two
// score-adapter material stages populated; it is not the formal M2 baseline.
struct ProtocolIRawScoreRouteAPackagePair {
  ProtocolIPartyPackage party0;
  ProtocolIPartyPackage party1;
};

ProtocolIRawScoreRouteAPackagePair protocol_i_raw_score_route_a_preprocess(
    const ProtocolIDealerCandidatePublicConfig& config);

}  // namespace moe_topk
