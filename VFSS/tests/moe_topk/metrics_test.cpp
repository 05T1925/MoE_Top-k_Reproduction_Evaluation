#include <moe_topk/metrics.h>

#include <cmath>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void require_close(double actual, double expected, const char* message) {
    require(std::abs(actual - expected) < 1e-12, message);
}

void expect_invalid_argument(void (*operation)(), const char* message) {
    try {
        operation();
    } catch (const std::invalid_argument&) {
        return;
    }
    throw std::runtime_error(message);
}

void derive_without_offline_measurement() {
    moe_topk::MetricsRecord record;
    record.online_time_ms = moe_topk::Measurement<double>::measured(2.0);
    record.party_communication = {{"party0", 8, 0}};
    moe_topk::derive_metrics(record);
}

void derive_with_mismatched_online_party_count() {
    moe_topk::MetricsRecord record;
    record.offline_time_ms = moe_topk::Measurement<double>::measured(1.0);
    record.online_time_ms = moe_topk::Measurement<double>::measured(2.0);
    record.online_party_count = 1;
    record.party_communication = {{"party0", 8, 0}, {"party1", 8, 0}};
    moe_topk::derive_metrics(record);
}

void derive_with_zero_online_party_count() {
    moe_topk::MetricsRecord record;
    record.offline_time_ms = moe_topk::Measurement<double>::measured(1.0);
    record.online_time_ms = moe_topk::Measurement<double>::measured(2.0);
    moe_topk::derive_metrics(record);
}

void measure_empty_provenance_string() {
    (void)moe_topk::Measurement<std::string>::measured("");
}

}  // namespace

int main() {
    moe_topk::MetricsRecord record;
    record.implementation_label = "m1_1_test_and_metrics_foundation";
    record.git_revision = "test";
    record.runtime = "VFSS";
    record.party_topology = "2+1";
    record.n = 7;
    record.K = 3;
    record.input_seed = moe_topk::Measurement<std::uint64_t>::measured(0x4d315f31U);
    record.input_distribution = moe_topk::Measurement<std::string>::measured(
        "uniform_quantized_integer[-32*2^12,32*2^12]");
    record.compiler = moe_topk::Measurement<std::string>::measured("test-compiler");
    record.compiler_flags = moe_topk::Measurement<std::string>::measured("-O0 -g");
    record.build_type = moe_topk::Measurement<std::string>::measured("Debug");
    record.cpu_model = moe_topk::Measurement<std::string>::measured("test-cpu");
    record.system_memory_bytes =
        moe_topk::Measurement<std::uint64_t>::measured(16ULL * 1024 * 1024 * 1024);
    record.operating_system = moe_topk::Measurement<std::string>::measured("test-os");
    record.network_environment =
        moe_topk::Measurement<std::string>::not_applicable();
    record.network_bandwidth_mbps = moe_topk::Measurement<double>::not_measured();
    record.network_rtt_ms = moe_topk::Measurement<double>::not_measured();
    record.warmup_runs = moe_topk::Measurement<std::uint64_t>::measured(1);
    record.repetitions = moe_topk::Measurement<std::uint64_t>::measured(5);
    record.score_interpretation = "SIGNED_TWOS_COMPLEMENT_FIXED_POINT";
    record.fixed_point_scale = moe_topk::Measurement<std::uint32_t>::measured(12);
    record.thread_count = 1;
    record.offline_time_ms = moe_topk::Measurement<double>::measured(1.25);
    record.online_time_ms = moe_topk::Measurement<double>::measured(2.75);
    record.party_communication = {
        {"party0", 100, 300},
        {"party1", 300, 100},
    };
    record.online_party_count = 2;

    moe_topk::derive_metrics(record);

    require_close(*record.total_time_ms.value, 4.0,
                  "total_time_ms must equal offline_time_ms + online_time_ms");
    require(record.implementation_label == "m1_1_test_and_metrics_foundation",
            "implementation label must preserve the M1.1 foundation identity");
    require(*record.online_comm_total_bits.value == 400,
            "online_comm_total_bits must sum sent_bits only");
    require_close(*record.online_comm_per_party_bits.value, 200.0,
                  "online_comm_per_party_bits must divide by online party count");
    require(record.aav86_iterations_r.state ==
                moe_topk::MeasurementState::NOT_APPLICABLE,
            "AAV86 iterations must remain explicitly not applicable");
    require(*record.input_seed.value == 0x4d315f31U,
            "input seed must preserve its measured value");
    require(*record.input_distribution.value ==
                "uniform_quantized_integer[-32*2^12,32*2^12]",
            "input distribution must preserve its measured value");
    require(*record.compiler.value == "test-compiler",
            "compiler must preserve its measured value");
    require(*record.compiler_flags.value == "-O0 -g",
            "compiler flags must preserve their measured value");
    require(*record.build_type.value == "Debug",
            "build type must preserve its measured value");
    require(*record.cpu_model.value == "test-cpu",
            "CPU model must preserve its measured value");
    require(*record.system_memory_bytes.value == 16ULL * 1024 * 1024 * 1024,
            "system memory must preserve its measured value");
    require(*record.operating_system.value == "test-os",
            "operating system must preserve its measured value");
    require(record.network_environment.state ==
                moe_topk::MeasurementState::NOT_APPLICABLE,
            "non-network functional tests must mark network as not applicable");
    require(record.network_bandwidth_mbps.state ==
                moe_topk::MeasurementState::NOT_MEASURED,
            "unmeasured network bandwidth must not be represented as zero");
    require(record.network_rtt_ms.state == moe_topk::MeasurementState::NOT_MEASURED,
            "unmeasured network RTT must not be represented as zero");
    require(*record.warmup_runs.value == 1,
            "warmup count must preserve its measured value");
    require(*record.repetitions.value == 5,
            "repetition count must preserve its measured value");
    require(*record.fixed_point_scale.value == 12,
            "fixed-point scale must preserve the unified semantic value");
    require(record.offline_material_total_bits.state ==
                moe_topk::MeasurementState::NOT_MEASURED,
            "unmeasured offline material must not be represented as zero");

    expect_invalid_argument(derive_without_offline_measurement,
                            "unmeasured time must not be derived as zero");
    expect_invalid_argument(derive_with_mismatched_online_party_count,
                            "online party count must be explicit and consistent");
    expect_invalid_argument(derive_with_zero_online_party_count,
                            "zero online party count must fail explicitly");
    expect_invalid_argument(measure_empty_provenance_string,
                            "empty provenance must not masquerade as measured");

    const auto zero = moe_topk::Measurement<double>::measured(0.0);
    require(zero.state == moe_topk::MeasurementState::MEASURED &&
                zero.value.has_value() && *zero.value == 0.0,
            "a measured zero must remain distinct from not measured");
    return 0;
}
