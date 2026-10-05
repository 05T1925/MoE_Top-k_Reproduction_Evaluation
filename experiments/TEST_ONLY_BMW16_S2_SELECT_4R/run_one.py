"""Reproducible one-case command-line entry point for the plaintext reference."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from select4r import generated_scores, run_select


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--n", type=int, default=128)
    parser.add_argument("--k", type=int, required=True)
    parser.add_argument("--input-seed", type=int, default=17)
    parser.add_argument("--algo-seed", type=int, default=23)
    parser.add_argument("--mode", choices=["random", "ties", "all_equal", "boundaries"], default="random")
    parser.add_argument("--scores", help="comma-separated signed int32 scores; overrides --n/--mode")
    parser.add_argument("--out", type=Path, default=Path("results") / "select4r_one_case.json")
    args = parser.parse_args()
    if args.scores is None:
        scores = generated_scores(args.n, args.input_seed, args.mode)
    else:
        scores = [int(token) for token in args.scores.split(",") if token]
    summary, trace = run_select(scores, args.k, args.algo_seed, args.input_seed)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(
        json.dumps({"summary": summary, "trace": trace}, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(json.dumps(summary, sort_keys=True))
    print(f"trace={args.out.resolve()}")


if __name__ == "__main__":
    main()
