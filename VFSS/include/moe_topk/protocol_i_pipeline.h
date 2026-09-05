#pragma once

#include <cstdint>
#include <vector>

#include <moe_topk/protocol_i_cmpagg.h>
#include <moe_topk/protocol_i_party_package.h>
#include <moe_topk/protocol_i_secret_shared_shuffle.h>

namespace moe_topk {
struct ProtocolIPriorityPipelineConfig {
  std::uint64_t session = 0, fingerprint = 0;
  std::uint32_t n = 0, k = 0;
  std::uint8_t comparison_bits = 0, party = 0;
  int timeout_ms = 0;
};
struct ProtocolIPriorityPipelineOutput { std::vector<std::uint8_t> xor_mask_share; };
ProtocolIPriorityPipelineOutput protocol_i_priority_pipeline_party(
    const ProtocolIPriorityPipelineConfig&, ProtocolIPartyPackage&&,
    ProtocolIShufflePartyMaterial&, const std::vector<std::uint64_t>& priority_key_share,
    const std::array<int, 2>& forward_fds, int cmpagg_fd, int rank_reveal_fd,
    const std::array<int, 2>& reverse_fds);
}  // namespace moe_topk
