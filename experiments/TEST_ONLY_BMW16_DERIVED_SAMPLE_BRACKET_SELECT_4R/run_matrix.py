#!/usr/bin/env python3
"""Freeze and execute the S4 TEST_ONLY fixed-input and random-input matrices."""

from __future__ import annotations

import argparse
import concurrent.futures
import hashlib
import json
import os
import platform
import random
import subprocess
import sys
from pathlib import Path
from typing import Any

from select4r import INT32_MAX, INT32_MIN, LABEL, parameters, run_select


ROOT_SEED = "BMW16-S4-2026-10-05-v1"
CONFIGS = ((128, 2), (128, 8), (256, 2), (256, 8), (1000, 80))
REPEATS = 1000


def seed64(domain: str) -> int:
    return int.from_bytes(hashlib.sha256(f"{ROOT_SEED}|{domain}".encode("ascii")).digest()[:8], "big")


def make_scores(n: int, input_seed: int) -> list[int]:
    rng = random.Random(input_seed)
    return [rng.randrange(INT32_MIN, INT32_MAX + 1) for _ in range(n)]


def canonical_hash(obj: Any) -> str:
    return hashlib.sha256(json.dumps(obj, sort_keys=True, separators=(",", ":")).encode("utf-8")).hexdigest()


def effective_median_size(n: int, k: int) -> int:
    return 2 * max(n - k + 1, k)


def sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def build_schedule() -> list[dict[str, Any]]:
    schedule = []
    for n, k in CONFIGS:
        config = f"n{n}_k{k}"
        fixed_input_seed = seed64(f"fixed-input|{config}")
        for repetition in range(REPEATS):
            schedule.append({
                "family": "fixed_input_coin_only",
                "config_id": config,
                "n": n,
                "K": k,
                "repetition": repetition,
                "input_seed": fixed_input_seed,
                "algorithm_seed": seed64(f"fixed-algorithm|{config}|{repetition}"),
            })
        for repetition in range(REPEATS):
            schedule.append({
                "family": "random_int32_input_and_independent_coins",
                "config_id": config,
                "n": n,
                "K": k,
                "repetition": repetition,
                "input_seed": seed64(f"random-input|{config}|{repetition}"),
                "algorithm_seed": seed64(f"random-algorithm|{config}|{repetition}"),
            })
    return schedule


def _run_one(job: dict[str, Any]) -> dict[str, Any]:
    scores = make_scores(job["n"], job["input_seed"])
    result = run_select(scores, job["K"], job["input_seed"], job["algorithm_seed"], trace=False)
    result.update({
        "family": job["family"],
        "config_id": job["config_id"],
        "repetition": job["repetition"],
    })
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=f"Run frozen {LABEL} validation matrix")
    parser.add_argument("--out-dir", type=Path, required=True, help="external evidence directory, outside the repo")
    parser.add_argument("--workers", type=int, default=min(8, os.cpu_count() or 1))
    parser.add_argument("--repeats", type=int, default=REPEATS)
    parser.add_argument("--limit-configs", type=int, default=0, help="debug only; 0 means all five configurations")
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    args = parser.parse_args()
    if args.repeats != REPEATS:
        raise SystemExit(f"S4 frozen matrix requires exactly {REPEATS} repetitions per family/config")
    if not 1 <= args.workers <= 64:
        raise SystemExit("workers must be in [1,64]")
    args.out_dir.mkdir(parents=True, exist_ok=True)

    schedule = build_schedule()
    if args.limit_configs:
        allowed = {f"n{n}_k{k}" for n, k in CONFIGS[:args.limit_configs]}
        schedule = [row for row in schedule if row["config_id"] in allowed]
    schedule_payload = {"label": LABEL, "root_seed": ROOT_SEED, "rows": schedule}
    schedule_path = args.out_dir / "seed_schedule.json"
    schedule_path.write_text(json.dumps(schedule_payload, sort_keys=True, separators=(",", ":")), encoding="utf-8")
    schedule_hash = sha256_file(schedule_path)

    source_files = [
        Path(__file__).with_name(name)
        for name in ("select4r.py", "run_matrix.py", "validate_results.py", "audit_edges.py", "test_select4r.py")
        if Path(__file__).with_name(name).exists()
    ]
    repo = args.root
    revision = subprocess.run(["git", "-C", str(repo), "rev-parse", "HEAD"], check=True,
                              capture_output=True, text=True).stdout.strip()
    frozen = {
        "created_before_experiment_execution": True,
        "label": LABEL,
        "root_seed": ROOT_SEED,
        "schedule_sha256": schedule_hash,
        "schedule_rows": len(schedule),
        "repeats_per_family_config": REPEATS,
        "official_configs": [list(x) for x in CONFIGS],
        "fixed_input_generator": "Python random.Random(input_seed), n independent uniform signed int32 raw scores; one input reused for 1000 algorithm seeds per config",
        "random_input_generator": "Python random.Random(input_seed), n independent uniform signed int32 raw scores; one independent input/algorithm seed pair per repetition",
        "algorithm_tape_rule": "SHA-256 domain-separated 64-bit seeds; run tapes are deterministic replay streams, not cryptographic coins",
        "confidence_method": "one-sided exact 95% Clopper-Pearson lower bound",
        "provisional_engineering_gate": "lower bound >= 0.99; candidate only, not an approved project policy",
        "revision": revision,
        "python": sys.version,
        "platform": platform.platform(),
        "workers": args.workers,
        "source_sha256": {p.name: sha256_file(p) for p in source_files},
        "effective_median_sizes": {
            f"n{n}_k{k}": effective_median_size(n, k) for n, k in CONFIGS
        },
        "parameter_examples": {
            f"n{n}_k{k}": parameters(effective_median_size(n, k)) for n, k in CONFIGS
        },
    }
    frozen_path = args.out_dir / "frozen_manifest.json"
    frozen_path.write_text(json.dumps(frozen, sort_keys=True, indent=2), encoding="utf-8")
    frozen_hash = sha256_file(frozen_path)

    records: list[dict[str, Any]] = []
    failures: list[dict[str, Any]] = []
    with concurrent.futures.ProcessPoolExecutor(max_workers=args.workers) as pool:
        future_to_job = {pool.submit(_run_one, job): job for job in schedule}
        for future in concurrent.futures.as_completed(future_to_job):
            job = future_to_job[future]
            try:
                row = future.result()
            except Exception as exc:  # retain worker error in the denominator
                row = {
                    "label": LABEL,
                    "status": "RESOURCE_FAILURE",
                    "n": job["n"], "K": job["K"],
                    "family": job["family"], "config_id": job["config_id"],
                    "repetition": job["repetition"],
                    "input_seed": job["input_seed"], "algorithm_seed": job["algorithm_seed"],
                    "failure_reason": f"{type(exc).__name__}: {exc}",
                }
            row["schedule_sha256"] = schedule_hash
            row["frozen_manifest_sha256"] = frozen_hash
            records.append(row)
            if row["status"] != "SUCCESS":
                failures.append(row)
    records.sort(key=lambda x: (x["family"], x["config_id"], x["repetition"]))
    result_path = args.out_dir / "algorithm_results.jsonl"
    with result_path.open("w", encoding="utf-8", newline="\n") as handle:
        for row in records:
            handle.write(json.dumps(row, sort_keys=True, separators=(",", ":")) + "\n")
    failure_path = args.out_dir / "algorithm_failures.jsonl"
    with failure_path.open("w", encoding="utf-8", newline="\n") as handle:
        for row in failures:
            handle.write(json.dumps(row, sort_keys=True, separators=(",", ":")) + "\n")
    summary = {
        "label": LABEL,
        "revision": revision,
        "schedule_sha256": schedule_hash,
        "frozen_manifest_sha256": frozen_hash,
        "schedule_rows": len(schedule),
        "status_counts": {},
        "algorithm_results_sha256": sha256_file(result_path),
        "algorithm_failures_sha256": sha256_file(failure_path),
        "worker_count": args.workers,
    }
    for row in records:
        key = f"{row['family']}|{row['config_id']}|{row['status']}"
        summary["status_counts"][key] = summary["status_counts"].get(key, 0) + 1
    summary_path = args.out_dir / "algorithm_run_summary.json"
    summary_path.write_text(json.dumps(summary, sort_keys=True, indent=2), encoding="utf-8")
    print(json.dumps(summary, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
