#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace moe_topk {

// C: M2.8 EMP chosen-OT conformance boundary; not a Protocol I shuffle API.
using ProtocolIBlock128 = std::array<std::uint8_t, 16>;

struct ProtocolIChosenOtConfig {
  std::uint64_t session;
  std::uint64_t fingerprint;
  std::uint64_t material_id;
  std::uint32_t item_count;
  int timeout_ms;
  // Binds this invocation to a protocol stage.  The default is the standalone
  // M2.8 IKNP conformance boundary; higher-level adapters must use their own.
  std::uint32_t protocol_id = UINT32_C(0x494b4e50);  // IKNP
};

struct ProtocolIChosenOtCounters {
  std::uint64_t sent_bytes = 0;
  std::uint64_t received_bytes = 0;
  std::uint64_t rounds = 0;
};

struct ProtocolIChosenOtReceiverResult {
  std::vector<ProtocolIBlock128> selected_messages;
  ProtocolIChosenOtCounters counters;
};

// The sender never receives choices or selected messages.
ProtocolIChosenOtCounters protocol_i_chosen_ot_sender(
    const ProtocolIChosenOtConfig& config, int connected_fd,
    const std::vector<ProtocolIBlock128>& message0,
    const std::vector<ProtocolIBlock128>& message1);

// The receiver obtains exactly one 128-bit message for each 0/1 choice.
ProtocolIChosenOtReceiverResult protocol_i_chosen_ot_receiver(
    const ProtocolIChosenOtConfig& config, int connected_fd,
    const std::vector<std::uint8_t>& choices);

}  // namespace moe_topk
