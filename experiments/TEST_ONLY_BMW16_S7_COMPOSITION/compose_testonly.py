#!/usr/bin/env python3
"""TEST_ONLY raw-score -> real FSS Select -> DCF membership -> original mask."""
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path
from typing import Any

from fss_adapter import FssBackend, stable_low_to_high_key
from select4r_s4_source import run_select


MASK64 = (1 << 64) - 1
LABEL = "Protocol_I_BMW16_DERIVED_Select_DCF_TEST_ONLY_CENTRAL_OPEN"


def send_shuffle(backend: FssBackend, scores: list[int], shuffle_seed: int) -> tuple[list[int], int, int]:
    n = len(scores)
    if n == 1:
        # The underlying shuffle primitive has n>=2; singleton identity is
        # an explicit TEST_ONLY boundary rule and needs no permutation material.
        return [0], 0, 0
    response = backend._request("S %d %d %s" % (
        n, shuffle_seed, " ".join(str(score & 0xFFFFFFFF) for score in scores)))
    if len(response) != n + 4 or response[0] != "S" or int(response[1]) != n:
        raise RuntimeError(f"malformed real-shuffle response: {response}")
    shuffle_cmpagg_slots = int(response[2])
    shuffle_cmpagg_key_wire_bytes = int(response[3])
    permutation = [int(value) for value in response[4:]]
    if sorted(permutation) != list(range(n)):
        raise RuntimeError("actual shuffle output did not carry each input record once")
    return permutation, shuffle_cmpagg_slots, shuffle_cmpagg_key_wire_bytes


def oracle_mask(backend: FssBackend, scores: list[int], k: int) -> list[int]:
    response = backend._request("O %d %d %s" % (
        len(scores), k, " ".join(str(score & 0xFFFFFFFF) for score in scores)))
    if len(response) != len(scores) + 2 or response[0] != "O":
        raise RuntimeError("malformed frozen-oracle response")
    return [int(x) for x in response[2:]]


def run_case(scores: list[int], k: int, input_seed: int, algorithm_seed: int,
             shuffle_seed: int, fss_seed: int, executable: Path,
             trace: bool = False) -> dict[str, Any]:
    if not scores or not 1 <= k <= len(scores):
        raise ValueError("require n>0 and 1<=K<=n")
    if len(scores) > 16 or any(not -(1 << 31) <= score < (1 << 31) for score in scores):
        raise ValueError("TEST_ONLY composition currently supports signed-int32 n<=16")
    n = len(scores)
    backend = FssBackend(executable, fss_seed)
    try:
        permutation, shuffle_cmpagg_slots, shuffle_cmpagg_key_wire_bytes = send_shuffle(
            backend, scores, shuffle_seed)
        shuffled_scores = [scores[index] for index in permutation]
        select_result = run_select(
            shuffled_scores, k, input_seed, algorithm_seed, trace=trace,
            original_indices=permutation, fss_backend=backend,
        )
        all_round_calls = sum(int(v["comparison_calls"]) for v in select_result["round_metrics"].values())
        all_round_slots = sum(int(v.get("fss_material_slots", 0)) for v in select_result["round_metrics"].values())
        xor0: list[int] | None = None
        xor1: list[int] | None = None
        oracle: list[int] | None = None
        candidate_opened_for_test_diagnostic: list[int] | None = None
        membership_calls = 0
        status = select_result["status"]
        oracle_differential = "NOT_RUN_NO_MASK"
        if status == "SUCCESS":
            selected_index = int(select_result["selected_original_index"])
            threshold = stable_low_to_high_key(scores[selected_index], selected_index, n)
            comparator_bits = 34 + max(1, (n - 1).bit_length())
            shuffled_mask0 = [0] * n
            shuffled_mask1 = [0] * n
            for slot, original_index in enumerate(permutation):
                record_key = stable_low_to_high_key(scores[original_index], original_index, n)
                c0, c1 = backend.compare_values(record_key, threshold, bits=comparator_bits)
                membership0 = (1 - c0) & MASK64
                membership1 = (-c1) & MASK64
                # Local LSB mapping from additive Z/(2^64) bit shares to XOR bit shares.
                shuffled_mask0[slot] = membership0 & 1
                shuffled_mask1[slot] = membership1 & 1
                membership_calls += 1
            xor0 = [0] * n
            xor1 = [0] * n
            for slot, original_index in enumerate(permutation):
                xor0[original_index] = shuffled_mask0[slot]
                xor1[original_index] = shuffled_mask1[slot]
            oracle = oracle_mask(backend, scores, k)
            opened = [a ^ b for a, b in zip(xor0, xor1)]
            candidate_opened_for_test_diagnostic = opened
            if opened == oracle and sum(opened) == k:
                oracle_differential = "PASS"
            else:
                oracle_differential = "FAIL"
                status = "COMPLETED_WRONG"
                # Preserve only an explicitly TEST_ONLY diagnostic; the
                # API-shaped output fields remain absent on any failure.
                xor0 = None
                xor1 = None
        backend.stop()
        result: dict[str, Any] = {
            "label": LABEL,
            "classification": "TEST_ONLY_CENTRAL_CONTROLLER_OPENS_SHUFFLE_AND_COMPARISON_SHARES",
            "status": status,
            "n": n,
            "K": k,
            "input_scores_signed_int32": scores,
            "input_seed": input_seed,
            "algorithm_seed": algorithm_seed,
            "shuffle_request_seed": shuffle_seed,
            "FSS_PRNG_seed": fss_seed,
            "actual_OS_shuffle_randomness": "getrandom(2) inside repository primitive; shuffle_request_seed is only a transcript label",
            "revealed_shuffled_to_original_index_map": permutation,
            "actual_forward_shuffle_online_rounds_in_primitive": 2 if n > 1 else 0,
            "shuffle_cmpagg_key_slots_generated_unused": shuffle_cmpagg_slots,
            "shuffle_cmpagg_combined_party_key_wire_bytes_generated_unused": shuffle_cmpagg_key_wire_bytes,
            "shuffle_cmpagg_DCF_Evals": 0,
            "inverse_route": "central TEST_ONLY scatter using opened shuffled-to-original index map; no secure inverse shuffle is composed",
            "selected_original_index": select_result["selected_original_index"],
            "select_result_class": select_result["result_class"],
            "partition_statuses": select_result["partition_statuses"],
            "round_metrics": select_result["round_metrics"],
            "select_logical_comparison_calls": all_round_calls,
            "select_unique_one_shot_uCMP_material_slots": all_round_slots,
            "select_two_party_DCF_Eval_count": 4 * all_round_slots,
            "membership_logical_calls": membership_calls,
            "membership_uCMP_material_slots": membership_calls,
            "membership_two_party_DCF_Eval_count": 4 * membership_calls,
            "actual_select_comparator": "VFSS/src/moe_topk/protocol_i_ucmp.cpp ProtocolIUcmpMaterial; DCF keys generated per TEST_ONLY request",
            "stable_key_comparison_bits": 34 + max(1, (n - 1).bit_length()),
            "actual_shuffle_primitive": "VFSS/src/moe_topk/protocol_i_parallel_shuffle.cpp; both party state machines centralized in harness",
            "mask_xor_share_P0": xor0,
            "mask_xor_share_P1": xor1,
            "candidate_mask_opened_for_test_diagnostic": candidate_opened_for_test_diagnostic
                if status == "COMPLETED_WRONG" else None,
            "oracle_mask_from_frozen_cpp_topk_oracle": oracle,
            "oracle_differential": oracle_differential,
            "mask_delivered_on_failure": False,
            "input_sha256": hashlib.sha256(json.dumps(scores, separators=(",", ":")).encode()).hexdigest(),
            "run_select": select_result,
        }
        return result
    finally:
        if backend.process.poll() is None:
            backend.process.kill()
            backend.process.wait(timeout=5)


def main() -> int:
    parser = argparse.ArgumentParser(description=LABEL)
    parser.add_argument("--scores-json", required=True, type=Path)
    parser.add_argument("--k", required=True, type=int)
    parser.add_argument("--fss-backend", required=True,
                        help="native executable path or wsl:/absolute/linux/path")
    parser.add_argument("--input-seed", default=0, type=int)
    parser.add_argument("--algorithm-seed", required=True, type=int)
    parser.add_argument("--shuffle-seed", default=5, type=int)
    parser.add_argument("--fss-seed", default=7, type=int)
    parser.add_argument("--trace", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    result = run_case(json.loads(args.scores_json.read_text(encoding="utf-8")), args.k,
                      args.input_seed, args.algorithm_seed, args.shuffle_seed, args.fss_seed,
                      args.fss_backend, args.trace)
    data = json.dumps(result, separators=(",", ":"), sort_keys=True)
    if args.output:
        args.output.write_text(data + "\n", encoding="utf-8")
    else:
        print(data)
    return 0 if result["status"] in {"SUCCESS", "PROJECT_RANDOM_FAILURE_PATH"} else 1


if __name__ == "__main__":
    raise SystemExit(main())
