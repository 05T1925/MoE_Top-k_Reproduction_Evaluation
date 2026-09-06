#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <moe_topk/protocol_i_cmpagg.h>
#include <moe_topk/protocol_i_party_package.h>
#include <moe_topk/protocol_i_secret_shared_shuffle.h>

namespace moe_topk {

// C: public layout contract for the modular Protocol-I candidate. The padded
// domain is part of the package and frame binding; callers receive only a
// logical-domain mask.
struct ProtocolIInputLayout {
  std::uint32_t logical_n = 0, padded_n = 0, k = 0;
  std::uint8_t index_bits = 0, minimum_comparison_bits = 0;
};
ProtocolIInputLayout protocol_i_make_input_layout(std::uint32_t logical_n, std::uint32_t k);

struct ProtocolIPriorityPipelineConfig {
  std::uint64_t session = 0, fingerprint = 0;
  std::uint32_t logical_n = 0, padded_n = 0, k = 0;
  std::uint8_t comparison_bits = 0, party = 0;
  int timeout_ms = 0;
};

struct ProtocolIPriorityPipelineMetrics {
  std::uint64_t forward_sent_bytes = 0, forward_received_bytes = 0;
  std::uint64_t cmpagg_sent_bytes = 0, cmpagg_received_bytes = 0;
  std::uint64_t rank_reveal_sent_bytes = 0, rank_reveal_received_bytes = 0;
  std::uint64_t reverse_sent_bytes = 0, reverse_received_bytes = 0;
  std::uint64_t comparison_edges = 0, raw_dcf_calls = 0;
  std::uint64_t forward_rounds = 2, cmpagg_rounds = 1, rank_reveal_rounds = 1,
                reverse_rounds = 2, online_rounds = 6;
};

struct ProtocolIPriorityPipelineOutput {
  std::vector<std::uint8_t> xor_mask_share;
  ProtocolIPriorityPipelineMetrics metrics;
};

// Input shares are padded comparison-ring priority keys. Legacy 32-bit score
// shares require a widen-and-carry protocol and are intentionally
// NOT_IMPLEMENTED rather than silently truncated.
ProtocolIPriorityPipelineOutput protocol_i_priority_pipeline_party(
    const ProtocolIPriorityPipelineConfig&, ProtocolIPartyPackage&&,
    ProtocolIShufflePartyMaterial&, const std::vector<std::uint64_t>& priority_key_share,
    const std::array<int, 2>& forward_fds, int cmpagg_fd, int rank_reveal_fd,
    const std::array<int, 2>& reverse_fds);
}  // namespace moe_topk
