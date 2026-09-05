#pragma once
#include <cstdint>
namespace moe_topk {
struct ProtocolIShufflePartyConfig { std::uint64_t session=0,fingerprint=0; std::uint32_t n=0,subpermutation_size=0; int timeout_ms=0; };
struct ProtocolIShufflePartyCounters { std::uint64_t forward_online_rounds=2,reverse_online_rounds=2,total_roundtrip_rounds=4; };
}  // namespace moe_topk
