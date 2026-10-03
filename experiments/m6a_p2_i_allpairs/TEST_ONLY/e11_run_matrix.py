#!/usr/bin/env python3
"""Run the bounded n=128 Protocol I+AAV86 TCP submatrix with fresh material."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import statistics
import subprocess
import sys


SOURCE_FILES = (
    "VFSS/include/moe_topk/protocol_i_aav86_small.h",
    "VFSS/src/moe_topk/protocol_i_aav86_small.cpp",
    "VFSS/ext/FSS/include/FSS/dcf.h",
    "VFSS/ext/FSS/dcf.cpp",
    "VFSS/tests/moe_topk/protocol_i_aav86_small_e2e_test.cpp",
    "VFSS/CMakeLists.txt",
    "experiments/m6a_p2_i_allpairs/TEST_ONLY/e11_network_calibrate.py",
    "experiments/m6a_p2_i_allpairs/TEST_ONLY/e11_run_matrix.py",
    "experiments/m6a_p2_i_allpairs/TEST_ONLY/e11_shaped_namespace.sh",
)
STAT_FIELDS = (
    "offline_time_ms", "online_time_ms", "total_time_ms",
    "score_p0_ms", "score_p1_ms", "core_p0_ms", "core_p1_ms",
    "inverse_p0_ms", "inverse_p1_ms", "online_comm_total_bits",
    "comparison_edges_total", "active_vertices_total", "online_prg_calls_total",
    "offline_material_total_bits", "peak_t_kib", "peak_p0_kib", "peak_p1_kib",
)


def sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def git(root, *args):
    marker = root / ".git"
    if marker.is_file():
        value = marker.read_text(encoding="utf-8").strip()
        if not value.startswith("gitdir: "):
            raise RuntimeError("unrecognized worktree gitdir marker")
        gitdir = value[len("gitdir: "):]
        if len(gitdir) >= 3 and gitdir[1:3] == ":/":
            gitdir = "/mnt/" + gitdir[0].lower() + "/" + gitdir[3:]
        gitdir = str(Path(gitdir).resolve())
    else:
        gitdir = str(marker.resolve())
    command = ("git", f"--git-dir={gitdir}", f"--work-tree={root}", *args)
    return subprocess.check_output(command, text=True).strip()


def parse_fields(line):
    fields = {}
    for word in line.split()[1:]:
        key, value = word.split("=", 1)
        if key in ("active_by_round", "vertices_by_round", "ca_prg_by_round"):
            fields[key] = [int(item) for item in value.split(",") if item]
        elif key == "transport":
            fields[key] = value
        else:
            fields[key] = int(value)
    return fields


def measured(case, meta, profile, calibration, common, repetition):
    r = case["r"]
    if case["transport"] != "tcp" or len(case["active_by_round"]) != r:
        raise RuntimeError("TCP transport or edge-array mismatch")
    if len(case["vertices_by_round"]) != r or len(case["ca_prg_by_round"]) != r:
        raise RuntimeError("vertex/PRG array mismatch")
    if sum(case["active_by_round"]) != case["active"] or \
            sum(case["vertices_by_round"]) != case["active_vertices"] or \
            sum(case["ca_prg_by_round"]) != case["ca_prg_per_party"]:
        raise RuntimeError("per-round totals mismatch")
    if case["p0_bytes"] != case["p1_received_bytes"] or \
            case["p1_bytes"] != case["p0_received_bytes"]:
        raise RuntimeError("cross-party communication mismatch")
    for party in ("p0", "p1"):
        if sum(case[f"{party}_{stage}_bytes"] for stage in ("score", "core", "inverse")) \
                != case[f"{party}_bytes"]:
            raise RuntimeError("send stage sum mismatch")
        if sum(case[f"{party}_{stage}_received_bytes"]
               for stage in ("score", "core", "inverse")) != case[f"{party}_received_bytes"]:
            raise RuntimeError("receive stage sum mismatch")
    if case["rounds"] != 2 * r + 4 or case["t_exit"] or case["p0_exit"] or case["p1_exit"]:
        raise RuntimeError("causal rounds or child exit mismatch")
    if case["online_prg_total"] != 2 * case["online_prg_per_party"]:
        raise RuntimeError("online PRG total mismatch")
    if case["score_eval_per_party"] + case["ca_eval_per_party"] \
            != case["total_eval_per_party"]:
        raise RuntimeError("DCF Eval stage sum mismatch")
    if case["active"] > case["reserved"]:
        raise RuntimeError("active edges exceed full pool")
    def ms(name):
        return case[name] / 1e6
    record = dict(common)
    record.update({
        "profile": profile, "repetition": repetition,
        "warmup": repetition == 0, "status": "PASS",
        "n": case["n"], "K": case["k"], "D": case["d"], "r": r,
        "input_seed": meta["input_seed"], "input_digest": meta["input_digest"],
        "public_algorithm_seed": case["pivot_seed_lo"],
        "public_pivot_seed_hi": case["pivot_seed_hi"],
        "network_rtt_median_ms": calibration["rtt_median_ms"],
        "network_throughput_mbps": calibration["throughput_mbps"],
        "correctness": "frozen_oracle_and_exact_K_PASS",
        "offline_generate_ms": ms("dealer_generate_ns"),
        "offline_serialize_ms": ms("dealer_serialize_ns"),
        "offline_distribute_ms": ms("dealer_distribute_ns"),
        "offline_receive_barrier_ms": ms("receive_barrier_ns"),
        "offline_time_ms": ms("offline_elapsed_ns"),
        "score_p0_ms": ms("p0_score_ns"), "score_p1_ms": ms("p1_score_ns"),
        "core_p0_ms": ms("p0_core_ns"), "core_p1_ms": ms("p1_core_ns"),
        "inverse_p0_ms": ms("p0_inverse_ns"),
        "inverse_p1_ms": ms("p1_inverse_ns"),
        "online_p0_ms": ms("online_p0_ns"), "online_p1_ms": ms("online_p1_ns"),
        "online_time_ms": ms("online_max_elapsed_ns"),
        "total_time_ms": ms("offline_elapsed_ns") + ms("online_max_elapsed_ns"),
        "offline_material_total_bits": 8 * (case["package_bytes_p0"] + case["package_bytes_p1"]),
        "package_bytes_p0": case["package_bytes_p0"],
        "package_bytes_p1": case["package_bytes_p1"],
        "reserved_slots_per_party": case["reserved"],
        "active_edges_by_round": case["active_by_round"],
        "active_vertices_by_round": case["vertices_by_round"],
        "ca_prg_by_round_per_party": case["ca_prg_by_round"],
        "comparison_edges_total": case["active"],
        "active_vertices_total": case["active_vertices"],
        "score_dcf_eval_per_party": case["score_eval_per_party"],
        "ca_dcf_eval_per_party": case["ca_eval_per_party"],
        "dcf_eval_per_party": case["total_eval_per_party"],
        "dcf_eval_total": 2 * case["total_eval_per_party"],
        "score_prg_per_party": case["score_prg_per_party"],
        "ca_prg_per_party": case["ca_prg_per_party"],
        "inverse_prg_per_party": case["inverse_prg_per_party"],
        "online_prg_calls_total": case["online_prg_total"],
        "other_aes_prg_breakdown": "NOT_MEASURED",
        "online_rounds": case["rounds"],
        "core_rounds": 2 * r + 1,
        "online_comm_total_bits": 8 * (case["p0_bytes"] + case["p1_bytes"]),
        "online_comm_per_party_bits": 4 * (case["p0_bytes"] + case["p1_bytes"]),
        "peak_t_kib": case["peak_t_kib"],
        "peak_p0_kib": case["peak_p0_kib"],
        "peak_p1_kib": case["peak_p1_kib"],
        "exit_t": case["t_exit"], "exit_p0": case["p0_exit"], "exit_p1": case["p1_exit"],
        "edge_digest": case["edge_digest"],
    })
    for party in ("p0", "p1"):
        record[f"{party}_sent_bytes"] = case[f"{party}_bytes"]
        record[f"{party}_received_bytes"] = case[f"{party}_received_bytes"]
        for stage in ("score", "core", "inverse"):
            record[f"{party}_{stage}_sent_bytes"] = case[f"{party}_{stage}_bytes"]
            record[f"{party}_{stage}_received_bytes"] = case[f"{party}_{stage}_received_bytes"]
    return record


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--profile", choices=("LAN", "WAN"), required=True)
    parser.add_argument("--calibration", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    root = args.source_root.resolve()
    if git(root, "status", "--porcelain=v1", "--untracked-files=no"):
        raise RuntimeError("formal runner requires clean tracked worktree")
    head = git(root, "rev-parse", "HEAD")
    source_hashes = {name: sha256(root / name) for name in SOURCE_FILES}
    binary_hash = sha256(args.binary)
    calibration = json.loads(args.calibration.read_text(encoding="utf-8"))
    if calibration["profile"] != args.profile:
        raise RuntimeError("network calibration profile mismatch")
    rtt_low, rtt_high, rate_low, rate_high = {
        "LAN": (0.5, 5.0, 200.0, 1200.0),
        "WAN": (35.0, 70.0, 20.0, 110.0),
    }[args.profile]
    if not (rtt_low <= calibration["rtt_median_ms"] <= rtt_high and
            rate_low <= calibration["throughput_mbps"] <= rate_high):
        raise RuntimeError("NETWORK_GATE calibration outside predeclared range")
    out = args.output_dir
    out.mkdir(parents=True, exist_ok=True)
    raw = out / f"{args.profile}_runs.jsonl"
    summary = out / f"{args.profile}_summary.json"
    if raw.exists() or summary.exists():
        raise RuntimeError("refusing to overwrite raw or summary")
    common = {
        "head": head, "tracked_state": "CLEAN", "source_sha256": source_hashes,
        "binary_sha256": binary_hash, "calibration_sha256": sha256(args.calibration),
        "implementation": "protocol_i_aav86_allpairs_e11",
        "topology": "one WSL2 host, T/P0/P1 fork+exec, TCP loopback in shaped netns",
        "rlimit_as_bytes": 768 * 1024 * 1024,
        "thread_count_per_party": 1,
        "input_distribution": "uniform integers [-131072,131072], inclusive",
        "time_boundary": "offline T startup through both party ready; online max of party secure-call times",
        "communication_layer": "framed application bytes, TCP/IP overhead NOT_MEASURED",
    }
    records = []
    with raw.open("x", encoding="utf-8") as stream:
        for k in (2, 8):
            for r in (2, 3, 4, 5):
                for repetition in range(6):
                    if git(root, "rev-parse", "HEAD") != head or \
                            git(root, "status", "--porcelain=v1", "--untracked-files=no"):
                        raise RuntimeError("source identity changed during batch")
                    input_seed = 0xE110000 + k * 100 + repetition
                    algorithm_seed = 0xA110000 + r * 1000 + repetition
                    serial = (1 if args.profile == "LAN" else 2) * 100000 + \
                        k * 10000 + r * 100 + repetition
                    command = [str(args.binary), "bench", "128", str(k), str(r),
                               str(input_seed), str(serial)]
                    env = dict(os.environ)
                    env["MOE_TOPK_M6A_E11_TRANSPORT"] = "tcp"
                    env["MOE_TOPK_M6A_E11_PUBLIC_PIVOT_SEED"] = str(algorithm_seed)
                    run_id = f"{args.profile}-n128-k{k}-r{r}-rep{repetition}"
                    log_path = out / f"{run_id}.log"
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
                            log_path.write_text(output, encoding="utf-8")
                            raise RuntimeError("bounded 60-second process timeout")
                        log_path.write_text(output, encoding="utf-8")
                        if process.returncode:
                            raise RuntimeError(f"child exit {process.returncode}")
                        case = next(parse_fields(line) for line in output.splitlines()
                                    if line.startswith("E7_E2E_CASE "))
                        meta = next(parse_fields(line) for line in output.splitlines()
                                    if line.startswith("E11_BENCH_META "))
                        if case["k"] != k or case["r"] != r or \
                                meta["input_seed"] != input_seed or \
                                case["pivot_seed_lo"] != algorithm_seed:
                            raise RuntimeError("seed/shape mismatch")
                        record = measured(case, meta, args.profile, calibration,
                                          common, repetition)
                    except Exception as error:
                        record = dict(common, profile=args.profile,
                                      run_id=run_id, K=k, r=r, repetition=repetition,
                                      status="FAILED", reason=repr(error))
                        if not log_path.exists():
                            log_path.write_text(repr(error) + "\n", encoding="utf-8")
                    record.update(run_id=run_id, command=command,
                                  input_seed=input_seed,
                                  public_algorithm_seed=algorithm_seed,
                                  raw_log=str(log_path), raw_log_sha256=sha256(log_path))
                    stream.write(json.dumps(record, sort_keys=True) + "\n")
                    stream.flush()
                    os.fsync(stream.fileno())
                    print(f"{run_id} {record['status']}", flush=True)
                    if record["status"] != "PASS":
                        raise RuntimeError(f"batch stopped after failed {run_id}")
                    records.append(record)
    statistics_by_config = {}
    for k in (2, 8):
        for r in (2, 3, 4, 5):
            sample = [entry for entry in records
                      if entry["K"] == k and entry["r"] == r and not entry["warmup"]]
            if len(sample) != 5:
                raise RuntimeError("missing five formal repetitions")
            statistics_by_config[f"n128-k{k}-r{r}"] = {
                "formal_runs": 5, "warmups_excluded": 1,
                **{field: {"median": statistics.median(item[field] for item in sample),
                           "min": min(item[field] for item in sample),
                           "max": max(item[field] for item in sample)}
                   for field in STAT_FIELDS},
            }
    summary.write_text(json.dumps({"head": head, "profile": args.profile,
                                   "raw_sha256": sha256(raw),
                                   "configurations": statistics_by_config},
                                  indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"E11_MATRIX_PASS profile={args.profile} runs={len(records)} "
          f"formal={len(records)-8} raw_sha256={sha256(raw)}")


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(f"E11_MATRIX_FAIL {error!r}", file=sys.stderr)
        raise
