#include <moe_topk/protocol_i_party_package.h>
#include <moe_topk/protocol_i_priority_key.h>
#include <moe_topk/protocol_i_score_input.h>
#include <FSS/prng.h>

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <random>
#include <stdexcept>
#include <thread>
#include <vector>
#include <sys/socket.h>

namespace {
using namespace moe_topk;
void require(bool value) { if (!value) throw std::runtime_error("M2.14 conformance"); }
void seed() { for (int i = 0; i < 256; ++i) FSSConfig::prngs[i].SetSeed(osuCrypto::toBlock(0x214, i)); }
ProtocolIPartyPackage package(int party) {
  seed(); ProtocolIPartyPackage result; result.session = 7; result.fingerprint = 8; result.party = party; result.n = 2; result.k = 1; result.comparison_bits = 34; result.node_mask_shares = {0, 0};
  ProtocolIUcmpMaterial edge(34, 0, 0); result.edge_materials.emplace_back(0, 1, edge.export_party_material(party));
  for (const auto stage : {UINT8_C(1), UINT8_C(2)}) for (std::uint32_t slot = 0; slot != 2; ++slot) { ProtocolIUcmpMaterial item(34, slot + 1, slot + 3); ProtocolIScoreInputPartyMaterial score(slot, stage, 0, 0, item.export_party_material(party)); if (stage == 1) result.carry_materials.push_back(std::move(score)); else result.sign_materials.push_back(std::move(score)); }
  return result;
}

void actual_adapter_case(const std::vector<std::int32_t>& scores, std::uint32_t k,
                         std::uint64_t seed_value) {
  const auto logical_n = static_cast<std::uint32_t>(scores.size());
  std::uint32_t padded_n = 2;
  while (padded_n < logical_n) padded_n <<= 1U;
  std::uint8_t index_bits = 0;
  for (auto value = padded_n - 1U; value != 0; value >>= 1U) ++index_bits;
  if (index_bits == 0) index_bits = 1;
  const auto comparison_bits = static_cast<std::uint8_t>(33U + index_bits);
  const auto score_mask = (UINT64_C(1) << 34U) - 1U;
  std::mt19937_64 rng(seed_value);

  std::array<ProtocolIPartyPackage, 2> packages;
  std::array<std::vector<std::uint32_t>, 2> raw_shares;
  for (int party = 0; party != 2; ++party) {
    auto& p = packages[party];
    p.session = 700 + logical_n;
    p.fingerprint = 800 + logical_n;
    p.party = party;
    p.n = padded_n;
    p.k = k;
    p.comparison_bits = comparison_bits;
  }
  for (std::uint32_t slot = 0; slot != padded_n; ++slot) {
    const auto left_mask = rng() & score_mask;
    const auto right_mask = rng() & score_mask;
    const auto left_share0 = rng() & score_mask;
    const auto right_share0 = rng() & score_mask;
    ProtocolIUcmpMaterial carry(34, left_mask, right_mask);
    packages[0].carry_materials.emplace_back(slot, 1, left_share0, right_share0,
                                              carry.export_party_material(0));
    packages[1].carry_materials.emplace_back(slot, 1,
        (left_mask - left_share0) & score_mask,
        (right_mask - right_share0) & score_mask,
        carry.export_party_material(1));

    const auto sign_left_share0 = rng() & score_mask;
    const auto sign_right_share0 = rng() & score_mask;
    ProtocolIUcmpMaterial sign(34, left_mask, right_mask);
    packages[0].sign_materials.emplace_back(slot, 2, sign_left_share0, sign_right_share0,
                                             sign.export_party_material(0));
    packages[1].sign_materials.emplace_back(slot, 2,
        (left_mask - sign_left_share0) & score_mask,
        (right_mask - sign_right_share0) & score_mask,
        sign.export_party_material(1));
  }

  for (std::size_t i = 0; i < scores.size(); ++i) {
    const auto share0 = static_cast<std::uint32_t>(rng());
    const auto word = static_cast<std::uint32_t>(scores[i]);
    raw_shares[0].push_back(share0);
    raw_shares[1].push_back(word - share0);
  }

  int carry_fds[2] = {-1, -1}, sign_fds[2] = {-1, -1};
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, carry_fds) == 0);
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, sign_fds) == 0);
  std::array<std::vector<std::uint64_t>, 2> output;
  std::array<std::exception_ptr, 2> errors{};
  std::array<ProtocolIScoreInputMetrics, 2> metrics{};
  std::array<std::thread, 2> parties;
  for (int party = 0; party != 2; ++party) {
    parties[party] = std::thread([&, party] {
      try {
        const ProtocolIScoreInputConfig config{
            700 + logical_n, 800 + logical_n, logical_n, padded_n, k,
            index_bits, comparison_bits, static_cast<std::uint8_t>(party), 5000};
        const std::array<int, 2> fds{party == 0 ? carry_fds[0] : carry_fds[1],
                                     party == 0 ? sign_fds[0] : sign_fds[1]};
        output[party] = protocol_i_raw_score_input_party(
            config, packages[party], raw_shares[party], fds, &metrics[party]);
      } catch (...) { errors[party] = std::current_exception(); }
    });
  }
  for (auto& party : parties) party.join();
  for (const auto& error : errors) if (error) std::rethrow_exception(error);

  const auto key_mask = (UINT64_C(1) << comparison_bits) - 1U;
  for (std::uint32_t i = 0; i < logical_n; ++i) {
    const auto reconstructed = (output[0][i] + output[1][i]) & key_mask;
    const auto expected = protocol_i_priority_key(static_cast<std::uint32_t>(scores[i]), i,
                                                   padded_n).value;
    require(reconstructed == expected);
  }
  require(metrics[0].ucmp_calls == 2ULL * padded_n &&
          metrics[1].ucmp_calls == 2ULL * padded_n);
}
}
int main() { try {
  const std::vector<std::uint32_t> raw{UINT32_C(0x80000000),UINT32_C(0x80000001),UINT32_MAX,0,1,UINT32_C(0x7ffffffe),UINT32_C(0x7fffffff)};
  for (const auto n : {UINT32_C(1),UINT32_C(2),UINT32_C(3),UINT32_C(5),UINT32_C(17),UINT32_C(31),UINT32_C(128)}) { std::uint32_t padded = 2; while (padded < n) padded <<= 1U; std::uint8_t bits = 0; for (auto value = padded - 1U; value != 0; value >>= 1U) ++bits; const auto mask = (UINT64_C(1) << (33U + bits)) - 1U; for (std::uint32_t index = 0; index < n; ++index) { const auto word = raw[index % raw.size()]; const auto q = static_cast<std::uint32_t>(INT32_MAX - word); require((((static_cast<std::uint64_t>(q) << bits) + index) & mask) == protocol_i_priority_key(word, index, padded).value); } }
  seed();
  for (const auto& scores : {std::vector<std::int32_t>{INT32_MIN},
                             std::vector<std::int32_t>{INT32_MIN, INT32_MAX},
                             std::vector<std::int32_t>{INT32_MAX, INT32_MIN, 0},
                             std::vector<std::int32_t>{INT32_MAX, INT32_MIN, 0, -1, INT32_MAX},
                             std::vector<std::int32_t>{INT32_MIN, INT32_MAX, 0, -1, INT32_MAX, 0, INT32_MIN, 17}}) {
    actual_adapter_case(scores, static_cast<std::uint32_t>((scores.size() + 1U) / 2U),
                        UINT64_C(0x534339) + scores.size());
  }
  auto valid = serialize_party_package(package(0)); require(deserialize_party_package(valid, 0).carry_materials.size() == 2); bool rejected = false; try { (void)deserialize_party_package(valid, 1); } catch (...) { rejected = true; } require(rejected); valid.push_back(0); rejected = false; try { (void)deserialize_party_package(valid, 0); } catch (...) { rejected = true; } require(rejected); return 0;
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; } }
