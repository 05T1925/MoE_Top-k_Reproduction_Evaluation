#include <moe_topk/protocol_i_cmpagg.h>
#include <moe_topk/protocol_i_priority_key.h>
#include <moe_topk/protocol_i_ucmp.h>
#include <moe_topk/topk_oracle.h>
#include <FSS/prng.h>
#include <stdexcept>
int main(){for(int i=0;i<256;++i)FSSConfig::prngs[i].SetSeed(osuCrypto::toBlock(0x4d32434d5041ULL,i));std::vector<std::uint32_t>s={0,0,UINT32_C(0x80000000),UINT32_C(0x7fffffff),1};const int b=36;const auto rm=(UINT64_C(1)<<b)-1;std::vector<std::uint64_t> z;std::vector<moe_topk::ProtocolIUcmpMaterial> e;for(size_t i=0;i<s.size();++i){auto v=moe_topk::protocol_i_priority_key(s[i],i,s.size()).value;z.push_back((v+i+3)&rm);}for(size_t i=0;i<s.size();++i)for(size_t j=i+1;j<s.size();++j)e.emplace_back(b,i+3,j+3);auto r0=moe_topk::protocol_i_cmpagg_eval_party(0,b,z,e);auto r1=moe_topk::protocol_i_cmpagg_eval_party(1,b,z,e);for(size_t i=0;i<r0.size();++i)r0[i]+=r1[i];if(moe_topk::top_k_mask_from_stable_ranks(r0,3)!=moe_topk::top_k_mask(s,3))throw std::runtime_error("cmpagg differential");return 0;}
