#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <moe_topk/protocol_i_dealer_candidate_package.h>
#include <moe_topk/protocol_i_secret_shared_shuffle.h>

namespace moe_topk {

struct ProtocolIDealerCandidateCoreConfig : ProtocolIDealerCandidateConfig {
  int timeout_ms = 0;
};

struct ProtocolIDealerCandidateTraceEvent {
  std::uint32_t barrier = 0, phase = 0;
  std::uint8_t type = 0, sender = 0, receiver = 0, completed = 0;
  std::uint64_t sequence = 0, sent_bytes = 0, received_bytes = 0;
};

struct ProtocolIDealerCandidateMetrics {
  std::uint64_t offline_material_bytes = 0;
  std::uint64_t offline_shuffle_sent_bytes = 0, offline_shuffle_received_bytes = 0;
  std::uint64_t r1_sent_bytes = 0, r1_received_bytes = 0;
  std::uint64_t r2_sent_bytes = 0, r2_received_bytes = 0;
  std::uint64_t r3_sent_bytes = 0, r3_received_bytes = 0;
  std::uint64_t party_to_party_bytes = 0;
  std::uint64_t comparison_edges = 0, raw_dcf_calls = 0;
  std::uint64_t forward_online_rounds = 0, masked_open_rounds = 0, online_rounds = 0;
  std::uint64_t rank_bits = 0;
};

struct ProtocolIDealerCandidateOutput {
  std::vector<std::uint64_t> public_masked_list;
  std::vector<std::uint64_t> shuffled_rank_share;
  ProtocolIDealerCandidateMetrics metrics;
  std::vector<ProtocolIDealerCandidateTraceEvent> trace;
};

ProtocolIDealerCandidateOutput protocol_i_dealer_candidate_core_party(
    const ProtocolIDealerCandidateCoreConfig& config,
    ProtocolIDealerCandidatePackage&& package,
    ProtocolIShufflePartyMaterial& shuffle_material,
    const std::vector<std::uint64_t>& priority_key_share,
    const std::array<int, 2>& forward_fds,
    int masked_open_fd);

std::vector<std::uint8_t> serialize_dealer_candidate_result(
    const ProtocolIDealerCandidateCoreConfig& config,
    const ProtocolIDealerCandidateOutput& output);
ProtocolIDealerCandidateOutput deserialize_dealer_candidate_result(
    const std::vector<std::uint8_t>& bytes,
    const ProtocolIDealerCandidateCoreConfig& expected_config,
    int expected_party);

}  // namespace moe_topk
