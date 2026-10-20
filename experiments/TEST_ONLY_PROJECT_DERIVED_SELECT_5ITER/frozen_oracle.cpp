#include <moe_topk/topk_oracle.h>

#include <cstdint>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <vector>

int main() {
  std::size_t n = 0;
  std::size_t k = 0;
  while (std::cin >> n >> k) {
    std::vector<std::uint32_t> scores(n);
    for (auto& raw : scores) {
      std::int64_t signed_raw = 0;
      if (!(std::cin >> signed_raw)) return 2;
      raw = static_cast<std::uint32_t>(static_cast<std::int32_t>(signed_raw));
    }
    try {
      const auto mask = moe_topk::top_k_mask(scores, k);
      std::vector<std::size_t> order(n);
      std::iota(order.begin(), order.end(), 0);
      std::sort(order.begin(), order.end(), [&](std::size_t left, std::size_t right) {
        return moe_topk::top_k_precedes(scores, left, right);
      });
      std::cout << order[k - 1] << ' ';
      for (const auto bit : mask) std::cout << static_cast<unsigned>(bit);
      std::cout << '\n';
    } catch (const std::exception&) {
      std::cout << "INVALID\n";
    }
  }
  return 0;
}
