#!/usr/bin/env python3
"""Create a compact, secret-free evidence index from TEST_ONLY run JSON."""
from __future__ import annotations

import argparse
import hashlib
import json
import platform
import sys
from pathlib import Path
from typing import Any


def read_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def raw_identity(path: Path) -> dict[str, Any]:
    return {"path": str(path), "bytes": path.stat().st_size,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def one_case(row: dict[str, Any]) -> dict[str, Any]:
    return {key: row.get(key) for key in (
        "case_id", "n", "K", "comparison_bits", "status", "oracle_differential",
        "party_exit_codes", "full_online_DAG_message_phases", "offline_package_bytes_per_party",
        "offline_material_slot_breakdown_per_party", "select_round_metrics",
        "os_identity_isolation_checks", "offline_material_conformance")}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--matrix", type=Path, required=True)
    parser.add_argument("--n64", type=Path, required=True)
    parser.add_argument("--forced-abort", type=Path, required=True)
    parser.add_argument("--uid-worlds", type=Path, required=True)
    parser.add_argument("--s9-matrix", type=Path, required=True)
    parser.add_argument("--s9-n64", type=Path, required=True)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    matrix = read_json(args.matrix)
    n64 = read_json(args.n64)
    abort = read_json(args.forced_abort)
    worlds = read_json(args.uid_worlds)
    s9_matrix = read_json(args.s9_matrix)
    s9_n64 = read_json(args.s9_n64)
    current_cases = [one_case(row) for row in matrix["matrix_cases"]]
    s9_cases = [one_case(row) for row in s9_matrix["matrix_cases"]]
    summary = {
        "label": "BMW16_S10_FORMAL_RAW_SCORE_UID_ISOLATED_EXPERIMENTAL_TEST_ONLY",
        "protocol_identity": "Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED",
        "security_status": "EXPERIMENTAL_TEST_ONLY; no secure entry; no BMW16 Theorem 8 inheritance",
        "environment": {"python": sys.version.split()[0], "platform": platform.platform()},
        "s9_independent_rerun": {
            "matrix_identity": raw_identity(args.s9_matrix),
            "matrix_summary": s9_matrix.get("summary"),
            "cases": s9_cases,
            "n64_identity": raw_identity(args.s9_n64),
            "n64": one_case(s9_n64),
            "scope": "S9 source behavior rerun before S10 changes; copied raw-score lift path, not official adapter composition",
        },
        "s10_formal_adapter_isolated_matrix": {
            "raw_identity": raw_identity(args.matrix),
            "summary": matrix.get("summary"),
            "cases": current_cases,
            "scope": "11 frozen n<=8 cases; native official raw-score adapter; distinct Linux UID process run; CSPRNG sample coins",
        },
        "s10_n64_nonfull_sampling": {
            "raw_identity": raw_identity(args.n64),
            "case": one_case(n64),
            "p0_metrics": n64.get("logical_and_actual_metrics_P0"),
            "scope": "one fixed n=64 TEST_ONLY sample; not a performance benchmark or failure-rate estimate",
        },
        "s10_forced_abort": {
            "raw_identity": raw_identity(args.forced_abort),
            "status": abort.get("status"),
            "no_mask": abort.get("no_mask"),
            "oracle_differential": abort.get("oracle_differential"),
            "party_exit_codes": abort.get("party_exit_codes"),
            "forced_injection": abort.get("test_only_forced_algorithm_abort"),
        },
        "s10_two_world_uid_access_test": worlds,
        "binary": raw_identity(args.binary),
        "raw_json_policy": "Full run JSON stays in WSL /tmp or Windows TEMP; this file excludes party stdout/stderr, open masks, shares, keys, and score values.",
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(summary, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(args.output), "sha256": raw_identity(args.output)["sha256"],
                      "bytes": args.output.stat().st_size}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
