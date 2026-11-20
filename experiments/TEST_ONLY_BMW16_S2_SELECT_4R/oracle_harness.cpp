#include <moe_topk/topk_oracle.h>
#include <moe_topk/score_semantics.h>

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

int main() {
  std::size_t cases = 0;
  if (!(std::cin >> cases)) return 2;
  for (std::size_t c = 0; c < cases; ++c) {
    std::size_t n = 0;
    std::size_t k = 0;
    if (!(std::cin >> n >> k)) return 3;
    std::vector<std::uint32_t> scores(n);
    for (std::size_t i = 0; i < n; ++i) {
      std::int64_t signed_value = 0;
      if (!(std::cin >> signed_value)) return 4;
      if (signed_value < INT32_MIN || signed_value > INT32_MAX) return 5;
      scores[i] = moe_topk::encode_signed_score(static_cast<std::int32_t>(signed_value));
    }
    const auto mask = moe_topk::top_k_mask(scores, k);
    const auto ranks = moe_topk::stable_ranks_cmpagg(scores);
    std::size_t selected = n;
    for (std::size_t i = 0; i < n; ++i) {
      if (ranks[i] == k - 1) selected = i;
    }
    if (selected == n) return 6;
    std::cout << selected << ' ';
    for (const auto bit : mask) std::cout << static_cast<unsigned>(bit);
    std::cout << '\n';
  }
  return 0;
}
