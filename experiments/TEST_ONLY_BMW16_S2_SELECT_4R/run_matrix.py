"""Run the frozen TEST_ONLY Select matrix and retain summaries plus edge traces."""

from __future__ import annotations

import argparse
import gzip
import hashlib
import itertools
import json
from collections import Counter
from pathlib import Path

from select4r import generated_scores, run_select


def _cases(seeds: int, small_max_n: int):
    serial = 0
    for n in range(1, small_max_n + 1):
        for permutation in itertools.permutations(range(n)):
            for k in range(1, n + 1):
                yield {
                    "case_id": f"small_perm_n{n}_p{serial}_k{k}",
                    "scores": list(permutation),
                    "k": k,
                    "input_seed": 300000 + serial,
                    "algo_seed": 900000 + 31 * serial + k,
                    "family": "exhaustive_strict_order_permutations",
                }
            serial += 1

    official = [(128, 2), (128, 8), (256, 2), (256, 8), (1000, 80)]
    for config_index, (n, k) in enumerate(official):
        for seed in range(seeds):
            input_seed = 0xB1600000 + config_index * 10000 + seed
            algo_seed = 0xA1600000 + config_index * 10000 + seed
            yield {
                "case_id": f"official_n{n}_k{k}_seed{seed}",
                "scores": generated_scores(n, input_seed, "random"),
                "k": k,
                "input_seed": input_seed,
                "algo_seed": algo_seed,
                "family": "official_random_matrix",
            }

    adversarial = [
        (8, 3, "fixed_example", 17),
        (9, 1, "boundaries", 71),
        (9, 9, "boundaries", 72),
        (18, 1, "ties", 73),
        (18, 18, "ties", 74),
        (17, 9, "all_equal", 75),
        (31, 16, "random", 76),
        (32, 17, "boundaries", 77),
        (33, 17, "ties", 78),
        (65, 2, "random", 79),
        (65, 65, "random", 80),
        (128, 2, "ties", 81),
        (128, 8, "all_equal", 82),
        (256, 2, "boundaries", 83),
        (256, 8, "ties", 84),
        (1000, 80, "all_equal", 85),
        (1000, 80, "boundaries", 86),
    ]
    for i, (n, k, mode, seed) in enumerate(adversarial):
        fixed_example = [5, 0, 7, 1, 6, 2, 4, 3]
        yield {
            "case_id": f"adversarial_{mode}_n{n}_k{k}_seed{seed}",
            "scores": fixed_example if mode == "fixed_example" else generated_scores(n, seed, mode),
            "k": k,
            "input_seed": seed,
            "algo_seed": 0x5A2B0000 + i,
            "family": "adversarial_fixed_example" if mode == "fixed_example" else f"adversarial_{mode}",
        }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--seeds", type=int, default=8)
    parser.add_argument("--small-max-n", type=int, default=5)
    parser.add_argument("--out-dir", type=Path, default=Path("results"))
    args = parser.parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=True)
    summaries_path = args.out_dir / "select4r_run_summaries.jsonl"
    failures_path = args.out_dir / "select4r_failed_or_non-success_runs.jsonl"
    trace_path = args.out_dir / "select4r_edge_traces.jsonl.gz"

    counts = Counter()
    total = 0
    with summaries_path.open("w", encoding="utf-8", newline="\n") as summaries, \
            failures_path.open("w", encoding="utf-8", newline="\n") as failures, \
            trace_path.open("wb") as trace_file:
        with gzip.GzipFile(fileobj=trace_file, mode="wb", mtime=0, compresslevel=9) as traces:
            for case in _cases(args.seeds, args.small_max_n):
                total += 1
                summary, trace = run_select(
                    case["scores"], case["k"], case["algo_seed"], case["input_seed"]
                )
                summary.update({
                    "case_id": case["case_id"],
                    "family": case["family"],
                    "score_input_sha256": hashlib.sha256(
                        json.dumps(case["scores"], separators=(",", ":")).encode("ascii")
                    ).hexdigest(),
                })
                trace["case_id"] = case["case_id"]
                trace["family"] = case["family"]
                summary_line = json.dumps(summary, sort_keys=True, separators=(",", ":"))
                summaries.write(summary_line + "\n")
                if summary.get("status") != "SUCCESS":
                    failures.write(summary_line + "\n")
                traces.write(
                    (json.dumps(trace, sort_keys=True, separators=(",", ":")) + "\n").encode("utf-8")
                )
                counts[summary.get("status", "MISSING_STATUS")] += 1
                counts[f"family:{case['family']}"] += 1

    meta = {
        "label": "BMW16_DERIVED_SELECT_4R_TEST_ONLY",
        "total_runs": total,
        "status_counts": {key: value for key, value in counts.items() if not key.startswith("family:")},
        "family_counts": {key.removeprefix("family:"): value for key, value in counts.items() if key.startswith("family:")},
        "seeds_per_official_config": args.seeds,
        "small_permutation_max_n": args.small_max_n,
        "summaries": summaries_path.name,
        "failures": failures_path.name,
        "traces": trace_path.name,
    }
    meta_path = args.out_dir / "select4r_run_matrix_meta.json"
    meta_path.write_text(json.dumps(meta, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(meta, sort_keys=True))


if __name__ == "__main__":
    main()
