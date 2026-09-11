#include <moe_topk/protocol_i_dealer_candidate_core.h>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>

#include <moe_topk/protocol_i_cmpagg.h>
#include <moe_topk/protocol_i_pipeline.h>
#include <moe_topk/protocol_i_transport.h>

namespace moe_topk {
namespace {

constexpr std::size_t kMaxResultBytes = 64U * 1024U * 1024U;
constexpr std::size_t kMaxEdges = 1'000'000U;
constexpr std::size_t kTraceCount = 3;

[[noreturn]] void fail(const char* message) { throw std::invalid_argument(message); }

std::size_t edge_count(std::uint32_t n) {
  if (n < 2) fail("candidate result edge count");
  const auto nodes = static_cast<std::size_t>(n);
  if (nodes > std::numeric_limits<std::size_t>::max() / (nodes - 1U)) {
    fail("candidate result edge count overflow");
  }
  const auto count = nodes * (nodes - 1U) / 2U;
  if (count > kMaxEdges) fail("candidate result edge count limit");
  return count;
}

std::uint64_t ring_mask(std::uint8_t bits) {
  if (bits < 34 || bits > 53) fail("candidate core comparison bits");
  return (UINT64_C(1) << bits) - 1U;
}

bool same_public_config(const ProtocolIDealerCandidatePublicConfig& left,
                        const ProtocolIDealerCandidatePublicConfig& right) {
  return left.session == right.session && left.fingerprint == right.fingerprint &&
         left.material_id == right.material_id && left.logical_n == right.logical_n &&
         left.padded_n == right.padded_n && left.k == right.k &&
         left.comparison_bits == right.comparison_bits && left.rank_bits == right.rank_bits;
}

void validate_core_config(const ProtocolIDealerCandidateCoreConfig& config) {
  if (config.party > 1 || config.timeout_ms <= 0) fail("candidate core configuration");
  const auto layout = protocol_i_make_input_layout(config.logical_n, config.k);
  if (config.padded_n != layout.padded_n || config.rank_bits != layout.index_bits ||
      config.comparison_bits < layout.minimum_comparison_bits || config.comparison_bits > 53 ||
      config.session == 0 || config.fingerprint == 0 || config.material_id == 0) {
    fail("candidate core layout");
  }
}

void validate_package_config(const ProtocolIDealerCandidateCoreConfig& config,
                             const ProtocolIDealerCandidatePackage& package) {
  const auto& package_config = package.config;
  if (!same_public_config(config, package_config) || config.party != package_config.party ||
      package.implementation_label != kProtocolIDealerCandidateLabel || package.consumed ||
      package.r_share.size() != config.padded_n || package.edge_materials.size() != edge_count(config.padded_n)) {
    fail("candidate package/core mismatch");
  }
  const auto mask = ring_mask(config.comparison_bits);
  for (const auto share : package.r_share) {
    if ((share & ~mask) != 0) fail("candidate package r share outside ring");
  }
  std::size_t slot = 0;
  for (std::uint32_t left = 0; left < config.padded_n; ++left) {
    for (std::uint32_t right = left + 1; right < config.padded_n; ++right) {
      const auto& edge = package.edge_materials[slot];
      if (edge.left != left || edge.right != right || edge.slot != slot || edge.stage != 1 ||
          edge.material.party_id() != config.party ||
          edge.material.comparison_bits() != config.comparison_bits) {
        fail("candidate package edge mismatch");
      }
      ++slot;
    }
  }
}

void validate_shuffle_material(const ProtocolIDealerCandidateCoreConfig& config,
                               const ProtocolIShufflePartyMaterial& material) {
  if (material.config.session != config.session || material.config.fingerprint != config.fingerprint ||
      material.config.party != config.party || material.config.n != config.padded_n ||
      material.config.subpermutation_size != 2 || material.forward_consumed) {
    fail("candidate shuffle material");
  }
}

struct ByteCounts {
  std::uint64_t sent = 0, received = 0;
};

ByteCounts offline_counter_bytes(const ProtocolIPermuteShareCounters& counters) {
  return {counters.offline_ot.sent_bytes, counters.offline_ot.received_bytes};
}

ByteCounts shuffle_offline_bytes(const ProtocolIShufflePartyMaterial& material) {
  const std::array<const ProtocolIPermuteShareCounters*, 8> counters{
      &material.forward_po_first.counters, &material.forward_do_first.counters,
      &material.forward_po_second.counters, &material.forward_do_second.counters,
      &material.reverse_po_first.counters, &material.reverse_do_first.counters,
      &material.reverse_po_second.counters, &material.reverse_do_second.counters};
  ByteCounts result;
  for (const auto* counter : counters) {
    const auto current = offline_counter_bytes(*counter);
    result.sent += current.sent;
    result.received += current.received;
  }
  return result;
}

std::vector<std::uint8_t> encode_words(const std::vector<std::uint64_t>& words) {
  if (words.size() > std::numeric_limits<std::size_t>::max() / sizeof(std::uint64_t)) {
    fail("candidate word vector");
  }
  std::vector<std::uint8_t> bytes;
  bytes.reserve(words.size() * sizeof(std::uint64_t));
  for (const auto word : words) {
    for (int shift = 56; shift >= 0; shift -= 8) {
      bytes.push_back(static_cast<std::uint8_t>(word >> shift));
    }
  }
  return bytes;
}

std::vector<std::uint64_t> decode_words(const std::vector<std::uint8_t>& bytes,
                                        std::size_t expected_count) {
  if (bytes.size() != expected_count * sizeof(std::uint64_t)) fail("candidate word frame");
  std::vector<std::uint64_t> words(expected_count);
  std::size_t offset = 0;
  for (auto& word : words) {
    for (int index = 0; index < 8; ++index) word = (word << 8U) | bytes[offset++];
  }
  return words;
}

void put_u64(std::vector<std::uint8_t>& output, std::uint64_t value) {
  if (output.size() > kMaxResultBytes - sizeof(value)) fail("candidate result size");
  for (int shift = 56; shift >= 0; shift -= 8) {
    output.push_back(static_cast<std::uint8_t>(value >> shift));
  }
}

std::uint64_t get_u64(const std::vector<std::uint8_t>& bytes, std::size_t& offset) {
  if (offset > bytes.size() || bytes.size() - offset < sizeof(std::uint64_t)) {
    fail("candidate result truncated");
  }
  std::uint64_t value = 0;
  for (int index = 0; index < 8; ++index) value = (value << 8U) | bytes[offset++];
  return value;
}

void put_metrics(std::vector<std::uint8_t>& output,
                 const ProtocolIDealerCandidateMetrics& metrics) {
  put_u64(output, metrics.offline_material_bytes);
  put_u64(output, metrics.offline_shuffle_sent_bytes);
  put_u64(output, metrics.offline_shuffle_received_bytes);
  put_u64(output, metrics.r1_sent_bytes);
  put_u64(output, metrics.r1_received_bytes);
  put_u64(output, metrics.r2_sent_bytes);
  put_u64(output, metrics.r2_received_bytes);
  put_u64(output, metrics.r3_sent_bytes);
  put_u64(output, metrics.r3_received_bytes);
  put_u64(output, metrics.party_to_party_bytes);
  put_u64(output, metrics.comparison_edges);
  put_u64(output, metrics.raw_dcf_calls);
  put_u64(output, metrics.forward_online_rounds);
  put_u64(output, metrics.masked_open_rounds);
  put_u64(output, metrics.online_rounds);
  put_u64(output, metrics.rank_bits);
}

ProtocolIDealerCandidateMetrics get_metrics(const std::vector<std::uint8_t>& bytes,
                                            std::size_t& offset) {
  ProtocolIDealerCandidateMetrics metrics;
  metrics.offline_material_bytes = get_u64(bytes, offset);
  metrics.offline_shuffle_sent_bytes = get_u64(bytes, offset);
  metrics.offline_shuffle_received_bytes = get_u64(bytes, offset);
  metrics.r1_sent_bytes = get_u64(bytes, offset);
  metrics.r1_received_bytes = get_u64(bytes, offset);
  metrics.r2_sent_bytes = get_u64(bytes, offset);
  metrics.r2_received_bytes = get_u64(bytes, offset);
  metrics.r3_sent_bytes = get_u64(bytes, offset);
  metrics.r3_received_bytes = get_u64(bytes, offset);
  metrics.party_to_party_bytes = get_u64(bytes, offset);
  metrics.comparison_edges = get_u64(bytes, offset);
  metrics.raw_dcf_calls = get_u64(bytes, offset);
  metrics.forward_online_rounds = get_u64(bytes, offset);
  metrics.masked_open_rounds = get_u64(bytes, offset);
  metrics.online_rounds = get_u64(bytes, offset);
  metrics.rank_bits = get_u64(bytes, offset);
  return metrics;
}

void put_trace(std::vector<std::uint8_t>& output,
               const ProtocolIDealerCandidateTraceEvent& event) {
  put_u64(output, event.barrier);
  put_u64(output, event.phase);
  put_u64(output, event.type);
  put_u64(output, event.sender);
  put_u64(output, event.receiver);
  put_u64(output, event.sequence);
  put_u64(output, event.sent_bytes);
  put_u64(output, event.received_bytes);
  put_u64(output, event.completed);
}

ProtocolIDealerCandidateTraceEvent get_trace(const std::vector<std::uint8_t>& bytes,
                                             std::size_t& offset) {
  ProtocolIDealerCandidateTraceEvent event;
  event.barrier = static_cast<std::uint32_t>(get_u64(bytes, offset));
  event.phase = static_cast<std::uint32_t>(get_u64(bytes, offset));
  event.type = static_cast<std::uint8_t>(get_u64(bytes, offset));
  event.sender = static_cast<std::uint8_t>(get_u64(bytes, offset));
  event.receiver = static_cast<std::uint8_t>(get_u64(bytes, offset));
  event.sequence = get_u64(bytes, offset);
  event.sent_bytes = get_u64(bytes, offset);
  event.received_bytes = get_u64(bytes, offset);
  event.completed = static_cast<std::uint8_t>(get_u64(bytes, offset));
  return event;
}

void validate_output(const ProtocolIDealerCandidateCoreConfig& config,
                     const ProtocolIDealerCandidateOutput& output) {
  if (output.public_masked_list.size() != config.padded_n ||
      output.shuffled_rank_share.size() != config.padded_n || output.trace.size() != kTraceCount ||
      output.metrics.comparison_edges != edge_count(config.padded_n) ||
      output.metrics.raw_dcf_calls != output.metrics.comparison_edges * 2U ||
      output.metrics.rank_bits != config.rank_bits || output.metrics.forward_online_rounds == 0 ||
      output.metrics.masked_open_rounds != 1 ||
      output.metrics.online_rounds != output.metrics.forward_online_rounds + output.metrics.masked_open_rounds) {
    fail("candidate result shape");
  }
  const auto party_bytes = output.metrics.r1_sent_bytes + output.metrics.r1_received_bytes +
                           output.metrics.r2_sent_bytes + output.metrics.r2_received_bytes +
                           output.metrics.r3_sent_bytes + output.metrics.r3_received_bytes;
  if (output.metrics.party_to_party_bytes != party_bytes) fail("candidate result byte accounting");
  const auto mask = ring_mask(config.comparison_bits);
  for (const auto value : output.public_masked_list) {
    if ((value & ~mask) != 0) fail("candidate result y outside ring");
  }
  for (const auto value : output.shuffled_rank_share) {
    if ((value & ~mask) != 0) fail("candidate rank share outside ring");
  }
  for (std::size_t index = 0; index < output.trace.size(); ++index) {
    const auto event = output.trace[index];
    if (event.barrier != index + 1U || event.phase != index + 1U || event.type != 1 ||
        event.sender > 2 || event.receiver > 2 || event.sender == event.receiver ||
        event.completed != 1) {
      fail("candidate trace event");
    }
  }
}

}  // namespace

ProtocolIDealerCandidateOutput protocol_i_dealer_candidate_core_party(
    const ProtocolIDealerCandidateCoreConfig& config,
    ProtocolIDealerCandidatePackage&& package,
    ProtocolIShufflePartyMaterial& shuffle_material,
    const std::vector<std::uint64_t>& priority_key_share,
    const std::array<int, 2>& forward_fds,
    int masked_open_fd) {
  validate_core_config(config);
  validate_package_config(config, package);
  validate_shuffle_material(config, shuffle_material);
  if (priority_key_share.size() != config.padded_n) fail("candidate priority-key shape");
  const auto mask = ring_mask(config.comparison_bits);
  for (const auto share : priority_key_share) {
    if ((share & ~mask) != 0) fail("candidate priority-key share outside ring");
  }

  package.consumed = true;
  const auto package_bytes = package.serialized_bytes;
  ProtocolIDealerCandidateOutput output;
  const auto offline_bytes = shuffle_offline_bytes(shuffle_material);
  const auto forward = protocol_i_shuffle_forward_party(
      config.party, forward_fds,
      [&]() {
        std::vector<ProtocolIBlock192> records(config.padded_n);
        for (std::size_t index = 0; index < records.size(); ++index) {
          records[index] = {priority_key_share[index], 0, 0};
        }
        return records;
      }(),
      shuffle_material);

  std::vector<std::uint64_t> local_masked(config.padded_n);
  for (std::size_t index = 0; index < config.padded_n; ++index) {
    local_masked[index] = (forward.share[index].word0 + package.r_share[index]) & mask;
  }

  std::vector<std::uint64_t> public_masked;
  std::uint64_t r3_sent = 0, r3_received = 0;
  {
    ProtocolIFramedChannel channel(
        masked_open_fd,
        {config.session, config.fingerprint, config.padded_n, config.k,
         config.comparison_bits, config.party, static_cast<std::uint8_t>(1U - config.party), 3, 1},
        config.timeout_ms);
    if (config.party == 0) channel.send(encode_words(local_masked));
    const auto peer_masked = decode_words(channel.receive(), config.padded_n);
    if (config.party == 1) channel.send(encode_words(local_masked));
    public_masked.resize(config.padded_n);
    for (std::size_t index = 0; index < config.padded_n; ++index) {
      public_masked[index] = (local_masked[index] + peer_masked[index]) & mask;
    }
    r3_sent = channel.sent_bytes();
    r3_received = channel.received_bytes();
  }

  std::vector<ProtocolIUcmpPartyMaterial> edge_materials;
  edge_materials.reserve(package.edge_materials.size());
  for (auto& edge : package.edge_materials) edge_materials.push_back(std::move(edge.material));
  auto rank_share = protocol_i_cmpagg_eval_party(
      config.party, config.comparison_bits, public_masked, edge_materials);
  for (auto& rank : rank_share) rank &= mask;

  const auto& first = forward.counters.forward_first;
  const auto& second = forward.counters.forward_second;
  output.public_masked_list = std::move(public_masked);
  output.shuffled_rank_share = rank_share;
  output.metrics.offline_material_bytes = package_bytes + offline_bytes.sent + offline_bytes.received;
  output.metrics.offline_shuffle_sent_bytes = offline_bytes.sent;
  output.metrics.offline_shuffle_received_bytes = offline_bytes.received;
  output.metrics.r1_sent_bytes = first.online_sent_bytes;
  output.metrics.r1_received_bytes = first.online_received_bytes;
  output.metrics.r2_sent_bytes = second.online_sent_bytes;
  output.metrics.r2_received_bytes = second.online_received_bytes;
  output.metrics.r3_sent_bytes = r3_sent;
  output.metrics.r3_received_bytes = r3_received;
  output.metrics.party_to_party_bytes = output.metrics.r1_sent_bytes + output.metrics.r1_received_bytes +
                                        output.metrics.r2_sent_bytes + output.metrics.r2_received_bytes +
                                        output.metrics.r3_sent_bytes + output.metrics.r3_received_bytes;
  output.metrics.comparison_edges = edge_count(config.padded_n);
  output.metrics.raw_dcf_calls = output.metrics.comparison_edges * 2U;
  output.metrics.forward_online_rounds = first.ps_online_rounds + second.ps_online_rounds;
  output.metrics.masked_open_rounds = 1;
  output.metrics.online_rounds = output.metrics.forward_online_rounds + output.metrics.masked_open_rounds;
  output.metrics.rank_bits = config.rank_bits;
  output.trace = {
      {1, 1, 1, 1, 0, 1, 0, first.online_sent_bytes, first.online_received_bytes},
      {2, 2, 1, 0, 1, 1, 0, second.online_sent_bytes, second.online_received_bytes},
      {3, 3, 1, config.party, static_cast<std::uint8_t>(1U - config.party), 0, 1, r3_sent, r3_received}};
  validate_output(config, output);
  return output;
}

std::vector<std::uint8_t> serialize_dealer_candidate_result(
    const ProtocolIDealerCandidateCoreConfig& config,
    const ProtocolIDealerCandidateOutput& output) {
  validate_core_config(config);
  validate_output(config, output);
  std::vector<std::uint8_t> bytes;
  bytes.reserve(512 + output.public_masked_list.size() * 16U);
  bytes.insert(bytes.end(), {'M', '2', 'D', 'R', 1, config.party, config.comparison_bits, config.rank_bits});
  put_u64(bytes, config.session);
  put_u64(bytes, config.fingerprint);
  put_u64(bytes, config.material_id);
  put_u64(bytes, config.logical_n);
  put_u64(bytes, config.padded_n);
  put_u64(bytes, config.k);
  put_u64(bytes, sizeof(kProtocolIDealerCandidateLabel) - 1U);
  bytes.insert(bytes.end(), kProtocolIDealerCandidateLabel,
               kProtocolIDealerCandidateLabel + sizeof(kProtocolIDealerCandidateLabel) - 1U);
  put_u64(bytes, output.public_masked_list.size());
  put_u64(bytes, output.shuffled_rank_share.size());
  put_u64(bytes, output.trace.size());
  put_metrics(bytes, output.metrics);
  for (const auto value : output.public_masked_list) put_u64(bytes, value);
  for (const auto value : output.shuffled_rank_share) put_u64(bytes, value);
  for (const auto event : output.trace) put_trace(bytes, event);
  return bytes;
}

ProtocolIDealerCandidateOutput deserialize_dealer_candidate_result(
    const std::vector<std::uint8_t>& bytes,
    const ProtocolIDealerCandidateCoreConfig& expected_config,
    int expected_party) {
  validate_core_config(expected_config);
  if (expected_party < 0 || expected_party > 1 || bytes.size() > kMaxResultBytes || bytes.size() < 8 ||
      bytes[0] != 'M' || bytes[1] != '2' || bytes[2] != 'D' || bytes[3] != 'R' || bytes[4] != 1 ||
      bytes[5] != expected_party) {
    fail("candidate result header");
  }
  ProtocolIDealerCandidateCoreConfig config = expected_config;
  config.party = static_cast<std::uint8_t>(bytes[5]);
  if (config.comparison_bits != bytes[6] || config.rank_bits != bytes[7]) fail("candidate result width");
  std::size_t offset = 8;
  const auto session = get_u64(bytes, offset);
  const auto fingerprint = get_u64(bytes, offset);
  const auto material_id = get_u64(bytes, offset);
  const auto logical = get_u64(bytes, offset);
  const auto padded = get_u64(bytes, offset);
  const auto k = get_u64(bytes, offset);
  const auto label_size = get_u64(bytes, offset);
  if (label_size != sizeof(kProtocolIDealerCandidateLabel) - 1U ||
      label_size > bytes.size() - offset ||
      !std::equal(kProtocolIDealerCandidateLabel,
                  kProtocolIDealerCandidateLabel + label_size,
                  bytes.begin() + offset)) {
    fail("candidate result label");
  }
  offset += static_cast<std::size_t>(label_size);
  if (session != config.session || fingerprint != config.fingerprint || material_id != config.material_id ||
      logical != config.logical_n || padded != config.padded_n || k != config.k) {
    fail("candidate result identity");
  }
  const auto y_count = get_u64(bytes, offset);
  const auto rank_count = get_u64(bytes, offset);
  const auto trace_count = get_u64(bytes, offset);
  if (y_count != config.padded_n || rank_count != config.padded_n || trace_count != kTraceCount) {
    fail("candidate result count");
  }
  ProtocolIDealerCandidateOutput output;
  output.metrics = get_metrics(bytes, offset);
  output.public_masked_list.resize(config.padded_n);
  output.shuffled_rank_share.resize(config.padded_n);
  const auto mask = ring_mask(config.comparison_bits);
  for (auto& value : output.public_masked_list) {
    value = get_u64(bytes, offset);
    if ((value & ~mask) != 0) fail("candidate result y outside ring");
  }
  for (auto& value : output.shuffled_rank_share) {
    value = get_u64(bytes, offset);
    if ((value & ~mask) != 0) fail("candidate rank share outside ring");
  }
  output.trace.reserve(kTraceCount);
  for (std::size_t index = 0; index < kTraceCount; ++index) {
    output.trace.push_back(get_trace(bytes, offset));
  }
  if (offset != bytes.size()) fail("candidate result trailing bytes");
  validate_output(config, output);
  return output;
}

}  // namespace moe_topk
