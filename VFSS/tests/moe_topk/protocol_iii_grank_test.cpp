#include <moe_topk/protocol_iii_grank.h>

#include <moe_topk/protocol_i_priority_key.h>
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
  std::uint32_t padded_n = 2U;

  while (padded_n < logical_n) {
    padded_n <<= 1U;
  }

  return padded_n;
}

std::uint8_t required_rank_bits(std::uint32_t logical_n) {
  return logical_n <= 1U
             ? 1U
             : bit_width(logical_n - 1U);
}

std::uint8_t required_comparison_bits(std::uint32_t padded_n) {
  return static_cast<std::uint8_t>(
      33U + bit_width(padded_n - 1U));
}

std::uint64_t ring_mask(std::uint8_t bits) {
  return (UINT64_C(1) << bits) - 1U;
}

std::uint64_t edge_count(std::uint32_t n) {
  return static_cast<std::uint64_t>(n) *
         static_cast<std::uint64_t>(n - 1U) / 2U;
}

void seed_fss(std::uint64_t seed) {
  for (int index = 0; index < 256; ++index) {
    FSSConfig::prngs[index].SetSeed(
        osuCrypto::toBlock(
            seed,
            static_cast<std::uint64_t>(index)));
  }
}

// ========== 修改：添加 duplicate 方法 ==========
class SocketPair final {
 public:
  SocketPair() {
    require(
        ::socketpair(AF_UNIX, SOCK_STREAM, 0, fds_.data()) == 0,
        "socketpair creation failed");
  }

  SocketPair(const SocketPair&) = delete;
  SocketPair& operator=(const SocketPair&) = delete;

  ~SocketPair() {
    for (auto& fd : fds_) {
      if (fd >= 0) {
        ::close(fd);
        fd = -1;
      }
    }
  }

  int operator[](std::size_t index) const {
    return fds_[index];
  }

  // 新增：复制文件描述符，供 ProtocolIFramedChannel 独占所有权
  int duplicate(std::size_t index) const {
    const int duplicated_fd = ::dup(fds_[index]);

    require(
        duplicated_fd >= 0,
        "socket descriptor duplication failed");

    return duplicated_fd;
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

ProtocolIIIGrankConfig make_config(
    const TestCase& test,
    std::uint8_t party) {
  const auto logical_n =
      static_cast<std::uint32_t>(test.scores.size());

  const auto padded_n =
      padded_size(logical_n);

  ProtocolIIIGrankConfig config;
  config.session = test.session;
  config.fingerprint = test.fingerprint;
  config.logical_n = logical_n;
  config.padded_n = padded_n;
  config.k = test.k;
  config.comparison_bits =
      required_comparison_bits(padded_n);
  config.rank_bits =
      required_rank_bits(logical_n);
  config.party = party;
  config.timeout_ms = kTimeoutMs;

  return config;
}

std::vector<std::uint64_t> make_plaintext_keys(
    const TestCase& test,
    const ProtocolIIIGrankConfig& config) {
  std::vector<std::uint64_t> keys(config.padded_n);

  for (std::uint32_t index = 0;
       index < config.logical_n;
       ++index) {
    keys[index] =
        protocol_i_priority_key(
            test.scores[index],
            index,
            config.padded_n)
            .value;
  }

  // Frozen M2 padding: padded slots represent the minimum signed score.
  for (std::uint32_t index = config.logical_n;
       index < config.padded_n;
       ++index) {
    keys[index] =
        protocol_i_priority_key(
            UINT32_C(0x80000000),
            index,
            config.padded_n)
            .value;
  }

  return keys;
}

struct PriorityKeyShares {
  std::vector<std::uint64_t> party0;
  std::vector<std::uint64_t> party1;
};

PriorityKeyShares share_priority_keys(
    const std::vector<std::uint64_t>& plaintext_keys,
    std::uint8_t comparison_bits,
    std::mt19937_64& generator) {
  const auto mask = ring_mask(comparison_bits);

  PriorityKeyShares result;
  result.party0.resize(plaintext_keys.size());
  result.party1.resize(plaintext_keys.size());

  for (std::size_t index = 0;
       index < plaintext_keys.size();
       ++index) {
    result.party0[index] = generator() & mask;

    result.party1[index] =
        (plaintext_keys[index] -
         result.party0[index]) &
        mask;
  }

  return result;
}

std::pair<ProtocolIPartyPackage, ProtocolIPartyPackage>
make_grank_packages(
    const ProtocolIIIGrankConfig& config,
    std::mt19937_64& generator) {
  const auto mask =
      ring_mask(config.comparison_bits);

  ProtocolIPartyPackage party0;
  ProtocolIPartyPackage party1;

  for (auto* package : {&party0, &party1}) {
    package->session = config.session;
    package->fingerprint = config.fingerprint;
    package->comparison_bits =
        config.comparison_bits;
    package->n = config.padded_n;
    package->k = config.k;
  }

  party0.party = 0;
  party1.party = 1;

  party0.node_mask_shares.resize(config.padded_n);
  party1.node_mask_shares.resize(config.padded_n);

  std::vector<std::uint64_t> full_masks(
      config.padded_n);

  for (std::uint32_t index = 0;
       index < config.padded_n;
       ++index) {
    full_masks[index] = generator() & mask;

    party0.node_mask_shares[index] =
        generator() & mask;

    party1.node_mask_shares[index] =
        (full_masks[index] -
         party0.node_mask_shares[index]) &
        mask;
  }

  for (std::uint32_t left = 0;
       left < config.padded_n;
       ++left) {
    for (std::uint32_t right = left + 1U;
         right < config.padded_n;
         ++right) {
      ProtocolIUcmpMaterial material(
          config.comparison_bits,
          full_masks[left],
          full_masks[right]);

      party0.edge_materials.emplace_back(
          left,
          right,
          material.export_party_material(0));

      party1.edge_materials.emplace_back(
          left,
          right,
          material.export_party_material(1));
    }
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

void verify_metrics(
    const ProtocolIIIGrankConfig& config,
    const ProtocolIIIGrankOutput& output0,
    const ProtocolIIIGrankOutput& output1) {
  const auto expected_edges =
      edge_count(config.padded_n);

  for (const auto* output : {&output0, &output1}) {
    require(
        output->metrics.sent_bytes > 0,
        "sent-byte metric must be non-zero");

    require(
        output->metrics.received_bytes > 0,
        "received-byte metric must be non-zero");

    require(
        output->metrics.comparison_edges == expected_edges,
        "comparison-edge metric mismatch");

    require(
        output->metrics.ucmp_calls == expected_edges,
        "Ucmp call metric mismatch");

    require(
        output->metrics.raw_dcf_calls ==
            expected_edges * 2U,
        "raw DCF call metric mismatch");

    require(
        output->metrics.online_rounds == 1U,
        "GRank must use one online round");
  }

  require(
      output0.metrics.sent_bytes ==
          output1.metrics.received_bytes,
      "P0 sent bytes differ from P1 received bytes");

  require(
      output1.metrics.sent_bytes ==
          output0.metrics.received_bytes,
      "P1 sent bytes differ from P0 received bytes");
}

void run_differential_case(const TestCase& test) {
  require(
      !test.scores.empty(),
      "test score list must be non-empty");

  require(
      test.k >= 1U &&
          test.k <= test.scores.size(),
      "test k is invalid");

  const auto config0 = make_config(test, 0);
  const auto config1 = make_config(test, 1);

  seed_fss(test.seed);

  std::mt19937_64 generator(test.seed);

  const auto plaintext_keys =
      make_plaintext_keys(test, config0);

  auto key_shares =
      share_priority_keys(
          plaintext_keys,
          config0.comparison_bits,
          generator);

  auto packages =
      make_grank_packages(config0, generator);

  // ========== 修改：使用 dup 副本，避免 POLLHUP 竞争 ==========
  SocketPair sockets;

  // ProtocolIFramedChannel 拥有并关闭传入的 fd。
  // 保持原始 socket pair 描述符打开直至双方结束，
  // 这样一方不会在另一方读取最后帧时触发 POLLHUP。
  const int party0_fd = sockets.duplicate(0);
  const int party1_fd = sockets.duplicate(1);

  ProtocolIIIGrankOutput output0;
  ProtocolIIIGrankOutput output1;

  std::exception_ptr error0;
  std::exception_ptr error1;

  std::thread party0([&] {
    try {
      output0 = protocol_iii_grank_party(
          config0,
          packages.first,
          key_shares.party0,
          party0_fd);          // 使用副本
    } catch (...) {
      error0 = std::current_exception();
    }
  });

  std::thread party1([&] {
    try {
      output1 = protocol_iii_grank_party(
          config1,
          packages.second,
          key_shares.party1,
          party1_fd);          // 使用副本
    } catch (...) {
      error1 = std::current_exception();
    }
  });

  party0.join();
  party1.join();

  // 注意：不手动关闭 party0_fd / party1_fd，它们由 ProtocolIFramedChannel 析构关闭。
  // 原始 fd 由 SocketPair 的析构函数关闭（sockets 在此函数返回时销毁）。

  rethrow_if_present(error0);
  rethrow_if_present(error1);

  require(
      output0.rank_additive_shares.size() ==
          config0.logical_n,
      "P0 rank-share length mismatch");

  require(
      output1.rank_additive_shares.size() ==
          config0.logical_n,
      "P1 rank-share length mismatch");

  const auto expected_ranks =
      stable_ranks_cmpagg(test.scores);

  require(
      expected_ranks.size() == config0.logical_n,
      "oracle rank length mismatch");

  const auto rank_mask =
      ring_mask(config0.rank_bits);

  std::vector<bool> seen(config0.logical_n, false);

  for (std::size_t index = 0;
       index < expected_ranks.size();
       ++index) {
    require(
        (output0.rank_additive_shares[index] &
         ~rank_mask) == 0,
        "P0 rank share is outside rank ring");

    require(
        (output1.rank_additive_shares[index] &
         ~rank_mask) == 0,
        "P1 rank share is outside rank ring");

    // Reconstruction exists only inside this TEST_ONLY oracle boundary.
    const auto reconstructed_rank =
        (output0.rank_additive_shares[index] +
         output1.rank_additive_shares[index]) &
        rank_mask;

    require(
        reconstructed_rank == expected_ranks[index],
        "GRank differs from stable-rank oracle");

    require(
        reconstructed_rank < config0.logical_n,
        "logical output contains padded rank");

    require(
        !seen[reconstructed_rank],
        "GRank output is not a permutation");

    seen[reconstructed_rank] = true;
  }

  for (const auto present : seen) {
    require(
        present,
        "GRank permutation has a gap");
  }

  verify_metrics(config0, output0, output1);

  require(
      packages.first.node_mask_shares.empty() &&
          packages.first.edge_materials.empty(),
      "P0 one-shot material was not consumed");

  require(
      packages.second.node_mask_shares.empty() &&
          packages.second.edge_materials.empty(),
      "P1 one-shot material was not consumed");
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
      "invalid GRank configuration was accepted");
}

void test_binding_rejection() {
  const TestCase test{
      {5U, 5U, 9U},
      2U,
      UINT64_C(0x6101),
      UINT64_C(0x7101),
      UINT64_C(0x8101)};

  const auto config = make_config(test, 0);

  seed_fss(test.seed);

  std::mt19937_64 generator(test.seed);

  const auto plaintext_keys =
      make_plaintext_keys(test, config);

  auto key_shares =
      share_priority_keys(
          plaintext_keys,
          config.comparison_bits,
          generator);

  {
    auto packages =
        make_grank_packages(config, generator);

    packages.first.session += 1U;

    require_invalid_argument([&] {
      (void)protocol_iii_grank_party(
          config,
          packages.first,
          key_shares.party0,
          0);
    });
  }

  {
    auto packages =
        make_grank_packages(config, generator);

    packages.first.fingerprint += 1U;

    require_invalid_argument([&] {
      (void)protocol_iii_grank_party(
          config,
          packages.first,
          key_shares.party0,
          0);
    });
  }

  {
    auto packages =
        make_grank_packages(config, generator);

    packages.first.node_mask_shares.pop_back();

    require_invalid_argument([&] {
      (void)protocol_iii_grank_party(
          config,
          packages.first,
          key_shares.party0,
          0);
    });
  }

  {
    auto packages =
        make_grank_packages(config, generator);

    packages.first.edge_materials.pop_back();

    require_invalid_argument([&] {
      (void)protocol_iii_grank_party(
          config,
          packages.first,
          key_shares.party0,
          0);
    });
  }

  {
    auto packages =
        make_grank_packages(config, generator);

    auto short_shares = key_shares.party0;
    short_shares.pop_back();

    require_invalid_argument([&] {
      (void)protocol_iii_grank_party(
          config,
          packages.first,
          short_shares,
          0);
    });
  }

  {
    auto packages =
        make_grank_packages(config, generator);

    require_invalid_argument([&] {
      (void)protocol_iii_grank_party(
          config,
          packages.first,
          key_shares.party0,
          -1);
    });
  }

  {
    auto packages =
        make_grank_packages(config, generator);

    auto outside_ring = key_shares.party0;
    outside_ring[0] =
        UINT64_C(1) << config.comparison_bits;

    require_invalid_argument([&] {
      (void)protocol_iii_grank_party(
          config,
          packages.first,
          outside_ring,
          0);
    });
  }
}

}  // namespace

int main() {
  try {
    run_differential_case(
        {{7U},
         1U,
         UINT64_C(0x3001),
         UINT64_C(0x4001),
         UINT64_C(0x5001)});

    run_differential_case(
        {{5U, 5U, 9U},
         2U,
         UINT64_C(0x3002),
         UINT64_C(0x4002),
         UINT64_C(0x5002)});

    run_differential_case(
        {{UINT32_C(0x80000000),
          UINT32_MAX,
          0U,
          UINT32_C(0x7fffffff),
          0U},
         3U,
         UINT64_C(0x3003),
         UINT64_C(0x4003),
         UINT64_C(0x5003)});

    run_differential_case(
        {{11U, 4U, 19U, 4U, 7U, 19U, 2U},
         4U,
         UINT64_C(0x3004),
         UINT64_C(0x4004),
         UINT64_C(0x5004)});

    test_binding_rejection();

    return 0;
  } catch (const std::exception& error) {
    std::cerr
        << "Protocol III GRank test failure: "
        << error.what()
        << '\n';

    return 1;
  }
}
