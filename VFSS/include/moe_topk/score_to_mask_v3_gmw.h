#pragma once

#ifndef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_GMW_EXPERIMENTAL
#error "score-to-mask-v3 is opt-in and must be built explicitly"
#endif

#include <array>
#include <cstddef>
#include <cstdint>
#include <moe_topk/protocol_i_transport.h>
#include <string>
#include <string_view>
#include <functional>
#include <vector>

namespace moe_topk {

inline constexpr std::uint32_t kScoreToMaskV3ProtocolVersion = 3;
inline constexpr std::uint32_t kScoreToMaskV3PackageFormatVersion = 4;
inline constexpr std::uint32_t kScoreToMaskV3EscrowContextFormatVersion = 1;
inline constexpr std::uint8_t kScoreToMaskV3ReceiptFormatV1 = 1;
inline constexpr std::uint8_t kScoreToMaskV3ReceiptFormatV2 = 2;
#if defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL) || defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL)
inline constexpr std::uint32_t kScoreToMaskV3PackageFormatVersionFtV2 = 5;
inline constexpr std::uint8_t kScoreToMaskV3FtVersionV1 = 1;
inline constexpr std::uint8_t kScoreToMaskV3FtVersionV2 = 2;
inline constexpr std::uint8_t kScoreToMaskV3SamplerVersionUnboundedV1 = 0;
inline constexpr std::uint8_t kScoreToMaskV3SamplerVersionBoundedV1 = 1;
inline constexpr std::uint8_t kScoreToMaskV3FtV2SecurityParameterBits = 128;
#endif
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
inline constexpr std::uint32_t kScoreToMaskV3PackageFormatVersionFtV3 = 6;
inline constexpr std::uint32_t kScoreToMaskV3EscrowContextFormatVersionFtV3 = 2;
inline constexpr std::uint8_t kScoreToMaskV3FtVersionV3 = 3;
inline constexpr std::uint8_t kScoreToMaskV3SamplerVersionBoundedV2 = 2;
inline constexpr std::uint8_t kScoreToMaskV3FtV3SecurityParameterBits = 128;
inline constexpr std::uint8_t kScoreToMaskV3FtV3MaxRetryLimit = 7;
inline constexpr std::uint32_t kScoreToMaskV3FtV3MaxN = 1000;
inline constexpr std::uint32_t kScoreToMaskV3FtV3MaxPaddedN = 1024;
inline constexpr std::uint64_t kScoreToMaskV3FtV3MaxGateCount = 100000000ULL;
inline constexpr std::size_t kScoreToMaskV3FtV3MaxPackageBytes = 64U * 1024U * 1024U;
inline constexpr std::uint32_t kScoreToMaskV3FtV3MaxBatchGates = 4193824U;
#endif
inline constexpr std::uint32_t kScoreToMaskV3MaxN = 256;
inline constexpr std::size_t kScoreToMaskV3MaxPackageBytes = 2U * 1024U * 1024U;
inline constexpr std::uint32_t kScoreToMaskV3MaxCiphertextBytes = 2U * 1024U * 1024U;
inline constexpr std::size_t kScoreToMaskV3MaxMetadataBytes = 64U * 1024U;
inline constexpr std::size_t kScoreToMaskV3MaxGateBatchFrameBytes = 1U * 1024U * 1024U;
inline constexpr std::string_view kScoreToMaskV3PartyPackageSignatureDomain =
    "ProtocolI/PROJECT_DERIVED_SELECT_5ITER/GMW-Beaver-v3/party-package";
inline constexpr std::string_view kScoreToMaskV3EscrowContextSignatureDomain =
    "ProtocolI/PROJECT_DERIVED_SELECT_5ITER/GMW-Beaver-v3/escrow-context";

struct ScoreToMaskV3Session {
  std::array<std::uint8_t, 16> session{};
  std::array<std::uint8_t, 16> invocation{};
  std::uint32_t n = 0;
  std::uint32_t k = 0;
  std::array<std::array<std::uint8_t, 32>, 2> party_identity_sha256{};
  std::array<std::array<std::uint8_t, 32>, 2> recipient_rsa_public_sha256{};
  std::array<std::uint8_t, 32> escrow_identity_sha256{};
};

struct ScoreToMaskV3GateBudget {
  std::uint32_t padded_n = 0;
  std::uint32_t index_bits = 0;
  std::uint32_t key_bits = 0;
  std::uint64_t gate_count = 0;
  std::uint64_t package_bytes_per_party = 0;
};

struct ScoreToMaskV3PackageInfo {
  ScoreToMaskV3Session parameters;
  std::uint8_t party = 0;
#if defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL) || defined(MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL)
  std::uint32_t package_format_version = kScoreToMaskV3PackageFormatVersion;
  std::uint8_t ft_version = kScoreToMaskV3FtVersionV1;
  std::uint8_t sampler_version = kScoreToMaskV3SamplerVersionUnboundedV1;
  std::uint8_t sampler_security_parameter_bits = 0;
  std::uint8_t sampler_retry_limit = 0;
#endif
  std::array<std::uint8_t, 32> circuit_sha256{};
  std::array<std::uint8_t, 32> gate_schedule_sha256{};
  std::array<std::uint8_t, 32> package_sha256{};
  std::uint64_t gate_count = 0;
  std::size_t package_bytes = 0;
};

struct ScoreToMaskV3EscrowStateParameters {
  std::uint32_t transaction_wait_timeout_ms = 30000;
  std::uint32_t max_ciphertext_bytes =
      static_cast<std::uint32_t>(kScoreToMaskV3MaxPackageBytes);
  std::uint8_t pending_restart_policy = 1;  // abort pending transactions
  std::uint8_t receipt_format_version = kScoreToMaskV3ReceiptFormatV1;
};

// Signed, metadata-only context consumed by escrow A. It intentionally has no
// package path, PackageInfo, Beaver material, permutation shares, or score
// shares. Package digests bind this service context to the two signed T outputs.
struct ScoreToMaskV3EscrowContext {
  std::uint32_t format_version = kScoreToMaskV3EscrowContextFormatVersion;
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
  std::uint8_t ft_version = 0;
  std::uint8_t sampler_version = 0;
  std::uint8_t sampler_security_parameter_bits = 0;
  std::uint8_t sampler_retry_limit = 0;
  std::uint32_t package_format_version = kScoreToMaskV3PackageFormatVersion;
#endif
  std::array<std::uint8_t, 16> session{};
  std::array<std::uint8_t, 16> invocation{};
  std::uint32_t n = 0;
  std::uint32_t k = 0;
  std::array<std::array<std::uint8_t, 32>, 2> party_identity_sha256{};
  std::array<std::uint8_t, 32> escrow_identity_sha256{};
  std::array<std::uint8_t, 32> circuit_sha256{};
  std::array<std::uint8_t, 32> gate_schedule_sha256{};
  std::array<std::array<std::uint8_t, 32>, 2> party_package_sha256{};
  std::array<std::uint8_t, 32> package_set_sha256{};
  std::array<std::uint8_t, 32> signer_identity_sha256{};
  ScoreToMaskV3EscrowStateParameters state;
  std::array<std::uint8_t, 64> signature{};
  std::array<std::uint8_t, 32> context_sha256{};
};

struct ScoreToMaskV3BeaverTripleShare {
  std::uint8_t a = 0;
  std::uint8_t b = 0;
  std::uint8_t c = 0;
};

struct ScoreToMaskV3LocalMaterial {
  ScoreToMaskV3PackageInfo info;
  std::string package_path;
  std::vector<std::uint8_t> packed_triple_bits;
  std::vector<std::uint8_t> packed_permutation_bits;

  ScoreToMaskV3BeaverTripleShare triple(std::uint64_t gate_id) const;
  std::uint8_t permutation_share(std::uint32_t row,
                                std::uint32_t original) const;
};

struct ScoreToMaskV3PartyInput {
  // Additive shares modulo 2^32, one per logical score. Padding is supplied
  // by the runtime and uses the S31 sentinel contract.
  std::vector<std::uint32_t> additive_score_share;
};

struct ScoreToMaskV3PartyOutput {
  // A transport failure may race with A's durable commit. In that case the
  // caller has no API-visible share and must recover/query by transaction id.
  enum class State { kAborted, kRecoveryRequired, kCommitted } state = State::kAborted;
  std::vector<std::uint8_t> local_mask_share;
  std::string transaction_id;
  // Opaque 32-byte receipt encoded as lowercase hex. The signed escrow
  // context selects receipt/v1 (deterministic digest) or opt-in receipt/v2
  // (A-generated random token); parties validate encoding, not authenticity.
  std::string commit_receipt;
  std::string abort_code;
  std::uint64_t party_channel_bytes_sent = 0;
  std::uint64_t party_channel_bytes_received = 0;
  std::uint64_t party_channel_frames_sent = 0;
  std::uint64_t party_channel_frames_received = 0;
  std::uint16_t party_channel_phases = 0;
  std::size_t party_channel_max_payload_sent = 0;
  std::size_t party_channel_max_payload_received = 0;
};

struct ScoreToMaskV3EscrowClientConfig {
  std::string host;
  std::uint16_t port = 0;
  std::string expected_server_identity;
  ProtocolITls13Credentials tls_credentials;
};

struct ScoreToMaskV3EscrowServerConfig {
  std::string bind_address = "127.0.0.1";
  std::array<std::uint16_t, 2> party_ports{};
  std::array<std::string, 2> party_tls_identities{};
  std::string sqlite_database_path;
  ProtocolITls13Credentials tls_credentials;
  std::string signed_context_path;
  std::string dealer_ed25519_public_key_pem;
};

struct ScoreToMaskV3EscrowFetchedRecord {
  std::vector<std::uint8_t> recipient_ciphertext;
  std::string commit_receipt;
};

using ScoreToMaskV3EscrowSubmitter = std::function<std::string(
    const std::string& transaction_id, std::uint8_t party,
    const std::vector<std::uint8_t>& recipient_ciphertext)>;

ScoreToMaskV3GateBudget score_to_mask_v3_gate_budget(std::uint32_t n);
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
ScoreToMaskV3GateBudget score_to_mask_v3_gate_budget_ft_v3(std::uint32_t n);
#endif
ProtocolIFrameConfig score_to_mask_v3_frame_config(
    const ScoreToMaskV3PackageInfo& package, std::uint8_t party);
ProtocolIFrameConfig score_to_mask_v3_escrow_frame_config(
    const ScoreToMaskV3EscrowContext& context, std::uint8_t party,
    bool server);

// Offline T entry point. It generates fresh, input-independent Boolean
// Beaver triples and XOR-shared one-hot permutation material, signs each
// party package with Ed25519, then returns. The private signing key remains
// with T and is not used online.
void score_to_mask_v3_dealer_issue_packages(
    const ScoreToMaskV3Session& session,
    const std::string& package_p0_path,
    const std::string& package_p1_path,
    const std::string& signed_escrow_context_path,
    const std::string& dealer_ed25519_private_key_pem,
    const ScoreToMaskV3EscrowStateParameters& escrow_state = {});

#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL
// Explicit F_T/v2 issuance. This never falls back to the F_T/v1 sampler.
// The v2 bounded-rejection profile is signed in the package format v5 header.
// The private registry directory must be stable across T restarts and all
// issuers for this deployment. Its 0700 owner-only files burn each
// session/invocation once, including bounded-sampler aborts.
void score_to_mask_v3_dealer_issue_packages_ft_v2_bounded(
    const ScoreToMaskV3Session& session,
    const std::string& package_p0_path,
    const std::string& package_p1_path,
    const std::string& signed_escrow_context_path,
    const std::string& dealer_ed25519_private_key_pem,
    const std::string& private_issuance_registry_directory,
    const ScoreToMaskV3EscrowStateParameters& escrow_state = {});
#endif
#ifdef MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL
// Explicit F_T/v3 issuance for N<=1000 and padded m<=1024. This does not
// change the legacy F_T/v1 or F_T/v2 entry points or their resource limits.
void score_to_mask_v3_dealer_issue_packages_ft_v3_bounded(
    const ScoreToMaskV3Session& session,
    const std::string& package_p0_path,
    const std::string& package_p1_path,
    const std::string& signed_escrow_context_path,
    const std::string& dealer_ed25519_private_key_pem,
    const std::string& private_issuance_registry_directory,
    const ScoreToMaskV3EscrowStateParameters& escrow_state = {});
#endif

ScoreToMaskV3PackageInfo score_to_mask_v3_validate_package(
    const std::string& package_path,
    const std::string& dealer_ed25519_public_key_pem,
    const ScoreToMaskV3Session& expected,
    std::uint8_t expected_party);

ScoreToMaskV3EscrowContext score_to_mask_v3_validate_escrow_context(
    const std::string& signed_context_path,
    const std::string& dealer_ed25519_public_key_pem);

ScoreToMaskV3LocalMaterial score_to_mask_v3_open_material(
    const std::string& package_path,
    const std::string& dealer_ed25519_public_key_pem,
    const ScoreToMaskV3Session& expected,
    std::uint8_t expected_party);

// A one-shot, durable local lease. A successful call consumes the package
// before any online message is sent. It never deletes or reopens a lease.
void score_to_mask_v3_consume_package_lease(
    const std::string& package_path,
    const std::string& lease_directory,
    const ScoreToMaskV3PackageInfo& package);

std::array<std::uint8_t, 32> score_to_mask_v3_public_key_sha256(
    const std::string& public_key_pem);

// Execute the fixed v3 GMW circuit over an already authenticated,
// session-bound Protocol I framed channel. The callback must submit only the
// recipient ciphertext to escrow and return only after A's durable atomic
// F_TOPK-ATOMIC commit receipt. The local share remains internal until then;
// a lost/invalid receipt yields kRecoveryRequired, never a false ABORT.
ScoreToMaskV3PartyOutput score_to_mask_v3_execute_party(
    const std::string& package_path,
    const std::string& dealer_ed25519_public_key_pem,
    const ScoreToMaskV3Session& expected_session,
    const ScoreToMaskV3PartyInput& input,
    std::uint8_t party,
    const std::string& lease_directory,
    std::uint64_t peak_memory_cap_bytes,
    ProtocolIFramedChannel& peer_channel,
    const std::string& own_recipient_rsa_public_key_pem,
    const ScoreToMaskV3EscrowSubmitter& escrow_submitter);

std::array<std::uint8_t, 32> score_to_mask_v3_certificate_sha256(
    const std::string& certificate_pem);
void score_to_mask_v3_run_escrow_service(
    const ScoreToMaskV3EscrowServerConfig& config);
std::string score_to_mask_v3_escrow_submit(
    const ScoreToMaskV3EscrowClientConfig& config,
    const ScoreToMaskV3PackageInfo& package, std::uint8_t party,
    const std::string& transaction_id,
    const std::vector<std::uint8_t>& recipient_ciphertext);
ScoreToMaskV3EscrowFetchedRecord score_to_mask_v3_escrow_fetch(
    const ScoreToMaskV3EscrowClientConfig& config,
    const ScoreToMaskV3PackageInfo& package, std::uint8_t party,
    const std::string& transaction_id);
std::vector<std::uint8_t> score_to_mask_v3_decrypt_recipient_share(
    const std::vector<std::uint8_t>& recipient_ciphertext,
    const std::string& recipient_rsa_private_key_pem,
    const ScoreToMaskV3PackageInfo& package, std::uint8_t party);

}  // namespace moe_topk
