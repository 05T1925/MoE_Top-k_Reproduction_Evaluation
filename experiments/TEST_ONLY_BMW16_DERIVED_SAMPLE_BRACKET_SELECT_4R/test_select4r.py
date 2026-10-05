#!/usr/bin/env python3
"""Exhaustive small-order and adversarial stable-select differential validator."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import itertools
import json
import random
import sys
from collections import Counter
from pathlib import Path

from select4r import INT32_MAX, INT32_MIN, LABEL, run_select


HERE = Path(__file__).resolve().parent
ORACLE_PATH = HERE.parents[1] / "experiments" / "TEST_ONLY_BMW16_DERIVED_SELECT_4R" / "oracle.py"
spec = importlib.util.spec_from_file_location("frozen_bmw16_oracle_exhaustive", ORACLE_PATH)
if spec is None or spec.loader is None:
    raise RuntimeError(f"unable to load frozen S1 oracle: {ORACLE_PATH}")
oracle = importlib.util.module_from_spec(spec)
spec.loader.exec_module(oracle)


def digest_scores(scores: list[int]) -> str:
    return hashlib.sha256(json.dumps(scores, separators=(",", ":")).encode()).hexdigest()


def seed64(label: str) -> int:
    return int.from_bytes(hashlib.sha256(f"BMW16-S4-small|{label}".encode()).digest()[:8], "big")


def cases():
    # Every strict-order permutation and every K for n <= 7.
    for n in range(1, 8):
        for permutation_id, scores in enumerate(itertools.permutations(range(n))):
            values = list(scores)
            for k in range(1, n + 1):
                label = f"exhaustive_n{n}_perm{permutation_id}_k{k}"
                yield label, values, k, seed64(label)

    # Duplicates, all equal, negative scores, signed extremes, odd/even and K endpoints.
    adversarial = {
        "all_equal_5": [0] * 5,
        "all_equal_17": [INT32_MIN] * 17,
        "duplicate_ties": [7, -2, 7, 0, -2, 7, INT32_MAX, INT32_MIN, 0],
        "signed_edges": [INT32_MIN, INT32_MAX, -1, 0, 1, INT32_MIN, INT32_MAX],
        "odd_non_power": [9, -9, 9, 4, 4, INT32_MIN, INT32_MAX, 0, 0, -1, 13],
        "even_non_power": [5, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0],
    }
    for name, values in adversarial.items():
        for k in sorted({1, (len(values) + 1) // 2, len(values), max(1, len(values) - 1)}):
            label = f"adversarial_{name}_k{k}"
            yield label, values, k, seed64(label)

    large_adversarial = {
        "n128_all_equal": [7] * 128,
        "n256_repeated_and_edges": [
            (INT32_MIN, -1, 0, 0, 1, INT32_MAX)[i % 6] for i in range(256)
        ],
        "n1000_all_equal": [INT32_MAX] * 1000,
        "n1000_alternating_edges": [INT32_MIN if i % 2 else INT32_MAX for i in range(1000)],
    }
    for name, values in large_adversarial.items():
        n = len(values)
        for k in sorted({1, 2, 8, (n + 1) // 2, n - 1, n}):
            label = f"large_adversarial_{name}_k{k}"
            yield label, values, k, seed64(label)

    # Reproducible coin-only repetitions on duplicate-heavy and boundary-heavy inputs.
    for pattern, values in (
        ("three_value_ties", [(-1, 0, 1)[i % 3] for i in range(65)]),
        ("alternating_extremes", [INT32_MIN if i % 2 else INT32_MAX for i in range(67)]),
    ):
        k = 8
        for repetition in range(100):
            label = f"coin_only_{pattern}_rep{repetition}"
            yield label, values, k, seed64(label)


def main() -> int:
    parser = argparse.ArgumentParser(description="Run exhaustive TEST_ONLY Select differential cases")
    parser.add_argument("--out-dir", type=Path, required=True, help="external evidence directory")
    args = parser.parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=True)
    output = args.out_dir / "small_and_adversarial_cases.jsonl"
    counts: Counter[str] = Counter()
    failures = []
    total = 0
    with output.open("w", encoding="utf-8", newline="\n") as handle:
        for label, scores, k, algorithm_seed in cases():
            row = run_select(scores, k, input_seed=seed64(label + "|input"),
                             algorithm_seed=algorithm_seed, trace=False)
            expected = oracle.select_index(scores, k)
            if row["status"] == "SUCCESS":
                validation = "SUCCESS" if row["selected_original_index"] == expected else "COMPLETED_WRONG"
            else:
                validation = row["status"]
            record = {
                "case": label, "n": len(scores), "K": k,
                "input_seed": seed64(label + "|input"), "algorithm_seed": algorithm_seed,
                "input_sha256": digest_scores(scores),
                "algorithm_status": row["status"], "validation_status": validation,
                "candidate_original_index": row["selected_original_index"],
                "oracle_original_index": expected,
                "failure_reason": next((p["failure_reason"] for p in row["partitions"].values()
                                         if p["failure_reason"]), None),
                "total_comparison_calls": row["total_comparison_calls"],
                "round_metrics": row["round_metrics"],
            }
            handle.write(json.dumps(record, sort_keys=True, separators=(",", ":")) + "\n")
            counts[validation] += 1
            if validation != "SUCCESS":
                failures.append(record)
            total += 1
    failure_path = args.out_dir / "small_and_adversarial_failures.jsonl"
    with failure_path.open("w", encoding="utf-8", newline="\n") as handle:
        for row in failures:
            handle.write(json.dumps(row, sort_keys=True, separators=(",", ":")) + "\n")
    summary = {
        "label": LABEL, "total_cases": total, "status_counts": dict(sorted(counts.items())),
        "failure_count": len(failures), "results_sha256": hashlib.sha256(output.read_bytes()).hexdigest(),
        "failures_sha256": hashlib.sha256(failure_path.read_bytes()).hexdigest(),
        "strict_permutation_scope": "all n! strict-order permutations and all 1<=K<=n for n=1..7",
        "oracle": str(ORACLE_PATH),
    }
    summary_path = args.out_dir / "small_and_adversarial_summary.json"
    summary_path.write_text(json.dumps(summary, sort_keys=True, indent=2), encoding="utf-8")
    print(json.dumps(summary, sort_keys=True))
    return 0 if not any(status in counts for status in ("COMPLETED_WRONG",)) else 1


if __name__ == "__main__":
    raise SystemExit(main())
