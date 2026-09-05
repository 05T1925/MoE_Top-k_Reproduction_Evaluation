// TEST_ONLY: independent P2/P0/P1 process harness for the M2 CmpAgg foundation.
#include <moe_topk/protocol_i_cmpagg.h>
#include <moe_topk/protocol_i_party_package.h>
#include <moe_topk/protocol_i_priority_key.h>
#include <moe_topk/topk_oracle.h>
#include <moe_topk/protocol_i_transport.h>
#include <FSS/prng.h>

#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <limits>
#include <poll.h>
#include <random>
#include <stdexcept>
#include <string>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {

using Bytes = std::vector<std::uint8_t>;
using namespace moe_topk;

constexpr int kIoTimeoutMs = 5000;
constexpr std::uint64_t kSession = UINT64_C(0x4d325f4543440001);
constexpr std::uint64_t kFingerprint = UINT64_C(0x6d325f7072696f72);
constexpr std::uint32_t kN = 5;
constexpr std::uint32_t kK = 2;
constexpr int kBits = 36;
constexpr std::size_t kMaxMessageBytes = 64U * 1024U * 1024U;

struct InputShares {
  std::uint64_t session = 0;
  std::uint64_t fingerprint = 0;
  std::uint32_t n = 0;
  std::uint32_t k = 0;
  int bits = 0;
  std::vector<std::uint64_t> keys;
};

struct Stats {
  std::uint64_t p0_package_bytes = 0;
  std::uint64_t p1_package_bytes = 0;
  std::uint64_t edge_count = 0;
};

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void put_u64(Bytes& bytes, std::uint64_t value) {
  for (int shift = 56; shift >= 0; shift -= 8) {
    bytes.push_back(static_cast<std::uint8_t>(value >> shift));
  }
}

std::uint64_t get_u64(const Bytes& bytes, std::size_t& offset, const char* message) {
  require(offset <= bytes.size() && bytes.size() - offset >= sizeof(std::uint64_t), message);
  std::uint64_t value = 0;
  for (int index = 0; index < 8; ++index) {
    value = (value << 8U) | bytes[offset++];
  }
  return value;
}

int parse_fd(const char* text) {
  char* end = nullptr;
  const auto value = std::strtol(text, &end, 10);
  require(end != text && *end == '\0' && value >= 0 && value <= std::numeric_limits<int>::max(),
          "invalid inherited fd");
  return static_cast<int>(value);
}

void wait_for_io(int fd, short events) {
  pollfd descriptor{fd, events, 0};
  const int result = ::poll(&descriptor, 1, kIoTimeoutMs);
  require(result > 0, "test channel timeout");
  require((descriptor.revents & (POLLERR | POLLNVAL)) == 0, "test channel poll error");
}

void write_all(int fd, const std::uint8_t* data, std::size_t size) {
  while (size != 0) {
    wait_for_io(fd, POLLOUT);
    const auto written = ::write(fd, data, size);
    if (written < 0 && errno == EINTR) {
      continue;
    }
    require(written > 0, "test channel write failed");
    data += written;
    size -= static_cast<std::size_t>(written);
  }
}

void write_bytes(int fd, const Bytes& bytes) {
  if (!bytes.empty()) {
    write_all(fd, bytes.data(), bytes.size());
  }
}

Bytes read_until_eof(int fd) {
  Bytes bytes;
  std::uint8_t chunk[4096];
  for (;;) {
    wait_for_io(fd, POLLIN);
    const auto read_count = ::read(fd, chunk, sizeof(chunk));
    if (read_count < 0 && errno == EINTR) {
      continue;
    }
    if (read_count == 0) {
      return bytes;
    }
    require(read_count > 0, "test channel read failed");
    require(bytes.size() <= kMaxMessageBytes - static_cast<std::size_t>(read_count),
            "test channel message too large");
    bytes.insert(bytes.end(), chunk, chunk + read_count);
  }
}

void close_fd(int& fd) {
  if (fd >= 0) {
    ::close(fd);
    fd = -1;
  }
}

void close_except(const std::vector<int>& keep) {
  struct rlimit limit{};
  require(::getrlimit(RLIMIT_NOFILE, &limit) == 0, "cannot query fd limit");
  const auto upper = limit.rlim_max == RLIM_INFINITY
                         ? 65536U
                         : static_cast<unsigned long long>(limit.rlim_max);
  for (unsigned long long value = 3; value < upper; ++value) {
    const int fd = static_cast<int>(value);
    bool retained = false;
    for (const auto allowed : keep) {
      if (fd == allowed) {
        retained = true;
        break;
      }
    }
    if (!retained) {
      ::close(fd);
    }
  }
}

void seed_fss() {
  for (int index = 0; index < 256; ++index) {
    FSSConfig::prngs[index].SetSeed(osuCrypto::toBlock(UINT64_C(0x4d325f454344), index));
  }
}

Bytes serialize_online_input(int target_party, const InputShares& input) {
  require(target_party == 0 || target_party == 1, "invalid input target");
  require(input.n == kN && input.k == kK && input.bits == kBits && input.keys.size() == input.n,
          "invalid input shares");
  Bytes bytes = {'M', '2', 'I', 'N', 1, static_cast<std::uint8_t>(target_party),
                 static_cast<std::uint8_t>(input.bits), 0};
  put_u64(bytes, input.session);
  put_u64(bytes, input.fingerprint);
  put_u64(bytes, input.n);
  put_u64(bytes, input.k);
  for (const auto key : input.keys) {
    put_u64(bytes, key);
  }
  return bytes;
}

InputShares deserialize_online_input(const Bytes& bytes, int expected_party) {
  require(bytes.size() >= 40 && bytes[0] == 'M' && bytes[1] == '2' && bytes[2] == 'I' &&
              bytes[3] == 'N' && bytes[4] == 1 && bytes[5] == expected_party && bytes[7] == 0,
          "online input header");
  InputShares input;
  input.bits = bytes[6];
  std::size_t offset = 8;
  input.session = get_u64(bytes, offset, "online input fields");
  input.fingerprint = get_u64(bytes, offset, "online input fields");
  const auto n = get_u64(bytes, offset, "online input n");
  const auto k = get_u64(bytes, offset, "online input k");
  require(n <= std::numeric_limits<std::uint32_t>::max() &&
              k <= std::numeric_limits<std::uint32_t>::max(),
          "online input dimensions");
  input.n = static_cast<std::uint32_t>(n);
  input.k = static_cast<std::uint32_t>(k);
  require(input.n == kN && input.k == kK && input.bits == kBits, "online input binding");
  require(bytes.size() == offset + sizeof(std::uint64_t) * input.n, "online input length");
  input.keys.resize(input.n);
  const auto ring = (UINT64_C(1) << input.bits) - 1U;
  for (auto& key : input.keys) {
    key = get_u64(bytes, offset, "online input key");
    require((key & ~ring) == 0, "online input key outside ring");
  }
  require(offset == bytes.size(), "online input trailing bytes");
  return input;
}

Bytes serialize_stats(const Stats& stats) {
  Bytes bytes = {'M', '2', 'S', 'T', 1, 0, 0, 0};
  put_u64(bytes, stats.p0_package_bytes);
  put_u64(bytes, stats.p1_package_bytes);
  put_u64(bytes, stats.edge_count);
  return bytes;
}

Stats deserialize_stats(const Bytes& bytes) {
  require(bytes.size() == 32 && bytes[0] == 'M' && bytes[1] == '2' && bytes[2] == 'S' &&
              bytes[3] == 'T' && bytes[4] == 1,
          "P2 stats");
  std::size_t offset = 8;
  Stats stats;
  stats.p0_package_bytes = get_u64(bytes, offset, "P2 stats bytes");
  stats.p1_package_bytes = get_u64(bytes, offset, "P2 stats bytes");
  stats.edge_count = get_u64(bytes, offset, "P2 stats edges");
  require(offset == bytes.size(), "P2 stats trailing bytes");
  return stats;
}

Bytes serialize_result(int party, const std::vector<std::uint64_t>& ranks) {
  require(party == 0 || party == 1, "invalid result party");
  require(ranks.size() == kN, "invalid result length");
  Bytes bytes = {'M', '2', 'R', 'S', 1, static_cast<std::uint8_t>(party),
                 static_cast<std::uint8_t>(kBits), 0};
  put_u64(bytes, kSession);
  put_u64(bytes, kFingerprint);
  put_u64(bytes, kN);
  put_u64(bytes, kK);
  for (const auto rank : ranks) {
    put_u64(bytes, rank);
  }
  return bytes;
}

std::vector<std::uint64_t> deserialize_result(const Bytes& bytes, int expected_party) {
  require(bytes.size() == 40 + sizeof(std::uint64_t) * kN && bytes[0] == 'M' &&
              bytes[1] == '2' && bytes[2] == 'R' && bytes[3] == 'S' && bytes[4] == 1 &&
              bytes[5] == expected_party && bytes[6] == kBits && bytes[7] == 0,
          "result header");
  std::size_t offset = 8;
  require(get_u64(bytes, offset, "result session") == kSession &&
              get_u64(bytes, offset, "result fingerprint") == kFingerprint &&
              get_u64(bytes, offset, "result n") == kN && get_u64(bytes, offset, "result k") == kK,
          "result binding");
  std::vector<std::uint64_t> ranks(kN);
  for (auto& rank : ranks) {
    rank = get_u64(bytes, offset, "result rank");
  }
  require(offset == bytes.size(), "result trailing bytes");
  return ranks;
}

void make_package(ProtocolIPartyPackage& package, int party,
                  const std::vector<std::uint64_t>& masks) {
  package.session = kSession;
  package.fingerprint = kFingerprint;
  package.party = party;
  package.comparison_bits = kBits;
  package.n = kN;
  package.k = kK;
  package.node_mask_shares = masks;
}

int run_p2(int p2_p0_fd, int p2_p1_fd, int stats_fd) {
  close_except({p2_p0_fd, p2_p1_fd, stats_fd});
  try {
    seed_fss();
    const auto ring = (UINT64_C(1) << kBits) - 1U;
    std::mt19937_64 rng(UINT64_C(0x4d325f504b325f31));
    std::vector<std::uint64_t> full_masks(kN);
    std::vector<std::uint64_t> mask0(kN);
    std::vector<std::uint64_t> mask1(kN);
    for (std::size_t index = 0; index < kN; ++index) {
      full_masks[index] = rng() & ring;
      mask0[index] = rng() & ring;
      mask1[index] = (full_masks[index] - mask0[index]) & ring;
      require(((mask0[index] + mask1[index]) & ring) == full_masks[index],
              "node mask share mismatch");
    }

    ProtocolIPartyPackage package0;
    ProtocolIPartyPackage package1;
    make_package(package0, 0, mask0);
    make_package(package1, 1, mask1);
    for (std::uint32_t left = 0; left < kN; ++left) {
      for (std::uint32_t right = left + 1; right < kN; ++right) {
        ProtocolIUcmpMaterial material(kBits, full_masks[left], full_masks[right]);
        package0.edge_materials.emplace_back(left, right, material.export_party_material(0));
        package1.edge_materials.emplace_back(left, right, material.export_party_material(1));
      }
    }

    const auto package0_bytes = serialize_party_package(package0);
    const auto package1_bytes = serialize_party_package(package1);
    write_bytes(p2_p0_fd, package0_bytes);
    require(::shutdown(p2_p0_fd, SHUT_WR) == 0, "P2 to P0 shutdown");
    write_bytes(p2_p1_fd, package1_bytes);
    require(::shutdown(p2_p1_fd, SHUT_WR) == 0, "P2 to P1 shutdown");
    const Stats stats{package0_bytes.size(), package1_bytes.size(), package0.edge_materials.size()};
    const auto stats_bytes = serialize_stats(stats);
    write_bytes(stats_fd, stats_bytes);
    close_fd(p2_p0_fd);
    close_fd(p2_p1_fd);
    close_fd(stats_fd);
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "P2: " << error.what() << '\n';
    close_fd(p2_p0_fd);
    close_fd(p2_p1_fd);
    close_fd(stats_fd);
    return 1;
  }
}

int run_party(int party, int package_fd, int input_fd, int peer_fd, int result_fd) {
  close_except({package_fd, input_fd, peer_fd, result_fd});
  try {
    const auto package_bytes = read_until_eof(package_fd);
    auto package = deserialize_party_package(package_bytes, party);
    const auto input = deserialize_online_input(read_until_eof(input_fd), party);
    require(package.session == input.session && package.fingerprint == input.fingerprint &&
                package.n == input.n && package.k == input.k &&
                package.comparison_bits == input.bits,
            "party package/input mismatch");

    std::vector<std::uint64_t> masked_keys(package.n);
    for (std::size_t index = 0; index < package.n; ++index) {
      masked_keys[index] = protocol_i_mask_priority_key_share(
          input.bits, input.keys[index], package.node_mask_shares[index]);
    }
    ProtocolIFrameConfig config{kSession, kFingerprint, kN, kK, static_cast<std::uint8_t>(kBits),
                                static_cast<std::uint8_t>(party), static_cast<std::uint8_t>(1 - party),
                                1, 1};
    ProtocolIFramedChannel channel(peer_fd, config, 2000);
    Bytes local(masked_keys.size() * sizeof(std::uint64_t));
    std::size_t offset = 0;
    for (const auto key : masked_keys) {
      for (int shift = 56; shift >= 0; shift -= 8) {
        local[offset++] = static_cast<std::uint8_t>(key >> shift);
      }
    }
    if (party == 0) {
      channel.send(local);
    }
    const auto peer_bytes = channel.receive();
    require(peer_bytes.size() == masked_keys.size() * sizeof(std::uint64_t),
            "masked-key vector length");
    std::vector<std::uint64_t> peer_keys(masked_keys.size());
    offset = 0;
    for (auto& key : peer_keys) {
      key = get_u64(peer_bytes, offset, "masked-key vector");
    }
    if (party == 1) {
      channel.send(local);
    }

    const auto ring = (UINT64_C(1) << input.bits) - 1U;
    std::vector<std::uint64_t> public_masked_keys(masked_keys.size());
    for (std::size_t index = 0; index < masked_keys.size(); ++index) {
      public_masked_keys[index] = (masked_keys[index] + peer_keys[index]) & ring;
    }
    std::vector<ProtocolIUcmpPartyMaterial> edge_materials;
    edge_materials.reserve(package.edge_materials.size());
    for (auto& edge : package.edge_materials) {
      require(edge.left < edge.right && edge.right < package.n &&
                  edge.material.party_id() == party &&
                  edge.material.comparison_bits() == input.bits,
              "party edge binding");
      edge_materials.push_back(std::move(edge.material));
    }
    const auto rank_shares = protocol_i_cmpagg_eval_party(
        party, input.bits, public_masked_keys, edge_materials);
    require(edge_materials.size() == kN * (kN - 1U) / 2U && rank_shares.size() == kN,
            "party material consumption");
    const auto result = serialize_result(party, rank_shares);
    write_bytes(result_fd, result);
    close_fd(package_fd);
    close_fd(input_fd);
    close_fd(peer_fd);
    close_fd(result_fd);
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "P" << party << ": " << error.what() << '\n';
    close_fd(package_fd);
    close_fd(input_fd);
    close_fd(peer_fd);
    close_fd(result_fd);
    return 1;
  }
}

pid_t spawn_role(const std::string& executable, const std::vector<std::string>& arguments) {
  const auto child = ::fork();
  require(child >= 0, "fork failed");
  if (child == 0) {
    std::vector<char*> argv;
    argv.reserve(arguments.size() + 1);
    argv.push_back(const_cast<char*>(executable.c_str()));
    for (const auto& argument : arguments) {
      argv.push_back(const_cast<char*>(argument.c_str()));
    }
    argv.push_back(nullptr);
    ::execv(executable.c_str(), argv.data());
    _exit(127);
  }
  return child;
}

void make_socketpair(int pair[2]) {
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0, "socketpair failed");
  const int flags0 = ::fcntl(pair[0], F_GETFD);
  const int flags1 = ::fcntl(pair[1], F_GETFD);
  require(flags0 >= 0 && flags1 >= 0 && ::fcntl(pair[0], F_SETFD, flags0 & ~FD_CLOEXEC) == 0 &&
              ::fcntl(pair[1], F_SETFD, flags1 & ~FD_CLOEXEC) == 0,
          "socket fd inheritance setup");
}

void close_pair(int pair[2]) {
  close_fd(pair[0]);
  close_fd(pair[1]);
}

void wait_success(pid_t child, const char* role) {
  int status = 0;
  require(::waitpid(child, &status, 0) == child, "waitpid failed");
  require(WIFEXITED(status) && WEXITSTATUS(status) == 0, role);
}

std::string self_executable(const char* argv0) {
  char path[4096]{};
  const auto length = ::readlink("/proc/self/exe", path, sizeof(path) - 1);
  if (length > 0) {
    path[length] = '\0';
    return path;
  }
  return argv0;
}

int run_controller(const std::string& executable, bool report) {
  int p2_p0[2]{};
  int p2_p1[2]{};
  int controller_p0[2]{};
  int controller_p1[2]{};
  int p0_p1[2]{};
  int p0_result[2]{};
  int p1_result[2]{};
  int p2_stats[2]{};
  make_socketpair(p2_p0);
  make_socketpair(p2_p1);
  make_socketpair(controller_p0);
  make_socketpair(controller_p1);
  make_socketpair(p0_p1);
  make_socketpair(p0_result);
  make_socketpair(p1_result);
  make_socketpair(p2_stats);

  pid_t p0 = -1;
  pid_t p1 = -1;
  pid_t p2 = -1;
  try {
    p0 = spawn_role(executable, {"--role", "p0", "--package-fd", std::to_string(p2_p0[1]),
                                 "--input-fd", std::to_string(controller_p0[1]), "--peer-fd",
                                 std::to_string(p0_p1[0]), "--result-fd", std::to_string(p0_result[0])});
    p1 = spawn_role(executable, {"--role", "p1", "--package-fd", std::to_string(p2_p1[1]),
                                 "--input-fd", std::to_string(controller_p1[1]), "--peer-fd",
                                 std::to_string(p0_p1[1]), "--result-fd", std::to_string(p1_result[0])});
    p2 = spawn_role(executable, {"--role", "p2", "--p2-p0-fd", std::to_string(p2_p0[0]),
                                 "--p2-p1-fd", std::to_string(p2_p1[0]), "--stats-fd",
                                 std::to_string(p2_stats[0])});

    close_fd(p2_p0[0]);
    close_fd(p2_p0[1]);
    close_fd(p2_p1[0]);
    close_fd(p2_p1[1]);
    close_fd(controller_p0[1]);
    close_fd(controller_p1[1]);
    close_fd(p0_p1[0]);
    close_fd(p0_p1[1]);
    close_fd(p0_result[0]);
    close_fd(p1_result[0]);
    close_fd(p2_stats[0]);

    wait_success(p2, "P2 did not exit normally");
    p2 = -1;
    const auto stats = deserialize_stats(read_until_eof(p2_stats[1]));
    require(stats.edge_count == kN * (kN - 1U) / 2U, "P2 edge count");
    require(stats.p0_package_bytes > 40 && stats.p1_package_bytes > 40,
            "P2 package byte counters");

    const std::vector<std::uint32_t> scores = {5, 5, 1, 5, 3};
    const auto oracle_ranks = stable_ranks_cmpagg(scores);
    const auto oracle_mask = top_k_mask(scores, kK);
    const auto ring = (UINT64_C(1) << kBits) - 1U;
    std::mt19937_64 rng(UINT64_C(0x4d325f494e505554));
    InputShares shares[2];
    for (auto& input : shares) {
      input.session = kSession;
      input.fingerprint = kFingerprint;
      input.n = kN;
      input.k = kK;
      input.bits = kBits;
      input.keys.resize(kN);
    }
    for (std::size_t index = 0; index < scores.size(); ++index) {
      const auto key = protocol_i_priority_key(scores[index], index, scores.size()).value;
      shares[0].keys[index] = rng() & ring;
      shares[1].keys[index] = (key - shares[0].keys[index]) & ring;
      require(((shares[0].keys[index] + shares[1].keys[index]) & ring) == key,
              "priority key share mismatch");
    }

    const auto input0 = serialize_online_input(0, shares[0]);
    const auto input1 = serialize_online_input(1, shares[1]);
    write_bytes(controller_p0[0], input0);
    require(::shutdown(controller_p0[0], SHUT_WR) == 0, "controller P0 shutdown");
    write_bytes(controller_p1[0], input1);
    require(::shutdown(controller_p1[0], SHUT_WR) == 0, "controller P1 shutdown");

    wait_success(p0, "P0 did not exit normally");
    p0 = -1;
    wait_success(p1, "P1 did not exit normally");
    p1 = -1;
    const auto rank0 = deserialize_result(read_until_eof(p0_result[1]), 0);
    const auto rank1 = deserialize_result(read_until_eof(p1_result[1]), 1);
    require(rank0.size() == oracle_ranks.size() && rank1.size() == oracle_ranks.size(),
            "rank result length");
    std::vector<std::uint64_t> ranks(rank0.size());
    for (std::size_t index = 0; index < ranks.size(); ++index) {
      ranks[index] = rank0[index] + rank1[index];
    }
    require(ranks == oracle_ranks, "CmpAgg rank oracle mismatch");
    std::vector<std::uint64_t> sorted_ranks = ranks;
    std::sort(sorted_ranks.begin(), sorted_ranks.end());
    for (std::size_t index = 0; index < sorted_ranks.size(); ++index) {
      require(sorted_ranks[index] == index, "CmpAgg ranks are not a permutation");
    }
    require(top_k_mask_from_stable_ranks(ranks, kK) == oracle_mask, "Top-K oracle mismatch");

    if (report) {
      std::cout << "implementation_label=m2_priority_cmpagg_three_process_e2e\n"
                << "n=" << kN << " K=" << kK << " comparison_bits=" << kBits << '\n'
                << "scores=5,5,1,5,3 seed_p2=0x4d325f504b325f31 seed_input=0x4d325f494e505554\n"
                << "p2_before_online_input=true edge_count=" << stats.edge_count << '\n'
                << "p2_to_p0_bytes=" << stats.p0_package_bytes
                << " p2_to_p1_bytes=" << stats.p1_package_bytes << '\n'
                << "p0_to_p1_bytes=NOT_MEASURED p1_to_p0_bytes=NOT_MEASURED\n"
                << "test_only_result_bytes=" << (40 + sizeof(std::uint64_t) * kN) * 2 << '\n'
                << "oracle_ranks=0,1,4,2,3 mask=1,1,0,0,0\n"
                << "process_exit_codes=p2:0,p0:0,p1:0\n";
    }

    close_fd(controller_p0[0]);
    close_fd(controller_p1[0]);
    close_fd(p0_result[1]);
    close_fd(p1_result[1]);
    close_fd(p2_stats[1]);
    return 0;
  } catch (...) {
    if (p0 > 0) {
      ::kill(p0, SIGKILL);
      ::waitpid(p0, nullptr, 0);
    }
    if (p1 > 0) {
      ::kill(p1, SIGKILL);
      ::waitpid(p1, nullptr, 0);
    }
    if (p2 > 0) {
      ::kill(p2, SIGKILL);
      ::waitpid(p2, nullptr, 0);
    }
    close_pair(p2_p0);
    close_pair(p2_p1);
    close_pair(controller_p0);
    close_pair(controller_p1);
    close_pair(p0_p1);
    close_pair(p0_result);
    close_pair(p1_result);
    close_pair(p2_stats);
    throw;
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc >= 2 && std::strcmp(argv[1], "--role") == 0) {
      require(argc >= 3, "missing child role");
      const std::string role = argv[2];
      if (role == "p2") {
        require(argc == 9 && std::strcmp(argv[3], "--p2-p0-fd") == 0 &&
                    std::strcmp(argv[5], "--p2-p1-fd") == 0 &&
                    std::strcmp(argv[7], "--stats-fd") == 0,
                "P2 arguments");
        return run_p2(parse_fd(argv[4]), parse_fd(argv[6]), parse_fd(argv[8]));
      }
      require((role == "p0" || role == "p1") && argc == 11 &&
                  std::strcmp(argv[3], "--package-fd") == 0 &&
                  std::strcmp(argv[5], "--input-fd") == 0 &&
                  std::strcmp(argv[7], "--peer-fd") == 0 &&
                  std::strcmp(argv[9], "--result-fd") == 0,
              "party arguments");
      const int party = role == "p0" ? 0 : 1;
      return run_party(party, parse_fd(argv[4]), parse_fd(argv[6]), parse_fd(argv[8]),
                       parse_fd(argv[10]));
    }
    require(argc == 1 || (argc == 2 && std::strcmp(argv[1], "--report") == 0),
            "controller arguments");
    return run_controller(self_executable(argv[0]), argc == 2);
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
