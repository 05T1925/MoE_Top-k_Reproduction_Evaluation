#!/usr/bin/env python3
"""Same-host loopback EXPERIMENTAL pilot (1 warm-up + 5 measured runs per profile)."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import statistics
import subprocess
import time


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def parse_block(text: str, name: str) -> dict[str, str]:
    match = re.search(rf"{name}=\{{(.*?)\}}", text, re.S)
    if not match:
        raise ValueError(f"missing {name} block")
    result = {}
    for line in match.group(1).splitlines():
        if "=" in line:
            k, v = line.strip().split("=", 1)
            result[k] = v
    return result


def summarize(values: list[float]) -> dict[str, float]:
    return {"min": min(values), "median": statistics.median(values), "max": max(values)}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--binary", required=True, help="WSL path to S14 three-process TEST_ONLY executable")
    ap.add_argument("--out", required=True, type=Path, help="external evidence directory")
    ap.add_argument("--distro", default="Ubuntu-24.04")
    ap.add_argument("--runs", type=int, default=5)
    args = ap.parse_args()
    if args.runs != 5:
        raise SystemExit("this pilot fixes five measured repetitions")
    if args.out.exists() and any(args.out.iterdir()):
        raise SystemExit("output directory must be new and empty")
    args.out.mkdir(parents=True, exist_ok=True)
    profiles = [(64, 8, 1206), (128, 80, 128080)]
    env = subprocess.run(["wsl.exe", "-d", args.distro, "-u", "root", "--", "bash", "-lc",
                          "uname -a; free -b; df -B1 /tmp"], check=True, capture_output=True,
                         text=True, encoding="utf-8", errors="replace")
    (args.out / "environment.txt").write_text(env.stdout, encoding="utf-8")
    binary = subprocess.run(["wsl.exe", "-d", args.distro, "-u", "root", "--", "sha256sum", args.binary],
                           check=True, capture_output=True, text=True,
                           encoding="utf-8", errors="replace").stdout.strip()
    records = []
    for n, k, seed in profiles:
        for rep in range(args.runs + 1):
            label = "warmup" if rep == 0 else f"run{rep}"
            command = ["wsl.exe", "-d", args.distro, "-u", "root", "--", args.binary,
                       "--pilot-case", str(n), str(k), str(seed)]
            start = time.perf_counter()
            proc = subprocess.run(command, capture_output=True, text=True, encoding="utf-8", errors="replace")
            wall = time.perf_counter() - start
            raw = proc.stdout + ("\nSTDERR\n" + proc.stderr if proc.stderr else "")
            logfile = args.out / f"n{n}_k{k}_{label}.txt"
            logfile.write_text(raw, encoding="utf-8")
            if proc.returncode != 0:
                raise RuntimeError(f"process failed n={n} K={k} {label} rc={proc.returncode}: {raw[-1000:]}")
            case = re.search(r"case n=(\d+) K=(\d+) seed=(\d+) status=([^ ]+)", raw)
            if not case:
                raise RuntimeError(f"missing case record n={n} K={k} {label}")
            p0 = parse_block(raw, "party0_metrics")
            dealer = parse_block(raw, "dealer")
            per_round = {}
            for round_no in range(1, 5):
                match = re.fullmatch(
                    r"(\d+)_unique=(\d+)_dummy=(\d+)_repeat=(\d+)_fnv64=([0-9a-fA-F]+)",
                    p0[f"round{round_no}_logical"])
                if not match:
                    raise ValueError(f"malformed R{round_no} count tuple: {p0[f'round{round_no}_logical']}")
                per_round[str(round_no)] = {
                    "logical": int(match.group(1)), "unique": int(match.group(2)),
                    "dummy": int(match.group(3)), "repeat": int(match.group(4)),
                    "edge_fnv64": match.group(5)}
            record = {"n": n, "K": k, "input_seed": seed, "iteration": label,
                      "status": case.group(4), "wall_time_s": wall,
                      "select_logical_calls": int(p0["logical"]),
                      "select_unique_slots": int(p0["select_unique"]),
                      "membership_slots": int(p0["membership_slots"]),
                      "party_online_time_us": int(p0["online_time_us"]),
                      "party_claim_time_us": int(p0["process_slot_claim_us"]),
                      "party_slot_lookup_time_us": int(p0["slot_lookup_us"]),
                      "party_bundle_open_us": int(p0["bundle_open_us"]),
                      "party_durable_bundle_claim_us": int(p0["bundle_claim_us"]),
                      "raw_share_read_us": int(p0["raw_share_read_us"]),
                      "party_peak_rss_kb": int(p0["peak_rss_kb"]),
                      "dealer_peak_rss_kb": int(dealer["peak_rss_kb"]),
                      "party_sent_bytes": int(p0["sent"]), "party_received_bytes": int(p0["recv"]),
                      "message_phases": int(p0["message_phases"]),
                      "party_ucmp_evals": int(p0["key_eval"]), "party_dcf_evals": int(p0["dcf_eval"]),
                      "claimed_slots": int(p0["claimed_slots"]),
                      "unused_cmpagg_slots": int(p0["unused_cmpagg"]),
                      "omitted_cmpagg_slots": int(p0["omitted_cmpagg"]),
                      "sampler_prf_words": int(p0["sampler_prf_words"]),
                      "party_stage_time_us": {
                          key: int(p0[key]) for key in (
                              "raw_adapter_us", "raw_adapter_eval_us", "raw_adapter_exchange_us",
                              "forward_shuffle_us", "forward_shuffle_exchange_us",
                              "sampling_coin_exchange_us", "membership_us", "membership_eval_us",
                              "inverse_shuffle_us", "inverse_shuffle_exchange_us",
                              "status_coordination_us", "round1_time_us", "round1_eval_us",
                              "round1_exchange_us", "round2_time_us", "round2_eval_us",
                              "round2_exchange_us", "round3_time_us", "round3_eval_us",
                              "round3_exchange_us", "round4_time_us", "round4_eval_us",
                              "round4_exchange_us")},
                      "per_round": per_round,
                      "dealer_offline_total_us": int(dealer["offline_total_us"]),
                      "dealer_generation_us": int(dealer["generation_us"]),
                      "dealer_seal_us": int(dealer["seal_us"]),
                      "dealer_delivery_us": int(dealer["delivery_us"]),
                      "bundle_bytes_per_party": int(dealer["party0_bundle_bytes"]),
                      "preissued_slots_per_party": int(dealer["slots_per_party"]),
                      "raw_sha256": sha256(logfile), "raw_file": logfile.name}
            records.append(record)
            if record["status"] != "SUCCESS":
                raise RuntimeError(f"pilot case was not SUCCESS: {record}")
    summaries = []
    for n, k, _ in profiles:
        group = [r for r in records if r["n"] == n and r["iteration"] != "warmup"]
        summaries.append({"n": n, "K": k, "warmups": 1, "measured_runs": 5,
                          "wall_time_s": summarize([r["wall_time_s"] for r in group]),
                          "select_logical_calls": summarize([r["select_logical_calls"] for r in group]),
                          "select_unique_slots": summarize([r["select_unique_slots"] for r in group]),
                          "membership_slots": summarize([r["membership_slots"] for r in group]),
                          "party_online_time_us": summarize([r["party_online_time_us"] for r in group]),
                          "dealer_offline_total_us": summarize([r["dealer_offline_total_us"] for r in group]),
                          "party_claim_time_us": summarize([r["party_claim_time_us"] for r in group]),
                          "party_slot_lookup_time_us": summarize([r["party_slot_lookup_time_us"] for r in group]),
                          "party_bundle_open_us": summarize([r["party_bundle_open_us"] for r in group]),
                          "party_durable_bundle_claim_us": summarize([r["party_durable_bundle_claim_us"] for r in group]),
                          "party_peak_rss_kb": summarize([r["party_peak_rss_kb"] for r in group]),
                          "dealer_peak_rss_kb": summarize([r["dealer_peak_rss_kb"] for r in group]),
                          "party_sent_bytes": summarize([r["party_sent_bytes"] for r in group]),
                          "party_ucmp_evals": summarize([r["party_ucmp_evals"] for r in group]),
                          "party_dcf_evals": summarize([r["party_dcf_evals"] for r in group])})
    report = {"label": "BMW16_DERIVED_SELECT_4R_TEST_ONLY_S14_LOOPBACK_PILOT_NOT_V4",
              "input_output": "same frozen raw scores per profile; TEST_ONLY parent checks xor mask against frozen oracle",
              "network": "local socketpair loopback; not LAN/WAN",
              "cpu_memory_limits": "no artificial ulimit; same WSL environment for all runs",
              "binary_sha256_record": binary, "environment_file_sha256": sha256(args.out / "environment.txt"),
              "profiles": summaries, "runs": records}
    report_path = args.out / "pilot.json"
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"result": "PASS", "report": str(report_path),
                      "report_sha256": sha256(report_path), "profiles": summaries}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
