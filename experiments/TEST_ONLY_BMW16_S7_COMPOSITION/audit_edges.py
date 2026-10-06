#!/usr/bin/env python3
"""Independent edge-list/count audit for one TEST_ONLY trace JSON."""
from __future__ import annotations

import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path
from typing import Any


def audit(path: Path) -> dict[str, Any]:
    run = json.loads(path.read_text(encoding="utf-8"))
    select = run["run_select"]
    trace = select["trace"]
    items = trace["items"]
    events = trace["events"]
    prior_edges: set[tuple[int, int]] = set()
    plan_position: dict[int, int] = {}
    results_position: dict[int, int] = {}
    by_round: dict[int, dict[str, Any]] = {}
    seen_plan_tasks: dict[int, list[str]] = defaultdict(list)
    for position, event in enumerate(events):
        if event["event"] == "ROUND_PLAN_FROZEN":
            round_no = int(event["round"])
            if round_no in plan_position:
                raise AssertionError(f"multiple plans for comparison round {round_no}")
            plan_position[round_no] = position
            if int(event["requires_results_through_round"]) != round_no - 1:
                raise AssertionError("round declares an incorrect causal predecessor")
            if round_no > 1 and results_position.get(round_no - 1, 10**9) >= position:
                raise AssertionError("round graph was planned before prior result barrier")
            edge_rows = event.get("edges", [])
            canonical_bytes = bytearray()
            local_edges: list[tuple[str, int, int]] = []
            for task, left, right in edge_rows:
                left, right = int(left), int(right)
                if not (0 <= left < len(items) and 0 <= right < len(items)) or left == right:
                    raise AssertionError("invalid comparison endpoint")
                lo, hi = sorted((left, right))
                canonical_bytes.extend(lo.to_bytes(4, "little"))
                canonical_bytes.extend(hi.to_bytes(4, "little"))
                local_edges.append((str(task), lo, hi))
            edge_digest = hashlib.sha256(canonical_bytes).hexdigest()
            if edge_digest != event["edge_sha256"]:
                raise AssertionError(f"round {round_no} digest mismatch")
            batch_order = list(dict.fromkeys(task for task, _, _ in local_edges))
            same_task_repeat = 0
            cross_task_repeat = 0
            seen_round_global: set[tuple[int, int]] = set()
            seen_task: dict[str, set[tuple[int, int]]] = defaultdict(set)
            for task, lo, hi in local_edges:
                edge = (lo, hi)
                seen_plan_tasks[round_no].append(task)
                if edge in seen_task[task]:
                    same_task_repeat += 1
                else:
                    seen_task[task].add(edge)
                    if any(edge in seen_task[earlier] for earlier in batch_order[:batch_order.index(task)]):
                        cross_task_repeat += 1
                seen_round_global.add(edge)
            logical = len(local_edges)
            prior_repeat = sum((lo, hi) in prior_edges for _, lo, hi in local_edges)
            round_real = sum(items[lo]["category"] == 2 and items[hi]["category"] == 2
                             for _, lo, hi in local_edges)
            by_round[round_no] = {
                "logical_edges_recomputed": logical,
                "real_real_edges_recomputed": round_real,
                "dummy_related_edges_recomputed": logical - round_real,
                "same_task_repeats_recomputed": same_task_repeat,
                "cross_task_repeats_recomputed": cross_task_repeat,
                "same_round_global_repeats_recomputed": logical - len(seen_round_global),
                "prior_round_repeat_calls_recomputed": prior_repeat,
                "unique_unordered_edges_recomputed": len(seen_round_global),
                "edge_sha256_recomputed": edge_digest,
            }
            prior_edges |= seen_round_global
        elif event["event"] == "ROUND_RESULTS":
            round_no = int(event["round"])
            if round_no not in plan_position or round_no in results_position:
                raise AssertionError("result does not follow exactly one frozen plan")
            if position <= plan_position[round_no]:
                raise AssertionError("results preceded plan")
            results_position[round_no] = position
    if set(plan_position) != {1, 2, 3, 4} or set(results_position) != {1, 2, 3, 4}:
        raise AssertionError("trace does not contain four plan/result barriers")
    for round_no, recounted in by_round.items():
        source = select["round_metrics"][str(round_no)]
        checks = {
            "comparison_calls": "logical_edges_recomputed",
            "real_real_calls": "real_real_edges_recomputed",
            "dummy_related_calls": "dummy_related_edges_recomputed",
            "same_task_repeat_calls_same_round": "same_task_repeats_recomputed",
            "cross_task_repeat_calls_same_round": "cross_task_repeats_recomputed",
            "same_round_duplicate_calls": "same_round_global_repeats_recomputed",
            "prior_round_repeat_calls": "prior_round_repeat_calls_recomputed",
            "unique_unordered_edges_this_round": "unique_unordered_edges_recomputed",
            "edge_sha256": "edge_sha256_recomputed",
        }
        for reported, audited in checks.items():
            if source[reported] != recounted[audited]:
                raise AssertionError(f"round {round_no} mismatch {reported}: {source[reported]} != {recounted[audited]}")
        # Per-task one-shot material domain: every unique unordered pair in
        # each Select instance consumes a distinct FSS slot.
        task_pairs = defaultdict(set)
        for task, left, right in trace["events"][plan_position[round_no]].get("edges", []):
            task_pairs[task].add(tuple(sorted((int(left), int(right)))))
        slots = sum(map(len, task_pairs.values()))
        if source["fss_material_slots"] != slots:
            raise AssertionError(f"round {round_no} FSS slot count mismatch")
        if source["fss_two_party_dcf_evals"] != 4 * slots:
            raise AssertionError(f"round {round_no} two-party DCF Eval count mismatch")
    return {
        "label": "BMW16_S7_INDEPENDENT_EDGE_LIST_AUDIT_TEST_ONLY",
        "source_run_json": str(path),
        "four_causal_plan_result_barriers": "PASS",
        "rounds": by_round,
        "total_logical_edges_recomputed": sum(v["logical_edges_recomputed"] for v in by_round.values()),
        "total_fss_material_slots_recomputed": sum(int(select["round_metrics"][str(r)]["fss_material_slots"])
                                                   for r in range(1, 5)),
        "total_two_party_dcf_evals_recomputed": sum(int(select["round_metrics"][str(r)]["fss_two_party_dcf_evals"])
                                                   for r in range(1, 5)),
        "scope": "Independent counting and event-order audit of TEST_ONLY trace; not a secure-view proof.",
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("trace", type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    result = audit(args.trace)
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"BMW16_S7_EDGE_AUDIT PASS logical_edges={result['total_logical_edges_recomputed']} "
          f"FSS_slots={result['total_fss_material_slots_recomputed']}")


if __name__ == "__main__":
    main()
