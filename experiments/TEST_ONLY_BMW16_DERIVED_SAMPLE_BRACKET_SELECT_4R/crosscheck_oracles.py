#!/usr/bin/env python3
"""Cross-check frozen Python and repository C++ stable-order oracles."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
import shutil
import subprocess
from pathlib import Path

from run_matrix import CONFIGS, INT32_MAX, INT32_MIN, make_scores, seed64


HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
ORACLE_PATH = REPO / "experiments" / "TEST_ONLY_BMW16_DERIVED_SELECT_4R" / "oracle.py"
spec = importlib.util.spec_from_file_location("frozen_bmw16_oracle", ORACLE_PATH)
oracle = importlib.util.module_from_spec(spec)
assert spec and spec.loader
spec.loader.exec_module(oracle)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description="Compare Python and C++ frozen Top-K oracles")
    parser.add_argument("--out-dir", type=Path, required=True, help="external build/evidence directory")
    args = parser.parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=True)
    cpp = shutil.which("g++") or r"C:\Program Files\MinGW64\bin\g++.exe"
    if not Path(cpp).exists() and shutil.which(cpp) is None:
        raise SystemExit("g++ is unavailable; C++ oracle cross-check was not run")
    exe = args.out_dir / ("oracle_harness.exe" if os.name == "nt" else "oracle_harness")
    source = HERE / "oracle_harness.cpp"
    include = REPO / "VFSS" / "include"
    command = [cpp, "-std=c++17", "-O2", "-I", str(include), str(source), "-o", str(exe)]
    subprocess.run(command, check=True, capture_output=True, text=True)

    cases = [
        ("boundary_two", [INT32_MIN, INT32_MAX], 1),
        ("boundary_two", [INT32_MIN, INT32_MAX], 2),
        ("all_equal", [0, 0, 0, 0, 0], 1),
        ("all_equal", [0, 0, 0, 0, 0], 3),
        ("all_equal", [0, 0, 0, 0, 0], 5),
        ("mixed_signed_ties", [INT32_MIN, 7, 7, INT32_MAX, -1, 7, INT32_MIN], 1),
        ("mixed_signed_ties", [INT32_MIN, 7, 7, INT32_MAX, -1, 7, INT32_MIN], 4),
        ("mixed_signed_ties", [INT32_MIN, 7, 7, INT32_MAX, -1, 7, INT32_MIN], 7),
    ]
    for n, k in CONFIGS:
        input_seed = seed64(f"fixed-input|n{n}_k{k}")
        cases.append((f"official_fixed_n{n}_k{k}", make_scores(n, input_seed), k))
    stdin = [str(len(cases))]
    for _, scores, k in cases:
        stdin.append(f"{len(scores)} {k}")
        stdin.append(" ".join(str(x) for x in scores))
    completed = subprocess.run([str(exe)], input="\n".join(stdin) + "\n", check=True,
                               capture_output=True, text=True)
    lines = completed.stdout.strip().splitlines()
    if len(lines) != len(cases):
        raise SystemExit(f"C++ oracle returned {len(lines)} rows for {len(cases)} cases")
    results = []
    for (label, scores, k), line in zip(cases, lines):
        selected_text, mask_text = line.split()
        cpp_selected = int(selected_text)
        cpp_mask = [int(bit) for bit in mask_text]
        py_selected = oracle.select_index(scores, k)
        py_mask = oracle.top_k_mask(scores, k)
        if cpp_selected != py_selected or cpp_mask != py_mask:
            raise SystemExit(f"Python/C++ oracle mismatch for {label}, n={len(scores)}, K={k}")
        results.append({
            "case": label, "n": len(scores), "K": k,
            "python_selected": py_selected, "cpp_selected": cpp_selected,
            "mask_count": sum(cpp_mask),
            "input_sha256": hashlib.sha256(json.dumps(scores, separators=(",", ":")).encode()).hexdigest(),
            "status": "AGREE",
        })
    output = {
        "cases": len(results), "status": "PASS", "python_oracle": str(ORACLE_PATH),
        "cpp_oracle_header": str(include / "moe_topk" / "topk_oracle.h"),
        "cpp_binary_sha256": sha(exe), "cpp_source_sha256": sha(source),
        "compile_command": command, "results": results,
    }
    out = args.out_dir / "python_cpp_oracle_crosscheck.json"
    out.write_text(json.dumps(output, sort_keys=True, indent=2), encoding="utf-8")
    print(json.dumps({"status": output["status"], "cases": len(results), "binary_sha256": output["cpp_binary_sha256"]}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
