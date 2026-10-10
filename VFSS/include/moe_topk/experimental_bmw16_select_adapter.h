#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <moe_topk/protocol_i_score_input.h>

#if !defined(MOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER)
#error "BMW16-derived Select adapter is opt-in and TEST_ONLY"
#endif

namespace moe_topk {

// EXPERIMENTAL_TEST_ONLY project mapping. Calls the formal Protocol I raw-score
// adapter, then reverses its ascending priority key into the descending key
// direction consumed by the BMW16-derived Select experiment. This is not a
// secure protocol entry and carries no BMW16 theorem guarantee.
std::vector<std::uint64_t> protocol_i_bmw16_experimental_select_key_party(
    const ProtocolIScoreInputConfig& config, ProtocolIPartyPackage& package,
    const std::vector<std::uint32_t>& raw_share,
    const std::array<int, 2>& stage_fds,
    ProtocolIScoreInputMetrics* metrics);

}  // namespace moe_topk
