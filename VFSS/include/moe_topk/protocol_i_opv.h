#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include <moe_topk/protocol_i_chosen_ot.h>

namespace moe_topk {

// C: EMP-backed M2.9 OPV conformance interface; this is not a shuffle API.
using ProtocolISeed128 = std::array<std::uint8_t, 16>;

// C-INSTANTIATION storage for the current Chase path.  Its logical additive
// group is exactly (Z_(2^64))^3: each lane wraps independently, serialization
// preserves the three lanes, and permutation only changes record positions.
// This does not claim support for an arbitrary ell-prime group.
struct ProtocolIBlock192 {
  std::uint64_t word0 = 0;
  std::uint64_t word1 = 0;
  std::uint64_t word2 = 0;
  bool operator==(const ProtocolIBlock192& other) const {
    return word0 == other.word0 && word1 == other.word1 && word2 == other.word2;
  }
};

inline constexpr ProtocolIBlock192 protocol_i_record_add(
    ProtocolIBlock192 left, const ProtocolIBlock192& right) {
  return {left.word0 + right.word0, left.word1 + right.word1,
          left.word2 + right.word2};
}

inline constexpr ProtocolIBlock192 protocol_i_record_sub(
    ProtocolIBlock192 left, const ProtocolIBlock192& right) {
  return {left.word0 - right.word0, left.word1 - right.word1,
          left.word2 - right.word2};
}

struct ProtocolIOpvConfig {
  std::uint64_t session;
  std::uint64_t fingerprint;
  std::uint64_t material_id;
  std::uint32_t vector_length;  // T: power of two, zero-based/MSB-first tree.
  std::uint32_t batch_count;
  int timeout_ms;
};

struct ProtocolIOpvFullVectorResult {
  std::vector<std::vector<ProtocolIBlock192>> leaves;  // batch_count x T
  ProtocolIChosenOtCounters counters;
  std::uint64_t chosen_ot_items = 0;
};

struct ProtocolIOpvPuncturedResult {
  std::vector<std::uint32_t> punctured_indices;
  std::vector<std::vector<std::optional<ProtocolIBlock192>>> leaves;
  ProtocolIChosenOtCounters counters;
  std::uint64_t chosen_ot_items = 0;
};

// FVO is the chosen-OT sender and never receives a puncture index.
ProtocolIOpvFullVectorResult protocol_i_opv_full_vector_owner(
    const ProtocolIOpvConfig& config, int connected_fd);

// PO is the chosen-OT receiver and obtains every leaf except its own puncture.
ProtocolIOpvPuncturedResult protocol_i_opv_puncture_owner(
    const ProtocolIOpvConfig& config, int connected_fd,
    const std::vector<std::uint32_t>& punctured_indices);

}  // namespace moe_topk
