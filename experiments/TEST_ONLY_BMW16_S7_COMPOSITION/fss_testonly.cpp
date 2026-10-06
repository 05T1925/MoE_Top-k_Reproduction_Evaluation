// TEST_ONLY centralized harness for the real repository uCMP/DCF and shuffle.
// It deliberately reconstructs shuffled record identities for the Python
// reference controller; this executable is not a secure protocol endpoint.
#include <FSS/prng.h>
#include <moe_topk/protocol_i_parallel_shuffle.h>
#include <moe_topk/protocol_i_ucmp.h>
#include <moe_topk/topk_oracle.h>

#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace moe_topk;

void seed_fss(std::uint64_t seed) {
  for (std::uint64_t i = 0; i < 256; ++i)
    FSSConfig::prngs[i].SetSeed(osuCrypto::toBlock(seed, i ^ UINT64_C(0x53375337465353)));
}

std::vector<ProtocolIBlock192> add_records(
    int bits, const std::vector<ProtocolIBlock192>& left,
    const std::vector<ProtocolIBlock192>& right) {
  if (left.size() != right.size()) throw std::runtime_error("shuffle share shape");
  auto result = left;
  for (std::size_t i = 0; i < result.size(); ++i)
    result[i] = protocol_i_parallel_record_add(bits, result[i], right[i]);
  return result;
}

void run_shuffle(std::uint32_t n, std::uint64_t seed,
                 const std::vector<std::uint32_t>& scores) {
  if (n < 2 || scores.size() != n) throw std::invalid_argument("shuffle requires n>=2");
  constexpr int bits = 43;
  const ProtocolIParallelShuffleDealerConfig dealer_config{
      seed | 1U, (seed + UINT64_C(0x10001)) | 1U,
      (seed + UINT64_C(0x20001)) | 1U, n, 1, static_cast<std::uint8_t>(bits)};
  auto generated = protocol_i_parallel_shuffle_dealer_generate(dealer_config);
  const auto shuffle_cmpagg_slots = generated.party0.edge_materials.size();
  const auto shuffle_cmpagg_key_wire_bytes = generated.metrics.cmpagg_party_material_wire_bytes;
  std::vector<ProtocolIBlock192> clear(n), input0(n), input1(n);
  std::mt19937_64 rng(seed ^ UINT64_C(0x53485546464c45));
  for (std::uint32_t i = 0; i < n; ++i) {
    clear[i] = {scores[i], i, rng()};
    input0[i] = {rng() & ((UINT64_C(1) << bits) - 1U), rng(), rng()};
    input1[i] = protocol_i_parallel_record_sub(bits, clear[i], input0[i]);
  }
  ProtocolIParallelShufflePartyConfig config0{
      dealer_config.session, dealer_config.fingerprint, dealer_config.material_id,
      n, 1, static_cast<std::uint8_t>(bits), 0, 5000};
  auto config1 = config0;
  config1.party = 1;
  ProtocolIParallelShuffleParty p0(config0, std::move(generated.party0));
  ProtocolIParallelShuffleParty p1(config1, std::move(generated.party1));
  const auto m0 = p0.prepare_round1(input0);
  const auto m1 = p1.prepare_round1(input1);
  const auto d0 = p0.receive_round1_prepare_round2(m1);
  const auto d1 = p1.receive_round1_prepare_round2(m0);
  const auto o0 = p0.receive_round2(d1);
  const auto o1 = p1.receive_round2(d0);
  const auto shuffled = add_records(bits, o0.shuffled_share, o1.shuffled_share);
  // Intentional TEST_ONLY leak: the central adapter opens the carried index
  // lane so the external Select harness can name the shuffled input records.
  std::cout << "S " << n << ' ' << shuffle_cmpagg_slots << ' '
            << shuffle_cmpagg_key_wire_bytes;
  for (const auto& record : shuffled) std::cout << ' ' << record.word1;
  std::cout << '\n' << std::flush;
}

void run_compare(int bits, std::uint64_t left, std::uint64_t right,
                 std::mt19937_64& rng) {
  if (bits < 34 || bits > 53) throw std::invalid_argument("uCMP width must be 34..53");
  const std::uint64_t mask = (UINT64_C(1) << bits) - 1U;
  const std::uint64_t ml = rng() & mask;
  const std::uint64_t mr = rng() & mask;
  ProtocolIUcmpMaterial material(bits, ml, mr);
  const auto c0 = material.eval_strict_lt(0, (left + ml) & mask, (right + mr) & mask);
  const auto c1 = material.eval_strict_lt(1, (left + ml) & mask, (right + mr) & mask);
  // Shares reconstruct in Z/(2^64); the TEST_ONLY orchestrator opens them.
  std::cout << "C " << c0 << ' ' << c1 << '\n' << std::flush;
}

void run_oracle(std::uint32_t n, std::uint32_t k,
                const std::vector<std::uint32_t>& scores) {
  const auto mask = top_k_mask(scores, k);
  std::cout << "O " << n;
  for (const auto bit : mask) std::cout << ' ' << static_cast<unsigned>(bit);
  std::cout << '\n' << std::flush;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc != 2) throw std::invalid_argument("usage: bmw16_s7_fss_testonly <test-seed>");
    const auto seed = std::stoull(argv[1]);
    seed_fss(seed);
    std::mt19937_64 masks(seed ^ UINT64_C(0x55434d504d41534b));
    std::string command;
    while (std::cin >> command) {
      if (command == "S") {
        std::uint32_t n = 0;
        std::uint64_t shuffle_seed = 0;
        if (!(std::cin >> n >> shuffle_seed)) throw std::runtime_error("shuffle request");
        std::vector<std::uint32_t> scores(n);
        for (auto& score : scores) if (!(std::cin >> score)) throw std::runtime_error("shuffle scores");
        run_shuffle(n, shuffle_seed, scores);
        // The real shuffle dealer seeds the repository FSS PRNG from OS
        // entropy.  Reset only the subsequent TEST_ONLY comparator stream so
        // the DCF keygen path is reproducible for this test seed.
        seed_fss(seed);
      } else if (command == "C") {
        int bits = 0;
        std::uint64_t left = 0, right = 0;
        if (!(std::cin >> bits >> left >> right)) throw std::runtime_error("compare request");
        run_compare(bits, left, right, masks);
      } else if (command == "O") {
        std::uint32_t n = 0, k = 0;
        if (!(std::cin >> n >> k)) throw std::runtime_error("oracle request");
        std::vector<std::uint32_t> scores(n);
        for (auto& score : scores) if (!(std::cin >> score)) throw std::runtime_error("oracle scores");
        run_oracle(n, k, scores);
      } else if (command == "Q") {
        std::cout << "Q\n" << std::flush;
        break;
      } else {
        throw std::runtime_error("unknown request token=" + command);
      }
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "BMW16_S7_TEST_ONLY_BACKEND_ERROR: " << error.what() << '\n';
    return 1;
  }
}
