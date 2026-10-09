#!/usr/bin/env python3
"""Exhaustive tiny-domain sanity check for the DCF one-time-pad algebra.

This is not a cryptographic security proof.  It models one DCF level with
independent ideal-expansion tapes at distinct root seeds, a 2-bit seed domain,
one control bit, and values in Z/2Z.  It checks the complete one-level party
key tuple (root, CW, v[0], g) for every threshold/payload and both corrupted
parties.  The general-bin proof is the dated S28 review document.
"""

from collections import Counter


def unpack_expansion(word):
    """(sL[2],tL,sR[2],tR,VL[1],VR[1]) from an 8-bit random tape."""
    return (
        word & 0b11,
        (word >> 2) & 1,
        (word >> 3) & 0b11,
        (word >> 5) & 1,
        (word >> 6) & 1,
        (word >> 7) & 1,
    )


def check():
    expected_per_key = 6144
    checked = 0
    for alpha in range(2):
        for beta in range(2):
            hist = (Counter(), Counter())
            # Condition on distinct root seeds so the two ideal-function
            # expansion outputs are independent.  The excluded collision is
            # accounted separately in the real security hybrid.
            for root0 in range(4):
                for root1 in range(4):
                    if root0 == root1:
                        continue
                    for t1 in range(2):
                        t0 = 1 - t1  # keyGenDCF's complementary root controls
                        keep, lose = alpha, 1 - alpha
                        for unused_bit0 in range(2):
                          for unused_bit1 in range(2):
                            for tape0 in range(256):
                                x0 = unpack_expansion(tape0)
                                s0 = (x0[0], x0[2])
                                c0 = (x0[1], x0[3])
                                val0 = (x0[4], x0[5])
                                for tape1 in range(256):
                                    x1 = unpack_expansion(tape1)
                                    s1 = (x1[0], x1[2])
                                    c1 = (x1[1], x1[3])
                                    val1 = (x1[4], x1[5])

                                    cw_seed = s0[lose] ^ s1[lose]
                                    d_left = c0[0] ^ c1[0] ^ keep ^ 1
                                    d_right = c0[1] ^ c1[1] ^ keep
                                    d_keep = d_left if keep == 0 else d_right

                                    # The source uses epsilon in {+1,-1}.  Modulo
                                    # two both are the unit 1; the general proof
                                    # uses the fact that +/-1 is invertible in
                                    # Z/(2^Bout).
                                    epsilon = 1
                                    v = (epsilon * (
                                        -val0[lose] + val1[lose]
                                        + (beta if keep == 1 else 0)
                                    )) & 1
                                    accumulator = (
                                        -val1[keep] + val0[keep] + epsilon * v
                                    ) & 1

                                    next_seed0 = s0[keep] ^ (t0 & cw_seed)
                                    next_seed1 = s1[keep] ^ (t1 & cw_seed)
                                    next_control1 = c1[keep] ^ (t1 & d_keep)
                                    g = (next_seed1 - next_seed0 - accumulator) & 1
                                    if next_control1:
                                        g = (-g) & 1

                                    common = (cw_seed, d_left, d_right, v, g)
                                    hist[0][(root0, unused_bit0, t0) + common] += 1
                                    hist[1][(root1, unused_bit1, t1) + common] += 1
                                    checked += 1

            for party_hist in hist:
                if len(party_hist) != 1024:
                    raise AssertionError(f"expected 1024 key tuples, got {len(party_hist)}")
                if set(party_hist.values()) != {expected_per_key}:
                    raise AssertionError("party-key tuple distribution is not uniform")
    print(
        "PASS: 4 threshold/payload settings; both party keys; "
        f"{checked:,} conditioned ideal-tape cases; each 1,024-tuple serialized "
        f"root/CW/v/g projection has mass {expected_per_key:,} per tuple. "
        "TEST_ONLY algebra check only."
    )


if __name__ == "__main__":
    check()
