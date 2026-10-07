#!/usr/bin/env python3
"""Checked size/resource preflight for the S18 EXPERIMENTAL streamed key pool.

This script never creates keys or opens a party bundle. Formula outputs are
capacity calculations; host free space/RAM are observations at invocation.
"""
from __future__ import annotations

import argparse
import json
import os
import shutil
import sys

U64_MAX = (1 << 64) - 1
I64_MAX = (1 << 63) - 1
DEFAULT_DISK_RESERVE = 1 << 30
DEFAULT_MEMORY_RESERVE = 2 << 30
STREAM_HEADER_BYTES = 164
GCM_TAG_BYTES = 16


def checked(value: int, label: str, limit: int = U64_MAX) -> int:
    if value < 0 or value > limit:
        raise OverflowError(f"{label} exceeds checked {limit.bit_length()}-bit range")
    return value


def linux_available_memory() -> int | None:
    try:
        for line in open("/proc/meminfo", encoding="ascii"):
            if line.startswith("MemAvailable:"):
                return int(line.split()[1]) * 1024
    except OSError:
        return None
    return None


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--n", type=int, required=True)
    parser.add_argument("--k", type=int, required=True)
    parser.add_argument("--directory", default="/tmp")
    parser.add_argument("--disk-reserve-bytes", type=int, default=DEFAULT_DISK_RESERVE)
    parser.add_argument("--memory-reserve-bytes", type=int, default=DEFAULT_MEMORY_RESERVE)
    args = parser.parse_args()
    if not (1 <= args.n <= (1 << 20)) or not (1 <= args.k <= args.n):
        parser.error("streamed party API domain is 1<=n<=2^20 and 1<=K<=n")

    padded = 2
    while padded < args.n:
        padded = checked(padded * 2, "padded_n", (1 << 20))
    index_bits = padded.bit_length() - 1
    comparison_bits = checked(33 + index_bits, "comparison_bits", 53)
    pairs = checked(args.n * (args.n - 1) // 2, "unordered pairs")
    ucmp_slots = checked(9 * pairs, "Select plus membership slots")
    total_slots = checked(2 * padded + 2 + ucmp_slots, "all material slots")
    key_bytes = checked(57 + 24 * comparison_bits, "serialized uCMP key bytes", U64_MAX)
    record_bytes = checked(key_bytes + GCM_TAG_BYTES, "sealed slot record bytes")
    sidecar_bytes_per_party = checked(
        STREAM_HEADER_BYTES + ucmp_slots * record_bytes, "sidecar bytes per party", I64_MAX
    )
    both_sidecars = checked(2 * sidecar_bytes_per_party, "both party sidecars", I64_MAX)
    minimum_disk = checked(both_sidecars + args.disk_reserve_bytes, "disk requirement", I64_MAX)
    disk = shutil.disk_usage(args.directory)
    available_memory = linux_available_memory()
    shell_bytes = "NOT_MEASURED"
    bundle_oracle = {
        "n": args.n,
        "K": args.k,
        "padded_n": padded,
        "index_bits": index_bits,
        "comparison_bits": comparison_bits,
        "unordered_pairs": pairs,
        "ucmp_slots_per_party": ucmp_slots,
        "all_slots_per_party": total_slots,
        "key_bytes_per_slot_plaintext": key_bytes,
        "record_bytes_per_slot_ciphertext": record_bytes,
        "sidecar_bytes_per_party_formula": sidecar_bytes_per_party,
        "both_sidecars_bytes_formula": both_sidecars,
        "shell_bundle_bytes": shell_bytes,
        "raw_and_shuffle_material_bytes": "NOT_MEASURED / O(n) generator objects",
        "per_slot_keygen_peak_bound": "one generated pair plus raw/shuffle objects; exact RSS NOT_MEASURED",
        "disk_directory": os.path.abspath(args.directory),
        "disk_available_bytes_observed": disk.free,
        "disk_reserve_bytes_policy": args.disk_reserve_bytes,
        "mem_available_bytes_observed": available_memory,
        "memory_reserve_bytes_policy": args.memory_reserve_bytes,
        "status": "CAPACITY_OK"
        if disk.free >= minimum_disk
        and (available_memory is None or available_memory >= args.memory_reserve_bytes)
        else "PRECHECK_REJECTED",
    }
    print(json.dumps(bundle_oracle, indent=2, sort_keys=True))
    return 0 if bundle_oracle["status"] == "CAPACITY_OK" else 2


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except OverflowError as exc:
        print(json.dumps({"status": "PRECHECK_REJECTED", "reason": str(exc)}, indent=2))
        raise SystemExit(2)
