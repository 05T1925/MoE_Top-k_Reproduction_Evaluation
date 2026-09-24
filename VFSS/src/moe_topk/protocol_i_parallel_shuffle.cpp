#include <moe_topk/protocol_i_parallel_shuffle.h>

#include <FSS/prng.h>
#include <moe_topk/protocol_i_cmpagg.h>
#include <moe_topk/protocol_i_transport.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <exception>
#include <limits>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <utility>
#include <sys/random.h>
#include <sys/socket.h>
#include <unistd.h>

namespace moe_topk {
namespace {

constexpr std::size_t kMaxMaterialBytes = 512U * 1024U * 1024U;
constexpr std::uint64_t kMaxEdges = 1'000'000U;

[[noreturn]] void fail(const char* message) { throw std::invalid_argument(message); }
void require(bool condition, const char* message) { if (!condition) fail(message); }

std::uint64_t ring_mask(int bits) {
  require(bits >= 34 && bits <= 53, "parallel shuffle comparison bits");
  return (UINT64_C(1) << bits) - 1U;
}

std::uint64_t edge_count(std::uint32_t n) {
  const auto result = static_cast<std::uint64_t>(n) * (n - 1U) / 2U;
  require(result <= kMaxEdges, "parallel shuffle edge count");
  return result;
}

void validate_dealer_config(const ProtocolIParallelShuffleDealerConfig& config) {
  require(config.session != 0 && config.fingerprint != 0 && config.material_id != 0 &&
              config.n >= 2 && config.k >= 1 && config.k <= config.n,
          "parallel shuffle dealer config");
  (void)ring_mask(config.comparison_bits);
  (void)edge_count(config.n);
}

void random_bytes(void* output, std::size_t size) {
  auto* cursor = static_cast<std::uint8_t*>(output);
  while (size != 0) {
    const auto count = ::getrandom(cursor, size, 0);
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) throw std::runtime_error("parallel shuffle entropy");
    cursor += count;
    size -= static_cast<std::size_t>(count);
  }
}

std::uint64_t random_u64() {
  std::uint64_t value = 0;
  random_bytes(&value, sizeof(value));
  return value;
}

std::uint64_t random_bounded(std::uint64_t bound) {
  require(bound != 0, "parallel shuffle random bound");
  const auto cutoff = UINT64_MAX - (UINT64_MAX % bound);
  std::uint64_t value = 0;
  do value = random_u64(); while (value >= cutoff);
  return value % bound;
}

ProtocolIPermutation random_permutation(std::uint32_t n) {
  auto permutation = protocol_i_identity_permutation(n);
  for (std::uint32_t remaining = n; remaining > 1; --remaining) {
    const auto selected = static_cast<std::uint32_t>(random_bounded(remaining));
    std::swap(permutation[remaining - 1U], permutation[selected]);
  }
  return permutation;
}

ProtocolIBlock192 random_record(int bits) {
  return {random_u64() & ring_mask(bits), random_u64(), random_u64()};
}

std::vector<ProtocolIBlock192> random_vector(std::uint32_t n, int bits) {
  std::vector<ProtocolIBlock192> result(n);
  for (auto& record : result) record = random_record(bits);
  return result;
}

std::vector<ProtocolIBlock192> add_vectors(int bits,
                                           const std::vector<ProtocolIBlock192>& left,
                                           const std::vector<ProtocolIBlock192>& right) {
  require(left.size() == right.size(), "parallel shuffle vector size");
  auto output = left;
  for (std::size_t index = 0; index < output.size(); ++index)
    output[index] = protocol_i_parallel_record_add(bits, output[index], right[index]);
  return output;
}

std::vector<ProtocolIBlock192> sub_vectors(int bits,
                                           const std::vector<ProtocolIBlock192>& left,
                                           const std::vector<ProtocolIBlock192>& right) {
  require(left.size() == right.size(), "parallel shuffle vector size");
  auto output = left;
  for (std::size_t index = 0; index < output.size(); ++index)
    output[index] = protocol_i_parallel_record_sub(bits, output[index], right[index]);
  return output;
}

void seed_fss_once() {
  static std::once_flag flag;
  std::call_once(flag, [] {
    for (int index = 0; index < 256; ++index) {
      std::uint64_t words[2]{};
      random_bytes(words, sizeof(words));
      FSSConfig::prngs[index].SetSeed(osuCrypto::toBlock(words[0], words[1]));
    }
  });
}

void put_u32(std::vector<std::uint8_t>& output, std::uint32_t value) {
  for (int shift = 24; shift >= 0; shift -= 8)
    output.push_back(static_cast<std::uint8_t>(value >> shift));
}

void put_u64(std::vector<std::uint8_t>& output, std::uint64_t value) {
  for (int shift = 56; shift >= 0; shift -= 8)
    output.push_back(static_cast<std::uint8_t>(value >> shift));
}

std::uint32_t get_u32(const std::vector<std::uint8_t>& input, std::size_t& offset) {
  require(offset <= input.size() && input.size() - offset >= 4, "parallel material truncated");
  std::uint32_t value = 0;
  for (int index = 0; index < 4; ++index) value = (value << 8U) | input[offset++];
  return value;
}

std::uint64_t get_u64(const std::vector<std::uint8_t>& input, std::size_t& offset) {
  require(offset <= input.size() && input.size() - offset >= 8, "parallel material truncated");
  std::uint64_t value = 0;
  for (int index = 0; index < 8; ++index) value = (value << 8U) | input[offset++];
  return value;
}

void validate_record(int bits, const ProtocolIBlock192& record) {
  require((record.word0 & ~ring_mask(bits)) == 0, "parallel shuffle key lane outside ring");
}

void validate_records(int bits, const std::vector<ProtocolIBlock192>& records,
                      std::uint32_t n) {
  require(records.size() == n, "parallel shuffle record shape");
  for (const auto& record : records) validate_record(bits, record);
}

void validate_material(const ProtocolIParallelShufflePartyMaterial& material) {
  require(material.session != 0 && material.fingerprint != 0 && material.material_id != 0 &&
              material.n >= 2 && material.k >= 1 && material.k <= material.n && material.party < 2,
          "parallel shuffle material binding");
  (void)ring_mask(material.comparison_bits);
  protocol_i_validate_permutation(material.sigma);
  protocol_i_validate_permutation(material.tau);
  require(material.sigma.size() == material.n && material.tau.size() == material.n,
          "parallel shuffle permutation shape");
  validate_records(material.comparison_bits, material.a, material.n);
  validate_records(material.comparison_bits, material.e, material.n);
  validate_records(material.comparison_bits, material.r_share, material.n);
  require(material.edge_materials.size() == edge_count(material.n),
          "parallel shuffle edge material count");
  for (const auto& edge : material.edge_materials)
    require(edge.party_id() == material.party &&
                edge.comparison_bits() == material.comparison_bits,
            "parallel shuffle edge material binding");
}

using Claim = std::tuple<std::uint64_t, std::uint64_t, std::uint64_t, std::uint8_t>;
std::mutex claim_mutex;
std::set<Claim> claimed_material;

void claim(const ProtocolIParallelShufflePartyMaterial& material) {
  const Claim identity{material.session, material.fingerprint, material.material_id, material.party};
  std::lock_guard<std::mutex> lock(claim_mutex);
  require(claimed_material.insert(identity).second, "parallel shuffle material replay");
}

std::vector<std::uint8_t> encode_records(const std::vector<ProtocolIBlock192>& records) {
  std::vector<std::uint8_t> output;
  output.reserve(records.size() * 24U);
  for (const auto& record : records) {
    put_u64(output, record.word0); put_u64(output, record.word1); put_u64(output, record.word2);
  }
  return output;
}

std::vector<ProtocolIBlock192> decode_records(const std::vector<std::uint8_t>& bytes,
                                               std::uint32_t n, int bits) {
  require(bytes.size() == static_cast<std::size_t>(n) * 24U,
          "parallel shuffle record message length");
  std::size_t offset = 0;
  std::vector<ProtocolIBlock192> result(n);
  for (auto& record : result) {
    record = {get_u64(bytes, offset), get_u64(bytes, offset), get_u64(bytes, offset)};
    validate_record(bits, record);
  }
  return result;
}

std::vector<std::uint8_t> encode_ranks(const std::vector<std::uint64_t>& ranks,
                                       std::uint8_t bits) {
  const auto bytes_per_rank = static_cast<std::size_t>((bits + 7U) / 8U);
  const auto mask = (UINT64_C(1) << bits) - 1U;
  std::vector<std::uint8_t> output;
  output.reserve(ranks.size() * bytes_per_rank);
  for (const auto rank : ranks) {
    require((rank & ~mask) == 0, "parallel shuffle rank share width");
    for (std::size_t byte = 0; byte < bytes_per_rank; ++byte) {
      const auto shift = 8U * (bytes_per_rank - 1U - byte);
      output.push_back(static_cast<std::uint8_t>(rank >> shift));
    }
  }
  return output;
}

std::vector<std::uint64_t> decode_ranks(const std::vector<std::uint8_t>& bytes,
                                        std::uint32_t n, std::uint8_t bits) {
  const auto bytes_per_rank = static_cast<std::size_t>((bits + 7U) / 8U);
  require(bytes.size() == static_cast<std::size_t>(n) * bytes_per_rank,
          "parallel shuffle rank message length");
  const auto mask = (UINT64_C(1) << bits) - 1U;
  std::vector<std::uint64_t> output(n);
  std::size_t offset = 0;
  for (auto& rank : output) {
    for (std::size_t byte = 0; byte < bytes_per_rank; ++byte)
      rank = (rank << 8U) | bytes[offset++];
    require((rank & ~mask) == 0, "parallel shuffle rank message width");
  }
  return output;
}

struct ExchangeResult {
  std::vector<std::uint8_t> peer;
  std::uint64_t sent_bytes = 0, received_bytes = 0;
};

ExchangeResult exchange_payload(int fd, const ProtocolIParallelShufflePartyConfig& config,
                                std::uint8_t phase, const std::vector<std::uint8_t>& outbound) {
  const auto send_fd = ::dup(fd);
  if (send_fd < 0) throw std::runtime_error("parallel shuffle dup");
  ProtocolIFrameConfig framing{config.session, config.fingerprint, config.n, config.k,
                               config.comparison_bits, config.party,
                               static_cast<std::uint8_t>(1U - config.party), phase, 7};
  ProtocolIFramedChannel sender(send_fd, framing, config.timeout_ms);
  ProtocolIFramedChannel receiver(fd, framing, config.timeout_ms);
  std::exception_ptr send_error;
  std::thread send_thread([&] {
    try { sender.send(outbound); } catch (...) { send_error = std::current_exception(); }
  });
  std::vector<std::uint8_t> incoming;
  std::exception_ptr receive_error;
  try { incoming = receiver.receive(); } catch (...) {
    receive_error = std::current_exception();
    (void)::shutdown(fd, SHUT_RDWR);
  }
  send_thread.join();
  if (send_error) std::rethrow_exception(send_error);
  if (receive_error) std::rethrow_exception(receive_error);
  return {std::move(incoming), sender.sent_bytes(), receiver.received_bytes()};
}

}  // namespace

ProtocolIBlock192 protocol_i_parallel_record_add(int bits, ProtocolIBlock192 left,
                                                  const ProtocolIBlock192& right) {
  left.word0 = (left.word0 + right.word0) & ring_mask(bits);
  left.word1 += right.word1;
  left.word2 += right.word2;
  return left;
}

ProtocolIBlock192 protocol_i_parallel_record_sub(int bits, ProtocolIBlock192 left,
                                                  const ProtocolIBlock192& right) {
  left.word0 = (left.word0 - right.word0) & ring_mask(bits);
  left.word1 -= right.word1;
  left.word2 -= right.word2;
  return left;
}

std::uint8_t protocol_i_parallel_rank_bits(std::uint32_t n) {
  require(n >= 2, "parallel shuffle rank domain");
  std::uint8_t bits = 0;
  for (auto value = n - 1U; value != 0; value >>= 1U) ++bits;
  return bits;
}

ProtocolIParallelShuffleDealerOutput protocol_i_parallel_shuffle_dealer_generate(
    const ProtocolIParallelShuffleDealerConfig& config) {
  validate_dealer_config(config);
  seed_fss_once();
  const auto pi = random_permutation(config.n);
  const auto sigma0 = random_permutation(config.n);
  const auto sigma1 = random_permutation(config.n);
  const auto tau0 = protocol_i_compose_permutation(
      pi, protocol_i_inverse_permutation(sigma1));
  const auto tau1 = protocol_i_compose_permutation(
      pi, protocol_i_inverse_permutation(sigma0));
  const auto a0 = random_vector(config.n, config.comparison_bits);
  const auto a1 = random_vector(config.n, config.comparison_bits);
  const auto h = random_vector(config.n, config.comparison_bits);
  const auto r0 = random_vector(config.n, config.comparison_bits);
  const auto r1 = random_vector(config.n, config.comparison_bits);
  const std::vector<ProtocolIBlock192> zero(config.n);
  const auto e0 = sub_vectors(config.comparison_bits,
                              sub_vectors(config.comparison_bits, zero,
                                          protocol_i_apply_permutation(tau0, a1)), h);
  const auto e1 = add_vectors(config.comparison_bits,
                              sub_vectors(config.comparison_bits, zero,
                                          protocol_i_apply_permutation(tau1, a0)), h);

  ProtocolIParallelShuffleDealerOutput output;
  auto initialize = [&](ProtocolIParallelShufflePartyMaterial& material, int party,
                        const ProtocolIPermutation& sigma, const ProtocolIPermutation& tau,
                        const std::vector<ProtocolIBlock192>& a,
                        const std::vector<ProtocolIBlock192>& e,
                        const std::vector<ProtocolIBlock192>& r) {
    material.session = config.session; material.fingerprint = config.fingerprint;
    material.material_id = config.material_id; material.n = config.n; material.k = config.k;
    material.comparison_bits = config.comparison_bits; material.party = party;
    material.sigma = sigma; material.tau = tau; material.a = a; material.e = e;
    material.r_share = r;
  };
  initialize(output.party0, 0, sigma0, tau0, a0, e0, r0);
  initialize(output.party1, 1, sigma1, tau1, a1, e1, r1);
  const auto full_r = add_vectors(config.comparison_bits, r0, r1);
  std::uint64_t key_wire_bytes = 0;
  for (std::uint32_t left = 0; left < config.n; ++left) {
    for (std::uint32_t right = left + 1U; right < config.n; ++right) {
      ProtocolIUcmpMaterial edge(config.comparison_bits, full_r[left].word0,
                                  full_r[right].word0);
      auto key0 = edge.export_party_material(0);
      auto key1 = edge.export_party_material(1);
      key_wire_bytes += key0.serialize().size() + key1.serialize().size();
      output.party0.edge_materials.push_back(std::move(key0));
      output.party1.edge_materials.push_back(std::move(key1));
    }
  }
  const auto record_bits = static_cast<std::uint64_t>(config.comparison_bits) + 128U;
  const auto permutation_bits = protocol_i_parallel_rank_bits(config.n);
  output.metrics.record_material_logical_bits = 6U * config.n * record_bits;
  output.metrics.permutation_material_logical_bits = 4U * config.n * permutation_bits;
  output.metrics.cmpagg_party_material_wire_bytes = key_wire_bytes;
  validate_material(output.party0);
  validate_material(output.party1);
  return output;
}

std::vector<std::uint8_t> protocol_i_parallel_shuffle_serialize_material(
    const ProtocolIParallelShufflePartyMaterial& material) {
  validate_material(material);
  std::vector<std::uint8_t> output{'M','2','C','P',1,material.party,
                                   material.comparison_bits,0};
  put_u64(output, material.session); put_u64(output, material.fingerprint);
  put_u64(output, material.material_id); put_u64(output, material.n); put_u64(output, material.k);
  for (const auto value : material.sigma) put_u32(output, value);
  for (const auto value : material.tau) put_u32(output, value);
  for (const auto* records : {&material.a, &material.e, &material.r_share})
    for (const auto& record : *records) {
      put_u64(output, record.word0); put_u64(output, record.word1); put_u64(output, record.word2);
    }
  put_u64(output, material.edge_materials.size());
  for (const auto& edge : material.edge_materials) {
    const auto encoded = edge.serialize();
    require(output.size() <= kMaxMaterialBytes - 8U &&
                encoded.size() <= kMaxMaterialBytes - output.size() - 8U,
            "parallel shuffle material size");
    put_u64(output, encoded.size());
    output.insert(output.end(), encoded.begin(), encoded.end());
  }
  require(output.size() <= kMaxMaterialBytes, "parallel shuffle material size");
  return output;
}

ProtocolIParallelShufflePartyMaterial protocol_i_parallel_shuffle_deserialize_material(
    const std::vector<std::uint8_t>& bytes, int expected_party) {
  require(expected_party >= 0 && expected_party <= 1 && bytes.size() >= 48U &&
              bytes.size() <= kMaxMaterialBytes && bytes[0]=='M' && bytes[1]=='2' &&
              bytes[2]=='C' && bytes[3]=='P' && bytes[4]==1 && bytes[5]==expected_party &&
              bytes[7]==0,
          "parallel shuffle material header");
  ProtocolIParallelShufflePartyMaterial material;
  material.party = bytes[5]; material.comparison_bits = bytes[6];
  std::size_t offset = 8;
  material.session = get_u64(bytes, offset); material.fingerprint = get_u64(bytes, offset);
  material.material_id = get_u64(bytes, offset);
  const auto n = get_u64(bytes, offset), k = get_u64(bytes, offset);
  require(n <= UINT32_MAX && k <= UINT32_MAX, "parallel shuffle material dimensions");
  material.n = static_cast<std::uint32_t>(n); material.k = static_cast<std::uint32_t>(k);
  const auto fixed_bytes = static_cast<std::uint64_t>(material.n) * (8U + 72U) + 8U;
  require(fixed_bytes <= bytes.size() - offset, "parallel shuffle material fixed fields");
  material.sigma.resize(material.n); material.tau.resize(material.n);
  for (auto& value : material.sigma) value = get_u32(bytes, offset);
  for (auto& value : material.tau) value = get_u32(bytes, offset);
  auto get_records = [&](std::vector<ProtocolIBlock192>& records) {
    records.resize(material.n);
    for (auto& record : records)
      record = {get_u64(bytes, offset), get_u64(bytes, offset), get_u64(bytes, offset)};
  };
  get_records(material.a); get_records(material.e); get_records(material.r_share);
  const auto edges = get_u64(bytes, offset);
  require(edges == edge_count(material.n), "parallel shuffle serialized edge count");
  material.edge_materials.reserve(static_cast<std::size_t>(edges));
  for (std::uint64_t index = 0; index < edges; ++index) {
    const auto length = get_u64(bytes, offset);
    require(length <= bytes.size() - offset && length <= std::numeric_limits<std::size_t>::max(),
            "parallel shuffle edge length");
    std::vector<std::uint8_t> encoded(bytes.begin() + offset, bytes.begin() + offset + length);
    offset += static_cast<std::size_t>(length);
    material.edge_materials.push_back(ProtocolIUcmpPartyMaterial::deserialize(encoded));
  }
  require(offset == bytes.size(), "parallel shuffle material trailing bytes");
  validate_material(material);
  return material;
}

ProtocolIParallelShuffleParty::ProtocolIParallelShuffleParty(
    const ProtocolIParallelShufflePartyConfig& config,
    ProtocolIParallelShufflePartyMaterial&& material)
    : config_(config), material_(std::move(material)) {
  require(config_.party < 2 && config_.timeout_ms > 0 &&
              config_.session == material_.session &&
              config_.fingerprint == material_.fingerprint &&
              config_.material_id == material_.material_id && config_.n == material_.n &&
              config_.k == material_.k && config_.comparison_bits == material_.comparison_bits &&
              config_.party == material_.party,
          "parallel shuffle runtime binding");
  validate_material(material_);
  claim(material_);
}

std::vector<ProtocolIBlock192> ProtocolIParallelShuffleParty::prepare_round1(
    const std::vector<ProtocolIBlock192>& input_share) {
  require(phase_ == 0, "parallel shuffle round1 state");
  validate_records(config_.comparison_bits, input_share, config_.n);
  phase_ = 1;
  return add_vectors(config_.comparison_bits,
                     protocol_i_apply_permutation(material_.sigma, input_share), material_.a);
}

std::vector<ProtocolIBlock192>
ProtocolIParallelShuffleParty::receive_round1_prepare_round2(
    const std::vector<ProtocolIBlock192>& peer_round1) {
  require(phase_ == 1, "parallel shuffle round1 receive state");
  validate_records(config_.comparison_bits, peer_round1, config_.n);
  shuffled_share_ = add_vectors(config_.comparison_bits,
      protocol_i_apply_permutation(material_.tau, peer_round1), material_.e);
  round2_outbound_ = add_vectors(config_.comparison_bits, shuffled_share_, material_.r_share);
  phase_ = 2;
  return round2_outbound_;
}

ProtocolIParallelShuffleRound2Output ProtocolIParallelShuffleParty::receive_round2(
    const std::vector<ProtocolIBlock192>& peer_round2) {
  require(phase_ == 2, "parallel shuffle round2 state");
  validate_records(config_.comparison_bits, peer_round2, config_.n);
  public_masked_ = add_vectors(config_.comparison_bits, round2_outbound_, peer_round2);
  phase_ = 3;
  return {shuffled_share_, public_masked_};
}

std::vector<std::uint64_t>
ProtocolIParallelShuffleParty::evaluate_cmpagg_prepare_round3() {
  require(phase_ == 3, "parallel shuffle CmpAgg state");
  std::vector<std::uint64_t> masked_keys(config_.n);
  for (std::size_t index = 0; index < masked_keys.size(); ++index)
    masked_keys[index] = public_masked_[index].word0;
  rank_shares_ = protocol_i_cmpagg_eval_party(config_.party, config_.comparison_bits,
                                               masked_keys, material_.edge_materials);
  const auto rank_mask = (UINT64_C(1) << protocol_i_parallel_rank_bits(config_.n)) - 1U;
  for (auto& rank : rank_shares_) rank &= rank_mask;
  phase_ = 4;
  return rank_shares_;
}

ProtocolIParallelShuffleCoreOutput ProtocolIParallelShuffleParty::receive_round3(
    const std::vector<std::uint64_t>& peer_rank_shares) {
  require(phase_ == 4 && peer_rank_shares.size() == config_.n,
          "parallel shuffle round3 state");
  const auto bits = protocol_i_parallel_rank_bits(config_.n);
  const auto mask = (UINT64_C(1) << bits) - 1U;
  std::vector<std::uint64_t> ranks(config_.n);
  std::vector<bool> seen(config_.n, false);
  std::vector<ProtocolIBlock192> sorted(config_.n);
  for (std::size_t index = 0; index < ranks.size(); ++index) {
    require((peer_rank_shares[index] & ~mask) == 0,
            "parallel shuffle peer rank share width");
    ranks[index] = (rank_shares_[index] + peer_rank_shares[index]) & mask;
    require(ranks[index] < config_.n && !seen[ranks[index]],
            "parallel shuffle public ranks are not a permutation");
    seen[ranks[index]] = true;
    sorted[ranks[index]] = shuffled_share_[index];
  }
  phase_ = 5;
  return {shuffled_share_, public_masked_, std::move(ranks), std::move(sorted)};
}

ProtocolIParallelShuffleNetworkOutput protocol_i_parallel_shuffle_three_round_party(
    const ProtocolIParallelShufflePartyConfig& config,
    ProtocolIParallelShufflePartyMaterial&& material,
    const std::vector<ProtocolIBlock192>& input_share,
    const std::array<int, 3>& round_fds) {
  ProtocolIParallelShuffleParty party(config, std::move(material));
  ProtocolIParallelShuffleNetworkOutput output;
  const auto m = party.prepare_round1(input_share);
  const auto r1 = exchange_payload(round_fds[0], config, 1, encode_records(m));
  const auto d = party.receive_round1_prepare_round2(
      decode_records(r1.peer, config.n, config.comparison_bits));
  const auto r2 = exchange_payload(round_fds[1], config, 2, encode_records(d));
  (void)party.receive_round2(decode_records(r2.peer, config.n, config.comparison_bits));
  const auto q = party.evaluate_cmpagg_prepare_round3();
  const auto rank_bits = protocol_i_parallel_rank_bits(config.n);
  const auto r3 = exchange_payload(round_fds[2], config, 3, encode_ranks(q, rank_bits));
  output.core = party.receive_round3(decode_ranks(r3.peer, config.n, rank_bits));
  output.metrics.round1_sent_bytes = r1.sent_bytes;
  output.metrics.round1_received_bytes = r1.received_bytes;
  output.metrics.round2_sent_bytes = r2.sent_bytes;
  output.metrics.round2_received_bytes = r2.received_bytes;
  output.metrics.round3_sent_bytes = r3.sent_bytes;
  output.metrics.round3_received_bytes = r3.received_bytes;
  const auto record_bits = static_cast<std::uint64_t>(config.comparison_bits) + 128U;
  output.metrics.round1_logical_sent_bits = static_cast<std::uint64_t>(config.n) * record_bits;
  output.metrics.round2_logical_sent_bits = static_cast<std::uint64_t>(config.n) * record_bits;
  output.metrics.round3_logical_sent_bits = static_cast<std::uint64_t>(config.n) * rank_bits;
  return output;
}

}  // namespace moe_topk
