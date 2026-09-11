#include <moe_topk/protocol_i_dealer_candidate_core.h>
#include <moe_topk/protocol_i_dealer_candidate_package.h>
#include <moe_topk/protocol_i_permutation.h>
#include <moe_topk/protocol_i_pipeline.h>
#include <moe_topk/protocol_i_transport.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <cstdint>
#include <fcntl.h>
#include <limits>
#include <map>
#include <random>
#include <stdexcept>
#include <string>
#include <sys/resource.h>
#include <unistd.h>
#include <vector>

#include <openssl/rand.h>

namespace {
using namespace moe_topk;

[[noreturn]] void fail(const char* message) { throw std::invalid_argument(message); }

class Arguments {
 public:
  explicit Arguments(int argc, char** argv) {
    for (int index = 1; index < argc; ++index) {
      const std::string key(argv[index]);
      if (key.rfind("--", 0) != 0 || index + 1 >= argc) fail("candidate arguments");
      values_[key] = argv[++index];
    }
  }

  std::string required(const char* key) const {
    const auto found = values_.find(key);
    if (found == values_.end()) fail("candidate argument missing");
    return found->second;
  }

  std::string optional(const char* key, const char* fallback) const {
    const auto found = values_.find(key);
    return found == values_.end() ? fallback : found->second;
  }

 private:
  std::map<std::string, std::string> values_;
};

std::uint64_t number(const Arguments& arguments, const char* key) {
  const auto value = std::stoull(arguments.required(key));
  return value;
}

std::uint32_t dimension(const Arguments& arguments, const char* key) {
  const auto value = number(arguments, key);
  if (value > std::numeric_limits<std::uint32_t>::max()) fail("candidate dimension");
  return static_cast<std::uint32_t>(value);
}

int descriptor(const Arguments& arguments, const char* key) {
  const auto value = std::stoll(arguments.required(key));
  if (value < 0 || value > std::numeric_limits<int>::max()) fail("candidate fd");
  return static_cast<int>(value);
}

int timeout_ms(const Arguments& arguments) {
  const auto value = number(arguments, "--timeout-ms");
  if (value == 0 || value > std::numeric_limits<int>::max()) fail("candidate timeout");
  return static_cast<int>(value);
}

void set_close_on_exec(int fd) {
  const auto flags = ::fcntl(fd, F_GETFD);
  if (flags < 0 || ::fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0) fail("candidate fd flags");
}

void close_except(const std::vector<int>& keep) {
  struct rlimit limit {};
  if (::getrlimit(RLIMIT_NOFILE, &limit) != 0) fail("candidate fd limit");
  const auto upper = std::min<std::uint64_t>(limit.rlim_cur, 65536U);
  for (int fd = 3; static_cast<std::uint64_t>(fd) < upper; ++fd) {
    if (std::find(keep.begin(), keep.end(), fd) == keep.end()) (void)::close(fd);
  }
  for (const auto fd : keep) set_close_on_exec(fd);
}

std::uint64_t random_value() {
  std::uint64_t value = 0;
  if (RAND_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value)) != 1) {
    fail("candidate permutation randomness");
  }
  return value;
}

ProtocolIPermutation local_permutation(std::uint32_t n, unsigned mode, std::uint64_t seed) {
  auto permutation = protocol_i_identity_permutation(n);
  if (mode == 0) return permutation;
  if (mode == 1) {
    std::reverse(permutation.begin(), permutation.end());
    return permutation;
  }
  if (mode != 2) fail("candidate permutation mode");
  std::mt19937_64 random(seed);
  std::shuffle(permutation.begin(), permutation.end(), random);
  return permutation;
}

std::vector<std::uint64_t> decode_words(const std::vector<std::uint8_t>& bytes,
                                        std::size_t expected_count) {
  if (bytes.size() != expected_count * sizeof(std::uint64_t)) fail("candidate input shape");
  std::vector<std::uint64_t> words(expected_count);
  std::size_t offset = 0;
  for (auto& word : words) {
    for (int index = 0; index < 8; ++index) word = (word << 8U) | bytes[offset++];
  }
  return words;
}

ProtocolIDealerCandidatePublicConfig public_config(const Arguments& arguments) {
  const auto logical_n = dimension(arguments, "--logical-n");
  const auto k = dimension(arguments, "--k");
  const auto layout = protocol_i_make_input_layout(
      logical_n, k);
  ProtocolIDealerCandidatePublicConfig config;
  config.session = number(arguments, "--session");
  config.fingerprint = number(arguments, "--fingerprint");
  config.material_id = number(arguments, "--material-id");
  config.logical_n = logical_n;
  config.padded_n = layout.padded_n;
  config.k = k;
  const auto bits = number(arguments, "--comparison-bits");
  if (bits > std::numeric_limits<std::uint8_t>::max()) fail("candidate comparison bits");
  config.comparison_bits = static_cast<std::uint8_t>(bits);
  config.rank_bits = layout.index_bits;
  return config;
}

int dealer_main(const Arguments& arguments) {
  const auto config = public_config(arguments);
  const int package0_fd = descriptor(arguments, "--package0-fd");
  const int package1_fd = descriptor(arguments, "--package1-fd");
  close_except({package0_fd, package1_fd});
  auto packages = protocol_i_dealer_candidate_preprocess(config);
  const auto package0 = serialize_dealer_candidate_package(packages.party0);
  const auto package1 = serialize_dealer_candidate_package(packages.party1);
  {
    ProtocolIFramedChannel channel(
        package0_fd, {config.session, config.fingerprint, config.padded_n, config.k,
                      config.comparison_bits, 2, 0, 1, 1}, timeout_ms(arguments));
    protocol_i_send_framed_chunks(channel, package0);
  }
  {
    ProtocolIFramedChannel channel(
        package1_fd, {config.session, config.fingerprint, config.padded_n, config.k,
                      config.comparison_bits, 2, 1, 1, 1}, timeout_ms(arguments));
    protocol_i_send_framed_chunks(channel, package1);
  }
  return 0;
}

int party_main(const Arguments& arguments, int party) {
  const auto public_parameters = public_config(arguments);
  ProtocolIDealerCandidateConfig config;
  static_cast<ProtocolIDealerCandidatePublicConfig&>(config) = public_parameters;
  config.party = static_cast<std::uint8_t>(party);
  const int package_fd = descriptor(arguments, "--package-fd");
  const int input_fd = descriptor(arguments, "--input-fd");
  const int masked_open_fd = descriptor(arguments, "--masked-open-fd");
  const int result_fd = descriptor(arguments, "--result-fd");
  const int ready_fd = descriptor(arguments, "--ready-fd");
  std::array<int, 4> offline_fds{};
  std::array<int, 2> forward_fds{};
  std::vector<int> keep{package_fd, input_fd, masked_open_fd, result_fd, ready_fd};
  for (unsigned index = 0; index < offline_fds.size(); ++index) {
    offline_fds[index] = descriptor(arguments, ("--offline-fd-" + std::to_string(index)).c_str());
    keep.push_back(offline_fds[index]);
  }
  for (unsigned index = 0; index < forward_fds.size(); ++index) {
    forward_fds[index] = descriptor(arguments, ("--forward-fd-" + std::to_string(index)).c_str());
    keep.push_back(forward_fds[index]);
  }
  close_except(keep);

  ProtocolIDealerCandidatePackage package;
  {
    ProtocolIFramedChannel channel(
        package_fd, {config.session, config.fingerprint, config.padded_n, config.k,
                     config.comparison_bits, static_cast<std::uint8_t>(party), 2, 1, 1},
        timeout_ms(arguments));
    const auto encoded_package = protocol_i_receive_framed_chunks(channel, 64U * 1024U * 1024U);
    package = deserialize_dealer_candidate_package(encoded_package, party);
    package.serialized_bytes = channel.received_bytes();
  }

  const auto mode = number(arguments, "--permutation-mode");
  const auto seed = arguments.optional("--permutation-seed", "0");
  const auto permutation_seed = seed == "0" ? random_value() : std::stoull(seed);
  const auto permutation = local_permutation(
      config.padded_n, static_cast<unsigned>(mode), permutation_seed);
  if (config.material_id > std::numeric_limits<std::uint64_t>::max() - 0x200U) {
    fail("candidate shuffle material id");
  }
  ProtocolIShufflePartyConfig shuffle_config{
      config.session, config.fingerprint, config.material_id + 0x100U,
      config.material_id + 0x200U, config.padded_n, 2,
      static_cast<std::uint8_t>(party), timeout_ms(arguments)};
  auto shuffle_material = protocol_i_shuffle_preprocess_party(
      shuffle_config, offline_fds, permutation);

  {
    ProtocolIFramedChannel ready(
        ready_fd,
        {config.session, config.fingerprint, config.padded_n, config.k,
         config.comparison_bits, static_cast<std::uint8_t>(party), 2, 2, 1},
        timeout_ms(arguments));
    ready.send({1});
  }

  std::vector<std::uint64_t> priority_key_share;
  {
    ProtocolIFramedChannel input(
        input_fd, {config.session, config.fingerprint, config.padded_n, config.k,
                   config.comparison_bits, static_cast<std::uint8_t>(party), 2, 4, 1},
        timeout_ms(arguments));
    priority_key_share = decode_words(input.receive(), config.padded_n);
  }

  ProtocolIDealerCandidateCoreConfig core_config;
  static_cast<ProtocolIDealerCandidateConfig&>(core_config) = config;
  core_config.timeout_ms = timeout_ms(arguments);
  const auto output = protocol_i_dealer_candidate_core_party(
      core_config, std::move(package), shuffle_material, priority_key_share,
      forward_fds, masked_open_fd);
  {
    ProtocolIFramedChannel result(
        result_fd, {config.session, config.fingerprint, config.padded_n, config.k,
                    config.comparison_bits, static_cast<std::uint8_t>(party), 2, 6, 1},
        timeout_ms(arguments));
    result.send(serialize_dealer_candidate_result(core_config, output));
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const Arguments arguments(argc, argv);
    const auto role = arguments.required("--role");
    if (role == "dealer") return dealer_main(arguments);
    if (role == "party0") return party_main(arguments, 0);
    if (role == "party1") return party_main(arguments, 1);
    fail("candidate role");
  } catch (const std::exception& error) {
    return (void)write(STDERR_FILENO, error.what(), std::strlen(error.what())), 1;
  }
}
