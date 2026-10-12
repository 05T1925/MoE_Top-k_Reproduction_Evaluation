#!/usr/bin/env python3
"""Collect paired Protocol I attempts without merging route implementations.

This TEST_ONLY runner retains stdout/stderr and harmless role logs, records one
schema-v3 attempt for every route (including timeout and parse failures), and
removes temporary directories containing shares, keys, certificates, or
sidecars. Formal campaigns are gated on a machine-readable closed V3 gate.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import re
import resource
import secrets
import shutil
import signal
import subprocess
import sys
import time
import uuid
from pathlib import Path
from typing import Any

SCHEMA_VERSION = "3.0"
METRIC_UNITS = {
    "offline_time_ms": "ms",
    "offline_material_total_bits": "bits",
    "online_time_ms": "ms",
    "online_comm_total_bits": "application_frame_bits",
    "online_comm_per_party_bits": "application_frame_bits",
    "online_rounds": "causal_application_message_stages",
    "online_prg_calls_total": "online_length_doubling_PRG_calls",
    "comparison_edges_total": "executed_unordered_secure_comparison_edges",
    "total_time_ms": "ms",
}
ROUTES = ("protocol_i_all_pairs", "protocol_i_aav86", "protocol_i_bmw16_derived_select_dcf")
PROTOCOL_LABELS = {
    ROUTES[0]: "Protocol I all-pairs",
    ROUTES[1]: "Protocol I + AAV86",
    ROUTES[2]: "Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED",
}


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as src:
        for chunk in iter(lambda: src.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def parse_kv(line: str) -> dict[str, str]:
    return {m.group(1): m.group(2) for m in re.finditer(r"([A-Za-z0-9_]+)=([^\s]+)", line)}


def marker(text: str, name: str) -> dict[str, str]:
    for line in text.splitlines():
        if line.startswith(name + " "):
            return parse_kv(line)
    return {}


def number(fields: dict[str, str], key: str) -> int:
    value = fields.get(key)
    if value is None or not re.fullmatch(r"[0-9]+", value):
        raise ValueError(f"missing or invalid counter {key}")
    return int(value)


def array(fields: dict[str, str], key: str) -> list[int]:
    value = fields.get(key, "")
    if not (value.startswith("[") and value.endswith("]")):
        raise ValueError(f"missing counter array {key}")
    return [] if value == "[]" else [int(v) for v in value[1:-1].split(",")]


def line_with(path: Path, prefix: str) -> dict[str, str]:
    for line in reversed(path.read_text(encoding="utf-8", errors="replace").splitlines()):
        if line.startswith(prefix + " "):
            return parse_kv(line)
    raise ValueError(f"missing {prefix} line in {path.name}")


def host_info(scratch: Path) -> dict[str, Any]:
    mem = None
    p = Path("/proc/meminfo")
    if p.exists():
        for line in p.read_text(errors="replace").splitlines():
            if line.startswith("MemTotal:"):
                mem = int(line.split()[1]) * 1024
                break
    cpu_model = "NOT_REPORTED"
    p = Path("/proc/cpuinfo")
    if p.exists():
        for line in p.read_text(errors="replace").splitlines():
            if line.lower().startswith("model name"):
                cpu_model = line.split(":", 1)[1].strip()
                break
    def version(cmd: list[str]) -> str:
        try:
            return subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                  check=False, timeout=5).stdout.splitlines()[0]
        except Exception:
            return "NOT_REPORTED"
    return {"os": platform.platform(), "kernel": platform.release(), "cpu_model": cpu_model,
            "vcpu": os.cpu_count() or 1, "ram_bytes": mem,
            "scratch_bytes": shutil.disk_usage(scratch).free, "compiler": version(["g++", "--version"]),
            "cmake": version(["cmake", "--version"]), "openssl": version(["openssl", "version"])}


def git_identity(repo: Path, binary: Path, cache: Path) -> dict[str, Any]:
    def git(*args: str) -> str | None:
        try:
            return subprocess.run(["git", "-C", str(repo), *args], check=True, text=True,
                                  stdout=subprocess.PIPE, stderr=subprocess.DEVNULL).stdout.strip()
        except Exception:
            win_git = Path("/mnt/c/Program Files/Git/cmd/git.exe")
            if not win_git.is_file() or not str(repo).startswith("/mnt/"):
                return None
            win_repo = str(repo).replace("/mnt/c/", "C:/", 1)
            try:
                result = subprocess.run([str(win_git), "-C", win_repo, *args], check=True,
                                        stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
                return result.stdout.decode("utf-8", errors="strict").strip()
            except Exception:
                return None
    base = git("rev-parse", "HEAD")
    diff = None
    try:
        raw = subprocess.run(["git", "-C", str(repo), "diff", "--binary", "HEAD"], check=True,
                             stdout=subprocess.PIPE, stderr=subprocess.DEVNULL).stdout
        diff = sha256_bytes(raw)
    except Exception:
        win_git = Path("/mnt/c/Program Files/Git/cmd/git.exe")
        if win_git.is_file() and str(repo).startswith("/mnt/"):
            win_repo = str(repo).replace("/mnt/c/", "C:/", 1)
            try:
                raw = subprocess.run([str(win_git), "-C", win_repo, "diff", "--binary", "HEAD"], check=True,
                                     stdout=subprocess.PIPE, stderr=subprocess.DEVNULL).stdout
                diff = sha256_bytes(raw)
            except Exception:
                pass
    return {"base_commit": base if base and re.fullmatch(r"[0-9a-f]{40}", base) else None,
            "working_tree_diff_sha256": diff,
            "binary_sha256": sha256_file(binary) if binary.is_file() else None,
            "build_options_sha256": sha256_file(cache) if cache.is_file() else None}


def run_captured(command: list[str], env: dict[str, str], timeout: int,
                 address_space_limit: int | None = None) -> tuple[int, str, str, bool, str]:
    def set_limit() -> None:
        if address_space_limit is not None:
            resource.setrlimit(resource.RLIMIT_AS, (address_space_limit, address_space_limit))
    proc = subprocess.Popen(command, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            start_new_session=True, preexec_fn=set_limit if address_space_limit is not None else None)
    try:
        stdout, stderr = proc.communicate(timeout=timeout)
        return int(proc.returncode), stdout, stderr, False, ""
    except subprocess.TimeoutExpired as exc:
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        tail_out, tail_err = proc.communicate()
        partial_out = exc.stdout or ""
        partial_err = exc.stderr or ""
        if isinstance(partial_out, bytes): partial_out = partial_out.decode(errors="replace")
        if isinstance(partial_err, bytes): partial_err = partial_err.decode(errors="replace")
        stdout = partial_out + tail_out
        stderr = partial_err + tail_err
        return 124, stdout, stderr, True, f"timeout after {timeout}s"


def command_identity(binary: Path, node: Path, n: int, k: int, seed: int, timeout: int) -> dict[str, str]:
    command = [str(binary), str(node), "--paired-input-identity", str(n), str(k), str(seed)]
    try:
        rc, stdout, stderr, timed_out, reason = run_captured(command, os.environ.copy(), timeout)
    except Exception as exc:
        raise RuntimeError(json.dumps({"command": command, "launch_error": str(exc)}, sort_keys=True)) from exc
    ident = marker(stdout, "paired_input_identity")
    if rc != 0 or timed_out or not ident:
        detail = {"command": command, "returncode": rc, "timed_out": timed_out, "reason": reason,
                  "stdout_tail": stdout[-4000:], "stderr_tail": stderr[-4000:],
                  "identity_marker_present": bool(ident)}
        raise RuntimeError(json.dumps(detail, sort_keys=True))
    ident["oracle_sha256"] = ident["oracle_mask_sha256"]
    ident["shared_oracle_fnv64"] = ident["oracle_fnv64"]
    return ident


def parse_success(route: str, output: str, fixture_path: Path | None) -> tuple[dict[str, Any], dict[str, Any], dict[str, str]]:
    counts: dict[str, Any] = {}
    aux: dict[str, Any] = {"online_counter_layer": "application frames on test TCP/FD transports; excludes TLS/IP/TCP wire overhead",
                           "tls_wire_bytes": None, "root_prg_stream_calls_total": None,
                           "dcf_eval_aes_blocks": None, "sha_sampler_words": None}
    if route == ROUTES[0]:
        f = marker(output, "E12_BASELINE_CASE")
        if not f: raise ValueError("baseline E2E result marker missing")
        d = number(f, "d")
        p0s, p1s = number(f, "p0_sent_bytes"), number(f, "p1_sent_bytes")
        p0r, p1r = number(f, "p0_received_bytes"), number(f, "p1_received_bytes")
        if p0s != p1r or p1s != p0r: raise ValueError("baseline sent/received bytes do not reconcile")
        offline = number(f, "offline_ns")
        online = number(f, "online_wall_ns")
        prg = sum(number(f, x) for x in ("p0_score_dcf_prg", "p1_score_dcf_prg", "p0_pipeline_dcf_prg", "p1_pipeline_dcf_prg"))
        raw = number(f, "score_ucmp_calls")
        core = number(f, "edges")
        counts = {"p0_sent_bytes": p0s, "p1_sent_bytes": p1s, "p0_received_bytes": p0r, "p1_received_bytes": p1r,
                  "raw_score_ucmp_calls": raw, "protocol_core_unique_edges": core,
                  "dcf_score_and_pipeline_length_doubling_calls_total": prg,
                  "p0_t_payload_bytes": number(f, "p0_t_payload_bytes"), "p1_t_payload_bytes": number(f, "p1_t_payload_bytes"),
                  "p0_shuffle_payload_bytes": number(f, "p0_shuffle_payload_bytes"), "p1_shuffle_payload_bytes": number(f, "p1_shuffle_payload_bytes"),
                  "logical_material_slots": None, "dummy_requests": None, "repeated_requests": None,
                  "dcf_eval_count": number(f, "dcf_eval_per_party") * 2}
        material = sum(number(f, x) for x in ("p0_t_payload_bytes", "p1_t_payload_bytes", "p0_shuffle_payload_bytes", "p1_shuffle_payload_bytes"))
        metrics = {"offline_time_ms": offline / 1e6, "offline_material_total_bits": material * 8,
                   "online_time_ms": online / 1e6, "online_comm_total_bits": 8 * (p0s + p1s),
                   "online_comm_per_party_bits": 4 * (p0s + p1s), "online_rounds": number(f, "rounds"),
                   "online_prg_calls_total": prg, "comparison_edges_total": raw + core,
                   "total_time_ms": (offline + online) / 1e6}
        counts.update({"offline_package_bytes_p0": number(f, "p0_package_bytes"), "offline_package_bytes_p1": number(f, "p1_package_bytes")})
        aux.update({"input_generation_ns": number(f, "input_generation_ns"),
                    "input_share_fixture_ns": number(f, "input_share_fixture_ns"),
                    "entry_path_wall_ns": number(f, "entry_path_wall_ns"), "padded_n": d,
                    "comparison_edges_definition": "raw score input uCMP calls + actual all-pairs core edge counter"})
        return metrics, counts, {"input_fnv64": f["input_digest"], "oracle_fnv64": f["oracle_digest"]}
    if route == ROUTES[1]:
        f = marker(output, "E12_AAV86_CASE")
        meta = marker(output, "E12_BENCH_META")
        if not f or not meta: raise ValueError("AAV86 E2E result marker missing")
        d = number(f, "d")
        p0s, p1s = number(f, "p0_bytes"), number(f, "p1_bytes")
        p0r, p1r = number(f, "p0_received_bytes"), number(f, "p1_received_bytes")
        if p0s != p1r or p1s != p0r: raise ValueError("AAV86 sent/received bytes do not reconcile")
        offline, online = number(f, "offline_elapsed_ns"), number(f, "online_wall_ns")
        active, raw = number(f, "active"), 2 * d
        prg = number(f, "online_prg_total")
        p0mat, p1mat = number(f, "t_payload_bytes_p0"), number(f, "t_payload_bytes_p1")
        counts = {"p0_sent_bytes": p0s, "p1_sent_bytes": p1s, "p0_received_bytes": p0r, "p1_received_bytes": p1r,
                  "raw_score_secure_comparison_edges": raw, "protocol_active_unique_edges": active,
                  "reserved_material_slots_per_party": number(f, "reserved"), "dcf_evaluations_per_party": number(f, "total_eval_per_party"),
                  "online_length_doubling_calls_total": prg, "t_payload_bytes_p0": p0mat, "t_payload_bytes_p1": p1mat,
                  "offline_package_bytes_p0": number(f, "package_bytes_p0"), "offline_package_bytes_p1": number(f, "package_bytes_p1"),
                  "logical_material_slots": None, "dummy_requests": None, "repeated_requests": None}
        metrics = {"offline_time_ms": offline / 1e6, "offline_material_total_bits": 8 * (p0mat + p1mat),
                   "online_time_ms": online / 1e6, "online_comm_total_bits": 8 * (p0s + p1s),
                   "online_comm_per_party_bits": 4 * (p0s + p1s), "online_rounds": number(f, "rounds"),
                   "online_prg_calls_total": prg, "comparison_edges_total": raw + active,
                   "total_time_ms": (offline + online) / 1e6}
        counts["r"] = number(f, "r")
        aux.update({"input_generation_ns": number(meta, "input_generation_ns"),
                    "input_share_fixture_ns": number(f, "input_share_fixture_ns"),
                    "entry_path_wall_ns": number(f, "entry_path_wall_ns"), "padded_n": d,
                    "comparison_edges_definition": "2D raw signed-score comparisons + unique active AAV86 graph edges"})
        return metrics, counts, {"input_fnv64": meta["input_digest"], "oracle_fnv64": f["oracle_digest"]}
    if fixture_path is None:
        raise ValueError("BMW16 successful attempt has no preserved TEST_ONLY fixture")
    f = marker(output, "benchmark_attempt")
    fixture_marker = marker(output, "benchmark_fixture")
    if f.get("status") != "SUCCESS_ORACLE_CHECKED": raise ValueError("BMW16 oracle-checked marker missing")
    if (fixture_path / "T.log").is_file():
        t_path, p0_path, p1_path = fixture_path / "T.log", fixture_path / "P0_online.log", fixture_path / "P1_online.log"
    else:
        t_path, p0_path, p1_path = fixture_path / "t" / "t.log", fixture_path / "p0" / "online.log", fixture_path / "p1" / "online.log"
    t = line_with(t_path, "role=T")
    p0 = line_with(p0_path, "role=P0")
    p1 = line_with(p1_path, "role=P1")
    for role in (p0, p1):
        if role.get("dcf_eval_counter_matches_runtime") != "true" or role.get("dcf_counter_enabled") != "true":
            raise ValueError("BMW16 party DCF counter is missing or disagrees with runtime")
    eq = ("raw_adapter_ucmp_calls", "raw_adapter_dcf_evaluations", "online_message_phases", "membership_slots_consumed", "logical_comparisons", "ucmp_eval", "dcf_eval")
    for key in eq:
        if number(p0, key) != number(p1, key): raise ValueError(f"BMW16 party count disagreement: {key}")
    p0l, p1l = [array(x, "select_logical_calls_r1_r4") for x in (p0, p1)]
    p0u, p1u = [array(x, "select_unique_slots_r1_r4") for x in (p0, p1)]
    p0d, p1d = [array(x, "select_dummy_calls_r1_r4") for x in (p0, p1)]
    p0rep, p1rep = [array(x, "select_repeated_calls_r1_r4") for x in (p0, p1)]
    if (p0l, p0u, p0d, p0rep) != (p1l, p1u, p1d, p1rep): raise ValueError("BMW16 per-round party counts differ")
    p0s, p1s = number(p0, "online_bytes_sent"), number(p1, "online_bytes_sent")
    p0r, p1r = number(p0, "online_bytes_received"), number(p1, "online_bytes_received")
    if p0s != p1r or p1s != p0r: raise ValueError("BMW16 application bytes do not reconcile")
    raw, unique, membership = number(p0, "raw_adapter_ucmp_calls"), sum(p0u), number(p0, "membership_slots_consumed")
    prg = number(p0, "dcf_eval_node_expansions") + number(p1, "dcf_eval_node_expansions")
    mat0, mat1 = number(t, "material_payload_bytes_p0"), number(t, "material_payload_bytes_p1")
    side0 = number(t, "sidecar_bytes_p0") if "sidecar_bytes_p0" in t else number(t, "sidecar_bytes_per_party")
    side1 = number(t, "sidecar_bytes_p1") if "sidecar_bytes_p1" in t else number(t, "sidecar_bytes_per_party")
    offline, online = number(f, "offline_time_us"), number(f, "online_time_us")
    if abs(number(f, "total_time_us") - offline - online) > 2: raise ValueError("BMW16 attempt time does not reconcile")
    metrics = {"offline_time_ms": offline / 1000, "offline_material_total_bits": 8 * (mat0 + mat1),
               "online_time_ms": online / 1000, "online_comm_total_bits": 8 * (p0s + p1s),
               "online_comm_per_party_bits": 4 * (p0s + p1s), "online_rounds": number(p0, "online_message_phases"),
               "online_prg_calls_total": prg, "comparison_edges_total": raw + unique + membership,
               "total_time_ms": number(f, "total_time_us") / 1000}
    counts = {"p0_sent_bytes": p0s, "p1_sent_bytes": p1s, "p0_received_bytes": p0r, "p1_received_bytes": p1r,
              "raw_score_secure_comparison_edges": raw, "select_unique_real_edges": unique,
              "select_logical_requests": sum(p0l), "select_dummy_requests": sum(p0d),
              "select_repeated_requests": sum(p0rep), "membership_secure_edges": membership,
              "unique_material_slots_per_party": number(p0, "process_slots_claimed"),
              "dcf_eval_count_p0": number(p0, "dcf_eval_calls_counted"), "dcf_eval_count_p1": number(p1, "dcf_eval_calls_counted"),
              "dcf_node_expansions_p0": number(p0, "dcf_eval_node_expansions"), "dcf_node_expansions_p1": number(p1, "dcf_eval_node_expansions"),
              "dcf_keygen_calls_offline": number(t, "dcf_keygen_calls"), "dcf_keygen_node_expansions_offline": number(t, "dcf_keygen_node_expansions"),
              "material_payload_bytes_p0": mat0, "material_payload_bytes_p1": mat1,
              "offline_sidecar_bytes_p0": side0, "offline_sidecar_bytes_p1": side1,
              "offline_shell_bytes_p0": number(t, "shell_bytes_p0"), "offline_shell_bytes_p1": number(t, "shell_bytes_p1"),
              "unique_select_edges_by_round": p0u, "logical_select_requests_by_round": p0l,
              "dummy_select_requests_by_round": p0d, "repeated_select_requests_by_round": p0rep}
    aux.update({"input_share_fixture_us": number(f, "input_fixture_time_us"),
                "input_generation_ns": number(fixture_marker, "input_generation_time_us") * 1000,
                "entry_path_wall_us": number(f, "entry_path_wall_time_us"),
                "offline_package_or_sidecar_bytes": side0 + side1 + number(t, "shell_bytes_p0") + number(t, "shell_bytes_p1"),
                "offline_tls_application_bytes": number(t, "tls_sent_bytes") + number(t, "tls_received_bytes"),
                "dcf_eval_aes_blocks": 2 * prg, "dcf_keygen_aes_blocks": 4 * number(t, "dcf_keygen_node_expansions"),
                "sampler_sha256_counter_words": number(p0, "sampler_sha256_counter_words") + number(p1, "sampler_sha256_counter_words"),
                "comparison_edges_definition": "raw adapter executed calls + per-round unique real Select material slots + membership edges; excludes dummy and repeated logical requests"})
    return metrics, counts, {"input_sha256": f["input_sha256"], "oracle_sha256": f["oracle_mask_sha256"],
                             "oracle_fnv64": fixture_marker["oracle_fnv64"]}


def attempt(args: argparse.Namespace, route: str, seed: int, run_index: int | None,
            attempt_type: str, identity: dict[str, str], campaign_id: str) -> dict[str, Any]:
    out = Path(args.out_dir).resolve() / campaign_id / route / (attempt_type.lower() + (f"_{run_index}" if run_index else "_1"))
    out.mkdir(parents=True, exist_ok=False)
    binaries = {ROUTES[0]: Path(args.all_pairs).resolve(), ROUTES[1]: Path(args.aav86).resolve(), ROUTES[2]: Path(args.bmw_e2e).resolve()}
    repos = {ROUTES[0]: Path(args.e16_repo).resolve(), ROUTES[1]: Path(args.e16_repo).resolve(), ROUTES[2]: Path(args.bmw_repo).resolve()}
    caches = {ROUTES[0]: Path(args.e16_cmake_cache).resolve(), ROUTES[1]: Path(args.e16_cmake_cache).resolve(), ROUTES[2]: Path(args.bmw_cmake_cache).resolve()}
    binary = binaries[route]
    serial = secrets.randbelow(2**62) + 1
    if route == ROUTES[0]: command = [str(binary), "bench", str(args.n), str(args.k), str(seed), str(serial)]
    elif route == ROUTES[1]: command = [str(binary), "bench", str(args.n), str(args.k), str(args.r), str(seed), str(serial)]
    else: command = [str(binary), str(args.bmw_node), "--conditional-secure-v2-paired-case", str(args.n), str(args.k), str(seed), str(serial)]
    env = os.environ.copy()
    env.update({"OMP_NUM_THREADS": "1", "MOE_TOPK_M6A_E15_BENCH": "1", "MOE_TOPK_M6A_E11_TRANSPORT": "tcp",
                "MOE_BMW16_KEEP_TEST_FIXTURES": "1"})
    started = time.monotonic_ns()
    padded_n = max(2, 1 << (args.n - 1).bit_length())
    address_space_limit = (3072 * 1024 * 1024 if padded_n == 256 else 768 * 1024 * 1024) if route == ROUTES[1] else (768 * 1024 * 1024 if route == ROUTES[0] else None)
    rc, stdout, stderr, timed_out, timeout_reason = run_captured(command, env, args.timeout_seconds,
                                                                 address_space_limit)
    elapsed = time.monotonic_ns() - started
    (out / "stdout.txt").write_text(stdout, encoding="utf-8")
    (out / "stderr.txt").write_text(stderr, encoding="utf-8")
    (out / "command.json").write_text(json.dumps({"argv": command, "shared_input_identity": identity},
                                                    indent=2, sort_keys=True) + "\n", encoding="utf-8")
    fixture = ""
    if route == ROUTES[2]:
        fixture = marker(stdout, "benchmark_attempt").get("fixture", "")
        if not fixture:
            m = re.search(r"fixture_preserved=([^\s]+)", stderr)
            fixture = m.group(1) if m else ""
    fixture_path = Path(fixture) if fixture else None
    safe_logs: list[str] = []
    if fixture_path and fixture_path.is_dir():
        for relative, name in (("t/t.log", "T.log"), ("p0/receive.log", "P0_receive.log"), ("p1/receive.log", "P1_receive.log"),
                               ("p0/online.log", "P0_online.log"), ("p1/online.log", "P1_online.log")):
            source = fixture_path / relative
            if source.is_file():
                shutil.copyfile(source, out / name)
                safe_logs.append(name)
        shutil.rmtree(fixture_path, ignore_errors=True)
        fixture_path = out if (out / "T.log").is_file() else None
    base_id = git_identity(repos[route], binary, caches[route])
    combined = stdout + "\n" + stderr
    if timed_out:
        status = "TIMEOUT"
    elif rc == 0:
        status = "ENGINEERING_ERROR"
    elif any(token in combined for token in ("PEER_AGREED_ALGORITHM_ABORT", "natural_algorithm_abort", "status=NATURAL_ABORT")):
        status = "PEER_AGREED_ALGORITHM_ABORT"
    elif any(token in combined.lower() for token in ("material abort", "material_error", "persistent claim replay", "sidecar")):
        status = "MATERIAL_ERROR"
    elif any(token in combined.lower() for token in ("communication", "tls_online", "eof", "timeout")):
        status = "COMMUNICATION_ERROR"
    else:
        status = "ENGINEERING_ERROR"
    reason = timeout_reason or (f"process exited {rc}" if rc else None)
    metrics = {name: None for name in METRIC_UNITS}
    counts: dict[str, Any] = {"process_exit_code": rc, "controller_elapsed_ns": elapsed,
                              "p0_sent_bytes": None, "p1_sent_bytes": None,
                              "p0_received_bytes": None, "p1_received_bytes": None}
    aux: dict[str, Any] = {"command": command, "input_generation_and_fixture_are_auxiliary": True,
                           "e2e_input_share_fixture_bytes": None, "controller_elapsed_ns": elapsed,
                           "network_layer": "application frame counters; TLS/TCP wire unavailable",
                           "collector_source_sha256": sha256_file(Path(__file__).resolve()),
                           "runtime_party_node_binary_sha256": sha256_file(Path(args.bmw_node).resolve()) if route == ROUTES[2] else None}
    observed_identity = dict(identity)
    observed_identity["oracle_fnv64"] = None
    correctness = {"oracle_checked": False, "oracle_equal": None, "mask_length": None,
                   "mask_binary": None, "mask_weight_k": None, "mask_published": False}
    try:
        if timed_out:
            raise RuntimeError(timeout_reason)
        if rc != 0:
            raise RuntimeError((stderr.strip() or stdout.strip() or f"process exited {rc}")[-1200:])
        metrics, counts, route_identity = parse_success(route, stdout, fixture_path)
        if route == ROUTES[0] or route == ROUTES[1]:
            if route_identity["input_fnv64"] != identity["input_fnv64"]:
                raise RuntimeError("route input FNV64 differs from shared BMW16 fixture generator")
            if route_identity["oracle_fnv64"] != identity["oracle_fnv64"]:
                raise RuntimeError("route oracle FNV64 differs from shared BMW16 frozen oracle")
            observed_identity.update(route_identity)
        else:
            for key in ("input_sha256", "oracle_sha256"):
                if route_identity[key] != identity[key]: raise RuntimeError(f"BMW16 {key} differs from identity fixture")
            if route_identity["oracle_fnv64"] != identity["oracle_fnv64"]:
                raise RuntimeError("BMW16 attempt oracle FNV64 differs from paired identity oracle")
            observed_identity.update(route_identity)
        correctness = {"oracle_checked": True, "oracle_equal": True, "mask_length": args.n,
                       "mask_binary": True, "mask_weight_k": True, "mask_published": True}
        status, reason = "SUCCESS", None
    except Exception as exc:
        reason = str(exc)
        metrics = {name: None for name in METRIC_UNITS}
    finally:
        if fixture_path and fixture_path != out and fixture_path.exists(): shutil.rmtree(fixture_path, ignore_errors=True)
    units = METRIC_UNITS
    provenance = {name: ("MEASURED" if value is not None else "NOT_MEASURED") for name, value in metrics.items()}
    for key in ("offline_material_total_bits", "comparison_edges_total", "online_prg_calls_total", "total_time_ms"):
        if metrics.get(key) is not None: provenance[key] = "CHECKED_CALLSITE_DERIVATION" if key != "total_time_ms" else "MEASURED"
    def mark_unmeasured(prefix: str, value: Any) -> None:
        if isinstance(value, dict):
            for key, child in value.items(): mark_unmeasured(f"{prefix}.{key}" if prefix else key, child)
        elif value is None:
            provenance[prefix] = "NOT_MEASURED"
    host = host_info(out)
    is_formal = attempt_type in {"WARMUP", "FORMAL"}
    network = {"mode": args.network_mode if is_formal else "loopback_diagnostic",
               "rtt_ms": args.rtt_ms if is_formal else None,
               "throughput_mbit_s": args.throughput_mbit_s if is_formal else None,
               "topology_id": args.topology_id if is_formal else "local-TCP-test-transport",
               "netem_id": args.netem_id if is_formal else None,
               "measurement_method": "preconfigured network impairment profile; externally verified before attempt" if is_formal else None,
               "online_tls_wire_bytes": None, "offline_tls_delivery_bytes": None}
    for section, value in (("counts", counts), ("auxiliary", aux), ("network", network)):
        mark_unmeasured(section, value)
    record = {"schema_version": SCHEMA_VERSION, "record_type": "attempt", "campaign_id": campaign_id,
              "attempt_id": str(uuid.uuid4()), "attempt_type": attempt_type, "run_index": run_index,
              "route": route, "implementation_label": "AAV86/r=2" if route == ROUTES[1] else ("conditional_secure_v2" if route == ROUTES[2] else "Protocol_I_all_pairs"),
              "security_profile": "TEST_ONLY; route-specific security assumptions remain in source decisions; BMW16 route is conditional_secure_v2 under trusted offline T and semi-honest one-party corruption",
              "source": base_id,
              "input": {"n": args.n, "k": args.k, "score_encoding": "signed_Q20.12",
                        "generator": "mt19937_64_uniform_signed_Q20.12_v1", "seed_id": str(seed),
                        "input_sha256": observed_identity.get("input_sha256"), "input_fnv64": observed_identity.get("input_fnv64"),
                        "oracle_sha256": observed_identity.get("oracle_sha256", observed_identity.get("oracle_mask_sha256")),
                        "oracle_fnv64": observed_identity.get("oracle_fnv64"),
                        "shared_oracle_fnv64": identity.get("shared_oracle_fnv64"),
                        "oracle_implementation_sha256": sha256_file(Path(args.oracle_source).resolve()) if Path(args.oracle_source).is_file() else None},
              "host": host, "network": network, "terminal_status": status, "failure_reason": reason,
              "correctness": correctness, "metrics": metrics, "metric_units": units,
              "metric_provenance": provenance, "counts": counts, "auxiliary": aux,
              "evidence": {"directory": str(out), "manifest_sha256": None, "files": []}}
    file_entries = []
    for path in sorted(out.iterdir()):
        if path.is_file() and path.name not in {"evidence_manifest.json", "run_record.json"}:
            file_entries.append({"path": path.name, "sha256": sha256_file(path), "bytes": path.stat().st_size})
    manifest = {"manifest_version": 1, "campaign_id": campaign_id, "attempt_id": record["attempt_id"], "files": file_entries}
    manifest_bytes = (json.dumps(manifest, sort_keys=True, separators=(",", ":")) + "\n").encode()
    (out / "evidence_manifest.json").write_bytes(manifest_bytes)
    record["evidence"] = {"directory": str(out), "manifest_sha256": sha256_bytes(manifest_bytes), "files": file_entries}
    (out / "run_record.json").write_text(json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"attempt route={route} status={status} record={out / 'run_record.json'}")
    return record


def preflight(args: argparse.Namespace) -> int:
    n, k = args.n, args.k
    d = max(2, 1 << (n - 1).bit_length())
    scratch = Path(args.scratch_path).resolve()
    disk = shutil.disk_usage(scratch)
    available_mem = None
    p = Path("/proc/meminfo")
    if p.exists():
        for line in p.read_text(errors="replace").splitlines():
            if line.startswith("MemAvailable:"):
                available_mem = int(line.split()[1]) * 1024
                break
    def classify(route: str) -> tuple[str, str]:
        if route == ROUTES[0]:
            if (n, k) not in {(128, 2), (128, 8), (256, 2), (256, 8)}:
                return "PRECHECK_REJECTED", "the current all-pairs bridge admits only (n,K) in {(128,2),(128,8),(256,2),(256,8)}"
            return "RESOURCE_REVIEW_REQUIRED", "the current all-pairs bridge has an executable shape; checked runtime and resource admission are still required"
        if route == ROUTES[1]:
            if d > 256:
                return "PRECHECK_REJECTED", f"AAV86 E16 hard cap D<=256 rejects padded D={d} before key generation"
            if (n, k) not in {(128, 2), (128, 8), (256, 2), (256, 8)}:
                return "PRECHECK_REJECTED", "the current AAV86 bridge admits only (n,K) in {(128,2),(128,8),(256,2),(256,8)}"
            return "RESOURCE_REVIEW_REQUIRED", "AAV86 r=2 has an executable shape; checked package, RAM, timeout and network admission are still required"
        if n < 2 or n > 1000 or k < 1 or k > n:
            return "PRECHECK_REJECTED", f"conditional_secure_v2 admits 2<=n<=1000 and 1<=K<=n; received n={n}, K={k}"
        return "RESOURCE_REVIEW_REQUIRED", "conditional_secure_v2 admits this shape; checked material, peak RSS, timeout and network admission are still required"

    route_results = {}
    for route in ROUTES:
        status, reason = classify(route)
        route_results[route] = {"status": status, "reason": reason}
    results = {
        "schema_version": SCHEMA_VERSION, "record_type": "preflight", "n": n, "k": k, "padded_n": d,
        "host": host_info(scratch), "checked_disk_free_bytes": disk.free,
        "checked_available_memory_bytes": available_mem,
        "routes": route_results,
        "network_flow_bytes": None, "runtime_seconds": None,
        "network_flow_and_runtime_provenance": "NOT_MEASURED; a preflight does not execute protocol routes; do not extrapolate historical results",
        "campaign_admission": None
    }
    results["campaign_admission"] = ("PRECHECK_REJECTED" if any(v["status"] == "PRECHECK_REJECTED" for v in route_results.values())
                                      else "RESOURCE_REVIEW_REQUIRED")
    print(json.dumps(results, indent=2, sort_keys=True))
    return 0


def record_identity_fixture_failure(args: argparse.Namespace, campaign_id: str, error: str) -> int:
    """Keep a machine-readable record when the shared TEST_ONLY fixture fails before routes start."""
    campaign_dir = Path(args.out_dir).resolve() / campaign_id
    campaign_dir.mkdir(parents=True, exist_ok=False)
    binaries = {ROUTES[0]: Path(args.all_pairs).resolve(), ROUTES[1]: Path(args.aav86).resolve(),
                ROUTES[2]: Path(args.bmw_e2e).resolve()}
    repos = {ROUTES[0]: Path(args.e16_repo).resolve(), ROUTES[1]: Path(args.e16_repo).resolve(),
             ROUTES[2]: Path(args.bmw_repo).resolve()}
    caches = {ROUTES[0]: Path(args.e16_cmake_cache).resolve(), ROUTES[1]: Path(args.e16_cmake_cache).resolve(),
              ROUTES[2]: Path(args.bmw_cmake_cache).resolve()}
    attempt_refs = []
    for route in ROUTES:
        out = campaign_dir / route / "diagnostic_1"
        out.mkdir(parents=True, exist_ok=False)
        binary, cache = binaries[route], caches[route]
        try:
            source = git_identity(repos[route], binary, cache)
        except Exception:
            source = {"base_commit": None, "working_tree_diff_sha256": None,
                      "binary_sha256": sha256_file(binary) if binary.is_file() else None,
                      "build_options_sha256": sha256_file(cache) if cache.is_file() else None}
        failure = {"failure_stage": "shared_TEST_ONLY_input_identity",
                   "failure_reason": error, "protocol_attempts_started": False,
                   "route_command_started": False}
        failure_path = out / "identity_fixture_failure.json"
        failure_path.write_text(json.dumps(failure, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        files = [{"path": failure_path.name, "sha256": sha256_file(failure_path), "bytes": failure_path.stat().st_size}]
        manifest = {"manifest_version": 1, "campaign_id": campaign_id,
                    "attempt_id": str(uuid.uuid4()), "files": files}
        manifest_bytes = (json.dumps(manifest, sort_keys=True, separators=(",", ":")) + "\n").encode()
        (out / "evidence_manifest.json").write_bytes(manifest_bytes)
        attempt_id = manifest["attempt_id"]
        network = {"mode": "loopback_diagnostic", "rtt_ms": None, "throughput_mbit_s": None,
                   "topology_id": "local-TCP-test-transport", "netem_id": None,
                   "measurement_method": None, "online_tls_wire_bytes": None,
                   "offline_tls_delivery_bytes": None}
        provenance = {name: "NOT_MEASURED" for name in METRIC_UNITS}
        provenance.update({f"network.{key}": "NOT_MEASURED" for key, value in network.items() if value is None})
        record = {"schema_version": SCHEMA_VERSION, "record_type": "attempt", "campaign_id": campaign_id,
                  "attempt_id": attempt_id, "attempt_type": "DIAGNOSTIC", "run_index": None,
                  "route": route, "implementation_label": "AAV86/r=2" if route == ROUTES[1] else
                  ("conditional_secure_v2" if route == ROUTES[2] else "Protocol_I_all_pairs"),
                  "security_profile": "TEST_ONLY; shared input identity failed before any protocol route started",
                  "source": source,
                  "input": {"n": args.n, "k": args.k, "score_encoding": "signed_Q20.12",
                            "generator": "uniform_signed_q20_12_v1", "seed_id": str(args.input_seed),
                            "input_sha256": None, "input_fnv64": None, "oracle_sha256": None,
                            "oracle_fnv64": None, "shared_oracle_fnv64": None,
                            "oracle_implementation_sha256": sha256_file(Path(args.oracle_source).resolve())
                            if Path(args.oracle_source).is_file() else None},
                  "host": host_info(Path(args.out_dir).resolve()), "network": network,
                  "terminal_status": "ENGINEERING_ERROR", "failure_reason": error,
                  "correctness": {"oracle_checked": False, "oracle_equal": None, "mask_length": None,
                                  "mask_binary": None, "mask_weight_k": None, "mask_published": False},
                  "metrics": {name: None for name in METRIC_UNITS}, "metric_units": METRIC_UNITS,
                  "metric_provenance": provenance,
                  "counts": {"protocol_attempts_started": False},
                  "auxiliary": {"failure_stage": "shared_TEST_ONLY_input_identity",
                                "route_command_started": False, "identity_fixture_command":
                                [str(Path(args.bmw_e2e).resolve()), str(Path(args.bmw_node).resolve()),
                                 "--paired-input-identity", str(args.n), str(args.k), str(args.input_seed)],
                                "collector_source_sha256": sha256_file(Path(__file__).resolve())},
                  "evidence": {"directory": str(out), "manifest_sha256": sha256_bytes(manifest_bytes), "files": files}}
        record_path = out / "run_record.json"
        record_path.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        attempt_refs.append({"route": route, "attempt_id": attempt_id, "attempt_type": "DIAGNOSTIC",
                             "run_index": None, "record_path": str(record_path),
                             "record_sha256": sha256_file(record_path), "terminal_status": "ENGINEERING_ERROR"})
    campaign = {"schema_version": SCHEMA_VERSION, "record_type": "campaign", "campaign_id": campaign_id,
                "campaign_type": "DIAGNOSTIC", "phase_gate": "V3_OPEN", "route_set": list(ROUTES),
                "configuration": {"n": args.n, "k": args.k, "input_seed_id": str(args.input_seed),
                                  "input_sha256": None, "oracle_sha256": None, "oracle_fnv64": None},
                "network": {"mode": "loopback_diagnostic", "rtt_ms": None,
                            "throughput_mbit_s": None, "topology_id": "local-TCP-test-transport"},
                "warmups_per_route": 0, "formal_repetitions_per_route": 5, "attempts": attempt_refs,
                "campaign_status": "COMPLETE", "campaign_outcome": "ATTEMPT_FAILURES_RETAINED"}
    campaign_path = campaign_dir / "campaign_record.json"
    campaign_path.write_text(json.dumps(campaign, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    errors = audit(campaign_path)
    if errors:
        raise RuntimeError("identity failure record audit failed: " + "; ".join(errors))
    print(f"campaign status=COMPLETE outcome=ATTEMPT_FAILURES_RETAINED record={campaign_path}")
    return 1


def collect(args: argparse.Namespace) -> int:
    if not (2 <= args.n <= 1000 and 1 <= args.k <= args.n): raise ValueError("invalid n/K")
    if args.campaign_type == "FORMAL":
        raise ValueError("local fork/exec harness is DIAGNOSTIC-only; use seal-campaign to validate server-collected FORMAL attempts after V3 closes")
    if args.campaign_type == "DIAGNOSTIC" and not (args.n in (128, 256) and args.k in (2, 8)):
        raise ValueError("paired DIAGNOSTIC runner admits n=128/256 and K=2/8 only")
    if args.campaign_type == "DIAGNOSTIC" and args.r != 2:
        raise ValueError("S33 paired AAV86 diagnostic fixes the admitted iteration count at r=2")
    campaign_id = f"pi-v3-{args.n}-{args.k}-{args.campaign_type.lower()}-{uuid.uuid4()}"
    try:
        identity = command_identity(Path(args.bmw_e2e).resolve(), Path(args.bmw_node).resolve(),
                                    args.n, args.k, args.input_seed, args.timeout_seconds)
    except Exception as exc:
        return record_identity_fixture_failure(args, campaign_id, str(exc))
    reps = 1 if args.campaign_type == "DIAGNOSTIC" else 5
    records = []
    if args.campaign_type == "FORMAL":
        for route in ROUTES:
            records.append(attempt(args, route, args.input_seed, None, "WARMUP", identity, campaign_id))
    for idx in range(1, reps + 1):
        for route in ROUTES:
            records.append(attempt(args, route, args.input_seed, idx if reps == 5 else None,
                                   args.campaign_type, identity, campaign_id))
    campaign_dir = Path(args.out_dir).resolve() / campaign_id
    campaign = {"schema_version": SCHEMA_VERSION, "record_type": "campaign", "campaign_id": campaign_id,
                "campaign_type": args.campaign_type, "phase_gate": "V3_CLOSED" if args.campaign_type == "FORMAL" else "V3_OPEN",
                "route_set": list(ROUTES), "configuration": {"n": args.n, "k": args.k, "input_seed_id": str(args.input_seed),
                                      "input_sha256": identity.get("input_sha256"), "oracle_sha256": identity.get("oracle_mask_sha256"),
                                      "oracle_fnv64": identity.get("oracle_fnv64")},
                "network": {"mode": args.network_mode, "rtt_ms": args.rtt_ms, "throughput_mbit_s": args.throughput_mbit_s,
                            "topology_id": args.topology_id, "netem_id": args.netem_id},
                "warmups_per_route": 0 if args.campaign_type == "DIAGNOSTIC" else 1,
                "formal_repetitions_per_route": 5,
                "attempts": [{"route": r["route"], "attempt_id": r["attempt_id"], "run_index": r["run_index"],
                              "attempt_type": r["attempt_type"], "record_path": str(campaign_dir / r["route"] / (r["attempt_type"].lower() + (f"_{r['run_index']}" if r["run_index"] else "_1")) / "run_record.json"),
                              "record_sha256": sha256_file(campaign_dir / r["route"] / (r["attempt_type"].lower() + (f"_{r['run_index']}" if r["run_index"] else "_1")) / "run_record.json"),
                              "terminal_status": r["terminal_status"]} for r in records],
                "campaign_status": "COMPLETE",
                "campaign_outcome": "ALL_SUCCEEDED" if all(r["terminal_status"] == "SUCCESS" for r in records) else "ATTEMPT_FAILURES_RETAINED"}
    campaign_path = campaign_dir / "campaign_record.json"
    campaign_path.write_text(json.dumps(campaign, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"campaign status={campaign['campaign_status']} record={campaign_path}")
    return 0 if campaign["campaign_status"] == "COMPLETE" else 1


def audit_campaign_identity(data: dict[str, Any], attempts: list[dict[str, Any]]) -> list[str]:
    errors: list[str] = []
    cfg = data.get("configuration", {})
    expected_shape = (cfg.get("n"), cfg.get("k"))
    expected_identity = (cfg.get("input_sha256"), cfg.get("oracle_sha256"))
    expected_seed = cfg.get("input_seed_id")
    observed_identities = set()
    session_serials: list[str] = []
    for attempt_data in attempts:
        if data.get("campaign_id") and attempt_data.get("campaign_id") != data.get("campaign_id"):
            errors.append("campaign attempt campaign_id differs from campaign")
        attempt_path = Path(attempt_data.get("evidence", {}).get("directory", ""))
        attempt_input = attempt_data.get("input", {})
        if (attempt_input.get("n"), attempt_input.get("k")) != expected_shape:
            errors.append("campaign attempt n/K differs from campaign configuration")
        attempt_identity = (attempt_input.get("input_sha256"), attempt_input.get("oracle_sha256"))
        observed_identities.add(attempt_identity)
        if attempt_identity != expected_identity:
            errors.append("campaign attempt input/oracle SHA-256 differs from campaign configuration")
        if expected_seed is not None and str(attempt_input.get("seed_id")) != str(expected_seed):
            errors.append("campaign attempt input seed differs from campaign configuration")
        started = attempt_data.get("counts", {}).get("protocol_attempts_started",
            attempt_data.get("auxiliary", {}).get("protocol_attempts_started"))
        if started is False:
            continue
        listed_paths = {entry.get("path") for entry in attempt_data.get("evidence", {}).get("files", [])}
        command_path = attempt_path / "command.json"
        if "command.json" not in listed_paths or not command_path.is_file():
            errors.append(f"started route lacks hash-bound command.json: {command_path}")
            continue
        try:
            command_data = json.loads(command_path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            errors.append(f"started route command.json is unreadable: {command_path}: {exc}")
            continue
        if not isinstance(command_data, dict):
            errors.append(f"started route command.json must contain an object: {command_path}")
            continue
        argv = command_data.get("argv", [])
        if not isinstance(argv, list):
            errors.append(f"started route command argv must be a list: {command_path}")
            continue
        serial = str(argv[-1]) if argv else ""
        if not serial.isdigit() or int(serial) <= 0:
            errors.append(f"started route command lacks a positive numeric fresh-session serial: {command_path}")
        else:
            session_serials.append(serial)
        command_identity = command_data.get("shared_input_identity")
        if not isinstance(command_identity, dict) or not command_identity:
            errors.append(f"command.json shared_input_identity must be an object: {command_path}")
        else:
            expected_command_identity = {
                "n": str(attempt_input.get("n")), "K": str(attempt_input.get("k")),
                "input_sha256": attempt_input.get("input_sha256"),
                "oracle_sha256": attempt_input.get("oracle_sha256"),
            }
            command_oracle_sha = command_identity.get("oracle_mask_sha256", command_identity.get("oracle_sha256"))
            if any(command_identity.get(key) != value for key, value in expected_command_identity.items()
                   if key != "oracle_sha256") or command_oracle_sha != expected_command_identity["oracle_sha256"]:
                errors.append(f"command.json input/oracle identity differs from its attempt record: {command_path}")
            if "input_seed" in command_identity and str(command_identity["input_seed"]) != str(attempt_input.get("seed_id")):
                errors.append(f"command.json input seed differs from its attempt record: {command_path}")
    if len(observed_identities) != 1:
        errors.append("campaign routes/repetitions do not share one input/oracle SHA-256 identity")
    if len(session_serials) != len(set(session_serials)):
        errors.append("campaign reused a fresh-session/material serial")
    return errors


def audit(path: Path) -> list[str]:
    data = json.loads(path.read_text(encoding="utf-8"))
    errors: list[str] = []
    if data.get("schema_version") != SCHEMA_VERSION:
        return ["schema_version must be 3.0"]
    if data.get("record_type") == "attempt":
        if set(data.get("metrics", {})) != set(METRIC_UNITS): errors.append("nine main metric fields mismatch")
        if data.get("metric_units") != METRIC_UNITS: errors.append("metric units mismatch")
        metrics, counts = data.get("metrics", {}), data.get("counts", {})
        for name, value in metrics.items():
            state = data.get("metric_provenance", {}).get(name)
            if (value is None) != (state == "NOT_MEASURED"):
                errors.append(f"null/provenance mismatch: {name}")
        def audit_nulls(prefix: str, value: Any) -> None:
            if isinstance(value, dict):
                for key, child in value.items(): audit_nulls(f"{prefix}.{key}" if prefix else key, child)
            elif value is None and data.get("metric_provenance", {}).get(prefix) != "NOT_MEASURED":
                errors.append(f"null field lacks NOT_MEASURED provenance: {prefix}")
        for section in ("counts", "auxiliary", "network"):
            audit_nulls(section, data.get(section, {}))
        if data.get("terminal_status") == "SUCCESS":
            if not all(data.get("correctness", {}).get(key) is True for key in
                       ("oracle_checked", "oracle_equal", "mask_binary", "mask_weight_k", "mask_published")):
                errors.append("SUCCESS lacks oracle and exact-mask assertions")
            if data.get("correctness", {}).get("mask_length") != data.get("input", {}).get("n"):
                errors.append("SUCCESS mask length mismatch")
            if data.get("input", {}).get("oracle_fnv64") != data.get("input", {}).get("shared_oracle_fnv64"):
                errors.append("SUCCESS route oracle digest differs from shared frozen oracle")
        elif data.get("correctness", {}).get("mask_published"):
            errors.append("failed attempt reports a published mask")
        if metrics.get("online_comm_total_bits") is not None:
            s0, s1, r0, r1 = (counts.get(name) for name in
                              ("p0_sent_bytes", "p1_sent_bytes", "p0_received_bytes", "p1_received_bytes"))
            if None in (s0, s1, r0, r1): errors.append("communication lacks party counters")
            else:
                if metrics["online_comm_total_bits"] != 8 * (s0 + s1): errors.append("total communication identity mismatch")
                if metrics["online_comm_per_party_bits"] != metrics["online_comm_total_bits"] / 2:
                    errors.append("per-party communication is not total/2")
                if s0 != r1 or s1 != r0: errors.append("sent/received counters do not reconcile")
        if metrics.get("total_time_ms") is not None and abs(metrics["total_time_ms"] - metrics["offline_time_ms"] - metrics["online_time_ms"]) > 0.002:
            errors.append("same-attempt total time mismatch")
        if metrics.get("comparison_edges_total") is not None:
            route = data.get("route")
            if route == ROUTES[0]:
                expected = counts.get("raw_score_ucmp_calls", -1) + counts.get("protocol_core_unique_edges", -1)
            elif route == ROUTES[1]:
                expected = counts.get("raw_score_secure_comparison_edges", -1) + counts.get("protocol_active_unique_edges", -1)
            else:
                expected = counts.get("raw_score_secure_comparison_edges", -1) + counts.get("select_unique_real_edges", -1) + counts.get("membership_secure_edges", -1)
            if metrics["comparison_edges_total"] != expected: errors.append("comparison edge identity mismatch")
        manifest = data.get("evidence", {}).get("manifest_sha256")
        manifest_path = path.parent / "evidence_manifest.json"
        if not manifest_path.is_file() or sha256_file(manifest_path) != manifest:
            errors.append("evidence manifest hash mismatch")
        else:
            listed = json.loads(manifest_path.read_text(encoding="utf-8")).get("files", [])
            if listed != data.get("evidence", {}).get("files", []): errors.append("record evidence list differs from manifest")
            for entry in listed:
                file_path = path.parent / entry.get("path", "")
                if not file_path.is_file() or file_path.is_symlink() or sha256_file(file_path) != entry.get("sha256"):
                    errors.append(f"evidence file missing or hash mismatch: {entry.get('path')}")
    elif data.get("record_type") == "campaign":
        refs = data.get("attempts", [])
        loaded_attempts: list[dict[str, Any]] = []
        if set(data.get("route_set", [])) != set(ROUTES): errors.append("campaign route set incomplete")
        expected_routes = set(ROUTES)
        for route in expected_routes:
            chosen = [x for x in refs if x.get("route") == route]
            if data.get("campaign_type") == "FORMAL":
                warm = [x for x in chosen if x.get("attempt_type") == "WARMUP"]
                formal = [x for x in chosen if x.get("attempt_type") == "FORMAL"]
                if len(warm) != 1 or len(formal) != 5 or sorted(x.get("run_index") for x in formal) != [1, 2, 3, 4, 5]:
                    errors.append(f"formal route repetition contract mismatch: {route}")
            elif len(chosen) != 1 or chosen[0].get("attempt_type") != "DIAGNOSTIC":
                errors.append(f"diagnostic route attempt count mismatch: {route}")
        for ref in refs:
            record_path = Path(ref.get("record_path", ""))
            if not record_path.is_file() or sha256_file(record_path) != ref.get("record_sha256"):
                errors.append(f"attempt record hash mismatch: {record_path}")
                continue
            attempt_data = json.loads(record_path.read_text(encoding="utf-8"))
            loaded_attempts.append(attempt_data)
            if attempt_data.get("attempt_id") != ref.get("attempt_id") or attempt_data.get("terminal_status") != ref.get("terminal_status"):
                errors.append(f"campaign reference identity mismatch: {record_path}")
            attempt_errors = audit(record_path)
            errors.extend(f"{record_path}: {error}" for error in attempt_errors)
        if loaded_attempts:
            errors.extend(audit_campaign_identity(data, loaded_attempts))
            shared_fnvs = {r.get("input", {}).get("shared_oracle_fnv64") for r in loaded_attempts}
            expected_fnv = data.get("configuration", {}).get("oracle_fnv64")
            has_success = any(r.get("terminal_status") == "SUCCESS" for r in loaded_attempts)
            if expected_fnv is None:
                if has_success or any(value is not None for value in shared_fnvs):
                    errors.append("campaign is missing its shared oracle FNV64 identity")
            elif len(shared_fnvs) != 1 or None in shared_fnvs or shared_fnvs != {expected_fnv}:
                errors.append("campaign shared oracle FNV64 mismatch")
            actual_fnvs = {r.get("input", {}).get("oracle_fnv64") for r in loaded_attempts
                           if r.get("terminal_status") == "SUCCESS"}
            if actual_fnvs not in (set(), {expected_fnv}):
                errors.append("campaign route oracle FNV64 mismatch")
        if data.get("campaign_type") == "FORMAL":
            if data.get("phase_gate") != "V3_CLOSED": errors.append("formal campaign without V3_CLOSED")
            if data.get("network", {}).get("mode") not in {"LAN", "WAN"}: errors.append("formal campaign missing LAN/WAN mode")
    else:
        errors.append("record_type must be attempt or campaign")
    return errors


def audit_cli(args: argparse.Namespace) -> int:
    errors = audit(Path(args.path).resolve())
    if errors:
        for error in errors: print("audit_error=" + error, file=sys.stderr)
        return 1
    print("audit=PASS")
    return 0


def seal_campaign(args: argparse.Namespace) -> int:
    gate = json.loads(Path(args.v3_gate_record).read_text(encoding="utf-8"))
    if gate.get("v3_status", gate.get("status")) != "PASS": raise ValueError("V3 gate record does not report PASS")
    meta = json.loads(Path(args.meta).read_text(encoding="utf-8"))
    network = meta.get("network", {})
    if network.get("mode") not in {"LAN", "WAN"} or not isinstance(network.get("rtt_ms"), (int, float)) or not isinstance(network.get("throughput_mbit_s"), (int, float)) or not network.get("topology_id"):
        raise ValueError("network record requires measured LAN/WAN RTT, throughput and topology")
    evidence_path = Path(network.get("evidence_path", ""))
    if not evidence_path.is_file() or sha256_file(evidence_path) != network.get("evidence_sha256"):
        raise ValueError("network measurement evidence is missing or its SHA-256 differs")
    attempt_paths = [Path(p).resolve() for p in args.attempt_record]
    if len(attempt_paths) != 18: raise ValueError("formal tri-route campaign requires 3 routes × (1 warmup + 5 formal attempts)")
    records = []
    for path in attempt_paths:
        problems = audit(path)
        if problems: raise ValueError(f"attempt audit failed for {path}: {problems}")
        record = json.loads(path.read_text(encoding="utf-8"))
        if record.get("record_type") != "attempt": raise ValueError(f"not an attempt record: {path}")
        records.append((path, record))
    campaign_id = meta.get("campaign_id")
    if not campaign_id or any(r.get("campaign_id") != campaign_id for _, r in records):
        raise ValueError("attempt campaign_id does not match campaign metadata")
    cfg = meta.get("configuration", {})
    if not (2 <= cfg.get("n", 0) <= 1000 and 1 <= cfg.get("k", 0) <= cfg.get("n", 0)):
        raise ValueError("campaign metadata n/K is invalid")
    if any((r.get("input", {}).get("n"), r.get("input", {}).get("k")) != (cfg["n"], cfg["k"]) for _, r in records):
        raise ValueError("attempt input shape differs from campaign")
    hashes = {r.get("input", {}).get("input_sha256") for _, r in records}
    oracles = {r.get("input", {}).get("oracle_sha256") for _, r in records}
    if len(hashes) != 1 or None in hashes or len(oracles) != 1 or None in oracles:
        raise ValueError("all routes/repetitions must use one common input and oracle hash")
    shared_fnvs = {r.get("input", {}).get("shared_oracle_fnv64") for _, r in records}
    actual_fnvs = {r.get("input", {}).get("oracle_fnv64") for _, r in records if r.get("terminal_status") == "SUCCESS"}
    if len(shared_fnvs) != 1 or None in shared_fnvs or not actual_fnvs.issubset(shared_fnvs):
        raise ValueError("all successful routes/repetitions must match the shared frozen oracle FNV64")
    identity_errors = audit_campaign_identity(
        {"campaign_id": campaign_id,
         "configuration": {"n": cfg["n"], "k": cfg["k"],
                           "input_seed_id": meta.get("input_seed_id"),
                           "input_sha256": next(iter(hashes)),
                           "oracle_sha256": next(iter(oracles))}},
        [record for _, record in records])
    if identity_errors:
        raise ValueError(f"campaign identity audit failed: {identity_errors}")
    for route in ROUTES:
        chosen = [(p, r) for p, r in records if r.get("route") == route]
        warmups = [r for _, r in chosen if r.get("attempt_type") == "WARMUP"]
        formal = [r for _, r in chosen if r.get("attempt_type") == "FORMAL"]
        if len(warmups) != 1 or len(formal) != 5 or sorted(r.get("run_index") for r in formal) != [1, 2, 3, 4, 5]:
            raise ValueError(f"{route} must have one warmup and formal run indices 1..5")
    for _, r in records:
        rn = r.get("network", {})
        for key in ("mode", "rtt_ms", "throughput_mbit_s", "topology_id"):
            if rn.get(key) != network.get(key): raise ValueError(f"attempt network mismatch: {key}")
        if rn.get("mode") not in {"LAN", "WAN"}: raise ValueError("attempt lacks formal LAN/WAN identity")
    out = Path(args.out_dir).resolve()
    out.mkdir(parents=True, exist_ok=True)
    refs = [{"route": r["route"], "attempt_id": r["attempt_id"], "run_index": r["run_index"],
             "attempt_type": r["attempt_type"], "record_path": str(path), "record_sha256": sha256_file(path),
             "terminal_status": r["terminal_status"]} for path, r in records]
    campaign = {"schema_version": SCHEMA_VERSION, "record_type": "campaign", "campaign_id": campaign_id,
                "campaign_type": "FORMAL", "phase_gate": "V3_CLOSED", "route_set": list(ROUTES),
                "configuration": {**cfg, "input_seed_id": meta.get("input_seed_id"),
                                  "input_sha256": next(iter(hashes)), "oracle_sha256": next(iter(oracles)),
                                  "oracle_fnv64": next(iter(shared_fnvs))},
                "network": {k: network.get(k) for k in ("mode", "rtt_ms", "throughput_mbit_s", "topology_id", "netem_id", "evidence_sha256", "evidence_path", "measurement_method")},
                "warmups_per_route": 1, "formal_repetitions_per_route": 5, "attempts": refs,
                "campaign_status": "COMPLETE",
                "campaign_outcome": "ALL_SUCCEEDED" if all(r["terminal_status"] == "SUCCESS" for _, r in records) else "ATTEMPT_FAILURES_RETAINED"}
    path = out / "campaign_record.json"
    path.write_text(json.dumps(campaign, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"campaign status=COMPLETE outcome={campaign['campaign_outcome']} record={path}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subs = parser.add_subparsers(dest="cmd", required=True)
    pf = subs.add_parser("preflight")
    pf.add_argument("--n", type=int, required=True); pf.add_argument("--k", type=int, required=True)
    pf.add_argument("--scratch-path", default="/tmp"); pf.set_defaults(func=preflight)
    run = subs.add_parser("collect", help="paired per-attempt collector; each attempt uses fresh session/material")
    run.add_argument("--campaign-type", choices=["DIAGNOSTIC", "FORMAL"], default="DIAGNOSTIC")
    run.add_argument("--n", type=int, required=True); run.add_argument("--k", type=int, required=True)
    run.add_argument("--r", type=int, default=2); run.add_argument("--input-seed", type=int, required=True)
    run.add_argument("--all-pairs", required=True); run.add_argument("--aav86", required=True)
    run.add_argument("--bmw-e2e", required=True); run.add_argument("--bmw-node", required=True)
    run.add_argument("--e16-repo", required=True); run.add_argument("--bmw-repo", required=True)
    run.add_argument("--e16-cmake-cache", required=True); run.add_argument("--bmw-cmake-cache", required=True)
    run.add_argument("--oracle-source", required=True); run.add_argument("--out-dir", required=True)
    run.add_argument("--timeout-seconds", type=int, default=1800)
    run.add_argument("--network-mode", choices=["loopback_diagnostic", "LAN", "WAN"], default="loopback_diagnostic")
    run.add_argument("--rtt-ms", type=float); run.add_argument("--throughput-mbit-s", type=float)
    run.add_argument("--topology-id"); run.add_argument("--netem-id")
    run.add_argument("--v3-gate-record"); run.add_argument("--warmups", type=int, default=1)
    run.set_defaults(func=collect)
    audit_parser = subs.add_parser("audit", help="audit one attempt/campaign and its raw evidence hashes")
    audit_parser.add_argument("path"); audit_parser.set_defaults(func=audit_cli)
    seal = subs.add_parser("seal-campaign", help="validate and index a V3-gated server 1+5 tri-route campaign")
    seal.add_argument("--v3-gate-record", required=True); seal.add_argument("--meta", required=True)
    seal.add_argument("--attempt-record", action="append", required=True)
    seal.add_argument("--out-dir", required=True); seal.set_defaults(func=seal_campaign)
    args = parser.parse_args()
    try: return args.func(args)
    except Exception as exc:
        print(f"runner_error={exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
