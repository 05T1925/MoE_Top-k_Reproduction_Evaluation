#include <moe_topk/protocol_iii_aav86_small.h>

#include <FSS/comms.h>
#include <FSS/dpf.h>
#include <FSS/prng.h>
#include <moe_topk/protocol_i_score_input.h>
#include <moe_topk/protocol_i_transport.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mutex>
#include <omp.h>
#include <set>
#include <stdexcept>
#include <string>
#include <sys/random.h>
#include <fcntl.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace moe_topk {
namespace {

constexpr std::uint32_t kMagic = UINT32_C(0x4d364138);  // M6A8
constexpr std::uint8_t kVersion = 1;
constexpr std::size_t kMaxPackageBytes = 64U * 1024U * 1024U;
constexpr std::uint8_t kFrameType = 8;
constexpr int kDpfOutputBits = 64;
std::mutex dealer_mutex;

void require(bool ok, const char* message) {
  if (!ok) throw std::invalid_argument(message);
}

std::uint64_t checked_mul(std::uint64_t a, std::uint64_t b) {
  if (a && b > UINT64_MAX / a)
    throw std::overflow_error("III+AAV86 shape multiplication overflow");
  return a * b;
}

std::uint64_t edge_slots(std::uint32_t d, std::uint32_t r) {
  return checked_mul(r, checked_mul(d, d - 1U) / 2U);
}

std::uint64_t rank_mask(std::uint8_t bits) {
  require(bits >= 1U && bits < 64U, "III+AAV86 rank width");
  return (UINT64_C(1) << bits) - 1U;
}

std::uint64_t comparison_mask(std::uint8_t bits) {
  require(bits >= 34U && bits <= 53U, "III+AAV86 comparison width");
  return (UINT64_C(1) << bits) - 1U;
}

void random_bytes(void* output, std::size_t size) {
  auto* cursor = static_cast<std::uint8_t*>(output);
  while (size) {
    const auto count = ::getrandom(cursor, size, 0);
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) throw std::runtime_error("III+AAV86 OS entropy");
    cursor += count;
    size -= static_cast<std::size_t>(count);
  }
}

std::uint64_t random_word() {
  std::uint64_t result = 0;
  random_bytes(&result, sizeof(result));
  return result;
}

void put(std::vector<std::uint8_t>& out, std::uint64_t value,
         std::size_t width) {
  require(width <= kMaxPackageBytes && out.size() <= kMaxPackageBytes - width,
          "III+AAV86 package size");
  for (std::size_t i = width; i != 0; --i)
    out.push_back(static_cast<std::uint8_t>(value >> (8U * (i - 1U))));
}

std::uint64_t get(const std::vector<std::uint8_t>& in, std::size_t& at,
                  std::size_t width) {
  require(at <= in.size() && width <= in.size() - at,
          "III+AAV86 truncated package");
  std::uint64_t value = 0;
  for (std::size_t i = 0; i < width; ++i) value = (value << 8U) | in[at++];
  return value;
}

std::vector<std::uint8_t> take(const std::vector<std::uint8_t>& in,
                               std::size_t& at, std::size_t size) {
  require(at <= in.size() && size <= in.size() - at,
          "III+AAV86 truncated section");
  std::vector<std::uint8_t> result(in.begin() + static_cast<std::ptrdiff_t>(at),
                                   in.begin() + static_cast<std::ptrdiff_t>(at + size));
  at += size;
  return result;
}

void append(std::vector<std::uint8_t>& out,
            const std::vector<std::uint8_t>& data) {
  require(data.size() <= kMaxPackageBytes - out.size(),
          "III+AAV86 package size");
  out.insert(out.end(), data.begin(), data.end());
}

ProtocolIAav86SmallConfig base_config(
    const ProtocolIIIAav86SmallConfig& c) {
  ProtocolIAav86SmallConfig result;
  result.session = c.session;
  result.fingerprint = c.fingerprint;
  result.material_id = c.material_id;
  result.logical_n = c.logical_n;
  result.k = c.k;
  result.iterations = c.iterations;
  result.party = c.party;
  result.timeout_ms = c.timeout_ms;
  result.durable_claim_directory = c.durable_claim_directory;
  return result;
}

void validate_config(const ProtocolIIIAav86SmallConfig& c) {
  const auto d = protocol_iii_aav86_small_domain(c.logical_n);
  const auto b = protocol_iii_aav86_small_rank_bits(d);
  require(c.session != 0 && c.fingerprint != 0 && c.material_id != 0 &&
              c.logical_n >= 2U && c.logical_n <= 8U && c.k >= 1U &&
              c.k <= c.logical_n && c.iterations >= 1U && c.iterations <= 5U &&
              c.party < 2U && c.timeout_ms > 0 &&
              !c.durable_claim_directory.empty(),
          "III+AAV86 public config");
  require(b == static_cast<std::uint8_t>(__builtin_ctz(d)) &&
              c.material_id <= UINT64_MAX - edge_slots(d, c.iterations) - d,
          "III+AAV86 material ID range");
  (void)comparison_mask(static_cast<std::uint8_t>(33U + b));
  (void)base_config(c);
}

void validate_material(const ProtocolIIIAav86SmallConfig& c,
                       const ProtocolIIIAav86SmallPartyMaterial& m) {
  const auto d = protocol_iii_aav86_small_domain(c.logical_n);
  const auto b = protocol_iii_aav86_small_rank_bits(d);
  const auto& base = m.layout_and_ca;
  const auto& route = m.handle_dpf;
  require(!m.started && !route.started &&
              m.material_id == c.material_id &&
              base.session == c.session && base.fingerprint == c.fingerprint &&
              base.material_id == c.material_id && base.party == c.party &&
              base.logical_n == c.logical_n && base.padded_n == d &&
              base.k == c.k && base.iterations == c.iterations &&
              base.comparison_bits == 33U + b &&
              route.session == c.session && route.fingerprint == c.fingerprint &&
              route.party == c.party && route.logical_n == d && route.k == c.k &&
              route.rank_bits == b && route.rank_mask_shares.size() == d &&
              route.dpf_keys.size() == d,
          "III+AAV86 material binding");
  const auto rm = rank_mask(b);
  for (const auto value : route.rank_mask_shares)
    require((value & ~rm) == 0U, "III+AAV86 rank mask width");
  for (const auto& owner : route.dpf_keys) {
    const auto& key = owner.native_key();
    require(key.s && key.bin == b && key.bout == kDpfOutputBits,
            "III+AAV86 DPF key width");
  }
  require(base.forward_sigma.size() == d && base.forward_tau.size() == d &&
              base.inverse_sigma.size() == d && base.inverse_tau.size() == d &&
              base.forward_a.size() == d && base.forward_e.size() == d &&
              base.inverse_a.size() == d && base.inverse_e.size() == d &&
              base.node_mask_shares.size() ==
                  static_cast<std::size_t>(c.iterations) * d &&
              base.edge_keys.size() ==
                  static_cast<std::size_t>(edge_slots(d, c.iterations)),
          "III+AAV86 nested layout shape");
  const auto cmp = comparison_mask(base.comparison_bits);
  for (const auto* values : {&base.forward_a, &base.forward_e,
                              &base.inverse_a, &base.inverse_e,
                              &base.node_mask_shares})
    for (const auto value : *values)
      require((value & ~cmp) == 0U, "III+AAV86 nested mask width");
  for (const auto& key : base.edge_keys)
    require(key.party_id() == c.party &&
                key.comparison_bits() == base.comparison_bits,
            "III+AAV86 edge party/width");
}

std::size_t dpf_key_bytes(std::uint8_t bits) {
  const std::size_t point_bytes = bits <= 8U ? 1U : bits <= 16U ? 2U : 4U;
  return (static_cast<std::size_t>(bits) + 1U) * sizeof(osuCrypto::block) +
         2U * point_bytes + 2U * sizeof(std::uint64_t);
}

std::vector<std::uint8_t> encode_key(const DPFKeyPack& key) {
  const auto expected = dpf_key_bytes(static_cast<std::uint8_t>(key.bin));
  std::vector<std::uint8_t> bytes(expected);
  char* cursor = reinterpret_cast<char*>(bytes.data());
  Peer sender(&cursor);
  sender.send_dpf_keypack(key);
  const auto size = static_cast<std::size_t>(sender.bytesSent());
  delete static_cast<MemBuf*>(sender.keyBuf);
  require(size == expected, "III+AAV86 native DPF wire size");
  return bytes;
}

DPFKeyPack decode_key(const std::vector<std::uint8_t>& bytes,
                      std::uint8_t bits) {
  require(bytes.size() == dpf_key_bytes(bits),
          "III+AAV86 native DPF key length");
  char* cursor = reinterpret_cast<char*>(
      const_cast<std::uint8_t*>(bytes.data()));
  Dealer receiver(&cursor);
  DPFKeyPack owned;
  try {
    auto wire = receiver.recv_dpf_keypack(bits, kDpfOutputBits);
    require(wire.s != nullptr, "III+AAV86 native DPF seeds");
    owned = DPFKeyPack(bits, kDpfOutputBits);
    std::memcpy(owned.s, wire.s,
                (static_cast<std::size_t>(bits) + 1U) * sizeof(osuCrypto::block));
    owned.tLcw = wire.tLcw;
    owned.tRcw = wire.tRcw;
    owned.payload = wire.payload;
    wire.s = nullptr;
    require(cursor == reinterpret_cast<char*>(
                const_cast<std::uint8_t*>(bytes.data() + bytes.size())),
            "III+AAV86 native DPF trailing bytes");
  } catch (...) {
    delete static_cast<MemBuf*>(receiver.keyBuf);
    throw;
  }
  delete static_cast<MemBuf*>(receiver.keyBuf);
  return owned;
}

void seed_dpf_stream() {
  std::array<std::uint64_t, 2> seed{};
  random_bytes(seed.data(), sizeof(seed));
  // DPF uses an independent FSS stream from the score/DCF pool.
  FSSConfig::prngs[0].SetSeed(osuCrypto::toBlock(seed[0], seed[1]));
}

void durable_claim(const ProtocolIIIAav86SmallConfig& c) {
  const int directory = ::open(c.durable_claim_directory.c_str(),
                               O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  if (directory < 0) throw std::runtime_error("III+AAV86 claim directory open");
  const auto name = std::to_string(c.session) + "-" +
                    std::to_string(c.material_id) + "-p" +
                    std::to_string(c.party);
  const int file = ::openat(directory, name.c_str(),
                            O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC,
                            0600);
  if (file < 0) {
    ::close(directory);
    throw std::runtime_error("III+AAV86 one-shot material already claimed");
  }
  const auto marker = std::to_string(c.fingerprint) + ":" +
                      std::to_string(c.logical_n) + ":" +
                      std::to_string(c.k) + ":" +
                      std::to_string(c.iterations) + "\n";
  const auto written = ::write(file, marker.data(), marker.size());
  const bool okay = written == static_cast<ssize_t>(marker.size()) &&
                    ::fsync(file) == 0 && ::fsync(directory) == 0;
  ::close(file);
  ::close(directory);
  if (!okay) throw std::runtime_error("III+AAV86 one-shot claim sync");
}

std::vector<std::uint8_t> encode_words(const std::vector<std::uint64_t>& words) {
  std::vector<std::uint8_t> bytes;
  bytes.reserve(words.size() * sizeof(std::uint64_t));
  for (const auto word : words)
    for (int shift = 56; shift >= 0; shift -= 8)
      bytes.push_back(static_cast<std::uint8_t>(word >> shift));
  return bytes;
}

std::vector<std::uint64_t> decode_words(
    const std::vector<std::uint8_t>& bytes, std::size_t expected,
    std::uint64_t value_mask) {
  require(bytes.size() == expected * sizeof(std::uint64_t),
          "III+AAV86 frame word count");
  std::vector<std::uint64_t> words(expected);
  for (std::size_t i = 0; i < expected; ++i) {
    for (std::size_t j = 0; j < sizeof(std::uint64_t); ++j)
      words[i] = (words[i] << 8U) | bytes[i * sizeof(std::uint64_t) + j];
    require((words[i] & ~value_mask) == 0U,
            "III+AAV86 frame canonical width");
  }
  return words;
}

struct Exchange {
  std::vector<std::uint64_t> peer;
  std::uint64_t sent = 0, received = 0;
};

Exchange exchange_words(int fd, const ProtocolIIIAav86SmallConfig& c,
                        std::uint32_t d, std::uint8_t comparison_bits,
                        std::uint8_t phase,
                        const std::vector<std::uint64_t>& outbound,
                        std::uint64_t value_mask) {
  require(fd >= 0 && outbound.size() == d,
          "III+AAV86 exchange shape/fd");
  ProtocolIFrameConfig frame{c.session, c.fingerprint, d, c.k,
                             comparison_bits, c.party,
                             static_cast<std::uint8_t>(1U - c.party),
                             phase, kFrameType};
  ProtocolIFramedChannel channel(fd, frame, c.timeout_ms);
  std::vector<std::uint8_t> inbound;
  const auto encoded = encode_words(outbound);
  if (c.party == 0U) {
    channel.send(encoded);
    inbound = channel.receive();
  } else {
    inbound = channel.receive();
    channel.send(encoded);
  }
  return {decode_words(inbound, d, value_mask),
          channel.sent_bytes(), channel.received_bytes()};
}

std::vector<std::uint64_t> permute_add(
    const ProtocolIPermutation& permutation,
    const std::vector<std::uint64_t>& input,
    const std::vector<std::uint64_t>& mask, std::uint64_t ring_mask_value) {
  auto permuted = protocol_i_apply_permutation(permutation, input);
  require(permuted.size() == mask.size(), "III+AAV86 permutation shape");
  for (std::size_t i = 0; i < permuted.size(); ++i)
    permuted[i] = (permuted[i] + mask[i]) & ring_mask_value;
  return permuted;
}

std::vector<std::uint64_t> permute_peer_add(
    const ProtocolIPermutation& permutation,
    const std::vector<std::uint64_t>& input,
    const std::vector<std::uint64_t>& mask, std::uint64_t ring_mask_value) {
  auto permuted = protocol_i_apply_permutation(permutation, input);
  require(permuted.size() == mask.size(), "III+AAV86 permutation shape");
  for (std::size_t i = 0; i < permuted.size(); ++i)
    permuted[i] = (permuted[i] + mask[i]) & ring_mask_value;
  return permuted;
}

std::size_t edge_index(std::uint32_t d, std::uint32_t a, std::uint32_t c) {
  require(a < c && c < d, "III+AAV86 canonical edge");
  return static_cast<std::size_t>(a) * (2U * d - a - 1U) / 2U + (c - a - 1U);
}

struct Node {
  std::vector<std::uint32_t> vertices;
  std::vector<std::uint32_t> pivots, pivot_order;
  std::vector<std::vector<std::uint32_t>> buckets;
  std::vector<int> children;
  std::uint32_t depth = 0;
  std::uint64_t offset = 0;
};

std::uint32_t ceil_root(std::uint32_t m, std::uint32_t depth) {
  require(m >= 2U && depth >= 1U, "III+AAV86 pivot root args");
  for (std::uint32_t t = 1; t <= m; ++t) {
    std::uint64_t power = 1;
    for (std::uint32_t i = 0; i < depth; ++i) {
      if (power > UINT64_MAX / t) { power = UINT64_MAX; break; }
      power *= t;
    }
    if (power >= m) return t;
  }
  throw std::logic_error("III+AAV86 pivot root");
}

std::uint64_t prg_bounded(osuCrypto::PRNG& prng, std::uint64_t bound) {
  require(bound != 0U, "III+AAV86 pivot bound");
  const auto cutoff = UINT64_MAX - UINT64_MAX % bound;
  std::uint64_t value = 0;
  do value = prng.get<std::uint64_t>(); while (value >= cutoff);
  return value % bound;
}

void choose_pivots(Node& node, osuCrypto::PRNG& prng) {
  auto remaining = node.vertices;
  const auto count = ceil_root(static_cast<std::uint32_t>(remaining.size()),
                               node.depth) - 1U;
  require(count >= 1U && count < remaining.size(),
          "III+AAV86 pivot count");
  for (std::uint32_t i = 0; i < count; ++i) {
    const auto selected = static_cast<std::size_t>(
        prg_bounded(prng, remaining.size() - i));
    node.pivots.push_back(remaining[selected]);
    std::swap(remaining[selected], remaining[remaining.size() - 1U - i]);
  }
}

void add_exchange_metrics(ProtocolIIIAav86SmallMetrics& m,
                          std::uint8_t phase, const Exchange& x,
                          std::uint64_t* stage_sent,
                          std::uint64_t* stage_received) {
  m.sent_bytes += x.sent;
  m.received_bytes += x.received;
  *stage_sent += x.sent;
  *stage_received += x.received;
  m.message_trace.push_back({phase, x.sent, x.received});
}

}  // namespace

std::uint32_t protocol_iii_aav86_small_domain(std::uint32_t logical_n) {
  require(logical_n >= 1U && logical_n <= 8U,
          "III+AAV86 small logical_n");
  std::uint32_t d = 2U;
  while (d < logical_n) d <<= 1U;
  return d;
}

std::uint8_t protocol_iii_aav86_small_rank_bits(std::uint32_t d) {
  require(d >= 2U && (d & (d - 1U)) == 0U,
          "III+AAV86 small domain");
  std::uint8_t bits = 0;
  for (auto value = d - 1U; value != 0U; value >>= 1U) ++bits;
  return bits;
}

ProtocolIIIAav86SmallDealerOutput protocol_iii_aav86_small_dealer_generate(
    const ProtocolIIIAav86SmallConfig& c) {
  validate_config(c);
  require(omp_in_parallel() == 0,
          "III+AAV86 T key generation must be single-threaded");
  std::lock_guard<std::mutex> lock(dealer_mutex);
  auto base = protocol_i_aav86_small_dealer_generate(base_config(c));
  ProtocolIIIAav86SmallDealerOutput out;
  out.party0.material_id = out.party1.material_id = c.material_id;
  out.party0.layout_and_ca = std::move(base.party0);
  out.party1.layout_and_ca = std::move(base.party1);
  const auto d = protocol_iii_aav86_small_domain(c.logical_n);
  const auto bits = protocol_iii_aav86_small_rank_bits(d);
  const auto mask = rank_mask(bits);
  auto initialize_route = [&](ProtocolIIIAav86SmallPartyMaterial& m,
                               std::uint8_t party) {
    auto& r = m.handle_dpf;
    r.session = c.session;
    r.fingerprint = c.fingerprint;
    r.logical_n = d;
    r.k = c.k;
    r.rank_bits = bits;
    r.party = party;
    r.rank_mask_shares.resize(d);
    r.dpf_keys.reserve(d);
  };
  initialize_route(out.party0, 0U);
  initialize_route(out.party1, 1U);
  seed_dpf_stream();
  for (std::uint32_t handle = 0; handle < d; ++handle) {
    const auto target = random_word() & mask;
    const auto share0 = random_word() & mask;
    out.party0.handle_dpf.rank_mask_shares[handle] = share0;
    out.party1.handle_dpf.rank_mask_shares[handle] = (target - share0) & mask;
    auto keys = keyGenDPF(bits, kDpfOutputBits, target, 1U);
    out.party0.handle_dpf.dpf_keys.emplace_back(std::move(keys.first));
    out.party1.handle_dpf.dpf_keys.emplace_back(std::move(keys.second));
  }
  auto c0 = c;
  c0.party = 0U;
  validate_material(c0, out.party0);
  auto c1 = c;
  c1.party = 1U;
  validate_material(c1, out.party1);
  return out;
}

std::vector<std::uint8_t> protocol_iii_aav86_small_serialize_material(
    const ProtocolIIIAav86SmallPartyMaterial& m) {
  const auto& base = m.layout_and_ca;
  const auto& route = m.handle_dpf;
  const auto d = base.padded_n;
  const auto bits = route.rank_bits;
  require(m.material_id != 0U && !m.started && !route.started &&
              route.logical_n == d && route.dpf_keys.size() == d &&
              route.rank_mask_shares.size() == d,
          "III+AAV86 serialize material shape");
  auto base_bytes = protocol_i_aav86_small_serialize_party_material(base);
  std::vector<std::uint8_t> out;
  out.reserve(64U + base_bytes.size() +
              d * (24U + dpf_key_bytes(bits)));
  put(out, kMagic, 4U); put(out, kVersion, 1U);
  put(out, base.party, 1U); put(out, 0U, 2U);
  put(out, m.material_id, 8U); put(out, base.session, 8U);
  put(out, base.fingerprint, 8U); put(out, base.logical_n, 4U);
  put(out, base.padded_n, 4U); put(out, base.k, 4U);
  put(out, base.iterations, 4U); put(out, base.comparison_bits, 1U);
  put(out, bits, 1U); put(out, kDpfOutputBits, 1U); put(out, 0U, 1U);
  put(out, base_bytes.size(), 8U); append(out, base_bytes);
  put(out, d, 4U);
  const auto base_slots = edge_slots(d, base.iterations);
  for (std::uint32_t h = 0; h < d; ++h) {
    require(m.material_id <= UINT64_MAX - base_slots - h,
            "III+AAV86 handle ID overflow");
    put(out, h, 4U); put(out, m.material_id + base_slots + h, 8U);
    put(out, route.rank_mask_shares[h], 8U);
  }
  put(out, d, 4U);
  for (std::uint32_t h = 0; h < d; ++h) {
    const auto& key = route.dpf_keys[h].native_key();
    require(key.bin == bits && key.bout == kDpfOutputBits && key.s,
            "III+AAV86 serialize DPF width");
    const auto wire = encode_key(key);
    put(out, h, 4U); put(out, m.material_id + base_slots + h, 8U);
    put(out, key.bin, 1U); put(out, key.bout, 1U);
    put(out, wire.size(), 4U); append(out, wire);
  }
  require(out.size() <= kMaxPackageBytes, "III+AAV86 package size cap");
  return out;
}

ProtocolIIIAav86SmallPartyMaterial protocol_iii_aav86_small_deserialize_material(
    const std::vector<std::uint8_t>& bytes, int expected_party,
    const ProtocolIIIAav86SmallConfig& expected_config) {
  validate_config(expected_config);
  require(bytes.size() <= kMaxPackageBytes && expected_party >= 0 &&
              expected_party < 2 && expected_party == expected_config.party,
          "III+AAV86 deserialize public binding");
  std::size_t at = 0;
  require(get(bytes, at, 4U) == kMagic && get(bytes, at, 1U) == kVersion,
          "III+AAV86 package magic/version");
  const auto party = get(bytes, at, 1U);
  require(party == static_cast<std::uint64_t>(expected_party) &&
              get(bytes, at, 2U) == 0U,
          "III+AAV86 package party/reserved");
  const auto id = get(bytes, at, 8U), session = get(bytes, at, 8U);
  const auto fingerprint = get(bytes, at, 8U);
  const auto n = get(bytes, at, 4U), d = get(bytes, at, 4U);
  const auto k = get(bytes, at, 4U), r = get(bytes, at, 4U);
  const auto comparison_bits = get(bytes, at, 1U);
  const auto bits = get(bytes, at, 1U), bout = get(bytes, at, 1U);
  require(get(bytes, at, 1U) == 0U && id == expected_config.material_id &&
              session == expected_config.session &&
              fingerprint == expected_config.fingerprint &&
              n == expected_config.logical_n &&
              d == protocol_iii_aav86_small_domain(expected_config.logical_n) &&
              k == expected_config.k && r == expected_config.iterations &&
              bits == protocol_iii_aav86_small_rank_bits(static_cast<std::uint32_t>(d)) &&
              bout == kDpfOutputBits && comparison_bits == 33U + bits,
          "III+AAV86 package shape/version binding");
  const auto base_length = get(bytes, at, 8U);
  require(base_length <= bytes.size() - at,
          "III+AAV86 base package length");
  const auto base_bytes = take(bytes, at, static_cast<std::size_t>(base_length));
  ProtocolIIIAav86SmallPartyMaterial result;
  result.material_id = id;
  result.layout_and_ca = protocol_i_aav86_small_deserialize_party_material(
      base_bytes, expected_party, base_config(expected_config));
  auto& route = result.handle_dpf;
  route.session = session; route.fingerprint = fingerprint;
  route.logical_n = static_cast<std::uint32_t>(d);
  route.k = static_cast<std::uint32_t>(k);
  route.rank_bits = static_cast<std::uint8_t>(bits);
  route.party = static_cast<std::uint8_t>(party);
  const auto base_slots = edge_slots(static_cast<std::uint32_t>(d),
                                     static_cast<std::uint32_t>(r));
  require(get(bytes, at, 4U) == d, "III+AAV86 rank mask count");
  route.rank_mask_shares.resize(static_cast<std::size_t>(d));
  const auto rm = rank_mask(static_cast<std::uint8_t>(bits));
  for (std::uint32_t h = 0; h < d; ++h) {
    require(get(bytes, at, 4U) == h &&
                get(bytes, at, 8U) == id + base_slots + h,
            "III+AAV86 rank mask handle/id");
    const auto share = get(bytes, at, 8U);
    require((share & ~rm) == 0U, "III+AAV86 rank mask canonical");
    route.rank_mask_shares[h] = share;
  }
  require(get(bytes, at, 4U) == d, "III+AAV86 DPF key count");
  route.dpf_keys.reserve(static_cast<std::size_t>(d));
  for (std::uint32_t h = 0; h < d; ++h) {
    require(get(bytes, at, 4U) == h &&
                get(bytes, at, 8U) == id + base_slots + h &&
                get(bytes, at, 1U) == bits && get(bytes, at, 1U) == bout,
            "III+AAV86 DPF handle/id/width");
    const auto length = get(bytes, at, 4U);
    require(length == dpf_key_bytes(static_cast<std::uint8_t>(bits)),
            "III+AAV86 DPF key byte length");
    auto key = decode_key(take(bytes, at, static_cast<std::size_t>(length)),
                          static_cast<std::uint8_t>(bits));
    route.dpf_keys.emplace_back(std::move(key));
  }
  require(at == bytes.size(), "III+AAV86 package trailing bytes");
  validate_material(expected_config, result);
  return result;
}

ProtocolIIIAav86SmallOutput protocol_iii_aav86_small_party(
    const ProtocolIIIAav86SmallConfig& c,
    ProtocolIIIAav86SmallPartyMaterial&& material,
    const std::vector<std::uint32_t>& raw_score_share,
    const ProtocolIIIAav86SmallFds& fds) {
  validate_config(c);
  validate_material(c, material);
  const auto d = protocol_iii_aav86_small_domain(c.logical_n);
  const auto b = protocol_iii_aav86_small_rank_bits(d);
  const auto cmp_mask = comparison_mask(material.layout_and_ca.comparison_bits);
  const auto rmask = rank_mask(b);
  require(raw_score_share.size() == c.logical_n &&
              fds.score[0] >= 0 && fds.score[1] >= 0 && fds.forward >= 0 &&
              fds.masked_lists.size() == c.iterations &&
              fds.early_ranks.size() + 1U == c.iterations &&
              fds.final_masked_rank >= 0 && fds.inverse >= 0,
          "III+AAV86 runtime channels/input");
  std::set<int> descriptors{fds.score[0], fds.score[1], fds.forward,
                            fds.final_masked_rank, fds.inverse};
  for (const auto fd : fds.masked_lists) require(fd >= 0 && descriptors.insert(fd).second,
                                                 "III+AAV86 duplicate channel");
  for (const auto fd : fds.early_ranks) require(fd >= 0 && descriptors.insert(fd).second,
                                                "III+AAV86 duplicate channel");
  require(descriptors.size() == 2U + 1U + c.iterations +
              (c.iterations - 1U) + 1U + 1U,
          "III+AAV86 channel count");
  durable_claim(c);
  material.started = true;
  material.handle_dpf.started = true;
  auto& base = material.layout_and_ca;
  auto& route = material.handle_dpf;
  ProtocolIIIAav86SmallOutput output;
  auto& metrics = output.metrics;
  const auto full_slots = edge_slots(d, c.iterations);
  metrics.reserved_edge_slots_per_party = full_slots;
  metrics.active_edges_by_round.reserve(c.iterations);
  metrics.active_vertices_by_round.reserve(c.iterations);
  metrics.dcf_prg_calls_by_round.reserve(c.iterations);
  metrics.causal_rounds = 2U + 1U + c.iterations + (c.iterations - 1U) + 1U + 1U;

  resetDCFOnlinePrgCalls();
  ProtocolIScoreInputMetrics score_metrics;
  const auto priority_key_shares = protocol_i_raw_score_input_party(
      {c.session, c.fingerprint, c.logical_n, d, c.k,
       static_cast<std::uint8_t>(b), base.comparison_bits, c.party, c.timeout_ms},
      base.score_materials, raw_score_share, fds.score, &score_metrics);
  metrics.score_sent_bytes = score_metrics.carry_sent_bytes +
                             score_metrics.sign_sent_bytes;
  metrics.score_received_bytes = score_metrics.carry_received_bytes +
                                score_metrics.sign_received_bytes;
  metrics.dcf_evaluations += score_metrics.raw_dcf_calls;
  metrics.sent_bytes += metrics.score_sent_bytes;
  metrics.received_bytes += metrics.score_received_bytes;
  metrics.message_trace.push_back({4U, score_metrics.carry_sent_bytes,
                                  score_metrics.carry_received_bytes});
  metrics.message_trace.push_back({5U, score_metrics.sign_sent_bytes,
                                  score_metrics.sign_received_bytes});

  // Forward hidden layout: the same hidden permutation used by the inverse
  // material, with two fresh additive masks in the comparison ring.
  const auto first_outbound = permute_add(base.forward_sigma,
                                          priority_key_shares,
                                          base.forward_a, cmp_mask);
  const auto forward_exchange = exchange_words(
      fds.forward, c, d, base.comparison_bits, 6U, first_outbound, cmp_mask);
  const auto shuffled_share = permute_peer_add(base.forward_tau,
                                                forward_exchange.peer,
                                                base.forward_e, cmp_mask);
  add_exchange_metrics(metrics, 6U, forward_exchange,
                       &metrics.forward_sent_bytes,
                       &metrics.forward_received_bytes);

  osuCrypto::PRNG pivot_prg;
  pivot_prg.SetSeed(osuCrypto::toBlock(base.pivot_seed_lo,
                                       base.pivot_seed_hi));
  Node root;
  root.depth = c.iterations;
  root.vertices.resize(d);
  for (std::uint32_t h = 0; h < d; ++h) root.vertices[h] = h;
  std::vector<Node> nodes;
  nodes.push_back(std::move(root));
  std::vector<int> active{0};
  std::vector<bool> consumed(static_cast<std::size_t>(full_slots), false);
  std::vector<std::uint64_t> final_rank_shares(d, 0U);

  for (std::uint32_t t = 0; t < c.iterations; ++t) {
    std::vector<std::uint64_t> local_open(d);
    for (std::uint32_t h = 0; h < d; ++h)
      local_open[h] = (shuffled_share[h] +
          base.node_mask_shares[static_cast<std::size_t>(t) * d + h]) & cmp_mask;
    const auto open_phase = static_cast<std::uint8_t>(20U + t);
    const auto open = exchange_words(fds.masked_lists[t], c, d,
        base.comparison_bits, open_phase, local_open, cmp_mask);
    std::vector<std::uint64_t> opened(d);
    for (std::uint32_t h = 0; h < d; ++h)
      opened[h] = (local_open[h] + open.peer[h]) & cmp_mask;
    add_exchange_metrics(metrics, open_phase, open,
                         &metrics.ca_sent_bytes, &metrics.ca_received_bytes);

    std::set<std::pair<std::uint32_t, std::uint32_t>> graph;
    for (const auto node_id : active) {
      auto& node = nodes.at(static_cast<std::size_t>(node_id));
      require(node.depth == c.iterations - t && node.vertices.size() > 1U,
              "III+AAV86 active node invariant");
      choose_pivots(node, pivot_prg);
      const std::set<std::uint32_t> pivots(node.pivots.begin(), node.pivots.end());
      for (std::size_t i = 0; i < node.vertices.size(); ++i)
        for (std::size_t j = i + 1; j < node.vertices.size(); ++j) {
          const auto a = node.vertices[i], z = node.vertices[j];
          if (pivots.count(a) || pivots.count(z))
            graph.emplace(std::min(a, z), std::max(a, z));
        }
    }
    std::set<std::uint32_t> graph_vertices;
    for (const auto& edge : graph) {
      graph_vertices.insert(edge.first);
      graph_vertices.insert(edge.second);
    }
    metrics.active_edges_by_round.push_back(graph.size());
    metrics.active_vertices_by_round.push_back(graph_vertices.size());
    metrics.active_edges += graph.size();
    metrics.active_vertices += graph_vertices.size();
    const auto prg_before = readDCFOnlinePrgCalls();
    std::vector<std::uint64_t> local_rank_shares(d, 0U);
    for (const auto& edge : graph) {
      const auto a = edge.first, z = edge.second;
      const auto slot = static_cast<std::size_t>(t) * (d * (d - 1U) / 2U) +
                        edge_index(d, a, z);
      require(slot < base.edge_keys.size() && !consumed[slot],
              "III+AAV86 edge slot replay");
      consumed[slot] = true;
      auto& key = base.edge_keys[slot];
      const auto lt = key.eval_strict_lt(opened[a], opened[z]);
      local_rank_shares[a] += (c.party == 0U ? UINT64_C(1) : UINT64_C(0)) - lt;
      local_rank_shares[z] += lt;
      metrics.dcf_evaluations += 2U;
    }
    for (auto& value : local_rank_shares) value &= rmask;
    const auto round_prg = readDCFOnlinePrgCalls() - prg_before;
    metrics.dcf_prg_calls_by_round.push_back(round_prg);

    if (t + 1U == c.iterations) {
      // Terminal active nodes are complete cliques. Keep their local rank
      // shares secret and add their already-public parent offset locally.
      for (const auto node_id : active) {
        const auto& node = nodes.at(static_cast<std::size_t>(node_id));
        for (const auto h : node.vertices)
          final_rank_shares[h] =
              (local_rank_shares[h] +
               (c.party == 0U ? node.offset : 0U)) & rmask;
      }
      continue;
    }

    const auto rank_phase = static_cast<std::uint8_t>(30U + t);
    const auto rank_exchange = exchange_words(
        fds.early_ranks[t], c, d, base.comparison_bits, rank_phase,
        local_rank_shares, rmask);
    std::vector<std::uint64_t> public_rank(d);
    for (std::uint32_t h = 0; h < d; ++h)
      public_rank[h] = (local_rank_shares[h] + rank_exchange.peer[h]) & rmask;
    add_exchange_metrics(metrics, rank_phase, rank_exchange,
                         &metrics.ca_sent_bytes, &metrics.ca_received_bytes);

    std::vector<int> next_active;
    for (const auto node_id : active) {
      const auto vertices = nodes.at(static_cast<std::size_t>(node_id)).vertices;
      const auto pivots = nodes.at(static_cast<std::size_t>(node_id)).pivots;
      const auto depth = nodes.at(static_cast<std::size_t>(node_id)).depth;
      const auto offset = nodes.at(static_cast<std::size_t>(node_id)).offset;
      auto pivot_order = pivots;
      std::sort(pivot_order.begin(), pivot_order.end(),
                [&](auto a, auto z) { return public_rank[a] < public_rank[z]; });
      for (std::size_t i = 0; i < pivot_order.size(); ++i) {
        require(public_rank[pivot_order[i]] < vertices.size(),
                "III+AAV86 pivot rank range");
        if (i) require(public_rank[pivot_order[i - 1U]] <
                           public_rank[pivot_order[i]],
                       "III+AAV86 pivot rank uniqueness");
        const auto absolute = (offset + public_rank[pivot_order[i]]) & rmask;
        final_rank_shares[pivot_order[i]] = c.party == 0U ? absolute : 0U;
      }
      const std::set<std::uint32_t> pivot_set(pivots.begin(), pivots.end());
      std::vector<std::vector<std::uint32_t>> buckets(pivots.size() + 1U);
      for (const auto h : vertices) {
        if (pivot_set.count(h)) continue;
        require(public_rank[h] <= pivots.size(), "III+AAV86 bucket rank");
        buckets[static_cast<std::size_t>(public_rank[h])].push_back(h);
      }
      auto& node = nodes.at(static_cast<std::size_t>(node_id));
      node.pivot_order = pivot_order;
      node.buckets = buckets;
      node.children.assign(node.buckets.size(), -1);
      const auto bucket_count = buckets.size();
      for (std::size_t bucket_index = 0;
           bucket_index < bucket_count; ++bucket_index) {
        const auto bucket = buckets[bucket_index];
        if (bucket.empty()) continue;
        const auto child_offset = bucket_index == 0U
            ? offset
            : (offset + public_rank[pivot_order[bucket_index - 1U]] + 1U) & rmask;
        if (bucket.size() == 1U) {
          final_rank_shares[bucket[0]] = c.party == 0U ? child_offset : 0U;
          continue;
        }
        require(depth > 1U, "III+AAV86 terminal bucket is not singleton");
        Node child;
        child.vertices = bucket;
        child.depth = depth - 1U;
        child.offset = child_offset;
        const auto child_id = static_cast<int>(nodes.size());
        nodes.push_back(std::move(child));
        nodes.at(static_cast<std::size_t>(node_id)).children[bucket_index] = child_id;
        next_active.push_back(child_id);
      }
    }
    active = std::move(next_active);
  }
  // Ring DPF stage. Accumulate each party's outputs in Z_(2^64), then map
  // that additive share to Z_(2^b) before inverse routing. This reduction is
  // additive; taking only local parity before the ring addition is not.
  std::vector<std::uint64_t> local_masked_rank(d);
  for (std::uint32_t h = 0; h < d; ++h)
    local_masked_rank[h] = (final_rank_shares[h] +
                            route.rank_mask_shares[h]) & rmask;
  const auto final_phase = static_cast<std::uint8_t>(40U);
  const auto final_open = exchange_words(
      fds.final_masked_rank, c, d, base.comparison_bits, final_phase,
      local_masked_rank, rmask);
  std::vector<std::uint64_t> opened_masked_rank(d);
  for (std::uint32_t h = 0; h < d; ++h)
    opened_masked_rank[h] = (local_masked_rank[h] + final_open.peer[h]) & rmask;
  add_exchange_metrics(metrics, final_phase, final_open,
                       &metrics.route_sent_bytes, &metrics.route_received_bytes);

  std::vector<std::uint64_t> handle_additive_shares(d);
  for (std::uint32_t h = 0; h < d; ++h) {
    auto& key = route.dpf_keys[h].native_key();
    std::uint64_t accumulator = 0U;
    for (std::uint32_t target_rank = 0; target_rank < c.k; ++target_rank) {
      const auto point = (opened_masked_rank[h] - target_rank) & rmask;
      accumulator += static_cast<std::uint64_t>(
          evalDPF_Payload(c.party, key, point));
      ++metrics.dpf_evaluations;
    }
    handle_additive_shares[h] = accumulator & rmask;
  }
  route.rank_mask_shares.clear();
  route.dpf_keys.clear();

  const auto inverse_first = permute_add(base.inverse_sigma,
                                         handle_additive_shares,
                                         base.inverse_a, rmask);
  const auto inverse_phase = static_cast<std::uint8_t>(41U);
  const auto inverse_exchange = exchange_words(
      fds.inverse, c, d, base.comparison_bits, inverse_phase,
      inverse_first, rmask);
  const auto original_additive_shares = permute_peer_add(
      base.inverse_tau, inverse_exchange.peer, base.inverse_e, rmask);
  add_exchange_metrics(metrics, inverse_phase, inverse_exchange,
                       &metrics.route_sent_bytes, &metrics.route_received_bytes);
  output.xor_mask_share.resize(c.logical_n);
  for (std::uint32_t i = 0; i < c.logical_n; ++i)
    output.xor_mask_share[i] = static_cast<std::uint8_t>(
        original_additive_shares[i] & 1U);
  metrics.dcf_online_prg_calls = readDCFOnlinePrgCalls();
  require(metrics.score_sent_bytes + metrics.forward_sent_bytes +
              metrics.ca_sent_bytes + metrics.route_sent_bytes == metrics.sent_bytes &&
              metrics.score_received_bytes + metrics.forward_received_bytes +
              metrics.ca_received_bytes + metrics.route_received_bytes ==
                  metrics.received_bytes,
          "III+AAV86 communication accounting");
  return output;
}

}  // namespace moe_topk
