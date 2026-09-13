#include <moe_topk/protocol_i_dealer_candidate_route_a.h>
#include <moe_topk/protocol_i_transport.h>

#include <stdexcept>
#include <string>

int main() {
  try {
    using namespace moe_topk;
    static_assert(kProtocolIDealerCandidateRouteAPriorityLabel[0] == 'm');
    if (std::string(kProtocolIDealerCandidateRouteAPriorityLabel).find("exact") != std::string::npos)
      throw std::runtime_error("route A must remain project extension");
    ProtocolIDealerCandidateRouteAOutput output;
    if (output.online_rounds != 6 || !output.xor_mask_share.empty()) {
      throw std::runtime_error("route A default contract");
    }
    // The executable-level process differential is intentionally separate:
    // this primitive test freezes the output shape and leakage label.
    return 0;
  } catch (...) {
    return 1;
  }
}
