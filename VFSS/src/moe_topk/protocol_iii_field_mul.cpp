#include <moe_topk/protocol_iii_field_mul.h>

#include <moe_topk/protocol_iii_field_payload.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace moe_topk {
namespace {
constexpr std::size_t kWireBytes = 4U + 1U + 1U + 2U + 8U + 8U + 4U + 3U * 16U;

void require(bool condition, const char* message) {
  if (!condition) throw std::invalid_argument(message);
}

void validate_binding(const ProtocolIIIFieldMulPartyMaterial& material,
                      std::uint8_t expected_party, std::uint64_t expected_session,
                      std::uint64_t expected_fingerprint, std::uint32_t expected_slot) {
  require(expected_party <= 1U && material.party == expected_party &&
              material.session == expected_session &&
              material.fingerprint == expected_fingerprint &&
              material.slot == expected_slot,
          "Protocol III field multiplication material binding");
}

void append_word(std::vector<std::uint8_t>& out, std::uint64_t value, std::size_t bytes) {
  for (std::size_t index = bytes; index > 0U; --index) {
    out.push_back(static_cast<std::uint8_t>(value >> (8U * (index - 1U))));
  }
}

std::uint64_t read_word(const std::vector<std::uint8_t>& bytes,
                        std::size_t& cursor, std::size_t count) {
  require(cursor <= bytes.size() && bytes.size() - cursor >= count,
          "Protocol III field multiplication material truncated");
  std::uint64_t value = 0;
  for (std::size_t index = 0; index < count; ++index) {
    value = (value << 8U) | bytes[cursor++];
  }
  return value;
}

ProtocolIIIField read_field(const std::vector<std::uint8_t>& bytes,
                            std::size_t& cursor) {
  require(cursor <= bytes.size() && bytes.size() - cursor >= 16U,
          "Protocol III field multiplication share truncated");
  std::vector<std::uint8_t> encoding(bytes.begin() + cursor, bytes.begin() + cursor + 16U);
  cursor += 16U;
  return ProtocolIIIField::deserialize(encoding);
}
}  // namespace

std::vector<std::uint8_t> ProtocolIIIFieldMulPartyMaterial::serialize() const {
  require(!started && !consumed, "Protocol III field multiplication material not fresh");
  require(session != 0U && fingerprint != 0U && party <= 1U,
          "Protocol III field multiplication material metadata");
  std::vector<std::uint8_t> out;
  out.reserve(kWireBytes);
  out.insert(out.end(), {'M', '5', 'F', 'M', 1U, party, 0U, 0U});
  append_word(out, session, 8U);
  append_word(out, fingerprint, 8U);
  append_word(out, slot, 4U);
  for (const auto share : {a_share, b_share, c_share}) {
    const auto encoded = share.serialize();
    out.insert(out.end(), encoded.begin(), encoded.end());
  }
  return out;
}

ProtocolIIIFieldMulPartyMaterial ProtocolIIIFieldMulPartyMaterial::deserialize(
    const std::vector<std::uint8_t>& bytes) {
  require(bytes.size() == kWireBytes, "Protocol III field multiplication material length");
  require(bytes[0] == 'M' && bytes[1] == '5' && bytes[2] == 'F' && bytes[3] == 'M' &&
              bytes[4] == 1U && bytes[5] <= 1U && bytes[6] == 0U && bytes[7] == 0U,
          "Protocol III field multiplication material tag/version/party");
  std::size_t cursor = 8U;
  ProtocolIIIFieldMulPartyMaterial result;
  result.party = bytes[5];
  result.session = read_word(bytes, cursor, 8U);
  result.fingerprint = read_word(bytes, cursor, 8U);
  result.slot = static_cast<std::uint32_t>(read_word(bytes, cursor, 4U));
  result.a_share = read_field(bytes, cursor);
  result.b_share = read_field(bytes, cursor);
  result.c_share = read_field(bytes, cursor);
  require(result.session != 0U && result.fingerprint != 0U,
          "Protocol III field multiplication material binding");
  return result;
}

std::pair<ProtocolIIIFieldMulPartyMaterial, ProtocolIIIFieldMulPartyMaterial>
protocol_iii_field_mul_preprocess(
    std::uint64_t session, std::uint64_t fingerprint,
    std::uint32_t slot, osuCrypto::PRNG& dealer_generator) {
  require(session != 0U && fingerprint != 0U,
          "Protocol III field multiplication dealer binding");
  const auto a = protocol_iii_sample_field_element(dealer_generator);
  const auto b = protocol_iii_sample_field_element(dealer_generator);
  const auto c = ProtocolIIIField::mul(a, b);
  const auto a_shares = protocol_iii_split_field_element(a, dealer_generator);
  const auto b_shares = protocol_iii_split_field_element(b, dealer_generator);
  const auto c_shares = protocol_iii_split_field_element(c, dealer_generator);
  return {
      ProtocolIIIFieldMulPartyMaterial{session, fingerprint, slot, 0U,
                                       a_shares.first, b_shares.first, c_shares.first},
      ProtocolIIIFieldMulPartyMaterial{session, fingerprint, slot, 1U,
                                       a_shares.second, b_shares.second, c_shares.second}};
}

ProtocolIIIFieldMulOpeningShare protocol_iii_field_mul_start(
    ProtocolIIIFieldMulPartyMaterial& material,
    std::uint8_t expected_party, std::uint64_t expected_session,
    std::uint64_t expected_fingerprint, std::uint32_t expected_slot,
    ProtocolIIIField payload_share, ProtocolIIIField mask_share) {
  validate_binding(material, expected_party, expected_session,
                   expected_fingerprint, expected_slot);
  require(!material.started && !material.consumed,
          "Protocol III field multiplication material already started");
  material.started = true;
  return {ProtocolIIIField::sub(payload_share, material.a_share),
          ProtocolIIIField::sub(mask_share, material.b_share)};
}

ProtocolIIIField protocol_iii_field_mul_finish(
    ProtocolIIIFieldMulPartyMaterial& material,
    std::uint8_t expected_party, std::uint64_t expected_session,
    std::uint64_t expected_fingerprint, std::uint32_t expected_slot,
    ProtocolIIIField public_d, ProtocolIIIField public_e) {
  validate_binding(material, expected_party, expected_session,
                   expected_fingerprint, expected_slot);
  require(material.started && !material.consumed,
          "Protocol III field multiplication material lifecycle");
  material.consumed = true;
  auto result = material.c_share;
  result = ProtocolIIIField::add(
      result, ProtocolIIIField::mul(public_d, material.b_share));
  result = ProtocolIIIField::add(
      result, ProtocolIIIField::mul(public_e, material.a_share));
  if (material.party == 0U) {
    result = ProtocolIIIField::add(result, ProtocolIIIField::mul(public_d, public_e));
  }
  return result;
}

}  // namespace moe_topk
