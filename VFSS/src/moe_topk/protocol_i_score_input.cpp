#include <moe_topk/protocol_i_score_input.h>

#include <moe_topk/protocol_i_transport.h>

#include <limits>
#include <stdexcept>

namespace moe_topk {
namespace {
constexpr int kScoreBits = 34;
constexpr std::uint64_t kScoreMask = (UINT64_C(1) << kScoreBits) - 1U;

std::vector<std::uint8_t> encode_words(const std::vector<std::uint64_t>& words) {
  std::vector<std::uint8_t> output;
  output.reserve(words.size() * sizeof(std::uint64_t));
  for (const auto word : words) for (int shift = 56; shift >= 0; shift -= 8) output.push_back(word >> shift);
  return output;
}

std::vector<std::uint64_t> decode_words(const std::vector<std::uint8_t>& bytes,
                                        std::size_t expected_words) {
  if (bytes.size() != expected_words * sizeof(std::uint64_t)) throw std::invalid_argument("score adapter payload size");
  std::vector<std::uint64_t> words(expected_words);
  for (std::size_t i = 0; i < words.size(); ++i)
    for (int byte = 0; byte < 8; ++byte) words[i] = (words[i] << 8U) | bytes[i * 8U + byte];
  return words;
}

std::vector<std::uint64_t> exchange(const ProtocolIScoreInputConfig& config, int fd,
                                    std::uint8_t phase, const std::vector<std::uint64_t>& local,
                                    std::uint64_t* sent, std::uint64_t* received) {
  const auto peer = static_cast<std::uint8_t>(1U - config.party);
  ProtocolIFramedChannel channel(fd, {config.session, config.fingerprint, config.padded_n,
      config.k, config.comparison_bits, config.party, peer, phase, 2}, config.timeout_ms);
  std::vector<std::uint8_t> remote;
  const auto encoded = encode_words(local);
  if (config.party == 0) { channel.send(encoded); remote = channel.receive(); }
  else { remote = channel.receive(); channel.send(encoded); }
  *sent += channel.sent_bytes();
  *received += channel.received_bytes();
  return decode_words(remote, local.size());
}

void validate(const ProtocolIScoreInputConfig& config, const ProtocolIPartyPackage& package,
              const std::vector<std::uint32_t>& raw_share, const std::array<int, 2>& fds) {
  if (config.party > 1 || config.logical_n == 0 || config.logical_n > config.padded_n ||
      config.k == 0 || config.k > config.logical_n || config.index_bits == 0 ||
      config.comparison_bits != 33U + config.index_bits || config.comparison_bits > 53 ||
      raw_share.size() != config.logical_n || fds[0] < 0 || fds[1] < 0 ||
      package.party != config.party || package.session != config.session ||
      package.fingerprint != config.fingerprint || package.n != config.padded_n ||
      package.k != config.k || package.comparison_bits != config.comparison_bits ||
      package.carry_materials.size() != config.padded_n ||
      package.sign_materials.size() != config.padded_n) throw std::invalid_argument("score adapter configuration");
}
}  // namespace

std::vector<std::uint64_t> protocol_i_raw_score_input_party(
    const ProtocolIScoreInputConfig& config, ProtocolIPartyPackage& package,
    const std::vector<std::uint32_t>& raw_share, const std::array<int, 2>& stage_fds,
    ProtocolIScoreInputMetrics* metrics) {
  validate(config, package, raw_share, stage_fds);
  ProtocolIScoreInputMetrics local_metrics;
  std::vector<std::uint64_t> x(config.padded_n);
  for (std::size_t i = 0; i < raw_share.size(); ++i) x[i] = raw_share[i];
  for (std::size_t i = raw_share.size(); i < x.size(); ++i) x[i] = config.party == 0 ? UINT32_C(0x80000000) : 0U;

  std::vector<std::uint64_t> carry_open(2U * config.padded_n), carry_local(2U * config.padded_n);
  for (std::uint32_t slot = 0; slot < config.padded_n; ++slot) {
    const auto& item = package.carry_materials[slot];
    if (item.slot != slot || item.stage != 1 || item.material.party_id() != config.party ||
        item.material.comparison_bits() != kScoreBits) throw std::invalid_argument("carry material binding");
    const auto left = config.party == 1 ? UINT32_C(0xffffffff) - x[slot] : 0U;
    const auto right = config.party == 0 ? x[slot] : 0U;
    carry_local[2U * slot] = (left + item.left_mask_share) & kScoreMask;
    carry_local[2U * slot + 1U] = (right + item.right_mask_share) & kScoreMask;
  }
  std::vector<std::uint64_t> carry_peer;
  try { carry_peer = exchange(config, stage_fds[0], 4, carry_local,
                              &local_metrics.carry_sent_bytes, &local_metrics.carry_received_bytes); }
  catch (const std::exception&) { throw std::runtime_error("score carry exchange"); }
  std::vector<std::uint64_t> lift(config.padded_n), carry(config.padded_n);
  for (std::uint32_t slot = 0; slot < config.padded_n; ++slot) {
    auto& item = package.carry_materials[slot];
    carry[slot] = item.material.eval_strict_lt((carry_local[2U * slot] + carry_peer[2U * slot]) & kScoreMask,
        (carry_local[2U * slot + 1U] + carry_peer[2U * slot + 1U]) & kScoreMask);
    lift[slot] = (x[slot] - ((carry[slot] & 3U) << 32U)) & kScoreMask;
  }

  std::vector<std::uint64_t> sign_local(2U * config.padded_n);
  for (std::uint32_t slot = 0; slot < config.padded_n; ++slot) {
    const auto& item = package.sign_materials[slot];
    if (item.slot != slot || item.stage != 2 || item.material.party_id() != config.party ||
        item.material.comparison_bits() != kScoreBits) throw std::invalid_argument("sign material binding");
    const auto left = config.party == 0 ? UINT32_C(0x7fffffff) : 0U;
    sign_local[2U * slot] = (left + item.left_mask_share) & kScoreMask;
    sign_local[2U * slot + 1U] = (lift[slot] + item.right_mask_share) & kScoreMask;
  }
  std::vector<std::uint64_t> sign_peer;
  try { sign_peer = exchange(config, stage_fds[1], 5, sign_local,
                             &local_metrics.sign_sent_bytes, &local_metrics.sign_received_bytes); }
  catch (const std::exception&) { throw std::runtime_error("score sign exchange"); }
  const auto key_mask = (UINT64_C(1) << config.comparison_bits) - 1U;
  std::vector<std::uint64_t> keys(config.padded_n);
  for (std::uint32_t slot = 0; slot < config.padded_n; ++slot) {
    auto& item = package.sign_materials[slot];
    const auto sign = item.material.eval_strict_lt((sign_local[2U * slot] + sign_peer[2U * slot]) & kScoreMask,
        (sign_local[2U * slot + 1U] + sign_peer[2U * slot + 1U]) & kScoreMask);
    const auto q_share = config.party == 0
        ? (UINT64_C(0x7fffffff) - lift[slot] + ((sign & 3U) << 32U))
        : (UINT64_C(0) - lift[slot] + ((sign & 3U) << 32U));
    keys[slot] = ((q_share << config.index_bits) + (config.party == 0 ? slot : 0U)) & key_mask;
  }
  package.carry_materials.clear();
  package.sign_materials.clear();
  local_metrics.ucmp_calls = 2U * config.padded_n;
  local_metrics.raw_dcf_calls = 4U * config.padded_n;
  if (metrics) *metrics = local_metrics;
  return keys;
}
}  // namespace moe_topk
