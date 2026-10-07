#include <moe_topk/topk_oracle.h>

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
void require(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }

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
int exec_capture(const std::vector<std::string>& args, const fs::path& output) {
  const auto pid = ::fork(); require(pid >= 0, "fork child");
  if (pid == 0) {
    const int fd = ::open(output.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    if (fd < 0 || ::dup2(fd, STDOUT_FILENO) < 0 || ::dup2(fd, STDERR_FILENO) < 0) ::_exit(126);
    ::close(fd);
    std::vector<char*> av; av.reserve(args.size() + 1);
    for (const auto& arg : args) av.push_back(const_cast<char*>(arg.c_str()));
    av.push_back(nullptr); ::execv(av[0], av.data()); ::_exit(127);
  }
  return wait_status(pid);
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
              std::uint64_t session) {
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
  const auto targs = std::vector<std::string>{node, "t", std::to_string(n), std::to_string(k),
      std::to_string(session), std::to_string(fingerprint), (p0/"claims").string(), (p1/"claims").string(),
      (p0/"material.shell").string(), (p1/"material.shell").string(),
      (p0/"ucmp.stream").string(), (p1/"ucmp.stream").string(), (p0/"aead.key").string(),
      (p1/"aead.key").string(), std::to_string(kUid0), std::to_string(kUid1)};
  const int t_exit = exec_capture(targs, root / "t.log");
  require(t_exit == 0, "offline T node failed");
  const auto t_log_bytes = read_all(root / "t.log");
  const std::string t_log(t_log_bytes.begin(), t_log_bytes.end());
  const auto expected_t_status = n == 1 ? "status=SINGLETON_NO_MATERIAL" : "status=OFFLINE_MATERIAL_READY";
  require(t_log.find(expected_t_status) != std::string::npos &&
          t_log.find("T_online=false") != std::string::npos, "T did not finish offline material role");
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
          (dir/"mask.share").string(), "30000"};
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
  if (e0 == 0) {
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
            << " status=" << (e0 == 0 ? "SUCCESS_ORACLE_CHECKED" : e0 == 10 ? "AGREED_ALGORITHM_ABORT" :
                e0 == 20 ? "MATERIAL_ABORT" : "COMMUNICATION_ABORT")
            << " p0_exit=" << e0 << " p1_exit=" << e1 << " T_online=false\n";
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
    require(argc == 2 || argc == 3, "pass party node executable path [--extended|--n1000]");
    require(argc == 2 || std::string(argv[2]) == "--extended" || std::string(argv[2]) == "--n1000",
            "unknown party node E2E option");
    const auto node = fs::canonical(argv[1]).string();
    run_case(node, {INT32_MIN}, 1, UINT64_C(0x171700));
    run_case(node, {INT32_MIN, INT32_MAX}, 1, UINT64_C(0x171702));
    run_case(node, {INT32_MIN, 13, -5, INT32_MAX, 13}, 2, UINT64_C(0x171703));
    run_case(node, {INT32_MIN, 19, -4, 19, INT32_MAX, 1, -99, 8}, 3, UINT64_C(0x171701));
    if (argc == 3) {
      if (std::string(argv[2]) == "--extended") {
        run_case(node, extended_scores(64), 8, UINT64_C(0x171740));
        run_case(node, seeded_scores(64, UINT64_C(0x18D1)), 1, UINT64_C(0x171741));
        run_case(node, seeded_scores(64, UINT64_C(0x18D2)), 64, UINT64_C(0x171742));
        run_case(node, extended_scores(128), 8, UINT64_C(0x171780));
        run_case(node, seeded_scores(128, UINT64_C(0x18D3)), 80, UINT64_C(0x171781));
        run_case(node, extended_scores(256), 2, UINT64_C(0x171800));
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
