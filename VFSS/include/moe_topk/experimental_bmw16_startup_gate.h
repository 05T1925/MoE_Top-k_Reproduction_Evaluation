#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <sys/types.h>

#if !defined(MOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER)
#error "BMW16-derived Select startup gate is opt-in and EXPERIMENTAL"
#endif

namespace moe_topk {

enum class ProtocolIBmw16OfflineRole : std::uint8_t {
  TrustedDealer = 1,
  Party0Receiver = 2,
  Party1Receiver = 3
};

struct ProtocolIBmw16StartupBinding {
  std::uint64_t session = 0;
  std::uint64_t fingerprint = 0;
  std::uint32_t n = 0;
  std::uint32_t k = 0;
};

struct ProtocolIBmw16OfflineChild {
  pid_t pid = -1;
  ProtocolIBmw16OfflineRole role = ProtocolIBmw16OfflineRole::TrustedDealer;
  ProtocolIBmw16StartupBinding binding;
};

struct ProtocolIBmw16StartupGateResult {
  // Normalized exit values. A signaled process is reported as 128 + signal.
  std::array<int, 3> exit_codes{{-1, -1, -1}};  // T, P0 receiver, P1 receiver
  bool gate_open = false;
  bool committed_pair_validated = false;
  int post_gate_status = -1;
};

// Trusted local supervisor primitive. The callback is the only place a caller
// should create input shares/start online parties. It runs only after T and
// both authenticated receivers have exited zero under the identical binding.
ProtocolIBmw16StartupGateResult protocol_i_bmw16_start_after_offline_gate(
    const ProtocolIBmw16StartupBinding& expected,
    const std::array<ProtocolIBmw16OfflineChild, 3>& children,
    const std::function<bool()>& validate_committed_pair,
    const std::function<int()>& create_inputs_and_start_parties);

}  // namespace moe_topk
