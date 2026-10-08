import unittest

from toy_source_dcf import FixedExpansionTape, eval_party_trace, keygen, reconstruct


class SourceTranscriptionTests(unittest.TestCase):
    def test_strict_less_than_on_all_toy_inputs(self):
        # Fixed roots and fixed independent expansion tape; exhaust all
        # thresholds/inputs for widths 1..5 and vary the deterministic tape.
        for bits in range(1, 6):
            for tape_seed in (0, 1, 7, 20261008):
                root_seed = 0xDCF150 + tape_seed
                for alpha in range(1 << bits):
                    for x in range(1 << bits):
                        with self.subTest(bits=bits, tape=tape_seed,
                                          alpha=alpha, x=x):
                            _, _, value = reconstruct(
                                bits, alpha, x, 1, tape_seed, root_seed)
                            self.assertEqual(value, int(x < alpha))

    def test_payload_and_zero_function(self):
        for bits in range(1, 5):
            for alpha in range(1 << bits):
                for payload in (0, 1, 7, 255):
                    for x in range(1 << bits):
                        _, _, value = reconstruct(
                            bits, alpha, x, payload,
                            tape_seed=23, root_seed=91)
                        expected = payload if x < alpha else 0
                        self.assertEqual(value, expected & 0xFF)

    def test_repeated_eval_is_deterministic(self):
        args = (5, 19, 11, 7, 55, 66)
        self.assertEqual(reconstruct(*args), reconstruct(*args))

    def test_per_level_control_convergence_and_share_accumulation(self):
        # Before the first differing bit, target-prefix controls remain
        # complementary. At that first differing edge the correction makes
        # the complete post-states equal; they stay equal thereafter.
        # Per-party trace sums must match the returned additive share.
        for bits in range(1, 6):
            for tape_seed in (0, 1, 7, 20261008):
                tape = FixedExpansionTape(4, 8, tape_seed)
                for alpha in range(1 << bits):
                    key = keygen(bits, alpha, 1, tape, 0xDCF150 + tape_seed)
                    for x in range(1 << bits):
                        y0, trace0 = eval_party_trace(0, x, key, tape)
                        y1, trace1 = eval_party_trace(1, x, key, tape)
                        mismatch_seen = False
                        for left, right in zip(trace0, trace1):
                            self.assertEqual(left.level, right.level)
                            if mismatch_seen:
                                self.assertEqual(left.before, right.before)
                            else:
                                self.assertEqual((left.before ^ right.before) & 1, 1)
                                if left.input_bit != ((alpha >> (bits - 1 - left.level)) & 1):
                                    mismatch_seen = True
                                    self.assertEqual(left.after, right.after)
                            self.assertEqual(
                                left.running_share,
                                sum(step.signed_term for step in trace0[:left.level + 1]) & 0xFF)
                            self.assertEqual(
                                right.running_share,
                                sum(step.signed_term for step in trace1[:right.level + 1]) & 0xFF)
                        self.assertEqual(y0 + y1 & 0xFF, int(x < alpha))

    def test_literal_bgi_target_seed_cw_assignment_is_not_distribution_equal(self):
        # Proposed literal mapping: BGI's two target-branch c_s words are
        # assigned from the source's common scw through the previous control
        # bits. Since target controls are complementary, the pair lies on one
        # coordinate axis. BGI Alg. 5 lines 7-8 samples the target pair as two
        # independent uniform lambda-bit words. This rejects that *literal
        # variable assignment*; it is not a distinguisher for the serialized
        # source key under some other, yet-to-be-proved encoding.
        lam = 4
        domain = range(1 << lam)
        source_pairs = [(0, cw) for cw in domain]  # fixed (t0,t1)=(0,1)
        source_axis_event = sum(a == 0 or b == 0 for a, b in source_pairs) / len(source_pairs)
        bgi_pair_count = (1 << (2 * lam))
        bgi_axis_count = 2 * (1 << lam) - 1
        bgi_axis_event = bgi_axis_count / bgi_pair_count
        self.assertEqual(source_axis_event, 1.0)
        self.assertEqual(bgi_axis_event, 31 / 256)
        self.assertEqual(source_axis_event - bgi_axis_event, 225 / 256)


if __name__ == "__main__":
    unittest.main(verbosity=2)
