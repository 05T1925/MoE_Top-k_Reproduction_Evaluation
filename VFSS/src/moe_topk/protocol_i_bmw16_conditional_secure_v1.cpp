#include <moe_topk/protocol_i_bmw16_conditional_secure_v1.h>

#include <stdexcept>
#include <utility>

namespace moe_topk {

ProtocolIBmw16ConditionalSecureV1Output
protocol_i_bmw16_conditional_secure_v1_raw_score_mask_party(
    const ProtocolIBmw16ConditionalSecureV1Config& config,
    ProtocolIBmw16ConditionalSecureV1Material&& material,
    const std::vector<std::uint32_t>& raw_score_share,
    const std::array<int, 2>& score_fds,
    const std::array<int, 2>& forward_shuffle_fds,
    const std::array<int, 4>& select_round_fds,
    const std::array<int, 2>& inverse_shuffle_fds,
    int final_agreement_fd,
    int sampling_coin_fd) {
  if (!config.require_authenticated_transport)
    throw std::invalid_argument("conditional BMW16 v1 requires authenticated TLS transport");
  if (config.n == 0 || config.n > 256)
    throw std::invalid_argument("conditional BMW16 v1 supported scope is 1 <= n <= 256");
  if (config.test_only_force_probability_abort_after_select ||
      config.test_only_force_engineering_failure_after_select ||
      config.test_only_force_final_status_disagreement)
    throw std::invalid_argument("TEST_ONLY fault controls are forbidden by conditional BMW16 v1");

  // The wrapper fixes the reviewed transport/randomness boundary and delegates
  // every protocol operation to the sole S26/S28-reviewed implementation.
  // No Select, shuffle, DCF, material, or mask logic is duplicated here.
  return protocol_i_bmw16_experimental_raw_score_mask_party(
      config, std::move(material), raw_score_share, score_fds,
      forward_shuffle_fds, select_round_fds, inverse_shuffle_fds,
      final_agreement_fd, sampling_coin_fd, nullptr);
}

}  // namespace moe_topk
