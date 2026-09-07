#pragma once

#include <moe_topk/masked_mul_adapter.h>

#include <cstdint>
#include <vector>

namespace moe_topk {

struct ProtocolIIISecureCombineConfig {
  std::uint64_t session = 0;
  std::uint64_t fingerprint = 0;

  std::uint32_t logical_n = 0;
  std::uint32_t k = 0;

  // Used to bind the frame to the preceding Protocol III request.
  // Multiplication itself remains in Z_(2^64).
  std::uint8_t comparison_bits = 0;
  std::uint8_t party = 0;

  int timeout_ms = 0;
};

struct ProtocolIIISecureCombinePartyMaterial {
  std::uint64_t session = 0;
  std::uint64_t fingerprint = 0;

  std::uint32_t logical_n = 0;
  std::uint32_t k = 0;

  std::uint8_t party = 0;

  // Row-major logical_n x k one-shot multiplication material.
  std::vector<MaskedMulMaterial> multiplication_materials;

  ProtocolIIISecureCombinePartyMaterial() = default;

  ProtocolIIISecureCombinePartyMaterial(
      const ProtocolIIISecureCombinePartyMaterial&) = delete;

  ProtocolIIISecureCombinePartyMaterial& operator=(
      const ProtocolIIISecureCombinePartyMaterial&) = delete;

  ProtocolIIISecureCombinePartyMaterial(
      ProtocolIIISecureCombinePartyMaterial&&) noexcept = default;

  ProtocolIIISecureCombinePartyMaterial& operator=(
      ProtocolIIISecureCombinePartyMaterial&&) noexcept = default;
};

struct ProtocolIIISecureCombineMetrics {
  std::uint64_t sent_bytes = 0;
  std::uint64_t received_bytes = 0;

  std::uint64_t multiplication_calls = 0;
  std::uint64_t opened_masked_values = 0;

  std::uint64_t online_rounds = 1;
};

struct ProtocolIIISecureCombineOutput {
  // Original-order XOR shares of the Top-K bit-mask.
  std::vector<std::uint8_t> xor_mask_shares;

  ProtocolIIISecureCombineMetrics metrics;
};

// indicator_shares is a row-major logical_n x k matrix in Z_(2^64).
//
// unit_payload_shares contains one additive share of u_i=1 for every logical
// input position. The same u_i share is used with fresh multiplication
// material for every target rank.
//
// The runtime never reconstructs indicators, products, selected indices or
// the arithmetic mask.
[[nodiscard]] ProtocolIIISecureCombineOutput
protocol_iii_secure_combine_party(
    const ProtocolIIISecureCombineConfig& config,
    ProtocolIIISecureCombinePartyMaterial& material,
    const std::vector<std::uint64_t>& indicator_shares,
    const std::vector<std::uint64_t>& unit_payload_shares,
    int combine_fd);

}  // namespace moe_topk
