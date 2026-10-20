#include <moe_topk/protocol_i_project_derived_select5.h>

#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using namespace moe_topk;

namespace {

void check(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

struct StoreCleanup {
  std::string path;
  ~StoreCleanup() {
    std::error_code ignored;
    std::filesystem::remove_all(path, ignored);
  }
};

std::string stem(std::uint64_t session, std::uint64_t fingerprint,
                 std::uint64_t material_id) {
  char buffer[96]{};
  const int count = std::snprintf(buffer, sizeof(buffer),
      "m6b5_%016llx_%016llx_%016llx",
      static_cast<unsigned long long>(session),
      static_cast<unsigned long long>(fingerprint),
      static_cast<unsigned long long>(material_id));
  check(count > 0 && static_cast<std::size_t>(count) < sizeof(buffer),
        "test store stem");
  return buffer;
}

std::string package_path(const std::string& dir, std::uint64_t session,
                         std::uint64_t fingerprint, std::uint64_t material_id,
                         unsigned party) {
  return dir + "/" + stem(session, fingerprint, material_id) + "_p" +
         std::to_string(party) + ".pkg";
}

std::vector<std::uint8_t> read_ready_marker(const std::string& dir,
    std::uint64_t session, std::uint64_t fingerprint,
    std::uint64_t material_id) {
  const auto path = dir + "/" + stem(session, fingerprint, material_id) +
                    ".ready";
  std::ifstream input(path, std::ios::binary);
  check(input.good(), "open party ready marker");
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input),
                                   std::istreambuf_iterator<char>());
}

ProtocolIProjectDerivedSelect5DealerConfig dealer_config(
    std::uint64_t session, std::uint64_t material_id, std::uint32_t k = 2) {
  return {session, UINT64_C(0x5588aacc), material_id, 4, k, 1000};
}

ProtocolIProjectDerivedSelect5DealerOutput make_packages(
    const ProtocolIProjectDerivedSelect5DealerConfig& config) {
  return protocol_i_project_derived_select5_dealer_generate(config);
}

void expect_rejected(const std::function<void()>& operation,
                     const char* message) {
  bool rejected = false;
  try { operation(); } catch (const std::exception&) { rejected = true; }
  check(rejected, message);
}

int wait_child(pid_t child) {
  int status = 0;
  while (::waitpid(child, &status, 0) < 0) {
    if (errno == EINTR) continue;
    throw std::runtime_error("material store waitpid");
  }
  return WIFEXITED(status) ? WEXITSTATUS(status) : 128;
}

void stage(const std::string& dir,
           const ProtocolIProjectDerivedSelect5DealerConfig& config,
           const ProtocolIProjectDerivedSelect5DealerOutput& packages,
           bool publish = true) {
  const std::array<std::string, 2> party_dirs{{dir + "/p0", dir + "/p1"}};
  protocol_i_project_derived_select5_stage_material_packages(
      party_dirs, config, packages);
  if (publish)
    protocol_i_project_derived_select5_publish_material_ready(party_dirs,
                                                               config);
}

std::vector<std::uint8_t> claim(const std::string& root,
    const ProtocolIProjectDerivedSelect5DealerConfig& config,
    std::uint8_t party = 0) {
  const auto dir = root + (party == 0 ? "/p0" : "/p1");
  return protocol_i_project_derived_select5_claim_party_package(
      dir, config.session, config.fingerprint, config.material_id_base,
      config.logical_n, 4, config.k, 35, party);
}

}  // namespace

int main() {
  char path_template[] = "/tmp/m6b5-store-XXXXXX";
  char* path = ::mkdtemp(path_template);
  if (path == nullptr) throw std::runtime_error("mkdtemp");
  const std::string dir(path);
  StoreCleanup cleanup{dir};
  if (::chmod(dir.c_str(), 0700) != 0) throw std::runtime_error("chmod store");
  const std::array<std::string, 2> party_dirs{{dir + "/p0", dir + "/p1"}};
  for (const auto& party_dir : party_dirs) {
    if (::mkdir(party_dir.c_str(), 0700) != 0)
      throw std::runtime_error("mkdir party store");
  }
  try {
    // BCG21 M6B5/v3 carries only the two-round Select5 shuffle metadata.
    // Legacy M6B5/v2 keeps its existing M2CP full edge-key pool unchanged.
    auto legacy_schema_config = dealer_config(100, 500);
    const auto legacy_schema = make_packages(legacy_schema_config);
    auto legacy_p0 = protocol_i_project_derived_select5_deserialize_party_material(
        legacy_schema.party0_package, 0);
    check(!legacy_p0.forward_shuffle.edge_materials.empty() &&
              !legacy_p0.inverse_shuffle.edge_materials.empty(),
          "legacy M6B5/v2 retains full M2CP comparison pools");

    auto bcg_schema_config = dealer_config(99, 499);
    bcg_schema_config.dcf_abi = ProtocolIDcfAbi::Bcg21Fig1Aes128V1;
    const auto bcg_schema = make_packages(bcg_schema_config);
    const auto bcg_p0 = protocol_i_project_derived_select5_deserialize_party_material(
        bcg_schema.party0_package, 0);
    check(bcg_p0.dcf_abi == ProtocolIDcfAbi::Bcg21Fig1Aes128V1 &&
              bcg_p0.forward_shuffle.edge_materials.empty() &&
              bcg_p0.inverse_shuffle.edge_materials.empty(),
          "BCG21 M6B5/v3 omits unused legacy M2CP comparison pools");
    const auto metadata_wire = protocol_i_parallel_shuffle_serialize_select5_metadata(
        bcg_p0.forward_shuffle);
    check(metadata_wire.size() >= 4 && metadata_wire[0] == 'M' &&
              metadata_wire[1] == '2' && metadata_wire[2] == 'C' &&
              metadata_wire[3] == 'M',
          "Select5 shuffle metadata has a distinct M2CM/v1 encoding");
    const auto decoded_metadata =
        protocol_i_parallel_shuffle_deserialize_select5_metadata(metadata_wire, 0);
    check(decoded_metadata.edge_materials.empty(),
          "M2CM/v1 decodes only metadata and no DCF edge pool");
    expect_rejected([&] {
      (void)protocol_i_parallel_shuffle_deserialize_material(metadata_wire, 0);
    }, "legacy M2CP parser rejects M2CM/v1 metadata encoding");

    auto same_binding_legacy = dealer_config(99, 499);
    auto legacy_with_same_binding = make_packages(same_binding_legacy);
    auto injected = protocol_i_project_derived_select5_deserialize_party_material(
        bcg_schema.party0_package, 0);
    auto legacy_with_edges =
        protocol_i_project_derived_select5_deserialize_party_material(
            legacy_with_same_binding.party0_package, 0);
    injected.forward_shuffle.edge_materials =
        std::move(legacy_with_edges.forward_shuffle.edge_materials);
    ProtocolIProjectDerivedSelect5RuntimeConfig injected_config;
    injected_config.session = same_binding_legacy.session;
    injected_config.fingerprint = same_binding_legacy.fingerprint;
    injected_config.logical_n = same_binding_legacy.logical_n;
    injected_config.padded_n = 4;
    injected_config.k = same_binding_legacy.k;
    injected_config.comparison_bits = 35;
    injected_config.party = 0;
    injected_config.dcf_abi = ProtocolIDcfAbi::Bcg21Fig1Aes128V1;
    injected_config.timeout_ms = 1000;
    injected_config.material_id_base = same_binding_legacy.material_id_base;
    injected_config.allow_plaintext_test_transport = true;
    ProtocolIProjectDerivedSelect5Fds invalid_fds;
    bool mixed_pool_rejected = false;
    try {
      (void)protocol_i_project_derived_select5_party_test_material(
          injected_config, std::move(injected), std::vector<std::uint32_t>(4, 0),
          invalid_fds);
    } catch (const std::invalid_argument& error) {
      mixed_pool_rejected = std::string(error.what()) ==
          "BCG21 Select5 metadata cannot carry legacy shuffle edge keys";
    }
    check(mixed_pool_rejected,
          "BCG21 direct material injection rejects legacy shuffle edge pool before I/O");

    // Staging is not sufficient authorization: only a post-T ready marker
    // makes a package claimable.
    auto unpublished = dealer_config(101, 501);
    auto unpublished_packages = make_packages(unpublished);
    stage(dir, unpublished, unpublished_packages, false);
    expect_rejected([&] { (void)claim(dir, unpublished); },
                    "unstaged T completion cannot release a package");
    protocol_i_project_derived_select5_publish_material_ready(party_dirs,
                                                               unpublished);
    const auto ready0 = read_ready_marker(party_dirs[0], unpublished.session,
        unpublished.fingerprint, unpublished.material_id_base);
    const auto ready1 = read_ready_marker(party_dirs[1], unpublished.session,
        unpublished.fingerprint, unpublished.material_id_base);
    check(ready0.size() == 88 && ready1.size() == 88 &&
              ready0[0] == 'M' && ready0[1] == '6' && ready0[2] == 'P' &&
              ready0[3] == '2' && ready1[0] == 'M' && ready1[1] == '6' &&
              ready1[2] == 'P' && ready1[3] == '2' &&
              ready0[45] == 0 && ready1[45] == 1,
          "M6P2 ready marker binds one recipient and one package digest");

    // Binding rejects shape/session confusion before creating a lease.
    expect_rejected([&] {
      (void)protocol_i_project_derived_select5_claim_party_package(
          party_dirs[0], unpublished.session, unpublished.fingerprint,
          unpublished.material_id_base, unpublished.logical_n, 4, 1, 35, 0);
    }, "wrong K rejected");
    expect_rejected([&] {
      (void)protocol_i_project_derived_select5_claim_party_package(
          party_dirs[0], unpublished.session + 1, unpublished.fingerprint,
          unpublished.material_id_base, unpublished.logical_n, 4,
          unpublished.k, 35, 0);
    }, "wrong session rejected");
    auto first_claim = claim(dir, unpublished);
    check(first_claim == unpublished_packages.party0_package,
          "durable package bytes preserved");
    expect_rejected([&] { (void)claim(dir, unpublished); },
                    "same party package cannot be claimed twice");

    // The refreshed full-key ABI gets its own package and one-shot binding
    // checks; these cases must not be inferred from the v1 store tests.
    auto refreshed = dealer_config(106, 506, 2);
    refreshed.dcf_abi = ProtocolIDcfAbi::Bcg21Fig1Aes128V1;
    refreshed.score_share_abi =
        ProtocolIScoreShareAbi::FullKeyZeroSumRefreshV2;
    auto refreshed_packages = make_packages(refreshed);
    stage(dir, refreshed, refreshed_packages);
    expect_rejected([&] {
      (void)protocol_i_project_derived_select5_claim_party_package(
          party_dirs[0], refreshed.session + 1, refreshed.fingerprint,
          refreshed.material_id_base, refreshed.logical_n, 4, refreshed.k,
          35, 0);
    }, "v2 wrong session rejected before lease");
    expect_rejected([&] {
      (void)protocol_i_project_derived_select5_claim_party_package(
          party_dirs[0], refreshed.session, refreshed.fingerprint,
          refreshed.material_id_base, refreshed.logical_n, 4, refreshed.k,
          35, 1);
    }, "v2 wrong party package rejected before lease");
    const auto refreshed_p0 =
        protocol_i_project_derived_select5_claim_party_package(
            party_dirs[0], refreshed.session, refreshed.fingerprint,
            refreshed.material_id_base, refreshed.logical_n, 4, refreshed.k,
            35, 0);
    check(refreshed_p0 == refreshed_packages.party0_package,
          "v2 package claim returns only its bound recipient bytes");
    expect_rejected([&] {
      (void)protocol_i_project_derived_select5_claim_party_package(
          party_dirs[0], refreshed.session, refreshed.fingerprint,
          refreshed.material_id_base, refreshed.logical_n, 4, refreshed.k,
          35, 0);
    }, "v2 package re-claim rejected after durable lease");

    // File tampering against the committed SHA-256 prevents release.
    auto tamper = dealer_config(102, 502);
    auto tamper_packages = make_packages(tamper);
    stage(dir, tamper, tamper_packages);
    const auto tamper_file = package_path(party_dirs[0], tamper.session,
        tamper.fingerprint, tamper.material_id_base, 0);
    check(::chmod(tamper_file.c_str(), 0600) == 0, "chmod package for tamper");
    const int fd = ::open(tamper_file.c_str(), O_WRONLY | O_CLOEXEC);
    check(fd >= 0, "open package for tamper");
    const std::uint8_t changed = 0xff;
    check(::pwrite(fd, &changed, 1, 64) == 1, "tamper package byte");
    ::close(fd);
    check(::chmod(tamper_file.c_str(), 0400) == 0, "restore package mode");
    expect_rejected([&] { (void)claim(dir, tamper); },
                    "tampered package rejected before lease");

    // A crash before claim leaves the package available; a crash after durable
    // lease creation burns the ID even if no consumer records successful use.
    auto crash_before = dealer_config(103, 503);
    auto before_packages = make_packages(crash_before);
    stage(dir, crash_before, before_packages);
    pid_t child = ::fork();
    check(child >= 0, "fork preclaim crash");
    if (child == 0) ::_exit(77);
    check(wait_child(child) == 77, "preclaim child crash observed");
    check(claim(dir, crash_before) == before_packages.party0_package,
          "preclaim crash leaves package claimable");

    auto crash_after = dealer_config(104, 504);
    auto after_packages = make_packages(crash_after);
    stage(dir, crash_after, after_packages);
    child = ::fork();
    check(child >= 0, "fork postclaim crash");
    if (child == 0) {
      try { (void)claim(dir, crash_after); ::_exit(78); }
      catch (...) { ::_exit(79); }
    }
    check(wait_child(child) == 78, "postclaim child exits after grant");
    expect_rejected([&] { (void)claim(dir, crash_after); },
                    "postclaim crash burns package ID");

    // O_EXCL lease creation is the cross-process concurrency serialization
    // point. Exactly one of two simultaneous claims may return key bytes.
    auto concurrent = dealer_config(105, 505);
    auto concurrent_packages = make_packages(concurrent);
    stage(dir, concurrent, concurrent_packages);
    int gate[2];
    check(::pipe(gate) == 0, "concurrent claim gate");
    std::array<pid_t, 2> children{};
    for (auto& process : children) {
      process = ::fork();
      check(process >= 0, "fork concurrent claim");
      if (process == 0) {
        ::close(gate[1]);
        char token = 0;
        if (::read(gate[0], &token, 1) != 1) ::_exit(81);
        try { (void)claim(dir, concurrent); ::_exit(0); }
        catch (...) { ::_exit(82); }
      }
    }
    ::close(gate[0]);
    const char start[2] = {'a', 'b'};
    check(::write(gate[1], start, sizeof(start)) == 2, "release claim race");
    ::close(gate[1]);
    const int result0 = wait_child(children[0]);
    const int result1 = wait_child(children[1]);
    check((result0 == 0 && result1 == 82) ||
              (result0 == 82 && result1 == 0),
          "exactly one concurrent lease winner");

    std::cout << "durable lease, binding, integrity, crash, and concurrent claim PASS\n";
    return 0;
  } catch (...) {
    std::cerr << "material store test failed; ephemeral key files will be removed: "
              << dir << '\n';
    throw;
  }
}
