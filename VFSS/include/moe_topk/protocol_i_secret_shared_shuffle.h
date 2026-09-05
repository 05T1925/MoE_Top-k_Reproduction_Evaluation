#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <moe_topk/protocol_i_permute_share.h>

namespace moe_topk {

// Project-extension composition of four one-shot Permute+Share materials.
// A material belongs to exactly one online party and never contains the other
// party's permutation or a jointly-owned state object.
struct ProtocolIShufflePartyConfig {
  std::uint64_t session = 0, fingerprint = 0, material_id_base = 0,
                online_message_id_base = 0;
  std::uint32_t n = 0, subpermutation_size = 0;
  std::uint8_t party = 0;
  int timeout_ms = 0;
};

struct ProtocolIShufflePartyCounters {
  ProtocolIPermuteShareCounters forward_first, forward_second;
  ProtocolIPermuteShareCounters reverse_first, reverse_second;
  std::uint64_t forward_online_rounds = 2, reverse_online_rounds = 2,
                total_roundtrip_rounds = 4;
};

struct ProtocolIShufflePartyMaterial {
  ProtocolIShufflePartyConfig config{};
  ProtocolIPermutation own_permutation, own_inverse_permutation;
  ProtocolIPermuteSharePoMaterial forward_po_first, forward_po_second;
  ProtocolIPermuteShareDoMaterial forward_do_first, forward_do_second;
  ProtocolIPermuteSharePoMaterial reverse_po_first, reverse_po_second;
  ProtocolIPermuteShareDoMaterial reverse_do_first, reverse_do_second;
  bool forward_consumed = false, reverse_consumed = false;
  ProtocolIShufflePartyMaterial() = default;
  ProtocolIShufflePartyMaterial(const ProtocolIShufflePartyMaterial&) = delete;
  ProtocolIShufflePartyMaterial& operator=(const ProtocolIShufflePartyMaterial&) = delete;
  ProtocolIShufflePartyMaterial(ProtocolIShufflePartyMaterial&&) = default;
  ProtocolIShufflePartyMaterial& operator=(ProtocolIShufflePartyMaterial&&) = default;
};

struct ProtocolIShufflePartyOutput {
  std::vector<ProtocolIBlock192> share;
  ProtocolIShufflePartyCounters counters;
};

ProtocolIShufflePartyMaterial protocol_i_shuffle_preprocess_party(
    const ProtocolIShufflePartyConfig& config, const std::array<int, 4>& offline_fds,
    const ProtocolIPermutation& own_permutation);
ProtocolIShufflePartyOutput protocol_i_shuffle_forward_party(
    int party, const std::array<int, 2>& online_fds,
    const std::vector<ProtocolIBlock192>& input, ProtocolIShufflePartyMaterial& material);
ProtocolIShufflePartyOutput protocol_i_shuffle_reverse_party(
    int party, const std::array<int, 2>& online_fds,
    const std::vector<ProtocolIBlock192>& shuffled_carrier,
    ProtocolIShufflePartyMaterial& material);
}  // namespace moe_topk
