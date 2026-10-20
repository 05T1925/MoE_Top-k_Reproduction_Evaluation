#pragma once

#include <cstdint>

namespace moe_topk {

// Versioned score-conversion output-share law. V2 refreshes the complete
// priority-key share in R_(2^(33+h)); it deliberately does not expose a local
// high-part/q share because floor division does not commute with sharing.
enum class ProtocolIScoreShareAbi : std::uint8_t {
  S15InducedV1 = 1,
  FullKeyZeroSumRefreshV2 = 2,
};

}  // namespace moe_topk
