#include <moe_topk/protocol_i_priority_key.h>
#include <moe_topk/topk_oracle.h>
#include <algorithm>
#include <random>
#include <stdexcept>
#include <vector>
using namespace moe_topk;
static void req(bool x) { if (!x) throw std::runtime_error("priority key conformance"); }
static std::vector<std::uint8_t> key_mask(const std::vector<std::uint32_t>& s, std::size_t k) {
  std::vector<std::size_t> p(s.size()); for (std::size_t i=0;i<p.size();++i) p[i]=i;
  std::sort(p.begin(),p.end(),[&](auto a,auto b){ return protocol_i_priority_key(s[a],a,s.size()).value < protocol_i_priority_key(s[b],b,s.size()).value; });
  std::vector<std::uint8_t> mask(s.size()); for(std::size_t i=0;i<k;++i) mask[p[i]]=1; return mask;
}
int main() {
  for (const auto [n,bits] : std::vector<std::pair<std::size_t,std::uint32_t>>{{1,1},{128,7},{256,8},{1000,10},{10000,14},{100000,17},{1000000,20}}) { req(protocol_i_index_bits(n)==bits); req(protocol_i_priority_key(0,0,n).key_bits==32+bits); }
  std::vector<std::uint32_t> s={UINT32_C(0x80000000),UINT32_C(0xffffffff),0,UINT32_C(0x7fffffff),0,0};
  std::vector<std::size_t> p(s.size()); for(std::size_t i=0;i<p.size();++i)p[i]=i;
  std::sort(p.begin(),p.end(),[&](auto a,auto b){return protocol_i_priority_key(s[a],a,s.size()).value<protocol_i_priority_key(s[b],b,s.size()).value;});
  auto mask=top_k_mask(s,3); for(std::size_t rank=0;rank<s.size();++rank) req((rank<3)==(mask[p[rank]]!=0));
  req(key_mask(s,1)==top_k_mask(s,1)); req(key_mask(s,s.size())==top_k_mask(s,s.size()));
  req(protocol_i_priority_key(0,0,6).value < protocol_i_priority_key(0,4,6).value);
  std::vector<std::uint32_t> equal(5, 0); auto equal_mask=key_mask(equal,2); req(equal_mask[0] && equal_mask[1] && !equal_mask[4]);
  std::mt19937 rng(0x4d32504b); std::vector<std::uint32_t> random_scores(17); for(auto& x:random_scores) x=static_cast<std::uint32_t>(rng()); random_scores[0]=UINT32_C(0x80000000); random_scores[1]=UINT32_C(0x7fffffff); random_scores[2]=random_scores[3]; for(std::size_t k:{std::size_t(1),8,17}) req(key_mask(random_scores,k)==top_k_mask(random_scores,k));
  bool threw=false; try { protocol_i_priority_key(0,0,0); } catch(...) { threw=true; } req(threw);
  threw=false; try { protocol_i_priority_key(0,1,1); } catch(...) { threw=true; } req(threw);
  threw=false; try { protocol_i_priority_key(0,0,1000001); } catch(...) { threw=true; } req(threw);
  return 0;
}
