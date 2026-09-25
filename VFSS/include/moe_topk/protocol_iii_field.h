#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace moe_topk {

// Project-chosen H = F_(2^127-1). This is separate from the rank ring and
// from the frozen Z_(2^64) Protocol III modular baseline.
class ProtocolIIIField {
 public:
  using Storage = unsigned __int128;

  ProtocolIIIField() = default;

  static constexpr Storage modulus() noexcept {
    return (Storage{1} << 127U) - 1U;
  }

  static ProtocolIIIField from_canonical(Storage value);
  static ProtocolIIIField from_u64(std::uint64_t value) noexcept;

  [[nodiscard]] Storage value() const noexcept { return value_; }
  [[nodiscard]] bool is_zero() const noexcept { return value_ == 0; }

  // Exactly 16 bytes, most significant byte first. Decode rejects x >= p.
  [[nodiscard]] std::vector<std::uint8_t> serialize() const;
  static ProtocolIIIField deserialize(const std::vector<std::uint8_t>& bytes);

  [[nodiscard]] static ProtocolIIIField add(
      ProtocolIIIField left, ProtocolIIIField right) noexcept;
  [[nodiscard]] static ProtocolIIIField sub(
      ProtocolIIIField left, ProtocolIIIField right) noexcept;
  [[nodiscard]] static ProtocolIIIField mul(
      ProtocolIIIField left, ProtocolIIIField right) noexcept;
  [[nodiscard]] static ProtocolIIIField inv(ProtocolIIIField value);

  [[nodiscard]] bool operator==(ProtocolIIIField other) const noexcept {
    return value_ == other.value_;
  }
  [[nodiscard]] bool operator!=(ProtocolIIIField other) const noexcept {
    return value_ != other.value_;
  }

 private:
  explicit ProtocolIIIField(Storage value) noexcept : value_(value) {}
  Storage value_ = 0;
};

}  // namespace moe_topk
