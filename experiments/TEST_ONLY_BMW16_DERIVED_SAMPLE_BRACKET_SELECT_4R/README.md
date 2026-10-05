# TEST_ONLY BMW16-derived sample-bracket Select

Implementation label: `BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R_TEST_ONLY`.

This directory contains a plaintext research reference only. It is a separate
project-derived four-layer sample-bracketing construction specified in
`docs/decisions/BMW16_S4_SAMPLE_BRACKET_SELECT_DECISION_2026-10-05.md`. It is not
the S2 Algorithm 5/7 interpretation, does not inherit Theorem 8's theorem
statement, does not reproduce BB90, and is not connected to `VFSS/`.

The only successful algorithm result is a single candidate original index.
Every sample is selected before its round plan is frozen; the complete batch is
recorded before the comparison callback evaluates any edge. Later-layer edges
are constructed only after the prior layer's results have been consumed. Aborts
return no candidate and are retained as failures. The algorithm module does not
import an oracle, call a sort, retry, or add uncounted comparisons.

Top-level results use `SUCCESS`, `PROJECT_RANDOM_FAILURE_PATH`, and
`COMPLETED_WRONG`. Random sample failures are explicit aborts with no candidate;
they are not paper-permitted random-label branches. `COMPLETED_WRONG` denotes a
violated implementation invariant and is never treated as a probabilistic
failure.

The stable ascending order is `(signed_score, -original_index)`. Internal
category tags place low/high rank padding and the two Select sentinels outside
the full signed score domain. The shuffled slot is never substituted for the
original index.

## Dependencies and one-case replay

The command uses Python 3 and NumPy for batched comparison execution. It requires
a JSON file containing signed int32 raw Q20.12 score integers:

```powershell
python -B select4r.py --scores-json scores.json --k 8 `
  --input-seed 1234 --algorithm-seed 5678 --trace trace.json
```

`trace.json` is a TEST_ONLY, plaintext edge trace. Keep it outside Git. Without
`--trace`, the CLI prints a compact summary to stdout. For validation, use the
separate `validate_results.py`; the selector itself never calls the frozen
oracle.

## Validation artifacts

- `test_select4r.py`: exhaustive n≤7 stable-order differential and adversarial
  boundary cases.
- `run_matrix.py`: freezes two seed schedules (fixed input and random int32
  scores), then runs 1,000 independent algorithm seeds for each of the five
  official `(n,K)` configurations in each family.
- `validate_results.py`: compares candidates with the frozen S1 Python oracle,
  retains every failure and computes one-sided 95% Clopper–Pearson lower bounds.
- `audit_edges.py`: independently reconstructs a saved full trace's batches,
  comparisons, rank-derived endpoints, causality and edge hashes.
- `oracle_harness.cpp` and `crosscheck_oracles.py`: cross-check stable order
  against `VFSS/include/moe_topk/topk_oracle.h` without modifying production code.

All matrix outputs, full traces, input schedules, and C++ build outputs belong in
an external evidence directory. A successful finite sample does not replace the
ideal-uniform finite probability proof or authorize secure composition.
