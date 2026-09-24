#include <moe_topk/protocol_i_permute_share.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {
using namespace moe_topk;

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

ProtocolIBlock192 clear_add(ProtocolIBlock192 left,
                            const ProtocolIBlock192& right) {
  return {left.word0 + right.word0, left.word1 + right.word1,
          left.word2 + right.word2};
}

template <typename T>
std::vector<T> clear_apply(const ProtocolIPermutation& permutation,
                           const std::vector<T>& input) {
  require(permutation.size() == input.size(), "clear permutation shape");
  std::vector<T> output(input.size());
  for (std::size_t index = 0; index < input.size(); ++index)
    output[index] = input[permutation[index]];
  return output;
}

ProtocolIPermutation permutation(std::uint32_t n, int mode) {
  ProtocolIPermutation result(n);
  for (std::uint32_t index = 0; index < n; ++index) result[index] = index;
  if (mode == 1) {
    std::reverse(result.begin(), result.end());
  } else if (mode == 2) {
    for (std::uint32_t index = 0; index < n; index += 2)
      std::swap(result[index], result[index + 1]);
  } else if (mode == 3) {
    for (std::uint32_t index = 0; index < n; ++index)
      result[index] = (index + 1) % n;
  } else if (mode == 4) {
    std::mt19937_64 random(UINT64_C(0x2100) + n);
    std::shuffle(result.begin(), result.end(), random);
  }
  return result;
}

std::vector<ProtocolIBlock192> records(std::uint32_t n, int mode) {
  std::vector<ProtocolIBlock192> result(n);
  std::mt19937_64 random(UINT64_C(0xc1000000) + n + mode);
  for (std::uint32_t index = 0; index < n; ++index) {
    if (mode == 0) {
      result[index] = {index, UINT64_C(0x100000000) + index,
                       UINT64_C(0x8000000000000000) + index};
    } else if (mode == 1) {
      result[index] = {std::numeric_limits<std::uint64_t>::max() - index,
                       index == 0 ? 0 : std::numeric_limits<std::uint64_t>::max(),
                       index & 1U ? 0 : std::numeric_limits<std::uint64_t>::max()};
    } else {
      result[index] = {random(), random(), random()};
    }
  }
  return result;
}

std::uint32_t log2_exact(std::uint32_t value) {
  std::uint32_t result = 0;
  while (value > 1) {
    ++result;
    value >>= 1;
  }
  return result;
}

void check_benes(std::uint32_t n, std::uint32_t t,
                 const ProtocolIPermutation& target) {
  const auto decomposition = protocol_i_benes_decompose(target, t);
  const auto expected_d = 2U * (log2_exact(n) / log2_exact(t)) - 1U;
  require(decomposition.layout.d == expected_d, "Benes layer count");
  require(decomposition.layers.size() == expected_d, "Benes decomposition size");

  std::vector<std::uint32_t> composed(n);
  for (std::uint32_t index = 0; index < n; ++index) composed[index] = index;
  for (std::size_t layer_index = 0; layer_index < decomposition.layers.size();
       ++layer_index) {
    const auto& layer = decomposition.layers[layer_index];
    const auto& groups = decomposition.layout.groups[layer_index];
    require(groups.size() == n / t, "Benes disjoint group count");
    require(layer.local_permutations.size() == groups.size(),
            "Benes local permutation count");
    std::vector<bool> covered(n);
    for (std::size_t group_index = 0; group_index < groups.size(); ++group_index) {
      const auto& group = groups[group_index];
      const auto& local = layer.local_permutations[group_index];
      require(group.size() == t && local.size() == t, "Benes group shape");
      protocol_i_validate_permutation(local);
      for (std::uint32_t local_index = 0; local_index < t; ++local_index) {
        const auto wire = group[local_index];
        require(wire < n && !covered[wire], "Benes groups not disjoint");
        covered[wire] = true;
        require(group[local[local_index]] == layer.permutation[wire],
                "Benes local direction");
      }
    }
    require(std::all_of(covered.begin(), covered.end(), [](bool value) { return value; }),
            "Benes group coverage");
    composed = clear_apply(layer.permutation, composed);
  }
  require(composed == target, "Benes composition direction");
}

void expect_bad_shape(std::uint32_t n, std::uint32_t t) {
  bool rejected = false;
  try {
    (void)protocol_i_benes_layout(n, t);
  } catch (const std::invalid_argument&) {
    rejected = true;
  }
  require(rejected, "unsupported Benes shape accepted");
}

void arithmetic_contract() {
  static_assert(sizeof(ProtocolIBlock192) == 3 * sizeof(std::uint64_t));
  const ProtocolIBlock192 left{std::numeric_limits<std::uint64_t>::max(), 1, 0};
  const ProtocolIBlock192 right{1, std::numeric_limits<std::uint64_t>::max(),
                                std::numeric_limits<std::uint64_t>::max()};
  const ProtocolIBlock192 sum{0, 0, std::numeric_limits<std::uint64_t>::max()};
  require(protocol_i_record_add(left, right) == sum, "record lane addition");
  require(protocol_i_record_sub(sum, right) == left, "record lane subtraction");
}

void run_case(std::uint32_t n, std::uint32_t t, int permutation_mode,
              int record_mode, std::uint8_t owner, std::uint64_t material_id) {
  int offline[2]{}, online[2]{};
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, offline) == 0 &&
              ::socketpair(AF_UNIX, SOCK_STREAM, 0, online) == 0,
          "socketpair");
  const auto pi = permutation(n, permutation_mode);
  const ProtocolIPermuteShareConfig config{
      UINT64_C(0x210000) + n, UINT64_C(0x220000) + t, material_id,
      material_id + 100, n, t, owner, 15000};

  ProtocolIPermuteSharePoMaterial po_material;
  ProtocolIPermuteShareDoMaterial do_material;
  std::exception_ptr po_error, do_error;
  std::thread po_preprocess([&] {
    try {
      po_material = protocol_i_permute_share_preprocess_po(config, offline[0], pi);
    } catch (...) {
      po_error = std::current_exception();
    }
  });
  std::thread do_preprocess([&] {
    try {
      do_material = protocol_i_permute_share_preprocess_do(config, offline[1]);
    } catch (...) {
      do_error = std::current_exception();
    }
  });
  po_preprocess.join();
  do_preprocess.join();
  ::close(offline[0]);
  ::close(offline[1]);
  if (po_error) std::rethrow_exception(po_error);
  if (do_error) std::rethrow_exception(do_error);
  require(do_material.w.size() == n, "w is not preprocessing material");

  // The input is chosen only after all preprocessing has completed.
  const auto input = records(n, record_mode);
  ProtocolIPermuteSharePoResult po_result;
  ProtocolIPermuteShareDoResult do_result;
  std::thread po_online([&] {
    try {
      po_result = protocol_i_permute_share_online_po(
          config, online[0], pi, std::move(po_material));
    } catch (...) {
      po_error = std::current_exception();
    }
  });
  std::thread do_online([&] {
    try {
      do_result = protocol_i_permute_share_online_do(
          config, online[1], input, std::move(do_material));
    } catch (...) {
      do_error = std::current_exception();
    }
  });
  po_online.join();
  do_online.join();
  ::close(online[0]);
  ::close(online[1]);
  if (po_error) std::rethrow_exception(po_error);
  if (do_error) std::rethrow_exception(do_error);

  const auto expected = clear_apply(pi, input);
  require(po_result.share.size() == expected.size() &&
              do_result.share.size() == expected.size(),
          "Permute+Share output shape");
  for (std::size_t index = 0; index < expected.size(); ++index)
    require(clear_add(po_result.share[index], do_result.share[index]) == expected[index],
            "Permute+Share clear oracle");
  const auto layout = protocol_i_benes_layout(n, t);
  require(po_result.counters.offline_translation_instances ==
              protocol_i_benes_translation_count(layout),
          "translation count");
  require(po_result.counters.ps_online_rounds == 1 &&
              do_result.counters.ps_online_rounds == 1,
          "Permute+Share online rounds");
  require(po_result.counters.online_received_bytes ==
              do_result.counters.online_sent_bytes,
          "Permute+Share online bytes");
}

void basic_contracts() {
  arithmetic_contract();
  for (const auto n : {2U, 4U, 8U, 16U, 64U, 256U}) {
    const auto pi = permutation(n, 4);
    const auto inverse = protocol_i_inverse_permutation(pi);
    std::vector<std::uint32_t> values(n);
    for (std::uint32_t index = 0; index < n; ++index) values[index] = index;
    require(clear_apply(inverse, clear_apply(pi, values)) == values,
            "inverse convention");
  }
  check_benes(4, 2, permutation(4, 3));
  check_benes(16, 4, permutation(16, 4));
  check_benes(256, 16, permutation(256, 4));
  expect_bad_shape(1, 1);
  expect_bad_shape(6, 2);
  expect_bad_shape(8, 3);
  expect_bad_shape(4, 8);
  expect_bad_shape(8, 4);  // log2(N) % log2(T) != 0.
  bool bad_permutation = false;
  try {
    protocol_i_validate_permutation({0, 0});
  } catch (const std::invalid_argument&) {
    bad_permutation = true;
  }
  require(bad_permutation, "invalid permutation accepted");
}
}  // namespace

int main() {
  try {
    basic_contracts();
    std::uint64_t material_id = 1;
    const std::array<std::pair<std::uint32_t, std::uint32_t>, 10> shapes{{
        {2, 2}, {4, 2}, {4, 4}, {8, 2}, {8, 8}, {16, 2}, {16, 4},
        {16, 16}, {64, 4}, {256, 16}}};
    for (const auto [n, t] : shapes)
      for (int mode = 0; mode < 5; ++mode)
        run_case(n, t, mode, mode % 3, static_cast<std::uint8_t>(mode & 1),
                 material_id++);
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
