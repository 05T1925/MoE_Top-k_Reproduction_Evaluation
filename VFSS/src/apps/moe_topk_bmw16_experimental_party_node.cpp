#include <moe_topk/experimental_bmw16_material_bundle.h>
#include <moe_topk/experimental_bmw16_select_party.h>
#include <moe_topk/protocol_i_transport.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
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
constexpr std::size_t kChannelCount = 12;

void require(bool value, const char* reason) {
  if (!value) throw std::runtime_error(reason);
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

void require_party_owned_private_file(const std::string& path, bool directory) {
  struct stat st{};
  if (::lstat(path.c_str(), &st) != 0 ||
      (directory ? !S_ISDIR(st.st_mode) : !S_ISREG(st.st_mode)) ||
      st.st_uid != ::geteuid() || (st.st_mode & 0077) != 0)
    throw std::runtime_error("party file ownership/permission contract");
}

void chown_bundle_for_party(const std::string& path, uid_t owner) {
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

void write_mask_exclusive(const std::string& path,
                          const std::vector<std::uint8_t>& bytes) {
  if (path.empty() || bytes.empty()) throw std::invalid_argument("mask output path/length");
  const auto temp = path + ".tmp." + std::to_string(::getpid());
  const int fd = ::open(temp.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
  if (fd < 0) throw std::runtime_error("mask output create");
  std::size_t offset = 0;
  while (offset < bytes.size()) {
    const auto wrote = ::write(fd, bytes.data() + offset, bytes.size() - offset);
    if (wrote < 0 && errno == EINTR) continue;
    if (wrote <= 0) {
      ::close(fd);
      ::unlink(temp.c_str());
      throw std::runtime_error("mask output write");
    }
    offset += static_cast<std::size_t>(wrote);
  }
  bool okay = ::fdatasync(fd) == 0;
  okay = ::close(fd) == 0 && okay;
  if (!okay || ::link(temp.c_str(), path.c_str()) != 0) {
    ::unlink(temp.c_str());
    throw std::runtime_error("mask output durable publish");
  }
  if (::unlink(temp.c_str()) != 0) throw std::runtime_error("mask output temp unlink");
}

ProtocolIBmw16ExperimentalPartyConfig config(std::uint32_t n, std::uint32_t k,
    std::uint64_t session, std::uint64_t fingerprint, std::uint8_t party,
    int timeout_ms, const std::string& claim_root) {
  if (party > 1 || n == 0 || n > 256 || k == 0 || k > n ||
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
  // T is a one-shot offline process. Root is required only to place each
  // sealed package into its pre-provisioned, party-owned local drop directory.
  // t n k session fingerprint claim0 claim1 bundle0 bundle1 key0 key1 uid0 uid1
  require(argc == 14, "usage: node t n k session fingerprint claim0 claim1 bundle0 bundle1 key0 key1 uid0 uid1");
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
  auto materials = protocol_i_bmw16_experimental_material_generate(c0);
  ProtocolIBmw16BundleStats b0{}, b1{};
  const auto e0 = protocol_i_bmw16_bundle_seal_party(c0, 0, materials.party0, read_key(argv[10]), &b0);
  const auto e1 = protocol_i_bmw16_bundle_seal_party(c1, 1, materials.party1, read_key(argv[11]), &b1);
  protocol_i_bmw16_bundle_write_atomic(argv[8], e0);
  protocol_i_bmw16_bundle_write_atomic(argv[9], e1);
  chown_bundle_for_party(argv[8], static_cast<uid_t>(std::stoul(argv[12])));
  chown_bundle_for_party(argv[9], static_cast<uid_t>(std::stoul(argv[13])));
  std::cout << "role=T status=OFFLINE_MATERIAL_READY"
            << " scheme=Protocol_I_BMW16_DERIVED_SELECT_DCF"
            << " label=PROJECT_DERIVED/EXPERIMENTAL"
            << " session=" << session << " n=" << n << " K=" << k
            << " slots_per_party=" << b0.slot_count
            << " bundle_bytes_p0=" << b0.envelope_bytes
            << " bundle_bytes_p1=" << b1.envelope_bytes
            << " root_bound_v3=true T_online=false\n";
  return kSuccess;
}

int party_online(int argc, char** argv) {
  // party id n k session fingerprint claim-root raw-share bundle key mask timeout fd[12] ready-fd
  require(argc == 26,
      "usage: node party id n k session fingerprint claim-root raw-share bundle key mask timeout fd0..fd11 ready-fd");
  const auto party = static_cast<std::uint8_t>(std::stoul(argv[2]));
  const auto n = static_cast<std::uint32_t>(std::stoul(argv[3]));
  const auto k = static_cast<std::uint32_t>(std::stoul(argv[4]));
  const auto session = std::stoull(argv[5]);
  const auto fingerprint = std::stoull(argv[6]);
  const auto timeout = std::stoi(argv[12]);
  auto c = config(n, k, session, fingerprint, party, timeout, argv[7]);
  ProtocolIBmw16ExperimentalPartyMaterial material;
  std::unique_ptr<ProtocolIBmw16PersistentClaimStore> claim;
  std::shared_ptr<ProtocolIBmw16ProcessSlotClaimSet> slots;
  require_party_owned_private_file(argv[8], false);
  if (n > 1) {
    require_party_owned_private_file(argv[7], true);
    require_party_owned_private_file(argv[9], false);
    require_party_owned_private_file(argv[10], false);
    try {
      auto package_bytes = protocol_i_bmw16_bundle_read(argv[9]);
      ProtocolIBmw16BundleStats package_stats{};
      material = protocol_i_bmw16_bundle_open_party(c, party, package_bytes,
                                                     read_key(argv[10]), &package_stats);
      claim = std::make_unique<ProtocolIBmw16PersistentClaimStore>(argv[7], session, party,
          ProtocolIBmw16ClaimRootIdentity{c.claim_root_device, c.claim_root_inode});
      claim->claim_bundle(package_stats.manifest_sha256);
    } catch (const ProtocolIBmw16ExpectedFailure&) {
      throw;
    } catch (const std::exception& error) {
      throw ProtocolIBmw16ExpectedFailure(ProtocolIBmw16ExpectedFailureKind::Material,
                                           error.what());
    }
    slots = std::make_shared<ProtocolIBmw16ProcessSlotClaimSet>();
    material.process_slot_claim_once = [slots](std::uint64_t id) { slots->claim_once(id); };
  }

  // The caller injects twelve already-connected descriptors from its private,
  // authenticated transport. Each phase descriptor is consumed by the API.
  std::array<int, 2> score{{std::stoi(argv[13]), std::stoi(argv[14])}};
  std::array<int, 2> forward{{std::stoi(argv[15]), std::stoi(argv[16])}};
  std::array<int, 4> select{{std::stoi(argv[17]), std::stoi(argv[18]),
                             std::stoi(argv[19]), std::stoi(argv[20])}};
  std::array<int, 2> inverse{{std::stoi(argv[21]), std::stoi(argv[22])}};
  const int agreement = std::stoi(argv[23]);
  const int coin = std::stoi(argv[24]);
  const int ready_fd = std::stoi(argv[25]);
  auto raw = decode_raw_share(read_bytes(argv[8]), n);
  const auto ready = exchange_ready(ready_fd, c);
  auto result = protocol_i_bmw16_experimental_raw_score_mask_party(
      c, std::move(material), raw, score, forward, select, inverse, agreement, coin, nullptr);
  const std::string status = result.status ? result.status : "ERROR_UNEXPECTED";
  const std::string scope = result.abort_scope ? result.abort_scope : "PROCESS_ERROR";
  int exit_code = kUnexpected;
  if (status == "SUCCESS" && scope == "NONE") {
    write_mask_exclusive(argv[11], result.xor_mask_share);
    exit_code = kSuccess;
  } else if (status == "ABORT_ALGORITHM_PROBABILITY" && scope == "PEER_AGREED") {
    exit_code = kAlgorithmAbort;
  } else if (status == "ABORT_MATERIAL") {
    exit_code = kMaterialAbort;
  } else if (status == "ABORT_COMMUNICATION") {
    exit_code = kCommunicationAbort;
  }
  std::cout << "role=P" << static_cast<unsigned>(party) << " status=" << status
            << " scope=" << scope << " exit_code=" << exit_code
            << " mask=" << (exit_code == kSuccess ? "PUBLISHED_SHARE" : "NONE")
            << " logical_comparisons="
            << std::accumulate(result.metrics.logical_comparison_calls.begin(),
                               result.metrics.logical_comparison_calls.end(), UINT64_C(0))
            << " ucmp_eval=" << result.metrics.ucmp_party_evaluations
            << " dcf_eval=" << result.metrics.dcf_party_evaluations
            << " ready_barrier_us=" << ready.elapsed_us
            << " ready_sent_bytes=" << ready.sent
            << " ready_received_bytes=" << ready.received << "\n";
  return exit_code;
}
}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc > 1 && std::string(argv[1]) == "t") return t_offline(argc, argv);
    if (argc > 1 && std::string(argv[1]) == "party") return party_online(argc, argv);
    std::cerr << "BMW16-derived Select / PROJECT_DERIVED / EXPERIMENTAL\n"
              << "usage: node t ... | node party ...\n";
    return 64;
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
