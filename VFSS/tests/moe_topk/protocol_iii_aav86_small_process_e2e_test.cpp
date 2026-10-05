#include <moe_topk/protocol_iii_aav86_small.h>
#include <moe_topk/topk_oracle.h>

#include <FSS/dcf.h>
#include <FSS/prng.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fcntl.h>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <sys/random.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
using namespace moe_topk;
using Bytes = std::vector<std::uint8_t>;
constexpr std::size_t kMaxPacket = 64U * 1024U * 1024U;
std::set<std::pair<std::uint64_t, std::uint64_t>> observed_pivot_seeds;

void require(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}

std::uint64_t random_word() {
  std::uint64_t value = 0;
  require(::getrandom(&value, sizeof(value), 0) == sizeof(value),
          "III+AAV86 E2E OS random");
  return value ? value : 1U;
}

ProtocolIIIAav86SmallConfig config(std::uint32_t n, std::uint32_t k,
                                   std::uint32_t r, std::uint8_t party,
                                   std::uint64_t session,
                                   std::uint64_t fingerprint,
                                   std::uint64_t material_id,
                                   const std::string& claim_dir) {
  ProtocolIIIAav86SmallConfig c;
  c.session = session;
  c.fingerprint = fingerprint;
  c.material_id = material_id;
  c.logical_n = n;
  c.k = k;
  c.iterations = r;
  c.party = party;
  c.timeout_ms = 10000;
  c.durable_claim_directory = claim_dir;
  return c;
}

void write_all(int fd, const std::uint8_t* data, std::size_t size) {
  while (size) {
    const auto written = ::send(fd, data, size, MSG_NOSIGNAL);
    if (written < 0 && errno == EINTR) continue;
    require(written > 0, "III+AAV86 IPC send");
    data += written;
    size -= static_cast<std::size_t>(written);
  }
}

void read_all(int fd, std::uint8_t* data, std::size_t size) {
  while (size) {
    const auto read = ::recv(fd, data, size, 0);
    if (read < 0 && errno == EINTR) continue;
    require(read > 0, "III+AAV86 IPC receive");
    data += read;
    size -= static_cast<std::size_t>(read);
  }
}

void append_u64(Bytes& out, std::uint64_t value) {
  for (int shift = 56; shift >= 0; shift -= 8)
    out.push_back(static_cast<std::uint8_t>(value >> shift));
}

std::uint64_t read_u64(const Bytes& in, std::size_t& at) {
  require(at <= in.size() && in.size() - at >= 8U,
          "III+AAV86 IPC u64 truncated");
  std::uint64_t value = 0;
  for (int i = 0; i < 8; ++i) value = (value << 8U) | in[at++];
  return value;
}

void send_packet(int fd, const Bytes& packet) {
  require(packet.size() <= kMaxPacket, "III+AAV86 IPC packet size");
  Bytes header;
  append_u64(header, packet.size());
  write_all(fd, header.data(), header.size());
  if (!packet.empty()) write_all(fd, packet.data(), packet.size());
}

Bytes receive_packet(int fd) {
  Bytes header(8U);
  read_all(fd, header.data(), header.size());
  std::size_t at = 0;
  const auto length = read_u64(header, at);
  require(length <= kMaxPacket, "III+AAV86 IPC packet length");
  Bytes packet(static_cast<std::size_t>(length));
  if (!packet.empty()) read_all(fd, packet.data(), packet.size());
  return packet;
}

Bytes encode_words(const std::vector<std::uint32_t>& values) {
  Bytes out;
  out.reserve(values.size() * sizeof(std::uint32_t));
  for (const auto value : values)
    for (int shift = 24; shift >= 0; shift -= 8)
      out.push_back(static_cast<std::uint8_t>(value >> shift));
  return out;
}

std::vector<std::uint32_t> decode_words(const Bytes& bytes, std::uint32_t n) {
  require(bytes.size() == static_cast<std::size_t>(n) * 4U,
          "III+AAV86 input-share length");
  std::vector<std::uint32_t> out(n);
  for (std::size_t i = 0; i < out.size(); ++i)
    for (std::size_t j = 0; j < 4U; ++j)
      out[i] = (out[i] << 8U) | bytes[i * 4U + j];
  return out;
}

Bytes encode_output(const ProtocolIIIAav86SmallOutput& output,
                    std::uint64_t pivot_seed_lo,
                    std::uint64_t pivot_seed_hi) {
  Bytes bytes = output.xor_mask_share;
  const auto& m = output.metrics;
  for (const auto value : {m.reserved_edge_slots_per_party, m.active_edges,
                           m.active_vertices, m.dcf_evaluations,
                           m.dpf_evaluations, m.dcf_online_prg_calls,
                           m.score_sent_bytes, m.score_received_bytes,
                           m.forward_sent_bytes, m.forward_received_bytes,
                           m.ca_sent_bytes, m.ca_received_bytes,
                           m.route_sent_bytes, m.route_received_bytes,
                           m.sent_bytes, m.received_bytes, m.causal_rounds})
    append_u64(bytes, value);
  append_u64(bytes, pivot_seed_lo);
  append_u64(bytes, pivot_seed_hi);
  for (const auto value : m.active_edges_by_round) append_u64(bytes, value);
  for (const auto value : m.active_vertices_by_round) append_u64(bytes, value);
  for (const auto value : m.dcf_prg_calls_by_round) append_u64(bytes, value);
  append_u64(bytes, m.message_trace.size());
  for (const auto& trace : m.message_trace) {
    bytes.push_back(trace.phase);
    append_u64(bytes, trace.sent_bytes);
    append_u64(bytes, trace.received_bytes);
  }
  return bytes;
}

struct Trace {
  std::uint8_t phase = 0;
  std::uint64_t sent = 0, received = 0;
};
struct Result {
  Bytes mask;
  std::array<std::uint64_t, 17> fixed{};
  std::uint64_t pivot_seed_lo = 0, pivot_seed_hi = 0;
  std::vector<std::uint64_t> edges, vertices, prg;
  std::vector<Trace> trace;
};

Result decode_output(const Bytes& bytes, std::uint32_t n, std::uint32_t r) {
  Result out;
  require(bytes.size() >= n, "III+AAV86 result mask truncated");
  out.mask.assign(bytes.begin(), bytes.begin() + n);
  std::size_t at = n;
  for (auto& value : out.fixed) value = read_u64(bytes, at);
  out.pivot_seed_lo = read_u64(bytes, at);
  out.pivot_seed_hi = read_u64(bytes, at);
  for (auto* values : {&out.edges, &out.vertices, &out.prg}) {
    values->resize(r);
    for (auto& value : *values) value = read_u64(bytes, at);
  }
  const auto count = read_u64(bytes, at);
  require(count <= 32U, "III+AAV86 trace count");
  out.trace.resize(static_cast<std::size_t>(count));
  for (auto& item : out.trace) {
    require(at < bytes.size(), "III+AAV86 trace phase truncated");
    item.phase = bytes[at++];
    item.sent = read_u64(bytes, at);
    item.received = read_u64(bytes, at);
  }
  require(at == bytes.size(), "III+AAV86 result trailing bytes");
  return out;
}

std::uint64_t parse_u(const char* text) { return std::stoull(text); }
std::string number(std::uint64_t value) { return std::to_string(value); }

void close_except(const std::vector<int>& keep) {
  for (int fd = 3; fd < 256; ++fd)
    if (std::find(keep.begin(), keep.end(), fd) == keep.end()) ::close(fd);
}

pid_t launch(const std::vector<std::string>& args,
             const std::vector<int>& keep) {
  const auto pid = ::fork();
  require(pid >= 0, "III+AAV86 fork");
  if (pid == 0) {
    close_except(keep);
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>("/proc/self/exe"));
    for (const auto& arg : args) argv.push_back(const_cast<char*>(arg.c_str()));
    argv.push_back(nullptr);
    ::execv("/proc/self/exe", argv.data());
    _exit(127);
  }
  return pid;
}

std::uint64_t wait_exit(pid_t pid, bool success, const char* role) {
  int status = 0;
  struct rusage usage{};
  require(::wait4(pid, &status, 0, &usage) == pid, "III+AAV86 wait4");
  const bool exited_ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
  require(exited_ok == success, role);
  return static_cast<std::uint64_t>(usage.ru_maxrss);
}

int dealer_process(const std::string& exe, int fd0, int fd1,
                   std::uint32_t n, std::uint32_t k, std::uint32_t r,
                   std::uint64_t session, std::uint64_t fingerprint,
                   std::uint64_t material_id, const std::string& claim_dir) {
  const auto c = config(n, k, r, 0U, session, fingerprint, material_id,
                        claim_dir);
  auto material = protocol_iii_aav86_small_dealer_generate(c);
  send_packet(fd0, protocol_iii_aav86_small_serialize_material(material.party0));
  send_packet(fd1, protocol_iii_aav86_small_serialize_material(material.party1));
  (void)exe;
  return 0;
}

int party_process(std::uint8_t party, int offline_fd, int input_fd,
                  int report_fd, std::uint32_t n, std::uint32_t k,
                  std::uint32_t r, std::uint64_t session,
                  std::uint64_t fingerprint, std::uint64_t material_id,
                  const std::string& claim_dir, bool abort_peer,
                  const std::vector<int>& channels) {
  const auto c = config(n, k, r, party, session, fingerprint, material_id,
                        claim_dir);
  const auto bytes = receive_packet(offline_fd);
  auto material = protocol_iii_aav86_small_deserialize_material(bytes, party, c);
  const std::uint8_t ready = 1U;
  write_all(report_fd, &ready, 1U);
  const auto raw = decode_words(receive_packet(input_fd), n);
  require(channels.size() == 2U * r + 4U, "III+AAV86 process channel count");
  if (abort_peer && party == 1U) {
    ::close(channels[2]);  // Inject a transport abort at the forward frame.
  }
  ProtocolIIIAav86SmallFds fds;
  fds.score = {{channels[0], channels[1]}};
  fds.forward = channels[2];
  auto at = std::size_t{3};
  fds.masked_lists.assign(channels.begin() + at,
                           channels.begin() + at + r);
  at += r;
  fds.early_ranks.assign(channels.begin() + at,
                          channels.begin() + at + (r - 1U));
  at += r - 1U;
  fds.final_masked_rank = channels[at++];
  fds.inverse = channels[at++];
  require(at == channels.size(), "III+AAV86 process frame mapping");
  const auto pivot_seed_lo = material.layout_and_ca.pivot_seed_lo;
  const auto pivot_seed_hi = material.layout_and_ca.pivot_seed_hi;
  const auto output = protocol_iii_aav86_small_party(c, std::move(material),
                                                      raw, fds);
  send_packet(report_fd, encode_output(output, pivot_seed_lo, pivot_seed_hi));
  return 0;
}

std::uint32_t padded(std::uint32_t n) {
  std::uint32_t d = 2U;
  while (d < n) d <<= 1U;
  return d;
}

void pair_fds(std::array<int, 2>& pair) {
  require(::socketpair(AF_UNIX, SOCK_STREAM, 0, pair.data()) == 0,
          "III+AAV86 socketpair");
}

std::vector<std::uint32_t> scores_for(std::uint32_t n, std::uint32_t style,
                                      std::uint64_t seed) {
  std::vector<std::uint32_t> values(n);
  if (style == 0U) {
    std::fill(values.begin(), values.end(), UINT32_C(0x80000000));
  } else if (style == 1U) {
    for (std::uint32_t i = 0; i < n; ++i)
      values[i] = (i % 3U == 0U) ? UINT32_MAX :
                  (i % 3U == 1U) ? UINT32_C(0x80000000) : UINT32_C(0x7fffffff);
  } else if (style == 2U) {
    for (std::uint32_t i = 0; i < n; ++i) values[i] = (i % 2U) ? 7U : UINT32_MAX;
  } else {
    std::mt19937_64 random(seed);
    std::uniform_int_distribution<std::int32_t> distribution(-32 * 4096, 32 * 4096);
    for (auto& value : values) value = static_cast<std::uint32_t>(distribution(random));
  }
  return values;
}

void run_case(const std::string& exe, std::uint32_t n, std::uint32_t k,
              std::uint32_t r, std::uint32_t style, std::uint64_t seed,
              bool abort_peer = false) {
  const auto session = random_word();
  const auto fingerprint = random_word();
  const auto material_id = random_word();
  const auto claim_dir = (std::filesystem::temp_directory_path() /
      ("m6a8-claim-" + number(::getpid()) + "-" + number(seed))).string();
  require(std::filesystem::create_directory(claim_dir), "III+AAV86 claim dir");
  std::array<std::array<int, 2>, 2> offline{}, input{}, report{};
  for (auto& pair : offline) pair_fds(pair);
  for (auto& pair : input) pair_fds(pair);
  for (auto& pair : report) pair_fds(pair);
  std::vector<std::array<int, 2>> online(2U * r + 4U);
  for (auto& pair : online) pair_fds(pair);

  std::vector<std::string> dealer_args{
      "--dealer", number(offline[0][1]), number(offline[1][1]), number(n),
      number(k), number(r), number(session), number(fingerprint),
      number(material_id), claim_dir};
  const auto dealer = launch(dealer_args, {offline[0][1], offline[1][1]});
  const auto spawn_party = [&](std::uint8_t party) {
    std::vector<int> keep{offline[party][0], input[party][1], report[party][1]};
    std::vector<std::string> args{
        abort_peer && party == 1U ? "--party-abort" : "--party",
        number(party), number(offline[party][0]), number(input[party][1]),
        number(report[party][1]), number(n), number(k), number(r),
        number(session), number(fingerprint), number(material_id), claim_dir};
    for (const auto& pair : online) {
      keep.push_back(pair[party]);
      args.push_back(number(pair[party]));
    }
    return launch(args, keep);
  };
  const auto p0 = spawn_party(0U);
  const auto p1 = spawn_party(1U);
  for (auto& pair : offline) { ::close(pair[0]); ::close(pair[1]); }
  for (auto& pair : input) ::close(pair[1]);
  for (auto& pair : report) ::close(pair[1]);
  for (auto& pair : online) { ::close(pair[0]); ::close(pair[1]); }
  const auto peak_t_kib = wait_exit(dealer, true, "III+AAV86 T exited unsuccessfully");
  std::array<std::uint8_t, 2> ready{};
  read_all(report[0][0], &ready[0], 1U);
  read_all(report[1][0], &ready[1], 1U);
  require(ready[0] == 1U && ready[1] == 1U, "III+AAV86 both parties ready");

  const auto scores = scores_for(n, style, seed);
  std::mt19937_64 share_random(seed ^ UINT64_C(0xA55A5AA5));
  std::vector<std::uint32_t> share0(n), share1(n);
  for (std::uint32_t i = 0; i < n; ++i) {
    share0[i] = static_cast<std::uint32_t>(share_random());
    share1[i] = scores[i] - share0[i];
  }
  send_packet(input[0][0], encode_words(share0));
  send_packet(input[1][0], encode_words(share1));

  if (abort_peer) {
    wait_exit(p0, false, "III+AAV86 peer abort did not fail P0");
    wait_exit(p1, false, "III+AAV86 peer abort did not fail P1");
    std::cout << "AAV86_III_E2E_ABORT n=" << n << " k=" << k << " r=" << r
              << " abort_phase=forward no_mask=1 PASS\n";
    for (auto& pair : input) ::close(pair[0]);
    for (auto& pair : report) ::close(pair[0]);
    std::filesystem::remove_all(claim_dir);
    return;
  }

  const auto out0 = decode_output(receive_packet(report[0][0]), n, r);
  const auto out1 = decode_output(receive_packet(report[1][0]), n, r);
  require(out0.pivot_seed_lo == out1.pivot_seed_lo &&
              out0.pivot_seed_hi == out1.pivot_seed_hi,
          "III+AAV86 parties received same public pivot seed");
  require(observed_pivot_seeds.emplace(out0.pivot_seed_lo,
                                       out0.pivot_seed_hi).second,
          "III+AAV86 fresh pivot seed across runs");
  const auto peak_p0_kib = wait_exit(p0, true, "III+AAV86 P0 exited unsuccessfully");
  const auto peak_p1_kib = wait_exit(p1, true, "III+AAV86 P1 exited unsuccessfully");
  constexpr std::uint64_t kSmallProcessMemoryGateKiB = UINT64_C(768) * 1024U;
  require(peak_t_kib < kSmallProcessMemoryGateKiB &&
              peak_p0_kib < kSmallProcessMemoryGateKiB &&
              peak_p1_kib < kSmallProcessMemoryGateKiB,
          "III+AAV86 D<=8 process memory gate");
  const auto expected = top_k_mask(scores, k);
  std::uint32_t selected = 0;
  for (std::uint32_t i = 0; i < n; ++i) {
    require(out0.mask[i] <= 1U && out1.mask[i] <= 1U,
            "III+AAV86 output share bit");
    const auto bit = static_cast<std::uint8_t>(out0.mask[i] ^ out1.mask[i]);
    if (bit != expected[i]) {
      std::cerr << "oracle mismatch n=" << n << " k=" << k << " r=" << r
                << " style=" << style << " i=" << i << " score="
                << static_cast<std::int32_t>(scores[i]) << " want="
                << static_cast<unsigned>(expected[i]) << " got="
                << static_cast<unsigned>(bit) << '\n';
      throw std::runtime_error("III+AAV86 frozen oracle differential");
    }
    selected += bit;
  }
  require(selected == k, "III+AAV86 output exact K");
  const auto d = padded(n);
  const auto full_slots = static_cast<std::uint64_t>(r) * d * (d - 1U) / 2U;
  require(out0.fixed[0] == full_slots && out1.fixed[0] == full_slots,
          "III+AAV86 full-pool reservations");
  require(out0.fixed[16] == 2U * r + 4U && out1.fixed[16] == 2U * r + 4U &&
              out0.trace.size() == 2U * r + 4U &&
              out1.trace.size() == 2U * r + 4U,
          "III+AAV86 observed causal frames");
  require(out0.fixed[14] == out1.fixed[15] &&
              out1.fixed[14] == out0.fixed[15],
          "III+AAV86 total wire conservation");
  require(out0.fixed[6] == out1.fixed[7] &&
              out1.fixed[6] == out0.fixed[7] &&
              out0.fixed[8] == out1.fixed[9] &&
              out1.fixed[8] == out0.fixed[9] &&
              out0.fixed[10] == out1.fixed[11] &&
              out1.fixed[10] == out0.fixed[11] &&
              out0.fixed[12] == out1.fixed[13] &&
              out1.fixed[12] == out0.fixed[13],
          "III+AAV86 stage wire conservation");
  for (std::size_t i = 0; i < out0.trace.size(); ++i)
    require(out0.trace[i].phase == out1.trace[i].phase &&
                out0.trace[i].sent == out1.trace[i].received &&
                out1.trace[i].sent == out0.trace[i].received,
            "III+AAV86 per-frame wire conservation");
  for (std::size_t i = 0; i < r; ++i)
    require(out0.edges[i] <= d * (d - 1U) / 2U &&
                out0.vertices[i] <= d &&
                (out0.edges[i] == 0U ? out0.prg[i] == 0U : out0.prg[i] > 0U),
            "III+AAV86 per-round graph/DCF telemetry");

  std::cout << "AAV86_III_E2E_PASS n=" << n << " k=" << k << " r=" << r
            << " D=" << d << " style=" << style << " input_seed=" << seed
            << " pivot_seed=" << out0.pivot_seed_lo << ":"
            << out0.pivot_seed_hi
            << " peak_t_kib=" << peak_t_kib
            << " peak_p0_kib=" << peak_p0_kib
            << " peak_p1_kib=" << peak_p1_kib
            << " selected=" << selected << " reserved_edges=" << full_slots
            << " active_edges=";
  for (const auto value : out0.edges) std::cout << value << ',';
  std::cout << " active_vertices=";
  for (const auto value : out0.vertices) std::cout << value << ',';
  std::cout << " rounds=" << out0.fixed[16]
            << " p0_sent=" << out0.fixed[14] << " p1_sent=" << out1.fixed[14]
            << " score=" << out0.fixed[6] << "/" << out1.fixed[6]
            << " forward=" << out0.fixed[8] << "/" << out1.fixed[8]
            << " ca=" << out0.fixed[10] << "/" << out1.fixed[10]
            << " route=" << out0.fixed[12] << "/" << out1.fixed[12]
            << " dcf_eval=" << out0.fixed[3] + out1.fixed[3]
            << " dpf_eval=" << out0.fixed[4] + out1.fixed[4]
            << " dcf_prg=" << out0.fixed[5] + out1.fixed[5] << '\n';
  if (n == 2U && k == 1U && r == 1U) {
    const auto c0 = config(n, k, r, 0U, session, fingerprint, material_id,
                           claim_dir);
    auto duplicate_material = protocol_iii_aav86_small_dealer_generate(c0);
    ProtocolIIIAav86SmallFds replay_fds;
    std::vector<int> opened_fds;
    const auto open_null = [&]() {
      const int fd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
      require(fd >= 0, "III+AAV86 replay test descriptor");
      opened_fds.push_back(fd);
      return fd;
    };
    replay_fds.score = {{open_null(), open_null()}};
    replay_fds.forward = open_null();
    for (std::uint32_t i = 0; i < r; ++i)
      replay_fds.masked_lists.push_back(open_null());
    for (std::uint32_t i = 1; i < r; ++i)
      replay_fds.early_ranks.push_back(open_null());
    replay_fds.final_masked_rank = open_null();
    replay_fds.inverse = open_null();
    bool replay_rejected = false;
    try {
      (void)protocol_iii_aav86_small_party(
          c0, std::move(duplicate_material.party0),
          std::vector<std::uint32_t>(n, 0U), replay_fds);
    } catch (const std::exception& error) {
      replay_rejected =
          std::string(error.what()).find("already claimed") != std::string::npos;
    }
    for (const auto fd : opened_fds) ::close(fd);
    require(replay_rejected, "III+AAV86 durable material replay rejected");
    std::cout << "AAV86_III_REPLAY_REJECT_PASS n=" << n
              << " material_id=" << material_id << '\n';
  }
  for (auto& pair : input) ::close(pair[0]);
  for (auto& pair : report) ::close(pair[0]);
  std::filesystem::remove_all(claim_dir);
}

int main_dispatch(int argc, char** argv) {
  if (argc == 11 && std::string(argv[1]) == "--dealer")
    return dealer_process("", std::stoi(argv[2]), std::stoi(argv[3]),
        static_cast<std::uint32_t>(parse_u(argv[4])),
        static_cast<std::uint32_t>(parse_u(argv[5])),
        static_cast<std::uint32_t>(parse_u(argv[6])), parse_u(argv[7]),
        parse_u(argv[8]), parse_u(argv[9]), argv[10]);
  if (argc >= 14 && (std::string(argv[1]) == "--party" ||
                     std::string(argv[1]) == "--party-abort")) {
    const auto n = static_cast<std::uint32_t>(parse_u(argv[6]));
    const auto k = static_cast<std::uint32_t>(parse_u(argv[7]));
    const auto r = static_cast<std::uint32_t>(parse_u(argv[8]));
    std::vector<int> channels;
    for (int i = 13; i < argc; ++i) channels.push_back(std::stoi(argv[i]));
    return party_process(static_cast<std::uint8_t>(parse_u(argv[2])),
        std::stoi(argv[3]), std::stoi(argv[4]), std::stoi(argv[5]), n, k, r,
        parse_u(argv[9]), parse_u(argv[10]), parse_u(argv[11]), argv[12],
        std::string(argv[1]) == "--party-abort", channels);
  }
  if (argc == 6 && std::string(argv[1]) == "--case") {
    run_case("/proc/self/exe",
        static_cast<std::uint32_t>(parse_u(argv[2])),
        static_cast<std::uint32_t>(parse_u(argv[3])),
        static_cast<std::uint32_t>(parse_u(argv[4])),
        static_cast<std::uint32_t>(parse_u(argv[5])), UINT64_C(0x6a8e3000));
    return 0;
  }
  require(argc == 1, "III+AAV86 E2E args");
  const std::string exe = "/proc/self/exe";
  std::uint64_t seed = UINT64_C(0x6a8e3000);
  for (const auto n : {2U, 5U, 8U}) {
    std::vector<std::uint32_t> ks{1U, (n + 1U) / 2U, n};
    std::sort(ks.begin(), ks.end());
    ks.erase(std::unique(ks.begin(), ks.end()), ks.end());
    for (const auto k : ks)
      for (std::uint32_t r = 1; r <= 5U; ++r)
        run_case(exe, n, k, r, static_cast<std::uint32_t>((n + k + r) % 4U),
                 seed++);
  }
  run_case(exe, 5U, 2U, 3U, 1U, seed++, true);
  return 0;
}
}  // namespace

int main(int argc, char** argv) {
  try {
    return main_dispatch(argc, argv);
  } catch (const std::exception& error) {
    std::cerr << "III+AAV86 process E2E failure: " << error.what() << '\n';
    return 1;
  }
}
