#include <moe_topk/protocol_iii_field_dpf.h>
#include <moe_topk/protocol_iii_field_payload.h>
#include <moe_topk/protocol_iii_field_mul.h>

#include <FSS/dpf.h>
#include <FSS/prng.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

namespace {
using namespace moe_topk;
using Field = ProtocolIIIField;
constexpr std::uint64_t kSession = 0x5d20260925ULL;
constexpr std::uint64_t kFingerprint = 0x5d20260926ULL;

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

template <typename Function>
void require_reject(Function function, const char* message) {
  try { function(); } catch (const std::invalid_argument&) { return; }
  throw std::runtime_error(message);
}

void seed_fss() {
  for (int index = 0; index < 256; ++index) {
    FSSConfig::prngs[index].SetSeed(
        osuCrypto::toBlock(kSession, static_cast<std::uint64_t>(index)));
  }
}

void test_native_ring_gap() {
  auto native = keyGenDPF(3, 64, 3, 1);
  ProtocolIIIDpfRoutingKey first(std::move(native.first));
  ProtocolIIIDpfRoutingKey second(std::move(native.second));
  bool saw_field_mismatch = false;
  for (std::uint64_t input = 0; input < 8; ++input) {
    const auto a = evalDPF_Payload(0, first.native_key(), input);
    const auto b = evalDPF_Payload(1, second.native_key(), input);
    require(static_cast<std::uint64_t>(a + b) == (input == 3 ? 1U : 0U),
            "native ring point function");
    const auto field_reconstruction = Field::add(Field::from_u64(a), Field::from_u64(b));
    if (field_reconstruction != Field::from_u64(input == 3 ? 1U : 0U)) {
      saw_field_mismatch = true;
    }
  }
  require(saw_field_mismatch, "native ring shares silently reinterpreted as field");
}

void test_point_function() {
  std::mt19937_64 generator(UINT64_C(0x70260925));
  for (const auto bits : {std::uint8_t{1}, std::uint8_t{2}, std::uint8_t{3}, std::uint8_t{5}, std::uint8_t{8}}) {
    const std::uint64_t domain = UINT64_C(1) << bits;
    for (const auto alpha : {UINT64_C(0), domain / 2U, domain - 1U}) {
      const std::vector<Field> betas{
          Field::from_u64(0), Field::from_u64(1), Field::from_u64(2),
          Field::from_canonical(Field::modulus() - 1U),
          Field::from_canonical((Field::Storage{generator() & UINT64_C(0x7fffffffffffffff)} << 64U) | generator())};
      for (std::size_t beta_index = 0; beta_index < betas.size(); ++beta_index) {
        auto keys = protocol_iii_field_dpf_generate(
            bits, alpha, betas[beta_index], kSession, kFingerprint,
            static_cast<std::uint32_t>(beta_index));
        auto key0 = ProtocolIIIFieldDpfPartyKey::deserialize(keys.first.serialize());
        auto key1 = ProtocolIIIFieldDpfPartyKey::deserialize(keys.second.serialize());
        for (std::uint64_t input = 0; input < domain; ++input) {
          const auto share0 = protocol_iii_field_dpf_eval(
              key0, 0U, kSession, kFingerprint,
              static_cast<std::uint32_t>(beta_index), input);
          const auto share1 = protocol_iii_field_dpf_eval(
              key1, 1U, kSession, kFingerprint,
              static_cast<std::uint32_t>(beta_index), input);
          require(Field::add(share0, share1) ==
                      (input == alpha ? betas[beta_index] : Field{}),
                  "field DPF point reconstruction");
        }
        require_reject([&] { (void)protocol_iii_field_dpf_eval(
            key0, 1U, kSession, kFingerprint,
            static_cast<std::uint32_t>(beta_index), 0); },
            "party-swapped key accepted");
        require_reject([&] { (void)protocol_iii_field_dpf_eval(
            key0, 0U, kSession + 1U, kFingerprint,
            static_cast<std::uint32_t>(beta_index), 0); },
            "wrong session accepted");
        require_reject([&] { (void)protocol_iii_field_dpf_eval(
            key0, 0U, kSession, kFingerprint + 1U,
            static_cast<std::uint32_t>(beta_index), 0); },
            "wrong fingerprint accepted");
        require_reject([&] { (void)protocol_iii_field_dpf_eval(
            key0, 0U, kSession, kFingerprint,
            static_cast<std::uint32_t>(beta_index + 1U), 0); },
            "wrong slot accepted");
        require_reject([&] { (void)protocol_iii_field_dpf_eval(
            key0, 0U, kSession, kFingerprint,
            static_cast<std::uint32_t>(beta_index), domain); },
            "out-of-domain input accepted");
        auto wire = keys.first.serialize();
        wire.pop_back();
        require_reject([&] { (void)ProtocolIIIFieldDpfPartyKey::deserialize(wire); },
                       "truncated key accepted");
        wire = keys.first.serialize();
        wire.push_back(0);
        require_reject([&] { (void)ProtocolIIIFieldDpfPartyKey::deserialize(wire); },
                       "oversize key accepted");
        wire = keys.first.serialize();
        wire[0] = 'X';
        require_reject([&] { (void)ProtocolIIIFieldDpfPartyKey::deserialize(wire); },
                       "bad key tag accepted");
        wire = keys.first.serialize();
        wire[5] = 2U;
        require_reject([&] { (void)ProtocolIIIFieldDpfPartyKey::deserialize(wire); },
                       "bad key party accepted");
        wire = keys.first.serialize();
        std::fill(wire.begin() + 28, wire.begin() + 44, 0xff);
        require_reject([&] { (void)ProtocolIIIFieldDpfPartyKey::deserialize(wire); },
                       "noncanonical field correction accepted");
        auto moved = std::move(keys.first);
        require_reject([&] { (void)keys.first.serialize(); },
                       "moved-from field DPF key serialized");
        require(!moved.serialize().empty(), "moved field DPF key unavailable");
      }
    }
  }
}

void test_randomized_point_function() {
  std::mt19937_64 generator(UINT64_C(0x527d20260925));
  for (std::uint32_t case_index = 0; case_index < 128U; ++case_index) {
    const auto bits = static_cast<std::uint8_t>(2U + case_index % 15U);
    const auto domain = UINT64_C(1) << bits;
    const auto alpha = generator() % domain;
    const auto raw =
        (Field::Storage{generator() & UINT64_C(0x7fffffffffffffff)} << 64U) |
        generator();
    const auto beta = Field::from_canonical(
        raw == Field::modulus() ? Field::Storage{0} : raw);
    auto keys = protocol_iii_field_dpf_generate(
        bits, alpha, beta, kSession, kFingerprint, case_index + 100U);
    for (const auto input : {alpha, std::uint64_t{0}, domain - 1U, generator() % domain}) {
      const auto left = protocol_iii_field_dpf_eval(
          keys.first, 0U, kSession, kFingerprint, case_index + 100U, input);
      const auto right = protocol_iii_field_dpf_eval(
          keys.second, 1U, kSession, kFingerprint, case_index + 100U, input);
      require(Field::add(left, right) == (input == alpha ? beta : Field{}),
              "randomized field DPF point");
    }
  }
  const auto large_domain = UINT64_C(1) << 63U;
  auto keys = protocol_iii_field_dpf_generate(
      63U, large_domain - 1U, Field::from_u64(7), kSession, kFingerprint, 500U);
  for (const auto input : {UINT64_C(0), large_domain - 1U}) {
    const auto left = protocol_iii_field_dpf_eval(
        keys.first, 0U, kSession, kFingerprint, 500U, input);
    const auto right = protocol_iii_field_dpf_eval(
        keys.second, 1U, kSession, kFingerprint, 500U, input);
    require(Field::add(left, right) ==
                (input == large_domain - 1U ? Field::from_u64(7) : Field{}),
            "63-bit field DPF boundary");
  }
}

void test_payload_selection() {
  osuCrypto::PRNG dealer(osuCrypto::toBlock(kSession + 7U, kFingerprint + 11U));
  const std::vector<std::uint64_t> payloads{0U, 1U, UINT64_MAX, 42U, 7U};
  const std::vector<std::uint64_t> ranks{2U, 4U, 0U, 3U, 1U};
  const std::uint8_t bits = 3U;  // n=5 embedded in Z8.
  for (std::uint64_t target = 0; target < payloads.size(); ++target) {
    const auto session = kSession + 100U + target;
    Field sum0;
    Field sum1;
    for (std::size_t index = 0; index < payloads.size(); ++index) {
      const auto alpha = dealer.get<std::uint64_t>() & 7U;
      const auto mask = protocol_iii_sample_nonzero_field_element(dealer);
      auto material = protocol_iii_field_payload_preprocess(
          bits, alpha, mask, session, kFingerprint,
          static_cast<std::uint32_t>(index), dealer);
      require(Field::add(material.first.mask_share, material.second.mask_share) == mask,
              "dealer mask shares");
      const auto encoded = protocol_iii_pack_key_payload_nonzero(
          static_cast<std::uint64_t>(index + 1U), 8U, payloads[index]);
      const auto encoded_shares = protocol_iii_split_field_element(encoded, dealer);
      auto mul_material = protocol_iii_field_mul_preprocess(
          session, kFingerprint, static_cast<std::uint32_t>(index), dealer);
      const auto opening0 = protocol_iii_field_mul_start(
          mul_material.first, 0U, session, kFingerprint,
          static_cast<std::uint32_t>(index), encoded_shares.first,
          material.first.mask_share);
      const auto opening1 = protocol_iii_field_mul_start(
          mul_material.second, 1U, session, kFingerprint,
          static_cast<std::uint32_t>(index), encoded_shares.second,
          material.second.mask_share);
      const auto public_d = Field::add(opening0.d_share, opening1.d_share);
      const auto public_e = Field::add(opening0.e_share, opening1.e_share);
      const auto product0 = protocol_iii_field_mul_finish(
          mul_material.first, 0U, session, kFingerprint,
          static_cast<std::uint32_t>(index), public_d, public_e);
      const auto product1 = protocol_iii_field_mul_finish(
          mul_material.second, 1U, session, kFingerprint,
          static_cast<std::uint32_t>(index), public_d, public_e);
      const auto public_masked = Field::add(product0, product1);
      require(public_masked == Field::mul(encoded, mask), "field masked payload");
      const auto masked_rank = (ranks[index] + alpha) & 7U;
      const auto dpf_input = (masked_rank - target) & 7U;
      const auto left = protocol_iii_field_dpf_eval(
          material.first.dpf_key, 0U, session, kFingerprint,
          static_cast<std::uint32_t>(index), dpf_input);
      const auto right = protocol_iii_field_dpf_eval(
          material.second.dpf_key, 1U, session, kFingerprint,
          static_cast<std::uint32_t>(index), dpf_input);
      sum0 = Field::add(sum0, Field::mul(public_masked, left));
      sum1 = Field::add(sum1, Field::mul(public_masked, right));
    }
    const auto selected = Field::add(sum0, sum1);
    const auto selected_index = static_cast<std::size_t>(
        std::find(ranks.begin(), ranks.end(), target) - ranks.begin());
    require(selected_index < payloads.size(), "target rank absent");
    const auto decoded = protocol_iii_unpack_key_payload_nonzero(selected, 8U);
    require(decoded.first == selected_index + 1U &&
                decoded.second == payloads[selected_index],
            "field Fselect building-block key/payload selection");
  }
  require_reject([&] { (void)protocol_iii_field_payload_preprocess(
      3U, 0U, Field{}, kSession, kFingerprint, 0U, dealer); },
      "zero dealer mask accepted");
}
}  // namespace

int main() {
  try {
    seed_fss();
    test_native_ring_gap();
    test_point_function();
    test_randomized_point_function();
    test_payload_selection();
    std::cout << "Protocol III field DPF: PASS (native ring gap, point, payload)\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Protocol III field DPF: FAIL: " << error.what() << '\n';
    return 1;
  }
}
