#!/usr/bin/env python3
"""Independent frozen-oracle differential and exact one-sided CP summaries."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import math
import random
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any

from select4r import INT32_MAX, INT32_MIN


ROOT_SEED = "BMW16-S4-2026-10-05-v1"
HERE = Path(__file__).resolve().parent
ORACLE_PATH = HERE.parents[1] / "experiments" / "TEST_ONLY_BMW16_DERIVED_SELECT_4R" / "oracle.py"
spec = importlib.util.spec_from_file_location("frozen_bmw16_oracle", ORACLE_PATH)
if spec is None or spec.loader is None:
    raise RuntimeError(f"unable to load frozen S1 oracle at {ORACLE_PATH}")
oracle = importlib.util.module_from_spec(spec)
spec.loader.exec_module(oracle)


def seed64(domain: str) -> int:
    return int.from_bytes(hashlib.sha256(f"{ROOT_SEED}|{domain}".encode("ascii")).digest()[:8], "big")


def make_scores(n: int, input_seed: int) -> list[int]:
    rng = random.Random(input_seed)
    return [rng.randrange(INT32_MIN, INT32_MAX + 1) for _ in range(n)]


def binomial_tail_at_least(k: int, n: int, p: float) -> float:
    if k <= 0:
        return 1.0
    if k > n or p <= 0:
        return 0.0
    if p >= 1:
        return 1.0
    log_term = (
        math.lgamma(n + 1) - math.lgamma(k + 1) - math.lgamma(n - k + 1)
        + k * math.log(p) + (n - k) * math.log1p(-p)
    )
    term = math.exp(log_term)
    total = term
    for i in range(k, n):
        term *= ((n - i) / (i + 1)) * (p / (1 - p))
        total += term
    return min(1.0, total)


def clopper_pearson_lower(successes: int, trials: int, alpha: float = 0.05) -> float:
    if trials <= 0 or successes <= 0:
        return 0.0
    lo, hi = 0.0, successes / trials
    for _ in range(100):
        mid = (lo + hi) / 2
        if binomial_tail_at_least(successes, trials, mid) > alpha:
            hi = mid
        else:
            lo = mid
    return lo


def _jsonl(path: Path):
    with path.open(encoding="utf-8") as handle:
        for line in handle:
            if line.strip():
                yield json.loads(line)


def main() -> int:
    parser = argparse.ArgumentParser(description="Oracle-check TEST_ONLY Select results")
    parser.add_argument("--out-dir", type=Path, required=True, help="same external evidence directory as run_matrix.py")
    parser.add_argument("--results", type=Path)
    parser.add_argument("--schedule", type=Path)
    args = parser.parse_args()
    result_path = args.results or args.out_dir / "algorithm_results.jsonl"
    schedule_path = args.schedule or args.out_dir / "seed_schedule.json"
    schedule = json.loads(schedule_path.read_text(encoding="utf-8"))
    planned = {(r["family"], r["config_id"], r["repetition"]): r for r in schedule["rows"]}
    rows = {(r["family"], r["config_id"], r["repetition"]): r for r in _jsonl(result_path)}
    if set(rows) != set(planned):
        raise SystemExit(f"schedule/results mismatch: planned={len(planned)}, results={len(rows)}")

    grouped: dict[tuple[str, str], list[dict[str, Any]]] = defaultdict(list)
    output_rows = []
    for key in sorted(planned):
        job, row = planned[key], rows[key]
        scores = make_scores(job["n"], job["input_seed"])
        digest = hashlib.sha256(json.dumps(scores, separators=(",", ":")).encode()).hexdigest()
        if row.get("input_seed") != job["input_seed"] or row.get("algorithm_seed") != job["algorithm_seed"]:
            raise SystemExit(f"seed mismatch for {key}")
        if row.get("input_sha256") != digest:
            raise SystemExit(f"generated input hash mismatch for {key}")
        expected = oracle.select_index(scores, job["K"])
        if row.get("status") == "SUCCESS":
            candidate = row.get("selected_original_index")
            status = "SUCCESS" if candidate == expected else "COMPLETED_WRONG"
        elif row.get("status") in ("COMPLETED_WRONG", "RESOURCE_FAILURE", "TIMEOUT"):
            candidate = row.get("selected_original_index")
            status = row["status"]
        else:
            candidate = row.get("selected_original_index")
            status = row.get("status", "UNCLASSIFIED")
        checked = {
            "family": job["family"], "config_id": job["config_id"], "repetition": job["repetition"],
            "n": job["n"], "K": job["K"], "input_seed": job["input_seed"],
            "algorithm_seed": job["algorithm_seed"], "input_sha256": digest,
            "algorithm_status": row.get("status"), "validation_status": status,
            "candidate_original_index": candidate, "oracle_original_index": expected,
            "failure_reason": row.get("failure_reason") or next((p.get("failure_reason") for p in row.get("partitions", {}).values() if p.get("failure_reason")), None),
            "total_comparison_calls": row.get("total_comparison_calls"),
            "round_metrics": row.get("round_metrics"),
        }
        output_rows.append(checked)
        grouped[(job["family"], job["config_id"])].append(checked)

    validation_path = args.out_dir / "oracle_validation.jsonl"
    failure_path = args.out_dir / "oracle_failures.jsonl"
    with validation_path.open("w", encoding="utf-8", newline="\n") as handle:
        for row in output_rows:
            handle.write(json.dumps(row, sort_keys=True, separators=(",", ":")) + "\n")
    bad = [row for row in output_rows if row["validation_status"] != "SUCCESS"]
    with failure_path.open("w", encoding="utf-8", newline="\n") as handle:
        for row in bad:
            handle.write(json.dumps(row, sort_keys=True, separators=(",", ":")) + "\n")

    summaries = []
    for (family, config), members in sorted(grouped.items()):
        statuses = Counter(row["validation_status"] for row in members)
        successes = statuses["SUCCESS"]
        lower = clopper_pearson_lower(successes, len(members))
        comparisons = [row["total_comparison_calls"] for row in members if row["total_comparison_calls"] is not None]
        summaries.append({
            "family": family, "config_id": config, "n": members[0]["n"], "K": members[0]["K"],
            "trials": len(members), "successes": successes, "failure_count": len(members) - successes,
            "status_counts": dict(sorted(statuses.items())),
            "one_sided_95_percent_CP_lower": lower,
            "provisional_99_percent_gate": lower >= 0.99,
            "total_comparison_calls": {
                "min": min(comparisons) if comparisons else None,
                "median": sorted(comparisons)[len(comparisons) // 2] if comparisons else None,
                "max": max(comparisons) if comparisons else None,
                "sum": sum(comparisons) if comparisons else None,
            },
            "failure_seed_pairs": [
                {"repetition": row["repetition"], "input_seed": row["input_seed"],
                 "algorithm_seed": row["algorithm_seed"], "status": row["validation_status"]}
                for row in members if row["validation_status"] != "SUCCESS"
            ],
        })
    summary = {
        "oracle": "frozen S1 Python stable Select oracle; used only in this separate validation process",
        "oracle_path": str(ORACLE_PATH),
        "seed_schedule_sha256": hashlib.sha256(schedule_path.read_bytes()).hexdigest(),
        "trials_total": len(output_rows), "successes_total": len(output_rows) - len(bad),
        "failures_total": len(bad),
        "oracle_validation_sha256": hashlib.sha256(validation_path.read_bytes()).hexdigest(),
        "oracle_failures_sha256": hashlib.sha256(failure_path.read_bytes()).hexdigest(),
        "groups": summaries,
    }
    summary_path = args.out_dir / "oracle_validation_summary.json"
    summary_path.write_text(json.dumps(summary, sort_keys=True, indent=2), encoding="utf-8")
    print(json.dumps({"trials": len(output_rows), "successes": len(output_rows) - len(bad), "failures": len(bad),
                      "groups": [{"family": x["family"], "config_id": x["config_id"], "successes": x["successes"],
                                  "trials": x["trials"], "CP_lower": x["one_sided_95_percent_CP_lower"],
                                  "gate": x["provisional_99_percent_gate"]} for x in summaries]}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
