#include <moe_topk/score_semantics.h>
#include <moe_topk/topk_oracle.h>

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

void expect_invalid_argument(const std::vector<std::uint32_t>& scores,
                             std::size_t K,
                             const char* message) {
    try {
        (void)moe_topk::top_k_mask(scores, K);
    } catch (const std::invalid_argument&) {
        return;
    }
    throw std::runtime_error(message);
}

bool unified_precedes(const std::vector<std::uint32_t>& scores,
                      std::size_t left,
                      std::size_t right) {
    const auto left_score = moe_topk::decode_signed_score(scores[left]);
    const auto right_score = moe_topk::decode_signed_score(scores[right]);
    return left_score != right_score ? left_score > right_score : left < right;
}

void validate_mask(const std::vector<std::uint32_t>& scores,
                   std::size_t K,
                   const std::vector<std::uint8_t>& mask) {
    require(mask.size() == scores.size(), "mask length mismatch");
    std::size_t sum = 0;
    for (const auto bit : mask) {
        require(bit == 0 || bit == 1, "mask is not binary");
        sum += bit;
    }
    require(sum == K, "mask cardinality mismatch");

    for (std::size_t selected = 0; selected < scores.size(); ++selected) {
        if (mask[selected] == 0) {
            continue;
        }
        for (std::size_t unselected = 0; unselected < scores.size(); ++unselected) {
            if (mask[unselected] == 1) {
                continue;
            }
            require(!unified_precedes(scores, unselected, selected),
                    "selected set violates the unified order");
        }
    }
}

void verify_case(const std::vector<std::int32_t>& signed_scores,
                 std::size_t K,
                 const std::vector<std::uint8_t>& expected) {
    std::vector<std::uint32_t> scores;
    scores.reserve(signed_scores.size());
    for (const auto score : signed_scores) {
        scores.push_back(moe_topk::encode_signed_score(score));
    }
    const auto actual = moe_topk::top_k_mask(scores, K);
    require(actual == expected, "oracle mask differs from expected mask");
    validate_mask(scores, K, actual);
}

}  // namespace

int main() {
    verify_case({0, 5, -3, 7, 2}, 2, {0, 1, 0, 1, 0});
    verify_case({3, 9, 9, 4}, 2, {0, 1, 1, 0});
    verify_case({5, 5, 5, 5}, 2, {1, 1, 0, 0});
    verify_case({-4, -1, -3}, 1, {0, 1, 0});
    verify_case({-4, 1, 0, 1, -2}, 5, {1, 1, 1, 1, 1});
    verify_case({INT32_MIN, -1, 0, INT32_MAX}, 2, {0, 0, 1, 1});

    std::mt19937 generator(0x4d315f31U);
    std::uniform_int_distribution<std::int32_t> distribution(
        moe_topk::kRandomScoreMinimum, moe_topk::kRandomScoreMaximum);
    std::vector<std::uint32_t> random_scores(7);
    for (auto& score : random_scores) {
        score = moe_topk::encode_signed_score(distribution(generator));
    }
    const auto random_mask = moe_topk::top_k_mask(random_scores, 3);
    validate_mask(random_scores, 3, random_mask);

    expect_invalid_argument({1, 2}, 0, "K=0 must fail");
    expect_invalid_argument({1, 2}, 3, "K>m must fail");
    expect_invalid_argument({}, 1, "empty input must fail");
    return 0;
}
