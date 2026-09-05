#include <moe_topk/protocol_i_ucmp.h>
#include <FSS/prng.h>
#include <cstdint>
#include <random>
#include <stdexcept>
namespace { void require(bool x){if(!x)throw std::runtime_error("uCMP conformance");}
void verify(int bits,std::uint64_t left,std::uint64_t right,std::uint64_t ml,std::uint64_t mr){const auto mask=(UINT64_C(1)<<bits)-1;moe_topk::ProtocolIUcmpMaterial m(bits,ml,mr);auto a=m.eval_strict_lt(0,(left+ml)&mask,(right+mr)&mask);auto b=m.eval_strict_lt(1,(left+ml)&mask,(right+mr)&mask);require(a+b==(left<right));}
}
int main(){for(int i=0;i<256;++i)FSSConfig::prngs[i].SetSeed(osuCrypto::toBlock(0x4d3255434d50ULL,i));for(int bits=34;bits<=53;++bits){auto half=UINT64_C(1)<<(bits-1);auto mask=(UINT64_C(1)<<bits)-1;for(auto l:{UINT64_C(0),UINT64_C(1),half-2,half-1})for(auto r:{UINT64_C(0),UINT64_C(1),half-2,half-1})for(auto ml:{UINT64_C(0),UINT64_C(1),half,mask})verify(bits,l,r,ml,(mask-ml)&mask);}return 0;}
