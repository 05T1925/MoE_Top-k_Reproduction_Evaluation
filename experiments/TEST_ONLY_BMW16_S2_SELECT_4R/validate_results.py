"""Post-run differential validation against S1 Python and VFSS C++ oracles.

The selector never imports either oracle. This validator is a separate pass
over frozen summaries and serialized input traces.
"""

from __future__ import annotations

import argparse
import gzip
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
from collections import Counter


HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
S1_ORACLE_DIR = REPO / "experiments" / "TEST_ONLY_BMW16_DERIVED_SELECT_4R"
sys.path.insert(0, str(S1_ORACLE_DIR))
from oracle import select_index, top_k_mask  # noqa: E402


def _load_runs(summaries_path: Path, traces_path: Path):
    summaries = {}
    with summaries_path.open("r", encoding="utf-8") as stream:
        for line in stream:
            value = json.loads(line)
            summaries[value["case_id"]] = value
    traces = {}
    with gzip.open(traces_path, "rt", encoding="utf-8") as stream:
        for line in stream:
            value = json.loads(line)
            case_id = value["case_id"]
            header = value["events"][0]
            traces[case_id] = {
                "trace": value,
                "scores": header["input_scores"],
                "summary": header["summary"],
            }
    if set(summaries) != set(traces):
        raise ValueError("summary and trace case IDs differ")
    return summaries, traces


def _run_cpp_oracle(traces: dict[str, dict], exe_path: Path) -> dict[str, tuple[int, list[int]]]:
    case_ids = list(traces)
    rows = [str(len(case_ids))]
    for case_id in case_ids:
        value = traces[case_id]
        scores = value["scores"]
        k = int(value["summary"]["K"])
        rows.append(f"{len(scores)} {k} " + " ".join(str(score) for score in scores))
    process = subprocess.run(
        [str(exe_path)], input="\n".join(rows) + "\n", text=True,
        capture_output=True, check=True,
    )
    lines = process.stdout.splitlines()
    if len(lines) != len(case_ids):
        raise ValueError("C++ oracle returned wrong number of cases")
    result = {}
    for case_id, line in zip(case_ids, lines):
        selected_text, mask_text = line.split()
        result[case_id] = (int(selected_text), [int(bit) for bit in mask_text])
    return result


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--summaries", type=Path, default=HERE / "results" / "select4r_run_summaries.jsonl")
    parser.add_argument("--traces", type=Path, default=HERE / "results" / "select4r_edge_traces.jsonl.gz")
    parser.add_argument("--out", type=Path, default=HERE / "results" / "select4r_oracle_validation.jsonl")
    parser.add_argument("--summary-out", type=Path, default=HERE / "results" / "select4r_oracle_validation_summary.json")
    args = parser.parse_args()
    summaries, traces = _load_runs(args.summaries, args.traces)

    compiler = os.environ.get("CXX") or "g++"
    with tempfile.TemporaryDirectory(prefix="bmw16_s2_oracle_") as temp_dir:
        exe_path = Path(temp_dir) / ("oracle_harness.exe" if os.name == "nt" else "oracle_harness")
        command = [
            compiler, "-std=c++17", "-O2", f"-I{REPO / 'VFSS' / 'include'}",
            str(HERE / "oracle_harness.cpp"), "-o", str(exe_path),
        ]
        compiled = subprocess.run(command, text=True, capture_output=True)
        if compiled.returncode:
            raise RuntimeError(
                "C++ oracle harness compile failed:\n" + compiled.stdout + compiled.stderr
            )
        cpp = _run_cpp_oracle(traces, exe_path)

    counts = Counter()
    records = []
    for case_id, run_summary in summaries.items():
        value = traces[case_id]
        scores = value["scores"]
        k = int(run_summary["K"])
        python_expected_index = select_index(scores, k)
        python_mask = top_k_mask(scores, k)
        cpp_index, cpp_mask = cpp[case_id]
        python_cpp_index_equal = python_expected_index == cpp_index
        python_cpp_mask_equal = python_mask == cpp_mask
        if not python_cpp_index_equal or not python_cpp_mask_equal:
            counts["ORACLE_CROSSCHECK_MISMATCH"] += 1
        else:
            counts["ORACLE_CROSSCHECK_MATCH"] += 1

        candidate = run_summary.get("selected_original_index")
        execution_status = run_summary.get("status")
        if candidate is None:
            candidate_result = "NO_CANDIDATE"
        elif candidate == python_expected_index:
            candidate_result = "MATCH"
        else:
            candidate_result = "MISMATCH"
        validated_status = execution_status
        if execution_status == "SUCCESS" and candidate_result != "MATCH":
            validated_status = "COMPLETED_WRONG"
        counts[f"execution:{execution_status}"] += 1
        counts[f"candidate:{candidate_result}"] += 1
        counts[f"validated:{validated_status}"] += 1
        records.append({
            "case_id": case_id,
            "family": run_summary.get("family"),
            "n": len(scores),
            "K": k,
            "input_seed": run_summary.get("input_seed"),
            "algorithm_seed": run_summary.get("algorithm_seed"),
            "execution_status": execution_status,
            "validated_status": validated_status,
            "selected_original_index": candidate,
            "python_oracle_index": python_expected_index,
            "cpp_oracle_index": cpp_index,
            "candidate_result": candidate_result,
            "python_cpp_index_equal": python_cpp_index_equal,
            "python_cpp_mask_equal": python_cpp_mask_equal,
            "score_input_sha256": run_summary.get("score_input_sha256"),
        })
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(
        "".join(json.dumps(record, sort_keys=True) + "\n" for record in records),
        encoding="utf-8",
    )
    summary = {
        "cpp_compile_command": command,
        "cpp_compile_exit_code": compiled.returncode,
        "cases": len(records),
        "counts": dict(sorted(counts.items())),
        "python_oracle": "S1 frozen TEST_ONLY oracle.py",
        "cpp_oracle": "VFSS/include/moe_topk/topk_oracle.h via TEST_ONLY harness",
        "selector_imported_oracle": False,
        "validated_success_requires_candidate_match": True,
    }
    args.summary_out.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(summary, sort_keys=True))


if __name__ == "__main__":
    main()
