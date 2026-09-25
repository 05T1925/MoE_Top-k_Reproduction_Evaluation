#pragma once

#include <cstdint>
#include <vector>

#include <moe_topk/protocol_iii_raw_score_mask.h>

namespace moe_topk {

// Same-build/same-architecture package because the embedded native FSS DPF
// key tail is not a portable canonical format. All application metadata is
// big-endian and bound to the expected party and material identity.
[[nodiscard]] std::vector<std::uint8_t>
protocol_iii_raw_score_mask_serialize_bundle(
    const ProtocolIIIRawScoreMaskConfig& config,
    const ProtocolIIIRawScoreMaskMaterial& material);

[[nodiscard]] ProtocolIIIRawScoreMaskMaterial
protocol_iii_raw_score_mask_deserialize_bundle(
    const ProtocolIIIRawScoreMaskConfig& expected,
    const std::vector<std::uint8_t>& bytes);

}  // namespace moe_topk
