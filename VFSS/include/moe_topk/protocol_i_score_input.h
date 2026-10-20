#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <moe_topk/protocol_i_party_package.h>
#include <moe_topk/protocol_i_transport.h>
#include <moe_topk/protocol_i_score_share_abi.h>

namespace moe_topk {

// C: narrow M2.14 adapter.  It turns only Q20.12 raw-word arithmetic shares
// into the existing padded priority-key ring; it is not a generic conversion API.
struct ProtocolIScoreInputConfig {
  std::uint64_t session = 0, fingerprint = 0;
  std::uint32_t logical_n = 0, padded_n = 0, k = 0;
  std::uint8_t index_bits = 0, comparison_bits = 0, party = 0;
  int timeout_ms = 0;
  std::uint64_t material_id = 0;
};

struct ProtocolIScoreInputMetrics {
  std::uint64_t carry_sent_bytes = 0, carry_received_bytes = 0;
  std::uint64_t sign_sent_bytes = 0, sign_received_bytes = 0;
  std::uint64_t ucmp_calls = 0, raw_dcf_calls = 0;
  std::uint64_t rounds = 2;
};

std::vector<std::uint64_t> protocol_i_raw_score_input_party(
    const ProtocolIScoreInputConfig&, ProtocolIPartyPackage&,
    const std::vector<std::uint32_t>& raw_share,
    const std::array<ProtocolIFramedEndpoint, 2>& stage_fds,
    ProtocolIScoreInputMetrics* metrics);

std::vector<std::uint64_t> protocol_i_raw_score_input_party(
    const ProtocolIScoreInputConfig&, ProtocolIPartyPackage&,
    const std::vector<std::uint32_t>& raw_share, const std::array<int, 2>& stage_fds,
    ProtocolIScoreInputMetrics* metrics);

#if defined(MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL)
// Applies F_scoreconv/v2 to the complete priority-key shares produced by the
// unchanged S15 carry/sign adapter. The one-shot rho vector is party/session/
// slot-bound by the caller's versioned material ABI. No q/high-part share is
// produced after this transformation. This API is absent from feature-OFF builds.
void protocol_i_refresh_full_priority_key_shares_v2(
    std::vector<std::uint64_t>& key_shares,
    const std::vector<std::uint64_t>& rho_shares,
    std::uint8_t comparison_bits);
#endif

}  // namespace moe_topk
