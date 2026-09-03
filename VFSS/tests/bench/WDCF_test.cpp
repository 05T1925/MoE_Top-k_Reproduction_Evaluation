#include <FSS/dpf.h>
#include <FSS/dcf.h>
#include <FSS/freekey.h>
#include <FSS/group_element.h>
#include <FSS/config.h>
#include <FSS/prng.h>

#include <cryptoTools/Common/Defines.h>

#include <utils.h>
#include <backend/FSS_base.h>

#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <string>
#include <array>
#include <cstdint>

using namespace osuCrypto;

// In case the declaration is not yet present in dpf.h.
GroupElement evalDPF_LT(
    int party,
    DPFKeyPack &key,
    GroupElement x
);

// ============================================================================
// Timing
// ============================================================================

static inline uint64_t nowMicroseconds()
{
    auto now =
        std::chrono::high_resolution_clock::now();

    return static_cast<uint64_t>(
        std::chrono::duration_cast<
            std::chrono::microseconds
        >(
            now.time_since_epoch()
        ).count()
    );
}

static inline double usToMs(
    uint64_t us)
{
    return static_cast<double>(us) /
           1000.0;
}

// ============================================================================
// PRNG
// ============================================================================

static void initLocalFSSPRNGs()
{
    const uint64_t seed0 =
        0x123456789abcdef0ULL;

    const uint64_t seed1 =
        0xfedcba9876543210ULL;

    for (int i = 0;
         i < 256;
         ++i)
    {
        FSSConfig::prngs[i].SetSeed(
            osuCrypto::toBlock(
                seed0 ^
                    static_cast<uint64_t>(i),
                seed1 +
                    static_cast<uint64_t>(i)
            )
        );
    }

    prngShared.SetSeed(
        osuCrypto::toBlock(
            0x1111222233334444ULL,
            0x5555666677778888ULL
        )
    );
}

// ============================================================================
// Wire-size helpers
// ============================================================================

static inline uint64_t geWireBytes(
    int bw)
{
    if (bw > 32) return 8;
    if (bw > 16) return 4;
    if (bw > 8) return 2;

    return 1;
}

// ----------------------------------------------------------------------------
// Traditional DCF:
//
//   k[0 ... bin]     : (bin+1) blocks
//   v[0 ... bin-1]   : bin group elements
//   g                 : one final group element
// ----------------------------------------------------------------------------

static uint64_t traditionalDCFKeyBytes(
    int bin,
    int bout)
{
    return
        static_cast<uint64_t>(bin + 1) * 16ULL
        +
        static_cast<uint64_t>(bin) *
            geWireBytes(bout)
        +
        geWireBytes(bout);
}

// ----------------------------------------------------------------------------
// Current engineering Half-Tree DCF:
//
//   Half-Tree DPF key
//   +
//   bin VCWs
// ----------------------------------------------------------------------------

static uint64_t halfTreeDPFKeyBytes(
    int bin,
    int bout)
{
    return
        16ULL                         // root
        + 16ULL                       // hashKey
        + static_cast<uint64_t>(
              bin - 1
          ) * 16ULL                   // cw[]
        + 16ULL                       // hcw
        + 1ULL                        // lcw
        + geWireBytes(bout);          // outCW
}

static uint64_t halfTreeDCFKeyBytes(
    int bin,
    int bout)
{
    return
        halfTreeDPFKeyBytes(
            bin,
            bout
        )
        +
        static_cast<uint64_t>(bin) *
            geWireBytes(bout);
}

// ----------------------------------------------------------------------------
// Grotto-style baseline using current DPFKeyPack:
//
//   DPF key with bout = 1
// ----------------------------------------------------------------------------

static uint64_t grottoKeyBytes(
    int bin)
{
    return
        static_cast<uint64_t>(
            bin + 1
        ) * 16ULL
        +
        2ULL * geWireBytes(bin)
        +
        geWireBytes(1);
}

// ----------------------------------------------------------------------------
// Four-ary WDCF:
//
//   seed                 : 16 B
//
//   per level:
//       4 seed CWs       : 4 * 16 B
//       packed tCW       : 1 B
//       4 value CWs      : 4 * geWireBytes(bout)
//
//   final CW             : geWireBytes(bout)
// ----------------------------------------------------------------------------

static uint64_t wdCFKeyBytes(
    int bin,
    int bout)
{
    const uint64_t depth =
        static_cast<uint64_t>(
            (bin + 1) / 2
        );

    return
        16ULL
        +
        depth * 4ULL * 16ULL
        +
        depth
        +
        depth * 4ULL *
            geWireBytes(bout)
        +
        geWireBytes(bout);
}

// ============================================================================
// Dataset
// ============================================================================

static inline GroupElement randomValue(
    std::mt19937_64 &rng,
    int bw)
{
    GroupElement x =
        static_cast<GroupElement>(
            rng()
        );

    mod(x, bw);

    return x;
}

struct Dataset
{
    std::vector<GroupElement> alpha;
    std::vector<GroupElement> beta;
    std::vector<GroupElement> x;
};

static Dataset makeDataset(
    int numKeys,
    int bin,
    int bout)
{
    Dataset data;

    data.alpha.resize(numKeys);
    data.beta.resize(numKeys);
    data.x.resize(numKeys);

    std::mt19937_64 rng(
        0x1234abcd5678ef90ULL
        ^
        static_cast<uint64_t>(bin)
    );

    for (int i = 0;
         i < numKeys;
         ++i)
    {
        data.alpha[i] =
            randomValue(
                rng,
                bin
            );

        data.beta[i] =
            randomValue(
                rng,
                bout
            );

        // Force a balanced set of useful cases.
        if (
            (i % 2 == 0)
            &&
            data.alpha[i] > 0
        )
        {
            data.x[i] =
                data.alpha[i] - 1;
        }
        else
        {
            data.x[i] =
                randomValue(
                    rng,
                    bin
                );
        }
    }

    return data;
}

// ============================================================================
// Result
// ============================================================================

struct BenchResult
{
    std::string protocol;

    int bin = 0;

    uint64_t keySizeBytes = 0;

    uint64_t offlineUs = 0;

    uint64_t evalP0Us = 0;
    uint64_t evalP1Us = 0;

    uint64_t onlineUs = 0;
    uint64_t totalUs = 0;

    uint64_t correct = 0;
    uint64_t numKeys = 0;
};

// ============================================================================
// Traditional DCF
// ============================================================================

static BenchResult benchmarkTraditionalDCF(
    int numKeys,
    int bin,
    int bout,
    const Dataset &data)
{
    BenchResult r;

    r.protocol = "DCF";
    r.bin = bin;
    r.numKeys = numKeys;

    r.keySizeBytes =
        traditionalDCFKeyBytes(
            bin,
            bout
        );

    std::vector<
        std::pair<
            DCFKeyPack,
            DCFKeyPack
        >
    > keys(numKeys);

    // Offline
    uint64_t start =
        nowMicroseconds();

    for (int i = 0;
         i < numKeys;
         ++i)
    {
        keys[i] =
            keyGenDCF(
                bin,
                bout,
                data.alpha[i],
                data.beta[i]
            );
    }

    r.offlineUs =
        nowMicroseconds() - start;

    std::vector<GroupElement>
        y0(numKeys);

    std::vector<GroupElement>
        y1(numKeys);

    // P0
    start =
        nowMicroseconds();

    for (int i = 0;
         i < numKeys;
         ++i)
    {
        GroupElement out[1] = {0};

        evalDCF(
            0,
            out,
            data.x[i],
            keys[i].first
        );

        y0[i] = out[0];
        mod(y0[i], bout);
    }

    r.evalP0Us =
        nowMicroseconds() - start;

    // P1
    start =
        nowMicroseconds();

    for (int i = 0;
         i < numKeys;
         ++i)
    {
        GroupElement out[1] = {0};

        evalDCF(
            1,
            out,
            data.x[i],
            keys[i].second
        );

        y1[i] = out[0];
        mod(y1[i], bout);
    }

    r.evalP1Us =
        nowMicroseconds() - start;

    r.onlineUs =
        r.evalP0Us +
        r.evalP1Us;

    r.totalUs =
        r.offlineUs +
        r.onlineUs;

    // Correctness
    for (int i = 0;
         i < numKeys;
         ++i)
    {
        GroupElement result =
            y0[i] + y1[i];

        mod(result, bout);

        GroupElement expected =
            (
                data.x[i]
                <
                data.alpha[i]
            )
            ? data.beta[i]
            : 0;

        mod(expected, bout);

        if (result == expected)
        {
            ++r.correct;
        }
        else if (i < 10)
        {
            std::cerr
                << "[DCF FAIL]"
                << " l=" << bin
                << " i=" << i
                << " x=" << data.x[i]
                << " alpha="
                << data.alpha[i]
                << " beta="
                << data.beta[i]
                << " got="
                << result
                << " expected="
                << expected
                << "\n";
        }
    }

    for (int i = 0;
         i < numKeys;
         ++i)
    {
        freeDCFKeyPackPair(
            keys[i]
        );
    }

    return r;
}

// ============================================================================
// Half-Tree DCF
// ============================================================================

static BenchResult benchmarkHalfTreeDCF(
    int numKeys,
    int bin,
    int bout,
    const Dataset &data)
{
    BenchResult r;

    r.protocol = "Half-Tree DCF";
    r.bin = bin;
    r.numKeys = numKeys;

    r.keySizeBytes =
        halfTreeDCFKeyBytes(
            bin,
            bout
        );

    std::vector<
        std::pair<
            HalfTreeDCFKeyPack,
            HalfTreeDCFKeyPack
        >
    > keys(numKeys);

    uint64_t start =
        nowMicroseconds();

    for (int i = 0;
         i < numKeys;
         ++i)
    {
        keys[i] =
            keyGenHalfTreeDCF(
                bin,
                bout,
                data.alpha[i],
                data.beta[i]
            );
    }

    r.offlineUs =
        nowMicroseconds() - start;

    std::vector<GroupElement>
        y0(numKeys);

    std::vector<GroupElement>
        y1(numKeys);

    start =
        nowMicroseconds();

    for (int i = 0;
         i < numKeys;
         ++i)
    {
        y0[i] =
            evalHalfTreeDCF(
                0,
                keys[i].first,
                data.x[i]
            );

        mod(y0[i], bout);
    }

    r.evalP0Us =
        nowMicroseconds() - start;

    start =
        nowMicroseconds();

    for (int i = 0;
         i < numKeys;
         ++i)
    {
        y1[i] =
            evalHalfTreeDCF(
                1,
                keys[i].second,
                data.x[i]
            );

        mod(y1[i], bout);
    }

    r.evalP1Us =
        nowMicroseconds() - start;

    r.onlineUs =
        r.evalP0Us +
        r.evalP1Us;

    r.totalUs =
        r.offlineUs +
        r.onlineUs;

    for (int i = 0;
         i < numKeys;
         ++i)
    {
        GroupElement result =
            y0[i] + y1[i];

        mod(result, bout);

        GroupElement expected =
            (
                data.x[i]
                <
                data.alpha[i]
            )
            ? data.beta[i]
            : 0;

        mod(expected, bout);

        if (result == expected)
        {
            ++r.correct;
        }
        else if (i < 10)
        {
            std::cerr
                << "[Half-Tree DCF FAIL]"
                << " l=" << bin
                << " i=" << i
                << "\n";
        }
    }

    for (int i = 0;
         i < numKeys;
         ++i)
    {
        freeHalfTreeDCFKeyPackPair(
            keys[i]
        );
    }

    return r;
}

// ============================================================================
// Grotto-style DPF comparison
//
// Current framework implementation:
//
//     keyGenDPF + evalDPF_LT
//
// Output sharing:
//     Boolean / XOR
//
// Reconstruction:
//     y = y0 XOR y1
// ============================================================================

static BenchResult benchmarkGrotto(
    int numKeys,
    int bin,
    const Dataset &data)
{
    BenchResult r;

    r.protocol = "Grotto";
    r.bin = bin;
    r.numKeys = numKeys;

    r.keySizeBytes =
        grottoKeyBytes(bin);

    std::vector<
        std::pair<
            DPFKeyPack,
            DPFKeyPack
        >
    > keys(numKeys);

    uint64_t start =
        nowMicroseconds();

    for (int i = 0;
         i < numKeys;
         ++i)
    {
        keys[i] =
            keyGenDPF(
                bin,
                1,
                data.alpha[i],
                1
            );
    }

    r.offlineUs =
        nowMicroseconds() - start;

    std::vector<GroupElement>
        y0(numKeys);

    std::vector<GroupElement>
        y1(numKeys);

    start =
        nowMicroseconds();

    for (int i = 0;
         i < numKeys;
         ++i)
    {
        y0[i] =
            evalDPF_LT(
                0,
                keys[i].first,
                data.x[i]
            ) & 1ULL;
    }

    r.evalP0Us =
        nowMicroseconds() - start;

    start =
        nowMicroseconds();

    for (int i = 0;
         i < numKeys;
         ++i)
    {
        y1[i] =
            evalDPF_LT(
                1,
                keys[i].second,
                data.x[i]
            ) & 1ULL;
    }

    r.evalP1Us =
        nowMicroseconds() - start;

    r.onlineUs =
        r.evalP0Us +
        r.evalP1Us;

    r.totalUs =
        r.offlineUs +
        r.onlineUs;

    for (int i = 0;
         i < numKeys;
         ++i)
    {
        const GroupElement result =
            (
                y0[i] ^
                y1[i]
            ) & 1ULL;

        const GroupElement expected =
            (
                data.x[i]
                <
                data.alpha[i]
            )
            ? 1
            : 0;

        if (result == expected)
        {
            ++r.correct;
        }
        else if (i < 10)
        {
            std::cerr
                << "[Grotto FAIL]"
                << " l=" << bin
                << " i=" << i
                << " x=" << data.x[i]
                << " alpha="
                << data.alpha[i]
                << " got=" << result
                << " expected="
                << expected
                << "\n";
        }
    }

    for (int i = 0;
         i < numKeys;
         ++i)
    {
        freeDPFKeyPackPair(
            keys[i]
        );
    }

    return r;
}

// ============================================================================
// WDCF
// ============================================================================

static BenchResult benchmarkWDCF(
    int numKeys,
    int bin,
    int bout,
    const Dataset &data)
{
    BenchResult r;

    r.protocol = "WDCF";
    r.bin = bin;
    r.numKeys = numKeys;

    r.keySizeBytes =
        wdCFKeyBytes(
            bin,
            bout
        );

    std::vector<
        std::pair<
            WDCFKeyPack,
            WDCFKeyPack
        >
    > keys(numKeys);

    uint64_t start =
        nowMicroseconds();

    for (int i = 0;
         i < numKeys;
         ++i)
    {
        keys[i] =
            keyGenWDCF(
                bin,
                bout,
                data.alpha[i],
                data.beta[i]
            );
    }

    r.offlineUs =
        nowMicroseconds() - start;

    std::vector<GroupElement>
        y0(numKeys);

    std::vector<GroupElement>
        y1(numKeys);

    start =
        nowMicroseconds();

    for (int i = 0;
         i < numKeys;
         ++i)
    {
        y0[i] =
            evalWDCF(
                0,
                keys[i].first,
                data.x[i]
            );
    }

    r.evalP0Us =
        nowMicroseconds() - start;

    start =
        nowMicroseconds();

    for (int i = 0;
         i < numKeys;
         ++i)
    {
        y1[i] =
            evalWDCF(
                1,
                keys[i].second,
                data.x[i]
            );
    }

    r.evalP1Us =
        nowMicroseconds() - start;

    r.onlineUs =
        r.evalP0Us +
        r.evalP1Us;

    r.totalUs =
        r.offlineUs +
        r.onlineUs;

    for (int i = 0;
         i < numKeys;
         ++i)
    {
        GroupElement result =
            y0[i] + y1[i];

        mod(result, bout);

        GroupElement expected =
            (
                data.x[i]
                <
                data.alpha[i]
            )
            ? data.beta[i]
            : 0;

        mod(expected, bout);

        if (result == expected)
        {
            ++r.correct;
        }
        else if (i < 10)
        {
            std::cerr
                << "[WDCF FAIL]"
                << " l=" << bin
                << " i=" << i
                << " x=" << data.x[i]
                << " alpha="
                << data.alpha[i]
                << " beta="
                << data.beta[i]
                << " got="
                << result
                << " expected="
                << expected
                << "\n";
        }
    }

    for (int i = 0;
         i < numKeys;
         ++i)
    {
        freeWDCFKeyPackPair(
            keys[i]
        );
    }

    return r;
}

// ============================================================================
// Output
// ============================================================================

static void printDetailed(
    const BenchResult &r)
{
    std::cout
        << "\n["
        << r.protocol
        << ", l="
        << r.bin
        << "]\n";

    std::cout
        << "  one-party key size  = "
        << r.keySizeBytes
        << " bytes\n";

    std::cout
        << "  offline             = "
        << usToMs(r.offlineUs)
        << " ms\n";

    std::cout
        << "  eval P0             = "
        << usToMs(r.evalP0Us)
        << " ms\n";

    std::cout
        << "  eval P1             = "
        << usToMs(r.evalP1Us)
        << " ms\n";

    std::cout
        << "  online              = "
        << usToMs(r.onlineUs)
        << " ms\n";

    std::cout
        << "  total               = "
        << usToMs(r.totalUs)
        << " ms\n";

    std::cout
        << "  correctness         = "
        << r.correct
        << "/"
        << r.numKeys
        << "\n";
}

static void printFinalTable(
    const std::vector<BenchResult> &results)
{
    std::cout
        << "\n"
        << "===============================================================================\n"
        << "Final DCF Table Data\n"
        << "===============================================================================\n";

    std::cout
        << std::left
        << std::setw(18) << "Protocol"
        << std::setw(8)  << "l"
        << std::setw(16) << "Key Size (B)"
        << std::setw(16) << "Offline (ms)"
        << std::setw(16) << "Online (ms)"
        << std::setw(16) << "Total (ms)"
        << "\n";

    std::cout
        << "-------------------------------------------------------------------------------\n";

    for (const auto &r : results)
    {
        std::cout
            << std::left
            << std::setw(18)
            << r.protocol

            << std::setw(8)
            << r.bin

            << std::setw(16)
            << r.keySizeBytes

            << std::setw(16)
            << std::fixed
            << std::setprecision(3)
            << usToMs(r.offlineUs)

            << std::setw(16)
            << usToMs(r.onlineUs)

            << std::setw(16)
            << usToMs(r.totalUs)

            << "\n";
    }

    std::cout
        << "===============================================================================\n";
}

static void printLatexRows(
    const std::vector<BenchResult> &results)
{
    std::cout
        << "\n"
        << "============================================================\n"
        << "LaTeX-ready Rows\n"
        << "============================================================\n";

    for (const auto &r : results)
    {
        std::cout
            << r.protocol
            << " & "
            << r.bin
            << " & "
            << r.keySizeBytes
            << " B & "
            << std::fixed
            << std::setprecision(3)
            << usToMs(r.offlineUs)
            << " & "
            << usToMs(r.onlineUs)
            << " & "
            << usToMs(r.totalUs)
            << " \\\\\n";
    }
}

// ============================================================================
// Main
// ============================================================================

int main(
    int argc,
    char **argv)
{
    int numKeys = 50000;
    int bout = 64;

    if (argc > 1)
    {
        numKeys =
            std::atoi(argv[1]);
    }

    if (argc > 2)
    {
        bout =
            std::atoi(argv[2]);
    }

    if (numKeys <= 0)
    {
        std::cerr
            << "numKeys must be positive\n";

        return 1;
    }

    if (
        bout <= 0
        ||
        bout > 64
    )
    {
        std::cerr
            << "bout must be in [1,64]\n";

        return 1;
    }

    sytorch_init();

    FSSConfig::num_threads = 1;
    FSSConfig::bitlength = 64;

    initLocalFSSPRNGs();

    const std::array<int, 3>
        bitLengths = {
            16,
            32,
            64
        };

    std::vector<BenchResult>
        results;

    results.reserve(12);

    std::cout
        << std::fixed
        << std::setprecision(3);

    std::cout
        << "============================================================\n"
        << "DCF Microbenchmark: DCF vs Half-Tree DCF vs Grotto vs WDCF\n"
        << "============================================================\n"
        << "  numKeys = "
        << numKeys
        << "\n"
        << "  bout    = "
        << bout
        << "\n"
        << "  l       = {16, 32, 64}\n"
        << "  online  = P0 + P1\n";

    // ====================================================================
    // Traditional DCF
    // ====================================================================

    for (const int bin : bitLengths)
    {
        Dataset data =
            makeDataset(
                numKeys,
                bin,
                bout
            );

        BenchResult r =
            benchmarkTraditionalDCF(
                numKeys,
                bin,
                bout,
                data
            );

        printDetailed(r);
        results.push_back(r);
    }

    // ====================================================================
    // Half-Tree DCF
    // ====================================================================

    for (const int bin : bitLengths)
    {
        Dataset data =
            makeDataset(
                numKeys,
                bin,
                bout
            );

        BenchResult r =
            benchmarkHalfTreeDCF(
                numKeys,
                bin,
                bout,
                data
            );

        printDetailed(r);
        results.push_back(r);
    }

    // ====================================================================
    // Grotto
    // ====================================================================

    for (const int bin : bitLengths)
    {
        Dataset data =
            makeDataset(
                numKeys,
                bin,
                bout
            );

        BenchResult r =
            benchmarkGrotto(
                numKeys,
                bin,
                data
            );

        printDetailed(r);
        results.push_back(r);
    }

    // ====================================================================
    // WDCF
    // ====================================================================

    for (const int bin : bitLengths)
    {
        Dataset data =
            makeDataset(
                numKeys,
                bin,
                bout
            );

        BenchResult r =
            benchmarkWDCF(
                numKeys,
                bin,
                bout,
                data
            );

        printDetailed(r);
        results.push_back(r);
    }

    printFinalTable(results);

    printLatexRows(results);

    bool allCorrect = true;

    std::cout
        << "\n"
        << "============================================================\n"
        << "Correctness Summary\n"
        << "============================================================\n";

    for (const auto &r : results)
    {
        const bool ok =
            (
                r.correct
                ==
                r.numKeys
            );

        allCorrect =
            allCorrect && ok;

        std::cout
            << "  "
            << std::setw(18)
            << std::left
            << r.protocol
            << " l="
            << std::setw(2)
            << r.bin
            << " : "
            << r.correct
            << "/"
            << r.numKeys
            << "  "
            << (
                ok
                ? "PASS"
                : "FAIL"
            )
            << "\n";
    }

    std::cout
        << "\nOverall = "
        << (
            allCorrect
            ? "PASS"
            : "FAIL"
        )
        << "\n";

    return allCorrect ? 0 : 1;
}