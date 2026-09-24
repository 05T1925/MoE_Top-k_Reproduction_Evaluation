#include <moe_topk/protocol_iii_field_mul.h>
#include <moe_topk/protocol_iii_field_payload.h>

#include <cryptoTools/Common/Defines.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

namespace {
using namespace moe_topk;
using Field = ProtocolIIIField;
constexpr std::uint64_t kSession = UINT64_C(0x5d20260925);
constexpr std::uint64_t kFingerprint = UINT64_C(0x5d20260926);

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

template <typename Function>
void require_reject(Function function, const char* message) {
  try { function(); } catch (const std::invalid_argument&) { return; }
  throw std::runtime_error(message);
}

void test_randomized() {
  osuCrypto::PRNG dealer(osuCrypto::toBlock(kSession, kFingerprint));
  std::mt19937_64 generator(UINT64_C(0x520260925));
  for (std::uint32_t index = 0; index < 2000; ++index) {
    const auto payload = protocol_iii_encode_payload_nonzero(generator());
    const auto mask = protocol_iii_sample_nonzero_field_element(dealer);
    const auto payload_shares = protocol_iii_split_field_element(payload, dealer);
    const auto mask_shares = protocol_iii_split_field_element(mask, dealer);
    auto material = protocol_iii_field_mul_preprocess(kSession, kFingerprint, index, dealer);
    auto first = ProtocolIIIFieldMulPartyMaterial::deserialize(material.first.serialize());
    auto second = ProtocolIIIFieldMulPartyMaterial::deserialize(material.second.serialize());
    require_reject([&] { (void)protocol_iii_field_mul_start(
        first, 1U, kSession, kFingerprint, index,
        payload_shares.first, mask_shares.first); }, "party mismatch accepted");
    require_reject([&] { (void)protocol_iii_field_mul_start(
        first, 0U, kSession + 1U, kFingerprint, index,
        payload_shares.first, mask_shares.first); }, "session mismatch accepted");
    const auto opening0 = protocol_iii_field_mul_start(
        first, 0U, kSession, kFingerprint, index,
        payload_shares.first, mask_shares.first);
    const auto opening1 = protocol_iii_field_mul_start(
        second, 1U, kSession, kFingerprint, index,
        payload_shares.second, mask_shares.second);
    require_reject([&] { (void)protocol_iii_field_mul_start(
        first, 0U, kSession, kFingerprint, index,
        payload_shares.first, mask_shares.first); }, "material replay accepted");
    require_reject([&] { (void)first.serialize(); }, "started material serialized as fresh");
    const auto public_d = Field::add(opening0.d_share, opening1.d_share);
    const auto public_e = Field::add(opening0.e_share, opening1.e_share);
    const auto share0 = protocol_iii_field_mul_finish(
        first, 0U, kSession, kFingerprint, index, public_d, public_e);
    const auto share1 = protocol_iii_field_mul_finish(
        second, 1U, kSession, kFingerprint, index, public_d, public_e);
    const auto public_masked = Field::add(share0, share1);
    require(public_masked == Field::mul(payload, mask), "field Beaver product");
    require(Field::mul(public_masked, Field::inv(mask)) == payload,
            "field Beaver mask/unmask");
    require_reject([&] { (void)protocol_iii_field_mul_finish(
        first, 0U, kSession, kFingerprint, index, public_d, public_e); },
        "consumed material replay accepted");
  }
}

void test_malformed_wire() {
  osuCrypto::PRNG dealer(osuCrypto::toBlock(kSession + 3U, kFingerprint + 4U));
  auto material = protocol_iii_field_mul_preprocess(kSession, kFingerprint, 7U, dealer);
  auto wire = material.first.serialize();
  wire.pop_back();
  require_reject([&] { (void)ProtocolIIIFieldMulPartyMaterial::deserialize(wire); },
                 "truncated multiplication material accepted");
  wire = material.first.serialize();
  wire.push_back(0U);
  require_reject([&] { (void)ProtocolIIIFieldMulPartyMaterial::deserialize(wire); },
                 "oversize multiplication material accepted");
  wire = material.first.serialize();
  wire[0] = 'X';
  require_reject([&] { (void)ProtocolIIIFieldMulPartyMaterial::deserialize(wire); },
                 "wrong multiplication material tag accepted");
  wire = material.first.serialize();
  std::fill(wire.begin() + 28U, wire.begin() + 44U, 0xff);
  require_reject([&] { (void)ProtocolIIIFieldMulPartyMaterial::deserialize(wire); },
                 "noncanonical field material accepted");
}
}  // namespace

int main() {
  try {
    test_randomized();
    test_malformed_wire();
    std::cout << "Protocol III field multiplication: PASS (2000 randomized)\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Protocol III field multiplication: FAIL: " << error.what() << '\n';
    return 1;
  }
}
