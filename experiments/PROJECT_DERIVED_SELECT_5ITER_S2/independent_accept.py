#!/usr/bin/env python3
"""Independent S2 replay of the S1 plaintext trace against VFSS's frozen oracle.

This is TEST_ONLY. It does not implement or claim a secure protocol.
"""
from __future__ import annotations

import argparse
import hashlib
import itertools
import json
import math
import os
import random
import shutil
import struct
import subprocess
import sys
from functools import cmp_to_key
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SUBJECT = ROOT / "experiments" / "TEST_ONLY_PROJECT_DERIVED_SELECT_5ITER"
sys.path.insert(0, str(SUBJECT))
from select5iter import INT32_MAX, INT32_MIN, select_kth  # noqa: E402


def precedes(scores: list[int], a: int, b: int) -> bool:
    return scores[a] > scores[b] or (scores[a] == scores[b] and a < b)


def ceil_sqrt(v: int) -> int:
    q = math.isqrt(v)
    return q + (q * q != v)


def params(m: int, r: int, original_n: int) -> tuple[int, int, int, int]:
    s = min(m, ceil_sqrt(m))
    a = ceil_sqrt(2 * s * original_n.bit_length())
    mu_floor = (s * (r - 1)) // m
    mu_ceil = (s * (r - 1) + m - 1) // m
    low = min(s - 1, max(1, mu_floor - a))
    high = min(s, max(low + 1, mu_ceil + a + 1))
    return s, low, high, a


def derive_sample(seed: int, round_no: int, candidates: list[int], size: int) -> list[int]:
    payload = f"PROJECT_DERIVED_SELECT_5ITER_TEST_ONLY:v1:{seed}:{round_no}".encode("ascii")
    round_seed = int.from_bytes(hashlib.sha256(payload).digest()[:8], "big")
    return random.Random(round_seed).sample(candidates, size)


def check_trace(scores: list[int], k: int, algorithm_seed: int, trace: dict) -> dict:
    n = len(scores)
    candidates = list(range(n))
    rank = k
    selected = None
    all_call_edges: list[tuple[int, int]] = []
    per_round: list[int] = []
    causal_sub_layers = 0

    def edge_results(round_row: dict) -> dict[tuple[int, int], bool]:
        rows = round_row["outcomes"]
        edges = [tuple(row[:2]) for row in rows]
        if edges != sorted(edges) or len(edges) != len(set(edges)):
            raise AssertionError("edge list is not sorted and duplicate-free")
        if any(a >= b for a, b in edges):
            raise AssertionError("edge endpoint orientation is not canonical")
        frozen = [tuple(edge) for edge in round_row["frozen_edges"]]
        if frozen != edges:
            raise AssertionError("frozen edge plan and evaluated outcome rows differ")
        digest = hashlib.sha256()
        for a, b in edges:
            digest.update(struct.pack("<II", a, b))
        if digest.hexdigest() != round_row["edge_sha256"]:
            raise AssertionError("frozen edge digest mismatch")
        return {(a, b): bool(row[2]) for (a, b), row in zip(edges, rows)}

    def outcome(outcomes: dict[tuple[int, int], bool], a: int, b: int) -> bool:
        if a == b:
            return False
        key = (min(a, b), max(a, b))
        value = outcomes[key]
        return value if a == key[0] else not value

    def rank_of(outcomes: dict[tuple[int, int], bool], item: int) -> int:
        return 1 + sum(outcome(outcomes, other, item) for other in candidates if other != item)

    rows = trace["rounds"]
    assert len(rows) == 5
    for i, row in enumerate(rows, 1):
        assert row["round"] == i
        assert row["candidate_before"] == candidates
        assert row["rank_before"] == rank
        if selected is not None:
            assert row["kind"] == "TERMINAL_EMPTY" and not row["frozen_edges"] and not row["outcomes"]
            assert row["candidate_after"] == candidates and row["rank_after"] == rank
            assert row["selected_index"] == selected
            per_round.append(0)
            continue

        m = len(candidates)
        outcomes = edge_results(row)
        for (left, right), bit in outcomes.items():
            assert bit == precedes(scores, left, right), "edge outcome disagrees with frozen score comparator"
        edges = set(outcomes)
        if i < 5:
            if m == 1:
                assert row["kind"] == "SINGLETON_EXACT" and not edges
                selected = candidates[0]
                assert row["selected_index"] == selected
                assert row["candidate_after"] == candidates and row["rank_after"] == rank
                per_round.append(0)
                continue
            s, ql, qh, _a = params(m, rank, n)
            assert row["kind"] == "SAMPLE_QUANTILE_BRACKET"
            sample = row["sample_ids"]
            assert len(sample) == s and len(set(sample)) == s and set(sample) <= set(candidates)
            assert sample == derive_sample(algorithm_seed, i, candidates, s)
            clique = {(min(a, b), max(a, b)) for a, b in itertools.combinations(sample, 2)}
            assert clique <= edges
            # Recover sample order only from the recorded clique outcomes. This is
            # an independent causal audit: pivots cannot precede this sub-layer.
            sample_rank = {x: 1 + sum(outcome(outcomes, y, x) for y in sample if y != x)
                           for x in sample}
            assert sorted(sample_rank.values()) == list(range(1, s + 1))
            order = sorted(sample, key=sample_rank.__getitem__)
            assert row["sample_order"] == order
            assert row["q_low"] == ql and row["q_high"] == qh
            pivots = [order[ql - 1], order[qh - 1]]
            assert row["pivot_ids"] == pivots
            stars = {(min(p, x), max(p, x)) for p in pivots for x in candidates if x not in set(sample)}
            assert edges == clique | stars
            # Two actual comparison dependency layers: clique outcomes determine
            # pivots; only then can non-sample pivot-star requests be fixed.
            causal_sub_layers += int(bool(clique)) + int(bool(stars))
            px, py = (rank_of(outcomes, p) for p in pivots)
            assert row["pivot_ranks"] == [px, py] and px < py
            if px == rank:
                selected = pivots[0]
            elif py == rank:
                selected = pivots[1]
            elif px < rank < py:
                new_candidates = [x for x in candidates
                                  if outcome(outcomes, pivots[0], x) and outcome(outcomes, x, pivots[1])]
                new_rank = rank - px
            elif py < rank:
                new_candidates = [x for x in candidates if outcome(outcomes, pivots[1], x)]
                new_rank = rank - py
            elif px > rank:
                new_candidates = [x for x in candidates if outcome(outcomes, x, pivots[0])]
                new_rank = rank
            else:
                raise AssertionError("rank state is not covered by pivot cases")
            if selected is None:
                assert row["candidate_after"] == new_candidates
                assert row["rank_after"] == new_rank
                assert 1 <= new_rank <= len(new_candidates)
                candidates, rank = new_candidates, new_rank
            else:
                assert row["candidate_after"] == candidates and row["rank_after"] == rank
            assert row["selected_index"] == selected
        else:
            expected = {(candidates[a], candidates[b]) for a in range(m) for b in range(a + 1, m)}
            assert row["kind"] == "EXACT_CANDIDATE_CLIQUE" and edges == expected
            causal_sub_layers += int(bool(edges))
            ranks = {x: rank_of(outcomes, x) for x in candidates}
            assert sorted(ranks.values()) == list(range(1, m + 1))
            selecteds = [x for x, xrank in ranks.items() if xrank == rank]
            assert len(selecteds) == 1
            selected = selecteds[0]
            assert row["candidate_after"] == candidates and row["rank_after"] == rank
            assert row["selected_index"] == selected

        per_round.append(len(edges))
        all_call_edges.extend(sorted(edges))

    assert selected == trace["selected_index"]
    order = sorted(range(n), key=cmp_to_key(lambda a, b: -1 if precedes(scores, a, b)
                                            else (1 if precedes(scores, b, a) else 0)))
    assert selected == order[k - 1]
    expected_mask = [int(pos in set(order[:k])) for pos in range(n)]
    assert trace["mask"] == expected_mask and sum(expected_mask) == k
    unique = set(all_call_edges)
    assert sum(per_round) == len(all_call_edges)
    return {"per_round": per_round, "logical_calls": len(all_call_edges),
            "unique_pairs": len(unique), "repeated_calls": len(all_call_edges) - len(unique),
            "causal_comparison_sub_layers": causal_sub_layers,
            "selected_index": selected, "mask_ones": sum(expected_mask)}


def cases():
    for n in range(1, 8):
        for pno, perm in enumerate(itertools.permutations(range(n))):
            for k in range(1, n + 1):
                yield ("all_permutations", list(perm), k, (n << 48) | (pno << 8) | k)
    patterns = [
        [0], [0, 0], [0] * 7,
        [INT32_MIN, INT32_MAX, -1, 0, 1, INT32_MIN, INT32_MAX],
        [4096 if i % 2 == 0 else -4096 for i in range(31)],
        [((i % 5) - 2) * 4096 for i in range(65)],
        [INT32_MIN if i % 7 == 0 else INT32_MAX if i % 7 == 1 else (i % 3) - 1 for i in range(257)],
        [INT32_MIN, INT32_MIN + 1, -4096, -1, 0, 1, 4096, INT32_MAX - 1, INT32_MAX],
    ]
    for pno, values in enumerate(patterns):
        for k in sorted({1, max(1, (len(values) + 1) // 2), len(values)}):
            for seed in (0, 1, 0x5A17, 0xC0FFEE):
                yield (f"fixed_{pno}", values, k, seed)
    fixed = [((i * 7919) % 10007) - 5003 for i in range(257)]
    for seed in range(32):
        yield ("fixed_input_seed_sweep", fixed, 129, seed)
    for input_seed in (17, 2026, 65537):
        for distribution_id, distribution in enumerate(("wide", "ties", "boundary")):
            rng = random.Random(input_seed ^ (distribution_id << 24))
            bounds = [INT32_MIN, INT32_MIN + 1, -4096, -1, 0, 1, 4096, INT32_MAX - 1, INT32_MAX]
            values = ([rng.randint(INT32_MIN, INT32_MAX) for _ in range(129)] if distribution == "wide"
                      else [rng.randint(-3 * 4096, 3 * 4096) for _ in range(129)] if distribution == "ties"
                      else [rng.choice(bounds) for _ in range(129)])
            for k in (1, 2, 64, 65, 128, 129):
                yield (f"random_{distribution}_{input_seed}", values, k, 9000 + input_seed + k)
    yield ("representative", [4096 if i % 2 == 0 else -4096 for i in range(31)], 16, 0)


def compile_oracle(out_dir: Path) -> tuple[Path, list[str]]:
    compiler = shutil.which(os.environ.get("CXX", "g++")) or shutil.which("clang++")
    if compiler is None:
        raise RuntimeError("g++/clang++ is required")
    executable = out_dir / ("s2_frozen_oracle.exe" if os.name == "nt" else "s2_frozen_oracle")
    command = [compiler, "-std=c++17", "-O2", "-I", str(ROOT / "VFSS" / "include"),
               str(SUBJECT / "frozen_oracle.cpp"), "-o", str(executable)]
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, check=False)
    if result.returncode:
        raise RuntimeError(result.stderr)
    return executable, command


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out-dir", required=True, type=Path)
    args = parser.parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=True)
    rows = list(cases())
    oracle, compile_cmd = compile_oracle(args.out_dir)
    oracle_in = "".join(f"{len(s)} {k} " + " ".join(map(str, s)) + "\n" for _, s, k, _ in rows)
    run = subprocess.run([str(oracle)], input=oracle_in, text=True, capture_output=True,
                         cwd=ROOT, check=False)
    if run.returncode:
        raise RuntimeError(f"frozen oracle exit {run.returncode}: {run.stderr}")
    oracle_rows = run.stdout.splitlines()
    if len(oracle_rows) != len(rows):
        raise RuntimeError("oracle output line count mismatch")
    result_file = args.out_dir / "independent_results.jsonl"
    failure_file = args.out_dir / "independent_failures.jsonl"
    failures = []
    active5 = 0
    all_five_nonempty = 0
    causal_layers_max = 0
    distinct_samples = set()
    with result_file.open("w", encoding="utf-8", newline="\n") as out:
        for (kind, scores, k, seed), oracle_row in zip(rows, oracle_rows):
            try:
                result = select_kth(scores, k, algorithm_seed=seed, with_trace=True)
                audit = check_trace(scores, k, seed, result.trace or {})
                oracle_parts = oracle_row.split()
                oracle_index = int(oracle_parts[0])
                oracle_mask = [int(x) for x in oracle_parts[1]]
                assert result.selected_index == oracle_index
                assert result.mask == oracle_mask
                assert result.per_round_comparisons == audit["per_round"]
                assert result.logical_comparisons == audit["logical_calls"]
                assert result.unique_unordered_pairs == audit["unique_pairs"]
                assert result.repeated_comparison_calls == audit["repeated_calls"]
                active5 += int(result.per_round_comparisons[4] > 0)
                all_five_nonempty += int(all(x > 0 for x in result.per_round_comparisons))
                causal_layers_max = max(causal_layers_max, audit["causal_comparison_sub_layers"])
                if kind == "fixed_input_seed_sweep":
                    distinct_samples.add(tuple(result.trace["rounds"][0]["sample_ids"]))
                out.write(json.dumps({"kind": kind, "n": len(scores), "k": k, "algorithm_seed": seed,
                                      "status": "PASS", **audit}, separators=(",", ":")) + "\n")
            except Exception as exc:
                failures.append({"kind": kind, "scores": scores, "k": k, "algorithm_seed": seed,
                                 "oracle": oracle_row, "error_type": type(exc).__name__, "error": str(exc)})
    # Explicitly test the frozen invalid-input contract without swallowing a failure.
    invalid_rows = []
    for scores, k in (([], 1), ([1], 0), ([1], 2), ([True], 1), ([INT32_MAX + 1], 1)):
        try:
            select_kth(scores, k)
        except ValueError as exc:
            invalid_rows.append({"status": "REJECTED", "error": str(exc)})
        else:
            invalid_rows.append({"status": "UNEXPECTED_SUCCESS"})
    if any(x["status"] != "REJECTED" for x in invalid_rows):
        failures.append({"kind": "invalid_input", "cases": invalid_rows})
    (args.out_dir / "independent_invalid_inputs.json").write_text(json.dumps(invalid_rows, indent=2) + "\n", encoding="utf-8")
    failure_file.write_text("".join(json.dumps(f, separators=(",", ":")) + "\n" for f in failures), encoding="utf-8")
    summary = {"label": "PROJECT_DERIVED_SELECT_5ITER_TEST_ONLY", "subject_revision": "v1",
               "independent_acceptance": "PASS" if not failures else "FAIL", "cases": len(rows),
               "passed": len(rows) - len(failures), "failures": len(failures),
               "exhaustive_strict_permutation_all_k_cases": 40319,
               "active_fifth_clique_cases": active5, "all_five_layers_nonempty_cases": all_five_nonempty,
               "max_causal_comparison_sub_layers": causal_layers_max,
               "distinct_first_samples_fixed_input": len(distinct_samples),
               "compiler_command": compile_cmd, "oracle_exe_sha256": hashlib.sha256(oracle.read_bytes()).hexdigest(),
               "results_sha256": hashlib.sha256(result_file.read_bytes()).hexdigest(),
               "failures_sha256": hashlib.sha256(failure_file.read_bytes()).hexdigest(),
               "invalid_input_checks": invalid_rows,
               "note": "The S1 code computes sample order from plaintext scores; this independent replay reconstructs it from sample-clique outcomes and counts clique-to-star dependency layers."}
    (args.out_dir / "independent_summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))
    return 0 if not failures else 1


if __name__ == "__main__":
    raise SystemExit(main())
