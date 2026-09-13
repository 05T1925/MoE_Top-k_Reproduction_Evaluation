#include <moe_topk/protocol_i_raw_score_route_a.h>

#include <openssl/crypto.h>
#include <openssl/rand.h>

#include <limits>
#include <stdexcept>

#include <moe_topk/protocol_i_pipeline.h>

namespace moe_topk {
namespace {

constexpr int kScoreBits = 34;
constexpr std::uint64_t kScoreMask = (UINT64_C(1) << kScoreBits) - 1U;

[[noreturn]] void fail(const char* message) { throw std::invalid_argument(message); }

std::size_t edge_count(std::uint32_t n) {
  if (n < 2) fail("raw Route A edge count");
  const auto nodes = static_cast<std::size_t>(n);
  const auto count = nodes * (nodes - 1U) / 2U;
  if (count > 1'000'000U) fail("raw Route A edge count limit");
  return count;
}

std::uint64_t random_ring_word(int bits) {
  if (bits < 1 || bits > 53) fail("raw Route A random width");
  std::uint64_t value = 0;
  if (RAND_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value)) != 1)
    fail("raw Route A random source");
  return value & ((UINT64_C(1) << bits) - 1U);
}

void clean(std::vector<std::uint64_t>& values) {
  if (!values.empty()) OPENSSL_cleanse(values.data(), values.size() * sizeof(values.front()));
  values.clear();
  values.shrink_to_fit();
}

void validate(const ProtocolIDealerCandidatePublicConfig& config) {
  const auto layout = protocol_i_make_input_layout(config.logical_n, config.k);
  if (config.session == 0 || config.fingerprint == 0 || config.material_id == 0 ||
      config.padded_n != layout.padded_n || config.rank_bits != layout.index_bits ||
      config.comparison_bits != layout.minimum_comparison_bits || config.comparison_bits > 53)
    fail("raw Route A public layout");
  (void)edge_count(config.padded_n);
}

ProtocolIPartyPackage make_package(
    const ProtocolIDealerCandidatePublicConfig& config, int party,
    const std::vector<std::uint64_t>& node_shares,
    std::vector<ProtocolIEdgePartyMaterial>&& edges,
    std::vector<ProtocolIScoreInputPartyMaterial>&& carry,
    std::vector<ProtocolIScoreInputPartyMaterial>&& sign) {
  ProtocolIPartyPackage package;
  package.session = config.session;
  package.fingerprint = config.fingerprint;
  package.party = party;
  package.comparison_bits = config.comparison_bits;
  package.n = config.padded_n;
  package.k = config.k;
  package.node_mask_shares = node_shares;
  package.edge_materials = std::move(edges);
  package.carry_materials = std::move(carry);
  package.sign_materials = std::move(sign);
  return package;
}

}  // namespace

ProtocolIRawScoreRouteAPackagePair protocol_i_raw_score_route_a_preprocess(
    const ProtocolIDealerCandidatePublicConfig& config) {
  validate(config);
  const auto n = config.padded_n;
  const auto candidate_mask = (UINT64_C(1) << config.comparison_bits) - 1U;
  std::vector<std::uint64_t> full_node(n), node0(n), node1(n);
  for (std::size_t slot = 0; slot < n; ++slot) {
    full_node[slot] = random_ring_word(config.comparison_bits);
    node0[slot] = random_ring_word(config.comparison_bits);
    node1[slot] = (full_node[slot] - node0[slot]) & candidate_mask;
  }

  std::vector<ProtocolIEdgePartyMaterial> edges0, edges1;
  edges0.reserve(edge_count(n));
  edges1.reserve(edge_count(n));
  for (std::uint32_t left = 0; left < n; ++left) {
    for (std::uint32_t right = left + 1; right < n; ++right) {
      ProtocolIUcmpMaterial material(config.comparison_bits, full_node[left], full_node[right]);
      edges0.emplace_back(left, right, material.export_party_material(0));
      edges1.emplace_back(left, right, material.export_party_material(1));
    }
  }

  std::vector<ProtocolIScoreInputPartyMaterial> carry0, carry1, sign0, sign1;
  carry0.reserve(n); carry1.reserve(n); sign0.reserve(n); sign1.reserve(n);
  for (std::uint32_t slot = 0; slot < n; ++slot) {
    const auto left = random_ring_word(kScoreBits);
    const auto right = random_ring_word(kScoreBits);
    const auto left0 = random_ring_word(kScoreBits);
    const auto right0 = random_ring_word(kScoreBits);
    ProtocolIUcmpMaterial carry_material(kScoreBits, left, right);
    carry0.emplace_back(slot, 1, left0, right0, carry_material.export_party_material(0));
    carry1.emplace_back(slot, 1, (left - left0) & kScoreMask,
                        (right - right0) & kScoreMask,
                        carry_material.export_party_material(1));

    const auto sign_left = random_ring_word(kScoreBits);
    const auto sign_right = random_ring_word(kScoreBits);
    const auto sign_left0 = random_ring_word(kScoreBits);
    const auto sign_right0 = random_ring_word(kScoreBits);
    ProtocolIUcmpMaterial sign_material(kScoreBits, sign_left, sign_right);
    sign0.emplace_back(slot, 2, sign_left0, sign_right0,
                       sign_material.export_party_material(0));
    sign1.emplace_back(slot, 2, (sign_left - sign_left0) & kScoreMask,
                       (sign_right - sign_right0) & kScoreMask,
                       sign_material.export_party_material(1));
  }

  ProtocolIRawScoreRouteAPackagePair result{
      make_package(config, 0, node0, std::move(edges0), std::move(carry0), std::move(sign0)),
      make_package(config, 1, node1, std::move(edges1), std::move(carry1), std::move(sign1))};
  clean(full_node); clean(node0); clean(node1);
  return result;
}

}  // namespace moe_topk
