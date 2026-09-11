#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <moe_topk/protocol_i_ucmp.h>

namespace moe_topk {

inline constexpr char kProtocolIDealerCandidateLabel[] =
    "m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate";

struct ProtocolIDealerCandidatePublicConfig {
  std::uint64_t session = 0, fingerprint = 0, material_id = 0;
  std::uint32_t logical_n = 0, padded_n = 0, k = 0;
  std::uint8_t comparison_bits = 0, rank_bits = 0;
};

struct ProtocolIDealerCandidateConfig : ProtocolIDealerCandidatePublicConfig {
  std::uint8_t party = 0;
};

struct ProtocolIDealerCandidateEdgeMaterial {
  std::uint32_t left = 0, right = 0, slot = 0;
  std::uint8_t stage = 1;
  ProtocolIUcmpPartyMaterial material;

  ProtocolIDealerCandidateEdgeMaterial() = default;
  ProtocolIDealerCandidateEdgeMaterial(std::uint32_t edge_left,
                                       std::uint32_t edge_right,
                                       std::uint32_t edge_slot,
                                       ProtocolIUcmpPartyMaterial&& edge_material)
      : left(edge_left), right(edge_right), slot(edge_slot), material(std::move(edge_material)) {}
  ProtocolIDealerCandidateEdgeMaterial(const ProtocolIDealerCandidateEdgeMaterial&) = delete;
  ProtocolIDealerCandidateEdgeMaterial& operator=(const ProtocolIDealerCandidateEdgeMaterial&) = delete;
  ProtocolIDealerCandidateEdgeMaterial(ProtocolIDealerCandidateEdgeMaterial&&) noexcept = default;
  ProtocolIDealerCandidateEdgeMaterial& operator=(ProtocolIDealerCandidateEdgeMaterial&&) noexcept = default;
};

struct ProtocolIDealerCandidatePackage {
  std::string implementation_label = kProtocolIDealerCandidateLabel;
  ProtocolIDealerCandidateConfig config{};
  std::vector<std::uint64_t> r_share;
  std::vector<ProtocolIDealerCandidateEdgeMaterial> edge_materials;

  // This field is populated by deserialization and is not serialized. It is
  // used only to report the bytes delivered by the offline channel.
  std::uint64_t serialized_bytes = 0;
  bool consumed = false;

  ProtocolIDealerCandidatePackage() = default;
  ProtocolIDealerCandidatePackage(const ProtocolIDealerCandidatePackage&) = delete;
  ProtocolIDealerCandidatePackage& operator=(const ProtocolIDealerCandidatePackage&) = delete;
  ProtocolIDealerCandidatePackage(ProtocolIDealerCandidatePackage&&) noexcept = default;
  ProtocolIDealerCandidatePackage& operator=(ProtocolIDealerCandidatePackage&&) noexcept = default;
};

struct ProtocolIDealerCandidatePackagePair {
  ProtocolIDealerCandidatePackage party0;
  ProtocolIDealerCandidatePackage party1;
};

ProtocolIDealerCandidatePackagePair protocol_i_dealer_candidate_preprocess(
    const ProtocolIDealerCandidatePublicConfig& config);

std::vector<std::uint8_t> serialize_dealer_candidate_package(
    const ProtocolIDealerCandidatePackage& package);
ProtocolIDealerCandidatePackage deserialize_dealer_candidate_package(
    const std::vector<std::uint8_t>& bytes, int expected_party);

}  // namespace moe_topk
