"""Independent TEST_ONLY edge-list, causality, and count auditor.

This file consumes serialized frozen batches. It does not import select4r.py
and does not trust the runtime's per-round counters.
"""

from __future__ import annotations

import hashlib
import itertools
import json
from pathlib import Path
from typing import Any


def norm(a: str, b: str) -> tuple[str, str]:
    return (a, b) if a < b else (b, a)


def canonical_hash(value: dict[str, Any]) -> str:
    copy = dict(value)
    copy.pop("plan_sha256", None)
    payload = json.dumps(copy, sort_keys=True, separators=(",", ":")).encode("utf-8")
    return hashlib.sha256(payload).hexdigest()


def expected_pairs(task: dict[str, Any], round_no: int) -> set[tuple[str, str]]:
    phase = task["phase"]
    if phase == "A5_R1" and round_no == 1:
        pairs = {norm(s, t) for s in task["S1_ids"] for t in task["T1_ids"] if s != t}
    elif phase == "A5_R2" and round_no == 2:
        pivot = task["pivot_x"]
        pairs = {norm(pivot, item_id) for item_id in task["input_ids"] if item_id != pivot}
    elif phase == "A7_R3_ALL_U_PAIRS" and round_no == 3:
        pairs = {norm(a, b) for a, b in itertools.combinations(task["U_ids"], 2)}
    elif phase == "A7_R3_SAMPLE_CROSS" and round_no == 3:
        pairs = {norm(v, u) for v in task["V_ids"] for u in task["U_ids"] if v != u}
    elif phase == "A7_R4_ALL_W_PAIRS" and round_no == 4:
        pairs = {norm(a, b) for a, b in itertools.combinations(task["W_ids"], 2)}
    else:
        raise ValueError(f"phase {phase} is not valid in round {round_no}")
    return pairs


def audit_trace(trace: dict[str, Any]) -> dict[str, Any]:
    events = trace.get("events", [])
    if not events or events[0].get("event") != "TRACE_HEADER":
        raise ValueError("trace has no header")
    header = events[0]
    items = {item["id"]: item for item in header["items"]}
    summary = header["summary"]
    cursor = 1
    all_seen: set[tuple[str, str]] = set()
    total_calls = 0
    total_unique = 0
    total_repeated = 0
    round_results: dict[int, dict[tuple[str, str, str], bool]] = {}
    per_round: dict[int, dict[str, int]] = {}

    for expected_round in range(1, 5):
        if cursor >= len(events) or events[cursor].get("event") != "ROUND_PLAN_FROZEN":
            raise ValueError(f"round {expected_round}: missing frozen plan")
        plan = events[cursor]
        cursor += 1
        if plan.get("round") != expected_round:
            raise ValueError(f"expected round {expected_round}, found {plan.get('round')}")
        if canonical_hash(plan) != plan.get("plan_sha256"):
            raise ValueError(f"round {expected_round}: frozen plan digest mismatch")
        tasks = {task["task"]: task for task in plan.get("tasks", [])}
        for task in tasks.values():
            if task.get("requires_results_through_round", 0) > expected_round - 1:
                raise ValueError(f"round {expected_round}: task depends on current/future results")

        edges = plan.get("edges", [])
        actual_by_task: dict[str, list[tuple[str, str]]] = {}
        rr = 0
        dummy = 0
        same_round_pairs: set[tuple[str, str]] = set()
        owners: dict[tuple[str, str], set[str]] = {}
        duplicate_same_round = 0
        cross_task_repeat = 0
        prior_round_repeats = 0
        repeated_this_round = 0
        for edge in edges:
            if edge.get("round") != expected_round:
                raise ValueError("edge is stored in the wrong round")
            if edge.get("max_info_round") > expected_round - 1:
                raise ValueError("edge endpoints use current or future information")
            if edge.get("left") == edge.get("right"):
                raise ValueError("self-comparison was counted as an edge")
            if edge.get("left") not in items or edge.get("right") not in items:
                raise ValueError("edge references an unknown item")
            if edge.get("left_kind") != items[edge["left"]]["kind"]:
                raise ValueError("left endpoint type does not match trace header")
            if edge.get("right_kind") != items[edge["right"]]["kind"]:
                raise ValueError("right endpoint type does not match trace header")
            task_id = edge["task"]
            if task_id not in tasks:
                raise ValueError("edge has no frozen task descriptor")
            pair = norm(edge["left"], edge["right"])
            actual_by_task.setdefault(task_id, []).append(pair)
            left_real = items[edge["left"]]["kind"] == "real"
            right_real = items[edge["right"]]["kind"] == "real"
            if left_real and right_real:
                rr += 1
            else:
                dummy += 1
            if pair in all_seen:
                repeated_this_round += 1
            if pair not in same_round_pairs and pair in all_seen:
                prior_round_repeats += 1
            if pair in same_round_pairs:
                duplicate_same_round += 1
                if task_id not in owners[pair]:
                    cross_task_repeat += 1
            same_round_pairs.add(pair)
            owners.setdefault(pair, set()).add(task_id)
            all_seen.add(pair)

        for task_id, task in tasks.items():
            expected = expected_pairs(task, expected_round)
            actual_list = actual_by_task.get(task_id, [])
            if len(actual_list) != len(set(actual_list)):
                raise ValueError(f"round {expected_round}/{task_id}: duplicate edge within task")
            if set(actual_list) != expected:
                raise ValueError(
                    f"round {expected_round}/{task_id}: emitted edge set differs from the frozen rule"
                )
        unknown_task_edges = set(actual_by_task) - set(tasks)
        if unknown_task_edges:
            raise ValueError(f"edges have missing task descriptors: {sorted(unknown_task_edges)}")

        if cursor >= len(events) or events[cursor].get("event") != "ROUND_RESULTS":
            raise ValueError(f"round {expected_round}: missing results after frozen plan")
        result_event = events[cursor]
        cursor += 1
        if result_event.get("round") != expected_round:
            raise ValueError("round results are out of order")
        outcomes = result_event.get("left_lt_right", [])
        if len(outcomes) != len(edges):
            raise ValueError("result count differs from edge count")
        lookup: dict[tuple[str, str, str], bool] = {}
        for edge, outcome in zip(edges, outcomes):
            left_key = tuple(items[edge["left"]]["order_key"])
            right_key = tuple(items[edge["right"]]["order_key"])
            if bool(outcome) != (left_key < right_key):
                raise ValueError("comparison result differs from TEST_ONLY strict key")
            pair = norm(edge["left"], edge["right"])
            canonical = bool(outcome) if edge["left"] < edge["right"] else not bool(outcome)
            lookup[(edge["task"], pair[0], pair[1])] = canonical
        round_results[expected_round] = lookup
        calls = len(edges)
        total_calls += calls
        total_unique += len(same_round_pairs)
        total_repeated += repeated_this_round
        per_round[expected_round] = {
            "comparison_calls": calls,
            "real_real_calls": rr,
            "dummy_related_calls": dummy,
            "unique_unordered_edges_this_round": len(same_round_pairs),
            "duplicate_calls_same_round": duplicate_same_round,
            "cross_task_repeated_calls_same_round": cross_task_repeat,
            "repeated_calls_from_prior_rounds": prior_round_repeats,
        }

    if cursor != len(events):
        raise ValueError("unexpected events after round four")
    recorded = summary.get("round_metrics", {})
    for round_no in range(1, 5):
        expected_record = recorded.get(str(round_no), recorded.get(round_no, {}))
        for field, value in per_round[round_no].items():
            if expected_record.get(field) != value:
                raise ValueError(
                    f"round {round_no}: runtime count {field}={expected_record.get(field)} "
                    f"does not match independent count {value}"
                )
    aggregate = recorded.get("0", recorded.get(0, {}))
    if aggregate.get("total_comparison_calls") != total_calls:
        raise ValueError("aggregate comparison call count mismatch")
    if aggregate.get("unique_unordered_edges_all_rounds") != len(all_seen):
        raise ValueError("aggregate unique edge count mismatch")
    if aggregate.get("repeated_comparison_calls") != total_repeated:
        raise ValueError("aggregate repeated call count mismatch")
    return {
        "n": header["summary"]["n"],
        "K": header["summary"]["K"],
        "status": header["summary"]["status"],
        "audit": "PASS",
        "rounds": per_round,
        "total_comparison_calls": total_calls,
        "unique_unordered_edges_all_rounds": len(all_seen),
        "repeated_comparison_calls": total_repeated,
        "round_causality": "PASS",
    }


def audit_jsonl_gz(path: Path) -> list[dict[str, Any]]:
    import gzip

    result = []
    with gzip.open(path, "rt", encoding="utf-8") as stream:
        for line_no, line in enumerate(stream, 1):
            if not line.strip():
                continue
            record = json.loads(line)
            trace = record.get("trace", record)
            try:
                audited = audit_trace(trace)
                if "case_id" in record:
                    audited["case_id"] = record["case_id"]
                result.append(audited)
            except Exception as exc:
                raise RuntimeError(f"trace line {line_no}: {exc}") from exc
    return result


if __name__ == "__main__":
    import argparse
    import gzip

    parser = argparse.ArgumentParser()
    parser.add_argument("trace", type=Path)
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    if args.trace.suffix == ".gz":
        audits = audit_jsonl_gz(args.trace)
    else:
        record = json.loads(args.trace.read_text(encoding="utf-8"))
        trace = record.get("trace", record)
        audited = audit_trace(trace)
        if "case_id" in trace:
            audited["case_id"] = trace["case_id"]
        audits = [audited]
    data = "".join(json.dumps(record, sort_keys=True) + "\n" for record in audits)
    if args.out:
        args.out.write_text(data, encoding="utf-8")
    else:
        print(data, end="")
