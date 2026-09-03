#include <FSS/dpf.h>
#include <FSS/dcf.h>
#include <FSS/freekey.h>
#include <FSS/group_element.h>
#include <FSS/config.h>
#include <FSS/prng.h>
#include <cryptoTools/Common/Defines.h>

#include <utils.h>

#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <string>

using namespace osuCrypto;

// If these are already declared in your headers, repeated declarations are fine.
GroupElement evalDPF_Payload(int party, DPFKeyPack &key, GroupElement x);
void evalDCF(int party, GroupElement *res, GroupElement idx, const DCFKeyPack &key);

static void initLocalFSSPRNGs()
{
    uint64_t seedKey = 0xdeadbeefbadc0ffeULL;

    for (int i = 0; i < 256; ++i)
    {
        FSSConfig::prngs[i].SetSeed(
            osuCrypto::toBlock(static_cast<uint64_t>(time(NULL)),
                               seedKey ^ static_cast<uint64_t>(i))
        );
    }

    prngShared.SetSeed(
        osuCrypto::toBlock(0x123456789abcdef0ULL, 0xfedcba9876543210ULL)
    );
}

static inline uint64_t nowMicroseconds()
{
    auto now = std::chrono::high_resolution_clock::now();

    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()
        ).count()
    );
}

static inline double bytesToMiB(uint64_t bytes)
{
    return static_cast<double>(bytes) / (1024.0 * 1024.0);
}

static inline double usToMs(uint64_t us)
{
    return static_cast<double>(us) / 1000.0;
}

static inline double safeRatio(uint64_t a, uint64_t b)
{
    if (b == 0) return 0.0;
    return static_cast<double>(a) / static_cast<double>(b);
}

static inline uint64_t bitsToBytesCeil(uint64_t bits)
{
    return (bits + 7) / 8;
}

// Match your current send_ge serialization:
// bw > 32 -> 8 bytes
// bw > 16 -> 4 bytes
// bw > 8  -> 2 bytes
// else    -> 1 byte
static inline uint64_t geWireBytes(int bw)
{
    if (bw > 32) return 8;
    if (bw > 16) return 4;
    if (bw > 8)  return 2;
    return 1;
}

static inline GroupElement randomValueWithBitwidth(std::mt19937_64 &rng, int bw)
{
    GroupElement x = static_cast<GroupElement>(rng());
    mod(x, bw);
    return x;
}

// ============================================================================
// Implementation key size: current C++ serialization/keypack size.
// ============================================================================

static uint64_t traditionalDPFKeyBytesOneParty_Impl(int bin, int bout)
{
    uint64_t blockBytes = 16;

    uint64_t sBytes = static_cast<uint64_t>(bin + 1) * blockBytes;
    uint64_t tcwBytes = 2 * geWireBytes(bin);
    uint64_t payloadBytes = geWireBytes(bout);

    return sBytes + tcwBytes + payloadBytes;
}

static uint64_t traditionalDCFKeyBytesOneParty_Impl(int bin, int bout)
{
    uint64_t blockBytes = 16;
    uint64_t groupSize = 1;

    uint64_t kBytes = static_cast<uint64_t>(bin + 1) * blockBytes;
    uint64_t gBytes = groupSize * geWireBytes(bout);
    uint64_t vBytes = static_cast<uint64_t>(bin) * groupSize * geWireBytes(bout);

    return kBytes + gBytes + vBytes;
}

static uint64_t halfTreeDPFKeyBytesOneParty_Impl(int bin, int bout)
{
    uint64_t blockBytes = 16;

    uint64_t rootBytes = blockBytes;
    uint64_t hashKeyBytes = blockBytes; // engineering implementation stores hashKey per key
    uint64_t cwBytes = (bin > 1) ? static_cast<uint64_t>(bin - 1) * blockBytes : 0;
    uint64_t hcwBytes = blockBytes;
    uint64_t lcwBytes = 1; // send_ge(lcw, 2)
    uint64_t outCWBytes = geWireBytes(bout);

    return rootBytes + hashKeyBytes + cwBytes + hcwBytes + lcwBytes + outCWBytes;
}

static uint64_t halfTreeDCFKeyBytesOneParty_Impl(int bin, int bout)
{
    uint64_t dpfBytes = halfTreeDPFKeyBytesOneParty_Impl(bin, bout);
    uint64_t vcwBytes = static_cast<uint64_t>(bin) * geWireBytes(bout);

    return dpfBytes + vcwBytes;
}

// ============================================================================
// Paper-style key size formula.
// These are theoretical final-key bit counts, not necessarily equal to current
// C++ serialized bytes. In particular, hashKey S is treated as setup material.
// ============================================================================

static uint64_t traditionalDPFKeyBitsOneParty_Paper(int bin, int bout)
{
    const uint64_t lambda = 128;

    return static_cast<uint64_t>(bin + 1) * lambda
         + 2ULL * static_cast<uint64_t>(bin)
         + static_cast<uint64_t>(bout);
}

static uint64_t halfTreeDPFKeyBitsOneParty_Paper(int bin, int bout)
{
    const uint64_t lambda = 128;

    // n lambda-bit CW material + final (lambda + 1) + output CW.
    return static_cast<uint64_t>(bin) * lambda
         + (lambda + 1)
         + static_cast<uint64_t>(bout);
}

static uint64_t traditionalDCFKeyBitsOneParty_Paper_CurrentCodeBaseline(int bin, int bout)
{
    const uint64_t lambda = 128;

    // Current framework DCFKeyPack baseline:
    // k[0..bin] + g[1] + v[bin].
    return static_cast<uint64_t>(bin + 1) * lambda
         + static_cast<uint64_t>(bout)
         + static_cast<uint64_t>(bin) * static_cast<uint64_t>(bout);
}

static uint64_t halfTreeDCFKeyBitsOneParty_Paper(int bin, int bout)
{
    const uint64_t lambda = 128;

    // DPF part + n VCWs.
    return static_cast<uint64_t>(bin) * lambda
         + (lambda + 1)
         + static_cast<uint64_t>(bin + 1) * static_cast<uint64_t>(bout);
}

struct BenchResult
{
    std::string name;

    uint64_t implOnePartyKeyBytes = 0;
    uint64_t implKeyPairBytes = 0;
    uint64_t implTotalKeyPairBytes = 0;

    uint64_t paperOnePartyKeyBits = 0;
    uint64_t paperOnePartyKeyBytes = 0;
    uint64_t paperKeyPairBytes = 0;
    uint64_t paperTotalKeyPairBytes = 0;

    uint64_t keygenUs = 0;
    uint64_t evalP0Us = 0;
    uint64_t evalP1Us = 0;
    uint64_t evalBothUs = 0;
    uint64_t checkUs = 0;

    uint64_t correct = 0;
    uint64_t total = 0;
};

static void fillSizeFields(BenchResult &r,
                           uint64_t implOnePartyBytes,
                           uint64_t paperOnePartyBits,
                           int numKeys)
{
    r.implOnePartyKeyBytes = implOnePartyBytes;
    r.implKeyPairBytes = 2 * r.implOnePartyKeyBytes;
    r.implTotalKeyPairBytes = r.implKeyPairBytes * static_cast<uint64_t>(numKeys);

    r.paperOnePartyKeyBits = paperOnePartyBits;
    r.paperOnePartyKeyBytes = bitsToBytesCeil(paperOnePartyBits);
    r.paperKeyPairBytes = 2 * r.paperOnePartyKeyBytes;
    r.paperTotalKeyPairBytes = r.paperKeyPairBytes * static_cast<uint64_t>(numKeys);
}

static void printBenchResult(const BenchResult &r)
{
    std::cout << "\n[" << r.name << "]\n";

    std::cout << "  impl one-party key size      = " << r.implOnePartyKeyBytes
              << " bytes (" << bytesToMiB(r.implOnePartyKeyBytes) << " MiB)\n";
    std::cout << "  impl key-pair size           = " << r.implKeyPairBytes
              << " bytes (" << bytesToMiB(r.implKeyPairBytes) << " MiB)\n";
    std::cout << "  impl total key-pair size     = " << r.implTotalKeyPairBytes
              << " bytes (" << bytesToMiB(r.implTotalKeyPairBytes) << " MiB)\n";

    std::cout << "  paper one-party key size     = " << r.paperOnePartyKeyBits
              << " bits = " << r.paperOnePartyKeyBytes
              << " bytes (" << bytesToMiB(r.paperOnePartyKeyBytes) << " MiB)\n";
    std::cout << "  paper key-pair size          = " << r.paperKeyPairBytes
              << " bytes (" << bytesToMiB(r.paperKeyPairBytes) << " MiB)\n";
    std::cout << "  paper total key-pair size    = " << r.paperTotalKeyPairBytes
              << " bytes (" << bytesToMiB(r.paperTotalKeyPairBytes) << " MiB)\n";

    std::cout << "  keygen total time            = " << usToMs(r.keygenUs) << " ms\n";
    std::cout << "  keygen avg per keypair       = "
              << static_cast<double>(r.keygenUs) / static_cast<double>(r.total)
              << " us\n";

    std::cout << "  eval P0 total time           = " << usToMs(r.evalP0Us) << " ms\n";
    std::cout << "  eval P1 total time           = " << usToMs(r.evalP1Us) << " ms\n";
    std::cout << "  eval both total time         = " << usToMs(r.evalBothUs) << " ms\n";
    std::cout << "  eval avg per party           = "
              << static_cast<double>(r.evalBothUs) / (2.0 * static_cast<double>(r.total))
              << " us\n";
    std::cout << "  eval avg per keypair         = "
              << static_cast<double>(r.evalBothUs) / static_cast<double>(r.total)
              << " us\n";

    std::cout << "  reconstruct/check time       = " << usToMs(r.checkUs) << " ms\n";
    std::cout << "  correctness                  = " << r.correct << "/" << r.total << "\n";
}

static void printComparison(const std::string &title,
                            const BenchResult &traditional,
                            const BenchResult &halfTree)
{
    std::cout << "\n============================================================\n";
    std::cout << title << " Comparison\n";
    std::cout << "============================================================\n";

    std::cout << "  impl key size ratio, Traditional / HalfTree    = "
              << safeRatio(traditional.implOnePartyKeyBytes, halfTree.implOnePartyKeyBytes)
              << "x\n";

    std::cout << "  paper key size ratio, Traditional / HalfTree   = "
              << safeRatio(traditional.paperOnePartyKeyBytes, halfTree.paperOnePartyKeyBytes)
              << "x\n";

    std::cout << "  keygen speedup, Traditional / HalfTree         = "
              << safeRatio(traditional.keygenUs, halfTree.keygenUs)
              << "x\n";

    std::cout << "  eval P0 speedup, Traditional / HalfTree        = "
              << safeRatio(traditional.evalP0Us, halfTree.evalP0Us)
              << "x\n";

    std::cout << "  eval P1 speedup, Traditional / HalfTree        = "
              << safeRatio(traditional.evalP1Us, halfTree.evalP1Us)
              << "x\n";

    std::cout << "  eval both speedup, Traditional / HalfTree      = "
              << safeRatio(traditional.evalBothUs, halfTree.evalBothUs)
              << "x\n";

    std::cout << "\n  Note: speedup/ratio > 1 means HalfTree is faster/smaller.\n";
}

static BenchResult benchmarkTraditionalDPF(int numKeys,
                                           int bin,
                                           int bout,
                                           const std::vector<GroupElement> &alpha,
                                           const std::vector<GroupElement> &beta,
                                           const std::vector<GroupElement> &x)
{
    BenchResult r;
    r.name = "Traditional DPF";
    r.total = numKeys;

    fillSizeFields(r,
                   traditionalDPFKeyBytesOneParty_Impl(bin, bout),
                   traditionalDPFKeyBitsOneParty_Paper(bin, bout),
                   numKeys);

    std::vector<std::pair<DPFKeyPack, DPFKeyPack>> keys(numKeys);

    uint64_t keygenStart = nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        keys[i] = keyGenDPF(bin, bout, alpha[i], beta[i]);
    }

    uint64_t keygenEnd = nowMicroseconds();
    r.keygenUs = keygenEnd - keygenStart;

    std::vector<GroupElement> y0(numKeys);
    std::vector<GroupElement> y1(numKeys);
    std::vector<GroupElement> y(numKeys);

    uint64_t eval0Start = nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        y0[i] = evalDPF_Payload(0, keys[i].first, x[i]);
    }

    uint64_t eval0End = nowMicroseconds();
    r.evalP0Us = eval0End - eval0Start;

    uint64_t eval1Start = nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        y1[i] = evalDPF_Payload(1, keys[i].second, x[i]);
    }

    uint64_t eval1End = nowMicroseconds();
    r.evalP1Us = eval1End - eval1Start;

    r.evalBothUs = r.evalP0Us + r.evalP1Us;

    uint64_t checkStart = nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        y[i] = y0[i] + y1[i];
        mod(y[i], bout);

        GroupElement expected = (x[i] == alpha[i]) ? beta[i] : 0;
        mod(expected, bout);

        if (y[i] == expected) {
            ++r.correct;
        } else if (r.correct + 10 >= static_cast<uint64_t>(i)) {
            std::cerr << "[Traditional DPF FAIL] i=" << i
                      << " x=" << x[i]
                      << " alpha=" << alpha[i]
                      << " beta=" << beta[i]
                      << " got=" << y[i]
                      << " expected=" << expected << "\n";
        }
    }

    uint64_t checkEnd = nowMicroseconds();
    r.checkUs = checkEnd - checkStart;

    for (int i = 0; i < numKeys; ++i)
    {
        freeDPFKeyPackPair(keys[i]);
    }

    return r;
}

static BenchResult benchmarkHalfTreeDPF(int numKeys,
                                        int bin,
                                        int bout,
                                        const std::vector<GroupElement> &alpha,
                                        const std::vector<GroupElement> &beta,
                                        const std::vector<GroupElement> &x)
{
    BenchResult r;
    r.name = "Half-Tree DPF";
    r.total = numKeys;

    fillSizeFields(r,
                   halfTreeDPFKeyBytesOneParty_Impl(bin, bout),
                   halfTreeDPFKeyBitsOneParty_Paper(bin, bout),
                   numKeys);

    std::vector<std::pair<HalfTreeDPFKeyPack, HalfTreeDPFKeyPack>> keys(numKeys);

    uint64_t keygenStart = nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        keys[i] = keyGenHalfTreeDPF(bin, bout, alpha[i], beta[i]);
    }

    uint64_t keygenEnd = nowMicroseconds();
    r.keygenUs = keygenEnd - keygenStart;

    std::vector<GroupElement> y0(numKeys);
    std::vector<GroupElement> y1(numKeys);
    std::vector<GroupElement> y(numKeys);

    uint64_t eval0Start = nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        y0[i] = evalHalfTreeDPF(0, keys[i].first, x[i]);
    }

    uint64_t eval0End = nowMicroseconds();
    r.evalP0Us = eval0End - eval0Start;

    uint64_t eval1Start = nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        y1[i] = evalHalfTreeDPF(1, keys[i].second, x[i]);
    }

    uint64_t eval1End = nowMicroseconds();
    r.evalP1Us = eval1End - eval1Start;

    r.evalBothUs = r.evalP0Us + r.evalP1Us;

    uint64_t checkStart = nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        y[i] = y0[i] + y1[i];
        mod(y[i], bout);

        GroupElement expected = (x[i] == alpha[i]) ? beta[i] : 0;
        mod(expected, bout);

        if (y[i] == expected) {
            ++r.correct;
        } else if (r.correct + 10 >= static_cast<uint64_t>(i)) {
            std::cerr << "[HalfTree DPF FAIL] i=" << i
                      << " x=" << x[i]
                      << " alpha=" << alpha[i]
                      << " beta=" << beta[i]
                      << " got=" << y[i]
                      << " expected=" << expected << "\n";
        }
    }

    uint64_t checkEnd = nowMicroseconds();
    r.checkUs = checkEnd - checkStart;

    for (int i = 0; i < numKeys; ++i)
    {
        freeHalfTreeDPFKeyPackPair(keys[i]);
    }

    return r;
}

static BenchResult benchmarkTraditionalDCF(int numKeys,
                                           int bin,
                                           int bout,
                                           const std::vector<GroupElement> &alpha,
                                           const std::vector<GroupElement> &beta,
                                           const std::vector<GroupElement> &x)
{
    BenchResult r;
    r.name = "Traditional DCF";
    r.total = numKeys;

    fillSizeFields(r,
                   traditionalDCFKeyBytesOneParty_Impl(bin, bout),
                   traditionalDCFKeyBitsOneParty_Paper_CurrentCodeBaseline(bin, bout),
                   numKeys);

    std::vector<std::pair<DCFKeyPack, DCFKeyPack>> keys(numKeys);

    uint64_t keygenStart = nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        keys[i] = keyGenDCF(bin, bout, alpha[i], beta[i]);
    }

    uint64_t keygenEnd = nowMicroseconds();
    r.keygenUs = keygenEnd - keygenStart;

    std::vector<GroupElement> y0(numKeys);
    std::vector<GroupElement> y1(numKeys);
    std::vector<GroupElement> y(numKeys);

    uint64_t eval0Start = nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        GroupElement res[1] = {0};
        evalDCF(0, res, x[i], keys[i].first);
        y0[i] = res[0];
        mod(y0[i], bout);
    }

    uint64_t eval0End = nowMicroseconds();
    r.evalP0Us = eval0End - eval0Start;

    uint64_t eval1Start = nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        GroupElement res[1] = {0};
        evalDCF(1, res, x[i], keys[i].second);
        y1[i] = res[0];
        mod(y1[i], bout);
    }

    uint64_t eval1End = nowMicroseconds();
    r.evalP1Us = eval1End - eval1Start;

    r.evalBothUs = r.evalP0Us + r.evalP1Us;

    uint64_t checkStart = nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        y[i] = y0[i] + y1[i];
        mod(y[i], bout);

        GroupElement expected = (x[i] < alpha[i]) ? beta[i] : 0;
        mod(expected, bout);

        if (y[i] == expected) {
            ++r.correct;
        } else if (r.correct + 10 >= static_cast<uint64_t>(i)) {
            std::cerr << "[Traditional DCF FAIL] i=" << i
                      << " x=" << x[i]
                      << " alpha=" << alpha[i]
                      << " beta=" << beta[i]
                      << " got=" << y[i]
                      << " expected=" << expected << "\n";
        }
    }

    uint64_t checkEnd = nowMicroseconds();
    r.checkUs = checkEnd - checkStart;

    for (int i = 0; i < numKeys; ++i)
    {
        freeDCFKeyPackPair(keys[i]);
    }

    return r;
}

static BenchResult benchmarkHalfTreeDCF(int numKeys,
                                        int bin,
                                        int bout,
                                        const std::vector<GroupElement> &alpha,
                                        const std::vector<GroupElement> &beta,
                                        const std::vector<GroupElement> &x)
{
    BenchResult r;
    r.name = "Half-Tree DCF";
    r.total = numKeys;

    fillSizeFields(r,
                   halfTreeDCFKeyBytesOneParty_Impl(bin, bout),
                   halfTreeDCFKeyBitsOneParty_Paper(bin, bout),
                   numKeys);

    std::vector<std::pair<HalfTreeDCFKeyPack, HalfTreeDCFKeyPack>> keys(numKeys);

    uint64_t keygenStart = nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        keys[i] = keyGenHalfTreeDCF(bin, bout, alpha[i], beta[i]);
    }

    uint64_t keygenEnd = nowMicroseconds();
    r.keygenUs = keygenEnd - keygenStart;

    std::vector<GroupElement> y0(numKeys);
    std::vector<GroupElement> y1(numKeys);
    std::vector<GroupElement> y(numKeys);

    uint64_t eval0Start = nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        y0[i] = evalHalfTreeDCF(0, keys[i].first, x[i]);
        mod(y0[i], bout);
    }

    uint64_t eval0End = nowMicroseconds();
    r.evalP0Us = eval0End - eval0Start;

    uint64_t eval1Start = nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        y1[i] = evalHalfTreeDCF(1, keys[i].second, x[i]);
        mod(y1[i], bout);
    }

    uint64_t eval1End = nowMicroseconds();
    r.evalP1Us = eval1End - eval1Start;

    r.evalBothUs = r.evalP0Us + r.evalP1Us;

    uint64_t checkStart = nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        y[i] = y0[i] + y1[i];
        mod(y[i], bout);

        GroupElement expected = (x[i] < alpha[i]) ? beta[i] : 0;
        mod(expected, bout);

        if (y[i] == expected) {
            ++r.correct;
        } else if (r.correct + 10 >= static_cast<uint64_t>(i)) {
            std::cerr << "[HalfTree DCF FAIL] i=" << i
                      << " x=" << x[i]
                      << " alpha=" << alpha[i]
                      << " beta=" << beta[i]
                      << " got=" << y[i]
                      << " expected=" << expected << "\n";
        }
    }

    uint64_t checkEnd = nowMicroseconds();
    r.checkUs = checkEnd - checkStart;

    for (int i = 0; i < numKeys; ++i)
    {
        freeHalfTreeDCFKeyPackPair(keys[i]);
    }

    return r;
}

int main(int argc, char **argv)
{
    int numKeys = 50000;
    int bin = 64;
    int bout = 64;

    if (argc > 1) numKeys = std::atoi(argv[1]);
    if (argc > 2) bin = std::atoi(argv[2]);
    if (argc > 3) bout = std::atoi(argv[3]);

    if (numKeys <= 0)
    {
        std::cerr << "numKeys must be positive\n";
        return 1;
    }

    if (bin <= 0 || bin > 64)
    {
        std::cerr << "bin must be in [1, 64]\n";
        return 1;
    }

    if (bout <= 0 || bout > 64)
    {
        std::cerr << "bout must be in [1, 64]\n";
        return 1;
    }

    sytorch_init();

    FSSConfig::num_threads = 1;
    FSSConfig::bitlength = 64;

    initLocalFSSPRNGs();

    std::cout << std::fixed << std::setprecision(4);

    std::cout << "============================================================\n";
    std::cout << "Traditional vs Half-Tree DPF/DCF Benchmark\n";
    std::cout << "============================================================\n";
    std::cout << "  numKeys = " << numKeys << "\n";
    std::cout << "  bin     = " << bin << "\n";
    std::cout << "  bout    = " << bout << "\n";

    std::mt19937_64 rngDPF(0x1111222233334444ULL);
    std::mt19937_64 rngDCF(0x5555666677778888ULL);

    std::vector<GroupElement> dpfAlpha(numKeys);
    std::vector<GroupElement> dpfBeta(numKeys);
    std::vector<GroupElement> dpfX(numKeys);

    for (int i = 0; i < numKeys; ++i)
    {
        dpfAlpha[i] = randomValueWithBitwidth(rngDPF, bin);
        dpfBeta[i] = randomValueWithBitwidth(rngDPF, bout);

        // Force some hits so DPF does not test almost all zeros.
        if (i % 4 == 0) {
            dpfX[i] = dpfAlpha[i];
        } else {
            dpfX[i] = randomValueWithBitwidth(rngDPF, bin);
        }
    }

    std::vector<GroupElement> dcfAlpha(numKeys);
    std::vector<GroupElement> dcfBeta(numKeys);
    std::vector<GroupElement> dcfX(numKeys);

    for (int i = 0; i < numKeys; ++i)
    {
        dcfAlpha[i] = randomValueWithBitwidth(rngDCF, bin);
        dcfBeta[i] = randomValueWithBitwidth(rngDCF, bout);

        // Half x < alpha, half random.
        if ((i % 2 == 0) && dcfAlpha[i] > 0) {
            dcfX[i] = dcfAlpha[i] - 1;
        } else {
            dcfX[i] = randomValueWithBitwidth(rngDCF, bin);
        }
    }

    std::cout << "\nRunning Traditional DPF...\n";
    BenchResult tradDPF = benchmarkTraditionalDPF(numKeys, bin, bout, dpfAlpha, dpfBeta, dpfX);
    printBenchResult(tradDPF);

    std::cout << "\nRunning Half-Tree DPF...\n";
    BenchResult halfDPF = benchmarkHalfTreeDPF(numKeys, bin, bout, dpfAlpha, dpfBeta, dpfX);
    printBenchResult(halfDPF);

    printComparison("DPF", tradDPF, halfDPF);

    std::cout << "\nRunning Traditional DCF...\n";
    BenchResult tradDCF = benchmarkTraditionalDCF(numKeys, bin, bout, dcfAlpha, dcfBeta, dcfX);
    printBenchResult(tradDCF);

    std::cout << "\nRunning Half-Tree DCF...\n";
    BenchResult halfDCF = benchmarkHalfTreeDCF(numKeys, bin, bout, dcfAlpha, dcfBeta, dcfX);
    printBenchResult(halfDCF);

    printComparison("DCF", tradDCF, halfDCF);

    bool ok = true;
    ok = ok && (tradDPF.correct == tradDPF.total);
    ok = ok && (halfDPF.correct == halfDPF.total);
    ok = ok && (tradDCF.correct == tradDCF.total);
    ok = ok && (halfDCF.correct == halfDCF.total);

    std::cout << "\n============================================================\n";
    std::cout << "Summary\n";
    std::cout << "============================================================\n";
    std::cout << "  Traditional DPF correctness = " << tradDPF.correct << "/" << tradDPF.total << "\n";
    std::cout << "  Half-Tree DPF correctness   = " << halfDPF.correct << "/" << halfDPF.total << "\n";
    std::cout << "  Traditional DCF correctness = " << tradDCF.correct << "/" << tradDCF.total << "\n";
    std::cout << "  Half-Tree DCF correctness   = " << halfDCF.correct << "/" << halfDCF.total << "\n";
    std::cout << "  Overall                     = " << (ok ? "PASS" : "FAIL") << "\n";

    return ok ? 0 : 1;
}