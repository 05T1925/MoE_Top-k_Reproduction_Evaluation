#include <moe_topk/protocol_iii_two_round.h>

#include <moe_topk/protocol_i_cmpagg.h>
#include <moe_topk/protocol_i_transport.h>
#include <moe_topk/protocol_iii_field_payload.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace moe_topk {
namespace {
constexpr std::size_t kHeaderBytes = 44U;

void require(bool condition, const char* message) {
  if (!condition) throw std::invalid_argument(message);
}

std::uint8_t width(std::uint32_t value) {
  std::uint8_t result = 0;
  while (value != 0U) { ++result; value >>= 1U; }
  return result;
}

std::uint32_t padded_size(std::uint32_t n) {
  std::uint32_t result = 2;
  while (result < n) result <<= 1U;
  return result;
}

std::uint64_t low_mask(std::uint8_t bits) {
  return (UINT64_C(1) << bits) - 1U;
}

void validate_config(const ProtocolIIITwoRoundConfig& c) {
  require(c.session != 0U && c.fingerprint != 0U && c.party <= 1U,
          "two-round session/fingerprint/party");
  require(c.logical_n >= 2U && c.logical_n <= 1'000'000U &&
              c.padded_n == padded_size(c.logical_n) &&
              c.k >= 1U && c.k <= c.logical_n && c.target_rank < c.k,
          "two-round shape/k/target");
  require(c.rank_bits == width(c.logical_n - 1U) &&
              c.comparison_bits >= 33U + width(c.padded_n - 1U) &&
              c.comparison_bits <= 53U,
          "two-round widths");
}

void validate_material(const ProtocolIIITwoRoundConfig& c,
                       const ProtocolIIITwoRoundPartyMaterial& m) {
  const auto& a = m.cmpagg;
  require(m.target_rank == c.target_rank, "two-round target material binding");
  require(a.session == c.session && a.fingerprint == c.fingerprint &&
              a.party == c.party && a.n == c.logical_n && a.k == c.k &&
              a.comparison_bits == c.comparison_bits &&
              a.node_mask_shares.size() == c.logical_n &&
              a.edge_materials.size() ==
                  static_cast<std::size_t>(c.logical_n) * (c.logical_n - 1U) / 2U,
          "two-round CmpAgg material binding");
  require(a.carry_materials.empty() && a.sign_materials.empty() &&
              m.rank_mask_shares.size() == c.logical_n &&
              m.payload_material.size() == c.logical_n &&
              m.multiplication_material.size() == c.logical_n,
          "two-round material shape");
  const auto key_mask = low_mask(c.comparison_bits);
  const auto rank_mask = low_mask(c.rank_bits);
  for (std::uint32_t i = 0; i < c.logical_n; ++i) {
    require((a.node_mask_shares[i] & ~key_mask) == 0U &&
                (m.rank_mask_shares[i] & ~rank_mask) == 0U,
            "two-round mask share outside ring");
    const auto& d = m.payload_material[i].dpf_key;
    const auto& t = m.multiplication_material[i];
    require(d.party() == c.party && d.domain_bits() == c.rank_bits &&
                d.session() == c.session && d.fingerprint() == c.fingerprint &&
                d.slot() == i && t.party == c.party &&
                t.session == c.session && t.fingerprint == c.fingerprint &&
                t.slot == i && !t.started && !t.consumed,
            "two-round DPF/multiplication material binding");
  }
  std::size_t edge = 0;
  for (std::uint32_t left = 0; left < c.logical_n; ++left) {
    for (std::uint32_t right = left + 1U; right < c.logical_n; ++right) {
      const auto& e = a.edge_materials[edge++];
      require(e.left == left && e.right == right && e.material.party_id() == c.party &&
                  e.material.comparison_bits() == c.comparison_bits,
              "two-round CmpAgg edge binding/order");
    }
  }
}

void put_word(std::vector<std::uint8_t>& out, std::uint64_t value, std::size_t bytes) {
  for (std::size_t i = bytes; i > 0U; --i)
    out.push_back(static_cast<std::uint8_t>(value >> (8U * (i - 1U))));
}

std::uint64_t get_word(const std::vector<std::uint8_t>& in,
                       std::size_t& cursor, std::size_t bytes) {
  require(cursor <= in.size() && in.size() - cursor >= bytes,
          "two-round message truncated");
  std::uint64_t result = 0;
  for (std::size_t i = 0; i < bytes; ++i) result = (result << 8U) | in[cursor++];
  return result;
}

void put_field(std::vector<std::uint8_t>& out, ProtocolIIIField value) {
  const auto bytes = value.serialize();
  out.insert(out.end(), bytes.begin(), bytes.end());
}

ProtocolIIIField get_field(const std::vector<std::uint8_t>& in,
                           std::size_t& cursor) {
  require(cursor <= in.size() && in.size() - cursor >= 16U,
          "two-round field share truncated");
  const std::vector<std::uint8_t> bytes(in.begin() + cursor, in.begin() + cursor + 16U);
  cursor += 16U;
  return ProtocolIIIField::deserialize(bytes);
}

std::vector<std::uint8_t> header(const ProtocolIIITwoRoundConfig& c,
                                 std::uint8_t round, std::size_t body_bytes) {
  std::vector<std::uint8_t> out;
  out.reserve(kHeaderBytes + body_bytes);
  out.insert(out.end(), {'M', '5', 'T', 'R', 1U, round, c.party, 0U});
  put_word(out, c.session, 8U);
  put_word(out, c.fingerprint, 8U);
  put_word(out, c.logical_n, 4U);
  put_word(out, c.padded_n, 4U);
  put_word(out, c.k, 4U);
  put_word(out, c.target_rank, 4U);
  out.insert(out.end(), {c.comparison_bits, c.rank_bits, 0U, 0U});
  return out;
}

std::size_t check_header(const std::vector<std::uint8_t>& in,
                         const ProtocolIIITwoRoundConfig& c,
                         std::uint8_t round, std::size_t body_bytes) {
  require(in.size() == kHeaderBytes + body_bytes, "two-round message length");
  require(in[0] == 'M' && in[1] == '5' && in[2] == 'T' && in[3] == 'R' &&
              in[4] == 1U && in[5] == round && in[6] == 1U - c.party &&
              in[7] == 0U, "two-round message tag/version/round/party");
  std::size_t cursor = 8U;
  require(get_word(in, cursor, 8U) == c.session &&
              get_word(in, cursor, 8U) == c.fingerprint &&
              get_word(in, cursor, 4U) == c.logical_n &&
              get_word(in, cursor, 4U) == c.padded_n &&
              get_word(in, cursor, 4U) == c.k &&
              get_word(in, cursor, 4U) == c.target_rank &&
              get_word(in, cursor, 1U) == c.comparison_bits &&
              get_word(in, cursor, 1U) == c.rank_bits &&
              get_word(in, cursor, 2U) == 0U,
          "two-round message metadata mismatch");
  return cursor;
}
}  // namespace

std::pair<ProtocolIIITwoRoundPartyMaterial, ProtocolIIITwoRoundPartyMaterial>
protocol_iii_two_round_preprocess(const ProtocolIIITwoRoundConfig& config,
                                  osuCrypto::PRNG& generator) {
  validate_config(config);
  ProtocolIIITwoRoundPartyMaterial parties[2];
  for (std::uint8_t p = 0; p < 2U; ++p) {
    auto& m = parties[p];
    m.target_rank = config.target_rank;
    m.cmpagg.session = config.session;
    m.cmpagg.fingerprint = config.fingerprint;
    m.cmpagg.party = p;
    m.cmpagg.n = config.logical_n;
    m.cmpagg.k = config.k;
    m.cmpagg.comparison_bits = config.comparison_bits;
    m.cmpagg.node_mask_shares.reserve(config.logical_n);
    m.rank_mask_shares.reserve(config.logical_n);
    m.payload_material.reserve(config.logical_n);
    m.multiplication_material.reserve(config.logical_n);
  }
  const auto comparison_mask = low_mask(config.comparison_bits);
  const auto rank_mask = low_mask(config.rank_bits);
  std::vector<std::uint64_t> comparison_masks(config.logical_n);
  for (std::uint32_t i = 0; i < config.logical_n; ++i) {
    comparison_masks[i] = generator.get<std::uint64_t>() & comparison_mask;
    const auto cmp0 = generator.get<std::uint64_t>() & comparison_mask;
    parties[0].cmpagg.node_mask_shares.push_back(cmp0);
    parties[1].cmpagg.node_mask_shares.push_back(
        (comparison_masks[i] - cmp0) & comparison_mask);

    const auto r_rank = generator.get<std::uint64_t>() & rank_mask;
    const auto rank0 = generator.get<std::uint64_t>() & rank_mask;
    parties[0].rank_mask_shares.push_back(rank0);
    parties[1].rank_mask_shares.push_back((r_rank - rank0) & rank_mask);
    auto payload = protocol_iii_field_payload_preprocess(
        config.rank_bits, r_rank,
        protocol_iii_sample_nonzero_field_element(generator),
        config.session, config.fingerprint, i, generator);
    parties[0].payload_material.push_back(std::move(payload.first));
    parties[1].payload_material.push_back(std::move(payload.second));
    auto mul = protocol_iii_field_mul_preprocess(
        config.session, config.fingerprint, i, generator);
    parties[0].multiplication_material.push_back(std::move(mul.first));
    parties[1].multiplication_material.push_back(std::move(mul.second));
  }
  for (std::uint32_t left = 0; left < config.logical_n; ++left) {
    for (std::uint32_t right = left + 1U; right < config.logical_n; ++right) {
      ProtocolIUcmpMaterial edge(config.comparison_bits,
                                 comparison_masks[left], comparison_masks[right]);
      parties[0].cmpagg.edge_materials.emplace_back(
          left, right, edge.export_party_material(0));
      parties[1].cmpagg.edge_materials.emplace_back(
          left, right, edge.export_party_material(1));
    }
  }
  return {std::move(parties[0]), std::move(parties[1])};
}

std::size_t protocol_iii_two_round_offline_material_bytes(
    const ProtocolIIITwoRoundPartyMaterial& material) {
  std::size_t size = serialize_party_package(material.cmpagg).size();
  size += 8U * material.rank_mask_shares.size();
  for (const auto& item : material.payload_material)
    size += 16U + item.dpf_key.serialize().size();
  for (const auto& item : material.multiplication_material)
    size += item.serialize().size();
  return size;
}

ProtocolIIITwoRoundParty::ProtocolIIITwoRoundParty(
    ProtocolIIITwoRoundConfig config, ProtocolIIITwoRoundPartyMaterial&& material,
    std::vector<std::uint64_t> priority_key_shares,
    std::vector<ProtocolIIIField> encoded_payload_shares)
    : config_(config), material_(std::move(material)),
      key_shares_(std::move(priority_key_shares)),
      payload_shares_(std::move(encoded_payload_shares)) {
  validate_config(config_);
  validate_material(config_, material_);
  require(key_shares_.size() == config_.padded_n &&
              payload_shares_.size() == config_.logical_n,
          "two-round input share count");
  const auto mask = low_mask(config_.comparison_bits);
  for (const auto value : key_shares_)
    require((value & ~mask) == 0U, "two-round priority-key share outside ring");
}

std::vector<std::uint8_t> ProtocolIIITwoRoundParty::prepare_round1() {
  require(phase_ == Phase::fresh, "two-round R1 lifecycle");
  phase_ = Phase::failed;
  const auto n = config_.logical_n;
  local_masked_keys_.reserve(n);
  local_openings_.reserve(n);
  edge_materials_.reserve(material_.cmpagg.edge_materials.size());
  auto out = header(config_, 1U, static_cast<std::size_t>(n) * 40U);
  for (std::uint32_t i = 0; i < n; ++i) {
    const auto masked = protocol_i_mask_priority_key_share(
        config_.comparison_bits, key_shares_[i], material_.cmpagg.node_mask_shares[i]);
    local_masked_keys_.push_back(masked);
    put_word(out, masked, 8U);
  }
  for (std::uint32_t i = 0; i < n; ++i) {
    const auto opening = protocol_iii_field_mul_start(
        material_.multiplication_material[i], config_.party, config_.session,
        config_.fingerprint, i, payload_shares_[i],
        material_.payload_material[i].mask_share);
    local_openings_.push_back(opening);
    put_field(out, opening.d_share);
    put_field(out, opening.e_share);
  }
  for (auto& edge : material_.cmpagg.edge_materials)
    edge_materials_.push_back(std::move(edge.material));
  material_.cmpagg.node_mask_shares.clear();
  material_.cmpagg.edge_materials.clear();
  key_shares_.clear();
  payload_shares_.clear();
  phase_ = Phase::round1_prepared;
  return out;
}

void ProtocolIIITwoRoundParty::consume_round1(
    const std::vector<std::uint8_t>& peer) {
  require(phase_ == Phase::round1_prepared, "two-round R1 receive lifecycle");
  phase_ = Phase::failed;
  const auto n = config_.logical_n;
  auto cursor = check_header(peer, config_, 1U, static_cast<std::size_t>(n) * 40U);
  std::vector<std::uint64_t> masked_keys(n);
  for (std::uint32_t i = 0; i < n; ++i) {
    const auto share = get_word(peer, cursor, 8U);
    require((share & ~low_mask(config_.comparison_bits)) == 0U,
            "two-round peer comparison share outside ring");
    masked_keys[i] = (local_masked_keys_[i] + share) & low_mask(config_.comparison_bits);
  }
  rank_shares_ = protocol_i_cmpagg_eval_party(
      config_.party, config_.comparison_bits, masked_keys, edge_materials_);
  for (auto& rank : rank_shares_) rank &= low_mask(config_.rank_bits);
  masked_payload_shares_.reserve(n);
  for (std::uint32_t i = 0; i < n; ++i) {
    const auto d = ProtocolIIIField::add(local_openings_[i].d_share,
                                         get_field(peer, cursor));
    const auto e = ProtocolIIIField::add(local_openings_[i].e_share,
                                         get_field(peer, cursor));
    masked_payload_shares_.push_back(protocol_iii_field_mul_finish(
        material_.multiplication_material[i], config_.party, config_.session,
        config_.fingerprint, i, d, e));
  }
  require(cursor == peer.size(), "two-round R1 trailing bytes");
  local_masked_keys_.clear();
  local_openings_.clear();
  edge_materials_.clear();
  phase_ = Phase::round1_consumed;
}

std::vector<std::uint8_t> ProtocolIIITwoRoundParty::prepare_round2() {
  require(phase_ == Phase::round1_consumed, "two-round R2 lifecycle");
  phase_ = Phase::failed;
  const auto n = config_.logical_n;
  auto out = header(config_, 2U, static_cast<std::size_t>(n) * 24U);
  local_masked_ranks_.reserve(n);
  for (std::uint32_t i = 0; i < n; ++i) {
    const auto share = (rank_shares_[i] + material_.rank_mask_shares[i]) &
                       low_mask(config_.rank_bits);
    local_masked_ranks_.push_back(share);
    put_word(out, share, 8U);
  }
  for (const auto share : masked_payload_shares_) put_field(out, share);
  material_.rank_mask_shares.clear();
  phase_ = Phase::round2_prepared;
  return out;
}

ProtocolIIITwoRoundParty::OpenedRound2
ProtocolIIITwoRoundParty::open_round2(
    const std::vector<std::uint8_t>& peer) {
  require(phase_ == Phase::round2_prepared, "two-round R2 receive lifecycle");
  phase_ = Phase::failed;
  const auto n = config_.logical_n;
  auto cursor = check_header(peer, config_, 2U, static_cast<std::size_t>(n) * 24U);
  OpenedRound2 opened;
  opened.masked_ranks.reserve(n);
  opened.masked_payloads.reserve(n);
  for (std::uint32_t i = 0; i < n; ++i) {
    const auto share = get_word(peer, cursor, 8U);
    require((share & ~low_mask(config_.rank_bits)) == 0U,
            "two-round peer rank-mask share outside ring");
    opened.masked_ranks.push_back(
        (local_masked_ranks_[i] + share) & low_mask(config_.rank_bits));
  }
  for (std::uint32_t i = 0; i < n; ++i)
    opened.masked_payloads.push_back(ProtocolIIIField::add(
        masked_payload_shares_[i], get_field(peer, cursor)));
  require(cursor == peer.size(), "two-round R2 trailing bytes");
  return opened;
}

ProtocolIIIField ProtocolIIITwoRoundParty::consume_round2(
    const std::vector<std::uint8_t>& peer) {
  const auto opened = open_round2(peer);
  ProtocolIIIField result;
  for (std::uint32_t i = 0; i < config_.logical_n; ++i) {
    const auto& dpf = material_.payload_material[i].dpf_key;
    const auto x = (opened.masked_ranks[i] - config_.target_rank) &
                   low_mask(config_.rank_bits);
    const auto indicator = protocol_iii_field_dpf_eval(
        dpf, config_.party, config_.session, config_.fingerprint, i, x);
    result = ProtocolIIIField::add(result,
        ProtocolIIIField::mul(opened.masked_payloads[i], indicator));
  }
  masked_payload_shares_.clear();
  local_masked_ranks_.clear();
  phase_ = Phase::finished;
  return result;
}

std::vector<ProtocolIIIField> ProtocolIIITwoRoundParty::consume_round2_sort(
    const std::vector<std::uint8_t>& peer) {
  require(config_.k == config_.logical_n && config_.target_rank == 0U,
          "two-round Fsort configuration");
  const auto opened = open_round2(peer);
  const auto n = config_.logical_n;
  const auto rank_mask = low_mask(config_.rank_bits);
  std::vector<ProtocolIIIField> out(n);
  for (std::uint32_t i = 0; i < n; ++i) {
    const auto& dpf = material_.payload_material[i].dpf_key;
    const auto full = protocol_iii_field_dpf_full_eval(
        dpf, config_.party, config_.session, config_.fingerprint, i,
        config_.rank_bits);
    // FullEval[x] represents f_{r_rank[i],s_i^-1}(x). For public masked
    // rank m_i, target t queries x=m_i-t in the project's rank ring.
    for (std::uint32_t target = 0; target < n; ++target) {
      const auto x = (opened.masked_ranks[i] - target) & rank_mask;
      out[target] = ProtocolIIIField::add(out[target],
          ProtocolIIIField::mul(opened.masked_payloads[i], full[x]));
    }
  }
  masked_payload_shares_.clear();
  local_masked_ranks_.clear();
  phase_ = Phase::finished;
  return out;
}

ProtocolIIITwoRoundOutput protocol_iii_two_round_party(
    ProtocolIIITwoRoundConfig config,
    ProtocolIIITwoRoundPartyMaterial&& material,
    std::vector<std::uint64_t> priority_key_shares,
    std::vector<ProtocolIIIField> encoded_payload_shares,
    ProtocolIIITwoRoundFds fds,
    int timeout_ms) {
  require(fds.round1_fd >= 0 && fds.round2_fd >= 0 && timeout_ms > 0 &&
              fds.round1_fd != fds.round2_fd,
          "two-round transport descriptor/timeout");
  ProtocolIIITwoRoundParty state(config, std::move(material),
      std::move(priority_key_shares), std::move(encoded_payload_shares));
  ProtocolIIITwoRoundOutput output;
  output.metrics.round1_logical_bits = static_cast<std::uint64_t>(config.logical_n) *
      (config.comparison_bits + 2U * 127U);
  output.metrics.round2_logical_bits = static_cast<std::uint64_t>(config.logical_n) *
      (config.rank_bits + 127U);
  const ProtocolIFrameConfig frame1{
      config.session, config.fingerprint, config.logical_n, config.k,
      config.comparison_bits, config.party,
      static_cast<std::uint8_t>(1U - config.party), 6U, 1U};
  const ProtocolIFrameConfig frame2{
      config.session, config.fingerprint, config.logical_n, config.k,
      config.comparison_bits, config.party,
      static_cast<std::uint8_t>(1U - config.party), 6U, 2U};
  // Both descriptors are owned before either online exchange begins.
  ProtocolIFramedChannel channel1(fds.round1_fd, frame1, timeout_ms);
  ProtocolIFramedChannel channel2(fds.round2_fd, frame2, timeout_ms);
  {
    const auto outbound = state.prepare_round1();
    std::vector<std::uint8_t> inbound;
    if (config.party == 0U) {
      channel1.send(outbound);
      inbound = channel1.receive();
    } else {
      inbound = channel1.receive();
      channel1.send(outbound);
    }
    state.consume_round1(inbound);
    output.metrics.round1_sent_bytes = channel1.sent_bytes();
    output.metrics.round1_received_bytes = channel1.received_bytes();
  }
  {
    const auto outbound = state.prepare_round2();
    std::vector<std::uint8_t> inbound;
    if (config.party == 0U) {
      channel2.send(outbound);
      inbound = channel2.receive();
    } else {
      inbound = channel2.receive();
      channel2.send(outbound);
    }
    output.selected_share = state.consume_round2(inbound);
    output.metrics.round2_sent_bytes = channel2.sent_bytes();
    output.metrics.round2_received_bytes = channel2.received_bytes();
  }
  return output;
}

}  // namespace moe_topk
