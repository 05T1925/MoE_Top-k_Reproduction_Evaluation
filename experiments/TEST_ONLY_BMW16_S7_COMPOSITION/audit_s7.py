#!/usr/bin/env python3
"""Independent finite audit of anonymity examples and key-pool namespaces."""
from __future__ import annotations

import argparse
import itertools
import json
from collections import Counter
from pathlib import Path


def transcript(order: tuple[int, ...], n: int, edges: tuple[tuple[int, int], ...]):
    # order lists secret object identities from low to high. A random hidden
    # relabeling pi maps object -> anonymous handle.
    rank = {obj: index for index, obj in enumerate(order)}
    for pi_tuple in itertools.permutations(range(n)):
        pi = dict(enumerate(pi_tuple))
        yield tuple(int(rank[next(obj for obj in range(n) if pi[obj] == left)] <
                         rank[next(obj for obj in range(n) if pi[obj] == right)])
                    for left, right in edges)


def anonymous_distribution(order: tuple[int, ...], n: int,
                           edges: tuple[tuple[int, int], ...]) -> Counter:
    return Counter(transcript(order, n, edges))


def visible_original_label_counterexample():
    # Same corrupted P0 additive input share (zero) in both executions.
    p0_local = (0, 0)
    world01 = (0, 1)
    world10 = (1, 0)
    # A visible original-ID comparison of endpoint 0 against endpoint 1.
    visible_bit_01 = int(world01[0] < world01[1])
    visible_bit_10 = int(world10[0] < world10[1])
    output_mask_01 = (0, 1)  # score DESC, original_index ASC; K=1
    output_mask_10 = (1, 0)
    assert p0_local == (0, 0) and visible_bit_01 != visible_bit_10
    return {
        "same_corrupted_P0_additive_shares": list(p0_local),
        "same_corrupted_P1_additive_shares": list(p0_local),
        "other_party_additive_shares_by_world": {
            "when_P0_is_corrupted": {"world_0_lt_1": [0, 1], "world_1_lt_0": [1, 0]},
            "when_P1_is_corrupted": {"world_0_lt_1": [0, 1], "world_1_lt_0": [1, 0]},
        },
        "world_order_0_lt_1_public_bit": visible_bit_01,
        "world_order_1_lt_0_public_bit": visible_bit_10,
        "ideal_top1_masks": {"world_0_lt_1": list(output_mask_01),
                             "world_1_lt_0": list(output_mask_10)},
        "P0_ideal_XOR_output_share_distribution_each_world": {
            "all_2_bit_share_vectors": [[0, 0], [0, 1], [1, 0], [1, 1]],
            "probability_each": "1/4 in both worlds",
        },
        "public_bit_supports_disjoint_even_conditioned_on_P0_ideal_output_share": True,
        "distinguishing_event": "public bit for endpoints named by original record IDs equals 1",
        "conclusion": "S6 attack applies when public endpoint handles can be mapped to original records",
    }


def anonymous_order_enumeration():
    rows = []
    for n in (2, 3, 4):
        all_edges = tuple((i, j) for i in range(n) for j in range(i + 1, n))
        subsets = [()] if n == 2 else [tuple(e for bit, e in enumerate(all_edges) if mask >> bit & 1)
                                       for mask in range(1 << len(all_edges))]
        for edges in subsets:
            distributions = [anonymous_distribution(order, n, edges)
                             for order in itertools.permutations(range(n))]
            if any(dist != distributions[0] for dist in distributions[1:]):
                raise AssertionError(f"anonymous transcript depends on order n={n}, edges={edges}")
            rows.append({
                "n": n,
                "secret_orders": len(distributions),
                "queried_undirected_edges": [list(e) for e in edges],
                "hidden_permutations_per_order": __import__("math").factorial(n),
                "transcript_distribution": {"number_of_transcripts": len(distributions[0]),
                                            "count_per_transcript": sorted(set(distributions[0].values()))[0]
                                            if distributions[0] else 0},
                "same_distribution_for_every_secret_order": True,
            })
    # n=2, a single anonymously named ordered endpoint test has exactly one
    # transcript bit 0 and one bit 1 under either secret order.
    n2 = [anonymous_distribution(order, 2, ((0, 1),))
          for order in itertools.permutations(range(2))]
    assert n2[0] == n2[1] == Counter({(0,): 1, (1,): 1})
    return rows


def namespace_capacity(n: int, k: int):
    r = n - k + 1
    m = 2 * max(r, k)
    h_union = 2 * m - n
    d = 1 << (h_union - 1).bit_length()
    b = max(1, (n - 1).bit_length())
    numeric_code_bits = 33 + b
    safe_ucmp_bits = 34 + b
    local_ordered = 8 * m * (m - 1) + n * (n - 1)
    local_unordered = 8 * (m * (m - 1) // 2) + n * (n - 1) // 2
    global_ordered = 8 * d * (d - 1) + n * (n - 1)
    global_unordered = 8 * (d * (d - 1) // 2) + n * (n - 1) // 2
    bytes_per_slot = 97 + 24 * safe_ucmp_bits
    address_local = 2 * m
    local_domain_ordered = local_ordered
    local_domain_unordered = local_unordered
    return {
        "n": n, "K": k, "M_each_select_partition": m,
        "two_instance_shared_real_plus_private_dummy_union": h_union,
        "S6_global_next_power_of_two_domain": d,
        "S6_comparator_numeric_code_bits_33_plus_b": numeric_code_bits,
        "corrected_current_uCMP_bits_34_plus_b": safe_ucmp_bits,
        "corrected_width_supported": safe_ucmp_bits <= 53,
        "S6_global_D_all_pairs_ordered_slots_direction_safe": global_ordered,
        "S6_global_D_all_pairs_unordered_slots_if_complement_proved": global_unordered,
        "per_instance_M_active_ordered_slots_direction_safe": local_domain_ordered,
        "per_instance_M_active_unordered_slots_if_complement_proved": local_domain_unordered,
        "membership_offline_ordered_slots_worst_case_threshold_unknown": n * (n - 1),
        "membership_offline_unordered_slots_if_complement_proved": n * (n - 1) // 2,
        "membership_actual_test_harness_calls_after_threshold_open": n,
        "per_party_serialized_lower_bound_bytes_corrected_width_M_active":
            local_ordered * bytes_per_slot if safe_ucmp_bits <= 53 else "NOT_SUPPORTED_BY_CURRENT_UCMP",
        "bytes_per_logical_slot_model": bytes_per_slot if safe_ucmp_bits <= 53 else "NOT_SUPPORTED_BY_CURRENT_UCMP",
        "three_contracts": {
            "global_shared_real_private_dummy_H_union_then_D_storage": {
                "two_sentinel_intersection": "preserved if opaque real IDs match across instances",
                "shuffle_privacy": "not established by domain arithmetic",
                "key_id": "must include instance/round/stage/pair; global reuse would be invalid",
                "capacity": {"D": d, "ordered_active_full_D_pool": global_ordered},
            },
            "two_disjoint_M_local_namespaces": {
                "two_sentinel_intersection": "not directly available; needs secret cross-instance real-record join",
                "shuffle_privacy": "not established by domain arithmetic",
                "key_id": "instance-scoped IDs naturally distinct",
                "capacity": {"ordered_full_active_pool": local_domain_ordered},
            },
            "shared_real_handles_plus_instance_local_dummy_addresses": {
                "two_sentinel_intersection": "preserved for shared real handles; dummy addresses remain task-local",
                "shuffle_privacy": "not established; public intersection may identify real cohort",
                "key_id": "must include instance/round/stage even on a shared real endpoint pair",
                "capacity": {"union_handles": h_union,
                             "per_instance_active_domain": m,
                             "ordered_full_active_pool": local_domain_ordered},
            },
        },
        "review": "next_power_of_two(2M-n) is a conservative global envelope, not required for each Select's active pair pool. Active-M capacity is smaller, but exceeds the existing 1,000,000 edge package limit at n=1000/K=80; DCF adaptive retrieval and joint views remain open.",
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = {
        "label": "BMW16_S7_INDEPENDENT_VIEW_AND_NAMESPACE_AUDIT_TEST_ONLY",
        "anonymous_handle_transcript_enumeration": anonymous_order_enumeration(),
        "original_record_endpoint_leak_counterexample": visible_original_label_counterexample(),
        "namespace_capacity_configs": [namespace_capacity(*pair) for pair in (
            (128, 2), (128, 8), (256, 2), (256, 8), (1000, 80), (1_000_000, 1))],
        "proof_boundary": {
            "enumeration_proves": "For fixed queried handle edges and a uniformly random hidden relabeling, public strict-order bits have the same distribution for every strict underlying order for n<=4.",
            "enumeration_does_not_prove": "Joint simulation of local shares, per-party shuffle state/messages, correlated multi-instance DCF materials/access trace, adaptive edge selection, abort, or output shares.",
        },
    }
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print("BMW16_S7_VIEW_NAMESPACE_AUDIT PASS; joint party-view security remains unproved")


if __name__ == "__main__":
    main()
