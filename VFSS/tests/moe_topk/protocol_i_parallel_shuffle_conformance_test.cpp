#include <moe_topk/protocol_i_parallel_shuffle.h>
#include <moe_topk/protocol_i_priority_key.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
using namespace moe_topk;

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

std::vector<ProtocolIBlock192> add_records(int bits,
    const std::vector<ProtocolIBlock192>& left,
    const std::vector<ProtocolIBlock192>& right) {
  require(left.size() == right.size(), "test record shape");
  auto result = left;
  for (std::size_t index = 0; index < result.size(); ++index)
    result[index] = protocol_i_parallel_record_add(bits, result[index], right[index]);
  return result;
}

std::vector<ProtocolIBlock192> split_records(int bits,
    const std::vector<ProtocolIBlock192>& clear, std::mt19937_64& rng,
    std::vector<ProtocolIBlock192>& second) {
  const auto mask = (UINT64_C(1) << bits) - 1U;
  std::vector<ProtocolIBlock192> first(clear.size());
  second.resize(clear.size());
  for (std::size_t index = 0; index < clear.size(); ++index) {
    first[index] = {rng() & mask, rng(), rng()};
    second[index] = protocol_i_parallel_record_sub(bits, clear[index], first[index]);
  }
  return first;
}

std::vector<std::uint64_t> clear_ranks(const std::vector<ProtocolIBlock192>& records) {
  std::vector<std::size_t> order(records.size());
  for (std::size_t index = 0; index < order.size(); ++index) order[index] = index;
  std::sort(order.begin(), order.end(), [&](auto left, auto right) {
    return records[left].word0 < records[right].word0;
  });
  std::vector<std::uint64_t> ranks(records.size());
  for (std::size_t rank = 0; rank < order.size(); ++rank) ranks[order[rank]] = rank;
  return ranks;
}

std::vector<ProtocolIBlock192> clear_sorted(std::vector<ProtocolIBlock192> records) {
  std::sort(records.begin(), records.end(), [](const auto& left, const auto& right) {
    return left.word0 < right.word0;
  });
  return records;
}

void run_correctness_case(std::uint32_t n, std::uint64_t serial, bool duplicate_scores) {
  const int bits = std::max<int>(34, 32 + protocol_i_index_bits(n) + 1);
  const ProtocolIParallelShuffleDealerConfig dealer_config{
      UINT64_C(0x50530000) + serial, UINT64_C(0x50540000) + serial,
      UINT64_C(0x50550000) + serial, n, std::max<std::uint32_t>(1, n / 2),
      static_cast<std::uint8_t>(bits)};
  auto generated = protocol_i_parallel_shuffle_dealer_generate(dealer_config);
  const auto pi0 = protocol_i_compose_permutation(generated.party0.tau,
                                                   generated.party1.sigma);
  const auto pi1 = protocol_i_compose_permutation(generated.party1.tau,
                                                   generated.party0.sigma);
  require(pi0 == pi1, "double factorization");
  const auto r = add_records(bits, generated.party0.r_share, generated.party1.r_share);

  std::vector<ProtocolIBlock192> clear(n);
  std::mt19937_64 rng(UINT64_C(0x20260923) + serial);
  for (std::size_t index = 0; index < clear.size(); ++index) {
    const auto raw_score = duplicate_scores ? static_cast<std::uint32_t>(index % 2 == 0 ? 7 : 7)
                                            : static_cast<std::uint32_t>(rng());
    clear[index] = {protocol_i_priority_key(raw_score, index, n).value,
                    static_cast<std::uint64_t>(index), rng()};
  }
  clear.front().word2 = 0;
  clear.back().word2 = UINT64_MAX;
  std::vector<ProtocolIBlock192> input1;
  auto input0 = split_records(bits, clear, rng, input1);

  ProtocolIParallelShufflePartyConfig config0{dealer_config.session, dealer_config.fingerprint,
      dealer_config.material_id, n, dealer_config.k, dealer_config.comparison_bits, 0, 2000};
  auto config1 = config0; config1.party = 1;
  ProtocolIParallelShuffleParty party0(config0, std::move(generated.party0));
  ProtocolIParallelShuffleParty party1(config1, std::move(generated.party1));

  const auto m0 = party0.prepare_round1(input0);
  const auto m1 = party1.prepare_round1(input1);
  const auto d0 = party0.receive_round1_prepare_round2(m1);
  const auto d1 = party1.receive_round1_prepare_round2(m0);
  const auto round20 = party0.receive_round2(d1);
  const auto round21 = party1.receive_round2(d0);
  const auto shuffled = protocol_i_apply_permutation(pi0, clear);
  require(add_records(bits, round20.shuffled_share, round21.shuffled_share) == shuffled,
          "parallel secret shuffle correctness");
  require(round20.public_masked_records == round21.public_masked_records &&
              round20.public_masked_records == add_records(bits, shuffled, r),
          "parallel public y correctness");

  const auto q0 = party0.evaluate_cmpagg_prepare_round3();
  const auto q1 = party1.evaluate_cmpagg_prepare_round3();
  const auto output0 = party0.receive_round3(q1);
  const auto output1 = party1.receive_round3(q0);
  require(output0.public_ranks == output1.public_ranks &&
              output0.public_ranks == clear_ranks(shuffled),
          "real CmpAgg rank correctness");
  require(add_records(bits, output0.sorted_share, output1.sorted_share) == clear_sorted(clear),
          "parallel local routing correctness");
}

using Perm = std::array<unsigned, 4>;
std::vector<Perm> all_permutations4() {
  Perm value{{0,1,2,3}};
  std::vector<Perm> output;
  do output.push_back(value); while (std::next_permutation(value.begin(), value.end()));
  return output;
}

Perm inverse4(const Perm& permutation) {
  Perm output{};
  for (unsigned index = 0; index < 4; ++index) output[permutation[index]] = index;
  return output;
}

Perm compose4(const Perm& outer, const Perm& inner) {
  Perm output{};
  for (unsigned index = 0; index < 4; ++index) output[index] = inner[outer[index]];
  return output;
}

void permutation_independence() {
  const auto permutations = all_permutations4();
  using Pair = std::pair<Perm,Perm>;
  std::map<Pair,std::uint64_t> baseline0, baseline1;
  bool first = true;
  for (const auto& pi : permutations) {
    std::map<Pair,std::uint64_t> view0, view1;
    for (const auto& sigma0 : permutations) for (const auto& sigma1 : permutations) {
      const auto tau0 = compose4(pi, inverse4(sigma1));
      const auto tau1 = compose4(pi, inverse4(sigma0));
      ++view0[{sigma0,tau0}];
      ++view1[{sigma1,tau1}];
    }
    if (first) { baseline0 = view0; baseline1 = view1; first = false; }
    require(view0 == baseline0 && view1 == baseline1,
            "single-party permutation factor distribution");
  }
}

using V2 = std::array<unsigned,2>;
V2 add2(const V2& left, const V2& right) {
  return {{(left[0]+right[0])&1U,(left[1]+right[1])&1U}};
}
V2 sub2(const V2& left, const V2& right) { return add2(left,right); }
V2 apply2(unsigned permutation, const V2& value) {
  return permutation == 0 ? value : V2{{value[1],value[0]}};
}
unsigned compose2(unsigned outer, unsigned inner) { return outer ^ inner; }
V2 vec2(unsigned value) { return {{value&1U,(value>>1U)&1U}}; }
std::uint64_t pack_view(unsigned sigma, unsigned tau, const std::array<V2,5>& vectors) {
  std::uint64_t output = sigma | (static_cast<std::uint64_t>(tau) << 1U);
  unsigned shift = 2;
  for (const auto& vector : vectors) {
    output |= static_cast<std::uint64_t>(vector[0]) << shift++;
    output |= static_cast<std::uint64_t>(vector[1]) << shift++;
  }
  return output;
}

void exhaustive_tiny_views() {
  using Counter = std::map<std::uint64_t,std::uint64_t>;
  std::array<std::array<Counter,4>,4> views0, views1;
  std::uint64_t assignments = 0;
  for (unsigned x0v=0;x0v<4;++x0v) for (unsigned x1v=0;x1v<4;++x1v) {
    const auto x0=vec2(x0v),x1=vec2(x1v);
    for(unsigned pi=0;pi<2;++pi)for(unsigned sigma0=0;sigma0<2;++sigma0)
      for(unsigned sigma1=0;sigma1<2;++sigma1) {
        const auto tau0=compose2(pi,sigma1),tau1=compose2(pi,sigma0);
        for(unsigned a0v=0;a0v<4;++a0v)for(unsigned a1v=0;a1v<4;++a1v)
          for(unsigned hv=0;hv<4;++hv)for(unsigned r0v=0;r0v<4;++r0v)
            for(unsigned r1v=0;r1v<4;++r1v) {
              const auto a0=vec2(a0v),a1=vec2(a1v),h=vec2(hv),r0=vec2(r0v),r1=vec2(r1v);
              const auto e0=sub2(sub2(V2{{0,0}},apply2(tau0,a1)),h);
              const auto e1=add2(sub2(V2{{0,0}},apply2(tau1,a0)),h);
              const auto m0=add2(apply2(sigma0,x0),a0),m1=add2(apply2(sigma1,x1),a1);
              const auto s0=add2(apply2(tau0,m1),e0),s1=add2(apply2(tau1,m0),e1);
              const auto d0=add2(s0,r0),d1=add2(s1,r1);
              require(add2(s0,s1)==apply2(pi,add2(x0,x1)),"tiny shuffle correctness");
              ++views0[x0v][x1v][pack_view(sigma0,tau0,{{a0,e0,r0,m1,d1}})];
              ++views1[x0v][x1v][pack_view(sigma1,tau1,{{a1,e1,r1,m0,d0}})];
              ++assignments;
            }
      }
  }
  require(assignments==131072,"tiny view assignment count");
  for(unsigned own=0;own<4;++own)for(unsigned other=1;other<4;++other) {
    require(views0[own][0]==views0[own][other],"P0 tiny view distribution");
    require(views1[0][own]==views1[other][own],"P1 tiny view distribution");
  }
}

template <typename Function> void expect_failure(Function function, const char* message) {
  bool failed = false;
  try { function(); } catch (...) { failed = true; }
  require(failed, message);
}

void misuse_checks() {
  const ProtocolIParallelShuffleDealerConfig dealer_config{
      0x710001,0x720001,0x730001,2,1,34};
  auto generated = protocol_i_parallel_shuffle_dealer_generate(dealer_config);
  const auto encoded = protocol_i_parallel_shuffle_serialize_material(generated.party0);
  auto first = protocol_i_parallel_shuffle_deserialize_material(encoded,0);
  auto replay = protocol_i_parallel_shuffle_deserialize_material(encoded,0);
  ProtocolIParallelShufflePartyConfig config{dealer_config.session,dealer_config.fingerprint,
      dealer_config.material_id,2,1,34,0,1000};
  ProtocolIParallelShuffleParty party(config,std::move(first));
  expect_failure([&]{ProtocolIParallelShuffleParty duplicate(config,std::move(replay));},
                 "serialized replay rejection");
  expect_failure([&]{(void)protocol_i_parallel_shuffle_deserialize_material(encoded,1);},
                 "wrong serialized party rejection");
  expect_failure([&]{party.receive_round2(std::vector<ProtocolIBlock192>(2));},
                 "out-of-order round rejection");
  const std::vector<ProtocolIBlock192> input(2);
  (void)party.prepare_round1(input);
  expect_failure([&]{(void)party.prepare_round1(input);},"round reuse rejection");

  auto wrong_generated = protocol_i_parallel_shuffle_dealer_generate(
      {0x710002,0x720002,0x730002,2,1,34});
  auto wrong_config = config;
  wrong_config.session = 0x710099; wrong_config.fingerprint = 0x720002;
  wrong_config.material_id = 0x730002;
  expect_failure([&]{ProtocolIParallelShuffleParty wrong(wrong_config,
                   std::move(wrong_generated.party0));},"wrong session rejection");
}

}  // namespace

int main() {
  try {
    std::uint64_t serial = 0;
    for (const auto n : {2U,3U,4U,8U,16U}) {
      run_correctness_case(n,++serial,false);
      run_correctness_case(n,++serial,true);
    }
    permutation_independence();
    exhaustive_tiny_views();
    misuse_checks();
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
