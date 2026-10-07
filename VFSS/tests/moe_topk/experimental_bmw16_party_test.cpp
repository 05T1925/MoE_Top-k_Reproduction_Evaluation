#include <moe_topk/experimental_bmw16_select_party.h>
#include <moe_topk/experimental_bmw16_material_bundle.h>
#include <moe_topk/experimental_bmw16_stream_store.h>
#include <moe_topk/topk_oracle.h>
#include <FSS/dcf.h>
#include <FSS/prng.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <exception>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
using namespace moe_topk;

void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }

void verify_dcf_prg_counter_contract() {
#if defined(MOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS)
  constexpr int bin = 7;
  FSSConfig::prngs[0].SetSeed(osuCrypto::toBlock(0, UINT64_C(0x53463135)));
  resetDcfPrgCallCounts();
  auto keys = keyGenDCF(bin, 1, UINT64_C(3), UINT64_C(1));
  auto after_keygen = getDcfPrgCallCounts();
  require(after_keygen.keygen_calls == 1, "DCF keygen call counter");
  require(after_keygen.keygen_node_expansions == 2U * bin, "DCF keygen node expansion counter");
  require(after_keygen.eval_calls == 0 && after_keygen.eval_node_expansions == 0,
          "DCF keygen must not count Eval expansions");
  GroupElement out0 = 0, out1 = 0;
  evalDCF(0, &out0, 0, keys.first);
  evalDCF(1, &out1, 0, keys.second);
  const auto counts = getDcfPrgCallCounts();
  require(counts.keygen_calls == 1 && counts.keygen_node_expansions == 2U * bin,
          "DCF counter keygen fields changed during Eval");
  require(counts.eval_calls == 2 && counts.eval_node_expansions == 2U * bin,
          "DCF Eval node expansion counter");
#endif
}

struct Case { std::vector<std::int32_t> scores; std::uint32_t k; std::uint64_t seed; bool force_abort = false; };

ProtocolIBmw16TestOnlyRandomTape read_common_tape(const std::string& path) {
  std::ifstream in(path);
  require(static_cast<bool>(in), "open common random tape");
  std::string magic;
  in >> magic;
  require(magic == "BMW16_S12_TAPE_V1", "common random tape version");
  ProtocolIBmw16TestOnlyRandomTape tape;
  for (auto& task : tape.words) for (auto& domain : task) {
    std::size_t count = 0;
    in >> count;
    require(static_cast<bool>(in) && count <= 1000000U, "common random tape word count");
    domain.resize(count);
    for (auto& word : domain) in >> word;
    require(static_cast<bool>(in), "common random tape word data");
  }
  std::string trailing;
  require(!(in >> trailing), "common random tape trailing data");
  return tape;
}

ProtocolIBmw16ExperimentalPartyConfig config_for(const Case& test, int party) {
  const auto n = static_cast<std::uint32_t>(test.scores.size());
  std::uint32_t padded = 2; while (padded < n) padded <<= 1U;
  std::uint8_t index_bits = 1;
  for (auto x = padded - 1U; x > 1U; x >>= 1U) ++index_bits;
  const auto session = UINT64_C(0x5341310000000000) + (test.seed << 8U) +
      (static_cast<std::uint64_t>(test.k) << 4U) + n;
  return {session, session ^ UINT64_C(0x5341318000000000),
          n, test.k, padded, index_bits, static_cast<std::uint8_t>(33U + index_bits),
          static_cast<std::uint8_t>(party), 15000, test.seed, test.force_abort};
}

std::array<int, 2> socket_pair() {
  int fds[2] = {-1, -1}; require(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0, "socketpair");
  return {fds[0], fds[1]};
}

void run_case(const Case& test,int tape_pattern=0,
              const ProtocolIBmw16TestOnlyRandomTape* provided_tape=nullptr,
              bool use_stream_store=false) {
  std::array<ProtocolIBmw16ExperimentalPartyConfig, 2> configs{
      config_for(test, 0), config_for(test, 1)};
  if (use_stream_store) {
    // Preserve the input/algorithm tape while giving this independently
    // generated material a fresh protocol session identity.
    for (auto& c : configs) {
      c.session ^= UINT64_C(0x5331375354524541);
      c.fingerprint = c.session ^ UINT64_C(0x533137465052494e);
    }
  }
  std::filesystem::path stream_root;
  std::array<std::shared_ptr<ProtocolIBmw16StreamedUcmpSlotStore>, 2> stream_stores;
  if (use_stream_store) {
    stream_root = std::filesystem::temp_directory_path() /
        ("bmw16-s19-stream-" + std::to_string(::getpid()) + "-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
         std::to_string(configs[0].session));
    std::filesystem::create_directories(stream_root / "p0");
    std::filesystem::create_directories(stream_root / "p1");
    std::filesystem::permissions(stream_root / "p0", std::filesystem::perms::owner_all);
    std::filesystem::permissions(stream_root / "p1", std::filesystem::perms::owner_all);
    for (int party = 0; party < 2; ++party) {
      const auto root_id = protocol_i_bmw16_claim_root_identity((stream_root / (party ? "p1" : "p0")).string());
      configs[party].claim_root_device = root_id.device;
      configs[party].claim_root_inode = root_id.inode;
    }
  }
  auto materials = protocol_i_bmw16_experimental_material_generate(configs[0]);
  std::array<ProtocolIBmw16StreamStoreStats, 2> stream_stats{};
  if (use_stream_store) {
    std::array<std::array<std::uint8_t, 32>, 2> recipient_keys{};
    for (std::size_t party = 0; party < 2; ++party) {
      for (std::size_t i = 0; i < recipient_keys[party].size(); ++i)
        recipient_keys[party][i] = static_cast<std::uint8_t>((configs[party].session >> (i % 8U)) ^ (party * 71U + i));
      const auto path = (stream_root / (party ? "p1" : "p0") / "ucmp.pool").string();
      const auto& material = party ? materials.party1 : materials.party0;
      protocol_i_bmw16_stream_store_seal_party(path, configs[party], static_cast<std::uint8_t>(party),
          material, recipient_keys[party], &stream_stats[party]);
      stream_stores[party] = ProtocolIBmw16StreamedUcmpSlotStore::open_party(
          path, configs[party], static_cast<std::uint8_t>(party), recipient_keys[party]);
      require(stream_stats[party].ucmp_slots == 9U * (test.scores.size() * (test.scores.size() - 1U) / 2U) &&
              stream_stats[party].sealed_file_bytes == stream_stores[party]->file_bytes(),
              "stream store size accounting");
    }
    const auto n = static_cast<std::uint32_t>(test.scores.size());
    const auto first_id = static_cast<std::uint64_t>(2U) * configs[0].padded_n + 2U;
    const ProtocolIBmw16MaterialSlot first_slot{first_id, ProtocolIBmw16MaterialStage::Select, 0, 1, 0, 1};
    auto byte_audit = ProtocolIBmw16StreamedUcmpSlotStore::open_party(
        (stream_root / "p0" / "ucmp.pool").string(), configs[0], 0,
        [&] { std::array<std::uint8_t, 32> k{}; for (std::size_t i=0;i<k.size();++i) k[i]=static_cast<std::uint8_t>((configs[0].session>>(i%8U))^(i)); return k; }());
    std::uint64_t expected_id = first_id;
    auto require_same_slot_bytes = [&](const ProtocolIBmw16MaterialSlot& slot,
                                       const ProtocolIUcmpPartyMaterial& source) {
      require(slot.id == expected_id++, "stream sequential ID audit");
      const auto streamed = byte_audit->load_once(slot).serialize();
      require(streamed == source.serialize(), "streamed key bytes differ from in-memory source");
    };
    const auto pairs = static_cast<std::size_t>(n) * (n - 1U) / 2U;
    for (std::uint8_t task = 0; task < 2; ++task) for (std::uint8_t round = 1; round <= 4; ++round) {
      const auto& keys = materials.party0.select_keys[task][round - 1U];
      require(keys.size() == pairs, "stream byte audit source pool size");
      std::size_t pair = 0;
      for (std::uint32_t left = 0; left < n; ++left) for (std::uint32_t right = left + 1U; right < n; ++right)
        require_same_slot_bytes({expected_id, ProtocolIBmw16MaterialStage::Select, task, round, left, right}, keys[pair++]);
    }
    for (std::uint32_t left = 0; left < n; ++left) for (std::uint32_t right = left + 1U; right < n; ++right) {
      const auto pair = static_cast<std::size_t>(left) * n - static_cast<std::size_t>(left) * (left + 1U) / 2U + right - left - 1U;
      require_same_slot_bytes({expected_id, ProtocolIBmw16MaterialStage::Membership, 0xff, 1, left, right},
                              materials.party0.membership_keys[pair]);
    }
    require(expected_id == first_id + stream_stats[0].ucmp_slots, "stream byte audit slot count");
    auto duplicate_probe = ProtocolIBmw16StreamedUcmpSlotStore::open_party(
        (stream_root / "p0" / "ucmp.pool").string(), configs[0], 0,
        [&] { std::array<std::uint8_t, 32> k{}; for (std::size_t i=0;i<k.size();++i) k[i]=static_cast<std::uint8_t>((configs[0].session>>(i%8U))^(i)); return k; }());
    require(n >= 2, "stream duplicate fixture n");
    auto wrong_endpoint = first_slot;
    wrong_endpoint.left = 1;
    wrong_endpoint.right = 2;
    bool endpoint_rejected = false;
    try { (void)duplicate_probe->load_once(wrong_endpoint); }
    catch (const std::exception&) { endpoint_rejected = true; }
    require(endpoint_rejected, "stream wrong endpoint binding rejected");
    bool wrong_party_rejected = false;
    try { (void)ProtocolIBmw16StreamedUcmpSlotStore::open_party(
        (stream_root / "p0" / "ucmp.pool").string(), configs[0], 1,
        [&] { std::array<std::uint8_t, 32> k{}; for (std::size_t i=0;i<k.size();++i) k[i]=static_cast<std::uint8_t>((configs[0].session>>(i%8U))^(i)); return k; }()); }
    catch (const std::exception&) { wrong_party_rejected = true; }
    require(wrong_party_rejected, "stream wrong party rejected");
    (void)duplicate_probe->load_once(first_slot);
    bool duplicate_rejected = false;
    try { (void)duplicate_probe->load_once(first_slot); }
    catch (const std::exception&) { duplicate_rejected = true; }
    require(duplicate_rejected, "stream duplicate slot read rejected");
    auto wrong_root = configs[0]; wrong_root.claim_root_inode ^= 1U;
    bool root_rejected = false;
    try { (void)ProtocolIBmw16StreamedUcmpSlotStore::open_party(
        (stream_root / "p0" / "ucmp.pool").string(), wrong_root, 0,
        [&] { std::array<std::uint8_t, 32> k{}; for (std::size_t i=0;i<k.size();++i) k[i]=static_cast<std::uint8_t>((configs[0].session>>(i%8U))^(i)); return k; }()); }
    catch (const std::exception&) { root_rejected = true; }
    require(root_rejected, "stream wrong claim-root binding rejected");
    const auto tampered_path = (stream_root / "p0" / "tampered.pool").string();
    std::filesystem::copy_file(stream_root / "p0" / "ucmp.pool", tampered_path);
    {
      std::fstream f(tampered_path, std::ios::binary | std::ios::in | std::ios::out);
      require(static_cast<bool>(f), "open stream tamper fixture");
      constexpr std::streamoff stream_header_bytes = 164;
      f.seekg(stream_header_bytes, std::ios::beg); char byte = 0; f.read(&byte, 1); byte ^= 0x40;
      f.seekp(stream_header_bytes, std::ios::beg); f.write(&byte, 1); f.flush();
      require(static_cast<bool>(f), "write stream tamper fixture");
    }
    auto tampered_store = ProtocolIBmw16StreamedUcmpSlotStore::open_party(tampered_path, configs[0], 0,
        [&] { std::array<std::uint8_t, 32> k{}; for (std::size_t i=0;i<k.size();++i) k[i]=static_cast<std::uint8_t>((configs[0].session>>(i%8U))^(i)); return k; }());
    bool tamper_rejected = false;
    try { (void)tampered_store->load_once(first_slot); }
    catch (const std::exception&) { tamper_rejected = true; }
    require(tamper_rejected, "stream corrupted record rejected by AEAD");
    const auto truncated_path = (stream_root / "p0" / "truncated.pool").string();
    std::filesystem::copy_file(stream_root / "p0" / "ucmp.pool", truncated_path);
    std::filesystem::resize_file(truncated_path, std::filesystem::file_size(truncated_path) - 1U);
    bool truncation_rejected = false;
    try { (void)ProtocolIBmw16StreamedUcmpSlotStore::open_party(truncated_path, configs[0], 0,
        [&] { std::array<std::uint8_t, 32> k{}; for (std::size_t i=0;i<k.size();++i) k[i]=static_cast<std::uint8_t>((configs[0].session>>(i%8U))^(i)); return k; }()); }
    catch (const std::exception&) { truncation_rejected = true; }
    require(truncation_rejected, "stream truncated file rejected");
  }
  ProtocolIBmw16TestOnlyRandomTape tape; std::vector<std::string> trace_lines; tape.trace_lines=&trace_lines;
  const auto handle_to_original=protocol_i_compose_permutation(
      materials.party0.forward_shuffle.tau,materials.party1.forward_shuffle.sigma);
  std::string map_line="TEST_ONLY_HANDLE_TO_ORIGINAL=";
  for(std::size_t i=0;i<handle_to_original.size();++i){if(i)map_line+=',';map_line+=std::to_string(handle_to_original[i]);}
  trace_lines.push_back(map_line);
  std::mt19937_64 tape_rng(test.seed ^ UINT64_C(0x636f6d6d6f6e7461));
  std::uint64_t tape_index=0;
  for(auto& task:tape.words)for(auto& domain:task){domain.resize(1024);for(auto& word:domain){
    if(tape_pattern==1)word=0;
    else if(tape_pattern==2)word=UINT64_MAX;
    else if(tape_pattern==3)word=(tape_index++&1U)?UINT64_MAX:0;
    else word=tape_rng();
  }}
  if(provided_tape)tape.words=provided_tape->words;
  const auto n = static_cast<std::uint32_t>(test.scores.size());
  std::mt19937_64 rng(test.seed ^ UINT64_C(0x7261777368617265));
  std::array<std::vector<std::uint32_t>, 2> shares;
  for (auto score : test.scores) {
    const auto word = static_cast<std::uint32_t>(score);
    const auto s0 = static_cast<std::uint32_t>(rng());
    shares[0].push_back(s0); shares[1].push_back(word - s0);
  }

  std::array<std::array<int, 2>, 2> score{}, forward{}, inverse{};
  std::array<std::array<int, 2>, 4> select{};
  std::array<int, 2> final{};
  for (int i = 0; i < 2; ++i) { score[i] = socket_pair(); forward[i] = socket_pair(); inverse[i] = socket_pair(); }
  for (auto& fds : select) fds = socket_pair();
  final = socket_pair();

  std::array<ProtocolIBmw16ExperimentalPartyOutput, 2> result;
  std::array<std::exception_ptr, 2> errors{};
  std::array<std::thread, 2> parties;
  // Isolate online DCF expansion counts from the fresh offline material generation.
  resetDcfPrgCallCounts();
  for (int party = 0; party < 2; ++party) {
    parties[party] = std::thread([&, party] {
      try {
        const std::array<int, 2> score_fds{score[0][party], score[1][party]};
        const std::array<int, 2> forward_fds{forward[0][party], forward[1][party]};
        std::array<int, 4> select_fds{}; for (int i = 0; i < 4; ++i) select_fds[i] = select[i][party];
        const std::array<int, 2> inverse_fds{inverse[0][party], inverse[1][party]};
        auto party_material = party == 0 ? std::move(materials.party0) : std::move(materials.party1);
        if (use_stream_store) {
          auto& store = stream_stores[party];
          party_material.slot_manifest.clear();
          for (auto& task : party_material.select_keys) for (auto& round : task) round.clear();
          party_material.membership_keys.clear();
          party_material.load_ucmp_slot = [store](const ProtocolIBmw16MaterialSlot& slot) {
            return store->load_once(slot);
          };
        }
        auto process_slots = std::make_shared<ProtocolIBmw16ProcessSlotClaimSet>();
        party_material.process_slot_claim_once = [process_slots](std::uint64_t id) {
          process_slots->claim_once(id);
        };
        result[party] = protocol_i_bmw16_experimental_raw_score_mask_party(
            configs[party], std::move(party_material), shares[party], score_fds,
            forward_fds, select_fds, inverse_fds, final[party], -1, &tape);
      } catch (const std::exception& error) {
        std::cerr << "party=" << party << " n=" << configs[party].n << " failed: " << error.what() << '\n';
        errors[party] = std::current_exception();
      } catch (...) { errors[party] = std::current_exception(); }
    });
  }
  for (auto& party : parties) party.join();
  for (const auto& error : errors) if (error) std::rethrow_exception(error);
  const auto online_dcf_counts = getDcfPrgCallCounts();
  if (online_dcf_counts.enabled && n > 1) {
    const auto eval_calls = result[0].metrics.dcf_party_evaluations +
                            result[1].metrics.dcf_party_evaluations;
    require(online_dcf_counts.eval_calls == eval_calls,
            "online DCF call counter does not match both parties");
    const auto raw_eval_calls = UINT64_C(8) * configs[0].padded_n;
    require(eval_calls >= raw_eval_calls,
            "online DCF call counter is smaller than raw-score adapter work");
    const auto expected_expansions = raw_eval_calls * 34U +
        (eval_calls - raw_eval_calls) * configs[0].comparison_bits;
    require(online_dcf_counts.eval_node_expansions == expected_expansions,
            "online DCF expansion count does not match raw/Select widths");
  }

  require(std::string(result[0].status) == result[1].status, "party status disagreement");
  if (std::string(result[0].status) == "SUCCESS") {
    require(result[0].xor_mask_share.size() == n && result[1].xor_mask_share.size() == n,
            "success mask dimensions");
    std::vector<std::uint8_t> mask(n);
    for (std::size_t i = 0; i < n; ++i) mask[i] = result[0].xor_mask_share[i] ^ result[1].xor_mask_share[i];
    require(mask == top_k_mask([&] {
      std::vector<std::uint32_t> words; for (auto score : test.scores) words.push_back(static_cast<std::uint32_t>(score));
      return words;
    }(), test.k), "frozen oracle mismatch");
    require(std::accumulate(mask.begin(), mask.end(), 0U) == test.k, "mask cardinality");
  } else {
    require(result[0].xor_mask_share.empty() && result[1].xor_mask_share.empty(), "abort returned mask");
    require(result[0].abort_reason == result[1].abort_reason && !result[0].abort_reason.empty(),
            "abort reason agreement");
  }
  const auto logical = std::accumulate(result[0].metrics.logical_comparison_calls.begin(),
      result[0].metrics.logical_comparison_calls.end(), UINT64_C(0));
  const auto select_slots = std::accumulate(result[0].metrics.unique_select_slots_consumed.begin(),
      result[0].metrics.unique_select_slots_consumed.end(), UINT64_C(0));
  const auto expected_score_slots = 2U * configs[0].padded_n;
  const auto expected_total_ucmp = expected_score_slots + select_slots +
      (std::string(result[0].status) == "SUCCESS" ? n - 1U : 0U);
  require(result[0].metrics.ucmp_party_evaluations == expected_total_ucmp,
          "uCMP slot accounting identity");
  require(result[0].metrics.dcf_party_evaluations == 2U * expected_total_ucmp,
          "DCF Eval accounting identity");
  const auto expected_process_claims = 2U * configs[0].padded_n + 1U + select_slots +
      (std::string(result[0].status) == "SUCCESS" ? n : 0U);
  require(result[0].metrics.process_slots_claimed == expected_process_claims,
          "process-local one-shot claim accounting");
  const auto pair_count = static_cast<std::uint64_t>(n) * (n - 1U) / 2U;
  require(result[0].metrics.forward_shuffle_cmpagg_slots_unused == 0 &&
          result[0].metrics.inverse_shuffle_cmpagg_slots_unused == 0 &&
          result[0].metrics.shuffle_cmpagg_slots_omitted == 2U * pair_count,
          "unused shuffle CmpAgg slot accounting");
  std::cout << "case n=" << n << " K=" << test.k << " seed=" << test.seed
            << " raw_share_seed=" << (test.seed ^ UINT64_C(0x7261777368617265))
            << " status=" << result[0].status << " select_logical=" << logical
            << " injected_abort=" << (test.force_abort ? "true" : "false")
            << " abort_reason=" << result[0].abort_reason
            << " select_unique_slots=" << select_slots
            << " membership_slots=" << result[0].metrics.membership_slots_consumed
            << " logical_rounds=" << result[0].metrics.logical_comparison_calls[0] << ','
            << result[0].metrics.logical_comparison_calls[1] << ','
            << result[0].metrics.logical_comparison_calls[2] << ','
            << result[0].metrics.logical_comparison_calls[3]
            << " dummy_calls=" << result[0].metrics.dummy_related_calls[0] << ','
            << result[0].metrics.dummy_related_calls[1] << ','
            << result[0].metrics.dummy_related_calls[2] << ','
            << result[0].metrics.dummy_related_calls[3]
            << " repeats=" << result[0].metrics.repeated_logical_calls[0] << ','
            << result[0].metrics.repeated_logical_calls[1] << ','
            << result[0].metrics.repeated_logical_calls[2] << ','
            << result[0].metrics.repeated_logical_calls[3]
            << " edge_fnv64=" << std::hex << result[0].metrics.edge_plan_fnv64[0] << ','
            << result[0].metrics.edge_plan_fnv64[1] << ','
            << result[0].metrics.edge_plan_fnv64[2] << ','
            << result[0].metrics.edge_plan_fnv64[3] << std::dec
            << " ucmp_party_evals=" << result[0].metrics.ucmp_party_evaluations
            << " dcf_party_evals=" << result[0].metrics.dcf_party_evaluations
            << " message_phases=" << result[0].metrics.online_message_phases
            << " sent_bytes=" << result[0].metrics.online_bytes_sent
            << " recv_bytes=" << result[0].metrics.online_bytes_received
            << " offline_key_slots_per_party=" << materials.offline_material_slots_per_party[0]
            << " primitive_key_and_shuffle_bytes_per_party=" << materials.primitive_key_and_shuffle_bytes_per_party[0]
            << " unused_shuffle_cmpagg=" << result[0].metrics.forward_shuffle_cmpagg_slots_unused +
                                                   result[0].metrics.inverse_shuffle_cmpagg_slots_unused
            << " omitted_shuffle_cmpagg=" << result[0].metrics.shuffle_cmpagg_slots_omitted
            << " online_us_p0=" << result[0].metrics.online_time_us
            << " online_us_p1=" << result[1].metrics.online_time_us
            << " sampler_prf_words_total=" << result[0].metrics.sampler_prf_words +
                                                result[1].metrics.sampler_prf_words
            << " dcf_counter_enabled=" << (online_dcf_counts.enabled ? 1 : 0)
            << " online_dcf_eval_calls_total=" << (online_dcf_counts.enabled ?
                   std::to_string(online_dcf_counts.eval_calls) : "NOT_MEASURED")
            << " online_prg_calls_total=" << (online_dcf_counts.enabled ?
                   std::to_string(online_dcf_counts.eval_node_expansions) : "NOT_MEASURED")
            << " online_aes_blocks_total=" << (online_dcf_counts.enabled ?
                   std::to_string(2U * online_dcf_counts.eval_node_expansions) : "NOT_MEASURED")
            << " streamed_ucmp=" << (use_stream_store ? 1 : 0)
            << " stream_slots=" << (use_stream_store ? stream_stats[0].ucmp_slots : 0)
            << " stream_file_bytes_p0=" << (use_stream_store ? stream_stats[0].sealed_file_bytes : 0)
            << " stream_file_bytes_p1=" << (use_stream_store ? stream_stats[1].sealed_file_bytes : 0) << '\n';
  if(const char* dir=std::getenv("BMW16_COMMON_TAPE_TRACE_DIR")){
    std::filesystem::create_directories(dir);
    std::ofstream tape_out(std::filesystem::path(dir)/"common_tape.json",std::ios::trunc);
    tape_out<<"{\"n\":"<<n<<",\"k\":"<<test.k<<",\"scores\":[";
    for(std::size_t i=0;i<test.scores.size();++i){if(i)tape_out<<',';tape_out<<test.scores[i];}
    tape_out<<"],\"words\":[";
    for(std::size_t t=0;t<2;++t){if(t)tape_out<<',';tape_out<<'[';for(std::size_t d=0;d<2;++d){if(d)tape_out<<',';tape_out<<'[';for(std::size_t i=0;i<tape.words[t][d].size();++i){if(i)tape_out<<',';tape_out<<tape.words[t][d][i];}tape_out<<']';}tape_out<<']';}
    tape_out<<"]}\n";tape_out.close();
    std::ofstream trace_out(std::filesystem::path(dir)/"cpp_trace.txt",std::ios::trunc);
    for(const auto& line:trace_lines)trace_out<<line<<'\n';
  }
  for (auto& pair : score) for (auto fd : pair) ::close(fd);
  for (auto& pair : forward) for (auto fd : pair) ::close(fd);
  for (auto& pair : inverse) for (auto fd : pair) ::close(fd);
  for (auto& row : select) for (auto fd : row) ::close(fd);
  for (auto fd : final) ::close(fd);
}

void run_singleton() {
  for (int party = 0; party < 2; ++party) {
    ProtocolIBmw16ExperimentalPartyConfig c{UINT64_C(0x5341310000000001),
        UINT64_C(0x5341318000000001), 1, 1, 2, 1, 34,
        static_cast<std::uint8_t>(party), 1000, 7};
    ProtocolIBmw16ExperimentalPartyMaterial m;
    const auto result = protocol_i_bmw16_experimental_raw_score_mask_party(
        c, std::move(m), {UINT32_C(0x80000000)}, {-1, -1}, {-1, -1},
        {-1, -1, -1, -1}, {-1, -1}, -1);
    require(std::string(result.status) == "SUCCESS" && result.xor_mask_share.size() == 1,
            "singleton success");
    require((result.xor_mask_share[0] == 1U) == (party == 0), "singleton xor share");
  }
  std::cout << "case n=1 K=1 status=SUCCESS singleton=true\n";
}

void verify_streamed_keygen_slot_equivalence() {
  constexpr std::uint32_t n = 5, k = 2;
  ProtocolIBmw16ExperimentalPartyConfig c0{UINT64_C(0x5331380000000011),
      UINT64_C(0x5331388000000011), n, k, 8, 3, 36, 0, 5000, 0};
  ProtocolIBmw16ExperimentalPartyConfig c1 = c0; c1.party = 1;
  const auto root = std::filesystem::temp_directory_path() /
      ("bmw16-s18-slot-equivalence-" + std::to_string(::getpid()));
  std::filesystem::create_directories(root / "p0");
  std::filesystem::create_directories(root / "p1");
  for (const auto& dir : {root / "p0", root / "p1"})
    std::filesystem::permissions(dir, std::filesystem::perms::owner_all);
  auto cleanup = std::unique_ptr<void, std::function<void(void*)>>(
      reinterpret_cast<void*>(1), [root](void*) {
        std::error_code ec; std::filesystem::remove_all(root, ec);
      });
  for (int party = 0; party < 2; ++party) {
    auto& cfg = party ? c1 : c0;
    const auto id = protocol_i_bmw16_claim_root_identity((root / (party ? "p1" : "p0")).string());
    cfg.claim_root_device = id.device; cfg.claim_root_inode = id.inode;
  }
  std::array<std::uint8_t, 32> stream_id{};
  for (std::size_t i = 0; i < stream_id.size(); ++i) stream_id[i] = static_cast<std::uint8_t>(0x51U + i);
  std::array<std::array<std::uint8_t, 32>, 2> recipient_keys{};
  for (int party = 0; party < 2; ++party)
    for (std::size_t i = 0; i < recipient_keys[party].size(); ++i)
      recipient_keys[party][i] = static_cast<std::uint8_t>(0xA0U + party * 17U + i);
  const auto path0 = (root / "p0" / "slots.bin").string();
  const auto path1 = (root / "p1" / "slots.bin").string();
  ProtocolIBmw16StreamedUcmpSlotWriter writer0(path0, c0, 0, recipient_keys[0], stream_id);
  ProtocolIBmw16StreamedUcmpSlotWriter writer1(path1, c1, 1, recipient_keys[1], stream_id);
  // This TEST_ONLY tee uses the very same fresh key object both for retained
  // memory material and for each sealed slot, so byte equality is meaningful.
  auto material = protocol_i_bmw16_test_only_material_generate_with_slot_sink(c0,
      [&](const ProtocolIBmw16MaterialSlot& slot, const ProtocolIUcmpPartyMaterial& key0,
          const ProtocolIUcmpPartyMaterial& key1) {
        writer0.append(slot, key0); writer1.append(slot, key1);
      });
  const auto stats0 = writer0.finalize(), stats1 = writer1.finalize();
  require(stats0.ucmp_slots == 9U * (n * (n - 1U) / 2U) &&
          stats0.ucmp_slots == stats1.ucmp_slots &&
          stats0.sealed_file_bytes == stats1.sealed_file_bytes,
          "stream writer completion counts");
  const auto store0 = ProtocolIBmw16StreamedUcmpSlotStore::open_party(
      path0, c0, 0, recipient_keys[0], stream_id, stats0.file_sha256);
  const auto store1 = ProtocolIBmw16StreamedUcmpSlotStore::open_party(
      path1, c1, 1, recipient_keys[1], stream_id, stats1.file_sha256);
  std::uint64_t id = 2U * c0.padded_n + 2U;
  auto check_pool = [&](const ProtocolIBmw16ExperimentalPartyMaterial& p,
                        ProtocolIBmw16StreamedUcmpSlotStore& store) {
    for (std::uint8_t task = 0; task < 2; ++task) for (std::uint8_t round = 1; round <= 4; ++round) {
      const auto& pool = p.select_keys[task][round - 1U]; std::size_t pair = 0;
      for (std::uint32_t left = 0; left < n; ++left) for (std::uint32_t right = left + 1; right < n; ++right) {
        const ProtocolIBmw16MaterialSlot slot{id++, ProtocolIBmw16MaterialStage::Select, task, round, left, right};
        require(store.load_once(slot).serialize() == pool[pair++].serialize(),
                "streamed Select slot differs from same-KeyGen memory slot");
      }
    }
    for (std::uint32_t left = 0; left < n; ++left) for (std::uint32_t right = left + 1; right < n; ++right) {
      const auto pair = static_cast<std::size_t>(left) * n - static_cast<std::size_t>(left) * (left + 1U) / 2U + right - left - 1U;
      const ProtocolIBmw16MaterialSlot slot{id++, ProtocolIBmw16MaterialStage::Membership, 0xff, 1, left, right};
      require(store.load_once(slot).serialize() == p.membership_keys[pair].serialize(),
              "streamed membership slot differs from same-KeyGen memory slot");
    }
  };
  check_pool(material.party0, *store0);
  require(id == 2U * c0.padded_n + 2U + stats0.ucmp_slots, "stream writer slot enumeration");
  // A distinct path/store validates the second party's independently sealed keys.
  id = 2U * c0.padded_n + 2U;
  check_pool(material.party1, *store1);
  require(id == 2U * c1.padded_n + 2U + stats1.ucmp_slots, "stream writer party-1 slot enumeration");
  const auto bad_hash_path = (root / "p0" / "tampered.bin").string();
  std::filesystem::copy_file(path0, bad_hash_path);
  {
    std::fstream f(bad_hash_path, std::ios::binary | std::ios::in | std::ios::out);
    require(static_cast<bool>(f), "stream tamper file open");
    constexpr std::streamoff header_bytes = 164;
    f.seekg(header_bytes); char b = 0; f.read(&b, 1); b ^= 1;
    f.seekp(header_bytes); f.write(&b, 1); f.flush(); require(static_cast<bool>(f), "stream tamper write");
  }
  bool hash_rejected = false;
  try { (void)ProtocolIBmw16StreamedUcmpSlotStore::open_party(
      bad_hash_path, c0, 0, recipient_keys[0], stream_id, stats0.file_sha256); }
  catch (const std::exception&) { hash_rejected = true; }
  require(hash_rejected, "whole-sidecar digest rejects modified ciphertext");
  const auto truncated_path = (root / "p0" / "truncated.bin").string();
  std::filesystem::copy_file(path0, truncated_path);
  std::filesystem::resize_file(truncated_path, std::filesystem::file_size(truncated_path) - 1U);
  bool truncation_rejected = false;
  try { (void)ProtocolIBmw16StreamedUcmpSlotStore::open_party(
      truncated_path, c0, 0, recipient_keys[0], stream_id, stats0.file_sha256); }
  catch (const std::exception&) { truncation_rejected = true; }
  require(truncation_rejected, "truncated stream file rejected");
  const auto incomplete_path = (root / "p0" / "incomplete.bin").string();
  const auto incomplete_pending = incomplete_path + ".pending." + std::to_string(::getpid());
  bool incomplete_finalize_rejected = false;
  {
    ProtocolIBmw16StreamedUcmpSlotWriter incomplete(incomplete_path, c0, 0,
        recipient_keys[0], stream_id);
    const ProtocolIBmw16MaterialSlot first{
        2U * c0.padded_n + 2U, ProtocolIBmw16MaterialStage::Select, 0, 1, 0, 1};
    incomplete.append(first, material.party0.select_keys[0][0][0]);
    try { (void)incomplete.finalize(); }
    catch (const std::logic_error&) { incomplete_finalize_rejected = true; }
  }
  require(incomplete_finalize_rejected && !std::filesystem::exists(incomplete_path) &&
          !std::filesystem::exists(incomplete_pending),
          "incomplete stream never publishes a claimable file and cleans pending file");
  std::cout << "stream_keygen_same_slot_equivalence slots_per_party=" << stats0.ucmp_slots
            << " sidecar_bytes=" << stats0.sealed_file_bytes << " file_hash_bound=true\n";
}

void verify_persistent_claim_survives_process_exit() {
  const auto root = std::filesystem::temp_directory_path() /
      ("bmw16-s18-claim-restart-" + std::to_string(::getpid()));
  std::filesystem::create_directories(root);
  std::filesystem::permissions(root, std::filesystem::perms::owner_all);
  auto cleanup = std::unique_ptr<void, std::function<void(void*)>>(
      reinterpret_cast<void*>(1), [root](void*) {
        std::error_code ec; std::filesystem::remove_all(root, ec);
      });
  const auto identity = protocol_i_bmw16_claim_root_identity(root.string());
  constexpr std::uint64_t session = UINT64_C(0x5331385253544152);
  std::array<std::uint8_t, 32> digest{};
  for (std::size_t i = 0; i < digest.size(); ++i)
    digest[i] = static_cast<std::uint8_t>(0xA5U ^ i);
  const auto child = ::fork();
  require(child >= 0, "claim restart fork");
  if (child == 0) {
    try {
      ProtocolIBmw16PersistentClaimStore store(root.string(), session, 0, identity);
      store.claim_bundle(digest);
      ::_exit(0);  // simulate process death after the durable claim
    } catch (...) {
      ::_exit(1);
    }
  }
  int status = 0;
  require(::waitpid(child, &status, 0) == child && WIFEXITED(status) &&
          WEXITSTATUS(status) == 0, "child durable claim before simulated crash");
  bool replay_rejected = false;
  try {
    ProtocolIBmw16PersistentClaimStore restarted(root.string(), session, 0, identity);
    restarted.claim_bundle(digest);
  } catch (const std::exception&) {
    replay_rejected = true;
  }
  require(replay_rejected, "bundle claim must reject after process exit/reopen");
  std::cout << "persistent_claim_restart_replay_rejected=true\n";
}

void verify_concurrent_bundle_claim_single_winner() {
  const auto root = std::filesystem::temp_directory_path() /
      ("bmw16-s18-claim-race-" + std::to_string(::getpid()));
  std::filesystem::create_directories(root);
  std::filesystem::permissions(root, std::filesystem::perms::owner_all);
  auto cleanup = std::unique_ptr<void, std::function<void(void*)>>(
      reinterpret_cast<void*>(1), [root](void*) {
        std::error_code ec; std::filesystem::remove_all(root, ec);
      });
  const auto identity = protocol_i_bmw16_claim_root_identity(root.string());
  constexpr std::uint64_t session = UINT64_C(0x533138434C41494D);
  std::array<std::uint8_t, 32> digest{};
  for (std::size_t i = 0; i < digest.size(); ++i)
    digest[i] = static_cast<std::uint8_t>(0x3CU + i);

  int ready_pipe[2]{};
  int start_pipe[2]{};
  int result_pipe[2]{};
  require(::pipe(ready_pipe) == 0 && ::pipe(start_pipe) == 0 && ::pipe(result_pipe) == 0,
          "claim-race pipes");
  const auto child = ::fork();
  require(child >= 0, "claim-race fork");
  if (child == 0) {
    ::close(ready_pipe[0]); ::close(start_pipe[1]); ::close(result_pipe[0]);
    std::uint8_t result = 3;
    try {
      ProtocolIBmw16PersistentClaimStore store(root.string(), session, 0, identity);
      const std::uint8_t ready = 1;
      (void)::write(ready_pipe[1], &ready, sizeof(ready));
      std::uint8_t start = 0;
      if (::read(start_pipe[0], &start, sizeof(start)) != sizeof(start) || start != 1)
        ::_exit(4);
      try {
        store.claim_bundle(digest);
        result = 1;
      } catch (const std::exception& e) {
        result = std::string(e.what()) == "BMW16 persistent claim replay" ? 2 : 3;
      }
      (void)::write(result_pipe[1], &result, sizeof(result));
      ::_exit(result == 1 || result == 2 ? 0 : 5);
    } catch (...) {
      (void)::write(result_pipe[1], &result, sizeof(result));
      ::_exit(6);
    }
  }

  ::close(ready_pipe[1]); ::close(start_pipe[0]); ::close(result_pipe[1]);
  std::uint8_t ready = 0;
  require(::read(ready_pipe[0], &ready, sizeof(ready)) == sizeof(ready) && ready == 1,
          "claim-race child ready");
  const std::uint8_t start = 1;
  require(::write(start_pipe[1], &start, sizeof(start)) == sizeof(start), "claim-race release");
  std::uint8_t parent_result = 3;
  try {
    ProtocolIBmw16PersistentClaimStore store(root.string(), session, 0, identity);
    store.claim_bundle(digest);
    parent_result = 1;
  } catch (const std::exception& e) {
    parent_result = std::string(e.what()) == "BMW16 persistent claim replay" ? 2 : 3;
  }
  std::uint8_t child_result = 3;
  require(::read(result_pipe[0], &child_result, sizeof(child_result)) == sizeof(child_result),
          "claim-race child result");
  int status = 0;
  require(::waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0,
          "claim-race child exit");
  ::close(ready_pipe[0]); ::close(start_pipe[1]); ::close(result_pipe[0]);
  require(parent_result <= 2 && child_result <= 2 &&
          (static_cast<unsigned>(parent_result == 1) + static_cast<unsigned>(child_result == 1) == 1U),
          "concurrent durable bundle claim has exactly one winner");
  std::cout << "concurrent_bundle_claim_single_winner=true parent="
            << static_cast<unsigned>(parent_result) << " child="
            << static_cast<unsigned>(child_result) << "\n";
}

void run_missing_process_slot_guard() {
  const ProtocolIBmw16ExperimentalPartyConfig config{
      UINT64_C(0x5331340000000001), UINT64_C(0x5331348000000001),
      2, 1, 2, 1, 34, 0, 1000, 7, false};
  auto pair=protocol_i_bmw16_experimental_material_generate(config);
  ProtocolIBmw16TestOnlyRandomTape tape;
  const auto result=protocol_i_bmw16_experimental_raw_score_mask_party(
      config,std::move(pair.party0),{0,0},{-1,-1},{-1,-1},{-1,-1,-1,-1},{-1,-1},-1,-1,&tape);
  require(std::string(result.status)=="ABORT_MATERIAL"&&
          std::string(result.abort_scope)=="LOCAL_ONLY"&&result.xor_mask_share.empty(),
          "missing process-local one-shot guard must fail closed");
}

}  // namespace

int main(int argc,char** argv) {
  try {
    verify_dcf_prg_counter_contract();
    if(argc>=8&&std::string(argv[1])=="--trace-case-tape"){
      const auto n=static_cast<std::size_t>(std::stoul(argv[4]));
      const auto k=static_cast<std::uint32_t>(std::stoul(argv[5]));
      const auto seed=std::stoull(argv[6]);
      require(argc==7+static_cast<int>(n),"trace-case-tape score count");
      auto tape=read_common_tape(argv[3]);
      std::vector<std::int32_t> scores;for(std::size_t i=0;i<n;++i)scores.push_back(static_cast<std::int32_t>(std::stoll(argv[7+i])));
      if(::setenv("BMW16_COMMON_TAPE_TRACE_DIR",argv[2],1)!=0)throw std::runtime_error("set trace output directory");
      run_case({std::move(scores),k,seed},0,&tape);return 0;
    }
    if(argc>=7&&std::string(argv[1])=="--trace-case-pattern"){
      const std::string pattern=argv[3];const int mode=pattern=="zero"?1:pattern=="max"?2:pattern=="alternating"?3:0;require(mode!=0,"unknown tape pattern");
      const auto n=static_cast<std::size_t>(std::stoul(argv[4]));const auto k=static_cast<std::uint32_t>(std::stoul(argv[5]));const auto seed=std::stoull(argv[6]);
      require(argc==7+static_cast<int>(n),"trace-case score count");std::vector<std::int32_t> scores;for(std::size_t i=0;i<n;++i)scores.push_back(static_cast<std::int32_t>(std::stoll(argv[7+i])));
      if(::setenv("BMW16_COMMON_TAPE_TRACE_DIR",argv[2],1)!=0)throw std::runtime_error("set trace output directory");
      run_case({std::move(scores),k,seed},mode);return 0;
    }
    if(argc>=7&&std::string(argv[1])=="--counter-bench-pattern"){
      const std::string pattern=argv[2];const int mode=pattern=="zero"?1:pattern=="max"?2:pattern=="alternating"?3:0;require(mode!=0,"unknown counter-bench tape pattern");
      const auto n=static_cast<std::size_t>(std::stoul(argv[3]));const auto k=static_cast<std::uint32_t>(std::stoul(argv[4]));const auto seed=std::stoull(argv[5]);
      require(argc==6+static_cast<int>(n),"counter-bench score count");std::vector<std::int32_t> scores;for(std::size_t i=0;i<n;++i)scores.push_back(static_cast<std::int32_t>(std::stoll(argv[6+i])));
      run_case({std::move(scores),k,seed},mode);return 0;
    }
    if(argc>=6&&std::string(argv[1])=="--trace-case"){
      const auto n=static_cast<std::size_t>(std::stoul(argv[3]));const auto k=static_cast<std::uint32_t>(std::stoul(argv[4]));const auto seed=std::stoull(argv[5]);
      require(argc==6+static_cast<int>(n),"trace-case score count");std::vector<std::int32_t> scores;for(std::size_t i=0;i<n;++i)scores.push_back(static_cast<std::int32_t>(std::stoll(argv[6+i])));
      if(::setenv("BMW16_COMMON_TAPE_TRACE_DIR",argv[2],1)!=0)throw std::runtime_error("set trace output directory");
      run_case({std::move(scores),k,seed});return 0;
    }
    if(argc==2){
      if(::setenv("BMW16_COMMON_TAPE_TRACE_DIR",argv[1],1)!=0)throw std::runtime_error("set trace output directory");
      run_case({{INT32_MIN,INT32_MAX,0,-1,INT32_MAX,7,7,-22},3,UINT64_C(0x1212)});
      return 0;
    }
    require(argc==1,"unexpected test args");
    run_missing_process_slot_guard();
    run_singleton();
    verify_streamed_keygen_slot_equivalence();
    verify_concurrent_bundle_claim_single_winner();
    verify_persistent_claim_survives_process_exit();
    run_case({{INT32_MIN, INT32_MAX, 0, -1, INT32_MAX}, 1, UINT64_C(0x1101)});
    run_case({{INT32_MIN, INT32_MAX, 0, -1, INT32_MAX}, 3, UINT64_C(0x1102)});
    run_case({{INT32_MIN, INT32_MAX, 0, -1, INT32_MAX}, 5, UINT64_C(0x1103)});
    run_case({{9, 9, 9, 9, 9, 9, 9, 9}, 4, UINT64_C(0x1104)});
    run_case({{6, -7, 4}, 2, UINT64_C(0x1105)});
    run_case({{-8, 3}, 1, UINT64_C(0x1106)});
    run_case({{12, 1, 9, -4, 2}, 2, UINT64_C(0x1107), true});
    const Case stream_equivalence_case{{INT32_MIN, 13, -5, INT32_MAX, 13}, 2, UINT64_C(0x1717)};
    // Same frozen input and deterministic TEST_ONLY algorithm tape. Offline
    // material is fresh per run; the separate exhaustive key-byte audit above
    // checks that sidecar serialization is exact for its source pool.
    run_case(stream_equivalence_case);
    run_case(stream_equivalence_case, 0, nullptr, true);
    std::vector<std::int32_t> scores64;
    for (int i = 0; i < 64; ++i) scores64.push_back((i * 17) % 23 - 11);
    run_case({std::move(scores64), 8, UINT64_C(0x1108)});
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "BMW16 experimental party test failed: " << error.what() << '\n';
    return 1;
  }
}
