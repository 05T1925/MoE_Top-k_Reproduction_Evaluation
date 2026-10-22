#ifndef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_GMW_EXPERIMENTAL
#error "This test requires the explicit v3 experimental option."
#endif
#ifndef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL
#error "This test requires the explicit F_T/v2 bounded option."
#endif

#include <moe_topk/score_to_mask_v3_gmw.h>
#include "score_to_mask_v3_ft_v2_test_support.h"
#include "moe_topk/score_to_mask_v3_bounded_sampler.h"

#include <openssl/evp.h>
#include <openssl/pem.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
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
                           "s45-ft-v2-XXXXXX").string();
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

moe_topk::ScoreToMaskV3Session test_session(std::uint32_t n,
                                             std::uint8_t invocation_tag) {
  moe_topk::ScoreToMaskV3Session session;
  session.session[0] = 0x51;
  session.invocation[0] = invocation_tag;
  session.n = n;
  session.k = n / 2U;
  if (session.k == 0) session.k = 1;
  for (std::size_t i = 0; i < 32; ++i) {
    session.party_identity_sha256[0][i] = static_cast<std::uint8_t>(0x10U + i);
    session.party_identity_sha256[1][i] = static_cast<std::uint8_t>(0x30U + i);
    session.recipient_rsa_public_sha256[0][i] = static_cast<std::uint8_t>(0x50U + i);
    session.recipient_rsa_public_sha256[1][i] = static_cast<std::uint8_t>(0x70U + i);
    session.escrow_identity_sha256[i] = static_cast<std::uint8_t>(0x90U + i);
  }
  return session;
}

struct ByteTape {
  std::vector<std::uint8_t> bytes;
  std::size_t offset = 0;
  std::uint32_t next_word() {
    if (offset + sizeof(std::uint32_t) > bytes.size())
      throw std::runtime_error("controlled byte tape exhausted");
    std::uint32_t value = 0;
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    offset += sizeof(value);
    return value;
  }
  void append_word(std::uint32_t value) {
    const auto* first = reinterpret_cast<const std::uint8_t*>(&value);
    bytes.insert(bytes.end(), first, first + sizeof(value));
  }
};

void check_sampler_boundaries() {
  using moe_topk::score_to_mask_v3_detail::bounded_retry_limit;
  using moe_topk::score_to_mask_v3_detail::next_uniform_bounded;
  require(bounded_retry_limit(128, 1) == 0,
          "m=1 must need no draw or retry");
  require(bounded_retry_limit(128, 2) == 6 &&
              bounded_retry_limit(128, 256) == 6,
          "lambda=128 retry formula changed on the supported range");
  for (std::uint32_t m = 2; m <= 256; ++m) {
    const auto r = bounded_retry_limit(128, m);
    require(r == 6, "unexpected lambda=128 retry limit for m<=256");
    require((std::uint64_t{1} << (24U * r - 128U)) >= m - 1U,
            "integer retry formula does not imply the union bound");
    for (std::uint32_t i = 2; i <= m; ++i) {
      const std::uint64_t remainder = (std::uint64_t{1} << 32U) % i;
      require(remainder < 256U,
              "rejection numerator is not strictly below 256");
    }
  }
  const long double delta256 = 255.0L * std::ldexp(1.0L, -144);
  require(delta256 < std::ldexp(1.0L, -136) &&
              delta256 <= std::ldexp(1.0L, -128),
          "N=256 conservative issuance-abort bound is invalid");

  for (const auto n : {128U, 256U}) {
    const auto budget = moe_topk::score_to_mask_v3_gate_budget(n);
    const auto m = budget.padded_n;
    const auto retry_limit = bounded_retry_limit(128, m);
    const std::uint64_t q_base = budget.gate_count +
        (static_cast<std::uint64_t>(m) * m + 7U) / 8U;
    const std::uint64_t q_max = q_base + 4ULL * retry_limit * (m - 1U);
    const std::uint64_t rand_calls_max =
        (budget.gate_count + (1U << 20U) - 1U) / (1U << 20U) + 1ULL +
        static_cast<std::uint64_t>(retry_limit) * (m - 1U);
    if (n == 128U)
      require(q_base == 1'291'328ULL && q_max == 1'294'376ULL &&
                  rand_calls_max == 765ULL,
              "N=128 maximum RNG byte/call bound changed");
    else
      require(q_base == 5'289'856ULL && q_max == 5'295'976ULL &&
                  rand_calls_max == 1'537ULL,
              "N=256 maximum RNG byte/call bound changed");
  }

  std::uint32_t result = 99;
  std::uint32_t used = 99;
  ByteTape one;
  require(next_uniform_bounded(1, 0, [&] { return one.next_word(); }, result, &used) &&
              result == 0 && used == 0 && one.offset == 0,
          "m=1 consumed random bytes");

  ByteTape power_of_two;
  power_of_two.append_word(std::numeric_limits<std::uint32_t>::max());
  require(next_uniform_bounded(256, 6,
              [&] { return power_of_two.next_word(); }, result, &used) &&
              result == 255 && used == 1 && power_of_two.offset == 4,
          "power-of-two range did not accept the upper boundary");

  ByteTape two;
  two.append_word(std::numeric_limits<std::uint32_t>::max());
  require(next_uniform_bounded(2, 6,
              [&] { return two.next_word(); }, result, &used) &&
              result == 1 && used == 1 && two.offset == 4,
          "m=2 did not accept one bounded word");

  ByteTape non_power_of_two;
  non_power_of_two.append_word(0);  // threshold 1: reject
  non_power_of_two.append_word(1);  // first accepted value
  require(next_uniform_bounded(255, 6,
              [&] { return non_power_of_two.next_word(); }, result, &used) &&
              result == 1 && used == 2 && non_power_of_two.offset == 8,
          "forced rejection then acceptance was not bounded/correct");

  ByteTape m3;
  m3.append_word(0);
  m3.append_word(1);
  require(next_uniform_bounded(3, 6, [&] { return m3.next_word(); }, result, &used) &&
              result == 1 && used == 2,
          "non-power-of-two m=3 boundary failed");

  ByteTape exhaustion;
  for (int i = 0; i < 6; ++i) exhaustion.append_word(0);
  require(!next_uniform_bounded(255, 6,
              [&] { return exhaustion.next_word(); }, result, &used) &&
              used == 6 && exhaustion.offset == 24,
          "six consecutive rejections did not stop at the cap");
}

std::vector<std::uint8_t> read_bytes(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void write_bytes(const std::string& path, const std::vector<std::uint8_t>& bytes) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
  if (!out) throw std::runtime_error("cannot write tampered package");
}

void check_versioned_issuance_and_abort() {
  TempDirectory temp;
  const auto keys = create_ed25519_keypair(temp.path);
  const auto session = test_session(256, 0x52);
  const auto successful_dir = std::filesystem::path(temp.path) / "success";
  const auto successful_receipt_v2_dir =
      std::filesystem::path(temp.path) / "success-receipt-v2";
  const auto failed_dir = std::filesystem::path(temp.path) / "aborted";
  std::filesystem::create_directory(successful_dir);
  std::filesystem::create_directory(successful_receipt_v2_dir);
  std::filesystem::create_directory(failed_dir);

  const auto path = [](const std::filesystem::path& dir, const char* name) {
    return (dir / name).string();
  };
  const auto p0 = path(successful_dir, "p0.pkg");
  const auto p1 = path(successful_dir, "p1.pkg");
  const auto context_path = path(successful_dir, "escrow.context");
  std::size_t success_bytes = 0;
  std::size_t success_calls = 0;
  std::size_t success_max_request = 0;
  moe_topk::test_only::score_to_mask_v3_dealer_issue_packages_ft_v2_with_test_random(
      session, p0, p1, context_path, keys.first, temp.path, {}, true,
      &success_bytes, &success_calls, &success_max_request);
  const auto budget = moe_topk::score_to_mask_v3_gate_budget(session.n);
  const std::size_t q_base = static_cast<std::size_t>(budget.gate_count) +
      (static_cast<std::uint64_t>(budget.padded_n) * budget.padded_n + 7U) / 8U;
  require(success_bytes == q_base + 4U * (budget.padded_n),
          "forced-rejection success did not stay within the sampler draw count");
  require(success_calls == 6U + 1U + budget.padded_n,
          "forced-rejection success exceeded its call bound");
  require(success_bytes <= q_base + 4ULL * 6U * (budget.padded_n - 1U) &&
              success_calls <= 6U + 1U + 6U * (budget.padded_n - 1U) &&
              success_max_request <= (1U << 20U),
          "successful deterministic run exceeded a whole-function RNG request bound");
  const auto info0 = moe_topk::score_to_mask_v3_validate_package(
      p0, keys.second, session, 0);
  const auto info1 = moe_topk::score_to_mask_v3_validate_package(
      p1, keys.second, session, 1);
  require(info0.package_format_version ==
              moe_topk::kScoreToMaskV3PackageFormatVersionFtV2 &&
              info1.ft_version == moe_topk::kScoreToMaskV3FtVersionV2 &&
              info0.sampler_version ==
                  moe_topk::kScoreToMaskV3SamplerVersionBoundedV1 &&
              info0.sampler_security_parameter_bits == 128 &&
              info0.sampler_retry_limit == 6,
          "F_T/sampler profile was not signed into the package header");
  auto context = moe_topk::score_to_mask_v3_validate_escrow_context(
      context_path, keys.second);
  require(context.state.receipt_format_version ==
              moe_topk::kScoreToMaskV3ReceiptFormatV1 &&
              context.party_package_sha256[0] == info0.package_sha256 &&
              context.party_package_sha256[1] == info1.package_sha256,
          "F_T/v2 package digests or independent receipt/v1 field were not bound");

  const auto burn_count_after_success = [&] {
    std::size_t count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(temp.path))
      if (entry.path().filename().string().rfind("ft-v2-", 0) == 0) ++count;
    return count;
  }();
  require(burn_count_after_success == 1,
          "successful invocation did not create exactly one issuance record");
  const auto duplicate_dir = std::filesystem::path(temp.path) / "duplicate-success";
  std::filesystem::create_directory(duplicate_dir);
  std::size_t duplicate_bytes = 999;
  std::size_t duplicate_calls = 999;
  std::size_t duplicate_max_request = 999;
  bool duplicate_rejected = false;
  try {
    moe_topk::test_only::score_to_mask_v3_dealer_issue_packages_ft_v2_with_test_random(
        session, path(duplicate_dir, "p0.pkg"), path(duplicate_dir, "p1.pkg"),
        path(duplicate_dir, "escrow.context"), keys.first, temp.path, {}, true,
        &duplicate_bytes, &duplicate_calls, &duplicate_max_request);
  } catch (const std::exception& error) {
    duplicate_rejected =
        std::string(error.what()).find("already issued or burned") !=
        std::string::npos;
  }
  require(duplicate_rejected && duplicate_bytes == 0 && duplicate_calls == 0 &&
              duplicate_max_request == 0 &&
              !std::filesystem::exists(path(duplicate_dir, "p0.pkg")) &&
              !std::filesystem::exists(path(duplicate_dir, "p1.pkg")) &&
              !std::filesystem::exists(path(duplicate_dir, "escrow.context")),
          "same successful session/invocation was issued to new paths");

  auto tampered = read_bytes(p0);
  constexpr std::size_t kProfileOffset = 8U + 4U + 4U + 4U + 16U + 16U +
      4U + 4U + 160U + 1U;
  require(tampered.size() > kProfileOffset + 3U,
          "F_T/v2 profile header missing");
  tampered[kProfileOffset] = moe_topk::kScoreToMaskV3FtVersionV1;
  const auto bad_profile = path(successful_dir, "bad-ft-version.pkg");
  write_bytes(bad_profile, tampered);
  must_reject([&] {
    (void)moe_topk::score_to_mask_v3_validate_package(
        bad_profile, keys.second, session, 0);
  }, "v5 package with mismatched F_T version was accepted");

  const auto lease = path(successful_dir, "lease-p0");
  moe_topk::score_to_mask_v3_consume_package_lease(p0, lease, info0);
  must_reject([&] {
    moe_topk::score_to_mask_v3_consume_package_lease(p0, lease, info0);
  }, "F_T/v2 package lease was reused");

#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_RECEIPT_V2_EXPERIMENTAL
  const auto p0_receipt_v2 = path(successful_receipt_v2_dir, "p0.pkg");
  const auto p1_receipt_v2 = path(successful_receipt_v2_dir, "p1.pkg");
  const auto context_receipt_v2 = path(successful_receipt_v2_dir, "escrow.context");
  auto receipt_v2 = moe_topk::ScoreToMaskV3EscrowStateParameters{};
  receipt_v2.receipt_format_version = moe_topk::kScoreToMaskV3ReceiptFormatV2;
  const auto receipt_v2_session = test_session(256, 0x54);
  std::size_t repeated_bytes = 0;
  std::size_t repeated_calls = 0;
  std::size_t repeated_max_request = 0;
  moe_topk::test_only::score_to_mask_v3_dealer_issue_packages_ft_v2_with_test_random(
      receipt_v2_session, p0_receipt_v2, p1_receipt_v2, context_receipt_v2,
      keys.first, temp.path, receipt_v2, true, &repeated_bytes, &repeated_calls,
      &repeated_max_request);
  const auto repeated_info = moe_topk::score_to_mask_v3_validate_package(
      p0_receipt_v2, keys.second, receipt_v2_session, 0);
  const auto repeated_context = moe_topk::score_to_mask_v3_validate_escrow_context(
      context_receipt_v2, keys.second);
  require(repeated_info.ft_version == info0.ft_version &&
              repeated_info.sampler_version == info0.sampler_version &&
              repeated_context.state.receipt_format_version ==
                  moe_topk::kScoreToMaskV3ReceiptFormatV2 &&
              repeated_context.party_package_sha256[0] ==
                  repeated_info.package_sha256,
          "F_T/v2 and receipt/v2 version fields were conflated");
#endif

  const auto count_burn_records = [](const std::string& registry) {
    std::size_t count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(registry)) {
      if (entry.path().filename().string().rfind("ft-v2-", 0) == 0) ++count;
    }
    return count;
  };
  const auto burn_count_before_failure = count_burn_records(temp.path);
  const auto failed_p0 = path(failed_dir, "p0.pkg");
  const auto failed_p1 = path(failed_dir, "p1.pkg");
  const auto failed_context = path(failed_dir, "escrow.context");
  const auto failed_lease = path(failed_dir, "lease-p0");
  const auto failed_session = test_session(256, 0x55);
  std::size_t failed_bytes = 0;
  std::size_t failed_calls = 0;
  std::size_t failed_max_request = 0;
  bool issuance_aborted = false;
  try {
    moe_topk::test_only::score_to_mask_v3_dealer_issue_packages_ft_v2_with_test_random(
        failed_session, failed_p0, failed_p1, failed_context, keys.first,
        temp.path, {}, false,
        &failed_bytes, &failed_calls, &failed_max_request);
  } catch (const std::exception& error) {
    issuance_aborted =
        std::string(error.what()).find("ISSUANCE_ABORT") != std::string::npos;
  }
  const std::size_t failed_draws = 1U + 6U;
  require(issuance_aborted, "bounded exhaustion did not return ISSUANCE_ABORT");
  require(failed_bytes == q_base + 4U * failed_draws &&
              failed_calls == 6U + 1U + failed_draws &&
              failed_bytes <= q_base + 4ULL * 6U * (budget.padded_n - 1U) &&
              failed_calls <= 6U + 1U + 6U * (budget.padded_n - 1U) &&
              failed_max_request <= (1U << 20U),
          "issuance abort did not stop exactly at its deterministic bound");
  require(!std::filesystem::exists(failed_p0) &&
              !std::filesystem::exists(failed_p1) &&
              !std::filesystem::exists(failed_context) &&
              !std::filesystem::exists(failed_lease),
          "issuance abort left package, context, or lease artifacts");

  const auto burn_records_after_failure = count_burn_records(temp.path);
  require(burn_records_after_failure == burn_count_before_failure + 1,
          "ISSUANCE_ABORT did not persist one-shot invocation record");
  const auto retry_dir = std::filesystem::path(temp.path) / "retry-after-abort";
  std::filesystem::create_directory(retry_dir);
  std::size_t retry_bytes = 999;
  std::size_t retry_calls = 999;
  bool retry_rejected = false;
  std::size_t retry_max_request = 999;
  try {
    moe_topk::test_only::score_to_mask_v3_dealer_issue_packages_ft_v2_with_test_random(
        failed_session, path(retry_dir, "p0.pkg"), path(retry_dir, "p1.pkg"),
        path(retry_dir, "escrow.context"), keys.first, temp.path, {}, true,
        &retry_bytes, &retry_calls, &retry_max_request);
  } catch (const std::exception& error) {
    retry_rejected = std::string(error.what()).find("already issued or burned") !=
                     std::string::npos;
  }
  require(retry_rejected && retry_bytes == 0 && retry_calls == 0 &&
              retry_max_request == 0 &&
              count_burn_records(temp.path) == burn_records_after_failure &&
              !std::filesystem::exists(path(retry_dir, "p0.pkg")) &&
              !std::filesystem::exists(path(retry_dir, "p1.pkg")) &&
              !std::filesystem::exists(path(retry_dir, "escrow.context")),
          "same session/invocation was reissued after its sampler abort");

  const auto v1_dir = std::filesystem::path(temp.path) / "legacy-v1";
  std::filesystem::create_directory(v1_dir);
  const auto v1_p0 = path(v1_dir, "p0.pkg");
  const auto v1_p1 = path(v1_dir, "p1.pkg");
  const auto v1_context_path = path(v1_dir, "escrow.context");
  auto v1_session = test_session(2, 0x53);
  moe_topk::score_to_mask_v3_dealer_issue_packages(
      v1_session, v1_p0, v1_p1, v1_context_path, keys.first);
  const auto v1_info = moe_topk::score_to_mask_v3_validate_package(
      v1_p0, keys.second, v1_session, 0);
  require(v1_info.package_format_version ==
              moe_topk::kScoreToMaskV3PackageFormatVersion &&
              v1_info.ft_version == moe_topk::kScoreToMaskV3FtVersionV1 &&
              v1_info.sampler_version ==
                  moe_topk::kScoreToMaskV3SamplerVersionUnboundedV1,
          "legacy API silently switched to F_T/v2");
}

}  // namespace

int main() {
  try {
    check_sampler_boundaries();
    check_versioned_issuance_and_abort();
    std::cout << "S45_FT_V2_BOUNDED_CONFORMANCE PASS\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "S45_FT_V2_BOUNDED_CONFORMANCE_FAIL " << error.what() << '\n';
    return 1;
  }
}
