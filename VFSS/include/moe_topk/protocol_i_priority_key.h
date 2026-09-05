#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace moe_topk {

// C: project priority encoding, not an Agarwal paper input encoding.
struct ProtocolIPriorityKey { std::uint64_t value; std::uint32_t key_bits; };

inline std::uint32_t protocol_i_index_bits(std::size_t n) {
    if (n == 0 || n > 1000000) throw std::invalid_argument("Protocol I requires 1 <= n <= 1000000");
    std::uint32_t bits = 0;
    for (std::size_t v = n - 1; v != 0; v >>= 1) ++bits;
    return bits == 0 ? 1 : bits;
}

inline ProtocolIPriorityKey protocol_i_priority_key(std::uint32_t raw_score,
                                                     std::size_t original_index,
                                                     std::size_t n) {
    const auto index_bits = protocol_i_index_bits(n);
    if (original_index >= n) throw std::invalid_argument("Protocol I original_index is out of range");
    const auto ordered = raw_score ^ UINT32_C(0x80000000);
    const auto high = UINT32_MAX - ordered;
    return {(static_cast<std::uint64_t>(high) << index_bits) | original_index,
            static_cast<std::uint32_t>(32 + index_bits)};
}

}  // namespace moe_topk
