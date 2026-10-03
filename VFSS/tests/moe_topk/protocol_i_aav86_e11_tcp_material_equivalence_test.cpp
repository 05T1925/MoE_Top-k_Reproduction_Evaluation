#include <moe_topk/protocol_i_aav86_small.h>
#include <moe_topk/topk_oracle.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <random>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {
using namespace moe_topk;
void require(bool ok,const char* message) {
  if(!ok) throw std::runtime_error(message);
}
std::array<int,2> connection(bool tcp) {
  std::array<int,2> result{-1,-1};
  if(!tcp) {
    require(::socketpair(AF_UNIX,SOCK_STREAM,0,result.data())==0,"Unix pair");
    return result;
  }
  const int listener=::socket(AF_INET,SOCK_STREAM,0);
  require(listener>=0,"TCP listener");
  sockaddr_in address{}; address.sin_family=AF_INET;
  address.sin_addr.s_addr=htonl(INADDR_LOOPBACK); address.sin_port=0;
  require(::bind(listener,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0,
          "TCP bind");
  require(::listen(listener,1)==0,"TCP listen");
  socklen_t length=sizeof(address);
  require(::getsockname(listener,reinterpret_cast<sockaddr*>(&address),&length)==0,
          "TCP port");
  result[0]=::socket(AF_INET,SOCK_STREAM,0);
  require(result[0]>=0,"TCP connector");
  require(::connect(result[0],reinterpret_cast<sockaddr*>(&address),sizeof(address))==0,
          "TCP connect");
  result[1]=::accept(listener,nullptr,nullptr);
  ::close(listener);
  require(result[1]>=0,"TCP accept");
  const int enabled=1;
  for(const int fd:result)
    require(::setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,&enabled,sizeof(enabled))==0,
            "TCP_NODELAY");
  return result;
}
std::string fresh_directory() {
  const auto pattern=std::string("/tmp/m6a11-tcp-equivalence-")+std::to_string(::getpid())+
      "-XXXXXX";
  std::vector<char> name(pattern.begin(),pattern.end()); name.push_back(0);
  require(::mkdtemp(name.data())!=nullptr,"claim directory");
  return name.data();
}
struct PairOutput {
  std::array<ProtocolIAav86SmallOutput,2> party;
};
PairOutput run(bool tcp,const ProtocolIAav86SmallConfig& base,
               const std::array<std::vector<std::uint8_t>,2>& packages,
               const std::array<std::vector<std::uint32_t>,2>& shares) {
  std::array<std::array<int,2>,2> score{connection(tcp),connection(tcp)};
  std::vector<std::array<int,2>> core(2U*base.iterations+1U);
  for(auto& pair:core) pair=connection(tcp);
  const auto inverse=connection(tcp);
  std::array<std::string,2> claim{fresh_directory(),fresh_directory()};
  PairOutput result;
  std::array<std::exception_ptr,2> failures{};
  std::array<std::thread,2> workers;
  for(int p=0;p<2;++p) workers[p]=std::thread([&,p] {
    try {
      auto config=base;
      config.party=static_cast<std::uint8_t>(p);
      config.durable_claim_directory=claim[p];
      auto material=protocol_i_aav86_small_deserialize_party_material(packages[p],p,config);
      std::array<int,2> score_fds{score[0][p],score[1][p]};
      std::vector<int> core_fds;
      for(const auto& pair:core) core_fds.push_back(pair[p]);
      result.party[p]=protocol_i_aav86_small_party(config,std::move(material),shares[p],
                                                    score_fds,core_fds,inverse[p]);
    } catch(...) { failures[p]=std::current_exception(); }
  });
  for(auto& worker:workers) worker.join();
  for(const auto& path:claim) std::filesystem::remove_all(path);
  for(const auto& error:failures) if(error) std::rethrow_exception(error);
  return result;
}
void equal_trace(const ProtocolIAav86SmallMetrics& a,const ProtocolIAav86SmallMetrics& b) {
  require(a.active_edges_by_iteration==b.active_edges_by_iteration &&
          a.active_vertices_by_iteration==b.active_vertices_by_iteration &&
          a.ca_prg_calls_by_iteration==b.ca_prg_calls_by_iteration &&
          a.active_edges==b.active_edges && a.active_vertices==b.active_vertices &&
          a.score_dcf_evaluations==b.score_dcf_evaluations &&
          a.ca_dcf_evaluations==b.ca_dcf_evaluations &&
          a.online_prg_calls==b.online_prg_calls &&
          a.causal_rounds==b.causal_rounds &&
          a.online_sent_bytes==b.online_sent_bytes &&
          a.online_received_bytes==b.online_received_bytes &&
          a.edge_trace.size()==b.edge_trace.size() &&
          a.message_trace.size()==b.message_trace.size(),"Unix/TCP metrics");
  for(std::size_t i=0;i<a.edge_trace.size();++i) {
    const auto& x=a.edge_trace[i]; const auto& y=b.edge_trace[i];
    require(x.iteration==y.iteration&&x.a==y.a&&x.c==y.c&&
            x.material_id==y.material_id,"Unix/TCP exact public graph");
  }
  for(std::size_t i=0;i<a.message_trace.size();++i) {
    const auto& x=a.message_trace[i]; const auto& y=b.message_trace[i];
    require(x.phase==y.phase&&x.sent_bytes==y.sent_bytes&&
            x.received_bytes==y.received_bytes,"Unix/TCP exact phase bytes");
  }
}
}

int main() {
  try {
    const ProtocolIAav86SmallConfig config{901,902,903,8,3,3,0,3000,""};
    // TEST_ONLY transport equivalence deliberately replays the SAME package
    // in separate claim directories. Formal benchmark runs never do this.
    auto material=protocol_i_aav86_small_dealer_generate(config);
    const std::array<std::vector<std::uint8_t>,2> packages{
        protocol_i_aav86_small_serialize_party_material(material.party0),
        protocol_i_aav86_small_serialize_party_material(material.party1)};
    const std::vector<std::uint32_t> scores{
        UINT32_C(0x80000000),5,5,0,UINT32_C(0x7fffffff),3,3,1};
    std::array<std::vector<std::uint32_t>,2> shares;
    for(auto& item:shares) item.resize(scores.size());
    std::mt19937_64 prng(9203);
    for(std::size_t i=0;i<scores.size();++i) {
      shares[0][i]=static_cast<std::uint32_t>(prng());
      shares[1][i]=scores[i]-shares[0][i];
    }
    const auto unix_output=run(false,config,packages,shares);
    const auto tcp_output=run(true,config,packages,shares);
    const auto oracle=top_k_mask(scores,config.k);
    std::uint32_t selected=0;
    for(std::size_t i=0;i<scores.size();++i) {
      const auto bit=static_cast<std::uint8_t>(
          unix_output.party[0].xor_mask_share[i]^unix_output.party[1].xor_mask_share[i]);
      require(bit==oracle[i],"transport oracle"); selected+=bit;
    }
    require(selected==config.k,"transport exact K");
    for(int p=0;p<2;++p) {
      require(unix_output.party[p].xor_mask_share==tcp_output.party[p].xor_mask_share,
              "Unix/TCP exact XOR share");
      equal_trace(unix_output.party[p].metrics,tcp_output.party[p].metrics);
    }
    std::cout<<"E11_SAME_MATERIAL_TCP_EQ_PASS n=8 r=3 edges="
             <<unix_output.party[0].metrics.active_edges
             <<" vertices="<<unix_output.party[0].metrics.active_vertices
             <<" rounds="<<unix_output.party[0].metrics.causal_rounds<<"\n";
    return 0;
  } catch(const std::exception& error) {
    std::cerr<<"E11_SAME_MATERIAL_TCP_EQ_FAIL "<<error.what()<<"\n";
    return 1;
  }
}
