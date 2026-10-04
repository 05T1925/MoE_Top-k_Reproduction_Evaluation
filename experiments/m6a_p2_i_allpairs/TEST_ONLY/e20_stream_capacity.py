#!/usr/bin/env python3
"""Checked fixed-record capacity, never a keygen or performance estimate."""

import argparse
import csv
from pathlib import Path
import sys


MAX = 2**64 - 1


def checked(value):
    if value < 0 or value > MAX:
        raise OverflowError("uint64 capacity")
    return value


def row(n, r, physical_disk_free):
    d = 2
    while d < n:
        d = checked(2*d)
    bits = 33 + (d-1).bit_length()
    pairs = checked(d*(d-1)//2)
    slots = checked(r*pairs)
    key_bytes = checked(24*bits+57)
    record_bytes = checked(32+key_bytes+16)
    party_file = checked(80+slots*record_bytes)
    both_files = checked(2*party_file)
    return dict(n=n, K=80, r=r, D=d, comparison_bits=bits,
                pairs_per_round=pairs, all_candidate_edges=slots,
                serialized_key_bytes=key_bytes, sealed_record_bytes=record_bytes,
                sealed_file_per_party_bytes=party_file,
                sealed_files_both_bytes=both_files,
                physical_disk_free_bytes=physical_disk_free,
                program_first_gate="E20_LOGICAL_N_GT_1000" if n > 1000 else "NONE",
                resource_status="RESOURCE_INFEASIBLE" if both_files > physical_disk_free
                else "DISK_CAPACITY_ONLY_PASS_NOT_EXECUTION")


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--physical-disk-free-bytes", type=int, required=True)
    p.add_argument("--output", type=Path, required=True)
    args = p.parse_args()
    if args.physical_disk_free_bytes <= 0:
        raise ValueError("physical disk snapshot required")
    rows = [row(n,r,args.physical_disk_free_bytes)
            for n in (1000,10000,100000,1000000) for r in (2,3,4,5)]
    with args.output.open("x", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)
    print(f"E20_CAPACITY_PASS rows={len(rows)}", file=sys.stdout)


if __name__ == "__main__":
    main()
