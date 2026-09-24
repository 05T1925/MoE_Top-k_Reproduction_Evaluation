#pragma once

#include <moe_topk/protocol_iii_dpf_routing.h>
#include <moe_topk/protocol_iii_field.h>

#include <cryptoTools/Crypto/PRNG.h>

#include <cstdint>
#include <utility>
#include <vector>

namespace moe_topk {

// Protocol-III-only field-output DPF. Reuses the native DPF tree keys, but
// never interprets their Z_(2^64) payload correction as a field element.
class ProtocolIIIFieldDpfPartyKey {
 public:
  ProtocolIIIFieldDpfPartyKey(
      ProtocolIIIDpfRoutingKey&& tree_key,
      ProtocolIIIField field_correction,
      std::uint8_t party,
      std::uint64_t session,
      std::uint64_t fingerprint,
      std::uint32_t slot);

  ProtocolIIIFieldDpfPartyKey(const ProtocolIIIFieldDpfPartyKey&) = delete;
  ProtocolIIIFieldDpfPartyKey& operator=(const ProtocolIIIFieldDpfPartyKey&) = delete;
  ProtocolIIIFieldDpfPartyKey(ProtocolIIIFieldDpfPartyKey&&) noexcept = default;
  ProtocolIIIFieldDpfPartyKey& operator=(ProtocolIIIFieldDpfPartyKey&&) noexcept = default;

  [[nodiscard]] std::uint8_t party() const noexcept { return party_; }
  [[nodiscard]] std::uint8_t domain_bits() const noexcept {
    return static_cast<std::uint8_t>(tree_key_.native_key().bin);
  }
  [[nodiscard]] std::uint64_t session() const noexcept { return session_; }
  [[nodiscard]] std::uint64_t fingerprint() const noexcept { return fingerprint_; }
  [[nodiscard]] std::uint32_t slot() const noexcept { return slot_; }
  [[nodiscard]] std::vector<std::uint8_t> serialize() const;
  static ProtocolIIIFieldDpfPartyKey deserialize(
      const std::vector<std::uint8_t>& bytes);

 private:
  ProtocolIIIDpfRoutingKey tree_key_;
  ProtocolIIIField correction_;
  std::uint8_t party_;
  std::uint64_t session_;
  std::uint64_t fingerprint_;
  std::uint32_t slot_;

  friend ProtocolIIIField protocol_iii_field_dpf_eval(
      const ProtocolIIIFieldDpfPartyKey&,
      std::uint8_t, std::uint64_t, std::uint64_t, std::uint32_t,
      std::uint64_t);
};

[[nodiscard]] std::pair<ProtocolIIIFieldDpfPartyKey, ProtocolIIIFieldDpfPartyKey>
protocol_iii_field_dpf_generate(
    std::uint8_t domain_bits,
    std::uint64_t alpha,
    ProtocolIIIField beta,
    std::uint64_t session,
    std::uint64_t fingerprint,
    std::uint32_t slot);

[[nodiscard]] ProtocolIIIField protocol_iii_field_dpf_eval(
    const ProtocolIIIFieldDpfPartyKey& key,
    std::uint8_t expected_party,
    std::uint64_t expected_session,
    std::uint64_t expected_fingerprint,
    std::uint32_t expected_slot,
    std::uint64_t input);

// Dealer-only factory. P0 and P1 receive one additive mask share and one
// DPF key each. Neither material contains the full s or scalar s^{-1}.
struct ProtocolIIIFieldPayloadPartyMaterial {
  ProtocolIIIField mask_share;
  ProtocolIIIFieldDpfPartyKey dpf_key;
};

[[nodiscard]] std::pair<ProtocolIIIFieldPayloadPartyMaterial,
                        ProtocolIIIFieldPayloadPartyMaterial>
protocol_iii_field_payload_preprocess(
    std::uint8_t domain_bits,
    std::uint64_t rank_mask_alpha,
    ProtocolIIIField nonzero_payload_mask,
    std::uint64_t session,
    std::uint64_t fingerprint,
    std::uint32_t slot,
    osuCrypto::PRNG& dealer_generator);

}  // namespace moe_topk
