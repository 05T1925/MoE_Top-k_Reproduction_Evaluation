#include <moe_topk/protocol_iii_raw_score_mask.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace moe_topk {
namespace {

void require(bool condition, const char* message) {
  if (!condition) throw std::invalid_argument(message);
}

std::uint8_t width(std::uint32_t value) {
  std::uint8_t bits = 0;
  while (value != 0U) { ++bits; value >>= 1U; }
  return bits;
}

void validate(const ProtocolIIIRawScoreMaskConfig& config,
              const ProtocolIIIRawScoreMaskMaterial& material,
              const std::vector<std::uint32_t>& raw_score_shares,
              const ProtocolIIIRawScoreMaskFds& fds) {
  const auto& input = config.score_input;
  const auto& rank = config.grank;
  const auto& route = config.routing;
  require(!material.started, "Protocol III raw-score mask material replay");
  require(config.material_id != 0U && material.material_id == config.material_id,
          "Protocol III raw-score mask material identity");
  require(input.session != 0 && input.session == rank.session &&
              input.session == route.session, "Protocol III raw-score mask session");
  require(input.fingerprint != 0 && input.fingerprint == rank.fingerprint &&
              input.fingerprint == route.fingerprint, "Protocol III raw-score mask fingerprint");
  require(input.party < 2U && input.party == rank.party && input.party == route.party,
          "Protocol III raw-score mask party");
  require(input.logical_n >= 2U && input.logical_n == rank.logical_n &&
              input.logical_n == route.logical_n && raw_score_shares.size() == input.logical_n,
          "Protocol III raw-score mask logical_n");
  require(input.padded_n >= input.logical_n &&
              (input.padded_n & (input.padded_n - 1U)) == 0U &&
              input.padded_n == rank.padded_n, "Protocol III raw-score mask padded_n");
  require(input.k >= 1U && input.k <= input.logical_n &&
              input.k == rank.k && input.k == route.k, "Protocol III raw-score mask k");
  require(input.index_bits == width(input.padded_n - 1U) &&
              input.comparison_bits == 33U + input.index_bits &&
              input.comparison_bits == rank.comparison_bits &&
              input.comparison_bits == route.comparison_bits &&
              rank.rank_bits == width(input.logical_n - 1U) &&
              rank.rank_bits == route.rank_bits, "Protocol III raw-score mask widths");
  require(input.timeout_ms > 0 && rank.timeout_ms > 0 && route.timeout_ms > 0,
          "Protocol III raw-score mask timeout");
  require(material.score_input_package.party == input.party &&
              material.score_input_package.session == input.session &&
              material.score_input_package.fingerprint == input.fingerprint &&
              material.score_input_package.n == input.padded_n &&
              material.score_input_package.k == input.k &&
              material.score_input_package.comparison_bits == input.comparison_bits &&
              material.score_input_package.carry_materials.size() == input.padded_n &&
              material.score_input_package.sign_materials.size() == input.padded_n,
          "Protocol III raw-score mask score material");
  require(material.grank_package.party == rank.party &&
              material.grank_package.session == rank.session &&
              material.grank_package.fingerprint == rank.fingerprint &&
              material.grank_package.n == rank.logical_n &&
              material.grank_package.k == rank.k &&
              material.grank_package.comparison_bits == rank.comparison_bits,
          "Protocol III raw-score mask GRank material");
  require(material.routing_material.party == route.party &&
              material.routing_material.session == route.session &&
              material.routing_material.fingerprint == route.fingerprint &&
              material.routing_material.logical_n == route.logical_n &&
              material.routing_material.k == route.k &&
              material.routing_material.rank_bits == route.rank_bits &&
              !material.routing_material.started, "Protocol III raw-score mask routing material");
  const std::array<int, 4> descriptors{{fds.score_input_fds[0],
                                         fds.score_input_fds[1], fds.grank_fd, fds.routing_fd}};
  auto sorted = descriptors;
  std::sort(sorted.begin(), sorted.end());
  require(sorted.front() >= 0 &&
              std::adjacent_find(sorted.begin(), sorted.end()) == sorted.end(),
          "Protocol III raw-score mask descriptors");
}

}  // namespace

std::vector<std::uint8_t> protocol_iii_ring_indicators_to_xor_mask(
    std::uint32_t logical_n, std::uint32_t k,
    const std::vector<std::uint64_t>& indicator_shares) {
  require(logical_n >= 1U && k >= 1U && k <= logical_n,
          "Protocol III indicator-to-mask shape");
  require(static_cast<std::size_t>(logical_n) <=
              std::numeric_limits<std::size_t>::max() / k &&
              indicator_shares.size() == static_cast<std::size_t>(logical_n) * k,
          "Protocol III indicator-to-mask matrix length");
  std::vector<std::uint8_t> mask(logical_n);
  for (std::size_t item = 0; item < logical_n; ++item) {
    std::uint64_t row_sum = 0;
    for (std::size_t target = 0; target < k; ++target) {
      row_sum += indicator_shares[item * k + target];
    }
    mask[item] = static_cast<std::uint8_t>(row_sum & UINT64_C(1));
  }
  return mask;
}

ProtocolIIIRawScoreMaskOutput protocol_iii_raw_score_mask_party(
    const ProtocolIIIRawScoreMaskConfig& config,
    ProtocolIIIRawScoreMaskMaterial& material,
    const std::vector<std::uint32_t>& raw_score_shares,
    const ProtocolIIIRawScoreMaskFds& fds) {
  validate(config, material, raw_score_shares, fds);
  // A failed exchange is terminal; never retry this input-dependent state.
  material.started = true;
  ProtocolIIIRawScoreMaskOutput output;
  const auto priority_key_shares = protocol_i_raw_score_input_party(
      config.score_input, material.score_input_package, raw_score_shares,
      fds.score_input_fds, &output.metrics.score_input);
  const auto rank_output = protocol_iii_grank_party(
      config.grank, material.grank_package, priority_key_shares, fds.grank_fd);
  const auto routing_output = protocol_iii_dpf_routing_party(
      config.routing, material.routing_material,
      rank_output.rank_additive_shares, fds.routing_fd);
  output.xor_mask_shares = protocol_iii_ring_indicators_to_xor_mask(
      config.routing.logical_n, config.routing.k, routing_output.indicator_shares);
  output.metrics.grank = rank_output.metrics;
  output.metrics.routing = routing_output.metrics;
  output.metrics.input_adapter_rounds = output.metrics.score_input.rounds;
  output.metrics.mask_core_rounds = rank_output.metrics.online_rounds +
                                    routing_output.metrics.online_rounds;
  output.metrics.output_adapter_rounds = 0;
  output.metrics.total_online_rounds = output.metrics.input_adapter_rounds +
                                       output.metrics.mask_core_rounds;
  require(output.metrics.total_online_rounds == 4U,
          "Protocol III raw-score mask round count");
  output.metrics.sent_bytes = output.metrics.score_input.carry_sent_bytes +
                              output.metrics.score_input.sign_sent_bytes +
                              rank_output.metrics.sent_bytes + routing_output.metrics.sent_bytes;
  output.metrics.received_bytes = output.metrics.score_input.carry_received_bytes +
                                  output.metrics.score_input.sign_received_bytes +
                                  rank_output.metrics.received_bytes + routing_output.metrics.received_bytes;
  // Both parties send 2*padded_n masked 34-bit words in each adapter round.
  output.metrics.input_logical_bits = UINT64_C(8) * config.score_input.padded_n * 34U;
  output.metrics.ranking_logical_bits = UINT64_C(2) * config.grank.logical_n *
                                        config.grank.comparison_bits;
  output.metrics.routing_logical_bits = UINT64_C(2) * config.routing.logical_n *
                                        config.routing.rank_bits;
  output.metrics.total_logical_bits = output.metrics.input_logical_bits +
                                      output.metrics.ranking_logical_bits +
                                      output.metrics.routing_logical_bits;
  output.metrics.output_local_ring_additions =
      static_cast<std::uint64_t>(config.routing.logical_n) * (config.routing.k - 1U);
  return output;
}

}  // namespace moe_topk
