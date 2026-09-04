#include <moe_topk/score_semantics.h>
#include <moe_topk/topk_oracle.h>

#include <algorithm>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void verify_case(const std::vector<std::int32_t>& signed_scores, std::size_t K) {
    std::vector<std::uint32_t> scores;
    scores.reserve(signed_scores.size());
    for (const auto score : signed_scores) {
        scores.push_back(moe_topk::encode_signed_score(score));
    }

    const auto direct_mask = moe_topk::top_k_mask(scores, K);
    const auto ranks = moe_topk::stable_ranks_cmpagg(scores);
    const auto cmpagg_mask = moe_topk::top_k_mask_from_stable_ranks(ranks, K);
    require(cmpagg_mask == direct_mask,
            "CmpAgg Top-K mask differs from direct oracle mask");

    std::vector<std::uint64_t> sorted_ranks = ranks;
    std::sort(sorted_ranks.begin(), sorted_ranks.end());
    for (std::size_t index = 0; index < sorted_ranks.size(); ++index) {
        require(sorted_ranks[index] == index, "CmpAgg ranks are not stable and unique");
    }
}

}  // namespace

int main() {
    verify_case({0, 5, -3, 7, 2}, 2);
    verify_case({3, 9, 9, 4}, 2);
    verify_case({5, 5, 5, 5}, 2);
    verify_case({INT32_MIN, -1, 0, INT32_MAX}, 3);
    verify_case({-4, 1, 0, 1, -2, 7, 7}, 4);

    std::mt19937 generator(0x434d5041U);
    std::uniform_int_distribution<std::int32_t> distribution(
        moe_topk::kRandomScoreMinimum, moe_topk::kRandomScoreMaximum);
    for (std::size_t m : {3U, 5U, 7U, 11U}) {
        std::vector<std::int32_t> scores(m);
        for (auto& score : scores) {
            score = distribution(generator);
        }
        for (std::size_t K = 1; K <= m; ++K) {
            verify_case(scores, K);
        }
    }
    return 0;
}
