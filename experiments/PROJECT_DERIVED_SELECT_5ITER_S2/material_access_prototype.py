#!/usr/bin/env python3
"""TEST_ONLY slot-coverage/leakage witness for adaptive comparison requests.

This models identifiers and counts only. It creates no DCF key and makes no
security claim. The point is to make the online slot-address exposure explicit.
"""
from __future__ import annotations

import argparse
import json
import random
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "experiments" / "TEST_ONLY_PROJECT_DERIVED_SELECT_5ITER"))
from select5iter import select_kth  # noqa: E402


def plan(n: int, trace: dict) -> dict:
    rounds = trace["rounds"]
    all_round_edges: list[list[list[int]]] = []
    per_round = []
    for row in rounds:
        edges = row["frozen_edges"]
        all_round_edges.append(edges)
        per_round.append({"round": row["round"], "kind": row["kind"],
                          "logical_edge_calls": len(edges),
                          "distinct_pairs_within_round": len({tuple(x) for x in edges}),
                          "public_pair_slot_ids_if_direct_addressing": [f"pair:{a}:{b}" for a, b in edges]})
    all_calls = [tuple(edge) for edges in all_round_edges for edge in edges]
    unique_pairs = set(all_calls)
    # Unique one-shot material must be per layer/request. Reuse is forbidden;
    # addresses still reveal the adaptive graph if fetched by public indices.
    one_shot_slots = sum(len(edges) for edges in all_round_edges)
    complete_pair_domain = n * (n - 1) // 2
    return {"status": "TEST_ONLY_INTERFACE_WITNESS", "n": n,
            "per_round": per_round,
            "logical_edge_calls": len(all_calls),
            "unique_unordered_pairs_over_all_rounds": len(unique_pairs),
            "cross_round_repeat_calls_requiring_fresh_slots": len(all_calls) - len(unique_pairs),
            "minimum_one_shot_slots_for_this_trace": one_shot_slots,
            "full_pair_pool_slots_if_preallocated_per_round_worst_case": 5 * complete_pair_domain,
            "slot_bytes_per_party": "NOT_MEASURED (no material ABI instantiated)",
            "dcf_evals": "NOT_MEASURED (no DCF key or eval)",
            "revealed_field_under_direct_addressing": ["round number", "pair endpoint identifiers", "request order/count"],
            "conclusion": "A trace-selected material address exposes the adaptive comparison graph unless hidden by an oblivious access layer; a complete input-independent pool alone does not hide which slots were read."}


def matrix_material_formula(logical_n: int) -> dict:
    padded = max(2, 1 << (logical_n - 1).bit_length())
    index_bits = (padded - 1).bit_length()
    bits = 33 + index_bits
    dcf_key_bytes = lambda width: 57 + 24 * width  # exact ProtocolIUcmpPartyMaterial serialization length
    full_pair = padded * (padded - 1) // 2
    select_pool = 5 * full_pair
    input_slots = 2 * padded
    membership_slots = padded
    # Proposed serialized directory entries: per select key round+left+right+length
    # (four u64); per membership key record+length (two u64). This is a lower
    # bound excluding shuffle material, sidecar framing, indexes and metadata.
    bytes_lower = (40 + 8 * padded + 8 +
                   input_slots * (40 + dcf_key_bytes(34)) +
                   8 + select_pool * (32 + dcf_key_bytes(bits)) +
                   8 + membership_slots * (16 + dcf_key_bytes(bits)))
    return {"logical_n": logical_n, "padded_n": padded, "index_bits": index_bits,
            "comparison_bits": bits, "per_party_uCMP_key_bytes": dcf_key_bytes(bits),
            "raw_score_conversion_slots_34bit": input_slots,
            "select_fresh_slots_full_5_iteration_pool": select_pool,
            "threshold_membership_slots_if_one_per_padded_record": membership_slots,
            "DCF_Eval_per_party_if_each_slot_evaluated_once": 2 * (input_slots + select_pool + membership_slots),
            "DCF_Eval_both_parties_if_each_slot_evaluated_once": 4 * (input_slots + select_pool + membership_slots),
            "minimum_serialized_DCF_sidecar_bytes_per_party": bytes_lower,
            "current_party_package_edge_limit": 1_000_000,
            "current_party_package_byte_limit": 64 * 1024 * 1024,
            "within_current_numeric_package_limits_if_round_aware_ABI_were_added": select_pool <= 1_000_000 and bytes_lower <= 64 * 1024 * 1024,
            "compatible_with_current_fixed_one_edge_per_pair_ABI": False,
            "excluded_bytes": ["four-stage shuffle/PermuteShare offline material", "sidecar framing/stream headers", "oblivious-access overhead", "TLS delivery", "allocator/index overhead"]}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--trace", required=True, type=Path)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--matrix", action="store_true", help="also emit padded target-matrix material lower bounds")
    parser.add_argument("--paired-data-dependent-demo", action="store_true",
                        help="compare two same-size inputs under the same algorithm seed")
    args = parser.parse_args()
    trace = json.loads(args.trace.read_text(encoding="utf-8"))
    record = plan(len(trace["scores"]), trace)
    if args.matrix:
        record["target_matrix_material_lower_bounds"] = [matrix_material_formula(n)
            for n in (128, 256, 1000, 10_000, 100_000, 1_000_000)]
    if args.paired_data_dependent_demo:
        shuffled_order = list(range(len(trace["scores"])))
        random.Random(0x5EED).shuffle(shuffled_order)
        contrast_scores = [0] * len(shuffled_order)
        for rank, item in enumerate(shuffled_order):
            contrast_scores[item] = len(shuffled_order) - rank
        other = select_kth(contrast_scores, trace["k"],
                           algorithm_seed=trace["algorithm_seed"], with_trace=True).trace
        first_edges = [set(tuple(edge) for edge in row["frozen_edges"]) for row in trace["rounds"]]
        second_edges = [set(tuple(edge) for edge in row["frozen_edges"]) for row in other["rounds"]]
        record["same_seed_input_change_witness"] = {
            "same_n_same_k_same_algorithm_seed": True,
            "same_first_sample_ids": trace["rounds"][0]["sample_ids"] == other["rounds"][0]["sample_ids"],
            "edge_set_symmetric_difference_per_iteration": [len(a ^ b) for a, b in zip(first_edges, second_edges)],
            "direct_pair_slot_schedule_is_data_dependent": any(a != b for a, b in zip(first_edges, second_edges)),
            "scope": "TEST_ONLY evidence that public pair-address scheduling changes with comparison order; not a cryptographic attack or proof about anonymous handles."}
    raw = json.dumps(record, indent=2) + "\n"
    if args.out:
        args.out.write_text(raw, encoding="utf-8")
    print(raw, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
