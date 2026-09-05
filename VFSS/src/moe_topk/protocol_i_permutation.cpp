#include <moe_topk/protocol_i_permutation.h>
#include <algorithm>
#include <random>
#include <stdexcept>
namespace moe_topk {
void protocol_i_validate_permutation(const ProtocolIPermutation& p){if(p.empty())throw std::invalid_argument("empty permutation");std::vector<bool>s(p.size());for(auto v:p){if(v>=p.size()||s[v])throw std::invalid_argument("invalid permutation");s[v]=true;}}
ProtocolIPermutation protocol_i_identity_permutation(std::uint32_t n){if(!n)throw std::invalid_argument("empty permutation");ProtocolIPermutation p(n);for(std::uint32_t i=0;i<n;++i)p[i]=i;return p;}
ProtocolIPermutation protocol_i_inverse_permutation(const ProtocolIPermutation&p){protocol_i_validate_permutation(p);ProtocolIPermutation r(p.size());for(std::uint32_t i=0;i<p.size();++i)r[p[i]]=i;return r;}
ProtocolIPermutation protocol_i_compose_permutation(const ProtocolIPermutation&o,const ProtocolIPermutation&i){protocol_i_validate_permutation(o);protocol_i_validate_permutation(i);if(o.size()!=i.size())throw std::invalid_argument("permutation compose shape");ProtocolIPermutation r(o.size());for(std::size_t x=0;x<r.size();++x)r[x]=i[o[x]];return r;}
ProtocolIPermutation protocol_i_test_permutation(std::uint32_t n,std::uint64_t seed){auto p=protocol_i_identity_permutation(n);std::mt19937_64 rng(seed);std::shuffle(p.begin(),p.end(),rng);return p;}
}  // namespace moe_topk
