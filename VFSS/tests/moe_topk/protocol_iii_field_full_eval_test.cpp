#include <moe_topk/protocol_iii_dpf_routing.h>
#include <moe_topk/protocol_iii_field_dpf.h>

#include <FSS/dpf.h>
#include <FSS/prng.h>

#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

namespace {
void check(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}

void native_full_eval_experiment() {
  for (int i = 0; i < 256; ++i)
    FSSConfig::prngs[i].SetSeed(osuCrypto::toBlock(
        UINT64_C(0x5f071), static_cast<std::uint64_t>(i)));
  for (int bits : {1, 2, 3}) {
    const auto domain = std::size_t{1} << bits;
    for (std::uint64_t alpha = 0; alpha < domain; ++alpha) {
      const auto beta = UINT64_C(0x7000000000000000) + alpha;
      auto raw = keyGenDPF(bits, 64, alpha, beta);
      moe_topk::ProtocolIIIDpfRoutingKey p0(std::move(raw.first));
      moe_topk::ProtocolIIIDpfRoutingKey p1(std::move(raw.second));
      for (std::uint64_t shift = 0; shift < domain; ++shift) {
        std::vector<GroupElement> out0(domain), out1(domain);
        evalAll(0, p0.native_key(), shift, out0.data());
        evalAll(1, p1.native_key(), shift, out1.data());
        for (std::uint64_t x = 0; x < domain; ++x) {
          const auto position = (x + shift) % domain;
          check(out0[position] == evalDPF_Payload(0, p0.native_key(), x) &&
                    out1[position] == evalDPF_Payload(1, p1.native_key(), x),
                "native FullEval local share ordering");
          check(static_cast<std::uint64_t>(out0[position] + out1[position]) ==
                    (x == alpha ? beta : 0U),
                "native FullEval ring reconstruction");
        }
      }
    }
  }
}
void field_full_eval_conformance() {
  using moe_topk::ProtocolIIIField;
  const auto session = UINT64_C(0x5f071123);
  const auto fingerprint = UINT64_C(0x5f071456);
  std::mt19937_64 random(UINT64_C(0x5f071789));
  for (const std::uint8_t bits : {1U, 2U, 3U, 5U, 8U}) {
    const auto domain = std::uint64_t{1} << bits;
    for (int iteration = 0; iteration < 8; ++iteration) {
      const auto alpha = random() % domain;
      const std::vector<ProtocolIIIField> betas{
          ProtocolIIIField::from_u64(0), ProtocolIIIField::from_u64(1),
          ProtocolIIIField::from_canonical(ProtocolIIIField::modulus() - 1U),
          ProtocolIIIField::from_u64(random())};
      for (std::uint32_t slot = 0; slot < betas.size(); ++slot) {
        auto keys = moe_topk::protocol_iii_field_dpf_generate(
            bits, alpha, betas[slot], session, fingerprint, slot);
        auto p0 = moe_topk::ProtocolIIIFieldDpfPartyKey::deserialize(
            keys.first.serialize());
        auto p1 = moe_topk::ProtocolIIIFieldDpfPartyKey::deserialize(
            keys.second.serialize());
        const auto v0 = moe_topk::protocol_iii_field_dpf_full_eval(
            p0, 0U, session, fingerprint, slot, bits);
        const auto v1 = moe_topk::protocol_iii_field_dpf_full_eval(
            p1, 1U, session, fingerprint, slot, bits);
        check(v0.size() == domain && v1.size() == domain,
              "field FullEval domain length");
        std::size_t nonzero = 0;
        for (std::uint64_t x = 0; x < domain; ++x) {
          const auto expected = x == alpha ? betas[slot] : ProtocolIIIField{};
          const auto actual = ProtocolIIIField::add(v0[x], v1[x]);
          check(actual == expected, "field FullEval point reconstruction");
          if (!actual.is_zero()) ++nonzero;
          check(v0[x] == moe_topk::protocol_iii_field_dpf_eval(
                    p0, 0U, session, fingerprint, slot, x) &&
                v1[x] == moe_topk::protocol_iii_field_dpf_eval(
                    p1, 1U, session, fingerprint, slot, x),
                "field FullEval vs single Eval local shares");
        }
        check(nonzero == (betas[slot].is_zero() ? 0U : 1U),
              "field FullEval unique nonzero point");
        auto rejects = [&](auto fn, const char* why) {
          try { fn(); } catch (const std::invalid_argument&) { return; }
          throw std::runtime_error(why);
        };
        rejects([&] { (void)moe_topk::protocol_iii_field_dpf_full_eval(
            p0, 1U, session, fingerprint, slot, bits); }, "wrong party accepted");
        rejects([&] { (void)moe_topk::protocol_iii_field_dpf_full_eval(
            p0, 0U, session + 1U, fingerprint, slot, bits); }, "wrong session accepted");
        rejects([&] { (void)moe_topk::protocol_iii_field_dpf_full_eval(
            p0, 0U, session, fingerprint + 1U, slot, bits); }, "wrong fingerprint accepted");
        rejects([&] { (void)moe_topk::protocol_iii_field_dpf_full_eval(
            p0, 0U, session, fingerprint, slot + 1U, bits); }, "wrong slot accepted");
        rejects([&] { (void)moe_topk::protocol_iii_field_dpf_full_eval(
            p0, 0U, session, fingerprint, slot,
            static_cast<std::uint8_t>(bits + 1U)); }, "wrong domain accepted");
        auto encoded = p0.serialize();
        auto truncated = encoded; truncated.pop_back();
        rejects([&] { (void)moe_topk::ProtocolIIIFieldDpfPartyKey::deserialize(
            truncated); }, "truncated key accepted");
        auto extra = encoded; extra.push_back(0U);
        rejects([&] { (void)moe_topk::ProtocolIIIFieldDpfPartyKey::deserialize(
            extra); }, "extra key byte accepted");
        for (std::size_t i = 28U; i < 44U; ++i) encoded[i] = 0xffU;
        rejects([&] { (void)moe_topk::ProtocolIIIFieldDpfPartyKey::deserialize(
            encoded); }, "noncanonical field correction accepted");
      }
    }
  }
  auto large = moe_topk::protocol_iii_field_dpf_generate(
      21U, 1U, ProtocolIIIField::from_u64(1), session, fingerprint, 0U);
  try {
    (void)moe_topk::protocol_iii_field_dpf_full_eval(
        large.first, 0U, session, fingerprint, 0U, 21U);
    throw std::runtime_error("oversized FullEval domain accepted");
  } catch (const std::invalid_argument&) {}
}
}  // namespace

int main() {
  try {
    native_full_eval_experiment();
    field_full_eval_conformance();
    std::cout << "M5-G native/field FullEval conformance PASS\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "M5-G FullEval experiment FAIL: " << e.what() << '\n';
    return 1;
  }
}
