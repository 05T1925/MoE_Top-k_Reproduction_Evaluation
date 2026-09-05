#include <moe_topk/protocol_i_ucmp.h>
#include <FSS/prng.h>
#include <cstdint>
#include <random>
#include <iostream>
#include <stdexcept>
namespace { void require(bool x){if(!x)throw std::runtime_error("uCMP conformance");}
void verify(int bits,std::uint64_t left,std::uint64_t right,std::uint64_t ml,std::uint64_t mr){const auto mask=(UINT64_C(1)<<bits)-1;const auto zl=(left+ml)&mask,zr=(right+mr)&mask;moe_topk::ProtocolIUcmpMaterial m(bits,ml,mr);auto a=m.eval_strict_lt(0,zl,zr);auto b=m.eval_strict_lt(1,zl,zr);if(a+b>1){std::cerr<<bits<<","<<left<<","<<right<<","<<ml<<","<<mr<<","<<zl<<","<<zr<<","<<(a+b)<<","<<(left<right)<<"\n";throw std::runtime_error("uCMP tuple");}require(a+b==(left<right));}
}
int main(){for(int i=0;i<256;++i)FSSConfig::prngs[i].SetSeed(osuCrypto::toBlock(0x4d3255434d50ULL,i));std::mt19937_64 rng(0x4d3255434d50ULL);for(int bits=34;bits<=53;++bits){auto half=UINT64_C(1)<<(bits-1);auto mask=(UINT64_C(1)<<bits)-1;for(auto l:{UINT64_C(0),UINT64_C(1),UINT64_C(2),half-2,half-1})for(auto r:{UINT64_C(0),UINT64_C(1),UINT64_C(2),half-2,half-1})for(auto ml:{UINT64_C(0),UINT64_C(1),half-1,half,mask-1,mask})for(auto mr:{UINT64_C(0),UINT64_C(1),half-1,half,mask-1,mask})verify(bits,l,r,ml,mr);for(int i=0;i<64;++i)verify(bits,rng()&(half-1),rng()&(half-1),rng()&mask,rng()&mask);moe_topk::ProtocolIUcmpMaterial pair(bits,0,0);auto p0=moe_topk::ProtocolIUcmpPartyMaterial::deserialize(pair.export_party_material(0).serialize());auto p1=pair.export_party_material(1);require(p0.eval_strict_lt(0,1)+p1.eval_strict_lt(0,1)==1);}return 0;}
