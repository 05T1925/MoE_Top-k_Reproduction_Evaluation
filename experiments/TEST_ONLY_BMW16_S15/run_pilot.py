#!/usr/bin/env python3
"""1 warm-up + 5 attempts, preserving SUCCESS and all abort/error records."""
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


def block(text: str, name: str) -> dict[str, str]:
    match = re.search(rf"{name}=\{{(.*?)\}}", text, re.S)
    if not match:
        return {}
    return {k: v for k, v in (line.strip().split("=", 1) for line in match.group(1).splitlines() if "=" in line)}


def integer(metrics: dict[str, str], key: str):
    return int(metrics[key]) if key in metrics and metrics[key].isdigit() else "NOT_MEASURED"


def summarize(values: list[float]) -> dict[str, float] | str:
    return {"min": min(values), "median": statistics.median(values), "max": max(values)} if values else "NOT_MEASURED_NO_SUCCESSFUL_SAMPLE"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--binary", required=True, help="WSL path to S15 C++ 3-process TEST_ONLY executable")
    ap.add_argument("--out", required=True, type=Path, help="external evidence directory")
    ap.add_argument("--distro", default="Ubuntu-24.04")
    args = ap.parse_args()
    if args.out.exists() and any(args.out.iterdir()):
        raise SystemExit("output directory must be new and empty")
    args.out.mkdir(parents=True, exist_ok=True)
    profiles = [(64, 8, 1206), (128, 80, 128080), (256, 2, 256002), (256, 8, 256008)]
    env = subprocess.run(["wsl.exe", "-d", args.distro, "-u", "root", "--", "bash", "-lc",
                          "uname -a; free -b; df -B1 /tmp"], check=True, capture_output=True,
                         text=True, encoding="utf-8", errors="replace")
    (args.out / "environment.txt").write_text(env.stdout, encoding="utf-8")
    binary = subprocess.run(["wsl.exe", "-d", args.distro, "-u", "root", "--", "sha256sum", args.binary],
                            check=True, capture_output=True, text=True,
                            encoding="utf-8", errors="replace").stdout.strip()
    records = []
    for n, k, seed in profiles:
        for rep in range(6):
            label = "warmup" if rep == 0 else f"run{rep}"
            cmd = ["wsl.exe", "-d", args.distro, "-u", "root", "--", args.binary,
                   "--pilot-case", str(n), str(k), str(seed)]
            start = time.perf_counter()
            proc = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
            wall = time.perf_counter() - start
            raw = proc.stdout + ("\nSTDERR\n" + proc.stderr if proc.stderr else "")
            raw_path = args.out / f"n{n}_k{k}_{label}.txt"
            raw_path.write_text(raw, encoding="utf-8")
            found = re.search(r"case n=(\d+) K=(\d+) seed=(\d+) status=([^ ]+)", raw)
            if not found:
                status = "HARNESS_ERROR_NO_CASE_RECORD"
                p0 = dealer = {}
            else:
                status = found.group(4)
                p0, dealer = block(raw, "party0_metrics"), block(raw, "dealer")
            rounds = {}
            for r in range(1, 5):
                value = p0.get(f"round{r}_logical", "")
                match = re.fullmatch(r"(\d+)_unique=(\d+)_dummy=(\d+)_repeat=(\d+)_fnv64=([0-9a-fA-F]+)", value)
                rounds[str(r)] = ({"logical": int(match.group(1)), "unique": int(match.group(2)),
                                   "dummy": int(match.group(3)), "repeat": int(match.group(4)),
                                   "edge_plan_fnv64": match.group(5)} if match else "NOT_MEASURED")
            records.append({"n": n, "K": k, "seed": seed, "iteration": label,
                            "controller_exit_code": proc.returncode, "status": status,
                            "attempt_wall_time_s": wall,
                            "party_metrics": {key: integer(p0, key) for key in (
                                "logical", "key_eval", "dcf_eval", "select_unique", "membership_slots",
                                "message_phases", "sent", "recv", "claimed_slots", "sampler_prf_words",
                                "online_time_us", "dcf_counters_enabled", "dcf_keygen_calls", "dcf_keygen_node_expansions",
                                "dcf_eval_calls", "dcf_eval_node_expansions", "bundle_open_us",
                                "bundle_claim_us", "raw_share_read_us", "raw_adapter_us",
                                "raw_adapter_eval_us", "raw_adapter_exchange_us", "forward_shuffle_us",
                                "forward_shuffle_exchange_us", "sampling_coin_exchange_us",
                                "membership_us", "membership_eval_us", "inverse_shuffle_us",
                                "inverse_shuffle_exchange_us", "status_coordination_us",
                                "process_slot_claim_us", "slot_lookup_us", "peak_rss_kb",
                                *(f"round{r}_{suffix}" for r in range(1, 5)
                                  for suffix in ("time_us", "eval_us", "exchange_us")))},
                            "per_round": rounds,
                            "dealer_metrics": {key: integer(dealer, key) for key in (
                                "party0_bundle_bytes", "party0_plaintext_bytes", "party0_ciphertext_bytes",
                                "slots_per_party", "generation_us", "seal_us", "delivery_us",
                                "offline_total_us", "peak_rss_kb", "dcf_keygen_calls",
                                "dcf_keygen_node_expansions", "dcf_counters_enabled")},
                            "raw_sha256": sha256(raw_path), "raw_file": raw_path.name})
    summaries = []
    party_metric_fields = (
        "logical", "key_eval", "dcf_eval", "select_unique", "membership_slots",
        "message_phases", "sent", "recv", "claimed_slots", "sampler_prf_words",
        "dcf_counters_enabled", "dcf_keygen_calls", "dcf_keygen_node_expansions",
        "dcf_eval_calls", "dcf_eval_node_expansions", "bundle_open_us",
        "bundle_claim_us", "raw_share_read_us", "raw_adapter_us",
        "raw_adapter_eval_us", "raw_adapter_exchange_us", "forward_shuffle_us",
        "forward_shuffle_exchange_us", "sampling_coin_exchange_us", "membership_us",
        "membership_eval_us", "inverse_shuffle_us", "inverse_shuffle_exchange_us",
        "status_coordination_us", "peak_rss_kb",
        *(f"round{r}_{suffix}" for r in range(1, 5)
          for suffix in ("time_us", "eval_us", "exchange_us")),
    )
    dealer_metric_fields = (
        "party0_bundle_bytes", "party0_plaintext_bytes", "party0_ciphertext_bytes",
        "slots_per_party", "generation_us", "seal_us", "delivery_us",
        "offline_total_us", "peak_rss_kb", "dcf_keygen_calls",
        "dcf_keygen_node_expansions", "dcf_counters_enabled",
    )
    for n, k, seed in profiles:
        group = [r for r in records if r["n"] == n and r["K"] == k and r["iteration"] != "warmup"]
        successful = [r for r in group if r["status"] == "SUCCESS" and r["controller_exit_code"] == 0]
        def field_values(rows, section, key):
            return [r[section][key] for r in rows if isinstance(r[section].get(key), int)]
        summaries.append({"n": n, "K": k, "input_seed": seed, "warmups": 1, "attempts": len(group),
                          "status_counts": {s: sum(r["status"] == s for r in group) for s in sorted({r["status"] for r in group})},
                          "all_attempt_wall_s": summarize([r["attempt_wall_time_s"] for r in group]),
                          "successful_wall_s": summarize([r["attempt_wall_time_s"] for r in successful]),
                          "successful_online_us": summarize(field_values(successful, "party_metrics", "online_time_us")),
                          "successful_offline_us": summarize(field_values(successful, "dealer_metrics", "offline_total_us")),
                          "successful_party_peak_rss_kb": summarize(field_values(successful, "party_metrics", "peak_rss_kb")),
                          "successful_dealer_peak_rss_kb": summarize(field_values(successful, "dealer_metrics", "peak_rss_kb")),
                          "successful_bundle_bytes_party": summarize(field_values(successful, "dealer_metrics", "party0_bundle_bytes")),
                          "successful_logical_comparisons": summarize(field_values(successful, "party_metrics", "logical")),
                          "successful_key_evals": summarize(field_values(successful, "party_metrics", "key_eval")),
                          "successful_dcf_evals_party": summarize(field_values(successful, "party_metrics", "dcf_eval")),
                          "successful_claimed_slots": summarize(field_values(successful, "party_metrics", "claimed_slots"))})
        summaries[-1]["successful_party_metric_ranges"] = {
            key: summarize(field_values(successful, "party_metrics", key))
            for key in party_metric_fields
        }
        summaries[-1]["successful_dealer_metric_ranges"] = {
            key: summarize(field_values(successful, "dealer_metrics", key))
            for key in dealer_metric_fields
        }
    report = {"label": "BMW16_DERIVED_SELECT_S15_LOOPBACK_PILOT_NOT_V4",
              "network": "local socketpair loopback; not LAN/WAN and not comparable to another revision",
              "input_output": "fixed raw scores per profile, fresh materials per attempt; parent TEST_ONLY checks oracle",
              "warmup_and_attempt_policy": "1 warm-up + 5 measured attempts; preserve aborts/errors and attempt cost",
              "binary_sha256_record": binary,
              "environment_sha256": sha256(args.out / "environment.txt"),
              "profiles": summaries, "runs": records}
    report_path = args.out / "pilot.json"
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"result": "RECORDED", "report": str(report_path),
                      "report_sha256": sha256(report_path), "profiles": summaries}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
