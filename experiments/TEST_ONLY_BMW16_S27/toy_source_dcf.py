"""TEST_ONLY transcription of the ordinary DCF recurrence in VFSS/ext/FSS/dcf.cpp.

This is a small-word executable model, not an implementation of AES or a proof
of the source's PRG assumptions.  A fixed, independently sampled expansion
tape is used so KeyGen and Eval share exactly the same node expansion table.
The equations and bit order are transcribed from source lines 87-147 and
152-335.  Do not import this module from VFSS or any party executable.
"""

from __future__ import annotations

from dataclasses import dataclass
import random


@dataclass(frozen=True)
class Expansion:
    # State words are (lambda-bit seed << 2) | one control bit.  Bit 1 is
    # deliberately zero; as in dcf.cpp, AES expansion masks the low two bits.
    child: tuple[int, int]
    value: tuple[int, int]


class FixedExpansionTape:
    """A fixed random function from toy seeds to two child states and values."""

    def __init__(self, lam: int, output_bits: int, tape_seed: int):
        self.lam = lam
        self.seed_mask = (1 << lam) - 1
        self.output_mask = (1 << output_bits) - 1
        rng = random.Random(tape_seed)
        self.table: dict[int, Expansion] = {}
        for seed in range(1 << lam):
            left = (rng.getrandbits(lam) << 2) | rng.getrandbits(1)
            right = (rng.getrandbits(lam) << 2) | rng.getrandbits(1)
            self.table[seed] = Expansion(
                child=(left, right),
                value=(rng.getrandbits(output_bits) & self.output_mask,
                       rng.getrandbits(output_bits) & self.output_mask),
            )

    def expand(self, state: int) -> Expansion:
        return self.table[(state >> 2) & self.seed_mask]


@dataclass
class ToyPairKey:
    roots: tuple[int, int]
    cw: tuple[int, ...]
    g: int
    v: tuple[int, ...]
    bits: int
    output_bits: int


@dataclass(frozen=True)
class EvalStep:
    """One party's toy Eval transition, for source-equation inspection."""

    level: int
    before: int
    input_bit: int
    after: int
    signed_term: int
    running_share: int


def keygen(bits: int, alpha: int, payload: int, tape: FixedExpansionTape,
           root_seed: int) -> ToyPairKey:
    """Source-shaped KeyGen; alpha is read MSB first, like keyGenDCF."""
    out_mod = 1 << tape.output_mask.bit_length()
    # output_mask.bit_length() is w for 2^w-1.
    out_bits = tape.output_mask.bit_length()
    out_mask = out_mod - 1
    domain_mask = (1 << bits) - 1
    if alpha & ~domain_mask:
        raise ValueError("alpha outside toy domain")
    payload &= out_mask

    root_rng = random.Random(root_seed)
    raw0 = root_rng.getrandbits(tape.lam + 2)
    raw1 = root_rng.getrandbits(tape.lam + 2)
    # dcf.cpp: s[0]=(s[0]&~1)^((s[1]&1)^1); then the AES seed
    # masks both control bits, leaving exactly complementary root controls.
    root0 = (raw0 & ~1) ^ ((raw1 & 1) ^ 1)
    roots = (root0, raw1)

    states = [root0, raw1]
    corrections: list[int] = []
    value_cw: list[int] = []
    v_alpha = 0

    for i in range(bits):
        keep = (alpha >> (bits - 1 - i)) & 1
        exp0, exp1 = tape.expand(states[0]), tape.expand(states[1])
        si = (exp0.child, exp1.child)
        vi = (exp0.value, exp1.value)
        t0, t1 = states[0] & 1, states[1] & 1
        sign = -1 if t1 else 1
        lose = keep ^ 1

        # dcf.cpp:228-242. greaterThan is false for the ordinary strict-less
        # constructor.  Thus payload is added exactly when keep==1.
        vcw = sign * (-v_alpha - vi[0][lose] + vi[1][lose]
                      + (payload if keep == 1 else 0))
        vcw &= out_mask
        value_cw.append(vcw)
        v_alpha = (v_alpha - vi[1][keep] + vi[0][keep] + sign * vcw) & out_mask

        lose_seed_cw = (si[0][lose] ^ si[1][lose]) & ~3
        left_t_cw = ((si[0][0] ^ si[1][0]) & 1) ^ keep ^ 1
        right_t_cw = ((si[0][1] ^ si[1][1]) & 1) ^ keep
        cw = lose_seed_cw | (left_t_cw << 1) | right_t_cw
        corrections.append(cw)
        keep_t_cw = left_t_cw if keep == 0 else right_t_cw

        next_states = []
        for party in (0, 1):
            tprev = states[party] & 1
            corr = lose_seed_cw ^ keep_t_cw
            next_states.append(si[party][keep] ^ (corr if tprev else 0))
        states = next_states

    seed_mask = ~3
    s0 = states[0] & seed_mask
    s1 = states[1] & seed_mask
    low_mask = out_mask
    g = ((s1 & low_mask) - (s0 & low_mask) - v_alpha) & out_mask
    if states[1] & 1:
        g = (-g) & out_mask
    return ToyPairKey(roots, tuple(corrections), g, tuple(value_cw), bits, out_bits)


def eval_party_trace(party: int, x: int, key: ToyPairKey,
                     tape: FixedExpansionTape) -> tuple[int, tuple[EvalStep, ...]]:
    """Eval plus each node transition and additive output-share contribution."""
    if party not in (0, 1) or x < 0 or x >= (1 << key.bits):
        raise ValueError("invalid party or input")
    out_mask = (1 << key.output_bits) - 1
    state = key.roots[party]
    value_share = 0
    sign = 1 if party == 0 else -1
    steps: list[EvalStep] = []
    for i in range(key.bits):
        bit = (x >> (key.bits - 1 - i)) & 1
        before = state
        tprev = state & 1
        exp = tape.expand(state)
        raw_child = exp.child[bit]
        cw = key.cw[i]
        seed_cw = cw & ~3
        t_cw = (cw >> (1 if bit == 0 else 0)) & 1
        state = raw_child ^ ((seed_cw ^ t_cw) if tprev else 0)
        signed_term = (sign * (exp.value[bit] + tprev * key.v[i])) & out_mask
        value_share = (value_share + signed_term) & out_mask
        steps.append(EvalStep(i, before, bit, state, signed_term, value_share))

    leaf = state & ~3
    final_term = (leaf & out_mask) + ((state & 1) * key.g)
    value_share = (value_share + sign * final_term) & out_mask
    return value_share, tuple(steps)


def eval_party(party: int, x: int, key: ToyPairKey, tape: FixedExpansionTape) -> int:
    """Source-shaped Eval, including the final seed/g correction and sign."""
    return eval_party_trace(party, x, key, tape)[0]


def reconstruct(bits: int, alpha: int, x: int, payload: int,
                tape_seed: int, root_seed: int, lam: int = 4,
                output_bits: int = 8) -> tuple[int, int, int]:
    tape = FixedExpansionTape(lam, output_bits, tape_seed)
    key = keygen(bits, alpha, payload, tape, root_seed)
    y0 = eval_party(0, x, key, tape)
    y1 = eval_party(1, x, key, tape)
    return y0, y1, (y0 + y1) & ((1 << output_bits) - 1)
