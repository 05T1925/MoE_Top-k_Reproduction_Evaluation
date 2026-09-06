#include <moe_topk/protocol_i_secret_shared_shuffle.h>

#include <stdexcept>

namespace moe_topk {
namespace {
void require(bool condition, const char* message) {
  if (!condition) throw std::invalid_argument(message);
}
ProtocolIPermuteShareConfig stage_config(const ProtocolIShufflePartyConfig& config,
                                         unsigned stage, unsigned owner) {
  require(config.material_id_base <= UINT64_MAX - stage &&
              config.online_message_id_base <= UINT64_MAX - stage,
          "shuffle material id overflow");
  return {config.session, config.fingerprint, config.material_id_base + stage,
          config.online_message_id_base + stage, config.n, config.subpermutation_size,
          static_cast<std::uint8_t>(owner), config.timeout_ms};
}
std::vector<ProtocolIBlock192> add(const std::vector<ProtocolIBlock192>& left,
                                   const std::vector<ProtocolIBlock192>& right) {
  require(left.size() == right.size(), "shuffle share length");
  auto output = left;
  for (std::size_t index = 0; index < output.size(); ++index) {
    output[index] = {output[index].word0 + right[index].word0,
                     output[index].word1 + right[index].word1,
                     output[index].word2 + right[index].word2};
  }
  return output;
}
ProtocolIShufflePartyCounters counters(const ProtocolIPermuteSharePoResult& first,
                                       const ProtocolIPermuteShareDoResult& second) {
  ProtocolIShufflePartyCounters output;
  output.forward_first = first.counters;
  output.forward_second = second.counters;
  return output;
}
}  // namespace

ProtocolIShufflePartyMaterial protocol_i_shuffle_preprocess_party(
    const ProtocolIShufflePartyConfig& config, const std::array<int, 4>& offline_fds,
    const ProtocolIPermutation& own_permutation) {
  require(config.party < 2 && config.session != 0 && config.fingerprint != 0 &&
              config.material_id_base != 0 && config.online_message_id_base != 0 &&
              config.n >= 2 && config.timeout_ms > 0,
          "shuffle configuration");
  protocol_i_validate_permutation(own_permutation);
  require(own_permutation.size() == config.n, "shuffle permutation length");
  ProtocolIShufflePartyMaterial output;
  output.config = config;
  output.own_permutation = own_permutation;
  output.own_inverse_permutation = protocol_i_inverse_permutation(own_permutation);
  const auto c1 = stage_config(config, 1, 0), c2 = stage_config(config, 2, 1);
  const auto c3 = stage_config(config, 3, 1), c4 = stage_config(config, 4, 0);
  if (config.party == 0) {
    output.forward_po_first = protocol_i_permute_share_preprocess_po(c1, offline_fds[0], own_permutation);
    output.forward_do_second = protocol_i_permute_share_preprocess_do(c2, offline_fds[1]);
    output.reverse_do_first = protocol_i_permute_share_preprocess_do(c3, offline_fds[2]);
    output.reverse_po_second = protocol_i_permute_share_preprocess_po(c4, offline_fds[3], output.own_inverse_permutation);
  } else {
    output.forward_do_first = protocol_i_permute_share_preprocess_do(c1, offline_fds[0]);
    output.forward_po_second = protocol_i_permute_share_preprocess_po(c2, offline_fds[1], own_permutation);
    output.reverse_po_first = protocol_i_permute_share_preprocess_po(c3, offline_fds[2], output.own_inverse_permutation);
    output.reverse_do_second = protocol_i_permute_share_preprocess_do(c4, offline_fds[3]);
  }
  return output;
}

ProtocolIShufflePartyOutput protocol_i_shuffle_forward_party(
    int party, const std::array<int, 2>& online_fds,
    const std::vector<ProtocolIBlock192>& input, ProtocolIShufflePartyMaterial& material) {
  require(party == material.config.party && party >= 0 && party < 2 && !material.forward_consumed &&
              input.size() == material.config.n,
          "shuffle forward material");
  material.forward_consumed = true;
  const auto c1 = stage_config(material.config, 1, 0), c2 = stage_config(material.config, 2, 1);
  ProtocolIShufflePartyOutput output;
  if (party == 0) {
    const auto a0 = protocol_i_permute_share_online_po(c1, online_fds[0], material.own_permutation,
                                                        std::move(material.forward_po_first));
    const auto b0 = add(protocol_i_apply_permutation(material.own_permutation, input), a0.share);
    const auto c0 = protocol_i_permute_share_online_do(c2, online_fds[1], b0,
                                                        std::move(material.forward_do_second));
    output.share = c0.share; output.counters.forward_first = a0.counters; output.counters.forward_second = c0.counters;
  } else {
    const auto a1 = protocol_i_permute_share_online_do(c1, online_fds[0], input,
                                                        std::move(material.forward_do_first));
    const auto c1_share = protocol_i_permute_share_online_po(c2, online_fds[1], material.own_permutation,
                                                              std::move(material.forward_po_second));
    output.share = add(protocol_i_apply_permutation(material.own_permutation, a1.share), c1_share.share);
    output.counters.forward_first = a1.counters; output.counters.forward_second = c1_share.counters;
  }
  return output;
}

ProtocolIShufflePartyOutput protocol_i_shuffle_reverse_party(
    int party, const std::array<int, 2>& online_fds,
    const std::vector<ProtocolIBlock192>& shuffled_carrier,
    ProtocolIShufflePartyMaterial& material) {
  require(party == material.config.party && party >= 0 && party < 2 && material.forward_consumed &&
              !material.reverse_consumed && shuffled_carrier.size() == material.config.n,
          "shuffle reverse material");
  material.reverse_consumed = true;
  const auto c3 = stage_config(material.config, 3, 1), c4 = stage_config(material.config, 4, 0);
  ProtocolIShufflePartyOutput output;
  if (party == 0) {
    const auto e0 = protocol_i_permute_share_online_do(c3, online_fds[0], shuffled_carrier,
                                                        std::move(material.reverse_do_first));
    const auto g0 = protocol_i_permute_share_online_po(c4, online_fds[1], material.own_inverse_permutation,
                                                        std::move(material.reverse_po_second));
    output.share = add(protocol_i_apply_permutation(material.own_inverse_permutation, e0.share), g0.share);
    output.counters.reverse_first = e0.counters; output.counters.reverse_second = g0.counters;
  } else {
    const auto e1 = protocol_i_permute_share_online_po(c3, online_fds[0], material.own_inverse_permutation,
                                                        std::move(material.reverse_po_first));
    const auto f1 = add(protocol_i_apply_permutation(material.own_inverse_permutation, shuffled_carrier), e1.share);
    const auto g1 = protocol_i_permute_share_online_do(c4, online_fds[1], f1,
                                                        std::move(material.reverse_do_second));
    output.share = g1.share; output.counters.reverse_first = e1.counters; output.counters.reverse_second = g1.counters;
  }
  return output;
}
}  // namespace moe_topk
