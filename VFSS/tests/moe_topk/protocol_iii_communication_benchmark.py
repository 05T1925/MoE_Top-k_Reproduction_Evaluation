#!/usr/bin/env python3
"""TEST_ONLY: run and validate independent-process M5-H1 communication CSV."""
import argparse
import csv
import subprocess
from pathlib import Path

DEFAULT_SCALES = (2, 3, 4, 5, 8, 16, 20, 32, 64, 128)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def validate(rows, scales, repeats):
    check(len(rows) == len(scales) * repeats * 4, "benchmark row count")
    for n in scales:
        group = [row for row in rows if int(row["n"]) == n]
        check(len(group) == repeats * 4, f"n={n} row count")
        expected = sorted((mode, target, repetition)
                          for repetition in range(repeats)
                          for mode, target in (("Fselect", 0),
                                               ("Fselect", n // 2),
                                               ("Fselect", n - 1),
                                               ("Fsort", 0)))
        actual = sorted((row["mode"], int(row["target"]), int(row["repeat"]))
                        for row in group)
        check(actual == expected, f"n={n} mode/target/repeat matrix")
        fixed = ("padded_n", "comparison_bits", "rank_bits",
                 "r1_logical_bits", "r2_logical_bits", "p0_r1_wire",
                 "p1_r1_wire", "p0_r2_wire", "p1_r2_wire",
                 "r1_wire_total", "r2_wire_total", "online_wire_total",
                 "p0_offline_bundle", "p1_offline_bundle",
                 "offline_bundle_total")
        for field in fixed:
            check(len({row[field] for row in group}) == 1,
                  f"n={n} nondeterministic {field}")
        for row in group:
            b = int(row["comparison_bits"])
            r = int(row["rank_bits"])
            check(int(row["padded_n"]) == 1 << r, "padded n")
            check(b == 33 + r, "comparison bits")
            check(int(row["r1_logical_bits"]) == 2 * n * (b + 254), "R1 logical")
            check(int(row["r2_logical_bits"]) == 2 * n * (r + 127), "R2 logical")
            a, b1 = int(row["p0_r1_wire"]), int(row["p1_r1_wire"])
            c, d = int(row["p0_r2_wire"]), int(row["p1_r2_wire"])
            check(a == b1 == 92 + 40 * n and c == d == 92 + 24 * n,
                  "application wire/schema")
            check(int(row["r1_wire_total"]) == a + b1, "R1 wire sum")
            check(int(row["r2_wire_total"]) == c + d, "R2 wire sum")
            check(int(row["online_wire_total"]) == a + b1 + c + d,
                  "total online wire sum")
            check(int(row["offline_bundle_total"]) ==
                  int(row["p0_offline_bundle"]) + int(row["p1_offline_bundle"]),
                  "offline bundle sum")
            check(int(row["rounds"]) == 2 and
                  int(row["post_r2_protocol_frames"]) == 0, "causal rounds")
            check(int(row["full_eval_domain"]) == 1 << r, "DPF domain")
            check(int(row["cmp_edges_per_party"]) == n * (n - 1) // 2,
                  "CmpAgg edge count")
            if row["mode"] == "Fselect":
                check(int(row["dpf_eval_per_party"]) == n and
                      int(row["full_eval_per_party"]) == 0, "Fselect DPF work")
            else:
                check(int(row["dpf_eval_per_party"]) == 0 and
                      int(row["full_eval_per_party"]) == n and
                      int(row["full_eval_leaves_per_party"]) == n * (1 << r),
                      "Fsort FullEval work")
            # Frozen Theorem 4.2 base formula with |H| bit length 127.
            ell = int(row["comparison_bits"])
            paper = 6 * n * ell + 2 * n * r + 4 * n * (127 - ell)
            implementation = int(row["r1_logical_bits"]) + int(row["r2_logical_bits"])
            check(implementation - paper == 254 * n,
                  "paper/implementation delta changed; re-audit, do not relabel")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("output_csv", type=Path)
    parser.add_argument("--repeats", type=int, default=2)
    parser.add_argument("--scales", type=int, nargs="+", default=DEFAULT_SCALES)
    args = parser.parse_args()
    check(1 <= args.repeats <= 5 and all(2 <= n <= 128 for n in args.scales) and
          len(set(args.scales)) == len(args.scales), "benchmark arguments")
    rows = []
    for n in args.scales:
        result = subprocess.run([str(args.executable), "--comm-benchmark",
                                 str(n), str(args.repeats)],
                                capture_output=True, text=True, check=True,
                                timeout=900)
        rows.extend(csv.DictReader(result.stdout.splitlines()))
        print(f"n={n} rows={args.repeats * 4}", flush=True)
    validate(rows, args.scales, args.repeats)
    args.output_csv.parent.mkdir(parents=True, exist_ok=True)
    with args.output_csv.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)
    print(f"VALIDATION_PASS rows={len(rows)} scales={len(args.scales)} "
          f"repeats={args.repeats} output={args.output_csv}")


if __name__ == "__main__":
    main()
