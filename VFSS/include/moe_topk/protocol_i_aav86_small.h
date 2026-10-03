#pragma once

#include <moe_topk/protocol_i_party_package.h>
#include <moe_topk/protocol_i_permutation.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace moe_topk {

// M6A project instance, not an author-exact Protocol I implementation.
// E9 admits only D <= 64 after pre-keygen package and dealer-memory checks.
struct ProtocolIAav86SmallConfig {
  std::uint64_t session = 0, fingerprint = 0, material_id = 0;
  std::uint32_t logical_n = 0, k = 0, iterations = 0;
  std::uint8_t party = 0;
  int timeout_ms = 0;
  std::string durable_claim_directory;
};

struct ProtocolIAav86SmallPartyMaterial {
  std::uint64_t session = 0, fingerprint = 0, material_id = 0;
  std::uint64_t pivot_seed_lo = 0, pivot_seed_hi = 0;
  std::uint32_t logical_n = 0, padded_n = 0, k = 0, iterations = 0;
  std::uint8_t comparison_bits = 0, party = 0;
  ProtocolIPermutation forward_sigma, forward_tau, inverse_sigma, inverse_tau;
  std::vector<std::uint64_t> forward_a, forward_e, inverse_a, inverse_e;
  std::vector<std::uint64_t> node_mask_shares;
  std::vector<ProtocolIUcmpPartyMaterial> edge_keys;
  ProtocolIPartyPackage score_materials;

  ProtocolIAav86SmallPartyMaterial() = default;
  ProtocolIAav86SmallPartyMaterial(const ProtocolIAav86SmallPartyMaterial&) = delete;
  ProtocolIAav86SmallPartyMaterial& operator=(const ProtocolIAav86SmallPartyMaterial&) = delete;
  ProtocolIAav86SmallPartyMaterial(ProtocolIAav86SmallPartyMaterial&&) = default;
  ProtocolIAav86SmallPartyMaterial& operator=(ProtocolIAav86SmallPartyMaterial&&) = default;
};

struct ProtocolIAav86SmallDealerOutput {
  ProtocolIAav86SmallPartyMaterial party0, party1;
};

struct ProtocolIAav86SmallCapacity {
  std::uint32_t padded_n = 0;
  std::uint8_t comparison_bits = 0;
  std::uint64_t pairs_per_iteration = 0, total_pair_slots = 0;
  std::uint64_t party_package_bytes = 0, dealer_memory_budget_bytes = 0;
  std::uint64_t available_memory_bytes = 0;
};

// Pure shape arithmetic plus current memory-availability check. Dealer calls
// this before reserve, keygen, or any size-dependent allocation.
ProtocolIAav86SmallCapacity protocol_i_aav86_small_preflight(
    const ProtocolIAav86SmallConfig& config);

struct ProtocolIAav86SmallMessageTrace {
  std::uint8_t phase = 0;
  std::uint64_t sent_bytes = 0, received_bytes = 0;
};

struct ProtocolIAav86SmallEdgeTrace {
  std::uint32_t iteration = 0, a = 0, c = 0;
  std::uint64_t material_id = 0;
};

struct ProtocolIAav86SmallMetrics {
  std::uint64_t pool_slots_per_party = 0;
  std::uint64_t active_edges = 0;
  std::uint64_t score_dcf_evaluations = 0, ca_dcf_evaluations = 0;
  std::uint64_t dcf_evaluations = 0;
  std::uint64_t online_sent_bytes = 0, online_received_bytes = 0;
  std::uint64_t score_sent_bytes = 0, core_sent_bytes = 0, inverse_sent_bytes = 0;
  std::uint64_t causal_rounds = 0;
  std::vector<std::uint64_t> active_edges_by_iteration;
  std::vector<ProtocolIAav86SmallMessageTrace> message_trace;
  std::vector<ProtocolIAav86SmallEdgeTrace> edge_trace;
};

struct ProtocolIAav86SmallOutput {
  std::vector<std::uint8_t> xor_mask_share;
  ProtocolIAav86SmallMetrics metrics;
};

// The dealer sees only public shape and offline randomness. It must be run
// before any online score input exists and never receives online channels.
ProtocolIAav86SmallDealerOutput protocol_i_aav86_small_dealer_generate(
    const ProtocolIAav86SmallConfig& config);

std::vector<std::uint8_t> protocol_i_aav86_small_serialize_party_material(
    const ProtocolIAav86SmallPartyMaterial& material);
// Deserialization checks structural labels, lengths, party and key format.
// It cannot verify that a same-spec key blob was generated for its declared
// mask/edge. That relationship relies on trusted T and an intact T->party
// delivery channel; TEST_ONLY dealer consistency checks cover generated pairs.
ProtocolIAav86SmallPartyMaterial protocol_i_aav86_small_deserialize_party_material(
    const std::vector<std::uint8_t>& bytes, int expected_party,
    const ProtocolIAav86SmallConfig& expected_config);

// All fds are distinct, connected P0<->P1 full-duplex channels and consumed
// by this call. core_fds has 2*r+1 entries: initial share shuffle, then
// (masked open, rank reveal) for each iteration.
ProtocolIAav86SmallOutput protocol_i_aav86_small_party(
    const ProtocolIAav86SmallConfig& config,
    ProtocolIAav86SmallPartyMaterial&& material,
    const std::vector<std::uint32_t>& raw_score_share,
    const std::array<int, 2>& score_fds,
    const std::vector<int>& core_fds, int inverse_fd);

}  // namespace moe_topk
