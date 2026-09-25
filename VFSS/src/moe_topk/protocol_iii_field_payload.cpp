#include <moe_topk/protocol_iii_field_payload.h>

#include <cryptoTools/Common/Defines.h>

#include <immintrin.h>

#include <limits>
#include <stdexcept>

namespace moe_topk {
namespace {
using Wide = ProtocolIIIField::Storage;

void validate_key_bits(std::uint8_t key_bits) {
  if (key_bits == 0U || key_bits > 62U) {
    throw std::invalid_argument("Protocol III field packed key width must be 1..62");
  }
}

Wide random_canonical_bits(osuCrypto::PRNG& generator) {
  for (;;) {
    const auto block = generator.get<osuCrypto::block>();
    const Wide candidate =
        (Wide{static_cast<std::uint64_t>(_mm_extract_epi64(block, 1))} << 64U) |
        static_cast<std::uint64_t>(_mm_extract_epi64(block, 0));
    if (candidate < ProtocolIIIField::modulus()) return candidate;
  }
}
}  // namespace

ProtocolIIIField protocol_iii_encode_payload_nonzero(std::uint64_t payload) {
  return ProtocolIIIField::from_canonical(Wide{payload} + 1U);
}

std::uint64_t protocol_iii_decode_payload_nonzero(ProtocolIIIField encoded) {
  if (encoded.is_zero() || encoded.value() > (Wide{1} << 64U)) {
    throw std::invalid_argument("Protocol III field payload encoding outside uint64 range");
  }
  return static_cast<std::uint64_t>(encoded.value() - 1U);
}

ProtocolIIIField protocol_iii_pack_key_payload_nonzero(
    std::uint64_t key, std::uint8_t key_bits, std::uint64_t payload) {
  validate_key_bits(key_bits);
  if (key >= (UINT64_C(1) << key_bits)) {
    throw std::invalid_argument("Protocol III field key exceeds declared width");
  }
  const Wide packed = (Wide{key} << 64U) | payload;
  return ProtocolIIIField::from_canonical(packed + 1U);
}

std::pair<std::uint64_t, std::uint64_t>
protocol_iii_unpack_key_payload_nonzero(
    ProtocolIIIField encoded, std::uint8_t key_bits) {
  validate_key_bits(key_bits);
  if (encoded.is_zero()) {
    throw std::invalid_argument("Protocol III field packed payload is zero");
  }
  const Wide packed = encoded.value() - 1U;
  const Wide key = packed >> 64U;
  if (key >= (Wide{1} << key_bits)) {
    throw std::invalid_argument("Protocol III field packed key exceeds declared width");
  }
  return {static_cast<std::uint64_t>(key), static_cast<std::uint64_t>(packed)};
}

ProtocolIIIField protocol_iii_sample_field_element(osuCrypto::PRNG& generator) {
  return ProtocolIIIField::from_canonical(random_canonical_bits(generator));
}

ProtocolIIIField protocol_iii_sample_nonzero_field_element(osuCrypto::PRNG& generator) {
  for (;;) {
    const auto result = protocol_iii_sample_field_element(generator);
    if (!result.is_zero()) return result;
  }
}

std::pair<ProtocolIIIField, ProtocolIIIField>
protocol_iii_split_field_element(ProtocolIIIField value, osuCrypto::PRNG& generator) {
  const auto first = protocol_iii_sample_field_element(generator);
  return {first, ProtocolIIIField::sub(value, first)};
}

ProtocolIIIField protocol_iii_mask_field_payload(
    ProtocolIIIField encoded_nonzero, ProtocolIIIField nonzero_mask) {
  if (encoded_nonzero.is_zero() || nonzero_mask.is_zero()) {
    throw std::invalid_argument("Protocol III field payload and mask must be nonzero");
  }
  return ProtocolIIIField::mul(encoded_nonzero, nonzero_mask);
}

}  // namespace moe_topk
