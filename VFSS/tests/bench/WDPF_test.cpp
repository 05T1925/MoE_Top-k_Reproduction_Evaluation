// DPF、half-tree DPF 和 WDPF的测试

#include <FSS/dpf.h>
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
#include <iomanip>
#include <string>
#include <array>
#include <algorithm>
#include <cstdint>

using namespace osuCrypto;

// ============================================================================
// 如果 dpf.h 中已经声明，则这里可以删除。
// ============================================================================
GroupElement evalDPF_Payload(
    int party,
    DPFKeyPack &key,
    GroupElement x
);

// ============================================================================
// 基本配置
// ============================================================================

// 与你之前的实验保持一致：Online = P0 eval + P1 eval。
// 如果以后希望模拟两个服务器并行执行，可改成 true，
// 此时 Online = max(P0 eval, P1 eval)。
static constexpr bool USE_PARALLEL_ONLINE_LATENCY = false;

// ============================================================================
// PRNG initialization
// ============================================================================

static void initLocalFSSPRNGs()
{
    // 使用固定种子，保证不同运行之间更容易复现。
    const uint64_t seedLo = 0x123456789abcdef0ULL;
    const uint64_t seedHi = 0xfedcba9876543210ULL;

    for (int i = 0; i < 256; ++i)
    {
        FSSConfig::prngs[i].SetSeed(
            osuCrypto::toBlock(
                seedLo ^ static_cast<uint64_t>(i),
                seedHi + static_cast<uint64_t>(i)
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
// Timing helpers
// ============================================================================

static inline uint64_t nowMicroseconds()
{
    const auto now = std::chrono::high_resolution_clock::now();

    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()
        ).count()
    );
}

static inline double usToMs(uint64_t us)
{
    return static_cast<double>(us) / 1000.0;
}

// ============================================================================
// Key-size helpers
// ============================================================================

// 与当前 send_ge 序列化方式对应：
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

// ----------------------------------------------------------------------------
// Traditional DPF
//
// Current implementation:
//
//   s[0 ... bin]       : (bin + 1) * 16 bytes
//   tcw[0], tcw[1]     : 2 * geWireBytes(bin)
//   payload             : geWireBytes(bout)
//
// ----------------------------------------------------------------------------
static uint64_t traditionalDPFKeyBytesOneParty(
    int bin,
    int bout
)
{
    const uint64_t blockBytes = 16;

    const uint64_t seedBytes =
        static_cast<uint64_t>(bin + 1) * blockBytes;

    const uint64_t tcwBytes =
        2ULL * geWireBytes(bin);

    const uint64_t payloadBytes =
        geWireBytes(bout);

    return seedBytes + tcwBytes + payloadBytes;
}

// ----------------------------------------------------------------------------
// Half-Tree DPF
//
// 按照你当前 HalfTreeDPFKeyPack 的工程实现：
//
//   root                    : 16
//   hashKey                 : 16
//   cw[0 ... bin-2]         : (bin - 1) * 16
//   hcw                     : 16
//   lcw                     : 1
//   outCW                   : geWireBytes(bout)
//
// 注意：这里把 hashKey 算进单方实际实现密钥。
// 如果以后严格按照论文，把 S 作为全局 setup material，
// 则应该去掉这 16 bytes。
// ----------------------------------------------------------------------------
static uint64_t halfTreeDPFKeyBytesOneParty(
    int bin,
    int bout
)
{
    const uint64_t blockBytes = 16;

    const uint64_t rootBytes = blockBytes;

    const uint64_t hashKeyBytes = blockBytes;

    const uint64_t cwBytes =
        (bin > 1)
        ? static_cast<uint64_t>(bin - 1) * blockBytes
        : 0;

    const uint64_t hcwBytes = blockBytes;

    const uint64_t lcwBytes = 1;

    const uint64_t outCWBytes =
        geWireBytes(bout);

    return rootBytes
         + hashKeyBytes
         + cwBytes
         + hcwBytes
         + lcwBytes
         + outCWBytes;
}

// ----------------------------------------------------------------------------
// WDPF: Four-ary / Quad-tree DPF
//
// Current implementation:
//
//   s0_initial              : 16
//   scw[depth * 4]          : depth * 4 * 16
//   tcw[depth]              : depth * 1
//   payload                 : geWireBytes(bout)
//
// depth = ceil(bin / 2)
// ----------------------------------------------------------------------------
static uint64_t wdpfKeyBytesOneParty(
    int bin,
    int bout
)
{
    const uint64_t blockBytes = 16;

    const uint64_t depth =
        static_cast<uint64_t>((bin + 1) / 2);

    const uint64_t rootBytes = blockBytes;

    const uint64_t scwBytes =
        depth * 4ULL * blockBytes;

    const uint64_t tcwBytes =
        depth;

    const uint64_t payloadBytes =
        geWireBytes(bout);

    return rootBytes
         + scwBytes
         + tcwBytes
         + payloadBytes;
}

// ============================================================================
// Random test data
// ============================================================================

static inline GroupElement randomValueWithBitwidth(
    std::mt19937_64 &rng,
    int bw
)
{
    GroupElement x =
        static_cast<GroupElement>(rng());

    mod(x, bw);

    return x;
}

struct TestDataset
{
    std::vector<GroupElement> alpha;
    std::vector<GroupElement> beta;
    std::vector<GroupElement> x;
};

static TestDataset makeDataset(
    int numKeys,
    int bin,
    int bout
)
{
    TestDataset data;

    data.alpha.resize(numKeys);
    data.beta.resize(numKeys);
    data.x.resize(numKeys);

    // 每个 bit length 使用确定性种子。
    // 三种协议调用此函数时得到完全相同的测试输入。
    std::mt19937_64 rng(
        0x13579bdf2468ace0ULL ^
        static_cast<uint64_t>(bin)
    );

    for (int i = 0; i < numKeys; ++i)
    {
        data.alpha[i] =
            randomValueWithBitwidth(rng, bin);

        data.beta[i] =
            randomValueWithBitwidth(rng, bout);

        // 强制 25% 查询命中 alpha，
        // 避免几乎所有 DPF 输出都为 0。
        if (i % 4 == 0)
        {
            data.x[i] = data.alpha[i];
        }
        else
        {
            data.x[i] =
                randomValueWithBitwidth(rng, bin);
        }
    }

    return data;
}

// ============================================================================
// Benchmark result
// ============================================================================

struct BenchResult
{
    std::string protocol;

    int bin = 0;

    uint64_t keySizeBytes = 0;

    uint64_t keygenUs = 0;

    uint64_t evalP0Us = 0;
    uint64_t evalP1Us = 0;

    uint64_t onlineUs = 0;
    uint64_t totalUs = 0;

    uint64_t correct = 0;
    uint64_t numKeys = 0;
};

// ============================================================================
// Traditional DPF benchmark
// ============================================================================

static BenchResult benchmarkTraditionalDPF(
    int numKeys,
    int bin,
    int bout,
    const TestDataset &data
)
{
    BenchResult r;

    r.protocol = "DPF";
    r.bin = bin;
    r.numKeys = numKeys;

    r.keySizeBytes =
        traditionalDPFKeyBytesOneParty(bin, bout);

    std::vector<
        std::pair<DPFKeyPack, DPFKeyPack>
    > keys(numKeys);

    // ============================================================
    // Offline: KeyGen
    // ============================================================

    const uint64_t keygenStart =
        nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        keys[i] = keyGenDPF(
            bin,
            bout,
            data.alpha[i],
            data.beta[i]
        );
    }

    const uint64_t keygenEnd =
        nowMicroseconds();

    r.keygenUs =
        keygenEnd - keygenStart;

    // ============================================================
    // Online: P0 evaluation
    // ============================================================

    std::vector<GroupElement> y0(numKeys);
    std::vector<GroupElement> y1(numKeys);

    const uint64_t evalP0Start =
        nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        y0[i] = evalDPF_Payload(
            0,
            keys[i].first,
            data.x[i]
        );
    }

    const uint64_t evalP0End =
        nowMicroseconds();

    r.evalP0Us =
        evalP0End - evalP0Start;

    // ============================================================
    // Online: P1 evaluation
    // ============================================================

    const uint64_t evalP1Start =
        nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        y1[i] = evalDPF_Payload(
            1,
            keys[i].second,
            data.x[i]
        );
    }

    const uint64_t evalP1End =
        nowMicroseconds();

    r.evalP1Us =
        evalP1End - evalP1Start;

    // ============================================================
    // Online definition
    // ============================================================

    if constexpr (USE_PARALLEL_ONLINE_LATENCY)
    {
        r.onlineUs =
            std::max(r.evalP0Us, r.evalP1Us);
    }
    else
    {
        r.onlineUs =
            r.evalP0Us + r.evalP1Us;
    }

    r.totalUs =
        r.keygenUs + r.onlineUs;

    // ============================================================
    // Correctness
    // ============================================================

    for (int i = 0; i < numKeys; ++i)
    {
        GroupElement result =
            y0[i] + y1[i];

        mod(result, bout);

        GroupElement expected =
            (data.x[i] == data.alpha[i])
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
                << "[DPF FAIL]"
                << " i=" << i
                << " l=" << bin
                << " x=" << data.x[i]
                << " alpha=" << data.alpha[i]
                << " beta=" << data.beta[i]
                << " got=" << result
                << " expected=" << expected
                << "\n";
        }
    }

    // ============================================================
    // Free
    // ============================================================

    for (int i = 0; i < numKeys; ++i)
    {
        freeDPFKeyPackPair(keys[i]);
    }

    return r;
}

// ============================================================================
// Half-Tree DPF benchmark
// ============================================================================

static BenchResult benchmarkHalfTreeDPF(
    int numKeys,
    int bin,
    int bout,
    const TestDataset &data
)
{
    BenchResult r;

    r.protocol = "Half-Tree DPF";
    r.bin = bin;
    r.numKeys = numKeys;

    r.keySizeBytes =
        halfTreeDPFKeyBytesOneParty(bin, bout);

    std::vector<
        std::pair<
            HalfTreeDPFKeyPack,
            HalfTreeDPFKeyPack
        >
    > keys(numKeys);

    // ============================================================
    // Offline: KeyGen
    // ============================================================

    const uint64_t keygenStart =
        nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        keys[i] = keyGenHalfTreeDPF(
            bin,
            bout,
            data.alpha[i],
            data.beta[i]
        );
    }

    const uint64_t keygenEnd =
        nowMicroseconds();

    r.keygenUs =
        keygenEnd - keygenStart;

    // ============================================================
    // Online: P0 evaluation
    // ============================================================

    std::vector<GroupElement> y0(numKeys);
    std::vector<GroupElement> y1(numKeys);

    const uint64_t evalP0Start =
        nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        y0[i] = evalHalfTreeDPF(
            0,
            keys[i].first,
            data.x[i]
        );
    }

    const uint64_t evalP0End =
        nowMicroseconds();

    r.evalP0Us =
        evalP0End - evalP0Start;

    // ============================================================
    // Online: P1 evaluation
    // ============================================================

    const uint64_t evalP1Start =
        nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        y1[i] = evalHalfTreeDPF(
            1,
            keys[i].second,
            data.x[i]
        );
    }

    const uint64_t evalP1End =
        nowMicroseconds();

    r.evalP1Us =
        evalP1End - evalP1Start;

    // ============================================================
    // Online definition
    // ============================================================

    if constexpr (USE_PARALLEL_ONLINE_LATENCY)
    {
        r.onlineUs =
            std::max(r.evalP0Us, r.evalP1Us);
    }
    else
    {
        r.onlineUs =
            r.evalP0Us + r.evalP1Us;
    }

    r.totalUs =
        r.keygenUs + r.onlineUs;

    // ============================================================
    // Correctness
    // ============================================================

    for (int i = 0; i < numKeys; ++i)
    {
        GroupElement result =
            y0[i] + y1[i];

        mod(result, bout);

        GroupElement expected =
            (data.x[i] == data.alpha[i])
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
                << "[Half-Tree DPF FAIL]"
                << " i=" << i
                << " l=" << bin
                << " x=" << data.x[i]
                << " alpha=" << data.alpha[i]
                << " beta=" << data.beta[i]
                << " got=" << result
                << " expected=" << expected
                << "\n";
        }
    }

    // ============================================================
    // Free
    // ============================================================

    for (int i = 0; i < numKeys; ++i)
    {
        freeHalfTreeDPFKeyPackPair(keys[i]);
    }

    return r;
}

// ============================================================================
// WDPF / Quad-tree DPF benchmark
// ============================================================================

static BenchResult benchmarkWDPF(
    int numKeys,
    int bin,
    int bout,
    const TestDataset &data
)
{
    BenchResult r;

    r.protocol = "WDPF";
    r.bin = bin;
    r.numKeys = numKeys;

    r.keySizeBytes =
        wdpfKeyBytesOneParty(bin, bout);

    std::vector<
        std::pair<
            QuadDPFKeyPack,
            QuadDPFKeyPack
        >
    > keys(numKeys);

    // ============================================================
    // Offline: KeyGen
    // ============================================================

    const uint64_t keygenStart =
        nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        keys[i] = keyGenQuadDPF(
            bin,
            bout,
            data.alpha[i],
            data.beta[i]
        );
    }

    const uint64_t keygenEnd =
        nowMicroseconds();

    r.keygenUs =
        keygenEnd - keygenStart;

    // ============================================================
    // Online: P0 evaluation
    // ============================================================

    std::vector<GroupElement> y0(numKeys);
    std::vector<GroupElement> y1(numKeys);

    const uint64_t evalP0Start =
        nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        y0[i] = evalQuadDPF(
            0,
            keys[i].first,
            data.x[i]
        );
    }

    const uint64_t evalP0End =
        nowMicroseconds();

    r.evalP0Us =
        evalP0End - evalP0Start;

    // ============================================================
    // Online: P1 evaluation
    // ============================================================

    const uint64_t evalP1Start =
        nowMicroseconds();

    for (int i = 0; i < numKeys; ++i)
    {
        y1[i] = evalQuadDPF(
            1,
            keys[i].second,
            data.x[i]
        );
    }

    const uint64_t evalP1End =
        nowMicroseconds();

    r.evalP1Us =
        evalP1End - evalP1Start;

    // ============================================================
    // Online definition
    // ============================================================

    if constexpr (USE_PARALLEL_ONLINE_LATENCY)
    {
        r.onlineUs =
            std::max(r.evalP0Us, r.evalP1Us);
    }
    else
    {
        r.onlineUs =
            r.evalP0Us + r.evalP1Us;
    }

    r.totalUs =
        r.keygenUs + r.onlineUs;

    // ============================================================
    // Correctness
    // ============================================================

    for (int i = 0; i < numKeys; ++i)
    {
        GroupElement result =
            y0[i] + y1[i];

        mod(result, bout);

        GroupElement expected =
            (data.x[i] == data.alpha[i])
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
                << "[WDPF FAIL]"
                << " i=" << i
                << " l=" << bin
                << " x=" << data.x[i]
                << " alpha=" << data.alpha[i]
                << " beta=" << data.beta[i]
                << " got=" << result
                << " expected=" << expected
                << "\n";
        }
    }

    // ============================================================
    // Free
    // ============================================================

    for (int i = 0; i < numKeys; ++i)
    {
        freeQuadDPFKeyPackPair(keys[i]);
    }

    return r;
}

// ============================================================================
// Output helpers
// ============================================================================

static void printDetailedResult(
    const BenchResult &r
)
{
    std::cout
        << "\n[" << r.protocol
        << ", l=" << r.bin << "]\n";

    std::cout
        << "  one-party key size  = "
        << r.keySizeBytes
        << " bytes\n";

    std::cout
        << "  offline             = "
        << usToMs(r.keygenUs)
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

// 输出最终论文表格需要的数据。
static void printPaperTable(
    const std::vector<BenchResult> &results
)
{
    std::cout
        << "\n"
        << "==========================================================================\n";

    std::cout
        << "Final Table Data\n";

    std::cout
        << "==========================================================================\n";

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
        << "--------------------------------------------------------------------------\n";

    for (const auto &r : results)
    {
        std::cout
            << std::left
            << std::setw(18) << r.protocol
            << std::setw(8)  << r.bin
            << std::setw(16) << r.keySizeBytes
            << std::setw(16) << std::fixed
            << std::setprecision(3)
            << usToMs(r.keygenUs)
            << std::setw(16)
            << usToMs(r.onlineUs)
            << std::setw(16)
            << usToMs(r.totalUs)
            << "\n";
    }

    std::cout
        << "==========================================================================\n";
}

// 直接输出 LaTeX 可复制的表格行。
static void printLatexRows(
    const std::vector<BenchResult> &results
)
{
    std::cout
        << "\n"
        << "============================================================\n";

    std::cout
        << "LaTeX-ready Rows\n";

    std::cout
        << "============================================================\n";

    for (const auto &r : results)
    {
        std::cout
            << r.protocol
            << " & "
            << r.bin
            << " & "
            << r.keySizeBytes
            << " B"
            << " & "
            << std::fixed
            << std::setprecision(3)
            << usToMs(r.keygenUs)
            << " & "
            << usToMs(r.onlineUs)
            << " & "
            << usToMs(r.totalUs)
            << " \\\\\n";
    }

    std::cout
        << "============================================================\n";
}

// ============================================================================
// Main
// ============================================================================

int main(
    int argc,
    char **argv
)
{
    // 默认 50,000 个 key pairs。
    int numKeys = 50000;

    // 输出范围固定为 64-bit GroupElement。
    int bout = 64;

    if (argc > 1)
    {
        numKeys = std::atoi(argv[1]);
    }

    if (argc > 2)
    {
        bout = std::atoi(argv[2]);
    }

    if (numKeys <= 0)
    {
        std::cerr
            << "numKeys must be positive.\n";

        return 1;
    }

    if (bout <= 0 || bout > 64)
    {
        std::cerr
            << "bout must be in [1, 64].\n";

        return 1;
    }

    // ============================================================
    // Framework initialization
    // ============================================================

    sytorch_init();

    FSSConfig::num_threads = 1;
    FSSConfig::bitlength = 64;

    initLocalFSSPRNGs();

    std::cout
        << std::fixed
        << std::setprecision(3);

    std::cout
        << "============================================================\n";

    std::cout
        << "DPF Microbenchmark: DPF vs Half-Tree DPF vs WDPF\n";

    std::cout
        << "============================================================\n";

    std::cout
        << "  numKeys = "
        << numKeys
        << "\n";

    std::cout
        << "  bout    = "
        << bout
        << "\n";

    std::cout
        << "  l       = {16, 32, 64}\n";

    std::cout
        << "  online  = "
        << (
            USE_PARALLEL_ONLINE_LATENCY
            ? "max(P0, P1)"
            : "P0 + P1"
        )
        << "\n";

    // ============================================================
    // Required bit lengths
    // ============================================================

    const std::array<int, 3> bitLengths = {
        16,
        32,
        64
    };

    std::vector<BenchResult> results;

    results.reserve(9);

    // ============================================================
    // DPF
    // ============================================================

    std::cout
        << "\n============================================================\n";

    std::cout
        << "Running Traditional DPF\n";

    std::cout
        << "============================================================\n";

    for (const int bin : bitLengths)
    {
        TestDataset data =
            makeDataset(numKeys, bin, bout);

        BenchResult result =
            benchmarkTraditionalDPF(
                numKeys,
                bin,
                bout,
                data
            );

        printDetailedResult(result);

        results.push_back(result);
    }

    // ============================================================
    // Half-Tree DPF
    // ============================================================

    std::cout
        << "\n============================================================\n";

    std::cout
        << "Running Half-Tree DPF\n";

    std::cout
        << "============================================================\n";

    for (const int bin : bitLengths)
    {
        TestDataset data =
            makeDataset(numKeys, bin, bout);

        BenchResult result =
            benchmarkHalfTreeDPF(
                numKeys,
                bin,
                bout,
                data
            );

        printDetailedResult(result);

        results.push_back(result);
    }

    // ============================================================
    // WDPF
    // ============================================================

    std::cout
        << "\n============================================================\n";

    std::cout
        << "Running WDPF (Four-ary DPF)\n";

    std::cout
        << "============================================================\n";

    for (const int bin : bitLengths)
    {
        TestDataset data =
            makeDataset(numKeys, bin, bout);

        BenchResult result =
            benchmarkWDPF(
                numKeys,
                bin,
                bout,
                data
            );

        printDetailedResult(result);

        results.push_back(result);
    }

    // ============================================================
    // Final outputs
    // ============================================================

    printPaperTable(results);

    printLatexRows(results);

    // ============================================================
    // Correctness summary
    // ============================================================

    bool allCorrect = true;

    std::cout
        << "\n============================================================\n";

    std::cout
        << "Correctness Summary\n";

    std::cout
        << "============================================================\n";

    for (const auto &r : results)
    {
        const bool ok =
            (r.correct == r.numKeys);

        allCorrect =
            allCorrect && ok;

        std::cout
            << "  "
            << std::setw(15)
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
            << (ok ? "PASS" : "FAIL")
            << "\n";
    }

    std::cout
        << "\n  Overall = "
        << (allCorrect ? "PASS" : "FAIL")
        << "\n";

    return allCorrect ? 0 : 1;
}