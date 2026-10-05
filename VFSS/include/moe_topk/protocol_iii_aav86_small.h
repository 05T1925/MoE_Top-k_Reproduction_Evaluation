#pragma once

#include <moe_topk/protocol_i_aav86_small.h>
#include <moe_topk/protocol_iii_dpf_routing.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace moe_topk {

// M6A project composition candidate. It consumes the existing signed raw
// score adapter, but runs an independent D-handle AAV86/DPF path. It does not
// expose Protocol I's final shuffled order or public carrier.
struct ProtocolIIIAav86SmallConfig {
  std::uint64_t session = 0, fingerprint = 0, material_id = 0;
  std::uint32_t logical_n = 0, k = 0, iterations = 0;
  std::uint8_t party = 0;
  int timeout_ms = 0;
  std::string durable_claim_directory;
};

struct ProtocolIIIAav86SmallPartyMaterial {
  std::uint64_t material_id = 0;
  ProtocolIAav86SmallPartyMaterial layout_and_ca;
  ProtocolIIIDpfRoutingPartyMaterial handle_dpf;
  bool started = false;

  ProtocolIIIAav86SmallPartyMaterial() = default;
  ProtocolIIIAav86SmallPartyMaterial(
      const ProtocolIIIAav86SmallPartyMaterial&) = delete;
  ProtocolIIIAav86SmallPartyMaterial& operator=(
      const ProtocolIIIAav86SmallPartyMaterial&) = delete;
  ProtocolIIIAav86SmallPartyMaterial(
      ProtocolIIIAav86SmallPartyMaterial&&) noexcept = default;
  ProtocolIIIAav86SmallPartyMaterial& operator=(
      ProtocolIIIAav86SmallPartyMaterial&&) noexcept = default;
};

struct ProtocolIIIAav86SmallDealerOutput {
  ProtocolIIIAav86SmallPartyMaterial party0, party1;
};

struct ProtocolIIIAav86SmallMessageTrace {
  std::uint8_t phase = 0;
  std::uint64_t sent_bytes = 0, received_bytes = 0;
};

struct ProtocolIIIAav86SmallMetrics {
  std::uint64_t reserved_edge_slots_per_party = 0;
  std::uint64_t active_edges = 0, active_vertices = 0;
  std::uint64_t dcf_evaluations = 0, dpf_evaluations = 0;
  std::uint64_t dcf_online_prg_calls = 0;
  std::uint64_t score_sent_bytes = 0, score_received_bytes = 0;
  std::uint64_t forward_sent_bytes = 0, forward_received_bytes = 0;
  std::uint64_t ca_sent_bytes = 0, ca_received_bytes = 0;
  std::uint64_t route_sent_bytes = 0, route_received_bytes = 0;
  std::uint64_t sent_bytes = 0, received_bytes = 0;
  std::uint64_t causal_rounds = 0;
  std::vector<std::uint64_t> active_edges_by_round;
  std::vector<std::uint64_t> active_vertices_by_round;
  std::vector<std::uint64_t> dcf_prg_calls_by_round;
  std::vector<ProtocolIIIAav86SmallMessageTrace> message_trace;
};

struct ProtocolIIIAav86SmallOutput {
  std::vector<std::uint8_t> xor_mask_share;
  ProtocolIIIAav86SmallMetrics metrics;
};

struct ProtocolIIIAav86SmallFds {
  std::array<int, 2> score{{-1, -1}};
  int forward = -1;
  std::vector<int> masked_lists;
  std::vector<int> early_ranks;
  int final_masked_rank = -1;
  int inverse = -1;
};

std::uint32_t protocol_iii_aav86_small_domain(std::uint32_t logical_n);
std::uint8_t protocol_iii_aav86_small_rank_bits(std::uint32_t domain);

// Trusted offline party material generation. The caller supplies public
// shape only; this function has no online input channels.
ProtocolIIIAav86SmallDealerOutput protocol_iii_aav86_small_dealer_generate(
    const ProtocolIIIAav86SmallConfig& config);

std::vector<std::uint8_t> protocol_iii_aav86_small_serialize_material(
    const ProtocolIIIAav86SmallPartyMaterial& material);
ProtocolIIIAav86SmallPartyMaterial protocol_iii_aav86_small_deserialize_material(
    const std::vector<std::uint8_t>& bytes, int expected_party,
    const ProtocolIIIAav86SmallConfig& expected_config);

ProtocolIIIAav86SmallOutput protocol_iii_aav86_small_party(
    const ProtocolIIIAav86SmallConfig& config,
    ProtocolIIIAav86SmallPartyMaterial&& material,
    const std::vector<std::uint32_t>& raw_score_share,
    const ProtocolIIIAav86SmallFds& fds);

}  // namespace moe_topk
