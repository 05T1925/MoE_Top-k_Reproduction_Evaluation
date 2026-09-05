#include <moe_topk/protocol_i_transport.h>

#include <array>
#include <cerrno>
#include <limits>
#include <poll.h>
#include <stdexcept>
#include <unistd.h>

namespace moe_topk {
namespace {

constexpr std::uint32_t kMagic = UINT32_C(0x4d325446);
constexpr std::uint8_t kVersion = 1;
constexpr std::uint32_t kMaxPayload = 1U << 20;
constexpr std::size_t kWireHeaderBytes = 48;

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
    throw std::runtime_error("frame header magic/version");
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

ProtocolIFramedChannel::ProtocolIFramedChannel(int fd, ProtocolIFrameConfig config, int timeout)
    : fd_(fd), timeout_(timeout), c_(config) {
  if (fd < 0 || timeout <= 0 || c_.bits < 34 || c_.bits > 53 || c_.sender > 1 ||
      c_.receiver > 1 || c_.sender == c_.receiver || c_.phase == 0 || c_.type == 0 ||
      c_.n == 0 || c_.k == 0 || c_.k > c_.n) {
    throw std::invalid_argument("frame config");
  }
}

ProtocolIFramedChannel::~ProtocolIFramedChannel() {
  if (fd_ >= 0) {
    ::close(fd_);
  }
}

void ProtocolIFramedChannel::exact(void* data, std::size_t size, bool writing,
                                   Clock::time_point deadline) {
  auto* cursor = static_cast<std::uint8_t*>(data);
  while (size != 0) {
    const auto now = Clock::now();
    if (now >= deadline) {
      throw std::runtime_error("frame timeout");
    }
    const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
    const int wait_ms = remaining.count() <= 0
                            ? 1
                            : remaining.count() > std::numeric_limits<int>::max()
                                  ? std::numeric_limits<int>::max()
                                  : static_cast<int>(remaining.count());
    pollfd descriptor{fd_, static_cast<short>(writing ? POLLOUT : POLLIN), 0};
    const int wait = ::poll(&descriptor, 1, wait_ms);
    if (wait == 0) {
      throw std::runtime_error("frame timeout");
    }
    if (wait < 0) {
      if (errno == EINTR) {
        continue;
      }
      throw std::runtime_error("frame poll failed");
    }
    if ((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
      throw std::runtime_error("frame poll error");
    }
    const auto count = writing ? ::write(fd_, cursor, size) : ::read(fd_, cursor, size);
    if (count < 0 && errno == EINTR) {
      continue;
    }
    if (count < 0) {
      throw std::runtime_error("frame I/O failed");
    }
    if (count == 0) {
      throw std::runtime_error("frame EOF");
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
    throw std::runtime_error("frame header");
  }
  ++in_;
  std::vector<std::uint8_t> payload(header.length);
  if (!payload.empty()) {
    exact(payload.data(), payload.size(), false, deadline);
  }
  return payload;
}
}  // namespace moe_topk
