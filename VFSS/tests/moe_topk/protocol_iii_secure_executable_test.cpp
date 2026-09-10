// TEST_ONLY: Dealer, Party 0 and Party 1 independent-process harness for
// the M3 Protocol III modular three-round engineering baseline.

#include <moe_topk/masked_mul_adapter.h>
#include <moe_topk/protocol_i_party_package.h>
#include <moe_topk/protocol_i_priority_key.h>
#include <moe_topk/protocol_i_ucmp.h>
#include <moe_topk/protocol_iii_dpf_routing.h>
#include <moe_topk/protocol_iii_grank.h>
#include <moe_topk/protocol_iii_metrics_record.h>
#include <moe_topk/protocol_iii_secure_combine.h>
#include <moe_topk/topk_oracle.h>

#include <FSS/comms.h>
#include <FSS/dpf.h>
#include <FSS/prng.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <poll.h>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef MOE_TOPK_PROTOCOL_III_EXECUTABLE
#error "MOE_TOPK_PROTOCOL_III_EXECUTABLE is not defined"
#endif

#ifndef MOE_TOPK_GIT_REVISION
#define MOE_TOPK_GIT_REVISION "unknown"
#endif

#ifndef MOE_TOPK_BUILD_TYPE
#define MOE_TOPK_BUILD_TYPE "unknown"
#endif

namespace {

using namespace moe_topk;
using Bytes = std::vector<std::uint8_t>;

constexpr int kTimeoutMs = 15000;

// Dealer preprocessing for padded_n=256 is intentionally much heavier than
// one online protocol round. Parties may therefore wait longer for the
// TEST_ONLY offline bundle without weakening the 15-second timeout used by
// GRank, DPF routing, or secure combine.
constexpr int kOfflineMaterialTimeoutMs = 300000;

constexpr int kDpfPayloadBits = 64;
constexpr const char* kProtocolLabel =
    "agarwal_protocol_iii_modular_3round";
constexpr std::size_t kMaximumMessageBytes =
    64U * 1024U * 1024U;
constexpr std::size_t kReportMetricCount = 14U;

struct TestCase {
  std::vector<std::uint32_t> scores;
  std::uint32_t k = 0;
  std::uint64_t session = 0;
  std::uint64_t fingerprint = 0;
  std::uint64_t seed = 0;
};

struct PriorityKeyShares {
  std::vector<std::uint64_t> party0;
  std::vector<std::uint64_t> party1;
};

struct OfflineBundle {
  ProtocolIPartyPackage grank_package;
  ProtocolIIIDpfRoutingPartyMaterial routing_material;
  ProtocolIIISecureCombinePartyMaterial combine_material;
  std::vector<std::uint64_t> unit_payload_shares;
};

struct PartyReport {
  std::vector<std::uint8_t> xor_mask_share;
  std::array<std::uint64_t, kReportMetricCount> metrics{};
};

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

using MetricsClock = std::chrono::steady_clock;

double elapsed_milliseconds(
    MetricsClock::time_point begin,
    MetricsClock::time_point end) {
  return std::chrono::duration<double, std::milli>(
             end - begin)
      .count();
}

std::string trim_copy(std::string value) {
  const auto first =
      value.find_first_not_of(" \t\r\n");

  if (first == std::string::npos) {
    return {};
  }

  const auto last =
      value.find_last_not_of(" \t\r\n");

  return value.substr(first, last - first + 1U);
}

std::string detect_cpu_model() {
  std::ifstream input("/proc/cpuinfo");
  std::string line;

  while (std::getline(input, line)) {
    const auto separator = line.find(':');

    if (separator == std::string::npos) {
      continue;
    }

    const auto name =
        trim_copy(line.substr(0U, separator));

    if (name == "model name" ||
        name == "Hardware" ||
        name == "Processor") {
      return trim_copy(line.substr(separator + 1U));
    }
  }

  return {};
}

std::string detect_operating_system() {
  struct utsname information {};

  if (::uname(&information) != 0) {
    return {};
  }

  std::ostringstream output;

  output
      << information.sysname
      << ' '
      << information.release
      << ' '
      << information.machine;

  return output.str();
}

Measurement<std::uint64_t>
detect_system_memory_bytes() {
  struct sysinfo information {};

  if (::sysinfo(&information) != 0) {
    return Measurement<std::uint64_t>::not_measured();
  }

  const auto total =
      static_cast<unsigned long long>(
          information.totalram);

  const auto unit =
      static_cast<unsigned long long>(
          information.mem_unit);

  if (unit != 0U &&
      total >
          std::numeric_limits<std::uint64_t>::max() /
              unit) {
    return Measurement<std::uint64_t>::not_measured();
  }

  return Measurement<std::uint64_t>::measured(
      static_cast<std::uint64_t>(total * unit));
}

std::string compiler_description() {
  std::ostringstream output;

#if defined(__clang__)
  output
      << "Clang "
      << __clang_major__
      << '.'
      << __clang_minor__
      << '.'
      << __clang_patchlevel__;
#elif defined(__GNUC__)
  output
      << "GNU "
      << __GNUC__
      << '.'
      << __GNUC_MINOR__
      << '.'
      << __GNUC_PATCHLEVEL__;
#else
  output << "unknown compiler";
#endif

  return output.str();
}

ProtocolIIIMetricsEnvironment
make_metrics_environment() {
  ProtocolIIIMetricsEnvironment environment;

  environment.git_revision =
      MOE_TOPK_GIT_REVISION;

  environment.runtime =
      "native Linux fork+exec";

  environment.party_topology =
      "one offline Dealer and two online Parties";

  environment.compiler =
      Measurement<std::string>::measured(
          compiler_description());

  // Exact command-line flags are not currently exported by CMake.
  environment.compiler_flags =
      Measurement<std::string>::not_measured();

  const std::string build_type =
      MOE_TOPK_BUILD_TYPE;

  if (!build_type.empty() &&
      build_type != "unknown") {
    environment.build_type =
        Measurement<std::string>::measured(
            build_type);
  }

  const auto cpu_model = detect_cpu_model();

  if (!cpu_model.empty()) {
    environment.cpu_model =
        Measurement<std::string>::measured(
            cpu_model);
  }

  environment.system_memory_bytes =
      detect_system_memory_bytes();

  const auto operating_system =
      detect_operating_system();

  if (!operating_system.empty()) {
    environment.operating_system =
        Measurement<std::string>::measured(
            operating_system);
  }

  environment.network_environment =
      Measurement<std::string>::measured(
          "local AF_UNIX socketpair on one Ubuntu VM");

  environment.network_bandwidth_mbps =
      Measurement<double>::not_measured();

  environment.network_rtt_ms =
      Measurement<double>::not_measured();

  environment.thread_count = 1U;

  return environment;
}

std::uint64_t checked_metric_sum(
    std::initializer_list<std::uint64_t> values) {
  std::uint64_t total = 0U;

  for (const auto value : values) {
    require(
        value <=
            std::numeric_limits<std::uint64_t>::max() -
                total,
        "metrics byte counter overflow");

    total += value;
  }

  return total;
}

std::uint8_t bit_width(std::uint32_t value) {
  std::uint8_t bits = 0;

  while (value != 0U) {
    ++bits;
    value >>= 1U;
  }

  return bits;
}

std::uint32_t padded_size(std::uint32_t logical_n) {
  std::uint32_t padded_n = 2U;

  while (padded_n < logical_n) {
    padded_n <<= 1U;
  }

  return padded_n;
}

std::uint8_t rank_bits(std::uint32_t logical_n) {
  return logical_n <= 1U
             ? 1U
             : bit_width(logical_n - 1U);
}

std::uint8_t comparison_bits(std::uint32_t padded_n) {
  return static_cast<std::uint8_t>(
      33U + bit_width(padded_n - 1U));
}

std::uint64_t low_mask(std::uint8_t bits) {
  return (UINT64_C(1) << bits) - 1U;
}

std::size_t cell_count(
    std::uint32_t logical_n,
    std::uint32_t k) {
  require(
      logical_n != 0U && k != 0U,
      "invalid cell dimensions");

  require(
      static_cast<std::size_t>(logical_n) <=
          std::numeric_limits<std::size_t>::max() /
              static_cast<std::size_t>(k),
      "cell count overflow");

  return static_cast<std::size_t>(logical_n) * k;
}

void seed_fss(std::uint64_t seed) {
  for (int index = 0; index < 256; ++index) {
    FSSConfig::prngs[index].SetSeed(
        osuCrypto::toBlock(
            seed,
            static_cast<std::uint64_t>(index)));
  }
}

void put_u64(Bytes& bytes, std::uint64_t value) {
  for (int adviser = 56; adviser >= 0; adviser -= 8) {
    bytes.push_back(
        static_cast<std::uint8_t>(value >> adviser));
  }
}

std::uint64_t get_u64(
    const Bytes& bytes,
    std::size_t& offset,
    const char* message) {
  require(
      offset <= bytes.size() &&
          bytes.size() - offset >= sizeof(std::uint64_t),
      message);

  std::uint64_t value = 0;

  for (int index = 0; index < 8; ++index) {
    value = (value << 8U) | bytes[offset++];
  }

  return value;
}

Bytes encode_words(
    const std::vector<std::uint64_t>& words) {
  Bytes bytes;
  bytes.reserve(words.size() * sizeof(std::uint64_t));

  for (const auto word : words) {
    put_u64(bytes, word);
  }

  return bytes;
}

std::vector<std::uint64_t> decode_words(
    const Bytes& bytes,
    std::size_t expected_words) {
  require(
      bytes.size() ==
          expected_words * sizeof(std::uint64_t),
      "word payload length");

  std::vector<std::uint64_t> words(expected_words);
  std::size_t offset = 0;

  for (auto& word : words) {
    word = get_u64(bytes, offset, "truncated word payload");
  }

  return words;
}

void wait_for_fd(
    int fd,
    short event,
    int timeout_ms = kTimeoutMs) {
  require(
      timeout_ms > 0,
      "invalid test harness timeout");

  pollfd descriptor{fd, event, 0};

  const int result =
      ::poll(&descriptor, 1, timeout_ms);

  require(result > 0, "test harness I/O timeout");

  require(
      (descriptor.revents & (POLLERR | POLLNVAL)) == 0,
      "test harness poll error");

  // POLLIN/POLLOUT remains usable when POLLHUP is also reported.
  require(
      (descriptor.revents & event) != 0,
      "test harness peer closed");
}

void write_all(
    int fd,
    const std::uint8_t* data,
    std::size_t size,
    int timeout_ms = kTimeoutMs) {
  require(
      timeout_ms > 0,
      "invalid test harness write timeout");

  while (size != 0U) {
    wait_for_fd(fd, POLLOUT, timeout_ms);

    const auto written = ::write(fd, data, size);

    if (written < 0 && errno == EINTR) {
      continue;
    }

    require(written > 0, "test harness write failed");

    data += written;
    size -= static_cast<std::size_t>(written);
  }
}

void read_exact(
    int fd,
    std::uint8_t* data,
    std::size_t size,
    int timeout_ms = kTimeoutMs) {
  while (size != 0U) {
    wait_for_fd(fd, POLLIN, timeout_ms);

    const auto count = ::read(fd, data, size);

    if (count < 0 && errno == EINTR) {
      continue;
    }

    require(count > 0, "test harness read failed");

    data += count;
    size -= static_cast<std::size_t>(count);
  }
}

void send_message(
    int fd,
    const Bytes& message,
    int timeout_ms = kTimeoutMs) {
  require(
      timeout_ms > 0,
      "invalid test harness message timeout");

  require(
      !message.empty() &&
          message.size() <= kMaximumMessageBytes,
      "invalid test harness message");

  Bytes header;
  put_u64(header, message.size());

  write_all(
      fd,
      header.data(),
      header.size(),
      timeout_ms);

  write_all(
      fd,
      message.data(),
      message.size(),
      timeout_ms);
}

Bytes receive_message(
    int fd,
    int timeout_ms = kTimeoutMs) {
  Bytes header(sizeof(std::uint64_t));
  read_exact(
      fd,
      header.data(),
      header.size(),
      timeout_ms);

  std::size_t offset = 0;
  const auto length =
      get_u64(header, offset, "message length");

  require(
      length != 0U &&
          length <= kMaximumMessageBytes &&
          length <=
              std::numeric_limits<std::size_t>::max(),
      "test harness message length");

  Bytes message(static_cast<std::size_t>(length));
  read_exact(
      fd,
      message.data(),
      message.size(),
      timeout_ms);

  return message;
}

class ScopedFd final {
 public:
  ScopedFd() = default;

  explicit ScopedFd(int fd)
      : fd_(fd) {}

  ~ScopedFd() {
    reset();
  }

  ScopedFd(const ScopedFd&) = delete;
  ScopedFd& operator=(const ScopedFd&) = delete;

  ScopedFd(ScopedFd&& other) noexcept
      : fd_(other.release()) {}

  ScopedFd& operator=(ScopedFd&& other) noexcept {
    if (this != &other) {
      reset();
      fd_ = other.release();
    }

    return *this;
  }

  int get() const noexcept {
    return fd_;
  }

  int release() noexcept {
    const int result = fd_;
    fd_ = -1;
    return result;
  }

  void reset() noexcept {
    if (fd_ >= 0) {
      ::close(fd_);
      fd_ = -1;
    }
  }

 private:
  int fd_ = -1;
};

ScopedFd duplicate_lifetime_fd(
    int fd,
    const char* message) {
  require(fd >= 0, message);

  int duplicated = -1;

  do {
    duplicated = ::dup(fd);
  } while (duplicated < 0 && errno == EINTR);

  require(duplicated >= 0, message);

  return ScopedFd(duplicated);
}

// TEST_ONLY synchronization performed only after the real Protocol III
// combine round has completed.
//
// The guard descriptor refers to the same socket endpoint as combine_fd but
// remains open independently of the descriptor handed to
// ProtocolIFramedChannel.
//
// Party 0 sends first; Party 1 acknowledges. Therefore a successful return
// proves that both online parties have left the real three-round runtime
// before either side releases its lifetime-protection descriptors.
void party_completion_handshake(
    int guard_fd,
    int party) {
  require(
      guard_fd >= 0,
      "completion guard file descriptor");

  require(
      party == 0 || party == 1,
      "completion handshake party");

  constexpr std::uint8_t kComplete =
      UINT8_C(0xc3);

  std::uint8_t peer_complete = 0;

  if (party == 0) {
    write_all(
        guard_fd,
        &kComplete,
        sizeof(kComplete));

    read_exact(
        guard_fd,
        &peer_complete,
        sizeof(peer_complete));
  } else {
    read_exact(
        guard_fd,
        &peer_complete,
        sizeof(peer_complete));

    require(
        peer_complete == kComplete,
        "completion handshake marker");

    write_all(
        guard_fd,
        &kComplete,
        sizeof(kComplete));

    return;
  }

  require(
      peer_complete == kComplete,
      "completion handshake marker");
}

class FdPool final {
 public:
  ~FdPool() {
    close_all();
  }

  void make_pair(std::array<int, 2>& pair) {
    pair = {{-1, -1}};

    require(
        ::socketpair(
            AF_UNIX,
            SOCK_STREAM,
            0,
            pair.data()) == 0,
        "socketpair creation failed");

    descriptors_.push_back(&pair[0]);
    descriptors_.push_back(&pair[1]);
  }

  void close_except(const std::vector<int>& keep) {
    for (auto* descriptor : descriptors_) {
      if (*descriptor >= 0 &&
          std::find(
              keep.begin(),
              keep.end(),
              *descriptor) == keep.end()) {
        ::close(*descriptor);
        *descriptor = -1;
      }
    }
  }

  void close_all() {
    close_except({});
  }

 private:
  std::vector<int*> descriptors_;
};

class ChildSet final {
 public:
  ~ChildSet() {
    for (const auto child : children_) {
      if (child > 0) {
        ::kill(child, SIGTERM);
      }
    }

    for (const auto child : children_) {
      if (child > 0) {
        while (::waitpid(child, nullptr, 0) < 0 &&
               errno == EINTR) {
        }
      }
    }
  }

  void add(pid_t child) {
    children_.push_back(child);
  }

  void wait_ok(pid_t child) {
    int status = 0;
    pid_t result = -1;

    do {
      result = ::waitpid(child, &status, 0);
    } while (result < 0 && errno == EINTR);

    require(
        result == child &&
            WIFEXITED(status) &&
            WEXITSTATUS(status) == 0,
        "M3 child process failed");

    for (auto& tracked : children_) {
      if (tracked == child) {
        tracked = -1;
        return;
      }
    }

    throw std::runtime_error("unknown M3 child");
  }

 private:
  std::vector<pid_t> children_;
};

ProtocolIIIGrankConfig grank_config(
    const TestCase& test,
    std::uint8_t party) {
  const auto logical_n =
      static_cast<std::uint32_t>(test.scores.size());

  const auto padded_n = padded_size(logical_n);

  ProtocolIIIGrankConfig config;
  config.session = test.session;
  config.fingerprint = test.fingerprint;
  config.logical_n = logical_n;
  config.padded_n = padded_n;
  config.k = test.k;
  config.comparison_bits =
      comparison_bits(padded_n);
  config.rank_bits = rank_bits(logical_n);
  config.party = party;
  config.timeout_ms = kTimeoutMs;

  return config;
}

ProtocolIIIDpfRoutingConfig routing_config(
    const TestCase& test,
    std::uint8_t party) {
  const auto grank = grank_config(test, party);

  ProtocolIIIDpfRoutingConfig config;
  config.session = grank.session;
  config.fingerprint = grank.fingerprint;
  config.logical_n = grank.logical_n;
  config.k = grank.k;
  config.rank_bits = grank.rank_bits;
  config.comparison_bits = grank.comparison_bits;
  config.party = party;
  config.timeout_ms = kTimeoutMs;

  return config;
}

ProtocolIIISecureCombineConfig combine_config(
    const TestCase& test,
    std::uint8_t party) {
  const auto grank = grank_config(test, party);

  ProtocolIIISecureCombineConfig config;
  config.session = grank.session;
  config.fingerprint = grank.fingerprint;
  config.logical_n = grank.logical_n;
  config.k = grank.k;
  config.comparison_bits = grank.comparison_bits;
  config.party = party;
  config.timeout_ms = kTimeoutMs;

  return config;
}

PriorityKeyShares make_priority_key_shares(
    const TestCase& test) {
  const auto config = grank_config(test, 0);
  const auto ring = low_mask(config.comparison_bits);

  std::vector<std::uint64_t> plaintext_keys(
      config.padded_n);

  for (std::uint32_t index = 0;
       index < config.logical_n;
       ++index) {
    plaintext_keys[index] =
        protocol_i_priority_key(
            test.scores[index],
            index,
            config.padded_n)
            .value;
  }

  for (std::uint32_t index = config.logical_n;
       index < config.padded_n;
       ++index) {
    plaintext_keys[index] =
        protocol_i_priority_key(
            UINT32_C(0x80000000),
            index,
            config.padded_n)
            .value;
  }

  std::mt19937_64 generator(
      test.seed ^ UINT64_C(0x494e505554));

  PriorityKeyShares shares;
  shares.party0.resize(config.padded_n);
  shares.party1.resize(config.padded_n);

  for (std::size_t index = 0;
       index < plaintext_keys.size();
       ++index) {
    shares.party0[index] = generator() & ring;

    shares.party1[index] =
        (plaintext_keys[index] -
         shares.party0[index]) &
        ring;
  }

  return shares;
}

std::pair<OfflineBundle, OfflineBundle>
generate_offline_bundles(const TestCase& test) {
  const auto grank = grank_config(test, 0);
  const auto routing = routing_config(test, 0);
  const auto combine = combine_config(test, 0);

  const auto comparison_ring =
      low_mask(grank.comparison_bits);

  const auto rank_ring =
      low_mask(grank.rank_bits);

  std::mt19937_64 generator(test.seed);

  OfflineBundle party0;
  OfflineBundle party1;

  for (auto* bundle : {&party0, &party1}) {
    bundle->grank_package.session = test.session;
    bundle->grank_package.fingerprint =
        test.fingerprint;
    bundle->grank_package.comparison_bits =
        grank.comparison_bits;
    bundle->grank_package.n = grank.logical_n;
    bundle->grank_package.k = test.k;

    bundle->routing_material.session = test.session;
    bundle->routing_material.fingerprint =
        test.fingerprint;
    bundle->routing_material.logical_n =
        grank.logical_n;
    bundle->routing_material.k = test.k;
    bundle->routing_material.rank_bits =
        grank.rank_bits;

    bundle->combine_material.session = test.session;
    bundle->combine_material.fingerprint =
        test.fingerprint;
    bundle->combine_material.logical_n =
        grank.logical_n;
    bundle->combine_material.k = test.k;

    bundle->unit_payload_shares.resize(
        grank.logical_n);
  }

  party0.grank_package.party = 0;
  party1.grank_package.party = 1;
  party0.routing_material.party = 0;
  party1.routing_material.party = 1;
  party0.combine_material.party = 0;
  party1.combine_material.party = 1;

  party0.grank_package.node_mask_shares.resize(
      grank.logical_n);

  party1.grank_package.node_mask_shares.resize(
      grank.logical_n);

  std::vector<std::uint64_t> full_node_masks(
      grank.logical_n);

  for (std::uint32_t index = 0;
       index < grank.logical_n;
       ++index) {
    full_node_masks[index] =
        generator() & comparison_ring;

    party0.grank_package.node_mask_shares[index] =
        generator() & comparison_ring;

    party1.grank_package.node_mask_shares[index] =
        (full_node_masks[index] -
         party0.grank_package.node_mask_shares[index]) &
        comparison_ring;
  }

  for (std::uint32_t left = 0;
       left < grank.logical_n;
       ++left) {
    for (std::uint32_t right = left + 1U;
         right < grank.logical_n;
         ++right) {
      ProtocolIUcmpMaterial material(
          grank.comparison_bits,
          full_node_masks[left],
          full_node_masks[right]);

      party0.grank_package.edge_materials.emplace_back(
          left,
          right,
          material.export_party_material(0));

      party1.grank_package.edge_materials.emplace_back(
          left,
          right,
          material.export_party_material(1));
    }
  }

  party0.routing_material.rank_mask_shares.resize(
      routing.logical_n);

  party1.routing_material.rank_mask_shares.resize(
      routing.logical_n);

  party0.routing_material.dpf_keys.reserve(
      routing.logical_n);

  party1.routing_material.dpf_keys.reserve(
      routing.logical_n);

  for (std::uint32_t index = 0;
       index < routing.logical_n;
       ++index) {
    const auto full_rank_mask =
        generator() & rank_ring;

    const auto party0_rank_mask =
        generator() & rank_ring;

    party0.routing_material.rank_mask_shares[index] =
        party0_rank_mask;

    party1.routing_material.rank_mask_shares[index] =
        (full_rank_mask - party0_rank_mask) &
        rank_ring;

    auto keys = keyGenDPF(
        routing.rank_bits,
        kDpfPayloadBits,
        full_rank_mask,
        1);

    party0.routing_material.dpf_keys.emplace_back(
        std::move(keys.first));

    party1.routing_material.dpf_keys.emplace_back(
        std::move(keys.second));
  }

  for (std::uint32_t index = 0;
       index < combine.logical_n;
       ++index) {
    party0.unit_payload_shares[index] = generator();

    party1.unit_payload_shares[index] =
        UINT64_C(1) -
        party0.unit_payload_shares[index];
  }

  const auto cells =
      cell_count(combine.logical_n, combine.k);

  party0.combine_material.multiplication_materials.reserve(
      cells);

  party1.combine_material.multiplication_materials.reserve(
      cells);

  for (std::size_t cell = 0;
       cell < cells;
       ++cell) {
    auto generated =
        generate_masked_mul_material(
            generator(),
            generator(),
            generator());

    party0.combine_material.multiplication_materials.push_back(
        std::move(generated.party0));

    party1.combine_material.multiplication_materials.push_back(
        std::move(generated.party1));
  }

  return {
      std::move(party0),
      std::move(party1)};
}

Bytes serialize_offline_bundle(
    const TestCase& test,
    int party,
    const OfflineBundle& bundle) {
  const auto grank =
      grank_config(test, static_cast<std::uint8_t>(party));

  const auto package_bytes =
      serialize_party_package(bundle.grank_package);

  Bytes bytes = {
      'M', '3', 'O', 'F',
      1,
      static_cast<std::uint8_t>(party),
      grank.rank_bits,
      grank.comparison_bits};

  put_u64(bytes, test.session);
  put_u64(bytes, test.fingerprint);
  put_u64(bytes, grank.logical_n);
  put_u64(bytes, grank.padded_n);
  put_u64(bytes, test.k);
  put_u64(bytes, package_bytes.size());

  bytes.insert(
      bytes.end(),
      package_bytes.begin(),
      package_bytes.end());

  for (const auto share :
       bundle.routing_material.rank_mask_shares) {
    put_u64(bytes, share);
  }

  for (const auto share :
       bundle.unit_payload_shares) {
    put_u64(bytes, share);
  }

  require(
      bytes.size() < kMaximumMessageBytes,
      "offline bundle prefix too large");

  const auto prefix_size = bytes.size();

  bytes.resize(kMaximumMessageBytes);

  char* base =
      reinterpret_cast<char*>(bytes.data());

  char* cursor = base + prefix_size;

  Peer sender(&cursor);

  for (const auto& owned_key :
       bundle.routing_material.dpf_keys) {
    sender.send_dpf_keypack(owned_key.native_key());
  }

  for (const auto& material :
       bundle.combine_material.multiplication_materials) {
    send_masked_mul_material(sender, material);
  }

  const auto tail_size =
      static_cast<std::size_t>(sender.bytesSent());

  require(
      prefix_size <= kMaximumMessageBytes - tail_size,
      "offline bundle exceeds limit");

  bytes.resize(prefix_size + tail_size);

  delete static_cast<MemBuf*>(sender.keyBuf);

  return bytes;
}

OfflineBundle deserialize_offline_bundle(
    const TestCase& test,
    int expected_party,
    const Bytes& bytes) {
  const auto grank =
      grank_config(
          test,
          static_cast<std::uint8_t>(expected_party));

  require(
      bytes.size() >= 56U &&
          bytes[0] == 'M' &&
          bytes[1] == '3' &&
          bytes[2] == 'O' &&
          bytes[3] == 'F' &&
          bytes[4] == 1 &&
          bytes[5] == expected_party &&
          bytes[6] == grank.rank_bits &&
          bytes[7] == grank.comparison_bits,
      "offline bundle header");

  std::size_t offset = 8U;

  require(
      get_u64(bytes, offset, "offline session") ==
              test.session &&
          get_u64(bytes, offset, "offline fingerprint") ==
              test.fingerprint &&
          get_u64(bytes, offset, "offline logical_n") ==
              grank.logical_n &&
          get_u64(bytes, offset, "offline padded_n") ==
              grank.padded_n &&
          get_u64(bytes, offset, "offline k") ==
              test.k,
      "offline bundle binding");

  const auto package_length =
      get_u64(bytes, offset, "offline package length");

  require(
      package_length <= bytes.size() - offset,
      "offline package truncated");

  const auto package_size =
      static_cast<std::size_t>(package_length);

  Bytes package_bytes(
      bytes.begin() + offset,
      bytes.begin() + offset + package_size);

  offset += package_size;

  OfflineBundle bundle;

  bundle.grank_package =
      deserialize_party_package(
          package_bytes,
          expected_party);

  bundle.routing_material.session = test.session;
  bundle.routing_material.fingerprint =
      test.fingerprint;
  bundle.routing_material.logical_n =
      grank.logical_n;
  bundle.routing_material.k = test.k;
  bundle.routing_material.rank_bits =
      grank.rank_bits;
  bundle.routing_material.party =
      static_cast<std::uint8_t>(expected_party);

  bundle.routing_material.rank_mask_shares.resize(
      grank.logical_n);

  for (auto& share :
       bundle.routing_material.rank_mask_shares) {
    share = get_u64(
        bytes,
        offset,
        "offline rank-mask share");
  }

  bundle.unit_payload_shares.resize(grank.logical_n);

  for (auto& share : bundle.unit_payload_shares) {
    share = get_u64(
        bytes,
        offset,
        "offline unit-payload share");
  }

  bundle.combine_material.session = test.session;
  bundle.combine_material.fingerprint =
      test.fingerprint;
  bundle.combine_material.logical_n =
      grank.logical_n;
  bundle.combine_material.k = test.k;
  bundle.combine_material.party =
      static_cast<std::uint8_t>(expected_party);

  char* cursor =
      reinterpret_cast<char*>(
          const_cast<std::uint8_t*>(
              bytes.data() + offset));

  Dealer receiver(&cursor);

  try {
    bundle.routing_material.dpf_keys.reserve(
        grank.logical_n);

    for (std::uint32_t index = 0;
         index < grank.logical_n;
         ++index) {
      // In MemBuf mode recv_dpf_keypack() returns a non-owning pointer into
      // the serialized message. ProtocolIIIDpfRoutingKey requires an owning,
      // correctly aligned DPFKeyPack, so copy the block array before wrapping.
      auto wire_key = receiver.recv_dpf_keypack(
          grank.rank_bits,
          kDpfPayloadBits);

      require(
          wire_key.s != nullptr,
          "offline DPF key has no seed blocks");

      DPFKeyPack owned_key(
          grank.rank_bits,
          kDpfPayloadBits);

      std::memcpy(
          owned_key.s,
          wire_key.s,
          static_cast<std::size_t>(grank.rank_bits + 1U) *
              sizeof(osuCrypto::block));

      owned_key.tLcw = wire_key.tLcw;
      owned_key.tRcw = wire_key.tRcw;
      owned_key.payload = wire_key.payload;

      bundle.routing_material.dpf_keys.emplace_back(
          std::move(owned_key));

      // wire_key.s points inside offline_bytes and must never be deleted.
      wire_key.s = nullptr;
    }

    const auto cells =
        cell_count(grank.logical_n, test.k);

    bundle.combine_material.multiplication_materials.reserve(
        cells);

    for (std::size_t cell = 0;
         cell < cells;
         ++cell) {
      bundle.combine_material.multiplication_materials.push_back(
          receive_masked_mul_material(receiver));
    }

    require(
        cursor ==
            reinterpret_cast<char*>(
                const_cast<std::uint8_t*>(
                    bytes.data() + bytes.size())),
        "offline bundle trailing bytes");
  } catch (...) {
    delete static_cast<MemBuf*>(receiver.keyBuf);
    throw;
  }

  delete static_cast<MemBuf*>(receiver.keyBuf);

  return bundle;
}

Bytes encode_report(
    const ProtocolIIIGrankOutput& grank,
    const ProtocolIIIDpfRoutingOutput& routing,
    const ProtocolIIISecureCombineOutput& combine,
    std::uint64_t offline_bytes) {
  Bytes bytes(
      combine.xor_mask_shares.begin(),
      combine.xor_mask_shares.end());

  const std::array<std::uint64_t, kReportMetricCount>
      metrics{{
          offline_bytes,
          grank.metrics.sent_bytes,
          grank.metrics.received_bytes,
          grank.metrics.comparison_edges,
          grank.metrics.raw_dcf_calls,
          routing.metrics.sent_bytes,
          routing.metrics.received_bytes,
          routing.metrics.dpf_keys,
          routing.metrics.eval_calls,
          combine.metrics.sent_bytes,
          combine.metrics.received_bytes,
          combine.metrics.multiplication_calls,
          combine.metrics.opened_masked_values,
          grank.metrics.online_rounds +
              routing.metrics.online_rounds +
              combine.metrics.online_rounds,
      }};

  for (const auto metric : metrics) {
    put_u64(bytes, metric);
  }

  return bytes;
}

PartyReport decode_report(
    const Bytes& bytes,
    std::uint32_t logical_n) {
  require(
      bytes.size() ==
          logical_n +
              kReportMetricCount *
                  sizeof(std::uint64_t),
      "party report length");

  PartyReport report;

  report.xor_mask_share.assign(
      bytes.begin(),
      bytes.begin() + logical_n);

  std::size_t offset = logical_n;

  for (auto& metric : report.metrics) {
    metric = get_u64(
        bytes,
        offset,
        "party report metric");
  }

  require(
      offset == bytes.size(),
      "party report trailing bytes");

  return report;
}

int dealer_main(
    const TestCase& test,
    int party0_fd,
    int party1_fd) {
  try {
    seed_fss(
        test.seed ^ UINT64_C(0x4445414c4552));

    auto bundles = generate_offline_bundles(test);

    const auto encoded0 =
        serialize_offline_bundle(
            test,
            0,
            bundles.first);

    const auto encoded1 =
        serialize_offline_bundle(
            test,
            1,
            bundles.second);

    send_message(
        party0_fd,
        encoded0,
        kOfflineMaterialTimeoutMs);

    send_message(
        party1_fd,
        encoded1,
        kOfflineMaterialTimeoutMs);

    return 0;
  } catch (const std::exception& error) {
    std::cerr
        << "Dealer: "
        << error.what()
        << '\n';

    return 1;
  }
}

int party_main(
    const TestCase& test,
    int party,
    int offline_fd,
    int input_fd,
    int grank_fd,
    int routing_fd,
    int combine_fd,
    int result_fd) {
  try {
    // TEST_ONLY descriptor-lifetime protection.
    //
    // The framed runtime may finish/close one descriptor while its peer is
    // still consuming the final bytes. Keep one duplicate of every online
    // endpoint alive until both parties have completed all three stages.
    auto grank_lifetime =
        duplicate_lifetime_fd(
            grank_fd,
            "GRank lifetime dup failed");

    auto routing_lifetime =
        duplicate_lifetime_fd(
            routing_fd,
            "routing lifetime dup failed");

    auto combine_lifetime =
        duplicate_lifetime_fd(
            combine_fd,
            "combine lifetime dup failed");

    const auto offline_bytes =
        receive_message(
            offline_fd,
            kOfflineMaterialTimeoutMs);

    auto bundle =
        deserialize_offline_bundle(
            test,
            party,
            offline_bytes);

    const auto grank_cfg =
        grank_config(
            test,
            static_cast<std::uint8_t>(party));

    const auto routing_cfg =
        routing_config(
            test,
            static_cast<std::uint8_t>(party));

    const auto combine_cfg =
        combine_config(
            test,
            static_cast<std::uint8_t>(party));

    const auto priority_key_shares =
        decode_words(
            receive_message(input_fd),
            grank_cfg.padded_n);

    const auto grank =
        protocol_iii_grank_party(
            grank_cfg,
            bundle.grank_package,
            priority_key_shares,
            grank_fd);

    const auto routing =
        protocol_iii_dpf_routing_party(
            routing_cfg,
            bundle.routing_material,
            grank.rank_additive_shares,
            routing_fd);

    const auto combine =
        protocol_iii_secure_combine_party(
            combine_cfg,
            bundle.combine_material,
            routing.indicator_shares,
            bundle.unit_payload_shares,
            combine_fd);

    // This is deliberately outside all protocol metrics and is TEST_ONLY.
    // It is not a fourth Protocol III online round. Its only purpose is to
    // keep both socket endpoints alive until both parties have returned from
    // R3, avoiding a POLLHUP/final-frame close race in the frozen framed
    // transport.
    party_completion_handshake(
        combine_lifetime.get(),
        party);

    // Both parties have now left the real online runtime, so the protective
    // duplicates can be released without affecting protocol execution.
    grank_lifetime.reset();
    routing_lifetime.reset();
    combine_lifetime.reset();

    require(
        bundle.grank_package.node_mask_shares.empty() &&
            bundle.grank_package.edge_materials.empty(),
        "GRank material not consumed");

    require(
        bundle.routing_material.rank_mask_shares.empty() &&
            bundle.routing_material.dpf_keys.empty(),
        "routing material not consumed");

    require(
        bundle.combine_material
            .multiplication_materials.empty(),
        "combine material not consumed");

    const auto report =
        encode_report(
            grank,
            routing,
            combine,
            offline_bytes.size() +
                sizeof(std::uint64_t));

    send_message(result_fd, report);

    return 0;
  } catch (const std::exception& error) {
    std::cerr
        << "Party "
        << party
        << ": "
        << error.what()
        << '\n';

    return 1;
  }
}

std::uint32_t parse_u32(
    const char* value,
    const char* message) {
  require(value != nullptr, message);

  const auto parsed = std::stoull(value);

  require(
      parsed <=
          std::numeric_limits<std::uint32_t>::max(),
      message);

  return static_cast<std::uint32_t>(parsed);
}

std::uint64_t parse_u64(
    const char* value,
    const char* message) {
  require(value != nullptr, message);
  return std::stoull(value);
}

int parse_fd(
    const char* value,
    const char* message) {
  require(value != nullptr, message);

  const auto parsed = std::stoll(value);

  require(
      parsed >= 0 &&
          parsed <= std::numeric_limits<int>::max(),
      message);

  return static_cast<int>(parsed);
}

TestCase make_public_test_case(
    std::uint32_t logical_n,
    std::uint32_t k,
    std::uint64_t session,
    std::uint64_t fingerprint,
    std::uint64_t seed) {
  require(
      logical_n >= 1U &&
          k >= 1U &&
          k <= logical_n &&
          session != 0U &&
          fingerprint != 0U,
      "invalid public M3 role configuration");

  // Role processes need only the public logical length. The values are
  // deliberately zero-filled because Dealer and Party code must never use
  // controller plaintext scores.
  return {
      std::vector<std::uint32_t>(logical_n, 0U),
      k,
      session,
      fingerprint,
      seed};
}

TestCase parse_public_test_case(
    int argc,
    char** argv,
    int start) {
  require(
      start >= 0 &&
          argc >= start + 5,
      "truncated public M3 role configuration");

  return make_public_test_case(
      parse_u32(argv[start], "invalid logical_n"),
      parse_u32(argv[start + 1], "invalid k"),
      parse_u64(argv[start + 2], "invalid session"),
      parse_u64(argv[start + 3], "invalid fingerprint"),
      parse_u64(argv[start + 4], "invalid seed"));
}

std::string current_executable(
    const char* fallback) {
  std::array<char, 4096> path{};

  const auto length =
      ::readlink(
          "/proc/self/exe",
          path.data(),
          path.size() - 1U);

  if (length > 0) {
    return std::string(
        path.data(),
        static_cast<std::size_t>(length));
  }

  require(
      fallback != nullptr &&
          fallback[0] != '\0',
      "M3 executable path");

  return fallback;
}

pid_t launch_exec_role(
    const std::string& executable,
    const std::vector<std::string>& arguments,
    FdPool& descriptors,
    ChildSet& children,
    const std::vector<int>& keep) {
  const auto child = ::fork();

  require(child >= 0, "fork M3 role failed");

  if (child == 0) {
    descriptors.close_except(keep);

    std::vector<char*> argv;
    argv.reserve(arguments.size() + 2U);

    argv.push_back(
        const_cast<char*>(executable.c_str()));

    for (const auto& argument : arguments) {
      argv.push_back(
          const_cast<char*>(argument.c_str()));
    }

    argv.push_back(nullptr);

    ::execv(executable.c_str(), argv.data());

    ::_exit(127);
  }

  children.add(child);
  return child;
}

pid_t launch_party(
    const char* self,
    const TestCase& test,
    int party,
    FdPool& descriptors,
    ChildSet& children,
    int offline_fd,
    int input_fd,
    int grank_fd,
    int routing_fd,
    int combine_fd,
    int result_fd) {
  require(
      self != nullptr &&
          (party == 0 || party == 1),
      "invalid M3 party launch");

  // The controller and TEST_ONLY Dealer remain in this test executable.
  // Both online parties are replaced by the formal production executable.
  const std::vector<std::string> arguments{
      "party",
      std::to_string(party),
      std::to_string(offline_fd),
      std::to_string(input_fd),
      std::to_string(grank_fd),
      std::to_string(routing_fd),
      std::to_string(combine_fd),
      std::to_string(result_fd),
      std::to_string(test.scores.size()),
      std::to_string(test.k),
      std::to_string(test.session),
      std::to_string(test.fingerprint),
  };

  return launch_exec_role(
      MOE_TOPK_PROTOCOL_III_EXECUTABLE,
      arguments,
      descriptors,
      children,
      {
          offline_fd,
          input_fd,
          grank_fd,
          routing_fd,
          combine_fd,
          result_fd,
      });
}
pid_t launch_dealer(
    const char* self,
    const TestCase& test,
    FdPool& descriptors,
    ChildSet& children,
    int party0_fd,
    int party1_fd) {
  require(
      self != nullptr,
      "invalid M3 Dealer launch");

  const std::vector<std::string> arguments{
      "m3-dealer",
      std::to_string(party0_fd),
      std::to_string(party1_fd),
      std::to_string(test.scores.size()),
      std::to_string(test.k),
      std::to_string(test.session),
      std::to_string(test.fingerprint),
      std::to_string(test.seed),
  };

  return launch_exec_role(
      current_executable(self),
      arguments,
      descriptors,
      children,
      {party0_fd, party1_fd});
}

void verify_reports(
    const TestCase& test,
    const PartyReport& party0,
    const PartyReport& party1) {
  const auto logical_n =
      static_cast<std::uint32_t>(test.scores.size());

  const auto expected_mask =
      top_k_mask(test.scores, test.k);

  require(
      party0.xor_mask_share.size() == logical_n &&
          party1.xor_mask_share.size() == logical_n,
      "E2E mask shape");

  std::size_t selected = 0;

  for (std::size_t index = 0;
       index < logical_n;
       ++index) {
    require(
        party0.xor_mask_share[index] <= 1U &&
            party1.xor_mask_share[index] <= 1U,
        "E2E output is not an XOR bit share");

    const auto reconstructed =
        static_cast<std::uint8_t>(
            party0.xor_mask_share[index] ^
            party1.xor_mask_share[index]);

    require(
        reconstructed == expected_mask[index],
        "Protocol III E2E differs from oracle");

    selected += reconstructed;
  }

  require(
      selected == test.k,
      "Protocol III E2E did not select exactly K");

  const auto expected_edges =
      static_cast<std::uint64_t>(logical_n) *
      static_cast<std::uint64_t>(logical_n - 1U) /
      2U;

  const auto expected_cells =
      static_cast<std::uint64_t>(logical_n) *
      test.k;

  for (const auto* report : {&party0, &party1}) {
    const auto& metrics = report->metrics;

    require(metrics[0] > 0U, "offline byte metric");
    require(metrics[1] > 0U, "GRank sent metric");
    require(metrics[2] > 0U, "GRank received metric");
    require(metrics[3] == expected_edges, "GRank edges");
    require(metrics[4] == expected_edges * 2U, "GRank DCF calls");
    require(metrics[5] > 0U, "routing sent metric");
    require(metrics[6] > 0U, "routing received metric");
    require(metrics[7] == logical_n, "routing DPF keys");
    require(metrics[8] == expected_cells, "routing eval calls");
    require(metrics[9] > 0U, "combine sent metric");
    require(metrics[10] > 0U, "combine received metric");
    require(metrics[11] == expected_cells, "multiplication calls");
    require(metrics[12] == expected_cells * 2U, "opened values");
    require(metrics[13] == 3U, "online round count");
  }

  require(
      party0.metrics[1] == party1.metrics[2] &&
          party1.metrics[1] == party0.metrics[2],
      "GRank communication mismatch");

  require(
      party0.metrics[5] == party1.metrics[6] &&
          party1.metrics[5] == party0.metrics[6],
      "routing communication mismatch");

  require(
      party0.metrics[9] == party1.metrics[10] &&
          party1.metrics[9] == party0.metrics[10],
      "combine communication mismatch");
}

void emit_three_round_metrics_record(
    const TestCase& test,
    const PartyReport& party0,
    const PartyReport& party1,
    double offline_time_ms,
    double online_time_ms) {
  require(
      party0.metrics[3] == party1.metrics[3],
      "metrics comparison-edge mismatch");

  require(
      party0.metrics[13] == 3U &&
          party1.metrics[13] == 3U,
      "metrics three-round mismatch");

  ProtocolIIIMetricsObservation observation;

  observation.n =
      static_cast<std::uint64_t>(
          test.scores.size());

  observation.k = test.k;

  observation.input_seed =
      Measurement<std::uint64_t>::measured(
          test.seed);

  observation.input_distribution =
      Measurement<std::string>::measured(
          "deterministic regression vectors with "
          "boundary and stable-tie coverage");

  observation.warmup_runs =
      Measurement<std::uint64_t>::measured(0U);

  observation.repetitions =
      Measurement<std::uint64_t>::measured(1U);

  observation.offline_time_ms =
      Measurement<double>::measured(
          offline_time_ms);

  observation.offline_material_total_bytes =
      Measurement<std::uint64_t>::measured(
          checked_metric_sum({
              party0.metrics[0],
              party1.metrics[0],
          }));

  observation.online_time_ms =
      Measurement<double>::measured(
          online_time_ms);

  // The present primitive interfaces do not expose a complete
  // online PRG-call counter.
  observation.online_prg_calls_total =
      Measurement<std::uint64_t>::not_measured();

  observation.comparison_edges_total =
      Measurement<std::uint64_t>::measured(
          party0.metrics[3]);

  observation.parties = {
      {
          "P0",
          checked_metric_sum({
              party0.metrics[1],
              party0.metrics[5],
              party0.metrics[9],
          }),
          checked_metric_sum({
              party0.metrics[2],
              party0.metrics[6],
              party0.metrics[10],
          }),
      },
      {
          "P1",
          checked_metric_sum({
              party1.metrics[1],
              party1.metrics[5],
              party1.metrics[9],
          }),
          checked_metric_sum({
              party1.metrics[2],
              party1.metrics[6],
              party1.metrics[10],
          }),
      },
  };

  observation.correctness_status =
      CorrectnessStatus::PASSED;

  const auto record =
      make_protocol_iii_modular_3round_metrics_record(
          make_metrics_environment(),
          observation);

  std::cout
      << protocol_iii_metrics_record_json(record)
      << '\n'
      << std::flush;
}

TestCase make_generated_case(
    std::uint32_t logical_n,
    std::uint32_t k,
    std::uint64_t session,
    std::uint64_t fingerprint,
    std::uint64_t seed) {
  require(
      logical_n >= 1U &&
          k >= 1U &&
          k <= logical_n,
      "invalid generated E2E case");

  std::mt19937_64 generator(
      seed ^ UINT64_C(0x4d335f4d41545258));

  std::vector<std::uint32_t> scores(logical_n);

  for (std::uint32_t index = 0;
       index < logical_n;
       ++index) {
    scores[index] =
        static_cast<std::uint32_t>(generator());

    // Deterministic repeated values exercise stable tie handling without
    // turning the entire vector into one degenerate tie case.
    if (index != 0U && index % 11U == 0U) {
      scores[index] = scores[index - 1U];
    }
  }

  // Explicit boundary values used by the priority-key encoding.
  if (logical_n >= 5U) {
    scores[0] = 0U;
    scores[1] = UINT32_MAX;
    scores[2] = UINT32_C(0x80000000);
    scores[3] = UINT32_C(0x7fffffff);
    scores[4] = 0U;
  }

  return {
      std::move(scores),
      k,
      session,
      fingerprint,
      seed};
}

void run_case(
    const char* self,
    const TestCase& test) {
  require(
      !test.scores.empty() &&
          test.k >= 1U &&
          test.k <= test.scores.size(),
      "invalid E2E test case");

  FdPool descriptors;
  ChildSet children;

  std::array<int, 2> dealer_party0;
  std::array<int, 2> dealer_party1;
  std::array<int, 2> input_party0;
  std::array<int, 2> input_party1;
  std::array<int, 2> grank;
  std::array<int, 2> routing;
  std::array<int, 2> combine;
  std::array<int, 2> result_party0;
  std::array<int, 2> result_party1;

  for (auto* pair :
       {&dealer_party0,
        &dealer_party1,
        &input_party0,
        &input_party1,
        &grank,
        &routing,
        &combine,
        &result_party0,
        &result_party1}) {
    descriptors.make_pair(*pair);
  }

  const auto party0 =
      launch_party(
          self,
          test,
          0,
          descriptors,
          children,
          dealer_party0[1],
          input_party0[1],
          grank[0],
          routing[0],
          combine[0],
          result_party0[1]);

  const auto party1 =
      launch_party(
          self,
          test,
          1,
          descriptors,
          children,
          dealer_party1[1],
          input_party1[1],
          grank[1],
          routing[1],
          combine[1],
          result_party1[1]);

  const auto offline_start =
      MetricsClock::now();

  const auto dealer =
      launch_dealer(
          self,
          test,
          descriptors,
          children,
          dealer_party0[0],
          dealer_party1[0]);

  // All three children have now inherited exactly the descriptors selected
  // by launch_party()/launch_dealer(). The controller must not retain copies
  // of Dealer or P0/P1 online sockets: such copies would hide peer failure
  // and suppress the HUP that should be visible when a Party exits.
  descriptors.close_except(
      {input_party0[0],
       input_party1[0],
       result_party0[0],
       result_party1[0]});

  // Strict offline/online boundary.
  //
  // P0/P1 can receive and deserialize their preprocessing bundles, but they
  // block on input_fd before GRank. Online secret shares are not released
  // until Dealer has completed preprocessing and exited successfully.
  children.wait_ok(dealer);

  const auto offline_end =
      MetricsClock::now();

  // The Dealer has now exited. Only the controller creates the TEST_ONLY
  // priority-key shares, so neither the exec-isolated Dealer nor either
  // exec-isolated Party inherited plaintext scores or both input shares.
  const auto input_shares =
      make_priority_key_shares(test);

  const auto online_start =
      MetricsClock::now();

  send_message(
      input_party0[0],
      encode_words(input_shares.party0));

  send_message(
      input_party1[0],
      encode_words(input_shares.party1));

  // The controller has finished sending online inputs. Keep only the two
  // report endpoints from this point onward.
  descriptors.close_except(
      {result_party0[0],
       result_party1[0]});

  const auto report0 =
      decode_report(
          receive_message(result_party0[0]),
          static_cast<std::uint32_t>(
              test.scores.size()));

  const auto report1 =
      decode_report(
          receive_message(result_party1[0]),
          static_cast<std::uint32_t>(
              test.scores.size()));

  const auto online_end =
      MetricsClock::now();

   verify_reports(test, report0, report1);

  emit_three_round_metrics_record(
      test,
      report0,
      report1,
      elapsed_milliseconds(
          offline_start,
          offline_end),
      elapsed_milliseconds(
          online_start,
          online_end));

  // Both formal Party executables have now completed the measured
  // three-round core and delivered their reports. Release their
  // process-lifetime guards over the separate controller channel.
  const Bytes acknowledgement{
      static_cast<std::uint8_t>('O'),
      static_cast<std::uint8_t>('K'),
  };

  send_message(
      result_party0[0],
      acknowledgement);

  send_message(
      result_party1[0],
      acknowledgement);

  descriptors.close_all();

  children.wait_ok(party0);
  children.wait_ok(party1);
}

}  // namespace

int main(int argc, char** argv) {
  try {
    std::signal(SIGPIPE, SIG_IGN);

    if (argc > 1 &&
        std::string(argv[1]) == "m3-dealer") {
      require(
          argc == 9,
          "invalid M3 Dealer argument count");

      const auto test =
          parse_public_test_case(argc, argv, 4);

      return dealer_main(
          test,
          parse_fd(argv[2], "invalid Dealer P0 fd"),
          parse_fd(argv[3], "invalid Dealer P1 fd"));
    }

    if (argc > 1 &&
        std::string(argv[1]) == "m3-party") {
      require(
          argc == 14,
          "invalid M3 Party argument count");

      const auto party =
          static_cast<int>(
              parse_u32(argv[2], "invalid Party id"));

      require(
          party == 0 || party == 1,
          "invalid M3 Party id");

      const auto test =
          parse_public_test_case(argc, argv, 9);

      return party_main(
          test,
          party,
          parse_fd(argv[3], "invalid Party offline fd"),
          parse_fd(argv[4], "invalid Party input fd"),
          parse_fd(argv[5], "invalid Party GRank fd"),
          parse_fd(argv[6], "invalid Party routing fd"),
          parse_fd(argv[7], "invalid Party combine fd"),
          parse_fd(argv[8], "invalid Party result fd"));
    }

    require(
        argc == 1,
        "unknown M3 E2E role");

    const auto self =
        current_executable(argv[0]);

    std::vector<TestCase> cases{
        {{7U},
         1U,
         UINT64_C(0x3501),
         UINT64_C(0x4501),
         UINT64_C(0x5501)},

        {{5U, 5U, 9U},
         2U,
         UINT64_C(0x3502),
         UINT64_C(0x4502),
         UINT64_C(0x5502)},

        {{5U, 5U, 5U},
         3U,
         UINT64_C(0x3503),
         UINT64_C(0x4503),
         UINT64_C(0x5503)},

        {{UINT32_C(0x80000000),
          UINT32_MAX,
          0U,
          UINT32_C(0x7fffffff),
          0U},
         3U,
         UINT64_C(0x3504),
         UINT64_C(0x4504),
         UINT64_C(0x5504)},

        {{11U, 4U, 19U, 4U, 7U, 19U, 2U},
         1U,
         UINT64_C(0x3505),
         UINT64_C(0x4505),
         UINT64_C(0x5505)},

        {{8U, 3U, 8U, 1U, 9U, 4U, 9U, 2U},
         8U,
         UINT64_C(0x3506),
         UINT64_C(0x4506),
         UINT64_C(0x5506)},
    };

    // Final M3.5 edge-size coverage.
    cases.push_back(
        make_generated_case(
            127U,
            2U,
            UINT64_C(0x3601),
            UINT64_C(0x4601),
            UINT64_C(0x5601)));

    // n=128,K=2 covers both the power-of-two boundary and the required
    // performance/correctness matrix.
    cases.push_back(
        make_generated_case(
            128U,
            2U,
            UINT64_C(0x3602),
            UINT64_C(0x4602),
            UINT64_C(0x5602)));

    cases.push_back(
        make_generated_case(
            128U,
            8U,
            UINT64_C(0x3603),
            UINT64_C(0x4603),
            UINT64_C(0x5603)));

    cases.push_back(
        make_generated_case(
            129U,
            2U,
            UINT64_C(0x3604),
            UINT64_C(0x4604),
            UINT64_C(0x5604)));

    cases.push_back(
        make_generated_case(
            256U,
            2U,
            UINT64_C(0x3605),
            UINT64_C(0x4605),
            UINT64_C(0x5605)));

    cases.push_back(
        make_generated_case(
            256U,
            8U,
            UINT64_C(0x3606),
            UINT64_C(0x4606),
            UINT64_C(0x5606)));

    for (const auto& test : cases) {
      std::cout
          << "protocol="
          << kProtocolLabel
          << " case n="
          << test.scores.size()
          << " k="
          << test.k
          << " start\n"
          << std::flush;

      run_case(self.c_str(), test);

      std::cout
          << "protocol="
          << kProtocolLabel
          << " case n="
          << test.scores.size()
          << " k="
          << test.k
          << " passed\n"
          << std::flush;
    }

    std::cout
        << "protocol="
        << kProtocolLabel
        << " status=passed cases="
        << cases.size()
        << '\n';

    return 0;
  } catch (const std::exception& error) {
    std::cerr
        << "Protocol III three-process E2E failure: "
        << error.what()
        << '\n';

    return 1;
  }
}
