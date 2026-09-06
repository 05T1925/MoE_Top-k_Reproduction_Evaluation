#include <moe_topk/masked_mul_adapter.h>

#include <FSS/comms.h>
#include <FSS/prng.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr std::size_t kWireBufferBytes = 128;

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename Exception, typename Operation>
void expect_exception(Operation operation, const std::string& message) {
    try {
        operation();
    } catch (const Exception&) {
        return;
    }
    throw std::runtime_error(message);
}

moe_topk::MaskedMulMaterial transport_material(
    const moe_topk::MaskedMulMaterial& original) {
    std::array<char, kWireBufferBytes> wire{};

    char* write_cursor = wire.data();
    Peer sender(&write_cursor);
    moe_topk::send_masked_mul_material(sender, original);
    const std::uint64_t sent_bytes = sender.bytesSent();

    require(sent_bytes == sizeof(MultKey) + sizeof(GroupElement),
            "masked multiplication material byte count changed");
    require(write_cursor == wire.data() + sent_bytes,
            "Peer byte counter disagrees with the memory channel cursor");
    require(sent_bytes <= wire.size(),
            "masked multiplication material exceeded the wire buffer");

    char* read_cursor = wire.data();
    Dealer receiver(&read_cursor);
    auto received = moe_topk::receive_masked_mul_material(receiver);

    require(receiver.bytesReceived() == sent_bytes,
            "Dealer did not receive the complete multiplication material");
    require(read_cursor == write_cursor,
            "Peer/Dealer cursors disagree after material transport");

    delete static_cast<MemBuf*>(sender.keyBuf);
    delete static_cast<MemBuf*>(receiver.keyBuf);
    return received;
}

void verify_case(GroupElement left, GroupElement right) {
    const GroupElement left_mask = random_ge(moe_topk::kMaskedMulRingBits);
    const GroupElement right_mask = random_ge(moe_topk::kMaskedMulRingBits);
    const GroupElement output_mask = random_ge(moe_topk::kMaskedMulRingBits);
    auto generated = moe_topk::generate_masked_mul_material(
        left_mask, right_mask, output_mask);

    auto material0 = transport_material(generated.party0);
    auto material1 = transport_material(generated.party1);
    const auto left_shares = splitShare(left, moe_topk::kMaskedMulRingBits);
    const auto right_shares = splitShare(right, moe_topk::kMaskedMulRingBits);

    const auto open_share0 = moe_topk::prepare_masked_mul_open_share(
        left_shares.first, right_shares.first, material0);
    const auto open_share1 = moe_topk::prepare_masked_mul_open_share(
        left_shares.second, right_shares.second, material1);
    const GroupElement opened_left = open_share0.left + open_share1.left;
    const GroupElement opened_right = open_share0.right + open_share1.right;

    const GroupElement product_share0 = moe_topk::evaluate_masked_mul_share(
        moe_topk::OnlineParty::kParty0,
        opened_left,
        opened_right,
        material0);
    const GroupElement product_share1 = moe_topk::evaluate_masked_mul_share(
        moe_topk::OnlineParty::kParty1,
        opened_left,
        opened_right,
        material1);

    require(product_share0 + product_share1 == left * right,
            "additive output shares do not equal the ring product");
    expect_exception<std::logic_error>(
        [&] {
            moe_topk::evaluate_masked_mul_share(
                moe_topk::OnlineParty::kParty0,
                opened_left,
                opened_right,
                material0);
        },
        "reusing consumed multiplication material must fail");
}

void verify_state_and_party_failures() {
    auto generated = moe_topk::generate_masked_mul_material(3, 5, 7);

    expect_exception<std::logic_error>(
        [&] {
            moe_topk::evaluate_masked_mul_share(
                moe_topk::OnlineParty::kParty0, 0, 0, generated.party0);
        },
        "evaluation before preparing open shares must fail");

    moe_topk::prepare_masked_mul_open_share(11, 13, generated.party0);
    expect_exception<std::invalid_argument>(
        [&] {
            moe_topk::evaluate_masked_mul_share(
                static_cast<moe_topk::OnlineParty>(2),
                0,
                0,
                generated.party0);
        },
        "an invalid online party must fail");

    moe_topk::evaluate_masked_mul_share(
        moe_topk::OnlineParty::kParty0, 0, 0, generated.party0);
    expect_exception<std::logic_error>(
        [&] {
            moe_topk::prepare_masked_mul_open_share(11, 13, generated.party0);
        },
        "preparing open shares with consumed material must fail");
}

}  // namespace

int main() {
    try {
        for (int i = 0; i < 256; ++i) {
            FSSConfig::prngs[i].SetSeed(
                osuCrypto::toBlock(UINT64_C(0x4d554c5f4d33),
                                   static_cast<std::uint64_t>(i)));
        }

        std::vector<std::pair<GroupElement, GroupElement>> cases = {
            {0, 0},
            {0, UINT64_MAX},
            {1, 1},
            {1, UINT64_MAX},
            {UINT64_MAX, UINT64_MAX},
            {UINT64_C(0x8000000000000000), 2},
            {UINT64_C(0x0123456789abcdef),
             UINT64_C(0xfedcba9876543210)},
        };

        std::mt19937_64 random(UINT64_C(0x4d554c5f54455354));
        for (int i = 0; i < 32; ++i) {
            cases.emplace_back(random(), random());
        }

        for (const auto& test_case : cases) {
            verify_case(test_case.first, test_case.second);
        }
        verify_state_and_party_failures();

        std::cout << "Share-preserving multiplication adapter passed: "
                  << cases.size() << " arithmetic cases and state checks\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
