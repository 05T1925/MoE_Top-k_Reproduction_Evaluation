"""Paper-literal finite-parameter audit; this is not a Select implementation.

It emits the exact source inconsistencies and conditional size/cost arithmetic
for a requested (n, k).  It deliberately does not repair Algorithm 5/7,
compare data, call the frozen oracle, retry, or produce a selected element.
"""

from __future__ import annotations

import argparse
from fractions import Fraction
import json
from typing import Any


def ceil_nth_root_power(n: int, numerator: int, denominator: int) -> int:
    """Return ceil(n**(numerator/denominator)) using integer arithmetic."""
    if n < 1 or numerator < 0 or denominator < 1:
        raise ValueError("invalid power arguments")
    target = n**numerator
    low, high = 0, n + 1
    while low < high:
        middle = (low + high) // 2
        if middle**denominator >= target:
            high = middle
        else:
            low = middle + 1
    return low


def parse_epsilon(value: str) -> Fraction:
    try:
        epsilon = Fraction(value)
    except (ValueError, ZeroDivisionError) as exc:
        raise argparse.ArgumentTypeError("epsilon must be a rational such as 1/36") from exc
    if not Fraction(0) < epsilon < Fraction(1, 18):
        raise argparse.ArgumentTypeError("Theorem 8 applies only to 0 < epsilon < 1/18")
    return epsilon


def select_partition_expansion(n: int, project_k_highest: int) -> dict[str, int]:
    """Map project Kth-highest to Appendix A's ascending kth-select reduction.

    Each Partition call receives n real items plus one outer sentinel.  The
    project order is descending priority, while the Appendix A derivation is
    stated for an ascending rank.  Therefore r=n-K+1 before adding the outer
    sentinel.  The same one-sided padding on both sentinel runs then makes the
    target lie at the median cut in each padded instance.
    """
    if n < 1 or not 1 <= project_k_highest <= n:
        raise ValueError("require n >= 1 and 1 <= k <= n")
    ascending_rank = n - project_k_highest + 1
    outer_n = n + 1
    if 2 * ascending_rank <= outer_n:
        low_padding = outer_n - 2 * ascending_rank
        high_padding = 0
    else:
        low_padding = 0
        high_padding = 2 * ascending_rank - outer_n
    median_input_n = outer_n + low_padding + high_padding
    median_rank = ascending_rank + low_padding
    if median_input_n % 2 or median_rank != median_input_n // 2:
        raise AssertionError("median-padding identity failed")
    return {
        "outer_partition_n": outer_n,
        "project_K_highest_1_based": project_k_highest,
        "equivalent_ascending_rank_1_based": ascending_rank,
        "low_padding": low_padding,
        "high_padding": high_padding,
        "median_input_n": median_input_n,
        "median_rank_1_based": median_rank,
    }


def algorithm5_literal_parameters(input_n: int) -> dict[str, Any]:
    """Expose the Algorithm 5 r=1 line-4/line-11 incompatibility."""
    if input_n < 2 or input_n % 2:
        raise ValueError("audit witness expects an even A5 input size >= 2")
    skeleton_n = ceil_nth_root_power(input_n, 2, 3)
    pivot_n = ceil_nth_root_power(input_n, 1, 3)
    literal_k1 = input_n // 2
    # A strict-order item can beat at most |A1|-1 distinct members of A1.
    max_beatable_in_a1 = skeleton_n - 1
    return {
        "input_n": input_n,
        "r": 1,
        "S1_size_project_ceil": skeleton_n,
        "T1_size_project_ceil": pivot_n,
        "Algorithm5_line4_literal_k1": literal_k1,
        "max_distinct_A1_items_any_pivot_can_beat": max_beatable_in_a1,
        "line11_b_exists_possible": literal_k1 <= max_beatable_in_a1,
        "classification": (
            "UNDEFINED_LITERAL_A5_B_PIVOT"
            if literal_k1 > max_beatable_in_a1
            else "PARAMETER_BOUND_ALONE_DOES_NOT_DECIDE"
        ),
    }


def algorithm7_ideal_partition_trace(m: int, inner_dummy_side_count: int) -> dict[str, int]:
    """Count Algorithm 7's sets after two exact median partitions.

    The two A5 calls each see m base elements and 2D strict low/high dummies.
    This is a set-cardinality witness, not an execution or random sample.
    """
    d = inner_dummy_side_count
    if m < 2 or m % 2 or d < 1:
        raise ValueError("require even m >= 2 and D >= 1")
    augmented_n = m + 2 * d
    half = augmented_n // 2
    r1_base = half - 2 * d
    a1_base = m - r1_base
    a2_base = half - 2 * d
    r2_base = half
    u_size = 2 * d
    source_rank = m // 2 - r2_base
    residual_reject_rank = m // 2 - r1_base
    return {
        "base_n": m,
        "inner_dummy_side_count_D": d,
        "each_A5_input_n": augmented_n,
        "exact_half_cut": half,
        "R1_base_size": r1_base,
        "A1_base_size": a1_base,
        "A2_base_size": a2_base,
        "R2_base_size": r2_base,
        "U_size": u_size,
        "inner_dummy_cardinality_feasible": 2 * d <= m,
        "Algorithm7_line11_source_rank_n_over_2_minus_R2": source_rank,
        "rank_from_rejected_prefix_invariant_n_over_2_minus_R1": residual_reject_rank,
        "U_half": u_size // 2,
    }


def build_audit(n: int, k: int, epsilon: Fraction) -> dict[str, Any]:
    expansion = select_partition_expansion(n, k)
    m = expansion["median_input_n"]
    alpha = Fraction(2, 3) + epsilon
    d = ceil_nth_root_power(m, alpha.numerator, alpha.denominator)
    a5_n = m + 2 * d
    a5 = algorithm5_literal_parameters(a5_n)
    trace = algorithm7_ideal_partition_trace(m, d)
    u = trace["U_size"]
    sample_request = Fraction(m, u)
    finite_structure_feasible = (
        trace["inner_dummy_cardinality_feasible"]
        and trace["R1_base_size"] >= 0
        and trace["A1_base_size"] >= 0
        and trace["A2_base_size"] >= 0
        and trace["R2_base_size"] >= 0
        and u <= m
    )
    candidate_v = min(u, (m + u - 1) // u) if finite_structure_feasible else None
    candidate_w_cutoff = ceil_nth_root_power(
        m,
        (Fraction(1, 3) + 3 * epsilon).numerator,
        (Fraction(1, 3) + 3 * epsilon).denominator,
    )
    a5_round1_upper = a5["S1_size_project_ceil"] * a5["T1_size_project_ceil"]
    a5_round2_calls = a5_n
    r4_exponent = Fraction(2, 3) + 6 * epsilon
    return {
        "label": "BMW16_DERIVED_SELECT_4R_TEST_ONLY_AUDIT_NOT_IMPLEMENTATION",
        "complete_select_implemented": False,
        "algorithm_seed": None,
        "randomized_algorithm_executed": False,
        "source_parameters": {"epsilon": str(epsilon), "epsilon_interval": "(0, 1/18)"},
        "request": {
            "n": n,
            "K_highest_1_based": k,
            "project_order": "score descending, original_index ascending",
        },
        "appendix_a_partition_reduction": expansion,
        "per_partition_algorithm7_candidate": {
            "given_input_n": m,
            "inner_dummy_count_per_side_project_ceil": d,
            "A5_input_n_after_inner_dummies": a5_n,
            "Algorithm5_literal_r1_parameters": a5,
            "exact_first_two_round_cardinality_witness": trace,
            "ideal_cardinality_witness_feasible": finite_structure_feasible,
            "paper_V_size_expression": f"{m}/{u} = {sample_request}",
            "paper_V_size_is_integer": sample_request.denominator == 1,
            "candidate_project_v_size_ceil_then_bound_by_u": candidate_v,
            "candidate_R3_calls_per_algorithm7": candidate_v * u if candidate_v is not None else None,
            "candidate_W_cutoff_ceil": candidate_w_cutoff,
            "candidate_R4_ordered_pair_requests_per_algorithm7_upper_bound": candidate_w_cutoff * (candidate_w_cutoff - 1),
            "candidate_R4_unique_unordered_edges_per_algorithm7": candidate_w_cutoff * (candidate_w_cutoff - 1) // 2,
            "conditional_logical_call_upper_bounds_for_two_parallel_A7_instances": {
                "R1_four_A5_requests": 4 * a5_round1_upper,
                "R2_four_A5_requests": 4 * a5_round2_calls,
                "R3_two_A7_requests": 2 * candidate_v * u if candidate_v is not None else None,
                "R4_two_A7_ordered_pair_request_upper_bound": 2 * candidate_w_cutoff * (candidate_w_cutoff - 1),
                "R4_two_A7_unique_unordered_edge_upper_bound": candidate_w_cutoff * (candidate_w_cutoff - 1),
                "actual_unique_comparison_calls": "NOT_MEASURED_NO_ALGORITHM_EXECUTION",
                "real_real_sentinel_duplicate_breakdown": "NOT_MEASURED_NO_SAMPLED_EDGE_LIST",
            },
        },
        "conditional_cost_exponents": {
            "A5_skeleton_times_pivots_exponent": "1",
            "A5_round2_exponent": "1",
            "A7_R3_with_explicit_integer_sampling_rule": "1",
            "A7_R4_pairwise_exponent": str(r4_exponent),
            "R4_exponent_less_than_1": r4_exponent < 1,
            "interpretation": "conditional on a repaired, fully specified A7 schedule",
        },
        "gates": {
            "PAPER_SOURCE_AND_REDUCTION": "PARTIAL_SOURCE_VERIFIED_K_ALGEBRA_CLOSED_BMW16_STEPS_UNRESOLVED",
            "FOUR_ROUND_SCHEDULE": "CONDITIONAL_ONLY_NO_RUNNABLE_SELECT",
            "LINEAR_COMPARISONS": "CONDITIONAL_ASYMPTOTIC_BOUND_ONLY",
            "FINITE_N_RUNNABLE_REFERENCE": "NO_GO_SOURCE_STEPS_UNRESOLVED",
            "STABLE_KTH_ORACLE": "AVAILABLE_FOR_VALIDATION_ONLY",
            "PROJECT_EXACT_MASK": "NOT_IMPLEMENTED",
            "SECURE_PROTOCOL_I_COMPOSITION": "DESIGN_LIMITS_DOCUMENTED_NOT_REVIEWED",
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--n", type=int, default=128)
    parser.add_argument("--k", type=int, default=2)
    parser.add_argument("--epsilon", type=parse_epsilon, default=Fraction(1, 36))
    args = parser.parse_args()
    print(json.dumps(build_audit(args.n, args.k, args.epsilon), indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
