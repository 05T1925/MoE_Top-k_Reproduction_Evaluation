#include <moe_topk/protocol_i_aav86_streamed_store.h>
#include <moe_topk/topk_oracle.h>
#include "protocol_i_aav86_e9_fixtures.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {
using namespace moe_topk;
void require(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
std::array<int,2> pair() {
  std::array<int,2> result{};
  require(::socketpair(AF_UNIX,SOCK_STREAM,0,result.data())==0,"E20 socketpair");
  return result;
}
std::string directory(std::uint64_t serial,int party) {
  std::string pattern="/tmp/m6a20-diff-"+std::to_string(::getpid())+"-"+
                      std::to_string(serial)+"-p"+std::to_string(party)+"-XXXXXX";
  std::vector<char> bytes(pattern.begin(),pattern.end());bytes.push_back(0);
  require(::mkdtemp(bytes.data())!=nullptr,"E20 mkdtemp");
  return bytes.data();
}
void run_case(const std::vector<std::uint32_t>& scores,std::uint32_t k,
              std::uint32_t r,std::uint64_t serial) {
  const auto n=static_cast<std::uint32_t>(scores.size());
  auto c0=ProtocolIAav86SmallConfig{0x210000+serial,0x220000+serial,
      0x230000+serial,n,k,r,0,30000,directory(serial,0)};
  auto c1=c0;c1.party=1;c1.durable_claim_directory=directory(serial,1);
  auto t0=pair(),t1=pair();
  ProtocolIAav86StreamedPartyMaterial m0,m1;
  std::exception_ptr dealer_error,e0,e1;
  std::thread p0([&] {
    try {m0=protocol_i_aav86_stream_receive_party(c0,t0[1],c0.durable_claim_directory);}
    catch(...) {e0=std::current_exception();}
    ::close(t0[1]);
  });
  std::thread p1([&] {
    try {m1=protocol_i_aav86_stream_receive_party(c1,t1[1],c1.durable_claim_directory);}
    catch(...) {e1=std::current_exception();}
    ::close(t1[1]);
  });
  std::thread dealer([&] {
    try {protocol_i_aav86_stream_dealer_send(c0,t0[0],t1[0]);}
    catch(...) {dealer_error=std::current_exception();}
    ::close(t0[0]);::close(t1[0]);
  });
  dealer.join();p0.join();p1.join();
  if(dealer_error) std::rethrow_exception(dealer_error);
  if(e0) std::rethrow_exception(e0);
  if(e1) std::rethrow_exception(e1);
  std::mt19937_64 rng(0x200000+serial);
  std::vector<std::uint32_t> x0(n),x1(n);
  for(std::size_t i=0;i<n;++i) {x0[i]=rng();x1[i]=scores[i]-x0[i];}
  std::array<std::array<int,2>,2> score{pair(),pair()};
  std::vector<std::array<int,2>> core(2U*r+1U);
  for(auto& channel:core) channel=pair();
  const auto inverse=pair();
  std::vector<int> core0,core1;
  for(const auto& channel:core) {
    core0.push_back(channel[0]);core1.push_back(channel[1]);
  }
  ProtocolIAav86SmallOutput out0,out1;
  e0=nullptr;e1=nullptr;
  std::thread online0([&] {
    try {out0=protocol_i_aav86_stream_party_from_store(
        c0,std::move(m0),x0,{score[0][0],score[1][0]},core0,inverse[0]);}
    catch(...) {e0=std::current_exception();}
  });
  std::thread online1([&] {
    try {out1=protocol_i_aav86_stream_party_from_store(
        c1,std::move(m1),x1,{score[0][1],score[1][1]},core1,inverse[1]);}
    catch(...) {e1=std::current_exception();}
  });
  online0.join();online1.join();
  for(const auto& channel:score){::close(channel[0]);::close(channel[1]);}
  for(const auto& channel:core){::close(channel[0]);::close(channel[1]);}
  ::close(inverse[0]);::close(inverse[1]);
  if(e0) std::rethrow_exception(e0);
  if(e1) std::rethrow_exception(e1);
  const auto oracle=top_k_mask(scores,k);
  require(out0.xor_mask_share.size()==n&&out1.xor_mask_share.size()==n,
          "E20 output shape");
  std::uint32_t selected=0;
  for(std::size_t i=0;i<n;++i) {
    const auto bit=out0.xor_mask_share[i]^out1.xor_mask_share[i];
    require(bit==oracle[i],"E20 frozen oracle differential");
    selected+=bit;
  }
  require(selected==k,"E20 exactly K");
  const auto d=m0.base.padded_n;
  const auto slots=static_cast<std::uint64_t>(r)*d*(d-1U)/2U;
  require(out0.metrics.pool_slots_per_party==slots&&
          out1.metrics.pool_slots_per_party==slots&&
          out0.metrics.active_edges==out1.metrics.active_edges&&
          out0.metrics.edge_trace.size()==out0.metrics.active_edges&&
          out0.metrics.online_sent_bytes==out1.metrics.online_received_bytes&&
          out1.metrics.online_sent_bytes==out0.metrics.online_received_bytes&&
          out0.metrics.causal_rounds==2U*r+4U,
          "E20 metric/trace invariant");
  std::filesystem::remove_all(c0.durable_claim_directory);
  std::filesystem::remove_all(c1.durable_claim_directory);
  std::cout<<"E20_STREAM_DIFFERENTIAL_CASE n="<<n<<" k="<<k<<" r="<<r
           <<" active="<<out0.metrics.active_edges<<" PASS\n";
}
} // namespace
int main() {
  try {
    std::uint64_t cases=0;
    for(const auto& fixture:moe_topk_e9_test::fixtures(16))
      run_case(fixture.scores,fixture.k,fixture.r,++cases);
    std::cout<<"E20_STREAM_DIFFERENTIAL_PASS cases="<<cases<<"\n";
    return 0;
  } catch(const std::exception& e) {
    std::cerr<<"E20_STREAM_DIFFERENTIAL_FAIL "<<e.what()<<"\n";
    return 1;
  }
}
