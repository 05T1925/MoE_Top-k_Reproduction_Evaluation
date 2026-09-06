#include <moe_topk/protocol_i_party_package.h>
#include <moe_topk/protocol_i_priority_key.h>
#include <FSS/prng.h>

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace {
using namespace moe_topk;
void require(bool value) { if (!value) throw std::runtime_error("M2.14 conformance"); }
void seed() { for (int i = 0; i < 256; ++i) FSSConfig::prngs[i].SetSeed(osuCrypto::toBlock(0x214, i)); }
ProtocolIPartyPackage package(int party) {
  seed(); ProtocolIPartyPackage result; result.session = 7; result.fingerprint = 8; result.party = party; result.n = 2; result.k = 1; result.comparison_bits = 34; result.node_mask_shares = {0, 0};
  ProtocolIUcmpMaterial edge(34, 0, 0); result.edge_materials.emplace_back(0, 1, edge.export_party_material(party));
  for (const auto stage : {UINT8_C(1), UINT8_C(2)}) for (std::uint32_t slot = 0; slot != 2; ++slot) { ProtocolIUcmpMaterial item(34, slot + 1, slot + 3); ProtocolIScoreInputPartyMaterial score(slot, stage, 0, 0, item.export_party_material(party)); if (stage == 1) result.carry_materials.push_back(std::move(score)); else result.sign_materials.push_back(std::move(score)); }
  return result;
}
}
int main() { try {
  const std::vector<std::uint32_t> raw{UINT32_C(0x80000000),UINT32_C(0x80000001),UINT32_MAX,0,1,UINT32_C(0x7ffffffe),UINT32_C(0x7fffffff)};
  for (const auto n : {UINT32_C(1),UINT32_C(2),UINT32_C(3),UINT32_C(5),UINT32_C(17),UINT32_C(31),UINT32_C(128)}) { std::uint32_t padded = 2; while (padded < n) padded <<= 1U; std::uint8_t bits = 0; for (auto value = padded - 1U; value != 0; value >>= 1U) ++bits; const auto mask = (UINT64_C(1) << (33U + bits)) - 1U; for (std::uint32_t index = 0; index < n; ++index) { const auto word = raw[index % raw.size()]; const auto q = static_cast<std::uint32_t>(INT32_MAX - word); require((((static_cast<std::uint64_t>(q) << bits) + index) & mask) == protocol_i_priority_key(word, index, padded).value); } }
  auto valid = serialize_party_package(package(0)); require(deserialize_party_package(valid, 0).carry_materials.size() == 2); bool rejected = false; try { (void)deserialize_party_package(valid, 1); } catch (...) { rejected = true; } require(rejected); valid.push_back(0); rejected = false; try { (void)deserialize_party_package(valid, 0); } catch (...) { rejected = true; } require(rejected); return 0;
} catch (...) { return 1; } }
