#include <moe_topk/protocol_i_share_translation.h>

#include <stdexcept>

namespace moe_topk {
namespace {
[[noreturn]] void fail(const char* message) { throw std::runtime_error(message); }
ProtocolIBlock192 add(ProtocolIBlock192 x, const ProtocolIBlock192& y) {
  return {x.word0 + y.word0, x.word1 + y.word1, x.word2 + y.word2};
}
ProtocolIBlock192 sub(ProtocolIBlock192 x, const ProtocolIBlock192& y) {
  return {x.word0 - y.word0, x.word1 - y.word1, x.word2 - y.word2};
}
ProtocolIOpvConfig translation_config(const ProtocolIOpvConfig& input) {
  ProtocolIOpvConfig config = input;
  config.batch_count = config.vector_length;
  return config;
}
}  // namespace

ProtocolIShareTranslationFvoResult protocol_i_share_translation_fvo(
    const ProtocolIOpvConfig& input, int connected_fd) {
  const auto config = translation_config(input);
  const auto opv = protocol_i_opv_full_vector_owner(config, connected_fd);
  ProtocolIShareTranslationFvoResult result;
  result.a.resize(config.vector_length); result.b.resize(config.vector_length);
  for (std::uint32_t i = 0; i != config.vector_length; ++i)
    for (std::uint32_t j = 0; j != config.vector_length; ++j) {
      result.b[i] = add(result.b[i], opv.leaves[i][j]);
      result.a[j] = add(result.a[j], opv.leaves[i][j]);
    }
  result.counters = opv.counters; result.opv_instances = config.vector_length;
  result.chosen_ot_items = opv.chosen_ot_items;
  return result;
}

ProtocolIShareTranslationPoResult protocol_i_share_translation_po(
    const ProtocolIOpvConfig& input, int connected_fd,
    const std::vector<std::uint32_t>& permutation) {
  const auto config = translation_config(input);
  if (permutation.size() != config.vector_length) fail("share translation permutation shape");
  std::vector<bool> seen(config.vector_length);
  for (const auto value : permutation) {
    if (value >= config.vector_length || seen[value]) fail("share translation invalid permutation");
    seen[value] = true;
  }
  const auto opv = protocol_i_opv_puncture_owner(config, connected_fd, permutation);
  ProtocolIShareTranslationPoResult result;
  result.delta.resize(config.vector_length);
  for (std::uint32_t i = 0; i != config.vector_length; ++i) {
    ProtocolIBlock192 row{}, column{};
    for (std::uint32_t j = 0; j != config.vector_length; ++j) {
      if (j != permutation[i]) row = add(row, *opv.leaves[i][j]);
      if (j != i) column = add(column, *opv.leaves[j][permutation[i]]);
    }
    result.delta[i] = sub(row, column);
  }
  result.counters = opv.counters; result.opv_instances = config.vector_length;
  result.chosen_ot_items = opv.chosen_ot_items;
  return result;
}

}  // namespace moe_topk
