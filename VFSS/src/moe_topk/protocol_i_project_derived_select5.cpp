#include <moe_topk/protocol_i_project_derived_select5.h>

#include <FSS/prng.h>
#include <moe_topk/protocol_i_material_slot_store.h>
#include <moe_topk/protocol_i_score_input.h>
#include <moe_topk/protocol_i_transport.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <map>
#include <mutex>
#include <openssl/evp.h>
#include <poll.h>
#include <set>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <thread>
#include <tuple>
#include <unistd.h>
#include <utility>

namespace moe_topk {
namespace {

constexpr std::uint32_t kSelectPurpose = 1;
constexpr std::uint32_t kSelectDisjointSubstageScope = 0x010203U;
constexpr std::uint32_t kSelectLeftStrictRight = 1;
constexpr std::uint32_t kMembershipPurpose = 2;
constexpr std::uint32_t kMembershipSubstage = 4;
constexpr std::uint32_t kMembershipTauStrictItem = 2;
constexpr std::uint32_t kMembershipIteration = 5;
constexpr std::uint32_t kScorePurpose = 3;
constexpr std::uint32_t kShuffleMetadataPurpose = 4;
constexpr std::uint32_t kScoreKeyRefreshPurpose = 5;
constexpr std::uint32_t kShuffleForwardDirection = 0;
constexpr std::uint32_t kShuffleInverseDirection = 1;
constexpr std::uint32_t kScoreBits = 34;
constexpr std::uint8_t kFrameType = 9;
constexpr std::uint8_t kForwardPhase1 = 20;
constexpr std::uint8_t kForwardPhase2 = 21;
constexpr std::uint8_t kInversePhase1 = 22;
constexpr std::uint8_t kInversePhase2 = 23;
constexpr std::uint8_t kMembershipPhase = 60;
constexpr std::uint8_t kFinalStatusPhase = 61;
constexpr std::uint8_t kFinalCommitPhase = 62;
constexpr std::uint8_t kFinalAbortPhase = 63;
constexpr std::uint8_t kSampleSchedulePhaseBase = 40;
constexpr std::uint32_t kSampleSeedAckMagic = UINT32_C(0x5335414b);  // S5AK
constexpr std::uint32_t kSampleSeedContractVersion = 3;
constexpr std::size_t kReadyBytes = 88;

using Bytes = std::vector<std::uint8_t>;
using EdgeKey = std::pair<std::uint32_t, std::uint32_t>;
using MaterialClaim = std::tuple<std::uint64_t, std::uint64_t, std::uint64_t,
                                 std::uint8_t>;
std::mutex material_claim_mutex;
std::set<MaterialClaim> material_claims;

struct PackageDigest {
  std::uint64_t size = 0;
  std::array<std::uint8_t, 32> sha256{};
};

struct ReadyRecord {
  std::uint64_t session = 0;
  std::uint64_t fingerprint = 0;
  std::uint64_t material_id = 0;
  std::uint32_t logical_n = 0;
  std::uint32_t padded_n = 0;
  std::uint32_t k = 0;
  std::uint8_t bits = 0;
  std::uint8_t party = 0;
  PackageDigest package{};
};

void require(bool ok, const char* message) {
  if (!ok) throw std::invalid_argument(message);
}

void append_checked(Bytes&, const std::uint8_t*, std::size_t);
void put_u32(Bytes&, std::uint32_t);
void put_u64(Bytes&, std::uint64_t);
void get_bytes(const Bytes&, std::size_t&, std::uint8_t*, std::size_t);
std::uint32_t get_u32(const Bytes&, std::size_t&);
std::uint64_t get_u64(const Bytes&, std::size_t&);

std::string store_stem(std::uint64_t session, std::uint64_t fingerprint,
                       std::uint64_t material_id) {
  char buffer[80]{};
  const int count = std::snprintf(buffer, sizeof(buffer),
      "m6b5_%016llx_%016llx_%016llx",
      static_cast<unsigned long long>(session),
      static_cast<unsigned long long>(fingerprint),
      static_cast<unsigned long long>(material_id));
  require(count > 0 && static_cast<std::size_t>(count) < sizeof(buffer),
          "Select5 material store identity");
  return buffer;
}

std::string party_state_name(const ProtocolIProjectDerivedSelect5RuntimeConfig& c,
                             const char* suffix) {
  return store_stem(c.session, c.fingerprint, c.material_id_base) +
      "_p" + std::to_string(c.party) + suffix;
}

int open_store_directory(const std::string& path) {
  require(!path.empty(), "Select5 material store path");
  const int fd = ::open(path.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  if (fd < 0) throw std::runtime_error("Select5 material store directory open");
  struct stat status{};
  if (::fstat(fd, &status) != 0 || !S_ISDIR(status.st_mode) ||
      status.st_uid != ::geteuid() || (status.st_mode & 0077) != 0) {
    ::close(fd);
    throw std::runtime_error("Select5 material store directory owner/mode");
  }
  return fd;
}

void write_all_fd(int fd, const std::uint8_t* data, std::size_t size) {
  while (size != 0) {
    const auto count = ::write(fd, data, size);
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) throw std::runtime_error("Select5 material store write");
    data += count;
    size -= static_cast<std::size_t>(count);
  }
}

void read_all_fd(int fd, std::uint8_t* data, std::size_t size) {
  while (size != 0) {
    const auto count = ::read(fd, data, size);
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) throw std::runtime_error("Select5 material store read");
    data += count;
    size -= static_cast<std::size_t>(count);
  }
}

void pread_all_fd(int fd, std::uint8_t* data, std::size_t size,
                  std::uint64_t offset) {
  while (size != 0) {
    const auto count = ::pread(fd, data, size, static_cast<off_t>(offset));
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) throw std::runtime_error("Select5 material store pread");
    data += count;
    size -= static_cast<std::size_t>(count);
    offset += static_cast<std::uint64_t>(count);
  }
}

PackageDigest digest_fd(int fd) {
  struct stat status{};
  require(::fstat(fd, &status) == 0 && S_ISREG(status.st_mode) &&
              status.st_uid == ::geteuid() && status.st_size >= 0 &&
              static_cast<std::uint64_t>(status.st_size) <=
                  kProtocolIProjectDerivedSelect5MaxPackageBytes,
          "Select5 material package file size/type");
  PackageDigest result;
  result.size = static_cast<std::uint64_t>(status.st_size);
  EVP_MD_CTX* context = EVP_MD_CTX_new();
  if (context == nullptr) throw std::runtime_error("Select5 SHA-256 context");
  const bool initialized = EVP_DigestInit_ex(context, EVP_sha256(), nullptr) == 1;
  std::array<std::uint8_t, 65536> buffer{};
  std::uint64_t offset = 0;
  bool ok = initialized;
  while (ok && offset < result.size) {
    const auto want = static_cast<std::size_t>(std::min<std::uint64_t>(
        buffer.size(), result.size - offset));
    const auto count = ::pread(fd, buffer.data(), want, static_cast<off_t>(offset));
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) { ok = false; break; }
    ok = EVP_DigestUpdate(context, buffer.data(),
                          static_cast<std::size_t>(count)) == 1;
    offset += static_cast<std::uint64_t>(count);
  }
  unsigned int digest_size = 0;
  ok = ok && EVP_DigestFinal_ex(context, result.sha256.data(), &digest_size) == 1 &&
       digest_size == result.sha256.size();
  EVP_MD_CTX_free(context);
  if (!ok) throw std::runtime_error("Select5 material package SHA-256");
  return result;
}

Bytes encode_ready(const ReadyRecord& record) {
  Bytes output{'M', '6', 'P', '2'};
  put_u32(output, 1);
  put_u64(output, record.session);
  put_u64(output, record.fingerprint);
  put_u64(output, record.material_id);
  put_u32(output, record.logical_n);
  put_u32(output, record.padded_n);
  put_u32(output, record.k);
  output.push_back(record.bits);
  output.push_back(record.party);
  output.insert(output.end(), 2, 0);
  put_u64(output, record.package.size);
  append_checked(output, record.package.sha256.data(),
                 record.package.sha256.size());
  require(output.size() == kReadyBytes, "Select5 ready marker encoding");
  return output;
}

ReadyRecord decode_ready(const Bytes& bytes) {
  require(bytes.size() == kReadyBytes && bytes[0] == 'M' && bytes[1] == '6' &&
              bytes[2] == 'P' && bytes[3] == '2',
          "Select5 ready marker header");
  std::size_t offset = 4;
  require(get_u32(bytes, offset) == 1, "Select5 ready marker version");
  ReadyRecord record;
  record.session = get_u64(bytes, offset);
  record.fingerprint = get_u64(bytes, offset);
  record.material_id = get_u64(bytes, offset);
  record.logical_n = get_u32(bytes, offset);
  record.padded_n = get_u32(bytes, offset);
  record.k = get_u32(bytes, offset);
  record.bits = bytes[offset++];
  record.party = bytes[offset++];
  require(record.party <= 1 && bytes[offset++] == 0 && bytes[offset++] == 0,
          "Select5 ready marker reserved bytes");
  record.package.size = get_u64(bytes, offset);
  require(record.package.size <= kProtocolIProjectDerivedSelect5MaxPackageBytes,
          "Select5 ready package cap");
  get_bytes(bytes, offset, record.package.sha256.data(),
            record.package.sha256.size());
  require(offset == bytes.size(), "Select5 ready marker trailing bytes");
  return record;
}

void sync_file_and_directory(int file_fd, int directory_fd) {
  if (::fsync(file_fd) != 0 || ::fsync(directory_fd) != 0)
    throw std::runtime_error("Select5 durable material store fsync");
}

void write_new_file(int directory_fd, const std::string& name,
                    const std::uint8_t* bytes, std::size_t size,
                    mode_t final_mode) {
  const int fd = ::openat(directory_fd, name.c_str(),
      O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
  if (fd < 0) throw std::runtime_error("Select5 material file already exists or cannot open");
  try {
    write_all_fd(fd, bytes, size);
    if (::fchmod(fd, final_mode) != 0)
      throw std::runtime_error("Select5 material file permissions");
    sync_file_and_directory(fd, directory_fd);
    ::close(fd);
  } catch (...) {
    ::close(fd);
    throw;
  }
}

void durable_session_claim(
    const ProtocolIProjectDerivedSelect5RuntimeConfig& c) {
  const int dirfd = open_store_directory(c.material_store_directory);
  Bytes marker{'M', '6', 'X', '1'};
  put_u32(marker, 1);
  put_u64(marker, c.session);
  put_u64(marker, c.fingerprint);
  put_u64(marker, c.material_id_base);
  marker.push_back(c.party);
  marker.insert(marker.end(), 7, 0);
  try {
    write_new_file(dirfd, party_state_name(c, ".run"), marker.data(),
                   marker.size(), 0400);
    ::close(dirfd);
  } catch (...) {
    ::close(dirfd);
    throw;
  }
}

void durable_output_phase(
    const ProtocolIProjectDerivedSelect5RuntimeConfig& c,
    const char* phase, const char* predecessor) {
  const int dirfd = open_store_directory(c.material_store_directory);
  try {
    if (predecessor != nullptr) {
      const auto previous = party_state_name(c, predecessor);
      const int previous_fd = ::openat(dirfd, previous.c_str(),
          O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
      require(previous_fd >= 0,
              "Select5 output transaction predecessor absent");
      ::close(previous_fd);
    }
    const auto name = party_state_name(c, phase);
    Bytes marker{'M', '6', 'O', '1'};
    put_u32(marker, 1);
    put_u64(marker, c.session);
    put_u64(marker, c.fingerprint);
    put_u64(marker, c.material_id_base);
    marker.push_back(c.party);
    marker.insert(marker.end(), 7, 0);
    write_new_file(dirfd, name, marker.data(), marker.size(), 0400);
    ::close(dirfd);
  } catch (...) {
    ::close(dirfd);
    throw;
  }
}

void test_crash_after_output_step(
    const ProtocolIProjectDerivedSelect5RuntimeConfig& c,
    std::uint8_t step) noexcept {
  if (c.test_crash_after_output_step == step)
    ::_exit(static_cast<int>(80U + step));
}

void trace_test_wire_event(
    const ProtocolIProjectDerivedSelect5RuntimeConfig& c,
    const char* event, std::uint8_t phase, std::uint8_t iteration,
    std::size_t payload_bytes = 0) {
  if (c.test_wire_trace_path.empty()) return;
  char line[160]{};
  const int length = std::snprintf(line, sizeof(line),
      "%s phase=%u iteration=%u bytes=%zu\n", event,
      static_cast<unsigned>(phase), static_cast<unsigned>(iteration),
      payload_bytes);
  require(length > 0 && static_cast<std::size_t>(length) < sizeof(line),
          "Select5 TEST_ONLY trace event shape");
  const int fd = ::open(c.test_wire_trace_path.c_str(),
      O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC | O_NOFOLLOW, 0600);
  if (fd < 0) throw std::runtime_error("Select5 TEST_ONLY trace file open");
  try {
    write_all_fd(fd, reinterpret_cast<const std::uint8_t*>(line),
                 static_cast<std::size_t>(length));
    if (::fsync(fd) != 0) throw std::runtime_error("Select5 trace fsync");
    ::close(fd);
  } catch (...) {
    ::close(fd);
    throw;
  }
}

Bytes read_exact_at(int directory_fd, const std::string& name,
                    std::size_t expected_size) {
  const int fd = ::openat(directory_fd, name.c_str(),
      O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
  if (fd < 0) throw std::runtime_error("Select5 material file missing");
  try {
    struct stat status{};
    require(::fstat(fd, &status) == 0 && S_ISREG(status.st_mode) &&
                status.st_size == static_cast<off_t>(expected_size),
            "Select5 material file shape");
    Bytes bytes(expected_size);
    read_all_fd(fd, bytes.data(), bytes.size());
    ::close(fd);
    return bytes;
  } catch (...) {
    ::close(fd);
    throw;
  }
}

std::uint64_t mask_for(unsigned bits) {
  require(bits >= 1 && bits <= 63, "Select5 ring width");
  return (UINT64_C(1) << bits) - 1U;
}

std::uint32_t padded_size(std::uint32_t logical_n) {
  require(logical_n >= 1 && logical_n <= kProtocolIProjectDerivedSelect5MaxN,
          "Select5 logical n outside experimental range");
  std::uint32_t result = 2;
  while (result < logical_n) result <<= 1U;
  return result;
}

std::uint8_t index_bits_for(std::uint32_t padded_n) {
  std::uint8_t bits = 0;
  for (auto value = padded_n - 1U; value != 0; value >>= 1U) ++bits;
  return std::max<std::uint8_t>(1, bits);
}

std::uint64_t edge_count(std::uint32_t n) {
  return static_cast<std::uint64_t>(n) * (n - 1U) / 2U;
}

std::uint64_t pair_index(std::uint32_t n, std::uint32_t left,
                         std::uint32_t right) {
  require(left < right && right < n, "Select5 edge address");
  return static_cast<std::uint64_t>(left) * (2U * n - left - 1U) / 2U +
         (right - left - 1U);
}

void random_bytes(void* output, std::size_t size) {
  auto* cursor = static_cast<std::uint8_t*>(output);
  while (size != 0) {
    const auto count = ::getrandom(cursor, size, 0);
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) throw std::runtime_error("Select5 operating-system entropy");
    cursor += count;
    size -= static_cast<std::size_t>(count);
  }
}

std::uint64_t random_u64() {
  std::uint64_t result = 0;
  random_bytes(&result, sizeof(result));
  return result;
}

std::uint64_t random_bounded(std::uint64_t bound) {
  require(bound != 0, "Select5 random bound");
  const auto threshold = static_cast<std::uint64_t>(-bound) % bound;
  for (;;) {
    const auto value = random_u64();
    if (value >= threshold) return value % bound;
  }
}

ProtocolIPermutation fresh_permutation(std::uint32_t n) {
  auto result = protocol_i_identity_permutation(n);
  for (std::uint32_t remaining = n; remaining > 1; --remaining) {
    const auto selected = static_cast<std::uint32_t>(random_bounded(remaining));
    std::swap(result[remaining - 1U], result[selected]);
  }
  return result;
}

void append_checked(Bytes& out, const std::uint8_t* data, std::size_t size) {
  require(size <= kProtocolIProjectDerivedSelect5MaxPackageBytes &&
              out.size() <= kProtocolIProjectDerivedSelect5MaxPackageBytes - size,
          "Select5 T package exceeds 64 MiB cap");
  out.insert(out.end(), data, data + size);
}

void put_u32(Bytes& out, std::uint32_t value) {
  const std::uint8_t bytes[4] = {
      static_cast<std::uint8_t>(value >> 24U),
      static_cast<std::uint8_t>(value >> 16U),
      static_cast<std::uint8_t>(value >> 8U),
      static_cast<std::uint8_t>(value)};
  append_checked(out, bytes, sizeof(bytes));
}

void put_u64(Bytes& out, std::uint64_t value) {
  std::uint8_t bytes[8];
  for (std::size_t i = 0; i < 8; ++i)
    bytes[i] = static_cast<std::uint8_t>(value >> (56U - 8U * i));
  append_checked(out, bytes, sizeof(bytes));
}

void put_blob(Bytes& out, const Bytes& blob) {
  put_u64(out, blob.size());
  append_checked(out, blob.data(), blob.size());
}

void get_bytes(const Bytes& input, std::size_t& offset, std::uint8_t* out,
               std::size_t size) {
  require(offset <= input.size() && size <= input.size() - offset,
          "Select5 truncated T package");
  if (size != 0) std::memcpy(out, input.data() + offset, size);
  offset += size;
}

std::uint32_t get_u32(const Bytes& input, std::size_t& offset) {
  std::uint8_t bytes[4];
  get_bytes(input, offset, bytes, sizeof(bytes));
  return (static_cast<std::uint32_t>(bytes[0]) << 24U) |
         (static_cast<std::uint32_t>(bytes[1]) << 16U) |
         (static_cast<std::uint32_t>(bytes[2]) << 8U) |
         static_cast<std::uint32_t>(bytes[3]);
}

std::uint64_t get_u64(const Bytes& input, std::size_t& offset) {
  std::uint8_t bytes[8];
  get_bytes(input, offset, bytes, sizeof(bytes));
  std::uint64_t result = 0;
  for (const auto byte : bytes) result = (result << 8U) | byte;
  return result;
}

Bytes get_blob(const Bytes& input, std::size_t& offset) {
  const auto length = get_u64(input, offset);
  require(length <= input.size() - offset &&
              length <= std::numeric_limits<std::size_t>::max(),
          "Select5 invalid nested material length");
  Bytes result(input.begin() + static_cast<std::ptrdiff_t>(offset),
               input.begin() + static_cast<std::ptrdiff_t>(offset + length));
  offset += static_cast<std::size_t>(length);
  return result;
}

struct OrderedExchange {
  Bytes peer;
  std::uint64_t sent = 0;
  std::uint64_t received = 0;
};

OrderedExchange ordered_exchange(ProtocolIFramedEndpoint endpoint,
                                 const ProtocolIProjectDerivedSelect5RuntimeConfig& c,
                                 std::uint8_t phase, const Bytes& outbound,
                                 std::uint64_t material_id = 0) {
  if (material_id == 0) material_id = c.material_id_base;
  ProtocolIFramedChannel channel(endpoint,
      {c.session, c.fingerprint, c.padded_n, c.k, c.comparison_bits,
       c.party, static_cast<std::uint8_t>(1U - c.party), phase, kFrameType,
       material_id},
      ProtocolIFramedChannelOptions{c.timeout_ms});
  Bytes peer;
  if (c.party == 0) {
    channel.send(outbound);
    trace_test_wire_event(c, "frame-send", phase, UINT8_MAX, outbound.size());
    peer = channel.receive();
    trace_test_wire_event(c, "frame-receive", phase, UINT8_MAX, peer.size());
  } else {
    peer = channel.receive();
    trace_test_wire_event(c, "frame-receive", phase, UINT8_MAX, peer.size());
    channel.send(outbound);
    trace_test_wire_event(c, "frame-send", phase, UINT8_MAX, outbound.size());
  }
  return {std::move(peer), channel.sent_bytes(), channel.received_bytes()};
}

class FinalStatusTransaction final {
 public:
  FinalStatusTransaction(ProtocolIFramedEndpoint endpoint,
                         const ProtocolIProjectDerivedSelect5RuntimeConfig& c)
      : channel_(endpoint,
            {c.session, c.fingerprint, c.padded_n, c.k, c.comparison_bits,
             c.party, static_cast<std::uint8_t>(1U - c.party),
             kFinalStatusPhase, kFrameType, c.material_id_base},
            ProtocolIFramedChannelOptions{c.timeout_ms}),
        party_(c.party) {}

  Bytes prepare() {
    auto peer = exchange(Bytes{1});
    channel_.set_phase(kFinalCommitPhase);
    return peer;
  }

  Bytes commit() { return exchange(Bytes{2}); }

  std::uint64_t sent() const { return channel_.sent_bytes(); }
  std::uint64_t received() const { return channel_.received_bytes(); }

 private:
  ProtocolIFramedChannel channel_;
  std::uint8_t party_;

  Bytes exchange(const Bytes& outbound) {
    if (party_ == 0) {
      channel_.send(outbound);
      return channel_.receive();
    }
    auto peer = channel_.receive();
    channel_.send(outbound);
    return peer;
  }
};

Bytes encode_words(const std::vector<std::uint64_t>& words) {
  Bytes output;
  output.reserve(words.size() * 8U);
  for (const auto word : words)
    for (int shift = 56; shift >= 0; shift -= 8)
      output.push_back(static_cast<std::uint8_t>(word >> shift));
  return output;
}

std::vector<std::uint64_t> decode_words(const Bytes& input,
                                       std::size_t expected_words) {
  require(input.size() == expected_words * 8U,
          "Select5 framed word count");
  std::vector<std::uint64_t> output(expected_words);
  std::size_t offset = 0;
  for (auto& word : output)
    for (int byte = 0; byte < 8; ++byte)
      word = (word << 8U) | input[offset++];
  return output;
}

Bytes encode_sample_seed_ack(std::uint8_t iteration) {
  Bytes result;
  put_u32(result, kSampleSeedAckMagic);
  put_u32(result, kSampleSeedContractVersion);
  result.push_back(iteration);
  return result;
}

Bytes encode_records(const std::vector<ProtocolIBlock192>& records) {
  Bytes output;
  output.reserve(records.size() * 24U);
  for (const auto& record : records) {
    put_u64(output, record.word0);
    put_u64(output, record.word1);
    put_u64(output, record.word2);
  }
  return output;
}

std::vector<ProtocolIBlock192> decode_records(const Bytes& input,
                                              std::uint32_t n,
                                              unsigned bits) {
  require(input.size() == static_cast<std::size_t>(n) * 24U,
          "Select5 shuffle record length");
  std::size_t offset = 0;
  std::vector<ProtocolIBlock192> records(n);
  for (auto& record : records) {
    record = {get_u64(input, offset), get_u64(input, offset),
              get_u64(input, offset)};
    require((record.word0 & ~mask_for(bits)) == 0,
            "Select5 shuffle key lane width");
  }
  return records;
}

struct TwoRoundShuffle {
  ProtocolIParallelShuffleRound2Output result;
  std::uint64_t sent = 0;
  std::uint64_t received = 0;
};

TwoRoundShuffle run_two_round_shuffle(
    const ProtocolIProjectDerivedSelect5RuntimeConfig& c,
    ProtocolIParallelShufflePartyMaterial&& material,
    std::uint64_t material_id, const std::vector<ProtocolIBlock192>& input,
    ProtocolIFramedEndpoint fd1, ProtocolIFramedEndpoint fd2,
    std::uint8_t phase1, std::uint8_t phase2) {
  require(material.session == c.session &&
              material.fingerprint == c.fingerprint &&
              material.material_id == material_id &&
              material.n == c.padded_n && material.k == c.k &&
              material.comparison_bits == c.comparison_bits &&
              material.party == c.party && input.size() == c.padded_n &&
              material.sigma.size() == c.padded_n &&
              material.tau.size() == c.padded_n &&
              material.a.size() == c.padded_n &&
              material.e.size() == c.padded_n &&
              material.r_share.size() == c.padded_n,
          "Select5 two-round shuffle metadata binding");
  protocol_i_validate_permutation(material.sigma);
  protocol_i_validate_permutation(material.tau);
  auto add = [&](std::vector<ProtocolIBlock192> left,
                 const std::vector<ProtocolIBlock192>& right) {
    require(left.size() == right.size(), "Select5 shuffle add shape");
    for (std::size_t i = 0; i < left.size(); ++i)
      left[i] = protocol_i_parallel_record_add(c.comparison_bits, left[i], right[i]);
    return left;
  };
  const auto first = add(protocol_i_apply_permutation(material.sigma, input),
                         material.a);
  const auto exchange1 = ordered_exchange(fd1, c, phase1,
                                           encode_records(first), material_id);
  const auto peer1 = decode_records(exchange1.peer, c.padded_n,
                                    c.comparison_bits);
  auto shuffled = add(protocol_i_apply_permutation(material.tau, peer1),
                      material.e);
  const auto second = add(shuffled, material.r_share);
  const auto exchange2 = ordered_exchange(fd2, c, phase2,
                                           encode_records(second), material_id);
  const auto peer2 = decode_records(exchange2.peer, c.padded_n,
                                    c.comparison_bits);
  auto public_masked = add(second, peer2);
  ProtocolIParallelShuffleRound2Output result{
      std::move(shuffled), std::move(public_masked)};
  return {std::move(result), exchange1.sent + exchange2.sent,
          exchange1.received + exchange2.received};
}

void claim_select_material(const ProtocolIProjectDerivedSelect5PartyMaterial& m) {
  const MaterialClaim identity{m.session, m.fingerprint, m.material_id_base,
                               m.party};
  std::lock_guard<std::mutex> lock(material_claim_mutex);
  require(material_claims.insert(identity).second,
          "Select5 package replay in this process");
}

void validate_runtime_material(
    const ProtocolIProjectDerivedSelect5RuntimeConfig& c,
    const ProtocolIProjectDerivedSelect5PartyMaterial& m,
    const std::vector<std::uint32_t>& raw,
    const ProtocolIProjectDerivedSelect5Fds& fds) {
  require(c.party <= 1 && c.timeout_ms > 0 && c.session != 0 &&
              c.fingerprint != 0 && c.logical_n >= 1 &&
              c.logical_n <= kProtocolIProjectDerivedSelect5MaxN &&
              c.padded_n == padded_size(c.logical_n) && c.k >= 1 &&
              c.k <= c.logical_n && raw.size() == c.logical_n &&
              c.comparison_bits == 33U + index_bits_for(c.padded_n) &&
              c.material_id_base != 0 &&
              c.material_id_base < UINT64_MAX - 1U &&
              (c.dcf_abi == ProtocolIDcfAbi::LegacyM2UcV1 ||
               c.dcf_abi == ProtocolIDcfAbi::Bcg21Fig1Aes128V1) &&
              (c.score_share_abi ==
                   ProtocolIScoreShareAbi::S15InducedV1 ||
               (c.score_share_abi ==
                    ProtocolIScoreShareAbi::FullKeyZeroSumRefreshV2 &&
                c.dcf_abi == ProtocolIDcfAbi::Bcg21Fig1Aes128V1)) &&
              (!c.material_store_directory.empty() ||
               c.allow_plaintext_test_transport),
          "Select5 runtime config");
  require(m.session == c.session && m.fingerprint == c.fingerprint &&
              m.party == c.party && m.logical_n == c.logical_n &&
              m.padded_n == c.padded_n && m.k == c.k &&
              m.comparison_bits == c.comparison_bits &&
              m.dcf_abi == c.dcf_abi &&
              m.score_share_abi == c.score_share_abi &&
              m.material_id_base == c.material_id_base &&
              m.material_id_base < UINT64_MAX - 1U,
          "Select5 runtime package binding");
  require(m.score_input.session == c.session &&
              m.score_input.fingerprint == c.fingerprint &&
              m.score_input.party == c.party && m.score_input.n == c.padded_n &&
              m.score_input.k == c.k &&
              m.score_input.comparison_bits == c.comparison_bits &&
              m.score_input.edge_materials.empty() &&
              m.score_input.carry_materials.size() == c.padded_n &&
              m.score_input.sign_materials.size() == c.padded_n &&
              ((c.score_share_abi == ProtocolIScoreShareAbi::S15InducedV1 &&
                m.score_key_refresh_shares.empty()) ||
               (c.score_share_abi ==
                    ProtocolIScoreShareAbi::FullKeyZeroSumRefreshV2 &&
                m.score_key_refresh_shares.size() == c.padded_n)),
          "Select5 score package shape");
  if (c.dcf_abi == ProtocolIDcfAbi::Bcg21Fig1Aes128V1)
    require(m.forward_shuffle.edge_materials.empty() &&
                m.inverse_shuffle.edge_materials.empty(),
            "BCG21 Select5 metadata cannot carry legacy shuffle edge keys");
  const auto pairs = edge_count(c.padded_n);
  require((m.streamed_material_reader
               ? m.select_slots.empty()
               : m.select_slots.size() == 5U * pairs) &&
              m.membership_slots.size() == c.padded_n,
          "Select5 online pool shape");
  if (!m.streamed_material_reader) {
    std::size_t offset = 0;
    for (std::uint32_t iteration = 0; iteration < 5; ++iteration)
      for (std::uint32_t left = 0; left < c.padded_n; ++left)
        for (std::uint32_t right = left + 1U; right < c.padded_n; ++right) {
          const auto& slot = m.select_slots[offset++];
          require(slot.iteration == iteration && slot.purpose == kSelectPurpose &&
                      slot.substage_scope == kSelectDisjointSubstageScope &&
                      slot.direction == kSelectLeftStrictRight &&
                      slot.left == left && slot.right == right &&
                      slot.material.party_id() == c.party &&
                      slot.material.comparison_bits() == c.comparison_bits &&
                      slot.material.dcf_abi() == c.dcf_abi,
                  "Select5 runtime edge descriptor");
        }
  }
  for (std::uint32_t i = 0; i < c.padded_n; ++i) {
    const auto& slot = m.membership_slots[i];
    require(slot.iteration == kMembershipIteration &&
                slot.purpose == kMembershipPurpose &&
                slot.substage == kMembershipSubstage &&
                slot.direction == kMembershipTauStrictItem && slot.item == i &&
                slot.material.party_id() == c.party &&
                slot.material.comparison_bits() == c.comparison_bits &&
                slot.material.dcf_abi() == c.dcf_abi,
            "Select5 runtime membership descriptor");
  }
  for (const auto& item : m.score_input.carry_materials)
    require(item.material.party_id() == c.party &&
                item.material.comparison_bits() == kScoreBits &&
                item.material.dcf_abi() == c.dcf_abi,
            "Select5 runtime carry key");
  for (const auto& item : m.score_input.sign_materials)
    require(item.material.party_id() == c.party &&
                item.material.comparison_bits() == kScoreBits &&
                item.material.dcf_abi() == c.dcf_abi,
            "Select5 runtime sign key");
  const auto key_ring = mask_for(c.comparison_bits);
  for (const auto rho : m.score_key_refresh_shares)
    require((rho & ~key_ring) == 0,
            "Select5 score-key refresh share width");
  std::vector<const ProtocolIFramedEndpoint*> endpoints{
      &fds.package_binding, &fds.score_input[0], &fds.score_input[1],
      &fds.forward_shuffle[0], &fds.forward_shuffle[1], &fds.membership,
      &fds.inverse_shuffle[0], &fds.inverse_shuffle[1], &fds.final_status};
  for (const auto& endpoint : fds.select_layers) endpoints.push_back(&endpoint);
  for (const auto& endpoint : fds.sample_schedule) endpoints.push_back(&endpoint);
  for (const auto* endpoint : endpoints) {
    require(endpoint->fd >= 0, "Select5 required runtime endpoint");
    if (c.allow_plaintext_test_transport) {
      require(endpoint->ssl == nullptr,
              "Select5 TEST_ONLY plaintext mode mixed with TLS");
    } else {
      require(endpoint->ssl != nullptr &&
                  endpoint->expected_peer_identity == c.expected_peer_identity &&
                  !c.expected_peer_identity.empty(),
              "Select5 TLS 1.3 mTLS endpoint required");
    }
  }
}

void abort_best_effort(
    const ProtocolIProjectDerivedSelect5RuntimeConfig& c,
    const ProtocolIProjectDerivedSelect5Fds& fds) noexcept {
  if (fds.final_status.fd < 0 || c.party > 1 || c.session == 0 ||
      c.fingerprint == 0 || c.padded_n == 0 || c.timeout_ms <= 0)
    return;
  try {
    const auto aborted = ordered_exchange(fds.final_status, c,
        kFinalAbortPhase, Bytes{0}, c.material_id_base);
    if (aborted.peer.size() == 1 && aborted.peer[0] == 0) return;
  } catch (...) {
  }
}

std::size_t dcf_serialized_size(unsigned bits, ProtocolIDcfAbi abi) {
  if (abi == ProtocolIDcfAbi::Bcg21Fig1Aes128V1)
    return bcg21_fig1_aes128_v1::serialized_size(
        static_cast<std::uint8_t>(bits));
  require(abi == ProtocolIDcfAbi::LegacyM2UcV1, "Select5 DCF ABI");
  return 57U + 24U * bits;
}

void write_score_items(Bytes& out,
                       const std::vector<ProtocolIScoreInputPartyMaterial>& items,
                       std::uint8_t expected_stage,
                       const ProtocolIProjectDerivedSelect5PartyMaterial& package) {
  for (std::uint32_t slot = 0; slot < package.padded_n; ++slot) {
    const auto& item = items.at(slot);
    require(item.slot == slot && item.stage == expected_stage &&
                item.material.party_id() == package.party &&
                item.material.comparison_bits() == kScoreBits &&
                item.material.dcf_abi() == package.dcf_abi,
            "Select5 raw-score slot binding");
    put_u64(out, item.slot);
    put_u64(out, item.stage);
    put_u64(out, item.left_mask_share);
    put_u64(out, item.right_mask_share);
    const auto key = item.material.serialize();
    put_u64(out, key.size());
    append_checked(out, key.data(), key.size());
  }
}

void write_compare_slots(Bytes& out,
                         const ProtocolIProjectDerivedSelect5PartyMaterial& package) {
  const auto pairs = edge_count(package.padded_n);
  require(package.select_slots.size() == 5U * pairs,
          "Select5 round-aware slot count");
  put_u64(out, package.select_slots.size());
  std::size_t index = 0;
  for (std::uint32_t iteration = 0; iteration < 5; ++iteration) {
    for (std::uint32_t left = 0; left < package.padded_n; ++left) {
      for (std::uint32_t right = left + 1U; right < package.padded_n; ++right) {
        const auto& slot = package.select_slots[index++];
        require(slot.iteration == iteration && slot.purpose == kSelectPurpose &&
                    slot.substage_scope == kSelectDisjointSubstageScope &&
                    slot.direction == kSelectLeftStrictRight && slot.left == left &&
                    slot.right == right && slot.material.party_id() == package.party &&
                    slot.material.comparison_bits() == package.comparison_bits &&
                    slot.material.dcf_abi() == package.dcf_abi,
                "Select5 compare slot descriptor");
        const auto ring = mask_for(package.comparison_bits);
        require((slot.left_mask_share & ~ring) == 0 &&
                    (slot.right_mask_share & ~ring) == 0,
                "Select5 compare mask share width");
        put_u32(out, slot.iteration);
        put_u32(out, slot.purpose);
        put_u32(out, slot.substage_scope);
        put_u32(out, slot.direction);
        put_u32(out, slot.left);
        put_u32(out, slot.right);
        put_u64(out, slot.left_mask_share);
        put_u64(out, slot.right_mask_share);
        const auto key = slot.material.serialize();
        require(key.size() == dcf_serialized_size(package.comparison_bits,
                                                  package.dcf_abi),
                "Select5 compare DCF serialized length");
        append_checked(out, key.data(), key.size());
      }
    }
  }
}

void write_membership_slots(
    Bytes& out, const ProtocolIProjectDerivedSelect5PartyMaterial& package) {
  require(package.membership_slots.size() == package.padded_n,
          "Select5 membership slot count");
  put_u64(out, package.membership_slots.size());
  const auto ring = mask_for(package.comparison_bits);
  for (std::uint32_t item_index = 0; item_index < package.padded_n; ++item_index) {
    const auto& slot = package.membership_slots[item_index];
    require(slot.iteration == kMembershipIteration &&
                slot.purpose == kMembershipPurpose &&
                slot.substage == kMembershipSubstage &&
                slot.direction == kMembershipTauStrictItem &&
                slot.item == item_index && slot.material.party_id() == package.party &&
                slot.material.comparison_bits() == package.comparison_bits &&
                slot.material.dcf_abi() == package.dcf_abi &&
                (slot.tau_mask_share & ~ring) == 0 &&
                (slot.item_mask_share & ~ring) == 0,
            "Select5 membership slot descriptor");
    put_u32(out, slot.iteration);
    put_u32(out, slot.purpose);
    put_u32(out, slot.substage);
    put_u32(out, slot.direction);
    put_u32(out, UINT32_MAX);  // virtual tau endpoint
    put_u32(out, slot.item);
    put_u64(out, slot.tau_mask_share);
    put_u64(out, slot.item_mask_share);
    const auto key = slot.material.serialize();
    require(key.size() == dcf_serialized_size(package.comparison_bits,
                                              package.dcf_abi),
            "Select5 membership DCF serialized length");
    append_checked(out, key.data(), key.size());
  }
}

ProtocolIPartyPackage make_score_package(std::uint64_t session,
                                         std::uint64_t fingerprint,
                                         std::uint32_t padded_n,
                                         std::uint32_t k, std::uint8_t bits,
                                         std::uint8_t party) {
  ProtocolIPartyPackage result;
  result.session = session;
  result.fingerprint = fingerprint;
  result.party = party;
  result.comparison_bits = bits;
  result.n = padded_n;
  result.k = k;
  return result;
}

void share_mask(std::uint64_t full, unsigned bits, std::uint64_t& share0,
                std::uint64_t& share1) {
  const auto ring = mask_for(bits);
  share0 = random_u64() & ring;
  share1 = (full - share0) & ring;
}

void append_score_stage(std::uint8_t stage, std::uint32_t padded_n,
                        unsigned bits, ProtocolIPartyPackage& p0,
                        ProtocolIPartyPackage& p1, ProtocolIDcfAbi abi) {
  const auto ring = mask_for(bits);
  auto& out0 = stage == 1 ? p0.carry_materials : p0.sign_materials;
  auto& out1 = stage == 1 ? p1.carry_materials : p1.sign_materials;
  out0.reserve(padded_n);
  out1.reserve(padded_n);
  for (std::uint32_t slot = 0; slot < padded_n; ++slot) {
    const auto left = random_u64() & ring;
    const auto right = random_u64() & ring;
    std::uint64_t left0, left1, right0, right1;
    share_mask(left, bits, left0, left1);
    share_mask(right, bits, right0, right1);
    ProtocolIUcmpMaterial generated(bits, left, right, abi);
    out0.emplace_back(slot, stage, left0, right0,
                      generated.export_party_material(0));
    out1.emplace_back(slot, stage, left1, right1,
                      generated.export_party_material(1));
  }
}

std::uint64_t streamed_slot_count(std::uint32_t padded_n) {
  return 5U * edge_count(padded_n) + 13U * padded_n;
}

std::uint64_t streamed_slot_count(
    std::uint32_t padded_n, ProtocolIScoreShareAbi score_share_abi) {
  return streamed_slot_count(padded_n) +
      (score_share_abi == ProtocolIScoreShareAbi::FullKeyZeroSumRefreshV2
           ? padded_n : 0U);
}

std::uint32_t package_version_for(
    ProtocolIDcfAbi dcf_abi, ProtocolIScoreShareAbi score_share_abi) {
  if (dcf_abi == ProtocolIDcfAbi::LegacyM2UcV1) {
    require(score_share_abi == ProtocolIScoreShareAbi::S15InducedV1,
            "legacy M2UC cannot carry F_scoreconv/v2");
    return 2U;
  }
  require(dcf_abi == ProtocolIDcfAbi::Bcg21Fig1Aes128V1,
          "Select5 package DCF ABI");
  if (score_share_abi == ProtocolIScoreShareAbi::S15InducedV1) return 3U;
  require(score_share_abi == ProtocolIScoreShareAbi::FullKeyZeroSumRefreshV2,
          "Select5 score-share ABI");
  return 4U;
}

ProtocolIMaterialSlotBinding streamed_binding(
    const ProtocolIProjectDerivedSelect5RuntimeConfig& config) {
  ProtocolIMaterialSlotBinding binding;
  binding.session = config.session;
  binding.fingerprint = config.fingerprint;
  binding.material_id = config.material_id_base;
  binding.logical_n = config.logical_n;
  binding.padded_n = config.padded_n;
  binding.k = config.k;
  binding.comparison_bits = config.comparison_bits;
  binding.party = config.party;
  binding.dcf_abi = config.dcf_abi;
  binding.score_share_abi = config.score_share_abi;
  binding.expected_slot_count =
      streamed_slot_count(config.padded_n, config.score_share_abi);
  return binding;
}

Bytes claim_streamed_slot(
    const std::shared_ptr<ProtocolIStreamedMaterialReader>& reader,
    const ProtocolIMaterialSlotAddress& address) {
  require(reader != nullptr, "Select5 streamed slot reader missing");
  return reader->claim_slot(address);
}

ProtocolIParallelShufflePartyMaterial load_streamed_shuffle_metadata(
    const std::shared_ptr<ProtocolIStreamedMaterialReader>& reader,
    const ProtocolIProjectDerivedSelect5RuntimeConfig& config,
    std::uint32_t direction, std::uint64_t material_id) {
  ProtocolIParallelShufflePartyMaterial result;
  result.session = config.session;
  result.fingerprint = config.fingerprint;
  result.material_id = material_id;
  result.n = config.padded_n;
  result.k = config.k;
  result.comparison_bits = config.comparison_bits;
  result.party = config.party;
  result.sigma.resize(config.padded_n);
  result.tau.resize(config.padded_n);
  result.a.resize(config.padded_n);
  result.e.resize(config.padded_n);
  result.r_share.resize(config.padded_n);
  auto load_permutation = [&](std::uint32_t substage,
                              ProtocolIPermutation& output) {
    for (std::uint32_t i = 0; i < config.padded_n; ++i) {
      const auto bytes = claim_streamed_slot(reader,
          {kShuffleMetadataPurpose, 0, substage, direction, i, 0});
      std::size_t offset = 0;
      output[i] = get_u32(bytes, offset);
      require(offset == bytes.size(), "Select5 shuffle permutation slot size");
    }
  };
  auto load_records = [&](std::uint32_t substage,
                          std::vector<ProtocolIBlock192>& output) {
    for (std::uint32_t i = 0; i < config.padded_n; ++i) {
      const auto bytes = claim_streamed_slot(reader,
          {kShuffleMetadataPurpose, 0, substage, direction, i, 0});
      std::size_t offset = 0;
      output[i] = {get_u64(bytes, offset), get_u64(bytes, offset),
                   get_u64(bytes, offset)};
      require(offset == bytes.size(), "Select5 shuffle record slot size");
    }
  };
  load_permutation(1, result.sigma);
  load_permutation(2, result.tau);
  load_records(3, result.a);
  load_records(4, result.e);
  load_records(5, result.r_share);
  protocol_i_validate_permutation(result.sigma);
  protocol_i_validate_permutation(result.tau);
  return result;
}

ProtocolIProjectDerivedSelect5PartyMaterial load_streamed_party_material(
    const ProtocolIProjectDerivedSelect5RuntimeConfig& config,
    const std::shared_ptr<ProtocolIStreamedMaterialReader>& reader) {
  ProtocolIProjectDerivedSelect5PartyMaterial result;
  result.session = config.session;
  result.fingerprint = config.fingerprint;
  result.material_id_base = config.material_id_base;
  result.logical_n = config.logical_n;
  result.padded_n = config.padded_n;
  result.k = config.k;
  result.comparison_bits = config.comparison_bits;
  result.party = config.party;
  result.dcf_abi = config.dcf_abi;
  result.score_share_abi = config.score_share_abi;
  result.streamed_material_reader = reader;
  result.score_input = make_score_package(config.session, config.fingerprint,
      config.padded_n, config.k, config.comparison_bits, config.party);
  result.score_input.carry_materials.reserve(config.padded_n);
  result.score_input.sign_materials.reserve(config.padded_n);
  for (std::uint32_t stage = 1; stage <= 2; ++stage) {
    auto& output = stage == 1 ? result.score_input.carry_materials
                              : result.score_input.sign_materials;
    for (std::uint32_t slot = 0; slot < config.padded_n; ++slot) {
      auto bytes = claim_streamed_slot(reader,
          {kScorePurpose, 0, stage, 0, slot, UINT32_MAX});
      std::size_t offset = 0;
      const auto left = get_u64(bytes, offset);
      const auto right = get_u64(bytes, offset);
      Bytes encoded(bytes.begin() + static_cast<std::ptrdiff_t>(offset), bytes.end());
      auto key = ProtocolIUcmpPartyMaterial::deserialize(encoded);
      require(key.party_id() == config.party &&
                  key.comparison_bits() == kScoreBits &&
                  key.dcf_abi() == config.dcf_abi,
              "Select5 streamed raw-score key binding");
      output.emplace_back(slot, static_cast<std::uint8_t>(stage), left, right,
                          std::move(key));
    }
  }
  if (config.score_share_abi ==
      ProtocolIScoreShareAbi::FullKeyZeroSumRefreshV2) {
    result.score_key_refresh_shares.reserve(config.padded_n);
    const auto ring = mask_for(config.comparison_bits);
    for (std::uint32_t slot = 0; slot < config.padded_n; ++slot) {
      const auto bytes = claim_streamed_slot(reader,
          {kScoreKeyRefreshPurpose, 0, 2, 0, slot, UINT32_MAX});
      std::size_t offset = 0;
      const auto rho = get_u64(bytes, offset);
      require(offset == bytes.size() && (rho & ~ring) == 0,
              "Select5 streamed score-key refresh slot");
      result.score_key_refresh_shares.push_back(rho);
    }
  }
  result.forward_shuffle = load_streamed_shuffle_metadata(
      reader, config, kShuffleForwardDirection, config.material_id_base);
  result.inverse_shuffle = load_streamed_shuffle_metadata(
      reader, config, kShuffleInverseDirection, config.material_id_base + 1U);
  result.membership_slots.reserve(config.padded_n);
  for (std::uint32_t item = 0; item < config.padded_n; ++item) {
    auto bytes = claim_streamed_slot(reader,
        {kMembershipPurpose, kMembershipIteration, kMembershipSubstage,
         kMembershipTauStrictItem, UINT32_MAX, item});
    std::size_t offset = 0;
    const auto tau_share = get_u64(bytes, offset);
    const auto item_share = get_u64(bytes, offset);
    Bytes encoded(bytes.begin() + static_cast<std::ptrdiff_t>(offset), bytes.end());
    auto key = ProtocolIUcmpPartyMaterial::deserialize(encoded);
    require(key.party_id() == config.party &&
                key.comparison_bits() == config.comparison_bits &&
                key.dcf_abi() == config.dcf_abi,
            "Select5 streamed membership key binding");
    result.membership_slots.emplace_back(item, tau_share, item_share,
                                         std::move(key));
  }
  return result;
}

}  // namespace

std::vector<std::uint8_t> protocol_i_project_derived_select5_serialize_party_material(
    const ProtocolIProjectDerivedSelect5PartyMaterial& material) {
  const auto expected_n = padded_size(material.logical_n);
  require(material.session != 0 && material.fingerprint != 0 &&
              material.material_id_base != 0 && material.padded_n == expected_n &&
              material.k >= 1 && material.k <= material.logical_n &&
              material.comparison_bits == 33U + index_bits_for(material.padded_n) &&
              material.party < 2 &&
              (material.dcf_abi == ProtocolIDcfAbi::LegacyM2UcV1 ||
               material.dcf_abi == ProtocolIDcfAbi::Bcg21Fig1Aes128V1),
          "Select5 T package metadata");
  require(material.score_input.session == material.session &&
              material.score_input.fingerprint == material.fingerprint &&
              material.score_input.party == material.party &&
              material.score_input.n == material.padded_n &&
              material.score_input.k == material.k &&
              material.score_input.comparison_bits == material.comparison_bits &&
              material.score_input.edge_materials.empty() &&
              material.score_input.carry_materials.size() == material.padded_n &&
              material.score_input.sign_materials.size() == material.padded_n &&
              ((material.score_share_abi ==
                    ProtocolIScoreShareAbi::S15InducedV1 &&
                material.score_key_refresh_shares.empty()) ||
               (material.score_share_abi ==
                    ProtocolIScoreShareAbi::FullKeyZeroSumRefreshV2 &&
                material.dcf_abi == ProtocolIDcfAbi::Bcg21Fig1Aes128V1 &&
                material.score_key_refresh_shares.size() == material.padded_n)),
          "Select5 raw-score material metadata");

  const auto package_version =
      package_version_for(material.dcf_abi, material.score_share_abi);
  Bytes out{'M', '6', 'B', '5', static_cast<std::uint8_t>(package_version),
            material.party, material.comparison_bits,
            static_cast<std::uint8_t>(material.dcf_abi)};
  put_u64(out, material.session);
  put_u64(out, material.fingerprint);
  put_u64(out, material.material_id_base);
  put_u32(out, material.logical_n);
  put_u32(out, material.padded_n);
  put_u32(out, material.k);
  write_score_items(out, material.score_input.carry_materials, 1, material);
  write_score_items(out, material.score_input.sign_materials, 2, material);
  if (material.score_share_abi ==
      ProtocolIScoreShareAbi::FullKeyZeroSumRefreshV2) {
    const auto ring = mask_for(material.comparison_bits);
    put_u64(out, material.score_key_refresh_shares.size());
    for (const auto rho : material.score_key_refresh_shares) {
      require((rho & ~ring) == 0, "Select5 refresh share width");
      put_u64(out, rho);
    }
  }

  const auto serialize_shuffle = [&](const ProtocolIParallelShufflePartyMaterial& value) {
    return material.dcf_abi == ProtocolIDcfAbi::Bcg21Fig1Aes128V1
        ? protocol_i_parallel_shuffle_serialize_select5_metadata(value)
        : protocol_i_parallel_shuffle_serialize_material(value);
  };
  const auto forward = serialize_shuffle(material.forward_shuffle);
  const auto inverse = serialize_shuffle(material.inverse_shuffle);
  require(material.forward_shuffle.session == material.session &&
              material.inverse_shuffle.session == material.session &&
              material.forward_shuffle.fingerprint == material.fingerprint &&
              material.inverse_shuffle.fingerprint == material.fingerprint &&
              material.forward_shuffle.material_id == material.material_id_base &&
              material.inverse_shuffle.material_id == material.material_id_base + 1U &&
              material.forward_shuffle.party == material.party &&
              material.inverse_shuffle.party == material.party &&
              material.forward_shuffle.n == material.padded_n &&
              material.inverse_shuffle.n == material.padded_n &&
              material.forward_shuffle.k == material.k &&
              material.inverse_shuffle.k == material.k &&
              material.forward_shuffle.comparison_bits == material.comparison_bits &&
              material.inverse_shuffle.comparison_bits == material.comparison_bits,
          "Select5 forward/inverse shuffle material binding");
  put_blob(out, forward);
  put_blob(out, inverse);
  write_compare_slots(out, material);
  write_membership_slots(out, material);
  require(out.size() <= kProtocolIProjectDerivedSelect5MaxPackageBytes,
          "Select5 T package cap");
  return out;
}

ProtocolIProjectDerivedSelect5PartyMaterial
protocol_i_project_derived_select5_deserialize_party_material(
    const std::vector<std::uint8_t>& package, int expected_party) {
  require(expected_party >= 0 && expected_party <= 1 &&
              package.size() <= kProtocolIProjectDerivedSelect5MaxPackageBytes &&
              package.size() >= 44U && package[0] == 'M' && package[1] == '6' &&
              package[2] == 'B' && package[3] == '5' &&
              ((package[4] == 2 && package[7] == 0) ||
               (package[4] == 3 && package[7] == 1) ||
               (package[4] == 4 && package[7] == 1)) &&
              package[5] == expected_party,
          "Select5 T package header");
  ProtocolIProjectDerivedSelect5PartyMaterial result;
  result.party = package[5];
  result.comparison_bits = package[6];
  result.dcf_abi = package[7] == 0
      ? ProtocolIDcfAbi::LegacyM2UcV1
      : ProtocolIDcfAbi::Bcg21Fig1Aes128V1;
  require((package[4] == 2 && result.dcf_abi == ProtocolIDcfAbi::LegacyM2UcV1) ||
              (package[4] == 3 && result.dcf_abi == ProtocolIDcfAbi::Bcg21Fig1Aes128V1) ||
              (package[4] == 4 && result.dcf_abi == ProtocolIDcfAbi::Bcg21Fig1Aes128V1),
          "Select5 package version/DCF ABI mismatch");
  result.score_share_abi = package[4] == 4
      ? ProtocolIScoreShareAbi::FullKeyZeroSumRefreshV2
      : ProtocolIScoreShareAbi::S15InducedV1;
  std::size_t offset = 8;
  result.session = get_u64(package, offset);
  result.fingerprint = get_u64(package, offset);
  result.material_id_base = get_u64(package, offset);
  result.logical_n = get_u32(package, offset);
  result.padded_n = get_u32(package, offset);
  result.k = get_u32(package, offset);
  require(result.session != 0 && result.fingerprint != 0 &&
              result.material_id_base != 0 &&
              result.padded_n == padded_size(result.logical_n) &&
              result.k >= 1 && result.k <= result.logical_n &&
              result.comparison_bits == 33U + index_bits_for(result.padded_n),
          "Select5 T package dimensions");

  result.score_input = make_score_package(
      result.session, result.fingerprint, result.padded_n, result.k,
      result.comparison_bits, result.party);
  auto read_score_stage = [&](std::uint8_t stage,
                              std::vector<ProtocolIScoreInputPartyMaterial>& items) {
    items.reserve(result.padded_n);
    for (std::uint32_t slot = 0; slot < result.padded_n; ++slot) {
      const auto encoded_slot = get_u64(package, offset);
      const auto encoded_stage = get_u64(package, offset);
      const auto left = get_u64(package, offset);
      const auto right = get_u64(package, offset);
      const auto length = get_u64(package, offset);
      require(encoded_slot == slot && encoded_stage == stage &&
                  length <= package.size() - offset &&
                  length <= std::numeric_limits<std::size_t>::max(),
              "Select5 raw-score descriptor");
      Bytes key(package.begin() + static_cast<std::ptrdiff_t>(offset),
                package.begin() + static_cast<std::ptrdiff_t>(offset + length));
      offset += static_cast<std::size_t>(length);
      auto decoded = ProtocolIUcmpPartyMaterial::deserialize(key);
      require(decoded.party_id() == result.party &&
                  decoded.comparison_bits() == kScoreBits &&
                  decoded.dcf_abi() == result.dcf_abi,
              "Select5 raw-score DCF binding");
      items.emplace_back(slot, stage, left, right, std::move(decoded));
    }
  };
  read_score_stage(1, result.score_input.carry_materials);
  read_score_stage(2, result.score_input.sign_materials);
  if (result.score_share_abi ==
      ProtocolIScoreShareAbi::FullKeyZeroSumRefreshV2) {
    const auto count = get_u64(package, offset);
    require(count == result.padded_n, "Select5 refresh slot count");
    const auto ring = mask_for(result.comparison_bits);
    result.score_key_refresh_shares.reserve(result.padded_n);
    for (std::uint32_t slot = 0; slot < result.padded_n; ++slot) {
      const auto rho = get_u64(package, offset);
      require((rho & ~ring) == 0, "Select5 refresh slot width");
      result.score_key_refresh_shares.push_back(rho);
    }
  }

  auto forward = get_blob(package, offset);
  auto inverse = get_blob(package, offset);
  const auto deserialize_shuffle = [&](const Bytes& value) {
    return result.dcf_abi == ProtocolIDcfAbi::Bcg21Fig1Aes128V1
        ? protocol_i_parallel_shuffle_deserialize_select5_metadata(value,
                                                                    result.party)
        : protocol_i_parallel_shuffle_deserialize_material(value, result.party);
  };
  result.forward_shuffle = deserialize_shuffle(forward);
  result.inverse_shuffle = deserialize_shuffle(inverse);

  const auto expected_pairs = edge_count(result.padded_n);
  const auto select_count = get_u64(package, offset);
  require(select_count == 5U * expected_pairs,
          "Select5 round-aware slot count");
  result.select_slots.reserve(static_cast<std::size_t>(select_count));
  const auto ring = mask_for(result.comparison_bits);
  for (std::uint32_t iteration = 0; iteration < 5; ++iteration) {
    for (std::uint32_t left = 0; left < result.padded_n; ++left) {
      for (std::uint32_t right = left + 1U; right < result.padded_n; ++right) {
        const auto got_iteration = get_u32(package, offset);
        const auto purpose = get_u32(package, offset);
        const auto scope = get_u32(package, offset);
        const auto direction = get_u32(package, offset);
        const auto got_left = get_u32(package, offset);
        const auto got_right = get_u32(package, offset);
        const auto left_share = get_u64(package, offset);
        const auto right_share = get_u64(package, offset);
        require(got_iteration == iteration && purpose == kSelectPurpose &&
                    scope == kSelectDisjointSubstageScope &&
                    direction == kSelectLeftStrictRight && got_left == left &&
                    got_right == right && (left_share & ~ring) == 0 &&
                    (right_share & ~ring) == 0,
                "Select5 compare slot descriptor");
        Bytes key(dcf_serialized_size(result.comparison_bits, result.dcf_abi));
        get_bytes(package, offset, key.data(), key.size());
        auto decoded = ProtocolIUcmpPartyMaterial::deserialize(key);
        require(decoded.party_id() == result.party &&
                    decoded.comparison_bits() == result.comparison_bits &&
                    decoded.dcf_abi() == result.dcf_abi,
                "Select5 compare DCF binding");
        result.select_slots.emplace_back(
            iteration, purpose, scope, direction, left, right,
            left_share, right_share, std::move(decoded));
      }
    }
  }

  const auto membership_count = get_u64(package, offset);
  require(membership_count == result.padded_n,
          "Select5 membership slot count");
  result.membership_slots.reserve(result.padded_n);
  for (std::uint32_t item = 0; item < result.padded_n; ++item) {
    const auto iteration = get_u32(package, offset);
    const auto purpose = get_u32(package, offset);
    const auto substage = get_u32(package, offset);
    const auto direction = get_u32(package, offset);
    const auto tau_endpoint = get_u32(package, offset);
    const auto got_item = get_u32(package, offset);
    const auto tau_share = get_u64(package, offset);
    const auto item_share = get_u64(package, offset);
    require(iteration == kMembershipIteration &&
                purpose == kMembershipPurpose &&
                substage == kMembershipSubstage &&
                direction == kMembershipTauStrictItem &&
                tau_endpoint == UINT32_MAX && got_item == item &&
                (tau_share & ~ring) == 0 && (item_share & ~ring) == 0,
            "Select5 membership descriptor");
    Bytes key(dcf_serialized_size(result.comparison_bits, result.dcf_abi));
    get_bytes(package, offset, key.data(), key.size());
    auto decoded = ProtocolIUcmpPartyMaterial::deserialize(key);
    require(decoded.party_id() == result.party &&
                decoded.comparison_bits() == result.comparison_bits &&
                decoded.dcf_abi() == result.dcf_abi,
            "Select5 membership DCF binding");
    result.membership_slots.emplace_back(item, tau_share, item_share,
                                          std::move(decoded));
  }
  require(offset == package.size(), "Select5 T package trailing bytes");
  require(result.forward_shuffle.session == result.session &&
              result.inverse_shuffle.session == result.session &&
              result.forward_shuffle.fingerprint == result.fingerprint &&
              result.inverse_shuffle.fingerprint == result.fingerprint &&
              result.forward_shuffle.material_id == result.material_id_base &&
              result.inverse_shuffle.material_id == result.material_id_base + 1U &&
              result.forward_shuffle.n == result.padded_n &&
              result.inverse_shuffle.n == result.padded_n &&
              result.forward_shuffle.k == result.k &&
              result.inverse_shuffle.k == result.k &&
              result.forward_shuffle.comparison_bits == result.comparison_bits &&
              result.inverse_shuffle.comparison_bits == result.comparison_bits,
          "Select5 shuffle binding");
  return result;
}

ProtocolIProjectDerivedSelect5DealerOutput
protocol_i_project_derived_select5_dealer_generate(
    const ProtocolIProjectDerivedSelect5DealerConfig& config) {
  require(config.session != 0 && config.fingerprint != 0 &&
              config.material_id_base != 0 && config.material_id_base < UINT64_MAX - 1U &&
              config.logical_n >= 1 && config.logical_n <= 128 &&
              config.k >= 1 && config.k <= config.logical_n &&
              config.timeout_ms > 0,
          "Select5 monolithic dealer supports logical N<=128; use streamed M6S2 for N<=256");
  (void)package_version_for(config.dcf_abi, config.score_share_abi);
  const auto padded_n = padded_size(config.logical_n);
  const auto index_bits = index_bits_for(padded_n);
  const auto comparison_bits = static_cast<std::uint8_t>(33U + index_bits);
  const auto permutation = fresh_permutation(padded_n);
  const auto inverse_permutation = protocol_i_inverse_permutation(permutation);
  ProtocolIParallelShuffleDealerConfig forward_config{
      config.session, config.fingerprint, config.material_id_base,
      padded_n, config.k, comparison_bits};
  ProtocolIParallelShuffleDealerConfig inverse_config{
      config.session, config.fingerprint, config.material_id_base + 1U,
      padded_n, config.k, comparison_bits};
  const bool bcg21 =
      config.dcf_abi == ProtocolIDcfAbi::Bcg21Fig1Aes128V1;
  auto forward = bcg21
      ? protocol_i_parallel_shuffle_dealer_generate_two_round_metadata_for_permutation(
            forward_config, permutation)
      : protocol_i_parallel_shuffle_dealer_generate_for_permutation(
            forward_config, permutation);
  auto inverse = bcg21
      ? protocol_i_parallel_shuffle_dealer_generate_two_round_metadata_for_permutation(
            inverse_config, inverse_permutation)
      : protocol_i_parallel_shuffle_dealer_generate_for_permutation(
            inverse_config, inverse_permutation);

  ProtocolIProjectDerivedSelect5PartyMaterial p0, p1;
  auto initialize = [&](ProtocolIProjectDerivedSelect5PartyMaterial& target,
                        std::uint8_t party) {
    target.session = config.session;
    target.fingerprint = config.fingerprint;
    target.material_id_base = config.material_id_base;
    target.logical_n = config.logical_n;
    target.padded_n = padded_n;
    target.k = config.k;
    target.comparison_bits = comparison_bits;
    target.party = party;
    target.score_share_abi = config.score_share_abi;
    target.score_input = make_score_package(config.session, config.fingerprint,
        padded_n, config.k, comparison_bits, party);
  };
  initialize(p0, 0);
  initialize(p1, 1);
  p0.forward_shuffle = std::move(forward.party0);
  p1.forward_shuffle = std::move(forward.party1);
  p0.inverse_shuffle = std::move(inverse.party0);
  p1.inverse_shuffle = std::move(inverse.party1);

  p0.dcf_abi = config.dcf_abi;
  p1.dcf_abi = config.dcf_abi;
  append_score_stage(1, padded_n, kScoreBits, p0.score_input, p1.score_input,
                     config.dcf_abi);
  append_score_stage(2, padded_n, kScoreBits, p0.score_input, p1.score_input,
                     config.dcf_abi);
  if (config.score_share_abi ==
      ProtocolIScoreShareAbi::FullKeyZeroSumRefreshV2) {
    const auto ring = mask_for(comparison_bits);
    p0.score_key_refresh_shares.reserve(padded_n);
    p1.score_key_refresh_shares.reserve(padded_n);
    for (std::uint32_t slot = 0; slot < padded_n; ++slot) {
      const auto rho0 = random_u64() & ring;
      p0.score_key_refresh_shares.push_back(rho0);
      p1.score_key_refresh_shares.push_back((UINT64_C(0) - rho0) & ring);
    }
  }

  const auto ring = mask_for(comparison_bits);
  const auto pairs = edge_count(padded_n);
  p0.select_slots.reserve(static_cast<std::size_t>(5U * pairs));
  p1.select_slots.reserve(static_cast<std::size_t>(5U * pairs));
  for (std::uint32_t iteration = 0; iteration < 5; ++iteration) {
    for (std::uint32_t left = 0; left < padded_n; ++left) {
      for (std::uint32_t right = left + 1U; right < padded_n; ++right) {
        const auto mask_left = random_u64() & ring;
        const auto mask_right = random_u64() & ring;
        std::uint64_t left0, left1, right0, right1;
        share_mask(mask_left, comparison_bits, left0, left1);
        share_mask(mask_right, comparison_bits, right0, right1);
        ProtocolIUcmpMaterial generated(comparison_bits, mask_left, mask_right,
                                        config.dcf_abi);
        p0.select_slots.emplace_back(iteration, kSelectPurpose,
            kSelectDisjointSubstageScope, kSelectLeftStrictRight, left, right,
            left0, right0, generated.export_party_material(0));
        p1.select_slots.emplace_back(iteration, kSelectPurpose,
            kSelectDisjointSubstageScope, kSelectLeftStrictRight, left, right,
            left1, right1, generated.export_party_material(1));
      }
    }
  }

  p0.membership_slots.reserve(padded_n);
  p1.membership_slots.reserve(padded_n);
  for (std::uint32_t item = 0; item < padded_n; ++item) {
    const auto mask_tau = random_u64() & ring;
    const auto mask_item = random_u64() & ring;
    std::uint64_t tau0, tau1, item0, item1;
    share_mask(mask_tau, comparison_bits, tau0, tau1);
    share_mask(mask_item, comparison_bits, item0, item1);
    ProtocolIUcmpMaterial generated(comparison_bits, mask_tau, mask_item,
                                    config.dcf_abi);
    p0.membership_slots.emplace_back(item, tau0, item0,
                                     generated.export_party_material(0));
    p1.membership_slots.emplace_back(item, tau1, item1,
                                     generated.export_party_material(1));
  }

  ProtocolIProjectDerivedSelect5DealerOutput output;
  output.party0_package =
      protocol_i_project_derived_select5_serialize_party_material(p0);
  output.party1_package =
      protocol_i_project_derived_select5_serialize_party_material(p1);
  return output;
}

void protocol_i_project_derived_select5_dealer_stage_streamed_material(
    const std::array<std::string, 2>& party_store_directories,
    const ProtocolIProjectDerivedSelect5DealerConfig& config) {
  require(config.session != 0 && config.fingerprint != 0 &&
              config.material_id_base != 0 && config.material_id_base < UINT64_MAX - 1U &&
              config.logical_n >= 1 && config.logical_n <= 256 &&
              config.k >= 1 && config.k <= config.logical_n &&
              (config.dcf_abi == ProtocolIDcfAbi::LegacyM2UcV1 ||
               config.dcf_abi == ProtocolIDcfAbi::Bcg21Fig1Aes128V1) &&
              (config.score_share_abi ==
                   ProtocolIScoreShareAbi::S15InducedV1 ||
               (config.score_share_abi ==
                    ProtocolIScoreShareAbi::FullKeyZeroSumRefreshV2 &&
                config.dcf_abi == ProtocolIDcfAbi::Bcg21Fig1Aes128V1)) &&
              config.timeout_ms > 0 && !party_store_directories[0].empty() &&
              !party_store_directories[1].empty(),
          "Select5 streamed dealer config");
  const auto padded_n = padded_size(config.logical_n);
  require(padded_n <= 256, "Select5 streamed padded N exceeds 256");
  const auto index_bits = index_bits_for(padded_n);
  const auto comparison_bits = static_cast<std::uint8_t>(33U + index_bits);
  const auto slot_count =
      streamed_slot_count(padded_n, config.score_share_abi);
  std::array<std::unique_ptr<ProtocolIStreamedMaterialWriter>, 2> writers;
  for (std::uint8_t party = 0; party < 2; ++party) {
    ProtocolIMaterialSlotBinding binding;
    binding.session = config.session;
    binding.fingerprint = config.fingerprint;
    binding.material_id = config.material_id_base;
    binding.logical_n = config.logical_n;
    binding.padded_n = padded_n;
    binding.k = config.k;
    binding.comparison_bits = comparison_bits;
    binding.party = party;
    binding.dcf_abi = config.dcf_abi;
    binding.score_share_abi = config.score_share_abi;
    binding.expected_slot_count = slot_count;
    writers[party].reset(new ProtocolIStreamedMaterialWriter(
        party_store_directories[party], binding));
  }

  auto append_pair = [&](const ProtocolIMaterialSlotAddress& address,
                         const Bytes& p0, const Bytes& p1) {
    writers[0]->append(address, p0);
    writers[1]->append(address, p1);
  };

  const auto permutation = fresh_permutation(padded_n);
  const auto inverse_permutation = protocol_i_inverse_permutation(permutation);
  ProtocolIParallelShuffleDealerConfig forward_config{
      config.session, config.fingerprint, config.material_id_base,
      padded_n, config.k, comparison_bits};
  ProtocolIParallelShuffleDealerConfig inverse_config{
      config.session, config.fingerprint, config.material_id_base + 1U,
      padded_n, config.k, comparison_bits};
  auto forward = protocol_i_parallel_shuffle_dealer_generate_two_round_metadata_for_permutation(
      forward_config, permutation);
  auto inverse = protocol_i_parallel_shuffle_dealer_generate_two_round_metadata_for_permutation(
      inverse_config, inverse_permutation);

  auto append_shuffle_metadata = [&](const ProtocolIParallelShufflePartyMaterial& p0,
                                     const ProtocolIParallelShufflePartyMaterial& p1,
                                     std::uint32_t direction) {
    auto append_permutation = [&](std::uint32_t substage,
                                  const ProtocolIPermutation& a,
                                  const ProtocolIPermutation& b) {
      require(a.size() == padded_n && b.size() == padded_n,
              "Select5 streamed shuffle permutation shape");
      for (std::uint32_t i = 0; i < padded_n; ++i) {
        Bytes v0, v1;
        put_u32(v0, a[i]); put_u32(v1, b[i]);
        append_pair({kShuffleMetadataPurpose, 0, substage, direction, i, 0}, v0, v1);
      }
    };
    append_permutation(1, p0.sigma, p1.sigma);
    append_permutation(2, p0.tau, p1.tau);
    auto append_records = [&](std::uint32_t substage,
                              const std::vector<ProtocolIBlock192>& a,
                              const std::vector<ProtocolIBlock192>& b) {
      require(a.size() == padded_n && b.size() == padded_n,
              "Select5 streamed shuffle record shape");
      for (std::uint32_t i = 0; i < padded_n; ++i) {
        Bytes v0, v1;
        for (const auto word : {a[i].word0, a[i].word1, a[i].word2}) put_u64(v0, word);
        for (const auto word : {b[i].word0, b[i].word1, b[i].word2}) put_u64(v1, word);
        append_pair({kShuffleMetadataPurpose, 0, substage, direction, i, 0}, v0, v1);
      }
    };
    append_records(3, p0.a, p1.a);
    append_records(4, p0.e, p1.e);
    append_records(5, p0.r_share, p1.r_share);
  };
  append_shuffle_metadata(forward.party0, forward.party1,
                          kShuffleForwardDirection);
  append_shuffle_metadata(inverse.party0, inverse.party1,
                          kShuffleInverseDirection);

  const auto score_ring = mask_for(kScoreBits);
  for (std::uint32_t stage = 1; stage <= 2; ++stage) {
    for (std::uint32_t slot = 0; slot < padded_n; ++slot) {
      const auto left = random_u64() & score_ring;
      const auto right = random_u64() & score_ring;
      std::uint64_t left0, left1, right0, right1;
      share_mask(left, kScoreBits, left0, left1);
      share_mask(right, kScoreBits, right0, right1);
      ProtocolIUcmpMaterial generated(kScoreBits, left, right,
                                      config.dcf_abi);
      const auto key0 = generated.export_party_material(0).serialize();
      const auto key1 = generated.export_party_material(1).serialize();
      Bytes p0, p1;
      put_u64(p0, left0); put_u64(p0, right0);
      put_u64(p1, left1); put_u64(p1, right1);
      append_checked(p0, key0.data(), key0.size());
      append_checked(p1, key1.data(), key1.size());
      append_pair({kScorePurpose, 0, stage, 0, slot, UINT32_MAX}, p0, p1);
    }
  }

  if (config.score_share_abi ==
      ProtocolIScoreShareAbi::FullKeyZeroSumRefreshV2) {
    const auto ring = mask_for(comparison_bits);
    for (std::uint32_t slot = 0; slot < padded_n; ++slot) {
      const auto rho0 = random_u64() & ring;
      const auto rho1 = (UINT64_C(0) - rho0) & ring;
      Bytes p0, p1;
      put_u64(p0, rho0);
      put_u64(p1, rho1);
      append_pair({kScoreKeyRefreshPurpose, 0, 2, 0, slot, UINT32_MAX},
                  p0, p1);
    }
  }

  const auto ring = mask_for(comparison_bits);
  for (std::uint32_t iteration = 0; iteration < 5; ++iteration) {
    for (std::uint32_t left = 0; left < padded_n; ++left) {
      for (std::uint32_t right = left + 1U; right < padded_n; ++right) {
        const auto mask_left = random_u64() & ring;
        const auto mask_right = random_u64() & ring;
        std::uint64_t left0, left1, right0, right1;
        share_mask(mask_left, comparison_bits, left0, left1);
        share_mask(mask_right, comparison_bits, right0, right1);
        ProtocolIUcmpMaterial generated(comparison_bits, mask_left, mask_right,
                                        config.dcf_abi);
        const auto key0 = generated.export_party_material(0).serialize();
        const auto key1 = generated.export_party_material(1).serialize();
        Bytes p0, p1;
        put_u64(p0, left0); put_u64(p0, right0);
        put_u64(p1, left1); put_u64(p1, right1);
        append_checked(p0, key0.data(), key0.size());
        append_checked(p1, key1.data(), key1.size());
        append_pair({kSelectPurpose, iteration, kSelectDisjointSubstageScope,
                     kSelectLeftStrictRight, left, right}, p0, p1);
      }
    }
  }

  for (std::uint32_t item = 0; item < padded_n; ++item) {
    const auto mask_tau = random_u64() & ring;
    const auto mask_item = random_u64() & ring;
    std::uint64_t tau0, tau1, item0, item1;
    share_mask(mask_tau, comparison_bits, tau0, tau1);
    share_mask(mask_item, comparison_bits, item0, item1);
    ProtocolIUcmpMaterial generated(comparison_bits, mask_tau, mask_item,
                                    config.dcf_abi);
    const auto key0 = generated.export_party_material(0).serialize();
    const auto key1 = generated.export_party_material(1).serialize();
    Bytes p0, p1;
    put_u64(p0, tau0); put_u64(p0, item0);
    put_u64(p1, tau1); put_u64(p1, item1);
    append_checked(p0, key0.data(), key0.size());
    append_checked(p1, key1.data(), key1.size());
    append_pair({kMembershipPurpose, kMembershipIteration,
                 kMembershipSubstage, kMembershipTauStrictItem,
                 UINT32_MAX, item}, p0, p1);
  }

  writers[0]->finalize_staging();
  writers[1]->finalize_staging();
}

void protocol_i_project_derived_select5_publish_streamed_material_ready(
    const std::array<std::string, 2>& party_store_directories,
    const ProtocolIProjectDerivedSelect5DealerConfig& config) {
  require(config.logical_n >= 1 && config.logical_n <= 256 &&
              config.k >= 1 && config.k <= config.logical_n,
          "Select5 streamed ready dimensions");
  (void)package_version_for(config.dcf_abi, config.score_share_abi);
  const auto padded_n = padded_size(config.logical_n);
  const auto slot_count = streamed_slot_count(padded_n, config.score_share_abi);
  for (std::uint8_t party = 0; party < 2; ++party) {
    ProtocolIMaterialSlotBinding binding;
    binding.session = config.session;
    binding.fingerprint = config.fingerprint;
    binding.material_id = config.material_id_base;
    binding.logical_n = config.logical_n;
    binding.padded_n = padded_n;
    binding.k = config.k;
    binding.comparison_bits = static_cast<std::uint8_t>(
        33U + index_bits_for(padded_n));
    binding.party = party;
    binding.dcf_abi = config.dcf_abi;
    binding.score_share_abi = config.score_share_abi;
    binding.expected_slot_count = slot_count;
    protocol_i_streamed_material_publish_ready(
        party_store_directories[party], binding);
  }
}

void protocol_i_project_derived_select5_stage_material_packages(
    const std::array<std::string, 2>& party_store_directories,
    const ProtocolIProjectDerivedSelect5DealerConfig& config,
    const ProtocolIProjectDerivedSelect5DealerOutput& output) {
  require(party_store_directories[0] != party_store_directories[1] &&
              !party_store_directories[0].empty() &&
              !party_store_directories[1].empty() &&
              config.session != 0 && config.fingerprint != 0 &&
              config.material_id_base != 0 &&
              config.material_id_base < UINT64_MAX - 1U &&
              config.k >= 1 && config.k <= config.logical_n &&
              config.timeout_ms > 0 && !output.party0_package.empty() &&
              !output.party1_package.empty() &&
              output.party0_package.size() <= kProtocolIProjectDerivedSelect5MaxPackageBytes &&
              output.party1_package.size() <= kProtocolIProjectDerivedSelect5MaxPackageBytes,
          "Select5 party-scoped staged material parameters");
  const auto stem = store_stem(config.session, config.fingerprint,
                               config.material_id_base);
  const std::array<const std::vector<std::uint8_t>*, 2> packages{{
      &output.party0_package, &output.party1_package}};
  for (std::uint8_t party = 0; party < 2; ++party) {
    const int directory_fd = open_store_directory(party_store_directories[party]);
    try {
      const auto& package = *packages[party];
      const auto name = stem + "_p" + std::to_string(party) + ".pkg";
      write_new_file(directory_fd, name, package.data(), package.size(), 0400);
      ::close(directory_fd);
    } catch (...) {
      ::close(directory_fd);
      throw;
    }
  }
}

void protocol_i_project_derived_select5_publish_material_ready(
    const std::array<std::string, 2>& party_store_directories,
    const ProtocolIProjectDerivedSelect5DealerConfig& config) {
  require(party_store_directories[0] != party_store_directories[1] &&
              !party_store_directories[0].empty() &&
              !party_store_directories[1].empty() && config.session != 0 &&
              config.fingerprint != 0 && config.material_id_base != 0 &&
              config.material_id_base < UINT64_MAX - 1U &&
              config.k >= 1 && config.k <= config.logical_n &&
              config.timeout_ms > 0,
          "Select5 party-scoped ready marker parameters");
  ReadyRecord ready;
  ready.session = config.session;
  ready.fingerprint = config.fingerprint;
  ready.material_id = config.material_id_base;
  ready.logical_n = config.logical_n;
  ready.padded_n = padded_size(config.logical_n);
  ready.k = config.k;
  ready.bits = static_cast<std::uint8_t>(33U + index_bits_for(ready.padded_n));
  const auto stem = store_stem(config.session, config.fingerprint,
                               config.material_id_base);
  std::array<PackageDigest, 2> digests{};
  for (std::uint8_t party = 0; party < 2; ++party) {
    const int directory_fd = open_store_directory(party_store_directories[party]);
    const auto name = stem + "_p" + std::to_string(party) + ".pkg";
    const int package_fd = ::openat(directory_fd, name.c_str(),
        O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (package_fd < 0) {
      ::close(directory_fd);
      throw std::runtime_error("Select5 party-scoped staged package missing");
    }
    try {
      digests[party] = digest_fd(package_fd);
      std::array<std::uint8_t, 44> header{};
      pread_all_fd(package_fd, header.data(), header.size(), 0);
      require(header[0] == 'M' && header[1] == '6' && header[2] == 'B' &&
                  header[3] == '5' &&
                  header[4] == package_version_for(
                          config.dcf_abi, config.score_share_abi) &&
                  header[5] == party &&
                  header[7] == static_cast<std::uint8_t>(config.dcf_abi),
              "Select5 party-scoped staged package header");
      Bytes header_bytes(header.begin(), header.end());
      std::size_t offset = 8;
      require(get_u64(header_bytes, offset) == ready.session &&
                  get_u64(header_bytes, offset) == ready.fingerprint &&
                  get_u64(header_bytes, offset) == ready.material_id &&
                  get_u32(header_bytes, offset) == ready.logical_n &&
                  get_u32(header_bytes, offset) == ready.padded_n &&
                  get_u32(header_bytes, offset) == ready.k &&
                  header[6] == ready.bits,
              "Select5 party-scoped staged package identity");
      ::close(package_fd);
      ::close(directory_fd);
    } catch (...) {
      ::close(package_fd);
      ::close(directory_fd);
      throw;
    }
  }
  for (std::uint8_t party = 0; party < 2; ++party) {
    ready.party = party;
    ready.package = digests[party];
    const auto encoded = encode_ready(ready);
    const int directory_fd = open_store_directory(party_store_directories[party]);
    try {
      write_new_file(directory_fd, stem + ".ready", encoded.data(),
                     encoded.size(), 0400);
      ::close(directory_fd);
    } catch (...) {
      ::close(directory_fd);
      throw;
    }
  }
}

std::vector<std::uint8_t>
protocol_i_project_derived_select5_claim_party_package(
    const std::string& store_directory,
    std::uint64_t session, std::uint64_t fingerprint,
    std::uint64_t material_id_base, std::uint32_t logical_n,
    std::uint32_t padded_n, std::uint32_t k,
    std::uint8_t comparison_bits, std::uint8_t party) {
  require(session != 0 && fingerprint != 0 && material_id_base != 0 &&
              party <= 1 && logical_n >= 1 &&
              logical_n <= kProtocolIProjectDerivedSelect5MaxN &&
              padded_n == padded_size(logical_n) && k >= 1 && k <= logical_n &&
              comparison_bits == 33U + index_bits_for(padded_n),
          "Select5 material claim parameters");
  const auto stem = store_stem(session, fingerprint, material_id_base);
  const int directory_fd = open_store_directory(store_directory);
  try {
    const auto marker = read_exact_at(directory_fd, stem + ".ready", kReadyBytes);
    const auto ready = decode_ready(marker);
    require(ready.session == session && ready.fingerprint == fingerprint &&
                ready.material_id == material_id_base &&
                ready.logical_n == logical_n && ready.padded_n == padded_n &&
                ready.k == k && ready.bits == comparison_bits &&
                ready.party == party,
            "Select5 material ready binding");

    const auto package_name = stem + "_p" + std::to_string(party) + ".pkg";
    const int package_fd = ::openat(directory_fd, package_name.c_str(),
        O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (package_fd < 0)
      throw std::runtime_error("Select5 party package unavailable");
    try {
      const auto digest = digest_fd(package_fd);
      const auto& committed = ready.package;
      require(digest.size == committed.size &&
                  digest.sha256 == committed.sha256,
              "Select5 party package integrity");

      Bytes claim_record{'M', '6', 'C', '1'};
      put_u32(claim_record, 1);
      put_u64(claim_record, session);
      put_u64(claim_record, fingerprint);
      put_u64(claim_record, material_id_base);
      claim_record.push_back(party);
      claim_record.insert(claim_record.end(), 3, 0);
      append_checked(claim_record, digest.sha256.data(), digest.sha256.size());
      const auto lease_name = stem + "_p" + std::to_string(party) + ".lease";
      write_new_file(directory_fd, lease_name, claim_record.data(),
                     claim_record.size(), 0400);

      std::vector<std::uint8_t> package(
          static_cast<std::size_t>(committed.size));
      pread_all_fd(package_fd, package.data(), package.size(), 0);
      const auto after_read = digest_fd(package_fd);
      require(after_read.size == committed.size &&
                  after_read.sha256 == committed.sha256,
              "Select5 package changed after durable claim");
      ::close(package_fd);
      ::close(directory_fd);
      return package;
    } catch (...) {
      ::close(package_fd);
      throw;
    }
  } catch (...) {
    ::close(directory_fd);
    throw;
  }
}

ProtocolIProjectDerivedSelect5PartyOutput run_party_with_material(
    const ProtocolIProjectDerivedSelect5RuntimeConfig&,
    ProtocolIProjectDerivedSelect5PartyMaterial&&,
    const std::vector<std::uint32_t>&,
    const ProtocolIProjectDerivedSelect5Fds&,
    bool corrupt_first_sample_seed_for_test = false);

ProtocolIProjectDerivedSelect5PartyOutput
protocol_i_project_derived_select5_party_test_material(
    const ProtocolIProjectDerivedSelect5RuntimeConfig& config,
    ProtocolIProjectDerivedSelect5PartyMaterial&& material,
    const std::vector<std::uint32_t>& raw_score_share,
    const ProtocolIProjectDerivedSelect5Fds& fds,
    bool corrupt_first_sample_seed_for_test) {
  require(config.allow_plaintext_test_transport ||
              (fds.package_binding.ssl != nullptr &&
               !config.expected_peer_identity.empty()),
          "direct material injection is TEST_ONLY");
  if (corrupt_first_sample_seed_for_test)
    require(config.party == 0, "sample seed fault injection is P0 TEST_ONLY");
  if (!config.material_store_directory.empty()) {
    durable_session_claim(config);
    test_crash_after_output_step(config, 1);
  }
  return run_party_with_material(config, std::move(material),
                                 raw_score_share, fds,
                                 corrupt_first_sample_seed_for_test);
}

ProtocolIProjectDerivedSelect5PartyOutput
protocol_i_project_derived_select5_party(
    const ProtocolIProjectDerivedSelect5RuntimeConfig& config,
    const std::vector<std::uint32_t>& raw_score_share,
    const ProtocolIProjectDerivedSelect5Fds& fds) {
  require(!config.allow_plaintext_test_transport &&
              !config.material_store_directory.empty() &&
              (config.logical_n <= 128 || config.streamed_material_store),
          "durable material store and TLS are required");
  if (config.streamed_material_store) {
    auto material = [&]() {
      try {
        durable_session_claim(config);
        test_crash_after_output_step(config, 1);
        auto reader = std::make_shared<ProtocolIStreamedMaterialReader>(
            config.material_store_directory, streamed_binding(config));
        return load_streamed_party_material(config, reader);
      } catch (...) {
        abort_best_effort(config, fds);
        throw;
      }
    }();
    return run_party_with_material(config, std::move(material),
                                   raw_score_share, fds);
  }
  auto material = [&]() {
    try {
      durable_session_claim(config);
      test_crash_after_output_step(config, 1);
      const auto package = protocol_i_project_derived_select5_claim_party_package(
          config.material_store_directory, config.session, config.fingerprint,
          config.material_id_base, config.logical_n, config.padded_n, config.k,
          config.comparison_bits, config.party);
      return protocol_i_project_derived_select5_deserialize_party_material(
          package, config.party);
    } catch (...) {
      abort_best_effort(config, fds);
      throw;
    }
  }();
  return run_party_with_material(config, std::move(material),
                                 raw_score_share, fds, false);
}

ProtocolIProjectDerivedSelect5PartyOutput run_party_with_material(
    const ProtocolIProjectDerivedSelect5RuntimeConfig& config,
    ProtocolIProjectDerivedSelect5PartyMaterial&& material,
    const std::vector<std::uint32_t>& raw_score_share,
    const ProtocolIProjectDerivedSelect5Fds& fds,
    bool corrupt_first_sample_seed_for_test) {
  ProtocolIProjectDerivedSelect5Metrics metrics;
  bool final_status_channel_used = false;
  try {
    validate_runtime_material(config, material, raw_score_share, fds);
    if (!material.streamed_material_reader) claim_select_material(material);

    Bytes binding;
    put_u32(binding, UINT32_C(0x4d364235));  // M6B5 schema magic
    // Binding v5 selects the new score-share law and cannot interoperate with
    // either frozen v1 package layout or the BCG21 v3 layout.
    put_u32(binding, config.score_share_abi ==
                             ProtocolIScoreShareAbi::FullKeyZeroSumRefreshV2
                         ? 5U
                         : (config.dcf_abi == ProtocolIDcfAbi::LegacyM2UcV1
                                ? 3U : 4U));
    put_u64(binding, material.material_id_base);
    put_u32(binding, material.logical_n);
    put_u32(binding, material.padded_n);
    put_u32(binding, material.k);
    binding.push_back(material.comparison_bits);
    if (config.dcf_abi == ProtocolIDcfAbi::LegacyM2UcV1) {
      binding.insert(binding.end(), 3U, 0U);
    } else {
      binding.push_back(static_cast<std::uint8_t>(config.dcf_abi));
      if (config.score_share_abi ==
          ProtocolIScoreShareAbi::FullKeyZeroSumRefreshV2) {
        binding.push_back(2U);
        binding.push_back(0U);
      } else {
        binding.insert(binding.end(), 2U, 0U);
      }
    }
    const auto binding_exchange = ordered_exchange(
        fds.package_binding, config, 3, binding);
    require(binding_exchange.peer == binding,
            "Select5 parties received different package bindings");
    metrics.binding_sent_bytes = binding_exchange.sent;
    metrics.binding_received_bytes = binding_exchange.received;

    const auto index_bits = index_bits_for(config.padded_n);
    ProtocolIScoreInputConfig input_config{
        config.session, config.fingerprint, config.logical_n, config.padded_n,
        config.k, index_bits, config.comparison_bits, config.party,
        config.timeout_ms};
    input_config.material_id = config.material_id_base;
    ProtocolIScoreInputMetrics input_metrics;
    auto keys = protocol_i_raw_score_input_party(
        input_config, material.score_input, raw_score_share, fds.score_input,
        &input_metrics);
    if (config.score_share_abi ==
        ProtocolIScoreShareAbi::FullKeyZeroSumRefreshV2) {
      protocol_i_refresh_full_priority_key_shares_v2(
          keys, material.score_key_refresh_shares,
          config.comparison_bits);
      material.score_key_refresh_shares.clear();
    }
    metrics.input_sent_bytes = input_metrics.carry_sent_bytes +
                               input_metrics.sign_sent_bytes;
    metrics.input_received_bytes = input_metrics.carry_received_bytes +
                                   input_metrics.sign_received_bytes;
    const auto ring = mask_for(config.comparison_bits);
    for (const auto key : keys)
      require((key & ~ring) == 0, "Select5 priority share outside ring");

    std::vector<ProtocolIBlock192> key_records(config.padded_n);
    for (std::uint32_t i = 0; i < config.padded_n; ++i)
      key_records[i] = {keys[i], 0, 0};
    auto forward = run_two_round_shuffle(
        config, std::move(material.forward_shuffle), material.material_id_base,
        key_records, fds.forward_shuffle[0], fds.forward_shuffle[1], 1, 2);
    trace_test_wire_event(config, "forward-shuffle-complete", kForwardPhase2,
                          UINT8_MAX);
    metrics.forward_sent_bytes = forward.sent;
    metrics.forward_received_bytes = forward.received;
    std::vector<std::uint64_t> shuffled_keys(config.padded_n);
    for (std::uint32_t i = 0; i < config.padded_n; ++i)
      shuffled_keys[i] = forward.result.shuffled_share[i].word0;
    ProjectDerivedSelect5DisclosureState sample_disclosure;
    sample_disclosure.mark_forward_shuffle_complete();
    bool injected_seed_fault = false;

    const auto pairs = edge_count(config.padded_n);
    std::vector<bool> consumed(material.select_slots.size(), false);
    std::uint32_t layer_sequence = 0;
    auto compare_wave = [&](std::uint8_t iteration, std::uint8_t substage,
                            const std::vector<ProjectDerivedSelect5Edge>& edges) {
      require(layer_sequence < 9, "Select5 active layer count");
      const auto current_layer = layer_sequence++;
      if (edges.empty()) return std::vector<std::uint8_t>{};
      require(edges.size() <= config.padded_n * (config.padded_n - 1U) / 2U,
              "Select5 edge layer size");
      Bytes operands;
      operands.reserve(edges.size() * 16U);
      std::vector<ProtocolIUcmpPartyMaterial> streamed_keys;
      streamed_keys.reserve(edges.size());
      std::vector<ProtocolIUcmpPartyMaterial*> edge_keys;
      edge_keys.reserve(edges.size());
      for (const auto& edge : edges) {
        require(edge.left < edge.right && edge.right < config.padded_n,
                "Select5 runtime edge coordinates");
        if (material.streamed_material_reader) {
          const auto address = ProtocolIMaterialSlotAddress{
              kSelectPurpose, iteration, kSelectDisjointSubstageScope,
              kSelectLeftStrictRight, edge.left, edge.right};
          const auto bytes = claim_streamed_slot(
              material.streamed_material_reader, address);
          std::size_t offset = 0;
          const auto left_mask_share = get_u64(bytes, offset);
          const auto right_mask_share = get_u64(bytes, offset);
          Bytes encoded(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                        bytes.end());
          streamed_keys.push_back(
              ProtocolIUcmpPartyMaterial::deserialize(encoded));
          auto& key = streamed_keys.back();
          require(key.party_id() == config.party &&
                      key.comparison_bits() == config.comparison_bits &&
                      key.dcf_abi() == config.dcf_abi,
                  "Select5 streamed edge key binding");
          edge_keys.push_back(&key);
          put_u64(operands, (shuffled_keys[edge.left] + left_mask_share) & ring);
          put_u64(operands, (shuffled_keys[edge.right] + right_mask_share) & ring);
        } else {
          const auto address = static_cast<std::size_t>(
              iteration * pairs + pair_index(config.padded_n,
                                             edge.left, edge.right));
          require(address < material.select_slots.size() && !consumed[address],
                  "Select5 duplicate/replayed edge slot");
          auto& slot = material.select_slots[address];
          require(slot.iteration == iteration && slot.left == edge.left &&
                      slot.right == edge.right && slot.purpose == kSelectPurpose &&
                      slot.substage_scope == kSelectDisjointSubstageScope &&
                      slot.direction == kSelectLeftStrictRight,
                  "Select5 selected edge slot binding");
          consumed[address] = true;
          edge_keys.push_back(&slot.material);
          put_u64(operands, (shuffled_keys[edge.left] +
                             slot.left_mask_share) & ring);
          put_u64(operands, (shuffled_keys[edge.right] +
                             slot.right_mask_share) & ring);
        }
      }

      const auto phase = static_cast<std::uint8_t>(10U + 2U * current_layer);
      auto operand_exchange = ordered_exchange(
          fds.select_layers[2U * current_layer], config, phase, operands);
      metrics.select_sent_bytes += operand_exchange.sent;
      metrics.select_received_bytes += operand_exchange.received;
      const auto local_operands = decode_words(operands, edges.size() * 2U);
      const auto peer_operands = decode_words(operand_exchange.peer,
                                               edges.size() * 2U);
      std::vector<std::uint64_t> local_results(edges.size());
      for (std::size_t i = 0; i < edges.size(); ++i) {
        const auto z_left = (local_operands[2U * i] + peer_operands[2U * i]) & ring;
        const auto z_right = (local_operands[2U * i + 1U] +
                              peer_operands[2U * i + 1U]) & ring;
        local_results[i] = edge_keys[i]->eval_strict_lt(z_left, z_right);
        metrics.select_dcf_eval_calls += 2U;
      }
      auto result_exchange = ordered_exchange(
          fds.select_layers[2U * current_layer + 1U], config,
          static_cast<std::uint8_t>(phase + 1U), encode_words(local_results));
      metrics.select_sent_bytes += result_exchange.sent;
      metrics.select_received_bytes += result_exchange.received;
      const auto peer_results = decode_words(result_exchange.peer, edges.size());
      std::vector<std::uint8_t> outcomes(edges.size());
      for (std::size_t i = 0; i < edges.size(); ++i) {
        const auto bit = local_results[i] + peer_results[i];
        require(bit <= 1, "Select5 opened comparator output is not a bit");
        outcomes[i] = static_cast<std::uint8_t>(bit);
      }
      metrics.comparison_calls += edges.size();
      ++metrics.active_compare_layers;
      (void)substage;  // the public scope binds both disjoint sublayers
      return outcomes;
    };

    auto sample_source = [&](std::uint8_t iteration,
                             const std::vector<std::uint32_t>& candidate,
                             std::uint32_t sample_size) {
      require(iteration < fds.sample_schedule.size(),
              "Select5 sample iteration binding");
      sample_disclosure.begin_active_iteration(iteration);
      ProjectDerivedSelect5PublicSeed local_seed{};
      if (config.party == 0) random_bytes(local_seed.data(), local_seed.size());
      std::vector<std::uint32_t> local_sample;
      ProtocolIFramedChannel channel(
          fds.sample_schedule[iteration],
          {config.session, config.fingerprint, config.padded_n, config.k,
           config.comparison_bits, config.party,
           static_cast<std::uint8_t>(1U - config.party),
           static_cast<std::uint8_t>(kSampleSchedulePhaseBase + iteration),
           kFrameType, config.material_id_base},
          ProtocolIFramedChannelOptions{config.timeout_ms});
      if (config.party == 0) {
        auto seed_frame =
            protocol_i_project_derived_select5_encode_public_sample_seed(
                iteration, local_seed);
        if (corrupt_first_sample_seed_for_test && !injected_seed_fault &&
            iteration == 0) {
          seed_frame[0] ^= 1U;
          injected_seed_fault = true;
        }
        channel.send(seed_frame);
        trace_test_wire_event(config, "sample-seed-send",
            static_cast<std::uint8_t>(kSampleSchedulePhaseBase + iteration),
            iteration, seed_frame.size());
        const auto peer_ack = channel.receive();
        require(peer_ack == encode_sample_seed_ack(iteration),
                "Select5 public sample seed acknowledgment binding");
      } else {
        const auto peer_seed = channel.receive();
        local_seed =
            protocol_i_project_derived_select5_decode_public_sample_seed(
                peer_seed, iteration);
        trace_test_wire_event(config, "sample-seed-receive",
            static_cast<std::uint8_t>(kSampleSchedulePhaseBase + iteration),
            iteration, peer_seed.size());
        channel.send(encode_sample_seed_ack(iteration));
      }
      metrics.sample_schedule_sent_bytes += channel.sent_bytes();
      metrics.sample_schedule_received_bytes += channel.received_bytes();
      ++metrics.active_sample_rounds;
      local_sample =
          protocol_i_project_derived_select5_sample_from_public_seed(
              candidate, sample_size, local_seed, config.session,
              config.fingerprint, iteration);
      return local_sample;
    };

    const auto selected = protocol_i_project_derived_select5_control_with_sampler(
        config.padded_n, config.k, sample_source, compare_wave);
    require(selected.iterations.size() == 5 &&
                selected.selected_position < config.padded_n,
            "Select5 public controller completion");
    const auto tau_share = shuffled_keys[selected.selected_position];

    Bytes membership_operands;
    membership_operands.reserve(static_cast<std::size_t>(config.padded_n) * 16U);
    for (std::uint32_t i = 0; i < config.padded_n; ++i) {
      const auto& slot = material.membership_slots[i];
      put_u64(membership_operands, (tau_share + slot.tau_mask_share) & ring);
      put_u64(membership_operands, (shuffled_keys[i] + slot.item_mask_share) & ring);
    }
    const auto membership_exchange = ordered_exchange(
        fds.membership, config, kMembershipPhase, membership_operands);
    metrics.membership_sent_bytes = membership_exchange.sent;
    metrics.membership_received_bytes = membership_exchange.received;
    const auto peer_membership = decode_words(
        membership_exchange.peer, static_cast<std::size_t>(config.padded_n) * 2U);
    const auto local_membership = decode_words(
        membership_operands, static_cast<std::size_t>(config.padded_n) * 2U);

    std::vector<ProtocolIBlock192> carrier(config.padded_n);
    for (std::uint32_t i = 0; i < config.padded_n; ++i) {
      const auto z_tau = (local_membership[2U * i] +
                          peer_membership[2U * i]) & ring;
      const auto z_item = (local_membership[2U * i + 1U] +
                           peer_membership[2U * i + 1U]) & ring;
      auto& slot = material.membership_slots[i];
      const auto lt_share = slot.material.eval_strict_lt(z_tau, z_item);
      metrics.membership_dcf_eval_calls += 2U;
      const auto selected_share = config.party == 0
          ? UINT64_C(1) - lt_share
          : UINT64_C(0) - lt_share;
      carrier[i] = {0, selected_share, 0};
    }

    auto inverse = run_two_round_shuffle(
        config, std::move(material.inverse_shuffle),
        material.material_id_base + 1U, carrier,
        fds.inverse_shuffle[0], fds.inverse_shuffle[1], 3, 4);
    metrics.inverse_sent_bytes = inverse.sent;
    metrics.inverse_received_bytes = inverse.received;
    std::vector<std::uint8_t> output_share(config.logical_n);
    for (std::uint32_t i = 0; i < config.logical_n; ++i)
      output_share[i] = static_cast<std::uint8_t>(
          inverse.result.shuffled_share[i].word1 & 1U);

    // Both parties cross PREPARE and COMMIT before this API returns its local
    // XOR share. A crash may orphan a local share, but no one party receives
    // the complete mask. Run/output markers prevent restart and republishing.
    durable_output_phase(config, ".prepared", ".run");
    test_crash_after_output_step(config, 2);
    final_status_channel_used = true;
    FinalStatusTransaction status(fds.final_status, config);
    const auto prepare_peer = status.prepare();
    require(prepare_peer.size() == 1 && prepare_peer[0] == 1,
            "Select5 peer PREPARE/no-mask");
    test_crash_after_output_step(config, 3);
    durable_output_phase(config, ".commit", ".prepared");
    test_crash_after_output_step(config, 4);
    const auto commit_peer = status.commit();
    require(commit_peer.size() == 1 && commit_peer[0] == 2,
            "Select5 peer COMMIT/no-mask");
    test_crash_after_output_step(config, 5);
    durable_output_phase(config, ".published", ".commit");
    test_crash_after_output_step(config, 6);
    metrics.final_status_sent_bytes = status.sent();
    metrics.final_status_received_bytes = status.received();
    metrics.online_rounds = 9U + 2U * metrics.active_compare_layers +
                            metrics.active_sample_rounds + 1U;
    return {std::move(output_share), metrics};
  } catch (...) {
    if (!final_status_channel_used) abort_best_effort(config, fds);
    throw;
  }
}

void protocol_i_project_derived_select5_abort_no_mask(
    const ProtocolIProjectDerivedSelect5RuntimeConfig& config,
    const ProtocolIProjectDerivedSelect5Fds& fds) {
  require(config.party <= 1 && config.session != 0 &&
              config.fingerprint != 0 && config.padded_n >= 2 &&
              config.padded_n <= kProtocolIProjectDerivedSelect5MaxN &&
              config.k >= 1 && config.k <= config.logical_n &&
              config.comparison_bits == 33U + index_bits_for(config.padded_n) &&
              config.timeout_ms > 0 && fds.final_status.fd >= 0 &&
              config.material_id_base != 0,
          "Select5 ABORT config");
  if (!config.allow_plaintext_test_transport) {
    require(fds.final_status.ssl != nullptr &&
                !config.expected_peer_identity.empty() &&
                fds.final_status.expected_peer_identity ==
                    config.expected_peer_identity,
            "Select5 ABORT requires authenticated TLS endpoint");
  } else {
    require(fds.final_status.ssl == nullptr,
            "Select5 TEST_ONLY ABORT must not mix TLS and plaintext");
  }
  const auto status = ordered_exchange(fds.final_status, config,
                                       kFinalAbortPhase, Bytes{0});
  require(status.peer.size() == 1 && status.peer[0] == 0,
          "Select5 peer success conflicts with ABORT");
}

}  // namespace moe_topk
