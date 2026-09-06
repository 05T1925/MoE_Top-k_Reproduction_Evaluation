#include <moe_topk/protocol_i_party_package.h>
#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>

namespace moe_topk {
namespace {

constexpr std::size_t kMaxPackageBytes = 64U * 1024U * 1024U;
constexpr std::size_t kMaxEdges = 1'000'000U;
constexpr std::size_t kHeaderBytes = 8U + 4U * sizeof(std::uint64_t);

void put_u64(std::vector<std::uint8_t>& output, std::uint64_t value) {
  if (output.size() > kMaxPackageBytes - sizeof(value)) {
    throw std::invalid_argument("party package exceeds allocation limit");
  }
  for (int shift = 56; shift >= 0; shift -= 8) {
    output.push_back(static_cast<std::uint8_t>(value >> shift));
  }
}

std::uint64_t get_u64(const std::vector<std::uint8_t>& bytes, std::size_t& offset) {
  if (bytes.size() - offset < sizeof(std::uint64_t)) {
    throw std::invalid_argument("truncated party package");
  }
  std::uint64_t value = 0;
  for (int index = 0; index < 8; ++index) {
    value = (value << 8U) | bytes[offset++];
  }
  return value;
}

std::size_t edge_count(std::uint64_t n) {
  if (n == 0 || n > std::numeric_limits<std::size_t>::max()) {
    throw std::invalid_argument("party package node count");
  }
  const auto nodes = static_cast<std::size_t>(n);
  const auto smaller = nodes - 1U;
  if (smaller != 0 && nodes > std::numeric_limits<std::size_t>::max() / smaller) {
    throw std::invalid_argument("party package edge count overflow");
  }
  const auto count = (nodes * smaller) / 2U;
  if (count > kMaxEdges) {
    throw std::invalid_argument("party package edge count exceeds allocation limit");
  }
  return count;
}

std::uint64_t ring_mask(int bits) {
  if (bits < 34 || bits > 53) {
    throw std::invalid_argument("party package comparison bits");
  }
  return (UINT64_C(1) << bits) - 1U;
}

void validate_common(const ProtocolIPartyPackage& package, std::size_t expected_edges) {
  if (package.party < 0 || package.party > 1 || package.n == 0 || package.k == 0 ||
      package.k > package.n || package.comparison_bits < 34 || package.comparison_bits > 53 ||
      package.node_mask_shares.size() != package.n ||
      package.edge_materials.size() != expected_edges) {
    throw std::invalid_argument("invalid party package");
  }
  if ((package.carry_materials.empty() != package.sign_materials.empty()) ||
      (!package.carry_materials.empty() &&
       (package.carry_materials.size() != package.n || package.sign_materials.size() != package.n))) {
    throw std::invalid_argument("party package score material count");
  }
  const auto mask = ring_mask(package.comparison_bits);
  for (const auto share : package.node_mask_shares) {
    if ((share & ~mask) != 0) {
      throw std::invalid_argument("party package node mask outside ring");
    }
  }
}

void validate_score_material(const ProtocolIScoreInputPartyMaterial& item,
                             std::uint32_t expected_slot, std::uint8_t expected_stage,
                             const ProtocolIPartyPackage& package) {
  constexpr int kScoreBits = 34;
  const auto score_mask = (UINT64_C(1) << kScoreBits) - 1U;
  if (item.slot != expected_slot || item.stage != expected_stage || item.slot >= package.n ||
      item.material.party_id() != package.party || item.material.comparison_bits() != kScoreBits ||
      (item.left_mask_share & ~score_mask) != 0 || (item.right_mask_share & ~score_mask) != 0) {
    throw std::invalid_argument("party package score material binding");
  }
}

void put_score_material(std::vector<std::uint8_t>& output,
                        const ProtocolIScoreInputPartyMaterial& item) {
  const auto material = item.material.serialize();
  put_u64(output, item.slot);
  put_u64(output, item.stage);
  put_u64(output, item.left_mask_share);
  put_u64(output, item.right_mask_share);
  put_u64(output, material.size());
  if (output.size() > kMaxPackageBytes - material.size()) {
    throw std::invalid_argument("party package exceeds allocation limit");
  }
  output.insert(output.end(), material.begin(), material.end());
}

ProtocolIScoreInputPartyMaterial get_score_material(const std::vector<std::uint8_t>& bytes,
                                                    std::size_t& offset, std::uint32_t slot,
                                                    std::uint8_t stage,
                                                    const ProtocolIPartyPackage& package) {
  const auto encoded_slot = get_u64(bytes, offset);
  const auto encoded_stage = get_u64(bytes, offset);
  const auto left = get_u64(bytes, offset);
  const auto right = get_u64(bytes, offset);
  const auto material_size = get_u64(bytes, offset);
  if (encoded_slot != slot || encoded_stage != stage || material_size > bytes.size() - offset ||
      material_size > std::numeric_limits<std::size_t>::max()) {
    throw std::invalid_argument("party package score identity or material length");
  }
  const auto material_length = static_cast<std::size_t>(material_size);
  std::vector<std::uint8_t> encoded(bytes.begin() + offset, bytes.begin() + offset + material_length);
  offset += material_length;
  ProtocolIScoreInputPartyMaterial item(slot, stage, left, right,
      ProtocolIUcmpPartyMaterial::deserialize(encoded));
  validate_score_material(item, slot, stage, package);
  return item;
}

void validate_edge(const ProtocolIEdgePartyMaterial& edge,
                   std::uint32_t expected_left, std::uint32_t expected_right,
                   const ProtocolIPartyPackage& package) {
  if (edge.left != expected_left || edge.right != expected_right || edge.left >= edge.right ||
      edge.right >= package.n || edge.material.party_id() != package.party ||
      edge.material.comparison_bits() != package.comparison_bits) {
    throw std::invalid_argument("party package edge binding");
  }
}

}  // namespace

std::vector<std::uint8_t> serialize_party_package(const ProtocolIPartyPackage& package) {
  const auto expected_edges = edge_count(package.n);
  validate_common(package, expected_edges);

  std::vector<std::uint8_t> output;
  output.reserve(std::min(kMaxPackageBytes, kHeaderBytes +
                                               sizeof(std::uint64_t) * package.n));
  output.insert(output.end(), {'M', '2', 'P', 'K', 2,
                               static_cast<std::uint8_t>(package.party),
                               static_cast<std::uint8_t>(package.comparison_bits), 0});
  put_u64(output, package.session);
  put_u64(output, package.fingerprint);
  put_u64(output, package.n);
  put_u64(output, package.k);
  for (const auto share : package.node_mask_shares) {
    put_u64(output, share);
  }
  put_u64(output, package.carry_materials.size());
  if (!package.carry_materials.empty()) {
    for (std::uint32_t slot = 0; slot < package.n; ++slot) {
      validate_score_material(package.carry_materials[slot], slot, 1, package);
      put_score_material(output, package.carry_materials[slot]);
    }
    for (std::uint32_t slot = 0; slot < package.n; ++slot) {
      validate_score_material(package.sign_materials[slot], slot, 2, package);
      put_score_material(output, package.sign_materials[slot]);
    }
  }

  std::size_t edge_index = 0;
  for (std::uint32_t left = 0; left < package.n; ++left) {
    for (std::uint32_t right = left + 1; right < package.n; ++right) {
      const auto& edge = package.edge_materials[edge_index++];
      validate_edge(edge, left, right, package);
      const auto material = edge.material.serialize();
      put_u64(output, edge.left);
      put_u64(output, edge.right);
      put_u64(output, material.size());
      if (output.size() > kMaxPackageBytes - material.size()) {
        throw std::invalid_argument("party package exceeds allocation limit");
      }
      output.insert(output.end(), material.begin(), material.end());
    }
  }
  return output;
}

ProtocolIPartyPackage deserialize_party_package(const std::vector<std::uint8_t>& bytes,
                                                int expected_party) {
  if (expected_party < 0 || expected_party > 1 || bytes.size() > kMaxPackageBytes ||
      bytes.size() < kHeaderBytes || bytes[0] != 'M' || bytes[1] != '2' || bytes[2] != 'P' ||
      bytes[3] != 'K' || bytes[4] != 2 || bytes[5] != expected_party || bytes[7] != 0) {
    throw std::invalid_argument("party package header");
  }

  ProtocolIPartyPackage package;
  package.party = bytes[5];
  package.comparison_bits = bytes[6];
  std::size_t offset = 8;
  package.session = get_u64(bytes, offset);
  package.fingerprint = get_u64(bytes, offset);
  const auto n64 = get_u64(bytes, offset);
  const auto k64 = get_u64(bytes, offset);
  if (n64 > std::numeric_limits<std::uint32_t>::max() ||
      k64 > std::numeric_limits<std::uint32_t>::max()) {
    throw std::invalid_argument("party package dimension exceeds uint32");
  }
  package.n = static_cast<std::uint32_t>(n64);
  package.k = static_cast<std::uint32_t>(k64);
  const auto expected_edges = edge_count(n64);
  if (package.k == 0 || package.k > package.n || package.comparison_bits < 34 ||
      package.comparison_bits > 53) {
    throw std::invalid_argument("party package dimensions");
  }

  if (package.n > (bytes.size() - offset) / sizeof(std::uint64_t)) {
    throw std::invalid_argument("party package masks");
  }
  package.node_mask_shares.resize(package.n);
  const auto mask = ring_mask(package.comparison_bits);
  for (auto& share : package.node_mask_shares) {
    share = get_u64(bytes, offset);
    if ((share & ~mask) != 0) {
      throw std::invalid_argument("party package node mask outside ring");
    }
  }

  const auto score_count = get_u64(bytes, offset);
  if (score_count != 0 && score_count != package.n) {
    throw std::invalid_argument("party package score material count");
  }
  if (score_count != 0) {
    package.carry_materials.reserve(package.n);
    package.sign_materials.reserve(package.n);
    for (std::uint32_t slot = 0; slot < package.n; ++slot) {
      package.carry_materials.push_back(get_score_material(bytes, offset, slot, 1, package));
    }
    for (std::uint32_t slot = 0; slot < package.n; ++slot) {
      package.sign_materials.push_back(get_score_material(bytes, offset, slot, 2, package));
    }
  }

  if (expected_edges > kMaxEdges || expected_edges > (bytes.size() - offset) / 41U) {
    throw std::invalid_argument("party package edge count");
  }
  package.edge_materials.reserve(expected_edges);
  std::size_t edge_index = 0;
  for (std::uint32_t left = 0; left < package.n; ++left) {
    for (std::uint32_t right = left + 1; right < package.n; ++right) {
      const auto serialized_left = get_u64(bytes, offset);
      const auto serialized_right = get_u64(bytes, offset);
      const auto material_size = get_u64(bytes, offset);
      if (serialized_left != left || serialized_right != right ||
          serialized_left > std::numeric_limits<std::uint32_t>::max() ||
          serialized_right > std::numeric_limits<std::uint32_t>::max() ||
          material_size > bytes.size() - offset ||
          material_size > std::numeric_limits<std::size_t>::max()) {
        throw std::invalid_argument("party package edge identity or material length");
      }
      const auto material_length = static_cast<std::size_t>(material_size);
      std::vector<std::uint8_t> material_bytes(bytes.begin() + offset,
                                                bytes.begin() + offset + material_length);
      offset += material_length;
      auto material = ProtocolIUcmpPartyMaterial::deserialize(material_bytes);
      ProtocolIEdgePartyMaterial edge(left, right, std::move(material));
      validate_edge(edge, left, right, package);
      package.edge_materials.push_back(std::move(edge));
      ++edge_index;
    }
  }
  if (edge_index != expected_edges || offset != bytes.size()) {
    throw std::invalid_argument("party package trailing bytes or edge count");
  }
  validate_common(package, expected_edges);
  return package;
}
}  // namespace moe_topk
