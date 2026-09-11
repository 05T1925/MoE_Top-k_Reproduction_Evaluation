#include <moe_topk/protocol_i_dealer_candidate_package.h>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <stdexcept>
#include <utility>

#include <moe_topk/protocol_i_pipeline.h>

namespace moe_topk {
namespace {

constexpr std::size_t kMaxPackageBytes = 64U * 1024U * 1024U;
constexpr std::size_t kMaxEdges = 1'000'000U;
constexpr std::size_t kMaxLabelBytes = 128U;

[[noreturn]] void fail(const char* message) { throw std::invalid_argument(message); }

std::size_t edge_count(std::uint32_t n) {
  if (n < 2) fail("candidate edge count");
  const auto nodes = static_cast<std::size_t>(n);
  if (nodes > std::numeric_limits<std::size_t>::max() / (nodes - 1U)) {
    fail("candidate edge count overflow");
  }
  const auto count = nodes * (nodes - 1U) / 2U;
  if (count > kMaxEdges) fail("candidate edge count limit");
  return count;
}

std::uint64_t ring_mask(std::uint8_t bits) {
  if (bits < 34 || bits > 53) fail("candidate comparison bits");
  return (UINT64_C(1) << bits) - 1U;
}

void validate_public(const ProtocolIDealerCandidatePublicConfig& config) {
  if (config.session == 0 || config.fingerprint == 0 || config.material_id == 0) {
    fail("candidate public identity");
  }
  const auto layout = protocol_i_make_input_layout(config.logical_n, config.k);
  if (config.padded_n != layout.padded_n || config.rank_bits != layout.index_bits ||
      config.comparison_bits < layout.minimum_comparison_bits || config.comparison_bits > 53) {
    fail("candidate public layout");
  }
  (void)edge_count(config.padded_n);
}

void validate_package(const ProtocolIDealerCandidatePackage& package) {
  validate_public(package.config);
  if (package.config.party > 1 ||
      package.implementation_label != kProtocolIDealerCandidateLabel ||
      package.r_share.size() != package.config.padded_n ||
      package.edge_materials.size() != edge_count(package.config.padded_n)) {
    fail("candidate package shape");
  }
  const auto mask = ring_mask(package.config.comparison_bits);
  for (const auto share : package.r_share) {
    if ((share & ~mask) != 0) fail("candidate r share outside ring");
  }
  std::size_t slot = 0;
  for (std::uint32_t left = 0; left < package.config.padded_n; ++left) {
    for (std::uint32_t right = left + 1; right < package.config.padded_n; ++right) {
      const auto& edge = package.edge_materials[slot];
      if (edge.left != left || edge.right != right || edge.slot != slot || edge.stage != 1 ||
          edge.material.party_id() != package.config.party ||
          edge.material.comparison_bits() != package.config.comparison_bits) {
        fail("candidate edge binding");
      }
      ++slot;
    }
  }
}

void put_u64(std::vector<std::uint8_t>& output, std::uint64_t value) {
  if (output.size() > kMaxPackageBytes - sizeof(value)) fail("candidate package size");
  for (int shift = 56; shift >= 0; shift -= 8) {
    output.push_back(static_cast<std::uint8_t>(value >> shift));
  }
}

std::uint64_t get_u64(const std::vector<std::uint8_t>& bytes, std::size_t& offset) {
  if (offset > bytes.size() || bytes.size() - offset < sizeof(std::uint64_t)) {
    fail("candidate package truncated");
  }
  std::uint64_t value = 0;
  for (int index = 0; index < 8; ++index) value = (value << 8U) | bytes[offset++];
  return value;
}

std::uint64_t random_ring_word(std::uint8_t bits) {
  std::uint64_t value = 0;
  if (RAND_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value)) != 1) {
    fail("candidate random source");
  }
  return value & ring_mask(bits);
}

void clean_words(std::vector<std::uint64_t>& values) {
  if (!values.empty()) OPENSSL_cleanse(values.data(), values.size() * sizeof(values.front()));
  values.clear();
  values.shrink_to_fit();
}

ProtocolIDealerCandidatePackage make_package(const ProtocolIDealerCandidatePublicConfig& public_config,
                                             int party,
                                             const std::vector<std::uint64_t>& shares,
                                             std::vector<ProtocolIDealerCandidateEdgeMaterial>&& edges) {
  ProtocolIDealerCandidatePackage package;
  package.implementation_label = kProtocolIDealerCandidateLabel;
  static_cast<ProtocolIDealerCandidatePublicConfig&>(package.config) = public_config;
  package.config.party = static_cast<std::uint8_t>(party);
  package.r_share = shares;
  package.edge_materials = std::move(edges);
  validate_package(package);
  return package;
}

}  // namespace

ProtocolIDealerCandidatePackagePair protocol_i_dealer_candidate_preprocess(
    const ProtocolIDealerCandidatePublicConfig& config) {
  validate_public(config);
  const auto mask = ring_mask(config.comparison_bits);
  std::vector<std::uint64_t> dealer_masks(config.padded_n);
  std::vector<std::uint64_t> share0(config.padded_n);
  std::vector<std::uint64_t> share1(config.padded_n);
  for (std::size_t index = 0; index < dealer_masks.size(); ++index) {
    dealer_masks[index] = random_ring_word(config.comparison_bits);
    share0[index] = random_ring_word(config.comparison_bits);
    share1[index] = (dealer_masks[index] - share0[index]) & mask;
  }

  std::vector<ProtocolIDealerCandidateEdgeMaterial> edges0;
  std::vector<ProtocolIDealerCandidateEdgeMaterial> edges1;
  edges0.reserve(edge_count(config.padded_n));
  edges1.reserve(edge_count(config.padded_n));
  std::uint32_t slot = 0;
  for (std::uint32_t left = 0; left < config.padded_n; ++left) {
    for (std::uint32_t right = left + 1; right < config.padded_n; ++right) {
      ProtocolIUcmpMaterial material(config.comparison_bits, dealer_masks[left], dealer_masks[right]);
      edges0.emplace_back(left, right, slot, material.export_party_material(0));
      edges1.emplace_back(left, right, slot, material.export_party_material(1));
      ++slot;
    }
  }

  auto result = ProtocolIDealerCandidatePackagePair{
      make_package(config, 0, share0, std::move(edges0)),
      make_package(config, 1, share1, std::move(edges1))};
  clean_words(dealer_masks);
  clean_words(share0);
  clean_words(share1);
  return result;
}

std::vector<std::uint8_t> serialize_dealer_candidate_package(
    const ProtocolIDealerCandidatePackage& package) {
  if (package.consumed) fail("candidate package consumed");
  validate_package(package);
  if (package.implementation_label.size() > kMaxLabelBytes) fail("candidate label size");

  std::vector<std::uint8_t> output;
  output.reserve(256);
  output.insert(output.end(), {'M', '2', 'D', 'C', 1, package.config.party,
                               package.config.comparison_bits, package.config.rank_bits});
  put_u64(output, package.config.session);
  put_u64(output, package.config.fingerprint);
  put_u64(output, package.config.material_id);
  put_u64(output, package.config.logical_n);
  put_u64(output, package.config.padded_n);
  put_u64(output, package.config.k);
  put_u64(output, package.implementation_label.size());
  if (output.size() > kMaxPackageBytes - package.implementation_label.size()) fail("candidate package size");
  output.insert(output.end(), package.implementation_label.begin(), package.implementation_label.end());
  put_u64(output, package.r_share.size());
  for (const auto share : package.r_share) put_u64(output, share);
  put_u64(output, package.edge_materials.size());
  for (const auto& edge : package.edge_materials) {
    put_u64(output, edge.stage);
    put_u64(output, edge.slot);
    put_u64(output, edge.left);
    put_u64(output, edge.right);
    const auto material = edge.material.serialize();
    put_u64(output, material.size());
    if (output.size() > kMaxPackageBytes - material.size()) fail("candidate package size");
    output.insert(output.end(), material.begin(), material.end());
  }
  return output;
}

ProtocolIDealerCandidatePackage deserialize_dealer_candidate_package(
    const std::vector<std::uint8_t>& bytes, int expected_party) {
  if (expected_party < 0 || expected_party > 1 || bytes.size() > kMaxPackageBytes || bytes.size() < 8 ||
      bytes[0] != 'M' || bytes[1] != '2' || bytes[2] != 'D' || bytes[3] != 'C' || bytes[4] != 1 ||
      bytes[5] != expected_party) {
    fail("candidate package header");
  }
  ProtocolIDealerCandidatePackage package;
  package.config.party = bytes[5];
  package.config.comparison_bits = bytes[6];
  package.config.rank_bits = bytes[7];
  std::size_t offset = 8;
  package.config.session = get_u64(bytes, offset);
  package.config.fingerprint = get_u64(bytes, offset);
  package.config.material_id = get_u64(bytes, offset);
  const auto logical = get_u64(bytes, offset);
  const auto padded = get_u64(bytes, offset);
  const auto k = get_u64(bytes, offset);
  const auto label_size = get_u64(bytes, offset);
  if (logical > std::numeric_limits<std::uint32_t>::max() ||
      padded > std::numeric_limits<std::uint32_t>::max() ||
      k > std::numeric_limits<std::uint32_t>::max() || label_size > kMaxLabelBytes ||
      label_size > bytes.size() - offset) {
    fail("candidate package identity");
  }
  package.config.logical_n = static_cast<std::uint32_t>(logical);
  package.config.padded_n = static_cast<std::uint32_t>(padded);
  package.config.k = static_cast<std::uint32_t>(k);
  package.implementation_label.assign(bytes.begin() + offset,
                                      bytes.begin() + offset + static_cast<std::size_t>(label_size));
  offset += static_cast<std::size_t>(label_size);
  validate_public(package.config);
  if (package.implementation_label != kProtocolIDealerCandidateLabel) fail("candidate label");

  const auto share_count = get_u64(bytes, offset);
  if (share_count != package.config.padded_n) fail("candidate share shape");
  package.r_share.resize(package.config.padded_n);
  const auto mask = ring_mask(package.config.comparison_bits);
  for (auto& share : package.r_share) {
    share = get_u64(bytes, offset);
    if ((share & ~mask) != 0) fail("candidate r share outside ring");
  }

  const auto encoded_edges = get_u64(bytes, offset);
  const auto expected_edges = edge_count(package.config.padded_n);
  if (encoded_edges != expected_edges) fail("candidate edge shape");
  package.edge_materials.reserve(expected_edges);
  for (std::uint32_t slot = 0; slot < expected_edges; ++slot) {
    const auto stage = get_u64(bytes, offset);
    const auto encoded_slot = get_u64(bytes, offset);
    const auto left = get_u64(bytes, offset);
    const auto right = get_u64(bytes, offset);
    const auto material_size = get_u64(bytes, offset);
    if (stage != 1 || encoded_slot != slot || left > std::numeric_limits<std::uint32_t>::max() ||
        right > std::numeric_limits<std::uint32_t>::max() || material_size > bytes.size() - offset) {
      fail("candidate edge identity");
    }
    const auto material_length = static_cast<std::size_t>(material_size);
    std::vector<std::uint8_t> material_bytes(bytes.begin() + offset,
                                              bytes.begin() + offset + material_length);
    offset += material_length;
    package.edge_materials.emplace_back(static_cast<std::uint32_t>(left),
                                        static_cast<std::uint32_t>(right), slot,
                                        ProtocolIUcmpPartyMaterial::deserialize(material_bytes));
  }
  if (offset != bytes.size()) fail("candidate package trailing bytes");
  package.serialized_bytes = bytes.size();
  validate_package(package);
  return package;
}

}  // namespace moe_topk
