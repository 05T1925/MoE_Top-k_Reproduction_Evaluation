#!/usr/bin/env python3
"""Read-only, checked resource audit for the E19 all-pairs matrix.

No key generation, benchmark execution, or extrapolated performance values.
The 3x Dealer budget is an admission rule, never an RSS measurement.
"""

import argparse
import csv
import hashlib
import json
import re
from pathlib import Path

U64 = (1 << 64) - 1
U32 = (1 << 32) - 1
MIB = 1 << 20
GIB = 1 << 30
CAPACITY = re.compile(
    r"E10_CAPACITY n=(\d+) r=(\d+) d=(\d+) slots_per_party=(\d+) "
    r"party_package_bytes=(\d+) hard_cap=(\d+) package_limit=(\d+) "
    r"budget_limit=(\d+) memory_limit=(\d+) process_limit=(\d+)"
)
REJECT = re.compile(r"E16_PREFLIGHT_REJECTED n=(\d+) r=(\d+) reason=(\S+)")


def checked(value):
    if not 0 <= value <= U64:
        raise OverflowError("capacity exceeds uint64_t")
    return value


def shape(n, r):
    d = max(2, 1 << (n - 1).bit_length())
    bits = 33 + (d - 1).bit_length()
    pairs = checked(d * (d - 1) // 2)
    slots = checked(r * pairs)
    package = checked(114 + checked(d * (1844 + 8 * r)) +
                      checked(slots * (81 + 24 * bits)))
    budget = checked(8 * package + 64 * MIB)
    key_payload = 24 * bits + 24
    t_payload = checked(2 * d * (16 + 840) + slots * key_payload +
                        d * (48 + 8 * r))
    return d, bits, pairs, slots, package, budget, t_payload


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--snapshot", required=True, type=Path)
    parser.add_argument("--preflight-log", required=True, type=Path)
    parser.add_argument("--baseline-log", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--source-revision", required=True)
    args = parser.parse_args()
    snapshot = json.loads(args.snapshot.read_text(encoding="utf-8-sig"))
    host_total = int(snapshot["windows_total_bytes"])
    host_free = int(snapshot["windows_available_bytes"])
    c_free = int(snapshot["drives"]["C:"]["free_bytes"])
    wsl_available = int(snapshot["wsl_available_bytes"])
    wsl_swap = int(snapshot["wsl_swap_free_bytes"])
    assert 0 < host_free <= host_total and 0 < c_free and 0 < wsl_available
    assert shape(256, 2)[4:6] == (69999474, 627104656)
    assert shape(128, 5)[4] == 42547506
    baseline_log = args.baseline_log.read_text(encoding="utf-8")
    for n in (1000, 10000, 100000, 1000000):
        assert re.search(rf"E19_BASELINE n={n} exit=1\nE16 baseline benchmark shape", baseline_log)

    cpp, rejected = {}, {}
    for line in args.preflight_log.read_text(encoding="utf-8").splitlines():
        if match := CAPACITY.fullmatch(line):
            n, r, *data = map(int, match.groups())
            cpp[n, r] = data
        elif match := REJECT.fullmatch(line):
            rejected[int(match[1]), int(match[2])] = match[3]

    rows = []
    for n in (1000, 10000, 100000, 1000000):
        for r in range(2, 6):
            d, bits, pairs, slots, package, budget, t_payload = shape(n, r)
            both = checked(2 * package)
            three_budget = checked(3 * budget)
            # Current T retains both pools while both serialized packages exist.
            # Each party retains its package while decoding the material pool.
            dealer_live_lower = checked(2 * checked(package + t_payload))
            party_live_lower = checked(package + t_payload)
            observed = cpp[n, r]
            assert observed[:3] == [d, slots, package]
            assert observed[3:6] == [1, 1, 1]
            assert rejected[n, r] == "HARD_CAP_D_GT_256"
            host_free_short = both > host_free
            disk_short = both > c_free
            total_budget_short = three_budget > host_total
            reason = "RESOURCE_INFEASIBLE_CURRENT_IMPL" if (
                host_free_short or disk_short or total_budget_short
            ) else "REQUIRES_CONTROLLED_TRIAL"
            rows.append(dict(
                n=n, K=80, r=r, D=d, comparison_bits=bits,
                pairs_per_round=pairs, slots_per_party=slots,
                derived_t_payload_per_party_bytes=t_payload,
                derived_party_package_bytes=package,
                derived_both_packages_bytes=both,
                derived_dealer_live_lower_bound_bytes=dealer_live_lower,
                derived_party_live_lower_bound_bytes=party_live_lower,
                admission_dealer_budget_bytes=budget,
                admission_three_times_budget_bytes=three_budget,
                package_fits_u32=int(package <= U32),
                material_id_3_fits_uint64=int(3 <= U64 - slots),
                windows_total_bytes=host_total,
                windows_available_at_snapshot_bytes=host_free,
                wsl_available_at_snapshot_bytes=wsl_available,
                wsl_swap_free_at_snapshot_bytes=wsl_swap,
                physical_c_free_at_snapshot_bytes=c_free,
                both_packages_exceed_host_available=int(host_free_short),
                three_budget_exceeds_host_physical_total=int(total_budget_short),
                three_budget_exceeds_wsl_available=int(three_budget > wsl_available),
                both_packages_exceed_physical_c_free=int(disk_short),
                actual_first_program_gate=rejected[n, r],
                first_gate_if_only_d_cap_removed="PACKAGE_GT_192_MIB",
                baseline_actual_first_gate="E16_BASELINE_BENCHMARK_SHAPE",
                e19_resource_decision=reason,
                e19_aav86_status="PRECHECK_REJECTED",
                e19_baseline_status="PRECHECK_REJECTED",
                actual_peak_rss_bytes="NOT_MEASURED",
                actual_keygen_time_ms="NOT_MEASURED",
                nine_metrics="NOT_MEASURED",
                source_revision=args.source_revision,
                snapshot_sha256=digest(args.snapshot),
                preflight_log_sha256=digest(args.preflight_log),
                baseline_log_sha256=digest(args.baseline_log),
            ))

    assert len(rows) == 16
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("x", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    print(f"E19_RESOURCE_MATRIX_PASS rows={len(rows)} "
          f"resource_infeasible={sum(x['e19_resource_decision'] == 'RESOURCE_INFEASIBLE_CURRENT_IMPL' for x in rows)} "
          f"sha256={digest(args.output)}")


if __name__ == "__main__":
    main()
