#include <moe_topk/protocol_iii_raw_score_mask_package.h>

#include <moe_topk/protocol_i_party_package.h>

#include <FSS/comms.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace moe_topk {
namespace {
constexpr std::size_t kMaxBytes = 64U * 1024U * 1024U;
constexpr std::uint64_t kScoreMask = (UINT64_C(1) << 34U) - 1U;

void require(bool ok, const char* message) {
  if (!ok) throw std::invalid_argument(message);
}
void put(std::vector<std::uint8_t>& out, std::uint64_t value, std::size_t width) {
  require(out.size() <= kMaxBytes - width, "F1 bundle exceeds limit");
  for (std::size_t i = width; i != 0U; --i)
    out.push_back(static_cast<std::uint8_t>(value >> (8U * (i - 1U))));
}
std::uint64_t get(const std::vector<std::uint8_t>& in, std::size_t& offset,
                  std::size_t width) {
  require(offset <= in.size() && in.size() - offset >= width,
          "F1 bundle truncated");
  std::uint64_t value = 0;
  for (std::size_t i = 0; i < width; ++i) value = (value << 8U) | in[offset++];
  return value;
}
void append(std::vector<std::uint8_t>& out,
            const std::vector<std::uint8_t>& section) {
  require(section.size() <= kMaxBytes - out.size(), "F1 bundle exceeds limit");
  out.insert(out.end(), section.begin(), section.end());
}
std::vector<std::uint8_t> section(const std::vector<std::uint8_t>& in,
                                  std::size_t& offset, std::size_t length) {
  require(offset <= in.size() && length <= in.size() - offset,
          "F1 bundle section truncated");
  std::vector<std::uint8_t> result(in.begin() + offset, in.begin() + offset + length);
  offset += length;
  return result;
}
std::size_t dpf_key_bytes(std::uint8_t bits) {
  require(bits >= 1U && bits <= 20U, "F1 DPF input width");
  const std::size_t point_bytes = bits <= 8U ? 1U : bits <= 16U ? 2U : 4U;
  return (static_cast<std::size_t>(bits) + 1U) * sizeof(osuCrypto::block) +
         2U * point_bytes + 2U * sizeof(std::uint64_t);
}
void validate_public(const ProtocolIIIRawScoreMaskConfig& c) {
  const auto& s = c.score_input;
  const auto& g = c.grank;
  const auto& r = c.routing;
  require(c.material_id != 0U && s.session != 0U && s.fingerprint != 0U &&
              s.party < 2U && s.logical_n >= 2U && s.logical_n <= 1'000'000U &&
              s.padded_n >= s.logical_n && (s.padded_n & (s.padded_n - 1U)) == 0U &&
              s.k >= 1U && s.k <= s.logical_n && s.index_bits >= 1U &&
              s.comparison_bits == 33U + s.index_bits &&
              s.comparison_bits >= 34U && s.comparison_bits <= 53U &&
              g.session == s.session && g.fingerprint == s.fingerprint &&
              g.party == s.party && g.logical_n == s.logical_n &&
              g.padded_n == s.padded_n && g.k == s.k &&
              g.comparison_bits == s.comparison_bits &&
              r.session == s.session && r.fingerprint == s.fingerprint &&
              r.party == s.party && r.logical_n == s.logical_n &&
              r.k == s.k && r.comparison_bits == s.comparison_bits &&
              r.rank_bits == g.rank_bits && r.rank_bits >= 1U &&
              r.rank_bits <= 20U, "F1 bundle public binding");
}
void validate_material(const ProtocolIIIRawScoreMaskConfig& c,
                       const ProtocolIIIRawScoreMaskMaterial& m) {
  const auto& s = m.score_input_package;
  const auto& g = m.grank_package;
  const auto& r = m.routing_material;
  require(!m.started && m.material_id == c.material_id &&
              s.session == c.score_input.session &&
              s.fingerprint == c.score_input.fingerprint &&
              s.party == c.score_input.party && s.n == c.score_input.padded_n &&
              s.k == c.score_input.k &&
              s.comparison_bits == c.score_input.comparison_bits &&
              s.node_mask_shares.empty() && s.edge_materials.empty() &&
              s.carry_materials.size() == s.n && s.sign_materials.size() == s.n &&
              g.session == c.grank.session && g.fingerprint == c.grank.fingerprint &&
              g.party == c.grank.party && g.n == c.grank.logical_n &&
              g.k == c.grank.k && g.comparison_bits == c.grank.comparison_bits &&
              g.carry_materials.empty() && g.sign_materials.empty() &&
              r.session == c.routing.session &&
              r.fingerprint == c.routing.fingerprint &&
              r.party == c.routing.party && r.logical_n == c.routing.logical_n &&
              r.k == c.routing.k && r.rank_bits == c.routing.rank_bits &&
              !r.started && r.rank_mask_shares.size() == r.logical_n &&
              r.dpf_keys.size() == r.logical_n,
          "F1 bundle material binding");
  const auto rank_mask = (UINT64_C(1) << c.routing.rank_bits) - 1U;
  for (const auto share : r.rank_mask_shares)
    require((share & ~rank_mask) == 0U, "F1 rank mask outside ring");
  for (const auto& key : r.dpf_keys) {
    const auto& native = key.native_key();
    require(native.s != nullptr && native.bin == c.routing.rank_bits &&
                native.bout == 64, "F1 DPF key width");
  }
}
std::vector<std::uint8_t> encode_score(const ProtocolIPartyPackage& p) {
  std::vector<std::uint8_t> out;
  for (const auto stage : {UINT8_C(1), UINT8_C(2)}) {
    const auto& items = stage == 1U ? p.carry_materials : p.sign_materials;
    for (std::uint32_t slot = 0; slot < p.n; ++slot) {
      const auto& item = items[slot];
      require(item.slot == slot && item.stage == stage &&
                  item.material.party_id() == p.party &&
                  item.material.comparison_bits() == 34 &&
                  (item.left_mask_share & ~kScoreMask) == 0U &&
                  (item.right_mask_share & ~kScoreMask) == 0U,
              "F1 score material binding");
      const auto key = item.material.serialize();
      require(key.size() <= UINT32_MAX, "F1 score key length");
      put(out, slot, 4U); put(out, stage, 1U);
      put(out, item.left_mask_share, 8U);
      put(out, item.right_mask_share, 8U);
      put(out, key.size(), 4U); append(out, key);
    }
  }
  return out;
}
ProtocolIPartyPackage decode_score(const ProtocolIIIRawScoreMaskConfig& c,
                                   const std::vector<std::uint8_t>& bytes) {
  ProtocolIPartyPackage p;
  p.party = c.score_input.party;
  p.session = c.score_input.session;
  p.fingerprint = c.score_input.fingerprint;
  p.n = c.score_input.padded_n;
  p.k = c.score_input.k;
  p.comparison_bits = c.score_input.comparison_bits;
  std::size_t offset = 0;
  for (const auto stage : {UINT8_C(1), UINT8_C(2)}) {
    auto& items = stage == 1U ? p.carry_materials : p.sign_materials;
    items.reserve(p.n);
    for (std::uint32_t slot = 0; slot < p.n; ++slot) {
      const auto encoded_slot = get(bytes, offset, 4U);
      const auto encoded_stage = get(bytes, offset, 1U);
      const auto left = get(bytes, offset, 8U);
      const auto right = get(bytes, offset, 8U);
      const auto length = get(bytes, offset, 4U);
      require(encoded_slot == slot && encoded_stage == stage &&
                  (left & ~kScoreMask) == 0U && (right & ~kScoreMask) == 0U &&
                  length <= bytes.size() - offset,
              "F1 score material identity");
      auto key = ProtocolIUcmpPartyMaterial::deserialize(
          section(bytes, offset, static_cast<std::size_t>(length)));
      require(key.party_id() == p.party && key.comparison_bits() == 34,
              "F1 score key party/width");
      items.emplace_back(slot, stage, left, right, std::move(key));
    }
  }
  require(offset == bytes.size(), "F1 score material trailing bytes");
  return p;
}
std::vector<std::uint8_t> encode_dpf(
    const ProtocolIIIDpfRoutingPartyMaterial& material) {
  const auto expected = static_cast<std::size_t>(material.logical_n) *
                        dpf_key_bytes(material.rank_bits);
  require(expected <= kMaxBytes, "F1 DPF key tail exceeds limit");
  std::vector<std::uint8_t> bytes(expected);
  char* cursor = reinterpret_cast<char*>(bytes.data());
  Peer sender(&cursor);
  for (const auto& owned : material.dpf_keys)
    sender.send_dpf_keypack(owned.native_key());
  const auto size = static_cast<std::size_t>(sender.bytesSent());
  delete static_cast<MemBuf*>(sender.keyBuf);
  require(size == expected,
          "F1 DPF serialized size");
  bytes.resize(size);
  return bytes;
}
void decode_dpf(const ProtocolIIIRawScoreMaskConfig& c,
                const std::vector<std::uint8_t>& bytes,
                ProtocolIIIDpfRoutingPartyMaterial& material) {
  require(bytes.size() == static_cast<std::size_t>(c.routing.logical_n) *
                              dpf_key_bytes(c.routing.rank_bits),
          "F1 DPF key-tail length");
  char* cursor = reinterpret_cast<char*>(const_cast<std::uint8_t*>(bytes.data()));
  Dealer receiver(&cursor);
  try {
    material.dpf_keys.reserve(c.routing.logical_n);
    for (std::uint32_t i = 0; i < c.routing.logical_n; ++i) {
      auto wire = receiver.recv_dpf_keypack(c.routing.rank_bits, 64);
      require(wire.s != nullptr, "F1 DPF key seed");
      DPFKeyPack owned(c.routing.rank_bits, 64);
      std::memcpy(owned.s, wire.s,
                  (static_cast<std::size_t>(c.routing.rank_bits) + 1U) *
                      sizeof(osuCrypto::block));
      owned.tLcw = wire.tLcw;
      owned.tRcw = wire.tRcw;
      owned.payload = wire.payload;
      material.dpf_keys.emplace_back(std::move(owned));
      wire.s = nullptr;  // Borrowed from bytes, never delete.
    }
    require(cursor == reinterpret_cast<char*>(
                          const_cast<std::uint8_t*>(bytes.data() + bytes.size())),
            "F1 DPF tail trailing bytes");
  } catch (...) {
    delete static_cast<MemBuf*>(receiver.keyBuf);
    throw;
  }
  delete static_cast<MemBuf*>(receiver.keyBuf);
}
}  // namespace

std::vector<std::uint8_t> protocol_iii_raw_score_mask_serialize_bundle(
    const ProtocolIIIRawScoreMaskConfig& c,
    const ProtocolIIIRawScoreMaskMaterial& m) {
  validate_public(c); validate_material(c, m);
  const auto score = encode_score(m.score_input_package);
  const auto grank = serialize_party_package(m.grank_package);
  const auto dpf = encode_dpf(m.routing_material);
  require(score.size() <= UINT32_MAX && grank.size() <= UINT32_MAX &&
              dpf.size() <= UINT32_MAX, "F1 bundle section size");
  std::vector<std::uint8_t> out{'M','5','F','1',1U,c.score_input.party,
                                c.score_input.index_bits,c.score_input.comparison_bits,
                                c.routing.rank_bits,0U};
  put(out, c.score_input.session, 8U);
  put(out, c.score_input.fingerprint, 8U);
  put(out, c.material_id, 8U);
  put(out, c.score_input.logical_n, 4U);
  put(out, c.score_input.padded_n, 4U);
  put(out, c.score_input.k, 4U);
  put(out, score.size(), 4U);
  put(out, grank.size(), 4U);
  put(out, dpf.size(), 4U);
  append(out, score); append(out, grank);
  for (const auto share : m.routing_material.rank_mask_shares) put(out, share, 8U);
  append(out, dpf);
  return out;
}

ProtocolIIIRawScoreMaskMaterial protocol_iii_raw_score_mask_deserialize_bundle(
    const ProtocolIIIRawScoreMaskConfig& c,
    const std::vector<std::uint8_t>& bytes) {
  validate_public(c);
  require(bytes.size() >= 58U && bytes.size() <= kMaxBytes &&
              bytes[0]=='M' && bytes[1]=='5' && bytes[2]=='F' && bytes[3]=='1' &&
              bytes[4]==1U && bytes[5]==c.score_input.party &&
              bytes[6]==c.score_input.index_bits &&
              bytes[7]==c.score_input.comparison_bits &&
              bytes[8]==c.routing.rank_bits && bytes[9]==0U,
          "F1 bundle header/version/party/width");
  std::size_t offset = 10U;
  require(get(bytes, offset, 8U) == c.score_input.session &&
              get(bytes, offset, 8U) == c.score_input.fingerprint &&
              get(bytes, offset, 8U) == c.material_id &&
              get(bytes, offset, 4U) == c.score_input.logical_n &&
              get(bytes, offset, 4U) == c.score_input.padded_n &&
              get(bytes, offset, 4U) == c.score_input.k,
          "F1 bundle session/material/shape");
  const auto score_length = static_cast<std::size_t>(get(bytes, offset, 4U));
  const auto grank_length = static_cast<std::size_t>(get(bytes, offset, 4U));
  const auto dpf_length = static_cast<std::size_t>(get(bytes, offset, 4U));
  require(score_length <= bytes.size() - offset &&
              grank_length <= bytes.size() - offset - score_length &&
              static_cast<std::size_t>(c.routing.logical_n) <=
                  (bytes.size() - offset - score_length - grank_length) / 8U,
          "F1 bundle section lengths");
  ProtocolIIIRawScoreMaskMaterial m;
  m.material_id = c.material_id;
  m.score_input_package = decode_score(c, section(bytes, offset, score_length));
  m.grank_package = deserialize_party_package(section(bytes, offset, grank_length),
                                               c.score_input.party);
  auto& r = m.routing_material;
  r.session = c.routing.session; r.fingerprint = c.routing.fingerprint;
  r.party = c.routing.party; r.logical_n = c.routing.logical_n;
  r.k = c.routing.k; r.rank_bits = c.routing.rank_bits;
  r.rank_mask_shares.resize(r.logical_n);
  const auto mask = (UINT64_C(1) << r.rank_bits) - 1U;
  for (auto& share : r.rank_mask_shares) {
    share = get(bytes, offset, 8U);
    require((share & ~mask) == 0U, "F1 rank mask canonical");
  }
  require(dpf_length == bytes.size() - offset, "F1 bundle DPF/trailing length");
  decode_dpf(c, section(bytes, offset, dpf_length), r);
  validate_material(c, m);
  return m;
}

}  // namespace moe_topk
