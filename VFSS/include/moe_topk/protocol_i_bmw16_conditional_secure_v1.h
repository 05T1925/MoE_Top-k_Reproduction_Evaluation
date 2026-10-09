#pragma once

#include <moe_topk/experimental_bmw16_select_party.h>

#if !defined(MOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V1)
#error "Conditional BMW16 security v1 entry point is opt-in and disabled by default"
#endif

namespace moe_topk {

// This versioned entry point is scoped to the S29 conditional security profile
// documented in docs/decisions/BMW16_S29_CONDITIONAL_SECURITY_DECISION_2026-10-09.md.
// It does not upgrade the algorithm to BB90, does not inherit BMW16 Theorem 8,
// and is not a production/deployment certification. Types intentionally reuse
// the already reviewed EXPERIMENTAL wire/material ABI.
using ProtocolIBmw16ConditionalSecureV1Config = ProtocolIBmw16ExperimentalPartyConfig;
using ProtocolIBmw16ConditionalSecureV1Material = ProtocolIBmw16ExperimentalPartyMaterial;
using ProtocolIBmw16ConditionalSecureV1Output = ProtocolIBmw16ExperimentalPartyOutput;

ProtocolIBmw16ConditionalSecureV1Output
protocol_i_bmw16_conditional_secure_v1_raw_score_mask_party(
    const ProtocolIBmw16ConditionalSecureV1Config& config,
    ProtocolIBmw16ConditionalSecureV1Material&& material,
    const std::vector<std::uint32_t>& raw_score_share,
    const std::array<int, 2>& score_fds,
    const std::array<int, 2>& forward_shuffle_fds,
    const std::array<int, 4>& select_round_fds,
    const std::array<int, 2>& inverse_shuffle_fds,
    int final_agreement_fd,
    int sampling_coin_fd);

}  // namespace moe_topk
