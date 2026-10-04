#!/usr/bin/env python3
"""E20 isolated, fresh-material Protocol I+AAV86 stream measurements."""

import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import signal
import statistics
import subprocess
import sys


SEEDS = (20201004, 20201005, 20201006, 20201007, 20201008, 20201009)
NINE = ("offline_time_ms", "offline_material_total_bits", "online_time_ms",
        "total_time_ms", "online_comm_total_bits", "online_comm_per_party_bits",
        "online_rounds", "online_dcf_length_doubling_prg_calls_total",
        "comparison_edges_total")


def sha(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def fields(line):
    out = {}
    for item in line.split()[1:]:
        key, value = item.split("=", 1)
        if key in ("active_by_round", "vertices_by_round", "ca_prg_by_round"):
            out[key] = [int(x) for x in value.split(",") if x]
        elif key == "transport":
            out[key] = value
        else:
            out[key] = int(value)
    return out


def snapshot():
    return {"meminfo": Path("/proc/meminfo").read_text().splitlines()[:5],
            "swaps": Path("/proc/swaps").read_text().splitlines(),
            "c_disk": subprocess.check_output(["df", "-B1", "/mnt/c"], text=True).splitlines()[-1],
            "pressure": Path("/proc/pressure/memory").read_text().splitlines()}


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--binary", type=Path, required=True)
    p.add_argument("--source-root", type=Path, required=True)
    p.add_argument("--output-dir", type=Path, required=True)
    p.add_argument("--profile", choices=("LAN", "WAN"), required=True)
    p.add_argument("--revision", required=True)
    p.add_argument("--r", type=int, nargs="+", default=[2, 3, 4, 5])
    args = p.parse_args()
    if os.geteuid() != 0 or any(r not in range(2, 6) for r in args.r):
        raise RuntimeError("root and r=2..5 required for per-party OS identity")
    out = args.output_dir.resolve()
    out.mkdir(parents=True, exist_ok=False)
    script_root = args.source_root.resolve()
    sources = ["VFSS/src/moe_topk/protocol_i_aav86_small.cpp",
               "VFSS/src/moe_topk/protocol_i_aav86_streamed_store.cpp",
               "VFSS/tests/moe_topk/protocol_i_aav86_small_e2e_test.cpp",
               "VFSS/CMakeLists.txt",
               "experiments/m6a_p2_i_allpairs/TEST_ONLY/e20_run_stream_matrix.py"]
    common = {"source_revision": args.revision, "binary_sha256": sha(args.binary),
              "source_sha256": {name: sha(script_root / name) for name in sources},
              "implementation": "E20_STREAM_AEAD_V1", "n": 1000, "K": 80,
              "topology": "same-host WSL2 T/P0/P1 exec; distinct OS UIDs; TCP in netem namespace",
              "rlimit_as_bytes_per_process": 1073741824}
    delay, rate, target_rtt, target_rate = {
        "LAN": ("0.5ms", "1000mbit", "1", "1000"),
        "WAN": ("25ms", "100mbit", "50", "100")}[args.profile]
    ns = f"m6a-e20-{args.profile.lower()}-{os.getpid()}"
    subprocess.run(["ip", "netns", "add", ns], check=True)
    try:
        subprocess.run(["ip", "netns", "exec", ns, "ip", "link", "set", "lo", "up"], check=True)
        subprocess.run(["ip", "netns", "exec", ns, "tc", "qdisc", "add", "dev", "lo",
                        "root", "netem", "delay", delay, "rate", rate], check=True)
        calibration = out / "calibration.json"
        cmd = ["ip", "netns", "exec", ns, "python3",
               str(script_root / "experiments/m6a_p2_i_allpairs/TEST_ONLY/e11_network_calibrate.py"),
               "--profile", args.profile, "--target-rtt-ms", target_rtt,
               "--target-mbps", target_rate, "--output", str(calibration)]
        (out / "calibration_command.json").write_text(json.dumps(cmd))
        subprocess.run(cmd, check=True, stdout=(out / "calibration.log").open("x", encoding="utf-8"))
        cal = json.loads(calibration.read_text())
        low, high, bwlow, bwhigh = {"LAN": (0.5, 5, 200, 1200),
                                    "WAN": (35, 70, 20, 110)}[args.profile]
        if not (low <= cal["rtt_median_ms"] <= high and
                bwlow <= cal["throughput_mbps"] <= bwhigh):
            raise RuntimeError("NETWORK_GATE calibration")
        records = []
        raw = out / "runs.jsonl"
        with raw.open("x", encoding="utf-8") as stream:
            for r in args.r:
                (out / f"pre_r{r}_resource.json").write_text(json.dumps(snapshot(), indent=2))
                for rep, seed in enumerate(SEEDS):
                    serial = (20 if args.profile == "LAN" else 21) * 100000 + r * 100 + rep
                    run_id = f"{args.profile}-n1000-k80-r{r}-rep{rep}"
                    log = out / f"{run_id}.log"
                    argv = ["ip", "netns", "exec", ns, "prlimit", "--as=1073741824:1073741824",
                            "--", str(args.binary), "bench-stream", "1000", "80", str(r),
                            str(seed), str(serial)]
                    env = dict(os.environ, MOE_TOPK_M6A_E11_TRANSPORT="tcp",
                               MOE_TOPK_M6A_E15_BENCH="1",
                               MOE_TOPK_M6A_E11_PUBLIC_PIVOT_SEED=str(200000 + 1000*r + rep))
                    with log.open("x", encoding="utf-8") as f:
                        proc = subprocess.Popen(argv, env=env, stdout=f, stderr=subprocess.STDOUT,
                                                start_new_session=True)
                        try:
                            code = proc.wait(timeout=300)
                        except subprocess.TimeoutExpired:
                            os.killpg(proc.pid, signal.SIGKILL)
                            proc.wait()
                            code = 124
                    record = dict(common, profile=args.profile, r=r, repetition=rep,
                                  warmup=(rep == 0), run_id=run_id, input_seed=seed,
                                  command=argv, env={k: env[k] for k in
                                  ("MOE_TOPK_M6A_E11_TRANSPORT", "MOE_TOPK_M6A_E15_BENCH",
                                   "MOE_TOPK_M6A_E11_PUBLIC_PIVOT_SEED")},
                                  exit_code=code, raw_log=str(log), raw_log_sha256=sha(log))
                    if code == 0:
                        lines = log.read_text().splitlines()
                        case = next(fields(x) for x in lines if x.startswith("E20_STREAM_CASE "))
                        ready = next(fields(x) for x in lines if x.startswith("E20_READY_MATERIAL "))
                        meta = next(fields(x) for x in lines if x.startswith("E12_BENCH_META "))
                        if (case["transport"] != "tcp" or case["n"] != 1000 or
                                case["k"] != 80 or case["r"] != r or meta["input_seed"] != seed or
                                case["p0_bytes"] != case["p1_received_bytes"] or
                                case["p1_bytes"] != case["p0_received_bytes"] or
                                case["rounds"] != 2*r+4 or case["t_exit"] or
                                case["p0_exit"] or case["p1_exit"] or
                                sum(case["active_by_round"]) != case["active"]):
                            raise RuntimeError(f"counter conservation: {run_id}")
                        record.update(status="PASS", input_digest=meta["input_digest"],
                                      public_algorithm_seed=case["pivot_seed_lo"],
                                      network_rtt_median_ms=cal["rtt_median_ms"],
                                      network_throughput_mbps=cal["throughput_mbps"],
                                      correctness="full_entry_oracle_and_exact_K_PASS",
                                      offline_time_ms=case["offline_elapsed_ns"]/1e6,
                                      offline_material_total_bits=8*(ready["disk_plain_payload_per_party_bytes"]*2+
                                                                      ready["memory_payload_p0_bytes"]+
                                                                      ready["memory_payload_p1_bytes"]),
                                      online_time_ms=case["online_max_elapsed_ns"]/1e6,
                                      total_time_ms=(case["offline_elapsed_ns"]+
                                                     case["online_max_elapsed_ns"])/1e6,
                                      online_comm_total_bits=8*(case["p0_bytes"]+case["p1_bytes"]),
                                      online_comm_per_party_bits=8*case["p0_bytes"],
                                      online_rounds=case["rounds"],
                                      online_dcf_length_doubling_prg_calls_total=case["online_prg_total"],
                                      comparison_edges_total=case["active"],
                                      disk_ciphertext_per_party_bytes=case["store_disk_bytes_p0"],
                                      peak_t_kib=case["peak_t_kib"], peak_p0_kib=case["peak_p0_kib"],
                                      peak_p1_kib=case["peak_p1_kib"],
                                      active_edges_by_round=case["active_by_round"],
                                      active_vertices_by_round=case["vertices_by_round"],
                                      ca_prg_by_round_per_party=case["ca_prg_by_round"],
                                      p0_sent_bytes=case["p0_bytes"], p1_sent_bytes=case["p1_bytes"],
                                      p0_received_bytes=case["p0_received_bytes"],
                                      p1_received_bytes=case["p1_received_bytes"])
                    else:
                        record.update(status="FAILED", reason=f"exit {code}")
                    stream.write(json.dumps(record, sort_keys=True)+"\n")
                    stream.flush()
                    os.fsync(stream.fileno())
                    print(run_id, record["status"], flush=True)
                    if code != 0:
                        raise RuntimeError(f"failed {run_id}; raw log retained")
                    records.append(record)
        summary = {}
        with (out / "nine_metrics.csv").open("x", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(("profile", "n", "K", "r", "metric", "min", "median", "max", "formal_runs"))
            for r in args.r:
                sample = [x for x in records if x["r"] == r and not x["warmup"]]
                if len(sample) != 5:
                    raise RuntimeError("missing five formal samples")
                summary[str(r)] = {}
                for key in NINE:
                    values = [x[key] for x in sample]
                    entry = {"min": min(values), "median": statistics.median(values),
                             "max": max(values)}
                    summary[str(r)][key] = entry
                    writer.writerow((args.profile, 1000, 80, r, key, *entry.values(), 5))
        (out / "summary.json").write_text(json.dumps({"common": common,
            "calibration_sha256": sha(calibration), "raw_sha256": sha(raw),
            "nine": summary}, indent=2, sort_keys=True)+"\n")
    finally:
        subprocess.run(["ip", "netns", "del", ns], check=False)


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(f"E20_MATRIX_FAIL {exc!r}", file=sys.stderr)
        raise
