#!/usr/bin/env python3
"""One P0/P1 endpoint for the isolated S8 three-process experiment.

The C++ child evaluates only this party's offline FSS keys and local shuffle
state. The peer link carries public comparison results and shuffle messages.
This module never reads the other party's score shares or a clear permutation.
"""
from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import os
import secrets
import socket
import struct
import subprocess
import sys
import time
from collections import defaultdict
from pathlib import Path
from typing import Any

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "experiments" / "TEST_ONLY_BMW16_S7_COMPOSITION"))
from select4r_s4_source import Context  # noqa: E402

MASK64 = (1 << 64) - 1


class CryptoSampleRNG:
    """TEST_ONLY SHA-256 counter PRF stream with unbiased sampling."""

    def __init__(self, seed: bytes, domain: str):
        self.key = hashlib.sha256(b"BMW16-S9-SAMPLE-v1\0" + seed + domain.encode("ascii")).digest()
        self.counter = 0
        self.buffer = bytearray()

    def _bytes(self, count: int) -> bytes:
        while len(self.buffer) < count:
            block = hmac.new(self.key, self.counter.to_bytes(16, "big"), hashlib.sha256).digest()
            self.counter += 1
            self.buffer.extend(block)
        result = bytes(self.buffer[:count])
        del self.buffer[:count]
        return result

    def getrandbits(self, bits: int) -> int:
        if bits < 0:
            raise ValueError("number of bits must be non-negative")
        if bits == 0:
            return 0
        count = (bits + 7) // 8
        return int.from_bytes(self._bytes(count), "big") & ((1 << bits) - 1)

    def randbelow(self, upper: int) -> int:
        if upper <= 0:
            raise ValueError("empty sampling range")
        bits = upper.bit_length()
        while True:
            candidate = self.getrandbits(bits)
            if candidate < upper:
                return candidate

    def sample(self, population: list[Any], count: int) -> list[Any]:
        if count < 0 or count > len(population):
            raise ValueError("sample larger than population")
        pool = list(population)
        selected: list[Any] = []
        for _ in range(count):
            selected.append(pool.pop(self.randbelow(len(pool))))
        return selected


class Peer:
    def __init__(self, party: int, port: int, timeout: float = 20.0):
        self.sock: socket.socket | None = None
        deadline = time.monotonic() + timeout
        if party == 0:
            server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server.bind(("127.0.0.1", port))
            server.listen(1)
            server.settimeout(timeout)
            self.sock, _ = server.accept()
            server.close()
        else:
            last: Exception | None = None
            while time.monotonic() < deadline:
                try:
                    s = socket.create_connection(("127.0.0.1", port), timeout=0.5)
                    self.sock = s
                    break
                except OSError as exc:
                    last = exc
                    time.sleep(0.05)
            if self.sock is None:
                raise TimeoutError(f"peer connect failed: {last}")
        self.sock.settimeout(timeout)
        self.sent_bytes = 0
        self.received_bytes = 0

    def send(self, value: dict[str, Any]) -> None:
        assert self.sock is not None
        payload = json.dumps(value, separators=(",", ":"), sort_keys=True).encode("ascii")
        frame = struct.pack("!I", len(payload)) + payload
        self.sock.sendall(frame)
        self.sent_bytes += len(frame)

    def recv(self) -> dict[str, Any]:
        assert self.sock is not None
        size = struct.unpack("!I", self._exact(4))[0]
        if size > 64 * 1024 * 1024:
            raise ValueError("peer frame too large")
        payload = self._exact(size)
        self.received_bytes += size + 4
        value = json.loads(payload.decode("ascii"))
        if value.get("kind") == "abort":
            raise RuntimeError(f"peer abort:{value.get('class', 'UNKNOWN')}")
        return value

    def _exact(self, n: int) -> bytes:
        assert self.sock is not None
        parts = bytearray()
        while len(parts) < n:
            chunk = self.sock.recv(n - len(parts))
            if not chunk:
                raise ConnectionError("peer closed channel")
            parts.extend(chunk)
        return bytes(parts)

    def exchange(self, value: dict[str, Any]) -> dict[str, Any]:
        self.send(value)
        return self.recv()

    def close(self) -> None:
        if self.sock is not None:
            self.sock.close()
            self.sock = None


def linux_path(path: str) -> str:
    if os.name != "nt":
        return path
    absolute = Path(path).resolve()
    drive = absolute.drive.rstrip(":").lower()
    if len(drive) != 1:
        raise ValueError("WSL bridge only supports drive-letter paths")
    return "/mnt/" + drive + "/" + "/".join(absolute.parts[1:])


class Worker:
    def __init__(self, binary: str, party: int, package: str):
        if binary.startswith("wsl:"):
            command = ["wsl.exe", "-d", "Ubuntu-24.04", "--", binary[4:], "--party",
                       str(party), linux_path(package)]
        else:
            command = [binary, "--party", str(party), package]
        self.proc = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                     stderr=subprocess.PIPE, text=True, encoding="ascii",
                                     errors="replace", bufsize=1)

    def call(self, command: str, lines: int = 1) -> list[list[str]]:
        assert self.proc.stdin is not None and self.proc.stdout is not None
        self.proc.stdin.write(command + "\n")
        self.proc.stdin.flush()
        out = []
        for _ in range(lines):
            line = self.proc.stdout.readline()
            if not line:
                err = self.proc.stderr.read()[-1000:] if self.proc.stderr else ""
                raise RuntimeError(f"FSS worker exited: {err}")
            out.append(line.split())
        return out

    def close(self) -> None:
        if self.proc.poll() is None:
            self.call("QUIT")
        code = self.proc.wait(timeout=10)
        err = self.proc.stderr.read() if self.proc.stderr else ""
        if code != 0:
            raise RuntimeError(f"FSS worker exit={code}: {err[-1000:]}")


def encode_records(records: list[list[int]]) -> str:
    return " ".join(str(int(v)) for rec in records for v in rec)


def parse_records(row: list[str], tag: str, n: int) -> list[list[int]]:
    if len(row) != 2 + 3 * n or row[0] != tag or int(row[1]) != n:
        raise ValueError(f"bad {tag} record vector")
    values = [int(v) for v in row[2:]]
    return [values[3*i:3*i+3] for i in range(n)]


def public_less(left: Any, right: Any) -> bool:
    if left.category != right.category:
        return left.category < right.category
    if left.category == 2:
        raise AssertionError("real-real comparison must use offline uCMP")
    return left.serial < right.serial


class PartyComparator:
    def __init__(self, party: int, worker: Worker, peer: Peer, z: list[int], n: int,
                 low_pad: int, bits: int):
        self.party, self.worker, self.peer = party, worker, peer
        self.z, self.n, self.low_pad, self.bits = z, n, low_pad, bits
        self.round_logical: dict[int, int] = defaultdict(int)
        self.round_logical_calls = self.round_logical
        self.round_slots: dict[int, int] = defaultdict(int)
        self.round_dummy: dict[int, int] = defaultdict(int)
        self.round_dup: dict[int, int] = defaultdict(int)
        self.round_reverse_complements: dict[int, int] = defaultdict(int)
        self.membership_slots = 0
        self.net_rounds = 0

    def compare_batches(self, round_no: int, batches: list[dict[str, Any]], items: list[Any],
                        n: int, k: int, low_pad: int) -> np.ndarray:
        edges = [[str(b["descriptor"]["task"]), int(a), int(c)]
                 for b in batches for a, c in zip(b["left"], b["right"])]
        digest = hashlib.sha256(json.dumps(edges, separators=(",", ":")).encode()).hexdigest()
        remote = self.peer.exchange({"kind": "plan", "round": round_no,
                                     "edges": edges, "sha256": digest})
        self.net_rounds += 1
        if remote != {"kind": "plan", "round": round_no, "edges": edges, "sha256": digest}:
            raise RuntimeError("peer adaptive graph differs")

        local: list[int] = []
        cache: dict[tuple[int, int, int], int] = {}
        dummies = 0
        calls = 0
        slots = 0
        ring = (1 << self.bits) - 1
        for batch in batches:
            task = str(batch["descriptor"]["task"])
            task_id = 0 if task == "SELECT_LOW_SENTINEL" else 1
            for a_raw, b_raw in zip(batch["left"], batch["right"]):
                a, b = int(a_raw), int(b_raw)
                ia, ib = items[a], items[b]
                calls += 1
                if ia.category != 2 or ib.category != 2:
                    bit = int(public_less(ia, ib))
                    local.append(bit if self.party == 0 else 0)
                    dummies += 1
                    continue
                lo, hi = sorted((a, b))
                cache_key = (task_id, lo, hi)
                if cache_key not in cache:
                    zlo = (self.z[lo] + self.low_pad + 1) & ring
                    zhi = (self.z[hi] + self.low_pad + 1) & ring
                    row = self.worker.call(f"CMP {round_no} {task_id} {lo} {hi} {zlo} {zhi}")[0]
                    if len(row) != 2 or row[0] != "C":
                        raise ValueError("bad uCMP result")
                    share = int(row[1]) & MASK64
                    cache[cache_key] = share
                    slots += 1
                share = cache[cache_key]
                if a != lo:
                    share = ((1 - share) if self.party == 0 else (-share)) & MASK64
                    self.round_reverse_complements[round_no] += 1
                local.append(share)
        remote = self.peer.exchange({"kind": "shares", "round": round_no, "shares": local})
        self.net_rounds += 1
        if remote.get("kind") != "shares" or remote.get("round") != round_no or len(remote.get("shares", [])) != len(local):
            raise RuntimeError("peer comparison share message mismatch")
        outcomes = []
        for a, b in zip(local, remote["shares"]):
            value = (int(a) + int(b)) & MASK64
            if value not in (0, 1):
                raise ValueError("uCMP shares did not open to a bit")
            outcomes.append(bool(value))
        self.round_logical[round_no] = self.round_logical.get(round_no, 0) + calls
        self.round_slots[round_no] = self.round_slots.get(round_no, 0) + slots
        self.round_dummy[round_no] = self.round_dummy.get(round_no, 0) + dummies
        self.round_dup[round_no] = self.round_dup.get(round_no, 0) + calls - slots - dummies
        return np.asarray(outcomes, dtype=np.bool_)

    def membership(self, selected: int) -> list[int]:
        shares: list[int] = []
        ring = (1 << self.bits) - 1
        for i in range(self.n):
            if i == selected:
                shares.append(1 if self.party == 0 else 0)
                continue
            lo, hi = sorted((i, selected))
            row = self.worker.call(f"MEM {lo} {hi} {self.z[lo]} {self.z[hi]}")[0]
            if len(row) != 2 or row[0] != "C":
                raise ValueError("bad membership DCF result")
            c = int(row[1]) & MASK64
            # Stored key is [R_lo < R_hi]; membership is [R_i >= R_selected].
            if i == lo:
                c = ((1 - c) if self.party == 0 else (-c)) & MASK64
            shares.append(c & 1)
            self.membership_slots += 1
        return shares


def run_party(args: argparse.Namespace) -> dict[str, Any]:
    online_started = time.perf_counter()
    source = json.loads(Path(args.input).read_text(encoding="utf-8"))
    party, n, k = args.party, int(source["n"]), int(source["k"])
    shares = [int(v) for v in source["shares"]]
    if len(shares) != n or not 1 <= k <= n:
        raise ValueError("invalid local input shape")
    b = max(1, (n - 1).bit_length())
    raw_score_abi = bool(args.raw_score_abi)
    label_suffix = ("EXPERIMENTAL_TEST_ONLY_BMW16_S9_RAW_ABI_CSPRNG"
                    if (raw_score_abi or args.joint_csprng_coins or
                        args.test_only_force_algorithm_abort or args.test_only_probe_peer_files)
                    else "EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS")
    implementation_label = f"Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / {label_suffix}"
    bits = (33 if raw_score_abi else 34) + b
    ring_raw = (1 << 34) - 1
    ring_key = (1 << bits) - 1
    records: list[list[int]] = []
    if raw_score_abi:
        if any(value < 0 or value > 0xFFFFFFFF for value in shares):
            raise ValueError("raw score ABI share must be an unsigned 32-bit word")
    else:
        for i, value in enumerate(shares):
            ordered_share = (value + ((1 << 31) if party == 0 else 0)) & ring_raw
            key_share = ((ordered_share << b) + ((n - 1 - i) if party == 0 else 0)) & ring_key
            records.append([key_share, 0, 0])

    peer_file_probe: dict[str, Any] | None = None
    if args.test_only_probe_peer_files and n >= 2:
        # Deliberately test the actual same-OS-token boundary used by this
        # loopback harness. The probe reads and discards bytes; it never emits
        # another party's shares or key material into the evidence file.
        peer = 1 - party
        peer_input = Path(args.input).with_name(f"p{peer}.input.json")
        peer_package = Path(args.package).with_name(f"p{peer}.offline")
        def read_probe(path: Path) -> dict[str, Any]:
            try:
                with path.open("rb") as handle:
                    count = 0
                    while True:
                        block = handle.read(1024 * 1024)
                        if not block:
                            break
                        count += len(block)
                return {"can_read": True, "bytes_read_and_discarded": count}
            except OSError as exc:
                return {"can_read": False, "error_class": type(exc).__name__}
        peer_file_probe = {
            "scope": "TEST_ONLY same OS identity/token; not a remote-host claim",
            "peer_input_share_file": read_probe(peer_input),
            "peer_offline_material_file": read_probe(peer_package),
        }
        if raw_score_abi and peer_file_probe["peer_input_share_file"].get("can_read"):
            peer_source = json.loads(peer_input.read_text(encoding="utf-8"))
            peer_shares = [int(value) for value in peer_source.get("shares", [])]
            if peer_source.get("n") == n and len(peer_shares) == n:
                # This intentionally models a corrupted party exploiting the
                # shared OS token. Only a Boolean/count is recorded.
                recovered = [((shares[i] + peer_shares[i]) & 0xFFFFFFFF) for i in range(n)]
                signed = [value - (1 << 32) if value & (1 << 31) else value for value in recovered]
                peer_file_probe["clear_score_reconstruction"] = {
                    "possible_from_sibling_share_file": True,
                    "recovered_count_not_values": len(signed),
                    "all_values_signed_int32": all(-(1 << 31) <= value < (1 << 31) for value in signed),
                }

    peer: Peer | None = None
    worker: Worker | None = None
    effective_algorithm_seed = args.algorithm_seed
    randomness_record: dict[str, Any] = {
        "mode": "FIXED_TEST_SEED",
        "source": "Python Random seed; reproducible only, not a cryptographic sampling claim",
        "joint_coin_message_phases": 0,
        "domain_separation": "S4-derived per-task sample domains",
    }
    try:
        peer = Peer(party, args.port, args.peer_timeout)
        if args.joint_csprng_coins and n >= 2:
            local_nonce = secrets.token_bytes(32)
            remote = peer.exchange({"kind": "select_joint_coin", "party": party,
                                    "session": int(source.get("session", 0)),
                                    "nonce": local_nonce.hex()})
            if (remote.get("kind") != "select_joint_coin" or remote.get("party") != 1 - party or
                    remote.get("session") != int(source.get("session", 0))):
                raise ValueError("joint algorithm coin binding")
            remote_nonce = bytes.fromhex(str(remote.get("nonce", "")))
            if len(remote_nonce) != 32:
                raise ValueError("joint algorithm coin width")
            pair = [local_nonce, remote_nonce] if party == 0 else [remote_nonce, local_nonce]
            xor_seed = bytes(a ^ b for a, b in zip(pair[0], pair[1]))
            seed = hashlib.sha256(b"BMW16-S9-JOINT-COIN-v1\0" + xor_seed).digest()
            effective_algorithm_seed = int.from_bytes(seed, "big")
            randomness_record = {
                "mode": "JOINT_CSPRNG_XOR_SHA256_HMAC_DRBG_TEST_ONLY",
                "source": "two 256-bit secrets.token_bytes contributions; uniform if either contribution is uniform",
                "joint_coin_message_phases": 1,
                "seed_sha256": hashlib.sha256(seed).hexdigest(),
                "domain_separation": "BMW16-S9-SAMPLE-v1 + task and S4 R1/R3 sample-domain string",
                "sampling": "HMAC-SHA256 counter stream; rejection randbelow; partial Fisher-Yates without replacement",
                "adversarial_scope": "semi-honest only; no commit/reveal bias resistance or authenticated channel",
            }
        z: list[int]
        if n == 1:
            z = [0]
        else:
            worker = Worker(args.backend, party, args.package)
            if raw_score_abi:
                padded_n = 1 << b
                carry_row = worker.call(f"SCORE_CARRY_PREP {n} " + " ".join(map(str, shares)))[0]
                if len(carry_row) != 2 + 2 * padded_n or carry_row[0] != "S":
                    raise ValueError("bad score carry prep batch")
                local_carry = [int(value) for value in carry_row[2:]]
                remote = peer.exchange({"kind": "score_carry", "values": local_carry})
                if remote.get("kind") != "score_carry" or len(remote.get("values", [])) != 2 * padded_n:
                    raise ValueError("bad peer score carry batch")
                carry_values = [((int(a) + int(b_)) & ((1 << 34) - 1))
                                for a, b_ in zip(local_carry, remote["values"])]
                carry_eval = worker.call("SCORE_CARRY_EVAL " + str(len(carry_values)) + " " +
                                         " ".join(map(str, carry_values)))[0]
                if carry_eval != ["Q", "OK"]:
                    raise ValueError("bad score carry evaluation")
                sign_row = worker.call("SCORE_SIGN_PREP")[0]
                if len(sign_row) != 2 + 2 * padded_n or sign_row[0] != "S":
                    raise ValueError("bad score sign prep batch")
                local_sign = [int(value) for value in sign_row[2:]]
                remote = peer.exchange({"kind": "score_sign", "values": local_sign})
                if remote.get("kind") != "score_sign" or len(remote.get("values", [])) != 2 * padded_n:
                    raise ValueError("bad peer score sign batch")
                sign_values = [((int(a) + int(b_)) & ((1 << 34) - 1))
                               for a, b_ in zip(local_sign, remote["values"])]
                key_row = worker.call("SCORE_SIGN_EVAL " + str(len(sign_values)) + " " +
                                      " ".join(map(str, sign_values)))[0]
                if len(key_row) != n + 2 or key_row[0] != "K" or int(key_row[1]) != n:
                    raise ValueError("bad stable-key output batch")
                records = [[int(value), 0, 0] for value in key_row[2:]]
            row = worker.call(f"FWD1 {n} {encode_records(records)}")[0]
            local1 = parse_records(row, "R", n)
            remote = peer.exchange({"kind": "shuffle_r1", "records": local1})
            if remote.get("kind") != "shuffle_r1" or len(remote.get("records", [])) != n:
                raise RuntimeError("bad peer shuffle R1")
            row = worker.call(f"FWD2 {n} {encode_records(remote['records'])}")[0]
            local2 = parse_records(row, "R", n)
            remote = peer.exchange({"kind": "shuffle_r2", "records": local2})
            if remote.get("kind") != "shuffle_r2" or len(remote.get("records", [])) != n:
                raise RuntimeError("bad peer shuffle R2")
            result_rows = worker.call(f"FWD3 {n} {encode_records(remote['records'])}", lines=2)
            shuffled = parse_records(result_rows[0], "F", n)
            public = parse_records(result_rows[1], "Z", n)
            z = [int(record[0]) for record in public]

        assert peer is not None
        comparator = PartyComparator(party, worker, peer, z, n,
                                     max(n + 1 - 2 * (n - k + 1), 0), bits) if worker else None
        if comparator is None:
            class DirectBackend:
                round_slots: dict[int, int] = defaultdict(int)
                round_reverse_complements: dict[int, int] = defaultdict(int)
                round_logical_calls: dict[int, int] = defaultdict(int)
                def compare_batches(self, _r: int, batches: list[dict[str, Any]], items: list[Any], *_: Any) -> np.ndarray:
                    out = []
                    for batch in batches:
                        for a, b_ in zip(batch["left"], batch["right"]):
                            out.append(public_less(items[int(a)], items[int(b_)]))
                    return np.asarray(out, dtype=np.bool_)
            backend: Any = DirectBackend()
        else:
            backend = comparator
        context = Context([0] * n, k, args.input_seed, effective_algorithm_seed,
                          True, list(range(n)), backend)
        if args.joint_csprng_coins and n >= 2:
            domains = []
            for partition in context.partitions:
                task = str(partition["task"])
                for suffix in ("R1_SAMPLE", "R3_SAMPLE"):
                    domain = f"{task}:{suffix}"
                    context.rngs[domain] = CryptoSampleRNG(seed, domain)
                    domains.append(domain)
            randomness_record["sample_domains"] = domains
        selected_result = context.run()
        raw_status = str(selected_result["status"])
        forced_abort = bool(args.test_only_force_algorithm_abort)
        if forced_abort:
            # Contract fixture only: inject the same algorithm-abort class
            # after Select control-flow execution and before membership/
            # inverse routing. It does not claim to trigger a paper failure.
            raw_status = "PROJECT_RANDOM_FAILURE_PATH"
        status = ("ABORT_ALGORITHM_PROBABILITY" if raw_status == "PROJECT_RANDOM_FAILURE_PATH" else
                  "ABORT_ALGORITHM_INVALID" if raw_status == "COMPLETED_WRONG" else raw_status)
        selected = None if forced_abort else selected_result.get("selected_original_index")
        round_metrics = selected_result["round_metrics"]
        mask_share: list[int] | None = None
        if status == "SUCCESS":
            if selected is None or not 0 <= int(selected) < n:
                raise ValueError("selected anonymous handle invalid")
            if n == 1:
                mask_share = [1 if party == 0 else 0]
            else:
                assert comparator is not None and worker is not None
                membership = comparator.membership(int(selected))
                inv_records = [[int(v) & ring_key, 0, 0] for v in membership]
                row = worker.call(f"INV1 {n} {encode_records(inv_records)}")[0]
                local1 = parse_records(row, "R", n)
                remote = peer.exchange({"kind": "inverse_r1", "records": local1})
                if remote.get("kind") != "inverse_r1":
                    raise RuntimeError("bad peer inverse R1")
                row = worker.call(f"INV2 {n} {encode_records(remote['records'])}")[0]
                local2 = parse_records(row, "R", n)
                remote = peer.exchange({"kind": "inverse_r2", "records": local2})
                if remote.get("kind") != "inverse_r2":
                    raise RuntimeError("bad peer inverse R2")
                row = worker.call(f"INV3 {n} {encode_records(remote['records'])}")[0]
                output = parse_records(row, "I", n)
                mask_share = [int(v[0]) & 1 for v in output]
        final = {"kind": "final", "status": status, "selected_handle": selected}
        remote = peer.exchange(final)
        if remote != final:
            status = "ABORT_PROTOCOL_STATE_MISMATCH"
            mask_share = None
        select_slots = sum(comparator.round_slots.values()) if comparator else 0
        membership_slots = comparator.membership_slots if comparator else 0
        result: dict[str, Any] = {
            "label": implementation_label,
            "party": party, "n": n, "K": k, "comparison_bits": bits,
            "input_seed": args.input_seed,
            "algorithm_seed": effective_algorithm_seed if args.joint_csprng_coins and n >= 2 else args.algorithm_seed,
            "algorithm_randomness": randomness_record,
            "status": status, "selected_anonymous_handle": selected,
            "round_metrics": round_metrics,
            "network_message_phases": (1 if n == 1 else (5 + (2 if raw_score_abi else 0) +
                                                           randomness_record["joint_coin_message_phases"])) +
                                      (comparator.net_rounds if comparator else 0),
            "public_graph_and_comparison_bits": True,
            "opened_original_index_or_shuffle_map": False,
            "test_only_forced_algorithm_abort": forced_abort,
            "test_only_same_os_peer_file_probe": peer_file_probe,
            "offline_keygen_online": False,
            "mask_share": mask_share if status == "SUCCESS" else None,
            "trace": selected_result.get("trace"),
            "metrics": {
                "raw_score_abi_input_adapter": "PROJECT_DERIVED_TEST_ONLY_PORT" if raw_score_abi else "NOT_USED_DIRECT_Z2_34",
                "raw_score_input_uCMP_key_slots_per_party": (2 * (1 << b)) if raw_score_abi and n >= 2 else 0,
                "raw_score_input_DCF_Eval_per_party": (4 * (1 << b)) if raw_score_abi and n >= 2 else 0,
                "raw_score_input_DCF_Eval_both_parties": (8 * (1 << b)) if raw_score_abi and n >= 2 else 0,
                "raw_score_input_peer_message_phases": (2 if raw_score_abi and n >= 2 else 0),
                "select_logical_comparison_calls": selected_result["total_comparison_calls"],
                "select_party_uCMP_key_slots_consumed": select_slots,
                "membership_party_key_slots_consumed": membership_slots,
                "select_dummy_related_logic_calls": sum(comparator.round_dummy.values()) if comparator else 0,
                "select_same_round_same_task_cached_calls": sum(comparator.round_dup.values()) if comparator else 0,
                "uCMP_calls_per_party": select_slots + membership_slots,
                "DCF_Eval_per_party": 2 * (select_slots + membership_slots),
                "DCF_Eval_both_parties": 4 * (select_slots + membership_slots),
                "offline_shuffle_cmpagg_slots_per_party_unused": 0 if n == 1 else n * (n - 1),
                "peer_payload_bytes_sent": peer.sent_bytes,
                "peer_payload_bytes_received": peer.received_bytes,
                "online_party_elapsed_seconds": time.perf_counter() - online_started,
                "online_PRG_calls": "NOT_MEASURED",
            },
        }
        Path(args.result).write_text(json.dumps(result, separators=(",", ":"), sort_keys=True) + "\n", encoding="utf-8")
        return result
    except Exception as exc:
        message = str(exc)
        if message.startswith("peer abort:"):
            peer_class = message.partition(":")[2]
            abort_class = peer_class if peer_class in {
                "ABORT_COMMUNICATION", "ABORT_MATERIAL", "ABORT_ALGORITHM_PROBABILITY"
            } else "ABORT_MATERIAL"
        elif isinstance(exc, (ConnectionError, TimeoutError, socket.timeout, BrokenPipeError)) or "peer closed" in message:
            abort_class = "ABORT_COMMUNICATION"
        else:
            abort_class = "ABORT_MATERIAL"
        try:
            if peer is not None:
                peer.send({"kind": "abort", "class": abort_class})
        except Exception:
            pass
        result = {"label": implementation_label,
                  "party": party, "n": n, "K": k,
                  "status": abort_class, "no_mask": True,
                  "error_class": type(exc).__name__, "error": str(exc)}
        Path(args.result).write_text(json.dumps(result, separators=(",", ":"), sort_keys=True) + "\n", encoding="utf-8")
        return result
    finally:
        if worker is not None:
            try:
                worker.close()
            except Exception:
                pass
        if peer is not None:
            peer.close()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--party", type=int, choices=(0, 1), required=True)
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--input", required=True)
    parser.add_argument("--package", required=True)
    parser.add_argument("--backend", required=True)
    parser.add_argument("--result", required=True)
    parser.add_argument("--input-seed", type=int, required=True)
    parser.add_argument("--algorithm-seed", type=int, required=True)
    parser.add_argument("--raw-score-abi", action="store_true")
    parser.add_argument("--joint-csprng-coins", action="store_true")
    parser.add_argument("--test-only-force-algorithm-abort", action="store_true")
    parser.add_argument("--test-only-probe-peer-files", action="store_true")
    parser.add_argument("--peer-timeout", type=float, default=20.0)
    args = parser.parse_args()
    result = run_party(args)
    print(json.dumps({k: v for k, v in result.items() if k not in {"trace", "mask_share"}},
                     separators=(",", ":"), sort_keys=True), flush=True)
    return 0 if result.get("status") == "SUCCESS" else 2


if __name__ == "__main__":
    raise SystemExit(main())
