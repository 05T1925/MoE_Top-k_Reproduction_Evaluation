#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <moe_topk/project_derived_select5.h>
#include <moe_topk/protocol_i_parallel_shuffle.h>
#include <moe_topk/protocol_i_party_package.h>
#include <moe_topk/protocol_i_score_input.h>
#include <moe_topk/protocol_i_score_share_abi.h>
#include <moe_topk/protocol_i_ucmp.h>

namespace moe_topk {

class ProtocolIStreamedMaterialReader;

inline constexpr const char* kProtocolIProjectDerivedSelect5Label =
    "Protocol_I_PROJECT_DERIVED_SELECT_5ITER_DCF_EXPERIMENTAL";
inline constexpr std::uint32_t kProtocolIProjectDerivedSelect5MaxN = 256;
inline constexpr std::size_t kProtocolIProjectDerivedSelect5MaxPackageBytes =
    64U * 1024U * 1024U;

struct ProtocolIProjectDerivedSelect5DealerConfig {
  std::uint64_t session = 0;
  std::uint64_t fingerprint = 0;
  std::uint64_t material_id_base = 0;
  std::uint32_t logical_n = 0;
  std::uint32_t k = 0;
  std::uint32_t timeout_ms = 0;
  ProtocolIDcfAbi dcf_abi = ProtocolIDcfAbi::LegacyM2UcV1;
  ProtocolIScoreShareAbi score_share_abi =
      ProtocolIScoreShareAbi::S15InducedV1;
};

struct ProtocolIProjectDerivedSelect5DealerOutput {
  std::vector<std::uint8_t> party0_package;
  std::vector<std::uint8_t> party1_package;
};

struct ProtocolIProjectDerivedSelect5CompareMaterial {
  std::uint32_t iteration = 0;
  std::uint32_t purpose = 0;
  std::uint32_t substage_scope = 0;
  std::uint32_t direction = 0;
  std::uint32_t left = 0;
  std::uint32_t right = 0;
  std::uint64_t left_mask_share = 0;
  std::uint64_t right_mask_share = 0;
  ProtocolIUcmpPartyMaterial material;
  ProtocolIProjectDerivedSelect5CompareMaterial() = default;
  ProtocolIProjectDerivedSelect5CompareMaterial(
      std::uint32_t iter, std::uint32_t use_purpose, std::uint32_t scope,
      std::uint32_t compare_direction, std::uint32_t a, std::uint32_t b,
      std::uint64_t left_share, std::uint64_t right_share,
      ProtocolIUcmpPartyMaterial&& key)
      : iteration(iter), purpose(use_purpose), substage_scope(scope),
        direction(compare_direction), left(a), right(b),
        left_mask_share(left_share), right_mask_share(right_share),
        material(std::move(key)) {}
  ProtocolIProjectDerivedSelect5CompareMaterial(
      const ProtocolIProjectDerivedSelect5CompareMaterial&) = delete;
  ProtocolIProjectDerivedSelect5CompareMaterial& operator=(
      const ProtocolIProjectDerivedSelect5CompareMaterial&) = delete;
  ProtocolIProjectDerivedSelect5CompareMaterial(
      ProtocolIProjectDerivedSelect5CompareMaterial&&) noexcept = default;
  ProtocolIProjectDerivedSelect5CompareMaterial& operator=(
      ProtocolIProjectDerivedSelect5CompareMaterial&&) noexcept = default;
};

struct ProtocolIProjectDerivedSelect5MembershipMaterial {
  std::uint32_t iteration = 5;
  std::uint32_t purpose = 2;
  std::uint32_t substage = 4;
  std::uint32_t direction = 2;  // strict_lt(tau, key_i), then complement
  std::uint32_t item = 0;
  std::uint64_t tau_mask_share = 0;
  std::uint64_t item_mask_share = 0;
  ProtocolIUcmpPartyMaterial material;
  ProtocolIProjectDerivedSelect5MembershipMaterial() = default;
  ProtocolIProjectDerivedSelect5MembershipMaterial(
      std::uint32_t index, std::uint64_t tau_share, std::uint64_t key_share,
      ProtocolIUcmpPartyMaterial&& key)
      : item(index), tau_mask_share(tau_share), item_mask_share(key_share),
        material(std::move(key)) {}
  ProtocolIProjectDerivedSelect5MembershipMaterial(
      const ProtocolIProjectDerivedSelect5MembershipMaterial&) = delete;
  ProtocolIProjectDerivedSelect5MembershipMaterial& operator=(
      const ProtocolIProjectDerivedSelect5MembershipMaterial&) = delete;
  ProtocolIProjectDerivedSelect5MembershipMaterial(
      ProtocolIProjectDerivedSelect5MembershipMaterial&&) noexcept = default;
  ProtocolIProjectDerivedSelect5MembershipMaterial& operator=(
      ProtocolIProjectDerivedSelect5MembershipMaterial&&) noexcept = default;
};

struct ProtocolIProjectDerivedSelect5PartyMaterial {
  std::uint64_t session = 0;
  std::uint64_t fingerprint = 0;
  std::uint64_t material_id_base = 0;
  std::uint32_t logical_n = 0;
  std::uint32_t padded_n = 0;
  std::uint32_t k = 0;
  std::uint8_t comparison_bits = 0;
  std::uint8_t party = 0;
  ProtocolIDcfAbi dcf_abi = ProtocolIDcfAbi::LegacyM2UcV1;
  ProtocolIScoreShareAbi score_share_abi =
      ProtocolIScoreShareAbi::S15InducedV1;
  std::vector<std::uint64_t> score_key_refresh_shares;
  ProtocolIPartyPackage score_input;
  ProtocolIParallelShufflePartyMaterial forward_shuffle;
  ProtocolIParallelShufflePartyMaterial inverse_shuffle;
  std::vector<ProtocolIProjectDerivedSelect5CompareMaterial> select_slots;
  std::vector<ProtocolIProjectDerivedSelect5MembershipMaterial> membership_slots;
  std::shared_ptr<ProtocolIStreamedMaterialReader> streamed_material_reader;

  ProtocolIProjectDerivedSelect5PartyMaterial() = default;
  ProtocolIProjectDerivedSelect5PartyMaterial(
      const ProtocolIProjectDerivedSelect5PartyMaterial&) = delete;
  ProtocolIProjectDerivedSelect5PartyMaterial& operator=(
      const ProtocolIProjectDerivedSelect5PartyMaterial&) = delete;
  ProtocolIProjectDerivedSelect5PartyMaterial(
      ProtocolIProjectDerivedSelect5PartyMaterial&&) noexcept = default;
  ProtocolIProjectDerivedSelect5PartyMaterial& operator=(
      ProtocolIProjectDerivedSelect5PartyMaterial&&) noexcept = default;
};

ProtocolIProjectDerivedSelect5DealerOutput
protocol_i_project_derived_select5_dealer_generate(
    const ProtocolIProjectDerivedSelect5DealerConfig& config);

// Offline durable package store. The trusted T stages both packages; a
// supervisor may publish readiness only after observing T exit successfully.
// Each party claim is fsync-backed and O_EXCL: a crash after claim burns that
// package ID permanently, while a crash before claim leaves it claimable.
// M6B5/v2 slot-store generator. It writes adapter masks, Select5 DCF keys,
// membership keys, and only the O(N) two-round shuffle vectors one slot at a
// time. It deliberately omits M2CP's unused full-comparison DCF pool.
// S12 party-scoped storage: each recipient's package/manifest/leases are in
// a distinct OS-owned directory. The supervisor/T must arrange distinct
// party UIDs and transfer directory ownership before launching P0/P1.
void protocol_i_project_derived_select5_dealer_stage_streamed_material(
    const std::array<std::string, 2>& party_store_directories,
    const ProtocolIProjectDerivedSelect5DealerConfig& config);
void protocol_i_project_derived_select5_publish_streamed_material_ready(
    const std::array<std::string, 2>& party_store_directories,
    const ProtocolIProjectDerivedSelect5DealerConfig& config);
void protocol_i_project_derived_select5_stage_material_packages(
    const std::array<std::string, 2>& party_store_directories,
    const ProtocolIProjectDerivedSelect5DealerConfig& config,
    const ProtocolIProjectDerivedSelect5DealerOutput& output);
void protocol_i_project_derived_select5_publish_material_ready(
    const std::array<std::string, 2>& party_store_directories,
    const ProtocolIProjectDerivedSelect5DealerConfig& config);
std::vector<std::uint8_t>
protocol_i_project_derived_select5_claim_party_package(
    const std::string& store_directory,
    std::uint64_t session, std::uint64_t fingerprint,
    std::uint64_t material_id_base, std::uint32_t logical_n,
    std::uint32_t padded_n, std::uint32_t k,
    std::uint8_t comparison_bits, std::uint8_t party);

std::vector<std::uint8_t> protocol_i_project_derived_select5_serialize_party_material(
    const ProtocolIProjectDerivedSelect5PartyMaterial& material);
ProtocolIProjectDerivedSelect5PartyMaterial
protocol_i_project_derived_select5_deserialize_party_material(
    const std::vector<std::uint8_t>& package, int expected_party);

struct ProtocolIProjectDerivedSelect5RuntimeConfig {
  std::uint64_t session = 0;
  std::uint64_t fingerprint = 0;
  std::uint32_t logical_n = 0;
  std::uint32_t padded_n = 0;
  std::uint32_t k = 0;
  std::uint8_t comparison_bits = 0;
  std::uint8_t party = 0;
  ProtocolIDcfAbi dcf_abi = ProtocolIDcfAbi::LegacyM2UcV1;
  int timeout_ms = 0;
  std::uint64_t material_id_base = 0;
  std::string material_store_directory;
  bool streamed_material_store = false;
  std::string expected_peer_identity;
  // Plain FD mode is available only for explicit TEST_ONLY harnesses.
  bool allow_plaintext_test_transport = false;
  // TEST_ONLY fault injection: _exit after a durable output/run transition or
  // final-status exchange (1..6). Production callers must leave this zero.
  std::uint8_t test_crash_after_output_step = 0;
  // TEST_ONLY wire-event trace path; records phase/order/length only, never
  // frame payloads or seed bytes.
  std::string test_wire_trace_path;
  ProtocolIScoreShareAbi score_share_abi =
      ProtocolIScoreShareAbi::S15InducedV1;
};

struct ProtocolIProjectDerivedSelect5Fds {
  ProtocolIFramedEndpoint package_binding;
  std::array<ProtocolIFramedEndpoint, 2> score_input;
  std::array<ProtocolIFramedEndpoint, 2> forward_shuffle;
  std::array<ProtocolIFramedEndpoint, 18> select_layers;
  // S8 contract v3: P0 discloses one fresh 32-byte public sampling seed at
  // the start of each active reduction iteration, after forward shuffle.
  std::array<ProtocolIFramedEndpoint, 4> sample_schedule;
  ProtocolIFramedEndpoint membership;
  std::array<ProtocolIFramedEndpoint, 2> inverse_shuffle;
  ProtocolIFramedEndpoint final_status;
};

struct ProtocolIProjectDerivedSelect5Metrics {
  std::uint64_t binding_sent_bytes = 0;
  std::uint64_t binding_received_bytes = 0;
  std::uint64_t input_sent_bytes = 0;
  std::uint64_t input_received_bytes = 0;
  std::uint64_t forward_sent_bytes = 0;
  std::uint64_t forward_received_bytes = 0;
  std::uint64_t sample_schedule_sent_bytes = 0;
  std::uint64_t sample_schedule_received_bytes = 0;
  std::uint64_t select_sent_bytes = 0;
  std::uint64_t select_received_bytes = 0;
  std::uint64_t membership_sent_bytes = 0;
  std::uint64_t membership_received_bytes = 0;
  std::uint64_t inverse_sent_bytes = 0;
  std::uint64_t inverse_received_bytes = 0;
  std::uint64_t final_status_sent_bytes = 0;
  std::uint64_t final_status_received_bytes = 0;
  std::uint64_t comparison_calls = 0;
  std::uint64_t select_dcf_eval_calls = 0;
  std::uint64_t membership_dcf_eval_calls = 0;
  std::uint32_t active_compare_layers = 0;
  std::uint32_t active_sample_rounds = 0;
  std::uint32_t online_rounds = 0;
};

struct ProtocolIProjectDerivedSelect5PartyOutput {
  std::vector<std::uint8_t> xor_mask_share;
  ProtocolIProjectDerivedSelect5Metrics metrics;
};

ProtocolIProjectDerivedSelect5PartyOutput
protocol_i_project_derived_select5_party(
    const ProtocolIProjectDerivedSelect5RuntimeConfig& config,
    const std::vector<std::uint32_t>& raw_score_share,
    const ProtocolIProjectDerivedSelect5Fds& fds);

// Explicit test harness injection. Runtime callers use the durable-store
// overload above; direct material injection is available only in TEST_ONLY.
ProtocolIProjectDerivedSelect5PartyOutput
protocol_i_project_derived_select5_party_test_material(
    const ProtocolIProjectDerivedSelect5RuntimeConfig& config,
    ProtocolIProjectDerivedSelect5PartyMaterial&& material,
    const std::vector<std::uint32_t>& raw_score_share,
    const ProtocolIProjectDerivedSelect5Fds& fds,
    bool corrupt_first_sample_seed_for_test = false);

// Used by the versioned caller when package import or input preflight fails
// before the party runtime can start. It exchanges only the approved uniform
// ABORT status and never returns a mask.
void protocol_i_project_derived_select5_abort_no_mask(
    const ProtocolIProjectDerivedSelect5RuntimeConfig& config,
    const ProtocolIProjectDerivedSelect5Fds& fds);

}  // namespace moe_topk
