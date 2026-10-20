#include <moe_topk/protocol_i_transport.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <openssl/err.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

using namespace moe_topk;

namespace {

void check(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

void run_command(const std::string& command) {
  check(std::system(command.c_str()) == 0, "openssl test identity command");
}

void cleanup(const std::string& dir) {
  for (const auto* name : {"identity.key", "identity.pem", "identity.cnf"})
    (void)::unlink((dir + "/" + name).c_str());
  (void)::rmdir(dir.c_str());
}

bool tls_frame_roundtrip(SSL_CTX* p0_context, SSL_CTX* p1_context,
                         const std::string& p0_peer,
                         bool expect_success,
                         bool seed_stale_error_queue = false) {
  int sockets[2];
  check(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0, "TLS socketpair");
  auto p0 = protocol_i_tls13_wrap_fd(sockets[0], p0_context, false, p0_peer);
  auto p1 = protocol_i_tls13_wrap_fd(sockets[1], p1_context, true,
                                      "party0.test");
  ProtocolIFrameConfig c0{909, 808, 2, 1, 34, 0, 1, 41, 7, 0x12345678};
  ProtocolIFrameConfig c1{909, 808, 2, 1, 34, 1, 0, 41, 7, 0x12345678};
  bool p0_ok = false, p1_ok = false;
  std::string p0_error, p1_error;
  std::thread first([&] {
    try {
      if (seed_stale_error_queue) ERR_raise(ERR_LIB_USER, 1);
      ProtocolIFramedChannel channel(p0, c0,
          ProtocolIFramedChannelOptions{1500});
      channel.send({1, 2, 3, 4});
      p0_ok = channel.receive() == std::vector<std::uint8_t>({5, 6, 7});
    } catch (const std::exception& error) {
      p0_error = error.what(); p0_ok = false;
    }
  });
  std::thread second([&] {
    try {
      if (seed_stale_error_queue) ERR_raise(ERR_LIB_USER, 2);
      ProtocolIFramedChannel channel(p1, c1,
          ProtocolIFramedChannelOptions{1500});
      const auto incoming = channel.receive();
      channel.send({5, 6, 7});
      p1_ok = incoming == std::vector<std::uint8_t>({1, 2, 3, 4});
    } catch (const std::exception& error) {
      p1_error = error.what(); p1_ok = false;
    }
  });
  first.join();
  second.join();
  if (expect_success && (!p0_ok || !p1_ok))
    std::cerr << "TLS endpoint errors p0='" << p0_error << "' p1='"
              << p1_error << "'\n";
  return expect_success ? p0_ok && p1_ok : !p0_ok;
}

bool tls_peer_close_during_handshake(SSL_CTX* client_context,
                                     const std::string& expected_peer) {
  int sockets[2];
  check(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0,
        "TLS handshake-close socketpair");
  check(::close(sockets[1]) == 0, "TLS handshake-close peer close");
  auto endpoint = protocol_i_tls13_wrap_fd(
      sockets[0], client_context, false, expected_peer);
  ProtocolIFramedChannel channel(
      endpoint, ProtocolIFrameConfig{909, 808, 2, 1, 34, 0, 1, 41, 7, 0x12345678},
      ProtocolIFramedChannelOptions{500});
  try {
    channel.complete_authenticated_handshake();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

}  // namespace

int main() {
  char path_template[] = "/tmp/m6b5-tls-XXXXXX";
  char* path = ::mkdtemp(path_template);
  if (path == nullptr) throw std::runtime_error("TLS test mkdtemp");
  const std::string dir(path);
  try {
    // A self-signed ephemeral test identity is both trust anchor and peer
    // certificate. SAN names distinguish the two expected peer identities.
    {
      std::ofstream config(dir + "/identity.cnf");
      config << "[req]\n"
             << "distinguished_name=dn\n"
             << "prompt=no\n"
             << "[dn]\n"
             << "CN=M6B5-Test-Identity\n"
             << "[cert_ext]\n"
             << "basicConstraints=critical,CA:TRUE\n"
             << "keyUsage=critical,digitalSignature,keyCertSign,cRLSign\n"
             << "extendedKeyUsage=serverAuth,clientAuth\n"
             << "subjectAltName=DNS:party0.test,DNS:party1.test\n"
             << "subjectKeyIdentifier=hash\n"
             << "authorityKeyIdentifier=keyid:always\n";
    }
    run_command("openssl req -x509 -newkey rsa:2048 -sha256 -nodes -days 1 "
        "-keyout " + dir + "/identity.key -out " + dir + "/identity.pem "
        "-config " + dir + "/identity.cnf -extensions cert_ext "
        ">/dev/null 2>&1");
    const ProtocolITls13Credentials credentials{
        dir + "/identity.pem", dir + "/identity.pem", dir + "/identity.key"};
    SSL_CTX* p0 = protocol_i_tls13_context_create(credentials);
    SSL_CTX* p1 = protocol_i_tls13_context_create(credentials);
    const bool good = tls_frame_roundtrip(p0, p1, "party1.test", true);
    const bool stale_error_queue_cleared =
        tls_frame_roundtrip(p0, p1, "party1.test", true, true);
    const bool identity_rejected =
        tls_frame_roundtrip(p0, p1, "wrong.test", false);
    const bool handshake_close_rejected =
        tls_peer_close_during_handshake(p0, "party1.test");
    protocol_i_tls13_context_free(p0);
    protocol_i_tls13_context_free(p1);
    cleanup(dir);
    check(good, "TLS 1.3 mTLS authenticated frame roundtrip");
    check(stale_error_queue_cleared,
          "TLS I/O clears stale OpenSSL errors before handshake and records");
    check(identity_rejected, "TLS 1.3 peer identity mismatch rejected");
    check(handshake_close_rejected,
          "TLS 1.3 peer close during handshake is reported without SIGPIPE");
    std::cout << "TLS 1.3 mTLS frame binding, peer identity, error queue, and "
                 "handshake close conformance PASS\n";
    return 0;
  } catch (...) {
    cleanup(dir);
    throw;
  }
}
