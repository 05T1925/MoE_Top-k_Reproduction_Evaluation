#include <moe_topk/project_derived_select5.h>
#include <moe_topk/protocol_i_priority_key.h>
#include <moe_topk/protocol_i_ucmp.h>
#include <moe_topk/protocol_i_transport.h>
#include <FSS/prng.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>
#include <tuple>
#include <vector>
#include <sys/socket.h>
#include <unistd.h>

namespace {
using namespace moe_topk;

void check(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}

std::uint64_t ring_mask(unsigned bits) {
  return (UINT64_C(1) << bits) - 1U;
}

std::uint64_t stable_key(std::uint32_t raw, std::uint32_t index,
                         unsigned index_bits) {
  const auto ordered = raw ^ UINT32_C(0x80000000);
  const auto high = UINT32_MAX - ordered;
  return (static_cast<std::uint64_t>(high) << index_bits) | index;
}

void seed_test_prgs() {
  for (int i = 0; i < 256; ++i)
    FSSConfig::prngs[i].SetSeed(osuCrypto::toBlock(
        UINT64_C(0x53454c4543543500) + static_cast<std::uint64_t>(i),
        UINT64_C(0x4d4154455249414c) ^ static_cast<std::uint64_t>(i)));
}

std::uint32_t select_rank(const std::vector<std::uint32_t>& rank_by_position,
                          std::uint32_t k, std::uint64_t seed,
                          std::vector<ProjectDerivedSelect5Trace>* trace_out = nullptr) {
  const auto control = protocol_i_project_derived_select5_control(
      static_cast<std::uint32_t>(rank_by_position.size()), k, seed,
      [&rank_by_position](std::uint8_t, std::uint8_t,
                          const std::vector<ProjectDerivedSelect5Edge>& edges) {
        std::vector<std::uint8_t> outcomes;
        outcomes.reserve(edges.size());
        for (const auto& edge : edges)
          outcomes.push_back(static_cast<std::uint8_t>(
              rank_by_position[edge.left] < rank_by_position[edge.right]));
        return outcomes;
      });
  check(control.iterations.size() == 5, "five trace iterations");
  check(control.selected_position < rank_by_position.size(), "selected coordinate range");
  check(rank_by_position[control.selected_position] == k - 1U,
        "control selector rank");
  if (trace_out) *trace_out = control.iterations;
  return control.selected_position;
}

void control_exhaustive_small() {
  for (std::uint32_t n = 1; n <= 8; ++n) {
    std::vector<std::uint32_t> order(n);
    for (std::uint32_t i = 0; i < n; ++i) order[i] = i;
    do {
      std::vector<std::uint32_t> rank(n);
      for (std::uint32_t r = 0; r < n; ++r) rank[order[r]] = r;
      for (std::uint32_t k = 1; k <= n; ++k) {
        const auto chosen = select_rank(rank, k, UINT64_C(0x53000000) + n * 257U + k);
        check(rank[chosen] == k - 1U, "all permutations and K");
      }
    } while (std::next_permutation(order.begin(), order.end()));
  }
}

void control_trace_and_tape_independence() {
  constexpr std::uint32_t n = 128;
  std::vector<std::uint32_t> rank(n);
  std::mt19937 rng(0x51a7);
  for (std::uint32_t i = 0; i < n; ++i) rank[i] = i;
  std::shuffle(rank.begin(), rank.end(), rng);
  std::vector<std::uint32_t> rank_by_position(n);
  for (std::uint32_t r = 0; r < n; ++r) rank_by_position[r] = r;
  for (std::uint32_t k : {1U, 2U, 8U, 64U, 128U}) {
    std::vector<ProjectDerivedSelect5Trace> first, second;
    (void)select_rank(rank_by_position, k, UINT64_C(0x1285), &first);
    (void)select_rank(rank_by_position, k, UINT64_C(0x9972), &second);
    check(first.size() == 5 && second.size() == 5, "trace has five iterations");
    std::uint64_t first_edges = 0;
    for (std::size_t i = 0; i < first.size(); ++i) {
      check(first[i].candidate_before.size() >= 1, "candidate nonempty");
      if (first[i].sample.empty()) continue;
      std::set<std::pair<std::uint32_t, std::uint32_t>> used;
      for (const auto& layer : first[i].layers) {
        first_edges += layer.edges.size();
        for (const auto& edge : layer.edges)
          check(used.emplace(edge.left, edge.right).second,
                "same-iteration sublayers have disjoint pairs");
      }
    }
    check(first_edges > 0, "trace has comparisons");
    const auto same = first[0].sample == second[0].sample;
    check(!same, "distinct public sampler tapes change the sample");
  }
}

template <typename Function>
void rejects(Function&& function, const char* message) {
  bool rejected = false;
  try {
    function();
  } catch (const std::exception&) {
    rejected = true;
  }
  check(rejected, message);
}

void public_sample_randomness_contract() {
  ProjectDerivedSelect5PublicSeed seed{};
  for (std::size_t i = 0; i < seed.size(); ++i)
    seed[i] = static_cast<std::uint8_t>(i);

  const auto encoded =
      protocol_i_project_derived_select5_encode_public_sample_seed(2, seed);
  check(encoded.size() == 41, "v3 sample-seed frame has fixed 41-byte payload");
  check(protocol_i_project_derived_select5_decode_public_sample_seed(encoded, 2) ==
            seed,
        "sample seed wire round trip");

  auto wrong_version = encoded;
  wrong_version[7] ^= 1U;
  rejects([&] {
    (void)protocol_i_project_derived_select5_decode_public_sample_seed(
        wrong_version, 2);
  }, "wrong sample seed contract version rejected");
  auto wrong_iteration = encoded;
  wrong_iteration[8] = 1;
  rejects([&] {
    (void)protocol_i_project_derived_select5_decode_public_sample_seed(
        wrong_iteration, 2);
  }, "misbound sample seed iteration rejected");
  auto truncated = encoded;
  truncated.pop_back();
  rejects([&] {
    (void)protocol_i_project_derived_select5_decode_public_sample_seed(
        truncated, 2);
  }, "truncated sample seed rejected");
  auto trailing = encoded;
  trailing.push_back(0);
  rejects([&] {
    (void)protocol_i_project_derived_select5_decode_public_sample_seed(
        trailing, 2);
  }, "sample seed trailing bytes rejected");

  std::vector<std::uint32_t> population(128);
  for (std::uint32_t i = 0; i < population.size(); ++i) population[i] = i;
  const auto p0 = protocol_i_project_derived_select5_sample_from_public_seed(
      population, 11, seed, UINT64_C(0x12345678), UINT64_C(0xabcdef01), 2);
  const auto p1 = protocol_i_project_derived_select5_sample_from_public_seed(
      population, 11, seed, UINT64_C(0x12345678), UINT64_C(0xabcdef01), 2);
  check(p0 == p1 && p0.size() == 11,
        "both parties reconstruct identical fixed-size sample");
  check(std::set<std::uint32_t>(p0.begin(), p0.end()).size() == p0.size(),
        "public sample is without replacement");
  check(p0 == std::vector<std::uint32_t>{4, 101, 124, 107, 34, 3,
                                        126, 43, 68, 1, 89},
        "public sample matches independently specified byte/PRNG vector");
  for (const auto item : p0)
    check(item < population.size(), "sample coordinate belongs to candidate set");
  const auto other_iteration =
      protocol_i_project_derived_select5_sample_from_public_seed(
          population, 11, seed, UINT64_C(0x12345678), UINT64_C(0xabcdef01), 1);
  check(p0 != other_iteration,
        "iteration domain separates deterministic sample streams");
  rejects([&] {
    (void)protocol_i_project_derived_select5_sample_from_public_seed(
        std::vector<std::uint32_t>{4, 4}, 1, seed, 1, 2, 0);
  }, "duplicate candidate coordinate rejected");

  ProjectDerivedSelect5DisclosureState state;
  rejects([&] { state.begin_active_iteration(0); },
          "sample disclosure before forward shuffle rejected");
  state.mark_forward_shuffle_complete();
  rejects([&] { state.mark_forward_shuffle_complete(); },
          "duplicate forward-shuffle completion rejected");
  state.begin_active_iteration(0);
  state.begin_active_iteration(1);
  rejects([&] { state.begin_active_iteration(1); },
          "replayed sample iteration rejected");
  rejects([&] { state.begin_active_iteration(3); },
          "out-of-order future sample iteration rejected");
}

void public_sample_transport_binding() {
  int sockets[2] = {-1, -1};
  check(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0,
        "sample seed session-binding socketpair");
  ProjectDerivedSelect5PublicSeed seed{};
  const auto message =
      protocol_i_project_derived_select5_encode_public_sample_seed(0, seed);
  ProtocolIFrameConfig sender_config{
      101, 202, 128, 8, 41, 0, 1, 40, 9, 303};
  ProtocolIFrameConfig wrong_session_config{
      102, 202, 128, 8, 41, 1, 0, 40, 9, 303};
  {
    ProtocolIFramedChannel sender(sockets[0], sender_config,
                                  ProtocolIFramedChannelOptions{1000});
    sender.send(message);
    ProtocolIFramedChannel receiver(sockets[1], wrong_session_config,
                                   ProtocolIFramedChannelOptions{1000});
    rejects([&] { (void)receiver.receive(); },
            "sample seed from another session rejected by frame binding");
  }
  sockets[0] = sockets[1] = -1;  // channels own and closed both descriptors

  int phase_sockets[2] = {-1, -1};
  check(::socketpair(AF_UNIX, SOCK_STREAM, 0, phase_sockets) == 0,
        "sample seed replay/phase socketpair");
  {
    ProtocolIFramedChannel sender(phase_sockets[0], sender_config,
                                  ProtocolIFramedChannelOptions{1000});
    sender.send(message);
    auto next_iteration_config = sender_config;
    next_iteration_config.sender = 1;
    next_iteration_config.receiver = 0;
    next_iteration_config.phase = 41;
    ProtocolIFramedChannel receiver(phase_sockets[1], next_iteration_config,
                                   ProtocolIFramedChannelOptions{1000});
    rejects([&] { (void)receiver.receive(); },
            "replayed prior-iteration sample frame rejected by phase binding");
  }
  phase_sockets[0] = phase_sockets[1] = -1;
}

void membership_dcf_conformance() {
  constexpr unsigned n = 128;
  constexpr unsigned index_bits = 7;
  constexpr unsigned bits = 40;
  const auto mask = ring_mask(bits);
  std::vector<std::pair<std::vector<std::uint32_t>, std::uint32_t>> cases;
  std::vector<std::uint32_t> edge_scores{UINT32_C(0x80000000), UINT32_MAX,
      0, UINT32_C(0x7fffffff), UINT32_C(0x7fffffff), 0,
      UINT32_C(0x80000001), UINT32_C(0x7ffffffe)};
  cases.emplace_back(edge_scores, 1);
  cases.emplace_back(edge_scores, 2);
  cases.emplace_back(edge_scores, static_cast<std::uint32_t>(edge_scores.size()));
  cases.emplace_back(std::vector<std::uint32_t>(128, UINT32_C(0x80000000)), 2);
  cases.emplace_back(std::vector<std::uint32_t>(128, UINT32_C(0x7fffffff)), 8);

  std::mt19937_64 masks(UINT64_C(0x5d4c4d454d424552));
  std::uint64_t material_number = 0;
  for (const auto& test : cases) {
    const auto& raw = test.first;
    const auto k = test.second;
    check(!raw.empty() && raw.size() <= n && k >= 1 && k <= raw.size(),
          "membership fixture");
    std::vector<std::uint64_t> keys(n);
    for (std::uint32_t i = 0; i < n; ++i) {
      const auto value = i < raw.size() ? raw[i] : UINT32_C(0x80000000);
      keys[i] = stable_key(value, i, index_bits);
    }
    std::vector<std::uint64_t> order = keys;
    std::sort(order.begin(), order.end());
    const auto tau = order[k - 1U];
    std::uint32_t selected_logical = 0;
    for (std::uint32_t i = 0; i < n; ++i) {
      const auto ml = masks() & mask, mr = masks() & mask;
      const auto ml0 = masks() & mask, mr0 = masks() & mask;
      const auto ml1 = (ml - ml0) & mask, mr1 = (mr - mr0) & mask;
      check(((ml0 + ml1) & mask) == ml && ((mr0 + mr1) & mask) == mr,
            "fresh one-shot mask sharing");
      ProtocolIUcmpMaterial generated(bits, ml, mr);
      auto p0 = ProtocolIUcmpPartyMaterial::deserialize(
          generated.export_party_material(0).serialize());
      auto p1 = ProtocolIUcmpPartyMaterial::deserialize(
          generated.export_party_material(1).serialize());
      const auto z_tau = (tau + ml) & mask;
      const auto z_item = (keys[i] + mr) & mask;
      const auto lt0 = p0.eval_strict_lt(z_tau, z_item);
      const auto lt1 = p1.eval_strict_lt(z_tau, z_item);
      const auto lt = lt0 + lt1;
      check(lt <= 1, "DCF strict result reconstructs to a bit");
      const auto membership0 = UINT64_C(1) - lt0;
      const auto membership1 = UINT64_C(0) - lt1;
      const auto xor_share = static_cast<std::uint8_t>(membership0 & 1U) ^
                             static_cast<std::uint8_t>(membership1 & 1U);
      const auto expected = static_cast<std::uint8_t>(keys[i] <= tau);
      check(xor_share == expected, "one minus strict_lt gives <= membership");
      if (i < raw.size()) selected_logical += xor_share;
      ++material_number;
    }
    check(selected_logical == k, "membership mask contains exactly K logical rows");
  }
  check(material_number == cases.size() * n, "all one-shot membership slots consumed");
}

}  // namespace

int main() {
  try {
    seed_test_prgs();
    control_exhaustive_small();
    control_trace_and_tape_independence();
    public_sample_randomness_contract();
    public_sample_transport_binding();
    membership_dcf_conformance();
    std::cout << "PROJECT_DERIVED_SELECT_5ITER C++ control + DCF membership PASS\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "PROJECT_DERIVED_SELECT_5ITER conformance FAIL: "
              << error.what() << '\n';
    return 1;
  }
}
