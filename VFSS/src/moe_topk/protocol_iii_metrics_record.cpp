#include <moe_topk/protocol_iii_metrics_record.h>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace moe_topk {
namespace {

void require(
    bool condition,
    const char* message) {
  if (!condition) {
    throw std::invalid_argument(message);
  }
}

std::uint64_t checked_bytes_to_bits(
    std::uint64_t bytes,
    const char* message) {
  require(
      bytes <=
          std::numeric_limits<std::uint64_t>::max() / 8U,
      message);

  return bytes * 8U;
}

std::uint64_t checked_add(
    std::uint64_t left,
    std::uint64_t right,
    const char* message) {
  require(
      right <=
          std::numeric_limits<std::uint64_t>::max() - left,
      message);

  return left + right;
}

template <typename T>
void validate_measurement(
    const Measurement<T>& measurement,
    const char* message) {
  if (measurement.state == MeasurementState::MEASURED) {
    require(
        measurement.value.has_value(),
        message);

    if constexpr (std::is_same_v<T, std::string>) {
      require(
          !measurement.value->empty(),
          message);
    }

    if constexpr (std::is_floating_point_v<T>) {
      require(
          std::isfinite(*measurement.value),
          message);
    }
  } else {
    require(
        !measurement.value.has_value(),
        message);
  }
}

Measurement<std::uint64_t>
bytes_measurement_to_bits(
    const Measurement<std::uint64_t>& bytes) {
  validate_measurement(
      bytes,
      "invalid offline-material measurement");

  if (bytes.state == MeasurementState::MEASURED) {
    return Measurement<std::uint64_t>::measured(
        checked_bytes_to_bits(
            *bytes.value,
            "offline-material bit count overflow"));
  }

  if (bytes.state ==
      MeasurementState::NOT_APPLICABLE) {
    return Measurement<std::uint64_t>::not_applicable();
  }

  return Measurement<std::uint64_t>::not_measured();
}

void validate_environment(
    const ProtocolIIIMetricsEnvironment& environment) {
  require(
      !environment.git_revision.empty(),
      "Protocol III metrics git revision");

  require(
      !environment.runtime.empty(),
      "Protocol III metrics runtime");

  require(
      !environment.party_topology.empty(),
      "Protocol III metrics party topology");

  require(
      environment.thread_count != 0U,
      "Protocol III metrics thread count");

  validate_measurement(
      environment.compiler,
      "Protocol III metrics compiler");

  validate_measurement(
      environment.compiler_flags,
      "Protocol III metrics compiler flags");

  validate_measurement(
      environment.build_type,
      "Protocol III metrics build type");

  validate_measurement(
      environment.cpu_model,
      "Protocol III metrics CPU model");

  validate_measurement(
      environment.system_memory_bytes,
      "Protocol III metrics system memory");

  validate_measurement(
      environment.operating_system,
      "Protocol III metrics operating system");

  validate_measurement(
      environment.network_environment,
      "Protocol III metrics network environment");

  validate_measurement(
      environment.network_bandwidth_mbps,
      "Protocol III metrics network bandwidth");

  validate_measurement(
      environment.network_rtt_ms,
      "Protocol III metrics network RTT");

  if (environment.network_bandwidth_mbps.state ==
      MeasurementState::MEASURED) {
    require(
        *environment.network_bandwidth_mbps.value >= 0.0,
        "Protocol III metrics negative network bandwidth");
  }

  if (environment.network_rtt_ms.state ==
      MeasurementState::MEASURED) {
    require(
        *environment.network_rtt_ms.value >= 0.0,
        "Protocol III metrics negative network RTT");
  }
}

void validate_observation(
    const ProtocolIIIMetricsObservation& observation) {
  require(
      observation.n != 0U &&
          observation.k != 0U &&
          observation.k <= observation.n,
      "Protocol III metrics dimensions");

  require(
      observation.parties.size() == 2U,
      "Protocol III metrics requires two online parties");

  require(
      !observation.parties[0].party_label.empty() &&
          !observation.parties[1].party_label.empty() &&
          observation.parties[0].party_label !=
              observation.parties[1].party_label,
      "Protocol III metrics party labels");

  validate_measurement(
      observation.input_seed,
      "Protocol III metrics input seed");

  validate_measurement(
      observation.input_distribution,
      "Protocol III metrics input distribution");

  validate_measurement(
      observation.warmup_runs,
      "Protocol III metrics warmup runs");

  validate_measurement(
      observation.repetitions,
      "Protocol III metrics repetitions");

  validate_measurement(
      observation.offline_time_ms,
      "Protocol III metrics offline time");

  validate_measurement(
      observation.offline_material_total_bytes,
      "Protocol III metrics offline material");

  validate_measurement(
      observation.online_time_ms,
      "Protocol III metrics online time");

  validate_measurement(
      observation.online_prg_calls_total,
      "Protocol III metrics PRG calls");

  validate_measurement(
      observation.comparison_edges_total,
      "Protocol III metrics comparison edges");

  if (observation.offline_time_ms.state ==
      MeasurementState::MEASURED) {
    require(
        *observation.offline_time_ms.value >= 0.0,
        "Protocol III metrics negative offline time");
  }

  if (observation.online_time_ms.state ==
      MeasurementState::MEASURED) {
    require(
        *observation.online_time_ms.value >= 0.0,
        "Protocol III metrics negative online time");
  }
}

MetricsRecord make_record(
    const char* implementation_label,
    const char* score_interpretation,
    std::uint64_t online_rounds,
    const ProtocolIIIMetricsEnvironment& environment,
    const ProtocolIIIMetricsObservation& observation) {
  validate_environment(environment);
  validate_observation(observation);

  require(
      implementation_label != nullptr &&
          implementation_label[0] != '\0',
      "Protocol III implementation label");

  require(
      score_interpretation != nullptr &&
          score_interpretation[0] != '\0',
      "Protocol III score interpretation");

  MetricsRecord record;

  record.implementation_label =
      implementation_label;

  record.git_revision =
      environment.git_revision;

  record.runtime =
      environment.runtime;

  record.party_topology =
      environment.party_topology;

  record.n = observation.n;
  record.K = observation.k;

  record.input_seed =
      observation.input_seed;

  record.input_distribution =
      observation.input_distribution;

  record.compiler =
      environment.compiler;

  record.compiler_flags =
      environment.compiler_flags;

  record.build_type =
      environment.build_type;

  record.cpu_model =
      environment.cpu_model;

  record.system_memory_bytes =
      environment.system_memory_bytes;

  record.operating_system =
      environment.operating_system;

  record.network_environment =
      environment.network_environment;

  record.network_bandwidth_mbps =
      environment.network_bandwidth_mbps;

  record.network_rtt_ms =
      environment.network_rtt_ms;

  record.warmup_runs =
      observation.warmup_runs;

  record.repetitions =
      observation.repetitions;

  record.score_bit_width = 32U;
  record.score_interpretation =
      score_interpretation;

  record.fixed_point_scale =
      Measurement<std::uint32_t>::measured(4096U);

  record.thread_count =
      environment.thread_count;

  record.offline_time_ms =
      observation.offline_time_ms;

  record.offline_material_total_bits =
      bytes_measurement_to_bits(
          observation.offline_material_total_bytes);

  record.online_time_ms =
      observation.online_time_ms;

  record.online_party_count = 2U;

  std::uint64_t total_sent_bits = 0;

  for (const auto& party :
       observation.parties) {
    const auto sent_bits =
        checked_bytes_to_bits(
            party.sent_bytes,
            "Protocol III sent-bit count overflow");

    const auto received_bits =
        checked_bytes_to_bits(
            party.received_bytes,
            "Protocol III received-bit count overflow");

    total_sent_bits =
        checked_add(
            total_sent_bits,
            sent_bits,
            "Protocol III total communication overflow");

    record.party_communication.push_back(
        {
            party.party_label,
            sent_bits,
            received_bits,
        });
  }

  record.online_comm_total_bits =
      Measurement<std::uint64_t>::measured(
          total_sent_bits);

  record.online_comm_per_party_bits =
      Measurement<double>::measured(
          static_cast<double>(total_sent_bits) /
          static_cast<double>(
              record.online_party_count));

  record.online_rounds =
      Measurement<std::uint64_t>::measured(
          online_rounds);

  record.online_prg_calls_total =
      observation.online_prg_calls_total;

  record.comparison_edges_total =
      observation.comparison_edges_total;

  record.correctness_status =
      observation.correctness_status;

  if (record.offline_time_ms.state ==
          MeasurementState::MEASURED &&
      record.online_time_ms.state ==
          MeasurementState::MEASURED) {
    record.total_time_ms =
        Measurement<double>::measured(
            *record.offline_time_ms.value +
            *record.online_time_ms.value);
  } else {
    record.total_time_ms =
        Measurement<double>::not_measured();
  }

  return record;
}

const char* measurement_state_name(
    MeasurementState state) {
  switch (state) {
    case MeasurementState::MEASURED:
      return "MEASURED";

    case MeasurementState::NOT_MEASURED:
      return "NOT_MEASURED";

    case MeasurementState::NOT_APPLICABLE:
      return "NOT_APPLICABLE";
  }

  throw std::invalid_argument(
      "unknown measurement state");
}

const char* correctness_status_name(
    CorrectnessStatus status) {
  switch (status) {
    case CorrectnessStatus::PASSED:
      return "PASSED";

    case CorrectnessStatus::FAILED:
      return "FAILED";

    case CorrectnessStatus::NOT_MEASURED:
      return "NOT_MEASURED";
  }

  throw std::invalid_argument(
      "unknown correctness status");
}

std::string json_escape(
    const std::string& value) {
  std::ostringstream output;

  constexpr char kBackslash =
      static_cast<char>(0x5c);

  for (const unsigned char character : value) {
    switch (character) {
      case '"':
        output.put(kBackslash);
        output.put('"');
        break;

      case 0x5c:
        output.put(kBackslash);
        output.put(kBackslash);
        break;

      case '\b':
        output.put(kBackslash);
        output.put('b');
        break;

      case '\f':
        output.put(kBackslash);
        output.put('f');
        break;

      case '\n':
        output.put(kBackslash);
        output.put('n');
        break;

      case '\r':
        output.put(kBackslash);
        output.put('r');
        break;

      case '\t':
        output.put(kBackslash);
        output.put('t');
        break;

      default:
        if (character < 0x20U) {
          output.put(kBackslash);
          output.put('u');

          output
              << std::hex
              << std::setw(4)
              << std::setfill('0')
              << static_cast<unsigned int>(character)
              << std::dec
              << std::setfill(' ');
        } else {
          output.put(
              static_cast<char>(character));
        }
    }
  }

  return output.str();
}

void append_json_string(
    std::ostringstream& output,
    const std::string& value) {
  output
      << '"'
      << json_escape(value)
      << '"';
}

template <typename T>
void append_json_value(
    std::ostringstream& output,
    const T& value) {
  if constexpr (std::is_same_v<T, std::string>) {
    append_json_string(output, value);
  } else if constexpr (std::is_floating_point_v<T>) {
    output
        << std::setprecision(17)
        << value;
  } else {
    output << value;
  }
}

template <typename T>
void append_measurement(
    std::ostringstream& output,
    const Measurement<T>& measurement) {
  validate_measurement(
      measurement,
      "invalid measurement during JSON encoding");

  output << "{\"state\":";
  append_json_string(
      output,
      measurement_state_name(
          measurement.state));

  if (measurement.state ==
      MeasurementState::MEASURED) {
    output << ",\"value\":";
    append_json_value(
        output,
        *measurement.value);
  }

  output << '}';
}

}  // namespace

MetricsRecord
make_protocol_iii_modular_3round_metrics_record(
    const ProtocolIIIMetricsEnvironment& environment,
    const ProtocolIIIMetricsObservation& observation) {
  return make_record(
      kProtocolIIIModular3RoundLabel,
      "signed Q20.12 score encoded as stable priority-key shares",
      3U,
      environment,
      observation);
}

MetricsRecord
make_protocol_iii_raw_score_5round_metrics_record(
    const ProtocolIIIMetricsEnvironment& environment,
    const ProtocolIIIMetricsObservation& observation) {
  return make_record(
      kProtocolIIIRawScore5RoundLabel,
      "signed Q20.12 raw-score arithmetic shares",
      5U,
      environment,
      observation);
}

std::string protocol_iii_metrics_record_json(
    const MetricsRecord& record) {
  require(
      !record.implementation_label.empty() &&
          !record.git_revision.empty() &&
          !record.runtime.empty() &&
          !record.party_topology.empty() &&
          record.n != 0U &&
          record.K != 0U &&
          record.K <= record.n,
      "invalid Protocol III MetricsRecord");

  std::ostringstream output;

  output << '{';

  output << "\"implementation_label\":";
  append_json_string(
      output,
      record.implementation_label);

  output << ",\"git_revision\":";
  append_json_string(
      output,
      record.git_revision);

  output << ",\"runtime\":";
  append_json_string(
      output,
      record.runtime);

  output << ",\"party_topology\":";
  append_json_string(
      output,
      record.party_topology);

  output << ",\"n\":" << record.n;
  output << ",\"K\":" << record.K;

  output << ",\"input_seed\":";
  append_measurement(
      output,
      record.input_seed);

  output << ",\"input_distribution\":";
  append_measurement(
      output,
      record.input_distribution);

  output << ",\"compiler\":";
  append_measurement(
      output,
      record.compiler);

  output << ",\"compiler_flags\":";
  append_measurement(
      output,
      record.compiler_flags);

  output << ",\"build_type\":";
  append_measurement(
      output,
      record.build_type);

  output << ",\"cpu_model\":";
  append_measurement(
      output,
      record.cpu_model);

  output << ",\"system_memory_bytes\":";
  append_measurement(
      output,
      record.system_memory_bytes);

  output << ",\"operating_system\":";
  append_measurement(
      output,
      record.operating_system);

  output << ",\"network_environment\":";
  append_measurement(
      output,
      record.network_environment);

  output << ",\"network_bandwidth_mbps\":";
  append_measurement(
      output,
      record.network_bandwidth_mbps);

  output << ",\"network_rtt_ms\":";
  append_measurement(
      output,
      record.network_rtt_ms);

  output << ",\"warmup_runs\":";
  append_measurement(
      output,
      record.warmup_runs);

  output << ",\"repetitions\":";
  append_measurement(
      output,
      record.repetitions);

  output << ",\"aav86_iterations_r\":";
  append_measurement(
      output,
      record.aav86_iterations_r);

  output
      << ",\"score_bit_width\":"
      << record.score_bit_width;

  output << ",\"score_interpretation\":";
  append_json_string(
      output,
      record.score_interpretation);

  output << ",\"fixed_point_scale\":";
  append_measurement(
      output,
      record.fixed_point_scale);

  output
      << ",\"thread_count\":"
      << record.thread_count;

  output << ",\"offline_time_ms\":";
  append_measurement(
      output,
      record.offline_time_ms);

  output << ",\"offline_material_total_bits\":";
  append_measurement(
      output,
      record.offline_material_total_bits);

  output << ",\"online_time_ms\":";
  append_measurement(
      output,
      record.online_time_ms);

  output << ",\"online_comm_total_bits\":";
  append_measurement(
      output,
      record.online_comm_total_bits);

  output << ",\"online_comm_per_party_bits\":";
  append_measurement(
      output,
      record.online_comm_per_party_bits);

  output
      << ",\"online_party_count\":"
      << record.online_party_count;

  output << ",\"party_communication\":[";

  for (std::size_t index = 0;
       index < record.party_communication.size();
       ++index) {
    if (index != 0U) {
      output << ',';
    }

    const auto& party =
        record.party_communication[index];

    output << "{\"party_label\":";
    append_json_string(
        output,
        party.party_label);

    output
        << ",\"sent_bits\":"
        << party.sent_bits
        << ",\"received_bits\":"
        << party.received_bits
        << '}';
  }

  output << ']';

  output << ",\"online_rounds\":";
  append_measurement(
      output,
      record.online_rounds);

  output << ",\"online_prg_calls_total\":";
  append_measurement(
      output,
      record.online_prg_calls_total);

  output << ",\"comparison_edges_total\":";
  append_measurement(
      output,
      record.comparison_edges_total);

  output << ",\"total_time_ms\":";
  append_measurement(
      output,
      record.total_time_ms);

  output << ",\"correctness_status\":";
  append_json_string(
      output,
      correctness_status_name(
          record.correctness_status));

  output << '}';

  return output.str();
}

}  // namespace moe_topk
