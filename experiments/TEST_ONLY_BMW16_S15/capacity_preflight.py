#!/usr/bin/env python3
"""Checked S15 bundle sizing; byte figures are lower bounds unless measured."""
from __future__ import annotations

import json

U64_MAX = (1 << 64) - 1
SIZES = (64, 128, 256, 1_000, 10_000, 100_000, 1_000_000)
MAX_BUNDLE_BYTES = 512 * 1024 * 1024


def add(a: int, b: int) -> int:
    if a < 0 or b < 0 or a > U64_MAX - b:
        raise OverflowError("uint64 add")
    return a + b


def mul(a: int, b: int) -> int:
    if a < 0 or b < 0 or (a and b > U64_MAX // a):
        raise OverflowError("uint64 multiply")
    return a * b


def choose2_checked(n: int) -> int:
    a, b = (n, n - 1)
    if a & 1:
        b //= 2
    else:
        a //= 2
    return mul(a, b)


def next_power_of_two(n: int) -> int:
    p = 2
    while p < n:
        if p > U64_MAX // 2:
            raise OverflowError("next power of two")
        p *= 2
    return p


def row(n: int) -> dict[str, int | str | list[int]]:
    if n < 1 or n > 1_000_000:
        raise ValueError("n outside requested planning domain")
    p = next_power_of_two(max(n, 2))
    index_bits = p.bit_length() - 1
    comparison_bits = add(33, index_bits)
    if comparison_bits > 53:
        raise ValueError("comparison width exceeds current uCMP 53-bit cap")
    pairs = choose2_checked(n)
    pair_slots = mul(9, pairs)
    slots = add(add(mul(2, p), 2), pair_slots)
    manifest_bytes = add(8, mul(20, slots))
    raw_key_bytes = mul(mul(2, p), add(57, mul(24, 34)))
    pair_key_bytes = mul(pair_slots, add(57, mul(24, comparison_bits)))
    lower = add(add(raw_key_bytes, pair_key_bytes), add(manifest_bytes, 116))
    return {
        "n": n,
        "padded_n": p,
        "index_bits": index_bits,
        "comparison_bits": comparison_bits,
        "pair_count": pairs,
        "slots_per_party": slots,
        "manifest_bytes_per_party": manifest_bytes,
        "raw_score_key_bytes_per_party": raw_key_bytes,
        "pair_key_bytes_per_party": pair_key_bytes,
        "party_bundle_bytes_lower_bound": lower,
        "two_party_bytes_lower_bound": mul(2, lower),
        "bundle_cap_bytes": MAX_BUNDLE_BYTES,
        "lower_bound_fits_bundle_cap": lower <= MAX_BUNDLE_BYTES,
        "runtime_domain": "SUPPORTED_BOUNDED" if n <= 256 else "PREFLIGHT_ONLY",
        "K_examples": [1, (n + 1) // 2, n],
    }


def main() -> None:
    out = [row(n) for n in SIZES]
    print(json.dumps({
        "label": "BMW16_S15_CHECKED_CAPACITY_PREFLIGHT_NOT_MEASURED",
        "key_bytes_formula": "57+24*b",
        "slot_formula": "2*padded_n+2+9*C(n,2)",
        "party_bundle_bytes_lower_bound_formula":
            "2*padded_n*(57+24*34)+9*C(n,2)*(57+24*comparison_bits)+manifest_bytes+116",
        "notes": [
            "All arithmetic is checked uint64.",
            "The lower bound excludes neither ciphertext compression nor fixed bundle overhead beyond the stated 116 bytes; it is not a measured package size.",
            "n<=256 is an API bound only after this S15 code change; E2E evidence is reported separately.",
        ],
        "rows": out,
    }, indent=2))


if __name__ == "__main__":
    main()
