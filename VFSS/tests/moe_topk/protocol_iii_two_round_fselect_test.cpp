#include <moe_topk/protocol_i_priority_key.h>
#include <moe_topk/protocol_i_transport.h>
#include <moe_topk/protocol_i_ucmp.h>
#include <moe_topk/protocol_iii_secure_core.h>
#include <moe_topk/protocol_iii_field_payload.h>
#include <moe_topk/protocol_iii_two_round.h>
#include <moe_topk/topk_oracle.h>

#include <FSS/prng.h>

#include <algorithm>
#include <array>
#include <exception>
#include <thread>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace moe_topk;

void check(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}

template <typename F> void rejects(F&& run, const char* message) {
  try { run(); } catch (const std::invalid_argument&) { return; }
  throw std::runtime_error(message);
}

std::uint8_t width(std::uint32_t value) {
  std::uint8_t bits = 0;
  while (value != 0U) { ++bits; value >>= 1U; }
  return bits;
}

std::uint32_t padded(std::uint32_t n) {
  std::uint32_t size = 2;
  while (size < n) size <<= 1U;
  return size;
}

void seed_fss() {
  for (int i = 0; i < 256; ++i)
    FSSConfig::prngs[i].SetSeed(osuCrypto::toBlock(
        UINT64_C(0x4d35453300000000), static_cast<std::uint64_t>(i)));
}

struct FramedExchange {
  std::vector<std::uint8_t> to_p0, to_p1;
  std::uint64_t wire_per_party = 0;
};

FramedExchange exchange(const ProtocolIIITwoRoundConfig& c,
                        const std::vector<std::uint8_t>& from_p0,
                        const std::vector<std::uint8_t>& from_p1,
                        std::uint8_t round) {
  std::array<int, 2> fd{{-1, -1}};
  check(::socketpair(AF_UNIX, SOCK_STREAM, 0, fd.data()) == 0,
        "two-round socketpair");
  const ProtocolIFrameConfig frame0{c.session, c.fingerprint, c.logical_n, c.k,
                                    c.comparison_bits, 0U, 1U, 6U, round};
  const ProtocolIFrameConfig frame1{c.session, c.fingerprint, c.logical_n, c.k,
                                    c.comparison_bits, 1U, 0U, 6U, round};
  ProtocolIFramedChannel channel0(fd[0], frame0, 5000);
  ProtocolIFramedChannel channel1(fd[1], frame1, 5000);
  FramedExchange result;
  std::exception_ptr peer_error;
  std::thread peer([&] {
    try {
      // The complete outbound from_p1 was prepared before this receive.
      result.to_p1 = channel1.receive();
      channel1.send(from_p1);
    } catch (...) { peer_error = std::current_exception(); }
  });
  try {
    channel0.send(from_p0);
    result.to_p0 = channel0.receive();
  } catch (...) {
    peer.join();
    throw;
  }
  peer.join();
  if (peer_error) std::rethrow_exception(peer_error);
  check(result.to_p0 == from_p1 && result.to_p1 == from_p0,
        "framed two-round exchange changed payload");
  check(channel0.sent_bytes() == channel1.sent_bytes(),
        "asymmetric online wire size");
  result.wire_per_party = channel0.sent_bytes();
  return result;
}

struct TestCase {
  std::vector<std::uint32_t> scores;
  std::vector<std::uint64_t> payloads;
};

TestCase make_case(std::uint32_t n, std::uint32_t variant, std::mt19937_64& random) {
  TestCase c;
  c.scores.resize(n);
  c.payloads.resize(n);
  for (std::uint32_t i = 0; i < n; ++i) {
    switch (variant) {
      case 0: c.scores[i] = static_cast<std::uint32_t>((n - i) * 4096U); break;
      case 1: c.scores[i] = static_cast<std::uint32_t>(i * 4096U); break;
      case 2: c.scores[i] = 7U; break;
      case 3: c.scores[i] = (i % 3U == 0U) ? 10U : 20U; break;
      case 4: c.scores[i] = i % 2U == 0U ? 0U : UINT32_MAX; break;
      case 5: c.scores[i] = i % 3U == 0U ? UINT32_C(0x80000000) :
                              (i % 3U == 1U ? UINT32_C(0x7fffffff) : 0U); break;
      default: c.scores[i] = static_cast<std::uint32_t>(random()); break;
    }
    switch ((variant + i) % 5U) {
      case 0: c.payloads[i] = 0U; break;
      case 1: c.payloads[i] = 1U; break;
      case 2: c.payloads[i] = UINT64_MAX; break;
      case 3: c.payloads[i] = 7U; break;
      default: c.payloads[i] = random(); break;
    }
  }
  return c;
}

// One same-input execution of the frozen M3 three-round core. The new
// candidate and this baseline meet only in the test controller's mask view.
std::vector<std::uint8_t> old_m3_mask(const TestCase& test,
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

void run(const TestCase& test, std::uint32_t k, std::uint32_t target,
         std::uint64_t seed, bool misuse, bool compare_m3) {
  const auto n = static_cast<std::uint32_t>(test.scores.size());
  ProtocolIIITwoRoundConfig c;
  c.session = seed | 1U;
  c.fingerprint = seed ^ UINT64_C(0x847923aa426b6df1);
  c.logical_n = n;
  c.padded_n = padded(n);
  c.k = k;
  c.target_rank = target;
  c.comparison_bits = static_cast<std::uint8_t>(33U + width(c.padded_n - 1U));
  c.rank_bits = width(n - 1U);
  osuCrypto::PRNG dealer(osuCrypto::toBlock(seed, c.fingerprint));
  // Preprocessing returns only local views. The controller splits inputs.
  auto materials = protocol_iii_two_round_preprocess(c, dealer);
  const auto offline0 = protocol_iii_two_round_offline_material_bytes(materials.first);
  const auto offline1 = protocol_iii_two_round_offline_material_bytes(materials.second);
  check(offline0 > 0U && offline1 > 0U, "offline material size");
  std::mt19937_64 random(seed);
  std::vector<std::uint64_t> key0(c.padded_n), key1(c.padded_n);
  std::vector<ProtocolIIIField> payload0(n), payload1(n);
  const auto ring_mask = (UINT64_C(1) << c.comparison_bits) - 1U;
  for (std::uint32_t i = 0; i < c.padded_n; ++i) {
    const auto score = i < n ? test.scores[i] : UINT32_C(0x80000000);
    const auto key = protocol_i_priority_key(score, i, c.padded_n).value;
    key0[i] = random() & ring_mask;
    key1[i] = (key - key0[i]) & ring_mask;
    if (i < n) {
      const auto encoded = protocol_iii_pack_key_payload_nonzero(
          key, c.comparison_bits, test.payloads[i]);
      auto shares = protocol_iii_split_field_element(encoded, dealer);
      payload0[i] = shares.first;
      payload1[i] = shares.second;
    }
  }
  c.party = 0U;
  ProtocolIIITwoRoundParty p0(c, std::move(materials.first),
                              std::move(key0), std::move(payload0));
  c.party = 1U;
  ProtocolIIITwoRoundParty p1(c, std::move(materials.second),
                              std::move(key1), std::move(payload1));

  std::uint64_t r1_wire = 0;
  std::uint64_t r2_wire = 0;
  // Both complete R1 outbounds exist before either party reads peer R1.
  auto r10 = p0.prepare_round1();
  auto r11 = p1.prepare_round1();
  if (misuse) {
    rejects([&] { (void)p0.prepare_round1(); }, "R1 material replay accepted");
  }
  if (misuse) {
    const auto delivered = exchange(c, r10, r11, 1U);
    check(delivered.wire_per_party > r10.size(), "R1 frame overhead missing");
    r1_wire = delivered.wire_per_party;
    p0.consume_round1(delivered.to_p0);
    p1.consume_round1(delivered.to_p1);
  } else {
    p0.consume_round1(r11);
    p1.consume_round1(r10);
  }
  const auto expected_ranks = stable_ranks_cmpagg(test.scores);
  check(p0.rank_additive_shares().size() == n &&
            p1.rank_additive_shares().size() == n,
        "rank logical shape");
  const auto rank_mask = (UINT64_C(1) << c.rank_bits) - 1U;
  for (std::uint32_t i = 0; i < n; ++i) {
    const auto rank = (p0.rank_additive_shares()[i] +
                       p1.rank_additive_shares()[i]) & rank_mask;
    check(rank == expected_ranks[i], "shared CmpAgg rank differs from oracle");
  }

  // Both complete R2 outbounds exist before either party reads peer R2.
  auto r20 = p0.prepare_round2();
  auto r21 = p1.prepare_round2();
  if (misuse)
    rejects([&] { (void)p0.prepare_round2(); }, "R2 material replay accepted");
  ProtocolIIIField out0, out1;
  if (misuse) {
    const auto delivered = exchange(c, r20, r21, 2U);
    check(delivered.wire_per_party > r20.size(), "R2 frame overhead missing");
    r2_wire = delivered.wire_per_party;
    out0 = p0.consume_round2(delivered.to_p0);
    out1 = p1.consume_round2(delivered.to_p1);
  } else {
    out0 = p0.consume_round2(r21);
    out1 = p1.consume_round2(r20);
  }
  if (misuse)
    rejects([&] { (void)p0.consume_round2(r21); }, "final replay accepted");
  const auto selected = ProtocolIIIField::add(out0, out1);
  std::uint32_t expected_index = 0;
  while (expected_ranks[expected_index] != target) ++expected_index;
  const auto expected_key = protocol_i_priority_key(
      test.scores[expected_index], expected_index, c.padded_n).value;
  const auto decoded = protocol_iii_unpack_key_payload_nonzero(
      selected, c.comparison_bits);
  check(decoded.first == expected_key &&
            decoded.second == test.payloads[expected_index],
        "two-round selected field record differs from oracle");
  // Common membership view with the frozen M3 original-order Top-K mask.
  const auto old_semantics = top_k_mask(test.scores, k);
  check(old_semantics[expected_index] == 1U,
        "selected index outside frozen M3 Top-K membership");
  if (compare_m3) {
    const auto actual_m3 = old_m3_mask(test, k, seed + 2U);
    check(actual_m3 == old_semantics && actual_m3[expected_index] == 1U,
          "same-input M3/new-candidate membership mismatch");
    const auto r1_bits = static_cast<std::uint64_t>(2U) * n *
                         (c.comparison_bits + 2U * 127U);
    const auto r2_bits = static_cast<std::uint64_t>(2U) * n *
                         (c.rank_bits + 127U);
    if (misuse) std::cout << "M5E_SAMPLE n=" << n << " k=" << k
              << " R1_LOGICAL_BITS=" << r1_bits
              << " R2_LOGICAL_BITS=" << r2_bits
              << " R1_WIRE_BYTES=" << 2U * r1_wire
              << " R2_WIRE_BYTES=" << 2U * r2_wire
              << " OFFLINE_MATERIAL_BYTES=" << offline0 + offline1 << '\n';
  }
  check(r10.size() == 44U + static_cast<std::size_t>(n) * 40U &&
            r20.size() == 44U + static_cast<std::size_t>(n) * 24U,
        "message schema size");
}
void owned_transport_case(const TestCase& test, std::uint32_t k,
                          std::uint32_t target, std::uint64_t seed) {
  const auto n = static_cast<std::uint32_t>(test.scores.size());
  ProtocolIIITwoRoundConfig c;
  c.session = seed | 1U;
  c.fingerprint = seed ^ UINT64_C(0x530ac76d49211b15);
  c.logical_n = n;
  c.padded_n = padded(n);
  c.k = k;
  c.target_rank = target;
  c.comparison_bits = static_cast<std::uint8_t>(33U + width(c.padded_n - 1U));
  c.rank_bits = width(n - 1U);
  osuCrypto::PRNG dealer(osuCrypto::toBlock(seed, c.fingerprint));
  auto materials = protocol_iii_two_round_preprocess(c, dealer);
  std::array<std::vector<std::uint64_t>, 2> keys{
      std::vector<std::uint64_t>(c.padded_n),
      std::vector<std::uint64_t>(c.padded_n)};
  std::array<std::vector<ProtocolIIIField>, 2> payloads{
      std::vector<ProtocolIIIField>(n),
      std::vector<ProtocolIIIField>(n)};
  std::mt19937_64 random(seed);
  const auto key_mask = (UINT64_C(1) << c.comparison_bits) - 1U;
  for (std::uint32_t i = 0; i < c.padded_n; ++i) {
    const auto score = i < n ? test.scores[i] : UINT32_C(0x80000000);
    const auto key = protocol_i_priority_key(score, i, c.padded_n).value;
    keys[0][i] = random() & key_mask;
    keys[1][i] = (key - keys[0][i]) & key_mask;
    if (i < n) {
      const auto encoded = protocol_iii_pack_key_payload_nonzero(
          key, c.comparison_bits, test.payloads[i]);
      const auto shares = protocol_iii_split_field_element(encoded, dealer);
      payloads[0][i] = shares.first;
      payloads[1][i] = shares.second;
    }
  }
  std::array<std::array<int, 2>, 2> fd;
  for (auto& pair : fd)
    check(::socketpair(AF_UNIX, SOCK_STREAM, 0, pair.data()) == 0,
          "owned transport socketpair");
  std::array<ProtocolIIITwoRoundOutput, 2> out;
  std::array<std::exception_ptr, 2> error;
  std::array<std::thread, 2> workers;
  for (std::uint8_t party = 0; party < 2U; ++party) {
    workers[party] = std::thread([&, party] {
      try {
        auto local = c;
        local.party = party;
        auto& material = party == 0U ? materials.first : materials.second;
        const ProtocolIIITwoRoundFds local_fds{fd[0][party], fd[1][party]};
        out[party] = protocol_iii_two_round_party(
            local, std::move(material), std::move(keys[party]),
            std::move(payloads[party]), local_fds, 5000);
      } catch (...) { error[party] = std::current_exception(); }
    });
  }
  for (auto& worker : workers) worker.join();
  for (const auto& failure : error) if (failure) std::rethrow_exception(failure);
  const auto reconstructed = ProtocolIIIField::add(
      out[0].selected_share, out[1].selected_share);
  const auto ranks = stable_ranks_cmpagg(test.scores);
  std::uint32_t index = 0;
  while (ranks[index] != target) ++index;
  const auto expected_key = protocol_i_priority_key(
      test.scores[index], index, c.padded_n).value;
  check(protocol_iii_unpack_key_payload_nonzero(
            reconstructed, c.comparison_bits) ==
            std::make_pair(expected_key, test.payloads[index]),
        "owned two-round transport differs from oracle");
  for (const auto& party : out) {
    check(party.metrics.online_rounds == 2U &&
              party.metrics.round1_sent_bytes == 92U + 40U * n &&
              party.metrics.round2_sent_bytes == 92U + 24U * n &&
              party.metrics.round1_received_bytes == party.metrics.round1_sent_bytes &&
              party.metrics.round2_received_bytes == party.metrics.round2_sent_bytes,
          "owned transport rounds/wire accounting");
  }
}

void partial_receive_fails_closed() {
  ProtocolIIITwoRoundConfig c;
  c.session = UINT64_C(0x5e2500c1);
  c.fingerprint = UINT64_C(0x673ff612);
  c.logical_n = 3U;
  c.padded_n = 4U;
  c.k = 1U;
  c.target_rank = 0U;
  c.comparison_bits = 35U;
  c.rank_bits = 2U;
  c.party = 1U;
  osuCrypto::PRNG dealer(osuCrypto::toBlock(c.session, c.fingerprint));
  auto material = protocol_iii_two_round_preprocess(c, dealer);
  std::array<int, 2> r1, r2;
  check(::socketpair(AF_UNIX, SOCK_STREAM, 0, r1.data()) == 0 &&
            ::socketpair(AF_UNIX, SOCK_STREAM, 0, r2.data()) == 0,
        "partial receive socketpair");
  const std::array<std::uint8_t, 3> partial{{'M', '5', 'T'}};
  check(::write(r1[0], partial.data(), partial.size()) ==
            static_cast<ssize_t>(partial.size()),
        "partial receive test write");
  ::close(r1[0]);
  try {
    (void)protocol_iii_two_round_party(
        c, std::move(material.second), std::vector<std::uint64_t>(4U),
        std::vector<ProtocolIIIField>(3U), {r1[1], r2[1]}, 1000);
    throw std::runtime_error("partial R1 frame accepted");
  } catch (const std::runtime_error& error) {
    check(std::string(error.what()) != "partial R1 frame accepted",
          "partial R1 frame accepted");
  }
  ::close(r2[0]);
  rejects([&] {
    ProtocolIIITwoRoundParty retry(
        c, std::move(material.second), std::vector<std::uint64_t>(4U),
        std::vector<ProtocolIIIField>(3U));
  }, "partial R1 I/O left material reusable");
}

void malformed_and_lifecycle() {
  ProtocolIIITwoRoundConfig c;
  c.session = UINT64_C(0x5e25000051);
  c.fingerprint = UINT64_C(0x6f11000092);
  c.logical_n = 3U;
  c.padded_n = 4U;
  c.k = 2U;
  c.target_rank = 1U;
  c.comparison_bits = 35U;
  c.rank_bits = 2U;
  osuCrypto::PRNG dealer(osuCrypto::toBlock(c.session, c.fingerprint));
  {
    auto wrong = protocol_iii_two_round_preprocess(c, dealer);
    rejects([&] {
      ProtocolIIITwoRoundParty invalid(
          c, std::move(wrong.second), std::vector<std::uint64_t>(4U),
          std::vector<ProtocolIIIField>(3U));
    }, "party-swapped material accepted");
  }
  {
    auto wrong = protocol_iii_two_round_preprocess(c, dealer);
    auto cfg = c;
    cfg.target_rank = 0U;
    rejects([&] {
      ProtocolIIITwoRoundParty invalid(
          cfg, std::move(wrong.first), std::vector<std::uint64_t>(4U),
          std::vector<ProtocolIIIField>(3U));
    }, "wrong-target material accepted");
  }
  {
    auto wrong = protocol_iii_two_round_preprocess(c, dealer);
    auto cfg = c;
    ++cfg.session;
    rejects([&] {
      ProtocolIIITwoRoundParty invalid(
          cfg, std::move(wrong.first), std::vector<std::uint64_t>(4U),
          std::vector<ProtocolIIIField>(3U));
    }, "wrong-session material accepted");
  }
  {
    auto m = protocol_iii_two_round_preprocess(c, dealer);
    ProtocolIIITwoRoundParty p0(c, std::move(m.first),
                                std::vector<std::uint64_t>(4U),
                                std::vector<ProtocolIIIField>(3U));
    c.party = 1U;
    ProtocolIIITwoRoundParty p1(c, std::move(m.second),
                                std::vector<std::uint64_t>(4U),
                                std::vector<ProtocolIIIField>(3U));
    const auto r10 = p0.prepare_round1();
    auto r11 = p1.prepare_round1();
    r11[5] = 2U;
    rejects([&] { p0.consume_round1(r11); }, "wrong R1 phase accepted");
    r11[5] = 1U;
    rejects([&] { p0.consume_round1(r11); }, "R1 retried after failure");
    (void)r10;
  }
  c.party = 0U;
  for (const auto byte : {0U, 4U, 6U, 8U, 16U, 24U, 28U,
                          32U, 36U, 40U, 42U}) {
    auto m = protocol_iii_two_round_preprocess(c, dealer);
    ProtocolIIITwoRoundParty p0(c, std::move(m.first),
                                std::vector<std::uint64_t>(4U),
                                std::vector<ProtocolIIIField>(3U));
    c.party = 1U;
    ProtocolIIITwoRoundParty p1(c, std::move(m.second),
                                std::vector<std::uint64_t>(4U),
                                std::vector<ProtocolIIIField>(3U));
    (void)p0.prepare_round1();
    auto peer = p1.prepare_round1();
    peer[byte] ^= 1U;
    rejects([&] { p0.consume_round1(peer); },
            "malformed R1 header accepted");
    c.party = 0U;
  }
  for (const auto resize_delta : {-1, 1}) {
    auto m = protocol_iii_two_round_preprocess(c, dealer);
    ProtocolIIITwoRoundParty p0(c, std::move(m.first),
                                std::vector<std::uint64_t>(4U),
                                std::vector<ProtocolIIIField>(3U));
    c.party = 1U;
    ProtocolIIITwoRoundParty p1(c, std::move(m.second),
                                std::vector<std::uint64_t>(4U),
                                std::vector<ProtocolIIIField>(3U));
    (void)p0.prepare_round1();
    auto peer = p1.prepare_round1();
    peer.resize(peer.size() + resize_delta);
    rejects([&] { p0.consume_round1(peer); },
            "wrong-size R1 message accepted");
    c.party = 0U;
  }
  c.party = 0U;
  {
    auto m = protocol_iii_two_round_preprocess(c, dealer);
    ProtocolIIITwoRoundParty p0(c, std::move(m.first),
                                std::vector<std::uint64_t>(4U),
                                std::vector<ProtocolIIIField>(3U));
    c.party = 1U;
    ProtocolIIITwoRoundParty p1(c, std::move(m.second),
                                std::vector<std::uint64_t>(4U),
                                std::vector<ProtocolIIIField>(3U));
    const auto r10 = p0.prepare_round1();
    const auto r11 = p1.prepare_round1();
    p0.consume_round1(r11);
    p1.consume_round1(r10);
    const auto r20 = p0.prepare_round2();
    auto r21 = p1.prepare_round2();
    const auto field_offset = 44U + 3U * 8U;
    std::fill(r21.begin() + field_offset,
              r21.begin() + field_offset + 16U, UINT8_C(0xff));
    rejects([&] { (void)p0.consume_round2(r21); },
            "noncanonical R2 field share accepted");
    rejects([&] { (void)p0.consume_round2(r20); },
            "R2 retried after failure");
  }
}
}  // namespace

int main() {
  try {
    seed_fss();
    malformed_and_lifecycle();
    partial_receive_fails_closed();
    std::mt19937_64 random(UINT64_C(0x5e259932c019));
    owned_transport_case(make_case(3U, 2U, random), 1U, 0U,
                         UINT64_C(0x5e250031));
    owned_transport_case(make_case(5U, 3U, random), 2U, 1U,
                         UINT64_C(0x5e250052));
    owned_transport_case(make_case(8U, 4U, random), 8U, 7U,
                         UINT64_C(0x5e250088));
    std::uint64_t cases = 0;
    for (const std::uint32_t n : {2U, 3U, 4U, 5U, 8U}) {
      for (std::uint32_t variant = 0; variant < 9U; ++variant) {
        const auto c = make_case(n, variant, random);
        std::vector<std::uint32_t> ks{1U, n, std::max(1U, n / 2U)};
        std::sort(ks.begin(), ks.end());
        ks.erase(std::unique(ks.begin(), ks.end()), ks.end());
        for (const auto k : ks) {
          const auto target = variant % k;
          const bool direct_m3 = cases == 0U ||
              (n == 3U && variant == 2U && k == 1U) ||
              (n == 5U && variant == 3U && k == 2U);
          run(c, k, target, UINT64_C(0x9cb5d0100000) + cases * 4U,
              cases == 0U, direct_m3);
          ++cases;
        }
      }
    }
    std::cout << "M5-E two-round Fselect: " << cases << " cases PASS\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "M5-E two-round Fselect FAIL: " << error.what() << '\n';
    return 1;
  }
}
