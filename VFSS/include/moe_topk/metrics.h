#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace moe_topk {

enum class MeasurementState {
    MEASURED,
    NOT_MEASURED,
    NOT_APPLICABLE,
};

template <typename T>
struct Measurement {
    MeasurementState state = MeasurementState::NOT_MEASURED;
    std::optional<T> value;

    static Measurement measured(T measured_value) {
        if constexpr (std::is_same_v<T, std::string>) {
            if (measured_value.empty()) {
                throw std::invalid_argument("measured string must not be empty");
            }
        }
        return {MeasurementState::MEASURED, std::move(measured_value)};
    }

    static Measurement not_measured() {
        return {MeasurementState::NOT_MEASURED, std::nullopt};
    }

    static Measurement not_applicable() {
        return {MeasurementState::NOT_APPLICABLE, std::nullopt};
    }
};

enum class CorrectnessStatus {
    PASSED,
    FAILED,
    NOT_MEASURED,
};

struct PartyCommunication {
    std::string party_label;
    std::uint64_t sent_bits = 0;
    std::uint64_t received_bits = 0;
};

struct MetricsRecord {
    std::string implementation_label;
    std::string git_revision;
    std::string runtime;
    std::string party_topology;
    std::uint64_t n = 0;
    std::uint64_t K = 0;
    Measurement<std::uint64_t> input_seed =
        Measurement<std::uint64_t>::not_measured();
    Measurement<std::string> input_distribution =
        Measurement<std::string>::not_measured();
    Measurement<std::string> compiler = Measurement<std::string>::not_measured();
    Measurement<std::string> compiler_flags =
        Measurement<std::string>::not_measured();
    Measurement<std::string> build_type = Measurement<std::string>::not_measured();
    Measurement<std::string> cpu_model = Measurement<std::string>::not_measured();
    Measurement<std::uint64_t> system_memory_bytes =
        Measurement<std::uint64_t>::not_measured();
    Measurement<std::string> operating_system =
        Measurement<std::string>::not_measured();
    Measurement<std::string> network_environment =
        Measurement<std::string>::not_measured();
    Measurement<double> network_bandwidth_mbps = Measurement<double>::not_measured();
    Measurement<double> network_rtt_ms = Measurement<double>::not_measured();
    Measurement<std::uint64_t> warmup_runs =
        Measurement<std::uint64_t>::not_measured();
    Measurement<std::uint64_t> repetitions =
        Measurement<std::uint64_t>::not_measured();
    Measurement<std::uint64_t> aav86_iterations_r =
        Measurement<std::uint64_t>::not_applicable();
    std::uint32_t score_bit_width = 32;
    std::string score_interpretation;
    Measurement<std::uint32_t> fixed_point_scale =
        Measurement<std::uint32_t>::not_measured();
    std::uint32_t thread_count = 0;
    Measurement<double> offline_time_ms = Measurement<double>::not_measured();
    Measurement<std::uint64_t> offline_material_total_bits =
        Measurement<std::uint64_t>::not_measured();
    Measurement<double> online_time_ms = Measurement<double>::not_measured();
    Measurement<std::uint64_t> online_comm_total_bits =
        Measurement<std::uint64_t>::not_measured();
    Measurement<double> online_comm_per_party_bits =
        Measurement<double>::not_measured();
    std::uint32_t online_party_count = 0;
    std::vector<PartyCommunication> party_communication;
    Measurement<std::uint64_t> online_rounds =
        Measurement<std::uint64_t>::not_measured();
    Measurement<std::uint64_t> online_prg_calls_total =
        Measurement<std::uint64_t>::not_measured();
    Measurement<std::uint64_t> comparison_edges_total =
        Measurement<std::uint64_t>::not_measured();
    Measurement<double> total_time_ms = Measurement<double>::not_measured();
    CorrectnessStatus correctness_status = CorrectnessStatus::NOT_MEASURED;
};

template <typename T>
inline const T& require_measured(const Measurement<T>& measurement,
                                 const char* field_name) {
    if (measurement.state != MeasurementState::MEASURED ||
        !measurement.value.has_value()) {
        throw std::invalid_argument(std::string(field_name) + " is not measured");
    }
    return *measurement.value;
}

inline void derive_metrics(MetricsRecord& record) {
    const double offline_time_ms =
        require_measured(record.offline_time_ms, "offline_time_ms");
    const double online_time_ms =
        require_measured(record.online_time_ms, "online_time_ms");
    if (record.online_party_count == 0 ||
        record.online_party_count != record.party_communication.size()) {
        throw std::invalid_argument("online party count does not match party communication");
    }

    std::uint64_t online_comm_total_bits = 0;
    for (const auto& party : record.party_communication) {
        online_comm_total_bits += party.sent_bits;
    }

    record.total_time_ms =
        Measurement<double>::measured(offline_time_ms + online_time_ms);
    record.online_comm_total_bits =
        Measurement<std::uint64_t>::measured(online_comm_total_bits);
    record.online_comm_per_party_bits = Measurement<double>::measured(
        static_cast<double>(online_comm_total_bits) /
        static_cast<double>(record.online_party_count));
}

}  // namespace moe_topk
