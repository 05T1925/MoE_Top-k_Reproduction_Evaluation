#!/usr/bin/env python3
"""Exact finite moment-bound calculator for PROJECT_DERIVED_SELECT_5ITER.

All outputs are mathematical bounds/diagnostics, not measured performance.
"""
from __future__ import annotations

import argparse
import itertools
import json
import math
from fractions import Fraction
from functools import lru_cache
from pathlib import Path


def ceil_sqrt(x: int) -> int:
    q = math.isqrt(x)
    return q + (q * q != x)


def ceil_fourth_root(x: int) -> int:
    q = math.isqrt(math.isqrt(x))
    return q + (q**4 < x)


def layer_counts(m: int) -> tuple[int, int, int, int]:
    if m <= 1:
        return 0, 0, 0, 0
    s = ceil_sqrt(m)
    sample_clique = s * (s - 1) // 2
    pivot_stars = 2 * (m - s)
    return s, sample_clique, pivot_stars, sample_clique + pivot_stars


def moment_envelope(m: int, n: int) -> Fraction:
    """Uniform exact E[M_next^2 | m,r] upper bound for all residual ranks r."""
    if m <= 2:
        return Fraction(0)
    s = ceil_sqrt(m)
    b = n.bit_length()
    a = ceil_sqrt(2 * s * b)
    d = min(s - 1, 2 * a + 3)
    S = s + 1
    N = m - s
    # Gap G=X_qh-X_ql-1 is increasing in d. Its exact first two moments
    # follow the uniform-composition spacings representation of a uniform s-subset.
    gap_mean = Fraction(d * (m + 1), S) - 1
    gap_var = Fraction(N * d * (S - d) * (m + 1), S * S * (S + 1))
    gap_second = gap_mean * gap_mean + gap_var
    endpoint_mean = Fraction(N, S)
    endpoint_var = Fraction(N * s * (m + 1), S * S * (S + 1))
    endpoint_second = endpoint_mean * endpoint_mean + endpoint_var
    # Conservatively include both endpoint cases and both one-sided tails.
    # Each non-boundary tail is <= n^-4 by one-sided Hoeffding and a^2>=2*s*b.
    tails = Fraction(2 * m * m, n**4)
    return gap_second + 2 * endpoint_second + tails


@lru_cache(maxsize=None)
def finite_c(n: int) -> Fraction:
    """Exact finite maximum C_n with envelope(m,n)<=C_n*m*floor(sqrt(m))."""
    best = Fraction(0)
    for m in range(3, n + 1):
        candidate = moment_envelope(m, n) / (m * math.isqrt(m))
        if candidate > best:
            best = candidate
    return best


def ceil_fraction_mul(frac: Fraction, value: int) -> int:
    return (frac.numerator * value + frac.denominator - 1) // frac.denominator


def expected_dp(n: int, k: int) -> Fraction:
    """Exact expected comparisons using explicit finite sample-subset recursion."""
    @lru_cache(maxsize=None)
    def value(layer: int, m: int, rank: int) -> Fraction:
        if m <= 1:
            return Fraction(0)
        if layer == 5:
            return Fraction(m * (m - 1), 2)
        s = min(m, ceil_sqrt(m))
        a = ceil_sqrt(2 * s * n.bit_length())
        mu = Fraction(s * (rank - 1), m)
        ql = min(s - 1, max(1, mu.numerator // mu.denominator - a))
        mu_ceil = (mu.numerator + mu.denominator - 1) // mu.denominator
        qh = min(s, max(ql + 1, mu_ceil + a + 1))
        edges = s * (s - 1) // 2 + 2 * (m - s)
        subsets = list(itertools.combinations(range(1, m + 1), s))
        continuation = Fraction(0)
        for subset in subsets:
            x, y = subset[ql - 1], subset[qh - 1]
            if x == rank or y == rank:
                continue
            if x < rank < y:
                next_state = (y - x - 1, rank - x)
            elif y < rank:
                next_state = (m - y, rank - y)
            elif x > rank:
                next_state = (x - 1, rank)
            else:
                raise AssertionError("uncovered exact-DP transition")
            continuation += value(layer + 1, *next_state)
        return Fraction(edges) + continuation / len(subsets)
    return value(1, n, k)


def brute_expected(n: int, k: int) -> Fraction:
    """Unmemoized path enumeration, independently checking the DP for n<=6."""
    def walk(layer: int, m: int, rank: int) -> Fraction:
        if m <= 1:
            return Fraction(0)
        if layer == 5:
            return Fraction(m * (m - 1), 2)
        s = min(m, ceil_sqrt(m))
        a = ceil_sqrt(2 * s * n.bit_length())
        mu = Fraction(s * (rank - 1), m)
        ql = min(s - 1, max(1, mu.numerator // mu.denominator - a))
        mu_ceil = (mu.numerator + mu.denominator - 1) // mu.denominator
        qh = min(s, max(ql + 1, mu_ceil + a + 1))
        edges = s * (s - 1) // 2 + 2 * (m - s)
        values = []
        for subset in itertools.combinations(range(1, m + 1), s):
            x, y = subset[ql - 1], subset[qh - 1]
            if x == rank or y == rank:
                values.append(Fraction(edges))
            elif x < rank < y:
                values.append(Fraction(edges) + walk(layer + 1, y - x - 1, rank - x))
            elif y < rank:
                values.append(Fraction(edges) + walk(layer + 1, m - y, rank - y))
            elif x > rank:
                values.append(Fraction(edges) + walk(layer + 1, x - 1, rank))
            else:
                raise AssertionError("uncovered brute transition")
        return sum(values, Fraction(0)) / len(values)
    return walk(1, n, k)


def bound_row(n: int, k: int) -> dict:
    c = finite_c(n)
    n2 = n * n
    B = n2
    moments = []
    for _ in range(4):
        root = ceil_fourth_root(B**3)
        B = min(n2, ceil_fraction_mul(c, root))
        moments.append(B)
    exact_first = layer_counts(n)[3]
    layer_expected_uppers = [exact_first]
    for i, previous in enumerate(moments[:3], 1):
        mmax = max(1, n - 2 * i)
        layer_expected_uppers.append(min(3 * ceil_sqrt(previous), layer_counts(mmax)[3]))
    m5max = max(0, n - 8)
    final_upper = min((moments[3] + 1) // 2, m5max * (m5max - 1) // 2)
    total_upper = sum(layer_expected_uppers) + final_upper
    deterministic = []
    for i in range(4):
        mmax = max(1, n - 2 * i)
        deterministic.append({"iteration": i + 1, "m_before_max": mmax,
                               "sample_size_max": ceil_sqrt(mmax),
                               "sample_clique_edges_max": layer_counts(mmax)[1],
                               "pivot_star_edges_max": layer_counts(mmax)[2],
                               "total_edges_max": layer_counts(mmax)[3],
                               "candidate_after_max": max(0, n - 2 * (i + 1))})
    m5 = max(0, n - 8)
    deterministic.append({"iteration": 5, "candidate_before_max": m5,
                          "exact_clique_edges_max": m5 * (m5 - 1) // 2})
    return {"n": n, "K": k, "bit_length_n": n.bit_length(),
            "finite_C_n_exact_fraction": f"{c.numerator}/{c.denominator}",
            "finite_C_n_decimal": float(c),
            "candidate_second_moment_upper_B_after_iterations": moments,
            "expected_layer_comparison_upper": layer_expected_uppers + [final_upper],
            "expected_total_comparisons_lower": exact_first,
            "expected_total_comparisons_upper": total_upper,
            "deterministic_per_iteration_maxima": deterministic,
            "worst_case_total_upper": sum(x["total_edges_max"] for x in deterministic[:4]) + deterministic[4]["exact_clique_edges_max"],
            "logical_calls_are_per_layer_calls; cross-layer repeats count again": True,
            "security_material_slots_dcf_evals_bytes": "NOT_MEASURED"}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path)
    parser.add_argument("--exact-small-max", type=int, default=6)
    args = parser.parse_args()
    cross_checks = []
    for n in range(2, args.exact_small_max + 1):
        for k in range(1, n + 1):
            dp = expected_dp(n, k)
            brute = brute_expected(n, k)
            if dp != brute:
                raise AssertionError(f"DP/brute mismatch n={n},K={k}: {dp} != {brute}")
            cross_checks.append({"n": n, "K": k, "exact_expected_fraction": f"{dp.numerator}/{dp.denominator}",
                                 "decimal": float(dp), "dp_matches_brute_path_enumeration": True})
    matrix = [(128, 2), (128, 8), (256, 2), (256, 8),
              (1000, 80), (10_000, 80), (100_000, 80), (1_000_000, 80)]
    output = {"label": "PROJECT_DERIVED_SELECT_5ITER", "status": "NON_FORMAL_COST_ANALYSIS",
              "exact_small_dp_brute_crosschecks": cross_checks,
              "frozen_matrix_bound_rows": [bound_row(n, k) for n, k in matrix],
              "expected_cost_claim": "Strict finite moment upper bounds are reported per matrix point. Analytically, fixed four sample/pivot reductions give E[M_{i+1}^2|M_i=m]=O(m^(3/2) log n), hence E[M4^2]=O(n^(2*(3/4)^4) polylog n), final clique O(n^(2*(3/4)^4) polylog n), and total expected logical comparisons O(n). This is a project-derived bound, not BB90/BMW16 or measured performance.",
              "probability_of_wrong_output": 0, "probability_abort_on_legal_input": 0,
              "resource_failure_contract": "error/no answer; no retry or fallback"}
    raw = json.dumps(output, indent=2) + "\n"
    if args.out:
        args.out.write_text(raw, encoding="utf-8")
    print(raw, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
