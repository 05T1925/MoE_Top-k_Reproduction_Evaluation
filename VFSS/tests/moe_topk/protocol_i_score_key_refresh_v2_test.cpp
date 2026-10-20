#include <moe_topk/protocol_i_project_derived_select5.h>
#include <moe_topk/protocol_i_score_input.h>

#include <FSS/prng.h>

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
using namespace moe_topk;

void check(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}

template <typename F>
void rejects(F&& f, const char* message) {
  bool rejected = false;
  try { f(); } catch (const std::exception&) { rejected = true; }
  check(rejected, message);
}

std::uint64_t mask(unsigned bits) {
  return (UINT64_C(1) << bits) - 1U;
}

std::uint64_t priority_key(std::uint32_t raw, std::uint32_t index,
                           unsigned index_bits) {
  const auto signed_score = static_cast<std::int32_t>(raw);
  const auto high = (INT64_C(0x7fffffff) -
                     static_cast<std::int64_t>(signed_score)) &
                    ((INT64_C(1) << 33U) - 1U);
  return ((static_cast<std::uint64_t>(high) << index_bits) | index) &
         mask(33U + index_bits);
}

void seed_test_prngs() {
  for (int i = 0; i < 256; ++i)
    FSSConfig::prngs[i].SetSeed(osuCrypto::toBlock(
        UINT64_C(0x5332345245465245) + static_cast<std::uint64_t>(i),
        UINT64_C(0x4653434f52454356) ^ static_cast<std::uint64_t>(i)));
}

void exhaustive_ring_and_stability_checks() {
  const std::vector<std::uint32_t> scores{
      UINT32_C(0x80000000), UINT32_C(0x7fffffff), UINT32_MAX, 0,
      UINT32_C(0x80000001), UINT32_C(0x7ffffffe), 0, UINT32_MAX};
  for (unsigned h = 1; h <= 8; ++h) {
    const unsigned bits = 33U + h;
    const auto ring = mask(bits);
    const auto indices = UINT32_C(1) << h;
    std::vector<std::uint64_t> q0(indices), q1(indices), rho0(indices), rho1(indices);
    for (std::uint32_t i = 0; i < indices; ++i) {
      const auto score = i < scores.size() ? scores[i]
          : static_cast<std::uint32_t>((i % 7U) * 4096U);
      const auto stable = priority_key(score, i, h);
      const auto high_share = (UINT64_C(0x9e3779b97f4a7c15) * (i + 1U)) &
                              ((UINT64_C(1) << 33U) - 1U);
      q0[i] = ((high_share << h) | i) & ring;
      q1[i] = (stable - q0[i]) & ring;
      check((q0[i] & ((UINT64_C(1) << h) - 1U)) == i,
            "v1 P0 residue is original index");
      check((q1[i] & ((UINT64_C(1) << h) - 1U)) == 0,
            "v1 P1 residue is zero");
      check(((q0[i] >> h) + (q1[i] >> h)) %
                (UINT64_C(1) << 33U) == (stable >> h),
            "v1 local high parts recompose because low residues do not carry");

      switch (i % 4U) {
        case 0: rho0[i] = 0; break;
        case 1: rho0[i] = 1; break;
        case 2: rho0[i] = ring; break;
        default: rho0[i] = ring - 1U; break;
      }
      rho1[i] = (UINT64_C(0) - rho0[i]) & ring;
    }
    protocol_i_refresh_full_priority_key_shares_v2(q0, rho0,
                                                    static_cast<std::uint8_t>(bits));
    protocol_i_refresh_full_priority_key_shares_v2(q1, rho1,
                                                    static_cast<std::uint8_t>(bits));
    for (std::uint32_t i = 0; i < indices; ++i) {
      const auto score = i < scores.size() ? scores[i]
          : static_cast<std::uint32_t>((i % 7U) * 4096U);
      check(((q0[i] + q1[i]) & ring) == priority_key(score, i, h),
            "zero-sum refresh preserves full stable key across ring wrap");
    }
  }
}

void local_shift_counterexample() {
  constexpr unsigned h = 1;
  constexpr unsigned bits = 34;
  const auto ring = mask(bits);
  const std::uint64_t q0 = 0;
  const std::uint64_t q1 = 0;
  const std::uint64_t rho0 = 1;
  const std::uint64_t rho1 = ring;
  const auto refreshed0 = (q0 + rho0) & ring;
  const auto refreshed1 = (q1 + rho1) & ring;
  check(((refreshed0 + refreshed1) & ring) == 0,
        "full-key shares retain zero sum at R34 boundary");
  check((((refreshed0 >> h) + (refreshed1 >> h)) &
         ((UINT64_C(1) << 33U) - 1U)) == ((UINT64_C(1) << 33U) - 1U),
        "independent local q extraction misses the low-bit carry");
}

void malformed_refresh_rejected() {
  std::vector<std::uint64_t> keys{0, 1};
  rejects([&] {
    protocol_i_refresh_full_priority_key_shares_v2(keys, {0}, 34);
  }, "refresh vector length mismatch rejected");
  rejects([&] {
    protocol_i_refresh_full_priority_key_shares_v2(keys,
        {0, UINT64_C(1) << 34U}, 34);
  }, "out-of-ring refresh share rejected");
  rejects([&] {
    protocol_i_refresh_full_priority_key_shares_v2(keys, {0, 0}, 0);
  }, "invalid key width rejected");
}

void versioned_package_roundtrip_and_rejection() {
  ProtocolIProjectDerivedSelect5DealerConfig config{
      801, 802, 803, 1, 1, 1000};
  config.dcf_abi = ProtocolIDcfAbi::Bcg21Fig1Aes128V1;
  const auto v1 = protocol_i_project_derived_select5_dealer_generate(config);
  check(v1.party0_package[4] == 3 && v1.party1_package[4] == 3,
        "unrefreshed BCG21 remains M6B5/v3");
  config.score_share_abi =
      ProtocolIScoreShareAbi::FullKeyZeroSumRefreshV2;
  const auto generated = protocol_i_project_derived_select5_dealer_generate(config);
  check(generated.party0_package[4] == 4 &&
            generated.party1_package[4] == 4,
        "full-key refresh uses M6B5/v4 package schema");
  check(generated.party0_package.size() == v1.party0_package.size() + 24U &&
            generated.party1_package.size() == v1.party1_package.size() + 24U,
        "padded N=2 package adds one count word plus two 64-bit refresh slots");
  auto p0 = protocol_i_project_derived_select5_deserialize_party_material(
      generated.party0_package, 0);
  auto p1 = protocol_i_project_derived_select5_deserialize_party_material(
      generated.party1_package, 1);
  rejects([&] {
    (void)protocol_i_project_derived_select5_deserialize_party_material(
        generated.party0_package, 1);
  }, "v2 party-0 package rejected under party-1 identity");
  check(p0.score_key_refresh_shares.size() == p0.padded_n &&
            p1.score_key_refresh_shares.size() == p1.padded_n,
        "v4 package contains one refresh share per padded record");
  const auto ring = mask(p0.comparison_bits);
  for (std::size_t i = 0; i < p0.score_key_refresh_shares.size(); ++i)
    check(((p0.score_key_refresh_shares[i] +
            p1.score_key_refresh_shares[i]) & ring) == 0,
          "dealer refresh shares are zero-sum per record");

  p0.score_key_refresh_shares[0] = UINT64_C(1) << p0.comparison_bits;
  rejects([&] {
    (void)protocol_i_project_derived_select5_serialize_party_material(p0);
  }, "out-of-ring refresh rejected by package serializer");
  auto missing_refresh =
      protocol_i_project_derived_select5_deserialize_party_material(
          generated.party0_package, 0);
  missing_refresh.score_key_refresh_shares.pop_back();
  rejects([&] {
    (void)protocol_i_project_derived_select5_serialize_party_material(
        missing_refresh);
  }, "missing per-record refresh slot rejected by serializer");
  auto truncated = generated.party0_package;
  truncated.pop_back();
  rejects([&] {
    (void)protocol_i_project_derived_select5_deserialize_party_material(
        truncated, 0);
  }, "truncated v4 refresh/package rejected");
  auto wrong_version = generated.party0_package;
  wrong_version[4] = 3;
  rejects([&] {
    (void)protocol_i_project_derived_select5_deserialize_party_material(
        wrong_version, 0);
  }, "v2 share payload cannot be parsed as frozen v3 package");
}

}  // namespace

int main() {
  try {
    seed_test_prngs();
    exhaustive_ring_and_stability_checks();
    local_shift_counterexample();
    malformed_refresh_rejected();
    versioned_package_roundtrip_and_rejection();
    std::cout << "F_SCORECONV_V2_REFRESH_CONFORMANCE_PASS\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "F_SCORECONV_V2_REFRESH_CONFORMANCE_FAIL: "
              << error.what() << '\n';
    return 1;
  }
}
