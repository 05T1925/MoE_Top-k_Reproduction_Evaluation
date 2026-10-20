#include <moe_topk/protocol_i_project_derived_select5.h>
#include <moe_topk/protocol_i_material_slot_store.h>
#include <moe_topk/protocol_i_priority_key.h>
#include <moe_topk/topk_oracle.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <grp.h>
#include <unistd.h>
#include <vector>

namespace {
using namespace moe_topk;
ProtocolIDcfAbi g_dcf_abi = ProtocolIDcfAbi::LegacyM2UcV1;
ProtocolIScoreShareAbi g_score_share_abi =
    ProtocolIScoreShareAbi::S15InducedV1;

void check(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}

void write_exact(int fd, const void* data, std::size_t size) {
  const auto* cursor = static_cast<const std::uint8_t*>(data);
  while (size != 0) {
    const auto count = ::write(fd, cursor, size);
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) throw std::runtime_error("e2e pipe write");
    cursor += count;
    size -= static_cast<std::size_t>(count);
  }
}

void read_exact(int fd, void* data, std::size_t size) {
  auto* cursor = static_cast<std::uint8_t*>(data);
  while (size != 0) {
    const auto count = ::read(fd, cursor, size);
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) throw std::runtime_error("e2e pipe read");
    cursor += count;
    size -= static_cast<std::size_t>(count);
  }
}

void put_u64(int fd, std::uint64_t value) {
  std::uint8_t bytes[8];
  for (int i = 0; i < 8; ++i)
    bytes[i] = static_cast<std::uint8_t>(value >> (56U - 8U * i));
  write_exact(fd, bytes, sizeof(bytes));
}

std::uint64_t fresh_public_id() {
  std::random_device source;
  return (static_cast<std::uint64_t>(source()) << 32U) ^ source();
}

std::string hex_u64(std::uint64_t value) {
  char text[17]{};
  const int count = std::snprintf(text, sizeof(text), "%016llx",
      static_cast<unsigned long long>(value));
  if (count != 16) throw std::runtime_error("public ID formatting");
  return text;
}

std::uint64_t get_u64(int fd) {
  std::uint8_t bytes[8];
  read_exact(fd, bytes, sizeof(bytes));
  std::uint64_t value = 0;
  for (const auto byte : bytes) value = (value << 8U) | byte;
  return value;
}

void pipe_pair(std::array<int, 2>& pair) {
  if (::pipe(pair.data()) != 0) throw std::runtime_error("pipe");
}

void socket_pair(std::array<int, 2>& pair) {
  if (::socketpair(AF_UNIX, SOCK_STREAM, 0, pair.data()) != 0)
    throw std::runtime_error("socketpair");
}

bool contains(const std::vector<int>& keep, int fd) {
  return std::find(keep.begin(), keep.end(), fd) != keep.end();
}

void close_except(const std::vector<int>& all, const std::vector<int>& keep) {
  for (const auto fd : all)
    if (fd > STDERR_FILENO && !contains(keep, fd)) (void)::close(fd);
}

struct TestCredentials {
  std::string directory;
  std::string ca, p0_cert, p0_key, p1_cert, p1_key, wrong_cert, wrong_key;
};

void run_test_command(const std::string& command) {
  check(std::system(command.c_str()) == 0, "E2E ephemeral TLS identity setup");
}

void write_ca_config(const std::string& path) {
  std::ofstream out(path);
  out << "[req]\n"
      << "distinguished_name=dn\n"
      << "prompt=no\n"
      << "[dn]\nCN=M6B5-E2E-CA\n"
      << "[v3_ca]\n"
      << "basicConstraints=critical,CA:TRUE\n"
      << "keyUsage=critical,keyCertSign,cRLSign\n"
      << "subjectKeyIdentifier=hash\n"
      << "authorityKeyIdentifier=keyid:always\n";
}

void make_e2e_leaf(const std::string& dir, const std::string& name,
                   const std::string& dns) {
  const auto key = dir + "/" + name + ".key";
  const auto csr = dir + "/" + name + ".csr";
  const auto cert = dir + "/" + name + ".pem";
  const auto ext = dir + "/" + name + ".ext";
  {
    std::ofstream out(ext);
    out << "[leaf]\n"
        << "basicConstraints=critical,CA:FALSE\n"
        << "keyUsage=critical,digitalSignature,keyEncipherment\n"
        << "extendedKeyUsage=clientAuth,serverAuth\n"
        << "subjectAltName=DNS:" << dns << "\n";
  }
  run_test_command("openssl req -new -newkey rsa:2048 -sha256 -nodes -keyout " +
      key + " -out " + csr + " -subj /CN=" + dns + " >/dev/null 2>&1");
  run_test_command("openssl x509 -req -in " + csr + " -CA " + dir +
      "/ca.pem -CAkey " + dir + "/ca.key -CAcreateserial -days 1 -sha256 "
      "-extfile " + ext + " -extensions leaf -out " + cert +
      " >/dev/null 2>&1");
  run_test_command("openssl verify -CAfile " + dir + "/ca.pem " + cert +
                   " >/dev/null 2>&1");
}

TestCredentials make_test_credentials() {
  char path[] = "/tmp/m6b5-e2e-XXXXXX";
  char* created = ::mkdtemp(path);
  check(created != nullptr, "E2E TLS temporary directory");
  TestCredentials result;
  result.directory = created;
  result.ca = result.directory + "/ca.pem";
  result.p0_cert = result.directory + "/p0.pem";
  result.p0_key = result.directory + "/p0.key";
  result.p1_cert = result.directory + "/p1.pem";
  result.p1_key = result.directory + "/p1.key";
  result.wrong_cert = result.directory + "/wrong.pem";
  result.wrong_key = result.directory + "/wrong.key";
  write_ca_config(result.directory + "/ca.cnf");
  run_test_command("openssl req -x509 -newkey rsa:2048 -sha256 -nodes -days 1 "
      "-keyout " + result.directory + "/ca.key -out " + result.ca +
      " -config " + result.directory + "/ca.cnf -extensions v3_ca "
      ">/dev/null 2>&1");
  make_e2e_leaf(result.directory, "p0", "party0.test");
  make_e2e_leaf(result.directory, "p1", "party1.test");
  make_e2e_leaf(result.directory, "wrong", "unexpected.test");
  check(::chmod(result.directory.c_str(), 0711) == 0,
        "E2E credential directory traversal mode");
  check(::chmod(result.ca.c_str(), 0444) == 0 &&
            ::chmod(result.p0_cert.c_str(), 0444) == 0 &&
            ::chmod(result.p1_cert.c_str(), 0444) == 0 &&
            ::chmod(result.wrong_cert.c_str(), 0444) == 0,
        "E2E public certificate permissions");
  check(::chown(result.p0_key.c_str(), 1000, 1000) == 0 &&
            ::chmod(result.p0_key.c_str(), 0400) == 0 &&
            ::chown(result.p1_key.c_str(), 65534, 65534) == 0 &&
            ::chmod(result.p1_key.c_str(), 0400) == 0 &&
            ::chown(result.wrong_key.c_str(), 65534, 65534) == 0 &&
            ::chmod(result.wrong_key.c_str(), 0400) == 0,
        "E2E party private-key ownership");
  return result;
}

std::string make_private_store() {
  char path[] = "/tmp/m6b5-store-e2e-XXXXXX";
  char* created = ::mkdtemp(path);
  check(created != nullptr, "E2E material store temporary directory");
  check(::chmod(created, 0711) == 0, "material supervisor root mode");
  for (const auto* name : {"p0", "p1"}) {
    const auto child = std::string(created) + "/" + name;
    check(::mkdir(child.c_str(), 0700) == 0,
          "create party-scoped store directory");
  }
  return created;
}

std::string party_store(const std::string& root, std::uint8_t party) {
  return root + (party == 0 ? "/p0" : "/p1");
}

uid_t party_uid(std::uint8_t party) {
  return party == 0 ? static_cast<uid_t>(1000) : static_cast<uid_t>(65534);
}

void assign_party_store_owner(const std::string& path, std::uint8_t party) {
  const auto uid = party_uid(party);
  for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
    const auto item = entry.path().string();
    check(::chown(item.c_str(), uid, uid) == 0,
          "supervisor assigns party-scoped material owner");
    if (entry.is_directory())
      check(::chmod(item.c_str(), 0700) == 0,
            "party-scoped nested directory mode");
  }
  check(::chown(path.c_str(), uid, uid) == 0 && ::chmod(path.c_str(), 0700) == 0,
        "party-scoped store root mode");
}

struct TemporaryArtifacts {
  std::string store;
  std::string credentials;
  ~TemporaryArtifacts() {
    std::error_code ignored;
    if (!store.empty()) std::filesystem::remove_all(store, ignored);
    ignored.clear();
    if (!credentials.empty()) std::filesystem::remove_all(credentials, ignored);
  }
};

void wrap_endpoint(ProtocolIFramedEndpoint& endpoint, SSL_CTX* context,
                   std::uint8_t party) {
  endpoint = protocol_i_tls13_wrap_fd(endpoint.fd, context, party == 1,
      party == 0 ? "party1.test" : "party0.test");
}

void wrap_all_endpoints(ProtocolIProjectDerivedSelect5Fds& fds,
                        SSL_CTX* context, std::uint8_t party) {
  wrap_endpoint(fds.package_binding, context, party);
  for (auto& endpoint : fds.score_input) wrap_endpoint(endpoint, context, party);
  for (auto& endpoint : fds.forward_shuffle) wrap_endpoint(endpoint, context, party);
  for (auto& endpoint : fds.select_layers) wrap_endpoint(endpoint, context, party);
  for (auto& endpoint : fds.sample_schedule) wrap_endpoint(endpoint, context, party);
  wrap_endpoint(fds.membership, context, party);
  for (auto& endpoint : fds.inverse_shuffle) wrap_endpoint(endpoint, context, party);
  wrap_endpoint(fds.final_status, context, party);
}

void tamper_staged_party_package(const std::string& store_directory,
                                 std::uint64_t session,
                                 std::uint64_t fingerprint,
                                 std::uint64_t material_id) {
  char name[128]{};
  const int count = std::snprintf(name, sizeof(name),
      "%s/m6b5_%016llx_%016llx_%016llx_p0.pkg", store_directory.c_str(),
      static_cast<unsigned long long>(session),
      static_cast<unsigned long long>(fingerprint),
      static_cast<unsigned long long>(material_id));
  check(count > 0 && static_cast<std::size_t>(count) < sizeof(name),
        "tamper package path");
  check(::chmod(name, 0600) == 0, "make staged package tamperable in test");
  const int fd = ::open(name, O_WRONLY | O_CLOEXEC);
  check(fd >= 0, "open staged package for tamper");
  const std::uint8_t changed = 0xa5;
  check(::pwrite(fd, &changed, 1, 64) == 1, "write staged package tamper");
  ::close(fd);
  check(::chmod(name, 0400) == 0, "restore staged package permissions");
}

[[noreturn]] void dealer_child(
    std::uint32_t n, std::uint32_t k, std::uint64_t session,
    std::uint64_t fingerprint, std::uint64_t material_id,
    const std::array<std::string, 2>& party_store_directories,
    int dealer_record_fd,
    const std::vector<int>& all_fds, const std::vector<int>& keep,
    bool tamper_party0, bool fail_after_staging, bool streamed) {
  close_except(all_fds, keep);
  try {
    ProtocolIProjectDerivedSelect5DealerConfig config{
        session, fingerprint, material_id, n, k, 30000};
    config.dcf_abi = g_dcf_abi;
    config.score_share_abi = g_score_share_abi;
    if (streamed) {
      protocol_i_project_derived_select5_dealer_stage_streamed_material(
          party_store_directories, config);
      put_u64(dealer_record_fd, 0);
      put_u64(dealer_record_fd, 0);
    } else {
      auto output = protocol_i_project_derived_select5_dealer_generate(config);
      put_u64(dealer_record_fd, output.party0_package.size());
      put_u64(dealer_record_fd, output.party1_package.size());
      protocol_i_project_derived_select5_stage_material_packages(
          party_store_directories, config, output);
    }
    if (fail_after_staging) {
      std::cerr << "T exits abnormally before ready publication\n";
      _exit(2);
    }
    _exit(0);
  } catch (const std::exception& error) {
    std::cerr << "T setup failed: " << error.what() << '\n';
    _exit(2);
  }
}

[[noreturn]] void party_child(
    std::uint8_t party, std::uint32_t n, std::uint32_t k,
    std::uint64_t session, std::uint64_t fingerprint,
    std::uint64_t material_id, const std::string& store_directory,
    const TestCredentials& credentials, int input_fd, int result_fd,
    ProtocolIProjectDerivedSelect5Fds fds,
    const std::vector<int>& all_fds, const std::vector<int>& keep,
    bool replay_material, bool disconnect_after_input,
    bool wrong_peer_certificate, bool wrong_party_k, bool streamed,
    bool corrupt_sample_seed, std::uint8_t crash_step) {
  close_except(all_fds, keep);
  bool runtime_called = false;
  if (::geteuid() == 0) {
    const auto uid = party_uid(party);
    if (::setgroups(0, nullptr) != 0 || ::setgid(uid) != 0 ||
        ::setuid(uid) != 0) _exit(90);
  }
  std::uint32_t padded_n = 2;
  while (padded_n < n) padded_n <<= 1U;
  std::uint8_t index_bits = 0;
  for (auto value = padded_n - 1U; value != 0; value >>= 1U) ++index_bits;
  const auto configured_k = wrong_party_k ? (k == n ? k - 1U : k + 1U) : k;
  check(configured_k >= 1 && configured_k <= n, "fault K remains in range");
  ProtocolIProjectDerivedSelect5RuntimeConfig config{
      session, fingerprint, n, padded_n, configured_k,
      static_cast<std::uint8_t>(33U + index_bits), party, g_dcf_abi, 1000};
  config.material_id_base = material_id;
  config.score_share_abi = g_score_share_abi;
  config.material_store_directory = store_directory;
  config.streamed_material_store = streamed;
  config.expected_peer_identity = party == 0 ? "party1.test" : "party0.test";
  config.test_crash_after_output_step = party == 0 ? crash_step : 0;
  config.test_wire_trace_path = store_directory + "/test-wire-events.log";
  try {
    const ProtocolITls13Credentials tls_credentials{
        credentials.ca,
        party == 0 ? credentials.p0_cert :
            (wrong_peer_certificate ? credentials.wrong_cert : credentials.p1_cert),
        party == 0 ? credentials.p0_key :
            (wrong_peer_certificate ? credentials.wrong_key : credentials.p1_key)};
    SSL_CTX* tls_context = protocol_i_tls13_context_create(tls_credentials);
    wrap_all_endpoints(fds, tls_context, party);

    std::vector<std::uint32_t> raw_share(n);
    for (auto& value : raw_share) {
      std::uint8_t bytes[4];
      read_exact(input_fd, bytes, sizeof(bytes));
      value = (static_cast<std::uint32_t>(bytes[0]) << 24U) |
              (static_cast<std::uint32_t>(bytes[1]) << 16U) |
              (static_cast<std::uint32_t>(bytes[2]) << 8U) | bytes[3];
    }
    ::close(input_fd);

    if (disconnect_after_input) {
      // Controlled peer disconnect at the first online phase, followed by
      // the permitted uniform ABORT notification on the dedicated barrier.
      (void)::close(fds.score_input[0].fd);
      protocol_i_project_derived_select5_abort_no_mask(config, fds);
      const std::uint8_t marker = 0xA0;
      write_exact(result_fd, &marker, sizeof(marker));
      _exit(3);
    }

    runtime_called = true;
    ProtocolIProjectDerivedSelect5PartyOutput output;
    if (corrupt_sample_seed) {
      check(party == 0 && !streamed,
            "malformed public sample seed fault targets monolithic P0 test path");
      const auto package = protocol_i_project_derived_select5_claim_party_package(
          store_directory, session, fingerprint, material_id, n, padded_n, k,
          config.comparison_bits, party);
      auto material = protocol_i_project_derived_select5_deserialize_party_material(
          package, party);
      output = protocol_i_project_derived_select5_party_test_material(
          config, std::move(material), raw_share, fds, true);
    } else {
      output = protocol_i_project_derived_select5_party(config, raw_share, fds);
    }
    put_u64(result_fd, output.xor_mask_share.size());
    write_exact(result_fd, output.xor_mask_share.data(),
                output.xor_mask_share.size());
    const std::array<std::uint64_t, 22> wire_metrics{{
        output.metrics.binding_sent_bytes,
        output.metrics.binding_received_bytes,
        output.metrics.input_sent_bytes, output.metrics.input_received_bytes,
        output.metrics.forward_sent_bytes, output.metrics.forward_received_bytes,
        output.metrics.select_sent_bytes, output.metrics.select_received_bytes,
        output.metrics.membership_sent_bytes,
        output.metrics.membership_received_bytes,
        output.metrics.inverse_sent_bytes, output.metrics.inverse_received_bytes,
        output.metrics.final_status_sent_bytes,
        output.metrics.final_status_received_bytes,
        output.metrics.comparison_calls, output.metrics.select_dcf_eval_calls,
        output.metrics.membership_dcf_eval_calls,
        output.metrics.active_compare_layers,
        output.metrics.sample_schedule_sent_bytes,
        output.metrics.sample_schedule_received_bytes,
        output.metrics.active_sample_rounds, output.metrics.online_rounds}};
    for (const auto value : wire_metrics) put_u64(result_fd, value);
    bool replay_rejected = false;
    if (replay_material) {
      try {
        if (streamed) {
          ProtocolIMaterialSlotBinding binding{
              session, fingerprint, material_id, n, padded_n, k,
              config.comparison_bits, party, g_dcf_abi,
              5U * (static_cast<std::uint64_t>(padded_n) * (padded_n - 1U) / 2U) +
                  13U * padded_n +
                  (g_score_share_abi ==
                           ProtocolIScoreShareAbi::FullKeyZeroSumRefreshV2
                       ? padded_n : 0U)};
          binding.score_share_abi = g_score_share_abi;
          (void)protocol_i_streamed_material_claim_slot(store_directory,
              binding, {3, 0, 1, 0, 0, UINT32_MAX});
        } else {
          (void)protocol_i_project_derived_select5_claim_party_package(
              store_directory, session, fingerprint, material_id, n, padded_n,
              k, config.comparison_bits, party);
        }
      } catch (const std::exception&) {
        replay_rejected = true;
      }
      check(replay_rejected, "durable one-shot package replay rejected");
    }
    put_u64(result_fd, replay_rejected ? 1U : 0U);
    protocol_i_tls13_context_free(tls_context);
    ::close(result_fd);
    _exit(0);
  } catch (const std::exception& error) {
    std::cerr << "P" << static_cast<unsigned>(party)
              << " failed without mask: " << error.what() << '\n';
    if (!runtime_called && config.timeout_ms > 0) {
      try {
        protocol_i_project_derived_select5_abort_no_mask(config, fds);
        const std::uint8_t marker = 0xA0;
        write_exact(result_fd, &marker, sizeof(marker));
      } catch (...) {
      }
    }
    _exit(2);
  }
}

int wait_child(pid_t child) {
  int status = 0;
  while (::waitpid(child, &status, 0) < 0) {
    if (errno == EINTR) continue;
    throw std::runtime_error("waitpid");
  }
  if (!WIFEXITED(status)) return WIFSIGNALED(status) ? 128 + WTERMSIG(status) : 255;
  return WEXITSTATUS(status);
}

std::vector<std::uint32_t> scores_fixture(std::uint32_t n) {
  std::vector<std::uint32_t> scores(n);
  const std::array<std::uint32_t, 8> boundary{{
      UINT32_C(0x80000000), UINT32_C(0x7fffffff), UINT32_MAX, 0,
      UINT32_C(0x80000001), UINT32_C(0x7ffffffe), 0, UINT32_MAX}};
  for (std::uint32_t i = 0; i < n; ++i)
    scores[i] = i < boundary.size() ? boundary[i]
                                    : static_cast<std::uint32_t>((i % 7U) * 4096U);
  return scores;
}

std::vector<std::uint8_t> oracle_mask(const std::vector<std::uint32_t>& scores,
                                     std::uint32_t k) {
  return top_k_mask(scores, k);
}

struct StoreStats {
  std::uint64_t total = 0;
  std::array<std::uint64_t, 2> party_bytes{{0, 0}};
  std::array<std::uint64_t, 2> manifest_bytes{{0, 0}};
  std::array<std::uint64_t, 2> segment_count{{0, 0}};
};

StoreStats store_stats(const std::string& path) {
  StoreStats stats;
  for (std::uint8_t party = 0; party < 2; ++party) {
    for (const auto& entry : std::filesystem::directory_iterator(
             party_store(path, party))) {
      if (!entry.is_regular_file()) continue;
      const auto name = entry.path().filename().string();
      const auto bytes = entry.file_size();
      stats.total += bytes;
      stats.party_bytes[party] += bytes;
      if (name.find(".manifest") != std::string::npos)
        stats.manifest_bytes[party] += bytes;
      if (name.find(".seg") != std::string::npos)
        ++stats.segment_count[party];
    }
  }
  return stats;
}

int open_as_party(uid_t uid, const std::string& path, bool directory) {
  const pid_t child = ::fork();
  if (child < 0) throw std::runtime_error("fork party isolation probe");
  if (child == 0) {
    if (::setgroups(0, nullptr) != 0 || ::setgid(uid) != 0 ||
        ::setuid(uid) != 0) _exit(90);
    const int flags = O_RDONLY | O_CLOEXEC | O_NOFOLLOW |
                      (directory ? O_DIRECTORY : 0);
    const int fd = ::open(path.c_str(), flags);
    if (fd >= 0) {
      ::close(fd);
      _exit(0);
    }
    _exit(errno == EACCES || errno == EPERM || errno == ELOOP ||
                  errno == ENOTDIR ? 1 : 2);
  }
  return wait_child(child);
}

void verify_party_scoped_isolation(const std::string& root) {
  for (std::uint8_t party = 0; party < 2; ++party) {
    const auto peer = static_cast<std::uint8_t>(1U - party);
    const auto peer_dir = party_store(root, peer);
    check(open_as_party(party_uid(party), peer_dir, true) == 1,
          "opposite party cannot traverse its store directory");
    std::size_t sensitive_files = 0;
    for (const auto& entry : std::filesystem::directory_iterator(peer_dir)) {
      if (!entry.is_regular_file()) continue;
      ++sensitive_files;
      check(open_as_party(party_uid(party), entry.path().string(), false) == 1,
            "opposite party cannot read package/manifest/segment/lease state");
    }
    check(sensitive_files != 0, "party isolation probe found staged files");

    const auto peer_file = std::filesystem::directory_iterator(peer_dir)->path().string();
    const auto traversal = party_store(root, party) + "/../" +
        (peer == 0 ? "p0" : "p1") + "/" +
        std::filesystem::path(peer_file).filename().string();
    check(open_as_party(party_uid(party), traversal, false) == 1,
          "party path traversal denied by recipient directory ownership");
  }

  const auto p0_peer_link = party_store(root, 0) + "/peer-store-link";
  check(::symlink("../p1", p0_peer_link.c_str()) == 0,
        "create supervisor symlink traversal probe");
  check(open_as_party(party_uid(0), p0_peer_link, true) == 1,
        "party store symlink traversal is rejected");
}

void verify_sample_seed_wire_order(const std::string& party0_store,
                                   std::uint32_t expected_samples) {
  std::ifstream input(party0_store + "/test-wire-events.log");
  check(input.good(), "actual Select5 TEST_ONLY wire trace exists");
  std::vector<std::string> lines;
  for (std::string line; std::getline(input, line);)
    lines.push_back(std::move(line));
  std::size_t forward_index = lines.size();
  std::vector<std::pair<std::size_t, std::uint32_t>> seed_sends;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (lines[index].find("forward-shuffle-complete phase=21") == 0)
      forward_index = index;
    if (lines[index].find("sample-seed-send phase=") == 0) {
      const auto phase_start = lines[index].find("phase=") + 6;
      const auto phase_end = lines[index].find(' ', phase_start);
      const auto phase = static_cast<std::uint32_t>(
          std::stoul(lines[index].substr(phase_start, phase_end - phase_start)));
      const auto iteration_start = lines[index].find("iteration=") + 10;
      const auto iteration_end = lines[index].find(' ', iteration_start);
      const auto iteration = static_cast<std::uint32_t>(std::stoul(
          lines[index].substr(iteration_start, iteration_end - iteration_start)));
      check(phase == 40U + iteration,
            "seed wire frame is bound to its active iteration");
      seed_sends.emplace_back(index, iteration);
    }
  }
  check(forward_index < lines.size() && seed_sends.size() == expected_samples,
        "forward shuffle completes and each active iteration sends one seed");
  std::uint32_t previous_iteration = 0;
  bool first = true;
  for (const auto& event : seed_sends) {
    check(event.first > forward_index,
          "sample seed is actually sent after forward shuffle completion");
    check(first || event.second > previous_iteration,
          "future iteration seed is sent only after prior iteration activation");
    first = false;
    previous_iteration = event.second;
  }
}

void check_resource_preflights() {
  const ProtocolIProjectDerivedSelect5DealerConfig n256{
      9001, 9002, 9003, 256, 1, 1000};
  bool monolithic_rejected = false;
  try {
    (void)protocol_i_project_derived_select5_dealer_generate(n256);
  } catch (const std::exception&) {
    monolithic_rejected = true;
  }
  check(monolithic_rejected, "N=256 monolithic package preflight rejects");
  const ProtocolIProjectDerivedSelect5DealerConfig n257{
      9011, 9012, 9013, 257, 1, 1000};
  bool streamed_rejected = false;
  try {
    protocol_i_project_derived_select5_dealer_stage_streamed_material(
        std::array<std::string, 2>{{"/tmp/no-select5-material-store/p0",
                                    "/tmp/no-select5-material-store/p1"}},
        n257);
  } catch (const std::exception&) {
    streamed_rejected = true;
  }
  check(streamed_rejected, "N=257 streamed package preflight rejects");
  std::cout << "RESOURCE_PREFLIGHT_JSON {\"N256_monolithic\":\"REJECTED_BEFORE_GENERATION\","
               "\"N257_streamed\":\"REJECTED\"}\n";
}

void run_case(std::uint32_t n, std::uint32_t k, std::uint64_t serial,
              bool tamper_party0 = false, bool replay_material = true,
              bool disconnect_party1 = false, bool wrong_party_k = false,
              bool wrong_identity = false, bool fail_t = false,
              bool streamed = false, bool corrupt_sample_seed = false,
              std::uint8_t crash_step = 0) {
  std::uint64_t session = 0, fingerprint = 0, material_id = 0;
  do { session = fresh_public_id(); } while (session == 0);
  do { fingerprint = fresh_public_id(); }
  while (fingerprint == 0 || fingerprint == session);
  do { material_id = (fresh_public_id() & UINT64_C(0x3ffffffffffffffe)) | 1U; }
  while (material_id == session || material_id == fingerprint);
  std::uint32_t padded_n = 2;
  while (padded_n < n) padded_n <<= 1U;
  const auto store_directory = make_private_store();
  const std::array<std::string, 2> party_store_directories{{
      party_store(store_directory, 0), party_store(store_directory, 1)}};
  const auto credentials = make_test_credentials();
  TemporaryArtifacts cleanup{store_directory, credentials.directory};
  std::array<int, 2> dealer_record{}, input0{}, input1{}, result0{}, result1{};
  pipe_pair(dealer_record);
  pipe_pair(input0); pipe_pair(input1);
  pipe_pair(result0); pipe_pair(result1);

  std::array<std::array<int, 2>, 31> channels{};
  for (auto& pair : channels) socket_pair(pair);
  ProtocolIProjectDerivedSelect5Fds fds0, fds1;
  fds0.package_binding = channels[26][0];
  fds1.package_binding = channels[26][1];
  fds0.score_input = {{channels[0][0], channels[1][0]}};
  fds1.score_input = {{channels[0][1], channels[1][1]}};
  fds0.forward_shuffle = {{channels[2][0], channels[3][0]}};
  fds1.forward_shuffle = {{channels[2][1], channels[3][1]}};
  for (std::size_t i = 0; i < 18; ++i) {
    fds0.select_layers[i] = channels[4 + i][0];
    fds1.select_layers[i] = channels[4 + i][1];
  }
  for (std::size_t i = 0; i < 4; ++i) {
    fds0.sample_schedule[i] = channels[27 + i][0];
    fds1.sample_schedule[i] = channels[27 + i][1];
  }
  fds0.membership = channels[22][0]; fds1.membership = channels[22][1];
  fds0.inverse_shuffle = {{channels[23][0], channels[24][0]}};
  fds1.inverse_shuffle = {{channels[23][1], channels[24][1]}};
  fds0.final_status = channels[25][0]; fds1.final_status = channels[25][1];

  std::vector<int> all_fds;
  for (const auto* group : {&dealer_record, &input0, &input1,
                            &result0, &result1})
    all_fds.insert(all_fds.end(), group->begin(), group->end());
  for (const auto& pair : channels)
    all_fds.insert(all_fds.end(), pair.begin(), pair.end());

  const pid_t dealer = ::fork();
  if (dealer < 0) throw std::runtime_error("fork dealer");
  if (dealer == 0) {
    dealer_child(n, k, session, fingerprint, material_id,
                 party_store_directories,
                 dealer_record[1],
                 all_fds, {dealer_record[1]}, tamper_party0,
                 fail_t, streamed);
  }
  ::close(dealer_record[1]);
  // The supervisor publishes the ready marker only after T exited successfully.
  const auto dealer_status = wait_child(dealer);
  const auto package_bytes_p0 = get_u64(dealer_record[0]);
  const auto package_bytes_p1 = get_u64(dealer_record[0]);
  ::close(dealer_record[0]);
  const auto staged_store = store_stats(store_directory);
  ProtocolIProjectDerivedSelect5DealerConfig dealer_config{
      session, fingerprint, material_id, n, k, 30000};
  dealer_config.dcf_abi = g_dcf_abi;
  dealer_config.score_share_abi = g_score_share_abi;
  if (dealer_status == 0) {
    if (streamed)
      protocol_i_project_derived_select5_publish_streamed_material_ready(
          party_store_directories, dealer_config);
    else
      protocol_i_project_derived_select5_publish_material_ready(
          party_store_directories, dealer_config);
  }
  else
    check(fail_t, "offline T exited successfully before release");
  if (tamper_party0)
    tamper_staged_party_package(party_store_directories[0], session, fingerprint,
                                material_id);

  // T has exited and the trusted supervisor now transfers each complete
  // package namespace to its recipient UID before either online party starts.
  assign_party_store_owner(party_store_directories[0], 0);
  assign_party_store_owner(party_store_directories[1], 1);

  const pid_t p0 = ::fork();
  if (p0 < 0) throw std::runtime_error("fork P0");
  if (p0 == 0) {
    std::vector<int> keep{input0[0], result0[1]};
    for (const auto& pair : channels) keep.push_back(pair[0]);
    party_child(0, n, k, session, fingerprint, material_id,
                party_store_directories[0], credentials, input0[0], result0[1],
                fds0, all_fds, keep,
                replay_material, false, false, false, streamed,
                corrupt_sample_seed, crash_step);
  }
  const pid_t p1 = ::fork();
  if (p1 < 0) throw std::runtime_error("fork P1");
  if (p1 == 0) {
    std::vector<int> keep{input1[0], result1[1]};
    for (const auto& pair : channels) keep.push_back(pair[1]);
    party_child(1, n, k, session, fingerprint, material_id,
                party_store_directories[1], credentials, input1[0], result1[1],
                fds1, all_fds, keep,
                replay_material, disconnect_party1, wrong_identity,
                wrong_party_k, streamed, false, 0);
  }

  // The controller constructs scores only after T/P0/P1 have forked.
  std::vector<int> parent_keep{input0[1], input1[1], result0[0], result1[0]};
  close_except(all_fds, parent_keep);
  const auto scores = scores_fixture(n);
  std::mt19937_64 rng(UINT64_C(0x5241575348415245) ^ serial);
  std::vector<std::uint32_t> share0(n), share1(n);
  for (std::uint32_t i = 0; i < n; ++i) {
    share0[i] = static_cast<std::uint32_t>(rng());
    share1[i] = scores[i] - share0[i];
  }
  for (const auto* share : {&share0, &share1}) {
    const auto fd = share == &share0 ? input0[1] : input1[1];
    for (const auto value : *share) {
      const std::uint8_t bytes[4] = {
          static_cast<std::uint8_t>(value >> 24U),
          static_cast<std::uint8_t>(value >> 16U),
          static_cast<std::uint8_t>(value >> 8U),
          static_cast<std::uint8_t>(value)};
      write_exact(fd, bytes, sizeof(bytes));
    }
  }
  ::close(input0[1]); ::close(input1[1]);

  const auto p0_status = wait_child(p0);
  const auto p1_status = wait_child(p1);
  verify_party_scoped_isolation(store_directory);
  if (crash_step != 0) {
    check(p0_status == static_cast<int>(80U + crash_step),
          "P0 terminates at injected output boundary");
    const auto read_result_prefix = [](int fd, std::array<std::uint8_t, 8>& prefix) {
      std::size_t used = 0;
      while (used < prefix.size()) {
        const auto count = ::read(fd, prefix.data() + used, prefix.size() - used);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) break;
        used += static_cast<std::size_t>(count);
      }
      return used;
    };
    std::array<std::uint8_t, 8> prefix0{}, prefix1{};
    const auto bytes0 = read_result_prefix(result0[0], prefix0);
    const auto bytes1 = read_result_prefix(result1[0], prefix1);
    const auto has_share = [n](std::size_t bytes,
                               const std::array<std::uint8_t, 8>& prefix) {
      if (bytes != 8) return false;
      std::uint64_t length = 0;
      for (const auto byte : prefix) length = (length << 8U) | byte;
      return length == n;
    };
    check(!(has_share(bytes0, prefix0) && has_share(bytes1, prefix1)),
          "crash leaves at most one orphan local share, never a full mask");
    std::cout << "CRASH_CASE_JSON {\"n\":" << n << ",\"k\":" << k
              << ",\"case_id\":" << serial << ",\"step\":"
              << static_cast<unsigned>(crash_step) << ",\"T\":"
              << dealer_status << ",\"P0\":" << p0_status
              << ",\"P1\":" << p1_status << ",\"P0_output_bytes\":"
              << bytes0 << ",\"P1_output_bytes\":" << bytes1
              << ",\"result\":\"NO_COMPLETE_MASK\"}\n";
    return;
  }
  if (tamper_party0 || disconnect_party1 || wrong_party_k ||
      corrupt_sample_seed ||
      wrong_identity || fail_t) {
    std::cerr << "FAULT_PROCESS_STATUS_JSON {\"n\":" << n << ",\"k\":" << k
              << ",\"fault_tamper\":" << (tamper_party0 ? "true" : "false")
              << ",\"T\":" << dealer_status << ",\"P0\":" << p0_status
              << ",\"P1\":" << p1_status << "}\n";
    check((fail_t ? dealer_status != 0 : dealer_status == 0) &&
              p0_status != 0 && p1_status != 0,
          "faulted T/P0/P1 exit without mask");
    std::uint8_t p0_first = 0, p1_first = 0;
    const auto p0_read = ::read(result0[0], &p0_first, 1);
    const auto p1_read = ::read(result1[0], &p1_first, 1);
    check(p0_read == 0 || p0_first == 0xA0,
          "P0 failure returned no mask");
    check(p1_read == 0 || p1_first == 0xA0,
          "P1 failure returned no mask");
    check(p0_read <= 1 && p1_read <= 1,
          "fault path has no encoded mask output");
    const auto fault_name = tamper_party0 ? "tampered-package" :
                  (corrupt_sample_seed ? "sample-seed-invalid-magic" :
                  (wrong_party_k ? "wrong-party-K" :
                   (wrong_identity ? "tls-wrong-peer-identity" :
                    (fail_t ? "T-exit-before-ready" : "disconnect"))));
    std::cout << "CASE_JSON {\"n\":" << n << ",\"padded_n\":"
              << padded_n << ",\"k\":" << k
              << ",\"case_id\":" << serial
              << ",\"session_hex\":\"" << hex_u64(session)
              << "\",\"fingerprint_hex\":\"" << hex_u64(fingerprint)
              << "\",\"material_id_hex\":\"" << hex_u64(material_id) << "\""
              << ",\"input_share_seed_hex\":\""
              << hex_u64(UINT64_C(0x5241575348415245) ^ serial) << "\""
              << ",\"streamed\":" << (streamed ? "true" : "false")
              << ",\"staged_store_bytes\":" << staged_store.total
              << ",\"party_store_bytes_p0\":" << staged_store.party_bytes[0]
              << ",\"party_store_bytes_p1\":" << staged_store.party_bytes[1]
              << ",\"manifest_bytes_p0\":" << staged_store.manifest_bytes[0]
              << ",\"manifest_bytes_p1\":" << staged_store.manifest_bytes[1]
              << ",\"segments_p0\":" << staged_store.segment_count[0]
              << ",\"segments_p1\":" << staged_store.segment_count[1]
              << ",\"package_bytes_p0\":" << package_bytes_p0
              << ",\"package_bytes_p1\":" << package_bytes_p1
              << ",\"result\":\"UNIFORM_ABORT_NO_MASK\",\"fault\":\""
              << fault_name << "\"}\n";
    return;
  }
  if (dealer_status != 0 || p0_status != 0 || p1_status != 0) {
    std::cerr << "PROCESS_STATUS_JSON {\"n\":" << n << ",\"k\":" << k
              << ",\"streamed\":" << (streamed ? "true" : "false")
              << ",\"T\":" << dealer_status << ",\"P0\":" << p0_status
              << ",\"P1\":" << p1_status << "}\n";
    throw std::runtime_error("T/P0/P1 process exit");
  }

  char state_stem[128]{};
  const int state_name_length = std::snprintf(state_stem, sizeof(state_stem),
      "m6b5_%016llx_%016llx_%016llx",
      static_cast<unsigned long long>(session),
      static_cast<unsigned long long>(fingerprint),
      static_cast<unsigned long long>(material_id));
  check(state_name_length > 0 &&
            static_cast<std::size_t>(state_name_length) < sizeof(state_stem),
        "output state filename");
  for (std::uint8_t party = 0; party < 2; ++party) {
    for (const auto* suffix : {".run", ".prepared", ".commit", ".published"}) {
      const auto marker = party_store(store_directory, party) + "/" +
          state_stem + "_p" + std::to_string(party) + suffix;
      check(std::filesystem::is_regular_file(marker),
            "durable prepare/commit/publish state recorded per party");
    }
  }

  const auto out_n0 = get_u64(result0[0]);
  check(out_n0 == n, "P0 output shape");
  std::vector<std::uint8_t> mask0(n), mask1(n);
  read_exact(result0[0], mask0.data(), n);
  const std::array<std::uint64_t, 22> metrics0{{
      get_u64(result0[0]), get_u64(result0[0]),
      get_u64(result0[0]), get_u64(result0[0]), get_u64(result0[0]),
      get_u64(result0[0]), get_u64(result0[0]), get_u64(result0[0]),
      get_u64(result0[0]), get_u64(result0[0]), get_u64(result0[0]),
      get_u64(result0[0]), get_u64(result0[0]), get_u64(result0[0]),
      get_u64(result0[0]), get_u64(result0[0]), get_u64(result0[0]),
      get_u64(result0[0]), get_u64(result0[0]), get_u64(result0[0]),
      get_u64(result0[0]), get_u64(result0[0])}};
  const auto replay0 = get_u64(result0[0]);
  const auto out_n1 = get_u64(result1[0]);
  check(out_n1 == n, "P1 output shape");
  read_exact(result1[0], mask1.data(), n);
  const std::array<std::uint64_t, 22> metrics1{{
      get_u64(result1[0]), get_u64(result1[0]),
      get_u64(result1[0]), get_u64(result1[0]), get_u64(result1[0]),
      get_u64(result1[0]), get_u64(result1[0]), get_u64(result1[0]),
      get_u64(result1[0]), get_u64(result1[0]), get_u64(result1[0]),
      get_u64(result1[0]), get_u64(result1[0]), get_u64(result1[0]),
      get_u64(result1[0]), get_u64(result1[0]), get_u64(result1[0]),
      get_u64(result1[0]), get_u64(result1[0]), get_u64(result1[0]),
      get_u64(result1[0]), get_u64(result1[0])}};
  const auto replay1 = get_u64(result1[0]);
  const auto expected = oracle_mask(scores, k);
  std::uint32_t weight = 0;
  for (std::uint32_t i = 0; i < n; ++i) {
    const auto bit = static_cast<std::uint8_t>(mask0[i] ^ mask1[i]);
    check(bit == expected[i], "independent frozen stable oracle differential");
    weight += bit;
  }
  check(weight == k, "exact K output bits");
  check(replay0 == (replay_material ? 1U : 0U) &&
            replay1 == (replay_material ? 1U : 0U),
        "replay materials rejected without second mask");
  for (const auto pair : std::array<std::pair<std::size_t,std::size_t>,8>{{
           {0,1},{2,3},{4,5},{6,7},{8,9},{10,11},{12,13},{18,19}}})
    check(metrics0[pair.first] == metrics1[pair.second] &&
              metrics1[pair.first] == metrics0[pair.second],
          "bidirectional byte counters reconcile");
  check(metrics0[14] == metrics1[14] && metrics0[15] == metrics1[15] &&
            metrics0[16] == metrics1[16] && metrics0[17] == metrics1[17] &&
            metrics0[20] == metrics1[20] && metrics0[21] == metrics1[21],
        "both parties agree on public work counts");
  check(metrics0[14] > 0 && metrics0[15] == 2U * metrics0[14] &&
            metrics0[16] == 2U * padded_n && metrics0[17] <= 9 &&
            metrics0[20] <= 4 &&
            metrics0[21] == 10U + 2U * metrics0[17] + metrics0[20],
        "DCF and causal online round accounting");
  verify_sample_seed_wire_order(party_store(store_directory, 0),
                                static_cast<std::uint32_t>(metrics0[20]));
  std::cout << "PASS T/P0/P1 N=" << n << " K=" << k
            << " comparisons=" << metrics0[14]
            << " compare_layers=" << metrics0[17]
            << " sample_rounds=" << metrics0[20]
            << " online_exchange_rounds=" << metrics0[21]
            << " P0_sent_bytes[binding,input,forward,select,schedule,membership,inverse,status]="
            << metrics0[0] << ',' << metrics0[2] << ',' << metrics0[4] << ','
            << metrics0[6] << ',' << metrics0[18] << ',' << metrics0[8] << ','
            << metrics0[10] << ',' << metrics0[12]
            << " P0_received_bytes=" << metrics0[1] << ',' << metrics0[3]
            << ',' << metrics0[5] << ',' << metrics0[7] << ',' << metrics0[19]
            << ',' << metrics0[9] << ',' << metrics0[11] << ',' << metrics0[13] << '\n';
  std::cout << "CASE_JSON {\"n\":" << n << ",\"padded_n\":"
            << padded_n << ",\"k\":" << k
            << ",\"case_id\":" << serial
            << ",\"session_hex\":\"" << hex_u64(session)
            << "\",\"fingerprint_hex\":\"" << hex_u64(fingerprint)
            << "\",\"material_id_hex\":\"" << hex_u64(material_id) << "\""
            << ",\"input_share_seed_hex\":\""
            << hex_u64(UINT64_C(0x5241575348415245) ^ serial) << "\""
            << ",\"streamed\":" << (streamed ? "true" : "false")
            << ",\"staged_store_bytes\":" << staged_store.total
            << ",\"party_store_bytes_p0\":" << staged_store.party_bytes[0]
            << ",\"party_store_bytes_p1\":" << staged_store.party_bytes[1]
            << ",\"manifest_bytes_p0\":" << staged_store.manifest_bytes[0]
            << ",\"manifest_bytes_p1\":" << staged_store.manifest_bytes[1]
            << ",\"segments_p0\":" << staged_store.segment_count[0]
            << ",\"segments_p1\":" << staged_store.segment_count[1]
            << ",\"package_bytes_p0\":" << package_bytes_p0
            << ",\"package_bytes_p1\":" << package_bytes_p1
            << ",\"comparison_calls\":" << metrics0[14]
            << ",\"active_compare_layers\":" << metrics0[17]
            << ",\"active_sample_rounds\":" << metrics0[20]
            << ",\"sample_schedule_sent_p0_bytes\":" << metrics0[18]
            << ",\"sample_schedule_received_p0_bytes\":" << metrics0[19]
            << ",\"sample_source\":\"P0_getrandom_32B_public_seed_per_active_iteration_v3\""
            << ",\"online_exchanges\":" << metrics0[21]
            << ",\"select_bytes_each_direction\":" << metrics0[6]
            << ",\"dcf_abi\":\""
            << (g_dcf_abi == ProtocolIDcfAbi::LegacyM2UcV1
                    ? "M2UCv1" : "M2B1_BCG21_FIG1_AES128_V1")
            << "\""
            << ",\"score_share_abi\":"
            << static_cast<unsigned>(g_score_share_abi)
            << ",\"result\":\"ORACLE_PASS\"}\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    std::cout.setf(std::ios::unitbuf);
    if (::geteuid() != 0) {
      std::cout << "SKIP: Select5 party-isolation E2E requires a trusted "
                   "supervisor to launch distinct P0/P1 UIDs\n";
      return 77;
    }
    check_resource_preflights();
    if (argc == 2 && std::string(argv[1]) == "--bcg21-smoke") {
      g_dcf_abi = ProtocolIDcfAbi::Bcg21Fig1Aes128V1;
      run_case(1, 1, 101);
      run_case(2, 1, 102);
      run_case(2, 2, 103);
      run_case(128, 1, 104, false, true, false, false, false, false, true);
      run_case(128, 64, 105, false, true, false, false, false, false, true);
      run_case(128, 128, 106, false, true, false, false, false, false, true);
      run_case(256, 1, 107, false, true, false, false, false, false, true);
      run_case(256, 2, 108, false, true, false, false, false, false, true);
      run_case(256, 8, 109, false, true, false, false, false, false, true);
      run_case(256, 128, 110, false, true, false, false, false, false, true);
      run_case(256, 255, 111, false, true, false, false, false, false, true);
      run_case(256, 256, 112, false, true, false, false, false, false, true);
      std::cout << "DCF_BCG21_FIG1_AES128_V1 Select5 T/P0/P1 E2E PASS\n";
      return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "--bcg21-faults") {
      g_dcf_abi = ProtocolIDcfAbi::Bcg21Fig1Aes128V1;
      run_case(7, 1, 201, true, false);  // altered monolithic M6B5/v3 package
      run_case(7, 1, 202, false, false, true);  // disconnect
      run_case(7, 1, 203, false, false, false, true);  // wrong K binding
      run_case(7, 1, 204, false, false, false, false, true);  // wrong TLS identity
      run_case(7, 1, 205, false, false, false, false, false, true);  // T exits early
      run_case(7, 1, 206, false, false, false, false, false, false,
               false, true);  // malformed seed
      return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "--score-share-v2-smoke") {
      g_dcf_abi = ProtocolIDcfAbi::Bcg21Fig1Aes128V1;
      g_score_share_abi = ProtocolIScoreShareAbi::S15InducedV1;
      run_case(7, 3, 500);  // Frozen fixture and K for v1/v2 oracle comparison.
      g_score_share_abi =
          ProtocolIScoreShareAbi::FullKeyZeroSumRefreshV2;
      run_case(7, 3, 501);
      run_case(7, 3, 502);
      run_case(7, 1, 504);
      run_case(7, 7, 505);
      run_case(128, 64, 507);  // M6B5/v4 monolithic path.
      run_case(256, 128, 503, false, true, false, false, false, false, true);
      return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "--score-share-v2-acceptance") {
      g_dcf_abi = ProtocolIDcfAbi::Bcg21Fig1Aes128V1;
      g_score_share_abi =
          ProtocolIScoreShareAbi::FullKeyZeroSumRefreshV2;
      run_case(1, 1, 701);  // singleton and padded dummy.
      run_case(2, 1, 702);
      run_case(2, 2, 703);  // all K values for the smallest non-singleton.
      run_case(7, 1, 704);  // repeated values, signed boundaries, padding.
      run_case(7, 7, 705);
      run_case(128, 1, 706);
      run_case(128, 64, 707);
      run_case(128, 128, 708);
      run_case(256, 128, 709, false, true, false, false, false, false, true);

      run_case(7, 1, 711, true, false);  // package tampering.
      run_case(7, 1, 712, false, false, true);  // peer disconnect.
      run_case(7, 1, 713, false, false, false, true);  // wrong K binding.
      run_case(7, 1, 714, false, false, false, false, true);  // wrong TLS identity.
      run_case(7, 1, 715, false, false, false, false, false, true);  // T exits.
      run_case(7, 1, 716, false, false, false, false, false, false,
               false, true);  // malformed seed/refresh execution.
      for (std::uint8_t step = 1; step <= 6; ++step)
        run_case(7, 1, UINT64_C(720) + step, false, false, false,
                 false, false, false, false, false, step);
      std::cout << "F_SCORECONV_V2 ACCEPTANCE E2E PASS\n";
      return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "--bcg21-monolithic-n128") {
      g_dcf_abi = ProtocolIDcfAbi::Bcg21Fig1Aes128V1;
      run_case(128, 64, 301, false, true, false, false, false, false, false);
      return 0;
    }
    if (argc >= 3 && std::string(argv[1]) == "--n256-k") {
      const auto k = static_cast<std::uint32_t>(std::stoul(argv[2]));
      run_case(256, k, UINT64_C(1000) + k, false, true, false, false,
               false, false, true);
      return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "--n256-only") {
      run_case(256, 1, 30, false, true, false, false, false, false, true);
      run_case(256, 2, 31, false, true, false, false, false, false, true);
      run_case(256, 8, 32, false, true, false, false, false, false, true);
      run_case(256, 64, 33, false, true, false, false, false, false, true);
      run_case(256, 128, 34, false, true, false, false, false, false, true);
      run_case(256, 255, 35, false, true, false, false, false, false, true);
      run_case(256, 256, 36, false, true, false, false, false, false, true);
      run_case(256, 128, 37, false, true, false, false, false, false, true);
      return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "--fault-tampered") {
      run_case(7, 1, 5, true, false, false);
      return 0;
    }
    if (argc == 3 && std::string(argv[1]) == "--crash-step") {
      const auto step = static_cast<std::uint8_t>(std::stoul(argv[2]));
      check(step >= 1 && step <= 6, "crash step selector");
      run_case(7, 1, UINT64_C(50) + step, false, false, false,
               false, false, false, false, false, step);
      return 0;
    }
    run_case(1, 1, 0);
    run_case(7, 1, 1);
    run_case(7, 7, 2);
    run_case(128, 2, 3);
    run_case(128, 8, 4);
    run_case(128, 1, 8);
    run_case(128, 64, 9);
    run_case(128, 65, 10);
    run_case(128, 128, 11);
    run_case(128, 64, 12);  // same n/K, new per-iteration OS-random samples
    run_case(7, 1, 5, true, false, false);
    run_case(7, 1, 6, false, false, true);
    run_case(7, 1, 7, false, false, false, true);
    run_case(7, 1, 13, false, false, false, false, true);
    run_case(7, 1, 14, false, false, false, false, false, true);
    run_case(7, 1, 15, false, false, false, false, false, false, false, true);
    run_case(256, 1, 30, false, true, false, false, false, false, true);
    run_case(256, 2, 31, false, true, false, false, false, false, true);
    run_case(256, 8, 32, false, true, false, false, false, false, true);
    run_case(256, 64, 33, false, true, false, false, false, false, true);
    run_case(256, 128, 34, false, true, false, false, false, false, true);
    run_case(256, 255, 35, false, true, false, false, false, false, true);
    run_case(256, 256, 36, false, true, false, false, false, false, true);
    run_case(256, 128, 37, false, true, false, false, false, false, true);
    for (std::uint8_t step = 1; step <= 6; ++step)
      run_case(7, 1, UINT64_C(50) + step, false, false, false,
               false, false, false, false, false, step);
    std::cout << "PROJECT_DERIVED_SELECT_5ITER Protocol I process E2E PASS\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "PROJECT_DERIVED_SELECT_5ITER process E2E FAIL: "
              << error.what() << '\n';
    return 1;
  }
}
