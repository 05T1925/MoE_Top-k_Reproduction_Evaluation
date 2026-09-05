#pragma once
#include <cstdint>
#include <vector>
#include <moe_topk/protocol_i_permutation.h>
namespace moe_topk {
struct ProtocolIBenesLayout { std::uint32_t n=0,t=0,d=0; std::vector<std::vector<std::vector<std::uint32_t>>> groups; };
struct ProtocolIBenesLayer { ProtocolIPermutation permutation; std::vector<ProtocolIPermutation> local_permutations; };
struct ProtocolIBenesDecomposition { ProtocolIBenesLayout layout; std::vector<ProtocolIBenesLayer> layers; };
ProtocolIBenesLayout protocol_i_benes_layout(std::uint32_t n, std::uint32_t t);
ProtocolIBenesDecomposition protocol_i_benes_decompose(const ProtocolIPermutation& permutation, std::uint32_t t);
std::uint64_t protocol_i_benes_translation_count(const ProtocolIBenesLayout& layout);
}  // namespace moe_topk
