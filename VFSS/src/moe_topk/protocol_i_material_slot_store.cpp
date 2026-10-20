#include <moe_topk/protocol_i_material_slot_store.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <map>
#include <openssl/evp.h>
#include <set>
#include <stdexcept>
#include <set>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace moe_topk {
namespace {

constexpr std::uint64_t kManifestCap = 64U * 1024U * 1024U;
constexpr std::uint64_t kSlotCap = 1U * 1024U * 1024U;
constexpr std::uint64_t kDefaultSegment = 16U * 1024U * 1024U;
constexpr std::size_t kManifestHeaderBytes = 64;
constexpr std::size_t kManifestEntryBytes = 76;
constexpr std::size_t kReadyBytes = 80;

struct Digest {
  std::array<std::uint8_t, 32> bytes{};
};

struct Entry {
  ProtocolIMaterialSlotAddress address;
  std::uint32_t segment = 0;
  std::uint64_t offset = 0;
  std::uint64_t length = 0;
  Digest digest;
};

void require(bool ok, const char* message) {
  if (!ok) throw std::invalid_argument(message);
}

std::uint32_t material_schema_version(
    const ProtocolIMaterialSlotBinding& binding) {
  if (binding.dcf_abi == ProtocolIDcfAbi::LegacyM2UcV1) {
    require(binding.score_share_abi ==
                ProtocolIScoreShareAbi::S15InducedV1,
            "legacy streamed ABI cannot carry F_scoreconv/v2");
    return 1U;
  }
  require(binding.dcf_abi == ProtocolIDcfAbi::Bcg21Fig1Aes128V1,
          "streamed material DCF ABI");
  if (binding.score_share_abi == ProtocolIScoreShareAbi::S15InducedV1)
    return 2U;
  require(binding.score_share_abi ==
              ProtocolIScoreShareAbi::FullKeyZeroSumRefreshV2,
          "streamed material score-share ABI");
  return 3U;
}

void put_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
  for (int shift = 24; shift >= 0; shift -= 8)
    out.push_back(static_cast<std::uint8_t>(value >> shift));
}

void put_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
  for (int shift = 56; shift >= 0; shift -= 8)
    out.push_back(static_cast<std::uint8_t>(value >> shift));
}

std::uint32_t get_u32(const std::vector<std::uint8_t>& in, std::size_t& pos) {
  require(pos <= in.size() && in.size() - pos >= 4, "slot store truncated u32");
  std::uint32_t value = 0;
  for (int i = 0; i < 4; ++i) value = (value << 8) | in[pos++];
  return value;
}

std::uint64_t get_u64(const std::vector<std::uint8_t>& in, std::size_t& pos) {
  require(pos <= in.size() && in.size() - pos >= 8, "slot store truncated u64");
  std::uint64_t value = 0;
  for (int i = 0; i < 8; ++i) value = (value << 8) | in[pos++];
  return value;
}

void append_bytes(std::vector<std::uint8_t>& out, const std::uint8_t* data,
                  std::size_t length) {
  require(length <= kManifestCap && out.size() <= kManifestCap - length,
          "slot store manifest exceeds 64 MiB");
  out.insert(out.end(), data, data + length);
}

void get_bytes(const std::vector<std::uint8_t>& in, std::size_t& pos,
               std::uint8_t* out, std::size_t length) {
  require(pos <= in.size() && length <= in.size() - pos,
          "slot store truncated bytes");
  if (length != 0) std::memcpy(out, in.data() + pos, length);
  pos += length;
}

Digest sha256(const std::uint8_t* data, std::size_t size) {
  Digest result;
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) throw std::runtime_error("slot store SHA-256 context");
  unsigned int length = 0;
  const bool ok = EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1 &&
      EVP_DigestUpdate(ctx, data, size) == 1 &&
      EVP_DigestFinal_ex(ctx, result.bytes.data(), &length) == 1 &&
      length == result.bytes.size();
  EVP_MD_CTX_free(ctx);
  if (!ok) throw std::runtime_error("slot store SHA-256");
  return result;
}

Digest sha256_fd_range(int fd, std::uint64_t offset, std::uint64_t length) {
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) throw std::runtime_error("slot store SHA-256 context");
  bool ok = EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1;
  std::array<std::uint8_t, 65536> buffer{};
  while (ok && length != 0) {
    const auto want = static_cast<std::size_t>(std::min<std::uint64_t>(
        buffer.size(), length));
    const auto count = ::pread(fd, buffer.data(), want,
                               static_cast<off_t>(offset));
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) { ok = false; break; }
    ok = EVP_DigestUpdate(ctx, buffer.data(),
                          static_cast<std::size_t>(count)) == 1;
    offset += static_cast<std::uint64_t>(count);
    length -= static_cast<std::uint64_t>(count);
  }
  Digest result;
  unsigned int digest_length = 0;
  ok = ok && EVP_DigestFinal_ex(ctx, result.bytes.data(), &digest_length) == 1 &&
       digest_length == result.bytes.size();
  EVP_MD_CTX_free(ctx);
  if (!ok) throw std::runtime_error("slot store range digest");
  return result;
}

void write_all(int fd, const std::uint8_t* data, std::size_t size) {
  while (size != 0) {
    const auto count = ::write(fd, data, size);
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) throw std::runtime_error("slot store write");
    data += count;
    size -= static_cast<std::size_t>(count);
  }
}

void pread_all(int fd, std::uint8_t* data, std::size_t size,
               std::uint64_t offset) {
  while (size != 0) {
    const auto count = ::pread(fd, data, size, static_cast<off_t>(offset));
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) throw std::runtime_error("slot store pread");
    data += count;
    size -= static_cast<std::size_t>(count);
    offset += static_cast<std::uint64_t>(count);
  }
}

int open_directory(const std::string& path) {
  require(!path.empty(), "slot store path");
  const int fd = ::open(path.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  if (fd < 0) throw std::runtime_error("slot store directory open");
  struct stat status{};
  if (::fstat(fd, &status) != 0 || !S_ISDIR(status.st_mode) ||
      status.st_uid != ::geteuid() || (status.st_mode & 0077) != 0) {
    ::close(fd);
    throw std::runtime_error("slot store directory owner/mode");
  }
  return fd;
}

std::string stem(const ProtocolIMaterialSlotBinding& b) {
  require(b.session != 0 && b.fingerprint != 0 && b.material_id != 0 &&
              b.logical_n != 0 && b.padded_n >= b.logical_n && b.k != 0 &&
              b.k <= b.logical_n && b.comparison_bits >= 34 &&
              b.comparison_bits <= 53 && b.party <= 1 &&
              b.expected_slot_count != 0,
          "slot store binding");
  require(b.dcf_abi == ProtocolIDcfAbi::LegacyM2UcV1 ||
              b.dcf_abi == ProtocolIDcfAbi::Bcg21Fig1Aes128V1,
          "slot store DCF ABI");
  (void)material_schema_version(b);
  char name[96]{};
  const int count = b.dcf_abi == ProtocolIDcfAbi::LegacyM2UcV1
      ? std::snprintf(name, sizeof(name),
          "m6b5s2_%016llx_%016llx_%016llx_p%u",
          static_cast<unsigned long long>(b.session),
          static_cast<unsigned long long>(b.fingerprint),
          static_cast<unsigned long long>(b.material_id),
          static_cast<unsigned>(b.party))
      : std::snprintf(name, sizeof(name),
          "m6b5s2_b1_%016llx_%016llx_%016llx_p%u",
          static_cast<unsigned long long>(b.session),
          static_cast<unsigned long long>(b.fingerprint),
          static_cast<unsigned long long>(b.material_id),
          static_cast<unsigned>(b.party));
  require(count > 0 && static_cast<std::size_t>(count) < sizeof(name),
          "slot store filename");
  return name;
}

std::string segment_name(const std::string& base, std::uint32_t segment) {
  char suffix[24]{};
  const int count = std::snprintf(suffix, sizeof(suffix), ".%06u.seg", segment);
  require(count > 0 && static_cast<std::size_t>(count) < sizeof(suffix),
          "slot segment filename");
  return base + suffix;
}

std::string address_suffix(const ProtocolIMaterialSlotAddress& a) {
  char suffix[96]{};
  const int count = std::snprintf(suffix, sizeof(suffix),
      ".%08x.%08x.%08x.%08x.%08x.%08x", a.purpose, a.iteration,
      a.substage, a.direction, a.left, a.right);
  require(count > 0 && static_cast<std::size_t>(count) < sizeof(suffix),
          "slot lease filename");
  return suffix;
}

void validate_binding(const std::vector<std::uint8_t>& in,
                      const ProtocolIMaterialSlotBinding& b,
                      std::size_t& pos) {
  require(in.size() >= kManifestHeaderBytes && in[0] == 'M' &&
              in[1] == '6' && in[2] == 'S' && in[3] == '2',
          "slot manifest header");
  pos = 4;
  require(get_u32(in, pos) == material_schema_version(b),
          "slot manifest version");
  require(get_u64(in, pos) == b.session &&
              get_u64(in, pos) == b.fingerprint &&
              get_u64(in, pos) == b.material_id &&
              get_u32(in, pos) == b.logical_n &&
              get_u32(in, pos) == b.padded_n &&
              get_u32(in, pos) == b.k &&
              get_u64(in, pos) == b.expected_slot_count &&
              in[pos++] == b.comparison_bits &&
              in[pos++] == b.party &&
              in[pos++] == static_cast<std::uint8_t>(b.dcf_abi) &&
              in[pos++] == static_cast<std::uint8_t>(
                  b.score_share_abi ==
                          ProtocolIScoreShareAbi::S15InducedV1
                      ? 0U : 2U),
          "slot manifest binding mismatch");
}

std::vector<std::uint8_t> read_file_at(int dirfd, const std::string& name,
                                       std::uint64_t max_bytes) {
  const int fd = ::openat(dirfd, name.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
  if (fd < 0) throw std::runtime_error("slot store file missing");
  try {
    struct stat status{};
    require(::fstat(fd, &status) == 0 && S_ISREG(status.st_mode) &&
                status.st_uid == ::geteuid() && status.st_size >= 0 &&
                static_cast<std::uint64_t>(status.st_size) <= max_bytes,
            "slot store file shape");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(status.st_size));
    pread_all(fd, bytes.data(), bytes.size(), 0);
    ::close(fd);
    return bytes;
  } catch (...) {
    ::close(fd);
    throw;
  }
}

void write_new_file(int dirfd, const std::string& name,
                    const std::uint8_t* bytes, std::size_t size,
                    mode_t mode) {
  const int fd = ::openat(dirfd, name.c_str(),
      O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
  if (fd < 0) throw std::runtime_error("slot store file exists or cannot open");
  try {
    write_all(fd, bytes, size);
    if (::fchmod(fd, mode) != 0 || ::fsync(fd) != 0 || ::fsync(dirfd) != 0)
      throw std::runtime_error("slot store durable write");
    ::close(fd);
  } catch (...) {
    ::close(fd);
    throw;
  }
}

std::vector<std::uint8_t> encode_manifest(
    const ProtocolIMaterialSlotBinding& b, const std::vector<Entry>& entries) {
  auto ordered = entries;
  std::sort(ordered.begin(), ordered.end(), [](const Entry& a, const Entry& c) {
    return a.address < c.address;
  });
  std::vector<std::uint8_t> out{'M','6','S','2'};
  put_u32(out, material_schema_version(b));
  put_u64(out, b.session); put_u64(out, b.fingerprint);
  put_u64(out, b.material_id);
  put_u32(out, b.logical_n); put_u32(out, b.padded_n); put_u32(out, b.k);
  put_u64(out, b.expected_slot_count);
  out.push_back(b.comparison_bits); out.push_back(b.party);
  out.push_back(static_cast<std::uint8_t>(b.dcf_abi));
  out.push_back(static_cast<std::uint8_t>(
      b.score_share_abi == ProtocolIScoreShareAbi::S15InducedV1 ? 0U : 2U));
  put_u64(out, ordered.size());
  for (const auto& e : ordered) {
    put_u32(out, e.address.purpose); put_u32(out, e.address.iteration);
    put_u32(out, e.address.substage); put_u32(out, e.address.direction);
    put_u32(out, e.address.left); put_u32(out, e.address.right);
    put_u32(out, e.segment); put_u64(out, e.offset); put_u64(out, e.length);
    append_bytes(out, e.digest.bytes.data(), e.digest.bytes.size());
  }
  require(out.size() <= kManifestCap, "slot store manifest capacity");
  return out;
}

}  // namespace

struct ProtocolIStreamedMaterialWriter::Impl {
  std::string directory;
  ProtocolIMaterialSlotBinding binding;
  std::uint64_t segment_limit = 0, segment_size = 0;
  std::uint32_t segment = 0;
  int dirfd = -1, segment_fd = -1;
  std::string base;
  std::set<ProtocolIMaterialSlotAddress> addresses;
  std::vector<Entry> entries;
  bool finalized = false;

  Impl(std::string path, ProtocolIMaterialSlotBinding bound,
       std::uint64_t segment_bytes)
      : directory(std::move(path)), binding(bound), segment_limit(segment_bytes),
        dirfd(open_directory(directory)), base(stem(binding)) {
    require(segment_limit >= kSlotCap && segment_limit <= kManifestCap,
            "slot store segment size must be 1..64 MiB and fit a slot");
  }

  ~Impl() {
    if (segment_fd >= 0) ::close(segment_fd);
    if (dirfd >= 0) ::close(dirfd);
  }

  void open_segment() {
    const auto name = segment_name(base, segment++);
    segment_fd = ::openat(dirfd, name.c_str(),
        O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (segment_fd < 0) throw std::runtime_error("slot store segment create");
    segment_size = 0;
  }

  void close_segment() {
    if (segment_fd < 0) return;
    if (::fchmod(segment_fd, 0400) != 0 || ::fsync(segment_fd) != 0 ||
        ::fsync(dirfd) != 0)
      throw std::runtime_error("slot store segment fsync");
    ::close(segment_fd);
    segment_fd = -1;
  }
};

struct ProtocolIStreamedMaterialReader::Impl {
  ProtocolIMaterialSlotBinding binding;
  int dirfd = -1;
  std::string base;
  std::vector<Entry> entries;

  Impl(std::string directory, ProtocolIMaterialSlotBinding bound)
      : binding(bound), dirfd(open_directory(directory)), base(stem(binding)) {
    try {
      const auto ready = read_file_at(dirfd, base + ".ready", kReadyBytes);
      require(ready.size() == kReadyBytes && ready[0] == 'M' &&
                  ready[1] == '6' && ready[2] == 'R' && ready[3] == '2',
              "streamed material ready marker");
      std::size_t ready_pos = 4;
      require(get_u32(ready, ready_pos) == material_schema_version(binding) &&
                  get_u64(ready, ready_pos) == binding.session &&
                  get_u64(ready, ready_pos) == binding.fingerprint &&
                  get_u64(ready, ready_pos) == binding.material_id &&
                  ready[ready_pos++] == binding.party,
              "streamed ready binding");
      require(ready[ready_pos++] == static_cast<std::uint8_t>(binding.dcf_abi),
              "streamed ready DCF ABI");
      require(ready[ready_pos++] == static_cast<std::uint8_t>(
                  binding.score_share_abi ==
                          ProtocolIScoreShareAbi::S15InducedV1
                      ? 0U : 2U),
              "streamed ready score-share ABI");
      for (int i = 0; i < 5; ++i)
        require(ready[ready_pos++] == 0, "streamed ready reserved bytes");
      const auto manifest_size = get_u64(ready, ready_pos);
      Digest manifest_digest;
      get_bytes(ready, ready_pos, manifest_digest.bytes.data(),
                manifest_digest.bytes.size());
      const auto manifest = read_file_at(dirfd, base + ".manifest", kManifestCap);
      require(manifest.size() == manifest_size &&
                  sha256(manifest.data(), manifest.size()).bytes ==
                      manifest_digest.bytes,
              "streamed manifest digest");
      std::size_t pos = 0;
      validate_binding(manifest, binding, pos);
      const auto count = get_u64(manifest, pos);
      require(count == binding.expected_slot_count &&
                  count <= (manifest.size() - pos) / kManifestEntryBytes &&
                  manifest.size() == pos + count * kManifestEntryBytes,
              "streamed manifest count");
      entries.reserve(static_cast<std::size_t>(count));
      for (std::uint64_t index = 0; index < count; ++index) {
        Entry entry;
        entry.address.purpose = get_u32(manifest, pos);
        entry.address.iteration = get_u32(manifest, pos);
        entry.address.substage = get_u32(manifest, pos);
        entry.address.direction = get_u32(manifest, pos);
        entry.address.left = get_u32(manifest, pos);
        entry.address.right = get_u32(manifest, pos);
        entry.segment = get_u32(manifest, pos);
        entry.offset = get_u64(manifest, pos);
        entry.length = get_u64(manifest, pos);
        get_bytes(manifest, pos, entry.digest.bytes.data(),
                  entry.digest.bytes.size());
        require(entry.length != 0 && entry.length <= kSlotCap &&
                    entry.offset <= kManifestCap &&
                    entry.length <= kManifestCap - entry.offset &&
                    (entries.empty() || entries.back().address < entry.address),
                "streamed manifest order/address/range");
        entries.push_back(entry);
      }
      require(pos == manifest.size(), "streamed manifest trailing bytes");
    } catch (...) {
      ::close(dirfd);
      dirfd = -1;
      throw;
    }
  }

  ~Impl() {
    if (dirfd >= 0) ::close(dirfd);
  }

  std::vector<std::uint8_t> claim(
      const ProtocolIMaterialSlotAddress& address) {
    const auto it = std::lower_bound(entries.begin(), entries.end(), address,
        [](const Entry& entry, const ProtocolIMaterialSlotAddress& key) {
          return entry.address < key;
        });
    require(it != entries.end() && it->address == address,
            "streamed material slot absent");
    const auto& selected = *it;
    const auto name = segment_name(base, selected.segment);
    const int segfd = ::openat(dirfd, name.c_str(),
        O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (segfd < 0) throw std::runtime_error("streamed material segment missing");
    try {
      struct stat st{};
      require(::fstat(segfd, &st) == 0 && S_ISREG(st.st_mode) &&
                  st.st_uid == ::geteuid() && st.st_size >= 0 &&
                  (st.st_mode & 0222) == 0 &&
                  selected.offset <= static_cast<std::uint64_t>(st.st_size) &&
                  selected.length <= static_cast<std::uint64_t>(st.st_size) - selected.offset,
              "streamed material segment bounds");
      const auto before = sha256_fd_range(segfd, selected.offset, selected.length);
      require(before.bytes == selected.digest.bytes,
              "streamed material slot digest");
      std::vector<std::uint8_t> lease{'M','6','L','2'};
      put_u32(lease, 1);
      put_u64(lease, binding.session); put_u64(lease, binding.fingerprint);
      put_u64(lease, binding.material_id); lease.push_back(binding.party);
      lease.insert(lease.end(), 3, 0);
      put_u32(lease, address.purpose); put_u32(lease, address.iteration);
      put_u32(lease, address.substage); put_u32(lease, address.direction);
      put_u32(lease, address.left); put_u32(lease, address.right);
      append_bytes(lease, selected.digest.bytes.data(), selected.digest.bytes.size());
      write_new_file(dirfd, base + address_suffix(address) + ".lease",
                     lease.data(), lease.size(), 0400);
      std::vector<std::uint8_t> bytes(static_cast<std::size_t>(selected.length));
      pread_all(segfd, bytes.data(), bytes.size(), selected.offset);
      require(sha256(bytes.data(), bytes.size()).bytes == selected.digest.bytes,
              "streamed material changed after durable lease");
      ::close(segfd);
      return bytes;
    } catch (...) {
      ::close(segfd);
      throw;
    }
  }
};

ProtocolIStreamedMaterialReader::ProtocolIStreamedMaterialReader(
    std::string directory, ProtocolIMaterialSlotBinding binding)
    : impl_(new Impl(std::move(directory), binding)) {}
ProtocolIStreamedMaterialReader::~ProtocolIStreamedMaterialReader() = default;
ProtocolIStreamedMaterialReader::ProtocolIStreamedMaterialReader(
    ProtocolIStreamedMaterialReader&&) noexcept = default;
ProtocolIStreamedMaterialReader& ProtocolIStreamedMaterialReader::operator=(
    ProtocolIStreamedMaterialReader&&) noexcept = default;

std::vector<std::uint8_t> ProtocolIStreamedMaterialReader::claim_slot(
    const ProtocolIMaterialSlotAddress& address) {
  require(impl_ != nullptr, "streamed reader moved-from");
  return impl_->claim(address);
}

ProtocolIStreamedMaterialWriter::ProtocolIStreamedMaterialWriter(
    std::string directory, ProtocolIMaterialSlotBinding binding,
    std::uint64_t segment_bytes)
    : impl_(new Impl(std::move(directory), binding, segment_bytes)) {}
ProtocolIStreamedMaterialWriter::~ProtocolIStreamedMaterialWriter() = default;
ProtocolIStreamedMaterialWriter::ProtocolIStreamedMaterialWriter(
    ProtocolIStreamedMaterialWriter&&) noexcept = default;
ProtocolIStreamedMaterialWriter& ProtocolIStreamedMaterialWriter::operator=(
    ProtocolIStreamedMaterialWriter&&) noexcept = default;

void ProtocolIStreamedMaterialWriter::append(
    const ProtocolIMaterialSlotAddress& address,
    const std::vector<std::uint8_t>& bytes) {
  require(impl_ && !impl_->finalized && !bytes.empty() &&
              bytes.size() <= kSlotCap,
          "slot store append state/size");
  require(impl_->addresses.insert(address).second,
          "duplicate streamed material slot address");
  if (impl_->segment_fd < 0 ||
      impl_->segment_size + bytes.size() > impl_->segment_limit) {
    impl_->close_segment();
    impl_->open_segment();
  }
  Entry entry;
  entry.address = address;
  entry.segment = impl_->segment - 1U;
  entry.offset = impl_->segment_size;
  entry.length = bytes.size();
  entry.digest = sha256(bytes.data(), bytes.size());
  write_all(impl_->segment_fd, bytes.data(), bytes.size());
  impl_->segment_size += bytes.size();
  impl_->entries.push_back(entry);
}

void ProtocolIStreamedMaterialWriter::finalize_staging() {
  require(impl_ && !impl_->finalized && !impl_->entries.empty(),
          "slot store finalize state/empty pool");
  impl_->close_segment();
  const auto manifest = encode_manifest(impl_->binding, impl_->entries);
  write_new_file(impl_->dirfd, impl_->base + ".manifest", manifest.data(),
                 manifest.size(), 0400);
  impl_->finalized = true;
}

void protocol_i_streamed_material_publish_ready(
    const std::string& directory, const ProtocolIMaterialSlotBinding& binding) {
  const auto base = stem(binding);
  const int dirfd = open_directory(directory);
  try {
    const auto manifest = read_file_at(dirfd, base + ".manifest", kManifestCap);
    std::size_t pos = 0;
    validate_binding(manifest, binding, pos);
    const auto count = get_u64(manifest, pos);
    require(count == binding.expected_slot_count &&
                count <= (manifest.size() - pos) / kManifestEntryBytes &&
                manifest.size() == pos + count * kManifestEntryBytes,
            "slot manifest record count");
    std::map<std::uint32_t, std::uint64_t> segment_ends;
    std::set<ProtocolIMaterialSlotAddress> addresses;
    auto records_pos = pos;
    for (std::uint64_t index = 0; index < count; ++index) {
      ProtocolIMaterialSlotAddress address;
      address.purpose = get_u32(manifest, records_pos);
      address.iteration = get_u32(manifest, records_pos);
      address.substage = get_u32(manifest, records_pos);
      address.direction = get_u32(manifest, records_pos);
      address.left = get_u32(manifest, records_pos);
      address.right = get_u32(manifest, records_pos);
      const auto segment = get_u32(manifest, records_pos);
      const auto offset = get_u64(manifest, records_pos);
      const auto length = get_u64(manifest, records_pos);
      std::array<std::uint8_t, 32> slot_digest{};
      get_bytes(manifest, records_pos, slot_digest.data(), slot_digest.size());
      require(addresses.insert(address).second && length != 0 &&
                  length <= kSlotCap && offset <= kManifestCap &&
                  length <= kManifestCap - offset,
              "slot manifest duplicate/address range");
      segment_ends[segment] = std::max(segment_ends[segment], offset + length);
    }
    require(records_pos == manifest.size() && !segment_ends.empty(),
            "slot manifest record framing");
    std::uint32_t expected_segment = 0;
    for (const auto& segment : segment_ends) {
      require(segment.first == expected_segment++ && segment.second <= kManifestCap,
              "slot segment sequence/cap");
      const auto name = segment_name(base, segment.first);
      const int fd = ::openat(dirfd, name.c_str(),
          O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
      if (fd < 0) throw std::runtime_error("slot segment missing before publish");
      struct stat status{};
      const bool valid = ::fstat(fd, &status) == 0 && S_ISREG(status.st_mode) &&
          status.st_uid == ::geteuid() && status.st_size >= 0 &&
          (status.st_mode & 0222) == 0 &&
          static_cast<std::uint64_t>(status.st_size) >= segment.second &&
          static_cast<std::uint64_t>(status.st_size) <= kManifestCap;
      ::close(fd);
      require(valid, "slot segment shape before publish");
    }
    const auto digest = sha256(manifest.data(), manifest.size());
    std::vector<std::uint8_t> ready{'M','6','R','2'};
    put_u32(ready, material_schema_version(binding));
    put_u64(ready, binding.session); put_u64(ready, binding.fingerprint);
    put_u64(ready, binding.material_id);
    ready.push_back(binding.party);
    ready.push_back(static_cast<std::uint8_t>(binding.dcf_abi));
    ready.push_back(static_cast<std::uint8_t>(
        binding.score_share_abi == ProtocolIScoreShareAbi::S15InducedV1
            ? 0U : 2U));
    ready.insert(ready.end(), 5, 0);
    put_u64(ready, manifest.size());
    append_bytes(ready, digest.bytes.data(), digest.bytes.size());
    require(ready.size() == kReadyBytes, "slot ready encoding");
    write_new_file(dirfd, base + ".ready", ready.data(), ready.size(), 0400);
    ::close(dirfd);
  } catch (...) {
    ::close(dirfd);
    throw;
  }
}

std::vector<std::uint8_t> protocol_i_streamed_material_claim_slot(
    const std::string& directory, const ProtocolIMaterialSlotBinding& binding,
    const ProtocolIMaterialSlotAddress& address) {
  const auto base = stem(binding);
  const int dirfd = open_directory(directory);
  try {
    const auto ready = read_file_at(dirfd, base + ".ready", kReadyBytes);
    require(ready.size() == kReadyBytes && ready[0] == 'M' &&
                ready[1] == '6' && ready[2] == 'R' && ready[3] == '2',
            "streamed material ready marker");
    std::size_t ready_pos = 4;
    require(get_u32(ready, ready_pos) == material_schema_version(binding) &&
                get_u64(ready, ready_pos) == binding.session &&
                get_u64(ready, ready_pos) == binding.fingerprint &&
                get_u64(ready, ready_pos) == binding.material_id &&
                ready[ready_pos++] == binding.party,
            "streamed ready binding");
    require(ready[ready_pos++] == static_cast<std::uint8_t>(binding.dcf_abi),
            "streamed ready DCF ABI");
    require(ready[ready_pos++] == static_cast<std::uint8_t>(
                binding.score_share_abi ==
                        ProtocolIScoreShareAbi::S15InducedV1
                    ? 0U : 2U),
            "streamed ready score-share ABI");
    for (int i = 0; i < 5; ++i)
      require(ready[ready_pos++] == 0, "streamed ready reserved bytes");
    const auto manifest_size = get_u64(ready, ready_pos);
    Digest manifest_digest;
    get_bytes(ready, ready_pos, manifest_digest.bytes.data(),
              manifest_digest.bytes.size());
    const auto manifest = read_file_at(dirfd, base + ".manifest", kManifestCap);
    require(manifest.size() == manifest_size &&
                sha256(manifest.data(), manifest.size()).bytes ==
                    manifest_digest.bytes,
            "streamed manifest digest");
    std::size_t pos = 0;
    validate_binding(manifest, binding, pos);
    const auto count = get_u64(manifest, pos);
    require(count == binding.expected_slot_count &&
                count <= (manifest.size() - pos) / kManifestEntryBytes &&
                manifest.size() == pos + count * kManifestEntryBytes,
            "streamed manifest count");
    bool found = false;
    Entry selected;
    for (std::uint64_t i = 0; i < count; ++i) {
      Entry current;
      current.address.purpose = get_u32(manifest, pos);
      current.address.iteration = get_u32(manifest, pos);
      current.address.substage = get_u32(manifest, pos);
      current.address.direction = get_u32(manifest, pos);
      current.address.left = get_u32(manifest, pos);
      current.address.right = get_u32(manifest, pos);
      current.segment = get_u32(manifest, pos);
      current.offset = get_u64(manifest, pos);
      current.length = get_u64(manifest, pos);
      get_bytes(manifest, pos, current.digest.bytes.data(),
                current.digest.bytes.size());
      if (current.address == address) {
        require(!found, "duplicate address in streamed manifest");
        found = true;
        selected = current;
      }
    }
    require(found && selected.length != 0 && selected.length <= kSlotCap,
            "streamed material slot absent/invalid");
    const auto seg_name = segment_name(base, selected.segment);
    const int segfd = ::openat(dirfd, seg_name.c_str(),
        O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (segfd < 0) throw std::runtime_error("streamed material segment missing");
    try {
      struct stat st{};
      require(::fstat(segfd, &st) == 0 && S_ISREG(st.st_mode) &&
                  st.st_uid == ::geteuid() && st.st_size >= 0 &&
                  selected.offset <= static_cast<std::uint64_t>(st.st_size) &&
                  selected.length <= static_cast<std::uint64_t>(st.st_size) - selected.offset,
              "streamed material segment bounds");
      const auto before = sha256_fd_range(segfd, selected.offset, selected.length);
      require(before.bytes == selected.digest.bytes,
              "streamed material slot digest");
      std::vector<std::uint8_t> lease{'M','6','L','2'};
      put_u32(lease, 1);
      put_u64(lease, binding.session); put_u64(lease, binding.fingerprint);
      put_u64(lease, binding.material_id); lease.push_back(binding.party);
      lease.insert(lease.end(), 3, 0);
      put_u32(lease, address.purpose); put_u32(lease, address.iteration);
      put_u32(lease, address.substage); put_u32(lease, address.direction);
      put_u32(lease, address.left); put_u32(lease, address.right);
      append_bytes(lease, selected.digest.bytes.data(), selected.digest.bytes.size());
      write_new_file(dirfd, base + address_suffix(address) + ".lease",
                     lease.data(), lease.size(), 0400);
      std::vector<std::uint8_t> bytes(static_cast<std::size_t>(selected.length));
      pread_all(segfd, bytes.data(), bytes.size(), selected.offset);
      const auto after = sha256(bytes.data(), bytes.size());
      require(after.bytes == selected.digest.bytes,
              "streamed material changed after durable lease");
      ::close(segfd);
      ::close(dirfd);
      return bytes;
    } catch (...) {
      ::close(segfd);
      throw;
    }
  } catch (...) {
    ::close(dirfd);
    throw;
  }
}

}  // namespace moe_topk
