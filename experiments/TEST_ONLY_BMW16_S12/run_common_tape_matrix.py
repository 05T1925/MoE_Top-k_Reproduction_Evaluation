#!/usr/bin/env python3
"""Run S4/C++ common-tape differential for every K at n=2..8 (TEST_ONLY)."""
from __future__ import annotations

import argparse
import hashlib
import json
import random
from pathlib import Path
import re
import subprocess
import sys


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def as_wsl_path(path: Path) -> str:
    resolved = path.resolve().as_posix()
    match = re.match(r"^([A-Za-z]):/(.*)$", resolved)
    if not match:
        raise ValueError(f"expected a Windows drive path for WSL mapping: {resolved}")
    return f"/mnt/{match.group(1).lower()}/{match.group(2)}"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--cpp-test", required=True)
    ap.add_argument("--s4", required=True, type=Path)
    ap.add_argument("--out", required=True, type=Path)
    ap.add_argument("--wsl-distro", help="run the Linux C++ test through wsl.exe")
    ap.add_argument("--first-seed", type=int, default=50000)
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    compare = Path(__file__).with_name("compare_common_tape.py")
    cases = []
    specifications = [(n, k, [13 * i - 31 for i in range(n)])
                      for n in range(2, 9) for k in range(1, n + 1)]
    specifications.extend([
        (64, 1, [(i * 17) % 23 - 11 for i in range(64)]),
        (64, 8, [(i * 17) % 23 - 11 for i in range(64)]),
        (64, 32, [(i * 7) % 64 - 31 for i in range(64)]),
        (64, 63, [(-(1 << 31) if i == 0 else (1 << 31) - 1 if i == 63 else (i * 11) % 19 - 9)
                  for i in range(64)]),
    ])
    for case_index, (n, k, scores) in enumerate(specifications):
        seed = args.first_seed + 100 * n + k + case_index * 10000
        case_dir = args.out / f"n{n}_k{k}"
        case_dir.mkdir(parents=True, exist_ok=True)
        tape_rng = random.Random(seed ^ 0x636F6D6D6F6E7461)
        words = [[[tape_rng.getrandbits(64) for _ in range(1024)]
                  for _ in range(2)] for _ in range(2)]
        tape_path = case_dir / "common_tape.txt"
        tape_lines = ["BMW16_S12_TAPE_V1"]
        for task in words:
            for domain in task:
                tape_lines.append(str(len(domain)))
                tape_lines.append(" ".join(str(word) for word in domain))
        tape_path.write_text("\n".join(tape_lines) + "\n", encoding="ascii")
        if args.wsl_distro:
            case_dir_arg = as_wsl_path(case_dir)
            tape_path_arg = as_wsl_path(tape_path)
            cmd = ["wsl.exe", "-d", args.wsl_distro, "--", args.cpp_test,
                   "--trace-case-tape", case_dir_arg, tape_path_arg, str(n), str(k), str(seed),
                   *(str(x) for x in scores)]
        else:
            case_dir_arg = str(case_dir)
            tape_path_arg = str(tape_path)
            cmd = [args.cpp_test, "--trace-case-tape", case_dir_arg, tape_path_arg, str(n), str(k), str(seed),
                   *(str(x) for x in scores)]
        run = subprocess.run(cmd, check=True, capture_output=True, text=True,
                             encoding="utf-8", errors="replace")
        (case_dir / "cpp_run.txt").write_text(run.stdout, encoding="utf-8")
        m = re.search(r"status=([^ ]+)", run.stdout)
        if not m:
            raise RuntimeError(f"missing status n={n} K={k}: {run.stdout[-1000:]}")
        diff_path = case_dir / "differential.json"
        cmd = [sys.executable, str(compare), "--s4", str(args.s4),
               "--tape", str(case_dir / "common_tape.json"),
               "--cpp-trace", str(case_dir / "cpp_trace.txt"),
               "--report", str(diff_path)]
        subprocess.run(cmd, check=True, capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
        diff = json.loads(diff_path.read_text(encoding="utf-8"))
        if diff.get("result") != "PASS":
            raise RuntimeError(f"differential failed n={n} K={k}: {diff}")
        cases.append({"n": n, "K": k, "seed": seed, "scores": scores,
                      "cpp_status": m.group(1), "result": diff["result"],
                      "input_tape_sha256": sha256(tape_path),
                      "tape_sha256": sha256(case_dir / "common_tape.json"),
                      "cpp_trace_sha256": sha256(case_dir / "cpp_trace.txt"),
                      "differential_sha256": sha256(diff_path),
                      "artifact_dir": case_dir.name})
    report = {"label": "BMW16_S12_COMMON_RANDOM_TAPE_SMALL_RANKS_AND_N64_TEST_ONLY",
              "n_range": [2, 64], "all_small_K": True,
              "n64_input_profiles": ["repeated_scores", "strict_order", "INT32_extremes_and_repeats"],
              "case_count": len(cases), "result": "PASS",
              "cpp_test": args.cpp_test, "s4_sha256": sha256(args.s4),
              "cases": cases}
    out = args.out / "matrix.json"
    out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"result": report["result"], "case_count": len(cases),
                      "sha256": sha256(out), "report": str(out)}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
