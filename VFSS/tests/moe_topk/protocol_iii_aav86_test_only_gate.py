"""TEST_ONLY plaintext algebra/counterexample fixture; no secure protocol or keys."""

import random
import unittest
from collections import Counter
from itertools import product

MASK32 = (1 << 32) - 1
MASK64 = (1 << 64) - 1


def signed(word):
    word &= MASK32
    return word - (1 << 32) if word & (1 << 31) else word


def padded_size(n):
    return max(2, 1 << (n - 1).bit_length())


def priority_keys(words, use_counterfactual_zero_dummy=False):
    n, d = len(words), padded_size(len(words))
    index_bits = (d - 1).bit_length()
    padded = list(words) + [0 if use_counterfactual_zero_dummy else 0x80000000] * (d - n)
    return [((MASK32 - ((word & MASK32) ^ 0x80000000)) << index_bits) | i
            for i, word in enumerate(padded)]


def oracle(words, k):
    order = sorted(range(len(words)), key=lambda i: (-signed(words[i]), i))
    chosen = set(order[:k])
    return [int(i in chosen) for i in range(len(words))]


def ceil_root(m, depth):
    return next(t for t in range(1, m + 1) if t ** depth >= m)


def model(words, k, rounds, seed, use_counterfactual_zero_dummy=False):
    """Returns reconstruction, public graph trace, and ring rank shares.

    All sorting, comparisons, shares and inverse routing are in this TEST_ONLY
    function. It must never be called from a secure party or a benchmark.
    """
    n = len(words)
    d = padded_size(n)
    assert n >= 2 and 1 <= k <= n and 1 <= rounds <= 5
    keys = priority_keys(words, use_counterfactual_zero_dummy)
    rng = random.Random(seed)
    by_handle = list(range(d))
    rng.shuffle(by_handle)  # TEST_ONLY clear permutation
    ring = 1 << (d - 1).bit_length()
    ranks = [None] * d
    public_trace = []
    active = [(list(range(d)), 0, rounds)]
    for stage in range(rounds):
        next_active = []
        edges = set()
        vertices_seen = set()
        for handles, offset, depth in active:
            if len(handles) == 1:
                ranks[handles[0]] = offset
                continue
            q = ceil_root(len(handles), depth) - 1
            pivots = rng.sample(handles, q)
            for i, a in enumerate(handles):
                for b in handles[i + 1:]:
                    if a in pivots or b in pivots:
                        edges.add(tuple(sorted((a, b))))
                        vertices_seen.update((a, b))
            # The actual early graph compares edges incident to pivots only.
            # At depth one q=m-1, so this graph is the complete clique.
            local = {h: sum(keys[by_handle[x]] < keys[by_handle[h]]
                            for x in handles if x in pivots or h in pivots)
                     for h in handles}
            if depth == 1:
                # Final local ranks are used as shares; none is opened here.
                for h in handles:
                    ranks[h] = offset + local[h]
                continue
            pivot_order = sorted(pivots, key=lambda h: local[h])
            buckets = [[] for _ in range(q + 1)]
            for h in handles:
                if h not in pivots:
                    bucket = sum(keys[by_handle[p]] < keys[by_handle[h]]
                                 for p in pivots)
                    buckets[bucket].append(h)
            for h in pivots:
                ranks[h] = offset + local[h]
            for bucket_index, bucket in enumerate(buckets):
                if not bucket:
                    continue
                child_offset = (offset if bucket_index == 0 else
                                offset + local[pivot_order[bucket_index - 1]] + 1)
                if len(bucket) == 1:
                    ranks[bucket[0]] = child_offset
                else:
                    next_active.append((bucket, child_offset, depth - 1))
        public_trace.append((len(edges), len(vertices_seen)))
        active = next_active
    assert all(rank is not None for rank in ranks)
    assert sorted(ranks) == list(range(d))
    assert sum(e for e, _ in public_trace) <= rounds * d * (d - 1) // 2
    # TEST_ONLY emulation of additive rank shares and 64-bit DPF output shares.
    rank_share0 = [rng.randrange(ring) for _ in range(d)]
    rank_share1 = [(ranks[h] - rank_share0[h]) % ring for h in range(d)]
    assert all((rank_share0[h] + rank_share1[h]) % ring == ranks[h]
               for h in range(d))
    handle_bit0, handle_bit1 = [], []
    for h in range(d):
        rank_mask = rng.randrange(ring)
        opened = (rank_share0[h] + rank_share1[h] + rank_mask) % ring
        total0 = total1 = 0
        for target in range(k):
            point = (opened - target) % ring
            indicator = int(point == rank_mask)
            share0 = rng.getrandbits(64)
            share1 = (indicator - share0) & MASK64
            total0 = (total0 + share0) & MASK64
            total1 = (total1 + share1) & MASK64
        handle_bit0.append(total0 & 1)
        handle_bit1.append(total1 & 1)
    # TEST_ONLY clear inverse π. This is the secure construction obligation.
    original0 = [0] * d
    original1 = [0] * d
    for h, original in enumerate(by_handle):
        original0[original] = handle_bit0[h]
        original1[original] = handle_bit1[h]
    return ([original0[i] ^ original1[i] for i in range(n)],
            public_trace, (rank_share0, rank_share1), by_handle)


class GateFixture(unittest.TestCase):
    def test_counterfactual_zero_padding_fails(self):
        words = [MASK32] * 3  # signed -1, -1, -1
        # This deliberately bypasses the real adapter, which uses INT32_MIN.
        got, _, _, _ = model(words, 3, 2, 7, use_counterfactual_zero_dummy=True)
        self.assertEqual(sum(got), 2)
        self.assertEqual(oracle(words, 3), [1, 1, 1])

    def test_handle_bits_cannot_be_returned_as_original_bits(self):
        # π swaps two slots. Handle selection [0,1] must inverse-route to [1,0].
        by_handle = [1, 0]
        handle_mask = [0, 1]
        correct = [0, 0]
        for h, original in enumerate(by_handle):
            correct[original] = handle_mask[h]
        self.assertEqual(correct, [1, 0])
        self.assertNotEqual(handle_mask, correct)

    def test_odd_field_parity_is_not_additive(self):
        prime = (1 << 127) - 1
        self.assertEqual((prime - 1 + 2) % prime, 1)
        self.assertNotEqual(((prime - 1) & 1) ^ (2 & 1), 1)

    def test_inverse_peer_frame_conditional_uniformity_toy_ring(self):
        # TEST_ONLY exhaustive two-handle, Z4 check of the E7 inverse
        # algebra with a secret peer carrier share (as III DPF would supply).
        # e0=-tau(a1)-h, h is fresh uniform, so conditioning on e0 leaves
        # a1 uniform even if the peer carrier c1 is fixed.
        ring = 4
        tau = (1, 0)
        gamma_peer = (0, 1)
        fixed_e0 = (1, 3)
        for peer_carrier in product(range(ring), repeat=2):
            outcomes = Counter()
            for peer_a in product(range(ring), repeat=2):
                h = tuple((-peer_a[tau[i]] - fixed_e0[i]) % ring
                          for i in range(2))
                self.assertEqual(
                    tuple((-peer_a[tau[i]] - h[i]) % ring for i in range(2)),
                    fixed_e0)
                peer_frame = tuple((peer_carrier[gamma_peer[i]] + peer_a[i]) % ring
                                   for i in range(2))
                own_output = tuple((peer_frame[tau[i]] + fixed_e0[i]) % ring
                                   for i in range(2))
                outcomes[own_output] += 1
            self.assertEqual(len(outcomes), ring ** 2)
            self.assertEqual(set(outcomes.values()), {1})

    def test_differential_boundaries_and_seeds(self):
        cases = [
            [0x80000000] * 5,
            [0x80000000, 0x7FFFFFFF, 7, 7, 0],
            [MASK32] * 5,
            [0x80000000, MASK32],
            [0x80000000, MASK32, 0, 1, 1, 0x7FFFFFFF, 0x80000000, 2],
            [17] * 8,
        ]
        for words in cases:
            n = len(words)
            for k in sorted({1, max(1, n // 2), n}):
                for rounds in range(1, 6):
                    for seed in (1, 7, 31):
                        with self.subTest(n=n, k=k, rounds=rounds, seed=seed):
                            got, trace, shares, _ = model(words, k, rounds, seed)
                            self.assertEqual(got, oracle(words, k))
                            self.assertEqual(sum(got), k)
                            self.assertEqual(len(trace), rounds)
                            self.assertEqual(len(shares[0]), padded_size(n))


if __name__ == "__main__":
    unittest.main()
