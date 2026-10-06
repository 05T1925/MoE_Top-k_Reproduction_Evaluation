#!/usr/bin/env python3
"""Independent, read-only receiver audit for BMW16 S4 TEST_ONLY evidence.

This verifier intentionally does not import the S4 implementation or either
S4/S1 oracle module. It regenerates frozen inputs and computes the stable kth
answer with Python's independent sort primitive. It also reconstructs all
four edge batches in the retained n=8 trace and re-hashes their outcomes.
"""

from __future__ import annotations

import argparse
import hashlib
import itertools
import json
import math
import random
import struct
import subprocess
from collections import Counter
from decimal import Decimal, localcontext
from pathlib import Path
from typing import Any, Iterable


LABEL = "BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R_TEST_ONLY"
ROOT_SEED = "BMW16-S4-2026-10-05-v1"
SMALL_ROOT = "BMW16-S4-small"
INT32_MIN = -(1 << 31)
INT32_MAX = (1 << 31) - 1
CONFIGS = ((128, 2), (128, 8), (256, 2), (256, 8), (1000, 80))
REPEATS = 1000


def sha_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha_file(path: Path) -> str:
    return sha_bytes(path.read_bytes())


def json_bytes(value: Any, *, sort_keys: bool = False) -> bytes:
    return json.dumps(value, sort_keys=sort_keys, separators=(",", ":")).encode("utf-8")


def seed64(text: str) -> int:
    return int.from_bytes(hashlib.sha256(text.encode("ascii")).digest()[:8], "big")


def scores_for(n: int, seed: int) -> list[int]:
    rng = random.Random(seed)
    return [rng.randrange(INT32_MIN, INT32_MAX + 1) for _ in range(n)]


def kth_index(scores: list[int], k: int) -> int:
    if not 1 <= k <= len(scores):
        raise AssertionError("invalid K in frozen record")
    return sorted(range(len(scores)), key=lambda i: (-scores[i], i))[k - 1]


def topk_mask(scores: list[int], k: int) -> list[int]:
    order = sorted(range(len(scores)), key=lambda i: (-scores[i], i))
    selected = set(order[:k])
    return [int(i in selected) for i in range(len(scores))]


def input_digest(scores: list[int]) -> str:
    return sha_bytes(json_bytes(scores))


def ceil_sqrt(n: int) -> int:
    r = math.isqrt(n)
    return r if r * r == n else r + 1


def ceil_fourth_root(n: int) -> int:
    lo, hi = 0, 1
    while hi**4 < n:
        hi *= 2
    while lo + 1 < hi:
        mid = (lo + hi) // 2
        if mid**4 >= n:
            hi = mid
        else:
            lo = mid
    return hi


def select_parameters(m: int) -> dict[str, int]:
    h = m // 2
    s = min(m, ceil_sqrt(64 * m))
    if s == m:
        ql, qh = h, h + 1
    else:
        a = ceil_sqrt(16 * s)
        ql = max(1, (s + 1) // 2 - a)
        qh = min(s, (s + 2) // 2 + a)
    cap = min(m, ceil_fourth_root(4096 * m**3))
    l = ceil_sqrt(4 * m)
    return {"M": m, "h": h, "s": s, "qL": ql, "qH": qh,
            "Ucap": cap, "L": l, "Wcap": 2 * l + 1}


def exact_bracket_failure(m: int, p: dict[str, int]) -> Decimal:
    h, s, ql, qh = p["h"], p["s"], p["qL"], p["qH"]
    denominator = math.comb(m, s)
    numerator = 0
    lower = max(0, s - h)
    upper = min(h, s)
    for j in range(lower, upper + 1):
        if j < ql or j >= qh:
            numerator += math.comb(h, j) * math.comb(h, s - j)
    return Decimal(numerator) / Decimal(denominator)


def ceil_sqrt_ratio(numerator: int, denominator: int) -> int:
    floor = math.isqrt(numerator // denominator)
    return floor if floor * floor * denominator == numerator else floor + 1


def ceil_bound_audit() -> dict[str, Any]:
    # Check all small/intermediate M for the integerized R1/R2/R4 envelopes,
    # then probe the asymptotic U-cap inequalities immediately above the
    # 4096 transition and at larger scales using exact integer roots.
    r1_checked = 0
    r4_checked = 0
    for m in range(2, 100_001):
        p = select_parameters(m)
        if p["s"] * (p["s"] - 1) // 2 > 42 * m:
            raise AssertionError(f"R1 ceiling bound fails at M={m}")
        if p["Wcap"] * (p["Wcap"] - 1) // 2 > 25 * m:
            raise AssertionError(f"R4 ceiling bound fails at M={m}")
        r1_checked += 1
        r4_checked += 1
    cap_rows = []
    with localcontext() as ctx:
        ctx.prec = 60
        for m in (4097, 4098, 5000, 10_000, 100_000, 1_000_000, 1_000_000_000):
            p = select_parameters(m)
            c = ceil_fourth_root(4096 * m**3)
            d = p["qH"] - p["qL"]
            mu = Decimal(p["s"] * c) / Decimal(m)
            m_quarter = Decimal(m) ** Decimal("0.25")
            exponent = (mu - Decimal(d)) ** 2 / (Decimal(2) * mu) if mu > d else Decimal(0)
            u = p["Ucap"]
            v = min(u, ceil_sqrt_ratio(64 * u * u, m))
            if not (c**4 >= 4096 * m**3 and (c - 1) ** 4 < 4096 * m**3):
                raise AssertionError(f"U-cap fourth-root ceiling mismatch at M={m}")
            if not (d <= 24 * m_quarter and mu >= 64 * m_quarter and (u == m or c == u)):
                raise AssertionError(f"U-cap Chernoff preconditions fail at M={m}")
            if u < m and exponent < Decimal(12) * m_quarter:
                raise AssertionError(f"U-cap Chernoff exponent constant fails at M={m}")
            if v * u > 657 * m:
                raise AssertionError(f"large-M R3 vU bound fails at M={m}")
            cap_bound = Decimal(0) if u == m else Decimal(m) * (-(Decimal(12) * m_quarter)).exp()
            cap_rows.append({"M": m, "s": p["s"], "qH_minus_qL": d, "Ucap": u,
                             "mu": format(mu, ".8E"), "chernoff_exponent_lower": format(exponent, ".8E"),
                             "cap_abort_upper_bound": "0" if cap_bound == 0 else format(cap_bound, ".8E"),
                             "vU_at_Ucap": v * u, "657M": 657 * m})
    return {"finite_ceil_checks": {"M_range": "2..100000", "R1_Cs2_le_42M_checked": r1_checked,
                                    "R4_CW2_le_25M_checked": r4_checked},
            "asymptotic_Ucap_boundary_probes": cap_rows,
            "integer_root_checks": True}


def probability_cost_audit() -> dict[str, Any]:
    official_ms = (254, 242, 510, 498, 1842)
    table = []
    with localcontext() as ctx:
        ctx.prec = 70
        win = Decimal(2) * Decimal(-16).exp()
        for m in official_ms:
            p = select_parameters(m)
            bracket = exact_bracket_failure(m, p)
            cap = Decimal(0) if p["Ucap"] == m else Decimal(m) * (-(Decimal(12) * (Decimal(m) ** Decimal("0.25")))).exp()
            delta = bracket + cap + win
            select_fail = Decimal(2) * delta - delta * delta
            # Per-partition comparison upper bounds from S4's integerized schedule.
            r1 = 42 * m
            r2 = 2 * m
            r3 = 2048 * m if m <= 4096 else 657 * m
            r4 = 25 * m
        fmt = lambda value: "0" if value == 0 else format(value, ".8E")
        table.append({
                **p,
                "delta_R1_hypergeom_exact": fmt(bracket),
                "delta_U_cap_union_bound": fmt(cap),
                "delta_R3_window": fmt(win),
                "one_partition_abort_upper_bound": fmt(delta),
                "two_instance_select_abort_upper_bound": fmt(select_fail),
                "one_partition_round_call_bounds": {"R1": r1, "R2": r2, "R3": r3, "R4": r4},
                "two_instance_total_bound": 4234 * m,
                "appendix_A_linear_bound_for_n": 8468,
            })
    return {"epsilon_note": "S4 finite sample-bracket algorithm has no epsilon; BMW16 Theorem 8 only covers 0<epsilon<1/18 and is not inherited.",
            "probability_model": "ideal independent uniform without-replacement samples; seedable Python PRNG runs are observations only",
            "official_M_table": table,
            "proof_conditions_rechecked": [
                "R1 bracket iff qL <= H < qH for H~Hypergeom(M,M/2,s)",
                "U-cap event U>ceil(8 M^(3/4)) implies some length-C interval has at most qH-qL sampled points; union over at most M starts and lower-tail hypergeometric Chernoff bound",
                "for M>4096, qH-qL<=24 M^(1/4), mu>=64 M^(1/4), yielding M exp(-12 M^(1/4)); if cap=M, cap-abort is impossible",
                "R3 misses either needed L=ceil(2 sqrt(M)) neighborhood with conditional probability at most 2 exp(-16); if V=U, miss probability is zero",
                "two independent ideal random tapes give success at least (1-delta)^2; a successful bracket plus exact R4 ranking yields exact partition cardinality"
            ]}


def checked_add(a: int, b: int) -> int:
    value = a + b
    if value > (1 << 64) - 1:
        raise OverflowError("uint64 addition")
    return value


def checked_mul(a: int, b: int) -> int:
    value = a * b
    if value > (1 << 64) - 1:
        raise OverflowError("uint64 multiplication")
    return value


def capacity_row(n: int, k: int, case: str) -> dict[str, Any]:
    m = checked_mul(2, max(n - k + 1, k))
    p = select_parameters(m)
    m, s = p["M"], p["s"]
    c2s = checked_mul(s, s - 1) // 2
    r2 = checked_mul(m, s) - checked_mul(s, s + 1) // 2
    c2m = checked_mul(m, m - 1) // 2
    r1_total = checked_mul(2, c2s)
    r2_total = checked_mul(2, r2)
    r3_total = checked_mul(2, c2m)
    r4_total = checked_mul(2, c2m)
    final_membership = checked_mul(n, n - 1) // 2
    slots = checked_add(checked_add(r1_total, r2_total),
                        checked_add(checked_add(r3_total, r4_total), final_membership))
    conservative_slots = checked_add(checked_mul(8, c2m), final_membership)
    index_bits = max(1, (n - 1).bit_length())
    # Five disjoint item classes need three category bits. One extra ring bit
    # follows the current ProtocolIUcmp integration's candidate range rule.
    comparison_bits = 32 + index_bits + 3 + 1
    if comparison_bits < 34 or comparison_bits > 53:
        bytes_per_party_slot = None
        party_bytes = None
        two_party_bytes = None
        conservative_party_bytes = None
        conservative_two_party_bytes = None
    else:
        # Lower bound: 3 uint64 edge fields + two per-edge mask-share uint64s
        # + ProtocolIUcmpPartyMaterial::serialize() (57 + 24*Bin bytes).
        bytes_per_party_slot = checked_add(97, checked_mul(24, comparison_bits))
        party_bytes = checked_mul(slots, bytes_per_party_slot)
        two_party_bytes = checked_mul(2, party_bytes)
        conservative_party_bytes = checked_mul(conservative_slots, bytes_per_party_slot)
        conservative_two_party_bytes = checked_mul(2, conservative_party_bytes)
    return {"case": case, "n": n, "K": k, "M_per_partition": m, "R1_slots_both_instances": r1_total,
            "R2_slots_both_instances": r2_total, "R3_slots_both_instances": r3_total,
            "R4_slots_both_instances": r4_total, "final_membership_pair_slots": final_membership,
            "total_one_shot_logical_material_slots": slots,
            "conservative_full_R1_R2_pair_pool_slots": conservative_slots,
            "under_uint64_checked_arithmetic": True,
            "stable_composite_key_bits": 32 + index_bits,
            "category_tag_bits_for_five_classes": 3,
            "candidate_uCMP_Bin_including_ring_slack": comparison_bits,
            "current_uCMP_supported_range": "34..53",
            "current_uCMP_supports_candidate_width": comparison_bits <= 53,
            "per_party_bytes_per_slot_lower_bound": bytes_per_party_slot,
            "per_party_payload_bytes_lower_bound": party_bytes,
            "two_party_payload_bytes_lower_bound": two_party_bytes,
            "conservative_per_party_payload_bytes_lower_bound": conservative_party_bytes,
            "conservative_two_party_payload_bytes_lower_bound": conservative_two_party_bytes,
            "bytes_excluded": ["outer authenticated-encryption/file framing", "phase/session/one-time-id fields beyond 3 uint64 edge fields", "T key-generation transient peak", "transport buffers", "audit metadata"]}


def capacity_audit() -> dict[str, Any]:
    rows = [capacity_row(n, k, f"official_n{n}_k{k}") for n, k in CONFIGS]
    rows.extend(capacity_row(n, 1, f"scale_n{n}_worst_rank_K1_M2n") for n in (10_000, 100_000, 1_000_000))
    return {"mode": "checked uint64 arithmetic only; no pool allocation", "pool_model": {
                "R1": "optimistic: C(s,2) per median instance only if an input-independent sample slot set is fixed offline and covered by a hidden-uniform-shuffle proof",
                "R2": "optimistic: all candidate pivots in S to all M handles: M*s-C(s+1,2) unordered pair slots per instance; if S/pivot handles are not prebound, conservative pool gives C(M,2)",
                "R3_R4": "all possible adaptive U/W pairs, conservatively C(M,2) per stage and instance",
                "membership": "all real-real pairs C(n,2), enough to serve a hidden selected-handle star without revealing selected identity",
                "separate_stage_ids": True,
                "fresh_one_shot_material_for_each_stage_pair": True,
                "conservative_total_slots": "8*C(M,2)+C(n,2), reserving complete pair domains for all four layers in each of two instances"},
            "serialization_basis": "Current ProtocolIUcmpPartyMaterial serialize length=57+24*Bin bytes; edge envelope 24 bytes; independent endpoint masks add 16 bytes per party. The estimate is a lower bound, not a promised E20 wire format.",
            "rows": rows}


def expected_schedule() -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for n, k in CONFIGS:
        config = f"n{n}_k{k}"
        fixed = seed64(f"{ROOT_SEED}|fixed-input|{config}")
        for rep in range(REPEATS):
            rows.append({"family": "fixed_input_coin_only", "config_id": config,
                         "n": n, "K": k, "repetition": rep, "input_seed": fixed,
                         "algorithm_seed": seed64(f"{ROOT_SEED}|fixed-algorithm|{config}|{rep}")})
        for rep in range(REPEATS):
            rows.append({"family": "random_int32_input_and_independent_coins", "config_id": config,
                         "n": n, "K": k, "repetition": rep,
                         "input_seed": seed64(f"{ROOT_SEED}|random-input|{config}|{rep}"),
                         "algorithm_seed": seed64(f"{ROOT_SEED}|random-algorithm|{config}|{rep}")})
    return rows


def check_official(evidence: Path) -> dict[str, Any]:
    # This archive's manifest binds select4r.py to the exact S4 receiver commit.
    base = evidence / "official_matrix_final_source"
    schedule_path = base / "seed_schedule.json"
    manifest_path = base / "frozen_manifest.json"
    results_path = base / "algorithm_results.jsonl"
    failures_path = base / "algorithm_failures.jsonl"
    summary_path = base / "algorithm_run_summary.json"
    schedule_obj = json.loads(schedule_path.read_text(encoding="utf-8"))
    if schedule_obj.get("label") != LABEL or schedule_obj.get("root_seed") != ROOT_SEED:
        raise AssertionError("official schedule label/root seed mismatch")
    schedule = expected_schedule()
    if schedule_obj.get("rows") != schedule:
        raise AssertionError("frozen schedule differs from independently regenerated schedule")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    summary = json.loads(summary_path.read_text(encoding="utf-8"))
    if manifest.get("schedule_sha256") != sha_file(schedule_path):
        raise AssertionError("manifest schedule hash mismatch")
    if summary.get("schedule_sha256") != sha_file(schedule_path):
        raise AssertionError("run summary schedule hash mismatch")
    if summary.get("frozen_manifest_sha256") != sha_file(manifest_path):
        raise AssertionError("run summary manifest hash mismatch")
    if summary.get("algorithm_results_sha256") != sha_file(results_path):
        raise AssertionError("run summary results hash mismatch")
    if summary.get("algorithm_failures_sha256") != sha_file(failures_path):
        raise AssertionError("run summary failures hash mismatch")
    by_key = {(r["family"], r["config_id"], r["repetition"]): r for r in schedule}
    result_count = 0
    status_counts: Counter[str] = Counter()
    per_group: Counter[tuple[str, str]] = Counter()
    total_comparisons = 0
    max_ratio = 0.0
    for line_no, line in enumerate(results_path.open(encoding="utf-8"), 1):
        row = json.loads(line)
        key = (row.get("family"), row.get("config_id"), row.get("repetition"))
        job = by_key.get(key)
        if job is None:
            raise AssertionError(f"unexpected official row {line_no}: {key}")
        if any(row.get(field) != job[field] for field in ("n", "K", "input_seed", "algorithm_seed")):
            raise AssertionError(f"seed/config mismatch at official row {line_no}")
        scores = scores_for(job["n"], job["input_seed"])
        expected = kth_index(scores, job["K"])
        if row.get("input_sha256") != input_digest(scores):
            raise AssertionError(f"input digest mismatch at official row {line_no}")
        if row.get("label") != LABEL or row.get("status") != "SUCCESS":
            raise AssertionError(f"non-success or wrong label at official row {line_no}")
        if row.get("candidate_original_indices") != [expected] or row.get("selected_original_index") != expected:
            raise AssertionError(f"independent stable kth mismatch at official row {line_no}")
        metrics = row["round_metrics"]
        if set(metrics) != {"1", "2", "3", "4"}:
            raise AssertionError(f"not exactly four metric layers at official row {line_no}")
        calls = sum(int(metrics[str(r)]["comparison_calls"]) for r in range(1, 5))
        if calls != row.get("total_comparison_calls"):
            raise AssertionError(f"round counts do not sum at official row {line_no}")
        if any(int(metrics[str(r)]["comparison_calls"]) < 0 for r in range(1, 5)):
            raise AssertionError(f"negative edge count at official row {line_no}")
        for r in range(1, 5):
            mr = metrics[str(r)]
            round_calls = int(mr["comparison_calls"])
            if int(mr["real_real_calls"]) + int(mr["dummy_related_calls"]) != round_calls:
                raise AssertionError(f"real/dummy edge classes do not sum at official row {line_no}, R{r}")
            if int(mr["same_round_duplicate_calls"]) != round_calls - int(mr["unique_unordered_edges_this_round"]):
                raise AssertionError(f"round duplicate edge count mismatch at official row {line_no}, R{r}")
            if int(mr["same_task_repeat_calls_same_round"]) + int(mr["cross_task_repeat_calls_same_round"]) != int(mr["same_round_duplicate_calls"]):
                raise AssertionError(f"same/cross task repeat count mismatch at official row {line_no}, R{r}")
        if calls > 8468 * job["n"]:
            raise AssertionError(f"observed count exceeds stated linear envelope at row {line_no}")
        if row.get("effective_median_size_M") != 2 * max(job["n"] - job["K"] + 1, job["K"]):
            raise AssertionError(f"Appendix-A expansion mismatch at official row {line_no}")
        if calls - int(row["unique_unordered_edges_all_rounds"]) != int(row["repeated_total_calls"]):
            raise AssertionError(f"cross-round repeat count mismatch at official row {line_no}")
        result_count += 1
        total_comparisons += calls
        status_counts[row["status"]] += 1
        per_group[(job["family"], job["config_id"])] += 1
        max_ratio = max(max_ratio, calls / job["n"])
    if result_count != 10000 or len(by_key) != 10000:
        raise AssertionError(f"official row count mismatch: {result_count}")
    if status_counts != Counter({"SUCCESS": 10000}):
        raise AssertionError(f"unexpected status counts {status_counts}")
    if failures_path.stat().st_size != 0:
        raise AssertionError("official failure JSONL is not empty")
    if any(v != 1000 for v in per_group.values()) or len(per_group) != 10:
        raise AssertionError("official family/config denominators mismatch")
    lower_1000 = math.pow(0.05, 1.0 / 1000.0)
    return {
        "rows": result_count,
        "status_counts": dict(status_counts),
        "groups": {f"{family}|{config}": n for (family, config), n in sorted(per_group.items())},
        "failures_jsonl_bytes": failures_path.stat().st_size,
        "max_observed_comparisons_per_n": max_ratio,
        "total_comparison_calls": total_comparisons,
        "zero_failure_one_sided_95pct_lower_at_each_n1000_group": lower_1000,
        "hashes": {p.name: sha_file(p) for p in (schedule_path, manifest_path, results_path, failures_path, summary_path)},
    }


def check_fresh_replay(evidence: Path, replay_root: Path) -> dict[str, Any]:
    """Bind a fresh exact-S4-source rerun to the retained authoritative run.

    The result rows embed the hash of their run manifest, so byte-for-byte
    result-file equality is not expected when the replay manifest records the
    S4 commit rather than the earlier S3 base. Compare every row after removing
    only that provenance field; all inputs, outcomes, statuses, and counters
    must remain identical.
    """
    old = evidence / "official_matrix_final_source"
    replay = replay_root.resolve()
    old_summary = json.loads((old / "algorithm_run_summary.json").read_text(encoding="utf-8"))
    new_summary = json.loads((replay / "algorithm_run_summary.json").read_text(encoding="utf-8"))
    new_manifest_path = replay / "frozen_manifest.json"
    new_manifest = json.loads(new_manifest_path.read_text(encoding="utf-8"))
    schedule_path = replay / "seed_schedule.json"
    results_path = replay / "algorithm_results.jsonl"
    failures_path = replay / "algorithm_failures.jsonl"
    if new_summary.get("revision") != "04ed6f8a352ab2277434344094471dff82f83e4b":
        raise AssertionError("fresh replay is not bound to the exact S4 receiver commit")
    if new_summary.get("label") != LABEL or new_summary.get("schedule_rows") != 10000:
        raise AssertionError("fresh replay label or row count mismatch")
    if new_summary.get("schedule_sha256") != sha_file(schedule_path):
        raise AssertionError("fresh replay schedule hash mismatch")
    if new_summary.get("frozen_manifest_sha256") != sha_file(new_manifest_path):
        raise AssertionError("fresh replay manifest hash mismatch")
    if new_summary.get("algorithm_results_sha256") != sha_file(results_path):
        raise AssertionError("fresh replay result hash mismatch")
    if new_summary.get("algorithm_failures_sha256") != sha_file(failures_path):
        raise AssertionError("fresh replay failure hash mismatch")
    if new_manifest.get("revision") != new_summary.get("revision"):
        raise AssertionError("fresh replay manifest and summary revisions disagree")
    if new_manifest.get("source_sha256") != json.loads((old / "frozen_manifest.json").read_text(encoding="utf-8")).get("source_sha256"):
        raise AssertionError("fresh replay source hashes differ from the authoritative run")
    if sha_file(schedule_path) != sha_file(old / "seed_schedule.json"):
        raise AssertionError("fresh replay seed schedule differs from the authoritative run")
    if failures_path.stat().st_size != 0:
        raise AssertionError("fresh replay failure file is not empty")
    compared = 0
    with (old / "algorithm_results.jsonl").open(encoding="utf-8") as old_f, results_path.open(encoding="utf-8") as new_f:
        for line_no, pair in enumerate(zip(old_f, new_f), 1):
            old_line, new_line = pair
            old_row, new_row = json.loads(old_line), json.loads(new_line)
            old_provenance = old_row.pop("frozen_manifest_sha256", None)
            new_provenance = new_row.pop("frozen_manifest_sha256", None)
            if old_provenance != old_summary.get("frozen_manifest_sha256"):
                raise AssertionError(f"authoritative row manifest binding mismatch at row {line_no}")
            if new_provenance != new_summary.get("frozen_manifest_sha256"):
                raise AssertionError(f"replay row manifest binding mismatch at row {line_no}")
            if old_row != new_row:
                raise AssertionError(f"fresh replay differs from authoritative outcomes/counters at row {line_no}")
            compared += 1
        if next(old_f, None) is not None or next(new_f, None) is not None:
            raise AssertionError("fresh replay result row count differs")
    if compared != 10000:
        raise AssertionError(f"fresh replay compared unexpected row count: {compared}")
    return {
        "rows_compared": compared,
        "all_outcomes_inputs_statuses_and_counters_match": True,
        "only_row_field_removed_for_comparison": "frozen_manifest_sha256",
        "authoritative_manifest_revision": old_summary.get("revision"),
        "fresh_replay_revision": new_summary.get("revision"),
        "authoritative_results_sha256": sha_file(old / "algorithm_results.jsonl"),
        "fresh_replay_results_sha256": sha_file(results_path),
        "fresh_replay_schedule_sha256": sha_file(schedule_path),
        "fresh_replay_manifest_sha256": sha_file(new_manifest_path),
        "fresh_replay_failures_sha256": sha_file(failures_path),
    }


def check_cpp_oracle(evidence: Path) -> dict[str, Any]:
    base = evidence / "oracle_crosscheck"
    report_path = base / "python_cpp_oracle_crosscheck.json"
    exe_path = base / "oracle_harness.exe"
    report = json.loads(report_path.read_text(encoding="utf-8"))
    if report.get("status") != "PASS" or report.get("cases") != 13:
        raise AssertionError("stored Python/C++ crosscheck summary is not the frozen 13-case PASS")
    if report.get("cpp_binary_sha256") != sha_file(exe_path):
        raise AssertionError("C++ oracle executable hash differs from crosscheck record")
    cases: list[tuple[str, list[int], int]] = [
        ("boundary_two", [INT32_MIN, INT32_MAX], 1),
        ("boundary_two", [INT32_MIN, INT32_MAX], 2),
        ("all_equal", [0, 0, 0, 0, 0], 1),
        ("all_equal", [0, 0, 0, 0, 0], 3),
        ("all_equal", [0, 0, 0, 0, 0], 5),
        ("mixed_signed_ties", [INT32_MIN, 7, 7, INT32_MAX, -1, 7, INT32_MIN], 1),
        ("mixed_signed_ties", [INT32_MIN, 7, 7, INT32_MAX, -1, 7, INT32_MIN], 4),
        ("mixed_signed_ties", [INT32_MIN, 7, 7, INT32_MAX, -1, 7, INT32_MIN], 7),
    ]
    for n, k in CONFIGS:
        seed = seed64(f"{ROOT_SEED}|fixed-input|n{n}_k{k}")
        cases.append((f"official_fixed_n{n}_k{k}", scores_for(n, seed), k))
    parts = [str(len(cases))]
    for _name, scores, k in cases:
        parts.extend((f"{len(scores)} {k}", " ".join(str(x) for x in scores)))
    completed = subprocess.run([str(exe_path)], input="\n".join(parts) + "\n",
                               text=True, capture_output=True, check=True)
    output = completed.stdout.splitlines()
    if len(output) != len(cases):
        raise AssertionError("compiled repository oracle returned the wrong number of rows")
    stored_rows = report["results"]
    if len(stored_rows) != len(cases):
        raise AssertionError("crosscheck JSON case count mismatch")
    for i, ((name, scores, k), line, stored) in enumerate(zip(cases, output, stored_rows)):
        parts_out = line.split()
        if len(parts_out) != 2:
            raise AssertionError(f"malformed C++ oracle output at case {i}")
        selected = int(parts_out[0])
        mask = [int(bit) for bit in parts_out[1]]
        expected = kth_index(scores, k)
        expected_mask = topk_mask(scores, k)
        if (selected != expected or mask != expected_mask or sum(mask) != k or
                stored.get("case") != name or stored.get("n") != len(scores) or
                stored.get("K") != k or stored.get("cpp_selected") != expected or
                stored.get("python_selected") != expected or stored.get("mask_count") != k or
                stored.get("input_sha256") != input_digest(scores) or stored.get("status") != "AGREE"):
            raise AssertionError(f"repository C++ oracle, recorded row, and independent sort disagree at case {i}")
    return {"cases": len(cases), "all_masks_exactly_k": True,
            "all_selected_indices_match_independent_sort": True,
            "binary_sha256": sha_file(exe_path), "report_sha256": sha_file(report_path)}


def check_s4_sources(source_root: Path, evidence: Path) -> dict[str, Any]:
    manifest = json.loads((evidence / "official_matrix_final_source" / "frozen_manifest.json").read_text(encoding="utf-8"))
    files = ("select4r.py", "run_matrix.py", "validate_results.py", "audit_edges.py", "test_select4r.py")
    observed = {name: sha_file(source_root / name) for name in files}
    for name, digest in observed.items():
        if manifest.get("source_sha256", {}).get(name) != digest:
            raise AssertionError(f"S4 official manifest source hash mismatch: {name}")
    return {"source_sha256": observed, "all_official_manifest_hashes_match": True}


def small_cases() -> Iterable[tuple[str, list[int], int, int, int]]:
    for n in range(1, 8):
        for permutation_id, perm in enumerate(itertools.permutations(range(n))):
            values = list(perm)
            for k in range(1, n + 1):
                label = f"exhaustive_n{n}_perm{permutation_id}_k{k}"
                yield label, values, k, seed64(f"{SMALL_ROOT}|{label}"), seed64(f"{SMALL_ROOT}|{label}|input")
    adversarial = {
        "all_equal_5": [0] * 5,
        "all_equal_17": [INT32_MIN] * 17,
        "duplicate_ties": [7, -2, 7, 0, -2, 7, INT32_MAX, INT32_MIN, 0],
        "signed_edges": [INT32_MIN, INT32_MAX, -1, 0, 1, INT32_MIN, INT32_MAX],
        "odd_non_power": [9, -9, 9, 4, 4, INT32_MIN, INT32_MAX, 0, 0, -1, 13],
        "even_non_power": [5, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0],
    }
    for name, values in adversarial.items():
        for k in sorted({1, (len(values) + 1) // 2, len(values), max(1, len(values) - 1)}):
            label = f"adversarial_{name}_k{k}"
            yield label, values, k, seed64(f"{SMALL_ROOT}|{label}"), seed64(f"{SMALL_ROOT}|{label}|input")
    large = {
        "n128_all_equal": [7] * 128,
        "n256_repeated_and_edges": [(INT32_MIN, -1, 0, 0, 1, INT32_MAX)[i % 6] for i in range(256)],
        "n1000_all_equal": [INT32_MAX] * 1000,
        "n1000_alternating_edges": [INT32_MIN if i % 2 else INT32_MAX for i in range(1000)],
    }
    for name, values in large.items():
        n = len(values)
        for k in sorted({1, 2, 8, (n + 1) // 2, n - 1, n}):
            label = f"large_adversarial_{name}_k{k}"
            yield label, values, k, seed64(f"{SMALL_ROOT}|{label}"), seed64(f"{SMALL_ROOT}|{label}|input")
    for pattern, values in (("three_value_ties", [(-1, 0, 1)[i % 3] for i in range(65)]),
                            ("alternating_extremes", [INT32_MIN if i % 2 else INT32_MAX for i in range(67)])):
        for repetition in range(100):
            label = f"coin_only_{pattern}_rep{repetition}"
            yield label, values, 8, seed64(f"{SMALL_ROOT}|{label}"), seed64(f"{SMALL_ROOT}|{label}|input")


def check_small(evidence: Path) -> dict[str, Any]:
    base = evidence / "small_and_adversarial_final_verify"
    path = base / "small_and_adversarial_cases.jsonl"
    failure_path = base / "small_and_adversarial_failures.jsonl"
    summary_path = base / "small_and_adversarial_summary.json"
    summary = json.loads(summary_path.read_text(encoding="utf-8"))
    generated = iter(small_cases())
    count = 0
    statuses: Counter[str] = Counter()
    max_ratio = 0.0
    total_calls = 0
    for line_no, line in enumerate(path.open(encoding="utf-8"), 1):
        row = json.loads(line)
        try:
            label, scores, k, algo_seed, in_seed = next(generated)
        except StopIteration as exc:
            raise AssertionError("small result has extra rows") from exc
        expected = kth_index(scores, k)
        if row.get("case") != label or row.get("n") != len(scores) or row.get("K") != k:
            raise AssertionError(f"small case identity mismatch at row {line_no}")
        if row.get("input_seed") != in_seed or row.get("algorithm_seed") != algo_seed:
            raise AssertionError(f"small seed mismatch at row {line_no}")
        if row.get("input_sha256") != input_digest(scores):
            raise AssertionError(f"small input hash mismatch at row {line_no}")
        if row.get("oracle_original_index") != expected:
            raise AssertionError(f"S4 stored oracle field differs from independent sort at row {line_no}")
        status = row.get("algorithm_status")
        candidate = row.get("candidate_original_index")
        derived = "SUCCESS" if status == "SUCCESS" and candidate == expected else (
            "COMPLETED_WRONG" if status == "SUCCESS" else status)
        if row.get("validation_status") != derived:
            raise AssertionError(f"validation label mismatch at small row {line_no}")
        metrics = row["round_metrics"]
        calls = sum(int(metrics[str(r)]["comparison_calls"]) for r in range(1, 5))
        if calls != row.get("total_comparison_calls"):
            raise AssertionError(f"small round cost sum mismatch at row {line_no}")
        if any(metrics[str(r)].get("comparison_calls") is None for r in range(1, 5)):
            raise AssertionError(f"small case missing one of four rounds at row {line_no}")
        for r in range(1, 5):
            mr = metrics[str(r)]
            if int(mr["real_real_calls"]) + int(mr["dummy_related_calls"]) != int(mr["comparison_calls"]):
                raise AssertionError(f"small real/dummy edge classes do not sum at row {line_no}, R{r}")
            if int(mr["same_round_duplicate_calls"]) != int(mr["comparison_calls"]) - int(mr["unique_unordered_edges_this_round"]):
                raise AssertionError(f"small duplicate edge count mismatch at row {line_no}, R{r}")
        statuses[derived] += 1
        total_calls += calls
        max_ratio = max(max_ratio, calls / len(scores))
        count += 1
    try:
        extra = next(generated)
        raise AssertionError(f"small result omits generated case {extra[0]}")
    except StopIteration:
        pass
    if count != 40567:
        raise AssertionError(f"small case count mismatch {count}")
    if summary.get("total_cases") != count or summary.get("results_sha256") != sha_file(path):
        raise AssertionError("small summary does not bind case file")
    if summary.get("failures_sha256") != sha_file(failure_path) or failure_path.stat().st_size != 0:
        raise AssertionError("small failure file/hash mismatch")
    if statuses != Counter({"SUCCESS": 40567}):
        raise AssertionError(f"unexpected small statuses {statuses}")
    return {"rows": count, "status_counts": dict(statuses), "failure_bytes": failure_path.stat().st_size,
            "max_observed_comparisons_per_n": max_ratio, "total_comparison_calls": total_calls,
            "case_sha256": sha_file(path), "summary_sha256": sha_file(summary_path)}


def strict_less(a: dict[str, Any], b: dict[str, Any]) -> bool:
    def key(x: dict[str, Any]) -> tuple[int, int, int]:
        category = x["category"]
        if category == 2:
            return (category, int(x["score"]), -int(x["original_index"]))
        return (category, int(x["serial"]), 0)
    return key(a) < key(b)


def trace_expected_edges(task: dict[str, Any], round_no: int) -> list[tuple[str, int, int]]:
    name = task["task"]
    if round_no == 1:
        ids = task["sample_ids"]
        pairs = itertools.combinations(ids, 2)
    elif round_no == 2:
        ids = task["input_ids"]
        x, y = task["pivot_low"], task["pivot_high"]
        pairs = itertools.chain(((i, x) for i in ids if i != x), ((i, y) for i in ids if i != y))
    elif round_no == 3:
        u, v = task["U_ids"], task["V_ids"]
        vset = set(v)
        pairs = itertools.chain(itertools.combinations(v, 2), ((i, j) for i in v for j in u if j not in vset))
    elif round_no == 4:
        pairs = itertools.combinations(task["W_ids"], 2)
    else:
        raise AssertionError("unexpected round")
    return [(name, int(a), int(b)) for a, b in pairs]


def check_trace(evidence: Path) -> dict[str, Any]:
    path = evidence / "small_trace_final.json"
    trace = json.loads(path.read_text(encoding="utf-8"))
    items = trace["trace"]["items"]
    events = trace["trace"]["events"]
    plan_by_round: dict[int, dict[str, Any]] = {}
    results_by_round: dict[int, dict[str, Any]] = {}
    outcomes_by_round: dict[int, list[bool]] = {}
    edge_count_total = 0
    duplicate_count_total = 0
    unique_prior_round_edges: set[tuple[int, int]] = set()
    round_accounting: dict[str, dict[str, int]] = {}
    for event in events:
        if event.get("event") == "ROUND_PLAN_FROZEN":
            r = int(event["round"])
            plans = [trace_expected_edges(t, r) for t in event["tasks"]]
            expected = list(itertools.chain.from_iterable(plans))
            actual = [(str(t), int(a), int(b)) for t, a, b in event["edges"]]
            if actual != expected:
                raise AssertionError(f"round {r}: edge list differs from independently rebuilt batch")
            if event.get("requires_results_through_round") != r - 1:
                raise AssertionError(f"round {r}: incorrect result barrier")
            if event.get("edge_count") != len(actual):
                raise AssertionError(f"round {r}: edge count mismatch")
            for task in event["tasks"]:
                if task.get("requires_results_through_round") != r - 1:
                    raise AssertionError(f"round {r}: task dependency metadata mismatch")
            canonical = b"".join(struct.pack("<II", min(a, b), max(a, b)) for _, a, b in actual)
            if event.get("edge_sha256") != sha_bytes(canonical):
                raise AssertionError(f"round {r}: edge digest mismatch")
            outcomes = [strict_less(items[a], items[b]) for _, a, b in actual]
            out_digest = sha_bytes(bytes(outcomes))
            result = next((e for e in events if e.get("event") == "ROUND_RESULTS" and e.get("round") == r), None)
            if result is None or result.get("outcome_sha256") != out_digest or result.get("outcome_count") != len(actual):
                raise AssertionError(f"round {r}: recomputed comparison results disagree with trace digest")
            plan_by_round[r] = event
            results_by_round[r] = result
            outcomes_by_round[r] = outcomes
            edge_count_total += len(actual)
            pair_keys = [(min(a, b), max(a, b)) for _, a, b in actual]
            unique_pairs = set(pair_keys)
            duplicates_this_round = len(actual) - len(unique_pairs)
            duplicate_count_total += duplicates_this_round
            task_pair_keys = [(task_name, min(a, b), max(a, b)) for task_name, a, b in actual]
            seen_task_pairs: set[tuple[str, int, int]] = set()
            same_task_repeats = 0
            for key in task_pair_keys:
                if key in seen_task_pairs:
                    same_task_repeats += 1
                seen_task_pairs.add(key)
            owner_sets: dict[str, set[tuple[int, int]]] = {}
            for task_name, a, b in actual:
                owner_sets.setdefault(task_name, set()).add((min(a, b), max(a, b)))
            prior_unique_owners: set[tuple[int, int]] = set()
            cross_task_repeats = 0
            for pair_set in owner_sets.values():
                cross_task_repeats += len(pair_set & prior_unique_owners)
                prior_unique_owners.update(pair_set)
            round_accounting[str(r)] = {
                "comparison_calls": len(actual),
                "real_real_calls": sum(items[a]["category"] == 2 and items[b]["category"] == 2 for _, a, b in actual),
                "dummy_related_calls": sum(not (items[a]["category"] == 2 and items[b]["category"] == 2) for _, a, b in actual),
                "same_round_duplicate_calls": duplicates_this_round,
                "same_task_repeat_calls": same_task_repeats,
                "cross_task_repeat_calls": cross_task_repeats,
                "prior_round_repeat_calls": sum(pair in unique_prior_round_edges for pair in pair_keys),
                "unique_unordered_edges_this_round": len(unique_pairs),
            }
            unique_prior_round_edges.update(unique_pairs)
    if set(plan_by_round) != {1, 2, 3, 4}:
        raise AssertionError("trace does not contain exactly four frozen comparison batches")
    # Validate R1 pivots using only the completed R1 schedule/results and the strict order.
    r1_tasks = {t["task"]: t for t in plan_by_round[1]["tasks"]}
    r1_derived = next(e for e in events if e.get("event") == "R1_DERIVED_STATE")
    by_task = {p["task"]: p for p in r1_derived["partitions"]}
    for name, task in r1_tasks.items():
        sample_order = sorted(task["sample_ids"], key=lambda i: _item_key(items[i]))
        low, high = sample_order[int(task["qL"]) - 1], sample_order[int(task["qH"]) - 1]
        if by_task[name]["pivot_low"] != low or by_task[name]["pivot_high"] != high:
            raise AssertionError(f"{name}: R1-derived pivots mismatch")
    # Reconstruct R2 contiguous intervals and residual ranks.
    r2_derived = {e["task"]: e for e in events if e.get("event") == "R2_DERIVED_STATE"}
    for task in plan_by_round[2]["tasks"]:
        name = task["task"]
        ids = task["input_ids"]
        ordered = sorted(ids, key=lambda i: _item_key(items[i]))
        x, y = task["pivot_low"], task["pivot_high"]
        ix, iy = ordered.index(x), ordered.index(y)
        if ix >= iy:
            raise AssertionError(f"{name}: R2 pivots cross")
        state = r2_derived[name]
        expected_u_set = set(ordered[ix:iy + 1])
        expected_prefix_set = set(ordered[:ix])
        expected_suffix_set = set(ordered[iy + 1:])
        expected_u = [item for item in ids if item in expected_u_set]
        expected_prefix = [item for item in ids if item in expected_prefix_set]
        expected_suffix = [item for item in ids if item in expected_suffix_set]
        h = trace["median_reject_count_h"]
        q = h - len(expected_prefix)
        if state["U_ids"] != expected_u or state["prefix_ids"] != expected_prefix or state["suffix_ids"] != expected_suffix or state["q"] != q:
            raise AssertionError(f"{name}: R2 interval or residual rank mismatch")
    # Reconstruct each R3 sample's ranks, nearest bracket, W, and residual rank.
    r3_states = {e["task"]: e for e in events if e.get("event") == "R3_DERIVED_STATE"}
    r3_tasks = {t["task"]: t for t in plan_by_round[3]["tasks"]}
    for name, task in r3_tasks.items():
        state = r3_states[name]
        u_order = sorted(task["U_ids"], key=lambda i: _item_key(items[i]))
        ranks = {str(i): j + 1 for j, i in enumerate(u_order) if i in set(task["V_ids"])}
        q = int(task["q"])
        lower = [int(rank) for rank in ranks.values() if int(rank) <= q]
        upper = [int(rank) for rank in ranks.values() if int(rank) > q]
        i_rank = max(lower, default=0)
        j_rank = min(upper, default=len(u_order) + 1)
        x = next((int(item) for item, rank in ranks.items() if int(rank) == i_rank), None)
        y = next((int(item) for item, rank in ranks.items() if int(rank) == j_rank), None)
        left_index = 0 if x is None else u_order.index(x)
        right_index = len(u_order) - 1 if y is None else u_order.index(y)
        wset = set(u_order[left_index:right_index + 1])
        w = [item for item in task["U_ids"] if item in wset]
        q_w = q - (i_rank - 1 if i_rank else 0)
        if state["sample_ranks"] != ranks or state["x_id"] != x or state["x_rank"] != i_rank or state["y_id"] != y or state["y_rank"] != j_rank:
            raise AssertionError(f"{name}: R3 sampled ranks/bracket mismatch")
        if state["W_ids"] != w or state["qW"] != q_w:
            raise AssertionError(f"{name}: R3 W or qW mismatch")
        if not (0 <= i_rank <= q < j_rank <= len(u_order) + 1 and 1 <= q_w <= len(w)):
            raise AssertionError(f"{name}: R3 residual rank invariant fails")
    expected = kth_index([int(x["score"]) for x in items if x["category"] == 2], trace["K"])
    if trace.get("status") != "SUCCESS" or trace.get("selected_original_index") != expected:
        raise AssertionError("trace candidate differs from independent stable kth answer")
    return {"trace_sha256": sha_file(path), "n": trace["n"], "K": trace["K"],
            "selected_original_index": expected, "per_round": round_accounting,
            "per_round_edges": {str(r): round_accounting[str(r)]["comparison_calls"] for r in range(1, 5)},
            "sum_edges": edge_count_total, "same_round_duplicate_edges": duplicate_count_total,
            "recomputed_outcome_digests": {str(r): results_by_round[r]["outcome_sha256"] for r in range(1, 5)},
            "barriers_validated": True, "R1_to_R4_derived_states_validated": True}


def _item_key(item: dict[str, Any]) -> tuple[int, int, int]:
    if item["category"] == 2:
        return (2, int(item["score"]), -int(item["original_index"]))
    return (int(item["category"]), int(item["serial"]), 0)


def check_archived_failures(evidence: Path) -> dict[str, Any]:
    path = evidence / "small_and_adversarial_pre_r3_window_fix" / "small_and_adversarial_cases.jsonl"
    statuses: Counter[str] = Counter()
    mismatch_rows: list[dict[str, Any]] = []
    candidate_emitted = 0
    for line in path.open(encoding="utf-8"):
        row = json.loads(line)
        statuses[row.get("validation_status", "MISSING")] += 1
        if row.get("validation_status") == "COMPLETED_WRONG":
            candidate_emitted += row.get("candidate_original_index") is not None
            mismatch_rows.append({"case": row.get("case"), "n": row.get("n"), "K": row.get("K"),
                                  "seed": row.get("algorithm_seed"), "candidate": row.get("candidate_original_index"),
                                  "oracle": row.get("oracle_original_index")})
    return {"preserved_pre_fix_rows": sum(statuses.values()), "status_counts": dict(statuses),
            "mismatch_examples_first_10": mismatch_rows[:10],
            "file_sha256": sha_file(path), "failure_count_in_file": len(mismatch_rows),
            "failed_rows_emitting_candidate": candidate_emitted}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-root", type=Path, required=True)
    parser.add_argument("--s4-source-root", type=Path, required=True,
                        help="read-only S4 experiment source directory for hash binding")
    parser.add_argument("--replay-root", type=Path, required=True,
                        help="fresh exact-S4-source matrix replay directory outside the repository")
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    evidence = args.evidence_root.resolve()
    result = {"auditor": "BMW16_S5_INDEPENDENT_RECEIVER_AUDIT_v1",
              "algorithm_label": LABEL,
              "method": "No imports from S4/S1 source; frozen cases regenerated; kth recomputed by sorted((-score,index)); trace edges/outcomes independently rebuilt.",
              "s4_source_hash_binding": check_s4_sources(args.s4_source_root.resolve(), evidence),
              "official_matrix": check_official(evidence),
              "fresh_s4_exact_source_replay": check_fresh_replay(evidence, args.replay_root),
              "repository_cpp_oracle": check_cpp_oracle(evidence),
              "small_and_adversarial": check_small(evidence),
              "full_trace": check_trace(evidence),
              "preserved_pre_fix_failure_archive": check_archived_failures(evidence),
              "independent_probability_and_linear_cost_audit": probability_cost_audit(),
              "integer_ceiling_and_Ucap_bound_audit": ceil_bound_audit(),
              "offline_material_capacity_precheck": capacity_audit()}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, sort_keys=True, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
