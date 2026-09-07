#include <moe_topk/protocol_iii_grank.h>

#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

using moe_topk::ProtocolIIIGrankConfig;
using moe_topk::ProtocolIIIGrankMetrics;
using moe_topk::ProtocolIPartyPackage;
using moe_topk::protocol_iii_grank_party;

void require(bool condition) {
  if (!condition) {
    throw std::runtime_error(
        "Protocol III GRank skeleton conformance failure");
  }
}

ProtocolIIIGrankConfig valid_config() {
  ProtocolIIIGrankConfig config;
  config.session = 17;
  config.fingerprint = 23;
  config.logical_n = 3;
  config.padded_n = 4;
  config.k = 2;
  config.comparison_bits = 35;
  config.rank_bits = 2;
  config.party = 0;
  config.timeout_ms = 2000;
  return config;
}

ProtocolIPartyPackage valid_package(
    const ProtocolIIIGrankConfig& config) {
  ProtocolIPartyPackage package;
  package.session = config.session;
  package.fingerprint = config.fingerprint;
  package.party = config.party;
  package.comparison_bits = config.comparison_bits;
  package.n = config.padded_n;
  package.k = config.k;
  package.node_mask_shares.resize(config.padded_n);
  return package;
}

template <typename Function>
void require_invalid_argument(Function&& function) {
  bool rejected = false;

  try {
    function();
  } catch (const std::invalid_argument&) {
    rejected = true;
  }

  require(rejected);
}

void test_metrics_defaults() {
  const ProtocolIIIGrankMetrics metrics;

  require(metrics.sent_bytes == 0);
  require(metrics.received_bytes == 0);
  require(metrics.comparison_edges == 0);
  require(metrics.ucmp_calls == 0);
  require(metrics.raw_dcf_calls == 0);
  require(metrics.online_rounds == 1);
}

void test_runtime_binding_validation() {
  const auto config = valid_config();
  const std::vector<std::uint64_t> priority_key_shares(
      config.padded_n, 0);

  {
    auto package = valid_package(config);
    package.session += 1;

    require_invalid_argument([&] {
      (void)protocol_iii_grank_party(
          config, package, priority_key_shares, 0);
    });
  }

  {
    auto package = valid_package(config);
    package.fingerprint += 1;

    require_invalid_argument([&] {
      (void)protocol_iii_grank_party(
          config, package, priority_key_shares, 0);
    });
  }

  {
    auto package = valid_package(config);
    package.party = 1;

    require_invalid_argument([&] {
      (void)protocol_iii_grank_party(
          config, package, priority_key_shares, 0);
    });
  }

  {
    auto package = valid_package(config);
    auto short_input = priority_key_shares;
    short_input.pop_back();

    require_invalid_argument([&] {
      (void)protocol_iii_grank_party(
          config, package, short_input, 0);
    });
  }

  {
    auto package = valid_package(config);

    require_invalid_argument([&] {
      (void)protocol_iii_grank_party(
          config, package, priority_key_shares, -1);
    });
  }
}

void test_valid_skeleton_is_explicitly_unimplemented() {
  const auto config = valid_config();
  auto package = valid_package(config);

  const std::vector<std::uint64_t> priority_key_shares(
      config.padded_n, 0);

  bool not_implemented = false;

  try {
    (void)protocol_iii_grank_party(
        config, package, priority_key_shares, 0);
  } catch (const std::invalid_argument&) {
    // A fully valid skeleton call must pass argument validation.
  } catch (const std::logic_error&) {
    not_implemented = true;
  }

  require(not_implemented);
}

}  // namespace

int main() {
  try {
    test_metrics_defaults();
    test_runtime_binding_validation();
    test_valid_skeleton_is_explicitly_unimplemented();
    return 0;
  } catch (...) {
    return 1;
  }
}
