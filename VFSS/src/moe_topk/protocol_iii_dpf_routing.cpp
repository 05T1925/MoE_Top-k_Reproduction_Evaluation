#include <moe_topk/protocol_iii_dpf_routing.h>

#include <moe_topk/protocol_i_transport.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace moe_topk {
namespace {

constexpr int kDpfPayloadBits = 64;
constexpr std::uint8_t kRoutingPhase = 3;
constexpr std::uint8_t kRoutingFrameType = 2;

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::invalid_argument(message);
  }
}

std::uint8_t required_rank_bits(std::uint32_t logical_n) {
  if (logical_n <= 1U) {
    return 1U;
  }

  std::uint8_t bits = 0;
  std::uint32_t value = logical_n - 1U;

  while (value != 0U) {
    ++bits;
    value >>= 1U;
  }

  return bits;
}

std::uint64_t rank_ring_mask(std::uint8_t bits) {
  require(
      bits >= 1U && bits < 64U,
      "Protocol III DPF routing rank width");

  return (UINT64_C(1) << bits) - 1U;
}

std::vector<std::uint8_t> encode_words(
    const std::vector<std::uint64_t>& words) {
  std::vector<std::uint8_t> bytes;

  bytes.reserve(
      words.size() * sizeof(std::uint64_t));

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
      bytes.size() ==
          expected_words * sizeof(std::uint64_t),
      "Protocol III DPF routing frame length");

  std::vector<std::uint64_t> words(expected_words);

  for (std::size_t index = 0;
       index < expected_words;
       ++index) {
    for (std::size_t byte = 0;
         byte < sizeof(std::uint64_t);
         ++byte) {
      words[index] =
          (words[index] << 8U) |
          bytes[
              index * sizeof(std::uint64_t) + byte];
    }
  }

  return words;
}

void validate_config(
    const ProtocolIIIDpfRoutingConfig& config) {
  require(
      config.session != 0,
      "Protocol III DPF routing session");

  require(
      config.fingerprint != 0,
      "Protocol III DPF routing fingerprint");

  require(
      config.party < 2U,
      "Protocol III DPF routing party");

  require(
      config.logical_n >= 1U,
      "Protocol III DPF routing logical_n");

  require(
      config.k >= 1U &&
          config.k <= config.logical_n,
      "Protocol III DPF routing k");

  require(
      config.rank_bits ==
          required_rank_bits(config.logical_n),
      "Protocol III DPF routing rank_bits");

  // ProtocolIFramedChannel currently accepts widths in [34, 53].
  // This field binds the routing frame to the preceding GRank request;
  // it is not the DPF input-domain width.
  require(
      config.comparison_bits >= 34U &&
          config.comparison_bits <= 53U,
      "Protocol III DPF routing comparison_bits");

  require(
      config.timeout_ms > 0,
      "Protocol III DPF routing timeout");
}

void validate_material(
    const ProtocolIIIDpfRoutingConfig& config,
    const ProtocolIIIDpfRoutingPartyMaterial& material) {
  require(
      material.session == config.session,
      "Protocol III DPF routing material session binding");

  require(
      material.fingerprint == config.fingerprint,
      "Protocol III DPF routing material fingerprint binding");

  require(
      material.party == config.party,
      "Protocol III DPF routing material party binding");

  require(
      material.logical_n == config.logical_n &&
          material.k == config.k &&
          material.rank_bits == config.rank_bits,
      "Protocol III DPF routing material parameter binding");

  require(
      material.rank_mask_shares.size() ==
          config.logical_n,
      "Protocol III DPF routing mask-share count");

  require(
      material.dpf_keys.size() ==
          config.logical_n,
      "Protocol III DPF routing DPF-key count");

  const auto mask =
      rank_ring_mask(config.rank_bits);

  for (const auto share :
       material.rank_mask_shares) {
    require(
        (share & ~mask) == 0,
        "Protocol III DPF routing mask share outside ring");
  }

  for (const auto& owned_key :
       material.dpf_keys) {
    const auto& key = owned_key.native_key();

    require(
        key.s != nullptr,
        "Protocol III DPF routing empty DPF key");

    require(
        key.bin ==
            static_cast<int>(config.rank_bits),
        "Protocol III DPF routing DPF input width");

    require(
        key.bout == kDpfPayloadBits,
        "Protocol III DPF routing DPF payload width");
  }
}

void validate_inputs(
    const ProtocolIIIDpfRoutingConfig& config,
    const std::vector<std::uint64_t>& rank_shares,
    int routing_fd) {
  require(
      rank_shares.size() == config.logical_n,
      "Protocol III DPF routing rank-share count");

  require(
      routing_fd >= 0,
      "Protocol III DPF routing file descriptor");

  const auto mask =
      rank_ring_mask(config.rank_bits);

  for (const auto share : rank_shares) {
    require(
        (share & ~mask) == 0,
        "Protocol III DPF routing rank share outside ring");
  }
}

}  // namespace

ProtocolIIIDpfRoutingKey::ProtocolIIIDpfRoutingKey(
    DPFKeyPack&& key)
    : key_() {
  take(key);
}

ProtocolIIIDpfRoutingKey::~ProtocolIIIDpfRoutingKey() {
  release();
}

ProtocolIIIDpfRoutingKey::ProtocolIIIDpfRoutingKey(
    ProtocolIIIDpfRoutingKey&& other) noexcept
    : key_() {
  take(other.key_);
}

ProtocolIIIDpfRoutingKey&
ProtocolIIIDpfRoutingKey::operator=(
    ProtocolIIIDpfRoutingKey&& other) noexcept {
  if (this != &other) {
    release();
    take(other.key_);
  }

  return *this;
}

void ProtocolIIIDpfRoutingKey::release() noexcept {
  delete[] key_.s;

  key_.s = nullptr;
  key_.bin = 0;
  key_.bout = 0;
  key_.tLcw = 0;
  key_.tRcw = 0;
  key_.payload = 0;
}

void ProtocolIIIDpfRoutingKey::take(
    DPFKeyPack& source) noexcept {
  key_.bin = source.bin;
  key_.bout = source.bout;
  key_.s = source.s;
  key_.tLcw = source.tLcw;
  key_.tRcw = source.tRcw;
  key_.payload = source.payload;

  source.s = nullptr;
  source.bin = 0;
  source.bout = 0;
  source.tLcw = 0;
  source.tRcw = 0;
  source.payload = 0;
}

ProtocolIIIDpfRoutingOutput
protocol_iii_dpf_routing_party(
    const ProtocolIIIDpfRoutingConfig& config,
    ProtocolIIIDpfRoutingPartyMaterial& material,
    const std::vector<std::uint64_t>& rank_shares,
    int routing_fd) {
  validate_config(config);
  validate_material(config, material);
  validate_inputs(config, rank_shares, routing_fd);

  const auto mask =
      rank_ring_mask(config.rank_bits);

  std::vector<std::uint64_t> local_masked_ranks(
      config.logical_n);

  for (std::size_t index = 0;
       index < config.logical_n;
       ++index) {
    local_masked_ranks[index] =
        (rank_shares[index] +
         material.rank_mask_shares[index]) &
        mask;
  }

  ProtocolIFrameConfig frame_config{
      config.session,
      config.fingerprint,
      config.logical_n,
      config.k,
      config.comparison_bits,
      config.party,
      static_cast<std::uint8_t>(
          1U - config.party),
      kRoutingPhase,
      kRoutingFrameType};

  ProtocolIFramedChannel channel(
      routing_fd,
      frame_config,
      config.timeout_ms);

  const auto encoded_local =
      encode_words(local_masked_ranks);

  std::vector<std::uint8_t> peer_payload;

  if (config.party == 0U) {
    channel.send(encoded_local);
    peer_payload = channel.receive();
  } else {
    peer_payload = channel.receive();
    channel.send(encoded_local);
  }

  const auto peer_masked_ranks =
      decode_words(
          peer_payload,
          config.logical_n);

  std::vector<std::uint64_t> opened_masked_ranks(
      config.logical_n);

  for (std::size_t index = 0;
       index < config.logical_n;
       ++index) {
    opened_masked_ranks[index] =
        (local_masked_ranks[index] +
         peer_masked_ranks[index]) &
        mask;
  }

  ProtocolIIIDpfRoutingOutput output;

  output.indicator_shares.resize(
      static_cast<std::size_t>(config.logical_n) *
      config.k);

  for (std::size_t index = 0;
       index < config.logical_n;
       ++index) {
    auto& key =
        material.dpf_keys[index].native_key();

    for (std::uint32_t target_rank = 0;
         target_rank < config.k;
         ++target_rank) {
      const auto dpf_input =
          (opened_masked_ranks[index] -
           target_rank) &
          mask;

      // GroupElement is an alias for uint64_t in this VFSS version.
      const GroupElement payload =
          evalDPF_Payload(
              config.party,
              key,
              dpf_input);

      output.indicator_shares[
          index * config.k + target_rank] =
          static_cast<std::uint64_t>(payload);
    }
  }

  output.metrics.sent_bytes =
      channel.sent_bytes();

  output.metrics.received_bytes =
      channel.received_bytes();

  output.metrics.dpf_keys =
      config.logical_n;

  output.metrics.eval_calls =
      static_cast<std::uint64_t>(config.logical_n) *
      config.k;

  output.metrics.online_rounds = 1;

  // One DPF key may be evaluated at every public target rank during this
  // invocation, but it and its corresponding mask are not reusable by a
  // later routing invocation.
  material.rank_mask_shares.clear();
  material.dpf_keys.clear();

  return output;
}

}  // namespace moe_topk
