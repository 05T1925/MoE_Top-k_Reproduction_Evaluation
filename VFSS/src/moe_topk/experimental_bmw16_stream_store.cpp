#include <moe_topk/experimental_bmw16_stream_store.h>

#include <openssl/evp.h>
#include <openssl/crypto.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

#include <fcntl.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <unistd.h>

namespace moe_topk {
namespace {

constexpr std::size_t kPrefixBytes = 84;
constexpr std::size_t kWrappedKeyBytes = 48;
constexpr std::size_t kHeaderBytes = kPrefixBytes + kWrappedKeyBytes;
constexpr std::size_t kNonceBytes = 12;
constexpr std::size_t kTagBytes = 16;
constexpr std::array<std::uint8_t, 8> kMagic{'B','M','W','1','6','S','T','R'};
using Key = std::array<std::uint8_t, 32>;
using Nonce = std::array<std::uint8_t, kNonceBytes>;
using Ctx = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

struct KeyCleanseGuard {
  explicit KeyCleanseGuard(Key& value) : key(value) {}
  ~KeyCleanseGuard() { OPENSSL_cleanse(key.data(), key.size()); }
  Key& key;
};

struct BufferCleanseGuard {
  explicit BufferCleanseGuard(std::vector<std::uint8_t>& value) : bytes(value) {}
  ~BufferCleanseGuard() {
    if (!bytes.empty()) OPENSSL_cleanse(bytes.data(), bytes.size());
  }
  std::vector<std::uint8_t>& bytes;
};

void require(bool ok, const char* what) {
  if (!ok) throw std::runtime_error(what);
}

class Encoder {
 public:
  std::vector<std::uint8_t> b;
  void u8(std::uint8_t x) { b.push_back(x); }
  void u32(std::uint32_t x) { for (int s = 24; s >= 0; s -= 8) b.push_back(static_cast<std::uint8_t>(x >> s)); }
  void u64(std::uint64_t x) { for (int s = 56; s >= 0; s -= 8) b.push_back(static_cast<std::uint8_t>(x >> s)); }
  void raw(const std::uint8_t* p, std::size_t n) { b.insert(b.end(), p, p + n); }
};

class Decoder {
 public:
  explicit Decoder(const std::uint8_t* p) : p_(p) {}
  std::uint8_t u8() { return *p_++; }
  std::uint32_t u32() { std::uint32_t x = 0; for (int i=0;i<4;++i) x=(x<<8)|*p_++; return x; }
  std::uint64_t u64() { std::uint64_t x = 0; for (int i=0;i<8;++i) x=(x<<8)|*p_++; return x; }
 private:
  const std::uint8_t* p_;
};

void random_bytes(std::uint8_t* p, std::size_t n) {
  while (n) {
    const auto got = ::getrandom(p, n, 0);
    if (got < 0 && errno == EINTR) continue;
    require(got > 0, "BMW16 stream OS entropy failure");
    p += got;
    n -= static_cast<std::size_t>(got);
  }
}

std::vector<std::uint8_t> gcm_encrypt(const Key& key, const Nonce& nonce,
    const std::vector<std::uint8_t>& aad, const std::vector<std::uint8_t>& plain) {
  require(plain.size() <= static_cast<std::size_t>(std::numeric_limits<int>::max()), "BMW16 stream AEAD size");
  Ctx ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
  require(bool(ctx), "BMW16 stream AEAD context");
  require(EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1 &&
          EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(nonce.size()), nullptr) == 1 &&
          EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr, key.data(), nonce.data()) == 1,
          "BMW16 stream AEAD init");
  int n = 0;
  require(EVP_EncryptUpdate(ctx.get(), nullptr, &n, aad.data(), static_cast<int>(aad.size())) == 1,
          "BMW16 stream AEAD AAD");
  std::vector<std::uint8_t> out(plain.size() + kTagBytes);
  int written = 0, tail = 0;
  require(EVP_EncryptUpdate(ctx.get(), out.data(), &written, plain.data(), static_cast<int>(plain.size())) == 1 &&
          static_cast<std::size_t>(written) == plain.size() &&
          EVP_EncryptFinal_ex(ctx.get(), out.data() + written, &tail) == 1 && tail == 0 &&
          EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, static_cast<int>(kTagBytes), out.data() + plain.size()) == 1,
          "BMW16 stream AEAD encrypt");
  return out;
}

std::vector<std::uint8_t> gcm_decrypt(const Key& key, const Nonce& nonce,
    const std::vector<std::uint8_t>& aad, const std::uint8_t* encrypted, std::size_t size) {
  if (size < kTagBytes || size - kTagBytes > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    throw std::runtime_error("BMW16 stream ciphertext length");
  const auto plain_size = size - kTagBytes;
  Ctx ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
  require(bool(ctx), "BMW16 stream AEAD context");
  require(EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1 &&
          EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(nonce.size()), nullptr) == 1 &&
          EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr, key.data(), nonce.data()) == 1,
          "BMW16 stream AEAD init");
  int n = 0;
  require(EVP_DecryptUpdate(ctx.get(), nullptr, &n, aad.data(), static_cast<int>(aad.size())) == 1,
          "BMW16 stream AEAD AAD");
  std::vector<std::uint8_t> plain(plain_size);
  int written = 0, tail = 0;
  require(EVP_DecryptUpdate(ctx.get(), plain.data(), &written, encrypted, static_cast<int>(plain_size)) == 1 &&
          static_cast<std::size_t>(written) == plain_size,
          "BMW16 stream AEAD decrypt");
  require(EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, static_cast<int>(kTagBytes),
                              const_cast<std::uint8_t*>(encrypted + plain_size)) == 1 &&
          EVP_DecryptFinal_ex(ctx.get(), plain.data() + written, &tail) == 1 && tail == 0,
          "BMW16 stream authentication failed");
  return plain;
}

std::uint64_t pair_count(std::uint32_t n) {
  return static_cast<std::uint64_t>(n) * (n - 1U) / 2U;
}

std::uint64_t pair_index(std::uint32_t n, std::uint32_t left, std::uint32_t right) {
  if (left >= right || right >= n) throw std::invalid_argument("BMW16 stream canonical endpoints");
  return static_cast<std::uint64_t>(left) * n -
      static_cast<std::uint64_t>(left) * (left + 1U) / 2U + (right - left - 1U);
}

std::uint64_t base_slot(const ProtocolIBmw16ExperimentalPartyConfig& c) {
  return static_cast<std::uint64_t>(2U) * c.padded_n + 2U;
}

std::uint64_t expected_slot_id(const ProtocolIBmw16ExperimentalPartyConfig& c,
                               const ProtocolIBmw16MaterialSlot& slot) {
  const auto p = pair_count(c.n);
  const auto ix = pair_index(c.n, slot.left, slot.right);
  if (slot.stage == ProtocolIBmw16MaterialStage::Select && slot.task < 2 && slot.round >= 1 && slot.round <= 4)
    return base_slot(c) + (static_cast<std::uint64_t>(slot.task) * 4U + slot.round - 1U) * p + ix;
  if (slot.stage == ProtocolIBmw16MaterialStage::Membership && slot.task == 0xff && slot.round == 1)
    return base_slot(c) + 8U * p + ix;
  throw std::invalid_argument("BMW16 stream slot tuple");
}

std::vector<std::uint8_t> prefix_for(const ProtocolIBmw16ExperimentalPartyConfig& c,
    std::uint8_t party, std::uint32_t key_bytes, std::uint64_t slots, const Nonce& nonce) {
  Encoder e;
  e.raw(kMagic.data(), kMagic.size()); e.u32(1);
  e.u64(c.session); e.u64(c.fingerprint); e.u32(c.n); e.u32(c.k); e.u32(c.padded_n);
  e.u8(c.index_bits); e.u8(c.comparison_bits); e.u8(party); e.u8(0);
  e.u64(c.claim_root_device); e.u64(c.claim_root_inode); e.u32(key_bytes); e.u64(slots);
  e.raw(nonce.data(), nonce.size());
  require(e.b.size() == kPrefixBytes, "BMW16 stream header layout");
  return std::move(e.b);
}

std::vector<std::uint8_t> slot_aad(const std::vector<std::uint8_t>& prefix,
    const ProtocolIBmw16MaterialSlot& slot) {
  Encoder e; e.raw(prefix.data(), prefix.size());
  e.u64(slot.id); e.u8(static_cast<std::uint8_t>(slot.stage)); e.u8(slot.task);
  e.u8(slot.round); e.u8(0); e.u32(slot.left); e.u32(slot.right);
  return std::move(e.b);
}

Nonce slot_nonce(const Nonce& base, std::uint64_t id) {
  Nonce out = base;
  for (unsigned i = 0; i < 8; ++i) out[11U - i] ^= static_cast<std::uint8_t>(id >> (8U * i));
  return out;
}

void write_all(int fd, const std::uint8_t* p, std::size_t n) {
  while (n) {
    const auto wrote = ::write(fd, p, n);
    if (wrote < 0 && errno == EINTR) continue;
    require(wrote > 0, "BMW16 stream write");
    p += wrote; n -= static_cast<std::size_t>(wrote);
  }
}

void pread_all(int fd, std::uint8_t* p, std::size_t n, std::uint64_t offset) {
  while (n) {
    if (offset > static_cast<std::uint64_t>(std::numeric_limits<off_t>::max()))
      throw std::overflow_error("BMW16 stream file offset");
    const auto got = ::pread(fd, p, n, static_cast<off_t>(offset));
    if (got < 0 && errno == EINTR) continue;
    require(got > 0, "BMW16 stream truncated record");
    p += got; n -= static_cast<std::size_t>(got); offset += static_cast<std::uint64_t>(got);
  }
}

void validate_config(const ProtocolIBmw16ExperimentalPartyConfig& c, std::uint8_t party) {
  if (party > 1 || c.party != party || c.session == 0 || c.fingerprint == 0 || c.n < 2 || c.n > 256 ||
      c.k == 0 || c.k > c.n || c.claim_root_device == 0 || c.claim_root_inode == 0)
    throw std::invalid_argument("BMW16 stream configuration");
  std::uint32_t padded = 2; while (padded < c.n) padded <<= 1U;
  std::uint8_t ib = 1; for (auto v = padded; v > 2; v >>= 1U) ++ib;
  if (c.padded_n != padded || c.index_bits != ib || c.comparison_bits != 33U + ib)
    throw std::invalid_argument("BMW16 stream dimensions");
}

std::uint64_t expected_slot_count(const ProtocolIBmw16ExperimentalPartyConfig& c) {
  const auto p = pair_count(c.n);
  if (p > std::numeric_limits<std::uint64_t>::max() / 9U) throw std::overflow_error("BMW16 stream slots");
  return p * 9U;
}

}  // namespace

void protocol_i_bmw16_stream_store_seal_party(const std::string& path,
    const ProtocolIBmw16ExperimentalPartyConfig& config, std::uint8_t party,
    const ProtocolIBmw16ExperimentalPartyMaterial& material,
    const std::array<std::uint8_t, 32>& recipient_key,
    ProtocolIBmw16StreamStoreStats* stats) {
  validate_config(config, party);
  const auto pairs = pair_count(config.n), total = expected_slot_count(config);
  if (material.select_keys[0][0].size() != pairs || material.membership_keys.size() != pairs)
    throw std::invalid_argument("BMW16 stream source coverage");
  auto first = material.select_keys[0][0].front().serialize();
  if (first.empty() || first.size() > std::numeric_limits<std::uint32_t>::max())
    throw std::invalid_argument("BMW16 stream key size");
  const auto key_bytes = static_cast<std::uint32_t>(first.size());
  for (const auto& task : material.select_keys) for (const auto& round : task) {
    if (round.size() != pairs) throw std::invalid_argument("BMW16 stream Select coverage");
    for (const auto& key : round)
      if (key.party_id() != party || key.comparison_bits() != config.comparison_bits || key.serialize().size() != key_bytes)
        throw std::invalid_argument("BMW16 stream Select key binding");
  }
  for (const auto& key : material.membership_keys)
    if (key.party_id() != party || key.comparison_bits() != config.comparison_bits || key.serialize().size() != key_bytes)
      throw std::invalid_argument("BMW16 stream membership key binding");

  Key data_key{};
  KeyCleanseGuard data_key_guard(data_key);
  Nonce nonce{};
  random_bytes(data_key.data(), data_key.size());
  random_bytes(nonce.data(), nonce.size());
  auto prefix = prefix_for(config, party, key_bytes, total, nonce);
  std::vector<std::uint8_t> data_key_plain(data_key.begin(), data_key.end());
  BufferCleanseGuard data_key_plain_guard(data_key_plain);
  auto wrapped = gcm_encrypt(recipient_key, nonce, prefix, data_key_plain);
  std::vector<std::uint8_t> header = prefix; header.insert(header.end(), wrapped.begin(), wrapped.end());
  const int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
  if (fd < 0) throw std::runtime_error("BMW16 stream create output");
  bool okay = false;
  try {
    write_all(fd, header.data(), header.size());
    std::uint64_t id = base_slot(config);
    auto write_key = [&](const ProtocolIBmw16MaterialSlot& slot, const ProtocolIUcmpPartyMaterial& key) {
      if (slot.id != id || expected_slot_id(config, slot) != id) throw std::logic_error("BMW16 stream slot order");
      const auto plain = key.serialize();
      auto record = gcm_encrypt(data_key, slot_nonce(nonce, id), slot_aad(prefix, slot), plain);
      if (record.size() != static_cast<std::size_t>(key_bytes) + kTagBytes) throw std::logic_error("BMW16 stream record size");
      write_all(fd, record.data(), record.size()); ++id;
    };
    for (std::uint8_t task = 0; task < 2; ++task) for (std::uint8_t round = 1; round <= 4; ++round) {
      const auto& keys = material.select_keys[task][round - 1U];
      for (std::uint32_t i = 0; i < config.n; ++i) for (std::uint32_t j = i + 1; j < config.n; ++j) {
        const auto pair = pair_index(config.n, i, j);
        ProtocolIBmw16MaterialSlot slot{id, ProtocolIBmw16MaterialStage::Select, task, round, i, j};
        if (keys[pair].party_id() != party) throw std::logic_error("BMW16 stream party key");
        write_key(slot, keys[pair]);
      }
    }
    for (std::uint32_t i = 0; i < config.n; ++i) for (std::uint32_t j = i + 1; j < config.n; ++j) {
      const auto pair = pair_index(config.n, i, j);
      ProtocolIBmw16MaterialSlot slot{id, ProtocolIBmw16MaterialStage::Membership, 0xff, 1, i, j};
      if (material.membership_keys[pair].party_id() != party) throw std::logic_error("BMW16 stream party key");
      write_key(slot, material.membership_keys[pair]);
    }
    if (id != base_slot(config) + total) throw std::logic_error("BMW16 stream count");
    require(::fsync(fd) == 0, "BMW16 stream fsync");
    okay = true;
  } catch (...) {
    ::close(fd); ::unlink(path.c_str()); throw;
  }
  require(::close(fd) == 0 && okay, "BMW16 stream close");
  const auto parent = std::filesystem::path(path).parent_path();
  const auto dir = parent.empty() ? std::string(".") : parent.string();
  const int dfd = ::open(dir.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  require(dfd >= 0 && ::fsync(dfd) == 0, "BMW16 stream directory fsync");
  ::close(dfd);
  if (stats) {
    stats->ucmp_slots = total;
    stats->plaintext_key_bytes = static_cast<std::uint64_t>(key_bytes) * total;
    stats->sealed_file_bytes = kHeaderBytes + static_cast<std::uint64_t>(key_bytes + kTagBytes) * total;
  }
}

std::shared_ptr<ProtocolIBmw16StreamedUcmpSlotStore> ProtocolIBmw16StreamedUcmpSlotStore::open_party(
    const std::string& path, const ProtocolIBmw16ExperimentalPartyConfig& expected,
    std::uint8_t party, const std::array<std::uint8_t, 32>& recipient_key) {
  validate_config(expected, party);
  auto store = std::shared_ptr<ProtocolIBmw16StreamedUcmpSlotStore>(new ProtocolIBmw16StreamedUcmpSlotStore());
  store->fd_ = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
  require(store->fd_ >= 0, "BMW16 stream open");
  struct stat st{};
  require(::fstat(store->fd_, &st) == 0 && S_ISREG(st.st_mode) && (st.st_mode & 0077) == 0 &&
          st.st_uid == ::geteuid() && st.st_size >= static_cast<off_t>(kHeaderBytes),
          "BMW16 stream file ownership/mode/length");
  std::vector<std::uint8_t> header(kHeaderBytes);
  pread_all(store->fd_, header.data(), header.size(), 0);
  Decoder d(header.data());
  for (const auto x : kMagic) require(d.u8() == x, "BMW16 stream magic");
  require(d.u32() == 1, "BMW16 stream version");
  require(d.u64() == expected.session && d.u64() == expected.fingerprint && d.u32() == expected.n &&
          d.u32() == expected.k && d.u32() == expected.padded_n && d.u8() == expected.index_bits &&
          d.u8() == expected.comparison_bits && d.u8() == party && d.u8() == 0 &&
          d.u64() == expected.claim_root_device && d.u64() == expected.claim_root_inode,
          "BMW16 stream bundle binding");
  store->key_bytes_ = d.u32();
  store->slot_count_ = d.u64();
  Nonce nonce{};
  for (auto& x : nonce) x = d.u8();
  const auto expected_slots = expected_slot_count(expected);
  require(store->slot_count_ == expected_slots && store->key_bytes_ == 57U + 24U * expected.comparison_bits,
          "BMW16 stream key/slot dimensions");
  const auto record_bytes64 = static_cast<std::uint64_t>(store->key_bytes_) + kTagBytes;
  require(record_bytes64 <= std::numeric_limits<std::uint32_t>::max(), "BMW16 stream record bound");
  store->record_bytes_ = static_cast<std::uint32_t>(record_bytes64);
  if (expected_slots > (std::numeric_limits<std::uint64_t>::max() - kHeaderBytes) / record_bytes64)
    throw std::overflow_error("BMW16 stream file size");
  const auto expected_size = kHeaderBytes + expected_slots * record_bytes64;
  require(st.st_size >= 0 && static_cast<std::uint64_t>(st.st_size) == expected_size,
          "BMW16 stream exact file size");
  std::vector<std::uint8_t> prefix(header.begin(), header.begin() + kPrefixBytes);
  std::vector<std::uint8_t> wrapped(header.begin() + kPrefixBytes, header.end());
  auto key = gcm_decrypt(recipient_key, nonce, prefix, wrapped.data(), wrapped.size());
  require(key.size() == store->data_key_.size(), "BMW16 stream wrapped data key size");
  std::copy(key.begin(), key.end(), store->data_key_.begin());
  OPENSSL_cleanse(key.data(), key.size());
  store->config_ = expected; store->party_ = party;
  store->file_bytes_ = expected_size;
  return store;
}

ProtocolIBmw16StreamedUcmpSlotStore::~ProtocolIBmw16StreamedUcmpSlotStore() {
  OPENSSL_cleanse(data_key_.data(), data_key_.size());
  if (fd_ >= 0) ::close(fd_);
}

ProtocolIUcmpPartyMaterial ProtocolIBmw16StreamedUcmpSlotStore::load_once(
    const ProtocolIBmw16MaterialSlot& slot) {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto id = expected_slot_id(config_, slot);
  if (id != slot.id || id < base_slot(config_) || id >= base_slot(config_) + slot_count_)
    throw std::runtime_error("BMW16 stream slot identity mismatch");
  if (!loaded_.insert(id).second) throw std::runtime_error("BMW16 stream duplicate slot read");
  const auto ordinal = id - base_slot(config_);
  const auto offset = kHeaderBytes + ordinal * record_bytes_;
  std::vector<std::uint8_t> encrypted(record_bytes_);
  pread_all(fd_, encrypted.data(), encrypted.size(), offset);
  Nonce base{};
  std::vector<std::uint8_t> header(kPrefixBytes);
  pread_all(fd_, header.data(), header.size(), 0);
  std::copy(header.end() - kNonceBytes, header.end(), base.begin());
  auto plain = gcm_decrypt(data_key_, slot_nonce(base, id), slot_aad(header, slot),
                           encrypted.data(), encrypted.size());
  auto key = ProtocolIUcmpPartyMaterial::deserialize(plain);
  OPENSSL_cleanse(plain.data(), plain.size());
  if (key.party_id() != party_ || key.comparison_bits() != config_.comparison_bits)
    throw std::runtime_error("BMW16 stream decoded key metadata");
  return key;
}

}  // namespace moe_topk
