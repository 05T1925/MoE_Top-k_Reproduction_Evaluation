#include <moe_topk/protocol_iii_two_round_package.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace moe_topk {
namespace {
constexpr std::size_t kMaxBundleBytes = 64U * 1024U * 1024U;
constexpr std::uint8_t kFieldId = 1U;  // F_(2^127-1), 16-byte canonical BE.

void require(bool ok, const char* message) {
  if (!ok) throw std::invalid_argument(message);
}

void append_word(std::vector<std::uint8_t>& out, std::uint64_t value,
                 std::size_t count) {
  require(out.size() <= kMaxBundleBytes - count, "M5-F bundle too large");
  for (std::size_t i = count; i != 0U; --i)
    out.push_back(static_cast<std::uint8_t>(value >> (8U * (i - 1U))));
}

void append_bytes(std::vector<std::uint8_t>& out,
                  const std::vector<std::uint8_t>& bytes) {
  require(bytes.size() <= kMaxBundleBytes - out.size(), "M5-F bundle too large");
  out.insert(out.end(), bytes.begin(), bytes.end());
}

std::uint64_t read_word(const std::vector<std::uint8_t>& in,
                        std::size_t& offset, std::size_t count) {
  require(offset <= in.size() && in.size() - offset >= count,
          "M5-F bundle truncated");
  std::uint64_t value = 0;
  for (std::size_t i = 0; i < count; ++i)
    value = (value << 8U) | in[offset++];
  return value;
}

std::vector<std::uint8_t> read_bytes(const std::vector<std::uint8_t>& in,
                                     std::size_t& offset, std::size_t count) {
  require(offset <= in.size() && in.size() - offset >= count,
          "M5-F bundle section truncated");
  std::vector<std::uint8_t> result(in.begin() + offset, in.begin() + offset + count);
  offset += count;
  return result;
}

void require_config(const ProtocolIIITwoRoundConfig& c) {
  require(c.session != 0U && c.fingerprint != 0U && c.party <= 1U &&
              c.logical_n >= 2U && c.logical_n <= 1'000'000U &&
              c.k != 0U && c.k <= c.logical_n && c.target_rank < c.k &&
              c.comparison_bits >= 34U && c.comparison_bits <= 53U &&
              c.rank_bits >= 1U && c.rank_bits <= 20U,
          "M5-F bundle public configuration");
}

void require_material(const ProtocolIIITwoRoundConfig& c,
                      const ProtocolIIITwoRoundPartyMaterial& m) {
  const auto& a = m.cmpagg;
  require(m.target_rank == c.target_rank && a.session == c.session &&
              a.fingerprint == c.fingerprint && a.party == c.party &&
              a.n == c.logical_n && a.k == c.k &&
              a.comparison_bits == c.comparison_bits &&
              a.node_mask_shares.size() == c.logical_n &&
              a.edge_materials.size() ==
                  static_cast<std::size_t>(c.logical_n) * (c.logical_n - 1U) / 2U &&
              a.carry_materials.empty() && a.sign_materials.empty() &&
              m.rank_mask_shares.size() == c.logical_n &&
              m.payload_material.size() == c.logical_n &&
              m.multiplication_material.size() == c.logical_n,
          "M5-F bundle material binding/shape");
  for (std::uint32_t i = 0; i < c.logical_n; ++i) {
    const auto& d = m.payload_material[i].dpf_key;
    const auto& t = m.multiplication_material[i];
    require(d.party() == c.party && d.domain_bits() == c.rank_bits &&
                d.session() == c.session && d.fingerprint() == c.fingerprint &&
                d.slot() == i && t.party == c.party && t.session == c.session &&
                t.fingerprint == c.fingerprint && t.slot == i &&
                !t.started && !t.consumed,
            "M5-F bundle field material binding");
  }
}
}  // namespace

std::vector<std::uint8_t> protocol_iii_two_round_serialize_bundle(
    const ProtocolIIITwoRoundConfig& c,
    const ProtocolIIITwoRoundPartyMaterial& m,
    std::uint64_t material_id) {
  require_config(c);
  require(material_id != 0U, "M5-F bundle material identity");
  require_material(c, m);
  std::vector<std::uint8_t> out;
  out.reserve(4096U);
  out.insert(out.end(), {'M', '5', 'F', 'B', 1U, kFieldId, c.party, 0U});
  append_word(out, c.session, 8U);
  append_word(out, c.fingerprint, 8U);
  append_word(out, material_id, 8U);
  append_word(out, c.logical_n, 4U);
  append_word(out, c.padded_n, 4U);
  append_word(out, c.k, 4U);
  append_word(out, c.target_rank, 4U);
  append_word(out, c.comparison_bits, 1U);
  append_word(out, c.rank_bits, 1U);
  append_word(out, 0U, 2U);

  const auto package = serialize_party_package(m.cmpagg);
  require(package.size() <= UINT32_MAX, "M5-F CmpAgg package too large");
  append_word(out, package.size(), 4U);
  append_bytes(out, package);
  append_word(out, m.rank_mask_shares.size(), 4U);
  for (const auto share : m.rank_mask_shares) append_word(out, share, 8U);
  append_word(out, m.payload_material.size(), 4U);
  for (const auto& item : m.payload_material) {
    append_bytes(out, item.mask_share.serialize());
    const auto key = item.dpf_key.serialize();
    require(key.size() <= UINT32_MAX, "M5-F DPF key too large");
    append_word(out, key.size(), 4U);
    append_bytes(out, key);
  }
  append_word(out, m.multiplication_material.size(), 4U);
  for (const auto& item : m.multiplication_material) {
    const auto encoded = item.serialize();
    require(encoded.size() <= UINT32_MAX, "M5-F field triple too large");
    append_word(out, encoded.size(), 4U);
    append_bytes(out, encoded);
  }
  return out;
}

ProtocolIIITwoRoundOfflineBundle protocol_iii_two_round_deserialize_bundle(
    const ProtocolIIITwoRoundConfig& c, std::uint64_t expected_material_id,
    const std::vector<std::uint8_t>& bytes) {
  require_config(c);
  require(expected_material_id != 0U && bytes.size() <= kMaxBundleBytes &&
              bytes.size() >= 56U,
          "M5-F bundle length/identity");
  require(bytes[0] == 'M' && bytes[1] == '5' && bytes[2] == 'F' &&
              bytes[3] == 'B' && bytes[4] == 1U &&
              bytes[5] == kFieldId && bytes[6] == c.party && bytes[7] == 0U,
          "M5-F bundle tag/version/field/party");
  std::size_t offset = 8U;
  const auto session = read_word(bytes, offset, 8U);
  const auto fingerprint = read_word(bytes, offset, 8U);
  const auto material_id = read_word(bytes, offset, 8U);
  const auto n = read_word(bytes, offset, 4U);
  const auto padded_n = read_word(bytes, offset, 4U);
  const auto k = read_word(bytes, offset, 4U);
  const auto target = read_word(bytes, offset, 4U);
  const auto comparison_bits = read_word(bytes, offset, 1U);
  const auto rank_bits = read_word(bytes, offset, 1U);
  const auto reserved = read_word(bytes, offset, 2U);
  require(session == c.session && fingerprint == c.fingerprint &&
              material_id == expected_material_id && n == c.logical_n &&
              padded_n == c.padded_n && k == c.k &&
              target == c.target_rank && comparison_bits == c.comparison_bits &&
              rank_bits == c.rank_bits && reserved == 0U,
          "M5-F bundle metadata mismatch");
  const auto package_length = read_word(bytes, offset, 4U);
  require(package_length <= bytes.size() - offset,
          "M5-F bundle CmpAgg package truncated");
  ProtocolIIITwoRoundOfflineBundle result;
  result.material_id = material_id;
  result.material.target_rank = c.target_rank;
  result.material.cmpagg = deserialize_party_package(
      read_bytes(bytes, offset, static_cast<std::size_t>(package_length)), c.party);
  require(read_word(bytes, offset, 4U) == c.logical_n,
          "M5-F bundle rank-mask count");
  result.material.rank_mask_shares.reserve(c.logical_n);
  const auto rank_mask = (UINT64_C(1) << c.rank_bits) - 1U;
  for (std::uint32_t i = 0; i < c.logical_n; ++i) {
    const auto share = read_word(bytes, offset, 8U);
    require((share & ~rank_mask) == 0U, "M5-F bundle rank-mask share");
    result.material.rank_mask_shares.push_back(share);
  }
  require(read_word(bytes, offset, 4U) == c.logical_n,
          "M5-F bundle DPF material count");
  result.material.payload_material.reserve(c.logical_n);
  for (std::uint32_t i = 0; i < c.logical_n; ++i) {
    const auto mask_share = ProtocolIIIField::deserialize(
        read_bytes(bytes, offset, 16U));
    const auto key_length = read_word(bytes, offset, 4U);
    require(key_length <= bytes.size() - offset,
            "M5-F bundle field DPF key truncated");
    auto key = ProtocolIIIFieldDpfPartyKey::deserialize(
        read_bytes(bytes, offset, static_cast<std::size_t>(key_length)));
    result.material.payload_material.push_back(
        {mask_share, std::move(key)});
  }
  require(read_word(bytes, offset, 4U) == c.logical_n,
          "M5-F bundle multiplication count");
  result.material.multiplication_material.reserve(c.logical_n);
  for (std::uint32_t i = 0; i < c.logical_n; ++i) {
    const auto length = read_word(bytes, offset, 4U);
    require(length <= bytes.size() - offset,
            "M5-F bundle multiplication material truncated");
    result.material.multiplication_material.push_back(
        ProtocolIIIFieldMulPartyMaterial::deserialize(
            read_bytes(bytes, offset, static_cast<std::size_t>(length))));
  }
  require(offset == bytes.size(), "M5-F bundle trailing bytes");
  require_material(c, result.material);
  return result;
}

}  // namespace moe_topk
