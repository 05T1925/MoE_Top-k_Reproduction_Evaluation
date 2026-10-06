#include <moe_topk/experimental_bmw16_select_party.h>
#include <moe_topk/topk_oracle.h>

#include <array>
#include <cstdint>
#include <exception>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <sys/socket.h>
#include <unistd.h>

namespace {
using namespace moe_topk;

void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }

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
              const ProtocolIBmw16TestOnlyRandomTape* provided_tape=nullptr) {
  std::array<ProtocolIBmw16ExperimentalPartyConfig, 2> configs{
      config_for(test, 0), config_for(test, 1)};
  auto materials = protocol_i_bmw16_experimental_material_generate(configs[0]);
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
  for (int party = 0; party < 2; ++party) {
    parties[party] = std::thread([&, party] {
      try {
        const std::array<int, 2> score_fds{score[0][party], score[1][party]};
        const std::array<int, 2> forward_fds{forward[0][party], forward[1][party]};
        std::array<int, 4> select_fds{}; for (int i = 0; i < 4; ++i) select_fds[i] = select[i][party];
        const std::array<int, 2> inverse_fds{inverse[0][party], inverse[1][party]};
        auto party_material = party == 0 ? std::move(materials.party0) : std::move(materials.party1);
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
  const auto pair_count = static_cast<std::uint64_t>(n) * (n - 1U) / 2U;
  require(result[0].metrics.forward_shuffle_cmpagg_slots_unused == pair_count &&
          result[0].metrics.inverse_shuffle_cmpagg_slots_unused == pair_count,
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
                                                   result[0].metrics.inverse_shuffle_cmpagg_slots_unused << '\n';
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

}  // namespace

int main(int argc,char** argv) {
  try {
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
    run_singleton();
    run_case({{INT32_MIN, INT32_MAX, 0, -1, INT32_MAX}, 1, UINT64_C(0x1101)});
    run_case({{INT32_MIN, INT32_MAX, 0, -1, INT32_MAX}, 3, UINT64_C(0x1102)});
    run_case({{INT32_MIN, INT32_MAX, 0, -1, INT32_MAX}, 5, UINT64_C(0x1103)});
    run_case({{9, 9, 9, 9, 9, 9, 9, 9}, 4, UINT64_C(0x1104)});
    run_case({{6, -7, 4}, 2, UINT64_C(0x1105)});
    run_case({{-8, 3}, 1, UINT64_C(0x1106)});
    run_case({{12, 1, 9, -4, 2}, 2, UINT64_C(0x1107), true});
    std::vector<std::int32_t> scores64;
    for (int i = 0; i < 64; ++i) scores64.push_back((i * 17) % 23 - 11);
    run_case({std::move(scores64), 8, UINT64_C(0x1108)});
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "BMW16 experimental party test failed: " << error.what() << '\n';
    return 1;
  }
}
