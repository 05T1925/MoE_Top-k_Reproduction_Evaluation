#pragma once

#include <cstdint>
#include <vector>

#include <moe_topk/protocol_i_party_package.h>

namespace moe_topk {

// Public runtime parameters for Protocol III round 1: GRank.
struct ProtocolIIIGrankConfig {
  std::uint64_t session = 0;
  std::uint64_t fingerprint = 0;

  // GRank node masks, comparison edges and online vectors are sized by
  // logical_n. padded_n remains the public priority-key input layout and
  // determines the priority-key comparison width.
  std::uint32_t logical_n = 0;
  std::uint32_t padded_n = 0;
  std::uint32_t k = 0;

  std::uint8_t comparison_bits = 0;
  std::uint8_t rank_bits = 0;
  std::uint8_t party = 0;

  int timeout_ms = 0;
};

// Metrics are restricted to the GRank stage. DPF routing and masked
// multiplication metrics belong to later Protocol III stages.
struct ProtocolIIIGrankMetrics {
  std::uint64_t sent_bytes = 0;
  std::uint64_t received_bytes = 0;

  std::uint64_t comparison_edges = 0;
  std::uint64_t ucmp_calls = 0;
  std::uint64_t raw_dcf_calls = 0;

  std::uint64_t online_rounds = 1;
};

struct ProtocolIIIGrankOutput {
  // One additive priority-rank share for each logical input, represented in
  // Z_(2^rank_bits). Padded slots must never appear in this public output.
  std::vector<std::uint64_t> rank_additive_shares;

  ProtocolIIIGrankMetrics metrics;
};

// M3.2 runtime boundary:
//
// priority-key additive shares
//              |
//              v
// protocol_iii_grank_party()
//              |
//              v
// priority-rank additive shares
//
// priority_key_shares retains the padded_n layout produced by the secure
// raw-score adapter. GRank consumes only its first logical_n positions; padded
// positions do not receive node masks or comparison-edge material.
//
// The runtime validates all public/package bindings, performs one masked
// CmpAgg exchange over the logical graph, and returns additive priority-rank
// shares. It never reconstructs priority keys or ranks.
[[nodiscard]] ProtocolIIIGrankOutput protocol_iii_grank_party(
    const ProtocolIIIGrankConfig& config,
    ProtocolIPartyPackage& package,
    const std::vector<std::uint64_t>& priority_key_shares,
    int grank_fd);

}  // namespace moe_topk
