#include <FSS/dcf.h>
#include <FSS/prng.h>
#include <moe_topk/protocol_i_ucmp.h>
#include <moe_topk/protocol_i_aav86_small.h>

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
void require(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}
}

int main() {
  try {
    FSSConfig::prngs[0].SetSeed(osuCrypto::toBlock(UINT64_C(11),UINT64_C(19)));
    auto keys=keyGenDCF(3,64,4,1);
    GroupElement y0=0,y1=0;
    resetDCFOnlinePrgCalls();
    evalDCF(0,&y0,2,keys.first);
    require(readDCFOnlinePrgCalls()==3,"three-level DCF must expand three seeds");
    std::thread isolated([] {
      require(readDCFOnlinePrgCalls()==0,"PRG count leaked between party threads");
    });
    isolated.join();
    require(readDCFOnlinePrgCalls()==3,"other thread changed current count");
    resetDCFOnlinePrgCalls();
    evalDCF(1,&y1,2,keys.second);
    require(readDCFOnlinePrgCalls()==3,"second party three-level count");
    require(y0+y1==1,"DCF functional fixture");

    moe_topk::ProtocolIUcmpMaterial comparison(34,0,0);
    resetDCFOnlinePrgCalls();
    const auto a=comparison.eval_strict_lt(0,1,2);
    require(readDCFOnlinePrgCalls()==68,"uCMP must invoke two 34-level DCF paths");
    resetDCFOnlinePrgCalls();
    const auto b=comparison.eval_strict_lt(1,1,2);
    require(readDCFOnlinePrgCalls()==68,"peer uCMP PRG count");
    require(a+b==1,"uCMP functional fixture");
    auto config=moe_topk::ProtocolIAav86SmallConfig{1,2,3,128,1,5,0,1000,""};
    const auto d128=moe_topk::protocol_i_aav86_small_assess_capacity(config);
    require(!d128.hard_cap&&!d128.budget_limit&&!d128.package_limit&&
            d128.shape.padded_n==128&&d128.shape.total_pair_slots==40640&&
            d128.shape.party_package_bytes==42547506,
            "D128 shape and rejection reasons");
    if (d128.process_limit) {
      bool rejected=false;
      try { (void)moe_topk::protocol_i_aav86_small_preflight(config); }
      catch(const std::invalid_argument& e) {
        rejected=std::string(e.what()).find("RLIMIT_AS")!=std::string::npos;
      }
      require(rejected,"D128 must reject unbounded process");
    } else {
      require(moe_topk::protocol_i_aav86_small_preflight(config).padded_n==128,
              "bounded D128 preflight");
    }
    for(const std::uint32_t n:{128U,256U,1000U,10000U,100000U,1000000U})
      for(std::uint32_t r=2;r<=5;++r) {
        config.logical_n=n; config.iterations=r;
        const auto assessment=moe_topk::protocol_i_aav86_small_assess_capacity(config);
        const auto& shape=assessment.shape;
        std::cout<<"E10_CAPACITY n="<<n<<" r="<<r<<" d="<<shape.padded_n
                 <<" slots_per_party="<<shape.total_pair_slots
                 <<" party_package_bytes="<<shape.party_package_bytes
                 <<" hard_cap="<<assessment.hard_cap
                 <<" package_limit="<<assessment.package_limit
                 <<" budget_limit="<<assessment.budget_limit
                 <<" memory_limit="<<assessment.memory_limit
                 <<" process_limit="<<assessment.process_limit<<"\n";
      }
    std::cout<<"E10_METRICS_CONFORMANCE_PASS dcf3=3 ucmp34=68 per_party\n";
    return 0;
  } catch(const std::exception& e) {
    std::cerr<<"E10_METRICS_CONFORMANCE_FAIL "<<e.what()<<"\n";
    return 1;
  }
}
