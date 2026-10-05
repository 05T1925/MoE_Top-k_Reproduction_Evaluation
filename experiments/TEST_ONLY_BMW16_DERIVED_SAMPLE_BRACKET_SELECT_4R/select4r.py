#!/usr/bin/env python3
"""Four-layer project-derived Select. TEST_ONLY plaintext; no secure runtime use."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import random
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

try:
    import numpy as np
except ImportError as exc:  # pragma: no cover - documented portability guard
    raise SystemExit("This TEST_ONLY implementation requires NumPy for batched edge execution") from exc


LABEL = "BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R_TEST_ONLY"
INT32_MIN = -(1 << 31)
INT32_MAX = (1 << 31) - 1


def ceil_sqrt(n: int) -> int:
    root = math.isqrt(n)
    return root if root * root == n else root + 1


def ceil_scaled_three_quarter(n: int, scale: int) -> int:
    """Return ceil(scale*n**(3/4)) using integer arithmetic only."""
    value = scale**4 * n**3
    lo, hi = 0, 1
    while hi**4 < value:
        hi *= 2
    while lo + 1 < hi:
        mid = (lo + hi) // 2
        if mid**4 >= value:
            hi = mid
        else:
            lo = mid
    return hi


def ceil_sqrt_ratio(numerator: int, denominator: int) -> int:
    floor = math.isqrt(numerator // denominator)
    return floor if floor * floor * denominator == numerator else floor + 1


def parameters(m: int) -> dict[str, int]:
    if m < 2 or m % 2:
        raise ValueError("median partition input size must be a positive even integer")
    s = min(m, ceil_sqrt(64 * m))
    if s == m:
        q_low, q_high = m // 2, m // 2 + 1
        a = 0
    else:
        a = ceil_sqrt(16 * s)
        q_low = max(1, (s + 1) // 2 - a)
        q_high = min(s, (s + 2) // 2 + a)
    u_cap = min(m, ceil_scaled_three_quarter(m, 8))
    gap = ceil_sqrt(4 * m)
    w_cap = 2 * gap + 1
    return {
        "M": m,
        "sample_size": s,
        "sample_offset": a,
        "sample_rank_low_1based": q_low,
        "sample_rank_high_1based": q_high,
        "U_cap": u_cap,
        "R3_gap_target": gap,
        "W_cap": w_cap,
    }


def _derive_seed(master: int, domain: str) -> int:
    payload = f"{master:016x}:{domain}".encode("ascii")
    return int.from_bytes(hashlib.sha256(payload).digest()[:8], "big")


@dataclass(frozen=True)
class Item:
    label: str
    category: int
    score: int
    original_index: int
    serial: int


class Context:
    def __init__(self, scores: list[int], k: int, input_seed: int, algorithm_seed: int, trace: bool):
        self.scores = scores
        self.n = len(scores)
        self.k = k
        self.input_seed = input_seed
        self.algorithm_seed = algorithm_seed
        self.trace_enabled = trace
        self.items: list[Item] = []
        self.labels: list[dict[str, Any]] = []
        self.serial = 0
        self.events: list[dict[str, Any]] = []
        self.metrics: dict[int, dict[str, int]] = {}
        self.seen_codes = np.empty(0, dtype=np.uint64)
        self.rngs: dict[str, random.Random] = {}
        self._build_inputs()
        self._arrays()

    def add_item(self, label: str, category: int, score: int = 0, original_index: int = -1) -> int:
        idx = len(self.items)
        self.items.append(Item(label, category, score, original_index, self.serial))
        self.serial += 1
        return idx

    def _build_inputs(self) -> None:
        if not self.scores or not 1 <= self.k <= self.n:
            raise ValueError("valid input requires n>0 and 1<=K<=n")
        if any(score < INT32_MIN or score > INT32_MAX for score in self.scores):
            raise ValueError("scores must be signed int32 Q20.12 raw integers")

        real_ids = [
            self.add_item(f"real:{i}", 2, score, i)
            for i, score in enumerate(self.scores)
        ]
        r = self.n - self.k + 1
        N = self.n + 1
        low_pad = max(N - 2 * r, 0)
        high_pad = max(2 * r - N, 0)
        m = N + low_pad + high_pad
        h = m // 2
        self.m = m
        self.h = h
        self.r = r
        self.low_pad = low_pad
        self.high_pad = high_pad
        self.real_ids = real_ids
        self.partitions: list[dict[str, Any]] = []
        for task, sentinel_category in (("SELECT_LOW_SENTINEL", 1), ("SELECT_HIGH_SENTINEL", 3)):
            lows = [
                self.add_item(f"{task}:rank-low:{i}", 0)
                for i in range(low_pad)
            ]
            high_sentinel = self.add_item(f"{task}:select-high", 3) if sentinel_category == 3 else None
            low_sentinel = self.add_item(f"{task}:select-low", 1) if sentinel_category == 1 else None
            highs = [
                self.add_item(f"{task}:rank-high:{i}", 4)
                for i in range(high_pad)
            ]
            if sentinel_category == 1:
                inputs = lows + [low_sentinel] + real_ids + highs
            else:
                inputs = lows + real_ids + [high_sentinel] + highs
            assert len(inputs) == m
            self.partitions.append({
                "task": task,
                "input": np.asarray(inputs, dtype=np.int32),
                "status": "PENDING",
                "reject_ids": None,
                "accept_ids": None,
                "reason": None,
                "params": parameters(m),
                "seed": _derive_seed(self.algorithm_seed, task),
                "sample_ids": None,
                "pivot_low": None,
                "pivot_high": None,
                "U_ids": None,
                "V_ids": None,
                "W_ids": None,
                "q": None,
                "qW": None,
                "sizes": {},
            })
        if self.r + low_pad + 1 != h + 1 or self.r + low_pad != h:
            raise AssertionError("two-sentinel arbitrary-rank mapping failed")

    def _arrays(self) -> None:
        self.cat = np.asarray([x.category for x in self.items], dtype=np.int8)
        self.p1 = np.asarray([x.score if x.category == 2 else x.serial for x in self.items], dtype=np.int64)
        self.p2 = np.asarray([-x.original_index if x.category == 2 else 0 for x in self.items], dtype=np.int64)

    def rng(self, domain: str) -> random.Random:
        if domain not in self.rngs:
            self.rngs[domain] = random.Random(_derive_seed(self.algorithm_seed, domain))
        return self.rngs[domain]

    def less(self, left: np.ndarray, right: np.ndarray) -> np.ndarray:
        lc, rc = self.cat[left], self.cat[right]
        lp1, rp1 = self.p1[left], self.p1[right]
        lp2, rp2 = self.p2[left], self.p2[right]
        same_cat = lc == rc
        same_p1 = lp1 == rp1
        return (lc < rc) | (same_cat & ((lp1 < rp1) | (same_p1 & (lp2 < rp2))))

    @staticmethod
    def _pair_codes(left: np.ndarray, right: np.ndarray) -> np.ndarray:
        low = np.minimum(left, right).astype(np.uint64)
        high = np.maximum(left, right).astype(np.uint64)
        return (low << np.uint64(32)) | high

    def execute_round(self, round_no: int, batches: list[dict[str, Any]]) -> dict[str, np.ndarray]:
        # This method freezes every task's complete edge list before reading any outcomes.
        total = sum(len(batch["left"]) for batch in batches)
        if any(len(batch["left"]) != len(batch["right"]) for batch in batches):
            raise AssertionError("edge arrays differ in length")
        plan_event: dict[str, Any] = {
            "event": "ROUND_PLAN_FROZEN",
            "round": round_no,
            "tasks": [batch["descriptor"] for batch in batches],
            "requires_results_through_round": round_no - 1,
            "edge_count": total,
        }
        all_left = np.concatenate([b["left"] for b in batches]) if batches else np.empty(0, dtype=np.int32)
        all_right = np.concatenate([b["right"] for b in batches]) if batches else np.empty(0, dtype=np.int32)
        codes = self._pair_codes(all_left, all_right)
        canonical = np.column_stack((np.minimum(all_left, all_right), np.maximum(all_left, all_right))).astype("<u4", copy=False)
        edge_digest = hashlib.sha256(canonical.tobytes()).hexdigest()
        plan_event["edge_sha256"] = edge_digest
        if self.trace_enabled:
            plan_event["edges"] = [
                [batch["descriptor"]["task"], int(a), int(b)]
                for batch in batches
                for a, b in zip(batch["left"], batch["right"])
            ]
        self.events.append(plan_event)

        # Comparison results are evaluated only after the plan event is committed.
        outcomes = self.less(all_left, all_right)
        result_digest = hashlib.sha256(outcomes.astype(np.uint8).tobytes()).hexdigest()
        counts: dict[str, int] = {}
        offset = 0
        for batch in batches:
            length = len(batch["left"])
            counts[batch["descriptor"]["task"]] = length
            batch["outcomes"] = outcomes[offset:offset + length]
            offset += length
        unique_codes = np.unique(codes)
        prior_repeat = int(np.isin(codes, self.seen_codes, assume_unique=False).sum()) if len(self.seen_codes) else 0
        round_duplicates = len(codes) - len(unique_codes)
        owners_by_code: dict[str, np.ndarray] = {}
        same_task_repeat_calls = 0
        cross_task_repeat_calls = 0
        for batch in batches:
            owner = batch["descriptor"]["task"]
            batch_codes = self._pair_codes(batch["left"], batch["right"])
            unique_batch_codes = np.unique(batch_codes)
            same_task_repeat_calls += len(batch_codes) - len(unique_batch_codes)
            if owners_by_code:
                already_requested = np.concatenate(list(owners_by_code.values()))
                cross_task_repeat_calls += int(np.isin(unique_batch_codes, already_requested).sum())
            owners_by_code[owner] = unique_batch_codes
        self.seen_codes = np.union1d(self.seen_codes, unique_codes)
        real_real = int(((self.cat[all_left] == 2) & (self.cat[all_right] == 2)).sum())
        self.metrics[round_no] = {
            "comparison_calls": int(total),
            "real_real_calls": real_real,
            "dummy_related_calls": int(total - real_real),
            "same_round_duplicate_calls": int(round_duplicates),
            "same_task_repeat_calls_same_round": int(same_task_repeat_calls),
            "cross_task_repeat_calls_same_round": int(cross_task_repeat_calls),
            "prior_round_repeat_calls": prior_repeat,
            "unique_unordered_edges_this_round": int(len(unique_codes)),
            "edge_sha256": edge_digest,
        }
        self.events.append({
            "event": "ROUND_RESULTS",
            "round": round_no,
            "outcome_sha256": result_digest,
            "outcome_count": int(len(outcomes)),
        })
        return {batch["descriptor"]["task"]: batch["outcomes"] for batch in batches}

    def rank_batch(self, ids: np.ndarray, left: np.ndarray, right: np.ndarray, outcomes: np.ndarray) -> dict[int, int]:
        # For each unordered pair, the higher item receives one lower item in its rank.
        higher = np.where(outcomes, right, left)
        add = np.bincount(higher, minlength=len(self.items))
        return {int(item): int(add[int(item)]) + 1 for item in ids}

    def _round1(self) -> None:
        batches = []
        for state in self.partitions:
            p = state["params"]
            ids = state["input"]
            sample = np.asarray(self.rng(f"{state['task']}:R1_SAMPLE").sample(ids.tolist(), p["sample_size"]), dtype=np.int32)
            state["sample_ids"] = sample
            li, ri = np.triu_indices(len(sample), k=1)
            left, right = sample[li], sample[ri]
            batches.append({
                "left": left,
                "right": right,
                "descriptor": {
                    "task": state["task"], "phase": "R1_SAMPLE_ALL_PAIRS",
                    "sample_ids": sample.tolist(), "sample_size": len(sample),
                    "qL": p["sample_rank_low_1based"], "qH": p["sample_rank_high_1based"],
                    "requires_results_through_round": 0,
                },
            })
        results = self.execute_round(1, batches)
        for state, batch in zip(self.partitions, batches):
            ranks = self.rank_batch(state["sample_ids"], batch["left"], batch["right"], batch["outcomes"])
            by_rank = {rank: item for item, rank in ranks.items()}
            p = state["params"]
            ql, qh = p["sample_rank_low_1based"], p["sample_rank_high_1based"]
            if len(by_rank) != len(state["sample_ids"]) or ql not in by_rank or qh not in by_rank or ql >= qh:
                state["status"] = "ABORT_R1_SAMPLE_RANK_INVALID"
                state["reason"] = "R1_SAMPLE_PAIRWISE_RANK_NOT_UNIQUE"
                continue
            state["pivot_low"] = by_rank[ql]
            state["pivot_high"] = by_rank[qh]
        if self.trace_enabled:
            self.events.append({
                "event": "R1_DERIVED_STATE",
                "partitions": [{
                    "task": s["task"], "pivot_low": s["pivot_low"], "pivot_high": s["pivot_high"],
                    "status": s["status"], "reason": s["reason"],
                } for s in self.partitions],
            })

    def _round2(self) -> None:
        batches = []
        descriptors: dict[str, dict[str, Any]] = {}
        for state in self.partitions:
            if state["status"] != "PENDING":
                continue
            ids = state["input"]
            x, y = int(state["pivot_low"]), int(state["pivot_high"])
            left_x = ids[ids != x]
            right_x = np.full(len(left_x), x, dtype=np.int32)
            left_y = ids[ids != y]
            right_y = np.full(len(left_y), y, dtype=np.int32)
            left = np.concatenate((left_x, left_y))
            right = np.concatenate((right_x, right_y))
            d = {
                "task": state["task"], "phase": "R2_INPUT_TO_TWO_PIVOTS",
                "input_ids": ids.tolist(), "pivot_low": x, "pivot_high": y,
                "requires_results_through_round": 1,
            }
            descriptors[state["task"]] = {"len_x": len(left_x), "len_y": len(left_y)}
            batches.append({"left": left, "right": right, "descriptor": d})
        outcomes_by_task = self.execute_round(2, batches)
        for state, batch in zip([s for s in self.partitions if s["status"] == "PENDING"], batches):
            ids = state["input"]
            x, y = int(state["pivot_low"]), int(state["pivot_high"])
            nx = descriptors[state["task"]]["len_x"]
            ny = descriptors[state["task"]]["len_y"]
            result = batch["outcomes"]
            lt_x = np.zeros(len(ids), dtype=bool)
            lt_y = np.zeros(len(ids), dtype=bool)
            id_pos = {int(item): pos for pos, item in enumerate(ids)}
            lt_x[[id_pos[int(i)] for i in ids[ids != x]]] = result[:nx]
            lt_y[[id_pos[int(i)] for i in ids[ids != y]]] = result[nx:nx + ny]
            c = int(lt_x.sum())
            cy = int(lt_y.sum())
            in_u = (~lt_x) & (lt_y | (ids == y))
            u_ids = ids[in_u]
            q = state["h"] - c if "h" in state else self.h - c
            state["U_ids"] = u_ids
            state["prefix_ids"] = ids[lt_x]
            state["suffix_ids"] = ids[(~lt_y) & (ids != y)]
            state["q"] = q
            state["sizes"].update({"c_low": c, "c_before_high": cy, "U": len(u_ids)})
            if not (c <= self.h <= cy):
                state["status"] = "ABORT_R2_MEDIAN_NOT_BRACKETED"
                state["reason"] = "R2_PIVOTS_DO_NOT_BRACKET_MEDIAN_CUT"
            elif len(u_ids) > state["params"]["U_cap"]:
                state["status"] = "ABORT_R2_U_OVERSIZE"
                state["reason"] = "R2_U_EXCEEDS_PROVED_LINEAR_WINDOW_CAP"
            else:
                state["status"] = "R2_READY"
            if self.trace_enabled:
                self.events.append({
                    "event": "R2_DERIVED_STATE", "task": state["task"],
                    "prefix_ids": state["prefix_ids"].tolist(), "suffix_ids": state["suffix_ids"].tolist(),
                    "U_ids": u_ids.tolist(), "q": q, "status": state["status"], "reason": state["reason"],
                })

    def _round3(self) -> None:
        batches = []
        active: list[dict[str, Any]] = []
        for state in self.partitions:
            if state["status"] != "R2_READY":
                continue
            u_ids = state["U_ids"]
            u = len(u_ids)
            q = int(state["q"])
            if u < 2 or q < 0 or q >= u:
                state["status"] = "COMPLETED_WRONG"
                state["reason"] = f"R2_BRACKET_INVARIANT_VIOLATION:u={u},q={q}"
                continue
            if q == 0:
                state["reject_ids"] = np.asarray(state["prefix_ids"], dtype=np.int32)
                state["accept_ids"] = np.concatenate((state["suffix_ids"], u_ids))
                state["status"] = "SUCCESS"
                state["reason"] = "R2_EXACT_RESIDUAL_ENDPOINT"
                continue
            vsize = min(u, ceil_sqrt_ratio(64 * u * u, self.m))
            v_ids = np.asarray(self.rng(f"{state['task']}:R3_SAMPLE").sample(u_ids.tolist(), vsize), dtype=np.int32)
            state["V_ids"] = v_ids
            vset = set(int(x) for x in v_ids)
            other = np.asarray([int(x) for x in u_ids if int(x) not in vset], dtype=np.int32)
            li, ri = np.triu_indices(vsize, k=1)
            internal_left, internal_right = v_ids[li], v_ids[ri]
            external_left = np.repeat(v_ids, len(other))
            external_right = np.tile(other, vsize)
            left = np.concatenate((internal_left, external_left))
            right = np.concatenate((internal_right, external_right))
            descriptor = {
                "task": state["task"], "phase": "R3_SAMPLE_CROSS_U",
                "U_ids": u_ids.tolist(), "V_ids": v_ids.tolist(), "q": q,
                "requires_results_through_round": 2,
            }
            batches.append({"left": left, "right": right, "descriptor": descriptor,
                            "internal_count": len(internal_left), "v_ids": v_ids,
                            "u_ids": u_ids, "other_ids": other})
            active.append(state)
        self.execute_round(3, batches)
        for state, batch in zip(active, batches):
            v_ids = batch["v_ids"]
            u_ids = batch["u_ids"]
            vsize = len(v_ids)
            other = batch["other_ids"]
            internal_count = batch["internal_count"]
            result = batch["outcomes"]
            matrix = np.zeros((vsize, len(u_ids)), dtype=bool)
            u_pos = {int(item): i for i, item in enumerate(u_ids)}
            v_pos = {int(item): i for i, item in enumerate(v_ids)}
            il, ir = np.triu_indices(vsize, k=1)
            internal_outcomes = result[:internal_count]
            internal_u_cols = np.asarray([u_pos[int(v_ids[i])] for i in range(vsize)], dtype=np.int64)
            matrix[il, internal_u_cols[ir]] = internal_outcomes
            matrix[ir, internal_u_cols[il]] = ~internal_outcomes
            ext_outcomes = result[internal_count:]
            if len(other):
                ext_rows = np.repeat(np.arange(vsize), len(other))
                ext_cols = np.tile(np.asarray([u_pos[int(item)] for item in other], dtype=np.int64), vsize)
                matrix[ext_rows, ext_cols] = ext_outcomes
            ranks = np.count_nonzero(~matrix, axis=1) - 1
            ranks_by_item = {int(item): int(rank + 1) for item, rank in zip(v_ids, ranks)}
            q = int(state["q"])
            lower = [(item, rank) for item, rank in ranks_by_item.items() if rank <= q]
            upper = [(item, rank) for item, rank in ranks_by_item.items() if rank > q]
            i = max((rank for _, rank in lower), default=0)
            j = min((rank for _, rank in upper), default=len(u_ids) + 1)
            x_id = next((item for item, rank in lower if rank == i), None)
            y_id = next((item for item, rank in upper if rank == j), None)
            u = len(u_ids)
            x_row = None if x_id is None else v_pos[int(x_id)]
            y_row = None if y_id is None else v_pos[int(y_id)]
            w_ids = np.asarray([
                int(item) for col, item in enumerate(u_ids)
                if (x_row is None or int(item) == int(x_id) or matrix[x_row, col])
                and (y_row is None or int(item) == int(y_id) or not matrix[y_row, col])
            ], dtype=np.int32)
            q_w = q - (i - 1 if i else 0)
            gap = state["params"]["R3_gap_target"]
            w_cap = state["params"]["W_cap"]
            state["W_ids"] = w_ids
            state["x_rank"] = i
            state["y_rank"] = j
            state["x_id"] = x_id
            state["y_id"] = y_id
            state["qW"] = q_w
            state["sizes"].update({"V": vsize, "W": len(w_ids), "R3_v": vsize, "R3_gap_target": gap})
            if not (0 <= i <= q < j <= u + 1 and 1 <= q_w <= len(w_ids)):
                state["status"] = "ABORT_R3_RESIDUAL_RANK_INVALID"
                state["reason"] = "R3_SAMPLE_OR_VIRTUAL_BOUNDARY_DOES_NOT_BRACKET_Q"
            elif len(w_ids) > w_cap:
                state["status"] = "ABORT_R3_W_OVERSIZE"
                state["reason"] = "R3_W_EXCEEDS_PROVED_FINAL_WINDOW_CAP"
            else:
                state["status"] = "R3_READY"
            if self.trace_enabled:
                self.events.append({
                    "event": "R3_DERIVED_STATE", "task": state["task"],
                    "sample_ranks": {str(k): v for k, v in ranks_by_item.items()},
                    "x_id": x_id, "x_rank": i, "y_id": y_id, "y_rank": j,
                    "W_ids": w_ids.tolist(), "qW": q_w,
                    "status": state["status"], "reason": state["reason"],
                })
            state["r3_matrix"] = matrix
            state["r3_u_pos"] = u_pos
            state["r3_v_pos"] = v_pos

    def _round4(self) -> None:
        batches = []
        active = []
        for state in self.partitions:
            if state["status"] != "R3_READY":
                continue
            w = state["W_ids"]
            li, ri = np.triu_indices(len(w), k=1)
            batches.append({
                "left": w[li], "right": w[ri],
                "descriptor": {
                    "task": state["task"], "phase": "R4_W_ALL_PAIRS",
                    "W_ids": w.tolist(), "qW": state["qW"],
                    "requires_results_through_round": 3,
                },
            })
            active.append(state)
        self.execute_round(4, batches)
        for state, batch in zip(active, batches):
            w_ids = state["W_ids"]
            ranks = self.rank_batch(w_ids, batch["left"], batch["right"], batch["outcomes"])
            q_w = int(state["qW"])
            reject_w = [item for item in w_ids if ranks[int(item)] <= q_w]
            if len(reject_w) != q_w or len(set(ranks.values())) != len(w_ids):
                state["status"] = "COMPLETED_WRONG"
                state["reason"] = "R4_PAIRWISE_RANKS_NOT_A_STRICT_TOTAL_ORDER"
                continue
            below = []
            above = []
            matrix = state["r3_matrix"]
            u_ids = state["U_ids"]
            u_pos = state["r3_u_pos"]
            if state["x_id"] is not None:
                row = state["r3_v_pos"][int(state["x_id"])]
                below = [int(item) for j, item in enumerate(u_ids) if int(item) != int(state["x_id"]) and not matrix[row, j]]
            if state["y_id"] is not None:
                row = state["r3_v_pos"][int(state["y_id"])]
                above = [int(item) for j, item in enumerate(u_ids) if int(item) != int(state["y_id"]) and matrix[row, j]]
            reject_ids = list(map(int, state["prefix_ids"])) + below + list(map(int, reject_w))
            accept_ids = list(map(int, state["suffix_ids"])) + above + [
                int(item) for item in w_ids if int(item) not in set(reject_w)
            ]
            if len(reject_ids) != self.h or len(set(reject_ids + accept_ids)) != self.m:
                state["status"] = "COMPLETED_WRONG"
                state["reason"] = (
                    "R4_PARTITION_CARDINALITY_OR_COVERAGE_INVARIANT_FAILED:"
                    f"reject={len(reject_ids)},accept={len(accept_ids)},"
                    f"union={len(set(reject_ids + accept_ids))},M={self.m},"
                    f"prefix={len(state['prefix_ids'])},below={len(below)},"
                    f"rejectW={len(reject_w)},suffix={len(state['suffix_ids'])},above={len(above)},"
                    f"acceptW={len(w_ids)-len(reject_w)}"
                )
                continue
            state["reject_ids"] = np.asarray(reject_ids, dtype=np.int32)
            state["accept_ids"] = np.asarray(accept_ids, dtype=np.int32)
            state["status"] = "SUCCESS"
            state["reason"] = "R4_EXACT_RESIDUAL_RANK_INCLUDES_BOUNDARY_ELEMENT"

    def run(self) -> dict[str, Any]:
        self._round1()
        self._round2()
        self._round3()
        self._round4()
        statuses = {state["task"]: state["status"] for state in self.partitions}
        if any(status != "SUCCESS" for status in statuses.values()):
            random_abort_states = {
                "ABORT_R2_MEDIAN_NOT_BRACKETED", "ABORT_R2_U_OVERSIZE", "ABORT_R3_W_OVERSIZE"
            }
            status = ("PROJECT_RANDOM_FAILURE_PATH"
                      if any(value in random_abort_states for value in statuses.values())
                      else "COMPLETED_WRONG")
            candidates: list[int] = []
        else:
            low, high = self.partitions
            low_accept = set(map(int, low["accept_ids"]))
            high_reject = set(map(int, high["reject_ids"]))
            candidates = [
                self.items[item].original_index
                for item in low_accept & high_reject
                if self.items[item].category == 2
            ]
            status = "SUCCESS" if len(candidates) == 1 else "COMPLETED_WRONG"
        per_round = {}
        for round_no in range(1, 5):
            per_round[str(round_no)] = self.metrics.get(round_no, {
                "comparison_calls": 0, "real_real_calls": 0, "dummy_related_calls": 0,
                "same_round_duplicate_calls": 0, "same_task_repeat_calls_same_round": 0,
                "cross_task_repeat_calls_same_round": 0,
                "prior_round_repeat_calls": 0, "unique_unordered_edges_this_round": 0,
                "edge_sha256": hashlib.sha256(b"").hexdigest(),
            })
        summary: dict[str, Any] = {
            "label": LABEL,
            "status": status,
            "result_class": status,
            "n": self.n,
            "K": self.k,
            "effective_median_size_M": self.m,
            "median_reject_count_h": self.h,
            "rank_padding_low": self.low_pad,
            "rank_padding_high": self.high_pad,
            "input_seed": self.input_seed,
            "algorithm_seed": self.algorithm_seed,
            "epsilon": "not-used (finite sample-bracket construction)",
            "comparison_rounds_max": 4,
            "selected_original_index": candidates[0] if len(candidates) == 1 else None,
            "candidate_original_indices": candidates,
            "partition_statuses": statuses,
            "partitions": {
                s["task"]: {
                    "status": s["status"], "reason": s["reason"],
                    "M": self.m, "h": self.h, "U_size": s["sizes"].get("U"),
                    "V_size": s["sizes"].get("V"), "W_size": s["sizes"].get("W"),
                    "R3_sample_ranks": {
                        "x": s.get("x_rank"), "y": s.get("y_rank"), "qW": s.get("qW")
                    },
                    "R1_sample_size": s["params"]["sample_size"],
                    "R1_qL": s["params"]["sample_rank_low_1based"],
                    "R1_qH": s["params"]["sample_rank_high_1based"],
                    "U_cap": s["params"]["U_cap"], "W_cap": s["params"]["W_cap"],
                    "failure_reason": s["reason"] if s["status"] != "SUCCESS" else None,
                } for s in self.partitions
            },
            "round_metrics": per_round,
            "total_comparison_calls": sum(x["comparison_calls"] for x in per_round.values()),
            "unique_unordered_edges_all_rounds": int(len(self.seen_codes)),
            "repeated_total_calls": sum(x["comparison_calls"] for x in per_round.values())
                                    - int(len(self.seen_codes)),
            "input_sha256": hashlib.sha256(json.dumps(self.scores, separators=(",", ":")).encode()).hexdigest(),
        }
        if self.trace_enabled:
            summary["trace"] = {
                "items": [
                    {"id": i, "label": x.label, "category": x.category,
                     "score": x.score if x.category == 2 else None,
                     "original_index": x.original_index if x.category == 2 else None,
                     "serial": x.serial}
                    for i, x in enumerate(self.items)
                ],
                "events": self.events,
            }
        return summary


def run_select(scores: list[int], k: int, input_seed: int, algorithm_seed: int, trace: bool = False) -> dict[str, Any]:
    return Context(scores, k, input_seed, algorithm_seed, trace).run()


def main() -> int:
    parser = argparse.ArgumentParser(description=LABEL)
    parser.add_argument("--scores-json", type=Path, required=True)
    parser.add_argument("--k", type=int, required=True)
    parser.add_argument("--input-seed", type=int, default=0)
    parser.add_argument("--algorithm-seed", type=int, required=True)
    parser.add_argument("--trace", type=Path)
    args = parser.parse_args()
    scores = json.loads(args.scores_json.read_text(encoding="utf-8"))
    result = run_select(scores, args.k, args.input_seed, args.algorithm_seed, trace=bool(args.trace))
    if args.trace:
        args.trace.write_text(json.dumps(result, separators=(",", ":")), encoding="utf-8")
    else:
        result.pop("trace", None)
        print(json.dumps(result, separators=(",", ":")))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
