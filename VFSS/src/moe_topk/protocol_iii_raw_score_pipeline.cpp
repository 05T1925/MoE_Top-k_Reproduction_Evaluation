#include <moe_topk/protocol_iii_raw_score_pipeline.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace moe_topk {
namespace {

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::invalid_argument(message);
  }
}

void validate_config(
    const ProtocolIIIRawScorePipelineConfig& config) {
  require(
      config.score_input.session != 0 &&
          config.score_input.session == config.grank.session &&
          config.score_input.session == config.routing.session &&
          config.score_input.session == config.combine.session,
      "Protocol III raw-score pipeline session");

  require(
      config.score_input.fingerprint != 0 &&
          config.score_input.fingerprint ==
              config.grank.fingerprint &&
          config.score_input.fingerprint ==
              config.routing.fingerprint &&
          config.score_input.fingerprint ==
              config.combine.fingerprint,
      "Protocol III raw-score pipeline fingerprint");

  require(
      config.score_input.party < 2U &&
          config.score_input.party == config.grank.party &&
          config.score_input.party == config.routing.party &&
          config.score_input.party == config.combine.party,
      "Protocol III raw-score pipeline party");

  require(
      config.score_input.logical_n != 0U &&
          config.score_input.logical_n ==
              config.grank.logical_n &&
          config.score_input.logical_n ==
              config.routing.logical_n &&
          config.score_input.logical_n ==
              config.combine.logical_n,
      "Protocol III raw-score pipeline logical_n");

  require(
      config.score_input.padded_n ==
          config.grank.padded_n,
      "Protocol III raw-score pipeline padded_n");

  require(
      config.score_input.k != 0U &&
          config.score_input.k ==
              config.grank.k &&
          config.score_input.k ==
              config.routing.k &&
          config.score_input.k ==
              config.combine.k,
      "Protocol III raw-score pipeline k");

  require(
      config.score_input.comparison_bits ==
              config.grank.comparison_bits &&
          config.score_input.comparison_bits ==
              config.routing.comparison_bits &&
          config.score_input.comparison_bits ==
              config.combine.comparison_bits,
      "Protocol III raw-score pipeline comparison bits");

  require(
      config.grank.rank_bits ==
          config.routing.rank_bits,
      "Protocol III raw-score pipeline rank bits");

  require(
      config.score_input.index_bits >= 1U &&
          config.score_input.comparison_bits ==
              static_cast<std::uint8_t>(
                  33U + config.score_input.index_bits),
      "Protocol III raw-score pipeline index bits");

  require(
      config.score_input.timeout_ms > 0 &&
          config.grank.timeout_ms > 0 &&
          config.routing.timeout_ms > 0 &&
          config.combine.timeout_ms > 0,
      "Protocol III raw-score pipeline timeout");
}

void validate_inputs(
    const ProtocolIIIRawScorePipelineConfig& config,
    const std::vector<std::uint32_t>& raw_score_shares,
    const ProtocolIIIRawScorePipelineFds& fds) {
  require(
      raw_score_shares.size() ==
          config.score_input.logical_n,
      "Protocol III raw-score pipeline input size");

  const std::array<int, 5> descriptors{{
      fds.score_input_fds[0],
      fds.score_input_fds[1],
      fds.grank_fd,
      fds.routing_fd,
      fds.combine_fd,
  }};

  for (const auto descriptor : descriptors) {
    require(
        descriptor >= 0,
        "Protocol III raw-score pipeline descriptor");
  }

  auto sorted = descriptors;
  std::sort(sorted.begin(), sorted.end());

  require(
      std::adjacent_find(
          sorted.begin(),
          sorted.end()) == sorted.end(),
      "Protocol III raw-score pipeline duplicate descriptor");
}

std::uint64_t add_without_wrap_check(
    std::initializer_list<std::uint64_t> values) {
  std::uint64_t total = 0;

  for (const auto value : values) {
    total += value;
  }

  return total;
}

}  // namespace

ProtocolIIIRawScorePipelineOutput
protocol_iii_raw_score_pipeline_party(
    const ProtocolIIIRawScorePipelineConfig& config,
    ProtocolIIIRawScorePipelineMaterial& material,
    const std::vector<std::uint32_t>& raw_score_shares,
    const ProtocolIIIRawScorePipelineFds& fds) {
  validate_config(config);
  validate_inputs(config, raw_score_shares, fds);

  ProtocolIIIRawScorePipelineOutput output;

  const auto priority_key_shares =
      protocol_i_raw_score_input_party(
          config.score_input,
          material.score_input_package,
          raw_score_shares,
          fds.score_input_fds,
          &output.metrics.score_input);

  require(
      priority_key_shares.size() ==
          config.grank.padded_n,
      "Protocol III raw-score pipeline priority-key size");

  const auto grank_output =
      protocol_iii_grank_party(
          config.grank,
          material.grank_package,
          priority_key_shares,
          fds.grank_fd);

  const auto routing_output =
      protocol_iii_dpf_routing_party(
          config.routing,
          material.routing_material,
          grank_output.rank_additive_shares,
          fds.routing_fd);

  auto combine_output =
      protocol_iii_secure_combine_party(
          config.combine,
          material.combine_material,
          routing_output.indicator_shares,
          material.unit_payload_shares,
          fds.combine_fd);

  output.metrics.grank = grank_output.metrics;
  output.metrics.routing = routing_output.metrics;
  output.metrics.combine = combine_output.metrics;

  output.metrics.input_adapter_rounds =
      output.metrics.score_input.rounds;

  output.metrics.core_rounds =
      output.metrics.grank.online_rounds +
      output.metrics.routing.online_rounds +
      output.metrics.combine.online_rounds;

  output.metrics.total_online_rounds =
      output.metrics.input_adapter_rounds +
      output.metrics.core_rounds;

  require(
      output.metrics.input_adapter_rounds == 2U,
      "Protocol III raw-score adapter round count");

  require(
      output.metrics.core_rounds == 3U,
      "Protocol III priority-key core round count");

  require(
      output.metrics.total_online_rounds == 5U,
      "Protocol III raw-score total round count");

  output.metrics.sent_bytes =
      add_without_wrap_check({
          output.metrics.score_input.carry_sent_bytes,
          output.metrics.score_input.sign_sent_bytes,
          output.metrics.grank.sent_bytes,
          output.metrics.routing.sent_bytes,
          output.metrics.combine.sent_bytes,
      });

  output.metrics.received_bytes =
      add_without_wrap_check({
          output.metrics.score_input.carry_received_bytes,
          output.metrics.score_input.sign_received_bytes,
          output.metrics.grank.received_bytes,
          output.metrics.routing.received_bytes,
          output.metrics.combine.received_bytes,
      });

  output.xor_mask_shares =
      std::move(combine_output.xor_mask_shares);

  require(
      output.xor_mask_shares.size() ==
          config.score_input.logical_n,
      "Protocol III raw-score pipeline output size");

  return output;
}

}  // namespace moe_topk
