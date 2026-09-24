#include <moe_topk/protocol_iii_field_dpf.h>

#include <moe_topk/protocol_iii_field_payload.h>

#include <FSS/dpf.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Crypto/AES.h>

#include <immintrin.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace moe_topk {
namespace {
using Wide = ProtocolIIIField::Storage;
using osuCrypto::block;

constexpr std::uint8_t kVersion = 1;
constexpr std::size_t kHeaderBytes = 4U + 1U + 1U + 1U + 1U + 8U + 8U + 4U + 16U + 8U + 8U;
constexpr std::uint64_t kLeafDomain = UINT64_C(0x4d35444650484f55);

void require(bool condition, const char* message) {
  if (!condition) throw std::invalid_argument(message);
}

std::uint64_t domain_size(std::uint8_t bits) {
  require(bits >= 1U && bits <= 63U, "Protocol III field DPF domain width");
  return UINT64_C(1) << bits;
}

std::uint8_t lsb(block value) noexcept {
  return static_cast<std::uint8_t>(_mm_cvtsi128_si64x(value) & 1);
}

Wide bits_to_wide(block value) noexcept {
  return (Wide{static_cast<std::uint64_t>(_mm_extract_epi64(value, 1))} << 64U) |
         static_cast<std::uint64_t>(_mm_extract_epi64(value, 0));
}

// Full-width field leaf with deterministic rejection; both parties use the
// same mapping whenever their DPF tree states agree off the point.
ProtocolIIIField field_leaf(block seed) {
  const osuCrypto::AES aes(seed);
  for (std::uint64_t counter = 0;; ++counter) {
    const auto candidate = bits_to_wide(
        aes.ecbEncBlock(osuCrypto::toBlock(kLeafDomain, counter)));
    if (candidate < ProtocolIIIField::modulus()) {
      return ProtocolIIIField::from_canonical(candidate);
    }
  }
}

std::pair<ProtocolIIIField, std::uint8_t> descend(
    const DPFKeyPack& key, std::uint8_t party, std::uint64_t input) {
  require(key.s != nullptr && key.bout == 64, "Protocol III field DPF tree key");
  const auto bits = static_cast<std::uint8_t>(key.bin);
  require(input < domain_size(bits), "Protocol III field DPF input outside domain");
  static const block not_one_block = osuCrypto::toBlock(~UINT64_C(0), ~UINT64_C(1));
  block state = _mm_loadu_si128(key.s);
  std::uint8_t t = party;
  for (std::uint8_t level = 0; level < bits; ++level) {
    const auto direction = static_cast<std::uint8_t>(
        (input >> (bits - 1U - level)) & 1U);
    const auto cipher = osuCrypto::AES(state).ecbEncBlock(
        osuCrypto::toBlock(0, direction));
    state = cipher & not_one_block;
    const auto old_t = t;
    t = lsb(cipher);
    if (old_t != 0U) {
      state = state ^ _mm_loadu_si128(key.s + level + 1U);
      t ^= static_cast<std::uint8_t>(
          (key.tcw[direction] >> (bits - 1U - level)) & 1U);
    }
  }
  return {field_leaf(state), t};
}

void append_word(std::vector<std::uint8_t>& out, std::uint64_t word, std::size_t bytes) {
  for (std::size_t index = bytes; index > 0U; --index) {
    out.push_back(static_cast<std::uint8_t>(word >> ((index - 1U) * 8U)));
  }
}

std::uint64_t read_word(const std::vector<std::uint8_t>& in,
                        std::size_t& cursor, std::size_t bytes) {
  require(cursor <= in.size() && in.size() - cursor >= bytes,
          "Protocol III field DPF key truncated");
  std::uint64_t word = 0;
  for (std::size_t index = 0; index < bytes; ++index) {
    word = (word << 8U) | in[cursor++];
  }
  return word;
}

std::vector<std::uint8_t> read_field_bytes(
    const std::vector<std::uint8_t>& in, std::size_t& cursor) {
  require(cursor <= in.size() && in.size() - cursor >= 16U,
          "Protocol III field DPF correction truncated");
  std::vector<std::uint8_t> value(in.begin() + cursor, in.begin() + cursor + 16U);
  cursor += 16U;
  return value;
}
}  // namespace

ProtocolIIIFieldDpfPartyKey::ProtocolIIIFieldDpfPartyKey(
    ProtocolIIIDpfRoutingKey&& tree_key,
    ProtocolIIIField field_correction,
    std::uint8_t party,
    std::uint64_t session,
    std::uint64_t fingerprint,
    std::uint32_t slot)
    : tree_key_(std::move(tree_key)), correction_(field_correction),
      party_(party), session_(session), fingerprint_(fingerprint), slot_(slot) {
  require(party <= 1U, "Protocol III field DPF party");
  require(session != 0U && fingerprint != 0U, "Protocol III field DPF binding");
  const auto& native = tree_key_.native_key();
  (void)domain_size(static_cast<std::uint8_t>(native.bin));
  require(native.s != nullptr && native.bout == 64 && native.payload == 0U,
          "Protocol III field DPF native tree layout");
}

std::vector<std::uint8_t> ProtocolIIIFieldDpfPartyKey::serialize() const {
  const auto& native = tree_key_.native_key();
  require(native.s != nullptr && native.bin >= 1 && native.bin <= 63 &&
              native.bout == 64 && native.payload == 0U,
          "Protocol III field DPF key is empty or moved-from");
  std::vector<std::uint8_t> out;
  out.reserve(kHeaderBytes + (static_cast<std::size_t>(native.bin) + 1U) * 16U);
  out.insert(out.end(), {'M', '5', 'F', 'D'});
  out.push_back(kVersion);
  out.push_back(party_);
  out.push_back(static_cast<std::uint8_t>(native.bin));
  out.push_back(0U);
  append_word(out, session_, 8U);
  append_word(out, fingerprint_, 8U);
  append_word(out, slot_, 4U);
  const auto correction_bytes = correction_.serialize();
  out.insert(out.end(), correction_bytes.begin(), correction_bytes.end());
  append_word(out, native.tLcw, 8U);
  append_word(out, native.tRcw, 8U);
  for (int level = 0; level <= native.bin; ++level) {
    const block seed = _mm_loadu_si128(native.s + level);
    append_word(out, static_cast<std::uint64_t>(_mm_extract_epi64(seed, 1)), 8U);
    append_word(out, static_cast<std::uint64_t>(_mm_extract_epi64(seed, 0)), 8U);
  }
  return out;
}

ProtocolIIIFieldDpfPartyKey ProtocolIIIFieldDpfPartyKey::deserialize(
    const std::vector<std::uint8_t>& bytes) {
  require(bytes.size() >= kHeaderBytes, "Protocol III field DPF key truncated");
  require(bytes[0] == 'M' && bytes[1] == '5' && bytes[2] == 'F' && bytes[3] == 'D',
          "Protocol III field DPF key tag");
  require(bytes[4] == kVersion && bytes[7] == 0U,
          "Protocol III field DPF key version/reserved");
  const auto party = bytes[5];
  const auto bits = bytes[6];
  (void)domain_size(bits);
  require(party <= 1U, "Protocol III field DPF key party");
  require(bytes.size() == kHeaderBytes + (static_cast<std::size_t>(bits) + 1U) * 16U,
          "Protocol III field DPF key length");
  std::size_t cursor = 8U;
  const auto session = read_word(bytes, cursor, 8U);
  const auto fingerprint = read_word(bytes, cursor, 8U);
  const auto slot = read_word(bytes, cursor, 4U);
  const auto correction = ProtocolIIIField::deserialize(read_field_bytes(bytes, cursor));
  DPFKeyPack native(bits, 64);
  ProtocolIIIDpfRoutingKey owner(std::move(native));
  auto& key = owner.native_key();
  key.tLcw = read_word(bytes, cursor, 8U);
  key.tRcw = read_word(bytes, cursor, 8U);
  require(key.tLcw < domain_size(bits) && key.tRcw < domain_size(bits),
          "Protocol III field DPF correction bits");
  key.payload = 0U;
  for (std::uint8_t level = 0; level <= bits; ++level) {
    const auto high = read_word(bytes, cursor, 8U);
    const auto low = read_word(bytes, cursor, 8U);
    require((low & 1U) == 0U, "Protocol III field DPF seed control bit");
    key.s[level] = osuCrypto::toBlock(high, low);
  }
  return ProtocolIIIFieldDpfPartyKey(
      std::move(owner), correction, party, session, fingerprint,
      static_cast<std::uint32_t>(slot));
}

std::pair<ProtocolIIIFieldDpfPartyKey, ProtocolIIIFieldDpfPartyKey>
protocol_iii_field_dpf_generate(
    std::uint8_t domain_bits, std::uint64_t alpha, ProtocolIIIField beta,
    std::uint64_t session, std::uint64_t fingerprint, std::uint32_t slot) {
  require(alpha < domain_size(domain_bits), "Protocol III field DPF point outside domain");
  require(session != 0U && fingerprint != 0U, "Protocol III field DPF binding");
  auto native = keyGenDPF(domain_bits, 64, alpha, 0);
  ProtocolIIIDpfRoutingKey first(std::move(native.first));
  ProtocolIIIDpfRoutingKey second(std::move(native.second));
  const auto [leaf0, t0] = descend(first.native_key(), 0U, alpha);
  const auto [leaf1, t1] = descend(second.native_key(), 1U, alpha);
  require(t0 != t1, "Protocol III field DPF tree point control invariant");
  const auto delta = ProtocolIIIField::sub(leaf0, leaf1);
  const auto correction = t0 == 1U
      ? ProtocolIIIField::sub(beta, delta)
      : ProtocolIIIField::sub(delta, beta);
  first.native_key().payload = 0U;
  second.native_key().payload = 0U;
  return {
      ProtocolIIIFieldDpfPartyKey(std::move(first), correction, 0U,
                                  session, fingerprint, slot),
      ProtocolIIIFieldDpfPartyKey(std::move(second), correction, 1U,
                                  session, fingerprint, slot)};
}

ProtocolIIIField protocol_iii_field_dpf_eval(
    const ProtocolIIIFieldDpfPartyKey& key, std::uint8_t expected_party,
    std::uint64_t expected_session, std::uint64_t expected_fingerprint,
    std::uint32_t expected_slot, std::uint64_t input) {
  require(key.party_ == expected_party && key.session_ == expected_session &&
              key.fingerprint_ == expected_fingerprint && key.slot_ == expected_slot,
          "Protocol III field DPF party/session/slot mismatch");
  const auto [leaf, t] = descend(key.tree_key_.native_key(), key.party_, input);
  const auto local = t == 0U ? leaf : ProtocolIIIField::add(leaf, key.correction_);
  return key.party_ == 0U ? local : ProtocolIIIField::sub(ProtocolIIIField{}, local);
}

std::pair<ProtocolIIIFieldPayloadPartyMaterial,
          ProtocolIIIFieldPayloadPartyMaterial>
protocol_iii_field_payload_preprocess(
    std::uint8_t domain_bits, std::uint64_t rank_mask_alpha,
    ProtocolIIIField nonzero_payload_mask, std::uint64_t session,
    std::uint64_t fingerprint, std::uint32_t slot,
    osuCrypto::PRNG& dealer_generator) {
  require(!nonzero_payload_mask.is_zero(), "Protocol III field payload mask is zero");
  auto keys = protocol_iii_field_dpf_generate(
      domain_bits, rank_mask_alpha, ProtocolIIIField::inv(nonzero_payload_mask),
      session, fingerprint, slot);
  auto shares = protocol_iii_split_field_element(nonzero_payload_mask, dealer_generator);
  return {
      ProtocolIIIFieldPayloadPartyMaterial{shares.first, std::move(keys.first)},
      ProtocolIIIFieldPayloadPartyMaterial{shares.second, std::move(keys.second)}};
}

}  // namespace moe_topk
