#include <moe_topk/experimental_bmw16_select_adapter.h>

#include <stdexcept>

namespace moe_topk {

std::vector<std::uint64_t> protocol_i_bmw16_experimental_select_key_party(
    const ProtocolIScoreInputConfig& config, ProtocolIPartyPackage& package,
    const std::vector<std::uint32_t>& raw_share,
    const std::array<int, 2>& stage_fds,
    ProtocolIScoreInputMetrics* metrics) {
  if (config.logical_n == 0 || config.k == 0 || config.k > config.logical_n ||
      config.index_bits == 0 || config.comparison_bits != 33U + config.index_bits ||
      config.comparison_bits >= 64 || config.party > 1) {
    throw std::invalid_argument("BMW16 experimental adapter configuration");
  }

  auto priority_shares = protocol_i_raw_score_input_party(
      config, package, raw_share, stage_fds, metrics);
  const auto key_mask = (UINT64_C(1) << config.comparison_bits) - 1U;
  const auto largest_real_priority =
      (static_cast<std::uint64_t>(UINT32_MAX) << config.index_bits) +
      (config.logical_n - 1U);
  std::vector<std::uint64_t> select_shares(config.logical_n);
  for (std::uint32_t index = 0; index < config.logical_n; ++index) {
    // The official Protocol I key is smaller for a higher-priority record.
    // S4 Select consumes the reversed order, so subtract shares from a public
    // bound on real priority keys. The bound is below 2^(32+index_bits), and
    // the adapter ring is wider (33+index_bits), so the mapping is exact.
    select_shares[index] = config.party == 0
        ? (largest_real_priority - priority_shares[index]) & key_mask
        : (UINT64_C(0) - priority_shares[index]) & key_mask;
  }
  return select_shares;
}

}  // namespace moe_topk
