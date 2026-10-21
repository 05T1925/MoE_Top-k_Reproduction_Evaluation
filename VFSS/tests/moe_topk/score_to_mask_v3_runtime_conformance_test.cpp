#ifndef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_GMW_EXPERIMENTAL
#error "This test must only be built with the explicit v3 experimental option."
#endif

#include <moe_topk/score_to_mask_v3_gmw.h>

#include <openssl/evp.h>
#include <openssl/pem.h>

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <unistd.h>

namespace {

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

template <typename F>
void must_reject(F&& function, const char* message) {
  bool rejected = false;
  try { function(); } catch (const std::exception&) { rejected = true; }
  require(rejected, message);
}

struct TempDirectory {
  std::string path;
  TempDirectory() {
    std::string pattern = (std::filesystem::temp_directory_path() /
                           "s32-v3-runtime-XXXXXX").string();
    std::vector<char> writable(pattern.begin(), pattern.end());
    writable.push_back('\0');
    auto* created = ::mkdtemp(writable.data());
    if (created == nullptr) throw std::runtime_error("mkdtemp failed");
    path = created;
  }
  ~TempDirectory() { std::error_code ec; std::filesystem::remove_all(path, ec); }
};

std::pair<std::string, std::string> create_ed25519_keypair(const std::string& dir) {
  std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> context(
      EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr), EVP_PKEY_CTX_free);
  if (!context || EVP_PKEY_keygen_init(context.get()) != 1)
    throw std::runtime_error("Ed25519 keygen initialization failed");
  EVP_PKEY* generated = nullptr;
  if (EVP_PKEY_keygen(context.get(), &generated) != 1)
    throw std::runtime_error("Ed25519 keygen failed");
  std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> key(generated, EVP_PKEY_free);
  const auto private_path = (std::filesystem::path(dir) / "dealer-private.pem").string();
  const auto public_path = (std::filesystem::path(dir) / "dealer-public.pem").string();
  FILE* private_file = std::fopen(private_path.c_str(), "wb");
  FILE* public_file = std::fopen(public_path.c_str(), "wb");
  if (private_file == nullptr || public_file == nullptr) {
    if (private_file) std::fclose(private_file);
    if (public_file) std::fclose(public_file);
    throw std::runtime_error("cannot create temporary signing key files");
  }
  const bool ok = PEM_write_PrivateKey(private_file, key.get(), nullptr, nullptr, 0,
                                      nullptr, nullptr) == 1 &&
                  PEM_write_PUBKEY(public_file, key.get()) == 1;
  std::fclose(private_file);
  std::fclose(public_file);
  if (!ok) throw std::runtime_error("cannot serialize signing key");
  return {private_path, public_path};
}

moe_topk::ScoreToMaskV3Session test_session() {
  moe_topk::ScoreToMaskV3Session session;
  session.session[0] = 0x31;
  session.invocation[0] = 0x32;
  session.n = 2;
  session.k = 1;
  for (std::size_t i = 0; i < 32; ++i) {
    session.party_identity_sha256[0][i] = static_cast<std::uint8_t>(0x10U + i);
    session.party_identity_sha256[1][i] = static_cast<std::uint8_t>(0x30U + i);
    session.recipient_rsa_public_sha256[0][i] = static_cast<std::uint8_t>(0x50U + i);
    session.recipient_rsa_public_sha256[1][i] = static_cast<std::uint8_t>(0x70U + i);
    session.escrow_identity_sha256[i] = static_cast<std::uint8_t>(0x90U + i);
  }
  return session;
}

void write_bytes(const std::string& path, const std::vector<std::uint8_t>& bytes) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
  if (!out) throw std::runtime_error("test package write failed");
}

std::vector<std::uint8_t> read_bytes(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void check_budget() {
  const auto n128 = moe_topk::score_to_mask_v3_gate_budget(128);
  const auto n256 = moe_topk::score_to_mask_v3_gate_budget(256);
  require(n128.padded_n == 128 && n128.gate_count == 1'289'280,
          "N=128 gate budget differs from S31");
  require(n256.padded_n == 256 && n256.gate_count == 5'281'664,
          "N=256 gate budget differs from S31");
  require(n128.package_bytes_per_party <= moe_topk::kScoreToMaskV3MaxPackageBytes &&
              n256.package_bytes_per_party <= moe_topk::kScoreToMaskV3MaxPackageBytes,
          "S31 package cap exceeded by formula");
  must_reject([] { (void)moe_topk::score_to_mask_v3_gate_budget(257); },
              "N=257 was accepted");
}

void check_material_and_leases() {
  TempDirectory temp;
  const auto keys = create_ed25519_keypair(temp.path);
  const auto p0_path = (std::filesystem::path(temp.path) / "p0.pkg").string();
  const auto p1_path = (std::filesystem::path(temp.path) / "p1.pkg").string();
  const auto escrow_context_path =
      (std::filesystem::path(temp.path) / "escrow.context").string();
  const auto session = test_session();
  moe_topk::score_to_mask_v3_dealer_issue_packages(session, p0_path, p1_path,
                                                   escrow_context_path, keys.first);
  auto p0_info = moe_topk::score_to_mask_v3_validate_package(p0_path, keys.second,
                                                              session, 0);
  auto p1_info = moe_topk::score_to_mask_v3_validate_package(p1_path, keys.second,
                                                              session, 1);
  require(p0_info.package_bytes < moe_topk::kScoreToMaskV3MaxPackageBytes &&
              p1_info.package_bytes < moe_topk::kScoreToMaskV3MaxPackageBytes,
          "test package exceeds package cap");
  require(p0_info.package_sha256 != p1_info.package_sha256,
          "party package hashes unexpectedly match");
  const auto escrow_context = moe_topk::score_to_mask_v3_validate_escrow_context(
      escrow_context_path, keys.second);
  require(escrow_context.party_package_sha256[0] == p0_info.package_sha256 &&
              escrow_context.party_package_sha256[1] == p1_info.package_sha256 &&
              escrow_context.package_set_sha256 != std::array<std::uint8_t, 32>{},
          "metadata context does not bind the two signed packages");
  require(std::filesystem::file_size(escrow_context_path) < 1024,
          "metadata context unexpectedly contains package-sized material");
  require(moe_topk::ScoreToMaskV3EscrowStateParameters{}.receipt_format_version == 1,
          "receipt/v2 unexpectedly became the default format");
  const auto p0_v2_path = (std::filesystem::path(temp.path) / "p0-v2.pkg").string();
  const auto p1_v2_path = (std::filesystem::path(temp.path) / "p1-v2.pkg").string();
  const auto context_v2_path =
      (std::filesystem::path(temp.path) / "escrow-v2.context").string();
  auto receipt_v2_state = moe_topk::ScoreToMaskV3EscrowStateParameters{};
  receipt_v2_state.receipt_format_version =
      moe_topk::kScoreToMaskV3ReceiptFormatV2;
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_RECEIPT_V2_EXPERIMENTAL
  moe_topk::score_to_mask_v3_dealer_issue_packages(
      session, p0_v2_path, p1_v2_path, context_v2_path, keys.first,
      receipt_v2_state);
  const auto context_v2 = moe_topk::score_to_mask_v3_validate_escrow_context(
      context_v2_path, keys.second);
  require(context_v2.state.receipt_format_version ==
              moe_topk::kScoreToMaskV3ReceiptFormatV2 &&
              context_v2.party_package_sha256[0] !=
                  context_v2.party_package_sha256[1],
          "signed context did not preserve explicit receipt/v2 selection");
#else
  must_reject([&] {
    moe_topk::score_to_mask_v3_dealer_issue_packages(
        session, p0_v2_path, p1_v2_path, context_v2_path, keys.first,
        receipt_v2_state);
  }, "receipt/v2 was accepted without its explicit build opt-in");
#endif
  auto unsupported_receipt_state = moe_topk::ScoreToMaskV3EscrowStateParameters{};
  unsupported_receipt_state.receipt_format_version = 3;
  must_reject([&] {
    moe_topk::score_to_mask_v3_dealer_issue_packages(
        session,
        (std::filesystem::path(temp.path) / "p0-v3.pkg").string(),
        (std::filesystem::path(temp.path) / "p1-v3.pkg").string(),
        (std::filesystem::path(temp.path) / "escrow-v3.context").string(),
        keys.first, unsupported_receipt_state);
  }, "unknown escrow receipt format version was accepted");
  must_reject([&] {
    (void)moe_topk::score_to_mask_v3_validate_escrow_context(p0_path, keys.second);
  }, "party package was accepted as an escrow context");
  must_reject([&] {
    (void)moe_topk::score_to_mask_v3_validate_package(
        escrow_context_path, keys.second, session, 0);
  }, "escrow context was accepted as a party package");
  const auto p0 = moe_topk::score_to_mask_v3_open_material(p0_path, keys.second,
                                                            session, 0);
  const auto p1 = moe_topk::score_to_mask_v3_open_material(p1_path, keys.second,
                                                            session, 1);
  for (std::uint64_t gate = 0; gate < p0.info.gate_count; ++gate) {
    const auto t0 = p0.triple(gate);
    const auto t1 = p1.triple(gate);
    const auto a = t0.a ^ t1.a;
    const auto b = t0.b ^ t1.b;
    const auto c = t0.c ^ t1.c;
    require(c == static_cast<std::uint8_t>(a & b),
            "Beaver triple share does not reconstruct a AND b");
  }
  const auto n = moe_topk::score_to_mask_v3_gate_budget(session.n).padded_n;
  for (std::uint32_t original = 0; original < n; ++original) {
    unsigned column_sum = 0;
    for (std::uint32_t row = 0; row < n; ++row)
      column_sum += p0.permutation_share(row, original) ^
                    p1.permutation_share(row, original);
    require(column_sum == 1, "permutation column is not one-hot");
  }
  for (std::uint32_t row = 0; row < n; ++row) {
    unsigned row_sum = 0;
    for (std::uint32_t original = 0; original < n; ++original)
      row_sum += p0.permutation_share(row, original) ^
                 p1.permutation_share(row, original);
    require(row_sum == 1, "permutation row is not one-hot");
  }
  must_reject([&] { (void)p0.triple(p0.info.gate_count); },
              "out-of-range gate id was accepted");
  must_reject([&] {
    (void)moe_topk::score_to_mask_v3_validate_package(p0_path, keys.second,
                                                       session, 1);
  }, "package party mismatch was accepted");
  auto wrong_session = session;
  wrong_session.invocation[1] = 1;
  must_reject([&] {
    (void)moe_topk::score_to_mask_v3_validate_package(p0_path, keys.second,
                                                       wrong_session, 0);
  }, "wrong invocation was accepted");
  auto wrong_identity = session;
  wrong_identity.party_identity_sha256[0][0] ^= 0x80U;
  must_reject([&] {
    (void)moe_topk::score_to_mask_v3_validate_package(p0_path, keys.second,
                                                       wrong_identity, 0);
  }, "wrong party identity was accepted");
  const auto bad_signature_path = (std::filesystem::path(temp.path) / "bad-signature.pkg").string();
  auto tampered = read_bytes(p0_path);
  tampered[tampered.size() / 2U] ^= 1U;
  write_bytes(bad_signature_path, tampered);
  must_reject([&] {
    (void)moe_topk::score_to_mask_v3_validate_package(bad_signature_path,
                                                       keys.second, session, 0);
  }, "tampered package was accepted");
  const auto truncated_path = (std::filesystem::path(temp.path) / "truncated.pkg").string();
  tampered.pop_back();
  write_bytes(truncated_path, tampered);
  must_reject([&] {
    (void)moe_topk::score_to_mask_v3_validate_package(truncated_path,
                                                       keys.second, session, 0);
  }, "truncated package was accepted");
  const auto lease_dir = (std::filesystem::path(temp.path) / "lease-p0").string();
  moe_topk::score_to_mask_v3_consume_package_lease(p0_path, lease_dir, p0_info);
  must_reject([&] {
    moe_topk::score_to_mask_v3_consume_package_lease(p0_path, lease_dir, p0_info);
  }, "duplicate package lease was accepted");
}

}  // namespace

int main() {
  try {
    check_budget();
    check_material_and_leases();
    std::cout << "S32_V3_RUNTIME_CONFORMANCE {\"package_signature\":\"PASS\","
                 "\"triple_shares\":\"PASS\",\"permutation_shares\":\"PASS\","
                 "\"one_shot_lease\":\"PASS\",\"package_faults\":\"PASS\","
                 "\"N128_formula\":1289280,\"N256_formula\":5281664}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "S32_V3_RUNTIME_CONFORMANCE_FAIL " << error.what() << '\n';
    return 1;
  }
}
