#include <moe_topk/protocol_i_dealer_candidate_route_a.h>

#include <stdexcept>

#include <moe_topk/protocol_i_transport.h>

namespace moe_topk {
namespace {

void require(bool ok, const char* message) {
  if (!ok) throw std::invalid_argument(message);
}

std::vector<std::uint8_t> encode_words(const std::vector<std::uint64_t>& words) {
  std::vector<std::uint8_t> bytes;
  bytes.reserve(words.size() * sizeof(std::uint64_t));
  for (const auto word : words)
    for (int shift = 56; shift >= 0; shift -= 8)
      bytes.push_back(static_cast<std::uint8_t>(word >> shift));
  return bytes;
}

std::vector<std::uint64_t> decode_words(const std::vector<std::uint8_t>& bytes,
                                        std::size_t expected) {
  require(bytes.size() == expected * sizeof(std::uint64_t), "route A rank frame");
  std::vector<std::uint64_t> words(expected);
  std::size_t offset = 0;
  for (auto& word : words)
    for (int index = 0; index < 8; ++index)
      word = (word << 8U) | bytes[offset++];
  return words;
}

}  // namespace

ProtocolIDealerCandidateRouteAOutput protocol_i_dealer_candidate_route_a_party(
    const ProtocolIDealerCandidateCoreConfig& config,
    ProtocolIDealerCandidatePackage&& package,
    ProtocolIShufflePartyMaterial& shuffle_material,
    const std::vector<std::uint64_t>& priority_key_share,
    const std::array<int, 2>& forward_fds,
    int masked_open_fd,
    int rank_reveal_fd,
    const std::array<int, 2>& reverse_fds) {
  const auto core = protocol_i_dealer_candidate_core_party(
      config, std::move(package), shuffle_material, priority_key_share,
      forward_fds, masked_open_fd);
  const auto ring = (UINT64_C(1) << config.comparison_bits) - 1U;
  ProtocolIFramedChannel reveal(
      rank_reveal_fd,
      {config.session, config.fingerprint, config.padded_n, config.k,
       config.comparison_bits, config.party,
       static_cast<std::uint8_t>(1U - config.party), 2, 1},
      config.timeout_ms);
  if (config.party == 0) reveal.send(encode_words(core.shuffled_rank_share));
  const auto peer = decode_words(reveal.receive(), config.padded_n);
  if (config.party == 1) reveal.send(encode_words(core.shuffled_rank_share));

  std::vector<bool> seen(config.padded_n, false);
  std::vector<ProtocolIBlock192> carrier(config.padded_n);
  for (std::size_t index = 0; index < config.padded_n; ++index) {
    const auto rank = (core.shuffled_rank_share[index] + peer[index]) & ring;
    require(rank < config.padded_n && !seen[rank], "route A rank permutation");
    seen[rank] = true;
    carrier[index] = {config.party == 0 && rank < config.k ? 1U : 0U, 0, 0};
  }
  for (const auto value : seen) require(value, "route A rank gap");

  const auto reverse = protocol_i_shuffle_reverse_party(
      config.party, reverse_fds, carrier, shuffle_material);
  ProtocolIDealerCandidateRouteAOutput output;
  output.xor_mask_share.resize(config.logical_n);
  for (std::size_t index = 0; index < config.logical_n; ++index)
    output.xor_mask_share[index] = static_cast<std::uint8_t>(reverse.share[index].word0 & 1U);
  output.core_metrics = core.metrics;
  output.rank_reveal_sent_bytes = reveal.sent_bytes();
  output.rank_reveal_received_bytes = reveal.received_bytes();
  output.reverse_sent_bytes = reverse.counters.reverse_first.online_sent_bytes +
                              reverse.counters.reverse_second.online_sent_bytes;
  output.reverse_received_bytes = reverse.counters.reverse_first.online_received_bytes +
                                  reverse.counters.reverse_second.online_received_bytes;
  output.online_rounds = core.metrics.online_rounds + 1U + reverse.counters.reverse_online_rounds;
  require(output.online_rounds == 6, "route A online rounds");
  return output;
}

ProtocolIPriorityPipelineOutput protocol_i_dealer_candidate_route_a_priority_party(
    const ProtocolIPriorityPipelineConfig& config,
    ProtocolIPartyPackage&& package,
    ProtocolIShufflePartyMaterial& shuffle_material,
    const std::vector<std::uint64_t>& priority_key_share,
    const std::array<int, 2>& forward_fds,
    int masked_open_fd,
    int rank_reveal_fd,
    const std::array<int, 2>& reverse_fds) {
  return protocol_i_priority_pipeline_party(
      config, std::move(package), shuffle_material, priority_key_share,
      forward_fds, masked_open_fd, rank_reveal_fd, reverse_fds);
}

}  // namespace moe_topk
