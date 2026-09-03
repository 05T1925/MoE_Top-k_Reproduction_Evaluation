#include <FSS/dcf.h>
#include <omp.h>
#include <FSS/assert.h>

#include <array>
#include <cstdint>

using namespace osuCrypto;
// uint64_t aes_evals_count = 0;

#define SERVER0 0
#define SERVER1 1
#define GROUP_LOOP(s)                  \
    int lp = (evalGroupIdxStart + groupSize) % groupSize;        \
    int ctr = 0;                       \
    while(ctr < evalGroupIdxLen)       \
    {                                  \
        s                              \
        lp = (lp + 1) % groupSize;     \
        ctr++;                         \
    }

void clearAESevals()
{
    // aes_evals_count = 0;
}

inline int bytesize(const int bitsize) {
    return (bitsize % 8) == 0 ? bitsize / 8 : (bitsize / 8)  + 1;
}

void convert(const int bitsize, const int groupSize, const block &b, uint64_t *out)
{
    static const block notThreeBlock = toBlock(~0, ~3);
    const int bys = bytesize(bitsize);
    const int totalBys = bys * groupSize;
    if (bys * groupSize <= 16) {
        uint8_t *bptr = (uint8_t *)&b;
        for(int i = 0; i < groupSize; i++) {
            out[i] = *(uint64_t *)(bptr + i * bys);
        }
    }
    else {
        int numblocks = totalBys % 16 == 0 ? totalBys / 16 : (totalBys / 16) + 1;
        AES aes(b);
        block pt[numblocks];
        block ct[numblocks];
        for(int i = 0; i < numblocks; i++) {
            pt[i] = toBlock(0, i);
        }
        aes.ecbEncBlocks(pt, numblocks, ct);
        uint8_t *bptr = (uint8_t *)ct;
        for(int i = 0; i < groupSize; i++) {
            out[i] = *(uint64_t *)(bptr + i * bys);
        }
    }
}

block traverseOneDCF(int Bin, int Bout, int groupSize, int party,
                        const block &s,
                        const block &cw,
                        const u8 &keep,
                        GroupElement *v_share,
                        GroupElement *v,
                        uint64_t level,
                        bool geq,
                        int evalGroupIdxStart,
                        int evalGroupIdxLen)

{
    static const block notThreeBlock = toBlock(~0, ~3);
    static const block TwoBlock = toBlock(0, 2);
    static const block ThreeBlock = toBlock(0, 3);
    static const block blocks[4] = {ZeroBlock, TwoBlock, OneBlock, ThreeBlock};

    block stcw;
    block ct[2]; // {tau, v_this_level}
    u8 t_previous = lsb(s);
    const auto scw = (cw & notThreeBlock);
    block ds[] = { ((cw >> 1) & OneBlock), (cw & OneBlock) };
    const auto mask = zeroAndAllOne[t_previous];
    auto ss = s & notThreeBlock;

    AES ak(ss);
    ak.ecbEncTwoBlocks(blocks + 2 * keep, ct);

    stcw = ((scw ^ ds[keep]) & mask) ^ ct[0];
    uint64_t sign = (party == SERVER1) ? -1 : 1;
    block temp = ZeroBlock;
    uint64_t v_this_level_converted[groupSize];
    convert(Bout, groupSize, ct[1], v_this_level_converted);
    GROUP_LOOP(
        v_share[lp] = v_share[lp] + sign * (v_this_level_converted[lp] + t_previous * (*(v + ((int)level) * groupSize + lp)));
    )
    return stcw;
}


block traversePathDCF(int Bin, int Bout, int groupSize, int party,
                        GroupElement idx,
                        block *k,
                        GroupElement *v_share,
                        GroupElement *v,
                        bool geq,
                        int evalGroupIdxStart,
                        int evalGroupIdxLen)
{
    block s = _mm_loadu_si128(k);
    GROUP_LOOP(v_share[lp] = 0;)

    for (int i = 0; i < Bin; ++i)
    {
        const u8 keep = static_cast<uint8_t>(idx >> (Bin - 1 - i)) & 1;
        s = traverseOneDCF(Bin, Bout, groupSize, party, s, _mm_loadu_si128(k + (i + 1)), keep, v_share, v, i, geq, evalGroupIdxStart, evalGroupIdxLen);
    }
    return s;
}


// Real Endpoints
std::pair<DCFKeyPack, DCFKeyPack> keyGenDCF(int Bin, int Bout, int groupSize,
                GroupElement idx, GroupElement* payload)
{
    // idx: bitsize Bin, payload: bitsize Bout & size groupSize
    bool greaterThan = false;

    static const block notOneBlock = toBlock(~0, ~1);
    static const block notThreeBlock = toBlock(~0, ~3);
    static const block TwoBlock = toBlock(0, 2);
    static const block ThreeBlock = toBlock(0, 3);
    const static block pt[4] = {ZeroBlock, OneBlock, TwoBlock, ThreeBlock};

    int tid = omp_get_thread_num();
    auto s = FSSConfig::prngs[tid].get<std::array<block, 2>>();
    block si[2][2];
    block vi[2][2];

    GroupElement *v_alpha = new GroupElement[groupSize];
    for (int i = 0; i < groupSize; ++i)
    {
        v_alpha[i] = 0;
    }

    block *k0 = new block[Bin + 1];
    block *k1 = new block[Bin + 1];
    GroupElement *v0 = new GroupElement[Bin * groupSize];    // bitsize Bout, size Bin x groupSize
    GroupElement *g0 = new GroupElement[groupSize];     // bitsize: Bout

    s[0] = (s[0] & notOneBlock) ^ ((s[1] & OneBlock) ^ OneBlock);
    k0[0] = s[0];
    k1[0] = s[1];
    block ct[4];

    for (int i = 0; i < Bin; ++i)
    {
        const u8 keep = static_cast<uint8_t>(idx >> (Bin - 1 - i)) & 1;
        auto a = toBlock(keep);

        auto ss0 = s[0] & notThreeBlock;
        auto ss1 = s[1] & notThreeBlock;

        AES ak0(ss0);
        AES ak1(ss1);
        ak0.ecbEncFourBlocks(pt, ct);
        si[0][0] = ct[0];
        si[0][1] = ct[1];
        vi[0][0] = ct[2];
        vi[0][1] = ct[3];
        ak1.ecbEncFourBlocks(pt, ct);
        si[1][0] = ct[0];
        si[1][1] = ct[1];
        vi[1][0] = ct[2];
        vi[1][1] = ct[3];

        auto ti0 = lsb(s[0]);
        auto ti1 = lsb(s[1]);
        GroupElement sign = (ti1 == 1) ? -1 : +1;

        uint64_t vi_01_converted[groupSize];
        uint64_t vi_11_converted[groupSize];
        uint64_t vi_10_converted[groupSize];
        uint64_t vi_00_converted[groupSize];
        convert(Bout, groupSize, vi[0][keep], vi_00_converted);
        convert(Bout, groupSize, vi[1][keep], vi_10_converted);
        convert(Bout, groupSize, vi[0][keep ^ 1], vi_01_converted);
        convert(Bout, groupSize, vi[1][keep ^ 1], vi_11_converted);

        for (int lp = 0; lp < groupSize; ++lp)
        {
            v0[i * groupSize + lp] = sign * (-v_alpha[lp] - vi_01_converted[lp] + vi_11_converted[lp]);
            if (keep == 0 && greaterThan)
            {
                // Lose is R
                v0[i * groupSize + lp] = v0[i * groupSize + lp] + sign * payload[lp];
            }
            else if (keep == 1 && !greaterThan)
            {
                // Lose is L
                v0[i * groupSize + lp] = v0[i * groupSize + lp] + sign * payload[lp];
            }
            v_alpha[lp] = v_alpha[lp] - vi_10_converted[lp] + vi_00_converted[lp] + sign * v0[i * groupSize + lp];
        }

        std::array<block, 2> siXOR{si[0][0] ^ si[1][0], si[0][1] ^ si[1][1]};

        // get the left and right t_CW bits
        std::array<block, 2> t{
            (OneBlock & siXOR[0]) ^ a ^ OneBlock,
            (OneBlock & siXOR[1]) ^ a};

        // take scw to be the bits [127, 2] as scw = s0_loss ^ s1_loss
        auto scw = siXOR[keep ^ 1] & notThreeBlock;

        k0[i + 1] = k1[i + 1] = scw           // set bits [127, 2] as scw = s0_loss ^ s1_loss
                                ^ (t[0] << 1) // set bit 1 as tL
                                ^ t[1];       // set bit 0 as tR

        auto si0Keep = si[0][keep];
        auto si1Keep = si[1][keep];

        // extract the t^Keep_CW bit
        auto TKeep = t[keep];

        // set the next level of s,t
        s[0] = si0Keep ^ (zeroAndAllOne[ti0] & (scw ^ TKeep));
        s[1] = si1Keep ^ (zeroAndAllOne[ti1] & (scw ^ TKeep));
    }

    uint64_t s0_converted[groupSize];
    uint64_t s1_converted[groupSize];
    convert(Bout, groupSize, s[0] & notThreeBlock, s0_converted);
    convert(Bout, groupSize, s[1] & notThreeBlock, s1_converted);

    for (int lp = 0; lp < groupSize; ++lp)
    {
        g0[lp] = s1_converted[lp] - s0_converted[lp] - v_alpha[lp];
        if (lsb(s[1]) == 1)
        {
            g0[lp] = g0[lp] * -1;
        }
    }

    return std::make_pair(DCFKeyPack(Bin, Bout, groupSize, k0, g0, v0), DCFKeyPack(Bin, Bout, groupSize, k1, g0, v0));
}

std::pair<DCFKeyPack, DCFKeyPack> keyGenDCF(int Bin, int Bout,
                GroupElement idx, GroupElement payload)
{
    // idx: bitsize Bin, payload: bitsize Bout
    return keyGenDCF(Bin, Bout, 1, idx, &payload);
}

void evalDCF(int Bin, int Bout, int groupSize, 
                GroupElement *out, // groupSize
                int party, GroupElement idx, 
                block *k, // bin + 1
                GroupElement *g , // groupSize
                GroupElement *v, // bin * groupSize
                bool geq /*= false*/, int evalGroupIdxStart /*= 0*/,
                int evalGroupIdxLen /*= -1*/)
{
    if (evalGroupIdxLen == 0)
    {
        return;
    }
    if (evalGroupIdxLen == -1)
    {
        evalGroupIdxLen = groupSize;
    }

    auto s = traversePathDCF(Bin, Bout, groupSize, party, idx, k, out, v, geq, evalGroupIdxStart, evalGroupIdxLen);

    u8 t = lsb(s);
    block temp = ZeroBlock;

    uint64_t s_converted[groupSize];
    static const block notThreeBlock = toBlock(~0, ~3);
    convert(Bout, groupSize, s & notThreeBlock, s_converted);
    GROUP_LOOP(
        GroupElement final_term = s_converted[lp];
        if (t)
            final_term = final_term + g[lp];
        if (party == SERVER1)
        {
            final_term = -final_term;
        } out[lp] = out[lp] + final_term;)
}

void evalDCF(int party, GroupElement *res, GroupElement idx, const DCFKeyPack &key)
{
    evalDCF(key.Bin, key.Bout, key.groupSize, res, party, idx, key.k, key.g, key.v);
}

void evalDCFPartial(int party, GroupElement *res, GroupElement idx, const DCFKeyPack &key, int start, int len)
{
    evalDCF(key.Bin, key.Bout, key.groupSize, res, party, idx, key.k, key.g, key.v, false, start, len);
}

// Dual DCF

std::pair<DualDCFKeyPack, DualDCFKeyPack> keyGenDualDCF(int Bin, int Bout, int groupSize, GroupElement idx, GroupElement *payload1, GroupElement *payload2)
{
    DualDCFKeyPack key0, key1;

    key0.Bin = Bin; key1.Bin = Bin;
    key0.Bout = Bout; key1.Bout = Bout;
    key0.groupSize = groupSize; key1.groupSize = groupSize;

    GroupElement *payload = new GroupElement[groupSize];
    for (int i = 0; i < groupSize; i++) {
        payload[i] = payload1[i] - payload2[i];
    }

    auto keys = keyGenDCF(Bin, Bout, groupSize, idx, payload);
    key0.dcfKey = keys.first, key1.dcfKey = keys.second;
    

    key0.sb = new GroupElement[groupSize];
    key1.sb = new GroupElement[groupSize];

    for (int i = 0; i < groupSize; i++) {
        auto payload2_split = splitShare(payload2[i], Bout);
        key0.sb[i] = payload2_split.first; key1.sb[i] = payload2_split.second;
    }

    return std::make_pair(key0, key1);
}

std::pair<DualDCFKeyPack, DualDCFKeyPack> keyGenDualDCF(int Bin, int Bout, GroupElement idx, GroupElement payload1, GroupElement payload2)
{
    return keyGenDualDCF(Bin, Bout, 1, idx, &payload1, &payload2);
}

void evalDualDCF(int party, GroupElement* res, GroupElement idx, const DualDCFKeyPack &key)
{
    evalDCF(key.Bin, key.Bout, key.groupSize, res, party, idx, key.dcfKey.k, key.dcfKey.g, key.dcfKey.v);
    for (int i = 0; i < key.groupSize; i++) {
        res[i] = res[i] + key.sb[i];
    }
}


// ============================================================
// IFSS_DCF implemented by two ordinary DCF keys
//
//   valKey = DCF(alpha, beta)
//   macKey = DCF(alpha, deltaA * beta)
//
// Eval returns:
//   value share = beta * {x < alpha}
//   tag share   = deltaA * beta * {x < alpha}
// ============================================================

std::pair<IFSS_DCF_TwoDCFKeyPack, IFSS_DCF_TwoDCFKeyPack>
keyGenIFSS_DCF_TwoDCF(int bin,
                      int bout,
                      GroupElement alpha,
                      GroupElement beta,
                      GroupElement deltaA)
{
    mod(alpha, bin);
    mod(beta, bout);
    mod(deltaA, bout);

    GroupElement macBeta = beta * deltaA;
    mod(macBeta, bout);

    auto valKeys = keyGenDCF(bin, bout, alpha, beta);
    auto macKeys = keyGenDCF(bin, bout, alpha, macBeta);

    IFSS_DCF_TwoDCFKeyPack k0;
    IFSS_DCF_TwoDCFKeyPack k1;

    k0.bin = k1.bin = bin;
    k0.bout = k1.bout = bout;

    k0.valKey = valKeys.first;
    k1.valKey = valKeys.second;

    k0.macKey = macKeys.first;
    k1.macKey = macKeys.second;

    return std::make_pair(k0, k1);
}

IFSSAuthShare evalIFSS_DCF_TwoDCF(int party,
                                  IFSS_DCF_TwoDCFKeyPack &key,
                                  GroupElement x)
{
    IFSSAuthShare out;

    GroupElement valOut[1] = {0};
    GroupElement macOut[1] = {0};

    evalDCF(party, valOut, x, key.valKey);
    evalDCF(party, macOut, x, key.macKey);

    out.value = valOut[0];
    out.tag = macOut[0];

    mod(out.value, key.bout);
    mod(out.tag, key.bout);

    return out;
}

// ============================================================
// IFSS_DCF implemented by one vector-payload DCF key
//
//   payload[0] = beta
//   payload[1] = deltaA * beta
//
// groupSize = 2
//
// Eval returns:
//   out[0] = value share
//   out[1] = tag share
// ============================================================

std::pair<IFSS_DCFKeyPack, IFSS_DCFKeyPack>
keyGenIFSS_DCF(int bin,
               int bout,
               GroupElement alpha,
               GroupElement beta,
               GroupElement deltaA)
{
    mod(alpha, bin);
    mod(beta, bout);
    mod(deltaA, bout);

    GroupElement macBeta = beta * deltaA;
    mod(macBeta, bout);

    GroupElement payload[2];
    payload[0] = beta;
    payload[1] = macBeta;

    auto dcfKeys = keyGenDCF(bin, bout, 2, alpha, payload);

    IFSS_DCFKeyPack k0;
    IFSS_DCFKeyPack k1;

    k0.bin = k1.bin = bin;
    k0.bout = k1.bout = bout;

    k0.dcfKey = dcfKeys.first;
    k1.dcfKey = dcfKeys.second;

    return std::make_pair(k0, k1);
}

IFSSAuthShare evalIFSS_DCF(int party,
                           IFSS_DCFKeyPack &key,
                           GroupElement x)
{
    IFSSAuthShare out;

    GroupElement dcfOut[2] = {0, 0};

    evalDCF(party, dcfOut, x, key.dcfKey);

    out.value = dcfOut[0];
    out.tag = dcfOut[1];

    mod(out.value, key.bout);
    mod(out.tag, key.bout);

    return out;
}



// ============================================================
// DIF from DCF implementation
// ============================================================

static inline GroupElement dif_domain_max(int bin)
{
    always_assert(bin > 0);
    always_assert(bin <= 64);

    if (bin == 64)
    {
        return ~GroupElement(0);
    }

    return (GroupElement(1) << bin) - 1;
}

static inline std::pair<GroupElement, GroupElement>
dif_split_share(GroupElement x, int bout)
{
    GroupElement s0 = random_ge(bout);
    GroupElement s1 = x - s0;

    mod(s0, bout);
    mod(s1, bout);

    return std::make_pair(s0, s1);
}

std::pair<DIFKeyPack, DIFKeyPack>
keyGenDIF(int bin,
          int bout,
          GroupElement a,
          GroupElement b,
          GroupElement beta)
{
    always_assert(bin > 0);
    always_assert(bin <= 64);
    always_assert(bout > 0);
    always_assert(bout <= 64);

    mod(a, bin);
    mod(b, bin);
    mod(beta, bout);

    // This implementation supports the ordinary ordered interval [a,b].
    // For cyclic intervals with a > b, split them at the boundary or call two DIFs.
    always_assert(a <= b);

    const GroupElement maxVal = dif_domain_max(bin);

    DIFKeyPack k0;
    DIFKeyPack k1;

    k0.bin = k1.bin = bin;
    k0.bout = k1.bout = bout;

    k0.hasUpperDcf = k1.hasUpperDcf = false;
    k0.hasLowerDcf = k1.hasLowerDcf = false;

    k0.constShare = 0;
    k1.constShare = 0;

    // ------------------------------------------------------------
    // Upper term:
    //   beta * {x <= b} = beta * {x < b+1}
    //
    // If b == maxVal, then {x <= b} is always 1.
    // So we use an additive sharing of the constant beta.
    // ------------------------------------------------------------

    if (b == maxVal)
    {
        auto c = dif_split_share(beta, bout);
        k0.constShare = c.first;
        k1.constShare = c.second;
    }
    else
    {
        GroupElement upperAlpha = b + 1;
        mod(upperAlpha, bin);

        auto upperKeys = keyGenDCF(bin, bout, upperAlpha, beta);

        k0.upperKey = upperKeys.first;
        k1.upperKey = upperKeys.second;

        k0.hasUpperDcf = true;
        k1.hasUpperDcf = true;
    }

    // ------------------------------------------------------------
    // Lower term:
    //   - beta * {x < a}
    //
    // If a == 0, then {x < a} is always 0.
    // ------------------------------------------------------------

    if (a != 0)
    {
        GroupElement negBeta = -beta;
        mod(negBeta, bout);

        auto lowerKeys = keyGenDCF(bin, bout, a, negBeta);

        k0.lowerKey = lowerKeys.first;
        k1.lowerKey = lowerKeys.second;

        k0.hasLowerDcf = true;
        k1.hasLowerDcf = true;
    }

    return std::make_pair(k0, k1);
}

GroupElement evalDIF(int party,
                     DIFKeyPack &key,
                     GroupElement x)
{
    always_assert(key.bin > 0);
    always_assert(key.bout > 0);

    mod(x, key.bin);

    GroupElement out = key.constShare;
    mod(out, key.bout);

    if (key.hasUpperDcf)
    {
        GroupElement tmp[1] = {0};

        evalDCF(party, tmp, x, key.upperKey);

        out += tmp[0];
        mod(out, key.bout);
    }

    if (key.hasLowerDcf)
    {
        GroupElement tmp[1] = {0};

        evalDCF(party, tmp, x, key.lowerKey);

        out += tmp[0];
        mod(out, key.bout);
    }

    mod(out, key.bout);

    return out;
}


// ============================================================================
// Half-Tree DCF implementation
// Put this block in FSS/dcf.cpp
// ============================================================================

#include <FSS/dcf.h>
#include <FSS/dpf.h>
#include <FSS/assert.h>
#include <FSS/config.h>
#include <omp.h>
#include <vector>

using namespace osuCrypto;

namespace {

inline uint8_t htdcf_lsb(const block &b)
{
    return static_cast<uint8_t>(_mm_cvtsi128_si64x(b) & 1);
}

inline block htdcf_two_block()
{
    return toBlock(0, 2);
}

inline block htdcf_clear_lsb(const block &b)
{
    static const block notOneBlock = toBlock(~0ULL, ~1ULL);
    return b & notOneBlock;
}

// No AES-object reuse version.
// Each hash call constructs AES(hashKey) locally.
inline block htdcf_hash(const block &hashKey, const block &x)
{
    AES aes(hashKey);
    block y = aes.ecbEncBlock(x);
    return y ^ x;
}

inline uint8_t htdcf_get_bit(GroupElement x, int bin, int level)
{
    return static_cast<uint8_t>((x >> (bin - 1 - level)) & 1ULL);
}

inline GroupElement htdcf_convert(const block &s, int bout)
{
    GroupElement y = static_cast<GroupElement>(_mm_extract_epi64(htdcf_clear_lsb(s), 0));
    mod(y, bout);
    return y;
}

inline GroupElement htdcf_neg(GroupElement x, int bout)
{
    GroupElement y = 0 - x;
    mod(y, bout);
    return y;
}

inline GroupElement htdcf_add(GroupElement a, GroupElement b, int bout)
{
    GroupElement y = a + b;
    mod(y, bout);
    return y;
}

inline GroupElement htdcf_sub(GroupElement a, GroupElement b, int bout)
{
    GroupElement y = a - b;
    mod(y, bout);
    return y;
}

inline GroupElement htdcf_mul_sign(int sign, GroupElement x, int bout)
{
    GroupElement y;

    if (sign == 1) {
        y = x;
    } else if (sign == -1) {
        y = 0 - x;
    } else {
        y = 0;
    }

    mod(y, bout);
    return y;
}

inline GroupElement htdcf_bitdiff_mul(int bit_i, int bit_prev, GroupElement beta, int bout)
{
    int d = bit_i - bit_prev;
    return htdcf_mul_sign(d, beta, bout);
}

} // namespace

std::pair<HalfTreeDCFKeyPack, HalfTreeDCFKeyPack>
keyGenHalfTreeDCF(int bin, int bout, GroupElement alpha, GroupElement beta)
{
    always_assert(bin >= 1 && bin <= 64);
    always_assert(bout >= 1 && bout <= 64);

    mod(alpha, bin);
    mod(beta, bout);

    uint8_t alphaLast = htdcf_get_bit(alpha, bin, bin - 1);

    // DCF decomposition:
    // f^<_{alpha,beta}(x)
    // = prefix_term(x) + f^bullet_{alpha, -alpha_n * beta}(x).
    GroupElement pointPayload = alphaLast ? htdcf_neg(beta, bout) : 0;

    std::vector<block> path0;
    std::vector<block> path1;

    auto dpfKeys = keyGenHalfTreeDPFWithPath(bin,
                                             bout,
                                             alpha,
                                             pointPayload,
                                             &path0,
                                             &path1);

    HalfTreeDCFKeyPack k0;
    HalfTreeDCFKeyPack k1;

    k0.bin = k1.bin = bin;
    k0.bout = k1.bout = bout;

    k0.dpfKey = dpfKeys.first;
    k1.dpfKey = dpfKeys.second;

    k0.vcw = new GroupElement[bin];
    k1.vcw = new GroupElement[bin];

    int alphaPrev = 0;

    for (int level = 0; level < bin; ++level)
    {
        int alphaBit = static_cast<int>(htdcf_get_bit(alpha, bin, level));

        block p0 = path0[level];
        block p1 = path1[level];

        // One additional child v_i is enough in Half-Tree DCF.
        block v0 = htdcf_hash(k0.dpfKey.hashKey, p0 ^ htdcf_two_block());
        block v1 = htdcf_hash(k0.dpfKey.hashKey, p1 ^ htdcf_two_block());

        GroupElement conv0 = htdcf_convert(v0, bout);
        GroupElement conv1 = htdcf_convert(v1, bout);

        GroupElement deltaAlphaBeta = htdcf_bitdiff_mul(alphaBit, alphaPrev, beta, bout);
        GroupElement inner = htdcf_add(htdcf_sub(conv1, conv0, bout), deltaAlphaBeta, bout);

        int sign = static_cast<int>(htdcf_lsb(p0)) - static_cast<int>(htdcf_lsb(p1));

        GroupElement vcw = htdcf_mul_sign(sign, inner, bout);

        k0.vcw[level] = vcw;
        k1.vcw[level] = vcw;

        alphaPrev = alphaBit;
    }

    return std::make_pair(k0, k1);
}

GroupElement evalHalfTreeDCF(int party,
                             const HalfTreeDCFKeyPack &key,
                             GroupElement x)
{
    always_assert(party == 0 || party == 1);
    always_assert(key.bin >= 1 && key.bin <= 64);

    mod(x, key.bin);

    HalfTreeDPFEvalTrace trace;

    // Point-function term.
    GroupElement y = evalHalfTreeDPF(party, key.dpfKey, x, &trace);

    // Prefix-comparison term.
    GroupElement V = 0;

    for (int level = 0; level < key.bin; ++level)
    {
        block parent = trace.parent[level];

        block v = htdcf_hash(key.dpfKey.hashKey, parent ^ htdcf_two_block());

        GroupElement term = htdcf_convert(v, key.bout);

        if (htdcf_lsb(parent)) {
            term = htdcf_add(term, key.vcw[level], key.bout);
        }

        if (party == 1) {
            term = htdcf_neg(term, key.bout);
        }

        V = htdcf_add(V, term, key.bout);
    }

    y = htdcf_add(y, V, key.bout);
    mod(y, key.bout);

    return y;
}

void evalAllHalfTreeDCF(int party,
                        const HalfTreeDCFKeyPack &key,
                        GroupElement *out)
{
    always_assert(key.bin >= 1 && key.bin <= 30);

    const uint64_t N = 1ULL << key.bin;

    for (uint64_t x = 0; x < N; ++x) {
        out[x] = evalHalfTreeDCF(party, key, static_cast<GroupElement>(x));
    }
}

GroupElement reconstructHalfTreeDCF(const HalfTreeDCFKeyPack &k0,
                                    const HalfTreeDCFKeyPack &k1,
                                    GroupElement x)
{
    GroupElement y0 = evalHalfTreeDCF(0, k0, x);
    GroupElement y1 = evalHalfTreeDCF(1, k1, x);

    GroupElement y = y0 + y1;
    mod(y, k0.bout);

    return y;
}




// ============================================================================
// Four-ary Wide Distributed Comparison Function (WDCF), m = 2
//
// Implements the user-provided WDCF_m pseudocode for:
//
//     m = 2
//     B = 2^m = 4
//
// Functionality:
//
//     f^<_{alpha,beta}(x)
//       = beta, if x < alpha
//       = 0,    otherwise
//
// This implementation follows the current framework's seed-keyed AES style.
// It does NOT introduce the Half-Tree fixed-key AES hash optimization.
//
// For each branch c:
//
//     G_c(s) -> (s^c, v^c, t^c)
//
// is instantiated using two domain-separated AES blocks:
//
//     st = AES_s(c)
//     v  = AES_s(4 + c)
//
// where:
//     s^c = clear_lsb(st)
//     t^c = lsb(st)
//
// ============================================================================

static inline uint8_t wdcf_lsb(const osuCrypto::block &b)
{
    return static_cast<uint8_t>(
        _mm_cvtsi128_si64x(b) & 1
    );
}

// ----------------------------------------------------------------------------
// Apply (-1)^t in Z_{2^bout}.
// ----------------------------------------------------------------------------
static inline GroupElement wdcf_apply_sign(GroupElement x,
                                            bool negative,
                                            int bout)
{
    if (negative)
    {
        x = -x;
    }

    mod(x, bout);
    return x;
}

// ----------------------------------------------------------------------------
// Convert one lambda-bit block to one group element.
//
// Reuse the existing convert(...) implementation already present in dcf.cpp.
// ----------------------------------------------------------------------------
static inline GroupElement wdcf_convert_block(int bout,
                                               const osuCrypto::block &x)
{
    uint64_t tmp[1] = {0};

    convert(
        bout,
        1,
        x,
        tmp
    );

    GroupElement out = tmp[0];
    mod(out, bout);

    return out;
}

// ----------------------------------------------------------------------------
// Extract one two-bit chunk.
//
// For even bin:
//     x = x_1 || x_2 || ... || x_depth
//
// For odd bin:
//     x || 0
//
// exactly following the pseudocode padding rule.
// ----------------------------------------------------------------------------
static inline uint8_t wdcf_extract_chunk(GroupElement x,
                                          int bin,
                                          int level)
{
    const int shift =
        bin - 2 - 2 * level;

    if (shift >= 0)
    {
        return static_cast<uint8_t>(
            (x >> shift) & 3ULL
        );
    }

    // Odd input length:
    // append one zero bit to the right.
    return static_cast<uint8_t>(
        (x << 1) & 3ULL
    );
}

// ----------------------------------------------------------------------------
// Expand all four branches:
//
//     G_0(s), G_1(s), G_2(s), G_3(s)
//
// Each G_c produces:
//
//     (s^c, v^c, t^c)
//
// AES plaintext domain:
//
//     0,1,2,3 -> seed/control outputs
//     4,5,6,7 -> value outputs
// ----------------------------------------------------------------------------
static inline void wdcf_expand_all(
    const osuCrypto::block &seed,
    osuCrypto::block childS[4],
    osuCrypto::block childV[4],
    uint8_t childT[4])
{
    static const osuCrypto::block notOneBlock =
        osuCrypto::toBlock(~0ULL, ~1ULL);

    static const osuCrypto::block pt[8] = {
        osuCrypto::toBlock(0, 0),
        osuCrypto::toBlock(0, 1),
        osuCrypto::toBlock(0, 2),
        osuCrypto::toBlock(0, 3),

        osuCrypto::toBlock(0, 4),
        osuCrypto::toBlock(0, 5),
        osuCrypto::toBlock(0, 6),
        osuCrypto::toBlock(0, 7)
    };

    osuCrypto::block ct[8];

    // Same seed-keyed AES style as the existing traditional DCF.
    osuCrypto::AES aes(seed);

    aes.ecbEncBlocks(
        pt,
        8,
        ct
    );

    for (int c = 0; c < 4; ++c)
    {
        childS[c] = ct[c] & notOneBlock;
        childT[c] = wdcf_lsb(ct[c]);

        childV[c] = ct[4 + c];
    }
}

// ----------------------------------------------------------------------------
// Expand only the selected branch during evaluation.
//
// Evaluation needs:
//     AES_s(c)
//     AES_s(4+c)
//
// hence exactly two AES block encryptions per WDCF level.
// ----------------------------------------------------------------------------
static inline void wdcf_expand_selected(
    const osuCrypto::block &seed,
    uint8_t c,
    osuCrypto::block &childS,
    osuCrypto::block &childV,
    uint8_t &childT)
{
    static const osuCrypto::block notOneBlock =
        osuCrypto::toBlock(~0ULL, ~1ULL);

    osuCrypto::block pt[2] = {
        osuCrypto::toBlock(0, c),
        osuCrypto::toBlock(0, 4 + c)
    };

    osuCrypto::block ct[2];

    osuCrypto::AES aes(seed);

    aes.ecbEncTwoBlocks(
        pt,
        ct
    );

    childS = ct[0] & notOneBlock;
    childT = wdcf_lsb(ct[0]);

    childV = ct[1];
}

// ============================================================================
// WDCF.Gen
// ============================================================================

std::pair<WDCFKeyPack, WDCFKeyPack>
keyGenWDCF(int bin,
           int bout,
           GroupElement alpha,
           GroupElement beta)
{
    always_assert(bin > 0);
    always_assert(bin <= 64);

    always_assert(bout > 0);
    always_assert(bout <= 64);

    mod(alpha, bin);
    mod(beta, bout);

    static const osuCrypto::block notOneBlock =
        osuCrypto::toBlock(~0ULL, ~1ULL);

    WDCFKeyPack key0(bin, bout);
    WDCFKeyPack key1(bin, bout);

    const int tid =
        omp_get_thread_num();

    // --------------------------------------------------------------------
    // Sample s_0^(0), s_1^(0).
    // --------------------------------------------------------------------

    auto roots =
        FSSConfig::prngs[tid]
            .get<std::array<osuCrypto::block, 2>>();

    osuCrypto::block s0 =
        roots[0] & notOneBlock;

    osuCrypto::block s1 =
        roots[1] & notOneBlock;

    key0.seed = s0;
    key1.seed = s1;

    // --------------------------------------------------------------------
    // t_0^(0) = 0
    // t_1^(0) = 1
    // --------------------------------------------------------------------

    uint8_t t0 = 0;
    uint8_t t1 = 1;

    // Accumulated alpha-path value.
    GroupElement V = 0;

    // ====================================================================
    // Process ceil(bin / 2) levels.
    // ====================================================================

    for (int level = 0;
         level < key0.depth;
         ++level)
    {
        const uint8_t keep =
            wdcf_extract_chunk(
                alpha,
                bin,
                level
            );

        // ================================================================
        // Expand all four children for both parties.
        // ================================================================

        osuCrypto::block sChild0[4];
        osuCrypto::block vChild0[4];
        uint8_t tChild0[4];

        osuCrypto::block sChild1[4];
        osuCrypto::block vChild1[4];
        uint8_t tChild1[4];

        wdcf_expand_all(
            s0,
            sChild0,
            vChild0,
            tChild0
        );

        wdcf_expand_all(
            s1,
            sChild1,
            vChild1,
            tChild1
        );

        // ================================================================
        // Sample r_i != Keep_i.
        //
        // Random integer from {0,1,2}, then skip Keep.
        // ================================================================

        uint8_t r =
            static_cast<uint8_t>(
                FSSConfig::prngs[tid]
                    .get<uint64_t>() % 3ULL
            );

        if (r >= keep)
        {
            ++r;
        }

        always_assert(r < 4);
        always_assert(r != keep);

        // ================================================================
        // Convert all v children.
        // ================================================================

        GroupElement convertedV0[4];
        GroupElement convertedV1[4];

        for (int c = 0; c < 4; ++c)
        {
            convertedV0[c] =
                wdcf_convert_block(
                    bout,
                    vChild0[c]
                );

            convertedV1[c] =
                wdcf_convert_block(
                    bout,
                    vChild1[c]
                );
        }

        // (-1)^{t_1^(i-1)}
        const bool negative =
            (t1 != 0);

        uint8_t packedTCW = 0;

        // ================================================================
        // Generate four branch-specific CWs.
        // ================================================================

        for (int c = 0; c < 4; ++c)
        {
            const int pos =
                level * 4 + c;

            osuCrypto::block scw;
            uint8_t tcw;
            GroupElement vcw;

            if (c != keep)
            {
                // --------------------------------------------------------
                // s_CW^c = s_0^c xor s_1^c
                // --------------------------------------------------------

                scw =
                    sChild0[c] ^
                    sChild1[c];

                scw &= notOneBlock;

                // --------------------------------------------------------
                // t_CW^c = t_0^c xor t_1^c
                // --------------------------------------------------------

                tcw =
                    tChild0[c] ^
                    tChild1[c];

                // --------------------------------------------------------
                // V_CW^c
                //   = (-1)^t1 *
                //     [Convert(v_1^c) - Convert(v_0^c) - V]
                // --------------------------------------------------------

                GroupElement inner =
                    convertedV1[c]
                    - convertedV0[c]
                    - V;

                mod(inner, bout);

                vcw =
                    wdcf_apply_sign(
                        inner,
                        negative,
                        bout
                    );

                // --------------------------------------------------------
                // If c < alpha_i:
                //
                //   V_CW^c += (-1)^t1 * beta
                // --------------------------------------------------------

                if (c < keep)
                {
                    GroupElement signedBeta =
                        wdcf_apply_sign(
                            beta,
                            negative,
                            bout
                        );

                    vcw += signedBeta;
                    mod(vcw, bout);
                }
            }
            else
            {
                // --------------------------------------------------------
                // Keep branch:
                //
                // s_CW^Keep = s_0^r xor s_1^r
                // --------------------------------------------------------

                scw =
                    sChild0[r] ^
                    sChild1[r];

                scw &= notOneBlock;

                // --------------------------------------------------------
                // t_CW^Keep
                //   = t_0^Keep xor t_1^Keep xor 1
                // --------------------------------------------------------

                tcw =
                    tChild0[c]
                    ^ tChild1[c]
                    ^ 1;

                // --------------------------------------------------------
                // V_CW^Keep
                //   = (-1)^t1 *
                //     [Convert(v_1^r) - Convert(v_0^r) - V]
                // --------------------------------------------------------

                GroupElement inner =
                    convertedV1[r]
                    - convertedV0[r]
                    - V;

                mod(inner, bout);

                vcw =
                    wdcf_apply_sign(
                        inner,
                        negative,
                        bout
                    );
            }

            key0.scw[pos] = scw;
            key1.scw[pos] = scw;

            key0.vcw[pos] = vcw;
            key1.vcw[pos] = vcw;

            packedTCW |=
                static_cast<uint8_t>(
                    (tcw & 1) << c
                );
        }

        key0.tcw[level] = packedTCW;
        key1.tcw[level] = packedTCW;

        // ================================================================
        // Update V on alpha path:
        //
        // V <- V
        //      - Convert(v_1^Keep)
        //      + Convert(v_0^Keep)
        //      + (-1)^t1 * V_CW^Keep
        // ================================================================

        const GroupElement signedKeepCW =
            wdcf_apply_sign(
                key0.vcw[level * 4 + keep],
                negative,
                bout
            );

        V =
            V
            - convertedV1[keep]
            + convertedV0[keep]
            + signedKeepCW;

        mod(V, bout);

        // ================================================================
        // Advance alpha path seeds and control bits.
        // ================================================================

        const uint8_t t0Previous = t0;
        const uint8_t t1Previous = t1;

        const osuCrypto::block keepSCW =
            key0.scw[level * 4 + keep];

        const uint8_t keepTCW =
            static_cast<uint8_t>(
                (packedTCW >> keep) & 1
            );

        s0 =
            sChild0[keep]
            ^
            (
                t0Previous
                ? keepSCW
                : osuCrypto::ZeroBlock
            );

        t0 =
            tChild0[keep]
            ^
            (
                t0Previous
                ? keepTCW
                : 0
            );

        s1 =
            sChild1[keep]
            ^
            (
                t1Previous
                ? keepSCW
                : osuCrypto::ZeroBlock
            );

        t1 =
            tChild1[keep]
            ^
            (
                t1Previous
                ? keepTCW
                : 0
            );
    }

    // ====================================================================
    // Final correction word:
    //
    // CW^(depth+1)
    //   = (-1)^t1 *
    //     [Convert(s_1^depth) - Convert(s_0^depth) - V]
    // ====================================================================

    GroupElement convertedS0 =
        wdcf_convert_block(
            bout,
            s0
        );

    GroupElement convertedS1 =
        wdcf_convert_block(
            bout,
            s1
        );

    GroupElement finalInner =
        convertedS1
        - convertedS0
        - V;

    mod(finalInner, bout);

    GroupElement finalCW =
        wdcf_apply_sign(
            finalInner,
            t1 != 0,
            bout
        );

    key0.finalCW = finalCW;
    key1.finalCW = finalCW;

    return std::make_pair(
        key0,
        key1
    );
}

// ============================================================================
// WDCF.Eval
// ============================================================================

GroupElement
evalWDCF(int party,
         const WDCFKeyPack &key,
         GroupElement x)
{
    always_assert(
        party == 0 ||
        party == 1
    );

    mod(x, key.bin);

    osuCrypto::block s =
        key.seed;

    uint8_t t =
        static_cast<uint8_t>(party);

    GroupElement VShare = 0;

    // ====================================================================
    // Process one selected branch per level.
    // ====================================================================

    for (int level = 0;
         level < key.depth;
         ++level)
    {
        const uint8_t c =
            wdcf_extract_chunk(
                x,
                key.bin,
                level
            );

        osuCrypto::block childS;
        osuCrypto::block childV;
        uint8_t childT;

        wdcf_expand_selected(
            s,
            c,
            childS,
            childV,
            childT
        );

        // The VCW term uses t^(i-1), not the updated t^i.
        const uint8_t tPrevious = t;

        // ================================================================
        // V_b <- V_b
        //        + (-1)^b *
        //          [Convert(v_b^c)
        //           + t_previous * V_CW^c]
        // ================================================================

        GroupElement term =
            wdcf_convert_block(
                key.bout,
                childV
            );

        if (tPrevious)
        {
            term +=
                key.vcw[
                    level * 4 + c
                ];

            mod(term, key.bout);
        }

        if (party == 1)
        {
            term = -term;
            mod(term, key.bout);
        }

        VShare += term;
        mod(VShare, key.bout);

        // ================================================================
        // Correct selected seed/control state.
        // ================================================================

        if (tPrevious)
        {
            childS ^=
                key.scw[
                    level * 4 + c
                ];

            childT ^=
                static_cast<uint8_t>(
                    (key.tcw[level] >> c)
                    & 1
                );
        }

        s = childS;
        t = childT;
    }

    // ====================================================================
    // Final term:
    //
    // (-1)^b *
    // [Convert(s^depth) + t^depth * finalCW]
    // ====================================================================

    GroupElement finalTerm =
        wdcf_convert_block(
            key.bout,
            s
        );

    if (t)
    {
        finalTerm += key.finalCW;
        mod(finalTerm, key.bout);
    }

    if (party == 1)
    {
        finalTerm = -finalTerm;
        mod(finalTerm, key.bout);
    }

    VShare += finalTerm;
    mod(VShare, key.bout);

    return VShare;
}




// ============================================================================
// Verifiable Distributed Comparison Function
//
// Implements Algorithm 4:
//
//   VerDCF.Gen
//   VerDCF.Eval
//   VerDCF.Verify
//
// Functionality:
//
//   f_{alpha,beta}(x)
//       = beta, if x < alpha
//       = 0,    otherwise
//
// Current framework:
//   lambda = 128
//   GroupElement = uint64_t
//   output group = Z_{2^bout}
//
// Hash:
//   H : {0,1}^128 -> {0,1}^384
//
// instantiated by three domain-separated AES-MMO blocks.
// ============================================================================


// ----------------------------------------------------------------------------
// Equality of two 128-bit blocks.
// ----------------------------------------------------------------------------

static inline bool
verdcf_block_equal(
    const block &a,
    const block &b
)
{
    return
        (_mm_extract_epi64(a, 0) ==
         _mm_extract_epi64(b, 0))
        &&
        (_mm_extract_epi64(a, 1) ==
         _mm_extract_epi64(b, 1));
}


// ----------------------------------------------------------------------------
// AES-MMO:
//     H_K(m) = AES_K(m) XOR m
// ----------------------------------------------------------------------------

static inline block
verdcf_mmo_hash_block(
    const block &key,
    const block &msg
)
{
    AES aes(key);

    return
        aes.ecbEncBlock(msg)
        ^
        msg;
}


// ----------------------------------------------------------------------------
// H : {0,1}^lambda -> {0,1}^{3 lambda}
//
// Three independently domain-separated AES-MMO outputs.
//
// This follows the engineering style already used by the current framework's
// VerDPF implementation.
// ----------------------------------------------------------------------------

static inline VerDCFHash3
verdcf_hash3(
    const block &seed
)
{
    static const block K[3] =
    {
        toBlock(
            0x5644434648310001ULL,
            0x1111111111111111ULL
        ),

        toBlock(
            0x5644434648310002ULL,
            0x2222222222222222ULL
        ),

        toBlock(
            0x5644434648310003ULL,
            0x3333333333333333ULL
        )
    };


    VerDCFHash3 out;


    for (int j = 0; j < 3; ++j)
    {
        const block domainSeparatedMessage =
            seed
            ^
            toBlock(
                0x5644434600000000ULL
                    ^ static_cast<uint64_t>(j),
                0xA500000000000000ULL
                    ^ static_cast<uint64_t>(j)
            );


        out.w[j] =
            verdcf_mmo_hash_block(
                K[j],
                domainSeparatedMessage
            );
    }


    return out;
}


// ----------------------------------------------------------------------------
// dst ^= src
// ----------------------------------------------------------------------------

static inline void
verdcf_hash3_xor_inplace(
    VerDCFHash3 &dst,
    const VerDCFHash3 &src
)
{
    for (int j = 0; j < 3; ++j)
    {
        dst.w[j] ^= src.w[j];
    }
}


// ----------------------------------------------------------------------------
// Encode one Boolean bit into the low bit of the first lambda-bit block.
//
//   0 -> (0,0,0)
//   1 -> (OneBlock,0,0)
// ----------------------------------------------------------------------------

static inline void
verdcf_hash3_xor_bit(
    VerDCFHash3 &dst,
    uint8_t bit
)
{
    if (bit & 1)
    {
        dst.w[0] ^= OneBlock;
    }
}


// ----------------------------------------------------------------------------
// XOR a lambda-bit random mask into the first 128-bit word of a 3lambda proof.
// ----------------------------------------------------------------------------

static inline void
verdcf_hash3_xor_block_first(
    VerDCFHash3 &dst,
    const block &mask
)
{
    dst.w[0] ^= mask;
}


// ----------------------------------------------------------------------------
// Check whether a 3lambda value is exactly a zero-extended bit:
//
//   (0,0,0)
//
// or
//
//   (OneBlock,0,0)
//
// On success, write the decoded bit to outBit.
// ----------------------------------------------------------------------------

static inline bool
verdcf_hash3_decode_bit(
    const VerDCFHash3 &h,
    uint8_t &outBit
)
{
    if (!verdcf_block_equal(
            h.w[1],
            ZeroBlock
        ))
    {
        return false;
    }


    if (!verdcf_block_equal(
            h.w[2],
            ZeroBlock
        ))
    {
        return false;
    }


    const uint64_t low =
        static_cast<uint64_t>(
            _mm_extract_epi64(
                h.w[0],
                0
            )
        );


    const uint64_t high =
        static_cast<uint64_t>(
            _mm_extract_epi64(
                h.w[0],
                1
            )
        );


    if (high != 0)
    {
        return false;
    }


    if ((low & ~1ULL) != 0)
    {
        return false;
    }


    outBit =
        static_cast<uint8_t>(
            low & 1ULL
        );


    return true;
}


// ============================================================================
// VerDCF.Gen
// ============================================================================

std::pair<VerDCFKeyPack, VerDCFKeyPack>
keyGenVerDCF(
    int bin,
    int bout,
    GroupElement alpha,
    GroupElement beta
)
{
    always_assert(bin > 0);
    always_assert(bin <= 64);

    always_assert(bout > 0);
    always_assert(bout <= 64);


    mod(alpha, bin);
    mod(beta, bout);


    static const block notOneBlock =
        toBlock(~0ULL, ~1ULL);

    static const block notThreeBlock =
        toBlock(~0ULL, ~3ULL);

    static const block TwoBlock =
        toBlock(0, 2);

    static const block ThreeBlock =
        toBlock(0, 3);


    const static block pt[4] =
    {
        ZeroBlock,
        OneBlock,
        TwoBlock,
        ThreeBlock
    };


    VerDCFKeyPack key0(
        bin,
        bout
    );

    VerDCFKeyPack key1(
        bin,
        bout
    );


    // ------------------------------------------------------------------------
    // Allocate the underlying ordinary DCF key material.
    //
    // groupSize = 1.
    // ------------------------------------------------------------------------

    block *k0 =
        new block[bin + 1];

    block *k1 =
        new block[bin + 1];


    GroupElement *v0 =
        new GroupElement[bin];

    GroupElement *g0 =
        new GroupElement[1];


    // ------------------------------------------------------------------------
    // Algorithm 4, Line 3:
    //
    //   Sample s_0^(0), s_1^(0).
    // ------------------------------------------------------------------------

    const int tid =
        omp_get_thread_num();


    auto roots =
        FSSConfig::prngs[tid]
            .get<std::array<block, 2>>();


    block s[2];

    s[0] = roots[0];
    s[1] = roots[1];


    // Same initial control-bit representation as the current ordinary DCF.
    //
    // t_0^(0) = 0
    // t_1^(0) = 1

    s[0] =
        (s[0] & notOneBlock)
        ^
        ((s[1] & OneBlock) ^ OneBlock);


    k0[0] = s[0];
    k1[0] = s[1];


    // ------------------------------------------------------------------------
    // V_alpha = 0.
    // ------------------------------------------------------------------------

    GroupElement V_alpha = 0;


    // Expanded children.
    block childS[2][2];
    block childV[2][2];

    block ct[4];


    // ========================================================================
    // Algorithm 4, Lines 5-105.
    // ========================================================================

    for (int i = 0; i < bin; ++i)
    {
        // alpha_i, from MSB to LSB.
        const uint8_t keep =
            static_cast<uint8_t>(
                alpha
                >>
                (bin - 1 - i)
            )
            &
            1;


        const uint8_t lose =
            keep ^ 1;


        // --------------------------------------------------------------------
        // Expand both parties:
        //
        // G(s_b) =
        //   s_b^L || v_b^L || t_b^L ||
        //   s_b^R || v_b^R || t_b^R
        //
        // Current framework representation:
        //
        //   ct[0] -> left seed/control state
        //   ct[1] -> right seed/control state
        //   ct[2] -> left value material
        //   ct[3] -> right value material
        // --------------------------------------------------------------------

        const block ss0 =
            s[0] & notThreeBlock;

        const block ss1 =
            s[1] & notThreeBlock;


        AES aes0(ss0);
        AES aes1(ss1);


        aes0.ecbEncFourBlocks(
            pt,
            ct
        );


        childS[0][0] = ct[0];
        childS[0][1] = ct[1];

        childV[0][0] = ct[2];
        childV[0][1] = ct[3];


        aes1.ecbEncFourBlocks(
            pt,
            ct
        );


        childS[1][0] = ct[0];
        childS[1][1] = ct[1];

        childV[1][0] = ct[2];
        childV[1][1] = ct[3];


        // --------------------------------------------------------------------
        // t_b^(i-1)
        // --------------------------------------------------------------------

        const uint8_t t0Previous =
            lsb(s[0]);

        const uint8_t t1Previous =
            lsb(s[1]);


        const GroupElement sign =
            (t1Previous == 1)
            ?
            static_cast<GroupElement>(-1)
            :
            static_cast<GroupElement>(1);


        // --------------------------------------------------------------------
        // Convert child value blocks.
        // --------------------------------------------------------------------

        uint64_t v0KeepConverted[1];
        uint64_t v1KeepConverted[1];

        uint64_t v0LoseConverted[1];
        uint64_t v1LoseConverted[1];


        convert(
            bout,
            1,
            childV[0][keep],
            v0KeepConverted
        );


        convert(
            bout,
            1,
            childV[1][keep],
            v1KeepConverted
        );


        convert(
            bout,
            1,
            childV[0][lose],
            v0LoseConverted
        );


        convert(
            bout,
            1,
            childV[1][lose],
            v1LoseConverted
        );


        // --------------------------------------------------------------------
        // Algorithm 4:
        //
        // V_CW =
        //   (-1)^t1 [
        //       Convert(v_1^Lose)
        //       - Convert(v_0^Lose)
        //       - V_alpha
        //   ]
        //
        // If Lose = L:
        //
        //   V_CW += (-1)^t1 * beta
        //
        // --------------------------------------------------------------------

        GroupElement vcw =
            sign
            *
            (
                static_cast<GroupElement>(
                    v1LoseConverted[0]
                )
                -
                static_cast<GroupElement>(
                    v0LoseConverted[0]
                )
                -
                V_alpha
            );


        if (lose == 0)
        {
            vcw +=
                sign * beta;
        }


        mod(vcw, bout);

        v0[i] = vcw;


        // --------------------------------------------------------------------
        // Update V_alpha:
        //
        // V_alpha =
        //   V_alpha
        //   - Convert(v_1^Keep)
        //   + Convert(v_0^Keep)
        //   + (-1)^t1 V_CW
        //
        // --------------------------------------------------------------------

        V_alpha =
            V_alpha
            -
            static_cast<GroupElement>(
                v1KeepConverted[0]
            )
            +
            static_cast<GroupElement>(
                v0KeepConverted[0]
            )
            +
            sign * vcw;


        mod(V_alpha, bout);


        // --------------------------------------------------------------------
        // s_CW = s_0^Lose XOR s_1^Lose
        // --------------------------------------------------------------------

        std::array<block, 2> childXor =
        {
            childS[0][0]
                ^
                childS[1][0],

            childS[0][1]
                ^
                childS[1][1]
        };


        const block alphaBit =
            toBlock(
                static_cast<uint64_t>(keep)
            );


        // --------------------------------------------------------------------
        // t_CW^L =
        //   t_0^L XOR t_1^L XOR alpha_i XOR 1
        //
        // t_CW^R =
        //   t_0^R XOR t_1^R XOR alpha_i
        // --------------------------------------------------------------------

        std::array<block, 2> tcw =
        {
            (OneBlock & childXor[0])
                ^
                alphaBit
                ^
                OneBlock,

            (OneBlock & childXor[1])
                ^
                alphaBit
        };


        const block scw =
            childXor[lose]
            &
            notThreeBlock;


        // Pack:
        //
        //   bits [127:2] = s_CW
        //   bit 1        = t_CW^L
        //   bit 0        = t_CW^R

        k0[i + 1] =
        k1[i + 1] =
            scw
            ^
            (tcw[0] << 1)
            ^
            tcw[1];


        // --------------------------------------------------------------------
        // Follow alpha path.
        // --------------------------------------------------------------------

        const block keepTCW =
            tcw[keep];


        s[0] =
            childS[0][keep]
            ^
            (
                zeroAndAllOne[t0Previous]
                &
                (
                    scw
                    ^
                    keepTCW
                )
            );


        s[1] =
            childS[1][keep]
            ^
            (
                zeroAndAllOne[t1Previous]
                &
                (
                    scw
                    ^
                    keepTCW
                )
            );


        // --------------------------------------------------------------------
        // Algorithm 4, Line 103:
        //
        // cs_i = H(s_0^(i)) XOR H(s_1^(i)).
        //
        // Hash only the actual seed part. Low two bits in the current DCF
        // representation are reserved for control information.
        // --------------------------------------------------------------------

        const VerDCFHash3 h0 =
            verdcf_hash3(
                s[0]
                &
                notThreeBlock
            );


        const VerDCFHash3 h1 =
            verdcf_hash3(
                s[1]
                &
                notThreeBlock
            );


        for (int j = 0; j < 3; ++j)
        {
            key0.cs[i].w[j] =
                h0.w[j]
                ^
                h1.w[j];


            key1.cs[i].w[j] =
                key0.cs[i].w[j];
        }
    }


    // ========================================================================
    // Algorithm 4, Line 106:
    //
    // CW^(n+1) =
    //   (-1)^t1 [
    //       Convert(s_1^n)
    //       - Convert(s_0^n)
    //       - V_alpha
    //   ]
    // ========================================================================

    uint64_t s0Converted[1];
    uint64_t s1Converted[1];


    convert(
        bout,
        1,
        s[0] & notThreeBlock,
        s0Converted
    );


    convert(
        bout,
        1,
        s[1] & notThreeBlock,
        s1Converted
    );


    GroupElement finalCW =
        static_cast<GroupElement>(
            s1Converted[0]
        )
        -
        static_cast<GroupElement>(
            s0Converted[0]
        )
        -
        V_alpha;


    if (lsb(s[1]) == 1)
    {
        finalCW =
            -finalCW;
    }


    mod(finalCW, bout);

    g0[0] = finalCW;


    // ------------------------------------------------------------------------
    // Install the underlying DCF keys.
    //
    // As in ordinary DCF key generation, g and v are common correction
    // material shared by the pair; only k0/k1 are distinct allocations.
    // ------------------------------------------------------------------------

    key0.dcfKey =
        DCFKeyPack(
            bin,
            bout,
            1,
            k0,
            g0,
            v0
        );


    key1.dcfKey =
        DCFKeyPack(
            bin,
            bout,
            1,
            k1,
            g0,
            v0
        );


    // ========================================================================
    // Algorithm 4, Lines 83 and 107:
    //
    // Sample:
    //
    //   p, l0, l1
    //
    // Set:
    //
    //   beta0 = beta XOR l0 XOR p
    //   beta1 = l1 XOR p
    //
    // ========================================================================

    GroupElement p =
        random_ge(bout);

    GroupElement l0 =
        random_ge(bout);

    GroupElement l1 =
        random_ge(bout);


    key0.lShare = l0;
    key1.lShare = l1;


    key0.betaShare =
        beta
        ^
        l0
        ^
        p;


    key1.betaShare =
        l1
        ^
        p;


    mod(
        key0.betaShare,
        bout
    );

    mod(
        key1.betaShare,
        bout
    );


    return std::make_pair(
        key0,
        key1
    );
}


// ============================================================================
// VerDCF.Eval
// ============================================================================

VerDCFEvalResult
evalVerDCF(
    int party,
    const VerDCFKeyPack &key,
    GroupElement x
)
{
    always_assert(
        party == SERVER0
        ||
        party == SERVER1
    );

    always_assert(
        key.bin > 0
    );

    always_assert(
        key.dcfKey.groupSize == 1
    );


    mod(x, key.bin);


    static const block notThreeBlock =
        toBlock(~0ULL, ~3ULL);

    static const block TwoBlock =
        toBlock(0, 2);

    static const block ThreeBlock =
        toBlock(0, 3);


    const static block expansionBlocks[4] =
    {
        ZeroBlock,
        TwoBlock,

        OneBlock,
        ThreeBlock
    };


    VerDCFProof proof(
        key.bin,
        key.bout
    );


    // ------------------------------------------------------------------------
    // Algorithm 4, Line 113.
    // ------------------------------------------------------------------------

    block s =
        key.dcfKey.k[0];


    GroupElement VShare = 0;


    proof.randomMaskXor =
        ZeroBlock;


    const int tid =
        omp_get_thread_num();


    // ========================================================================
    // Algorithm 4, Lines 114-129.
    // ========================================================================

    for (int i = 0; i < key.bin; ++i)
    {
        const uint8_t x_i =
            static_cast<uint8_t>(
                x
                >>
                (
                    key.bin
                    -
                    1
                    -
                    i
                )
            )
            &
            1;


        // t_b^(i-1)
        const uint8_t tPrevious =
            lsb(s);


        // --------------------------------------------------------------------
        // Parse CW^(i).
        // --------------------------------------------------------------------

        const block cw =
            key.dcfKey.k[i + 1];


        const block scw =
            cw
            &
            notThreeBlock;


        const block tCW[2] =
        {
            (
                cw
                >>
                1
            )
            &
            OneBlock,

            cw
            &
            OneBlock
        };


        // --------------------------------------------------------------------
        // G_C(s):
        //
        // selected branch gives:
        //   ct[0] -> child seed/control state
        //   ct[1] -> child value material
        // --------------------------------------------------------------------

        const block seedOnly =
            s
            &
            notThreeBlock;


        AES aes(seedOnly);


        block ct[2];


        aes.ecbEncTwoBlocks(
            expansionBlocks
                +
                2 * x_i,
            ct
        );


        // --------------------------------------------------------------------
        // Correct selected seed/control state.
        //
        // Same operation as current traverseOneDCF().
        // --------------------------------------------------------------------

        block sNext =
            (
                (
                    scw
                    ^
                    tCW[x_i]
                )
                &
                zeroAndAllOne[
                    tPrevious
                ]
            )
            ^
            ct[0];


        const uint8_t tNext =
            lsb(sNext);


        // --------------------------------------------------------------------
        // Value share:
        //
        // V_b +=
        //   (-1)^b [
        //       Convert(v_b^{x_i})
        //       +
        //       t_b^(i-1) V_CW^(i)
        //   ]
        // --------------------------------------------------------------------

        uint64_t convertedValue[1];


        convert(
            key.bout,
            1,
            ct[1],
            convertedValue
        );


        GroupElement valueTerm =
            static_cast<GroupElement>(
                convertedValue[0]
            );


        if (tPrevious)
        {
            valueTerm +=
                key.dcfKey.v[i];
        }


        if (party == SERVER1)
        {
            valueTerm =
                -valueTerm;
        }


        VShare +=
            valueTerm;


        mod(
            VShare,
            key.bout
        );


        // ====================================================================
        // Proof generation.
        //
        // Algorithm 4, Line 128:
        //
        // pi_b[i] =
        //   H(s_b^(i))
        //   XOR t_b^(i) * cs_i
        //   XOR t_b^(i) * x_i
        //   XOR t_b^(i-1) * x_i
        //   XOR r_b^(i)
        // ====================================================================

        proof.path[i] =
            verdcf_hash3(
                sNext
                &
                notThreeBlock
            );


        if (tNext)
        {
            verdcf_hash3_xor_inplace(
                proof.path[i],
                key.cs[i]
            );
        }


        verdcf_hash3_xor_bit(
            proof.path[i],
            static_cast<uint8_t>(
                tNext
                &
                x_i
            )
        );


        verdcf_hash3_xor_bit(
            proof.path[i],
            static_cast<uint8_t>(
                tPrevious
                &
                x_i
            )
        );


        // --------------------------------------------------------------------
        // r_b^(i) <- {0,1}^lambda
        // --------------------------------------------------------------------

        const block r =
            FSSConfig::prngs[tid]
                .get<block>();


        verdcf_hash3_xor_block_first(
            proof.path[i],
            r
        );


        // R_b <- R_b XOR r_b^(i)

        proof.randomMaskXor ^=
            r;


        s =
            sNext;
    }


    // ========================================================================
    // Algorithm 4, Line 130:
    //
    // V_b +=
    //   (-1)^b [
    //       Convert(s_b^n)
    //       +
    //       t_b^n * CW^(n+1)
    //   ]
    // ========================================================================

    const uint8_t tFinal =
        lsb(s);


    uint64_t finalSeedConverted[1];


    convert(
        key.bout,
        1,
        s & notThreeBlock,
        finalSeedConverted
    );


    GroupElement finalTerm =
        static_cast<GroupElement>(
            finalSeedConverted[0]
        );


    if (tFinal)
    {
        finalTerm +=
            key.dcfKey.g[0];
    }


    if (party == SERVER1)
    {
        finalTerm =
            -finalTerm;
    }


    VShare +=
        finalTerm;


    mod(
        VShare,
        key.bout
    );


    // ========================================================================
    // Strict-less-than correction.
    //
    // The proof equation in the supplied pseudocode distinguishes:
    //
    //   x < alpha     -> divergence bit x_i = 0
    //   x > alpha     -> divergence bit x_i = 1
    //
    // but x = alpha has no divergence and would otherwise be indistinguishable
    // from the x < alpha case in the final XOR equation.
    //
    // Along the alpha path:
    //
    //   t_0^n XOR t_1^n = 1.
    //
    // After any divergence:
    //
    //   t_0^n XOR t_1^n = 0.
    //
    // Therefore XORing each party's final t bit into the final path-proof
    // component converts the proof predicate from x <= alpha to the strict
    // predicate x < alpha without increasing proof size.
    // ========================================================================

    verdcf_hash3_xor_bit(
        proof.path[key.bin - 1],
        tFinal
    );


    // ========================================================================
    // Algorithm 4, Line 131.
    // ========================================================================

    proof.valueShare =
        VShare;


    proof.betaShare =
        key.betaShare;


    proof.lShare =
        key.lShare;


    VerDCFEvalResult result;


    result.value =
        VShare;


    result.proof =
        proof;


    return result;
}


// ============================================================================
// VerDCF.Verify
// ============================================================================

bool
verifyVerDCF(
    const VerDCFProof &pi0,
    const VerDCFProof &pi1
)
{
    if (
        pi0.bin != pi1.bin
        ||
        pi0.bout != pi1.bout
        ||
        pi0.bin <= 0
    )
    {
        return false;
    }


    const int bin =
        pi0.bin;

    const int bout =
        pi0.bout;


    // ========================================================================
    // Algorithm 4, Line 138:
    //
    // tilde_pi[i] =
    //     pi0[i] XOR pi1[i].
    //
    // Algorithm 4, Lines 140-142:
    //
    // V' =
    //     XOR_i tilde_pi[i].
    //
    // We use all n path components. The supplied pseudocode writes
    // "j = 1 to n-1" but references pi[i]; this is an indexing typo.
    // ========================================================================

    VerDCFHash3 accumulated;


    for (int i = 0; i < bin; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            accumulated.w[j] ^=
                pi0.path[i].w[j]
                ^
                pi1.path[i].w[j];
        }
    }


    // ------------------------------------------------------------------------
    // Cancel:
    //
    //   XOR_i r_0^(i)
    //   XOR
    //   XOR_i r_1^(i)
    //
    // using:
    //
    //   R_0 XOR R_1.
    //
    // This implements the:
    //
    //   V' XOR pi[n+2]
    //
    // portion of Algorithm 4's verification equation.
    // ------------------------------------------------------------------------

    accumulated.w[0] ^=
        pi0.randomMaskXor
        ^
        pi1.randomMaskXor;


    // Algorithm 4:
    //
    //   V' XOR 1 XOR tilde_pi[n+2]

    accumulated.w[0] ^=
        OneBlock;


    // After all hash and random-mask cancellations, the accumulated proof
    // must encode exactly one Boolean comparison result.

    uint8_t comparisonBit = 0;


    if (!verdcf_hash3_decode_bit(
            accumulated,
            comparisonBit
        ))
    {
        return false;
    }


    // ========================================================================
    // Recover beta:
    //
    // beta0 = beta XOR l0 XOR p
    // beta1 = l1 XOR p
    //
    // therefore:
    //
    // beta0 XOR beta1 XOR l0 XOR l1 = beta.
    //
    // This corresponds to:
    //
    //   tilde_pi[n+1] XOR tilde_pi[n+3].
    // ========================================================================

    GroupElement recoveredBeta =
        pi0.betaShare
        ^
        pi1.betaShare
        ^
        pi0.lShare
        ^
        pi1.lShare;


    mod(
        recoveredBeta,
        bout
    );


    // Expected function value:
    //
    //   beta * [x < alpha].

    GroupElement expected =
        comparisonBit
        ?
        recoveredBeta
        :
        0;


    mod(
        expected,
        bout
    );


    // The framework's DCF outputs arithmetic shares over Z_{2^bout},
    // therefore reconstruction uses addition, not XOR.

    GroupElement reconstructedValue =
        pi0.valueShare
        +
        pi1.valueShare;


    mod(
        reconstructedValue,
        bout
    );


    return
        reconstructedValue
        ==
        expected;
}



// ============================================================================
// VWDCF
// Verifiable Wide Distributed Comparison Function
//
// Four-ary implementation:
//     m     = 2
//     arity = 4
//
// Function:
//     f_{alpha,beta}(x) = beta, if x < alpha
//                         0,    otherwise
//
// This implementation follows Algorithm 6 and is adapted to the current
// framework's arithmetic-share DCF output representation.
// ============================================================================


// ----------------------------------------------------------------------------
// Constants.
// ----------------------------------------------------------------------------

static constexpr int VWDCF_STEP  = 2;
static constexpr int VWDCF_ARITY = 4;


// ----------------------------------------------------------------------------
// Extract the least significant control bit.
// ----------------------------------------------------------------------------

static inline uint8_t
vwdcf_lsb(const osuCrypto::block &b)
{
    return static_cast<uint8_t>(
        _mm_cvtsi128_si64x(b) & 1
    );
}


// ----------------------------------------------------------------------------
// Compare two 128-bit blocks.
// ----------------------------------------------------------------------------

static inline bool
vwdcf_block_equal(
    const osuCrypto::block &a,
    const osuCrypto::block &b
)
{
    return
        (
            _mm_extract_epi64(a, 0)
            ==
            _mm_extract_epi64(b, 0)
        )
        &&
        (
            _mm_extract_epi64(a, 1)
            ==
            _mm_extract_epi64(b, 1)
        );
}


// ----------------------------------------------------------------------------
// AES-MMO engineering hash.
//
//     H_K(m) = AES_K(m) XOR m
// ----------------------------------------------------------------------------

static inline osuCrypto::block
vwdcf_mmo_hash_block(
    const osuCrypto::block &key,
    const osuCrypto::block &msg
)
{
    osuCrypto::AES aes(key);

    return
        aes.ecbEncBlock(msg)
        ^
        msg;
}


// ----------------------------------------------------------------------------
// H : {0,1}^lambda -> {0,1}^{3 lambda}
//
// Engineering instantiation using three independently domain-separated
// AES-MMO outputs.
//
// Important:
// This is an implementation-level instantiation. A formal proof that relies on
// collision resistance and XOR-collision resistance should explicitly state
// the corresponding hash assumption.
// ----------------------------------------------------------------------------

static inline VWDCFHash3
vwdcf_hash3(
    const osuCrypto::block &seed
)
{
    using namespace osuCrypto;

    static const block K[3] =
    {
        toBlock(
            0x5657444346483101ULL,
            0x1111111111111111ULL
        ),

        toBlock(
            0x5657444346483102ULL,
            0x2222222222222222ULL
        ),

        toBlock(
            0x5657444346483103ULL,
            0x3333333333333333ULL
        )
    };


    VWDCFHash3 out;


    for (int j = 0; j < 3; ++j)
    {
        const block msg =
            seed
            ^
            toBlock(
                0x5657444300000000ULL
                    ^
                    static_cast<uint64_t>(j),

                0xA500000000000000ULL
                    ^
                    static_cast<uint64_t>(j)
            );


        out.w[j] =
            vwdcf_mmo_hash_block(
                K[j],
                msg
            );
    }


    return out;
}


// ----------------------------------------------------------------------------
// dst ^= src.
// ----------------------------------------------------------------------------

static inline void
vwdcf_hash3_xor_inplace(
    VWDCFHash3 &dst,
    const VWDCFHash3 &src
)
{
    for (int j = 0; j < 3; ++j)
    {
        dst.w[j] ^= src.w[j];
    }
}


// ----------------------------------------------------------------------------
// XOR a small m-bit value into a 3-lambda-bit proof.
//
// For m = 2, the chunk is zero-extended into the low bits of w[0].
// ----------------------------------------------------------------------------

static inline void
vwdcf_hash3_xor_chunk(
    VWDCFHash3 &dst,
    uint8_t chunk
)
{
    using namespace osuCrypto;

    dst.w[0] ^=
        toBlock(
            0,
            static_cast<uint64_t>(
                chunk & 0x03
            )
        );
}


// ----------------------------------------------------------------------------
// XOR one lambda-bit random mask into the first lambda-bit component.
//
// This corresponds to zero-extending r_b^(i) into the 3-lambda proof space.
// ----------------------------------------------------------------------------

static inline void
vwdcf_hash3_xor_random_mask(
    VWDCFHash3 &dst,
    const osuCrypto::block &r
)
{
    dst.w[0] ^= r;
}


// ----------------------------------------------------------------------------
// Check whether a Hash3 value is all zero.
// ----------------------------------------------------------------------------

static inline bool
vwdcf_hash3_is_zero(
    const VWDCFHash3 &h
)
{
    using namespace osuCrypto;

    return
        vwdcf_block_equal(
            h.w[0],
            ZeroBlock
        )
        &&
        vwdcf_block_equal(
            h.w[1],
            ZeroBlock
        )
        &&
        vwdcf_block_equal(
            h.w[2],
            ZeroBlock
        );
}


// ----------------------------------------------------------------------------
// Decode a 2-bit wide-tree chunk from a proof accumulator.
//
// A valid accumulator must be:
//
//     w[0] = 0...00 || chunk
//     w[1] = 0
//     w[2] = 0
//
// Returns false if additional non-zero bits exist.
// ----------------------------------------------------------------------------

static inline bool
vwdcf_decode_chunk(
    const VWDCFHash3 &h,
    uint8_t &chunk
)
{
    using namespace osuCrypto;

    if (
        !vwdcf_block_equal(
            h.w[1],
            ZeroBlock
        )
    )
    {
        return false;
    }


    if (
        !vwdcf_block_equal(
            h.w[2],
            ZeroBlock
        )
    )
    {
        return false;
    }


    const uint64_t low =
        static_cast<uint64_t>(
            _mm_extract_epi64(
                h.w[0],
                0
            )
        );


    const uint64_t high =
        static_cast<uint64_t>(
            _mm_extract_epi64(
                h.w[0],
                1
            )
        );


    if (high != 0)
    {
        return false;
    }


    if ((low & ~0x03ULL) != 0)
    {
        return false;
    }


    chunk =
        static_cast<uint8_t>(
            low & 0x03ULL
        );


    return true;
}


// ----------------------------------------------------------------------------
// Extract one 2-bit chunk from an n-bit input.
//
// Chunks are processed from MSB to LSB.
//
// Example:
//
//     bin = 8
//
//         x1 = bits 7..6
//         x2 = bits 5..4
//         x3 = bits 3..2
//         x4 = bits 1..0
//
// For odd bin, append one zero bit at the right:
//
//     bin = 5
//
//         x1 = bits 4..3
//         x2 = bits 2..1
//         x3 = bit 0 || 0
//
// This matches the current QuadDPF representation.
// ----------------------------------------------------------------------------

static inline uint8_t
vwdcf_extract_chunk(
    GroupElement x,
    int bin,
    int level
)
{
    const int shift =
        bin
        -
        VWDCF_STEP
        -
        VWDCF_STEP * level;


    if (shift >= 0)
    {
        return
            static_cast<uint8_t>(
                (x >> shift)
                &
                0x03ULL
            );
    }


    // For m = 2, the only possible negative shift is -1.
    // Append one zero at the LSB side.

    return
        static_cast<uint8_t>(
            (x << 1)
            &
            0x03ULL
        );
}


// ----------------------------------------------------------------------------
// Expand one seed into four branches.
//
// For every branch c:
//
//     G_c(s)
//         -> seed/control block
//         -> value block
//
// Plaintexts:
//
//     branch 0: 0, 1
//     branch 1: 2, 3
//     branch 2: 4, 5
//     branch 3: 6, 7
//
// The low bit of seedRaw[c] is used as t^c.
// The remaining 127 bits form s^c.
// ----------------------------------------------------------------------------

static inline void
vwdcf_expand4(
    const osuCrypto::block &seed,
    osuCrypto::block seedRaw[4],
    osuCrypto::block valueRaw[4]
)
{
    using namespace osuCrypto;

    static const block pt[8] =
    {
        toBlock(0, 0),
        toBlock(0, 1),

        toBlock(0, 2),
        toBlock(0, 3),

        toBlock(0, 4),
        toBlock(0, 5),

        toBlock(0, 6),
        toBlock(0, 7)
    };


    block ct[8];

    AES aes(seed);

    aes.ecbEncBlocks(
        pt,
        8,
        ct
    );


    for (int c = 0; c < 4; ++c)
    {
        seedRaw[c] =
            ct[2 * c];

        valueRaw[c] =
            ct[2 * c + 1];
    }
}


// ----------------------------------------------------------------------------
// Expand only one selected branch during Eval.
//
// This preserves the wide-tree online advantage:
// only the queried branch is evaluated.
// ----------------------------------------------------------------------------

static inline void
vwdcf_expand_selected(
    const osuCrypto::block &seed,
    uint8_t branch,
    osuCrypto::block &seedRaw,
    osuCrypto::block &valueRaw
)
{
    using namespace osuCrypto;

    block pt[2] =
    {
        toBlock(
            0,
            static_cast<uint64_t>(
                2 * branch
            )
        ),

        toBlock(
            0,
            static_cast<uint64_t>(
                2 * branch + 1
            )
        )
    };


    block ct[2];

    AES aes(seed);

    aes.ecbEncTwoBlocks(
        pt,
        ct
    );


    seedRaw = ct[0];
    valueRaw = ct[1];
}


// ----------------------------------------------------------------------------
// Construct d_b^J for m = 2.
//
// For J != alphaChunk:
//
//   Let h be the first position, from MSB to LSB, where
//
//       J_h != alpha_h.
//
//   d_b^J is one-hot at h iff J_h == b.
//
// Therefore:
//
//       d_0^J XOR d_1^J
//
// gives a one-hot vector locating the first different bit.
//
// Encoding:
//
//   first/MSB bit  -> 0b10
//   second/LSB bit -> 0b01
// ----------------------------------------------------------------------------

static inline void
vwdcf_make_d_pair(
    uint8_t alphaChunk,
    uint8_t branch,
    uint8_t &d0,
    uint8_t &d1
)
{
    d0 = 0;
    d1 = 0;


    if (alphaChunk == branch)
    {
        return;
    }


    // Position 1 = MSB of 2-bit chunk.
    // Position 0 = LSB of 2-bit chunk.

    for (int pos = 1; pos >= 0; --pos)
    {
        const uint8_t alphaBit =
            static_cast<uint8_t>(
                (alphaChunk >> pos)
                &
                1
            );


        const uint8_t branchBit =
            static_cast<uint8_t>(
                (branch >> pos)
                &
                1
            );


        if (alphaBit != branchBit)
        {
            const uint8_t oneHot =
                static_cast<uint8_t>(
                    1U << pos
                );


            if (branchBit == 0)
            {
                d0 = oneHot;
            }
            else
            {
                d1 = oneHot;
            }


            return;
        }
    }
}


// ============================================================================
// VWDCF.Gen
// ============================================================================

std::pair<VWDCFKeyPack, VWDCFKeyPack>
keyGenVWDCF(
    int bin,
    int bout,
    GroupElement alpha,
    GroupElement beta
)
{
    using namespace osuCrypto;


    always_assert(bin > 0);
    always_assert(bin <= 64);

    always_assert(bout > 0);
    always_assert(bout <= 64);


    mod(alpha, bin);
    mod(beta, bout);


    static const block notOneBlock =
        toBlock(
            ~0ULL,
            ~1ULL
        );


    VWDCFKeyPack key0(
        bin,
        bout
    );

    VWDCFKeyPack key1(
        bin,
        bout
    );


    const int tid =
        omp_get_thread_num();


    // ------------------------------------------------------------------------
    // Algorithm 6:
    //
    // Sample s_0^(0), s_1^(0).
    //
    // t_0^(0) = 0
    // t_1^(0) = 1
    // ------------------------------------------------------------------------

    auto roots =
        FSSConfig::prngs[tid]
            .get<std::array<block, 2>>();


    block s0 =
        roots[0]
        &
        notOneBlock;


    block s1 =
        roots[1]
        &
        notOneBlock;


    uint8_t t0 = 0;
    uint8_t t1 = 1;


    key0.root = s0;
    key1.root = s1;


    // V := 0 in G.
    GroupElement V = 0;


    // ========================================================================
    // Wide-tree expansion.
    // ========================================================================

    for (int i = 0; i < key0.depth; ++i)
    {
        // alpha_i in {0,1}^2.
        const uint8_t keep =
            vwdcf_extract_chunk(
                alpha,
                bin,
                i
            );


        // --------------------------------------------------------------------
        // Expand all four children for both parties.
        // --------------------------------------------------------------------

        block sRaw0[4];
        block sRaw1[4];

        block vRaw0[4];
        block vRaw1[4];


        vwdcf_expand4(
            s0,
            sRaw0,
            vRaw0
        );


        vwdcf_expand4(
            s1,
            sRaw1,
            vRaw1
        );


        const uint8_t t0Previous =
            t0;


        const uint8_t t1Previous =
            t1;


        const GroupElement sign =
            t1Previous
            ?
            GroupElement(-1)
            :
            GroupElement(1);


        // --------------------------------------------------------------------
        // Algorithm 6:
        //
        // Sample random r != Keep.
        //
        // Select uniformly among the other three branches.
        // --------------------------------------------------------------------

        const uint64_t randomValue =
            FSSConfig::prngs[tid]
                .get<uint64_t>();


        const uint8_t delta =
            static_cast<uint8_t>(
                1
                +
                (
                    randomValue
                    %
                    3
                )
            );


        const uint8_t r =
            static_cast<uint8_t>(
                (keep + delta)
                &
                0x03
            );


        uint8_t packedTCW = 0;


        // ====================================================================
        // Construct four branch correction words.
        // ====================================================================

        for (int c = 0; c < 4; ++c)
        {
            uint64_t convertedV0[1];
            uint64_t convertedV1[1];


            if (c != keep)
            {
                // ------------------------------------------------------------
                // s_CW^c = s_0^c XOR s_1^c.
                // ------------------------------------------------------------

                const block scw =
                    (
                        sRaw0[c]
                        ^
                        sRaw1[c]
                    )
                    &
                    notOneBlock;


                key0.scw[i * 4 + c] = scw;
                key1.scw[i * 4 + c] = scw;


                // ------------------------------------------------------------
                // t_CW^c = t_0^c XOR t_1^c.
                // ------------------------------------------------------------

                const uint8_t tcw =
                    static_cast<uint8_t>(
                        vwdcf_lsb(
                            sRaw0[c]
                        )
                        ^
                        vwdcf_lsb(
                            sRaw1[c]
                        )
                    );


                packedTCW |=
                    static_cast<uint8_t>(
                        tcw
                        <<
                        c
                    );


                // ------------------------------------------------------------
                // V_CW^c =
                //
                // (-1)^t1 [
                //     Convert(v_1^c)
                //     - Convert(v_0^c)
                //     - V
                // ]
                //
                // If c < alpha_i:
                //
                //     += (-1)^t1 beta.
                // ------------------------------------------------------------

                convert(
                    bout,
                    1,
                    vRaw0[c],
                    convertedV0
                );


                convert(
                    bout,
                    1,
                    vRaw1[c],
                    convertedV1
                );


                GroupElement vcw =
                    sign
                    *
                    (
                        static_cast<GroupElement>(
                            convertedV1[0]
                        )
                        -
                        static_cast<GroupElement>(
                            convertedV0[0]
                        )
                        -
                        V
                    );


                if (c < keep)
                {
                    vcw +=
                        sign
                        *
                        beta;
                }


                mod(vcw, bout);


                key0.vcw[i * 4 + c] = vcw;
                key1.vcw[i * 4 + c] = vcw;
            }
            else
            {
                // ------------------------------------------------------------
                // Keep branch:
                //
                // s_CW^Keep = s_0^r XOR s_1^r.
                // ------------------------------------------------------------

                const block scw =
                    (
                        sRaw0[r]
                        ^
                        sRaw1[r]
                    )
                    &
                    notOneBlock;


                key0.scw[i * 4 + c] = scw;
                key1.scw[i * 4 + c] = scw;


                // ------------------------------------------------------------
                // t_CW^Keep =
                //
                // t_0^Keep XOR t_1^Keep XOR 1.
                // ------------------------------------------------------------

                const uint8_t tcw =
                    static_cast<uint8_t>(
                        vwdcf_lsb(
                            sRaw0[c]
                        )
                        ^
                        vwdcf_lsb(
                            sRaw1[c]
                        )
                        ^
                        1
                    );


                packedTCW |=
                    static_cast<uint8_t>(
                        tcw
                        <<
                        c
                    );


                // ------------------------------------------------------------
                // V_CW^Keep uses random branch r.
                // ------------------------------------------------------------

                convert(
                    bout,
                    1,
                    vRaw0[r],
                    convertedV0
                );


                convert(
                    bout,
                    1,
                    vRaw1[r],
                    convertedV1
                );


                GroupElement vcw =
                    sign
                    *
                    (
                        static_cast<GroupElement>(
                            convertedV1[0]
                        )
                        -
                        static_cast<GroupElement>(
                            convertedV0[0]
                        )
                        -
                        V
                    );


                mod(vcw, bout);


                key0.vcw[i * 4 + c] = vcw;
                key1.vcw[i * 4 + c] = vcw;
            }


            // ------------------------------------------------------------
            // Construct party-specific d_b^c.
            // ------------------------------------------------------------

            uint8_t d0 = 0;
            uint8_t d1 = 0;


            vwdcf_make_d_pair(
                keep,
                static_cast<uint8_t>(c),
                d0,
                d1
            );


            key0.dvec[i * 4 + c] = d0;
            key1.dvec[i * 4 + c] = d1;
        }


        key0.tcw[i] = packedTCW;
        key1.tcw[i] = packedTCW;


        // ====================================================================
        // Update V along the real Keep branch.
        //
        // The pseudocode's line:
        //
        //     V <- V - Convert(v_1^b) + Convert(v_0^b)
        //              + sign * V_CW^Keep
        //
        // is interpreted as using b = Keep.
        // ====================================================================

        uint64_t keepV0[1];
        uint64_t keepV1[1];


        convert(
            bout,
            1,
            vRaw0[keep],
            keepV0
        );


        convert(
            bout,
            1,
            vRaw1[keep],
            keepV1
        );


        V =
            V
            -
            static_cast<GroupElement>(
                keepV1[0]
            )
            +
            static_cast<GroupElement>(
                keepV0[0]
            )
            +
            sign
            *
            key0.vcw[
                i * 4 + keep
            ];


        mod(V, bout);


        // ====================================================================
        // Follow Keep branch.
        // ====================================================================

        const block scwKeep =
            key0.scw[
                i * 4 + keep
            ];


        const uint8_t tcwKeep =
            static_cast<uint8_t>(
                (
                    packedTCW
                    >>
                    keep
                )
                &
                1
            );


        s0 =
            (
                sRaw0[keep]
                &
                notOneBlock
            )
            ^
            (
                t0Previous
                ?
                scwKeep
                :
                ZeroBlock
            );


        t0 =
            static_cast<uint8_t>(
                vwdcf_lsb(
                    sRaw0[keep]
                )
                ^
                (
                    t0Previous
                    ?
                    tcwKeep
                    :
                    0
                )
            );


        s1 =
            (
                sRaw1[keep]
                &
                notOneBlock
            )
            ^
            (
                t1Previous
                ?
                scwKeep
                :
                ZeroBlock
            );


        t1 =
            static_cast<uint8_t>(
                vwdcf_lsb(
                    sRaw1[keep]
                )
                ^
                (
                    t1Previous
                    ?
                    tcwKeep
                    :
                    0
                )
            );


        // ====================================================================
        // cs_i = H(s_0^(i)) XOR H(s_1^(i)).
        // ====================================================================

        const VWDCFHash3 h0 =
            vwdcf_hash3(s0);


        const VWDCFHash3 h1 =
            vwdcf_hash3(s1);


        for (int j = 0; j < 3; ++j)
        {
            key0.cs[i].w[j] =
                h0.w[j]
                ^
                h1.w[j];


            key1.cs[i].w[j] =
                key0.cs[i].w[j];
        }
    }


    // ========================================================================
    // Final CW:
    //
    // CW^(depth+1) =
    //
    // (-1)^t1 [
    //     Convert(s_1^depth)
    //     - Convert(s_0^depth)
    //     - V
    // ]
    // ========================================================================

    uint64_t convertedS0[1];
    uint64_t convertedS1[1];


    convert(
        bout,
        1,
        s0,
        convertedS0
    );


    convert(
        bout,
        1,
        s1,
        convertedS1
    );


    GroupElement finalCW =
        static_cast<GroupElement>(
            convertedS1[0]
        )
        -
        static_cast<GroupElement>(
            convertedS0[0]
        )
        -
        V;


    if (t1)
    {
        finalCW =
            -finalCW;
    }


    mod(finalCW, bout);


    key0.finalCW = finalCW;
    key1.finalCW = finalCW;


    // ========================================================================
    // beta shares:
    //
    // beta_0 = beta XOR l_0 XOR p
    // beta_1 = l_1 XOR p
    // ========================================================================

    const GroupElement p =
        random_ge(bout);


    const GroupElement l0 =
        random_ge(bout);


    const GroupElement l1 =
        random_ge(bout);


    key0.lShare = l0;
    key1.lShare = l1;


    key0.betaShare =
        beta
        ^
        l0
        ^
        p;


    key1.betaShare =
        l1
        ^
        p;


    mod(
        key0.betaShare,
        bout
    );


    mod(
        key1.betaShare,
        bout
    );


    return
        std::make_pair(
            key0,
            key1
        );
}


// ============================================================================
// VWDCF.Eval
// ============================================================================

VWDCFEvalResult
evalVWDCF(
    int party,
    const VWDCFKeyPack &key,
    GroupElement x
)
{
    using namespace osuCrypto;


    always_assert(
        party == 0
        ||
        party == 1
    );


    always_assert(
        key.bin > 0
    );


    always_assert(
        key.depth
        ==
        (key.bin + 1) / 2
    );


    mod(x, key.bin);


    static const block notOneBlock =
        toBlock(
            ~0ULL,
            ~1ULL
        );


    // ------------------------------------------------------------------------
    // Parse key.
    // ------------------------------------------------------------------------

    block s =
        key.root;


    uint8_t t =
        static_cast<uint8_t>(
            party
        );


    GroupElement VShare = 0;


    VWDCFProof proof(
        key.bin,
        key.bout
    );


    proof.randomMaskXor =
        ZeroBlock;


    const int tid =
        omp_get_thread_num();


    // ========================================================================
    // Wide-tree traversal.
    // ========================================================================

    for (int i = 0; i < key.depth; ++i)
    {
        const uint8_t xChunk =
            vwdcf_extract_chunk(
                x,
                key.bin,
                i
            );


        const uint8_t tPrevious =
            t;


        // --------------------------------------------------------------------
        // Evaluate only the selected child.
        // --------------------------------------------------------------------

        block rawSeed;
        block rawValue;


        vwdcf_expand_selected(
            s,
            xChunk,
            rawSeed,
            rawValue
        );


        const block scw =
            key.scw[
                i * 4 + xChunk
            ];


        const uint8_t tcw =
            static_cast<uint8_t>(
                (
                    key.tcw[i]
                    >>
                    xChunk
                )
                &
                1
            );


        const GroupElement vcw =
            key.vcw[
                i * 4 + xChunk
            ];


        // --------------------------------------------------------------------
        // Correct seed and t.
        // --------------------------------------------------------------------

        const block sNext =
            (
                rawSeed
                &
                notOneBlock
            )
            ^
            (
                tPrevious
                ?
                scw
                :
                ZeroBlock
            );


        const uint8_t tNext =
            static_cast<uint8_t>(
                vwdcf_lsb(
                    rawSeed
                )
                ^
                (
                    tPrevious
                    ?
                    tcw
                    :
                    0
                )
            );


        // --------------------------------------------------------------------
        // V_b +=
        //
        // (-1)^b [
        //     Convert(v_b^{x_i})
        //     + t_b^(i-1) V_CW^{x_i}
        // ]
        // --------------------------------------------------------------------

        uint64_t convertedValue[1];


        convert(
            key.bout,
            1,
            rawValue,
            convertedValue
        );


        GroupElement valueTerm =
            static_cast<GroupElement>(
                convertedValue[0]
            );


        if (tPrevious)
        {
            valueTerm +=
                vcw;
        }


        if (party == 1)
        {
            valueTerm =
                -valueTerm;
        }


        VShare +=
            valueTerm;


        mod(
            VShare,
            key.bout
        );


        // ====================================================================
        // Proof:
        //
        // pi_b[i] =
        //
        // H(s_b^(i))
        // XOR t_b^(i) * cs_i
        // XOR t_b^(i) * x_i
        // XOR t_b^(i-1) * x_i
        // XOR r_b^(i)
        // ====================================================================

        proof.path[i] =
            vwdcf_hash3(
                sNext
            );


        if (tNext)
        {
            vwdcf_hash3_xor_inplace(
                proof.path[i],
                key.cs[i]
            );
        }


        if (tNext)
        {
            vwdcf_hash3_xor_chunk(
                proof.path[i],
                xChunk
            );
        }


        if (tPrevious)
        {
            vwdcf_hash3_xor_chunk(
                proof.path[i],
                xChunk
            );
        }


        const block r =
            FSSConfig::prngs[tid]
                .get<block>();


        vwdcf_hash3_xor_random_mask(
            proof.path[i],
            r
        );


        proof.randomMaskXor ^=
            r;


        // --------------------------------------------------------------------
        // pi_b[depth + i] = d_b^{x_i}.
        // --------------------------------------------------------------------

        proof.dProof[i] =
            key.dvec[
                i * 4 + xChunk
            ];


        s = sNext;
        t = tNext;
    }


    // ========================================================================
    // Final output correction:
    //
    // V_b +=
    //
    // (-1)^b [
    //     Convert(s_b^depth)
    //     + t_b^depth * finalCW
    // ]
    // ========================================================================

    uint64_t convertedFinalSeed[1];


    convert(
        key.bout,
        1,
        s,
        convertedFinalSeed
    );


    GroupElement finalTerm =
        static_cast<GroupElement>(
            convertedFinalSeed[0]
        );


    if (t)
    {
        finalTerm +=
            key.finalCW;
    }


    if (party == 1)
    {
        finalTerm =
            -finalTerm;
    }


    VShare +=
        finalTerm;


    mod(
        VShare,
        key.bout
    );


    // ------------------------------------------------------------------------
    // Final proof components.
    // ------------------------------------------------------------------------

    proof.valueShare =
        VShare;


    proof.betaShare =
        key.betaShare;


    proof.lShare =
        key.lShare;


    VWDCFEvalResult result;


    result.value =
        VShare;


    result.proof =
        proof;


    return result;
}


// ============================================================================
// VWDCF.Verify
// ============================================================================

bool
verifyVWDCF(
    const VWDCFProof &pi0,
    const VWDCFProof &pi1
)
{
    using namespace osuCrypto;


    // ------------------------------------------------------------------------
    // Basic structural validation.
    // ------------------------------------------------------------------------

    if (
        pi0.bin != pi1.bin
        ||
        pi0.bout != pi1.bout
        ||
        pi0.depth != pi1.depth
        ||
        pi0.bin <= 0
        ||
        pi0.depth <= 0
    )
    {
        return false;
    }


    const int depth =
        pi0.depth;


    const int bout =
        pi0.bout;


    // ========================================================================
    // Accumulate:
    //
    // V' =
    //
    // XOR_i (
    //     pi_0[i]
    //     XOR
    //     pi_1[i]
    // )
    //
    // Then cancel:
    //
    //     R_0 XOR R_1.
    //
    // Before the first divergent wide level:
    //     path proof cancels to random masks.
    //
    // At the first divergent wide level:
    //     path proof contributes x_i.
    //
    // After divergence:
    //     the paths have merged and contribute only random masks.
    //
    // Therefore after removing R_0 XOR R_1:
    //
    //     accumulator = x_k
    //
    // where k is the first differing wide level.
    // ========================================================================

    VWDCFHash3 accumulator;


    for (int i = 0; i < depth; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            accumulator.w[j] ^=
                pi0.path[i].w[j]
                ^
                pi1.path[i].w[j];
        }
    }


    accumulator.w[0] ^=
        pi0.randomMaskXor
        ^
        pi1.randomMaskXor;


    // ========================================================================
    // Find the first wide level where alpha_i != x_i.
    //
    // d_0^{x_i} XOR d_1^{x_i}
    //
    // must be:
    //
    //     00 : no differing bit in this wide chunk
    //     10 : first difference at the MSB of the 2-bit chunk
    //     01 : first difference at the LSB of the 2-bit chunk
    // ========================================================================

    int firstDifferentLevel = -1;

    uint8_t firstDifferenceMask = 0;


    for (int i = 0; i < depth; ++i)
    {
        const uint8_t d =
            static_cast<uint8_t>(
                pi0.dProof[i]
                ^
                pi1.dProof[i]
            );


        // Only two bits are valid.
        if ((d & ~0x03U) != 0)
        {
            return false;
        }


        if (
            firstDifferentLevel < 0
            &&
            d != 0
        )
        {
            // A valid first-difference locator must be one-hot.
            if (
                d != 0x01
                &&
                d != 0x02
            )
            {
                return false;
            }


            firstDifferentLevel = i;
            firstDifferenceMask = d;
        }
    }


    uint8_t comparisonBit = 0;


    // ========================================================================
    // Equality case:
    //
    // x == alpha.
    //
    // There is no first different wide chunk, and the accumulated path proof
    // must be zero after random-mask cancellation.
    // ========================================================================

    if (firstDifferentLevel < 0)
    {
        if (
            !vwdcf_hash3_is_zero(
                accumulator
            )
        )
        {
            return false;
        }


        comparisonBit = 0;
    }
    else
    {
        // ====================================================================
        // Non-equality case.
        //
        // Decode x_k from V'.
        // ====================================================================

        uint8_t xChunk = 0;


        if (
            !vwdcf_decode_chunk(
                accumulator,
                xChunk
            )
        )
        {
            return false;
        }


        // --------------------------------------------------------------------
        // D identifies the first different bit inside this 2-bit chunk.
        //
        // If:
        //
        //     firstDifferenceMask = 10
        //
        // inspect the chunk MSB.
        //
        // If:
        //
        //     firstDifferenceMask = 01
        //
        // inspect the chunk LSB.
        //
        // At the first different bit:
        //
        //     x_h = 0 => alpha_h = 1 => x < alpha
        //
        //     x_h = 1 => alpha_h = 0 => x > alpha
        // --------------------------------------------------------------------

        const int bitPosition =
            (
                firstDifferenceMask
                ==
                0x02
            )
            ?
            1
            :
            0;


        const uint8_t xBit =
            static_cast<uint8_t>(
                (
                    xChunk
                    >>
                    bitPosition
                )
                &
                1
            );


        comparisonBit =
            static_cast<uint8_t>(
                xBit
                ^
                1
            );
    }


    // ========================================================================
    // Recover beta.
    //
    // beta_0 = beta XOR l_0 XOR p
    // beta_1 = l_1 XOR p
    //
    // Therefore:
    //
    // beta =
    //     beta_0 XOR beta_1 XOR l_0 XOR l_1.
    // ========================================================================

    GroupElement recoveredBeta =
        pi0.betaShare
        ^
        pi1.betaShare
        ^
        pi0.lShare
        ^
        pi1.lShare;


    mod(
        recoveredBeta,
        bout
    );


    // ========================================================================
    // Expected comparison output.
    // ========================================================================

    GroupElement expected =
        comparisonBit
        ?
        recoveredBeta
        :
        0;


    mod(
        expected,
        bout
    );


    // ========================================================================
    // Current framework uses arithmetic output shares.
    //
    // Therefore:
    //
    //     y = y_0 + y_1 mod 2^bout
    //
    // rather than XOR reconstruction.
    // ========================================================================

    GroupElement reconstructed =
        pi0.valueShare
        +
        pi1.valueShare;


    mod(
        reconstructed,
        bout
    );


    return
        reconstructed
        ==
        expected;
}