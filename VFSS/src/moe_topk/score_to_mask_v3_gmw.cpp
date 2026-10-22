#include <moe_topk/score_to_mask_v3_gmw.h>
#include <moe_topk/protocol_i_transport.h>
#include "score_to_mask_v3_bounded_sampler.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <sqlite3.h>

namespace moe_topk {
namespace {

constexpr std::array<std::uint8_t, 8> kMagic = {'S','2','M','V','3','P','K','G'};
constexpr std::array<std::uint8_t, 8> kEscrowContextMagic =
    {'S','2','M','V','3','E','C','T'};
constexpr std::size_t kSignatureBytes = 64;
constexpr std::size_t kHeaderBytes = 8 + 4 + 4 + 4 + 16 + 16 + 4 + 4 + 160 + 1 + 7 + 8 +
    4 + 4 + 32 + 32 + 32 + 32;
constexpr std::string_view kCircuitDescription =
    "PROJECT_DERIVED_SELECT_5ITER/GMW-Beaver-v3;score=Q20.12-signed32;"
    "priority=descending-score,ascending-original-index;pad=2^ceil(log2(max(2,n)));"
    "add=63N;forward=N2W;compare=C(N,2)(2W-1);inverse=N2;"
    "five-iteration-select=public-approved-edge-open;membership=secret;"
    "output=F_TOPK-U;escrow=F_TOPK-ATOMIC;W=32+ceil(log2(max(2,N)))";
constexpr std::string_view kScheduleDescription =
    "gate-id-order:v3;add-word-major:generate-bits-0..31,carry-bits-1..31;"
    "forward:row,original,key-bit;compare:pair-lexicographic,generate-bit,"
    "borrow-bit-1..W-1;inverse:original,shuffled;all-batches-depth-ordered";
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL
constexpr std::string_view kFtV2PartyPackageSignatureDomain =
    "ProtocolI/PROJECT_DERIVED_SELECT_5ITER/GMW-Beaver-v3/party-package/"
    "F_T-v2/bounded-rejection-v1";
#endif
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
constexpr std::string_view kFtV3PartyPackageSignatureDomain =
    "ProtocolI/PROJECT_DERIVED_SELECT_5ITER/GMW-Beaver-v3/party-package/"
    "F_T-v3/bounded-rejection-v2";
constexpr std::string_view kFtV3ScheduleDomain =
    "ProtocolI/PROJECT_DERIVED_SELECT_5ITER/GMW-Beaver-v3/gate-schedule/"
    "F_T-v3/batched-AND-frame-1MiB/v1";
#endif

struct PackageProfile {
  std::uint32_t package_format_version = kScoreToMaskV3PackageFormatVersion;
  std::uint8_t ft_version = 1;
  std::uint8_t sampler_version = 0;
  std::uint8_t sampler_security_parameter_bits = 0;
  std::uint8_t sampler_retry_limit = 0;
};

[[noreturn]] void fail(const std::string& what) { throw std::runtime_error(what); }

bool receipt_format_supported(std::uint8_t version) {
  if (version == kScoreToMaskV3ReceiptFormatV1) return true;
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_RECEIPT_V2_EXPERIMENTAL
  if (version == kScoreToMaskV3ReceiptFormatV2) return true;
#endif
  return false;
}

void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
  out.push_back(static_cast<std::uint8_t>(value >> 24U));
  out.push_back(static_cast<std::uint8_t>(value >> 16U));
  out.push_back(static_cast<std::uint8_t>(value >> 8U));
  out.push_back(static_cast<std::uint8_t>(value));
}

void append_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
  for (int shift = 56; shift >= 0; shift -= 8)
    out.push_back(static_cast<std::uint8_t>(value >> shift));
}

std::uint32_t read_u32(const std::vector<std::uint8_t>& in, std::size_t& at) {
  if (at + 4 > in.size()) fail("v3 package truncated u32");
  std::uint32_t value = 0;
  for (int i = 0; i < 4; ++i) value = (value << 8U) | in[at++];
  return value;
}

std::uint64_t read_u64(const std::vector<std::uint8_t>& in, std::size_t& at) {
  if (at + 8 > in.size()) fail("v3 package truncated u64");
  std::uint64_t value = 0;
  for (int i = 0; i < 8; ++i) value = (value << 8U) | in[at++];
  return value;
}

template <std::size_t N>
void append_array(std::vector<std::uint8_t>& out,
                  const std::array<std::uint8_t, N>& value) {
  out.insert(out.end(), value.begin(), value.end());
}

template <std::size_t N>
void read_array(const std::vector<std::uint8_t>& in, std::size_t& at,
                std::array<std::uint8_t, N>& value) {
  if (at + N > in.size()) fail("v3 package truncated array");
  std::copy_n(in.begin() + static_cast<std::ptrdiff_t>(at), N, value.begin());
  at += N;
}

std::array<std::uint8_t, 32> sha256(const std::uint8_t* bytes, std::size_t size) {
  std::array<std::uint8_t, 32> out{};
  if (SHA256(bytes, size, out.data()) == nullptr) fail("SHA-256 failed");
  return out;
}

std::array<std::uint8_t, 32> sha256(const std::vector<std::uint8_t>& bytes) {
  return sha256(bytes.data(), bytes.size());
}

template <std::size_t N>
std::array<std::uint8_t, 32> hash_text(const std::array<std::uint8_t, N>& prefix,
                                       std::string_view text) {
  std::vector<std::uint8_t> bytes(prefix.begin(), prefix.end());
  bytes.insert(bytes.end(), text.begin(), text.end());
  return sha256(bytes);
}

std::uint64_t first_u64(const std::array<std::uint8_t, 16>& value) {
  std::uint64_t result = 0;
  for (std::size_t i = 0; i < 8; ++i) result = (result << 8U) | value[i];
  return result;
}

std::array<std::uint8_t, 32> circuit_hash() {
  return hash_text(kMagic, kCircuitDescription);
}

std::array<std::uint8_t, 32> schedule_hash() {
  return hash_text(kMagic, kScheduleDescription);
}
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
std::array<std::uint8_t, 32> schedule_hash_ft_v3() {
  std::vector<std::uint8_t> bytes(kMagic.begin(), kMagic.end());
  bytes.insert(bytes.end(), kFtV3ScheduleDomain.begin(), kFtV3ScheduleDomain.end());
  bytes.push_back(0);
  bytes.insert(bytes.end(), kScheduleDescription.begin(), kScheduleDescription.end());
  append_u32(bytes, kScoreToMaskV3FtV3MaxBatchGates);
  append_u32(bytes, static_cast<std::uint32_t>(kScoreToMaskV3MaxGateBatchFrameBytes));
  append_u32(bytes, 91U);
  append_u32(bytes, 13U);
  append_u32(bytes, 1U);
  append_u32(bytes, 1U);
  return sha256(bytes);
}
#endif

std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> read_key(
    const std::string& path, bool private_key) {
  FILE* file = std::fopen(path.c_str(), "rb");
  if (file == nullptr) fail("cannot open v3 Ed25519 key: " + path);
  EVP_PKEY* key = private_key ? PEM_read_PrivateKey(file, nullptr, nullptr, nullptr)
                              : PEM_read_PUBKEY(file, nullptr, nullptr, nullptr);
  std::fclose(file);
  if (key == nullptr || EVP_PKEY_id(key) != EVP_PKEY_ED25519) {
    if (key != nullptr) EVP_PKEY_free(key);
    fail("v3 package key must be Ed25519 PEM");
  }
  return {key, EVP_PKEY_free};
}

std::array<std::uint8_t, 32> key_fingerprint(EVP_PKEY* key) {
  const int len = i2d_PUBKEY(key, nullptr);
  if (len <= 0 || len > 4096) fail("invalid Ed25519 public key encoding");
  std::vector<std::uint8_t> der(static_cast<std::size_t>(len));
  auto* cursor = der.data();
  if (i2d_PUBKEY(key, &cursor) != len) fail("Ed25519 public key encode failed");
  return sha256(der);
}

std::array<std::uint8_t, kSignatureBytes> sign_bytes(
    EVP_PKEY* key, const std::vector<std::uint8_t>& bytes) {
  std::array<std::uint8_t, kSignatureBytes> signature{};
  std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(),
                                                            EVP_MD_CTX_free);
  std::size_t size = signature.size();
  if (!ctx || EVP_DigestSignInit(ctx.get(), nullptr, nullptr, nullptr, key) != 1 ||
      EVP_DigestSign(ctx.get(), signature.data(), &size, bytes.data(),
                     bytes.size()) != 1 || size != signature.size())
    fail("Ed25519 package signing failed");
  return signature;
}

bool verify_bytes(EVP_PKEY* key, const std::vector<std::uint8_t>& bytes,
                  const std::uint8_t* signature, std::size_t signature_size) {
  std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(),
                                                            EVP_MD_CTX_free);
  if (!ctx || EVP_DigestVerifyInit(ctx.get(), nullptr, nullptr, nullptr, key) != 1)
    fail("Ed25519 package verify initialization failed");
  return EVP_DigestVerify(ctx.get(), signature, signature_size, bytes.data(),
                          bytes.size()) == 1;
}

std::vector<std::uint8_t> domain_separated_message(
    std::string_view domain, std::uint32_t version,
    const std::vector<std::uint8_t>& message) {
  std::vector<std::uint8_t> result(domain.begin(), domain.end());
  result.push_back(0);
  append_u32(result, version);
  result.insert(result.end(), message.begin(), message.end());
  return result;
}

std::array<std::uint8_t, 32> package_set_hash(
    const std::array<std::array<std::uint8_t, 32>, 2>& package_hashes) {
  static constexpr std::string_view domain =
      "ProtocolI/PROJECT_DERIVED_SELECT_5ITER/GMW-Beaver-v3/package-set/v1";
  std::vector<std::uint8_t> bytes(domain.begin(), domain.end());
  bytes.push_back(0);
  append_array(bytes, package_hashes[0]);
  append_array(bytes, package_hashes[1]);
  return sha256(bytes);
}

std::vector<std::uint8_t> serialize_escrow_context_body(
    const ScoreToMaskV3EscrowContext& context) {
  std::vector<std::uint8_t> out;
  out.insert(out.end(), kEscrowContextMagic.begin(), kEscrowContextMagic.end());
  append_u32(out, context.format_version);
  append_u32(out, kScoreToMaskV3ProtocolVersion);
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
  if (context.format_version == kScoreToMaskV3EscrowContextFormatVersionFtV3) {
    out.push_back(context.ft_version);
    out.push_back(context.sampler_version);
    out.push_back(context.sampler_security_parameter_bits);
    out.push_back(context.sampler_retry_limit);
    append_u32(out, context.package_format_version);
  }
#endif
  append_array(out, context.session);
  append_array(out, context.invocation);
  append_u32(out, context.n);
  append_u32(out, context.k);
  for (const auto& identity : context.party_identity_sha256) append_array(out, identity);
  append_array(out, context.escrow_identity_sha256);
  append_array(out, context.circuit_sha256);
  append_array(out, context.gate_schedule_sha256);
  for (const auto& package_hash : context.party_package_sha256)
    append_array(out, package_hash);
  append_array(out, context.package_set_sha256);
  append_array(out, context.signer_identity_sha256);
  append_u32(out, context.state.transaction_wait_timeout_ms);
  append_u32(out, context.state.max_ciphertext_bytes);
  out.push_back(context.state.pending_restart_policy);
  out.push_back(context.state.receipt_format_version);
  out.push_back(0);
  out.push_back(0);
  return out;
}

std::vector<std::uint8_t> make_signed_escrow_context(
    const ScoreToMaskV3EscrowContext& context, EVP_PKEY* signer) {
  const auto body = serialize_escrow_context_body(context);
  const auto message = domain_separated_message(
      kScoreToMaskV3EscrowContextSignatureDomain,
      context.format_version, body);
  const auto signature = sign_bytes(signer, message);
  auto file = body;
  append_array(file, signature);
  if (file.size() > kScoreToMaskV3MaxMetadataBytes)
    fail("v3 escrow context exceeds metadata limit");
  return file;
}

void random_bytes(std::uint8_t* out, std::size_t size) {
  while (size != 0) {
    const auto chunk = static_cast<int>(std::min<std::size_t>(size, 1U << 20));
    if (RAND_bytes(out, chunk) != 1) fail("OpenSSL CSPRNG failed");
    out += chunk;
    size -= static_cast<std::size_t>(chunk);
  }
}

bool bit_at(const std::vector<std::uint8_t>& bits, std::uint64_t index) {
  return ((bits.at(static_cast<std::size_t>(index / 8U)) >> (index % 8U)) & 1U) != 0;
}

void set_bit(std::vector<std::uint8_t>& bits, std::uint64_t index, bool value) {
  const auto byte = static_cast<std::size_t>(index / 8U);
  const auto mask = static_cast<std::uint8_t>(1U << (index % 8U));
  if (value) bits.at(byte) |= mask;
  else bits.at(byte) &= static_cast<std::uint8_t>(~mask);
}

std::uint32_t next_uniform(std::uint32_t bound) {
  if (bound == 0) fail("zero uniform bound");
  const std::uint32_t threshold = static_cast<std::uint32_t>(-bound) % bound;
  for (;;) {
    std::uint32_t value = 0;
    random_bytes(reinterpret_cast<std::uint8_t*>(&value), sizeof(value));
    if (value >= threshold) return value % bound;
  }
}

bool all_zero(const std::array<std::uint8_t, 16>& value) {
  return std::all_of(value.begin(), value.end(), [](std::uint8_t b) { return b == 0; });
}

bool all_zero(const std::array<std::uint8_t, 32>& value) {
  return std::all_of(value.begin(), value.end(), [](std::uint8_t b) { return b == 0; });
}

std::vector<std::uint8_t> read_file(const std::string& path,
                                    std::size_t maximum) {
  std::ifstream input(path, std::ios::binary);
  if (!input) fail("cannot open v3 package: " + path);
  input.seekg(0, std::ios::end);
  const auto end = input.tellg();
  if (end < 0 || static_cast<std::uint64_t>(end) > maximum)
    fail("v3 package size out of bounds");
  input.seekg(0, std::ios::beg);
  std::vector<std::uint8_t> result(static_cast<std::size_t>(end));
  if (!result.empty()) input.read(reinterpret_cast<char*>(result.data()), end);
  if (!input) fail("v3 package read failed");
  return result;
}

std::vector<std::uint8_t> read_package_file(const std::string& path) {
  std::ifstream prefix(path, std::ios::binary);
  if (!prefix) fail("cannot open v3 package: " + path);
  std::array<std::uint8_t, 12> first{};
  prefix.read(reinterpret_cast<char*>(first.data()), first.size());
  if (prefix.gcount() != static_cast<std::streamsize>(first.size()) ||
      !std::equal(kMagic.begin(), kMagic.end(), first.begin()))
    fail("v3 package magic/length");
  const auto format = (static_cast<std::uint32_t>(first[8]) << 24U) |
      (static_cast<std::uint32_t>(first[9]) << 16U) |
      (static_cast<std::uint32_t>(first[10]) << 8U) | first[11];
  std::size_t maximum = kScoreToMaskV3MaxPackageBytes;
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
  if (format == kScoreToMaskV3PackageFormatVersionFtV3)
    maximum = kScoreToMaskV3FtV3MaxPackageBytes;
#endif
  if (format != kScoreToMaskV3PackageFormatVersion
#if defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL) || defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL)
      && format != kScoreToMaskV3PackageFormatVersionFtV2
#endif
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
      && format != kScoreToMaskV3PackageFormatVersionFtV3
#endif
      )
    fail("v3 package format is unavailable in this build");
  return read_file(path, maximum);
}

void durable_create(const std::string& path, const std::vector<std::uint8_t>& bytes) {
  const int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
  if (fd < 0) fail("v3 output exists or cannot be created: " + path + ": " +
                   std::strerror(errno));
  std::size_t offset = 0;
  try {
    while (offset < bytes.size()) {
      const auto count = ::write(fd, bytes.data() + offset, bytes.size() - offset);
      if (count < 0 && errno == EINTR) continue;
      if (count <= 0) fail("v3 durable write failed");
      offset += static_cast<std::size_t>(count);
    }
    if (::fsync(fd) != 0) fail("v3 durable fsync failed");
    if (::close(fd) != 0) fail("v3 durable close failed");
    const auto parent = std::filesystem::path(path).parent_path();
    const int dirfd = ::open(parent.empty() ? "." : parent.c_str(),
                             O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (dirfd < 0) fail("v3 package parent directory unavailable");
    const auto sync_result = ::fsync(dirfd);
    ::close(dirfd);
    if (sync_result != 0) fail("v3 package directory fsync failed");
  } catch (...) {
    ::close(fd);
    throw;
  }
}

void register_ft_v2_invocation_once(
    const ScoreToMaskV3Session& session,
    const std::string& private_registry_directory) {
  static constexpr std::string_view key_domain =
      "ProtocolI/PROJECT_DERIVED_SELECT_5ITER/GMW-Beaver-v3/"
      "F_T-v2/issuance-registry-key/v1";
  static constexpr std::string_view record_domain =
      "ProtocolI/PROJECT_DERIVED_SELECT_5ITER/GMW-Beaver-v3/"
      "F_T-v2/issuance-burn-record/v1";

  std::vector<std::uint8_t> key_input(key_domain.begin(), key_domain.end());
  key_input.push_back(0);
  append_array(key_input, session.session);
  append_array(key_input, session.invocation);
  const auto key_hash = sha256(key_input);
  static constexpr char kHex[] = "0123456789abcdef";
  std::string filename = "ft-v2-";
  filename.reserve(filename.size() + 2U * key_hash.size());
  for (const auto byte : key_hash) {
    filename.push_back(kHex[byte >> 4U]);
    filename.push_back(kHex[byte & 0x0fU]);
  }

  const int dirfd = ::open(private_registry_directory.c_str(),
      O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  if (dirfd < 0)
    fail("F_T/v2 issuance registry directory is unavailable: " +
         std::string(std::strerror(errno)));
  struct stat registry_stat {};
  if (::fstat(dirfd, &registry_stat) != 0) {
    const int saved_errno = errno;
    ::close(dirfd);
    fail("F_T/v2 issuance registry stat failed: " +
         std::string(std::strerror(saved_errno)));
  }
  if (!S_ISDIR(registry_stat.st_mode) || registry_stat.st_uid != ::geteuid() ||
      (registry_stat.st_mode & 0077) != 0) {
    ::close(dirfd);
    fail("F_T/v2 issuance registry must be a private 0700 directory owned by T");
  }

  const int marker_fd = ::openat(dirfd, filename.c_str(),
      O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
  if (marker_fd < 0) {
    const int saved_errno = errno;
    ::close(dirfd);
    if (saved_errno == EEXIST)
      fail("F_T/v2 ISSUANCE_ABORT: session/invocation already issued or burned");
    fail("F_T/v2 issuance registry record cannot be created: " +
         std::string(std::strerror(saved_errno)));
  }

  std::vector<std::uint8_t> record(record_domain.begin(), record_domain.end());
  record.push_back(0);
  append_u32(record, 1);
  append_array(record, session.session);
  append_array(record, session.invocation);
  append_u32(record, session.n);
  append_u32(record, session.k);
  append_array(record, circuit_hash());
  append_array(record, schedule_hash());

  std::size_t offset = 0;
  int saved_errno = 0;
  while (offset < record.size()) {
    const auto written = ::write(marker_fd, record.data() + offset,
                                  record.size() - offset);
    if (written < 0 && errno == EINTR) continue;
    if (written <= 0) {
      saved_errno = written == 0 ? EIO : errno;
      break;
    }
    offset += static_cast<std::size_t>(written);
  }
  if (saved_errno == 0 && ::fsync(marker_fd) != 0) saved_errno = errno;
  if (::close(marker_fd) != 0 && saved_errno == 0) saved_errno = errno;
  if (saved_errno == 0 && ::fsync(dirfd) != 0) saved_errno = errno;
  ::close(dirfd);
  if (saved_errno != 0)
    fail("F_T/v2 issuance burn record could not be durably registered: " +
         std::string(std::strerror(saved_errno)));
}

#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
void register_ft_v3_invocation_once(
    const ScoreToMaskV3Session& session, const PackageProfile& profile,
    const std::string& private_registry_directory) {
  static constexpr std::string_view key_domain =
      "ProtocolI/PROJECT_DERIVED_SELECT_5ITER/GMW-Beaver-v3/"
      "F_T-v3/issuance-registry-key/v1";
  static constexpr std::string_view record_domain =
      "ProtocolI/PROJECT_DERIVED_SELECT_5ITER/GMW-Beaver-v3/"
      "F_T-v3/issuance-burn-record/v1";
  std::vector<std::uint8_t> key_input(key_domain.begin(), key_domain.end());
  key_input.push_back(0);
  append_array(key_input, session.session);
  append_array(key_input, session.invocation);
  const auto key_hash = sha256(key_input);
  static constexpr char hex[] = "0123456789abcdef";
  std::string filename = "ft-v3-";
  for (const auto byte : key_hash) {
    filename.push_back(hex[byte >> 4U]);
    filename.push_back(hex[byte & 0x0fU]);
  }
  const int dirfd = ::open(private_registry_directory.c_str(),
      O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  if (dirfd < 0) fail("F_T/v3 issuance registry is unavailable: " +
                      std::string(std::strerror(errno)));
  struct stat registry_stat {};
  if (::fstat(dirfd, &registry_stat) != 0) {
    const int saved = errno; ::close(dirfd);
    fail("F_T/v3 issuance registry stat failed: " + std::string(std::strerror(saved)));
  }
  if (!S_ISDIR(registry_stat.st_mode) || registry_stat.st_uid != ::geteuid() ||
      (registry_stat.st_mode & 0077) != 0) {
    ::close(dirfd);
    fail("F_T/v3 issuance registry must be a private 0700 directory owned by T");
  }
  const int marker_fd = ::openat(dirfd, filename.c_str(),
      O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
  if (marker_fd < 0) {
    const int saved = errno; ::close(dirfd);
    if (saved == EEXIST)
      fail("F_T/v3 ISSUANCE_ABORT: session/invocation already issued or burned");
    fail("F_T/v3 issuance marker cannot be created: " +
         std::string(std::strerror(saved)));
  }
  std::vector<std::uint8_t> record(record_domain.begin(), record_domain.end());
  record.push_back(0);
  append_u32(record, 1U);
  append_array(record, session.session);
  append_array(record, session.invocation);
  append_u32(record, session.n);
  append_u32(record, session.k);
  record.push_back(profile.ft_version);
  record.push_back(profile.sampler_version);
  record.push_back(profile.sampler_security_parameter_bits);
  record.push_back(profile.sampler_retry_limit);
  append_u32(record, profile.package_format_version);
  append_array(record, circuit_hash());
  append_array(record, schedule_hash_ft_v3());
  std::size_t offset = 0;
  int saved = 0;
  while (offset < record.size()) {
    const auto n = ::write(marker_fd, record.data() + offset, record.size() - offset);
    if (n < 0 && errno == EINTR) continue;
    if (n <= 0) { saved = n == 0 ? EIO : errno; break; }
    offset += static_cast<std::size_t>(n);
  }
  if (saved == 0 && ::fsync(marker_fd) != 0) saved = errno;
  if (::close(marker_fd) != 0 && saved == 0) saved = errno;
  if (saved == 0 && ::fsync(dirfd) != 0) saved = errno;
  ::close(dirfd);
  if (saved != 0)
    fail("F_T/v3 ISSUANCE_ABORT: burn marker durability failed; marker retained: " +
         std::string(std::strerror(saved)));
}
#endif

std::vector<std::uint8_t> make_header(const ScoreToMaskV3Session& session,
                                     std::uint8_t party,
                                     const ScoreToMaskV3GateBudget& budget,
                                     const std::array<std::uint8_t, 32>& circuit,
                                     const std::array<std::uint8_t, 32>& schedule,
                                     const std::array<std::uint8_t, 32>& material_hash,
                                     const std::array<std::uint8_t, 32>& signer_hash,
                                     std::uint32_t triple_bytes,
                                     std::uint32_t permutation_bytes,
                                     const PackageProfile& profile) {
  std::vector<std::uint8_t> out;
  out.reserve(kHeaderBytes);
  out.insert(out.end(), kMagic.begin(), kMagic.end());
  append_u32(out, profile.package_format_version);
  append_u32(out, 32);
  append_u32(out, 12);
  append_array(out, session.session);
  append_array(out, session.invocation);
  append_u32(out, session.n);
  append_u32(out, session.k);
  for (const auto& identity : session.party_identity_sha256) append_array(out, identity);
  for (const auto& key_hash : session.recipient_rsa_public_sha256) append_array(out, key_hash);
  append_array(out, session.escrow_identity_sha256);
  out.push_back(party);
  if (profile.package_format_version == kScoreToMaskV3PackageFormatVersion) {
    // F_T/v1 package format 4 is byte-for-byte unchanged.
    out.insert(out.end(), 7, 0);
  } else {
    out.push_back(profile.ft_version);
    out.push_back(profile.sampler_version);
    out.push_back(profile.sampler_security_parameter_bits);
    out.push_back(profile.sampler_retry_limit);
    out.insert(out.end(), 3, 0);
  }
  append_u64(out, budget.gate_count);
  append_u32(out, triple_bytes);
  append_u32(out, permutation_bytes);
  append_array(out, circuit);
  append_array(out, schedule);
  append_array(out, material_hash);
  append_array(out, signer_hash);
  if (out.size() != kHeaderBytes) fail("internal v3 package header size");
  return out;
}

struct ParsedHeader {
  ScoreToMaskV3Session session;
  std::uint32_t package_format_version = 0;
  std::uint8_t party = 0;
  std::uint8_t ft_version = 0;
  std::uint8_t sampler_version = 0;
  std::uint8_t sampler_security_parameter_bits = 0;
  std::uint8_t sampler_retry_limit = 0;
  std::uint64_t gate_count = 0;
  std::uint32_t triple_bytes = 0;
  std::uint32_t permutation_bytes = 0;
  std::array<std::uint8_t, 32> circuit{};
  std::array<std::uint8_t, 32> schedule{};
  std::array<std::uint8_t, 32> material{};
  std::array<std::uint8_t, 32> signer{};
};

ParsedHeader parse_header(const std::vector<std::uint8_t>& bytes) {
  if (bytes.size() < kHeaderBytes + kSignatureBytes ||
      !std::equal(kMagic.begin(), kMagic.end(), bytes.begin()))
    fail("v3 package magic/length");
  std::size_t at = kMagic.size();
  ParsedHeader header;
  header.package_format_version = read_u32(bytes, at);
  if (header.package_format_version != kScoreToMaskV3PackageFormatVersion
#if defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL) || defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL)
      && header.package_format_version != kScoreToMaskV3PackageFormatVersionFtV2
#endif
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
      && header.package_format_version != kScoreToMaskV3PackageFormatVersionFtV3
#endif
  )
    fail("v3 package format version");
  if (read_u32(bytes, at) != 32 || read_u32(bytes, at) != 12)
    fail("v3 package score format is not signed Q20.12");
  read_array(bytes, at, header.session.session);
  read_array(bytes, at, header.session.invocation);
  header.session.n = read_u32(bytes, at);
  header.session.k = read_u32(bytes, at);
  for (auto& identity : header.session.party_identity_sha256) read_array(bytes, at, identity);
  for (auto& key_hash : header.session.recipient_rsa_public_sha256) read_array(bytes, at, key_hash);
  read_array(bytes, at, header.session.escrow_identity_sha256);
  if (at + 8 > bytes.size()) fail("v3 package reserved fields truncated");
  header.party = bytes[at++];
  if (header.package_format_version == kScoreToMaskV3PackageFormatVersion) {
    for (int i = 0; i < 7; ++i)
      if (bytes[at++] != 0) fail("v3 header reserved bits");
    header.ft_version = 1;
    header.sampler_version = 0;
  } else {
#if defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL) || defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL)
    header.ft_version = bytes[at++];
    header.sampler_version = bytes[at++];
    header.sampler_security_parameter_bits = bytes[at++];
    header.sampler_retry_limit = bytes[at++];
    for (int i = 0; i < 3; ++i)
      if (bytes[at++] != 0) fail("F_T bounded package reserved bits");
#else
    fail("F_T bounded package format is not enabled in this build");
#endif
  }
  header.gate_count = read_u64(bytes, at);
  header.triple_bytes = read_u32(bytes, at);
  header.permutation_bytes = read_u32(bytes, at);
  read_array(bytes, at, header.circuit);
  read_array(bytes, at, header.schedule);
  read_array(bytes, at, header.material);
  read_array(bytes, at, header.signer);
  if (at != kHeaderBytes) fail("v3 header parse size");
  return header;
}

void check_session(const ScoreToMaskV3Session& actual,
                   const ScoreToMaskV3Session& expected) {
  if (actual.session != expected.session || actual.invocation != expected.invocation ||
      actual.n != expected.n || actual.k != expected.k ||
      actual.party_identity_sha256 != expected.party_identity_sha256 ||
      actual.recipient_rsa_public_sha256 != expected.recipient_rsa_public_sha256 ||
      actual.escrow_identity_sha256 != expected.escrow_identity_sha256)
    fail("v3 package session/invocation/N/K mismatch");
}

void validate_identity_bindings(const ScoreToMaskV3Session& session) {
  if (all_zero(session.session) || all_zero(session.invocation) ||
      all_zero(session.escrow_identity_sha256) ||
      std::any_of(session.party_identity_sha256.begin(), session.party_identity_sha256.end(),
                  [](const auto& value) { return all_zero(value); }) ||
      std::any_of(session.recipient_rsa_public_sha256.begin(),
                  session.recipient_rsa_public_sha256.end(),
                  [](const auto& value) { return all_zero(value); }))
    fail("v3 session, party, recipient-key, and escrow identities must be bound");
}

void sqlite_ok(int status, sqlite3* db, const char* what) {
  if (status != SQLITE_OK && status != SQLITE_DONE && status != SQLITE_ROW)
    fail(std::string(what) + ": " + (db ? sqlite3_errmsg(db) : sqlite3_errstr(status)));
}

}  // namespace

ScoreToMaskV3GateBudget score_to_mask_v3_gate_budget(std::uint32_t n) {
  if (n == 0 || n > kScoreToMaskV3MaxN) fail("v3 N must be in [1,256]");
  std::uint32_t padded = 2;
  while (padded < n) padded <<= 1U;
  std::uint32_t index_bits = 0;
  for (auto value = padded - 1U; value != 0; value >>= 1U)
    ++index_bits;
  const std::uint64_t width = 32U + index_bits;
  const std::uint64_t pairs = static_cast<std::uint64_t>(padded) * (padded - 1U) / 2U;
  const std::uint64_t gates = 63ULL * padded +
      static_cast<std::uint64_t>(padded) * padded * width +
      pairs * (2U * width - 1U) +
      static_cast<std::uint64_t>(padded) * padded;
  const auto triple_bytes = (3ULL * gates + 7U) / 8U;
  const auto permutation_bytes =
      (static_cast<std::uint64_t>(padded) * padded + 7U) / 8U;
  const auto package_bytes = kHeaderBytes + kSignatureBytes + triple_bytes +
                             permutation_bytes;
  if (gates > 5'281'664ULL || package_bytes > kScoreToMaskV3MaxPackageBytes)
    fail("v3 material resource budget exceeded: G=" + std::to_string(gates) +
         ", package_bytes=" + std::to_string(package_bytes));
  return {padded, index_bits, static_cast<std::uint32_t>(width), gates,
          package_bytes};
}

#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
ScoreToMaskV3GateBudget score_to_mask_v3_gate_budget_ft_v3(std::uint32_t n) {
  if (n == 0 || n > kScoreToMaskV3FtV3MaxN)
    fail("F_T/v3 N must be in [1,1000]");
  std::uint32_t padded = 2;
  while (padded < n) padded <<= 1U;
  if (padded > kScoreToMaskV3FtV3MaxPaddedN)
    fail("F_T/v3 padded N exceeds 1024");
  std::uint32_t index_bits = 0;
  for (auto value = padded - 1U; value != 0; value >>= 1U) ++index_bits;
  const std::uint64_t width = 32U + index_bits;
  const std::uint64_t pairs = static_cast<std::uint64_t>(padded) * (padded - 1U) / 2U;
  const std::uint64_t gates = 63ULL * padded +
      static_cast<std::uint64_t>(padded) * padded * width +
      pairs * (2U * width - 1U) +
      static_cast<std::uint64_t>(padded) * padded;
  const auto triple_bytes = (3ULL * gates + 7U) / 8U;
  const auto permutation_bytes =
      (static_cast<std::uint64_t>(padded) * padded + 7U) / 8U;
  const auto package_bytes = kHeaderBytes + kSignatureBytes + triple_bytes + permutation_bytes;
  if (gates > kScoreToMaskV3FtV3MaxGateCount ||
      package_bytes > kScoreToMaskV3FtV3MaxPackageBytes)
    fail("F_T/v3 material resource budget exceeded");
  return {padded, index_bits, static_cast<std::uint32_t>(width), gates,
          package_bytes};
}
#endif

namespace {
std::uint8_t package_ft_version(const ScoreToMaskV3PackageInfo& package) {
#if defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL) || defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL)
  return package.ft_version;
#else
  (void)package;
  return 1;
#endif
}
std::uint8_t context_ft_version(const ScoreToMaskV3EscrowContext& context) {
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
  return context.ft_version;
#else
  (void)context;
  return 1;
#endif
}
ScoreToMaskV3GateBudget budget_for_ft_version(std::uint32_t n,
                                               std::uint8_t ft_version) {
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
  if (ft_version == kScoreToMaskV3FtVersionV3)
    return score_to_mask_v3_gate_budget_ft_v3(n);
#endif
  if (ft_version == 1 || ft_version == 2)
    return score_to_mask_v3_gate_budget(n);
  fail("v3 unsupported F_T profile");
}
std::size_t package_limit_for_format(std::uint32_t format) {
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
  if (format == kScoreToMaskV3PackageFormatVersionFtV3)
    return kScoreToMaskV3FtV3MaxPackageBytes;
#endif
  return kScoreToMaskV3MaxPackageBytes;
}
std::array<std::uint8_t, 32> schedule_hash_for_ft_version(std::uint8_t version) {
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
  if (version == kScoreToMaskV3FtVersionV3) return schedule_hash_ft_v3();
#endif
  return schedule_hash();
}
}  // namespace

ProtocolIFrameConfig score_to_mask_v3_frame_config(
    const ScoreToMaskV3PackageInfo& package, std::uint8_t party) {
  if (party > 1 || package.party != party) fail("v3 frame party/package mismatch");
  const auto budget = budget_for_ft_version(package.parameters.n,
                                             package_ft_version(package));
  const auto session_id = first_u64(package.parameters.session);
  const auto invocation_id = first_u64(package.parameters.invocation);
  std::uint64_t fingerprint = 0;
  for (std::size_t i = 0; i < 8; ++i)
    fingerprint = (fingerprint << 8U) | package.circuit_sha256[i];
  if (session_id == 0 || invocation_id == 0 || fingerprint == 0)
    fail("v3 frame identity prefix must be nonzero");
  return {session_id, fingerprint, package.parameters.n, package.parameters.k,
          static_cast<std::uint8_t>(std::max<std::uint32_t>(34U, budget.key_bits)),
          party, static_cast<std::uint8_t>(1U - party), 1, 0x73, invocation_id};
}

ProtocolIFrameConfig score_to_mask_v3_escrow_frame_config(
    const ScoreToMaskV3EscrowContext& context, std::uint8_t party,
    bool server) {
  if (party > 1) fail("v3 escrow frame party invalid");
  const auto budget = budget_for_ft_version(context.n,
      context.format_version == kScoreToMaskV3EscrowContextFormatVersion ? 1 :
      context_ft_version(context));
  const auto session_id = first_u64(context.session);
  const auto invocation_id = first_u64(context.invocation);
  std::uint64_t fingerprint = 0;
  for (std::size_t i = 0; i < 8; ++i)
    fingerprint = (fingerprint << 8U) | context.circuit_sha256[i];
  if (session_id == 0 || invocation_id == 0 || fingerprint == 0)
    fail("v3 escrow frame identity prefix must be nonzero");
  return {session_id, fingerprint, context.n, context.k,
          static_cast<std::uint8_t>(std::max<std::uint32_t>(34U, budget.key_bits)),
          static_cast<std::uint8_t>(server ? 2U : party),
          static_cast<std::uint8_t>(server ? party : 2U), 1, 0x73, invocation_id};
}

std::array<std::uint8_t, 32> score_to_mask_v3_public_key_sha256(
    const std::string& public_key_pem) {
  FILE* file = std::fopen(public_key_pem.c_str(), "rb");
  if (file == nullptr) fail("cannot open recipient public key: " + public_key_pem);
  EVP_PKEY* raw = PEM_read_PUBKEY(file, nullptr, nullptr, nullptr);
  std::fclose(file);
  if (raw == nullptr) fail("cannot parse recipient public key PEM");
  std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> key(raw, EVP_PKEY_free);
  if (EVP_PKEY_base_id(key.get()) != EVP_PKEY_RSA || EVP_PKEY_bits(key.get()) < 2048)
    fail("v3 recipient envelope requires RSA >= 2048-bit key");
  return key_fingerprint(key.get());
}

namespace {

template <typename FillRandom, typename BeforeMaterial>
void dealer_issue_packages_impl(
    const ScoreToMaskV3Session& session, const std::string& package_p0_path,
    const std::string& package_p1_path,
    const std::string& signed_escrow_context_path,
    const std::string& dealer_ed25519_private_key_pem,
    const ScoreToMaskV3EscrowStateParameters& escrow_state,
    const PackageProfile& profile, FillRandom&& fill_random,
    BeforeMaterial&& before_material) {
  const auto budget = budget_for_ft_version(session.n, profile.ft_version);
  if (session.k == 0 || session.k > session.n) fail("v3 K outside [1,N]");
  validate_identity_bindings(session);
  if (package_p0_path == package_p1_path ||
      package_p0_path == signed_escrow_context_path ||
      package_p1_path == signed_escrow_context_path)
    fail("v3 party package and escrow context paths must be distinct");
  if (escrow_state.transaction_wait_timeout_ms == 0 ||
      escrow_state.transaction_wait_timeout_ms > 60000U ||
      escrow_state.max_ciphertext_bytes == 0 ||
      escrow_state.max_ciphertext_bytes > kScoreToMaskV3MaxCiphertextBytes ||
      escrow_state.pending_restart_policy != 1 ||
      !receipt_format_supported(escrow_state.receipt_format_version))
    fail("v3 escrow state parameters are unsupported");
  auto signer = read_key(dealer_ed25519_private_key_pem, true);
  const auto signer_fp = key_fingerprint(signer.get());
  const auto circuit = circuit_hash();
  const auto schedule = schedule_hash_for_ft_version(profile.ft_version);
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
  if (profile.ft_version == kScoreToMaskV3FtVersionV3 &&
      escrow_state.receipt_format_version != kScoreToMaskV3ReceiptFormatV2)
    fail("F_T/v3 requires receipt_format_version=2");
#endif
  const auto triple_bytes = static_cast<std::uint32_t>((3ULL * budget.gate_count + 7U) / 8U);
  const auto permutation_bytes = static_cast<std::uint32_t>(
      (static_cast<std::uint64_t>(budget.padded_n) * budget.padded_n + 7U) / 8U);
  before_material();
  std::array<std::vector<std::uint8_t>, 2> triples{
      std::vector<std::uint8_t>(triple_bytes), std::vector<std::uint8_t>(triple_bytes)};
  std::array<std::vector<std::uint8_t>, 2> permutations{
      std::vector<std::uint8_t>(permutation_bytes), std::vector<std::uint8_t>(permutation_bytes)};
  std::vector<std::uint8_t> randomness(static_cast<std::size_t>(budget.gate_count));
  fill_random(randomness.data(), randomness.size());
  for (std::uint64_t gate = 0; gate < budget.gate_count; ++gate) {
    const auto r = randomness[static_cast<std::size_t>(gate)];
    const bool a = (r & 1U) != 0;
    const bool b = (r & 2U) != 0;
    const bool a0 = (r & 4U) != 0;
    const bool b0 = (r & 8U) != 0;
    const bool c0 = (r & 16U) != 0;
    const bool bits[3][2] = {{a0, static_cast<bool>(a ^ a0)},
                             {b0, static_cast<bool>(b ^ b0)},
                             {c0, static_cast<bool>((a & b) ^ c0)}};
    for (std::uint64_t component = 0; component < 3; ++component)
      for (std::uint64_t party = 0; party < 2; ++party)
        set_bit(triples[party], 3ULL * gate + component, bits[component][party]);
  }
  std::vector<std::uint32_t> permutation(budget.padded_n);
  std::vector<std::uint8_t> permutation_random(permutation_bytes);
  fill_random(permutation_random.data(), permutation_random.size());
  for (std::uint32_t i = 0; i < budget.padded_n; ++i) permutation[i] = i;
  for (std::uint32_t i = budget.padded_n; i > 1; --i) {
    std::uint32_t selected = 0;
    if (profile.ft_version == 1) {
      selected = next_uniform(i);
    } else if (profile.ft_version == 2) {
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL
      const auto next_word = [&]() {
        std::uint32_t word = 0;
        fill_random(reinterpret_cast<std::uint8_t*>(&word), sizeof(word));
        return word;
      };
      if (!score_to_mask_v3_detail::next_uniform_bounded(
              i, profile.sampler_retry_limit, next_word, selected))
        fail("F_T/v2 ISSUANCE_ABORT: bounded rejection limit reached");
#else
      fail("F_T/v2 issuance is not enabled in this build");
#endif
    } else if (profile.ft_version == 3) {
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
      const auto next_word = [&]() {
        std::uint32_t word = 0;
        fill_random(reinterpret_cast<std::uint8_t*>(&word), sizeof(word));
        return word;
      };
      if (!score_to_mask_v3_detail::next_uniform_bounded(
              i, profile.sampler_retry_limit, next_word, selected))
        fail("F_T/v3 ISSUANCE_ABORT: bounded rejection limit reached");
#else
      fail("F_T/v3 issuance is not enabled in this build");
#endif
    } else {
      fail("unsupported F_T sampler profile");
    }
    std::swap(permutation[i - 1U], permutation[selected]);
  }
  for (std::uint32_t original = 0; original < budget.padded_n; ++original) {
    for (std::uint32_t row = 0; row < budget.padded_n; ++row) {
      const auto gate = static_cast<std::uint64_t>(row) * budget.padded_n + original;
      const bool secret = row == permutation[original];
      const bool share0 = (permutation_random[static_cast<std::size_t>(gate / 8U)] &
                           (1U << (gate % 8U))) != 0;
      set_bit(permutations[0], gate, share0);
      set_bit(permutations[1], gate, secret ^ share0);
    }
  }
  std::array<std::vector<std::uint8_t>, 2> materials;
  std::array<std::vector<std::uint8_t>, 2> packages;
  std::array<std::array<std::uint8_t, 32>, 2> material_hashes{};
  for (std::uint8_t party = 0; party < 2; ++party) {
    materials[party].reserve(triples[party].size() + permutations[party].size());
    materials[party].insert(materials[party].end(), triples[party].begin(), triples[party].end());
    materials[party].insert(materials[party].end(), permutations[party].begin(), permutations[party].end());
    material_hashes[party] = sha256(materials[party]);
    packages[party] = make_header(session, party, budget, circuit, schedule,
        material_hashes[party], signer_fp, triple_bytes, permutation_bytes,
        profile);
    packages[party].insert(packages[party].end(), materials[party].begin(), materials[party].end());
    std::string_view signature_domain = kScoreToMaskV3PartyPackageSignatureDomain;
    if (profile.ft_version == 2) {
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL
      signature_domain = kFtV2PartyPackageSignatureDomain;
#else
      fail("F_T/v2 signing is not enabled in this build");
#endif
    } else if (profile.ft_version == 3) {
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
      signature_domain = kFtV3PartyPackageSignatureDomain;
#else
      fail("F_T/v3 signing is not enabled in this build");
#endif
    }
    const auto signing_message = domain_separated_message(
        signature_domain, profile.package_format_version, packages[party]);
    const auto domain_signature = sign_bytes(signer.get(), signing_message);
    append_array(packages[party], domain_signature);
    if (packages[party].size() > package_limit_for_format(profile.package_format_version) ||
        packages[party].size() > budget.package_bytes_per_party)
      fail("v3 package exceeds signed resource budget");
  }
  ScoreToMaskV3EscrowContext escrow_context;
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
  if (profile.ft_version == kScoreToMaskV3FtVersionV3) {
    escrow_context.format_version = kScoreToMaskV3EscrowContextFormatVersionFtV3;
    escrow_context.ft_version = profile.ft_version;
    escrow_context.sampler_version = profile.sampler_version;
    escrow_context.sampler_security_parameter_bits =
        profile.sampler_security_parameter_bits;
    escrow_context.sampler_retry_limit = profile.sampler_retry_limit;
    escrow_context.package_format_version = profile.package_format_version;
  }
#endif
  escrow_context.session = session.session;
  escrow_context.invocation = session.invocation;
  escrow_context.n = session.n;
  escrow_context.k = session.k;
  escrow_context.party_identity_sha256 = session.party_identity_sha256;
  escrow_context.escrow_identity_sha256 = session.escrow_identity_sha256;
  escrow_context.circuit_sha256 = circuit;
  escrow_context.gate_schedule_sha256 = schedule;
  for (std::size_t party = 0; party < packages.size(); ++party)
    escrow_context.party_package_sha256[party] = sha256(packages[party]);
  escrow_context.package_set_sha256 = package_set_hash(
      escrow_context.party_package_sha256);
  escrow_context.signer_identity_sha256 = signer_fp;
  escrow_context.state = escrow_state;
  const auto context_file = make_signed_escrow_context(escrow_context, signer.get());
  durable_create(package_p0_path, packages[0]);
  durable_create(package_p1_path, packages[1]);
  durable_create(signed_escrow_context_path, context_file);
}

}  // namespace

void score_to_mask_v3_dealer_issue_packages(
    const ScoreToMaskV3Session& session, const std::string& package_p0_path,
    const std::string& package_p1_path,
    const std::string& signed_escrow_context_path,
    const std::string& dealer_ed25519_private_key_pem,
    const ScoreToMaskV3EscrowStateParameters& escrow_state) {
  const PackageProfile f_t_v1_profile{};
  dealer_issue_packages_impl(session, package_p0_path, package_p1_path,
      signed_escrow_context_path, dealer_ed25519_private_key_pem, escrow_state,
      f_t_v1_profile, [](std::uint8_t* out, std::size_t size) {
        random_bytes(out, size);
      }, [] {});
}

#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL
void score_to_mask_v3_dealer_issue_packages_ft_v2_bounded(
    const ScoreToMaskV3Session& session, const std::string& package_p0_path,
    const std::string& package_p1_path,
    const std::string& signed_escrow_context_path,
    const std::string& dealer_ed25519_private_key_pem,
    const std::string& private_issuance_registry_directory,
    const ScoreToMaskV3EscrowStateParameters& escrow_state) {
  const auto budget = score_to_mask_v3_gate_budget(session.n);
  const PackageProfile f_t_v2_profile{
      kScoreToMaskV3PackageFormatVersionFtV2,
      kScoreToMaskV3FtVersionV2,
      kScoreToMaskV3SamplerVersionBoundedV1,
      kScoreToMaskV3FtV2SecurityParameterBits,
      static_cast<std::uint8_t>(score_to_mask_v3_detail::bounded_retry_limit(
          kScoreToMaskV3FtV2SecurityParameterBits, budget.padded_n))};
  dealer_issue_packages_impl(session, package_p0_path, package_p1_path,
      signed_escrow_context_path, dealer_ed25519_private_key_pem, escrow_state,
      f_t_v2_profile, [](std::uint8_t* out, std::size_t size) {
        random_bytes(out, size);
      }, [&] {
        register_ft_v2_invocation_once(session,
                                       private_issuance_registry_directory);
      });
}

#ifdef MOE_TOPK_SCORE_TO_MASK_V3_FT_V2_TESTING
namespace test_only {
void score_to_mask_v3_dealer_issue_packages_ft_v2_with_test_random(
    const ScoreToMaskV3Session& session, const std::string& package_p0_path,
    const std::string& package_p1_path,
    const std::string& signed_escrow_context_path,
    const std::string& dealer_ed25519_private_key_pem,
    const std::string& private_issuance_registry_directory,
    const ScoreToMaskV3EscrowStateParameters& escrow_state,
    bool reject_once_then_accept, std::size_t* bytes_consumed,
    std::size_t* random_call_count, std::size_t* max_request_bytes) {
  if (bytes_consumed == nullptr || random_call_count == nullptr ||
      max_request_bytes == nullptr)
    fail("TEST_ONLY random counters are required");
  std::size_t bytes = 0;
  std::size_t calls = 0;
  std::size_t max_request = 0;
  std::size_t word_index = 0;
  const auto budget = score_to_mask_v3_gate_budget(session.n);
  const auto triple_bytes = static_cast<std::size_t>(budget.gate_count);
  const auto permutation_bytes = static_cast<std::size_t>(
      (static_cast<std::uint64_t>(budget.padded_n) * budget.padded_n + 7U) / 8U);
  const auto sampler_offset = triple_bytes + permutation_bytes;
  auto test_random = [&](std::uint8_t* out, std::size_t size) {
    while (size != 0) {
      const auto chunk = std::min<std::size_t>(size, 1U << 20);
      max_request = std::max(max_request, chunk);
      std::fill_n(out, chunk, 0);
      if (bytes >= sampler_offset) {
        if (chunk != sizeof(std::uint32_t))
          fail("TEST_ONLY sampler request was not one 32-bit word");
        std::uint32_t value = 0;
        if (word_index == 0) value = std::numeric_limits<std::uint32_t>::max();
        else if (word_index == 1 && reject_once_then_accept) value = 0;
        else if (word_index == 2 && reject_once_then_accept) value = 1;
        else if (word_index > 0 && !reject_once_then_accept) value = 0;
        else value = std::numeric_limits<std::uint32_t>::max();
        std::memcpy(out, &value, sizeof(value));
        ++word_index;
      }
      bytes += chunk;
      ++calls;
      out += chunk;
      size -= chunk;
    }
  };
  const auto f_t_v2_profile = PackageProfile{
      kScoreToMaskV3PackageFormatVersionFtV2,
      kScoreToMaskV3FtVersionV2,
      kScoreToMaskV3SamplerVersionBoundedV1,
      kScoreToMaskV3FtV2SecurityParameterBits,
      static_cast<std::uint8_t>(score_to_mask_v3_detail::bounded_retry_limit(
          kScoreToMaskV3FtV2SecurityParameterBits, budget.padded_n))};
  try {
    dealer_issue_packages_impl(session, package_p0_path, package_p1_path,
        signed_escrow_context_path, dealer_ed25519_private_key_pem, escrow_state,
        f_t_v2_profile, test_random, [&] {
          register_ft_v2_invocation_once(session,
                                         private_issuance_registry_directory);
        });
  } catch (...) {
    *bytes_consumed = bytes;
    *random_call_count = calls;
    *max_request_bytes = max_request;
    throw;
  }
  *bytes_consumed = bytes;
  *random_call_count = calls;
  *max_request_bytes = max_request;
}
}  // namespace test_only
#endif
#endif
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
void score_to_mask_v3_dealer_issue_packages_ft_v3_bounded(
    const ScoreToMaskV3Session& session, const std::string& package_p0_path,
    const std::string& package_p1_path,
    const std::string& signed_escrow_context_path,
    const std::string& dealer_ed25519_private_key_pem,
    const std::string& private_issuance_registry_directory,
    const ScoreToMaskV3EscrowStateParameters& escrow_state) {
  const auto budget = score_to_mask_v3_gate_budget_ft_v3(session.n);
  const PackageProfile profile{
      kScoreToMaskV3PackageFormatVersionFtV3,
      kScoreToMaskV3FtVersionV3,
      kScoreToMaskV3SamplerVersionBoundedV2,
      kScoreToMaskV3FtV3SecurityParameterBits,
      kScoreToMaskV3FtV3MaxRetryLimit};
  dealer_issue_packages_impl(session, package_p0_path, package_p1_path,
      signed_escrow_context_path, dealer_ed25519_private_key_pem, escrow_state,
      profile, [](std::uint8_t* out, std::size_t size) { random_bytes(out, size); },
      [&] { register_ft_v3_invocation_once(session, profile,
                                           private_issuance_registry_directory); });
}
#ifdef MOE_TOPK_SCORE_TO_MASK_V3_FT_V3_TESTING
namespace test_only {
void score_to_mask_v3_dealer_issue_packages_ft_v3_with_test_random(
    const ScoreToMaskV3Session& session, const std::string& package_p0_path,
    const std::string& package_p1_path,
    const std::string& signed_escrow_context_path,
    const std::string& dealer_ed25519_private_key_pem,
    const std::string& private_issuance_registry_directory,
    const ScoreToMaskV3EscrowStateParameters& escrow_state,
    std::size_t* bytes_consumed, std::size_t* random_call_count,
    std::size_t* max_request_bytes) {
  if (!bytes_consumed || !random_call_count || !max_request_bytes)
    fail("TEST_ONLY F_T/v3 counters are required");
  std::size_t bytes = 0, calls = 0, max_request = 0;
  std::size_t sampler_offset = 0, word_index = 0;
  const auto budget = score_to_mask_v3_gate_budget_ft_v3(session.n);
  const auto permutation_bytes = static_cast<std::size_t>(
      (static_cast<std::uint64_t>(budget.padded_n) * budget.padded_n + 7U) / 8U);
  sampler_offset = static_cast<std::size_t>(budget.gate_count) + permutation_bytes;
  auto test_random = [&](std::uint8_t* out, std::size_t size) {
    while (size != 0) {
      const auto chunk = std::min<std::size_t>(size, 1U << 20);
      std::fill_n(out, chunk, 0);
      max_request = std::max(max_request, chunk);
      if (bytes >= sampler_offset) {
        if (chunk != sizeof(std::uint32_t))
          fail("TEST_ONLY F_T/v3 sampler request was not one 32-bit word");
        const std::uint32_t word = 0;
        std::memcpy(out, &word, sizeof(word));
        ++word_index;
      }
      bytes += chunk;
      ++calls;
      out += chunk;
      size -= chunk;
    }
  };
  const PackageProfile profile{
      kScoreToMaskV3PackageFormatVersionFtV3,
      kScoreToMaskV3FtVersionV3,
      kScoreToMaskV3SamplerVersionBoundedV2,
      kScoreToMaskV3FtV3SecurityParameterBits,
      kScoreToMaskV3FtV3MaxRetryLimit};
  try {
    dealer_issue_packages_impl(session, package_p0_path, package_p1_path,
        signed_escrow_context_path, dealer_ed25519_private_key_pem, escrow_state,
        profile, test_random, [&] { register_ft_v3_invocation_once(session, profile,
            private_issuance_registry_directory); });
  } catch (...) {
    *bytes_consumed = bytes; *random_call_count = calls;
    *max_request_bytes = max_request; throw;
  }
  *bytes_consumed = bytes; *random_call_count = calls;
  *max_request_bytes = max_request;
  (void)word_index;
}
}  // namespace test_only
#endif
#endif

ScoreToMaskV3PackageInfo score_to_mask_v3_validate_package(
    const std::string& package_path,
    const std::string& dealer_ed25519_public_key_pem,
    const ScoreToMaskV3Session& expected, std::uint8_t expected_party) {
  if (expected_party > 1) fail("invalid v3 party id");
  const auto bytes = read_package_file(package_path);
  const auto header = parse_header(bytes);
  validate_identity_bindings(expected);
  check_session(header.session, expected);
  if (header.party != expected_party) fail("v3 package party identity mismatch");
  const auto budget = budget_for_ft_version(expected.n, header.ft_version);
  if (expected.k == 0 || expected.k > expected.n ||
      header.gate_count != budget.gate_count ||
      header.triple_bytes != (3ULL * budget.gate_count + 7U) / 8U ||
      header.permutation_bytes != (static_cast<std::uint64_t>(budget.padded_n) *
                                   budget.padded_n + 7U) / 8U)
    fail("v3 package gate/material count mismatch");
  if (header.package_format_version == kScoreToMaskV3PackageFormatVersion) {
    if (header.ft_version != 1 || header.sampler_version != 0 ||
        header.sampler_security_parameter_bits != 0 ||
        header.sampler_retry_limit != 0)
      fail("F_T/v1 package profile mismatch");
    if (expected.n > kScoreToMaskV3MaxN)
      fail("F_T/v1 N exceeds 256");
#if defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL) || defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL)
  } else if (header.package_format_version == kScoreToMaskV3PackageFormatVersionFtV2) {
    const auto retry = static_cast<std::uint8_t>(
        score_to_mask_v3_detail::bounded_retry_limit(
            kScoreToMaskV3FtV2SecurityParameterBits, budget.padded_n));
    if (header.ft_version != kScoreToMaskV3FtVersionV2 ||
        header.sampler_version != kScoreToMaskV3SamplerVersionBoundedV1 ||
        header.sampler_security_parameter_bits != kScoreToMaskV3FtV2SecurityParameterBits ||
        header.sampler_retry_limit != retry || expected.n > kScoreToMaskV3MaxN)
      fail("F_T/v2 bounded-rejection profile mismatch");
#endif
  } else {
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
    const auto minimum_retry = score_to_mask_v3_detail::bounded_retry_limit_ft_v3(
        kScoreToMaskV3FtV3SecurityParameterBits, budget.padded_n);
    if (header.package_format_version != kScoreToMaskV3PackageFormatVersionFtV3 ||
        header.ft_version != kScoreToMaskV3FtVersionV3 ||
        header.sampler_version != kScoreToMaskV3SamplerVersionBoundedV2 ||
        header.sampler_security_parameter_bits != kScoreToMaskV3FtV3SecurityParameterBits ||
        header.sampler_retry_limit != kScoreToMaskV3FtV3MaxRetryLimit ||
        minimum_retry > header.sampler_retry_limit ||
        expected.n > kScoreToMaskV3FtV3MaxN)
      fail("F_T/v3 bounded-rejection profile mismatch");
#else
    fail("F_T/v3 package format is not enabled in this build");
#endif
  }
  const auto expected_schedule = schedule_hash_for_ft_version(header.ft_version);
  if (header.circuit != circuit_hash() || header.schedule != expected_schedule)
    fail("v3 package circuit/gate schedule mismatch");
  const auto content_bytes = static_cast<std::size_t>(header.triple_bytes) +
                             header.permutation_bytes;
  const auto package_limit = package_limit_for_format(header.package_format_version);
  if (bytes.size() != kHeaderBytes + content_bytes + kSignatureBytes ||
      bytes.size() > package_limit)
    fail("v3 package exact length/resource limit mismatch");
  const auto content_hash = sha256(bytes.data() + kHeaderBytes, content_bytes);
  if (content_hash != header.material) fail("v3 package material digest mismatch");
  auto verifier = read_key(dealer_ed25519_public_key_pem, false);
  if (key_fingerprint(verifier.get()) != header.signer)
    fail("v3 package signer identity mismatch");
  const auto signed_size = bytes.size() - kSignatureBytes;
  const std::vector<std::uint8_t> signed_body(
      bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(signed_size));
  std::string_view signature_domain = kScoreToMaskV3PartyPackageSignatureDomain;
  if (header.ft_version == 2) {
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL
    signature_domain = kFtV2PartyPackageSignatureDomain;
#else
    fail("F_T/v2 package format is not enabled in this build");
#endif
  } else if (header.ft_version == 3) {
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
    signature_domain = kFtV3PartyPackageSignatureDomain;
#else
    fail("F_T/v3 package format is not enabled in this build");
#endif
  }
  const auto signing_message = domain_separated_message(
      signature_domain, header.package_format_version, signed_body);
  if (!verify_bytes(verifier.get(), signing_message,
                    bytes.data() + signed_size, kSignatureBytes))
    fail("v3 package signature mismatch");
  ScoreToMaskV3PackageInfo info;
  info.parameters = expected;
  info.party = expected_party;
#if defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL) || defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL)
  info.package_format_version = header.package_format_version;
  info.ft_version = header.ft_version;
  info.sampler_version = header.sampler_version;
  info.sampler_security_parameter_bits = header.sampler_security_parameter_bits;
  info.sampler_retry_limit = header.sampler_retry_limit;
#endif
  info.circuit_sha256 = header.circuit;
  info.gate_schedule_sha256 = header.schedule;
  info.package_sha256 = sha256(bytes);
  info.gate_count = header.gate_count;
  info.package_bytes = bytes.size();
  return info;
}

ScoreToMaskV3EscrowContext score_to_mask_v3_validate_escrow_context(
    const std::string& signed_context_path,
    const std::string& dealer_ed25519_public_key_pem) {
  const auto bytes = read_file(signed_context_path, kScoreToMaskV3MaxMetadataBytes);
  constexpr std::size_t kFixedV1BodyBytes = 8 + 4 + 4 + 16 + 16 + 4 + 4 +
      64 + 32 + 32 + 32 + 64 + 32 + 32 + 4 + 4 + 4;
  if (bytes.size() < kFixedV1BodyBytes + kSignatureBytes ||
      !std::equal(kEscrowContextMagic.begin(), kEscrowContextMagic.end(), bytes.begin()))
    fail("v3 escrow context magic/length mismatch");
  std::size_t at = kEscrowContextMagic.size();
  ScoreToMaskV3EscrowContext context;
  context.format_version = read_u32(bytes, at);
  if (read_u32(bytes, at) != kScoreToMaskV3ProtocolVersion)
    fail("v3 escrow context protocol version mismatch");
  if (context.format_version == kScoreToMaskV3EscrowContextFormatVersion) {
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
    context.ft_version = kScoreToMaskV3FtVersionV1;
#endif
  }
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
  else if (context.format_version == kScoreToMaskV3EscrowContextFormatVersionFtV3) {
    if (at + 8 > bytes.size()) fail("F_T/v3 context profile truncated");
    context.ft_version = bytes[at++];
    context.sampler_version = bytes[at++];
    context.sampler_security_parameter_bits = bytes[at++];
    context.sampler_retry_limit = bytes[at++];
    context.package_format_version = read_u32(bytes, at);
  }
#endif
  else {
    fail("v3 escrow context format is unavailable in this build");
  }
  const auto expected_body_size = kFixedV1BodyBytes +
      (context.format_version == kScoreToMaskV3EscrowContextFormatVersion ? 0U : 8U);
  if (bytes.size() != expected_body_size + kSignatureBytes)
    fail("v3 escrow context exact length mismatch");
  const auto body_size = bytes.size() - kSignatureBytes;
  const std::vector<std::uint8_t> body(bytes.begin(),
      bytes.begin() + static_cast<std::ptrdiff_t>(body_size));
  read_array(bytes, at, context.session);
  read_array(bytes, at, context.invocation);
  context.n = read_u32(bytes, at);
  context.k = read_u32(bytes, at);
  for (auto& identity : context.party_identity_sha256) read_array(bytes, at, identity);
  read_array(bytes, at, context.escrow_identity_sha256);
  read_array(bytes, at, context.circuit_sha256);
  read_array(bytes, at, context.gate_schedule_sha256);
  for (auto& package_hash : context.party_package_sha256) read_array(bytes, at, package_hash);
  read_array(bytes, at, context.package_set_sha256);
  read_array(bytes, at, context.signer_identity_sha256);
  context.state.transaction_wait_timeout_ms = read_u32(bytes, at);
  context.state.max_ciphertext_bytes = read_u32(bytes, at);
  if (at + 4 != body_size || bytes[at] != 1 ||
      !receipt_format_supported(bytes[at + 1]) || bytes[at + 2] != 0 || bytes[at + 3] != 0)
    fail("v3 escrow context state-machine parameters unsupported");
  context.state.pending_restart_policy = bytes[at++];
  context.state.receipt_format_version = bytes[at++];
  at += 2;
  if (at != body_size) fail("v3 escrow context body length mismatch");
  std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(body_size),
              context.signature.size(), context.signature.begin());

  ScoreToMaskV3GateBudget budget;
  std::array<std::uint8_t, 32> expected_schedule{};
  if (context.format_version == kScoreToMaskV3EscrowContextFormatVersion) {
    if (context.n == 0 || context.n > kScoreToMaskV3MaxN)
      fail("F_T/v1 context N exceeds 256");
    budget = score_to_mask_v3_gate_budget(context.n);
    expected_schedule = schedule_hash();
  }
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
  else {
    if (context.ft_version != kScoreToMaskV3FtVersionV3 ||
        context.sampler_version != kScoreToMaskV3SamplerVersionBoundedV2 ||
        context.sampler_security_parameter_bits != kScoreToMaskV3FtV3SecurityParameterBits ||
        context.package_format_version != kScoreToMaskV3PackageFormatVersionFtV3 ||
        context.state.receipt_format_version != kScoreToMaskV3ReceiptFormatV2)
      fail("F_T/v3 context profile/receipt mismatch");
    budget = score_to_mask_v3_gate_budget_ft_v3(context.n);
    const auto minimum_retry = score_to_mask_v3_detail::bounded_retry_limit_ft_v3(
        kScoreToMaskV3FtV3SecurityParameterBits, budget.padded_n);
    if (context.sampler_retry_limit != kScoreToMaskV3FtV3MaxRetryLimit ||
        minimum_retry > context.sampler_retry_limit)
      fail("F_T/v3 context retry limit mismatch");
    expected_schedule = schedule_hash_ft_v3();
  }
#endif
  if (context.k == 0 || context.k > context.n ||
      all_zero(context.session) || all_zero(context.invocation) ||
      all_zero(context.escrow_identity_sha256) ||
      std::any_of(context.party_identity_sha256.begin(), context.party_identity_sha256.end(),
                  [](const auto& value) { return all_zero(value); }) ||
      std::any_of(context.party_package_sha256.begin(), context.party_package_sha256.end(),
                  [](const auto& value) { return all_zero(value); }) ||
      context.circuit_sha256 != circuit_hash() ||
      context.gate_schedule_sha256 != expected_schedule ||
      context.package_set_sha256 != package_set_hash(context.party_package_sha256) ||
      context.state.transaction_wait_timeout_ms == 0 ||
      context.state.transaction_wait_timeout_ms > 60000U ||
      context.state.max_ciphertext_bytes == 0 ||
      context.state.max_ciphertext_bytes > kScoreToMaskV3MaxCiphertextBytes ||
      !receipt_format_supported(context.state.receipt_format_version) ||
      all_zero(context.signer_identity_sha256))
    fail("v3 escrow context fields invalid");
  auto verifier = read_key(dealer_ed25519_public_key_pem, false);
  if (key_fingerprint(verifier.get()) != context.signer_identity_sha256)
    fail("v3 escrow context signer identity mismatch");
  const auto signing_message = domain_separated_message(
      kScoreToMaskV3EscrowContextSignatureDomain, context.format_version, body);
  if (!verify_bytes(verifier.get(), signing_message,
                    bytes.data() + body_size, kSignatureBytes))
    fail("v3 escrow context signature mismatch");
  context.context_sha256 = sha256(bytes);
  (void)budget;
  return context;
}

ScoreToMaskV3BeaverTripleShare ScoreToMaskV3LocalMaterial::triple(
    std::uint64_t gate_id) const {
  if (gate_id >= info.gate_count) fail("v3 gate id outside package");
  const auto base = 3ULL * gate_id;
  return {static_cast<std::uint8_t>(bit_at(packed_triple_bits, base)),
          static_cast<std::uint8_t>(bit_at(packed_triple_bits, base + 1U)),
          static_cast<std::uint8_t>(bit_at(packed_triple_bits, base + 2U))};
}

std::uint8_t ScoreToMaskV3LocalMaterial::permutation_share(
    std::uint32_t row, std::uint32_t original) const {
  const auto budget = budget_for_ft_version(info.parameters.n,
                                             package_ft_version(info));
  if (row >= budget.padded_n || original >= budget.padded_n)
    fail("v3 permutation coordinate outside package");
  const auto index = static_cast<std::uint64_t>(row) * budget.padded_n + original;
  return static_cast<std::uint8_t>(bit_at(packed_permutation_bits, index));
}

ScoreToMaskV3LocalMaterial score_to_mask_v3_open_material(
    const std::string& package_path,
    const std::string& dealer_ed25519_public_key_pem,
    const ScoreToMaskV3Session& expected, std::uint8_t expected_party) {
  ScoreToMaskV3LocalMaterial material;
  material.package_path = package_path;
  material.info = score_to_mask_v3_validate_package(package_path,
      dealer_ed25519_public_key_pem, expected, expected_party);
  const auto bytes = read_package_file(package_path);
  if (sha256(bytes) != material.info.package_sha256)
    fail("v3 package changed after verification");
  const auto header = parse_header(bytes);
  const auto triple_begin = kHeaderBytes;
  const auto permutation_begin = triple_begin + header.triple_bytes;
  material.packed_triple_bits.assign(
      bytes.begin() + static_cast<std::ptrdiff_t>(triple_begin),
      bytes.begin() + static_cast<std::ptrdiff_t>(permutation_begin));
  material.packed_permutation_bits.assign(
      bytes.begin() + static_cast<std::ptrdiff_t>(permutation_begin),
      bytes.begin() + static_cast<std::ptrdiff_t>(permutation_begin + header.permutation_bytes));
  return material;
}

void score_to_mask_v3_consume_package_lease(
    const std::string& package_path, const std::string& lease_directory,
    const ScoreToMaskV3PackageInfo& package) {
  std::error_code ec;
  std::filesystem::create_directories(lease_directory, ec);
  if (ec) fail("v3 lease directory create failed: " + ec.message());
  const auto lease_db = (std::filesystem::path(lease_directory) / "leases.sqlite3").string();
  sqlite3* raw_db = nullptr;
  sqlite_ok(sqlite3_open_v2(lease_db.c_str(), &raw_db,
      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr), raw_db,
      "open v3 lease database");
  std::unique_ptr<sqlite3, decltype(&sqlite3_close)> db(raw_db, sqlite3_close);
  sqlite3_busy_timeout(db.get(), 5000);
  sqlite_ok(sqlite3_exec(db.get(),
      "PRAGMA journal_mode=WAL;PRAGMA synchronous=FULL;"
      "CREATE TABLE IF NOT EXISTS consumed(package_hash BLOB PRIMARY KEY CHECK(length(package_hash)=32),"
      "session BLOB NOT NULL, invocation BLOB NOT NULL, party INTEGER NOT NULL, consumed_utc TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);",
      nullptr, nullptr, nullptr), db.get(), "initialize v3 lease database");
  sqlite_ok(sqlite3_exec(db.get(), "BEGIN IMMEDIATE", nullptr, nullptr, nullptr),
            db.get(), "begin v3 lease transaction");
  sqlite3_stmt* statement = nullptr;
  sqlite_ok(sqlite3_prepare_v2(db.get(),
      "INSERT INTO consumed(package_hash,session,invocation,party) VALUES(?1,?2,?3,?4)",
      -1, &statement, nullptr), db.get(), "prepare v3 lease insertion");
  std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> stmt(statement, sqlite3_finalize);
  sqlite_ok(sqlite3_bind_blob(stmt.get(), 1, package.package_sha256.data(), 32, SQLITE_TRANSIENT),
            db.get(), "bind v3 lease package hash");
  sqlite_ok(sqlite3_bind_blob(stmt.get(), 2, package.parameters.session.data(), 16, SQLITE_TRANSIENT),
            db.get(), "bind v3 lease session");
  sqlite_ok(sqlite3_bind_blob(stmt.get(), 3, package.parameters.invocation.data(), 16, SQLITE_TRANSIENT),
            db.get(), "bind v3 lease invocation");
  sqlite_ok(sqlite3_bind_int(stmt.get(), 4, package.party), db.get(), "bind v3 lease party");
  const int inserted = sqlite3_step(stmt.get());
  if (inserted != SQLITE_DONE) {
    sqlite3_exec(db.get(), "ROLLBACK", nullptr, nullptr, nullptr);
    if (inserted == SQLITE_CONSTRAINT) fail("v3 package already leased; no retry/fallback");
    sqlite_ok(inserted, db.get(), "consume v3 package lease");
  }
  sqlite_ok(sqlite3_exec(db.get(), "COMMIT", nullptr, nullptr, nullptr),
            db.get(), "commit v3 package lease");
  const auto bytes = read_package_file(package_path);
  if (sha256(bytes) != package.package_sha256)
    fail("v3 package changed after validation; lease remains burned");
}

}  // namespace moe_topk
