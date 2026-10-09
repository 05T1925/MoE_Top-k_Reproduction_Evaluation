#include <moe_topk/protocol_i_bmw16_conditional_secure_v1.h>

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace moe_topk;

bool rejects(const ProtocolIBmw16ConditionalSecureV1Config& config,
             const std::string& expected) {
  ProtocolIBmw16ConditionalSecureV1Material material;
  try {
    (void)protocol_i_bmw16_conditional_secure_v1_raw_score_mask_party(
        config, std::move(material), {}, {}, {}, {}, {}, -1, -1);
  } catch (const std::invalid_argument& error) {
    return std::string(error.what()) == expected;
  }
  return false;
}

void require(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}
}  // namespace

int main() {
  try {
    ProtocolIBmw16ConditionalSecureV1Config config{};
    config.n = 8;
    require(rejects(config, "conditional BMW16 v1 requires authenticated TLS transport"),
            "conditional v1 accepted a caller-owned FD path");

    config.require_authenticated_transport = true;
    config.n = 257;
    require(rejects(config, "conditional BMW16 v1 supported scope is 1 <= n <= 256"),
            "conditional v1 accepted an unreviewed n range");

    config.n = 8;
    config.test_only_force_probability_abort_after_select = true;
    require(rejects(config, "TEST_ONLY fault controls are forbidden by conditional BMW16 v1"),
            "conditional v1 accepted a TEST_ONLY fault control");

    std::cout << "conditional_secure_v1_conformance=PASS tls_required=1 n_max=256 test_hooks=REJECTED\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "conditional_secure_v1_conformance=FAIL reason=" << error.what() << '\n';
    return 1;
  }
}
