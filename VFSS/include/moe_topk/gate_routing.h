#pragma once

#include <cstdint>
#include <vector>

namespace moe_topk {


struct GateRoutingConfig {

  uint32_t num_experts;

  uint32_t top_k;

};


struct SecretToken {

  std::vector<uint64_t> share;

};


struct RoutingResult {

  std::vector<uint32_t> expert_indices;

};


RoutingResult secure_gate_routing(
    const GateRoutingConfig& config,
    const std::vector<SecretToken>& tokens);


}
