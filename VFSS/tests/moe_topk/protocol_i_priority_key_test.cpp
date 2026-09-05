#include <moe_topk/protocol_i_priority_key.h>
#include <moe_topk/topk_oracle.h>
#include <algorithm>
#include <stdexcept>
#include <vector>
using namespace moe_topk;
static void req(bool x) { if (!x) throw std::runtime_error("priority key conformance"); }
int main() {
  for (std::size_t n: {std::size_t(1),128,256,1000,10000,100000,1000000}) req(protocol_i_priority_key(0,0,n).key_bits==32+protocol_i_index_bits(n));
  std::vector<std::uint32_t> s={UINT32_C(0x80000000),UINT32_C(0xffffffff),0,UINT32_C(0x7fffffff),0,0};
  std::vector<std::size_t> p(s.size()); for(std::size_t i=0;i<p.size();++i)p[i]=i;
  std::sort(p.begin(),p.end(),[&](auto a,auto b){return protocol_i_priority_key(s[a],a,s.size()).value<protocol_i_priority_key(s[b],b,s.size()).value;});
  auto mask=top_k_mask(s,3); for(std::size_t rank=0;rank<s.size();++rank) req((rank<3)==(mask[p[rank]]!=0));
  req(protocol_i_priority_key(0,0,6).value < protocol_i_priority_key(0,4,6).value);
  bool threw=false; try { protocol_i_priority_key(0,0,0); } catch(...) { threw=true; } req(threw);
  threw=false; try { protocol_i_priority_key(0,1,1); } catch(...) { threw=true; } req(threw);
  return 0;
}
