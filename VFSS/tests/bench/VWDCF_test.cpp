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
#include <array>
#include <random>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <cstdint>
#include <algorithm>


using namespace osuCrypto;


// ============================================================================
// Timing helpers
// ============================================================================

static inline uint64_t
nowMicroseconds()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<
            std::chrono::microseconds
        >(
            std::chrono::high_resolution_clock::now()
                .time_since_epoch()
        ).count()
    );
}


static inline uint64_t
nowNanoseconds()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<
            std::chrono::nanoseconds
        >(
            std::chrono::high_resolution_clock::now()
                .time_since_epoch()
        ).count()
    );
}


static inline double
usToMs(
    uint64_t us
)
{
    return
        static_cast<double>(us)
        /
        1000.0;
}


// ============================================================================
// Local PRNG initialization
// ============================================================================

static void
initLocalFSSPRNGs()
{
    const uint64_t seedLo =
        0x123456789abcdef0ULL;


    const uint64_t seedHi =
        0xfedcba9876543210ULL;


    for (int i = 0; i < 256; ++i)
    {
        FSSConfig::prngs[i].SetSeed(
            osuCrypto::toBlock(
                seedLo
                    ^
                    static_cast<uint64_t>(i),

                seedHi
                    +
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

static inline uint64_t
geWireBytes(
    int bw
)
{
    if (bw > 32)
    {
        return 8;
    }


    if (bw > 16)
    {
        return 4;
    }


    if (bw > 8)
    {
        return 2;
    }


    return 1;
}


// ----------------------------------------------------------------------------
// Actual implementation key size.
//
// One-party VWDCF key:
//
//   root:
//       16 bytes
//
//   scw:
//       depth * 4 * 16
//
//   tcw:
//       depth bytes
//
//   vcw:
//       depth * 4 * GE
//
//   cs:
//       depth * 3 * 16
//
//   dvec:
//       depth * 4 bytes
//
//   finalCW:
//       GE
//
//   betaShare:
//       GE
//
//   lShare:
//       GE
// ----------------------------------------------------------------------------

static uint64_t
vwdcfKeyBytesOneParty(
    int bin,
    int bout
)
{
    const uint64_t depth =
        static_cast<uint64_t>(
            (bin + 1) / 2
        );


    const uint64_t geBytes =
        geWireBytes(bout);


    return
        16ULL

        +

        depth
        *
        4ULL
        *
        16ULL

        +

        depth

        +

        depth
        *
        4ULL
        *
        geBytes

        +

        depth
        *
        3ULL
        *
        16ULL

        +

        depth
        *
        4ULL

        +

        3ULL
        *
        geBytes;
}


// ----------------------------------------------------------------------------
// Actual implementation proof size.
//
//   path:
//       depth * 3 blocks
//
//   dProof:
//       depth bytes
//
//   valueShare:
//       GE
//
//   betaShare:
//       GE
//
//   R:
//       one block
//
//   lShare:
//       GE
// ----------------------------------------------------------------------------

static uint64_t
vwdcfProofBytesOneParty(
    int bin,
    int bout
)
{
    const uint64_t depth =
        static_cast<uint64_t>(
            (bin + 1) / 2
        );


    const uint64_t geBytes =
        geWireBytes(bout);


    return
        depth
        *
        3ULL
        *
        16ULL

        +

        depth

        +

        3ULL
        *
        geBytes

        +

        16ULL;
}


// ============================================================================
// Dataset
// ============================================================================

struct Dataset
{
    std::vector<GroupElement> alpha;

    std::vector<GroupElement> beta;

    std::vector<GroupElement> x;
};


static inline GroupElement
randomValue(
    std::mt19937_64 &rng,
    int bw
)
{
    GroupElement x =
        static_cast<GroupElement>(
            rng()
        );


    mod(x, bw);


    return x;
}


static inline GroupElement
domainMax(
    int bin
)
{
    if (bin == 64)
    {
        return
            ~GroupElement(0);
    }


    return
        (
            GroupElement(1)
            <<
            bin
        )
        -
        1;
}


static Dataset
makeDataset(
    int numKeys,
    int bin,
    int bout
)
{
    Dataset data;


    data.alpha.resize(
        numKeys
    );


    data.beta.resize(
        numKeys
    );


    data.x.resize(
        numKeys
    );


    std::mt19937_64 rng(
        0x13579BDF2468ACE0ULL
        ^
        static_cast<uint64_t>(bin)
    );


    const GroupElement maxVal =
        domainMax(bin);


    for (int i = 0; i < numKeys; ++i)
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


        switch (i % 4)
        {
            // ------------------------------------------------------------
            // Equality:
            //
            //     x = alpha
            //
            // Expected:
            //
            //     0
            // ------------------------------------------------------------

            case 0:
            {
                data.x[i] =
                    data.alpha[i];

                break;
            }


            // ------------------------------------------------------------
            // Less-than:
            //
            //     x = alpha - 1
            //
            // whenever possible.
            // ------------------------------------------------------------

            case 1:
            {
                if (data.alpha[i] > 0)
                {
                    data.x[i] =
                        data.alpha[i]
                        -
                        1;
                }
                else
                {
                    data.x[i] = 0;
                }


                break;
            }


            // ------------------------------------------------------------
            // Greater-than:
            //
            //     x = alpha + 1
            //
            // whenever possible.
            // ------------------------------------------------------------

            case 2:
            {
                if (
                    data.alpha[i]
                    <
                    maxVal
                )
                {
                    data.x[i] =
                        data.alpha[i]
                        +
                        1;
                }
                else
                {
                    data.x[i] =
                        maxVal;
                }


                break;
            }


            // ------------------------------------------------------------
            // Fully random.
            // ------------------------------------------------------------

            default:
            {
                data.x[i] =
                    randomValue(
                        rng,
                        bin
                    );


                break;
            }
        }


        mod(
            data.x[i],
            bin
        );
    }


    return data;
}


// ============================================================================
// Benchmark result
// ============================================================================

struct BenchResult
{
    int bin = 0;
    int depth = 0;

    uint64_t numKeys = 0;


    uint64_t keySizeBytes = 0;

    uint64_t proofSizeBytes = 0;


    uint64_t offlineUs = 0;

    uint64_t evalP0Us = 0;

    uint64_t evalP1Us = 0;

    uint64_t verifyUs = 0;

    uint64_t onlineUs = 0;

    uint64_t totalUs = 0;


    uint64_t correct = 0;

    uint64_t honestAccepted = 0;


    uint64_t pathTamperDetected = 0;

    uint64_t pathTamperTrials = 0;


    uint64_t dTamperDetected = 0;

    uint64_t dTamperTrials = 0;


    uint64_t keyTamperDetected = 0;

    uint64_t keyTamperTrials = 0;
};


// ============================================================================
// Benchmark
// ============================================================================

static BenchResult
benchmarkVWDCF(
    int numKeys,
    int bin,
    int bout,
    const Dataset &data
)
{
    BenchResult r;


    r.bin =
        bin;


    r.depth =
        (bin + 1) / 2;


    r.numKeys =
        static_cast<uint64_t>(
            numKeys
        );


    r.keySizeBytes =
        vwdcfKeyBytesOneParty(
            bin,
            bout
        );


    r.proofSizeBytes =
        vwdcfProofBytesOneParty(
            bin,
            bout
        );


    // ========================================================================
    // Store key pairs.
    // ========================================================================

    std::vector<
        std::pair<
            VWDCFKeyPack,
            VWDCFKeyPack
        >
    > keys(numKeys);


    // ========================================================================
    // Offline / KeyGen
    // ========================================================================

    uint64_t start =
        nowMicroseconds();


    for (int i = 0; i < numKeys; ++i)
    {
        keys[i] =
            keyGenVWDCF(
                bin,
                bout,
                data.alpha[i],
                data.beta[i]
            );
    }


    r.offlineUs =
        nowMicroseconds()
        -
        start;


    // ========================================================================
    // Output-share storage.
    // ========================================================================

    std::vector<GroupElement> y0(
        numKeys
    );


    std::vector<GroupElement> y1(
        numKeys
    );


    // ========================================================================
    // Eval P0
    // ========================================================================

    start =
        nowMicroseconds();


    for (int i = 0; i < numKeys; ++i)
    {
        VWDCFEvalResult out =
            evalVWDCF(
                0,
                keys[i].first,
                data.x[i]
            );


        y0[i] =
            out.value;


        mod(
            y0[i],
            bout
        );


        freeVWDCFProof(
            out.proof
        );
    }


    r.evalP0Us =
        nowMicroseconds()
        -
        start;


    // ========================================================================
    // Eval P1
    // ========================================================================

    start =
        nowMicroseconds();


    for (int i = 0; i < numKeys; ++i)
    {
        VWDCFEvalResult out =
            evalVWDCF(
                1,
                keys[i].second,
                data.x[i]
            );


        y1[i] =
            out.value;


        mod(
            y1[i],
            bout
        );


        freeVWDCFProof(
            out.proof
        );
    }


    r.evalP1Us =
        nowMicroseconds()
        -
        start;


    // ========================================================================
    // Functional correctness.
    // ========================================================================

    for (int i = 0; i < numKeys; ++i)
    {
        GroupElement reconstructed =
            y0[i]
            +
            y1[i];


        mod(
            reconstructed,
            bout
        );


        GroupElement expected =
            (
                data.x[i]
                <
                data.alpha[i]
            )
            ?
            data.beta[i]
            :
            0;


        mod(
            expected,
            bout
        );


        if (
            reconstructed
            ==
            expected
        )
        {
            ++r.correct;
        }
        else if (
            r.correct < 10
        )
        {
            std::cerr
                << "[FUNCTION FAIL]"
                << " l=" << bin
                << " i=" << i
                << " x=" << data.x[i]
                << " alpha=" << data.alpha[i]
                << " beta=" << data.beta[i]
                << " got=" << reconstructed
                << " expected=" << expected
                << "\n";
        }
    }


    // ========================================================================
    // Honest Verify.
    //
    // Eval is repeated here, but only verifyVWDCF itself contributes to
    // verifyUs.
    // ========================================================================

    uint64_t verifyNs = 0;


    for (int i = 0; i < numKeys; ++i)
    {
        VWDCFEvalResult out0 =
            evalVWDCF(
                0,
                keys[i].first,
                data.x[i]
            );


        VWDCFEvalResult out1 =
            evalVWDCF(
                1,
                keys[i].second,
                data.x[i]
            );


        const uint64_t verifyStart =
            nowNanoseconds();


        const bool accepted =
            verifyVWDCF(
                out0.proof,
                out1.proof
            );


        verifyNs +=
            nowNanoseconds()
            -
            verifyStart;


        if (accepted)
        {
            ++r.honestAccepted;
        }
        else if (
            r.honestAccepted < 10
        )
        {
            std::cerr
                << "[VERIFY FAIL]"
                << " l=" << bin
                << " i=" << i
                << " x=" << data.x[i]
                << " alpha=" << data.alpha[i]
                << "\n";
        }


        freeVWDCFProof(
            out0.proof
        );


        freeVWDCFProof(
            out1.proof
        );
    }


    r.verifyUs =
        verifyNs
        /
        1000ULL;


    // ========================================================================
    // Tampering tests.
    // ========================================================================

    const int maxTamperTrials =
        std::min(
            numKeys,
            1000
        );


    // ------------------------------------------------------------------------
    // 1. Path-proof tampering.
    // ------------------------------------------------------------------------

    for (
        int i = 0;
        i < maxTamperTrials;
        ++i
    )
    {
        VWDCFEvalResult out0 =
            evalVWDCF(
                0,
                keys[i].first,
                data.x[i]
            );


        VWDCFEvalResult out1 =
            evalVWDCF(
                1,
                keys[i].second,
                data.x[i]
            );


        // Modify a high hash component.
        out0.proof.path[0].w[1] ^=
            OneBlock;


        const bool accepted =
            verifyVWDCF(
                out0.proof,
                out1.proof
            );


        ++r.pathTamperTrials;


        if (!accepted)
        {
            ++r.pathTamperDetected;
        }


        freeVWDCFProof(
            out0.proof
        );


        freeVWDCFProof(
            out1.proof
        );
    }


    // ------------------------------------------------------------------------
    // 2. D-vector proof tampering.
    //
    // Valid D components use only two low bits.
    // Force an invalid high bit.
    // ------------------------------------------------------------------------

    for (
        int i = 0;
        i < maxTamperTrials;
        ++i
    )
    {
        VWDCFEvalResult out0 =
            evalVWDCF(
                0,
                keys[i].first,
                data.x[i]
            );


        VWDCFEvalResult out1 =
            evalVWDCF(
                1,
                keys[i].second,
                data.x[i]
            );


        out0.proof.dProof[0] ^=
            0x80;


        const bool accepted =
            verifyVWDCF(
                out0.proof,
                out1.proof
            );


        ++r.dTamperTrials;


        if (!accepted)
        {
            ++r.dTamperDetected;
        }


        freeVWDCFProof(
            out0.proof
        );


        freeVWDCFProof(
            out1.proof
        );
    }


    // ------------------------------------------------------------------------
    // 3. betaShare key-metadata tampering.
    //
    // Use x = alpha - 1 so the true DCF output is beta.
    //
    // Modify one party's betaShare. The underlying VWDCF value shares still
    // reconstruct the original beta, while verification recovers a forged beta.
    // Verify must reject.
    // ------------------------------------------------------------------------

    for (
        int i = 0;
        i < numKeys
        &&
        r.keyTamperTrials
            <
            static_cast<uint64_t>(
                maxTamperTrials
            );
        ++i
    )
    {
        if (data.alpha[i] == 0)
        {
            continue;
        }


        const GroupElement xTamper =
            data.alpha[i]
            -
            1;


        const GroupElement oldBetaShare =
            keys[i].first.betaShare;


        keys[i].first.betaShare ^=
            1ULL;


        VWDCFEvalResult out0 =
            evalVWDCF(
                0,
                keys[i].first,
                xTamper
            );


        VWDCFEvalResult out1 =
            evalVWDCF(
                1,
                keys[i].second,
                xTamper
            );


        const bool accepted =
            verifyVWDCF(
                out0.proof,
                out1.proof
            );


        ++r.keyTamperTrials;


        if (!accepted)
        {
            ++r.keyTamperDetected;
        }


        keys[i].first.betaShare =
            oldBetaShare;


        freeVWDCFProof(
            out0.proof
        );


        freeVWDCFProof(
            out1.proof
        );
    }


    // ========================================================================
    // Latency convention.
    //
    // Same as your current single-machine microbenchmarks:
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

    r.onlineUs =
        r.evalP0Us
        +
        r.evalP1Us
        +
        r.verifyUs;


    r.totalUs =
        r.offlineUs
        +
        r.onlineUs;


    // ========================================================================
    // Free keys.
    // ========================================================================

    for (int i = 0; i < numKeys; ++i)
    {
        freeVWDCFKeyPackPair(
            keys[i]
        );
    }


    return r;
}


// ============================================================================
// Print result
// ============================================================================

static void
printResult(
    const BenchResult &r
)
{
    std::cout
        << "\n"
        << "[VWDCF-4, l="
        << r.bin
        << "]\n";


    std::cout
        << "  depth                    = "
        << r.depth
        << "\n";


    std::cout
        << "  one-party key size       = "
        << r.keySizeBytes
        << " bytes\n";


    std::cout
        << "  one-party proof size     = "
        << r.proofSizeBytes
        << " bytes\n";


    std::cout
        << "  offline                  = "
        << usToMs(r.offlineUs)
        << " ms\n";


    std::cout
        << "  eval P0                  = "
        << usToMs(r.evalP0Us)
        << " ms\n";


    std::cout
        << "  eval P1                  = "
        << usToMs(r.evalP1Us)
        << " ms\n";


    std::cout
        << "  verify                   = "
        << usToMs(r.verifyUs)
        << " ms\n";


    std::cout
        << "  online                   = "
        << usToMs(r.onlineUs)
        << " ms\n";


    std::cout
        << "  total                    = "
        << usToMs(r.totalUs)
        << " ms\n";


    std::cout
        << "  correctness              = "
        << r.correct
        << "/"
        << r.numKeys
        << "\n";


    std::cout
        << "  honest verify accepted   = "
        << r.honestAccepted
        << "/"
        << r.numKeys
        << "\n";


    std::cout
        << "  path tamper detected     = "
        << r.pathTamperDetected
        << "/"
        << r.pathTamperTrials
        << "\n";


    std::cout
        << "  D tamper detected        = "
        << r.dTamperDetected
        << "/"
        << r.dTamperTrials
        << "\n";


    std::cout
        << "  key tamper detected      = "
        << r.keyTamperDetected
        << "/"
        << r.keyTamperTrials
        << "\n";
}


// ============================================================================
// Final paper-style table
// ============================================================================

static void
printFinalTable(
    const std::vector<BenchResult> &results
)
{
    std::cout
        << "\n"
        << "========================================================================================================\n"
        << "Final VWDCF Table Data\n"
        << "========================================================================================================\n";


    std::cout
        << std::left

        << std::setw(12)
        << "Protocol"

        << std::setw(8)
        << "l"

        << std::setw(10)
        << "Depth"

        << std::setw(14)
        << "Key(B)"

        << std::setw(14)
        << "Proof(B)"

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
        << "--------------------------------------------------------------------------------------------------------\n";


    for (const auto &r : results)
    {
        std::cout
            << std::left

            << std::setw(12)
            << "VWDCF-4"

            << std::setw(8)
            << r.bin

            << std::setw(10)
            << r.depth

            << std::setw(14)
            << r.keySizeBytes

            << std::setw(14)
            << r.proofSizeBytes

            << std::setw(16)
            << std::fixed
            << std::setprecision(3)
            << usToMs(r.offlineUs)

            << std::setw(16)
            << usToMs(r.onlineUs)

            << std::setw(16)
            << usToMs(r.verifyUs)

            << std::setw(16)
            << usToMs(r.totalUs)

            << "\n";
    }


    std::cout
        << "========================================================================================================\n";
}


// ============================================================================
// LaTeX-ready rows
// ============================================================================

static void
printLatexRows(
    const std::vector<BenchResult> &results
)
{
    std::cout
        << "\n"
        << "LaTeX-ready rows:\n\n";


    for (const auto &r : results)
    {
        std::cout
            << "VWDCF"
            << " & "
            << r.bin
            << " & "
            << r.depth
            << " & "
            << r.keySizeBytes
            << " & "
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
// main
// ============================================================================

int main(
    int argc,
    char **argv
)
{
    int numKeys =
        50000;


    int bout =
        64;


    if (argc > 1)
    {
        numKeys =
            std::atoi(
                argv[1]
            );
    }


    if (argc > 2)
    {
        bout =
            std::atoi(
                argv[2]
            );
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


    std::vector<BenchResult> results;


    results.reserve(
        bitLengths.size()
    );


    std::cout
        << std::fixed
        << std::setprecision(3);


    std::cout
        << "============================================================\n"
        << "VWDCF Four-ary Microbenchmark\n"
        << "============================================================\n"
        << "  m       = 2\n"
        << "  arity   = 4\n"
        << "  numKeys = "
        << numKeys
        << "\n"
        << "  bout    = "
        << bout
        << "\n"
        << "  l       = {16, 32, 64}\n"
        << "  online  = P0 Eval + P1 Eval + Verify\n"
        << "  tamper  = up to 1000 trials per attack type\n";


    for (const int bin : bitLengths)
    {
        std::cout
            << "\n"
            << "============================================================\n"
            << "Running VWDCF-4, l="
            << bin
            << ", depth="
            << (bin + 1) / 2
            << "\n"
            << "============================================================\n";


        const Dataset data =
            makeDataset(
                numKeys,
                bin,
                bout
            );


        const BenchResult result =
            benchmarkVWDCF(
                numKeys,
                bin,
                bout,
                data
            );


        printResult(
            result
        );


        results.push_back(
            result
        );
    }


    printFinalTable(
        results
    );


    printLatexRows(
        results
    );


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


    for (const auto &r : results)
    {
        const bool functionalPass =
            (
                r.correct
                ==
                r.numKeys
            );


        const bool honestVerifyPass =
            (
                r.honestAccepted
                ==
                r.numKeys
            );


        const bool pathTamperPass =
            (
                r.pathTamperDetected
                ==
                r.pathTamperTrials
            );


        const bool dTamperPass =
            (
                r.dTamperDetected
                ==
                r.dTamperTrials
            );


        const bool keyTamperPass =
            (
                r.keyTamperDetected
                ==
                r.keyTamperTrials
            );


        const bool pass =
            functionalPass
            &&
            honestVerifyPass
            &&
            pathTamperPass
            &&
            dTamperPass
            &&
            keyTamperPass;


        allPass =
            allPass
            &&
            pass;


        std::cout
            << "  VWDCF-4 l="
            << std::setw(2)
            << r.bin

            << "  correctness="
            << r.correct
            << "/"
            << r.numKeys

            << "  honest="
            << r.honestAccepted
            << "/"
            << r.numKeys

            << "  path-tamper="
            << r.pathTamperDetected
            << "/"
            << r.pathTamperTrials

            << "  D-tamper="
            << r.dTamperDetected
            << "/"
            << r.dTamperTrials

            << "  key-tamper="
            << r.keyTamperDetected
            << "/"
            << r.keyTamperTrials

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