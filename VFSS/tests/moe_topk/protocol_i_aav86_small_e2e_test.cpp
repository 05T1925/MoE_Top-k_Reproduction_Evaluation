#include <moe_topk/protocol_i_aav86_small.h>
#include <moe_topk/protocol_i_transport.h>
#include <moe_topk/topk_oracle.h>
#include "protocol_i_aav86_e9_fixtures.h"
#include "protocol_i_e14_material_metrics.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <sys/resource.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {
using namespace moe_topk;
void require(bool value, const char* why) { if (!value) throw std::runtime_error(why); }
std::uint32_t fresh_share() {
  std::uint32_t value=0;
  auto* p=reinterpret_cast<std::uint8_t*>(&value);
  std::size_t left=sizeof(value);
  while(left) {
    const auto count=::getrandom(p,left,0);
    if(count<0&&errno==EINTR) continue;
    require(count>0,"E14 input share entropy");
    p+=count;left-=static_cast<std::size_t>(count);
  }
  return value;
}
void send_all(int fd, const void* data, std::size_t length) {
  const auto* p=static_cast<const char*>(data);
  while(length) { const auto n=::write(fd,p,length); require(n>0,"E2E write"); p+=n; length-=n; }
}
void receive_all(int fd, void* data, std::size_t length) {
  auto* p=static_cast<char*>(data);
  while(length) { const auto n=::read(fd,p,length); require(n>0,"E2E read"); p+=n; length-=n; }
}
void send_bytes(int fd,const std::vector<std::uint8_t>& bytes) {
  const auto n=static_cast<std::uint64_t>(bytes.size()); send_all(fd,&n,sizeof(n));
  if(n) send_all(fd,bytes.data(),bytes.size());
}
std::vector<std::uint8_t> receive_bytes(int fd,std::size_t cap) {
  std::uint64_t n=0; receive_all(fd,&n,sizeof(n)); require(n<=cap,"E2E length cap");
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(n));
  if(n) receive_all(fd,bytes.data(),bytes.size()); return bytes;
}
std::array<int,2> pair() {
  std::array<int,2> p{-1,-1};
  require(::socketpair(AF_UNIX,SOCK_STREAM,0,p.data())==0,"E2E socketpair"); return p;
}
std::array<int,2> tcp_pair() {
  const int listener=::socket(AF_INET,SOCK_STREAM,0);
  require(listener>=0,"E11 TCP listener");
  sockaddr_in address{}; address.sin_family=AF_INET;
  address.sin_addr.s_addr=htonl(INADDR_LOOPBACK); address.sin_port=0;
  require(::bind(listener,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0,
          "E11 TCP bind");
  require(::listen(listener,1)==0,"E11 TCP listen");
  socklen_t length=sizeof(address);
  require(::getsockname(listener,reinterpret_cast<sockaddr*>(&address),&length)==0,
          "E11 TCP getsockname");
  const int connector=::socket(AF_INET,SOCK_STREAM,0);
  require(connector>=0,"E11 TCP connector");
  require(::connect(connector,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0,
          "E11 TCP connect");
  const int accepted=::accept(listener,nullptr,nullptr);
  ::close(listener);
  require(accepted>=0,"E11 TCP accept");
  const int enabled=1;
  require(::setsockopt(connector,IPPROTO_TCP,TCP_NODELAY,&enabled,sizeof(enabled))==0 &&
          ::setsockopt(accepted,IPPROTO_TCP,TCP_NODELAY,&enabled,sizeof(enabled))==0,
          "E11 TCP_NODELAY");
  return {connector,accepted};
}
bool tcp_online() {
  const char* setting=std::getenv("MOE_TOPK_M6A_E11_TRANSPORT");
  if(!setting || std::string(setting)=="unix") return false;
  require(std::string(setting)=="tcp","E11 transport name");
  return true;
}
std::array<int,2> online_pair() { return tcp_online()?tcp_pair():pair(); }
std::string directory(std::uint64_t serial,int party) {
  auto pattern="/tmp/m6a7-e2e-"+std::to_string(::getpid())+"-"+
      std::to_string(serial)+"-"+std::to_string(party)+"-XXXXXX";
  std::vector<char> buffer(pattern.begin(),pattern.end()); buffer.push_back(0);
  require(::mkdtemp(buffer.data())!=nullptr,"E2E mkdtemp"); return buffer.data();
}
void close_except(const std::vector<int>& all,const std::vector<int>& keep) {
  for(const auto fd:all) if(std::find(keep.begin(),keep.end(),fd)==keep.end()) ::close(fd);
}
void wait_ok(pid_t child,const char* role) {
  int status=0; require(::waitpid(child,&status,0)==child,"E2E waitpid");
  if(!(WIFEXITED(status)&&WEXITSTATUS(status)==0))
    throw std::runtime_error(std::string("E2E child failed: ")+role);
}
struct ChildResult {
  std::vector<std::uint8_t> mask;
  std::vector<std::uint64_t> active_by_round;
  std::vector<std::uint64_t> vertices_by_round,ca_prg_by_round;
  std::uint64_t pool=0,active=0,score_evals=0,ca_evals=0,evals=0;
  std::uint64_t sent=0,received=0,rounds=0;
  std::uint64_t online_ns=0,peak_kib=0,edge_digest=0;
  std::uint64_t package_bytes=0,pivot_seed_lo=0,pivot_seed_hi=0;
  std::uint64_t t_payload_bytes=0;
  std::uint64_t active_vertices=0,score_prg=0,ca_prg=0,inverse_prg=0,total_prg=0;
  std::uint64_t score_bytes=0,core_bytes=0,inverse_bytes=0;
  std::uint64_t score_ns=0,core_ns=0,inverse_ns=0;
  std::uint64_t score_received_bytes=0,core_received_bytes=0,inverse_received_bytes=0;
  std::uint64_t ca_ns=0,carrier_ns=0;
};
std::vector<std::uint8_t> encode_result(const ProtocolIAav86SmallOutput& o,
                                         std::uint64_t online_ns,
                                         std::uint64_t package_bytes,
                                         std::uint64_t t_payload_bytes,
                                         std::uint64_t pivot_seed_lo,
                                         std::uint64_t pivot_seed_hi) {
  std::vector<std::uint8_t> bytes=o.xor_mask_share;
  const auto rss=[] { struct rusage u{}; require(::getrusage(RUSAGE_SELF,&u)==0,"getrusage");
                      return static_cast<std::uint64_t>(u.ru_maxrss); }();
  require(o.metrics.message_trace.size()==o.metrics.causal_rounds&&
          o.metrics.edge_trace.size()==o.metrics.active_edges,"E2E trace shape");
  std::vector<std::set<std::uint32_t>> incident(o.metrics.active_edges_by_iteration.size());
  std::vector<std::uint64_t> edges_by_round(incident.size());
  for(const auto& edge:o.metrics.edge_trace) {
    require(edge.iteration<incident.size(),"E11 public edge round");
    incident[edge.iteration].insert(edge.a);
    incident[edge.iteration].insert(edge.c);
    ++edges_by_round[edge.iteration];
  }
  require(edges_by_round==o.metrics.active_edges_by_iteration,
          "E11 public edge counts");
  for(std::size_t t=0;t<incident.size();++t)
    require(incident[t].size()==o.metrics.active_vertices_by_iteration[t],
            "E11 public vertex count");
  std::uint64_t sent=0,received=0,digest=UINT64_C(1469598103934665603);
  for(std::size_t i=0;i<o.metrics.message_trace.size();++i) {
    const auto phase=i==0?4U:i==1?5U:i==2?9U:
        i==o.metrics.message_trace.size()-1?40U:static_cast<unsigned>(7U+i);
    require(o.metrics.message_trace[i].phase==phase,"E2E message DAG");
    sent+=o.metrics.message_trace[i].sent_bytes;
    received+=o.metrics.message_trace[i].received_bytes;
  }
  require(sent==o.metrics.online_sent_bytes&&received==o.metrics.online_received_bytes,
          "E2E trace byte accounting");
  std::uint64_t previous=0;
  for(const auto& edge:o.metrics.edge_trace) {
    require(edge.a<edge.c&&edge.c<128&&edge.material_id>previous,"E2E edge reuse/order");
    previous=edge.material_id;
    digest=(digest^edge.material_id)*UINT64_C(1099511628211);
  }
  const std::array<std::uint64_t,31> values{o.metrics.pool_slots_per_party,o.metrics.active_edges,
      o.metrics.score_dcf_evaluations,o.metrics.ca_dcf_evaluations,
      o.metrics.dcf_evaluations,o.metrics.online_sent_bytes,o.metrics.online_received_bytes,
      o.metrics.causal_rounds,online_ns,rss,digest,package_bytes,pivot_seed_lo,pivot_seed_hi,
      o.metrics.active_vertices,o.metrics.score_prg_calls,o.metrics.ca_prg_calls,
      o.metrics.inverse_prg_calls,o.metrics.online_prg_calls,
      o.metrics.score_sent_bytes,o.metrics.core_sent_bytes,o.metrics.inverse_sent_bytes,
      o.metrics.score_time_ns,o.metrics.core_time_ns,o.metrics.inverse_time_ns,
      o.metrics.score_received_bytes,o.metrics.core_received_bytes,
      o.metrics.inverse_received_bytes,o.metrics.ca_time_ns,o.metrics.carrier_time_ns,
      t_payload_bytes};
  const auto* p=reinterpret_cast<const std::uint8_t*>(values.data());
  bytes.insert(bytes.end(),p,p+sizeof(values));
  const auto* by_round=reinterpret_cast<const std::uint8_t*>(
      o.metrics.active_edges_by_iteration.data());
  bytes.insert(bytes.end(),by_round,by_round+
      o.metrics.active_edges_by_iteration.size()*sizeof(std::uint64_t));
  const auto* vertices=reinterpret_cast<const std::uint8_t*>(
      o.metrics.active_vertices_by_iteration.data());
  bytes.insert(bytes.end(),vertices,vertices+
      o.metrics.active_vertices_by_iteration.size()*sizeof(std::uint64_t));
  const auto* prg=reinterpret_cast<const std::uint8_t*>(
      o.metrics.ca_prg_calls_by_iteration.data());
  bytes.insert(bytes.end(),prg,prg+
      o.metrics.ca_prg_calls_by_iteration.size()*sizeof(std::uint64_t));
  return bytes;
}
ChildResult decode_result(const std::vector<std::uint8_t>& bytes,std::size_t n,
                          std::size_t rounds) {
  require(bytes.size()==n+(31U+3U*rounds)*sizeof(std::uint64_t),"E2E result length");
  ChildResult result; result.mask.assign(bytes.begin(),bytes.begin()+n);
  std::array<std::uint64_t,31> values{};
  std::copy_n(bytes.begin()+n,sizeof(values),reinterpret_cast<std::uint8_t*>(values.data()));
  result.pool=values[0]; result.active=values[1]; result.score_evals=values[2];
  result.ca_evals=values[3]; result.evals=values[4];
  result.sent=values[5]; result.received=values[6]; result.rounds=values[7];
  result.online_ns=values[8]; result.peak_kib=values[9]; result.edge_digest=values[10];
  result.package_bytes=values[11]; result.pivot_seed_lo=values[12];
  result.pivot_seed_hi=values[13];
  result.active_vertices=values[14]; result.score_prg=values[15];
  result.ca_prg=values[16]; result.inverse_prg=values[17]; result.total_prg=values[18];
  result.score_bytes=values[19]; result.core_bytes=values[20];
  result.inverse_bytes=values[21];
  result.score_ns=values[22]; result.core_ns=values[23]; result.inverse_ns=values[24];
  result.score_received_bytes=values[25]; result.core_received_bytes=values[26];
  result.inverse_received_bytes=values[27];
  result.ca_ns=values[28]; result.carrier_ns=values[29];
  result.t_payload_bytes=values[30];
  result.active_by_round.resize(rounds);
  result.vertices_by_round.resize(rounds);
  result.ca_prg_by_round.resize(rounds);
  const auto after_values=bytes.begin()+n+sizeof(values);
  std::copy_n(after_values,rounds*sizeof(std::uint64_t),
            reinterpret_cast<std::uint8_t*>(result.active_by_round.data()));
  std::copy_n(after_values+rounds*sizeof(std::uint64_t),rounds*sizeof(std::uint64_t),
            reinterpret_cast<std::uint8_t*>(result.vertices_by_round.data()));
  std::copy_n(after_values+2U*rounds*sizeof(std::uint64_t),rounds*sizeof(std::uint64_t),
            reinterpret_cast<std::uint8_t*>(result.ca_prg_by_round.data()));
  return result;
}
struct Totals { std::uint64_t cases=0,pool=0,active=0,score_evals=0,ca_evals=0,evals=0;
                std::uint64_t sent0=0,sent1=0;
                std::uint64_t offline_ns=0,online_ns=0,peak_kib=0;
                std::uint64_t package_bytes0=0,package_bytes1=0; };
void exec_role(const std::vector<std::string>& arguments) {
  std::vector<char*> argv;
  argv.reserve(arguments.size()+2U);
  argv.push_back(const_cast<char*>("/proc/self/exe"));
  for(const auto& item:arguments) argv.push_back(const_cast<char*>(item.c_str()));
  argv.push_back(nullptr);
  ::execv("/proc/self/exe",argv.data());
  _exit(127);
}
std::uint64_t number(const char* text) { return std::stoull(text); }
int fd(const char* text) { return std::stoi(text); }
int party_process(int argc,char** argv) {
  try {
    require(argc>=21,"E2E party arguments");
    const auto who=fd(argv[2]);
    require(who==0||who==1,"E2E party identity");
    ProtocolIAav86SmallConfig c{number(argv[3]),number(argv[4]),number(argv[5]),
        static_cast<std::uint32_t>(number(argv[6])),static_cast<std::uint32_t>(number(argv[7])),
        static_cast<std::uint32_t>(number(argv[8])),static_cast<std::uint8_t>(who),
        fd(argv[9]),argv[10]};
    require(argc==19+2*static_cast<int>(c.iterations),"E2E core fd count");
    const auto package_fd=fd(argv[11]),input_fd=fd(argv[12]),result_fd=fd(argv[13]);
    const std::array<int,2> score_fds{fd(argv[14]),fd(argv[15])};
    const auto inverse_fd=fd(argv[16]);
    std::vector<int> core_fds;
    for(int i=18;i<argc;++i) core_fds.push_back(fd(argv[i]));
    const auto material_bytes=receive_bytes(package_fd,64U*1024U*1024U);
    auto material=protocol_i_aav86_small_deserialize_party_material(material_bytes,who,c);
    if (!std::getenv("MOE_TOPK_M6A_E15_BENCH"))
      require(test_only::payload_from_shape(
                  material.padded_n,material.iterations,
                  material.comparison_bits,true).t_package_payload_bytes==
                  test_only::payload(material).t_package_payload_bytes,
              "E15 AAV86 shape versus serialized material");
    const char ready=1;
    send_all(fd(argv[17]),&ready,1);
    const auto pivot_seed_lo=material.pivot_seed_lo;
    const auto pivot_seed_hi=material.pivot_seed_hi;
    std::vector<std::uint32_t> shares(c.logical_n);
    receive_all(input_fd,shares.data(),shares.size()*sizeof(std::uint32_t));
    const auto start=std::chrono::steady_clock::now();
    const auto output=protocol_i_aav86_small_party(c,std::move(material),shares,
        score_fds,core_fds,inverse_fd);
    const auto elapsed=std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now()-start).count();
    const auto shape=protocol_i_aav86_small_capacity_shape(c.logical_n,c.iterations);
    const auto payload=test_only::payload_from_shape(
        shape.padded_n,c.iterations,shape.comparison_bits,true);
    send_bytes(result_fd,encode_result(output,static_cast<std::uint64_t>(elapsed),
        material_bytes.size(),payload.t_package_payload_bytes,
        pivot_seed_lo,pivot_seed_hi));
    return 0;
  } catch(const std::exception& e) {
    std::cerr<<"E7_PARTY_FAIL "<<e.what()<<"\n"; return 1;
  }
}
int dealer_process(int argc,char** argv) {
  try {
    require(argc==12,"E2E dealer arguments");
    ProtocolIAav86SmallConfig c{number(argv[2]),number(argv[3]),number(argv[4]),
        static_cast<std::uint32_t>(number(argv[5])),static_cast<std::uint32_t>(number(argv[6])),
        static_cast<std::uint32_t>(number(argv[7])),0,fd(argv[8]),""};
    const auto started=std::chrono::steady_clock::now();
    auto materials=protocol_i_aav86_small_dealer_generate(c);
    if(const auto* public_seed=std::getenv("MOE_TOPK_M6A_E11_PUBLIC_PIVOT_SEED")) {
      const auto lo=number(public_seed), hi=lo^UINT64_C(0x9e3779b97f4a7c15);
      materials.party0.pivot_seed_lo=materials.party1.pivot_seed_lo=lo;
      materials.party0.pivot_seed_hi=materials.party1.pivot_seed_hi=hi;
    }
    const auto generated=std::chrono::steady_clock::now();
    const auto bytes0=protocol_i_aav86_small_serialize_party_material(materials.party0);
    const auto bytes1=protocol_i_aav86_small_serialize_party_material(materials.party1);
    const auto serialized=std::chrono::steady_clock::now();
    send_bytes(fd(argv[9]),bytes0);
    send_bytes(fd(argv[10]),bytes1);
    const auto distributed=std::chrono::steady_clock::now();
    struct rusage usage{};
    require(::getrusage(RUSAGE_SELF,&usage)==0,"T getrusage");
    const auto ns=[](auto a,auto b) { return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(b-a).count()); };
    const std::array<std::uint64_t,4> telemetry{
        ns(started,generated),ns(generated,serialized),ns(serialized,distributed),
        static_cast<std::uint64_t>(usage.ru_maxrss)};
    send_all(fd(argv[11]),telemetry.data(),sizeof(telemetry));
    return 0;
  } catch(const std::exception& e) {
    std::cerr<<"E7_T_FAIL "<<e.what()<<"\n"; return 1;
  }
}
void run_case(const std::vector<std::uint32_t>& scores,std::uint32_t k,
              std::uint32_t r,std::uint64_t serial,Totals& totals) {
  const auto n=static_cast<std::uint32_t>(scores.size());
  auto c0=ProtocolIAav86SmallConfig{0xd70000+serial,0xe70000+serial,0xf70000+serial,
                                    n,k,r,0,15000,directory(serial,0)};
  auto c1=c0; c1.party=1; c1.durable_claim_directory=directory(serial,1);
  std::uint32_t d=2;
  while(d<n) d*=2;
  const auto transport_setup_start=std::chrono::steady_clock::now();
  std::array<std::array<int,2>,2> package{pair(),pair()};
  std::array<std::array<int,2>,2> input{pair(),pair()};
  std::array<std::array<int,2>,2> result{pair(),pair()};
  std::array<std::array<int,2>,2> ready{pair(),pair()};
  std::array<std::array<int,2>,2> score{online_pair(),online_pair()};
  std::vector<std::array<int,2>> core(2U*r+1U);
  for(auto& edge:core) edge=online_pair();
  const auto inverse=online_pair();
  const auto dealer_telemetry=pair();
  std::vector<int> all;
  for(const auto& a:package) all.insert(all.end(),a.begin(),a.end());
  for(const auto& a:input) all.insert(all.end(),a.begin(),a.end());
  for(const auto& a:result) all.insert(all.end(),a.begin(),a.end());
  for(const auto& a:ready) all.insert(all.end(),a.begin(),a.end());
  for(const auto& a:score) all.insert(all.end(),a.begin(),a.end());
  for(const auto& a:core) all.insert(all.end(),a.begin(),a.end());
  all.insert(all.end(),inverse.begin(),inverse.end());
  all.insert(all.end(),dealer_telemetry.begin(),dealer_telemetry.end());
  const auto transport_setup_end=std::chrono::steady_clock::now();
  const auto offline_start=transport_setup_end;
  std::array<pid_t,2> parties{};
  for(int who=0;who<2;++who) {
    const auto child=::fork(); require(child>=0,"E2E party fork");
    if(child==0) {
      std::vector<int> keep{package[who][1],input[who][1],result[who][1],ready[who][1],
                            score[0][who],score[1][who],inverse[who]};
      for(const auto& a:core) keep.push_back(a[who]);
      close_except(all,keep);
      const auto c=who?c1:c0;
      std::vector<std::string> args{"party",std::to_string(who),std::to_string(c.session),
          std::to_string(c.fingerprint),std::to_string(c.material_id),
          std::to_string(n),std::to_string(k),std::to_string(r),
          std::to_string(c.timeout_ms),c.durable_claim_directory,
          std::to_string(package[who][1]),std::to_string(input[who][1]),
          std::to_string(result[who][1]),std::to_string(score[0][who]),
          std::to_string(score[1][who]),std::to_string(inverse[who]),
          std::to_string(ready[who][1])};
      for(const auto& a:core) args.push_back(std::to_string(a[who]));
      exec_role(args);
    }
    parties[who]=child;
  }
  const auto dealer=::fork(); require(dealer>=0,"E2E dealer fork");
  if(dealer==0) {
    // exec discards the harness plaintext score vector and all inherited
    // address-space data. T keeps only two offline package descriptors.
    close_except(all,{package[0][0],package[1][0],dealer_telemetry[1]});
    exec_role({"dealer",std::to_string(c0.session),std::to_string(c0.fingerprint),
        std::to_string(c0.material_id),std::to_string(n),std::to_string(k),
        std::to_string(r),std::to_string(c0.timeout_ms),
        std::to_string(package[0][0]),std::to_string(package[1][0]),
        std::to_string(dealer_telemetry[1])});
  }
  std::vector<int> parent_keep{package[0][0],package[1][0],input[0][0],input[1][0],
      result[0][0],result[1][0],ready[0][0],ready[1][0],dealer_telemetry[0]};
  close_except(all,parent_keep);
  wait_ok(dealer,"T");
  const auto dealer_exited=std::chrono::steady_clock::now();
  std::array<std::uint64_t,4> dealer_metrics{};
  receive_all(dealer_telemetry[0],dealer_metrics.data(),sizeof(dealer_metrics));
  char ready_byte=0;
  receive_all(ready[0][0],&ready_byte,1); require(ready_byte==1,"P0 offline ready");
  receive_all(ready[1][0],&ready_byte,1); require(ready_byte==1,"P1 offline ready");
  const auto offline_end=std::chrono::steady_clock::now();
  const auto offline_ns=std::chrono::duration_cast<std::chrono::nanoseconds>(
      offline_end-offline_start).count();
  const auto receive_barrier_ns=std::chrono::duration_cast<std::chrono::nanoseconds>(
      offline_end-dealer_exited).count();
  std::mt19937_64 rng(0x770000+serial);
  std::vector<std::uint32_t> x0(n),x1(n);
  for(std::size_t i=0;i<n;++i) {
    x0[i]=std::getenv("MOE_TOPK_M6A_E15_BENCH")?fresh_share():static_cast<std::uint32_t>(rng());
    x1[i]=scores[i]-x0[i];
  }
  send_all(input[0][0],x0.data(),n*sizeof(std::uint32_t));
  send_all(input[1][0],x1.data(),n*sizeof(std::uint32_t));
  const auto a=decode_result(receive_bytes(result[0][0],1024),n,r);
  const auto b=decode_result(receive_bytes(result[1][0],1024),n,r);
  wait_ok(parties[0],"P0"); wait_ok(parties[1],"P1");
  for(const auto fd:parent_keep) ::close(fd);
  const auto oracle=top_k_mask(scores,k);
  std::uint32_t ones=0;
  for(std::size_t i=0;i<n;++i) {
    const auto bit=static_cast<std::uint8_t>(a.mask[i]^b.mask[i]);
    require(bit==oracle[i],"E2E oracle differential"); ones+=bit;
  }
  require(ones==k,"E2E exactly K");
  const auto slots=static_cast<std::uint64_t>(r)*d*(d-1U)/2U;
  require(a.pool==slots&&b.pool==slots&&a.active==b.active&&a.active<=slots&&
          a.score_evals==4U*d&&b.score_evals==4U*d&&
          a.ca_evals==2U*a.active&&b.ca_evals==2U*b.active&&
          a.evals==a.score_evals+a.ca_evals&&b.evals==b.score_evals+b.ca_evals&&
          a.rounds==2U*r+4U&&b.rounds==a.rounds&&a.sent==b.received&&b.sent==a.received&&
          a.edge_digest==b.edge_digest&&
          a.active_by_round==b.active_by_round&&
          a.vertices_by_round==b.vertices_by_round&&
          a.ca_prg_by_round==b.ca_prg_by_round&&
          a.active_vertices==b.active_vertices&&
          a.score_bytes+a.core_bytes+a.inverse_bytes==a.sent&&
          b.score_bytes+b.core_bytes+b.inverse_bytes==b.sent&&
          a.score_received_bytes+a.core_received_bytes+a.inverse_received_bytes==a.received&&
          b.score_received_bytes+b.core_received_bytes+b.inverse_received_bytes==b.received&&
          a.score_received_bytes==b.score_bytes&&a.core_received_bytes==b.core_bytes&&
          a.inverse_received_bytes==b.inverse_bytes&&
          b.score_received_bytes==a.score_bytes&&b.core_received_bytes==a.core_bytes&&
          b.inverse_received_bytes==a.inverse_bytes&&
          a.score_prg==b.score_prg&&a.ca_prg==b.ca_prg&&
          a.inverse_prg==0&&b.inverse_prg==0&&
          a.total_prg==a.score_prg+a.ca_prg&&
          b.total_prg==b.score_prg+b.ca_prg&&
          a.score_prg==4U*d*34U&&b.score_prg==4U*d*34U&&
          a.pivot_seed_lo==b.pivot_seed_lo&&a.pivot_seed_hi==b.pivot_seed_hi&&
          a.package_bytes>0&&b.package_bytes>0&&
          a.t_payload_bytes>0&&b.t_payload_bytes>0,
          "E2E metrics/trace");
  require(a.score_ns+a.core_ns+a.inverse_ns<=a.online_ns &&
          b.score_ns+b.core_ns+b.inverse_ns<=b.online_ns,
          "E11 stage timing bounds");
  require(a.ca_ns+a.carrier_ns==a.core_ns &&
          b.ca_ns+b.carrier_ns==b.core_ns,
          "E12 CA and carrier timing bounds");
  std::uint32_t bits=33;
  for(auto value=d-1U;value;value>>=1U) ++bits;
  std::uint64_t vertex_sum=0,ca_prg_sum=0;
  for(std::size_t t=0;t<r;++t) {
    require(a.ca_prg_by_round[t]==2U*a.active_by_round[t]*bits,
            "E2E PRG by iteration");
    vertex_sum+=a.vertices_by_round[t];
    ca_prg_sum+=a.ca_prg_by_round[t];
  }
  require(vertex_sum==a.active_vertices&&ca_prg_sum==a.ca_prg,
          "E2E vertex/PRG totals");
  if(d==2&&r==1) {
    // Four two-entry permutations, four two-entry word vectors, two node
    // masks, four 34-bit score keys/mask pairs, one 34-bit edge key.
    require(a.t_payload_bytes==4376&&b.t_payload_bytes==4376,
            "E14 n2 hand-counted AAV86 payload fixture");
  }
  totals.cases++; totals.pool+=slots; totals.active+=a.active;
  totals.score_evals+=a.score_evals; totals.ca_evals+=a.ca_evals; totals.evals+=a.evals;
  totals.sent0+=a.sent; totals.sent1+=b.sent;
  totals.offline_ns+=static_cast<std::uint64_t>(offline_ns);
  totals.online_ns+=std::max(a.online_ns,b.online_ns);
  totals.peak_kib=std::max({totals.peak_kib,a.peak_kib,b.peak_kib});
  totals.package_bytes0+=a.package_bytes; totals.package_bytes1+=b.package_bytes;
  std::cout<<"E12_AAV86_CASE serial="<<serial<<" n="<<n<<" k="<<k<<" r="<<r
           <<" d="<<d<<" reserved="<<slots<<" active="<<a.active
           <<" score_eval_per_party="<<a.score_evals
           <<" ca_eval_per_party="<<a.ca_evals
           <<" total_eval_per_party="<<a.evals<<" p0_bytes="<<a.sent
           <<" p1_bytes="<<b.sent<<" rounds="<<a.rounds
           <<" offline_elapsed_ns="<<offline_ns
           <<" transport_setup_ns="<<std::chrono::duration_cast<std::chrono::nanoseconds>(
                   transport_setup_end-transport_setup_start).count()
           <<" dealer_generate_ns="<<dealer_metrics[0]
           <<" dealer_serialize_ns="<<dealer_metrics[1]
           <<" dealer_distribute_ns="<<dealer_metrics[2]
           <<" receive_barrier_ns="<<receive_barrier_ns
           <<" t_exit=0 p0_exit=0 p1_exit=0"
           <<" peak_t_kib="<<dealer_metrics[3]
           <<" online_max_elapsed_ns="<<std::max(a.online_ns,b.online_ns)
           <<" online_p0_ns="<<a.online_ns<<" online_p1_ns="<<b.online_ns
           <<" peak_party_kib="<<std::max(a.peak_kib,b.peak_kib)
           <<" peak_p0_kib="<<a.peak_kib<<" peak_p1_kib="<<b.peak_kib
           <<" package_bytes_p0="<<a.package_bytes
           <<" package_bytes_p1="<<b.package_bytes
           <<" t_payload_bytes_p0="<<a.t_payload_bytes
           <<" t_payload_bytes_p1="<<b.t_payload_bytes
           <<" pivot_seed_lo="<<a.pivot_seed_lo
           <<" pivot_seed_hi="<<a.pivot_seed_hi
           <<" edge_digest="<<a.edge_digest<<" active_by_round=";
  for(const auto count:a.active_by_round) std::cout<<count<<",";
  std::cout<<" vertices_by_round=";
  for(const auto count:a.vertices_by_round) std::cout<<count<<",";
  std::cout<<" ca_prg_by_round=";
  for(const auto count:a.ca_prg_by_round) std::cout<<count<<",";
  std::cout<<" active_vertices="<<a.active_vertices
           <<" score_prg_per_party="<<a.score_prg
           <<" ca_prg_per_party="<<a.ca_prg
           <<" inverse_prg_per_party="<<a.inverse_prg
           <<" online_prg_per_party="<<a.total_prg
           <<" online_prg_total="<<(a.total_prg+b.total_prg);
  std::cout<<" p0_score_bytes="<<a.score_bytes<<" p0_core_bytes="<<a.core_bytes
           <<" p0_inverse_bytes="<<a.inverse_bytes
           <<" p1_score_bytes="<<b.score_bytes<<" p1_core_bytes="<<b.core_bytes
           <<" p1_inverse_bytes="<<b.inverse_bytes;
  std::cout<<" p0_received_bytes="<<a.received
           <<" p1_received_bytes="<<b.received
           <<" p0_score_received_bytes="<<a.score_received_bytes
           <<" p0_core_received_bytes="<<a.core_received_bytes
           <<" p0_inverse_received_bytes="<<a.inverse_received_bytes
           <<" p1_score_received_bytes="<<b.score_received_bytes
           <<" p1_core_received_bytes="<<b.core_received_bytes
           <<" p1_inverse_received_bytes="<<b.inverse_received_bytes
           <<" p0_score_ns="<<a.score_ns<<" p0_core_ns="<<a.core_ns
           <<" p0_ca_ns="<<a.ca_ns<<" p0_carrier_ns="<<a.carrier_ns
           <<" p0_inverse_ns="<<a.inverse_ns
           <<" p1_score_ns="<<b.score_ns<<" p1_core_ns="<<b.core_ns
           <<" p1_ca_ns="<<b.ca_ns<<" p1_carrier_ns="<<b.carrier_ns
           <<" p1_inverse_ns="<<b.inverse_ns
           <<" transport="<<(tcp_online()?"tcp":"unix")<<"\n";
  std::filesystem::remove_all(c0.durable_claim_directory);
  std::filesystem::remove_all(c1.durable_claim_directory);
}
void tcp_transport_negative() {
  auto check=[](const char* label,auto prepare) {
    auto fds=tcp_pair();
    prepare(fds[1]);
    const auto started=std::chrono::steady_clock::now();
    bool rejected=false;
    try {
      ProtocolIFramedChannel channel(fds[0],{91,92,8,2,40,0,1,4,7},120);
      (void)channel.receive();
    } catch(const std::exception&) { rejected=true; }
    ::close(fds[1]);
    const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now()-started).count();
    require(rejected&&elapsed<1000,"E11 TCP bounded failure");
    std::cout<<"E11_TCP_NEGATIVE case="<<label<<" elapsed_ms="<<elapsed<<" PASS\n";
  };
  check("closed",[](int peer) { ::shutdown(peer,SHUT_RDWR); });
  check("silent",[](int) {});
  check("truncated",[](int peer) {
    const std::array<std::uint8_t,4> prefix{0x50,0x4b,0x31,0x46};
    send_all(peer,prefix.data(),prefix.size());
    ::shutdown(peer,SHUT_WR);
  });
}
void transport_frame_conformance() {
  const std::array<std::uint8_t,6> phases{4,5,9,10,11,40};
  for(const auto phase:phases) {
    std::array<std::uint64_t,4> observed{};
    for(int mode=0;mode<2;++mode) {
      const auto sockets=mode?tcp_pair():pair();
      const ProtocolIFrameConfig c0{111,222,128,2,40,0,1,phase,7};
      const ProtocolIFrameConfig c1{111,222,128,2,40,1,0,phase,7};
      ProtocolIFramedChannel p0(sockets[0],c0,1000),p1(sockets[1],c1,1000);
      const std::vector<std::uint8_t> outbound(32U+phase,phase);
      const std::vector<std::uint8_t> reply(17U+phase,static_cast<std::uint8_t>(phase+1U));
      p0.send(outbound);
      require(p1.receive()==outbound,"E11 TCP frame P0/P1 payload");
      p1.send(reply);
      require(p0.receive()==reply,"E11 TCP frame P1/P0 payload");
      const std::array<std::uint64_t,4> counters{
          p0.sent_bytes(),p0.received_bytes(),p1.sent_bytes(),p1.received_bytes()};
      require(counters[0]==counters[3]&&counters[1]==counters[2],
              "E11 TCP frame cross-accounting");
      if(mode) require(observed==counters,"E11 Unix/TCP frame byte equivalence");
      else observed=counters;
    }
    std::cout<<"E11_FRAME_EQ phase="<<static_cast<unsigned>(phase)
             <<" p0_sent="<<observed[0]<<" p1_sent="<<observed[2]<<" PASS\n";
  }
}
}
int main(int argc,char** argv) {
  try {
    if(argc>1&&std::string(argv[1])=="party") return party_process(argc,argv);
    if(argc>1&&std::string(argv[1])=="dealer") return dealer_process(argc,argv);
    if(argc==2&&std::string(argv[1])=="transport-negative") {
      tcp_transport_negative(); return 0;
    }
    if(argc==2&&std::string(argv[1])=="transport-conformance") {
      transport_frame_conformance(); return 0;
    }
    Totals totals;
    if(argc==7&&std::string(argv[1])=="bench") {
      const auto n=static_cast<std::uint32_t>(number(argv[2]));
      const auto k=static_cast<std::uint32_t>(number(argv[3]));
      const auto r=static_cast<std::uint32_t>(number(argv[4]));
      const auto input_seed=number(argv[5]);
      const auto serial=number(argv[6]);
      require(n==128&&k>=1&&k<=n&&r>=2&&r<=5,"E11 benchmark shape");
      std::mt19937_64 input_rng(input_seed);
      std::uniform_int_distribution<std::int32_t> distribution(-32*4096,32*4096);
      std::vector<std::uint32_t> scores(n);
      std::uint64_t digest=UINT64_C(1469598103934665603);
      for(auto& score:scores) {
        score=static_cast<std::uint32_t>(distribution(input_rng));
        digest=(digest^score)*UINT64_C(1099511628211);
      }
      run_case(scores,k,r,serial,totals);
      std::cout<<"E12_BENCH_META input_seed="<<input_seed
               <<" input_digest="<<digest<<" serial="<<serial<<"\n";
      return 0;
    }
    if(const auto* tier=std::getenv("MOE_TOPK_M6A_E9_D")) {
      const auto d=static_cast<std::uint32_t>(std::stoul(tier));
      std::uint64_t serial=0;
      for(const auto& fixture:moe_topk_e9_test::fixtures(d))
        run_case(fixture.scores,fixture.k,fixture.r,++serial,totals);
      std::cout<<"E9_E2E_PASS d="<<d<<" cases="<<totals.cases
               <<" reserved_per_party_sum="<<totals.pool
               <<" active_edges_sum="<<totals.active<<" p0_sent_bytes_sum="<<totals.sent0
               <<" p1_sent_bytes_sum="<<totals.sent1
               <<" package_bytes_p0_sum="<<totals.package_bytes0
               <<" package_bytes_p1_sum="<<totals.package_bytes1<<"\n";
      return 0;
    }
    const std::vector<std::vector<std::uint32_t>> sets{
      {UINT32_C(0x80000000)}, {5,5,5},
      {UINT32_C(0x80000000),UINT32_C(0x7fffffff),7,7,0},
      {0,0,0,0,0,0,0,0}, {8,7,6,5,4,3,2,1}};
    std::uint64_t serial=0;
    for(const auto& scores:sets) for(const auto k:{1U,static_cast<unsigned>(scores.size())})
      for(const auto r:{1U,2U,3U,4U,5U}) run_case(scores,k,r,++serial,totals);
    std::cout<<"E7_E2E_PASS cases="<<totals.cases<<" reserved_per_party_sum="<<totals.pool
             <<" active_edges_sum="<<totals.active
             <<" score_eval_per_party_sum="<<totals.score_evals
             <<" ca_eval_per_party_sum="<<totals.ca_evals
             <<" total_eval_per_party_sum="<<totals.evals
             <<" p0_sent_bytes_sum="<<totals.sent0<<" p1_sent_bytes_sum="<<totals.sent1
             <<" offline_elapsed_ns_sum="<<totals.offline_ns
             <<" online_max_elapsed_ns_sum="<<totals.online_ns
             <<" peak_party_kib="<<totals.peak_kib
             <<" package_bytes_p0_sum="<<totals.package_bytes0
             <<" package_bytes_p1_sum="<<totals.package_bytes1<<"\n";
    return 0;
  } catch(const std::exception& e) {
    std::cerr<<"E7_E2E_FAIL "<<e.what()<<"\n"; return 1;
  }
}
