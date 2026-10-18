#include <moe_topk/protocol_i_transport.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <limits>
#include <memory>
#include <mutex>
#include <poll.h>
#include <stdexcept>
#include <unordered_map>
#include <unistd.h>

namespace moe_topk {
namespace {

constexpr std::uint32_t kMagic = UINT32_C(0x4d325446);
constexpr std::uint8_t kVersion = 1;
constexpr std::uint32_t kMaxPayload = 1U << 20;
constexpr std::size_t kChunkEnvelopeBytes = 16;
constexpr std::size_t kWireHeaderBytes = 48;

std::mutex& authenticated_stream_mutex() {
  static std::mutex mutex;
  return mutex;
}

std::unordered_map<int, std::shared_ptr<ProtocolIAuthenticatedByteStream>>&
authenticated_streams() {
  static std::unordered_map<int, std::shared_ptr<ProtocolIAuthenticatedByteStream>> streams;
  return streams;
}

std::shared_ptr<ProtocolIAuthenticatedByteStream> take_authenticated_stream(int fd) {
  std::lock_guard<std::mutex> lock(authenticated_stream_mutex());
  auto& streams = authenticated_streams();
  const auto it = streams.find(fd);
  if (it == streams.end()) return {};
  auto result = std::move(it->second);
  streams.erase(it);
  return result;
}

void put_u32(std::array<std::uint8_t, kWireHeaderBytes>& bytes, std::size_t offset,
             std::uint32_t value) {
  for (int shift = 24; shift >= 0; shift -= 8) {
    bytes[offset++] = static_cast<std::uint8_t>(value >> shift);
  }
}

void put_u64(std::array<std::uint8_t, kWireHeaderBytes>& bytes, std::size_t offset,
             std::uint64_t value) {
  for (int shift = 56; shift >= 0; shift -= 8) {
    bytes[offset++] = static_cast<std::uint8_t>(value >> shift);
  }
}

std::uint32_t get_u32(const std::array<std::uint8_t, kWireHeaderBytes>& bytes,
                     std::size_t offset) {
  std::uint32_t value = 0;
  for (int index = 0; index < 4; ++index) {
    value = (value << 8U) | bytes[offset++];
  }
  return value;
}

std::uint64_t get_u64(const std::array<std::uint8_t, kWireHeaderBytes>& bytes,
                     std::size_t offset) {
  std::uint64_t value = 0;
  for (int index = 0; index < 8; ++index) {
    value = (value << 8U) | bytes[offset++];
  }
  return value;
}

struct WireHeader {
  std::uint32_t n = 0, k = 0, length = 0;
  std::uint64_t session = 0, fingerprint = 0, sequence = 0;
  std::uint8_t bits = 0, sender = 0, receiver = 0, phase = 0, type = 0;
};

std::array<std::uint8_t, kWireHeaderBytes> encode(const WireHeader& header) {
  std::array<std::uint8_t, kWireHeaderBytes> bytes{};
  put_u32(bytes, 0, kMagic);
  bytes[4] = kVersion;
  bytes[5] = header.bits;
  bytes[6] = header.sender;
  bytes[7] = header.receiver;
  bytes[8] = header.phase;
  bytes[9] = header.type;
  put_u32(bytes, 12, header.n);
  put_u32(bytes, 16, header.k);
  put_u32(bytes, 20, header.length);
  put_u64(bytes, 24, header.session);
  put_u64(bytes, 32, header.fingerprint);
  put_u64(bytes, 40, header.sequence);
  return bytes;
}

WireHeader decode(const std::array<std::uint8_t, kWireHeaderBytes>& bytes) {
  if (get_u32(bytes, 0) != kMagic || bytes[4] != kVersion || bytes[10] != 0 || bytes[11] != 0) {
    throw ProtocolITransportError("frame header magic/version");
  }
  WireHeader header;
  header.bits = bytes[5];
  header.sender = bytes[6];
  header.receiver = bytes[7];
  header.phase = bytes[8];
  header.type = bytes[9];
  header.n = get_u32(bytes, 12);
  header.k = get_u32(bytes, 16);
  header.length = get_u32(bytes, 20);
  header.session = get_u64(bytes, 24);
  header.fingerprint = get_u64(bytes, 32);
  header.sequence = get_u64(bytes, 40);
  return header;
}

}  // namespace

void protocol_i_attach_authenticated_stream(
    int owned_fd, std::shared_ptr<ProtocolIAuthenticatedByteStream> stream) {
  if (owned_fd < 0 || !stream) throw std::invalid_argument("authenticated stream registration");
  std::lock_guard<std::mutex> lock(authenticated_stream_mutex());
  const auto inserted = authenticated_streams().emplace(owned_fd, std::move(stream)).second;
  if (!inserted) throw std::invalid_argument("authenticated stream already registered for descriptor");
}

void protocol_i_discard_unclaimed_authenticated_stream(int owned_fd) noexcept {
  if (owned_fd < 0) return;
  std::shared_ptr<ProtocolIAuthenticatedByteStream> discarded;
  {
    std::lock_guard<std::mutex> lock(authenticated_stream_mutex());
    auto& streams = authenticated_streams();
    const auto it = streams.find(owned_fd);
    if (it == streams.end()) return;
    discarded = std::move(it->second);
    streams.erase(it);
  }
  // Releasing the last registered owner closes its TLS socket.
  discarded.reset();
}

bool protocol_i_has_authenticated_stream(int owned_fd) noexcept {
  std::lock_guard<std::mutex> lock(authenticated_stream_mutex());
  return authenticated_streams().find(owned_fd) != authenticated_streams().end();
}

ProtocolIFramedChannel::ProtocolIFramedChannel(int fd, ProtocolIFrameConfig config,
                                               ProtocolIFramedChannelOptions options)
    : fd_(fd), timeout_(options.timeout_ms), max_io_chunk_(options.max_io_chunk), c_(config) {
  if (fd < 0 || timeout_ <= 0 || max_io_chunk_ == 0 || c_.bits < 34 || c_.bits > 53 || c_.sender > 2 ||
      c_.receiver > 2 || c_.sender == c_.receiver || c_.phase == 0 || c_.type == 0 ||
      c_.n == 0 || c_.k == 0 || c_.k > c_.n) {
    throw std::invalid_argument("frame config");
  }
  const bool registered = protocol_i_has_authenticated_stream(fd_);
  if (options.transport_mode == ProtocolITransportMode::RequireAuthenticatedStream) {
    if (!registered) {
      // Ownership stays with the caller on constructor failure. This matters
      // when the TLS owner is unwinding the channel set after a consumed or
      // discarded stream; closing the integer FD here could double-close it.
      throw ProtocolITransportError("authenticated stream required but absent or already consumed");
    }
    authenticated_stream_ = take_authenticated_stream(fd_);
    if (!authenticated_stream_)
      throw ProtocolITransportError("authenticated stream registration raced consumption");
  } else {
    if (registered)
      throw ProtocolITransportError("authenticated stream registered but caller-FD mode requested");
  }
}

ProtocolIFramedChannel::ProtocolIFramedChannel(int fd, ProtocolIFrameConfig config, int timeout)
    : ProtocolIFramedChannel(fd, config, ProtocolIFramedChannelOptions{timeout, std::numeric_limits<std::size_t>::max()}) {}

ProtocolIFramedChannel::~ProtocolIFramedChannel() {
  if (fd_ >= 0 && !authenticated_stream_) {
    ::close(fd_);
  }
}

void ProtocolIFramedChannel::exact(void* data, std::size_t size, bool writing,
                                   Clock::time_point deadline) {
  auto* cursor = static_cast<std::uint8_t*>(data);
  while (size != 0) {
    const auto now = Clock::now();
    if (now >= deadline) {
      throw ProtocolITransportError("frame timeout");
    }
    const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
    const int wait_ms = remaining.count() <= 0
                            ? 1
                            : remaining.count() > std::numeric_limits<int>::max()
                                  ? std::numeric_limits<int>::max()
                                  : static_cast<int>(remaining.count());
    const auto chunk = std::min(size, max_io_chunk_);
    ssize_t count = 0;
    if (authenticated_stream_) {
      try {
        count = static_cast<ssize_t>(writing
            ? authenticated_stream_->write_some(cursor, chunk, wait_ms)
            : authenticated_stream_->read_some(cursor, chunk, wait_ms));
      } catch (const ProtocolITransportError&) {
        throw;
      } catch (const std::exception& error) {
        throw ProtocolITransportError(std::string("authenticated frame I/O failed: ") + error.what());
      }
    } else {
      pollfd descriptor{fd_, static_cast<short>(writing ? POLLOUT : POLLIN), 0};
      const int wait = ::poll(&descriptor, 1, wait_ms);
      if (wait == 0) throw ProtocolITransportError("frame timeout");
      if (wait < 0) {
        if (errno == EINTR) continue;
        throw ProtocolITransportError("frame poll failed");
      }
      // A peer may close immediately after its final write. poll then legally
      // reports POLLIN|POLLHUP while the final framed bytes remain readable.
      if ((descriptor.revents & (POLLERR | POLLNVAL)) != 0 ||
          ((descriptor.revents & POLLHUP) != 0 && (writing || (descriptor.revents & POLLIN) == 0))) {
        throw ProtocolITransportError("frame poll error");
      }
      count = writing ? ::write(fd_, cursor, chunk) : ::read(fd_, cursor, chunk);
    }
    if (count < 0 && errno == EINTR) {
      continue;
    }
    if (count < 0) {
      throw ProtocolITransportError("frame I/O failed");
    }
    if (count == 0) {
      throw ProtocolITransportError("frame EOF");
    }
    if (writing) {
      sent_ += static_cast<std::uint64_t>(count);
    } else {
      received_ += static_cast<std::uint64_t>(count);
    }
    cursor += count;
    size -= static_cast<std::size_t>(count);
  }
}

void ProtocolIFramedChannel::send(const std::vector<std::uint8_t>& payload) {
  if (payload.size() > kMaxPayload) {
    throw std::invalid_argument("frame length");
  }
  const auto deadline = Clock::now() + std::chrono::milliseconds(timeout_);
  const WireHeader header{c_.n,
                          c_.k,
                          static_cast<std::uint32_t>(payload.size()),
                          c_.session,
                          c_.fingerprint,
                          out_,
                          c_.bits,
                          c_.sender,
                          c_.receiver,
                          c_.phase,
                          c_.type};
  const auto encoded = encode(header);
  exact(const_cast<std::uint8_t*>(encoded.data()), encoded.size(), true, deadline);
  if (!payload.empty()) {
    exact(const_cast<std::uint8_t*>(payload.data()), payload.size(), true, deadline);
  }
  ++out_;
}

std::vector<std::uint8_t> ProtocolIFramedChannel::receive() {
  const auto deadline = Clock::now() + std::chrono::milliseconds(timeout_);
  std::array<std::uint8_t, kWireHeaderBytes> encoded{};
  exact(encoded.data(), encoded.size(), false, deadline);
  const auto header = decode(encoded);
  if (header.n != c_.n || header.k != c_.k || header.session != c_.session ||
      header.fingerprint != c_.fingerprint || header.sequence != in_ ||
      header.bits != c_.bits || header.sender != c_.receiver || header.receiver != c_.sender ||
      header.phase != c_.phase || header.type != c_.type || header.length > kMaxPayload) {
    throw ProtocolITransportError("frame header");
  }
  std::vector<std::uint8_t> payload(header.length);
  if (!payload.empty()) {
    exact(payload.data(), payload.size(), false, deadline);
  }
  ++in_;
  return payload;
}

void protocol_i_send_framed_chunks(ProtocolIFramedChannel& channel,
                                   const std::vector<std::uint8_t>& message) {
  constexpr std::size_t kChunkBytes = kMaxPayload - kChunkEnvelopeBytes;
  if (message.empty()) {
    throw std::invalid_argument("chunked message is empty");
  }
  for (std::size_t offset = 0; offset < message.size(); offset += kChunkBytes) {
    const auto length = std::min(kChunkBytes, message.size() - offset);
    std::vector<std::uint8_t> chunk;
    chunk.reserve(kChunkEnvelopeBytes + length);
    for (int shift = 56; shift >= 0; shift -= 8) {
      chunk.push_back(static_cast<std::uint8_t>(message.size() >> shift));
    }
    for (int shift = 56; shift >= 0; shift -= 8) {
      chunk.push_back(static_cast<std::uint8_t>(offset >> shift));
    }
    chunk.insert(chunk.end(), message.begin() + offset, message.begin() + offset + length);
    channel.send(chunk);
  }
}

std::vector<std::uint8_t> protocol_i_receive_framed_chunks(ProtocolIFramedChannel& channel,
                                                            std::size_t max_message_bytes) {
  std::vector<std::uint8_t> message;
  std::size_t expected = 0;
  for (;;) {
    const auto chunk = channel.receive();
    if (chunk.size() < kChunkEnvelopeBytes) {
      throw ProtocolITransportError("chunked frame envelope");
    }
    std::uint64_t total = 0, offset = 0;
    for (int index = 0; index < 8; ++index) total = (total << 8U) | chunk[index];
    for (int index = 0; index < 8; ++index) offset = (offset << 8U) | chunk[8 + index];
    if (total == 0 || total > max_message_bytes || total > std::numeric_limits<std::size_t>::max() ||
        offset != expected || offset > total || chunk.size() - kChunkEnvelopeBytes > total - offset) {
      throw ProtocolITransportError("chunked frame bounds");
    }
    if (message.empty()) message.reserve(static_cast<std::size_t>(total));
    message.insert(message.end(), chunk.begin() + kChunkEnvelopeBytes, chunk.end());
    expected = message.size();
    if (expected == total) return message;
  }
}
}  // namespace moe_topk
