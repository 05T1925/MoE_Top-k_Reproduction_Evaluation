#!/usr/bin/env python3
"""TEST_ONLY semantic fixture for the proposed non-adaptive fixed-graph route.

This checks only pairwise-rank arithmetic and stable Top-K semantics. It is not
an MPC implementation, a DCF test, or a security argument.
"""

from __future__ import annotations

import itertools
import random

MASK64 = (1 << 64) - 1
RNG = random.Random(0x53420C)


def stable_key(score: int, original_index: int) -> tuple[int, int]:
    if not -(1 << 31) <= score < (1 << 31):
        raise ValueError("score is outside signed Q20.12 storage")
    return (-score, original_index)


def fixed_graph_top_k(scores_by_handle: list[int], original_by_handle: list[int], k: int) -> list[int]:
    n = len(scores_by_handle)
    if not (1 <= k <= n) or len(original_by_handle) != n:
        raise ValueError("invalid fixture input")

    # Schedule is determined from public n before any result exists. Each task
    # gets the same real handle universe but disjoint slot IDs/material domains.
    edges = [(left, right) for left in range(n) for right in range(left + 1, n)]
    task_slots = [[(task, left, right) for left, right in edges] for task in (0, 1)]
    assert len(task_slots[0]) == len(task_slots[1]) == n * (n - 1) // 2
    assert set((u, v) for _, u, v in task_slots[0]) == set((u, v) for _, u, v in task_slots[1])
    assert not (set(task_slots[0]) & set(task_slots[1]))

    rank_share_0 = [0] * n
    rank_share_1 = [0] * n
    for left, right in edges:
        left_key = stable_key(scores_by_handle[left], original_by_handle[left])
        right_key = stable_key(scores_by_handle[right], original_by_handle[right])
        # The clear comparison exists only in this TEST_ONLY fixture. Model its
        # additive shares and aggregate locally; the fixture opens only ranks.
        left_precedes = int(left_key < right_key)
        right_precedes = 1 - left_precedes
        for handle, bit in ((left, right_precedes), (right, left_precedes)):
            share = RNG.getrandbits(64)
            rank_share_0[handle] = (rank_share_0[handle] + share) & MASK64
            rank_share_1[handle] = (rank_share_1[handle] + bit - share) & MASK64

    ranks = [(a + b) & MASK64 for a, b in zip(rank_share_0, rank_share_1)]
    mask_by_handle = [int(rank < k) for rank in ranks]
    oracle_handles = sorted(
        range(n), key=lambda h: stable_key(scores_by_handle[h], original_by_handle[h])
    )[:k]
    oracle_mask = [int(handle in oracle_handles) for handle in range(n)]
    assert mask_by_handle == oracle_mask
    assert sum(mask_by_handle) == k
    return mask_by_handle


def main() -> None:
    cases = 0
    # Exhaust every handle permutation and K for several strict total orders.
    for n in range(1, 7):
        base_scores = [17 * i - 41 for i in range(n)]
        originals = list(range(n))
        for permutation in itertools.permutations(range(n)):
            scores = [base_scores[i] for i in permutation]
            indices = [originals[i] for i in permutation]
            for k in range(1, n + 1):
                fixed_graph_top_k(scores, indices, k)
                cases += 1

    # Stable tie and signed-boundary fixture, all K values, n=8.
    boundary_scores = [-(1 << 31), (1 << 31) - 1, 7, 7, 0, -(1 << 31), 0, (1 << 31) - 1]
    original_indices = [41, 3, 19, 7, 0, 2, 11, 29]
    # Keep n=8 bounded: use every rotation and its reverse rather than all 8!.
    for shift in range(8):
        order = list(range(8))[shift:] + list(range(8))[:shift]
        for handles in (order, list(reversed(order))):
            fixed_graph_top_k([boundary_scores[i] for i in handles],
                              [original_indices[i] for i in handles], 1 + shift % 8)
            cases += 1

    print(
        "FIXED_GRAPH_RANK_FIXTURE TEST_ONLY result=PASS "
        f"cases={cases} exhaustive_n_le_6=True n8_boundary_rotations=16 "
        "all_pairs_nonadaptive=True additive_rank_shares=True security=NOT_TESTED"
    )


if __name__ == "__main__":
    main()
