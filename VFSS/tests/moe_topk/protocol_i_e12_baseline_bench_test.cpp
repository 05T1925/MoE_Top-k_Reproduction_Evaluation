#include <moe_topk/protocol_i_pipeline.h>
#include <moe_topk/protocol_i_priority_key.h>
#include <moe_topk/protocol_i_score_input.h>
#include <moe_topk/protocol_i_transport.h>
#include <moe_topk/topk_oracle.h>
#include "protocol_i_e14_material_metrics.h"
#include <FSS/dcf.h>
#include <FSS/prng.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <random>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/random.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
namespace {
using namespace moe_topk; using Bytes=std::vector<std::uint8_t>;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::uint64_t secure_random_u64() {
  std::uint64_t value=0;
  auto* bytes=reinterpret_cast<std::uint8_t*>(&value);
  std::size_t remaining=sizeof(value);
  while(remaining) {
    const auto read=::getrandom(bytes,remaining,0);
    if(read<0&&errno==EINTR) continue;
    require(read>0,"E12 baseline entropy");
    bytes+=read; remaining-=static_cast<std::size_t>(read);
  }
  return value;
}
bool benchmark_mode(){return std::getenv("MOE_TOPK_M6A_E15_BENCH")!=nullptr;}
struct FreshRandom {std::uint64_t operator()() const {return secure_random_u64();}};
std::uint64_t elapsed_ns(std::chrono::steady_clock::time_point from,
                         std::chrono::steady_clock::time_point to){
  return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(to-from).count());
}
void send_exact(int fd,const void* data,std::size_t size){
  auto* bytes=static_cast<const std::uint8_t*>(data);
  while(size){const auto count=::write(fd,bytes,size);require(count>0,"E12 telemetry write");
    bytes+=count;size-=static_cast<std::size_t>(count);}
}
void receive_exact(int fd,void* data,std::size_t size){
  auto* bytes=static_cast<std::uint8_t*>(data);
  while(size){const auto count=::read(fd,bytes,size);require(count>0,"E12 telemetry read");
    bytes+=count;size-=static_cast<std::size_t>(count);}
}

class FdPool final {
 public:
  ~FdPool() { close_all(); }

  void socket_pair(std::array<int,2>& pair) {
    pair = {{-1,-1}};
    require(::socketpair(AF_UNIX,SOCK_STREAM,0,pair.data())==0,"socketpair");
    fds_.push_back(&pair[0]);
    fds_.push_back(&pair[1]);
  }

  void online_pair(std::array<int,2>& pair) {
    const auto* transport=std::getenv("MOE_TOPK_M2_E12_TRANSPORT");
    if(!benchmark_mode()||(transport&&std::string(transport)=="unix")){
      socket_pair(pair);return;
    }
    require(!transport||std::string(transport)=="tcp","E12 baseline transport name");
    const int listener=::socket(AF_INET,SOCK_STREAM,0);
    require(listener>=0,"E12 TCP listener");
    sockaddr_in address{};address.sin_family=AF_INET;
    address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    require(::bind(listener,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0,
            "E12 TCP bind");
    require(::listen(listener,1)==0,"E12 TCP listen");
    socklen_t length=sizeof(address);
    require(::getsockname(listener,reinterpret_cast<sockaddr*>(&address),&length)==0,
            "E12 TCP address");
    const int connector=::socket(AF_INET,SOCK_STREAM,0);
    require(connector>=0&&::connect(connector,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0,
            "E12 TCP connect");
    const int accepted=::accept(listener,nullptr,nullptr);
    ::close(listener);
    require(accepted>=0,"E12 TCP accept");
    const int enabled=1;
    require(::setsockopt(connector,IPPROTO_TCP,TCP_NODELAY,&enabled,sizeof(enabled))==0&&
            ::setsockopt(accepted,IPPROTO_TCP,TCP_NODELAY,&enabled,sizeof(enabled))==0,
            "E12 TCP_NODELAY");
    pair={connector,accepted};
    fds_.push_back(&pair[0]);fds_.push_back(&pair[1]);
  }

  void close_except(const std::vector<int>& keep) {
    for (auto* fd : fds_) {
      if (*fd >= 0 && std::find(keep.begin(),keep.end(),*fd)==keep.end()) {
        ::close(*fd);
        *fd=-1;
      }
    }
  }

  void close_all() { close_except({}); }

 private:
  std::vector<int*> fds_;
};

class ChildSet final {
 public:
  ~ChildSet() {
    for (const auto child : children_) if (child > 0) ::kill(child,SIGTERM);
    for (const auto child : children_) if (child > 0) while (::waitpid(child,nullptr,0)<0 && errno==EINTR) {}
  }

  void add(pid_t child) { children_.push_back(child); }

  void wait_ok(pid_t child) {
    int status=0;
    pid_t result;
    do result=::waitpid(child,&status,0); while (result<0 && errno==EINTR);
    require(result==child && WIFEXITED(status) && WEXITSTATUS(status)==0,"child failure");
    for (auto& tracked : children_) if (tracked==child) { tracked=-1; return; }
    throw std::runtime_error("unknown child");
  }

 private:
  std::vector<pid_t> children_;
};

void seed_fss(){for(int i=0;i<256;++i)FSSConfig::prngs[i].SetSeed(
    osuCrypto::toBlock(secure_random_u64(),secure_random_u64()));}
struct Case{std::uint32_t logical_n,k;std::uint64_t session,fingerprint,seed;unsigned p0_style,p1_style,score_style;};
ProtocolIPriorityPipelineConfig config_for(const Case&t,int party){const auto l=protocol_i_make_input_layout(t.logical_n,t.k);return{t.session,t.fingerprint,l.logical_n,l.padded_n,t.k,l.minimum_comparison_bits,static_cast<std::uint8_t>(party),15000};}
std::pair<ProtocolIPartyPackage,ProtocolIPartyPackage> make_packages(const ProtocolIPriorityPipelineConfig&c,std::uint64_t seed){seed_fss();FreshRandom r;const auto ring=(UINT64_C(1)<<c.comparison_bits)-1U,score_ring=(UINT64_C(1)<<34)-1U;ProtocolIPartyPackage p0,p1;for(auto*p:{&p0,&p1}){p->session=c.session;p->fingerprint=c.fingerprint;p->n=c.padded_n;p->k=c.k;p->comparison_bits=c.comparison_bits;}p0.party=0;p1.party=1;p0.node_mask_shares.resize(c.padded_n);p1.node_mask_shares.resize(c.padded_n);std::vector<std::uint64_t>full(c.padded_n);for(std::size_t i=0;i<full.size();++i){full[i]=r()&ring;p0.node_mask_shares[i]=r()&ring;p1.node_mask_shares[i]=(full[i]-p0.node_mask_shares[i])&ring;}for(std::uint32_t a=0;a<c.padded_n;++a)for(std::uint32_t b=a+1;b<c.padded_n;++b){ProtocolIUcmpMaterial m(c.comparison_bits,full[a],full[b]);p0.edge_materials.emplace_back(a,b,m.export_party_material(0));p1.edge_materials.emplace_back(a,b,m.export_party_material(1));}auto score=[&](std::uint8_t stage){for(std::uint32_t slot=0;slot<c.padded_n;++slot){const auto left=r()&score_ring,right=r()&score_ring,l0=r()&score_ring,r0=r()&score_ring;ProtocolIUcmpMaterial m(34,left,right);ProtocolIScoreInputPartyMaterial a(slot,stage,l0,r0,m.export_party_material(0)),b(slot,stage,(left-l0)&score_ring,(right-r0)&score_ring,m.export_party_material(1));if(stage==1){p0.carry_materials.push_back(std::move(a));p1.carry_materials.push_back(std::move(b));}else{p0.sign_materials.push_back(std::move(a));p1.sign_materials.push_back(std::move(b));}}};score(1);score(2);return{std::move(p0),std::move(p1)};}
ProtocolIPermutation permutation(std::uint32_t n,unsigned style,std::uint64_t seed){auto p=protocol_i_identity_permutation(n);if(style==1)std::reverse(p.begin(),p.end());else if(style==2)for(std::uint32_t i=0;i+1<n;i+=2)std::swap(p[i],p[i+1]);else if(style==3)std::rotate(p.begin(),p.begin()+1,p.end());else if(style==4)p=protocol_i_test_permutation(n,seed);else if(style==5)for(std::uint32_t remaining=n;remaining>1;--remaining){const auto limit=UINT64_MAX-(UINT64_MAX%remaining);std::uint64_t value;do value=secure_random_u64();while(value>=limit);std::swap(p[remaining-1],p[value%remaining]);}return p;}
Bytes encode_words(const std::vector<std::uint64_t>&v){Bytes b;for(auto x:v)for(int s=56;s>=0;s-=8)b.push_back(x>>s);return b;}std::vector<std::uint64_t>decode_words(const Bytes&b){require(b.size()%8==0,"word encoding");std::vector<std::uint64_t>v(b.size()/8);for(std::size_t i=0;i<v.size();++i)for(int j=0;j<8;++j)v[i]=(v[i]<<8U)|b[8*i+j];return v;}
Bytes encode_result(const ProtocolIPriorityPipelineOutput&o,const ProtocolIScoreInputMetrics&s,
                    std::uint64_t package_bytes,std::uint64_t score_ns,
                    std::uint64_t pipeline_ns,std::uint64_t online_ns,
                    const test_only::MaterialPayload& payload,
                    std::uint64_t score_prg,std::uint64_t pipeline_prg){
  Bytes b=o.xor_mask_share;
  struct rusage usage{};require(::getrusage(RUSAGE_SELF,&usage)==0,"E12 party RSS");
  const std::vector<std::uint64_t>m{package_bytes,s.carry_sent_bytes,s.carry_received_bytes,
      s.sign_sent_bytes,s.sign_received_bytes,s.ucmp_calls,s.raw_dcf_calls,s.rounds,
      o.metrics.forward_sent_bytes,o.metrics.forward_received_bytes,
      o.metrics.cmpagg_sent_bytes,o.metrics.cmpagg_received_bytes,
      o.metrics.rank_reveal_sent_bytes,o.metrics.rank_reveal_received_bytes,
      o.metrics.reverse_sent_bytes,o.metrics.reverse_received_bytes,
      o.metrics.comparison_edges,o.metrics.raw_dcf_calls,o.metrics.online_rounds,
      score_ns,pipeline_ns,online_ns,static_cast<std::uint64_t>(usage.ru_maxrss),
      payload.t_package_payload_bytes,payload.local_shuffle_payload_bytes,
      payload.local_shuffle_ot_sent_bytes,payload.local_shuffle_ot_received_bytes,
      score_prg,pipeline_prg};
  const auto tail=encode_words(m);b.insert(b.end(),tail.begin(),tail.end());return b;
}
int p2_main(const Case&t,int fd0,int fd1,int telemetry_fd){
  try{
    const auto started=std::chrono::steady_clock::now();
    const auto c0=config_for(t,0),c1=config_for(t,1);
    auto[p0,p1]=make_packages(c0,t.seed);
    const auto generated=std::chrono::steady_clock::now();
    const auto bytes0=serialize_party_package(p0),bytes1=serialize_party_package(p1);
    const auto serialized=std::chrono::steady_clock::now();
    ProtocolIFramedChannel to0(fd0,{t.session,t.fingerprint,c0.padded_n,t.k,c0.comparison_bits,2,0,1,1},c0.timeout_ms),
                           to1(fd1,{t.session,t.fingerprint,c1.padded_n,t.k,c1.comparison_bits,2,1,1,1},c1.timeout_ms);
    protocol_i_send_framed_chunks(to0,bytes0);
    protocol_i_send_framed_chunks(to1,bytes1);
    const auto distributed=std::chrono::steady_clock::now();
    struct rusage usage{};require(::getrusage(RUSAGE_SELF,&usage)==0,"E12 T RSS");
    const std::array<std::uint64_t,4> metrics{
        elapsed_ns(started,generated),elapsed_ns(generated,serialized),
        elapsed_ns(serialized,distributed),static_cast<std::uint64_t>(usage.ru_maxrss)};
    send_exact(telemetry_fd,metrics.data(),sizeof(metrics));
    return 0;
  }catch(const std::exception&e){std::cerr<<"P2: "<<e.what()<<'\n';return 1;}
}
int party_main(const Case&t,int who,const std::array<int,4>&offline,
               const std::array<int,2>&forward,const std::array<int,2>&reverse,
               const std::array<int,2>&score_fds,int package_fd,int ready_fd,
               int input_fd,int cmp_fd,int rank_fd,int result_fd){
  try {
    const auto c=config_for(t,who);
    ProtocolIFramedChannel package_channel(package_fd,
        {t.session,t.fingerprint,c.padded_n,t.k,c.comparison_bits,
         static_cast<std::uint8_t>(who),2,1,1},c.timeout_ms);
    auto package=deserialize_party_package(
        protocol_i_receive_framed_chunks(package_channel,64U*1024U*1024U),who);
    auto material=protocol_i_shuffle_preprocess_party(
        {t.session,t.fingerprint,t.seed+100,t.seed+200,c.padded_n,2,
         static_cast<std::uint8_t>(who),c.timeout_ms},offline,
        permutation(c.padded_n,who?t.p1_style:t.p0_style,t.seed+who));
    test_only::MaterialPayload diagnostic;
    if (!benchmark_mode()) diagnostic=test_only::payload(package,material);
    ProtocolIFramedChannel ready(ready_fd,
        {t.session,t.fingerprint,c.padded_n,t.k,c.comparison_bits,
         static_cast<std::uint8_t>(who),2,5,1},c.timeout_ms);
    ready.send({1});
    ProtocolIFramedChannel input(input_fd,
        {t.session,t.fingerprint,c.padded_n,t.k,c.comparison_bits,
         static_cast<std::uint8_t>(who),2,4,1},c.timeout_ms);
    const auto encoded=decode_words(input.receive());
    require(encoded.size()==t.logical_n,"raw input length");
    std::vector<std::uint32_t> shares;
    for(const auto x:encoded){
      require(x<=UINT32_MAX,"raw input width");
      shares.push_back(static_cast<std::uint32_t>(x));
    }
    resetDCFOnlinePrgCalls();
    const auto online_started=std::chrono::steady_clock::now();
    ProtocolIScoreInputMetrics score_metrics;
    const auto keys=protocol_i_raw_score_input_party(
        {t.session,t.fingerprint,c.logical_n,c.padded_n,t.k,
         protocol_i_make_input_layout(t.logical_n,t.k).index_bits,
         c.comparison_bits,static_cast<std::uint8_t>(who),c.timeout_ms},
        package,shares,score_fds,&score_metrics);
    const auto score_ended=std::chrono::steady_clock::now();
    const auto score_prg=readDCFOnlinePrgCalls();
    const auto output=protocol_i_priority_pipeline_party(
        c,std::move(package),material,keys,forward,cmp_fd,rank_fd,reverse);
    const auto pipeline_ended=std::chrono::steady_clock::now();
    const auto pipeline_prg=readDCFOnlinePrgCalls()-score_prg;
    require(material.forward_consumed&&material.reverse_consumed,
            "shuffle material consumption");
    const auto ot_sent=
        material.forward_po_first.counters.offline_ot.sent_bytes+
        material.forward_po_second.counters.offline_ot.sent_bytes+
        material.reverse_po_first.counters.offline_ot.sent_bytes+
        material.reverse_po_second.counters.offline_ot.sent_bytes+
        material.forward_do_first.counters.offline_ot.sent_bytes+
        material.forward_do_second.counters.offline_ot.sent_bytes+
        material.reverse_do_first.counters.offline_ot.sent_bytes+
        material.reverse_do_second.counters.offline_ot.sent_bytes;
    const auto ot_received=
        material.forward_po_first.counters.offline_ot.received_bytes+
        material.forward_po_second.counters.offline_ot.received_bytes+
        material.reverse_po_first.counters.offline_ot.received_bytes+
        material.reverse_po_second.counters.offline_ot.received_bytes+
        material.forward_do_first.counters.offline_ot.received_bytes+
        material.forward_do_second.counters.offline_ot.received_bytes+
        material.reverse_do_first.counters.offline_ot.received_bytes+
        material.reverse_do_second.counters.offline_ot.received_bytes;
    const auto payload=test_only::payload_from_shape(
        c.padded_n,0,c.comparison_bits,false,ot_sent,ot_received);
    if (!benchmark_mode()) {
      require(payload.t_package_payload_bytes==diagnostic.t_package_payload_bytes&&
              payload.local_shuffle_payload_bytes==diagnostic.local_shuffle_payload_bytes&&
              payload.local_shuffle_ot_sent_bytes==diagnostic.local_shuffle_ot_sent_bytes&&
              payload.local_shuffle_ot_received_bytes==diagnostic.local_shuffle_ot_received_bytes,
              "E15 baseline shape versus serialized material");
    }
    ProtocolIFramedChannel result(result_fd,
        {t.session,t.fingerprint,c.padded_n,t.k,c.comparison_bits,
         static_cast<std::uint8_t>(who),2,6,1},c.timeout_ms);
    result.send(encode_result(output,score_metrics,
        package_channel.received_bytes(),elapsed_ns(online_started,score_ended),
        elapsed_ns(score_ended,pipeline_ended),
        elapsed_ns(online_started,pipeline_ended),payload,score_prg,pipeline_prg));
    return 0;
  } catch(const std::exception&e) {
    std::cerr<<"P"<<who<<": "<<e.what()<<'\n';return 1;
  }
}
pid_t launch(const char*self,const std::vector<std::string>&v,FdPool&fds,
             const std::vector<int>&keep,ChildSet&children){
  const auto p=fork();
  require(p>=0,"fork");
  if(!p){
    fds.close_except(keep);
    std::vector<char*>a{const_cast<char*>(self)};
    for(const auto&s:v)a.push_back(const_cast<char*>(s.c_str()));
    a.push_back(nullptr);
    execv(self,a.data());
    _exit(127);
  }
  children.add(p);
  return p;
}
std::string executable(const char*fallback){char p[4096]{};const auto n=readlink("/proc/self/exe",p,sizeof(p)-1);return n>0?std::string(p,n):fallback;}
std::vector<std::uint32_t> scores_for(const Case&t,std::uint64_t input_seed){
  std::vector<std::uint32_t>s(t.logical_n);
  if(t.score_style==5){
    std::mt19937_64 input_rng(input_seed);
    std::uniform_int_distribution<std::int32_t> distribution(-32*4096,32*4096);
    for(auto&x:s)x=static_cast<std::uint32_t>(distribution(input_rng));
    return s;
  }
  std::mt19937_64 r(t.seed+900);
  for(auto&x:s)x=static_cast<std::uint32_t>(r());
  if(t.score_style==1)std::fill(s.begin(),s.end(),5);
  if(t.score_style==2)std::fill(s.begin(),s.end(),UINT32_C(0x80000000));
  if(t.score_style==3)for(std::size_t i=0;i<s.size();++i)s[i]=i%2?UINT32_C(0x7fffffff):UINT32_C(0x80000000);
  if(t.score_style==4)for(std::size_t i=0;i<s.size();++i)s[i]=static_cast<std::uint32_t>(i);
  return s;
}
void run_case(const char*self,const Case&t,std::uint64_t input_seed=0){
  FdPool fds;
  ChildSet children;
  const auto c=config_for(t,0);
  const auto scores=scores_for(t,input_seed);
  const auto transport_start=std::chrono::steady_clock::now();
  std::array<std::array<int,2>,4>offline;
  std::array<std::array<int,2>,2>forward,reverse,score;
  for(auto&x:offline)fds.socket_pair(x);
  for(auto&x:forward)fds.online_pair(x);
  for(auto&x:reverse)fds.online_pair(x);
  for(auto&x:score)fds.online_pair(x);
  std::array<int,2> package0,package1,ready0,ready1,input0,input1,cmp,rank,result0,result1;
  for(auto*x:{&package0,&package1,&ready0,&ready1,&input0,&input1,&result0,&result1})fds.socket_pair(*x);
  std::array<int,2> dealer_telemetry;fds.socket_pair(dealer_telemetry);
  fds.online_pair(cmp);fds.online_pair(rank);
  // Only party-to-party online edges traverse the shaped TCP qdisc.
  const auto transport_end=std::chrono::steady_clock::now();
  auto party_fds=[&](int who){
    std::vector<int>v;
    for(const auto&x:offline)v.push_back(x[who]);
    for(const auto&x:forward)v.push_back(x[who]);
    for(const auto&x:reverse)v.push_back(x[who]);
    for(const auto&x:score)v.push_back(x[who]);
    v.insert(v.end(),{who?package1[1]:package0[1],who?ready1[1]:ready0[1],who?input1[1]:input0[1],cmp[who],rank[who],who?result1[1]:result0[1]});
    return v;
  };
  auto role_args=[&](int who){
    std::vector<std::string>v{"party",std::to_string(who)};
    for(const auto&x:offline)v.push_back(std::to_string(x[who]));
    for(const auto&x:forward)v.push_back(std::to_string(x[who]));
    for(const auto&x:reverse)v.push_back(std::to_string(x[who]));
    for(const auto&x:score)v.push_back(std::to_string(x[who]));
    v.insert(v.end(),{std::to_string(who?package1[1]:package0[1]),std::to_string(who?ready1[1]:ready0[1]),std::to_string(who?input1[1]:input0[1]),std::to_string(cmp[who]),std::to_string(rank[who]),std::to_string(who?result1[1]:result0[1]),std::to_string(t.logical_n),std::to_string(t.k),std::to_string(t.session),std::to_string(t.fingerprint),std::to_string(t.seed),std::to_string(t.p0_style),std::to_string(t.p1_style),std::to_string(t.score_style)});
    return v;
  };
  const std::vector<std::string>dealer{"p2",std::to_string(package0[0]),std::to_string(package1[0]),std::to_string(dealer_telemetry[1]),std::to_string(t.logical_n),std::to_string(t.k),std::to_string(t.session),std::to_string(t.fingerprint),std::to_string(t.seed),std::to_string(t.p0_style),std::to_string(t.p1_style),std::to_string(t.score_style)};
  const auto offline_start=std::chrono::steady_clock::now();
  const auto p0=launch(self,role_args(0),fds,party_fds(0),children);
  const auto p1=launch(self,role_args(1),fds,party_fds(1),children);
  const auto p2=launch(self,dealer,fds,{package0[0],package1[0],dealer_telemetry[1]},children);
  // The production framed transport rejects a terminal HUP even when the
  // final frame is readable.  Keep the controller aliases for this case so
  // the harness preserves that transport contract; FdPool closes them before
  // the next case rather than accumulating them across the test matrix.
  children.wait_ok(p2);
  const auto dealer_exited=std::chrono::steady_clock::now();
  std::array<std::uint64_t,4> dealer_metrics{};
  receive_exact(dealer_telemetry[0],dealer_metrics.data(),sizeof(dealer_metrics));
  ProtocolIFramedChannel ready_p0(ready0[0],{t.session,t.fingerprint,c.padded_n,t.k,c.comparison_bits,2,0,5,1},c.timeout_ms),ready_p1(ready1[0],{t.session,t.fingerprint,c.padded_n,t.k,c.comparison_bits,2,1,5,1},c.timeout_ms);
  require(ready_p0.receive()==Bytes{1}&&ready_p1.receive()==Bytes{1},"offline readiness");
  const auto offline_end=std::chrono::steady_clock::now();
  std::mt19937_64 r(t.seed+999);
  std::vector<std::uint64_t>x0(t.logical_n),x1(t.logical_n);
  for(std::size_t i=0;i<x0.size();++i){x0[i]=benchmark_mode()?static_cast<std::uint32_t>(secure_random_u64()):static_cast<std::uint32_t>(r());x1[i]=static_cast<std::uint32_t>(scores[i]-static_cast<std::uint32_t>(x0[i]));}
  ProtocolIFramedChannel input_p0(input0[0],{t.session,t.fingerprint,c.padded_n,t.k,c.comparison_bits,2,0,4,1},c.timeout_ms),input_p1(input1[0],{t.session,t.fingerprint,c.padded_n,t.k,c.comparison_bits,2,1,4,1},c.timeout_ms);
  input_p0.send(encode_words(x0));
  input_p1.send(encode_words(x1));
  ProtocolIFramedChannel result_p0(result0[0],{t.session,t.fingerprint,c.padded_n,t.k,c.comparison_bits,2,0,6,1},c.timeout_ms),result_p1(result1[0],{t.session,t.fingerprint,c.padded_n,t.k,c.comparison_bits,2,1,6,1},c.timeout_ms);
  const auto out0=result_p0.receive(),out1=result_p1.receive();
  fds.close_all();
  children.wait_ok(p0);
  children.wait_ok(p1);
  require(out0.size()==t.logical_n+29*8&&out1.size()==out0.size(),"result shape");
  const auto want=top_k_mask(scores,t.k);
  for(std::size_t i=0;i<t.logical_n;++i)require((out0[i]^out1[i])==want[i],"oracle mask");
  const auto m0=decode_words(Bytes(out0.begin()+t.logical_n,out0.end())),m1=decode_words(Bytes(out1.begin()+t.logical_n,out1.end()));
  for(const auto&m:{m0,m1})require(m[0]>48&&m[5]==2U*c.padded_n&&m[6]==4U*c.padded_n&&m[7]==2&&m[16]==static_cast<std::uint64_t>(c.padded_n)*(c.padded_n-1U)/2U&&m[17]==m[16]*2U&&m[18]==6&&m[23]>0&&m[24]>0&&m[27]>0&&m[28]>0,"metrics audit");
  require(m0[25]==m1[26]&&m1[25]==m0[26],"E14 offline OT byte conservation");
  require(m0[27]==m0[6]*34U&&m1[27]==m1[6]*34U&&
          m0[28]==m0[17]*c.comparison_bits&&
          m1[28]==m1[17]*c.comparison_bits,
          "E14 DCF length-doubling fixture");
  if(c.padded_n==2){
    // Four 34-bit score keys + one 34-bit edge key; 840 payload bytes/key.
    // Four score mask pairs, two node mask shares.  Four shuffle stages at
    // T=2 retain two PO and two DO states per party.
    require(m0[23]==4280&&m1[23]==4280&&m0[24]==432&&m1[24]==432&&
            m0[27]==272&&m1[27]==272&&m0[28]==68&&m1[28]==68,
            "E14 n2 hand-counted material/PRG fixture");
  }
  if(benchmark_mode()){
    const auto sent=[](const auto&m){return m[1]+m[3]+m[8]+m[10]+m[12]+m[14];};
    const auto received=[](const auto&m){return m[2]+m[4]+m[9]+m[11]+m[13]+m[15];};
    require(sent(m0)==received(m1)&&sent(m1)==received(m0),
            "E12 baseline stage communication conservation");
    require(m0[19]+m0[20]==m0[21]&&m1[19]+m1[20]==m1[21],
            "E12 baseline online timing conservation");
    std::uint64_t digest=UINT64_C(1469598103934665603);
    for(auto value:scores)digest=(digest^value)*UINT64_C(1099511628211);
    const auto offline_ns=elapsed_ns(offline_start,offline_end);
    const auto transport_ns=elapsed_ns(transport_start,transport_end);
    std::cout<<"E12_BASELINE_CASE n="<<t.logical_n<<" k="<<t.k
             <<" d="<<c.padded_n<<" input_seed="<<input_seed
             <<" input_digest="<<digest<<" offline_ns="<<offline_ns
             <<" dealer_generate_ns="<<dealer_metrics[0]
             <<" dealer_serialize_ns="<<dealer_metrics[1]
             <<" dealer_distribute_ns="<<dealer_metrics[2]
             <<" receive_barrier_ns="<<elapsed_ns(dealer_exited,offline_end)
             <<" peak_t_kib="<<dealer_metrics[3]
             <<" transport_setup_ns="<<transport_ns
             <<" online_p0_ns="<<m0[21]<<" online_p1_ns="<<m1[21]
             <<" online_max_ns="<<std::max(m0[21],m1[21])
             <<" p0_score_ns="<<m0[19]<<" p1_score_ns="<<m1[19]
             <<" p0_pipeline_ns="<<m0[20]<<" p1_pipeline_ns="<<m1[20]
             <<" p0_package_bytes="<<m0[0]<<" p1_package_bytes="<<m1[0]
             <<" p0_t_payload_bytes="<<m0[23]<<" p1_t_payload_bytes="<<m1[23]
             <<" p0_shuffle_payload_bytes="<<m0[24]
             <<" p1_shuffle_payload_bytes="<<m1[24]
             <<" p0_ot_sent_bytes="<<m0[25]<<" p1_ot_sent_bytes="<<m1[25]
             <<" p0_ot_received_bytes="<<m0[26]
             <<" p1_ot_received_bytes="<<m1[26]
             <<" p0_score_dcf_prg="<<m0[27]<<" p1_score_dcf_prg="<<m1[27]
             <<" p0_pipeline_dcf_prg="<<m0[28]
             <<" p1_pipeline_dcf_prg="<<m1[28]
             <<" edges="<<m0[16]<<" dcf_eval_per_party="<<m0[6]+m0[17]
             <<" rounds="<<m0[7]+m0[18]
             <<" p0_sent_bytes="<<sent(m0)<<" p1_sent_bytes="<<sent(m1)
             <<" p0_received_bytes="<<received(m0)
             <<" p1_received_bytes="<<received(m1)
             <<" p0_score_sent_bytes="<<m0[1]+m0[3]
             <<" p1_score_sent_bytes="<<m1[1]+m1[3]
             <<" p0_score_received_bytes="<<m0[2]+m0[4]
             <<" p1_score_received_bytes="<<m1[2]+m1[4]
             <<" p0_forward_sent_bytes="<<m0[8]
             <<" p1_forward_sent_bytes="<<m1[8]
             <<" p0_forward_received_bytes="<<m0[9]
             <<" p1_forward_received_bytes="<<m1[9]
             <<" p0_cmpagg_sent_bytes="<<m0[10]
             <<" p1_cmpagg_sent_bytes="<<m1[10]
             <<" p0_cmpagg_received_bytes="<<m0[11]
             <<" p1_cmpagg_received_bytes="<<m1[11]
             <<" p0_reveal_sent_bytes="<<m0[12]
             <<" p1_reveal_sent_bytes="<<m1[12]
             <<" p0_reveal_received_bytes="<<m0[13]
             <<" p1_reveal_received_bytes="<<m1[13]
             <<" p0_reverse_sent_bytes="<<m0[14]
             <<" p1_reverse_sent_bytes="<<m1[14]
             <<" p0_reverse_received_bytes="<<m0[15]
             <<" p1_reverse_received_bytes="<<m1[15]
             <<" peak_p0_kib="<<m0[22]<<" peak_p1_kib="<<m1[22]
             <<" t_exit=0 p0_exit=0 p1_exit=0\n";
  }
}
Case parse_case(int n,char**v,int at){require(n>=at+8,"case arguments");return{static_cast<std::uint32_t>(std::stoul(v[at])),static_cast<std::uint32_t>(std::stoul(v[at+1])),std::stoull(v[at+2]),std::stoull(v[at+3]),std::stoull(v[at+4]),static_cast<unsigned>(std::stoul(v[at+5])),static_cast<unsigned>(std::stoul(v[at+6])),static_cast<unsigned>(std::stoul(v[at+7]))};}int fd(const char*x){return std::stoi(x);}
}
int main(int argc,char**argv){try{if(argc>1&&std::string(argv[1])=="p2"){const auto t=parse_case(argc,argv,5);return p2_main(t,fd(argv[2]),fd(argv[3]),fd(argv[4]));}if(argc>1&&std::string(argv[1])=="party"){const int who=std::stoi(argv[2]);int at=3;std::array<int,4>o{};std::array<int,2>f{},r{},s{};for(auto&x:o)x=fd(argv[at++]);for(auto&x:f)x=fd(argv[at++]);for(auto&x:r)x=fd(argv[at++]);for(auto&x:s)x=fd(argv[at++]);const int package=fd(argv[at++]),ready=fd(argv[at++]),input=fd(argv[at++]),cmp=fd(argv[at++]),rank=fd(argv[at++]),result=fd(argv[at++]);return party_main(parse_case(argc,argv,at),who,o,f,r,s,package,ready,input,cmp,rank,result);}const auto self=executable(argv[0]);if(argc==6&&std::string(argv[1])=="bench"){const auto n=static_cast<std::uint32_t>(std::stoul(argv[2]));const auto k=static_cast<std::uint32_t>(std::stoul(argv[3]));const auto input_seed=std::stoull(argv[4]);const auto serial=std::stoull(argv[5]);require((n==128||n==256)&&(k==2||k==8),"E16 baseline benchmark shape");require(benchmark_mode()&&serial!=input_seed,"E12 baseline input seed isolation");run_case(self.c_str(),{n,k,0x120000+serial,0x220000+serial,serial,5,5,5},input_seed);return 0;}if(const auto*n=std::getenv("MOE_TOPK_M2_E2E_N")){const auto logical=static_cast<std::uint32_t>(std::stoul(n));const auto*k=std::getenv("MOE_TOPK_M2_E2E_K");run_case(self.c_str(),{logical,k?static_cast<std::uint32_t>(std::stoul(k)):1,0x213888,0x313888,888,4,3,0});return 0;}const std::array<std::uint32_t,11>sizes{{1,2,3,4,5,7,8,11,16,17,31}};std::uint64_t serial=0;for(const auto n:sizes){std::vector<std::uint32_t>ks{1,n,static_cast<std::uint32_t>((n+1)/2)};std::sort(ks.begin(),ks.end());ks.erase(std::unique(ks.begin(),ks.end()),ks.end());for(const auto k:ks){run_case(self.c_str(),{n,k,0x213000+serial,0x313000+serial,100+serial,static_cast<unsigned>(serial%5),static_cast<unsigned>((serial+1)%5),static_cast<unsigned>(serial%5)});++serial;}}return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
