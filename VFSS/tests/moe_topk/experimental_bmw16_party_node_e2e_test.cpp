#include <moe_topk/topk_oracle.h>
#include <moe_topk/experimental_bmw16_startup_gate.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <grp.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;
namespace {
constexpr uid_t kUid0 = 22012, kUid1 = 22013;
constexpr std::size_t kChannels = 13;  // Twelve API channels plus the pre-protocol ready barrier.
constexpr std::size_t kOnlineTlsChannels = 12;
void require(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
std::vector<std::int32_t> extended_scores(std::uint32_t n);
std::vector<std::int32_t> seeded_scores(std::uint32_t n, std::uint64_t seed);

void write_all(int fd, const std::uint8_t* p, std::size_t n) {
  while (n) {
    auto amount = ::write(fd, p, n);
    if (amount < 0 && errno == EINTR) continue;
    require(amount > 0, "fixture write");
    p += amount; n -= static_cast<std::size_t>(amount);
  }
}
void write_private(const fs::path& path, const std::vector<std::uint8_t>& bytes, uid_t uid) {
  const int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
  require(fd >= 0, "private fixture create");
  write_all(fd, bytes.data(), bytes.size());
  require(::fdatasync(fd) == 0, "private fixture sync");
  require(::close(fd) == 0 && ::chown(path.c_str(), uid, uid) == 0 && ::chmod(path.c_str(), 0600) == 0,
          "private fixture ownership");
}
std::vector<std::uint8_t> read_all(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  require(static_cast<bool>(in), "fixture read");
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
std::array<std::uint8_t, 32> os_key() {
  std::array<std::uint8_t, 32> key{};
  std::size_t done = 0;
  while (done < key.size()) {
    auto got = ::getrandom(key.data() + done, key.size() - done, 0);
    if (got < 0 && errno == EINTR) continue;
    require(got > 0, "OS key generation"); done += static_cast<std::size_t>(got);
  }
  return key;
}
std::string self_path() {
  std::array<char, 4096> bytes{};
  auto n = ::readlink("/proc/self/exe", bytes.data(), bytes.size() - 1);
  require(n > 0, "node executable path");
  return std::string(bytes.data(), static_cast<std::size_t>(n));
}
int wait_status(pid_t pid) {
  int status = 0; require(::waitpid(pid, &status, 0) == pid, "wait child");
  return WIFEXITED(status) ? WEXITSTATUS(status) : 128;
}
int exec_capture(const std::vector<std::string>& args, const fs::path& output,
                 const std::string& failpoint = {}) {
  const auto pid = ::fork(); require(pid >= 0, "fork child");
  if (pid == 0) {
    if (!failpoint.empty() && ::setenv("MOE_BMW16_TEST_FAILPOINT", failpoint.c_str(), 1) != 0)
      ::_exit(125);
    const int fd = ::open(output.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    if (fd < 0 || ::dup2(fd, STDOUT_FILENO) < 0 || ::dup2(fd, STDERR_FILENO) < 0) ::_exit(126);
    ::close(fd);
    std::vector<char*> av; av.reserve(args.size() + 1);
    for (const auto& arg : args) av.push_back(const_cast<char*>(arg.c_str()));
    av.push_back(nullptr); ::execv(av[0], av.data()); ::_exit(127);
  }
  return wait_status(pid);
}

int run_tool(const std::vector<std::string>& args) {
  const auto pid=::fork();require(pid>=0,"fork test utility");
  if(pid==0){const int nullfd=::open("/dev/null",O_WRONLY|O_CLOEXEC);if(nullfd>=0){::dup2(nullfd,STDOUT_FILENO);::dup2(nullfd,STDERR_FILENO);}
    std::vector<char*> av;for(const auto& a:args)av.push_back(const_cast<char*>(a.c_str()));av.push_back(nullptr);
    ::execvp(av[0],av.data());::_exit(127);}
  return wait_status(pid);
}

struct TestTlsCredentials {
  fs::path ca_cert,dealer_cert,dealer_key,p0_cert,p0_key,p1_cert,p1_key,rogue_ca,rogue_cert,rogue_key;
};

void make_signed_cert(const fs::path& dir,const std::string& name,const std::string& dns,
                      const fs::path& ca_cert,const fs::path& ca_key,fs::path& cert,fs::path& key) {
  const auto csr=dir/(name+".csr"),ext=dir/(name+".ext");key=dir/(name+".key");cert=dir/(name+".crt");
  {std::ofstream out(ext);require(static_cast<bool>(out),"write test cert extensions");
    out<<"subjectAltName=DNS:"<<dns<<"\nextendedKeyUsage=serverAuth,clientAuth\nkeyUsage=digitalSignature,keyEncipherment\n";}
  require(run_tool({"openssl","req","-new","-newkey","rsa:2048","-nodes","-keyout",key.string(),
      "-out",csr.string(),"-subj","/CN="+dns})==0,"generate test TLS key/csr");
  require(run_tool({"openssl","x509","-req","-in",csr.string(),"-CA",ca_cert.string(),"-CAkey",ca_key.string(),
      "-CAcreateserial","-out",cert.string(),"-days","2","-extfile",ext.string()})==0,"sign test TLS certificate");
}

TestTlsCredentials make_test_tls_credentials(const fs::path& root) {
  const auto dir=root/"tls-test-ca";require(::mkdir(dir.c_str(),0700)==0,"create TLS test CA directory");
  TestTlsCredentials c;const auto ca_key=dir/"ca.key";c.ca_cert=dir/"ca.crt";
  require(run_tool({"openssl","req","-x509","-newkey","rsa:2048","-nodes","-keyout",ca_key.string(),
      "-out",c.ca_cert.string(),"-subj","/CN=BMW16-S20-test-CA","-days","2"})==0,"generate test CA");
  fs::path ignored;
  make_signed_cert(dir,"dealer","dealer",c.ca_cert,ca_key,c.dealer_cert,c.dealer_key);
  make_signed_cert(dir,"party0","party0",c.ca_cert,ca_key,c.p0_cert,c.p0_key);
  make_signed_cert(dir,"party1","party1",c.ca_cert,ca_key,c.p1_cert,c.p1_key);
  c.rogue_ca=dir/"rogue-ca.crt";const auto rogue_key=dir/"rogue-ca.key";
  require(run_tool({"openssl","req","-x509","-newkey","rsa:2048","-nodes","-keyout",rogue_key.string(),
      "-out",c.rogue_ca.string(),"-subj","/CN=BMW16-S20-rogue-CA","-days","2"})==0,"generate rogue test CA");
  make_signed_cert(dir,"rogue-dealer","dealer",c.rogue_ca,rogue_key,c.rogue_cert,c.rogue_key);
  return c;
}

int listen_loopback(std::uint16_t& port) {
  const int fd=::socket(AF_INET,SOCK_STREAM,0);require(fd>=0,"TLS test socket create");
  int one=1;(void)::setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one));
  sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);address.sin_port=0;
  require(::bind(fd,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0&&::listen(fd,4)==0,"TLS test bind/listen");
  socklen_t size=sizeof(address);require(::getsockname(fd,reinterpret_cast<sockaddr*>(&address),&size)==0,"TLS test bound port");
  port=ntohs(address.sin_port);return fd;
}

std::uint16_t free_loopback_port_block() {
  for (std::uint32_t base = 24000; base <= 59000; base += 16) {
    std::array<int, kOnlineTlsChannels> fds{};
    fds.fill(-1);
    bool available = true;
    for (std::size_t i = 0; i < fds.size(); ++i) {
      fds[i] = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
      if (fds[i] < 0) { available = false; break; }
      sockaddr_in address{};
      address.sin_family = AF_INET;
      address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      address.sin_port = htons(static_cast<std::uint16_t>(base + i));
      if (::bind(fds[i], reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        available = false;
        break;
      }
    }
    for (auto fd : fds) if (fd >= 0) ::close(fd);
    if (available) return static_cast<std::uint16_t>(base);
  }
  throw std::runtime_error("could not reserve loopback port block for online TLS");
}

std::vector<std::uint8_t> read_exact_fd(int fd, std::size_t length) {
  std::vector<std::uint8_t> bytes(length);
  std::size_t offset = 0;
  while (offset < bytes.size()) {
    const auto got = ::read(fd, bytes.data() + offset, bytes.size() - offset);
    if (got < 0 && errno == EINTR) continue;
    require(got > 0, "online listener receipt truncated");
    offset += static_cast<std::size_t>(got);
  }
  return bytes;
}

pid_t spawn_as(const std::vector<std::string>& args,const fs::path& output,uid_t uid,
               const std::vector<int>& close_fds={},const std::string& failpoint={}) {
  const auto pid=::fork();require(pid>=0,"fork independent TLS role");
  if(pid==0){for(const auto fd:close_fds)::close(fd);
    if(!failpoint.empty()&&::setenv("MOE_BMW16_TEST_FAILPOINT",failpoint.c_str(),1)!=0)::_exit(125);
    if(::setgroups(0,nullptr)!=0||::setgid(uid)!=0||::setuid(uid)!=0)::_exit(126);
    const int out=::open(output.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600);
    if(out<0||::dup2(out,STDOUT_FILENO)<0||::dup2(out,STDERR_FILENO)<0)::_exit(126);::close(out);
    std::vector<char*> av;for(const auto& a:args)av.push_back(const_cast<char*>(a.c_str()));av.push_back(nullptr);
    ::execv(av[0],av.data());::_exit(127);}
  return pid;
}
void require_unreadable_as(uid_t uid,const fs::path& path) {
  const auto pid=::fork();require(pid>=0,"fork cross-UID file access check");
  if(pid==0){
    if(::setgroups(0,nullptr)!=0||::setgid(uid)!=0||::setuid(uid)!=0)::_exit(126);
    const int fd=::open(path.c_str(),O_RDONLY|O_CLOEXEC);
    if(fd>=0){::close(fd);::_exit(1);}
    ::_exit(errno==EACCES||errno==EPERM?0:2);
  }
  require(wait_status(pid)==0,"party UID read another party private file");
}
void append_u32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
  for (unsigned i=0; i<4; ++i) bytes.push_back(static_cast<std::uint8_t>(value >> (8U*i)));
}
std::string find_field(const std::string& line, const std::string& key) {
  const auto at = line.find(key + "=");
  if (at == std::string::npos) return {};
  auto begin = at + key.size() + 1U;
  auto end = line.find_first_of(" \r\n", begin);
  return line.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
}

void run_case(const std::string& node, const std::vector<std::int32_t>& scores, std::uint32_t k,
              std::uint64_t session, const std::string& marker_failure = {}) {
  require(::geteuid() == 0, "party node E2E requires root to provision distinct OS identities");
  const auto root = fs::path("/tmp") / ("bmw16-s18-node-" + std::to_string(::getpid()) + "-" + std::to_string(session));
  require(::mkdir(root.c_str(), 0700) == 0 && ::chmod(root.c_str(), 0711) == 0, "node fixture root");
  auto cleanup = std::unique_ptr<void, std::function<void(void*)>>(reinterpret_cast<void*>(1), [root](void*) {
    std::error_code ec; fs::remove_all(root, ec);
  });
  const auto p0 = root / "p0", p1 = root / "p1";
  for (const auto& p : {p0, p1}) require(::mkdir(p.c_str(), 0700) == 0, "party directory");
  for (const auto& pair : {std::pair<fs::path, uid_t>{p0, kUid0}, {p1, kUid1}}) {
    require(::chown(pair.first.c_str(), pair.second, pair.second) == 0 && ::chmod(pair.first.c_str(), 0700) == 0,
            "party directory owner");
    const auto claims = pair.first / "claims";
    require(::mkdir(claims.c_str(), 0700) == 0 && ::chown(claims.c_str(), pair.second, pair.second) == 0 &&
            ::chmod(claims.c_str(), 0700) == 0, "party claim directory");
  }
  const auto key0 = os_key(), key1 = os_key();
  write_private(p0 / "aead.key", {key0.begin(), key0.end()}, kUid0);
  write_private(p1 / "aead.key", {key1.begin(), key1.end()}, kUid1);

  const std::uint32_t n = static_cast<std::uint32_t>(scores.size());
  const std::uint64_t fingerprint = session ^ UINT64_C(0x535331375f465052);
  const auto targs = std::vector<std::string>{node, "t-local-test-only", std::to_string(n), std::to_string(k),
      std::to_string(session), std::to_string(fingerprint), (p0/"claims").string(), (p1/"claims").string(),
      (p0/"material.shell").string(), (p1/"material.shell").string(),
      (p0/"ucmp.stream").string(), (p1/"ucmp.stream").string(), (p0/"aead.key").string(),
      (p1/"aead.key").string(), std::to_string(kUid0), std::to_string(kUid1),
      (root/"pair.ready").string()};
  const int t_exit = exec_capture(targs, root / "t.log");
  if (t_exit != 0) {
    const auto failed_t_log = read_all(root / "t.log");
    std::cerr << "offline T child exit=" << t_exit << " output="
              << std::string(failed_t_log.begin(), failed_t_log.end()) << '\n';
  }
  require(t_exit == 0, "offline T node failed");
  const auto t_log_bytes = read_all(root / "t.log");
  const std::string t_log(t_log_bytes.begin(), t_log_bytes.end());
  const auto expected_t_status = n == 1 ? "status=SINGLETON_NO_MATERIAL" : "status=OFFLINE_MATERIAL_READY";
  require(t_log.find(expected_t_status) != std::string::npos &&
      t_log.find("T_online=false") != std::string::npos, "T did not finish offline material role");
  const auto ready_path = root / "pair.ready";
  if (n > 1) require(fs::exists(ready_path) && fs::file_size(ready_path) == 132,
                     "T did not publish the paired completion marker");
  else require(!fs::exists(ready_path), "singleton unexpectedly published a paired-ready marker");
  std::cout << "node_T " << t_log;

  // The clear TEST_ONLY driver creates and writes shares only after T exits.
  std::mt19937_64 split_rng(session ^ UINT64_C(0x534841524553));
  std::array<std::vector<std::uint8_t>, 2> shares;
  for (const auto score : scores) {
    const auto word = static_cast<std::uint32_t>(score);
    const auto a = static_cast<std::uint32_t>(split_rng());
    append_u32(shares[0], a); append_u32(shares[1], word - a);
  }
  write_private(p0 / "raw-share.bin", shares[0], kUid0);
  write_private(p1 / "raw-share.bin", shares[1], kUid1);
  if (marker_failure == "missing") require(::unlink(ready_path.c_str()) == 0,
                                            "remove completion marker for early-party test");
  else if (marker_failure == "truncate") fs::resize_file(ready_path, 131);
  else if (marker_failure == "tamper") {
    std::fstream marker(ready_path, std::ios::binary | std::ios::in | std::ios::out);
    require(static_cast<bool>(marker), "open ready marker tamper fixture");
    char byte = 0; marker.read(&byte, 1); byte ^= 1; marker.seekp(0); marker.write(&byte, 1);
    marker.flush(); require(static_cast<bool>(marker), "tamper ready marker");
  } else require(marker_failure.empty(), "unknown ready marker failure fixture");

  std::array<std::array<int, 2>, kChannels> channels{};
  for (auto& pair : channels) require(::socketpair(AF_UNIX, SOCK_STREAM, 0, pair.data()) == 0, "party channels");
  std::array<pid_t, 2> children{};
  for (int party = 0; party < 2; ++party) {
    const auto child = ::fork(); require(child >= 0, "fork party node"); children[party] = child;
    if (child == 0) {
      for (const auto& pair : channels) ::close(pair[1 - party]);
      if (::setgroups(0, nullptr) != 0 || ::setgid(party ? kUid1 : kUid0) != 0 ||
          ::setuid(party ? kUid1 : kUid0) != 0) ::_exit(126);
      const auto dir = party ? p1 : p0;
      std::vector<std::string> args{node, "party", std::to_string(party), std::to_string(n), std::to_string(k),
          std::to_string(session), std::to_string(fingerprint), (dir/"claims").string(),
          (dir/"raw-share.bin").string(), (dir/"material.shell").string(),
          (dir/"ucmp.stream").string(), (dir/"aead.key").string(),
          (dir/"mask.share").string(), "30000", ready_path.string()};
      for (const auto& pair : channels) args.push_back(std::to_string(pair[party]));
      std::vector<char*> av; for (auto& arg : args) av.push_back(arg.data()); av.push_back(nullptr);
      ::execv(node.c_str(), av.data()); ::_exit(127);
    }
  }
  for (auto& pair : channels) { ::close(pair[0]); ::close(pair[1]); }
  // The controller is TEST_ONLY and may read both party shares only after exit
  // to perform the frozen-oracle differential.
  const auto e0 = wait_status(children[0]), e1 = wait_status(children[1]);
  require(e0 == e1, "independent node exit codes differ");
  if (!marker_failure.empty()) {
    require(e0 == 20 && e1 == 20 && !fs::exists(p0 / "mask.share") &&
            !fs::exists(p1 / "mask.share"),
            "parties accepted invalid paired completion marker");
  } else if (e0 == 0) {
    const auto m0 = read_all(p0 / "mask.share"), m1 = read_all(p1 / "mask.share");
    require(m0.size() == n && m1.size() == n, "independent node output share size");
    std::vector<std::uint8_t> mask(n);
    for (std::size_t i=0;i<n;++i) mask[i] = m0[i] ^ m1[i];
    std::vector<std::uint32_t> words; for (auto score : scores) words.push_back(static_cast<std::uint32_t>(score));
    require(mask == moe_topk::top_k_mask(words, k), "independent node oracle mismatch");
  } else {
    require((e0 == 10 || e0 == 20 || e0 == 30) && !fs::exists(p0 / "mask.share") &&
            !fs::exists(p1 / "mask.share"), "independent node failure status/mask contract");
  }
  std::cout << "node_e2e n=" << n << " K=" << k << " session=" << session
            << " ready_failure=" << (marker_failure.empty() ? "none" : marker_failure)
            << " status=" << (!marker_failure.empty() ? "INVALID_READY_REJECTED" : e0 == 0 ? "SUCCESS_ORACLE_CHECKED" : e0 == 10 ? "AGREED_ALGORITHM_ABORT" :
                e0 == 20 ? "MATERIAL_ABORT" : "COMMUNICATION_ABORT")
            << " p0_exit=" << e0 << " p1_exit=" << e1 << " T_online=false\n";
}

void run_tls_case(const std::string& node,const std::vector<std::int32_t>& scores,std::uint32_t k,
                  std::uint64_t session,const TestTlsCredentials& tls,
                  const std::string& t_failpoint={},bool expect_delivery_abort=false,
                  const std::string& conflict={}, bool use_rogue_t=false,
                  const std::string& online_peer_identity_override={},
                  bool expect_online_abort=false,
                  const std::string& online_failpoint={},
                  bool expect_algorithm_abort=false,
                  bool expect_online_timeout=false,
                  const std::string& p1_only_failpoint={},
                  bool expect_engineering_failure=false,
                  bool expect_tls_stream_failure=false,
                  bool expect_final_disagreement=false,
                  bool expect_mask_publish_failure=false,
                  bool conditional_secure_v1=false) {
  require(::geteuid()==0,"TLS material E2E requires isolated UID provisioning");
  static std::uint64_t serial=0;
  const auto root=fs::path("/tmp")/("bmw16-s20-tls-"+std::to_string(::getpid())+"-"+std::to_string(serial++));
  require(::mkdir(root.c_str(),0700)==0&&::chmod(root.c_str(),0711)==0,"TLS case root");
  auto cleanup=std::unique_ptr<void,std::function<void(void*)>>(reinterpret_cast<void*>(1),[root](void*){
    if (std::getenv("MOE_BMW16_KEEP_TEST_FIXTURES")) {
      std::cerr << "TEST_ONLY fixture_preserved=" << root << '\n';
      return;
    }
    std::error_code ec;fs::remove_all(root,ec);
  });
  const auto p0=root/"p0",p1=root/"p1",td=root/"t";
  for(const auto& p:{p0,p1,td})require(::mkdir(p.c_str(),0700)==0,"TLS role directory");
  for(const auto& pair:{std::pair<fs::path,uid_t>{p0,kUid0},{p1,kUid1},{td,22011}})
    require(::chown(pair.first.c_str(),pair.second,pair.second)==0&&::chmod(pair.first.c_str(),0700)==0,"TLS role ownership");
  const auto claims0=p0/"claims",claims1=p1/"claims",spool=td/"spool";
  for(const auto& p:{claims0,claims1,spool})require(::mkdir(p.c_str(),0700)==0,"TLS package/claim directory");
  require(::chown(claims0.c_str(),kUid0,kUid0)==0&&::chown(claims1.c_str(),kUid1,kUid1)==0&&
          ::chown(spool.c_str(),22011,22011)==0,"TLS private directory ownership");
  require(::chmod(claims0.c_str(),0700)==0&&::chmod(claims1.c_str(),0700)==0&&::chmod(spool.c_str(),0700)==0,"TLS private directory mode");

  const auto key0=os_key(),key1=os_key();
  write_private(p0/"aead.key",{key0.begin(),key0.end()},kUid0);
  write_private(p1/"aead.key",{key1.begin(),key1.end()},kUid1);
  write_private(td/"wrap0.key",{key0.begin(),key0.end()},22011);
  write_private(td/"wrap1.key",{key1.begin(),key1.end()},22011);
  write_private(td/"dealer.crt",read_all(tls.dealer_cert),22011);
  write_private(td/"dealer.key",read_all(tls.dealer_key),22011);
  write_private(td/"ca.pem",read_all(tls.ca_cert),22011);
  write_private(td/"rogue.crt",read_all(tls.rogue_cert),22011);
  write_private(td/"rogue.key",read_all(tls.rogue_key),22011);
  write_private(td/"rogue-ca.pem",read_all(tls.rogue_ca),22011);
  write_private(p0/"party.crt",read_all(tls.p0_cert),kUid0);
  write_private(p0/"party.key",read_all(tls.p0_key),kUid0);
  write_private(p0/"ca.pem",read_all(tls.ca_cert),kUid0);
  write_private(p1/"party.crt",read_all(tls.p1_cert),kUid1);
  write_private(p1/"party.key",read_all(tls.p1_key),kUid1);
  write_private(p1/"ca.pem",read_all(tls.ca_cert),kUid1);

  const auto n=static_cast<std::uint32_t>(scores.size());
  const auto fingerprint=session^UINT64_C(0x5332305f544c5346);
  struct stat st0{},st1{};require(::stat(claims0.c_str(),&st0)==0&&::stat(claims1.c_str(),&st1)==0,"TLS claim identity stat");
  std::uint16_t port0=0,port1=0;const int listen0=listen_loopback(port0),listen1=listen_loopback(port1);
  const auto shell0=claims0/"material.shell",side0=claims0/"ucmp.stream",ready0=claims0/"pair.ready";
  const auto shell1=claims1/"material.shell",side1=claims1/"ucmp.stream",ready1=claims1/"pair.ready";
  const auto t_shell0=spool/"p0.material.shell",t_side0=spool/"p0.ucmp.stream";
  const auto t_shell1=spool/"p1.material.shell",t_side1=spool/"p1.ucmp.stream";
  fs::path protected_path;
  if(conflict=="existing-p0-shell")protected_path=shell0;
  else if(conflict=="existing-p1-sidecar")protected_path=side1;
  if(!protected_path.empty()){
    const std::vector<std::uint8_t> sentinel{'S','2','0','-','P','R','E','E','X','I','S','T'};
    const int fd=::open(protected_path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600);
    require(fd>=0,"TLS preexisting package fixture");write_all(fd,sentinel.data(),sentinel.size());
    require(::fdatasync(fd)==0&&::close(fd)==0,"TLS preexisting package sync");
  }
  const std::vector<std::uint8_t> preexisting_sentinel{'S','2','0','-','P','R','E','E','X','I','S','T'};
  // Full-pool n=1000 KeyGen may run longer than 10 minutes before T opens
  // its first delivery connection; keep receivers alive for the bounded 30m run.
  auto receiver_args=[&](std::uint8_t party,std::uint16_t /*port*/,int listen_fd){
    const auto& claim=party?claims1:claims0;const auto& shell=party?shell1:shell0;
    const auto& side=party?side1:side0;const auto& ready=party?ready1:ready0;const auto& dir=party?p1:p0;
    return std::vector<std::string>{node,"recv-tls",std::to_string(party),std::to_string(n),std::to_string(k),
      std::to_string(session),std::to_string(fingerprint),claim.string(),shell.string(),side.string(),ready.string(),
      (dir/"aead.key").string(),(dir/"party.crt").string(),(dir/"party.key").string(),(dir/"ca.pem").string(),
      "dealer",expect_delivery_abort?"2500":(n>=1000?"1800000":"30000"),std::to_string(listen_fd)};
  };
  const auto receiver_failpoint=t_failpoint=="receiver_fsync"?t_failpoint:
      t_failpoint=="receiver_publish"?t_failpoint:
      t_failpoint=="receiver_silent"?t_failpoint:std::string{};
  const auto recv0=spawn_as(receiver_args(0,port0,listen0),p0/"receive.log",kUid0,{listen1},
      (receiver_failpoint=="receiver_fsync"||receiver_failpoint=="receiver_silent")?receiver_failpoint:std::string{});
  const auto recv1=spawn_as(receiver_args(1,port1,listen1),p1/"receive.log",kUid1,{listen0},
      receiver_failpoint=="receiver_publish"?receiver_failpoint:std::string{});
  std::vector<std::string> targs{node,"t-tls",std::to_string(n),std::to_string(k),std::to_string(session),
      std::to_string(fingerprint),std::to_string(st0.st_dev),std::to_string(st0.st_ino),
      std::to_string(st1.st_dev),std::to_string(st1.st_ino),t_side0.string(),t_side1.string(),
      t_shell0.string(),t_shell1.string(),(td/"wrap0.key").string(),(td/"wrap1.key").string(),
      (td/(use_rogue_t?"rogue.crt":"dealer.crt")).string(),
      (td/(use_rogue_t?"rogue.key":"dealer.key")).string(),
      (td/(use_rogue_t?"rogue-ca.pem":"ca.pem")).string(),
      "127.0.0.1",std::to_string(port0),"party0","127.0.0.1",std::to_string(port1),"party1"};
  const auto tpid=spawn_as(targs,td/"t.log",22011,{listen0,listen1},t_failpoint);
  ::close(listen0);::close(listen1);
  const moe_topk::ProtocolIBmw16StartupBinding startup_binding{session,fingerprint,n,k};
  const std::array<moe_topk::ProtocolIBmw16OfflineChild,3> offline_children{{
    {tpid,moe_topk::ProtocolIBmw16OfflineRole::TrustedDealer,startup_binding},
    {recv0,moe_topk::ProtocolIBmw16OfflineRole::Party0Receiver,startup_binding},
    {recv1,moe_topk::ProtocolIBmw16OfflineRole::Party1Receiver,startup_binding}}};
  const auto gate=moe_topk::protocol_i_bmw16_start_after_offline_gate(
      startup_binding,offline_children,[&] {
    if (!fs::exists(ready0) || !fs::exists(ready1)) return false;
    const auto marker0 = read_all(ready0), marker1 = read_all(ready1);
    return marker0.size() == 132 && marker0 == marker1 && fs::exists(shell0) &&
        fs::exists(side0) && fs::exists(shell1) && fs::exists(side1);
  },[&] {
    // This callback is the only location at which the test creates input
    // shares. It cannot run until T and both committed receivers exit zero.
    std::mt19937_64 split_rng(session^UINT64_C(0x534841524553));
    std::array<std::vector<std::uint8_t>,2> shares;
    for(const auto score:scores){const auto word=static_cast<std::uint32_t>(score);const auto a=static_cast<std::uint32_t>(split_rng());
      append_u32(shares[0],a);append_u32(shares[1],word-a);}
    write_private(p0/"raw-share.bin",shares[0],kUid0);
    write_private(p1/"raw-share.bin",shares[1],kUid1);
    return 0;
  });
  const int t_exit=gate.exit_codes[0],r0=gate.exit_codes[1],r1=gate.exit_codes[2];
  const auto shell_exists=fs::exists(shell0)||fs::exists(shell1);
  const auto marker_exists=fs::exists(ready0)||fs::exists(ready1);
  if(expect_delivery_abort){
    require(!gate.gate_open&&t_exit!=0&&!fs::exists(p0/"raw-share.bin")&&!fs::exists(p1/"raw-share.bin")&&
            !fs::exists(p0/"mask.share")&&!fs::exists(p1/"mask.share"),"failed TLS delivery reached input/online/mask output");
    require((t_exit==20||t_exit==30||t_exit==71)&&
            (r0==0||r0==20||r0==30)&&(r1==0||r1==20||r1==30),
            "TLS failure escaped the structured material/transport/crash status contract");
    if(!protected_path.empty())require(fs::exists(protected_path)&&read_all(protected_path)==preexisting_sentinel,
                                       "TLS failure overwrote or removed preexisting package");
    if(t_failpoint!="receiver_publish"&&t_failpoint!="crash_t_tls_after_prepare0"&&
       t_failpoint!="crash_t_tls_after_commit0")
      require(!marker_exists,"failed TLS transfer exposed ready marker");
    std::cout<<"tls_delivery_abort n="<<n<<" K="<<k<<" failpoint="<<(t_failpoint.empty()?"none":t_failpoint)
             <<" credential="<<(use_rogue_t?"untrusted-T":"provisioned-test-T")
             <<" conflict="<<(conflict.empty()?"none":conflict)<<" T_exit="<<t_exit<<" P0_receive_exit="<<r0
             <<" P1_receive_exit="<<r1<<" final_files="<<(shell_exists?"present":"none")
             <<" ready_p0="<<(fs::exists(ready0)?"present":"none")<<" ready_p1="<<(fs::exists(ready1)?"present":"none")
             <<" orphan_policy=ONLINE_FORBIDDEN_UNLESS_T_AND_BOTH_RECEIVERS_EXIT_ZERO"
             <<" mask=NONE startup=BLOCKED_BY_T_STATUS\n";
    return;
  }
  if(t_exit!=0||r0!=0||r1!=0){
    for(const auto& path:{td/"t.log",p0/"receive.log",p1/"receive.log"})if(fs::exists(path)){
      const auto bytes=read_all(path);std::cerr<<"tls_failure_log "<<path.filename().string()<<" "
        <<std::string(bytes.begin(),bytes.end());}
    std::cerr<<"tls_failure_status T="<<t_exit<<" P0="<<r0<<" P1="<<r1<<" fixture="<<root<<"\n";
  }
  require(gate.gate_open&&gate.committed_pair_validated&&gate.post_gate_status==0&&
          t_exit==0&&r0==0&&r1==0,
          "trusted startup gate did not require T and both receiver exit-zero statuses");
  const auto tlog=read_all(td/"t.log"),log0=read_all(p0/"receive.log"),log1=read_all(p1/"receive.log");
  require(fs::exists(shell0)&&fs::exists(side0)&&fs::exists(ready0)&&fs::exists(shell1)&&fs::exists(side1)&&fs::exists(ready1),
          "TLS delivery missing one package component");
  std::cout<<"tls_T "<<std::string(tlog.begin(),tlog.end());
  std::cout<<"tls_P0_receive "<<std::string(log0.begin(),log0.end());
  std::cout<<"tls_P1_receive "<<std::string(log1.begin(),log1.end());

  // The startup gate callback has now produced the local input shares.
  for(const auto& path:{p1/"aead.key",p1/"party.key",claims1/"material.shell",claims1/"ucmp.stream",p1/"raw-share.bin"})
    require_unreadable_as(kUid0,path);
  for(const auto& path:{p0/"aead.key",p0/"party.key",claims0/"material.shell",claims0/"ucmp.stream",p0/"raw-share.bin"})
    require_unreadable_as(kUid1,path);
  for(const auto& path:{p0/"raw-share.bin",p1/"raw-share.bin",claims0/"ucmp.stream",claims1/"ucmp.stream"})
    require_unreadable_as(22011,path);
  std::cout<<"tls_uid_isolation n="<<n<<" P0_cannot_read_P1=PASS P1_cannot_read_P0=PASS T_cannot_read_online_input=PASS scope=LOCAL_TEST_UIDS\n";
  const auto online_port_base=free_loopback_port_block();
  int listener_ready[2]{};require(::pipe(listener_ready)==0,"online TLS listener-ready pipe");
  const std::string online_timeout =
      (!online_failpoint.empty() || !p1_only_failpoint.empty()) ? "1500" :
      (n>=1000 ? "1800000" : "30000");
  const auto party_mode=conditional_secure_v1?"party-conditional-secure-v1":"party-tls";
  const auto p1_args=std::vector<std::string>{node,party_mode,"1",std::to_string(n),std::to_string(k),
    std::to_string(session),std::to_string(fingerprint),claims1.string(),(p1/"raw-share.bin").string(),
    shell1.string(),side1.string(),(p1/"aead.key").string(),(p1/"mask.share").string(),
    online_timeout,ready1.string(),"127.0.0.1",std::to_string(online_port_base),"","0",
    (p1/"party.crt").string(),(p1/"party.key").string(),(p1/"ca.pem").string(),"party0",
    std::to_string(listener_ready[1])};
  const auto p1_online=spawn_as(p1_args,p1/"online.log",kUid1,{listener_ready[0]},
      !p1_only_failpoint.empty()?p1_only_failpoint:online_failpoint);
  ::close(listener_ready[1]);
  const auto listener_receipt=read_exact_fd(listener_ready[0],36);
  ::close(listener_ready[0]);
  auto get_be16=[&](std::size_t offset){return static_cast<std::uint16_t>((listener_receipt[offset]<<8U)|listener_receipt[offset+1]);};
  auto get_be32=[&](std::size_t offset){std::uint32_t v=0;for(std::size_t i=0;i<4;++i)v=(v<<8U)|listener_receipt[offset+i];return v;};
  auto get_be64=[&](std::size_t offset){std::uint64_t v=0;for(std::size_t i=0;i<8;++i)v=(v<<8U)|listener_receipt[offset+i];return v;};
  require(std::equal(listener_receipt.begin(),listener_receipt.begin()+8,
      std::array<std::uint8_t,8>{{'M','6','B','2','1','L','S','T'}}.begin())&&
      get_be16(8)==1&&get_be64(10)==session&&get_be64(18)==fingerprint&&
      get_be32(26)==n&&get_be32(30)==k&&get_be16(34)==online_port_base,
      "online TLS listener readiness binding");
  const auto p0_args=std::vector<std::string>{node,party_mode,"0",std::to_string(n),std::to_string(k),
    std::to_string(session),std::to_string(fingerprint),claims0.string(),(p0/"raw-share.bin").string(),
    shell0.string(),side0.string(),(p0/"aead.key").string(),(p0/"mask.share").string(),
    online_timeout,ready0.string(),"127.0.0.1","0","127.0.0.1",
    std::to_string(online_port_base),(p0/"party.crt").string(),(p0/"party.key").string(),
    (p0/"ca.pem").string(),online_peer_identity_override.empty()?"party1":online_peer_identity_override,"-1"};
  const auto p0_online=spawn_as(p0_args,p0/"online.log",kUid0,{},online_failpoint);
  const auto e0=wait_status(p0_online),e1=wait_status(p1_online);
  if(e0!=0||e1!=0){
    const bool mask0=fs::exists(p0/"mask.share"),mask1=fs::exists(p1/"mask.share");
    std::cout<<"tls_online_failure n="<<n<<" K="<<k<<" session="<<session<<" P0_exit="<<e0
             <<" P1_exit="<<e1<<" P0_mask="<<(mask0?"present":"none")<<" P1_mask="<<(mask1?"present":"none")
             <<" abort_scope=LOCAL_ONLY_OR_PEER_OBSERVED\n";
    require(!mask0&&!mask1,"TLS E2E failure published a partial mask share");
    if (expect_algorithm_abort) {
      require(e0 == 10 && e1 == 10 && gate.gate_open,
              "forced probability abort did not agree across TLS parties");
      std::cout << "tls_algorithm_abort n=" << n << " K=" << k << " P0_exit=" << e0
                << " P1_exit=" << e1 << " scope=PEER_AGREED mask=NONE injected=TEST_ONLY\n";
      return;
    }
    if (expect_online_abort) {
      require((e0 == 30 || e1 == 30) && gate.gate_open,
              "online TLS identity failure did not remain a post-start LOCAL_ONLY communication abort");
      std::cout << "tls_online_auth_abort n=" << n << " K=" << k << " P0_exit=" << e0
                << " P1_exit=" << e1 << " scope=LOCAL_ONLY mask=NONE identity=REJECTED\n";
      return;
    }
    if (expect_online_timeout) {
      require(e0 == 30 && e1 == 30 && gate.gate_open,
              "online silent peer did not produce bounded LOCAL_ONLY communication aborts");
      std::cout << "tls_online_silent_timeout n=" << n << " K=" << k << " P0_exit=" << e0
                << " P1_exit=" << e1 << " scope=LOCAL_ONLY_OR_PEER_OBSERVED mask=NONE\n";
      return;
    }
    if (expect_tls_stream_failure) {
      require(e0 == 30 && e1 == 30 && gate.gate_open,
              "missing authenticated TLS stream did not fail closed as communication abort");
      std::cout << "tls_missing_stream_fail_closed n=" << n << " K=" << k
                << " P0_exit=" << e0 << " P1_exit=" << e1
                << " mode=REQUIRE_AUTHENTICATED_STREAM fallback=NONE mask=NONE\n";
      return;
    }
    if (expect_engineering_failure || expect_final_disagreement || expect_mask_publish_failure) {
      require(e0 == 70 && e1 == 70 && gate.gate_open,
              "algorithm invariant or final status disagreement was not classified as engineering failure");
      const auto label = expect_mask_publish_failure ? "tls_mask_publish_failure" :
          expect_engineering_failure ? "tls_engineering_fault" : "tls_final_status_disagreement";
      std::cout << label
                << " n=" << n << " K=" << k << " P0_exit=" << e0 << " P1_exit=" << e1
                << " scope=" << (expect_mask_publish_failure ? "LOCAL_OUTPUT_ERROR" : "PEER_AGREED_OR_OBSERVED")
                << " mask=NONE\n";
      return;
    }
    throw std::runtime_error("TLS-delivered parties failed online E2E");
  }
  const auto m0=read_all(p0/"mask.share"),m1=read_all(p1/"mask.share");require(m0.size()==n&&m1.size()==n,"TLS E2E mask share length");
  std::vector<std::uint8_t> mask(n);for(std::size_t i=0;i<n;++i)mask[i]=m0[i]^m1[i];
  std::vector<std::uint32_t> words;for(auto score:scores)words.push_back(static_cast<std::uint32_t>(score));
  require(mask==moe_topk::top_k_mask(words,k),"TLS E2E frozen oracle mismatch");
  std::array<std::array<int,2>,kChannels> replay_channels{};
  for(auto& pair:replay_channels)require(::socketpair(AF_UNIX,SOCK_STREAM,0,pair.data())==0,"TLS replay channels");
  std::array<pid_t,2> replay_children{};
  for(int party=0;party<2;++party){const auto child=::fork();require(child>=0,"TLS replay process fork");replay_children[party]=child;
    if(child==0){for(const auto& pair:replay_channels)::close(pair[1-party]);
      const auto uid=party?kUid1:kUid0;if(::setgroups(0,nullptr)!=0||::setgid(uid)!=0||::setuid(uid)!=0)::_exit(126);
      const auto& dir=party?p1:p0;const auto& claim=party?claims1:claims0;const auto& shell=party?shell1:shell0;
      const auto& side=party?side1:side0;const auto& ready=party?ready1:ready0;
      std::vector<std::string> args{node,"party",std::to_string(party),std::to_string(n),std::to_string(k),
        std::to_string(session),std::to_string(fingerprint),claim.string(),(dir/"raw-share.bin").string(),
        shell.string(),side.string(),(dir/"aead.key").string(),(dir/"replay.mask.share").string(),
        n>=1000?"600000":"30000",ready.string()};
      for(const auto& pair:replay_channels)args.push_back(std::to_string(pair[party]));
      std::vector<char*> av;for(auto& a:args)av.push_back(a.data());av.push_back(nullptr);::execv(node.c_str(),av.data());::_exit(127);}}
  for(auto& pair:replay_channels){::close(pair[0]);::close(pair[1]);}
  const auto replay0=wait_status(replay_children[0]),replay1=wait_status(replay_children[1]);
  require(replay0==20&&replay1==20&&!fs::exists(p0/"replay.mask.share")&&!fs::exists(p1/"replay.mask.share"),
          "durable bundle claim did not reject process replay without a mask");
  std::cout<<"tls_replay n="<<n<<" K="<<k<<" P0_exit="<<replay0<<" P1_exit="<<replay1
           <<" status=DURABLE_CLAIM_REJECTED mask=NONE\n";
  std::cout<<(conditional_secure_v1?"conditional_secure_v1_e2e":"tls_e2e")
           <<" n="<<n<<" K="<<k<<" session="<<session<<" T_exit="<<t_exit
           <<" P0_receive_exit="<<r0<<" P1_receive_exit="<<r1<<" P0_online_exit="<<e0
           <<" P1_online_exit="<<e1<<" status=SUCCESS_ORACLE_CHECKED mask=original_order_weight_K\n";
}

void run_tls_suite(const std::string& node) {
  require(::geteuid()==0,"TLS E2E suite requires distinct OS identities");
  const auto root=fs::path("/tmp")/("bmw16-s20-tls-certs-"+std::to_string(::getpid()));
  require(::mkdir(root.c_str(),0700)==0,"create TLS certificate fixture root");
  auto cleanup=std::unique_ptr<void,std::function<void(void*)>>(reinterpret_cast<void*>(1),[root](void*){
    std::error_code ec;fs::remove_all(root,ec);
  });
  const auto tls=make_test_tls_credentials(root);
  run_tls_case(node,{INT32_MIN,INT32_MAX},1,UINT64_C(0x20201001),tls);
  run_tls_case(node,{INT32_MIN,0,INT32_MAX},2,UINT64_C(0x20201003),tls);
  run_tls_case(node,{INT32_MIN,7,7,INT32_MAX,-4},5,UINT64_C(0x20201005),tls);
  run_tls_case(node,extended_scores(8),4,UINT64_C(0x20201008),tls);
  run_tls_case(node,seeded_scores(64,UINT64_C(0x20206401)),8,UINT64_C(0x20206401),tls);
  run_tls_case(node,seeded_scores(128,UINT64_C(0x20212880)),128,UINT64_C(0x20212880),tls);
  run_tls_case(node,seeded_scores(256,UINT64_C(0x20225602)),2,UINT64_C(0x20225602),tls);
  run_tls_case(node,{INT32_MIN,INT32_MAX},1,UINT64_C(0x2020a001),tls,"",true,"",true);
  run_tls_case(node,{INT32_MIN,INT32_MAX},1,UINT64_C(0x2020a002),tls,"delivery_wrong_party",true);
  run_tls_case(node,{INT32_MIN,INT32_MAX},1,UINT64_C(0x2020a003),tls,"delivery_wrong_session",true);
  run_tls_case(node,{INT32_MIN,INT32_MAX},1,UINT64_C(0x2020a00c),tls,"delivery_wrong_stream",true);
  run_tls_case(node,{INT32_MIN,INT32_MAX},1,UINT64_C(0x2020a004),tls,"receiver_publish",true);
  run_tls_case(node,{INT32_MIN,INT32_MAX},1,UINT64_C(0x2020a005),tls,"receiver_fsync",true);
  run_tls_case(node,{INT32_MIN,INT32_MAX},1,UINT64_C(0x2020a00d),tls,"receiver_silent",true);
  run_tls_case(node,extended_scores(64),8,UINT64_C(0x2020a006),tls,"delivery_reorder_chunks",true);
  run_tls_case(node,extended_scores(64),8,UINT64_C(0x2020a007),tls,"delivery_truncate_sidecar",true);
  run_tls_case(node,extended_scores(64),8,UINT64_C(0x2020a008),tls,"delivery_corrupt_sidecar_chunk",true);
  run_tls_case(node,{INT32_MIN,INT32_MAX},1,UINT64_C(0x2020a009),tls,"",true,"existing-p1-sidecar");
  run_tls_case(node,{INT32_MIN,INT32_MAX},1,UINT64_C(0x2020a00a),tls,"crash_t_tls_after_prepare0",true);
  run_tls_case(node,{INT32_MIN,INT32_MAX},1,UINT64_C(0x2020a00b),tls,"crash_t_tls_after_commit0",true);
  run_tls_case(node,{INT32_MIN,INT32_MAX},1,UINT64_C(0x2020a00e),tls,"",false,"",false,
               "wrong-party1",true);
  run_tls_case(node,{INT32_MIN,0,INT32_MAX},2,UINT64_C(0x2020a00f),tls,"",false,"",false,
               "",false,"force_probability_abort_after_select",true);
  run_tls_case(node,{INT32_MIN,0,INT32_MAX},2,UINT64_C(0x2020a010),tls,"",false,"",false,
               "",false,"",false,true,"online_silent_after_tls");
  run_tls_case(node,{INT32_MIN,0,INT32_MAX},2,UINT64_C(0x2020a011),tls,"",false,"",false,
               "",false,"force_engineering_failure_after_select",false,false,"",true);
  run_tls_case(node,{INT32_MIN,0,INT32_MAX},2,UINT64_C(0x2020a012),tls,"",false,"",false,
               "",false,"online_missing_tls_stream",false,false,"",false,true);
  run_tls_case(node,{INT32_MIN,0,INT32_MAX},2,UINT64_C(0x2020a013),tls,"",false,"",false,
               "",false,"",false,false,"force_final_status_disagreement",false,false,true);
  run_tls_case(node,{INT32_MIN,0,INT32_MAX},2,UINT64_C(0x2020a014),tls,"",false,"",false,
               "",false,"mask_unlink_after_publish",false,false,"",false,false,false,true);
  run_tls_case(node,{INT32_MIN,0,INT32_MAX},2,UINT64_C(0x2020a015),tls,"",false,"",false,
               "",false,"mask_dir_fsync_after_publish",false,false,"",false,false,false,true);
}

void run_tls_n1000(const std::string& node) {
  require(::geteuid()==0,"n=1000 TLS E2E requires distinct OS identities");
  const auto root=fs::path("/tmp")/("bmw16-s20-tls-n1000-certs-"+std::to_string(::getpid()));
  require(::mkdir(root.c_str(),0700)==0,"create n=1000 TLS certificate fixture root");
  auto cleanup=std::unique_ptr<void,std::function<void(void*)>>(reinterpret_cast<void*>(1),[root](void*){
    std::error_code ec;fs::remove_all(root,ec);
  });
  const auto tls=make_test_tls_credentials(root);
  run_tls_case(node,seeded_scores(1000,UINT64_C(0x2020100080)),80,UINT64_C(0x2020100080),tls);
}

void verify_publication_failures(const std::string& node) {
  require(::geteuid() == 0, "publication failure tests require root for test UID setup");
  static std::uint64_t serial = 0;
  auto run = [&](const std::string& failpoint, const std::string& conflict,
                 bool crash, bool retry_same_paths, bool retry_new_paths) {
    const auto session = UINT64_C(0x191900) + serial++;
    const auto root = fs::path("/tmp") / ("bmw16-s19-publish-" + std::to_string(::getpid()) + "-" + std::to_string(session));
    require(::mkdir(root.c_str(), 0700) == 0 && ::chmod(root.c_str(), 0711) == 0,
            "publication fixture root");
    auto cleanup = std::unique_ptr<void, std::function<void(void*)>>(reinterpret_cast<void*>(1), [root](void*) {
      std::error_code ec; fs::remove_all(root, ec);
    });
    const auto p0 = root / "p0", p1 = root / "p1";
    for (const auto& p : {p0, p1}) require(::mkdir(p.c_str(), 0700) == 0, "publication party dir");
    for (const auto& pair : {std::pair<fs::path, uid_t>{p0, kUid0}, {p1, kUid1}}) {
      require(::chown(pair.first.c_str(), pair.second, pair.second) == 0 &&
              ::chmod(pair.first.c_str(), 0700) == 0, "publication party ownership");
      const auto claims = pair.first / "claims";
      require(::mkdir(claims.c_str(), 0700) == 0 &&
              ::chown(claims.c_str(), pair.second, pair.second) == 0 &&
              ::chmod(claims.c_str(), 0700) == 0, "publication claim dir");
    }
    const auto key0 = os_key(), key1 = os_key();
    write_private(p0 / "aead.key", {key0.begin(), key0.end()}, kUid0);
    write_private(p1 / "aead.key", {key1.begin(), key1.end()}, kUid1);
    const auto fingerprint = session ^ UINT64_C(0x535331375f465052);
    auto args = std::vector<std::string>{node, "t-local-test-only", "2", "1", std::to_string(session),
        std::to_string(fingerprint), (p0/"claims").string(), (p1/"claims").string(),
        (p0/"material.shell").string(), (p1/"material.shell").string(),
        (p0/"ucmp.stream").string(), (p1/"ucmp.stream").string(),
        (p0/"aead.key").string(), (p1/"aead.key").string(),
        std::to_string(kUid0), std::to_string(kUid1), (root/"pair.ready").string()};
    fs::path protected_path;
    const std::vector<std::uint8_t> sentinel{'S','1','9','-','P','R','E','E','X','I','S','T'};
    if (conflict == "sidecar1") protected_path = p1 / "ucmp.stream";
    else if (conflict == "shell1") protected_path = p1 / "material.shell";
    else if (conflict == "ready") protected_path = root / "pair.ready";
    if (!protected_path.empty()) {
      const int fd = ::open(protected_path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
      require(fd >= 0, "preexisting publication sentinel create");
      write_all(fd, sentinel.data(), sentinel.size());
      require(::fdatasync(fd) == 0 && ::close(fd) == 0, "preexisting publication sentinel sync");
    }
    const auto exit = exec_capture(args, root / "t.log", failpoint);
    const int expected_exit = crash ? 71 : 70;
    require(exit == expected_exit, "publication failpoint exit classification");
    if (protected_path != root / "pair.ready") {
      if (crash && failpoint == "crash_after_ready") {
        struct stat marker_stat{};
        require(::lstat((root/"pair.ready").c_str(), &marker_stat) == 0 &&
                S_ISREG(marker_stat.st_mode) && (marker_stat.st_mode & 0777) == 0600 &&
                marker_stat.st_uid == 0,
                "crash-left marker is private and cannot authorize party start");
      } else if (crash && failpoint == "crash_after_ready_durable") {
        struct stat marker_stat{};
        require(::lstat((root/"pair.ready").c_str(), &marker_stat) == 0 &&
                S_ISREG(marker_stat.st_mode) && (marker_stat.st_mode & 0777) == 0444 &&
                marker_stat.st_uid == 0 && marker_stat.st_size == 132,
                "durable marker crash fixture shape");
        // This TEST_ONLY launcher deliberately does not launch P0/P1 when T
        // exits nonzero, even though all material and the marker are durable.
      } else require(!fs::exists(root / "pair.ready"), "failed T published paired-ready marker");
    }
    if (!protected_path.empty()) require(read_all(protected_path) == sentinel,
                                         "failed T changed a preexisting destination");
    if (!crash) {
      for (const auto& path : {p0/"material.shell", p1/"material.shell",
                               p0/"ucmp.stream", p1/"ucmp.stream"})
        if (path != protected_path) require(!fs::exists(path), "failed T left its owned final file");
    }
    std::cout << "publication_fault failpoint=" << failpoint
              << " conflict=" << (conflict.empty() ? "none" : conflict)
              << " exit=" << exit << " ready_state_checked=true"
              << " party_start=" << (crash && failpoint == "crash_after_ready_durable"
                    ? "blocked_by_nonzero_T_exit" : "not_attempted")
              << " preexisting_preserved=" << (!protected_path.empty() ? "true" : "n/a")
              << " own_finals_rolled_back=" << (!crash ? "true" :
                    failpoint == "crash_after_ready_durable" ? "durable_orphan_quarantined" :
                    "crash_orphan_nonready") << '\n';
    if (retry_same_paths) {
      // A preexisting conflict is retained by T. The TEST_ONLY operator then
      // moves that unrelated sentinel away before explicitly retrying.
      if (!protected_path.empty()) require(::unlink(protected_path.c_str()) == 0,
                                           "test operator removes retained sentinel before retry");
      require(exec_capture(args, root / "t-retry.log") == 0 && fs::exists(root / "pair.ready"),
              "same-session retry after handled T failure");
      std::cout << "publication_retry mode=same_paths same_session=true status=ready\n";
    }
    if (retry_new_paths) {
      auto retry = args;
      retry[8] = (p0/"retry.shell").string(); retry[9] = (p1/"retry.shell").string();
      retry[10] = (p0/"retry.stream").string(); retry[11] = (p1/"retry.stream").string();
      retry[16] = (root/"retry.ready").string();
      require(exec_capture(retry, root / "t-retry-new-paths.log") == 0 &&
              fs::exists(root / "retry.ready"), "same-session retry with fresh paths after crash");
      std::cout << "publication_retry mode=fresh_paths same_session=true status=ready\n";
    }
  };

  for (const auto& point : {"after_sidecar0", "after_sidecar1", "after_shell0", "after_shell1",
       "chown_shell0", "chown_shell1", "chown_sidecar0",
       "chown_sidecar1", "before_ready", "bundle_dir_fsync", "sidecar0_dir_fsync",
       "sidecar1_dir_fsync", "ready_dir_fsync", "after_ready"})
    run(point, "", false, std::string(point) == "after_shell0", false);
  run("", "sidecar1", false, true, false);
  run("", "shell1", false, true, false);
  run("", "ready", false, true, false);
  run("crash_after_sidecar0", "", true, false, true);
  run("crash_after_sidecar1", "", true, false, false);
  run("crash_after_shell0", "", true, false, false);
  run("crash_after_shell1", "", true, false, false);
  run("crash_after_ready", "", true, false, false);
  run("crash_after_ready_durable", "", true, false, true);
  run("crash_before_ready", "", true, false, false);
}

std::vector<std::int32_t> extended_scores(std::uint32_t n) {
  std::vector<std::int32_t> scores(n);
  for (std::uint32_t i = 0; i < n; ++i) {
    const auto centered = static_cast<std::int64_t>((static_cast<std::uint64_t>(i) * 7919U) % 2000000001U) -
                          INT64_C(1000000000);
    scores[i] = static_cast<std::int32_t>(centered);
    if (i % 11U == 3U) scores[i] = -17;
  }
  scores.front() = INT32_MIN;
  scores.back() = INT32_MAX;
  return scores;
}

std::vector<std::int32_t> seeded_scores(std::uint32_t n, std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::vector<std::int32_t> scores(n);
  for (auto& score : scores) score = static_cast<std::int32_t>(rng());
  if (n >= 8) {
    scores[0] = INT32_MIN; scores[1] = INT32_MAX;
    scores[2] = -1; scores[3] = 0;
    scores[4] = scores[5];
  }
  return scores;
}
}  // namespace

int main(int argc, char** argv) {
  try {
    require(argc == 2 || argc == 3,
            "pass party node executable path [--extended|--small-ranks|--n1000|--tls|--tls-n1000]");
    const auto node = fs::canonical(argv[1]).string();
    if(argc==3&&std::string(argv[2])=="--tls") {
      run_case(node,{INT32_MIN},1,UINT64_C(0x20201000));
      run_tls_suite(node);
      return 0;
    }
    if(argc==3&&std::string(argv[2])=="--tls-n1000") {
      run_tls_n1000(node);
      return 0;
    }
    if(argc==3&&std::string(argv[2])=="--conditional-secure-v1") {
      const auto root=fs::path("/tmp")/("bmw16-s29-conditional-tls-certs-"+std::to_string(::getpid()));
      require(::mkdir(root.c_str(),0700)==0,"create conditional v1 TLS certificate fixture root");
      auto cleanup=std::unique_ptr<void,std::function<void(void*)>>(reinterpret_cast<void*>(1),[root](void*){
        std::error_code ec;fs::remove_all(root,ec);
      });
      const auto tls=make_test_tls_credentials(root);
      run_tls_case(node,extended_scores(8),4,UINT64_C(0x2026100908),tls,
                   "",false,"",false,"",false,"",false,false,"",false,false,false,false,true);
      return 0;
    }
    require(argc == 2 || std::string(argv[2]) == "--extended" ||
            std::string(argv[2]) == "--small-ranks" || std::string(argv[2]) == "--n1000",
            "unknown party node E2E option");
    run_case(node, {INT32_MIN}, 1, UINT64_C(0x171700));
    run_case(node, {INT32_MIN, INT32_MAX}, 1, UINT64_C(0x171702));
    run_case(node, {INT32_MIN, INT32_MAX}, 1, UINT64_C(0x1717e0), "missing");
    run_case(node, {INT32_MIN, INT32_MAX}, 1, UINT64_C(0x1717e1), "truncate");
    run_case(node, {INT32_MIN, INT32_MAX}, 1, UINT64_C(0x1717e2), "tamper");
    run_case(node, {INT32_MIN, 13, -5, INT32_MAX, 13}, 2, UINT64_C(0x171703));
    run_case(node, {INT32_MIN, 19, -4, 19, INT32_MAX, 1, -99, 8}, 3, UINT64_C(0x171701));
    verify_publication_failures(node);
    if (argc == 3) {
      if (std::string(argv[2]) == "--extended") {
        run_case(node, extended_scores(64), 8, UINT64_C(0x171740));
        run_case(node, seeded_scores(64, UINT64_C(0x18D1)), 1, UINT64_C(0x171741));
        run_case(node, seeded_scores(64, UINT64_C(0x18D2)), 64, UINT64_C(0x171742));
        run_case(node, extended_scores(128), 8, UINT64_C(0x171780));
        run_case(node, seeded_scores(128, UINT64_C(0x18D4)), 1, UINT64_C(0x171782));
        run_case(node, seeded_scores(128, UINT64_C(0x18D3)), 80, UINT64_C(0x171781));
        run_case(node, extended_scores(128), 128, UINT64_C(0x171783));
        run_case(node, seeded_scores(256, UINT64_C(0x19D1)), 1, UINT64_C(0x1718f1));
        run_case(node, extended_scores(256), 2, UINT64_C(0x171800));
        run_case(node, seeded_scores(256, UINT64_C(0x19D2)), 256, UINT64_C(0x1718ff));
      } else if (std::string(argv[2]) == "--small-ranks") {
        for (std::uint32_t n = 2; n <= 8; ++n) {
          std::vector<std::int32_t> scores(n);
          for (std::uint32_t i = 0; i < n; ++i)
            scores[i] = static_cast<std::int32_t>((i * 5U) % 7U) - 3;
          scores.front() = INT32_MIN;
          if (n > 1) scores.back() = INT32_MAX;
          if (n >= 5) scores[2] = scores[1];
          for (std::uint32_t k = 1; k <= n; ++k) {
            const std::uint64_t session = UINT64_C(0x25250000) +
                static_cast<std::uint64_t>(n) * 256U + k;
            run_case(node, scores, k, session);
          }
        }
      } else {
        run_case(node, seeded_scores(1000, UINT64_C(0x18D1000)), 80, UINT64_C(0x1711000));
      }
    }
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "BMW16 experimental independent party-node E2E failed: " << e.what() << '\n';
    return 1;
  }
}
