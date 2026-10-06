#!/usr/bin/env python3
"""Exact finite permutation audit of S4's adaptive public graph transcript.

This checks only the public graph/comparison/abort/selected-handle transcript
and ideal uniformly shared output bits, conditioned on a uniform hidden
permutation. It is not a DCF-pool or shuffle party-view security proof.
"""
from __future__ import annotations

import argparse
import hashlib
import itertools
import json
import sys
from collections import Counter
from pathlib import Path
from typing import Any

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "TEST_ONLY_BMW16_S7_COMPOSITION"))
from select4r_s4_source import Context

MASK64 = (1 << 64) - 1


def public_less(left: Any, right: Any) -> bool:
    if left.category != right.category:
        return left.category < right.category
    if left.category == 2:
        raise AssertionError("real-real edge delegated to hidden order")
    return left.serial < right.serial


class ExactOrderBackend:
    def __init__(self, order_by_handle: list[int]):
        self.order = order_by_handle
        self.round_slots: dict[int, int] = Counter()
        self.round_reverse_complements: dict[int, int] = Counter()
        self.round_logical_calls: dict[int, int] = Counter()

    def compare_batches(self, round_no: int, batches: list[dict[str, Any]], items: list[Any], *_: Any) -> np.ndarray:
        outcomes = []
        for batch in batches:
            for l, r in zip(batch["left"], batch["right"]):
                left, right = items[int(l)], items[int(r)]
                if left.category == right.category == 2:
                    outcomes.append(self.order[int(l)] < self.order[int(r)])
                else:
                    outcomes.append(public_less(left, right))
        self.round_logical_calls[round_no] += len(outcomes)
        return np.asarray(outcomes, dtype=np.bool_)


def transcript_for_world(order_by_handle: list[int], k: int, seed: int) -> dict[str, Any]:
    n = len(order_by_handle)
    backend = ExactOrderBackend(order_by_handle)
    result = Context([0] * n, k, 0, seed, True, list(range(n)), backend).run()
    trace = result["trace"]
    plans = []
    for event in trace["events"]:
        if event["event"] == "ROUND_PLAN_FROZEN":
            plans.append({"round": event["round"], "tasks": event["tasks"], "edges": event.get("edges", [])})
        elif event["event"] == "ROUND_RESULTS":
            plans[-1]["outcome_sha256"] = event["outcome_sha256"]
    derived = [event for event in trace["events"] if event["event"] in {
        "R1_DERIVED_STATE", "R2_DERIVED_STATE", "R3_DERIVED_STATE"}]
    return {
        "shared_real_handles_used_by_both_selects": list(range(n)),
        "public_rounds": plans,
        "derived_public_state": derived,
        "abort_class": result["status"],
        "partition_abort_classes": result["partition_statuses"],
        "selected_anonymous_handle": result.get("selected_original_index"),
    }


def distribution(world_ranks: list[int], k: int, seed: int) -> tuple[Counter[str], Counter[str]]:
    n = len(world_ranks)
    transcript_counts: Counter[str] = Counter()
    corrupted_view: Counter[str] = Counter()
    fixed_corrupt_input_shares = [0] * n
    # pi[original] = shuffled opaque handle. The same pi is used by both
    # sentinel Select instances, so their common-real-handle relation is kept.
    for pi in itertools.permutations(range(n)):
        handle_to_original = [0] * n
        for original, handle in enumerate(pi):
            handle_to_original[handle] = original
        hidden_handle_order = [world_ranks[original] for original in handle_to_original]
        public = transcript_for_world(hidden_handle_order, k, seed)
        serial = json.dumps(public, separators=(",", ":"), sort_keys=True)
        digest = hashlib.sha256(serial.encode()).hexdigest()
        transcript_counts[digest] += 1
        local_view = {"fixed_corrupt_input_shares": fixed_corrupt_input_shares,
                      "public_transcript_sha256": digest}
        # Ideal output shares are independent uniform bit vectors to either
        # one-party observer; enumerate every local share value exactly.
        for local_output_share in range(1 << n):
            view = dict(local_view, local_output_share=local_output_share)
            corrupted_view[hashlib.sha256(json.dumps(view, sort_keys=True).encode()).hexdigest()] += 1
    return transcript_counts, corrupted_view


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--max-n", type=int, default=5)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if not 2 <= args.max_n <= 6:
        parser.error("max-n must be in [2,6]")
    rows = []
    all_equal = True
    for n in range(2, args.max_n + 1):
        k = max(1, n // 2)
        # Distinct stable total orders representing two different assignments
        # of the same record set to scores; original indices remain tie keys.
        worlds = [list(range(n)), list(reversed(range(n)))]
        seed_rows = []
        for seed in (42, 43):
            a_t, a_v = distribution(worlds[0], k, seed)
            b_t, b_v = distribution(worlds[1], k, seed)
            equal = a_t == b_t and a_v == b_v
            all_equal &= equal
            seed_rows.append({
                "algorithm_seed": seed,
                "hidden_permutations_per_world": __import__("math").factorial(n),
                "transcript_support_world0": len(a_t),
                "transcript_support_world1": len(b_t),
                "transcript_multiset_equal": a_t == b_t,
                "corrupt_P0_view_multiset_equal_under_ideal_output_shares": a_v == b_v,
                "corrupt_P1_view_multiset_equal_under_ideal_output_shares": a_v == b_v,
                "scope": "adaptive R1-R4 edges/outcomes, both sentinel instances, shared handles, selected handle, and abort; no local shuffle messages or DCF keys",
            })
        rows.append({"n": n, "K": k, "rows": seed_rows})
    out = {
        "label": "BMW16_S8_EXACT_ADAPTIVE_GRAPH_VIEW_ENUMERATION_TEST_ONLY",
        "permutation_model": "uniform hidden mapping from original records to anonymous handles",
        "fixed_corrupt_party_input_shares": "all-zero vector in both worlds",
        "both_select_instances": "same real handles, separate sentinel/dummy identities and derived seeds",
        "enumerated": rows,
        "all_public_transcript_distributions_equal": all_equal,
        "security_boundary": "finite exact check for public comparison graph and ideal output-share marginal only; it excludes actual local shuffle state/messages, FSS key pools/access traces, and their joint correlation",
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(out, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({"all_equal": all_equal, "n_cases": len(rows), "output": str(args.output)}))
    return 0 if all_equal else 1


if __name__ == "__main__":
    raise SystemExit(main())
