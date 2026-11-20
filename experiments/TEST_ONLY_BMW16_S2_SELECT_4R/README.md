# BMW16-derived four-round Select (plaintext TEST_ONLY)

Implementation label: `BMW16_DERIVED_SELECT_4R_TEST_ONLY`.

This directory contains an isolated plaintext reference derived from Braverman,
Mao, and Weinberg (2016), plus a separate edge-list auditor and post-run
differential tools. It is not an author-exact Algorithm 5/7 reproduction, a
BB90 implementation, a secure Protocol I path, or a Top-K mask protocol.

The reference schedules at most four causal comparison rounds. A batch is
serialized and hashed before any result in that round is evaluated. The two
Appendix A sentinel partitions share the same R1/R2/R3/R4 clock and use
domain-separated random streams. `select4r.py` does not import an oracle or
sort the input to decide a pivot or output.

## Reproduce

From this directory in PowerShell:

```powershell
python -B run_one.py --scores 5,0,7,1,6,2,4,3 --k 3 --input-seed 17 --algo-seed 1 --out results/select4r_small_trace_seed1.json
python -B -m unittest -v test_select4r.py
python -B run_matrix.py --seeds 8 --small-max-n 5 --out-dir results
python -B audit_edges.py results/select4r_edge_traces.jsonl.gz --out results/select4r_independent_edge_audit.jsonl
python -B validate_results.py
```

The matrix retains every attempted seed. It includes all K values over every
strict-order permutation for n=1..5, eight seeds for each requested configuration
`(128,2)`, `(128,8)`, `(256,2)`, `(256,8)`, `(1000,80)`, and tie, all-equal,
signed-boundary, odd/even, endpoint-K, and non-power-of-two cases.

The edge auditor independently rebuilds each expected edge set from the frozen
R1/R2/R3/R4 stage descriptors, checks that a round uses only earlier-round
information, verifies comparison outcomes against the TEST_ONLY strict key,
and recomputes real-real, dummy-related, repeated, unique, and total call counts.
`validate_results.py` runs only after execution and compares candidates against
the S1 Python oracle and a temporary C++ harness using
`VFSS/include/moe_topk/topk_oracle.h`.

## Evidence boundary

`select4r.py` uses a project-derived r=1 `k1`, outward endpoint choices for the
two padded A5 calls, the residual rank `h-|R1_real|`, explicit q=0/q=|U|
handling, and assignment of z to Reject. Integer samples use exact ceil/floor
rules. R3 uses a project-derived factor 32 in its `m/|U|` sample scale; when
`|U|^2 <= 32m`, one R3 all-pairs batch exactly orders U at a linear cost.
These choices and their proof obligations are detailed in the dated S2
decision and validation documents.

Paper random-label branches remain visible as
`PAPER_RANDOM_FAILURE_PATH`. Missing pivots, crossed cuts, and out-of-range
residual ranks are reported as `UNDEFINED/INVALID_FINITE_CASE`. If a complete
candidate disagrees with the independent oracle, the validation record says
`COMPLETED_WRONG`. A failed seed is never retried or removed.

The SplitMix64 replay generator is deterministic and non-cryptographic. The
paper's success probability assumes ideal independent random tapes; observed
finite-run rates are empirical only.
