#pragma once

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace moe_topk_e9_test {
struct Fixture {
  std::vector<std::uint32_t> scores;
  std::uint32_t k;
  std::uint32_t r;
  const char* label;
};

inline std::vector<Fixture> fixtures(std::uint32_t d) {
  if (d != 16 && d != 32 && d != 64)
    throw std::invalid_argument("E9 fixture D must be 16, 32 or 64");
  std::vector<std::uint32_t> ties(d - 1, 7);
  std::vector<std::uint32_t> extremes(d);
  std::vector<std::uint32_t> mixed(d - 1);
  std::vector<std::uint32_t> descending(d);
  std::vector<std::uint32_t> bounds(d - 1);
  std::vector<std::uint32_t> seeded(d);
  std::uint64_t state = UINT64_C(0x9e3779b97f4a7c15) ^ d;
  for (std::uint32_t i = 0; i < d; ++i) {
    extremes[i] = i % 3 == 0 ? UINT32_C(0x80000000) :
                  i % 3 == 1 ? UINT32_C(0x7fffffff) : UINT32_C(0xffffffff);
    descending[i] = static_cast<std::uint32_t>(d - i);
    state ^= state << 13; state ^= state >> 7; state ^= state << 17;
    seeded[i] = static_cast<std::uint32_t>(state);
    if (i < d - 1) {
      mixed[i] = i % 4 == 0 ? static_cast<std::uint32_t>(-5) :
                 i % 4 == 1 ? 0U : i % 4 == 2 ? 13U : static_cast<std::uint32_t>(-1);
      bounds[i] = i % 2 ? UINT32_C(0x7fffffff) : UINT32_C(0x80000000);
    }
  }
  return {{std::move(ties),1,2,"ties_nonpower_k1_r2"},
          {std::move(extremes),d,5,"extremes_power_kn_r5"},
          {std::move(mixed),(d-1)/2,5,"mixed_nonpower_kmid_r5"},
          {std::move(descending),1,5,"ordered_power_k1_r5"},
          {std::move(bounds),d-1,2,"bounds_nonpower_kn_r2"},
          {std::move(seeded),d/2,2,"seeded_power_kmid_r2"}};
}
}  // namespace moe_topk_e9_test
