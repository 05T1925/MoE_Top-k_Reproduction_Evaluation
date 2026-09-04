#pragma once

#include <moe_topk/score_semantics.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace moe_topk {

inline void validate_top_k_request(std::size_t m, std::size_t K) {
    if (m == 0) {
        throw std::invalid_argument("Top-K input is empty");
    }
    if (K == 0 || K > m) {
        throw std::invalid_argument("Top-K requires 1 <= K <= m");
    }
}

inline bool top_k_precedes(const std::vector<std::uint32_t>& scores,
                           std::size_t left,
                           std::size_t right) {
    const auto left_ordered = signed_score_to_ordered(scores[left]);
    const auto right_ordered = signed_score_to_ordered(scores[right]);
    if (left_ordered != right_ordered) {
        return left_ordered > right_ordered;
    }
    return left < right;
}

inline std::vector<std::uint8_t> top_k_mask(
    const std::vector<std::uint32_t>& scores,
    std::size_t K) {
    validate_top_k_request(scores.size(), K);

    std::vector<std::size_t> indices(scores.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](std::size_t left, std::size_t right) {
        return top_k_precedes(scores, left, right);
    });

    std::vector<std::uint8_t> mask(scores.size(), 0);
    for (std::size_t i = 0; i < K; ++i) {
        mask[indices[i]] = 1;
    }
    return mask;
}

inline std::vector<std::uint64_t> stable_ranks_cmpagg(
    const std::vector<std::uint32_t>& scores) {
    if (scores.empty()) {
        throw std::invalid_argument("CmpAgg input is empty");
    }

    std::vector<std::uint64_t> ranks(scores.size(), 0);
    for (std::size_t left = 0; left < scores.size(); ++left) {
        for (std::size_t right = left + 1; right < scores.size(); ++right) {
            if (top_k_precedes(scores, left, right)) {
                ++ranks[right];
            } else {
                ++ranks[left];
            }
        }
    }
    return ranks;
}

inline std::vector<std::uint8_t> top_k_mask_from_stable_ranks(
    const std::vector<std::uint64_t>& ranks,
    std::size_t K) {
    validate_top_k_request(ranks.size(), K);

    std::vector<std::uint8_t> mask(ranks.size(), 0);
    for (std::size_t index = 0; index < ranks.size(); ++index) {
        if (ranks[index] < K) {
            mask[index] = 1;
        }
    }
    return mask;
}

}  // namespace moe_topk
