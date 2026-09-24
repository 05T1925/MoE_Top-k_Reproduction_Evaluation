#include <moe_topk/protocol_iii_field_payload.h>

#include <cryptoTools/Common/Defines.h>

#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>

namespace {
using namespace moe_topk;
using Field = ProtocolIIIField;

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

template <typename Function>
void require_reject(Function function, const char* message) {
  try { function(); } catch (const std::invalid_argument&) { return; }
  throw std::runtime_error(message);
}

void test_boundaries() {
  const auto max = std::numeric_limits<std::uint64_t>::max();
  for (const auto payload : {UINT64_C(0), UINT64_C(1), max}) {
    const auto encoded = protocol_iii_encode_payload_nonzero(payload);
    require(!encoded.is_zero(), "encoded payload zero");
    require(protocol_iii_decode_payload_nonzero(encoded) == payload, "payload roundtrip");
  }
  require_reject([&] { (void)protocol_iii_decode_payload_nonzero(Field{}); },
                 "zero field payload decoded");
  require_reject([&] { (void)protocol_iii_decode_payload_nonzero(
      Field::from_canonical((Field::Storage{1} << 64U) + 1U)); },
      "out-of-range field payload decoded");
  for (const auto bits : {std::uint8_t{1}, std::uint8_t{34}, std::uint8_t{53}, std::uint8_t{62}}) {
    const auto key = (UINT64_C(1) << bits) - 1U;
    const auto encoded = protocol_iii_pack_key_payload_nonzero(key, bits, max);
    require(!encoded.is_zero(), "packed payload zero");
    const auto decoded = protocol_iii_unpack_key_payload_nonzero(encoded, bits);
    require(decoded.first == key && decoded.second == max, "packed roundtrip");
  }
  require_reject([&] { (void)protocol_iii_pack_key_payload_nonzero(2, 1, 0); },
                 "oversize key accepted");
  require_reject([&] { (void)protocol_iii_pack_key_payload_nonzero(0, 0, 0); },
                 "zero key width accepted");
  require_reject([&] { (void)protocol_iii_pack_key_payload_nonzero(0, 63, 0); },
                 "63-bit key width accepted");
  require_reject([&] { (void)protocol_iii_unpack_key_payload_nonzero(Field{}, 34); },
                 "packed zero accepted");
  const auto wide = protocol_iii_pack_key_payload_nonzero(1000, 34, 0);
  require_reject([&] { (void)protocol_iii_unpack_key_payload_nonzero(wide, 1); },
                 "declared width mismatch accepted");
  require_reject([&] { (void)protocol_iii_mask_field_payload(Field{}, Field::from_u64(1)); },
                 "zero encoded payload masked");
  require_reject([&] { (void)protocol_iii_mask_field_payload(Field::from_u64(1), Field{}); },
                 "zero mask accepted");
}

void test_randomized() {
  osuCrypto::PRNG dealer(osuCrypto::toBlock(UINT64_C(0x5d20260925), UINT64_C(0x5d20260926)));
  std::mt19937_64 generator(UINT64_C(0xd20260925));
  for (int index = 0; index < 3000; ++index) {
    const auto payload = generator();
    const auto key = generator() & ((UINT64_C(1) << 53U) - 1U);
    const auto encoded = protocol_iii_encode_payload_nonzero(payload);
    const auto packed = protocol_iii_pack_key_payload_nonzero(key, 53U, payload);
    require(protocol_iii_decode_payload_nonzero(encoded) == payload, "random payload roundtrip");
    require(protocol_iii_unpack_key_payload_nonzero(packed, 53U) ==
                std::make_pair(key, payload), "random packed roundtrip");
    const auto mask = protocol_iii_sample_nonzero_field_element(dealer);
    require(!mask.is_zero(), "sampled zero mask");
    const auto inverse = Field::inv(mask);
    const auto public_masked = protocol_iii_mask_field_payload(encoded, mask);
    require(Field::mul(public_masked, inverse) == encoded, "mask/unmask");
    const auto shares = protocol_iii_split_field_element(mask, dealer);
    require(Field::add(shares.first, shares.second) == mask, "mask shares");
  }
}
}  // namespace

int main() {
  try {
    test_boundaries();
    test_randomized();
    std::cout << "Protocol III field payload: PASS (3000 randomized)\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Protocol III field payload: FAIL: " << error.what() << '\n';
    return 1;
  }
}
