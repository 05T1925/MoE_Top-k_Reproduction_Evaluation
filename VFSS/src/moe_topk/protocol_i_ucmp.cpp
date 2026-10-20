#include <moe_topk/protocol_i_ucmp.h>

#include <FSS/freekey.h>

#include <cstring>
#include <stdexcept>

namespace moe_topk {
namespace {

void valid(int bits) {
  if (bits < 34 || bits > 53)
    throw std::invalid_argument("comparison_bits must be 34..53");
}

std::uint64_t get_u64(const std::vector<std::uint8_t>& input,
                      std::size_t& offset) {
  if (offset > input.size() || input.size() - offset < 8)
    throw std::invalid_argument("material truncated");
  std::uint64_t value = 0;
  for (int i = 0; i < 8; ++i) value = (value << 8U) | input[offset++];
  return value;
}

}  // namespace

ProtocolIUcmpMaterial::ProtocolIUcmpMaterial(
    int bits, std::uint64_t mask_left, std::uint64_t mask_right)
    : ProtocolIUcmpMaterial(bits, mask_left, mask_right,
                            ProtocolIDcfAbi::LegacyM2UcV1) {}

ProtocolIUcmpMaterial::ProtocolIUcmpMaterial(
    int bits, std::uint64_t mask_left, std::uint64_t mask_right,
    ProtocolIDcfAbi abi)
    : bits_(bits), abi_(abi) {
  valid(bits);
  ring_mask_ = (UINT64_C(1) << bits) - 1;
  if ((mask_left & ~ring_mask_) != 0 || (mask_right & ~ring_mask_) != 0)
    throw std::invalid_argument("mask outside ring");
  alpha_ = (mask_left - mask_right) & ring_mask_;
  if (abi_ == ProtocolIDcfAbi::LegacyM2UcV1) {
    auto pair = keyGenDCF(bits, 64, alpha_, 1);
    keys_[0] = pair.first;
    keys_[1] = pair.second;
    live_ = true;
    return;
  }
#if defined(MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL)
  if (abi_ != ProtocolIDcfAbi::Bcg21Fig1Aes128V1)
    throw std::invalid_argument("unsupported uCMP DCF ABI");
  auto pair = bcg21_fig1_aes128_v1::keygen(
      static_cast<std::uint8_t>(bits), alpha_, 1);
  bcg_keys_[0].reset(new bcg21_fig1_aes128_v1::PartyKey(
      std::move(pair.party0)));
  bcg_keys_[1].reset(new bcg21_fig1_aes128_v1::PartyKey(
      std::move(pair.party1)));
  live_ = true;
#else
  throw std::invalid_argument(
      "BCG21 DCF ABI is unavailable without the Select5 experimental build");
#endif
}

ProtocolIUcmpMaterial::~ProtocolIUcmpMaterial() {
  if (live_ && abi_ == ProtocolIDcfAbi::LegacyM2UcV1) {
    auto pair = std::make_pair(keys_[0], keys_[1]);
    freeDCFKeyPackPair(pair);
  }
}

ProtocolIUcmpMaterial::ProtocolIUcmpMaterial(ProtocolIUcmpMaterial&& other) noexcept {
  *this = std::move(other);
}

ProtocolIUcmpMaterial& ProtocolIUcmpMaterial::operator=(
    ProtocolIUcmpMaterial&& other) noexcept {
  if (this != &other) {
    if (live_ && abi_ == ProtocolIDcfAbi::LegacyM2UcV1) {
      auto pair = std::make_pair(keys_[0], keys_[1]);
      freeDCFKeyPackPair(pair);
    }
    bits_ = other.bits_;
    ring_mask_ = other.ring_mask_;
    alpha_ = other.alpha_;
    keys_[0] = other.keys_[0];
    keys_[1] = other.keys_[1];
    live_ = other.live_;
    used_[0] = other.used_[0];
    used_[1] = other.used_[1];
    abi_ = other.abi_;
#if defined(MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL)
    bcg_keys_[0] = std::move(other.bcg_keys_[0]);
    bcg_keys_[1] = std::move(other.bcg_keys_[1]);
#endif
    other.live_ = false;
  }
  return *this;
}

std::uint64_t ProtocolIUcmpMaterial::eval_strict_lt(
    int party, std::uint64_t z_left, std::uint64_t z_right) {
  if (!live_ || party < 0 || party > 1 || used_[party])
    throw std::invalid_argument("invalid or consumed uCMP material");
  if ((z_left & ~ring_mask_) != 0 || (z_right & ~ring_mask_) != 0)
    throw std::invalid_argument("masked operand outside ring");
  used_[party] = true;
  const auto x = (z_left - z_right) & ring_mask_;
  const auto half = UINT64_C(1) << (bits_ - 1);
  const auto y = (x - half) & ring_mask_;
  std::uint64_t t1 = 0, t2 = 0;
  if (abi_ == ProtocolIDcfAbi::LegacyM2UcV1) {
    evalDCF(party, &t1, x, keys_[party]);
    evalDCF(party, &t2, y, keys_[party]);
  }
#if defined(MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL)
  else {
    t1 = bcg21_fig1_aes128_v1::eval_share(*bcg_keys_[party], x);
    t2 = bcg21_fig1_aes128_v1::eval_share(*bcg_keys_[party], y);
  }
#else
  else {
    throw std::invalid_argument(
        "BCG21 DCF ABI is unavailable without the Select5 experimental build");
  }
#endif
  auto ge = t2 - t1;
  if (party == 1 && y >= half) ++ge;
  return party == 0 ? UINT64_C(1) - ge : UINT64_C(0) - ge;
}

ProtocolIUcmpPartyMaterial ProtocolIUcmpMaterial::export_party_material(
    int party) const {
  if (!live_ || party < 0 || party > 1)
    throw std::invalid_argument("invalid party");
#if defined(MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL)
  if (abi_ == ProtocolIDcfAbi::Bcg21Fig1Aes128V1)
    return ProtocolIUcmpPartyMaterial(
        bcg21_fig1_aes128_v1::PartyKey(*bcg_keys_[party]));
#endif
  return ProtocolIUcmpPartyMaterial(party, keys_[party]);
}

ProtocolIUcmpPartyMaterial::ProtocolIUcmpPartyMaterial(
    int party, const DCFKeyPack& key)
    : party_(party), bits_(key.Bin), abi_(ProtocolIDcfAbi::LegacyM2UcV1),
      k_(key.k, key.k + key.Bin + 1),
      g_(key.g, key.g + key.groupSize),
      v_(key.v, key.v + key.Bin * key.groupSize) {
  if (party < 0 || party > 1 || bits_ < 34 || bits_ > 53 ||
      key.Bout != 64 || key.groupSize != 1)
    throw std::invalid_argument("invalid legacy party material");
}

#if defined(MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL)
ProtocolIUcmpPartyMaterial::ProtocolIUcmpPartyMaterial(
    bcg21_fig1_aes128_v1::PartyKey&& key)
    : party_(key.party), bits_(key.input_bits),
      abi_(ProtocolIDcfAbi::Bcg21Fig1Aes128V1),
      bcg_key_(new bcg21_fig1_aes128_v1::PartyKey(std::move(key))) {
  if (party_ < 0 || party_ > 1 || bits_ < 34 || bits_ > 53)
    throw std::invalid_argument("invalid BCG21 party material");
}
#endif

std::uint64_t ProtocolIUcmpPartyMaterial::eval_strict_lt(
    std::uint64_t z_left, std::uint64_t z_right) {
  if (party_ < 0 || used_)
    throw std::invalid_argument("consumed party material");
  const auto ring_mask = (UINT64_C(1) << bits_) - 1;
  if ((z_left & ~ring_mask) != 0 || (z_right & ~ring_mask) != 0)
    throw std::invalid_argument("operand outside ring");
  used_ = true;
  const auto x = (z_left - z_right) & ring_mask;
  const auto half = UINT64_C(1) << (bits_ - 1);
  const auto y = (x - half) & ring_mask;
  std::uint64_t t1 = 0, t2 = 0;
  if (abi_ == ProtocolIDcfAbi::LegacyM2UcV1) {
    DCFKeyPack key(bits_, 64, 1, k_.data(), g_.data(), v_.data());
    evalDCF(party_, &t1, x, key);
    evalDCF(party_, &t2, y, key);
  }
#if defined(MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL)
  else {
    if (bcg_key_ == nullptr) throw std::invalid_argument("missing BCG21 key");
    t1 = bcg21_fig1_aes128_v1::eval_share(*bcg_key_, x);
    t2 = bcg21_fig1_aes128_v1::eval_share(*bcg_key_, y);
  }
#else
  else {
    throw std::invalid_argument(
        "BCG21 DCF ABI is unavailable without the Select5 experimental build");
  }
#endif
  auto ge = t2 - t1;
  if (party_ == 1 && y >= half) ++ge;
  return party_ == 0 ? UINT64_C(1) - ge : UINT64_C(0) - ge;
}

std::vector<std::uint8_t> ProtocolIUcmpPartyMaterial::serialize() const {
  if (party_ < 0 || used_)
    throw std::invalid_argument("material unavailable");
#if defined(MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL)
  if (abi_ == ProtocolIDcfAbi::Bcg21Fig1Aes128V1) {
    if (bcg_key_ == nullptr) throw std::invalid_argument("missing BCG21 key");
    return bcg21_fig1_aes128_v1::serialize(*bcg_key_);
  }
#endif
  std::vector<std::uint8_t> output = {
      'M','2','U','C',1,static_cast<std::uint8_t>(party_),
      static_cast<std::uint8_t>(bits_),64,1};
  auto put = [&](std::uint64_t value) {
    for (int i = 7; i >= 0; --i)
      output.push_back(static_cast<std::uint8_t>(value >> (i * 8)));
  };
  put(k_.size()); put(g_.size()); put(v_.size());
  for (const auto block : k_) {
    std::uint8_t bytes[16];
    std::memcpy(bytes, &block, sizeof(bytes));
    output.insert(output.end(), bytes, bytes + sizeof(bytes));
  }
  for (const auto value : g_) put(value);
  for (const auto value : v_) put(value);
  return output;
}

ProtocolIUcmpPartyMaterial ProtocolIUcmpPartyMaterial::deserialize(
    const std::vector<std::uint8_t>& input) {
  if (input.size() >= 4 && input[0] == 'M' && input[1] == '2' &&
      input[2] == 'B' && input[3] == '1') {
#if defined(MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL)
    return ProtocolIUcmpPartyMaterial(
        bcg21_fig1_aes128_v1::deserialize(input));
#else
    throw std::invalid_argument(
        "BCG21 DCF ABI is unavailable without the Select5 experimental build");
#endif
  }
  if (input.size() < 33 || input[0] != 'M' || input[1] != '2' ||
      input[2] != 'U' || input[3] != 'C' || input[4] != 1 ||
      input[7] != 64 || input[8] != 1)
    throw std::invalid_argument("material header");
  std::size_t offset = 9;
  const auto key_count = get_u64(input, offset);
  const auto group_count = get_u64(input, offset);
  const auto value_count = get_u64(input, offset);
  const int party = input[5], bits = input[6];
  if (party > 1 || bits < 34 || bits > 53 ||
      key_count != static_cast<std::uint64_t>(bits + 1) || group_count != 1 ||
      value_count != static_cast<std::uint64_t>(bits) ||
      input.size() != offset + 16U * key_count + 8U * group_count +
                          8U * value_count)
    throw std::invalid_argument("material length");
  ProtocolIUcmpPartyMaterial result;
  result.party_ = party;
  result.bits_ = bits;
  result.abi_ = ProtocolIDcfAbi::LegacyM2UcV1;
  result.k_.resize(key_count);
  result.g_.resize(group_count);
  result.v_.resize(value_count);
  for (auto& block : result.k_) {
    std::memcpy(&block, input.data() + offset, 16);
    offset += 16;
  }
  for (auto& value : result.g_) value = get_u64(input, offset);
  for (auto& value : result.v_) value = get_u64(input, offset);
  return result;
}

}  // namespace moe_topk
