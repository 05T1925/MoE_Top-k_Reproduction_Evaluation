#include <moe_topk/protocol_i_dealer_candidate_package.h>
#include <moe_topk/protocol_i_party_package.h>
#include <moe_topk/protocol_i_pipeline.h>
#include <moe_topk/protocol_i_raw_score_route_a.h>
#include <moe_topk/protocol_i_transport.h>
#include <FSS/config.h>
#include <cryptoTools/Common/Defines.h>

#include <algorithm>
#include <array>
#include <cerrno>
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

[[noreturn]] void fail(const std::string& message) { throw std::runtime_error(message); }
void require(bool condition, const char* message) { if (!condition) fail(message); }

struct FdPair { int first = -1, second = -1; };
void close_fd(int& fd) { if (fd >= 0) { (void)::close(fd); fd = -1; } }
int take_fd(int& fd) { const auto result = fd; fd = -1; return result; }

void socket_pair(FdPair& pair) {
  int fds[2]{};
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0, "raw Route A socketpair");
  for (const auto fd : fds) {
    const auto flags = ::fcntl(fd, F_GETFD);
    require(flags >= 0 && ::fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == 0,
            "raw Route A socket flags");
  }
  pair = {fds[0], fds[1]};
}

struct Channels {
  std::array<FdPair, 4> offline{};
  std::array<FdPair, 2> forward{}, reverse{}, score{};
  FdPair package0{}, package1{}, ready0{}, ready1{}, input0{}, input1{}, masked{}, rank{};
  FdPair result0{}, result1{}, route_result0{}, route_result1{};
};

std::vector<int> all_fds(const Channels& c) {
  std::vector<int> result;
  auto add = [&result](const FdPair& p) { if (p.first >= 0) result.push_back(p.first); if (p.second >= 0) result.push_back(p.second); };
  for (const auto& p : c.offline) add(p);
  for (const auto& p : c.forward) add(p);
  for (const auto& p : c.reverse) add(p);
  for (const auto& p : c.score) add(p);
  for (const auto* p : {&c.package0, &c.package1, &c.ready0, &c.ready1, &c.input0, &c.input1,
                        &c.masked, &c.rank, &c.result0, &c.result1,
                        &c.route_result0, &c.route_result1}) add(*p);
  return result;
}

void close_all(Channels& c) {
  auto close = [](FdPair& p) { close_fd(p.first); close_fd(p.second); };
  for (auto& p : c.offline) close(p); for (auto& p : c.forward) close(p);
  for (auto& p : c.reverse) close(p); for (auto& p : c.score) close(p);
  close(c.package0); close(c.package1); close(c.ready0); close(c.ready1);
  close(c.input0); close(c.input1); close(c.masked); close(c.rank);
  close(c.result0); close(c.result1); close(c.route_result0); close(c.route_result1);
}

bool contains(const std::vector<int>& values, int fd) {
  return std::find(values.begin(), values.end(), fd) != values.end();
}

void prepare_child(const std::vector<int>& fds, const std::vector<int>& keep) {
  for (const auto fd : fds) if (fd >= 0 && !contains(keep, fd)) (void)::close(fd);
  for (const auto fd : keep) {
    const auto flags = ::fcntl(fd, F_GETFD);
    require(flags >= 0 && ::fcntl(fd, F_SETFD, flags & ~FD_CLOEXEC) == 0,
            "raw Route A child fd inheritance");
  }
}

pid_t spawn(const std::string& executable, const std::vector<std::string>& args,
            const std::vector<int>& fds, const std::vector<int>& keep) {
  const auto child = ::fork(); require(child >= 0, "raw Route A fork");
  if (child != 0) return child;
  try {
    prepare_child(fds, keep);
    std::vector<char*> argv; argv.reserve(args.size() + 2U);
    argv.push_back(const_cast<char*>(executable.c_str()));
    for (const auto& arg : args) argv.push_back(const_cast<char*>(arg.c_str()));
    argv.push_back(nullptr); ::execv(executable.c_str(), argv.data());
  } catch (...) {}
  _exit(127);
}

void wait_ok(pid_t child, const char* message) {
  int status = 0; require(::waitpid(child, &status, 0) == child, message);
  require(WIFEXITED(status) && WEXITSTATUS(status) == 0, message);
}

void add_arg(std::vector<std::string>& args, const std::string& key, std::uint64_t value) {
  args.push_back(key); args.push_back(std::to_string(value));
}

std::vector<std::uint8_t> encode_words(const std::vector<std::uint32_t>& words) {
  std::vector<std::uint8_t> bytes; bytes.reserve(words.size() * 8U);
  for (const auto word : words) for (int shift = 56; shift >= 0; shift -= 8)
    bytes.push_back(static_cast<std::uint8_t>(static_cast<std::uint64_t>(word) >> shift));
  return bytes;
}

ProtocolIDealerCandidatePublicConfig config_for(std::uint32_t n, std::uint32_t k,
                                                std::uint64_t serial) {
  const auto layout = protocol_i_make_input_layout(n, k);
  return {0xA10000 + serial, 0xA20000 + serial, 0xA30000 + serial, n, layout.padded_n, k,
          layout.minimum_comparison_bits, layout.index_bits};
}

std::vector<std::uint32_t> scores_for(std::uint32_t n, unsigned style, std::uint64_t seed) {
  std::vector<std::uint32_t> scores(n); std::mt19937_64 random(seed);
  for (auto& score : scores) score = static_cast<std::uint32_t>(random());
  if (style == 1) std::fill(scores.begin(), scores.end(), UINT32_C(7));
  if (style == 2) for (std::size_t i = 0; i < scores.size(); ++i)
    scores[i] = i % 2U == 0 ? UINT32_C(0x80000000) : UINT32_C(0x7fffffff);
  if (style == 3) std::fill(scores.begin(), scores.end(), UINT32_C(0x80000000));
  if (style == 4) for (std::size_t i = 0; i < scores.size(); ++i)
    scores[i] = static_cast<std::uint32_t>(i);
  return scores;
}

std::pair<std::vector<std::uint32_t>, std::vector<std::uint32_t>> split_scores(
    const std::vector<std::uint32_t>& scores, std::uint64_t seed) {
  std::mt19937_64 random(seed); std::vector<std::uint32_t> left(scores.size()), right(scores.size());
  for (std::size_t i = 0; i < scores.size(); ++i) {
    left[i] = static_cast<std::uint32_t>(random());
    right[i] = scores[i] - left[i];
  }
  return {std::move(left), std::move(right)};
}

std::vector<std::string> dealer_args(const ProtocolIDealerCandidatePublicConfig& c,
                                     const Channels& ch) {
  std::vector<std::string> args{"--role", "dealer", "--raw-score-route-a", "1"};
  add_arg(args, "--package0-fd", ch.package0.first); add_arg(args, "--package1-fd", ch.package1.first);
  add_arg(args, "--logical-n", c.logical_n); add_arg(args, "--k", c.k);
  add_arg(args, "--session", c.session); add_arg(args, "--fingerprint", c.fingerprint);
  add_arg(args, "--material-id", c.material_id); add_arg(args, "--comparison-bits", c.comparison_bits);
  add_arg(args, "--timeout-ms", kTimeoutMs); return args;
}

std::vector<std::string> party_args(const ProtocolIDealerCandidatePublicConfig& c,
                                    const Channels& ch, int party, unsigned mode,
                                    std::uint64_t seed) {
  const auto& package = party == 0 ? ch.package0 : ch.package1;
  const auto& ready = party == 0 ? ch.ready0 : ch.ready1;
  const auto& input = party == 0 ? ch.input0 : ch.input1;
  const auto& result = party == 0 ? ch.result0 : ch.result1;
  const auto& route_result = party == 0 ? ch.route_result0 : ch.route_result1;
  std::vector<std::string> args{"--role", party == 0 ? "party0" : "party1",
                                "--route-a", "1", "--raw-score-route-a", "1"};
  add_arg(args, "--package-fd", package.second); add_arg(args, "--ready-fd", ready.first);
  add_arg(args, "--input-fd", input.first); add_arg(args, "--masked-open-fd", party == 0 ? ch.masked.first : ch.masked.second);
  add_arg(args, "--result-fd", result.first); add_arg(args, "--rank-reveal-fd", party == 0 ? ch.rank.first : ch.rank.second);
  add_arg(args, "--route-result-fd", route_result.first);
  add_arg(args, "--score-fd-0", party == 0 ? ch.score[0].first : ch.score[0].second);
  add_arg(args, "--score-fd-1", party == 0 ? ch.score[1].first : ch.score[1].second);
  for (unsigned i = 0; i < ch.offline.size(); ++i)
    add_arg(args, "--offline-fd-" + std::to_string(i), party == 0 ? ch.offline[i].first : ch.offline[i].second);
  for (unsigned i = 0; i < ch.forward.size(); ++i)
    add_arg(args, "--forward-fd-" + std::to_string(i), party == 0 ? ch.forward[i].first : ch.forward[i].second);
  for (unsigned i = 0; i < ch.reverse.size(); ++i)
    add_arg(args, "--reverse-fd-" + std::to_string(i), party == 0 ? ch.reverse[i].first : ch.reverse[i].second);
  add_arg(args, "--logical-n", c.logical_n); add_arg(args, "--k", c.k);
  add_arg(args, "--session", c.session); add_arg(args, "--fingerprint", c.fingerprint);
  add_arg(args, "--material-id", c.material_id); add_arg(args, "--comparison-bits", c.comparison_bits);
  add_arg(args, "--timeout-ms", kTimeoutMs); add_arg(args, "--permutation-mode", mode);
  add_arg(args, "--permutation-seed", seed); return args;
}

std::vector<std::uint8_t> parse_result(const std::vector<std::uint8_t>& bytes,
                                       std::uint32_t n,
                                       std::uint64_t material_id) {
  require(bytes.size() == 5U + 15U * sizeof(std::uint64_t) + n, "raw Route A result length");
  const std::array<std::uint8_t, 5> identity{'R', 'A', '8', 'M', 1};
  require(std::equal(identity.begin(), identity.end(), bytes.begin()), "raw Route A result identity");
  std::size_t offset = 5;
  auto get = [&]() { std::uint64_t value = 0; for (unsigned i = 0; i < 8; ++i) value = (value << 8U) | bytes[offset++]; return value; };
  require(get() == material_id, "raw Route A result material identity");
  const auto count = get(); const auto carry_sent = get(); const auto carry_received = get();
  const auto sign_sent = get(); const auto sign_received = get();
  const auto forward_sent = get(); const auto forward_received = get();
  const auto cmp_sent = get(); const auto cmp_received = get();
  const auto rank_sent = get(); const auto rank_received = get();
  const auto reverse_sent = get(); const auto reverse_received = get();
  const auto rounds = get();
  require(count == n && rounds == 8 && carry_sent > 0 && carry_received > 0 &&
              sign_sent > 0 && sign_received > 0 && forward_sent > 0 && forward_received > 0 &&
              cmp_sent > 0 && cmp_received > 0 && rank_sent > 0 && rank_received > 0 &&
              reverse_sent > 0 && reverse_received > 0, "raw Route A result metrics");
  return {bytes.begin() + static_cast<std::ptrdiff_t>(offset), bytes.end()};
}

void result_negative() {
  bool rejected = false;
  try { (void)parse_result({}, 1, 1); } catch (...) { rejected = true; }
  require(rejected, "raw Route A result truncation negative");
  std::vector<std::uint8_t> wrong(5U + 15U * sizeof(std::uint64_t) + 1U, 0);
  wrong[0] = 'R'; wrong[1] = 'A'; wrong[2] = '6'; wrong[3] = 'M'; wrong[4] = 1;
  rejected = false;
  try { (void)parse_result(wrong, 1, 1); } catch (...) { rejected = true; }
  require(rejected, "raw Route A result identity negative");
}

void run_case(std::uint32_t n, std::uint32_t k, unsigned style, unsigned mode,
              std::uint64_t serial) {
  const auto config = config_for(n, k, serial); const auto scores = scores_for(n, style, 0xB40000 + serial);
  const auto shares = split_scores(scores, 0xB50000 + serial); Channels ch;
  for (auto& p : ch.offline) socket_pair(p); for (auto& p : ch.forward) socket_pair(p);
  for (auto& p : ch.reverse) socket_pair(p); for (auto& p : ch.score) socket_pair(p);
  socket_pair(ch.package0); socket_pair(ch.package1); socket_pair(ch.ready0); socket_pair(ch.ready1);
  socket_pair(ch.input0); socket_pair(ch.input1); socket_pair(ch.masked); socket_pair(ch.rank);
  socket_pair(ch.result0); socket_pair(ch.result1); socket_pair(ch.route_result0); socket_pair(ch.route_result1);
  const auto fds = all_fds(ch); const std::string executable = MOE_TOPK_M2_DEALER_CANDIDATE_EXECUTABLE;
  const auto dealer = spawn(executable, dealer_args(config, ch), fds, {ch.package0.first, ch.package1.first});
  close_fd(ch.package0.first); close_fd(ch.package1.first);
  const auto p0 = spawn(executable, party_args(config, ch, 0, mode, 0xB60000 + serial), fds,
                        [&] { std::vector<int> keep{ch.package0.second, ch.ready0.first, ch.input0.first,
                          ch.masked.first, ch.result0.first, ch.rank.first, ch.route_result0.first,
                          ch.score[0].first, ch.score[1].first}; for (const auto& p : ch.offline) keep.push_back(p.first);
                          for (const auto& p : ch.forward) keep.push_back(p.first); for (const auto& p : ch.reverse) keep.push_back(p.first); return keep; }());
  const auto p1 = spawn(executable, party_args(config, ch, 1, mode, 0xB70000 + serial), fds,
                        [&] { std::vector<int> keep{ch.package1.second, ch.ready1.first, ch.input1.first,
                          ch.masked.second, ch.result1.first, ch.rank.second, ch.route_result1.first,
                          ch.score[0].second, ch.score[1].second}; for (const auto& p : ch.offline) keep.push_back(p.second);
                          for (const auto& p : ch.forward) keep.push_back(p.second); for (const auto& p : ch.reverse) keep.push_back(p.second); return keep; }());
  wait_ok(dealer, "raw Route A dealer");
  close_fd(ch.package0.second); close_fd(ch.package1.second); close_fd(ch.ready0.first); close_fd(ch.ready1.first);
  close_fd(ch.input0.first); close_fd(ch.input1.first); close_fd(ch.result0.first); close_fd(ch.result1.first);
  close_fd(ch.rank.first); close_fd(ch.rank.second); close_fd(ch.route_result0.first); close_fd(ch.route_result1.first);
  for (auto& p : ch.offline) { close_fd(p.first); close_fd(p.second); }
  for (auto& p : ch.forward) { close_fd(p.first); close_fd(p.second); }
  for (auto& p : ch.reverse) { close_fd(p.first); close_fd(p.second); }
  for (auto& p : ch.score) { close_fd(p.first); close_fd(p.second); }
  close_fd(ch.masked.first); close_fd(ch.masked.second);
  const auto frame = [&](int party, int phase) { return ProtocolIFrameConfig{config.session, config.fingerprint,
      config.padded_n, config.k, config.comparison_bits, 2, static_cast<std::uint8_t>(party),
      static_cast<std::uint8_t>(phase), 1}; };
  { ProtocolIFramedChannel r0(take_fd(ch.ready0.second), frame(0, 2), kTimeoutMs);
    ProtocolIFramedChannel r1(take_fd(ch.ready1.second), frame(1, 2), kTimeoutMs);
    require(r0.receive() == std::vector<std::uint8_t>{1} && r1.receive() == std::vector<std::uint8_t>{1}, "raw Route A readiness"); }
  { ProtocolIFramedChannel i0(take_fd(ch.input0.second), frame(0, 4), kTimeoutMs);
    ProtocolIFramedChannel i1(take_fd(ch.input1.second), frame(1, 4), kTimeoutMs);
    i0.send(encode_words(shares.first)); i1.send(encode_words(shares.second)); }
  ProtocolIFramedChannel r0(take_fd(ch.route_result0.second), frame(0, 8), kTimeoutMs);
  ProtocolIFramedChannel r1(take_fd(ch.route_result1.second), frame(1, 8), kTimeoutMs);
  const auto mask0 = parse_result(r0.receive(), n, config.material_id);
  const auto mask1 = parse_result(r1.receive(), n, config.material_id);
  wait_ok(p0, "raw Route A party 0"); wait_ok(p1, "raw Route A party 1"); close_all(ch);
  std::vector<std::size_t> order(n); for (std::size_t i = 0; i < n; ++i) order[i] = i;
  std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
    return static_cast<std::int32_t>(scores[a]) > static_cast<std::int32_t>(scores[b]); });
  std::vector<std::uint8_t> expected(n); for (std::size_t i = 0; i < k; ++i) expected[order[i]] = 1;
  std::size_t selected = 0; for (std::size_t i = 0; i < n; ++i) {
    const auto bit = static_cast<std::uint8_t>((mask0[i] ^ mask1[i]) & 1U);
    require(bit == expected[i], "raw Route A original-order oracle"); selected += bit;
  }
  require(selected == k && r0.received_bytes() > 0 && r1.received_bytes() > 0, "raw Route A output accounting");
}

}  // namespace

int main() {
  try {
    for (int index = 0; index < 256; ++index)
      FSSConfig::prngs[index].SetSeed(osuCrypto::toBlock(0x524157524f555445ULL, static_cast<std::uint64_t>(index)));
    result_negative();
    std::uint64_t serial = 1;
    for (const auto n : {1U, 2U, 3U, 5U, 7U, 8U, 11U, 16U, 31U,
                         127U, 128U, 129U, 256U}) {
      std::vector<std::uint32_t> ks{1U, (n + 1U) / 2U, n};
      if (n >= 2U) ks.push_back(2U);
      if (n >= 8U) ks.push_back(8U);
      std::sort(ks.begin(), ks.end());
      ks.erase(std::unique(ks.begin(), ks.end()), ks.end());
      for (const auto k : ks)
        run_case(n, k, static_cast<unsigned>(serial % 5U), static_cast<unsigned>(serial % 3U), serial++);
    }
    return 0;
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
