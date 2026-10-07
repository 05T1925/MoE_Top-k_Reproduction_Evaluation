#!/usr/bin/env python3
"""Recompute S15's stored 1+5 pilot summaries and raw evidence hashes."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import statistics


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def triple(values: list[float | int]) -> dict[str, float | int]:
    return {"min": min(values), "median": statistics.median(values), "max": max(values)}


def parse_metrics(raw: str, party: int) -> dict[str, str]:
    match = re.search(
        r"party0_metrics=\{(.*?)\} party1_metrics=\{(.*?)\} dealer=\{(.*?)\}",
        raw, re.S)
    if not match:
        raise ValueError("raw pilot attempt lacks structured party metrics")
    body = match.group(party + 1)
    return dict(line.split("=", 1) for line in body.splitlines() if "=" in line)


def expansions(n: int, calls: int) -> int:
    padded = 2
    while padded < n:
        padded <<= 1
    raw = 4 * padded
    if calls < raw:
        raise ValueError(f"n={n}: DCF calls below raw adapter minimum")
    return raw * 34 + (calls - raw) * (33 + padded.bit_length() - 1)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("pilot", type=Path)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    source = json.loads(args.pilot.read_text(encoding="utf-8"))
    basedir = args.pilot.parent
    raw_checks = []
    grouped: dict[tuple[int, int], list[dict]] = {}
    expanded_rows = []
    for row in source["runs"]:
        raw_path = basedir / row["raw_file"]
        digest = sha256(raw_path)
        if digest != row["raw_sha256"]:
            raise ValueError(f"raw evidence hash mismatch: {raw_path.name}")
        raw_checks.append({"file": row["raw_file"], "sha256": digest})
        raw_text = raw_path.read_text(encoding="utf-8")
        if row["n"] > 1:
            p0, p1 = parse_metrics(raw_text, 0), parse_metrics(raw_text, 1)
            if any(m.get("dcf_counters_enabled") != "1" for m in (p0, p1)):
                raise ValueError(f"counter missing in pilot raw attempt {row['raw_file']}")
            if any(int(m["dcf_eval_calls"]) != int(m["dcf_eval"]) for m in (p0, p1)):
                raise ValueError(f"DCF call/party metric mismatch in {row['raw_file']}")
            per_party = [expansions(row["n"], int(m["dcf_eval_calls"])) for m in (p0, p1)]
            measured = [int(m["dcf_eval_node_expansions"]) for m in (p0, p1)]
            if measured != per_party:
                raise ValueError(f"DCF node expansion formula mismatch in {row['raw_file']}")
            row["audit_online_prg_calls_total"] = sum(measured)
            row["audit_online_aes_blocks_total"] = 2 * sum(measured)
            row["audit_sampler_prf_words_total"] = sum(int(m["sampler_prf_words"]) for m in (p0, p1))
            expanded_rows.append({"raw_file": row["raw_file"], "status": row["status"],
                                  "online_prg_calls_total": row["audit_online_prg_calls_total"],
                                  "online_aes_blocks_total": row["audit_online_aes_blocks_total"],
                                  "sampler_prf_words_total": row["audit_sampler_prf_words_total"]})
        grouped.setdefault((row["n"], row["K"]), []).append(row)
    recomputed = []
    for profile in source["profiles"]:
        key = profile["n"], profile["K"]
        rows = grouped[key]
        measured = [r for r in rows if r["iteration"] != "warmup"]
        success = [r for r in measured if r["status"] == "SUCCESS" and r["controller_exit_code"] == 0]
        if len(rows) != 6 or len(measured) != 5:
            raise ValueError(f"profile {key}: expected one warmup plus five measured attempts")
        status_counts = {s: sum(r["status"] == s for r in measured)
                         for s in sorted({r["status"] for r in measured})}
        checks = {
            "status_counts": status_counts == profile["status_counts"],
            "all_attempt_wall_s": triple([r["attempt_wall_time_s"] for r in measured]) == profile["all_attempt_wall_s"],
            "successful_online_us": triple([r["party_metrics"]["online_time_us"] for r in success]) == profile["successful_online_us"],
            "successful_offline_us": triple([r["dealer_metrics"]["offline_total_us"] for r in success]) == profile["successful_offline_us"],
            "successful_bundle_bytes_party": triple([r["dealer_metrics"]["party0_bundle_bytes"] for r in success]) == profile["successful_bundle_bytes_party"],
            "successful_logical_comparisons": triple([r["party_metrics"]["logical"] for r in success]) == profile["successful_logical_comparisons"],
        }
        if not all(checks.values()):
            raise ValueError(f"profile summary mismatch {key}: {checks}")
        recomputed.append({"n": key[0], "K": key[1], "attempts": len(measured),
                           "status_counts": status_counts, "summary_checks": checks,
                           "natural_or_injected_abort_attempts": [
                               {"iteration": r["iteration"], "status": r["status"],
                                "attempt_wall_time_s": r["attempt_wall_time_s"]}
                               for r in measured if r["status"] != "SUCCESS"]})
    report = {"label": "BMW16_S16_S15_PILOT_INDEPENDENT_AUDIT_NOT_V4",
              "source_pilot": str(args.pilot), "source_pilot_sha256": sha256(args.pilot),
              "raw_file_count": len(raw_checks), "all_raw_hashes_match": True,
              "raw_files": raw_checks, "online_counter_rechecks": expanded_rows,
              "profiles": recomputed,
              "boundary": "This verifies the stored S15 records and summaries; it is not a new timing run and does not make S15 comparable to other schemes/revisions."}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"result": "PASS", "raw_files": len(raw_checks),
                      "profiles": len(recomputed), "report_sha256": sha256(args.out)}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
