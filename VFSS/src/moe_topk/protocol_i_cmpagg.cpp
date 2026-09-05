#include <moe_topk/protocol_i_cmpagg.h>
#include <moe_topk/protocol_i_ucmp.h>
#include <stdexcept>
namespace moe_topk { namespace { std::uint64_t mask(int b){if(b<34||b>53)throw std::invalid_argument("comparison bits");return (UINT64_C(1)<<b)-1;} }
std::uint64_t protocol_i_mask_priority_key_share(int b,std::uint64_t k,std::uint64_t m){return (k+m)&mask(b);}
std::vector<std::uint64_t> protocol_i_cmpagg_eval_party(int party,int b,const std::vector<std::uint64_t>& z,std::vector<ProtocolIUcmpMaterial>& e){if(party<0||party>1||z.empty())throw std::invalid_argument("CmpAgg input");auto rm=mask(b);for(auto x:z)if(x&~rm)throw std::invalid_argument("masked key outside ring");if(e.size()!=z.size()*(z.size()-1)/2)throw std::invalid_argument("edge material count");std::vector<std::uint64_t> r(z.size());size_t q=0;for(size_t i=0;i<z.size();++i)for(size_t j=i+1;j<z.size();++j){if(e[q].comparison_bits()!=b)throw std::invalid_argument("edge width");auto lt=e[q++].eval_strict_lt(party,z[i],z[j]);r[i]+=(party==0?1:0)-lt;r[j]+=lt;}return r;}}
