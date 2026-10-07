#!/usr/bin/env python3
"""Recompute S15/S16 full-path online DCF expansion totals from raw E2E logs."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re


def parse_map(text: str) -> dict[str, str]:
    return dict(line.split("=", 1) for line in text.splitlines() if "=" in line)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def expected_expansions(n: int, calls: int) -> tuple[int, int]:
    padded = 2
    while padded < n:
        padded <<= 1
    index_bits = padded.bit_length() - 1
    width = 33 + index_bits
    raw_calls = 4 * padded  # per party; raw adapter uses 34-bit comparisons
    if calls < raw_calls:
        raise ValueError(f"n={n}: calls={calls} below raw adapter lower count {raw_calls}")
    return raw_calls * 34 + (calls - raw_calls) * width, width


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("log", type=Path)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    raw = args.log.read_text(encoding="utf-8")
    pattern = re.compile(
        r"case n=(\d+) K=(\d+) seed=(\d+) status=([^ ]+).*?"
        r"party0_metrics=\{(.*?)\} party1_metrics=\{(.*?)\} dealer=\{(.*?)\}\s*$",
        re.S | re.M)
    records = []
    for match in pattern.finditer(raw):
        n, k, seed = map(int, match.group(1, 2, 3))
        status = match.group(4)
        p0, p1 = parse_map(match.group(5)), parse_map(match.group(6))
        if n == 1:
            records.append({"n": n, "K": k, "seed": seed, "status": status,
                            "online_prg_calls_total": 0,
                            "dcf_counter_scope": "singleton shortcut; no DCF"})
            continue
        if any(m.get("dcf_counters_enabled") != "1" for m in (p0, p1)):
            records.append({"n": n, "K": k, "seed": seed, "status": status,
                            "online_prg_calls_total": "NOT_MEASURED",
                            "dcf_counter_scope": "party metrics absent or counters not enabled on this exceptional path"})
            continue
        if all(int(m.get("dcf_eval_calls", "0")) == 0 and
               int(m.get("dcf_eval_node_expansions", "0")) == 0 for m in (p0, p1)):
            records.append({"n": n, "K": k, "seed": seed, "status": status,
                            "dcf_eval_calls_total_both_parties": 0,
                            "online_prg_calls_total": 0,
                            "online_aes_blocks_total": 0,
                            "sampler_prf_words_total": sum(int(m.get("sampler_prf_words", "0")) for m in (p0, p1)),
                            "dcf_counter_scope": "enabled counters observed zero calls before online DCF work"})
            continue
        for party, metrics in ((0, p0), (1, p1)):
            calls = int(metrics["dcf_eval_calls"])
            reported_calls = int(metrics["dcf_eval"])
            if calls != reported_calls:
                raise ValueError(f"case n={n} K={k} seed={seed}: party {party} Eval call mismatch")
            expected, width = expected_expansions(n, calls)
            measured = int(metrics["dcf_eval_node_expansions"])
            if measured != expected:
                raise ValueError(
                    f"case n={n} K={k} seed={seed}: party {party} expansions {measured}!={expected}")
        expansions = sum(int(m["dcf_eval_node_expansions"]) for m in (p0, p1))
        eval_calls = sum(int(m["dcf_eval_calls"]) for m in (p0, p1))
        sampler_words = sum(int(m.get("sampler_prf_words", "0")) for m in (p0, p1))
        records.append({"n": n, "K": k, "seed": seed, "status": status,
                        "comparison_bits": width,
                        "dcf_eval_calls_total_both_parties": eval_calls,
                        "online_prg_calls_total": expansions,
                        "online_aes_blocks_total": 2 * expansions,
                        "sampler_prf_words_total": sampler_words,
                        "p0_expansions": int(p0["dcf_eval_node_expansions"]),
                        "p1_expansions": int(p1["dcf_eval_node_expansions"]),
                        "formula_recomputed": True})
    if not records:
        raise SystemExit("no complete E2E case records parsed")
    report = {"label": "BMW16_S16_FULL_PATH_ONLINE_DCF_PRG_AUDIT_TEST_ONLY",
              "source_log": str(args.log), "source_log_sha256": sha256(args.log),
              "case_count": len(records), "records": records,
              "definition": {
                  "online_prg_calls_total": "sum over P0/P1 of ordinary DCF ecbEncTwoBlocks node-expansion invocations in evalDCF",
                  "online_aes_blocks_total": "2*online_prg_calls_total; each counted invocation encrypts two AES blocks",
                  "sampler_prf_words_total": "kept separate from DCF PRG calls",
                  "scope_note": "Source audit found no other online PRG/AES expansion calls in raw adapter, Protocol I shuffle, Select, membership, or inverse routing; offline DCF KeyGen and AES-GCM are not included.",
                  "not_measured": "cryptographic AES instructions internal to system libraries outside counted DCF ecbEncTwoBlocks sites",
              }}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"result": "PASS", "case_count": len(records),
                      "report": str(args.out), "report_sha256": sha256(args.out)}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
