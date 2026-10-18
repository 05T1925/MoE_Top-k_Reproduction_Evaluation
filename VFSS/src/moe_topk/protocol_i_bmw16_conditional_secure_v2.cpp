#include <moe_topk/protocol_i_bmw16_conditional_secure_v2.h>

#include <stdexcept>
#include <utility>

namespace moe_topk {

ProtocolIBmw16ConditionalSecureV2Output
protocol_i_bmw16_conditional_secure_v2_raw_score_mask_party(
    const ProtocolIBmw16ConditionalSecureV2Config& config,
    ProtocolIBmw16ConditionalSecureV2Material&& material,
    const std::vector<std::uint32_t>& raw_score_share,
    const std::array<int, 2>& score_fds,
    const std::array<int, 2>& forward_shuffle_fds,
    const std::array<int, 4>& select_round_fds,
    const std::array<int, 2>& inverse_shuffle_fds,
    int final_agreement_fd,
    int sampling_coin_fd) {
  if (!config.require_authenticated_transport)
    throw std::invalid_argument("conditional BMW16 v2 requires authenticated TLS transport");
  if (config.n == 0 || config.n > 1000)
    throw std::invalid_argument("conditional BMW16 v2 supported scope is 1 <= n <= 1000");
  if (config.test_only_force_probability_abort_after_select ||
      config.test_only_force_engineering_failure_after_select ||
      config.test_only_force_final_status_disagreement)
    throw std::invalid_argument("TEST_ONLY fault controls are forbidden by conditional BMW16 v2");

  // Keep a single Select/shuffle/DCF/membership/inverse implementation. This
  // wrapper changes only the explicit admission cap and never accepts a test
  // tape, a raw FD transport, or an online material source.
  return protocol_i_bmw16_experimental_raw_score_mask_party(
      config, std::move(material), raw_score_share, score_fds,
      forward_shuffle_fds, select_round_fds, inverse_shuffle_fds,
      final_agreement_fd, sampling_coin_fd, nullptr);
}

}  // namespace moe_topk
