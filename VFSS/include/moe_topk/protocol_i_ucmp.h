#pragma once
#include <FSS/dcf.h>
#include <cstdint>
#include <utility>
#include <vector>
namespace moe_topk {
class ProtocolIUcmpPartyMaterial { public: ProtocolIUcmpPartyMaterial()=default; ProtocolIUcmpPartyMaterial(const ProtocolIUcmpPartyMaterial&)=delete; ProtocolIUcmpPartyMaterial& operator=(const ProtocolIUcmpPartyMaterial&)=delete; ProtocolIUcmpPartyMaterial(ProtocolIUcmpPartyMaterial&&)=default; ProtocolIUcmpPartyMaterial& operator=(ProtocolIUcmpPartyMaterial&&)=default; std::uint64_t eval_strict_lt(std::uint64_t,std::uint64_t); int party_id()const{return party_;} int comparison_bits()const{return bits_;} std::vector<std::uint8_t> serialize()const; static ProtocolIUcmpPartyMaterial deserialize(const std::vector<std::uint8_t>&); private: friend class ProtocolIUcmpMaterial; ProtocolIUcmpPartyMaterial(int,const DCFKeyPack&);int party_=-1,bits_=0;bool used_=false;std::vector<osuCrypto::block>k_;std::vector<GroupElement>g_,v_;};
// C: primitive conformance adapter; not a Protocol I runtime API.
class ProtocolIUcmpMaterial {
 public:
  ProtocolIUcmpMaterial(int bits, std::uint64_t mask_left, std::uint64_t mask_right);
  ~ProtocolIUcmpMaterial();
  ProtocolIUcmpMaterial(const ProtocolIUcmpMaterial&) = delete;
  ProtocolIUcmpMaterial& operator=(const ProtocolIUcmpMaterial&) = delete;
  ProtocolIUcmpMaterial(ProtocolIUcmpMaterial&& other) noexcept;
  ProtocolIUcmpMaterial& operator=(ProtocolIUcmpMaterial&& other) noexcept;
  std::uint64_t eval_strict_lt(int party, std::uint64_t z_left, std::uint64_t z_right);
  ProtocolIUcmpPartyMaterial export_party_material(int party) const;
  int comparison_bits() const noexcept { return bits_; }
 private:
  int bits_ = 0; std::uint64_t ring_mask_ = 0, alpha_ = 0; DCFKeyPack keys_[2]; bool live_ = false; bool used_[2] = {false,false};
};
}
