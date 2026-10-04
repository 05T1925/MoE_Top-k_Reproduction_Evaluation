#!/usr/bin/env python3
"""TEST_ONLY differential driver: E5 supplies ranks, E6 only controls CA state."""
from __future__ import annotations

import argparse
import hashlib
import itertools
import json
import random
import subprocess
from pathlib import Path

import aav86_ca_reference_TEST_ONLY as oracle
from aav86_ca_control_E6_TEST_ONLY import choose_pivots, compose, control_step


def check_case(records, rounds: int, seed: int) -> int:
    expected, ctx = oracle.run_reference(records, rounds, seed, True, True)
    trace = iter(ctx.trace)
    coins = random.Random(seed)
    graph_nodes = 0

    def replay(handles, depth, path="root", parent=""):
        nonlocal graph_nodes
        node = next(trace)
        vertices = list(handles)
        assert node["vertices"] == vertices
        assert node["remaining_depth"] == depth
        assert node["subproblem_id"] == path and node["parent_id"] == parent
        if len(vertices) == 1:
            assert node["state"] == "SINGLETON_NOOP"
            return vertices
        chosen = choose_pivots(vertices, depth, coins)
        assert chosen == node["pivots"]
        # Only the verifier/oracle has records. The control module receives
        # shuffled handles, public pivots, and opened local ranks alone.
        ranks = {int(k): v for k, v in node["local_ranks"].items()}
        step = control_step(vertices, chosen, ranks, depth)
        assert list(step.pivot_order) == node["pivot_order"]
        assert [list(b) for b in step.buckets] == node["buckets"]
        assert [list(e) for e in step.edges] == node["edges"]
        expected_children = [tuple(b) for b in node["buckets"] if b and depth > 1]
        assert list(step.child_states) == expected_children
        graph_nodes += 1
        if depth == 1:
            ordered = [list(b) for b in step.buckets]
        else:
            ordered = [replay(b, depth - 1, f"{path}/{i}", path) if b else []
                       for i, b in enumerate(step.buckets)]
        return compose(step, ordered)

    actual = replay(list(records), rounds)
    try:
        next(trace)
        raise AssertionError("unused oracle trace node")
    except StopIteration:
        pass
    assert actual == expected
    assert actual == sorted(records, key=lambda h: records[h].priority)
    for k in {1, max(1, len(records) // 2), len(records)}:
        assert oracle.topk_mask(actual, records, k) == oracle.topk_mask(expected, records, k)
    return graph_nodes


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    totals = {"exhaustive_cases": 0, "random_cases": 0,
              "tie_boundary_cases": 0, "n5_fixture_cases": 0,
              "handle_relabel_cases": 0,
              "padded_sentinel_cases": 0,
              "graph_nodes_checked": 0}
    for n in range(1, 6):
        for perm in itertools.permutations(range(n)):
            records = {h: oracle.Record(-perm[h], h) for h in range(n)}
            for rounds in range(1, 5):
                for seed in (17 + n, 9001 + n):
                    totals["graph_nodes_checked"] += check_case(records, rounds, seed)
                    totals["exhaustive_cases"] += 1
    for n in (2, 3, 5, 8, 13, 32):
        for input_seed in (1, 7, 42, 20260930):
            records, _ = oracle.make_random_records(n, input_seed)
            for rounds in (1, 2, 3, 4, 5):
                seed = 50000 + 101 * n + input_seed * 7 + rounds
                totals["graph_nodes_checked"] += check_case(records, rounds, seed)
                totals["random_cases"] += 1
    fixtures = [[5, 5, 5, 5], [7, -2, 7, -2, 0, 0],
                [-(1 << 31), (1 << 31) - 1, -1, 0, (1 << 31) - 1]]
    for values in fixtures:
        mapping = [(i + 1) % len(values) for i in range(len(values))]
        records = {h: oracle.Record(values[o], o) for h, o in enumerate(mapping)}
        for rounds in (1, 2, 3, 4):
            totals["graph_nodes_checked"] += check_case(records, rounds, 771 + len(values))
            totals["tie_boundary_cases"] += 1
    fixture_mapping = [3, 0, 4, 1, 2]
    fixture_scores = [(1 << 31) - 1, 7, 7, -4, -(1 << 31)]
    records = {h: oracle.Record(fixture_scores[o], o)
               for h, o in enumerate(fixture_mapping)}
    _, fixture_ctx = oracle.run_reference(records, 2, 20260930, True, True)
    assert [stage.active_edges for stage in fixture_ctx.stats] == [7, 1]
    totals["graph_nodes_checked"] += check_case(records, 2, 20260930)
    totals["n5_fixture_cases"] = 1
    relabel = {h: slot for h, slot in zip(records, (3, 0, 4, 1, 2))}
    renamed = {relabel[h]: value for h, value in records.items()}
    totals["graph_nodes_checked"] += check_case(renamed, 2, 20260930)
    totals["handle_relabel_cases"] = 1
    for logical_n in (1, 3, 5, 8):
        domain = oracle.padded_n(logical_n)
        for score in (-(1 << 31), (1 << 31) - 1):
            records = {h: oracle.Record(score, h) for h in range(logical_n)}
            records.update({h: oracle.Record(-(1 << 31), h)
                            for h in range(logical_n, domain)})
            totals["graph_nodes_checked"] += check_case(records, 2, 40000 + logical_n)
            got, _ = oracle.run_reference(records, 2, 40000 + logical_n)
            assert all(records[h].original_index < logical_n for h in got[:logical_n])
            assert sum(oracle.topk_mask(got, records, logical_n)[:logical_n]) == logical_n
            totals["padded_sentinel_cases"] += 1
    source_dir = Path(__file__).resolve().parent
    root = source_dir.parents[2]
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root,
                                       text=True).strip()
    result = {"label": "E6_TEST_ONLY_CONTROL_DIFFERENTIAL", "status": "PASS",
              "revision": revision,
              "oracle_sha256": hashlib.sha256((source_dir / "aav86_ca_reference_TEST_ONLY.py").read_bytes()).hexdigest(),
              "controller_sha256": hashlib.sha256((source_dir / "aav86_ca_control_E6_TEST_ONLY.py").read_bytes()).hexdigest(),
              "command": "python experiments/m6a_p2_i_allpairs/TEST_ONLY/verify_aav86_ca_control_E6_TEST_ONLY.py --out experiments/m6a_p2_i_allpairs/TEST_ONLY/aav86_ca_control_verification_E6_TEST_ONLY.json",
              "seed_contract": "E5 exhaustive 17+n/9001+n; random 50000+101*n+input_seed*7+r; tie 771+len(values); fixture 20260930; padded 40000+n",
              **totals,
              "scope": "E5 oracle trace supplies opened ranks; no secure protocol tested"}
    args.out.write_bytes((json.dumps(result, ensure_ascii=False, indent=2) + "\n").encode("utf-8"))
    print(json.dumps(result, ensure_ascii=False))


if __name__ == "__main__":
    main()
