#!/usr/bin/env python3
"""TEST_ONLY projection of plaintext traces onto the newly approved public L."""

from __future__ import annotations

import argparse
import json
import random
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
S1 = ROOT / "experiments" / "TEST_ONLY_PROJECT_DERIVED_SELECT_5ITER"
sys.path.insert(0, str(S1))
from select5iter import select_kth  # noqa: E402


ALGORITHM = "PROJECT_DERIVED_SELECT_5ITER"
PURPOSE = "select_compare"
DIRECTION = "strict_lt_lower_to_higher"


def slot_id(session: int, iteration: int, substage: str, left: int, right: int) -> str:
    # Injective readable logical address. Physical material may share one
    # iteration/pair slot only because clique and star edge sets are disjoint;
    # the use claim retains substage.
    return (
        f"session-{session:016x}-{PURPOSE}-{substage}-{DIRECTION}-"
        f"iter-{iteration:02d}-pair-{left:03d}-{right:03d}"
    )


def shuffled_scores(scores: list[int], original_to_shuffled: list[int]) -> list[int]:
    output = [0] * len(scores)
    for original, position in enumerate(original_to_shuffled):
        output[position] = scores[original]
    return output


def project_public_transcript(scores: list[int], k: int, seed: int, session: int) -> dict[str, Any]:
    result = select_kth(scores, k, algorithm_seed=seed, with_trace=True)
    assert result.trace is not None
    rounds = []
    all_slots: list[str] = []
    for record in result.trace["rounds"]:
        iteration = int(record["round"])
        edges = [tuple(map(int, edge)) for edge in record["frozen_edges"]]
        sample_positions = set(map(int, record["sample_ids"]))
        if edges != sorted(set(edges)):
            raise AssertionError(f"round {iteration} edge list is not canonical")
        slots = []
        for left, right in edges:
            if iteration == 5:
                substage = "final_candidate_clique"
            elif left in sample_positions and right in sample_positions:
                substage = "sample_clique"
            else:
                substage = "pivot_star"
            slots.append(slot_id(session, iteration, substage, left, right))
        if len(slots) != len(set(slots)):
            raise AssertionError(f"round {iteration} reused a slot")
        all_slots.extend(slots)
        rounds.append(
            {
                "iteration": iteration,
                "shuffled_comparison_endpoints": [list(edge) for edge in edges],
                "shuffled_pivot_positions": list(map(int, record["pivot_ids"])),
                "key_slot_ids": slots,
                "comparison_count": len(edges),
                "communication_bytes": "NOT_MEASURED",
                "unified_abort_state": "NOT_RUN",
            }
        )
    if len(all_slots) != len(set(all_slots)):
        raise AssertionError("slot identity collided across iterations")
    all_pairs = [
        tuple(edge)
        for row in rounds
        for edge in row["shuffled_comparison_endpoints"]
    ]
    unique_pairs = set(all_pairs)
    allowed = {
        "iteration",
        "shuffled_comparison_endpoints",
        "shuffled_pivot_positions",
        "key_slot_ids",
        "comparison_count",
        "communication_bytes",
        "unified_abort_state",
    }
    if any(set(row) != allowed for row in rounds):
        raise AssertionError("projected transcript contains a field outside the allowlist")
    return {
        "algorithm_label": ALGORITHM,
        "transcript_schema": "S3_PUBLIC_L_PROBE_v1",
        "session_id": session,
        "public_n": len(scores),
        "public_k": k,
        "logical_comparison_calls": len(all_pairs),
        "unique_unordered_pairs": len(unique_pairs),
        "repeated_cross_iteration_calls": len(all_pairs) - len(unique_pairs),
        "one_shot_key_slots": len(all_slots),
        "rounds": rounds,
    }


def make_fixture() -> dict[str, Any]:
    n, k, algorithm_seed, session = 128, 8, 0x5A2, 0x50330001
    # The same fixed hidden shuffle is applied to two different, unique-score
    # inputs. The mapping is never copied into either public transcript.
    original_to_shuffled = list(range(n))
    random.Random(0x51A7).shuffle(original_to_shuffled)
    input_a = list(range(n))
    input_b = [(index * 73) % n for index in range(n)]
    a = project_public_transcript(
        shuffled_scores(input_a, original_to_shuffled), k, algorithm_seed, session
    )
    b = project_public_transcript(
        shuffled_scores(input_b, original_to_shuffled), k, algorithm_seed, session
    )
    a_rounds, b_rounds = a["rounds"], b["rounds"]
    changed = {
        "shuffled_comparison_endpoints": any(
            x["shuffled_comparison_endpoints"] != y["shuffled_comparison_endpoints"]
            for x, y in zip(a_rounds, b_rounds)
        ),
        "shuffled_pivot_positions": any(
            x["shuffled_pivot_positions"] != y["shuffled_pivot_positions"]
            for x, y in zip(a_rounds, b_rounds)
        ),
        "key_slot_ids": any(
            x["key_slot_ids"] != y["key_slot_ids"] for x, y in zip(a_rounds, b_rounds)
        ),
        "comparison_count": [
            x["comparison_count"] != y["comparison_count"]
            for x, y in zip(a_rounds, b_rounds)
        ],
        "communication_bytes": "NOT_MEASURED",
        "unified_abort_state": "NOT_RUN",
    }
    return {
        "scope": "plaintext trace projection only; not a party-view or protocol simulation",
        "algorithm_seed": algorithm_seed,
        "n": n,
        "k": k,
        "same_hidden_shuffle_fixture": True,
        "input_vectors_differ": True,
        "same_input_multiset": True,
        "different_input_multisets": False,
        "input_a_values": "permutation of [0,127]",
        "input_b_values": "permutation (73*i mod 128)",
        "input_to_shuffled_mapping_disclosed": False,
        "comparison_outcomes_disclosed": False,
        "ranks_threshold_and_raw_scores_disclosed": False,
        "round_counts_a": [x["comparison_count"] for x in a_rounds],
        "round_counts_b": [x["comparison_count"] for x in b_rounds],
        "cost_projection_a": {
            key: a[key]
            for key in (
                "logical_comparison_calls",
                "unique_unordered_pairs",
                "repeated_cross_iteration_calls",
                "one_shot_key_slots",
            )
        },
        "cost_projection_b": {
            key: b[key]
            for key in (
                "logical_comparison_calls",
                "unique_unordered_pairs",
                "repeated_cross_iteration_calls",
                "one_shot_key_slots",
            )
        },
        "changed_approved_fields": changed,
        "unexpected_fields": [],
        "transcript_a": a,
        "transcript_b": b,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    data = make_fixture()
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        json.dumps(
            {
                "result": "PASS_WITH_SCOPE",
                "output": str(args.out),
                "round_counts_a": data["round_counts_a"],
                "round_counts_b": data["round_counts_b"],
                "unexpected_fields": data["unexpected_fields"],
            },
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
