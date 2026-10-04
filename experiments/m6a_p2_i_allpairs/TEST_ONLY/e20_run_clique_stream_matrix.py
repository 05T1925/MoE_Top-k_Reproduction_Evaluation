#!/usr/bin/env python3
"""E20 n=1000 EMP-ON full-clique baseline, sealed offline material."""

import argparse
import csv
import json
import os
from pathlib import Path
import signal
import statistics
import subprocess
import sys
import time

from e20_run_stream_matrix import NINE, SEEDS, sha, fields, snapshot


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--binary", type=Path, required=True)
    p.add_argument("--source-root", type=Path, required=True)
    p.add_argument("--output-dir", type=Path, required=True)
    p.add_argument("--profile", choices=("LAN", "WAN"), required=True)
    p.add_argument("--revision", required=True)
    args = p.parse_args()
    if os.geteuid() != 0:
        raise RuntimeError("distinct party OS identities require root controller")
    root = args.source_root.resolve()
    out = args.output_dir.resolve()
    out.mkdir(parents=True, exist_ok=False)
    sources = ["VFSS/src/moe_topk/protocol_i_cmpagg.cpp",
               "VFSS/src/moe_topk/protocol_i_pipeline.cpp",
               "VFSS/src/moe_topk/protocol_i_aav86_streamed_store.cpp",
               "VFSS/tests/moe_topk/protocol_i_e20_baseline_stream_bench_test.cpp",
               "VFSS/CMakeLists.txt",
               "experiments/m6a_p2_i_allpairs/TEST_ONLY/e20_run_clique_stream_matrix.py"]
    common = {"source_revision": args.revision, "binary_sha256": sha(args.binary),
              "source_sha256": {s: sha(root / s) for s in sources},
              "implementation": "E20_CLIQUE_SEALED_V1", "n": 1000, "K": 80,
              "rlimit_as_bytes_per_process": 1073741824,
              "topology": "same-host WSL2 T/P0/P1 exec; distinct OS UIDs; real EMP OT; TCP in netem namespace"}
    delay, rate, target_rtt, target_rate = {
        "LAN": ("0.5ms", "1000mbit", "1", "1000"),
        "WAN": ("25ms", "100mbit", "50", "100")}[args.profile]
    ns = f"m6a-e20-clique-{args.profile.lower()}-{os.getpid()}"
    subprocess.run(["ip", "netns", "add", ns], check=True)
    try:
        subprocess.run(["ip", "netns", "exec", ns, "ip", "link", "set", "lo", "up"], check=True)
        subprocess.run(["ip", "netns", "exec", ns, "tc", "qdisc", "add", "dev", "lo",
                        "root", "netem", "delay", delay, "rate", rate], check=True)
        calibration = out / "calibration.json"
        calibrate = ["ip", "netns", "exec", ns, "python3",
                     str(root / "experiments/m6a_p2_i_allpairs/TEST_ONLY/e11_network_calibrate.py"),
                     "--profile", args.profile, "--target-rtt-ms", target_rtt,
                     "--target-mbps", target_rate, "--output", str(calibration)]
        (out / "calibration_command.json").write_text(json.dumps(calibrate))
        with (out / "calibration.log").open("x", encoding="utf-8") as f:
            subprocess.run(calibrate, check=True, stdout=f, stderr=subprocess.STDOUT)
        cal = json.loads(calibration.read_text())
        low, high, bwlow, bwhigh = {"LAN": (0.5, 5, 200, 1200),
                                    "WAN": (35, 70, 20, 110)}[args.profile]
        if not (low <= cal["rtt_median_ms"] <= high and
                bwlow <= cal["throughput_mbps"] <= bwhigh):
            raise RuntimeError("NETWORK_GATE calibration")
        (out / "pre_resource.json").write_text(json.dumps(snapshot(), indent=2))
        records = []
        raw = out / "runs.jsonl"
        with raw.open("x", encoding="utf-8") as stream:
            for rep, seed in enumerate(SEEDS):
                serial = (22 if args.profile == "LAN" else 23)*100000 + rep
                run_id = f"{args.profile}-n1000-k80-clique-rep{rep}"
                log = out / f"{run_id}.log"
                cmd = ["ip", "netns", "exec", ns, "prlimit", "--as=1073741824:1073741824",
                       "--", str(args.binary), "bench-stream", "1000", "80", str(seed), str(serial)]
                env = dict(os.environ, MOE_TOPK_M6A_E15_BENCH="1",
                           MOE_TOPK_M2_E12_TRANSPORT="tcp")
                with log.open("x", encoding="utf-8") as f:
                    started_utc_ns=time.time_ns()
                    proc = subprocess.Popen(cmd, env=env, stdout=f, stderr=subprocess.STDOUT,
                                            start_new_session=True)
                    try:
                        code = proc.wait(timeout=300)
                    except subprocess.TimeoutExpired:
                        os.killpg(proc.pid,signal.SIGKILL)
                        proc.wait()
                        code = 124
                    finished_utc_ns=time.time_ns()
                record = dict(common, profile=args.profile, repetition=rep,
                              warmup=(rep==0), run_id=run_id, input_seed=seed,
                              command=cmd, env={"MOE_TOPK_M6A_E15_BENCH":"1",
                                                "MOE_TOPK_M2_E12_TRANSPORT":"tcp"},
                              exit_code=code, raw_log=str(log), raw_log_sha256=sha(log))
                record.update(started_utc_ns=started_utc_ns,
                              finished_utc_ns=finished_utc_ns)
                if code == 0:
                    case = next(fields(line) for line in log.read_text().splitlines()
                                if line.startswith("E12_BASELINE_CASE "))
                    if (case["n"] != 1000 or case["k"] != 80 or case["d"] != 1024 or
                            case["input_seed"] != seed or case["rounds"] != 8 or
                            case["edges"] != 523776 or case["t_exit"] or
                            case["p0_exit"] or case["p1_exit"] or
                            case["p0_sent_bytes"] != case["p1_received_bytes"] or
                            case["p1_sent_bytes"] != case["p0_received_bytes"] or
                            case["p0_ot_sent_bytes"] != case["p1_ot_received_bytes"] or
                            case["p1_ot_sent_bytes"] != case["p0_ot_received_bytes"]):
                        raise RuntimeError(f"baseline counter conservation {run_id}")
                    record.update(status="PASS", input_digest=case["input_digest"],
                                  correctness="full_entry_oracle_and_exact_K_PASS",
                                  network_rtt_median_ms=cal["rtt_median_ms"],
                                  network_throughput_mbps=cal["throughput_mbps"],
                                  offline_time_ms=case["offline_ns"]/1e6,
                                  offline_material_total_bits=8*sum(case[f"{party}_{part}_bytes"]
                                      for party in ("p0","p1") for part in ("t_payload","shuffle_payload")),
                                  online_time_ms=case["online_max_ns"]/1e6,
                                  total_time_ms=(case["offline_ns"]+case["online_max_ns"])/1e6,
                                  online_comm_total_bits=8*(case["p0_sent_bytes"]+case["p1_sent_bytes"]),
                                  online_comm_per_party_bits=8*case["p0_sent_bytes"],
                                  online_rounds=case["rounds"],
                                  online_dcf_length_doubling_prg_calls_total=sum(
                                      case[f"{party}_{stage}_dcf_prg"]
                                      for party in ("p0","p1") for stage in ("score","pipeline")),
                                  comparison_edges_total=case["edges"],
                                  disk_ciphertext_p0_bytes=case["p0_package_bytes"],
                                  disk_ciphertext_p1_bytes=case["p1_package_bytes"],
                                  peak_t_kib=case["peak_t_kib"],
                                  peak_p0_kib=case["peak_p0_kib"], peak_p1_kib=case["peak_p1_kib"],
                                  p0_sent_bytes=case["p0_sent_bytes"],
                                  p1_sent_bytes=case["p1_sent_bytes"],
                                  p0_received_bytes=case["p0_received_bytes"],
                                  p1_received_bytes=case["p1_received_bytes"],
                                  offline_ot_sent_total_bytes=case["p0_ot_sent_bytes"]+
                                                              case["p1_ot_sent_bytes"])
                else:
                    record.update(status="FAILED", reason=f"exit {code}")
                stream.write(json.dumps(record,sort_keys=True)+"\n")
                stream.flush()
                os.fsync(stream.fileno())
                print(run_id, record["status"], flush=True)
                if code != 0:
                    raise RuntimeError(f"failed {run_id}; raw log retained")
                records.append(record)
        with (out / "nine_metrics.csv").open("x", newline="") as f:
            writer=csv.writer(f)
            writer.writerow(("profile","n","K","route","metric","min","median","max","formal_runs"))
            summary={}
            for key in NINE:
                values=[x[key] for x in records if not x["warmup"]]
                entry={"min":min(values),"median":statistics.median(values),"max":max(values)}
                summary[key]=entry
                writer.writerow((args.profile,1000,80,"EMP_ON_CLIQUE",key,*entry.values(),5))
        (out / "summary.json").write_text(json.dumps({"common":common,
            "calibration_sha256":sha(calibration),"raw_sha256":sha(raw),
            "nine":summary},indent=2,sort_keys=True)+"\n")
    finally:
        subprocess.run(["ip", "netns", "del", ns], check=False)


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(f"E20_CLIQUE_MATRIX_FAIL {exc!r}", file=sys.stderr)
        raise
