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

std::vector<std::uint8_t> singleton_share(std::uint8_t party) {
  ProtocolIBmw16ConditionalSecureV1Config config{};
  config.n = 1; config.k = 1; config.padded_n = 2;
  config.index_bits = 1; config.comparison_bits = 34;
  config.party = party; config.timeout_ms = 100;
  config.require_authenticated_transport = true;
  ProtocolIBmw16ConditionalSecureV1Material material;
  const auto result = protocol_i_bmw16_conditional_secure_v1_raw_score_mask_party(
      config, std::move(material), {0}, {}, {}, {}, {}, -1, -1);
  require(std::string(result.status) == "SUCCESS" &&
              std::string(result.abort_scope) == "NONE",
          "conditional v1 n=1 did not use its exact shortcut");
  return result.xor_mask_share;
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

    const auto share0 = singleton_share(0), share1 = singleton_share(1);
    require(share0 == std::vector<std::uint8_t>{1} &&
                share1 == std::vector<std::uint8_t>{0} &&
                (share0[0] ^ share1[0]) == 1,
            "conditional v1 n=1 XOR shortcut shares");

    std::cout << "conditional_secure_v1_conformance=PASS tls_required=1 n_max=256 test_hooks=REJECTED n1_shortcut=PASS\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "conditional_secure_v1_conformance=FAIL reason=" << error.what() << '\n';
    return 1;
  }
}
