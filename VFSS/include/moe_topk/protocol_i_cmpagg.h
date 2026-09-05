#pragma once
#include <cstdint>
#include <vector>
namespace moe_topk { // C: additive priority-key CmpAgg core, no opening/reconstruction.
class ProtocolIUcmpMaterial;
class ProtocolIUcmpPartyMaterial;
std::uint64_t protocol_i_mask_priority_key_share(int comparison_bits, std::uint64_t key_share, std::uint64_t mask_share);
std::vector<std::uint64_t> protocol_i_cmpagg_eval_party(int party, int comparison_bits, const std::vector<std::uint64_t>& masked_keys, std::vector<ProtocolIUcmpPartyMaterial>& edge_materials);
}
