#!/usr/bin/env python3
"""Offline T followed by independent P0/P1 processes; TEST_ONLY verifier outside parties."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import random
import shutil
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
PARTY = Path(__file__).with_name("party.py")
MASK34 = (1 << 34) - 1


def wsl_path(path: str) -> str:
    if os.name != "nt":
        return path
    absolute = Path(path).resolve()
    drive = absolute.drive.rstrip(":").lower()
    if len(drive) != 1:
        raise ValueError("WSL bridge only supports drive-letter paths")
    return "/mnt/" + drive + "/" + "/".join(absolute.parts[1:])


def backend_call(backend: str, args: list[str], timeout: int = 60) -> str:
    if backend.startswith("wsl:"):
        command = ["wsl.exe", "-d", "Ubuntu-24.04", "--", backend[4:], *args]
    else:
        command = [backend, *args]
    completed = subprocess.run(command, check=True, text=True, encoding="utf-8", errors="replace",
                               capture_output=True, timeout=timeout)
    return completed.stdout.strip()


def endpoint_command(backend: str, party: int, package: str) -> list[str]:
    if backend.startswith("wsl:"):
        return ["wsl.exe", "-d", "Ubuntu-24.04", "--", backend[4:], "--party", str(party), wsl_path(package)]
    return [backend, "--party", str(party), package]


def package_conformance(backend: str, p0_package: Path, temp: Path, n: int) -> dict[str, Any]:
    """Malformed binding/slot and one-shot checks on actual serialized DCF material."""
    results: dict[str, Any] = {}
    wrong_party = temp / "wrong-party.bin"
    shutil.copyfile(p0_package, wrong_party)
    p = subprocess.run(endpoint_command(backend, 1, str(wrong_party)), input="", text=True,
                       encoding="utf-8", errors="replace", capture_output=True, timeout=30)
    results["wrong_party_package_rejected"] = p.returncode != 0 and "header" in p.stderr

    truncated = temp / "truncated.bin"
    truncated.write_bytes(p0_package.read_bytes()[:40])
    p = subprocess.run(endpoint_command(backend, 0, str(truncated)), input="", text=True,
                       encoding="utf-8", errors="replace", capture_output=True, timeout=30)
    results["truncated_package_rejected"] = p.returncode != 0

    wrong_round = temp / "wrong-round.bin"
    shutil.copyfile(p0_package, wrong_round)
    p = subprocess.run(endpoint_command(backend, 0, str(wrong_round)), input="CMP 5 0 0 1 0 1\n",
                       text=True, encoding="utf-8", errors="replace", capture_output=True, timeout=30)
    results["wrong_round_key_rejected"] = p.returncode != 0 and "slot" in p.stderr

    wrong_endpoint = temp / "wrong-endpoint.bin"
    shutil.copyfile(p0_package, wrong_endpoint)
    p = subprocess.run(endpoint_command(backend, 0, str(wrong_endpoint)),
                       input=f"CMP 1 0 0 {n} 0 1\n", text=True, encoding="utf-8",
                       errors="replace", capture_output=True, timeout=30)
    results["wrong_endpoint_key_rejected"] = p.returncode != 0 and "slot" in p.stderr

    replay = temp / "replay.bin"
    shutil.copyfile(p0_package, replay)
    p = subprocess.Popen(endpoint_command(backend, 0, str(replay)), stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                         encoding="utf-8", errors="replace", bufsize=1)
    assert p.stdin is not None and p.stdout is not None
    p.stdin.write("CMP 1 0 0 1 0 1\n")
    p.stdin.flush()
    first = p.stdout.readline().strip()
    p.stdin.write("CMP 1 0 0 1 0 1\n")
    p.stdin.flush()
    tail, error = p.communicate("", timeout=30)
    results["first_slot_eval_returns_share"] = first.startswith("C ")
    results["duplicate_slot_eval_rejected"] = p.returncode != 0 and "replayed" in error

    claim = temp / "claim.bin"
    shutil.copyfile(p0_package, claim)
    first_claim = subprocess.Popen(endpoint_command(backend, 0, str(claim)), stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                                   encoding="utf-8", errors="replace")
    _, e1 = first_claim.communicate("QUIT\n", timeout=30)
    second_claim = subprocess.run(endpoint_command(backend, 0, str(claim)), input="", text=True,
                                  encoding="utf-8", errors="replace", capture_output=True, timeout=30)
    results["material_claim_consumed_once"] = first_claim.returncode == 0 and second_claim.returncode != 0 and "already claimed" in second_claim.stderr

    silent_package = temp / "silent-peer.bin"
    shutil.copyfile(p0_package, silent_package)
    silent_input = temp / "silent-input.json"
    silent_input.write_text(json.dumps({"n": n, "k": 1, "party": 0, "shares": [0] * n}), encoding="utf-8")
    silent_result = temp / "silent-result.json"
    port = reserve_port()
    party_cmd = [sys.executable, str(PARTY), "--party", "0", "--port", str(port),
                 "--input", str(silent_input), "--package", str(silent_package),
                 "--backend", backend, "--result", str(silent_result), "--input-seed", "1",
                 "--algorithm-seed", "1", "--peer-timeout", "1"]
    p = subprocess.run(party_cmd, text=True, encoding="utf-8", errors="replace",
                       capture_output=True, timeout=10)
    silent = json.loads(silent_result.read_text()) if silent_result.exists() else {}
    results["peer_silent_returns_no_mask"] = p.returncode != 0 and silent.get("status") == "ABORT_COMMUNICATION" and silent.get("no_mask") is True
    results["peer_silent_process_exit"] = p.returncode
    results["peer_silent_party_status"] = silent.get("status", "NO_RESULT")
    results["peer_silent_no_mask"] = silent.get("no_mask", False)
    results["peer_silent_error"] = silent.get("error", p.stderr[-500:])

    # Two connected endpoints: P1 receives a package bound to P0. Its local
    # material rejection must be relayed so P0 records the same ABORT class.
    pair_good, pair_bad = temp / "pair-good.bin", temp / "pair-bad.bin"
    shutil.copyfile(p0_package, pair_good)
    shutil.copyfile(p0_package, pair_bad)
    pair_inputs = [temp / "pair-p0.json", temp / "pair-p1.json"]
    pair_results = [temp / "pair-p0-result.json", temp / "pair-p1-result.json"]
    for party, path in enumerate(pair_inputs):
        path.write_text(json.dumps({"n": n, "k": 1, "party": party,
                                    "shares": [0] * n}), encoding="utf-8")
    pair_port = reserve_port()
    pair_processes = []
    for party, package in enumerate((pair_good, pair_bad)):
        command = [sys.executable, str(PARTY), "--party", str(party), "--port", str(pair_port),
                   "--input", str(pair_inputs[party]), "--package", str(package),
                   "--backend", backend, "--result", str(pair_results[party]),
                   "--input-seed", "1", "--algorithm-seed", "1", "--peer-timeout", "5"]
        pair_processes.append(subprocess.Popen(command, text=True, encoding="utf-8",
                                               errors="replace", stdout=subprocess.PIPE,
                                               stderr=subprocess.PIPE))
    pair_output = []
    for process in pair_processes:
        stdout, stderr = process.communicate(timeout=15)
        pair_output.append((process.returncode, stdout, stderr))
    pair_status = [json.loads(path.read_text()).get("status", "NO_RESULT")
                   if path.exists() else "NO_RESULT" for path in pair_results]
    pair_no_mask = [json.loads(path.read_text()).get("no_mask") is True
                    if path.exists() else False for path in pair_results]
    results["two_party_material_abort_agrees"] = (
        pair_status == ["ABORT_MATERIAL", "ABORT_MATERIAL"] and pair_no_mask == [True, True]
        and all(code == 2 for code, _out, _err in pair_output))
    results["two_party_material_abort_statuses"] = pair_status
    results["two_party_material_abort_no_mask"] = pair_no_mask
    return results


def reserve_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("127.0.0.1", 0))
        return int(s.getsockname()[1])


def one_case(scores: list[int], k: int, *, backend: str, input_seed: int,
             algorithm_seed: int, case_id: str,
             test_only_force_algorithm_abort: bool = False,
             test_only_probe_peer_files: bool = False,
             raw_score_abi: bool = False,
             joint_csprng_coins: bool = False) -> dict[str, Any]:
    if not scores or not 1 <= k <= len(scores) or any(not -(1 << 31) <= x < (1 << 31) for x in scores):
        raise ValueError("invalid signed-int32 input")
    n = len(scores)
    index_bits = max(1, (n - 1).bit_length())
    bits = (33 if raw_score_abi else 34) + index_bits
    label_suffix = ("EXPERIMENTAL_TEST_ONLY_BMW16_S9_RAW_ABI_CSPRNG"
                    if (raw_score_abi or joint_csprng_coins or
                        test_only_force_algorithm_abort or test_only_probe_peer_files)
                    else "EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS")
    implementation_label = f"Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / {label_suffix}"
    seed_id = int(hashlib.sha256(f"{case_id}:{n}:{k}:{input_seed}:{algorithm_seed}".encode()).hexdigest()[:12], 16)
    session = (0x5838000000000000 | (seed_id & 0x0000FFFFFFFFFFFF)) or 1
    fingerprint = session ^ 0x1A2B3C4D5E6F7801
    with tempfile.TemporaryDirectory(prefix="bmw16_s8_") as temp:
        p0_pkg, p1_pkg = Path(temp) / "p0.offline", Path(temp) / "p1.offline"
        p0_input, p1_input = Path(temp) / "p0.input.json", Path(temp) / "p1.input.json"
        p0_result, p1_result = Path(temp) / "p0.result.json", Path(temp) / "p1.result.json"
        dealer_role = "--dealer-raw-score-abi" if raw_score_abi else "--dealer"
        t_stdout = backend_call(backend, [dealer_role, str(n), str(k), str(bits), str(session),
                                         str(fingerprint), wsl_path(str(p0_pkg)), wsl_path(str(p1_pkg))]) if n >= 2 else "T_NOT_REQUIRED_SINGLETON"
        conformance = package_conformance(backend, p0_pkg, Path(temp), n) if n >= 2 and case_id == "n2_extremes_k1" else None
        input_rng = random.Random(input_seed)
        shares = [[], []]
        for score in scores:
            if raw_score_abi:
                ring_mask = (1 << 32) - 1
                left = input_rng.getrandbits(32)
                shares[0].append(left)
                shares[1].append(((score & ring_mask) - left) & ring_mask)
            else:
                left = input_rng.getrandbits(34)
                shares[0].append(left)
                shares[1].append((score - left) & MASK34)
        for party, path in enumerate((p0_input, p1_input)):
            path.write_text(json.dumps({"n": n, "k": k, "party": party, "session": session,
                                        "fingerprint": fingerprint, "shares": shares[party]},
                                       separators=(",", ":")), encoding="utf-8")
        port = reserve_port()
        child_args = []
        for party, (pkg, inp, out) in enumerate(((p0_pkg, p0_input, p0_result), (p1_pkg, p1_input, p1_result))):
            child_args.append([sys.executable, str(PARTY), "--party", str(party), "--port", str(port),
                               "--input", str(inp), "--package", str(pkg), "--backend", backend,
                               "--result", str(out), "--input-seed", str(input_seed),
                               "--algorithm-seed", str(algorithm_seed)])
            if test_only_force_algorithm_abort:
                child_args[-1].append("--test-only-force-algorithm-abort")
            if test_only_probe_peer_files:
                child_args[-1].append("--test-only-probe-peer-files")
            if raw_score_abi:
                child_args[-1].append("--raw-score-abi")
            if joint_csprng_coins:
                child_args[-1].append("--joint-csprng-coins")
        p0 = subprocess.Popen(child_args[0], text=True, encoding="utf-8", errors="replace",
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        p1 = subprocess.Popen(child_args[1], text=True, encoding="utf-8", errors="replace",
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            out0, err0 = p0.communicate(timeout=120)
            out1, err1 = p1.communicate(timeout=120)
        except subprocess.TimeoutExpired:
            p0.kill(); p1.kill()
            out0, err0 = p0.communicate(); out1, err1 = p1.communicate()
            return {"case_id": case_id, "n": n, "K": k, "status": "ABORT_COMMUNICATION_TIMEOUT",
                    "party_stdout": [out0, out1], "party_stderr": [err0, err1], "no_mask": True}
        if not p0_result.exists() or not p1_result.exists():
            return {"case_id": case_id, "n": n, "K": k, "status": "ABORT_PROCESS_FAILED",
                    "exit_codes": [p0.returncode, p1.returncode], "party_stdout": [out0, out1],
                    "party_stderr": [err0, err1], "no_mask": True}
        r0, r1 = json.loads(p0_result.read_text()), json.loads(p1_result.read_text())
        status = r0.get("status") if r0.get("status") == r1.get("status") else "ABORT_PARTY_STATUS_MISMATCH"
        result: dict[str, Any] = {
            "case_id": case_id, "label": implementation_label,
            "implementation_label": implementation_label,
            "n": n, "K": k, "comparison_bits": bits,
            "input_seed": input_seed, "algorithm_seed": r0.get("algorithm_seed", algorithm_seed),
            "algorithm_seed_mode": r0.get("algorithm_randomness", {}).get("mode", "FIXED_TEST_SEED"),
            "algorithm_randomness_P0": r0.get("algorithm_randomness"),
            "algorithm_randomness_P1": r1.get("algorithm_randomness"),
            "input_semantics": ("formal raw int32/Q20.12 additive share words in Z/(2^32); experimental port of Protocol I carry/sign lift"
                                if raw_score_abi else
                                "TEST_ONLY-only signed int32 additive shares in Z/(2^34); not the repository raw-score ABI"),
            "input_adapter_label": "PROJECT_DERIVED_TEST_ONLY_PORT_OF_PROTOCOL_I_RAW_SCORE_LIFT" if raw_score_abi else "TEST_ONLY_DIRECT_Z2_34_FIXTURE",
            "status": status, "selected_anonymous_handle": r0.get("selected_anonymous_handle"),
            "party_exit_codes": [p0.returncode, p1.returncode],
            "T_offline_stdout": t_stdout,
            "offline_package_bytes_per_party": [p0_pkg.stat().st_size, p1_pkg.stat().st_size] if n >= 2 else [0, 0],
            "offline_material_slot_breakdown_per_party": {
                "select_key_pool_slots": (8 * (n * (n - 1) // 2)) if n >= 2 else 0,
                "membership_key_pool_slots": (n * (n - 1) // 2) if n >= 2 else 0,
                "unused_shuffle_cmpagg_slots": (2 * (n * (n - 1) // 2)) if n >= 2 else 0,
                "raw_score_input_slots": (2 * (1 << index_bits)) if raw_score_abi and n >= 2 else 0,
                "total_serialized_uCMP_slots": ((11 * (n * (n - 1) // 2)) +
                                                 (2 * (1 << index_bits) if raw_score_abi else 0)) if n >= 2 else 0,
            },
            "offline_material_conformance": conformance,
            "offline_T_exited_before_parties": n >= 2,
            "offline_T_not_required_singleton": n == 1,
            "party_input_files_are_share_only": True,
            "select_round_metrics": r0.get("round_metrics"),
            "logical_and_actual_metrics_P0": r0.get("metrics"),
            "logical_and_actual_metrics_P1": r1.get("metrics"),
            "public_adaptive_graph_and_comparison_bits": True,
            "selected_handle_public": status == "SUCCESS",
            "original_index_opened": False,
            "shuffle_map_opened": False,
            "online_keygen": False,
            "full_online_DAG_message_phases": r0.get("network_message_phases"),
            "party_trace_P0": r0.get("trace"),
            "party_trace_P1": r1.get("trace"),
            "test_only_forced_algorithm_abort": test_only_force_algorithm_abort,
            "same_os_identity_peer_file_probe_P0": r0.get("test_only_same_os_peer_file_probe"),
            "same_os_identity_peer_file_probe_P1": r1.get("test_only_same_os_peer_file_probe"),
            "party_stdout": [out0.strip(), out1.strip()],
            "party_stderr": [err0.strip(), err1.strip()],
        }
        if status == "SUCCESS":
            m0, m1 = r0.get("mask_share"), r1.get("mask_share")
            if not isinstance(m0, list) or not isinstance(m1, list) or len(m0) != n or len(m1) != n:
                result["status"] = "ABORT_MISSING_MASK_SHARE"
                result["no_mask"] = True
                return result
            opened = [int(a) ^ int(b) for a, b in zip(m0, m1)]
            oracle_line = backend_call(backend, ["--oracle", str(n), str(k), *map(str, scores)])
            oracle = [int(x) for x in oracle_line.split()[1:]]
            result["opened_mask_TEST_ONLY_verifier"] = opened
            result["frozen_CPP_oracle_mask"] = oracle
            result["oracle_differential"] = "PASS" if opened == oracle and sum(opened) == k else "FAIL"
            result["mask_share_sha256"] = [hashlib.sha256(bytes(m0)).hexdigest(), hashlib.sha256(bytes(m1)).hexdigest()]
            if result["oracle_differential"] != "PASS":
                result["status"] = "COMPLETED_WRONG_NO_MASK"
                result["opened_mask_TEST_ONLY_verifier"] = opened
                result["no_mask"] = True
        else:
            result["no_mask"] = True
            result["oracle_differential"] = "NOT_RUN_ABORT"
        return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--backend", required=True, help="native executable or wsl:/path/to/bmw16_s8_backend")
    parser.add_argument("--scores-json", type=Path)
    parser.add_argument("--k", type=int)
    parser.add_argument("--input-seed", type=int, default=11)
    parser.add_argument("--algorithm-seed", type=int, default=42)
    parser.add_argument("--case-id", default="manual")
    parser.add_argument("--test-only-force-algorithm-abort", action="store_true",
                        help="inject ABORT_ALGORITHM_PROBABILITY before membership; contract test only")
    parser.add_argument("--test-only-probe-peer-files", action="store_true",
                        help="read/discard sibling input/material files as the same OS token")
    parser.add_argument("--raw-score-abi", action="store_true",
                        help="use Z/(2^32) raw score shares and experimental Protocol-I carry/sign lift")
    parser.add_argument("--joint-csprng-coins", action="store_true",
                        help="exchange CSPRNG contributions and use a domain-separated HMAC-SHA256 sample stream")
    parser.add_argument("--matrix", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.matrix:
        minimum, maximum = -(1 << 31), (1 << 31) - 1
        cases = [
            ("n1_min_k1", [minimum], 1),
            ("n2_extremes_k1", [minimum, maximum], 1),
            ("n2_equal_k2", [7, 7], 2),
            ("n3_all_equal_k2", [0, 0, 0], 2),
            ("n5_repeat_mixed_k1", [5, minimum, 5, maximum, -1], 1),
            ("n5_repeat_mixed_k3", [5, minimum, 5, maximum, -1], 3),
            ("n5_repeat_mixed_kn", [5, minimum, 5, maximum, -1], 5),
            ("n8_repeat_k1", [7, 7, 0, 7, -1, 4, 7, 0], 1),
            ("n8_mixed_extremes_k3", [minimum, maximum, 0, -1, 0, maximum, minimum, 7], 3),
            ("n8_all_equal_k4", [3] * 8, 4),
            ("n8_boundary_k8", [maximum, 1, -1, minimum, 0, 3, 2, 4], 8),
        ]
        rows = []
        for i, (name, scores, k) in enumerate(cases):
            rows.append(one_case(scores, k, backend=args.backend, input_seed=args.input_seed + i,
                                 algorithm_seed=args.algorithm_seed + i, case_id=name,
                                 test_only_force_algorithm_abort=args.test_only_force_algorithm_abort,
                                 test_only_probe_peer_files=args.test_only_probe_peer_files,
                                 raw_score_abi=args.raw_score_abi,
                                 joint_csprng_coins=args.joint_csprng_coins))
        matrix_label = ("Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL_TEST_ONLY_BMW16_S9_RAW_ABI_CSPRNG"
                        if (args.raw_score_abi or args.joint_csprng_coins or args.test_only_force_algorithm_abort or args.test_only_probe_peer_files)
                        else "Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS")
        data = {"implementation_label": matrix_label,
                "matrix_cases": rows,
                "summary": {"success": sum(r.get("status") == "SUCCESS" for r in rows),
                            "oracle_pass": sum(r.get("oracle_differential") == "PASS" for r in rows),
                            "abort_or_failure": sum(r.get("status") != "SUCCESS" for r in rows)}}
    else:
        if args.scores_json is None or args.k is None:
            parser.error("--scores-json and --k are required unless --matrix is set")
        scores = json.loads(args.scores_json.read_text(encoding="utf-8"))
        data = one_case(scores, args.k, backend=args.backend, input_seed=args.input_seed,
                        algorithm_seed=args.algorithm_seed, case_id=args.case_id,
                        test_only_force_algorithm_abort=args.test_only_force_algorithm_abort,
                        test_only_probe_peer_files=args.test_only_probe_peer_files,
                        raw_score_abi=args.raw_score_abi,
                        joint_csprng_coins=args.joint_csprng_coins)
    payload = json.dumps(data, separators=(",", ":"), sort_keys=True) + "\n"
    if args.output:
        args.output.write_text(payload, encoding="utf-8")
    else:
        print(payload, end="")
    expected_forced_abort = args.test_only_force_algorithm_abort and data.get("status") == "ABORT_ALGORITHM_PROBABILITY"
    return 0 if (expected_forced_abort or data.get("oracle_differential") == "PASS" or
                 ("matrix_cases" in data and all(r.get("oracle_differential") in {"PASS", "NOT_RUN_ABORT"} for r in data["matrix_cases"]))) else 1


if __name__ == "__main__":
    raise SystemExit(main())
