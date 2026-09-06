#pragma once
#include <moe_topk/protocol_i_ucmp.h>
#include <cstdint>
#include <utility>
#include <vector>
namespace moe_topk {
struct ProtocolIEdgePartyMaterial {
  std::uint32_t left = 0, right = 0;
  ProtocolIUcmpPartyMaterial material;

  ProtocolIEdgePartyMaterial() = default;
  ProtocolIEdgePartyMaterial(std::uint32_t edge_left,
                             std::uint32_t edge_right,
                             ProtocolIUcmpPartyMaterial&& edge_material)
      : left(edge_left), right(edge_right), material(std::move(edge_material)) {}
  ProtocolIEdgePartyMaterial(const ProtocolIEdgePartyMaterial&) = delete;
  ProtocolIEdgePartyMaterial& operator=(const ProtocolIEdgePartyMaterial&) = delete;
  ProtocolIEdgePartyMaterial(ProtocolIEdgePartyMaterial&&) noexcept = default;
  ProtocolIEdgePartyMaterial& operator=(ProtocolIEdgePartyMaterial&&) noexcept = default;
};
// C: M2.14-specific, input-independent material for one raw-score slot.  The
// stage/slot binding prevents a carry key from being reused as a sign key.
struct ProtocolIScoreInputPartyMaterial {
  std::uint32_t slot = 0;
  std::uint8_t stage = 0;  // 1 = carry, 2 = sign
  std::uint64_t left_mask_share = 0, right_mask_share = 0;
  ProtocolIUcmpPartyMaterial material;

  ProtocolIScoreInputPartyMaterial() = default;
  ProtocolIScoreInputPartyMaterial(std::uint32_t input_slot, std::uint8_t input_stage,
                                   std::uint64_t left_share, std::uint64_t right_share,
                                   ProtocolIUcmpPartyMaterial&& input_material)
      : slot(input_slot), stage(input_stage), left_mask_share(left_share),
        right_mask_share(right_share), material(std::move(input_material)) {}
  ProtocolIScoreInputPartyMaterial(const ProtocolIScoreInputPartyMaterial&) = delete;
  ProtocolIScoreInputPartyMaterial& operator=(const ProtocolIScoreInputPartyMaterial&) = delete;
  ProtocolIScoreInputPartyMaterial(ProtocolIScoreInputPartyMaterial&&) noexcept = default;
  ProtocolIScoreInputPartyMaterial& operator=(ProtocolIScoreInputPartyMaterial&&) noexcept = default;
};
struct ProtocolIPartyPackage {
  std::uint64_t session = 0, fingerprint = 0;
  int party = -1, comparison_bits = 0;
  std::uint32_t n = 0, k = 0;
  std::vector<std::uint64_t> node_mask_shares;
  std::vector<ProtocolIEdgePartyMaterial> edge_materials;
  std::vector<ProtocolIScoreInputPartyMaterial> carry_materials;
  std::vector<ProtocolIScoreInputPartyMaterial> sign_materials;
};
std::vector<std::uint8_t> serialize_party_package(const ProtocolIPartyPackage& package);
ProtocolIPartyPackage deserialize_party_package(const std::vector<std::uint8_t>& bytes, int expected_party);
}
