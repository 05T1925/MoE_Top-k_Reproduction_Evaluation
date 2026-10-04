#!/usr/bin/env python3
"""Recompute the maintained 32 shapes and join actual E15 preflight output.

This is arithmetic and evidence joining only. It never allocates key pools.
"""

import argparse
import csv
import re
from pathlib import Path


MIB = 1024 * 1024
OLD_LOG = re.compile(
    r"E10_CAPACITY n=(\d+) r=(\d+) d=(\d+) slots_per_party=(\d+) "
    r"party_package_bytes=(\d+) hard_cap=(\d+) package_limit=(\d+) "
    r"budget_limit=(\d+) memory_limit=(\d+) process_limit=(\d+)")
REJECT = re.compile(r"E11_PREFLIGHT_REJECTED n=(\d+) r=(\d+) reason=(\S+)")


def shape(n, r):
    d = max(2, 1 << (n - 1).bit_length())
    bits = 33 + (d - 1).bit_length()
    pairs = d * (d - 1) // 2
    slots = r * pairs
    package = 114 + d * (1844 + 8 * r) + slots * (81 + 24 * bits)
    budget = 8 * package + 64 * MIB
    return d, bits, pairs, slots, package, budget


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--current-log", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--available-memory-bytes", type=int, required=True)
    p.add_argument("--disk-free-bytes", type=int, required=True)
    a = p.parse_args()
    current, rejected = {}, {}
    for line in a.current_log.read_text(encoding="utf-8").splitlines():
        if match := OLD_LOG.fullmatch(line):
            n, r, d, slots, package, hard, packet, budget, memory, process = map(
                int, match.groups())
            current[n, r] = (d, slots, package, hard, packet, budget, memory, process)
        elif match := REJECT.fullmatch(line):
            n, r = map(int, match.groups()[:2])
            rejected[n, r] = match.group(3)
    assert len(current) == 24 and len(rejected) == 20
    rows = []
    for n in (128, 256, 1000, 10000, 100000, 1000000):
        for k in ((2, 8) if n in (128, 256) else (80,)):
            for r in (2, 3, 4, 5):
                d, bits, pairs, slots, packet, budget = shape(n, r)
                observed = current[n, r]
                assert observed[:3] == (d, slots, packet), (n, r, observed)
                old_reason = rejected.get((n, r), "NONE")
                assert old_reason == ("NONE" if n == 128 else "HARD_CAP_D_GT_128")
                # Proposed E16 bound is deliberately finite at every layer.
                proposed = (
                    "HARD_CAP_D_GT_256" if d > 256 else
                    "PACKAGE_GT_192_MIB" if packet > 192 * MIB else
                    "DEALER_BUDGET_GT_2048_MIB" if budget > 2048 * MIB else
                    "AVAILABLE_MEMORY_LT_3X_BUDGET" if 3 * budget > a.available_memory_bytes else
                    "DISK_FREE_LT_8_GIB" if a.disk_free_bytes < 8 * 1024 * MIB else
                    "ELIGIBLE_FOR_STAGED_TRIAL")
                rows.append(dict(n=n, K=k, r=r, D=d, comparison_bits=bits,
                                 pairs_per_round=pairs, slots_per_party=slots,
                                 party_package_bytes=packet,
                                 dealer_budget_bytes=budget,
                                 three_times_budget_bytes=3 * budget,
                                 old_hard_cap=observed[3], old_package_limit=observed[4],
                                 old_budget_limit=observed[5], old_memory_limit=observed[6],
                                 old_process_limit=observed[7],
                                 actual_e15_preflight=old_reason,
                                 proposed_e16_gate=proposed,
                                 actual_e16_preflight="NOT_RUN",
                                 e16_aav86_status="NOT_MEASURED",
                                 e16_baseline_status="NOT_MEASURED"))
    assert len(rows) == 32
    a.output.parent.mkdir(parents=True, exist_ok=True)
    with a.output.open("x", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    print(f"E16_RESOURCE_GATE_PASS rows=32 n256_eligible="
          f"{sum(x['n'] == 256 and x['proposed_e16_gate'] == 'ELIGIBLE_FOR_STAGED_TRIAL' for x in rows)} "
          f"n_ge_1000_rejected={sum(x['n'] >= 1000 and x['proposed_e16_gate'] != 'ELIGIBLE_FOR_STAGED_TRIAL' for x in rows)}")


if __name__ == "__main__":
    main()
