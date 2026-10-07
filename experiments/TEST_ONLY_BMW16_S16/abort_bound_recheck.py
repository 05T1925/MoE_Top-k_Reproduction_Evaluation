#!/usr/bin/env python3
"""Independent finite-domain recomputation of the S15 ideal-sampler bound.

The probability formulas are those in the frozen S15 proof; this checker
recomputes integer thresholds and sweeps every reachable even M. It does not
model the SHA-256 counter sampler's computational advantage.
"""
from __future__ import annotations

import json
import math


def ceil_root(value: int, power: int) -> int:
    lo, hi = 0, 1
    while hi**power < value:
        hi *= 2
    while lo + 1 < hi:
        mid = (lo + hi) // 2
        if mid**power >= value:
            hi = mid
        else:
            lo = mid
    return hi


def per_partition(M: int) -> tuple[float, float, float]:
    if M < 2 or M % 2:
        raise ValueError("M must be reachable and even")
    s = min(M, ceil_root(64 * M, 2))
    bracket = 0.0 if s == M else 2.0 * math.exp(-31.0)
    u_tail = 0.0 if M <= 4096 else M * math.exp(-8.0 * M**0.25)
    w_cap = 2 * ceil_root(4 * M, 2) + 1
    w_tail = 0.0 if M <= 64 else M * math.exp(-32.0)
    return bracket, u_tail, w_tail


def combined(M: int) -> float:
    return 2.0 * sum(per_partition(M))


def sweep(max_M: int) -> dict[str, float | int]:
    # M=2*max(K,n-K+1); K=1 makes every even M in [2, 2n] reachable.
    best_M = -1
    best = -1.0
    for M in range(2, max_M + 1, 2):
        value = combined(M)
        if value > best:
            best_M, best = M, value
    return {"max_M": max_M, "reachable_even_M_count": max_M // 2,
            "maximizer_M": best_M, "max_bound": best}


def main() -> None:
    domains = {"n_le_256": sweep(512), "n_le_1e6_formula_only": sweep(2_000_000)}
    report = {
        "label": "BMW16_S16_INDEPENDENT_ABORT_BOUND_RECHECK_IDEAL_SAMPLER",
        "source_formulas": "Frozen S15 decision, Section 4; no BMW16 Theorem 8 claim.",
        "sampler_model": "Uniform independent-without-replacement samples; SHA-256 PRF computational assumption is separate and excluded.",
        "per_partition_bounds": {
            "bracket": "0 if s=M, otherwise 2 exp(-31)",
            "U_cap": "0 if U_cap=M, otherwise M exp(-8 M^(1/4))",
            "W_cap": "0 if M<=64 or W_cap>=M, otherwise M exp(-32)",
            "two_tasks": "2*(bracket+U_cap+W_cap), union bound; task independence not needed",
        },
        "monotonicity_check": {
            "M<=65": "all bracket/U/W terms are zero",
            "66<=M<=4096": "bracket is constant and W term increases; U term is zero",
            "M>4096": "write x=M^(1/4)>8; derivative of x^4*(exp(-8x)+exp(-32)) is positive iff (2x-1)exp(-8x)<exp(-32). The left side decreases for x>8 and is at most 15 exp(-64)<exp(-32).",
            "S15_comment_correction": "The frozen checker comment compared against exp(-31); the sufficient derivative condition is the stronger exp(-32). The inequality follows directly from x>8, so the reported endpoint maxima remain valid.",
        },
        "domains": domains,
        "limits": [
            "This checks the mathematical expression over every reachable even M, not correctness of all proof premises.",
            "It does not establish a statistical bound for the implementation's CSPRNG/PRF stream.",
            "The n<=1e6 row is formula-only; the current party API is capped at n<=256.",
        ],
    }
    print(json.dumps(report, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
