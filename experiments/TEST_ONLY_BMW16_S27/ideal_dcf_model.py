#!/usr/bin/env python3
"""TEST_ONLY ideal-expansion model for VFSS's compressed comparison DCF.

This is an algebra/conformance aid, not cryptographic code and not a proof of
AES security.  It replaces each restricted-key AES expansion by a deterministic
random function with a fixed, reproducible random tape, then follows the field
updates in VFSS/ext/FSS/dcf.cpp for groupSize=1.
"""

from __future__ import annotations

import argparse
import random
from dataclasses import dataclass

MASK128 = (1 << 128) - 1
MASK126 = MASK128 ^ 3
MASK64 = (1 << 64) - 1


class IdealExpansion:
    def __init__(self, seed: int):
        self.tape = random.Random(seed)
        self.outputs: dict[int, tuple[int, int, int, int]] = {}

    def __call__(self, seed_block: int) -> tuple[int, int, int, int]:
        key = seed_block & MASK126
        if key not in self.outputs:
            self.outputs[key] = tuple(
                self.tape.getrandbits(128) if i < 2 else self.tape.getrandbits(64)
                for i in range(4)
            )  # two child blocks and two 64-bit value projections
        return self.outputs[key]


@dataclass(frozen=True)
class PartyKey:
    party: int
    bits: int
    root: int
    cw: tuple[int, ...]
    g: int
    v: tuple[int, ...]

    def serialize(self) -> bytes:
        # Matches M2UC v1 shape on the repository's little-endian block ABI.
        out = bytearray(b"M2UC\x01")
        out += bytes((self.party, self.bits, 64, 1))
        for size in (self.bits + 1, 1, self.bits):
            out += size.to_bytes(8, "big")
        for block in (self.root, *self.cw):
            out += block.to_bytes(16, "little")
        out += self.g.to_bytes(8, "big")
        for value in self.v:
            out += value.to_bytes(8, "big")
        return bytes(out)


def keygen(bits: int, alpha: int, payload: int, expand: IdealExpansion,
           tape: random.Random) -> tuple[PartyKey, PartyKey]:
    assert 1 <= bits <= 32
    assert 0 <= alpha < (1 << bits)
    assert 0 <= payload <= MASK64
    state = [tape.getrandbits(128), tape.getrandbits(128)]
    state[0] = (state[0] & (MASK128 ^ 1)) | ((state[1] & 1) ^ 1)
    roots = tuple(state)
    v_alpha = 0
    cws: list[int] = []
    values: list[int] = []

    for level in range(bits):
        keep = (alpha >> (bits - 1 - level)) & 1
        children = [expand(state[party] & MASK126) for party in range(2)]
        t = [state[party] & 1 for party in range(2)]
        sign = -1 if t[1] else 1

        lose = keep ^ 1
        lose0, lose1 = children[0][lose + 2], children[1][lose + 2]
        payload_on_lose = payload if keep == 1 else 0  # greaterThan=false
        vi = sign * (-v_alpha - lose0 + lose1 + payload_on_lose)
        vi &= MASK64

        keep0, keep1 = children[0][keep + 2], children[1][keep + 2]
        v_alpha = (v_alpha - keep1 + keep0 + sign * vi) & MASK64

        left_xor = children[0][0] ^ children[1][0]
        right_xor = children[0][1] ^ children[1][1]
        d_left = ((left_xor & 1) ^ keep ^ 1)
        d_right = ((right_xor & 1) ^ keep)
        scw = (children[0][lose] ^ children[1][lose]) & MASK126
        cw = scw | (d_left << 1) | d_right
        cws.append(cw)

        for party in range(2):
            child = children[party][keep]
            t_keep = d_left if keep == 0 else d_right
            state[party] = child ^ ((scw ^ t_keep) if t[party] else 0)

        values.append(vi)

    # convert(64,1,block) reads the first little-endian uint64 after clearing
    # the two low control/reserved bits.
    q0 = (state[0] & MASK126) & MASK64
    q1 = (state[1] & MASK126) & MASK64
    g = (q1 - q0 - v_alpha) & MASK64
    if state[1] & 1:
        g = (-g) & MASK64

    return (
        PartyKey(0, bits, roots[0], tuple(cws), g, tuple(values)),
        PartyKey(1, bits, roots[1], tuple(cws), g, tuple(values)),
    )


def eval_key(key: PartyKey, x: int, expand: IdealExpansion) -> int:
    state = key.root
    out = 0
    for level in range(key.bits):
        keep = (x >> (key.bits - 1 - level)) & 1
        t_previous = state & 1
        cw = key.cw[level]
        scw = cw & MASK126
        ds = ((cw >> 1) & 1, cw & 1)
        # ecbEncTwoBlocks(blocks+2*keep) returns the chosen seed child and
        # its paired value block.
        selected = expand(state & MASK126)
        child_seed = selected[keep]
        child_value = selected[keep + 2]
        d_keep = ds[keep]
        stcw = ((scw ^ d_keep) if t_previous else 0)
        sign = -1 if key.party == 1 else 1
        out = (out + sign * (child_value + t_previous * key.v[level])) & MASK64
        state = child_seed ^ stcw

    final = (state & MASK126) & MASK64
    if state & 1:
        final = (final + key.g) & MASK64
    if key.party == 1:
        final = (-final) & MASK64
    return (out + final) & MASK64


def check(bits: int, seed: int) -> tuple[int, int]:
    tape = random.Random(seed)
    checks = 0
    key_bytes = 0
    for alpha in range(1 << bits):
        for payload in (0, 1, MASK64):
            expand = IdealExpansion(tape.getrandbits(64))
            key0, key1 = keygen(bits, alpha, payload, expand, tape)
            expected_len = 57 + 24 * bits
            assert len(key0.serialize()) == expected_len
            assert len(key1.serialize()) == expected_len
            key_bytes += len(key0.serialize()) + len(key1.serialize())
            for x in range(1 << bits):
                share0 = eval_key(key0, x, expand)
                share1 = eval_key(key1, x, expand)
                expected = payload if x < alpha else 0
                assert (share0 + share1) & MASK64 == expected, (
                    f"DCF mismatch b={bits} alpha={alpha} x={x} payload={payload}"
                )
                checks += 1
    return checks, key_bytes


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--seed", type=int, default=270109)
    parser.add_argument("--max-bits", type=int, default=5)
    args = parser.parse_args()
    if not 1 <= args.max_bits <= 8:
        parser.error("--max-bits must be in 1..8")
    total_checks = total_bytes = 0
    for bits in range(1, args.max_bits + 1):
        checks, key_bytes = check(bits, args.seed + bits)
        total_checks += checks
        total_bytes += key_bytes
        print(f"IDEAL_DCF_BITS={bits} inputs_checked={checks} serialized_key_bytes={key_bytes} result=PASS")
    print(f"TOTAL fixed_tape_seed={args.seed} max_bits={args.max_bits} input_checks={total_checks} serialized_key_bytes={total_bytes} result=PASS")
    print("LIMITATION=TEST_ONLY ideal random expansion model; not an AES proof or security experiment")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
