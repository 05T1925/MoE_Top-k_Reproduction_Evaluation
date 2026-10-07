#!/usr/bin/env python3
"""Paired DCF-counter ON/OFF probe using the same frozen TEST_ONLY tape.

This is an in-process socketpair diagnostic, not a protocol benchmark. Each
attempt generates fresh material; only input scores and the explicit sampler
tape are held fixed. Raw stdout/stderr and JSON belong in an external directory.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import statistics
import subprocess
import time


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def wsl_hash(distro: str, binary: str) -> str:
    result = subprocess.run(
        ["wsl.exe", "-d", distro, "-u", "root", "--", "sha256sum", binary],
        check=True, capture_output=True, text=True, encoding="utf-8", errors="replace")
    return result.stdout.split()[0]


def fields(text: str) -> dict[str, str]:
    line = next((line for line in text.splitlines() if line.startswith("case n=")), None)
    if line is None:
        raise RuntimeError("party test did not emit a case record")
    return dict(re.findall(r"([A-Za-z0-9_]+)=([^ ]+)", line))


def stats(rows: list[float]) -> dict[str, float]:
    return {"min": min(rows), "median": statistics.median(rows), "max": max(rows)}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--counter-on", required=True, help="WSL path to counter-enabled party test binary")
    ap.add_argument("--counter-off", required=True, help="WSL path to counter-disabled party test binary")
    ap.add_argument("--test-entropy-so", required=True,
                    help="WSL path to the deterministic TEST_ONLY getrandom interposer")
    ap.add_argument("--out", required=True, type=Path, help="new empty evidence directory outside Git")
    ap.add_argument("--distro", default="Ubuntu-24.04")
    ap.add_argument("--repetitions", type=int, default=5)
    args = ap.parse_args()
    if args.repetitions < 3:
        raise SystemExit("at least three paired repetitions are required")
    if args.out.exists() and any(args.out.iterdir()):
        raise SystemExit("output directory must be new and empty")
    args.out.mkdir(parents=True, exist_ok=True)

    n, k, seed = 64, 8, 160608
    scores = [((i * 17) % 23) - 11 for i in range(n)]
    tape_pattern = "alternating"
    fixed_args = ["--counter-bench-pattern", tape_pattern, str(n), str(k), str(seed),
                  *(str(value) for value in scores)]
    binaries = {"on": args.counter_on, "off": args.counter_off}
    binary_hashes = {key: wsl_hash(args.distro, value) for key, value in binaries.items()}
    entropy_hash = wsl_hash(args.distro, args.test_entropy_so)
    runs = []

    def invoke(binary: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            ["wsl.exe", "-d", args.distro, "-u", "root", "--", "env",
             f"LD_PRELOAD={args.test_entropy_so}", "OMP_NUM_THREADS=1", binary,
             *fixed_args], capture_output=True, text=True,
            encoding="utf-8", errors="replace")

    # One warm-up per binary. Materials are generated afresh for every process.
    for name, binary in binaries.items():
        result = invoke(binary)
        raw = result.stdout + ("\nSTDERR\n" + result.stderr if result.stderr else "")
        (args.out / f"warmup-{name}.txt").write_text(raw, encoding="utf-8")
        if result.returncode != 0:
            raise RuntimeError(f"{name} warm-up failed with exit {result.returncode}")
        fields(result.stdout)

    # Alternate order to reduce monotonic host-load bias; every pair uses the
    # exact same input, algorithm seed and deterministic TEST_ONLY tape pattern.
    for rep in range(1, args.repetitions + 1):
        order = ("on", "off") if rep % 2 else ("off", "on")
        pair = {}
        for name in order:
            start = time.perf_counter()
            result = invoke(binaries[name])
            wall = time.perf_counter() - start
            raw = result.stdout + ("\nSTDERR\n" + result.stderr if result.stderr else "")
            path = args.out / f"rep{rep}-{name}.txt"
            path.write_text(raw, encoding="utf-8")
            if result.returncode != 0:
                raise RuntimeError(f"{name} repetition {rep} failed with exit {result.returncode}")
            value = fields(result.stdout)
            if value.get("status") != "SUCCESS":
                raise RuntimeError(f"{name} repetition {rep} status={value.get('status')}")
            pair[name] = value
            runs.append({"rep": rep, "build": name, "exit_code": result.returncode,
                         "status": value["status"], "online_us_p0": int(value["online_us_p0"]),
                         "online_us_p1": int(value["online_us_p1"]),
                         "online_critical_us": max(int(value["online_us_p0"]),
                                                    int(value["online_us_p1"])),
                         "process_wall_s": wall,
                         "online_prg_calls_total": value["online_prg_calls_total"],
                         "online_dcf_eval_calls_total": value["online_dcf_eval_calls_total"],
                         "online_aes_blocks_total": value["online_aes_blocks_total"],
                         "sampler_prf_words_total": int(value["sampler_prf_words_total"]),
                         "ucmp_party_evals": value["ucmp_party_evals"],
                         "dcf_party_evals": value["dcf_party_evals"],
                         "logical_rounds": value["logical_rounds"],
                         "edge_fnv64": value["edge_fnv64"], "raw_sha256": sha256(path),
                         "raw_file": path.name})
        stable = ("logical_rounds", "edge_fnv64", "ucmp_party_evals", "dcf_party_evals",
                  "sampler_prf_words_total")
        if any(pair["on"].get(key) != pair["off"].get(key) for key in stable):
            raise RuntimeError(f"counter ON/OFF workload mismatch in repetition {rep}")
        if pair["on"].get("online_prg_calls_total") == "NOT_MEASURED":
            raise RuntimeError("counter-enabled binary did not report online PRG count")
        if pair["off"].get("online_prg_calls_total") != "NOT_MEASURED":
            raise RuntimeError("counter-disabled binary reported a measured PRG count")

    on_rows = [row for row in runs if row["build"] == "on"]
    off_rows = [row for row in runs if row["build"] == "off"]
    workload_fields = ("logical_rounds", "edge_fnv64", "ucmp_party_evals",
                       "dcf_party_evals", "sampler_prf_words_total")
    for name, rows in (("on", on_rows), ("off", off_rows)):
        for key in workload_fields:
            if len({row[key] for row in rows}) != 1:
                raise RuntimeError(f"{name} fixed-entropy runs changed workload field {key}")
    for key in ("online_prg_calls_total", "online_aes_blocks_total"):
        if len({row[key] for row in on_rows}) != 1:
            raise RuntimeError(f"counter-on fixed-entropy runs changed measured work {key}")
    if any(on[key] != off[key] for on, off in zip(on_rows, off_rows) for key in workload_fields):
        raise RuntimeError("counter ON/OFF builds did not retain identical measured workloads")
    on_stats = stats([row["online_critical_us"] for row in on_rows])
    off_stats = stats([row["online_critical_us"] for row in off_rows])
    report = {
        "label": "BMW16_S16_FIXED_TAPE_COUNTER_OVERHEAD_TEST_ONLY_NOT_PERFORMANCE",
        "scope": "one process hosting two party API threads over socketpairs; fixed TEST_ONLY sampling tape; no sampling-coin exchange; includes protocol API stages, excludes offline generation from online_us",
        "input": {"n": n, "K": k, "input_seed": seed, "scores": scores,
                  "algorithm_tape_pattern": tape_pattern,
                  "tape_rule": "1024 words per task/domain, alternating 0 and UINT64_MAX; fixed by source and seed-independent"},
        "policy": {"warmups": 1, "measured_pairs": args.repetitions,
                   "fresh_material_every_attempt": True,
                   "same_input_and_tape_across_builds": True,
                   "counter_definitions": {"online_prg_calls_total": "sum of ordinary DCF ecbEncTwoBlocks node expansions across both API threads; one invocation per DCF level, two AES blocks per invocation",
                                           "sampler_prf_words_total": "separate; zero here because fixed tape supplies decisions",
                                           "online_aes_blocks_total": "2 times ordinary DCF Eval node expansions; not PRG calls"}},
        "binary_sha256": binary_hashes,
        "test_entropy_interposer_sha256": entropy_hash,
        "counter_on_critical_online_us": on_stats,
        "counter_off_critical_online_us": off_stats,
        "median_counter_overhead_percent": (on_stats["median"] / off_stats["median"] - 1.0) * 100.0,
        "runs": runs,
    }
    report_path = args.out / "counter-overhead.json"
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"result": "PASS", "report": str(report_path),
                      "report_sha256": sha256(report_path),
                      "counter_on_median_us": on_stats["median"],
                      "counter_off_median_us": off_stats["median"],
                      "median_overhead_percent": report["median_counter_overhead_percent"]},
                     sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
