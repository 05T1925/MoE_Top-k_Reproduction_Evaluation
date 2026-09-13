#include <moe_topk/protocol_i_dealer_candidate_core.h>
#include <moe_topk/protocol_i_dealer_candidate_package.h>
#include <moe_topk/protocol_i_dealer_candidate_route_a.h>
#include <moe_topk/protocol_i_raw_score_route_a.h>
#include <moe_topk/protocol_i_score_input.h>
#include <moe_topk/protocol_i_permutation.h>
#include <moe_topk/protocol_i_pipeline.h>
#include <moe_topk/protocol_i_transport.h>

#include <FSS/config.h>
#include <cryptoTools/Common/Defines.h>

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

void initialize_fss_prngs() {
  for (int index = 0; index < 256; ++index) {
    std::array<std::uint64_t, 2> seed{};
    if (RAND_bytes(reinterpret_cast<unsigned char*>(seed.data()), sizeof(seed)) != 1) {
      fail("candidate FSS randomness");
    }
    FSSConfig::prngs[index].SetSeed(osuCrypto::toBlock(seed[0], seed[1]));
  }
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

std::vector<std::uint8_t> encode_route_result(
    const ProtocolIDealerCandidatePublicConfig& config,
    const ProtocolIDealerCandidateRouteAOutput& output) {
  std::vector<std::uint8_t> bytes;
  bytes.insert(bytes.end(), {'R', 'A', '6', 'M', 1});
  auto put = [&bytes](std::uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8)
      bytes.push_back(static_cast<std::uint8_t>(value >> shift));
  };
  put(config.material_id);
  put(output.xor_mask_share.size());
  put(output.rank_reveal_sent_bytes);
  put(output.rank_reveal_received_bytes);
  put(output.reverse_sent_bytes);
  put(output.reverse_received_bytes);
  put(output.online_rounds);
  bytes.insert(bytes.end(), output.xor_mask_share.begin(), output.xor_mask_share.end());
  return bytes;
}

std::vector<std::uint8_t> encode_raw_route_result(
    const ProtocolIDealerCandidatePublicConfig& config,
    const ProtocolIPriorityPipelineOutput& output,
    const ProtocolIScoreInputMetrics& score_metrics) {
  std::vector<std::uint8_t> bytes;
  bytes.insert(bytes.end(), {'R', 'A', '8', 'M', 1});
  auto put = [&bytes](std::uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8)
      bytes.push_back(static_cast<std::uint8_t>(value >> shift));
  };
  put(config.material_id);
  put(output.xor_mask_share.size());
  put(score_metrics.carry_sent_bytes);
  put(score_metrics.carry_received_bytes);
  put(score_metrics.sign_sent_bytes);
  put(score_metrics.sign_received_bytes);
  put(output.metrics.forward_sent_bytes);
  put(output.metrics.forward_received_bytes);
  put(output.metrics.cmpagg_sent_bytes);
  put(output.metrics.cmpagg_received_bytes);
  put(output.metrics.rank_reveal_sent_bytes);
  put(output.metrics.rank_reveal_received_bytes);
  put(output.metrics.reverse_sent_bytes);
  put(output.metrics.reverse_received_bytes);
  put(score_metrics.rounds + output.metrics.online_rounds);
  bytes.insert(bytes.end(), output.xor_mask_share.begin(), output.xor_mask_share.end());
  return bytes;
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
  const bool raw_route = arguments.optional("--raw-score-route-a", "0") == "1";
  if (raw_route) {
    auto packages = protocol_i_raw_score_route_a_preprocess(config);
    const auto package0 = serialize_party_package(packages.party0);
    const auto package1 = serialize_party_package(packages.party1);
    ProtocolIFramedChannel channel0(
        package0_fd, {config.session, config.fingerprint, config.padded_n, config.k,
                      config.comparison_bits, 2, 0, 1, 1}, timeout_ms(arguments));
    ProtocolIFramedChannel channel1(
        package1_fd, {config.session, config.fingerprint, config.padded_n, config.k,
                      config.comparison_bits, 2, 1, 1, 1}, timeout_ms(arguments));
    protocol_i_send_framed_chunks(channel0, package0);
    protocol_i_send_framed_chunks(channel1, package1);
    return 0;
  }
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
  const bool route_a = arguments.optional("--route-a", "0") == "1";
  const bool raw_route = arguments.optional("--raw-score-route-a", "0") == "1";
  if (raw_route && !route_a) fail("raw Route A requires route mode");
  const int rank_reveal_fd = route_a ? descriptor(arguments, "--rank-reveal-fd") : -1;
  const int route_result_fd = route_a ? descriptor(arguments, "--route-result-fd") : -1;
  const std::array<int, 2> score_fds{
      raw_route ? descriptor(arguments, "--score-fd-0") : -1,
      raw_route ? descriptor(arguments, "--score-fd-1") : -1};
  std::array<int, 4> offline_fds{};
  std::array<int, 2> forward_fds{};
  std::array<int, 2> reverse_fds{};
  std::vector<int> keep{package_fd, input_fd, masked_open_fd, result_fd, ready_fd};
  if (route_a) { keep.push_back(rank_reveal_fd); keep.push_back(route_result_fd); }
  if (raw_route) { keep.push_back(score_fds[0]); keep.push_back(score_fds[1]); }
  for (unsigned index = 0; index < offline_fds.size(); ++index) {
    offline_fds[index] = descriptor(arguments, ("--offline-fd-" + std::to_string(index)).c_str());
    keep.push_back(offline_fds[index]);
  }
  for (unsigned index = 0; index < forward_fds.size(); ++index) {
    forward_fds[index] = descriptor(arguments, ("--forward-fd-" + std::to_string(index)).c_str());
    keep.push_back(forward_fds[index]);
  }
  if (route_a) {
    for (unsigned index = 0; index < reverse_fds.size(); ++index) {
      reverse_fds[index] = descriptor(arguments, ("--reverse-fd-" + std::to_string(index)).c_str());
      keep.push_back(reverse_fds[index]);
    }
  }
  close_except(keep);

  const auto package_config = ProtocolIFrameConfig{
      config.session, config.fingerprint, config.padded_n, config.k,
      config.comparison_bits, static_cast<std::uint8_t>(party), 2, 1, 1};
  std::vector<std::uint8_t> encoded_package;
  std::uint64_t package_bytes = 0;
  {
    ProtocolIFramedChannel channel(package_fd, package_config, timeout_ms(arguments));
    encoded_package = protocol_i_receive_framed_chunks(channel, 64U * 1024U * 1024U);
    package_bytes = channel.received_bytes();
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
    priority_key_share = decode_words(input.receive(), raw_route ? config.logical_n : config.padded_n);
  }

  ProtocolIDealerCandidateCoreConfig core_config;
  static_cast<ProtocolIDealerCandidateConfig&>(core_config) = config;
  core_config.timeout_ms = timeout_ms(arguments);
  if (raw_route) {
    auto package = deserialize_party_package(encoded_package, party);
    std::vector<std::uint32_t> raw_score_shares;
    raw_score_shares.reserve(priority_key_share.size());
    for (const auto share : priority_key_share) {
      if (share > UINT32_MAX) fail("raw score share width");
      raw_score_shares.push_back(static_cast<std::uint32_t>(share));
    }
    ProtocolIScoreInputMetrics score_metrics;
    const auto keys = protocol_i_raw_score_input_party(
        {config.session, config.fingerprint, config.logical_n, config.padded_n, config.k,
         config.rank_bits, config.comparison_bits, static_cast<std::uint8_t>(party),
         core_config.timeout_ms}, package, raw_score_shares, score_fds, &score_metrics);
    const auto route_output = protocol_i_dealer_candidate_route_a_priority_party(
        {config.session, config.fingerprint, config.logical_n, config.padded_n, config.k,
         config.comparison_bits, static_cast<std::uint8_t>(party), core_config.timeout_ms},
        std::move(package), shuffle_material, keys, forward_fds, masked_open_fd,
        rank_reveal_fd, reverse_fds);
    ProtocolIFramedChannel result(
        route_result_fd, {config.session, config.fingerprint, config.padded_n, config.k,
                          config.comparison_bits, static_cast<std::uint8_t>(party), 2, 8, 1},
        timeout_ms(arguments));
    result.send(encode_raw_route_result(config, route_output, score_metrics));
    return 0;
  }
  ProtocolIDealerCandidatePackage package =
      deserialize_dealer_candidate_package(encoded_package, party);
  package.serialized_bytes = package_bytes;
  if (route_a) {
    auto route_output = protocol_i_dealer_candidate_route_a_party(
        core_config, std::move(package), shuffle_material,
        priority_key_share, forward_fds, masked_open_fd, rank_reveal_fd, reverse_fds);
    ProtocolIFramedChannel result(
        route_result_fd, {config.session, config.fingerprint, config.padded_n, config.k,
                          config.comparison_bits, static_cast<std::uint8_t>(party), 2, 7, 1},
        timeout_ms(arguments));
    result.send(encode_route_result(config, route_output));
    return 0;
  }
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
    initialize_fss_prngs();
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
