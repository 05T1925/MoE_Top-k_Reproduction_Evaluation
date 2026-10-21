#include <moe_topk/score_to_mask_v3_gmw.h>
#include <moe_topk/project_derived_select5.h>
#include <moe_topk/protocol_i_transport.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>

namespace moe_topk {
namespace {

constexpr std::uint8_t kTagReady = 1;
constexpr std::uint8_t kTagBeaver = 2;
constexpr std::uint8_t kTagSample = 3;
constexpr std::uint8_t kTagSampleAck = 4;
constexpr std::uint8_t kTagOpenedCompare = 5;
constexpr std::uint8_t kTagOutputRefresh = 6;
constexpr std::uint8_t kTagAbort = 0xff;
constexpr std::uint8_t kEnvelopeVersion = 1;
constexpr std::uint32_t kFrameMagic = 0x53334d46U;  // S3MF
constexpr std::size_t kMaxPayload = 1U << 20;

void fail(const std::string& message) { throw std::runtime_error(message); }

void put_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
  out.push_back(static_cast<std::uint8_t>(value >> 8U));
  out.push_back(static_cast<std::uint8_t>(value));
}

void put_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
  out.push_back(static_cast<std::uint8_t>(value >> 24U));
  out.push_back(static_cast<std::uint8_t>(value >> 16U));
  out.push_back(static_cast<std::uint8_t>(value >> 8U));
  out.push_back(static_cast<std::uint8_t>(value));
}

std::uint32_t read_u32(const std::vector<std::uint8_t>& in, std::size_t& at) {
  if (at + 4 > in.size()) fail("v3 message truncated u32");
  std::uint32_t value = 0;
  for (int i = 0; i < 4; ++i) value = (value << 8U) | in[at++];
  return value;
}

std::uint64_t first_u64(const std::array<std::uint8_t, 16>& value) {
  std::uint64_t out = 0;
  for (std::size_t i = 0; i < 8; ++i) out = (out << 8U) | value[i];
  return out;
}

std::uint64_t first_u64(const std::array<std::uint8_t, 32>& value) {
  std::uint64_t out = 0;
  for (std::size_t i = 0; i < 8; ++i) out = (out << 8U) | value[i];
  return out;
}

std::vector<std::uint8_t> encode_context(
    const ScoreToMaskV3PackageInfo& package, std::uint8_t party,
    std::uint8_t phase, std::uint8_t tag,
    const std::vector<std::uint8_t>& body) {
  std::vector<std::uint8_t> out;
  out.reserve(4 + 1 + 1 + 1 + 1 + 16 + 16 + 32 + 32 + body.size());
  put_u32(out, kFrameMagic);
  out.push_back(kEnvelopeVersion);
  out.push_back(phase);
  out.push_back(party);
  out.push_back(tag);
  out.insert(out.end(), package.parameters.session.begin(), package.parameters.session.end());
  out.insert(out.end(), package.parameters.invocation.begin(), package.parameters.invocation.end());
  out.insert(out.end(), package.circuit_sha256.begin(), package.circuit_sha256.end());
  out.insert(out.end(), package.gate_schedule_sha256.begin(),
             package.gate_schedule_sha256.end());
  out.insert(out.end(), body.begin(), body.end());
  if (out.size() > kMaxPayload) fail("v3 inner frame exceeds 1 MiB");
  return out;
}

struct DecodedContext {
  std::uint8_t phase = 0;
  std::uint8_t party = 0;
  std::uint8_t tag = 0;
  std::vector<std::uint8_t> body;
};

DecodedContext decode_context(const std::vector<std::uint8_t>& in,
                              const ScoreToMaskV3PackageInfo& package,
                              std::uint8_t expected_party,
                              std::uint8_t expected_phase) {
  constexpr std::size_t prefix = 4 + 4 + 16 + 16 + 32 + 32;
  if (in.size() < prefix) fail("v3 inner frame truncated");
  std::size_t at = 0;
  if (read_u32(in, at) != kFrameMagic || in[at++] != kEnvelopeVersion)
    fail("v3 inner frame magic/version");
  DecodedContext decoded;
  decoded.phase = in[at++];
  decoded.party = in[at++];
  decoded.tag = in[at++];
  if (decoded.phase != expected_phase || decoded.party != expected_party)
    fail("v3 inner frame phase/party mismatch");
  if (!std::equal(package.parameters.session.begin(), package.parameters.session.end(),
                  in.begin() + static_cast<std::ptrdiff_t>(at)))
    fail("v3 inner frame session mismatch");
  at += package.parameters.session.size();
  if (!std::equal(package.parameters.invocation.begin(), package.parameters.invocation.end(),
                  in.begin() + static_cast<std::ptrdiff_t>(at)))
    fail("v3 inner frame invocation mismatch");
  at += package.parameters.invocation.size();
  if (!std::equal(package.circuit_sha256.begin(), package.circuit_sha256.end(),
                  in.begin() + static_cast<std::ptrdiff_t>(at)))
    fail("v3 inner frame circuit mismatch");
  at += package.circuit_sha256.size();
  if (!std::equal(package.gate_schedule_sha256.begin(), package.gate_schedule_sha256.end(),
                  in.begin() + static_cast<std::ptrdiff_t>(at)))
    fail("v3 inner frame gate schedule mismatch");
  at += package.gate_schedule_sha256.size();
  if (decoded.tag == kTagAbort) fail("uniform peer ABORT");
  decoded.body.assign(in.begin() + static_cast<std::ptrdiff_t>(at), in.end());
  return decoded;
}

std::uint8_t exchange_phase(ProtocolIFramedChannel& channel,
                            std::uint16_t& next_phase,
                            const ScoreToMaskV3PackageInfo& package,
                            std::uint8_t party, std::uint8_t tag,
                            const std::vector<std::uint8_t>& local_body,
                            std::vector<std::uint8_t>* peer_body) {
  if (next_phase == 0 || next_phase > 255) fail("v3 online phase budget exceeded");
  const auto phase = static_cast<std::uint8_t>(next_phase++);
  channel.set_phase(phase);
  const auto local = encode_context(package, party, phase, tag, local_body);
  std::vector<std::uint8_t> remote;
  if (party == 0) {
    channel.send(local);
    remote = channel.receive();
    auto decoded = decode_context(remote, package, 1, phase);
    if (decoded.tag != tag) fail("v3 peer message type mismatch");
    if (peer_body != nullptr) *peer_body = std::move(decoded.body);
  } else {
    remote = channel.receive();
    auto decoded = decode_context(remote, package, 0, phase);
    if (decoded.tag != tag) fail("v3 peer message type mismatch");
    if (peer_body != nullptr) *peer_body = std::move(decoded.body);
    channel.send(local);
  }
  return phase;
}

void exchange_sample_seed(ProtocolIFramedChannel& channel,
                          std::uint16_t& next_phase,
                          const ScoreToMaskV3PackageInfo& package,
                          std::uint8_t party, std::uint8_t iteration,
                          ProjectDerivedSelect5PublicSeed& seed) {
  if (next_phase == 0 || next_phase > 255) fail("v3 online phase budget exceeded");
  const auto phase = static_cast<std::uint8_t>(next_phase++);
  channel.set_phase(phase);
  if (party == 0) {
    if (RAND_bytes(seed.data(), static_cast<int>(seed.size())) != 1)
      fail("v3 CSPRNG failed for Select5 sample seed");
    std::vector<std::uint8_t> body{iteration};
    body.insert(body.end(), seed.begin(), seed.end());
    channel.send(encode_context(package, 0, phase, kTagSample, body));
    const auto remote = decode_context(channel.receive(), package, 1, phase);
    if (remote.tag != kTagSampleAck || remote.body.size() != 33 ||
        remote.body[0] != iteration)
      fail("v3 sample-seed acknowledgement mismatch");
    std::array<std::uint8_t, 32> digest{};
    if (SHA256(seed.data(), seed.size(), digest.data()) == nullptr ||
        !std::equal(digest.begin(), digest.end(), remote.body.begin() + 1))
      fail("v3 sample-seed acknowledgement digest mismatch");
  } else {
    const auto remote = decode_context(channel.receive(), package, 0, phase);
    if (remote.tag != kTagSample || remote.body.size() != 33 ||
        remote.body[0] != iteration)
      fail("v3 sample seed message malformed");
    std::copy(remote.body.begin() + 1, remote.body.end(), seed.begin());
    std::array<std::uint8_t, 32> digest{};
    if (SHA256(seed.data(), seed.size(), digest.data()) == nullptr)
      fail("v3 sample seed digest failed");
    std::vector<std::uint8_t> body{iteration};
    body.insert(body.end(), digest.begin(), digest.end());
    channel.send(encode_context(package, 1, phase, kTagSampleAck, body));
  }
}

struct GateInput {
  std::uint64_t id;
  std::uint8_t x;
  std::uint8_t y;
};

class GmwParty {
 public:
  GmwParty(const ScoreToMaskV3LocalMaterial& material, std::uint8_t party,
           ProtocolIFramedChannel& channel)
      : material_(material), party_(party), channel_(channel),
        consumed_(static_cast<std::size_t>(material.info.gate_count), 0) {}

  std::vector<std::uint8_t> and_layer(const std::vector<GateInput>& gates) {
    if (gates.empty()) return {};
    const auto packed_bytes = (2ULL * gates.size() + 7U) / 8U;
    if (4U + packed_bytes > kMaxPayload - (4 + 4 + 16 + 16 + 32 + 32))
      fail("v3 AND layer exceeds one gate-batch frame");
    std::vector<std::uint8_t> body;
    body.reserve(4U + static_cast<std::size_t>(packed_bytes));
    put_u32(body, static_cast<std::uint32_t>(gates.size()));
    body.resize(4U + static_cast<std::size_t>(packed_bytes), 0);
    std::vector<ScoreToMaskV3BeaverTripleShare> triples;
    triples.reserve(gates.size());
    for (std::size_t i = 0; i < gates.size(); ++i) {
      const auto& gate = gates[i];
      if (gate.id >= material_.info.gate_count || consumed_[gate.id] != 0 ||
          gate.x > 1 || gate.y > 1)
        fail("v3 invalid, repeated, or out-of-range gate id/input");
      consumed_[gate.id] = 1;
      const auto triple = material_.triple(gate.id);
      triples.push_back(triple);
      const auto d = static_cast<std::uint8_t>(gate.x ^ triple.a);
      const auto e = static_cast<std::uint8_t>(gate.y ^ triple.b);
      if (d != 0) body[4U + (2U * i) / 8U] |=
          static_cast<std::uint8_t>(1U << ((2U * i) % 8U));
      if (e != 0) body[4U + (2U * i + 1U) / 8U] |=
          static_cast<std::uint8_t>(1U << ((2U * i + 1U) % 8U));
    }
    std::vector<std::uint8_t> remote;
    exchange_phase(channel_, next_phase_, material_.info, party_, kTagBeaver,
                   body, &remote);
    if (remote.size() != body.size()) fail("v3 Beaver opening frame length mismatch");
    std::size_t at = 0;
    if (read_u32(remote, at) != gates.size()) fail("v3 Beaver gate batch count mismatch");
    std::vector<std::uint8_t> outputs(gates.size());
    for (std::size_t i = 0; i < gates.size(); ++i) {
      const auto own_d = static_cast<std::uint8_t>(
          (body[4U + (2U * i) / 8U] >> ((2U * i) % 8U)) & 1U);
      const auto own_e = static_cast<std::uint8_t>(
          (body[4U + (2U * i + 1U) / 8U] >> ((2U * i + 1U) % 8U)) & 1U);
      const auto other_d = static_cast<std::uint8_t>(
          (remote[4U + (2U * i) / 8U] >> ((2U * i) % 8U)) & 1U);
      const auto other_e = static_cast<std::uint8_t>(
          (remote[4U + (2U * i + 1U) / 8U] >> ((2U * i + 1U) % 8U)) & 1U);
      const auto d = static_cast<std::uint8_t>(own_d ^ other_d);
      const auto e = static_cast<std::uint8_t>(own_e ^ other_e);
      const auto& triple = triples[i];
      outputs[i] = static_cast<std::uint8_t>(triple.c ^ (d & triple.b) ^
          (e & triple.a) ^ ((party_ == 0) ? (d & e) : 0U));
    }
    return outputs;
  }

  std::uint8_t exchange(std::uint8_t tag, const std::vector<std::uint8_t>& local,
                        std::vector<std::uint8_t>* remote) {
    return exchange_phase(channel_, next_phase_, material_.info, party_, tag,
                          local, remote);
  }

  void seed(std::uint8_t iteration, ProjectDerivedSelect5PublicSeed& out) {
    exchange_sample_seed(channel_, next_phase_, material_.info, party_, iteration, out);
  }

  void abort_best_effort() noexcept {
    try {
      if (next_phase_ == 0 || next_phase_ > 255) return;
      const auto phase = static_cast<std::uint8_t>(next_phase_++);
      channel_.set_phase(phase);
      channel_.send(encode_context(material_.info, party_, phase, kTagAbort, {0}));
    } catch (...) {}
  }

  bool all_gates_consumed() const {
    return std::all_of(consumed_.begin(), consumed_.end(),
                       [](std::uint8_t value) { return value == 1; });
  }

  std::uint16_t next_phase() const { return next_phase_; }

 private:
  const ScoreToMaskV3LocalMaterial& material_;
  std::uint8_t party_;
  ProtocolIFramedChannel& channel_;
  std::uint16_t next_phase_ = 1;
  std::vector<std::uint8_t> consumed_;
};

std::uint8_t xor_bit(std::uint8_t a, std::uint8_t b) {
  return static_cast<std::uint8_t>(a ^ b);
}

std::uint8_t not_share(std::uint8_t a, std::uint8_t party) {
  return static_cast<std::uint8_t>(a ^ (party == 0 ? 1U : 0U));
}

std::uint8_t public_share(std::uint8_t value, std::uint8_t party) {
  return party == 0 ? value : 0;
}

struct PairInfo {
  std::uint32_t left;
  std::uint32_t right;
  std::uint64_t gate_base;
};

void check_channel(ProtocolIFramedChannel& channel,
                   const ScoreToMaskV3PackageInfo& package,
                   std::uint8_t party) {
  const auto expected = score_to_mask_v3_frame_config(package, party);
  const auto& actual = channel.configuration();
  if (actual.session != expected.session || actual.fingerprint != expected.fingerprint ||
      actual.n != expected.n || actual.k != expected.k || actual.bits != expected.bits ||
      actual.sender != expected.sender || actual.receiver != expected.receiver ||
      actual.type != expected.type || actual.material_id != expected.material_id)
    fail("v3 authenticated channel does not match package context");
  channel.complete_authenticated_handshake();
  if (channel.peer_certificate_sha256() !=
      package.parameters.party_identity_sha256[1U - party])
    fail("v3 TLS peer certificate does not match signed party identity");
}

std::uint64_t checked_add(std::uint64_t left, std::uint64_t right) {
  if (right > std::numeric_limits<std::uint64_t>::max() - left)
    fail("v3 resource estimate overflow");
  return left + right;
}

std::uint64_t checked_mul(std::uint64_t left, std::uint64_t right) {
  if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left)
    fail("v3 resource estimate overflow");
  return left * right;
}

std::uint64_t estimated_peak_bytes(const ScoreToMaskV3GateBudget& budget,
                                   std::uint64_t gate_count) {
  const std::uint64_t n = budget.padded_n;
  const std::uint64_t pair_count = n * (n - 1U) / 2U;
  const auto max_layer = std::max({n * n * budget.key_bits,
                                   pair_count * budget.key_bits,
                                   n * n, 63U * n});
  std::uint64_t total = budget.package_bytes_per_party;
  total = checked_add(total, gate_count);  // one byte per consumed-gate marker
  total = checked_add(total, checked_mul(max_layer, sizeof(GateInput)));
  total = checked_add(total, max_layer);   // local result bits
  total = checked_add(total, checked_mul(n * n, 2U));  // comparator shares + membership
  total = checked_add(total, checked_mul(n, budget.key_bits * 3U));
  return total;
}

std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> read_rsa_public_key(
    const std::string& path) {
  FILE* file = std::fopen(path.c_str(), "rb");
  if (file == nullptr) fail("cannot open v3 recipient RSA public key");
  EVP_PKEY* raw = PEM_read_PUBKEY(file, nullptr, nullptr, nullptr);
  std::fclose(file);
  if (raw == nullptr || EVP_PKEY_base_id(raw) != EVP_PKEY_RSA || EVP_PKEY_bits(raw) < 2048) {
    if (raw != nullptr) EVP_PKEY_free(raw);
    fail("v3 recipient public key must be RSA >= 2048 bits");
  }
  return {raw, EVP_PKEY_free};
}

std::vector<std::uint8_t> encrypt_share(
    const std::vector<std::uint8_t>& share,
    const ScoreToMaskV3PackageInfo& package, std::uint8_t party,
    const std::string& recipient_public_key_path) {
  if (score_to_mask_v3_public_key_sha256(recipient_public_key_path) !=
      package.parameters.recipient_rsa_public_sha256[party])
    fail("v3 recipient public key does not match signed package identity");
  auto recipient = read_rsa_public_key(recipient_public_key_path);
  std::array<std::uint8_t, 32> aes_key{};
  std::array<std::uint8_t, 12> nonce{};
  if (RAND_bytes(aes_key.data(), static_cast<int>(aes_key.size())) != 1 ||
      RAND_bytes(nonce.data(), static_cast<int>(nonce.size())) != 1)
    fail("v3 output envelope CSPRNG failed");
  std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> rsa_ctx(
      EVP_PKEY_CTX_new(recipient.get(), nullptr), EVP_PKEY_CTX_free);
  if (!rsa_ctx || EVP_PKEY_encrypt_init(rsa_ctx.get()) <= 0 ||
      EVP_PKEY_CTX_set_rsa_padding(rsa_ctx.get(), RSA_PKCS1_OAEP_PADDING) <= 0 ||
      EVP_PKEY_CTX_set_rsa_oaep_md(rsa_ctx.get(), EVP_sha256()) <= 0 ||
      EVP_PKEY_CTX_set_rsa_mgf1_md(rsa_ctx.get(), EVP_sha256()) <= 0)
    fail("v3 RSA-OAEP initialization failed");
  std::size_t encrypted_key_size = 0;
  if (EVP_PKEY_encrypt(rsa_ctx.get(), nullptr, &encrypted_key_size,
                       aes_key.data(), aes_key.size()) <= 0 || encrypted_key_size > 65535)
    fail("v3 RSA-OAEP encrypted-key sizing failed");
  std::vector<std::uint8_t> encrypted_key(encrypted_key_size);
  if (EVP_PKEY_encrypt(rsa_ctx.get(), encrypted_key.data(), &encrypted_key_size,
                       aes_key.data(), aes_key.size()) <= 0)
    fail("v3 RSA-OAEP key encryption failed");
  encrypted_key.resize(encrypted_key_size);

  std::vector<std::uint8_t> aad;
  aad.insert(aad.end(), package.parameters.session.begin(), package.parameters.session.end());
  aad.insert(aad.end(), package.parameters.invocation.begin(), package.parameters.invocation.end());
  aad.push_back(party);
  aad.insert(aad.end(), package.circuit_sha256.begin(), package.circuit_sha256.end());
  aad.insert(aad.end(), package.gate_schedule_sha256.begin(),
             package.gate_schedule_sha256.end());
  std::vector<std::uint8_t> encrypted_share(share.size() + 16U);
  std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> aes_ctx(
      EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
  int written = 0;
  int total = 0;
  if (!aes_ctx || EVP_EncryptInit_ex(aes_ctx.get(), EVP_aes_256_gcm(), nullptr,
                                     nullptr, nullptr) != 1 ||
      EVP_CIPHER_CTX_ctrl(aes_ctx.get(), EVP_CTRL_GCM_SET_IVLEN,
                          static_cast<int>(nonce.size()), nullptr) != 1 ||
      EVP_EncryptInit_ex(aes_ctx.get(), nullptr, nullptr, aes_key.data(), nonce.data()) != 1 ||
      EVP_EncryptUpdate(aes_ctx.get(), nullptr, &written, aad.data(),
                        static_cast<int>(aad.size())) != 1 ||
      EVP_EncryptUpdate(aes_ctx.get(), encrypted_share.data(), &written,
                        share.data(), static_cast<int>(share.size())) != 1)
    fail("v3 AES-GCM encryption failed");
  total = written;
  if (EVP_EncryptFinal_ex(aes_ctx.get(), encrypted_share.data() + total, &written) != 1)
    fail("v3 AES-GCM finalization failed");
  total += written;
  encrypted_share.resize(static_cast<std::size_t>(total));
  std::array<std::uint8_t, 16> tag{};
  if (EVP_CIPHER_CTX_ctrl(aes_ctx.get(), EVP_CTRL_GCM_GET_TAG,
                          static_cast<int>(tag.size()), tag.data()) != 1)
    fail("v3 AES-GCM tag extraction failed");

  const std::array<std::uint8_t, 8> magic{'S','2','M','E','N','C','3',1};
  std::vector<std::uint8_t> envelope(magic.begin(), magic.end());
  put_u16(envelope, static_cast<std::uint16_t>(encrypted_key.size()));
  put_u32(envelope, static_cast<std::uint32_t>(share.size()));
  envelope.insert(envelope.end(), nonce.begin(), nonce.end());
  envelope.insert(envelope.end(), tag.begin(), tag.end());
  envelope.insert(envelope.end(), encrypted_key.begin(), encrypted_key.end());
  envelope.insert(envelope.end(), encrypted_share.begin(), encrypted_share.end());
  return envelope;
}

std::string transaction_id(const ScoreToMaskV3PackageInfo& package) {
  static constexpr char hex[] = "0123456789abcdef";
  std::string result = "s2m-v3-";
  for (const auto byte : package.parameters.session) {
    result.push_back(hex[byte >> 4U]); result.push_back(hex[byte & 0xfU]);
  }
  result.push_back('-');
  for (const auto byte : package.parameters.invocation) {
    result.push_back(hex[byte >> 4U]); result.push_back(hex[byte & 0xfU]);
  }
  return result;
}

}  // namespace

ScoreToMaskV3PartyOutput score_to_mask_v3_execute_party(
    const std::string& package_path,
    const std::string& dealer_ed25519_public_key_pem,
    const ScoreToMaskV3Session& expected_session,
    const ScoreToMaskV3PartyInput& input, std::uint8_t party,
    const std::string& lease_directory, std::uint64_t peak_memory_cap_bytes,
    ProtocolIFramedChannel& peer_channel,
    const std::string& own_recipient_rsa_public_key_pem,
    const ScoreToMaskV3EscrowSubmitter& escrow_submitter) {
  ScoreToMaskV3PartyOutput output;
  std::unique_ptr<ScoreToMaskV3LocalMaterial> material_ptr;
  std::unique_ptr<GmwParty> engine;
  try {
    material_ptr = std::make_unique<ScoreToMaskV3LocalMaterial>(
        score_to_mask_v3_open_material(package_path,
            dealer_ed25519_public_key_pem, expected_session, party));
    const auto& material = *material_ptr;
    const auto& package = material.info;
    if (party > 1 || package.party != party) fail("v3 party/package mismatch");
    if (!escrow_submitter) fail("v3 escrow submitter is required");
    if (input.additive_score_share.size() != package.parameters.n)
      fail("v3 additive input share length mismatch");
    const auto budget = score_to_mask_v3_gate_budget(package.parameters.n);
    if (package.gate_count != budget.gate_count ||
        material.packed_triple_bits.size() != (3ULL * budget.gate_count + 7U) / 8U ||
        material.packed_permutation_bits.size() !=
            (static_cast<std::uint64_t>(budget.padded_n) * budget.padded_n + 7U) / 8U)
      fail("v3 local material package preflight failed");
    if (package.parameters.k == 0 || package.parameters.k > package.parameters.n ||
        package.parameters.n > kScoreToMaskV3MaxN)
      fail("v3 N/K preflight failed");
    if (estimated_peak_bytes(budget, budget.gate_count) > peak_memory_cap_bytes)
      fail("v3 checked peak-memory estimate exceeds configured cap");
    check_channel(peer_channel, package, party);
    peer_channel.reset_counters();
    if (score_to_mask_v3_public_key_sha256(own_recipient_rsa_public_key_pem) !=
        package.parameters.recipient_rsa_public_sha256[party])
      fail("v3 recipient key identity does not match package");

    score_to_mask_v3_consume_package_lease(
        material.package_path, lease_directory, package);
    engine = std::make_unique<GmwParty>(material, party, peer_channel);
    std::vector<std::uint8_t> peer_ready;
    engine->exchange(kTagReady, {1}, &peer_ready);
    if (peer_ready != std::vector<std::uint8_t>{1}) fail("v3 peer READY mismatch");

    const auto n = budget.padded_n;
    const auto width = budget.key_bits;
    std::vector<std::array<std::uint8_t, 32>> input0(n), input1(n), score(n);
    for (std::uint32_t i = 0; i < n; ++i) {
      const auto share0 = (party == 0)
          ? (i < package.parameters.n ? input.additive_score_share[i] : UINT32_C(0x80000000))
          : 0U;
      const auto share1 = (party == 1 && i < package.parameters.n)
          ? input.additive_score_share[i] : 0U;
      for (std::uint32_t bit = 0; bit < 32; ++bit) {
        input0[i][bit] = static_cast<std::uint8_t>((share0 >> bit) & 1U);
        input1[i][bit] = static_cast<std::uint8_t>((share1 >> bit) & 1U);
      }
    }
    std::vector<GateInput> gates;
    gates.reserve(static_cast<std::size_t>(32U) * n);
    for (std::uint32_t word = 0; word < n; ++word)
      for (std::uint32_t bit = 0; bit < 32; ++bit)
        gates.push_back({static_cast<std::uint64_t>(word) * 63U + bit,
                         input0[word][bit], input1[word][bit]});
    const auto generated = engine->and_layer(gates);
    gates.clear();
    std::vector<std::array<std::uint8_t, 32>> generate(n);
    std::vector<std::uint8_t> carry(n);
    for (std::uint32_t word = 0; word < n; ++word)
      for (std::uint32_t bit = 0; bit < 32; ++bit)
        generate[word][bit] = generated[static_cast<std::size_t>(word) * 32U + bit];
    for (std::uint32_t word = 0; word < n; ++word) {
      carry[word] = generate[word][0];
      score[word][0] = xor_bit(input0[word][0], input1[word][0]);
    }
    for (std::uint32_t bit = 1; bit < 32; ++bit) {
      gates.clear();
      for (std::uint32_t word = 0; word < n; ++word) {
        const auto propagate = xor_bit(input0[word][bit], input1[word][bit]);
        gates.push_back({static_cast<std::uint64_t>(word) * 63U + 32U + bit - 1U,
                         propagate, carry[word]});
      }
      const auto carry_terms = engine->and_layer(gates);
      for (std::uint32_t word = 0; word < n; ++word) {
        const auto propagate = xor_bit(input0[word][bit], input1[word][bit]);
        score[word][bit] = xor_bit(propagate, carry[word]);
        carry[word] = xor_bit(generate[word][bit], carry_terms[word]);
      }
    }

    std::vector<std::vector<std::uint8_t>> keys(n, std::vector<std::uint8_t>(width));
    for (std::uint32_t i = 0; i < n; ++i) {
      for (std::uint32_t bit = 0; bit < budget.index_bits; ++bit)
        keys[i][bit] = public_share(static_cast<std::uint8_t>((i >> bit) & 1U), party);
      for (std::uint32_t bit = 0; bit < 32; ++bit) {
        auto ordered = score[i][bit];
        if (bit == 31) ordered = not_share(ordered, party);
        keys[i][budget.index_bits + bit] = not_share(ordered, party);
      }
    }

    const std::uint64_t route_base = 63ULL * n;
    gates.clear();
    const auto route_gates = static_cast<std::size_t>(n) * n * width;
    gates.reserve(route_gates);
    for (std::uint32_t row = 0; row < n; ++row)
      for (std::uint32_t original = 0; original < n; ++original)
        for (std::uint32_t bit = 0; bit < width; ++bit) {
          const auto local_permutation = material.permutation_share(row, original);
          const auto gate = route_base +
              (static_cast<std::uint64_t>(row) * n + original) * width + bit;
          gates.push_back({gate, local_permutation, keys[original][bit]});
        }
    const auto routed_terms = engine->and_layer(gates);
    std::vector<std::vector<std::uint8_t>> shuffled(n, std::vector<std::uint8_t>(width, 0));
    for (std::size_t term = 0; term < routed_terms.size(); ++term) {
      const auto bit = static_cast<std::uint32_t>(term % width);
      const auto original = static_cast<std::uint32_t>((term / width) % n);
      const auto row = static_cast<std::uint32_t>(term / (static_cast<std::size_t>(n) * width));
      shuffled[row][bit] ^= routed_terms[term];
      (void)original;
    }

    const auto pair_count = static_cast<std::size_t>(n) * (n - 1U) / 2U;
    const auto gates_per_pair = static_cast<std::uint64_t>(2U * width - 1U);
    const std::uint64_t compare_base = route_base + route_gates;
    std::vector<PairInfo> pairs;
    pairs.reserve(pair_count);
    for (std::uint32_t left = 0; left < n; ++left)
      for (std::uint32_t right = left + 1U; right < n; ++right) {
        const auto pi = pairs.size();
        pairs.push_back({left, right, compare_base + pi * gates_per_pair});
      }
    gates.clear();
    gates.reserve(pair_count * width);
    for (std::size_t pi = 0; pi < pairs.size(); ++pi)
      for (std::uint32_t bit = 0; bit < width; ++bit)
        gates.push_back({pairs[pi].gate_base + bit,
                         not_share(shuffled[pairs[pi].left][bit], party),
                         shuffled[pairs[pi].right][bit]});
    const auto compare_generate_flat = engine->and_layer(gates);
    std::vector<std::vector<std::uint8_t>> compare_generate(
        pair_count, std::vector<std::uint8_t>(width));
    for (std::size_t i = 0; i < compare_generate_flat.size(); ++i)
      compare_generate[i / width][i % width] = compare_generate_flat[i];
    std::vector<std::uint8_t> borrow(pair_count);
    for (std::size_t pi = 0; pi < pair_count; ++pi)
      borrow[pi] = compare_generate[pi][0];
    for (std::uint32_t bit = 1; bit < width; ++bit) {
      gates.clear();
      gates.reserve(pair_count);
      for (std::size_t pi = 0; pi < pairs.size(); ++pi) {
        const auto equal = not_share(xor_bit(
            shuffled[pairs[pi].left][bit], shuffled[pairs[pi].right][bit]), party);
        gates.push_back({pairs[pi].gate_base + width + bit - 1U,
                         equal, borrow[pi]});
      }
      const auto h = engine->and_layer(gates);
      for (std::size_t pi = 0; pi < pairs.size(); ++pi)
        borrow[pi] = xor_bit(compare_generate[pi][bit], h[pi]);
    }
    std::vector<std::vector<std::uint8_t>> compare_table(
        n, std::vector<std::uint8_t>(n, 0));
    for (std::size_t pi = 0; pi < pairs.size(); ++pi) {
      compare_table[pairs[pi].left][pairs[pi].right] = borrow[pi];
      compare_table[pairs[pi].right][pairs[pi].left] = not_share(borrow[pi], party);
    }

    const auto session64 = first_u64(package.parameters.session);
    const auto fingerprint64 = first_u64(package.circuit_sha256);
    const auto sample_source = [&](std::uint8_t iteration,
                                   const std::vector<std::uint32_t>& candidate,
                                   std::uint32_t sample_size) {
      ProjectDerivedSelect5PublicSeed seed{};
      engine->seed(iteration, seed);
      return protocol_i_project_derived_select5_sample_from_public_seed(
          candidate, sample_size, seed, session64, fingerprint64, iteration);
    };
    const auto compare_wave = [&](std::uint8_t iteration, std::uint8_t substage,
                                  const std::vector<ProjectDerivedSelect5Edge>& edges) {
      std::vector<std::uint8_t> local_bits((edges.size() + 7U) / 8U, 0);
      for (std::size_t i = 0; i < edges.size(); ++i) {
        if (edges[i].left >= edges[i].right || edges[i].right >= n)
          fail("v3 active comparison edge is not canonical");
        if (compare_table[edges[i].left][edges[i].right] != 0)
          local_bits[i / 8U] |= static_cast<std::uint8_t>(1U << (i % 8U));
      }
      std::vector<std::uint8_t> body{iteration, substage};
      put_u32(body, static_cast<std::uint32_t>(edges.size()));
      body.insert(body.end(), local_bits.begin(), local_bits.end());
      std::vector<std::uint8_t> remote;
      engine->exchange(kTagOpenedCompare, body, &remote);
      if (remote.size() != body.size() || remote[0] != iteration ||
          remote[1] != substage)
        fail("v3 opened comparison wave shape mismatch");
      std::size_t count_at = 2;
      if (read_u32(remote, count_at) != edges.size())
        fail("v3 opened comparison edge count mismatch");
      std::vector<std::uint8_t> result(edges.size());
      for (std::size_t i = 0; i < edges.size(); ++i) {
        const auto left = (local_bits[i / 8U] >> (i % 8U)) & 1U;
        const auto right = (remote[count_at + i / 8U] >> (i % 8U)) & 1U;
        result[i] = static_cast<std::uint8_t>(left ^ right);
      }
      return result;
    };
    const auto selected = protocol_i_project_derived_select5_control_with_sampler(
        n, package.parameters.k, sample_source, compare_wave);
    if (selected.selected_position >= n) fail("v3 Select5 returned invalid handle");

    std::vector<std::uint8_t> membership_shuffled(n, 0);
    const auto tau = selected.selected_position;
    for (std::uint32_t i = 0; i < n; ++i) {
      if (i == tau) membership_shuffled[i] = public_share(1, party);
      else if (i < tau) membership_shuffled[i] = compare_table[i][tau];
      else membership_shuffled[i] = not_share(compare_table[tau][i], party);
    }
    const std::uint64_t inverse_base = compare_base + pair_count * gates_per_pair;
    gates.clear();
    gates.reserve(static_cast<std::size_t>(n) * n);
    for (std::uint32_t original = 0; original < n; ++original)
      for (std::uint32_t shuffled_index = 0; shuffled_index < n; ++shuffled_index) {
        const auto id = inverse_base + static_cast<std::uint64_t>(original) * n + shuffled_index;
        gates.push_back({id, material.permutation_share(shuffled_index, original),
                         membership_shuffled[shuffled_index]});
      }
    const auto inverse_terms = engine->and_layer(gates);
    std::vector<std::uint8_t> original_mask(n, 0);
    for (std::uint32_t original = 0; original < n; ++original)
      for (std::uint32_t j = 0; j < n; ++j)
        original_mask[original] ^= inverse_terms[static_cast<std::size_t>(original) * n + j];
    if (!engine->all_gates_consumed()) fail("v3 gate schedule left triples unused");

    std::vector<std::uint8_t> local_output(package.parameters.n, 0);
    if (party == 0) {
      std::vector<std::uint8_t> refresh(package.parameters.n, 0);
      if (RAND_bytes(refresh.data(), static_cast<int>(refresh.size())) != 1)
        fail("v3 output-share CSPRNG failed");
      for (std::size_t i = 0; i < refresh.size(); ++i) {
        refresh[i] &= 1U;
        local_output[i] = refresh[i];
      }
      std::vector<std::uint8_t> body{0};
      put_u32(body, static_cast<std::uint32_t>(package.parameters.n));
      for (std::size_t i = 0; i < refresh.size(); ++i)
        body.push_back(static_cast<std::uint8_t>(refresh[i] ^ original_mask[i]));
      std::vector<std::uint8_t> ack;
      engine->exchange(kTagOutputRefresh, body, &ack);
      if (ack != std::vector<std::uint8_t>{1}) fail("v3 output refresh acknowledgement");
    } else {
      std::vector<std::uint8_t> body{1};
      std::vector<std::uint8_t> ack;
      engine->exchange(kTagOutputRefresh, body, &ack);
      if (ack.size() != 5U + package.parameters.n || ack[0] != 0)
        fail("v3 output refresh frame malformed");
      std::size_t at = 1;
      if (read_u32(ack, at) != package.parameters.n)
        fail("v3 output refresh length mismatch");
      for (std::size_t i = 0; i < local_output.size(); ++i) {
        const auto u = ack[at + i];
        if (u > 1 || original_mask[i] > 1) fail("v3 output share is not a bit");
        local_output[i] = static_cast<std::uint8_t>(u ^ original_mask[i]);
      }
    }
    for (const auto bit : local_output) if (bit > 1) fail("v3 output share is not a bit");
    const auto ciphertext = encrypt_share(local_output, package, party,
                                           own_recipient_rsa_public_key_pem);
    const auto expected_transaction = transaction_id(package);
    std::string receipt;
    try {
      receipt = escrow_submitter(expected_transaction, party, ciphertext);
    } catch (const std::exception&) {
      // The request may have reached A and been durably committed even though
      // this process lost the response. Never report ABORT for this ambiguity.
      output.state = ScoreToMaskV3PartyOutput::State::kRecoveryRequired;
      output.transaction_id = expected_transaction;
      output.abort_code = "COMMIT_STATUS_UNKNOWN";
      output.party_channel_bytes_sent = peer_channel.sent_bytes();
      output.party_channel_bytes_received = peer_channel.received_bytes();
      output.party_channel_frames_sent = peer_channel.sent_frames();
      output.party_channel_frames_received = peer_channel.received_frames();
      output.party_channel_phases = static_cast<std::uint16_t>(engine->next_phase() - 1U);
      output.party_channel_max_payload_sent = peer_channel.max_payload_sent();
      output.party_channel_max_payload_received = peer_channel.max_payload_received();
      return output;
    }
    if (receipt.size() != 64 ||
        !std::all_of(receipt.begin(), receipt.end(), [](char c) {
          return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        })) {
      output.state = ScoreToMaskV3PartyOutput::State::kRecoveryRequired;
      output.transaction_id = expected_transaction;
      output.abort_code = "COMMIT_RECEIPT_INVALID";
      output.party_channel_bytes_sent = peer_channel.sent_bytes();
      output.party_channel_bytes_received = peer_channel.received_bytes();
      output.party_channel_frames_sent = peer_channel.sent_frames();
      output.party_channel_frames_received = peer_channel.received_frames();
      output.party_channel_phases = static_cast<std::uint16_t>(engine->next_phase() - 1U);
      output.party_channel_max_payload_sent = peer_channel.max_payload_sent();
      output.party_channel_max_payload_received = peer_channel.max_payload_received();
      return output;
    }
    output.state = ScoreToMaskV3PartyOutput::State::kCommitted;
    output.local_mask_share = std::move(local_output);
    output.transaction_id = expected_transaction;
    output.commit_receipt = receipt;
    output.party_channel_bytes_sent = peer_channel.sent_bytes();
    output.party_channel_bytes_received = peer_channel.received_bytes();
    output.party_channel_frames_sent = peer_channel.sent_frames();
    output.party_channel_frames_received = peer_channel.received_frames();
    output.party_channel_phases = static_cast<std::uint16_t>(engine->next_phase() - 1U);
    output.party_channel_max_payload_sent = peer_channel.max_payload_sent();
    output.party_channel_max_payload_received = peer_channel.max_payload_received();
    return output;
  } catch (const std::exception&) {
    if (engine != nullptr) engine->abort_best_effort();
    output.state = ScoreToMaskV3PartyOutput::State::kAborted;
    output.local_mask_share.clear();
    output.transaction_id.clear();
    output.commit_receipt.clear();
    output.abort_code = "ABORT_V3";
    output.party_channel_bytes_sent = peer_channel.sent_bytes();
    output.party_channel_bytes_received = peer_channel.received_bytes();
    output.party_channel_frames_sent = peer_channel.sent_frames();
    output.party_channel_frames_received = peer_channel.received_frames();
    output.party_channel_phases = engine == nullptr ? 0
        : static_cast<std::uint16_t>(engine->next_phase() - 1U);
    output.party_channel_max_payload_sent = peer_channel.max_payload_sent();
    output.party_channel_max_payload_received = peer_channel.max_payload_received();
    return output;
  }
}

}  // namespace moe_topk
