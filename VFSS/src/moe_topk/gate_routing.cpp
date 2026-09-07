#include <moe_topk/gate_routing.h>

#include <algorithm>
#include <stdexcept>


namespace moe_topk {


RoutingResult secure_gate_routing(
    const GateRoutingConfig& config,
    const std::vector<SecretToken>& tokens) {


  if (config.num_experts == 0) {
    throw std::runtime_error(
        "num experts must be positive");
  }


  if (config.top_k == 0 ||
      config.top_k > config.num_experts) {
    throw std::runtime_error(
        "invalid top k");
  }


  RoutingResult result;


  /*
   * M3.1 skeleton implementation.
   *
   * Current stage:
   *   deterministic routing oracle
   *
   * Later replaced by:
   *   secure gate computation
   *
   */


  for (const auto& token : tokens) {

    if (token.share.empty()) {
      throw std::runtime_error(
          "empty secret token");
    }


    std::vector<uint32_t> candidates;


    /*
     * Placeholder expert score:
     *
     * secret share first word
     *
     * This keeps deterministic
     * behaviour for conformance.
     */

    const auto score =
        token.share[0];


    uint32_t expert =
        static_cast<uint32_t>(
            score % config.num_experts);


    for (uint32_t k = 0;
         k < config.top_k;
         ++k) {

      result.expert_indices.push_back(
          (expert + k) %
          config.num_experts);
    }
  }


  return result;
}


}
