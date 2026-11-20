"""BMW16-derived four-round Select, plaintext TEST_ONLY reference.

This module deliberately contains no oracle import and no secure-protocol code.
All comparisons are frozen in a complete round batch before that batch is
evaluated. The implementation is a project-derived repair of the r=1 A5 and
A7 rank/boundary details; it is not an author-exact reproduction.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from fractions import Fraction
import hashlib
import itertools
import json
from typing import Any, Iterable


LABEL = "BMW16_DERIVED_SELECT_4R_TEST_ONLY"
EPSILON = Fraction(1, 36)
EPSILON5 = Fraction(1, 72)
R3_SAMPLE_MULTIPLIER = 32
INT32_MIN = -(1 << 31)
INT32_MAX = (1 << 31) - 1
MASK64 = (1 << 64) - 1


def _ceil_power(n: int, p: int, q: int) -> int:
    """Smallest x with x**q >= n**p; exact integer arithmetic."""
    if n < 0 or p <= 0 or q <= 0:
        raise ValueError("power-root arguments must be positive")
    target = n**p
    lo, hi = 0, 1
    while hi**q < target:
        hi *= 2
    while lo + 1 < hi:
        mid = (lo + hi) // 2
        if mid**q >= target:
            hi = mid
        else:
            lo = mid
    return hi


def _floor_power(n: int, p: int, q: int) -> int:
    value = _ceil_power(n, p, q)
    if value**q == n**p:
        return value
    return value - 1


def _derive_seed(master: int, label: str) -> int:
    digest = hashlib.sha256(f"BMW16-S2|{master}|{label}".encode("utf-8")).digest()
    return int.from_bytes(digest[:8], "big")


class SplitMix64:
    """Tiny deterministic replay PRNG; not a cryptographic random generator."""

    def __init__(self, seed: int):
        self.state = seed & MASK64

    def next_u64(self) -> int:
        self.state = (self.state + 0x9E3779B97F4A7C15) & MASK64
        z = self.state
        z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & MASK64
        z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & MASK64
        return (z ^ (z >> 31)) & MASK64

    def randbelow(self, n: int) -> int:
        if n <= 0:
            raise ValueError("randbelow requires n > 0")
        limit = (1 << 64) - ((1 << 64) % n)
        while True:
            value = self.next_u64()
            if value < limit:
                return value % n

    def sample(self, population: list[str], count: int) -> list[str]:
        if count < 0 or count > len(population):
            raise ValueError("sample size is outside the population")
        work = list(population)
        for i in range(count):
            j = i + self.randbelow(len(work) - i)
            work[i], work[j] = work[j], work[i]
        return work[:count]

    def bit(self) -> int:
        return self.next_u64() & 1


@dataclass(frozen=True)
class Item:
    item_id: str
    kind: str
    score: int | None = None
    original_index: int | None = None
    band: int = 1
    tie_tag: str = ""

    def order_key(self) -> tuple[int, int, int, str]:
        # Internal order is low priority -> high priority. Real ties are
        # reversed by original_index so the public priority remains score
        # descending, then original_index ascending.
        if self.kind == "a7_scratch_low":
            return (-1, 0, 0, self.item_id)
        if self.kind == "a7_scratch_high":
            return (3, 0, 0, self.item_id)
        if self.kind in {"rank_low", "select_low"}:
            return (0, 0 if self.kind == "rank_low" else 1, 0, self.item_id)
        if self.kind == "real":
            assert self.score is not None and self.original_index is not None
            return (1, self.score, -self.original_index, self.item_id)
        if self.kind in {"select_high", "rank_high"}:
            return (2, 0 if self.kind == "select_high" else 1, 0, self.item_id)
        raise ValueError(f"unknown item kind {self.kind}")

    def trace_record(self) -> dict[str, Any]:
        return {
            "id": self.item_id,
            "kind": self.kind,
            "score": self.score,
            "original_index": self.original_index,
            "order_key": list(self.order_key()),
        }


@dataclass(frozen=True)
class Edge:
    task: str
    left: str
    right: str
    round_no: int
    max_info_round: int
    generator: str

    def trace_record(self, items: dict[str, Item]) -> dict[str, Any]:
        a, b = items[self.left], items[self.right]
        return {
            "task": self.task,
            "left": self.left,
            "right": self.right,
            "left_kind": a.kind,
            "right_kind": b.kind,
            "round": self.round_no,
            "max_info_round": self.max_info_round,
            "generator": self.generator,
        }


@dataclass
class A5State:
    task_id: str
    input_ids: list[str]
    sample_ids: list[str]
    pivot_sample_ids: list[str]
    sample_seed: int
    target_lower_count: int
    pivot_a: str | None = None
    pivot_b: str | None = None
    pivot_x: str | None = None
    labels: dict[str, bool] = field(default_factory=dict)
    reason: str | None = None
    lower_counts: dict[str, int] = field(default_factory=dict)


@dataclass
class A7State:
    instance_id: str
    input_ids: list[str]
    m: int
    half: int
    dummy_count: int
    rng_seed: int
    low_a5: A5State
    high_a5: A5State
    labels: dict[str, bool] = field(default_factory=dict)  # True=Accept.
    status: str = "PENDING"
    reason: str | None = None
    r1_ids: set[str] = field(default_factory=set)
    a2_ids: set[str] = field(default_factory=set)
    u_ids: list[str] = field(default_factory=list)
    residual_reject_count: int | None = None
    v_ids: list[str] = field(default_factory=list)
    x_id: str | None = None
    y_id: str | None = None
    w_ids: list[str] = field(default_factory=list)
    rstar_count: int | None = None
    exact_via_all_pairs: bool = False
    random_branch: str | None = None


def _normalized_pair(a: str, b: str) -> tuple[str, str]:
    return (a, b) if a < b else (b, a)


def _r1_edges(state: A5State) -> list[Edge]:
    edges: list[Edge] = []
    seen: set[tuple[str, str]] = set()
    tset = set(state.pivot_sample_ids)
    for s in state.sample_ids:
        for t in state.pivot_sample_ids:
            if s == t:
                continue
            pair = _normalized_pair(s, t)
            if pair in seen:
                continue
            seen.add(pair)
            edges.append(Edge(state.task_id, s, t, 1, 0, "A5_S1_x_T1"))
    return edges


def _r2_edges(state: A5State) -> list[Edge]:
    assert state.pivot_x is not None
    return [
        Edge(state.task_id, state.pivot_x, item_id, 2, 1, "A5_all_to_selected_pivot")
        for item_id in state.input_ids
        if item_id != state.pivot_x
    ]


class Select4R:
    """Four-round Select over signed Q20.12 scores with stable original index."""

    def __init__(self, scores: list[int], k: int, algo_seed: int, input_seed: int | None = None):
        self.scores = list(scores)
        self.n = len(scores)
        self.k = k
        self.algo_seed = int(algo_seed)
        self.input_seed = input_seed
        self.items: dict[str, Item] = {}
        self.events: list[dict[str, Any]] = []
        self.round_edges: dict[int, list[Edge]] = {i: [] for i in range(1, 5)}
        self.round_outcomes: dict[int, dict[tuple[str, str, str], bool]] = {i: {} for i in range(1, 5)}
        self.a5: dict[str, A5State] = {}
        self.a7: dict[str, A7State] = {}
        self.round_metrics: dict[int, dict[str, int]] = {}
        self.expansion: dict[str, int] | None = None
        self.final_status = "PENDING"
        self.failure_reasons: list[str] = []
        self.candidate_ids: list[str] = []
        self.selected_original_index: int | None = None
        self.oracle_validation: dict[str, Any] = {"status": "NOT_RUN_BY_ALGORITHM"}

    def _add(self, item: Item) -> str:
        if item.item_id in self.items:
            if self.items[item.item_id] != item:
                raise ValueError(f"conflicting item identity {item.item_id}")
            return item.item_id
        self.items[item.item_id] = item
        return item.item_id

    def _real_items(self) -> list[str]:
        result = []
        for i, score in enumerate(self.scores):
            result.append(self._add(Item(f"real:{i}", "real", score, i)))
        return result

    def _sample_a5(self, task: str, input_ids: list[str], seed_label: str) -> A5State:
        N = len(input_ids)
        s = min(N, _ceil_power(N, 2, 3))
        p = min(s, _ceil_power(N, 1, 3))
        seed = _derive_seed(self.algo_seed, seed_label)
        rng = SplitMix64(seed)
        S = rng.sample(input_ids, s)
        T = rng.sample(S, p)
        # PAPER_DERIVED: k1 is a lower-count threshold within A1=S1; q=k1+1
        # is the chosen upper median position, never the full input N/2.
        k1 = s // 2
        state = A5State(task, list(input_ids), S, T, seed, k1)
        self.a5[task] = state
        return state

    def _new_a7(self, instance_id: str, current_ids: list[str]) -> A7State:
        m = len(current_ids)
        if m == 0 or m % 2:
            raise ValueError("A7 median input must be non-empty and even")
        D = _ceil_power(m, 25, 36)
        seed = _derive_seed(self.algo_seed, f"{instance_id}:A7")
        low_ids: list[str] = []
        high_ids: list[str] = []
        for i in range(2 * D):
            low_ids.append(self._add(Item(f"{instance_id}:a7-low:{i}", "a7_scratch_low", band=-1)))
            high_ids.append(self._add(Item(f"{instance_id}:a7-high:{i}", "a7_scratch_high", band=3)))
        low_state = self._sample_a5(
            f"{instance_id}:A5_LOW", low_ids + current_ids,
            f"{instance_id}:A5_LOW:sample",
        )
        high_state = self._sample_a5(
            f"{instance_id}:A5_HIGH", current_ids + high_ids,
            f"{instance_id}:A5_HIGH:sample",
        )
        state = A7State(instance_id, list(current_ids), m, m // 2, D, seed, low_state, high_state)
        self.a7[instance_id] = state
        return state

    def _build_reduction_inputs(self) -> tuple[A7State, A7State]:
        if not self.scores:
            raise ValueError("Select input must be non-empty")
        if not 1 <= self.k <= self.n:
            raise ValueError("K must satisfy 1 <= K <= n")
        if any(not INT32_MIN <= score <= INT32_MAX for score in self.scores):
            raise ValueError("scores must be signed int32 Q20.12 integers")
        # Ascending rank r=n-K+1 is the K-th highest stable priority.
        r = self.n - self.k + 1
        N = self.n + 1
        low_pad = max(N - 2 * r, 0)
        high_pad = max(2 * r - N, 0)
        M = N + low_pad + high_pad
        h = M // 2
        real_ids = self._real_items()
        rank_low = [self._add(Item(f"rank-low:{i}", "rank_low")) for i in range(low_pad)]
        rank_high = [self._add(Item(f"rank-high:{i}", "rank_high")) for i in range(high_pad)]
        low_sentinel = self._add(Item("select-sentinel:low", "select_low"))
        high_sentinel = self._add(Item("select-sentinel:high", "select_high"))
        low_input = rank_low + [low_sentinel] + real_ids + rank_high
        high_input = rank_low + real_ids + [high_sentinel] + rank_high
        if len(low_input) != M or len(high_input) != M or M % 2:
            raise AssertionError("two-sentinel expansion must have even equal size")
        if r + low_pad + 1 != h + 1 or r + low_pad != h:
            raise AssertionError("Appendix A selected-rank mapping failed")
        self.expansion = {
            "n": self.n,
            "K": self.k,
            "ascending_rank_1_based": r,
            "N_with_one_sentinel": N,
            "low_padding": low_pad,
            "high_padding": high_pad,
            "effective_median_size": M,
            "median_reject_count": h,
            "algorithm_epsilon": "1/36",
            "input_seed": self.input_seed,
            "algorithm_seed": self.algo_seed,
            "two_select_partitions_parallel": True,
        }
        return self._new_a7("SELECT_LOW_SENTINEL", low_input), self._new_a7(
            "SELECT_HIGH_SENTINEL", high_input
        )

    def _freeze_and_execute(self, round_no: int, tasks: list[dict[str, Any]], edges: list[Edge]) -> None:
        # Batch is frozen as one manifest before any result is evaluated.
        plan = {
            "event": "ROUND_PLAN_FROZEN",
            "round": round_no,
            "tasks": tasks,
            "edges": [edge.trace_record(self.items) for edge in edges],
        }
        plan_bytes = json.dumps(plan, sort_keys=True, separators=(",", ":")).encode("utf-8")
        plan["plan_sha256"] = hashlib.sha256(plan_bytes).hexdigest()
        self.events.append(plan)
        outcomes: list[bool] = []
        lookup: dict[tuple[str, str, str], bool] = {}
        for edge in edges:
            left = self.items[edge.left].order_key()
            right = self.items[edge.right].order_key()
            bit = left < right
            outcomes.append(bit)
            lookup[(edge.task, *_normalized_pair(edge.left, edge.right))] = (
                bit if edge.left < edge.right else not bit
            )
        self.round_outcomes[round_no] = lookup
        self.round_edges[round_no] = list(edges)
        self.events.append({"event": "ROUND_RESULTS", "round": round_no, "left_lt_right": outcomes})

    def _less_from_round(self, round_no: int, task: str, a: str, b: str) -> bool:
        if a == b:
            return False
        pair = _normalized_pair(a, b)
        value = self.round_outcomes[round_no][(task, pair[0], pair[1])]
        return value if a == pair[0] else not value

    def _stage_r1(self) -> None:
        tasks = []
        edges: list[Edge] = []
        for state in self.a5.values():
            tasks.append({
                "task": state.task_id,
                "phase": "A5_R1",
                "input_ids": state.input_ids,
                "S1_ids": state.sample_ids,
                "T1_ids": state.pivot_sample_ids,
                "target_lower_count_in_S1": state.target_lower_count,
                "sample_seed": state.sample_seed,
                "requires_results_through_round": 0,
            })
            edges.extend(_r1_edges(state))
        self._freeze_and_execute(1, tasks, edges)
        for state in self.a5.values():
            for pivot in state.pivot_sample_ids:
                state.lower_counts[pivot] = sum(
                    1 for sample_id in state.sample_ids
                    if sample_id != pivot and self._less_from_round(1, state.task_id, sample_id, pivot)
                )
            lower = [
                item_id for item_id in state.pivot_sample_ids
                if state.lower_counts[item_id] <= state.target_lower_count
            ]
            upper = [
                item_id for item_id in state.pivot_sample_ids
                if state.lower_counts[item_id] >= state.target_lower_count
            ]
            if not lower or not upper:
                state.reason = "A5_R1_MISSING_ONE_SIDE_OF_S1_TARGET_BRACKET"
                continue
            state.pivot_a = max(lower, key=lambda item_id: state.lower_counts[item_id])
            state.pivot_b = min(upper, key=lambda item_id: state.lower_counts[item_id])
            if state.lower_counts[state.pivot_a] > state.lower_counts[state.pivot_b]:
                state.reason = "A5_R1_BRACKET_ORDER_INVALID"
                state.pivot_a = state.pivot_b = None
                continue
            # The paper permits any point in [a1,b1]. PROJECT_DERIVED tie:
            # choose the lower endpoint for the low-dummy run and the upper
            # endpoint for the high-dummy run, so the two cuts face outward.
            state.pivot_x = (
                state.pivot_b if state.task_id.endswith("A5_HIGH") else state.pivot_a
            )

    def _stage_r2(self) -> None:
        tasks = []
        edges: list[Edge] = []
        for state in self.a5.values():
            if state.pivot_x is None:
                continue
            tasks.append({
                "task": state.task_id,
                "phase": "A5_R2",
                "input_ids": state.input_ids,
                "pivot_x": state.pivot_x,
                "pivot_a": state.pivot_a,
                "pivot_b": state.pivot_b,
                "requires_results_through_round": 1,
            })
            edges.extend(_r2_edges(state))
        self._freeze_and_execute(2, tasks, edges)
        for state in self.a5.values():
            if state.pivot_x is None:
                continue
            for item_id in state.input_ids:
                # Accept iff item is strictly greater than x; x itself is Reject.
                state.labels[item_id] = (
                    item_id != state.pivot_x
                    and self._less_from_round(2, state.task_id, state.pivot_x, item_id)
                )

    def _a7_after_r2(self, state: A7State) -> None:
        if state.low_a5.pivot_x is None or state.high_a5.pivot_x is None:
            state.status = "UNDEFINED/INVALID_FINITE_CASE"
            missing = [
                sub.reason for sub in (state.low_a5, state.high_a5) if sub.pivot_x is None
            ]
            state.reason = ";".join(missing) or "A5_R2_NOT_RUN"
            self.failure_reasons.append(f"{state.instance_id}:{state.reason}")
            return
        low_reject = {
            item_id for item_id in state.input_ids
            if not state.low_a5.labels[item_id]
        }
        high_accept = {
            item_id for item_id in state.input_ids
            if state.high_a5.labels[item_id]
        }
        state.r1_ids = low_reject
        state.a2_ids = high_accept
        conflict = low_reject & high_accept
        c1 = len(low_reject)
        c2 = state.m - len(high_accept)
        state.u_ids = [
            item_id for item_id in state.input_ids
            if item_id not in low_reject and item_id not in high_accept
        ]
        paper_u = {
            item_id for item_id in state.input_ids
            if not state.high_a5.labels[item_id] and state.low_a5.labels[item_id]
        }
        if set(state.u_ids) != paper_u:
            state.status = "UNDEFINED/INVALID_FINITE_CASE"
            state.reason = "A7_U_INTERSECTION_PARTITION_IDENTITY_FAILED"
            self.failure_reasons.append(f"{state.instance_id}:{state.reason}")
            return
        if conflict or c1 > c2 or c1 > state.half or c2 < state.half:
            state.status = "PAPER_RANDOM_FAILURE_PATH"
            state.reason = "A7_FIRST_TWO_CUTS_CROSS_OR_FAIL_TO_BRACKET_MEDIAN"
            self.failure_reasons.append(f"{state.instance_id}:{state.reason}")
            return
        if len(state.u_ids) != c2 - c1:
            state.status = "UNDEFINED/INVALID_FINITE_CASE"
            state.reason = "A7_U_NOT_A_CONTIGUOUS_INTERVAL_BY_CARDINALITY"
            self.failure_reasons.append(f"{state.instance_id}:{state.reason}")
            return
        q = state.half - c1
        if not 0 <= q <= len(state.u_ids):
            state.status = "PAPER_RANDOM_FAILURE_PATH"
            state.reason = "A7_RESIDUAL_REJECT_RANK_OUT_OF_RANGE"
            self.failure_reasons.append(f"{state.instance_id}:{state.reason}")
            return
        state.residual_reject_count = q
        if len(state.u_ids) > 4 * state.dummy_count:
            rng = SplitMix64(_derive_seed(self.algo_seed, f"{state.instance_id}:R3_RANDOM_U"))
            state.labels = {item_id: False for item_id in low_reject}
            state.labels.update({item_id: True for item_id in high_accept})
            for item_id in state.u_ids:
                state.labels[item_id] = bool(rng.bit())
            state.random_branch = "A7_U_GREATER_THAN_4D_RANDOM_LABELS"
            state.status = "PAPER_RANDOM_FAILURE_PATH"
            state.reason = state.random_branch
            self.failure_reasons.append(f"{state.instance_id}:{state.reason}")
            return
        if q == 0:
            state.labels = {item_id: False for item_id in low_reject}
            state.labels.update({item_id: True for item_id in high_accept})
            state.labels.update({item_id: True for item_id in state.u_ids})
            state.status = "SUCCESS"
            state.reason = "A7_EXACT_EMPTY_RESIDUAL_REJECT_PREFIX"
            return
        if q == len(state.u_ids):
            state.labels = {item_id: False for item_id in low_reject}
            state.labels.update({item_id: True for item_id in high_accept})
            state.labels.update({item_id: False for item_id in state.u_ids})
            state.status = "SUCCESS"
            state.reason = "A7_EXACT_FULL_RESIDUAL_REJECT_PREFIX"
            return
        state.status = "READY_R3"

    def _stage_r3(self) -> None:
        tasks: list[dict[str, Any]] = []
        edges: list[Edge] = []
        active: list[A7State] = []
        for state in self.a7.values():
            if state.status == "PENDING":
                self._a7_after_r2(state)
            if state.status != "READY_R3":
                continue
            u = len(state.u_ids)
            if u == 0:
                state.status = "UNDEFINED/INVALID_FINITE_CASE"
                state.reason = "A7_ZERO_U_WITH_INTERIOR_TARGET"
                self.failure_reasons.append(f"{state.instance_id}:{state.reason}")
                continue
            # PROJECT_DERIVED finite/sampling rule. The constant 32 keeps the
            # R3 edge budget linear and raises the bracketing exponent while
            # retaining the paper's m/u scale.
            if u * u <= R3_SAMPLE_MULTIPLIER * state.m:
                state.v_ids = list(state.u_ids)
                state.exact_via_all_pairs = True
                phase = "A7_R3_ALL_U_PAIRS"
                task_edges = [
                    Edge(state.instance_id, a, b, 3, 2, "PROJECT_DERIVED_R3_ALL_PAIRS")
                    for a, b in itertools.combinations(state.u_ids, 2)
                ]
            else:
                vsize = min(
                    u,
                    (R3_SAMPLE_MULTIPLIER * state.m + u - 1) // u,
                )
                sample_seed = _derive_seed(self.algo_seed, f"{state.instance_id}:R3_V")
                state.v_ids = SplitMix64(sample_seed).sample(state.u_ids, vsize)
                phase = "A7_R3_SAMPLE_CROSS"
                seen: set[tuple[str, str]] = set()
                task_edges = []
                for v_id in state.v_ids:
                    for u_id in state.u_ids:
                        if v_id == u_id:
                            continue
                        pair = _normalized_pair(v_id, u_id)
                        if pair in seen:
                            continue
                        seen.add(pair)
                        task_edges.append(Edge(state.instance_id, v_id, u_id, 3, 2, "A7_V_x_U"))
            tasks.append({
                "task": state.instance_id,
                "phase": phase,
                "input_ids": state.input_ids,
                "U_ids": state.u_ids,
                "V_ids": state.v_ids,
                "m": state.m,
                "q_reject_in_U_1_based_count": state.residual_reject_count,
                "sample_seed": None if state.exact_via_all_pairs else sample_seed,
                "requires_results_through_round": 2,
            })
            edges.extend(task_edges)
            active.append(state)
        self._freeze_and_execute(3, tasks, edges)
        for state in active:
            q = int(state.residual_reject_count)
            ranks: dict[str, int] = {}
            for v_id in state.v_ids:
                lower_count = 0
                for u_id in state.u_ids:
                    if u_id != v_id and self._less_from_round(3, state.instance_id, u_id, v_id):
                        lower_count += 1
                ranks[v_id] = lower_count + 1
            if state.exact_via_all_pairs:
                if any(item_id not in ranks for item_id in state.u_ids):
                    state.status = "UNDEFINED/INVALID_FINITE_CASE"
                    state.reason = "R3_ALL_PAIRS_RANK_MISSING"
                    self.failure_reasons.append(f"{state.instance_id}:{state.reason}")
                    continue
                state.labels = {item_id: False for item_id in state.r1_ids}
                state.labels.update({item_id: True for item_id in state.a2_ids})
                for item_id in state.u_ids:
                    state.labels[item_id] = ranks[item_id] > q
                state.status = "SUCCESS"
                state.reason = "PROJECT_DERIVED_R3_ALL_PAIRS_EXACT_RESIDUAL_ORDER"
                continue
            lower_sample = [item_id for item_id, rank in ranks.items() if rank <= q]
            upper_sample = [item_id for item_id, rank in ranks.items() if rank > q]
            if not lower_sample or not upper_sample:
                state.status = "UNDEFINED/INVALID_FINITE_CASE"
                state.reason = "A7_R3_MISSING_X_OR_Y_SAMPLE_BRACKET"
                self.failure_reasons.append(f"{state.instance_id}:{state.reason}")
                continue
            # R3's rank target is q=h-|R1_real|. x/y are the nearest sampled
            # endpoints around the q / q+1 cut, and both remain in W.
            state.x_id = max(lower_sample, key=lambda item_id: ranks[item_id])
            state.y_id = min(upper_sample, key=lambda item_id: ranks[item_id])
            xrank, yrank = ranks[state.x_id], ranks[state.y_id]
            if not xrank <= q < yrank:
                state.status = "UNDEFINED/INVALID_FINITE_CASE"
                state.reason = "A7_R3_PIVOT_RANKS_DO_NOT_BRACKET_RESIDUAL_CUT"
                self.failure_reasons.append(f"{state.instance_id}:{state.reason}")
                continue
            below_x = {
                item_id for item_id in state.u_ids
                if item_id != state.x_id and self._less_from_round(3, state.instance_id, item_id, state.x_id)
            }
            above_y = {
                item_id for item_id in state.u_ids
                if item_id != state.y_id and self._less_from_round(3, state.instance_id, state.y_id, item_id)
            }
            state.w_ids = [item_id for item_id in state.u_ids if item_id not in below_x | above_y]
            state.labels = {item_id: False for item_id in state.r1_ids | below_x}
            state.labels.update({item_id: True for item_id in state.a2_ids | above_y})
            if state.w_ids:
                state.labels.update({item_id: False for item_id in state.w_ids})
            B = _floor_power(state.m, 5, 12)
            if len(state.w_ids) > B:
                rng = SplitMix64(_derive_seed(self.algo_seed, f"{state.instance_id}:R3_RANDOM_W"))
                for item_id in state.w_ids:
                    state.labels[item_id] = bool(rng.bit())
                state.random_branch = "A7_W_GREATER_THAN_M_POWER_5_OVER_12_RANDOM_LABELS"
                state.status = "PAPER_RANDOM_FAILURE_PATH"
                state.reason = state.random_branch
                self.failure_reasons.append(f"{state.instance_id}:{state.reason}")
                continue
            state.status = "READY_R4"

    def _stage_r4(self) -> None:
        tasks: list[dict[str, Any]] = []
        edges: list[Edge] = []
        active: list[A7State] = []
        for state in self.a7.values():
            if state.status != "READY_R4":
                continue
            tasks.append({
                "task": state.instance_id,
                "phase": "A7_R4_ALL_W_PAIRS",
                "input_ids": state.input_ids,
                "U_ids": state.u_ids,
                "W_ids": state.w_ids,
                "x_id": state.x_id,
                "y_id": state.y_id,
                "R1_ids": sorted(state.r1_ids),
                "Rstar_count_before_R4": len(state.r1_ids) + sum(
                    1 for item_id in state.u_ids
                    if item_id not in state.w_ids and not state.labels[item_id]
                ),
                "requires_results_through_round": 3,
            })
            edges.extend(
                Edge(state.instance_id, a, b, 4, 3, "A7_all_unordered_W_pairs")
                for a, b in itertools.combinations(state.w_ids, 2)
            )
            active.append(state)
        self._freeze_and_execute(4, tasks, edges)
        for state in active:
            # W's placeholder False values above are not prior rejects.
            already_rejected = sum(
                1 for item_id in state.input_ids
                if item_id not in set(state.w_ids) and not state.labels[item_id]
            )
            q_w = state.half - already_rejected
            state.rstar_count = already_rejected
            if not 1 <= q_w <= len(state.w_ids):
                state.status = "UNDEFINED/INVALID_FINITE_CASE"
                state.reason = "A7_R4_TARGET_RANK_OUTSIDE_W"
                self.failure_reasons.append(f"{state.instance_id}:{state.reason}")
                continue
            lower_counts: dict[str, int] = {}
            for item_id in state.w_ids:
                lower_counts[item_id] = sum(
                    1 for other in state.w_ids
                    if other != item_id and self._less_from_round(4, state.instance_id, other, item_id)
                )
            z_candidates = [
                item_id for item_id, count in lower_counts.items() if count == q_w - 1
            ]
            if len(z_candidates) != 1:
                state.status = "UNDEFINED/INVALID_FINITE_CASE"
                state.reason = "A7_R4_STRICT_ORDER_DID_NOT_IDENTIFY_UNIQUE_Z"
                self.failure_reasons.append(f"{state.instance_id}:{state.reason}")
                continue
            z = z_candidates[0]
            # PROJECT_DERIVED: z itself is rejected to make exactly h rejects.
            for item_id in state.w_ids:
                state.labels[item_id] = self._less_from_round(4, state.instance_id, z, item_id)
            state.labels[z] = False
            if sum(not value for value in state.labels.values()) != state.half:
                state.status = "COMPLETED_WRONG"
                state.reason = "A7_R4_PROJECTED_BOUNDARY_DID_NOT_PRESERVE_H_REJECTS"
                self.failure_reasons.append(f"{state.instance_id}:{state.reason}")
                continue
            state.status = "SUCCESS"
            state.reason = "A7_R4_EXACT_RANK_WITH_Z_ASSIGNED_TO_REJECT"

    def _count_round_metrics(self) -> None:
        seen_global: set[tuple[str, str]] = set()
        total_repeats = 0
        for round_no in range(1, 5):
            edges = self.round_edges[round_no]
            prior_round_edges = set(seen_global)
            rr = sum(self.items[e.left].kind == "real" and self.items[e.right].kind == "real" for e in edges)
            dummy = len(edges) - rr
            per_round_seen: set[tuple[str, str]] = set()
            per_round_owners: dict[tuple[str, str], set[str]] = {}
            duplicate_in_round = 0
            repeated_prior = 0
            cross_task_repeats = 0
            repeated_total_round = 0
            for edge in edges:
                pair = _normalized_pair(edge.left, edge.right)
                if pair in seen_global:
                    repeated_total_round += 1
                if pair in per_round_seen:
                    duplicate_in_round += 1
                    if edge.task not in per_round_owners[pair]:
                        cross_task_repeats += 1
                elif pair in prior_round_edges:
                    repeated_prior += 1
                per_round_seen.add(pair)
                per_round_owners.setdefault(pair, set()).add(edge.task)
                seen_global.add(pair)
            total_repeats += repeated_total_round
            self.round_metrics[round_no] = {
                "comparison_calls": len(edges),
                "real_real_calls": rr,
                "dummy_related_calls": dummy,
                "duplicate_calls_same_round": duplicate_in_round,
                "cross_task_repeated_calls_same_round": cross_task_repeats,
                "repeated_calls_from_prior_rounds": repeated_prior,
                "unique_unordered_edges_this_round": len(per_round_seen),
            }
        self.round_metrics[0] = {
            "total_comparison_calls": sum(self.round_metrics[r]["comparison_calls"] for r in range(1, 5)),
            "unique_unordered_edges_all_rounds": len(seen_global),
            "repeated_comparison_calls": total_repeats,
        }

    def run(self) -> dict[str, Any]:
        if self.n == 0 or not 1 <= self.k <= self.n:
            return {"label": LABEL, "status": "UNDEFINED/INVALID_FINITE_CASE", "reason": "INVALID_INPUT_OR_K"}
        if any(not INT32_MIN <= score <= INT32_MAX for score in self.scores):
            return {"label": LABEL, "status": "UNDEFINED/INVALID_FINITE_CASE", "reason": "SCORE_OUTSIDE_SIGNED_INT32"}
        try:
            low_a7, high_a7 = self._build_reduction_inputs()
        except ValueError as exc:
            return {"label": LABEL, "status": "UNDEFINED/INVALID_FINITE_CASE", "reason": str(exc)}
        self._stage_r1()
        self._stage_r2()
        self._stage_r3()
        self._stage_r4()
        self._count_round_metrics()
        if all(set(state.labels) == set(state.input_ids) for state in self.a7.values()):
            low_accept = {
                item_id for item_id, bit in self.a7["SELECT_LOW_SENTINEL"].labels.items() if bit
            }
            high_reject = {
                item_id for item_id, bit in self.a7["SELECT_HIGH_SENTINEL"].labels.items() if not bit
            }
            self.candidate_ids = sorted((low_accept & high_reject) & {
                f"real:{i}" for i in range(self.n)
            })
            if len(self.candidate_ids) == 1:
                self.selected_original_index = self.items[self.candidate_ids[0]].original_index
        if any(s.status == "COMPLETED_WRONG" for s in self.a7.values()):
            self.final_status = "COMPLETED_WRONG"
        elif any(s.status == "UNDEFINED/INVALID_FINITE_CASE" for s in self.a7.values()):
            self.final_status = "UNDEFINED/INVALID_FINITE_CASE"
        elif any(s.status == "PAPER_RANDOM_FAILURE_PATH" for s in self.a7.values()):
            self.final_status = "PAPER_RANDOM_FAILURE_PATH"
        else:
            if len(self.candidate_ids) != 1:
                self.final_status = "UNDEFINED/INVALID_FINITE_CASE"
                self.failure_reasons.append("TWO_SENTINEL_INTERSECTION_NOT_A_UNIQUE_REAL_ITEM")
            else:
                self.final_status = "SUCCESS"
        self._count_round_metrics()
        summary = self.summary()
        self.events.insert(0, {
            "event": "TRACE_HEADER",
            "trace_version": 1,
            "label": LABEL,
            "input_scores": self.scores,
            "K": self.k,
            "input_seed": self.input_seed,
            "algorithm_seed": self.algo_seed,
            "epsilon": "1/36",
            "epsilon_A5_analysis": "1/72",
            "effective_expansion": self.expansion,
            "items": [item.trace_record() for item in self.items.values()],
            "summary": summary,
        })
        return summary

    def summary(self) -> dict[str, Any]:
        return {
            "label": LABEL,
            "n": self.n,
            "K": self.k,
            "status": self.final_status,
            "selected_original_index": self.selected_original_index,
            "candidate_ids": list(self.candidate_ids),
            "input_seed": self.input_seed,
            "algorithm_seed": self.algo_seed,
            "epsilon": "1/36",
            "effective_expansion": self.expansion,
            "round_metrics": self.round_metrics,
            "partitions": {
                key: {
                    "status": value.status,
                    "reason": value.reason,
                    "m": value.m,
                    "h": value.half,
                    "D": value.dummy_count,
                    "U_size": len(value.u_ids),
                    "V_size": len(value.v_ids),
                    "W_size": len(value.w_ids),
                    "residual_reject_count_in_U": value.residual_reject_count,
                    "Rstar_real_input_count_before_R4": value.rstar_count,
                    "x_id": value.x_id,
                    "y_id": value.y_id,
                    "random_branch": value.random_branch,
                    "rejected_count": sum(not bit for bit in value.labels.values()) if value.labels else None,
                }
                for key, value in self.a7.items()
            },
            "A5": {
                key: {
                    "N": len(value.input_ids),
                    "S1_size": len(value.sample_ids),
                    "T1_size": len(value.pivot_sample_ids),
                    "target_lower_count_in_S1": value.target_lower_count,
                    "sample_seed": value.sample_seed,
                    "pivot_a": value.pivot_a,
                    "pivot_b": value.pivot_b,
                    "pivot_x": value.pivot_x,
                    "failure_reason": value.reason,
                }
                for key, value in self.a5.items()
            },
            "failure_reasons": list(self.failure_reasons),
            "oracle_validation": dict(self.oracle_validation),
            "comparison_rounds": 4,
            "protocol_I_full_online_rounds": "NOT_REVIEWED",
        }

    def trace(self) -> dict[str, Any]:
        return {
            "trace_version": 1,
            "events": self.events,
            "summary": self.summary(),
        }


def run_select(scores: list[int], k: int, algo_seed: int, input_seed: int | None = None) -> tuple[dict[str, Any], dict[str, Any]]:
    run = Select4R(scores, k, algo_seed, input_seed)
    summary = run.run()
    return summary, run.trace()


def generated_scores(n: int, input_seed: int, mode: str = "random") -> list[int]:
    rng = SplitMix64(input_seed)
    if n < 0:
        raise ValueError("n must be non-negative")
    if mode == "all_equal":
        return [0] * n
    if mode == "ties":
        return [int(rng.randbelow(9)) - 4 for _ in range(n)]
    if mode == "boundaries":
        fixed = [INT32_MIN, INT32_MAX, -1, 0, 1, INT32_MIN + 1, INT32_MAX - 1]
        return [fixed[i % len(fixed)] for i in range(n)]
    if mode == "random":
        return [((rng.next_u64() >> 32) & 0xFFFFFFFF) - (1 << 31) for _ in range(n)]
    raise ValueError(f"unsupported input mode {mode}")


def summary_json(summary: dict[str, Any]) -> str:
    return json.dumps(summary, sort_keys=True, separators=(",", ":"))
