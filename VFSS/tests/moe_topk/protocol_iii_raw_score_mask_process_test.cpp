// TEST_ONLY independent P2/P0/P1 process harness for M5-FIX-F1.
// The parent controller holds the clear oracle; it sends only each party's
// own additive Q20.12 shares after the offline dealer has exited.
#include <moe_topk/protocol_i_ucmp.h>
#include <moe_topk/protocol_iii_raw_score_mask.h>
#include <moe_topk/protocol_iii_raw_score_mask_package.h>
#include <moe_topk/topk_oracle.h>

#include <FSS/dpf.h>
#include <FSS/prng.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <poll.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
using namespace moe_topk;
using Bytes = std::vector<std::uint8_t>;
constexpr int kTimeoutMs = 30000;
constexpr std::size_t kMaxPacket = 64U * 1024U * 1024U;

void require(bool ok, const char* msg) {
  if (!ok) throw std::runtime_error(msg);
}
std::uint8_t width(std::uint32_t x) {
  std::uint8_t result = 0;
  while (x != 0U) { ++result; x >>= 1U; }
  return result;
}
std::uint32_t pad(std::uint32_t n) {
  std::uint32_t result = 2;
  while (result < n) result <<= 1U;
  return result;
}
void put(Bytes& b, std::uint64_t x) {
  for (int shift = 56; shift >= 0; shift -= 8)
    b.push_back(static_cast<std::uint8_t>(x >> shift));
}
std::uint64_t get(const Bytes& b, std::size_t& at) {
  require(at <= b.size() && b.size() - at >= 8U, "report truncated");
  std::uint64_t x = 0;
  for (int i = 0; i < 8; ++i) x = (x << 8U) | b[at++];
  return x;
}
void wait_ready(int fd, short events) {
  pollfd p{fd, events, 0};
  int result;
  do { result = ::poll(&p, 1, kTimeoutMs); } while (result < 0 && errno == EINTR);
  require(result > 0 && (p.revents & events), "F1 IPC timeout/closed");
}
void write_all(int fd, const std::uint8_t* data, std::size_t n) {
  while (n != 0U) {
    wait_ready(fd, POLLOUT);
    const auto count = ::send(fd, data, n, MSG_NOSIGNAL);
    if (count < 0 && errno == EINTR) continue;
    require(count > 0, "F1 IPC send");
    data += count; n -= static_cast<std::size_t>(count);
  }
}
void read_all(int fd, std::uint8_t* data, std::size_t n) {
  while (n != 0U) {
    wait_ready(fd, POLLIN);
    const auto count = ::recv(fd, data, n, 0);
    if (count < 0 && errno == EINTR) continue;
    require(count > 0, "F1 IPC receive");
    data += count; n -= static_cast<std::size_t>(count);
  }
}
void send_packet(int fd, const Bytes& b) {
  require(b.size() <= kMaxPacket, "F1 IPC packet too large");
  Bytes header; put(header, b.size());
  write_all(fd, header.data(), header.size());
  if (!b.empty()) write_all(fd, b.data(), b.size());
}
Bytes receive_packet(int fd) {
  Bytes header(8U); read_all(fd, header.data(), header.size());
  std::size_t offset = 0;
  const auto n = get(header, offset);
  require(n <= kMaxPacket, "F1 IPC packet length");
  Bytes b(static_cast<std::size_t>(n));
  if (!b.empty()) read_all(fd, b.data(), b.size());
  return b;
}
struct TestCase {
  std::vector<std::uint32_t> scores;
  std::uint32_t k;
  std::uint64_t seed;
};
ProtocolIIIRawScoreMaskConfig config(std::uint32_t n, std::uint32_t k,
                                     std::uint8_t party,std::uint64_t session,
                                     std::uint64_t fingerprint,
                                     std::uint64_t material_id) {
  ProtocolIIIRawScoreMaskConfig c;
  c.material_id = material_id;
  const auto padded = pad(n);
  const auto index = width(padded - 1U);
  const auto comparison = static_cast<std::uint8_t>(33U + index);
  const auto rank = width(n - 1U);
  c.score_input = {session,fingerprint,n,padded,k,index,comparison,party,kTimeoutMs};
  c.grank = {c.score_input.session,c.score_input.fingerprint,
             n,padded,k,comparison,rank,party,kTimeoutMs};
  c.routing = {c.score_input.session,c.score_input.fingerprint,
               n,k,rank,comparison,party,kTimeoutMs};
  return c;
}
void seed_fss(osuCrypto::PRNG& private_random) {
  for (int i = 0; i < 256; ++i)
    FSSConfig::prngs[i].SetSeed(
        osuCrypto::toBlock(private_random(),private_random()));
}
std::pair<ProtocolIIIRawScoreMaskMaterial,ProtocolIIIRawScoreMaskMaterial>
make_material(const ProtocolIIIRawScoreMaskConfig& c, osuCrypto::PRNG& random) {
  const auto score_ring = (UINT64_C(1) << 34U) - 1U;
  const auto key_ring = (UINT64_C(1) << c.grank.comparison_bits) - 1U;
  const auto rank_ring = (UINT64_C(1) << c.routing.rank_bits) - 1U;
  ProtocolIIIRawScoreMaskMaterial a,b;
  a.material_id = b.material_id = c.material_id;
  for (std::uint8_t party = 0; party < 2; ++party) {
    auto& m = party == 0 ? a : b;
    auto& s = m.score_input_package;
    s.session = c.score_input.session; s.fingerprint = c.score_input.fingerprint;
    s.party = party; s.n = c.score_input.padded_n;
    s.k = c.score_input.k; s.comparison_bits = c.score_input.comparison_bits;
    auto& g = m.grank_package;
    g.session = c.grank.session; g.fingerprint = c.grank.fingerprint;
    g.party = party; g.n = c.grank.logical_n;
    g.k = c.grank.k; g.comparison_bits = c.grank.comparison_bits;
    auto& r = m.routing_material;
    r.session = c.routing.session; r.fingerprint = c.routing.fingerprint;
    r.party = party; r.logical_n = c.routing.logical_n;
    r.k = c.routing.k; r.rank_bits = c.routing.rank_bits;
    r.rank_mask_shares.resize(r.logical_n);
    r.dpf_keys.reserve(r.logical_n);
    g.node_mask_shares.resize(g.n);
  }
  for (const auto stage : {UINT8_C(1),UINT8_C(2)}) {
    for (std::uint32_t slot = 0; slot < c.score_input.padded_n; ++slot) {
      const auto full_left = random() & score_ring;
      const auto full_right = random() & score_ring;
      const auto left0 = random() & score_ring;
      const auto right0 = random() & score_ring;
      ProtocolIUcmpMaterial item(34, full_left, full_right);
      ProtocolIScoreInputPartyMaterial x(slot,stage,left0,right0,
                                           item.export_party_material(0));
      ProtocolIScoreInputPartyMaterial y(slot,stage,
          (full_left-left0)&score_ring,(full_right-right0)&score_ring,
          item.export_party_material(1));
      auto& xs = stage == 1U ? a.score_input_package.carry_materials :
                                a.score_input_package.sign_materials;
      auto& ys = stage == 1U ? b.score_input_package.carry_materials :
                                b.score_input_package.sign_materials;
      xs.push_back(std::move(x)); ys.push_back(std::move(y));
    }
  }
  std::vector<std::uint64_t> full_node(c.grank.logical_n);
  for (std::uint32_t i = 0; i < c.grank.logical_n; ++i) {
    full_node[i] = random() & key_ring;
    a.grank_package.node_mask_shares[i] = random() & key_ring;
    b.grank_package.node_mask_shares[i] =
        (full_node[i]-a.grank_package.node_mask_shares[i])&key_ring;
  }
  for (std::uint32_t i = 0; i < c.grank.logical_n; ++i) {
    for (std::uint32_t j = i+1; j < c.grank.logical_n; ++j) {
      ProtocolIUcmpMaterial item(c.grank.comparison_bits,full_node[i],full_node[j]);
      a.grank_package.edge_materials.emplace_back(i,j,item.export_party_material(0));
      b.grank_package.edge_materials.emplace_back(i,j,item.export_party_material(1));
    }
  }
  for (std::uint32_t i = 0; i < c.routing.logical_n; ++i) {
    const auto full = random() & rank_ring;
    a.routing_material.rank_mask_shares[i] = random() & rank_ring;
    b.routing_material.rank_mask_shares[i] =
        (full-a.routing_material.rank_mask_shares[i])&rank_ring;
    auto keys = keyGenDPF(c.routing.rank_bits,64,full,1);
    a.routing_material.dpf_keys.emplace_back(std::move(keys.first));
    b.routing_material.dpf_keys.emplace_back(std::move(keys.second));
  }
  return {std::move(a),std::move(b)};
}
std::vector<std::uint32_t> decode_shares(const Bytes& b, std::uint32_t n) {
  require(b.size() == static_cast<std::size_t>(n)*4U, "F1 input length");
  std::vector<std::uint32_t> out(n);
  for (std::size_t i = 0; i < n; ++i) {
    out[i] = (static_cast<std::uint32_t>(b[4*i])<<24U) |
             (static_cast<std::uint32_t>(b[4*i+1])<<16U) |
             (static_cast<std::uint32_t>(b[4*i+2])<<8U) | b[4*i+3];
  }
  return out;
}
Bytes encode_shares(const std::vector<std::uint32_t>& x) {
  Bytes out; out.reserve(x.size()*4U);
  for (const auto v : x) for (int shift=24;shift>=0;shift-=8)
    out.push_back(static_cast<std::uint8_t>(v>>shift));
  return out;
}
std::uint32_t get_u32_at(const Bytes& b,std::size_t at) {
  require(at+4U<=b.size(),"F1 offline header length");
  return (static_cast<std::uint32_t>(b[at])<<24U) |
         (static_cast<std::uint32_t>(b[at+1])<<16U) |
         (static_cast<std::uint32_t>(b[at+2])<<8U) | b[at+3];
}
Bytes encode_report(const ProtocolIIIRawScoreMaskOutput& out,
                    const Bytes& offline) {
  const auto score=get_u32_at(offline,46U);
  const auto grank=get_u32_at(offline,50U);
  const auto dpf=get_u32_at(offline,54U);
  const auto routing=static_cast<std::uint64_t>(out.xor_mask_shares.size())*8U+dpf;
  const auto metadata=UINT64_C(58)+8U;  // bundle header + IPC length prefix
  require(metadata+score+grank+routing==offline.size()+8U,
          "F1 offline stage accounting");
  Bytes b(out.xor_mask_shares.begin(),out.xor_mask_shares.end());
  for (const auto value : {static_cast<std::uint64_t>(offline.size()+8U),
                           out.metrics.sent_bytes,out.metrics.received_bytes,
                           out.metrics.total_logical_bits,out.metrics.input_logical_bits,
                           out.metrics.ranking_logical_bits,out.metrics.routing_logical_bits,
                           out.metrics.score_input.raw_dcf_calls,
                           out.metrics.grank.raw_dcf_calls,
                           out.metrics.routing.eval_calls,
                           out.metrics.total_online_rounds,
                           out.metrics.score_input.carry_sent_bytes,
                           out.metrics.score_input.sign_sent_bytes,
                           out.metrics.grank.sent_bytes,
                           out.metrics.routing.sent_bytes,
                           static_cast<std::uint64_t>(score),
                           static_cast<std::uint64_t>(grank),routing,metadata}) put(b,value);
  return b;
}
struct Report {
  Bytes mask;
  std::array<std::uint64_t,19> metrics{};
};
Report decode_report(const Bytes& b, std::uint32_t n) {
  require(b.size() == n+19U*8U, "F1 report length");
  Report r; r.mask.assign(b.begin(),b.begin()+n);
  std::size_t at=n;
  for (auto& v:r.metrics) v=get(b,at);
  return r;
}
int dealer(int fd0,int fd1,std::uint32_t n,std::uint32_t k,
           std::uint64_t session,std::uint64_t fingerprint,
           std::uint64_t material_id) {
  std::array<std::uint64_t,2> private_seed{};
  require(::getrandom(private_seed.data(),sizeof(private_seed),0)==
              sizeof(private_seed),"F1 dealer 128-bit OS random seed");
  osuCrypto::PRNG private_random;
  private_random.SetSeed(osuCrypto::toBlock(private_seed[0],private_seed[1]));
  seed_fss(private_random);
  const auto c0=config(n,k,0,session,fingerprint,material_id);
  const auto c1=config(n,k,1,session,fingerprint,material_id);
  auto materials=make_material(c0,private_random);
  send_packet(fd0,protocol_iii_raw_score_mask_serialize_bundle(c0,materials.first));
  send_packet(fd1,protocol_iii_raw_score_mask_serialize_bundle(c1,materials.second));
  return 0;
}
int party(std::uint8_t id,int offline_fd,int input_fd,int carry_fd,int sign_fd,
          int grank_fd,int routing_fd,int report_fd,std::uint32_t n,std::uint32_t k,
          std::uint64_t session,std::uint64_t fingerprint,
          std::uint64_t material_id,bool force_close) {
  const auto c=config(n,k,id,session,fingerprint,material_id);
  const auto bytes=receive_packet(offline_fd);
  auto material=protocol_iii_raw_score_mask_deserialize_bundle(c,bytes);
  const auto raw=decode_shares(receive_packet(input_fd),n);
  if (force_close) { ::close(carry_fd); throw std::runtime_error("injected early close"); }
  ProtocolIIIRawScoreMaskFds fds{{carry_fd,sign_fd},grank_fd,routing_fd};
  const auto output=protocol_iii_raw_score_mask_party(c,material,raw,fds);
  send_packet(report_fd,encode_report(output,bytes));
  // TEST_ONLY lifetime barrier on the report socket. It conveys no protocol
  // value and is outside all online metrics; both parties have finished.
  std::uint8_t done=0; read_all(report_fd,&done,1U);
  require(done==1U,"F1 controller completion");
  return 0;
}
int pair_fds(std::array<int,2>& p) {
  return ::socketpair(AF_UNIX,SOCK_STREAM,0,p.data());
}
void close_except(const std::vector<int>& keep) {
  for (int fd=3;fd<128;++fd)
    if (std::find(keep.begin(),keep.end(),fd)==keep.end()) ::close(fd);
}
pid_t launch(const std::vector<std::string>& arguments,const std::vector<int>& keep) {
  const auto pid=::fork();
  require(pid>=0,"F1 fork");
  if (pid==0) {
    close_except(keep);
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>("/proc/self/exe"));
    for (const auto& arg:arguments) argv.push_back(const_cast<char*>(arg.c_str()));
    argv.push_back(nullptr);
    ::execv("/proc/self/exe",argv.data());
    _exit(127);
  }
  return pid;
}
std::string number(std::uint64_t x) { return std::to_string(x); }
void wait_success(pid_t pid,const char* role) {
  int status=0;
  require(::waitpid(pid,&status,0)==pid,"F1 waitpid");
  require(WIFEXITED(status)&&WEXITSTATUS(status)==0,role);
}
void run_case(const TestCase& test,bool inject_early_close=false) {
  const auto n=static_cast<std::uint32_t>(test.scores.size());
  std::array<std::array<int,2>,2> offline{},input{},report{};
  std::array<std::array<int,2>,4> online{};
  for (auto& p:offline) require(pair_fds(p)==0,"F1 offline socket");
  for (auto& p:input) require(pair_fds(p)==0,"F1 input socket");
  for (auto& p:report) require(pair_fds(p)==0,"F1 report socket");
  for (auto& p:online) require(pair_fds(p)==0,"F1 online socket");
  // Public bindings are sampled independently of the controller's private
  // deterministic input-sharing seed. A party cannot infer the peer share
  // by recovering that seed from session or material metadata.
  std::array<std::uint64_t,3> public_ids{};
  require(::getrandom(public_ids.data(),sizeof(public_ids),0)==sizeof(public_ids),
          "F1 public binding random IDs");
  for (const auto id:public_ids) require(id!=0U,"F1 public binding nonzero");
  const auto session=public_ids[0];
  const auto fingerprint=public_ids[1];
  const auto material_id=public_ids[2];
  const auto dealer_pid=launch({"--dealer",number(offline[0][1]),number(offline[1][1]),
                                 number(n),number(test.k),number(session),
                                 number(fingerprint),number(material_id)},
                                {offline[0][1],offline[1][1]});
  const auto launch_party=[&](std::uint8_t id) {
    std::vector<int> keep{offline[id][0],input[id][1],report[id][1]};
    for (const auto& p:online) keep.push_back(p[id]);
    return launch({inject_early_close && id==1U ? "--party-close" : "--party",
                   number(id),number(offline[id][0]),number(input[id][1]),
                   number(online[0][id]),number(online[1][id]),
                   number(online[2][id]),number(online[3][id]),
                   number(report[id][1]),number(n),number(test.k),
                   number(session),number(fingerprint),number(material_id)},keep);
  };
  const auto p0=launch_party(0),p1=launch_party(1);
  for (auto& p:online) { ::close(p[0]); ::close(p[1]); }
  for (auto& p:offline) { ::close(p[0]); ::close(p[1]); }
  for (auto& p:input) ::close(p[1]);
  for (auto& p:report) ::close(p[1]);
  wait_success(dealer_pid,"F1 dealer exited unsuccessfully");
  // Dealer has exited before controller releases any online input share.
  std::mt19937_64 random(test.seed^UINT64_C(0x1A2B3C));
  std::vector<std::uint32_t> s0(n),s1(n);
  for (std::size_t i=0;i<n;++i) {
    s0[i]=static_cast<std::uint32_t>(random());
    s1[i]=test.scores[i]-s0[i];
  }
  send_packet(input[0][0],encode_shares(s0));
  send_packet(input[1][0],encode_shares(s1));
  if (inject_early_close) {
    int status0=0,status1=0;
    require(::waitpid(p0,&status0,0)==p0 && ::waitpid(p1,&status1,0)==p1,
            "F1 early-close waitpid");
    require(WIFEXITED(status0) && WEXITSTATUS(status0)!=0 &&
            WIFEXITED(status1) && WEXITSTATUS(status1)!=0,
            "F1 early close did not fail both parties");
    std::cout<<"F1_FAILURE_SMOKE_PASS early_close no_valid_mask=1"<<'\n';
    for (auto& p:input) ::close(p[0]);
    for (auto& p:report) ::close(p[0]);
    return;
  }
  const auto a=decode_report(receive_packet(report[0][0]),n);
  const auto b=decode_report(receive_packet(report[1][0]),n);
  const auto expected=top_k_mask(test.scores,test.k);
  std::size_t selected=0;
  for (std::size_t i=0;i<n;++i) {
    require(a.mask[i]<=1U&&b.mask[i]<=1U,"F1 process mask shares");
    const auto v=static_cast<std::uint8_t>(a.mask[i]^b.mask[i]);
    require(v==expected[i],"F1 process oracle");
    selected+=v;
  }
  require(selected==test.k,"F1 process K count");
  require(a.metrics[1]==b.metrics[2]&&b.metrics[1]==a.metrics[2],
          "F1 process wire accounting");
  require(a.metrics[10]==4U&&b.metrics[10]==4U,
          "F1 process causal round count");
  for (const auto* r : {&a,&b}) {
    require(r->metrics[11]+r->metrics[12]+r->metrics[13]+r->metrics[14]==
                r->metrics[1], "F1 stage wire sum");
    require(r->metrics[15]+r->metrics[16]+r->metrics[17]+r->metrics[18]==
                r->metrics[0], "F1 stage offline sum");
  }
  const std::uint8_t done=1U;
  write_all(report[0][0],&done,1U); write_all(report[1][0],&done,1U);
  wait_success(p0,"F1 P0 exited unsuccessfully");
  wait_success(p1,"F1 P1 exited unsuccessfully");
  const auto wire=a.metrics[1]+b.metrics[1];
  const auto offline_bytes=a.metrics[0]+b.metrics[0];
  std::cout<<"F1_PROCESS_PASS n="<<n<<" k="<<test.k
           <<" rounds=4 logical_bits="<<a.metrics[3]
           <<" wire_bytes="<<wire<<" offline_bytes="<<offline_bytes
           <<" raw_dcf="<<(a.metrics[7]+b.metrics[7]+a.metrics[8]+b.metrics[8])
           <<" dpf_eval="<<(a.metrics[9]+b.metrics[9])
           <<" stage_wire="<<(a.metrics[11]+b.metrics[11])<<","
           <<(a.metrics[12]+b.metrics[12])<<","
           <<(a.metrics[13]+b.metrics[13])<<","
           <<(a.metrics[14]+b.metrics[14])
           <<" stage_offline="<<(a.metrics[15]+b.metrics[15])<<","
           <<(a.metrics[16]+b.metrics[16])<<","
           <<(a.metrics[17]+b.metrics[17])<<","
           <<(a.metrics[18]+b.metrics[18])<<'\n';
  for (auto& p:input) ::close(p[0]);
  for (auto& p:report) ::close(p[0]);
}
int parse_i(const char* s) { return std::stoi(s); }
std::uint64_t parse_u(const char* s) { return std::stoull(s); }
}  // namespace

int main(int argc,char** argv) {
  try {
    if (argc==9 && std::string(argv[1])=="--dealer")
      return dealer(parse_i(argv[2]),parse_i(argv[3]),parse_u(argv[4]),
                    parse_u(argv[5]),parse_u(argv[6]),parse_u(argv[7]),
                    parse_u(argv[8]));
    if (argc==15 && (std::string(argv[1])=="--party" ||
                     std::string(argv[1])=="--party-close"))
      return party(parse_u(argv[2]),parse_i(argv[3]),parse_i(argv[4]),
                   parse_i(argv[5]),parse_i(argv[6]),parse_i(argv[7]),
                   parse_i(argv[8]),parse_i(argv[9]),parse_u(argv[10]),
                   parse_u(argv[11]),parse_u(argv[12]),parse_u(argv[13]),
                   parse_u(argv[14]),std::string(argv[1])=="--party-close");
    require(argc==1 || (argc==2 && (std::string(argv[1])=="--cost-smoke" ||
                                  std::string(argv[1])=="--failure-smoke")),
            "F1 command arguments");
    if (argc==2 && std::string(argv[1])=="--failure-smoke") {
      run_case({{7U,7U,7U},1U,201U},true);
      return 0;
    }
    const bool cost=argc==2;
    std::vector<TestCase> cases{
      {{7U,7U,7U},1U,101U},
      {{UINT32_C(0x80000000),0U,UINT32_C(0x7fffffff),
        UINT32_C(0xfffff001),UINT32_C(0x00001001)},2U,102U},
      {{9U,8U,7U,6U,5U,4U,3U,2U},8U,103U},
    };
    if (cost) {
      cases.push_back({{0U,UINT32_MAX},1U,104U});
      std::vector<std::uint32_t> v16(16),v20(20);
      for (std::uint32_t i=0;i<16;++i) v16[i]=(16U-i)*4096U;
      for (std::uint32_t i=0;i<20;++i) v20[i]=(20U-i)*4096U;
      cases.push_back({v16,8U,105U});
      cases.push_back({v20,10U,106U});
    }
    for (const auto& item:cases) run_case(item);
    return 0;
  } catch (const std::exception& e) {
    std::cerr<<"F1 process failure: "<<e.what()<<'\n';
    return 1;
  }
}
