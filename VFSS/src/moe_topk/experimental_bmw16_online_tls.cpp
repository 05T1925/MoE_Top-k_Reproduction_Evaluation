#include <moe_topk/experimental_bmw16_online_tls.h>

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <netdb.h>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

namespace moe_topk {
namespace {

constexpr std::array<const char*, kProtocolIBmw16OnlineChannelCount> kChannelNames{{
    "raw-score-0", "raw-score-1", "forward-shuffle-0", "forward-shuffle-1",
    "select-r1", "select-r2", "select-r3", "select-r4",
    "inverse-shuffle-0", "inverse-shuffle-1", "final-agreement", "sampling-coin"}};
constexpr char kImplementation[] =
    "Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL";

std::string openssl_error(const char* prefix) {
  const auto code = ERR_get_error();
  char text[256]{};
  if (code != 0) ERR_error_string_n(code, text, sizeof(text));
  return std::string(prefix) + (code == 0 ? "" : ": ") + text;
}

void append_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
  out.push_back(static_cast<std::uint8_t>(value >> 8U));
  out.push_back(static_cast<std::uint8_t>(value));
}
void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
  for (int shift = 24; shift >= 0; shift -= 8)
    out.push_back(static_cast<std::uint8_t>(value >> shift));
}
void append_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
  for (int shift = 56; shift >= 0; shift -= 8)
    out.push_back(static_cast<std::uint8_t>(value >> shift));
}

std::vector<std::uint8_t> binding_bytes(const ProtocolIBmw16OnlineTlsConfig& c,
                                        std::size_t channel) {
  std::vector<std::uint8_t> out{'M','6','B','2','1','O','N','L'};
  append_u16(out, 1);
  append_u64(out, c.session);
  append_u64(out, c.fingerprint);
  append_u32(out, c.n);
  append_u32(out, c.k);
  out.push_back(c.comparison_bits);
  out.push_back(static_cast<std::uint8_t>(channel));
  out.push_back(0);  // The canonical connection direction is P0 -> P1.
  out.push_back(1);
  append_u16(out, static_cast<std::uint16_t>(std::strlen(kChannelNames[channel])));
  const auto* label = reinterpret_cast<const std::uint8_t*>(kChannelNames[channel]);
  out.insert(out.end(), label, label + std::strlen(kChannelNames[channel]));
  append_u16(out, static_cast<std::uint16_t>(sizeof(kImplementation) - 1U));
  const auto* implementation = reinterpret_cast<const std::uint8_t*>(kImplementation);
  out.insert(out.end(), implementation, implementation + sizeof(kImplementation) - 1U);
  return out;
}

void wait_fd(int fd, short events, int timeout_ms, const char* failure) {
  pollfd descriptor{fd, events, 0};
  for (;;) {
    const auto result = ::poll(&descriptor, 1, timeout_ms);
    if (result > 0) {
      if ((descriptor.revents & (POLLERR | POLLNVAL)) != 0 ||
          ((descriptor.revents & POLLHUP) != 0 && (descriptor.revents & events) == 0))
        throw ProtocolITransportError(failure);
      return;
    }
    if (result == 0) throw ProtocolITransportError("online TLS timeout");
    if (errno != EINTR) throw ProtocolITransportError(failure);
  }
}

int make_listener(const std::string& address, std::uint16_t port) {
  addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICSERV;
  addrinfo* list = nullptr;
  const auto service = std::to_string(port);
  const int resolved = ::getaddrinfo(address.c_str(), service.c_str(), &hints, &list);
  if (resolved != 0) throw ProtocolITransportError("online TLS bind address resolution");
  int fd = -1;
  for (auto* item = list; item; item = item->ai_next) {
    fd = ::socket(item->ai_family, item->ai_socktype | SOCK_CLOEXEC, item->ai_protocol);
    if (fd < 0) continue;
    int one = 1;
    (void)::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if (::bind(fd, item->ai_addr, item->ai_addrlen) == 0 && ::listen(fd, 4) == 0) break;
    ::close(fd);
    fd = -1;
  }
  ::freeaddrinfo(list);
  if (fd < 0) throw ProtocolITransportError("online TLS listener bind/listen failed");
  return fd;
}

int connect_peer(const std::string& address, std::uint16_t port, int timeout_ms) {
  addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICSERV;
  addrinfo* list = nullptr;
  const auto service = std::to_string(port);
  const int resolved = ::getaddrinfo(address.c_str(), service.c_str(), &hints, &list);
  if (resolved != 0) throw ProtocolITransportError("online TLS peer address resolution");
  int connected = -1;
  for (auto* item = list; item; item = item->ai_next) {
    const int fd = ::socket(item->ai_family, item->ai_socktype | SOCK_CLOEXEC, item->ai_protocol);
    if (fd < 0) continue;
    const int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
      ::close(fd);
      continue;
    }
    const int result = ::connect(fd, item->ai_addr, item->ai_addrlen);
    if (result == 0) {
      connected = fd;
      break;
    }
    if (errno == EINPROGRESS) {
      try {
        wait_fd(fd, POLLOUT, timeout_ms, "online TLS connect failed");
        int error = 0;
        socklen_t size = sizeof(error);
        if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &size) == 0 && error == 0) {
          connected = fd;
          break;
        }
      } catch (...) {
      }
    }
    ::close(fd);
  }
  ::freeaddrinfo(list);
  if (connected < 0) throw ProtocolITransportError("online TLS connect failed");
  const int flags = ::fcntl(connected, F_GETFL, 0);
  if (flags < 0 || ::fcntl(connected, F_SETFL, flags & ~O_NONBLOCK) != 0) {
    ::close(connected);
    throw ProtocolITransportError("online TLS connected socket blocking setup");
  }
  return connected;
}

int accept_peer(int listener, int timeout_ms) {
  wait_fd(listener, POLLIN, timeout_ms, "online TLS accept failed");
  const int fd = ::accept4(listener, nullptr, nullptr, SOCK_CLOEXEC);
  if (fd < 0) throw ProtocolITransportError("online TLS accept failed");
  return fd;
}

using SslContext = std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)>;
using SslHandle = std::unique_ptr<SSL, decltype(&SSL_free)>;

SslContext make_context(const ProtocolIBmw16OnlineTlsConfig& c) {
  ERR_clear_error();
  SslContext context(SSL_CTX_new(TLS_method()), SSL_CTX_free);
  if (!context) throw ProtocolITransportError(openssl_error("online TLS context"));
  if (SSL_CTX_set_min_proto_version(context.get(), TLS1_3_VERSION) != 1 ||
      SSL_CTX_set_max_proto_version(context.get(), TLS1_3_VERSION) != 1)
    throw ProtocolITransportError(openssl_error("online TLS 1.3 version"));
  SSL_CTX_set_session_cache_mode(context.get(), SSL_SESS_CACHE_OFF);
  SSL_CTX_set_options(context.get(), SSL_OP_NO_TICKET);
  SSL_CTX_set_max_early_data(context.get(), 0);
  SSL_CTX_set_verify(context.get(), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
  if (SSL_CTX_load_verify_locations(context.get(), c.trust_bundle_pem.c_str(), nullptr) != 1 ||
      SSL_CTX_use_certificate_chain_file(context.get(), c.certificate_pem.c_str()) != 1 ||
      SSL_CTX_use_PrivateKey_file(context.get(), c.private_key_pem.c_str(), SSL_FILETYPE_PEM) != 1 ||
      SSL_CTX_check_private_key(context.get()) != 1)
    throw ProtocolITransportError(openssl_error("online TLS credential load"));
  return context;
}

void set_socket_timeouts(int fd, int timeout_ms) {
  timeval timeout{};
  timeout.tv_sec = timeout_ms / 1000;
  timeout.tv_usec = (timeout_ms % 1000) * 1000;
  if (::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0 ||
      ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) != 0)
    throw ProtocolITransportError("online TLS socket timeout configuration");
}

void set_nonblocking(int fd) {
  const int flags = ::fcntl(fd, F_GETFL, 0);
  if (flags < 0 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0)
    throw ProtocolITransportError("online TLS nonblocking data socket setup");
}

int remaining_ms(std::chrono::steady_clock::time_point deadline) {
  const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
      deadline - std::chrono::steady_clock::now()).count();
  if (remaining <= 0) throw ProtocolITransportError("online TLS I/O timeout");
  return static_cast<int>(std::min<std::int64_t>(remaining, INT32_MAX));
}

std::size_t ssl_read_bounded(SSL* ssl, int fd, std::uint8_t* out,
                             std::size_t capacity, int timeout_ms) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  for (;;) {
    std::size_t received = 0;
    ERR_clear_error();
    const int result = SSL_read_ex(ssl, out, capacity, &received);
    if (result == 1) return received;
    const int error = SSL_get_error(ssl, result);
    if (error == SSL_ERROR_ZERO_RETURN) return 0;
    short events = 0;
    if (error == SSL_ERROR_WANT_READ) events = POLLIN;
    else if (error == SSL_ERROR_WANT_WRITE) events = POLLOUT;
    else throw ProtocolITransportError(openssl_error("authenticated TLS read"));
    wait_fd(fd, events, remaining_ms(deadline), "authenticated TLS read/peer close");
  }
}

std::size_t ssl_write_bounded(SSL* ssl, int fd, const std::uint8_t* data,
                              std::size_t size, int timeout_ms) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  for (;;) {
    std::size_t written = 0;
    ERR_clear_error();
    const int result = SSL_write_ex(ssl, data, size, &written);
    if (result == 1) return written;
    const int error = SSL_get_error(ssl, result);
    short events = 0;
    if (error == SSL_ERROR_WANT_READ) events = POLLIN;
    else if (error == SSL_ERROR_WANT_WRITE) events = POLLOUT;
    else throw ProtocolITransportError(openssl_error("authenticated TLS write"));
    wait_fd(fd, events, remaining_ms(deadline), "authenticated TLS write/peer close");
  }
}

void handshake(SSL* ssl, int fd, bool server, int timeout_ms) {
  const int original_flags = ::fcntl(fd, F_GETFL, 0);
  if (original_flags < 0 || ::fcntl(fd, F_SETFL, original_flags | O_NONBLOCK) != 0)
    throw ProtocolITransportError("online TLS socket nonblocking setup");
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  for (;;) {
    const int result = server ? SSL_accept(ssl) : SSL_connect(ssl);
    if (result == 1) break;
    const int error = SSL_get_error(ssl, result);
    short events = 0;
    if (error == SSL_ERROR_WANT_READ) events = POLLIN;
    else if (error == SSL_ERROR_WANT_WRITE) events = POLLOUT;
    else throw ProtocolITransportError(openssl_error("online TLS handshake/identity"));
    const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
        deadline - std::chrono::steady_clock::now()).count();
    if (remaining <= 0) throw ProtocolITransportError("online TLS handshake timeout");
    wait_fd(fd, events, static_cast<int>(std::min<std::int64_t>(remaining, INT32_MAX)),
            "online TLS handshake I/O");
  }
  if (::fcntl(fd, F_SETFL, original_flags) != 0)
    throw ProtocolITransportError("online TLS socket restore failed");
  set_socket_timeouts(fd, timeout_ms);
  if (SSL_get_verify_result(ssl) != X509_V_OK)
    throw ProtocolITransportError("online TLS certificate chain rejected");
  X509* peer = SSL_get1_peer_certificate(ssl);
  if (!peer) throw ProtocolITransportError("online TLS peer certificate absent");
  X509_free(peer);
}

void verify_peer_identity(SSL* ssl, const std::string& expected) {
  X509* peer = SSL_get1_peer_certificate(ssl);
  if (!peer) throw ProtocolITransportError("online TLS peer certificate absent");
  const int matched = X509_check_host(peer, expected.c_str(), expected.size(), 0, nullptr);
  X509_free(peer);
  if (matched != 1) throw ProtocolITransportError("online TLS peer identity mismatch");
}

SslHandle make_ssl(SSL_CTX* context, int fd, bool server,
                   const ProtocolIBmw16OnlineTlsConfig& c) {
  SslHandle ssl(SSL_new(context), SSL_free);
  if (!ssl || SSL_set_fd(ssl.get(), fd) != 1)
    throw ProtocolITransportError(openssl_error("online TLS SSL object"));
  // The stream wrapper owns and closes this descriptor explicitly.
  BIO_set_close(SSL_get_rbio(ssl.get()), BIO_NOCLOSE);
  SSL_set_mode(ssl.get(), SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
  if (!server && SSL_set1_host(ssl.get(), c.expected_peer_identity.c_str()) != 1)
    throw ProtocolITransportError(openssl_error("online TLS expected identity"));
  handshake(ssl.get(), fd, server, c.timeout_ms);
  verify_peer_identity(ssl.get(), c.expected_peer_identity);
  set_nonblocking(fd);
  return ssl;
}

void ssl_write_all(SSL* ssl, const std::uint8_t* data, std::size_t length, int timeout_ms) {
  while (length != 0) {
    const auto written = ssl_write_bounded(ssl, SSL_get_fd(ssl), data, length, timeout_ms);
    if (written == 0) throw ProtocolITransportError("online TLS binding write made no progress");
    data += written;
    length -= written;
  }
}

void ssl_read_all(SSL* ssl, std::uint8_t* data, std::size_t length, int timeout_ms) {
  while (length != 0) {
    const auto received = ssl_read_bounded(ssl, SSL_get_fd(ssl), data, length, timeout_ms);
    if (received == 0) throw ProtocolITransportError("online TLS binding peer closed");
    data += received;
    length -= received;
  }
}

void verify_channel_binding(SSL* ssl, const ProtocolIBmw16OnlineTlsConfig& c,
                            std::size_t channel, bool server) {
  const auto expected = binding_bytes(c, channel);
  if (server) {
    std::vector<std::uint8_t> received(expected.size());
    ssl_read_all(ssl, received.data(), received.size(), c.timeout_ms);
    if (received != expected)
      throw ProtocolITransportError("online TLS session/config/channel binding mismatch");
  } else {
    ssl_write_all(ssl, expected.data(), expected.size(), c.timeout_ms);
  }
  std::array<std::uint8_t, 16> acknowledgement{};
  acknowledgement[0]='M'; acknowledgement[1]='6'; acknowledgement[2]='B';
  acknowledgement[3]='2'; acknowledgement[4]='1'; acknowledgement[5]='A';
  acknowledgement[6]='C'; acknowledgement[7]='K';
  for (unsigned i = 0; i < 8; ++i)
    acknowledgement[8U + i] = static_cast<std::uint8_t>(c.session >> (56U - 8U * i));
  if (server) {
    ssl_write_all(ssl, acknowledgement.data(), acknowledgement.size(), c.timeout_ms);
  } else {
    std::array<std::uint8_t, 16> received{};
    ssl_read_all(ssl, received.data(), received.size(), c.timeout_ms);
    if (received != acknowledgement)
      throw ProtocolITransportError("online TLS acknowledgement binding mismatch");
  }
}

class SslAuthenticatedStream final : public ProtocolIAuthenticatedByteStream {
 public:
  SslAuthenticatedStream(SSL* ssl, int fd, int timeout_ms)
      : ssl_(ssl, SSL_free), fd_(fd), timeout_ms_(timeout_ms) {}
  ~SslAuthenticatedStream() override {
    if (ssl_) SSL_free(ssl_.release());
    if (fd_ >= 0) {
      (void)::shutdown(fd_, SHUT_RDWR);
      ::close(fd_);
    }
  }

  std::size_t read_some(std::uint8_t* out, std::size_t capacity,
                        int timeout_ms) override {
    const auto received = ssl_read_bounded(ssl_.get(), fd_, out, capacity,
                                           std::min(timeout_ms_, timeout_ms));
    if (received == 0) throw ProtocolITransportError("authenticated peer closed TLS stream");
    return received;
  }

  std::size_t write_some(const std::uint8_t* data, std::size_t size,
                         int timeout_ms) override {
    const auto written = ssl_write_bounded(ssl_.get(), fd_, data, size,
                                           std::min(timeout_ms_, timeout_ms));
    if (written == 0) throw ProtocolITransportError("authenticated TLS write made no progress");
    return written;
  }

 private:
  SslHandle ssl_;
  int fd_;
  int timeout_ms_;
};

void signal_listener_ready(const ProtocolIBmw16OnlineTlsConfig& c) {
  if (c.listener_ready_fd < 0) return;
  std::vector<std::uint8_t> receipt{'M','6','B','2','1','L','S','T'};
  append_u16(receipt, 1);
  append_u64(receipt, c.session);
  append_u64(receipt, c.fingerprint);
  append_u32(receipt, c.n);
  append_u32(receipt, c.k);
  append_u16(receipt, c.local_port_base);
  std::size_t offset = 0;
  while (offset < receipt.size()) {
    const auto written = ::write(c.listener_ready_fd, receipt.data() + offset, receipt.size() - offset);
    if (written < 0 && errno == EINTR) continue;
    if (written <= 0) throw ProtocolITransportError("startup supervisor listener receipt failed");
    offset += static_cast<std::size_t>(written);
  }
  ::close(c.listener_ready_fd);
}

void validate_config(const ProtocolIBmw16OnlineTlsConfig& c) {
  if (c.session == 0 || c.fingerprint == 0 || c.n == 0 || c.k == 0 || c.k > c.n ||
      c.comparison_bits < 34 || c.comparison_bits > 53 || c.party > 1 || c.timeout_ms <= 0 ||
      c.bind_address.empty() || c.certificate_pem.empty() || c.private_key_pem.empty() ||
      c.trust_bundle_pem.empty() || c.expected_peer_identity.empty())
    throw std::invalid_argument("online TLS configuration");
  if (c.party == 1) {
    if (c.local_port_base == 0 ||
        static_cast<std::uint32_t>(c.local_port_base) + kProtocolIBmw16OnlineChannelCount - 1U > 65535U)
      throw std::invalid_argument("online TLS listener port range");
  } else if (c.peer_address.empty() || c.peer_port_base == 0 ||
      static_cast<std::uint32_t>(c.peer_port_base) + kProtocolIBmw16OnlineChannelCount - 1U > 65535U) {
    throw std::invalid_argument("online TLS peer port range");
  }
}

}  // namespace

ProtocolIBmw16OnlineTlsChannels::ProtocolIBmw16OnlineTlsChannels(
    const ProtocolIBmw16OnlineTlsConfig& c) {
  fds_.fill(-1);
  registered_.fill(false);
  validate_config(c);
  auto context = make_context(c);
  std::array<int, kProtocolIBmw16OnlineChannelCount> listeners{};
  listeners.fill(-1);
  try {
    if (c.party == 1) {
      for (std::size_t i = 0; i < listeners.size(); ++i)
        listeners[i] = make_listener(c.bind_address,
            static_cast<std::uint16_t>(c.local_port_base + i));
      signal_listener_ready(c);
      for (std::size_t i = 0; i < fds_.size(); ++i) {
        const int fd = accept_peer(listeners[i], c.timeout_ms);
        fds_[i] = fd;
        auto ssl = make_ssl(context.get(), fd, true, c);
        verify_channel_binding(ssl.get(), c, i, true);
        protocol_i_attach_authenticated_stream(fd,
            std::make_shared<SslAuthenticatedStream>(ssl.release(), fd, c.timeout_ms));
        registered_[i] = true;
        ::close(listeners[i]);
        listeners[i] = -1;
      }
    } else {
      for (std::size_t i = 0; i < fds_.size(); ++i) {
        const auto port = static_cast<std::uint16_t>(c.peer_port_base + i);
        const int fd = connect_peer(c.peer_address, port, c.timeout_ms);
        fds_[i] = fd;
        auto ssl = make_ssl(context.get(), fd, false, c);
        verify_channel_binding(ssl.get(), c, i, false);
        protocol_i_attach_authenticated_stream(fd,
            std::make_shared<SslAuthenticatedStream>(ssl.release(), fd, c.timeout_ms));
        registered_[i] = true;
      }
    }
  } catch (...) {
    for (auto fd : listeners) if (fd >= 0) ::close(fd);
    for (std::size_t i = 0; i < fds_.size(); ++i) {
      if (fds_[i] < 0) continue;
      if (registered_[i]) protocol_i_discard_unclaimed_authenticated_stream(fds_[i]);
      else ::close(fds_[i]);
      fds_[i] = -1;
    }
    throw;
  }
}

ProtocolIBmw16OnlineTlsChannels::~ProtocolIBmw16OnlineTlsChannels() {
  for (std::size_t i = 0; i < fds_.size(); ++i)
    if (fds_[i] >= 0 && registered_[i])
      protocol_i_discard_unclaimed_authenticated_stream(fds_[i]);
}

std::array<int, kProtocolIBmw16OnlineChannelCount>
ProtocolIBmw16OnlineTlsChannels::party_fds() const { return fds_; }

void ProtocolIBmw16OnlineTlsChannels::verify_all_registered() const {
  for (auto fd : fds_)
    if (fd < 0 || !protocol_i_has_authenticated_stream(fd))
      throw ProtocolITransportError("online TLS channel set is incomplete");
}

}  // namespace moe_topk
