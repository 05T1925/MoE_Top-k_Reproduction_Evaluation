#include <moe_topk/protocol_i_secret_shared_shuffle.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {
using namespace moe_topk;

struct Fds {
  std::array<int, 4> offline{}, online{};
  int report = -1, barrier = -1;
};

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

ProtocolIBlock192 clear_add(ProtocolIBlock192 left,
                            const ProtocolIBlock192& right) {
  return {left.word0 + right.word0, left.word1 + right.word1,
          left.word2 + right.word2};
}

template <typename T>
std::vector<T> clear_apply(const ProtocolIPermutation& permutation,
                           const std::vector<T>& input) {
  require(permutation.size() == input.size(), "clear permutation shape");
  std::vector<T> output(input.size());
  for (std::size_t index = 0; index < input.size(); ++index)
    output[index] = input[permutation[index]];
  return output;
}

ProtocolIPermutation test_permutation(std::uint32_t n, unsigned style,
                                      std::uint64_t seed) {
  ProtocolIPermutation result(n);
  for (std::uint32_t index = 0; index < n; ++index) result[index] = index;
  if (style == 1) {
    std::reverse(result.begin(), result.end());
  } else if (style == 2) {
    for (std::uint32_t index = 0; index < n; ++index)
      result[index] = (index + 1) % n;
  } else if (style == 3) {
    for (std::uint32_t index = 0; index < n; index += 2)
      std::swap(result[index], result[index + 1]);
  } else if (style == 4) {
    std::mt19937_64 random(seed);
    std::shuffle(result.begin(), result.end(), random);
  }
  return result;
}

std::vector<ProtocolIBlock192> clear_records(std::uint32_t n, std::uint64_t seed,
                                             unsigned type) {
  std::mt19937_64 random(seed ^ UINT64_C(0xc1f04a7d));
  std::vector<ProtocolIBlock192> result(n);
  for (std::uint32_t index = 0; index < n; ++index) {
    if (type == 0) {
      result[index] = {random(), random(), random()};
    } else if (type == 1) {
      switch (index % 4U) {
        case 0:
          result[index] = {0, std::numeric_limits<std::uint64_t>::max(), 1};
          break;
        case 1:
          result[index] = {std::numeric_limits<std::uint64_t>::max(), 0,
                           std::numeric_limits<std::uint64_t>::max()};
          break;
        case 2:
          result[index] = {1, 1, 0};
          break;
        default:
          result[index] = {std::numeric_limits<std::uint64_t>::max() - 1, 42, 42};
          break;
      }
    } else if (type == 2) {
      // Distinct key lane, duplicate payload lane, independent random lane.
      result[index] = {UINT64_C(0x100000000) + index, index % 3U, random()};
    } else if (type == 3) {
      result[index] = {std::numeric_limits<std::uint64_t>::max() - index, 7,
                       index & 1U ? std::numeric_limits<std::uint64_t>::max() : 0};
    } else {
      throw std::runtime_error("unknown record type");
    }
  }
  return result;
}

std::vector<ProtocolIBlock192> share_for_party(std::uint32_t n,
                                               std::uint64_t seed,
                                               unsigned type,
                                               unsigned party) {
  const auto clear = clear_records(n, seed, type);
  std::mt19937_64 random(seed ^ UINT64_C(0x51a4e5));
  std::vector<ProtocolIBlock192> share0(n), share1(n);
  for (std::uint32_t index = 0; index < n; ++index) {
    share0[index] = {random(), random(), random()};
    share1[index] = protocol_i_record_sub(clear[index], share0[index]);
  }
  return party == 0 ? share0 : share1;
}

void socket_pair(int fds[2]) {
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0, "socketpair failed");
}
void close_fd(int fd) {
  if (fd >= 0) (void)::close(fd);
}

void write_exact(int fd, const void* data, std::size_t size) {
  const auto* cursor = static_cast<const std::uint8_t*>(data);
  while (size != 0) {
    const ssize_t written = ::write(fd, cursor, size);
    if (written > 0) {
      cursor += written;
      size -= static_cast<std::size_t>(written);
    } else if (written < 0 && errno == EINTR) {
    } else {
      throw std::runtime_error("report write failed");
    }
  }
}

void read_exact(int fd, void* data, std::size_t size) {
  auto* cursor = static_cast<std::uint8_t*>(data);
  while (size != 0) {
    const ssize_t count = ::read(fd, cursor, size);
    if (count > 0) {
      cursor += count;
      size -= static_cast<std::size_t>(count);
    } else if (count < 0 && errno == EINTR) {
    } else {
      throw std::runtime_error("report read failed");
    }
  }
}

Fds parse_fds(int argc, char** argv) {
  require(argc == 17, "invalid party arguments");
  Fds fds;
  for (unsigned index = 0; index < 4; ++index)
    fds.offline[index] = std::stoi(argv[2 + index]);
  for (unsigned index = 0; index < 4; ++index)
    fds.online[index] = std::stoi(argv[6 + index]);
  fds.report = std::stoi(argv[10]);
  fds.barrier = std::stoi(argv[11]);
  return fds;
}

int run_party(int argc, char** argv) {
  const unsigned party = std::stoul(argv[1]);
  const Fds fds = parse_fds(argc, argv);
  const std::uint32_t n = static_cast<std::uint32_t>(std::stoul(argv[12]));
  const std::uint32_t t = static_cast<std::uint32_t>(std::stoul(argv[13]));
  const std::uint64_t seed = std::stoull(argv[14]);
  const unsigned type = std::stoul(argv[15]);
  const unsigned own_style = std::stoul(argv[16]);
  require(party < 2, "invalid party id");

  // Each process receives only its own permutation recipe.  No party API is
  // given the peer permutation or the composed permutation.
  const auto own_permutation = test_permutation(n, own_style, seed + 1U + party);
  ProtocolIShufflePartyConfig shuffle_config{
      UINT64_C(0x211000) + n, UINT64_C(0x212000) + t, seed + 10U,
      seed + 1010U, n, t, static_cast<std::uint8_t>(party), 15000};
  auto material = protocol_i_shuffle_preprocess_party(
      shuffle_config, fds.offline, own_permutation);
  require(material.own_permutation == own_permutation,
          "party material lost own permutation");

  std::uint8_t barrier_byte = 1;
  write_exact(fds.barrier, &barrier_byte, sizeof(barrier_byte));
  read_exact(fds.barrier, &barrier_byte, sizeof(barrier_byte));

  const auto input = share_for_party(n, seed, type, party);
  const auto forward = protocol_i_shuffle_forward_party(
      static_cast<int>(party), {fds.online[0], fds.online[1]}, input, material);
  require(material.forward_consumed && !material.reverse_consumed,
          "forward/reverse consumption boundary");
  write_exact(fds.report, forward.share.data(),
              forward.share.size() * sizeof(forward.share.front()));

  // Reverse carrier routing is a separate C-INSTANTIATION extension and is
  // checked independently after the forward result has already been reported.
  const auto reverse = protocol_i_shuffle_reverse_party(
      static_cast<int>(party), {fds.online[2], fds.online[3]}, forward.share,
      material);
  require(material.reverse_consumed, "reverse material not consumed");
  write_exact(fds.report, reverse.share.data(),
              reverse.share.size() * sizeof(reverse.share.front()));
  return 0;
}

pid_t launch_party(const char* executable, unsigned party, const Fds& fds,
                   std::uint32_t n, std::uint32_t t, std::uint64_t seed,
                   unsigned type, unsigned own_style) {
  const pid_t pid = ::fork();
  require(pid >= 0, "fork failed");
  if (pid != 0) return pid;

  std::array<std::string, 16> values{};
  values[0] = std::to_string(party);
  for (unsigned index = 0; index < 4; ++index)
    values[1 + index] = std::to_string(fds.offline[index]);
  for (unsigned index = 0; index < 4; ++index)
    values[5 + index] = std::to_string(fds.online[index]);
  values[9] = std::to_string(fds.report);
  values[10] = std::to_string(fds.barrier);
  values[11] = std::to_string(n);
  values[12] = std::to_string(t);
  values[13] = std::to_string(seed);
  values[14] = std::to_string(type);
  values[15] = std::to_string(own_style);

  std::array<char*, 18> arguments{};
  arguments[0] = const_cast<char*>(executable);
  for (unsigned index = 0; index < values.size(); ++index)
    arguments[1 + index] = values[index].data();
  ::execv(executable, arguments.data());
  _exit(127);
}

void run_case(const char* executable, std::uint32_t n, std::uint32_t t,
              std::uint64_t seed, unsigned type, unsigned p0_style,
              unsigned p1_style) {
  std::array<std::array<int, 2>, 4> offline{}, online{};
  int report0[2]{}, report1[2]{}, barrier0[2]{}, barrier1[2]{};
  for (auto& channel : offline) socket_pair(channel.data());
  for (auto& channel : online) socket_pair(channel.data());
  socket_pair(report0);
  socket_pair(report1);
  socket_pair(barrier0);
  socket_pair(barrier1);

  Fds p0{}, p1{};
  for (unsigned index = 0; index < 4; ++index) {
    p0.offline[index] = offline[index][0];
    p1.offline[index] = offline[index][1];
    p0.online[index] = online[index][0];
    p1.online[index] = online[index][1];
  }
  p0.report = report0[0];
  p1.report = report1[0];
  p0.barrier = barrier0[0];
  p1.barrier = barrier1[0];

  const pid_t child0 = launch_party(executable, 0, p0, n, t, seed, type, p0_style);
  const pid_t child1 = launch_party(executable, 1, p1, n, t, seed, type, p1_style);
  for (const auto& channel : offline) {
    close_fd(channel[0]);
    close_fd(channel[1]);
  }
  for (const auto& channel : online) {
    close_fd(channel[0]);
    close_fd(channel[1]);
  }
  close_fd(report0[0]);
  close_fd(report1[0]);
  close_fd(barrier0[0]);
  close_fd(barrier1[0]);

  std::uint8_t barrier_byte = 0;
  read_exact(barrier0[1], &barrier_byte, sizeof(barrier_byte));
  read_exact(barrier1[1], &barrier_byte, sizeof(barrier_byte));
  barrier_byte = 1;
  write_exact(barrier0[1], &barrier_byte, sizeof(barrier_byte));
  write_exact(barrier1[1], &barrier_byte, sizeof(barrier_byte));
  close_fd(barrier0[1]);
  close_fd(barrier1[1]);

  std::vector<ProtocolIBlock192> forward0(n), forward1(n), reverse0(n), reverse1(n);
  read_exact(report0[1], forward0.data(), forward0.size() * sizeof(forward0.front()));
  read_exact(report1[1], forward1.data(), forward1.size() * sizeof(forward1.front()));
  read_exact(report0[1], reverse0.data(), reverse0.size() * sizeof(reverse0.front()));
  read_exact(report1[1], reverse1.data(), reverse1.size() * sizeof(reverse1.front()));
  close_fd(report0[1]);
  close_fd(report1[1]);

  int status0 = 0, status1 = 0;
  require(::waitpid(child0, &status0, 0) == child0, "waitpid party 0 failed");
  require(::waitpid(child1, &status1, 0) == child1, "waitpid party 1 failed");
  require(WIFEXITED(status0) && WEXITSTATUS(status0) == 0, "party 0 failed");
  require(WIFEXITED(status1) && WEXITSTATUS(status1) == 0, "party 1 failed");

  const auto input0 = share_for_party(n, seed, type, 0);
  const auto input1 = share_for_party(n, seed, type, 1);
  std::vector<ProtocolIBlock192> clear_input(n), clear_forward(n), clear_reverse(n);
  for (std::size_t index = 0; index < n; ++index) {
    clear_input[index] = clear_add(input0[index], input1[index]);
    clear_forward[index] = clear_add(forward0[index], forward1[index]);
    clear_reverse[index] = clear_add(reverse0[index], reverse1[index]);
  }

  const auto pi0 = test_permutation(n, p0_style, seed + 1U);
  const auto pi1 = test_permutation(n, p1_style, seed + 2U);
  const auto expected_forward = clear_apply(pi1, clear_apply(pi0, clear_input));
  require(clear_forward == expected_forward,
          "standalone forward SecretSharedShuffle oracle");

  // Freeze the production composition helper against an independent oracle.
  const auto composed = protocol_i_compose_permutation(pi1, pi0);
  require(clear_apply(composed, clear_input) == expected_forward,
          "forward composition direction");

  // This separate assertion covers only the project reverse extension.
  require(clear_reverse == clear_input, "reverse carrier extension oracle");
}
}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc > 1) return run_party(argc, argv);
    std::uint64_t seed = 1;
    for (unsigned type = 0; type < 4; ++type)
      run_case(argv[0], 2, 2, seed += 100U, type, 0, 0);
    for (unsigned type = 0; type < 4; ++type)
      run_case(argv[0], 4, 2, seed += 100U, type, 1, 2);
    const std::array<std::pair<std::uint32_t, std::uint32_t>, 7> random_shapes{{
        {4, 4}, {8, 2}, {8, 8}, {16, 4}, {16, 16}, {64, 4}, {256, 16}}};
    for (const auto [n, t] : random_shapes)
      for (unsigned type = 0; type < 4; ++type)
        run_case(argv[0], n, t, seed += 100U, type, 4, 4);
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
