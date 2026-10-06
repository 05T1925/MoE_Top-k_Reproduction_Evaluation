#!/usr/bin/env python3
"""Finite-parameter union bound for the project's actual S4-derived sampler."""
from __future__ import annotations

import json
import math
import sys


def ceil_sqrt(x: int) -> int:
    r = math.isqrt(x)
    return r if r * r == x else r + 1


def ceil_pow_3_4(m: int) -> int:
    target = (8**4) * m * m * m
    lo, hi = 0, 1
    while hi**4 < target:
        hi *= 2
    while lo + 1 < hi:
        mid = (lo + hi) // 2
        if mid**4 >= target:
            hi = mid
        else:
            lo = mid
    return hi


def params(n: int, k: int) -> dict[str, int]:
    if n < 1 or not 1 <= k <= n:
        raise ValueError("invalid n,K")
    r = n - k + 1
    N = n + 1
    low = max(N - 2 * r, 0)
    high = max(2 * r - N, 0)
    M = 2 * max(k, n - k + 1)
    if M != N + low + high or M % 2:
        raise AssertionError("two-sentinel expansion identity")
    s = min(M, ceil_sqrt(64 * M))
    if s == M:
        ql, qh = M // 2, M // 2 + 1
    else:
        a = ceil_sqrt(16 * s)
        ql = max(1, (s + 1) // 2 - a)
        qh = min(s, (s + 2) // 2 + a)
    ucap = min(M, ceil_pow_3_4(M))
    # v = min(u, ceil(8u/sqrt(M))); expression is integer-exact via ceil_sqrt(64*u^2/M).
    # Parameterized at a representative worst-case U for reporting only; the proof uses any u.
    return {"n": n, "K": k, "M_each_partition": M, "s": s,
            "qL": ql, "qH": qh, "U_cap": ucap,
            "W_cap": 2 * ceil_sqrt(4 * M) + 1}


def bounds(M: int) -> dict[str, float]:
    # If s=M, bracket is exact. Otherwise M>=66, s>=65; account for the
    # half-unit qL/qH rounding slack with a conservative exp(-31) tail
    # throughout the runtime domain M<=2,000,000.
    s = min(M, ceil_sqrt(64 * M))
    bracket = 0.0 if s == M else 2 * math.exp(-31)
    ucap = 0.0 if M <= 4096 else M * math.exp(-8 * M ** 0.25)
    wcap = 0.0 if M <= 64 else M * math.exp(-32)
    # two independently randomized tasks; union bound needs no independence.
    combined = 2.0 * (bracket + ucap + wcap)
    return {"bracket_per_partition": bracket, "U_cap_per_partition": ucap,
            "W_cap_per_partition": wcap, "two_partition_union_bound": combined}


def domain_max(max_m: int) -> dict[str, object]:
    # For M<=4096, p_U=0 and the bound is zero through M=65, then increases
    # with M. For M>4096, write x=M^(1/4)>8; the derivative of
    # M*exp(-8x)+M*exp(-32) is positive because
    # (2x-1)*exp(-8x)<exp(-31). Thus only the cut points and upper endpoint
    # need evaluation, rather than sampling a subset of K values.
    candidates = sorted({m for m in (2, 64, 65, 66, 4096, 4097, max_m)
                         if 2 <= m <= max_m})
    values = [(m, bounds(m)["two_partition_union_bound"]) for m in candidates]
    best_m, best = max(values, key=lambda row: row[1])
    return {"max_M": max_m, "checked_piecewise_candidates": candidates,
            "candidate_bounds": [{"M": m, "bound": value} for m, value in values],
            "maximum_M": best_m, "maximum_bound": best}


def main() -> int:
    rows = []
    for n, ks in ((1, (1,)), (2, (1, 2)), (64, (1, 8, 32, 64)),
                  (128, (2, 8, 80, 128)), (256, (2, 8, 128, 256)),
                  (1_000_000, (1, 80, 500_000, 1_000_000))):
        for k in ks:
            p = params(n, k)
            p.update(bounds(p["M_each_partition"]))
            rows.append(p)
    out = {"label": "BMW16_S15_IDEAL_SAMPLER_ABORT_UNION_BOUND_PROJECT_DERIVATION",
           "not_bmw16_theorem8": True,
           "domain_maxima": {"n_le_256": domain_max(512),
                             "n_le_1e6_formula_only": domain_max(2_000_000)},
           "assumptions": [
               "Strict total order and exact comparisons.",
               "Uniform independent-without-replacement S and V samples within each task.",
               "Actual integer parameters: s=min(M,ceil(8 sqrt(M))), qL/qH as in the C++ implementation, Ucap=min(M,ceil(8 M^(3/4))), v=min(u,ceil(8u/sqrt(M))), Wcap=2ceil(2sqrt(M))+1.",
               "Two task union bound does not require independence of their abort events.",
               "This ideal statistical bound is separate from the CSPRNG/PRF computational assumption used in the party implementation.",
           ],
           "proof_components": {
               "bracket": "For s<M, M>=66 and s>=65. Integer qL/qH rounding leaves deviation at least 4*sqrt(s)-1/2; Hoeffding exponent exceeds 31 for s<=ceil(8*sqrt(2,000,000)), so each hypergeometric tail <=exp(-31); s=M is exact.",
               "U_cap": "For M>4096, x=M^1/4>8. A fixed length-c window has mean >=64x; qH-qL+1 <=24x+4 <32x, so its lower-tail threshold is <=half the mean; lower-tail <=exp(-8x), union over <=M windows.",
               "W_cap": "For M>64, a cap violation contains a V-free block of at least 4 sqrt(M) positions; each block is missed with probability <=exp(-32), union over <=M blocks.",
           },
           "rows": rows}
    print(json.dumps(out, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
