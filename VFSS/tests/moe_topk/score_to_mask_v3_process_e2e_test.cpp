#ifndef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_GMW_EXPERIMENTAL
#error "This test must only be built with the explicit v3 experimental option."
#endif

#include <moe_topk/score_to_mask_v3_gmw.h>
#include <moe_topk/topk_oracle.h>
#if defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL) && \
    defined(MOE_TOPK_SCORE_TO_MASK_V3_FT_V2_TESTING)
#include "score_to_mask_v3_ft_v2_test_support.h"
#endif
#if defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL) && \
    defined(MOE_TOPK_SCORE_TO_MASK_V3_FT_V3_TESTING)
#include "score_to_mask_v3_ft_v3_test_support.h"
#endif

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <sqlite3.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sched.h>
#include <grp.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

using namespace moe_topk;

std::uint32_t test_package_format(const ScoreToMaskV3PackageInfo& info) {
#if defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL) || defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL)
  return info.package_format_version;
#else
  (void)info;
  return kScoreToMaskV3PackageFormatVersion;
#endif
}

ScoreToMaskV3GateBudget test_gate_budget(std::uint32_t n,
                                         std::uint32_t ft_version) {
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
  if (ft_version == 3) return score_to_mask_v3_gate_budget_ft_v3(n);
#else
  (void)ft_version;
#endif
  return score_to_mask_v3_gate_budget(n);
}

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

struct TempDirectory {
  std::string path;
  TempDirectory() {
    std::string pattern = (std::filesystem::temp_directory_path() /
                           "s32-v3-process-e2e-XXXXXX").string();
    std::vector<char> writable(pattern.begin(), pattern.end());
    writable.push_back('\0');
    auto* result = ::mkdtemp(writable.data());
    if (result == nullptr) throw std::runtime_error("mkdtemp failed");
    path = result;
    if (::chmod(path.c_str(), 0711) != 0)
      throw std::runtime_error("test root permission setup failed");
  }
  ~TempDirectory() {
    if (std::getenv("S35_KEEP_TEST_TMP") != nullptr) {
      std::cerr << "S35_TEMP_RETAINED " << path << '\n';
      return;
    }
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
  }
};

std::string bytes_hex(const std::uint8_t* bytes, std::size_t size) {
  static constexpr char digits[] = "0123456789abcdef";
  std::string out;
  out.reserve(size * 2U);
  for (std::size_t i = 0; i < size; ++i) {
    out.push_back(digits[bytes[i] >> 4U]);
    out.push_back(digits[bytes[i] & 0xfU]);
  }
  return out;
}

std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> generate_rsa_key() {
  std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> context(
      EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr), EVP_PKEY_CTX_free);
  if (!context || EVP_PKEY_keygen_init(context.get()) != 1 ||
      EVP_PKEY_CTX_set_rsa_keygen_bits(context.get(), 2048) != 1)
    throw std::runtime_error("RSA keygen init failed");
  EVP_PKEY* raw = nullptr;
  if (EVP_PKEY_keygen(context.get(), &raw) != 1)
    throw std::runtime_error("RSA keygen failed");
  return {raw, EVP_PKEY_free};
}

std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> generate_ed25519_key() {
  std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> context(
      EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr), EVP_PKEY_CTX_free);
  if (!context || EVP_PKEY_keygen_init(context.get()) != 1)
    throw std::runtime_error("Ed25519 keygen init failed");
  EVP_PKEY* raw = nullptr;
  if (EVP_PKEY_keygen(context.get(), &raw) != 1)
    throw std::runtime_error("Ed25519 keygen failed");
  return {raw, EVP_PKEY_free};
}

void write_private_key(const std::string& path, EVP_PKEY* key) {
  FILE* file = std::fopen(path.c_str(), "wb");
  if (file == nullptr) throw std::runtime_error("cannot create private key");
  const int ok = PEM_write_PrivateKey(file, key, nullptr, nullptr, 0, nullptr, nullptr);
  std::fclose(file);
  if (ok != 1) throw std::runtime_error("private key PEM write failed");
  if (::chmod(path.c_str(), 0600) != 0)
    throw std::runtime_error("private key permission hardening failed");
}

void write_public_key(const std::string& path, EVP_PKEY* key) {
  FILE* file = std::fopen(path.c_str(), "wb");
  if (file == nullptr) throw std::runtime_error("cannot create public key");
  const int ok = PEM_write_PUBKEY(file, key);
  std::fclose(file);
  if (ok != 1) throw std::runtime_error("public key PEM write failed");
  if (::chmod(path.c_str(), 0444) != 0)
    throw std::runtime_error("public key permission setup failed");
}

void add_extension(X509* certificate, X509* issuer, int nid,
                   const std::string& value) {
  X509V3_CTX context;
  X509V3_set_ctx(&context, issuer, certificate, nullptr, nullptr, 0);
  X509_EXTENSION* extension = X509V3_EXT_conf_nid(
      nullptr, &context, nid, const_cast<char*>(value.c_str()));
  if (extension == nullptr || X509_add_ext(certificate, extension, -1) != 1) {
    if (extension) X509_EXTENSION_free(extension);
    throw std::runtime_error("X509 extension creation failed");
  }
  X509_EXTENSION_free(extension);
}

std::unique_ptr<X509, decltype(&X509_free)> make_certificate(
    EVP_PKEY* subject_key, const std::string& common_name,
    const std::string& dns_name, X509* issuer, EVP_PKEY* issuer_key,
    bool is_ca, long serial) {
  std::unique_ptr<X509, decltype(&X509_free)> certificate(X509_new(), X509_free);
  if (!certificate || X509_set_version(certificate.get(), 2) != 1 ||
      ASN1_INTEGER_set(X509_get_serialNumber(certificate.get()), serial) != 1 ||
      X509_gmtime_adj(X509_getm_notBefore(certificate.get()), 0) == nullptr ||
      X509_gmtime_adj(X509_getm_notAfter(certificate.get()), 3600 * 24 * 365) == nullptr ||
      X509_set_pubkey(certificate.get(), subject_key) != 1)
    throw std::runtime_error("X509 certificate initialization failed");
  X509_NAME* subject = X509_get_subject_name(certificate.get());
  if (X509_NAME_add_entry_by_txt(subject, "CN", MBSTRING_ASC,
      reinterpret_cast<const unsigned char*>(common_name.data()),
      static_cast<int>(common_name.size()), -1, 0) != 1)
    throw std::runtime_error("X509 subject construction failed");
  if (issuer != nullptr) {
    if (X509_set_issuer_name(certificate.get(), X509_get_subject_name(issuer)) != 1)
      throw std::runtime_error("X509 issuer construction failed");
  } else if (X509_set_issuer_name(certificate.get(), subject) != 1) {
    throw std::runtime_error("X509 self issuer construction failed");
  }
  add_extension(certificate.get(), issuer == nullptr ? certificate.get() : issuer,
      NID_basic_constraints, is_ca ? "critical,CA:TRUE" : "critical,CA:FALSE");
  add_extension(certificate.get(), issuer == nullptr ? certificate.get() : issuer,
      NID_key_usage, is_ca ? "critical,keyCertSign,cRLSign,digitalSignature"
                           : "critical,digitalSignature,keyEncipherment");
  if (!is_ca) {
    add_extension(certificate.get(), issuer,
                  NID_ext_key_usage, "serverAuth,clientAuth");
    add_extension(certificate.get(), issuer, NID_subject_alt_name,
                  "DNS:" + dns_name);
  }
  if (X509_sign(certificate.get(), issuer_key, EVP_sha256()) <= 0)
    throw std::runtime_error("X509 certificate signing failed");
  return certificate;
}

void write_certificate(const std::string& path, X509* certificate) {
  FILE* file = std::fopen(path.c_str(), "wb");
  if (file == nullptr) throw std::runtime_error("cannot create certificate");
  const int ok = PEM_write_X509(file, certificate);
  std::fclose(file);
  if (ok != 1) throw std::runtime_error("certificate PEM write failed");
  if (::chmod(path.c_str(), 0444) != 0)
    throw std::runtime_error("certificate permission setup failed");
}

struct IdentityFiles {
  std::string certificate;
  std::string private_key;
  std::string public_key;
};

IdentityFiles create_identity(const std::string& directory, const std::string& name,
                             X509* ca, EVP_PKEY* ca_key, long serial) {
  auto key = generate_rsa_key();
  const auto cert_path = (std::filesystem::path(directory) / (name + ".crt")).string();
  const auto private_path = (std::filesystem::path(directory) / (name + ".key")).string();
  const auto public_path = (std::filesystem::path(directory) / (name + ".pub.pem")).string();
  auto certificate = make_certificate(key.get(), name, name, ca, ca_key, false, serial);
  write_certificate(cert_path, certificate.get());
  write_private_key(private_path, key.get());
  write_public_key(public_path, key.get());
  return {cert_path, private_path, public_path};
}

IdentityFiles create_identity_for_dns(const std::string& directory,
                                      const std::string& file_stem,
                                      const std::string& common_name,
                                      const std::string& dns_name,
                                      X509* ca, EVP_PKEY* ca_key, long serial) {
  auto key = generate_rsa_key();
  const auto cert_path =
      (std::filesystem::path(directory) / (file_stem + ".crt")).string();
  const auto private_path =
      (std::filesystem::path(directory) / (file_stem + ".key")).string();
  const auto public_path =
      (std::filesystem::path(directory) / (file_stem + ".pub.pem")).string();
  auto certificate = make_certificate(key.get(), common_name, dns_name,
                                      ca, ca_key, false, serial);
  write_certificate(cert_path, certificate.get());
  write_private_key(private_path, key.get());
  write_public_key(public_path, key.get());
  return {cert_path, private_path, public_path};
}

struct TestIdentities {
  std::string ca_certificate;
  std::string ca_private_key;
  IdentityFiles p0_tls;
  IdentityFiles p1_tls;
  IdentityFiles escrow_tls;
  IdentityFiles p0_recipient;
  IdentityFiles p1_recipient;
  std::string dealer_private;
  std::string dealer_public;
};

TestIdentities create_test_identities(const std::string& directory) {
  auto ca_key = generate_rsa_key();
  auto ca = make_certificate(ca_key.get(), "s32-test-ca", "", nullptr,
                             ca_key.get(), true, 1);
  const auto ca_path = (std::filesystem::path(directory) / "ca.crt").string();
  const auto ca_private_path = (std::filesystem::path(directory) / "ca.key").string();
  write_certificate(ca_path, ca.get());
  write_private_key(ca_private_path, ca_key.get());
  auto p0 = create_identity(directory, "p0", ca.get(), ca_key.get(), 2);
  auto p1 = create_identity(directory, "p1", ca.get(), ca_key.get(), 3);
  auto escrow = create_identity(directory, "escrow", ca.get(), ca_key.get(), 4);
  auto p0_recipient_key = generate_rsa_key();
  auto p1_recipient_key = generate_rsa_key();
  IdentityFiles p0_recipient{
      {}, (std::filesystem::path(directory) / "p0-recipient-private.pem").string(),
      (std::filesystem::path(directory) / "p0-recipient-public.pem").string()};
  IdentityFiles p1_recipient{
      {}, (std::filesystem::path(directory) / "p1-recipient-private.pem").string(),
      (std::filesystem::path(directory) / "p1-recipient-public.pem").string()};
  write_private_key(p0_recipient.private_key, p0_recipient_key.get());
  write_public_key(p0_recipient.public_key, p0_recipient_key.get());
  write_private_key(p1_recipient.private_key, p1_recipient_key.get());
  write_public_key(p1_recipient.public_key, p1_recipient_key.get());
  auto dealer = generate_ed25519_key();
  const auto dealer_private = (std::filesystem::path(directory) / "dealer.key").string();
  const auto dealer_public = (std::filesystem::path(directory) / "dealer.pub").string();
  write_private_key(dealer_private, dealer.get());
  write_public_key(dealer_public, dealer.get());
  return {ca_path, ca_private_path, p0, p1, escrow, p0_recipient, p1_recipient,
          dealer_private, dealer_public};
}

void write_u32(std::ostream& out, std::uint32_t value) {
  const std::array<char, 4> bytes{static_cast<char>(value >> 24U),
      static_cast<char>(value >> 16U), static_cast<char>(value >> 8U),
      static_cast<char>(value)};
  out.write(bytes.data(), bytes.size());
}

void write_u64(std::ostream& out, std::uint64_t value) {
  std::array<char, 8> bytes{};
  for (std::size_t i = 0; i < bytes.size(); ++i)
    bytes[i] = static_cast<char>(value >> (56U - 8U * i));
  out.write(bytes.data(), bytes.size());
}

std::uint32_t read_u32(std::istream& in) {
  std::array<unsigned char, 4> bytes{};
  in.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
  if (!in) throw std::runtime_error("test input u32 truncated");
  return (static_cast<std::uint32_t>(bytes[0]) << 24U) |
         (static_cast<std::uint32_t>(bytes[1]) << 16U) |
         (static_cast<std::uint32_t>(bytes[2]) << 8U) | bytes[3];
}

std::uint64_t read_u64(std::istream& in) {
  std::array<unsigned char, 8> bytes{};
  in.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
  if (!in) throw std::runtime_error("test input u64 truncated");
  std::uint64_t value = 0;
  for (const auto byte : bytes) value = (value << 8U) | byte;
  return value;
}

void write_session_file(const std::string& path,
                        const ScoreToMaskV3Session& session) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out.write(reinterpret_cast<const char*>(session.session.data()), 16);
  out.write(reinterpret_cast<const char*>(session.invocation.data()), 16);
  write_u32(out, session.n);
  write_u32(out, session.k);
  for (const auto& value : session.party_identity_sha256)
    out.write(reinterpret_cast<const char*>(value.data()), value.size());
  for (const auto& value : session.recipient_rsa_public_sha256)
    out.write(reinterpret_cast<const char*>(value.data()), value.size());
  out.write(reinterpret_cast<const char*>(session.escrow_identity_sha256.data()), 32);
  if (!out) throw std::runtime_error("test session write failed");
  out.close();
  if (::chmod(path.c_str(), 0600) != 0)
    throw std::runtime_error("test session permission hardening failed");
}

ScoreToMaskV3Session read_session_file(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("test session file unavailable");
  ScoreToMaskV3Session session;
  in.read(reinterpret_cast<char*>(session.session.data()), 16);
  in.read(reinterpret_cast<char*>(session.invocation.data()), 16);
  session.n = read_u32(in);
  session.k = read_u32(in);
  for (auto& value : session.party_identity_sha256)
    in.read(reinterpret_cast<char*>(value.data()), value.size());
  for (auto& value : session.recipient_rsa_public_sha256)
    in.read(reinterpret_cast<char*>(value.data()), value.size());
  in.read(reinterpret_cast<char*>(session.escrow_identity_sha256.data()), 32);
  if (!in) throw std::runtime_error("test session file truncated");
  return session;
}

void write_input_share(const std::string& path,
                       const std::vector<std::uint32_t>& values) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  write_u32(out, static_cast<std::uint32_t>(values.size()));
  for (auto value : values) write_u32(out, value);
  if (!out) throw std::runtime_error("test input share write failed");
  out.close();
  if (::chmod(path.c_str(), 0600) != 0)
    throw std::runtime_error("test input share permission hardening failed");
}

std::vector<std::uint32_t> read_input_share(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("party input share unavailable");
  const auto count = read_u32(in);
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
  if (count == 0 || count > kScoreToMaskV3FtV3MaxN)
#else
  if (count == 0 || count > kScoreToMaskV3MaxN)
#endif
    throw std::runtime_error("party input share count invalid");
  std::vector<std::uint32_t> result(count);
  for (auto& value : result) value = read_u32(in);
  if (in.peek() != std::char_traits<char>::eof())
    throw std::runtime_error("party input share has trailing data");
  return result;
}

struct ProcessOutput {
  std::uint8_t state = 0;
  std::string status_code;
  std::string transaction;
  std::string receipt;
  std::vector<std::uint8_t> share;
  std::uint64_t sent_bytes = 0;
  std::uint64_t received_bytes = 0;
  std::uint64_t sent_frames = 0;
  std::uint64_t received_frames = 0;
  std::uint64_t phases = 0;
  std::uint64_t max_payload_sent = 0;
  std::uint64_t max_payload_received = 0;
};

void write_process_output(const std::string& path,
                          const ScoreToMaskV3PartyOutput& output) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out.put(static_cast<char>(output.state));
  write_u32(out, static_cast<std::uint32_t>(output.abort_code.size()));
  out.write(output.abort_code.data(), static_cast<std::streamsize>(output.abort_code.size()));
  write_u32(out, static_cast<std::uint32_t>(output.transaction_id.size()));
  out.write(output.transaction_id.data(), static_cast<std::streamsize>(output.transaction_id.size()));
  write_u32(out, static_cast<std::uint32_t>(output.commit_receipt.size()));
  out.write(output.commit_receipt.data(),
            static_cast<std::streamsize>(output.commit_receipt.size()));
  write_u32(out, static_cast<std::uint32_t>(output.local_mask_share.size()));
  out.write(reinterpret_cast<const char*>(output.local_mask_share.data()),
            static_cast<std::streamsize>(output.local_mask_share.size()));
  write_u64(out, output.party_channel_bytes_sent);
  write_u64(out, output.party_channel_bytes_received);
  write_u64(out, output.party_channel_frames_sent);
  write_u64(out, output.party_channel_frames_received);
  write_u64(out, output.party_channel_phases);
  write_u64(out, output.party_channel_max_payload_sent);
  write_u64(out, output.party_channel_max_payload_received);
  if (!out) throw std::runtime_error("party output file write failed");
}

ProcessOutput read_process_output(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("party process output unavailable");
  ProcessOutput output;
  output.state = static_cast<std::uint8_t>(in.get());
  auto count = read_u32(in);
  if (count > 128) throw std::runtime_error("party status code too long");
  output.status_code.resize(count);
  in.read(output.status_code.data(), count);
  count = read_u32(in);
  if (count > 128) throw std::runtime_error("transaction output too long");
  output.transaction.resize(count);
  in.read(output.transaction.data(), count);
  count = read_u32(in);
  if ((output.state == static_cast<std::uint8_t>(
           ScoreToMaskV3PartyOutput::State::kCommitted) && count != 64) ||
      (output.state != static_cast<std::uint8_t>(
           ScoreToMaskV3PartyOutput::State::kCommitted) && count != 0))
    throw std::runtime_error("commit receipt length mismatch");
  output.receipt.resize(count);
  in.read(output.receipt.data(), count);
  count = read_u32(in);
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
  if (count > kScoreToMaskV3FtV3MaxN ||
#else
  if (count > kScoreToMaskV3MaxN ||
#endif
      (output.state == static_cast<std::uint8_t>(
           ScoreToMaskV3PartyOutput::State::kCommitted) && count == 0) ||
      (output.state != static_cast<std::uint8_t>(
           ScoreToMaskV3PartyOutput::State::kCommitted) && count != 0))
    throw std::runtime_error("local output share size mismatch");
  output.share.resize(count);
  in.read(reinterpret_cast<char*>(output.share.data()), count);
  output.sent_bytes = read_u64(in);
  output.received_bytes = read_u64(in);
  output.sent_frames = read_u64(in);
  output.received_frames = read_u64(in);
  output.phases = read_u64(in);
  output.max_payload_sent = read_u64(in);
  output.max_payload_received = read_u64(in);
  if (!in || in.peek() != std::char_traits<char>::eof())
    throw std::runtime_error("party output file truncated/trailing data");
  return output;
}

pid_t exec_child(const std::string& binary, const std::vector<std::string>& arguments,
                 const std::string& stdout_path = {},
                 const std::string& stderr_path = {},
                 bool drop_to_nobody = false) {
  const auto pid = ::fork();
  if (pid < 0) throw std::runtime_error("fork failed");
  if (pid == 0) {
    const auto redirect = [](const std::string& path, int target) {
      if (path.empty()) return true;
      const int file = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
      if (file < 0) return false;
      const bool ok = ::dup2(file, target) >= 0;
      ::close(file);
      return ok;
    };
    if (!redirect(stdout_path, STDOUT_FILENO) ||
        !redirect(stderr_path, STDERR_FILENO)) _exit(126);
    const rlimit no_core{0, 0};
    if (::setrlimit(RLIMIT_CORE, &no_core) != 0) _exit(125);
    if (drop_to_nobody) {
      if (::setgroups(0, nullptr) != 0 ||
          ::setgid(65534) != 0 || ::setuid(65534) != 0)
        _exit(125);
    }
    std::vector<char*> argv;
    argv.reserve(arguments.size() + 2U);
    argv.push_back(const_cast<char*>(binary.c_str()));
    for (const auto& arg : arguments) argv.push_back(const_cast<char*>(arg.c_str()));
    argv.push_back(nullptr);
    ::execv(binary.c_str(), argv.data());
    _exit(127);
  }
  return pid;
}

int wait_child(pid_t pid, long* peak_rss_kib = nullptr) {
  int status = 0;
  rusage usage{};
  if (::wait4(pid, &status, 0, &usage) != pid) throw std::runtime_error("wait4 failed");
  if (peak_rss_kib != nullptr) *peak_rss_kib = usage.ru_maxrss;
  if (!WIFEXITED(status)) return 128 + (WIFSIGNALED(status) ? WTERMSIG(status) : 0);
  return WEXITSTATUS(status);
}

class ChildProcess {
 public:
  explicit ChildProcess(pid_t pid) : pid_(pid) {}
  ChildProcess(const ChildProcess&) = delete;
  ChildProcess& operator=(const ChildProcess&) = delete;
  ~ChildProcess() {
    if (pid_ <= 0) return;
    (void)::kill(pid_, SIGTERM);
    int status = 0;
    while (::waitpid(pid_, &status, 0) < 0 && errno == EINTR) {}
  }
  void signal(int sig) {
    if (pid_ > 0 && ::kill(pid_, sig) != 0 && errno != ESRCH)
      throw std::runtime_error("child process signal failed");
  }
  int wait(long* peak_rss_kib = nullptr) {
    if (pid_ <= 0) throw std::runtime_error("child process already reaped");
    const auto pid = pid_;
    pid_ = -1;
    return wait_child(pid, peak_rss_kib);
  }
  pid_t pid() const { return pid_; }

 private:
  pid_t pid_ = -1;
};

struct EscrowSandbox {
  std::string context;
  std::string dealer_public;
  std::string ca_certificate;
  std::string tls_certificate;
  std::string tls_private_key;
  std::string database;
  bool run_as_nobody = false;
};

void require_sys(bool ok, const char* operation) {
  if (!ok) throw std::runtime_error(std::string(operation) + " failed: " +
                                    std::strerror(errno));
}

EscrowSandbox prepare_escrow_sandbox(
    const std::string& case_directory, const std::string& p0_package,
    const std::string& p1_package, const std::string& context,
    const TestIdentities& identities) {
  EscrowSandbox sandbox;
  sandbox.run_as_nobody = ::geteuid() == 0;
  const auto root = std::filesystem::path(case_directory);
  if (sandbox.run_as_nobody) {
    require_sys(::chmod(case_directory.c_str(), 0711) == 0,
                "case directory traversal permission");
    require_sys(::chmod(p0_package.c_str(), 0600) == 0 &&
                    ::chmod(p1_package.c_str(), 0600) == 0,
                "party package permission isolation");
  }
  const auto config_dir = root / "escrow-a-config";
  const auto state_dir = root / "escrow-a-state";
  std::filesystem::create_directory(config_dir);
  std::filesystem::create_directory(state_dir);
  const auto copy_for_a = [&](const std::string& src, const char* name,
                              mode_t mode) {
    const auto dst = config_dir / name;
    std::filesystem::copy_file(src, dst,
        std::filesystem::copy_options::overwrite_existing);
    if (sandbox.run_as_nobody) {
      require_sys(::chown(dst.c_str(), 65534, 65534) == 0,
                  "escrow config ownership");
      require_sys(::chmod(dst.c_str(), mode) == 0,
                  "escrow config permissions");
    }
    return dst.string();
  };
  if (sandbox.run_as_nobody) {
    require_sys(::chown(config_dir.c_str(), 65534, 65534) == 0 &&
                    ::chmod(config_dir.c_str(), 0711) == 0,
                "escrow config directory isolation");
    require_sys(::chown(state_dir.c_str(), 65534, 65534) == 0 &&
                    ::chmod(state_dir.c_str(), 0700) == 0,
                "escrow database directory isolation");
  }
  sandbox.context = copy_for_a(context, "escrow.context", 0444);
  sandbox.dealer_public = copy_for_a(identities.dealer_public, "dealer.pub", 0444);
  sandbox.ca_certificate = copy_for_a(identities.ca_certificate, "ca.crt", 0444);
  sandbox.tls_certificate = copy_for_a(identities.escrow_tls.certificate,
                                       "escrow.crt", 0444);
  sandbox.tls_private_key = copy_for_a(identities.escrow_tls.private_key,
                                       "escrow.key", 0400);
  sandbox.database = (state_dir / "escrow.sqlite3").string();
  if (sandbox.run_as_nobody) {
    const auto pid = ::fork();
    if (pid < 0) throw std::runtime_error("fork for escrow material isolation failed");
    if (pid == 0) {
      if (::setgroups(0, nullptr) != 0 || ::setgid(65534) != 0 ||
          ::setuid(65534) != 0) _exit(120);
      for (const auto& path : {p0_package, p1_package}) {
        const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
        if (fd >= 0) { ::close(fd); _exit(121); }
        if (errno != EACCES) _exit(122);
      }
      _exit(0);
    }
    int status = 0;
    while (::waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
    require(WIFEXITED(status) && WEXITSTATUS(status) == 0,
            "escrow identity could read a party package or did not run as nobody");
  }
  return sandbox;
}

void verify_escrow_process_identity(const ChildProcess& service,
                                   const EscrowSandbox& sandbox,
                                   const std::string& p0_package,
                                   const std::string& p1_package) {
  if (!sandbox.run_as_nobody) return;
  std::ifstream status_file("/proc/" + std::to_string(service.pid()) + "/status");
  const std::string status((std::istreambuf_iterator<char>(status_file)), {});
  require(status.find("Uid:\t65534\t65534\t65534\t65534") != std::string::npos,
          "escrow process did not retain nobody credentials");
  std::ifstream command_file("/proc/" + std::to_string(service.pid()) + "/cmdline",
                             std::ios::binary);
  const std::string command((std::istreambuf_iterator<char>(command_file)), {});
  require(command.find(p0_package) == std::string::npos &&
              command.find(p1_package) == std::string::npos &&
              command.find(sandbox.context) != std::string::npos,
          "escrow argv exposes a party package or omits metadata context");
}

std::uint16_t reserve_port() {
  const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) throw std::runtime_error("test socket create failed");
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  address.sin_port = 0;
  if (::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0)
    throw std::runtime_error("test socket bind failed");
  socklen_t size = sizeof(address);
  if (::getsockname(fd, reinterpret_cast<sockaddr*>(&address), &size) != 0)
    throw std::runtime_error("test getsockname failed");
  const auto port = ntohs(address.sin_port);
  ::close(fd);
  return port;
}

void run_escrow_same_san_wrong_pin_probe(
    const std::string& directory, const TestIdentities& identities,
    const ScoreToMaskV3PackageInfo& package,
    const std::string& signed_context_path, const std::string& dealer_public,
    const std::string& transaction) {
  auto ca_file = std::fopen(identities.ca_certificate.c_str(), "rb");
  require(ca_file != nullptr, "same-SAN probe could not read test CA");
  X509* ca_raw = PEM_read_X509(ca_file, nullptr, nullptr, nullptr);
  std::fclose(ca_file);
  require(ca_raw != nullptr, "same-SAN probe could not parse test CA");
  std::unique_ptr<X509, decltype(&X509_free)> ca(ca_raw, X509_free);
  auto ca_key_file = std::fopen(identities.ca_private_key.c_str(), "rb");
  require(ca_key_file != nullptr, "same-SAN probe could not read test CA key");
  EVP_PKEY* ca_key_raw = PEM_read_PrivateKey(ca_key_file, nullptr, nullptr, nullptr);
  std::fclose(ca_key_file);
  require(ca_key_raw != nullptr, "same-SAN probe could not parse test CA key");
  std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> ca_key(ca_key_raw,
                                                            EVP_PKEY_free);
  const auto alternate = create_identity_for_dns(
      directory, "escrow-alt-same-san", "escrow", "escrow",
      ca.get(), ca_key.get(), 92);
  const auto expected_fingerprint = package.parameters.escrow_identity_sha256;
  require(score_to_mask_v3_certificate_sha256(alternate.certificate) !=
              expected_fingerprint,
          "same-SAN probe alternate certificate unexpectedly matches signed pin");
  const auto context = score_to_mask_v3_validate_escrow_context(
      signed_context_path, dealer_public);
  const int listener = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  require(listener >= 0, "same-SAN probe listener socket failed");
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  address.sin_port = 0;
  require(::bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0,
          "same-SAN probe bind failed");
  socklen_t address_size = sizeof(address);
  require(::getsockname(listener, reinterpret_cast<sockaddr*>(&address),
                        &address_size) == 0 && ::listen(listener, 1) == 0,
          "same-SAN probe listener setup failed");
  const auto port = ntohs(address.sin_port);
  std::atomic<bool> server_handshake_ok{false};
  std::atomic<bool> server_received_app_frame{false};
  std::string server_error;
  std::thread server([&] {
    SSL_CTX* tls = nullptr;
    try {
      const int peer = ::accept4(listener, nullptr, nullptr, SOCK_CLOEXEC);
      if (peer < 0) throw std::runtime_error("same-SAN probe accept failed");
      tls = protocol_i_tls13_context_create(
          {identities.ca_certificate, alternate.certificate, alternate.private_key});
      auto endpoint = protocol_i_tls13_wrap_fd(peer, tls, true, "p0");
      ProtocolIFramedChannel channel(
          endpoint, score_to_mask_v3_escrow_frame_config(context, 0, true),
          ProtocolIFramedChannelOptions{1200, 4096});
      channel.complete_authenticated_handshake();
      if (channel.peer_certificate_sha256() != context.party_identity_sha256[0])
        throw std::runtime_error("same-SAN probe party pin mismatch");
      server_handshake_ok.store(true);
      try {
        (void)channel.receive();
        server_received_app_frame.store(true);
      } catch (const std::exception&) {
        // A correctly pinned client closes immediately after TLS identity
        // verification and before waiting for an application READY frame.
      }
    } catch (const std::exception& error) {
      server_error = error.what();
    }
    if (tls != nullptr) protocol_i_tls13_context_free(tls);
  });

  std::string client_error;
  try {
    const ScoreToMaskV3EscrowClientConfig client{
        "127.0.0.1", port, "escrow",
        {identities.ca_certificate, identities.p0_tls.certificate,
         identities.p0_tls.private_key}};
    (void)score_to_mask_v3_escrow_fetch(client, package, 0, transaction);
  } catch (const std::exception& error) {
    client_error = error.what();
  }
  server.join();
  ::close(listener);
  if (!server_handshake_ok.load())
    throw std::runtime_error("same-SAN probe server TLS handshake failed: " +
                             server_error);
  require(!server_received_app_frame.load(),
          "client sent an app request to same-SAN wrong-pin escrow endpoint");
  require(client_error.find("certificate pin mismatch") != std::string::npos,
          "client did not reject same-SAN wrong-pin escrow before READY");
}

void wait_port_ready(std::uint16_t port) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while (std::chrono::steady_clock::now() < deadline) {
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) throw std::runtime_error("test readiness socket failed");
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port);
    const auto connected = ::connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address));
    ::close(fd);
    if (connected == 0) return;
    ::sched_yield();
  }
  throw std::runtime_error("escrow service listener readiness timeout");
}

std::pair<int, int> escrow_state_and_share_count(const std::string& path,
                                                const std::string& transaction) {
  sqlite3* db = nullptr;
  if (sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
    if (db != nullptr) sqlite3_close(db);
    throw std::runtime_error("test controller cannot read escrow SQLite state");
  }
  sqlite3_stmt* stmt = nullptr;
  const char* query =
      "SELECT state,(SELECT count(*) FROM shares WHERE transaction_id=?1) "
      "FROM transactions WHERE transaction_id=?1";
  if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
    sqlite3_close(db);
    throw std::runtime_error("test controller cannot prepare escrow state query");
  }
  sqlite3_bind_text(stmt, 1, transaction.c_str(), -1, SQLITE_TRANSIENT);
  const int step = sqlite3_step(stmt);
  if (step != SQLITE_ROW) {
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return {-1, 0};
  }
  const std::pair<int, int> result{sqlite3_column_int(stmt, 0),
                                   sqlite3_column_int(stmt, 1)};
  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return result;
}

void wait_for_one_precommit_ciphertext(const std::string& db_path) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while (std::chrono::steady_clock::now() < deadline) {
    sqlite3* db = nullptr;
    if (sqlite3_open_v2(db_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) == SQLITE_OK) {
      sqlite3_stmt* stmt = nullptr;
      const char* query =
          "SELECT count(*) FROM transactions t WHERE state=0 AND "
          "(SELECT count(*) FROM shares WHERE transaction_id=t.transaction_id)=1";
      if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) == SQLITE_OK &&
          sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) == 1) {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
      }
      if (stmt != nullptr) sqlite3_finalize(stmt);
      sqlite3_close(db);
    } else if (db != nullptr) {
      sqlite3_close(db);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  throw std::runtime_error("A did not durably stage exactly one pre-commit ciphertext");
}

bool has_aborted_precommit_record(const std::string& db_path,
                                  const std::string& transaction) {
  return escrow_state_and_share_count(db_path, transaction) == std::pair<int, int>{2, 1};
}

int party_worker(int argc, char** argv) {
  if (argc != 20 && argc != 21) return 64;
  const auto party = static_cast<std::uint8_t>(std::stoi(argv[2]));
  const int fd = std::stoi(argv[3]);
  const int close_fd = std::stoi(argv[4]);
  if (close_fd >= 0) ::close(close_fd);
  if (argc == 21) {
    const int extra_close_fd = std::stoi(argv[20]);
    if (extra_close_fd >= 0 && extra_close_fd != fd && extra_close_fd != close_fd)
      ::close(extra_close_fd);
  }
  const auto package_path = std::string(argv[5]);
  const auto dealer_public = std::string(argv[6]);
  const auto session = read_session_file(argv[7]);
  const auto input = read_input_share(argv[8]);
  const auto lease_dir = std::string(argv[9]);
  const auto recipient_public = std::string(argv[10]);
  const auto ca = std::string(argv[11]);
  const auto tls_certificate = std::string(argv[12]);
  const auto tls_private = std::string(argv[13]);
  const auto port = static_cast<std::uint16_t>(std::stoi(argv[14]));
  const auto output_path = std::string(argv[15]);
  const bool drop_receipt_after_commit =
      std::string(argv[16]) == "drop-receipt-after-commit";
  const bool fail_before_submit =
      std::string(argv[16]) == "p1-fail-before-submit";
  const auto& peer_name = party == 0 ? "p1" : "p0";
  auto info = score_to_mask_v3_validate_package(package_path, dealer_public,
                                                session, party);
  auto tls_context = protocol_i_tls13_context_create(
      {ca, tls_certificate, tls_private});
  ScoreToMaskV3PartyOutput output;
  {
    auto endpoint = protocol_i_tls13_wrap_fd(fd, tls_context, party == 0,
                                             peer_name);
    ProtocolIFramedChannel channel(endpoint, score_to_mask_v3_frame_config(info, party),
                                   ProtocolIFramedChannelOptions{15000, 4096});
    ScoreToMaskV3EscrowClientConfig escrow{
        "127.0.0.1", port, "escrow", {ca, tls_certificate, tls_private}};
    const auto submitter = [&](const std::string& transaction,
                               std::uint8_t recipient,
                               const std::vector<std::uint8_t>& cipher) {
      if (fail_before_submit && party == 1)
        throw std::runtime_error("TEST_ONLY P1 exits before escrow submit");
      const auto receipt = score_to_mask_v3_escrow_submit(
          escrow, info, recipient, transaction, cipher);
      if (drop_receipt_after_commit && party == 0)
        throw std::runtime_error("TEST_ONLY lost receipt after durable commit");
      return receipt;
    };
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
    const auto party_memory_cap = info.ft_version == kScoreToMaskV3FtVersionV3
        ? 2ULL * 1024ULL * 1024ULL * 1024ULL
        : 256ULL * 1024ULL * 1024ULL;
#else
    const auto party_memory_cap = 256ULL * 1024ULL * 1024ULL;
#endif
    output = score_to_mask_v3_execute_party(package_path, dealer_public,
        session, {input}, party, lease_dir, party_memory_cap,
        channel, recipient_public, submitter);
  }
  protocol_i_tls13_context_free(tls_context);
  write_process_output(output_path, output);
  if (output.state == ScoreToMaskV3PartyOutput::State::kCommitted) return 0;
  if (output.state == ScoreToMaskV3PartyOutput::State::kRecoveryRequired) return 11;
  return 10;
}

int dealer_worker(int argc, char** argv) {
  if (argc != 10 && argc != 11 && argc != 12) return 64;
  const auto session = read_session_file(argv[2]);
  ScoreToMaskV3EscrowStateParameters state;
  state.transaction_wait_timeout_ms = static_cast<std::uint32_t>(std::stoul(argv[7]));
  state.receipt_format_version = static_cast<std::uint8_t>(std::stoul(argv[8]));
  const auto ft_version = static_cast<std::uint32_t>(std::stoul(argv[9]));
  if (ft_version == 1 && (argc == 10 || argc == 11)) {
    score_to_mask_v3_dealer_issue_packages(session, argv[3], argv[4], argv[5],
                                            argv[6], state);
  } else if (ft_version == 2 && argc == 11) {
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL
    score_to_mask_v3_dealer_issue_packages_ft_v2_bounded(
        session, argv[3], argv[4], argv[5], argv[6], argv[10], state);
#else
    throw std::runtime_error("F_T/v2 is not enabled in this build");
#endif
  } else if (ft_version == 2 && argc == 12 &&
             std::string(argv[11]) == "test-only-force-bounded-abort") {
#if defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL) && \
    defined(MOE_TOPK_SCORE_TO_MASK_V3_FT_V2_TESTING)
    std::size_t bytes = 0;
    std::size_t calls = 0;
    std::size_t max_request = 0;
    test_only::score_to_mask_v3_dealer_issue_packages_ft_v2_with_test_random(
        session, argv[3], argv[4], argv[5], argv[6], argv[10], state, false,
        &bytes, &calls, &max_request);
#else
    throw std::runtime_error("TEST_ONLY F_T/v2 random source is unavailable");
#endif
  } else if (ft_version == 3 && argc == 11) {
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
    score_to_mask_v3_dealer_issue_packages_ft_v3_bounded(
        session, argv[3], argv[4], argv[5], argv[6], argv[10], state);
#else
    throw std::runtime_error("F_T/v3 is not enabled in this build");
#endif
  } else if (ft_version == 3 && argc == 12 &&
             std::string(argv[11]) == "test-only-force-ft-v3-abort") {
#if defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL) && \
    defined(MOE_TOPK_SCORE_TO_MASK_V3_FT_V3_TESTING)
    std::size_t bytes = 0, calls = 0, maximum = 0;
    try {
      test_only::score_to_mask_v3_dealer_issue_packages_ft_v3_with_test_random(
          session, argv[3], argv[4], argv[5], argv[6], argv[10], state,
          &bytes, &calls, &maximum);
    } catch (...) {
      std::cout << "S47_FT_V3_TEST_RANDOM {\"bytes\":" << bytes
                << ",\"calls\":" << calls
                << ",\"max_request_bytes\":" << maximum << "}\n";
      throw;
    }
#else
    throw std::runtime_error("TEST_ONLY F_T/v3 random source is unavailable");
#endif
  } else {
    throw std::runtime_error("unsupported F_T version or test mode");
  }
  return 0;
}

int escrow_worker(int argc, char** argv) {
  if (argc != 10) return 64;
  ScoreToMaskV3EscrowServerConfig config;
  config.bind_address = "127.0.0.1";
  config.party_ports = {static_cast<std::uint16_t>(std::stoi(argv[4])),
                        static_cast<std::uint16_t>(std::stoi(argv[5]))};
  config.party_tls_identities = {"p0", "p1"};
  config.sqlite_database_path = argv[6];
  config.tls_credentials = {argv[7], argv[8], argv[9]};
  config.signed_context_path = argv[2];
  config.dealer_ed25519_public_key_pem = argv[3];
  score_to_mask_v3_run_escrow_service(config);
  return 0;
}

void run_one_e2e(const std::string& binary, const std::string& directory,
                 const TestIdentities& identities,
                 const std::vector<std::int32_t>& scores, std::uint32_t k,
                 bool lose_receipt_p0 = false,
                 bool fail_p1_before_submit = false,
                 bool timeout_precommit = false,
                 std::uint8_t receipt_format_version = 1,
                 const std::string& case_id = "unspecified",
                 std::uint32_t ft_version = 1) {
  if (std::getenv("S35_TRACE_CASES") != nullptr)
    std::cerr << "S35_CASE_START dir=" << directory << " n=" << scores.size()
              << " k=" << k << " lost_receipt=" << lose_receipt_p0
              << " p1_pre_submit_failure=" << fail_p1_before_submit
              << " precommit_timeout=" << timeout_precommit
              << " receipt_version=" << static_cast<unsigned>(receipt_format_version)
              << " ft_version=" << ft_version
              << '\n';
  const auto diagnostic_start = std::chrono::steady_clock::now();
  ScoreToMaskV3Session session;
  if (RAND_bytes(session.session.data(), 16) != 1 ||
      RAND_bytes(session.invocation.data(), 16) != 1)
    throw std::runtime_error("test session CSPRNG failed");
  session.session[0] |= 1U;
  session.invocation[0] |= 1U;
  session.n = static_cast<std::uint32_t>(scores.size());
  session.k = k;
  session.party_identity_sha256[0] =
      score_to_mask_v3_certificate_sha256(identities.p0_tls.certificate);
  session.party_identity_sha256[1] =
      score_to_mask_v3_certificate_sha256(identities.p1_tls.certificate);
  session.recipient_rsa_public_sha256[0] =
      score_to_mask_v3_public_key_sha256(identities.p0_recipient.public_key);
  session.recipient_rsa_public_sha256[1] =
      score_to_mask_v3_public_key_sha256(identities.p1_recipient.public_key);
  session.escrow_identity_sha256 =
      score_to_mask_v3_certificate_sha256(identities.escrow_tls.certificate);
  const auto session_path = (std::filesystem::path(directory) / "session.bin").string();
  const auto p0_package = (std::filesystem::path(directory) / "p0.v3pkg").string();
  const auto p1_package = (std::filesystem::path(directory) / "p1.v3pkg").string();
  const auto context_path = (std::filesystem::path(directory) / "escrow.context").string();
  const auto issuance_registry =
      (std::filesystem::path(directory) / "dealer-issuance-registry").string();
  if (ft_version == 2 || ft_version == 3) {
    std::filesystem::create_directory(issuance_registry);
    if (::chmod(issuance_registry.c_str(), 0700) != 0)
      throw std::runtime_error("F_T/v2 issuance registry permissions failed");
  }
  write_session_file(session_path, session);
  const auto dealer_stdout = (std::filesystem::path(directory) / "dealer.stdout").string();
  const auto dealer_stderr = (std::filesystem::path(directory) / "dealer.stderr").string();
  std::vector<std::string> dealer_arguments = {"--dealer", session_path, p0_package,
      p1_package, context_path, identities.dealer_private,
      timeout_precommit ? "2000" : "15000",
      std::to_string(static_cast<unsigned>(receipt_format_version)),
      std::to_string(ft_version)};
  if (ft_version == 2 || ft_version == 3) dealer_arguments.push_back(issuance_registry);
  ChildProcess dealer(exec_child(binary, dealer_arguments,
      dealer_stdout, dealer_stderr));
  const int dealer_status = dealer.wait();
  require(dealer_status == 0, "T process failed to issue bound packages");
  const auto package_info = score_to_mask_v3_validate_package(
      p0_package, identities.dealer_public, session, 0);
  const auto package_info_p1 = score_to_mask_v3_validate_package(
      p1_package, identities.dealer_public, session, 1);
#if defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL) || defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL)
  require(package_info.ft_version == ft_version &&
              package_info_p1.ft_version == ft_version,
          "Dealer emitted a package for the wrong F_T version");
#else
  require(ft_version == 1, "F_T/v2/v3 E2E requires its explicit build opt-in");
#endif

  std::vector<std::uint32_t> share0(scores.size()), share1(scores.size());
  if (RAND_bytes(reinterpret_cast<std::uint8_t*>(share0.data()),
                 static_cast<int>(share0.size() * sizeof(std::uint32_t))) != 1)
    throw std::runtime_error("test additive share CSPRNG failed");
  for (std::size_t i = 0; i < scores.size(); ++i)
    share1[i] = static_cast<std::uint32_t>(scores[i]) - share0[i];
  const auto p0_input = (std::filesystem::path(directory) / "p0.input").string();
  const auto p1_input = (std::filesystem::path(directory) / "p1.input").string();
  write_input_share(p0_input, share0);
  write_input_share(p1_input, share1);
  const auto lease0 = (std::filesystem::path(directory) / "lease-p0").string();
  const auto lease1 = (std::filesystem::path(directory) / "lease-p1").string();
  const auto p0_output = (std::filesystem::path(directory) / "p0.output").string();
  const auto p1_output = (std::filesystem::path(directory) / "p1.output").string();
  const auto p0_stdout = (std::filesystem::path(directory) / "p0.stdout").string();
  const auto p0_stderr = (std::filesystem::path(directory) / "p0.stderr").string();
  const auto p1_stdout = (std::filesystem::path(directory) / "p1.stdout").string();
  const auto p1_stderr = (std::filesystem::path(directory) / "p1.stderr").string();
  const auto sandbox = prepare_escrow_sandbox(directory, p0_package, p1_package,
                                               context_path, identities);
  const auto db_path = sandbox.database;
  const auto port0 = reserve_port();
  const auto port1 = reserve_port();
  const auto service_stdout = (std::filesystem::path(directory) / "escrow.stdout").string();
  const auto service_stderr = (std::filesystem::path(directory) / "escrow.stderr").string();
  ChildProcess service(exec_child(binary, {"--service", sandbox.context,
      sandbox.dealer_public, std::to_string(port0), std::to_string(port1), db_path,
      sandbox.ca_certificate, sandbox.tls_certificate, sandbox.tls_private_key},
      service_stdout, service_stderr, sandbox.run_as_nobody));
  wait_port_ready(port0);
  wait_port_ready(port1);
  verify_escrow_process_identity(service, sandbox, p0_package, p1_package);

  int channels[2] = {-1, -1};
  if (::socketpair(AF_UNIX, SOCK_STREAM, 0, channels) != 0)
    throw std::runtime_error("party socketpair failed");
  ChildProcess p0_process(exec_child(binary, {"--party", "0", std::to_string(channels[0]),
      std::to_string(channels[1]), p0_package, identities.dealer_public,
      session_path, p0_input, lease0, identities.p0_recipient.public_key,
      identities.ca_certificate, identities.p0_tls.certificate,
      identities.p0_tls.private_key, std::to_string(port0), p0_output,
      lose_receipt_p0 ? "drop-receipt-after-commit" : "reserved",
      "reserved", "reserved", "reserved"}, p0_stdout, p0_stderr));
  ::close(channels[0]);
  channels[0] = -1;
  ChildProcess p1_process(exec_child(binary, {"--party", "1", std::to_string(channels[1]),
      "-1", p1_package, identities.dealer_public, session_path, p1_input,
      lease1, identities.p1_recipient.public_key, identities.ca_certificate,
      identities.p1_tls.certificate, identities.p1_tls.private_key,
      std::to_string(port1), p1_output,
      (fail_p1_before_submit || timeout_precommit)
          ? "p1-fail-before-submit" : "reserved",
      "reserved", "reserved", "reserved"}, p1_stdout, p1_stderr));
  ::close(channels[1]);
  channels[1] = -1;
  long p0_rss_kib = 0;
  long p1_rss_kib = 0;
  int p0_status = 0;
  int p1_status = 0;
  int service_status = -1;
  if (fail_p1_before_submit || timeout_precommit) {
    p1_status = p1_process.wait(&p1_rss_kib);
    wait_for_one_precommit_ciphertext(db_path);
    if (fail_p1_before_submit) {
      service.signal(SIGKILL);
      service_status = service.wait();
      require(service_status >= 128, "pre-commit escrow crash failpoint failed");
    }
    p0_status = p0_process.wait(&p0_rss_kib);
  } else {
    p0_status = p0_process.wait(&p0_rss_kib);
    p1_status = p1_process.wait(&p1_rss_kib);
  }
  if (p0_status != (lose_receipt_p0 || fail_p1_before_submit || timeout_precommit ? 11 : 0) ||
      p1_status != (fail_p1_before_submit || timeout_precommit ? 11 : 0)) {
    throw std::runtime_error("party E2E failed: P0=" + std::to_string(p0_status) +
                             " P1=" + std::to_string(p1_status));
  }
  auto output0 = read_process_output(p0_output);
  auto output1 = read_process_output(p1_output);
  if (lose_receipt_p0 || fail_p1_before_submit || timeout_precommit) {
    require(output0.state == static_cast<std::uint8_t>(
                ScoreToMaskV3PartyOutput::State::kRecoveryRequired) &&
                output0.status_code == "COMMIT_STATUS_UNKNOWN" &&
                output0.share.empty(),
            "lost receipt did not yield no-share recovery-required state");
    if (fail_p1_before_submit || timeout_precommit) {
      require(output1.state == static_cast<std::uint8_t>(
                  ScoreToMaskV3PartyOutput::State::kRecoveryRequired) &&
                  output1.share.empty(),
              "pre-submit failure exposed a P1 API share");
      require(std::filesystem::exists(lease0) && std::filesystem::exists(lease1),
              "pre-commit escrow crash did not burn both local package leases");
    } else {
      require(output1.state == static_cast<std::uint8_t>(
                  ScoreToMaskV3PartyOutput::State::kCommitted),
              "peer did not receive the committed API share");
    }
  } else {
    require(output0.state == static_cast<std::uint8_t>(
                ScoreToMaskV3PartyOutput::State::kCommitted) &&
                output1.state == static_cast<std::uint8_t>(
                ScoreToMaskV3PartyOutput::State::kCommitted),
            "party APIs did not return committed shares");
  }
  require(output0.transaction == output1.transaction,
          "parties received different escrow transaction ids");
  if (!lose_receipt_p0 && !fail_p1_before_submit)
    require(output0.receipt == output1.receipt,
            "parties received different durable escrow commit receipts");
  if (receipt_format_version == 2 &&
      !fail_p1_before_submit && !timeout_precommit) {
    const auto& delivered_token = lose_receipt_p0 ? output1.receipt : output0.receipt;
    require(delivered_token.size() == 64 &&
                std::all_of(delivered_token.begin(), delivered_token.end(),
                    [](char c) { return (c >= '0' && c <= '9') ||
                                        (c >= 'a' && c <= 'f'); }),
            "receipt/v2 did not return a fixed 32-byte opaque token");
  }
  require(output0.phases == output1.phases && output0.phases <=
              (ft_version == 3 ? 106U :
                  test_gate_budget(session.n, ft_version).key_bits + 51U),
          "observed party exchange phases exceed the S31 DAG bound");
  require(output0.max_payload_sent <= kScoreToMaskV3MaxGateBatchFrameBytes &&
              output1.max_payload_sent <= kScoreToMaskV3MaxGateBatchFrameBytes &&
              output0.max_payload_received <= kScoreToMaskV3MaxGateBatchFrameBytes &&
              output1.max_payload_received <= kScoreToMaskV3MaxGateBatchFrameBytes,
          "observed v3 frame exceeds S31 one-MiB limit");
  require(((lose_receipt_p0 || fail_p1_before_submit || timeout_precommit) ? output0.share.empty()
                            : output0.share.size() == scores.size()) &&
              ((fail_p1_before_submit || timeout_precommit) ? output1.share.empty()
                                     : output1.share.size() == scores.size()),
          "party output share length mismatch");
  std::vector<std::uint32_t> raw_scores(scores.size());
  std::transform(scores.begin(), scores.end(), raw_scores.begin(),
      [](std::int32_t value) { return static_cast<std::uint32_t>(value); });
  const auto oracle = top_k_mask(raw_scores, k);
  if (!lose_receipt_p0 && !fail_p1_before_submit && !timeout_precommit) {
    std::size_t selected_count = 0;
    for (std::size_t i = 0; i < scores.size(); ++i)
      {
        const auto recovered = static_cast<std::uint8_t>(
            output0.share[i] ^ output1.share[i]);
        require(recovered == oracle[i],
                "two-party v3 share XOR differs from stable original-order oracle");
        selected_count += recovered;
      }
    require(selected_count == k,
            "two-party recovered mask does not contain exactly K ones");
  }

  if (!fail_p1_before_submit) {
    service.signal(SIGTERM);
    service_status = service.wait();
    require(service_status >= 128, "escrow service did not stop on SIGTERM");
  }
  ChildProcess restart_service(exec_child(binary, {"--service", sandbox.context,
      sandbox.dealer_public, std::to_string(port0), std::to_string(port1), db_path,
      sandbox.ca_certificate, sandbox.tls_certificate, sandbox.tls_private_key},
      (std::filesystem::path(directory) / "escrow-restart.stdout").string(),
      (std::filesystem::path(directory) / "escrow-restart.stderr").string(),
      sandbox.run_as_nobody));
  wait_port_ready(port0);
  wait_port_ready(port1);
  ScoreToMaskV3EscrowClientConfig recovery0{"127.0.0.1", port0, "escrow",
      {identities.ca_certificate, identities.p0_tls.certificate,
       identities.p0_tls.private_key}};
  ScoreToMaskV3EscrowClientConfig recovery1{"127.0.0.1", port1, "escrow",
      {identities.ca_certificate, identities.p1_tls.certificate,
       identities.p1_tls.private_key}};
  if (fail_p1_before_submit || timeout_precommit) {
    require(has_aborted_precommit_record(db_path, output0.transaction),
            "A restart did not durably abort the staged one-share transaction");
    bool p0_fetch_rejected = false;
    bool p1_fetch_rejected = false;
    try { (void)score_to_mask_v3_escrow_fetch(recovery0, package_info, 0,
                                              output0.transaction); }
    catch (const std::exception&) { p0_fetch_rejected = true; }
    try { (void)score_to_mask_v3_escrow_fetch(recovery1, package_info_p1, 1,
                                              output1.transaction); }
    catch (const std::exception&) { p1_fetch_rejected = true; }
    require(p0_fetch_rejected && p1_fetch_rejected,
            "pre-commit abort incorrectly allowed a recipient fetch");
  } else {
    const auto recovered_cipher0 = score_to_mask_v3_escrow_fetch(
        recovery0, package_info, 0, output0.transaction);
    const auto recovered_cipher1 = score_to_mask_v3_escrow_fetch(
        recovery1, package_info_p1, 1, output1.transaction);
    require(recovered_cipher0.commit_receipt ==
                recovered_cipher1.commit_receipt &&
                recovered_cipher1.commit_receipt == output1.receipt,
            "escrow recovery did not return the common durable commit receipt");
    const auto recovered0 = score_to_mask_v3_decrypt_recipient_share(
        recovered_cipher0.recipient_ciphertext,
        identities.p0_recipient.private_key, package_info, 0);
    const auto recovered1 = score_to_mask_v3_decrypt_recipient_share(
        recovered_cipher1.recipient_ciphertext,
        identities.p1_recipient.private_key, package_info_p1, 1);
    require((lose_receipt_p0 ? recovered0.size() == scores.size()
                             : recovered0 == output0.share) &&
                recovered1 == output1.share,
            "post-restart recipient-only escrow recovery differs from API shares");
    const auto replayed_cipher0 = score_to_mask_v3_escrow_fetch(
        recovery0, package_info, 0, output0.transaction);
    const auto replayed_cipher1 = score_to_mask_v3_escrow_fetch(
        recovery1, package_info_p1, 1, output1.transaction);
    require(replayed_cipher0.recipient_ciphertext ==
                recovered_cipher0.recipient_ciphertext &&
                replayed_cipher1.recipient_ciphertext ==
                recovered_cipher1.recipient_ciphertext &&
                replayed_cipher0.commit_receipt ==
                recovered_cipher0.commit_receipt &&
                replayed_cipher1.commit_receipt ==
                recovered_cipher1.commit_receipt,
            "repeated post-restart fetch changed ciphertext or receipt");
    if (receipt_format_version == 2) {
      const auto duplicate_receipt = score_to_mask_v3_escrow_submit(
          recovery0, package_info, 0, output0.transaction,
          recovered_cipher0.recipient_ciphertext);
      require(duplicate_receipt == recovered_cipher0.commit_receipt,
              "duplicate submit did not reuse the committed receipt");
      auto changed_ciphertext = recovered_cipher0.recipient_ciphertext;
      require(!changed_ciphertext.empty(), "fetched ciphertext unexpectedly empty");
      changed_ciphertext.back() ^= 1U;
      bool changed_duplicate_rejected = false;
      try {
        (void)score_to_mask_v3_escrow_submit(
            recovery0, package_info, 0, output0.transaction, changed_ciphertext);
      } catch (const std::exception&) {
        changed_duplicate_rejected = true;
      }
      require(changed_duplicate_rejected,
              "duplicate submit with changed ciphertext was accepted");
    }
    auto wrong_identity = recovery1;
    wrong_identity.tls_credentials = {identities.ca_certificate,
        identities.p0_tls.certificate, identities.p0_tls.private_key};
    bool wrong_identity_rejected = false;
    try {
      (void)score_to_mask_v3_escrow_fetch(
          wrong_identity, package_info_p1, 1, output1.transaction);
    } catch (const std::exception&) {
      wrong_identity_rejected = true;
    }
    require(wrong_identity_rejected,
            "escrow recovery accepted P0 TLS identity on P1 recipient endpoint");
    if (scores.size() == 1 && k == 1) {
      run_escrow_same_san_wrong_pin_probe(
          directory, identities, package_info, context_path,
          identities.dealer_public, output0.transaction);
      std::cout << "S35_ESCROW_CA_SAME_SAN_WRONG_PIN PASS\n";
    }
    if (lose_receipt_p0) {
      for (std::size_t i = 0; i < scores.size(); ++i)
        require(static_cast<std::uint8_t>(recovered0[i] ^ recovered1[i]) == oracle[i],
                "recovered shares after lost receipt differ from the frozen oracle");
    }
  }
  restart_service.signal(SIGTERM);
  const int restart_service_status = restart_service.wait();
  require(restart_service_status >= 128, "restarted escrow service did not stop");
  const auto diagnostic_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - diagnostic_start).count();
  const auto budget = test_gate_budget(session.n, ft_version);
  const auto material_bytes = (3ULL * budget.gate_count + 7U) / 8U +
      (static_cast<std::uint64_t>(budget.padded_n) * budget.padded_n + 7U) / 8U;
  std::cout << (ft_version == 3 ? "S47_FT_V3_PROCESS_E2E {" :
                (ft_version == 2 ? "S45_FT_V2_PROCESS_E2E {" :
                                   "S40_V3_PROCESS_E2E {"))
            << "\"case_id\":\"" << case_id
            << "\",\"ft_version\":" << ft_version
            << ",\"receipt_format_version\":"
            << static_cast<unsigned>(receipt_format_version)
            << ",\"dealer_exit\":" << dealer_status
            << ",\"p0_exit\":" << p0_status
            << ",\"p1_exit\":" << p1_status
            << ",\"service_exit\":" << service_status
            << ",\"restart_service_exit\":" << restart_service_status
            << ",\"log_dir\":\"" << directory
            << "\",\"session\":\""
            << bytes_hex(session.session.data(), session.session.size())
            << "\",\"invocation\":\""
            << bytes_hex(session.invocation.data(), session.invocation.size())
            << "\",\"package_sha256_p0\":\""
            << bytes_hex(package_info.package_sha256.data(), 32)
            << "\",\"package_sha256_p1\":\""
            << bytes_hex(package_info_p1.package_sha256.data(), 32)
            << "\",\"N\":" << scores.size()
            << ",\"K\":" << k << ",\"scores\":[";
  for (std::size_t i = 0; i < scores.size(); ++i) {
    if (i != 0) std::cout << ',';
    std::cout << scores[i];
  }
  std::cout << "],\"oracle\":\""
            << ((fail_p1_before_submit || timeout_precommit)
                    ? "NOT_APPLICABLE_NO_SHARE" : "PASS")
            << "\",\"party_api\":\""
            << ((fail_p1_before_submit || timeout_precommit)
                ? "BOTH_RECOVERY_REQUIRED_NO_SHARE"
                : (lose_receipt_p0 ? "P0_RECOVERY_REQUIRED_P1_COMMITTED"
                                   : "BOTH_COMMITTED"))
            << "\",\"escrow_post_restart_recovery\":\""
            << ((fail_p1_before_submit || timeout_precommit)
                    ? "ABORTED_NO_FETCH" : "PASS") << "\","
            << "\"gate_count\":" << budget.gate_count
            << ",\"material_payload_bytes_per_party\":" << material_bytes
            << ",\"package_format_version\":" << test_package_format(package_info)
            << ",\"context_format_version\":" <<
               score_to_mask_v3_validate_escrow_context(
                   sandbox.context, sandbox.dealer_public).format_version
            << ",\"package_metadata_bytes_including_signature\":"
            << (package_info.package_bytes - material_bytes)
            << ",\"package_bytes_p0\":" << std::filesystem::file_size(p0_package)
            << ",\"package_bytes_p1\":" << std::filesystem::file_size(p1_package)
            << ",\"party_exchange_phases\":" << output0.phases
            << ",\"party_exchange_phases_bound\":" <<
               (ft_version == 3 ? 106U : 255U)
            << ",\"party_frames_each_direction\":" << output0.sent_frames
            << ",\"party_bytes_sent_p0\":" << output0.sent_bytes
            << ",\"party_bytes_sent_p1\":" << output1.sent_bytes
            << ",\"max_payload_bytes\":" << std::max({output0.max_payload_sent,
                  output0.max_payload_received, output1.max_payload_sent,
                  output1.max_payload_received})
            << ",\"p0_peak_rss_kib\":" << p0_rss_kib
            << ",\"p1_peak_rss_kib\":" << p1_rss_kib
            << ",\"peak_party_rss_kib\":" << std::max(p0_rss_kib, p1_rss_kib)
            << ",\"aggregate_party_peak_rss_kib\":" << (p0_rss_kib + p1_rss_kib)
            << ",\"diagnostic_elapsed_ms\":" << diagnostic_ms
            << ",\"injected_lost_receipt_after_commit\":"
            << (lose_receipt_p0 ? "true" : "false")
            << ",\"injected_precommit_escrow_crash\":"
            << (fail_p1_before_submit ? "true" : "false")
            << ",\"injected_precommit_timeout\":"
            << (timeout_precommit ? "true" : "false") << "}\n";
}

#if defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL) && \
    defined(MOE_TOPK_SCORE_TO_MASK_V3_FT_V2_TESTING)
void run_ft_v2_issuance_abort_process_case(
    const std::string& binary, const std::string& root,
    const TestIdentities& identities) {
  const auto dir = std::filesystem::path(root) / "ft-v2-issuance-abort";
  std::filesystem::create_directory(dir);
  ScoreToMaskV3Session session;
  if (RAND_bytes(session.session.data(), 16) != 1 ||
      RAND_bytes(session.invocation.data(), 16) != 1)
    throw std::runtime_error("F_T/v2 abort test session CSPRNG failed");
  session.session[0] |= 1U;
  session.invocation[0] |= 1U;
  session.n = 256;
  session.k = 128;
  session.party_identity_sha256[0] =
      score_to_mask_v3_certificate_sha256(identities.p0_tls.certificate);
  session.party_identity_sha256[1] =
      score_to_mask_v3_certificate_sha256(identities.p1_tls.certificate);
  session.recipient_rsa_public_sha256[0] =
      score_to_mask_v3_public_key_sha256(identities.p0_recipient.public_key);
  session.recipient_rsa_public_sha256[1] =
      score_to_mask_v3_public_key_sha256(identities.p1_recipient.public_key);
  session.escrow_identity_sha256 =
      score_to_mask_v3_certificate_sha256(identities.escrow_tls.certificate);

  const auto session_path = (dir / "session.bin").string();
  const auto p0_package = (dir / "p0.pkg").string();
  const auto p1_package = (dir / "p1.pkg").string();
  const auto context_path = (dir / "escrow.context").string();
  const auto lease_path = (dir / "lease-p0").string();
  const auto issuance_registry = (dir / "dealer-issuance-registry").string();
  std::filesystem::create_directory(issuance_registry);
  if (::chmod(issuance_registry.c_str(), 0700) != 0)
    throw std::runtime_error("F_T/v2 abort registry permissions failed");
  write_session_file(session_path, session);
  const auto dealer_stdout = (dir / "dealer.stdout").string();
  const auto dealer_stderr = (dir / "dealer.stderr").string();
  ChildProcess dealer(exec_child(binary, {"--dealer", session_path, p0_package,
      p1_package, context_path, identities.dealer_private, "15000", "1", "2",
      issuance_registry,
      "test-only-force-bounded-abort"}, dealer_stdout, dealer_stderr));
  const int status = dealer.wait();
  require(status != 0, "forced F_T/v2 rejection exhaustion did not abort T process");
  std::ifstream error_file(dealer_stderr, std::ios::binary);
  const std::string error((std::istreambuf_iterator<char>(error_file)),
                          std::istreambuf_iterator<char>());
  require(error.find("ISSUANCE_ABORT") != std::string::npos,
          "Dealer process did not report the unified issuance abort");
  require(!std::filesystem::exists(p0_package) &&
              !std::filesystem::exists(p1_package) &&
              !std::filesystem::exists(context_path) &&
              !std::filesystem::exists(lease_path),
          "process-level ISSUANCE_ABORT left a package/context/lease artifact");
  require(std::filesystem::directory_iterator(issuance_registry) !=
              std::filesystem::directory_iterator{},
          "process-level ISSUANCE_ABORT did not persist its invocation burn record");
  std::cout << "S45_FT_V2_ISSUANCE_ABORT {\"dealer_exit\":" << status
            << ",\"N\":256,\"K\":128,\"receipt_format_version\":1"
            << ",\"ft_version\":2,\"package_artifacts\":false"
            << ",\"context_artifact\":false,\"lease_artifact\":false}\n";
}
#endif

enum class CommunicationFault {
  kNoFault,
  kReadyMismatch,
  kDuplicateReady,
  kKillP1AfterOpening
};

const char* communication_fault_name(CommunicationFault fault) {
  switch (fault) {
    case CommunicationFault::kNoFault: return "normal-path-trace";
    case CommunicationFault::kReadyMismatch: return "ready-mismatch";
    case CommunicationFault::kDuplicateReady: return "duplicate-ready";
    case CommunicationFault::kKillP1AfterOpening: return "kill-p1-after-opening";
  }
  return "unknown";
}

void run_communication_fault_case(const std::string& binary,
                                  const std::string& root,
                                  const TestIdentities& identities,
                                  CommunicationFault fault,
                                  const std::string& evidence_logs_root,
                                  const std::vector<std::int32_t>& scores = {7, 7, -3},
                                  std::uint32_t k = 2,
                                  std::uint8_t receipt_format_version = 1) {
  const std::string name = communication_fault_name(fault);
  const auto directory = (std::filesystem::path(root) / name).string();
  std::filesystem::create_directory(directory);
  const auto logs = (std::filesystem::path(evidence_logs_root) / name).string();
  std::filesystem::create_directories(logs);

  ScoreToMaskV3Session session;
  if (RAND_bytes(session.session.data(), 16) != 1 ||
      RAND_bytes(session.invocation.data(), 16) != 1)
    throw std::runtime_error("fault test session CSPRNG failed");
  session.session[0] |= 1U;
  session.invocation[0] |= 1U;
  session.n = static_cast<std::uint32_t>(scores.size());
  session.k = k;
  session.party_identity_sha256[0] =
      score_to_mask_v3_certificate_sha256(identities.p0_tls.certificate);
  session.party_identity_sha256[1] =
      score_to_mask_v3_certificate_sha256(identities.p1_tls.certificate);
  session.recipient_rsa_public_sha256[0] =
      score_to_mask_v3_public_key_sha256(identities.p0_recipient.public_key);
  session.recipient_rsa_public_sha256[1] =
      score_to_mask_v3_public_key_sha256(identities.p1_recipient.public_key);
  session.escrow_identity_sha256 =
      score_to_mask_v3_certificate_sha256(identities.escrow_tls.certificate);

  const auto session_path = (std::filesystem::path(directory) / "session.bin").string();
  const auto p0_package = (std::filesystem::path(directory) / "p0.v3pkg").string();
  const auto p1_package = (std::filesystem::path(directory) / "p1.v3pkg").string();
  const auto context_path = (std::filesystem::path(directory) / "escrow.context").string();
  write_session_file(session_path, session);
  const auto dealer_stdout = (std::filesystem::path(logs) / "dealer.stdout").string();
  const auto dealer_stderr = (std::filesystem::path(logs) / "dealer.stderr").string();
  ChildProcess dealer(exec_child(binary, {"--dealer", session_path, p0_package,
      p1_package, context_path, identities.dealer_private, "15000",
      std::to_string(static_cast<unsigned>(receipt_format_version)), "1"},
      dealer_stdout, dealer_stderr));
  const int dealer_status = dealer.wait();
  require(dealer_status == 0, "fault test T process failed");
  const auto info0 = score_to_mask_v3_validate_package(
      p0_package, identities.dealer_public, session, 0);
  const auto info1 = score_to_mask_v3_validate_package(
      p1_package, identities.dealer_public, session, 1);

  std::vector<std::uint32_t> share0(scores.size()), share1(scores.size());
  if (RAND_bytes(reinterpret_cast<std::uint8_t*>(share0.data()),
                 static_cast<int>(share0.size() * sizeof(std::uint32_t))) != 1)
    throw std::runtime_error("fault test input share CSPRNG failed");
  for (std::size_t i = 0; i < scores.size(); ++i)
    share1[i] = static_cast<std::uint32_t>(scores[i]) - share0[i];
  const auto p0_input = (std::filesystem::path(directory) / "p0.input").string();
  const auto p1_input = (std::filesystem::path(directory) / "p1.input").string();
  write_input_share(p0_input, share0);
  write_input_share(p1_input, share1);
  const auto lease0 = (std::filesystem::path(directory) / "lease-p0").string();
  const auto lease1 = (std::filesystem::path(directory) / "lease-p1").string();
  const auto p0_output = (std::filesystem::path(directory) / "p0.output").string();
  const auto p1_output = (std::filesystem::path(directory) / "p1.output").string();
  const auto sandbox = prepare_escrow_sandbox(directory, p0_package, p1_package,
                                               context_path, identities);
  const auto db_path = sandbox.database;
  const auto port0 = reserve_port();
  const auto port1 = reserve_port();
  ChildProcess service(exec_child(binary, {"--service", sandbox.context,
      sandbox.dealer_public, std::to_string(port0), std::to_string(port1), db_path,
      sandbox.ca_certificate, sandbox.tls_certificate, sandbox.tls_private_key},
      (std::filesystem::path(logs) / "escrow.stdout").string(),
      (std::filesystem::path(logs) / "escrow.stderr").string(),
      sandbox.run_as_nobody));
  wait_port_ready(port0);
  wait_port_ready(port1);
  verify_escrow_process_identity(service, sandbox, p0_package, p1_package);

  int p0_pair[2] = {-1, -1};
  if (::socketpair(AF_UNIX, SOCK_STREAM, 0, p0_pair) != 0)
    throw std::runtime_error("fault proxy P0 socketpair failed");
  const auto p0_log_out = (std::filesystem::path(logs) / "p0.stdout").string();
  const auto p0_log_err = (std::filesystem::path(logs) / "p0.stderr").string();
  const auto p1_log_out = (std::filesystem::path(logs) / "p1.stdout").string();
  const auto p1_log_err = (std::filesystem::path(logs) / "p1.stderr").string();
  ChildProcess p0_process(exec_child(binary, {"--party", "0",
      std::to_string(p0_pair[0]), std::to_string(p0_pair[1]), p0_package,
      identities.dealer_public, session_path, p0_input, lease0,
      identities.p0_recipient.public_key, identities.ca_certificate,
      identities.p0_tls.certificate, identities.p0_tls.private_key,
      std::to_string(port0), p0_output, "reserved", "reserved", "reserved",
      "reserved"}, p0_log_out, p0_log_err));
  ::close(p0_pair[0]);
  p0_pair[0] = -1;
  int p1_pair[2] = {-1, -1};
  if (::socketpair(AF_UNIX, SOCK_STREAM, 0, p1_pair) != 0)
    throw std::runtime_error("fault proxy P1 socketpair failed");
  ChildProcess p1_process(exec_child(binary, {"--party", "1",
      std::to_string(p1_pair[0]), std::to_string(p1_pair[1]), p1_package,
      identities.dealer_public, session_path, p1_input, lease1,
      identities.p1_recipient.public_key, identities.ca_certificate,
      identities.p1_tls.certificate, identities.p1_tls.private_key,
      std::to_string(port1), p1_output, "reserved", "reserved", "reserved",
      "reserved", std::to_string(p0_pair[1])}, p1_log_out, p1_log_err));
  ::close(p1_pair[0]);
  p1_pair[0] = -1;

  bool injected = false;
  std::string proxy_error;
  std::thread proxy([&] {
    std::ofstream trace((std::filesystem::path(logs) / "proxy.frames.log").string(),
                        std::ios::trunc);
    try {
      auto p0_tls = protocol_i_tls13_context_create(
          {identities.ca_certificate, identities.p1_tls.certificate,
           identities.p1_tls.private_key});
      auto p1_tls = protocol_i_tls13_context_create(
          {identities.ca_certificate, identities.p0_tls.certificate,
           identities.p0_tls.private_key});
      {
        auto endpoint0 = protocol_i_tls13_wrap_fd(p0_pair[1], p0_tls, false, "p0");
        auto endpoint1 = protocol_i_tls13_wrap_fd(p1_pair[1], p1_tls, true, "p1");
        ProtocolIFramedChannel to_p0(endpoint0, score_to_mask_v3_frame_config(info1, 1),
                                     ProtocolIFramedChannelOptions{15000, 4096});
        ProtocolIFramedChannel to_p1(endpoint1, score_to_mask_v3_frame_config(info0, 0),
                                     ProtocolIFramedChannelOptions{15000, 4096});
        to_p0.complete_authenticated_handshake();
        to_p1.complete_authenticated_handshake();
        require(to_p0.peer_certificate_sha256() == session.party_identity_sha256[0] &&
                    to_p1.peer_certificate_sha256() == session.party_identity_sha256[1],
                "fault proxy TLS peer certificate binding mismatch");

        constexpr std::size_t kInnerBodyOffset = 104;
        constexpr std::uint8_t kReadyTag = 1;
        constexpr std::uint8_t kBeaverTag = 2;
        constexpr std::uint8_t kAbortTag = 255;
        for (std::uint8_t phase = 1; phase <= 255; ++phase) {
          to_p0.set_phase(phase);
          to_p1.set_phase(phase);
          auto from_p0 = to_p0.receive();
          require(from_p0.size() > kInnerBodyOffset,
                  "fault proxy received a truncated P0 inner frame");
          trace << "P0->proxy phase=" << static_cast<unsigned>(phase)
                << " seq=" << (to_p0.received_frames() - 1U)
                << " tag=" << static_cast<unsigned>(from_p0[7])
                << " read=OK payload_bytes=" << from_p0.size() << '\n';
          to_p1.send(from_p0);
          trace << "proxy->P1 phase=" << static_cast<unsigned>(phase)
                << " seq=" << (to_p1.sent_frames() - 1U)
                << " tag=" << static_cast<unsigned>(from_p0[7])
                << " write=OK payload_bytes=" << from_p0.size() << '\n';
          if (fault == CommunicationFault::kDuplicateReady && phase == 1) {
            to_p1.send(from_p0);
            trace << "INJECT duplicate inner READY phase=1 outer-seq="
                  << (to_p1.sent_frames() - 1U) << '\n';
            injected = true;
          }

          auto from_p1 = to_p1.receive();
          require(from_p1.size() > kInnerBodyOffset,
                  "fault proxy received a truncated P1 inner frame");
          trace << "P1->proxy phase=" << static_cast<unsigned>(phase)
                << " seq=" << (to_p1.received_frames() - 1U)
                << " tag=" << static_cast<unsigned>(from_p1[7])
                << " read=OK payload_bytes=" << from_p1.size() << '\n';
          if (fault == CommunicationFault::kReadyMismatch && phase == 1 &&
              from_p1[7] == kReadyTag) {
            from_p1[kInnerBodyOffset] = 0;
            trace << "INJECT READY body byte 1->0 phase=1\n";
            injected = true;
          }
          if (fault == CommunicationFault::kKillP1AfterOpening && phase == 2 &&
              from_p1[7] == kBeaverTag) {
            if (::kill(p1_process.pid(), SIGKILL) != 0)
              throw std::runtime_error("fault proxy could not SIGKILL P1");
            trace << "INJECT SIGKILL P1 after Beaver response phase=2 before forwarding\n";
            injected = true;
            break;
          }
          to_p0.send(from_p1);
          trace << "proxy->P0 phase=" << static_cast<unsigned>(phase)
                << " seq=" << (to_p0.sent_frames() - 1U)
                << " tag=" << static_cast<unsigned>(from_p1[7])
                << " write=OK payload_bytes=" << from_p1.size() << '\n';
          if ((fault == CommunicationFault::kReadyMismatch ||
               fault == CommunicationFault::kDuplicateReady) && phase == 1)
            break;
          if (from_p0[7] == kAbortTag || from_p1[7] == kAbortTag) break;
        }
      }
      protocol_i_tls13_context_free(p0_tls);
      protocol_i_tls13_context_free(p1_tls);
    } catch (const std::exception& error) {
      proxy_error = error.what();
      trace << "PROXY_EXCEPTION " << proxy_error << '\n';
    }
  });
  proxy.join();

  const auto p0_status = p0_process.wait();
  const auto p1_status = p1_process.wait();
  const bool normal_path = fault == CommunicationFault::kNoFault;
  if (p0_status != (normal_path ? 0 : 10) ||
      (fault == CommunicationFault::kKillP1AfterOpening
           ? p1_status != 137 : p1_status != (normal_path ? 0 : 10))) {
    throw std::runtime_error(name + " unexpected party process outcomes P0=" +
        std::to_string(p0_status) + " P1=" + std::to_string(p1_status) +
        " proxy=" + proxy_error);
  }
  if (!normal_path)
    require(injected, "fault proxy did not reach the requested injection point");
  require(std::filesystem::exists(lease0) && std::filesystem::exists(lease1),
          "communication fault did not burn both one-shot leases");
  const auto p0_output_exists = std::filesystem::exists(p0_output);
  const auto p1_output_exists = std::filesystem::exists(p1_output);
  require(p0_output_exists, "P0 did not publish its API result record");
  const auto output0 = read_process_output(p0_output);
  if (normal_path) {
    require(output0.state == static_cast<std::uint8_t>(
                ScoreToMaskV3PartyOutput::State::kCommitted) &&
                output0.share.size() == scores.size(),
            "P0 normal trace did not return its committed share");
  } else {
    require(output0.state == static_cast<std::uint8_t>(
                ScoreToMaskV3PartyOutput::State::kAborted) && output0.share.empty(),
            "P0 API unexpectedly returned a share after injected communication fault");
  }
  std::string p0_api = normal_path ? "committed/share-returned" : "aborted/no-share";
  std::string p1_api = "process-killed-before-api-return";
  if (p1_status == 0 || p1_status == 10) {
    require(p1_output_exists, "P1 API result record missing after ordinary abort");
    const auto output1 = read_process_output(p1_output);
    if (normal_path) {
      require(output1.state == static_cast<std::uint8_t>(
                  ScoreToMaskV3PartyOutput::State::kCommitted) &&
                  output1.share.size() == scores.size(),
              "P1 normal trace did not return its committed share");
      p1_api = "committed/share-returned";
    } else {
      require(output1.state == static_cast<std::uint8_t>(
                  ScoreToMaskV3PartyOutput::State::kAborted) && output1.share.empty(),
              "P1 API unexpectedly returned a share after injected communication fault");
      p1_api = "aborted/no-share";
    }
  } else {
    require(!p1_output_exists, "killed P1 unexpectedly wrote an API result");
  }

  sqlite3* db = nullptr;
  require(sqlite3_open_v2(db_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) == SQLITE_OK,
          "fault controller could not open A database");
  sqlite3_stmt* stmt = nullptr;
  require(sqlite3_prepare_v2(db,
      "SELECT (SELECT count(*) FROM transactions), (SELECT count(*) FROM shares)",
      -1, &stmt, nullptr) == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW,
      "fault controller could not count A records");
  const int transaction_count = sqlite3_column_int(stmt, 0);
  const int share_count = sqlite3_column_int(stmt, 1);
  sqlite3_finalize(stmt);
  sqlite3_close(db);
  if (normal_path) {
    require(transaction_count == 1 && share_count == 2,
            "normal trace did not produce one committed A transaction with both shares");
    std::vector<std::uint32_t> normal_scores(scores.size());
    std::transform(scores.begin(), scores.end(), normal_scores.begin(),
        [](std::int32_t value) { return static_cast<std::uint32_t>(value); });
    const auto normal_oracle = top_k_mask(normal_scores, session.k);
    const auto output1 = read_process_output(p1_output);
    for (std::size_t i = 0; i < scores.size(); ++i)
      require(static_cast<std::uint8_t>(output0.share[i] ^
                                       output1.share[i]) == normal_oracle[i],
              "normal traced shares differ from the stable frozen oracle");
  } else {
    require(transaction_count == 0 && share_count == 0,
            "A stored escrow material before the communication fault resolved");
  }

  service.signal(SIGTERM);
  const int service_status = service.wait();
  require(service_status >= 128, "fault escrow service did not stop on SIGTERM");

  std::ofstream result((std::filesystem::path(logs) / "case-result.json").string(),
                       std::ios::trunc);
  result << "{\"case\":\"" << name << "\",\"receipt_format_version\":"
         << static_cast<unsigned>(receipt_format_version)
         << ",\"N\":" << scores.size() << ",\"K\":" << k
         << ",\"scores\":[";
  for (std::size_t i = 0; i < scores.size(); ++i) {
    if (i != 0) result << ',';
    result << scores[i];
  }
  result << "],\"session\":\""
         << bytes_hex(session.session.data(), session.session.size())
         << "\",\"package_sha256_p0\":\""
         << bytes_hex(info0.package_sha256.data(), info0.package_sha256.size())
         << "\",\"package_sha256_p1\":\""
         << bytes_hex(info1.package_sha256.data(), info1.package_sha256.size())
         << "\",\"injected\":"
         << (normal_path ? "false" : "true")
         << ",\"dealer_exit\":" << dealer_status
         << ",\"p0_exit\":" << p0_status << ",\"p0_api\":\"" << p0_api << "\""
         << ",\"p1_exit\":" << p1_status << ",\"p1_api\":\"" << p1_api << "\""
         << ",\"escrow_exit\":" << service_status
         << ",\"p0_lease_burned\":true,\"p1_lease_burned\":true"
         << ",\"a_transactions\":" << transaction_count
         << ",\"a_share_rows\":" << share_count
         << ",\"a_status\":\""
         << (normal_path ? "COMMITTED" : "NO_TRANSACTION") << "\""
         << ",\"transport_timeout_ms\":15000"
         << ",\"p0_exchange_phases\":" << output0.phases
         << ",\"p1_exchange_phases\":"
         << (p1_status == 137 ? 0 : read_process_output(p1_output).phases)
         << ",\"p0_bytes_sent\":" << output0.sent_bytes
         << ",\"p0_bytes_received\":" << output0.received_bytes
         << ",\"p0_frames_sent\":" << output0.sent_frames
         << ",\"p0_frames_received\":" << output0.received_frames
         << ",\"p0_max_payload_sent\":" << output0.max_payload_sent
         << ",\"p0_max_payload_received\":" << output0.max_payload_received
         << ",\"p1_bytes_sent\":"
         << (p1_status == 137 ? 0 : read_process_output(p1_output).sent_bytes)
         << ",\"p1_bytes_received\":"
         << (p1_status == 137 ? 0 : read_process_output(p1_output).received_bytes)
         << ",\"p1_frames_sent\":"
         << (p1_status == 137 ? 0 : read_process_output(p1_output).sent_frames)
         << ",\"p1_frames_received\":"
         << (p1_status == 137 ? 0 : read_process_output(p1_output).received_frames)
         << ",\"p1_max_payload_sent\":"
         << (p1_status == 137 ? 0 : read_process_output(p1_output).max_payload_sent)
         << ",\"p1_max_payload_received\":"
         << (p1_status == 137 ? 0 : read_process_output(p1_output).max_payload_received)
         << ",\"p0_restore\":\""
         << (normal_path ? "committed; recovery path separately verified"
                         : "unavailable from A; no commit") << "\""
         << ",\"p1_restore\":\""
         << (normal_path ? "committed; recovery path separately verified"
                         : "unavailable from A; no commit") << "\""
         << ",\"proxy_exception\":\"" << proxy_error << "\"}\n";
  result.close();
  std::cout << (normal_path ? "S34_V3_NORMAL_PATH_TRACE "
                            : "S34_V3_COMMUNICATION_FAULT ") << name
            << " PASS dealer_exit=" << dealer_status << " p0_exit=" << p0_status
            << " p1_exit=" << p1_status << " escrow_exit=" << service_status
            << " leases=burned A_rows=" << transaction_count << ',' << share_count
            << " p0_api=" << p0_api << " p1_api=" << p1_api
            << " proxy_error=" << proxy_error << '\n';
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc > 1 && std::string(argv[1]) == "--dealer") return dealer_worker(argc, argv);
    if (argc > 1 && std::string(argv[1]) == "--service") return escrow_worker(argc, argv);
    if (argc > 1 && std::string(argv[1]) == "--party") return party_worker(argc, argv);
    const auto binary = std::filesystem::canonical(argv[0]).string();
    TempDirectory directory;
    const auto identities = create_test_identities(directory.path);
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
    if (argc > 1 && std::string(argv[1]) == "--ft-v3-n1000") {
      const auto make_scores = [](std::uint32_t n) {
        std::vector<std::int32_t> scores(n);
        for (std::uint32_t i = 0; i < n; ++i) {
          switch (i % 10U) {
            case 0: scores[i] = INT32_MAX; break;
            case 1: scores[i] = INT32_MIN; break;
            case 2: scores[i] = -1; break;
            case 3: scores[i] = 0; break;
            case 4: scores[i] = 17; break;
            case 5: scores[i] = INT32_MAX; break;
            case 6: scores[i] = -17; break;
            case 7: scores[i] = 4096; break;
            case 8: scores[i] = -4096; break;
            default: scores[i] = 17; break;
          }
        }
        return scores;
      };
      std::size_t count = 0;
      const auto run = [&](std::uint32_t n, std::uint32_t k,
                           std::uint32_t version, std::uint8_t receipt,
                           const std::string& id) {
        const auto case_dir = (std::filesystem::path(directory.path) / id).string();
        std::filesystem::create_directory(case_dir);
        run_one_e2e(binary, case_dir, identities, make_scores(n), k, false,
                    false, false, receipt, id, version);
        ++count;
      };
      for (const auto k : {1U, 8U, 500U, 999U, 1000U})
        run(1000U, k, 3U, kScoreToMaskV3ReceiptFormatV2,
            "s47-ft-v3-n1000-k" + std::to_string(k));
      run(1U, 1U, 3U, kScoreToMaskV3ReceiptFormatV2,
          "s47-ft-v3-n1-k1");
      run(128U, 64U, 3U, kScoreToMaskV3ReceiptFormatV2,
          "s47-ft-v3-n128-k64");
      run(256U, 128U, 3U, kScoreToMaskV3ReceiptFormatV2,
          "s47-ft-v3-n256-k128");
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL
      run(128U, 64U, 1U, kScoreToMaskV3ReceiptFormatV1,
          "s47-legacy-ft-v1-n128-k64");
      run(256U, 128U, 2U, kScoreToMaskV3ReceiptFormatV2,
          "s47-legacy-ft-v2-n256-k128");
#endif
      std::cout << "S47_FT_V3_TARGETED_CASES PASS cases=" << count << '\n';
      return 0;
    }
#endif
#if defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL) && \
    defined(MOE_TOPK_SCORE_TO_MASK_V3_FT_V2_TESTING)
    if (argc > 1 && std::string(argv[1]) == "--ft-v2-target-k-cases") {
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_RECEIPT_V2_EXPERIMENTAL
      const auto make_scores = [](std::uint32_t n) {
        std::vector<std::int32_t> scores(n);
        for (std::uint32_t i = 0; i < n; ++i) {
          switch (i % 8U) {
            case 0: scores[i] = INT32_MAX; break;
            case 1: scores[i] = INT32_MIN; break;
            case 2: scores[i] = -1; break;
            case 3: scores[i] = 17; break;
            case 4: scores[i] = INT32_MAX; break;
            case 5: scores[i] = -17; break;
            case 6: scores[i] = 0; break;
            default: scores[i] = 17; break;
          }
        }
        return scores;
      };
      std::size_t case_count = 0;
      for (const auto n : {128U, 256U}) {
        const auto scores = make_scores(n);
        const std::vector<std::uint32_t> k_values =
            {1U, 8U, n / 2U, n - 1U, n};
        for (std::size_t index = 0; index < k_values.size(); ++index) {
          const auto k = k_values[index];
          const bool use_receipt_v2 = ((index + (n == 256U ? 1U : 0U)) % 2U) != 0;
          const auto receipt_version = use_receipt_v2
              ? kScoreToMaskV3ReceiptFormatV2
              : kScoreToMaskV3ReceiptFormatV1;
          const auto receipt_label = use_receipt_v2 ? "receipt-v2" : "receipt-v1";
          const auto case_id = "s46-ft-v2-n" + std::to_string(n) + "-k" +
              std::to_string(k) + "-" + receipt_label;
          const auto case_directory = (std::filesystem::path(directory.path) /
              case_id).string();
          std::filesystem::create_directory(case_directory);
          run_one_e2e(binary, case_directory, identities, scores, k,
              false, false, false, receipt_version, case_id, 2);
          ++case_count;
        }
      }
      run_ft_v2_issuance_abort_process_case(binary, directory.path, identities);
      std::cout << "S46_FT_V2_TARGET_K_CASES PASS cases=" << case_count << '\n';
      return 0;
#else
      throw std::runtime_error(
          "F_T/v2 target K cases require receipt/v2 opt-in for both receipt profiles");
#endif
    }
    if (argc > 1 && std::string(argv[1]) == "--ft-v2-smoke") {
      const auto make_scores = [](std::uint32_t n) {
        std::vector<std::int32_t> scores(n);
        for (std::uint32_t i = 0; i < n; ++i) {
          switch (i % 8U) {
            case 0: scores[i] = INT32_MAX; break;
            case 1: scores[i] = INT32_MIN; break;
            case 2: scores[i] = -1; break;
            case 3: scores[i] = 17; break;
            case 4: scores[i] = INT32_MAX; break;
            case 5: scores[i] = -17; break;
            case 6: scores[i] = 0; break;
            default: scores[i] = 17; break;
          }
        }
        return scores;
      };
      const auto n128_dir = (std::filesystem::path(directory.path) / "ft-v2-n128").string();
      std::filesystem::create_directory(n128_dir);
      run_one_e2e(binary, n128_dir, identities, make_scores(128), 64,
          false, false, false, kScoreToMaskV3ReceiptFormatV1,
          "ft-v2-n128-receipt-v1", 2);
      const auto n256_dir = (std::filesystem::path(directory.path) / "ft-v2-n256").string();
      std::filesystem::create_directory(n256_dir);
      std::uint8_t n256_receipt = kScoreToMaskV3ReceiptFormatV1;
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_RECEIPT_V2_EXPERIMENTAL
      n256_receipt = kScoreToMaskV3ReceiptFormatV2;
#endif
      run_one_e2e(binary, n256_dir, identities, make_scores(256), 128,
          false, false, false, n256_receipt,
          "ft-v2-n256-independent-receipt", 2);
      run_ft_v2_issuance_abort_process_case(binary, directory.path, identities);
      std::cout << "S45_FT_V2_TARGETED_MULTI_PROCESS_E2E PASS\n";
      return 0;
    }
#endif
    if (argc > 1 && std::string(argv[1]) == "--communication-faults-only") {
      if (argc != 3) throw std::runtime_error(
          "--communication-faults-only requires an evidence log directory");
      const std::string logs_root = argv[2];
      std::filesystem::create_directories(logs_root);
      run_communication_fault_case(binary, directory.path, identities,
          CommunicationFault::kReadyMismatch, logs_root);
      run_communication_fault_case(binary, directory.path, identities,
          CommunicationFault::kDuplicateReady, logs_root);
      run_communication_fault_case(binary, directory.path, identities,
          CommunicationFault::kKillP1AfterOpening, logs_root);
      return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "--normal-trace-only") {
      if (argc != 3) throw std::runtime_error(
          "--normal-trace-only requires an evidence log directory");
      const std::string logs_root = argv[2];
      std::filesystem::create_directories(logs_root);
      run_communication_fault_case(binary, directory.path, identities,
          CommunicationFault::kNoFault, logs_root);
      return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "--fault-only") {
      const auto lost_receipt = (std::filesystem::path(directory.path) /
                                 "lost-receipt").string();
      std::filesystem::create_directory(lost_receipt);
      run_one_e2e(binary, lost_receipt, identities,
                  {-9, -9, 10, INT32_MIN}, 2, true);
      const auto crash = (std::filesystem::path(directory.path) /
                          "precommit-crash").string();
      std::filesystem::create_directory(crash);
      run_one_e2e(binary, crash, identities, {5, 5, 0}, 2, false, true);
      const auto timeout = (std::filesystem::path(directory.path) /
                            "precommit-timeout").string();
      std::filesystem::create_directory(timeout);
      run_one_e2e(binary, timeout, identities, {INT32_MAX, 1, 1}, 1,
                  false, false, true);
      std::cout << "S32 score-to-mask-v3 escrow fault recovery PASS\n";
      return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "--receipt-v2-only") {
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_RECEIPT_V2_EXPERIMENTAL
      const auto success = (std::filesystem::path(directory.path) /
                            "receipt-v2-success").string();
      std::filesystem::create_directory(success);
      run_one_e2e(binary, success, identities, {-7, 11, 11, INT32_MIN}, 2,
                  false, false, false, kScoreToMaskV3ReceiptFormatV2);
      const auto lost = (std::filesystem::path(directory.path) /
                         "receipt-v2-lost-response").string();
      std::filesystem::create_directory(lost);
      run_one_e2e(binary, lost, identities, {-9, -9, 10, INT32_MIN}, 2,
                  true, false, false, kScoreToMaskV3ReceiptFormatV2);
      const auto abort = (std::filesystem::path(directory.path) /
                          "receipt-v2-precommit-abort").string();
      std::filesystem::create_directory(abort);
      run_one_e2e(binary, abort, identities, {5, 5, 0}, 2,
                  false, true, false, kScoreToMaskV3ReceiptFormatV2);
      std::cout << "S39 receipt/v2 targeted independent-process E2E PASS\n";
      return 0;
#else
      throw std::runtime_error(
          "receipt/v2 E2E requires MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_RECEIPT_V2_EXPERIMENTAL=ON");
#endif
    }
    bool receipt_v2_matrix = false;
    if (argc > 1 && std::string(argv[1]) == "--receipt-v2-matrix") {
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_RECEIPT_V2_EXPERIMENTAL
      receipt_v2_matrix = true;
#else
      throw std::runtime_error(
          "receipt/v2 matrix requires MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_RECEIPT_V2_EXPERIMENTAL=ON");
#endif
    }
    if (argc > 1 && std::string(argv[1]) == "--receipt-v2-normal-target-trace") {
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_RECEIPT_V2_EXPERIMENTAL
      if (argc != 3) throw std::runtime_error(
          "--receipt-v2-normal-target-trace requires an evidence log directory");
      const std::uint32_t n = 128;
      std::vector<std::int32_t> scores(n);
      for (std::uint32_t i = 0; i < n; ++i) {
        switch (i % 8U) {
          case 0: scores[i] = INT32_MAX; break;
          case 1: scores[i] = INT32_MIN; break;
          case 2: scores[i] = -1; break;
          case 3: scores[i] = 17; break;
          case 4: scores[i] = INT32_MAX; break;
          case 5: scores[i] = -17; break;
          case 6: scores[i] = 0; break;
          default: scores[i] = 17; break;
        }
      }
      std::filesystem::create_directories(argv[2]);
      run_communication_fault_case(binary, directory.path, identities,
          CommunicationFault::kNoFault, argv[2], scores, n / 2U,
          kScoreToMaskV3ReceiptFormatV2);
      return 0;
#else
      throw std::runtime_error(
          "receipt/v2 normal trace requires MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_RECEIPT_V2_EXPERIMENTAL=ON");
#endif
    }
    if (argc > 1 && std::string(argv[1]) == "--receipt-v1-smoke") {
      const auto v1_smoke = (std::filesystem::path(directory.path) /
                             "receipt-v1-smoke").string();
      std::filesystem::create_directory(v1_smoke);
      run_one_e2e(binary, v1_smoke, identities, {-7, 11, 11, INT32_MIN}, 2);
      std::cout << "S39 receipt/v1 compatibility E2E PASS\n";
      return 0;
    }
    const std::uint8_t receipt_format_version = receipt_v2_matrix
        ? kScoreToMaskV3ReceiptFormatV2 : kScoreToMaskV3ReceiptFormatV1;
    auto run_matrix_case = [&](const std::string& case_directory,
                               const std::vector<std::int32_t>& scores,
                               std::uint32_t k, const std::string& case_id,
                               bool lose_receipt = false,
                               bool precommit_abort = false,
                               bool precommit_timeout = false) {
      run_one_e2e(binary, case_directory, identities, scores, k,
          lose_receipt, precommit_abort, precommit_timeout,
          receipt_format_version, case_id);
    };
    struct Fixture {
      std::vector<std::int32_t> scores;
      std::vector<std::uint32_t> k_values;
    };
    std::vector<Fixture> fixtures{
        {{INT32_MIN}, {1}},
        {{INT32_MIN, INT32_MAX}, {1, 2}},
        {{-5, -5, -5}, {1, 2, 3}},
        {{7, 7, -8, INT32_MAX}, {1, 2, 3, 4}},
        {{7, 7, INT32_MIN, -2, INT32_MAX}, {3}},
        {{INT32_MIN, INT32_MIN, INT32_MIN, INT32_MIN,
          INT32_MIN, INT32_MIN, INT32_MIN, INT32_MIN}, {1, 8}}};
    for (const std::uint32_t n : {128U, 256U}) {
      std::vector<std::int32_t> resource_scores(n);
      if (RAND_bytes(reinterpret_cast<std::uint8_t*>(resource_scores.data()),
                     static_cast<int>(resource_scores.size() * sizeof(std::int32_t))) != 1)
        throw std::runtime_error("resource fixture CSPRNG failed");
      fixtures.push_back({std::move(resource_scores), {n / 2U}});
    }
    std::size_t case_index = 0;
    for (const auto& fixture : fixtures) {
      for (const auto k : fixture.k_values) {
        const auto case_dir = (std::filesystem::path(directory.path) /
            ("case-" + std::to_string(case_index++))).string();
        std::filesystem::create_directory(case_dir);
        run_matrix_case(case_dir, fixture.scores, k,
            "fixture-" + std::to_string(case_index - 1U));
      }
    }
    for (std::uint32_t code = 0; code < 27; ++code) {
      auto value = code;
      std::vector<std::int32_t> scores(3);
      for (auto& score : scores) {
        score = static_cast<std::int32_t>(value % 3U) - 1;
        value /= 3U;
      }
      for (std::uint32_t k = 1; k <= 3; ++k) {
        const auto case_dir = (std::filesystem::path(directory.path) /
            ("case-ternary-n3-" + std::to_string(case_index++))).string();
        std::filesystem::create_directory(case_dir);
        run_matrix_case(case_dir, scores, k,
            "ternary-n3-" + std::to_string(code) + "-k-" + std::to_string(k));
      }
    }
    std::array<std::int32_t, 4> permuted_scores{
        INT32_MIN, -1, 1, INT32_MAX};
    do {
      for (std::uint32_t k = 1; k <= 4; ++k) {
        const auto case_dir = (std::filesystem::path(directory.path) /
            ("case-permutation-n4-" + std::to_string(case_index++))).string();
        std::filesystem::create_directory(case_dir);
        run_matrix_case(case_dir,
            std::vector<std::int32_t>(permuted_scores.begin(),
                                      permuted_scores.end()), k,
            "permutation-n4-k-" + std::to_string(k) + "-case-" +
                std::to_string(case_index - 1U));
      }
    } while (std::next_permutation(permuted_scores.begin(), permuted_scores.end()));
    const auto recovery_dir = (std::filesystem::path(directory.path) /
                               "case-lost-receipt-after-commit").string();
    std::filesystem::create_directory(recovery_dir);
    run_matrix_case(recovery_dir, {-9, -9, 10, INT32_MIN}, 2,
                    "lost-receipt-n4", true);
    const auto abort_dir = (std::filesystem::path(directory.path) /
                            "case-precommit-escrow-crash").string();
    std::filesystem::create_directory(abort_dir);
    run_matrix_case(abort_dir, {5, 5, 0}, 2, "precommit-abort-n3", false, true);
    const auto timeout_dir = (std::filesystem::path(directory.path) /
                              "case-precommit-timeout").string();
    std::filesystem::create_directory(timeout_dir);
    run_matrix_case(timeout_dir, {INT32_MAX, 1, 1}, 1,
                    "precommit-timeout-n3", false, false, true);
    if (receipt_v2_matrix) {
      for (const std::uint32_t n : {128U, 256U}) {
        for (const std::uint32_t k : {1U, 2U, 8U, n / 2U, n - 1U, n}) {
          std::vector<std::int32_t> scores(n);
          const std::size_t pattern = k == 1U ? 0U : k == 2U ? 1U :
              k == 8U ? 2U : k == n / 2U ? 3U : k == n - 1U ? 4U : 5U;
          for (std::uint32_t i = 0; i < n; ++i) {
            switch (pattern) {
              case 0: scores[i] = (i % 8U == 0U) ? INT32_MAX
                                                 : -static_cast<std::int32_t>(i % 97U); break;
              case 1: scores[i] = 23; break;
              case 2: {
                const std::array<std::int32_t, 8> values{
                    INT32_MIN, -1, 0, 1, INT32_MAX, -1, 17, 17};
                scores[i] = values[i % values.size()];
                break;
              }
              case 3: scores[i] = (i % 2U == 0U) ? INT32_MAX : INT32_MIN; break;
              case 4: scores[i] = (i % 16U == 0U) ? INT32_MIN
                                                   : -static_cast<std::int32_t>(i % 11U); break;
              default: scores[i] = (i % 3U == 0U) ? INT32_MIN
                                                   : (i % 3U == 1U ? INT32_MAX : -7); break;
            }
          }
          const auto case_dir = (std::filesystem::path(directory.path) /
              ("receipt-v2-target-n" + std::to_string(n) + "-k-" +
               std::to_string(k))).string();
          std::filesystem::create_directory(case_dir);
          run_matrix_case(case_dir, scores, k,
              "receipt-v2-target-n" + std::to_string(n) + "-k-" +
                  std::to_string(k));
        }
        std::vector<std::int32_t> recovery_scores(n);
        for (std::uint32_t i = 0; i < n; ++i)
          recovery_scores[i] = (i % 4U == 0U) ? INT32_MAX
                                               : -static_cast<std::int32_t>(i % 31U);
        const auto lost_dir = (std::filesystem::path(directory.path) /
            ("receipt-v2-target-n" + std::to_string(n) + "-lost-receipt")).string();
        std::filesystem::create_directory(lost_dir);
        run_matrix_case(lost_dir, recovery_scores, n / 2U,
            "receipt-v2-target-n" + std::to_string(n) + "-lost-receipt", true);
        const auto abort_dir = (std::filesystem::path(directory.path) /
            ("receipt-v2-target-n" + std::to_string(n) + "-precommit-abort")).string();
        std::filesystem::create_directory(abort_dir);
        run_matrix_case(abort_dir, recovery_scores, n / 2U,
            "receipt-v2-target-n" + std::to_string(n) + "-precommit-abort",
            false, true);
      }
    }
    std::cout << (receipt_v2_matrix
        ? "S40 receipt/v2 195-case plus target-boundary independent-process E2E PASS\n"
        : "S32 score-to-mask-v3 independent-process E2E PASS\n");
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "S32_V3_PROCESS_E2E_FAIL " << error.what() << '\n';
    return 1;
  }
}
