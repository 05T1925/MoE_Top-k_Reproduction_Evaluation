#include <moe_topk/protocol_i_transport.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {

constexpr std::uint32_t kMagic = UINT32_C(0x4d325446);
constexpr std::size_t kHeaderBytes = 48;
constexpr std::uint32_t kMaxPayload = 1U << 20;
constexpr int kBits = 36;
using Bytes = std::vector<std::uint8_t>;
using namespace moe_topk;

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

class TestOwnedAuthenticatedStream final : public ProtocolIAuthenticatedByteStream {
 public:
  explicit TestOwnedAuthenticatedStream(int fd) : fd_(fd) {}
  ~TestOwnedAuthenticatedStream() override { if (fd_ >= 0) ::close(fd_); }
  std::size_t read_some(std::uint8_t*, std::size_t, int) override { return 0; }
  std::size_t write_some(const std::uint8_t*, std::size_t, int) override { return 0; }
 private:
  int fd_;
};

void expect_explicit_transport_modes() {
  int missing[2]{};
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, missing) == 0, "required stream missing socketpair");
  bool rejected = false;
  try {
    ProtocolIFramedChannel channel(missing[0], config(0, 1),
        ProtocolIFramedChannelOptions{200, 1024,
            ProtocolITransportMode::RequireAuthenticatedStream});
  } catch (const ProtocolITransportError&) { rejected = true; }
  require(rejected, "TLS-required channel rejects missing stream before any raw I/O");
  require(::fcntl(missing[0], F_GETFD) >= 0, "failed channel constructor leaves caller descriptor owned by caller");
  ::close(missing[0]); ::close(missing[1]);

  int wrong_mode[2]{};
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, wrong_mode) == 0, "registered stream socketpair");
  std::weak_ptr<ProtocolIAuthenticatedByteStream> retained;
  auto stream = std::make_shared<TestOwnedAuthenticatedStream>(wrong_mode[0]);
  retained = stream;
  protocol_i_attach_authenticated_stream(wrong_mode[0], std::move(stream));
  rejected = false;
  try { ProtocolIFramedChannel channel(wrong_mode[0], config(0, 1)); }
  catch (const ProtocolITransportError&) { rejected = true; }
  require(rejected && protocol_i_has_authenticated_stream(wrong_mode[0]),
          "explicit caller-FD mode rejects without consuming registered TLS stream");
  protocol_i_discard_unclaimed_authenticated_stream(wrong_mode[0]);
  require(retained.expired(), "unclaimed test stream is destroyed exactly once");
  ::close(wrong_mode[1]);

  int consumed[2]{};
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, consumed) == 0, "required stream consume socketpair");
  auto owned = std::make_shared<TestOwnedAuthenticatedStream>(consumed[0]);
  std::weak_ptr<ProtocolIAuthenticatedByteStream> consumed_weak = owned;
  protocol_i_attach_authenticated_stream(consumed[0], std::move(owned));
  {
    ProtocolIFramedChannel channel(consumed[0], config(0, 1),
        ProtocolIFramedChannelOptions{200, 1024,
            ProtocolITransportMode::RequireAuthenticatedStream});
    require(!protocol_i_has_authenticated_stream(consumed[0]),
            "required channel atomically consumes registered stream once");
  }
  require(consumed_weak.expired(), "consumed authenticated stream is released once by channel");
  ::close(consumed[1]);
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

void expect_bounded_chunk_message(std::size_t message_size) {
  constexpr std::size_t chunk_data = (1U << 20U) - 16U;
  int sockets[2]{};
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0, "bounded chunk socketpair");
  const auto child = ::fork();
  require(child >= 0, "bounded chunk fork");
  if (child == 0) {
    ::close(sockets[0]);
    try {
      ProtocolIFramedChannel peer(sockets[1], config(1, 0));
      const auto received = protocol_i_receive_framed_chunks(peer, message_size);
      require(received.size() == message_size, "bounded chunks reconstructed length");
      for (std::size_t i = 0; i < received.size(); ++i)
        require(received[i] == static_cast<std::uint8_t>((i * 131U + 7U) & 0xffU),
                "bounded chunks reconstructed bytes");
      const std::uint64_t frames = (message_size + chunk_data - 1U) / chunk_data;
      require(peer.received_bytes() == frames * (kHeaderBytes + 16U) + message_size,
              "bounded chunks receive byte conservation");
      peer.send(Bytes{0x5a});
      _exit(0);
    } catch (...) { _exit(3); }
  }
  ::close(sockets[1]);
  ProtocolIFramedChannel sender(sockets[0], config(0, 1));
  Bytes message(message_size);
  for (std::size_t i = 0; i < message.size(); ++i)
    message[i] = static_cast<std::uint8_t>((i * 131U + 7U) & 0xffU);
  protocol_i_send_framed_chunks(sender, message);
  const std::uint64_t frames = (message_size + chunk_data - 1U) / chunk_data;
  require(sender.sent_bytes() == frames * (kHeaderBytes + 16U) + message_size,
          "bounded chunks send byte conservation");
  require(sender.receive() == Bytes{0x5a}, "bounded chunk acknowledgement");
  wait_child(child);
  ::close(sockets[0]);
}

void expect_chunk_sequence_rejected(bool duplicate_offset, bool truncate_after_first = false) {
  int sockets[2]{};
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0, "malformed chunk socketpair");
  const auto child = ::fork();
  require(child >= 0, "malformed chunk fork");
  if (child == 0) {
    ::close(sockets[0]);
    ProtocolIFramedChannel sender(sockets[1], config(0, 1));
    auto make = [](std::uint64_t total, std::uint64_t offset, std::size_t n) {
      Bytes frame;
      for (int shift = 56; shift >= 0; shift -= 8) frame.push_back(static_cast<std::uint8_t>(total >> shift));
      for (int shift = 56; shift >= 0; shift -= 8) frame.push_back(static_cast<std::uint8_t>(offset >> shift));
      frame.insert(frame.end(), n, 0x31);
      return frame;
    };
    sender.send(make(32, 0, 16));
    if (!truncate_after_first) sender.send(make(32, duplicate_offset ? 0 : 17, 16));
    _exit(0);
  }
  ::close(sockets[1]);
  ProtocolIFramedChannel receiver(sockets[0], config(1, 0));
  bool rejected = false;
  try { (void)protocol_i_receive_framed_chunks(receiver, 64); }
  catch (const ProtocolITransportError&) { rejected = true; }
  require(rejected, truncate_after_first ? "truncated chunk sequence rejected" :
      duplicate_offset ? "duplicate chunk offset rejected" : "chunk gap rejected");
  wait_child(child);
  ::close(sockets[0]);
}

int main() {
  try {
    expect_explicit_transport_modes();
    for (const auto chunk : {std::size_t{1}, std::size_t{2}, std::size_t{3}, std::size_t{7}}) {
      expect_chunked_io(chunk);
    }
    constexpr std::size_t kSingleFrameData = (1U << 20U) - 16U;
    expect_bounded_chunk_message(kSingleFrameData - 1U);
    expect_bounded_chunk_message(kSingleFrameData);
    expect_bounded_chunk_message(kSingleFrameData + 1U);
    expect_chunk_sequence_rejected(true);
    expect_chunk_sequence_rejected(false);
    expect_chunk_sequence_rejected(false, true);

    expect_bad_header([](RawHeader& header) { header.magic ^= 1U; });
    expect_bad_header([](RawHeader& header) { ++header.version; });
    expect_bad_header([](RawHeader& header) { header.reserved0 = 1; });
    expect_bad_header([](RawHeader& header) { header.session = 100; });
    expect_bad_header([](RawHeader& header) { header.fingerprint = 100; });
    expect_bad_header([](RawHeader& header) { header.sender = 0; });
    expect_bad_header([](RawHeader& header) { header.sequence = 1; });
    expect_bad_header([](RawHeader& header) { ++header.phase; });
    expect_bad_header([](RawHeader& header) { header.length = kMaxPayload + 1; });
    expect_truncated_header();
    expect_truncated_payload();
    expect_payload_deadline_timeout();
    expect_eof();
    expect_timeout();
    expect_rejected_sequence_preserved();
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
