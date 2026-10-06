#!/usr/bin/env python3
"""S15 n=256 S4/C++ intermediate trace differential; evidence is external."""
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
        raise ValueError(f"expected Windows drive path for WSL mapping: {resolved}")
    return f"/mnt/{match.group(1).lower()}/{match.group(2)}"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--cpp-test", required=True, help="WSL path to rebuilt party TEST_ONLY binary")
    ap.add_argument("--s4", required=True, type=Path, help="frozen S4 select4r.py")
    ap.add_argument("--out", required=True, type=Path, help="external evidence directory")
    ap.add_argument("--wsl-distro", default="Ubuntu-24.04")
    ap.add_argument("--first-seed", type=int, default=825600)
    args = ap.parse_args()
    if args.out.resolve().is_relative_to(Path(__file__).resolve().parents[2]):
        raise SystemExit("evidence output must be outside the repository")
    if args.out.exists() and any(args.out.iterdir()):
        raise SystemExit("output directory must be new and empty")
    args.out.mkdir(parents=True, exist_ok=True)
    compare = Path(__file__).parents[1] / "TEST_ONLY_BMW16_S12" / "compare_common_tape.py"
    specs = [
        (2, [-(1 << 31) if i == 0 else (1 << 31) - 1 if i == 255 else (i * 71) % 257 - 128
             for i in range(256)]),
        (8, [((i * 17) % 23) - 11 if i not in (1, 254) else (1 << 31) - 1 if i == 1 else -(1 << 31)
             for i in range(256)]),
    ]
    cases = []
    for ci, (k, scores) in enumerate(specs):
        seed = args.first_seed + ci
        case_dir = args.out / f"n256_k{k}"
        case_dir.mkdir(parents=True, exist_ok=False)
        tape_rng = random.Random(seed ^ 0x636F6D6D6F4E3235)
        words = [[[tape_rng.getrandbits(64) for _ in range(4096)]
                  for _ in range(2)] for _ in range(2)]
        tape_path = case_dir / "common_tape.txt"
        lines = ["BMW16_S12_TAPE_V1"]
        for task in words:
            for domain in task:
                lines.extend((str(len(domain)), " ".join(map(str, domain))))
        tape_path.write_text("\n".join(lines) + "\n", encoding="ascii")
        cpp_cmd = ["wsl.exe", "-d", args.wsl_distro, "--", args.cpp_test,
                   "--trace-case-tape", as_wsl_path(case_dir), as_wsl_path(tape_path),
                   "256", str(k), str(seed), *(str(x) for x in scores)]
        run = subprocess.run(cpp_cmd, check=True, capture_output=True, text=True,
                             encoding="utf-8", errors="replace")
        (case_dir / "cpp_run.txt").write_text(run.stdout, encoding="utf-8")
        if "status=SUCCESS" not in run.stdout:
            raise RuntimeError(f"n=256 K={k} C++ selector did not succeed: {run.stdout[-1200:]}")
        diff_path = case_dir / "differential.json"
        compare_cmd = [sys.executable, str(compare), "--s4", str(args.s4),
                       "--tape", str(case_dir / "common_tape.json"),
                       "--cpp-trace", str(case_dir / "cpp_trace.txt"),
                       "--report", str(diff_path)]
        subprocess.run(compare_cmd, check=True, capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
        diff = json.loads(diff_path.read_text(encoding="utf-8"))
        if diff.get("result") != "PASS":
            raise RuntimeError(f"n=256 K={k} common-tape mismatch: {diff}")
        cases.append({"n": 256, "K": k, "seed": seed, "cpp_status": "SUCCESS",
                      "result": "PASS", "tape_sha256": sha256(case_dir / "common_tape.json"),
                      "trace_sha256": sha256(case_dir / "cpp_trace.txt"),
                      "differential_sha256": sha256(diff_path), "artifact_dir": case_dir.name})
    matrix = {"label": "BMW16_S15_N256_COMMON_RANDOM_TAPE_DIFFERENTIAL_TEST_ONLY",
              "cases": cases, "case_count": len(cases), "result": "PASS",
              "s4_sha256": sha256(args.s4), "binary": args.cpp_test}
    path = args.out / "matrix.json"
    path.write_text(json.dumps(matrix, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"result": "PASS", "cases": len(cases), "matrix_sha256": sha256(path),
                      "matrix": str(path)}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
