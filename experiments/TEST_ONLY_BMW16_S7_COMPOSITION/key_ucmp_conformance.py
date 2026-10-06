#!/usr/bin/env python3
"""Real uCMP/DCF conformance for S6 strict real+dummy key geometry."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

from fss_adapter import FssBackend


MIN = -(1 << 31)
MAX = (1 << 31) - 1
MASK64 = (1 << 64) - 1


def geometry(n: int, k: int) -> dict[str, int]:
    r = n - k + 1
    base = n + 1
    low = max(base - 2 * r, 0)
    high = max(2 * r - base, 0)
    m = base + low + high
    b = max(1, (n - 1).bit_length())
    rmax = ((1 << 32) - 1) * (1 << b) + (n - 1)
    return {"r": r, "L": low, "H": high, "M": m, "b": b,
            "Rmax": rmax, "numeric_code_bits": 33 + b, "Bin": 34 + b,
            "maximum_code": rmax + m - n + 1}


def real_key(score: int, index: int, n: int) -> int:
    b = max(1, (n - 1).bit_length())
    ordered = (score & 0xFFFFFFFF) ^ 0x80000000
    return (ordered << b) + (n - 1 - index)


def verify_compare(backend: FssBackend, bits: int, left: int, right: int) -> tuple[bool, bool]:
    c0, c1 = backend.compare_values(left, right, bits)
    clear = (c0 + c1) & MASK64
    expected = int(left < right)
    if clear != expected:
        raise AssertionError(f"uCMP mismatch bits={bits} left={left} right={right} got={clear}")
    if left != right:
        reverse = ((1 - c0) & MASK64, (-c1) & MASK64)
        if ((reverse[0] + reverse[1]) & MASK64) != int(right < left):
            raise AssertionError("(1-c0,-c1) reverse-share conversion mismatch")
        return True, True
    return True, False


def run(executable: str, seed: int) -> dict[str, Any]:
    rows = []
    comparisons = 0
    reverse_conversions = 0
    backend = FssBackend(executable, seed)
    try:
        for n in (1, 128, 256, 1000):
            for k in sorted({1, max(1, n // 2), n}):
                g = geometry(n, k)
                if not 34 <= g["Bin"] <= 53 or g["maximum_code"] >= (1 << (g["Bin"] - 1)):
                    raise AssertionError(f"uCMP width/overflow n={n} K={k}: {g}")
                cases = [
                    (real_key(MIN, n - 1, n), real_key(MAX, 0, n)),
                    (real_key(0, n - 1, n), real_key(0, 0, n)),
                    (real_key(-1, 0, n), real_key(0, n - 1, n)),
                ]
                if n > 1:
                    cases.extend([
                        (real_key(17, n - 1, n), real_key(17, 0, n)),
                        (real_key(MAX, n - 1, n), real_key(MAX, 0, n)),
                    ])
                # Exact low-pad/sentinel/real/high-sentinel/high-pad code ladder.
                rlo = g["L"] + 1
                rhi = g["L"] + 1 + g["Rmax"]
                ladder = []
                if g["L"]:
                    ladder.extend([0, g["L"]])
                else:
                    ladder.append(g["L"])
                ladder.append(rlo)
                if rhi != rlo:
                    ladder.append(rhi)
                ladder.append(g["L"] + g["Rmax"] + 2)
                if g["H"]:
                    ladder.append(g["L"] + g["Rmax"] + 3)
                if any(a >= b for a, b in zip(ladder, ladder[1:])):
                    raise AssertionError(f"dummy ladder not strict n={n} K={k}: {ladder}")
                cases.extend(zip(ladder, ladder[1:]))
                for left, right in cases:
                    _, converted = verify_compare(backend, g["Bin"], left, right)
                    comparisons += 1
                    reverse_conversions += int(converted)
                rows.append({"n": n, "K": k, **g, "uCMP_conformance_comparisons": len(cases),
                             "stable_extrema_ties_dummy_ladder": "PASS"})
        # Re-run the exact S6 width bug against the actual implementation,
        # then demonstrate the corrected width on the same ordered pair.
        witness_left, witness_right = 0, (1 << 33) + 1
        bad0, bad1 = backend.compare_values(witness_left, witness_right, bits=34)
        bad_open = (bad0 + bad1) & MASK64
        if bad_open != 0 or witness_left >= witness_right:
            raise AssertionError("expected the recorded S6 34-bit boundary failure changed")
        good0, good1 = backend.compare_values(witness_left, witness_right, bits=35)
        good_open = (good0 + good1) & MASK64
        if good_open != 1:
            raise AssertionError("corrected 35-bit uCMP did not compare boundary witness")
        comparisons += 2
        backend.stop()
    finally:
        if backend.process.poll() is None:
            backend.process.kill()
            backend.process.wait(timeout=5)
    return {
        "label": "BMW16_S7_REAL_UCMP_DCF_KEY_AND_REVERSE_SHARE_TEST_ONLY",
        "configurations": rows,
        "actual_uCMP_DCF_comparisons": comparisons,
        "two_party_DCF_Eval_count": 4 * comparisons,
        "reverse_affine_share_conversions_checked": reverse_conversions,
        "implementation": "ProtocolIUcmpMaterial and real keyGenDCF/evalDCF in VFSS; centralized TEST_ONLY harness",
        "s6_advertised_width_failure_witness": {
            "n": 2, "K": 1, "bits_advertised_as_34": 34,
            "actual_uCMP_safe_nonnegative_limit": 1 << 33,
            "left_code": 0, "right_code": (1 << 33) + 1,
            "mathematical_left_lt_right": True,
            "actual_uCMP_reconstructed_bit_at_34_bits": 0,
            "corrected_width": 35,
            "actual_uCMP_reconstructed_bit_at_35_bits": 1,
        },
        "n_1e6": {"numeric_code_bits": geometry(1_000_000, 1)["numeric_code_bits"],
                  "required_actual_uCMP_bits": geometry(1_000_000, 1)["Bin"],
                  "comparison_input_range_test": "UNSUPPORTED_BY_CURRENT_34_TO_53_BIT_API; would require 54-bit uCMP or a separately proved alternative encoding"},
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--fss-backend", required=True,
                        help="native executable path or wsl:/absolute/linux/path")
    parser.add_argument("--seed", type=int, default=20261006)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = run(args.fss_backend, args.seed)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"BMW16_S7_UCMP_KEY_CONFORMANCE PASS configs={len(result['configurations'])} "
          f"comparisons={result['actual_uCMP_DCF_comparisons']}")


if __name__ == "__main__":
    main()
