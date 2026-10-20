#include <moe_topk/project_derived_select5.h>

#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <random>
#include <set>
#include <stdexcept>
#include <tuple>

namespace moe_topk {
namespace {

using EdgeKey = std::pair<std::uint32_t, std::uint32_t>;
constexpr std::uint32_t kPublicSampleSeedMagic = UINT32_C(0x53355352);  // S5SR
constexpr std::uint32_t kPublicSampleSeedContractVersion = 3;
constexpr char kPublicSamplePrngDomain[] =
    "PROJECT_DERIVED_SELECT_5ITER/public-sample-prng/v3";

void append_u32_be(std::vector<std::uint8_t>& output, std::uint32_t value) {
  output.push_back(static_cast<std::uint8_t>(value >> 24U));
  output.push_back(static_cast<std::uint8_t>(value >> 16U));
  output.push_back(static_cast<std::uint8_t>(value >> 8U));
  output.push_back(static_cast<std::uint8_t>(value));
}

std::uint32_t read_u32_be(const std::vector<std::uint8_t>& input,
                          std::size_t offset) {
  return (static_cast<std::uint32_t>(input[offset]) << 24U) |
         (static_cast<std::uint32_t>(input[offset + 1U]) << 16U) |
         (static_cast<std::uint32_t>(input[offset + 2U]) << 8U) |
         static_cast<std::uint32_t>(input[offset + 3U]);
}

void require(bool ok, const char* message) {
  if (!ok) throw std::invalid_argument(message);
}

std::uint32_t ceil_sqrt(std::uint32_t value) {
  std::uint32_t root = 0;
  while (static_cast<std::uint64_t>(root) * root < value) ++root;
  return root;
}

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
  const auto sample_size = std::min(m, ceil_sqrt(m));
  require(sample_size >= 2, "Select5 sample smaller than two");
  const auto radicand = 2U * sample_size * bit_length(n);
  const auto window = ceil_sqrt(radicand);
  const auto mu_numerator = sample_size * (rank - 1U);
  const auto mu_floor = mu_numerator / m;
  const auto mu_ceil = (mu_numerator + m - 1U) / m;
  const auto q_low = std::min(sample_size - 1U,
                              std::max(1U, mu_floor > window ? mu_floor - window : 1U));
  const auto q_high = std::min(sample_size,
                               std::max(q_low + 1U, mu_ceil + window + 1U));
  return {sample_size, q_low, q_high};
}

std::uint64_t iteration_seed(std::uint64_t seed, std::uint8_t iteration) {
  std::uint64_t value = seed + UINT64_C(0x9e3779b97f4a7c15) *
                                  (static_cast<std::uint64_t>(iteration) + 1U);
  value = (value ^ (value >> 30U)) * UINT64_C(0xbf58476d1ce4e5b9);
  value = (value ^ (value >> 27U)) * UINT64_C(0x94d049bb133111eb);
  return value ^ (value >> 31U);
}

std::uint64_t bounded(std::mt19937_64& rng, std::uint64_t bound) {
  require(bound != 0, "Select5 random bound");
  const auto threshold = static_cast<std::uint64_t>(-bound) % bound;
  for (;;) {
    const auto value = rng();
    if (value >= threshold) return value % bound;
  }
}

std::vector<std::uint32_t> sample_without_replacement(
    const std::vector<std::uint32_t>& population, std::uint32_t count,
    std::uint64_t seed) {
  require(count <= population.size(), "Select5 sample exceeds population");
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

void append_u64_be(std::vector<std::uint8_t>& output, std::uint64_t value) {
  for (int shift = 56; shift >= 0; shift -= 8)
    output.push_back(static_cast<std::uint8_t>(value >> shift));
}

class PublicSampleRandomStream {
 public:
  PublicSampleRandomStream(const ProjectDerivedSelect5PublicSeed& seed,
                           std::uint64_t session, std::uint64_t fingerprint,
                           std::uint8_t iteration)
      : seed_(seed), session_(session), fingerprint_(fingerprint),
        iteration_(iteration) {}

  std::uint64_t bounded(std::uint64_t bound) {
    require(bound != 0, "Select5 public sample bound");
    const auto reject_below = static_cast<std::uint64_t>(-bound) % bound;
    for (;;) {
      const auto word = next_u64();
      if (word >= reject_below) return word % bound;
    }
  }

 private:
  const ProjectDerivedSelect5PublicSeed& seed_;
  std::uint64_t session_;
  std::uint64_t fingerprint_;
  std::uint8_t iteration_;
  std::uint64_t block_counter_ = 0;
  std::array<std::uint8_t, 32> block_{};
  std::size_t offset_ = block_.size();

  void refill() {
    std::vector<std::uint8_t> input;
    constexpr auto domain_size = sizeof(kPublicSamplePrngDomain) - 1U;
    input.reserve(domain_size + seed_.size() + 8U + 8U + 1U + 8U);
    input.insert(input.end(), kPublicSamplePrngDomain,
                 kPublicSamplePrngDomain + domain_size);
    input.insert(input.end(), seed_.begin(), seed_.end());
    append_u64_be(input, session_);
    append_u64_be(input, fingerprint_);
    input.push_back(iteration_);
    append_u64_be(input, block_counter_);

    unsigned int digest_size = 0;
    require(EVP_Digest(input.data(), input.size(), block_.data(), &digest_size,
                       EVP_sha256(), nullptr) == 1 &&
                digest_size == block_.size(),
            "Select5 public sample SHA-256 expansion");
    require(block_counter_ != UINT64_MAX,
            "Select5 public sample stream exhausted");
    ++block_counter_;
    offset_ = 0;
  }

  std::uint64_t next_u64() {
    if (offset_ + sizeof(std::uint64_t) > block_.size()) refill();
    std::uint64_t result = 0;
    for (std::size_t i = 0; i < sizeof(std::uint64_t); ++i)
      result = (result << 8U) | block_[offset_++];
    return result;
  }
};

std::vector<std::uint32_t> sample_from_public_seed(
    const std::vector<std::uint32_t>& population, std::uint32_t count,
    const ProjectDerivedSelect5PublicSeed& seed, std::uint64_t session,
    std::uint64_t fingerprint, std::uint8_t iteration) {
  require(!population.empty() && population.size() <= kProjectDerivedSelect5MaxN &&
              count >= 1U && count <= population.size() && iteration < 4U,
          "Select5 public sample parameters");
  const std::set<std::uint32_t> distinct(population.begin(), population.end());
  require(distinct.size() == population.size(),
          "Select5 public sample population has duplicate coordinates");

  auto pool = population;
  PublicSampleRandomStream random(seed, session, fingerprint, iteration);
  for (std::uint32_t i = 0; i < count; ++i) {
    const auto remaining = static_cast<std::uint64_t>(pool.size() - i);
    const auto selected = i + static_cast<std::uint32_t>(random.bounded(remaining));
    std::swap(pool[i], pool[selected]);
  }
  pool.resize(count);
  return pool;
}

ProjectDerivedSelect5Edge canonical(std::uint32_t a, std::uint32_t b) {
  require(a != b, "Select5 self comparison");
  return {std::min(a, b), std::max(a, b)};
}

std::vector<ProjectDerivedSelect5Edge> freeze_edges(std::set<EdgeKey>&& keys) {
  std::vector<ProjectDerivedSelect5Edge> edges;
  edges.reserve(keys.size());
  for (const auto& edge : keys) edges.push_back({edge.first, edge.second});
  return edges;
}

std::vector<std::uint8_t> compare(
    const ProjectDerivedSelect5CompareWave& callback, std::uint8_t iteration,
    std::uint8_t substage, const std::vector<ProjectDerivedSelect5Edge>& edges,
    std::uint64_t& calls) {
  const auto outcomes = callback(iteration, substage, edges);
  require(outcomes.size() == edges.size(), "Select5 comparison result count");
  for (const auto bit : outcomes) require(bit <= 1, "Select5 comparison bit");
  calls += edges.size();
  return outcomes;
}

class OutcomeMap {
 public:
  void insert(const ProjectDerivedSelect5Layer& layer) {
    require(layer.edges.size() == layer.outcomes.size(), "Select5 layer shape");
    for (std::size_t i = 0; i < layer.edges.size(); ++i) {
      const auto& edge = layer.edges[i];
      require(edge.left < edge.right && layer.outcomes[i] <= 1,
              "Select5 canonical edge or outcome");
      const auto inserted = values_.emplace(EdgeKey{edge.left, edge.right},
                                             layer.outcomes[i]);
      if (!inserted.second) {
        require(inserted.first->second == layer.outcomes[i],
                "Select5 repeated edge disagrees");
      }
    }
  }

  bool before(std::uint32_t left, std::uint32_t right) const {
    if (left == right) return false;
    const auto key = EdgeKey{std::min(left, right), std::max(left, right)};
    const auto found = values_.find(key);
    require(found != values_.end(), "Select5 required comparison is absent");
    return left < right ? found->second != 0 : found->second == 0;
  }

 private:
  std::map<EdgeKey, std::uint8_t> values_;
};

std::uint32_t one_based_rank(std::uint32_t pivot,
                             const std::vector<std::uint32_t>& candidate,
                             const OutcomeMap& outcomes) {
  std::uint32_t rank = 1;
  for (const auto item : candidate) {
    if (item != pivot && outcomes.before(item, pivot)) ++rank;
  }
  return rank;
}

void verify_total_order(const std::vector<std::uint32_t>& vertices,
                        const OutcomeMap& outcomes) {
  std::set<std::uint32_t> ranks;
  for (const auto vertex : vertices) {
    std::uint32_t rank = 1;
    for (const auto other : vertices) {
      if (other != vertex && outcomes.before(other, vertex)) ++rank;
    }
    require(rank <= vertices.size() && ranks.insert(rank).second,
            "Select5 comparison graph is not a strict total order");
  }
  require(ranks.size() == vertices.size(), "Select5 total order rank gap");
}

}  // namespace

std::vector<std::uint8_t>
protocol_i_project_derived_select5_encode_public_sample_seed(
    std::uint8_t iteration, const ProjectDerivedSelect5PublicSeed& seed) {
  require(iteration < 4U, "Select5 public seed iteration");
  std::vector<std::uint8_t> result;
  result.reserve(9U + seed.size());
  append_u32_be(result, kPublicSampleSeedMagic);
  append_u32_be(result, kPublicSampleSeedContractVersion);
  result.push_back(iteration);
  result.insert(result.end(), seed.begin(), seed.end());
  return result;
}

ProjectDerivedSelect5PublicSeed
protocol_i_project_derived_select5_decode_public_sample_seed(
    const std::vector<std::uint8_t>& encoded,
    std::uint8_t expected_iteration) {
  require(expected_iteration < 4U && encoded.size() == 9U +
              kProjectDerivedSelect5PublicSeedBytes,
          "Select5 public seed frame length or iteration");
  require(read_u32_be(encoded, 0) == kPublicSampleSeedMagic &&
              read_u32_be(encoded, 4) == kPublicSampleSeedContractVersion &&
              encoded[8] == expected_iteration,
          "Select5 public seed frame binding");
  ProjectDerivedSelect5PublicSeed result{};
  std::copy(encoded.begin() + 9, encoded.end(), result.begin());
  return result;
}

std::vector<std::uint32_t>
protocol_i_project_derived_select5_sample_from_public_seed(
    const std::vector<std::uint32_t>& population, std::uint32_t sample_size,
    const ProjectDerivedSelect5PublicSeed& seed, std::uint64_t session,
    std::uint64_t fingerprint, std::uint8_t iteration) {
  return sample_from_public_seed(population, sample_size, seed, session,
                                 fingerprint, iteration);
}

void ProjectDerivedSelect5DisclosureState::mark_forward_shuffle_complete() {
  require(!forward_shuffle_complete_,
          "Select5 forward shuffle completion repeated");
  forward_shuffle_complete_ = true;
}

void ProjectDerivedSelect5DisclosureState::begin_active_iteration(
    std::uint8_t iteration) {
  require(forward_shuffle_complete_ && iteration < 4U &&
              iteration == next_iteration_,
          "Select5 sample randomness disclosed before shuffle or out of order");
  ++next_iteration_;
}

ProjectDerivedSelect5Result protocol_i_project_derived_select5_control(
    std::uint32_t n, std::uint32_t k, std::uint64_t test_only_seed,
    const ProjectDerivedSelect5CompareWave& compare_wave) {
  const ProjectDerivedSelect5SampleSource sample_source =
      [test_only_seed](std::uint8_t iteration,
                       const std::vector<std::uint32_t>& candidate,
                       std::uint32_t sample_size) {
        return sample_without_replacement(
            candidate, sample_size, iteration_seed(test_only_seed, iteration));
      };
  return protocol_i_project_derived_select5_control_with_sampler(
      n, k, sample_source, compare_wave);
}

ProjectDerivedSelect5Result protocol_i_project_derived_select5_control_with_sampler(
    std::uint32_t n, std::uint32_t k,
    const ProjectDerivedSelect5SampleSource& sample_source,
    const ProjectDerivedSelect5CompareWave& compare_wave) {
  require(n >= 1 && n <= kProjectDerivedSelect5MaxN && k >= 1 && k <= n &&
              static_cast<bool>(sample_source) && static_cast<bool>(compare_wave),
          "Select5 supported input");

  ProjectDerivedSelect5Result result;
  std::vector<std::uint32_t> candidate(n);
  for (std::uint32_t i = 0; i < n; ++i) candidate[i] = i;
  std::uint32_t residual = k;

  for (std::uint8_t iteration = 0; iteration < 5; ++iteration) {
    ProjectDerivedSelect5Trace trace;
    trace.candidate_before = candidate;
    trace.rank_before = residual;

    if (result.selected_position != UINT32_MAX) {
      trace.candidate_after = candidate;
      trace.rank_after = residual;
      trace.selected_position = result.selected_position;
      result.iterations.push_back(std::move(trace));
      continue;
    }

    if (iteration == 4) {
      trace.layers.push_back({iteration, 3, {}, {}});
      auto& layer = trace.layers.back();
      std::set<EdgeKey> keys;
      for (std::size_t i = 0; i < candidate.size(); ++i)
        for (std::size_t j = i + 1; j < candidate.size(); ++j)
          keys.emplace(candidate[i], candidate[j]);
      layer.edges = freeze_edges(std::move(keys));
      layer.outcomes = compare(compare_wave, iteration, 3, layer.edges,
                               result.comparison_calls);
      OutcomeMap outcomes;
      outcomes.insert(layer);
      verify_total_order(candidate, outcomes);
      for (const auto vertex : candidate) {
        if (one_based_rank(vertex, candidate, outcomes) == residual) {
          require(result.selected_position == UINT32_MAX,
                  "Select5 exact rank is not unique");
          result.selected_position = vertex;
        }
      }
      require(result.selected_position != UINT32_MAX,
              "Select5 exact rank is absent");
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

    const auto p = parameters(static_cast<std::uint32_t>(candidate.size()), residual, n);
    trace.sample = sample_source(iteration, candidate, p.sample_size);
    require(trace.sample.size() == p.sample_size,
            "Select5 sample source size");
    const std::set<std::uint32_t> candidate_set(candidate.begin(), candidate.end());
    const std::set<std::uint32_t> selected_sample(trace.sample.begin(),
                                                  trace.sample.end());
    require(selected_sample.size() == trace.sample.size() &&
                std::all_of(trace.sample.begin(), trace.sample.end(),
                            [&candidate_set](std::uint32_t item) {
                              return candidate_set.count(item) == 1;
                            }),
            "Select5 sample source is not without replacement from candidate");
    std::set<EdgeKey> sample_edges;
    for (std::size_t i = 0; i < trace.sample.size(); ++i)
      for (std::size_t j = i + 1; j < trace.sample.size(); ++j)
      {
        const auto edge = canonical(trace.sample[i], trace.sample[j]);
        sample_edges.emplace(edge.left, edge.right);
      }
    trace.layers.push_back({iteration, 1, {}, {}});
    trace.layers.back().edges = freeze_edges(std::move(sample_edges));
    trace.layers.back().outcomes = compare(compare_wave, iteration, 1,
                                           trace.layers.back().edges,
                                           result.comparison_calls);

    OutcomeMap outcomes;
    outcomes.insert(trace.layers.back());
    trace.sample_order = trace.sample;
    std::sort(trace.sample_order.begin(), trace.sample_order.end(),
              [&outcomes](std::uint32_t left, std::uint32_t right) {
                return outcomes.before(left, right);
              });
    require(trace.sample_order.size() == p.sample_size,
            "Select5 sample order shape");
    const auto pivot_x = trace.sample_order[p.q_low - 1U];
    const auto pivot_y = trace.sample_order[p.q_high - 1U];
    require(pivot_x != pivot_y && outcomes.before(pivot_x, pivot_y),
            "Select5 pivot order");
    trace.pivots = {pivot_x, pivot_y};

    std::set<std::uint32_t> sample_set(trace.sample.begin(), trace.sample.end());
    std::set<EdgeKey> star_edges;
    for (const auto pivot : trace.pivots)
      for (const auto item : candidate)
        if (sample_set.count(item) == 0)
          star_edges.emplace(canonical(pivot, item).left,
                             canonical(pivot, item).right);
    for (const auto& edge : trace.layers.front().edges)
      require(star_edges.count({edge.left, edge.right}) == 0,
              "Select5 sublayer edge overlap");

    trace.layers.push_back({iteration, 2, {}, {}});
    trace.layers.back().edges = freeze_edges(std::move(star_edges));
    trace.layers.back().outcomes = compare(compare_wave, iteration, 2,
                                           trace.layers.back().edges,
                                           result.comparison_calls);
    outcomes.insert(trace.layers.back());

    const auto rank_x = one_based_rank(pivot_x, candidate, outcomes);
    const auto rank_y = one_based_rank(pivot_y, candidate, outcomes);
    require(rank_x < rank_y && rank_y <= candidate.size(),
            "Select5 pivot rank invariant");

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
      throw std::invalid_argument("Select5 pivot branch is inconsistent");
    }

    if (result.selected_position == UINT32_MAX)
      require(!candidate.empty() && residual >= 1 && residual <= candidate.size(),
              "Select5 residual rank invariant");
    trace.candidate_after = candidate;
    trace.rank_after = residual;
    trace.selected_position = result.selected_position;
    result.iterations.push_back(std::move(trace));
  }

  require(result.selected_position != UINT32_MAX && result.iterations.size() == 5,
          "Select5 failed to terminate in five iterations");
  return result;
}

}  // namespace moe_topk
