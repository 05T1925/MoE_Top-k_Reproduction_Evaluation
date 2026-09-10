#include <moe_topk/protocol_iii_metrics_record.h>

#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

using namespace moe_topk;

void require(
    bool condition,
    const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

template <typename Function>
void require_throws(
    Function&& function,
    const char* message) {
  bool threw = false;

  try {
    function();
  } catch (const std::invalid_argument&) {
    threw = true;
  }

  require(threw, message);
}

ProtocolIIIMetricsEnvironment
make_environment() {
  ProtocolIIIMetricsEnvironment environment;

  environment.git_revision = "0123456789abcdef";
  environment.runtime = "native-linux-fork-exec";
  environment.party_topology =
      "offline Dealer plus two online Parties";

  environment.compiler =
      Measurement<std::string>::measured(
          "GCC 13.3.0");

  environment.compiler_flags =
      Measurement<std::string>::measured(
          "-O2 -march=native");

  environment.build_type =
      Measurement<std::string>::measured(
          "Release");

  environment.cpu_model =
      Measurement<std::string>::measured(
          "test CPU");

  environment.system_memory_bytes =
      Measurement<std::uint64_t>::measured(
          UINT64_C(8) * 1024U * 1024U * 1024U);

  environment.operating_system =
      Measurement<std::string>::measured(
          "Ubuntu 24.04");

  environment.network_environment =
      Measurement<std::string>::measured(
          "local Unix socketpair");

  environment.network_bandwidth_mbps =
      Measurement<double>::not_measured();

  environment.network_rtt_ms =
      Measurement<double>::not_measured();

  environment.thread_count = 1U;

  return environment;
}

ProtocolIIIMetricsObservation
make_observation() {
  ProtocolIIIMetricsObservation observation;

  observation.n = 129U;
  observation.k = 2U;

  observation.input_seed =
      Measurement<std::uint64_t>::measured(
          UINT64_C(0x12345678));

  observation.input_distribution =
      Measurement<std::string>::measured(
          "deterministic boundaries, ties and uniform words");

  observation.warmup_runs =
      Measurement<std::uint64_t>::measured(0U);

  observation.repetitions =
      Measurement<std::uint64_t>::measured(12U);

  observation.offline_time_ms =
      Measurement<double>::measured(125.5);

  observation.offline_material_total_bytes =
      Measurement<std::uint64_t>::measured(1000U);

  observation.online_time_ms =
      Measurement<double>::measured(74.5);

  observation.online_prg_calls_total =
      Measurement<std::uint64_t>::not_measured();

  observation.comparison_edges_total =
      Measurement<std::uint64_t>::measured(8256U);

  observation.parties = {
      {"P0", 10U, 11U},
      {"P1", 11U, 10U},
  };

  observation.correctness_status =
      CorrectnessStatus::PASSED;

  return observation;
}

void test_modular_three_round_record() {
  const auto record =
      make_protocol_iii_modular_3round_metrics_record(
          make_environment(),
          make_observation());

  require(
      record.implementation_label ==
          kProtocolIIIModular3RoundLabel,
      "three-round implementation label");

  require(
      record.n == 129U &&
          record.K == 2U,
      "three-round dimensions");

  require(
      require_measured(
          record.online_rounds,
          "online_rounds") == 3U,
      "three-round count");

  require(
      require_measured(
          record.offline_material_total_bits,
          "offline_material_total_bits") == 8000U,
      "offline bytes-to-bits conversion");

  require(
      require_measured(
          record.online_comm_total_bits,
          "online_comm_total_bits") == 168U,
      "online total communication");

  require(
      std::abs(
          require_measured(
              record.online_comm_per_party_bits,
              "online_comm_per_party_bits") -
          84.0) < 1e-9,
      "online communication per party");

  require(
      std::abs(
          require_measured(
              record.total_time_ms,
              "total_time_ms") -
          200.0) < 1e-9,
      "total time");

  require(
      require_measured(
          record.comparison_edges_total,
          "comparison_edges_total") == 8256U,
      "logical comparison edges");

  require(
      record.online_prg_calls_total.state ==
          MeasurementState::NOT_MEASURED,
      "unmeasured PRG calls");

  require(
      record.correctness_status ==
          CorrectnessStatus::PASSED,
      "correctness status");

  require(
      record.party_communication.size() == 2U &&
          record.party_communication[0].sent_bits == 80U &&
          record.party_communication[0].received_bits == 88U &&
          record.party_communication[1].sent_bits == 88U &&
          record.party_communication[1].received_bits == 80U,
      "party communication conversion");
}

void test_raw_score_five_round_record() {
  const auto record =
      make_protocol_iii_raw_score_5round_metrics_record(
          make_environment(),
          make_observation());

  require(
      record.implementation_label ==
          kProtocolIIIRawScore5RoundLabel,
      "five-round implementation label");

  require(
      require_measured(
          record.online_rounds,
          "online_rounds") == 5U,
      "five-round count");

  require(
      record.score_interpretation ==
          "signed Q20.12 raw-score arithmetic shares",
      "raw-score interpretation");

  require(
      require_measured(
          record.fixed_point_scale,
          "fixed_point_scale") == 4096U,
      "fixed-point scale");
}

void test_not_measured_time_is_preserved() {
  auto observation = make_observation();

  observation.offline_time_ms =
      Measurement<double>::not_measured();

  observation.online_time_ms =
      Measurement<double>::not_measured();

  const auto record =
      make_protocol_iii_modular_3round_metrics_record(
          make_environment(),
          observation);

  require(
      record.total_time_ms.state ==
          MeasurementState::NOT_MEASURED,
      "unmeasured total time");

  require(
      record.online_comm_total_bits.state ==
          MeasurementState::MEASURED,
      "communication remains measured");
}

void test_not_applicable_offline_material() {
  auto observation = make_observation();

  observation.offline_material_total_bytes =
      Measurement<std::uint64_t>::not_applicable();

  const auto record =
      make_protocol_iii_modular_3round_metrics_record(
          make_environment(),
          observation);

  require(
      record.offline_material_total_bits.state ==
          MeasurementState::NOT_APPLICABLE,
      "not-applicable offline material");
}

void test_json_output() {
  auto environment = make_environment();

  environment.runtime = "native ";
  environment.runtime.push_back('"');
  environment.runtime += "Linux";
  environment.runtime.push_back('"');
  environment.runtime.push_back('\n');
  environment.runtime += "fork-exec";

  const auto record =
      make_protocol_iii_raw_score_5round_metrics_record(
          environment,
          make_observation());

  const auto json =
      protocol_iii_metrics_record_json(record);

  require(
      json.find(
          "\"implementation_label\":"
          "\"moe_topk_protocol_iii_raw_score_modular_5round\"") !=
          std::string::npos,
      "JSON implementation label");

  require(
      json.find(
          "\"online_rounds\":"
          "{\"state\":\"MEASURED\",\"value\":5}") !=
          std::string::npos,
      "JSON online rounds");

  require(
      json.find(
          "\"online_prg_calls_total\":"
          "{\"state\":\"NOT_MEASURED\"}") !=
          std::string::npos,
      "JSON NOT_MEASURED state");

  constexpr char kBackslash =
      static_cast<char>(0x5c);

  std::string escaped_runtime = "native ";
  escaped_runtime.push_back(kBackslash);
  escaped_runtime.push_back('"');
  escaped_runtime += "Linux";
  escaped_runtime.push_back(kBackslash);
  escaped_runtime.push_back('"');
  escaped_runtime.push_back(kBackslash);
  escaped_runtime.push_back('n');
  escaped_runtime += "fork-exec";

  require(
      json.find(escaped_runtime) !=
          std::string::npos,
      "JSON string escaping");

  require(
      json.find('\n') ==
          std::string::npos,
      "JSON contains a raw newline");

  require(
      json.find(
          "\"correctness_status\":\"PASSED\"") !=
          std::string::npos,
      "JSON correctness status");
}
void test_invalid_inputs_are_rejected() {
  require_throws(
      [] {
        auto environment = make_environment();
        environment.git_revision.clear();

        make_protocol_iii_modular_3round_metrics_record(
            environment,
            make_observation());
      },
      "empty revision was accepted");

  require_throws(
      [] {
        auto observation = make_observation();
        observation.k = observation.n + 1U;

        make_protocol_iii_modular_3round_metrics_record(
            make_environment(),
            observation);
      },
      "invalid K was accepted");

  require_throws(
      [] {
        auto observation = make_observation();
        observation.parties.pop_back();

        make_protocol_iii_modular_3round_metrics_record(
            make_environment(),
            observation);
      },
      "single-party observation was accepted");

  require_throws(
      [] {
        auto observation = make_observation();

        observation.offline_material_total_bytes =
            Measurement<std::uint64_t>::measured(
                std::numeric_limits<std::uint64_t>::max());

        make_protocol_iii_modular_3round_metrics_record(
            make_environment(),
            observation);
      },
      "offline bit-count overflow was accepted");

  require_throws(
      [] {
        auto observation = make_observation();

        observation.parties[0].sent_bytes =
            std::numeric_limits<std::uint64_t>::max();

        make_protocol_iii_modular_3round_metrics_record(
            make_environment(),
            observation);
      },
      "party bit-count overflow was accepted");
}

}  // namespace

int main() {
  try {
    test_modular_three_round_record();
    test_raw_score_five_round_record();
    test_not_measured_time_is_preserved();
    test_not_applicable_offline_material();
    test_json_output();
    test_invalid_inputs_are_rejected();

    std::cout
        << "Protocol III MetricsRecord tests passed\n";

    return 0;
  } catch (const std::exception& error) {
    std::cerr
        << "Protocol III MetricsRecord test failure: "
        << error.what()
        << '\n';

    return 1;
  }
}
