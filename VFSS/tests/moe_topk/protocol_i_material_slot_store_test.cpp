#include <moe_topk/protocol_i_material_slot_store.h>

#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using namespace moe_topk;

namespace {

void check(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}

void expect_rejected(const std::function<void()>& f, const char* message) {
  bool rejected = false;
  try { f(); } catch (const std::exception&) { rejected = true; }
  check(rejected, message);
}

struct Cleanup {
  std::string path;
  ~Cleanup() {
    std::error_code ignored;
    std::filesystem::remove_all(path, ignored);
  }
};

ProtocolIMaterialSlotAddress addr(std::uint32_t purpose, std::uint32_t iteration,
                                  std::uint32_t left) {
  return {purpose, iteration, 0, 1, left, left + 1U};
}

int wait_child(pid_t child) {
  int status = 0;
  while (::waitpid(child, &status, 0) < 0) {
    if (errno == EINTR) continue;
    throw std::runtime_error("slot store waitpid");
  }
  return WIFEXITED(status) ? WEXITSTATUS(status) : 128;
}

std::string stem(const ProtocolIMaterialSlotBinding& b) {
  char name[96]{};
  std::snprintf(name, sizeof(name), "m6b5s2_%016llx_%016llx_%016llx_p%u",
      static_cast<unsigned long long>(b.session),
      static_cast<unsigned long long>(b.fingerprint),
      static_cast<unsigned long long>(b.material_id),
      static_cast<unsigned>(b.party));
  return name;
}

}  // namespace

int main() {
  char path[] = "/tmp/m6b5-slot-store-XXXXXX";
  char* created = ::mkdtemp(path);
  if (!created) throw std::runtime_error("slot store mkdtemp");
  const std::string directory(created);
  Cleanup cleanup{directory};
  try {
    const ProtocolIMaterialSlotBinding binding{
        7001, 7002, 7003, 256, 256, 8, 41, 0,
        ProtocolIDcfAbi::LegacyM2UcV1, 6};
    std::vector<std::uint8_t> a(700000, 0x11), b(700000, 0x22),
        c(700000, 0x33), d(700000, 0x44), e(700000, 0x55), f(700000, 0x66);
    const auto aa = addr(1, 0, 10), ab = addr(1, 1, 11), ac = addr(1, 2, 12),
               ad = addr(1, 3, 13), ae = addr(1, 4, 14), af = addr(2, 5, 15);
    {
      ProtocolIStreamedMaterialWriter writer(directory, binding, 1024U * 1024U);
      writer.append(aa, a); writer.append(ab, b); writer.append(ac, c);
      writer.append(ad, d); writer.append(ae, e); writer.append(af, f);
      expect_rejected([&] { writer.append(aa, a); },
                      "streamed duplicate slot address rejected");
      writer.finalize_staging();
      expect_rejected([&] { writer.append(addr(9, 0, 1), a); },
                      "finalized stream cannot accept new slots");
    }
    expect_rejected([&] {
      (void)protocol_i_streamed_material_claim_slot(directory, binding, aa);
    }, "unpublished T staging is not claimable");
    protocol_i_streamed_material_publish_ready(directory, binding);
    check(protocol_i_streamed_material_claim_slot(directory, binding, aa) == a,
          "slot 0 roundtrip");
    check(protocol_i_streamed_material_claim_slot(directory, binding, ab) == b,
          "slot 1 roundtrip");
    check(protocol_i_streamed_material_claim_slot(directory, binding, ac) == c,
          "slot 2 roundtrip");
    expect_rejected([&] {
      (void)protocol_i_streamed_material_claim_slot(directory, binding, aa);
    }, "streamed slot durable replay rejected");
    auto wrong_binding = binding;
    ++wrong_binding.k;
    expect_rejected([&] {
      (void)protocol_i_streamed_material_claim_slot(directory, wrong_binding, ad);
    }, "streamed slot K binding enforced");

    // A process that exits after receiving a slot cannot cause re-release.
    pid_t child = ::fork();
    check(child >= 0, "slot claim crash fork");
    if (child == 0) {
      try { (void)protocol_i_streamed_material_claim_slot(directory, binding, ad); ::_exit(71); }
      catch (...) { ::_exit(72); }
    }
    check(wait_child(child) == 71, "slot claim child exit after release");
    expect_rejected([&] {
      (void)protocol_i_streamed_material_claim_slot(directory, binding, ad);
    }, "crash-after-claim burns streamed slot");

    // Two processes race on the same lease file; O_EXCL allows one result.
    int gate[2];
    check(::pipe(gate) == 0, "slot race gate");
    std::array<pid_t, 2> children{};
    for (auto& process : children) {
      process = ::fork();
      check(process >= 0, "slot race fork");
      if (process == 0) {
        ::close(gate[1]);
        char token = 0;
        if (::read(gate[0], &token, 1) != 1) ::_exit(81);
        try { (void)protocol_i_streamed_material_claim_slot(directory, binding, ae); ::_exit(0); }
        catch (...) { ::_exit(82); }
      }
    }
    ::close(gate[0]);
    const char start[2] = {'x', 'y'};
    check(::write(gate[1], start, sizeof(start)) == 2, "slot race release");
    ::close(gate[1]);
    const int r0 = wait_child(children[0]), r1 = wait_child(children[1]);
    check((r0 == 0 && r1 == 82) || (r0 == 82 && r1 == 0),
          "one streamed slot lease winner");

    // Segment tampering after ready is detected before bytes are released.
    char segment[160]{};
    std::snprintf(segment, sizeof(segment), "%s/%s.%06u.seg",
                  directory.c_str(), stem(binding).c_str(), 5U);
    check(::chmod(segment, 0600) == 0, "slot segment chmod");
    const int segment_fd = ::open(segment, O_WRONLY | O_CLOEXEC);
    check(segment_fd >= 0, "slot segment open");
    const std::uint8_t changed = 0xfe;
    check(::pwrite(segment_fd, &changed, 1, 0) == 1, "slot segment tamper");
    ::close(segment_fd);
    check(::chmod(segment, 0400) == 0, "slot segment restore permissions");
    expect_rejected([&] {
      (void)protocol_i_streamed_material_claim_slot(directory, binding, af);
    }, "streamed slot digest tampering rejected");

    std::cout << "segmented stream, slot binding, durable one-shot, crash, race, integrity PASS\n";
    return 0;
  } catch (...) {
    std::cerr << "streamed slot-store test failed; ephemeral slot material removed\n";
    throw;
  }
}
