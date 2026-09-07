#include <moe_topk/protocol_iii_grank.h>

#include <limits>
#include <stdexcept>

namespace moe_topk {
namespace {

bool is_power_of_two(std::uint32_t value) {
  return value != 0 && (value & (value - 1U)) == 0;
}

std::uint8_t required_rank_bits(std::uint32_t logical_n) {
  if (logical_n <= 1U) {
    return 1U;
  }

  std::uint8_t bits = 0;
  std::uint32_t value = logical_n - 1U;

  while (value != 0U) {
    ++bits;
    value >>= 1U;
  }

  return bits;
}

void validate_config(const ProtocolIIIGrankConfig& config) {
  if (config.session == 0 || config.fingerprint == 0) {
    throw std::invalid_argument(
        "Protocol III GRank requires non-zero session and fingerprint");
  }

  if (config.party > 1U) {
    throw std::invalid_argument(
        "Protocol III GRank party must be 0 or 1");
  }

  if (config.logical_n == 0 ||
      config.padded_n < config.logical_n ||
      !is_power_of_two(config.padded_n)) {
    throw std::invalid_argument(
        "Protocol III GRank input dimensions are invalid");
  }

  if (config.k == 0 || config.k > config.logical_n) {
    throw std::invalid_argument(
        "Protocol III GRank requires 1 <= k <= logical_n");
  }

  if (config.rank_bits != required_rank_bits(config.logical_n)) {
    throw std::invalid_argument(
        "Protocol III GRank rank_bits does not match logical_n");
  }

  if (config.comparison_bits == 0 ||
      config.comparison_bits >=
          std::numeric_limits<std::uint64_t>::digits) {
    throw std::invalid_argument(
        "Protocol III GRank comparison_bits is invalid");
  }

  if (config.timeout_ms <= 0) {
    throw std::invalid_argument(
        "Protocol III GRank timeout must be positive");
  }
}

void validate_package(const ProtocolIIIGrankConfig& config,
                      const ProtocolIPartyPackage& package) {
  if (package.session != config.session) {
    throw std::invalid_argument(
        "Protocol III GRank session mismatch");
  }

  if (package.fingerprint != config.fingerprint) {
    throw std::invalid_argument(
        "Protocol III GRank fingerprint mismatch");
  }

  if (package.party != static_cast<int>(config.party)) {
    throw std::invalid_argument(
        "Protocol III GRank party package mismatch");
  }

  if (package.n != config.padded_n ||
      package.k != config.k ||
      package.comparison_bits != config.comparison_bits) {
    throw std::invalid_argument(
        "Protocol III GRank package dimensions mismatch");
  }

  if (package.node_mask_shares.size() != config.padded_n) {
    throw std::invalid_argument(
        "Protocol III GRank node-mask material size mismatch");
  }
}

void validate_inputs(
    const ProtocolIIIGrankConfig& config,
    const std::vector<std::uint64_t>& priority_key_shares,
    int grank_fd) {
  if (priority_key_shares.size() != config.padded_n) {
    throw std::invalid_argument(
        "Protocol III GRank priority-key share count mismatch");
  }

  if (grank_fd < 0) {
    throw std::invalid_argument(
        "Protocol III GRank requires a valid runtime file descriptor");
  }
}

}  // namespace

ProtocolIIIGrankOutput protocol_iii_grank_party(
    const ProtocolIIIGrankConfig& config,
    ProtocolIPartyPackage& package,
    const std::vector<std::uint64_t>& priority_key_shares,
    int grank_fd) {
  validate_config(config);
  validate_package(config, package);
  validate_inputs(config, priority_key_shares, grank_fd);

  ProtocolIIIGrankOutput output;
  output.rank_additive_shares.reserve(config.logical_n);

  // M3.2 skeleton boundary:
  //
  // 1. Mask the padded priority-key shares with package.node_mask_shares.
  // 2. Exchange the masked values through ProtocolIFramedChannel.
  // 3. Invoke protocol_i_cmpagg_eval_party using package.edge_materials.
  // 4. Reduce the first logical_n rank shares into Z_(2^rank_bits).
  // 5. Populate output.metrics from the framed channel and CmpAgg counts.
  //
  // No rank or priority key may be reconstructed here.
  throw std::logic_error(
      "Protocol III GRank runtime is not implemented");
}

}  // namespace moe_topk
