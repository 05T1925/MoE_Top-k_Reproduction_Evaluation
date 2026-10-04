#!/usr/bin/env python3
"""TEST_ONLY public CA control. No record values or comparison bits enter this module.

Handles are shuffled slot identifiers. The caller supplies already opened local
ranks for the current graph; this module chooses pivots and derives public state.
"""
from __future__ import annotations

import itertools
import random
from dataclasses import dataclass
from typing import Mapping, Sequence


def ceil_root(m: int, depth: int) -> int:
    if m < 1 or depth < 1:
        raise ValueError("invalid CA size/depth")
    lo, hi = 1, 1
    while hi ** depth < m:
        hi *= 2
    while lo < hi:
        mid = (lo + hi) // 2
        if mid ** depth >= m:
            hi = mid
        else:
            lo = mid + 1
    return lo


def choose_pivots(handles: Sequence[int], depth: int, coins: random.Random) -> list[int]:
    if len(handles) < 2 or len(set(handles)) != len(handles):
        raise ValueError("graph needs distinct handles")
    q = ceil_root(len(handles), depth) - 1
    if not 1 <= q < len(handles):
        raise ValueError("invalid pivot count")
    return coins.sample(list(handles), q)


@dataclass(frozen=True)
class ControlStep:
    pivot_order: tuple[int, ...]
    buckets: tuple[tuple[int, ...], ...]
    edges: tuple[tuple[int, int], ...]
    child_states: tuple[tuple[int, ...], ...]


def control_step(handles: Sequence[int], pivots: Sequence[int],
                 local_ranks: Mapping[int, int], depth: int) -> ControlStep:
    """Derive exactly the state exposed by a CA local-rank opening.

    This is a control-flow function, not a rank correctness or privacy proof.
    """
    vertices = tuple(handles)
    m = len(vertices)
    if m < 2 or depth < 1 or len(set(vertices)) != m:
        raise ValueError("invalid graph state")
    ps = set(pivots)
    q = ceil_root(m, depth) - 1
    if len(ps) != q or len(pivots) != q or not ps <= set(vertices):
        raise ValueError("invalid public pivots")
    if set(local_ranks) != set(vertices):
        raise ValueError("rank domain mismatch")
    if any(type(v) is not int or v < 0 or v >= m for v in local_ranks.values()):
        raise ValueError("rank outside subproblem")
    porder = tuple(sorted(pivots, key=lambda h: local_ranks[h]))
    if len({local_ranks[p] for p in pivots}) != q:
        raise ValueError("pivot ranks must be distinct")
    buckets: list[list[int]] = [[] for _ in range(q + 1)]
    for handle in vertices:
        if handle not in ps:
            bucket = local_ranks[handle]
            if bucket > q:
                raise ValueError("nonpivot bucket rank outside range")
            buckets[bucket].append(handle)
    edges = {tuple(sorted((a, b))) for a, b in itertools.combinations(pivots, 2)}
    edges.update(tuple(sorted((p, v))) for p in pivots for v in vertices if v not in ps)
    frozen = tuple(tuple(bucket) for bucket in buckets)
    return ControlStep(porder, frozen, tuple(sorted(edges)),
                       tuple(bucket for bucket in frozen if bucket and depth > 1))


def compose(step: ControlStep, ordered_buckets: Sequence[Sequence[int]]) -> list[int]:
    if len(ordered_buckets) != len(step.buckets):
        raise ValueError("bucket count mismatch")
    result: list[int] = []
    for i, pivot in enumerate(step.pivot_order):
        if (len(ordered_buckets[i]) != len(step.buckets[i]) or
                set(ordered_buckets[i]) != set(step.buckets[i])):
            raise ValueError("bucket membership mismatch")
        result.extend(ordered_buckets[i])
        result.append(pivot)
    if (len(ordered_buckets[-1]) != len(step.buckets[-1]) or
            set(ordered_buckets[-1]) != set(step.buckets[-1])):
        raise ValueError("last bucket membership mismatch")
    result.extend(ordered_buckets[-1])
    return result
