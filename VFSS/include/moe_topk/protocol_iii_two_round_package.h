#pragma once

#include <moe_topk/protocol_iii_two_round.h>

#include <cstdint>
#include <vector>

namespace moe_topk {

// Same-build/same-architecture offline envelope: the embedded M2 CmpAgg
// package retains its existing FSS key ABI. Field elements and all new
// metadata are canonical. material_id is a controller-managed one-shot ID;
// this codec does not implement persistent replay protection.
struct ProtocolIIITwoRoundOfflineBundle {
  std::uint64_t material_id = 0;
  ProtocolIIITwoRoundPartyMaterial material;
};

[[nodiscard]] std::vector<std::uint8_t> protocol_iii_two_round_serialize_bundle(
    const ProtocolIIITwoRoundConfig& config,
    const ProtocolIIITwoRoundPartyMaterial& material,
    std::uint64_t material_id);

[[nodiscard]] ProtocolIIITwoRoundOfflineBundle
protocol_iii_two_round_deserialize_bundle(
    const ProtocolIIITwoRoundConfig& expected_config,
    std::uint64_t expected_material_id,
    const std::vector<std::uint8_t>& bytes);

}  // namespace moe_topk
