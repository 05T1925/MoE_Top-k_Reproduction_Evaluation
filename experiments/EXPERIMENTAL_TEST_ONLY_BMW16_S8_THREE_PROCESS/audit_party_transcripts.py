#!/usr/bin/env python3
"""Independent accounting and causality audit of S8 party trace JSON.

This consumes captured TEST_ONLY traces only; it does not import or invoke the
Select implementation, uCMP backend, shuffle, or oracle.
"""
from __future__ import annotations

import argparse
import json
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any


def audit_row(row: dict[str, Any]) -> dict[str, Any]:
    n = int(row["n"])
    if row.get("status") != "SUCCESS":
        return {"case_id": row.get("case_id"), "status": row.get("status"),
                "trace_audit": "NOT_APPLICABLE_NON_SUCCESS"}
    p0, p1 = row["party_trace_P0"], row["party_trace_P1"]
    if p0 != p1:
        raise ValueError(f"{row['case_id']}: P0/P1 traces differ")
    events = p0["events"]
    plans = [e for e in events if e.get("event") == "ROUND_PLAN_FROZEN"]
    results = [e for e in events if e.get("event") == "ROUND_RESULTS"]
    if len(plans) != 4 or len(results) != 4:
        raise ValueError(f"{row['case_id']}: expected four frozen plans/results")
    items = {int(x["id"]): x for x in p0["items"]}
    seen_prior: set[tuple[int, int]] = set()
    round_rows = []
    total_calls = total_real = total_dummy = total_slots = 0
    for expected_round, (plan, result) in enumerate(zip(plans, results), start=1):
        if plan["round"] != expected_round or plan["requires_results_through_round"] != expected_round - 1:
            raise ValueError(f"{row['case_id']}: round causality marker mismatch")
        if result["round"] != expected_round:
            raise ValueError(f"{row['case_id']}: result follows wrong round")
        # Event order must place this round's results after its frozen graph,
        # and before the next round's plan is frozen.
        if events.index(plan) >= events.index(result):
            raise ValueError(f"{row['case_id']}: plan was not frozen before results")
        if expected_round < 4 and events.index(result) >= events.index(plans[expected_round]):
            raise ValueError(f"{row['case_id']}: next-round graph preceded prior results")
        edges = plan["edges"]
        if len(edges) != plan["edge_count"] or result["outcome_count"] != len(edges):
            raise ValueError(f"{row['case_id']}: edge/result cardinality mismatch")
        real_edges: list[tuple[str, int, int]] = []
        dummy = 0
        reverse = 0
        same_round_unique: set[tuple[int, int]] = set()
        task_unique: set[tuple[str, int, int]] = set()
        prior_repeats = 0
        for task, left_raw, right_raw in edges:
            left, right = int(left_raw), int(right_raw)
            if left not in items or right not in items:
                raise ValueError(f"{row['case_id']}: edge endpoint missing from item table")
            pair = tuple(sorted((left, right)))
            if pair in seen_prior:
                prior_repeats += 1
            same_round_unique.add(pair)
            if items[left]["category"] == 2 and items[right]["category"] == 2:
                real_edges.append((str(task), left, right))
                lo, hi = sorted((left, right))
                task_unique.add((str(task), lo, hi))
                if left > right:
                    reverse += 1
            else:
                dummy += 1
        slots = len(task_unique)
        logical, real, reported_dummy = len(edges), len(real_edges), dummy
        metrics = row["select_round_metrics"][str(expected_round)]
        checks = {
            "comparison_calls": logical,
            "real_real_calls": real,
            "dummy_related_calls": reported_dummy,
            "fss_material_slots": slots,
            "reverse_share_complements": reverse,
            "prior_round_repeat_calls": prior_repeats,
        }
        for field, actual in checks.items():
            if int(metrics[field]) != actual:
                raise ValueError(f"{row['case_id']} R{expected_round}: {field} {metrics[field]} != independently recomputed {actual}")
        total_calls += logical
        total_real += real
        total_dummy += dummy
        total_slots += slots
        seen_prior.update(same_round_unique)
        round_rows.append({"round": expected_round, "logical_edges": logical,
                           "real_real_edges": real, "dummy_related_edges": dummy,
                           "unique_task_real_edges_and_key_slots": slots,
                           "cross_round_repeat_calls": prior_repeats,
                           "reverse_share_complement_calls": reverse,
                           "requires_results_through_round": expected_round - 1})
    if total_calls != sum(int(x["comparison_calls"]) for x in row["select_round_metrics"].values()):
        raise ValueError(f"{row['case_id']}: total logical call sum mismatch")
    if total_slots != int(row["logical_and_actual_metrics_P0"]["select_party_uCMP_key_slots_consumed"]):
        raise ValueError(f"{row['case_id']}: actual uCMP slot total mismatch")
    mem = int(row["logical_and_actual_metrics_P0"]["membership_party_key_slots_consumed"])
    if mem != max(0, n - 1):
        raise ValueError(f"{row['case_id']}: membership star slot count mismatch")
    expected_eval_both = 4 * (total_slots + mem)
    if int(row["logical_and_actual_metrics_P0"]["DCF_Eval_both_parties"]) != expected_eval_both:
        raise ValueError(f"{row['case_id']}: both-party DCF eval formula mismatch")
    metrics = row["logical_and_actual_metrics_P0"]
    raw_slots = int(metrics.get("raw_score_input_uCMP_key_slots_per_party", 0))
    raw_eval_party = int(metrics.get("raw_score_input_DCF_Eval_per_party", 0))
    raw_eval_both = int(metrics.get("raw_score_input_DCF_Eval_both_parties", 0))
    raw_phases = int(metrics.get("raw_score_input_peer_message_phases", 0))
    if (raw_eval_party != 2 * raw_slots or raw_eval_both != 4 * raw_slots or
            raw_phases not in (0, 2)):
        raise ValueError(f"{row['case_id']}: raw-score material/eval ledger mismatch")
    if raw_slots and raw_slots != 2 * (1 << max(1, (n - 1).bit_length())):
        raise ValueError(f"{row['case_id']}: raw-score padded slot count mismatch")
    random_p0 = row.get("algorithm_randomness_P0", {}) or {}
    random_p1 = row.get("algorithm_randomness_P1", {}) or {}
    joint_phases = int(random_p0.get("joint_coin_message_phases", 0))
    if joint_phases not in (0, 1):
        raise ValueError(f"{row['case_id']}: joint random coin phase count")
    if joint_phases and random_p0.get("seed_sha256") != random_p1.get("seed_sha256"):
        raise ValueError(f"{row['case_id']}: parties did not share the same sampling seed")
    expected_phases = 1 if n == 1 else 13 + raw_phases + joint_phases
    if row["full_online_DAG_message_phases"] != expected_phases:
        raise ValueError(f"{row['case_id']}: full-protocol message phase mismatch")
    if row.get("original_index_opened") or row.get("shuffle_map_opened"):
        raise ValueError(f"{row['case_id']}: party result claims mapping opened")
    return {"case_id": row["case_id"], "trace_audit": "PASS",
            "rounds": round_rows, "logical_edges": total_calls,
            "real_real_edges": total_real, "dummy_related_edges": total_dummy,
            "fss_slots": total_slots, "membership_slots": mem,
            "dcf_eval_both_parties": expected_eval_both,
            "full_online_message_phases": expected_phases}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("inputs", nargs="+", type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    rows = []
    for path in args.inputs:
        data = json.loads(path.read_text(encoding="utf-8"))
        candidates = data.get("matrix_cases", [data])
        rows.extend(audit_row(row) for row in candidates)
    out = {"label": "BMW16_S8_INDEPENDENT_PARTY_TRACE_EDGE_AUDIT_TEST_ONLY",
           "audit_source": "serialized party trace JSON; no algorithm imports",
           "rows": rows,
           "summary": {"pass": sum(x["trace_audit"].startswith("PASS") for x in rows),
                       "not_applicable_non_success": sum(x["trace_audit"] == "NOT_APPLICABLE_NON_SUCCESS" for x in rows),
                       "total": len(rows)}}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(out, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(out["summary"], sort_keys=True))
    return 0 if out["summary"]["pass"] == len(rows) else 1


if __name__ == "__main__":
    raise SystemExit(main())
