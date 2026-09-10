#include <moe_topk/protocol_iii_secure_core.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace moe_topk {
namespace {

void require_argument(
    bool condition,
    const char* message) {
  if (!condition) {
    throw std::invalid_argument(message);
  }
}

void require_runtime(
    bool condition,
    const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::uint64_t checked_sum(
    std::initializer_list<std::uint64_t> values,
    const char* message) {
  std::uint64_t result = 0;

  for (const auto value : values) {
    if (value >
        std::numeric_limits<std::uint64_t>::max() - result) {
      throw std::overflow_error(message);
    }

    result += value;
  }

  return result;
}

void validate_config(
    const ProtocolIIISecureCoreConfig& config) {
  require_argument(
      config.grank.session != 0U &&
          config.grank.session == config.routing.session &&
          config.grank.session == config.combine.session,
      "Protocol III secure core session");

  require_argument(
      config.grank.fingerprint != 0U &&
          config.grank.fingerprint ==
              config.routing.fingerprint &&
          config.grank.fingerprint ==
              config.combine.fingerprint,
      "Protocol III secure core fingerprint");

  require_argument(
      config.grank.party < 2U &&
          config.grank.party == config.routing.party &&
          config.grank.party == config.combine.party,
      "Protocol III secure core party");

  require_argument(
      config.grank.logical_n != 0U &&
          config.grank.logical_n ==
              config.routing.logical_n &&
          config.grank.logical_n ==
              config.combine.logical_n,
      "Protocol III secure core logical_n");

  require_argument(
      config.grank.padded_n >= config.grank.logical_n,
      "Protocol III secure core padded_n");

  require_argument(
      config.grank.k != 0U &&
          config.grank.k == config.routing.k &&
          config.grank.k == config.combine.k,
      "Protocol III secure core k");

  require_argument(
      config.grank.comparison_bits ==
              config.routing.comparison_bits &&
          config.grank.comparison_bits ==
              config.combine.comparison_bits,
      "Protocol III secure core comparison bits");

  require_argument(
      config.grank.rank_bits == config.routing.rank_bits,
      "Protocol III secure core rank bits");

  require_argument(
      config.grank.timeout_ms > 0 &&
          config.routing.timeout_ms > 0 &&
          config.combine.timeout_ms > 0,
      "Protocol III secure core timeout");
}

void validate_material(
    const ProtocolIIISecureCoreConfig& config,
    const ProtocolIIISecureCoreMaterial& material) {
  require_argument(
      material.grank_package.session ==
              config.grank.session &&
          material.routing_material.session ==
              config.grank.session &&
          material.combine_material.session ==
              config.grank.session,
      "Protocol III secure core material session");

  require_argument(
      material.grank_package.fingerprint ==
              config.grank.fingerprint &&
          material.routing_material.fingerprint ==
              config.grank.fingerprint &&
          material.combine_material.fingerprint ==
              config.grank.fingerprint,
      "Protocol III secure core material fingerprint");

  require_argument(
      material.grank_package.party ==
              static_cast<int>(config.grank.party) &&
          material.routing_material.party ==
              config.grank.party &&
          material.combine_material.party ==
              config.grank.party,
      "Protocol III secure core material party");

  require_argument(
      material.grank_package.n ==
              config.grank.logical_n &&
          material.routing_material.logical_n ==
              config.grank.logical_n &&
          material.combine_material.logical_n ==
              config.grank.logical_n,
      "Protocol III secure core material logical_n");

  require_argument(
      material.grank_package.k == config.grank.k &&
          material.routing_material.k == config.grank.k &&
          material.combine_material.k == config.grank.k,
      "Protocol III secure core material k");

  require_argument(
      material.grank_package.comparison_bits ==
          static_cast<int>(config.grank.comparison_bits),
      "Protocol III secure core material comparison bits");

  require_argument(
      material.routing_material.rank_bits ==
          config.grank.rank_bits,
      "Protocol III secure core material rank bits");

  require_argument(
      material.unit_payload_shares.size() ==
          config.grank.logical_n,
      "Protocol III secure core unit-payload count");
}

void validate_inputs(
    const ProtocolIIISecureCoreConfig& config,
    const std::vector<std::uint64_t>& priority_key_shares,
    const ProtocolIIISecureCoreFds& fds) {
  require_argument(
      priority_key_shares.size() ==
          config.grank.padded_n,
      "Protocol III secure core priority-key share count");

  const std::array<int, 3> descriptors{{
      fds.grank_fd,
      fds.routing_fd,
      fds.combine_fd,
  }};

  for (const auto descriptor : descriptors) {
    require_argument(
        descriptor >= 0,
        "Protocol III secure core descriptor");
  }

  auto sorted = descriptors;
  std::sort(sorted.begin(), sorted.end());

  require_argument(
      std::adjacent_find(
          sorted.begin(),
          sorted.end()) == sorted.end(),
      "Protocol III secure core duplicate descriptor");
}

}  // namespace

ProtocolIIISecureCoreOutput
protocol_iii_secure_core_party(
    const ProtocolIIISecureCoreConfig& config,
    ProtocolIIISecureCoreMaterial& material,
    const std::vector<std::uint64_t>& priority_key_shares,
    const ProtocolIIISecureCoreFds& fds) {
  validate_config(config);
  validate_material(config, material);
  validate_inputs(config, priority_key_shares, fds);

  const auto grank =
      protocol_iii_grank_party(
          config.grank,
          material.grank_package,
          priority_key_shares,
          fds.grank_fd);

  const auto routing =
      protocol_iii_dpf_routing_party(
          config.routing,
          material.routing_material,
          grank.rank_additive_shares,
          fds.routing_fd);

  auto combine =
      protocol_iii_secure_combine_party(
          config.combine,
          material.combine_material,
          routing.indicator_shares,
          material.unit_payload_shares,
          fds.combine_fd);

  require_runtime(
      grank.rank_additive_shares.size() ==
          config.grank.logical_n,
      "Protocol III secure core GRank output size");

  require_runtime(
      routing.indicator_shares.size() ==
          static_cast<std::size_t>(
              config.grank.logical_n) *
              config.grank.k,
      "Protocol III secure core routing output size");

  require_runtime(
      combine.xor_mask_shares.size() ==
          config.grank.logical_n,
      "Protocol III secure core combine output size");

  for (const auto share : combine.xor_mask_shares) {
    require_runtime(
        share <= 1U,
        "Protocol III secure core non-bit output share");
  }

  require_runtime(
      material.grank_package.node_mask_shares.empty() &&
          material.grank_package.edge_materials.empty(),
      "Protocol III secure core GRank material not consumed");

  require_runtime(
      material.routing_material.rank_mask_shares.empty() &&
          material.routing_material.dpf_keys.empty(),
      "Protocol III secure core routing material not consumed");

  require_runtime(
      material.combine_material
          .multiplication_materials.empty(),
      "Protocol III secure core multiplication material not consumed");

  ProtocolIIISecureCoreOutput output;

  output.metrics.grank = grank.metrics;
  output.metrics.routing = routing.metrics;
  output.metrics.combine = combine.metrics;

  output.metrics.sent_bytes =
      checked_sum(
          {
              grank.metrics.sent_bytes,
              routing.metrics.sent_bytes,
              combine.metrics.sent_bytes,
          },
          "Protocol III secure core sent-byte overflow");

  output.metrics.received_bytes =
      checked_sum(
          {
              grank.metrics.received_bytes,
              routing.metrics.received_bytes,
              combine.metrics.received_bytes,
          },
          "Protocol III secure core received-byte overflow");

  output.metrics.online_rounds =
      checked_sum(
          {
              grank.metrics.online_rounds,
              routing.metrics.online_rounds,
              combine.metrics.online_rounds,
          },
          "Protocol III secure core round-count overflow");

  require_runtime(
      output.metrics.online_rounds == 3U,
      "Protocol III secure core must use three online rounds");

  output.xor_mask_shares =
      std::move(combine.xor_mask_shares);

  return output;
}

}  // namespace moe_topk
