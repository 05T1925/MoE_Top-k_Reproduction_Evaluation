#!/usr/bin/env python3
"""Fresh-material EMP Protocol I clique baseline under E12 TCP profiles."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import statistics
import subprocess
import sys


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest().upper()


def git(root, *args):
    marker = root / ".git"
    value = marker.read_text(encoding="utf-8").strip()
    if not value.startswith("gitdir: "):
        raise RuntimeError("worktree git marker")
    directory = value[8:]
    if len(directory) >= 3 and directory[1:3] == ":/":
        directory = "/mnt/" + directory[0].lower() + "/" + directory[3:]
    return subprocess.check_output(
        ["git", f"--git-dir={Path(directory).resolve()}",
         f"--work-tree={root}", *args], text=True).strip()


def fields(line):
    return {key: int(value) for key, value in
            (word.split("=", 1) for word in line.split()[1:])}


def measure(case, profile, calibration, common, repetition):
    n, k, d = case["n"], case["k"], case["d"]
    if n != 128 or d != 128 or k not in (2, 8) or case["rounds"] != 8:
        raise RuntimeError("baseline shape/round mismatch")
    if case["edges"] != d * (d - 1) // 2 or \
            case["dcf_eval_per_party"] != 4 * d + 2 * case["edges"]:
        raise RuntimeError("baseline comparison work mismatch")
    if any(case[f"{role}_exit"] for role in ("t", "p0", "p1")):
        raise RuntimeError("baseline role exit mismatch")
    for party, peer in (("p0", "p1"), ("p1", "p0")):
        if case[f"{party}_sent_bytes"] != case[f"{peer}_received_bytes"]:
            raise RuntimeError("baseline communication conservation")
        if case[f"{party}_score_ns"] + case[f"{party}_pipeline_ns"] != \
                case[f"online_{party}_ns"]:
            raise RuntimeError("baseline online timing conservation")
        if sum(case[f"{party}_{stage}_sent_bytes"] for stage in
               ("score", "forward", "cmpagg", "reveal", "reverse")) != \
                case[f"{party}_sent_bytes"]:
            raise RuntimeError("baseline stage send sum")
        if sum(case[f"{party}_{stage}_received_bytes"] for stage in
               ("score", "forward", "cmpagg", "reveal", "reverse")) != \
                case[f"{party}_received_bytes"]:
            raise RuntimeError("baseline stage receive sum")
        for stage in ("score", "forward", "cmpagg", "reveal", "reverse"):
            if case[f"{party}_{stage}_received_bytes"] != \
                    case[f"{peer}_{stage}_sent_bytes"]:
                raise RuntimeError("baseline cross-party stage mismatch")
    if case["offline_ns"] < sum(case[f"dealer_{stage}_ns"] for stage in
                                ("generate", "serialize", "distribute")):
        raise RuntimeError("baseline offline timing bounds")
    ms = lambda key: case[key] / 1e6
    result = dict(common, profile=profile, repetition=repetition,
                  warmup=repetition == 0, status="PASS", n=n, K=k, D=d,
                  input_seed=case["input_seed"], input_digest=case["input_digest"],
                  network_rtt_median_ms=calibration["rtt_median_ms"],
                  network_throughput_mbps=calibration["throughput_mbps"],
                  correctness="frozen_oracle_and_exact_K_PASS",
                  offline_time_ms=ms("offline_ns"),
                  transport_setup_ms=ms("transport_setup_ns"),
                  offline_generate_ms=ms("dealer_generate_ns"),
                  offline_serialize_ms=ms("dealer_serialize_ns"),
                  offline_distribute_ms=ms("dealer_distribute_ns"),
                  offline_receive_barrier_ms=ms("receive_barrier_ns"),
                  online_time_ms=ms("online_max_ns"),
                  online_p0_ms=ms("online_p0_ns"),
                  online_p1_ms=ms("online_p1_ns"),
                  score_p0_ms=ms("p0_score_ns"), score_p1_ms=ms("p1_score_ns"),
                  pipeline_p0_ms=ms("p0_pipeline_ns"),
                  pipeline_p1_ms=ms("p1_pipeline_ns"),
                  total_time_ms=ms("offline_ns") + ms("online_max_ns"),
                  package_bytes_p0=case["p0_package_bytes"],
                  package_bytes_p1=case["p1_package_bytes"],
                  offline_material_total_bits="NOT_MEASURED: OT material is party-local",
                  package_only_material_bits=8 * (case["p0_package_bytes"] +
                                                  case["p1_package_bytes"]),
                  comparison_edges_total=case["edges"],
                  active_vertices_total=d,
                  dcf_eval_per_party=case["dcf_eval_per_party"],
                  online_rounds=case["rounds"],
                  paper_core_time_ms="NOT_MEASURED",
                  online_comm_total_bits=8 * (case["p0_sent_bytes"] +
                                              case["p1_sent_bytes"]),
                  online_prg_calls_total="NOT_MEASURED",
                  other_aes_prg_breakdown="NOT_MEASURED",
                  peak_t_kib=case["peak_t_kib"],
                  peak_p0_kib=case["peak_p0_kib"],
                  peak_p1_kib=case["peak_p1_kib"])
    for party in ("p0", "p1"):
        result[f"{party}_sent_bytes"] = case[f"{party}_sent_bytes"]
        result[f"{party}_received_bytes"] = case[f"{party}_received_bytes"]
        for stage in ("score", "forward", "cmpagg", "reveal", "reverse"):
            for direction in ("sent", "received"):
                result[f"{party}_{stage}_{direction}_bytes"] = \
                    case[f"{party}_{stage}_{direction}_bytes"]
    return result


def main():
    parser = argparse.ArgumentParser()
    for name in ("binary", "source-root", "calibration", "output-dir", "aav86-raw"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--profile", choices=("LAN", "WAN"), required=True)
    args = parser.parse_args()
    root = args.source_root.resolve()
    head = git(root, "rev-parse", "HEAD")
    if git(root, "status", "--porcelain=v1", "--untracked-files=no"):
        raise RuntimeError("baseline formal runner requires clean tracked source")
    calibration = json.loads(args.calibration.read_text(encoding="utf-8"))
    if calibration["profile"] != args.profile:
        raise RuntimeError("baseline calibration profile")
    rtt_low, rtt_high, rate_low, rate_high = {
        "LAN": (0.5, 5.0, 200.0, 1200.0),
        "WAN": (35.0, 70.0, 20.0, 110.0),
    }[args.profile]
    if not (rtt_low <= calibration["rtt_median_ms"] <= rtt_high and
            rate_low <= calibration["throughput_mbps"] <= rate_high):
        raise RuntimeError("baseline NETWORK_GATE outside predeclared range")
    refs = [json.loads(line) for line in args.aav86_raw.read_text(encoding="utf-8").splitlines()]
    expected = {(row["K"], row["repetition"]): row["input_digest"] for row in refs}
    source_names = ("VFSS/tests/moe_topk/protocol_i_e12_baseline_bench_test.cpp",
                    "VFSS/src/moe_topk/protocol_i_pipeline.cpp",
                    "VFSS/src/moe_topk/protocol_i_score_input.cpp",
                    "VFSS/src/moe_topk/protocol_i_secret_shared_shuffle.cpp",
                    "VFSS/ext/FSS/dcf.cpp", "VFSS/CMakeLists.txt",
                    "experiments/m6a_p2_i_allpairs/TEST_ONLY/e12_run_clique_baseline.py",
                    "experiments/m6a_p2_i_allpairs/TEST_ONLY/e12_clique_shaped_namespace.sh",
                    "experiments/m6a_p2_i_allpairs/TEST_ONLY/e11_network_calibrate.py")
    common = dict(head=head, tracked_state="CLEAN",
                  source_sha256={name: sha256(root / name) for name in source_names},
                  matched_aav86_raw_sha256=sha256(args.aav86_raw),
                  binary_sha256=sha256(args.binary),
                  calibration_sha256=sha256(args.calibration),
                  implementation="protocol_i_full_clique_emp_e12",
                  topology="one WSL2 host, T/P0/P1 fork+exec, online TCP loopback in shaped netns",
                  measurement_contract="E12: TCP setup excluded; offline before all role forks to T exit and both ready; online max party secure call",
                  compiler_flags="Release -O3 -DNDEBUG; MOE_TOPK_ENABLE_EMP_OT=ON",
                  thread_count_per_party=1)
    out = args.output_dir
    out.mkdir(parents=True, exist_ok=True)
    raw = out / f"{args.profile}_baseline_runs.jsonl"
    summary = out / f"{args.profile}_baseline_summary.json"
    if raw.exists() or summary.exists():
        raise RuntimeError("refusing to overwrite baseline evidence")
    rows = []
    with raw.open("x", encoding="utf-8") as stream:
        for k in (2, 8):
            for repetition in range(6):
                if head != git(root, "rev-parse", "HEAD") or \
                        git(root, "status", "--porcelain=v1", "--untracked-files=no"):
                    raise RuntimeError("baseline source changed during batch")
                input_seed = 0xE110000 + k * 100 + repetition
                serial = (1 if args.profile == "LAN" else 2) * 1000000 + \
                    k * 10000 + repetition
                command = [str(args.binary), "bench", "128", str(k),
                           str(input_seed), str(serial)]
                env = dict(os.environ, MOE_TOPK_M2_E12_BENCH="1",
                           MOE_TOPK_M2_E12_TRANSPORT="tcp")
                run_id = f"{args.profile}-clique-n128-k{k}-rep{repetition}"
                log = out / f"{run_id}.log"
                try:
                    process = subprocess.Popen(command, env=env, text=True,
                                               stdout=subprocess.PIPE,
                                               stderr=subprocess.STDOUT,
                                               start_new_session=True)
                    try:
                        output, _ = process.communicate(timeout=60)
                    except subprocess.TimeoutExpired:
                        os.killpg(process.pid, signal.SIGKILL)
                        output, _ = process.communicate()
                        log.write_text(output, encoding="utf-8")
                        raise RuntimeError("bounded 60-second baseline timeout")
                    log.write_text(output, encoding="utf-8")
                    if process.returncode:
                        raise RuntimeError(f"baseline child exit {process.returncode}")
                    case = next(fields(line) for line in output.splitlines()
                                if line.startswith("E12_BASELINE_CASE "))
                    if case["input_seed"] != input_seed or \
                            case["input_digest"] != expected[k, repetition]:
                        raise RuntimeError("matched input digest mismatch")
                    row = measure(case, args.profile, calibration, common, repetition)
                except Exception as error:
                    row = dict(common, profile=args.profile, K=k, repetition=repetition,
                               status="FAILED", reason=repr(error))
                    if not log.exists():
                        log.write_text(repr(error) + "\n", encoding="utf-8")
                row.update(run_id=run_id, command=command, input_seed=input_seed,
                           raw_log=str(log), raw_log_sha256=sha256(log))
                stream.write(json.dumps(row, sort_keys=True) + "\n")
                stream.flush()
                os.fsync(stream.fileno())
                print(f"{run_id} {row['status']}", flush=True)
                if row["status"] != "PASS":
                    raise RuntimeError(f"baseline stopped after {run_id}")
                rows.append(row)
    stats = {}
    numeric = ("offline_time_ms", "online_time_ms", "total_time_ms",
               "score_p0_ms", "score_p1_ms", "pipeline_p0_ms", "pipeline_p1_ms",
               "online_comm_total_bits", "peak_t_kib", "peak_p0_kib", "peak_p1_kib")
    for k in (2, 8):
        formal = [row for row in rows if row["K"] == k and not row["warmup"]]
        if len(formal) != 5:
            raise RuntimeError("baseline missing five formal repetitions")
        stats[f"n128-k{k}"] = dict(formal_runs=5, warmups_excluded=1,
                                   matched_aav86_r=[2, 3, 4, 5],
                                   **{name: dict(median=statistics.median(row[name] for row in formal),
                                                 min=min(row[name] for row in formal),
                                                 max=max(row[name] for row in formal))
                                      for name in numeric})
    summary.write_text(json.dumps(dict(head=head, profile=args.profile,
                                       raw_sha256=sha256(raw), configurations=stats),
                                  indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"E12_BASELINE_MATRIX_PASS profile={args.profile} runs={len(rows)} "
          f"formal={len(rows)-2} raw_sha256={sha256(raw)}")


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(f"E12_BASELINE_MATRIX_FAIL {error!r}", file=sys.stderr)
        raise
