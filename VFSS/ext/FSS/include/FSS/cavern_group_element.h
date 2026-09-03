#pragma once

#include <FSS/group_element.h>
#include <FSS/config.h>

#include <cstdint>
#include <cstdlib>
#include <utility>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace cavern_detail
{

inline void require(bool condition)
{
    if (!condition)
    {
        std::abort();
    }
}

inline int thread_id()
{
#ifdef _OPENMP
    return omp_get_thread_num();
#else
    return 0;
#endif
}

} // namespace cavern_detail

// ============================================================================
// CAVERN arithmetic over Z_{2^bw}, 1 <= bw <= 128.
// ============================================================================

struct WideGroupElement
{
    uint64_t lo;
    uint64_t hi;

    WideGroupElement(uint64_t value = 0)
        : lo(value),
          hi(0)
    {
    }

    WideGroupElement(
        uint64_t low,
        uint64_t high)
        : lo(low),
          hi(high)
    {
    }
};

inline bool wideEqual(
    const WideGroupElement &a,
    const WideGroupElement &b)
{
    return
        a.lo == b.lo &&
        a.hi == b.hi;
}

inline bool wideIsZero(
    const WideGroupElement &value)
{
    return
        value.lo == 0 &&
        value.hi == 0;
}

inline void wideMod(
    WideGroupElement &value,
    int bw)
{
    cavern_detail::require(
        bw >= 1 &&
        bw <= 128);

    if (bw == 128)
    {
        return;
    }

    if (bw <= 64)
    {
        value.hi = 0;

        if (bw < 64)
        {
            value.lo &=
                (uint64_t(1) << bw) - 1;
        }

        return;
    }

    const int highBits =
        bw - 64;

    if (highBits < 64)
    {
        value.hi &=
            (uint64_t(1) << highBits) - 1;
    }
}

inline WideGroupElement wideAdd(
    const WideGroupElement &a,
    const WideGroupElement &b,
    int bw = 128)
{
    WideGroupElement result;

    result.lo =
        a.lo + b.lo;

    result.hi =
        a.hi +
        b.hi +
        static_cast<uint64_t>(
            result.lo < a.lo);

    wideMod(
        result,
        bw);

    return result;
}

inline WideGroupElement wideNeg(
    const WideGroupElement &value,
    int bw = 128)
{
    WideGroupElement result;

    result.lo =
        ~value.lo + 1;

    result.hi =
        ~value.hi +
        static_cast<uint64_t>(
            result.lo == 0);

    wideMod(
        result,
        bw);

    return result;
}

inline WideGroupElement wideSub(
    const WideGroupElement &a,
    const WideGroupElement &b,
    int bw = 128)
{
    return wideAdd(
        a,
        wideNeg(b, bw),
        bw);
}

// Return the low 128 bits of a*b.
inline WideGroupElement wideMul(
    const WideGroupElement &a,
    const WideGroupElement &b,
    int bw = 128)
{
    const unsigned __int128 lowProduct =
        static_cast<unsigned __int128>(
            a.lo) *
        b.lo;

    const unsigned __int128 crossProduct =
        static_cast<unsigned __int128>(
            a.lo) *
        b.hi
        +
        static_cast<unsigned __int128>(
            a.hi) *
        b.lo;

    WideGroupElement result;

    result.lo =
        static_cast<uint64_t>(
            lowProduct);

    result.hi =
        static_cast<uint64_t>(
            lowProduct >> 64)
        +
        static_cast<uint64_t>(
            crossProduct);

    wideMod(
        result,
        bw);

    return result;
}

inline WideGroupElement wideMulPublic(
    const WideGroupElement &value,
    GroupElement publicValue,
    int bw = 128)
{
    return wideMul(
        value,
        WideGroupElement(publicValue),
        bw);
}

inline WideGroupElement wideFromGE(
    GroupElement value,
    int bw = 128)
{
    WideGroupElement result(
        value,
        0);

    wideMod(
        result,
        bw);

    return result;
}

inline GroupElement wideLowGE(
    const WideGroupElement &value,
    int bw)
{
    cavern_detail::require(
        bw >= 1 &&
        bw <= 64);

    GroupElement result =
        value.lo;

    if (bw < 64)
    {
        result &=
            (GroupElement(1) << bw) - 1;
    }

    return result;
}

inline WideGroupElement wideShiftLeft(
    const WideGroupElement &value,
    int shift,
    int bw = 128)
{
    cavern_detail::require(
        shift >= 0 &&
        shift <= 127);

    WideGroupElement result;

    if (shift == 0)
    {
        result = value;
    }
    else if (shift < 64)
    {
        result.lo =
            value.lo << shift;

        result.hi =
            (value.hi << shift)
            |
            (value.lo >> (64 - shift));
    }
    else
    {
        result.lo = 0;

        result.hi =
            value.lo << (shift - 64);
    }

    wideMod(
        result,
        bw);

    return result;
}

inline WideGroupElement widePow(
    WideGroupElement base,
    uint64_t exponent,
    int bw = 128)
{
    WideGroupElement result(1);

    wideMod(
        base,
        bw);

    while (exponent != 0)
    {
        if ((exponent & 1) != 0)
        {
            result =
                wideMul(
                    result,
                    base,
                    bw);
        }

        exponent >>= 1;

        if (exponent != 0)
        {
            base =
                wideMul(
                    base,
                    base,
                    bw);
        }
    }

    return result;
}

inline WideGroupElement random_wide_ge(
    int bw)
{
    cavern_detail::require(
        bw >= 1 &&
        bw <= 128);

    const int tid =
        cavern_detail::thread_id();

    WideGroupElement result(
        FSSConfig::prngs[tid]
            .get<uint64_t>(),

        FSSConfig::prngs[tid]
            .get<uint64_t>());

    wideMod(
        result,
        bw);

    return result;
}

inline std::pair<
    WideGroupElement,
    WideGroupElement>
splitWideShare(
    const WideGroupElement &value,
    int bw)
{
    const WideGroupElement first =
        random_wide_ge(bw);

    const WideGroupElement second =
        wideSub(
            value,
            first,
            bw);

    return std::make_pair(
        first,
        second);
}