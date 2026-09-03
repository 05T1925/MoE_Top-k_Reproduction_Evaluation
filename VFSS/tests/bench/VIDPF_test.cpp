#include <FSS/dpf.h>
#include <FSS/freekey.h>
#include <FSS/config.h>
#include <FSS/cavern_group_element.h>

#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Crypto/PRNG.h>

#include <utils.h>
#include <backend/FSS_base.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace osuCrypto;

// ============================================================================
// Timing helpers
// ============================================================================

static inline uint64_t nowMicroseconds()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<
            std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now()
                    .time_since_epoch())
            .count());
}

static inline uint64_t nowNanoseconds()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<
            std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now()
                    .time_since_epoch())
            .count());
}

static inline double usToMs(
    uint64_t microseconds)
{
    return
        static_cast<double>(microseconds)
        /
        1000.0;
}

static inline double usPerKey(
    uint64_t microseconds,
    uint64_t numKeys)
{
    if (numKeys == 0)
    {
        return 0.0;
    }

    return
        static_cast<double>(microseconds)
        /
        static_cast<double>(numKeys);
}

// ============================================================================
// PRNG initialization
// ============================================================================

static void initLocalFSSPRNGs()
{
    constexpr uint64_t seedLo =
        0x123456789ABCDEF0ULL;

    constexpr uint64_t seedHi =
        0xFEDCBA9876543210ULL;

    // Initialize all possible framework thread PRNGs.
    for (int i = 0;
         i < 256;
         ++i)
    {
        FSSConfig::prngs[i].SetSeed(
            osuCrypto::toBlock(
                seedLo
                    ^
                    static_cast<uint64_t>(i),

                seedHi
                    +
                    static_cast<uint64_t>(i)));
    }
}

// ============================================================================
// VIDPF serialized key-size helpers
// ============================================================================

// One VIDPF key held by one online party:
//
//   root:
//       1 block = 16 bytes
//
// For every level:
//
//   scw:
//       1 block = 16 bytes
//
//   tcw:
//       1 byte
//
//   ocw:
//       value = WideGroupElement = 16 bytes
//       mac   = WideGroupElement = 16 bytes
//       total = 32 bytes
//
//   cs:
//       VIDPFHash4 = 4 blocks = 64 bytes
//
// Thus:
//
//   one-party key size
//       = 16 + bin * (16 + 1 + 32 + 64)
//       = 16 + 113 * bin.
//
// Public metadata bin and ringBw are not transmitted because the receiver
// already knows them.
static uint64_t vidpfKeyBytesOneParty(
    int bin)
{
    const uint64_t rootBytes =
        sizeof(osuCrypto::block);

    const uint64_t levelBytes =
        sizeof(osuCrypto::block)
        +
        sizeof(uint8_t)
        +
        sizeof(VIDPFPayload)
        +
        sizeof(VIDPFHash4);

    return
        rootBytes
        +
        static_cast<uint64_t>(bin)
            *
            levelBytes;
}

// Each online party produces one verification token consisting of two blocks.
static uint64_t vidpfTokenBytesOneParty()
{
    return sizeof(VIDPFToken);
}

// ============================================================================
// Dataset
// ============================================================================

struct Dataset
{
    std::vector<GroupElement> alpha;

    std::vector<VIDPFPayload> beta;

    std::vector<GroupElement> x;
};

static GroupElement randomGroupElement(
    std::mt19937_64 &rng,
    int bitwidth)
{
    GroupElement value =
        static_cast<GroupElement>(
            rng());

    mod(
        value,
        bitwidth);

    return value;
}

static WideGroupElement randomWideElement(
    std::mt19937_64 &rng,
    int ringBw)
{
    WideGroupElement value(
        static_cast<uint64_t>(
            rng()),

        static_cast<uint64_t>(
            rng()));

    wideMod(
        value,
        ringBw);

    return value;
}

static GroupElement domainMaximum(
    int bin)
{
    if (bin == 64)
    {
        return
            ~GroupElement(0);
    }

    return
        (GroupElement(1) << bin)
        -
        GroupElement(1);
}

static Dataset makeDataset(
    int numKeys,
    int bin,
    int ringBw)
{
    Dataset data;

    data.alpha.resize(
        numKeys);

    data.beta.resize(
        numKeys);

    data.x.resize(
        numKeys);

    std::mt19937_64 rng(
        0x13579BDF2468ACE0ULL
        ^
        static_cast<uint64_t>(bin)
        ^
        (
            static_cast<uint64_t>(ringBw)
            <<
            32
        ));

    const GroupElement maxValue =
        domainMaximum(bin);

    for (int i = 0;
         i < numKeys;
         ++i)
    {
        data.alpha[i] =
            randomGroupElement(
                rng,
                bin);

        data.beta[i].value =
            randomWideElement(
                rng,
                ringBw);

        data.beta[i].mac =
            randomWideElement(
                rng,
                ringBw);

        // Ensure that the target payload is nonzero. Otherwise a correct
        // target-point result cannot be distinguished from a zero result.
        if (wideIsZero(
                data.beta[i].value)
            &&
            wideIsZero(
                data.beta[i].mac))
        {
            data.beta[i].value =
                WideGroupElement(1);

            wideMod(
                data.beta[i].value,
                ringBw);
        }

        switch (i % 4)
        {
            // ------------------------------------------------------------
            // Target query:
            //
            //     x = alpha
            //
            // Expected output:
            //
            //     beta
            // ------------------------------------------------------------

            case 0:
            {
                data.x[i] =
                    data.alpha[i];

                break;
            }

            // ------------------------------------------------------------
            // Non-target query below alpha.
            // ------------------------------------------------------------

            case 1:
            {
                if (data.alpha[i] > 0)
                {
                    data.x[i] =
                        data.alpha[i]
                        -
                        GroupElement(1);
                }
                else
                {
                    data.x[i] =
                        maxValue;
                }

                break;
            }

            // ------------------------------------------------------------
            // Non-target query above alpha.
            // ------------------------------------------------------------

            case 2:
            {
                if (data.alpha[i] < maxValue)
                {
                    data.x[i] =
                        data.alpha[i]
                        +
                        GroupElement(1);
                }
                else
                {
                    data.x[i] = 0;
                }

                break;
            }

            // ------------------------------------------------------------
            // Fully random query.
            // ------------------------------------------------------------

            default:
            {
                data.x[i] =
                    randomGroupElement(
                        rng,
                        bin);

                break;
            }
        }

        mod(
            data.x[i],
            bin);
    }

    return data;
}

// ============================================================================
// Payload helpers
// ============================================================================

static VIDPFPayload addPayloads(
    const VIDPFPayload &first,
    const VIDPFPayload &second,
    int ringBw)
{
    VIDPFPayload result;

    result.value =
        wideAdd(
            first.value,
            second.value,
            ringBw);

    result.mac =
        wideAdd(
            first.mac,
            second.mac,
            ringBw);

    return result;
}

static bool payloadEqual(
    const VIDPFPayload &first,
    const VIDPFPayload &second)
{
    return
        wideEqual(
            first.value,
            second.value)
        &&
        wideEqual(
            first.mac,
            second.mac);
}

static VIDPFPayload zeroPayload()
{
    VIDPFPayload result;

    result.value =
        WideGroupElement(0);

    result.mac =
        WideGroupElement(0);

    return result;
}

// ============================================================================
// Benchmark result
// ============================================================================

struct BenchResult
{
    int bin = 0;

    int ringBw = 0;

    uint64_t numKeys = 0;

    uint64_t keySizeBytesOneParty = 0;

    uint64_t keyPairSizeBytes = 0;

    uint64_t tokenSizeBytesOneParty = 0;

    uint64_t offlineUs = 0;

    uint64_t evalP0Us = 0;

    uint64_t evalP1Us = 0;

    uint64_t verifyUs = 0;

    uint64_t onlineUs = 0;

    uint64_t totalUs = 0;

    uint64_t correct = 0;

    uint64_t honestAccepted = 0;
};

// ============================================================================
// Benchmark
// ============================================================================

static BenchResult benchmarkVIDPF(
    int numKeys,
    int bin,
    int slack,
    int batchSize,
    const Dataset &data)
{
    BenchResult result;

    result.bin =
        bin;

    result.ringBw =
        bin + slack;

    result.numKeys =
        static_cast<uint64_t>(
            numKeys);

    result.keySizeBytesOneParty =
        vidpfKeyBytesOneParty(
            bin);

    result.keyPairSizeBytes =
        2ULL
        *
        result.keySizeBytesOneParty;

    result.tokenSizeBytesOneParty =
        vidpfTokenBytesOneParty();

    const VIDPFPayload zero =
        zeroPayload();

    // ========================================================================
    // Process keys in batches.
    //
    // This prevents 50,000 64-bit VIDPF key pairs from occupying more than
    // roughly 700 MB at once.
    // ========================================================================

    for (int batchStart = 0;
         batchStart < numKeys;
         batchStart += batchSize)
    {
        const int currentBatchSize =
            std::min(
                batchSize,
                numKeys - batchStart);

        // --------------------------------------------------------------------
        // Allocate key and result containers outside timed regions.
        // --------------------------------------------------------------------

        std::vector<VIDPFKeyGenResult>
            keys(currentBatchSize);

        std::vector<
            std::vector<VIDPFQuery>>
            queries(currentBatchSize);

        std::vector<VIDPFEvalResult>
            output0(currentBatchSize);

        std::vector<VIDPFEvalResult>
            output1(currentBatchSize);

        for (int localIndex = 0;
             localIndex < currentBatchSize;
             ++localIndex)
        {
            const int globalIndex =
                batchStart
                +
                localIndex;

            // As in a normal point-function benchmark, evaluate one
            // full-length query for each independently generated key.
            queries[localIndex].emplace_back(
                bin,
                data.x[globalIndex]);
        }

        // ====================================================================
        // Offline: 50,000 independent VIDPF.Gen executions.
        // ====================================================================

        uint64_t start =
            nowMicroseconds();

        for (int localIndex = 0;
             localIndex < currentBatchSize;
             ++localIndex)
        {
            const int globalIndex =
                batchStart
                +
                localIndex;

            keys[localIndex] =
                keyGenVIDPF(
                    bin,
                    result.ringBw,
                    data.alpha[globalIndex],
                    data.beta[globalIndex]);
        }

        result.offlineUs +=
            nowMicroseconds()
            -
            start;

        // ====================================================================
        // Online: P0 Eval.
        // ====================================================================

        start =
            nowMicroseconds();

        for (int localIndex = 0;
             localIndex < currentBatchSize;
             ++localIndex)
        {
            output0[localIndex] =
                evalVIDPF(
                    0,
                    keys[localIndex].key0,
                    queries[localIndex]);
        }

        result.evalP0Us +=
            nowMicroseconds()
            -
            start;

        // ====================================================================
        // Online: P1 Eval.
        // ====================================================================

        start =
            nowMicroseconds();

        for (int localIndex = 0;
             localIndex < currentBatchSize;
             ++localIndex)
        {
            output1[localIndex] =
                evalVIDPF(
                    1,
                    keys[localIndex].key1,
                    queries[localIndex]);
        }

        result.evalP1Us +=
            nowMicroseconds()
            -
            start;

        // ====================================================================
        // Online: verification.
        //
        // Use nanoseconds internally because one token comparison is very
        // short and may otherwise be rounded to zero microseconds.
        // ====================================================================

        uint64_t verifyNanoseconds = 0;

        for (int localIndex = 0;
             localIndex < currentBatchSize;
             ++localIndex)
        {
            const uint64_t verifyStart =
                nowNanoseconds();

            const bool accepted =
                verifyVIDPF(
                    output0[localIndex].mu,
                    output1[localIndex].mu);

            verifyNanoseconds +=
                nowNanoseconds()
                -
                verifyStart;

            if (accepted)
            {
                ++result.honestAccepted;
            }
            else if (result.honestAccepted < 10)
            {
                const int globalIndex =
                    batchStart
                    +
                    localIndex;

                std::cerr
                    << "[VERIFY FAIL]"
                    << " l="
                    << bin
                    << " i="
                    << globalIndex
                    << " alpha="
                    << data.alpha[globalIndex]
                    << " x="
                    << data.x[globalIndex]
                    << "\n";
            }
        }

        result.verifyUs +=
            verifyNanoseconds
            /
            1000ULL;

        // ====================================================================
        // Functional correctness.
        //
        // At a full-length query:
        //
        //     y0 + y1 = beta, if x = alpha;
        //     y0 + y1 = 0,    otherwise.
        // ====================================================================

        for (int localIndex = 0;
             localIndex < currentBatchSize;
             ++localIndex)
        {
            const int globalIndex =
                batchStart
                +
                localIndex;

            if (output0[localIndex].y.size() != 1
                ||
                output1[localIndex].y.size() != 1)
            {
                std::cerr
                    << "[SIZE FAIL]"
                    << " l="
                    << bin
                    << " i="
                    << globalIndex
                    << "\n";

                continue;
            }

            const VIDPFPayload reconstructed =
                addPayloads(
                    output0[localIndex].y[0],
                    output1[localIndex].y[0],
                    result.ringBw);

            const VIDPFPayload &expected =
                data.x[globalIndex]
                    ==
                    data.alpha[globalIndex]
                ?
                    data.beta[globalIndex]
                :
                    zero;

            if (payloadEqual(
                    reconstructed,
                    expected))
            {
                ++result.correct;
            }
            else if (result.correct < 10)
            {
                std::cerr
                    << "[FUNCTION FAIL]"
                    << " l="
                    << bin
                    << " i="
                    << globalIndex
                    << " alpha="
                    << data.alpha[globalIndex]
                    << " x="
                    << data.x[globalIndex]
                    << "\n";

                std::cerr
                    << "  got.value = {hi="
                    << reconstructed.value.hi
                    << ", lo="
                    << reconstructed.value.lo
                    << "}\n";

                std::cerr
                    << "  expected.value = {hi="
                    << expected.value.hi
                    << ", lo="
                    << expected.value.lo
                    << "}\n";

                std::cerr
                    << "  got.mac = {hi="
                    << reconstructed.mac.hi
                    << ", lo="
                    << reconstructed.mac.lo
                    << "}\n";

                std::cerr
                    << "  expected.mac = {hi="
                    << expected.mac.hi
                    << ", lo="
                    << expected.mac.lo
                    << "}\n";
            }
        }

        // ====================================================================
        // Release raw-pointer key material.
        // ====================================================================

        for (int localIndex = 0;
             localIndex < currentBatchSize;
             ++localIndex)
        {
            freeVIDPFKeyPack(
                keys[localIndex].key0);

            freeVIDPFKeyPack(
                keys[localIndex].key1);
        }
    }

    // ========================================================================
    // Latency convention.
    //
    //     Online =
    //         Eval(P0)
    //         +
    //         Eval(P1)
    //         +
    //         Verify
    //
    //     Total =
    //         Offline
    //         +
    //         Online
    // ========================================================================

    result.onlineUs =
        result.evalP0Us
        +
        result.evalP1Us
        +
        result.verifyUs;

    result.totalUs =
        result.offlineUs
        +
        result.onlineUs;

    return result;
}

// ============================================================================
// Detailed result output
// ============================================================================

static void printResult(
    const BenchResult &result)
{
    std::cout
        << "\n"
        << "[VIDPF, l="
        << result.bin
        << ", ringBw="
        << result.ringBw
        << "]\n";

    std::cout
        << "  one-party key size       = "
        << result.keySizeBytesOneParty
        << " bytes\n";

    std::cout
        << "  key-pair size            = "
        << result.keyPairSizeBytes
        << " bytes\n";

    std::cout
        << "  one-party token size     = "
        << result.tokenSizeBytesOneParty
        << " bytes\n";

    std::cout
        << "  offline                  = "
        << usToMs(
            result.offlineUs)
        << " ms\n";

    std::cout
        << "  offline per key          = "
        << usPerKey(
            result.offlineUs,
            result.numKeys)
        << " us\n";

    std::cout
        << "  eval P0                  = "
        << usToMs(
            result.evalP0Us)
        << " ms\n";

    std::cout
        << "  eval P1                  = "
        << usToMs(
            result.evalP1Us)
        << " ms\n";

    std::cout
        << "  verify                   = "
        << usToMs(
            result.verifyUs)
        << " ms\n";

    std::cout
        << "  online                   = "
        << usToMs(
            result.onlineUs)
        << " ms\n";

    std::cout
        << "  online per key           = "
        << usPerKey(
            result.onlineUs,
            result.numKeys)
        << " us\n";

    std::cout
        << "  total                    = "
        << usToMs(
            result.totalUs)
        << " ms\n";

    std::cout
        << "  correctness              = "
        << result.correct
        << "/"
        << result.numKeys
        << "\n";

    std::cout
        << "  honest verify accepted   = "
        << result.honestAccepted
        << "/"
        << result.numKeys
        << "\n";
}

// ============================================================================
// Final table
// ============================================================================

static void printFinalTable(
    const std::vector<BenchResult> &results)
{
    std::cout
        << "\n"
        << "============================================================================================================\n"
        << "Final VIDPF Table Data\n"
        << "============================================================================================================\n";

    std::cout
        << std::left

        << std::setw(12)
        << "Protocol"

        << std::setw(8)
        << "l"

        << std::setw(10)
        << "RingBw"

        << std::setw(14)
        << "Key(B)"

        << std::setw(14)
        << "Token(B)"

        << std::setw(16)
        << "Offline(ms)"

        << std::setw(16)
        << "Online(ms)"

        << std::setw(16)
        << "Verify(ms)"

        << std::setw(16)
        << "Total(ms)"

        << "\n";

    std::cout
        << "------------------------------------------------------------------------------------------------------------\n";

    for (const BenchResult &result : results)
    {
        std::cout
            << std::left

            << std::setw(12)
            << "VIDPF"

            << std::setw(8)
            << result.bin

            << std::setw(10)
            << result.ringBw

            << std::setw(14)
            << result.keySizeBytesOneParty

            << std::setw(14)
            << result.tokenSizeBytesOneParty

            << std::setw(16)
            << std::fixed
            << std::setprecision(3)
            << usToMs(
                result.offlineUs)

            << std::setw(16)
            << usToMs(
                result.onlineUs)

            << std::setw(16)
            << usToMs(
                result.verifyUs)

            << std::setw(16)
            << usToMs(
                result.totalUs)

            << "\n";
    }

    std::cout
        << "============================================================================================================\n";
}

// ============================================================================
// LaTeX-ready table rows
// ============================================================================

static void printLatexRows(
    const std::vector<BenchResult> &results)
{
    std::cout
        << "\n"
        << "LaTeX-ready rows:\n\n";

    for (const BenchResult &result : results)
    {
        std::cout
            << "VIDPF"
            << " & "
            << result.bin
            << " & "
            << result.keySizeBytesOneParty
            << " & "
            << std::fixed
            << std::setprecision(3)
            << usToMs(
                result.offlineUs)
            << " & "
            << usToMs(
                result.onlineUs)
            << " & "
            << usToMs(
                result.totalUs)
            << " \\\\\n";
    }
}

// ============================================================================
// main
// ============================================================================

int main(
    int argc,
    char **argv)
{
    int numKeys =
        50000;

    int slack =
        64;

    int batchSize =
        1000;

    if (argc > 1)
    {
        numKeys =
            std::atoi(
                argv[1]);
    }

    if (argc > 2)
    {
        slack =
            std::atoi(
                argv[2]);
    }

    if (argc > 3)
    {
        batchSize =
            std::atoi(
                argv[3]);
    }

    if (numKeys <= 0)
    {
        std::cerr
            << "numKeys must be positive\n";

        return 1;
    }

    if (slack <= 0
        ||
        64 + slack > 128)
    {
        std::cerr
            << "slack must satisfy 1 <= slack <= 64\n";

        return 1;
    }

    if (batchSize <= 0)
    {
        std::cerr
            << "batchSize must be positive\n";

        return 1;
    }

    sytorch_init();

    FSSConfig::num_threads =
        1;

    FSSConfig::bitlength =
        64;

    initLocalFSSPRNGs();

    const std::array<int, 3> bitLengths =
    {
        16,
        32,
        64
    };

    std::vector<BenchResult>
        results;

    results.reserve(
        bitLengths.size());

    std::cout
        << std::fixed
        << std::setprecision(3);

    std::cout
        << "============================================================\n"
        << "CAVERN VIDPF Microbenchmark\n"
        << "============================================================\n"
        << "  numKeys          = "
        << numKeys
        << "\n"
        << "  slack            = "
        << slack
        << "\n"
        << "  ringBw           = bin + slack\n"
        << "  l                = {16, 32, 64}\n"
        << "  queries per key  = 1 full-length query\n"
        << "  batch size       = "
        << batchSize
        << "\n"
        << "  online           = P0 Eval + P1 Eval + Verify\n";

    for (const int bin : bitLengths)
    {
        const int ringBw =
            bin
            +
            slack;

        std::cout
            << "\n"
            << "============================================================\n"
            << "Running VIDPF, l="
            << bin
            << ", ringBw="
            << ringBw
            << "\n"
            << "============================================================\n";

        const Dataset data =
            makeDataset(
                numKeys,
                bin,
                ringBw);

        const BenchResult result =
            benchmarkVIDPF(
                numKeys,
                bin,
                slack,
                batchSize,
                data);

        printResult(
            result);

        results.push_back(
            result);
    }

    printFinalTable(
        results);

    printLatexRows(
        results);

    // ========================================================================
    // Final PASS / FAIL summary.
    // ========================================================================

    bool allPass =
        true;

    std::cout
        << "\n"
        << "============================================================\n"
        << "Correctness and Verification Summary\n"
        << "============================================================\n";

    for (const BenchResult &result : results)
    {
        const bool correctnessPass =
            result.correct
            ==
            result.numKeys;

        const bool verificationPass =
            result.honestAccepted
            ==
            result.numKeys;

        const bool pass =
            correctnessPass
            &&
            verificationPass;

        allPass =
            allPass
            &&
            pass;

        std::cout
            << "  VIDPF l="
            << std::setw(2)
            << result.bin

            << "  correctness="
            << result.correct
            << "/"
            << result.numKeys

            << "  honest="
            << result.honestAccepted
            << "/"
            << result.numKeys

            << "  "
            << (
                pass
                ?
                    "PASS"
                :
                    "FAIL"
            )

            << "\n";
    }

    std::cout
        << "\nOverall = "
        << (
            allPass
            ?
                "PASS"
            :
                "FAIL"
        )
        << "\n";

    return
        allPass
        ?
            0
        :
            1;
}