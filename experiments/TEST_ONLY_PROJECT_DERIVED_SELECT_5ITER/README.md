# PROJECT_DERIVED_SELECT_5ITER_TEST_ONLY

This is an independent plaintext project algorithm. It is not BB90, a five-iteration BB90 variant, or a direct BMW16 implementation. The source specification and decision are in:

`docs/decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_DECISION_2026-10-12.md`

The algorithm accepts raw signed Q20.12 `int32` values and `1 <= K <= n`, uses the frozen stable order (signed score descending, original index ascending), runs four sampled pivot-window comparison layers and one exact candidate-clique layer, and returns both the stable Kth original index and a plaintext Top-K mask. Uniform sampling changes only the comparison cost. Every update preserves the target rank, and the final complete candidate graph makes correctness exact for every sample tape.

The output is **plaintext TEST_ONLY**. It is not wired into VFSS secure runtime, Protocol I, DCF, offline material generation, or network transport. The `input_seed` field records test-input provenance; `algorithm_seed` independently controls the reproducible TEST_ONLY sample tape. Python's seeded PRNG is not cryptographic randomness.

Run a single traced case:

```powershell
python experiments/TEST_ONLY_PROJECT_DERIVED_SELECT_5ITER/select5iter.py `
  --scores-json C:\temp\scores.json --k 4 `
  --input-seed 101 --algorithm-seed 202 `
  --trace C:\temp\select5iter_trace.json
python experiments/TEST_ONLY_PROJECT_DERIVED_SELECT_5ITER/audit_edges.py `
  C:\temp\select5iter_trace.json --out C:\temp\select5iter_audit.json
```

Run exhaustive frozen-oracle differential and independent edge audits. All generated build/test evidence is written to the required external `--out-dir`:

```powershell
python experiments/TEST_ONLY_PROJECT_DERIVED_SELECT_5ITER/test_select5iter.py `
  --out-dir C:\temp\PROJECT_DERIVED_SELECT_5ITER_EVIDENCE
```

The differential compiles a small adapter over `VFSS/include/moe_topk/topk_oracle.h`; it does not add generated binaries or results to the repository. Every valid case is written to `results.jsonl`, every failure retains its input, K, seeds, oracle row and trace when available, and invalid-input rejection checks are separate.
