#pragma once

#if defined(MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL)
#include <moe_topk/dcf_bcg21_fig1_aes128_v1.h>
#endif

#include <FSS/dcf.h>

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace moe_topk {

enum class ProtocolIDcfAbi : std::uint8_t {
  LegacyM2UcV1 = 0,
#if defined(MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL)
  Bcg21Fig1Aes128V1 = 1,
#endif
};

class ProtocolIUcmpPartyMaterial {
 public:
  ProtocolIUcmpPartyMaterial() = default;
  ProtocolIUcmpPartyMaterial(const ProtocolIUcmpPartyMaterial&) = delete;
  ProtocolIUcmpPartyMaterial& operator=(const ProtocolIUcmpPartyMaterial&) = delete;
  ProtocolIUcmpPartyMaterial(ProtocolIUcmpPartyMaterial&&) = default;
  ProtocolIUcmpPartyMaterial& operator=(ProtocolIUcmpPartyMaterial&&) = default;

  std::uint64_t eval_strict_lt(std::uint64_t z_left, std::uint64_t z_right);
  int party_id() const { return party_; }
  int comparison_bits() const { return bits_; }
  ProtocolIDcfAbi dcf_abi() const { return abi_; }
  std::vector<std::uint8_t> serialize() const;
  static ProtocolIUcmpPartyMaterial deserialize(
      const std::vector<std::uint8_t>& input);

 private:
  friend class ProtocolIUcmpMaterial;
  ProtocolIUcmpPartyMaterial(int party, const DCFKeyPack& key);
#if defined(MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL)
  explicit ProtocolIUcmpPartyMaterial(
      bcg21_fig1_aes128_v1::PartyKey&& key);
#endif

  int party_ = -1;
  int bits_ = 0;
  bool used_ = false;
  ProtocolIDcfAbi abi_ = ProtocolIDcfAbi::LegacyM2UcV1;
  std::vector<osuCrypto::block> k_;
  std::vector<GroupElement> g_, v_;
#if defined(MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL)
  std::unique_ptr<bcg21_fig1_aes128_v1::PartyKey> bcg_key_;
#endif
};

// Primitive adapter used by conformance tests and the experimental runtime.
class ProtocolIUcmpMaterial {
 public:
  ProtocolIUcmpMaterial(int bits, std::uint64_t mask_left,
                        std::uint64_t mask_right);
  ProtocolIUcmpMaterial(int bits, std::uint64_t mask_left,
                        std::uint64_t mask_right, ProtocolIDcfAbi abi);
  ~ProtocolIUcmpMaterial();
  ProtocolIUcmpMaterial(const ProtocolIUcmpMaterial&) = delete;
  ProtocolIUcmpMaterial& operator=(const ProtocolIUcmpMaterial&) = delete;
  ProtocolIUcmpMaterial(ProtocolIUcmpMaterial&& other) noexcept;
  ProtocolIUcmpMaterial& operator=(ProtocolIUcmpMaterial&& other) noexcept;

  std::uint64_t eval_strict_lt(int party, std::uint64_t z_left,
                               std::uint64_t z_right);
  ProtocolIUcmpPartyMaterial export_party_material(int party) const;
  int comparison_bits() const noexcept { return bits_; }

 private:
  int bits_ = 0;
  std::uint64_t ring_mask_ = 0;
  std::uint64_t alpha_ = 0;
  DCFKeyPack keys_[2];
  bool live_ = false;
  bool used_[2] = {false, false};
  ProtocolIDcfAbi abi_ = ProtocolIDcfAbi::LegacyM2UcV1;
#if defined(MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL)
  std::unique_ptr<bcg21_fig1_aes128_v1::PartyKey> bcg_keys_[2];
#endif
};

}  // namespace moe_topk
