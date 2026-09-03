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
#include <string>
#include <vector>

namespace
{

// Stateless deterministic generator.
//
// Every process derives the same test values and masks without depending on
// the state of std::rand(). This is only for protocol testing.
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
            0xC4A6E25A91D37B0FULL +
            static_cast<uint64_t>(index)));
}

// The clear spline used by this test:
//
//   [0, 10)     : x^2 + 3
//   [10, 20)    : 2x + 1
//   [20, 2^64)  : 5
//
// Arithmetic follows uint64_t ring semantics.
GroupElement clearSpline(
    GroupElement value)
{
    if (value < 10)
    {
        return
            value * value +
            GroupElement(3);
    }

    if (value < 20)
    {
        return
            GroupElement(2) * value +
            GroupElement(1);
    }

    return GroupElement(5);
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

    FSSConfig::bitlength = 64;
    FSSConfig::party = partyId;
    FSSConfig::num_threads = 4;

    backend->init(
        ip,
        true);

    // The last endpoint 0 positionally represents 2^64.
    const std::vector<GroupElement> endpoints =
    {
        GroupElement(0),
        GroupElement(10),
        GroupElement(20),
        GroupElement(0)
    };

    // Coefficients are stored in ascending order:
    //
    //   polynomial[j][0]
    // + polynomial[j][1] * x
    // + polynomial[j][2] * x^2
    //
    const std::vector<
        std::vector<GroupElement>>
        coefficients =
    {
        // p_0(x) = x^2 + 3
        {
            GroupElement(3),
            GroupElement(0),
            GroupElement(1)
        },

        // p_1(x) = 2x + 1
        {
            GroupElement(1),
            GroupElement(2),
            GroupElement(0)
        },

        // p_2(x) = 5
        {
            GroupElement(5),
            GroupElement(0),
            GroupElement(0)
        }
    };

    constexpr int32_t numSamples = 10000;
    constexpr int slack = 64;

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

    // Test all values from 0 through 63. This includes all interval
    // boundaries and their neighboring values.
    for (int32_t i = 0;
         i < numSamples;
         ++i)
    {
        const GroupElement value =
            static_cast<GroupElement>(i);

        const GroupElement mask =
            deterministicMask(
                static_cast<std::size_t>(i));

        clearInput[i] = value;

        if (partyId == DEALER)
        {
            // Dealer owns the input mask used during preprocessing.
            inputMask[i] = mask;
            input[i] = 0;
        }
        else
        {
            // Both online parties receive the same masked input x+r.
            input[i] =
                value + mask;

            inputMask[i] = 0;
        }
    }

    FSS::start();

    const auto start =
        std::chrono::high_resolution_clock::now();

    CavernSpline(
        numSamples,
        input.data(),
        inputMask.data(),
        output.data(),
        outputMask.data(),
        endpoints,
        coefficients,
        slack,
        "CAVERN-Spline-Test::");

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
        << "] CAVERN spline protocol time = "
        << protocolMicroseconds
        << " us"
        << std::endl;

    // Dealer sends the final output masks to the client only for testing.
    // This operation is outside the measured protocol invocation.
    if (partyId == DEALER)
    {
        FSSConfig::client->send_batched_input(
            outputMask.data(),
            numSamples,
            64);
    }
    else if (partyId == CLIENT)
    {
        std::vector<GroupElement>
            receivedOutputMask(
                numSamples,
                0);

        // The existing framework provides:
        //
        // Dealer::recv_ge_array(const GroupElement *buffer, int size)
        //
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
                clearSpline(
                    clearInput[i]);

            if (clearOutput == expected)
            {
                ++correct;
            }
            else
            {
                std::cerr
                    << "Spline mismatch at index "
                    << i
                    << ": input="
                    << clearInput[i]
                    << ", expected="
                    << expected
                    << ", got="
                    << clearOutput
                    << std::endl;
            }
        }

        std::cout
            << "CAVERN spline correctness: "
            << correct
            << "/"
            << numSamples
            << std::endl;

        if (correct != numSamples)
        {
            std::cerr
                << "CAVERN spline test: FAIL"
                << std::endl;

            backend->finalize();
            delete backend;

            return 1;
        }

        std::cout
            << "CAVERN spline test: PASS"
            << std::endl;
    }

    backend->finalize();
    delete backend;

    return 0;
}