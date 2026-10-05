"""Tests for source algebra and the frozen TEST_ONLY oracle, not BMW16 Select."""

from __future__ import annotations

import unittest
from fractions import Fraction

from audit_reference import (
    algorithm5_literal_parameters,
    algorithm7_ideal_partition_trace,
    build_audit,
    ceil_nth_root_power,
    select_partition_expansion,
)
from oracle import INT32_MAX, INT32_MIN, select_index, stable_order, top_k_mask


class StableOracleTests(unittest.TestCase):
    def test_signed_boundaries_and_ties(self) -> None:
        scores = [INT32_MIN, 0, INT32_MAX, 0]
        self.assertEqual(stable_order(scores), [2, 1, 3, 0])
        self.assertEqual(select_index(scores, 2), 1)
        self.assertEqual(top_k_mask(scores, 2), [0, 1, 1, 0])

    def test_all_equal_uses_original_index(self) -> None:
        self.assertEqual(stable_order([7, 7, 7, 7]), [0, 1, 2, 3])
        self.assertEqual(top_k_mask([7, 7, 7, 7], 2), [1, 1, 0, 0])

    def test_input_contract(self) -> None:
        for scores, k in [([], 1), ([1], 0), ([1], 2), ([1 << 31], 1)]:
            with self.subTest(scores=scores, k=k), self.assertRaises(ValueError):
                select_index(scores, k)


class PaperAlgebraTests(unittest.TestCase):
    def test_integer_roots(self) -> None:
        self.assertEqual(ceil_nth_root_power(128, 1, 3), 6)
        self.assertEqual(ceil_nth_root_power(128, 2, 3), 26)
        self.assertEqual(ceil_nth_root_power(10, 25, 36), 5)

    def test_appendix_reduction_expansion_for_all_small_k(self) -> None:
        for n in range(1, 32):
            for k in range(1, n + 1):
                result = select_partition_expansion(n, k)
                m = result["median_input_n"]
                ascending_rank = n - k + 1
                pad_low = result["low_padding"]
                self.assertEqual(m % 2, 0)
                self.assertLessEqual(m, 2 * n)
                self.assertEqual(ascending_rank + pad_low, m // 2)
                self.assertEqual(result["equivalent_ascending_rank_1_based"], ascending_rank)

                # Simulate the two exact median partitions over tagged strict
                # ranks.  This checks only the appendix set-intersection
                # identity; it is not a comparison algorithm.
                real = [("real", rank) for rank in range(1, n + 1)]
                outer_min = ("outer_min", 0)
                outer_max = ("outer_max", 0)
                low_pad = [("reduction_low", i) for i in range(pad_low)]
                high_pad = [
                    ("reduction_high", i) for i in range(result["high_padding"])
                ]
                order = {
                    "reduction_low": 0,
                    "outer_min": 1,
                    "real": 2,
                    "outer_max": 3,
                    "reduction_high": 4,
                }
                key = lambda item: (order[item[0]], item[1])
                first = sorted(low_pad + [outer_min] + real + high_pad, key=key)
                second = sorted(low_pad + real + [outer_max] + high_pad, key=key)
                first_accept = set(first[m // 2 :])
                second_reject = set(second[: m // 2])
                intersection = [
                    tag[1]
                    for tag in first_accept & second_reject
                    if tag[0] == "real"
                ]
                self.assertEqual(intersection, [ascending_rank])

    def test_project_rank_endpoints_and_middle(self) -> None:
        expected = {
            (8, 1): (8, 0, 7, 16),
            (8, 4): (5, 0, 1, 10),
            (8, 5): (4, 1, 0, 10),
            (8, 8): (1, 7, 0, 16),
            (7, 1): (7, 0, 6, 14),
            (7, 7): (1, 6, 0, 14),
        }
        for (n, k), (r, low, high, median_n) in expected.items():
            with self.subTest(n=n, k=k):
                got = select_partition_expansion(n, k)
                self.assertEqual(got["equivalent_ascending_rank_1_based"], r)
                self.assertEqual(got["low_padding"], low)
                self.assertEqual(got["high_padding"], high)
                self.assertEqual(got["median_input_n"], median_n)

    def test_algorithm5_literal_line_can_make_b_undefined(self) -> None:
        result = algorithm5_literal_parameters(348)
        self.assertEqual(result["S1_size_project_ceil"], 50)
        self.assertEqual(result["Algorithm5_line4_literal_k1"], 174)
        self.assertFalse(result["line11_b_exists_possible"])

    def test_algorithm7_line11_negative_rank_on_exact_partitions(self) -> None:
        result = algorithm7_ideal_partition_trace(254, 47)
        self.assertEqual(result["U_size"], 94)
        self.assertEqual(result["Algorithm7_line11_source_rank_n_over_2_minus_R2"], -47)
        self.assertEqual(result["rank_from_rejected_prefix_invariant_n_over_2_minus_R1"], 47)
        self.assertEqual(result["U_half"], 47)

    def test_smallest_project_input_is_reported_as_not_runnable(self) -> None:
        result = build_audit(1, 1, Fraction(1, 36))
        self.assertFalse(result["complete_select_implemented"])
        self.assertFalse(result["per_partition_algorithm7_candidate"]["ideal_cardinality_witness_feasible"])
        self.assertIsNone(result["per_partition_algorithm7_candidate"]["candidate_project_v_size_ceil_then_bound_by_u"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
