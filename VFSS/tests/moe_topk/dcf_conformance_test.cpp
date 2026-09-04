#include <FSS/dcf.h>
#include <FSS/freekey.h>
#include <FSS/prng.h>

#include <moe_topk/score_semantics.h>

#include <cstdint>
#include <initializer_list>
#include <stdexcept>

namespace {

constexpr int kScoreBits = 32;

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::uint64_t eval_less_than(const DCFKeyPack& key0,
                             const DCFKeyPack& key1,
                             std::uint32_t value) {
    GroupElement share0 = 0;
    GroupElement share1 = 0;
    evalDCF(0, &share0, value, key0);
    evalDCF(1, &share1, value, key1);
    return (share0 + share1) & 1U;
}

void verify_threshold(std::uint32_t threshold,
                      std::initializer_list<std::uint32_t> values) {
    auto keys = keyGenDCF(kScoreBits, 1, threshold, 1);
    for (const auto value : values) {
        const auto actual = eval_less_than(keys.first, keys.second, value);
        const auto expected = value < threshold ? 1U : 0U;
        require(actual == expected,
                "VFSS DCF reconstruction does not match raw unsigned x < threshold");
    }
    freeDCFKeyPackPair(keys);
}

}  // namespace

int main() {
    for (int i = 0; i < 256; ++i) {
        FSSConfig::prngs[i].SetSeed(osuCrypto::toBlock(0x4d315f444346ULL, i));
    }

    verify_threshold(0U, {0U, 1U, UINT32_MAX});
    verify_threshold(1U, {0U, 1U, 2U});
    verify_threshold(0x80000000U,
                     {0U, 0x7fffffffU, 0x80000000U, 0x80000001U, UINT32_MAX});
    verify_threshold(UINT32_MAX, {0U, UINT32_MAX - 1U, UINT32_MAX});

    // The signed score boundary maps into the DCF's unsigned order before
    // calling the raw x < threshold API.
    verify_threshold(moe_topk::signed_score_to_ordered(0U),
                     {moe_topk::signed_score_to_ordered(UINT32_C(0x80000000)),
                      moe_topk::signed_score_to_ordered(UINT32_C(0xffffffff)),
                      moe_topk::signed_score_to_ordered(0U),
                      moe_topk::signed_score_to_ordered(UINT32_C(0x7fffffff))});
    return 0;
}
