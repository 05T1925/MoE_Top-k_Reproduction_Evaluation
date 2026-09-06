#include <moe_topk/protocol_i_opv.h>

#include <openssl/rand.h>
#include <openssl/sha.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace moe_topk {
namespace {

constexpr std::uint32_t kOpvProtocolId = UINT32_C(0x4f505631);  // OPV1
constexpr std::string_view kChildDomain = "M2-OPV-CHILD-v1";
constexpr std::string_view kLeafDomain = "M2-OPV-LEAF-v1";

[[noreturn]] void fail(const char* message) { throw std::runtime_error(message); }

bool power_of_two(std::uint32_t value) { return value >= 2 && (value & (value - 1)) == 0; }
std::uint32_t depth(std::uint32_t value) {
  std::uint32_t result = 0;
  while (value > 1) { ++result; value >>= 1; }
  return result;
}
void validate(const ProtocolIOpvConfig& config) {
  if (!power_of_two(config.vector_length) || config.batch_count == 0 || config.timeout_ms <= 0)
    fail("OPV invalid config");
  const auto d = depth(config.vector_length);
  if (config.batch_count > std::numeric_limits<std::uint32_t>::max() / d ||
      static_cast<std::uint64_t>(config.batch_count) * config.vector_length >
          std::numeric_limits<std::size_t>::max() / sizeof(ProtocolIBlock192))
    fail("OPV shape overflow");
}

void put_u64_be(std::uint8_t out[8], std::uint64_t value) {
  for (int i = 7; i >= 0; --i) { out[i] = static_cast<std::uint8_t>(value); value >>= 8; }
}
std::array<std::uint8_t, SHA256_DIGEST_LENGTH> digest(std::string_view domain,
                                                        const ProtocolISeed128& seed,
                                                        std::uint64_t counter) {
  SHA256_CTX context;
  std::array<std::uint8_t, SHA256_DIGEST_LENGTH> output{};
  std::uint8_t encoded[8];
  put_u64_be(encoded, counter);
  if (SHA256_Init(&context) != 1 ||
      SHA256_Update(&context, domain.data(), domain.size()) != 1 ||
      SHA256_Update(&context, seed.data(), seed.size()) != 1 ||
      SHA256_Update(&context, encoded, sizeof(encoded)) != 1 ||
      SHA256_Final(output.data(), &context) != 1) fail("OPV SHA-256 failure");
  return output;
}
ProtocolISeed128 derive_seed(const ProtocolISeed128& seed, std::uint64_t side) {
  const auto value = digest(kChildDomain, seed, side);
  ProtocolISeed128 result{};
  std::copy_n(value.begin(), result.size(), result.begin());
  return result;
}
std::pair<ProtocolISeed128, ProtocolISeed128> children(const ProtocolISeed128& seed) {
  return {derive_seed(seed, 0), derive_seed(seed, 1)};
}
ProtocolIBlock192 leaf(const ProtocolISeed128& seed) {
  ProtocolIBlock192 result{};
  auto* words = &result.word0;
  for (std::uint64_t i = 0; i != 3; ++i) {
    const auto value = digest(kLeafDomain, seed, i);
    std::uint64_t word = 0;
    for (int byte = 0; byte != 8; ++byte) word = (word << 8) | value[byte];
    words[i] = word;
  }
  return result;
}
ProtocolISeed128 fresh_seed() {
  ProtocolISeed128 seed{};
  if (RAND_bytes(seed.data(), static_cast<int>(seed.size())) != 1) fail("OPV random seed failure");
  return seed;
}
ProtocolISeed128 xor_seed(ProtocolISeed128 left, const ProtocolISeed128& right) {
  for (std::size_t i = 0; i != left.size(); ++i) left[i] ^= right[i];
  return left;
}
ProtocolIChosenOtConfig ot_config(const ProtocolIOpvConfig& config, std::uint32_t items) {
  return {config.session, config.fingerprint, config.material_id, items, config.timeout_ms, kOpvProtocolId};
}

}  // namespace

ProtocolIOpvFullVectorResult protocol_i_opv_full_vector_owner(
    const ProtocolIOpvConfig& config, int connected_fd) {
  validate(config);
  const auto d = depth(config.vector_length);
  const auto items64 = static_cast<std::uint64_t>(config.batch_count) * d;
  const auto items = static_cast<std::uint32_t>(items64);
  std::vector<ProtocolIBlock128> slot0;
  std::vector<ProtocolIBlock128> slot1;
  slot0.reserve(items); slot1.reserve(items);
  ProtocolIOpvFullVectorResult result;
  result.leaves.resize(config.batch_count);
  for (std::uint32_t instance = 0; instance != config.batch_count; ++instance) {
    std::vector<ProtocolISeed128> level{fresh_seed()};
    for (std::uint32_t layer = 0; layer != d; ++layer) {
      ProtocolISeed128 left_xor{}, right_xor{};
      std::vector<ProtocolISeed128> next(level.size() * 2);
      for (std::size_t index = 0; index != level.size(); ++index) {
        const auto pair = children(level[index]);
        next[2 * index] = pair.first; next[2 * index + 1] = pair.second;
        left_xor = xor_seed(left_xor, pair.first);
        right_xor = xor_seed(right_xor, pair.second);
      }
      // For an MSB-first puncture bit b, the sibling is right for b=0 and
      // left for b=1.  Thus chosen-OT slot 0=right, slot 1=left.
      slot0.push_back(right_xor);
      slot1.push_back(left_xor);
      level = std::move(next);
    }
    result.leaves[instance].reserve(config.vector_length);
    for (const auto& seed : level) result.leaves[instance].push_back(leaf(seed));
  }
  result.counters = protocol_i_chosen_ot_sender(ot_config(config, items), connected_fd, slot0, slot1);
  result.chosen_ot_items = items;
  return result;
}

ProtocolIOpvPuncturedResult protocol_i_opv_puncture_owner(
    const ProtocolIOpvConfig& config, int connected_fd,
    const std::vector<std::uint32_t>& punctured_indices) {
  validate(config);
  if (punctured_indices.size() != config.batch_count) fail("OPV puncture count");
  const auto d = depth(config.vector_length);
  const auto items64 = static_cast<std::uint64_t>(config.batch_count) * d;
  const auto items = static_cast<std::uint32_t>(items64);
  std::vector<std::uint8_t> choices(items);
  for (std::uint32_t instance = 0; instance != config.batch_count; ++instance) {
    if (punctured_indices[instance] >= config.vector_length) fail("OPV puncture range");
    for (std::uint32_t layer = 0; layer != d; ++layer)
      choices[instance * d + layer] = static_cast<std::uint8_t>(
          (punctured_indices[instance] >> (d - 1 - layer)) & 1U);
  }
  const auto selected = protocol_i_chosen_ot_receiver(ot_config(config, items), connected_fd, choices);
  ProtocolIOpvPuncturedResult result;
  result.punctured_indices = punctured_indices;
  result.leaves.resize(config.batch_count);
  for (std::uint32_t instance = 0; instance != config.batch_count; ++instance) {
    std::vector<std::optional<ProtocolISeed128>> known(1);
    for (std::uint32_t layer = 0; layer != d; ++layer) {
      const auto bit = choices[instance * d + layer];
      const auto path = punctured_indices[instance] >> (d - layer);
      ProtocolISeed128 sibling = selected.selected_messages[instance * d + layer];
      std::vector<std::optional<ProtocolISeed128>> next(known.size() * 2);
      for (std::size_t index = 0; index != known.size(); ++index) if (known[index]) {
        const auto pair = children(*known[index]);
        next[2 * index] = pair.first; next[2 * index + 1] = pair.second;
        sibling = xor_seed(sibling, bit ? pair.first : pair.second);
      }
      next[2 * path + (bit ? 0 : 1)] = sibling;
      known = std::move(next);
    }
    result.leaves[instance].resize(config.vector_length);
    for (std::uint32_t index = 0; index != config.vector_length; ++index)
      if (known[index]) result.leaves[instance][index] = leaf(*known[index]);
  }
  result.counters = selected.counters;
  result.chosen_ot_items = items;
  return result;
}

}  // namespace moe_topk
