#include <moe_topk/protocol_i_permute_share.h>

#include <array>
#include <cerrno>
#include <cstdint>
#include <exception>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {
using namespace moe_topk;

struct Fds { std::array<int, 4> offline{}, online{}; int report = -1, barrier = -1; };
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
ProtocolIBlock192 add(ProtocolIBlock192 a, const ProtocolIBlock192& b) { return {a.word0 + b.word0, a.word1 + b.word1, a.word2 + b.word2}; }
std::vector<ProtocolIBlock192> add_vectors(std::vector<ProtocolIBlock192> a, const std::vector<ProtocolIBlock192>& b) { for (std::size_t i = 0; i < a.size(); ++i) a[i] = add(a[i], b[i]); return a; }
ProtocolIPermuteShareConfig config(std::uint32_t n, std::uint32_t t, std::uint64_t id, std::uint8_t owner) { return {0x211000U + n, 0x212000U + t, id, id + 1000U, n, t, owner, 15000}; }
void socket_pair(int fds[2]) { require(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0, "socketpair failed"); }
void close_fd(int fd) { if (fd >= 0) (void)::close(fd); }

void write_exact(int fd, const void* data, std::size_t size) {
  const auto* cursor = static_cast<const std::uint8_t*>(data);
  while (size != 0) { const ssize_t written = ::write(fd, cursor, size); if (written > 0) { cursor += written; size -= static_cast<std::size_t>(written); } else if (written < 0 && errno == EINTR) {} else throw std::runtime_error("report write failed"); }
}
void read_exact(int fd, void* data, std::size_t size) {
  auto* cursor = static_cast<std::uint8_t*>(data);
  while (size != 0) { const ssize_t count = ::read(fd, cursor, size); if (count > 0) { cursor += count; size -= static_cast<std::size_t>(count); } else if (count < 0 && errno == EINTR) {} else throw std::runtime_error("report read failed"); }
}

std::vector<ProtocolIBlock192> share_for_party(std::uint32_t n, std::uint64_t seed, unsigned type, unsigned party) {
  std::mt19937_64 random(seed); std::vector<ProtocolIBlock192> x0(n), x1(n);
  for (std::uint32_t i = 0; i < n; ++i) switch (type) {
    case 0: x0[i] = {random(), random(), random()}; x1[i] = {random(), random(), random()}; break;
    case 1: x0[i] = {}; x1[i] = {}; break;
    case 2: x0[i] = {random(), 0, 0}; x1[i] = {random(), 0, 0}; break;
    case 3: { const std::uint64_t bit = (random() >> (i % 17U)) & 1U; x0[i] = {random(), random(), random()}; x1[i] = {bit - x0[i].word0, 0U - x0[i].word1, 0U - x0[i].word2}; break; }
    default: throw std::runtime_error("unknown record type");
  }
  return party == 0 ? x0 : x1;
}

Fds parse_fds(int argc, char** argv) {
  require(argc == 16, "invalid party arguments"); Fds fds;
  for (unsigned i = 0; i < 4; ++i) fds.offline[i] = std::stoi(argv[2 + i]);
  for (unsigned i = 0; i < 4; ++i) fds.online[i] = std::stoi(argv[6 + i]);
  fds.report = std::stoi(argv[10]); fds.barrier = std::stoi(argv[11]); return fds;
}

int run_party(int argc, char** argv) {
  const unsigned party = std::stoul(argv[1]); const Fds fds = parse_fds(argc, argv);
  const std::uint32_t n = static_cast<std::uint32_t>(std::stoul(argv[12]));
  const std::uint32_t t = static_cast<std::uint32_t>(std::stoul(argv[13]));
  const std::uint64_t seed = std::stoull(argv[14]); const unsigned type = std::stoul(argv[15]);
  require(party < 2, "invalid party id");
  std::uint8_t barrier_byte = 1; write_exact(fds.barrier, &barrier_byte, sizeof(barrier_byte));
  read_exact(fds.barrier, &barrier_byte, sizeof(barrier_byte));
  const auto input = share_for_party(n, seed, type, party);
  const auto c1 = config(n, t, seed + 10U, 0), c2 = config(n, t, seed + 20U, 1), c3 = config(n, t, seed + 30U, 1), c4 = config(n, t, seed + 40U, 0);
  if (party == 0) {
    const auto p0 = protocol_i_test_permutation(n, seed + 1U);
    const auto inverse_p0 = protocol_i_inverse_permutation(p0);
    auto po_f1 = protocol_i_permute_share_preprocess_po(c1, fds.offline[0], p0);
    auto do_f2 = protocol_i_permute_share_preprocess_do(c2, fds.offline[1]);
    auto do_r1 = protocol_i_permute_share_preprocess_do(c3, fds.offline[2]);
    auto po_r2 = protocol_i_permute_share_preprocess_po(c4, fds.offline[3], inverse_p0);
    const auto a0 = protocol_i_permute_share_online_po(c1, fds.online[0], p0, std::move(po_f1));
    const auto b0 = add_vectors(protocol_i_apply_permutation(p0, input), a0.share);
    const auto c0 = protocol_i_permute_share_online_do(c2, fds.online[1], b0, std::move(do_f2));
    const auto e0 = protocol_i_permute_share_online_do(c3, fds.online[2], c0.share, std::move(do_r1));
    const auto g0 = protocol_i_permute_share_online_po(c4, fds.online[3], inverse_p0, std::move(po_r2));
    const auto h0 = add_vectors(protocol_i_apply_permutation(inverse_p0, e0.share), g0.share);
    write_exact(fds.report, h0.data(), h0.size() * sizeof(h0.front()));
  } else {
    const auto p1 = protocol_i_test_permutation(n, seed + 2U);
    const auto inverse_p1 = protocol_i_inverse_permutation(p1);
    auto do_f1 = protocol_i_permute_share_preprocess_do(c1, fds.offline[0]);
    auto po_f2 = protocol_i_permute_share_preprocess_po(c2, fds.offline[1], p1);
    auto po_r1 = protocol_i_permute_share_preprocess_po(c3, fds.offline[2], inverse_p1);
    auto do_r2 = protocol_i_permute_share_preprocess_do(c4, fds.offline[3]);
    const auto a1 = protocol_i_permute_share_online_do(c1, fds.online[0], input, std::move(do_f1));
    const auto e1 = protocol_i_permute_share_online_po(c2, fds.online[1], p1, std::move(po_f2));
    const auto z1 = add_vectors(protocol_i_apply_permutation(p1, a1.share), e1.share);
    const auto g0 = protocol_i_permute_share_online_po(c3, fds.online[2], inverse_p1, std::move(po_r1));
    const auto f1 = add_vectors(protocol_i_apply_permutation(inverse_p1, z1), g0.share);
    const auto h1 = protocol_i_permute_share_online_do(c4, fds.online[3], f1, std::move(do_r2));
    write_exact(fds.report, h1.share.data(), h1.share.size() * sizeof(h1.share.front()));
  }
  return 0;
}

pid_t launch_party(const char* executable, unsigned party, const Fds& fds, std::uint32_t n, std::uint32_t t, std::uint64_t seed, unsigned type) {
  const pid_t pid = ::fork(); require(pid >= 0, "fork failed"); if (pid != 0) return pid;
  std::array<std::string, 15> values{}; values[0] = std::to_string(party);
  for (unsigned i = 0; i < 4; ++i) values[1 + i] = std::to_string(fds.offline[i]);
  for (unsigned i = 0; i < 4; ++i) values[5 + i] = std::to_string(fds.online[i]);
  values[9] = std::to_string(fds.report); values[10] = std::to_string(fds.barrier); values[11] = std::to_string(n); values[12] = std::to_string(t); values[13] = std::to_string(seed); values[14] = std::to_string(type);
  std::array<char*, 17> args{}; args[0] = const_cast<char*>(executable); args[1] = values[0].data();
  for (unsigned i = 0; i < 14; ++i) args[2 + i] = values[1 + i].data();
  ::execv(executable, args.data()); _exit(127);
}

void run_case(const char* executable, std::uint32_t n, std::uint32_t t, std::uint64_t seed, unsigned type) {
  std::array<std::array<int, 2>, 4> offline{}, online{}; int report0[2]{}, report1[2]{}, barrier0[2]{}, barrier1[2]{};
  for (auto& channel : offline) socket_pair(channel.data()); for (auto& channel : online) socket_pair(channel.data()); socket_pair(report0); socket_pair(report1); socket_pair(barrier0); socket_pair(barrier1);
  Fds p0{}, p1{}; for (unsigned i = 0; i < 4; ++i) { p0.offline[i] = offline[i][0]; p1.offline[i] = offline[i][1]; p0.online[i] = online[i][0]; p1.online[i] = online[i][1]; }
  p0.report = report0[0]; p1.report = report1[0]; p0.barrier = barrier0[0]; p1.barrier = barrier1[0];
  const pid_t child0 = launch_party(executable, 0, p0, n, t, seed, type), child1 = launch_party(executable, 1, p1, n, t, seed, type);
  for (const auto& channel : offline) { close_fd(channel[0]); close_fd(channel[1]); } for (const auto& channel : online) { close_fd(channel[0]); close_fd(channel[1]); } close_fd(report0[0]); close_fd(report1[0]); close_fd(barrier0[0]); close_fd(barrier1[0]);
  std::uint8_t barrier_byte = 0; read_exact(barrier0[1], &barrier_byte, sizeof(barrier_byte)); read_exact(barrier1[1], &barrier_byte, sizeof(barrier_byte)); barrier_byte = 1; write_exact(barrier0[1], &barrier_byte, sizeof(barrier_byte)); write_exact(barrier1[1], &barrier_byte, sizeof(barrier_byte)); close_fd(barrier0[1]); close_fd(barrier1[1]);
  std::vector<ProtocolIBlock192> h0(n), h1(n); read_exact(report0[1], h0.data(), h0.size() * sizeof(h0.front())); read_exact(report1[1], h1.data(), h1.size() * sizeof(h1.front())); close_fd(report0[1]); close_fd(report1[1]);
  int status0 = 0, status1 = 0; require(::waitpid(child0, &status0, 0) == child0, "waitpid party 0 failed"); require(::waitpid(child1, &status1, 0) == child1, "waitpid party 1 failed"); require(WIFEXITED(status0) && WEXITSTATUS(status0) == 0, "party 0 failed"); require(WIFEXITED(status1) && WEXITSTATUS(status1) == 0, "party 1 failed");
  const auto x0 = share_for_party(n, seed, type, 0), x1 = share_for_party(n, seed, type, 1);
  for (std::uint32_t i = 0; i < n; ++i) { const auto result = add(h0[i], h1[i]); require(result == add(x0[i], x1[i]), "shuffle roundtrip mismatch"); if (type == 3) { require(((h0[i].word0 & 1U) ^ (h1[i].word0 & 1U)) == (result.word0 & 1U), "carrier XOR reconstruction mismatch"); require(result.word1 == 0 && result.word2 == 0, "carrier non-bit lanes changed"); } }
}
}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc > 1) return run_party(argc, argv);
    std::uint64_t seed = 1;
    for (const auto [n, t] : std::array<std::pair<std::uint32_t, std::uint32_t>, 9>{{{2, 2}, {4, 2}, {4, 4}, {8, 2}, {8, 8}, {16, 4}, {16, 16}, {64, 4}, {256, 16}}})
      for (unsigned type = 0; type < 4; ++type) run_case(argv[0], n, t, seed += 100U, type);
    return 0;
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
