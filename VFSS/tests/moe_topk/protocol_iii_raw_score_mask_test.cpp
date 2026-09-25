// Reuse the frozen M3 deterministic fixture generator and clear oracle in
// this test translation unit. The production path below never calls them.
#define main m3_fixture_main_not_run
#include "protocol_iii_raw_score_pipeline_test.cpp"
#undef main

#include <moe_topk/protocol_iii_raw_score_mask.h>
#include <moe_topk/protocol_iii_raw_score_mask_package.h>
#include <moe_topk/protocol_i_priority_key.h>

namespace {

void check_input_and_m3_output_reference(
    const TestCase& test, const RawShares& raw,
    const std::vector<std::uint8_t>& f1_mask) {
  const auto c0 = make_config(test, 0), c1 = make_config(test, 1);
  seed_fss(test.seed ^ UINT64_C(0xA11A));
  std::mt19937_64 input_rng(test.seed ^ UINT64_C(0xA11A));
  auto input_materials = make_materials(c0, input_rng);
  std::array<SocketPair, 2> input_sockets;
  std::array<int, 2> fds0{{input_sockets[0].duplicate(0),input_sockets[1].duplicate(0)}};
  std::array<int, 2> fds1{{input_sockets[0].duplicate(1),input_sockets[1].duplicate(1)}};
  std::vector<std::uint64_t> keys0,keys1;
  std::exception_ptr error0,error1;
  std::thread p0([&] {
    try { keys0 = protocol_i_raw_score_input_party(
        c0.score_input,input_materials.first.score_input_package,raw.party0,fds0,nullptr); }
    catch (...) { error0=std::current_exception(); }
  });
  std::thread p1([&] {
    try { keys1 = protocol_i_raw_score_input_party(
        c1.score_input,input_materials.second.score_input_package,raw.party1,fds1,nullptr); }
    catch (...) { error1=std::current_exception(); }
  });
  p0.join(); p1.join();
  rethrow_if_present(error0); rethrow_if_present(error1);
  const auto key_mask=(UINT64_C(1)<<c0.grank.comparison_bits)-1U;
  require(keys0.size()==c0.grank.padded_n && keys1.size()==keys0.size(),
          "F1 input key count");
  for (std::size_t i=0;i<keys0.size();++i) {
    const auto score = i<test.scores.size() ? test.scores[i] : UINT32_C(0x80000000);
    const auto expected = protocol_i_priority_key(score,i,c0.grank.padded_n).value;
    require(((keys0[i]+keys1[i])&key_mask)==expected,
            "F1/M3 priority-key input differential");
  }
  seed_fss(test.seed ^ UINT64_C(0xA22A));
  std::mt19937_64 reference_rng(test.seed ^ UINT64_C(0xA22A));
  auto materials=make_materials(c0,reference_rng);
  std::array<SocketPair, 5> sockets;
  const auto old_fds0=make_party_fds(sockets,0);
  const auto old_fds1=make_party_fds(sockets,1);
  ProtocolIIIRawScorePipelineOutput a,b;
  error0=nullptr; error1=nullptr;
  std::thread q0([&] {
    try { a=protocol_iii_raw_score_pipeline_party(
        c0,materials.first,raw.party0,old_fds0); }
    catch (...) { error0=std::current_exception(); }
  });
  std::thread q1([&] {
    try { b=protocol_iii_raw_score_pipeline_party(
        c1,materials.second,raw.party1,old_fds1); }
    catch (...) { error1=std::current_exception(); }
  });
  q0.join(); q1.join();
  rethrow_if_present(error0); rethrow_if_present(error1);
  for (std::size_t i=0;i<f1_mask.size();++i)
    require((a.xor_mask_shares[i]^b.xor_mask_shares[i])==f1_mask[i],
            "F1 output differs from frozen M3 original-order mask");
}

void run_f1_case(const TestCase& test) {
  require(test.scores.size() >= 2U, "F1 n");
  const auto old0 = make_config(test, 0);
  const auto old1 = make_config(test, 1);
  ProtocolIIIRawScoreMaskConfig config0{test.session ^ test.fingerprint, old0.score_input, old0.grank, old0.routing};
  ProtocolIIIRawScoreMaskConfig config1{test.session ^ test.fingerprint, old1.score_input, old1.grank, old1.routing};
  seed_fss(test.seed);
  std::mt19937_64 generator(test.seed);
  auto raw = share_raw_scores(test, generator);
  auto old_materials = make_materials(old0, generator);
  ProtocolIIIRawScoreMaskMaterial material0;
  ProtocolIIIRawScoreMaskMaterial material1;
  material0.material_id = config0.material_id;
  material1.material_id = config1.material_id;
  material0.score_input_package = std::move(old_materials.first.score_input_package);
  material0.grank_package = std::move(old_materials.first.grank_package);
  material0.routing_material = std::move(old_materials.first.routing_material);
  material1.score_input_package = std::move(old_materials.second.score_input_package);
  material1.grank_package = std::move(old_materials.second.grank_package);
  material1.routing_material = std::move(old_materials.second.routing_material);
  const auto encoded0 = protocol_iii_raw_score_mask_serialize_bundle(config0, material0);
  const auto encoded1 = protocol_iii_raw_score_mask_serialize_bundle(config1, material1);
  require_invalid_argument([&] {
    (void)protocol_iii_raw_score_mask_deserialize_bundle(config1, encoded0);
  });
  if (test.session == UINT64_C(0xF10000)) {
    auto reject_changed_byte = [&](std::size_t byte) {
      auto damaged = encoded0;
      damaged[byte] ^= 1U;
      require_invalid_argument([&] {
        (void)protocol_iii_raw_score_mask_deserialize_bundle(config0, damaged);
      });
    };
    // Version, party, score/index width, rank width, session, fingerprint,
    // material ID, logical n, padded n, K and section length.
    for (const auto byte : {4U,5U,6U,7U,8U,17U,25U,33U,37U,41U,45U,49U})
      reject_changed_byte(byte);
    auto damaged = encoded0; damaged.pop_back();
    require_invalid_argument([&] {
      (void)protocol_iii_raw_score_mask_deserialize_bundle(config0, damaged);
    });
    damaged = encoded0; damaged.push_back(0U);
    require_invalid_argument([&] {
      (void)protocol_iii_raw_score_mask_deserialize_bundle(config0, damaged);
    });
  }
  if (test.session == UINT64_C(0xF10000)) {
    auto failed = protocol_iii_raw_score_mask_deserialize_bundle(config0, encoded0);
    std::array<std::array<int, 2>, 4> disconnected{};
    for (auto& pair : disconnected) {
      require(::socketpair(AF_UNIX, SOCK_STREAM, 0, pair.data()) == 0,
              "F1 partial-failure socket");
      ::close(pair[1]);
    }
    ProtocolIIIRawScoreMaskFds bad_fds{{disconnected[0][0], disconnected[1][0]},
                                        disconnected[2][0], disconnected[3][0]};
    bool failed_closed = false;
    try {
      (void)protocol_iii_raw_score_mask_party(config0, failed, raw.party0, bad_fds);
    } catch (const std::exception&) { failed_closed = true; }
    require(failed_closed && failed.started, "F1 partial failure did not consume material");
    require_invalid_argument([&] {
      (void)protocol_iii_raw_score_mask_party(config0, failed, raw.party0, bad_fds);
    });
    for (auto& pair : disconnected) ::close(pair[0]);
  }
  material0 = protocol_iii_raw_score_mask_deserialize_bundle(config0, encoded0);
  material1 = protocol_iii_raw_score_mask_deserialize_bundle(config1, encoded1);
  std::array<SocketPair, 4> sockets;
  const auto make_fds = [&](std::size_t party) {
    ProtocolIIIRawScoreMaskFds fds;
    fds.score_input_fds = {{sockets[0].duplicate(party), sockets[1].duplicate(party)}};
    fds.grank_fd = sockets[2].duplicate(party);
    fds.routing_fd = sockets[3].duplicate(party);
    return fds;
  };
  const auto fds0 = make_fds(0);
  const auto fds1 = make_fds(1);
  ProtocolIIIRawScoreMaskOutput output0, output1;
  std::exception_ptr error0, error1;
  std::thread p0([&] {
    try { output0 = protocol_iii_raw_score_mask_party(config0, material0, raw.party0, fds0); }
    catch (...) { error0 = std::current_exception(); }
  });
  std::thread p1([&] {
    try { output1 = protocol_iii_raw_score_mask_party(config1, material1, raw.party1, fds1); }
    catch (...) { error1 = std::current_exception(); }
  });
  p0.join(); p1.join();
  rethrow_if_present(error0); rethrow_if_present(error1);
  const auto oracle = top_k_mask(test.scores, test.k);
  require(output0.xor_mask_shares.size() == test.scores.size() &&
          output1.xor_mask_shares.size() == test.scores.size(), "F1 output length");
  std::size_t selected = 0;
  for (std::size_t i = 0; i < oracle.size(); ++i) {
    require(output0.xor_mask_shares[i] <= 1U && output1.xor_mask_shares[i] <= 1U,
            "F1 output shares are bits");
    const auto bit = static_cast<std::uint8_t>(output0.xor_mask_shares[i] ^
                                               output1.xor_mask_shares[i]);
    require(bit == oracle[i], "F1 raw-score-to-mask oracle mismatch");
    selected += bit;
  }
  require(selected == test.k, "F1 selected count");
  if (test.k==1U && test.scores[0]==test.scores.size()*4096U)
    check_input_and_m3_output_reference(test,raw,oracle);
  for (const auto* output : {&output0, &output1}) {
    require(output->metrics.input_adapter_rounds == 2U &&
            output->metrics.mask_core_rounds == 2U &&
            output->metrics.output_adapter_rounds == 0U &&
            output->metrics.total_online_rounds == 4U, "F1 stage rounds");
    require(output->metrics.routing.eval_calls ==
            static_cast<std::uint64_t>(test.scores.size()) * test.k,
            "F1 DPF Eval count");
    require(output->metrics.total_logical_bits ==
            output->metrics.input_logical_bits + output->metrics.ranking_logical_bits +
            output->metrics.routing_logical_bits, "F1 logical accounting");
  }
  require(output0.metrics.sent_bytes == output1.metrics.received_bytes &&
          output1.metrics.sent_bytes == output0.metrics.received_bytes,
          "F1 wire accounting");
  require(material0.started && material1.started &&
          material0.routing_material.dpf_keys.empty() &&
          material1.routing_material.dpf_keys.empty(), "F1 one-shot consumption");
  require_invalid_argument([&] {
    (void)protocol_iii_raw_score_mask_party(config0, material0, raw.party0, fds0);
  });
}

void test_local_homomorphism() {
  const std::vector<std::uint64_t> a{UINT64_MAX, 18U, 3U, 9U};
  const std::vector<std::uint64_t> b{2U, UINT64_MAX - 17U, UINT64_MAX - 2U, UINT64_MAX - 8U};
  const auto left = protocol_iii_ring_indicators_to_xor_mask(2, 2, a);
  const auto right = protocol_iii_ring_indicators_to_xor_mask(2, 2, b);
  require((left[0] ^ right[0]) == 1U && (left[1] ^ right[1]) == 0U,
          "ring parity homomorphism");
  require_invalid_argument([&] {
    (void)protocol_iii_ring_indicators_to_xor_mask(2, 0, a);
  });
  require_invalid_argument([&] {
    (void)protocol_iii_ring_indicators_to_xor_mask(2, 2, {1U});
  });
}

}  // namespace

int main() {
  try {
    test_local_homomorphism();
    const std::array<std::uint32_t, 5> sizes{{2,3,4,5,8}};
    std::uint64_t counter = 0;
    for (const auto n : sizes) {
      std::vector<std::vector<std::uint32_t>> cases;
      std::vector<std::uint32_t> descending(n), ascending(n), equal(n, 7U), mixed(n);
      for (std::uint32_t i = 0; i < n; ++i) {
        descending[i] = (n - i) * 4096U;
        ascending[i] = (i + 1U) * 4096U;
        mixed[i] = (i % 3U == 0U) ? UINT32_C(0xfffff001) :
                   (i % 3U == 1U) ? 0U : UINT32_C(0x00001001);
      }
      cases.push_back(descending); cases.push_back(ascending);
      cases.push_back(equal); cases.push_back(mixed);
      cases.push_back({UINT32_C(0x80000000), UINT32_C(0x7fffffff)});
      cases.back().resize(n, UINT32_C(0xffffffff));
      cases.push_back({UINT32_C(0x00001000), UINT32_C(0x00001001),
                       UINT32_C(0x00001fff), UINT32_C(0x00001000)});
      cases.back().resize(n, UINT32_C(0xffffefff));
      std::mt19937_64 case_rng(UINT64_C(0xF1A000) + n);
      for (int sample = 0; sample < 4; ++sample) {
        std::vector<std::uint32_t> scores(n);
        for (auto& score : scores) score = static_cast<std::uint32_t>(case_rng());
        cases.push_back(std::move(scores));
      }
      std::vector<std::uint32_t> ks{1U, n};
      if (n > 2U) ks.push_back(n / 2U);
      for (const auto& scores : cases) {
        for (const auto k : ks) {
          run_f1_case({scores, k, UINT64_C(0xF10000) + counter,
                       UINT64_C(0xF20000) + counter, UINT64_C(0xF30000) + counter});
          ++counter;
        }
      }
    }
    std::cout << "M5-FIX-F1 raw-score-to-XOR-mask differential PASS cases="
              << counter << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "M5-FIX-F1 failure: " << error.what() << '\n';
    return 1;
  }
}
