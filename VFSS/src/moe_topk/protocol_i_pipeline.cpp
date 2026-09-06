#include <moe_topk/protocol_i_pipeline.h>
#include <moe_topk/protocol_i_transport.h>

#include <limits>
#include <stdexcept>

namespace moe_topk {
namespace {

void require(bool ok, const char* what) { if (!ok) throw std::invalid_argument(what); }

std::vector<std::uint8_t> encode_words(const std::vector<std::uint64_t>& words) {
  std::vector<std::uint8_t> bytes;
  bytes.reserve(words.size() * sizeof(std::uint64_t));
  for (const auto word : words) for (int shift = 56; shift >= 0; shift -= 8) bytes.push_back(word >> shift);
  return bytes;
}

std::vector<std::uint64_t> decode_words(const std::vector<std::uint8_t>& bytes) {
  require(bytes.size() % sizeof(std::uint64_t) == 0, "word frame length");
  std::vector<std::uint64_t> words(bytes.size() / sizeof(std::uint64_t));
  for (std::size_t index = 0; index < words.size(); ++index)
    for (int byte = 0; byte < 8; ++byte) words[index] = (words[index] << 8U) | bytes[8 * index + byte];
  return words;
}

void validate_package(const ProtocolIPartyPackage& package, const ProtocolIPriorityPipelineConfig& config) {
  require(package.party == config.party && package.n == config.padded_n && package.k == config.k &&
              package.comparison_bits == config.comparison_bits && package.session == config.session && package.fingerprint == config.fingerprint,
          "pipeline package binding");
  const auto expected = static_cast<std::uint64_t>(config.padded_n) * (config.padded_n - 1U) / 2U;
  require(package.node_mask_shares.size() == config.padded_n && package.edge_materials.size() == expected, "pipeline package shape");
  std::size_t edge = 0;
  for (std::uint32_t left = 0; left < config.padded_n; ++left) for (std::uint32_t right = left + 1; right < config.padded_n; ++right) {
    const auto& item = package.edge_materials[edge++];
    require(item.left == left && item.right == right && item.material.party_id() == config.party && item.material.comparison_bits() == config.comparison_bits,
            "pipeline package edge binding");
  }
}

void save_shuffle_metrics(ProtocolIPriorityPipelineMetrics& metrics, const ProtocolIShufflePartyCounters& counters, bool forward) {
  if (forward) {
    metrics.forward_sent_bytes = counters.forward_first.online_sent_bytes + counters.forward_second.online_sent_bytes;
    metrics.forward_received_bytes = counters.forward_first.online_received_bytes + counters.forward_second.online_received_bytes;
  } else {
    metrics.reverse_sent_bytes = counters.reverse_first.online_sent_bytes + counters.reverse_second.online_sent_bytes;
    metrics.reverse_received_bytes = counters.reverse_first.online_received_bytes + counters.reverse_second.online_received_bytes;
  }
}
}  // namespace

ProtocolIInputLayout protocol_i_make_input_layout(std::uint32_t logical_n, std::uint32_t k) {
  require(logical_n >= 1 && logical_n <= 1'000'000 && k >= 1 && k <= logical_n, "input layout");
  std::uint32_t padded = 2;
  while (padded < logical_n) { require(padded <= 1'048'576 / 2, "padded input too large"); padded <<= 1U; }
  std::uint8_t index_bits = 0;
  for (std::uint32_t value = padded - 1; value != 0; value >>= 1U) ++index_bits;
  const auto minimum = static_cast<std::uint8_t>(32U + index_bits + 1U);
  require(minimum <= 53, "priority-key comparison width");
  return {logical_n, padded, k, index_bits, minimum};
}

ProtocolIPriorityPipelineOutput protocol_i_priority_pipeline_party(
    const ProtocolIPriorityPipelineConfig& config, ProtocolIPartyPackage&& package,
    ProtocolIShufflePartyMaterial& material, const std::vector<std::uint64_t>& key_shares,
    const std::array<int, 2>& forward_fds, int cmpagg_fd, int rank_reveal_fd,
    const std::array<int, 2>& reverse_fds) {
  const auto layout = protocol_i_make_input_layout(config.logical_n, config.k);
  require(config.party < 2 && config.padded_n == layout.padded_n && config.comparison_bits >= layout.minimum_comparison_bits &&
              config.comparison_bits <= 53 && config.timeout_ms > 0 && key_shares.size() == config.padded_n, "pipeline config");
  validate_package(package, config);
  const auto ring = (UINT64_C(1) << config.comparison_bits) - 1U;
  for (const auto share : key_shares) require((share & ~ring) == 0, "priority key share outside ring");

  std::vector<ProtocolIBlock192> records(config.padded_n);
  for (std::size_t index = 0; index < records.size(); ++index) records[index] = {key_shares[index], 0, 0};
  const auto forward = protocol_i_shuffle_forward_party(config.party, forward_fds, records, material);
  std::vector<std::uint64_t> local(config.padded_n);
  for (std::size_t index = 0; index < local.size(); ++index)
    local[index] = protocol_i_mask_priority_key_share(config.comparison_bits, forward.share[index].word0, package.node_mask_shares[index]);

  ProtocolIFrameConfig cmp_config{config.session, config.fingerprint, config.padded_n, config.k, config.comparison_bits,
                                  config.party, static_cast<std::uint8_t>(1U - config.party), 2, 1};
  ProtocolIFramedChannel cmp_channel(cmpagg_fd, cmp_config, config.timeout_ms);
  if (config.party == 0) cmp_channel.send(encode_words(local));
  const auto peer = decode_words(cmp_channel.receive());
  if (config.party == 1) cmp_channel.send(encode_words(local));
  require(peer.size() == config.padded_n, "cmp frame");
  std::vector<std::uint64_t> opened(config.padded_n);
  for (std::size_t index = 0; index < opened.size(); ++index) opened[index] = (local[index] + peer[index]) & ring;

  std::vector<ProtocolIUcmpPartyMaterial> edges;
  edges.reserve(package.edge_materials.size());
  for (auto& edge : package.edge_materials) edges.push_back(std::move(edge.material));
  package.edge_materials.clear();
  const auto ranks = protocol_i_cmpagg_eval_party(config.party, config.comparison_bits, opened, edges);

  ProtocolIFrameConfig reveal_config{config.session, config.fingerprint, config.padded_n, config.k, config.comparison_bits,
                                     config.party, static_cast<std::uint8_t>(1U - config.party), 3, 1};
  ProtocolIFramedChannel reveal_channel(rank_reveal_fd, reveal_config, config.timeout_ms);
  if (config.party == 0) reveal_channel.send(encode_words(ranks));
  const auto peer_ranks = decode_words(reveal_channel.receive());
  if (config.party == 1) reveal_channel.send(encode_words(ranks));
  require(peer_ranks.size() == config.padded_n, "rank reveal frame");
  std::vector<bool> seen(config.padded_n, false);
  std::vector<ProtocolIBlock192> carrier(config.padded_n);
  for (std::size_t index = 0; index < carrier.size(); ++index) {
    // Rank shares are additive comparison-ring elements; only their modular
    // reconstruction is a public shuffled-domain rank.
    const auto rank = (ranks[index] + peer_ranks[index]) & ring;
    require(rank < config.padded_n && !seen[rank], "rank is not a full permutation");
    seen[rank] = true;
    carrier[index] = {config.party == 0 && rank < config.k ? 1U : 0U, 0, 0};
  }
  for (const auto value : seen) require(value, "rank permutation gap");
  const auto reverse = protocol_i_shuffle_reverse_party(config.party, reverse_fds, carrier, material);

  ProtocolIPriorityPipelineOutput output;
  output.xor_mask_share.resize(config.logical_n);
  for (std::size_t index = 0; index < output.xor_mask_share.size(); ++index)
    output.xor_mask_share[index] = static_cast<std::uint8_t>(reverse.share[index].word0 & 1U);
  save_shuffle_metrics(output.metrics, forward.counters, true);
  save_shuffle_metrics(output.metrics, reverse.counters, false);
  output.metrics.cmpagg_sent_bytes = cmp_channel.sent_bytes(); output.metrics.cmpagg_received_bytes = cmp_channel.received_bytes();
  output.metrics.rank_reveal_sent_bytes = reveal_channel.sent_bytes(); output.metrics.rank_reveal_received_bytes = reveal_channel.received_bytes();
  output.metrics.comparison_edges = static_cast<std::uint64_t>(config.padded_n) * (config.padded_n - 1U) / 2U;
  output.metrics.raw_dcf_calls = output.metrics.comparison_edges * 2U;
  return output;
}
}  // namespace moe_topk
