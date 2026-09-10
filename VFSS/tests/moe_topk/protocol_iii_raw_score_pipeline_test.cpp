#include <moe_topk/masked_mul_adapter.h>
#include <moe_topk/protocol_i_ucmp.h>
#include <moe_topk/protocol_iii_raw_score_pipeline.h>
#include <moe_topk/topk_oracle.h>

#include <FSS/dpf.h>
#include <FSS/prng.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
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

constexpr int kTimeoutMs = 15000;
constexpr int kDpfPayloadBits = 64;

struct TestCase {
  std::vector<std::uint32_t> scores;
  std::uint32_t k = 0;
  std::uint64_t session = 0;
  std::uint64_t fingerprint = 0;
  std::uint64_t seed = 0;
};

struct RawShares {
  std::vector<std::uint32_t> party0;
  std::vector<std::uint32_t> party1;
};

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
  std::uint32_t padded_n = 2U;

  while (padded_n < logical_n) {
    padded_n <<= 1U;
  }

  return padded_n;
}

std::uint8_t rank_bits(std::uint32_t logical_n) {
  return logical_n <= 1U
             ? 1U
             : bit_width(logical_n - 1U);
}

std::uint64_t low_mask(std::uint8_t bits) {
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
        ::socketpair(
            AF_UNIX,
            SOCK_STREAM,
            0,
            descriptors_.data()) == 0,
        "socketpair creation failed");
  }

  ~SocketPair() {
    for (auto& descriptor : descriptors_) {
      if (descriptor >= 0) {
        ::close(descriptor);
        descriptor = -1;
      }
    }
  }

  SocketPair(const SocketPair&) = delete;
  SocketPair& operator=(const SocketPair&) = delete;

  int duplicate(std::size_t endpoint) const {
    require(endpoint < descriptors_.size(), "socket endpoint");

    int result = -1;

    do {
      result = ::dup(descriptors_[endpoint]);
    } while (result < 0 && errno == EINTR);

    require(result >= 0, "socket duplicate failed");
    return result;
  }

 private:
  std::array<int, 2> descriptors_{{-1, -1}};
};

ProtocolIIIRawScorePipelineConfig make_config(
    const TestCase& test,
    std::uint8_t party) {
  const auto logical_n =
      static_cast<std::uint32_t>(test.scores.size());

  const auto padded_n = padded_size(logical_n);
  const auto index_bits = bit_width(padded_n - 1U);
  const auto comparison_bits =
      static_cast<std::uint8_t>(33U + index_bits);

  ProtocolIIIRawScorePipelineConfig config;

  config.score_input.session = test.session;
  config.score_input.fingerprint = test.fingerprint;
  config.score_input.logical_n = logical_n;
  config.score_input.padded_n = padded_n;
  config.score_input.k = test.k;
  config.score_input.index_bits = index_bits;
  config.score_input.comparison_bits = comparison_bits;
  config.score_input.party = party;
  config.score_input.timeout_ms = kTimeoutMs;

  config.grank.session = test.session;
  config.grank.fingerprint = test.fingerprint;
  config.grank.logical_n = logical_n;
  config.grank.padded_n = padded_n;
  config.grank.k = test.k;
  config.grank.comparison_bits = comparison_bits;
  config.grank.rank_bits = rank_bits(logical_n);
  config.grank.party = party;
  config.grank.timeout_ms = kTimeoutMs;

  config.routing.session = test.session;
  config.routing.fingerprint = test.fingerprint;
  config.routing.logical_n = logical_n;
  config.routing.k = test.k;
  config.routing.rank_bits = config.grank.rank_bits;
  config.routing.comparison_bits = comparison_bits;
  config.routing.party = party;
  config.routing.timeout_ms = kTimeoutMs;

  config.combine.session = test.session;
  config.combine.fingerprint = test.fingerprint;
  config.combine.logical_n = logical_n;
  config.combine.k = test.k;
  config.combine.comparison_bits = comparison_bits;
  config.combine.party = party;
  config.combine.timeout_ms = kTimeoutMs;

  return config;
}

RawShares share_raw_scores(
    const TestCase& test,
    std::mt19937_64& generator) {
  RawShares shares;

  shares.party0.resize(test.scores.size());
  shares.party1.resize(test.scores.size());

  for (std::size_t index = 0;
       index < test.scores.size();
       ++index) {
    shares.party0[index] =
        static_cast<std::uint32_t>(generator());

    shares.party1[index] =
        test.scores[index] - shares.party0[index];
  }

  return shares;
}

std::pair<
    ProtocolIIIRawScorePipelineMaterial,
    ProtocolIIIRawScorePipelineMaterial>
make_materials(
    const ProtocolIIIRawScorePipelineConfig& config,
    std::mt19937_64& generator) {
  constexpr std::uint8_t kScoreBits = 34U;

  const auto comparison_ring =
      low_mask(config.grank.comparison_bits);

  const auto score_ring = low_mask(kScoreBits);
  const auto rank_ring =
      low_mask(config.grank.rank_bits);

  ProtocolIIIRawScorePipelineMaterial party0;
  ProtocolIIIRawScorePipelineMaterial party1;

  for (auto* material : {&party0, &party1}) {
    auto& score_package =
        material->score_input_package;

    score_package.session = config.score_input.session;
    score_package.fingerprint =
        config.score_input.fingerprint;
    score_package.n = config.score_input.padded_n;
    score_package.k = config.score_input.k;
    score_package.comparison_bits =
        config.score_input.comparison_bits;

    auto& grank_package =
        material->grank_package;

    grank_package.session = config.grank.session;
    grank_package.fingerprint =
        config.grank.fingerprint;
    grank_package.n = config.grank.padded_n;
    grank_package.k = config.grank.k;
    grank_package.comparison_bits =
        config.grank.comparison_bits;

    material->routing_material.session =
        config.routing.session;
    material->routing_material.fingerprint =
        config.routing.fingerprint;
    material->routing_material.logical_n =
        config.routing.logical_n;
    material->routing_material.k =
        config.routing.k;
    material->routing_material.rank_bits =
        config.routing.rank_bits;

    material->combine_material.session =
        config.combine.session;
    material->combine_material.fingerprint =
        config.combine.fingerprint;
    material->combine_material.logical_n =
        config.combine.logical_n;
    material->combine_material.k =
        config.combine.k;

    material->unit_payload_shares.resize(
        config.combine.logical_n);
  }

  party0.score_input_package.party = 0;
  party1.score_input_package.party = 1;

  party0.grank_package.party = 0;
  party1.grank_package.party = 1;

  party0.routing_material.party = 0;
  party1.routing_material.party = 1;

  party0.combine_material.party = 0;
  party1.combine_material.party = 1;

  // Raw-score adapter carry/sign materials.
  for (const auto stage :
       {UINT8_C(1), UINT8_C(2)}) {
    for (std::uint32_t slot = 0;
         slot < config.score_input.padded_n;
         ++slot) {
      const auto full_left =
          generator() & score_ring;

      const auto full_right =
          generator() & score_ring;

      const auto left0 =
          generator() & score_ring;

      const auto right0 =
          generator() & score_ring;

      ProtocolIUcmpMaterial generated(
          kScoreBits,
          full_left,
          full_right);

      ProtocolIScoreInputPartyMaterial item0(
          slot,
          stage,
          left0,
          right0,
          generated.export_party_material(0));

      ProtocolIScoreInputPartyMaterial item1(
          slot,
          stage,
          (full_left - left0) & score_ring,
          (full_right - right0) & score_ring,
          generated.export_party_material(1));

      if (stage == 1U) {
        party0.score_input_package.carry_materials.push_back(
            std::move(item0));

        party1.score_input_package.carry_materials.push_back(
            std::move(item1));
      } else {
        party0.score_input_package.sign_materials.push_back(
            std::move(item0));

        party1.score_input_package.sign_materials.push_back(
            std::move(item1));
      }
    }
  }

  // GRank node masks and complete comparison graph.
  party0.grank_package.node_mask_shares.resize(
      config.grank.padded_n);

  party1.grank_package.node_mask_shares.resize(
      config.grank.padded_n);

  std::vector<std::uint64_t> full_node_masks(
      config.grank.padded_n);

  for (std::uint32_t index = 0;
       index < config.grank.padded_n;
       ++index) {
    full_node_masks[index] =
        generator() & comparison_ring;

    party0.grank_package.node_mask_shares[index] =
        generator() & comparison_ring;

    party1.grank_package.node_mask_shares[index] =
        (full_node_masks[index] -
         party0.grank_package.node_mask_shares[index]) &
        comparison_ring;
  }

  for (std::uint32_t left = 0;
       left < config.grank.padded_n;
       ++left) {
    for (std::uint32_t right = left + 1U;
         right < config.grank.padded_n;
         ++right) {
      ProtocolIUcmpMaterial generated(
          config.grank.comparison_bits,
          full_node_masks[left],
          full_node_masks[right]);

      party0.grank_package.edge_materials.emplace_back(
          left,
          right,
          generated.export_party_material(0));

      party1.grank_package.edge_materials.emplace_back(
          left,
          right,
          generated.export_party_material(1));
    }
  }

  // DPF routing material.
  party0.routing_material.rank_mask_shares.resize(
      config.routing.logical_n);

  party1.routing_material.rank_mask_shares.resize(
      config.routing.logical_n);

  party0.routing_material.dpf_keys.reserve(
      config.routing.logical_n);

  party1.routing_material.dpf_keys.reserve(
      config.routing.logical_n);

  for (std::uint32_t index = 0;
       index < config.routing.logical_n;
       ++index) {
    const auto full_mask =
        generator() & rank_ring;

    const auto party0_mask =
        generator() & rank_ring;

    party0.routing_material.rank_mask_shares[index] =
        party0_mask;

    party1.routing_material.rank_mask_shares[index] =
        (full_mask - party0_mask) & rank_ring;

    auto keys = keyGenDPF(
        config.routing.rank_bits,
        kDpfPayloadBits,
        full_mask,
        1);

    party0.routing_material.dpf_keys.emplace_back(
        std::move(keys.first));

    party1.routing_material.dpf_keys.emplace_back(
        std::move(keys.second));
  }

  // Unit payload shares.
  for (std::uint32_t index = 0;
       index < config.combine.logical_n;
       ++index) {
    party0.unit_payload_shares[index] = generator();

    party1.unit_payload_shares[index] =
        UINT64_C(1) -
        party0.unit_payload_shares[index];
  }

  // Secure-combine multiplication material.
  const auto cells =
      static_cast<std::size_t>(
          config.combine.logical_n) *
      config.combine.k;

  party0.combine_material.multiplication_materials.reserve(
      cells);

  party1.combine_material.multiplication_materials.reserve(
      cells);

  for (std::size_t cell = 0;
       cell < cells;
       ++cell) {
    auto generated =
        generate_masked_mul_material(
            generator(),
            generator(),
            generator());

    party0.combine_material.multiplication_materials.push_back(
        std::move(generated.party0));

    party1.combine_material.multiplication_materials.push_back(
        std::move(generated.party1));
  }

  return {
      std::move(party0),
      std::move(party1)};
}

ProtocolIIIRawScorePipelineFds make_party_fds(
    const std::array<SocketPair, 5>& sockets,
    std::size_t endpoint) {
  ProtocolIIIRawScorePipelineFds fds;

  fds.score_input_fds = {{
      sockets[0].duplicate(endpoint),
      sockets[1].duplicate(endpoint),
  }};

  fds.grank_fd = sockets[2].duplicate(endpoint);
  fds.routing_fd = sockets[3].duplicate(endpoint);
  fds.combine_fd = sockets[4].duplicate(endpoint);

  return fds;
}

void rethrow_if_present(
    const std::exception_ptr& error) {
  if (error) {
    std::rethrow_exception(error);
  }
}

void run_case(const TestCase& test) {
  require(!test.scores.empty(), "empty test case");

  require(
      test.k >= 1U &&
          test.k <= test.scores.size(),
      "invalid test k");

  const auto config0 = make_config(test, 0);
  const auto config1 = make_config(test, 1);

  seed_fss(test.seed);

  std::mt19937_64 generator(test.seed);

  auto raw_shares =
      share_raw_scores(test, generator);

  auto materials =
      make_materials(config0, generator);

  // The original endpoints stay open until both parties return. Each runtime
  // stage consumes and closes only its duplicated descriptor.
  std::array<SocketPair, 5> sockets;

  const auto fds0 = make_party_fds(sockets, 0);
  const auto fds1 = make_party_fds(sockets, 1);

  ProtocolIIIRawScorePipelineOutput output0;
  ProtocolIIIRawScorePipelineOutput output1;

  std::exception_ptr error0;
  std::exception_ptr error1;

  std::thread party0([&] {
    try {
      output0 =
          protocol_iii_raw_score_pipeline_party(
              config0,
              materials.first,
              raw_shares.party0,
              fds0);
    } catch (...) {
      error0 = std::current_exception();
    }
  });

  std::thread party1([&] {
    try {
      output1 =
          protocol_iii_raw_score_pipeline_party(
              config1,
              materials.second,
              raw_shares.party1,
              fds1);
    } catch (...) {
      error1 = std::current_exception();
    }
  });

  party0.join();
  party1.join();

  rethrow_if_present(error0);
  rethrow_if_present(error1);

  const auto expected =
      top_k_mask(test.scores, test.k);

  require(
      output0.xor_mask_shares.size() ==
              test.scores.size() &&
          output1.xor_mask_shares.size() ==
              test.scores.size(),
      "raw-score pipeline output size");

  std::size_t selected = 0;

  for (std::size_t index = 0;
       index < test.scores.size();
       ++index) {
    require(
        output0.xor_mask_shares[index] <= 1U &&
            output1.xor_mask_shares[index] <= 1U,
        "raw-score pipeline output is not an XOR bit share");

    const auto reconstructed =
        static_cast<std::uint8_t>(
            output0.xor_mask_shares[index] ^
            output1.xor_mask_shares[index]);

    require(
        reconstructed == expected[index],
        "raw-score pipeline differs from oracle");

    selected += reconstructed;
  }

  require(
      selected == test.k,
      "raw-score pipeline selected-count mismatch");

  for (const auto* output : {&output0, &output1}) {
    require(
        output->metrics.input_adapter_rounds == 2U,
        "raw-score adapter rounds");

    require(
        output->metrics.core_rounds == 3U,
        "priority-key core rounds");

    require(
        output->metrics.total_online_rounds == 5U,
        "raw-score total rounds");

    require(
        output->metrics.score_input.ucmp_calls ==
            2U * config0.score_input.padded_n,
        "raw-score uCMP calls");

    require(
        output->metrics.score_input.raw_dcf_calls ==
            4U * config0.score_input.padded_n,
        "raw-score DCF calls");

    require(
        output->metrics.sent_bytes > 0U &&
            output->metrics.received_bytes > 0U,
        "raw-score pipeline communication metrics");
  }

  require(
      output0.metrics.sent_bytes ==
          output1.metrics.received_bytes &&
          output1.metrics.sent_bytes ==
              output0.metrics.received_bytes,
      "raw-score pipeline communication mismatch");

  require(
      materials.first.score_input_package
              .carry_materials.empty() &&
          materials.first.score_input_package
              .sign_materials.empty() &&
          materials.second.score_input_package
              .carry_materials.empty() &&
          materials.second.score_input_package
              .sign_materials.empty(),
      "raw-score materials were not consumed");

  require(
      materials.first.grank_package
              .node_mask_shares.empty() &&
          materials.first.grank_package
              .edge_materials.empty() &&
          materials.second.grank_package
              .node_mask_shares.empty() &&
          materials.second.grank_package
              .edge_materials.empty(),
      "GRank materials were not consumed");

  require(
      materials.first.routing_material
              .rank_mask_shares.empty() &&
          materials.first.routing_material
              .dpf_keys.empty() &&
          materials.second.routing_material
              .rank_mask_shares.empty() &&
          materials.second.routing_material
              .dpf_keys.empty(),
      "routing materials were not consumed");

  require(
      materials.first.combine_material
              .multiplication_materials.empty() &&
          materials.second.combine_material
              .multiplication_materials.empty(),
      "combine materials were not consumed");
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
      "invalid raw-score pipeline input was accepted");
}

void test_cross_stage_binding_rejection() {
  const TestCase test{
      {5U, 5U, 9U},
      2U,
      UINT64_C(0x5101),
      UINT64_C(0x6101),
      UINT64_C(0x7101)};

  auto config = make_config(test, 0);
  config.routing.session += 1U;

  ProtocolIIIRawScorePipelineMaterial material;
  ProtocolIIIRawScorePipelineFds fds;

  require_invalid_argument([&] {
    (void)protocol_iii_raw_score_pipeline_party(
        config,
        material,
        test.scores,
        fds);
  });
}

}  // namespace

int main() {
  try {
    run_case(
        {{7U},
         1U,
         UINT64_C(0x5201),
         UINT64_C(0x6201),
         UINT64_C(0x7201)});

    run_case(
        {{5U, 5U, 9U},
         2U,
         UINT64_C(0x5202),
         UINT64_C(0x6202),
         UINT64_C(0x7202)});

    run_case(
        {{5U, 5U, 5U},
         3U,
         UINT64_C(0x5203),
         UINT64_C(0x6203),
         UINT64_C(0x7203)});

    run_case(
        {{UINT32_C(0x80000000),
          UINT32_MAX,
          0U,
          UINT32_C(0x7fffffff),
          0U},
         3U,
         UINT64_C(0x5204),
         UINT64_C(0x6204),
         UINT64_C(0x7204)});

    run_case(
        {{11U, 4U, 19U, 4U, 7U, 19U, 2U},
         1U,
         UINT64_C(0x5205),
         UINT64_C(0x6205),
         UINT64_C(0x7205)});

    run_case(
        {{8U, 3U, 8U, 1U, 9U, 4U, 9U, 2U},
         8U,
         UINT64_C(0x5206),
         UINT64_C(0x6206),
         UINT64_C(0x7206)});

    test_cross_stage_binding_rejection();

    std::cout
        << "Protocol III raw-score pipeline passed\n";

    return 0;
  } catch (const std::exception& error) {
    std::cerr
        << "Protocol III raw-score pipeline failure: "
        << error.what()
        << '\n';

    return 1;
  }
}
