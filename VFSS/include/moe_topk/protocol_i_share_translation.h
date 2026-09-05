#pragma once

#include <vector>

#include <moe_topk/protocol_i_opv.h>

namespace moe_topk {

struct ProtocolIShareTranslationFvoResult {
  // Both vectors are local FVO output; neither encodes the PO permutation.
  std::vector<ProtocolIBlock192> a;
  std::vector<ProtocolIBlock192> b;
  ProtocolIChosenOtCounters counters;
  std::uint64_t opv_instances = 0;
  std::uint64_t chosen_ot_items = 0;
};

struct ProtocolIShareTranslationPoResult {
  std::vector<ProtocolIBlock192> delta;
  ProtocolIChosenOtCounters counters;
  std::uint64_t opv_instances = 0;
  std::uint64_t chosen_ot_items = 0;
};

// These role-local outputs intentionally cannot be jointly verified by a
// production helper.  Reconstruction belongs solely to TEST_ONLY controllers.
ProtocolIShareTranslationFvoResult protocol_i_share_translation_fvo(
    const ProtocolIOpvConfig& config, int connected_fd);
ProtocolIShareTranslationPoResult protocol_i_share_translation_po(
    const ProtocolIOpvConfig& config, int connected_fd,
    const std::vector<std::uint32_t>& permutation);

}  // namespace moe_topk
