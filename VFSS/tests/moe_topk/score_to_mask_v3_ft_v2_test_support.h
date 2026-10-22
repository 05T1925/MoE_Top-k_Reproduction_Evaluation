#pragma once

#ifndef MOE_TOPK_SCORE_TO_MASK_V3_FT_V2_TESTING
#error "This TEST_ONLY header requires the test-instrumented F_T/v2 build."
#endif

#include <moe_topk/score_to_mask_v3_gmw.h>

#include <cstddef>
#include <string>

namespace moe_topk::test_only {

// Uses deterministic bytes only in a CTest-enabled build. The production
// issuer and its public API always use the OpenSSL RAND_bytes wrapper.
void score_to_mask_v3_dealer_issue_packages_ft_v2_with_test_random(
    const ScoreToMaskV3Session& session, const std::string& package_p0_path,
    const std::string& package_p1_path,
    const std::string& signed_escrow_context_path,
    const std::string& dealer_ed25519_private_key_pem,
    const std::string& private_issuance_registry_directory,
    const ScoreToMaskV3EscrowStateParameters& escrow_state,
    bool reject_once_then_accept, std::size_t* bytes_consumed,
    std::size_t* random_call_count, std::size_t* max_request_bytes);

}  // namespace moe_topk::test_only
