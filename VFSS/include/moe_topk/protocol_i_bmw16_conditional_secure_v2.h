#pragma once

#include <moe_topk/experimental_bmw16_select_party.h>

#if !defined(MOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V2)
#error "Conditional BMW16 security v2 entry point is opt-in and disabled by default"
#endif

namespace moe_topk {

// Bounded n<=1000 extension of the S29 conditional profile. The implementation
// reuses the exact S26/S28 party runtime and material ABI; only the public
// admission bound differs from conditional_secure_v1. See the S30 decision for
// the parameterized pool, collision, and ROM sampler bounds at this scale.
using ProtocolIBmw16ConditionalSecureV2Config = ProtocolIBmw16ExperimentalPartyConfig;
using ProtocolIBmw16ConditionalSecureV2Material = ProtocolIBmw16ExperimentalPartyMaterial;
using ProtocolIBmw16ConditionalSecureV2Output = ProtocolIBmw16ExperimentalPartyOutput;

ProtocolIBmw16ConditionalSecureV2Output
protocol_i_bmw16_conditional_secure_v2_raw_score_mask_party(
    const ProtocolIBmw16ConditionalSecureV2Config& config,
    ProtocolIBmw16ConditionalSecureV2Material&& material,
    const std::vector<std::uint32_t>& raw_score_share,
    const std::array<int, 2>& score_fds,
    const std::array<int, 2>& forward_shuffle_fds,
    const std::array<int, 4>& select_round_fds,
    const std::array<int, 2>& inverse_shuffle_fds,
    int final_agreement_fd,
    int sampling_coin_fd);

}  // namespace moe_topk
