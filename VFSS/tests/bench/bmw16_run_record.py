#!/usr/bin/env python3
"""Collect/audit one TEST_ONLY conditional-v2 E2E attempt.

The evidence directory must be outside the source tree. This runner retains only
non-secret process logs and hashes; it deletes the temporary fixture containing
shares, keys, certificates, and encrypted sidecars after collection.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import re
import shutil
import subprocess
import sys
import uuid
from pathlib import Path
from typing import Any

SCHEMA_VERSION = "2.0"
PROTOCOL_TAG = "Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED"
METRIC_UNITS = {
    "offline_time_ms": "ms",
    "offline_material_total_bits": "bits",
    "online_time_ms": "ms",
    "online_comm_total_bits": "bits",
    "online_comm_per_party_bits": "bits",
    "online_rounds": "causal_application_rounds",
    "online_prg_calls_total": "length_doubling_PRG_calls",
    "comparison_edges_total": "executed_unordered_secure_comparison_edges",
    "total_time_ms": "ms",
}
MEASURED_METRICS = tuple(METRIC_UNITS)


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as src:
        for block in iter(lambda: src.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def parse_kv(line: str) -> dict[str, str]:
    return {m.group(1): m.group(2) for m in re.finditer(r"([A-Za-z0-9_]+)=([^\s]+)", line)}


def read_role_line(path: Path, role: str) -> str:
    if not path.is_file():
        raise ValueError(f"missing expected process log: {path.name}")
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    for line in reversed(lines):
        if line.startswith(f"role={role} "):
            return line
    raise ValueError(f"missing {role} metric line in {path.name}")


def uint(fields: dict[str, str], name: str) -> int:
    value = fields.get(name)
    if value is None or not value.isdigit():
        raise ValueError(f"missing/non-integer counter: {name}")
    return int(value)


def uint_array(fields: dict[str, str], name: str) -> list[int]:
    raw = fields.get(name, "")
    if not (raw.startswith("[") and raw.endswith("]")):
        raise ValueError(f"missing counter vector: {name}")
    values = [] if raw == "[]" else [int(x) for x in raw[1:-1].split(",")]
    if len(values) != 4 or any(x < 0 for x in values):
        raise ValueError(f"invalid four-round counter vector: {name}")
    return values


def parse_marker(output: str, marker: str) -> dict[str, str]:
    for line in output.splitlines():
        if line.startswith(marker + " "):
            return parse_kv(line)
    return {}


def host_info(scratch_path: Path, compiler: str, cmake: str, openssl: str) -> dict[str, Any]:
    cpu_model = "NOT_REPORTED"
    cpuinfo = Path("/proc/cpuinfo")
    if cpuinfo.exists():
        for line in cpuinfo.read_text(errors="replace").splitlines():
            if line.lower().startswith("model name"):
                cpu_model = line.split(":", 1)[1].strip()
                break
    mem_bytes = 0
    meminfo = Path("/proc/meminfo")
    if meminfo.exists():
        for line in meminfo.read_text(errors="replace").splitlines():
            if line.startswith("MemTotal:"):
                mem_bytes = int(line.split()[1]) * 1024
                break
    return {
        "os": platform.platform(), "kernel": platform.release(), "cpu_model": cpu_model,
        "vcpu": os.cpu_count() or 1, "ram_bytes": mem_bytes,
        "scratch_bytes": shutil.disk_usage(scratch_path).free,
        "compiler": compiler.strip(), "cmake": cmake.strip(), "openssl": openssl.strip(),
    }


def command_text(cmd: list[str]) -> str:
    import shlex
    return shlex.join(cmd)


def build_failed_record(args: argparse.Namespace, rc: int, stdout: str, stderr: str,
                        input_sha: str, fixture: str, evidence_dir: Path,
                        evidence_manifest_name: str, manifest_hash: str,
                        common: dict[str, Any]) -> dict[str, Any]:
    combined = stdout + "\n" + stderr
    if "tls_natural_algorithm_abort" in combined:
        status = "PEER_AGREED_ALGORITHM_ABORT"
    elif "tls_delivery_abort" in combined or "tls_replay" in combined:
        status = "MATERIAL_ERROR"
    elif "tls_online_failure" in combined or "tls_online_auth_abort" in combined or "tls_online_silent_timeout" in combined:
        status = "COMMUNICATION_ERROR"
    else:
        status = "ENGINEERING_ERROR"
    metrics = {name: None for name in MEASURED_METRICS}
    provenance = {name: "NOT_MEASURED" for name in MEASURED_METRICS}
    provenance.update({"network.rtt_ms": "NOT_MEASURED", "network.bandwidth_mbit_s": "NOT_MEASURED",
                       "network.netem_id": "NOT_MEASURED", "network.online_tls_wire_bytes": "NOT_MEASURED"})
    return {
        **common, "terminal_status": status,
        "correctness": {"oracle_checked": False, "oracle_equal": None, "mask_length": None,
                        "mask_binary": None, "mask_weight_k": None, "mask_published": False},
        "metrics": metrics, "metric_units": METRIC_UNITS, "metric_provenance": provenance,
        "counts": {"process_exit_code": rc, "fixture_path_was_preserved": bool(fixture)},
        "auxiliary": {"e2e_command": command_text(args.command_line), "failure_classification": status},
        "raw_evidence_manifest": evidence_manifest_name, "raw_record_sha256": manifest_hash,
    }


def audit_record(record_path: Path) -> list[str]:
    record = json.loads(record_path.read_text(encoding="utf-8"))
    errors: list[str] = []
    required = {
        "schema_version", "run_id", "attempt_type", "protocol_tag", "implementation_label",
        "source_commit", "binary_sha256", "build_options_sha256", "host", "network", "n", "k",
        "dataset_id", "input_sha256", "oracle_sha256", "oracle_implementation_sha256",
        "input_seed_id", "share_split_seed_id",
        "algorithm_randomness", "session_id", "terminal_status", "correctness", "metrics",
        "metric_units", "metric_provenance", "counts", "auxiliary", "raw_evidence_manifest",
        "raw_record_sha256",
    }
    missing = sorted(required - record.keys())
    if missing:
        return ["missing fields: " + ", ".join(missing)]
    extra = sorted(record.keys() - required)
    if extra:
        errors.append("unexpected top-level fields: " + ", ".join(extra))
    if record["schema_version"] != SCHEMA_VERSION or record["protocol_tag"] != PROTOCOL_TAG:
        errors.append("schema/protocol identity mismatch")
    if record["implementation_label"] != "conditional_secure_v2" or record["attempt_type"] != "EXPERIMENTAL_DIAGNOSTIC":
        errors.append("implementation/attempt label mismatch")
    if not (1 <= record["k"] <= record["n"] <= 1000):
        errors.append("invalid n/K admission")
    for name in ("source_commit", "binary_sha256", "build_options_sha256", "input_sha256",
                 "oracle_sha256", "oracle_implementation_sha256", "raw_record_sha256"):
        expected = 40 if name == "source_commit" else 64
        if not isinstance(record[name], str) or len(record[name]) != expected or not re.fullmatch(r"[0-9a-f]+", record[name]):
            errors.append(f"invalid hash identity: {name}")
    host_required = {"os", "kernel", "cpu_model", "vcpu", "ram_bytes", "scratch_bytes", "compiler", "cmake", "openssl"}
    if not host_required.issubset(record["host"]):
        errors.append("host metadata incomplete")
    network_required = {"mode", "rtt_ms", "bandwidth_mbit_s", "netem_id", "online_tls_wire_bytes"}
    if not network_required.issubset(record["network"]) or record["network"].get("mode") != "loopback_diagnostic":
        errors.append("network metadata incomplete or wrong mode")
    if record["network"].get("online_tls_wire_bytes", "missing") is not None:
        errors.append("unavailable TLS wire count must remain null")
    if record["terminal_status"] not in {"SUCCESS", "PEER_AGREED_ALGORITHM_ABORT", "MATERIAL_ERROR", "COMMUNICATION_ERROR", "ENGINEERING_ERROR", "PRECHECK_REJECTED"}:
        errors.append("unknown terminal status")
    if set(record["metrics"]) != set(METRIC_UNITS):
        errors.append("main metric field set mismatch")
    for name, unit in METRIC_UNITS.items():
        if record["metric_units"].get(name) != unit:
            errors.append(f"unit mismatch: {name}")
        value = record["metrics"].get(name)
        provenance = record["metric_provenance"].get(name)
        if (value is None) != (provenance == "NOT_MEASURED"):
            errors.append(f"null/provenance mismatch: {name}")
    for field, provenance in record["metric_provenance"].items():
        if provenance not in {"MEASURED", "CHECKED_CALLSITE_DERIVATION", "NOT_MEASURED"}:
            errors.append(f"unknown metric provenance: {field}")
    def check_null_provenance(value: Any, prefix: str) -> None:
        if isinstance(value, dict):
            for key, child in value.items():
                check_null_provenance(child, f"{prefix}.{key}" if prefix else key)
        elif value is None:
            if record["metric_provenance"].get(prefix) != "NOT_MEASURED":
                errors.append(f"null field lacks NOT_MEASURED provenance: {prefix}")
        elif isinstance(value, (int, float)) and not isinstance(value, bool):
            if record["metric_provenance"].get(prefix) == "NOT_MEASURED":
                errors.append(f"numeric field marked NOT_MEASURED: {prefix}")
    for section in ("metrics", "counts", "auxiliary", "network"):
        check_null_provenance(record.get(section, {}), "" if section == "metrics" else section)
    status = record["terminal_status"]
    correctness = record["correctness"]
    if status == "SUCCESS":
        if not (correctness["oracle_checked"] and correctness["oracle_equal"] is True and
                correctness["mask_binary"] is True and correctness["mask_weight_k"] is True and
                correctness["mask_length"] == record["n"] and correctness["mask_published"]):
            errors.append("SUCCESS does not carry complete oracle/mask checks")
    elif correctness["mask_published"]:
        errors.append("non-success attempt published a mask")
    m = record["metrics"]
    c = record["counts"]
    if m["online_comm_total_bits"] is not None:
        sent = (c.get("p0_sent_bytes"), c.get("p1_sent_bytes"))
        recv = (c.get("p0_received_bytes"), c.get("p1_received_bytes"))
        if any(x is None for x in (*sent, *recv)):
            errors.append("communication marked measured without both-party send/receive counters")
        else:
            if m["online_comm_total_bits"] != 8 * sum(sent):
                errors.append("online communication total is not sum(sent) × 8")
            if m["online_comm_per_party_bits"] != m["online_comm_total_bits"] // 2:
                errors.append("per-party communication is not scalar total/2")
            if sent[0] != recv[1] or sent[1] != recv[0]:
                errors.append("two-party sent/received counters do not reconcile")
    if m["comparison_edges_total"] is not None:
        edge_sum = (c.get("raw_score_ucmp_calls"), c.get("select_unique_real_edges"), c.get("membership_secure_edges"))
        if any(x is None for x in edge_sum):
            errors.append("comparison edge metric lacks its three counted components")
        elif m["comparison_edges_total"] != sum(edge_sum):
            errors.append("comparison_edges_total does not equal raw + Select unique + membership")
        padded_n = record.get("auxiliary", {}).get("padded_n")
        raw_real = record.get("auxiliary", {}).get("raw_score_real_ucmp_calls")
        raw_padding = record.get("auxiliary", {}).get("raw_score_padding_ucmp_calls")
        if any(value is None for value in (padded_n, raw_real, raw_padding)):
            errors.append("raw-score edge decomposition is incomplete")
        expected_padded_n = 1 << max(1, (record["n"] - 1).bit_length())
        if not any(value is None for value in (padded_n, raw_real, raw_padding)) and (
              padded_n != expected_padded_n or padded_n & (padded_n - 1) or
              raw_real != 2 * record["n"] or raw_padding != 2 * (padded_n - record["n"]) or
              raw_real + raw_padding != c.get("raw_score_ucmp_calls")):
            errors.append("raw-score real/padding edge decomposition mismatch")
    if m["online_prg_calls_total"] is not None:
        expansion_sum = c.get("p0_dcf_eval_node_expansions"), c.get("p1_dcf_eval_node_expansions")
        if any(x is None for x in expansion_sum):
            errors.append("PRG calls marked measured without both-party DCF expansion counters")
        elif m["online_prg_calls_total"] != sum(expansion_sum):
            errors.append("online PRG total does not equal summed DCF node expansion calls")
    if m["total_time_ms"] is not None:
        if m["offline_time_ms"] is None or m["online_time_ms"] is None:
            errors.append("total time measured while a component time is missing")
        elif abs(m["total_time_ms"] - m["offline_time_ms"] - m["online_time_ms"]) > 0.002:
            errors.append("same-attempt total time does not equal offline + online")
    manifest_name = Path(record["raw_evidence_manifest"])
    manifest_path = record_path.parent / manifest_name
    if (manifest_name.name != str(manifest_name) or manifest_name.is_absolute() or
            str(manifest_name) in {"", ".", ".."} or manifest_path.is_symlink()):
        errors.append("raw evidence manifest must be a record-local regular file")
    elif not manifest_path.is_file():
        errors.append("raw evidence manifest missing")
    elif sha256_file(manifest_path) != record["raw_record_sha256"]:
        errors.append("raw_record_sha256 is not SHA-256 of the evidence manifest bytes")
    else:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        for item in manifest.get("files", []):
            relative = Path(item.get("path", ""))
            if relative.name != str(relative) or relative.is_absolute() or str(relative) in {"", ".", ".."}:
                errors.append(f"evidence path is not a record-local filename: {item.get('path')}")
                continue
            path = record_path.parent / relative
            if path.is_symlink() or not path.is_file() or sha256_file(path) != item["sha256"]:
                errors.append(f"evidence file hash mismatch: {item['path']}")
        stdout_path = record_path.parent / "e2e_stdout.txt"
        if stdout_path.is_file():
            output = stdout_path.read_text(encoding="utf-8", errors="replace")
            fixture = parse_marker(output, "benchmark_fixture")
            if fixture:
                for key, expected in (("n", record["n"]), ("K", record["k"]),
                                      ("session", record["session_id"])):
                    if fixture.get(key) != str(expected):
                        errors.append(f"fixture identity mismatch: {key}")
                if fixture.get("input_sha256") != record["input_sha256"]:
                    errors.append("fixture input hash mismatch")
                seed_id = record["input_seed_id"]
                if seed_id.startswith("mt19937_64:") and fixture.get("input_seed_id") != seed_id.split(":", 1)[1]:
                    errors.append("fixture input seed mismatch")
                if fixture.get("oracle_mask_sha256") != record["oracle_sha256"]:
                    errors.append("fixture oracle mask hash mismatch")
            attempt = parse_marker(output, "benchmark_attempt")
            if attempt:
                for key, expected in (("n", record["n"]), ("K", record["k"]),
                                      ("session", record["session_id"])):
                    if attempt.get(key) != str(expected):
                        errors.append(f"attempt identity mismatch: {key}")
                if attempt.get("input_sha256") != record["input_sha256"]:
                    errors.append("attempt input hash mismatch")
                if attempt.get("oracle_mask_sha256") != record["oracle_sha256"]:
                    errors.append("attempt oracle mask hash mismatch")
                if record["terminal_status"] == "SUCCESS" and attempt.get("status") != "SUCCESS_ORACLE_CHECKED":
                    errors.append("SUCCESS has no oracle-checked E2E marker")
    return errors


def collect(args: argparse.Namespace) -> int:
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    e2e = Path(args.e2e_test).resolve()
    node = Path(args.party_node).resolve()
    oracle = Path(args.oracle_source).resolve()
    cmake_cache = Path(args.cmake_cache).resolve()
    binary_hash = sha256_file(node)
    build_options_hash = sha256_file(cmake_cache)
    oracle_implementation_hash = sha256_file(oracle)
    input_seed = str(args.input_seed)
    session = str(args.session)
    command = [str(e2e), str(node), "--conditional-secure-v2-case", str(args.n), str(args.k), input_seed, session]
    args.command_line = command
    env = os.environ.copy()
    env["MOE_BMW16_KEEP_TEST_FIXTURES"] = "1"
    env["OMP_NUM_THREADS"] = str(args.threads)
    proc = subprocess.run(command, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          timeout=args.timeout_seconds, check=False)
    stdout, stderr = proc.stdout, proc.stderr
    (out_dir / "e2e_stdout.txt").write_text(stdout, encoding="utf-8")
    (out_dir / "e2e_stderr.txt").write_text(stderr, encoding="utf-8")
    fixture = ""
    marker = parse_marker(stdout, "benchmark_attempt")
    fixture = marker.get("fixture", "")
    if not fixture:
        match = re.search(r"fixture_preserved=([^\s]+)", stderr)
        if match:
            fixture = match.group(1)
    fixture_path = Path(fixture) if fixture else None
    safe_files: list[Path] = [out_dir / "e2e_stdout.txt", out_dir / "e2e_stderr.txt"]
    if fixture_path and fixture_path.is_dir():
        for relative in ("t/t.log", "p0/receive.log", "p1/receive.log", "p0/online.log", "p1/online.log", "p0/replay.log", "p1/replay.log"):
            src = fixture_path / relative
            if src.is_file():
                dst = out_dir / ("party0_" + src.name if relative.startswith("p0/") else
                                 "party1_" + src.name if relative.startswith("p1/") else src.name)
                shutil.copyfile(src, dst)
                safe_files.append(dst)

    files = [{"path": f.name, "sha256": sha256_file(f), "bytes": f.stat().st_size}
             for f in sorted(safe_files, key=lambda p: p.name)]
    manifest_name = "evidence_manifest.json"
    manifest_bytes = (json.dumps({"manifest_version": 1, "files": files}, sort_keys=True,
                                 separators=(",", ":")) + "\n").encode()
    (out_dir / manifest_name).write_bytes(manifest_bytes)
    manifest_hash = sha256_bytes(manifest_bytes)
    cmake_text = cmake_cache.read_text(errors="replace")
    compiler = subprocess.run(["g++", "--version"], text=True, stdout=subprocess.PIPE, check=False).stdout.splitlines()[0]
    cmake_version = subprocess.run(["cmake", "--version"], text=True, stdout=subprocess.PIPE, check=False).stdout.splitlines()[0]
    openssl_version = subprocess.run(["openssl", "version"], text=True, stdout=subprocess.PIPE, check=False).stdout.strip()
    host = host_info(out_dir, compiler, cmake_version, openssl_version)
    input_marker = parse_marker(stdout, "benchmark_fixture")
    if not re.fullmatch(r"[0-9a-f]{64}", input_marker.get("input_sha256", "")):
        raise ValueError("TEST_ONLY E2E did not emit a canonical input hash; refusing to invent one")
    if not re.fullmatch(r"[0-9a-f]{64}", input_marker.get("oracle_mask_sha256", "")):
        raise ValueError("TEST_ONLY E2E did not emit a canonical oracle-mask hash; refusing to invent one")
    expected_fixture = {"n": str(args.n), "K": str(args.k), "session": session,
                        "input_seed_id": input_seed}
    for key, expected in expected_fixture.items():
        if input_marker.get(key) != expected:
            raise ValueError(f"TEST_ONLY fixture identity mismatch: {key}")
    common: dict[str, Any] = {
        "schema_version": SCHEMA_VERSION, "run_id": str(uuid.uuid4()), "attempt_type": "EXPERIMENTAL_DIAGNOSTIC",
        "protocol_tag": PROTOCOL_TAG, "implementation_label": "conditional_secure_v2",
        "source_commit": args.source_commit, "binary_sha256": binary_hash,
        "build_options_sha256": build_options_hash, "host": host,
        "network": {"mode": "loopback_diagnostic", "rtt_ms": None, "bandwidth_mbit_s": None,
                    "netem_id": None, "online_tls_wire_bytes": None},
        "n": args.n, "k": args.k, "dataset_id": f"seeded_scores_v1:n={args.n}:seed={input_seed}",
        "input_sha256": input_marker["input_sha256"],
        "oracle_sha256": input_marker["oracle_mask_sha256"], "input_seed_id": f"mt19937_64:{input_seed}",
        "oracle_implementation_sha256": oracle_implementation_hash,
        "share_split_seed_id": f"TEST_ONLY_session_xor_0x534841524553:{int(session) ^ 0x534841524553}",
        "algorithm_randomness": "OS_CSPRNG_fresh_per_attempt", "session_id": session,
    }
    success = (proc.returncode == 0 and marker.get("status") == "SUCCESS_ORACLE_CHECKED")
    if not success:
        record = build_failed_record(args, proc.returncode, stdout, stderr, common["input_sha256"], fixture,
                                     out_dir, manifest_name, manifest_hash, common)
    else:
        t_fields = parse_kv(read_role_line(fixture_path / "t" / "t.log", "T"))  # type: ignore[operator]
        p0_fields = parse_kv(read_role_line(fixture_path / "p0" / "online.log", "P0"))  # type: ignore[operator]
        p1_fields = parse_kv(read_role_line(fixture_path / "p1" / "online.log", "P1"))  # type: ignore[operator]
        if p0_fields.get("dcf_eval_counter_matches_runtime") != "true" or p1_fields.get("dcf_eval_counter_matches_runtime") != "true":
            raise ValueError("per-party DCF Eval counter did not match runtime evaluation count")
        if p0_fields.get("dcf_counter_enabled") != "true" or p1_fields.get("dcf_counter_enabled") != "true":
            raise ValueError("diagnostic build lacks FSS DCF expansion counters")
        p0_logical = uint_array(p0_fields, "select_logical_calls_r1_r4")
        p1_logical = uint_array(p1_fields, "select_logical_calls_r1_r4")
        p0_unique = uint_array(p0_fields, "select_unique_slots_r1_r4")
        p1_unique = uint_array(p1_fields, "select_unique_slots_r1_r4")
        p0_dummy = uint_array(p0_fields, "select_dummy_calls_r1_r4")
        p1_dummy = uint_array(p1_fields, "select_dummy_calls_r1_r4")
        p0_repeats = uint_array(p0_fields, "select_repeated_calls_r1_r4")
        p1_repeats = uint_array(p1_fields, "select_repeated_calls_r1_r4")
        equal_fields = ("raw_adapter_ucmp_calls", "raw_adapter_dcf_evaluations", "online_message_phases",
                        "membership_slots_consumed", "process_slots_claimed", "logical_comparisons", "ucmp_eval", "dcf_eval")
        for field in equal_fields:
            if uint(p0_fields, field) != uint(p1_fields, field):
                raise ValueError(f"two parties disagree on count {field}")
        if (p0_logical, p0_unique, p0_dummy, p0_repeats) != (p1_logical, p1_unique, p1_dummy, p1_repeats):
            raise ValueError("two parties disagree on Select request/slot vectors")
        p0_sent, p0_recv = uint(p0_fields, "online_bytes_sent"), uint(p0_fields, "online_bytes_received")
        p1_sent, p1_recv = uint(p1_fields, "online_bytes_sent"), uint(p1_fields, "online_bytes_received")
        if p0_sent != p1_recv or p1_sent != p0_recv:
            raise ValueError("online application send/receive accounting does not reconcile")
        online_rounds = uint(p0_fields, "online_message_phases")
        if online_rounds != uint(p1_fields, "online_message_phases"):
            raise ValueError("party causal-phase counts differ")
        offline_us, online_us, total_us = (int(marker[x]) for x in ("offline_time_us", "online_time_us", "total_time_us"))
        if abs(total_us - offline_us - online_us) > 2:
            raise ValueError("wall-clock intervals do not reconcile within the 1-us timer resolution")
        payload0, payload1 = uint(t_fields, "material_payload_bytes_p0"), uint(t_fields, "material_payload_bytes_p1")
        sidecar = uint(t_fields, "sidecar_bytes_per_party")
        package_bytes = 2 * sidecar + uint(t_fields, "shell_bytes_p0") + uint(t_fields, "shell_bytes_p1")
        total_edges = (uint(p0_fields, "raw_adapter_ucmp_calls") + sum(p0_unique) +
                       uint(p0_fields, "membership_slots_consumed"))
        prg_total = uint(p0_fields, "dcf_eval_node_expansions") + uint(p1_fields, "dcf_eval_node_expansions")
        metrics = {
            "offline_time_ms": offline_us / 1000.0,
            "offline_material_total_bits": 8 * (payload0 + payload1),
            "online_time_ms": online_us / 1000.0,
            "online_comm_total_bits": 8 * (p0_sent + p1_sent),
            "online_comm_per_party_bits": 4 * (p0_sent + p1_sent),
            "online_rounds": online_rounds,
            "online_prg_calls_total": prg_total,
            "comparison_edges_total": total_edges,
            "total_time_ms": total_us / 1000.0,
        }
        counts = {
            "raw_score_ucmp_calls": uint(p0_fields, "raw_adapter_ucmp_calls"),
            "select_logical_calls_r1_r4": sum(p0_logical),
            "select_unique_real_edges": sum(p0_unique),
            "select_dummy_requests": sum(p0_dummy),
            "select_repeated_requests": sum(p0_repeats),
            "membership_secure_edges": uint(p0_fields, "membership_slots_consumed"),
            "unique_material_slots_per_party": uint(p0_fields, "process_slots_claimed"),
            "dcf_party_evaluations_p0": uint(p0_fields, "dcf_eval_calls_counted"),
            "dcf_party_evaluations_p1": uint(p1_fields, "dcf_eval_calls_counted"),
            "p0_dcf_eval_node_expansions": uint(p0_fields, "dcf_eval_node_expansions"),
            "p1_dcf_eval_node_expansions": uint(p1_fields, "dcf_eval_node_expansions"),
            "sampler_sha256_counter_words_p0": uint(p0_fields, "sampler_sha256_counter_words"),
            "sampler_sha256_counter_words_p1": uint(p1_fields, "sampler_sha256_counter_words"),
            "p0_sent_bytes": p0_sent, "p0_received_bytes": p0_recv,
            "p1_sent_bytes": p1_sent, "p1_received_bytes": p1_recv,
            "p0_rss_peak_bytes": uint(p0_fields, "peak_rss_kb") * 1024,
            "p1_rss_peak_bytes": uint(p1_fields, "peak_rss_kb") * 1024,
            "t_rss_peak_bytes": None,
            "dcf_eval_aes_blocks": 2 * prg_total,
            "root_prg_stream_calls_total": None,
            "dcf_eval_count": uint(p0_fields, "dcf_eval_calls_counted") + uint(p1_fields, "dcf_eval_calls_counted"),
            "dcf_keygen_calls_offline": uint(t_fields, "dcf_keygen_calls"),
            "dcf_keygen_node_expansions_offline": uint(t_fields, "dcf_keygen_node_expansions"),
            "dcf_keygen_aes_blocks_offline": 4 * uint(t_fields, "dcf_keygen_node_expansions"),
        }
        provenance = {name: "MEASURED" for name in MEASURED_METRICS}
        provenance.update({"counts.t_rss_peak_bytes": "NOT_MEASURED",
                          "counts.dcf_eval_aes_blocks": "CHECKED_CALLSITE_DERIVATION",
                          "counts.root_prg_stream_calls_total": "NOT_MEASURED",
                          "counts.dcf_keygen_aes_blocks_offline": "CHECKED_CALLSITE_DERIVATION",
                          "auxiliary.t_rss_peak_bytes": "NOT_MEASURED",
                          "auxiliary.offline_tls_wire_bytes": "NOT_MEASURED",
                          "auxiliary.offline_tls_handshake_bytes": "NOT_MEASURED",
                          "auxiliary.online_tls_wire_bytes": "NOT_MEASURED",
                          "auxiliary.online_tls_handshake_bytes": "NOT_MEASURED",
                          "auxiliary.online_protocol_payload_bytes_excluding_frames": "NOT_MEASURED",
                          "auxiliary.root_prg_stream_calls_total": "NOT_MEASURED",
                          "auxiliary.padded_n": "CHECKED_CALLSITE_DERIVATION",
                          "auxiliary.raw_score_real_ucmp_calls": "CHECKED_CALLSITE_DERIVATION",
                          "auxiliary.raw_score_padding_ucmp_calls": "CHECKED_CALLSITE_DERIVATION"})
        provenance.update({"network.rtt_ms": "NOT_MEASURED", "network.bandwidth_mbit_s": "NOT_MEASURED",
                           "network.netem_id": "NOT_MEASURED", "network.online_tls_wire_bytes": "NOT_MEASURED"})
        record = {
            **common, "terminal_status": "SUCCESS",
            "correctness": {"oracle_checked": True, "oracle_equal": True, "mask_length": args.n,
                            "mask_binary": True, "mask_weight_k": True, "mask_published": True},
            "metrics": metrics, "metric_units": METRIC_UNITS, "metric_provenance": provenance,
            "counts": counts,
            "auxiliary": {
                "online_counter_layer": "ProtocolIFramedChannel application bytes, including application frame/chunk envelopes; excludes TLS record/handshake/IP/TCP overhead",
                "causal_round_basis": "measured stage DAG: raw adapter 2, forward shuffle 2, coin 1, Select R1-R4 4, inverse 2, final agreement 1; actual phase counter checked against both parties",
                "offline_material_definition": "serialized online-held score/shuffle material bytes plus serialized primitive DCF key bytes; excludes bundle envelope, manifest, AEAD tags and TLS delivery",
                "offline_material_payload_bytes_p0": payload0, "offline_material_payload_bytes_p1": payload1,
                "offline_package_file_bytes_total": package_bytes,
                "offline_sidecar_file_bytes_per_party": sidecar,
                "offline_shell_file_bytes_p0": uint(t_fields, "shell_bytes_p0"),
                "offline_shell_file_bytes_p1": uint(t_fields, "shell_bytes_p1"),
                "offline_tls_application_sent_bytes_from_T": uint(t_fields, "tls_sent_bytes"),
                "offline_tls_application_received_bytes_at_T": uint(t_fields, "tls_received_bytes"),
                "offline_tls_wire_bytes": None, "offline_tls_handshake_bytes": None,
                "online_tls_wire_bytes": None, "online_tls_handshake_bytes": None,
                "online_app_framed_sent_bytes_p0": p0_sent, "online_app_framed_sent_bytes_p1": p1_sent,
                "online_received_bytes_used_only_for_reconciliation": p0_recv + p1_recv,
                "online_protocol_payload_bytes_excluding_frames": None,
                "select_logical_calls_by_round": p0_logical,
                "select_unique_real_edges_by_round": p0_unique,
                "select_dummy_local_requests_by_round": p0_dummy,
                "select_repeated_requests_by_round": p0_repeats,
                "dcf_eval_aes_blocks_callsite_identity": "2 AES blocks per measured DCF expansion ecbEncTwoBlocks; auxiliary count, not the PRG-call unit",
                "dcf_keygen_aes_blocks_callsite_identity": "4 AES blocks per measured DCF KeyGen expansion ecbEncFourBlocks; offline auxiliary count",
                "natural_algorithm_abort_count": 0, "injected_abort_count": 0,
                "padded_n": 1 << max(1, (args.n - 1).bit_length()),
                "raw_score_real_ucmp_calls": 2 * args.n,
                "raw_score_padding_ucmp_calls": 2 * ((1 << max(1, (args.n - 1).bit_length())) - args.n),
                "attempt_boundary": "diagnostic offline wall begins before receiver/T launch and ends at raw-share plus material readiness; online wall begins at that input/material-ready barrier and ends after both party processes exit after output publication; input fixture setup is included in this diagnostic offline envelope; total is the same-attempt outer wall interval",
                "online_party_api_time_us_p0": uint(p0_fields, "online_time_us"),
                "online_party_api_time_us_p1": uint(p1_fields, "online_time_us"),
                "online_party_api_critical_path_us": max(uint(p0_fields, "online_time_us"), uint(p1_fields, "online_time_us")),
                "online_party_api_time_note": "measured inside party runtime; excludes test controller overhead and TLS channel setup performed before party API entry",
                "e2e_test_binary_sha256": sha256_file(e2e),
                "e2e_command": command_text(command), "threads": args.threads,
            },
            "raw_evidence_manifest": manifest_name, "raw_record_sha256": manifest_hash,
        }
    record_path = out_dir / "run_record.json"
    record_path.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if fixture_path and fixture_path.is_dir():
        shutil.rmtree(fixture_path)
    if audit_record(record_path):
        for error in audit_record(record_path):
            print("audit_error=" + error, file=sys.stderr)
        return 2
    print(f"record={record_path}")
    print(f"raw_record_sha256={manifest_hash}")
    print(f"terminal_status={record['terminal_status']}")
    print(f"metrics={json.dumps(record['metrics'], sort_keys=True)}")
    print("audit=PASS")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    collect_parser = sub.add_parser("collect", help="run one fresh-material conditional-v2 diagnostic attempt")
    collect_parser.add_argument("--e2e-test", required=True)
    collect_parser.add_argument("--party-node", required=True)
    collect_parser.add_argument("--cmake-cache", required=True)
    collect_parser.add_argument("--oracle-source", required=True)
    collect_parser.add_argument("--source-commit", required=True)
    collect_parser.add_argument("--n", type=int, required=True)
    collect_parser.add_argument("--k", type=int, required=True)
    collect_parser.add_argument("--input-seed", type=int, required=True)
    collect_parser.add_argument("--session", type=int, required=True)
    collect_parser.add_argument("--threads", type=int, default=1)
    collect_parser.add_argument("--timeout-seconds", type=int, default=7200)
    collect_parser.add_argument("--out-dir", required=True)
    collect_parser.set_defaults(func=collect)
    audit_parser = sub.add_parser("audit", help="cross-check one run record and evidence hashes")
    audit_parser.add_argument("record")
    audit_parser.set_defaults(func=lambda a: _audit_cli(Path(a.record)))
    args = parser.parse_args()
    if args.command == "collect":
        if not (2 <= args.n <= 1000 and 1 <= args.k <= args.n):
            parser.error("collect requires 2<=n<=1000 and 1<=K<=n")
        return args.func(args)
    return args.func(args)


def _audit_cli(record: Path) -> int:
    errors = audit_record(record.resolve())
    if errors:
        for error in errors:
            print("audit_error=" + error, file=sys.stderr)
        return 1
    print("audit=PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
