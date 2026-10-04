#!/usr/bin/env python3
"""Join checked sealed-file capacity with E20 accepted n=1000 run states."""

import argparse
import csv
import json
from pathlib import Path


def load_runs(root, name):
    return [json.loads(line) for line in (root / name / "runs.jsonl").read_text().splitlines()]


def maximum(rows, key):
    return max(int(row[key]) for row in rows)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("evidence_root", type=Path)
    parser.add_argument("capacity_csv", type=Path)
    parser.add_argument("output_csv", type=Path)
    args = parser.parse_args()
    with args.capacity_csv.open(newline="") as stream:
        capacities = list(csv.DictReader(stream))
    if len(capacities) != 16:
        raise RuntimeError("expected 16 checked configurations")
    root = args.evidence_root
    disk_free = int(capacities[0]["physical_disk_free_bytes"])
    baseline = {profile: load_runs(root, f"CLIQUE_{profile}_768mb")
                for profile in ("LAN", "WAN")}
    for profile, records in baseline.items():
        if len(records) != 6 or any(x["status"] != "PASS" for x in records):
            raise RuntimeError(f"incomplete paired baseline {profile}")
    rows = []
    for capacity in capacities:
        n, r = int(capacity["n"]), int(capacity["r"])
        common = {key: capacity[key] for key in (
            "n", "K", "r", "D", "all_candidate_edges",
            "sealed_file_per_party_bytes", "sealed_files_both_bytes",
            "physical_disk_free_bytes", "program_first_gate", "next_secure_store_gate")}
        if n > 1000:
            if capacity["resource_status"] != "RESOURCE_INFEASIBLE" or \
                    int(capacity["sealed_files_both_bytes"]) <= disk_free:
                raise RuntimeError(f"large resource gate mismatch n={n} r={r}")
            rows.append(dict(common, profile="NOT_RUN", implementation="E20_STREAM_AEAD_V1",
                             status="RESOURCE_INFEASIBLE/PRECHECK_REJECTED",
                             runs="NOT_MEASURED", formal_runs="NOT_MEASURED",
                             nine_metrics="NOT_MEASURED", peak_t_kib="NOT_MEASURED",
                             peak_p0_kib="NOT_MEASURED", peak_p1_kib="NOT_MEASURED",
                             binary_sha256="NOT_MEASURED", raw_batch="NOT_MEASURED"))
            continue
        for profile in ("LAN", "WAN"):
            batch = f"AAV_{profile}_768mb"
            group = [x for x in load_runs(root, batch) if x["r"] == r]
            if len(group) != 6 or {x["repetition"] for x in group} != set(range(6)) \
                    or any(x["status"] != "PASS" for x in group):
                raise RuntimeError(f"incomplete AAV batch {profile} r={r}")
            if int(capacity["sealed_file_per_party_bytes"]) != \
                    group[0]["disk_ciphertext_per_party_bytes"]:
                raise RuntimeError(f"disk layout mismatch {profile} r={r}")
            rows.append(dict(common, profile=profile, implementation="E20_STREAM_AEAD_V1",
                             status="MEASURED_PAIRED", runs=6, formal_runs=5,
                             nine_metrics="8_ACTUAL_RUN_1_VERIFIED_FIXED_LAYOUT",
                             peak_t_kib=maximum(group,"peak_t_kib"),
                             peak_p0_kib=maximum(group,"peak_p0_kib"),
                             peak_p1_kib=maximum(group,"peak_p1_kib"),
                             binary_sha256=group[0]["binary_sha256"], raw_batch=batch))
    for profile, group in baseline.items():
        file_bytes = int(group[0]["disk_ciphertext_p0_bytes"])
        if any(int(x["disk_ciphertext_p0_bytes"]) != file_bytes or
               int(x["disk_ciphertext_p1_bytes"]) != file_bytes for x in group):
            raise RuntimeError(f"baseline sealed file mismatch {profile}")
        rows.append(dict(n=1000, K=80, r="CLIQUE", D=1024,
                         all_candidate_edges=523776,
                         sealed_file_per_party_bytes=file_bytes,
                         sealed_files_both_bytes=2*file_bytes,
                         physical_disk_free_bytes=disk_free,
                         program_first_gate="NONE", next_secure_store_gate="NONE",
                         profile=profile,
                         implementation="E20_CLIQUE_SEALED_V1",
                         status="MEASURED_PAIRED", runs=6, formal_runs=5,
                         nine_metrics="8_ACTUAL_RUN_1_VERIFIED_FIXED_LAYOUT",
                         peak_t_kib=maximum(group,"peak_t_kib"),
                         peak_p0_kib=maximum(group,"peak_p0_kib"),
                         peak_p1_kib=maximum(group,"peak_p1_kib"),
                         binary_sha256=group[0]["binary_sha256"],
                         raw_batch=f"CLIQUE_{profile}_768mb"))
    if len(rows) != 22:
        raise RuntimeError("expected 8 AAV + 12 large + 2 baseline statuses")
    with args.output_csv.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)
    print("E20_STATUS_AUDIT_PASS rows=22 paired=10 large_resource_rejected=12")


if __name__ == "__main__":
    main()
