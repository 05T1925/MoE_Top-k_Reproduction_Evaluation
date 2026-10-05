#include <moe_topk/protocol_iii_aav86_small.h>

#include <FSS/dpf.h>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace moe_topk;

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

ProtocolIIIAav86SmallConfig config(std::uint32_t n, std::uint32_t k,
                                   std::uint32_t r, std::uint8_t party) {
  ProtocolIIIAav86SmallConfig c;
  c.session = 0x730000U + n * 100U + k * 10U + r;
  c.fingerprint = 0x740000U + n * 100U + k * 10U + r;
  c.material_id = 0x750000U + n * 100U + k * 10U + r;
  c.logical_n = n;
  c.k = k;
  c.iterations = r;
  c.party = party;
  c.timeout_ms = 5000;
  c.durable_claim_directory = std::filesystem::temp_directory_path().string();
  return c;
}

template <class F>
void rejects(F&& operation, const char* message) {
  bool rejected = false;
  try { operation(); } catch (const std::exception&) { rejected = true; }
  require(rejected, message);
}
std::uint64_t read_u64(const std::vector<std::uint8_t>& bytes,
                       std::size_t offset) {
  require(offset <= bytes.size() && bytes.size() - offset >= 8U,
          "test package offset");
  std::uint64_t value = 0;
  for (std::size_t i = 0; i < 8U; ++i)
    value = (value << 8U) | bytes[offset + i];
  return value;
}
void flip_low_byte(std::vector<std::uint8_t>& bytes, std::size_t offset) {
  require(offset < bytes.size(), "test mutation offset");
  bytes[offset] ^= 1U;
}

void run_shape(std::uint32_t n, std::uint32_t k, std::uint32_t r) {
  auto c0 = config(n, k, r, 0U);
  auto c1 = config(n, k, r, 1U);
  auto dealer = protocol_iii_aav86_small_dealer_generate(c0);
  const auto bytes0 = protocol_iii_aav86_small_serialize_material(dealer.party0);
  const auto bytes1 = protocol_iii_aav86_small_serialize_material(dealer.party1);
  auto p0 = protocol_iii_aav86_small_deserialize_material(bytes0, 0, c0);
  auto p1 = protocol_iii_aav86_small_deserialize_material(bytes1, 1, c1);
  const auto d = protocol_iii_aav86_small_domain(n);
  const auto rank_bits = protocol_iii_aav86_small_rank_bits(d);
  const auto rank_mask = (UINT64_C(1) << rank_bits) - 1U;
  require(p0.handle_dpf.dpf_keys.size() == d &&
              p1.handle_dpf.dpf_keys.size() == d,
          "one DPF key per hidden handle and party");
  for (std::uint32_t h = 0; h < d; ++h) {
    const auto target = (p0.handle_dpf.rank_mask_shares[h] +
                         p1.handle_dpf.rank_mask_shares[h]) & rank_mask;
    for (std::uint64_t x = 0; x < d; ++x) {
      const auto y0 = evalDPF_Payload(0,
          p0.handle_dpf.dpf_keys[h].native_key(), x);
      const auto y1 = evalDPF_Payload(1,
          p1.handle_dpf.dpf_keys[h].native_key(), x);
      const auto reconstructed = static_cast<std::uint64_t>(y0) +
                                 static_cast<std::uint64_t>(y1);
      require(reconstructed == (x == target ? 1U : 0U),
              "native ring DPF package roundtrip point function");
    }
  }

  auto truncated = bytes0;
  truncated.pop_back();
  rejects([&] {
    (void)protocol_iii_aav86_small_deserialize_material(truncated, 0, c0);
  }, "truncated combined material rejected");
  auto wrong_party = bytes0;
  wrong_party[5] = 1U;
  rejects([&] {
    (void)protocol_iii_aav86_small_deserialize_material(wrong_party, 0, c0);
  }, "wrong-party combined material rejected");
  auto wrong_version = bytes0;
  flip_low_byte(wrong_version, 4U);
  rejects([&] {
    (void)protocol_iii_aav86_small_deserialize_material(wrong_version, 0, c0);
  }, "wrong-version combined material rejected");
  auto swapped_party_package = bytes1;
  rejects([&] {
    (void)protocol_iii_aav86_small_deserialize_material(swapped_party_package, 0, c0);
  }, "swapped-party package rejected");
  auto wrong_session = c0;
  ++wrong_session.session;
  rejects([&] {
    (void)protocol_iii_aav86_small_deserialize_material(bytes0, 0,
                                                        wrong_session);
  }, "wrong-session combined material rejected");
  auto wrong_n = c0;
  wrong_n.logical_n = n == 8U ? 7U : n + 1U;
  rejects([&] {
    (void)protocol_iii_aav86_small_deserialize_material(bytes0, 0, wrong_n);
  }, "wrong-n combined material rejected");
  auto wrong_k = c0;
  wrong_k.k = k == n ? 1U : k + 1U;
  rejects([&] {
    (void)protocol_iii_aav86_small_deserialize_material(bytes0, 0, wrong_k);
  }, "wrong-K combined material rejected");
  auto wrong_r = c0;
  wrong_r.iterations = r == 5U ? 4U : r + 1U;
  rejects([&] {
    (void)protocol_iii_aav86_small_deserialize_material(bytes0, 0, wrong_r);
  }, "wrong-r combined material rejected");
  auto wrong_width = bytes0;
  wrong_width[38] ^= 1U;
  rejects([&] {
    (void)protocol_iii_aav86_small_deserialize_material(wrong_width, 0, c0);
  }, "wrong-width combined material rejected");
  const auto base_length = static_cast<std::size_t>(read_u64(bytes0, 52U));
  const auto mask_records = 64U + base_length;
  const auto dpf_records = mask_records + 20U * d + 4U;
  auto wrong_handle = bytes0;
  flip_low_byte(wrong_handle, mask_records + 3U);
  rejects([&] {
    (void)protocol_iii_aav86_small_deserialize_material(wrong_handle, 0, c0);
  }, "wrong handle label rejected");
  auto wrong_handle_id = bytes0;
  flip_low_byte(wrong_handle_id, mask_records + 11U);
  rejects([&] {
    (void)protocol_iii_aav86_small_deserialize_material(wrong_handle_id, 0, c0);
  }, "wrong handle material ID rejected");
  auto wrong_dpf_width = bytes0;
  flip_low_byte(wrong_dpf_width, dpf_records + 12U);
  rejects([&] {
    (void)protocol_iii_aav86_small_deserialize_material(wrong_dpf_width, 0, c0);
  }, "wrong DPF key width rejected");
  auto wrong_dpf_id = bytes0;
  flip_low_byte(wrong_dpf_id, dpf_records + 11U);
  rejects([&] {
    (void)protocol_iii_aav86_small_deserialize_material(wrong_dpf_id, 0, c0);
  }, "wrong DPF key material ID rejected");
  auto trailing = bytes0;
  trailing.push_back(0U);
  rejects([&] {
    (void)protocol_iii_aav86_small_deserialize_material(trailing, 0, c0);
  }, "trailing material bytes rejected");
  std::cout << "AAV86_III_PACKAGE_CONFORMANCE n=" << n << " k=" << k
            << " r=" << r << " D=" << d << " PASS\n";
}
}  // namespace

int main() {
  try {
    for (const auto n : {2U, 5U, 8U}) {
      for (const auto k : {1U, (n + 1U) / 2U, n}) {
        for (const auto r : {1U, 2U, 3U, 4U, 5U}) run_shape(n, k, r);
      }
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "III+AAV86 conformance failure: " << error.what() << '\n';
    return 1;
  }
}
