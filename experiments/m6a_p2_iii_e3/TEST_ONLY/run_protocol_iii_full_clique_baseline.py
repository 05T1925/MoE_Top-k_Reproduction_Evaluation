#!/usr/bin/env python3
"""Run the E17-seeded TEST_ONLY Protocol III full-clique TCP batch.

LAN/WAN rows require the matching E17-style qdisc calibration artifact. Output
paths are create-once and raw JSONL is append-only during a run.
"""

import argparse
import datetime as dt
import hashlib
import json
import os
import pathlib
import statistics
import subprocess
import sys
import time
import shutil


E17_PLAN_HASH = "ADD4011CA3EFE8B41E2D89DCE57C85C6A724AA659DDEE23466D99362C1D0F84"
SEEDS = {
    2: [
        2640535145453483992,
        9637301074933284394,
        16432516750059569485,
        11086338292375816240,
        3295764374691053742,
        15275769531814601273,
    ],
    8: [
        12787728726309049324,
        5109066531882058098,
        2862633102810662142,
        18036093308687145516,
        11362616487598262087,
        15042774087355271839,
    ],
}
E17_TARGETS = {
    "LAN": {"rtt_ms": 1.0, "mbps": 1000.0, "rtt_tolerance": 0.20},
    "WAN": {"rtt_ms": 50.0, "mbps": 100.0, "rtt_tolerance": 0.20},
}
PROFILE = None
CALIBRATION = None
CALIBRATION_VALID = False
CALIBRATION_INPUTS = {}
WORKTREE_ROOT = pathlib.Path(__file__).resolve().parents[3]
SOURCE_FILES = (
    "VFSS/CMakeLists.txt",
    "VFSS/include/moe_topk/protocol_iii_raw_score_mask.h",
    "VFSS/src/moe_topk/protocol_i_score_input.cpp",
    "VFSS/src/moe_topk/protocol_i_ucmp.cpp",
    "VFSS/src/moe_topk/protocol_iii_grank.cpp",
    "VFSS/src/moe_topk/protocol_iii_raw_score_mask.cpp",
    "VFSS/tests/moe_topk/protocol_iii_raw_score_mask_process_test.cpp",
    "experiments/m6a_p2_i_allpairs/TEST_ONLY/e11_network_calibrate.py",
)
METRICS = {
    "offline_time_ms": lambda row: row["offline_ns"] / 1e6,
    "offline_material_total_bits": lambda row: row["offline_material_total_bits"],
    "online_time_ms": lambda row: row["online_max_ns"] / 1e6,
    "total_time_ms": lambda row: row["total_ns"] / 1e6,
    "online_total_communication_bits": lambda row: row["wire_bytes"] * 8,
    "online_causal_rounds": lambda row: row["rounds"],
    "online_dcf_prg_calls": lambda row: row["dcf_prg"],
    "online_actual_comparison_edges": lambda row: row["comparison_edges"],
}
PARTY_METRICS = (
    ("P0_sent_bytes", lambda row: row["p0_sent_bytes"]),
    ("P0_received_bytes", lambda row: row["p0_received_bytes"]),
    ("P1_sent_bytes", lambda row: row["p1_sent_bytes"]),
    ("P1_received_bytes", lambda row: row["p1_received_bytes"]),
)


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as source:
        for block in iter(lambda: source.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest().upper()


def utc_now():
    return dt.datetime.now(dt.timezone.utc).isoformat(timespec="milliseconds")


def command_version(command):
    try:
        result = subprocess.run(command, text=True, capture_output=True, check=False, timeout=10)
        return (result.stdout or result.stderr).splitlines()[0]
    except Exception as error:
        return f"NOT_AVAILABLE: {error!r}"


def parse_output(text):
    lines = [line for line in text.splitlines() if line.startswith("F1_PROCESS_PASS ")]
    if len(lines) != 1:
        raise ValueError(f"expected one F1_PROCESS_PASS line, got {len(lines)}")
    row = {}
    for token in lines[0].split()[1:]:
        key, value = token.split("=", 1)
        row[key] = value
    int_fields = {
        "n", "k", "rounds", "logical_bits", "wire_bytes", "offline_bytes",
        "offline_ns", "online_p0_ns", "online_p1_ns", "online_max_ns",
        "offline_material_total_bits", "offline_material_score_bits",
        "offline_material_grank_bits", "offline_material_routing_bits",
        "total_ns", "peak_t_kib", "peak_p0_kib", "peak_p1_kib", "input_seed",
        "input_share_seed", "input_digest", "raw_dcf", "dpf_eval", "dcf_prg", "comparison_edges",
        "p0_sent_bytes", "p0_received_bytes", "p1_sent_bytes", "p1_received_bytes",
        "session", "fingerprint", "material_id",
    }
    for key in int_fields:
        row[key] = int(row[key])
    row["stage_wire_bytes"] = [int(x) for x in row["stage_wire"].split(",")]
    row["stage_receive_bytes"] = [int(x) for x in row["stage_receive"].split(",")]
    row["stage_offline_bytes"] = [int(x) for x in row["stage_offline"].split(",")]
    row["stage_dcf_prg_calls"] = [int(x) for x in row["stage_dcf_prg"].split(",")]
    if row["offline_material_status"] != "DERIVED_FROM_VALIDATED_FIXED_LAYOUT":
        raise ValueError("unexpected material provenance")
    if row["online_transport"] != "TCP_LOOPBACK" or row["rounds"] != 4:
        raise ValueError("online transport/causal-round contract mismatch")
    if row["stage_wire_bytes"] != row["stage_receive_bytes"]:
        raise ValueError("aggregate stage sent/received byte mismatch")
    if sum(row["stage_dcf_prg_calls"]) != row["dcf_prg"]:
        raise ValueError("stage DCF PRG total mismatch")
    if row["p0_sent_bytes"] != row["p1_received_bytes"] or row["p1_sent_bytes"] != row["p0_received_bytes"]:
        raise ValueError("party sent/received byte mismatch")
    if sum(row["stage_wire_bytes"]) != row["wire_bytes"]:
        raise ValueError("stage communication total mismatch")
    if sum(row["stage_offline_bytes"]) + 2 * 66 != row["offline_bytes"]:
        # Per-party serialized sections plus two 66-byte header/IPC envelopes.
        raise ValueError("offline bundle stage accounting mismatch")
    return row


def validate_material_diagnostic(path):
    rows = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.startswith("F1_MATERIAL_DIAGNOSTIC "):
            continue
        row = {}
        for token in line.split()[1:]:
            if token == "PASS":
                row["status"] = token
                continue
            key, value = token.split("=", 1)
            row[key] = int(value) if key != "status" else value
        required = {"n", "d", "score_payload_bytes_per_party",
                    "grank_payload_bytes_per_party", "routing_payload_bytes_per_party",
                    "actual_party0", "actual_party1", "bundle0", "bundle1", "status"}
        if set(row) != required or row["status"] != "PASS":
            raise SystemExit("invalid material diagnostic record")
        expected = (row["score_payload_bytes_per_party"] +
                    row["grank_payload_bytes_per_party"] +
                    row["routing_payload_bytes_per_party"])
        if (row["actual_party0"] != expected or row["actual_party1"] != expected or
                row["bundle0"] < expected or row["bundle1"] < expected or
                row["bundle0"] > 64 * 1024 * 1024 or
                row["bundle1"] > 64 * 1024 * 1024):
            raise SystemExit(f"actual material count mismatch for n={row['n']}")
        rows[row["n"]] = row
    if set(rows) != {128, 256}:
        raise SystemExit("expected independent material diagnostic for n=128/256")
    return rows


def run_one(binary, n, k, rep, seed, binary_hash, source_revision,
            source_manifest_hash, plan_hash):
    cmd = [str(binary), "--bench", str(n), str(k), str(seed)]
    started = utc_now()
    wall_start_ns = time.time_ns()
    process = subprocess.run(cmd, text=True, capture_output=True, check=False)
    wall_end_ns = time.time_ns()
    record = {
        "started_utc": started,
        "ended_utc": utc_now(),
        "wall_elapsed_ns": wall_end_ns - wall_start_ns,
        "profile": PROFILE,
        "implementation_label": "M6A-P2-III-E3-ALLPAIRS-RAW-SCORE-MASK-TEST_ONLY",
        "source_revision": source_revision,
        "source_manifest_sha256": source_manifest_hash,
        "binary_sha256": binary_hash,
        "input_plan_sha256": plan_hash,
        "e17_reference_input_plan_sha256": E17_PLAN_HASH,
        "network_calibration": CALIBRATION if CALIBRATION is not None else "NOT_APPLIED",
        "network_calibration_valid": CALIBRATION_VALID,
        "network_rtt_ms": (CALIBRATION["rtt_median_ms"]
                            if CALIBRATION is not None else "NOT_MEASURED"),
        "tcp_goodput_mbps": (CALIBRATION["throughput_mbps"]
                             if CALIBRATION is not None else "NOT_MEASURED"),
        "n": n,
        "k": k,
        "rep": rep,
        "role": "warmup" if rep == 0 else "formal",
        "input_seed": seed,
        "command": cmd,
        "exit_code": process.returncode,
        "stdout": process.stdout,
        "stderr": process.stderr,
    }
    if process.returncode != 0:
        record["status"] = "FAIL"
        return record
    try:
        metrics = parse_output(process.stdout)
    except Exception as error:  # Preserve a raw row for any parser rejection.
        record["status"] = "INVALID_OUTPUT"
        record["validation_error"] = repr(error)
        return record
    record["status"] = "PASS"
    record["metrics"] = metrics
    metric_status = ("MEASURED_ON_CALIBRATED_" + PROFILE
                     if CALIBRATION_VALID else "MEASURED_ON_LOOPBACK_NOT_COMPARABLE")
    record["nine_metric_status"] = {name: metric_status for name in METRICS}
    record["nine_metric_status"]["online_per_party_communication"] = metric_status
    record["nine_metric_status"]["offline_material_total_bits"] = "DERIVED_FROM_VALIDATED_FIXED_LAYOUT"
    record["per_party_communication"] = {
        "P0": {"sent_bytes": metrics["p0_sent_bytes"], "received_bytes": metrics["p0_received_bytes"]},
        "P1": {"sent_bytes": metrics["p1_sent_bytes"], "received_bytes": metrics["p1_received_bytes"]},
    }
    record["communication_stages"] = {
        name: {"sent_bytes_both_parties": metrics["stage_wire_bytes"][i],
              "received_bytes_both_parties": metrics["stage_receive_bytes"][i]}
        for i, name in enumerate(("carry", "sign", "grank", "dpf_routing"))
    }
    record["dcf_prg_stages"] = dict(zip(("score_adapter", "grank", "dpf_routing"), metrics["stage_dcf_prg_calls"]))
    record["n_raw_metrics"] = {name: METRICS[name](metrics) for name in METRICS}
    record["n_raw_metrics"]["online_per_party_communication"] = record["per_party_communication"]
    return record


def summarize(records):
    grouped = {}
    for record in records:
        if record["role"] != "formal" or record["status"] != "PASS":
            continue
        key = (record["n"], record["k"])
        grouped.setdefault(key, []).append(record)
    result = {}
    for (n, k), rows in grouped.items():
        if len(rows) != 5:
            raise RuntimeError(f"{n=},{k=}: expected five formal rows, got {len(rows)}")
        metrics = {}
        for name in METRICS:
            values = [row["n_raw_metrics"][name] for row in rows]
            metrics[name] = {
                "min": min(values),
                "median": statistics.median(values),
                "max": max(values),
            }
        party_communication = {}
        for name, getter in PARTY_METRICS:
            values = [getter(row["metrics"]) for row in rows]
            party_communication[name] = {
                "min": min(values),
                "median": statistics.median(values),
                "max": max(values),
            }
        metrics["online_per_party_communication"] = party_communication
        result[f"n{n}_k{k}"] = {
            "profile": PROFILE,
            "formal_repetitions": 5,
            "warmups_excluded": 1,
            "metrics": metrics,
            "lan_wan_comparable": CALIBRATION_VALID,
            "network_calibration": CALIBRATION,
        }
    return result


def main():
    global PROFILE, CALIBRATION, CALIBRATION_VALID, CALIBRATION_INPUTS
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", required=True, type=pathlib.Path)
    parser.add_argument("--output-dir", required=True, type=pathlib.Path)
    parser.add_argument("--source-revision", required=True)
    parser.add_argument("--profile", required=True, choices=("LAN", "WAN", "TCP_LOOPBACK_UNSHAPED"))
    parser.add_argument("--calibration-json", type=pathlib.Path)
    parser.add_argument("--qdisc-before", type=pathlib.Path)
    parser.add_argument("--profile-command", type=pathlib.Path)
    parser.add_argument("--material-diagnostic", type=pathlib.Path)
    args = parser.parse_args()
    PROFILE = args.profile
    binary = args.binary.resolve(strict=True)
    if not os.access(binary, os.X_OK):
        raise SystemExit(f"binary is not executable: {binary}")
    if PROFILE in E17_TARGETS:
        if not all((args.calibration_json, args.qdisc_before, args.profile_command,
                    args.material_diagnostic)):
            raise SystemExit("LAN/WAN profile requires calibration, qdisc, command, and material diagnostic evidence")
        for path in (args.calibration_json, args.qdisc_before, args.profile_command,
                     args.material_diagnostic):
            if not path.is_file():
                raise SystemExit(f"missing network evidence file: {path}")
        material_diagnostic = validate_material_diagnostic(args.material_diagnostic)
        CALIBRATION = json.loads(args.calibration_json.read_text(encoding="utf-8"))
        qdisc_before = args.qdisc_before.read_text(encoding="utf-8")
        command_text = args.profile_command.read_text(encoding="utf-8")
        target = E17_TARGETS[PROFILE]
        expected_qdisc = ("delay 499us rate 1gbit" if PROFILE == "LAN"
                          else "delay 25ms rate 100mbit")
        qdisc_forms = (("delay 498us rate 1Gbit", "delay 499us rate 1Gbit")
                       if PROFILE == "LAN" else ("delay 25ms rate 100Mbit",))
        rtt_ok = abs(CALIBRATION.get("rtt_median_ms", -1) - target["rtt_ms"]) <= target["rtt_ms"] * target["rtt_tolerance"]
        throughput_ok = 0.5 * target["mbps"] <= CALIBRATION.get("throughput_mbps", -1) <= 1.5 * target["mbps"]
        CALIBRATION_VALID = (
            CALIBRATION.get("profile") == PROFILE and
            CALIBRATION.get("topology") == "single WSL2 host, TCP 127.0.0.1 inside dedicated network namespace" and
            CALIBRATION.get("connection") == "same loopback qdisc as all P0/P1 protocol TCP sockets" and
            CALIBRATION.get("rtt_samples") == 20 and
            CALIBRATION.get("target_rtt_ms") == target["rtt_ms"] and
            CALIBRATION.get("target_mbps") == target["mbps"] and
            any(shape in qdisc_before for shape in qdisc_forms) and
            "unshare -n" in command_text and
            expected_qdisc in command_text.casefold() and rtt_ok and throughput_ok
        )
        if not CALIBRATION_VALID:
            raise SystemExit("profile calibration/qdisc did not meet the E17 acceptance envelope")
        CALIBRATION_INPUTS = {
            "calibration_json": str(args.calibration_json.resolve()),
            "calibration_sha256": sha256(args.calibration_json),
            "qdisc_before": str(args.qdisc_before.resolve()),
            "qdisc_before_sha256": sha256(args.qdisc_before),
            "profile_command": str(args.profile_command.resolve()),
            "profile_command_sha256": sha256(args.profile_command),
            "material_diagnostic": str(args.material_diagnostic.resolve()),
            "material_diagnostic_sha256": sha256(args.material_diagnostic),
            "material_diagnostic_rows": material_diagnostic,
        }
    else:
        if any((args.calibration_json, args.qdisc_before, args.profile_command,
                args.material_diagnostic)):
            raise SystemExit("unshaped profile must not claim calibration evidence")
        CALIBRATION = None
        CALIBRATION_VALID = False
    binary_hash = sha256(binary)
    args.output_dir.mkdir(parents=True, exist_ok=False)
    source_manifest = {
        "source_revision": args.source_revision,
        "implementation_label": "M6A-P2-III-E3-ALLPAIRS-RAW-SCORE-MASK-TEST_ONLY",
        "runner_path": str(pathlib.Path(__file__).resolve()),
        "runner_sha256": sha256(pathlib.Path(__file__).resolve()),
        "binary_path": str(binary),
        "binary_sha256": binary_hash,
        "source_files_sha256": {
            name: sha256(WORKTREE_ROOT / name) for name in SOURCE_FILES
        },
    }
    (args.output_dir / "source_manifest.json").write_text(
        json.dumps(source_manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    source_manifest_hash = sha256(args.output_dir / "source_manifest.json")
    if CALIBRATION_VALID:
        shutil.copyfile(args.calibration_json, args.output_dir / "network_calibration.json")
        shutil.copyfile(args.qdisc_before, args.output_dir / "qdisc_before.txt")
        shutil.copyfile(args.profile_command, args.output_dir / "profile_command.txt")
        shutil.copyfile(args.material_diagnostic,
                        args.output_dir / "material_diagnostic.txt")
    plan = {
        "source": "E17 bridge_final_v2 LAN/WAN JSONL seed rows",
        "source_original_input_plan_hash": E17_PLAN_HASH,
        "source_plan_file_status": "NOT_PRESENT; seed rows reconstructed from untouched JSONL records",
        "sampler": "std::mt19937_64(input_seed), uniform_int_distribution<int32_t>(-32*4096,+32*4096), cast to uint32 Q20.12 word",
        "repetitions": "rep0 warmup; rep1..rep5 formal",
        "seeds_by_k": {str(k): values for k, values in SEEDS.items()},
        "input_share_seed": "independent fresh OS seed generated by the TEST_ONLY controller after T exits",
        "input_seed_is_T_visible": False,
    }
    plan_bytes = json.dumps(plan, sort_keys=True, separators=(",", ":")).encode()
    plan_hash = hashlib.sha256(plan_bytes).hexdigest().upper()
    plan["reconstructed_plan_sha256"] = plan_hash
    (args.output_dir / "seed_plan.json").write_text(json.dumps(plan, indent=2) + "\n", encoding="utf-8")
    environment = {
        "created_utc": utc_now(),
        "profile": PROFILE,
        "formal_lan_wan_status": "PROFILE_CALIBRATED_BATCH_RUNNING" if CALIBRATION_VALID else "NOT_RUN",
        "network_calibration": CALIBRATION if CALIBRATION is not None else "NOT_MEASURED",
        "network_calibration_inputs": CALIBRATION_INPUTS,
        "qdisc_observed": command_version(["tc", "qdisc", "show", "dev", "lo"]),
        "binary": str(binary),
        "binary_sha256": binary_hash,
        "source_revision": args.source_revision,
        "source_manifest": source_manifest,
        "source_manifest_sha256": source_manifest_hash,
        "implementation_label": "M6A-P2-III-E3-ALLPAIRS-RAW-SCORE-MASK-TEST_ONLY",
        "python": sys.version,
        "platform": sys.platform,
        "platform_release": os.uname().release if hasattr(os, "uname") else "NOT_AVAILABLE",
        "kernel": command_version(["uname", "-a"]),
        "compiler": command_version(["g++", "--version"]),
        "cmake": command_version(["cmake", "--version"]),
        "cpu_count": os.cpu_count(),
        "meminfo": pathlib.Path("/proc/meminfo").read_text(encoding="ascii") if pathlib.Path("/proc/meminfo").exists() else "NOT_AVAILABLE",
        "cgroup_memory_max": pathlib.Path("/sys/fs/cgroup/memory.max").read_text().strip() if pathlib.Path("/sys/fs/cgroup/memory.max").exists() else "NOT_AVAILABLE",
        "cgroup_cpu_max": pathlib.Path("/sys/fs/cgroup/cpu.max").read_text().strip() if pathlib.Path("/sys/fs/cgroup/cpu.max").exists() else "NOT_AVAILABLE",
        "command_template": [str(binary), "--bench", "{n}", "{k}", "{input_seed}"],
        "build_type": "Release",
    }
    (args.output_dir / "environment.json").write_text(json.dumps(environment, indent=2) + "\n", encoding="utf-8")
    raw_path = args.output_dir / "raw_runs.jsonl"
    records = []
    seen_material_ids = set()
    with raw_path.open("x", encoding="utf-8", buffering=1) as raw:
        for n in (128, 256):
            for k in (2, 8):
                for rep, seed in enumerate(SEEDS[k]):
                    record = run_one(binary, n, k, rep, seed,
                                     binary_hash, args.source_revision,
                                     source_manifest_hash, plan_hash)
                    if record.get("status") == "PASS":
                        material_id = record["metrics"]["material_id"]
                        if material_id in seen_material_ids:
                            record["status"] = "REUSED_MATERIAL_ID"
                        else:
                            seen_material_ids.add(material_id)
                    records.append(record)
                    raw.write(json.dumps(record, sort_keys=True) + "\n")
                    if record["status"] != "PASS":
                        raise SystemExit(f"run failed; raw row preserved: n={n},k={k},rep={rep}")
                    print(f"{n=} {k=} {rep=} status=PASS online_ns={record['metrics']['online_max_ns']}", flush=True)
    if len(records) != 24:
        raise RuntimeError("expected 24 rows: 4 configurations x (1 warmup + 5 formal)")
    if CALIBRATION_VALID:
        qdisc_after = command_version(["tc", "qdisc", "show", "dev", "lo"])
        qdisc_forms = (("delay 498us rate 1Gbit", "delay 499us rate 1Gbit")
                       if PROFILE == "LAN" else ("delay 25ms rate 100Mbit",))
        if not any(shape in qdisc_after for shape in qdisc_forms):
            raise RuntimeError("loopback qdisc changed during the batch")
        (args.output_dir / "qdisc_after.txt").write_text(qdisc_after + "\n", encoding="utf-8")
    summary = summarize(records)
    (args.output_dir / "summary.json").write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if CALIBRATION_VALID:
        environment["formal_lan_wan_status"] = "24_ROWS_PASS_4_CONFIGS_1_WARMUP_PLUS_5_FORMAL"
        environment["accepted_rows"] = len(records)
        environment["formal_rows"] = 20
        (args.output_dir / "environment.json").write_text(
            json.dumps(environment, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    hash_names = ["seed_plan.json", "source_manifest.json", "environment.json",
                  "raw_runs.jsonl", "summary.json"]
    if CALIBRATION_VALID:
        hash_names.extend(("network_calibration.json", "qdisc_before.txt",
                           "qdisc_after.txt", "profile_command.txt",
                           "material_diagnostic.txt"))
    hashes = {name: sha256(args.output_dir / name) for name in hash_names}
    (args.output_dir / "SHA256SUMS.json").write_text(json.dumps(hashes, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({"rows": len(records), "summary": summary, "sha256": hashes}, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
