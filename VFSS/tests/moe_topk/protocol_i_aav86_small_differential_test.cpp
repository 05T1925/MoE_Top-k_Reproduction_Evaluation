#include <moe_topk/protocol_i_aav86_small.h>
#include <moe_topk/topk_oracle.h>
#include "protocol_i_aav86_e9_fixtures.h"

#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <random>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {
using namespace moe_topk;
void require(bool ok, const char* what) {
  if (!ok) throw std::runtime_error(what);
}
template <typename F> void rejects(F&& f, const char* what) {
  bool rejected = false;
  try { f(); } catch (const std::exception&) { rejected = true; }
  require(rejected, what);
}
std::array<int,2> pair() {
  std::array<int,2> p{-1,-1};
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, p.data()) == 0, "socketpair");
  return p;
}
std::string claim_directory(std::uint64_t serial, int party) {
  std::string pattern = "/tmp/m6a7-diff-" + std::to_string(::getpid()) + "-" +
                        std::to_string(serial) + "-p" + std::to_string(party) + "-XXXXXX";
  std::vector<char> storage(pattern.begin(),pattern.end());
  storage.push_back(0);
  require(::mkdtemp(storage.data()) != nullptr, "mkdtemp");
  return storage.data();
}
struct Result {
  ProtocolIAav86SmallMetrics p0, p1;
};
void timeout_and_abort() {
  auto c=ProtocolIAav86SmallConfig{0xa7ff01,0xb7ff01,0xc7ff01,1,1,1,0,100,
                                   claim_directory(99001,0)};
  auto materials=protocol_i_aav86_small_dealer_generate(c);
  const auto s0=pair(),s1=pair(),inverse=pair();
  std::vector<std::array<int,2>> core(3);
  for(auto& channel:core) channel=pair();
  std::vector<int> own;
  for(const auto& channel:core) own.push_back(channel[0]);
  const auto start=std::chrono::steady_clock::now();
  rejects([&] { (void)protocol_i_aav86_small_party(c,std::move(materials.party0),{0},
      {s0[0],s1[0]},own,inverse[0]); },"silent peer timeout accepted");
  const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now()-start).count();
  require(elapsed>=50&&elapsed<2000,"timeout was not bounded");
  for(const auto& channel:core) { ::close(channel[0]); ::close(channel[1]); }
  ::close(s0[1]); ::close(s1[0]); ::close(s1[1]);
  ::close(inverse[0]); ::close(inverse[1]);
  std::filesystem::remove_all(c.durable_claim_directory);

  ++c.session; ++c.fingerprint; ++c.material_id;
  c.durable_claim_directory=claim_directory(99002,0);
  auto abort_material=protocol_i_aav86_small_dealer_generate(c);
  const auto closed=pair(),other=pair(),reverse=pair();
  std::vector<std::array<int,2>> abort_core(3);
  for(auto& channel:abort_core) channel=pair();
  ::close(closed[1]);
  std::vector<int> abort_own;
  for(const auto& channel:abort_core) abort_own.push_back(channel[0]);
  const auto child=::fork(); require(child>=0,"abort fork");
  if(child==0) {
    ::alarm(3);
    try { (void)protocol_i_aav86_small_party(c,std::move(abort_material.party0),{0},
          {closed[0],other[0]},abort_own,reverse[0]); _exit(1); }
    catch(const std::exception&) { _exit(0); }
  }
  int status=0;
  require(::waitpid(child,&status,0)==child,"abort waitpid");
  require((WIFEXITED(status)&&WEXITSTATUS(status)==0)||
          (WIFSIGNALED(status)&&WTERMSIG(status)==SIGPIPE),
          "peer abort did not fail closed within bound");
  ::close(closed[0]); ::close(other[0]); ::close(other[1]);
  ::close(reverse[0]); ::close(reverse[1]);
  for(const auto& channel:abort_core) { ::close(channel[0]); ::close(channel[1]); }
  std::filesystem::remove_all(c.durable_claim_directory);
  std::cout<<"E8_FAILURE_CASE timeout_ms="<<elapsed<<" peer_abort=bounded\n";
}
Result run_case(const std::vector<std::uint32_t>& scores, std::uint32_t k,
                std::uint32_t r, std::uint64_t serial) {
  const auto n = static_cast<std::uint32_t>(scores.size());
  auto c0 = ProtocolIAav86SmallConfig{0xa70000+serial,0xb70000+serial,0xc70000+serial,
                                     n,k,r,0,10000,claim_directory(serial,0)};
  auto c1 = c0; c1.party = 1; c1.durable_claim_directory = claim_directory(serial,1);
  auto materials = protocol_i_aav86_small_dealer_generate(c0);
  const auto bytes0 = protocol_i_aav86_small_serialize_party_material(materials.party0);
  const auto bytes1 = protocol_i_aav86_small_serialize_party_material(materials.party1);
  auto m0 = protocol_i_aav86_small_deserialize_party_material(bytes0,0,c0);
  auto m1 = protocol_i_aav86_small_deserialize_party_material(bytes1,1,c1);
  std::mt19937_64 rng(serial * 1024U + 9U);
  std::vector<std::uint32_t> x0(n),x1(n);
  for (std::size_t j = 0; j < n; ++j) {
    x0[j] = static_cast<std::uint32_t>(rng());
    x1[j] = scores[j] - x0[j];
  }
  std::array<std::array<int,2>,2> score{pair(),pair()};
  std::vector<std::array<int,2>> core(2U*r+1U);
  for (auto& item : core) item = pair();
  const auto inverse = pair();
  std::vector<int> core0,core1;
  for (const auto& item : core) { core0.push_back(item[0]); core1.push_back(item[1]); }
  ProtocolIAav86SmallOutput out0,out1;
  std::exception_ptr error0,error1;
  std::thread t0([&] {
    try {
      out0=protocol_i_aav86_small_party(c0,std::move(m0),x0,
                                        {score[0][0],score[1][0]},core0,inverse[0]);
    } catch (...) { error0=std::current_exception(); }
  });
  std::thread t1([&] {
    try {
      out1=protocol_i_aav86_small_party(c1,std::move(m1),x1,
                                        {score[0][1],score[1][1]},core1,inverse[1]);
    } catch (...) { error1=std::current_exception(); }
  });
  t0.join(); t1.join();
  if (error0) std::rethrow_exception(error0);
  if (error1) std::rethrow_exception(error1);
  require(out0.xor_mask_share.size()==n && out1.xor_mask_share.size()==n,"mask shape");
  const auto want=top_k_mask(scores,k);
  for (std::size_t j=0;j<n;++j)
    require((out0.xor_mask_share[j]^out1.xor_mask_share[j])==want[j],"oracle differential");
  const auto d=materials.party0.padded_n;
  const auto slots=static_cast<std::uint64_t>(r)*d*(d-1U)/2U;
  for (const auto* m:{&out0.metrics,&out1.metrics}) {
    require(m->pool_slots_per_party==slots && m->active_edges<=slots &&
                m->score_dcf_evaluations==4U*d &&
                m->ca_dcf_evaluations==2U*m->active_edges &&
                m->dcf_evaluations==m->score_dcf_evaluations+m->ca_dcf_evaluations &&
                m->active_edges_by_iteration.size()==r &&
                m->causal_rounds==2U*r+4U &&
                m->online_sent_bytes==m->score_sent_bytes+m->core_sent_bytes+m->inverse_sent_bytes,
            "metrics accounting");
    require(m->message_trace.size()==2U*r+4U &&
            m->edge_trace.size()==m->active_edges,"trace shape");
    std::uint64_t sent=0,received=0;
    for(std::size_t i=0;i<m->message_trace.size();++i) {
      const auto expected=i==0?4U:i==1?5U:i==m->message_trace.size()-1?40U:
          i==2?9U:static_cast<unsigned>(7U+i);
      require(m->message_trace[i].phase==expected,"message phase order");
      sent+=m->message_trace[i].sent_bytes;
      received+=m->message_trace[i].received_bytes;
    }
    require(sent==m->online_sent_bytes&&received==m->online_received_bytes,
            "message byte trace");
    std::vector<std::uint64_t> per_round(r);
    std::uint64_t prior=0;
    bool first=true;
    for(const auto& e:m->edge_trace) {
      require(e.iteration<r&&e.a<e.c&&e.c<d&&
              e.material_id==c0.material_id+
                  static_cast<std::uint64_t>(e.iteration)*d*(d-1U)/2U+
                  static_cast<std::uint64_t>(e.a)*(2U*d-e.a-1U)/2U+(e.c-e.a-1U),
              "edge slot/id trace");
      require(first||e.material_id>prior,"edge lookup order/reuse");
      first=false; prior=e.material_id; per_round[e.iteration]++;
    }
    require(per_round==m->active_edges_by_iteration,"edge round trace");
  }
  require(out0.metrics.active_edges==out1.metrics.active_edges,"edge count symmetry");
  require(out0.metrics.edge_trace.size()==out1.metrics.edge_trace.size(),"edge graph symmetry");
  for(std::size_t i=0;i<out0.metrics.edge_trace.size();++i)
    require(out0.metrics.edge_trace[i].material_id==out1.metrics.edge_trace[i].material_id,
            "edge graph identity");
  std::cout<<"E7_DIFF_CASE serial="<<serial<<" n="<<n<<" k="<<k<<" r="<<r
           <<" d="<<d<<" reserved="<<slots<<" active="<<out0.metrics.active_edges
           <<" score_eval_per_party="<<out0.metrics.score_dcf_evaluations
           <<" ca_eval_per_party="<<out0.metrics.ca_dcf_evaluations
           <<" total_eval_per_party="<<out0.metrics.dcf_evaluations
           <<" p0_bytes="<<out0.metrics.online_sent_bytes
           <<" p1_bytes="<<out1.metrics.online_sent_bytes<<" edges=";
  for(const auto& edge:out0.metrics.edge_trace)
    std::cout<<edge.iteration<<":"<<edge.a<<":"<<edge.c<<":"<<edge.material_id<<",";
  std::cout<<" phases=";
  for(const auto& phase:out0.metrics.message_trace)
    std::cout<<static_cast<unsigned>(phase.phase)<<":"<<phase.sent_bytes<<":"
             <<phase.received_bytes<<",";
  std::cout<<"\n";
  auto replay=protocol_i_aav86_small_deserialize_party_material(bytes0,0,c0);
  std::vector<int> no_channels(2U*r+1U,0);
  rejects([&] {
    (void)protocol_i_aav86_small_party(c0,std::move(replay),x0,{0,0},no_channels,0);
  },"durable replay accepted");
  const auto restarted=::fork();
  require(restarted>=0,"replay fork");
  if(restarted==0) {
    try {
      auto old=protocol_i_aav86_small_deserialize_party_material(bytes0,0,c0);
      (void)protocol_i_aav86_small_party(c0,std::move(old),x0,{0,0},no_channels,0);
      _exit(1);
    } catch(const std::exception&) { _exit(0); }
  }
  int replay_status=0;
  require(::waitpid(restarted,&replay_status,0)==restarted&&WIFEXITED(replay_status)&&
          WEXITSTATUS(replay_status)==0,"restart replay accepted");
  std::filesystem::remove_all(c0.durable_claim_directory);
  std::filesystem::remove_all(c1.durable_claim_directory);
  return {out0.metrics,out1.metrics};
}
}
int main() {
  try {
    timeout_and_abort();
    std::uint64_t cases=0, active=0, reserved=0, sent0=0, sent1=0, score_eval=0;
    if (const auto* tier=std::getenv("MOE_TOPK_M6A_E9_D")) {
      const auto d=static_cast<std::uint32_t>(std::stoul(tier));
      for(const auto& fixture:moe_topk_e9_test::fixtures(d)) {
        const auto result=run_case(fixture.scores,fixture.k,fixture.r,++cases);
        active+=result.p0.active_edges; reserved+=result.p0.pool_slots_per_party;
        score_eval+=result.p0.score_dcf_evaluations;
        sent0+=result.p0.online_sent_bytes; sent1+=result.p1.online_sent_bytes;
      }
      std::cout<<"E9_DIFFERENTIAL_PASS d="<<d<<" cases="<<cases
               <<" reserved_per_party_sum="<<reserved<<" active_edges_sum="<<active
               <<" score_eval_per_party_sum="<<score_eval
               <<" ca_eval_per_party_sum="<<(2U*active)
               <<" p0_sent_bytes_sum="<<sent0<<" p1_sent_bytes_sum="<<sent1<<"\n";
      return 0;
    }
    const std::vector<std::vector<std::uint32_t>> fixtures{
      {UINT32_C(0x80000000)}, {5,5,5}, {UINT32_C(0x80000000),UINT32_C(0x7fffffff),7,7,0},
      {0,0,0,0,0,0,0,0}, {8,7,6,5,4,3,2,1}};
    for (const auto& scores:fixtures) for (const auto k:{1U,static_cast<unsigned>(scores.size())})
      for (const auto r:{1U,2U,3U,4U,5U}) {
        const auto result=run_case(scores,k,r,++cases);
        active+=result.p0.active_edges; reserved+=result.p0.pool_slots_per_party;
        score_eval+=result.p0.score_dcf_evaluations;
        sent0+=result.p0.online_sent_bytes; sent1+=result.p1.online_sent_bytes;
      }
    for (std::uint32_t n=2;n<=8;++n) {
      std::mt19937_64 rng(0x700000+n);
      std::vector<std::uint32_t> scores(n);
      for (auto& word:scores) word=static_cast<std::uint32_t>(rng());
      const auto result=run_case(scores,(n+1U)/2U,2U,++cases);
      active+=result.p0.active_edges; reserved+=result.p0.pool_slots_per_party;
      score_eval+=result.p0.score_dcf_evaluations;
      sent0+=result.p0.online_sent_bytes; sent1+=result.p1.online_sent_bytes;
    }
    std::cout<<"E7_DIFFERENTIAL_PASS cases="<<cases<<" reserved_per_party_sum="<<reserved
             <<" active_edges_sum="<<active<<" score_eval_per_party_sum="<<score_eval
             <<" ca_eval_per_party_sum="<<(2U*active)
             <<" total_eval_per_party_sum="<<(score_eval+2U*active)
             <<" p0_sent_bytes_sum="<<sent0<<" p1_sent_bytes_sum="<<sent1<<"\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr<<"E7_DIFFERENTIAL_FAIL "<<e.what()<<"\n";
    return 1;
  }
}
