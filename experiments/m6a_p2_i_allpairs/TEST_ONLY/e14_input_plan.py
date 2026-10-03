#!/usr/bin/env python3
"""Create an E14 TEST_ONLY matched input schedule, independently of T labels."""

import argparse
import hashlib
import json
from pathlib import Path
import secrets


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    plan = {f"k{k}-rep{rep}": secrets.randbits(64)
            for k in (2, 8) for rep in range(6)}
    while len(set(plan.values())) != len(plan):
        plan = {f"k{k}-rep{rep}": secrets.randbits(64)
                for k in (2, 8) for rep in range(6)}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("x", encoding="utf-8") as stream:
        stream.write(json.dumps(plan, sort_keys=True, indent=2) + "\n")
    print(f"E14_INPUT_PLAN sha256={hashlib.sha256(args.output.read_bytes()).hexdigest().upper()} "
          f"path={args.output}")


if __name__ == "__main__":
    main()
