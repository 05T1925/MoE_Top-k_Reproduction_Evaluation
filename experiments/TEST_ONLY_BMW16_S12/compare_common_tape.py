#!/usr/bin/env python3
"""Compare the S12 C++ selector with the frozen S4 Python source on one tape."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

EXPECTED_S4_SHA256 = "8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab"


class CommonTape:
    def __init__(self, words: list[int]):
        self.words = iter(words)

    def sample(self, population, k):
        pool = list(population)
        out = []
        remaining = len(pool)
        for _ in range(k):
            word = next(self.words)
            j = (word * remaining) >> 64
            out.append(pool[j])
            pool[j] = pool[remaining - 1]
            remaining -= 1
        return out


def parse_ids(s: str) -> list[int]:
    return [] if not s else [int(x) for x in s.split(",")]


def parse_fields(line: str) -> tuple[str, dict[str, str]]:
    head, *parts = line.split()
    values = {}
    for part in parts:
        key, value = part.split("=", 1)
        values[key] = value
    return head, values


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--s4", required=True, type=Path)
    ap.add_argument("--tape", required=True, type=Path)
    ap.add_argument("--cpp-trace", required=True, type=Path)
    ap.add_argument("--report", required=True, type=Path)
    args = ap.parse_args()
    s4_bytes = args.s4.read_bytes()
    # Git stores this frozen TEST_ONLY oracle with LF line endings. Windows
    # checkouts may expand those bytes to CRLF; hash the canonical Git-blob
    # content so line-ending policy does not masquerade as a source change.
    canonical_s4_bytes = s4_bytes.replace(b"\r\n", b"\n")
    digest = hashlib.sha256(canonical_s4_bytes).hexdigest()
    if digest != EXPECTED_S4_SHA256:
        raise SystemExit(f"S4 source hash mismatch: {digest}")
    spec = importlib.util.spec_from_file_location("s4_frozen", args.s4)
    if spec is None or spec.loader is None:
        raise SystemExit("unable to import frozen S4 source")
    s4 = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = s4
    spec.loader.exec_module(s4)

    tape = json.loads(args.tape.read_text(encoding="utf-8"))
    cpp_lines = args.cpp_trace.read_text(encoding="utf-8").splitlines()
    zline = next(line for line in cpp_lines if line.startswith("PUBLIC_Z="))
    z = [int(x) for x in zline.partition("=")[2].split(",")]
    n, k = int(tape["n"]), int(tape["k"])
    if len(z) != n:
        raise SystemExit("public Z length does not match tape metadata")
    mapline = next(line for line in cpp_lines if line.startswith("TEST_ONLY_HANDLE_TO_ORIGINAL="))
    handle_to_original = [int(x) for x in mapline.partition("=")[2].split(",")]
    if len(handle_to_original) != n:
        raise SystemExit("test-only shuffle map length mismatch")
    # The C++ adapter reverses the official smaller-is-higher Protocol I key;
    # ascending synthetic values therefore sort by score ascending/index descending.
    original_scores = [int(x) for x in tape["scores"]]
    original_rank = {item: rank for rank, item in enumerate(sorted(range(n), key=lambda i: (original_scores[i], -i)))}
    scores = [original_rank[handle_to_original[handle]] for handle in range(n)]
    ctx = s4.Context(scores, k, input_seed=0, algorithm_seed=0, trace=True)
    words = tape["words"]
    task_names = ["SELECT_LOW_SENTINEL", "SELECT_HIGH_SENTINEL"]
    for task, task_name in enumerate(task_names):
        ctx.rngs[f"{task_name}:R1_SAMPLE"] = CommonTape(words[task][0])
        ctx.rngs[f"{task_name}:R3_SAMPLE"] = CommonTape(words[task][1])

    snapshots = []
    original_execute = ctx.execute_round

    def execute_round(round_no, batches):
        plans = [(task_names.index(batch["descriptor"]["task"]), list(zip(map(int, batch["left"]), map(int, batch["right"]))))
                 for batch in batches]
        returned = original_execute(round_no, batches)
        results = [(task_names.index(batch["descriptor"]["task"]), [int(x) for x in batch["outcomes"]]) for batch in batches]
        snapshots.append((round_no, plans, results))
        return returned

    ctx.execute_round = execute_round
    summary = ctx.run()

    cpp_plans: dict[int, list[tuple[int, list[tuple[int, int]]]]] = {}
    cpp_results: dict[int, list[tuple[int, list[int]]]] = {}
    states: dict[str, dict[int, dict[str, str]]] = {"R1_STATE": {}, "R2_STATE": {}, "R3_STATE": {}, "R4_STATE": {}}
    selected = None
    cpp_abort = None
    for line in cpp_lines:
        if line.startswith("R") and "_PLAN task=" in line:
            head, fields = parse_fields(line)
            rnd = int(head[1:head.index("_")]); edge_text = fields.get("edges", "")
            edges = [] if not edge_text else [tuple(map(int, p.split("-"))) for p in edge_text.split(",")]
            cpp_plans.setdefault(rnd, []).append((int(fields["task"]), edges))
        elif line.startswith("R") and "_RESULT task=" in line:
            head, fields = parse_fields(line)
            rnd = int(head[1:head.index("_")]); bits = [int(x) for x in fields.get("bits", "")]
            cpp_results.setdefault(rnd, []).append((int(fields["task"]), bits))
        elif any(line.startswith(name + " ") for name in states):
            head, fields = parse_fields(line); states[head][int(fields["task"])] = fields
        elif line.startswith("SELECTED handle="):
            selected = int(line.partition("=")[2])
        elif line.startswith("SELECT_ABORT "):
            _, fields = parse_fields(line);cpp_abort=fields

    comparisons = []
    for rnd, plans, results in snapshots:
        if cpp_plans.get(rnd, []) != plans:
            got=cpp_plans.get(rnd,[])
            raise SystemExit(f"round {rnd} edge-plan mismatch cpp={got[:1]} python={plans[:1]}")
        if cpp_results.get(rnd, []) != results:
            got=cpp_results.get(rnd,[])
            mismatch=None
            for (ct, cbits), (pt, pbits) in zip(got, results):
                for i,(a,b) in enumerate(zip(cbits,pbits)):
                    if a!=b:
                        edge=next(edges for task,edges in plans if task==pt)[i]
                        mismatch={"task":pt,"edge_index":i,"edge":edge,"cpp_bit":a,"python_bit":b,"z_left":z[edge[0]] if edge[0]<n else None,"z_right":z[edge[1]] if edge[1]<n else None}
                        break
                if mismatch: break
            raise SystemExit(f"round {rnd} comparison-result mismatch first={mismatch}")
        comparisons.append({"round": rnd, "task_edge_counts": [len(edges) for _, edges in plans], "result_bits_match": True})

    for t, part in enumerate(ctx.partitions):
        c1, c2 = states["R1_STATE"][t], states["R2_STATE"][t]
        if parse_ids(c1["sample"]) != list(map(int, part["sample_ids"])):
            raise SystemExit(f"task {t}: R1 sample mismatch")
        if int(c1["pivot_low"]) != int(part["pivot_low"]) or int(c1["pivot_high"]) != int(part["pivot_high"]):
            raise SystemExit(f"task {t}: R1 pivot mismatch")
        for key, py_key in (("prefix", "prefix_ids"), ("suffix", "suffix_ids"), ("U", "U_ids")):
            if parse_ids(c2[key]) != list(map(int, part[py_key])):
                raise SystemExit(f"task {t}: R2 {key} mismatch")
        if int(c2["q"]) != int(part["q"]):
            raise SystemExit(f"task {t}: R2 rank mismatch")
        if part.get("V_ids") is not None:
            c3 = states["R3_STATE"][t]
            if parse_ids(c3["V"]) != list(map(int, part["V_ids"])):
                raise SystemExit(f"task {t}: R3 sample mismatch")
            if int(c3["x_rank"]) != int(part["x_rank"]) or int(c3["y_rank"]) != int(part["y_rank"]):
                raise SystemExit(f"task {t}: R3 bracket-rank mismatch")
            if parse_ids(c3["W"]) != list(map(int, part["W_ids"])) or int(c3["qW"]) != int(part["qW"]):
                raise SystemExit(f"task {t}: R3 W/rank mismatch")
        if part.get("reject_ids") is not None:
            c4 = states["R4_STATE"][t]
            if parse_ids(c4["reject"]) != list(map(int, part["reject_ids"])) or parse_ids(c4["accept"]) != list(map(int, part["accept_ids"])):
                raise SystemExit(f"task {t}: R4 partition mismatch")
    py_selected = summary["candidate_original_indices"][0] if summary["candidate_original_indices"] else None
    py_status = summary["status"]
    cpp_status = "SUCCESS" if selected is not None else (cpp_abort or {}).get("status", "UNRESOLVED")
    def status_class(value):
        if value == "SUCCESS": return "SUCCESS"
        if value in ("ABORT_ALGORITHM_PROBABILITY", "PROJECT_RANDOM_FAILURE_PATH") or value.startswith(("ABORT_R2_", "ABORT_R3_")): return "PROBABILITY_ABORT"
        if value in ("ABORT_ALGORITHM_INVALID", "COMPLETED_WRONG") or value.startswith(("ABORT_R1_", "R1_SAMPLE_")): return "INVALID_ABORT"
        return value
    if status_class(cpp_status) != status_class(py_status):
        raise SystemExit(f"status class mismatch C++={cpp_status} Python={py_status}")
    if selected != py_selected:
        raise SystemExit(f"selected handle mismatch C++={selected} Python={py_selected}")
    report = {
        "label": "BMW16_S12_COMMON_RANDOM_TAPE_DIFFERENTIAL_TEST_ONLY",
        "s4_source_sha256": digest,
        "n": n, "K": k, "status": summary["status"], "selected_handle": selected,
        "rounds": comparisons,
        "state_checks": ["R1 sample/pivots", "R2 prefix/suffix/U/q", "R3 V/x/y/W/qW", "R4 reject/accept", "selected handle"],
        "result": "PASS",
    }
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
