#!/usr/bin/env python3
"""TEST_ONLY algebra check for strict-< threshold membership orientation.

This checks the stable-key and complement equation only. It does not instantiate
uCMP/DCF and is not a protocol conformance or security test.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


INT32_MIN = -(1 << 31)
INT32_MAX = (1 << 31) - 1
PADDED_N = 128
INDEX_BITS = 7
MASK40 = (1 << 40) - 1


def packed_priority_key(score: int, original_index: int) -> int:
    if not INT32_MIN <= score <= INT32_MAX or not 0 <= original_index < PADDED_N:
        raise ValueError("score/index outside N=128 Protocol I key domain")
    ordered = (score & 0xFFFFFFFF) ^ 0x80000000
    high = 0xFFFFFFFF - ordered
    value = (high << INDEX_BITS) | original_index
    if value > MASK40:
        raise AssertionError("40-bit packed key overflow")
    return value


def project_membership(scores: list[int]) -> dict[str, Any]:
    n = len(scores)
    keys = [packed_priority_key(score, index) for index, score in enumerate(scores)]
    if len(set(keys)) != n:
        raise AssertionError("stable composite keys must be unique")
    order = sorted(range(n), key=keys.__getitem__)
    rows = []
    for k in range(1, n + 1):
        tau = keys[order[k - 1]]
        # Existing uCMP semantic is strict left < right. To include key_i <= tau,
        # call strict_lt(tau, key_i) and complement its reconstructed bit.
        got = [1 - int(tau < key) for key in keys]
        want = [int(key <= tau) for key in keys]
        if got != want or sum(got) != k:
            raise AssertionError(f"membership direction/count failed for K={k}")
        rows.append({"k": k, "tau_key": tau, "mask_weight": sum(got), "mask": got})
    return {"n": n, "unique_keys": True, "all_k_pass": True, "cases": rows}


def run() -> dict[str, Any]:
    fixtures = {
        "distinct_signed_boundaries": [INT32_MAX, 1, 0, -1, INT32_MIN],
        "repeated_values_stable_index": [7, 7, -3, 7, -3, 0],
        "all_equal": [INT32_MIN] * 9,
        "q20_12_near_zero_and_edges": [INT32_MIN, INT32_MAX, -1, 0, 1, -2048, 2048],
    }
    checked = {name: project_membership(scores) for name, scores in fixtures.items()}
    return {
        "label": "PROJECT_DERIVED_SELECT_5ITER",
        "scope": "PROJECT_DERIVED algebra probe; no runtime DCF key or party share",
        "comparison_abi_semantic": "strict_lt(left,right)",
        "membership_formula": "1 - strict_lt(tau,key_i) = [key_i <= tau]",
        "tie_rule": "score descending, original_index ascending",
        "packed_key_domain": {
            "packed_key_bits": 32 + INDEX_BITS,
            "comparison_ring_bits": 40,
            "index_bits": INDEX_BITS,
        },
        "fixtures": checked,
        "runtime_uCMP_DCF_conformance": "NOT_RUN; no built test executable in this worktree",
        "existing_source_test": "VFSS/tests/moe_topk/protocol_i_ucmp_conformance_test.cpp",
        "admission": "ALGEBRA_PASS_RUNTIME_INTERFACE_NOT_CLOSED",
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    result = run()
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({"result": result["admission"], "fixtures": len(result["fixtures"]),
                      "all_k_checks": sum(len(row["cases"]) for row in result["fixtures"].values())},
                     sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
