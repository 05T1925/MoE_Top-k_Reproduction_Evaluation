#pragma once

#include <FSS/dpf.h>
#include <FSS/keypack.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace moe_topk {

// Exact native FSS key tail used by the current M3/M5 modular offline
// bundles. The FSS MemBuf reader has no bound, so callers must compare the
// remaining byte count before invoking recv_dpf_keypack/recv_mult_key.
inline std::size_t protocol_iii_offline_key_tail_bytes(
    std::uint32_t logical_n,
    std::uint32_t k,
    std::uint8_t rank_bits) {
  if (logical_n == 0U || k == 0U || k > logical_n ||
      rank_bits == 0U || rank_bits >= 64U) {
    throw std::invalid_argument("Protocol III offline key-tail shape");
  }
  const std::size_t point_bytes =
      rank_bits <= 8U ? 1U :
      rank_bits <= 16U ? 2U :
      rank_bits <= 32U ? 4U : 8U;
  const std::size_t dpf_bytes =
      (static_cast<std::size_t>(rank_bits) + 1U) *
          sizeof(osuCrypto::block) +
      2U * point_bytes + 2U * sizeof(std::uint64_t);
  const std::size_t mul_bytes =
      sizeof(MultKey) + sizeof(std::uint64_t);
  const auto n = static_cast<std::size_t>(logical_n);
  if (n > std::numeric_limits<std::size_t>::max() /
              static_cast<std::size_t>(k)) {
    throw std::overflow_error("Protocol III offline cell count");
  }
  const auto cells = n * static_cast<std::size_t>(k);
  if (n > std::numeric_limits<std::size_t>::max() / dpf_bytes ||
      cells > std::numeric_limits<std::size_t>::max() / mul_bytes) {
    throw std::overflow_error("Protocol III offline key-tail size");
  }
  const auto dpf_total = n * dpf_bytes;
  const auto mul_total = cells * mul_bytes;
  if (mul_total > std::numeric_limits<std::size_t>::max() - dpf_total) {
    throw std::overflow_error("Protocol III offline key-tail sum");
  }
  return dpf_total + mul_total;
}

}  // namespace moe_topk
