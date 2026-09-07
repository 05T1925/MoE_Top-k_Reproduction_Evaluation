#include <moe_topk/protocol_iii_dpf_routing.h>

#include <moe_topk/topk_oracle.h>

#include <FSS/dpf.h>
#include <FSS/prng.h>

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <random>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include <sys/socket.h>
#include <unistd.h>

namespace {

using namespace moe_topk;

constexpr int kTimeoutMs = 5000;
constexpr int kPayloadBits = 64;

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::uint8_t bit_width(std::uint32_t value) {
  std::uint8_t bits = 0;

  while (value != 0U) {
    ++bits;
    value >>= 1U;
  }

  return bits;
}

std::uint8_t rank_bits(std::uint32_t logical_n) {
  return logical_n <= 1U
             ? 1U
             : bit_width(logical_n - 1U);
}

std::uint32_t padded_size(std::uint32_t logical_n) {
  std::uint32_t padded_n = 2U;

  while (padded_n < logical_n) {
    padded_n <<= 1U;
  }

  return padded_n;
}

std::uint8_t comparison_bits(std::uint32_t logical_n) {
  const auto padded_n = padded_size(logical_n);

  return static_cast<std::uint8_t>(
      33U + bit_width(padded_n - 1U));
}

std::uint64_t rank_mask(std::uint8_t bits) {
  return (UINT64_C(1) << bits) - 1U;
}

void seed_fss(std::uint64_t seed) {
  for (int index = 0; index < 256; ++index) {
    FSSConfig::prngs[index].SetSeed(
        osuCrypto::toBlock(
            seed,
            static_cast<std::uint64_t>(index)));
  }
}

class SocketPair final {
 public:
  SocketPair() {
    require(
        ::socketpair(AF_UNIX, SOCK_STREAM, 0, fds_.data()) == 0,
        "DPF routing socketpair failed");
  }

  ~SocketPair() {
    for (auto& fd : fds_) {
      if (fd >= 0) {
        ::close(fd);
        fd = -1;
      }
    }
  }

  SocketPair(const SocketPair&) = delete;
  SocketPair& operator=(const SocketPair&) = delete;

  int duplicate(std::size_t index) const {
    const int result = ::dup(fds_[index]);

    require(
        result >= 0,
        "DPF routing socket duplication failed");

    return result;
  }

 private:
  std::array<int, 2> fds_{{-1, -1}};
};

struct TestCase {
  std::vector<std::uint32_t> scores;
  std::uint32_t k = 0;
  std::uint64_t session = 0;
  std::uint64_t fingerprint = 0;
  std::uint64_t seed = 0;
};

ProtocolIIIDpfRoutingConfig make_config(
    const TestCase& test,
    std::uint8_t party) {
  ProtocolIIIDpfRoutingConfig config;

  config.session = test.session;
  config.fingerprint = test.fingerprint;
  config.logical_n =
      static_cast<std::uint32_t>(test.scores.size());
  config.k = test.k;
  config.rank_bits = rank_bits(config.logical_n);
  config.comparison_bits =
      comparison_bits(config.logical_n);
  config.party = party;
  config.timeout_ms = kTimeoutMs;

  return config;
}

struct RankShares {
  std::vector<std::uint64_t> party0;
  std::vector<std::uint64_t> party1;
};

RankShares share_ranks(
    const std::vector<std::uint64_t>& ranks,
    std::uint8_t bits,
    std::mt19937_64& generator) {
  const auto mask = rank_mask(bits);

  RankShares result;
  result.party0.resize(ranks.size());
  result.party1.resize(ranks.size());

  for (std::size_t index = 0;
       index < ranks.size();
       ++index) {
    result.party0[index] = generator() & mask;

    result.party1[index] =
        (ranks[index] - result.party0[index]) &
        mask;
  }

  return result;
}

std::pair<
    ProtocolIIIDpfRoutingPartyMaterial,
    ProtocolIIIDpfRoutingPartyMaterial>
make_materials(
    const ProtocolIIIDpfRoutingConfig& config,
    std::mt19937_64& generator) {
  const auto mask = rank_mask(config.rank_bits);

  ProtocolIIIDpfRoutingPartyMaterial party0;
  ProtocolIIIDpfRoutingPartyMaterial party1;

  for (auto* material : {&party0, &party1}) {
    material->session = config.session;
    material->fingerprint = config.fingerprint;
    material->logical_n = config.logical_n;
    material->k = config.k;
    material->rank_bits = config.rank_bits;
    material->rank_mask_shares.resize(config.logical_n);
    material->dpf_keys.reserve(config.logical_n);
  }

  party0.party = 0;
  party1.party = 1;

  for (std::size_t index = 0;
       index < config.logical_n;
       ++index) {
    const auto full_mask = generator() & mask;
    const auto party0_mask = generator() & mask;

    party0.rank_mask_shares[index] =
        party0_mask;

    party1.rank_mask_shares[index] =
        (full_mask - party0_mask) & mask;

    auto keys = keyGenDPF(
        config.rank_bits,
        kPayloadBits,
        full_mask,
        1);

    party0.dpf_keys.emplace_back(
        std::move(keys.first));

    party1.dpf_keys.emplace_back(
        std::move(keys.second));
  }

  return {
      std::move(party0),
      std::move(party1)};
}

void rethrow_if_present(
    const std::exception_ptr& error) {
  if (error) {
    std::rethrow_exception(error);
  }
}

void run_case(const TestCase& test) {
  const auto config0 = make_config(test, 0);
  const auto config1 = make_config(test, 1);

  seed_fss(test.seed);

  std::mt19937_64 generator(test.seed);

  const auto oracle_ranks =
      stable_ranks_cmpagg(test.scores);

  auto rank_shares =
      share_ranks(
          oracle_ranks,
          config0.rank_bits,
          generator);

  auto materials =
      make_materials(config0, generator);

  SocketPair sockets;

  // Preserve the original endpoints so ProtocolIFramedChannel closing one
  // duplicate cannot expose POLLHUP while the peer reads the final frame.
  const int party0_fd = sockets.duplicate(0);
  const int party1_fd = sockets.duplicate(1);

  ProtocolIIIDpfRoutingOutput output0;
  ProtocolIIIDpfRoutingOutput output1;

  std::exception_ptr error0;
  std::exception_ptr error1;

  std::thread party0([&] {
    try {
      output0 = protocol_iii_dpf_routing_party(
          config0,
          materials.first,
          rank_shares.party0,
          party0_fd);
    } catch (...) {
      error0 = std::current_exception();
    }
  });

  std::thread party1([&] {
    try {
      output1 = protocol_iii_dpf_routing_party(
          config1,
          materials.second,
          rank_shares.party1,
          party1_fd);
    } catch (...) {
      error1 = std::current_exception();
    }
  });

  party0.join();
  party1.join();

  rethrow_if_present(error0);
  rethrow_if_present(error1);

  const auto expected_size =
      static_cast<std::size_t>(config0.logical_n) *
      config0.k;

  require(
      output0.indicator_shares.size() == expected_size,
      "P0 indicator shape mismatch");

  require(
      output1.indicator_shares.size() == expected_size,
      "P1 indicator shape mismatch");

  for (std::size_t input = 0;
       input < config0.logical_n;
       ++input) {
    for (std::uint32_t target = 0;
         target < config0.k;
         ++target) {
      const auto offset =
          input * config0.k + target;

      // TEST_ONLY reconstruction for oracle differential.
      const auto reconstructed =
          output0.indicator_shares[offset] +
          output1.indicator_shares[offset];

      const std::uint64_t expected =
          oracle_ranks[input] == target ? 1U : 0U;

      require(
          reconstructed == expected,
          "DPF routing indicator differs from oracle");
    }
  }

  const auto expected_calls =
      static_cast<std::uint64_t>(config0.logical_n) *
      config0.k;

  for (const auto* output : {&output0, &output1}) {
    require(
        output->metrics.sent_bytes > 0,
        "routing sent-byte metric is empty");

    require(
        output->metrics.received_bytes > 0,
        "routing received-byte metric is empty");

    require(
        output->metrics.dpf_keys == config0.logical_n,
        "routing DPF-key metric mismatch");

    require(
        output->metrics.eval_calls == expected_calls,
        "routing evaluation metric mismatch");

    require(
        output->metrics.online_rounds == 1U,
        "routing must report one online round");
  }

  require(
      output0.metrics.sent_bytes ==
          output1.metrics.received_bytes,
      "P0 sent bytes differ from P1 received bytes");

  require(
      output1.metrics.sent_bytes ==
          output0.metrics.received_bytes,
      "P1 sent bytes differ from P0 received bytes");

  require(
      materials.first.rank_mask_shares.empty() &&
          materials.first.dpf_keys.empty(),
      "P0 routing material was not consumed");

  require(
      materials.second.rank_mask_shares.empty() &&
          materials.second.dpf_keys.empty(),
      "P1 routing material was not consumed");
}

template <typename Function>
void require_invalid_argument(Function&& function) {
  bool rejected = false;

  try {
    function();
  } catch (const std::invalid_argument&) {
    rejected = true;
  }

  require(
      rejected,
      "invalid DPF routing input was accepted");
}

void test_binding_rejection() {
  const TestCase test{
      {5U, 5U, 9U},
      2U,
      UINT64_C(0x9101),
      UINT64_C(0xa101),
      UINT64_C(0xb101)};

  const auto config = make_config(test, 0);

  seed_fss(test.seed);

  std::mt19937_64 generator(test.seed);

  const auto oracle_ranks =
      stable_ranks_cmpagg(test.scores);

  auto shares =
      share_ranks(
          oracle_ranks,
          config.rank_bits,
          generator);

  {
    auto materials = make_materials(config, generator);
    materials.first.session += 1U;

    require_invalid_argument([&] {
      (void)protocol_iii_dpf_routing_party(
          config,
          materials.first,
          shares.party0,
          0);
    });
  }

  {
    auto materials = make_materials(config, generator);
    materials.first.dpf_keys.pop_back();

    require_invalid_argument([&] {
      (void)protocol_iii_dpf_routing_party(
          config,
          materials.first,
          shares.party0,
          0);
    });
  }

  {
    auto materials = make_materials(config, generator);
    auto short_shares = shares.party0;
    short_shares.pop_back();

    require_invalid_argument([&] {
      (void)protocol_iii_dpf_routing_party(
          config,
          materials.first,
          short_shares,
          0);
    });
  }

  {
    auto materials = make_materials(config, generator);

    require_invalid_argument([&] {
      (void)protocol_iii_dpf_routing_party(
          config,
          materials.first,
          shares.party0,
          -1);
    });
  }
}

}  // namespace

int main() {
  try {
    run_case(
        {{7U},
         1U,
         UINT64_C(0xc001),
         UINT64_C(0xd001),
         UINT64_C(0xe001)});

    run_case(
        {{5U, 5U, 9U},
         2U,
         UINT64_C(0xc002),
         UINT64_C(0xd002),
         UINT64_C(0xe002)});

    run_case(
        {{UINT32_C(0x80000000),
          UINT32_MAX,
          0U,
          UINT32_C(0x7fffffff),
          0U},
         3U,
         UINT64_C(0xc003),
         UINT64_C(0xd003),
         UINT64_C(0xe003)});

    run_case(
        {{11U, 4U, 19U, 4U, 7U, 19U, 2U},
         4U,
         UINT64_C(0xc004),
         UINT64_C(0xd004),
         UINT64_C(0xe004)});

    test_binding_rejection();

    return 0;
  } catch (const std::exception& error) {
    std::cerr
        << "Protocol III DPF routing test failure: "
        << error.what()
        << '\n';

    return 1;
  }
}
