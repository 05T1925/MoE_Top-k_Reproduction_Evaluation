#!/usr/bin/env python3
"""Execute the E20 benchmark's first gate for all 12 large configurations."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("binary", type=Path)
    parser.add_argument("output_jsonl", type=Path)
    args = parser.parse_args()
    binary_hash = hashlib.sha256(args.binary.read_bytes()).hexdigest().upper()
    with args.output_jsonl.open("x", encoding="utf-8") as stream:
        for n in (10000, 100000, 1000000):
            for r in (2, 3, 4, 5):
                command = [str(args.binary), "bench-stream", str(n), "80", str(r),
                           "20201004", str(2900000 + n + r)]
                result = subprocess.run(
                    command, env=dict(os.environ, MOE_TOPK_M6A_E15_BENCH="1"),
                    text=True, capture_output=True, timeout=10)
                if result.returncode != 1 or \
                        "E20 stream benchmark shape" not in result.stderr:
                    raise RuntimeError(f"unexpected large-shape entry n={n} r={r}")
                stream.write(json.dumps({"n": n, "K": 80, "r": r,
                                         "command": command,
                                         "binary_sha256": binary_hash,
                                         "exit_code": result.returncode,
                                         "stdout": result.stdout,
                                         "stderr": result.stderr,
                                         "first_gate": "E20_STREAM_BENCHMARK_SHAPE",
                                         "keygen_entered": False}) + "\n")
    print("E20_LARGE_SHAPE_PROBE_PASS rows=12 first_gate=E20_STREAM_BENCHMARK_SHAPE")


if __name__ == "__main__":
    main()
