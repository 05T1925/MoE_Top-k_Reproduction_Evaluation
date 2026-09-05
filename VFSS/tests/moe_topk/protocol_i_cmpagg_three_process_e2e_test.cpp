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
#include <cstdlib>
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
constexpr std::size_t kMaxMessageBytes = 64U * 1024U * 1024U;

struct CaseConfig {
  std::uint32_t n = 0;
  std::uint32_t k = 0;
  int bits = 0;
  std::uint64_t session = 0;
  std::uint64_t fingerprint = 0;
  std::uint64_t p2_seed = 0;
  std::uint64_t input_seed = 0;
};

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

struct PartyResult {
  std::vector<std::uint64_t> ranks;
  std::uint64_t sent_bytes = 0;
  std::uint64_t received_bytes = 0;
};

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::size_t edge_count(const CaseConfig& config) {
  require(config.n > 0, "case node count");
  const auto nodes = static_cast<std::size_t>(config.n);
  return nodes * (nodes - 1U) / 2U;
}

void put_u64(Bytes& bytes, std::uint64_t value) {
  for (int shift = 56; shift >= 0; shift -= 8) {
    bytes.push_back(static_cast<std::uint8_t>(value >> shift));
  }
}

void put_u64_at(Bytes& bytes, std::size_t offset, std::uint64_t value) {
  require(offset <= bytes.size() && bytes.size() - offset >= sizeof(value),
          "test byte mutation offset");
  for (int shift = 56; shift >= 0; shift -= 8) {
    bytes[offset++] = static_cast<std::uint8_t>(value >> shift);
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

std::uint64_t parse_u64(const char* text) {
  char* end = nullptr;
  errno = 0;
  const auto value = std::strtoull(text, &end, 10);
  require(errno == 0 && end != text && *end == '\0', "invalid case integer");
  return static_cast<std::uint64_t>(value);
}

CaseConfig parse_case(int argc, char** argv, int start) {
  require(argc == start + 8 && std::strcmp(argv[start], "--case") == 0,
          "case arguments");
  const auto n = parse_u64(argv[start + 1]);
  const auto k = parse_u64(argv[start + 2]);
  const auto bits = parse_u64(argv[start + 3]);
  require(n <= std::numeric_limits<std::uint32_t>::max() &&
              k <= std::numeric_limits<std::uint32_t>::max() &&
              bits <= std::numeric_limits<int>::max(),
          "case argument range");
  CaseConfig config{static_cast<std::uint32_t>(n), static_cast<std::uint32_t>(k),
                    static_cast<int>(bits), parse_u64(argv[start + 4]),
                    parse_u64(argv[start + 5]), parse_u64(argv[start + 6]),
                    parse_u64(argv[start + 7])};
  require(config.n > 0 && config.k > 0 && config.k <= config.n && config.bits >= 34 &&
              config.bits <= 53 && config.session != 0 && config.fingerprint != 0,
          "case configuration");
  return config;
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

void read_exact(int fd, std::uint8_t* data, std::size_t size) {
  while (size != 0) {
    wait_for_io(fd, POLLIN);
    const auto read_count = ::read(fd, data, size);
    if (read_count < 0 && errno == EINTR) {
      continue;
    }
    require(read_count > 0, "test channel truncated");
    data += read_count;
    size -= static_cast<std::size_t>(read_count);
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
  require(input.n > 0 && input.k > 0 && input.k <= input.n && input.bits >= 34 &&
              input.bits <= 53 && input.keys.size() == input.n,
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
  require(input.n > 0 && input.k > 0 && input.k <= input.n && input.bits >= 34 &&
              input.bits <= 53, "online input binding");
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

Bytes serialize_result(const CaseConfig& config, int party, const std::vector<std::uint64_t>& ranks,
                       std::uint64_t sent_bytes, std::uint64_t received_bytes) {
  require(party == 0 || party == 1, "invalid result party");
  require(ranks.size() == config.n, "invalid result length");
  Bytes bytes = {'M', '2', 'R', 'S', 1, static_cast<std::uint8_t>(party),
                 static_cast<std::uint8_t>(config.bits), 0};
  put_u64(bytes, config.session);
  put_u64(bytes, config.fingerprint);
  put_u64(bytes, config.n);
  put_u64(bytes, config.k);
  put_u64(bytes, sent_bytes);
  put_u64(bytes, received_bytes);
  for (const auto rank : ranks) {
    put_u64(bytes, rank);
  }
  return bytes;
}

PartyResult deserialize_result(const Bytes& bytes, const CaseConfig& config, int expected_party) {
  require(bytes.size() == 56 + sizeof(std::uint64_t) * config.n && bytes[0] == 'M' &&
              bytes[1] == '2' && bytes[2] == 'R' && bytes[3] == 'S' && bytes[4] == 1 &&
              bytes[5] == expected_party && bytes[6] == config.bits && bytes[7] == 0,
          "result header");
  std::size_t offset = 8;
  require(get_u64(bytes, offset, "result session") == config.session &&
              get_u64(bytes, offset, "result fingerprint") == config.fingerprint &&
              get_u64(bytes, offset, "result n") == config.n &&
              get_u64(bytes, offset, "result k") == config.k,
          "result binding");
  PartyResult result;
  result.sent_bytes = get_u64(bytes, offset, "result sent bytes");
  result.received_bytes = get_u64(bytes, offset, "result received bytes");
  result.ranks.resize(config.n);
  for (auto& rank : result.ranks) {
    rank = get_u64(bytes, offset, "result rank");
  }
  require(offset == bytes.size(), "result trailing bytes");
  return result;
}

void make_package(const CaseConfig& config, ProtocolIPartyPackage& package, int party,
                  const std::vector<std::uint64_t>& masks) {
  package.session = config.session;
  package.fingerprint = config.fingerprint;
  package.party = party;
  package.comparison_bits = config.bits;
  package.n = config.n;
  package.k = config.k;
  package.node_mask_shares = masks;
}

int run_p2(const CaseConfig& config, int p2_p0_fd, int p2_p1_fd, int stats_fd) {
  close_except({p2_p0_fd, p2_p1_fd, stats_fd});
  try {
    seed_fss();
    const auto ring = (UINT64_C(1) << config.bits) - 1U;
    std::mt19937_64 rng(config.p2_seed);
    std::vector<std::uint64_t> full_masks(config.n);
    std::vector<std::uint64_t> mask0(config.n);
    std::vector<std::uint64_t> mask1(config.n);
    for (std::size_t index = 0; index < config.n; ++index) {
      full_masks[index] = rng() & ring;
      mask0[index] = rng() & ring;
      mask1[index] = (full_masks[index] - mask0[index]) & ring;
      require(((mask0[index] + mask1[index]) & ring) == full_masks[index],
              "node mask share mismatch");
    }

    ProtocolIPartyPackage package0;
    ProtocolIPartyPackage package1;
    make_package(config, package0, 0, mask0);
    make_package(config, package1, 1, mask1);
    for (std::uint32_t left = 0; left < config.n; ++left) {
      for (std::uint32_t right = left + 1; right < config.n; ++right) {
        ProtocolIUcmpMaterial material(config.bits, full_masks[left], full_masks[right]);
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

int run_party(const CaseConfig& config, int party, int package_fd, int input_fd, int peer_fd,
              int result_fd) {
  close_except({package_fd, input_fd, peer_fd, result_fd});
  try {
    const auto package_bytes = read_until_eof(package_fd);
    auto package = deserialize_party_package(package_bytes, party);
    const auto input = deserialize_online_input(read_until_eof(input_fd), party);
    require(package.session == config.session && package.fingerprint == config.fingerprint &&
                input.session == config.session && input.fingerprint == config.fingerprint &&
                package.n == config.n && package.k == config.k && package.n == input.n &&
                package.k == input.k && package.comparison_bits == config.bits &&
                package.comparison_bits == input.bits,
            "party package/input mismatch");

    std::vector<std::uint64_t> masked_keys(package.n);
    for (std::size_t index = 0; index < package.n; ++index) {
      masked_keys[index] = protocol_i_mask_priority_key_share(
          input.bits, input.keys[index], package.node_mask_shares[index]);
    }
    ProtocolIFrameConfig frame_config{config.session,
                                      config.fingerprint,
                                      config.n,
                                      config.k,
                                      static_cast<std::uint8_t>(config.bits),
                                      static_cast<std::uint8_t>(party),
                                      static_cast<std::uint8_t>(1 - party),
                                      1,
                                      1};
    ProtocolIFramedChannel channel(peer_fd, frame_config, 2000);
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
    require(edge_materials.size() == edge_count(config) && rank_shares.size() == config.n,
            "party material consumption");
    const auto result = serialize_result(config, party, rank_shares, channel.sent_bytes(),
                                         channel.received_bytes());
    write_bytes(result_fd, result);
    if (party == 1) {
      require(::shutdown(result_fd, SHUT_WR) == 0, "P1 result shutdown");
      std::uint8_t acknowledgement = 0;
      read_exact(result_fd, &acknowledgement, 1);
      require(acknowledgement == 1, "P1 result acknowledgement");
    }
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

int run_controller(const CaseConfig& config, const std::vector<std::uint32_t>& scores,
                   const std::string& executable, bool report) {
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
    const auto case_args = [&config]() {
      return std::vector<std::string>{"--case", std::to_string(config.n), std::to_string(config.k),
                                      std::to_string(config.bits), std::to_string(config.session),
                                      std::to_string(config.fingerprint),
                                      std::to_string(config.p2_seed),
                                      std::to_string(config.input_seed)};
    };
    auto p0_args = std::vector<std::string>{"--role", "p0", "--package-fd", std::to_string(p2_p0[1]),
                                 "--input-fd", std::to_string(controller_p0[1]), "--peer-fd",
                                 std::to_string(p0_p1[0]), "--result-fd", std::to_string(p0_result[0])};
    auto p1_args = std::vector<std::string>{"--role", "p1", "--package-fd", std::to_string(p2_p1[1]),
                                 "--input-fd", std::to_string(controller_p1[1]), "--peer-fd",
                                 std::to_string(p0_p1[1]), "--result-fd", std::to_string(p1_result[0])};
    auto p2_args = std::vector<std::string>{"--role", "p2", "--p2-p0-fd", std::to_string(p2_p0[0]),
                                 "--p2-p1-fd", std::to_string(p2_p1[0]), "--stats-fd",
                                 std::to_string(p2_stats[0])};
    const auto extra = case_args();
    p0_args.insert(p0_args.end(), extra.begin(), extra.end());
    p1_args.insert(p1_args.end(), extra.begin(), extra.end());
    p2_args.insert(p2_args.end(), extra.begin(), extra.end());
    p0 = spawn_role(executable, p0_args);
    p1 = spawn_role(executable, p1_args);
    p2 = spawn_role(executable, p2_args);

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
    require(stats.edge_count == edge_count(config), "P2 edge count");
    require(stats.p0_package_bytes > 40 && stats.p1_package_bytes > 40,
            "P2 package byte counters");

    require(scores.size() == config.n, "score case length");
    const auto oracle_ranks = stable_ranks_cmpagg(scores);
    const auto oracle_mask = top_k_mask(scores, config.k);
    const auto ring = (UINT64_C(1) << config.bits) - 1U;
    std::mt19937_64 rng(config.input_seed);
    InputShares shares[2];
    for (auto& input : shares) {
      input.session = config.session;
      input.fingerprint = config.fingerprint;
      input.n = config.n;
      input.k = config.k;
      input.bits = config.bits;
      input.keys.resize(config.n);
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

    const auto result0 = deserialize_result(read_until_eof(p0_result[1]), config, 0);
    const auto result1_bytes = read_until_eof(p1_result[1]);
    const std::uint8_t acknowledgement = 1;
    require(::write(p1_result[1], &acknowledgement, 1) == 1, "P1 result acknowledgement");
    wait_success(p1, "P1 did not exit normally");
    p1 = -1;
    wait_success(p0, "P0 did not exit normally");
    p0 = -1;
    const auto result1 = deserialize_result(result1_bytes, config, 1);
    require(result0.ranks.size() == oracle_ranks.size() && result1.ranks.size() == oracle_ranks.size(),
            "rank result length");
    std::vector<std::uint64_t> ranks(result0.ranks.size());
    for (std::size_t index = 0; index < ranks.size(); ++index) {
      ranks[index] = result0.ranks[index] + result1.ranks[index];
    }
    require(ranks == oracle_ranks, "CmpAgg rank oracle mismatch");
    std::vector<std::uint64_t> sorted_ranks = ranks;
    std::sort(sorted_ranks.begin(), sorted_ranks.end());
    for (std::size_t index = 0; index < sorted_ranks.size(); ++index) {
      require(sorted_ranks[index] == index, "CmpAgg ranks are not a permutation");
    }
    require(top_k_mask_from_stable_ranks(ranks, config.k) == oracle_mask, "Top-K oracle mismatch");

    if (report) {
      std::cout << "implementation_label=m2_priority_cmpagg_three_process_e2e\n"
                << "n=" << config.n << " K=" << config.k << " comparison_bits=" << config.bits << '\n'
                << "seed_p2=" << config.p2_seed << " seed_input=" << config.input_seed << '\n'
                << "p2_before_online_input=true edge_count=" << stats.edge_count << '\n'
                << "p2_to_p0_bytes=" << stats.p0_package_bytes
                << " p2_to_p1_bytes=" << stats.p1_package_bytes << '\n'
                << "p0_to_p1_bytes=" << result0.sent_bytes
                << " p1_to_p0_bytes=" << result1.sent_bytes << '\n'
                << "test_only_result_bytes=" << (56 + sizeof(std::uint64_t) * config.n) * 2 << '\n'
                << "oracle_ranks=";
      for (std::size_t index = 0; index < oracle_ranks.size(); ++index) {
        if (index != 0) std::cout << ',';
        std::cout << oracle_ranks[index];
      }
      std::cout << " mask=";
      for (std::size_t index = 0; index < oracle_mask.size(); ++index) {
        if (index != 0) std::cout << ',';
        std::cout << static_cast<int>(oracle_mask[index]);
      }
      std::cout << '\n'
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

void run_negative_package_checks(const std::string& executable) {
  seed_fss();
  const CaseConfig config{3, 2, 34, 0x4d32ff01, 0x6d32ff01, 0x3001, 0x4001};
  ProtocolIPartyPackage package;
  make_package(config, package, 0, {1, 2, 3});
  for (std::uint32_t left = 0; left < config.n; ++left) {
    for (std::uint32_t right = left + 1; right < config.n; ++right) {
      ProtocolIUcmpMaterial material(config.bits, left + 1U, right + 1U);
      package.edge_materials.emplace_back(left, right, material.export_party_material(0));
    }
  }
  const auto valid = serialize_party_package(package);
  auto expect_throw = [](const auto& function, const char* message) {
    bool threw = false;
    try {
      function();
    } catch (...) {
      threw = true;
    }
    require(threw, message);
  };

  ProtocolIPartyPackage invalid_n;
  invalid_n.party = 0;
  invalid_n.n = 0;
  invalid_n.k = 1;
  invalid_n.comparison_bits = config.bits;
  expect_throw([&] { (void)serialize_party_package(invalid_n); }, "n=0 rejection");
  ProtocolIPartyPackage invalid_k0;
  make_package(config, invalid_k0, 0, {1, 2, 3});
  invalid_k0.k = 0;
  expect_throw([&] { (void)serialize_party_package(invalid_k0); }, "K=0 rejection");
  ProtocolIPartyPackage invalid_klarger;
  make_package(config, invalid_klarger, 0, {1, 2, 3});
  invalid_klarger.k = config.n + 1U;
  expect_throw([&] { (void)serialize_party_package(invalid_klarger); }, "K>n rejection");
  expect_throw([&] { (void)deserialize_party_package(valid, 1); }, "wrong party rejection");

  auto wrong_bits = valid;
  wrong_bits[6] = static_cast<std::uint8_t>(config.bits - 1);
  expect_throw([&] { (void)deserialize_party_package(wrong_bits, 0); },
               "wrong comparison bits rejection");
  auto trailing = valid;
  trailing.push_back(0);
  expect_throw([&] { (void)deserialize_party_package(trailing, 0); },
               "trailing package bytes rejection");
  auto truncated = valid;
  truncated.pop_back();
  expect_throw([&] { (void)deserialize_party_package(truncated, 0); },
               "truncated package rejection");

  std::size_t offset = 8 + 4 * sizeof(std::uint64_t) + config.n * sizeof(std::uint64_t);
  const auto first_edge_offset = offset;
  auto first_length_offset = offset + 16;
  const auto first_material_length = get_u64(valid, first_length_offset, "negative package edge");
  offset += 24 + static_cast<std::size_t>(first_material_length);
  const auto second_edge_offset = offset;
  auto duplicate = valid;
  put_u64_at(duplicate, second_edge_offset, 0);
  put_u64_at(duplicate, second_edge_offset + 8, 1);
  expect_throw([&] { (void)deserialize_party_package(duplicate, 0); },
               "duplicate edge rejection");
  auto reordered = valid;
  put_u64_at(reordered, second_edge_offset, 1);
  put_u64_at(reordered, second_edge_offset + 8, 2);
  expect_throw([&] { (void)deserialize_party_package(reordered, 0); },
               "unordered edge rejection");
  auto missing = valid;
  missing.resize(second_edge_offset);
  expect_throw([&] { (void)deserialize_party_package(missing, 0); },
               "missing edge rejection");
  require(first_edge_offset < second_edge_offset, "negative edge offsets");

  const auto child = spawn_role(executable, {"--role", "fail"});
  bool child_failed = false;
  try {
    wait_success(child, "invalid child unexpectedly succeeded");
  } catch (...) {
    child_failed = true;
  }
  require(child_failed, "child nonzero exit propagation");
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc >= 2 && std::strcmp(argv[1], "--role") == 0) {
      require(argc >= 3, "missing child role");
      const std::string role = argv[2];
      if (role == "fail") {
        return 7;
      }
      if (role == "p2") {
        require(argc >= 9 && std::strcmp(argv[3], "--p2-p0-fd") == 0 &&
                    std::strcmp(argv[5], "--p2-p1-fd") == 0 &&
                    std::strcmp(argv[7], "--stats-fd") == 0,
                "P2 arguments");
        const auto config = parse_case(argc, argv, 9);
        return run_p2(config, parse_fd(argv[4]), parse_fd(argv[6]), parse_fd(argv[8]));
      }
      require((role == "p0" || role == "p1") && argc >= 11 &&
                  std::strcmp(argv[3], "--package-fd") == 0 &&
                  std::strcmp(argv[5], "--input-fd") == 0 &&
                  std::strcmp(argv[7], "--peer-fd") == 0 &&
                  std::strcmp(argv[9], "--result-fd") == 0,
              "party arguments");
      const int party = role == "p0" ? 0 : 1;
      const auto config = parse_case(argc, argv, 11);
      return run_party(config, party, parse_fd(argv[4]), parse_fd(argv[6]), parse_fd(argv[8]),
                       parse_fd(argv[10]));
    }
    require(argc == 1 || (argc == 2 && std::strcmp(argv[1], "--report") == 0),
            "controller arguments");
    const auto executable = self_executable(argv[0]);
    const bool report = argc == 2;
    run_negative_package_checks(executable);
    const std::vector<CaseConfig> cases = {
        {1, 1, 34, 0x4d320001, 0x6d320001, 0x1001, 0x2001},
        {2, 1, 34, 0x4d320002, 0x6d320002, 0x1002, 0x2002},
        {2, 2, 34, 0x4d320003, 0x6d320003, 0x1003, 0x2003},
        {5, 1, 36, 0x4d320005, 0x6d320005, 0x1005, 0x2005},
        {5, 5, 36, 0x4d320006, 0x6d320006, 0x1006, 0x2006},
        {7, 1, 36, 0x4d320007, 0x6d320007, 0x1007, 0x2007},
        {7, 7, 36, 0x4d320008, 0x6d320008, 0x1008, 0x2008},
        {11, 1, 40, 0x4d32000b, 0x6d32000b, 0x100b, 0x200b},
        {11, 2, 40, 0x4d32000c, 0x6d32000c, 0x100c, 0x200c},
        {11, 11, 40, 0x4d32000d, 0x6d32000d, 0x100d, 0x200d},
    };
    const std::vector<std::vector<std::uint32_t>> scores = {
        {0},
        {static_cast<std::uint32_t>(INT32_MIN), static_cast<std::uint32_t>(INT32_MAX)},
        {7, 7},
        {5, 5, 1, 5, 3},
        {static_cast<std::uint32_t>(INT32_MAX), 0, static_cast<std::uint32_t>(INT32_MIN),
         0, static_cast<std::uint32_t>(INT32_MAX)},
        {static_cast<std::uint32_t>(INT32_MIN), 4, 4, static_cast<std::uint32_t>(INT32_MAX),
         3, 2, 1},
        {9, 9, 9, 9, 9, 9, 9},
        {UINT32_C(0x80000000), 11, UINT32_C(0x7fffffff), 42, 11, 0, 42, 3, 8, 8, 19},
        {UINT32_C(0x80000000), 11, UINT32_C(0x7fffffff), 42, 11, 0, 42, 3, 8, 8, 19},
        {6, 6, 6, 2, 2, 0, 0, 1, 1, 3, 3},
    };
    require(cases.size() == scores.size(), "case matrix");
    for (std::size_t index = 0; index < cases.size(); ++index) {
      run_controller(cases[index], scores[index], executable, report);
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
