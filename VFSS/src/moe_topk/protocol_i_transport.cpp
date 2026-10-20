#include <moe_topk/protocol_i_transport.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <fcntl.h>
#include <limits>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/sha.h>
#include <openssl/x509v3.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdexcept>
#include <time.h>
#include <unistd.h>

namespace moe_topk {
namespace {

constexpr std::uint32_t kMagic = UINT32_C(0x4d325446);
constexpr std::uint8_t kLegacyVersion = 1;
constexpr std::uint8_t kMaterialBoundVersion = 2;
constexpr std::uint32_t kMaxPayload = 1U << 20;
constexpr std::size_t kChunkEnvelopeBytes = 16;
constexpr std::size_t kLegacyWireHeaderBytes = 48;
constexpr std::size_t kMaterialBoundWireHeaderBytes = 56;
constexpr int kPollSliceMs = 100;

template <typename WriteOperation, typename IsSocketSigpipeFailure>
auto socket_operation_without_sigpipe(WriteOperation&& write_operation,
                                      IsSocketSigpipeFailure&& is_socket_sigpipe_failure)
    -> decltype(write_operation()) {
  sigset_t sigpipe_set{};
  sigset_t old_mask{};
  sigset_t pending_before{};
  if (::sigemptyset(&sigpipe_set) != 0 || ::sigaddset(&sigpipe_set, SIGPIPE) != 0)
    throw std::runtime_error("cannot configure SIGPIPE mask");
  const int block_status = ::pthread_sigmask(SIG_BLOCK, &sigpipe_set, &old_mask);
  if (block_status != 0)
    throw std::runtime_error("cannot block SIGPIPE for socket write");
  if (::sigpending(&pending_before) != 0) {
    (void)::pthread_sigmask(SIG_SETMASK, &old_mask, nullptr);
    throw std::runtime_error("cannot inspect pending SIGPIPE state");
  }

  const auto result = write_operation();
  const int write_errno = errno;
  sigset_t pending_after{};
  if (::sigpending(&pending_after) != 0) {
    const int pending_errno = errno;
    (void)::pthread_sigmask(SIG_SETMASK, &old_mask, nullptr);
    errno = pending_errno;
    throw std::runtime_error("cannot inspect socket-write SIGPIPE state");
  }
  const bool signal_was_preexisting =
      ::sigismember(&pending_before, SIGPIPE) == 1;
  const bool new_sigpipe_pending = !signal_was_preexisting &&
      ::sigismember(&pending_after, SIGPIPE) == 1;
  // Inspect the pending set independently from the operation result. OpenSSL
  // can finish a protocol step while a lower-level socket write records
  // EPIPE; the newly pending signal plus errno captured from this operation
  // identifies that case without treating every SSL_ERROR_SSL as a socket
  // failure. A failure without a new signal never enters sigtimedwait.
  const bool socket_failure = is_socket_sigpipe_failure(
      result, write_errno, new_sigpipe_pending);
  if (new_sigpipe_pending && socket_failure) {
    const timespec no_wait{0, 0};
    for (;;) {
      if (::sigtimedwait(&sigpipe_set, nullptr, &no_wait) >= 0) break;
      if (errno == EINTR) continue;
      if (errno == EAGAIN) break;
      const int wait_errno = errno;
      (void)::pthread_sigmask(SIG_SETMASK, &old_mask, nullptr);
      errno = write_errno;
      throw std::runtime_error("cannot consume socket-write SIGPIPE: " +
                               std::to_string(wait_errno));
    }
  }
  const int restore_status = ::pthread_sigmask(SIG_SETMASK, &old_mask, nullptr);
  errno = write_errno;
  if (restore_status != 0)
    throw std::runtime_error("cannot restore SIGPIPE mask after socket write");
  return result;
}

void put_u32(std::array<std::uint8_t, kMaterialBoundWireHeaderBytes>& bytes, std::size_t offset,
             std::uint32_t value) {
  for (int shift = 24; shift >= 0; shift -= 8) {
    bytes[offset++] = static_cast<std::uint8_t>(value >> shift);
  }
}

void put_u64(std::array<std::uint8_t, kMaterialBoundWireHeaderBytes>& bytes, std::size_t offset,
             std::uint64_t value) {
  for (int shift = 56; shift >= 0; shift -= 8) {
    bytes[offset++] = static_cast<std::uint8_t>(value >> shift);
  }
}

std::uint32_t get_u32(const std::array<std::uint8_t, kMaterialBoundWireHeaderBytes>& bytes,
                     std::size_t offset) {
  std::uint32_t value = 0;
  for (int index = 0; index < 4; ++index) {
    value = (value << 8U) | bytes[offset++];
  }
  return value;
}

std::uint64_t get_u64(const std::array<std::uint8_t, kMaterialBoundWireHeaderBytes>& bytes,
                     std::size_t offset) {
  std::uint64_t value = 0;
  for (int index = 0; index < 8; ++index) {
    value = (value << 8U) | bytes[offset++];
  }
  return value;
}

struct WireHeader {
  std::uint32_t n = 0, k = 0, length = 0;
  std::uint64_t session = 0, fingerprint = 0, sequence = 0, material_id = 0;
  std::uint8_t version = 0;
  std::uint8_t bits = 0, sender = 0, receiver = 0, phase = 0, type = 0;
};

std::array<std::uint8_t, kMaterialBoundWireHeaderBytes> encode(
    const WireHeader& header) {
  std::array<std::uint8_t, kMaterialBoundWireHeaderBytes> bytes{};
  put_u32(bytes, 0, kMagic);
  bytes[4] = header.version;
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
  if (header.version == kMaterialBoundVersion)
    put_u64(bytes, 48, header.material_id);
  return bytes;
}

WireHeader decode(const std::array<std::uint8_t, kMaterialBoundWireHeaderBytes>& bytes,
                  std::uint64_t expected_material_id) {
  const auto expected_version = expected_material_id == 0
      ? kLegacyVersion : kMaterialBoundVersion;
  if (get_u32(bytes, 0) != kMagic || bytes[4] != expected_version ||
      bytes[10] != 0 || bytes[11] != 0) {
    throw std::runtime_error("frame header magic/version");
  }
  WireHeader header;
  header.version = bytes[4];
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
  if (header.version == kMaterialBoundVersion)
    header.material_id = get_u64(bytes, 48);
  return header;
}

int remaining_ms(std::chrono::steady_clock::time_point deadline) {
  const auto now = std::chrono::steady_clock::now();
  if (now >= deadline) throw std::runtime_error("frame timeout");
  const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
      deadline - now);
  return remaining.count() <= 0 ? 1 : static_cast<int>(std::min<std::int64_t>(
      remaining.count(), std::numeric_limits<int>::max()));
}

void wait_for_fd(int fd, short events,
                 std::chrono::steady_clock::time_point deadline) {
  for (;;) {
    pollfd descriptor{fd, events, 0};
    const int wait_ms = std::min(remaining_ms(deadline), kPollSliceMs);
    const int result = ::poll(&descriptor, 1, wait_ms);
    if (result == 0) continue;
    if (result < 0 && errno == EINTR) continue;
    if (result < 0 || (descriptor.revents & (POLLERR | POLLNVAL)) != 0 ||
        ((descriptor.revents & POLLHUP) != 0 &&
         (descriptor.revents & events) == 0)) {
      throw std::runtime_error("frame poll error");
    }
    if ((descriptor.revents & events) != 0) return;
  }
}

std::string openssl_error(const char* prefix) {
  const auto code = ERR_get_error();
  if (code == 0) return prefix;
  char detail[256]{};
  ERR_error_string_n(code, detail, sizeof(detail));
  return std::string(prefix) + ": " + detail;
}

}  // namespace

SSL_CTX* protocol_i_tls13_context_create(
    const ProtocolITls13Credentials& credentials) {
  if (credentials.ca_certificate_file.empty() ||
      credentials.certificate_file.empty() ||
      credentials.private_key_file.empty()) {
    throw std::invalid_argument("TLS 1.3 mTLS credentials are incomplete");
  }
  SSL_CTX* context = SSL_CTX_new(TLS_method());
  if (context == nullptr) throw std::runtime_error(openssl_error("TLS context"));
  const bool configured =
      SSL_CTX_set_min_proto_version(context, TLS1_3_VERSION) == 1 &&
      SSL_CTX_set_max_proto_version(context, TLS1_3_VERSION) == 1 &&
      SSL_CTX_load_verify_locations(context,
          credentials.ca_certificate_file.c_str(), nullptr) == 1 &&
      SSL_CTX_use_certificate_chain_file(context,
          credentials.certificate_file.c_str()) == 1 &&
      SSL_CTX_use_PrivateKey_file(context,
          credentials.private_key_file.c_str(), SSL_FILETYPE_PEM) == 1 &&
      SSL_CTX_check_private_key(context) == 1;
  if (!configured) {
    const auto error = openssl_error("TLS 1.3 mTLS credential load");
    SSL_CTX_free(context);
    throw std::runtime_error(error);
  }
  SSL_CTX_set_verify(context,
      SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
  SSL_CTX_set_verify_depth(context, 4);
  return context;
}

void protocol_i_tls13_context_free(SSL_CTX* context) noexcept {
  if (context != nullptr) SSL_CTX_free(context);
}

ProtocolIFramedEndpoint protocol_i_tls13_wrap_fd(
    int connected_fd, SSL_CTX* context, bool server,
    const std::string& expected_peer_identity) {
  if (connected_fd < 0 || context == nullptr || expected_peer_identity.empty())
    throw std::invalid_argument("TLS endpoint binding");
  SSL* ssl = SSL_new(context);
  if (ssl == nullptr) throw std::runtime_error(openssl_error("TLS stream"));
  if (SSL_set_fd(ssl, connected_fd) != 1 ||
      SSL_set1_host(ssl, expected_peer_identity.c_str()) != 1) {
    const auto error = openssl_error("TLS stream setup");
    SSL_free(ssl);
    throw std::runtime_error(error);
  }
  if (!server && SSL_set_tlsext_host_name(ssl,
          expected_peer_identity.c_str()) != 1) {
    const auto error = openssl_error("TLS SNI setup");
    SSL_free(ssl);
    throw std::runtime_error(error);
  }
  const int old_flags = ::fcntl(connected_fd, F_GETFL, 0);
  if (old_flags < 0 || ::fcntl(connected_fd, F_SETFL, old_flags | O_NONBLOCK) < 0) {
    SSL_free(ssl);
    throw std::runtime_error("TLS nonblocking fd setup");
  }
  if (server) SSL_set_accept_state(ssl);
  else SSL_set_connect_state(ssl);
  return {connected_fd, ssl, server, expected_peer_identity};
}

ProtocolIFramedChannel::ProtocolIFramedChannel(int fd, ProtocolIFrameConfig config,
                                               ProtocolIFramedChannelOptions options)
    : ProtocolIFramedChannel(ProtocolIFramedEndpoint(fd), config, options) {}

ProtocolIFramedChannel::ProtocolIFramedChannel(
    ProtocolIFramedEndpoint endpoint, ProtocolIFrameConfig config,
    ProtocolIFramedChannelOptions options)
    : fd_(endpoint.fd), timeout_(options.timeout_ms),
      max_io_chunk_(options.max_io_chunk), ssl_(endpoint.ssl),
      tls_server_(endpoint.tls_server),
      expected_peer_identity_(std::move(endpoint.expected_peer_identity)), c_(config) {
  if (fd_ < 0 || timeout_ <= 0 || max_io_chunk_ == 0 || c_.bits < 34 || c_.bits > 53 || c_.sender > 2 ||
      c_.receiver > 2 || c_.sender == c_.receiver || c_.phase == 0 || c_.type == 0 ||
      c_.n == 0 || c_.k == 0 || c_.k > c_.n ||
      (ssl_ != nullptr && expected_peer_identity_.empty())) {
    throw std::invalid_argument("frame config");
  }
}

ProtocolIFramedChannel::ProtocolIFramedChannel(int fd, ProtocolIFrameConfig config, int timeout)
    : ProtocolIFramedChannel(fd, config, ProtocolIFramedChannelOptions{timeout, std::numeric_limits<std::size_t>::max()}) {}

ProtocolIFramedChannel::~ProtocolIFramedChannel() {
  if (ssl_ != nullptr) SSL_free(ssl_);
  if (fd_ >= 0) {
    ::close(fd_);
  }
}

void ProtocolIFramedChannel::set_phase(std::uint8_t phase) {
  if (phase == 0) throw std::invalid_argument("frame phase");
  c_.phase = phase;
}

void ProtocolIFramedChannel::complete_authenticated_handshake() {
  if (ssl_ == nullptr) throw std::logic_error("v3 requires an authenticated TLS endpoint");
  ensure_tls_handshake(Clock::now() + std::chrono::milliseconds(timeout_));
}

std::array<std::uint8_t, 32>
ProtocolIFramedChannel::peer_certificate_sha256() const {
  if (ssl_ == nullptr || !tls_handshake_complete_)
    throw std::logic_error("peer certificate requested before authenticated TLS handshake");
  X509* certificate = SSL_get_peer_certificate(ssl_);
  if (certificate == nullptr)
    throw std::runtime_error("authenticated channel has no peer certificate");
  const int encoded_size = i2d_X509(certificate, nullptr);
  if (encoded_size <= 0 || encoded_size > (1 << 20)) {
    X509_free(certificate);
    throw std::runtime_error("peer certificate DER size is invalid");
  }
  std::vector<std::uint8_t> encoded(static_cast<std::size_t>(encoded_size));
  auto* cursor = encoded.data();
  const int written = i2d_X509(certificate, &cursor);
  X509_free(certificate);
  if (written != encoded_size)
    throw std::runtime_error("peer certificate DER encoding failed");
  std::array<std::uint8_t, 32> digest{};
  if (SHA256(encoded.data(), encoded.size(), digest.data()) == nullptr)
    throw std::runtime_error("peer certificate SHA-256 failed");
  return digest;
}

void ProtocolIFramedChannel::ensure_tls_handshake(Clock::time_point deadline) {
  if (ssl_ == nullptr || tls_handshake_complete_) return;
  for (;;) {
    struct TlsHandshakeResult {
      int result;
      int ssl_error;
      int operation_errno;
    };
    const auto handshake = socket_operation_without_sigpipe([&] {
      ERR_clear_error();
      errno = 0;
      const int result = SSL_do_handshake(ssl_);
      const int operation_errno = errno;
      const int ssl_error = result == 1 ? SSL_ERROR_NONE : SSL_get_error(ssl_, result);
      errno = operation_errno;
      return TlsHandshakeResult{result, ssl_error, operation_errno};
    }, [](const TlsHandshakeResult& result, int, bool) {
      return result.operation_errno == EPIPE;
    });
    const int result = handshake.result;
    if (result == 1) {
      if (SSL_get_verify_result(ssl_) != X509_V_OK)
        throw std::runtime_error("TLS peer certificate chain rejected");
      X509* peer = SSL_get_peer_certificate(ssl_);
      if (peer == nullptr) throw std::runtime_error("TLS peer certificate missing");
      const int identity_ok = X509_check_host(
          peer, expected_peer_identity_.c_str(),
          expected_peer_identity_.size(), 0, nullptr);
      X509_free(peer);
      if (identity_ok != 1)
        throw std::runtime_error("TLS peer identity mismatch");
      tls_handshake_complete_ = true;
      return;
    }
    const int error = handshake.ssl_error;
    if (error == SSL_ERROR_WANT_READ) wait_for_fd(fd_, POLLIN, deadline);
    else if (error == SSL_ERROR_WANT_WRITE) wait_for_fd(fd_, POLLOUT, deadline);
    else throw std::runtime_error(openssl_error("TLS handshake failed"));
  }
}

void ProtocolIFramedChannel::exact(void* data, std::size_t size, bool writing,
                                   Clock::time_point deadline) {
  ensure_tls_handshake(deadline);
  auto* cursor = static_cast<std::uint8_t*>(data);
  while (size != 0) {
    const auto chunk = std::min(size, max_io_chunk_);
    if (ssl_ != nullptr) {
      // SSL_read may write a fatal TLS alert while reporting an abrupt peer
      // close, so both TLS directions must protect the socket operation.
      struct TlsIoResult {
        int count;
        int ssl_error;
        int operation_errno;
      };
      const auto tls_io = socket_operation_without_sigpipe([&] {
        ERR_clear_error();
        errno = 0;
        const int count = writing
            ? SSL_write(ssl_, cursor,
                        static_cast<int>(std::min<std::size_t>(chunk, INT_MAX)))
            : SSL_read(ssl_, cursor,
                       static_cast<int>(std::min<std::size_t>(chunk, INT_MAX)));
        const int operation_errno = errno;
        const int ssl_error = count > 0 ? SSL_ERROR_NONE : SSL_get_error(ssl_, count);
        errno = operation_errno;
        return TlsIoResult{count, ssl_error, operation_errno};
      }, [](const TlsIoResult& result, int, bool) {
        return result.operation_errno == EPIPE;
      });
      const int count = tls_io.count;
      if (count > 0) {
        if (writing) sent_ += static_cast<std::uint64_t>(count);
        else received_ += static_cast<std::uint64_t>(count);
        cursor += count;
        size -= static_cast<std::size_t>(count);
        continue;
      }
      const int error = tls_io.ssl_error;
      if (error == SSL_ERROR_WANT_READ) wait_for_fd(fd_, POLLIN, deadline);
      else if (error == SSL_ERROR_WANT_WRITE) wait_for_fd(fd_, POLLOUT, deadline);
      else if (error == SSL_ERROR_ZERO_RETURN)
        throw std::runtime_error("TLS peer closed");
      else
        throw std::runtime_error(openssl_error("TLS record I/O failed"));
      continue;
    }
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
    // A peer may close immediately after its final write. poll then legally
    // reports POLLIN|POLLHUP while the final framed bytes remain readable.
    if ((descriptor.revents & (POLLERR | POLLNVAL)) != 0 ||
        ((descriptor.revents & POLLHUP) != 0 && (writing || (descriptor.revents & POLLIN) == 0))) {
      throw std::runtime_error("frame poll error");
    }
    const auto count = writing
        ? socket_operation_without_sigpipe(
              [&] {
                errno = 0;
                return ::write(fd_, cursor, chunk);
              },
              [](ssize_t result, int operation_errno, bool) {
                return result < 0 && operation_errno == EPIPE;
              })
        : ::read(fd_, cursor, chunk);
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
  const auto version = c_.material_id == 0
      ? kLegacyVersion : kMaterialBoundVersion;
  const WireHeader header{c_.n,
                          c_.k,
                          static_cast<std::uint32_t>(payload.size()),
                          c_.session,
                          c_.fingerprint,
                          out_,
                          c_.material_id,
                          version,
                          c_.bits,
                          c_.sender,
                          c_.receiver,
                          c_.phase,
                          c_.type};
  const auto encoded = encode(header);
  const auto header_size = c_.material_id == 0
      ? kLegacyWireHeaderBytes : kMaterialBoundWireHeaderBytes;
  exact(const_cast<std::uint8_t*>(encoded.data()), header_size, true, deadline);
  if (!payload.empty()) {
    exact(const_cast<std::uint8_t*>(payload.data()), payload.size(), true, deadline);
  }
  ++out_;
  ++sent_frames_;
  max_payload_sent_ = std::max(max_payload_sent_, payload.size());
}

std::vector<std::uint8_t> ProtocolIFramedChannel::receive() {
  const auto deadline = Clock::now() + std::chrono::milliseconds(timeout_);
  std::array<std::uint8_t, kMaterialBoundWireHeaderBytes> encoded{};
  const auto header_size = c_.material_id == 0
      ? kLegacyWireHeaderBytes : kMaterialBoundWireHeaderBytes;
  exact(encoded.data(), header_size, false, deadline);
  const auto header = decode(encoded, c_.material_id);
  if (header.n != c_.n || header.k != c_.k || header.session != c_.session ||
      header.fingerprint != c_.fingerprint || header.sequence != in_ ||
      header.material_id != c_.material_id ||
      header.bits != c_.bits || header.sender != c_.receiver || header.receiver != c_.sender ||
      header.phase != c_.phase || header.type != c_.type || header.length > kMaxPayload) {
    throw std::runtime_error("frame header");
  }
  std::vector<std::uint8_t> payload(header.length);
  if (!payload.empty()) {
    exact(payload.data(), payload.size(), false, deadline);
  }
  ++in_;
  ++received_frames_;
  max_payload_received_ = std::max(max_payload_received_, payload.size());
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
      throw std::runtime_error("chunked frame envelope");
    }
    std::uint64_t total = 0, offset = 0;
    for (int index = 0; index < 8; ++index) total = (total << 8U) | chunk[index];
    for (int index = 0; index < 8; ++index) offset = (offset << 8U) | chunk[8 + index];
    if (total == 0 || total > max_message_bytes || total > std::numeric_limits<std::size_t>::max() ||
        offset != expected || offset > total || chunk.size() - kChunkEnvelopeBytes > total - offset) {
      throw std::runtime_error("chunked frame bounds");
    }
    if (message.empty()) message.reserve(static_cast<std::size_t>(total));
    message.insert(message.end(), chunk.begin() + kChunkEnvelopeBytes, chunk.end());
    expected = message.size();
    if (expected == total) return message;
  }
}
}  // namespace moe_topk
