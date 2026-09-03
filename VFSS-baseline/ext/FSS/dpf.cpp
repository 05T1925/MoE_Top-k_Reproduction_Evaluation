#include <FSS/dpf.h>
#include <FSS/assert.h>
#include <cassert>
#include <array>
#include <vector>
#include <cstdint>
#include <omp.h>

using namespace osuCrypto;

inline u8 lsb(const block &b)
{
    return _mm_cvtsi128_si64x(b) & 1;
}

/*
 * lambda = 127
 */
std::pair<DPFKeyPack, DPFKeyPack> keyGenDPF(int bin, int bout, GroupElement idx, GroupElement payload)
{
    always_assert(bin <= 64);
    always_assert(bout <= 64);
    static const block notOneBlock = toBlock(~0, ~1);
    const static block pt[2] = {ZeroBlock, OneBlock};

    DPFKeyPack key0(bin, bout);
    DPFKeyPack key1(bin, bout);

    int tid = omp_get_thread_num();
    auto s = FSSConfig::prngs[tid].get<std::array<block, 2>>();
    auto s0 = s[0];
    auto s1 = s[1];

    s0 = s0 & notOneBlock;
    s1 = s1 & notOneBlock;
    key0.s[0] = s0;
    key1.s[0] = s1;
    
    u8 t0 = 0;
    u8 t1 = 1;

    block ct0[2];
    block ct1[2];

    for (int i = 0; i < bin; ++i)
    {
        const u8 keep = static_cast<uint8_t>(idx >> (bin - 1 - i)) & 1;
        const u8 loose = keep ^ 1;

        AES ak0(s0);
        AES ak1(s1);

        ak0.ecbEncTwoBlocks(pt, ct0);
        ak1.ecbEncTwoBlocks(pt, ct1);

        auto scw = (ct0[loose] ^ ct1[loose]) & notOneBlock;
        u64 tcw[2];
        u64 &tLcw = tcw[0];
        u64 &tRcw = tcw[1];
        tLcw = lsb(ct0[0]) ^ lsb(ct1[0]) ^ keep ^ 1;
        tRcw = lsb(ct0[1]) ^ lsb(ct1[1]) ^ keep;

        key0.s[i+1] = scw;
        key1.s[i+1] = scw;
        key0.tLcw |= (tLcw << (bin - 1 - i));
        key0.tRcw |= (tRcw << (bin - 1 - i));

        if (t0 == 0)
        {
            s0 = ct0[keep] & notOneBlock;
            t0 = lsb(ct0[keep]);
        }
        else
        {
            s0 = (ct0[keep] & notOneBlock) ^ scw;
            t0 = lsb(ct0[keep]) ^ tcw[keep];
        }

        if (t1 == 0)
        {
            s1 = ct1[keep] & notOneBlock;
            t1 = lsb(ct1[keep]);
        }
        else
        {
            s1 = (ct1[keep] & notOneBlock) ^ scw;
            t1 = lsb(ct1[keep]) ^ tcw[keep];
        }
    }

    key1.tLcw = key0.tLcw;
    key1.tRcw = key0.tRcw;

    key0.payload = payload - _mm_extract_epi64(s0, 0) + _mm_extract_epi64(s1, 0);
    if (t1 == 1) key0.payload = -key0.payload;
    key1.payload = key0.payload;

    return std::make_pair(key0, key1);
}

GroupElement evalDPF_EQ(int party, DPFKeyPack &key, GroupElement x)
{
    static const block notOneBlock = toBlock(~0, ~1);
    int bin = key.bin;
    int bout = key.bout;

    auto s = _mm_loadu_si128(key.s);
    u8 t = party;
    for (int i = 0; i < bin; ++i)
    {
        assert(lsb(s) == 0);
        const u8 x_i = static_cast<uint8_t>(x >> (bin - 1 - i)) & 1;
        
        AES ak(s);
        block ct = ak.ecbEncBlock(toBlock(0, x_i));
        s = ct & notOneBlock;
        u8 t_old = t;
        t = lsb(ct);

        if (t_old) {
            s = s ^ _mm_loadu_si128(key.s + i + 1);
            t = t ^ ((key.tcw[x_i] >> (bin - 1 - i)) & 1);
        }
    }

    return t;
}

GroupElement evalDPF_EQ2(int party, DPFKeyPack &key, GroupElement x)
{
    static const osuCrypto::block notOneBlock = osuCrypto::toBlock(~0, ~1);
    // 定义左右子节点的明文块 0 和 1
    const static osuCrypto::block pt[2] = {osuCrypto::ZeroBlock, osuCrypto::OneBlock};
    
    int bin = key.bin;
    auto s = _mm_loadu_si128(key.s);
    u8 t = party;

    for (int i = 0; i < bin; ++i)
    {
        assert(lsb(s) == 0);
        const u8 x_i = static_cast<uint8_t>(x >> (bin - 1 - i)) & 1;

        osuCrypto::AES ak(s);
        osuCrypto::block ct[2];
        
        // 【核心修改点】：严格按照论文伪代码，强制生成左右两棵子树（产生2次AES加密开销）
        ak.ecbEncTwoBlocks(pt, ct);

        // 生成完毕后，再根据 x_i 获取真实需要走的那一条分支
        osuCrypto::block selected_ct = ct[x_i];

        u8 t_old = t;
        s = selected_ct & notOneBlock;
        t = lsb(selected_ct);

        if (t_old) {
            s = s ^ _mm_loadu_si128(key.s + i + 1);
            t = t ^ ((key.tcw[x_i] >> (bin - 1 - i)) & 1);
        }
    }

    return t;
}
GroupElement evalDPF_EQ2_slow(int party, DPFKeyPack &key, GroupElement x)
{
    static const block notOneBlock = toBlock(~0, ~1);
    int bin = key.bin;

    auto s = _mm_loadu_si128(key.s);
    u8 t = party;

    for (int i = 0; i < bin; ++i)
    {
        const u8 x_i = static_cast<uint8_t>(x >> (bin - 1 - i)) & 1;

        AES ak(s);

        block ct0 = ak.ecbEncBlock(toBlock(0, 0));
        block ct1 = ak.ecbEncBlock(toBlock(0, 1));

        block selected_ct = x_i ? ct1 : ct0;

        u8 t_old = t;
        s = selected_ct & notOneBlock;
        t = lsb(selected_ct);

        if (t_old) {
            s = s ^ _mm_loadu_si128(key.s + i + 1);
            t = t ^ ((key.tcw[x_i] >> (bin - 1 - i)) & 1);
        }
    }

    return t;
}

GroupElement evalDPF_GT(int party, DPFKeyPack &key, GroupElement x)
{
    static const block notOneBlock = toBlock(~0, ~1);
    int bin = key.bin;
    int bout = key.bout;

    auto s = _mm_loadu_si128(key.s);
    u8 t = party;
    
    u8 x_prev = 0;
    u8 t_dcf = 0;

    for (int i = 0; i < bin; ++i)
    {
        assert(lsb(s) == 0);
        const u8 x_i = static_cast<uint8_t>(x >> (bin - 1 - i)) & 1;

        if (x_prev != x_i)
        {
            t_dcf = t_dcf ^ t;
        }
        x_prev = x_i;

        AES ak(s);
        block ct = ak.ecbEncBlock(toBlock(0, x_i));
        s = ct & notOneBlock;
        u8 t_old = t;
        t = lsb(ct);
        

        if (t_old) {
            s = s ^ _mm_loadu_si128(key.s + i + 1);
            t = t ^ ((key.tcw[x_i] >> (bin - 1 - i)) & 1);
        }
    }

    if (x_prev == 1)
    {
        t_dcf = t_dcf ^ t;
    }
    return t_dcf;
}

GroupElement evalDPF_LT(int party, DPFKeyPack &key, GroupElement x)
{
    static const block notOneBlock = toBlock(~0, ~1);
    int bin = key.bin;
    int bout = key.bout;

    auto s = _mm_loadu_si128(key.s);
    u8 t = party;
    
    u8 x_prev = 1;
    u8 t_dcf = 0;

    for (int i = 0; i < bin; ++i)
    {
        assert(lsb(s) == 0);
        const u8 x_i = static_cast<uint8_t>(x >> (bin - 1 - i)) & 1;

        if (x_prev != x_i)
        {
            t_dcf = t_dcf ^ t;
        }
        x_prev = x_i;

        AES ak(s);
        block ct = ak.ecbEncBlock(toBlock(0, x_i));
        s = ct & notOneBlock;
        u8 t_old = t;
        t = lsb(ct);
        

        if (t_old) {
            s = s ^ _mm_loadu_si128(key.s + i + 1);
            t = t ^ ((key.tcw[x_i] >> (bin - 1 - i)) & 1);
        }
    }

    if (x_prev == 0)
    {
        t_dcf = t_dcf ^ t;
    }
    return t_dcf;
}

void evalAll_helper(int party, DPFKeyPack &key, GroupElement rightShift, GroupElement *out, block s_prev, u8 t_prev, int i, GroupElement acc)
{
    if (i == key.bin)
    {
        GroupElement idx = acc + rightShift;
        mod(idx, key.bin);
        out[idx] = (1 - 2 * party) * (_mm_extract_epi64(s_prev, 0) + key.payload * t_prev);
        return;
    }

    const static block pt[2] = {ZeroBlock, OneBlock};
    static const block notOneBlock = toBlock(~0, ~1);

    AES ak(s_prev);
    block ct[2];
    ak.ecbEncTwoBlocks(pt, ct);

    for (int x_i = 0; x_i < 2; ++x_i)
    {
        block s = ct[x_i] & notOneBlock;
        u8 t = lsb(ct[x_i]);

        if (t_prev) {
            s = s ^ _mm_loadu_si128(key.s + i + 1);
            t = t ^ ((key.tcw[x_i] >> (key.bin - 1 - i)) & 1);
        }

        evalAll_helper(party, key, rightShift, out, s, t, i+1, 2 * acc + x_i);
    }
}

void evalAll(int party, DPFKeyPack &key, GroupElement rightShift, GroupElement *out)
{
    auto s = _mm_loadu_si128(key.s);
    u8 t = party;
    
    evalAll_helper(party, key, rightShift, out, s, t, 0, 0);
}

void evalAll_reduce_helper(int party, DPFKeyPack &key, GroupElement rightShift, const std::vector<GroupElement> &tab, GroupElement &out, block &s_prev, u8 t_prev, int i, GroupElement acc)
{
    if (i == key.bin)
    {
        GroupElement idx = acc + rightShift;
        mod(idx, key.bin);
        out = out + tab[idx] * ((1 - 2 * party) * (_mm_extract_epi64(s_prev, 0) + key.payload * t_prev));
        return;
    }

    const static block pt[2] = {ZeroBlock, OneBlock};
    static const block notOneBlock = toBlock(~0, ~1);

    AES ak(s_prev);
    block ct[2];
    ak.ecbEncTwoBlocks(pt, ct);

    for (int x_i = 0; x_i < 2; ++x_i)
    {
        block s = ct[x_i] & notOneBlock;
        u8 t = lsb(ct[x_i]);

        if (t_prev) {
            s = s ^ _mm_loadu_si128(key.s + i + 1);
            t = t ^ ((key.tcw[x_i] >> (key.bin - 1 - i)) & 1);
        }

        evalAll_reduce_helper(party, key, rightShift, tab, out, s, t, i+1, 2 * acc + x_i);
    }
}

GroupElement evalAll_reduce(int party, DPFKeyPack &key, GroupElement rightShift, const std::vector<GroupElement> &tab)
{
    auto s = _mm_loadu_si128(key.s);
    u8 t = party;
    GroupElement out = 0;
    
    evalAll_reduce_helper(party, key, rightShift, tab, out, s, t, 0, 0);
    return out;
}

std::pair<DPFETKeyPack, DPFETKeyPack> keyGenDPFET(int bin, GroupElement idx)
{
    always_assert(bin <= 64);
    always_assert(bin >= 8);
    static const block notOneBlock = toBlock(~0, ~1);
    const static block pt[2] = {ZeroBlock, OneBlock};

    DPFETKeyPack key0(bin);
    DPFETKeyPack key1(bin);

    int tid = omp_get_thread_num();
    auto s = FSSConfig::prngs[tid].get<std::array<block, 2>>();
    auto s0 = s[0];
    auto s1 = s[1];

    s0 = s0 & notOneBlock;
    s1 = s1 & notOneBlock;
    key0.s[0] = s0;
    key1.s[0] = s1;
    
    u8 t0 = 0;
    u8 t1 = 1;

    block ct0[2];
    block ct1[2];

    for (int i = 0; i < bin - 7; ++i)
    {
        const u8 keep = static_cast<uint8_t>(idx >> (bin - 1 - i)) & 1;
        const u8 loose = keep ^ 1;

        AES ak0(s0);
        AES ak1(s1);

        ak0.ecbEncTwoBlocks(pt, ct0);
        ak1.ecbEncTwoBlocks(pt, ct1);

        auto scw = (ct0[loose] ^ ct1[loose]) & notOneBlock;
        u64 tcw[2];
        u64 &tLcw = tcw[0];
        u64 &tRcw = tcw[1];
        tLcw = lsb(ct0[0]) ^ lsb(ct1[0]) ^ keep ^ 1;
        tRcw = lsb(ct0[1]) ^ lsb(ct1[1]) ^ keep;

        key0.s[i+1] = scw;
        key1.s[i+1] = scw;
        key0.tLcw |= (tLcw << (bin - 1 - i));
        key0.tRcw |= (tRcw << (bin - 1 - i));

        if (t0 == 0)
        {
            s0 = ct0[keep] & notOneBlock;
            t0 = lsb(ct0[keep]);
        }
        else
        {
            s0 = (ct0[keep] & notOneBlock) ^ scw;
            t0 = lsb(ct0[keep]) ^ tcw[keep];
        }

        if (t1 == 0)
        {
            s1 = ct1[keep] & notOneBlock;
            t1 = lsb(ct1[keep]);
        }
        else
        {
            s1 = (ct1[keep] & notOneBlock) ^ scw;
            t1 = lsb(ct1[keep]) ^ tcw[keep];
        }
    }

    key1.tLcw = key0.tLcw;
    key1.tRcw = key0.tRcw;

    if (t0 == 1) s0 = s0 ^ OneBlock;
    if (t1 == 1) s1 = s1 ^ OneBlock;
    uint64_t e0, e1;
    GroupElement ip = idx % 128;
    if (ip >= 64) {
        e0 = 0;
        e1 = 1ULL << (127 - ip);
    }
    else {
        e0 = 1ULL << (63 - ip);
        e1 = 0;
    }
    key0.leaf = s0 ^ s1 ^ osuCrypto::toBlock(e0, e1);
    key1.leaf = key0.leaf;

    return std::make_pair(key0, key1);
}

GroupElement evalDPFET_LT(int party, const DPFETKeyPack &key, GroupElement x)
{
    static const block notOneBlock = toBlock(~0, ~1);
    int bin = key.bin;

    auto s = _mm_loadu_si128(key.s);
    u8 t = party;
    
    u8 x_prev = 1;
    u8 t_dcf = 0;

    for (int i = 0; i < bin - 7; ++i)
    {
        assert(lsb(s) == 0);
        const u8 x_i = static_cast<uint8_t>(x >> (bin - 1 - i)) & 1;

        if (x_prev != x_i)
        {
            t_dcf = t_dcf ^ t;
        }
        x_prev = x_i;

        AES ak(s);
        block ct = ak.ecbEncBlock(toBlock(0, x_i));
        s = ct & notOneBlock;
        u8 t_old = t;
        t = lsb(ct);

        if (t_old) {
            s = s ^ _mm_loadu_si128(key.s + i + 1);
            t = t ^ ((key.tcw[x_i] >> (bin - 1 - i)) & 1);
        }
    }

    osuCrypto::block leaf = s;
    if (t) leaf = leaf ^ OneBlock ^ key.leaf;
    uint64_t b;

    {
        const u8 x_i = static_cast<uint8_t>(x >> 6) & 1;
        if (x_prev != x_i)
        {
            t_dcf = t_dcf ^ t;
        }
        x_prev = x_i;
        if (x_i) 
        {
            b = _mm_extract_epi64(leaf, 0);
        }
        else 
        {
            b = _mm_extract_epi64(leaf, 1);
        }
        t = __builtin_parityll(b);
    }

    GroupElement xp = x % 64;

    if (x_prev == 0)
    {
        for (int i = 0; i <= xp; ++i)
        {
            t_dcf = t_dcf ^ ((b >> (63 - i)) & 1);
        }
    }
    else
    {
        for (int i = xp + 1; i < 64; ++i)
        {
            t_dcf = t_dcf ^ ((b >> (63 - i)) & 1);
        }
    }

    return t_dcf;
}

void evalAll_reduce_helper_et(int party, DPFETKeyPack &key, GroupElement rightShift, const std::vector<GroupElement> &tab, GroupElement &out, GroupElement &corr, block &s_prev, u8 t_prev, int i, GroupElement acc)
{
    if (i == key.bin - 7)
    {
        osuCrypto::block leaf = s_prev;
        if (t_prev) leaf = leaf ^ OneBlock ^ key.leaf;
        
        uint64_t b = _mm_extract_epi64(leaf, 1);
        for (int j = 0; j < 64; ++j) {
            GroupElement idx = 128 * acc + j + rightShift;
            mod(idx, key.bin);
            GroupElement e = ((1 - 2 * party) * ((b >> (63 - j)) & 1));
            out = out + tab[idx] * e;
            corr = corr + e;
        }

        b = _mm_extract_epi64(leaf, 0);
        for (int j = 64; j < 128; ++j) {
            GroupElement idx = 128 * acc + j + rightShift;
            mod(idx, key.bin);
            GroupElement e = ((1 - 2 * party) * ((b >> (127 - j)) & 1));
            out = out + tab[idx] * e;
            corr = corr + e;
        }
        return;
    }

    const static block pt[2] = {ZeroBlock, OneBlock};
    static const block notOneBlock = toBlock(~0, ~1);

    AES ak(s_prev);
    block ct[2];
    ak.ecbEncTwoBlocks(pt, ct);

    for (int x_i = 0; x_i < 2; ++x_i)
    {
        block s = ct[x_i] & notOneBlock;
        u8 t = lsb(ct[x_i]);

        if (t_prev) {
            s = s ^ _mm_loadu_si128(key.s + i + 1);
            t = t ^ ((key.tcw[x_i] >> (key.bin - 1 - i)) & 1);
        }

        evalAll_reduce_helper_et(party, key, rightShift, tab, out, corr, s, t, i+1, 2 * acc + x_i);
    }
}

std::pair<GroupElement, GroupElement> evalAll_reduce_et(int party, DPFETKeyPack &key, GroupElement rightShift, const std::vector<GroupElement> &tab)
{
    auto s = _mm_loadu_si128(key.s);
    u8 t = party;
    GroupElement out = 0;
    GroupElement corr = 0;
    
    evalAll_reduce_helper_et(party, key, rightShift, tab, out, corr, s, t, 0, 0);
    return std::make_pair(out, corr);
}

std::pair<DPFETKeyPack, DPFETKeyPack> keyGenGTDPF(int bin, GroupElement idx)
{
    always_assert(bin <= 64);
    always_assert(bin >= 8);
    static const block notOneBlock = toBlock(~0, ~1);
    const static block pt[2] = {ZeroBlock, OneBlock};

    DPFETKeyPack key0(bin);
    DPFETKeyPack key1(bin);

    int tid = omp_get_thread_num();
    auto s = FSSConfig::prngs[tid].get<std::array<block, 2>>();
    auto s0 = s[0];
    auto s1 = s[1];

    s0 = s0 & notOneBlock;
    s1 = s1 & notOneBlock;
    key0.s[0] = s0;
    key1.s[0] = s1;
    
    u8 t0 = 0;
    u8 t1 = 1;

    block ct0[2];
    block ct1[2];

    for (int i = 0; i < bin - 7; ++i)
    {
        const u8 keep = static_cast<uint8_t>(idx >> (bin - 1 - i)) & 1;
        const u8 loose = keep ^ 1;

        AES ak0(s0);
        AES ak1(s1);

        ak0.ecbEncTwoBlocks(pt, ct0);
        ak1.ecbEncTwoBlocks(pt, ct1);

        auto scw = (ct0[loose] ^ ct1[loose]) & notOneBlock;
        u64 tcw[2];
        u64 &tLcw = tcw[0];
        u64 &tRcw = tcw[1];
        tLcw = lsb(ct0[0]) ^ lsb(ct1[0]) ^ keep ^ 1;
        tRcw = lsb(ct0[1]) ^ lsb(ct1[1]) ^ keep;

        key0.s[i+1] = scw;
        key1.s[i+1] = scw;
        key0.tLcw |= (tLcw << (bin - 1 - i));
        key0.tRcw |= (tRcw << (bin - 1 - i));

        if (t0 == 0)
        {
            s0 = ct0[keep] & notOneBlock;
            t0 = lsb(ct0[keep]);
        }
        else
        {
            s0 = (ct0[keep] & notOneBlock) ^ scw;
            t0 = lsb(ct0[keep]) ^ tcw[keep];
        }

        if (t1 == 0)
        {
            s1 = ct1[keep] & notOneBlock;
            t1 = lsb(ct1[keep]);
        }
        else
        {
            s1 = (ct1[keep] & notOneBlock) ^ scw;
            t1 = lsb(ct1[keep]) ^ tcw[keep];
        }
    }

    key1.tLcw = key0.tLcw;
    key1.tRcw = key0.tRcw;

    if (t0 == 1) s0 = s0 ^ OneBlock;
    if (t1 == 1) s1 = s1 ^ OneBlock;
    uint64_t e0, e1;
    GroupElement ip = idx % 128;
    if (ip >= 64) {
        e0 = 0;
        e1 = 1ULL << (127 - ip);
    }
    else {
        e0 = 1ULL << (63 - ip);
        e1 = 0;
    }
    key0.leaf = s0 ^ s1 ^ osuCrypto::toBlock(e0, e1);
    key1.leaf = key0.leaf;

    return std::make_pair(key0, key1);
}


GroupElement evalGTDPF(int party, const DPFETKeyPack &key, GroupElement x)
{
    static const block notOneBlock = toBlock(~0, ~1);
    int bin = key.bin;

    auto s = _mm_loadu_si128(key.s);
    u8 t = party;
    
    u8 x_prev = 1;
    u8 t_dcf = 0;

    for (int i = 0; i < bin - 7; ++i)
    {
        assert(lsb(s) == 0);
        const u8 x_i = static_cast<uint8_t>(x >> (bin - 1 - i)) & 1;

        if (x_prev != x_i)
        {
            t_dcf = t_dcf ^ t;
        }
        x_prev = x_i;

        AES ak(s);
        block ct = ak.ecbEncBlock(toBlock(0, x_i));
        s = ct & notOneBlock;
        u8 t_old = t;
        t = lsb(ct);

        if (t_old) {
            s = s ^ _mm_loadu_si128(key.s + i + 1);
            t = t ^ ((key.tcw[x_i] >> (bin - 1 - i)) & 1);
        }
    }

    osuCrypto::block leaf = s;
    if (t) leaf = leaf ^ OneBlock ^ key.leaf;
    uint64_t b;

    {
        const u8 x_i = static_cast<uint8_t>(x >> 6) & 1;
        if (x_prev != x_i)
        {
            t_dcf = t_dcf ^ t;
        }
        x_prev = x_i;
        if (x_i) 
        {
            b = _mm_extract_epi64(leaf, 0);
        }
        else 
        {
            b = _mm_extract_epi64(leaf, 1);
        }
        t = __builtin_parityll(b);
    }

    GroupElement xp = x % 64;

    if (x_prev == 0)
    {
        for (int i = 0; i <= xp; ++i)
        {
            t_dcf = t_dcf ^ ((b >> (63 - i)) & 1);
        }
    }
    else
    {
        for (int i = xp + 1; i < 64; ++i)
        {
            t_dcf = t_dcf ^ ((b >> (63 - i)) & 1);
        }
    }
    if (party==1)
    {
       t_dcf = t_dcf ^ 1;
    }
    else{
        t_dcf = t_dcf ^ 0;
    }
    

    return t_dcf;
}

// =========================================================================
// GTDCF
// =========================================================================

std::pair<GTDCFKeyPack, GTDCFKeyPack> keyGenGTDCF(
    int bin, int w, int groupSize, GroupElement idx, const GroupElement* beta)
{
    always_assert(bin <= 64);
    always_assert(groupSize == 2); // 强制为2以启用极限优化
    static const block notOneBlock = toBlock(~0, ~1);
    
    // 0:左分支, 1:右分支, 2:负载掩码
    const static block pt[3] = {ZeroBlock, OneBlock, toBlock(0, 2)};

    GTDCFKeyPack key0(bin, w, 2);
    GTDCFKeyPack key1(bin, w, 2);

    int tid = omp_get_thread_num();
    auto s = FSSConfig::prngs[tid].get<std::array<block, 2>>();
    
    key0.seed = s[0] & notOneBlock;
    key1.seed = s[1] & notOneBlock;
    block s0 = key0.seed, s1 = key1.seed;
    u8 t0 = 0, t1 = 1;
    int d = bin - w;

    for (int i = 0; i < d; ++i)
    {
        const u8 keep_dir = static_cast<uint8_t>(idx >> (bin - 1 - i)) & 1;
        const u8 loose_dir = keep_dir ^ 1;

        AES ak0(s0); AES ak1(s1);
        block ct0[3], ct1[3];
        ak0.ecbEncBlocks(pt, 3, ct0);
        ak1.ecbEncBlocks(pt, 3, ct1);

        block scw = (ct0[loose_dir] ^ ct1[loose_dir]) & notOneBlock;
        uint8_t tcwL = lsb(ct0[0]) ^ lsb(ct1[0]) ^ keep_dir ^ 1;
        uint8_t tcwR = lsb(ct0[1]) ^ lsb(ct1[1]) ^ keep_dir;

        key0.scw[i] = key1.scw[i] = scw;
        key0.tcw[2 * i] = key1.tcw[2 * i] = tcwL;
        key0.tcw[2 * i + 1] = key1.tcw[2 * i + 1] = tcwR;

        // 极限优化：直接用 SSE 指令提取 64 位整数，告别 memcpy
        GroupElement v0_0 = _mm_extract_epi64(ct0[2], 0);
        GroupElement v0_1 = _mm_extract_epi64(ct0[2], 1);
        GroupElement v1_0 = _mm_extract_epi64(ct1[2], 0);
        GroupElement v1_1 = _mm_extract_epi64(ct1[2], 1);

        int offset = i * 2;
        GroupElement target0 = (keep_dir == 0) ? beta[0] : 0;
        GroupElement target1 = (keep_dir == 0) ? beta[1] : 0;

        if (t0 == 1) {
            key0.vcw[offset]     = key1.vcw[offset]     = target0 - v0_0 + v1_0;
            key0.vcw[offset + 1] = key1.vcw[offset + 1] = target1 - v0_1 + v1_1;
        } else {
            key0.vcw[offset]     = key1.vcw[offset]     = -(target0 - v0_0 + v1_0);
            key0.vcw[offset + 1] = key1.vcw[offset + 1] = -(target1 - v0_1 + v1_1);
        }

        s0 = (ct0[keep_dir] & notOneBlock) ^ (t0 ? scw : ZeroBlock);
        t0 = lsb(ct0[keep_dir]) ^ (t0 ? (keep_dir == 0 ? tcwL : tcwR) : 0);
        
        s1 = (ct1[keep_dir] & notOneBlock) ^ (t1 ? scw : ZeroBlock);
        t1 = lsb(ct1[keep_dir]) ^ (t1 ? (keep_dir == 0 ? tcwL : tcwR) : 0);
    }

    AES ak0_leaf(s0); AES ak1_leaf(s1);
    int B = 1 << w;
    GroupElement alpha_lo = idx & (B - 1); 

    for (int j = 0; j < B; ++j) {
        block pt_leaf = toBlock(0, j);
        block ct0_leaf = ak0_leaf.ecbEncBlock(pt_leaf);
        block ct1_leaf = ak1_leaf.ecbEncBlock(pt_leaf);
        
        GroupElement v0_0 = _mm_extract_epi64(ct0_leaf, 0);
        GroupElement v0_1 = _mm_extract_epi64(ct0_leaf, 1);
        GroupElement v1_0 = _mm_extract_epi64(ct1_leaf, 0);
        GroupElement v1_1 = _mm_extract_epi64(ct1_leaf, 1);

        GroupElement target0 = (j >= alpha_lo) ? beta[0] : 0;
        GroupElement target1 = (j >= alpha_lo) ? beta[1] : 0;

        int offset = j * 2;
        if (t0 == 1) {
            key0.leaf_vcw[offset]     = key1.leaf_vcw[offset]     = target0 - v0_0 + v1_0;
            key0.leaf_vcw[offset + 1] = key1.leaf_vcw[offset + 1] = target1 - v0_1 + v1_1;
        } else {
            key0.leaf_vcw[offset]     = key1.leaf_vcw[offset]     = -(target0 - v0_0 + v1_0);
            key0.leaf_vcw[offset + 1] = key1.leaf_vcw[offset + 1] = -(target1 - v0_1 + v1_1);
        }
    }

    return std::make_pair(key0, key1);
}

void evalGTDCF(int party, const GTDCFKeyPack &key, GroupElement x, GroupElement* res)
{
    static const block notOneBlock = toBlock(~0, ~1);
    int bin = key.bin, d = key.d;
    block s = key.seed;
    u8 t = party; 
    int64_t sign = (party == 0) ? 1 : -1;
    
    // 使用局部寄存器变量，避免内存读写
    GroupElement res0 = 0, res1 = 0;

    for (int i = 0; i < d; ++i) {
        const u8 x_i = static_cast<uint8_t>(x >> (bin - 1 - i)) & 1;
        AES ak(s);
        block s_next_raw;
        
        if (x_i == 1) {
            // 核心优化：只需要右分支(OneBlock)和掩码块(toBlock(0,2))
            block pt[2] = {OneBlock, toBlock(0, 2)};
            block ct[2];
            ak.ecbEncTwoBlocks(pt, ct); // 硬件加速，同时加密两块
            
            s_next_raw = ct[0]; 
            
            GroupElement v0 = _mm_extract_epi64(ct[1], 0);
            GroupElement v1 = _mm_extract_epi64(ct[1], 1);
            
            int offset = i * 2;
            if (t) {
                v0 += key.vcw[offset];
                v1 += key.vcw[offset + 1];
            }
            res0 += sign * v0;
            res1 += sign * v1;
        } else {
            // x_i == 0 极速模式：只需要左分支
            s_next_raw = ak.ecbEncBlock(ZeroBlock);
        }

        s = (s_next_raw & notOneBlock) ^ (t ? key.scw[i] : ZeroBlock);
        t = lsb(s_next_raw) ^ (t ? key.tcw[2 * i + x_i] : 0);
    }

    GroupElement x_lo = x & ((1 << key.w) - 1); 
    AES ak_leaf(s);
    block ct_leaf = ak_leaf.ecbEncBlock(toBlock(0, x_lo));
    
    GroupElement v_leaf0 = _mm_extract_epi64(ct_leaf, 0);
    GroupElement v_leaf1 = _mm_extract_epi64(ct_leaf, 1);

    int offset_leaf = x_lo * 2;
    if (t) {
        v_leaf0 += key.leaf_vcw[offset_leaf];
        v_leaf1 += key.leaf_vcw[offset_leaf + 1];
    }
    
    res[0] = res0 + sign * v_leaf0;
    res[1] = res1 + sign * v_leaf1;
}


// ==== 请添加到 dpf.cpp 的末尾 ====

std::pair<QuadDPFKeyPack, QuadDPFKeyPack> keyGenQuadDPF(int bin, int bout, GroupElement idx, GroupElement payload)
{
    static const osuCrypto::block notOneBlock = osuCrypto::toBlock(~0, ~1);
    const static osuCrypto::block pt[4] = {
        osuCrypto::toBlock(0, 0), osuCrypto::toBlock(0, 1), 
        osuCrypto::toBlock(0, 2), osuCrypto::toBlock(0, 3)
    };

    QuadDPFKeyPack key0(bin, bout);
    QuadDPFKeyPack key1(bin, bout);

    int tid = omp_get_thread_num();
    auto s = FSSConfig::prngs[tid].get<std::array<osuCrypto::block, 2>>();
    osuCrypto::block s0 = s[0] & notOneBlock;
    osuCrypto::block s1 = s[1] & notOneBlock;
    
    key0.s0_initial = s0;
    key1.s0_initial = s1;
    
    uint8_t t0 = 0;
    uint8_t t1 = 1;

    for (int i = 0; i < key0.depth; ++i)
    {
        // 提取当前的 2 bits 作为目标分支 keep。如果位数是奇数且是最后一次，则低位补0
        int shift = bin - 2 - 2 * i;
        uint8_t keep = 0;
        if (shift >= 0) {
            keep = (idx >> shift) & 3;
        } else {
            keep = ((idx << 1) & 3); 
        }

        osuCrypto::AES ak0(s0), ak1(s1);
        osuCrypto::block ct0[4], ct1[4];
        
        ak0.ecbEncFourBlocks(pt, ct0);
        ak1.ecbEncFourBlocks(pt, ct1);

        uint8_t t_cw_bits = 0;
        uint8_t r_idx = (keep + 1) % 4; // Sample r != Keep

        for (int c = 0; c < 4; ++c)
        {
            osuCrypto::block scw;
            uint8_t tc0 = lsb(ct0[c]);
            uint8_t tc1 = lsb(ct1[c]);
            uint8_t tcw;

            if (c != keep) {
                scw = (ct0[c] ^ ct1[c]) & notOneBlock;
                tcw = tc0 ^ tc1;
            } else {
                scw = (ct0[r_idx] ^ ct1[r_idx]) & notOneBlock; // 使用随机分支的值掩盖Keep分支
                tcw = tc0 ^ tc1 ^ 1;
            }

            key0.scw[i * 4 + c] = scw;
            key1.scw[i * 4 + c] = scw;
            t_cw_bits |= (tcw << c);
        }
        
        key0.tcw[i] = t_cw_bits;
        key1.tcw[i] = t_cw_bits;

        // 更新 s 和 t，沿着 keep 分支走下去
        osuCrypto::block scw_keep = key0.scw[i * 4 + keep];
        uint8_t tcw_keep = (t_cw_bits >> keep) & 1;

        s0 = (ct0[keep] & notOneBlock) ^ (t0 ? scw_keep : osuCrypto::ZeroBlock);
        t0 = lsb(ct0[keep]) ^ (t0 ? tcw_keep : 0);

        s1 = (ct1[keep] & notOneBlock) ^ (t1 ? scw_keep : osuCrypto::ZeroBlock);
        t1 = lsb(ct1[keep]) ^ (t1 ? tcw_keep : 0);
    }

    // 最后一个 Correction Word 计算 Payload (Convert 函数等效于提取 block 的一半)
    GroupElement conv_s0 = _mm_extract_epi64(s0, 0);
    GroupElement conv_s1 = _mm_extract_epi64(s1, 0);

    GroupElement cw_payload = payload - conv_s0 + conv_s1;
    if (t1 == 1) cw_payload = -cw_payload;

    key0.payload = cw_payload;
    key1.payload = cw_payload;

    return std::make_pair(key0, key1);
}

GroupElement evalQuadDPF(int party, const QuadDPFKeyPack &key, GroupElement x)
{
    static const osuCrypto::block notOneBlock = osuCrypto::toBlock(~0, ~1);
    osuCrypto::block s = key.s0_initial;
    uint8_t t = party;

    for (int i = 0; i < key.depth; ++i)
    {
        // 同样提取当前对应的 2 个 bit
        int shift = key.bin - 2 - 2 * i;
        uint8_t c = 0;
        if (shift >= 0) {
            c = (x >> shift) & 3;
        } else {
            c = ((x << 1) & 3); 
        }

        osuCrypto::AES ak(s);
        osuCrypto::block pt = osuCrypto::toBlock(0, c);
        
        // 核心加速点：我们只解密需要走的那一条分支！
        osuCrypto::block ct = ak.ecbEncBlock(pt);

        osuCrypto::block scw = key.scw[i * 4 + c];
        uint8_t tcw = (key.tcw[i] >> c) & 1;

        s = (ct & notOneBlock) ^ (t ? scw : osuCrypto::ZeroBlock);
        t = lsb(ct) ^ (t ? tcw : 0);
    }

    GroupElement conv_s = _mm_extract_epi64(s, 0);
    GroupElement res = conv_s + t * key.payload;
    
    // 如果是 Party 1 (Server1)，根据算法需要取反
    if (party == 1) {
        res = -res;
    }
    
    mod(res, key.bout);
    return res;
}

std::pair<OctDPFKeyPack, OctDPFKeyPack> keyGenOctDPF(int bin, int bout, GroupElement idx, GroupElement payload)
{
    static const osuCrypto::block notOneBlock = osuCrypto::toBlock(~0, ~1);
    const static osuCrypto::block pt[8] = {
        osuCrypto::toBlock(0, 0), osuCrypto::toBlock(0, 1), 
        osuCrypto::toBlock(0, 2), osuCrypto::toBlock(0, 3),
        osuCrypto::toBlock(0, 4), osuCrypto::toBlock(0, 5), 
        osuCrypto::toBlock(0, 6), osuCrypto::toBlock(0, 7)
    };

    OctDPFKeyPack key0(bin, bout);
    OctDPFKeyPack key1(bin, bout);

    int tid = omp_get_thread_num();
    auto s = FSSConfig::prngs[tid].get<std::array<osuCrypto::block, 2>>();
    osuCrypto::block s0 = s[0] & notOneBlock;
    osuCrypto::block s1 = s[1] & notOneBlock;
    
    key0.s0_initial = s0;
    key1.s0_initial = s1;
    
    uint8_t t0 = 0;
    uint8_t t1 = 1;

    for (int i = 0; i < key0.depth; ++i)
    {
        // 计算当前3位的偏移量。若无法整除，最后几位会左移补零，严格对应伪代码中的 ||00 和 ||0
        int shift = bin - 3 - 3 * i;
        uint8_t keep = 0;
        if (shift >= 0) {
            keep = (idx >> shift) & 7;
        } else if (shift == -1) {
            keep = ((idx << 1) & 7); 
        } else if (shift == -2) {
            keep = ((idx << 2) & 7);
        }

        osuCrypto::AES ak0(s0), ak1(s1);
        osuCrypto::block ct0[8], ct1[8];
        
        // 生成 8 个分支
        ak0.ecbEncBlocks(pt, 8, ct0);
        ak1.ecbEncBlocks(pt, 8, ct1);

        uint8_t t_cw_bits = 0;
        uint8_t r_idx = (keep + 1) % 8; // 随机找一个非Keep的兄弟节点掩盖CW

        for (int c = 0; c < 8; ++c)
        {
            osuCrypto::block scw;
            uint8_t tc0 = lsb(ct0[c]);
            uint8_t tc1 = lsb(ct1[c]);
            uint8_t tcw;

            if (c != keep) {
                scw = (ct0[c] ^ ct1[c]) & notOneBlock;
                tcw = tc0 ^ tc1;
            } else {
                scw = (ct0[r_idx] ^ ct1[r_idx]) & notOneBlock;
                tcw = tc0 ^ tc1 ^ 1;
            }

            key0.scw[i * 8 + c] = scw;
            key1.scw[i * 8 + c] = scw;
            t_cw_bits |= (tcw << c);
        }
        
        key0.tcw[i] = t_cw_bits;
        key1.tcw[i] = t_cw_bits;

        osuCrypto::block scw_keep = key0.scw[i * 8 + keep];
        uint8_t tcw_keep = (t_cw_bits >> keep) & 1;

        s0 = (ct0[keep] & notOneBlock) ^ (t0 ? scw_keep : osuCrypto::ZeroBlock);
        t0 = lsb(ct0[keep]) ^ (t0 ? tcw_keep : 0);

        s1 = (ct1[keep] & notOneBlock) ^ (t1 ? scw_keep : osuCrypto::ZeroBlock);
        t1 = lsb(ct1[keep]) ^ (t1 ? tcw_keep : 0);
    }

    GroupElement conv_s0 = _mm_extract_epi64(s0, 0);
    GroupElement conv_s1 = _mm_extract_epi64(s1, 0);

    GroupElement cw_payload = payload - conv_s0 + conv_s1;
    if (t1 == 1) cw_payload = -cw_payload;

    key0.payload = cw_payload;
    key1.payload = cw_payload;

    return std::make_pair(key0, key1);
}

GroupElement evalOctDPF(int party, const OctDPFKeyPack &key, GroupElement x)
{
    static const osuCrypto::block notOneBlock = osuCrypto::toBlock(~0, ~1);
    osuCrypto::block s = key.s0_initial;
    uint8_t t = party;

    for (int i = 0; i < key.depth; ++i)
    {
        int shift = key.bin - 3 - 3 * i;
        uint8_t c = 0;
        if (shift >= 0) {
            c = (x >> shift) & 7;
        } else if (shift == -1) {
            c = ((x << 1) & 7); 
        } else if (shift == -2) {
            c = ((x << 2) & 7);
        }

        osuCrypto::AES ak(s);
        osuCrypto::block pt = osuCrypto::toBlock(0, c);
        
        // 在线执行极速：只解密目标子树这1个 Block！
        osuCrypto::block ct = ak.ecbEncBlock(pt);

        osuCrypto::block scw = key.scw[i * 8 + c];
        uint8_t tcw = (key.tcw[i] >> c) & 1;

        s = (ct & notOneBlock) ^ (t ? scw : osuCrypto::ZeroBlock);
        t = lsb(ct) ^ (t ? tcw : 0);
    }

    GroupElement conv_s = _mm_extract_epi64(s, 0);
    GroupElement res = conv_s + t * key.payload;
    
    if (party == 1) res = -res;
    
    mod(res, key.bout);
    return res;
}


// VDPF
static inline uint8_t verdpf_lsb(const block &b)
{
    return _mm_cvtsi128_si64x(b) & 1;
}

static inline bool verdpf_block_equal(const block &a, const block &b)
{
    return (_mm_extract_epi64(a, 0) == _mm_extract_epi64(b, 0)) &&
           (_mm_extract_epi64(a, 1) == _mm_extract_epi64(b, 1));
}

static inline block verdpf_mmo_hash_block(const block &key, const block &msg)
{
    AES aes(key);
    return aes.ecbEncBlock(msg) ^ msg;
}

// Public hash H : {0,1}^{n+λ} -> {0,1}^{4λ}.
// Engineering instantiation by domain-separated AES-MMO.
// For a proof-level implementation, replace this with SHA-512/BLAKE3/RandomOracle.
static inline void ver_hash_H4(GroupElement x, const block &s, block out[4])
{
    const block K[4] = {
        toBlock(0x5644504648310001ULL, 0x1111111111111111ULL),
        toBlock(0x5644504648310002ULL, 0x2222222222222222ULL),
        toBlock(0x5644504648310003ULL, 0x3333333333333333ULL),
        toBlock(0x5644504648310004ULL, 0x4444444444444444ULL)
    };

    for (int i = 0; i < 4; ++i) {
        block m = s ^ toBlock(x, 0xA500000000000000ULL ^ (uint64_t)i);
        out[i] = verdpf_mmo_hash_block(K[i], m);
    }
}

// Deterministic final leaf bit derived from seed.
// This replaces paper's LSB(s) because the existing DPF code clears LSB(s).
static inline uint8_t ver_leaf_bit(const block &s)
{
    block h = verdpf_mmo_hash_block(
        toBlock(0x564450464C454146ULL, 0x7777777777777777ULL),
        s ^ toBlock(0, 0xBEEFBEEFBEEFBEEFULL)
    );
    return verdpf_lsb(h);
}

// H' : proof-state + 4λ corrected value -> 2λ.
// The paper writes pi <- pi XOR H'(pi XOR corrected).
// In this implementation pi is a 2-block state and corrected is 4 blocks.
// We feed both into a public compression hash.
static inline void ver_hash_Hprime2(const block pi[2], const block corrected[4], block out[2])
{
    const block K0 = toBlock(0x5644504648500001ULL, 0x9999999999999999ULL);
    const block K1 = toBlock(0x5644504648500002ULL, 0xAAAAAAAAAAAAAAAAULL);

    block m0 = pi[0] ^ corrected[0] ^ corrected[1] ^ toBlock(0, 0x12345678);
    block m1 = pi[1] ^ corrected[2] ^ corrected[3] ^ toBlock(0, 0x87654321);

    out[0] = verdpf_mmo_hash_block(K0, m0);
    out[1] = verdpf_mmo_hash_block(K1, m1);
}

static inline void ver_proof_init(const block cs[4], block pi[2])
{
    block zero[2] = {ZeroBlock, ZeroBlock};
    ver_hash_Hprime2(zero, cs, pi);
}

static inline void ver_proof_update(block pi[2], const block corrected[4])
{
    block hp[2];
    ver_hash_Hprime2(pi, corrected, hp);
    pi[0] = pi[0] ^ hp[0];
    pi[1] = pi[1] ^ hp[1];
}

static inline GroupElement verdpf_convert(const block &s)
{
    return static_cast<GroupElement>(_mm_extract_epi64(s, 0));
}

std::pair<VerDPFKeyPack, VerDPFKeyPack>
keyGenVerDPF(int bin, int bout, GroupElement idx, GroupElement payload)
{
    always_assert(bin <= 64);
    always_assert(bout <= 64);

    static const block notOneBlock = toBlock(~0, ~1);
    const static block pt[2] = {ZeroBlock, OneBlock};

    VerDPFKeyPack key0(bin, bout);
    VerDPFKeyPack key1(bin, bout);

    bool success = false;

    while (!success)
    {
        key0.tcw[0] = key0.tcw[1] = 0;
        key1.tcw[0] = key1.tcw[1] = 0;

        for (int i = 0; i < 4; ++i) {
            key0.cs[i] = ZeroBlock;
            key1.cs[i] = ZeroBlock;
        }

        int tid = omp_get_thread_num();
        auto ss = FSSConfig::prngs[tid].get<std::array<block, 2>>();

        block s0 = ss[0] & notOneBlock;
        block s1 = ss[1] & notOneBlock;

        key0.s[0] = s0;
        key1.s[0] = s1;

        uint8_t t0 = 0;
        uint8_t t1 = 1;

        block ct0[2], ct1[2];

        // Standard DPF GGM expansion for n levels.
        for (int i = 0; i < bin; ++i)
        {
            const uint8_t keep = static_cast<uint8_t>(idx >> (bin - 1 - i)) & 1;
            const uint8_t loose = keep ^ 1;

            AES ak0(s0);
            AES ak1(s1);

            ak0.ecbEncTwoBlocks(pt, ct0);
            ak1.ecbEncTwoBlocks(pt, ct1);

            block scw = (ct0[loose] ^ ct1[loose]) & notOneBlock;

            uint64_t tLcw = verdpf_lsb(ct0[0]) ^ verdpf_lsb(ct1[0]) ^ keep ^ 1;
            uint64_t tRcw = verdpf_lsb(ct0[1]) ^ verdpf_lsb(ct1[1]) ^ keep;

            key0.s[i + 1] = scw;
            key1.s[i + 1] = scw;

            key0.tcw[0] |= (tLcw << (bin - 1 - i));
            key0.tcw[1] |= (tRcw << (bin - 1 - i));

            const uint8_t tcw_keep = keep == 0 ? tLcw : tRcw;

            s0 = (ct0[keep] & notOneBlock) ^ (t0 ? scw : ZeroBlock);
            t0 = verdpf_lsb(ct0[keep]) ^ (t0 ? tcw_keep : 0);

            s1 = (ct1[keep] & notOneBlock) ^ (t1 ? scw : ZeroBlock);
            t1 = verdpf_lsb(ct1[keep]) ^ (t1 ? tcw_keep : 0);
        }

        key1.tcw[0] = key0.tcw[0];
        key1.tcw[1] = key0.tcw[1];

        // Final verifiable hash level:
        //   cs = H(alpha || s0_n) XOR H(alpha || s1_n)
        block h0[4], h1[4];
        ver_hash_H4(idx, s0, h0);
        ver_hash_H4(idx, s1, h1);

        for (int i = 0; i < 4; ++i) {
            key0.cs[i] = h0[i] ^ h1[i];
            key1.cs[i] = key0.cs[i];
        }

        // Final deterministic control bit derived from leaf seed.
        uint8_t t0_leaf = ver_leaf_bit(s0);
        uint8_t t1_leaf = ver_leaf_bit(s1);

        // Rejection sampling, expected 2 trials.
        if (t0_leaf != t1_leaf)
        {
            success = true;

            GroupElement g0 = verdpf_convert(s0);
            GroupElement g1 = verdpf_convert(s1);

            GroupElement ocw = payload - g0 + g1;
            mod(ocw, bout);

            if (t1_leaf == 1) {
                ocw = -ocw;
                mod(ocw, bout);
            }

            key0.ocw = ocw;
            key1.ocw = ocw;
        }
    }

    return std::make_pair(key0, key1);
}

GroupElement evalVerDPF(int party,
                        const VerDPFKeyPack &key,
                        GroupElement x)
{
    std::vector<GroupElement> xs(1, x);
    std::vector<GroupElement> ys(1);
    block pi[2];
    evalVerDPF_Batch(party, key, xs, ys, pi);
    return ys[0];
}

void evalVerDPF_Batch(int party,
                      const VerDPFKeyPack &key,
                      const std::vector<GroupElement> &x_vec,
                      std::vector<GroupElement> &y_vec,
                      block pi_out[2])
{
    always_assert(party == 0 || party == 1);

    static const block notOneBlock = toBlock(~0, ~1);
    const static block pt[2] = {ZeroBlock, OneBlock};

    y_vec.resize(x_vec.size());

    // Proof initialization. Paper writes pi <- cs, but the final proof is 2λ.
    // We compress cs into a 2-block proof state.
    ver_proof_init(key.cs, pi_out);

    for (size_t l = 0; l < x_vec.size(); ++l)
    {
        GroupElement x = x_vec[l];

        block s = key.s[0];
        uint8_t t = static_cast<uint8_t>(party);

        // Follow the path for x.
        for (int i = 0; i < key.bin; ++i)
        {
            uint8_t x_i = static_cast<uint8_t>(x >> (key.bin - 1 - i)) & 1;

            AES ak(s);
            block ct = ak.ecbEncBlock(pt[x_i]);

            s = (ct & notOneBlock) ^ (t ? key.s[i + 1] : ZeroBlock);
            t = verdpf_lsb(ct) ^ (t ? ((key.tcw[x_i] >> (key.bin - 1 - i)) & 1) : 0);
        }

        uint8_t t_leaf = ver_leaf_bit(s);

        // Output:
        //   y_b = (-1)^b * correct_G(convert(s), ocw, t_leaf)
        GroupElement y = verdpf_convert(s);
        if (t_leaf) {
            y += key.ocw;
        }

        if (party == 1) {
            y = -y;
        }

        mod(y, key.bout);
        y_vec[l] = y;

        // Proof:
        //   pi_tilde = H(x || s)
        //   corrected = pi_tilde XOR (t_leaf ? cs : 0)
        //   pi = pi XOR H'(pi, corrected)
        block pi_tilde[4];
        ver_hash_H4(x, s, pi_tilde);

        block corrected[4];
        for (int i = 0; i < 4; ++i) {
            corrected[i] = pi_tilde[i] ^ (t_leaf ? key.cs[i] : ZeroBlock);
        }

        ver_proof_update(pi_out, corrected);
    }
}

bool verifyVerDPF(const block pi0[2], const block pi1[2])
{
    return verdpf_block_equal(pi0[0], pi1[0]) &&
           verdpf_block_equal(pi0[1], pi1[1]);
}



// ============================================================
// IFSS common arithmetic MAC helpers
// ============================================================

static inline uint64_t ifss_next_u64(uint64_t &state)
{
    // xorshift64*
    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;
    return state * 2685821657736338717ULL;
}

IFSSGlobalMACKey ifss_setup_arithmetic_mac(int bout)
{
    IFSSGlobalMACKey mac;

    mac.deltaA0 = random_ge(bout);
    mac.deltaA1 = random_ge(bout);

    mac.deltaA = mac.deltaA0 + mac.deltaA1;
    mod(mac.deltaA, bout);

    // Avoid zero MAC key in tests.
    if (mac.deltaA == 0)
    {
        mac.deltaA1 += 1;
        mod(mac.deltaA1, bout);

        mac.deltaA = mac.deltaA0 + mac.deltaA1;
        mod(mac.deltaA, bout);
    }

    return mac;
}

GroupElement ifss_reconstruct_value(IFSSAuthShare s0,
                                    IFSSAuthShare s1,
                                    int bout)
{
    GroupElement y = s0.value + s1.value;
    mod(y, bout);
    return y;
}

bool ifss_check_mac_single(IFSSAuthShare s0,
                           IFSSAuthShare s1,
                           GroupElement deltaA0,
                           GroupElement deltaA1,
                           int bout)
{
    GroupElement y = ifss_reconstruct_value(s0, s1, bout);

    GroupElement z0 = s0.tag - y * deltaA0;
    GroupElement z1 = s1.tag - y * deltaA1;

    mod(z0, bout);
    mod(z1, bout);

    GroupElement z = z0 + z1;
    mod(z, bout);

    return z == 0;
}

bool ifss_batch_check_arithmetic(const std::vector<IFSSAuthShare> &shares0,
                                 const std::vector<IFSSAuthShare> &shares1,
                                 GroupElement deltaA0,
                                 GroupElement deltaA1,
                                 int bout,
                                 uint64_t seed)
{
    always_assert(shares0.size() == shares1.size());

    uint64_t state = seed;
    GroupElement acc0 = 0;
    GroupElement acc1 = 0;

    for (size_t i = 0; i < shares0.size(); ++i)
    {
        GroupElement y = ifss_reconstruct_value(shares0[i], shares1[i], bout);

        GroupElement z0 = shares0[i].tag - y * deltaA0;
        GroupElement z1 = shares1[i].tag - y * deltaA1;

        mod(z0, bout);
        mod(z1, bout);

        GroupElement chi = ifss_next_u64(state);
        mod(chi, bout);

        // Avoid zero coefficient in tests.
        if (chi == 0)
        {
            chi = 1;
        }

        acc0 += chi * z0;
        acc1 += chi * z1;

        mod(acc0, bout);
        mod(acc1, bout);
    }

    GroupElement acc = acc0 + acc1;
    mod(acc, bout);

    return acc == 0;
}

// ============================================================
// DPF payload evaluation
//
// This is different from evalDPF_EQ.
// evalDPF_EQ only returns the equality bit share.
// evalDPF_Payload returns the arithmetic payload share:
//   beta * {x = alpha}
// ============================================================

GroupElement evalDPF_Payload(int party,
                             DPFKeyPack &key,
                             GroupElement x)
{
    static const block notOneBlock = toBlock(~0, ~1);

    int bin = key.bin;
    int bout = key.bout;

    mod(x, bin);

    auto s = _mm_loadu_si128(key.s);
    u8 t = party;

    for (int i = 0; i < bin; ++i)
    {
        assert(lsb(s) == 0);

        const u8 x_i =
            static_cast<uint8_t>(x >> (bin - 1 - i)) & 1;

        AES ak(s);
        block ct = ak.ecbEncBlock(toBlock(0, x_i));

        s = ct & notOneBlock;

        u8 t_old = t;
        t = lsb(ct);

        if (t_old)
        {
            s = s ^ _mm_loadu_si128(key.s + i + 1);
            t = t ^ ((key.tcw[x_i] >> (bin - 1 - i)) & 1);
        }
    }

    GroupElement out =
        _mm_extract_epi64(s, 0) + key.payload * static_cast<GroupElement>(t);

    if (party == 1)
    {
        out = -out;
    }

    mod(out, bout);
    return out;
}

// ============================================================
// IFSS_DPF
//
// Since current DPF supports only one payload,
// IFSS_DPF uses two DPF instances:
//
//   valKey = DPF(alpha, beta)
//   macKey = DPF(alpha, deltaA * beta)
//
// Eval returns:
//   value share = beta * {x = alpha}
//   tag share   = deltaA * beta * {x = alpha}
// ============================================================

std::pair<IFSS_DPFKeyPack, IFSS_DPFKeyPack>
keyGenIFSS_DPF(int bin,
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

    auto valKeys = keyGenDPF(bin, bout, alpha, beta);
    auto macKeys = keyGenDPF(bin, bout, alpha, macBeta);

    IFSS_DPFKeyPack k0;
    IFSS_DPFKeyPack k1;

    k0.bin = k1.bin = bin;
    k0.bout = k1.bout = bout;

    k0.valKey = valKeys.first;
    k1.valKey = valKeys.second;

    k0.macKey = macKeys.first;
    k1.macKey = macKeys.second;

    return std::make_pair(k0, k1);
}

IFSSAuthShare evalIFSS_DPF(int party,
                           IFSS_DPFKeyPack &key,
                           GroupElement x)
{
    IFSSAuthShare out;

    out.value = evalDPF_Payload(party, key.valKey, x);
    out.tag = evalDPF_Payload(party, key.macKey, x);

    mod(out.value, key.bout);
    mod(out.tag, key.bout);

    return out;
}


// ============================================================================
// Half-Tree DPF implementation
// Put this block in FSS/dpf.cpp
// ============================================================================

#include <FSS/dpf.h>
#include <FSS/assert.h>
#include <FSS/config.h>
#include <omp.h>
#include <vector>

using namespace osuCrypto;

namespace {

inline uint8_t htdpf_lsb(const block &b)
{
    return static_cast<uint8_t>(_mm_cvtsi128_si64x(b) & 1);
}

inline block htdpf_bit_block(uint8_t b)
{
    return b ? OneBlock : ZeroBlock;
}

inline block htdpf_clear_lsb(const block &b)
{
    static const block notOneBlock = toBlock(~0ULL, ~1ULL);
    return b & notOneBlock;
}

inline block htdpf_set_lsb(const block &b, uint8_t bit)
{
    return htdpf_clear_lsb(b) ^ htdpf_bit_block(bit & 1);
}

inline block htdpf_random_block()
{
    int tid = omp_get_thread_num();
    return FSSConfig::prngs[tid].get<block>();
}

// No AES-object reuse version.
// Each hash call constructs AES(hashKey) locally.
inline block htdpf_hash(const block &hashKey, const block &x)
{
    AES aes(hashKey);
    block y = aes.ecbEncBlock(x);
    return y ^ x;
}

inline uint8_t htdpf_get_bit(GroupElement x, int bin, int level)
{
    return static_cast<uint8_t>((x >> (bin - 1 - level)) & 1ULL);
}

inline GroupElement htdpf_convert(const block &s, int bout)
{
    GroupElement y = static_cast<GroupElement>(_mm_extract_epi64(htdpf_clear_lsb(s), 0));
    mod(y, bout);
    return y;
}

inline GroupElement htdpf_neg(GroupElement x, int bout)
{
    GroupElement y = 0 - x;
    mod(y, bout);
    return y;
}

inline GroupElement htdpf_add(GroupElement a, GroupElement b, int bout)
{
    GroupElement y = a + b;
    mod(y, bout);
    return y;
}

inline GroupElement htdpf_sub(GroupElement a, GroupElement b, int bout)
{
    GroupElement y = a - b;
    mod(y, bout);
    return y;
}

inline GroupElement htdpf_mul_sign(int sign, GroupElement x, int bout)
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

} // namespace

std::pair<HalfTreeDPFKeyPack, HalfTreeDPFKeyPack>
keyGenHalfTreeDPFWithPath(int bin,
                          int bout,
                          GroupElement alpha,
                          GroupElement beta,
                          std::vector<block> *path0,
                          std::vector<block> *path1)
{
    always_assert(bin >= 1 && bin <= 64);
    always_assert(bout >= 1 && bout <= 64);

    mod(alpha, bin);
    mod(beta, bout);

    HalfTreeDPFKeyPack k0(bin, bout);
    HalfTreeDPFKeyPack k1(bin, bout);

    block hashKey = htdpf_random_block();
    k0.hashKey = hashKey;
    k1.hashKey = hashKey;

    // Root shares satisfy root0 xor root1 = Delta and lsb(Delta)=1.
    block delta = htdpf_set_lsb(htdpf_random_block(), 1);
    block root0 = htdpf_random_block();
    block root1 = root0 ^ delta;

    k0.root = root0;
    k1.root = root1;

    block cur0 = root0;
    block cur1 = root1;

    if (path0) path0->clear();
    if (path1) path1->clear();

    // Internal levels: 1,...,n-1.
    // child_b = H(parent) xor b * parent xor t_parent * CW_i.
    for (int level = 0; level < bin - 1; ++level)
    {
        if (path0) path0->push_back(cur0);
        if (path1) path1->push_back(cur1);

        uint8_t alphaBit = htdpf_get_bit(alpha, bin, level);

        block h0 = htdpf_hash(hashKey, cur0);
        block h1 = htdpf_hash(hashKey, cur1);

        // Make alpha-side child reconstruct to Delta and off-path child reconstruct to 0.
        block cwi = h0 ^ h1 ^ (alphaBit ? delta : ZeroBlock) ^ delta;

        k0.cw[level] = cwi;
        k1.cw[level] = cwi;

        block next0 = h0 ^ (alphaBit ? cur0 : ZeroBlock) ^ (htdpf_lsb(cur0) ? cwi : ZeroBlock);
        block next1 = h1 ^ (alphaBit ? cur1 : ZeroBlock) ^ (htdpf_lsb(cur1) ? cwi : ZeroBlock);

        cur0 = next0;
        cur1 = next1;
    }

    // Store X_{n-1} shares for the last DPF level and DCF VCW generation.
    if (path0) path0->push_back(cur0);
    if (path1) path1->push_back(cur1);

    uint8_t alphaLast = htdpf_get_bit(alpha, bin, bin - 1);
    uint8_t looseLast = alphaLast ^ 1;

    block h0[2];
    block h1[2];

    h0[0] = htdpf_hash(hashKey, cur0 ^ htdpf_bit_block(0));
    h0[1] = htdpf_hash(hashKey, cur0 ^ htdpf_bit_block(1));
    h1[0] = htdpf_hash(hashKey, cur1 ^ htdpf_bit_block(0));
    h1[1] = htdpf_hash(hashKey, cur1 ^ htdpf_bit_block(1));

    // Compressed final correction:
    // Use loose side high bits so that the off-path final leaf reconstructs to 0.
    block looseDiff = h0[looseLast] ^ h1[looseLast];
    k0.hcw = htdpf_clear_lsb(looseDiff);
    k1.hcw = k0.hcw;

    // Off-path low bit becomes 0; on-path low bit becomes 1.
    uint8_t lcwLoose = htdpf_lsb(looseDiff);
    block keepDiff = h0[alphaLast] ^ h1[alphaLast];
    uint8_t lcwKeep = htdpf_lsb(keepDiff) ^ 1;

    uint8_t lcw0 = (looseLast == 0) ? lcwLoose : lcwKeep;
    uint8_t lcw1 = (looseLast == 1) ? lcwLoose : lcwKeep;

    k0.lcw = static_cast<uint8_t>((lcw0 & 1) | ((lcw1 & 1) << 1));
    k1.lcw = k0.lcw;

    uint8_t keepLCW = static_cast<uint8_t>((k0.lcw >> alphaLast) & 1);
    block finalCWKeep = k0.hcw ^ htdpf_bit_block(keepLCW);

    block leaf0 = h0[alphaLast] ^ (htdpf_lsb(cur0) ? finalCWKeep : ZeroBlock);
    block leaf1 = h1[alphaLast] ^ (htdpf_lsb(cur1) ? finalCWKeep : ZeroBlock);

    uint8_t t0 = htdpf_lsb(leaf0);
    uint8_t t1 = htdpf_lsb(leaf1);

    always_assert((t0 ^ t1) == 1);

    GroupElement conv0 = htdpf_convert(leaf0, bout);
    GroupElement conv1 = htdpf_convert(leaf1, bout);

    // outCW = (t0 - t1) * (Convert(s1) - Convert(s0) + beta)
    GroupElement inner = htdpf_add(htdpf_sub(conv1, conv0, bout), beta, bout);
    int sign = static_cast<int>(t0) - static_cast<int>(t1);

    k0.outCW = htdpf_mul_sign(sign, inner, bout);
    k1.outCW = k0.outCW;

    return std::make_pair(k0, k1);
}

std::pair<HalfTreeDPFKeyPack, HalfTreeDPFKeyPack>
keyGenHalfTreeDPF(int bin, int bout, GroupElement alpha, GroupElement beta)
{
    return keyGenHalfTreeDPFWithPath(bin, bout, alpha, beta, nullptr, nullptr);
}

GroupElement evalHalfTreeDPF(int party,
                             const HalfTreeDPFKeyPack &key,
                             GroupElement x,
                             HalfTreeDPFEvalTrace *trace)
{
    always_assert(party == 0 || party == 1);
    always_assert(key.bin >= 1 && key.bin <= 64);

    mod(x, key.bin);

    block cur = key.root;

    if (trace) {
        trace->parent.clear();
        trace->parent.reserve(key.bin);
    }

    for (int level = 0; level < key.bin - 1; ++level)
    {
        if (trace) trace->parent.push_back(cur);

        uint8_t xBit = htdpf_get_bit(x, key.bin, level);

        block h = htdpf_hash(key.hashKey, cur);
        block corr = htdpf_lsb(cur) ? key.cw[level] : ZeroBlock;

        cur = h ^ (xBit ? cur : ZeroBlock) ^ corr;
    }

    if (trace) trace->parent.push_back(cur);

    uint8_t xLast = htdpf_get_bit(x, key.bin, key.bin - 1);

    block hLeaf = htdpf_hash(key.hashKey, cur ^ htdpf_bit_block(xLast));

    uint8_t lcw = static_cast<uint8_t>((key.lcw >> xLast) & 1);
    block finalCW = key.hcw ^ htdpf_bit_block(lcw);

    block leaf = hLeaf ^ (htdpf_lsb(cur) ? finalCW : ZeroBlock);

    if (trace) trace->leaf = leaf;

    GroupElement y = htdpf_convert(leaf, key.bout);

    if (htdpf_lsb(leaf)) {
        y = htdpf_add(y, key.outCW, key.bout);
    }

    if (party == 1) {
        y = htdpf_neg(y, key.bout);
    }

    mod(y, key.bout);
    return y;
}

void evalAllHalfTreeDPF(int party,
                        const HalfTreeDPFKeyPack &key,
                        GroupElement *out)
{
    always_assert(party == 0 || party == 1);
    always_assert(key.bin >= 1 && key.bin <= 30);

    const uint64_t N = 1ULL << key.bin;

    std::vector<block> level;
    std::vector<block> next;

    level.reserve(N);
    next.reserve(N);
    level.push_back(key.root);

    for (int d = 0; d < key.bin - 1; ++d)
    {
        next.resize(level.size() * 2);

        for (size_t j = 0; j < level.size(); ++j)
        {
            block parent = level[j];
            block h = htdpf_hash(key.hashKey, parent);
            block corr = htdpf_lsb(parent) ? key.cw[d] : ZeroBlock;

            next[2 * j] = h ^ corr;
            next[2 * j + 1] = h ^ parent ^ corr;
        }

        level.swap(next);
    }

    for (size_t j = 0; j < level.size(); ++j)
    {
        block parent = level[j];

        for (int b = 0; b < 2; ++b)
        {
            block hLeaf = htdpf_hash(key.hashKey, parent ^ htdpf_bit_block(static_cast<uint8_t>(b)));

            uint8_t lcw = static_cast<uint8_t>((key.lcw >> b) & 1);
            block finalCW = key.hcw ^ htdpf_bit_block(lcw);

            block leaf = hLeaf ^ (htdpf_lsb(parent) ? finalCW : ZeroBlock);

            GroupElement y = htdpf_convert(leaf, key.bout);

            if (htdpf_lsb(leaf)) {
                y = htdpf_add(y, key.outCW, key.bout);
            }

            if (party == 1) {
                y = htdpf_neg(y, key.bout);
            }

            mod(y, key.bout);
            out[2 * j + b] = y;
        }
    }
}

GroupElement reconstructHalfTreeDPF(const HalfTreeDPFKeyPack &k0,
                                    const HalfTreeDPFKeyPack &k1,
                                    GroupElement x)
{
    GroupElement y0 = evalHalfTreeDPF(0, k0, x);
    GroupElement y1 = evalHalfTreeDPF(1, k1, x);

    GroupElement y = y0 + y1;
    mod(y, k0.bout);

    return y;
}




// ======================================================
// VIDPF
// ======================================================
namespace
{

static inline uint8_t vidpf_lsb(
    const osuCrypto::block &block)
{
    return static_cast<uint8_t>(
        _mm_cvtsi128_si64x(block) & 1);
}

static inline bool vidpf_block_equal(
    const osuCrypto::block &a,
    const osuCrypto::block &b)
{
    return
        (_mm_extract_epi64(a, 0) ==
         _mm_extract_epi64(b, 0))
        &&
        (_mm_extract_epi64(a, 1) ==
         _mm_extract_epi64(b, 1));
}

static inline WideGroupElement vidpf_block_to_wide(
    const osuCrypto::block &block,
    int ringBw)
{
    WideGroupElement value(
        static_cast<uint64_t>(
            _mm_extract_epi64(block, 0)),
        static_cast<uint64_t>(
            _mm_extract_epi64(block, 1)));

    wideMod(value, ringBw);
    return value;
}

static inline uint8_t vidpf_get_full_bit(
    GroupElement value,
    int bin,
    int index)
{
    return static_cast<uint8_t>(
        (value >> (bin - 1 - index)) & 1ULL);
}

static inline uint8_t vidpf_get_prefix_bit(
    GroupElement prefix,
    int length,
    int index)
{
    return static_cast<uint8_t>(
        (prefix >> (length - 1 - index)) & 1ULL);
}

static inline GroupElement vidpf_prefix_of(
    GroupElement alpha,
    int bin,
    int length)
{
    if (length == 64)
    {
        return alpha;
    }

    const GroupElement mask =
        (GroupElement(1) << length) - 1;

    return
        (alpha >> (bin - length)) &
        mask;
}

static inline osuCrypto::block vidpf_mmo_hash(
    const osuCrypto::block &key,
    const osuCrypto::block &message)
{
    osuCrypto::AES aes(key);

    return
        aes.ecbEncBlock(message) ^
        message;
}

static inline VIDPFHash4 vidpf_Hi(
    int level,
    const osuCrypto::block &seedTilde,
    uint8_t controlBit,
    GroupElement prefix)
{
    using namespace osuCrypto;

    static const block hashKeys[4] =
    {
        toBlock(
            0x5649445046483100ULL,
            0x1111111111111111ULL),

        toBlock(
            0x5649445046483200ULL,
            0x2222222222222222ULL),

        toBlock(
            0x5649445046483300ULL,
            0x3333333333333333ULL),

        toBlock(
            0x5649445046483400ULL,
            0x4444444444444444ULL)
    };

    VIDPFHash4 output;

    for (int i = 0; i < 4; ++i)
    {
        const block domain =
            toBlock(
                (static_cast<uint64_t>(level) << 32) ^
                (static_cast<uint64_t>(controlBit) << 16) ^
                static_cast<uint64_t>(i),

                static_cast<uint64_t>(prefix));

        output.h[i] =
            vidpf_mmo_hash(
                hashKeys[i],
                seedTilde ^ domain);
    }

    return output;
}

static inline VIDPFToken vidpf_HPrime(
    const VIDPFHash4 &input)
{
    using namespace osuCrypto;

    static const block hashKeys[2] =
    {
        toBlock(
            0x5649445048503100ULL,
            0x5555555555555555ULL),

        toBlock(
            0x5649445048503200ULL,
            0x6666666666666666ULL)
    };

    VIDPFToken output;

    const block message0 =
        input.h[0] ^
        input.h[2] ^
        toBlock(0xA0, 0xA1);

    const block message1 =
        input.h[1] ^
        input.h[3] ^
        toBlock(0xB0, 0xB1);

    output.v[0] =
        vidpf_mmo_hash(
            hashKeys[0],
            message0);

    output.v[1] =
        vidpf_mmo_hash(
            hashKeys[1],
            message1);

    return output;
}

static inline void vidpf_hash4_xor_inplace(
    VIDPFHash4 &a,
    const VIDPFHash4 &b)
{
    for (int i = 0; i < 4; ++i)
    {
        a.h[i] ^= b.h[i];
    }
}

static inline VIDPFHash4 vidpf_hash4_xor(
    const VIDPFHash4 &a,
    const VIDPFHash4 &b)
{
    VIDPFHash4 output;

    for (int i = 0; i < 4; ++i)
    {
        output.h[i] =
            a.h[i] ^ b.h[i];
    }

    return output;
}

static inline void vidpf_hash4_xor_token_inplace(
    VIDPFHash4 &a,
    const VIDPFToken &b)
{
    a.h[0] ^= b.v[0];
    a.h[1] ^= b.v[1];
}

static inline void vidpf_token_xor_inplace(
    VIDPFToken &a,
    const VIDPFToken &b)
{
    a.v[0] ^= b.v[0];
    a.v[1] ^= b.v[1];
}

static inline VIDPFPayload vidpf_payload_add(
    const VIDPFPayload &a,
    const VIDPFPayload &b,
    int ringBw)
{
    VIDPFPayload output;

    output.value =
        wideAdd(
            a.value,
            b.value,
            ringBw);

    output.mac =
        wideAdd(
            a.mac,
            b.mac,
            ringBw);

    return output;
}

static inline VIDPFPayload vidpf_payload_sub(
    const VIDPFPayload &a,
    const VIDPFPayload &b,
    int ringBw)
{
    VIDPFPayload output;

    output.value =
        wideSub(
            a.value,
            b.value,
            ringBw);

    output.mac =
        wideSub(
            a.mac,
            b.mac,
            ringBw);

    return output;
}

static inline VIDPFPayload vidpf_payload_neg(
    const VIDPFPayload &input,
    int ringBw)
{
    VIDPFPayload output;

    output.value =
        wideNeg(
            input.value,
            ringBw);

    output.mac =
        wideNeg(
            input.mac,
            ringBw);

    return output;
}

// G : seed -> left/right seed and control bits.
static inline void vidpf_expand_tree(
    const osuCrypto::block &seed,
    osuCrypto::block &leftRaw,
    osuCrypto::block &rightRaw)
{
    using namespace osuCrypto;

    static const block plaintext[2] =
    {
        toBlock(0, 0),
        toBlock(0, 1)
    };

    block ciphertext[2];

    AES aes(seed);

    aes.ecbEncTwoBlocks(
        plaintext,
        ciphertext);

    leftRaw = ciphertext[0];
    rightRaw = ciphertext[1];
}

// G_i : seed -> nextSeed || valueOutput || macOutput.
static inline void vidpf_expand_output(
    const osuCrypto::block &seedTilde,
    int level,
    int ringBw,
    osuCrypto::block &nextSeed,
    VIDPFPayload &output)
{
    using namespace osuCrypto;

    static const block notOneBlock =
        toBlock(~0ULL, ~1ULL);

    const block plaintext0 =
        toBlock(
            static_cast<uint64_t>(level),
            0x1000ULL);

    const block plaintext1 =
        toBlock(
            static_cast<uint64_t>(level),
            0x2000ULL);

    const block plaintext2 =
        toBlock(
            static_cast<uint64_t>(level),
            0x3000ULL);

    AES aes(seedTilde);

    const block ciphertext0 =
        aes.ecbEncBlock(plaintext0);

    const block ciphertext1 =
        aes.ecbEncBlock(plaintext1);

    const block ciphertext2 =
        aes.ecbEncBlock(plaintext2);

    nextSeed =
        ciphertext0 &
        notOneBlock;

    output.value =
        vidpf_block_to_wide(
            ciphertext1,
            ringBw);

    output.mac =
        vidpf_block_to_wide(
            ciphertext2,
            ringBw);
}

} // namespace

VIDPFKeyGenResult keyGenVIDPF(
    int bin,
    int ringBw,
    GroupElement alpha,
    const std::vector<VIDPFPayload> &betaVec)
{
    using namespace osuCrypto;

    always_assert(bin > 0 && bin <= 64);
    always_assert(ringBw > 0 && ringBw <= 128);
    always_assert(
        static_cast<int>(betaVec.size()) == bin);

    mod(alpha, bin);

    static const block notOneBlock =
        toBlock(~0ULL, ~1ULL);

    VIDPFKeyGenResult result;

    result.key0 =
        VIDPFKeyPack(bin, ringBw);

    result.key1 =
        VIDPFKeyPack(bin, ringBw);

    result.t0.resize(bin);
    result.t1.resize(bin);

    const int tid =
        omp_get_thread_num();

    const auto roots =
        FSSConfig::prngs[tid]
            .get<std::array<block, 2>>();

    block seed0 =
        roots[0] &
        notOneBlock;

    block seed1 =
        roots[1] &
        notOneBlock;

    uint8_t control0 = 0;
    uint8_t control1 = 1;

    result.key0.root = seed0;
    result.key1.root = seed1;

    for (int i = 0; i < bin; ++i)
    {
        const int level = i + 1;

        const uint8_t alphaBit =
            vidpf_get_full_bit(
                alpha,
                bin,
                i);

        const GroupElement alphaPrefix =
            vidpf_prefix_of(
                alpha,
                bin,
                level);

        block leftRaw0;
        block rightRaw0;
        block leftRaw1;
        block rightRaw1;

        vidpf_expand_tree(
            seed0,
            leftRaw0,
            rightRaw0);

        vidpf_expand_tree(
            seed1,
            leftRaw1,
            rightRaw1);

        const block seedLeft0 =
            leftRaw0 &
            notOneBlock;

        const block seedRight0 =
            rightRaw0 &
            notOneBlock;

        const block seedLeft1 =
            leftRaw1 &
            notOneBlock;

        const block seedRight1 =
            rightRaw1 &
            notOneBlock;

        const uint8_t controlLeft0 =
            vidpf_lsb(leftRaw0);

        const uint8_t controlRight0 =
            vidpf_lsb(rightRaw0);

        const uint8_t controlLeft1 =
            vidpf_lsb(leftRaw1);

        const uint8_t controlRight1 =
            vidpf_lsb(rightRaw1);

        block seedKeep0;
        block seedKeep1;
        block seedLose0;
        block seedLose1;

        uint8_t controlKeep0;
        uint8_t controlKeep1;

        if (alphaBit == 0)
        {
            seedKeep0 = seedLeft0;
            seedKeep1 = seedLeft1;

            seedLose0 = seedRight0;
            seedLose1 = seedRight1;

            controlKeep0 = controlLeft0;
            controlKeep1 = controlLeft1;
        }
        else
        {
            seedKeep0 = seedRight0;
            seedKeep1 = seedRight1;

            seedLose0 = seedLeft0;
            seedLose1 = seedLeft1;

            controlKeep0 = controlRight0;
            controlKeep1 = controlRight1;
        }

        const uint8_t controlLeftCW =
            static_cast<uint8_t>(
                controlLeft0 ^
                controlLeft1 ^
                (alphaBit ^ 1));

        const uint8_t controlRightCW =
            static_cast<uint8_t>(
                controlRight0 ^
                controlRight1 ^
                alphaBit);

        const block seedCW =
            seedLose0 ^
            seedLose1;

        result.key0.scw[i] = seedCW;
        result.key1.scw[i] = seedCW;

        result.key0.tcw[i] =
            static_cast<uint8_t>(
                controlLeftCW |
                (controlRightCW << 1));

        result.key1.tcw[i] =
            result.key0.tcw[i];

        const uint8_t controlKeepCW =
            alphaBit == 0
                ? controlLeftCW
                : controlRightCW;

        const block seedTilde0 =
            seedKeep0 ^
            (control0
                ? seedCW
                : ZeroBlock);

        const block seedTilde1 =
            seedKeep1 ^
            (control1
                ? seedCW
                : ZeroBlock);

        const uint8_t newControl0 =
            static_cast<uint8_t>(
                controlKeep0 ^
                (control0
                    ? controlKeepCW
                    : 0));

        const uint8_t newControl1 =
            static_cast<uint8_t>(
                controlKeep1 ^
                (control1
                    ? controlKeepCW
                    : 0));

        result.key0.cs[i] =
            vidpf_hash4_xor(
                vidpf_Hi(
                    level,
                    seedTilde0,
                    newControl0,
                    alphaPrefix),

                vidpf_Hi(
                    level,
                    seedTilde1,
                    newControl1,
                    alphaPrefix));

        result.key1.cs[i] =
            result.key0.cs[i];

        block nextSeed0;
        block nextSeed1;

        VIDPFPayload output0;
        VIDPFPayload output1;

        vidpf_expand_output(
            seedTilde0,
            level,
            ringBw,
            nextSeed0,
            output0);

        vidpf_expand_output(
            seedTilde1,
            level,
            ringBw,
            nextSeed1,
            output1);

        VIDPFPayload outputCW =
            vidpf_payload_add(
                vidpf_payload_sub(
                    output1,
                    output0,
                    ringBw),

                betaVec[i],
                ringBw);

        if (newControl1)
        {
            outputCW =
                vidpf_payload_neg(
                    outputCW,
                    ringBw);
        }

        result.key0.ocw[i] = outputCW;
        result.key1.ocw[i] = outputCW;

        result.t0[i] = newControl0;
        result.t1[i] = newControl1;

        seed0 = nextSeed0;
        seed1 = nextSeed1;

        control0 = newControl0;
        control1 = newControl1;
    }

    return result;
}

VIDPFKeyGenResult keyGenVIDPF(
    int bin,
    int ringBw,
    GroupElement alpha,
    const VIDPFPayload &beta)
{
    return keyGenVIDPF(
        bin,
        ringBw,
        alpha,
        std::vector<VIDPFPayload>(
            bin,
            beta));
}

VIDPFEvalResult evalVIDPF(
    int party,
    const VIDPFKeyPack &key,
    const std::vector<VIDPFQuery> &queries)
{
    using namespace osuCrypto;

    always_assert(party == 0 || party == 1);
    always_assert(key.bin > 0 && key.bin <= 64);
    always_assert(key.ringBw > 0 && key.ringBw <= 128);

    static const block notOneBlock =
        toBlock(~0ULL, ~1ULL);

    VIDPFEvalResult result;

    result.y.resize(queries.size());
    result.t.resize(queries.size());

    std::vector<VIDPFHash4> muLevel(
        key.bin);

    for (int i = 0; i < key.bin; ++i)
    {
        muLevel[i] = key.cs[i];
    }

    for (size_t queryIndex = 0;
         queryIndex < queries.size();
         ++queryIndex)
    {
        const int length =
            queries[queryIndex].length;

        always_assert(
            length >= 1 &&
            length <= key.bin);

        GroupElement prefix =
            queries[queryIndex].prefix;

        if (length < 64)
        {
            prefix &=
                (GroupElement(1) << length) - 1;
        }

        block seed = key.root;

        uint8_t control =
            static_cast<uint8_t>(party);

        VIDPFPayload value;

        for (int i = 0; i < length; ++i)
        {
            const int level = i + 1;

            const uint8_t bit =
                vidpf_get_prefix_bit(
                    prefix,
                    length,
                    i);

            block leftRaw;
            block rightRaw;

            vidpf_expand_tree(
                seed,
                leftRaw,
                rightRaw);

            const block seedLeft =
                leftRaw &
                notOneBlock;

            const block seedRight =
                rightRaw &
                notOneBlock;

            const uint8_t controlLeft =
                vidpf_lsb(leftRaw);

            const uint8_t controlRight =
                vidpf_lsb(rightRaw);

            const uint8_t controlLeftCW =
                static_cast<uint8_t>(
                    key.tcw[i] & 1);

            const uint8_t controlRightCW =
                static_cast<uint8_t>(
                    (key.tcw[i] >> 1) & 1);

            const block seedKeep =
                bit == 0
                    ? seedLeft
                    : seedRight;

            const uint8_t controlKeep =
                bit == 0
                    ? controlLeft
                    : controlRight;

            const uint8_t controlKeepCW =
                bit == 0
                    ? controlLeftCW
                    : controlRightCW;

            const block seedTilde =
                seedKeep ^
                (control
                    ? key.scw[i]
                    : ZeroBlock);

            const uint8_t newControl =
                static_cast<uint8_t>(
                    controlKeep ^
                    (control
                        ? controlKeepCW
                        : 0));

            block nextSeed;
            VIDPFPayload expandedOutput;

            vidpf_expand_output(
                seedTilde,
                level,
                key.ringBw,
                nextSeed,
                expandedOutput);

            if (i == length - 1)
            {
                VIDPFHash4 temporary =
                    muLevel[i];

                vidpf_hash4_xor_inplace(
                    temporary,

                    vidpf_Hi(
                        level,
                        seedTilde,
                        newControl,
                        prefix));

                if (newControl)
                {
                    vidpf_hash4_xor_inplace(
                        temporary,
                        key.cs[i]);
                }

                vidpf_hash4_xor_token_inplace(
                    muLevel[i],
                    vidpf_HPrime(temporary));

                value = expandedOutput;

                if (newControl)
                {
                    value =
                        vidpf_payload_add(
                            value,
                            key.ocw[i],
                            key.ringBw);
                }

                if (party == 1)
                {
                    value =
                        vidpf_payload_neg(
                            value,
                            key.ringBw);
                }
            }

            seed = nextSeed;
            control = newControl;
        }

        result.y[queryIndex] = value;
        result.t[queryIndex] = control;
    }

    VIDPFToken finalToken;

    for (int i = 0; i < key.bin; ++i)
    {
        VIDPFHash4 temporary =
            muLevel[i];

        temporary.h[0] ^=
            finalToken.v[0];

        temporary.h[1] ^=
            finalToken.v[1];

        vidpf_token_xor_inplace(
            finalToken,
            vidpf_HPrime(temporary));
    }

    result.mu = finalToken;

    return result;
}

VIDPFPayload evalVIDPFPrefix(
    int party,
    const VIDPFKeyPack &key,
    GroupElement prefix,
    int prefixLength,
    uint8_t *controlBit)
{
    std::vector<VIDPFQuery> queries;

    queries.emplace_back(
        prefixLength,
        prefix);

    VIDPFEvalResult result =
        evalVIDPF(
            party,
            key,
            queries);

    if (controlBit != nullptr)
    {
        *controlBit = result.t[0];
    }

    return result.y[0];
}

bool verifyVIDPF(
    const VIDPFToken &mu0,
    const VIDPFToken &mu1)
{
    return
        vidpf_block_equal(
            mu0.v[0],
            mu1.v[0])
        &&
        vidpf_block_equal(
            mu0.v[1],
            mu1.v[1]);
}