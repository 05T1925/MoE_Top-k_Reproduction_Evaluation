#pragma once

#include <moe_topk/metrics.h>

#include <cstdint>
#include <string>
#include <vector>

namespace moe_topk {

inline constexpr const char*
    kProtocolIIIModular3RoundLabel =
        "agarwal_protocol_iii_modular_3round";

inline constexpr const char*
    kProtocolIIIRawScore5RoundLabel =
        "moe_topk_protocol_iii_raw_score_modular_5round";

struct ProtocolIIIMetricsEnvironment {
  std::string git_revision;
  std::string runtime;
  std::string party_topology;

  Measurement<std::string> compiler =
      Measurement<std::string>::not_measured();

  Measurement<std::string> compiler_flags =
      Measurement<std::string>::not_measured();

  Measurement<std::string> build_type =
      Measurement<std::string>::not_measured();

  Measurement<std::string> cpu_model =
      Measurement<std::string>::not_measured();

  Measurement<std::uint64_t> system_memory_bytes =
      Measurement<std::uint64_t>::not_measured();

  Measurement<std::string> operating_system =
      Measurement<std::string>::not_measured();

  Measurement<std::string> network_environment =
      Measurement<std::string>::not_measured();

  Measurement<double> network_bandwidth_mbps =
      Measurement<double>::not_measured();

  Measurement<double> network_rtt_ms =
      Measurement<double>::not_measured();

  std::uint32_t thread_count = 1;
};

struct ProtocolIIIPartyObservation {
  std::string party_label;
  std::uint64_t sent_bytes = 0;
  std::uint64_t received_bytes = 0;
};

struct ProtocolIIIMetricsObservation {
  std::uint64_t n = 0;
  std::uint64_t k = 0;

  Measurement<std::uint64_t> input_seed =
      Measurement<std::uint64_t>::not_measured();

  Measurement<std::string> input_distribution =
      Measurement<std::string>::not_measured();

  Measurement<std::uint64_t> warmup_runs =
      Measurement<std::uint64_t>::not_measured();

  Measurement<std::uint64_t> repetitions =
      Measurement<std::uint64_t>::not_measured();

  Measurement<double> offline_time_ms =
      Measurement<double>::not_measured();

  Measurement<std::uint64_t> offline_material_total_bytes =
      Measurement<std::uint64_t>::not_measured();

  Measurement<double> online_time_ms =
      Measurement<double>::not_measured();

  Measurement<std::uint64_t> online_prg_calls_total =
      Measurement<std::uint64_t>::not_measured();

  Measurement<std::uint64_t> comparison_edges_total =
      Measurement<std::uint64_t>::not_measured();

  std::vector<ProtocolIIIPartyObservation> parties;

  CorrectnessStatus correctness_status =
      CorrectnessStatus::NOT_MEASURED;
};

[[nodiscard]] MetricsRecord
make_protocol_iii_modular_3round_metrics_record(
    const ProtocolIIIMetricsEnvironment& environment,
    const ProtocolIIIMetricsObservation& observation);

[[nodiscard]] MetricsRecord
make_protocol_iii_raw_score_5round_metrics_record(
    const ProtocolIIIMetricsEnvironment& environment,
    const ProtocolIIIMetricsObservation& observation);

[[nodiscard]] std::string
protocol_iii_metrics_record_json(
    const MetricsRecord& record);

}  // namespace moe_topk
