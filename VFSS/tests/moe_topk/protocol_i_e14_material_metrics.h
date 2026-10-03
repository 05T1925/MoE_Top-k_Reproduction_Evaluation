#pragma once

// TEST_ONLY canonical effective-payload accounting.  This counts each
// populated field retained at the offline ready barrier that the online
// Protocol I entries read.  It excludes container capacity, C++ padding,
// package labels, wire framing, and discarded EMP OT intermediate state.

#include <moe_topk/protocol_i_aav86_small.h>
#include <moe_topk/protocol_i_secret_shared_shuffle.h>

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace moe_topk::test_only {

inline std::uint64_t checked_add(std::uint64_t a, std::uint64_t b) {
  if (b > std::numeric_limits<std::uint64_t>::max() - a)
    throw std::overflow_error("E14 material byte sum");
  return a + b;
}
inline std::uint64_t checked_bytes(std::size_t count, std::uint64_t width) {
  if (count > std::numeric_limits<std::uint64_t>::max() / width)
    throw std::overflow_error("E14 material byte product");
  return static_cast<std::uint64_t>(count) * width;
}
inline std::uint64_t key_payload_bytes(const ProtocolIUcmpPartyMaterial& key) {
  // ProtocolIUcmpPartyMaterial::serialize: fixed 33-byte structure header,
  // then actual k/g/v contents.  Serialization is measured before Eval.
  const auto encoded = key.serialize();
  if (encoded.size() < 33) throw std::runtime_error("E14 DCF key encoding");
  return static_cast<std::uint64_t>(encoded.size() - 33);
}

struct MaterialPayload {
  std::uint64_t t_package_payload_bytes = 0;
  std::uint64_t local_shuffle_payload_bytes = 0;
  std::uint64_t local_shuffle_ot_sent_bytes = 0;
  std::uint64_t local_shuffle_ot_received_bytes = 0;
  std::uint64_t total_payload_bytes() const {
    return checked_add(t_package_payload_bytes, local_shuffle_payload_bytes);
  }
};

inline std::uint64_t score_payload_bytes(const ProtocolIPartyPackage& package) {
  std::uint64_t out = 0;
  for (const auto* group : {&package.carry_materials, &package.sign_materials})
    for (const auto& item : *group) {
      out = checked_add(out, 16); // two actual 64-bit mask shares
      out = checked_add(out, key_payload_bytes(item.material));
    }
  return out;
}
inline MaterialPayload payload(const ProtocolIPartyPackage& package,
                               const ProtocolIShufflePartyMaterial& shuffle) {
  MaterialPayload out;
  out.t_package_payload_bytes = checked_bytes(package.node_mask_shares.size(), 8);
  out.t_package_payload_bytes = checked_add(out.t_package_payload_bytes,
                                            score_payload_bytes(package));
  for (const auto& edge : package.edge_materials)
    out.t_package_payload_bytes = checked_add(out.t_package_payload_bytes,
                                              key_payload_bytes(edge.material));

  auto& local = out.local_shuffle_payload_bytes;
  local = checked_add(local, checked_bytes(shuffle.own_permutation.size(), 4));
  local = checked_add(local, checked_bytes(shuffle.own_inverse_permutation.size(), 4));
  for (const auto* po : {&shuffle.forward_po_first, &shuffle.forward_po_second,
                         &shuffle.reverse_po_first, &shuffle.reverse_po_second}) {
    local = checked_add(local, checked_bytes(po->permutation.size(), 4));
    for (const auto& layer : po->layers)
      local = checked_add(local, checked_bytes(layer.permutation.size(), 4));
    for (const auto& row : po->delta)
      local = checked_add(local, checked_bytes(row.size(), 24));
    out.local_shuffle_ot_sent_bytes = checked_add(
        out.local_shuffle_ot_sent_bytes, po->counters.offline_ot.sent_bytes);
    out.local_shuffle_ot_received_bytes = checked_add(
        out.local_shuffle_ot_received_bytes, po->counters.offline_ot.received_bytes);
  }
  for (const auto* data : {&shuffle.forward_do_first, &shuffle.forward_do_second,
                           &shuffle.reverse_do_first, &shuffle.reverse_do_second}) {
    for (const auto* matrix : {&data->a, &data->b})
      for (const auto& row : *matrix)
        local = checked_add(local, checked_bytes(row.size(), 24));
    local = checked_add(local, checked_bytes(data->w.size(), 24));
    out.local_shuffle_ot_sent_bytes = checked_add(
        out.local_shuffle_ot_sent_bytes, data->counters.offline_ot.sent_bytes);
    out.local_shuffle_ot_received_bytes = checked_add(
        out.local_shuffle_ot_received_bytes, data->counters.offline_ot.received_bytes);
  }
  return out;
}
inline MaterialPayload payload(const ProtocolIAav86SmallPartyMaterial& material) {
  MaterialPayload out;
  auto& bytes = out.t_package_payload_bytes;
  for (const auto* perm : {&material.forward_sigma, &material.forward_tau,
                           &material.inverse_sigma, &material.inverse_tau})
    bytes = checked_add(bytes, checked_bytes(perm->size(), 4));
  for (const auto* words : {&material.forward_a, &material.forward_e,
                            &material.inverse_a, &material.inverse_e,
                            &material.node_mask_shares})
    bytes = checked_add(bytes, checked_bytes(words->size(), 8));
  bytes = checked_add(bytes, score_payload_bytes(material.score_materials));
  for (const auto& edge : material.edge_keys)
    bytes = checked_add(bytes, key_payload_bytes(edge));
  return out;
}
} // namespace moe_topk::test_only
