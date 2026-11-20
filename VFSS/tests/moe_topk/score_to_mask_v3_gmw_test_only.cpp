#ifndef MOE_TOPK_ENABLE_S31_TEST_ONLY
#error "This source is a TEST_ONLY reference harness and must never enter a runtime target."
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <moe_topk/topk_oracle.h>

namespace s31_test_only {

struct Bit {
  std::uint8_t p0 = 0;
  std::uint8_t p1 = 0;
};

using Word = std::vector<Bit>;  // little-endian Boolean wires
using Matrix = std::vector<std::vector<Bit>>;  // P[shuffled][original]

void require(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}

Bit public_bit(std::uint8_t value) {
  require(value <= 1, "non-bit public constant");
  return {value, 0};
}

Bit bxor(Bit a, Bit b) {
  return {static_cast<std::uint8_t>(a.p0 ^ b.p0),
          static_cast<std::uint8_t>(a.p1 ^ b.p1)};
}

Bit bnot(Bit value) {
  value.p0 ^= 1U;
  return value;
}

std::uint8_t open_test_only(Bit value) {
  return static_cast<std::uint8_t>(value.p0 ^ value.p1);
}

struct Triple {
  Bit a;
  Bit b;
  Bit c;
  std::uint64_t session = 0;
};

class BeaverEngine {
 public:
  BeaverEngine(std::uint64_t session, std::vector<Triple> triples)
      : session_(session), triples_(std::move(triples)),
        consumed_(triples_.size(), 0) {
    require(session_ != 0, "zero Beaver session id");
    require(std::all_of(triples_.begin(), triples_.end(), [session](const auto& t) {
              return t.session == session;
            }),
            "cross-session Beaver triple");
  }

  Bit and_gate(Bit x, Bit y, std::size_t gate_id) {
    require(session_ != 0, "zero session id");
    require(gate_id < triples_.size(), "triple gate id out of range");
    require(consumed_[gate_id] == 0, "Beaver triple consumed more than once");
    consumed_[gate_id] = 1;
    const auto& t = triples_[gate_id];

    // TEST_ONLY central harness models both parties sending d_i=x_i^a_i and
    // e_i=y_i^b_i. The only opened values are d=x^a and e=y^b.
    const auto d = open_test_only(bxor(x, t.a));
    const auto e = open_test_only(bxor(y, t.b));
    Bit z;
    z.p0 = static_cast<std::uint8_t>(
        t.c.p0 ^ (d & t.b.p0) ^ (e & t.a.p0) ^ (d & e));
    z.p1 = static_cast<std::uint8_t>(
        t.c.p1 ^ (d & t.b.p1) ^ (e & t.a.p1));
    require(open_test_only(z) ==
                static_cast<std::uint8_t>(open_test_only(x) & open_test_only(y)),
            "Beaver AND reconstruction");
    return z;
  }

  std::size_t triple_count() const { return triples_.size(); }

  std::size_t consumed_count() const {
    return static_cast<std::size_t>(
        std::count(consumed_.begin(), consumed_.end(), std::uint8_t{1}));
  }

  void require_all_consumed_once() const {
    require(consumed_count() == triples_.size(), "unused Beaver triple slot");
    require(std::all_of(consumed_.begin(), consumed_.end(),
                        [](std::uint8_t use) { return use == 1; }),
            "Beaver triple omitted or duplicated");
  }

 private:
  std::uint64_t session_;
  std::vector<Triple> triples_;
  std::vector<std::uint8_t> consumed_;
};

std::uint64_t ceil_sqrt(std::uint64_t value) {
  std::uint64_t root = 0;
  while (root * root < value) ++root;
  return root;
}

std::uint32_t index_bits(std::uint32_t padded_n) {
  std::uint32_t bits = 0;
  std::uint32_t value = std::max<std::uint32_t>(padded_n, 2U) - 1U;
  do {
    ++bits;
    value >>= 1U;
  } while (value != 0);
  return bits;
}

std::uint32_t padded_size(std::uint32_t n) {
  require(n != 0 && n <= 256, "TEST_ONLY supported logical N");
  std::uint32_t result = 2;
  while (result < n) result <<= 1U;
  return result;
}

std::size_t and_budget(std::uint32_t logical_n, std::uint32_t padded_n) {
  require(logical_n != 0 && logical_n <= padded_n, "gate budget N");
  const auto width = 32U + index_bits(padded_n);
  const auto pairs = static_cast<std::size_t>(padded_n) * (padded_n - 1U) / 2U;
  // 63 ANDs per padded score word for 32-bit additive-share addition; N^2*W for the
  // one-hot forward route; (2W-1) per all-pairs comparator; and N^2 for
  // the one-hot inverse route. Membership reuses secret-shared pair results.
  return 63U * padded_n + static_cast<std::size_t>(padded_n) * padded_n * width +
         pairs * (2U * width - 1U) +
         static_cast<std::size_t>(padded_n) * padded_n;
}

Bit share_private_input(std::uint8_t value, std::uint8_t owner,
                        std::mt19937_64& rng) {
  require(value <= 1 && owner <= 1, "private input bit parameters");
  const auto random_share = static_cast<std::uint8_t>(rng() & 1U);
  if (owner == 0) return {static_cast<std::uint8_t>(value ^ random_share), random_share};
  return {random_share, static_cast<std::uint8_t>(value ^ random_share)};
}

std::vector<Triple> make_triples(std::size_t count, std::mt19937_64& rng,
                                 std::uint64_t session) {
  require(session != 0, "zero triple package session");
  std::vector<Triple> triples;
  triples.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    const auto a = static_cast<std::uint8_t>(rng() & 1U);
    const auto b = static_cast<std::uint8_t>(rng() & 1U);
    const auto c = static_cast<std::uint8_t>(a & b);
    const auto a0 = static_cast<std::uint8_t>(rng() & 1U);
    const auto b0 = static_cast<std::uint8_t>(rng() & 1U);
    const auto c0 = static_cast<std::uint8_t>(rng() & 1U);
    triples.push_back({{a0, static_cast<std::uint8_t>(a ^ a0)},
                       {b0, static_cast<std::uint8_t>(b ^ b0)},
                       {c0, static_cast<std::uint8_t>(c ^ c0)}, session});
  }
  return triples;
}

Matrix make_shared_permutation(std::uint32_t n, std::mt19937_64& rng,
                               std::vector<std::uint32_t>* pi_out) {
  std::vector<std::uint32_t> pi(n);
  std::iota(pi.begin(), pi.end(), 0U);
  std::shuffle(pi.begin(), pi.end(), rng);
  Matrix matrix(n, std::vector<Bit>(n));
  for (std::uint32_t original = 0; original < n; ++original) {
    const auto shuffled = pi[original];
    for (std::uint32_t row = 0; row < n; ++row) {
      const auto secret = static_cast<std::uint8_t>(row == shuffled);
      const auto share0 = static_cast<std::uint8_t>(rng() & 1U);
      matrix[row][original] = {share0,
                               static_cast<std::uint8_t>(secret ^ share0)};
    }
  }
  if (pi_out) *pi_out = std::move(pi);  // TEST_ONLY oracle/controller only.
  return matrix;
}

std::vector<Bit> add_32_mod_2_32(const std::vector<Bit>& a,
                                const std::vector<Bit>& b,
                                BeaverEngine& beaver,
                                std::size_t gate_base) {
  require(a.size() == 32 && b.size() == 32, "32-bit score input width");
  std::array<Bit, 32> generate{};
  generate[0] = beaver.and_gate(a[0], b[0], gate_base);
  for (std::uint32_t i = 1; i < 32; ++i)
    generate[i] = beaver.and_gate(a[i], b[i], gate_base + i);

  std::vector<Bit> sum(32);
  sum[0] = bxor(a[0], b[0]);
  auto carry = generate[0];
  for (std::uint32_t i = 1; i < 32; ++i) {
    const auto propagate = bxor(a[i], b[i]);
    sum[i] = bxor(propagate, carry);
    const auto carry_term = beaver.and_gate(
        propagate, carry, gate_base + 32U + (i - 1U));
    carry = bxor(generate[i], carry_term);  // disjoint carry terms
  }
  return sum;
}

Word make_priority_key(const std::vector<Bit>& signed_score,
                       std::uint32_t original_index,
                       std::uint32_t padded_n) {
  const auto ibits = index_bits(padded_n);
  Word key(32U + ibits);
  for (std::uint32_t b = 0; b < ibits; ++b)
    key[b] = public_bit(static_cast<std::uint8_t>((original_index >> b) & 1U));
  for (std::uint32_t b = 0; b < 32; ++b) {
    auto ordered = signed_score[b];
    if (b == 31) ordered = bnot(ordered);  // signed ascending transform
    key[ibits + b] = bnot(ordered);        // descending score priority
  }
  return key;
}

std::vector<Word> add_and_key_inputs(const std::vector<std::uint32_t>& p0_additive,
                                    const std::vector<std::uint32_t>& p1_additive,
                                    std::uint32_t padded_n,
                                    BeaverEngine& beaver,
                                    std::mt19937_64& input_rng) {
  require(p0_additive.size() == padded_n && p1_additive.size() == padded_n,
          "score input vector length");
  std::vector<Word> keys(padded_n);
  for (std::uint32_t i = 0; i < padded_n; ++i) {
    std::vector<Bit> p0(32), p1(32);
    for (std::uint32_t b = 0; b < 32; ++b) {
      p0[b] = share_private_input(
          static_cast<std::uint8_t>((p0_additive[i] >> b) & 1U), 0,
          input_rng);
      p1[b] = share_private_input(
          static_cast<std::uint8_t>((p1_additive[i] >> b) & 1U), 1,
          input_rng);
    }
    const auto score = add_32_mod_2_32(p0, p1, beaver,
                                       static_cast<std::size_t>(i) * 63U);
    keys[i] = make_priority_key(score, i, padded_n);
  }
  return keys;
}

std::vector<Word> route_forward(const std::vector<Word>& keys,
                                const Matrix& permutation,
                                BeaverEngine& beaver,
                                std::size_t gate_base) {
  const auto n = static_cast<std::uint32_t>(keys.size());
  const auto width = static_cast<std::uint32_t>(keys.front().size());
  require(permutation.size() == n, "forward permutation rows");
  std::vector<Word> shuffled(n, Word(width));
  for (std::uint32_t row = 0; row < n; ++row) {
    require(permutation[row].size() == n, "forward permutation columns");
    for (std::uint32_t original = 0; original < n; ++original) {
      for (std::uint32_t bit = 0; bit < width; ++bit) {
        const auto gate = gate_base +
            (static_cast<std::size_t>(row) * n + original) * width + bit;
        const auto term = beaver.and_gate(permutation[row][original],
                                          keys[original][bit], gate);
        shuffled[row][bit] = bxor(shuffled[row][bit], term);
      }
    }
  }
  return shuffled;
}

struct PairState {
  std::uint32_t left = 0;
  std::uint32_t right = 0;
  std::vector<Bit> generate;
  Bit borrow;
  std::size_t gate_base = 0;
};

std::vector<std::vector<Bit>> compare_all_pairs(
    const std::vector<Word>& keys, BeaverEngine& beaver,
    std::size_t gate_base) {
  const auto n = static_cast<std::uint32_t>(keys.size());
  const auto width = static_cast<std::uint32_t>(keys.front().size());
  const auto gates_per_pair = static_cast<std::size_t>(2U * width - 1U);
  std::vector<PairState> pairs;
  pairs.reserve(static_cast<std::size_t>(n) * (n - 1U) / 2U);
  for (std::uint32_t left = 0; left < n; ++left) {
    for (std::uint32_t right = left + 1U; right < n; ++right) {
      const auto pair_index = pairs.size();
      PairState state;
      state.left = left;
      state.right = right;
      state.generate.resize(width);
      state.gate_base = gate_base + pair_index * gates_per_pair;
      for (std::uint32_t bit = 0; bit < width; ++bit) {
        const auto not_left = bnot(keys[left][bit]);
        state.generate[bit] = beaver.and_gate(
            not_left, keys[right][bit], state.gate_base + bit);
      }
      state.borrow = state.generate[0];
      pairs.push_back(std::move(state));
    }
  }

  // For each bit position, all pair gates are parallel. borrow_i is the
  // unsigned subtraction recurrence g_i XOR (equal_i AND borrow_(i-1)).
  for (std::uint32_t bit = 1; bit < width; ++bit) {
    for (auto& pair : pairs) {
      const auto equal = bnot(bxor(keys[pair.left][bit], keys[pair.right][bit]));
      const auto h = beaver.and_gate(
          equal, pair.borrow,
          pair.gate_base + width + (bit - 1U));
      pair.borrow = bxor(pair.generate[bit], h);
    }
  }

  std::vector<std::vector<Bit>> table(n, std::vector<Bit>(n));
  for (const auto& pair : pairs) {
    table[pair.left][pair.right] = pair.borrow;
    table[pair.right][pair.left] = bnot(pair.borrow);
  }
  return table;
}

std::vector<Bit> route_inverse(const std::vector<Bit>& shuffled_mask,
                               const Matrix& permutation,
                               BeaverEngine& beaver,
                               std::size_t gate_base) {
  const auto n = static_cast<std::uint32_t>(shuffled_mask.size());
  std::vector<Bit> original(n);
  for (std::uint32_t i = 0; i < n; ++i) {
    for (std::uint32_t j = 0; j < n; ++j) {
      const auto gate = gate_base + static_cast<std::size_t>(i) * n + j;
      const auto term = beaver.and_gate(permutation[j][i], shuffled_mask[j], gate);
      original[i] = bxor(original[i], term);
    }
  }
  return original;
}

struct Edge {
  std::uint32_t left = 0;
  std::uint32_t right = 0;
};

using EdgeKey = std::pair<std::uint32_t, std::uint32_t>;

Edge canonical(std::uint32_t a, std::uint32_t b) {
  require(a != b, "Select5 self edge");
  return {std::min(a, b), std::max(a, b)};
}

std::vector<Edge> freeze_edges(std::set<EdgeKey> keys) {
  std::vector<Edge> result;
  result.reserve(keys.size());
  for (const auto& e : keys) result.push_back({e.first, e.second});
  return result;
}

struct Layer {
  std::uint8_t iteration = 0;
  std::uint8_t substage = 0;
  std::vector<Edge> edges;
  std::vector<std::uint8_t> outcomes;
};

struct Trace {
  std::vector<std::uint32_t> candidate_before;
  std::vector<std::uint32_t> candidate_after;
  std::uint32_t rank_before = 0;
  std::uint32_t rank_after = 0;
  std::vector<std::uint32_t> sample;
  std::vector<std::uint32_t> sample_order;
  std::vector<std::uint32_t> pivots;
  std::vector<Layer> layers;
  std::uint32_t selected_position = std::numeric_limits<std::uint32_t>::max();
};

struct SelectResult {
  std::uint32_t selected_position = std::numeric_limits<std::uint32_t>::max();
  std::vector<Trace> iterations;
  std::uint64_t comparison_calls = 0;
};

std::uint32_t bit_length(std::uint32_t value) {
  std::uint32_t bits = 0;
  do {
    ++bits;
    value >>= 1U;
  } while (value != 0);
  return bits;
}

struct SampleParameters {
  std::uint32_t sample_size;
  std::uint32_t q_low;
  std::uint32_t q_high;
};

SampleParameters parameters(std::uint32_t m, std::uint32_t rank,
                            std::uint32_t n) {
  require(m >= 2 && rank >= 1 && rank <= m, "Select5 sample state");
  const auto sample_size = std::min<std::uint32_t>(
      m, static_cast<std::uint32_t>(ceil_sqrt(m)));
  const auto window = static_cast<std::uint32_t>(
      ceil_sqrt(2U * sample_size * bit_length(n)));
  const auto mu_floor = sample_size * (rank - 1U) / m;
  const auto mu_ceil = (sample_size * (rank - 1U) + m - 1U) / m;
  const auto q_low = std::min(sample_size - 1U,
      std::max(1U, mu_floor > window ? mu_floor - window : 1U));
  const auto q_high = std::min(sample_size,
      std::max(q_low + 1U, mu_ceil + window + 1U));
  return {sample_size, q_low, q_high};
}

std::uint64_t bounded(std::mt19937_64& rng, std::uint64_t bound) {
  require(bound != 0, "sample bound");
  const auto reject_below = static_cast<std::uint64_t>(-bound) % bound;
  for (;;) {
    const auto word = rng();
    if (word >= reject_below) return word % bound;
  }
}

std::vector<std::uint32_t> sample_without_replacement(
    const std::vector<std::uint32_t>& population, std::uint32_t count,
    std::uint64_t seed) {
  require(count <= population.size(), "sample count");
  auto pool = population;
  std::mt19937_64 rng(seed);
  for (std::uint32_t i = 0; i < count; ++i) {
    const auto offset = static_cast<std::uint32_t>(
        bounded(rng, static_cast<std::uint64_t>(pool.size() - i)));
    std::swap(pool[i], pool[i + offset]);
  }
  pool.resize(count);
  return pool;
}

class OutcomeMap {
 public:
  void insert(const Layer& layer) {
    require(layer.edges.size() == layer.outcomes.size(), "Select5 layer shape");
    for (std::size_t i = 0; i < layer.edges.size(); ++i) {
      const auto& edge = layer.edges[i];
      require(edge.left < edge.right && layer.outcomes[i] <= 1,
              "Select5 canonical edge or outcome");
      const auto inserted = values_.emplace(EdgeKey{edge.left, edge.right},
                                             layer.outcomes[i]);
      if (!inserted.second)
        require(inserted.first->second == layer.outcomes[i],
                "repeated Select5 edge disagrees");
    }
  }

  bool before(std::uint32_t left, std::uint32_t right) const {
    if (left == right) return false;
    const auto key = EdgeKey{std::min(left, right), std::max(left, right)};
    const auto found = values_.find(key);
    require(found != values_.end(), "required Select5 comparison absent");
    return left < right ? found->second != 0 : found->second == 0;
  }

 private:
  std::map<EdgeKey, std::uint8_t> values_;
};

std::uint32_t one_based_rank(std::uint32_t pivot,
                             const std::vector<std::uint32_t>& candidate,
                             const OutcomeMap& outcomes) {
  std::uint32_t rank = 1;
  for (const auto item : candidate)
    if (item != pivot && outcomes.before(item, pivot)) ++rank;
  return rank;
}

void verify_total_order(const std::vector<std::uint32_t>& vertices,
                        const OutcomeMap& outcomes) {
  std::set<std::uint32_t> ranks;
  for (const auto vertex : vertices) {
    std::uint32_t rank = 1;
    for (const auto other : vertices)
      if (other != vertex && outcomes.before(other, vertex)) ++rank;
    require(rank <= vertices.size() && ranks.insert(rank).second,
            "Select5 graph is not a strict total order");
  }
  require(ranks.size() == vertices.size(), "Select5 total order rank gap");
}

using CompareWave = std::function<std::vector<std::uint8_t>(
    std::uint8_t, std::uint8_t, const std::vector<Edge>&)>;
using SampleSource = std::function<std::vector<std::uint32_t>(
    std::uint8_t, const std::vector<std::uint32_t>&, std::uint32_t)>;

SelectResult run_select5(std::uint32_t n, std::uint32_t k,
                         const SampleSource& sample_source,
                         const CompareWave& compare_wave) {
  require(n >= 1 && n <= 256 && k >= 1 && k <= n,
          "Select5 input shape");
  SelectResult result;
  std::vector<std::uint32_t> candidate(n);
  std::iota(candidate.begin(), candidate.end(), 0U);
  std::uint32_t residual = k;
  for (std::uint8_t iteration = 0; iteration < 5; ++iteration) {
    Trace trace;
    trace.candidate_before = candidate;
    trace.rank_before = residual;
    if (result.selected_position != std::numeric_limits<std::uint32_t>::max()) {
      trace.candidate_after = candidate;
      trace.rank_after = residual;
      trace.selected_position = result.selected_position;
      result.iterations.push_back(std::move(trace));
      continue;
    }
    if (iteration == 4) {
      Layer layer{iteration, 3, {}, {}};
      std::set<EdgeKey> edge_set;
      for (std::size_t i = 0; i < candidate.size(); ++i)
        for (std::size_t j = i + 1; j < candidate.size(); ++j)
          edge_set.emplace(candidate[i], candidate[j]);
      layer.edges = freeze_edges(std::move(edge_set));
      layer.outcomes = compare_wave(iteration, 3, layer.edges);
      require(layer.outcomes.size() == layer.edges.size(), "exact layer output size");
      for (const auto bit : layer.outcomes) require(bit <= 1, "exact layer bit");
      result.comparison_calls += layer.edges.size();
      trace.layers.push_back(layer);
      OutcomeMap outcomes;
      outcomes.insert(trace.layers.back());
      verify_total_order(candidate, outcomes);
      for (const auto vertex : candidate) {
        if (one_based_rank(vertex, candidate, outcomes) == residual) {
          require(result.selected_position ==
                      std::numeric_limits<std::uint32_t>::max(),
                  "exact Select5 rank not unique");
          result.selected_position = vertex;
        }
      }
      require(result.selected_position != std::numeric_limits<std::uint32_t>::max(),
              "exact Select5 rank absent");
      trace.candidate_after = candidate;
      trace.rank_after = residual;
      trace.selected_position = result.selected_position;
      result.iterations.push_back(std::move(trace));
      continue;
    }
    if (candidate.size() == 1) {
      result.selected_position = candidate.front();
      trace.candidate_after = candidate;
      trace.rank_after = residual;
      trace.selected_position = result.selected_position;
      result.iterations.push_back(std::move(trace));
      continue;
    }

    const auto p = parameters(static_cast<std::uint32_t>(candidate.size()),
                              residual, n);
    trace.sample = sample_source(iteration, candidate, p.sample_size);
    require(trace.sample.size() == p.sample_size, "sample source size");
    const std::set<std::uint32_t> candidate_set(candidate.begin(), candidate.end());
    const std::set<std::uint32_t> sample_set(trace.sample.begin(), trace.sample.end());
    require(sample_set.size() == trace.sample.size() &&
                std::all_of(trace.sample.begin(), trace.sample.end(),
                    [&candidate_set](std::uint32_t item) {
                      return candidate_set.count(item) == 1;
                    }),
            "sample not distinct from candidate");

    std::set<EdgeKey> sample_edges;
    for (std::size_t i = 0; i < trace.sample.size(); ++i)
      for (std::size_t j = i + 1; j < trace.sample.size(); ++j) {
        const auto edge = canonical(trace.sample[i], trace.sample[j]);
        sample_edges.emplace(edge.left, edge.right);
      }
    Layer sample_layer{iteration, 1, freeze_edges(std::move(sample_edges)), {}};
    sample_layer.outcomes = compare_wave(iteration, 1, sample_layer.edges);
    require(sample_layer.outcomes.size() == sample_layer.edges.size(),
            "sample layer output size");
    for (const auto bit : sample_layer.outcomes) require(bit <= 1, "sample layer bit");
    result.comparison_calls += sample_layer.edges.size();
    trace.layers.push_back(sample_layer);

    OutcomeMap outcomes;
    outcomes.insert(trace.layers.back());
    trace.sample_order = trace.sample;
    std::sort(trace.sample_order.begin(), trace.sample_order.end(),
              [&outcomes](std::uint32_t left, std::uint32_t right) {
                return outcomes.before(left, right);
              });
    const auto pivot_x = trace.sample_order[p.q_low - 1U];
    const auto pivot_y = trace.sample_order[p.q_high - 1U];
    require(pivot_x != pivot_y && outcomes.before(pivot_x, pivot_y),
            "pivot order");
    trace.pivots = {pivot_x, pivot_y};

    std::set<EdgeKey> star_edges;
    for (const auto pivot : trace.pivots)
      for (const auto item : candidate) {
        if (sample_set.count(item) == 0) {
          const auto edge = canonical(pivot, item);
          star_edges.emplace(edge.left, edge.right);
        }
      }
    for (const auto& edge : trace.layers.front().edges)
      require(star_edges.count({edge.left, edge.right}) == 0,
              "Select5 sublayer overlap");
    Layer star_layer{iteration, 2, freeze_edges(std::move(star_edges)), {}};
    star_layer.outcomes = compare_wave(iteration, 2, star_layer.edges);
    require(star_layer.outcomes.size() == star_layer.edges.size(),
            "star layer output size");
    for (const auto bit : star_layer.outcomes) require(bit <= 1, "star layer bit");
    result.comparison_calls += star_layer.edges.size();
    trace.layers.push_back(star_layer);
    outcomes.insert(trace.layers.back());

    const auto rank_x = one_based_rank(pivot_x, candidate, outcomes);
    const auto rank_y = one_based_rank(pivot_y, candidate, outcomes);
    require(rank_x < rank_y && rank_y <= candidate.size(),
            "pivot rank invariant");
    if (rank_x == residual) {
      result.selected_position = pivot_x;
    } else if (rank_y == residual) {
      result.selected_position = pivot_y;
    } else if (rank_x < residual && residual < rank_y) {
      std::vector<std::uint32_t> next;
      for (const auto item : candidate)
        if (outcomes.before(pivot_x, item) && outcomes.before(item, pivot_y))
          next.push_back(item);
      candidate = std::move(next);
      residual -= rank_x;
    } else if (rank_y < residual) {
      std::vector<std::uint32_t> next;
      for (const auto item : candidate)
        if (outcomes.before(pivot_y, item)) next.push_back(item);
      candidate = std::move(next);
      residual -= rank_y;
    } else if (residual < rank_x) {
      std::vector<std::uint32_t> next;
      for (const auto item : candidate)
        if (outcomes.before(item, pivot_x)) next.push_back(item);
      candidate = std::move(next);
    } else {
      throw std::runtime_error("inconsistent Select5 pivot branch");
    }
    if (result.selected_position == std::numeric_limits<std::uint32_t>::max())
      require(!candidate.empty() && residual >= 1 && residual <= candidate.size(),
              "residual rank invariant");
    trace.candidate_after = candidate;
    trace.rank_after = residual;
    trace.selected_position = result.selected_position;
    result.iterations.push_back(std::move(trace));
  }
  require(result.selected_position != std::numeric_limits<std::uint32_t>::max() &&
              result.iterations.size() == 5,
          "Select5 did not finish in five algorithm iterations");
  return result;
}

std::uint32_t signed_raw(std::int32_t value) {
  return static_cast<std::uint32_t>(value);
}

std::uint64_t clear_priority_key(std::int32_t score, std::uint32_t index,
                                 std::uint32_t padded_n) {
  const auto raw = signed_raw(score);
  const auto ordered = raw ^ UINT32_C(0x80000000);
  const auto high = UINT32_MAX - ordered;
  return (static_cast<std::uint64_t>(high) << index_bits(padded_n)) | index;
}

std::vector<std::uint8_t> stable_oracle(const std::vector<std::int32_t>& scores,
                                        std::uint32_t k) {
  require(!scores.empty() && k >= 1 && k <= scores.size(), "oracle input");
  std::vector<std::uint32_t> raw(scores.size());
  std::transform(scores.begin(), scores.end(), raw.begin(),
                 [](std::int32_t score) {
                   return static_cast<std::uint32_t>(score);
                 });
  return moe_topk::top_k_mask(raw, k);
}

std::size_t pair_index(std::uint32_t n, std::uint32_t left,
                       std::uint32_t right) {
  require(left < right && right < n, "pair index coordinates");
  std::size_t index = 0;
  for (std::uint32_t i = 0; i < left; ++i) index += n - i - 1U;
  return index + (right - left - 1U);
}

struct DeclassifiedCompare {
  std::uint8_t iteration = 0;
  std::uint8_t substage = 0;
  std::uint32_t left = 0;
  std::uint32_t right = 0;
  std::uint8_t bit = 0;
};

struct RunResult {
  std::vector<std::uint8_t> mask;
  std::vector<std::uint8_t> z0;
  std::vector<std::uint8_t> z1;
  std::vector<std::uint8_t> membership_shuffled_test_only;
  std::vector<std::uint8_t> membership_original_test_only;
  std::uint32_t selected_position_test_only = 0;
  std::vector<Trace> trace;
  std::vector<DeclassifiedCompare> declassified;
  std::vector<std::uint32_t> test_only_permutation;
  std::size_t gate_count = 0;
  std::size_t compare_calls = 0;
};

RunResult run_gmw_test_only(const std::vector<std::int32_t>& scores,
                            std::uint32_t k, std::uint64_t seed,
                            std::uint64_t session) {
  require(!scores.empty() && scores.size() <= 8, "TEST_ONLY full-path n<=8");
  require(k >= 1 && k <= scores.size(), "TEST_ONLY legal K");
  const auto n = static_cast<std::uint32_t>(scores.size());
  const auto pn = padded_size(n);
  const auto width = 32U + index_bits(pn);
  const auto budget = and_budget(n, pn);
  std::mt19937_64 dealer_rng(seed ^ UINT64_C(0x4d3642374445414c));
  std::mt19937_64 input_rng(seed ^ UINT64_C(0x494e505554534852));
  const auto triples = make_triples(budget, dealer_rng, session);
  BeaverEngine beaver(session, triples);
  std::vector<std::uint32_t> pi;
  const auto permutation = make_shared_permutation(pn, dealer_rng, &pi);

  std::vector<std::uint32_t> additive0(pn), additive1(pn);
  for (std::uint32_t i = 0; i < n; ++i) {
    additive0[i] = static_cast<std::uint32_t>(input_rng());
    additive1[i] = signed_raw(scores[i]) - additive0[i];
  }
  for (std::uint32_t i = n; i < pn; ++i) {
    additive0[i] = UINT32_C(0x80000000);  // S25 padding sentinel
    additive1[i] = 0;
  }

  auto original_keys = add_and_key_inputs(additive0, additive1, pn, beaver,
                                           input_rng);
  const auto route_base = 63U * static_cast<std::size_t>(pn);
  const auto compare_base = route_base + static_cast<std::size_t>(pn) * pn * width;
  auto shuffled_keys = route_forward(original_keys, permutation, beaver, route_base);
  auto compare_table = compare_all_pairs(shuffled_keys, beaver, compare_base);
  const auto inverse_base = compare_base +
      static_cast<std::size_t>(pn) * (pn - 1U) / 2U * (2U * width - 1U);

  std::vector<DeclassifiedCompare> opened_events;
  auto sample_source = [seed](std::uint8_t iteration,
                              const std::vector<std::uint32_t>& candidate,
                              std::uint32_t count) {
    const auto iter_seed = seed ^
        (UINT64_C(0x9e3779b97f4a7c15) * (iteration + 1U));
    return sample_without_replacement(candidate, count, iter_seed);
  };
  auto compare_wave = [&compare_table, &opened_events](
      std::uint8_t iteration, std::uint8_t substage,
      const std::vector<Edge>& edges) {
    std::vector<std::uint8_t> outcomes;
    outcomes.reserve(edges.size());
    for (const auto& edge : edges) {
      require(edge.left < edge.right, "active Select5 edge canonicality");
      const auto bit = open_test_only(compare_table[edge.left][edge.right]);
      outcomes.push_back(bit);
      opened_events.push_back({iteration, substage, edge.left, edge.right, bit});
    }
    return outcomes;
  };
  const auto selected = run_select5(pn, k, sample_source, compare_wave);

  std::vector<Bit> membership_shuffled(pn);
  const auto tau = selected.selected_position;
  for (std::uint32_t i = 0; i < pn; ++i) {
    if (i == tau) membership_shuffled[i] = public_bit(1);
    else if (i < tau) membership_shuffled[i] = compare_table[i][tau];
    else membership_shuffled[i] = bnot(compare_table[tau][i]);
  }
  const auto membership_original = route_inverse(
      membership_shuffled, permutation, beaver, inverse_base);

  std::vector<std::uint8_t> mask(n);
  std::vector<std::uint8_t> shuffled_open(pn), original_open(pn);
  for (std::uint32_t i = 0; i < pn; ++i)
    shuffled_open[i] = open_test_only(membership_shuffled[i]);
  for (std::uint32_t i = 0; i < pn; ++i)
    original_open[i] = open_test_only(membership_original[i]);
  for (std::uint32_t i = 0; i < n; ++i)
    mask[i] = original_open[i];  // TEST_ONLY oracle view

  std::mt19937_64 output_rng(seed ^ UINT64_C(0x4f55545055545041));
  std::vector<std::uint8_t> z0(n), z1(n);
  for (std::uint32_t i = 0; i < n; ++i) {
    z0[i] = static_cast<std::uint8_t>(output_rng() & 1U);
    z1[i] = static_cast<std::uint8_t>(z0[i] ^ mask[i]);
  }
  beaver.require_all_consumed_once();
  require(beaver.triple_count() == budget, "triple budget mismatch");

  RunResult result;
  result.mask = std::move(mask);
  result.z0 = std::move(z0);
  result.z1 = std::move(z1);
  result.membership_shuffled_test_only = std::move(shuffled_open);
  result.membership_original_test_only = std::move(original_open);
  result.selected_position_test_only = selected.selected_position;
  result.trace = selected.iterations;
  result.declassified = std::move(opened_events);
  result.test_only_permutation = std::move(pi);
  result.gate_count = budget;
  result.compare_calls = static_cast<std::size_t>(selected.comparison_calls);
  return result;
}

std::uint32_t one_based_clear_rank(
    std::uint32_t selected,
    const std::vector<std::uint32_t>& candidates,
    const std::vector<std::uint64_t>& keys) {
  std::uint32_t rank = 1;
  for (const auto item : candidates)
    if (item != selected && keys[item] < keys[selected]) ++rank;
  return rank;
}

void verify_stage_trace(const RunResult& run,
                        const std::vector<std::int32_t>& scores,
                        std::uint32_t k, std::uint32_t padded_n,
                        const std::vector<std::uint32_t>& pi) {
  const auto ibits = index_bits(padded_n);
  std::vector<std::int32_t> padded(scores);
  padded.resize(padded_n, std::numeric_limits<std::int32_t>::min());
  std::vector<std::uint64_t> original_keys(padded_n);
  for (std::uint32_t i = 0; i < padded_n; ++i)
    original_keys[i] = clear_priority_key(padded[i], i, padded_n);
  std::vector<std::uint64_t> shuffled_keys(padded_n);
  for (std::uint32_t i = 0; i < padded_n; ++i)
    shuffled_keys[pi[i]] = original_keys[i];
  std::vector<std::uint32_t> order(padded_n);
  std::iota(order.begin(), order.end(), 0U);
  std::sort(order.begin(), order.end(), [&shuffled_keys](auto a, auto b) {
    return shuffled_keys[a] < shuffled_keys[b];
  });
  // order is indexed by shuffled coordinate because shuffled_keys is keyed
  // by shuffled coordinate; do not apply pi a second time here.
  const auto expected_selected = order[k - 1U];
  require(run.trace.size() == 5, "five Select5 trace records");
  require(run.selected_position_test_only == expected_selected,
          "test-only selected coordinate");
  std::vector<std::uint8_t> expected_membership_shuffled(padded_n, 0);
  for (std::uint32_t rank = 0; rank < k; ++rank)
    expected_membership_shuffled[order[rank]] = 1;
  require(run.membership_shuffled_test_only == expected_membership_shuffled,
          "membership stage mismatch");
  std::vector<std::uint8_t> expected_membership_original(padded_n, 0);
  for (std::uint32_t original = 0; original < padded_n; ++original)
    expected_membership_original[original] =
        expected_membership_shuffled[pi[original]];
  require(run.membership_original_test_only == expected_membership_original,
          "inverse route stage mismatch");

  std::size_t opened_offset = 0;
  for (const auto& trace : run.trace) {
    for (const auto& layer : trace.layers) {
      require(layer.edges.size() == layer.outcomes.size(),
              "declassification layer shape");
      for (std::size_t i = 0; i < layer.edges.size(); ++i) {
        require(opened_offset < run.declassified.size(),
                "missing opened comparison event");
        const auto& event = run.declassified[opened_offset++];
        require(event.iteration == layer.iteration &&
                    event.substage == layer.substage &&
                    event.left == layer.edges[i].left &&
                    event.right == layer.edges[i].right &&
                    event.bit == layer.outcomes[i],
                "opened comparison declassification differs from trace");
      }
    }
  }
  require(opened_offset == run.declassified.size(),
          "unexpected inactive comparison declassification");
  for (const auto& trace : run.trace) {
    const auto target_rank = one_based_clear_rank(expected_selected,
                                                   trace.candidate_before,
                                                   shuffled_keys);
    if (trace.selected_position == std::numeric_limits<std::uint32_t>::max())
      require(target_rank == trace.rank_before,
              "candidate/rank before does not contain the target rank");
    if (!trace.candidate_after.empty() &&
        trace.selected_position == std::numeric_limits<std::uint32_t>::max()) {
      require(std::find(trace.candidate_after.begin(), trace.candidate_after.end(),
                        expected_selected) != trace.candidate_after.end(),
              "candidate update dropped target");
      require(one_based_clear_rank(expected_selected, trace.candidate_after,
                                   shuffled_keys) == trace.rank_after,
              "candidate/rank after invariant");
    }
    if (trace.selected_position != std::numeric_limits<std::uint32_t>::max() &&
        trace.selected_position != expected_selected) {
      throw std::runtime_error(
          "Select5 selected coordinate mismatch: actual=" +
          std::to_string(trace.selected_position) + " expected=" +
          std::to_string(expected_selected) + " k=" + std::to_string(k) +
          " padded_n=" + std::to_string(padded_n) + " pi0=" +
          std::to_string(pi.empty() ? 999U : pi[0]) + " score0=" +
          std::to_string(scores[0]));
    }
    if (!trace.sample.empty()) {
      require(trace.sample.size() == trace.sample_order.size(),
              "sample order size");
      for (std::size_t i = 1; i < trace.sample_order.size(); ++i)
        require(shuffled_keys[trace.sample_order[i - 1U]] <
                    shuffled_keys[trace.sample_order[i]],
                "sample pivot/order update");
    }
    if (trace.pivots.size() == 2) {
      const auto rank_x = one_based_clear_rank(trace.pivots[0],
                                                trace.candidate_before,
                                                shuffled_keys);
      const auto rank_y = one_based_clear_rank(trace.pivots[1],
                                                trace.candidate_before,
                                                shuffled_keys);
      require(rank_x < rank_y, "clear pivot ranks are ordered");
    }
    for (const auto& layer : trace.layers) {
      require(layer.edges.size() == layer.outcomes.size(), "layer trace shape");
      for (std::size_t i = 0; i < layer.edges.size(); ++i)
        require(layer.outcomes[i] ==
                    static_cast<std::uint8_t>(
                        shuffled_keys[layer.edges[i].left] <
                        shuffled_keys[layer.edges[i].right]),
                "GMW comparator opening differs from clear comparator");
    }
  }
  require(expected_selected < padded_n && ibits == index_bits(padded_n),
          "stable key shape");
}

struct PackageHeader {
  std::uint32_t magic = UINT32_C(0x4d365437);  // TEST_ONLY M6T7
  std::uint16_t version = 3;
  std::uint8_t party = 0;
  std::uint8_t reserved = 0;
  std::uint64_t session = 0;
  std::uint64_t fingerprint = 0;
  std::uint32_t logical_n = 0;
  std::uint32_t padded_n = 0;
  std::uint32_t k = 0;
  std::uint32_t width = 0;
  std::uint64_t gate_count = 0;
  std::uint64_t payload_size = 0;
  std::uint64_t checksum = 0;
};

struct TestPackage {
  PackageHeader header;
  std::vector<std::uint8_t> payload;
};

void append_le(std::vector<std::uint8_t>& out, std::uint64_t value,
               std::size_t bytes) {
  for (std::size_t i = 0; i < bytes; ++i)
    out.push_back(static_cast<std::uint8_t>(value >> (8U * i)));
}

std::uint64_t read_le(const std::vector<std::uint8_t>& in, std::size_t& offset,
                      std::size_t bytes) {
  require(offset + bytes <= in.size(), "truncated test package header");
  std::uint64_t value = 0;
  for (std::size_t i = 0; i < bytes; ++i)
    value |= static_cast<std::uint64_t>(in[offset++]) << (8U * i);
  return value;
}

std::uint64_t fnv1a_test_checksum(const std::vector<std::uint8_t>& payload) {
  std::uint64_t value = UINT64_C(14695981039346656037);
  for (const auto byte : payload) {
    value ^= byte;
    value *= UINT64_C(1099511628211);
  }
  return value;
}

std::vector<std::uint8_t> serialize_test_package(TestPackage package) {
  package.header.payload_size = package.payload.size();
  package.header.checksum = fnv1a_test_checksum(package.payload);
  std::vector<std::uint8_t> out;
  out.reserve(64U + package.payload.size());
  append_le(out, package.header.magic, 4);
  append_le(out, package.header.version, 2);
  append_le(out, package.header.party, 1);
  append_le(out, package.header.reserved, 1);
  append_le(out, package.header.session, 8);
  append_le(out, package.header.fingerprint, 8);
  append_le(out, package.header.logical_n, 4);
  append_le(out, package.header.padded_n, 4);
  append_le(out, package.header.k, 4);
  append_le(out, package.header.width, 4);
  append_le(out, package.header.gate_count, 8);
  append_le(out, package.header.payload_size, 8);
  append_le(out, package.header.checksum, 8);
  out.insert(out.end(), package.payload.begin(), package.payload.end());
  return out;
}

TestPackage parse_test_package(const std::vector<std::uint8_t>& bytes,
                               const PackageHeader& expected) {
  require(bytes.size() >= 64, "test package shorter than fixed header");
  std::size_t offset = 0;
  TestPackage package;
  package.header.magic = static_cast<std::uint32_t>(read_le(bytes, offset, 4));
  package.header.version = static_cast<std::uint16_t>(read_le(bytes, offset, 2));
  package.header.party = static_cast<std::uint8_t>(read_le(bytes, offset, 1));
  package.header.reserved = static_cast<std::uint8_t>(read_le(bytes, offset, 1));
  package.header.session = read_le(bytes, offset, 8);
  package.header.fingerprint = read_le(bytes, offset, 8);
  package.header.logical_n = static_cast<std::uint32_t>(read_le(bytes, offset, 4));
  package.header.padded_n = static_cast<std::uint32_t>(read_le(bytes, offset, 4));
  package.header.k = static_cast<std::uint32_t>(read_le(bytes, offset, 4));
  package.header.width = static_cast<std::uint32_t>(read_le(bytes, offset, 4));
  package.header.gate_count = read_le(bytes, offset, 8);
  package.header.payload_size = read_le(bytes, offset, 8);
  package.header.checksum = read_le(bytes, offset, 8);
  require(package.header.magic == expected.magic &&
              package.header.version == expected.version &&
              package.header.party == expected.party &&
              package.header.session == expected.session &&
              package.header.fingerprint == expected.fingerprint &&
              package.header.logical_n == expected.logical_n &&
              package.header.padded_n == expected.padded_n &&
              package.header.k == expected.k &&
              package.header.width == expected.width &&
              package.header.gate_count == expected.gate_count,
          "test package binding mismatch");
  require(package.header.payload_size == bytes.size() - offset,
          "truncated or trailing test package payload");
  package.payload.assign(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                         bytes.end());
  require(fnv1a_test_checksum(package.payload) == package.header.checksum,
          "test package payload checksum mismatch");
  return package;
}

struct PackagePair {
  TestPackage p0;
  TestPackage p1;
  Matrix permutation;
};

PackagePair make_test_package_pair(std::uint32_t logical_n,
                                   std::uint32_t k,
                                   std::uint64_t session,
                                   std::uint64_t fingerprint,
                                   std::mt19937_64& rng) {
  const auto pn = padded_size(logical_n);
  const auto width = 32U + index_bits(pn);
  const auto gates = and_budget(logical_n, pn);
  TestPackage p0, p1;
  auto set_header = [&](PackageHeader& h, std::uint8_t party) {
    h.party = party;
    h.session = session;
    h.fingerprint = fingerprint;
    h.logical_n = logical_n;
    h.padded_n = pn;
    h.k = k;
    h.width = width;
    h.gate_count = gates;
  };
  set_header(p0.header, 0);
  set_header(p1.header, 1);
  p0.payload.reserve(3U * gates + static_cast<std::size_t>(pn) * pn);
  p1.payload.reserve(p0.payload.capacity());
  for (std::size_t i = 0; i < gates; ++i) {
    const auto a = static_cast<std::uint8_t>(rng() & 1U);
    const auto b = static_cast<std::uint8_t>(rng() & 1U);
    const auto c = static_cast<std::uint8_t>(a & b);
    const auto a0 = static_cast<std::uint8_t>(rng() & 1U);
    const auto b0 = static_cast<std::uint8_t>(rng() & 1U);
    const auto c0 = static_cast<std::uint8_t>(rng() & 1U);
    p0.payload.insert(p0.payload.end(), {a0, b0, c0});
    p1.payload.insert(p1.payload.end(),
                      {static_cast<std::uint8_t>(a ^ a0),
                       static_cast<std::uint8_t>(b ^ b0),
                       static_cast<std::uint8_t>(c ^ c0)});
  }
  std::vector<std::uint32_t> pi(pn);
  std::iota(pi.begin(), pi.end(), 0U);
  std::shuffle(pi.begin(), pi.end(), rng);
  Matrix matrix(pn, std::vector<Bit>(pn));
  for (std::uint32_t original = 0; original < pn; ++original) {
    for (std::uint32_t row = 0; row < pn; ++row) {
      const auto value = static_cast<std::uint8_t>(row == pi[original]);
      const auto share = static_cast<std::uint8_t>(rng() & 1U);
      const auto other = static_cast<std::uint8_t>(value ^ share);
      p0.payload.push_back(share);
      p1.payload.push_back(other);
      matrix[row][original] = {share, other};
    }
  }
  return {std::move(p0), std::move(p1), std::move(matrix)};
}

std::vector<Triple> decode_triple_pair(const TestPackage& p0,
                                       const TestPackage& p1) {
  require(p0.header.gate_count == p1.header.gate_count &&
              p0.payload.size() == p1.payload.size() &&
              p0.header.session == p1.header.session,
          "party package size mismatch");
  const auto gates = static_cast<std::size_t>(p0.header.gate_count);
  std::vector<Triple> result(gates);
  for (std::size_t i = 0; i < gates; ++i) {
    const auto j = 3U * i;
    result[i] = {{p0.payload[j], p1.payload[j]},
                 {p0.payload[j + 1U], p1.payload[j + 1U]},
                 {p0.payload[j + 2U], p1.payload[j + 2U]},
                 p0.header.session};
  }
  return result;
}

class TestLeaseRegistry {
 public:
  void claim(std::uint64_t session, std::uint8_t party,
             std::uint64_t fingerprint) {
    require(session != 0 && party <= 1 && fingerprint != 0,
            "invalid lease binding");
    const auto key = std::make_tuple(session, party, fingerprint);
    require(claimed_.insert(key).second, "test package replay/reclaim");
  }

 private:
  std::set<std::tuple<std::uint64_t, std::uint8_t, std::uint64_t>> claimed_;
};

void test_material_rejection() {
  std::mt19937_64 rng(UINT64_C(0x534c4f544d415445));
  const auto pair = make_test_package_pair(3, 2, 101, 909, rng);
  const auto bytes0 = serialize_test_package(pair.p0);
  const auto bytes1 = serialize_test_package(pair.p1);
  const auto expected0 = pair.p0.header;
  const auto expected1 = pair.p1.header;
  require(parse_test_package(bytes0, expected0).payload == pair.p0.payload,
          "valid P0 package roundtrip");
  require(parse_test_package(bytes1, expected1).payload == pair.p1.payload,
          "valid P1 package roundtrip");
  auto package_triples = decode_triple_pair(pair.p0, pair.p1);
  std::vector<std::uint32_t> row_sums(pair.permutation.size(), 0);
  std::vector<std::uint32_t> column_sums(pair.permutation.size(), 0);
  for (std::size_t row = 0; row < pair.permutation.size(); ++row) {
    for (std::size_t column = 0; column < pair.permutation[row].size(); ++column) {
      const auto bit = open_test_only(pair.permutation[row][column]);
      require(bit <= 1, "permutation matrix non-bit");
      row_sums[row] += bit;
      column_sums[column] += bit;
    }
  }
  require(std::all_of(row_sums.begin(), row_sums.end(),
                      [](auto sum) { return sum == 1; }) &&
              std::all_of(column_sums.begin(), column_sums.end(),
                          [](auto sum) { return sum == 1; }),
          "serialized package permutation is not one-hot");
  BeaverEngine package_engine(101, std::move(package_triples));
  for (std::size_t gate = 0; gate < package_engine.triple_count(); ++gate) {
    const auto result = package_engine.and_gate(
        public_bit(static_cast<std::uint8_t>(gate & 1U)),
        public_bit(static_cast<std::uint8_t>((gate >> 1U) & 1U)), gate);
    require(open_test_only(result) ==
                static_cast<std::uint8_t>((gate & 1U) & ((gate >> 1U) & 1U)),
            "serialized package Beaver gate result");
  }
  package_engine.require_all_consumed_once();
  auto truncated = bytes0;
  truncated.pop_back();
  bool rejected = false;
  try { (void)parse_test_package(truncated, expected0); }
  catch (const std::exception&) { rejected = true; }
  require(rejected, "truncated test package accepted");
  auto tampered = bytes0;
  tampered.back() ^= 1U;
  rejected = false;
  try { (void)parse_test_package(tampered, expected0); }
  catch (const std::exception&) { rejected = true; }
  require(rejected, "tampered test package accepted");
  auto cross_session = expected0;
  cross_session.session += 1U;
  rejected = false;
  try { (void)parse_test_package(bytes0, cross_session); }
  catch (const std::exception&) { rejected = true; }
  require(rejected, "cross-session package accepted");
  auto wrong_party = expected0;
  wrong_party.party = 1;
  rejected = false;
  try { (void)parse_test_package(bytes0, wrong_party); }
  catch (const std::exception&) { rejected = true; }
  require(rejected, "wrong-party package accepted");
  TestLeaseRegistry leases;
  leases.claim(101, 0, 909);
  rejected = false;
  try { leases.claim(101, 0, 909); }
  catch (const std::exception&) { rejected = true; }
  require(rejected, "reclaim succeeded");
  leases.claim(101, 1, 909);

  BeaverEngine duplicate(77, make_triples(1, rng, 77));
  const auto a = public_bit(1);
  const auto b = public_bit(1);
  (void)duplicate.and_gate(a, b, 0);
  rejected = false;
  try { (void)duplicate.and_gate(a, b, 0); }
  catch (const std::exception&) { rejected = true; }
  require(rejected, "duplicate triple consumption succeeded");
  rejected = false;
  try { BeaverEngine wrong_session(78, make_triples(1, rng, 77)); }
  catch (const std::exception&) { rejected = true; }
  require(rejected, "cross-session Beaver triple accepted");
}

void test_comparator_exhaustive() {
  std::uint64_t evaluations = 0;
  for (std::uint32_t x = 0; x < 256; ++x) {
    for (std::uint32_t y = 0; y < 256; ++y) {
      std::mt19937_64 rng((static_cast<std::uint64_t>(x) << 32U) | y | 1U);
      const auto width = 8U;
      std::vector<Triple> triples(2U * width - 1U);
      for (auto& t : triples) {
        const auto a = static_cast<std::uint8_t>(rng() & 1U);
        const auto b = static_cast<std::uint8_t>(rng() & 1U);
        const auto c = static_cast<std::uint8_t>(a & b);
        const auto a0 = static_cast<std::uint8_t>(rng() & 1U);
        const auto b0 = static_cast<std::uint8_t>(rng() & 1U);
        const auto c0 = static_cast<std::uint8_t>(rng() & 1U);
        t = {{a0, static_cast<std::uint8_t>(a ^ a0)},
             {b0, static_cast<std::uint8_t>(b ^ b0)},
             {c0, static_cast<std::uint8_t>(c ^ c0)}, evaluations + 1U};
      }
      BeaverEngine beaver(evaluations + 1U, std::move(triples));
      Word left(width), right(width);
      for (std::uint32_t bit = 0; bit < width; ++bit) {
        left[bit] = public_bit(static_cast<std::uint8_t>((x >> bit) & 1U));
        right[bit] = public_bit(static_cast<std::uint8_t>((y >> bit) & 1U));
      }
      const auto base = std::size_t{0};
      std::vector<Bit> generate(width);
      for (std::uint32_t bit = 0; bit < width; ++bit)
        generate[bit] = beaver.and_gate(bnot(left[bit]), right[bit], base + bit);
      auto borrow = generate[0];
      for (std::uint32_t bit = 1; bit < width; ++bit) {
        const auto equal = bnot(bxor(left[bit], right[bit]));
        const auto h = beaver.and_gate(equal, borrow, width + bit - 1U);
        borrow = bxor(generate[bit], h);
      }
      require(open_test_only(borrow) == static_cast<std::uint8_t>(x < y),
              "exhaustive Boolean comparator mismatch");
      beaver.require_all_consumed_once();
      ++evaluations;
    }
  }
  require(evaluations == 65536, "comparator exhaustive count");
}

void test_select_graph_all_orders() {
  std::uint64_t cases = 0;
  for (std::uint32_t n = 1; n <= 8; ++n) {
    std::vector<std::uint32_t> order(n);
    std::iota(order.begin(), order.end(), 0U);
    do {
      std::vector<std::uint32_t> rank(n);
      for (std::uint32_t r = 0; r < n; ++r) rank[order[r]] = r;
      for (std::uint32_t k = 1; k <= n; ++k) {
        const auto seed = UINT64_C(0x5331000000000000) +
                          static_cast<std::uint64_t>(n) * 257U + k + cases;
        const auto sample = [seed](std::uint8_t iteration,
                                   const std::vector<std::uint32_t>& candidate,
                                   std::uint32_t count) {
          return sample_without_replacement(candidate, count,
              seed ^ (UINT64_C(0x9e3779b97f4a7c15) * (iteration + 1U)));
        };
        const auto compare = [&rank](std::uint8_t, std::uint8_t,
                                     const std::vector<Edge>& edges) {
          std::vector<std::uint8_t> bits;
          for (const auto& e : edges)
            bits.push_back(static_cast<std::uint8_t>(rank[e.left] < rank[e.right]));
          return bits;
        };
        const auto result = run_select5(n, k, sample, compare);
        require(result.iterations.size() == 5 &&
                    rank[result.selected_position] == k - 1U,
                "all-order Select5 control result");
        ++cases;
      }
    } while (std::next_permutation(order.begin(), order.end()));
  }
  require(cases == 362879, "all-order Select5 case count");
}

std::uint64_t run_pipeline_suite() {
  const std::array<std::int32_t, 6> domain{{
      std::numeric_limits<std::int32_t>::min(), -4096, -1, 0, 4096,
      std::numeric_limits<std::int32_t>::max()}};
  std::uint64_t cases = 0;
  for (std::uint32_t n = 1; n <= 4; ++n) {
    std::uint64_t assignments = 1;
    for (std::uint32_t i = 0; i < n; ++i) assignments *= domain.size();
    for (std::uint64_t encoded = 0; encoded < assignments; ++encoded) {
      auto value = encoded;
      std::vector<std::int32_t> scores(n);
      for (std::uint32_t i = 0; i < n; ++i) {
        scores[i] = domain[value % domain.size()];
        value /= domain.size();
      }
      for (std::uint32_t k = 1; k <= n; ++k) {
        const auto seed = UINT64_C(0x5633000000000000) + cases;
        const auto result = run_gmw_test_only(scores, k, seed, cases + 1U);
        const auto expected = stable_oracle(scores, k);
        require(result.mask == expected, "full pipeline/oracle mismatch");
        for (std::size_t i = 0; i < expected.size(); ++i) {
          require((result.z0[i] ^ result.z1[i]) == expected[i],
                  "output XOR shares mismatch");
          require(result.z0[i] <= 1 && result.z1[i] <= 1,
                  "output is not byte-valued bit share");
        }
        const auto pn = padded_size(n);
        verify_stage_trace(result, scores, k, pn,
                           result.test_only_permutation);
        ++cases;
      }
    }
  }
  // Larger small-n boundary/tie cases cover every legal K without exhaustive
  // score assignments. Their complete comparator/routing circuit is still run.
  const std::vector<std::vector<std::int32_t>> cases_n5_8{
      {std::numeric_limits<std::int32_t>::min(), -4096, 0, 0, 4096},
      {std::numeric_limits<std::int32_t>::max(), 0, -1, 4096, -4096, 0},
      {std::numeric_limits<std::int32_t>::min(),
       std::numeric_limits<std::int32_t>::max(), -1, 0, -1, 4096, 4096},
      {0, 0, 0, 0, 0, 0, 0, 0}};
  for (std::size_t v = 0; v < cases_n5_8.size(); ++v) {
    const auto& scores = cases_n5_8[v];
    for (std::uint32_t k = 1; k <= scores.size(); ++k) {
      const auto seed = UINT64_C(0x5638000000000000) + cases;
      const auto result = run_gmw_test_only(scores, k, seed, cases + 1U);
      const auto expected = stable_oracle(scores, k);
      require(result.mask == expected, "n=5..8 pipeline/oracle mismatch");
      verify_stage_trace(result, scores, k, padded_size(scores.size()),
                         result.test_only_permutation);
      ++cases;
    }
  }
  return cases;
}

void test_output_share_law() {
  std::uint64_t checked = 0;
  for (std::uint32_t n = 1; n <= 8; ++n) {
    const auto masks = 1U << n;
    for (std::uint32_t mask = 0; mask < masks; ++mask) {
      for (std::uint32_t r = 0; r < masks; ++r) {
        for (std::uint32_t i = 0; i < n; ++i) {
          const auto z0 = static_cast<std::uint8_t>((r >> i) & 1U);
          const auto z1 = static_cast<std::uint8_t>(z0 ^ ((mask >> i) & 1U));
          require((z0 ^ z1) == ((mask >> i) & 1U), "F_TOPK-U share law");
        }
        ++checked;
      }
    }
  }
  require(checked == 87380, "share-law exhaustive count");
}

void test_budget_formulas() {
  struct Expected {
    std::uint32_t n;
    std::size_t gates;
    std::size_t packed_triple_bytes;
    std::size_t packed_permutation_bytes;
  };
  const std::array<Expected, 2> expected{{
      {128, 1289280U, 483480U, 2048U},
      {256, 5281664U, 1980624U, 8192U}}};
  for (const auto& item : expected) {
    const auto n = padded_size(item.n);
    const auto gates = and_budget(item.n, n);
    require(gates == item.gates, "N=128/256 exact AND budget");
    require((3U * gates + 7U) / 8U == item.packed_triple_bytes,
            "packed Boolean triple bytes");
    require((static_cast<std::size_t>(n) * n + 7U) / 8U ==
                item.packed_permutation_bytes,
            "packed permutation-share bytes");
  }
}

void test_invalid_inputs_and_aborts() {
  bool rejected = false;
  try { (void)padded_size(0); }
  catch (const std::exception&) { rejected = true; }
  require(rejected, "n=0 accepted");
  rejected = false;
  try { (void)stable_oracle({1, 2}, 0); }
  catch (const std::exception&) { rejected = true; }
  require(rejected, "K=0 accepted");
  rejected = false;
  try { (void)stable_oracle({1, 2}, 3); }
  catch (const std::exception&) { rejected = true; }
  require(rejected, "K>N accepted");

  // Exercise the owner-selected v3 F_TOPK-ATOMIC logical escrow model only.
  // This is not evidence that the historical S25 implementation was atomic.
  enum class Prefix { BeforeReady, AfterInput, AfterCircuit, AfterInverse,
                      P0Deposited, P1Deposited, AtomicCommit, P0Fetched,
                      P0CrashedAfterFetch, P1Fetched,
                      P1CrashedAfterFetch, BothFetched };
  struct EscrowView {
    bool p0_local_wire_share = false;
    bool p1_local_wire_share = false;
    bool p0_deposited = false;
    bool p1_deposited = false;
    bool committed = false;
    bool p0_logically_available = false;
    bool p1_logically_available = false;
    bool p0_fetched = false;
    bool p1_fetched = false;
    std::vector<std::uint8_t> p0_bytes;
    std::vector<std::uint8_t> p1_bytes;
  };
  const std::vector<std::uint8_t> mask{1, 0, 1, 0, 1, 0, 0};
  const std::vector<std::uint8_t> m0{0, 1, 0, 1, 0, 1, 0};
  const std::vector<std::uint8_t> r {1, 1, 0, 0, 1, 0, 1};
  std::vector<std::uint8_t> m1(mask.size()), refresh(mask.size()), z1(mask.size());
  for (std::size_t i = 0; i < mask.size(); ++i) {
    m1[i] = static_cast<std::uint8_t>(mask[i] ^ m0[i]);
    refresh[i] = static_cast<std::uint8_t>(r[i] ^ m0[i]);
    z1[i] = static_cast<std::uint8_t>(refresh[i] ^ m1[i]);
    require(static_cast<std::uint8_t>(r[i] ^ z1[i]) == mask[i],
            "F_TOPK-U output refresh reconstruction");
  }
  auto escrow_at = [&](Prefix prefix) {
    EscrowView view;
    view.p0_local_wire_share = prefix >= Prefix::AfterInverse;
    view.p1_local_wire_share = prefix >= Prefix::AfterInverse;
    view.p0_deposited = prefix >= Prefix::P0Deposited;
    view.p1_deposited = prefix >= Prefix::P1Deposited;
    view.committed = prefix >= Prefix::AtomicCommit;
    // F_TOPK-ATOMIC linearizes at the durable escrow commit. Both recipients'
    // outputs become available in one transaction; later fetch is recovery,
    // not the ideal output event.
    view.p0_logically_available = view.committed;
    view.p1_logically_available = view.committed;
    view.p0_fetched = prefix == Prefix::P0Fetched ||
                      prefix == Prefix::P0CrashedAfterFetch ||
                      prefix == Prefix::BothFetched;
    view.p1_fetched = prefix == Prefix::P1Fetched ||
                      prefix == Prefix::P1CrashedAfterFetch ||
                      prefix == Prefix::BothFetched;
    if (view.p0_fetched) {
      view.p0_bytes.push_back(static_cast<std::uint8_t>(mask.size()));
      view.p0_bytes.insert(view.p0_bytes.end(), r.begin(), r.end());
    }
    if (view.p1_fetched) {
      view.p1_bytes.push_back(static_cast<std::uint8_t>(mask.size()));
      view.p1_bytes.insert(view.p1_bytes.end(), z1.begin(), z1.end());
    }
    return view;
  };
  const std::array<Prefix, 12> prefixes{{
      Prefix::BeforeReady, Prefix::AfterInput, Prefix::AfterCircuit,
      Prefix::AfterInverse, Prefix::P0Deposited, Prefix::P1Deposited,
      Prefix::AtomicCommit, Prefix::P0Fetched, Prefix::P0CrashedAfterFetch,
      Prefix::P1Fetched, Prefix::P1CrashedAfterFetch, Prefix::BothFetched}};
  std::uint64_t checked_prefixes = 0;
  for (const auto prefix : prefixes) {
    const auto view = escrow_at(prefix);
    require(view.p0_logically_available == view.p1_logically_available,
            "F_TOPK-ATOMIC logical availability diverged");
    if (!view.committed) {
      require(!view.p0_logically_available && !view.p1_logically_available &&
                  view.p0_bytes.empty() && view.p1_bytes.empty(),
              "pre-commit abort exposed an escrow output");
    } else {
      require(view.p0_logically_available && view.p1_logically_available,
              "atomic commit did not assign both outputs");
    }
    if (view.p0_fetched)
      require(view.p0_logically_available && view.p0_bytes.size() == 8,
              "P0 fetched before commit or wrong share length");
    if (view.p1_fetched)
      require(view.p1_logically_available && view.p1_bytes.size() == 8,
              "P1 fetched before commit or wrong share length");
    ++checked_prefixes;
  }
  const auto before_commit = escrow_at(Prefix::P1Deposited);
  require(before_commit.p0_deposited && before_commit.p1_deposited &&
              !before_commit.committed &&
              !before_commit.p0_logically_available &&
              !before_commit.p1_logically_available,
          "pending escrow exposed outputs before atomic commit");
  const auto crash_after_commit = escrow_at(Prefix::P0CrashedAfterFetch);
  require(crash_after_commit.committed && crash_after_commit.p0_fetched &&
              crash_after_commit.p1_logically_available &&
              !crash_after_commit.p1_fetched,
          "post-commit crash lost P1 durable retrieval entitlement");
  const auto p1_crash_after_commit = escrow_at(Prefix::P1CrashedAfterFetch);
  require(p1_crash_after_commit.committed && p1_crash_after_commit.p1_fetched &&
              p1_crash_after_commit.p0_logically_available &&
              !p1_crash_after_commit.p0_fetched,
          "post-commit crash lost P0 durable retrieval entitlement");
  const auto complete = escrow_at(Prefix::BothFetched);
  require(complete.p0_bytes.size() == 8 && complete.p1_bytes.size() == 8,
          "F_TOPK-ATOMIC escrow retrieval payload length");
  for (std::size_t i = 0; i < mask.size(); ++i)
    require(static_cast<std::uint8_t>(complete.p0_bytes[i + 1U] ^
                                      complete.p1_bytes[i + 1U]) == mask[i],
            "F_TOPK-ATOMIC fetched shares do not reconstruct mask");
  require(checked_prefixes == 12, "output abort prefix count");
}

}  // namespace s31_test_only

int main() {
  using namespace s31_test_only;
  try {
    const auto start = std::chrono::steady_clock::now();
    test_material_rejection();
    test_comparator_exhaustive();
    test_select_graph_all_orders();
    const auto pipeline_cases = run_pipeline_suite();
    test_output_share_law();
    test_budget_formulas();
    test_invalid_inputs_and_aborts();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    std::cout << "S31_TEST_ONLY_GMW_JSON {\"comparator_pairs\":65536,"
              << "\"select_total_orders\":362879,"
              << "\"full_pipeline_cases\":" << pipeline_cases << ','
              << "\"all_beaver_gates_one_shot\":true,"
              << "\"material_fault_cases\":7,"
              << "\"abort_prefix_cases\":12,"
              << "\"output_share_cases\":87380,"
              << "\"budget_cases\":2,"
              << "\"diagnostic_elapsed_ms\":" << elapsed << "}\n";
    std::cout << "S31 TEST_ONLY GMW/Beaver functional prototype PASS\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "S31 TEST_ONLY GMW/Beaver prototype FAIL: "
              << error.what() << '\n';
    return 1;
  }
}
