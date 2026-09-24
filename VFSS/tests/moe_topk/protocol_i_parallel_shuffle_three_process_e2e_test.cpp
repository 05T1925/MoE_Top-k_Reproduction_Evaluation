// TEST_ONLY: independent P2/P0/P1 process harness for the three-round
// correlated parallel-shuffle C-INSTANTIATION.
#include <moe_topk/protocol_i_parallel_shuffle.h>
#include <moe_topk/protocol_i_priority_key.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <poll.h>
#include <random>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {
using namespace moe_topk;
using Bytes = std::vector<std::uint8_t>;
constexpr int kTimeoutMs = 10000;
constexpr std::size_t kMaxBytes = 512U * 1024U * 1024U;

struct CaseConfig {
  std::uint32_t n = 0, k = 0;
  std::uint8_t bits = 0;
  std::uint64_t session = 0, fingerprint = 0, material_id = 0;
};
struct DealerStats {
  std::uint64_t package0_bytes = 0, package1_bytes = 0;
  ProtocolIParallelShuffleDealerMetrics metrics;
  ProtocolIPermutation pi;
  std::vector<ProtocolIBlock192> full_r;
};
struct PartyResult {
  ProtocolIParallelShuffleCoreOutput core;
  ProtocolIParallelShuffleMetrics metrics;
};
struct CommunicationSample {
  ProtocolIParallelShuffleMetrics party0;
  ProtocolIParallelShuffleMetrics party1;
};

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}
void put_u32(Bytes& output, std::uint32_t value) {
  for (int shift = 24; shift >= 0; shift -= 8)
    output.push_back(static_cast<std::uint8_t>(value >> shift));
}
void put_u64(Bytes& output, std::uint64_t value) {
  for (int shift = 56; shift >= 0; shift -= 8)
    output.push_back(static_cast<std::uint8_t>(value >> shift));
}
std::uint32_t get_u32(const Bytes& input, std::size_t& offset, const char* message) {
  require(offset <= input.size() && input.size() - offset >= 4, message);
  std::uint32_t value = 0;
  for (int i = 0; i < 4; ++i) value = (value << 8U) | input[offset++];
  return value;
}
std::uint64_t get_u64(const Bytes& input, std::size_t& offset, const char* message) {
  require(offset <= input.size() && input.size() - offset >= 8, message);
  std::uint64_t value = 0;
  for (int i = 0; i < 8; ++i) value = (value << 8U) | input[offset++];
  return value;
}
void put_record(Bytes& output, const ProtocolIBlock192& record) {
  put_u64(output, record.word0); put_u64(output, record.word1); put_u64(output, record.word2);
}
ProtocolIBlock192 get_record(const Bytes& input, std::size_t& offset, const char* message) {
  return {get_u64(input, offset, message), get_u64(input, offset, message),
          get_u64(input, offset, message)};
}

void wait_io(int fd, short events) {
  pollfd descriptor{fd, events, 0};
  const auto status = ::poll(&descriptor, 1, kTimeoutMs);
  require(status > 0 && (descriptor.revents & (POLLERR | POLLNVAL)) == 0,
          "process channel timeout/error");
}
void write_all(int fd, const Bytes& bytes) {
  std::size_t offset = 0;
  while (offset != bytes.size()) {
    wait_io(fd, POLLOUT);
    const auto count = ::write(fd, bytes.data() + offset, bytes.size() - offset);
    if (count < 0 && errno == EINTR) continue;
    require(count > 0, "process write");
    offset += static_cast<std::size_t>(count);
  }
}
Bytes read_to_eof(int fd) {
  Bytes output;
  std::array<std::uint8_t, 4096> buffer{};
  for (;;) {
    wait_io(fd, POLLIN);
    const auto count = ::read(fd, buffer.data(), buffer.size());
    if (count < 0 && errno == EINTR) continue;
    require(count >= 0, "process read");
    if (count == 0) return output;
    require(output.size() <= kMaxBytes - static_cast<std::size_t>(count), "message bound");
    output.insert(output.end(), buffer.begin(), buffer.begin() + count);
  }
}
void close_fd(int& fd) { if (fd >= 0) { ::close(fd); fd = -1; } }
void close_except(const std::vector<int>& keep) {
  const auto maximum = std::min<long>(4096, ::sysconf(_SC_OPEN_MAX));
  for (int fd = 3; fd < maximum; ++fd)
    if (std::find(keep.begin(), keep.end(), fd) == keep.end()) ::close(fd);
}
void make_socketpair(int pair[2]) {
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0, "socketpair");
  for (const auto fd : {pair[0], pair[1]}) {
    const auto flags = ::fcntl(fd, F_GETFD);
    require(flags >= 0 && ::fcntl(fd, F_SETFD, flags & ~FD_CLOEXEC) == 0,
            "socket inheritance");
  }
}

std::uint64_t parse_u64(const char* text) {
  char* end = nullptr; errno = 0;
  const auto value = std::strtoull(text, &end, 10);
  require(errno == 0 && end != text && *end == '\0', "integer argument");
  return value;
}
int parse_fd(const char* text) {
  const auto value = parse_u64(text);
  require(value <= static_cast<std::uint64_t>(std::numeric_limits<int>::max()), "fd range");
  return static_cast<int>(value);
}
CaseConfig parse_case(int argc, char** argv, int offset) {
  require(argc == offset + 7 && std::strcmp(argv[offset], "--case") == 0, "case arguments");
  const auto n = parse_u64(argv[offset + 1]), k = parse_u64(argv[offset + 2]);
  const auto bits = parse_u64(argv[offset + 3]);
  require(n <= UINT32_MAX && k <= UINT32_MAX && bits <= UINT8_MAX, "case range");
  CaseConfig config{static_cast<std::uint32_t>(n), static_cast<std::uint32_t>(k),
                    static_cast<std::uint8_t>(bits), parse_u64(argv[offset + 4]),
                    parse_u64(argv[offset + 5]), parse_u64(argv[offset + 6])};
  require(config.n >= 2 && config.k >= 1 && config.k <= config.n && config.bits >= 34 &&
              config.bits <= 53 && config.session && config.fingerprint && config.material_id,
          "case binding");
  return config;
}

Bytes serialize_input(const CaseConfig& config, int party,
                      const std::vector<ProtocolIBlock192>& records) {
  require((party == 0 || party == 1) && records.size() == config.n, "input shape");
  Bytes output{'M','2','P','I',1,static_cast<std::uint8_t>(party),config.bits,0};
  put_u64(output, config.session); put_u64(output, config.fingerprint);
  put_u64(output, config.material_id); put_u32(output, config.n); put_u32(output, config.k);
  for (const auto& record : records) put_record(output, record);
  return output;
}
std::vector<ProtocolIBlock192> deserialize_input(const Bytes& input,
                                                  const CaseConfig& config, int party) {
  require(input.size() == 40U + 24U * config.n && input[0]=='M' && input[1]=='2' &&
              input[2]=='P' && input[3]=='I' && input[4]==1 && input[5]==party &&
              input[6]==config.bits && input[7]==0, "input header");
  std::size_t offset = 8;
  require(get_u64(input,offset,"input session")==config.session &&
              get_u64(input,offset,"input fingerprint")==config.fingerprint &&
              get_u64(input,offset,"input material")==config.material_id &&
              get_u32(input,offset,"input n")==config.n &&
              get_u32(input,offset,"input k")==config.k, "input binding");
  std::vector<ProtocolIBlock192> records(config.n);
  const auto mask = (UINT64_C(1) << config.bits) - 1U;
  for (auto& record : records) {
    record = get_record(input, offset, "input record");
    require((record.word0 & ~mask) == 0, "input key ring");
  }
  return records;
}

Bytes serialize_stats(const DealerStats& stats) {
  require(stats.pi.size() == stats.full_r.size(), "stats shape");
  Bytes output{'M','2','P','S',1,0,0,0};
  put_u64(output, stats.package0_bytes); put_u64(output, stats.package1_bytes);
  put_u64(output, stats.metrics.record_material_logical_bits);
  put_u64(output, stats.metrics.permutation_material_logical_bits);
  put_u64(output, stats.metrics.cmpagg_party_material_wire_bytes);
  put_u32(output, stats.pi.size()); put_u32(output, 0);
  for (const auto value : stats.pi) put_u32(output, value);
  for (const auto& record : stats.full_r) put_record(output, record);
  return output;
}
DealerStats deserialize_stats(const Bytes& input, std::uint32_t n) {
  require(input.size() == 56U + 28U * n && input[0]=='M' && input[1]=='2' &&
              input[2]=='P' && input[3]=='S' && input[4]==1, "stats header");
  std::size_t offset = 8; DealerStats stats;
  stats.package0_bytes=get_u64(input,offset,"stats package0");
  stats.package1_bytes=get_u64(input,offset,"stats package1");
  stats.metrics.record_material_logical_bits=get_u64(input,offset,"stats record bits");
  stats.metrics.permutation_material_logical_bits=get_u64(input,offset,"stats permutation bits");
  stats.metrics.cmpagg_party_material_wire_bytes=get_u64(input,offset,"stats cmpagg bytes");
  require(get_u32(input,offset,"stats n")==n && get_u32(input,offset,"stats reserved")==0,
          "stats dimensions");
  stats.pi.resize(n); stats.full_r.resize(n);
  for (auto& value : stats.pi) value=get_u32(input,offset,"stats permutation");
  for (auto& record : stats.full_r) record=get_record(input,offset,"stats mask");
  protocol_i_validate_permutation(stats.pi);
  return stats;
}

Bytes serialize_result(const CaseConfig& config, int party,
                       const ProtocolIParallelShuffleNetworkOutput& result) {
  const auto& core = result.core;
  require(core.shuffled_share.size()==config.n && core.public_masked_records.size()==config.n &&
              core.public_ranks.size()==config.n && core.sorted_share.size()==config.n,
          "result shape");
  Bytes output{'M','2','P','R',1,static_cast<std::uint8_t>(party),config.bits,0};
  put_u64(output,config.session); put_u64(output,config.fingerprint);
  put_u64(output,config.material_id); put_u32(output,config.n); put_u32(output,config.k);
  for (const auto value : {result.metrics.round1_sent_bytes,result.metrics.round1_received_bytes,
                           result.metrics.round2_sent_bytes,result.metrics.round2_received_bytes,
                           result.metrics.round3_sent_bytes,result.metrics.round3_received_bytes,
                           result.metrics.round1_logical_sent_bits,
                           result.metrics.round2_logical_sent_bits,
                           result.metrics.round3_logical_sent_bits,
                           result.metrics.online_rounds}) put_u64(output,value);
  for (const auto& record : core.shuffled_share) put_record(output,record);
  for (const auto& record : core.public_masked_records) put_record(output,record);
  for (const auto rank : core.public_ranks) put_u64(output,rank);
  for (const auto& record : core.sorted_share) put_record(output,record);
  return output;
}
PartyResult deserialize_result(const Bytes& input, const CaseConfig& config, int party) {
  require(input.size()==120U+80U*config.n && input[0]=='M' && input[1]=='2' &&
              input[2]=='P' && input[3]=='R' && input[4]==1 && input[5]==party &&
              input[6]==config.bits && input[7]==0, "result header");
  std::size_t offset=8;
  require(get_u64(input,offset,"result session")==config.session &&
              get_u64(input,offset,"result fingerprint")==config.fingerprint &&
              get_u64(input,offset,"result material")==config.material_id &&
              get_u32(input,offset,"result n")==config.n &&
              get_u32(input,offset,"result k")==config.k, "result binding");
  PartyResult result;
  auto& m=result.metrics;
  m.round1_sent_bytes=get_u64(input,offset,"metric"); m.round1_received_bytes=get_u64(input,offset,"metric");
  m.round2_sent_bytes=get_u64(input,offset,"metric"); m.round2_received_bytes=get_u64(input,offset,"metric");
  m.round3_sent_bytes=get_u64(input,offset,"metric"); m.round3_received_bytes=get_u64(input,offset,"metric");
  m.round1_logical_sent_bits=get_u64(input,offset,"metric");
  m.round2_logical_sent_bits=get_u64(input,offset,"metric");
  m.round3_logical_sent_bits=get_u64(input,offset,"metric"); m.online_rounds=get_u64(input,offset,"metric");
  auto read_records=[&](std::vector<ProtocolIBlock192>& records) {
    records.resize(config.n); for(auto& record:records) record=get_record(input,offset,"record");
  };
  read_records(result.core.shuffled_share); read_records(result.core.public_masked_records);
  result.core.public_ranks.resize(config.n);
  for(auto& rank:result.core.public_ranks) rank=get_u64(input,offset,"rank");
  read_records(result.core.sorted_share);
  require(offset==input.size(),"result trailing bytes"); return result;
}

int run_p2(const CaseConfig& config, int p0_fd, int p1_fd, int stats_fd) {
  close_except({p0_fd,p1_fd,stats_fd});
  try {
    auto generated=protocol_i_parallel_shuffle_dealer_generate(
        {config.session,config.fingerprint,config.material_id,config.n,config.k,config.bits});
    DealerStats stats;
    stats.pi=protocol_i_compose_permutation(generated.party0.tau,generated.party1.sigma);
    stats.full_r.resize(config.n);
    for(std::size_t i=0;i<config.n;++i)
      stats.full_r[i]=protocol_i_parallel_record_add(
          config.bits,generated.party0.r_share[i],generated.party1.r_share[i]);
    stats.metrics=generated.metrics;
    const auto package0=protocol_i_parallel_shuffle_serialize_material(generated.party0);
    const auto package1=protocol_i_parallel_shuffle_serialize_material(generated.party1);
    stats.package0_bytes=package0.size(); stats.package1_bytes=package1.size();
    write_all(p0_fd,package0); require(::shutdown(p0_fd,SHUT_WR)==0,"P2 P0 shutdown");
    write_all(p1_fd,package1); require(::shutdown(p1_fd,SHUT_WR)==0,"P2 P1 shutdown");
    write_all(stats_fd,serialize_stats(stats)); require(::shutdown(stats_fd,SHUT_WR)==0,"P2 stats shutdown");
    return 0;
  } catch(const std::exception& error) { std::cerr << "P2: " << error.what() << '\n'; return 1; }
}
int run_party(const CaseConfig& config, int party, int package_fd, int input_fd,
              const std::array<int,3>& round_fds, int result_fd) {
  close_except({package_fd,input_fd,round_fds[0],round_fds[1],round_fds[2],result_fd});
  try {
    auto material=protocol_i_parallel_shuffle_deserialize_material(read_to_eof(package_fd),party);
    const auto input=deserialize_input(read_to_eof(input_fd),config,party);
    ProtocolIParallelShufflePartyConfig party_config{config.session,config.fingerprint,
        config.material_id,config.n,config.k,config.bits,static_cast<std::uint8_t>(party),kTimeoutMs};
    auto output=protocol_i_parallel_shuffle_three_round_party(
        party_config,std::move(material),input,round_fds);
    write_all(result_fd,serialize_result(config,party,output));
    require(::shutdown(result_fd,SHUT_WR)==0,"result shutdown"); return 0;
  } catch(const std::exception& error) { std::cerr << "P" << party << ": " << error.what() << '\n'; return 1; }
}

pid_t spawn(const std::string& executable, const std::vector<std::string>& arguments) {
  const auto child=::fork(); require(child>=0,"fork");
  if(child==0) {
    std::vector<char*> argv; argv.push_back(const_cast<char*>(executable.c_str()));
    for(const auto& argument:arguments) argv.push_back(const_cast<char*>(argument.c_str()));
    argv.push_back(nullptr); ::execv(executable.c_str(),argv.data()); _exit(127);
  }
  return child;
}
void wait_success(pid_t child, const char* message) {
  int status=0; require(::waitpid(child,&status,0)==child,"waitpid");
  require(WIFEXITED(status) && WEXITSTATUS(status)==0,message);
}
std::string executable_path(const char* argv0) {
  std::array<char,4096> path{};
  const auto length=::readlink("/proc/self/exe",path.data(),path.size()-1U);
  if(length>0) { path[length]='\0'; return path.data(); } return argv0;
}
std::vector<ProtocolIBlock192> add_records(int bits,const std::vector<ProtocolIBlock192>& left,
                                            const std::vector<ProtocolIBlock192>& right) {
  require(left.size()==right.size(),"add shape"); auto output=left;
  for(std::size_t i=0;i<output.size();++i)
    output[i]=protocol_i_parallel_record_add(bits,output[i],right[i]);
  return output;
}
std::vector<std::uint64_t> clear_ranks(const std::vector<ProtocolIBlock192>& records) {
  std::vector<std::size_t> order(records.size());
  for(std::size_t i=0;i<order.size();++i) order[i]=i;
  std::sort(order.begin(),order.end(),[&](auto a,auto b){return records[a].word0<records[b].word0;});
  std::vector<std::uint64_t> ranks(records.size());
  for(std::size_t rank=0;rank<order.size();++rank) ranks[order[rank]]=rank;
  return ranks;
}
std::vector<ProtocolIBlock192> sorted_records(std::vector<ProtocolIBlock192> records) {
  std::sort(records.begin(),records.end(),[](const auto& a,const auto& b){return a.word0<b.word0;});
  return records;
}

CommunicationSample run_case(const CaseConfig& config,const std::vector<std::uint32_t>& scores,
                             const std::string& executable,bool report) {
  int p2p0[2],p2p1[2],stats_pair[2],input0[2],input1[2],round1[2],round2[2],round3[2],
      result0[2],result1[2];
  int* pairs[]={p2p0,p2p1,stats_pair,input0,input1,round1,round2,round3,result0,result1};
  for(auto pair:pairs) make_socketpair(pair);
  pid_t p0=-1,p1=-1,p2=-1;
  try {
    const std::vector<std::string> suffix{"--case",std::to_string(config.n),
      std::to_string(config.k),std::to_string(config.bits),std::to_string(config.session),
      std::to_string(config.fingerprint),std::to_string(config.material_id)};
    auto a0=std::vector<std::string>{"--role","p0","--package-fd",std::to_string(p2p0[1]),
      "--input-fd",std::to_string(input0[1]),"--r1-fd",std::to_string(round1[0]),
      "--r2-fd",std::to_string(round2[0]),"--r3-fd",std::to_string(round3[0]),
      "--result-fd",std::to_string(result0[0])};
    auto a1=std::vector<std::string>{"--role","p1","--package-fd",std::to_string(p2p1[1]),
      "--input-fd",std::to_string(input1[1]),"--r1-fd",std::to_string(round1[1]),
      "--r2-fd",std::to_string(round2[1]),"--r3-fd",std::to_string(round3[1]),
      "--result-fd",std::to_string(result1[0])};
    auto a2=std::vector<std::string>{"--role","p2","--p0-fd",std::to_string(p2p0[0]),
      "--p1-fd",std::to_string(p2p1[0]),"--stats-fd",std::to_string(stats_pair[0])};
    a0.insert(a0.end(),suffix.begin(),suffix.end()); a1.insert(a1.end(),suffix.begin(),suffix.end());
    a2.insert(a2.end(),suffix.begin(),suffix.end());
    p0=spawn(executable,a0); p1=spawn(executable,a1); p2=spawn(executable,a2);
    for(auto pair:{p2p0,p2p1,round1,round2,round3}) { close_fd(pair[0]); close_fd(pair[1]); }
    close_fd(stats_pair[0]); close_fd(input0[1]); close_fd(input1[1]);
    close_fd(result0[0]); close_fd(result1[0]);

    wait_success(p2,"P2 exit"); p2=-1;
    const auto stats=deserialize_stats(read_to_eof(stats_pair[1]),config.n);
    require(scores.size()==config.n,"score shape");
    std::vector<ProtocolIBlock192> clear(config.n),share0(config.n),share1(config.n);
    std::mt19937_64 rng(config.session^config.fingerprint); const auto mask=(UINT64_C(1)<<config.bits)-1U;
    for(std::size_t i=0;i<config.n;++i) {
      clear[i]={protocol_i_priority_key(scores[i],i,config.n).value,static_cast<std::uint64_t>(i),rng()};
      share0[i]={rng()&mask,rng(),rng()};
      share1[i]=protocol_i_parallel_record_sub(config.bits,clear[i],share0[i]);
    }
    clear.front().word2=0; clear.back().word2=UINT64_MAX;
    share1.front()=protocol_i_parallel_record_sub(config.bits,clear.front(),share0.front());
    share1.back()=protocol_i_parallel_record_sub(config.bits,clear.back(),share0.back());
    write_all(input0[0],serialize_input(config,0,share0)); require(::shutdown(input0[0],SHUT_WR)==0,"input0");
    write_all(input1[0],serialize_input(config,1,share1)); require(::shutdown(input1[0],SHUT_WR)==0,"input1");
    const auto out0=deserialize_result(read_to_eof(result0[1]),config,0);
    const auto out1=deserialize_result(read_to_eof(result1[1]),config,1);
    wait_success(p0,"P0 exit"); p0=-1; wait_success(p1,"P1 exit"); p1=-1;

    const auto shuffled=protocol_i_apply_permutation(stats.pi,clear);
    require(add_records(config.bits,out0.core.shuffled_share,out1.core.shuffled_share)==shuffled,
            "secret shuffle oracle");
    require(out0.core.public_masked_records==out1.core.public_masked_records &&
              out0.core.public_masked_records==add_records(config.bits,shuffled,stats.full_r),
            "public y oracle");
    require(out0.core.public_ranks==out1.core.public_ranks &&
              out0.core.public_ranks==clear_ranks(shuffled),"real CmpAgg oracle");
    require(add_records(config.bits,out0.core.sorted_share,out1.core.sorted_share)==sorted_records(clear),
            "sorted record oracle");
    const auto rank_bits=protocol_i_parallel_rank_bits(config.n);
    const auto record_bits=static_cast<std::uint64_t>(config.bits)+128U;
    const auto expected_online=4U*config.n*record_bits+2U*config.n*rank_bits;
    const auto observed_online=out0.metrics.round1_logical_sent_bits+out1.metrics.round1_logical_sent_bits+
      out0.metrics.round2_logical_sent_bits+out1.metrics.round2_logical_sent_bits+
      out0.metrics.round3_logical_sent_bits+out1.metrics.round3_logical_sent_bits;
    require(observed_online==expected_online && out0.metrics.online_rounds==3 &&
              out1.metrics.online_rounds==3,"online accounting");
    require(stats.metrics.record_material_logical_bits==6U*config.n*record_bits &&
              stats.metrics.permutation_material_logical_bits==4U*config.n*rank_bits,
            "offline accounting");
    const auto record_wire=48U+24U*config.n;
    const auto rank_wire=48U+config.n*((rank_bits+7U)/8U);
    for(const auto* m:{&out0.metrics,&out1.metrics})
      require(m->round1_sent_bytes==record_wire && m->round1_received_bytes==record_wire &&
              m->round2_sent_bytes==record_wire && m->round2_received_bytes==record_wire &&
              m->round3_sent_bytes==rank_wire && m->round3_received_bytes==rank_wire,
              "wire accounting");
    if(report) std::cout << "n=" << config.n << " k=" << config.k
      << " bits=" << static_cast<unsigned>(config.bits) << " online_logical_bits=" << observed_online
      << " offline_record_bits=" << stats.metrics.record_material_logical_bits
      << " offline_permutation_bits=" << stats.metrics.permutation_material_logical_bits
      << " p2_exited_before_online=true rounds=3\n";
    return {out0.metrics,out1.metrics};
  } catch(...) {
    for(const auto child:{p0,p1,p2}) if(child>0) { ::kill(child,SIGKILL); ::waitpid(child,nullptr,0); }
    throw;
  }
}
std::uint64_t total_sent(const ProtocolIParallelShuffleMetrics& metrics) {
  return metrics.round1_sent_bytes+metrics.round2_sent_bytes+metrics.round3_sent_bytes;
}

void check_cross_direction(const CommunicationSample& sample) {
  require(sample.party0.round1_sent_bytes==sample.party1.round1_received_bytes &&
              sample.party1.round1_sent_bytes==sample.party0.round1_received_bytes,
          "round1 sent/received accounting");
  require(sample.party0.round2_sent_bytes==sample.party1.round2_received_bytes &&
              sample.party1.round2_sent_bytes==sample.party0.round2_received_bytes,
          "round2 sent/received accounting");
  require(sample.party0.round3_sent_bytes==sample.party1.round3_received_bytes &&
              sample.party1.round3_sent_bytes==sample.party0.round3_received_bytes,
          "round3 sent/received accounting");
}

void run_communication_benchmark(const std::string& executable,bool include_128) {
  constexpr std::uint64_t kPayloadBits=128;
  constexpr unsigned kRepetitions=5;
  std::vector<std::uint32_t> sizes{2,4,8,16,20,32,64};
  if(include_128) sizes.push_back(128);
  std::ofstream csv("/tmp/m2_protocol_i_online_comm_results.csv",std::ios::trunc);
  require(csv.good(),"communication CSV open");
  csv << "repetition,n,comparison_bits,payload_bits,rank_bits,paper_total_bits,"
         "our_logical_bits,p0_wire_bytes,p1_wire_bytes,total_wire_bytes,"
         "wire_overhead_bits,wire_overhead_percent,r1_logical_bits,r1_wire_bytes,"
         "r2_logical_bits,r2_wire_bytes,r3_logical_bits,r3_wire_bytes\n";
  std::uint64_t serial=0;
  for(const auto n:sizes) {
    const auto bits=static_cast<std::uint8_t>(33U+protocol_i_index_bits(n));
    const auto rank_bits=protocol_i_parallel_rank_bits(n);
    const auto record_bits=static_cast<std::uint64_t>(bits)+kPayloadBits;
    const auto paper_bits=4U*n*record_bits+2U*n*rank_bits;
    std::vector<std::uint32_t> scores(n);
    for(std::size_t index=0;index<scores.size();++index)
      scores[index]=static_cast<std::uint32_t>((index*UINT64_C(2654435761))^(n-index));
    auto make_config=[&](std::uint64_t run) {
      const auto id=UINT64_C(0x90000000)+n*100U+run;
      return CaseConfig{n,std::max<std::uint32_t>(1,n/2),bits,id,id+UINT64_C(0x10000000),
                        id+UINT64_C(0x20000000)};
    };
    (void)run_case(make_config(++serial),scores,executable,false); // warmup
    std::uint64_t min_wire=UINT64_MAX,max_wire=0,sum_wire=0;
    std::uint64_t min_p0=UINT64_MAX,max_p0=0,sum_p0=0;
    std::uint64_t min_p1=UINT64_MAX,max_p1=0,sum_p1=0;
    CommunicationSample first{};
    for(unsigned repetition=1;repetition<=kRepetitions;++repetition) {
      const auto sample=run_case(make_config(++serial),scores,executable,false);
      check_cross_direction(sample);
      const auto p0=total_sent(sample.party0),p1=total_sent(sample.party1),wire=p0+p1;
      const auto r1_logical=sample.party0.round1_logical_sent_bits+
                            sample.party1.round1_logical_sent_bits;
      const auto r2_logical=sample.party0.round2_logical_sent_bits+
                            sample.party1.round2_logical_sent_bits;
      const auto r3_logical=sample.party0.round3_logical_sent_bits+
                            sample.party1.round3_logical_sent_bits;
      const auto logical=r1_logical+r2_logical+r3_logical;
      const auto r1_wire=sample.party0.round1_sent_bytes+sample.party1.round1_sent_bytes;
      const auto r2_wire=sample.party0.round2_sent_bytes+sample.party1.round2_sent_bytes;
      const auto r3_wire=sample.party0.round3_sent_bytes+sample.party1.round3_sent_bytes;
      require(logical==paper_bits,"paper/logical communication mismatch");
      if(repetition==1) first=sample;
      else require(sample.party0.round1_sent_bytes==first.party0.round1_sent_bytes &&
                       sample.party0.round2_sent_bytes==first.party0.round2_sent_bytes &&
                       sample.party0.round3_sent_bytes==first.party0.round3_sent_bytes &&
                       sample.party1.round1_sent_bytes==first.party1.round1_sent_bytes &&
                       sample.party1.round2_sent_bytes==first.party1.round2_sent_bytes &&
                       sample.party1.round3_sent_bytes==first.party1.round3_sent_bytes,
                   "non-deterministic communication bytes");
      min_wire=std::min(min_wire,wire); max_wire=std::max(max_wire,wire); sum_wire+=wire;
      min_p0=std::min(min_p0,p0); max_p0=std::max(max_p0,p0); sum_p0+=p0;
      min_p1=std::min(min_p1,p1); max_p1=std::max(max_p1,p1); sum_p1+=p1;
      const auto overhead_bits=wire*8U-logical;
      const auto overhead_percent=100.0*static_cast<double>(overhead_bits)/logical;
      csv << repetition << ',' << n << ',' << static_cast<unsigned>(bits) << ','
          << kPayloadBits << ',' << static_cast<unsigned>(rank_bits) << ',' << paper_bits << ','
          << logical << ',' << p0 << ',' << p1 << ',' << wire << ',' << overhead_bits << ','
          << std::fixed << std::setprecision(9) << overhead_percent << ',' << r1_logical << ','
          << r1_wire << ',' << r2_logical << ',' << r2_wire << ',' << r3_logical << ','
          << r3_wire << '\n';
    }
    std::cout << "COMM_SUMMARY n=" << n << " comparison_bits=" << static_cast<unsigned>(bits)
      << " payload_bits=" << kPayloadBits << " rank_bits=" << static_cast<unsigned>(rank_bits)
      << " paper_bits=" << paper_bits << " logical_bits=" << paper_bits
      << " p0_wire_min=" << min_p0 << " p0_wire_max=" << max_p0
      << " p0_wire_mean=" << static_cast<double>(sum_p0)/kRepetitions
      << " p1_wire_min=" << min_p1 << " p1_wire_max=" << max_p1
      << " p1_wire_mean=" << static_cast<double>(sum_p1)/kRepetitions
      << " total_wire_min=" << min_wire << " total_wire_max=" << max_wire
      << " total_wire_mean=" << static_cast<double>(sum_wire)/kRepetitions
      << " deterministic_bytes=" << (min_wire==max_wire?"YES":"NO") << '\n';
  }
  require(csv.good(),"communication CSV write");
}

}  // namespace

int main(int argc,char** argv) {
  try {
    if(argc>=3 && std::strcmp(argv[1],"--role")==0) {
      const std::string role=argv[2];
      if(role=="p2") {
        require(argc>=9 && std::strcmp(argv[3],"--p0-fd")==0 &&
          std::strcmp(argv[5],"--p1-fd")==0 && std::strcmp(argv[7],"--stats-fd")==0,"P2 args");
        const auto config=parse_case(argc,argv,9);
        return run_p2(config,parse_fd(argv[4]),parse_fd(argv[6]),parse_fd(argv[8]));
      }
      require((role=="p0"||role=="p1") && argc>=15 &&
        std::strcmp(argv[3],"--package-fd")==0 && std::strcmp(argv[5],"--input-fd")==0 &&
        std::strcmp(argv[7],"--r1-fd")==0 && std::strcmp(argv[9],"--r2-fd")==0 &&
        std::strcmp(argv[11],"--r3-fd")==0 && std::strcmp(argv[13],"--result-fd")==0,"party args");
      const auto config=parse_case(argc,argv,15); const int party=role=="p0"?0:1;
      return run_party(config,party,parse_fd(argv[4]),parse_fd(argv[6]),
        {parse_fd(argv[8]),parse_fd(argv[10]),parse_fd(argv[12])},parse_fd(argv[14]));
    }
    require(argc==1 || (argc==2 && (std::strcmp(argv[1],"--report")==0 ||
                std::strcmp(argv[1],"--comm-benchmark")==0 ||
                std::strcmp(argv[1],"--comm-benchmark-128")==0)),"controller args");
    const auto executable=executable_path(argv[0]);
    if(argc==2 && (std::strcmp(argv[1],"--comm-benchmark")==0 ||
                    std::strcmp(argv[1],"--comm-benchmark-128")==0)) {
      run_communication_benchmark(executable,std::strcmp(argv[1],"--comm-benchmark-128")==0);
      return 0;
    }
    const bool report=argc==2;
    const std::vector<CaseConfig> cases{{2,1,34,0x810001,0x820001,0x830001},
      {4,2,35,0x810002,0x820002,0x830002},{8,4,36,0x810003,0x820003,0x830003},
      {8,8,36,0x810004,0x820004,0x830004}};
    const std::vector<std::vector<std::uint32_t>> scores{
      {static_cast<std::uint32_t>(INT32_MIN),static_cast<std::uint32_t>(INT32_MAX)},
      {7,7,7,7},{9,1,8,2,7,3,6,4},
      {UINT32_C(0x80000000),11,UINT32_C(0x7fffffff),42,11,0,42,3}};
    for(std::size_t i=0;i<cases.size();++i) run_case(cases[i],scores[i],executable,report);
    return 0;
  } catch(const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
