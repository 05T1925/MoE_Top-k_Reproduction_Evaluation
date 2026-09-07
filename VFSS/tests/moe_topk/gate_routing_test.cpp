#include <moe_topk/gate_routing.h>

#include <cassert>
#include <iostream>


int main() {


  moe_topk::GateRoutingConfig config;

  config.num_experts = 4;
  config.top_k = 2;


  moe_topk::SecretToken token;

  token.share = {5};


  auto result =
      moe_topk::secure_gate_routing(
          config,
          {token});


  assert(result.expert_indices.size()
         == 2);


  assert(result.expert_indices[0]
         == 1);


  assert(result.expert_indices[1]
         == 2);


  std::cout
      << "gate routing ok\n";


  return 0;
}
