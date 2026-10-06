#!/usr/bin/env python3
"""Frozen deterministic TEST_ONLY matrix; retain every SUCCESS and failure row."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

from compose_testonly import run_case


MIN = -(1 << 31)
MAX = (1 << 31) - 1


CASES: list[tuple[str, list[int], int, int]] = [
    ("singleton_min_k1", [MIN], 1, 3),
    ("n2_extremes_k1", [MIN, MAX], 1, 5),
    ("n2_extremes_k2", [MAX, MIN], 2, 7),
    ("n3_all_equal_k2", [9, 9, 9], 2, 11),
    ("n5_mixed_k1", [MIN, 0, MAX, 0, -1], 1, 13),
    ("n5_mixed_kn", [MIN, 0, MAX, 0, -1], 5, 17),
    ("n7_repeated_k3", [-7, 4, 4, MIN, MAX, 0, 4], 3, 19),
    ("n8_mixed_k3", [MIN, MAX, 0, -1, 0, MAX, MIN, 7], 3, 42),
    ("n8_all_equal_k4", [5] * 8, 4, 123),
    ("n16_boundary_k8", [MIN, MAX, 0, -1, 0, MAX, MIN, 7] * 2, 8, 1024),
]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--fss-backend", required=True,
                        help="native executable path or wsl:/absolute/linux/path")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    rows: list[dict[str, Any]] = []
    for serial, (name, scores, k, algorithm_seed) in enumerate(CASES):
        result = run_case(
            scores, k, input_seed=serial + 100, algorithm_seed=algorithm_seed,
            shuffle_seed=serial + 700, fss_seed=serial + 9000,
            executable=args.fss_backend, trace=(name == "n8_mixed_k3"),
        )
        result["case_name"] = name
        rows.append(result)
        print(f"{name}: status={result['status']} oracle={result['oracle_differential']} "
              f"select_calls={result['select_logical_comparison_calls']} "
              f"slots={result['select_unique_one_shot_uCMP_material_slots']}")
    summary = {
        "label": "BMW16_S7_REAL_UCMP_DCF_AND_SHUFFLE_TEST_ONLY_MATRIX",
        "cases_requested": len(CASES),
        "cases_retained": len(rows),
        "successes": sum(row["status"] == "SUCCESS" for row in rows),
        "paper_random_failure_paths": sum(row["status"] == "PROJECT_RANDOM_FAILURE_PATH" for row in rows),
        "completed_wrong_or_other": sum(row["status"] not in {"SUCCESS", "PROJECT_RANDOM_FAILURE_PATH"}
                                         for row in rows),
        "all_rows_retained": True,
        "interpretation": "Finite functional evidence only; not a probability estimate, theorem, independent-process E2E, or security proof.",
        "cases": rows,
    }
    args.output.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print("summary", {key: summary[key] for key in (
        "cases_requested", "successes", "paper_random_failure_paths", "completed_wrong_or_other")})
    return 0 if summary["completed_wrong_or_other"] == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
