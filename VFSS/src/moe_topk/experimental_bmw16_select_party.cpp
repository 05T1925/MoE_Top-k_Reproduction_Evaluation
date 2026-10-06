#include <moe_topk/experimental_bmw16_select_party.h>

#include <moe_topk/protocol_i_transport.h>
#include <moe_topk/topk_oracle.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <sys/random.h>

namespace moe_topk {
namespace {

using ItemId = std::uint32_t;
using PairKey = std::pair<ItemId, ItemId>;
using CacheKey = std::tuple<int, ItemId, ItemId>;

struct Item {
  int category = 2;  // S4-derived public ordering class: low, sentinels, real, high.
  std::uint64_t serial = 0;
  bool real = false;
};

struct Task {
  std::string name;
  std::vector<ItemId> input;
  std::vector<ItemId> sample, u, v, w, prefix, suffix, accept, reject;
  ItemId pivot_low = 0, pivot_high = 0, x = 0, y = 0;
  std::size_t h = 0, q = 0, q_w = 0;
  std::size_t x_rank = 0, y_rank = 0;
  std::vector<std::vector<std::uint8_t>> r3_matrix;
  std::size_t sample_size = 0, q_low = 0, q_high = 0, u_cap = 0, w_cap = 0;
  std::size_t gap = 0;
  std::string status = "PENDING", reason;
};

struct Edge {
  int task = 0;
  ItemId left = 0, right = 0;
};

struct Batch {
  int task = 0;
  std::vector<Edge> edges;
  std::vector<std::uint8_t> result;
};

std::vector<std::uint8_t> encode_records(const std::vector<ProtocolIBlock192>& values) {
  std::vector<std::uint8_t> out; out.reserve(values.size() * 24U);
  for (const auto& v : values) for (auto x : {v.word0, v.word1, v.word2})
    for (int shift = 56; shift >= 0; shift -= 8) out.push_back(static_cast<std::uint8_t>(x >> shift));
  return out;
}
std::vector<ProtocolIBlock192> decode_records(const std::vector<std::uint8_t>& in,
                                               std::size_t expected) {
  if (in.size() != expected * 24U) throw std::runtime_error("record exchange length");
  std::vector<ProtocolIBlock192> out(expected); std::size_t p = 0;
  for (auto& v : out) for (auto* x : {&v.word0, &v.word1, &v.word2}) {
    *x = 0; for (int j = 0; j < 8; ++j) *x = (*x << 8U) | in[p++];
  }
  return out;
}
std::vector<std::uint8_t> exchange_frame(int fd, std::uint64_t session,
    std::uint64_t fingerprint, std::uint32_t n, std::uint32_t k, std::uint8_t bits,
    std::uint8_t party, int timeout_ms, std::uint8_t phase,
    const std::vector<std::uint8_t>& payload, std::uint64_t* sent, std::uint64_t* received) {
  ProtocolIFramedChannel channel(fd, {session, fingerprint, n, k, bits, party,
      static_cast<std::uint8_t>(1U - party), phase, 9}, timeout_ms);
  std::vector<std::uint8_t> incoming;
  if (party == 0) { channel.send(payload); incoming = channel.receive(); }
  else { incoming = channel.receive(); channel.send(payload); }
  if (sent) *sent += channel.sent_bytes();
  if (received) *received += channel.received_bytes();
  return incoming;
}
std::vector<std::uint8_t> encode_word_vector(const std::vector<std::uint64_t>& values) {
  std::vector<std::uint8_t> out; out.reserve(values.size() * 8U);
  for (auto x : values) for (int shift = 56; shift >= 0; shift -= 8)
    out.push_back(static_cast<std::uint8_t>(x >> shift));
  return out;
}

struct Runtime {
  const ProtocolIBmw16ExperimentalPartyConfig& c;
  ProtocolIBmw16ExperimentalPartyMaterial& m;
  const std::vector<std::uint64_t>& z;
  ProtocolIBmw16ExperimentalMetrics& metrics;
  std::vector<Item> items;
  std::array<Task, 2> tasks;
  std::string failure_reason;
  std::uint64_t ring_mask = 0;
  std::size_t low_pad = 0, median_size = 0, median_cut = 0;
  std::array<std::array<std::uint64_t, 2>, 2> rng_state{};
  std::set<CacheKey> all_used_edges;

  Runtime(const ProtocolIBmw16ExperimentalPartyConfig& config,
          ProtocolIBmw16ExperimentalPartyMaterial& material,
          const std::vector<std::uint64_t>& public_z,
          ProtocolIBmw16ExperimentalMetrics& output_metrics)
      : c(config), m(material), z(public_z), metrics(output_metrics) {
    if (c.n < 2 || c.n > 64 || c.k == 0 || c.k > c.n || z.size() != c.n)
      throw std::invalid_argument("BMW16 experimental finite domain");
    ring_mask = (UINT64_C(1) << c.comparison_bits) - 1U;
    items.reserve(static_cast<std::size_t>(c.n) * 3U + 8U);
    for (ItemId i = 0; i < c.n; ++i) items.push_back({2, i, true});

    const std::size_t r = static_cast<std::size_t>(c.n) - c.k + 1U;
    const std::size_t N = static_cast<std::size_t>(c.n) + 1U;
    low_pad = N > 2U * r ? N - 2U * r : 0U;
    const std::size_t high_pad = 2U * r > N ? 2U * r - N : 0U;
    const std::size_t M = N + low_pad + high_pad;
    if ((M & 1U) != 0 || M < 2U) throw std::logic_error("S4 median padding parity");
    median_size = M;
    median_cut = M / 2U;

    auto add = [&](int category) {
      const auto id = static_cast<ItemId>(items.size());
      items.push_back({category, static_cast<std::uint64_t>(id), false});
      return id;
    };
    tasks[0].name = "SELECT_LOW_SENTINEL";
    for (std::size_t i = 0; i < low_pad; ++i) tasks[0].input.push_back(add(0));
    tasks[0].input.push_back(add(1));
    for (ItemId i = 0; i < c.n; ++i) tasks[0].input.push_back(i);
    for (std::size_t i = 0; i < high_pad; ++i) tasks[0].input.push_back(add(4));

    tasks[1].name = "SELECT_HIGH_SENTINEL";
    for (std::size_t i = 0; i < low_pad; ++i) tasks[1].input.push_back(add(0));
    for (ItemId i = 0; i < c.n; ++i) tasks[1].input.push_back(i);
    tasks[1].input.push_back(add(3));
    for (std::size_t i = 0; i < high_pad; ++i) tasks[1].input.push_back(add(4));
    if (tasks[0].input.size() != M || tasks[1].input.size() != M ||
        r + low_pad + 1U != median_cut + 1U || r + low_pad != median_cut)
      throw std::logic_error("S4 two-sentinel rank mapping");

    for (int t = 0; t < 2; ++t) {
      auto& task = tasks[t];
      task.h = median_cut;
      const auto s = std::min<std::size_t>(M, ceil_sqrt(64U * M));
      task.sample_size = s;
      if (s == M) {
        task.q_low = M / 2U;
        task.q_high = M / 2U + 1U;
      } else {
        const auto a = ceil_sqrt(16U * s);
        task.q_low = std::max<std::size_t>(1U, (s + 1U) / 2U > a ? (s + 1U) / 2U - a : 1U);
        task.q_high = std::min<std::size_t>(s, (s + 2U) / 2U + a);
      }
      task.u_cap = std::min<std::size_t>(M, ceil_scaled_three_quarter(M, 8U));
      task.gap = ceil_sqrt(4U * M);
      task.w_cap = 2U * task.gap + 1U;
      for (int sample_domain = 0; sample_domain < 2; ++sample_domain)
        rng_state[t][sample_domain] = mix64(c.public_algorithm_seed ^
            (t == 0 ? UINT64_C(0x4c4f5753454c4543) : UINT64_C(0x4849474853454c45)) ^
            (sample_domain == 0 ? UINT64_C(0x52315f53414d504c) : UINT64_C(0x52335f53414d504c)));
    }
  }

  static std::uint64_t mix64(std::uint64_t x) {
    x += UINT64_C(0x9e3779b97f4a7c15);
    x = (x ^ (x >> 30U)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27U)) * UINT64_C(0x94d049bb133111eb);
    return x ^ (x >> 31U);
  }
  std::uint64_t next_random(int task, int domain) {
    auto& x = rng_state[task][domain];
    x += UINT64_C(0x9e3779b97f4a7c15);
    std::uint64_t z0 = x;
    z0 = (z0 ^ (z0 >> 30U)) * UINT64_C(0xbf58476d1ce4e5b9);
    z0 = (z0 ^ (z0 >> 27U)) * UINT64_C(0x94d049bb133111eb);
    return z0 ^ (z0 >> 31U);
  }
  std::uint64_t bounded(int task, int domain, std::uint64_t bound) {
    if (bound == 0) throw std::logic_error("zero sample bound");
    const auto threshold = static_cast<std::uint64_t>(-bound) % bound;
    for (;;) { const auto x = next_random(task, domain); if (x >= threshold) return x % bound; }
  }
  std::vector<ItemId> sample(int task, int domain, const std::vector<ItemId>& source, std::size_t count) {
    if (count > source.size()) throw std::logic_error("S4 sample exceeds source");
    auto pool = source;
    for (std::size_t i = 0; i < count; ++i) {
      const auto j = i + bounded(task, domain, pool.size() - i);
      std::swap(pool[i], pool[j]);
    }
    pool.resize(count);
    return pool;
  }
  static std::size_t ceil_sqrt(std::size_t x) {
    std::size_t lo = 0, hi = 1;
    while (hi * hi < x) hi *= 2U;
    while (lo + 1U < hi) { const auto mid = lo + (hi - lo) / 2U; if (mid * mid >= x) hi = mid; else lo = mid; }
    return hi;
  }
  static std::size_t ceil_scaled_three_quarter(std::size_t m, std::size_t scale) {
    const __uint128_t target = static_cast<__uint128_t>(scale) * scale * scale * scale * m * m * m;
    std::size_t lo = 0, hi = 1;
    auto enough = [&](std::size_t x) { const __uint128_t q = x; return q * q * q * q >= target; };
    while (!enough(hi)) hi *= 2U;
    while (lo + 1U < hi) { const auto mid = lo + (hi - lo) / 2U; if (enough(mid)) hi = mid; else lo = mid; }
    return hi;
  }
  static std::size_t ceil_sqrt_ratio(std::size_t numerator, std::size_t denominator) {
    std::size_t lo = 0, hi = 1;
    while (hi * hi * denominator < numerator) hi *= 2U;
    while (lo + 1U < hi) { const auto mid = lo + (hi - lo) / 2U; if (mid * mid * denominator >= numerator) hi = mid; else lo = mid; }
    return hi;
  }
  static std::size_t pair_index(std::uint32_t n, ItemId lo, ItemId hi) {
    if (!(lo < hi && hi < n)) throw std::logic_error("canonical pair index");
    return static_cast<std::size_t>(lo) * (2U * n - lo - 1U) / 2U + (hi - lo - 1U);
  }
  bool public_less(ItemId a, ItemId b) const {
    if (items[a].category != items[b].category) return items[a].category < items[b].category;
    return items[a].serial < items[b].serial;
  }

  std::vector<std::uint8_t> exchange(int fd, std::uint8_t phase,
      const std::vector<std::uint8_t>& outbound, std::uint64_t* sent, std::uint64_t* received) {
    ProtocolIFramedChannel channel(fd, {c.session, c.fingerprint, c.n, c.k,
        c.comparison_bits, c.party, static_cast<std::uint8_t>(1U - c.party), phase, 9}, c.timeout_ms);
    std::vector<std::uint8_t> inbound;
    if (c.party == 0) { channel.send(outbound); inbound = channel.receive(); }
    else { inbound = channel.receive(); channel.send(outbound); }
    if (sent) *sent += channel.sent_bytes();
    if (received) *received += channel.received_bytes();
    return inbound;
  }
  static std::vector<std::uint8_t> encode_records(const std::vector<ProtocolIBlock192>& values) {
    std::vector<std::uint8_t> out; out.reserve(values.size() * 24U);
    for (const auto& v : values) for (auto x : {v.word0, v.word1, v.word2})
      for (int shift = 56; shift >= 0; shift -= 8) out.push_back(static_cast<std::uint8_t>(x >> shift));
    return out;
  }
  std::vector<ProtocolIBlock192> exchange_records(int fd, std::uint8_t phase,
      const std::vector<ProtocolIBlock192>& values) {
    std::uint64_t sent = 0, received = 0;
    const auto in = exchange(fd, phase, ::moe_topk::encode_records(values), &sent, &received);
    auto out = decode_records(in, values.size());
    metrics.online_bytes_sent += sent; metrics.online_bytes_received += received;
    ++metrics.online_message_phases;
    return out;
  }
  static std::vector<std::uint8_t> encode_words(const std::vector<std::uint64_t>& values) {
    std::vector<std::uint8_t> out; out.reserve(values.size() * 8U);
    for (auto x : values) for (int shift = 56; shift >= 0; shift -= 8) out.push_back(static_cast<std::uint8_t>(x >> shift));
    return out;
  }
  static std::vector<std::uint64_t> decode_words(const std::vector<std::uint8_t>& in, std::size_t expected) {
    if (in.size() != expected * 8U) throw std::runtime_error("share exchange length");
    std::vector<std::uint64_t> out(expected); std::size_t p = 0;
    for (auto& x : out) { x = 0; for (int j = 0; j < 8; ++j) x = (x << 8U) | in[p++]; }
    return out;
  }

  std::vector<std::uint8_t> compare_round(int round, const std::vector<Batch*>& batches,
      int fd) {
    std::vector<std::uint64_t> local;
    std::map<CacheKey, std::uint64_t> cache;
    std::vector<std::pair<Batch*, std::size_t>> offsets;
    std::uint64_t plan_hash = UINT64_C(14695981039346656037);
    auto hash_byte = [&](std::uint8_t byte) { plan_hash = (plan_hash ^ byte) * UINT64_C(1099511628211); };
    for (auto* batch : batches) {
      offsets.push_back({batch, local.size()});
      for (const auto& edge : batch->edges) {
        hash_byte(static_cast<std::uint8_t>(edge.task));
        for (int shift = 24; shift >= 0; shift -= 8) hash_byte(static_cast<std::uint8_t>(edge.left >> shift));
        for (int shift = 24; shift >= 0; shift -= 8) hash_byte(static_cast<std::uint8_t>(edge.right >> shift));
        ++metrics.logical_comparison_calls[round - 1];
        const auto& a = items.at(edge.left); const auto& b = items.at(edge.right);
        if (!a.real || !b.real) {
          ++metrics.dummy_related_calls[round - 1];
          local.push_back(c.party == 0 && public_less(edge.left, edge.right) ? 1U : 0U);
          continue;
        }
        const auto lo = std::min(edge.left, edge.right), hi = std::max(edge.left, edge.right);
        const CacheKey ck{batch->task, lo, hi};
        auto it = cache.find(ck);
        if (it == cache.end()) {
          const auto slot = pair_index(c.n, lo, hi);
          auto& pool = m.select_keys[batch->task][round - 1];
          if (pool.size() != static_cast<std::size_t>(c.n) * (c.n - 1U) / 2U)
            throw std::invalid_argument("Select offline pool coverage");
          auto& key = pool.at(slot);
          if (key.party_id() != c.party || key.comparison_bits() != c.comparison_bits)
            throw std::invalid_argument("Select offline key binding");
          const auto offset = static_cast<std::uint64_t>(low_pad + 1U);
          const auto value = key.eval_strict_lt((z[lo] + offset) & ring_mask,
                                                (z[hi] + offset) & ring_mask);
          it = cache.emplace(ck, value).first;
          ++metrics.unique_select_slots_consumed[round - 1];
          ++metrics.ucmp_party_evaluations;
          metrics.dcf_party_evaluations += 2U;
          all_used_edges.insert(ck);
        } else ++metrics.repeated_logical_calls[round - 1];
        auto share = it->second;
        if (edge.left != lo) share = c.party == 0 ? UINT64_C(1) - share : UINT64_C(0) - share;
        local.push_back(share);
      }
    }
    metrics.edge_plan_fnv64[round - 1] = plan_hash;
    const auto inbound = exchange(fd, static_cast<std::uint8_t>(20 + round), encode_word_vector(local),
                                  &metrics.online_bytes_sent, &metrics.online_bytes_received);
    ++metrics.online_message_phases;
    const auto peer = decode_words(inbound, local.size());
    std::vector<std::uint8_t> opened(local.size());
    for (std::size_t i = 0; i < local.size(); ++i) {
      const auto bit = local[i] + peer[i];
      if (bit > 1U) throw std::runtime_error("opened uCMP share is not a bit");
      opened[i] = static_cast<std::uint8_t>(bit);
    }
    for (const auto& [batch, offset] : offsets) {
      batch->result.assign(opened.begin() + offset, opened.begin() + offset + batch->edges.size());
    }
    return opened;
  }

  std::map<ItemId, std::size_t> ranks(const std::vector<ItemId>& ids, const Batch& batch) const {
    std::map<ItemId, std::size_t> out;
    for (auto id : ids) out[id] = 1U;
    if (batch.result.size() != batch.edges.size()) throw std::logic_error("rank result shape");
    for (std::size_t i = 0; i < batch.edges.size(); ++i) {
      const auto& e = batch.edges[i];
      const ItemId higher = batch.result[i] ? e.right : e.left;
      ++out[higher];
    }
    return out;
  }

  void round1(int fd) {
    std::array<Batch, 2> batch;
    for (int t = 0; t < 2; ++t) {
      auto& state = tasks[t]; state.sample = sample(t, 0, state.input, state.sample_size);
      batch[t].task = t;
      for (std::size_t i = 0; i < state.sample.size(); ++i)
        for (std::size_t j = i + 1U; j < state.sample.size(); ++j)
          batch[t].edges.push_back({t, state.sample[i], state.sample[j]});
    }
    compare_round(1, {&batch[0], &batch[1]}, fd);
    for (int t = 0; t < 2; ++t) {
      auto& state = tasks[t]; const auto rank = ranks(state.sample, batch[t]);
      std::map<std::size_t, ItemId> by_rank;
      bool unique = true;
      for (const auto& [id, r] : rank) unique &= by_rank.emplace(r, id).second;
      if (!unique || !by_rank.count(state.q_low) || !by_rank.count(state.q_high) || state.q_low >= state.q_high) {
        state.status = "ABORT_ALGORITHM_INVALID"; state.reason = "R1_SAMPLE_RANK_NOT_UNIQUE";
      } else { state.pivot_low = by_rank.at(state.q_low); state.pivot_high = by_rank.at(state.q_high); }
    }
  }

  void round2(int fd) {
    std::array<Batch, 2> batch; std::vector<Batch*> active;
    std::array<std::size_t, 2> split{};
    for (int t = 0; t < 2; ++t) {
      auto& state = tasks[t]; if (state.status != "PENDING") continue;
      batch[t].task = t;
      for (auto id : state.input) if (id != state.pivot_low) batch[t].edges.push_back({t, id, state.pivot_low});
      split[t] = batch[t].edges.size();
      for (auto id : state.input) if (id != state.pivot_high) batch[t].edges.push_back({t, id, state.pivot_high});
      active.push_back(&batch[t]);
    }
    compare_round(2, active, fd);
    for (int t = 0; t < 2; ++t) {
      auto& state = tasks[t]; if (state.status != "PENDING") continue;
      std::map<ItemId, bool> lx, ly;
      for (std::size_t i = 0; i < split[t]; ++i) lx[batch[t].edges[i].left] = batch[t].result[i] != 0;
      for (std::size_t i = split[t]; i < batch[t].edges.size(); ++i) ly[batch[t].edges[i].left] = batch[t].result[i] != 0;
      std::size_t c_low = 0, c_high = 0;
      for (auto id : state.input) {
        const bool a = lx.count(id) ? lx[id] : false;
        const bool b = ly.count(id) ? ly[id] : false;
        c_low += a; c_high += b;
        if (a) state.prefix.push_back(id);
        if (!b && id != state.pivot_high) state.suffix.push_back(id);
        if (!a && (b || id == state.pivot_high)) state.u.push_back(id);
      }
      state.q = state.h - std::min(state.h, c_low);
      if (!(c_low <= state.h && state.h <= c_high)) {
        state.status = "ABORT_ALGORITHM_PROBABILITY"; state.reason = "R2_PIVOTS_NOT_BRACKETED_MEDIAN";
      } else if (state.u.size() > state.u_cap) {
        state.status = "ABORT_ALGORITHM_PROBABILITY"; state.reason = "R2_U_OVERSIZE";
      } else state.status = "R2_READY";
    }
  }

  void round3(int fd) {
    std::array<Batch, 2> batch; std::vector<Batch*> active;
    for (int t = 0; t < 2; ++t) {
      auto& state = tasks[t]; if (state.status != "R2_READY") continue;
      const auto u = state.u.size();
      if (u == 0 || state.q >= u) { state.status = "ABORT_ALGORITHM_INVALID"; state.reason = "R2_RESIDUAL_RANK_INVALID"; continue; }
      if (state.q == 0) {
        state.reject = state.prefix; state.accept = state.suffix;
        state.accept.insert(state.accept.end(), state.u.begin(), state.u.end());
        state.status = "SUCCESS"; state.reason = "R2_EXACT_RESIDUAL_ENDPOINT"; continue;
      }
      const auto vsize = std::min<std::size_t>(u, ceil_sqrt_ratio(64U * u * u, median_size));
      state.v = sample(t, 1, state.u, vsize);
      std::set<ItemId> vset(state.v.begin(), state.v.end());
      std::vector<ItemId> other;
      for (auto id : state.u) if (!vset.count(id)) other.push_back(id);
      batch[t].task = t;
      for (std::size_t i = 0; i < state.v.size(); ++i)
        for (std::size_t j = i + 1U; j < state.v.size(); ++j)
          batch[t].edges.push_back({t, state.v[i], state.v[j]});
      for (auto a : state.v) for (auto b : other) batch[t].edges.push_back({t, a, b});
      active.push_back(&batch[t]);
    }
    compare_round(3, active, fd);
    for (int t = 0; t < 2; ++t) {
      auto& state = tasks[t]; if (state.status != "R2_READY") continue;
      const auto v = state.v.size(), u = state.u.size();
      const auto internal = v * (v - 1U) / 2U;
      std::map<ItemId, std::size_t> upos, vpos;
      for (std::size_t i = 0; i < u; ++i) upos[state.u[i]] = i;
      for (std::size_t i = 0; i < v; ++i) vpos[state.v[i]] = i;
      state.r3_matrix.assign(v, std::vector<std::uint8_t>(u, 0));
      for (std::size_t i = 0; i < v; ++i) state.r3_matrix[i][upos[state.v[i]]] = 0;
      for (std::size_t index = 0; index < internal; ++index) {
        const auto& e = batch[t].edges[index]; const auto i = vpos[e.left], j = vpos[e.right];
        state.r3_matrix[i][upos[e.right]] = batch[t].result[index];
        state.r3_matrix[j][upos[e.left]] = static_cast<std::uint8_t>(!batch[t].result[index]);
      }
      for (std::size_t index = internal; index < batch[t].edges.size(); ++index) {
        const auto& e = batch[t].edges[index];
        state.r3_matrix[vpos[e.left]][upos[e.right]] = batch[t].result[index];
      }
      std::map<ItemId, std::size_t> vrank;
      for (auto id : state.v) {
        std::size_t rank = 0;
        for (auto x : state.r3_matrix[vpos[id]]) rank += !x;
        if (rank == 0) throw std::logic_error("R3 rank underflow");
        vrank[id] = rank;
      }
      std::size_t i = 0, j = u + 1U;
      for (const auto& [id, rank] : vrank) { (void)id; if (rank <= state.q) i = std::max(i, rank); else j = std::min(j, rank); }
      bool have_x = false, have_y = false;
      for (const auto& [id, rank] : vrank) {
        if (rank == i && i != 0) { state.x = id; have_x = true; }
        if (rank == j && j != u + 1U) { state.y = id; have_y = true; }
      }
      state.x_rank = i; state.y_rank = j;
      for (std::size_t col = 0; col < u; ++col) {
        const auto id = state.u[col];
        const bool at_or_above_x = !have_x || id == state.x || state.r3_matrix[vpos[state.x]][col];
        const bool at_or_below_y = !have_y || id == state.y || !state.r3_matrix[vpos[state.y]][col];
        if (at_or_above_x && at_or_below_y) state.w.push_back(id);
      }
      state.q_w = state.q - (i ? i - 1U : 0U);
      if (!(i <= state.q && state.q < j && j <= u + 1U && state.q_w >= 1U && state.q_w <= state.w.size())) {
        state.status = "ABORT_ALGORITHM_INVALID"; state.reason = "R3_RESIDUAL_RANK_NOT_BRACKETED";
      } else if (state.w.size() > state.w_cap) {
        state.status = "ABORT_ALGORITHM_PROBABILITY"; state.reason = "R3_W_OVERSIZE";
      } else state.status = "R3_READY";
    }
  }

  void round4(int fd) {
    std::array<Batch, 2> batch; std::vector<Batch*> active;
    for (int t = 0; t < 2; ++t) {
      auto& state = tasks[t]; if (state.status != "R3_READY") continue;
      batch[t].task = t;
      for (std::size_t i = 0; i < state.w.size(); ++i)
        for (std::size_t j = i + 1U; j < state.w.size(); ++j)
          batch[t].edges.push_back({t, state.w[i], state.w[j]});
      active.push_back(&batch[t]);
    }
    compare_round(4, active, fd);
    for (int t = 0; t < 2; ++t) {
      auto& state = tasks[t]; if (state.status != "R3_READY") continue;
      const auto rank = ranks(state.w, batch[t]); std::set<std::size_t> rank_set;
      for (const auto& [id, r] : rank) { (void)id; rank_set.insert(r); }
      std::vector<ItemId> reject_w;
      for (auto id : state.w) if (rank.at(id) <= state.q_w) reject_w.push_back(id);
      if (rank_set.size() != state.w.size() || reject_w.size() != state.q_w) {
        state.status = "ABORT_ALGORITHM_INVALID"; state.reason = "R4_PAIRWISE_RANKS_NOT_TOTAL"; continue;
      }
      std::set<ItemId> reject_w_set(reject_w.begin(), reject_w.end());
      std::vector<ItemId> below, above;
      auto vp = std::find(state.v.begin(), state.v.end(), state.x);
      if (state.x_rank != 0 && vp != state.v.end()) {
        const auto row = static_cast<std::size_t>(vp - state.v.begin());
        for (std::size_t j = 0; j < state.u.size(); ++j)
          if (state.u[j] != state.x && !state.r3_matrix[row][j]) below.push_back(state.u[j]);
      }
      vp = std::find(state.v.begin(), state.v.end(), state.y);
      if (state.y_rank != state.u.size() + 1U && vp != state.v.end()) {
        const auto row = static_cast<std::size_t>(vp - state.v.begin());
        for (std::size_t j = 0; j < state.u.size(); ++j)
          if (state.u[j] != state.y && state.r3_matrix[row][j]) above.push_back(state.u[j]);
      }
      state.reject = state.prefix; state.reject.insert(state.reject.end(), below.begin(), below.end());
      state.reject.insert(state.reject.end(), reject_w.begin(), reject_w.end());
      state.accept = state.suffix; state.accept.insert(state.accept.end(), above.begin(), above.end());
      for (auto id : state.w) if (!reject_w_set.count(id)) state.accept.push_back(id);
      std::set<ItemId> union_ids(state.reject.begin(), state.reject.end());
      union_ids.insert(state.accept.begin(), state.accept.end());
      bool disjoint = true;
      for (auto id : state.accept) if (std::find(state.reject.begin(), state.reject.end(), id) != state.reject.end()) disjoint = false;
      if (state.reject.size() != state.h || state.accept.size() != median_size - state.h ||
          union_ids.size() != median_size || !disjoint) {
        state.status = "ABORT_ALGORITHM_INVALID"; state.reason = "R4_PARTITION_CARDINALITY";
      } else { state.status = "SUCCESS"; state.reason = "R4_EXACT_RANK"; }
    }
  }

  std::pair<std::string, ItemId> run(const std::array<int, 4>& fds) {
    round1(fds[0]); round2(fds[1]); round3(fds[2]); round4(fds[3]);
    for (const auto& state : tasks) if (state.status != "SUCCESS") {
      failure_reason = state.name + ":" + state.reason;
      return {state.status == "ABORT_ALGORITHM_PROBABILITY" ? state.status : "ABORT_ALGORITHM_INVALID", 0};
    }
    std::set<ItemId> low_accept(tasks[0].accept.begin(), tasks[0].accept.end());
    std::vector<ItemId> candidate;
    for (auto id : tasks[1].reject) if (low_accept.count(id) && id < c.n) candidate.push_back(id);
    if (candidate.size() != 1) { failure_reason = "TWO_PARTITION_INTERSECTION_NOT_UNIQUE"; return {"ABORT_ALGORITHM_INVALID", 0}; }
    return {"SUCCESS", candidate.front()};
  }
};

std::uint64_t random64() {
  std::uint64_t value = 0; auto* out = reinterpret_cast<std::uint8_t*>(&value); std::size_t left = sizeof(value);
  while (left) { const auto n = ::getrandom(out, left, 0); if (n < 0 && errno == EINTR) continue; if (n <= 0) throw std::runtime_error("BMW16 material entropy"); out += n; left -= static_cast<std::size_t>(n); }
  return value;
}
std::uint64_t random_bounded(std::uint64_t bound) {
  if (!bound) throw std::invalid_argument("random bound");
  const auto threshold = static_cast<std::uint64_t>(-bound) % bound;
  for (;;) { const auto x = random64(); if (x >= threshold) return x % bound; }
}
std::uint64_t ring_mask(int bits) { return (UINT64_C(1) << bits) - 1U; }
std::vector<ProtocolIBlock192> random_records(std::uint32_t n, int bits) {
  std::vector<ProtocolIBlock192> out(n);
  for (auto& x : out) x = {random64() & ring_mask(bits), random64(), random64()};
  return out;
}
ProtocolIPermutation random_permutation(std::uint32_t n) {
  auto out = protocol_i_identity_permutation(n);
  for (std::uint32_t r = n; r > 1; --r) std::swap(out[r - 1U], out[random_bounded(r)]);
  return out;
}
std::vector<ProtocolIBlock192> add_vectors(int bits, const std::vector<ProtocolIBlock192>& a,
                                           const std::vector<ProtocolIBlock192>& b) {
  auto out = a; if (a.size() != b.size()) throw std::invalid_argument("record vector shape");
  for (std::size_t i = 0; i < out.size(); ++i) out[i] = protocol_i_parallel_record_add(bits, out[i], b[i]); return out;
}
std::vector<ProtocolIBlock192> sub_vectors(int bits, const std::vector<ProtocolIBlock192>& a,
                                           const std::vector<ProtocolIBlock192>& b) {
  auto out = a; if (a.size() != b.size()) throw std::invalid_argument("record vector shape");
  for (std::size_t i = 0; i < out.size(); ++i) out[i] = protocol_i_parallel_record_sub(bits, out[i], b[i]); return out;
}

ProtocolIParallelShuffleDealerOutput make_inverse_shuffle(
    const ProtocolIParallelShuffleDealerConfig& cfg, const ProtocolIPermutation& pi) {
  const auto sigma0 = random_permutation(cfg.n), sigma1 = random_permutation(cfg.n);
  const auto tau0 = protocol_i_compose_permutation(pi, protocol_i_inverse_permutation(sigma1));
  const auto tau1 = protocol_i_compose_permutation(pi, protocol_i_inverse_permutation(sigma0));
  const auto a0 = random_records(cfg.n, cfg.comparison_bits), a1 = random_records(cfg.n, cfg.comparison_bits);
  const auto h = random_records(cfg.n, cfg.comparison_bits), r0 = random_records(cfg.n, cfg.comparison_bits),
             r1 = random_records(cfg.n, cfg.comparison_bits);
  const std::vector<ProtocolIBlock192> zero(cfg.n);
  const auto e0 = sub_vectors(cfg.comparison_bits,
      sub_vectors(cfg.comparison_bits, zero, protocol_i_apply_permutation(tau0, a1)), h);
  const auto e1 = add_vectors(cfg.comparison_bits,
      sub_vectors(cfg.comparison_bits, zero, protocol_i_apply_permutation(tau1, a0)), h);
  ProtocolIParallelShuffleDealerOutput out;
  auto init = [&](ProtocolIParallelShufflePartyMaterial& m, int party, const ProtocolIPermutation& s,
      const ProtocolIPermutation& t, const std::vector<ProtocolIBlock192>& a,
      const std::vector<ProtocolIBlock192>& e, const std::vector<ProtocolIBlock192>& r) {
    m.session = cfg.session; m.fingerprint = cfg.fingerprint; m.material_id = cfg.material_id;
    m.n = cfg.n; m.k = cfg.k; m.comparison_bits = cfg.comparison_bits; m.party = party;
    m.sigma = s; m.tau = t; m.a = a; m.e = e; m.r_share = r;
  };
  init(out.party0, 0, sigma0, tau0, a0, e0, r0); init(out.party1, 1, sigma1, tau1, a1, e1, r1);
  const auto full_r = add_vectors(cfg.comparison_bits, r0, r1);
  for (std::uint32_t lo = 0; lo < cfg.n; ++lo) for (std::uint32_t hi = lo + 1U; hi < cfg.n; ++hi) {
    ProtocolIUcmpMaterial key(cfg.comparison_bits, full_r[lo].word0, full_r[hi].word0);
    out.party0.edge_materials.push_back(key.export_party_material(0));
    out.party1.edge_materials.push_back(key.export_party_material(1));
  }
  return out;
}

void make_score_package_pair(const ProtocolIBmw16ExperimentalPartyConfig& c,
    ProtocolIPartyPackage& p0, ProtocolIPartyPackage& p1) {
  const std::uint32_t padded = c.padded_n; const auto mask = (UINT64_C(1) << 34U) - 1U;
  for (std::uint8_t stage = 1; stage <= 2; ++stage) for (std::uint32_t slot = 0; slot < padded; ++slot) {
    const auto ml = random64() & mask, mr = random64() & mask;
    const auto l0 = random64() & mask, r0 = random64() & mask;
    ProtocolIUcmpMaterial key(34, ml, mr);
    const auto l1 = (ml - l0) & mask, r1 = (mr - r0) & mask;
    auto k0 = key.export_party_material(0), k1 = key.export_party_material(1);
    if (stage == 1) {
      p0.carry_materials.emplace_back(slot, stage, l0, r0, std::move(k0));
      p1.carry_materials.emplace_back(slot, stage, l1, r1, std::move(k1));
    } else {
      p0.sign_materials.emplace_back(slot, stage, l0, r0, std::move(k0));
      p1.sign_materials.emplace_back(slot, stage, l1, r1, std::move(k1));
    }
  }
}

void configure_score_packages(const ProtocolIBmw16ExperimentalPartyConfig& c,
    ProtocolIPartyPackage& p0, ProtocolIPartyPackage& p1) {
  for (int party = 0; party < 2; ++party) {
    auto& p = party == 0 ? p0 : p1;
    p.session = c.session; p.fingerprint = c.fingerprint; p.party = party;
    p.n = c.padded_n; p.k = c.k; p.comparison_bits = c.comparison_bits;
  }
}

}  // namespace

ProtocolIBmw16ExperimentalMaterialPair protocol_i_bmw16_experimental_material_generate(
    const ProtocolIBmw16ExperimentalPartyConfig& c) {
  if (c.session == 0 || c.fingerprint == 0 || c.n < 2 || c.n > 64 || c.k == 0 || c.k > c.n ||
      c.padded_n < c.n || (c.padded_n & (c.padded_n - 1U)) != 0 ||
      c.comparison_bits != 33U + c.index_bits || c.comparison_bits > 53)
    throw std::invalid_argument("BMW16 offline generator configuration");
  const ProtocolIParallelShuffleDealerConfig fcfg{c.session, c.fingerprint,
      c.session ^ UINT64_C(0x8100000000000001), c.n, c.k, c.comparison_bits};
  auto forward = protocol_i_parallel_shuffle_dealer_generate(fcfg);
  const auto pi = protocol_i_compose_permutation(forward.party0.tau, forward.party1.sigma);
  auto icfg = fcfg; icfg.material_id = c.session ^ UINT64_C(0x8200000000000001);
  auto inverse = make_inverse_shuffle(icfg, protocol_i_inverse_permutation(pi));

  ProtocolIBmw16ExperimentalMaterialPair out;
  configure_score_packages(c, out.party0.score_input, out.party1.score_input);
  make_score_package_pair(c, out.party0.score_input, out.party1.score_input);
  out.party0.forward_shuffle = std::move(forward.party0);
  out.party1.forward_shuffle = std::move(forward.party1);
  out.party0.inverse_shuffle = std::move(inverse.party0);
  out.party1.inverse_shuffle = std::move(inverse.party1);
  const auto pair_count = static_cast<std::size_t>(c.n) * (c.n - 1U) / 2U;
  for (int task = 0; task < 2; ++task) for (int round = 0; round < 4; ++round) {
    auto& a = out.party0.select_keys[task][round]; auto& b = out.party1.select_keys[task][round];
    a.reserve(pair_count); b.reserve(pair_count);
    for (std::uint32_t lo = 0; lo < c.n; ++lo) for (std::uint32_t hi = lo + 1U; hi < c.n; ++hi) {
      const auto mask = ring_mask(c.comparison_bits);
      const auto rl = (out.party0.forward_shuffle.r_share[lo].word0 + out.party1.forward_shuffle.r_share[lo].word0) & mask;
      const auto rh = (out.party0.forward_shuffle.r_share[hi].word0 + out.party1.forward_shuffle.r_share[hi].word0) & mask;
      ProtocolIUcmpMaterial key(c.comparison_bits, rl, rh);
      a.push_back(key.export_party_material(0)); b.push_back(key.export_party_material(1));
    }
  }
  out.party0.membership_keys.reserve(pair_count); out.party1.membership_keys.reserve(pair_count);
  for (std::uint32_t lo = 0; lo < c.n; ++lo) for (std::uint32_t hi = lo + 1U; hi < c.n; ++hi) {
    const auto mask = ring_mask(c.comparison_bits);
    const auto rl = (out.party0.forward_shuffle.r_share[lo].word0 + out.party1.forward_shuffle.r_share[lo].word0) & mask;
    const auto rh = (out.party0.forward_shuffle.r_share[hi].word0 + out.party1.forward_shuffle.r_share[hi].word0) & mask;
    ProtocolIUcmpMaterial key(c.comparison_bits, rl, rh);
    out.party0.membership_keys.push_back(key.export_party_material(0));
    out.party1.membership_keys.push_back(key.export_party_material(1));
  }
  for (int party = 0; party < 2; ++party) {
    auto& p = party == 0 ? out.party0 : out.party1;
    out.offline_material_slots_per_party[party] = 2U * c.padded_n + pair_count * 11U;
    std::uint64_t byte_count = 0;
    for (const auto& task : p.select_keys) for (const auto& round : task)
      for (const auto& key : round) byte_count += key.serialize().size();
    for (const auto& key : p.membership_keys) byte_count += key.serialize().size();
    for (const auto& key : p.score_input.carry_materials) byte_count += key.material.serialize().size();
    for (const auto& key : p.score_input.sign_materials) byte_count += key.material.serialize().size();
    const auto& shuffle = party == 0 ? p.forward_shuffle : p.inverse_shuffle;
    byte_count += protocol_i_parallel_shuffle_serialize_material(shuffle).size();
    const auto& other = party == 0 ? p.inverse_shuffle : p.forward_shuffle;
    byte_count += protocol_i_parallel_shuffle_serialize_material(other).size();
    out.primitive_key_and_shuffle_bytes_per_party[party] = byte_count;
  }
  return out;
}

ProtocolIBmw16ExperimentalPartyOutput protocol_i_bmw16_experimental_raw_score_mask_party(
    const ProtocolIBmw16ExperimentalPartyConfig& c,
    ProtocolIBmw16ExperimentalPartyMaterial&& material,
    const std::vector<std::uint32_t>& raw_score_share,
    const std::array<int, 2>& score_fds,
    const std::array<int, 2>& forward_shuffle_fds,
    const std::array<int, 4>& select_round_fds,
    const std::array<int, 2>& inverse_shuffle_fds,
    int final_agreement_fd) {
  if (c.party > 1 || c.n == 0 || c.k == 0 || c.k > c.n || c.timeout_ms <= 0 ||
      c.comparison_bits != 33U + c.index_bits || c.comparison_bits > 53 ||
      raw_score_share.size() != c.n)
    throw std::invalid_argument("BMW16 experimental party configuration");
  ProtocolIBmw16ExperimentalPartyOutput output;
  if (c.n == 1) {
    output.status = "SUCCESS";
    output.xor_mask_share = {static_cast<std::uint8_t>(c.party == 0)};
    return output;
  }
  ProtocolIScoreInputMetrics score_metrics;
  const auto stable_share = protocol_i_bmw16_experimental_select_key_party(
      {c.session, c.fingerprint, c.n, c.padded_n, c.k, c.index_bits,
       c.comparison_bits, c.party, c.timeout_ms}, material.score_input,
      raw_score_share, score_fds, &score_metrics);
  const auto ring = (UINT64_C(1) << c.comparison_bits) - 1U;
  std::vector<ProtocolIBlock192> input(c.n);
  for (std::size_t i = 0; i < input.size(); ++i) input[i] = {stable_share[i] & ring, 0, 0};

  ProtocolIParallelShufflePartyConfig fcfg{c.session, c.fingerprint,
      material.forward_shuffle.material_id, c.n, c.k, c.comparison_bits, c.party, c.timeout_ms};
  ProtocolIParallelShuffleParty fwd(fcfg, std::move(material.forward_shuffle));
  auto r1 = fwd.prepare_round1(input);
  auto p1b = exchange_frame(forward_shuffle_fds[0], c.session, c.fingerprint, c.n, c.k,
      c.comparison_bits, c.party, c.timeout_ms, 10, encode_records(r1),
      &output.metrics.online_bytes_sent, &output.metrics.online_bytes_received);
  ++output.metrics.online_message_phases;
  auto r2 = fwd.receive_round1_prepare_round2(decode_records(p1b, c.n));
  auto p2b = exchange_frame(forward_shuffle_fds[1], c.session, c.fingerprint, c.n, c.k,
      c.comparison_bits, c.party, c.timeout_ms, 11, encode_records(r2),
      &output.metrics.online_bytes_sent, &output.metrics.online_bytes_received);
  ++output.metrics.online_message_phases;
  auto shuffled = fwd.receive_round2(decode_records(p2b, c.n));
  std::vector<std::uint64_t> public_z(c.n);
  for (std::size_t i = 0; i < c.n; ++i) public_z[i] = shuffled.public_masked_records[i].word0;

  Runtime select(c, material, public_z, output.metrics);
  const auto selected = select.run(select_round_fds);
  std::string status = selected.first;
  if (c.test_only_force_probability_abort_after_select && status == "SUCCESS") {
    status = "ABORT_ALGORITHM_PROBABILITY";
    select.failure_reason = "TEST_ONLY_FORCED_ABORT_AFTER_SELECT";
  }
  if (status == "SUCCESS") {
    std::vector<std::uint64_t> membership(c.n);
    const auto chosen = selected.second;
    for (std::uint32_t i = 0; i < c.n; ++i) {
      if (i == chosen) { membership[i] = c.party == 0 ? 1U : 0U; continue; }
      const auto lo = std::min<ItemId>(i, chosen), hi = std::max<ItemId>(i, chosen);
      const auto slot = Runtime::pair_index(c.n, lo, hi);
      auto& key = material.membership_keys.at(slot);
      if (key.party_id() != c.party || key.comparison_bits() != c.comparison_bits)
        throw std::invalid_argument("membership material binding");
      auto share = key.eval_strict_lt(public_z[lo], public_z[hi]);
      ++output.metrics.membership_slots_consumed;
      ++output.metrics.ucmp_party_evaluations; output.metrics.dcf_party_evaluations += 2U;
      if (i == lo) share = c.party == 0 ? UINT64_C(1) - share : UINT64_C(0) - share;
      membership[i] = share & ring;
    }
    std::vector<ProtocolIBlock192> membership_records(c.n);
    for (std::size_t i = 0; i < c.n; ++i) membership_records[i] = {membership[i], 0, 0};
    ProtocolIParallelShufflePartyConfig icfg{c.session, c.fingerprint,
        material.inverse_shuffle.material_id, c.n, c.k, c.comparison_bits, c.party, c.timeout_ms};
    ProtocolIParallelShuffleParty inverse(icfg, std::move(material.inverse_shuffle));
    auto ir1 = inverse.prepare_round1(membership_records);
    auto ip1b = exchange_frame(inverse_shuffle_fds[0], c.session, c.fingerprint, c.n, c.k,
        c.comparison_bits, c.party, c.timeout_ms, 31, encode_records(ir1),
        &output.metrics.online_bytes_sent, &output.metrics.online_bytes_received);
    ++output.metrics.online_message_phases;
    auto ir2 = inverse.receive_round1_prepare_round2(decode_records(ip1b, c.n));
    auto ip2b = exchange_frame(inverse_shuffle_fds[1], c.session, c.fingerprint, c.n, c.k,
        c.comparison_bits, c.party, c.timeout_ms, 32, encode_records(ir2),
        &output.metrics.online_bytes_sent, &output.metrics.online_bytes_received);
    ++output.metrics.online_message_phases;
    auto mask_records = inverse.receive_round2(decode_records(ip2b, c.n));
    output.xor_mask_share.resize(c.n);
    for (std::size_t i = 0; i < c.n; ++i)
      output.xor_mask_share[i] = static_cast<std::uint8_t>(mask_records.shuffled_share[i].word0 & 1U);
  }

  const std::uint8_t status_code = status == "SUCCESS" ? 0U :
      status == "ABORT_ALGORITHM_PROBABILITY" ? 1U : 2U;
  const auto remote_status = exchange_frame(final_agreement_fd, c.session, c.fingerprint,
      c.n, c.k, c.comparison_bits, c.party, c.timeout_ms, 40, {status_code},
      &output.metrics.online_bytes_sent, &output.metrics.online_bytes_received);
  ++output.metrics.online_message_phases;
  if (remote_status != std::vector<std::uint8_t>{status_code}) {
    output.status = "ABORT_MATERIAL"; output.abort_reason = "FINAL_STATUS_MISMATCH"; output.xor_mask_share.clear();
  } else if (status == "SUCCESS") output.status = "SUCCESS";
  else if (status == "ABORT_ALGORITHM_PROBABILITY") {
    output.status = "ABORT_ALGORITHM_PROBABILITY"; output.abort_reason = select.failure_reason;
  } else { output.status = "ABORT_ALGORITHM_INVALID"; output.abort_reason = select.failure_reason; output.xor_mask_share.clear(); }
  output.metrics.online_bytes_sent += score_metrics.carry_sent_bytes + score_metrics.sign_sent_bytes;
  output.metrics.online_bytes_received += score_metrics.carry_received_bytes + score_metrics.sign_received_bytes;
  output.metrics.online_message_phases += 2U;
  output.metrics.ucmp_party_evaluations += score_metrics.ucmp_calls;
  output.metrics.dcf_party_evaluations += score_metrics.raw_dcf_calls;
  const auto pairs = static_cast<std::uint64_t>(c.n) * (c.n - 1U) / 2U;
  output.metrics.forward_shuffle_cmpagg_slots_unused = pairs;
  output.metrics.inverse_shuffle_cmpagg_slots_unused = pairs;
  return output;
}

}  // namespace moe_topk
