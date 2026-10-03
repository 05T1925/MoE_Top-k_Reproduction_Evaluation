#!/usr/bin/env python3
"""Independently check E12 raw rows, timing contract, summaries and qdisc."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import statistics


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def qdisc_bytes(path):
    match = re.search(r"Sent (\d+) bytes (\d+) pkt", path.read_text(encoding="utf-8"))
    if not match:
        raise RuntimeError(f"missing qdisc byte/packet count: {path}")
    return tuple(map(int, match.groups()))


def verify(root, profile, head):
    raw_path = root / f"{profile}_runs.jsonl"
    summary_path = root / f"{profile}_summary.json"
    calibration_path = root / f"{profile}_calibration.json"
    rows = [json.loads(line) for line in raw_path.read_text(encoding="utf-8").splitlines()]
    summary = json.loads(summary_path.read_text(encoding="utf-8"))
    calibration = json.loads(calibration_path.read_text(encoding="utf-8"))
    if len(rows) != 48 or summary["head"] != head or \
            summary["raw_sha256"] != sha256(raw_path):
        raise RuntimeError(f"{profile} row count, HEAD or raw hash")
    if len({row["run_id"] for row in rows}) != 48:
        raise RuntimeError(f"{profile} duplicate run ID")
    source_ids = {json.dumps(row["source_sha256"], sort_keys=True) for row in rows}
    binary_ids = {row["binary_sha256"] for row in rows}
    if len(source_ids) != 1 or len(binary_ids) != 1:
        raise RuntimeError(f"{profile} source/binary changed")
    for row in rows:
        k, r, rep = row["K"], row["r"], row["repetition"]
        if row["status"] != "PASS" or row["head"] != head or \
                row["tracked_state"] != "CLEAN" or row["profile"] != profile or \
                row["correctness"] != "frozen_oracle_and_exact_K_PASS" or \
                row["input_seed"] != 0xE110000 + k * 100 + rep or \
                row["public_algorithm_seed"] != 0xA110000 + r * 1000 + rep or \
                row["warmup"] != (rep == 0):
            raise RuntimeError(f"{profile} run metadata: {row['run_id']}")
        if row["D"] != 128 or row["n"] != 128 or row["online_rounds"] != 2 * r + 4 or \
                row["core_rounds"] != 2 * r + 1 or \
                row["exit_t"] or row["exit_p0"] or row["exit_p1"]:
            raise RuntimeError(f"{profile} shape/round/exit: {row['run_id']}")
        if len(row["active_edges_by_round"]) != r or \
                len(row["active_vertices_by_round"]) != r or \
                len(row["ca_prg_by_round_per_party"]) != r or \
                sum(row["active_edges_by_round"]) != row["comparison_edges_total"] or \
                sum(row["active_vertices_by_round"]) != row["active_vertices_total"] or \
                sum(row["ca_prg_by_round_per_party"]) != row["ca_prg_per_party"]:
            raise RuntimeError(f"{profile} graph/PRG rounds: {row['run_id']}")
        if row["comparison_edges_total"] > row["reserved_slots_per_party"] or \
                row["dcf_eval_per_party"] != row["score_dcf_eval_per_party"] + \
                row["ca_dcf_eval_per_party"] or \
                row["online_prg_calls_total"] != 2 * (row["score_prg_per_party"] +
                                                      row["ca_prg_per_party"] +
                                                      row["inverse_prg_per_party"]):
            raise RuntimeError(f"{profile} work accounting: {row['run_id']}")
        if row["p0_sent_bytes"] != row["p1_received_bytes"] or \
                row["p1_sent_bytes"] != row["p0_received_bytes"] or \
                row["online_comm_total_bits"] != 8 * (row["p0_sent_bytes"] +
                                                        row["p1_sent_bytes"]):
            raise RuntimeError(f"{profile} communication accounting: {row['run_id']}")
        if row["measurement_contract"] != (
                "E12: transport setup excluded; offline before all role forks to both ready; online max party secure call"):
            raise RuntimeError(f"{profile} measurement contract: {row['run_id']}")
        if row["offline_time_ms"] < row["offline_generate_ms"] + \
                row["offline_serialize_ms"] + row["offline_distribute_ms"] or \
                row["total_time_ms"] != row["offline_time_ms"] + row["online_time_ms"]:
            raise RuntimeError(f"{profile} offline/total timing: {row['run_id']}")
        for party in ("p0", "p1"):
            if sum(row[f"{party}_{stage}_sent_bytes"]
                   for stage in ("score", "core", "inverse")) != row[f"{party}_sent_bytes"] or \
                    sum(row[f"{party}_{stage}_received_bytes"]
                        for stage in ("score", "core", "inverse")) != row[f"{party}_received_bytes"]:
                raise RuntimeError(f"{profile} stage accounting: {row['run_id']}")
            if abs(row[f"{party}_ca_ms"] + row[f"{party}_carrier_ms"] -
                   row[f"combination_{party}_ms"]) > 1e-9:
                raise RuntimeError(f"{profile} CA/carrier timing: {row['run_id']}")
            peer = "p1" if party == "p0" else "p0"
            for stage in ("score", "core", "inverse"):
                if row[f"{party}_{stage}_received_bytes"] != row[f"{peer}_{stage}_sent_bytes"]:
                    raise RuntimeError(f"{profile} actual stage receive: {row['run_id']}")
        if row["raw_log_sha256"] != sha256(Path(row["raw_log"])):
            raise RuntimeError(f"{profile} log changed: {row['run_id']}")
    for k in (2, 8):
        for r in (2, 3, 4, 5):
            group = [row for row in rows if row["K"] == k and row["r"] == r]
            if len(group) != 6 or sorted(row["repetition"] for row in group) != list(range(6)):
                raise RuntimeError(f"{profile} repetition coverage k{k} r{r}")
            formal = [row for row in group if not row["warmup"]]
            item = summary["configurations"][f"n128-k{k}-r{r}"]
            if item["formal_runs"] != 5 or item["warmups_excluded"] != 1:
                raise RuntimeError(f"{profile} summary counts k{k} r{r}")
            for field, measures in item.items():
                if field in ("formal_runs", "warmups_excluded"):
                    continue
                values = [row[field] for row in formal]
                if measures != {"median": statistics.median(values),
                                "min": min(values), "max": max(values)}:
                    raise RuntimeError(f"{profile} summary {field} k{k} r{r}")
            online = item["online_time_ms"]
            offline = item["offline_time_ms"]
            print(f"{profile} K={k} r={r} online={online['median']:.3f} "
                  f"[{online['min']:.3f},{online['max']:.3f}] ms "
                  f"offline={offline['median']:.3f} "
                  f"[{offline['min']:.3f},{offline['max']:.3f}] ms "
                  f"e_A={item['comparison_edges_total']['median']}")
    before = qdisc_bytes(root / f"{profile}_qdisc_after_calibration.txt")
    after = qdisc_bytes(root / f"{profile}_qdisc_after.txt")
    if after[0] <= before[0] or after[1] <= before[1]:
        raise RuntimeError(f"{profile} no protocol traffic through qdisc")
    print(f"{profile} qdisc_protocol_delta_bytes={after[0]-before[0]} "
          f"packets={after[1]-before[1]} "
          f"calibration_RTT_ms={calibration['rtt_median_ms']:.3f} "
          f"throughput_Mbps={calibration['throughput_mbps']:.3f}")
    return rows


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--raw-dir", type=Path, required=True)
    parser.add_argument("--head", required=True)
    args = parser.parse_args()
    lan = verify(args.raw_dir, "LAN", args.head)
    wan = verify(args.raw_dir, "WAN", args.head)
    for profile_rows in (lan, wan):
        for k in (2, 8):
            for repetition in range(6):
                digests = {row["input_digest"] for row in profile_rows
                           if row["K"] == k and row["repetition"] == repetition}
                if len(digests) != 1:
                    raise RuntimeError("input differs across r for matched group")
        for row in profile_rows:
            counterpart = next(other for other in wan if other["K"] == row["K"] and
                               other["r"] == row["r"] and
                               other["repetition"] == row["repetition"])
            if row["input_digest"] != counterpart["input_digest"] or \
                    row["public_algorithm_seed"] != counterpart["public_algorithm_seed"]:
                raise RuntimeError("LAN/WAN seed or input mismatch")
    print("E12_RAW_AUDIT_PASS 96 runs, 80 formal, 16 warmup, 16 summaries")


if __name__ == "__main__":
    main()
