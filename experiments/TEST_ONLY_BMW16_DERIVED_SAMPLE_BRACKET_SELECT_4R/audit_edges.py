#!/usr/bin/env python3
"""Independent structural, causal and comparison-result audit of a full trace."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any


def key_of(item: dict[str, Any]) -> tuple[int, int, int]:
    if item["category"] == 2:
        return (2, item["score"], -item["original_index"])
    return (item["category"], item["serial"], 0)


def pair_set(edges: list[list[Any]]) -> set[tuple[int, int]]:
    return {(min(int(a), int(b)), max(int(a), int(b))) for _, a, b in edges}


def edge_hash(edges: list[list[Any]]) -> str:
    digest = hashlib.sha256()
    for _, a, b in edges:
        digest.update(struct.pack("<II", min(int(a), int(b)), max(int(a), int(b))))
    return digest.hexdigest()


def outcome_hash(edges: list[list[Any]], keys: dict[int, tuple[int, int, int]]) -> str:
    payload = bytes(1 if keys[int(a)] < keys[int(b)] else 0 for _, a, b in edges)
    return hashlib.sha256(payload).hexdigest()


def combinations(values: list[int]):
    for i, a in enumerate(values):
        for b in values[i + 1:]:
            yield min(a, b), max(a, b)


def ceil_sqrt(n: int) -> int:
    root = math.isqrt(n)
    return root if root * root == n else root + 1


def ceil_sqrt_ratio(numerator: int, denominator: int) -> int:
    floor = math.isqrt(numerator // denominator)
    return floor if floor * floor * denominator == numerator else floor + 1


def check(trace: dict[str, Any]) -> dict[str, Any]:
    items_list = trace["trace"]["items"]
    items = {int(item["id"]): item for item in items_list}
    if len(items) != len(items_list):
        raise AssertionError("duplicate item IDs")
    keys = {idx: key_of(item) for idx, item in items.items()}
    if len(set(keys.values())) != len(keys):
        raise AssertionError("internal order keys are not strict/unique")
    events = trace["trace"]["events"]
    plans = [event for event in events if event.get("event") == "ROUND_PLAN_FROZEN"]
    results = [event for event in events if event.get("event") == "ROUND_RESULTS"]
    if [e["round"] for e in plans] != [1, 2, 3, 4] or [e["round"] for e in results] != [1, 2, 3, 4]:
        raise AssertionError("trace must contain exactly four ordered frozen plans and result events")
    plan_by_round = {e["round"]: e for e in plans}
    result_by_round = {e["round"]: e for e in results}
    if any(plan["requires_results_through_round"] != plan["round"] - 1 for plan in plans):
        raise AssertionError("round plan cites same-round or future comparison results")
    if any(task.get("requires_results_through_round") != plan["round"] - 1
           for plan in plans for task in plan["tasks"]):
        raise AssertionError("task edge descriptor cites same-round or future results")

    summary_parts = trace["partitions"]
    derived = defaultdict(list)
    for event in events:
        if event.get("event", "").endswith("DERIVED_STATE"):
            derived[event.get("task")].append(event)
        elif event.get("event") == "R1_DERIVED_STATE":
            for part in event["partitions"]:
                derived[part["task"]].append({"event": "R1_DERIVED_STATE", **part})

    round_stats = {}
    seen_all: set[tuple[int, int]] = set()
    for round_no in range(1, 5):
        plan = plan_by_round[round_no]
        result_event = result_by_round[round_no]
        edges = plan.get("edges")
        if edges is None:
            raise AssertionError("full trace has no edge list")
        if len(edges) != plan["edge_count"]:
            raise AssertionError(f"R{round_no} plan count does not match edge list")
        if edge_hash(edges) != plan["edge_sha256"]:
            raise AssertionError(f"R{round_no} canonical edge hash mismatch")
        if outcome_hash(edges, keys) != result_event["outcome_sha256"]:
            raise AssertionError(f"R{round_no} outcomes do not match keys or edge order")
        if result_event["outcome_count"] != len(edges):
            raise AssertionError(f"R{round_no} outcome count mismatch")
        task_edges: dict[str, list[list[Any]]] = defaultdict(list)
        for edge in edges:
            task_edges[edge[0]].append(edge)
        expected_groups = {task["task"]: task for task in plan["tasks"]}
        if set(task_edges) != set(expected_groups):
            raise AssertionError(f"R{round_no} edge owners differ from task plan")
        for task, desc in expected_groups.items():
            if round_no == 1:
                expected_list = list(combinations(desc["sample_ids"]))
            elif round_no == 2:
                inp = desc["input_ids"]
                pivots = (desc["pivot_low"], desc["pivot_high"])
                expected_list = [
                    (min(int(i), int(p)), max(int(i), int(p)))
                    for i in inp for p in pivots if int(i) != int(p)
                ]
            elif round_no == 3:
                u, v = list(map(int, desc["U_ids"])), list(map(int, desc["V_ids"]))
                vset = set(v)
                expected_list = list(combinations(v)) + [
                    (min(x, y), max(x, y))
                    for x in v for y in u if y not in vset
                ]
            else:
                expected_list = list(combinations(desc["W_ids"]))
            observed_counts = Counter((min(int(a), int(b)), max(int(a), int(b))) for _, a, b in task_edges[task])
            expected_counts = Counter(expected_list)
            if observed_counts != expected_counts:
                missing = sum((expected_counts - observed_counts).values())
                extra = sum((observed_counts - expected_counts).values())
                raise AssertionError(f"R{round_no} task {task} edge set mismatch: missing={missing}, extra={extra}")

        codes = [(min(int(a), int(b)), max(int(a), int(b))) for _, a, b in edges]
        unique = set(codes)
        prior_repeat = sum(code in seen_all for code in codes)
        real_real = sum(items[int(a)]["category"] == 2 and items[int(b)]["category"] == 2 for _, a, b in edges)
        current_dups = len(codes) - len(unique)
        same_task_repeats = sum(
            len(task_codes) - len(set(task_codes))
            for task_codes in (
                [(min(int(a), int(b)), max(int(a), int(b))) for _, a, b in task_edges[task_code]]
                for task_code in task_edges
            )
        )
        earlier_task_codes: set[tuple[int, int]] = set()
        cross_task_repeats = 0
        for task_code in task_edges:
            current_task_codes = [(min(int(a), int(b)), max(int(a), int(b)))
                                  for _, a, b in task_edges[task_code]]
            unique_task_codes = set(current_task_codes)
            cross_task_repeats += len(unique_task_codes & earlier_task_codes)
            earlier_task_codes.update(unique_task_codes)
        if current_dups != same_task_repeats + cross_task_repeats:
            raise AssertionError("same-task and cross-task duplicate accounting does not sum to duplicate calls")
        round_stats[str(round_no)] = {
            "calls": len(edges), "unique_unordered_edges": len(unique),
            "same_round_repeated_calls": current_dups,
            "same_task_repeated_calls": same_task_repeats,
            "cross_task_repeated_calls": cross_task_repeats,
            "prior_round_repeated_calls": prior_repeat,
            "real_real_calls": real_real, "dummy_related_calls": len(edges) - real_real,
            "edge_sha256": edge_hash(edges),
        }
        seen_all.update(unique)
        reported = trace["round_metrics"][str(round_no)]
        for field in ("comparison_calls", "real_real_calls", "dummy_related_calls",
                      "same_round_duplicate_calls", "same_task_repeat_calls_same_round",
                      "cross_task_repeat_calls_same_round", "prior_round_repeat_calls",
                      "unique_unordered_edges_this_round"):
            audit_field = {
                "comparison_calls": "calls",
                "real_real_calls": "real_real_calls",
                "dummy_related_calls": "dummy_related_calls",
                "same_round_duplicate_calls": "same_round_repeated_calls",
                "same_task_repeat_calls_same_round": "same_task_repeated_calls",
                "cross_task_repeat_calls_same_round": "cross_task_repeated_calls",
                "prior_round_repeat_calls": "prior_round_repeated_calls",
                "unique_unordered_edges_this_round": "unique_unordered_edges",
            }[field]
            if reported[field] != round_stats[str(round_no)][audit_field]:
                raise AssertionError(f"algorithm counter differs from independent edge audit for R{round_no}/{field}")

    # Rebuild R1 pivot ranks from its recorded pair outcomes, without using the algorithm module.
    r1 = plan_by_round[1]
    pivot_expectations = {}
    for task in r1["tasks"]:
        sample = list(map(int, task["sample_ids"]))
        counts = Counter()
        for owner, a, b in r1["edges"]:
            if owner == task["task"]:
                if keys[int(a)] < keys[int(b)]:
                    counts[int(b)] += 1
                else:
                    counts[int(a)] += 1
        by_rank = {counts[item] + 1: item for item in sample}
        ql, qh = task["qL"], task["qH"]
        expected_pivots = (by_rank[ql], by_rank[qh])
        pivot_expectations[task["task"]] = expected_pivots
        r2_task = next(t for t in plan_by_round[2]["tasks"] if t["task"] == task["task"])
        if (r2_task["pivot_low"], r2_task["pivot_high"]) != expected_pivots:
            raise AssertionError(f"R2 pivots for {task['task']} are not derived from R1 results")

    # Independently derive R2 U sets and compare them to the state consumed by R3.
    r2_derived = {event["task"]: event for event in events if event.get("event") == "R2_DERIVED_STATE"}
    r2_tasks = {task["task"]: task for task in plan_by_round[2]["tasks"]}
    r3_tasks = {task["task"]: task for task in plan_by_round[3]["tasks"]}
    for task, desc in r2_tasks.items():
        x, y = int(desc["pivot_low"]), int(desc["pivot_high"])
        inp = list(map(int, desc["input_ids"]))
        prefix = [i for i in inp if keys[i] < keys[x]]
        suffix = [i for i in inp if keys[y] < keys[i]]
        u_ids = [i for i in inp if not keys[i] < keys[x] and (keys[i] < keys[y] or i == y)]
        q = len(inp) // 2 - len(prefix)
        state = r2_derived[task]
        if (state["prefix_ids"] != prefix or state["suffix_ids"] != suffix
                or state["U_ids"] != u_ids or state["q"] != q):
            raise AssertionError(f"R2 derived prefix/U/rank mismatch for {task}")
        if task in r3_tasks:
            r3 = r3_tasks[task]
            if r3["U_ids"] != u_ids or r3["q"] != q:
                raise AssertionError(f"R3 graph for {task} depends on unexpected R2 state")
            u, m = len(u_ids), len(inp)
            expected_v = min(u, ceil_sqrt_ratio(64 * u * u, m))
            if len(r3["V_ids"]) != expected_v or not set(map(int, r3["V_ids"])) <= set(u_ids):
                raise AssertionError(f"R3 sample rule/membership mismatch for {task}")

    # Derive each R3 sample rank/window directly from keys, then validate R4's graph.
    r3_derived = {event["task"]: event for event in events if event.get("event") == "R3_DERIVED_STATE"}
    r4_tasks = {task["task"]: task for task in plan_by_round[4]["tasks"]}
    for task, desc in r3_tasks.items():
        u_ids = list(map(int, desc["U_ids"]))
        sample = list(map(int, desc["V_ids"]))
        q = int(desc["q"])
        sample_ranks = {item: sum(keys[other] < keys[item] for other in u_ids) + 1 for item in sample}
        lower = [(item, rank) for item, rank in sample_ranks.items() if rank <= q]
        upper = [(item, rank) for item, rank in sample_ranks.items() if rank > q]
        i = max((rank for _, rank in lower), default=0)
        j = min((rank for _, rank in upper), default=len(u_ids) + 1)
        x = next((item for item, rank in lower if rank == i), None)
        y = next((item for item, rank in upper if rank == j), None)
        w = [item for item in u_ids
             if (x is None or keys[item] >= keys[x])
             and (y is None or keys[item] <= keys[y])]
        q_w = q - (i - 1 if i else 0)
        state = r3_derived[task]
        if (state["sample_ranks"] != {str(k): v for k, v in sample_ranks.items()}
                or state["x_id"] != x or state["x_rank"] != i
                or state["y_id"] != y or state["y_rank"] != j
                or state["W_ids"] != w or state["qW"] != q_w):
            raise AssertionError(f"R3 rank/window state mismatch for {task}")
        r4 = r4_tasks[task]
        if r4["W_ids"] != w or r4["qW"] != q_w:
            raise AssertionError(f"R4 graph for {task} depends on unexpected R3 state")

    # The final candidate must be a single item in both correct median partitions.
    n, k = trace["n"], trace["K"]
    expected_r = n - k + 1
    if trace["effective_median_size_M"] != 2 * max(expected_r, n + 1 - expected_r):
        raise AssertionError("Appendix A expansion size mismatch")
    metric_total = sum(stat["calls"] for stat in round_stats.values())
    if metric_total != trace["total_comparison_calls"]:
        raise AssertionError("independent edge count differs from algorithm counter")
    if trace.get("status") == "SUCCESS":
        expected_item = sorted(
            (item for item in items_list if item["category"] == 2),
            key=lambda item: keys[int(item["id"])],
        )[n - k]
        if trace["selected_original_index"] != expected_item["original_index"]:
            raise AssertionError("candidate differs from independently ranked stable Kth item")
    return {
        "status": "PASS",
        "label": trace["label"], "n": n, "K": k,
        "rounds": round_stats,
        "total_comparison_calls": metric_total,
        "unique_unordered_edges_all_rounds": len(seen_all),
        "repeated_total_calls": metric_total - len(seen_all),
        "candidate_original_index": trace["selected_original_index"],
        "pivot_ids_derived_from_R1": pivot_expectations,
        "four_causal_rounds": True,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Independently audit one complete TEST_ONLY edge trace")
    parser.add_argument("trace", type=Path)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    result = check(json.loads(args.trace.read_text(encoding="utf-8")))
    args.out.write_text(json.dumps(result, sort_keys=True, indent=2), encoding="utf-8")
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
