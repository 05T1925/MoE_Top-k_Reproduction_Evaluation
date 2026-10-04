#!/usr/bin/env python3
"""Join the frozen 32-shape resource gate with E16 final-run evidence."""

import argparse
import csv
import json
from pathlib import Path
import re


def read_runs(path):
    return [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines()]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--gate", type=Path, required=True)
    parser.add_argument("--preflight", type=Path, required=True)
    parser.add_argument("--raw-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    with args.gate.open(newline="", encoding="utf-8-sig") as stream:
        rows = list(csv.DictReader(stream))
    assert len(rows) == 32
    actual = {}
    pattern = re.compile(r"E16_PREFLIGHT_(ACCEPTED|REJECTED) n=(\d+) r=(\d+)(?: reason=(\S+))?")
    for line in args.preflight.read_text(encoding="utf-8").splitlines():
        match = pattern.fullmatch(line)
        if match:
            disposition, n, r, reason = match.groups()
            actual[(int(n), int(r))] = (disposition, reason or "NONE")
    assert len(actual) == 20
    measured = set()
    baseline = set()
    for profile in ("LAN", "WAN"):
        for tier in range(2, 6):
            path = args.raw_root / f"r{tier}" / "aav86" / f"{profile}_runs.jsonl"
            samples = read_runs(path)
            assert len(samples) == 12 and all(x["status"] == "PASS" for x in samples)
            for k in (2, 8):
                assert sum(x["K"] == k and not x["warmup"] for x in samples) == 5
                measured.add((256, k, tier))
        path = args.raw_root / "r2" / "baseline" / f"{profile}_baseline_runs.jsonl"
        samples = read_runs(path)
        assert len(samples) == 12 and all(x["status"] == "PASS" for x in samples)
        for k in (2, 8):
            assert sum(x["K"] == k and not x["warmup"] for x in samples) == 5
            baseline.add((256, k))
    output = []
    for source in rows:
        n, k, r = (int(source[name]) for name in ("n", "K", "r"))
        if n == 128:
            status = "NOT_RUN_E16_NO_BRIDGE"
            preflight = "NOT_RUN_E16"
            reason = "E15_SEPARATE_REVISION"
            baseline_status = "NOT_MEASURED"
            scope = "NO_E16_PERFORMANCE_CLAIM"
        elif n == 256:
            assert actual[(n, r)] == ("ACCEPTED", "NONE")
            assert (n, k, r) in measured and (n, k) in baseline
            status = "MEASURED"
            preflight = "ACCEPTED"
            reason = "NONE"
            baseline_status = "MEASURED_R_INDEPENDENT_COMPARATOR"
            scope = "PAIRED_NINE_METRIC_CONCLUSION"
        else:
            assert actual[(n, r)] == ("REJECTED", "HARD_CAP_D_GT_256")
            status = "PRECHECK_REJECTED"
            preflight = "REJECTED"
            reason = "HARD_CAP_D_GT_256"
            baseline_status = "NOT_MEASURED"
            scope = "CAPACITY_ONLY"
        output.append({
            "n": n, "K": k, "r": r, "padded_D": source["D"],
            "pairs_per_round": source["pairs_per_round"],
            "reserved_slots_per_party": source["slots_per_party"],
            "party_package_bytes_formula": source["party_package_bytes"],
            "dealer_budget_bytes_formula": source["dealer_budget_bytes"],
            "actual_e15_preflight": source["actual_e15_preflight"],
            "actual_e16_preflight": preflight, "first_e16_reject_gate": reason,
            "aav86_status": status, "baseline_status": baseline_status,
            "nine_metrics": "MEASURED" if status == "MEASURED" else "NOT_MEASURED",
            "claim_scope": scope,
        })
    assert len(output) == 32
    with args.output.open("x", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(output[0]))
        writer.writeheader()
        writer.writerows(output)
    counts = {status: sum(row["aav86_status"] == status for row in output)
              for status in ("MEASURED", "PRECHECK_REJECTED", "NOT_RUN_E16_NO_BRIDGE")}
    print(f"E16_STATUS_PASS rows={len(output)} counts={counts} path={args.output}")


if __name__ == "__main__":
    main()
