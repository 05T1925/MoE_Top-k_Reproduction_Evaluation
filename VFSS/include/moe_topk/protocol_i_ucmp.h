#pragma once
#include <FSS/dcf.h>
#include <cstdint>
#include <utility>
namespace moe_topk {
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
 private:
  int bits_ = 0; std::uint64_t ring_mask_ = 0, alpha_ = 0; DCFKeyPack keys_[2]; bool live_ = false; bool used_[2] = {false,false};
};
}
