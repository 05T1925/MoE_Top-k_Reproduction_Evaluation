#pragma once
#include <moe_topk/protocol_i_ucmp.h>
#include <cstdint>
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
struct ProtocolIPartyPackage {
  std::uint64_t session = 0, fingerprint = 0;
  int party = -1, comparison_bits = 0;
  std::uint32_t n = 0, k = 0;
  std::vector<std::uint64_t> node_mask_shares;
  std::vector<ProtocolIEdgePartyMaterial> edge_materials;
};
std::vector<std::uint8_t> serialize_party_package(const ProtocolIPartyPackage& package);
ProtocolIPartyPackage deserialize_party_package(const std::vector<std::uint8_t>& bytes, int expected_party);
}
