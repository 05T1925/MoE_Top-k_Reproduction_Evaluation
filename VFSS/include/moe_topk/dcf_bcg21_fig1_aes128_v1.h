#pragma once

#if !defined(MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL)
#error "DCF_BCG21_FIG1_AES128_V1 requires the opt-in Select5 experimental build"
#endif

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace moe_topk::bcg21_fig1_aes128_v1 {

inline constexpr std::uint8_t kAbiId = 1;
inline constexpr const char* kAlgorithmLabel =
    "DCF_BCG21_FIG1_AES128_V1";
inline constexpr std::uint8_t kOutputBits = 64;

using Seed = std::array<std::uint8_t, 16>;

struct CorrectionWord {
  Seed seed{};
  std::uint64_t value = 0;
  std::uint8_t control = 0;  // bit 0: left; bit 1: right; remaining bits zero.
};

struct PartyKey {
  std::uint8_t party = 0;
  std::uint8_t input_bits = 0;
  Seed root_seed{};
  std::vector<CorrectionWord> correction_words;
  std::uint64_t final_correction = 0;

  PartyKey() = default;
  PartyKey(const PartyKey&) = default;
  PartyKey& operator=(const PartyKey&) = default;
  PartyKey(PartyKey&&) noexcept = default;
  PartyKey& operator=(PartyKey&&) noexcept = default;
  ~PartyKey();
};

struct KeyPair {
  PartyKey party0;
  PartyKey party1;
};

// The production entry point uses OpenSSL RAND_bytes for two independent
// 128-bit roots. Failure to obtain the full root tape throws; there is no
// deterministic or legacy-DCF fallback.
KeyPair keygen(std::uint8_t input_bits, std::uint64_t alpha,
               std::uint64_t beta);

// Deterministic entry point for conformance vectors only. Runtime/dealer code
// must call keygen(), never this helper.
KeyPair keygen_for_test(std::uint8_t input_bits, std::uint64_t alpha,
                        std::uint64_t beta, const Seed& root0,
                        const Seed& root1);

std::uint64_t eval_share(const PartyKey& key, std::uint64_t input);

std::size_t serialized_size(std::uint8_t input_bits);
std::vector<std::uint8_t> serialize(const PartyKey& key);
PartyKey deserialize(const std::vector<std::uint8_t>& encoded);

// Test hook for the raw-byte AES adapter. The conformance suite checks the
// FIPS-197 AES-128 known-answer vector through this same implementation path.
std::array<std::uint8_t, 16> aes128_encrypt_block_for_test(
    const Seed& key, const std::array<std::uint8_t, 16>& plaintext);

}  // namespace moe_topk::bcg21_fig1_aes128_v1
