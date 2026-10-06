#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#include <moe_topk/experimental_bmw16_select_adapter.h>
#include <moe_topk/protocol_i_parallel_shuffle.h>

#if !defined(MOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER)
#error "BMW16-derived Select party entry is opt-in and EXPERIMENTAL"
#endif

namespace moe_topk {

enum class ProtocolIBmw16ExpectedFailureKind : std::uint8_t { Material = 1 };

class ProtocolIBmw16ExpectedFailure : public std::runtime_error {
 public:
  ProtocolIBmw16ExpectedFailure(ProtocolIBmw16ExpectedFailureKind kind,
                                const std::string& message)
      : std::runtime_error(message), kind_(kind) {}
  ProtocolIBmw16ExpectedFailureKind kind() const noexcept { return kind_; }

 private:
  ProtocolIBmw16ExpectedFailureKind kind_;
};

enum class ProtocolIBmw16MaterialStage : std::uint8_t {
  RawCarry = 1, RawSign = 2, ForwardShuffle = 3, InverseShuffle = 4,
  ForwardShuffleCmpAgg = 5, InverseShuffleCmpAgg = 6,
  Select = 7, Membership = 8
};

struct ProtocolIBmw16MaterialSlot {
  std::uint64_t id = 0;
  ProtocolIBmw16MaterialStage stage = ProtocolIBmw16MaterialStage::RawCarry;
  std::uint8_t task = 0xff, round = 0;
  std::uint32_t left = 0, right = 0;
};

// TEST_ONLY common tape contains raw 64-bit words for [task][R1/R3]. Both
// implementations use the same multiply-high bounded map and remaining-pool
// sampling order; production sampling uses OS-CSPRNG rejection mapping.
struct ProtocolIBmw16TestOnlyRandomTape {
  std::array<std::array<std::vector<std::uint64_t>, 2>, 2> words;
  std::vector<std::string>* trace_lines = nullptr;
};

// EXPERIMENTAL / PROJECT_DERIVED. Every material object is supplied by an
// offline caller. This entry intentionally opens the Select comparison bits
// and transcript; it is not a secure API and has no BMW16 Theorem 8 guarantee.
struct ProtocolIBmw16ExperimentalPartyMaterial {
  ProtocolIPartyPackage score_input;
  ProtocolIParallelShufflePartyMaterial forward_shuffle;
  ProtocolIParallelShufflePartyMaterial inverse_shuffle;
  // Independent one-shot uCMP keys: [task][round-1][canonical real pair].
  std::array<std::array<std::vector<ProtocolIUcmpPartyMaterial>, 4>, 2> select_keys;
  // Independent one-shot keys indexed by the canonical real pair.
  std::vector<ProtocolIUcmpPartyMaterial> membership_keys;
  std::vector<ProtocolIBmw16MaterialSlot> slot_manifest;
  std::function<void(std::uint64_t)> durable_slot_claim;
  std::function<void(std::uint8_t, std::uint32_t)> durable_raw_slot_claim;

  ProtocolIBmw16ExperimentalPartyMaterial() = default;
  ProtocolIBmw16ExperimentalPartyMaterial(const ProtocolIBmw16ExperimentalPartyMaterial&) = delete;
  ProtocolIBmw16ExperimentalPartyMaterial& operator=(const ProtocolIBmw16ExperimentalPartyMaterial&) = delete;
  ProtocolIBmw16ExperimentalPartyMaterial(ProtocolIBmw16ExperimentalPartyMaterial&&) noexcept = default;
  ProtocolIBmw16ExperimentalPartyMaterial& operator=(ProtocolIBmw16ExperimentalPartyMaterial&&) noexcept = default;
};

struct ProtocolIBmw16ExperimentalPartyConfig {
  std::uint64_t session = 0, fingerprint = 0;
  std::uint32_t n = 0, k = 0, padded_n = 0;
  std::uint8_t index_bits = 0, comparison_bits = 0, party = 0;
  int timeout_ms = 0;
  // Metadata only. Online party randomness comes from an OS-CSPRNG coin toss.
  // Deterministic choices are accepted only through the explicit TEST_ONLY tape.
  std::uint64_t test_only_algorithm_seed = 0;
  // This hook exists only in the opt-in experimental/test interface.
  bool test_only_force_probability_abort_after_select = false;
};

struct ProtocolIBmw16ExperimentalMetrics {
  std::array<std::uint64_t, 4> logical_comparison_calls{};
  std::array<std::uint64_t, 4> unique_select_slots_consumed{};
  std::array<std::uint64_t, 4> dummy_related_calls{};
  std::array<std::uint64_t, 4> repeated_logical_calls{};
  std::array<std::uint64_t, 4> edge_plan_fnv64{};
  std::uint64_t membership_slots_consumed = 0;
  std::uint64_t ucmp_party_evaluations = 0;
  std::uint64_t dcf_party_evaluations = 0;
  std::uint64_t online_message_phases = 0;
  std::uint64_t online_bytes_sent = 0, online_bytes_received = 0;
  std::uint64_t online_time_us = 0;
  // Inclusive stage timings for the EXPERIMENTAL path. FSS timing excludes
  // serialization/transport; round elapsed includes planning, eval, and exchange.
  std::uint64_t raw_adapter_time_us = 0;
  std::uint64_t raw_adapter_eval_time_us = 0;
  std::uint64_t raw_adapter_exchange_time_us = 0;
  std::uint64_t forward_shuffle_time_us = 0;
  std::uint64_t forward_shuffle_exchange_time_us = 0;
  std::uint64_t sampling_coin_exchange_time_us = 0;
  std::array<std::uint64_t, 4> select_round_time_us{};
  std::array<std::uint64_t, 4> select_eval_time_us{};
  std::array<std::uint64_t, 4> select_exchange_time_us{};
  std::uint64_t membership_time_us = 0;
  std::uint64_t membership_eval_time_us = 0;
  std::uint64_t inverse_shuffle_time_us = 0;
  std::uint64_t inverse_shuffle_exchange_time_us = 0;
  std::uint64_t status_coordination_time_us = 0;
  std::uint64_t durable_claim_time_us = 0;
  std::uint64_t slot_lookup_time_us = 0;
  std::uint64_t durable_claim_io_time_us = 0;
  std::uint64_t sampler_prf_words = 0;
  std::uint64_t durable_slots_claimed = 0;
  std::uint64_t forward_shuffle_cmpagg_slots_unused = 0;
  std::uint64_t inverse_shuffle_cmpagg_slots_unused = 0;
};

struct ProtocolIBmw16ExperimentalPartyOutput {
  // SUCCESS, ABORT_ALGORITHM_PROBABILITY, ABORT_ALGORITHM_INVALID,
  // ABORT_MATERIAL, or ABORT_COMMUNICATION.
  const char* status = "ABORT_MATERIAL";
  // NONE, PEER_AGREED (algorithm status exchanged), or LOCAL_ONLY (no
  // acknowledgement; caller must not claim a bilateral abort).
  const char* abort_scope = "LOCAL_ONLY";
  std::string abort_reason;
  std::vector<std::uint8_t> xor_mask_share;
  ProtocolIBmw16ExperimentalMetrics metrics;
};

struct ProtocolIBmw16ExperimentalMaterialPair {
  ProtocolIBmw16ExperimentalPartyMaterial party0, party1;
  std::array<std::uint64_t, 2> offline_material_slots_per_party{};
  // Exact serialization sizes of primitive key encodings plus both shuffle
  // material encodings; excludes the not-yet-defined outer package envelope.
  std::array<std::uint64_t, 2> primitive_key_and_shuffle_bytes_per_party{};
};

std::vector<ProtocolIBmw16MaterialSlot> protocol_i_bmw16_enumerate_material_slots(
    const ProtocolIBmw16ExperimentalPartyConfig& config);

// Trusted-offline experimental generator. It consumes public parameters only.
ProtocolIBmw16ExperimentalMaterialPair protocol_i_bmw16_experimental_material_generate(
    const ProtocolIBmw16ExperimentalPartyConfig& public_config);

// Full VFSS C++ party-side functional composition. fd arrays are consumed by
// ProtocolIFramedChannel; pass owned descriptors (dup() when reusing a socket).
// comparison bits and the anonymous adaptive transcript are opened to both
// parties by this experimental design. The selected handle is not returned.
ProtocolIBmw16ExperimentalPartyOutput protocol_i_bmw16_experimental_raw_score_mask_party(
    const ProtocolIBmw16ExperimentalPartyConfig& config,
    ProtocolIBmw16ExperimentalPartyMaterial&& material,
    const std::vector<std::uint32_t>& raw_score_share,
    const std::array<int, 2>& score_fds,
    const std::array<int, 2>& forward_shuffle_fds,
    const std::array<int, 4>& select_round_fds,
    const std::array<int, 2>& inverse_shuffle_fds,
    int final_agreement_fd,
    int sampling_coin_fd = -1,
    const ProtocolIBmw16TestOnlyRandomTape* test_only_tape = nullptr);

}  // namespace moe_topk
