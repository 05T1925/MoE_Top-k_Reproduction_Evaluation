#include <moe_topk/score_to_mask_v3_gmw.h>
#include "moe_topk/score_to_mask_v3_bounded_sampler.h"
#include "score_to_mask_v3_ft_v3_test_support.h"

#if !defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL) || \
    !defined(MOE_TOPK_SCORE_TO_MASK_V3_FT_V3_TESTING)
#error "F_T/v3 conformance is an explicitly opted-in TEST_ONLY target"
#endif

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

namespace {
using namespace moe_topk;
void require(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}
template <typename F> void must_reject(F&& f, const char* message) {
  bool rejected = false;
  try { f(); } catch (const std::exception&) { rejected = true; }
  require(rejected, message);
}

std::string temporary_directory() {
  std::string pattern = (std::filesystem::temp_directory_path() /
      "s47-ft-v3-conformance-XXXXXX").string();
  std::vector<char> buffer(pattern.begin(), pattern.end());
  buffer.push_back('\0');
  auto* path = ::mkdtemp(buffer.data());
  if (!path) throw std::runtime_error("mkdtemp failed");
  return path;
}
EVP_PKEY* generate_ed25519() {
  EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
  if (!ctx || EVP_PKEY_keygen_init(ctx) != 1) throw std::runtime_error("keygen init");
  EVP_PKEY* key = nullptr;
  if (EVP_PKEY_keygen(ctx, &key) != 1) throw std::runtime_error("Ed25519 keygen");
  EVP_PKEY_CTX_free(ctx);
  return key;
}
void save_key(const std::string& path, EVP_PKEY* key, bool private_key) {
  FILE* f = std::fopen(path.c_str(), "wb");
  if (!f) throw std::runtime_error("key output open");
  const int ok = private_key ? PEM_write_PrivateKey(f, key, nullptr, nullptr, 0, nullptr, nullptr)
                             : PEM_write_PUBKEY(f, key);
  std::fclose(f);
  if (ok != 1) throw std::runtime_error("key output write");
  ::chmod(path.c_str(), private_key ? 0600 : 0444);
}
ScoreToMaskV3Session test_session(std::uint32_t n, std::uint32_t k) {
  ScoreToMaskV3Session session;
  if (RAND_bytes(session.session.data(), 16) != 1 ||
      RAND_bytes(session.invocation.data(), 16) != 1)
    throw std::runtime_error("session RNG failed");
  session.session[0] |= 1U;
  session.invocation[0] |= 1U;
  session.n = n; session.k = k;
  for (std::size_t p = 0; p < 2; ++p) {
    session.party_identity_sha256[p].fill(static_cast<std::uint8_t>(p + 1U));
    session.recipient_rsa_public_sha256[p].fill(static_cast<std::uint8_t>(p + 3U));
  }
  session.escrow_identity_sha256.fill(9);
  return session;
}
std::vector<std::uint8_t> read_bytes(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void write_bytes(const std::string& path, const std::vector<std::uint8_t>& bytes) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
}
}  // namespace

int main() {
  using moe_topk::score_to_mask_v3_detail::bounded_retry_limit_ft_v3;
  using moe_topk::score_to_mask_v3_detail::next_uniform_bounded;
  try {
    require(bounded_retry_limit_ft_v3(128, 1) == 0, "m=1 retry count");
    require(bounded_retry_limit_ft_v3(128, 2) == 6, "m=2 retry count");
    require(bounded_retry_limit_ft_v3(128, 3) == 6, "m=3 retry count");
    require(bounded_retry_limit_ft_v3(128, 128) == 7, "m=128 retry count");
    require(bounded_retry_limit_ft_v3(128, 256) == 7, "m=256 retry count");
    require(bounded_retry_limit_ft_v3(128, 1024) == 7, "m=1024 retry count");
    require(bounded_retry_limit_ft_v3(128, 1025) == 0, "m>1024 rejected");

    std::uint32_t result = 99, used = 99;
    std::size_t calls = 0;
    require(next_uniform_bounded(1, 0, [&] { ++calls; return 0U; }, result, &used) &&
                result == 0 && used == 0 && calls == 0,
            "m=1 must consume no random word");
    std::vector<std::uint32_t> reject_then_accept{0U, 1U};
    std::size_t at = 0;
    require(next_uniform_bounded(3, 7, [&] { return reject_then_accept.at(at++); },
                                 result, &used) && result == 1 && used == 2,
            "reject-then-accept boundary");
    require(next_uniform_bounded(1024, 7, [] { return 0U; }, result, &used) &&
                result == 0 && used == 1,
            "m=1024 acceptance boundary");
    require(!next_uniform_bounded(1023, 7, [] { return 0U; }, result, &used) &&
                used == 7,
            "m=1023 exact retry exhaustion");
    require(next_uniform_bounded(2, 6, [] { return UINT32_MAX; }, result, &used) &&
                result == 1 && used == 1,
            "m=2 acceptance boundary");

    const auto budget = score_to_mask_v3_gate_budget_ft_v3(1000);
    require(budget.padded_n == 1024 && budget.index_bits == 10 &&
                budget.key_bits == 42 && budget.gate_count == 88626688ULL &&
                budget.package_bytes_per_party == 33366516ULL,
            "N=1000 F_T/v3 resource equation mismatch");
    must_reject([] { (void)score_to_mask_v3_gate_budget_ft_v3(1001); },
                "N>1000 F_T/v3 budget accepted");
    must_reject([] { (void)score_to_mask_v3_gate_budget(1000); },
                "legacy budget silently grew beyond N=256");
    const std::uint64_t qmax = budget.gate_count + 131072ULL +
        4ULL * 7ULL * (budget.padded_n - 1U);
    const std::uint64_t cmax = (budget.gate_count + (1U << 20) - 1U) /
        (1U << 20) + 1ULL + 7ULL * (budget.padded_n - 1U);
    require(qmax == 88786404ULL && cmax == 7247ULL,
            "N=1000 finite random query bound mismatch");

    const auto root = temporary_directory();
    const auto cleanup = [&] { std::error_code ec; std::filesystem::remove_all(root, ec); };
    try {
      const auto registry = (std::filesystem::path(root) / "registry").string();
      std::filesystem::create_directory(registry);
      ::chmod(registry.c_str(), 0700);
      EVP_PKEY* raw = generate_ed25519();
      const std::string private_key = (std::filesystem::path(root) / "dealer.key").string();
      const std::string public_key = (std::filesystem::path(root) / "dealer.pub").string();
      save_key(private_key, raw, true); save_key(public_key, raw, false);
      EVP_PKEY_free(raw);
      auto session = test_session(128, 64);
      ScoreToMaskV3EscrowStateParameters state;
      state.receipt_format_version = kScoreToMaskV3ReceiptFormatV2;
      const auto p0 = (std::filesystem::path(root) / "p0.pkg").string();
      const auto p1 = (std::filesystem::path(root) / "p1.pkg").string();
      const auto context = (std::filesystem::path(root) / "context.bin").string();
      score_to_mask_v3_dealer_issue_packages_ft_v3_bounded(
          session, p0, p1, context, private_key, registry, state);
      const auto info = score_to_mask_v3_validate_package(p0, public_key, session, 0);
      require(info.ft_version == 3 && info.sampler_version == 2 &&
                  info.sampler_security_parameter_bits == 128 &&
                  info.sampler_retry_limit == 7 &&
                  info.package_format_version == 6,
              "F_T/v3 signed package tuple mismatch");
      const auto ctx = score_to_mask_v3_validate_escrow_context(context, public_key);
      require(ctx.format_version == 2 && ctx.ft_version == 3 &&
                  ctx.sampler_version == 2 && ctx.sampler_retry_limit == 7 &&
                  ctx.package_format_version == 6 &&
                  ctx.state.receipt_format_version == 2 &&
                  ctx.state.max_ciphertext_bytes == 2U * 1024U * 1024U,
              "F_T/v3 context profile or receipt version mismatch");

      auto small_session = test_session(1, 1);
      const auto small_p0 = (std::filesystem::path(root) / "small-p0.pkg").string();
      const auto small_p1 = (std::filesystem::path(root) / "small-p1.pkg").string();
      const auto small_context = (std::filesystem::path(root) / "small.context").string();
      score_to_mask_v3_dealer_issue_packages_ft_v3_bounded(
          small_session, small_p0, small_p1, small_context, private_key, registry, state);
      const auto small_info = score_to_mask_v3_validate_package(
          small_p0, public_key, small_session, 0);
      const auto small_ctx = score_to_mask_v3_validate_escrow_context(
          small_context, public_key);
      require(small_info.sampler_retry_limit == kScoreToMaskV3FtV3MaxRetryLimit &&
                  small_ctx.sampler_retry_limit == kScoreToMaskV3FtV3MaxRetryLimit,
              "small-domain F_T/v3 package did not use the signed version retry cap");

      auto package = read_bytes(p0);
      require(package.size() > 228 && package[221] == 3 && package[222] == 2,
              "F_T/v3 package header tuple not encoded at specified offsets");
      package[222] = 1;
      const auto wrong_package = (std::filesystem::path(root) / "wrong.pkg").string();
      write_bytes(wrong_package, package);
      must_reject([&] { (void)score_to_mask_v3_validate_package(
          wrong_package, public_key, session, 0); },
          "sampler-version mismatch accepted before package signature check");
      auto context_bytes = read_bytes(context);
      require(context_bytes.size() > 22 && context_bytes[16] == 3 &&
                  context_bytes[17] == 2,
              "F_T/v3 context profile tuple not serialized in v2 prefix");
      context_bytes[17] = 1;
      const auto wrong_context = (std::filesystem::path(root) / "wrong.context").string();
      write_bytes(wrong_context, context_bytes);
      must_reject([&] { (void)score_to_mask_v3_validate_escrow_context(
          wrong_context, public_key); },
          "context sampler-version mismatch accepted");
      context_bytes = read_bytes(context);
      context_bytes[context_bytes.size() - 64U - 3U] = 1;
      const auto wrong_receipt = (std::filesystem::path(root) / "wrong-receipt.context").string();
      write_bytes(wrong_receipt, context_bytes);
      must_reject([&] { (void)score_to_mask_v3_validate_escrow_context(
          wrong_receipt, public_key); },
          "F_T/v3 context accepted receipt/v1 mismatch");

      const auto abort_registry = (std::filesystem::path(root) / "abort-registry").string();
      std::filesystem::create_directory(abort_registry); ::chmod(abort_registry.c_str(), 0700);
      auto abort_session = test_session(1000, 500);
      const auto abort_p0 = (std::filesystem::path(root) / "abort-p0.pkg").string();
      const auto abort_p1 = (std::filesystem::path(root) / "abort-p1.pkg").string();
      const auto abort_context = (std::filesystem::path(root) / "abort.context").string();
      std::size_t bytes = 0, random_calls = 0, max_request = 0;
      bool aborted = false;
      try {
        test_only::score_to_mask_v3_dealer_issue_packages_ft_v3_with_test_random(
            abort_session, abort_p0, abort_p1, abort_context, private_key,
            abort_registry, state, &bytes, &random_calls, &max_request);
      } catch (const std::exception& e) {
        aborted = std::string(e.what()).find("ISSUANCE_ABORT") != std::string::npos;
      }
      const auto abort_budget = score_to_mask_v3_gate_budget_ft_v3(1000);
      const auto expected_bytes = abort_budget.gate_count + 131072ULL + 32ULL;
      const auto expected_calls = (abort_budget.gate_count + (1U << 20) - 1U) /
          (1U << 20) + 1ULL + 8ULL;
      require(aborted && bytes == expected_bytes && random_calls == expected_calls &&
                  max_request == (1U << 20),
              "N=1000 forced rejection did not stop at the bounded request count");
      require(!std::filesystem::exists(abort_p0) && !std::filesystem::exists(abort_p1) &&
                  !std::filesystem::exists(abort_context),
              "ISSUANCE_ABORT published package or signed context");
      std::size_t markers = 0;
      for (const auto& item : std::filesystem::directory_iterator(abort_registry)) {
        ++markers;
        require(item.file_size() > 0 &&
                    (item.status().permissions() & std::filesystem::perms::owner_read) !=
                        std::filesystem::perms::none,
                "private burn marker missing or unreadable");
      }
      require(markers == 1, "ISSUANCE_ABORT must retain exactly one private burn marker");
      bool replay_rejected = false;
      try {
        test_only::score_to_mask_v3_dealer_issue_packages_ft_v3_with_test_random(
            abort_session, abort_p0, abort_p1, abort_context, private_key,
            abort_registry, state, &bytes, &random_calls, &max_request);
      } catch (const std::exception&) { replay_rejected = true; }
      require(replay_rejected && !std::filesystem::exists(abort_p0) &&
                  !std::filesystem::exists(abort_context),
              "burned invocation was reused after sampler abort");
      std::cout << "S47_FT_V3_BOUNDED_CONFORMANCE PASS gates=88626688 "
                   "Qmax=88786404 Cmax=7247 issuance_abort=PASS\n";
    } catch (...) { cleanup(); throw; }
    cleanup();
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "S47_FT_V3_BOUNDED_CONFORMANCE_FAIL " << e.what() << '\n';
    return 1;
  }
}
