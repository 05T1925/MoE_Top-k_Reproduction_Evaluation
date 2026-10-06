#!/usr/bin/env python3
"""S14 bounded n=128 S4/C++ trace differential; all generated evidence stays external."""
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
    ap.add_argument("--cpp-test", required=True, help="Linux path of the built party TEST_ONLY binary")
    ap.add_argument("--s4", required=True, type=Path, help="frozen S4 select4r.py")
    ap.add_argument("--out", required=True, type=Path, help="external evidence directory")
    ap.add_argument("--wsl-distro", default="Ubuntu-24.04")
    ap.add_argument("--first-seed", type=int, default=814000)
    args = ap.parse_args()
    if args.out.resolve().is_relative_to(Path(__file__).resolve().parents[2]):
        raise SystemExit("evidence output must be outside the repository")
    args.out.mkdir(parents=True, exist_ok=True)
    compare = Path(__file__).parents[1] / "TEST_ONLY_BMW16_S12" / "compare_common_tape.py"
    rng = random.Random(args.first_seed ^ 0x5331344E313238)
    specs = []
    specs.append((2, [-(1 << 31) if i == 0 else (1 << 31) - 1 if i == 127
                      else (i * 37) % 101 - 50 for i in range(128)]))
    specs.append((8, [(i * 17) % 23 - 11 for i in range(128)]))
    specs.append((80, [rng.randint(-1000, 1000) for _ in range(128)]))
    cases = []
    for ci, (k, scores) in enumerate(specs):
        seed = args.first_seed + ci
        case_dir = args.out / f"n128_k{k}"
        case_dir.mkdir(parents=True, exist_ok=True)
        tape_rng = random.Random(seed ^ 0x636F6D6D6F4E3132)
        words = [[[tape_rng.getrandbits(64) for _ in range(1024)]
                  for _ in range(2)] for _ in range(2)]
        tape_path = case_dir / "common_tape.txt"
        lines = ["BMW16_S12_TAPE_V1"]
        for task in words:
            for domain in task:
                lines.extend((str(len(domain)), " ".join(map(str, domain))))
        tape_path.write_text("\n".join(lines) + "\n", encoding="ascii")
        cpp_cmd = ["wsl.exe", "-d", args.wsl_distro, "--", args.cpp_test,
                   "--trace-case-tape", as_wsl_path(case_dir), as_wsl_path(tape_path),
                   "128", str(k), str(seed), *(str(x) for x in scores)]
        run = subprocess.run(cpp_cmd, check=True, capture_output=True, text=True,
                             encoding="utf-8", errors="replace")
        (case_dir / "cpp_run.txt").write_text(run.stdout, encoding="utf-8")
        if "status=SUCCESS" not in run.stdout:
            raise RuntimeError(f"n=128 K={k} C++ selector did not succeed: {run.stdout[-1000:]}")
        diff_path = case_dir / "differential.json"
        compare_cmd = [sys.executable, str(compare), "--s4", str(args.s4),
                       "--tape", str(case_dir / "common_tape.json"),
                       "--cpp-trace", str(case_dir / "cpp_trace.txt"),
                       "--report", str(diff_path)]
        subprocess.run(compare_cmd, check=True, capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
        diff = json.loads(diff_path.read_text(encoding="utf-8"))
        if diff.get("result") != "PASS":
            raise RuntimeError(f"n=128 K={k} common-tape mismatch: {diff}")
        cases.append({"n": 128, "K": k, "seed": seed, "cpp_status": "SUCCESS",
                      "result": "PASS", "tape_sha256": sha256(case_dir / "common_tape.json"),
                      "trace_sha256": sha256(case_dir / "cpp_trace.txt"),
                      "differential_sha256": sha256(diff_path), "artifact_dir": case_dir.name})
    report = {"label": "BMW16_S14_N128_COMMON_RANDOM_TAPE_DIFFERENTIAL_TEST_ONLY",
              "n": 128, "cases": cases, "case_count": len(cases), "result": "PASS",
              "s4_sha256": sha256(args.s4), "binary": args.cpp_test}
    out = args.out / "matrix.json"
    out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"result": "PASS", "cases": len(cases), "matrix_sha256": sha256(out),
                      "matrix": str(out)}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
