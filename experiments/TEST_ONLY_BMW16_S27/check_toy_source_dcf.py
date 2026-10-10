#!/usr/bin/env python3
"""Exhaustively check the TEST_ONLY small-word DCF source transcription."""

from __future__ import annotations

import argparse
import hashlib

from toy_source_dcf import FixedExpansionTape, eval_party_trace, keygen


def run(seed: int, max_bits: int) -> tuple[int, str]:
    checked = 0
    trace_hash = hashlib.sha256()
    for bits in range(1, max_bits + 1):
        domain = 1 << bits
        for alpha in range(domain):
            for payload in (0, 1, 255):
                tape = FixedExpansionTape(bits + 1, 8, seed + bits)
                pair = keygen(bits, alpha, payload, tape, seed ^ (bits << 16))
                for x in range(domain):
                    left, left_steps = eval_party_trace(0, x, pair, tape)
                    right, right_steps = eval_party_trace(1, x, pair, tape)
                    assert len(left_steps) == bits and len(right_steps) == bits
                    expected = payload if x < alpha else 0
                    actual = (left + right) & 255
                    assert actual == expected, (
                        f"bits={bits} alpha={alpha} x={x} payload={payload} "
                        f"got={actual} expected={expected}"
                    )
                    trace_hash.update(
                        f"{bits}:{alpha}:{payload}:{x}:{left}:{right}:"
                        f"{left_steps!r}:{right_steps!r}\n".encode()
                    )
                    checked += 1
    return checked, trace_hash.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--seed", type=int, default=270109)
    parser.add_argument("--max-bits", type=int, default=5)
    args = parser.parse_args()
    if not 1 <= args.max_bits <= 5:
        parser.error("--max-bits must be in 1..5 for this exhaustive toy run")
    checked, trace_hash = run(args.seed, args.max_bits)
    print(
        f"TOY_SOURCE_DCF max_bits={args.max_bits} input_checks={checked} "
        f"fixed_tape_seed={args.seed} trace_sha256={trace_hash} result=PASS"
    )
    print("LIMITATION=TEST_ONLY small-word transcription; no AES or cryptographic proof")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
