#pragma once

#include <moe_topk/protocol_iii_field.h>

#include <cryptoTools/Crypto/PRNG.h>

#include <cstdint>
#include <utility>

namespace moe_topk {

// M5-D project encoding: offset by one, preserving every uint64 payload.
[[nodiscard]] ProtocolIIIField protocol_iii_encode_payload_nonzero(std::uint64_t payload);
[[nodiscard]] std::uint64_t protocol_iii_decode_payload_nonzero(ProtocolIIIField encoded);

// Key and payload in one field element. The key may use 1..62 bits; no silent
// truncation or implicit conversion from the frozen ring-share representation.
[[nodiscard]] ProtocolIIIField protocol_iii_pack_key_payload_nonzero(
    std::uint64_t key, std::uint8_t key_bits, std::uint64_t payload);
[[nodiscard]] std::pair<std::uint64_t, std::uint64_t>
protocol_iii_unpack_key_payload_nonzero(
    ProtocolIIIField encoded, std::uint8_t key_bits);

// Dealer-side sampling. One online party receives only an additive share of s.
[[nodiscard]] ProtocolIIIField protocol_iii_sample_field_element(osuCrypto::PRNG& generator);
[[nodiscard]] ProtocolIIIField protocol_iii_sample_nonzero_field_element(osuCrypto::PRNG& generator);
[[nodiscard]] std::pair<ProtocolIIIField, ProtocolIIIField>
protocol_iii_split_field_element(ProtocolIIIField value, osuCrypto::PRNG& generator);

// Algebraic conformance helper for dealer/test controllers that know both
// operands. Online parties use field multiplication shares, not this helper.
[[nodiscard]] ProtocolIIIField protocol_iii_mask_field_payload(
    ProtocolIIIField encoded_nonzero, ProtocolIIIField nonzero_mask);

}  // namespace moe_topk
