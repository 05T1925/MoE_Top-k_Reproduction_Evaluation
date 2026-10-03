#!/usr/bin/env python3
"""Independent accounting audit for E12 EMP clique baseline JSONL."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import statistics


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest().upper()


def qdisc(path):
    text = path.read_text(encoding="utf-8")
    match = re.search(r"Sent (\d+) bytes (\d+) pkt", text)
    if not match:
        raise RuntimeError(f"missing qdisc counters: {path}")
    return tuple(map(int, match.groups()))


def verify(directory, profile, head, aav86_rows):
    raw = directory / f"{profile}_baseline_runs.jsonl"
    summary = directory / f"{profile}_baseline_summary.json"
    rows = [json.loads(line) for line in raw.read_text(encoding="utf-8").splitlines()]
    stats = json.loads(summary.read_text(encoding="utf-8"))
    if len(rows) != 12 or stats["head"] != head or stats["raw_sha256"] != sha256(raw):
        raise RuntimeError(f"{profile} count/HEAD/raw hash")
    if len({row["run_id"] for row in rows}) != 12:
        raise RuntimeError(f"{profile} duplicate run ID")
    if len({row["binary_sha256"] for row in rows}) != 1:
        raise RuntimeError(f"{profile} binary changed")
    for row in rows:
        k, rep = row["K"], row["repetition"]
        matches = [item for item in aav86_rows if item["K"] == k and
                   item["repetition"] == rep]
        if len(matches) != 4 or \
                any(item["input_digest"] != row["input_digest"] for item in matches):
            raise RuntimeError(f"{profile} input mismatch: {row['run_id']}")
        if row["status"] != "PASS" or row["head"] != head or \
                row["tracked_state"] != "CLEAN" or row["warmup"] != (rep == 0) or \
                row["correctness"] != "frozen_oracle_and_exact_K_PASS" or \
                row["online_rounds"] != 8 or row["comparison_edges_total"] != 8128 or \
                row["dcf_eval_per_party"] != 16768:
            raise RuntimeError(f"{profile} shape/status/work: {row['run_id']}")
        if row["p0_sent_bytes"] != row["p1_received_bytes"] or \
                row["p1_sent_bytes"] != row["p0_received_bytes"] or \
                row["online_comm_total_bits"] != 8 * (row["p0_sent_bytes"] +
                                                        row["p1_sent_bytes"]):
            raise RuntimeError(f"{profile} communication: {row['run_id']}")
        for party, peer in (("p0", "p1"), ("p1", "p0")):
            stages = ("score", "forward", "cmpagg", "reveal", "reverse")
            for direction in ("sent", "received"):
                if sum(row[f"{party}_{stage}_{direction}_bytes"] for stage in stages) != \
                        row[f"{party}_{direction}_bytes"]:
                    raise RuntimeError(f"{profile} stage sum: {row['run_id']}")
            for stage in stages:
                if row[f"{party}_{stage}_received_bytes"] != \
                        row[f"{peer}_{stage}_sent_bytes"]:
                    raise RuntimeError(f"{profile} cross stage: {row['run_id']}")
        if row["offline_time_ms"] < (row["offline_generate_ms"] +
                                       row["offline_serialize_ms"] +
                                       row["offline_distribute_ms"]) or \
                row["total_time_ms"] != row["offline_time_ms"] + row["online_time_ms"]:
            raise RuntimeError(f"{profile} timing: {row['run_id']}")
        if row["raw_log_sha256"] != sha256(row["raw_log"]):
            raise RuntimeError(f"{profile} log hash: {row['run_id']}")
    for k in (2, 8):
        group = [row for row in rows if row["K"] == k]
        if sorted(row["repetition"] for row in group) != list(range(6)):
            raise RuntimeError(f"{profile} K{k} repetitions")
        formal = [row for row in group if not row["warmup"]]
        item = stats["configurations"][f"n128-k{k}"]
        if item["formal_runs"] != 5 or item["warmups_excluded"] != 1 or \
                item["matched_aav86_r"] != [2, 3, 4, 5]:
            raise RuntimeError(f"{profile} K{k} summary metadata")
        for key, figures in item.items():
            if key in ("formal_runs", "warmups_excluded", "matched_aav86_r"):
                continue
            values = [row[key] for row in formal]
            if figures != {"median": statistics.median(values), "min": min(values),
                           "max": max(values)}:
                raise RuntimeError(f"{profile} K{k} {key} summary")
        print(f"{profile} clique K={k} offline_median_ms={item['offline_time_ms']['median']:.3f} "
              f"online_median_ms={item['online_time_ms']['median']:.3f}")
    before = qdisc(directory / f"{profile}_qdisc_after_calibration.txt")
    after = qdisc(directory / f"{profile}_qdisc_after.txt")
    if not after[0] > before[0] or not after[1] > before[1]:
        raise RuntimeError(f"{profile} no shaped baseline traffic")
    print(f"{profile} clique_qdisc_delta_bytes={after[0]-before[0]} "
          f"packets={after[1]-before[1]}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--raw-dir", type=Path, required=True)
    parser.add_argument("--aav86-dir", type=Path, required=True)
    parser.add_argument("--head", required=True)
    args = parser.parse_args()
    for profile in ("LAN", "WAN"):
        ref = args.aav86_dir / f"{profile}_runs.jsonl"
        aav86_rows = [json.loads(line) for line in ref.read_text(encoding="utf-8").splitlines()]
        verify(args.raw_dir, profile, args.head, aav86_rows)
    print("E12_CLIQUE_BASELINE_AUDIT_PASS 24 runs, 20 formal, 4 warmup")


if __name__ == "__main__":
    main()
