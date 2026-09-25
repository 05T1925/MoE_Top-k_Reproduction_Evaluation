#pragma once

#include <moe_topk/protocol_i_party_package.h>
#include <moe_topk/protocol_iii_field_dpf.h>
#include <moe_topk/protocol_iii_field_mul.h>

#include <cryptoTools/Crypto/PRNG.h>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace moe_topk {

// One selected priority rank. Inputs are additive priority-key shares and
// already-field-shared, nonzero encoded records whose embedded keys match
// those priority-key shares. No ring-to-field conversion or consistency proof
// is implied by this boundary.
struct ProtocolIIITwoRoundConfig {
  std::uint64_t session = 0;
  std::uint64_t fingerprint = 0;
  std::uint32_t logical_n = 0;
  std::uint32_t padded_n = 0;
  std::uint32_t k = 0;
  std::uint32_t target_rank = 0;
  std::uint8_t comparison_bits = 0;
  std::uint8_t rank_bits = 0;
  std::uint8_t party = 0;
};

struct ProtocolIIITwoRoundPartyMaterial {
  std::uint32_t target_rank = 0;
  ProtocolIPartyPackage cmpagg;
  std::vector<std::uint64_t> rank_mask_shares;
  std::vector<ProtocolIIIFieldPayloadPartyMaterial> payload_material;
  std::vector<ProtocolIIIFieldMulPartyMaterial> multiplication_material;
};

// Dealer-only, input-independent preprocessing. P0/P1 receive only their
// respective return value, never this pair or the dealer's complete masks.
[[nodiscard]] std::pair<ProtocolIIITwoRoundPartyMaterial,
                        ProtocolIIITwoRoundPartyMaterial>
protocol_iii_two_round_preprocess(const ProtocolIIITwoRoundConfig& config,
                                  osuCrypto::PRNG& dealer_generator);

[[nodiscard]] std::size_t protocol_iii_two_round_offline_material_bytes(
    const ProtocolIIITwoRoundPartyMaterial& material);

class ProtocolIIITwoRoundParty {
 public:
  ProtocolIIITwoRoundParty(
      ProtocolIIITwoRoundConfig config,
      ProtocolIIITwoRoundPartyMaterial&& material,
      std::vector<std::uint64_t> priority_key_shares,
      std::vector<ProtocolIIIField> encoded_payload_shares);

  ProtocolIIITwoRoundParty(const ProtocolIIITwoRoundParty&) = delete;
  ProtocolIIITwoRoundParty& operator=(const ProtocolIIITwoRoundParty&) = delete;

  // Each complete outbound is frozen before the corresponding peer message
  // is read. All messages are canonical, versioned, and metadata-bound.
  [[nodiscard]] std::vector<std::uint8_t> prepare_round1();
  void consume_round1(const std::vector<std::uint8_t>& peer_message);
  [[nodiscard]] std::vector<std::uint8_t> prepare_round2();
  [[nodiscard]] ProtocolIIIField consume_round2(
      const std::vector<std::uint8_t>& peer_message);

  // Fsort extension: the same R1/R2 transcript and preprocessing yield all
  // logical rank slots by local field DPF FullEval after receiving R2.
  // Use k=logical_n and target_rank=0 in the existing bound configuration;
  // the target field is unused by this finalizer. No padded slot is returned.
  [[nodiscard]] std::vector<ProtocolIIIField> consume_round2_sort(
      const std::vector<std::uint8_t>& peer_message);

  [[nodiscard]] const std::vector<std::uint64_t>& rank_additive_shares() const noexcept {
    return rank_shares_;
  }

 private:
  struct OpenedRound2 {
    std::vector<std::uint64_t> masked_ranks;
    std::vector<ProtocolIIIField> masked_payloads;
  };
  [[nodiscard]] OpenedRound2 open_round2(
      const std::vector<std::uint8_t>& peer_message);
  enum class Phase { fresh, round1_prepared, round1_consumed,
                     round2_prepared, finished, failed };
  ProtocolIIITwoRoundConfig config_;
  ProtocolIIITwoRoundPartyMaterial material_;
  std::vector<std::uint64_t> key_shares_;
  std::vector<ProtocolIIIField> payload_shares_;
  std::vector<std::uint64_t> local_masked_keys_;
  std::vector<ProtocolIIIFieldMulOpeningShare> local_openings_;
  std::vector<ProtocolIUcmpPartyMaterial> edge_materials_;
  std::vector<std::uint64_t> rank_shares_;
  std::vector<ProtocolIIIField> masked_payload_shares_;
  std::vector<std::uint64_t> local_masked_ranks_;
  Phase phase_ = Phase::fresh;
};

struct ProtocolIIITwoRoundFds {
  int round1_fd = -1;
  int round2_fd = -1;
};

struct ProtocolIIITwoRoundMetrics {
  std::uint64_t round1_sent_bytes = 0;
  std::uint64_t round1_received_bytes = 0;
  std::uint64_t round2_sent_bytes = 0;
  std::uint64_t round2_received_bytes = 0;
  std::uint64_t round1_logical_bits = 0;
  std::uint64_t round2_logical_bits = 0;
  std::uint8_t online_rounds = 2;
};

struct ProtocolIIITwoRoundOutput {
  ProtocolIIIField selected_share;
  ProtocolIIITwoRoundMetrics metrics;
};

// Owns the one-shot state for both framed exchanges. A partial I/O failure
// destroys the state; callers cannot retry with the moved material.
[[nodiscard]] ProtocolIIITwoRoundOutput protocol_iii_two_round_party(
    ProtocolIIITwoRoundConfig config,
    ProtocolIIITwoRoundPartyMaterial&& material,
    std::vector<std::uint64_t> priority_key_shares,
    std::vector<ProtocolIIIField> encoded_payload_shares,
    ProtocolIIITwoRoundFds fds,
    int timeout_ms);

}  // namespace moe_topk
