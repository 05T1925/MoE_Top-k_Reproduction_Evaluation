#include <moe_topk/experimental_bmw16_startup_gate.h>

#include <cerrno>
#include <set>
#include <stdexcept>
#include <sys/wait.h>

namespace moe_topk {
namespace {

bool same_binding(const ProtocolIBmw16StartupBinding& left,
                  const ProtocolIBmw16StartupBinding& right) {
  return left.session == right.session && left.fingerprint == right.fingerprint &&
      left.n == right.n && left.k == right.k;
}

int wait_normalized(pid_t pid) {
  int status = 0;
  pid_t result = -1;
  do {
    result = ::waitpid(pid, &status, 0);
  } while (result < 0 && errno == EINTR);
  if (result != pid) throw std::runtime_error("BMW16 startup supervisor waitpid failed");
  if (WIFEXITED(status)) return WEXITSTATUS(status);
  if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
  return 255;
}

}  // namespace

ProtocolIBmw16StartupGateResult protocol_i_bmw16_start_after_offline_gate(
    const ProtocolIBmw16StartupBinding& expected,
    const std::array<ProtocolIBmw16OfflineChild, 3>& children,
    const std::function<bool()>& validate_committed_pair,
    const std::function<int()>& create_inputs_and_start_parties) {
  if (expected.session == 0 || expected.fingerprint == 0 || expected.n == 0 ||
      expected.k == 0 || expected.k > expected.n || !validate_committed_pair ||
      !create_inputs_and_start_parties)
    throw std::invalid_argument("BMW16 startup gate binding/callback");

  const std::array<ProtocolIBmw16OfflineRole, 3> expected_roles{{
      ProtocolIBmw16OfflineRole::TrustedDealer,
      ProtocolIBmw16OfflineRole::Party0Receiver,
      ProtocolIBmw16OfflineRole::Party1Receiver}};
  std::set<pid_t> unique_pids;
  for (const auto& child : children)
    if (child.pid <= 0 || !unique_pids.insert(child.pid).second)
      throw std::invalid_argument("BMW16 startup gate child pid contract");

  ProtocolIBmw16StartupGateResult result;
  for (std::size_t i = 0; i < children.size(); ++i)
    result.exit_codes[i] = wait_normalized(children[i].pid);
  for (std::size_t i = 0; i < children.size(); ++i)
    if (children[i].role != expected_roles[i] || !same_binding(children[i].binding, expected))
      return result;
  if (result.exit_codes[0] != 0 || result.exit_codes[1] != 0 || result.exit_codes[2] != 0)
    return result;
  result.committed_pair_validated = validate_committed_pair();
  if (!result.committed_pair_validated) return result;

  result.gate_open = true;
  result.post_gate_status = create_inputs_and_start_parties();
  return result;
}

}  // namespace moe_topk
