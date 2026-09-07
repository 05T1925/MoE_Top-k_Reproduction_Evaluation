#include <moe_topk/protocol_iii_secure_combine.h>

#include <moe_topk/protocol_i_transport.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace moe_topk {
namespace {

constexpr std::uint8_t kCombinePhase = 4;
constexpr std::uint8_t kCombineFrameType = 2;

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::invalid_argument(message);
  }
}

std::size_t checked_cell_count(
    std::uint32_t logical_n,
    std::uint32_t k) {
  require(
      logical_n != 0 && k != 0,
      "Protocol III secure combine dimensions");

  require(
      static_cast<std::size_t>(logical_n) <=
          std::numeric_limits<std::size_t>::max() /
              static_cast<std::size_t>(k),
      "Protocol III secure combine dimensions overflow");

  return static_cast<std::size_t>(logical_n) *
         static_cast<std::size_t>(k);
}

std::vector<std::uint8_t> encode_words(
    const std::vector<std::uint64_t>& words) {
  std::vector<std::uint8_t> bytes;

  require(
      words.size() <=
          std::numeric_limits<std::size_t>::max() /
              sizeof(std::uint64_t),
      "Protocol III secure combine frame overflow");

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
      expected_words <=
          std::numeric_limits<std::size_t>::max() /
              sizeof(std::uint64_t),
      "Protocol III secure combine frame overflow");

  require(
      bytes.size() ==
          expected_words * sizeof(std::uint64_t),
      "Protocol III secure combine frame length");

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
    const ProtocolIIISecureCombineConfig& config) {
  require(
      config.session != 0,
      "Protocol III secure combine session");

  require(
      config.fingerprint != 0,
      "Protocol III secure combine fingerprint");

  require(
      config.party < 2U,
      "Protocol III secure combine party");

  require(
      config.logical_n >= 1U,
      "Protocol III secure combine logical_n");

  require(
      config.k >= 1U &&
          config.k <= config.logical_n,
      "Protocol III secure combine k");

  require(
      config.comparison_bits >= 34U &&
          config.comparison_bits <= 53U,
      "Protocol III secure combine comparison_bits");

  require(
      config.timeout_ms > 0,
      "Protocol III secure combine timeout");

  (void)checked_cell_count(
      config.logical_n,
      config.k);
}

void validate_material(
    const ProtocolIIISecureCombineConfig& config,
    const ProtocolIIISecureCombinePartyMaterial& material) {
  require(
      material.session == config.session,
      "Protocol III secure combine material session binding");

  require(
      material.fingerprint == config.fingerprint,
      "Protocol III secure combine material fingerprint binding");

  require(
      material.party == config.party,
      "Protocol III secure combine material party binding");

  require(
      material.logical_n == config.logical_n &&
          material.k == config.k,
      "Protocol III secure combine material parameter binding");

  require(
      material.multiplication_materials.size() ==
          checked_cell_count(config.logical_n, config.k),
      "Protocol III secure combine material count");
}

void validate_inputs(
    const ProtocolIIISecureCombineConfig& config,
    const std::vector<std::uint64_t>& indicator_shares,
    const std::vector<std::uint64_t>& unit_payload_shares,
    int combine_fd) {
  require(
      indicator_shares.size() ==
          checked_cell_count(config.logical_n, config.k),
      "Protocol III secure combine indicator count");

  require(
      unit_payload_shares.size() ==
          config.logical_n,
      "Protocol III secure combine payload count");

  require(
      combine_fd >= 0,
      "Protocol III secure combine file descriptor");
}

OnlineParty online_party(std::uint8_t party) {
  if (party == 0U) {
    return OnlineParty::kParty0;
  }

  if (party == 1U) {
    return OnlineParty::kParty1;
  }

  throw std::invalid_argument(
      "Protocol III secure combine party");
}

}  // namespace

ProtocolIIISecureCombineOutput
protocol_iii_secure_combine_party(
    const ProtocolIIISecureCombineConfig& config,
    ProtocolIIISecureCombinePartyMaterial& material,
    const std::vector<std::uint64_t>& indicator_shares,
    const std::vector<std::uint64_t>& unit_payload_shares,
    int combine_fd) {
  validate_config(config);
  validate_material(config, material);
  validate_inputs(
      config,
      indicator_shares,
      unit_payload_shares,
      combine_fd);

  const auto cells =
      checked_cell_count(config.logical_n, config.k);

  // Two opened masked values are required for every multiplication:
  // indicator+a and unit_payload+b.
  std::vector<std::uint64_t> local_open_shares(
      cells * 2U);

  for (std::size_t input = 0;
       input < config.logical_n;
       ++input) {
    for (std::uint32_t target = 0;
         target < config.k;
         ++target) {
      const auto cell =
          input * config.k + target;

      const auto open_share =
          prepare_masked_mul_open_share(
              indicator_shares[cell],
              unit_payload_shares[input],
              material.multiplication_materials[cell]);

      local_open_shares[2U * cell] =
          static_cast<std::uint64_t>(open_share.left);

      local_open_shares[2U * cell + 1U] =
          static_cast<std::uint64_t>(open_share.right);
    }
  }

  ProtocolIFrameConfig frame_config{
      config.session,
      config.fingerprint,
      config.logical_n,
      config.k,
      config.comparison_bits,
      config.party,
      static_cast<std::uint8_t>(1U - config.party),
      kCombinePhase,
      kCombineFrameType};

  ProtocolIFramedChannel channel(
      combine_fd,
      frame_config,
      config.timeout_ms);

  const auto encoded_local =
      encode_words(local_open_shares);

  std::vector<std::uint8_t> peer_payload;

  if (config.party == 0U) {
    channel.send(encoded_local);
    peer_payload = channel.receive();
  } else {
    peer_payload = channel.receive();
    channel.send(encoded_local);
  }

  const auto peer_open_shares =
      decode_words(
          peer_payload,
          local_open_shares.size());

  std::vector<std::uint64_t> arithmetic_mask_shares(
      config.logical_n,
      0);

  const auto party = online_party(config.party);

  for (std::size_t input = 0;
       input < config.logical_n;
       ++input) {
    std::uint64_t combined_share = 0;

    for (std::uint32_t target = 0;
         target < config.k;
         ++target) {
      const auto cell =
          input * config.k + target;

      const GroupElement opened_indicator =
          local_open_shares[2U * cell] +
          peer_open_shares[2U * cell];

      const GroupElement opened_payload =
          local_open_shares[2U * cell + 1U] +
          peer_open_shares[2U * cell + 1U];

      const GroupElement product_share =
          evaluate_masked_mul_share(
              party,
              opened_indicator,
              opened_payload,
              material.multiplication_materials[cell]);

      combined_share +=
          static_cast<std::uint64_t>(product_share);
    }

    arithmetic_mask_shares[input] = combined_share;
  }

  ProtocolIIISecureCombineOutput output;
  output.xor_mask_shares.resize(config.logical_n);

  // If the reconstructed arithmetic mask is a bit, the XOR of the local
  // least-significant bits equals that reconstructed bit.
  for (std::size_t input = 0;
       input < config.logical_n;
       ++input) {
    output.xor_mask_shares[input] =
        static_cast<std::uint8_t>(
            arithmetic_mask_shares[input] & 1U);
  }

  output.metrics.sent_bytes = channel.sent_bytes();
  output.metrics.received_bytes = channel.received_bytes();
  output.metrics.multiplication_calls = cells;
  output.metrics.opened_masked_values = cells * 2U;
  output.metrics.online_rounds = 1;

  // Every multiplication material is one-shot.
  material.multiplication_materials.clear();

  return output;
}

}  // namespace moe_topk
