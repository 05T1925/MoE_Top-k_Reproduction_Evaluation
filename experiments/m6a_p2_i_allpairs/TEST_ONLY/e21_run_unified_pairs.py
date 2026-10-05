#!/usr/bin/env python3
"""E21 matched full-clique/AAV86 runs under one measured network profile."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import statistics
import subprocess
import time

from e20_run_stream_matrix import NINE, SEEDS, fields, snapshot


SHAPES = ((128, 2), (128, 8), (256, 2), (256, 8), (1000, 80))
LABELS = {"clique": "E21_CLIQUE_MINIMAL_SEALED_V1",
          "aav": "E21_AAV86_STREAM_AEAD_V1"}


def sha(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def run_one(args, namespace, profile, calibration, n, k, route, r, rep, seed, stream):
    serial = (53 if profile == "LAN" else 54) * 1000000000 + n * 100000 + k * 1000 + r * 100 + rep
    run_id = f"{profile}-n{n}-k{k}-{route}-r{r}-rep{rep}"
    log = args.output_dir / f"{run_id}.log"
    command = ["ip", "netns", "exec", namespace, "prlimit",
               "--as=1073741824:1073741824", "--", str(args.clique_binary if route == "clique" else args.aav_binary)]
    if route == "clique":
        command += ["bench-minimal", str(n), str(k), str(seed), str(serial)]
    else:
        command += ["bench-stream", str(n), str(k), str(r), str(seed), str(serial)]
    environment = {"MOE_TOPK_M6A_E15_BENCH": "1"}
    if route == "clique":
        environment["MOE_TOPK_M2_E12_TRANSPORT"] = "tcp"
    else:
        environment.update(MOE_TOPK_M6A_E11_TRANSPORT="tcp",
                           MOE_TOPK_M6A_E11_PUBLIC_PIVOT_SEED=str(200000 + 1000*r + rep))
    resource_before = snapshot()
    with log.open("x", encoding="utf-8") as file:
        start = time.time_ns()
        proc = subprocess.Popen(command, env=dict(os.environ, **environment), stdout=file,
                                stderr=subprocess.STDOUT, start_new_session=True)
        try:
            exit_code = proc.wait(timeout=300)
        except subprocess.TimeoutExpired:
            os.killpg(proc.pid, signal.SIGKILL)
            proc.wait()
            exit_code = 124
        finish = time.time_ns()
    row = dict(source_revision=args.revision, implementation=LABELS[route], route=route,
               binary_sha256=sha(args.clique_binary if route == "clique" else args.aav_binary),
               profile=profile, topology="same-host WSL2 T/P0/P1 exec, real EMP OT, TCP in one netem namespace",
               n=n, K=k, r=(None if route == "clique" else r), repetition=rep,
               warmup=(rep == 0), input_seed=seed, serial=serial, run_id=run_id,
               command=command, env=environment, rlimit_as_bytes_per_process=1073741824,
               started_utc_ns=start, finished_utc_ns=finish, exit_code=exit_code,
               raw_log=str(log), raw_log_sha256=sha(log), calibration_sha256=sha(calibration),
               source_sha256=args.source_sha256, pre_resource=resource_before)
    if exit_code == 0:
        lines = log.read_text().splitlines()
        if route == "clique":
            case = next(fields(x) for x in lines if x.startswith("E12_BASELINE_CASE "))
            d = 1 << (max(2, n) - 1).bit_length()
            if (case["n"] != n or case["k"] != k or case["d"] != d or
                    case["input_seed"] != seed or case["edges"] != d*(d-1)//2 or
                    case["rounds"] != 8 or any(case[f"{p}_exit"] for p in ("t", "p0", "p1")) or
                    case["p0_sent_bytes"] != case["p1_received_bytes"] or
                    case["p1_sent_bytes"] != case["p0_received_bytes"] or
                    case["p0_ot_sent_bytes"] != case["p1_ot_received_bytes"] or
                    case["p1_ot_sent_bytes"] != case["p0_ot_received_bytes"]):
                raise RuntimeError(f"clique conservation: {run_id}")
            row.update(input_digest=case["input_digest"],
                       offline_time_ms=case["offline_ns"]/1e6,
                       offline_material_total_bits=8*sum(case[f"{p}_{part}_bytes"]
                           for p in ("p0", "p1") for part in ("t_payload", "shuffle_payload")),
                       online_time_ms=case["online_max_ns"]/1e6,
                       total_time_ms=(case["offline_ns"]+case["online_max_ns"])/1e6,
                       online_comm_total_bits=8*(case["p0_sent_bytes"]+case["p1_sent_bytes"]),
                       online_comm_per_party_bits=8*case["p0_sent_bytes"],
                       online_rounds=case["rounds"],
                       online_dcf_length_doubling_prg_calls_total=sum(case[f"{p}_{stage}_dcf_prg"]
                           for p in ("p0", "p1") for stage in ("score", "pipeline")),
                       comparison_edges_total=case["edges"],
                       offline_t_to_p0_bytes=case["t_to_p0_bytes"],
                       offline_t_to_p1_bytes=case["t_to_p1_bytes"],
                       offline_ot_sent_total_bytes=case["p0_ot_sent_bytes"]+case["p1_ot_sent_bytes"],
                       disk_ciphertext_p0_bytes=case["p0_package_bytes"],
                       disk_ciphertext_p1_bytes=case["p1_package_bytes"],
                       peak_t_kib=case["peak_t_kib"], peak_p0_kib=case["peak_p0_kib"],
                       peak_p1_kib=case["peak_p1_kib"],
                       p0_sent_bytes=case["p0_sent_bytes"], p1_sent_bytes=case["p1_sent_bytes"],
                       p0_received_bytes=case["p0_received_bytes"],
                       p1_received_bytes=case["p1_received_bytes"])
        else:
            case = next(fields(x) for x in lines if x.startswith("E20_STREAM_CASE "))
            ready = next(fields(x) for x in lines if x.startswith("E20_READY_MATERIAL "))
            meta = next(fields(x) for x in lines if x.startswith("E12_BENCH_META "))
            if (case["transport"] != "tcp" or case["n"] != n or case["k"] != k or
                    case["r"] != r or meta["input_seed"] != seed or case["rounds"] != 2*r+4 or
                    any(case[f"{p}_exit"] for p in ("t", "p0", "p1")) or
                    case["p0_bytes"] != case["p1_received_bytes"] or
                    case["p1_bytes"] != case["p0_received_bytes"] or
                    sum(case["active_by_round"]) != case["active"]):
                raise RuntimeError(f"AAV conservation: {run_id}")
            row.update(input_digest=meta["input_digest"],
                       public_algorithm_seed=case["pivot_seed_lo"],
                       offline_time_ms=case["offline_elapsed_ns"]/1e6,
                       offline_material_total_bits=8*(2*ready["disk_plain_payload_per_party_bytes"]+
                           ready["memory_payload_p0_bytes"]+ready["memory_payload_p1_bytes"]),
                       online_time_ms=case["online_max_elapsed_ns"]/1e6,
                       total_time_ms=(case["offline_elapsed_ns"]+case["online_max_elapsed_ns"])/1e6,
                       online_comm_total_bits=8*(case["p0_bytes"]+case["p1_bytes"]),
                       online_comm_per_party_bits=8*case["p0_bytes"],
                       online_rounds=case["rounds"],
                       online_dcf_length_doubling_prg_calls_total=case["online_prg_total"],
                       comparison_edges_total=case["active"],
                       offline_t_to_p0_bytes=case["t_to_p0_bytes"],
                       offline_t_to_p1_bytes=case["t_to_p1_bytes"],
                       disk_ciphertext_p0_bytes=case["store_disk_bytes_p0"],
                       disk_ciphertext_p1_bytes=case["store_disk_bytes_p1"],
                       active_edges_by_round=case["active_by_round"],
                       active_vertices_by_round=case["vertices_by_round"],
                       ca_prg_by_round_per_party=case["ca_prg_by_round"],
                       peak_t_kib=case["peak_t_kib"], peak_p0_kib=case["peak_p0_kib"],
                       peak_p1_kib=case["peak_p1_kib"],
                       p0_sent_bytes=case["p0_bytes"], p1_sent_bytes=case["p1_bytes"],
                       p0_received_bytes=case["p0_received_bytes"],
                       p1_received_bytes=case["p1_received_bytes"])
        row.update(status="PASS", correctness="full_entry_oracle_and_exact_K_PASS")
    else:
        row.update(status="FAILED", reason=f"exit {exit_code}")
    row.update(network_rtt_median_ms=args.cal["rtt_median_ms"],
               network_throughput_mbps=args.cal["throughput_mbps"],
               post_resource=snapshot())
    stream.write(json.dumps(row, sort_keys=True) + "\n")
    stream.flush()
    os.fsync(stream.fileno())
    print(run_id, row["status"], flush=True)
    if row["status"] != "PASS":
        raise RuntimeError(f"failed run: {run_id}; retained complete batch")
    return row


def main():
    parser = argparse.ArgumentParser()
    for name in ("clique-binary", "aav-binary", "source-root", "output-dir"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--profile", choices=("LAN", "WAN"), required=True)
    parser.add_argument("--revision", required=True)
    parser.add_argument("--shapes", nargs="*", default=[f"{n}:{k}" for n, k in SHAPES])
    args = parser.parse_args()
    if os.geteuid() != 0:
        raise RuntimeError("root controller required for distinct party OS identities")
    shapes = [tuple(map(int, shape.split(":"))) for shape in args.shapes]
    if not shapes or any(shape not in SHAPES for shape in shapes) or len(set(shapes)) != len(shapes):
        raise RuntimeError("invalid/duplicate shape")
    args.output_dir = args.output_dir.resolve()
    args.output_dir.mkdir(parents=True, exist_ok=False)
    source_files = (
        "VFSS/include/moe_topk/protocol_i_aav86_streamed_store.h",
        "VFSS/src/moe_topk/protocol_i_aav86_streamed_store.cpp",
        "VFSS/src/moe_topk/protocol_i_cmpagg.cpp",
        "VFSS/src/moe_topk/protocol_i_pipeline.cpp",
        "VFSS/src/moe_topk/protocol_i_aav86_small.cpp",
        "VFSS/tests/moe_topk/protocol_i_e20_baseline_stream_bench_test.cpp",
        "VFSS/tests/moe_topk/protocol_i_aav86_small_e2e_test.cpp",
        "VFSS/CMakeLists.txt",
        "experiments/m6a_p2_i_allpairs/TEST_ONLY/e21_run_unified_pairs.py",
    )
    args.source_sha256 = {name: sha(args.source_root / name) for name in source_files}
    ns = f"m6a-e21-pair-{args.profile.lower()}-{os.getpid()}"
    delay, rate, target_rtt, target_rate = {
        "LAN": ("0.5ms", "1000mbit", "1", "1000"),
        "WAN": ("25ms", "100mbit", "50", "100")}[args.profile]
    subprocess.run(["ip", "netns", "add", ns], check=True)
    try:
        subprocess.run(["ip", "netns", "exec", ns, "ip", "link", "set", "lo", "up"], check=True)
        subprocess.run(["ip", "netns", "exec", ns, "tc", "qdisc", "add", "dev", "lo",
                        "root", "netem", "delay", delay, "rate", rate], check=True)
        calibration = args.output_dir / "calibration.json"
        command = ["ip", "netns", "exec", ns, "python3",
                   str(args.source_root / "experiments/m6a_p2_i_allpairs/TEST_ONLY/e11_network_calibrate.py"),
                   "--profile", args.profile, "--target-rtt-ms", target_rtt,
                   "--target-mbps", target_rate, "--output", str(calibration)]
        (args.output_dir / "calibration_command.json").write_text(json.dumps(command))
        with (args.output_dir / "calibration.log").open("x") as file:
            subprocess.run(command, check=True, stdout=file, stderr=subprocess.STDOUT)
        args.cal = json.loads(calibration.read_text())
        lo, hi, bwlo, bwhi = {"LAN": (0.5, 5, 200, 1200),
                              "WAN": (35, 70, 20, 110)}[args.profile]
        if not (lo <= args.cal["rtt_median_ms"] <= hi and
                bwlo <= args.cal["throughput_mbps"] <= bwhi):
            raise RuntimeError("NETWORK_GATE calibration")
        (args.output_dir / "pre_resource.json").write_text(json.dumps(snapshot(), indent=2))
        records = []
        with (args.output_dir / "runs.jsonl").open("x") as stream:
            for n, k in shapes:
                for rep, seed in enumerate(SEEDS):
                    group = [run_one(args, ns, args.profile, calibration, n, k, "clique", 0,
                                     rep, seed, stream)]
                    for r in range(2, 6):
                        group.append(run_one(args, ns, args.profile, calibration, n, k, "aav", r,
                                             rep, seed, stream))
                    if len({row["input_digest"] for row in group}) != 1:
                        raise RuntimeError(f"input digest mismatch n={n} k={k} rep={rep}")
                    if len({row["calibration_sha256"] for row in group}) != 1:
                        raise RuntimeError("network calibration mismatch")
                    records.extend(group)
        summary = []
        for n, k in shapes:
            for route, rs in (("clique", (None,)), ("aav", (2, 3, 4, 5))):
                for r in rs:
                    sample = [row for row in records if (row["n"], row["K"], row["route"], row["r"]) ==
                              (n, k, route, r) and not row["warmup"]]
                    if len(sample) != 5:
                        raise RuntimeError("incomplete formal group")
                    summary.append(dict(profile=args.profile, n=n, K=k, route=route, r=r,
                                        implementation=LABELS[route], status="MEASURED", formal_runs=5,
                                        nine={key: dict(min=min(v := [row[key] for row in sample]),
                                                        median=statistics.median(v), max=max(v))
                                              for key in NINE}))
        (args.output_dir / "summary.json").write_text(json.dumps(dict(
            revision=args.revision, calibration_sha256=sha(calibration),
            raw_sha256=sha(args.output_dir / "runs.jsonl"), groups=summary), indent=2) + "\n")
        (args.output_dir / "post_resource.json").write_text(json.dumps(snapshot(), indent=2))
    finally:
        subprocess.run(["ip", "netns", "del", ns], check=False)


if __name__ == "__main__":
    main()
