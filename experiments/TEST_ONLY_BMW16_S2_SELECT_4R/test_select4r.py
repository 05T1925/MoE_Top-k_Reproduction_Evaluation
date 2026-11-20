"""Unit tests for the TEST_ONLY scheduler and independent trace auditor."""

from __future__ import annotations

import itertools
from pathlib import Path
import unittest

from audit_edges import audit_trace
from select4r import Item, INT32_MAX, INT32_MIN, generated_scores, run_select


class KeyAndExpansionTests(unittest.TestCase):
    def test_stable_key_ties_and_signed_edges(self) -> None:
        low_score = Item("real:0", "real", INT32_MIN, 0)
        high_score = Item("real:1", "real", INT32_MAX, 1)
        tie_first = Item("real:2", "real", 7, 2)
        tie_later = Item("real:3", "real", 7, 3)
        self.assertLess(low_score.order_key(), tie_later.order_key())
        self.assertLess(tie_later.order_key(), tie_first.order_key())
        self.assertLess(tie_first.order_key(), high_score.order_key())

    def test_outer_padding_maps_every_small_n_k_to_even_median(self) -> None:
        for n in range(1, 50):
            for k in range(1, n + 1):
                r = n - k + 1
                N = n + 1
                low = max(N - 2 * r, 0)
                high = max(2 * r - N, 0)
                M = N + low + high
                h = M // 2
                self.assertEqual(M % 2, 0)
                self.assertLessEqual(M, 2 * n)
                self.assertEqual(r + low, h)
                self.assertEqual(r + low + 1, h + 1)


class SchedulerTests(unittest.TestCase):
    def test_success_trace_is_four_rounds_and_audits_independently(self) -> None:
        scores = generated_scores(128, 1, "random")
        summary, trace = run_select(scores, 8, algo_seed=101, input_seed=1)
        self.assertEqual(summary["status"], "SUCCESS")
        self.assertIsNotNone(summary["selected_original_index"])
        self.assertEqual(summary["comparison_rounds"], 4)
        audit = audit_trace(trace)
        self.assertEqual(audit["audit"], "PASS")
        self.assertEqual(set(audit["rounds"]), {1, 2, 3, 4})
        self.assertEqual(
            audit["total_comparison_calls"],
            summary["round_metrics"][0]["total_comparison_calls"],
        )

    def test_paper_random_branch_remains_classified(self) -> None:
        scores = generated_scores(128, 2, "random")
        summary, trace = run_select(scores, 8, algo_seed=102, input_seed=2)
        self.assertEqual(summary["status"], "PAPER_RANDOM_FAILURE_PATH")
        self.assertTrue(any(p["random_branch"] for p in summary["partitions"].values()))
        self.assertEqual(audit_trace(trace)["audit"], "PASS")

    def test_undefined_r1_bracket_is_not_silently_repaired(self) -> None:
        scores = [((i * 17) % 199) - 99 for i in range(256)]
        summary, trace = run_select(scores, 8, algo_seed=100, input_seed=0)
        # Seed 0 under this fixed case preserves the missing-sample bracket as
        # an explicit result; no retry, oracle, or extra round is attempted.
        self.assertEqual(summary["status"], "UNDEFINED/INVALID_FINITE_CASE")
        self.assertIn(
            "A5_R1_MISSING_ONE_SIDE_OF_S1_TARGET_BRACKET",
            summary["failure_reasons"][0],
        )
        self.assertEqual(audit_trace(trace)["audit"], "PASS")

    def test_small_strict_order_permutations_execute_without_hidden_oracle(self) -> None:
        ordinal = 0
        for n in range(1, 5):
            for permutation in itertools.permutations(range(n)):
                for k in range(1, n + 1):
                    summary, trace = run_select(list(permutation), k, 7000 + ordinal, ordinal)
                    self.assertIn(summary["status"], {
                        "SUCCESS", "PAPER_RANDOM_FAILURE_PATH", "UNDEFINED/INVALID_FINITE_CASE",
                        "COMPLETED_WRONG",
                    })
                    self.assertEqual(audit_trace(trace)["audit"], "PASS")
                    ordinal += 1

    def test_runtime_has_no_oracle_import(self) -> None:
        runtime = Path(__file__).with_name("select4r.py").read_text(encoding="utf-8")
        self.assertNotIn("from oracle", runtime)
        self.assertNotIn("import oracle", runtime)


if __name__ == "__main__":
    unittest.main(verbosity=2)
