# TEST_ONLY BMW16 source audit

Current identity: BMW16_DERIVED_SELECT_4R_TEST_ONLY_AUDIT_NOT_IMPLEMENTATION.

This directory does not contain a runnable Algorithm 5, Algorithm 7, or Select implementation. The paper-text audit found unresolved pseudocode steps, so no comparison scheduler or comparison counter was written. The code is limited to:

- a frozen plaintext stable Kth/top-K oracle, which imports no algorithm;
- arithmetic for the Appendix A padding/set-intersection reduction;
- finite-parameter witnesses for the Algorithm 5 and Algorithm 7 source inconsistencies;
- unit checks for that arithmetic and the oracle.

Run the checks from this directory:

    python -B -m unittest -v test_audit.py

Emit the requested parameter audit matrix:

    python -B audit_matrix.py

The matrix contains no input seed, algorithm seed, selected item, comparison edges, measured counts, or oracle differential. Those fields are absent because no randomized Select algorithm was run. Do not interpret conditional logical edge-request formulas as actual comparisons.
