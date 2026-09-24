#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <moe_topk/protocol_i_opv.h>
#include <moe_topk/protocol_i_permutation.h>
#include <moe_topk/protocol_i_ucmp.h>

namespace moe_topk {

// C-INSTANTIATION: dealer-compiled correlated double-factorization shuffle.
// This is isolated from the CHASE-DIRECT two-pass conformance stack.
// The logical record group is Z_(2^comparison_bits) x Z_(2^64)^2.
struct ProtocolIParallelShuffleDealerConfig {
  std::uint64_t session = 0, fingerprint = 0, material_id = 0;
  std::uint32_t n = 0, k = 0;
  std::uint8_t comparison_bits = 0;
};

struct ProtocolIParallelShufflePartyConfig {
  std::uint64_t session = 0, fingerprint = 0, material_id = 0;
  std::uint32_t n = 0, k = 0;
  std::uint8_t comparison_bits = 0, party = 0;
  int timeout_ms = 0;
};

struct ProtocolIParallelShufflePartyMaterial {
  std::uint64_t session = 0, fingerprint = 0, material_id = 0;
  std::uint32_t n = 0, k = 0;
  std::uint8_t comparison_bits = 0, party = 0;
  ProtocolIPermutation sigma, tau;
  std::vector<ProtocolIBlock192> a, e, r_share;
  std::vector<ProtocolIUcmpPartyMaterial> edge_materials;

  ProtocolIParallelShufflePartyMaterial() = default;
  ProtocolIParallelShufflePartyMaterial(const ProtocolIParallelShufflePartyMaterial&) = delete;
  ProtocolIParallelShufflePartyMaterial& operator=(const ProtocolIParallelShufflePartyMaterial&) = delete;
  ProtocolIParallelShufflePartyMaterial(ProtocolIParallelShufflePartyMaterial&&) noexcept = default;
  ProtocolIParallelShufflePartyMaterial& operator=(ProtocolIParallelShufflePartyMaterial&&) noexcept = default;
};

struct ProtocolIParallelShuffleDealerMetrics {
  std::uint64_t record_material_logical_bits = 0;
  std::uint64_t permutation_material_logical_bits = 0;
  std::uint64_t cmpagg_party_material_wire_bytes = 0;
};

struct ProtocolIParallelShuffleDealerOutput {
  ProtocolIParallelShufflePartyMaterial party0, party1;
  ProtocolIParallelShuffleDealerMetrics metrics;
};

ProtocolIParallelShuffleDealerOutput protocol_i_parallel_shuffle_dealer_generate(
    const ProtocolIParallelShuffleDealerConfig& config);

std::vector<std::uint8_t> protocol_i_parallel_shuffle_serialize_material(
    const ProtocolIParallelShufflePartyMaterial& material);
ProtocolIParallelShufflePartyMaterial protocol_i_parallel_shuffle_deserialize_material(
    const std::vector<std::uint8_t>& bytes, int expected_party);

ProtocolIBlock192 protocol_i_parallel_record_add(int comparison_bits,
                                                  ProtocolIBlock192 left,
                                                  const ProtocolIBlock192& right);
ProtocolIBlock192 protocol_i_parallel_record_sub(int comparison_bits,
                                                  ProtocolIBlock192 left,
                                                  const ProtocolIBlock192& right);
std::uint8_t protocol_i_parallel_rank_bits(std::uint32_t n);

struct ProtocolIParallelShuffleRound2Output {
  std::vector<ProtocolIBlock192> shuffled_share;
  std::vector<ProtocolIBlock192> public_masked_records;
};

struct ProtocolIParallelShuffleCoreOutput {
  std::vector<ProtocolIBlock192> shuffled_share;
  std::vector<ProtocolIBlock192> public_masked_records;
  std::vector<std::uint64_t> public_ranks;
  std::vector<ProtocolIBlock192> sorted_share;
};

// The state machine freezes each outbound before accepting the peer's message
// for that round.  Its process-local claim ledger rejects copied/reloaded
// material IDs; durable cross-restart claims remain the material-pool owner's
// responsibility.
class ProtocolIParallelShuffleParty final {
 public:
  ProtocolIParallelShuffleParty(const ProtocolIParallelShufflePartyConfig& config,
                                ProtocolIParallelShufflePartyMaterial&& material);
  ProtocolIParallelShuffleParty(const ProtocolIParallelShuffleParty&) = delete;
  ProtocolIParallelShuffleParty& operator=(const ProtocolIParallelShuffleParty&) = delete;

  std::vector<ProtocolIBlock192> prepare_round1(
      const std::vector<ProtocolIBlock192>& input_share);
  std::vector<ProtocolIBlock192> receive_round1_prepare_round2(
      const std::vector<ProtocolIBlock192>& peer_round1);
  ProtocolIParallelShuffleRound2Output receive_round2(
      const std::vector<ProtocolIBlock192>& peer_round2);
  std::vector<std::uint64_t> evaluate_cmpagg_prepare_round3();
  ProtocolIParallelShuffleCoreOutput receive_round3(
      const std::vector<std::uint64_t>& peer_rank_shares);

 private:
  ProtocolIParallelShufflePartyConfig config_{};
  ProtocolIParallelShufflePartyMaterial material_;
  unsigned phase_ = 0;
  std::vector<ProtocolIBlock192> shuffled_share_, round2_outbound_, public_masked_;
  std::vector<std::uint64_t> rank_shares_;
};

struct ProtocolIParallelShuffleMetrics {
  std::uint64_t round1_sent_bytes = 0, round1_received_bytes = 0;
  std::uint64_t round2_sent_bytes = 0, round2_received_bytes = 0;
  std::uint64_t round3_sent_bytes = 0, round3_received_bytes = 0;
  std::uint64_t round1_logical_sent_bits = 0;
  std::uint64_t round2_logical_sent_bits = 0;
  std::uint64_t round3_logical_sent_bits = 0;
  std::uint64_t online_rounds = 3;
};

struct ProtocolIParallelShuffleNetworkOutput {
  ProtocolIParallelShuffleCoreOutput core;
  ProtocolIParallelShuffleMetrics metrics;
};

// Consumes one connected full-duplex fd per causal round.
ProtocolIParallelShuffleNetworkOutput protocol_i_parallel_shuffle_three_round_party(
    const ProtocolIParallelShufflePartyConfig& config,
    ProtocolIParallelShufflePartyMaterial&& material,
    const std::vector<ProtocolIBlock192>& input_share,
    const std::array<int, 3>& round_fds);

}  // namespace moe_topk
