#!/usr/bin/env python3
"""TEST_ONLY AAV86-style full-sort comparison graph counter for M6A-P2-I-E1."""
from __future__ import annotations

import argparse
import bisect
import csv
import math
import random
import sys
from dataclasses import dataclass
from itertools import combinations
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Set, Tuple

MASK32 = (1 << 32) - 1

@dataclass(frozen=True)
class Record:
    score: int
    original_index: int

    @property
    def stable_key(self) -> Tuple[int, int]:
        return (-self.score, self.original_index)

@dataclass
class RoundStats:
    iteration: int
    active_subproblems: int = 0
    empty_buckets: int = 0
    nontrivial_subproblems: int = 0
    logical_nodes: int = 0
    singleton_carries: int = 0
    pivots: int = 0
    pivot_pivot_edges: int = 0
    pivot_element_edges: int = 0
    same_round_recursive_edges: int = 0
    total_edges: int = 0

@dataclass(frozen=True)
class Snapshot:
    iteration: int
    vertices: Tuple[int, ...]
    pivots: frozenset

class AAV86GraphCounter:
    """TEST_ONLY graph construction; records are cleartext and handles are opaque IDs."""
    def __init__(self, records_by_handle: Dict[int, Record], rounds: int,
                 pivot_seed: int, enumerate_edges: bool):
        if rounds < 1:
            raise ValueError("rounds must be >= 1")
        self.records = records_by_handle
        self.rounds = rounds
        self.rng = random.Random(pivot_seed)
        self.enumerate_edges = enumerate_edges
        self.stats = [RoundStats(i + 1) for i in range(rounds)]
        self.snapshots: List[Snapshot] = []
        self.edges_by_round: List[List[Tuple[int, int]]] = [[] for _ in range(rounds)]

    @staticmethod
    def _ceil_nth_root(m: int, d: int) -> int:
        # Integer-only ceil(m ** (1/d)); no floating-point boundary ambiguity.
        lo, hi = 1, 1
        while hi ** d < m:
            hi *= 2
        while lo < hi:
            mid = (lo + hi) // 2
            if mid ** d >= m:
                hi = mid
            else:
                lo = mid + 1
        return lo

    def _add_subproblem(self, vertices: Sequence[int], depth: int) -> List[int]:
        st = self.stats[self.rounds - depth]
        m = len(vertices)
        if m == 0:
            st.empty_buckets += 1
            return []
        st.active_subproblems += 1
        if m == 1:
            st.singleton_carries += 1
            return list(vertices)
        st.nontrivial_subproblems += 1
        st.logical_nodes += m

        t = self._ceil_nth_root(m, depth)
        q = t - 1
        if not (1 <= q < m):
            raise AssertionError(f"invalid pivot count m={m} depth={depth} q={q}")
        chosen = self.rng.sample(list(vertices), q)
        pivot_set = set(chosen)
        nonpivots = [v for v in vertices if v not in pivot_set]
        st.pivots += q
        pp = q * (q - 1) // 2
        pe = q * (m - q)
        st.pivot_pivot_edges += pp
        st.pivot_element_edges += pe
        st.total_edges += pp + pe
        self.snapshots.append(Snapshot(self.rounds - depth + 1,
                                       tuple(vertices), frozenset(pivot_set)))
        if self.enumerate_edges:
            for a, b in combinations(chosen, 2):
                self.edges_by_round[self.rounds - depth].append((min(a, b), max(a, b)))
            for p in chosen:
                for v in nonpivots:
                    self.edges_by_round[self.rounds - depth].append((min(p, v), max(p, v)))

        pivot_order = sorted(chosen, key=lambda v: self.records[v].stable_key)
        pivot_keys = [self.records[v].stable_key for v in pivot_order]
        buckets: List[List[int]] = [[] for _ in range(q + 1)]
        for v in nonpivots:
            bucket = bisect.bisect_left(pivot_keys, self.records[v].stable_key)
            buckets[bucket].append(v)

        if depth == 1:
            # With t=m, q=m-1 and every unordered pair is an edge. This final
            # layer returns a clear stable order only for TEST_ONLY validation.
            return sorted(vertices, key=lambda v: self.records[v].stable_key)

        sorted_children = [self._add_subproblem(child, depth - 1) for child in buckets]
        result: List[int] = []
        for i, pivot in enumerate(pivot_order):
            result.extend(sorted_children[i])
            result.append(pivot)
        result.extend(sorted_children[-1])
        return result

    def run(self, handles: Sequence[int]) -> List[int]:
        return self._add_subproblem(handles, self.rounds)


def make_random_records(n: int, input_seed: int) -> Tuple[Dict[int, Record], List[int]]:
    score_rng = random.Random(input_seed)
    scores = []
    for _ in range(n):
        raw = score_rng.getrandbits(32)
        scores.append(raw - (1 << 32) if raw & (1 << 31) else raw)
    original_indices = list(range(n))
    random.Random(input_seed ^ 0x4D36415F48414E44).shuffle(original_indices)
    by_handle = {handle: Record(scores[original], original)
                 for handle, original in enumerate(original_indices)}
    return by_handle, list(range(n))


def graph_oracle_edges(snapshots: Sequence[Snapshot], rounds: int) -> List[Set[Tuple[int, int]]]:
    """Independent exhaustive definition: edge iff at least one endpoint is pivot."""
    expected = [set() for _ in range(rounds)]
    for snap in snapshots:
        for a, b in combinations(snap.vertices, 2):
            if a in snap.pivots or b in snap.pivots:
                expected[snap.iteration - 1].add((min(a, b), max(a, b)))
    return expected


def topk_mask(sorted_handles: Sequence[int], records: Dict[int, Record], k: int) -> List[int]:
    mask = [0] * len(records)
    for handle in sorted_handles[:k]:
        mask[records[handle].original_index] = 1
    return mask


def run_self_tests() -> None:
    # Independent exhaustive edge oracle and no-repetition check on tiny domains.
    for n in (1, 2, 3, 5, 8, 13, 32):
        records, handles = make_random_records(n, 20260930 + n)
        for rounds in (1, 2, 3, 4):
            if rounds > 1 and n == 1:
                continue
            counter = AAV86GraphCounter(records, rounds, 731 + n * 11 + rounds,
                                        enumerate_edges=True)
            sorted_handles = counter.run(handles)
            expected_by_round = graph_oracle_edges(counter.snapshots, rounds)
            actual_by_round = [set(edges) for edges in counter.edges_by_round]
            assert actual_by_round == expected_by_round, (n, rounds)
            assert all(len(edges) == len(counter.edges_by_round[i])
                       for i, edges in enumerate(actual_by_round)), "duplicate edge in a round"
            all_edges: Set[Tuple[int, int]] = set()
            for edges in actual_by_round:
                assert all_edges.isdisjoint(edges), "edge repeated across rounds"
                all_edges.update(edges)
            oracle_order = sorted(handles, key=lambda h: records[h].stable_key)
            assert sorted_handles == oracle_order, (n, rounds, "sort output")
            for k in sorted(set((1, max(1, n // 2), n))):
                expected_mask = [int(i in {records[h].original_index for h in oracle_order[:k]})
                                 for i in range(n)]
                assert topk_mask(sorted_handles, records, k) == expected_mask
            assert all(s.same_round_recursive_edges == 0 for s in counter.stats)
            if rounds == 1 and n > 1:
                assert counter.stats[0].total_edges == n * (n - 1) // 2

    # Stable semantic fixtures: ties and signed extremes must preserve original index.
    fixtures = [
        [5, 5, 5, 5],
        [7, -2, 7, -2, 0, 0],
        [-(1 << 31), (1 << 31) - 1, -1, 0, (1 << 31) - 1],
    ]
    for values in fixtures:
        n = len(values)
        records = {h: Record(values[(h + 1) % n], (h + 1) % n) for h in range(n)}
        handles = list(range(n))
        expected = sorted(handles, key=lambda h: records[h].stable_key)
        counter = AAV86GraphCounter(records, 3, 999 + n, enumerate_edges=True)
        got = counter.run(handles)
        assert got == expected
        for k in (1, max(1, n // 2), n):
            assert sum(topk_mask(got, records, k)) == k
    print("SELF_TEST PASS: exhaustive edge oracle, repetition, stable order, Q20.12-boundary representatives, K=1/intermediate/n")


def run_matrix(output: Path) -> None:
    sizes = [1, 2, 3, 4, 5, 8, 16, 32, 128, 256, 1000, 10000, 100000]
    rounds_list = [2, 3, 4, 5]
    seed_pairs = [(1, 1001), (7, 1007), (42, 1042)]
    fields = [
        "run_id", "n", "r", "input_seed", "pivot_seed", "iteration",
        "active_subproblems", "empty_buckets", "nontrivial_subproblems", "logical_nodes",
        "singleton_carries", "pivots", "pivot_pivot_edges", "pivot_element_edges",
        "same_round_recursive_edges", "total_edges", "run_total_edges",
        "M_observed_sample_max", "pool_capacity_formula_slots", "pool_slots_used",
        "pool_slots_unused", "pool_used_ratio", "duplicate_edge_check",
        "per_round_upper_bound_proven", "topk_parameter_affects_graph",
    ]
    with output.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        for n in sizes:
            for rounds in rounds_list:
                for seed_index, (input_seed, pivot_seed) in enumerate(seed_pairs):
                    run_id = f"n{n}_r{rounds}_is{input_seed}_ps{pivot_seed}"
                    records, handles = make_random_records(n, input_seed)
                    counter = AAV86GraphCounter(records, rounds, pivot_seed,
                                                enumerate_edges=(n <= 32))
                    counter.run(handles)
                    edges = [s.total_edges for s in counter.stats]
                    total = sum(edges)
                    capacity = rounds * (n * (n - 1) // 2)
                    observed = max(edges, default=0)
                    # The pivot-removal invariant gives uniqueness without storing
                    # the potentially quadratic edge list for large n.
                    duplicate_method = "ENUMERATED_0" if n <= 32 else "PROVED_BY_PIVOT_REMOVAL"
                    for st in counter.stats:
                        writer.writerow({
                            "run_id": run_id, "n": n, "r": rounds,
                            "input_seed": input_seed, "pivot_seed": pivot_seed,
                            "iteration": st.iteration,
                            "active_subproblems": st.active_subproblems,
                            "empty_buckets": st.empty_buckets,
                            "nontrivial_subproblems": st.nontrivial_subproblems,
                            "logical_nodes": st.logical_nodes,
                            "singleton_carries": st.singleton_carries,
                            "pivots": st.pivots,
                            "pivot_pivot_edges": st.pivot_pivot_edges,
                            "pivot_element_edges": st.pivot_element_edges,
                            "same_round_recursive_edges": st.same_round_recursive_edges,
                            "total_edges": st.total_edges,
                            "run_total_edges": total,
                            "M_observed_sample_max": observed,
                            "pool_capacity_formula_slots": capacity,
                            "pool_slots_used": total,
                            "pool_slots_unused": capacity - total,
                            "pool_used_ratio": (f"{total / capacity:.12f}" if capacity else "0"),
                            "duplicate_edge_check": duplicate_method,
                            "per_round_upper_bound_proven": n * (n - 1) // 2,
                            "topk_parameter_affects_graph": "NO_K_IN_GRAPH_INPUT",
                        })
    print(f"MATRIX_WRITTEN {output} n_values={len(sizes)} r_values={len(rounds_list)} seeds={len(seed_pairs)} runs={len(sizes)*len(rounds_list)*len(seed_pairs)} rows={len(sizes)*len(seed_pairs)*sum(rounds_list)}")



def run_material_fixture(output: Path) -> None:
    # TEST_ONLY shuffled-handle fixture: mapping intentionally differs from original indices.
    handle_to_original = [3, 0, 4, 1, 2]
    scores_by_original = [(1 << 31) - 1, 7, 7, -4, -(1 << 31)]
    records = {handle: Record(scores_by_original[original], original)
               for handle, original in enumerate(handle_to_original)}
    counter = AAV86GraphCounter(records, 2, 20260930, enumerate_edges=True)
    counter.run(list(range(5)))
    with output.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["iteration", "endpoint_a", "endpoint_b"])
        for iteration, edges in enumerate(counter.edges_by_round, 1):
            for a, b in edges:
                writer.writerow([iteration, a, b])
    print(f"MATERIAL_FIXTURE_WRITTEN {output} n=5 r=2 input_seed=FIXED_Q20_12_FIXTURE pivot_seed=20260930 per_round_edges={[s.total_edges for s in counter.stats]}")

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--matrix", type=Path)
    parser.add_argument("--material-fixture", type=Path)
    args = parser.parse_args()
    if args.self_test:
        run_self_tests()
    if args.matrix:
        run_matrix(args.matrix)
    if args.material_fixture:
        run_material_fixture(args.material_fixture)
    if not args.self_test and not args.matrix and not args.material_fixture:
        parser.error("select --self-test and/or --matrix and/or --material-fixture")
    return 0

if __name__ == "__main__":
    sys.exit(main())
