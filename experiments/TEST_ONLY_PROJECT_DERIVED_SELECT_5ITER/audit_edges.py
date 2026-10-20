#!/usr/bin/env python3
"""Independent trace/edge auditor for PROJECT_DERIVED_SELECT_5ITER_TEST_ONLY.

This module intentionally does not import the selector implementation.
"""

from __future__ import annotations

import hashlib
import json
import math
import random
import struct
from pathlib import Path
from typing import Any


LABEL = "PROJECT_DERIVED_SELECT_5ITER_TEST_ONLY"
REVISION = "v1"


def _key(scores: list[int], item: int) -> tuple[int, int]:
    return (-scores[item], item)


def _seed(master: int, round_no: int) -> int:
    data = f"{LABEL}:{REVISION}:{master}:{round_no}".encode("ascii")
    return int.from_bytes(hashlib.sha256(data).digest()[:8], "big")


def _params(m: int, r: int, n: int) -> tuple[int, int, int, int]:
    root = math.isqrt(m)
    s = min(m, root + int(root * root != m))
    if s < 2:
        raise AssertionError("sample round has fewer than two candidates")
    radicand = 2 * s * n.bit_length()
    root = math.isqrt(radicand)
    a = root + int(root * root != radicand)
    mu_numerator = s * (r - 1)
    mu_floor = mu_numerator // m
    mu_ceil = (mu_numerator + m - 1) // m
    ql = min(s - 1, max(1, mu_floor - a))
    qh = min(s, max(ql + 1, mu_ceil + a + 1))
    return s, a, ql, qh


def _digest(edges: list[tuple[int, int]]) -> str:
    h = hashlib.sha256()
    for left, right in edges:
        h.update(struct.pack("<II", left, right))
    return h.hexdigest()


def _expected_edges(candidate: list[int], sample: list[int], x: int, y: int) -> list[tuple[int, int]]:
    sample_set = set(sample)
    edges = {tuple(sorted((sample[i], sample[j])))
             for i in range(len(sample)) for j in range(i + 1, len(sample))}
    for pivot in (x, y):
        for item in candidate:
            if item not in sample_set:
                edges.add(tuple(sorted((pivot, item))))
    return sorted(edges)


def _outcome(scores: list[int], left: int, right: int) -> bool:
    return _key(scores, left) < _key(scores, right)


def _rank(pivot: int, candidate: list[int], outcomes: dict[tuple[int, int], bool]) -> int:
    lower = 0
    for item in candidate:
        if item == pivot:
            continue
        edge = tuple(sorted((item, pivot)))
        item_before = outcomes[edge] if item == edge[0] else not outcomes[edge]
        lower += int(item_before)
    return lower + 1


def audit_trace(trace: dict[str, Any]) -> dict[str, Any]:
    if trace.get("algorithm_label") != LABEL or trace.get("algorithm_revision") != REVISION:
        raise AssertionError("trace label/revision mismatch")
    scores = trace["scores"]
    n = len(scores)
    k = trace["k"]
    if not n or not 1 <= k <= n or len(trace.get("rounds", [])) != 5:
        raise AssertionError("input or five-round trace shape mismatch")
    candidate = list(range(n))
    residual = k
    selected: int | None = None
    all_calls: list[tuple[int, int]] = []
    per_round: list[int] = []

    for round_no, record in enumerate(trace["rounds"], start=1):
        if record.get("round") != round_no:
            raise AssertionError("round numbering is not contiguous")
        if record.get("candidate_before") != candidate or record.get("rank_before") != residual:
            raise AssertionError(f"round {round_no}: candidate state is not causally derived")

        if selected is not None:
            if record.get("kind") != "TERMINAL_EMPTY" or record.get("frozen_edges") or record.get("outcomes"):
                raise AssertionError(f"round {round_no}: post-terminal layer is not empty")
            if (record.get("selected_index") != selected or record.get("candidate_after") != candidate
                    or record.get("rank_after") != residual or record.get("terminal") is not True):
                raise AssertionError(f"round {round_no}: terminal state changed")
            if record.get("edge_sha256") != _digest([]):
                raise AssertionError(f"round {round_no}: empty edge digest mismatch")
            per_round.append(0)
            continue

        m = len(candidate)
        if round_no < 5 and m == 1:
            selected = candidate[0]
            if record.get("kind") != "SINGLETON_EXACT" or record.get("frozen_edges") or record.get("outcomes"):
                raise AssertionError(f"round {round_no}: singleton termination malformed")
            if (record.get("selected_index") != selected or record.get("candidate_after") != candidate
                    or record.get("rank_after") != residual or record.get("terminal") is not True):
                raise AssertionError(f"round {round_no}: singleton result mismatch")
            if record.get("edge_sha256") != _digest([]):
                raise AssertionError(f"round {round_no}: singleton edge digest mismatch")
            per_round.append(0)
            continue

        if round_no == 5:
            if record.get("kind") != "EXACT_CANDIDATE_CLIQUE":
                raise AssertionError("fifth layer is not the exact candidate clique")
            expected = [(candidate[i], candidate[j]) for i in range(m) for j in range(i + 1, m)]
            expected = sorted(tuple(sorted(edge)) for edge in expected)
        else:
            if record.get("kind") != "SAMPLE_QUANTILE_BRACKET":
                raise AssertionError(f"round {round_no}: wrong algorithm layer")
            s, a, ql, qh = _params(m, residual, n)
            if (record.get("q_low"), record.get("q_high")) != (ql, qh):
                raise AssertionError(f"round {round_no}: sample order statistic parameters mismatch")
            if record.get("window_a") != a or record.get("mu_numerator") != s * (residual - 1) or record.get("mu_denominator") != m:
                raise AssertionError(f"round {round_no}: confidence window provenance mismatch")
            sampled = random.Random(_seed(trace["algorithm_seed"], round_no)).sample(candidate, s)
            sample_order = sorted(sampled, key=lambda item: _key(scores, item))
            if record.get("sample_ids") != sampled or record.get("sample_order") != sample_order:
                raise AssertionError(f"round {round_no}: sample provenance/order mismatch")
            x, y = sample_order[ql - 1], sample_order[qh - 1]
            if record.get("pivot_ids") != [x, y]:
                raise AssertionError(f"round {round_no}: pivot derivation mismatch")
            expected = _expected_edges(candidate, sampled, x, y)

        observed = [tuple(edge) for edge in record.get("frozen_edges", [])]
        if observed != expected:
            raise AssertionError(f"round {round_no}: frozen edge set mismatch")
        if len(observed) != len(set(observed)):
            raise AssertionError(f"round {round_no}: duplicate edge inside layer")
        if any(left not in candidate or right not in candidate or left >= right for left, right in observed):
            raise AssertionError(f"round {round_no}: invalid endpoint or noncanonical direction")
        if record.get("edge_sha256") != _digest(expected):
            raise AssertionError(f"round {round_no}: frozen edge digest mismatch")
        expected_outcomes = [[left, right, _outcome(scores, left, right)] for left, right in expected]
        if record.get("outcomes") != expected_outcomes:
            raise AssertionError(f"round {round_no}: outcome does not match frozen edge")
        outcomes = {(left, right): result for left, right, result in expected_outcomes}
        before = candidate.copy()

        if round_no < 5:
            x, y = record["pivot_ids"]
            px, py = _rank(x, candidate, outcomes), _rank(y, candidate, outcomes)
            if record.get("pivot_ranks") != [px, py] or not px < py:
                raise AssertionError(f"round {round_no}: pivot ranks are not certified by this layer")
            if px == residual:
                selected = x
            elif py == residual:
                selected = y
            elif px < residual < py:
                candidate = [item for item in candidate if _key(scores, x) < _key(scores, item) < _key(scores, y)]
                residual -= px
            elif py < residual:
                candidate = [item for item in candidate if _key(scores, y) < _key(scores, item)]
                residual -= py
            elif px > residual:
                candidate = [item for item in candidate if _key(scores, item) < _key(scores, x)]
            else:
                raise AssertionError(f"round {round_no}: transition does not preserve target rank")
            if selected is None and not 1 <= residual <= len(candidate):
                raise AssertionError(f"round {round_no}: residual rank outside retained candidates")
        else:
            ranks = {item: 1 + sum(
                _outcome_for_pair(outcomes, other, item)
                for other in candidate if other != item
            ) for item in candidate}
            target = [item for item, rank in ranks.items() if rank == residual]
            if len(target) != 1:
                raise AssertionError("fifth layer did not determine exactly one target")
            selected = target[0]

        if record.get("candidate_after") != candidate or record.get("rank_after") != residual:
            raise AssertionError(f"round {round_no}: state transition mismatch")
        if record.get("selected_index") != selected:
            raise AssertionError(f"round {round_no}: selected identity mismatch")
        if record.get("terminal") != (selected is not None):
            raise AssertionError(f"round {round_no}: terminal marker mismatch")
        all_calls.extend(expected)
        per_round.append(len(expected))

    if selected is None or selected != trace.get("selected_index"):
        raise AssertionError("final selected identity mismatch")
    key = _key(scores, selected)
    expected_mask = [int(_key(scores, index) <= key) for index in range(n)]
    if trace.get("mask") != expected_mask or sum(expected_mask) != k:
        raise AssertionError("stable Top-K mask mismatch")
    unique = set(all_calls)
    return {
        "status": "PASS",
        "rounds": 5,
        "causal_dependencies": "PASS",
        "frozen_edge_count_per_round": per_round,
        "logical_comparison_calls": len(all_calls),
        "unique_unordered_pairs": len(unique),
        "repeated_comparison_calls": len(all_calls) - len(unique),
        "selected_index": selected,
        "mask_ones": sum(expected_mask),
    }


def _outcome_for_pair(outcomes: dict[tuple[int, int], bool], left: int, right: int) -> bool:
    edge = tuple(sorted((left, right)))
    value = outcomes[edge]
    return value if left == edge[0] else not value


def main() -> int:
    import argparse

    parser = argparse.ArgumentParser(description="independent edge/causality audit")
    parser.add_argument("trace", type=Path)
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    result = audit_trace(json.loads(args.trace.read_text(encoding="utf-8")))
    payload = json.dumps(result, indent=2) + "\n"
    if args.out:
        args.out.write_text(payload, encoding="utf-8")
    else:
        print(payload, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
