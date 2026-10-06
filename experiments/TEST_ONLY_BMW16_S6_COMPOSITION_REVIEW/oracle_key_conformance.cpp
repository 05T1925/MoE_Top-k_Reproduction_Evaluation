// TEST_ONLY conformance checker: compare the derived reverse-priority key
// threshold against the repository's frozen stable Top-K oracle.
#include <moe_topk/topk_oracle.h>
#include <moe_topk/protocol_i_priority_key.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::uint64_t reverse_priority(std::uint32_t raw, std::size_t index,
                               std::size_t n, std::uint32_t bits) {
  const auto ordered = raw ^ UINT32_C(0x80000000);
  return (static_cast<std::uint64_t>(ordered) << bits) + (n - 1U - index);
}

std::uint64_t max_real_key(std::size_t n, std::uint32_t bits) {
  return ((UINT64_C(1) << 32U) - 1U) * (UINT64_C(1) << bits) + (n - 1U);
}

std::uint64_t tested_vectors = 0;
std::uint64_t tested_kth = 0;

void check_scores(const std::vector<std::uint32_t>& scores) {
  const auto n = scores.size();
  const auto bits = moe_topk::protocol_i_index_bits(n);
  std::vector<std::uint64_t> keys(n);
  for (std::size_t i = 0; i < n; ++i) {
    keys[i] = reverse_priority(scores[i], i, n, bits);
    const auto project_key = moe_topk::protocol_i_priority_key(scores[i], i, n).value;
    if (keys[i] != max_real_key(n, bits) - project_key) {
      throw std::runtime_error("derived key differs from reversed project key");
    }
  }
  auto sorted = keys;
  std::sort(sorted.begin(), sorted.end());
  if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end()) {
    throw std::runtime_error("real stable key collision");
  }

  for (std::size_t k = 1; k <= n; ++k) {
    const auto expected = moe_topk::top_k_mask(scores, k);
    const auto threshold = sorted[n - k];  // r=n-K+1 from low end
    std::vector<std::uint8_t> got(n, 0);
    for (std::size_t i = 0; i < n; ++i) got[i] = keys[i] >= threshold ? 1U : 0U;
    if (got != expected) {
      throw std::runtime_error("key threshold differs from frozen Top-K oracle");
    }
    ++tested_kth;
  }
  ++tested_vectors;
}

void enumerate(std::size_t n, std::size_t at,
               const std::vector<std::uint32_t>& alphabet,
               std::vector<std::uint32_t>& current) {
  if (at == n) {
    check_scores(current);
    return;
  }
  for (const auto value : alphabet) {
    current[at] = value;
    enumerate(n, at + 1U, alphabet, current);
  }
}

}  // namespace

int main() {
  const std::vector<std::uint32_t> boundary_alphabet = {
      UINT32_C(0x80000000), UINT32_C(0xffffffff), UINT32_C(0),
      UINT32_C(1), UINT32_C(0x7fffffff)};
  std::vector<std::uint32_t> current;
  for (std::size_t n = 1; n <= 4; ++n) {
    current.resize(n);
    enumerate(n, 0, boundary_alphabet, current);
  }

  const std::array<std::vector<std::uint32_t>, 7> directed = {{
      {UINT32_C(0x80000000)},
      {UINT32_C(0x7fffffff)},
      {UINT32_C(0x80000000), UINT32_C(0x7fffffff)},
      {UINT32_C(0x7fffffff), UINT32_C(0x80000000), UINT32_C(0x7fffffff)},
      {UINT32_C(0), UINT32_C(0), UINT32_C(0), UINT32_C(0)},
      {UINT32_C(0xffffffff), UINT32_C(0xffffffff), UINT32_C(0xffffffff)},
      {UINT32_C(0x80000000), UINT32_C(0x80000000), UINT32_C(0x7fffffff),
       UINT32_C(0x7fffffff), UINT32_C(0xffffffff), UINT32_C(0)}}};
  for (const auto& scores : directed) check_scores(scores);

  std::mt19937_64 rng(UINT64_C(0x423136534b455931));
  for (std::size_t trial = 0; trial < 2000; ++trial) {
    const auto n = 1U + (trial % 16U);
    std::vector<std::uint32_t> scores(n);
    for (std::size_t i = 0; i < n; ++i) {
      const auto selector = rng() % 7U;
      if (selector == 0) scores[i] = UINT32_C(0x80000000);
      else if (selector == 1) scores[i] = UINT32_C(0x7fffffff);
      else if (selector == 2) scores[i] = UINT32_C(0);
      else if (selector == 3) scores[i] = UINT32_C(0xffffffff);
      else scores[i] = static_cast<std::uint32_t>(rng());
    }
    check_scores(scores);
  }

  std::cout << "BMW16_S6_TEST_ONLY_ORACLE_KEY_CONFORMANCE PASS vectors="
            << tested_vectors << " kth_cases=" << tested_kth << '\n';
  return 0;
}
