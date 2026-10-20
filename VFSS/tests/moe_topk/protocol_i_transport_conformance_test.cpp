#include <moe_topk/protocol_i_transport.h>

#include <atomic>
#include <chrono>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <poll.h>
#include <pthread.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

constexpr std::uint32_t kMagic = UINT32_C(0x4d325446);
constexpr std::size_t kHeaderBytes = 48;
constexpr std::uint32_t kMaxPayload = 1U << 20;
constexpr int kBits = 36;
using Bytes = std::vector<std::uint8_t>;
using namespace moe_topk;

volatile sig_atomic_t g_sigpipe_handler_calls = 0;

void record_sigpipe(int) { ++g_sigpipe_handler_calls; }

struct RawHeader {
  std::uint32_t n = 5, k = 2, length = 0;
  std::uint64_t session = 7, fingerprint = 9, sequence = 0;
  std::uint32_t magic = kMagic;
  std::uint8_t version = 1, bits = kBits, sender = 1, receiver = 0, phase = 1, type = 1;
  std::uint8_t reserved0 = 0, reserved1 = 0;
};

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void put_u32(Bytes& bytes, std::size_t offset, std::uint32_t value) {
  for (int shift = 24; shift >= 0; shift -= 8) {
    bytes[offset++] = static_cast<std::uint8_t>(value >> shift);
  }
}

void put_u64(Bytes& bytes, std::size_t offset, std::uint64_t value) {
  for (int shift = 56; shift >= 0; shift -= 8) {
    bytes[offset++] = static_cast<std::uint8_t>(value >> shift);
  }
}

Bytes encode(RawHeader header) {
  Bytes bytes(kHeaderBytes, 0);
  put_u32(bytes, 0, header.magic);
  bytes[4] = header.version;
  bytes[5] = header.bits;
  bytes[6] = header.sender;
  bytes[7] = header.receiver;
  bytes[8] = header.phase;
  bytes[9] = header.type;
  bytes[10] = header.reserved0;
  bytes[11] = header.reserved1;
  put_u32(bytes, 12, header.n);
  put_u32(bytes, 16, header.k);
  put_u32(bytes, 20, header.length);
  put_u64(bytes, 24, header.session);
  put_u64(bytes, 32, header.fingerprint);
  put_u64(bytes, 40, header.sequence);
  return bytes;
}

void write_all(int fd, const Bytes& bytes) {
  std::size_t offset = 0;
  while (offset != bytes.size()) {
    const auto count = ::write(fd, bytes.data() + offset, bytes.size() - offset);
    if (count < 0 && errno == EINTR) {
      continue;
    }
    require(count > 0, "raw frame write");
    offset += static_cast<std::size_t>(count);
  }
}

ProtocolIFrameConfig config(int sender, int receiver) {
  return {7, 9, 5, 2, static_cast<std::uint8_t>(kBits), static_cast<std::uint8_t>(sender),
          static_cast<std::uint8_t>(receiver), 1, 1};
}

void wait_child(pid_t child) {
  int status = 0;
  require(::waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "transport child");
}

void expect_bad_header(void (*mutate)(RawHeader&)) {
  int sockets[2]{};
  int sync_pipe[2]{};
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0 && ::pipe(sync_pipe) == 0,
          "transport test setup");
  const auto child = ::fork();
  require(child >= 0, "transport fork");
  if (child == 0) {
    ::close(sockets[0]);
    ::close(sync_pipe[1]);
    RawHeader header;
    mutate(header);
    write_all(sockets[1], encode(header));
    std::uint8_t acknowledgement = 0;
    ::read(sync_pipe[0], &acknowledgement, 1);
    _exit(0);
  }
  ::close(sockets[1]);
  ::close(sync_pipe[0]);
  ProtocolIFramedChannel channel(sockets[0], config(0, 1), 200);
  bool threw = false;
  try {
    channel.receive();
  } catch (...) {
    threw = true;
  }
  require(threw, "rejected frame header");
  const std::uint8_t acknowledgement = 1;
  require(::write(sync_pipe[1], &acknowledgement, 1) == 1, "transport test acknowledgement");
  ::close(sync_pipe[1]);
  wait_child(child);
  ::close(sockets[0]);
}

void expect_truncated_header() {
  int sockets[2]{};
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0, "truncated setup");
  const auto child = ::fork();
  require(child >= 0, "truncated fork");
  if (child == 0) {
    ::close(sockets[0]);
    const auto header = encode(RawHeader{});
    write_all(sockets[1], Bytes(header.begin(), header.begin() + 7));
    ::close(sockets[1]);
    _exit(0);
  }
  ::close(sockets[1]);
  ProtocolIFramedChannel channel(sockets[0], config(0, 1), 200);
  bool threw = false;
  try {
    channel.receive();
  } catch (...) {
    threw = true;
  }
  require(threw, "truncated frame header");
  wait_child(child);
  ::close(sockets[0]);
}

void expect_truncated_payload() {
  int sockets[2]{};
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0, "truncated payload setup");
  const auto child = ::fork();
  require(child >= 0, "truncated payload fork");
  if (child == 0) {
    ::close(sockets[0]);
    RawHeader header;
    header.length = 2;
    write_all(sockets[1], encode(header));
    write_all(sockets[1], Bytes{1});
    ::close(sockets[1]);
    _exit(0);
  }
  ::close(sockets[1]);
  ProtocolIFramedChannel channel(sockets[0], config(0, 1), {200, 1});
  bool threw = false;
  try {
    channel.receive();
  } catch (...) {
    threw = true;
  }
  require(threw, "truncated payload rejection");
  wait_child(child);
  ::close(sockets[0]);
}

void expect_payload_deadline_timeout() {
  int sockets[2]{};
  int sync_pipe[2]{};
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0 && ::pipe(sync_pipe) == 0,
          "payload timeout setup");
  const auto child = ::fork();
  require(child >= 0, "payload timeout fork");
  if (child == 0) {
    ::close(sockets[0]);
    ::close(sync_pipe[1]);
    RawHeader header;
    header.length = 1;
    write_all(sockets[1], encode(header));
    std::uint8_t acknowledgement = 0;
    ::read(sync_pipe[0], &acknowledgement, 1);
    _exit(0);
  }
  ::close(sockets[1]);
  ::close(sync_pipe[0]);
  ProtocolIFramedChannel channel(sockets[0], config(0, 1), {20, 3});
  bool threw = false;
  try {
    channel.receive();
  } catch (...) {
    threw = true;
  }
  require(threw && channel.received_bytes() == kHeaderBytes, "payload absolute deadline");
  const std::uint8_t acknowledgement = 1;
  require(::write(sync_pipe[1], &acknowledgement, 1) == 1, "payload timeout acknowledgement");
  ::close(sync_pipe[1]);
  wait_child(child);
  ::close(sockets[0]);
}

void expect_eof() {
  int sockets[2]{};
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0, "EOF setup");
  ::close(sockets[1]);
  ProtocolIFramedChannel channel(sockets[0], config(0, 1), 200);
  bool threw = false;
  try {
    channel.receive();
  } catch (...) {
    threw = true;
  }
  require(threw, "EOF frame");
  ::close(sockets[0]);
}

void expect_timeout() {
  int sockets[2]{};
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0, "timeout setup");
  ProtocolIFramedChannel channel(sockets[0], config(0, 1), 20);
  bool threw = false;
  try {
    channel.receive();
  } catch (...) {
    threw = true;
  }
  require(threw, "frame timeout");
  ::close(sockets[0]);
  ::close(sockets[1]);
}

void expect_external_sigpipe_not_consumed_by_successful_write() {
  int sockets[2]{};
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0,
          "SIGPIPE preservation setup");
  int send_buffer = 4096;
  require(::setsockopt(sockets[0], SOL_SOCKET, SO_SNDBUF, &send_buffer,
                       sizeof(send_buffer)) == 0,
          "SIGPIPE preservation send buffer");

  struct sigaction action{};
  struct sigaction previous{};
  action.sa_handler = record_sigpipe;
  ::sigemptyset(&action.sa_mask);
  require(::sigaction(SIGPIPE, &action, &previous) == 0,
          "SIGPIPE preservation handler install");
  g_sigpipe_handler_calls = 0;

  Bytes payload(kMaxPayload, 0x5a);
  Bytes received(kHeaderBytes + payload.size());
  std::atomic<pid_t> writer_tid{0};
  std::atomic<bool> send_succeeded{false};
  pthread_t writer_id{};
  std::exception_ptr writer_error;
  ProtocolIFramedChannel channel(sockets[0], config(0, 1), {5000, kMaxPayload});

  std::thread reader([&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::size_t offset = 0;
    while (offset < received.size()) {
      const auto count = ::read(sockets[1], received.data() + offset,
                                received.size() - offset);
      if (count < 0 && errno == EINTR) continue;
      require(count > 0, "SIGPIPE preservation receiver EOF");
      offset += static_cast<std::size_t>(count);
    }
  });
  std::thread writer([&] {
    writer_id = ::pthread_self();
    writer_tid.store(static_cast<pid_t>(::syscall(SYS_gettid)),
                     std::memory_order_release);
    try {
      channel.send(payload);
      send_succeeded.store(true, std::memory_order_release);
    } catch (...) {
      writer_error = std::current_exception();
    }
  });
  bool sigpipe_blocked_in_writer = false;
  const auto block_deadline = std::chrono::steady_clock::now() +
      std::chrono::seconds(2);
  while (writer_tid.load(std::memory_order_acquire) == 0 &&
         std::chrono::steady_clock::now() < block_deadline)
    std::this_thread::yield();
  while (writer_tid.load(std::memory_order_acquire) != 0 &&
         std::chrono::steady_clock::now() < block_deadline) {
    const auto status_path = "/proc/self/task/" +
        std::to_string(writer_tid.load(std::memory_order_acquire)) + "/status";
    std::ifstream status(status_path);
    std::string line;
    while (std::getline(status, line)) {
      if (line.compare(0, 7, "SigBlk:") == 0) {
        const auto mask = std::stoull(line.substr(7), nullptr, 16);
        sigpipe_blocked_in_writer = (mask & (1ULL << (SIGPIPE - 1))) != 0;
        break;
      }
    }
    if (sigpipe_blocked_in_writer) break;
    std::this_thread::yield();
  }
  const int signal_status = sigpipe_blocked_in_writer
      ? ::pthread_kill(writer_id, SIGPIPE) : EAGAIN;
  writer.join();
  reader.join();
  ::close(sockets[1]);
  const int restore_status = ::sigaction(SIGPIPE, &previous, nullptr);

  require(sigpipe_blocked_in_writer && signal_status == 0 && restore_status == 0,
          "SIGPIPE preservation signal setup/restore");
  if (writer_error) std::rethrow_exception(writer_error);
  require(send_succeeded.load(std::memory_order_acquire) &&
              g_sigpipe_handler_calls == 1,
          "unrelated SIGPIPE was consumed by successful framed I/O");
}

void expect_preexisting_sigpipe_remains_pending() {
  sigset_t set{};
  sigset_t old_mask{};
  require(::sigemptyset(&set) == 0 && ::sigaddset(&set, SIGPIPE) == 0 &&
              ::pthread_sigmask(SIG_BLOCK, &set, &old_mask) == 0,
          "preexisting SIGPIPE block");
  require(::raise(SIGPIPE) == 0, "preexisting SIGPIPE raise");

  int sockets[2]{};
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0,
          "preexisting SIGPIPE socketpair");
  {
    ProtocolIFramedChannel channel(sockets[0], config(0, 1), 200);
    channel.send(Bytes{9});
    sigset_t pending{};
    require(::sigpending(&pending) == 0 &&
                ::sigismember(&pending, SIGPIPE) == 1,
            "transport consumed a preexisting SIGPIPE");
  }
  ::close(sockets[1]);
  const timespec no_wait{0, 0};
  require(::sigtimedwait(&set, nullptr, &no_wait) == SIGPIPE,
          "preexisting SIGPIPE cleanup");
  require(::pthread_sigmask(SIG_SETMASK, &old_mask, nullptr) == 0,
          "preexisting SIGPIPE restore");
}

void expect_rejected_sequence_preserved() {
  int sockets[2]{};
  int sync_pipe[2]{};
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0 && ::pipe(sync_pipe) == 0,
          "sequence setup");
  const auto child = ::fork();
  require(child >= 0, "sequence fork");
  if (child == 0) {
    ::close(sockets[0]);
    ::close(sync_pipe[1]);
    RawHeader rejected;
    rejected.session = 99;
    write_all(sockets[1], encode(rejected));
    RawHeader accepted;
    accepted.length = 1;
    write_all(sockets[1], encode(accepted));
    write_all(sockets[1], Bytes{42});
    std::uint8_t acknowledgement = 0;
    ::read(sync_pipe[0], &acknowledgement, 1);
    _exit(0);
  }
  ::close(sockets[1]);
  ::close(sync_pipe[0]);
  ProtocolIFramedChannel channel(sockets[0], config(0, 1), 200);
  bool threw = false;
  try {
    channel.receive();
  } catch (...) {
    threw = true;
  }
  require(threw && channel.received_bytes() == kHeaderBytes, "rejected sequence accounting");
  require(channel.receive() == Bytes{42}, "rejected header advanced sequence");
  const std::uint8_t acknowledgement = 1;
  require(::write(sync_pipe[1], &acknowledgement, 1) == 1, "sequence acknowledgement");
  ::close(sync_pipe[1]);
  wait_child(child);
  ::close(sockets[0]);
}

}  // namespace

void expect_chunked_io(std::size_t chunk) {
    int sockets[2]{};
    int sync_pipe[2]{};
    require(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0 && ::pipe(sync_pipe) == 0,
            "chunked transport setup");
    const auto child = ::fork();
    require(child >= 0, "chunked transport fork");
    if (child == 0) {
      ::close(sockets[0]);
      ::close(sync_pipe[1]);
      try {
        ProtocolIFramedChannel channel(sockets[1], config(1, 0), {2000, chunk});
        require(channel.receive() == Bytes{1, 2}, "transport receive payload");
        require(channel.received_bytes() == kHeaderBytes + 2, "transport receive counter");
        channel.reset_counters();
        channel.send(Bytes{3});
        require(channel.sent_bytes() == kHeaderBytes + 1, "transport send counter");
        std::uint8_t acknowledgement = 0;
        require(::read(sync_pipe[0], &acknowledgement, 1) == 1 && acknowledgement == 1,
                "chunked transport acknowledgement");
        _exit(0);
      } catch (...) {
        _exit(3);
      }
    }
    ::close(sockets[1]);
    ::close(sync_pipe[0]);
    ProtocolIFramedChannel channel(sockets[0], config(0, 1), {2000, chunk});
    channel.send(Bytes{1, 2});
    require(channel.sent_bytes() == kHeaderBytes + 2, "transport send counter");
    channel.reset_counters();
    require(channel.receive() == Bytes{3}, "transport reply payload");
    require(channel.received_bytes() == kHeaderBytes + 1, "transport reply counter");
    const std::uint8_t acknowledgement = 1;
    require(::write(sync_pipe[1], &acknowledgement, 1) == 1, "chunked parent acknowledgement");
    ::close(sync_pipe[1]);
    wait_child(child);
    ::close(sockets[0]);
}

int main() {
  try {
    for (const auto chunk : {std::size_t{1}, std::size_t{2}, std::size_t{3}, std::size_t{7}}) {
      expect_chunked_io(chunk);
    }

    expect_bad_header([](RawHeader& header) { header.magic ^= 1U; });
    expect_bad_header([](RawHeader& header) { ++header.version; });
    expect_bad_header([](RawHeader& header) { header.reserved0 = 1; });
    expect_bad_header([](RawHeader& header) { header.session = 100; });
    expect_bad_header([](RawHeader& header) { header.fingerprint = 100; });
    expect_bad_header([](RawHeader& header) { header.sender = 0; });
    expect_bad_header([](RawHeader& header) { header.sequence = 1; });
    expect_bad_header([](RawHeader& header) { header.length = kMaxPayload + 1; });
    expect_truncated_header();
    expect_truncated_payload();
    expect_payload_deadline_timeout();
    expect_eof();
    expect_timeout();
    expect_external_sigpipe_not_consumed_by_successful_write();
    expect_preexisting_sigpipe_remains_pending();
    expect_rejected_sequence_preserved();
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
