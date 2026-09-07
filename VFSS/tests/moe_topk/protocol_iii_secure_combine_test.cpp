#include <moe_topk/protocol_iii_secure_combine.h>

#include <moe_topk/topk_oracle.h>

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

std::uint32_t padded_size(std::uint32_t logical_n) {
  std::uint32_t result = 2U;

  while (result < logical_n) {
    result <<= 1U;
  }

  return result;
}

std::uint8_t comparison_bits(std::uint32_t logical_n) {
  return static_cast<std::uint8_t>(
      33U + bit_width(padded_size(logical_n) - 1U));
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
        "secure combine socketpair failed");
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
        "secure combine socket duplication failed");

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

ProtocolIIISecureCombineConfig make_config(
    const TestCase& test,
    std::uint8_t party) {
  ProtocolIIISecureCombineConfig config;

  config.session = test.session;
  config.fingerprint = test.fingerprint;
  config.logical_n =
      static_cast<std::uint32_t>(test.scores.size());
  config.k = test.k;
  config.comparison_bits =
      comparison_bits(config.logical_n);
  config.party = party;
  config.timeout_ms = kTimeoutMs;

  return config;
}

struct SharedInputs {
  std::vector<std::uint64_t> indicator0;
  std::vector<std::uint64_t> indicator1;

  std::vector<std::uint64_t> payload0;
  std::vector<std::uint64_t> payload1;
};

SharedInputs make_shared_inputs(
    const TestCase& test,
    std::mt19937_64& generator) {
  const auto ranks =
      stable_ranks_cmpagg(test.scores);

  const auto logical_n =
      static_cast<std::uint32_t>(test.scores.size());

  const auto cells =
      static_cast<std::size_t>(logical_n) * test.k;

  SharedInputs result;
  result.indicator0.resize(cells);
  result.indicator1.resize(cells);
  result.payload0.resize(logical_n);
  result.payload1.resize(logical_n);

  for (std::size_t input = 0;
       input < logical_n;
       ++input) {
    const auto payload_share0 = generator();

    result.payload0[input] = payload_share0;
    result.payload1[input] =
        UINT64_C(1) - payload_share0;

    for (std::uint32_t target = 0;
         target < test.k;
         ++target) {
      const auto cell =
          input * test.k + target;

      const std::uint64_t indicator =
          ranks[input] == target ? 1U : 0U;

      result.indicator0[cell] = generator();
      result.indicator1[cell] =
          indicator - result.indicator0[cell];
    }
  }

  return result;
}

std::pair<
    ProtocolIIISecureCombinePartyMaterial,
    ProtocolIIISecureCombinePartyMaterial>
make_materials(
    const ProtocolIIISecureCombineConfig& config,
    std::mt19937_64& generator) {
  ProtocolIIISecureCombinePartyMaterial party0;
  ProtocolIIISecureCombinePartyMaterial party1;

  for (auto* material : {&party0, &party1}) {
    material->session = config.session;
    material->fingerprint = config.fingerprint;
    material->logical_n = config.logical_n;
    material->k = config.k;
  }

  party0.party = 0;
  party1.party = 1;

  const auto cells =
      static_cast<std::size_t>(config.logical_n) *
      config.k;

  party0.multiplication_materials.reserve(cells);
  party1.multiplication_materials.reserve(cells);

  for (std::size_t cell = 0;
       cell < cells;
       ++cell) {
    auto generated =
        generate_masked_mul_material(
            generator(),
            generator(),
            generator());

    party0.multiplication_materials.push_back(
        std::move(generated.party0));

    party1.multiplication_materials.push_back(
        std::move(generated.party1));
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

  auto inputs =
      make_shared_inputs(test, generator);

  auto materials =
      make_materials(config0, generator);

  SocketPair sockets;

  // Keep the original socket endpoints open while both framed channels
  // consume and close their duplicate descriptors.
  const int party0_fd = sockets.duplicate(0);
  const int party1_fd = sockets.duplicate(1);

  ProtocolIIISecureCombineOutput output0;
  ProtocolIIISecureCombineOutput output1;

  std::exception_ptr error0;
  std::exception_ptr error1;

  std::thread party0([&] {
    try {
      output0 =
          protocol_iii_secure_combine_party(
              config0,
              materials.first,
              inputs.indicator0,
              inputs.payload0,
              party0_fd);
    } catch (...) {
      error0 = std::current_exception();
    }
  });

  std::thread party1([&] {
    try {
      output1 =
          protocol_iii_secure_combine_party(
              config1,
              materials.second,
              inputs.indicator1,
              inputs.payload1,
              party1_fd);
    } catch (...) {
      error1 = std::current_exception();
    }
  });

  party0.join();
  party1.join();

  rethrow_if_present(error0);
  rethrow_if_present(error1);

  require(
      output0.xor_mask_shares.size() ==
          config0.logical_n,
      "P0 XOR-mask length mismatch");

  require(
      output1.xor_mask_shares.size() ==
          config0.logical_n,
      "P1 XOR-mask length mismatch");

  const auto expected =
      top_k_mask(test.scores, test.k);

  require(
      expected.size() == config0.logical_n,
      "oracle mask length mismatch");

  std::size_t selected = 0;

  for (std::size_t input = 0;
       input < config0.logical_n;
       ++input) {
    require(
        output0.xor_mask_shares[input] <= 1U,
        "P0 output is not an XOR bit share");

    require(
        output1.xor_mask_shares[input] <= 1U,
        "P1 output is not an XOR bit share");

    // TEST_ONLY reconstruction.
    const auto reconstructed =
        static_cast<std::uint8_t>(
            output0.xor_mask_shares[input] ^
            output1.xor_mask_shares[input]);

    require(
        reconstructed == expected[input],
        "secure combine differs from Top-K oracle");

    selected += reconstructed;
  }

  require(
      selected == test.k,
      "secure combine did not select exactly K positions");

  const auto expected_calls =
      static_cast<std::uint64_t>(config0.logical_n) *
      config0.k;

  for (const auto* output : {&output0, &output1}) {
    require(
        output->metrics.sent_bytes > 0,
        "secure combine sent-byte metric is empty");

    require(
        output->metrics.received_bytes > 0,
        "secure combine received-byte metric is empty");

    require(
        output->metrics.multiplication_calls ==
            expected_calls,
        "secure combine multiplication metric mismatch");

    require(
        output->metrics.opened_masked_values ==
            expected_calls * 2U,
        "secure combine opened-value metric mismatch");

    require(
        output->metrics.online_rounds == 1U,
        "secure combine must report one online round");
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
      materials.first.multiplication_materials.empty(),
      "P0 multiplication material was not consumed");

  require(
      materials.second.multiplication_materials.empty(),
      "P1 multiplication material was not consumed");
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
      "invalid secure-combine input was accepted");
}

void test_binding_rejection() {
  const TestCase test{
      {5U, 5U, 9U},
      2U,
      UINT64_C(0x1401),
      UINT64_C(0x2401),
      UINT64_C(0x3401)};

  const auto config = make_config(test, 0);

  seed_fss(test.seed);

  std::mt19937_64 generator(test.seed);

  auto inputs = make_shared_inputs(test, generator);

  {
    auto materials = make_materials(config, generator);
    materials.first.session += 1U;

    require_invalid_argument([&] {
      (void)protocol_iii_secure_combine_party(
          config,
          materials.first,
          inputs.indicator0,
          inputs.payload0,
          0);
    });
  }

  {
    auto materials = make_materials(config, generator);
    materials.first.multiplication_materials.pop_back();

    require_invalid_argument([&] {
      (void)protocol_iii_secure_combine_party(
          config,
          materials.first,
          inputs.indicator0,
          inputs.payload0,
          0);
    });
  }

  {
    auto materials = make_materials(config, generator);
    auto short_indicators = inputs.indicator0;
    short_indicators.pop_back();

    require_invalid_argument([&] {
      (void)protocol_iii_secure_combine_party(
          config,
          materials.first,
          short_indicators,
          inputs.payload0,
          0);
    });
  }

  {
    auto materials = make_materials(config, generator);
    auto short_payloads = inputs.payload0;
    short_payloads.pop_back();

    require_invalid_argument([&] {
      (void)protocol_iii_secure_combine_party(
          config,
          materials.first,
          inputs.indicator0,
          short_payloads,
          0);
    });
  }

  {
    auto materials = make_materials(config, generator);

    require_invalid_argument([&] {
      (void)protocol_iii_secure_combine_party(
          config,
          materials.first,
          inputs.indicator0,
          inputs.payload0,
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
         UINT64_C(0x4101),
         UINT64_C(0x5101),
         UINT64_C(0x6101)});

    run_case(
        {{5U, 5U, 9U},
         1U,
         UINT64_C(0x4102),
         UINT64_C(0x5102),
         UINT64_C(0x6102)});

    run_case(
        {{5U, 5U, 5U},
         3U,
         UINT64_C(0x4103),
         UINT64_C(0x5103),
         UINT64_C(0x6103)});

    run_case(
        {{UINT32_C(0x80000000),
          UINT32_MAX,
          0U,
          UINT32_C(0x7fffffff),
          0U},
         3U,
         UINT64_C(0x4104),
         UINT64_C(0x5104),
         UINT64_C(0x6104)});

    run_case(
        {{11U, 4U, 19U, 4U, 7U},
         5U,
         UINT64_C(0x4105),
         UINT64_C(0x5105),
         UINT64_C(0x6105)});

    run_case(
        {{8U, 3U, 8U, 1U, 9U, 4U, 9U, 2U},
         4U,
         UINT64_C(0x4106),
         UINT64_C(0x5106),
         UINT64_C(0x6106)});

    test_binding_rejection();

    return 0;
  } catch (const std::exception& error) {
    std::cerr
        << "Protocol III secure combine test failure: "
        << error.what()
        << '\n';

    return 1;
  }
}
