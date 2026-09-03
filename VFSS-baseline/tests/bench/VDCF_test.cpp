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
#include <string>
#include <algorithm>


using namespace osuCrypto;


// ============================================================================
// Timing
// ============================================================================

static inline uint64_t nowMicroseconds()
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


static inline uint64_t nowNanoseconds()
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


static inline double usToMs(
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

static void initLocalFSSPRNGs()
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
// Wire size helpers
// ============================================================================

static inline uint64_t geWireBytes(
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
// One-party ordinary DCF key:
//
//   k[0 ... bin] : (bin + 1) blocks
//   g[0]         : one GroupElement
//   v[0 ... bin-1]
// ----------------------------------------------------------------------------

static uint64_t ordinaryDCFKeyBytesOneParty(
    int bin,
    int bout
)
{
    return
        static_cast<uint64_t>(
            bin + 1
        )
        *
        16ULL

        +

        static_cast<uint64_t>(
            bin + 1
        )
        *
        geWireBytes(bout);
}


// ----------------------------------------------------------------------------
// One-party VerDCF key:
//
//   ordinary DCF
//
//   +
//   bin * 3 blocks of cs
//
//   +
//   betaShare
//
//   +
//   lShare
// ----------------------------------------------------------------------------

static uint64_t verDCFKeyBytesOneParty(
    int bin,
    int bout
)
{
    return
        ordinaryDCFKeyBytesOneParty(
            bin,
            bout
        )

        +

        static_cast<uint64_t>(bin)
        *
        3ULL
        *
        16ULL

        +

        2ULL
        *
        geWireBytes(bout);
}


// ----------------------------------------------------------------------------
// One-party proof:
//
//   bin * 3 blocks
//   valueShare
//   betaShare
//   randomMaskXor
//   lShare
// ----------------------------------------------------------------------------

static uint64_t verDCFProofBytesOneParty(
    int bin,
    int bout
)
{
    return
        static_cast<uint64_t>(bin)
        *
        3ULL
        *
        16ULL

        +

        3ULL
        *
        geWireBytes(bout)

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


static inline GroupElement randomValue(
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


static inline GroupElement domainMax(
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


static Dataset makeDataset(
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
            // Equality case:
            //
            // x = alpha
            //
            // Expected strict DCF output = 0.

            case 0:
            {
                data.x[i] =
                    data.alpha[i];

                break;
            }


            // Strict less-than case.

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
                    data.x[i] =
                        0;
                }

                break;
            }


            // Strict greater-than case.

            case 2:
            {
                if (data.alpha[i] < maxVal)
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


            // Fully random case.

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


    uint64_t proofTamperDetected = 0;

    uint64_t proofTamperTrials = 0;


    uint64_t keyTamperDetected = 0;

    uint64_t keyTamperTrials = 0;
};


// ============================================================================
// Benchmark VerDCF
// ============================================================================

static BenchResult benchmarkVerDCF(
    int numKeys,
    int bin,
    int bout,
    const Dataset &data
)
{
    BenchResult r;


    r.bin =
        bin;


    r.numKeys =
        static_cast<uint64_t>(
            numKeys
        );


    r.keySizeBytes =
        verDCFKeyBytesOneParty(
            bin,
            bout
        );


    r.proofSizeBytes =
        verDCFProofBytesOneParty(
            bin,
            bout
        );


    // ------------------------------------------------------------------------
    // Store all key pairs.
    // ------------------------------------------------------------------------

    std::vector<
        std::pair<
            VerDCFKeyPack,
            VerDCFKeyPack
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
            keyGenVerDCF(
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


    // ------------------------------------------------------------------------
    // Store only output shares.
    //
    // Proofs are freed immediately during isolated P0/P1 timing to avoid
    // retaining roughly O(numKeys * bin * 3 lambda) additional memory.
    // ------------------------------------------------------------------------

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
        VerDCFEvalResult out =
            evalVerDCF(
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


        freeVerDCFProof(
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
        VerDCFEvalResult out =
            evalVerDCF(
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


        freeVerDCFProof(
            out.proof
        );
    }


    r.evalP1Us =
        nowMicroseconds()
        -
        start;


    // ========================================================================
    // Functional correctness
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
        else if (r.correct < 10)
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
    // Honest verification.
    //
    // Eval is executed again here, but only verifyVerDCF() itself is included
    // in verifyUs. This keeps Eval and Verify timings separated.
    // ========================================================================

    uint64_t verifyNs = 0;


    for (int i = 0; i < numKeys; ++i)
    {
        VerDCFEvalResult out0 =
            evalVerDCF(
                0,
                keys[i].first,
                data.x[i]
            );


        VerDCFEvalResult out1 =
            evalVerDCF(
                1,
                keys[i].second,
                data.x[i]
            );


        const uint64_t verifyStart =
            nowNanoseconds();


        const bool accepted =
            verifyVerDCF(
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


        freeVerDCFProof(
            out0.proof
        );


        freeVerDCFProof(
            out1.proof
        );
    }


    r.verifyUs =
        verifyNs
        /
        1000ULL;


    // ========================================================================
    // Proof-tampering detection.
    //
    // Flip one bit in a 3lambda proof component.
    // This must be rejected.
    // ========================================================================

    const int maxTamperTrials =
        std::min(
            numKeys,
            1000
        );


    for (
        int i = 0;
        i < maxTamperTrials;
        ++i
    )
    {
        VerDCFEvalResult out0 =
            evalVerDCF(
                0,
                keys[i].first,
                data.x[i]
            );


        VerDCFEvalResult out1 =
            evalVerDCF(
                1,
                keys[i].second,
                data.x[i]
            );


        // Tamper with a high hash word so the final accumulated proof can no
        // longer decode to a zero-extended Boolean bit.

        out0.proof.path[0].w[1] ^=
            OneBlock;


        const bool accepted =
            verifyVerDCF(
                out0.proof,
                out1.proof
            );


        ++r.proofTamperTrials;


        if (!accepted)
        {
            ++r.proofTamperDetected;
        }


        freeVerDCFProof(
            out0.proof
        );


        freeVerDCFProof(
            out1.proof
        );
    }


    // ========================================================================
    // Key-tampering detection.
    //
    // For a strict less-than query x = alpha - 1, modify one party's betaShare.
    // The output shares remain generated from the DCF key, but the verification
    // relation now claims a different beta. Verify must reject.
    // ========================================================================

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


        // Save original metadata.
        const GroupElement oldBetaShare =
            keys[i].first.betaShare;


        // Malicious modification.
        keys[i].first.betaShare ^=
            1ULL;


        VerDCFEvalResult out0 =
            evalVerDCF(
                0,
                keys[i].first,
                xTamper
            );


        VerDCFEvalResult out1 =
            evalVerDCF(
                1,
                keys[i].second,
                xTamper
            );


        const bool accepted =
            verifyVerDCF(
                out0.proof,
                out1.proof
            );


        ++r.keyTamperTrials;


        if (!accepted)
        {
            ++r.keyTamperDetected;
        }


        // Restore honest key.
        keys[i].first.betaShare =
            oldBetaShare;


        freeVerDCFProof(
            out0.proof
        );


        freeVerDCFProof(
            out1.proof
        );
    }


    // ========================================================================
    // Aggregate latency.
    //
    // Same single-machine convention as your previous microbenchmarks:
    //
    //   Online =
    //       Eval(P0)
    //       +
    //       Eval(P1)
    //       +
    //       Verify
    //
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
        freeVerDCFKeyPackPair(
            keys[i]
        );
    }


    return r;
}


// ============================================================================
// Output
// ============================================================================

static void printResult(
    const BenchResult &r
)
{
    std::cout
        << "\n"
        << "[VerDCF, l="
        << r.bin
        << "]\n";


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
        << "  proof tamper detected    = "
        << r.proofTamperDetected
        << "/"
        << r.proofTamperTrials
        << "\n";


    std::cout
        << "  key tamper detected      = "
        << r.keyTamperDetected
        << "/"
        << r.keyTamperTrials
        << "\n";
}


static void printFinalTable(
    const std::vector<BenchResult> &results
)
{
    std::cout
        << "\n"
        << "===============================================================================================\n"
        << "Final VerDCF Table Data\n"
        << "===============================================================================================\n";


    std::cout
        << std::left
        << std::setw(10)
        << "Protocol"

        << std::setw(8)
        << "l"

        << std::setw(15)
        << "Key (B)"

        << std::setw(15)
        << "Proof (B)"

        << std::setw(16)
        << "Offline (ms)"

        << std::setw(16)
        << "Online (ms)"

        << std::setw(16)
        << "Verify (ms)"

        << std::setw(16)
        << "Total (ms)"

        << "\n";


    std::cout
        << "-----------------------------------------------------------------------------------------------\n";


    for (const auto &r : results)
    {
        std::cout
            << std::left

            << std::setw(10)
            << "VerDCF"

            << std::setw(8)
            << r.bin

            << std::setw(15)
            << r.keySizeBytes

            << std::setw(15)
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
        << "===============================================================================================\n";
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


    const std::array<int, 3>
        bitLengths =
        {
            16,
            32,
            64
        };


    std::vector<BenchResult>
        results;


    results.reserve(3);


    std::cout
        << std::fixed
        << std::setprecision(3);


    std::cout
        << "============================================================\n"
        << "VerDCF Microbenchmark\n"
        << "============================================================\n"
        << "  numKeys = "
        << numKeys
        << "\n"
        << "  bout    = "
        << bout
        << "\n"
        << "  l       = {16, 32, 64}\n"
        << "  online  = P0 Eval + P1 Eval + Verify\n"
        << "  tamper  = up to 1000 proof/key tampering trials\n";


    for (const int bin : bitLengths)
    {
        std::cout
            << "\n"
            << "============================================================\n"
            << "Running VerDCF, l="
            << bin
            << "\n"
            << "============================================================\n";


        Dataset data =
            makeDataset(
                numKeys,
                bin,
                bout
            );


        BenchResult r =
            benchmarkVerDCF(
                numKeys,
                bin,
                bout,
                data
            );


        printResult(r);


        results.push_back(r);
    }


    printFinalTable(
        results
    );


    bool allPass =
        true;


    std::cout
        << "\n"
        << "============================================================\n"
        << "Correctness and Verification Summary\n"
        << "============================================================\n";


    for (const auto &r : results)
    {
        const bool correct =
            (
                r.correct
                ==
                r.numKeys
            );


        const bool honestAccepted =
            (
                r.honestAccepted
                ==
                r.numKeys
            );


        const bool proofTamperPass =
            (
                r.proofTamperDetected
                ==
                r.proofTamperTrials
            );


        const bool keyTamperPass =
            (
                r.keyTamperDetected
                ==
                r.keyTamperTrials
            );


        const bool pass =
            correct
            &&
            honestAccepted
            &&
            proofTamperPass
            &&
            keyTamperPass;


        allPass =
            allPass
            &&
            pass;


        std::cout
            << "  VerDCF l="
            << std::setw(2)
            << r.bin

            << "  correctness="
            << r.correct
            << "/"
            << r.numKeys

            << "  honest-verify="
            << r.honestAccepted
            << "/"
            << r.numKeys

            << "  proof-tamper="
            << r.proofTamperDetected
            << "/"
            << r.proofTamperTrials

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