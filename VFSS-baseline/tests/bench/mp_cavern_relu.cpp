#include <backend/FSS_transformer.h>
#include <layers/layers.h>
#include <module.h>

#include <FSS/api.h>
#include <FSS/config.h>
#include <FSS/utils.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace
{

uint64_t splitmix64(uint64_t value)
{
    value += 0x9E3779B97F4A7C15ULL;

    value =
        (value ^ (value >> 30)) *
        0xBF58476D1CE4E5B9ULL;

    value =
        (value ^ (value >> 27)) *
        0x94D049BB133111EBULL;

    return value ^ (value >> 31);
}

GroupElement deterministicMask(
    std::size_t index)
{
    return static_cast<GroupElement>(
        splitmix64(
            0xA93E6C4F17B2D850ULL +
            static_cast<uint64_t>(index)));
}

GroupElement deterministicSignedInput(
    std::size_t index)
{
    // Explicit boundary cases.
    static const int64_t edgeValues[] =
    {
        int64_t(0),
        int64_t(1),
        int64_t(-1),
        int64_t(2),
        int64_t(-2),
        int64_t(7),
        int64_t(-7),
        int64_t(123456789),
        int64_t(-123456789),
        std::numeric_limits<int64_t>::max(),
        std::numeric_limits<int64_t>::min(),
        int64_t(1) << 40,
        -(int64_t(1) << 40),
        (int64_t(1) << 32) - 1,
        -((int64_t(1) << 32) - 1)
    };

    constexpr std::size_t edgeCount =
        sizeof(edgeValues) /
        sizeof(edgeValues[0]);

    if (index < edgeCount)
    {
        return static_cast<GroupElement>(
            edgeValues[index]);
    }

    // Generate deterministic signed values with magnitudes below 2^40.
    const uint64_t randomValue =
        splitmix64(
            0x5EED1234A5A5A5A5ULL +
            static_cast<uint64_t>(index));

    const uint64_t magnitude =
        randomValue &
        ((uint64_t(1) << 40) - 1);

    int64_t signedValue =
        static_cast<int64_t>(
            magnitude);

    if ((randomValue >> 63) != 0)
    {
        signedValue =
            -signedValue;
    }

    return static_cast<GroupElement>(
        signedValue);
}

GroupElement clearRelu(
    GroupElement value,
    int bitlength)
{
    const GroupElement signBit =
        (value >> (bitlength - 1)) &
        GroupElement(1);

    return
        signBit != 0
            ? GroupElement(0)
            : value;
}

} // namespace

int main(
    int argc,
    char **argv)
{
    sytorch_init();

    if (argc < 2)
    {
        std::cerr
            << "Usage: "
            << argv[0]
            << " <party_id> [ip]"
            << std::endl;

        return 1;
    }

    const int partyId =
        std::atoi(argv[1]);

    const std::string ip =
        argc > 2
            ? argv[2]
            : "127.0.0.1";

    if (partyId != DEALER &&
        partyId != SERVER &&
        partyId != CLIENT)
    {
        std::cerr
            << "Invalid party id: "
            << partyId
            << std::endl;

        return 1;
    }

    using FSSVersion =
        FSSTransformer<u64>;

    FSSVersion *backend =
        new FSSVersion();

    constexpr int bitlength = 64;
    constexpr int slack = 64;
    constexpr int32_t numSamples = 10000;

    FSSConfig::bitlength = bitlength;
    FSSConfig::party = partyId;
    FSSConfig::num_threads = 4;

    backend->init(
        ip,
        true);

    std::vector<GroupElement>
        input(numSamples, 0);

    std::vector<GroupElement>
        inputMask(numSamples, 0);

    std::vector<GroupElement>
        output(numSamples, 0);

    std::vector<GroupElement>
        outputMask(numSamples, 0);

    std::vector<GroupElement>
        clearInput(numSamples, 0);

    for (int32_t i = 0;
         i < numSamples;
         ++i)
    {
        const GroupElement value =
            deterministicSignedInput(
                static_cast<std::size_t>(i));

        const GroupElement mask =
            deterministicMask(
                static_cast<std::size_t>(i));

        clearInput[i] = value;

        if (partyId == DEALER)
        {
            inputMask[i] = mask;
            input[i] = 0;
        }
        else
        {
            // Both online evaluators hold the same masked input x+r.
            input[i] =
                value + mask;

            inputMask[i] = 0;
        }
    }

    FSS::start();

    const auto start =
        std::chrono::high_resolution_clock::now();

    CavernRelu(
        numSamples,
        input.data(),
        inputMask.data(),
        output.data(),
        outputMask.data(),
        slack,
        "CAVERN-ReLU-Test::");

    const auto end =
        std::chrono::high_resolution_clock::now();

    FSS::end();

    const uint64_t protocolMicroseconds =
        static_cast<uint64_t>(
            std::chrono::duration_cast<
                std::chrono::microseconds>(
                    end - start)
                .count());

    std::cout
        << "[Party "
        << partyId
        << "] CAVERN ReLU protocol time = "
        << protocolMicroseconds
        << " us"
        << std::endl;

    // Send the output mask only for clear-result verification.
    if (partyId == DEALER)
    {
        FSSConfig::client->send_batched_input(
            outputMask.data(),
            numSamples,
            bitlength);
    }
    else if (partyId == CLIENT)
    {
        std::vector<GroupElement>
            receivedOutputMask(
                numSamples,
                0);

        FSSConfig::dealer->recv_ge_array(
            receivedOutputMask.data(),
            numSamples);

        int correct = 0;

        for (int32_t i = 0;
             i < numSamples;
             ++i)
        {
            const GroupElement clearOutput =
                output[i] -
                receivedOutputMask[i];

            const GroupElement expected =
                clearRelu(
                    clearInput[i],
                    bitlength);

            if (clearOutput == expected)
            {
                ++correct;
            }
            else
            {
                std::cerr
                    << "ReLU mismatch at index "
                    << i
                    << ": input="
                    << static_cast<int64_t>(
                        clearInput[i])
                    << ", expected="
                    << static_cast<int64_t>(
                        expected)
                    << ", got="
                    << static_cast<int64_t>(
                        clearOutput)
                    << std::endl;
            }
        }

        std::cout
            << "CAVERN ReLU correctness: "
            << correct
            << "/"
            << numSamples
            << std::endl;

        if (correct != numSamples)
        {
            std::cerr
                << "CAVERN ReLU test: FAIL"
                << std::endl;

            backend->finalize();
            delete backend;

            return 1;
        }

        std::cout
            << "CAVERN ReLU test: PASS"
            << std::endl;
    }

    backend->finalize();
    delete backend;

    return 0;
}