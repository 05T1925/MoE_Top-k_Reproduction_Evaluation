#!/usr/bin/env python3
"""Exhaustive and adversarial differential tests against VFSS's frozen oracle."""

from __future__ import annotations

import argparse
import hashlib
import itertools
import json
import os
import platform
import random
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Iterator

from audit_edges import audit_trace
from select5iter import ALGORITHM_LABEL, ALGORITHM_REVISION, INT32_MAX, INT32_MIN, select_kth


ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent


def _score_hash(scores: list[int]) -> str:
    return hashlib.sha256(" ".join(str(value) for value in scores).encode("ascii")).hexdigest()


def _cases() -> Iterator[tuple[str, list[int], int, int, int]]:
    # Every strict-order permutation and every K for every n <= 7.
    case_no = 0
    for n in range(1, 8):
        for permutation_no, scores in enumerate(itertools.permutations(range(n))):
            values = list(scores)
            for k in range(1, n + 1):
                yield ("exhaustive_permutation", values, k,
                       permutation_no, (n << 48) | (permutation_no << 8) | k)
                case_no += 1

    fixed_patterns = [
        [0],
        [0, 0],
        [0, 0, 0, 0, 0, 0, 0],
        [INT32_MIN, INT32_MAX, -1, 0, 1, INT32_MIN, INT32_MAX],
        [(-1 if i % 2 else 1) * (1 << 12) for i in range(31)],
        [((i % 5) - 2) * (1 << 12) for i in range(65)],
        [INT32_MIN if i % 7 == 0 else INT32_MAX if i % 7 == 1 else (i % 3) - 1
         for i in range(257)],
    ]
    input_seed = 1000
    for pattern_no, values in enumerate(fixed_patterns):
        ks = sorted({1, max(1, (len(values) + 1) // 2), len(values)})
        for k in ks:
            for algorithm_seed in (0, 1, 0x5A17, 0xC0FFEE):
                yield (f"fixed_pattern_{pattern_no}", values, k, input_seed, algorithm_seed)
            input_seed += 1

    # Hold an input fixed while changing only the algorithm randomness.
    fixed = [((i * 7919) % 10007) - 5003 for i in range(257)]
    for algorithm_seed in range(32):
        yield ("fixed_input_algorithm_seed_sweep", fixed, 129, 777, algorithm_seed)

    # Independent input seeds exercise different fixed inputs and tie densities.
    for input_seed in (17, 2026, 65537):
        for distribution_id, mode in enumerate(("wide", "ties", "boundary")):
            source = random.Random(input_seed ^ (distribution_id << 24))
            if mode == "wide":
                values = [source.randint(INT32_MIN, INT32_MAX) for _ in range(129)]
            elif mode == "ties":
                values = [source.randint(-3 * 4096, 3 * 4096) for _ in range(129)]
            else:
                boundary = [INT32_MIN, INT32_MIN + 1, -(1 << 12), -1, 0, 1,
                            1 << 12, INT32_MAX - 1, INT32_MAX]
                values = [source.choice(boundary) for _ in range(129)]
            for k in (1, 2, 64, 65, 128, 129):
                yield (f"random_{mode}", values, k, input_seed, 9000 + input_seed + k)

    yield ("representative_trace", [4096 if i % 2 == 0 else -4096 for i in range(31)], 16, 1010, 0)


def _compile_oracle(out_dir: Path) -> tuple[Path, list[str]]:
    compiler = shutil.which(os.environ.get("CXX", "g++")) or shutil.which("clang++")
    if compiler is None:
        raise RuntimeError("g++/clang++ is required to run frozen-oracle differential")
    exe = out_dir / ("frozen_oracle.exe" if os.name == "nt" else "frozen_oracle")
    command = [compiler, "-std=c++17", "-O2", "-I", str(ROOT / "VFSS" / "include"),
               str(HERE / "frozen_oracle.cpp"), "-o", str(exe)]
    completed = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, check=False)
    if completed.returncode != 0:
        raise RuntimeError(f"frozen oracle compile failed: {completed.stderr}")
    return exe, command


def _invalid_input_checks() -> list[dict[str, object]]:
    invalid = [([], 1), ([1], 0), ([1], 2), ([True], 1), ([INT32_MAX + 1], 1)]
    records: list[dict[str, object]] = []
    for scores, k in invalid:
        try:
            select_kth(scores, k)  # type: ignore[arg-type]
        except ValueError as error:
            records.append({"scores": scores, "k": k, "status": "REJECTED", "error": str(error)})
        else:
            records.append({"scores": scores, "k": k, "status": "UNEXPECTED_SUCCESS"})
    return records


def main() -> int:
    parser = argparse.ArgumentParser(description=ALGORITHM_LABEL)
    parser.add_argument("--out-dir", required=True, type=Path,
                        help="external evidence directory; no generated files are written into the repository")
    args = parser.parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=True)
    failures_path = args.out_dir / "failures.jsonl"
    results_path = args.out_dir / "results.jsonl"
    invalid_path = args.out_dir / "invalid_input_checks.jsonl"
    summary_path = args.out_dir / "summary.json"
    failures_path.write_text("", encoding="utf-8")
    results_path.write_text("", encoding="utf-8")
    (args.out_dir / "representative_trace.json").write_text("", encoding="utf-8")
    (args.out_dir / "representative_trace_audit.json").write_text("", encoding="utf-8")

    oracle, compile_command = _compile_oracle(args.out_dir)
    cases = list(_cases())
    oracle_input = "".join(
        f"{len(scores)} {k} " + " ".join(str(value) for value in scores) + "\n"
        for _, scores, k, _, _ in cases
    )
    oracle_run = subprocess.run([str(oracle)], input=oracle_input, text=True,
                                capture_output=True, cwd=ROOT, check=False)
    if oracle_run.returncode != 0:
        raise RuntimeError(f"frozen oracle failed with exit={oracle_run.returncode}: {oracle_run.stderr}")
    oracle_rows = oracle_run.stdout.splitlines()
    if len(oracle_rows) != len(cases):
        raise RuntimeError(f"frozen oracle returned {len(oracle_rows)} lines for {len(cases)} cases")

    counts: dict[str, int] = {}
    audited_cases = 0
    selected_sample_signatures: set[tuple[int, ...]] = set()
    all_five_active_cases = 0
    active_exact_final_cases = 0
    cases_checked = 0
    with results_path.open("w", encoding="utf-8", newline="\n") as results_file, \
            failures_path.open("a", encoding="utf-8", newline="\n") as failures_file:
        for case, oracle_row in zip(cases, oracle_rows):
            kind, scores, k, input_seed, algorithm_seed = case
            counts[kind] = counts.get(kind, 0) + 1
            result = None
            try:
                result = select_kth(scores, k, input_seed=input_seed,
                                    algorithm_seed=algorithm_seed, with_trace=True)
                audit = audit_trace(result.trace or {})
                expected_fields = oracle_row.split()
                if len(expected_fields) != 2:
                    raise AssertionError(f"malformed frozen oracle row: {oracle_row!r}")
                expected_index = int(expected_fields[0])
                expected_mask = [int(bit) for bit in expected_fields[1]]
                if result.selected_index != expected_index or result.mask != expected_mask:
                    raise AssertionError("selector result differs from frozen C++ oracle")
                if result.logical_comparisons != audit["logical_comparison_calls"]:
                    raise AssertionError("logical comparison count differs from independent edge audit")
                if result.unique_unordered_pairs != audit["unique_unordered_pairs"]:
                    raise AssertionError("unique edge count differs from independent edge audit")
                if result.repeated_comparison_calls != audit["repeated_comparison_calls"]:
                    raise AssertionError("cross-round duplicate count differs from independent edge audit")
                if result.per_round_comparisons != audit["frozen_edge_count_per_round"]:
                    raise AssertionError("per-round edge count differs from independent edge audit")
                if result.per_round_comparisons[4] > 0:
                    active_exact_final_cases += 1
                if all(count > 0 for count in result.per_round_comparisons):
                    all_five_active_cases += 1
                if kind == "representative_trace":
                    if not all(count > 0 for count in result.per_round_comparisons):
                        raise AssertionError("representative trace must exercise all five comparison layers")
                    (args.out_dir / "representative_trace.json").write_text(
                        json.dumps(result.trace, separators=(",", ":")) + "\n", encoding="utf-8")
                    (args.out_dir / "representative_trace_audit.json").write_text(
                        json.dumps(audit, indent=2) + "\n", encoding="utf-8")
                if kind == "fixed_input_algorithm_seed_sweep":
                    selected_sample_signatures.add(tuple(result.trace["rounds"][0]["sample_ids"]))
                results_file.write(json.dumps({
                    "kind": kind,
                    "n": len(scores),
                    "k": k,
                    "input_seed": input_seed,
                    "algorithm_seed": algorithm_seed,
                    "scores_sha256": _score_hash(scores),
                    "status": "SUCCESS",
                    "selected_index": result.selected_index,
                    "mask_ones": sum(result.mask),
                    "logical_comparisons": result.logical_comparisons,
                    "unique_unordered_pairs": result.unique_unordered_pairs,
                    "repeated_comparison_calls": result.repeated_comparison_calls,
                    "per_round_comparisons": result.per_round_comparisons,
                    "edge_audit": "PASS",
                    "oracle": "PASS",
                }, separators=(",", ":")) + "\n")
                audited_cases += 1
            except Exception as error:  # retain every failure with original input, seeds, and trace when available
                trace = result.trace if result is not None else None
                failures_file.write(json.dumps({
                    "kind": kind,
                    "scores": scores,
                    "k": k,
                    "input_seed": input_seed,
                    "algorithm_seed": algorithm_seed,
                    "oracle_row": oracle_row,
                    "error_type": type(error).__name__,
                    "error": str(error),
                    "trace": trace,
                }, separators=(",", ":")) + "\n")
            cases_checked += 1

    invalid_records = _invalid_input_checks()
    invalid_path.write_text("".join(json.dumps(row, separators=(",", ":")) + "\n" for row in invalid_records),
                            encoding="utf-8")
    for row in invalid_records:
        if row["status"] != "REJECTED":
            with failures_path.open("a", encoding="utf-8", newline="\n") as failures_file:
                failures_file.write(json.dumps({"kind": "invalid_input", **row}, separators=(",", ":")) + "\n")

    if len(selected_sample_signatures) < 2:
        with failures_path.open("a", encoding="utf-8", newline="\n") as failures_file:
            failures_file.write(json.dumps({
                "kind": "randomness_separation",
                "input_seed": 777,
                "fixed_scores_sha256": _score_hash([((i * 7919) % 10007) - 5003 for i in range(257)]),
                "distinct_first_round_samples": len(selected_sample_signatures),
                "error": "different algorithm seeds did not produce distinct samples",
            }, separators=(",", ":")) + "\n")

    failure_count = sum(1 for _ in failures_path.open("r", encoding="utf-8"))
    summary = {
        "algorithm_label": ALGORITHM_LABEL,
        "algorithm_revision": ALGORITHM_REVISION,
        "status": "PASS" if failure_count == 0 and audited_cases == len(cases) else "FAIL",
        "python": sys.version,
        "platform": platform.platform(),
        "oracle_source": "VFSS/include/moe_topk/topk_oracle.h",
        "oracle_compile_command": compile_command,
        "case_count": cases_checked,
        "audited_case_count": audited_cases,
        "case_counts_by_kind": counts,
        "invalid_input_checks": len(invalid_records),
        "invalid_inputs_rejected": sum(row["status"] == "REJECTED" for row in invalid_records),
        "distinct_first_round_samples_for_fixed_input": len(selected_sample_signatures),
        "cases_exercising_exact_final_layer": active_exact_final_cases,
        "cases_with_all_five_nonempty_layers": all_five_active_cases,
        "failure_count": failure_count,
        "results_jsonl": results_path.name,
        "results_sha256": hashlib.sha256(results_path.read_bytes()).hexdigest(),
        "failures_jsonl": failures_path.name,
        "failures_sha256": hashlib.sha256(failures_path.read_bytes()).hexdigest(),
        "invalid_input_checks_jsonl": invalid_path.name,
        "invalid_input_checks_sha256": hashlib.sha256(invalid_path.read_bytes()).hexdigest(),
        "representative_trace_sha256": hashlib.sha256((args.out_dir / "representative_trace.json").read_bytes()).hexdigest(),
        "representative_trace_audit_sha256": hashlib.sha256((args.out_dir / "representative_trace_audit.json").read_bytes()).hexdigest(),
    }
    summary_path.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))
    return 0 if summary["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
