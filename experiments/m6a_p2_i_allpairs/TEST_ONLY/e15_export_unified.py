#!/usr/bin/env python3
"""Export the E15 Protocol I full-entry columns for later six-route comparison."""

import argparse
import csv
import json
from pathlib import Path
import statistics


FIELDS = (
    "offline_time_ms", "offline_material_total_bits", "online_time_ms",
    "total_time_ms", "online_comm_total_bits", "online_comm_per_party_bits",
    "online_rounds", "online_prg_calls_total", "comparison_edges_total",
)


def read(path):
    return [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines()]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--raw-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    rows = []
    for profile in ("LAN", "WAN"):
        for route, filename in (("aav86", f"{profile}_runs.jsonl"),
                                ("baseline", f"{profile}_baseline_runs.jsonl")):
            formal = [row for row in read(args.raw_root / route / filename)
                      if row["status"] == "PASS" and not row["warmup"]]
            for k in (2, 8):
                for r in ((2, 3, 4, 5) if route == "aav86" else (None,)):
                    group = [row for row in formal if row["K"] == k and
                             (r is None or row["r"] == r)]
                    if len(group) != 5:
                        raise RuntimeError("E15 five-run group missing")
                    item = {"profile": profile, "route": route, "n": 128,
                            "K": k, "r": "NA" if r is None else r,
                            "implementation": group[0]["implementation"],
                            "head": group[0]["head"], "formal_runs": 5,
                            "warmup_runs": 1,
                            "network": "same_host_WSL2_TCP_simulated"}
                    for field in FIELDS:
                        values = [row[field] for row in group]
                        if not all(isinstance(value, (int, float)) for value in values):
                            raise RuntimeError(f"E15 unified metric missing: {field}")
                        item[field + "_median"] = statistics.median(values)
                        item[field + "_min"] = min(values)
                        item[field + "_max"] = max(values)
                    rows.append(item)
    with args.output.open("x", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    print(f"E15_UNIFIED_EXPORT_PASS groups={len(rows)} fields={len(FIELDS)} "
          f"path={args.output}")


if __name__ == "__main__":
    main()
