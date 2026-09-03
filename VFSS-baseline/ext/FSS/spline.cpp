#include "spline.h"

#include <FSS/dpf.h>
#include <FSS/assert.h>

#include <algorithm>
#include <unordered_map>
#include <sstream>
#include <omp.h>

namespace
{

std::pair<CavernAuthShare, CavernAuthShare>
cavernSplitAuthShare(
    const WideGroupElement &clearValue,
    const WideGroupElement &delta,
    int ringBw)
{
    CavernAuthShare share0;
    CavernAuthShare share1;

    share0.value =
        random_wide_ge(ringBw);

    share1.value =
        wideSub(
            clearValue,
            share0.value,
            ringBw);

    const WideGroupElement fullMac =
        wideMul(
            delta,
            clearValue,
            ringBw);

    share0.mac =
        random_wide_ge(ringBw);

    share1.mac =
        wideSub(
            fullMac,
            share0.mac,
            ringBw);

    return std::make_pair(
        share0,
        share1);
}

CavernTripleShare cavernGetPartyTriple(
    int party,
    const std::pair<CavernAuthShare, CavernAuthShare> &a,
    const std::pair<CavernAuthShare, CavernAuthShare> &b,
    const std::pair<CavernAuthShare, CavernAuthShare> &c)
{
    CavernTripleShare result;

    result.a =
        party == 0
            ? a.first
            : a.second;

    result.b =
        party == 0
            ? b.first
            : b.second;

    result.c =
        party == 0
            ? c.first
            : c.second;

    return result;
}

WideGroupElement cavernUpperOpeningMaskValue(
    int bin,
    int slack,
    int ringBw)
{
    if (slack == 0)
    {
        return WideGroupElement(0);
    }

    const WideGroupElement randomHigh =
        random_wide_ge(slack);

    return wideShiftLeft(
        randomHigh,
        bin,
        ringBw);
}

GroupElement cavernPrefix(
    GroupElement value,
    int bin,
    int length)
{
    if (length == 64)
    {
        return value;
    }

    return
        (value >> (bin - length)) &
        ((GroupElement(1) << length) - 1);
}

struct CavernSegment
{
    unsigned __int128 start;
    unsigned __int128 end;

    std::vector<GroupElement> coeff;
};

std::string cavernQueryKey(
    const VIDPFQuery &query)
{
    std::ostringstream output;

    output
        << query.length
        << ':'
        << query.prefix;

    return output.str();
}

} // namespace

CavernAuthShare cavernAuthAdd(
    const CavernAuthShare &a,
    const CavernAuthShare &b,
    int ringBw)
{
    CavernAuthShare result;

    result.value =
        wideAdd(
            a.value,
            b.value,
            ringBw);

    result.mac =
        wideAdd(
            a.mac,
            b.mac,
            ringBw);

    return result;
}

CavernAuthShare cavernAuthSub(
    const CavernAuthShare &a,
    const CavernAuthShare &b,
    int ringBw)
{
    CavernAuthShare result;

    result.value =
        wideSub(
            a.value,
            b.value,
            ringBw);

    result.mac =
        wideSub(
            a.mac,
            b.mac,
            ringBw);

    return result;
}

CavernAuthShare cavernAuthNeg(
    const CavernAuthShare &input,
    int ringBw)
{
    CavernAuthShare result;

    result.value =
        wideNeg(
            input.value,
            ringBw);

    result.mac =
        wideNeg(
            input.mac,
            ringBw);

    return result;
}

CavernAuthShare cavernAuthMulPublic(
    const CavernAuthShare &input,
    const WideGroupElement &constant,
    int ringBw)
{
    CavernAuthShare result;

    result.value =
        wideMul(
            input.value,
            constant,
            ringBw);

    result.mac =
        wideMul(
            input.mac,
            constant,
            ringBw);

    return result;
}

CavernAuthShare cavernAuthPublic(
    int party,
    GroupElement value,
    const WideGroupElement &deltaShare,
    int ringBw)
{
    CavernAuthShare result;

    const WideGroupElement wideValue =
        wideFromGE(
            value,
            ringBw);

    result.value =
        party == 0
            ? wideValue
            : WideGroupElement(0);

    result.mac =
        wideMul(
            deltaShare,
            wideValue,
            ringBw);

    return result;
}

CavernAuthShare cavernBuildInputShare(
    int party,
    GroupElement maskedInput,
    const CavernSplineKeyPack &key)
{
    const CavernAuthShare publicMaskedInput =
        cavernAuthPublic(
            party,
            maskedInput,
            key.deltaShare,
            key.ringBw);

    return cavernAuthAdd(
        publicMaskedInput,
        key.negInputMaskShare,
        key.ringBw);
}

std::pair<CavernSplineKeyPack, CavernSplineKeyPack>
keyGenCavernSplineWithMACKeys(
    int bin,
    int slack,
    int numPoly,
    int degree,
    GroupElement inputMask,
    GroupElement outputMask,
    const WideGroupElement &delta0Input,
    const WideGroupElement &delta1Input)
{
    always_assert(bin >= 1 && bin <= 64);
    always_assert(slack >= 0);
    always_assert(bin + slack <= 128);
    always_assert(numPoly >= 1);
    always_assert(degree >= 0);

    const int ringBw =
        bin + slack;

    CavernSplineKeyPack key0;
    CavernSplineKeyPack key1;

    key0.bin = key1.bin = bin;
    key0.slack = key1.slack = slack;
    key0.ringBw = key1.ringBw = ringBw;
    key0.numPoly = key1.numPoly = numPoly;
    key0.degree = key1.degree = degree;

    WideGroupElement delta0 =
        delta0Input;

    WideGroupElement delta1 =
        delta1Input;

    if (slack == 0)
    {
        delta0 = WideGroupElement(1);
        delta1 = WideGroupElement(0);
    }

    wideMod(delta0, ringBw);
    wideMod(delta1, ringBw);

    const WideGroupElement delta =
        wideAdd(
            delta0,
            delta1,
            ringBw);

    key0.deltaShare = delta0;
    key1.deltaShare = delta1;

    const GroupElement alpha =
        random_ge(bin);

    const GroupElement m =
        random_ge(bin);

    const GroupElement tagA =
        random_ge(bin);

    const GroupElement tagB =
        random_ge(bin);

    const GroupElement tagC =
        random_ge(bin);

    const GroupElement tagH =
        random_ge(bin);

    key0.tagA = key1.tagA = tagA;
    key0.tagB = key1.tagB = tagB;
    key0.tagC = key1.tagC = tagC;
    key0.tagH = key1.tagH = tagH;

    GroupElement negInputMask =
        0 - inputMask;

    mod(negInputMask, bin);
    mod(outputMask, bin);

    const auto alphaShares =
        cavernSplitAuthShare(
            wideFromGE(alpha, ringBw),
            delta,
            ringBw);

    const auto betaShares =
        cavernSplitAuthShare(
            wideFromGE(1, ringBw),
            delta,
            ringBw);

    const auto negInputMaskShares =
        cavernSplitAuthShare(
            wideFromGE(
                negInputMask,
                ringBw),

            delta,
            ringBw);

    const auto outputMaskShares =
        cavernSplitAuthShare(
            wideFromGE(
                outputMask,
                ringBw),

            delta,
            ringBw);

    key0.alphaShare =
        alphaShares.first;

    key1.alphaShare =
        alphaShares.second;

    key0.betaShare =
        betaShares.first;

    key1.betaShare =
        betaShares.second;

    key0.negInputMaskShare =
        negInputMaskShares.first;

    key1.negInputMaskShare =
        negInputMaskShares.second;

    key0.outputMaskShare =
        outputMaskShares.first;

    key1.outputMaskShare =
        outputMaskShares.second;

    VIDPFPayload mainPayload;

    mainPayload.value =
        wideFromGE(
            1,
            ringBw);

    mainPayload.mac =
        wideMul(
            delta,
            mainPayload.value,
            ringBw);

#ifdef CAVERN_LITERAL_FIGURE7_AND

    GroupElement eta =
        tagA &
        (alpha ^ tagB);

#else

    // The literal AND transformation in the current paper is not injective.
    // The XOR transformation preserves:
    //
    // tag(z) == eta_prefix  <=>  z == alpha_prefix.
    //
    GroupElement eta =
        tagA ^
        alpha ^
        tagB;

#endif

    mod(eta, bin);

    WideGroupElement theta =
        wideAdd(
            wideFromGE(
                1,
                ringBw),

            wideMul(
                wideFromGE(
                    tagH,
                    ringBw),

                wideFromGE(
                    m,
                    ringBw),

                ringBw),

            ringBw);

    theta =
        wideMul(
            wideFromGE(
                tagC,
                ringBw),

            theta,
            ringBw);

    VIDPFPayload tagPayload;

    tagPayload.value =
        theta;

    tagPayload.mac =
        wideMul(
            delta,
            theta,
            ringBw);

    VIDPFKeyGenResult mainKeys =
        keyGenVIDPF(
            bin,
            ringBw,
            alpha,
            mainPayload);

    VIDPFKeyGenResult tagKeys =
        keyGenVIDPF(
            bin,
            ringBw,
            eta,
            tagPayload);

    key0.mainVIDPF =
        mainKeys.key0;

    key1.mainVIDPF =
        mainKeys.key1;

    key0.tagVIDPF =
        tagKeys.key0;

    key1.tagVIDPF =
        tagKeys.key1;

    key0.q.resize(bin);
    key1.q.resize(bin);

    const WideGroupElement wideM =
        wideFromGE(
            m,
            ringBw);

    for (int level = 0;
         level < bin;
         ++level)
    {
        const WideGroupElement q =
            mainKeys.t1[level]
                ? wideNeg(
                    wideM,
                    ringBw)
                : wideM;

        key0.q[level] = q;
        key1.q[level] = q;
    }

    key0.polyTriples.resize(degree);
    key1.polyTriples.resize(degree);

    for (int i = 0; i < degree; ++i)
    {
        // Clear Beaver values are sampled from the n-bit value ring.
        const GroupElement clearA =
            random_ge(bin);

        const GroupElement clearB =
            random_ge(bin);

        GroupElement clearC =
            clearA * clearB;

        mod(clearC, bin);

        const WideGroupElement a =
            wideFromGE(
                clearA,
                ringBw);

        const WideGroupElement b =
            wideFromGE(
                clearB,
                ringBw);

        const WideGroupElement c =
            wideFromGE(
                clearC,
                ringBw);

        const auto aShares =
            cavernSplitAuthShare(
                a,
                delta,
                ringBw);

        const auto bShares =
            cavernSplitAuthShare(
                b,
                delta,
                ringBw);

        const auto cShares =
            cavernSplitAuthShare(
                c,
                delta,
                ringBw);

        key0.polyTriples[i] =
            cavernGetPartyTriple(
                0,
                aShares,
                bShares,
                cShares);

        key1.polyTriples[i] =
            cavernGetPartyTriple(
                1,
                aShares,
                bShares,
                cShares);
    }

    const int numOpenMasks =
        1 +
        2 * degree +
        1;

    key0.openMasks.resize(
        numOpenMasks);

    key1.openMasks.resize(
        numOpenMasks);

    for (int i = 0;
         i < numOpenMasks;
         ++i)
    {
        const WideGroupElement maskValue =
            cavernUpperOpeningMaskValue(
                bin,
                slack,
                ringBw);

        const auto maskShares =
            cavernSplitAuthShare(
                maskValue,
                delta,
                ringBw);

        key0.openMasks[i] =
            maskShares.first;

        key1.openMasks[i] =
            maskShares.second;
    }

    return std::make_pair(
        key0,
        key1);
}

std::pair<CavernSplineKeyPack, CavernSplineKeyPack>
keyGenCavernSpline(
    int bin,
    int slack,
    int numPoly,
    int degree,
    GroupElement inputMask,
    GroupElement outputMask)
{
    WideGroupElement delta0 =
        random_wide_ge(
            std::max(1, slack));

    WideGroupElement delta1 =
        random_wide_ge(
            std::max(1, slack));

    return keyGenCavernSplineWithMACKeys(
        bin,
        slack,
        numPoly,
        degree,
        inputMask,
        outputMask,
        delta0,
        delta1);
}

std::vector<VIDPFQuery>
cavernLeftSiblingNodes(
    GroupElement endpoint,
    int bin)
{
    always_assert(bin >= 1 && bin <= 64);

    mod(endpoint, bin);

    std::vector<VIDPFQuery> result;

    for (int level = 1;
         level <= bin;
         ++level)
    {
        const uint8_t endpointBit =
            static_cast<uint8_t>(
                (endpoint >> (bin - level)) &
                1ULL);

        if (endpointBit == 0)
        {
            continue;
        }

        GroupElement prefix =
            cavernPrefix(
                endpoint,
                bin,
                level);

        // Replace the final 1 with its left sibling 0.
        prefix &= ~GroupElement(1);

        result.emplace_back(
            level,
            prefix);
    }

    return result;
}

CavernShiftedSpline cavernShiftSpline(
    int bin,
    GroupElement shift,
    const std::vector<GroupElement> &endpoints,
    const std::vector<std::vector<GroupElement>> &coeffs)
{
    always_assert(bin >= 1 && bin <= 64);
    always_assert(
        endpoints.size() ==
        coeffs.size() + 1);

    always_assert(!coeffs.empty());
    always_assert(endpoints.front() == 0);

    mod(shift, bin);

    const unsigned __int128 domainSize =
        static_cast<unsigned __int128>(1)
        << bin;

    const unsigned __int128 shiftWide =
        shift;

    std::vector<CavernSegment> segments;

    segments.reserve(
        coeffs.size() + 1);

    for (size_t interval = 0;
         interval < coeffs.size();
         ++interval)
    {
        const unsigned __int128 left =
            interval == 0
                ? 0
                : static_cast<unsigned __int128>(
                    endpoints[interval]);

        const unsigned __int128 right =
            interval + 1 ==
                    endpoints.size() - 1
                ? domainSize
                : static_cast<unsigned __int128>(
                    endpoints[interval + 1]);

        const unsigned __int128 shiftedLeft =
            (left +
             domainSize -
             shiftWide)
            %
            domainSize;

        const unsigned __int128 shiftedRight =
            (right +
             domainSize -
             shiftWide)
            %
            domainSize;

        if (shiftedLeft < shiftedRight)
        {
            segments.push_back(
                {
                    shiftedLeft,
                    shiftedRight,
                    coeffs[interval]
                });
        }
        else if (shiftedLeft > shiftedRight)
        {
            segments.push_back(
                {
                    shiftedLeft,
                    domainSize,
                    coeffs[interval]
                });

            if (shiftedRight != 0)
            {
                segments.push_back(
                    {
                        0,
                        shiftedRight,
                        coeffs[interval]
                    });
            }
        }
        else
        {
            segments.push_back(
                {
                    0,
                    domainSize,
                    coeffs[interval]
                });
        }
    }

    std::sort(
        segments.begin(),
        segments.end(),

        [](
            const CavernSegment &a,
            const CavernSegment &b)
        {
            return a.start < b.start;
        });

    always_assert(!segments.empty());
    always_assert(
        segments.front().start == 0);

    CavernShiftedSpline result;

    result.endpoints.reserve(
        segments.size() + 1);

    result.coeffs.reserve(
        segments.size());

    for (const auto &segment : segments)
    {
        result.endpoints.push_back(
            static_cast<GroupElement>(
                segment.start));

        result.coeffs.push_back(
            segment.coeff);
    }

    // Positionally represents 2^bin.
    result.endpoints.push_back(0);

    return result;
}

CavernQueryPlan cavernBuildQueryPlan(
    int bin,
    const std::vector<GroupElement> &shiftedEndpoints)
{
    always_assert(
        shiftedEndpoints.size() >= 2);

    CavernQueryPlan result;

    result.endpointQueryIndices.resize(
        shiftedEndpoints.size());

    std::unordered_map<std::string, size_t>
        queryIndexMap;

    for (size_t endpointIndex = 1;
         endpointIndex + 1 <
             shiftedEndpoints.size();
         ++endpointIndex)
    {
        const auto nodes =
            cavernLeftSiblingNodes(
                shiftedEndpoints[endpointIndex],
                bin);

        for (const auto &query : nodes)
        {
            const std::string key =
                cavernQueryKey(query);

            const auto iterator =
                queryIndexMap.find(key);

            size_t queryIndex;

            if (iterator ==
                queryIndexMap.end())
            {
                queryIndex =
                    result.queries.size();

                result.queries.push_back(
                    query);

                queryIndexMap.emplace(
                    key,
                    queryIndex);
            }
            else
            {
                queryIndex =
                    iterator->second;
            }

            result
                .endpointQueryIndices[endpointIndex]
                .push_back(queryIndex);
        }
    }

    return result;
}

std::vector<VIDPFQuery> cavernBuildTagQueries(
    int bin,
    GroupElement a,
    GroupElement b,
    const std::vector<VIDPFQuery> &mainQueries)
{
    std::vector<VIDPFQuery> result;

    result.reserve(
        mainQueries.size());

    for (const auto &query :
         mainQueries)
    {
        const int length =
            query.length;

        GroupElement mask =
            ~GroupElement(0);

        if (length < 64)
        {
            mask =
                (GroupElement(1) << length) - 1;
        }

        const GroupElement aPrefix =
            cavernPrefix(
                a,
                bin,
                length);

        const GroupElement bPrefix =
            cavernPrefix(
                b,
                bin,
                length);

#ifdef CAVERN_LITERAL_FIGURE7_AND

        const GroupElement transformed =
            aPrefix &
            ((query.prefix ^ bPrefix) & mask);

#else

        const GroupElement transformed =
            (
                aPrefix ^
                query.prefix ^
                bPrefix
            )
            &
            mask;

#endif

        result.emplace_back(
            length,
            transformed);
    }

    return result;
}

std::vector<CavernAuthShare> cavernBuildIndicators(
    int party,
    int ringBw,
    const CavernQueryPlan &plan,
    const VIDPFEvalResult &mainEval,
    const CavernAuthShare &betaShare)
{
    always_assert(
        mainEval.y.size() ==
        plan.queries.size());

    const size_t numIntervals =
        plan.endpointQueryIndices.size() - 1;

    std::vector<CavernAuthShare> prefixValues(
        plan.endpointQueryIndices.size());

    for (size_t endpoint = 1;
         endpoint < numIntervals;
         ++endpoint)
    {
        CavernAuthShare sum;

        for (size_t queryIndex :
             plan.endpointQueryIndices[endpoint])
        {
            sum =
                cavernAuthAdd(
                    sum,
                    mainEval.y[queryIndex],
                    ringBw);
        }

        prefixValues[endpoint] =
            sum;
    }

    std::vector<CavernAuthShare> indicators(
        numIntervals);

    if (numIntervals == 1)
    {
        indicators[0] =
            betaShare;

        return indicators;
    }

    indicators[0] =
        prefixValues[1];

    for (size_t interval = 1;
         interval + 1 < numIntervals;
         ++interval)
    {
        indicators[interval] =
            cavernAuthSub(
                prefixValues[interval + 1],
                prefixValues[interval],
                ringBw);
    }

    indicators[numIntervals - 1] =
        cavernAuthSub(
            betaShare,
            prefixValues[numIntervals - 1],
            ringBw);

    (void)party;

    return indicators;
}

std::vector<CavernAuthShare> cavernSelectPolynomial(
    int ringBw,
    int degree,
    const std::vector<CavernAuthShare> &indicators,
    const std::vector<std::vector<GroupElement>> &coeffs)
{
    always_assert(
        indicators.size() ==
        coeffs.size());

    std::vector<CavernAuthShare> selected(
        degree + 1);

    for (size_t interval = 0;
         interval < coeffs.size();
         ++interval)
    {
        always_assert(
            static_cast<int>(
                coeffs[interval].size())
            ==
            degree + 1);

        for (int coefficient = 0;
             coefficient <= degree;
             ++coefficient)
        {
            selected[coefficient] =
                cavernAuthAdd(
                    selected[coefficient],

                    cavernAuthMulPublic(
                        indicators[interval],

                        wideFromGE(
                            coeffs[interval][coefficient],
                            ringBw),

                        ringBw),

                    ringBw);
        }
    }

    return selected;
}

CavernAuthShare cavernFinishAuthMultiply(
    int party,
    int ringBw,
    const CavernTripleShare &triple,
    const WideGroupElement &e,
    const WideGroupElement &f,
    const WideGroupElement &deltaShare)
{
    CavernAuthShare result =
        triple.c;

    result =
        cavernAuthAdd(
            result,

            cavernAuthMulPublic(
                triple.b,
                e,
                ringBw),

            ringBw);

    result =
        cavernAuthAdd(
            result,

            cavernAuthMulPublic(
                triple.a,
                f,
                ringBw),

            ringBw);

    const WideGroupElement ef =
        wideMul(
            e,
            f,
            ringBw);

    if (party == 0)
    {
        result.value =
            wideAdd(
                result.value,
                ef,
                ringBw);
    }

    result.mac =
        wideAdd(
            result.mac,

            wideMul(
                deltaShare,
                ef,
                ringBw),

            ringBw);

    return result;
}

WideGroupElement cavernComputeOmegaShare(
    int party,
    int ringBw,
    GroupElement c,
    GroupElement h,
    GroupElement batchingChallenge,
    const std::vector<VIDPFQuery> &queries,
    const VIDPFEvalResult &mainEval,
    const VIDPFEvalResult &tagEval,
    const std::vector<WideGroupElement> &qByLevel)
{
    always_assert(
        mainEval.y.size() ==
        queries.size());

    always_assert(
        tagEval.y.size() ==
        queries.size());

    always_assert(
        mainEval.t.size() ==
        queries.size());

    const WideGroupElement wideC =
        wideFromGE(
            c,
            ringBw);

    const WideGroupElement wideH =
        wideFromGE(
            h,
            ringBw);

    const WideGroupElement challenge =
        wideFromGE(
            batchingChallenge,
            ringBw);

    WideGroupElement coefficient(1);
    WideGroupElement mainSum(0);
    WideGroupElement tagSum(0);

    for (size_t queryIndex = 0;
         queryIndex < queries.size();
         ++queryIndex)
    {
        coefficient =
            wideMul(
                coefficient,
                challenge,
                ringBw);

        WideGroupElement term =
            mainEval.y[queryIndex].value;

        if (mainEval.t[queryIndex])
        {
            WideGroupElement correction =
                wideMul(
                    qByLevel[
                        queries[queryIndex].length - 1],

                    wideH,
                    ringBw);

            if (party == 1)
            {
                correction =
                    wideNeg(
                        correction,
                        ringBw);
            }

            term =
                wideAdd(
                    term,
                    correction,
                    ringBw);
        }

        mainSum =
            wideAdd(
                mainSum,

                wideMul(
                    coefficient,
                    term,
                    ringBw),

                ringBw);

        tagSum =
            wideAdd(
                tagSum,

                wideMul(
                    coefficient,
                    tagEval.y[queryIndex].value,
                    ringBw),

                ringBw);
    }

    return wideSub(
        wideMul(
            wideC,
            mainSum,
            ringBw),

        tagSum,
        ringBw);
}