#include <moe_topk/protocol_i_cmpagg.h>
#include <moe_topk/protocol_i_ucmp.h>

#include <stdexcept>

namespace moe_topk {
namespace {

std::uint64_t mask(int bits) {
  if (bits < 34 || bits > 53) {
    throw std::invalid_argument("comparison bits");
  }
  return (UINT64_C(1) << bits) - 1;
}

}  // namespace

std::uint64_t protocol_i_mask_priority_key_share(int comparison_bits, std::uint64_t key_share,
                                                 std::uint64_t mask_share) {
  return (key_share + mask_share) & mask(comparison_bits);
}

std::vector<std::uint64_t> protocol_i_cmpagg_eval_party(
    int party, int comparison_bits, const std::vector<std::uint64_t>& masked_keys,
    std::vector<ProtocolIUcmpPartyMaterial>& edge_materials) {
  if (party < 0 || party > 1 || masked_keys.empty()) {
    throw std::invalid_argument("CmpAgg input");
  }
  const auto ring_mask = mask(comparison_bits);
  for (const auto key : masked_keys) {
    if ((key & ~ring_mask) != 0) {
      throw std::invalid_argument("masked key outside ring");
    }
  }
  if (edge_materials.size() != masked_keys.size() * (masked_keys.size() - 1) / 2) {
    throw std::invalid_argument("edge material count");
  }

  std::vector<std::uint64_t> ranks(masked_keys.size());
  std::size_t edge = 0;
  for (std::size_t left = 0; left < masked_keys.size(); ++left) {
    for (std::size_t right = left + 1; right < masked_keys.size(); ++right) {
      auto& material = edge_materials[edge++];
      if (material.comparison_bits() != comparison_bits || material.party_id() != party) {
        throw std::invalid_argument("edge material mismatch");
      }
      const auto less_than = material.eval_strict_lt(masked_keys[left], masked_keys[right]);
      ranks[left] += (party == 0 ? 1 : 0) - less_than;
      ranks[right] += less_than;
    }
  }
  return ranks;
}

}  // namespace moe_topk
