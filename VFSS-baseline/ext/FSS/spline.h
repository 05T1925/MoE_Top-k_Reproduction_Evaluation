#pragma once

#include <FSS/keypack.h>
#include <FSS/group_element.h>

#include <vector>
#include <utility>
#include <cstddef>

struct CavernShiftedSpline
{
    // endpoints.size() == coeffs.size() + 1
    //
    // endpoints[0] == 0
    // endpoints.back() == 0 positionally represents 2^bin.
    //
    std::vector<GroupElement> endpoints;

    // Coefficient order:
    //
    // c[0] + c[1] X + ... + c[d] X^d
    //
    std::vector<std::vector<GroupElement>> coeffs;
};

struct CavernQueryPlan
{
    std::vector<VIDPFQuery> queries;

    // For each shifted endpoint, store the indices of the
    // corresponding left-sibling queries.
    std::vector<std::vector<size_t>>
        endpointQueryIndices;
};

struct CavernOpenedValue
{
    CavernAuthShare localMaskedShare;
    WideGroupElement openedFull;
};

CavernAuthShare cavernAuthAdd(
    const CavernAuthShare &a,
    const CavernAuthShare &b,
    int ringBw);

CavernAuthShare cavernAuthSub(
    const CavernAuthShare &a,
    const CavernAuthShare &b,
    int ringBw);

CavernAuthShare cavernAuthNeg(
    const CavernAuthShare &a,
    int ringBw);

CavernAuthShare cavernAuthMulPublic(
    const CavernAuthShare &a,
    const WideGroupElement &constant,
    int ringBw);

CavernAuthShare cavernAuthPublic(
    int party,
    GroupElement value,
    const WideGroupElement &deltaShare,
    int ringBw);

CavernAuthShare cavernBuildInputShare(
    int party,
    GroupElement maskedInput,
    const CavernSplineKeyPack &key);

std::pair<CavernSplineKeyPack, CavernSplineKeyPack>
keyGenCavernSplineWithMACKeys(
    int bin,
    int slack,
    int numPoly,
    int degree,
    GroupElement inputMask,
    GroupElement outputMask,
    const WideGroupElement &delta0,
    const WideGroupElement &delta1);

std::pair<CavernSplineKeyPack, CavernSplineKeyPack>
keyGenCavernSpline(
    int bin,
    int slack,
    int numPoly,
    int degree,
    GroupElement inputMask,
    GroupElement outputMask);

std::vector<VIDPFQuery>
cavernLeftSiblingNodes(
    GroupElement endpoint,
    int bin);

CavernShiftedSpline cavernShiftSpline(
    int bin,
    GroupElement shift,
    const std::vector<GroupElement> &endpoints,
    const std::vector<std::vector<GroupElement>> &coeffs);

CavernQueryPlan cavernBuildQueryPlan(
    int bin,
    const std::vector<GroupElement> &shiftedEndpoints);

std::vector<VIDPFQuery> cavernBuildTagQueries(
    int bin,
    GroupElement a,
    GroupElement b,
    const std::vector<VIDPFQuery> &mainQueries);

std::vector<CavernAuthShare> cavernBuildIndicators(
    int party,
    int ringBw,
    const CavernQueryPlan &plan,
    const VIDPFEvalResult &mainEval,
    const CavernAuthShare &betaShare);

std::vector<CavernAuthShare> cavernSelectPolynomial(
    int ringBw,
    int degree,
    const std::vector<CavernAuthShare> &indicators,
    const std::vector<std::vector<GroupElement>> &coeffs);

CavernAuthShare cavernFinishAuthMultiply(
    int party,
    int ringBw,
    const CavernTripleShare &triple,
    const WideGroupElement &e,
    const WideGroupElement &f,
    const WideGroupElement &deltaShare);

WideGroupElement cavernComputeOmegaShare(
    int party,
    int ringBw,
    GroupElement c,
    GroupElement h,
    GroupElement batchingChallenge,
    const std::vector<VIDPFQuery> &queries,
    const VIDPFEvalResult &mainEval,
    const VIDPFEvalResult &tagEval,
    const std::vector<WideGroupElement> &qByLevel);