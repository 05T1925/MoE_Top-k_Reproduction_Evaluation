#include <moe_topk/protocol_i_chosen_ot.h>

#include <array>
#include <chrono>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {
using moe_topk::ProtocolIBlock128;
using moe_topk::ProtocolIChosenOtConfig;
volatile sig_atomic_t deadline_interrupts = 0;
void deadline_signal(int) {
  ++deadline_interrupts;
  if (deadline_interrupts == 10) {
    itimerval stop{};
    ::setitimer(ITIMER_REAL, &stop, nullptr);
  }
}

void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
void close_fd(int& fd) { if (fd >= 0) { ::close(fd); fd = -1; } }
void exact_write(int fd, const void* data, std::size_t size) {
  auto* p = static_cast<const std::uint8_t*>(data);
  while (size) { const auto n = ::write(fd, p, size); if (n < 0 && errno == EINTR) continue; require(n > 0, "test write"); p += n; size -= static_cast<std::size_t>(n); }
}
void exact_read(int fd, void* data, std::size_t size) {
  auto* p = static_cast<std::uint8_t*>(data);
  while (size) { const auto n = ::read(fd, p, size); if (n < 0 && errno == EINTR) continue; require(n > 0, "test read"); p += n; size -= static_cast<std::size_t>(n); }
}
void pair(int p[2]) { require(::socketpair(AF_UNIX, SOCK_STREAM, 0, p) == 0, "socketpair"); }
void close_except(std::initializer_list<int> keep) {
  for (int fd = 3; fd < 256; ++fd) {
    bool retained = false; for (const int permitted : keep) retained = retained || fd == permitted;
    if (!retained) ::close(fd);
  }
}
std::string self(const char* argv0) { char path[4096]{}; const auto n = ::readlink("/proc/self/exe", path, sizeof(path)-1); if (n > 0) { path[n] = 0; return path; } return argv0; }
pid_t spawn(const std::string& executable, const std::vector<std::string>& arguments) {
  const auto child = ::fork(); require(child >= 0, "fork");
  if (child == 0) { std::vector<char*> argv; argv.push_back(const_cast<char*>(executable.c_str())); for (const auto& argument : arguments) argv.push_back(const_cast<char*>(argument.c_str())); argv.push_back(nullptr); ::execv(executable.c_str(), argv.data()); _exit(127); }
  return child;
}
void wait_code(pid_t child, int expected, const char* label) { int status = 0; require(::waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == expected, label); }

ProtocolIChosenOtConfig config(std::uint32_t n, std::uint64_t material) { return {UINT64_C(0x4d323800) + n, UINT64_C(0x6d323800) + n, material, n, 15000}; }
std::vector<ProtocolIBlock128> messages(std::uint32_t n, int mode, bool right) {
  std::vector<ProtocolIBlock128> out(n); std::mt19937_64 rng(UINT64_C(0x4d324f54) + mode * 17 + right);
  for (std::uint32_t i=0;i<n;++i) for (std::size_t j=0;j<16;++j) {
    if (mode == 1) out[i][j] = 0;
    else if (mode == 2) out[i][j] = 0xff;
    else if (mode == 3) out[i][j] = static_cast<std::uint8_t>((i == 0 ? 0x80 : 0x7f) ^ (right ? 0x55 : 0xaa) ^ j);
    else out[i][j] = static_cast<std::uint8_t>(rng());
  }
  return out;
}
std::vector<std::uint8_t> choices(std::uint32_t n, int mode) {
  std::vector<std::uint8_t> out(n); std::mt19937_64 rng(UINT64_C(0x43484f49) + mode);
  for (std::uint32_t i=0;i<n;++i) out[i] = mode == 0 ? 0 : mode == 1 ? 1 : mode == 2 ? (i & 1U) : static_cast<std::uint8_t>(rng() & 1U);
  return out;
}
std::vector<std::string> args(const char* role, int peer, int input, int result, const ProtocolIChosenOtConfig& c) {
  return {"--role", role, "--peer-fd", std::to_string(peer), "--input-fd", std::to_string(input), "--result-fd", std::to_string(result), "--session", std::to_string(c.session), "--fingerprint", std::to_string(c.fingerprint), "--material", std::to_string(c.material_id), "--count", std::to_string(c.item_count)};
}

int run_sender(int peer, int input, int result, const ProtocolIChosenOtConfig& c) {
  try { std::vector<ProtocolIBlock128> a(c.item_count), b(c.item_count); exact_read(input, a.data(), a.size()*16); exact_read(input, b.data(), b.size()*16); const auto stats = moe_topk::protocol_i_chosen_ot_sender(c, peer, a, b); exact_write(result, &stats, sizeof(stats)); return 0; } catch (...) { return 1; }
}
int run_receiver(int peer, int input, int result, const ProtocolIChosenOtConfig& c) {
  try { std::vector<std::uint8_t> choice(c.item_count); exact_read(input, choice.data(), choice.size()); const auto output = moe_topk::protocol_i_chosen_ot_receiver(c, peer, choice); exact_write(result, output.selected_messages.data(), output.selected_messages.size()*16); exact_write(result, &output.counters, sizeof(output.counters)); return 0; } catch (...) { return 1; }
}

void run_case(const std::string& executable, std::uint32_t n, int choice_mode, int message_mode, std::uint64_t material) {
  int ot[2]{}, sender_input[2]{}, receiver_input[2]{}, sender_result[2]{}, receiver_result[2]{}; pair(ot); pair(sender_input); pair(receiver_input); pair(sender_result); pair(receiver_result);
  const auto c = config(n, material); const auto left = messages(n, message_mode, false); auto right = messages(n, message_mode, true); if (message_mode == 4) right = left; const auto bit = choices(n, choice_mode);
  const auto sender = spawn(executable, args("sender", ot[0], sender_input[1], sender_result[1], c));
  const auto receiver = spawn(executable, args("receiver", ot[1], receiver_input[1], receiver_result[1], c));
  close_fd(ot[0]); close_fd(ot[1]); close_fd(sender_input[1]); close_fd(receiver_input[1]); close_fd(sender_result[1]); close_fd(receiver_result[1]);
  exact_write(sender_input[0], left.data(), left.size()*16); exact_write(sender_input[0], right.data(), right.size()*16); ::shutdown(sender_input[0], SHUT_WR);
  exact_write(receiver_input[0], bit.data(), bit.size()); ::shutdown(receiver_input[0], SHUT_WR);
  moe_topk::ProtocolIChosenOtCounters sender_stats{}; exact_read(sender_result[0], &sender_stats, sizeof(sender_stats));
  std::vector<ProtocolIBlock128> selected(n); moe_topk::ProtocolIChosenOtCounters receiver_stats{}; exact_read(receiver_result[0], selected.data(), selected.size()*16); exact_read(receiver_result[0], &receiver_stats, sizeof(receiver_stats));
  wait_code(sender, 0, "sender child"); wait_code(receiver, 0, "receiver child");
  require(sender_stats.sent_bytes > 0 && sender_stats.received_bytes > 0 && receiver_stats.sent_bytes > 0 && receiver_stats.received_bytes > 0, "EMP counters");
  for (std::size_t i=0;i<selected.size();++i) require(selected[i] == (bit[i] ? right[i] : left[i]), "chosen-message differential");
  if (std::getenv("MOE_TOPK_CHOSEN_OT_REPORT") && n == 1 && choice_mode == 0 && message_mode == 0)
    std::cout << "chosen_ot_n=1 sender_sent=" << sender_stats.sent_bytes << " sender_received=" << sender_stats.received_bytes
              << " receiver_sent=" << receiver_stats.sent_bytes << " receiver_received=" << receiver_stats.received_bytes << '\n';
  close_fd(sender_input[0]); close_fd(receiver_input[0]); close_fd(sender_result[0]); close_fd(receiver_result[0]);
}

void negative_inputs() {
  int fds[2]{}; pair(fds); bool threw = false; try { (void)moe_topk::protocol_i_chosen_ot_receiver({1,2,3,0,1}, fds[0], {}); } catch (...) { threw = true; } require(threw, "zero item count");
  threw = false; try { (void)moe_topk::protocol_i_chosen_ot_receiver({1,2,3,1,1}, fds[0], {2}); } catch (...) { threw = true; } require(threw, "invalid choice");
  threw = false; try { (void)moe_topk::protocol_i_chosen_ot_sender({1,2,3,2,1}, fds[0], messages(1,0,false), messages(1,0,true)); } catch (...) { threw = true; } require(threw, "sender length"); close_fd(fds[0]); close_fd(fds[1]);
}

void expect_eof_and_timeout() {
  int fds[2]{}; pair(fds);
  const auto child = ::fork(); require(child >= 0, "EOF fork");
  if (child == 0) { close_fd(fds[0]); const std::uint8_t partial[7]{}; exact_write(fds[1], partial, sizeof(partial)); close_fd(fds[1]); _exit(0); }
  close_fd(fds[1]); bool threw = false;
  try { (void)moe_topk::protocol_i_chosen_ot_receiver({1,2,4,1,200}, fds[0], {0}); } catch (...) { threw = true; }
  require(threw, "truncated preamble EOF"); wait_code(child, 0, "EOF child"); close_fd(fds[0]);
  pair(fds); threw = false;
  try { (void)moe_topk::protocol_i_chosen_ot_receiver({1,2,5,1,20}, fds[0], {0}); } catch (...) { threw = true; }
  require(threw, "preamble timeout"); close_fd(fds[0]); close_fd(fds[1]);
}

void expect_interrupted_absolute_deadline() {
  int fds[2]{}; pair(fds);
  struct sigaction old_action{}, action{};
  action.sa_handler = deadline_signal; ::sigemptyset(&action.sa_mask);
  require(::sigaction(SIGALRM, &action, &old_action) == 0, "signal setup");
  deadline_interrupts = 0;
  itimerval timer{}; timer.it_value.tv_usec = 2000; timer.it_interval.tv_usec = 2000;
  require(::setitimer(ITIMER_REAL, &timer, nullptr) == 0, "timer setup");
  const auto began = std::chrono::steady_clock::now(); bool threw = false;
  try { (void)moe_topk::protocol_i_chosen_ot_receiver({9,8,7,1,40}, fds[0], {0}); } catch (...) { threw = true; }
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count();
  itimerval stop{}; ::setitimer(ITIMER_REAL, &stop, nullptr); ::sigaction(SIGALRM, &old_action, nullptr);
  require(threw && deadline_interrupts >= 5, "interrupted deadline failure");
  require(elapsed < 56, "absolute deadline overrun after EINTR");
  close_fd(fds[0]); close_fd(fds[1]);
}

void run_mismatch(const std::string& executable, const ProtocolIChosenOtConfig& sender_config,
                  const ProtocolIChosenOtConfig& receiver_config, bool sender_on_both) {
  int ot[2]{}, sender_input[2]{}, receiver_input[2]{}, sender_result[2]{}, receiver_result[2]{}; pair(ot); pair(sender_input); pair(receiver_input); pair(sender_result); pair(receiver_result);
  const auto left = messages(sender_config.item_count, 0, false), right = messages(sender_config.item_count, 0, true); const auto bit = choices(receiver_config.item_count, 0);
  const auto sender = spawn(executable, args("sender", ot[0], sender_input[1], sender_result[1], sender_config));
  const auto receiver = spawn(executable, args(sender_on_both ? "sender" : "receiver", ot[1], receiver_input[1], receiver_result[1], receiver_config));
  close_fd(ot[0]); close_fd(ot[1]); close_fd(sender_input[1]); close_fd(receiver_input[1]); close_fd(sender_result[1]); close_fd(receiver_result[1]);
  exact_write(sender_input[0], left.data(), left.size()*16); exact_write(sender_input[0], right.data(), right.size()*16); ::shutdown(sender_input[0], SHUT_WR);
  if (sender_on_both) { exact_write(receiver_input[0], left.data(), left.size()*16); exact_write(receiver_input[0], right.data(), right.size()*16); } else exact_write(receiver_input[0], bit.data(), bit.size());
  ::shutdown(receiver_input[0], SHUT_WR); wait_code(sender, 1, "mismatch sender failure"); wait_code(receiver, 1, "mismatch receiver failure");
  close_fd(sender_input[0]); close_fd(receiver_input[0]); close_fd(sender_result[0]); close_fd(receiver_result[0]);
}

void expect_preamble_errors(const std::string& executable) {
  const auto valid = config(2, 0x1111);
  auto wrong_session = valid; ++wrong_session.session; run_mismatch(executable, valid, wrong_session, false);
  auto wrong_fingerprint = valid; ++wrong_fingerprint.fingerprint; run_mismatch(executable, valid, wrong_fingerprint, false);
  auto wrong_material = valid; ++wrong_material.material_id; run_mismatch(executable, valid, wrong_material, false);
  run_mismatch(executable, valid, valid, true);
}

void expect_replay() {
  int first[2]{}, second[2]{}; pair(first); pair(second); const auto c = config(1, 0x7777); const auto a = messages(1,0,false), b = messages(1,0,true); bool first_sender = false, first_receiver = false;
  std::thread sender([&] { try { (void)moe_topk::protocol_i_chosen_ot_sender(c, first[0], a, b); first_sender = true; } catch (...) {} });
  std::thread receiver([&] { try { (void)moe_topk::protocol_i_chosen_ot_receiver(c, first[1], {0}); first_receiver = true; } catch (...) {} }); sender.join(); receiver.join(); require(first_sender && first_receiver, "replay setup");
  bool replay_sender = false, replay_receiver = false;
  std::thread sender_replay([&] { try { (void)moe_topk::protocol_i_chosen_ot_sender(c, second[0], a, b); } catch (...) { replay_sender = true; } });
  std::thread receiver_replay([&] { try { (void)moe_topk::protocol_i_chosen_ot_receiver(c, second[1], {0}); } catch (...) { replay_receiver = true; } }); sender_replay.join(); receiver_replay.join(); require(replay_sender && replay_receiver, "material replay");
  close_fd(first[0]); close_fd(first[1]); close_fd(second[0]); close_fd(second[1]);
}

ProtocolIChosenOtConfig parse_config(int argc, char** argv) {
  ProtocolIChosenOtConfig c{}; for (int i=0;i+1<argc;i+=2) { const std::string key(argv[i]), value(argv[i+1]); if (key == "--session") c.session = std::stoull(value); else if (key == "--fingerprint") c.fingerprint = std::stoull(value); else if (key == "--material") c.material_id = std::stoull(value); else if (key == "--count") c.item_count = static_cast<std::uint32_t>(std::stoul(value)); } c.timeout_ms = 15000; return c;
}
int role_main(int argc, char** argv) {
  require(argc == 17 && std::string(argv[1]) == "--role" && std::string(argv[3]) == "--peer-fd" && std::string(argv[5]) == "--input-fd" && std::string(argv[7]) == "--result-fd", "role args");
  const int peer = std::stoi(argv[4]), input = std::stoi(argv[6]), result = std::stoi(argv[8]); const auto c = parse_config(argc - 9, argv + 9); close_except({peer,input,result}); return std::string(argv[2]) == "sender" ? run_sender(peer,input,result,c) : std::string(argv[2]) == "receiver" ? run_receiver(peer,input,result,c) : 2;
}
}  // namespace

int main(int argc, char** argv) {
  try {
    ::signal(SIGPIPE, SIG_IGN);
    if (argc > 1) return role_main(argc, argv);
    const auto executable = self(argv[0]); negative_inputs(); expect_eof_and_timeout(); expect_interrupted_absolute_deadline(); expect_preamble_errors(executable); expect_replay(); std::uint64_t material = 1;
    for (const auto n : {1U,2U,17U,128U}) for (int choice_mode=0; choice_mode<4; ++choice_mode) for (int message_mode=0; message_mode<5; ++message_mode) run_case(executable, n, choice_mode, message_mode, material++);
    run_case(executable, 17, 3, 0, material++); run_case(executable, 17, 3, 1, material++);  // fresh-material batches
    return 0;
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
