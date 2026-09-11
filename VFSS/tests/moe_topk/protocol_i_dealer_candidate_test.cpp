#include <moe_topk/protocol_i_dealer_candidate_core.h>
#include <moe_topk/protocol_i_dealer_candidate_package.h>
#include <moe_topk/protocol_i_permutation.h>
#include <moe_topk/protocol_i_pipeline.h>
#include <moe_topk/protocol_i_priority_key.h>
#include <moe_topk/protocol_i_transport.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#ifndef MOE_TOPK_M2_DEALER_CANDIDATE_EXECUTABLE
#error "candidate executable path is required"
#endif

namespace {
using namespace moe_topk;
constexpr int kTimeoutMs = 15000;

[[noreturn]] void fail(const char* message) { throw std::runtime_error(message); }
void require(bool condition, const char* message) { if (!condition) fail(message); }

struct FdPair {
  int first = -1, second = -1;
};

void close_fd(int& fd) {
  if (fd >= 0) {
    (void)::close(fd);
    fd = -1;
  }
}

int take_fd(int& fd) {
  const auto result = fd;
  fd = -1;
  return result;
}

void make_socket_pair(FdPair& pair) {
  int fds[2]{};
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0, "candidate socketpair");
  for (const auto fd : fds) {
    const auto flags = ::fcntl(fd, F_GETFD);
    require(flags >= 0 && ::fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == 0,
            "candidate socket fd flags");
  }
  pair = {fds[0], fds[1]};
}

bool contains(const std::vector<int>& values, int fd) {
  return std::find(values.begin(), values.end(), fd) != values.end();
}

void prepare_child(const std::vector<int>& all_fds, const std::vector<int>& keep) {
  for (const auto fd : all_fds) {
    if (fd >= 0 && !contains(keep, fd)) (void)::close(fd);
  }
  for (const auto fd : keep) {
    const auto flags = ::fcntl(fd, F_GETFD);
    require(flags >= 0 && ::fcntl(fd, F_SETFD, flags & ~FD_CLOEXEC) == 0,
            "candidate child fd inheritance");
  }
}

pid_t spawn_role(const std::string& executable, const std::vector<std::string>& arguments,
                const std::vector<int>& all_fds, const std::vector<int>& keep) {
  const auto child = ::fork();
  require(child >= 0, "candidate fork");
  if (child != 0) return child;
  try {
    prepare_child(all_fds, keep);
    std::vector<char*> argv;
    argv.reserve(arguments.size() + 2U);
    argv.push_back(const_cast<char*>(executable.c_str()));
    for (const auto& argument : arguments) argv.push_back(const_cast<char*>(argument.c_str()));
    argv.push_back(nullptr);
    ::execv(executable.c_str(), argv.data());
  } catch (...) {
  }
  _exit(127);
}

void wait_ok(pid_t child, const char* message) {
  int status = 0;
  require(::waitpid(child, &status, 0) == child, message);
  require(WIFEXITED(status) && WEXITSTATUS(status) == 0, message);
}

void add_arg(std::vector<std::string>& args, const char* key, std::uint64_t value) {
  args.emplace_back(key);
  args.emplace_back(std::to_string(value));
}

std::vector<std::uint8_t> encode_words(const std::vector<std::uint64_t>& words) {
  std::vector<std::uint8_t> bytes;
  bytes.reserve(words.size() * sizeof(std::uint64_t));
  for (const auto word : words) {
    for (int shift = 56; shift >= 0; shift -= 8) {
      bytes.push_back(static_cast<std::uint8_t>(word >> shift));
    }
  }
  return bytes;
}

ProtocolIPermutation local_permutation(std::uint32_t n, unsigned mode, std::uint64_t seed) {
  auto permutation = protocol_i_identity_permutation(n);
  if (mode == 0) return permutation;
  if (mode == 1) {
    std::reverse(permutation.begin(), permutation.end());
    return permutation;
  }
  require(mode == 2, "candidate test permutation mode");
  std::mt19937_64 random(seed);
  std::shuffle(permutation.begin(), permutation.end(), random);
  return permutation;
}

std::vector<std::uint32_t> scores_for(std::uint32_t n, unsigned style, std::uint64_t seed) {
  std::vector<std::uint32_t> scores(n);
  std::mt19937_64 random(seed);
  for (auto& score : scores) score = static_cast<std::uint32_t>(random());
  if (style == 1) std::fill(scores.begin(), scores.end(), UINT32_C(7));
  if (style == 2) {
    for (std::size_t index = 0; index < scores.size(); ++index) {
      scores[index] = index % 2U == 0 ? UINT32_C(0x80000000) : UINT32_C(0x7fffffff);
    }
  }
  if (style == 3) std::fill(scores.begin(), scores.end(), UINT32_C(0x80000000));
  if (style == 4) {
    for (std::size_t index = 0; index < scores.size(); ++index) {
      scores[index] = static_cast<std::uint32_t>(index);
    }
  }
  return scores;
}

std::vector<std::uint64_t> priority_keys(const std::vector<std::uint32_t>& scores,
                                         std::uint32_t logical_n,
                                         std::uint32_t padded_n) {
  const auto index_bits = protocol_i_index_bits(logical_n);
  const auto max_index = (UINT64_C(1) << index_bits) - 1U;
  const auto dummy = (static_cast<std::uint64_t>(UINT32_MAX) << index_bits) | max_index;
  std::vector<std::uint64_t> keys;
  keys.reserve(padded_n);
  for (std::uint32_t index = 0; index < logical_n; ++index) {
    keys.push_back(protocol_i_priority_key(scores[index], index, logical_n).value);
  }
  while (keys.size() < padded_n) keys.push_back(dummy);
  return keys;
}

std::pair<std::vector<std::uint64_t>, std::vector<std::uint64_t>> split_keys(
    const std::vector<std::uint64_t>& keys, std::uint8_t bits, std::uint64_t seed) {
  const auto mask = (UINT64_C(1) << bits) - 1U;
  std::mt19937_64 random(seed);
  std::vector<std::uint64_t> left(keys.size()), right(keys.size());
  for (std::size_t index = 0; index < keys.size(); ++index) {
    left[index] = random() & mask;
    right[index] = (keys[index] - left[index]) & mask;
  }
  return {std::move(left), std::move(right)};
}

std::vector<std::uint64_t> expected_ranks(const std::vector<std::uint64_t>& shuffled_keys) {
  std::vector<std::uint64_t> ranks(shuffled_keys.size());
  for (std::size_t index = 0; index < shuffled_keys.size(); ++index) {
    for (const auto other : shuffled_keys) ranks[index] += other < shuffled_keys[index] ? 1U : 0U;
  }
  return ranks;
}

ProtocolIDealerCandidateCoreConfig core_config(const ProtocolIDealerCandidatePublicConfig& public_config,
                                               int party) {
  ProtocolIDealerCandidateCoreConfig config;
  static_cast<ProtocolIDealerCandidatePublicConfig&>(config) = public_config;
  config.party = static_cast<std::uint8_t>(party);
  config.timeout_ms = kTimeoutMs;
  return config;
}

void package_conformance() {
  const auto layout = protocol_i_make_input_layout(1, 1);
  const ProtocolIDealerCandidatePublicConfig config{
      0x710001, 0x720001, 0x730001, 1, layout.padded_n, 1,
      layout.minimum_comparison_bits, layout.index_bits};
  auto packages = protocol_i_dealer_candidate_preprocess(config);
  const auto bytes0 = serialize_dealer_candidate_package(packages.party0);
  const auto bytes1 = serialize_dealer_candidate_package(packages.party1);
  auto decoded0 = deserialize_dealer_candidate_package(bytes0, 0);
  auto decoded1 = deserialize_dealer_candidate_package(bytes1, 1);
  require(decoded0.serialized_bytes == bytes0.size() && decoded1.serialized_bytes == bytes1.size(),
          "candidate package byte accounting");
  require(decoded0.config.material_id == config.material_id && decoded1.config.party == 1,
          "candidate package identity");
  require(decoded0.r_share.size() == layout.padded_n && decoded0.edge_materials.size() == 1,
          "candidate package shape");
  const auto ring = (UINT64_C(1) << config.comparison_bits) - 1U;
  std::vector<std::uint64_t> clear_r(layout.padded_n);
  for (std::size_t index = 0; index < clear_r.size(); ++index) {
    clear_r[index] = (decoded0.r_share[index] + decoded1.r_share[index]) & ring;
  }
  const auto clear_left = UINT64_C(3), clear_right = UINT64_C(9);
  const auto masked_left = (clear_left + clear_r[0]) & ring;
  const auto masked_right = (clear_right + clear_r[1]) & ring;
  const auto party0_less = decoded0.edge_materials.front().material.eval_strict_lt(
      masked_left, masked_right);
  const auto party1_less = decoded1.edge_materials.front().material.eval_strict_lt(
      masked_left, masked_right);
  require(party0_less + party1_less == (clear_left < clear_right),
          "candidate correlated r primitive");

  bool threw = false;
  auto bad = bytes0;
  bad[0] ^= 1U;
  try { (void)deserialize_dealer_candidate_package(bad, 0); } catch (...) { threw = true; }
  require(threw, "candidate package magic negative");
  threw = false;
  bad = bytes0;
  bad.pop_back();
  try { (void)deserialize_dealer_candidate_package(bad, 0); } catch (...) { threw = true; }
  require(threw, "candidate package truncation negative");
  threw = false;
  try { (void)deserialize_dealer_candidate_package(bytes0, 1); } catch (...) { threw = true; }
  require(threw, "candidate package party negative");

  auto replay = deserialize_dealer_candidate_package(bytes0, 0);
  auto material = std::move(replay.edge_materials.front().material);
  (void)material.eval_strict_lt(0, 0);
  threw = false;
  try { (void)material.eval_strict_lt(0, 0); } catch (...) { threw = true; }
  require(threw, "candidate material replay negative");
}

void transport_negative() {
  const ProtocolIFrameConfig config{0x740001, 0x750001, 2, 1, 34, 0, 1, 3, 1};
  FdPair pair;
  make_socket_pair(pair);
  {
    ProtocolIFramedChannel channel(take_fd(pair.first), config, kTimeoutMs);
    close_fd(pair.second);
    bool threw = false;
    try { (void)channel.receive(); } catch (...) { threw = true; }
    require(threw, "candidate peer-exit negative");
  }

  make_socket_pair(pair);
  {
    ProtocolIFramedChannel channel(take_fd(pair.first), config, 5);
    bool threw = false;
    try { (void)channel.receive(); } catch (...) { threw = true; }
    require(threw, "candidate timeout negative");
  }
  close_fd(pair.second);
}

struct CaseChannels {
  std::array<FdPair, 4> offline{};
  std::array<FdPair, 2> forward{};
  FdPair package0{}, package1{}, ready0{}, ready1{}, input0{}, input1{}, masked{}, result0{}, result1{};
};

std::vector<int> all_fds(const CaseChannels& channels) {
  std::vector<int> values;
  auto add = [&values](const FdPair& pair) {
    values.push_back(pair.first);
    values.push_back(pair.second);
  };
  for (const auto& pair : channels.offline) add(pair);
  for (const auto& pair : channels.forward) add(pair);
  for (const auto* pair : {&channels.package0, &channels.package1, &channels.ready0, &channels.ready1,
                           &channels.input0, &channels.input1, &channels.masked, &channels.result0,
                           &channels.result1}) add(*pair);
  return values;
}

void close_all(CaseChannels& channels) {
  auto close = [](FdPair& pair) { close_fd(pair.first); close_fd(pair.second); };
  for (auto& pair : channels.offline) close(pair);
  for (auto& pair : channels.forward) close(pair);
  close(channels.package0); close(channels.package1); close(channels.ready0); close(channels.ready1);
  close(channels.input0); close(channels.input1); close(channels.masked);
  close(channels.result0); close(channels.result1);
}

std::vector<std::string> dealer_args(const ProtocolIDealerCandidatePublicConfig& config,
                                     const CaseChannels& channels) {
  std::vector<std::string> args{"--role", "dealer"};
  add_arg(args, "--package0-fd", channels.package0.first);
  add_arg(args, "--package1-fd", channels.package1.first);
  add_arg(args, "--logical-n", config.logical_n);
  add_arg(args, "--k", config.k);
  add_arg(args, "--session", config.session);
  add_arg(args, "--fingerprint", config.fingerprint);
  add_arg(args, "--material-id", config.material_id);
  add_arg(args, "--comparison-bits", config.comparison_bits);
  add_arg(args, "--timeout-ms", kTimeoutMs);
  return args;
}

std::vector<std::string> party_args(const ProtocolIDealerCandidatePublicConfig& config,
                                    const CaseChannels& channels, int party,
                                    unsigned permutation_mode, std::uint64_t permutation_seed) {
  const auto& package = party == 0 ? channels.package0 : channels.package1;
  const auto& ready = party == 0 ? channels.ready0 : channels.ready1;
  const auto& input = party == 0 ? channels.input0 : channels.input1;
  const auto& result = party == 0 ? channels.result0 : channels.result1;
  std::vector<std::string> args{"--role", party == 0 ? "party0" : "party1"};
  add_arg(args, "--package-fd", package.second);
  add_arg(args, "--ready-fd", ready.first);
  add_arg(args, "--input-fd", input.first);
  add_arg(args, "--masked-open-fd", party == 0 ? channels.masked.first : channels.masked.second);
  add_arg(args, "--result-fd", result.first);
  for (unsigned index = 0; index < channels.offline.size(); ++index) {
    add_arg(args, ("--offline-fd-" + std::to_string(index)).c_str(),
            party == 0 ? channels.offline[index].first : channels.offline[index].second);
  }
  for (unsigned index = 0; index < channels.forward.size(); ++index) {
    add_arg(args, ("--forward-fd-" + std::to_string(index)).c_str(),
            party == 0 ? channels.forward[index].first : channels.forward[index].second);
  }
  add_arg(args, "--logical-n", config.logical_n);
  add_arg(args, "--k", config.k);
  add_arg(args, "--session", config.session);
  add_arg(args, "--fingerprint", config.fingerprint);
  add_arg(args, "--material-id", config.material_id);
  add_arg(args, "--comparison-bits", config.comparison_bits);
  add_arg(args, "--timeout-ms", kTimeoutMs);
  add_arg(args, "--permutation-mode", permutation_mode);
  add_arg(args, "--permutation-seed", permutation_seed);
  return args;
}

void run_case(std::uint32_t logical_n, std::uint32_t k, unsigned style, unsigned permutation_mode,
              std::uint64_t serial) {
  const auto layout = protocol_i_make_input_layout(logical_n, k);
  const ProtocolIDealerCandidatePublicConfig config{
      0x810000 + serial, 0x820000 + serial, 0x830000 + serial, logical_n,
      layout.padded_n, k, layout.minimum_comparison_bits, layout.index_bits};
  const auto scores = scores_for(logical_n, style, 0x910000 + serial);
  const auto keys = priority_keys(scores, logical_n, layout.padded_n);
  const auto shares = split_keys(keys, config.comparison_bits, 0xa10000 + serial);
  const auto p0 = local_permutation(layout.padded_n, permutation_mode, 0xb10000 + serial);
  const auto p1 = local_permutation(layout.padded_n, permutation_mode, 0xc10000 + serial);
  const auto composite = protocol_i_compose_permutation(p1, p0);
  const auto shuffled_keys = protocol_i_apply_permutation(composite, keys);
  const auto expected = expected_ranks(shuffled_keys);

  CaseChannels channels;
  for (auto& pair : channels.offline) make_socket_pair(pair);
  for (auto& pair : channels.forward) make_socket_pair(pair);
  make_socket_pair(channels.package0); make_socket_pair(channels.package1);
  make_socket_pair(channels.ready0); make_socket_pair(channels.ready1);
  make_socket_pair(channels.input0); make_socket_pair(channels.input1);
  make_socket_pair(channels.masked); make_socket_pair(channels.result0); make_socket_pair(channels.result1);
  const auto all = all_fds(channels);

  const std::string executable = MOE_TOPK_M2_DEALER_CANDIDATE_EXECUTABLE;
  const auto dealer = spawn_role(executable, dealer_args(config, channels), all,
                                 {channels.package0.first, channels.package1.first});
  close_fd(channels.package0.first); close_fd(channels.package1.first);
  wait_ok(dealer, "candidate dealer process");

  const auto party0 = spawn_role(executable, party_args(config, channels, 0, permutation_mode,
                                                       0xb10000 + serial), all,
                                 [&] {
                                   std::vector<int> keep{channels.package0.second, channels.ready0.first,
                                                         channels.input0.first, channels.masked.first,
                                                         channels.result0.first};
                                   for (const auto& pair : channels.offline) keep.push_back(pair.first);
                                   for (const auto& pair : channels.forward) keep.push_back(pair.first);
                                   return keep;
                                 }());
  const auto party1 = spawn_role(executable, party_args(config, channels, 1, permutation_mode,
                                                       0xc10000 + serial), all,
                                 [&] {
                                   std::vector<int> keep{channels.package1.second, channels.ready1.first,
                                                         channels.input1.first, channels.masked.second,
                                                         channels.result1.first};
                                   for (const auto& pair : channels.offline) keep.push_back(pair.second);
                                   for (const auto& pair : channels.forward) keep.push_back(pair.second);
                                   return keep;
                                 }());
  close_fd(channels.package0.second); close_fd(channels.package1.second);
  close_fd(channels.ready0.first); close_fd(channels.ready1.first);
  close_fd(channels.input0.first); close_fd(channels.input1.first);
  close_fd(channels.result0.first); close_fd(channels.result1.first);
  for (auto& pair : channels.offline) { close_fd(pair.first); close_fd(pair.second); }
  for (auto& pair : channels.forward) { close_fd(pair.first); close_fd(pair.second); }
  close_fd(channels.masked.first); close_fd(channels.masked.second);

  const auto receive_config = [&](int party, int phase) {
    return ProtocolIFrameConfig{config.session, config.fingerprint, config.padded_n, config.k,
                                config.comparison_bits, 2, static_cast<std::uint8_t>(party),
                                static_cast<std::uint8_t>(phase), 1};
  };
  {
    ProtocolIFramedChannel ready0(take_fd(channels.ready0.second), receive_config(0, 2), kTimeoutMs);
    ProtocolIFramedChannel ready1(take_fd(channels.ready1.second), receive_config(1, 2), kTimeoutMs);
    require(ready0.receive() == std::vector<std::uint8_t>{1} &&
                ready1.receive() == std::vector<std::uint8_t>{1},
            "candidate offline readiness");
  }
  {
    ProtocolIFramedChannel input0(take_fd(channels.input0.second), receive_config(0, 4), kTimeoutMs);
    ProtocolIFramedChannel input1(take_fd(channels.input1.second), receive_config(1, 4), kTimeoutMs);
    input0.send(encode_words(shares.first));
    input1.send(encode_words(shares.second));
  }

  ProtocolIDealerCandidateOutput output0, output1;
  {
    ProtocolIFramedChannel result0(take_fd(channels.result0.second), receive_config(0, 6), kTimeoutMs);
    ProtocolIFramedChannel result1(take_fd(channels.result1.second), receive_config(1, 6), kTimeoutMs);
    output0 = deserialize_dealer_candidate_result(
        result0.receive(), core_config(config, 0), 0);
    output1 = deserialize_dealer_candidate_result(
        result1.receive(), core_config(config, 1), 1);
  }
  wait_ok(party0, "candidate party 0 process");
  wait_ok(party1, "candidate party 1 process");
  close_all(channels);

  require(output0.public_masked_list == output1.public_masked_list, "candidate public y agreement");
  require(output0.metrics.forward_online_rounds == 2 && output1.metrics.forward_online_rounds == 2,
          "candidate forward round accounting");
  require(output0.metrics.masked_open_rounds == 1 && output1.metrics.masked_open_rounds == 1,
          "candidate masked opening accounting");
  require(output0.metrics.online_rounds == output0.metrics.forward_online_rounds +
              output0.metrics.masked_open_rounds &&
              output1.metrics.online_rounds == output1.metrics.forward_online_rounds +
              output1.metrics.masked_open_rounds,
          "candidate causal round accounting");
  require(output0.metrics.r1_received_bytes > 0 && output1.metrics.r1_sent_bytes > 0 &&
              output0.metrics.r2_sent_bytes > 0 && output1.metrics.r2_received_bytes > 0 &&
              output0.metrics.r3_sent_bytes > 0 && output1.metrics.r3_sent_bytes > 0,
          "candidate transport trace bytes");
  require(output0.metrics.comparison_edges ==
              static_cast<std::uint64_t>(layout.padded_n) * (layout.padded_n - 1U) / 2U &&
              output0.metrics.raw_dcf_calls == output0.metrics.comparison_edges * 2U,
          "candidate comparison metrics");
  const auto ring = (UINT64_C(1) << config.comparison_bits) - 1U;
  for (std::size_t index = 0; index < layout.padded_n; ++index) {
    require((output0.public_masked_list[index] & ~ring) == 0, "candidate public y ring");
    const auto rank = (output0.shuffled_rank_share[index] + output1.shuffled_rank_share[index]) & ring;
    require(rank == expected[index], "candidate rank differential");
  }
}

}  // namespace

int main() {
  try {
    package_conformance();
    transport_negative();
    std::uint64_t serial = 1;
    for (const auto logical_n : {1U, 2U, 3U, 5U, 7U, 8U}) {
      const std::vector<std::uint32_t> ks{1U, (logical_n + 1U) / 2U, logical_n};
      for (const auto k : ks) {
        const auto case_serial = serial++;
        run_case(logical_n, k, static_cast<unsigned>(case_serial % 5U),
                 static_cast<unsigned>(case_serial % 3U), case_serial);
      }
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
