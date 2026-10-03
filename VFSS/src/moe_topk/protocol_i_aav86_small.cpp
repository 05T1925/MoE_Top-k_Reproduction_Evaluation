#include <moe_topk/protocol_i_aav86_small.h>
#include <moe_topk/protocol_i_pipeline.h>
#include <moe_topk/protocol_i_score_input.h>
#include <moe_topk/protocol_i_transport.h>

#include <FSS/prng.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <sys/random.h>
#include <sys/resource.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace moe_topk {
namespace {

constexpr std::uint32_t kMagic = UINT32_C(0x4d364137);
constexpr std::uint32_t kVersion = 1;
constexpr std::size_t kMaxPackageBytes = 64U * 1024U * 1024U;
constexpr std::uint64_t kMaxDealerBudgetBytes = UINT64_C(512) * 1024U * 1024U;
constexpr std::uint64_t kDealerFixedHeadroomBytes = UINT64_C(64) * 1024U * 1024U;
constexpr std::uint64_t kD128MinVirtualBytes = UINT64_C(640) * 1024U * 1024U;
constexpr std::uint64_t kD128MaxVirtualBytes = UINT64_C(768) * 1024U * 1024U;
std::mutex dealer_mutex;

void require(bool ok, const char* message) {
  if (!ok) throw std::invalid_argument(message);
}
std::uint64_t checked_add(std::uint64_t a, std::uint64_t b) {
  if (b > UINT64_MAX - a) throw std::overflow_error("AAV86 capacity addition overflow");
  return a + b;
}
std::uint64_t checked_mul(std::uint64_t a, std::uint64_t b) {
  if (a && b > UINT64_MAX / a)
    throw std::overflow_error("AAV86 capacity multiplication overflow");
  return a * b;
}
std::uint64_t ring(int bits) {
  require(bits >= 34 && bits <= 53, "AAV86 comparison bits");
  return (UINT64_C(1) << bits) - 1U;
}
std::uint32_t domain(std::uint32_t n) {
  require(n >= 1 && n <= 128, "AAV86 bounded logical_n");
  std::uint32_t d = 2;
  while (d < n) d <<= 1U;
  return d;
}
std::optional<std::uint64_t> read_limit_file(const char* path) {
  std::ifstream file(path);
  if (!file) return std::nullopt;
  std::string token;
  require(static_cast<bool>(file >> token),"AAV86 cgroup limit read");
  if (token == "max") return std::nullopt;
  std::size_t used=0;
  const auto value=std::stoull(token,&used);
  require(used==token.size(),"AAV86 cgroup limit syntax");
  return value;
}
std::uint32_t index_bits(std::uint32_t d) {
  std::uint32_t bits = 0;
  for (auto value = d - 1U; value != 0; value >>= 1U) ++bits;
  return bits;
}
std::size_t edges_per_round(std::uint32_t d) {
  return static_cast<std::size_t>(d) * (d - 1U) / 2U;
}
std::size_t edge_index(std::uint32_t d, std::uint32_t a, std::uint32_t c) {
  require(a < c && c < d, "AAV86 canonical edge");
  return static_cast<std::size_t>(a) * (2U * d - a - 1U) / 2U + (c - a - 1U);
}
void random_bytes(void* output, std::size_t size) {
  auto* cursor = static_cast<std::uint8_t*>(output);
  while (size) {
    const auto count = ::getrandom(cursor, size, 0);
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) throw std::runtime_error("AAV86 dealer entropy");
    cursor += count;
    size -= static_cast<std::size_t>(count);
  }
}
std::uint64_t random_word() {
  std::uint64_t value = 0;
  random_bytes(&value, sizeof(value));
  return value;
}
std::uint64_t random_bounded(std::uint64_t bound) {
  require(bound != 0, "AAV86 random bound");
  const auto cutoff = UINT64_MAX - (UINT64_MAX % bound);
  std::uint64_t value;
  do value = random_word(); while (value >= cutoff);
  return value % bound;
}
ProtocolIPermutation random_permutation(std::uint32_t d) {
  auto result = protocol_i_identity_permutation(d);
  for (auto remaining = d; remaining > 1; --remaining)
    std::swap(result[remaining - 1U], result[static_cast<std::size_t>(random_bounded(remaining))]);
  return result;
}
std::vector<std::uint64_t> random_vector(std::uint32_t d, std::uint64_t mask) {
  std::vector<std::uint64_t> result(d);
  for (auto& value : result) value = random_word() & mask;
  return result;
}
std::vector<std::uint64_t> add_vectors(const std::vector<std::uint64_t>& left,
                                       const std::vector<std::uint64_t>& right,
                                       std::uint64_t mask) {
  require(left.size() == right.size(), "AAV86 vector shape");
  auto output = left;
  for (std::size_t i = 0; i < output.size(); ++i) output[i] = (left[i] + right[i]) & mask;
  return output;
}
std::vector<std::uint64_t> make_e(const ProtocolIPermutation& tau,
                                  const std::vector<std::uint64_t>& peer_a,
                                  const std::vector<std::uint64_t>& h,
                                  std::uint64_t mask, int party) {
  const auto permuted = protocol_i_apply_permutation(tau, peer_a);
  std::vector<std::uint64_t> result(h.size());
  for (std::size_t i = 0; i < result.size(); ++i)
    result[i] = (0U - permuted[i] + (party == 0 ? 0U - h[i] : h[i])) & mask;
  return result;
}

void validate_config(const ProtocolIAav86SmallConfig& c) {
  const auto d = domain(c.logical_n);
  const auto pairs = checked_mul(d, d - 1U) / 2U;
  const auto slots = checked_mul(c.iterations, pairs);
  require(c.session && c.fingerprint && c.material_id && c.k >= 1 && c.k <= c.logical_n &&
              c.iterations >= 1 && c.iterations <= 5 && c.party < 2 &&
              c.material_id <= UINT64_MAX - slots &&
              c.timeout_ms > 0, "AAV86 small config");
}

struct Writer {
  std::vector<std::uint8_t> bytes;
  void u8(std::uint8_t x) { bytes.push_back(x); }
  void u32(std::uint32_t x) {
    for (int shift = 24; shift >= 0; shift -= 8) u8(static_cast<std::uint8_t>(x >> shift));
  }
  void u64(std::uint64_t x) {
    for (int shift = 56; shift >= 0; shift -= 8) u8(static_cast<std::uint8_t>(x >> shift));
  }
  void blob(const std::vector<std::uint8_t>& data) {
    require(data.size() <= kMaxPackageBytes - 4U &&
                bytes.size() <= kMaxPackageBytes - 4U - data.size(),
            "AAV86 package size");
    u32(static_cast<std::uint32_t>(data.size()));
    bytes.insert(bytes.end(), data.begin(), data.end());
  }
  void words(const std::vector<std::uint64_t>& values) {
    u32(static_cast<std::uint32_t>(values.size()));
    for (auto value : values) u64(value);
  }
  void perm(const ProtocolIPermutation& values) {
    u32(static_cast<std::uint32_t>(values.size()));
    for (auto value : values) u32(value);
  }
};
struct Reader {
  const std::vector<std::uint8_t>& bytes;
  std::size_t at = 0;
  std::uint8_t u8() {
    require(at < bytes.size(), "AAV86 truncated u8");
    return bytes[at++];
  }
  std::uint32_t u32() {
    std::uint32_t value = 0;
    for (int i = 0; i < 4; ++i) value = (value << 8U) | u8();
    return value;
  }
  std::uint64_t u64() {
    std::uint64_t value = 0;
    for (int i = 0; i < 8; ++i) value = (value << 8U) | u8();
    return value;
  }
  std::vector<std::uint8_t> blob() {
    const auto size = u32();
    require(size <= bytes.size() - at, "AAV86 truncated blob");
    std::vector<std::uint8_t> result(bytes.begin() + at, bytes.begin() + at + size);
    at += size;
    return result;
  }
  std::vector<std::uint64_t> words(std::size_t expected) {
    require(u32() == expected, "AAV86 word count");
    std::vector<std::uint64_t> result(expected);
    for (auto& value : result) value = u64();
    return result;
  }
  ProtocolIPermutation perm(std::size_t expected) {
    require(u32() == expected, "AAV86 permutation count");
    ProtocolIPermutation result(expected);
    for (auto& value : result) value = u32();
    protocol_i_validate_permutation(result);
    return result;
  }
};

void validate_material(const ProtocolIAav86SmallPartyMaterial& m) {
  const auto d = domain(m.logical_n);
  const auto mask = ring(m.comparison_bits);
  require(m.session && m.fingerprint && m.material_id && m.party < 2 &&
              m.padded_n == d && m.k >= 1 && m.k <= m.logical_n &&
              m.iterations >= 1 && m.iterations <= 5 &&
              m.comparison_bits == 33U + index_bits(d) &&
              m.material_id <= UINT64_MAX - checked_mul(m.iterations, edges_per_round(d)),
          "AAV86 material header");
  for (const auto* permutation : {&m.forward_sigma, &m.forward_tau,
                                   &m.inverse_sigma, &m.inverse_tau}) {
    protocol_i_validate_permutation(*permutation);
    require(permutation->size() == d, "AAV86 permutation shape");
  }
  for (const auto* values : {&m.forward_a, &m.forward_e, &m.inverse_a, &m.inverse_e}) {
    require(values->size() == d, "AAV86 mask vector shape");
    for (auto value : *values) require((value & ~mask) == 0, "AAV86 mask width");
  }
  require(m.node_mask_shares.size() == static_cast<std::size_t>(m.iterations) * d &&
              m.edge_keys.size() == static_cast<std::size_t>(m.iterations) * edges_per_round(d),
          "AAV86 pool shape");
  for (auto value : m.node_mask_shares) require((value & ~mask) == 0, "AAV86 node mask width");
  for (const auto& key : m.edge_keys)
    require(key.party_id() == m.party && key.comparison_bits() == m.comparison_bits,
            "AAV86 edge key binding");
  const auto& score = m.score_materials;
  require(score.party == m.party && score.session == m.session &&
              score.fingerprint == m.fingerprint && score.n == d && score.k == m.k &&
              score.comparison_bits == m.comparison_bits &&
              score.carry_materials.size() == d && score.sign_materials.size() == d,
          "AAV86 score material binding");
  for (std::size_t stage = 0; stage < 2; ++stage) {
    const auto& items = stage == 0 ? score.carry_materials : score.sign_materials;
    for (std::size_t slot = 0; slot < d; ++slot) {
      const auto& item = items[slot];
      require(item.slot == slot && item.stage == stage + 1 &&
                  item.material.party_id() == m.party && item.material.comparison_bits() == 34 &&
                  item.left_mask_share < (UINT64_C(1) << 34) &&
                  item.right_mask_share < (UINT64_C(1) << 34),
              "AAV86 score key binding");
    }
  }
}

}  // namespace

ProtocolIAav86SmallCapacity protocol_i_aav86_small_capacity_shape(
    std::uint32_t logical_n, std::uint32_t iterations) {
  require(logical_n >= 1 && logical_n <= 1000000U &&
              iterations >= 1 && iterations <= 5,
          "AAV86 shape n/r range");
  ProtocolIAav86SmallCapacity result;
  std::uint64_t padded = 2;
  while (padded < logical_n) padded = checked_mul(padded, 2U);
  result.padded_n = static_cast<std::uint32_t>(padded);
  result.comparison_bits = static_cast<std::uint8_t>(33U + index_bits(result.padded_n));
  result.pairs_per_iteration = checked_mul(result.padded_n, result.padded_n - 1U) / 2U;
  result.total_pair_slots = checked_mul(iterations, result.pairs_per_iteration);
  const auto d_term = checked_mul(result.padded_n,
      checked_add(1844U, checked_mul(8U, iterations)));
  const auto edge_term = checked_mul(result.total_pair_slots,
      checked_add(81U, checked_mul(24U, result.comparison_bits)));
  result.party_package_bytes = checked_add(checked_add(114U,d_term),edge_term);
  // Two in-memory party pools, key-vector objects/allocator overhead, one
  // serialized package at a time, transient DCF pairs, and process overhead.
  // 8x the exact wire size plus 64 MiB is deliberately above those terms for
  // this GNU/glibc bounded-D build; it is a resource admission budget, not a
  // portable C++ allocator theorem or a prediction of observed peak RSS.
  result.dealer_memory_budget_bytes = checked_add(
      checked_mul(8U,result.party_package_bytes),kDealerFixedHeadroomBytes);
  return result;
}

ProtocolIAav86SmallCapacityAssessment protocol_i_aav86_small_assess_capacity(
    const ProtocolIAav86SmallConfig& config) {
  ProtocolIAav86SmallCapacityAssessment result;
  result.shape=protocol_i_aav86_small_capacity_shape(config.logical_n,config.iterations);
  result.hard_cap=result.shape.padded_n>128U;
  result.package_limit=result.shape.party_package_bytes>kMaxPackageBytes;
  result.budget_limit=result.shape.dealer_memory_budget_bytes>kMaxDealerBudgetBytes;
  result.material_id_limit=config.material_id==0 ||
      config.material_id>UINT64_MAX-result.shape.total_pair_slots;
  const auto pages = ::sysconf(_SC_AVPHYS_PAGES);
  const auto page_bytes = ::sysconf(_SC_PAGESIZE);
  require(pages > 0 && page_bytes > 0,"AAV86 available memory unavailable");
  result.shape.available_memory_bytes=checked_mul(
      static_cast<std::uint64_t>(pages),static_cast<std::uint64_t>(page_bytes));
  for (const auto [max_path,current_path]:{
           std::pair{"/sys/fs/cgroup/memory.max","/sys/fs/cgroup/memory.current"},
           std::pair{"/sys/fs/cgroup/memory/memory.limit_in_bytes",
                     "/sys/fs/cgroup/memory/memory.usage_in_bytes"}}) {
    const auto limit=read_limit_file(max_path);
    const auto used=read_limit_file(current_path);
    if (limit && used) {
      require(*used<=*limit,"AAV86 cgroup memory exceeded");
      result.shape.available_memory_bytes=std::min(
          result.shape.available_memory_bytes,*limit-*used);
    }
  }
  result.memory_limit=checked_mul(result.shape.padded_n==128U?3U:2U,
      result.shape.dealer_memory_budget_bytes)>
      result.shape.available_memory_bytes;
  if (result.shape.padded_n == 128U) {
    struct rlimit virtual_limit{};
    require(::getrlimit(RLIMIT_AS,&virtual_limit)==0,"AAV86 address-space limit query");
    result.process_limit=virtual_limit.rlim_cur==RLIM_INFINITY ||
        virtual_limit.rlim_cur<kD128MinVirtualBytes ||
        virtual_limit.rlim_cur>kD128MaxVirtualBytes;
  }
  return result;
}

ProtocolIAav86SmallCapacity protocol_i_aav86_small_preflight(
    const ProtocolIAav86SmallConfig& config) {
  const auto assessment=protocol_i_aav86_small_assess_capacity(config);
  require(!assessment.hard_cap,"AAV86 preflight hard cap D>128");
  require(!assessment.package_limit,"AAV86 preflight package exceeds 64 MiB");
  require(!assessment.budget_limit,"AAV86 preflight dealer budget exceeds 512 MiB");
  require(!assessment.memory_limit,"AAV86 preflight insufficient available memory");
  require(!assessment.process_limit,"AAV86 preflight D128 requires 640-768 MiB RLIMIT_AS");
  require(!assessment.material_id_limit,"AAV86 preflight material ID range");
  validate_config(config);
  return assessment.shape;
}

ProtocolIAav86SmallDealerOutput protocol_i_aav86_small_dealer_generate(
    const ProtocolIAav86SmallConfig& config) {
  const auto capacity = protocol_i_aav86_small_preflight(config);
  std::lock_guard<std::mutex> guard(dealer_mutex);
  std::uint64_t seed_words[2]{};
  random_bytes(seed_words, sizeof(seed_words));
  FSSConfig::prngs[0].SetSeed(osuCrypto::toBlock(seed_words[0], seed_words[1]));
  const auto d = capacity.padded_n;
  const auto bits = capacity.comparison_bits;
  const auto mask = ring(bits), score_mask = ring(34);
  const auto pi = random_permutation(d);
  const auto pi_inverse = protocol_i_inverse_permutation(pi);
  const auto sigma0 = random_permutation(d), sigma1 = random_permutation(d);
  const auto tau0 = protocol_i_compose_permutation(pi, protocol_i_inverse_permutation(sigma1));
  const auto tau1 = protocol_i_compose_permutation(pi, protocol_i_inverse_permutation(sigma0));
  const auto gamma0 = random_permutation(d), gamma1 = random_permutation(d);
  const auto delta0 = protocol_i_compose_permutation(pi_inverse, protocol_i_inverse_permutation(gamma1));
  const auto delta1 = protocol_i_compose_permutation(pi_inverse, protocol_i_inverse_permutation(gamma0));
  const auto a0 = random_vector(d, mask), a1 = random_vector(d, mask);
  const auto h = random_vector(d, mask);
  const auto inverse_a0 = random_vector(d, mask), inverse_a1 = random_vector(d, mask);
  const auto inverse_h = random_vector(d, mask);
  const auto e0 = make_e(tau0, a1, h, mask, 0);
  const auto e1 = make_e(tau1, a0, h, mask, 1);
  const auto inverse_e0 = make_e(delta0, inverse_a1, inverse_h, mask, 0);
  const auto inverse_e1 = make_e(delta1, inverse_a0, inverse_h, mask, 1);
  ProtocolIAav86SmallDealerOutput out;
  const auto pivot_lo = random_word(), pivot_hi = random_word();
  auto init = [&](ProtocolIAav86SmallPartyMaterial& m, int party) {
    m.session = config.session; m.fingerprint = config.fingerprint;
    m.material_id = config.material_id; m.pivot_seed_lo = pivot_lo; m.pivot_seed_hi = pivot_hi;
    m.logical_n = config.logical_n; m.padded_n = d; m.k = config.k;
    m.iterations = config.iterations; m.comparison_bits = bits; m.party = party;
    m.forward_sigma = party == 0 ? sigma0 : sigma1;
    m.forward_tau = party == 0 ? tau0 : tau1;
    m.inverse_sigma = party == 0 ? gamma0 : gamma1;
    m.inverse_tau = party == 0 ? delta0 : delta1;
    m.forward_a = party == 0 ? a0 : a1;
    m.forward_e = party == 0 ? e0 : e1;
    m.inverse_a = party == 0 ? inverse_a0 : inverse_a1;
    m.inverse_e = party == 0 ? inverse_e0 : inverse_e1;
    m.score_materials.session = config.session;
    m.score_materials.fingerprint = config.fingerprint;
    m.score_materials.party = party;
    m.score_materials.n = d; m.score_materials.k = config.k;
    m.score_materials.comparison_bits = bits;
    m.node_mask_shares.reserve(static_cast<std::size_t>(config.iterations) * d);
    m.edge_keys.reserve(static_cast<std::size_t>(config.iterations) * edges_per_round(d));
  };
  init(out.party0, 0); init(out.party1, 1);
  for (std::uint32_t stage = 1; stage <= 2; ++stage) {
    for (std::uint32_t slot = 0; slot < d; ++slot) {
      const auto left = random_word() & score_mask, right = random_word() & score_mask;
      const auto left0 = random_word() & score_mask, right0 = random_word() & score_mask;
      ProtocolIUcmpMaterial material(34, left, right);
      auto key0 = material.export_party_material(0), key1 = material.export_party_material(1);
      auto& v0 = stage == 1 ? out.party0.score_materials.carry_materials
                            : out.party0.score_materials.sign_materials;
      auto& v1 = stage == 1 ? out.party1.score_materials.carry_materials
                            : out.party1.score_materials.sign_materials;
      v0.emplace_back(slot, stage, left0, right0, std::move(key0));
      v1.emplace_back(slot, stage, (left - left0) & score_mask,
                      (right - right0) & score_mask, std::move(key1));
    }
  }
  for (std::uint32_t t = 0; t < config.iterations; ++t) {
    auto full = random_vector(d, mask);
    for (std::uint32_t a = 0; a < d; ++a) {
      const auto share0 = random_word() & mask;
      out.party0.node_mask_shares.push_back(share0);
      out.party1.node_mask_shares.push_back((full[a] - share0) & mask);
    }
    for (std::uint32_t a = 0; a < d; ++a)
      for (std::uint32_t c = a + 1U; c < d; ++c) {
        ProtocolIUcmpMaterial material(bits, full[a], full[c]);
        out.party0.edge_keys.push_back(material.export_party_material(0));
        out.party1.edge_keys.push_back(material.export_party_material(1));
      }
  }
  validate_material(out.party0);
  validate_material(out.party1);
  return out;
}

std::vector<std::uint8_t> protocol_i_aav86_small_serialize_party_material(
    const ProtocolIAav86SmallPartyMaterial& m) {
  validate_material(m);
  Writer w;
  w.u32(kMagic); w.u32(kVersion);
  w.u64(m.session); w.u64(m.fingerprint); w.u64(m.material_id);
  w.u32(m.logical_n); w.u32(m.padded_n); w.u32(m.k); w.u32(m.iterations);
  w.u8(m.comparison_bits); w.u8(m.party);
  w.u64(m.pivot_seed_lo); w.u64(m.pivot_seed_hi);
  w.perm(m.forward_sigma); w.perm(m.forward_tau);
  w.perm(m.inverse_sigma); w.perm(m.inverse_tau);
  w.words(m.forward_a); w.words(m.forward_e);
  w.words(m.inverse_a); w.words(m.inverse_e);
  w.words(m.node_mask_shares);
  for (std::uint32_t stage = 1; stage <= 2; ++stage) {
    const auto& items = stage == 1 ? m.score_materials.carry_materials
                                   : m.score_materials.sign_materials;
    w.u32(static_cast<std::uint32_t>(items.size()));
    for (const auto& item : items) {
      w.u32(item.slot); w.u8(item.stage);
      w.u64(item.left_mask_share); w.u64(item.right_mask_share);
      w.blob(item.material.serialize());
    }
  }
  w.u32(static_cast<std::uint32_t>(m.edge_keys.size()));
  std::size_t index = 0;
  for (std::uint32_t t = 0; t < m.iterations; ++t)
    for (std::uint32_t a = 0; a < m.padded_n; ++a)
      for (std::uint32_t c = a + 1U; c < m.padded_n; ++c) {
        w.u32(t); w.u32(a); w.u32(c);
        w.u64(m.material_id + index);
        w.blob(m.edge_keys[index++].serialize());
      }
  require(w.bytes.size() <= kMaxPackageBytes, "AAV86 serialized package limit");
  return std::move(w.bytes);
}

ProtocolIAav86SmallPartyMaterial protocol_i_aav86_small_deserialize_party_material(
    const std::vector<std::uint8_t>& bytes, int expected_party,
    const ProtocolIAav86SmallConfig& expected_config) {
  validate_config(expected_config);
  require(expected_party == expected_config.party && bytes.size() <= kMaxPackageBytes,
          "AAV86 expected party/package size");
  Reader r{bytes};
  require(r.u32() == kMagic && r.u32() == kVersion, "AAV86 package magic/version");
  ProtocolIAav86SmallPartyMaterial m;
  m.session = r.u64(); m.fingerprint = r.u64(); m.material_id = r.u64();
  m.logical_n = r.u32(); m.padded_n = r.u32(); m.k = r.u32(); m.iterations = r.u32();
  m.comparison_bits = r.u8(); m.party = r.u8();
  m.pivot_seed_lo = r.u64(); m.pivot_seed_hi = r.u64();
  require(m.session == expected_config.session &&
              m.fingerprint == expected_config.fingerprint &&
              m.material_id == expected_config.material_id &&
              m.logical_n == expected_config.logical_n &&
              m.padded_n == domain(expected_config.logical_n) &&
              m.k == expected_config.k && m.iterations == expected_config.iterations &&
              m.comparison_bits == 33U + index_bits(m.padded_n) &&
              m.party == expected_party,
          "AAV86 package binding");
  const auto d = m.padded_n;
  m.forward_sigma = r.perm(d); m.forward_tau = r.perm(d);
  m.inverse_sigma = r.perm(d); m.inverse_tau = r.perm(d);
  m.forward_a = r.words(d); m.forward_e = r.words(d);
  m.inverse_a = r.words(d); m.inverse_e = r.words(d);
  m.node_mask_shares = r.words(static_cast<std::size_t>(m.iterations) * d);
  auto& score = m.score_materials;
  score.session = m.session; score.fingerprint = m.fingerprint;
  score.party = m.party; score.n = d; score.k = m.k;
  score.comparison_bits = m.comparison_bits;
  for (std::uint32_t stage = 1; stage <= 2; ++stage) {
    require(r.u32() == d, "AAV86 score material count");
    auto& items = stage == 1 ? score.carry_materials : score.sign_materials;
    for (std::uint32_t slot = 0; slot < d; ++slot) {
      const auto received_slot = r.u32();
      const auto received_stage = r.u8();
      const auto left = r.u64(), right = r.u64();
      auto key = ProtocolIUcmpPartyMaterial::deserialize(r.blob());
      require(received_slot == slot && received_stage == stage,
              "AAV86 score slot/stage");
      items.emplace_back(slot, static_cast<std::uint8_t>(stage), left, right, std::move(key));
    }
  }
  const auto expected_edges = static_cast<std::size_t>(m.iterations) * edges_per_round(d);
  require(r.u32() == expected_edges, "AAV86 edge material count");
  m.edge_keys.reserve(expected_edges);
  std::size_t index = 0;
  for (std::uint32_t t = 0; t < m.iterations; ++t)
    for (std::uint32_t a = 0; a < d; ++a)
      for (std::uint32_t c = a + 1U; c < d; ++c) {
        require(r.u32() == t && r.u32() == a && r.u32() == c &&
                    r.u64() == m.material_id + index,
                "AAV86 edge round/pair/id");
        m.edge_keys.push_back(ProtocolIUcmpPartyMaterial::deserialize(r.blob()));
        ++index;
      }
  require(r.at == bytes.size(), "AAV86 package trailing bytes");
  validate_material(m);
  return m;
}

namespace {
std::vector<std::uint8_t> encode_words(const std::vector<std::uint64_t>& words) {
  std::vector<std::uint8_t> bytes;
  bytes.reserve(words.size() * sizeof(std::uint64_t));
  for (auto value : words)
    for (int shift = 56; shift >= 0; shift -= 8)
      bytes.push_back(static_cast<std::uint8_t>(value >> shift));
  return bytes;
}
std::vector<std::uint64_t> decode_words(const std::vector<std::uint8_t>& bytes,
                                        std::size_t expected) {
  require(bytes.size() == expected * sizeof(std::uint64_t), "AAV86 frame word count");
  std::vector<std::uint64_t> words(expected);
  for (std::size_t i = 0; i < expected; ++i)
    for (int j = 0; j < 8; ++j)
      words[i] = (words[i] << 8U) | bytes[8U * i + j];
  return words;
}
std::vector<std::uint64_t> exchange_words(
    int fd, const ProtocolIAav86SmallPartyMaterial& m,
    int timeout_ms, std::uint8_t phase,
    const std::vector<std::uint64_t>& local,
    ProtocolIAav86SmallMetrics& metrics, std::uint64_t* stage_sent) {
  ProtocolIFramedChannel channel(fd,
      {m.session, m.fingerprint, m.padded_n, m.k, m.comparison_bits,
       m.party, static_cast<std::uint8_t>(1U - m.party), phase, 7}, timeout_ms);
  const auto outbound = encode_words(local);
  std::vector<std::uint8_t> incoming;
  if (m.party == 0) { channel.send(outbound); incoming = channel.receive(); }
  else { incoming = channel.receive(); channel.send(outbound); }
  metrics.online_sent_bytes += channel.sent_bytes();
  metrics.online_received_bytes += channel.received_bytes();
  *stage_sent += channel.sent_bytes();
  metrics.message_trace.push_back({phase,channel.sent_bytes(),channel.received_bytes()});
  return decode_words(incoming, local.size());
}

void durable_claim(const ProtocolIAav86SmallConfig& c) {
  require(!c.durable_claim_directory.empty(), "AAV86 durable claim directory");
  const int directory = ::open(c.durable_claim_directory.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
  if (directory < 0) throw std::runtime_error("AAV86 durable claim directory unavailable");
  const auto name = std::string("m6a7-") + std::to_string(c.session) + "-" +
                    std::to_string(c.material_id) + "-p" + std::to_string(c.party);
  const int file = ::openat(directory, name.c_str(),
                            O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
  if (file < 0) {
    ::close(directory);
    throw std::runtime_error("AAV86 material already claimed or claim failed");
  }
  const auto marker = std::to_string(c.fingerprint) + ":" + std::to_string(c.logical_n) +
                      ":" + std::to_string(c.k) + ":" + std::to_string(c.iterations) + "\n";
  const auto written = ::write(file, marker.data(), marker.size());
  const bool okay = written == static_cast<ssize_t>(marker.size()) &&
                    ::fsync(file) == 0 && ::fsync(directory) == 0;
  ::close(file);
  ::close(directory);
  if (!okay) throw std::runtime_error("AAV86 durable claim sync failed");
}

struct Node {
  std::vector<std::uint32_t> vertices, pivots, pivot_order;
  std::vector<std::vector<std::uint32_t>> buckets;
  std::vector<int> children;
  std::uint32_t depth = 0;
};
std::uint32_t ceil_root(std::uint32_t m, std::uint32_t depth) {
  require(m >= 2 && depth >= 1, "AAV86 root arguments");
  for (std::uint32_t t = 1; t <= m; ++t) {
    std::uint64_t power = 1;
    for (std::uint32_t i = 0; i < depth; ++i) power = checked_mul(power, t);
    if (power >= m) return t;
  }
  throw std::logic_error("AAV86 root unreachable");
}
std::uint64_t prg_bounded(osuCrypto::PRNG& prng, std::uint64_t bound) {
  require(bound != 0, "AAV86 pivot bound");
  const auto cutoff = UINT64_MAX - (UINT64_MAX % bound);
  std::uint64_t value;
  do value = prng.get<std::uint64_t>(); while (value >= cutoff);
  return value % bound;
}
void choose_pivots(Node& node, osuCrypto::PRNG& prng) {
  auto remaining = node.vertices;
  const auto q = ceil_root(static_cast<std::uint32_t>(remaining.size()), node.depth) - 1U;
  require(q >= 1 && q < remaining.size(), "AAV86 pivot count");
  for (std::uint32_t i = 0; i < q; ++i) {
    const auto chosen = static_cast<std::size_t>(prg_bounded(prng, remaining.size() - i));
    node.pivots.push_back(remaining[chosen]);
    std::swap(remaining[chosen], remaining[remaining.size() - 1U - i]);
  }
}
std::vector<std::uint32_t> flatten(const std::vector<Node>& nodes, int id) {
  const auto& node = nodes.at(static_cast<std::size_t>(id));
  if (node.vertices.size() == 1) return node.vertices;
  std::vector<std::uint32_t> result;
  for (std::size_t i = 0; i <= node.pivot_order.size(); ++i) {
    if (node.depth == 1) {
      require(node.buckets[i].size() <= 1, "AAV86 terminal bucket");
      result.insert(result.end(), node.buckets[i].begin(), node.buckets[i].end());
    } else if (node.children[i] >= 0) {
      auto child = flatten(nodes, node.children[i]);
      result.insert(result.end(), child.begin(), child.end());
    }
    if (i < node.pivot_order.size()) result.push_back(node.pivot_order[i]);
  }
  require(result.size() == node.vertices.size(), "AAV86 flatten shape");
  return result;
}
}  // namespace

ProtocolIAav86SmallOutput protocol_i_aav86_small_party(
    const ProtocolIAav86SmallConfig& config,
    ProtocolIAav86SmallPartyMaterial&& material,
    const std::vector<std::uint32_t>& raw_score_share,
    const std::array<int, 2>& score_fds,
    const std::vector<int>& core_fds, int inverse_fd) {
  validate_config(config);
  validate_material(material);
  const auto d = domain(config.logical_n);
  require(config.party == material.party &&
              config.session == material.session &&
              config.fingerprint == material.fingerprint &&
              config.material_id == material.material_id &&
              config.logical_n == material.logical_n &&
              config.k == material.k && config.iterations == material.iterations &&
              raw_score_share.size() == config.logical_n &&
              core_fds.size() == 2U * config.iterations + 1U &&
              score_fds[0] >= 0 && score_fds[1] >= 0 && inverse_fd >= 0,
          "AAV86 party binding/channels");
  for (const auto fd : core_fds) require(fd >= 0, "AAV86 core fd");
  // Claim before the first online input-dependent message. A crash consumes
  // the package rather than allowing an old key to be reloaded.
  durable_claim(config);
  resetDCFOnlinePrgCalls();
  const auto bits = material.comparison_bits;
  const auto mask = ring(bits);
  ProtocolIAav86SmallOutput output;
  auto& metrics = output.metrics;
  metrics.pool_slots_per_party =
      static_cast<std::uint64_t>(config.iterations) * edges_per_round(d);
  metrics.causal_rounds = 2U * config.iterations + 4U;
  metrics.active_edges_by_iteration.reserve(config.iterations);
  metrics.active_vertices_by_iteration.reserve(config.iterations);
  metrics.ca_prg_calls_by_iteration.reserve(config.iterations);
  ProtocolIScoreInputMetrics score_metrics;
  const auto key_shares = protocol_i_raw_score_input_party(
      {config.session, config.fingerprint, config.logical_n, d, config.k,
       static_cast<std::uint8_t>(index_bits(d)), bits, config.party, config.timeout_ms},
      material.score_materials, raw_score_share, score_fds, &score_metrics);
  metrics.score_prg_calls = readDCFOnlinePrgCalls();
  metrics.score_sent_bytes = score_metrics.carry_sent_bytes + score_metrics.sign_sent_bytes;
  metrics.score_dcf_evaluations = score_metrics.raw_dcf_calls;
  metrics.dcf_evaluations = score_metrics.raw_dcf_calls;
  metrics.online_sent_bytes += metrics.score_sent_bytes;
  metrics.online_received_bytes +=
      score_metrics.carry_received_bytes + score_metrics.sign_received_bytes;
  metrics.message_trace.push_back({4,score_metrics.carry_sent_bytes,
                                   score_metrics.carry_received_bytes});
  metrics.message_trace.push_back({5,score_metrics.sign_sent_bytes,
                                   score_metrics.sign_received_bytes});
  const auto first_outbound = add_vectors(
      protocol_i_apply_permutation(material.forward_sigma, key_shares),
      material.forward_a, mask);
  auto peer_first = exchange_words(core_fds[0], material, config.timeout_ms, 9,
                                   first_outbound, metrics, &metrics.core_sent_bytes);
  auto shuffled_share = add_vectors(
      protocol_i_apply_permutation(material.forward_tau, peer_first),
      material.forward_e, mask);
  osuCrypto::PRNG public_prg;
  public_prg.SetSeed(osuCrypto::toBlock(material.pivot_seed_lo, material.pivot_seed_hi));
  std::vector<Node> nodes;
  Node root;
  root.depth = config.iterations;
  root.vertices.resize(d);
  for (std::uint32_t a = 0; a < d; ++a) root.vertices[a] = a;
  nodes.push_back(std::move(root));
  std::vector<int> active{0};
  std::vector<bool> consumed(material.edge_keys.size(), false);
  for (std::uint32_t t = 0; t < config.iterations; ++t) {
    std::vector<std::uint64_t> local_open(d);
    for (std::uint32_t a = 0; a < d; ++a)
      local_open[a] = (shuffled_share[a] + material.node_mask_shares[t * d + a]) & mask;
    const auto peer_open = exchange_words(core_fds[1U + 2U * t], material,
        config.timeout_ms, static_cast<std::uint8_t>(10U + 2U * t),
        local_open, metrics, &metrics.core_sent_bytes);
    std::vector<std::uint64_t> opened(d);
    for (std::uint32_t a = 0; a < d; ++a) {
      require((peer_open[a] & ~mask) == 0, "AAV86 masked open width");
      opened[a] = (local_open[a] + peer_open[a]) & mask;
    }
    std::set<std::pair<std::uint32_t, std::uint32_t>> graph;
    for (const auto id : active) {
      auto& node = nodes.at(static_cast<std::size_t>(id));
      require(node.depth == config.iterations - t && node.vertices.size() > 1,
              "AAV86 active node depth");
      choose_pivots(node, public_prg);
      std::set<std::uint32_t> pivots(node.pivots.begin(), node.pivots.end());
      for (std::size_t i = 0; i < node.vertices.size(); ++i)
        for (std::size_t j = i + 1; j < node.vertices.size(); ++j) {
          const auto a = node.vertices[i], c = node.vertices[j];
          if (pivots.count(a) || pivots.count(c))
            graph.emplace(std::min(a, c), std::max(a, c));
        }
    }
    metrics.active_edges_by_iteration.push_back(graph.size());
    metrics.active_edges += graph.size();
    std::set<std::uint32_t> graph_vertices;
    for (const auto& pair : graph) {
      graph_vertices.insert(pair.first);
      graph_vertices.insert(pair.second);
    }
    metrics.active_vertices_by_iteration.push_back(graph_vertices.size());
    metrics.active_vertices += graph_vertices.size();
    const auto prg_before_round = readDCFOnlinePrgCalls();
    std::vector<std::uint64_t> rank_share(d);
    for (const auto& pair : graph) {
      const auto a = pair.first, c = pair.second;
      const auto slot = t * edges_per_round(d) + edge_index(d, a, c);
      require(slot < material.edge_keys.size() && !consumed[slot], "AAV86 edge reuse");
      consumed[slot] = true;
      metrics.edge_trace.push_back({t,a,c,material.material_id+slot});
      const auto lt = material.edge_keys[slot].eval_strict_lt(opened[a], opened[c]);
      rank_share[a] += (config.party == 0 ? 1U : 0U) - lt;
      rank_share[c] += lt;
      metrics.ca_dcf_evaluations += 2U;
      metrics.dcf_evaluations += 2U;
    }
    metrics.ca_prg_calls_by_iteration.push_back(
        readDCFOnlinePrgCalls() - prg_before_round);
    const auto rank_mask = (UINT64_C(1) << index_bits(d)) - 1U;
    for (auto& value : rank_share) value &= rank_mask;
    const auto peer_rank = exchange_words(core_fds[2U + 2U * t], material,
        config.timeout_ms, static_cast<std::uint8_t>(11U + 2U * t),
        rank_share, metrics, &metrics.core_sent_bytes);
    std::vector<std::uint64_t> public_rank(d);
    for (std::uint32_t a = 0; a < d; ++a) {
      require((peer_rank[a] & ~rank_mask) == 0, "AAV86 rank share width");
      public_rank[a] = (rank_share[a] + peer_rank[a]) & rank_mask;
    }
    std::vector<int> next_active;
    for (const auto id : active) {
      const auto vertices = nodes[static_cast<std::size_t>(id)].vertices;
      const auto pivots = nodes[static_cast<std::size_t>(id)].pivots;
      const auto depth = nodes[static_cast<std::size_t>(id)].depth;
      auto pivot_order = pivots;
      std::sort(pivot_order.begin(), pivot_order.end(),
                [&](auto a, auto c) { return public_rank[a] < public_rank[c]; });
      for (std::size_t i = 0; i < pivot_order.size(); ++i) {
        require(public_rank[pivot_order[i]] < vertices.size(), "AAV86 pivot rank range");
        if (i) require(public_rank[pivot_order[i-1]] < public_rank[pivot_order[i]],
                       "AAV86 pivot rank uniqueness");
      }
      std::set<std::uint32_t> pivot_set(pivots.begin(), pivots.end());
      std::vector<std::vector<std::uint32_t>> buckets(pivots.size() + 1U);
      for (const auto handle : vertices) if (!pivot_set.count(handle)) {
        require(public_rank[handle] <= pivots.size(), "AAV86 bucket rank");
        buckets[static_cast<std::size_t>(public_rank[handle])].push_back(handle);
      }
      auto& node = nodes[static_cast<std::size_t>(id)];
      node.pivot_order = std::move(pivot_order);
      node.buckets = std::move(buckets);
      node.children.assign(node.buckets.size(), -1);
      if (depth > 1) {
        const auto bucket_count = node.children.size();
        for (std::size_t i = 0; i < bucket_count; ++i) {
          const auto bucket = nodes[static_cast<std::size_t>(id)].buckets[i];
          if (bucket.empty()) continue;
          Node child;
          child.vertices = bucket;
          child.depth = depth - 1U;
          const auto child_id = static_cast<int>(nodes.size());
          nodes.push_back(std::move(child));
          nodes[static_cast<std::size_t>(id)].children[i] = child_id;
          if (bucket.size() > 1) next_active.push_back(child_id);
        }
      }
    }
    active = std::move(next_active);
  }
  const auto sorted = flatten(nodes, 0);
  require(sorted.size() == d, "AAV86 full sort shape");
  std::vector<bool> seen(d);
  std::vector<std::uint64_t> carrier(d);
  for (std::size_t position = 0; position < sorted.size(); ++position) {
    const auto handle = sorted[position];
    require(handle < d && !seen[handle], "AAV86 full sort permutation");
    seen[handle] = true;
    carrier[handle] = (config.party == 0 && position < config.k) ? 1U : 0U;
  }
  const auto inverse_first = add_vectors(
      protocol_i_apply_permutation(material.inverse_sigma, carrier),
      material.inverse_a, mask);
  const auto prg_before_inverse = readDCFOnlinePrgCalls();
  const auto inverse_peer = exchange_words(inverse_fd, material, config.timeout_ms, 40,
                                           inverse_first, metrics, &metrics.inverse_sent_bytes);
  const auto original_share = add_vectors(
      protocol_i_apply_permutation(material.inverse_tau, inverse_peer),
      material.inverse_e, mask);
  metrics.online_prg_calls = readDCFOnlinePrgCalls();
  metrics.ca_prg_calls = metrics.online_prg_calls - metrics.score_prg_calls;
  metrics.inverse_prg_calls = metrics.online_prg_calls - prg_before_inverse;
  metrics.ca_prg_calls -= metrics.inverse_prg_calls;
  output.xor_mask_share.resize(config.logical_n);
  for (std::size_t j = 0; j < output.xor_mask_share.size(); ++j)
    output.xor_mask_share[j] = static_cast<std::uint8_t>(original_share[j] & 1U);
  return output;
}
}  // namespace moe_topk
