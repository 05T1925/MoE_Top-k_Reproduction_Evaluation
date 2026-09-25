#pragma once

#include <moe_topk/protocol_iii_field.h>

#include <cryptoTools/Crypto/PRNG.h>

#include <cstdint>
#include <utility>
#include <vector>

namespace moe_topk {

// Offline dealer material for one field multiplication of additive shares.
// This is an isolated primitive, with no transport or online-round state machine.
struct ProtocolIIIFieldMulPartyMaterial {
  std::uint64_t session = 0;
  std::uint64_t fingerprint = 0;
  std::uint32_t slot = 0;
  std::uint8_t party = 0;
  ProtocolIIIField a_share;
  ProtocolIIIField b_share;
  ProtocolIIIField c_share;
  bool started = false;
  bool consumed = false;

  [[nodiscard]] std::vector<std::uint8_t> serialize() const;
  static ProtocolIIIFieldMulPartyMaterial deserialize(
      const std::vector<std::uint8_t>& bytes);
};

struct ProtocolIIIFieldMulOpeningShare {
  ProtocolIIIField d_share;
  ProtocolIIIField e_share;
};

[[nodiscard]] std::pair<ProtocolIIIFieldMulPartyMaterial,
                        ProtocolIIIFieldMulPartyMaterial>
protocol_iii_field_mul_preprocess(
    std::uint64_t session, std::uint64_t fingerprint,
    std::uint32_t slot, osuCrypto::PRNG& dealer_generator);

[[nodiscard]] ProtocolIIIFieldMulOpeningShare protocol_iii_field_mul_start(
    ProtocolIIIFieldMulPartyMaterial& material,
    std::uint8_t expected_party, std::uint64_t expected_session,
    std::uint64_t expected_fingerprint, std::uint32_t expected_slot,
    ProtocolIIIField payload_share, ProtocolIIIField mask_share);

// Public d/e are opened masked operands. Output remains an additive field
// share; opening the product requires a separate, explicit exchange.
[[nodiscard]] ProtocolIIIField protocol_iii_field_mul_finish(
    ProtocolIIIFieldMulPartyMaterial& material,
    std::uint8_t expected_party, std::uint64_t expected_session,
    std::uint64_t expected_fingerprint, std::uint32_t expected_slot,
    ProtocolIIIField public_d, ProtocolIIIField public_e);

}  // namespace moe_topk
