# TEST_ONLY BMW16 S5 receiver audit

`audit_s4_independent.py` performs a read-only receiver audit of S4's exact
`BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R_TEST_ONLY` source/evidence. It does not
import the S4 implementation or S1 oracle, does not change S4 files, and does
not create secure protocol code.

It regenerates the frozen official and small inputs; checks independent stable
Kth and Top-K answers; executes the retained repository C++ oracle harness;
reconstructs the n=8 trace's R1–R4 edge lists, comparison outcomes, derived
states and barriers; recomputes exact R1 hypergeometric tails and the S4
Chernoff/window bounds; and calculates optimistic plus conservative offline
one-shot comparison material slots using uint64 checked arithmetic without
allocating a pool. Raw input logs, results and the C++ executable remain in the
external `.codex/evidence` directory.

The audit also verifies a fresh 10,000-row rerun from the exact S4 commit
against the retained authoritative run. It compares all input, status, answer,
round-count, and edge-count fields after removing only the embedded run
manifest hash; that provenance field differs because the old archived run was
created before the S4 commit.

From the S5 worktree root:

```powershell
python -m py_compile experiments/TEST_ONLY_BMW16_S5_RECEIVER_REVIEW/audit_s4_independent.py
python experiments/TEST_ONLY_BMW16_S5_RECEIVER_REVIEW/audit_s4_independent.py `
  --evidence-root C:\Users\28641\.codex\evidence\BMW16_S4_2026-10-05 `
  --s4-source-root C:\Users\28641\.codex\worktrees\bmw16-s4\MoE_Top-k_Reproduction_Evaluation\experiments\TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R `
  --replay-root C:\Users\28641\.codex\evidence\BMW16_S5_2026-10-06\replay_official `
  --out C:\Users\28641\.codex\evidence\BMW16_S5_2026-10-06\independent_s4_audit.json
```

The JSON report distinguishes observed finite test results from ideal random
tape bounds and from secure Protocol I claims. It is not a cryptographic
simulator, benchmark, or secure runtime.
