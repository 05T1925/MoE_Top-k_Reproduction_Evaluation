#include <FSS/dcf.h>
#include <FSS/freekey.h>
#include <FSS/prng.h>
#include <stdexcept>
#include <cstdint>
static void req(bool x){if(!x)throw std::runtime_error("priority DCF conformance");}
static void check(int b){ const auto max=(UINT64_C(1)<<b)-1; const auto t=max/2; auto k=keyGenDCF(b,1,t,1); for(auto x:{UINT64_C(0),t-1,t,t+1,max}){GroupElement a=0,c=0;evalDCF(0,&a,x,k.first);evalDCF(1,&c,x,k.second);req(((a+c)&1)==(x<t));} freeDCFKeyPackPair(k); }
int main(){for(int i=0;i<256;++i)FSSConfig::prngs[i].SetSeed(osuCrypto::toBlock(0x4d32444346ULL,i)); for(int b:{34,40,41,43,47,50,53})check(b); return 0;}
