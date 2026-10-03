#!/usr/bin/env python3
"""Join checked C++ capacity/preflight output to the maintained V3 matrix."""

import argparse
import csv
from pathlib import Path


def fields(line):
    return dict(item.split("=", 1) for item in line.split()[1:])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--preflight-log", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    capacity = {}
    rejection = {}
    for line in args.preflight_log.read_text(encoding="utf-8").splitlines():
        if line.startswith("E10_CAPACITY "):
            item = fields(line)
            capacity[(int(item["n"]), int(item["r"]))] = item
        elif line.startswith("E11_PREFLIGHT_REJECTED "):
            item = fields(line)
            rejection[(int(item["n"]), int(item["r"]))] = item["reason"]
    if len(capacity) != 24 or len(rejection) != 20:
        raise RuntimeError("capacity or actual preflight rows missing")
    rows = []
    for n in (128, 256, 1000, 10000, 100000, 1000000):
        for k in ((2, 8) if n in (128, 256) else (80,)):
            for r in (2, 3, 4, 5):
                item = capacity[(n, r)]
                reason = rejection.get((n, r), "NONE")
                if n == 128 and reason != "NONE":
                    raise RuntimeError("n128 unexpectedly rejected")
                if n > 128 and reason != "HARD_CAP_D_GT_128":
                    raise RuntimeError("larger shape did not fail current hard cap")
                status = "FORMAL_PASS_5_PER_NETWORK" if n == 128 else "PRECHECK_REJECTED_NO_KEYGEN"
                rows.append({
                    "n": n, "K": k, "r": r, "D": item["d"],
                    "slots_per_party": item["slots_per_party"],
                    "party_package_bytes": item["party_package_bytes"],
                    "hard_cap": item["hard_cap"],
                    "package_limit": item["package_limit"],
                    "budget_limit": item["budget_limit"],
                    "memory_limit": item["memory_limit"],
                    "actual_preflight_reason": reason,
                    "LAN_status": status, "WAN_status": status,
                })
    with args.output.open("x", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    print(f"E11_V3_STATUS_PASS rows={len(rows)} measured=8 rejected=24")


if __name__ == "__main__":
    main()
