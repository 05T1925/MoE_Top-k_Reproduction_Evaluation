#pragma once
#include <cstdint>
#include <vector>
#include <moe_topk/protocol_i_benes.h>
#include <moe_topk/protocol_i_share_translation.h>
namespace moe_topk {
using ProtocolIPermuteShareRecord = ProtocolIBlock192;
struct ProtocolIPermuteShareConfig { std::uint64_t session,fingerprint,offline_material_id,online_message_id; std::uint32_t n,subpermutation_size; std::uint8_t permutation_owner_party; int timeout_ms; };
struct ProtocolIPermuteShareCounters { ProtocolIChosenOtCounters offline_ot; std::uint64_t offline_translation_instances=0,offline_opv_instances=0,offline_chosen_ot_items=0,online_sent_bytes=0,online_received_bytes=0,ps_online_rounds=1; };
struct ProtocolIPermuteSharePoResult { std::vector<ProtocolIPermuteShareRecord> share; ProtocolIPermuteShareCounters counters; };
struct ProtocolIPermuteShareDoResult { std::vector<ProtocolIPermuteShareRecord> share; ProtocolIPermuteShareCounters counters; };
struct ProtocolIPermuteSharePoMaterial {
  ProtocolIPermuteShareConfig config{}; ProtocolIPermutation permutation;
  std::vector<ProtocolIBenesLayer> layers;
  std::vector<std::vector<ProtocolIBlock192>> delta;
  ProtocolIPermuteShareCounters counters{}; bool consumed = false;
  ProtocolIPermuteSharePoMaterial() = default;
  ProtocolIPermuteSharePoMaterial(const ProtocolIPermuteSharePoMaterial&) = delete;
  ProtocolIPermuteSharePoMaterial& operator=(const ProtocolIPermuteSharePoMaterial&) = delete;
  ProtocolIPermuteSharePoMaterial(ProtocolIPermuteSharePoMaterial&&) = default;
  ProtocolIPermuteSharePoMaterial& operator=(ProtocolIPermuteSharePoMaterial&&) = default;
};
struct ProtocolIPermuteShareDoMaterial {
  ProtocolIPermuteShareConfig config{};
  std::vector<std::vector<ProtocolIBlock192>> a, b;
  ProtocolIPermuteShareCounters counters{}; bool consumed = false;
  ProtocolIPermuteShareDoMaterial() = default;
  ProtocolIPermuteShareDoMaterial(const ProtocolIPermuteShareDoMaterial&) = delete;
  ProtocolIPermuteShareDoMaterial& operator=(const ProtocolIPermuteShareDoMaterial&) = delete;
  ProtocolIPermuteShareDoMaterial(ProtocolIPermuteShareDoMaterial&&) = default;
  ProtocolIPermuteShareDoMaterial& operator=(ProtocolIPermuteShareDoMaterial&&) = default;
};
ProtocolIPermuteSharePoMaterial protocol_i_permute_share_preprocess_po(const ProtocolIPermuteShareConfig&, int offline_fd, const ProtocolIPermutation&);
ProtocolIPermuteShareDoMaterial protocol_i_permute_share_preprocess_do(const ProtocolIPermuteShareConfig&, int offline_fd);
ProtocolIPermuteSharePoResult protocol_i_permute_share_online_po(const ProtocolIPermuteShareConfig&, int online_fd, const ProtocolIPermutation&, ProtocolIPermuteSharePoMaterial&&);
ProtocolIPermuteShareDoResult protocol_i_permute_share_online_do(const ProtocolIPermuteShareConfig&, int online_fd, const std::vector<ProtocolIPermuteShareRecord>&, ProtocolIPermuteShareDoMaterial&&);
ProtocolIPermuteSharePoResult protocol_i_permute_share_po(const ProtocolIPermuteShareConfig&,int offline_fd,int online_fd,const ProtocolIPermutation& permutation);
ProtocolIPermuteShareDoResult protocol_i_permute_share_do(const ProtocolIPermuteShareConfig&,int offline_fd,int online_fd,const std::vector<ProtocolIPermuteShareRecord>& input);
}  // namespace moe_topk
