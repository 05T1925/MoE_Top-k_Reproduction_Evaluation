#include <moe_topk/dcf_bcg21_fig1_aes128_v1.h>
#include <moe_topk/protocol_i_ucmp.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace dcf = moe_topk::bcg21_fig1_aes128_v1;

void require(bool condition, const std::string& message) {
  if (!condition) throw std::runtime_error(message);
}

dcf::Seed seed_with(std::uint8_t start) {
  dcf::Seed seed{};
  for (std::size_t i = 0; i < seed.size(); ++i)
    seed[i] = static_cast<std::uint8_t>(start + 17U * i);
  return seed;
}

void check_inputs(std::uint8_t bits, std::uint64_t alpha, std::uint64_t beta,
                  const std::vector<std::uint64_t>& inputs,
                  std::uint8_t seed_tag) {
  const auto roots0 = seed_with(seed_tag);
  const auto roots1 = seed_with(static_cast<std::uint8_t>(seed_tag + 83U));
  auto pair = dcf::keygen_for_test(bits, alpha, beta, roots0, roots1);
  const auto encoded0 = dcf::serialize(pair.party0);
  const auto encoded1 = dcf::serialize(pair.party1);
  require(encoded0.size() == dcf::serialized_size(bits) &&
              encoded1.size() == dcf::serialized_size(bits),
          "DCF encoded key size");
  const auto decoded0 = dcf::deserialize(encoded0);
  const auto decoded1 = dcf::deserialize(encoded1);
  require(decoded0.party == 0 && decoded1.party == 1,
          "DCF party serialization identity");
  for (const auto x : inputs) {
    const auto expected = x < alpha ? beta : 0;
    const auto actual0 = dcf::eval_share(pair.party0, x);
    const auto actual1 = dcf::eval_share(pair.party1, x);
    require(actual0 + actual1 == expected,
            "DCF function mismatch bits=" + std::to_string(bits) +
                " alpha=" + std::to_string(alpha) + " x=" + std::to_string(x) +
                " shares=" + std::to_string(actual0) + "," +
                std::to_string(actual1));
    require(dcf::eval_share(decoded0, x) + dcf::eval_share(decoded1, x) == expected,
            "DCF serialized key mismatch");
  }
}

void check_ucmp_adapter() {
  for (const int bits : {34, 40, 53}) {
    const auto ring_mask = (UINT64_C(1) << bits) - 1U;
    const auto half = UINT64_C(1) << (bits - 1);
    const std::array<std::pair<std::uint64_t, std::uint64_t>, 5> pairs{{
        {0, 0}, {0, 1}, {1, 0}, {half - 1U, half}, {half, half - 1U}}};
    for (const auto& values : pairs) {
      const std::uint64_t left_mask = ring_mask - 7U;
      const std::uint64_t right_mask = half + 3U;
      moe_topk::ProtocolIUcmpMaterial dealer(
          bits, left_mask, right_mask,
          moe_topk::ProtocolIDcfAbi::Bcg21Fig1Aes128V1);
      auto p0 = moe_topk::ProtocolIUcmpPartyMaterial::deserialize(
          dealer.export_party_material(0).serialize());
      auto p1 = moe_topk::ProtocolIUcmpPartyMaterial::deserialize(
          dealer.export_party_material(1).serialize());
      require(p0.dcf_abi() == moe_topk::ProtocolIDcfAbi::Bcg21Fig1Aes128V1 &&
                  p1.dcf_abi() == moe_topk::ProtocolIDcfAbi::Bcg21Fig1Aes128V1,
              "uCMP BCG21 ABI round trip");
      const auto z_left = (values.first + left_mask) & ring_mask;
      const auto z_right = (values.second + right_mask) & ring_mask;
      const auto share0 = p0.eval_strict_lt(z_left, z_right);
      const auto share1 = p1.eval_strict_lt(z_left, z_right);
      require(share0 + share1 == static_cast<std::uint64_t>(
                  values.first < values.second),
              "uCMP BCG21 adapter frozen comparison");
      bool replay_rejected = false;
      try { (void)p0.eval_strict_lt(z_left, z_right); }
      catch (const std::invalid_argument&) { replay_rejected = true; }
      require(replay_rejected, "uCMP BCG21 material is one-shot");
    }
  }
}

int main() {
  try {
    const dcf::Seed aes_key{{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                             0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f}};
    const std::array<std::uint8_t, 16> plaintext{{
        0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,
        0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff}};
    const std::array<std::uint8_t, 16> ciphertext{{
        0x69,0xc4,0xe0,0xd8,0x6a,0x7b,0x04,0x30,
        0xd8,0xcd,0xb7,0x80,0x70,0xb4,0xc5,0x5a}};
    require(dcf::aes128_encrypt_block_for_test(aes_key, plaintext) == ciphertext,
            "AES-128 FIPS-197 known-answer vector");

    std::uint8_t tag = 1;
    for (std::uint8_t bits = 1; bits <= 8; ++bits) {
      const std::uint64_t size = UINT64_C(1) << bits;
      const std::uint64_t mask = size - 1U;
      for (std::uint64_t alpha = 0; alpha < size; ++alpha) {
        const std::array<std::uint64_t, 3> betas{{0, 1, UINT64_MAX}};
        for (const auto beta : betas) {
          std::vector<std::uint64_t> inputs;
          if (bits <= 5) {
            inputs.resize(static_cast<std::size_t>(size));
            for (std::uint64_t x = 0; x < size; ++x)
              inputs[static_cast<std::size_t>(x)] = x;
          } else {
            inputs = {0, alpha == 0 ? 0 : alpha - 1U, alpha,
                      alpha == mask ? mask : alpha + 1U, mask};
          }
          check_inputs(bits, alpha, beta, inputs, tag++);
        }
      }
    }

    for (std::uint8_t bits = 34; bits <= 53; ++bits) {
      const auto mask = (UINT64_C(1) << bits) - 1U;
      const std::array<std::uint64_t, 7> alphas{{
          0, 1, 2, mask / 2U, mask - 1U, mask, UINT64_C(0x123456789abcdef) & mask}};
      const std::array<std::uint64_t, 4> betas{{0, 1, UINT64_MAX, UINT64_C(0x8000000000000001)}};
      for (const auto alpha : alphas) {
        std::array<std::uint64_t, 5> xs{{
            0, alpha == 0 ? 0 : alpha - 1U, alpha,
            alpha == mask ? mask : alpha + 1U, mask}};
        for (const auto beta : betas)
          check_inputs(bits, alpha, beta,
                       std::vector<std::uint64_t>(xs.begin(), xs.end()), tag++);
      }
    }

    auto malformed = dcf::keygen_for_test(34, 5, 9, seed_with(7), seed_with(91));
    auto invalid = dcf::serialize(malformed.party0);
    invalid[5] = 2;
    bool rejected = false;
    try { (void)dcf::deserialize(invalid); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "invalid party key identity must reject");
    invalid = dcf::serialize(malformed.party0);
    invalid.pop_back();
    rejected = false;
    try { (void)dcf::deserialize(invalid); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "truncated DCF key must reject");
    invalid = dcf::serialize(malformed.party0);
    invalid[4] = 2;
    rejected = false;
    try { (void)dcf::deserialize(invalid); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected, "wrong DCF ABI version must reject");
    check_ucmp_adapter();
    std::cout << "PASS DCF_BCG21_FIG1_AES128_V1 conformance\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "FAIL DCF_BCG21_FIG1_AES128_V1: " << error.what() << '\n';
    return 1;
  }
}
