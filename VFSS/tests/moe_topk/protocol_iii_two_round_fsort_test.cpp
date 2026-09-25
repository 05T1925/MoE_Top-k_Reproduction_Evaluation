#include <moe_topk/protocol_i_priority_key.h>
#include <moe_topk/protocol_i_ucmp.h>
#include <moe_topk/protocol_iii_secure_core.h>
#include <FSS/dpf.h>
#include <moe_topk/protocol_iii_field_payload.h>
#include <moe_topk/protocol_iii_two_round.h>
#include <moe_topk/topk_oracle.h>

#include <FSS/prng.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <exception>
#include <thread>
#include <sys/socket.h>
#include <unistd.h>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>

namespace {
using namespace moe_topk;
using Field = ProtocolIIIField;

void check(bool ok, const char* reason) {
  if (!ok) throw std::runtime_error(reason);
}
template <class F> void rejects(F&& run, const char* reason) {
  try { run(); } catch (const std::invalid_argument&) { return; }
  throw std::runtime_error(reason);
}
std::uint8_t width(std::uint32_t v) {
  std::uint8_t result = 0;
  while (v != 0U) { ++result; v >>= 1U; }
  return result;
}
std::uint32_t padded(std::uint32_t n) {
  std::uint32_t result = 2U;
  while (result < n) result <<= 1U;
  return result;
}
struct Case {
  std::vector<std::uint32_t> scores;
  std::vector<std::uint64_t> payloads;
};
Case make_case(std::uint32_t n, std::uint32_t variant,
               std::mt19937_64& random) {
  Case out;
  out.scores.resize(n);
  out.payloads.resize(n);
  for (std::uint32_t i = 0; i < n; ++i) {
    switch (variant) {
      case 0U: out.scores[i] = (n - i) * 4096U; break;
      case 1U: out.scores[i] = i * 4096U; break;
      case 2U: out.scores[i] = 7U; break;
      case 3U: out.scores[i] = i % 3U == 0U ? 10U : 20U; break;
      case 4U: out.scores[i] = i % 2U == 0U ? 0U : UINT32_MAX; break;
      case 5U: out.scores[i] = i % 3U == 0U ? UINT32_C(0x80000000) :
                        (i % 3U == 1U ? UINT32_C(0x7fffffff) : 0U); break;
      default: out.scores[i] = static_cast<std::uint32_t>(random()); break;
    }
    switch ((i + variant) % 5U) {
      case 0U: out.payloads[i] = 0U; break;
      case 1U: out.payloads[i] = 1U; break;
      case 2U: out.payloads[i] = UINT64_MAX; break;
      case 3U: out.payloads[i] = 9U; break;
      default: out.payloads[i] = random(); break;
    }
  }
  return out;
}

struct Result {
  std::vector<Field> values;
  std::vector<std::uint64_t> ranks;
};
Result execute(const Case& data, std::uint32_t target,
               std::uint64_t seed, bool sort_mode, bool misuse = false,
               bool malformed_r2 = false) {
  const auto n = static_cast<std::uint32_t>(data.scores.size());
  ProtocolIIITwoRoundConfig cfg;
  cfg.session = seed | 1U;
  cfg.fingerprint = seed ^ UINT64_C(0x56d17eaa01553005);
  cfg.logical_n = n;
  cfg.padded_n = padded(n);
  cfg.k = n;
  cfg.target_rank = sort_mode ? 0U : target;
  cfg.comparison_bits = static_cast<std::uint8_t>(33U + width(cfg.padded_n - 1U));
  cfg.rank_bits = width(n - 1U);
  osuCrypto::PRNG dealer(osuCrypto::toBlock(seed, cfg.fingerprint));
  auto materials = protocol_iii_two_round_preprocess(cfg, dealer);
  std::mt19937_64 random(seed);
  std::array<std::vector<std::uint64_t>, 2> keys;
  std::array<std::vector<Field>, 2> payloads;
  for (auto& part : keys) part.resize(cfg.padded_n);
  for (auto& part : payloads) part.resize(n);
  const auto key_mask = (UINT64_C(1) << cfg.comparison_bits) - 1U;
  const auto rank_mask = (UINT64_C(1) << cfg.rank_bits) - 1U;
  const auto clear_ranks = stable_ranks_cmpagg(data.scores);
  for (std::uint32_t i = 0; i < cfg.padded_n; ++i) {
    const auto score = i < n ? data.scores[i] : UINT32_C(0x80000000);
    const auto key = protocol_i_priority_key(score, i, cfg.padded_n).value;
    keys[0][i] = random() & key_mask;
    keys[1][i] = (key - keys[0][i]) & key_mask;
    if (i < n) {
      const auto encoded = protocol_iii_pack_key_payload_nonzero(
          key, cfg.comparison_bits, data.payloads[i]);
      const auto shares = protocol_iii_split_field_element(encoded, dealer);
      payloads[0][i] = shares.first;
      payloads[1][i] = shares.second;
    }
  }
  if (sort_mode && (n == 3U || n == 5U)) {
    for (std::uint32_t i = 0; i < n; ++i) {
      const auto r_rank = (materials.first.rank_mask_shares[i] +
                           materials.second.rank_mask_shares[i]) & rank_mask;
      const auto m = (clear_ranks[i] + r_rank) & rank_mask;
      const auto full0 = protocol_iii_field_dpf_full_eval(
          materials.first.payload_material[i].dpf_key, 0U,
          cfg.session, cfg.fingerprint, i, cfg.rank_bits);
      const auto full1 = protocol_iii_field_dpf_full_eval(
          materials.second.payload_material[i].dpf_key, 1U,
          cfg.session, cfg.fingerprint, i, cfg.rank_bits);
      for (std::uint32_t invalid = n; invalid <= rank_mask; ++invalid) {
        const auto x = (m - invalid) & rank_mask;
        check(Field::add(full0[x], full1[x]).is_zero(),
              "padded-domain target is nonzero");
      }
    }
  }
  cfg.party = 0U;
  ProtocolIIITwoRoundParty p0(cfg, std::move(materials.first),
                              std::move(keys[0]), std::move(payloads[0]));
  cfg.party = 1U;
  ProtocolIIITwoRoundParty p1(cfg, std::move(materials.second),
                              std::move(keys[1]), std::move(payloads[1]));
  const auto r10 = p0.prepare_round1();
  const auto r11 = p1.prepare_round1();
  p0.consume_round1(r11);
  p1.consume_round1(r10);
  check(p0.rank_additive_shares().size() == n &&
            p1.rank_additive_shares().size() == n, "single CmpAgg rank shape");
  for (std::uint32_t i = 0; i < n; ++i)
    check(((p0.rank_additive_shares()[i] + p1.rank_additive_shares()[i]) &
            rank_mask) == clear_ranks[i], "shared CmpAgg rank oracle");
  const auto r20 = p0.prepare_round2();
  const auto r21 = p1.prepare_round2();
  if (malformed_r2) {
    auto broken = r21;
    broken.pop_back();
    rejects([&] { (void)p0.consume_round2_sort(broken); },
            "Fsort malformed R2 accepted");
    rejects([&] { (void)p0.consume_round2_sort(r21); },
            "Fsort failed material retried");
    return {};
  }
  Result result;
  result.ranks = clear_ranks;
  if (sort_mode) {
    auto out0 = p0.consume_round2_sort(r21);
    auto out1 = p1.consume_round2_sort(r20);
    check(out0.size() == n && out1.size() == n,
          "Fsort leaked padded output slots");
    for (std::uint32_t t = 0; t < n; ++t)
      result.values.push_back(Field::add(out0[t], out1[t]));
    if (misuse) {
      rejects([&] { (void)p0.consume_round2_sort(r21); },
              "Fsort success material reused");
      rejects([&] { (void)p0.consume_round2(r21); },
              "Fsort material reused as Fselect");
    }
  } else {
    const auto out0 = p0.consume_round2(r21);
    const auto out1 = p1.consume_round2(r20);
    result.values.push_back(Field::add(out0, out1));
  }
  return result;
}

std::vector<std::uint8_t> old_m3_mask(const Case& test,
                                       std::uint32_t k, std::uint64_t seed) {
  const auto n = static_cast<std::uint32_t>(test.scores.size());
  const auto padded_n = padded(n);
  const auto comparison_bits = static_cast<std::uint8_t>(33U + width(padded_n - 1U));
  const auto rank_bits = width(n - 1U);
  const auto key_mask = (UINT64_C(1) << comparison_bits) - 1U;
  const auto rank_mask = (UINT64_C(1) << rank_bits) - 1U;
  const auto session = seed | 1U;
  const auto fingerprint = seed ^ UINT64_C(0x72891a4ecfa95352);
  std::mt19937_64 random(seed);
  std::array<ProtocolIIISecureCoreMaterial, 2> material;
  std::array<std::vector<std::uint64_t>, 2> keys;
  for (std::uint8_t party = 0; party < 2U; ++party) {
    auto& m = material[party];
    auto& g = m.grank_package;
    g.session = session; g.fingerprint = fingerprint; g.party = party;
    g.n = n; g.k = k; g.comparison_bits = comparison_bits;
    auto& r = m.routing_material;
    r.session = session; r.fingerprint = fingerprint; r.party = party;
    r.logical_n = n; r.k = k; r.rank_bits = rank_bits;
    auto& c = m.combine_material;
    c.session = session; c.fingerprint = fingerprint; c.party = party;
    c.logical_n = n; c.k = k;
    keys[party].resize(padded_n);
    m.unit_payload_shares.resize(n);
  }
  for (std::uint32_t i = 0; i < padded_n; ++i) {
    const auto score = i < n ? test.scores[i] : UINT32_C(0x80000000);
    const auto key = protocol_i_priority_key(score, i, padded_n).value;
    keys[0][i] = random() & key_mask;
    keys[1][i] = (key - keys[0][i]) & key_mask;
  }
  std::vector<std::uint64_t> cmp_masks(n);
  for (std::uint32_t i = 0; i < n; ++i) {
    cmp_masks[i] = random() & key_mask;
    const auto cmp0 = random() & key_mask;
    material[0].grank_package.node_mask_shares.push_back(cmp0);
    material[1].grank_package.node_mask_shares.push_back(
        (cmp_masks[i] - cmp0) & key_mask);
    const auto r_rank = random() & rank_mask;
    const auto rank0 = random() & rank_mask;
    material[0].routing_material.rank_mask_shares.push_back(rank0);
    material[1].routing_material.rank_mask_shares.push_back(
        (r_rank - rank0) & rank_mask);
    auto dpf = keyGenDPF(rank_bits, 64, r_rank, 1);
    material[0].routing_material.dpf_keys.emplace_back(std::move(dpf.first));
    material[1].routing_material.dpf_keys.emplace_back(std::move(dpf.second));
    material[0].unit_payload_shares[i] = random();
    material[1].unit_payload_shares[i] = 1U - material[0].unit_payload_shares[i];
  }
  for (std::uint32_t left = 0; left < n; ++left) {
    for (std::uint32_t right = left + 1U; right < n; ++right) {
      ProtocolIUcmpMaterial edge(comparison_bits, cmp_masks[left], cmp_masks[right]);
      material[0].grank_package.edge_materials.emplace_back(
          left, right, edge.export_party_material(0));
      material[1].grank_package.edge_materials.emplace_back(
          left, right, edge.export_party_material(1));
    }
  }
  for (std::size_t cell = 0; cell < static_cast<std::size_t>(n) * k; ++cell) {
    auto mul = generate_masked_mul_material(random(), random(), random());
    material[0].combine_material.multiplication_materials.push_back(
        std::move(mul.party0));
    material[1].combine_material.multiplication_materials.push_back(
        std::move(mul.party1));
  }
  std::array<std::array<int, 2>, 3> fds;
  for (auto& pair : fds)
    check(::socketpair(AF_UNIX, SOCK_STREAM, 0, pair.data()) == 0,
          "old M3 comparison socketpair");
  std::array<ProtocolIIISecureCoreOutput, 2> result;
  std::array<std::exception_ptr, 2> error;
  std::array<std::thread, 2> workers;
  for (std::uint8_t party = 0; party < 2U; ++party) {
    workers[party] = std::thread([&, party] {
      try {
        ProtocolIIISecureCoreConfig cfg;
        cfg.grank = {session, fingerprint, n, padded_n, k,
                     comparison_bits, rank_bits, party, 5000};
        cfg.routing = {session, fingerprint, n, k,
                       rank_bits, comparison_bits, party, 5000};
        cfg.combine = {session, fingerprint, n, k,
                       comparison_bits, party, 5000};
        const ProtocolIIISecureCoreFds local{fds[0][party], fds[1][party],
                                              fds[2][party]};
        result[party] = protocol_iii_secure_core_party(
            cfg, material[party], keys[party], local);
      } catch (...) { error[party] = std::current_exception(); }
    });
  }
  for (auto& worker : workers) worker.join();
  for (const auto& failure : error) if (failure) std::rethrow_exception(failure);
  check(result[0].metrics.online_rounds == 3U &&
            result[1].metrics.online_rounds == 3U,
        "frozen M3 causal baseline changed");
  std::vector<std::uint8_t> output(n);
  for (std::uint32_t i = 0; i < n; ++i)
    output[i] = result[0].xor_mask_shares[i] ^
                result[1].xor_mask_shares[i];
  return output;
}

void check_sort(const Case& data, std::uint64_t seed, bool deep) {
  const auto n = static_cast<std::uint32_t>(data.scores.size());
  const auto sorted = execute(data, 0U, seed, true, deep);
  std::vector<bool> seen(n, false);
  for (std::uint32_t target = 0; target < n; ++target) {
    std::uint32_t original = 0;
    while (original < n && sorted.ranks[original] != target) ++original;
    check(original < n && !seen[original], "sort is not a permutation");
    seen[original] = true;
    const auto unpacked = protocol_iii_unpack_key_payload_nonzero(
        sorted.values[target],
        static_cast<std::uint8_t>(33U + width(padded(n) - 1U)));
    check(unpacked.first == protocol_i_priority_key(
              data.scores[original], original, padded(n)).value &&
          unpacked.second == data.payloads[original],
          "Fsort field record differs from rank-order oracle");
  }
  check(std::all_of(seen.begin(), seen.end(), [](bool x) { return x; }),
        "Fsort lost an input record");
  for (const auto k : {1U, n / 2U, n}) {
    const auto expected = top_k_mask(data.scores, k);
    std::vector<std::uint8_t> from_sort(n, 0U);
    for (std::uint32_t t = 0; t < k; ++t) {
      for (std::uint32_t i = 0; i < n; ++i)
        if (sorted.ranks[i] == t) from_sort[i] = 1U;
    }
    check(from_sort == expected, "Fsort prefix/M3 mask semantics");
  }
  if (deep) {
    const auto k = std::max(1U, n / 2U);
    const auto expected_m3 = top_k_mask(data.scores, k);
    const auto actual_m3 = old_m3_mask(data, k, seed + UINT64_C(0x600000));
    check(actual_m3 == expected_m3, "frozen M3 secure mask changed");
    std::vector<std::uint8_t> prefix_mask(n, 0U);
    for (std::uint32_t t = 0; t < k; ++t)
      for (std::uint32_t i = 0; i < n; ++i)
        if (sorted.ranks[i] == t) prefix_mask[i] = 1U;
    check(prefix_mask == actual_m3, "Fsort prefix vs real M3 membership");
    for (const auto target : {0U, n / 2U, n - 1U}) {
      const auto selected = execute(data, target,
                                    seed + target + UINT64_C(0x4f1000), false);
      check(selected.values[0] == sorted.values[target],
            "Fsort slot differs from frozen Fselect");
    }
  }
}

void seed_fss() {
  for (int i = 0; i < 256; ++i)
    FSSConfig::prngs[i].SetSeed(osuCrypto::toBlock(
        UINT64_C(0x5f071000), static_cast<std::uint64_t>(i)));
}
}  // namespace

int main() {
  try {
    seed_fss();
    std::mt19937_64 random(UINT64_C(0x5f071abc));
    std::uint64_t seed = UINT64_C(0x5f0710000000);
    std::size_t cases = 0;
    for (const auto n : {2U,3U,4U,5U,8U}) {
      for (std::uint32_t variant = 0; variant <= 7U; ++variant) {
        const auto data = make_case(n,variant,random);
        check_sort(data,++seed,(variant == 2U || variant == 3U));
        ++cases;
      }
    }
    (void)execute(make_case(3U,2U,random),0U,++seed,true,false,true);
    std::cout << "M5-G FSORT_DIFFERENTIAL_PASS=" << cases << '\n';
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "M5-G Fsort differential FAIL: " << e.what() << '\n';
    return 1;
  }
}
