#include <moe_topk/protocol_iii_field.h>

#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

namespace {
using Field = moe_topk::ProtocolIIIField;
using Wide = Field::Storage;

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

template <typename Function>
void require_reject(Function function, const char* message) {
  try { function(); } catch (const std::invalid_argument&) { return; }
  catch (const std::domain_error&) { return; }
  throw std::runtime_error(message);
}

Wide oracle_add(Wide a, Wide b) {
  Wide result = a + b;
  if (result >= Field::modulus()) result -= Field::modulus();
  return result;
}

// Independent double-and-add oracle, separate from production limb reduction.
Wide oracle_mul(Wide a, Wide b) {
  Wide result = 0;
  while (b != 0) {
    if ((b & 1U) != 0) result = oracle_add(result, a);
    a = oracle_add(a, a);
    b >>= 1U;
  }
  return result;
}

Wide random_canonical(std::mt19937_64& generator) {
  const Wide value = (Wide{generator() & UINT64_C(0x7fffffffffffffff)} << 64U) | generator();
  return value == Field::modulus() ? 0 : value;
}

void test_boundaries() {
  const Field zero = Field::from_u64(0);
  const Field one = Field::from_u64(1);
  const Field maximum = Field::from_canonical(Field::modulus() - 1U);
  require(Field::add(maximum, one) == zero, "p-1 + 1");
  require(Field::sub(zero, one) == maximum, "0 - 1");
  require(Field::mul(maximum, maximum) == one, "(p-1)^2");
  require(Field::inv(one) == one, "inverse 1");
  require(Field::inv(maximum) == maximum, "inverse p-1");
  require_reject([&] { (void)Field::inv(zero); }, "inverse zero accepted");
  require_reject([&] { (void)Field::from_canonical(Field::modulus()); }, "p accepted as canonical");
  require_reject([&] { (void)Field::from_canonical(Field::modulus() + 1U); }, "p+1 accepted as canonical");
  require_reject([&] { (void)Field::deserialize(std::vector<std::uint8_t>(15, 0)); }, "truncated element accepted");
  require_reject([&] { (void)Field::deserialize(std::vector<std::uint8_t>(17, 0)); }, "oversize element accepted");
  auto invalid = maximum.serialize();
  invalid[15] = 0xff;
  require_reject([&] { (void)Field::deserialize(invalid); }, "noncanonical encoding accepted");
  require(Field::deserialize(zero.serialize()) == zero, "zero serialization");
  require(Field::deserialize(maximum.serialize()) == maximum, "maximum serialization");
  require(one.serialize().back() == 1U, "big endian wire encoding");
}

void test_randomized() {
  std::mt19937_64 generator(UINT64_C(0x5d20260925));
  for (int case_index = 0; case_index < 5000; ++case_index) {
    const Wide a = random_canonical(generator);
    const Wide b = random_canonical(generator);
    const Field fa = Field::from_canonical(a);
    const Field fb = Field::from_canonical(b);
    require(Field::add(fa, fb).value() == oracle_add(a, b), "random add");
    require(Field::sub(Field::add(fa, fb), fb) == fa, "random sub");
    require(Field::mul(fa, fb).value() == oracle_mul(a, b), "random mul");
    require(Field::deserialize(fa.serialize()) == fa, "random serialization");
    if (case_index < 256 && !fa.is_zero()) {
      require(Field::mul(fa, Field::inv(fa)) == Field::from_u64(1), "random inverse");
    }
  }
}
}  // namespace

int main() {
  try {
    test_boundaries();
    test_randomized();
    std::cout << "Protocol III field arithmetic: PASS (5000 randomized, 256 inverses)\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Protocol III field arithmetic: FAIL: " << error.what() << '\n';
    return 1;
  }
}
