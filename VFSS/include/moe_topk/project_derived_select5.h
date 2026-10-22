#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace moe_topk {

inline constexpr std::uint32_t kProjectDerivedSelect5MaxN = 256;
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
inline constexpr std::uint32_t kProjectDerivedSelect5FtV3MaxN = 1024;
#endif
inline constexpr std::size_t kProjectDerivedSelect5PublicSeedBytes = 32;
using ProjectDerivedSelect5PublicSeed =
    std::array<std::uint8_t, kProjectDerivedSelect5PublicSeedBytes>;

// S8 version-3 public sampling contract. The fixed-width wire value binds its
// iteration internally; the framed transport separately binds session,
// fingerprint, N/K, party direction, phase, and one-shot material id.
std::vector<std::uint8_t>
protocol_i_project_derived_select5_encode_public_sample_seed(
    std::uint8_t iteration, const ProjectDerivedSelect5PublicSeed& seed);
ProjectDerivedSelect5PublicSeed
protocol_i_project_derived_select5_decode_public_sample_seed(
    const std::vector<std::uint8_t>& encoded,
    std::uint8_t expected_iteration);
std::vector<std::uint32_t>
protocol_i_project_derived_select5_sample_from_public_seed(
    const std::vector<std::uint32_t>& population, std::uint32_t sample_size,
    const ProjectDerivedSelect5PublicSeed& seed, std::uint64_t session,
    std::uint64_t fingerprint, std::uint8_t iteration);

// Enforces that a public per-iteration sample seed can only be disclosed
// after forward shuffle completion and exactly once in increasing iteration
// order. Used by the runtime and directly tested before protocol execution.
class ProjectDerivedSelect5DisclosureState {
 public:
  void mark_forward_shuffle_complete();
  void begin_active_iteration(std::uint8_t iteration);

 private:
  bool forward_shuffle_complete_ = false;
  std::uint8_t next_iteration_ = 0;
};

// PROJECT_DERIVED_SELECT_5ITER control logic. All vertex ids are shuffled
// coordinates. The callback returns the publicly approved strict-order bits
// for the exact, frozen graph it receives.
struct ProjectDerivedSelect5Edge {
  std::uint32_t left = 0;
  std::uint32_t right = 0;
  bool operator==(const ProjectDerivedSelect5Edge& other) const {
    return left == other.left && right == other.right;
  }
};

struct ProjectDerivedSelect5Layer {
  std::uint8_t iteration = 0;  // 0..3 reduction iterations, 4 exact layer
  std::uint8_t substage = 0;   // 1 sample clique, 2 pivot stars, 3 exact clique
  std::vector<ProjectDerivedSelect5Edge> edges;
  std::vector<std::uint8_t> outcomes;  // 1 iff key[left] < key[right]
};

struct ProjectDerivedSelect5Trace {
  std::vector<std::uint32_t> candidate_before;
  std::vector<std::uint32_t> candidate_after;
  std::uint32_t rank_before = 0;
  std::uint32_t rank_after = 0;
  std::vector<std::uint32_t> sample;
  std::vector<std::uint32_t> sample_order;
  std::vector<std::uint32_t> pivots;
  std::vector<ProjectDerivedSelect5Layer> layers;
  std::uint32_t selected_position = UINT32_MAX;
};

struct ProjectDerivedSelect5Result {
  std::uint32_t selected_position = UINT32_MAX;
  std::vector<ProjectDerivedSelect5Trace> iterations;
  std::uint64_t comparison_calls = 0;
};

using ProjectDerivedSelect5CompareWave = std::function<std::vector<std::uint8_t>(
    std::uint8_t iteration, std::uint8_t substage,
    const std::vector<ProjectDerivedSelect5Edge>& frozen_edges)>;

using ProjectDerivedSelect5SampleSource = std::function<std::vector<std::uint32_t>(
    std::uint8_t iteration,
    const std::vector<std::uint32_t>& candidate,
    std::uint32_t sample_size)>;

// TEST_ONLY deterministic entry point. The seed is local to this plaintext
// oracle API and must not be used as the runtime schedule mechanism.
ProjectDerivedSelect5Result protocol_i_project_derived_select5_control(
    std::uint32_t n, std::uint32_t k, std::uint64_t test_only_seed,
    const ProjectDerivedSelect5CompareWave& compare_wave);

// Runtime-capable control entry point. The caller supplies a sample set only
// when its iteration is active. Protocol I supplies it by disclosing the
// iteration's public seed after shuffle completion; later seeds are generated
// independently only when their own active iterations begin.
ProjectDerivedSelect5Result protocol_i_project_derived_select5_control_with_sampler(
    std::uint32_t n, std::uint32_t k,
    const ProjectDerivedSelect5SampleSource& sample_source,
    const ProjectDerivedSelect5CompareWave& compare_wave);
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
std::vector<std::uint32_t>
protocol_i_project_derived_select5_sample_from_public_seed_ft_v3(
    const std::vector<std::uint32_t>& population, std::uint32_t sample_size,
    const ProjectDerivedSelect5PublicSeed& seed, std::uint64_t session,
    std::uint64_t fingerprint, std::uint8_t iteration);
ProjectDerivedSelect5Result
protocol_i_project_derived_select5_control_with_sampler_ft_v3(
    std::uint32_t n, std::uint32_t k,
    const ProjectDerivedSelect5SampleSource& sample_source,
    const ProjectDerivedSelect5CompareWave& compare_wave);
#endif

}  // namespace moe_topk
