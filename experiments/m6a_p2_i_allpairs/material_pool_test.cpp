#include <moe_topk/protocol_i_ucmp.h>
#include <moe_topk/protocol_i_priority_key.h>
#include <FSS/prng.h>
#include <cryptoTools/Common/Defines.h>

#include <algorithm>
#include <chrono>
#include <array>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

const std::string kVersion = "PI-AAV86-E1-v1";
const std::string kSession = "e1-session";
constexpr std::uint32_t kN = 5;
constexpr std::uint32_t kRounds = 2;
constexpr int kBits = 36;
const std::uint64_t kRingMask = (UINT64_C(1) << kBits) - 1;

struct Edge {
  std::uint32_t a;
  std::uint32_t b;
  bool operator<(const Edge& other) const { return std::tie(a, b) < std::tie(other.a, other.b); }
  bool operator==(const Edge& other) const { return a == other.a && b == other.b; }
};

Edge canonical_edge(std::uint32_t a, std::uint32_t b) {
  if (a >= b) throw std::invalid_argument("non-canonical edge");
  return {a, b};
}

struct Request {
  std::string version;
  std::string session;
  std::uint32_t iteration;
  std::uint32_t endpoint_a;
  std::uint32_t endpoint_b;
  std::uint32_t invocation_id;
  int party_id;
  int bits;
};

using SlotKey = std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>;

void append_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
  out.push_back(static_cast<std::uint8_t>(value));
  out.push_back(static_cast<std::uint8_t>(value >> 8));
}
void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
  for (unsigned i = 0; i != 4; ++i) out.push_back(static_cast<std::uint8_t>(value >> (8 * i)));
}
void append_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
  for (unsigned i = 0; i != 8; ++i) out.push_back(static_cast<std::uint8_t>(value >> (8 * i)));
}
void append_string(std::vector<std::uint8_t>& out, const std::string& value) {
  if (value.size() > UINT16_MAX) throw std::invalid_argument("string too long");
  append_u16(out, static_cast<std::uint16_t>(value.size()));
  out.insert(out.end(), value.begin(), value.end());
}

class PartyPool {
 public:
  PartyPool() = default;

  void configure(int party, std::uint32_t n, std::uint32_t rounds, int bits,
                 const std::vector<std::uint64_t>& mask_shares, const std::string& session) {
    if (party < 0 || party > 1 || bits < 34 || bits > 53 || mask_shares.size() != n * rounds)
      throw std::invalid_argument("pool config");
    party_ = party;
    n_ = n;
    rounds_ = rounds;
    bits_ = bits;
    session_ = session;
    mask_shares_ = mask_shares;
  }

  void add(std::uint32_t iteration, std::uint32_t a, std::uint32_t b,
           std::vector<std::uint8_t> party_key) {
    const auto edge = canonical_edge(a, b);
    if (iteration >= rounds_ || edge.b >= n_ || party_key.empty())
      throw std::invalid_argument("pool slot");
    const SlotKey key{iteration, edge.a, edge.b};
    if (index_.count(key)) throw std::invalid_argument("duplicate pool slot");
    index_[key] = entries_.size();
    entries_.push_back(Entry{iteration, edge.a, edge.b, std::move(party_key)});
    consumed_.push_back(false);
  }

  void bind_graph(std::uint32_t iteration, const std::string& digest,
                  const std::vector<Edge>& edges) {
    if (iteration >= rounds_ || digest.empty() || graph_bindings_.count(iteration))
      throw std::invalid_argument("graph binding rejected");
    std::set<Edge> allowed;
    for (const auto& edge : edges) {
      if (edge.a >= edge.b || edge.b >= n_ || !index_.count(SlotKey{iteration, edge.a, edge.b}) ||
          !allowed.insert(edge).second)
        throw std::invalid_argument("graph edge rejected");
    }
    graph_bindings_.emplace(iteration, GraphBinding{digest, std::move(allowed)});
  }

  moe_topk::ProtocolIUcmpPartyMaterial take(const Request& request,
                                             const std::string& graph_digest) {
    // Deliberately generic failure text: no original position or missing-key detail escapes.
    if (request.version != kVersion || request.session != session_ ||
        request.party_id != party_ || request.bits != bits_ ||
        request.iteration >= rounds_ || request.endpoint_a >= request.endpoint_b ||
        request.endpoint_b >= n_ || request.invocation_id != 0)
      throw std::invalid_argument("pool request rejected");
    const auto binding = graph_bindings_.find(request.iteration);
    if (binding == graph_bindings_.end() || binding->second.digest != graph_digest ||
        !binding->second.edges.count(Edge{request.endpoint_a, request.endpoint_b}))
      throw std::invalid_argument("pool request rejected");
    const SlotKey key{request.iteration, request.endpoint_a, request.endpoint_b};
    const auto found = index_.find(key);
    if (found == index_.end() || consumed_[found->second])
      throw std::invalid_argument("pool request rejected");
    const auto& entry = entries_[found->second];
    auto material = moe_topk::ProtocolIUcmpPartyMaterial::deserialize(entry.key_bytes);
    if (material.party_id() != party_ || material.comparison_bits() != bits_)
      throw std::invalid_argument("pool request rejected");
    consumed_[found->second] = true;
    return material;
  }

  std::size_t count() const { return entries_.size(); }
  std::size_t consumed_count() const {
    return static_cast<std::size_t>(std::count(consumed_.begin(), consumed_.end(), true));
  }
  const std::vector<std::uint64_t>& mask_shares() const { return mask_shares_; }
  const std::vector<std::uint8_t>& key_bytes(std::uint32_t iteration,
                                              std::uint32_t a,
                                              std::uint32_t b) const {
    const auto found = index_.find(SlotKey{iteration, a, b});
    if (found == index_.end()) throw std::invalid_argument("pool slot absent");
    return entries_[found->second].key_bytes;
  }

  std::vector<std::uint8_t> serialize() const {
    std::vector<std::uint8_t> out{'M','6','A','P','I','A','P','1'};
    append_string(out, kVersion);
    append_string(out, session_);
    out.push_back(static_cast<std::uint8_t>(party_));
    append_u32(out, n_);
    append_u32(out, rounds_);
    append_u16(out, static_cast<std::uint16_t>(bits_));
    append_u64(out, mask_shares_.size());
    for (auto share : mask_shares_) append_u64(out, share);
    append_u64(out, entries_.size());
    for (const auto& entry : entries_) {
      append_u32(out, entry.iteration);
      append_u32(out, entry.a);
      append_u32(out, entry.b);
      append_u32(out, 0);  // one invocation slot per pair and iteration
      append_u32(out, static_cast<std::uint32_t>(entry.key_bytes.size()));
      out.insert(out.end(), entry.key_bytes.begin(), entry.key_bytes.end());
    }
    return out;
  }

 private:
  struct Entry {
    std::uint32_t iteration;
    std::uint32_t a;
    std::uint32_t b;
    std::vector<std::uint8_t> key_bytes;
  };
  struct GraphBinding {
    std::string digest;
    std::set<Edge> edges;
  };
  int party_ = -1;
  int bits_ = 0;
  std::string session_;
  std::uint32_t n_ = 0;
  std::uint32_t rounds_ = 0;
  std::vector<std::uint64_t> mask_shares_;
  std::vector<Entry> entries_;
  std::map<SlotKey, std::size_t> index_;
  std::vector<bool> consumed_;
  std::map<std::uint32_t, GraphBinding> graph_bindings_;
};

struct PoolPair {
  std::array<PartyPool, 2> parties;
  std::vector<std::uint64_t> full_masks;
};

PoolPair generate_pool(std::uint64_t test_seed, const std::string& session) {
  PoolPair result;
  std::mt19937_64 rng(test_seed);
  std::array<std::vector<std::uint64_t>, 2> local_mask_shares;
  local_mask_shares[0].reserve(kRounds * kN);
  local_mask_shares[1].reserve(kRounds * kN);
  result.full_masks.reserve(kRounds * kN);
  for (std::uint32_t iteration = 0; iteration < kRounds; ++iteration) {
    for (std::uint32_t endpoint = 0; endpoint < kN; ++endpoint) {
      const auto full = rng() & kRingMask;
      const auto share0 = rng() & kRingMask;
      const auto share1 = (full - share0) & kRingMask;
      result.full_masks.push_back(full);
      local_mask_shares[0].push_back(share0);
      local_mask_shares[1].push_back(share1);
    }
  }
  result.parties[0].configure(0, kN, kRounds, kBits, local_mask_shares[0], session);
  result.parties[1].configure(1, kN, kRounds, kBits, local_mask_shares[1], session);
  for (std::uint32_t iteration = 0; iteration < kRounds; ++iteration) {
    for (std::uint32_t a = 0; a < kN; ++a) {
      for (std::uint32_t b = a + 1; b < kN; ++b) {
        moe_topk::ProtocolIUcmpMaterial generated(
            kBits, result.full_masks[iteration * kN + a],
            result.full_masks[iteration * kN + b]);
        for (int party = 0; party < 2; ++party) {
          auto serialized = generated.export_party_material(party).serialize();
          result.parties[party].add(iteration, a, b, std::move(serialized));
        }
      }
    }
  }
  return result;
}

std::vector<std::vector<Edge>> read_graph_fixture(const std::string& path) {
  std::ifstream input(path);
  if (!input) throw std::runtime_error("cannot open graph fixture");
  std::string line;
  std::getline(input, line);  // header
  std::vector<std::vector<Edge>> edges(kRounds);
  while (std::getline(input, line)) {
    if (line.empty()) continue;
    std::istringstream row(line);
    std::uint32_t iteration, a, b;
    char comma1, comma2;
    if (!(row >> iteration >> comma1 >> a >> comma2 >> b) || comma1 != ',' || comma2 != ',' ||
        iteration == 0 || iteration > kRounds)
      throw std::runtime_error("invalid graph fixture");
    edges[iteration - 1].push_back(canonical_edge(a, b));
  }
  return edges;
}

std::string graph_label(std::uint32_t iteration, const std::vector<Edge>& edges) {
  // TEST_ONLY non-cryptographic label; detects accidental caller mismatch in this process.
  std::uint64_t h = UINT64_C(1469598103934665603) ^ iteration;
  for (const auto& edge : edges) {
    for (std::uint32_t word : {edge.a, edge.b}) {
      for (unsigned shift = 0; shift != 32; shift += 8) {
        h ^= static_cast<std::uint8_t>(word >> shift);
        h *= UINT64_C(1099511628211);
      }
    }
  }
  std::ostringstream out;
  out << "TEST-FNV64-" << std::hex << h;
  return out.str();
}

std::uint64_t validate_all_pair_slots(
    const std::array<std::uint64_t, kN>& priority, std::uint64_t test_seed) {
  auto validation_pool = generate_pool(test_seed, "e1-slot-conformance");
  std::vector<std::vector<Edge>> complete_graph(kRounds);
  for (std::uint32_t iteration = 0; iteration < kRounds; ++iteration) {
    for (std::uint32_t a = 0; a < kN; ++a)
      for (std::uint32_t b = a + 1; b < kN; ++b)
        complete_graph[iteration].push_back(Edge{a, b});
    const auto digest = graph_label(iteration, complete_graph[iteration]);
    validation_pool.parties[0].bind_graph(iteration, digest, complete_graph[iteration]);
    validation_pool.parties[1].bind_graph(iteration, digest, complete_graph[iteration]);
  }
  std::uint64_t checked = 0;
  for (std::uint32_t iteration = 0; iteration < kRounds; ++iteration) {
    const auto digest = graph_label(iteration, complete_graph[iteration]);
    for (const auto& edge : complete_graph[iteration]) {
      Request req{kVersion, "e1-slot-conformance", iteration, edge.a, edge.b, 0, 0, kBits};
      auto p0 = validation_pool.parties[0].take(req, digest);
      req.party_id = 1;
      auto p1 = validation_pool.parties[1].take(req, digest);
      const auto z_a = (priority[edge.a] + validation_pool.full_masks[iteration * kN + edge.a]) & kRingMask;
      const auto z_b = (priority[edge.b] + validation_pool.full_masks[iteration * kN + edge.b]) & kRingMask;
      const bool actual = (p0.eval_strict_lt(z_a, z_b) + p1.eval_strict_lt(z_a, z_b)) == 1;
      if (actual != (priority[edge.a] < priority[edge.b]))
        throw std::runtime_error("all-pair slot uCMP oracle mismatch");
      ++checked;
    }
  }
  if (checked != kRounds * (kN * (kN - 1) / 2) ||
      validation_pool.parties[0].consumed_count() != checked ||
      validation_pool.parties[1].consumed_count() != checked)
    throw std::runtime_error("all-pair slot validation accounting mismatch");
  return checked;
}
void expect_reject(const std::string& label, const std::function<void()>& action) {
  bool rejected = false;
  try { action(); } catch (const std::exception&) { rejected = true; }
  if (!rejected) throw std::runtime_error("expected rejection: " + label);
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc != 2) throw std::invalid_argument("usage: material_pool_test <graph_edges.csv>");
    for (int i = 0; i < 256; ++i)
      FSSConfig::prngs[i].SetSeed(osuCrypto::toBlock(UINT64_C(0x4d36414531544553),
                                                   static_cast<std::uint64_t>(i)));

    // Offline T-side setup runs before reading the active graph fixture.
    const auto offline_begin = std::chrono::steady_clock::now();
    auto pools = generate_pool(UINT64_C(0x20260930E1), kSession);
    const auto offline_end = std::chrono::steady_clock::now();
    const auto offline_ms = std::chrono::duration<double, std::milli>(offline_end - offline_begin).count();
    const auto first_round_key = pools.parties[0].key_bytes(0, 0, 1);
    const auto next_round_key = pools.parties[0].key_bytes(1, 0, 1);
    if (first_round_key == next_round_key) throw std::runtime_error("cross-round key reuse/equality");

    const auto graph = read_graph_fixture(argv[1]);
    std::size_t graph_edges = 0;
    for (std::uint32_t i = 0; i < kRounds; ++i) {
      graph_edges += graph[i].size();
      const auto digest = graph_label(i, graph[i]);
      pools.parties[0].bind_graph(i, digest, graph[i]);
      pools.parties[1].bind_graph(i, digest, graph[i]);
    }

    // Each endpoint label is a shuffled handle; the mapping is intentionally non-identity.
    const std::array<std::uint32_t, kN> original_for_handle{{3, 0, 4, 1, 2}};
    const std::array<std::int32_t, kN> raw_scores_by_original{{INT32_MAX, 7, 7, -4, INT32_MIN}};
    std::array<std::uint64_t, kN> priority{};
    std::array<std::uint64_t, kN> input_share0{};
    std::array<std::uint64_t, kN> input_share1{};
    std::mt19937_64 input_share_rng(UINT64_C(0x20260930A));
    for (std::uint32_t handle = 0; handle < kN; ++handle) {
      const auto original = original_for_handle[handle];
      priority[handle] = moe_topk::protocol_i_priority_key(
          static_cast<std::uint32_t>(raw_scores_by_original[original]), original, kN).value;
      input_share0[handle] = input_share_rng() & kRingMask;
      input_share1[handle] = (priority[handle] - input_share0[handle]) & kRingMask;
    }

    const auto all_pair_slot_checks = validate_all_pair_slots(priority, UINT64_C(0x20260930E2));
    Request valid{kVersion, kSession, 0, 0, 1, 0, 0, kBits};
    const auto digest0 = graph_label(0, graph[0]);
    expect_reject("wrong session", [&] { auto q = valid; q.session = "wrong"; (void)pools.parties[0].take(q, digest0); });
    expect_reject("wrong iteration", [&] { auto q = valid; q.iteration = 9; (void)pools.parties[0].take(q, digest0); });
    expect_reject("reversed endpoint order", [&] { auto q = valid; q.endpoint_a = 1; q.endpoint_b = 0; (void)pools.parties[0].take(q, digest0); });
    expect_reject("wrong party", [&] { auto q = valid; q.party_id = 1; (void)pools.parties[0].take(q, digest0); });
    expect_reject("missing invocation slot", [&] { auto q = valid; q.invocation_id = 1; (void)pools.parties[0].take(q, digest0); });
    expect_reject("wrong graph digest", [&] { (void)pools.parties[0].take(valid, "wrong-digest"); });
    expect_reject("wrong parameters", [&] { auto q = valid; q.bits = 34; (void)pools.parties[0].take(q, digest0); });
    expect_reject("edge outside active graph", [&] { auto q = valid; q.endpoint_a = 0; q.endpoint_b = 2; (void)pools.parties[0].take(q, digest0); });

    const auto local_online_begin = std::chrono::steady_clock::now();
    std::uint64_t online_edges = 0;
    for (std::uint32_t iteration = 0; iteration < kRounds; ++iteration) {
      const auto digest = graph_label(iteration, graph[iteration]);
      for (const auto& edge : graph[iteration]) {
        Request req{kVersion, kSession, iteration, edge.a, edge.b, 0, 0, kBits};
        auto p0 = pools.parties[0].take(req, digest);
        req.party_id = 1;
        auto p1 = pools.parties[1].take(req, digest);
        const auto mask_index_a = iteration * kN + edge.a;
        const auto mask_index_b = iteration * kN + edge.b;
        const auto z_a = (priority[edge.a] + pools.full_masks[mask_index_a]) & kRingMask;
        const auto z_b = (priority[edge.b] + pools.full_masks[mask_index_b]) & kRingMask;
        const auto share_a0 = (input_share0[edge.a] + pools.parties[0].mask_shares()[mask_index_a]) & kRingMask;
        const auto share_a1 = (input_share1[edge.a] + pools.parties[1].mask_shares()[mask_index_a]) & kRingMask;
        if (((share_a0 + share_a1) & kRingMask) != z_a)
          throw std::runtime_error("test-only masked share reconstruction mismatch");
        const auto share_b0 = (input_share0[edge.b] + pools.parties[0].mask_shares()[mask_index_b]) & kRingMask;
        const auto share_b1 = (input_share1[edge.b] + pools.parties[1].mask_shares()[mask_index_b]) & kRingMask;
        if (((share_b0 + share_b1) & kRingMask) != z_b)
          throw std::runtime_error("test-only masked share reconstruction mismatch");
        const auto out0 = p0.eval_strict_lt(z_a, z_b);
        const auto out1 = p1.eval_strict_lt(z_a, z_b);
        const bool actual = (out0 + out1) == 1;
        const bool expected = priority[edge.a] < priority[edge.b];
        if (actual != expected) throw std::runtime_error("uCMP stable-order oracle mismatch");
        ++online_edges;
        expect_reject("repeated consumption", [&] { (void)pools.parties[0].take(req, digest); });
      }
    }

    const auto local_online_end = std::chrono::steady_clock::now();
    const auto local_online_ms = std::chrono::duration<double, std::milli>(local_online_end - local_online_begin).count();
    expect_reject("truncated serialized key", [&] {
      auto truncated = pools.parties[0].key_bytes(0, 0, 3);
      truncated.pop_back();
      (void)moe_topk::ProtocolIUcmpPartyMaterial::deserialize(truncated);
    });
    if (pools.parties[0].consumed_count() != online_edges ||
        pools.parties[1].consumed_count() != online_edges || online_edges != graph_edges)
      throw std::runtime_error("graph/key accounting mismatch");

    const auto wire0 = pools.parties[0].serialize();
    const auto wire1 = pools.parties[1].serialize();
    const auto capacity = kRounds * (kN * (kN - 1) / 2);
    std::cout << "MATERIAL_POOL_TEST PASS\n"
              << "n=" << kN << " r=" << kRounds << " bits=" << kBits << "\n"
              << "pool_slots_per_party=" << pools.parties[0].count() << " total_both_party_slots="
              << pools.parties[0].count() + pools.parties[1].count() << " logical_slot_capacity=" << capacity << "\n"
              << "active_graph_edges=" << graph_edges << " one_shot_party_consumptions_each=" << online_edges
              << " unused_logical_slots=" << capacity - online_edges << "\n"
              << "serialized_key_bytes_per_party_slot=" << pools.parties[0].key_bytes(0, 0, 1).size() << "\n"
              << "party_pool_wire_bytes_p0=" << wire0.size() << " party_pool_wire_bytes_p1=" << wire1.size()
              << " combined=" << wire0.size() + wire1.size() << "\n"
              << "offline_pool_generation_ms=" << offline_ms << " (deterministic_TEST_ONLY_PRNG)\n"
              << "local_wrapper_and_uCMP_eval_ms=" << local_online_ms << " (single_process_TEST_ONLY; not production online latency)\n"
              << "all_pair_iteration_function_checks=" << all_pair_slot_checks << " (separate_fresh_validation_pool)\n"
              << "active_graph_DCF_eval_calls_by_source=32 exhaustive_slot_DCF_eval_calls_by_source=80\n"
              << "cross_iteration_same_pair_serialized_keys_differ=YES\n"
              << "fail_closed_cases=8_plus_replay_and_truncation\n"
              << "test_only_cleartext_reconstruction=YES\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "MATERIAL_POOL_TEST FAIL: " << error.what() << "\n";
    return 1;
  }
}
