#pragma once

#include <cstdint>

namespace moe_topk {

constexpr std::uint32_t kScoreBitWidth = 32;
constexpr std::uint32_t kFixedPointScale = 12;
constexpr std::uint32_t kScoreSignBit = 0x80000000U;
constexpr std::int32_t kRandomScoreMinimum = -(32 * (1 << kFixedPointScale));
constexpr std::int32_t kRandomScoreMaximum = 32 * (1 << kFixedPointScale);

// Scores are stored as 32-bit two's-complement fixed-point words. XOR with
// the sign bit maps that signed order to the unsigned order VFSS DCF exposes.
constexpr std::uint32_t signed_score_to_ordered(std::uint32_t raw_score) {
    return raw_score ^ kScoreSignBit;
}

constexpr std::int64_t decode_signed_score(std::uint32_t raw_score) {
    return (raw_score & kScoreSignBit) == 0
               ? static_cast<std::int64_t>(raw_score)
               : static_cast<std::int64_t>(raw_score) - (std::int64_t{1} << 32);
}

constexpr std::uint32_t encode_signed_score(std::int32_t score) {
    return static_cast<std::uint32_t>(score);
}

}  // namespace moe_topk
