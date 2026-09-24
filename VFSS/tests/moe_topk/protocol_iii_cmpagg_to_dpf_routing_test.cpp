#include <moe_topk/protocol_i_priority_key.h>
#include <moe_topk/protocol_iii_dpf_routing.h>
#include <moe_topk/protocol_iii_grank.h>
#ifdef MOE_TOPK_M5C_FULL_CHAIN
#include <moe_topk/protocol_iii_secure_combine.h>
#include <moe_topk/protocol_iii_secure_core.h>
#endif
#include <moe_topk/topk_oracle.h>

#include <FSS/prng.h>

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include <sys/socket.h>
#include <unistd.h>

namespace {

using namespace moe_topk;

void check(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}

template <typename Function>
void rejects(Function&& run, const char* message) {
  try {
    run();
  } catch (const std::invalid_argument&) {
    return;
  }
  throw std::runtime_error(message);
}

std::uint8_t width(std::uint32_t value) {
  std::uint8_t bits = 0;
  for (; value != 0; value >>= 1U) ++bits;
  return bits;
}

std::uint64_t mask(std::uint8_t bits) {
  return (UINT64_C(1) << bits) - 1U;
}

std::uint32_t padded_size(std::uint32_t n) {
  std::uint32_t padded = 2;
  while (padded < n) padded <<= 1U;
  return padded;
}

void seed_fss(std::uint64_t seed) {
  for (int i = 0; i < 256; ++i)
    FSSConfig::prngs[i].SetSeed(osuCrypto::toBlock(seed, static_cast<std::uint64_t>(i)));
}

class SocketPair {
 public:
  SocketPair() {
    check(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds_.data()) == 0, "socketpair");
  }
  SocketPair(const SocketPair&) = delete;
  SocketPair& operator=(const SocketPair&) = delete;
  ~SocketPair() {
    for (int fd : fds_) if (fd >= 0) ::close(fd);
  }
  int duplicate(std::size_t party) const {
    const int fd = ::dup(fds_[party]);
    check(fd >= 0, "socket duplicate");
    return fd;
  }
 private:
  std::array<int, 2> fds_{{-1, -1}};
};

struct Case {
  std::vector<std::uint32_t> scores;
  std::uint32_t k;
  std::uint64_t seed;
};

ProtocolIIIGrankConfig grank_config(const Case& test, std::uint8_t party) {
  ProtocolIIIGrankConfig c;
  c.session = test.seed | 1U;
  c.fingerprint = test.seed ^ UINT64_C(0x4d3542494e44494e);
  c.logical_n = static_cast<std::uint32_t>(test.scores.size());
  c.padded_n = padded_size(c.logical_n);
  c.k = test.k;
  c.comparison_bits = static_cast<std::uint8_t>(33U + width(c.padded_n - 1U));
  c.rank_bits = width(c.logical_n - 1U);
  c.party = party;
  c.timeout_ms = 5000;
  return c;
}

ProtocolIIIDpfRoutingConfig routing_config(const ProtocolIIIGrankConfig& g) {
  ProtocolIIIDpfRoutingConfig c;
  c.session = g.session;
  c.fingerprint = g.fingerprint;
  c.logical_n = g.logical_n;
  c.k = g.k;
  c.rank_bits = g.rank_bits;
  c.comparison_bits = g.comparison_bits;
  c.party = g.party;
  c.timeout_ms = g.timeout_ms;
  return c;
}

struct PartyState {
  std::vector<std::uint64_t> key_shares;
  ProtocolIPartyPackage grank_material;
  ProtocolIIIDpfRoutingPartyMaterial routing_material;
#ifdef MOE_TOPK_M5C_FULL_CHAIN
  ProtocolIIISecureCombinePartyMaterial combine_material;
  std::vector<std::uint64_t> unit_payload_shares;
#endif
};

std::array<PartyState, 2> prepare(const Case& test,
                                  const ProtocolIIIGrankConfig& c,
                                  std::mt19937_64& rng) {
  std::array<PartyState, 2> parties;
  const auto key_mask = mask(c.comparison_bits);
  const auto rank_mask = mask(c.rank_bits);
  for (std::uint8_t p = 0; p < 2; ++p) {
    auto& state = parties[p];
    state.key_shares.resize(c.padded_n);
    auto& g = state.grank_material;
    g.session = c.session;
    g.fingerprint = c.fingerprint;
    g.party = p;
    g.comparison_bits = c.comparison_bits;
    g.n = c.logical_n;
    g.k = c.k;
    g.node_mask_shares.resize(c.logical_n);
    auto& r = state.routing_material;
    r.session = c.session;
    r.fingerprint = c.fingerprint;
    r.party = p;
    r.logical_n = c.logical_n;
    r.k = c.k;
    r.rank_bits = c.rank_bits;
    r.rank_mask_shares.resize(c.logical_n);
    r.dpf_keys.reserve(c.logical_n);
#ifdef MOE_TOPK_M5C_FULL_CHAIN
    auto& m = state.combine_material;
    m.session = c.session;
    m.fingerprint = c.fingerprint;
    m.party = p;
    m.logical_n = c.logical_n;
    m.k = c.k;
    m.multiplication_materials.reserve(
        static_cast<std::size_t>(c.logical_n) * c.k);
    state.unit_payload_shares.resize(c.logical_n);
#endif
  }

  // TEST_ONLY input distribution. The party execution below receives shares,
  // never plaintext scores, both FSS keys, or the clear-rank oracle.
  for (std::uint32_t i = 0; i < c.padded_n; ++i) {
    const auto score = i < c.logical_n ? test.scores[i] : UINT32_C(0x80000000);
    const auto key = protocol_i_priority_key(score, i, c.padded_n).value;
    parties[0].key_shares[i] = rng() & key_mask;
    parties[1].key_shares[i] = (key - parties[0].key_shares[i]) & key_mask;
  }

  std::vector<std::uint64_t> comparison_masks(c.logical_n);
  for (std::uint32_t i = 0; i < c.logical_n; ++i) {
    comparison_masks[i] = rng() & key_mask;
    parties[0].grank_material.node_mask_shares[i] = rng() & key_mask;
    parties[1].grank_material.node_mask_shares[i] =
        (comparison_masks[i] - parties[0].grank_material.node_mask_shares[i]) & key_mask;

    const auto r_rank = rng() & rank_mask;
    parties[0].routing_material.rank_mask_shares[i] = rng() & rank_mask;
    parties[1].routing_material.rank_mask_shares[i] =
        (r_rank - parties[0].routing_material.rank_mask_shares[i]) & rank_mask;
    auto keys = keyGenDPF(c.rank_bits, 64, r_rank, 1);
    parties[0].routing_material.dpf_keys.emplace_back(std::move(keys.first));
    parties[1].routing_material.dpf_keys.emplace_back(std::move(keys.second));
#ifdef MOE_TOPK_M5C_FULL_CHAIN
    parties[0].unit_payload_shares[i] = rng();
    parties[1].unit_payload_shares[i] =
        UINT64_C(1) - parties[0].unit_payload_shares[i];
#endif
  }

  for (std::uint32_t left = 0; left < c.logical_n; ++left) {
    for (std::uint32_t right = left + 1U; right < c.logical_n; ++right) {
      ProtocolIUcmpMaterial edge(c.comparison_bits, comparison_masks[left],
                                 comparison_masks[right]);
      parties[0].grank_material.edge_materials.emplace_back(
          left, right, edge.export_party_material(0));
      parties[1].grank_material.edge_materials.emplace_back(
          left, right, edge.export_party_material(1));
    }
  }
#ifdef MOE_TOPK_M5C_FULL_CHAIN
  for (std::size_t cell = 0;
       cell < static_cast<std::size_t>(c.logical_n) * c.k; ++cell) {
    auto pair = generate_masked_mul_material(rng(), rng(), rng());
    parties[0].combine_material.multiplication_materials.push_back(
        std::move(pair.party0));
    parties[1].combine_material.multiplication_materials.push_back(
        std::move(pair.party1));
  }
#endif
  return parties;
}

struct PartyOutput {
  ProtocolIIIGrankOutput grank;
  ProtocolIIIDpfRoutingOutput routing;
#ifdef MOE_TOPK_M5C_FULL_CHAIN
  ProtocolIIISecureCombineOutput combine;
#endif
};

PartyOutput execute_party(const ProtocolIIIGrankConfig& c, PartyState& state,
                          int grank_fd, int routing_fd
#ifdef MOE_TOPK_M5C_FULL_CHAIN
                          , int combine_fd
#endif
                          ) {
  PartyOutput out;
  out.grank = protocol_iii_grank_party(c, state.grank_material,
                                       state.key_shares, grank_fd);
  out.routing = protocol_iii_dpf_routing_party(
      routing_config(c), state.routing_material,
      out.grank.rank_additive_shares, routing_fd);
#ifdef MOE_TOPK_M5C_FULL_CHAIN
  ProtocolIIISecureCombineConfig config;
  config.session = c.session;
  config.fingerprint = c.fingerprint;
  config.logical_n = c.logical_n;
  config.k = c.k;
  config.comparison_bits = c.comparison_bits;
  config.party = c.party;
  config.timeout_ms = c.timeout_ms;
  out.combine = protocol_iii_secure_combine_party(
      config, state.combine_material, out.routing.indicator_shares,
      state.unit_payload_shares, combine_fd);
#endif
  return out;
}

void run_case(const Case& test) {
  const auto c0 = grank_config(test, 0);
  const auto c1 = grank_config(test, 1);
  check(c0.logical_n >= 2 && test.k >= 1 && test.k <= c0.logical_n, "case shape");
  // C-INSTANTIATION: paper routing uses Z_n; this ABI uses the smallest
  // power-of-two rank ring and checks that legitimate ranks stay in [0,n).
  if (c0.logical_n == 3U) check(c0.rank_bits == 2U, "n=3 must use Z4");
  if (c0.logical_n == 5U) check(c0.rank_bits == 3U, "n=5 must use Z8");
  seed_fss(test.seed);
  std::mt19937_64 rng(test.seed);
  auto states = prepare(test, c0, rng);

  rejects([&] {
    (void)protocol_iii_grank_party(c0, states[1].grank_material,
                                    states[1].key_shares, -1);
  }, "GRank accepted peer material");
  rejects([&] {
    (void)protocol_iii_dpf_routing_party(
        routing_config(c0), states[1].routing_material, {}, -1);
  }, "routing accepted peer material");

  // Malformed public/API inputs fail before any online exchange. A clear
  // reconstructed rank is deliberately never checked by the secure API.
  rejects([&] {
    (void)protocol_iii_dpf_routing_party(
        routing_config(c0), states[0].routing_material,
        std::vector<std::uint64_t>(c0.logical_n - 1U), -1);
  }, "routing accepted short rank-share vector");
  auto noncanonical_shares = std::vector<std::uint64_t>(c0.logical_n, 0);
  noncanonical_shares[0] = UINT64_C(1) << c0.rank_bits;
  rejects([&] {
    (void)protocol_iii_dpf_routing_party(
        routing_config(c0), states[0].routing_material,
        noncanonical_shares, -1);
  }, "routing accepted noncanonical rank share");
  auto wrong_session = routing_config(c0);
  ++wrong_session.session;
  rejects([&] {
    (void)protocol_iii_dpf_routing_party(
        wrong_session, states[0].routing_material,
        noncanonical_shares, -1);
  }, "routing accepted wrong-session material");
  auto padded_target = routing_config(c0);
  padded_target.k = c0.logical_n + 1U;
  rejects([&] {
    (void)protocol_iii_dpf_routing_party(
        padded_target, states[0].routing_material,
        noncanonical_shares, -1);
  }, "routing accepted a padded target rank");

  SocketPair grank_sockets;
  SocketPair routing_sockets;
#ifdef MOE_TOPK_M5C_FULL_CHAIN
  SocketPair combine_sockets;
#endif
  std::array<PartyOutput, 2> outputs;
  std::array<std::exception_ptr, 2> errors;
  std::array<std::thread, 2> workers;
  for (std::uint8_t p = 0; p < 2; ++p) {
    const int gfd = grank_sockets.duplicate(p);
    const int rfd = routing_sockets.duplicate(p);
#ifdef MOE_TOPK_M5C_FULL_CHAIN
    const int cfd = combine_sockets.duplicate(p);
    workers[p] = std::thread([&, p, gfd, rfd, cfd] {
#else
    workers[p] = std::thread([&, p, gfd, rfd] {
#endif
      try {
        outputs[p] = execute_party(p == 0 ? c0 : c1, states[p], gfd, rfd
#ifdef MOE_TOPK_M5C_FULL_CHAIN
                                   , cfd
#endif
                                   );
      } catch (...) {
        errors[p] = std::current_exception();
      }
    });
  }
  for (auto& worker : workers) worker.join();
  for (const auto& error : errors) if (error) std::rethrow_exception(error);

  const auto oracle = stable_ranks_cmpagg(test.scores);
  std::vector<bool> seen(c0.logical_n);
  const auto rank_mask = mask(c0.rank_bits);
  for (std::uint8_t p = 0; p < 2; ++p) {
    check(outputs[p].grank.rank_additive_shares.size() == c0.logical_n,
          "padded rank leaked");
    check(outputs[p].routing.indicator_shares.size() ==
              static_cast<std::size_t>(c0.logical_n) * test.k,
          "indicator shape");
    check(outputs[p].grank.metrics.comparison_edges ==
              static_cast<std::uint64_t>(c0.logical_n) * (c0.logical_n - 1U) / 2U,
          "logical clique count");
    check(outputs[p].routing.metrics.eval_calls ==
              static_cast<std::uint64_t>(c0.logical_n) * test.k,
          "DPF eval count");
    check(states[p].grank_material.node_mask_shares.empty() &&
              states[p].grank_material.edge_materials.empty(),
          "CmpAgg material not consumed");
    check(states[p].routing_material.rank_mask_shares.empty() &&
              states[p].routing_material.dpf_keys.empty(),
          "DPF material not consumed");
#ifdef MOE_TOPK_M5C_FULL_CHAIN
    check(states[p].combine_material.multiplication_materials.empty(),
          "combine material not consumed");
    check(outputs[p].combine.xor_mask_shares.size() == c0.logical_n,
          "final mask shape");
    check(outputs[p].combine.metrics.multiplication_calls ==
              static_cast<std::uint64_t>(c0.logical_n) * test.k,
          "combine call count");
    check(outputs[p].grank.metrics.online_rounds +
                  outputs[p].routing.metrics.online_rounds +
                  outputs[p].combine.metrics.online_rounds == 3U,
          "modular round count");
#endif
  }
  for (std::size_t i = 0; i < oracle.size(); ++i) {
    for (const auto& out : outputs)
      check((out.grank.rank_additive_shares[i] & ~rank_mask) == 0,
            "noncanonical rank share");
    const auto rank = (outputs[0].grank.rank_additive_shares[i] +
                       outputs[1].grank.rank_additive_shares[i]) & rank_mask;
    check(rank == oracle[i], "CmpAgg/oracle mismatch");
    check(rank < c0.logical_n && !seen[rank], "invalid logical rank");
    seen[rank] = true;
    for (std::uint32_t target = 0; target < test.k; ++target) {
      const auto cell = i * test.k + target;
      const auto indicator = outputs[0].routing.indicator_shares[cell] +
                             outputs[1].routing.indicator_shares[cell];
      check(indicator == (oracle[i] == target ? 1U : 0U),
            "DPF/oracle mismatch");
    }
  }
#ifdef MOE_TOPK_M5C_FULL_CHAIN
  const auto expected_mask = top_k_mask(test.scores, test.k);
  std::uint32_t selected = 0;
  for (std::size_t i = 0; i < oracle.size(); ++i) {
    check(expected_mask[i] == static_cast<std::uint8_t>(oracle[i] < test.k),
          "rank and Top-K oracle disagree");
    for (const auto& out : outputs)
      check(out.combine.xor_mask_shares[i] <= 1U, "non-bit final share");
    const auto actual = static_cast<std::uint8_t>(
        outputs[0].combine.xor_mask_shares[i] ^
        outputs[1].combine.xor_mask_shares[i]);
    check(actual == expected_mask[i], "full modular output/oracle mismatch");
    selected += actual;
  }
  check(selected == test.k, "final output did not select k items");
#endif
  for (std::uint8_t p = 0; p < 2; ++p) {
    const auto& c = p == 0 ? c0 : c1;
    rejects([&] {
      (void)protocol_iii_grank_party(c, states[p].grank_material,
                                      states[p].key_shares, -1);
    }, "CmpAgg material reused");
    rejects([&] {
      (void)protocol_iii_dpf_routing_party(
          routing_config(c), states[p].routing_material,
          outputs[p].grank.rank_additive_shares, -1);
    }, "DPF material reused");
#ifdef MOE_TOPK_M5C_FULL_CHAIN
    ProtocolIIISecureCombineConfig combine_config;
    combine_config.session = c.session;
    combine_config.fingerprint = c.fingerprint;
    combine_config.logical_n = c.logical_n;
    combine_config.k = c.k;
    combine_config.comparison_bits = c.comparison_bits;
    combine_config.party = c.party;
    combine_config.timeout_ms = c.timeout_ms;
    rejects([&] {
      (void)protocol_iii_secure_combine_party(
          combine_config, states[p].combine_material,
          outputs[p].routing.indicator_shares,
          states[p].unit_payload_shares, -1);
    }, "combine material reused");
#endif
  }
}

std::vector<std::uint32_t> scores_for(std::uint32_t n, int pattern,
                                      std::uint64_t seed) {
  const std::array<std::int32_t, 5> mixed{{10, 20, 20, 5, 20}};
  const std::array<std::int32_t, 5> boundary{{
      std::numeric_limits<std::int32_t>::min(),
      std::numeric_limits<std::int32_t>::max(), 0, -1, 1}};
  std::mt19937_64 rng(seed);
  std::vector<std::uint32_t> scores(n);
  for (std::uint32_t i = 0; i < n; ++i) {
    std::int32_t score = 0;
    switch (pattern) {
      case 0: score = static_cast<std::int32_t>(n - i); break;
      case 1: score = static_cast<std::int32_t>(i); break;
      case 2: score = 7; break;
      case 3: score = mixed[i % mixed.size()]; break;
      case 4: score = boundary[i % boundary.size()]; break;
      default: score = static_cast<std::int32_t>(rng() % 17U) - 8; break;
    }
    scores[i] = static_cast<std::uint32_t>(score);
  }
  return scores;
}

#ifdef MOE_TOPK_M5C_FULL_CHAIN
void test_cross_stage_binding() {
  const Case test{{9U, 9U, 3U}, 2U, UINT64_C(0x4d354343524f5353)};
  seed_fss(test.seed);
  std::mt19937_64 rng(test.seed);
  auto states = prepare(test, grank_config(test, 0U), rng);
  auto& state = states[0];
  ProtocolIIISecureCoreConfig config;
  config.grank = grank_config(test, 0U);
  config.routing = routing_config(config.grank);
  config.combine.session = config.grank.session;
  config.combine.fingerprint = config.grank.fingerprint;
  config.combine.logical_n = config.grank.logical_n;
  config.combine.k = config.grank.k;
  config.combine.comparison_bits = config.grank.comparison_bits;
  config.combine.party = 0U;
  config.combine.timeout_ms = config.grank.timeout_ms;
  ProtocolIIISecureCoreMaterial material;
  material.grank_package = std::move(state.grank_material);
  material.routing_material = std::move(state.routing_material);
  material.combine_material = std::move(state.combine_material);
  material.unit_payload_shares = std::move(state.unit_payload_shares);
  const ProtocolIIISecureCoreFds invalid_fds{-1, -1, -1};
  auto call = [&] {
    (void)protocol_iii_secure_core_party(
        config, material, state.key_shares, invalid_fds);
  };
  ++config.routing.session;
  rejects(call, "cross-stage routing session mismatch");
  --config.routing.session;
  ++config.combine.fingerprint;
  rejects(call, "cross-stage combine fingerprint mismatch");
  --config.combine.fingerprint;
  ++material.routing_material.session;
  rejects(call, "cross-stage routing material session mismatch");
  --material.routing_material.session;
  ++material.combine_material.fingerprint;
  rejects(call, "cross-stage combine material fingerprint mismatch");
  --material.combine_material.fingerprint;
  material.combine_material.party = 1U;
  rejects(call, "cross-stage party-swapped combine material");
  material.combine_material.party = 0U;
  ++config.routing.rank_bits;
  rejects(call, "cross-stage rank width mismatch");
  --config.routing.rank_bits;
  material.unit_payload_shares.pop_back();
  rejects(call, "cross-stage unit-payload count mismatch");
  material.unit_payload_shares.push_back(0U);
  const ProtocolIIISecureCoreFds duplicate_fds{0, 0, 1};
  rejects([&] {
    (void)protocol_iii_secure_core_party(
        config, material, state.key_shares, duplicate_fds);
  }, "duplicate online descriptors accepted");

  auto routing_call = [&] {
    (void)protocol_iii_dpf_routing_party(
        config.routing, material.routing_material,
        std::vector<std::uint64_t>(config.grank.logical_n, 0U), -1);
  };
  config.routing.k = 0U;
  rejects(routing_call, "zero k accepted");
  config.routing.k = config.grank.logical_n + 1U;
  rejects(routing_call, "k beyond logical n accepted");
  config.routing.k = config.grank.k;
  material.routing_material.dpf_keys[0].native_key().bin += 1;
  rejects(routing_call, "wrong DPF input width accepted");
  material.routing_material.dpf_keys[0].native_key().bin -= 1;
  material.routing_material.dpf_keys[0].native_key().bout = 63;
  rejects(routing_call, "wrong DPF payload width accepted");
  material.routing_material.dpf_keys[0].native_key().bout = 64;
  material.routing_material.rank_mask_shares.push_back(0U);
  rejects(routing_call, "extra rank mask accepted");
  material.routing_material.rank_mask_shares.pop_back();
  material.routing_material.dpf_keys.pop_back();
  rejects(routing_call, "truncated DPF material accepted");

  ProtocolIIISecureCombineConfig combine_config = config.combine;
  const std::vector<std::uint64_t> indicators(
      static_cast<std::size_t>(config.grank.logical_n) * config.grank.k, 0U);
  auto combine_call = [&] {
    (void)protocol_iii_secure_combine_party(
        combine_config, material.combine_material, indicators,
        material.unit_payload_shares, -1);
  };
  combine_config.k = 0U;
  rejects(combine_call, "combine accepted zero k");
  combine_config.k = config.grank.k;
  material.combine_material.multiplication_materials.pop_back();
  rejects(combine_call, "truncated combine material accepted");
  auto extra = generate_masked_mul_material(rng(), rng(), rng());
  material.combine_material.multiplication_materials.push_back(
      std::move(extra.party0));
  material.combine_material.multiplication_materials.push_back(
      std::move(extra.party1));
  rejects(combine_call, "extra combine material accepted");
}

void test_partial_failure_consumes_material() {
  const Case test{{7U, 7U, 3U}, 2U, UINT64_C(0x4d35435041525431)};
  seed_fss(test.seed);
  std::mt19937_64 rng(test.seed);
  const auto c = grank_config(test, 1U);
  auto states = prepare(test, c, rng);
  auto& state = states[1];

  auto closed_peer_exchange = [](auto&& attempt, const char* message) {
    int fds[2]{-1, -1};
    check(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0,
          "partial-failure socketpair");
    ::close(fds[0]);
    bool failed = false;
    try {
      attempt(fds[1]);
    } catch (const std::exception&) {
      failed = true;
    }
    check(failed, message);
  };

  closed_peer_exchange([&](int fd) {
    (void)protocol_iii_grank_party(
        c, state.grank_material, state.key_shares, fd);
  }, "GRank accepted closed peer");
  check(state.grank_material.node_mask_shares.empty() &&
            state.grank_material.edge_materials.empty(),
        "GRank material survived failed exchange");
  rejects([&] {
    (void)protocol_iii_grank_party(
        c, state.grank_material, state.key_shares, -1);
  }, "GRank material reused after failed exchange");

  const std::vector<std::uint64_t> rank_shares(c.logical_n, 0U);
  closed_peer_exchange([&](int fd) {
    (void)protocol_iii_dpf_routing_party(
        routing_config(c), state.routing_material, rank_shares, fd);
  }, "routing accepted closed peer");
  check(state.routing_material.started,
        "routing material not marked started after failed exchange");
  rejects([&] {
    (void)protocol_iii_dpf_routing_party(
        routing_config(c), state.routing_material, rank_shares, -1);
  }, "routing material reused after failed exchange");

  ProtocolIIISecureCombineConfig combine_config;
  combine_config.session = c.session;
  combine_config.fingerprint = c.fingerprint;
  combine_config.logical_n = c.logical_n;
  combine_config.k = c.k;
  combine_config.comparison_bits = c.comparison_bits;
  combine_config.party = c.party;
  combine_config.timeout_ms = c.timeout_ms;
  const std::vector<std::uint64_t> indicators(
      static_cast<std::size_t>(c.logical_n) * c.k, 0U);
  closed_peer_exchange([&](int fd) {
    (void)protocol_iii_secure_combine_party(
        combine_config, state.combine_material, indicators,
        state.unit_payload_shares, fd);
  }, "combine accepted closed peer");
  int retry_fds[2]{-1, -1};
  check(::socketpair(AF_UNIX, SOCK_STREAM, 0, retry_fds) == 0,
        "combine retry socketpair");
  bool rejected = false;
  try {
    (void)protocol_iii_secure_combine_party(
        combine_config, state.combine_material, indicators,
        state.unit_payload_shares, retry_fds[1]);
  } catch (const std::logic_error&) {
    rejected = true;
  }
  ::close(retry_fds[0]);
  ::close(retry_fds[1]);
  check(rejected, "combine material reused after failed exchange");
}
#endif

}  // namespace

int main() {
  try {
#ifdef MOE_TOPK_M5C_FULL_CHAIN
    test_cross_stage_binding();
    test_partial_failure_consumes_material();
#endif
    std::uint64_t cases = 0;
    for (const std::uint32_t n : {2U, 3U, 4U, 5U, 8U}) {
      std::vector<std::uint32_t> targets{1U, n};
      if (n > 2U) targets.push_back(n == 3U ? 2U : n / 2U);
      for (auto k : targets) {
        for (int pattern = 0; pattern < 8; ++pattern) {
          const auto seed = UINT64_C(0x4d35424300000000) + ++cases;
          auto scores = scores_for(n, pattern, seed);
          if (pattern == 2) {
            const auto expected = stable_ranks_cmpagg(scores);
            for (std::uint32_t i = 0; i < n; ++i)
              check(expected[i] == i, "all-equal tie order");
          }
          run_case({std::move(scores), k, seed});
        }
      }
    }
#ifdef MOE_TOPK_M5C_FULL_CHAIN
    std::cout << "M5-C CmpAgg-to-final-mask differential cases: "
              << cases << '\n';
#else
    std::cout << "M5-B CmpAgg-to-DPF differential cases: " << cases << '\n';
#endif
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "M5-B differential failed: " << error.what() << '\n';
    return 1;
  }
}
