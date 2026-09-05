#include <moe_topk/protocol_i_ucmp.h>
#include <FSS/freekey.h>
#include <stdexcept>
namespace moe_topk { namespace { void valid(int b){if(b<34||b>53)throw std::invalid_argument("comparison_bits must be 34..53");} }
ProtocolIUcmpMaterial::ProtocolIUcmpMaterial(int b,std::uint64_t ml,std::uint64_t mr):bits_(b){valid(b);ring_mask_=(UINT64_C(1)<<b)-1;if((ml&~ring_mask_)||(mr&~ring_mask_))throw std::invalid_argument("mask outside ring");alpha_=(ml-mr)&ring_mask_;auto p=keyGenDCF(b,64,alpha_,1);keys_[0]=p.first;keys_[1]=p.second;live_=true;}
ProtocolIUcmpMaterial::~ProtocolIUcmpMaterial(){if(live_){auto pair=std::make_pair(keys_[0],keys_[1]);freeDCFKeyPackPair(pair);}}
ProtocolIUcmpMaterial::ProtocolIUcmpMaterial(ProtocolIUcmpMaterial&& o) noexcept { *this=std::move(o); }
ProtocolIUcmpMaterial& ProtocolIUcmpMaterial::operator=(ProtocolIUcmpMaterial&& o) noexcept {if(this!=&o){if(live_){auto pair=std::make_pair(keys_[0],keys_[1]);freeDCFKeyPackPair(pair);}bits_=o.bits_;ring_mask_=o.ring_mask_;alpha_=o.alpha_;keys_[0]=o.keys_[0];keys_[1]=o.keys_[1];live_=o.live_;used_[0]=o.used_[0];used_[1]=o.used_[1];o.live_=false;}return *this;}
std::uint64_t ProtocolIUcmpMaterial::eval_strict_lt(int party,std::uint64_t zl,std::uint64_t zr){if(!live_||party<0||party>1||used_[party])throw std::invalid_argument("invalid or consumed uCMP material");if((zl&~ring_mask_)||(zr&~ring_mask_))throw std::invalid_argument("masked operand outside ring");used_[party]=true; std::uint64_t x=(zl-zr)&ring_mask_, half=UINT64_C(1)<<(bits_-1), y=(x-half)&ring_mask_;GroupElement t1=0,t2=0;evalDCF(party,&t1,x,keys_[party]);evalDCF(party,&t2,y,keys_[party]);std::uint64_t ge=t2-t1;if(party==1&&y>=half)++ge;return party==0?UINT64_C(1)-ge:UINT64_C(0)-ge;}
}
