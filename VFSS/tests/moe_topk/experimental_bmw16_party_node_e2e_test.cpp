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
  const auto targs = std::vector<std::string>{node, "t", std::to_string(n), std::to_string(k),
      std::to_string(session), std::to_string(fingerprint), (p0/"claims").string(), (p1/"claims").string(),
      (p0/"material.shell").string(), (p1/"material.shell").string(),
      (p0/"ucmp.stream").string(), (p1/"ucmp.stream").string(), (p0/"aead.key").string(),
      (p1/"aead.key").string(), std::to_string(kUid0), std::to_string(kUid1),
      (root/"pair.ready").string()};
  const int t_exit = exec_capture(targs, root / "t.log");
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
    auto args = std::vector<std::string>{node, "t", "2", "1", std::to_string(session),
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
    require(argc == 2 || argc == 3, "pass party node executable path [--extended|--n1000]");
    require(argc == 2 || std::string(argv[2]) == "--extended" || std::string(argv[2]) == "--n1000",
            "unknown party node E2E option");
    const auto node = fs::canonical(argv[1]).string();
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
        run_case(node, seeded_scores(128, UINT64_C(0x18D3)), 80, UINT64_C(0x171781));
        run_case(node, seeded_scores(256, UINT64_C(0x19D1)), 1, UINT64_C(0x1718f1));
        run_case(node, extended_scores(256), 2, UINT64_C(0x171800));
        run_case(node, seeded_scores(256, UINT64_C(0x19D2)), 256, UINT64_C(0x1718ff));
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
