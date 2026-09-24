#include <moe_topk/protocol_iii_field.h>

#include <array>
#include <stdexcept>

namespace moe_topk {
namespace {

using Word = std::uint64_t;
using Wide = ProtocolIIIField::Storage;

constexpr Wide kModulus = ProtocolIIIField::modulus();
constexpr Word kLow63 = (Word{1} << 63U) - 1U;

Word low64(Wide value) noexcept {
  return static_cast<Word>(value);
}

Word high64(Wide value) noexcept {
  return static_cast<Word>(value >> 64U);
}

// Four 64-bit limbs of a 128x128 product, using exact 128-bit partial
// products. No overflowing 128-bit intermediate represents the full product.
std::array<Word, 4> product_limbs(Wide left, Wide right) noexcept {
  const Word a0 = low64(left), a1 = high64(left);
  const Word b0 = low64(right), b1 = high64(right);
  const Wide p00 = Wide{a0} * b0;
  const Wide p01 = Wide{a0} * b1;
  const Wide p10 = Wide{a1} * b0;
  const Wide p11 = Wide{a1} * b1;

  const Wide middle1 =
      Wide{high64(p00)} + low64(p01) + low64(p10);
  const Wide middle2 =
      Wide{high64(p01)} + high64(p10) +
      low64(p11) + high64(middle1);
  const Wide top = Wide{high64(p11)} + high64(middle2);
  return {low64(p00), low64(middle1), low64(middle2),
          static_cast<Word>(top)};
}

Wide reduce_mersenne(const std::array<Word, 4>& limbs) noexcept {
  // For p=2^127-1, 2^127 = 1 (mod p). Inputs are <p, so their product
  // is <2^254. Fold the low and high 127-bit halves, then fold once more.
  const Wide low =
      (Wide{limbs[1] & kLow63} << 64U) | limbs[0];
  const Word high0 =
      (limbs[1] >> 63U) | (limbs[2] << 1U);
  const Word high1 =
      (limbs[2] >> 63U) | (limbs[3] << 1U);
  const Wide high =
      (Wide{high1} << 64U) | high0;
  const Wide first = low + high;
  Wide folded = (first & kModulus) + (first >> 127U);
  if (folded >= kModulus) {
    folded -= kModulus;
  }
  return folded;
}

}  // namespace

ProtocolIIIField ProtocolIIIField::from_canonical(Storage value) {
  if (value >= modulus()) {
    throw std::invalid_argument("Protocol III field element is noncanonical");
  }
  return ProtocolIIIField(value);
}

ProtocolIIIField ProtocolIIIField::from_u64(
    std::uint64_t value) noexcept {
  return ProtocolIIIField(Storage{value});
}

std::vector<std::uint8_t> ProtocolIIIField::serialize() const {
  std::vector<std::uint8_t> bytes(16U);
  for (std::size_t index = 0; index < bytes.size(); ++index) {
    bytes[15U - index] =
        static_cast<std::uint8_t>(value_ >> (8U * index));
  }
  return bytes;
}

ProtocolIIIField ProtocolIIIField::deserialize(
    const std::vector<std::uint8_t>& bytes) {
  if (bytes.size() != 16U) {
    throw std::invalid_argument("Protocol III field encoding length");
  }
  Storage value = 0;
  for (const auto byte : bytes) {
    value = (value << 8U) | byte;
  }
  return from_canonical(value);
}

ProtocolIIIField ProtocolIIIField::add(
    ProtocolIIIField left, ProtocolIIIField right) noexcept {
  // left,right < 2^127-1, so the sum fits in 128 bits.
  Storage sum = left.value_ + right.value_;
  if (sum >= modulus()) {
    sum -= modulus();
  }
  return ProtocolIIIField(sum);
}

ProtocolIIIField ProtocolIIIField::sub(
    ProtocolIIIField left, ProtocolIIIField right) noexcept {
  if (left.value_ >= right.value_) {
    return ProtocolIIIField(left.value_ - right.value_);
  }
  return ProtocolIIIField(
      modulus() - (right.value_ - left.value_));
}

ProtocolIIIField ProtocolIIIField::mul(
    ProtocolIIIField left, ProtocolIIIField right) noexcept {
  return ProtocolIIIField(
      reduce_mersenne(product_limbs(left.value_, right.value_)));
}

ProtocolIIIField ProtocolIIIField::inv(ProtocolIIIField value) {
  if (value.is_zero()) {
    throw std::domain_error("Protocol III field inverse of zero");
  }
  // p is prime; Fermat's little theorem gives x^(p-2).
  Storage exponent = modulus() - 2U;
  ProtocolIIIField result = from_u64(1);
  while (exponent != 0) {
    if ((exponent & 1U) != 0) {
      result = mul(result, value);
    }
    value = mul(value, value);
    exponent >>= 1U;
  }
  return result;
}

}  // namespace moe_topk
