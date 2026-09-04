#include <FSS/comms.h>
#include <FSS/dpf.h>
#include <FSS/freekey.h>
#include <FSS/prng.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {

constexpr int kPayloadBits = 64;
constexpr std::size_t kWireBufferBytes = 4096;

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

int rank_bits(std::size_t n) {
    int bits = 1;
    while ((UINT64_C(1) << bits) < n) {
        ++bits;
    }
    return bits;
}

GroupElement truncate_to_bits(GroupElement value, int bits) {
    if (bits == 64) {
        return value;
    }
    return value & ((UINT64_C(1) << bits) - 1);
}

std::string case_description(std::size_t n,
                             int bin,
                             int bout,
                             GroupElement alpha,
                             GroupElement payload,
                             GroupElement x) {
    std::ostringstream out;
    out << "n=" << n << ", bin=" << bin << ", bout=" << bout
        << ", alpha=" << alpha << ", payload=" << payload << ", x=" << x;
    return out.str();
}

void verify_local_evaluation(std::size_t n,
                             int bout,
                             GroupElement alpha,
                             GroupElement payload,
                             DPFKeyPack& key0,
                             DPFKeyPack& key1) {
    const int bin = rank_bits(n);
    const GroupElement domain_size = UINT64_C(1) << bin;
    const GroupElement expected_payload = truncate_to_bits(payload, bout);

    for (GroupElement x = 0; x < domain_size; ++x) {
        const GroupElement share0 = evalDPF_Payload(0, key0, x);
        const GroupElement share1 = evalDPF_Payload(1, key1, x);
        const GroupElement actual = truncate_to_bits(share0 + share1, bout);
        const GroupElement expected = x == alpha ? expected_payload : 0;

        require(actual == expected,
                "DPF payload reconstruction mismatch: " +
                    case_description(n, bin, bout, alpha, payload, x));
    }
}

void verify_transmitted_key(std::size_t n,
                            int bout,
                            GroupElement alpha,
                            GroupElement payload,
                            int party,
                            DPFKeyPack& original) {
    const int bin = rank_bits(n);
    const GroupElement domain_size = UINT64_C(1) << bin;
    std::array<char, kWireBufferBytes> wire{};

    char* write_cursor = wire.data();
    Peer sender(&write_cursor);
    sender.send_dpf_keypack(original);
    const std::uint64_t sent_bytes = sender.bytesSent();

    require(write_cursor == wire.data() + sent_bytes,
            "Peer byte counter disagrees with the memory channel cursor");
    require(sent_bytes <= wire.size(), "DPF key exceeded the test wire buffer");

    char* read_cursor = wire.data();
    Dealer receiver(&read_cursor);
    DPFKeyPack received = receiver.recv_dpf_keypack(bin, bout);

    require(receiver.bytesReceived() == sent_bytes,
            "Dealer did not receive the complete serialized DPF key");
    require(read_cursor == write_cursor,
            "Peer/Dealer cursors disagree after DPF key transport");
    require(received.bin == original.bin && received.bout == original.bout,
            "DPF key metadata changed during Peer/Dealer transport");

    for (GroupElement x = 0; x < domain_size; ++x) {
        const GroupElement before = evalDPF_Payload(party, original, x);
        const GroupElement after = evalDPF_Payload(party, received, x);
        require(before == after,
                "DPF party share changed after Peer/Dealer transport: " +
                    case_description(n, bin, bout, alpha, payload, x));
    }

    delete static_cast<MemBuf*>(sender.keyBuf);
    delete static_cast<MemBuf*>(receiver.keyBuf);
}

void verify_case(std::size_t n,
                 int bout,
                 GroupElement alpha,
                 GroupElement payload) {
    const int bin = rank_bits(n);
    auto keys = keyGenDPF(bin, bout, alpha, payload);

    verify_local_evaluation(n, bout, alpha, payload, keys.first, keys.second);
    verify_transmitted_key(n, bout, alpha, payload, 0, keys.first);
    verify_transmitted_key(n, bout, alpha, payload, 1, keys.second);

    freeDPFKeyPackPair(keys);
}

}  // namespace

int main() {
    try {
        for (int i = 0; i < 256; ++i) {
            FSSConfig::prngs[i].SetSeed(
                osuCrypto::toBlock(UINT64_C(0x4450465f4d33),
                                   static_cast<std::uint64_t>(i)));
        }

        constexpr std::array<std::size_t, 6> sizes = {1, 3, 5, 127, 128, 129};
        constexpr std::array<GroupElement, 3> payloads = {
            0,
            1,
            UINT64_C(0x0123456789abcdef),
        };

        std::size_t cases = 0;
        for (const std::size_t n : sizes) {
            const std::array<GroupElement, 2> points = {
                0,
                static_cast<GroupElement>(n - 1),
            };
            for (const GroupElement alpha : points) {
                for (const GroupElement payload : payloads) {
                    verify_case(n, kPayloadBits, alpha, payload);
                    ++cases;
                }
            }
        }

        // Exercise each serialized GroupElement width used by the channel.
        constexpr std::array<int, 5> output_widths = {1, 8, 17, 32, 64};
        for (const int bout : output_widths) {
            verify_case(5, bout, 4, UINT64_C(0x0123456789abcdef));
            ++cases;
        }

        std::cout << "DPF local and Peer/Dealer transport conformance passed: "
                  << cases << " cases\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
