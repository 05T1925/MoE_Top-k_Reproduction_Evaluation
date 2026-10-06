#!/usr/bin/env python3
"""TEST_ONLY arithmetic audit for the S6 BMW16-derived composition review.

This file does not implement Select, FSS, a shuffle, or Protocol I.  It checks
stable-key algebra, sentinel/dummy code ranges, edge-pool coverage formulae,
and serialized-capacity arithmetic without allocating any material pool.
"""

from __future__ import annotations

import argparse
import hashlib
import itertools
import json
import math
import platform
import sys
from pathlib import Path
from typing import Any


LABEL = "BMW16_DERIVED_SELECT_PROTOCOL_I_COMPOSITION_REVIEW_TEST_ONLY"
U64_MAX = (1 << 64) - 1
INT32_MIN = -(1 << 31)
INT32_MAX = (1 << 31) - 1
CURRENT_UCMP_MIN_BITS = 34
CURRENT_UCMP_MAX_BITS = 53
CURRENT_PARTY_PACKAGE_MAX_EDGES = 1_000_000
CURRENT_PROTOCOL_I_MAX_PADDED_N = 1_048_576
CURRENT_UCMP_SERIALIZED_BYTES = lambda bits: 57 + 24 * bits
S5_ACCOUNTED_BYTES_PER_PARTY_SLOT = lambda bits: 97 + 24 * bits


def ceil_log2(n: int) -> int:
    if n < 1:
        raise ValueError("n must be positive")
    return (n - 1).bit_length()


def ceil_sqrt(n: int) -> int:
    root = math.isqrt(n)
    return root if root * root == n else root + 1


def next_power_of_two(n: int) -> int:
    if n < 1:
        raise ValueError("domain must be positive")
    return 1 << (n - 1).bit_length()


def choose2(n: int) -> int:
    return n * (n - 1) // 2


def checked_u64(value: int, label: str) -> int:
    if value < 0 or value > U64_MAX:
        raise OverflowError(f"{label} does not fit uint64: {value}")
    return value


def median_geometry(n: int, k: int) -> dict[str, int]:
    if n < 1 or not 1 <= k <= n:
        raise ValueError("require n >= 1 and 1 <= K <= n")
    target_real_rank = n - k + 1  # one based in S4's low-to-high real order
    m = 2 * max(target_real_rank, k)
    half = m // 2
    low_pad = half - target_real_rank
    high_pad = half - k
    if low_pad < 0 or high_pad < 0 or low_pad + high_pad != m - n - 1:
        raise AssertionError("sentinel/padding cardinality identity failed")
    # S4 shares real handles across the two instances, while its current
    # plaintext builder allocates separate pads and a separate sentinel to
    # each instance.  This source-faithful union is n+2*(M-n)=2M-n handles.
    # A PROJECT_DERIVED compact layout could share the L/H pad handles and use
    # M+1, but that optimization is not assumed in the conservative pool.
    union_handles = 2 * m - n
    compact_shared_pad_union = m + 1
    storage_domain = next_power_of_two(union_handles)
    return {
        "n": n,
        "K": k,
        "target_real_rank_1based": target_real_rank,
        "median_input_size_M": m,
        "half_h": half,
        "low_pad_L": low_pad,
        "high_pad_H": high_pad,
        "source_faithful_two_instance_union_handles_2M_minus_n": union_handles,
        "compact_shared_pad_union_handles_M_plus_1_project_extension": compact_shared_pad_union,
        "power_of_two_storage_domain_D": storage_domain,
        "storage_only_padding_D_minus_M_plus_1": storage_domain - union_handles,
    }


def stable_reverse_priority_key(score: int, original_index: int, n: int) -> tuple[int, int]:
    if not INT32_MIN <= score <= INT32_MAX:
        raise ValueError("score must be signed int32 raw Q20.12")
    if n < 1 or not 0 <= original_index < n:
        raise ValueError("invalid n/original_index")
    b = max(1, ceil_log2(n))
    raw = score & 0xFFFFFFFF
    sign_biased = raw ^ 0x80000000
    r = (sign_biased << b) + (n - 1 - original_index)
    r_max = ((1 << 32) - 1) * (1 << b) + (n - 1)
    # Protocol I's existing ascending priority is high-score-first, then
    # smaller-index-first.  S4's median order is its exact reverse.
    priority = (((0xFFFFFFFF - sign_biased) << b) | original_index)
    if r != r_max - priority:
        raise AssertionError("reverse-priority identity failed")
    return r, r_max


def width_geometry(n: int, k: int) -> dict[str, int | bool]:
    g = median_geometry(n, k)
    b = max(1, ceil_log2(n))
    r_max = ((1 << 32) - 1) * (1 << b) + (n - 1)
    # A single common code space: low pads < low sentinel < all reals < high
    # sentinel < high pads.  Each partition instance activates only its own
    # sentinel; the other sentinel's code reserves a disjoint gap.
    max_code = r_max + g["median_input_size_M"] - n + 1
    bits = 33 + b
    if max_code < 0 or max_code >= (1 << bits):
        raise AssertionError("derived key width does not cover maximum code")
    return {
        **g,
        "index_bits_b": b,
        "real_key_min": 0,
        "real_key_max_Rmax": r_max,
        "encoded_union_max_code": max_code,
        "derived_comparison_bits_33_plus_b": bits,
        "uCMP_34_to_53_supported": CURRENT_UCMP_MIN_BITS <= bits <= CURRENT_UCMP_MAX_BITS,
        "S5_tagged_comparison_bits_36_plus_ceil_log2_n": 36 + ceil_log2(n),
    }


def class_code_layout(n: int, k: int, scores: list[int]) -> tuple[list[tuple[str, int]], dict[str, Any]]:
    g = median_geometry(n, k)
    low, high = g["low_pad_L"], g["high_pad_H"]
    b = max(1, ceil_log2(n))
    rmax = ((1 << 32) - 1) * (1 << b) + (n - 1)
    real_items = []
    for i, score in enumerate(scores):
        r, actual_rmax = stable_reverse_priority_key(score, i, n)
        if actual_rmax != rmax:
            raise AssertionError("inconsistent Rmax")
        real_items.append((f"real:{i}", low + 1 + r))
    low_pads = [(f"low-pad:{j}", j) for j in range(low)]
    low_sentinel = ("low-sentinel", low)
    high_sentinel = ("high-sentinel", low + rmax + 2)
    high_pads = [(f"high-pad:{j}", low + rmax + 3 + j) for j in range(high)]
    real_items.sort(key=lambda item: item[1])
    low_instance = low_pads + [low_sentinel] + real_items + high_pads
    high_instance = low_pads + real_items + [high_sentinel] + high_pads
    if len(low_instance) != g["median_input_size_M"] or len(high_instance) != g["median_input_size_M"]:
        raise AssertionError("each S4 instance must have exactly M items")
    max_code = max(code for _, code in low_instance + high_instance)
    return low_instance + high_instance, {
        "M": g["median_input_size_M"],
        "L": low,
        "H": high,
        "Rmax": rmax,
        "max_code": max_code,
        "low_instance_codes": [code for _, code in low_instance],
        "high_instance_codes": [code for _, code in high_instance],
        "real_codes_by_original_index": [dict(real_items)[f"real:{i}"] for i in range(n)],
    }


def assert_code_order(n: int, k: int, scores: list[int]) -> None:
    _, data = class_code_layout(n, k, scores)
    for name in ("low_instance_codes", "high_instance_codes"):
        codes = data[name]
        if any(left >= right for left, right in zip(codes, codes[1:])):
            raise AssertionError(f"{name} is not strictly increasing")
    # For this reverse-priority real-key order, the top-K threshold is the
    # (n-K+1)-th item from the low end; membership is R >= threshold.
    real_codes = data["real_codes_by_original_index"]
    for left in range(n):
        for right in range(n):
            if left == right:
                continue
            semantic_precedes = (scores[left] < scores[right]) or (
                scores[left] == scores[right] and left > right
            )
            if (real_codes[left] < real_codes[right]) != semantic_precedes:
                raise AssertionError("real-key order differs from S4 reverse stable order")
    threshold = sorted(real_codes)[n - k]
    got = [i for i, value in enumerate(real_codes) if value >= threshold]
    expected = sorted(range(n), key=lambda i: (-scores[i], i))[:k]
    if set(got) != set(expected) or len(got) != k:
        raise AssertionError(f"stable Top-K threshold mismatch: got={got}, expected={expected}")
    bits = 33 + max(1, ceil_log2(n))
    expected_max = data["Rmax"] + data["M"] - n + 1
    if data["max_code"] != expected_max:
        raise AssertionError("sentinel/dummy maximum-code formula mismatch")
    if data["max_code"] >= (1 << bits):
        raise AssertionError("class code overflows declared width")


def enumerate_key_conformance() -> dict[str, int]:
    vectors = 0
    topk_cases = 0
    alphabet = (-1, 0, 1)
    for n in range(1, 7):
        for scores in itertools.product(alphabet, repeat=n):
            vectors += 1
            for k in range(1, n + 1):
                assert_code_order(n, k, list(scores))
                topk_cases += 1
    boundary_alphabet = (INT32_MIN, -1, 0, 1, INT32_MAX)
    boundary_vectors = 0
    for n in range(1, 5):
        for scores in itertools.product(boundary_alphabet, repeat=n):
            boundary_vectors += 1
            for k in range(1, n + 1):
                assert_code_order(n, k, list(scores))
                topk_cases += 1
    # Explicit signed endpoints and all-equal vectors at all requested K edges.
    targeted = [
        [INT32_MIN, INT32_MAX],
        [INT32_MAX, INT32_MIN, INT32_MAX, INT32_MIN],
        [0] * 8,
        [INT32_MIN] * 8,
        [INT32_MAX] * 8,
    ]
    for scores in targeted:
        for k in range(1, len(scores) + 1):
            assert_code_order(len(scores), k, scores)
            topk_cases += 1
    return {
        "exhaustive_ternary_vectors_n_le_6": vectors,
        "exhaustive_boundary_vectors_n_le_4": boundary_vectors,
        "topk_kth_cases_checked": topk_cases,
    }


def enumerate_r2_coverage() -> dict[str, int]:
    sample_subsets = 0
    pivot_pair_cases = 0
    for domain in range(2, 9):
        for sample_size in range(2, domain + 1):
            for sample in itertools.combinations(range(domain), sample_size):
                sample_subsets += 1
                covered: set[tuple[int, int]] = set()
                covered_directed: set[tuple[int, int]] = set()
                for pivots in itertools.combinations(sample, 2):
                    pivot_pair_cases += 1
                    for pivot in pivots:
                        for item in range(domain):
                            if item != pivot:
                                covered.add(tuple(sorted((pivot, item))))
                                # S4 R2 emits item -> pivot. If both endpoints
                                # are sample pivots, both orientations occur.
                                covered_directed.add((item, pivot))
                expected = {
                    (left, right)
                    for left in range(domain)
                    for right in range(left + 1, domain)
                    if left in sample or right in sample
                }
                formula = sample_size * domain - choose2(sample_size + 1)
                if covered != expected or len(covered) != formula:
                    raise AssertionError("fixed-sample R2 union formula/coverage mismatch")
                expected_directed = {
                    (item, pivot)
                    for pivot in sample
                    for item in range(domain)
                    if item != pivot
                }
                directed_formula = sample_size * (domain - 1)
                if covered_directed != expected_directed or len(covered_directed) != directed_formula:
                    raise AssertionError("fixed-sample directed R2 union formula/coverage mismatch")
                if choose2(sample_size) != len(list(itertools.combinations(sample, 2))):
                    raise AssertionError("R1 sample clique count mismatch")
    return {
        "sample_subsets_exhausted": sample_subsets,
        "possible_pivot_pair_sets_exhausted": pivot_pair_cases,
        "r2_union_formula": "s*D - C(s+1,2)",
        "r2_directed_union_formula": "s*(D-1): item->pivot; sample-sample edges include both orientations",
        "r1_formula": "C(s,2)",
        "r3_r4_full_pool": "D*(D-1) ordered slots per round and instance; C(D,2) only with a proven reverse-share complement transform",
    }


def capacity_row(n: int, k: int) -> dict[str, Any]:
    g = median_geometry(n, k)
    m = g["median_input_size_M"]
    d = g["power_of_two_storage_domain_D"]
    old_b = 36 + ceil_log2(n)  # S5's 5-class tagged-width estimate
    new_b = 33 + max(1, ceil_log2(n))

    def pools(universe: int, algorithm_size: int) -> tuple[int, int, int, int, int]:
        s = min(algorithm_size, ceil_sqrt(64 * algorithm_size))
        canonical = 8 * choose2(universe) + choose2(n)
        direction_safe = 8 * universe * (universe - 1) + n * (n - 1)
        fixed_sample_canonical = 2 * (
            choose2(s)
            + universe * s - choose2(s + 1)
            + 2 * choose2(universe)
        ) + choose2(n)
        # Without same-round caching/complement, reserve a one-shot key for
        # every possible ordered edge in every round and instance. R1 uses
        # all ordered sample pairs across possible sample orders; R2's union
        # is s*(D-1); R3/R4 use full ordered pair pools.
        fixed_sample_directed = 2 * (
            s * (s - 1)
            + s * (universe - 1)
            + 2 * universe * (universe - 1)
        ) + n * (n - 1)
        return s, canonical, direction_safe, fixed_sample_canonical, fixed_sample_directed

    s5, s5_reported_canonical, s5_direction_safe, s5_fixed_canonical, s5_fixed_directed = pools(m, m)
    s6, s6_storage_canonical, s6_storage_directed, s6_fixed_canonical, s6_fixed_directed = pools(d, m)
    # Complete pool over the exact common logical handle union, before adding
    # power-of-two-only inactive addresses.
    common_union = g["source_faithful_two_instance_union_handles_2M_minus_n"]
    compact_union = g["compact_shared_pad_union_handles_M_plus_1_project_extension"]
    common_union_pool = 8 * common_union * (common_union - 1) + n * (n - 1)
    common_union_canonical_pool = 8 * choose2(common_union) + choose2(n)
    compact_union_pool = 8 * compact_union * (compact_union - 1) + n * (n - 1)

    def byte_fields(bits: int, slots: int) -> dict[str, Any]:
        if not CURRENT_UCMP_MIN_BITS <= bits <= CURRENT_UCMP_MAX_BITS:
            return {
                "bits_supported": False,
                "ucmp_party_key_bytes_per_slot": None,
                "accounted_lower_bound_bytes_per_party_slot": None,
                "conservative_party_bytes": None,
                "fixed_sample_party_bytes": None,
            }
        per_key = CURRENT_UCMP_SERIALIZED_BYTES(bits)
        per_accounted = S5_ACCOUNTED_BYTES_PER_PARTY_SLOT(bits)
        return {
            "bits_supported": True,
            "ucmp_party_key_bytes_per_slot": per_key,
            "accounted_lower_bound_bytes_per_party_slot": per_accounted,
            "conservative_party_bytes": checked_u64(slots * per_accounted, "party capacity bytes"),
        }

    fields_old_canonical = byte_fields(old_b, s5_reported_canonical)
    fields_old_direction_safe = byte_fields(old_b, s5_direction_safe)
    fields_new = byte_fields(new_b, s6_storage_directed)
    fields_new_canonical = byte_fields(new_b, s6_storage_canonical)
    s5_fixed_canonical_bytes = byte_fields(old_b, s5_fixed_canonical)
    s5_fixed_direction_bytes = byte_fields(old_b, s5_fixed_directed)
    s6_fixed_bytes = byte_fields(new_b, s6_fixed_directed)
    s6_fixed_canonical_bytes = byte_fields(new_b, s6_fixed_canonical)
    return {
        **g,
        "sample_s5_M": s5,
        "sample_s6_M_algorithm_D_storage_envelope_conditional": s6,
        "S5_tagged_bin": old_b,
        "S6_offset_bin": new_b,
        "S5_reported_canonical_edge_material_slots_conditional": checked_u64(s5_reported_canonical, "S5 reported canonical slots"),
        "S5_direction_safe_ordered_edge_material_slots": checked_u64(s5_direction_safe, "S5 direction-safe ordered slots"),
        "S5_fixed_sample_canonical_slots_conditional": checked_u64(s5_fixed_canonical, "S5 fixed sample canonical slots"),
        "S5_fixed_sample_direction_safe_slots_if_no_orientation_reuse": checked_u64(s5_fixed_directed, "S5 fixed sample directed slots"),
        "S6_common_logical_union_ordered_slots": checked_u64(common_union_pool, "common union ordered slots"),
        "S6_common_logical_union_canonical_slots_conditional": checked_u64(common_union_canonical_pool, "common union canonical slots"),
        "S6_compact_shared_pad_union_slots_project_extension": checked_u64(compact_union_pool, "compact union slots"),
        "S6_storage_domain_ordered_slots": checked_u64(s6_storage_directed, "S6 storage ordered slots"),
        "S6_storage_domain_canonical_slots_conditional": checked_u64(s6_storage_canonical, "S6 canonical slots"),
        "S6_fixed_sample_storage_domain_ordered_slots_conditional": checked_u64(s6_fixed_directed, "S6 fixed sample ordered slots"),
        "S6_fixed_sample_storage_domain_slots_canonical_conditional": checked_u64(s6_fixed_canonical, "S6 fixed sample canonical slots"),
        "S5_reported_canonical_capacity_conditional": fields_old_canonical,
        "S5_direction_safe_capacity_if_no_orientation_reuse": fields_old_direction_safe,
        "S5_fixed_sample_capacity_canonical_conditional": s5_fixed_canonical_bytes,
        "S5_fixed_sample_direction_safe_capacity_if_no_orientation_reuse": s5_fixed_direction_bytes,
        "S6_ordered_capacity": fields_new,
        "S6_canonical_capacity_conditional": fields_new_canonical,
        "S6_fixed_sample_capacity_ordered_conditional": s6_fixed_bytes,
        "S6_fixed_sample_capacity_canonical_conditional": s6_fixed_canonical_bytes,
        "one_round_clique_slots_M": choose2(m),
        "one_round_clique_slots_source_faithful_union_2M_minus_n": choose2(common_union),
        "one_round_clique_slots_compact_shared_pad_union": choose2(g["compact_shared_pad_union_handles_M_plus_1_project_extension"]),
        "one_round_clique_slots_storage_D": choose2(d),
        "one_round_directed_material_slots_storage_D": d * (d - 1),
        "current_party_package_edge_limit": CURRENT_PARTY_PACKAGE_MAX_EDGES,
        "one_round_storage_clique_exceeds_current_package_limit": choose2(d) > CURRENT_PARTY_PACKAGE_MAX_EDGES,
        "one_round_storage_directed_pool_exceeds_current_package_limit": d * (d - 1) > CURRENT_PARTY_PACKAGE_MAX_EDGES,
        "current_protocol_i_max_padded_domain": CURRENT_PROTOCOL_I_MAX_PADDED_N,
        "storage_D_fits_current_protocol_i_padded_domain": d <= CURRENT_PROTOCOL_I_MAX_PADDED_N,
        "estimated_generated_bytes": "NOT_MEASURED",
        "actual_disk_bytes": "NOT_MEASURED",
        "keygen_time": "NOT_MEASURED",
    }


def cost_bound_row(n: int, k: int) -> dict[str, Any]:
    g = median_geometry(n, k)
    m = g["median_input_size_M"]
    # Select executes over each M-item S4 instance; D is only an offline
    # address/storage envelope. Thus S4's finite bound uses M, not D.
    if m <= 4096:
        per_instance = {"R1": 42 * m, "R2": 2 * m, "R3": 2048 * m, "R4": 25 * m}
    else:
        per_instance = {"R1": 42 * m, "R2": 2 * m, "R3": 657 * m, "R4": 25 * m}
    combined = {round_name: 2 * value for round_name, value in per_instance.items()}
    total = sum(combined.values())
    return {
        "M": m,
        "per_instance_logical_comparison_call_upper_bounds": per_instance,
        "two_partition_instances_same_clock_upper_bounds": combined,
        "two_partition_logical_comparison_call_total_upper_bound": total,
        "linear_in_original_n_bound": total <= 8468 * n,
        "four_comparison_layers": 4,
        "actual_logical_calls": "NOT_RUN_IN_S6",
        "actual_true_true_calls": "NOT_RUN_IN_S6",
        "actual_dummy_related_calls": "NOT_RUN_IN_S6",
        "if_each_call_uses_current_uCMP_DCF_Evals_per_party": 2 * total,
        "if_each_call_uses_current_uCMP_DCF_Evals_both_parties": 4 * total,
        "actual_uCMP_DCF_Evals": "NOT_RUN_IN_S6",
        "actual_final_DCF_membership_Evals": "NOT_IMPLEMENTED",
        "membership_candidate_if_hidden_selected_star_is_available": {
            "logical_real_record_comparisons": n - 1,
            "current_uCMP_evalDCF_per_party": 2 * (n - 1),
            "current_uCMP_evalDCF_both_parties": 4 * (n - 1),
            "offline_unordered_real_pair_slot_universe": choose2(n),
            "status": "HYPOTHETICAL; hidden selected-handle key lookup/MUX is not implemented",
        },
        "membership_full_pair_then_secret_mux_fallback": {
            "logical_real_real_comparisons": choose2(n),
            "current_uCMP_evalDCF_per_party": 2 * choose2(n),
            "current_uCMP_evalDCF_both_parties": 4 * choose2(n),
            "status": "HYPOTHETICAL; quadratic online work, not an approved fallback",
        },
    }


def make_report() -> dict[str, Any]:
    key = enumerate_key_conformance()
    coverage = enumerate_r2_coverage()
    configurations = [
        (128, 2), (128, 8), (128, 1), (128, 128),
        (256, 2), (256, 8), (256, 1), (256, 256),
        (1000, 80), (1000, 1), (1000, 1000),
        (10_000, 1), (10_000, 10_000),
        (100_000, 1), (100_000, 100_000),
        (1_000_000, 1), (1_000_000, 1_000_000),
    ]
    widths = []
    for n, k in configurations:
        w = width_geometry(n, k)
        if w["uCMP_34_to_53_supported"] != (34 <= int(w["derived_comparison_bits_33_plus_b"]) <= 53):
            raise AssertionError("uCMP width boundary mismatch")
        widths.append(w)
    rows = [capacity_row(n, k) for n, k in configurations]
    costs = [cost_bound_row(n, k) for n, k in configurations]
    return {
        "label": LABEL,
        "status": "PASS_ARITHMETIC_AND_COVERAGE_CHECKS; NO_SECURE_COMPOSITION_CLAIM",
        "environment": {
            "python": sys.version.split()[0],
            "platform": platform.platform(),
            "implementation": platform.python_implementation(),
        },
        "assumptions": {
            "scores": "signed int32 raw Q20.12 bit patterns",
            "stable_order": "score descending, original_index ascending",
            "S4_low_to_high_real_order": "score ascending, original_index descending",
            "select_rank_1based": "r=n-K+1",
            "membership_direction_for_reverse_priority_R": "R_i >= R_selected",
            "comparison_bits": "33 + max(1, ceil(log2(n)))",
            "offline_full_pool_direction_safe": "8*D*(D-1)+n*(n-1), where D=next_power_of_two(2*M-n); one slot per ordered comparator request and stage/instance",
            "offline_full_pool_canonical_optimization": "8*C(D,2)+C(n,2), only if canonical comparison shares can safely derive the reverse bit without another one-shot key/Eval",
            "compact_shared_pad_option": "M+1 union is a PROJECT_DERIVED identity-sharing extension and is not used for conservative sizing",
            "fixed_sample_pool": "conditional only; requires fixed hidden sample-handle proof; direction-safe R2 union is s*(D-1)",
            "storage_padding": "D-(2*M-n) addresses are capacity-only and are not S4 Select items",
            "current_runtime_domain_limit": "Protocol I layout currently pads logical n only through 1,048,576; a larger common-handle D requires a new domain API",
            "slot_accounting": "one ordered one-shot material request requires one party key slot at P0 and one at P1; canonical reuse is separately identified as conditional",
            "S5_byte_estimate": "97+24*Bin bytes per party/edge (16 endpoint-mask + 24 envelope + current 57+24*Bin key serialization); lower bound only",
        },
        "key_conformance": key,
        "edge_coverage_enumeration": coverage,
        "width_configurations": widths,
        "capacity_configurations": rows,
        "S4_comparison_bound_configurations": costs,
        "metrics_not_measured": [
            "actual key generation/Dealer runtime",
            "actual material bytes generated and persisted",
            "online time, per-party sent/received bytes",
            "PRG calls, abort frequency",
            "actual DCF/uCMP evaluations for an S6 secure entry",
            "secure online causal rounds",
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, help="write deterministic JSON evidence")
    args = parser.parse_args()
    report = make_report()
    encoded = json.dumps(report, sort_keys=True, indent=2) + "\n"
    digest = hashlib.sha256(encoded.encode("utf-8")).hexdigest()
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(encoded, encoding="utf-8", newline="\n")
        print(f"evidence={args.output}")
    print(f"label={LABEL}")
    print(f"status={report['status']}")
    print(f"key_cases={report['key_conformance']}")
    print(f"coverage={report['edge_coverage_enumeration']}")
    print(f"configs={len(report['capacity_configurations'])}")
    print(f"sha256={digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
