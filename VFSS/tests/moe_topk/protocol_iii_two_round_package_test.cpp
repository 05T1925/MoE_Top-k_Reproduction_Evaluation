#include <moe_topk/protocol_iii_two_round_package.h>

#include <FSS/prng.h>

#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using namespace moe_topk;
void check(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}
template <typename F> void reject(F&& test, const char* message) {
  try { test(); } catch (const std::invalid_argument&) { return; }
  throw std::runtime_error(message);
}
std::uint32_t get_u32(const std::vector<std::uint8_t>& data, std::size_t offset) {
  return (std::uint32_t{data[offset]} << 24U) |
         (std::uint32_t{data[offset + 1U]} << 16U) |
         (std::uint32_t{data[offset + 2U]} << 8U) |
         data[offset + 3U];
}
void run(std::uint32_t n) {
  ProtocolIIITwoRoundConfig c;
  c.session = UINT64_C(0x5f060000) + n;
  c.fingerprint = UINT64_C(0x6610972000) + n;
  c.logical_n = n;
  c.padded_n = n <= 4U ? 4U : 8U;
  c.k = n;
  c.target_rank = n - 1U;
  c.comparison_bits = n <= 4U ? 35U : 36U;
  c.rank_bits = n <= 4U ? 2U : 3U;
  constexpr std::uint64_t material_id = UINT64_C(0x5511ac12000021);
  osuCrypto::PRNG dealer(osuCrypto::toBlock(c.session, c.fingerprint));
  auto material = protocol_iii_two_round_preprocess(c, dealer);
  for (std::uint8_t p = 0; p < 2U; ++p) {
    auto local = c;
    local.party = p;
    const auto& view = p == 0U ? material.first : material.second;
    const auto bytes = protocol_iii_two_round_serialize_bundle(
        local, view, material_id);
    auto decoded = protocol_iii_two_round_deserialize_bundle(
        local, material_id, bytes);
    check(decoded.material_id == material_id &&
              protocol_iii_two_round_serialize_bundle(
                  local, decoded.material, material_id) == bytes,
          "M5-F bundle roundtrip");
    auto wrong = local;
    wrong.party ^= 1U;
    reject([&] { (void)protocol_iii_two_round_deserialize_bundle(
        wrong, material_id, bytes); }, "wrong party accepted");
    wrong = local; ++wrong.session;
    reject([&] { (void)protocol_iii_two_round_deserialize_bundle(
        wrong, material_id, bytes); }, "wrong session accepted");
    wrong = local; ++wrong.fingerprint;
    reject([&] { (void)protocol_iii_two_round_deserialize_bundle(
        wrong, material_id, bytes); }, "wrong fingerprint accepted");
    wrong = local; ++wrong.logical_n;
    reject([&] { (void)protocol_iii_two_round_deserialize_bundle(
        wrong, material_id, bytes); }, "wrong logical_n accepted");
    wrong = local; ++wrong.padded_n;
    reject([&] { (void)protocol_iii_two_round_deserialize_bundle(
        wrong, material_id, bytes); }, "wrong padded_n accepted");
    wrong = local; wrong.target_rank = 0U;
    reject([&] { (void)protocol_iii_two_round_deserialize_bundle(
        wrong, material_id, bytes); }, "wrong target accepted");
    wrong = local; ++wrong.comparison_bits;
    reject([&] { (void)protocol_iii_two_round_deserialize_bundle(
        wrong, material_id, bytes); }, "wrong comparison width accepted");
    wrong = local; ++wrong.rank_bits;
    reject([&] { (void)protocol_iii_two_round_deserialize_bundle(
        wrong, material_id, bytes); }, "wrong rank width accepted");
    reject([&] { (void)protocol_iii_two_round_deserialize_bundle(
        local, material_id + 1U, bytes); }, "wrong material id accepted");
    for (std::size_t size : {std::size_t{0}, std::size_t{1},
                             std::size_t{55}, bytes.size() - 1U}) {
      auto damaged = bytes;
      damaged.resize(size);
      reject([&] { (void)protocol_iii_two_round_deserialize_bundle(
          local, material_id, damaged); }, "truncated bundle accepted");
    }
    {
      auto damaged = bytes; damaged.push_back(0U);
      reject([&] { (void)protocol_iii_two_round_deserialize_bundle(
          local, material_id, damaged); }, "extra bundle bytes accepted");
    }
    for (std::size_t offset : {std::size_t{0}, std::size_t{4},
                               std::size_t{5}, std::size_t{6},
                               std::size_t{7}}) {
      auto damaged = bytes; damaged[offset] ^= 1U;
      reject([&] { (void)protocol_iii_two_round_deserialize_bundle(
          local, material_id, damaged); }, "bad header accepted");
    }
    const auto package_size = get_u32(bytes, 52U);
    const auto rank_count_offset = 56U + package_size;
    const auto dpf_count_offset = rank_count_offset + 4U + n * 8U;
    for (const auto offset : {rank_count_offset, dpf_count_offset}) {
      auto damaged = bytes; damaged[offset + 3U] ^= 1U;
      reject([&] { (void)protocol_iii_two_round_deserialize_bundle(
          local, material_id, damaged); }, "wrong material count accepted");
    }
    const auto dpf_first_field_offset = dpf_count_offset + 4U;
    {
      auto damaged = bytes;
      for (std::size_t i = 0; i < 16U; ++i)
        damaged[dpf_first_field_offset + i] = UINT8_C(0xff);
      reject([&] { (void)protocol_iii_two_round_deserialize_bundle(
          local, material_id, damaged); }, "noncanonical field accepted");
    }
    // The first field DPF key is length-prefixed after one 16-byte mask.
    const auto key_length_offset = dpf_first_field_offset + 16U;
    {
      auto damaged = bytes; damaged[key_length_offset + 3U] ^= 1U;
      reject([&] { (void)protocol_iii_two_round_deserialize_bundle(
          local, material_id, damaged); }, "wrong DPF key length accepted");
    }
  }
}
}  // namespace
int main() {
  try {
    for (int i = 0; i < 256; ++i)
      FSSConfig::prngs[i].SetSeed(osuCrypto::toBlock(
          UINT64_C(0x5f060000), static_cast<std::uint64_t>(i)));
    run(3U);
    run(5U);
    std::cout << "M5-F offline bundle conformance PASS\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "M5-F offline bundle FAIL: " << e.what() << '\n';
    return 1;
  }
}
