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

}  // namespace

int main() {
    moe_topk::MetricsRecord record;
    record.implementation_label = "m1_metrics_test";
    record.git_revision = "test";
    record.runtime = "VFSS";
    record.party_topology = "2+1";
    record.n = 7;
    record.K = 3;
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
    require(*record.online_comm_total_bits.value == 400,
            "online_comm_total_bits must sum sent_bits only");
    require_close(*record.online_comm_per_party_bits.value, 200.0,
                  "online_comm_per_party_bits must divide by online party count");
    require(record.aav86_iterations_r.state ==
                moe_topk::MeasurementState::NOT_APPLICABLE,
            "AAV86 iterations must remain explicitly not applicable");
    require(*record.fixed_point_scale.value == 12,
            "fixed-point scale must preserve the unified semantic value");
    require(record.offline_material_total_bits.state ==
                moe_topk::MeasurementState::NOT_MEASURED,
            "unmeasured offline material must not be represented as zero");

    expect_invalid_argument(derive_without_offline_measurement,
                            "unmeasured time must not be derived as zero");
    expect_invalid_argument(derive_with_mismatched_online_party_count,
                            "online party count must be explicit and consistent");
    return 0;
}
