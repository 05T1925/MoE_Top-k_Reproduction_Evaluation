#pragma once

#include <cstdint>

namespace moe_topk::score_to_mask_v3_detail {

// Returns false only after max_attempts words have all fallen in the biased
// prefix. A bound of one consumes no randomness and returns zero.
template <typename NextWord>
bool next_uniform_bounded(std::uint32_t bound, std::uint32_t max_attempts,
                          NextWord&& next_word, std::uint32_t& result,
                          std::uint32_t* attempts_used = nullptr) {
  if (attempts_used != nullptr) *attempts_used = 0;
  if (bound == 0) return false;
  if (bound == 1) {
    result = 0;
    return true;
  }

  const std::uint32_t threshold = static_cast<std::uint32_t>(-bound) % bound;
  for (std::uint32_t attempt = 0; attempt < max_attempts; ++attempt) {
    const auto value = next_word();
    if (attempts_used != nullptr) *attempts_used = attempt + 1U;
    if (value >= threshold) {
      result = value % bound;
      return true;
    }
  }
  return false;
}

// Exact integer test for ceil((lambda + log2(m - 1)) / 24), avoiding a
// floating-point boundary decision. The supported S45 domain is m <= 256.
inline std::uint32_t bounded_retry_limit(std::uint32_t lambda,
                                        std::uint32_t m) {
  if (m <= 1U) return 0;
  if (m > 256U) return 0;
  for (std::uint32_t r = 0; r <= 32U; ++r) {
    const std::uint64_t scaled = 24ULL * r;
    if (scaled < lambda) continue;
    const std::uint64_t exponent = scaled - lambda;
    if (exponent >= 63U || (std::uint64_t{1} << exponent) >= m - 1U)
      return r;
  }
  return 0;
}

}  // namespace moe_topk::score_to_mask_v3_detail
