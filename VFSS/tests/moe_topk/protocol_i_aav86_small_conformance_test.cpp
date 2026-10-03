#include <moe_topk/protocol_i_aav86_small.h>
#include <moe_topk/protocol_i_priority_key.h>
#include <moe_topk/protocol_i_permutation.h>
#include "protocol_i_aav86_e9_fixtures.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace moe_topk;
void require(bool ok, const char* what) {
  if (!ok) throw std::runtime_error(what);
}
std::pair<std::size_t,std::size_t> edge_blob(
    const std::vector<std::uint8_t>& bytes,std::uint32_t t,std::uint32_t a,
    std::uint32_t c,std::uint64_t material_id) {
  std::array<std::uint8_t,20> tag{};
  const std::array<std::uint32_t,3> words{t,a,c};
  for(std::size_t i=0;i<words.size();++i)
    for(int j=0;j<4;++j) tag[4U*i+j]=static_cast<std::uint8_t>(words[i]>>(24-8*j));
  for(int j=0;j<8;++j) tag[12U+j]=static_cast<std::uint8_t>(material_id>>(56-8*j));
  const auto it=std::search(bytes.begin(),bytes.end(),tag.begin(),tag.end());
  require(it!=bytes.end(),"edge tag location");
  const auto at=static_cast<std::size_t>(it-bytes.begin())+tag.size();
  require(at+4<=bytes.size(),"edge blob length field");
  std::uint32_t length=0;
  for(std::size_t j=0;j<4;++j) length=(length<<8U)|bytes[at+j];
  require(length>0&&at+4U+length<=bytes.size(),"edge blob extent");
  return {at+4U,length};
}
template <typename F> void rejects(F&& f, const char* what) {
  bool rejected = false;
  try { f(); } catch (const std::exception&) { rejected = true; }
  require(rejected, what);
}
std::vector<std::uint64_t> add(const std::vector<std::uint64_t>& a,
                               const std::vector<std::uint64_t>& b,
                               std::uint64_t mask) {
  require(a.size() == b.size(), "vector shape");
  auto out = a;
  for (std::size_t i = 0; i < out.size(); ++i) out[i] = (a[i] + b[i]) & mask;
  return out;
}
void case_one(const std::vector<std::uint32_t>& scores, std::uint32_t k,
              std::uint32_t iterations, std::uint64_t serial) {
  const auto n = static_cast<std::uint32_t>(scores.size());
  ProtocolIAav86SmallConfig c{0x7000 + serial, 0x8000 + serial, 0x9000 + serial,
                               n, k, iterations, 0, 5000, ""};
  const auto capacity=protocol_i_aav86_small_preflight(c);
  auto bundle = protocol_i_aav86_small_dealer_generate(c);
  auto& p0 = bundle.party0; auto& p1 = bundle.party1;
  const auto bytes0 = protocol_i_aav86_small_serialize_party_material(p0);
  const auto bytes1 = protocol_i_aav86_small_serialize_party_material(p1);
  require(bytes0.size()==capacity.party_package_bytes&&
          bytes1.size()==capacity.party_package_bytes&&
          capacity.total_pair_slots==static_cast<std::uint64_t>(iterations)*
              capacity.pairs_per_iteration,"preflight matches exact encoding");
  auto c1 = c; c1.party = 1;
  auto decoded0 = protocol_i_aav86_small_deserialize_party_material(bytes0, 0, c);
  auto decoded1 = protocol_i_aav86_small_deserialize_party_material(bytes1, 1, c1);
  rejects([&] { (void)protocol_i_aav86_small_deserialize_party_material(bytes0, 1, c1); },
          "wrong party accepted");
  auto truncated = bytes0; truncated.pop_back();
  rejects([&] { (void)protocol_i_aav86_small_deserialize_party_material(truncated, 0, c); },
          "truncated package accepted");
  auto extended = bytes0; extended.push_back(0);
  rejects([&] { (void)protocol_i_aav86_small_deserialize_party_material(extended, 0, c); },
          "trailing package accepted");
  auto bad = c; ++bad.material_id;
  rejects([&] { (void)protocol_i_aav86_small_deserialize_party_material(bytes0, 0, bad); },
          "wrong material ID accepted");
  bad=c; ++bad.session;
  rejects([&] { (void)protocol_i_aav86_small_deserialize_party_material(bytes0, 0, bad); },
          "wrong session accepted");
  bad=c; bad.k=bad.k==n?1U:n;
  if(bad.k!=c.k)
    rejects([&] { (void)protocol_i_aav86_small_deserialize_party_material(bytes0, 0, bad); },
            "wrong K accepted");
  bad=c; bad.iterations=bad.iterations==5?4U:bad.iterations+1U;
  rejects([&] { (void)protocol_i_aav86_small_deserialize_party_material(bytes0, 0, bad); },
          "wrong round count accepted");
  if(serial==1) {
    // Locate the canonical first edge tuple (t=0,a=0,c=1, material ID)
    // in the serialized package and corrupt its round label/count.
    std::vector<std::uint8_t> marker(20);
    marker[11]=1;
    for(int j=0;j<8;++j) marker[12+j]=static_cast<std::uint8_t>(c.material_id>>(56-8*j));
    const auto found=std::search(bytes0.begin(),bytes0.end(),marker.begin(),marker.end());
    require(found!=bytes0.end()&&found-bytes0.begin()>=4,"edge marker location");
    auto wrong_round=bytes0; wrong_round[found-bytes0.begin()+3]=1;
    rejects([&] { (void)protocol_i_aav86_small_deserialize_party_material(wrong_round,0,c); },
            "wrong edge round accepted");
    auto missing_slot=bytes0; missing_slot[found-bytes0.begin()-1]=0;
    rejects([&] { (void)protocol_i_aav86_small_deserialize_party_material(missing_slot,0,c); },
            "missing edge slot accepted");
  }
  if(serial==7) {
    const auto first=edge_blob(bytes0,0,0,1,c.material_id);
    const auto second=edge_blob(bytes0,0,0,2,c.material_id+1U);
    require(first.second==second.second,"same-spec key lengths");
    auto swapped=bytes0;
    std::swap_ranges(swapped.begin()+first.first,swapped.begin()+first.first+first.second,
                     swapped.begin()+second.first);
    auto accepted=protocol_i_aav86_small_deserialize_party_material(swapped,0,c);
    require(accepted.edge_keys.size()==decoded0.edge_keys.size(),
            "blob-swap parser behavior");
    // The structural labels still pass. Independent TEST_ONLY consistency
    // probes of the two dealer outputs must detect the mismatched key/mask.
    const auto full_r0=(decoded0.node_mask_shares[0]+decoded1.node_mask_shares[0])&
        ((UINT64_C(1)<<decoded0.comparison_bits)-1U);
    const auto full_r1=(decoded0.node_mask_shares[1]+decoded1.node_mask_shares[1])&
        ((UINT64_C(1)<<decoded0.comparison_bits)-1U);
    const auto ring_mask=(UINT64_C(1)<<decoded0.comparison_bits)-1U;
    const auto wrong=accepted.edge_keys[0].serialize();
    const auto right=decoded1.edge_keys[0].serialize();
    bool mismatch=false;
    for(std::uint64_t x=0;x<16;++x) {
      auto a=ProtocolIUcmpPartyMaterial::deserialize(wrong);
      auto b=ProtocolIUcmpPartyMaterial::deserialize(right);
      const auto value0=x,value1=15U-x;
      const auto y0=(value0+full_r0)&ring_mask,y1=(value1+full_r1)&ring_mask;
      if(a.eval_strict_lt(y0,y1)+b.eval_strict_lt(y0,y1)!=(value0<value1))
        mismatch=true;
    }
    require(mismatch,"dealer consistency failed to detect swapped blob");
  }

  const auto d = decoded0.padded_n;
  const auto bits = decoded0.comparison_bits;
  const auto mask = (UINT64_C(1) << bits) - 1U;
  auto pi = protocol_i_compose_permutation(decoded0.forward_tau, decoded1.forward_sigma);
  require(pi == protocol_i_compose_permutation(decoded1.forward_tau, decoded0.forward_sigma),
          "forward pi disagreement");
  auto inverse_pi = protocol_i_compose_permutation(decoded0.inverse_tau, decoded1.inverse_sigma);
  require(inverse_pi == protocol_i_compose_permutation(decoded1.inverse_tau, decoded0.inverse_sigma),
          "inverse pi disagreement");
  require(inverse_pi == protocol_i_inverse_permutation(pi), "inverse wrong pi");
  std::vector<std::uint64_t> full_key(d), share0(d), share1(d);
  for (std::uint32_t j = 0; j < d; ++j) {
    const auto raw = j < n ? scores[j] : UINT32_C(0x80000000);
    full_key[j] = protocol_i_priority_key(raw, j, d).value;
    share0[j] = (0x12345U * (j + 1U)) & mask;
    share1[j] = (full_key[j] - share0[j]) & mask;
  }
  const auto u0 = add(protocol_i_apply_permutation(decoded0.forward_sigma, share0),
                      decoded0.forward_a, mask);
  const auto u1 = add(protocol_i_apply_permutation(decoded1.forward_sigma, share1),
                      decoded1.forward_a, mask);
  const auto s0 = add(protocol_i_apply_permutation(decoded0.forward_tau, u1),
                      decoded0.forward_e, mask);
  const auto s1 = add(protocol_i_apply_permutation(decoded1.forward_tau, u0),
                      decoded1.forward_e, mask);
  const auto shuffled_key = add(s0, s1, mask);
  require(shuffled_key == protocol_i_apply_permutation(pi, full_key),
          "forward shuffled shares");
  for (std::uint32_t t = 0; t < iterations; ++t) {
    std::vector<std::uint64_t> y(d);
    for (std::uint32_t a = 0; a < d; ++a)
      y[a] = (shuffled_key[a] + decoded0.node_mask_shares[t*d+a] +
              decoded1.node_mask_shares[t*d+a]) & mask;
    std::size_t slot = t * (d*(d-1U)/2U);
    for (std::uint32_t a = 0; a < d; ++a)
      for (std::uint32_t b = a + 1U; b < d; ++b) {
        const auto l0 = decoded0.edge_keys[slot].eval_strict_lt(y[a],y[b]);
        const auto l1 = decoded1.edge_keys[slot].eval_strict_lt(y[a],y[b]);
        require(l0 + l1 == static_cast<std::uint64_t>(shuffled_key[a] < shuffled_key[b]),
                "first/subsequent round R-key binding");
        rejects([&] { (void)decoded0.edge_keys[slot].eval_strict_lt(y[a],y[b]); },
                "edge key reuse accepted");
        ++slot;
      }
  }
  std::vector<std::uint64_t> carrier(d);
  std::vector<std::uint32_t> handles(d);
  for (std::uint32_t a = 0; a < d; ++a) handles[a] = a;
  std::sort(handles.begin(), handles.end(),
            [&](auto a, auto b) { return shuffled_key[a] < shuffled_key[b]; });
  for (std::size_t i = 0; i < k; ++i) carrier[handles[i]] = 1;
  const std::vector<std::uint64_t> zero(d);
  const auto v0 = add(protocol_i_apply_permutation(decoded0.inverse_sigma, carrier),
                      decoded0.inverse_a, mask);
  const auto v1 = add(protocol_i_apply_permutation(decoded1.inverse_sigma, zero),
                      decoded1.inverse_a, mask);
  const auto o0 = add(protocol_i_apply_permutation(decoded0.inverse_tau, v1),
                      decoded0.inverse_e, mask);
  const auto o1 = add(protocol_i_apply_permutation(decoded1.inverse_tau, v0),
                      decoded1.inverse_e, mask);
  const auto original = add(o0,o1,mask);
  require(original == protocol_i_apply_permutation(inverse_pi, carrier),
          "inverse share route");
  std::uint32_t selected = 0;
  for (std::uint32_t j = 0; j < n; ++j) selected += original[j] & 1U;
  require(selected == k, "exact K real membership");
  for (std::uint32_t j = n; j < d; ++j) require(original[j] == 0, "dummy selected");
}
}
int main() {
  try {
    ProtocolIAav86SmallConfig over_cap{1,2,3,65,1,2,0,5000,""};
    rejects([&] { (void)protocol_i_aav86_small_dealer_generate(over_cap); },
            "D>64 capacity accepted");
    over_cap.logical_n=64; over_cap.iterations=6;
    rejects([&] { (void)protocol_i_aav86_small_dealer_generate(over_cap); },
            "r>5 capacity accepted");
    over_cap.iterations=5; over_cap.material_id=std::numeric_limits<std::uint64_t>::max();
    rejects([&] { (void)protocol_i_aav86_small_preflight(over_cap); },
            "material ID overflow accepted");
    if (const auto* tier = std::getenv("MOE_TOPK_M6A_E9_D")) {
      const auto d = static_cast<std::uint32_t>(std::stoul(tier));
      std::uint64_t serial=0;
      for (const auto& fixture:moe_topk_e9_test::fixtures(d))
        case_one(fixture.scores,fixture.k,fixture.r,++serial);
      std::cout<<"E9_CONFORMANCE_PASS d="<<d<<" cases="<<serial<<"\n";
      return 0;
    }
    std::uint64_t serial = 0;
    for (const auto& scores : {std::vector<std::uint32_t>{UINT32_C(0x80000000)},
                               std::vector<std::uint32_t>{5,5,5},
                               std::vector<std::uint32_t>{UINT32_C(0x80000000),UINT32_C(0x7fffffff),7,7,0},
                               std::vector<std::uint32_t>{8,7,6,5,4,3,2,1}})
      for (const auto k : {1U,static_cast<unsigned>(scores.size())})
        for (const auto r : {1U,2U,5U}) case_one(scores,k,r,++serial);
    std::cout << "E7_CONFORMANCE_PASS cases=" << serial << "\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "E7_CONFORMANCE_FAIL " << e.what() << "\n";
    return 1;
  }
}
