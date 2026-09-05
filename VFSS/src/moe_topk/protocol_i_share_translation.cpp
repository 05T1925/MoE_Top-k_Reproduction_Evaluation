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
ProtocolIBlock192 required(const std::optional<ProtocolIBlock192>& value, const char* message) {
  if (!value) fail(message); return *value;
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
      if (j != permutation[i]) row = add(row, required(opv.leaves[i][j], "share translation missing row leaf"));
      if (j != i) column = add(column, required(opv.leaves[j][permutation[i]], "share translation missing column leaf"));
    }
    result.delta[i] = sub(row, column);
  }
  result.counters = opv.counters; result.opv_instances = config.vector_length;
  result.chosen_ot_items = opv.chosen_ot_items;
  return result;
}

ProtocolIBatchedTranslationFvoResult protocol_i_share_translation_fvo_batch(
    const ProtocolIOpvConfig& input, int connected_fd, std::uint32_t count) {
  if (count == 0 || count > UINT32_MAX / input.vector_length) fail("share translation batch count");
  ProtocolIOpvConfig config = input; config.batch_count = count * config.vector_length;
  const auto opv = protocol_i_opv_full_vector_owner(config, connected_fd);
  ProtocolIBatchedTranslationFvoResult result; result.a.assign(count, std::vector<ProtocolIBlock192>(config.vector_length)); result.b = result.a;
  for (std::uint32_t group=0; group<count; ++group) for (std::uint32_t i=0;i<config.vector_length;++i) for(std::uint32_t j=0;j<config.vector_length;++j) {
    const auto& value=opv.leaves[group*config.vector_length+i][j]; result.b[group][i]=add(result.b[group][i],value); result.a[group][j]=add(result.a[group][j],value);
  }
  result.counters=opv.counters; result.translation_count=count; result.opv_instances=config.batch_count; result.chosen_ot_items=opv.chosen_ot_items; return result;
}

ProtocolIBatchedTranslationPoResult protocol_i_share_translation_po_batch(
    const ProtocolIOpvConfig& input, int connected_fd, const std::vector<std::vector<std::uint32_t>>& permutations) {
  const auto count=static_cast<std::uint32_t>(permutations.size());
  if(count==0||count>UINT32_MAX/input.vector_length) fail("share translation batch count");
  std::vector<std::uint32_t> punctures; punctures.reserve(static_cast<std::size_t>(count)*input.vector_length);
  for(const auto& p:permutations){if(p.size()!=input.vector_length)fail("share translation permutation shape");std::vector<bool> seen(input.vector_length);for(auto v:p){if(v>=input.vector_length||seen[v])fail("share translation invalid permutation");seen[v]=true;punctures.push_back(v);}}
  ProtocolIOpvConfig config=input;config.batch_count=count*config.vector_length;const auto opv=protocol_i_opv_puncture_owner(config,connected_fd,punctures);
  ProtocolIBatchedTranslationPoResult result;result.delta.assign(count,std::vector<ProtocolIBlock192>(config.vector_length));
  for(std::uint32_t g=0;g<count;++g)for(std::uint32_t i=0;i<config.vector_length;++i){const auto& p=permutations[g];ProtocolIBlock192 row{},column{};for(std::uint32_t j=0;j<config.vector_length;++j){const auto& current=opv.leaves[g*config.vector_length+i][j];if(j==p[i]){if(current)fail("share translation puncture leaf present");}else row=add(row,required(current,"share translation missing row leaf"));if(j!=i)column=add(column,required(opv.leaves[g*config.vector_length+j][p[i]],"share translation missing column leaf"));}result.delta[g][i]=sub(row,column);}
  result.counters=opv.counters;result.translation_count=count;result.opv_instances=config.batch_count;result.chosen_ot_items=opv.chosen_ot_items;return result;
}

}  // namespace moe_topk
