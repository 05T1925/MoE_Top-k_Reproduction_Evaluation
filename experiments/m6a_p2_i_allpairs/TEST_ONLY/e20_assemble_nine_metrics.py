#!/usr/bin/env python3
"""Audit E20 successful batches and emit one nine-metric, all-route CSV."""

import argparse
import csv
import json
from pathlib import Path
import statistics


NINE = (
    "offline_time_ms", "offline_material_total_bits", "online_time_ms",
    "total_time_ms", "online_comm_total_bits", "online_comm_per_party_bits",
    "online_rounds", "online_dcf_length_doubling_prg_calls_total",
    "comparison_edges_total",
)


def same(a, b):
    return abs(a - b) < 1e-6


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("evidence_root", type=Path)
    parser.add_argument("output_csv", type=Path)
    parser.add_argument("--batch", action="append", help="Selected completed batch directory; repeatable")
    args = parser.parse_args()
    batches = ([args.evidence_root / name / "summary.json" for name in args.batch]
               if args.batch else sorted(args.evidence_root.glob("*/summary.json")))
    if not batches:
        raise RuntimeError("no completed E20 batches")
    if any(not path.is_file() for path in batches):
        raise RuntimeError("selected batch missing summary.json")
    rows = []
    input_digests = {}
    for summary_path in batches:
        folder = summary_path.parent
        summary = json.loads(summary_path.read_text())
        common = summary["common"]
        records = [json.loads(line) for line in (folder / "runs.jsonl").read_text().splitlines()]
        groups = {}
        for record in records:
            if record["status"] != "PASS" or record["exit_code"] != 0:
                raise RuntimeError(f"unsuccessful record in {folder}: {record['run_id']}")
            if record["correctness"] != "full_entry_oracle_and_exact_K_PASS":
                raise RuntimeError(f"oracle missing: {record['run_id']}")
            if record["p0_sent_bytes"] != record["p1_received_bytes"] or \
                    record["p1_sent_bytes"] != record["p0_received_bytes"]:
                raise RuntimeError(f"communication mismatch: {record['run_id']}")
            if not same(record["total_time_ms"],
                        record["offline_time_ms"] + record["online_time_ms"]):
                raise RuntimeError(f"time mismatch: {record['run_id']}")
            if record["binary_sha256"] != common["binary_sha256"]:
                raise RuntimeError(f"binary mismatch: {record['run_id']}")
            r = record.get("r", "CLIQUE")
            groups.setdefault(r, []).append(record)
            digest_key = (record["n"], record["profile"], record["repetition"])
            prior = input_digests.setdefault(digest_key, record["input_digest"])
            if prior != record["input_digest"]:
                raise RuntimeError(f"paired input mismatch: {record['run_id']}")
        for r, group in groups.items():
            if len(group) != 6 or {x["repetition"] for x in group} != set(range(6)):
                raise RuntimeError(f"incomplete 1+5 batch: {folder} r={r}")
            if not next(x for x in group if x["repetition"] == 0)["warmup"] or \
                    any(x["warmup"] for x in group if x["repetition"]):
                raise RuntimeError(f"warmup error: {folder} r={r}")
            formal = [x for x in group if x["repetition"]]
            reported = summary["nine"][str(r)] if r != "CLIQUE" else summary["nine"]
            for metric in NINE:
                values = sorted(x[metric] for x in formal)
                observed = {"min": values[0], "median": statistics.median(values),
                            "max": values[-1]}
                if any(not same(observed[key], reported[metric][key]) for key in observed):
                    raise RuntimeError(f"summary mismatch: {folder} r={r} {metric}")
                rows.append({
                    "n": common["n"], "K": common["K"], "r": r,
                    "profile": group[0]["profile"],
                    "implementation": common["implementation"],
                    "metric": metric, **observed,
                    "evidence_grade": ("VERIFIED_FIXED_LAYOUT_READY_PAYLOAD"
                                       if metric == "offline_material_total_bits"
                                       else "ACTUAL_RUN"),
                    "formal_repetitions": len(formal),
                    "source_revision": common["source_revision"],
                    "binary_sha256": common["binary_sha256"],
                    "raw_batch": folder.name,
                })
    rows.sort(key=lambda x: (int(x["n"]), x["profile"], x["implementation"],
                             str(x["r"]), NINE.index(x["metric"])))
    args.output_csv.parent.mkdir(parents=True, exist_ok=True)
    with args.output_csv.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)
    print(f"E20_NINE_AUDIT_PASS batches={len(batches)} groups={len(rows)//9} "
          f"formal={len(rows)//9*5} metric_rows={len(rows)} paired_inputs=PASS")


if __name__ == "__main__":
    main()
