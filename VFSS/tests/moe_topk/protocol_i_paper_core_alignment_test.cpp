#include <moe_topk/protocol_i_pipeline.h>

#include <cstdint>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

}  // namespace

int main() {
  try {
    const moe_topk::ProtocolIPriorityPipelineMetrics metrics;

    // Current VFSS accounting is an executable contract: two forward PS
    // rounds, one masked-key opening, one rank reveal, and two reverse PS
    // rounds.  The paper candidate is kept as a comparison value only.
    require(metrics.forward_rounds == 2, "forward round accounting");
    require(metrics.cmpagg_rounds == 1, "CmpAgg round accounting");
    require(metrics.rank_reveal_rounds == 1, "rank reveal round accounting");
    require(metrics.reverse_rounds == 2, "reverse round accounting");
    require(metrics.online_rounds == 6, "current core round accounting");

    constexpr std::uint64_t paper_core_rounds = 3;
    constexpr std::uint64_t current_core_rounds = 2 + 1 + 1;
    constexpr std::uint64_t raw_adapter_rounds = 2;
    constexpr std::uint64_t reverse_mask_adapter_rounds = 2;
    constexpr std::uint64_t current_total_rounds =
        raw_adapter_rounds + current_core_rounds + reverse_mask_adapter_rounds;
    constexpr std::uint64_t candidate_total_rounds =
        raw_adapter_rounds + paper_core_rounds + reverse_mask_adapter_rounds;

    require(current_core_rounds == 4, "current core decomposition");
    require(current_total_rounds == 8, "current total decomposition");
    require(candidate_total_rounds == 7, "candidate total decomposition");
    require(current_core_rounds != paper_core_rounds,
            "paper candidate must not overwrite current accounting");

    // There is intentionally no public-list output in the current shuffle
    // contract.  This test freezes the distinction at the accounting layer;
    // the alignment decision records the missing message/material contract.
    return 0;
  } catch (const std::exception& error) {
    return error.what() == nullptr ? 1 : 1;
  }
}
