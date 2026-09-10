#include <moe_topk/protocol_iii_grank.h>

#include <moe_topk/protocol_i_cmpagg.h>
#include <moe_topk/protocol_i_transport.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace moe_topk {
namespace {

constexpr std::uint32_t kMaximumPaddedN = 1'048'576;

// Reuse the frozen M2 CmpAgg frame identity. Protocol III GRank is the same
// masked CmpAgg primitive without Protocol I's shuffle or rank-reveal stages.
constexpr std::uint8_t kCmpAggPhase = 2;
constexpr std::uint8_t kCmpAggFrameType = 1;

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::invalid_argument(message);
  }
}

bool is_power_of_two(std::uint32_t value) {
  return value != 0 && (value & (value - 1U)) == 0;
}

std::uint32_t required_padded_n(std::uint32_t logical_n) {
  require(
      logical_n >= 1U && logical_n <= 1'000'000U,
      "Protocol III GRank logical_n");

  std::uint32_t padded_n = 2U;

  while (padded_n < logical_n) {
    require(
        padded_n <= kMaximumPaddedN / 2U,
        "Protocol III GRank padded input too large");

    padded_n <<= 1U;
  }

  return padded_n;
}

std::uint8_t bit_width(std::uint32_t maximum_value) {
  std::uint8_t bits = 0;

  while (maximum_value != 0U) {
    ++bits;
    maximum_value >>= 1U;
  }

  return bits;
}

std::uint8_t required_rank_bits(std::uint32_t logical_n) {
  if (logical_n <= 1U) {
    return 1U;
  }

  return bit_width(logical_n - 1U);
}

std::uint8_t required_comparison_bits(std::uint32_t padded_n) {
  const auto index_bits = bit_width(padded_n - 1U);
  return static_cast<std::uint8_t>(33U + index_bits);
}

std::uint64_t low_bit_mask(std::uint8_t bits) {
  require(
      bits >= 1U && bits < 64U,
      "Protocol III GRank ring width");

  return (UINT64_C(1) << bits) - 1U;
}

std::vector<std::uint8_t> encode_words(
    const std::vector<std::uint64_t>& words) {
  std::vector<std::uint8_t> bytes;
  bytes.reserve(words.size() * sizeof(std::uint64_t));

  for (const auto word : words) {
    for (int shift = 56; shift >= 0; shift -= 8) {
      bytes.push_back(
          static_cast<std::uint8_t>(word >> shift));
    }
  }

  return bytes;
}

std::vector<std::uint64_t> decode_words(
    const std::vector<std::uint8_t>& bytes,
    std::size_t expected_words) {
  require(
      bytes.size() == expected_words * sizeof(std::uint64_t),
      "Protocol III GRank frame length");

  std::vector<std::uint64_t> words(expected_words);

  for (std::size_t index = 0; index < expected_words; ++index) {
    for (std::size_t byte = 0; byte < sizeof(std::uint64_t); ++byte) {
      words[index] =
          (words[index] << 8U) |
          bytes[index * sizeof(std::uint64_t) + byte];
    }
  }

  return words;
}

std::uint64_t expected_edge_count(std::uint32_t logical_n) {
  return static_cast<std::uint64_t>(logical_n) *
         static_cast<std::uint64_t>(logical_n - 1U) / 2U;
}

void validate_config(const ProtocolIIIGrankConfig& config) {
  require(
      config.session != 0,
      "Protocol III GRank session");

  require(
      config.fingerprint != 0,
      "Protocol III GRank fingerprint");

  require(
      config.party < 2U,
      "Protocol III GRank party");

  require(
      config.logical_n >= 1U &&
          config.logical_n <= 1'000'000U,
      "Protocol III GRank logical input size");

  require(
      config.padded_n >= 2U &&
          config.padded_n <= kMaximumPaddedN &&
          is_power_of_two(config.padded_n) &&
          config.padded_n == required_padded_n(config.logical_n),
      "Protocol III GRank padded input size");

  require(
      config.k >= 1U && config.k <= config.logical_n,
      "Protocol III GRank k");

  require(
      config.rank_bits == required_rank_bits(config.logical_n),
      "Protocol III GRank rank width");

  const auto minimum_comparison_bits =
      required_comparison_bits(config.padded_n);

  require(
      config.comparison_bits >= minimum_comparison_bits &&
          config.comparison_bits <= 53U,
      "Protocol III GRank comparison width");

  require(
      config.timeout_ms > 0,
      "Protocol III GRank timeout");
}

void validate_package(
    const ProtocolIIIGrankConfig& config,
    const ProtocolIPartyPackage& package) {
  require(
      package.session == config.session,
      "Protocol III GRank package session binding");

  require(
      package.fingerprint == config.fingerprint,
      "Protocol III GRank package fingerprint binding");

  require(
      package.party == static_cast<int>(config.party),
      "Protocol III GRank package party binding");

  require(
      package.n == config.logical_n &&
          package.k == config.k &&
          package.comparison_bits ==
              static_cast<int>(config.comparison_bits),
      "Protocol III GRank package parameter binding");

  require(
      package.node_mask_shares.size() == config.logical_n,
      "Protocol III GRank node-mask count");

  require(
      package.edge_materials.size() ==
          expected_edge_count(config.logical_n),
      "Protocol III GRank edge-material count");

  std::size_t edge_index = 0;

  for (std::uint32_t left = 0;
       left < config.logical_n;
       ++left) {
    for (std::uint32_t right = left + 1U;
         right < config.logical_n;
         ++right) {
      const auto& edge = package.edge_materials[edge_index++];

      require(
          edge.left == left && edge.right == right,
          "Protocol III GRank edge ordering");

      require(
          edge.material.party_id() ==
              static_cast<int>(config.party),
          "Protocol III GRank edge party binding");

      require(
          edge.material.comparison_bits() ==
              static_cast<int>(config.comparison_bits),
          "Protocol III GRank edge width binding");
    }
  }
}

void validate_inputs(
    const ProtocolIIIGrankConfig& config,
    const std::vector<std::uint64_t>& priority_key_shares,
    int grank_fd) {
  require(
      priority_key_shares.size() == config.padded_n,
      "Protocol III GRank priority-key share count");

  require(
      grank_fd >= 0,
      "Protocol III GRank file descriptor");

  const auto comparison_ring_mask =
      low_bit_mask(config.comparison_bits);

  for (const auto share : priority_key_shares) {
    require(
        (share & ~comparison_ring_mask) == 0,
        "Protocol III GRank priority-key share outside ring");
  }
}

}  // namespace

ProtocolIIIGrankOutput protocol_iii_grank_party(
    const ProtocolIIIGrankConfig& config,
    ProtocolIPartyPackage& package,
    const std::vector<std::uint64_t>& priority_key_shares,
    int grank_fd) {
  validate_config(config);
  validate_package(config, package);
  validate_inputs(config, priority_key_shares, grank_fd);

  const auto comparison_ring_mask =
      low_bit_mask(config.comparison_bits);

  const auto rank_ring_mask =
      low_bit_mask(config.rank_bits);

  // Locally add the one-shot node-mask share to each priority-key share.
  std::vector<std::uint64_t> local_masked_keys(config.logical_n);

  for (std::size_t index = 0;
       index < local_masked_keys.size();
       ++index) {
    local_masked_keys[index] =
        protocol_i_mask_priority_key_share(
            config.comparison_bits,
            priority_key_shares[index],
            package.node_mask_shares[index]);
  }

  ProtocolIFrameConfig frame_config{
      config.session,
      config.fingerprint,
      config.logical_n,
      config.k,
      config.comparison_bits,
      config.party,
      static_cast<std::uint8_t>(1U - config.party),
      kCmpAggPhase,
      kCmpAggFrameType};

  ProtocolIFramedChannel channel(
      grank_fd,
      frame_config,
      config.timeout_ms);

  const auto encoded_local =
      encode_words(local_masked_keys);

  std::vector<std::uint8_t> peer_payload;

  // Fixed ordering prevents both parties from blocking on send.
  if (config.party == 0U) {
    channel.send(encoded_local);
    peer_payload = channel.receive();
  } else {
    peer_payload = channel.receive();
    channel.send(encoded_local);
  }

  const auto peer_masked_keys =
      decode_words(peer_payload, config.logical_n);

  std::vector<std::uint64_t> opened_masked_keys(
      config.logical_n);

  for (std::size_t index = 0;
       index < opened_masked_keys.size();
       ++index) {
    opened_masked_keys[index] =
        (local_masked_keys[index] +
         peer_masked_keys[index]) &
        comparison_ring_mask;
  }

  // CmpAgg accepts only the underlying M2 party materials. Move them out in
  // the package's validated lexicographic edge order.
  std::vector<ProtocolIUcmpPartyMaterial> edge_materials;
  edge_materials.reserve(package.edge_materials.size());

  for (auto& edge : package.edge_materials) {
    edge_materials.push_back(std::move(edge.material));
  }

  // Both the masks and edge keys are one-shot material.
  package.node_mask_shares.clear();
  package.edge_materials.clear();

  const auto logical_rank_shares =
      protocol_i_cmpagg_eval_party(
          config.party,
          config.comparison_bits,
          opened_masked_keys,
          edge_materials);

  require(
      logical_rank_shares.size() == config.logical_n,
      "Protocol III GRank CmpAgg output size");

  ProtocolIIIGrankOutput output;
  output.rank_additive_shares.resize(config.logical_n);

  // CmpAgg now evaluates only the logical comparison graph. Reducing each
  // local additive share into Z_(2^rank_bits) preserves additive-share
  // correctness without opening any rank value.
  for (std::size_t index = 0;
       index < output.rank_additive_shares.size();
       ++index) {
    output.rank_additive_shares[index] =
        logical_rank_shares[index] & rank_ring_mask;
  }

  output.metrics.sent_bytes = channel.sent_bytes();
  output.metrics.received_bytes = channel.received_bytes();

  output.metrics.comparison_edges =
      expected_edge_count(config.logical_n);

  output.metrics.ucmp_calls =
      output.metrics.comparison_edges;

  output.metrics.raw_dcf_calls =
      output.metrics.comparison_edges * 2U;

  output.metrics.online_rounds = 1;

  return output;
}

}  // namespace moe_topk
