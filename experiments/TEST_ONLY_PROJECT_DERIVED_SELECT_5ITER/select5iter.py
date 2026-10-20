#!/usr/bin/env python3
"""Project-derived five-iteration stable Select; plaintext TEST_ONLY only."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import random
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Any


ALGORITHM_LABEL = "PROJECT_DERIVED_SELECT_5ITER_TEST_ONLY"
ALGORITHM_REVISION = "v1"
INT32_MIN = -(1 << 31)
INT32_MAX = (1 << 31) - 1
MAX_N = 1_000_000


def stable_key(scores: list[int], item: int) -> tuple[int, int]:
    """Ascending key is descending signed score, then ascending input index."""
    return (-scores[item], item)


def _validate(scores: list[int], k: int) -> None:
    if not isinstance(scores, list) or not 1 <= len(scores) <= MAX_N:
        raise ValueError("n must satisfy 1 <= n <= 1,000,000")
    if not isinstance(k, int) or isinstance(k, bool) or not 1 <= k <= len(scores):
        raise ValueError("K must satisfy 1 <= K <= n")
    for score in scores:
        if not isinstance(score, int) or isinstance(score, bool) or not INT32_MIN <= score <= INT32_MAX:
            raise ValueError("scores must be raw signed Q20.12 int32 integers")


def _derive_seed(master_seed: int, round_no: int) -> int:
    payload = f"{ALGORITHM_LABEL}:{ALGORITHM_REVISION}:{master_seed}:{round_no}".encode("ascii")
    return int.from_bytes(hashlib.sha256(payload).digest()[:8], "big")


def _sample_parameters(m: int, r: int, n: int) -> tuple[int, int, int, int, int, int]:
    """Return (s, a, q_low, mu, q_high) using the frozen integer rules."""
    s = min(m, math.isqrt(m) + (math.isqrt(m) ** 2 != m))
    if s < 2:
        raise ValueError("sample parameters require at least two candidates")
    radicand = 2 * s * n.bit_length()
    a = math.isqrt(radicand) + (math.isqrt(radicand) ** 2 != radicand)
    mu_numerator = s * (r - 1)
    mu_floor = mu_numerator // m
    mu_ceil = (mu_numerator + m - 1) // m
    q_low = min(s - 1, max(1, mu_floor - a))
    q_high = min(s, max(q_low + 1, mu_ceil + a + 1))
    return s, a, q_low, mu_numerator, m, q_high


def _edge_digest(edges: list[tuple[int, int]]) -> str:
    digest = hashlib.sha256()
    for left, right in edges:
        digest.update(struct.pack("<II", left, right))
    return digest.hexdigest()


def _planned_edges(candidate: list[int], sample: list[int], pivot_x: int, pivot_y: int) -> list[tuple[int, int]]:
    sample_set = set(sample)
    edges = {tuple(sorted((sample[i], sample[j]))) for i in range(len(sample)) for j in range(i + 1, len(sample))}
    for pivot in (pivot_x, pivot_y):
        for item in candidate:
            if item not in sample_set:
                edges.add(tuple(sorted((pivot, item))))
    return sorted(edges)


def _outcome(scores: list[int], left: int, right: int) -> bool:
    """Outcome for canonically oriented edge left < right by input identity."""
    return stable_key(scores, left) < stable_key(scores, right)


def _pivot_rank(pivot: int, candidate: list[int], outcomes: dict[tuple[int, int], bool]) -> int:
    lower = 0
    for item in candidate:
        if item == pivot:
            continue
        edge = tuple(sorted((item, pivot)))
        item_before_pivot = outcomes[edge] if edge[0] == item else not outcomes[edge]
        lower += int(item_before_pivot)
    return lower + 1


@dataclass(frozen=True)
class SelectResult:
    selected_index: int
    mask: list[int]
    logical_comparisons: int
    unique_unordered_pairs: int
    repeated_comparison_calls: int
    per_round_comparisons: list[int]
    trace: dict[str, Any] | None


def select_kth(
    scores: list[int],
    k: int,
    *,
    input_seed: int = 0,
    algorithm_seed: int = 0,
    with_trace: bool = False,
) -> SelectResult:
    """Return exact stable Kth item/mask with five planned comparison layers.

    Four sample/pivot layers reduce a safe rank interval. The fifth complete
    candidate graph closes selection exactly. A found target makes subsequent
    planned layers explicit empty terminal layers.
    """
    _validate(scores, k)
    n = len(scores)
    candidate = list(range(n))
    residual_rank = k
    terminal_index: int | None = None
    all_edges: set[tuple[int, int]] = set()
    logical_calls = 0
    per_round: list[int] = []
    rounds: list[dict[str, Any]] = []

    for round_no in range(1, 6):
        if terminal_index is not None:
            rounds.append({
                "round": round_no,
                "kind": "TERMINAL_EMPTY",
                "candidate_before": candidate.copy(),
                "rank_before": residual_rank,
                "sample_ids": [],
                "sample_order": [],
                "q_low": None,
                "q_high": None,
                "pivot_ids": [],
                "frozen_edges": [],
                "edge_sha256": _edge_digest([]),
                "outcomes": [],
                "candidate_after": candidate.copy(),
                "rank_after": residual_rank,
                "selected_index": terminal_index,
                "terminal": True,
            })
            per_round.append(0)
            continue

        m = len(candidate)
        if round_no == 5:
            edges = [(candidate[i], candidate[j]) for i in range(m) for j in range(i + 1, m)]
            edge_plan = [tuple(sorted(edge)) for edge in edges]
            kind = "EXACT_CANDIDATE_CLIQUE"
            sample_ids: list[int] = []
            sample_order: list[int] = []
            q_low = q_high = None
            pivot_ids: list[int] = []
        elif m == 1:
            terminal_index = candidate[0]
            rounds.append({
                "round": round_no,
                "kind": "SINGLETON_EXACT",
                "candidate_before": candidate.copy(),
                "rank_before": residual_rank,
                "sample_ids": [],
                "sample_order": [],
                "q_low": None,
                "q_high": None,
                "pivot_ids": [],
                "frozen_edges": [],
                "edge_sha256": _edge_digest([]),
                "outcomes": [],
                "candidate_after": candidate.copy(),
                "rank_after": residual_rank,
                "selected_index": terminal_index,
                "terminal": True,
            })
            per_round.append(0)
            continue
        else:
            s, a, q_low, mu_numerator, mu_denominator, q_high = _sample_parameters(m, residual_rank, n)
            sample_ids = random.Random(_derive_seed(algorithm_seed, round_no)).sample(candidate, s)
            sample_order = sorted(sample_ids, key=lambda item: stable_key(scores, item))
            pivot_x, pivot_y = sample_order[q_low - 1], sample_order[q_high - 1]
            edge_plan = _planned_edges(candidate, sample_ids, pivot_x, pivot_y)
            kind = "SAMPLE_QUANTILE_BRACKET"
            pivot_ids = [pivot_x, pivot_y]

        # Freeze and record the entire edge set before evaluating any outcome.
        edge_plan = sorted(edge_plan)
        outcome_rows = [[left, right, _outcome(scores, left, right)] for left, right in edge_plan]
        outcomes = {(left, right): outcome for left, right, outcome in outcome_rows}
        before_candidate = candidate.copy()
        before_rank = residual_rank
        round_record: dict[str, Any] = {
            "round": round_no,
            "kind": kind,
            "candidate_before": before_candidate,
            "rank_before": before_rank,
            "sample_ids": sample_ids,
            "sample_order": sample_order,
            "q_low": q_low,
            "q_high": q_high,
            "window_a": a if round_no < 5 else None,
            "mu_numerator": mu_numerator if round_no < 5 else None,
            "mu_denominator": mu_denominator if round_no < 5 else None,
            "pivot_ids": pivot_ids,
            "frozen_edges": [list(edge) for edge in edge_plan],
            "edge_sha256": _edge_digest(edge_plan),
            "outcomes": outcome_rows,
            "candidate_after": [],
            "rank_after": None,
            "selected_index": None,
            "terminal": False,
        }

        if round_no < 5:
            pivot_x, pivot_y = pivot_ids
            rank_x = _pivot_rank(pivot_x, candidate, outcomes)
            rank_y = _pivot_rank(pivot_y, candidate, outcomes)
            if not rank_x < rank_y:
                raise RuntimeError("internal invariant: ordered pivots have non-increasing ranks")
            round_record["pivot_ranks"] = [rank_x, rank_y]
            if rank_x == residual_rank:
                terminal_index = pivot_x
            elif rank_y == residual_rank:
                terminal_index = pivot_y
            elif rank_x < residual_rank < rank_y:
                candidate = [item for item in candidate
                             if stable_key(scores, pivot_x) < stable_key(scores, item)
                             and stable_key(scores, item) < stable_key(scores, pivot_y)]
                residual_rank -= rank_x
            elif rank_y < residual_rank:
                candidate = [item for item in candidate
                             if stable_key(scores, pivot_y) < stable_key(scores, item)]
                residual_rank -= rank_y
            elif rank_x > residual_rank:
                candidate = [item for item in candidate
                             if stable_key(scores, item) < stable_key(scores, pivot_x)]
            else:
                raise RuntimeError("internal invariant: pivot ranks do not bracket target state")
            if terminal_index is None and not 1 <= residual_rank <= len(candidate):
                raise RuntimeError("internal invariant: residual rank is outside candidate set")
        else:
            ranks = {
                item: 1 + sum(
                    _outcome_for_pair(outcomes, other, item)
                    for other in candidate if other != item
                )
                for item in candidate
            }
            if len(set(ranks.values())) != len(candidate):
                raise RuntimeError("internal invariant: exact candidate ranks are not unique")
            matches = [item for item, rank in ranks.items() if rank == residual_rank]
            if len(matches) != 1:
                raise RuntimeError("internal invariant: exact target rank is absent or ambiguous")
            terminal_index = matches[0]

        round_record["candidate_after"] = candidate.copy()
        round_record["rank_after"] = residual_rank
        round_record["selected_index"] = terminal_index
        round_record["terminal"] = terminal_index is not None
        rounds.append(round_record)
        per_round.append(len(edge_plan))
        logical_calls += len(edge_plan)
        all_edges.update(edge_plan)

    if terminal_index is None:
        raise RuntimeError("internal invariant: five planned layers did not produce an answer")
    selected_key = stable_key(scores, terminal_index)
    mask = [int(stable_key(scores, index) <= selected_key) for index in range(n)]
    if sum(mask) != k:
        raise RuntimeError("internal invariant: output mask does not contain exactly K items")
    trace = None
    if with_trace:
        trace = {
            "algorithm_label": ALGORITHM_LABEL,
            "algorithm_revision": ALGORITHM_REVISION,
            "input_seed": input_seed,
            "algorithm_seed": algorithm_seed,
            "k": k,
            "scores": scores.copy(),
            "selected_index": terminal_index,
            "mask": mask,
            "rounds": rounds,
        }
    return SelectResult(
        selected_index=terminal_index,
        mask=mask,
        logical_comparisons=logical_calls,
        unique_unordered_pairs=len(all_edges),
        repeated_comparison_calls=logical_calls - len(all_edges),
        per_round_comparisons=per_round,
        trace=trace,
    )


def _outcome_for_pair(outcomes: dict[tuple[int, int], bool], left: int, right: int) -> bool:
    """Return whether left precedes right for a canonically oriented edge."""
    edge = tuple(sorted((left, right)))
    result = outcomes[edge]
    return result if left == edge[0] else not result


def main() -> int:
    parser = argparse.ArgumentParser(description=ALGORITHM_LABEL)
    parser.add_argument("--scores-json", required=True, type=Path)
    parser.add_argument("--k", required=True, type=int)
    parser.add_argument("--input-seed", type=int, default=0)
    parser.add_argument("--algorithm-seed", type=int, default=0)
    parser.add_argument("--trace", type=Path)
    args = parser.parse_args()
    scores = json.loads(args.scores_json.read_text(encoding="utf-8"))
    result = select_kth(scores, args.k, input_seed=args.input_seed,
                        algorithm_seed=args.algorithm_seed, with_trace=args.trace is not None)
    if args.trace is not None:
        args.trace.write_text(json.dumps(result.trace, separators=(",", ":")) + "\n", encoding="utf-8")
    print(json.dumps({
        "algorithm_label": ALGORITHM_LABEL,
        "algorithm_revision": ALGORITHM_REVISION,
        "status": "SUCCESS",
        "selected_index": result.selected_index,
        "mask": result.mask,
        "logical_comparisons": result.logical_comparisons,
        "unique_unordered_pairs": result.unique_unordered_pairs,
        "repeated_comparison_calls": result.repeated_comparison_calls,
        "per_round_comparisons": result.per_round_comparisons,
    }, separators=(",", ":")))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
