#include <FSS/dcf.h>
#include <FSS/freekey.h>
#include <FSS/prng.h>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <vector>

namespace {
void require(bool value) { if (!value) throw std::runtime_error("raw DCF x < threshold conformance"); }

void verify_threshold(int bin, std::uint64_t threshold) {
    const std::uint64_t domain_max = (UINT64_C(1) << bin) - 1;
    auto keys = keyGenDCF(bin, 1, threshold, 1);
    std::vector<std::uint64_t> inputs{0, domain_max, threshold};
    if (threshold > 0) inputs.push_back(threshold - 1);
    if (threshold < domain_max) inputs.push_back(threshold + 1);
    std::mt19937_64 rng(UINT64_C(0x4d32444346) + bin + threshold);
    for (int i = 0; i < 4; ++i) inputs.push_back(rng() & domain_max);
    for (const auto x : inputs) {
        GroupElement share0 = 0, share1 = 0;
        evalDCF(0, &share0, x, keys.first);
        evalDCF(1, &share1, x, keys.second);
        require(((share0 + share1) & 1U) == (x < threshold));
    }
    freeDCFKeyPackPair(keys);
}
}

int main() {
    for (int i = 0; i < 256; ++i) FSSConfig::prngs[i].SetSeed(osuCrypto::toBlock(0x4d32444346ULL, i));
    for (const int bin : {34, 40, 41, 43, 47, 50, 53}) {
        const std::uint64_t domain_max = (UINT64_C(1) << bin) - 1;
        const std::uint64_t priority_max = (UINT64_C(1) << (bin - 1)) - 1;
        for (const auto threshold : {UINT64_C(0), UINT64_C(1), UINT64_C(2), priority_max,
                                     priority_max + 1, domain_max - 1, domain_max}) {
            verify_threshold(bin, threshold);
        }
    }
    return 0;
}
