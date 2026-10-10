#include <moe_topk/experimental_bmw16_material_bundle.h>
#include <moe_topk/experimental_bmw16_material_delivery.h>
#include <moe_topk/experimental_bmw16_online_tls.h>
#include <moe_topk/experimental_bmw16_select_party.h>
#if defined(MOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V1)
#include <moe_topk/protocol_i_bmw16_conditional_secure_v1.h>
#endif
#if defined(MOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V2)
#include <moe_topk/protocol_i_bmw16_conditional_secure_v2.h>
#endif
#include <moe_topk/experimental_bmw16_startup_gate.h>
#include <moe_topk/experimental_bmw16_stream_store.h>
#include <moe_topk/protocol_i_transport.h>
#include <FSS/dcf.h>

#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/random.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <thread>

namespace {
using namespace moe_topk;

constexpr int kSuccess = 0;
constexpr int kAlgorithmAbort = 10;
constexpr int kMaterialAbort = 20;
constexpr int kCommunicationAbort = 30;
constexpr int kUnexpected = 70;
constexpr std::size_t kChannelCount = kProtocolIBmw16OnlineChannelCount;

void require(bool value, const char* reason) {
  if (!value) throw std::runtime_error(reason);
}

bool test_failpoint_is(const std::string& point) {
#if defined(MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS)
  const char* configured = std::getenv("MOE_BMW16_TEST_FAILPOINT");
  return configured && point == configured;
#else
  (void)point;
  return false;
#endif
}

void random_bytes(std::uint8_t* out, std::size_t left) {
  while (left) {
    const auto n = ::getrandom(out, left, 0);
    if (n < 0 && errno == EINTR) continue;
    require(n > 0, "offline T OS entropy failure");
    out += n; left -= static_cast<std::size_t>(n);
  }
}

std::array<std::uint8_t, 32> read_key(const std::string& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) throw std::runtime_error("recipient credential unavailable");
  std::array<std::uint8_t, 32> key{};
  input.read(reinterpret_cast<char*>(key.data()), key.size());
  if (input.gcount() != static_cast<std::streamsize>(key.size()) || input.peek() != EOF)
    throw std::runtime_error("recipient credential length");
  return key;
}

std::uint64_t peak_rss_kb() {
  struct rusage usage{};
  if (::getrusage(RUSAGE_SELF, &usage) != 0) return 0;
  return static_cast<std::uint64_t>(usage.ru_maxrss);
}

std::uint64_t elapsed_us(std::chrono::steady_clock::time_point start) {
  return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now() - start).count());
}

void require_party_owned_private_file(const std::string& path, bool directory) {
  struct stat st{};
  if (::lstat(path.c_str(), &st) != 0 ||
      (directory ? !S_ISDIR(st.st_mode) : !S_ISREG(st.st_mode)) ||
      st.st_uid != ::geteuid() || (st.st_mode & 0077) != 0)
    throw std::runtime_error("party file ownership/permission contract");
}

void chown_bundle_for_party(const std::string& path, uid_t owner,
                            const std::string& test_failpoint) {
  if (test_failpoint_is(test_failpoint))
    throw std::runtime_error("TEST_ONLY injected chown EPERM at " + test_failpoint);
  if (::chown(path.c_str(), owner, owner) != 0 || ::chmod(path.c_str(), 0400) != 0)
    throw std::runtime_error("T cannot deliver sealed bundle to configured party owner");
  const auto parent = std::filesystem::path(path).parent_path();
  const int dfd = ::open(parent.empty() ? "." : parent.c_str(),
                         O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  if (dfd < 0) throw std::runtime_error("T bundle delivery directory open");
  const bool synced = ::fsync(dfd) == 0;
  ::close(dfd);
  if (!synced) throw std::runtime_error("T bundle delivery directory sync");
}

void make_ready_marker_public_read_only(const std::string& path) {
  if (::chmod(path.c_str(), 0444) != 0)
    throw std::runtime_error("T cannot set paired-ready marker permissions");
  const auto parent = std::filesystem::path(path).parent_path();
  const int dfd = ::open(parent.empty() ? "." : parent.c_str(),
                         O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  if (dfd < 0) throw std::runtime_error("T ready-marker directory open");
  const bool injected = test_failpoint_is("ready_dir_fsync");
  const bool synced = !injected && ::fsync(dfd) == 0;
  ::close(dfd);
  if (!synced) throw std::runtime_error("T ready-marker directory sync");
}

void t_failpoint(const std::string& point) {
#if defined(MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS)
  const char* configured = std::getenv("MOE_BMW16_TEST_FAILPOINT");
  if (!configured) return;
  const std::string value(configured);
  if (value == "crash_" + point) ::_exit(71);
  if (value == point) throw std::runtime_error("TEST_ONLY injected T failure: " + point);
#else
  (void)point;
#endif
}

void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
  for (unsigned i = 0; i < 4; ++i) out.push_back(static_cast<std::uint8_t>(value >> (8U * i)));
}
void append_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
  for (unsigned i = 0; i < 8; ++i) out.push_back(static_cast<std::uint8_t>(value >> (8U * i)));
}

std::vector<std::uint8_t> ready_marker_bytes(
    std::uint64_t session, std::uint64_t fingerprint, std::uint32_t n,
    std::uint32_t k, const std::array<std::uint8_t, 32>& stream_id,
    const std::array<std::uint8_t, 32>& manifest0,
    const std::array<std::uint8_t, 32>& manifest1) {
  std::vector<std::uint8_t> bytes{'B','M','W','1','6','R','D','Y'};
  append_u32(bytes, 1); append_u64(bytes, session); append_u64(bytes, fingerprint);
  append_u32(bytes, n); append_u32(bytes, k);
  bytes.insert(bytes.end(), stream_id.begin(), stream_id.end());
  bytes.insert(bytes.end(), manifest0.begin(), manifest0.end());
  bytes.insert(bytes.end(), manifest1.begin(), manifest1.end());
  return bytes;
}

struct ReadyBarrierStats { std::uint64_t sent = 0, received = 0, elapsed_us = 0; };

ReadyBarrierStats exchange_ready(int fd, const ProtocolIBmw16ExperimentalPartyConfig& c) {
  if (fd < 0) throw ProtocolITransportError("ready barrier descriptor");
  const int owned_fd = ::dup(fd);
  if (owned_fd < 0) throw ProtocolITransportError("ready barrier descriptor duplicate");
  ::close(fd);
  ProtocolIFrameConfig frame{c.session, c.fingerprint, c.n, c.k, c.comparison_bits,
                             c.party, static_cast<std::uint8_t>(1U - c.party), 1, 1};
  ProtocolIFramedChannel channel(owned_fd, frame, c.timeout_ms);
  const std::vector<std::uint8_t> ready{'B','M','W','1','6','R','D','Y',1,c.party};
  const auto start = std::chrono::steady_clock::now();
  std::exception_ptr send_error;
  std::thread sender([&] { try { channel.send(ready); } catch (...) { send_error = std::current_exception(); } });
  std::vector<std::uint8_t> peer;
  std::exception_ptr receive_error;
  try { peer = channel.receive(); }
  catch (...) { receive_error = std::current_exception(); (void)::shutdown(owned_fd, SHUT_RDWR); }
  sender.join();
  if (send_error) std::rethrow_exception(send_error);
  if (receive_error) std::rethrow_exception(receive_error);
  if (peer.size() != ready.size() || !std::equal(peer.begin(), peer.end() - 1, ready.begin()) ||
      peer.back() != static_cast<std::uint8_t>(1U - c.party))
    throw ProtocolITransportError("ready barrier peer identity payload");
  return {channel.sent_bytes(), channel.received_bytes(),
      static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::steady_clock::now() - start).count())};
}

std::vector<std::uint8_t> read_bytes(const std::string& path,
                                     std::size_t limit = 512U * 1024U * 1024U) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input) throw std::runtime_error("party input file unavailable");
  const auto end = input.tellg();
  if (end < 0 || static_cast<std::uint64_t>(end) > limit)
    throw std::runtime_error("party input file size");
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));
  input.seekg(0);
  if (!bytes.empty()) input.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
  if (!input || static_cast<std::size_t>(input.gcount()) != bytes.size())
    throw std::runtime_error("party input file truncated");
  return bytes;
}

void verify_ready_marker(const std::string& path,
    const ProtocolIBmw16ExperimentalPartyConfig& c, std::uint8_t party,
    const ProtocolIBmw16StreamBundleOpenResult& shell) {
  const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
  if (fd < 0) throw std::runtime_error("BMW16 paired completion marker unavailable");
  struct stat st{};
  if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size != 132) {
    ::close(fd);
    throw std::runtime_error("BMW16 paired completion marker ownership/shape");
  }
  const bool root_published = st.st_uid == 0 && (st.st_mode & 0777) == 0444;
  const bool party_published = st.st_uid == ::geteuid() && (st.st_mode & 0777) == 0400;
  if (!root_published && !party_published) {
    ::close(fd);
    throw std::runtime_error("BMW16 paired completion marker ownership/shape");
  }
  std::vector<std::uint8_t> bytes(132);
  std::size_t offset = 0;
  while (offset < bytes.size()) {
    const auto got = ::read(fd, bytes.data() + offset, bytes.size() - offset);
    if (got < 0 && errno == EINTR) continue;
    if (got <= 0) { ::close(fd); throw std::runtime_error("BMW16 paired completion marker truncated"); }
    offset += static_cast<std::size_t>(got);
  }
  std::uint8_t extra = 0;
  const auto extra_count = ::read(fd, &extra, 1);
  ::close(fd);
  if (extra_count != 0) throw std::runtime_error("BMW16 paired completion marker trailing bytes");
  const auto expected = ready_marker_bytes(c.session, c.fingerprint, c.n, c.k,
      shell.stream.stream_id,
      party == 0 ? shell.stats.manifest_sha256 : std::array<std::uint8_t, 32>{},
      party == 1 ? shell.stats.manifest_sha256 : std::array<std::uint8_t, 32>{});
  if (expected.size() != 132 ||
      !std::equal(bytes.begin(), bytes.begin() + 68, expected.begin()) ||
      !std::equal(bytes.begin() + 68 + 32U * party,
                  bytes.begin() + 100 + 32U * party,
                  expected.begin() + 68 + 32U * party))
    throw std::runtime_error("BMW16 paired material completion marker mismatch");
}

void write_mask_exclusive(const std::string& path,
                          const std::vector<std::uint8_t>& bytes) {
  if (path.empty() || bytes.empty()) throw std::invalid_argument("mask output path/length");
  const auto temp = path + ".tmp." + std::to_string(::getpid());
  const std::filesystem::path output_path(path);
  const auto parent = output_path.has_parent_path() ? output_path.parent_path() : std::filesystem::path(".");
  const auto sync_parent = [&]() {
    const int dir_fd = ::open(parent.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (dir_fd < 0) return false;
    const bool synced = ::fsync(dir_fd) == 0;
    const bool closed = ::close(dir_fd) == 0;
    return synced && closed;
  };
  int fd = ::open(temp.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
  if (fd < 0) throw std::runtime_error("mask output create");
  bool published = false;
  bool temp_exists = true;
  try {
    std::size_t offset = 0;
    while (offset < bytes.size()) {
      const auto wrote = ::write(fd, bytes.data() + offset, bytes.size() - offset);
      if (wrote < 0 && errno == EINTR) continue;
      if (wrote <= 0) throw std::runtime_error("mask output write");
      offset += static_cast<std::size_t>(wrote);
    }
    if (::fdatasync(fd) != 0) throw std::runtime_error("mask output data sync");
    const int close_result = ::close(fd);
    fd = -1;
    if (close_result != 0) throw std::runtime_error("mask output close");
    if (::link(temp.c_str(), path.c_str()) != 0) throw std::runtime_error("mask output no-clobber publish");
    published = true;
#if defined(MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS)
    if (test_failpoint_is("mask_unlink_after_publish"))
      throw std::runtime_error("TEST_ONLY mask temp unlink failure after publish");
#endif
    if (::unlink(temp.c_str()) != 0) throw std::runtime_error("mask output temp unlink");
    temp_exists = false;
#if defined(MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS)
    if (test_failpoint_is("mask_dir_fsync_after_publish"))
      throw std::runtime_error("TEST_ONLY mask directory sync failure after publish");
#endif
    if (!sync_parent()) throw std::runtime_error("mask output directory sync");
    published = false;
  } catch (...) {
    if (fd >= 0) ::close(fd);
    if (published) ::unlink(path.c_str());
    if (temp_exists) ::unlink(temp.c_str());
    (void)sync_parent();
    throw;
  }
}

ProtocolIBmw16ExperimentalPartyConfig config_base(std::uint32_t n, std::uint32_t k,
    std::uint64_t session, std::uint64_t fingerprint, std::uint8_t party,
    int timeout_ms) {
  if (party > 1 || n == 0 || n > (1U << 20U) || k == 0 || k > n ||
      session == 0 || fingerprint == 0 || timeout_ms <= 0)
    throw std::invalid_argument("BMW16 EXPERIMENTAL public configuration");
  std::uint32_t padded = 2;
  while (padded < n) padded <<= 1U;
  std::uint8_t index_bits = 1;
  for (auto v = padded; v > 2; v >>= 1U) ++index_bits;
  ProtocolIBmw16ExperimentalPartyConfig c;
  c.session = session;
  c.fingerprint = fingerprint;
  c.n = n;
  c.k = k;
  c.padded_n = padded;
  c.index_bits = index_bits;
  c.comparison_bits = static_cast<std::uint8_t>(33U + index_bits);
  c.party = party;
  c.timeout_ms = timeout_ms;
  return c;
}

ProtocolIBmw16ExperimentalPartyConfig config(std::uint32_t n, std::uint32_t k,
    std::uint64_t session, std::uint64_t fingerprint, std::uint8_t party,
    int timeout_ms, const std::string& claim_root) {
  auto c = config_base(n,k,session,fingerprint,party,timeout_ms);
  if (n > 1) {
    const auto root = protocol_i_bmw16_claim_root_identity(claim_root);
    c.claim_root_device = root.device;
    c.claim_root_inode = root.inode;
  }
  return c;
}

std::vector<std::uint32_t> decode_raw_share(const std::vector<std::uint8_t>& bytes,
                                            std::uint32_t n) {
  if (bytes.size() != static_cast<std::size_t>(n) * 4U)
    throw std::runtime_error("raw score share shape");
  std::vector<std::uint32_t> values(n);
  for (std::size_t i = 0; i < n; ++i) {
    const auto p = 4U * i;
    values[i] = static_cast<std::uint32_t>(bytes[p]) |
        (static_cast<std::uint32_t>(bytes[p + 1]) << 8U) |
        (static_cast<std::uint32_t>(bytes[p + 2]) << 16U) |
        (static_cast<std::uint32_t>(bytes[p + 3]) << 24U);
  }
  return values;
}

int t_offline(int argc, char** argv) {
  const auto t_start = std::chrono::steady_clock::now();
  // T is a one-shot offline process. Root is required only to place each
  // sealed package into its pre-provisioned, party-owned local drop directory.
  // t n k session fingerprint claim0 claim1 shell0 shell1 slots0 slots1 key0 key1 uid0 uid1 pair-ready
  require(argc == 17, "usage: node t n k session fingerprint claim0 claim1 shell0 shell1 slots0 slots1 key0 key1 uid0 uid1 pair-ready");
  require(::geteuid() == 0, "offline T delivery requires the configured local privileged drop service");
  const auto n = static_cast<std::uint32_t>(std::stoul(argv[2]));
  const auto k = static_cast<std::uint32_t>(std::stoul(argv[3]));
  const auto session = std::stoull(argv[4]);
  const auto fingerprint = std::stoull(argv[5]);
  if (n == 1) {
    std::cout << "role=T status=SINGLETON_NO_MATERIAL T_online=false\n";
    return kSuccess;
  }
  auto c0 = config(n, k, session, fingerprint, 0, 30000, argv[6]);
  auto c1 = config(n, k, session, fingerprint, 1, 30000, argv[7]);
  std::array<std::uint8_t, 32> stream_id{}; random_bytes(stream_id.data(), stream_id.size());
  ProtocolIBmw16StreamedUcmpSlotWriter writer0(argv[10], c0, 0, read_key(argv[12]), stream_id);
  ProtocolIBmw16StreamedUcmpSlotWriter writer1(argv[11], c1, 1, read_key(argv[13]), stream_id);
  auto materials = protocol_i_bmw16_experimental_material_generate_streaming(c0,
      [&](const ProtocolIBmw16MaterialSlot& slot, const ProtocolIUcmpPartyMaterial& key0,
          const ProtocolIUcmpPartyMaterial& key1) {
        writer0.append(slot, key0);
        writer1.append(slot, key1);
      });
  ProtocolIBmw16StreamStoreStats stream0{}, stream1{};
  stream0 = writer0.finalize();
  t_failpoint("after_sidecar0");
  stream1 = writer1.finalize();
  t_failpoint("after_sidecar1");
  require(stream0.stream_id == stream1.stream_id && stream0.ucmp_slots == stream1.ucmp_slots &&
          stream0.sealed_file_bytes == stream1.sealed_file_bytes, "party stream sidecar mismatch");
  ProtocolIBmw16StreamBundleBinding binding0, binding1;
  binding0.stream_id = binding1.stream_id = stream_id;
  binding0.sidecar_sha256 = stream0.file_sha256;
  binding1.sidecar_sha256 = stream1.file_sha256;
  binding0.ucmp_slot_count = binding1.ucmp_slot_count = stream0.ucmp_slots;
  binding0.sealed_sidecar_bytes = stream0.sealed_file_bytes;
  binding1.sealed_sidecar_bytes = stream1.sealed_file_bytes;
  ProtocolIBmw16BundleStats b0{}, b1{};
  std::vector<ProtocolIBmw16PublishedFileIdentity> owned_shells;
  try {
    const auto e0 = protocol_i_bmw16_bundle_seal_stream_shell_party(c0, 0, materials.party0, binding0, read_key(argv[12]), &b0);
    const auto e1 = protocol_i_bmw16_bundle_seal_stream_shell_party(c1, 1, materials.party1, binding1, read_key(argv[13]), &b1);
    owned_shells.push_back(protocol_i_bmw16_bundle_write_atomic(argv[8], e0));
    t_failpoint("after_shell0");
    owned_shells.push_back(protocol_i_bmw16_bundle_write_atomic(argv[9], e1));
    t_failpoint("after_shell1");
    chown_bundle_for_party(argv[8], static_cast<uid_t>(std::stoul(argv[14])), "chown_shell0");
    chown_bundle_for_party(argv[9], static_cast<uid_t>(std::stoul(argv[15])), "chown_shell1");
    chown_bundle_for_party(argv[10], static_cast<uid_t>(std::stoul(argv[14])), "chown_sidecar0");
    chown_bundle_for_party(argv[11], static_cast<uid_t>(std::stoul(argv[15])), "chown_sidecar1");
    t_failpoint("before_ready");
    const auto marker = ready_marker_bytes(session, fingerprint, n, k, stream_id,
                                            b0.manifest_sha256, b1.manifest_sha256);
    owned_shells.push_back(protocol_i_bmw16_bundle_write_atomic(argv[16], marker));
    t_failpoint("after_ready");
    make_ready_marker_public_read_only(argv[16]);
    // An orchestrator is required to wait for this process to exit zero before
    // it starts either party. A crash here leaves a complete-looking marker,
    // but a nonzero T exit must prevent the launch transition.
    t_failpoint("after_ready_durable");
    writer0.commit_published();
    writer1.commit_published();
  } catch (...) {
    for (auto it = owned_shells.rbegin(); it != owned_shells.rend(); ++it)
      (void)protocol_i_bmw16_remove_published_if_owned(*it);
    throw;
  }
  std::cout << "role=T status=OFFLINE_MATERIAL_READY"
            << " scheme=Protocol_I_BMW16_DERIVED_SELECT_DCF"
            << " label=PROJECT_DERIVED/EXPERIMENTAL"
            << " session=" << session << " n=" << n << " K=" << k
            << " slots_per_party=" << b0.slot_count
            << " ucmp_stream_slots_per_party=" << stream0.ucmp_slots
            << " sidecar_bytes_p0=" << stream0.sealed_file_bytes
            << " sidecar_bytes_p1=" << stream1.sealed_file_bytes
            << " shell_bytes_p0=" << b0.envelope_bytes
            << " shell_bytes_p1=" << b1.envelope_bytes
            << " offline_elapsed_us=" << elapsed_us(t_start)
            << " peak_rss_kb=" << peak_rss_kb()
            << " shell_version=4 root_bound=true pair_ready=true T_online=false\n";
  return kSuccess;
}

std::array<std::uint8_t,32> sha256_bytes(const std::vector<std::uint8_t>& bytes) {
  std::array<std::uint8_t,32> out{};unsigned n=0;
  require(EVP_Digest(bytes.data(),bytes.size(),out.data(),&n,EVP_sha256(),nullptr)==1&&n==out.size(),
          "BMW16 TLS shell SHA-256");return out;
}

int t_tls_offline(int argc,char** argv) {
  const auto started=std::chrono::steady_clock::now();
  // t-tls n k session fingerprint root0_dev root0_ino root1_dev root1_ino
  // side0 side1 shell0 shell1 wrap0 wrap1 cert key ca host0 port0 id0 host1 port1 id1
  require(argc==25,"usage: node t-tls n k session fingerprint root0_dev root0_ino root1_dev root1_ino side0 side1 shell0 shell1 wrap0 wrap1 cert key ca host0 port0 id0 host1 port1 id1");
  const auto n=static_cast<std::uint32_t>(std::stoul(argv[2]));
  const auto k=static_cast<std::uint32_t>(std::stoul(argv[3]));
  const auto session=std::stoull(argv[4]),fingerprint=std::stoull(argv[5]);
  require(n>1,"BMW16 TLS material delivery is unnecessary for singleton");
  require_party_owned_private_file(argv[14],false);
  require_party_owned_private_file(argv[15],false);
  require_party_owned_private_file(argv[17],false);
  auto c0=config_base(n,k,session,fingerprint,0,120000);
  auto c1=config_base(n,k,session,fingerprint,1,120000);
  c0.claim_root_device=std::stoull(argv[6]);c0.claim_root_inode=std::stoull(argv[7]);
  c1.claim_root_device=std::stoull(argv[8]);c1.claim_root_inode=std::stoull(argv[9]);
  std::array<std::uint8_t,32> stream_id{};random_bytes(stream_id.data(),stream_id.size());
  resetDcfPrgCallCounts();
  ProtocolIBmw16StreamedUcmpSlotWriter writer0(argv[10],c0,0,read_key(argv[14]),stream_id);
  ProtocolIBmw16StreamedUcmpSlotWriter writer1(argv[11],c1,1,read_key(argv[15]),stream_id);
  auto materials=protocol_i_bmw16_experimental_material_generate_streaming(c0,
      [&](const ProtocolIBmw16MaterialSlot& slot,const ProtocolIUcmpPartyMaterial& key0,
          const ProtocolIUcmpPartyMaterial& key1){writer0.append(slot,key0);writer1.append(slot,key1);});
  const auto dcf_keygen_counts = getDcfPrgCallCounts();
  const auto stream0=writer0.finalize(),stream1=writer1.finalize();
  require(stream0.stream_id==stream1.stream_id&&stream0.ucmp_slots==stream1.ucmp_slots&&
          stream0.sealed_file_bytes==stream1.sealed_file_bytes,"BMW16 TLS sidecar pair mismatch");
  ProtocolIBmw16StreamBundleBinding binding0,binding1;
  binding0.stream_id=binding1.stream_id=stream_id;
  binding0.sidecar_sha256=stream0.file_sha256;binding1.sidecar_sha256=stream1.file_sha256;
  binding0.ucmp_slot_count=binding1.ucmp_slot_count=stream0.ucmp_slots;
  binding0.sealed_sidecar_bytes=stream0.sealed_file_bytes;binding1.sealed_sidecar_bytes=stream1.sealed_file_bytes;
  ProtocolIBmw16BundleStats b0{},b1{};
  const auto shell0=protocol_i_bmw16_bundle_seal_stream_shell_party(c0,0,materials.party0,binding0,read_key(argv[14]),&b0);
  const auto shell1=protocol_i_bmw16_bundle_seal_stream_shell_party(c1,1,materials.party1,binding1,read_key(argv[15]),&b1);
  std::vector<ProtocolIBmw16PublishedFileIdentity> shells;
  try {
    shells.push_back(protocol_i_bmw16_bundle_write_atomic(argv[12],shell0));
    shells.push_back(protocol_i_bmw16_bundle_write_atomic(argv[13],shell1));
    std::array<ProtocolIBmw16TlsDeliveryManifest,2> manifests{};
    manifests[0].config=c0;manifests[1].config=c1;
    for(auto& m:manifests){m.stream_id=stream_id;m.slot_count=stream0.ucmp_slots;}
    manifests[0].own_shell_manifest_sha256=b0.manifest_sha256;
    manifests[0].peer_shell_manifest_sha256=b1.manifest_sha256;
    manifests[1].own_shell_manifest_sha256=b1.manifest_sha256;
    manifests[1].peer_shell_manifest_sha256=b0.manifest_sha256;
    manifests[0].shell_bytes=shell0.size();manifests[1].shell_bytes=shell1.size();
    manifests[0].shell_sha256=sha256_bytes(shell0);manifests[1].shell_sha256=sha256_bytes(shell1);
    manifests[0].sidecar_bytes=stream0.sealed_file_bytes;manifests[1].sidecar_bytes=stream1.sealed_file_bytes;
    manifests[0].sidecar_sha256=stream0.file_sha256;manifests[1].sidecar_sha256=stream1.file_sha256;
    const auto marker_vec=ready_marker_bytes(session,fingerprint,n,k,stream_id,b0.manifest_sha256,b1.manifest_sha256);
    require(marker_vec.size()==132,"BMW16 TLS ready marker size");
    std::array<std::uint8_t,132> marker{};std::copy(marker_vec.begin(),marker_vec.end(),marker.begin());
    ProtocolIBmw16TlsCredentials dealer;dealer.certificate_pem=argv[16];dealer.private_key_pem=argv[17];
    dealer.trust_bundle_pem=argv[18];dealer.io_timeout_ms=120000;
#if defined(MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS)
    if(test_failpoint_is("receiver_silent"))dealer.io_timeout_ms=1000;
#endif
    std::array<ProtocolIBmw16TlsEndpoint,2> endpoints{{
      {argv[19],static_cast<std::uint16_t>(std::stoul(argv[20])),argv[21]},
      {argv[22],static_cast<std::uint16_t>(std::stoul(argv[23])),argv[24]}}};
    std::array<ProtocolIBmw16TlsDeliveryFiles,2> paths{{
      {argv[12],argv[10],""},{argv[13],argv[11],""}}};
    ProtocolIBmw16TlsDeliveryStats delivery{};
    protocol_i_bmw16_tls_deliver_pair(dealer,endpoints,manifests,paths,marker,&delivery);
    constexpr std::uint64_t stream_shell_fixed_wrapper_bytes = 44;
    require(b0.plaintext_bytes >= stream_shell_fixed_wrapper_bytes + b0.manifest_bytes &&
            b1.plaintext_bytes >= stream_shell_fixed_wrapper_bytes + b1.manifest_bytes,
            "BMW16 stream shell payload accounting underflow");
    const auto shell_material0 = b0.plaintext_bytes - stream_shell_fixed_wrapper_bytes - b0.manifest_bytes;
    const auto shell_material1 = b1.plaintext_bytes - stream_shell_fixed_wrapper_bytes - b1.manifest_bytes;
    std::cout<<"role=T status=OFFLINE_MATERIAL_DELIVERED scheme=Protocol_I_BMW16_DERIVED_SELECT_DCF"
             <<" label=PROJECT_DERIVED/EXPERIMENTAL session="<<session<<" n="<<n<<" K="<<k
             <<" slots_per_party="<<b0.slot_count<<" sidecar_bytes_per_party="<<stream0.sealed_file_bytes
             <<" shell_bytes_p0="<<shell0.size()<<" shell_bytes_p1="<<shell1.size()
             <<" material_payload_bytes_p0="<<(shell_material0+stream0.plaintext_key_bytes)
             <<" material_payload_bytes_p1="<<(shell_material1+stream1.plaintext_key_bytes)
             <<" shell_serialized_material_bytes_p0="<<shell_material0
             <<" shell_serialized_material_bytes_p1="<<shell_material1
             <<" dcf_plaintext_key_bytes_per_party="<<stream0.plaintext_key_bytes
             <<" dcf_keygen_calls="<<dcf_keygen_counts.keygen_calls
             <<" dcf_keygen_node_expansions="<<dcf_keygen_counts.keygen_node_expansions
             <<" dcf_keygen_counter_enabled="<<(dcf_keygen_counts.enabled?"true":"false")
             <<" shell_plaintext_bytes_p0="<<b0.plaintext_bytes
             <<" shell_plaintext_bytes_p1="<<b1.plaintext_bytes
             <<" tls_sent_bytes="<<delivery.bytes_sent<<" tls_received_bytes="<<delivery.bytes_received
             <<" delivery_elapsed_us="<<delivery.elapsed_us<<" offline_elapsed_us="<<elapsed_us(started)
             <<" T_online=false delivery=mutual_TLS_1.3\n";
  } catch(...) {
    for(auto it=shells.rbegin();it!=shells.rend();++it)(void)protocol_i_bmw16_remove_published_if_owned(*it);
    throw;
  }
  for(auto it=shells.rbegin();it!=shells.rend();++it)(void)protocol_i_bmw16_remove_published_if_owned(*it);
  return kSuccess;
}

int receive_tls(int argc,char** argv) {
  // recv-tls id n k session fingerprint claim-root shell sidecar pair-ready wrap cert key ca expected-T timeout fd
  require(argc==18,"usage: node recv-tls id n k session fingerprint claim-root shell sidecar pair-ready wrap cert key ca expected-T timeout listen-fd");
  const auto party=static_cast<std::uint8_t>(std::stoul(argv[2]));
  const auto n=static_cast<std::uint32_t>(std::stoul(argv[3]));
  const auto k=static_cast<std::uint32_t>(std::stoul(argv[4]));
  const auto session=std::stoull(argv[5]),fingerprint=std::stoull(argv[6]);
  const auto timeout=std::stoi(argv[16]);
  require_party_owned_private_file(argv[7],true);
  require_party_owned_private_file(argv[11],false);
  require_party_owned_private_file(argv[13],false);
  const auto c=config(n,k,session,fingerprint,party,timeout,argv[7]);
  ProtocolIBmw16TlsCredentials credentials;credentials.certificate_pem=argv[12];
  credentials.private_key_pem=argv[13];credentials.trust_bundle_pem=argv[14];
  credentials.peer_dns_identity=argv[15];credentials.io_timeout_ms=timeout;
  ProtocolIBmw16TlsDeliveryFiles destination{argv[8],argv[9],argv[10]};
  ProtocolIBmw16TlsDeliveryStats stats{};
  try {
    stats=protocol_i_bmw16_tls_receive_once(std::stoi(argv[17]),credentials,c,argv[7],destination,read_key(argv[11]));
  } catch(const ProtocolIBmw16MaterialDeliveryError&) {
    throw;
  } catch(const std::exception& error) {
    throw ProtocolIBmw16MaterialDeliveryError(ProtocolIBmw16MaterialDeliveryError::Kind::Material,error.what());
  }
  std::cout<<"role=P"<<static_cast<unsigned>(party)<<" status=OFFLINE_MATERIAL_COMMITTED"
           <<" bytes_received="<<stats.bytes_received<<" bytes_sent="<<stats.bytes_sent
           <<" delivery_elapsed_us="<<stats.elapsed_us<<" T_online=false\n";
  return kSuccess;
}

int party_online(int argc, char** argv, bool online_tls = false,
                 bool conditional_secure_v1 = false,
                 bool conditional_secure_v2 = false) {
  // party id n k session fingerprint claim-root raw-share shell sidecar key mask timeout pair-ready fd[12] ready-fd
  if (online_tls) {
    require(argc == 24,
        "usage: node party-tls id n k session fingerprint claim-root raw-share shell sidecar key mask timeout pair-ready bind-address local-port-base peer-address peer-port-base cert key ca expected-peer ready-fd");
  } else {
    require(argc == 28,
        "usage: node party id n k session fingerprint claim-root raw-share shell sidecar key mask timeout pair-ready fd0..fd11 ready-fd");
  }
  const auto party = static_cast<std::uint8_t>(std::stoul(argv[2]));
  const auto n = static_cast<std::uint32_t>(std::stoul(argv[3]));
  const auto k = static_cast<std::uint32_t>(std::stoul(argv[4]));
  const auto session = std::stoull(argv[5]);
  const auto fingerprint = std::stoull(argv[6]);
  const auto timeout = std::stoi(argv[13]);
  require(!conditional_secure_v1 || online_tls,
          "conditional BMW16 v1 requires the authenticated TLS party mode");
  require(!conditional_secure_v2 || online_tls,
          "conditional BMW16 v2 requires the authenticated TLS party mode");
  auto c = config(n, k, session, fingerprint, party, timeout, argv[7]);
  c.require_authenticated_transport = online_tls;
#if defined(MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS)
  c.test_only_force_probability_abort_after_select =
      test_failpoint_is("force_probability_abort_after_select");
  c.test_only_force_engineering_failure_after_select =
      test_failpoint_is("force_engineering_failure_after_select");
  c.test_only_force_final_status_disagreement =
      test_failpoint_is("force_final_status_disagreement");
#endif
  ProtocolIBmw16ExperimentalPartyMaterial material;
  std::unique_ptr<ProtocolIBmw16PersistentClaimStore> claim;
  std::shared_ptr<ProtocolIBmw16ProcessSlotClaimSet> slots;
  std::uint64_t shell_open_us = 0, bundle_claim_us = 0, sidecar_validate_us = 0;
  require_party_owned_private_file(argv[8], false);
  if (n > 1) {
    require_party_owned_private_file(argv[7], true);
    require_party_owned_private_file(argv[9], false);
    require_party_owned_private_file(argv[10], false);
    require_party_owned_private_file(argv[11], false);
    try {
      auto open_start = std::chrono::steady_clock::now();
      auto package_bytes = protocol_i_bmw16_bundle_read(argv[9]);
      auto shell = protocol_i_bmw16_bundle_open_stream_shell_party(c, party, package_bytes,
                                                                    read_key(argv[11]));
      shell_open_us = elapsed_us(open_start);
      verify_ready_marker(argv[14], c, party, shell);
      claim = std::make_unique<ProtocolIBmw16PersistentClaimStore>(argv[7], session, party,
          ProtocolIBmw16ClaimRootIdentity{c.claim_root_device, c.claim_root_inode});
      const auto claim_start = std::chrono::steady_clock::now();
      claim->claim_bundle(shell.stats.manifest_sha256);
      bundle_claim_us = elapsed_us(claim_start);
      material = std::move(shell.material);
      const auto sidecar_start = std::chrono::steady_clock::now();
      auto store = ProtocolIBmw16StreamedUcmpSlotStore::open_party(
          argv[10], c, party, read_key(argv[11]), shell.stream.stream_id,
          shell.stream.sidecar_sha256);
      sidecar_validate_us = elapsed_us(sidecar_start);
      if (store->slots() != shell.stream.ucmp_slot_count ||
          store->file_bytes() != shell.stream.sealed_sidecar_bytes)
        throw std::runtime_error("BMW16 stream sidecar/shell dimensions");
      material.load_ucmp_slot = [store](const ProtocolIBmw16MaterialSlot& slot) {
        return store->load_once(slot);
      };
    } catch (const ProtocolIBmw16ExpectedFailure&) {
      throw;
    } catch (const std::exception& error) {
      throw ProtocolIBmw16ExpectedFailure(ProtocolIBmw16ExpectedFailureKind::Material,
                                           error.what());
    }
    slots = std::make_shared<ProtocolIBmw16ProcessSlotClaimSet>();
    material.process_slot_claim_once = [slots](std::uint64_t id) { slots->claim_once(id); };
  }

  std::unique_ptr<ProtocolIBmw16OnlineTlsChannels> online_tls_channels;
  std::array<int, kChannelCount> all_fds{};
  ReadyBarrierStats ready{};
  if (online_tls) {
    const auto parse_port = [&](int index) {
      const auto parsed = std::stoul(argv[index]);
      require(parsed <= 65535U, "online TLS port range");
      return static_cast<std::uint16_t>(parsed);
    };
    ProtocolIBmw16OnlineTlsConfig online_config;
    online_config.session = session;
    online_config.fingerprint = fingerprint;
    online_config.n = n;
    online_config.k = k;
    online_config.comparison_bits = c.comparison_bits;
    online_config.party = party;
    online_config.timeout_ms = timeout;
    online_config.bind_address = argv[15];
    online_config.local_port_base = party == 1 ? parse_port(16) : 0;
    online_config.peer_address = party == 0 ? argv[17] : std::string{};
    online_config.peer_port_base = party == 0 ? parse_port(18) : 0;
    online_config.certificate_pem = argv[19];
    online_config.private_key_pem = argv[20];
    online_config.trust_bundle_pem = argv[21];
    online_config.expected_peer_identity = argv[22];
    online_config.listener_ready_fd = std::stoi(argv[23]);
    require_party_owned_private_file(argv[20],false);
    online_tls_channels = std::make_unique<ProtocolIBmw16OnlineTlsChannels>(online_config);
    online_tls_channels->verify_all_registered();
    all_fds = online_tls_channels->party_fds();
#if defined(MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS)
    if (test_failpoint_is("online_missing_tls_stream"))
      protocol_i_discard_unclaimed_authenticated_stream(all_fds[0]);
    if (test_failpoint_is("online_silent_after_tls"))
      std::this_thread::sleep_for(std::chrono::milliseconds(timeout) * 3);
#endif
  } else {
    for (std::size_t i = 0; i < all_fds.size(); ++i)
      all_fds[i] = std::stoi(argv[15 + i]);
    ready = exchange_ready(std::stoi(argv[27]), c);
  }
  std::array<int, 2> score{{all_fds[0], all_fds[1]}};
  std::array<int, 2> forward{{all_fds[2], all_fds[3]}};
  std::array<int, 4> select{{all_fds[4], all_fds[5], all_fds[6], all_fds[7]}};
  std::array<int, 2> inverse{{all_fds[8], all_fds[9]}};
  const int agreement = all_fds[10];
  const int coin = all_fds[11];
  auto raw = decode_raw_share(read_bytes(argv[8]), n);
  resetDcfPrgCallCounts();
  auto result = [&] {
    if (conditional_secure_v1) {
#if defined(MOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V1)
      return protocol_i_bmw16_conditional_secure_v1_raw_score_mask_party(
          c, std::move(material), raw, score, forward, select, inverse, agreement, coin);
#else
      throw std::runtime_error("conditional BMW16 security v1 was not compiled");
#endif
    }
    if (conditional_secure_v2) {
#if defined(MOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V2)
      return protocol_i_bmw16_conditional_secure_v2_raw_score_mask_party(
          c, std::move(material), raw, score, forward, select, inverse, agreement, coin);
#else
      throw std::runtime_error("conditional BMW16 security v2 was not compiled");
#endif
    }
    return protocol_i_bmw16_experimental_raw_score_mask_party(
        c, std::move(material), raw, score, forward, select, inverse, agreement, coin, nullptr);
  }();
  const auto dcf_counts = getDcfPrgCallCounts();
  const std::string status = result.status ? result.status : "ERROR_UNEXPECTED";
  const std::string scope = result.abort_scope ? result.abort_scope : "PROCESS_ERROR";
  int exit_code = kUnexpected;
  if (status == "SUCCESS" && scope == "NONE") {
    write_mask_exclusive(argv[12], result.xor_mask_share);
    exit_code = kSuccess;
  } else if (status == "ABORT_ALGORITHM_PROBABILITY" && scope == "PEER_AGREED") {
    exit_code = kAlgorithmAbort;
  } else if (status == "ABORT_MATERIAL") {
    exit_code = kMaterialAbort;
  } else if (status == "ABORT_COMMUNICATION") {
    exit_code = kCommunicationAbort;
  } else if (status == "ABORT_ALGORITHM_INVALID" && result.engineering_failure) {
    exit_code = kUnexpected;
  }
  const auto emit_array = [](const char* name, const std::array<std::uint64_t, 4>& values) {
    std::cout << ' ' << name << "=[" << values[0] << ',' << values[1] << ','
              << values[2] << ',' << values[3] << ']';
  };
  std::cout << "role=P" << static_cast<unsigned>(party) << " status=" << status
            << " scope=" << scope << " exit_code=" << exit_code
            << " local_reason=" << result.abort_reason
            << " mask=" << (exit_code == kSuccess ? "PUBLISHED_SHARE" : "NONE")
            << " logical_comparisons="
            << std::accumulate(result.metrics.logical_comparison_calls.begin(),
                               result.metrics.logical_comparison_calls.end(), UINT64_C(0))
            << " ucmp_eval=" << result.metrics.ucmp_party_evaluations
            << " dcf_eval=" << result.metrics.dcf_party_evaluations
            << " raw_adapter_ucmp_calls=" << result.metrics.raw_adapter_ucmp_calls
            << " raw_adapter_dcf_evaluations=" << result.metrics.raw_adapter_dcf_evaluations
            << " online_message_phases=" << result.metrics.online_message_phases
            << " online_bytes_sent=" << result.metrics.online_bytes_sent
            << " online_bytes_received=" << result.metrics.online_bytes_received
            << " online_time_us=" << result.metrics.online_time_us
            << " membership_slots_consumed=" << result.metrics.membership_slots_consumed
            << " sampler_sha256_counter_words=" << result.metrics.sampler_prf_words
            << " process_slots_claimed=" << result.metrics.process_slots_claimed
            << " dcf_counter_enabled=" << (dcf_counts.enabled ? "true" : "false")
            << " dcf_eval_calls_counted=" << dcf_counts.eval_calls
            << " dcf_eval_node_expansions=" << dcf_counts.eval_node_expansions
            << " dcf_eval_counter_matches_runtime="
            << (dcf_counts.enabled && dcf_counts.eval_calls == result.metrics.dcf_party_evaluations ? "true" : "false")
            << " shell_open_us=" << shell_open_us
            << " bundle_claim_us=" << bundle_claim_us
            << " sidecar_validate_us=" << sidecar_validate_us
            << " peak_rss_kb=" << peak_rss_kb()
            << " ready_barrier_us=" << ready.elapsed_us
            << " ready_sent_bytes=" << ready.sent
            << " ready_received_bytes=" << ready.received
            << " online_transport=" << (online_tls ? "mutual_TLS_1.3_data_path" : "caller_FD")
            << " online_tls_channels=" << (online_tls ? kChannelCount : 0U);
  emit_array("select_logical_calls_r1_r4", result.metrics.logical_comparison_calls);
  emit_array("select_unique_slots_r1_r4", result.metrics.unique_select_slots_consumed);
  emit_array("select_dummy_calls_r1_r4", result.metrics.dummy_related_calls);
  emit_array("select_repeated_calls_r1_r4", result.metrics.repeated_logical_calls);
  emit_array("select_round_time_us_r1_r4", result.metrics.select_round_time_us);
  emit_array("select_eval_time_us_r1_r4", result.metrics.select_eval_time_us);
  emit_array("select_exchange_time_us_r1_r4", result.metrics.select_exchange_time_us);
  std::cout << " raw_adapter_time_us=" << result.metrics.raw_adapter_time_us
            << " forward_shuffle_time_us=" << result.metrics.forward_shuffle_time_us
            << " sampling_coin_exchange_time_us=" << result.metrics.sampling_coin_exchange_time_us
            << " membership_time_us=" << result.metrics.membership_time_us
            << " inverse_shuffle_time_us=" << result.metrics.inverse_shuffle_time_us
            << " status_coordination_time_us=" << result.metrics.status_coordination_time_us;
  std::cout << '\n';
  return exit_code;
}
}  // namespace

int main(int argc, char** argv) {
  // Peer-close and negative-delivery paths must reach the structured exit-code
  // contract instead of being terminated asynchronously by SIGPIPE.
  std::signal(SIGPIPE,SIG_IGN);
  try {
#if defined(MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS)
    if (argc > 1 && std::string(argv[1]) == "t-local-test-only") return t_offline(argc, argv);
#endif
    if (argc > 1 && std::string(argv[1]) == "t-tls") return t_tls_offline(argc, argv);
    if (argc > 1 && std::string(argv[1]) == "recv-tls") return receive_tls(argc, argv);
    if (argc > 1 && std::string(argv[1]) == "party") return party_online(argc, argv);
    if (argc > 1 && std::string(argv[1]) == "party-tls") return party_online(argc, argv, true);
#if defined(MOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V1)
    if (argc > 1 && std::string(argv[1]) == "party-conditional-secure-v1")
      return party_online(argc, argv, true, true);
#endif
#if defined(MOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V2)
    if (argc > 1 && std::string(argv[1]) == "party-conditional-secure-v2")
      return party_online(argc, argv, true, false, true);
#endif
    std::cerr << "BMW16-derived Select / PROJECT_DERIVED / EXPERIMENTAL\n"
              << "usage: node t-tls ... | node recv-tls ... | node party ... | node party-tls ...";
#if defined(MOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V1)
    std::cerr << " | node party-conditional-secure-v1 ...";
#endif
#if defined(MOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V2)
    std::cerr << " | node party-conditional-secure-v2 ...";
#endif
#if defined(MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS)
    std::cerr << " | node t-local-test-only ...";
#endif
    std::cerr << '\n';
    return 64;
  } catch (const ProtocolIBmw16MaterialDeliveryError& error) {
    if(error.kind()==ProtocolIBmw16MaterialDeliveryError::Kind::Transport){
      std::cerr<<"status=ABORT_COMMUNICATION scope=LOCAL_ONLY reason="<<error.what()<<"\n";
      return kCommunicationAbort;
    }
    std::cerr<<"status=ABORT_MATERIAL scope=LOCAL_ONLY reason="<<error.what()<<"\n";
    return kMaterialAbort;
  } catch (const ProtocolITransportError& error) {
    std::cerr << "status=ABORT_COMMUNICATION scope=LOCAL_ONLY reason="
              << error.what() << "\n";
    return kCommunicationAbort;
  } catch (const ProtocolIBmw16ExpectedFailure& error) {
    std::cerr << "status=ABORT_MATERIAL scope=LOCAL_ONLY reason="
              << error.what() << "\n";
    return kMaterialAbort;
  } catch (const std::exception& error) {
    std::cerr << "status=ERROR_UNEXPECTED scope=PROCESS_ERROR reason="
              << error.what() << "\n";
    return kUnexpected;
  }
}
