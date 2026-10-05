# BMW16-derived Select audit: validation report

Date: 2026-10-05

Decision: **NO-GO for the complete runnable Select reference**

Current code identity: **BMW16_DERIVED_SELECT_4R_TEST_ONLY_AUDIT_NOT_IMPLEMENTATION**

Worktree branch: **codex/bmw16-derived-select-4r**

## 1. Result summary

The source audit verified the 2016 paper’s noiseless model, Theorem 8, Algorithm 5, Algorithm 7, and Appendix A Lemma 1 against the local source PDF. The theorem states a four-round O(n) exact median Partition with probability at least 1−exp(−Ω(n^ε)), for 0<ε<1/18. Algorithm 7 writes the larger closed epsilon interval, so the audit uses ε=1/36.

The arbitrary-K set algebra is now explicit for project Kth-highest: convert to ascending rank r=n−K+1, then use two parallel sentinel Partition instances padded to the same even median size M=2 max(r,n+1−r)≤2n. On two successful partitions, accepted-in-low-sentinel intersect rejected-in-high-sentinel returns the unique target. Independent random tapes support the paper’s p² probability relation.

The full algorithm did not pass source closure. Algorithm 5’s printed initialization can request a pivot that cannot exist; Algorithm 7’s literal third-round target can be negative on exact first-two-round partitions. No Select algorithm, comparison edge list, random seed run, oracle differential, comparison count, or mask was produced. The scripts and JSON records intentionally report that result instead of filling missing data.

## 2. Worktree and protection record

The isolated task worktree was created from verified current main. At start, local main, origin/main, and HEAD all resolved to c3926c68fd14f270faa8b55234311071947fa080. Work used only branch codex/bmw16-derived-select-4r, which was clean before task changes. No data, code, or result was imported from another worktree.

Snapshot of the twelve existing worktrees at inspection:

| Path alias | Branch | HEAD | Pre-existing changed paths |
| --- | --- | --- | ---: |
| Desktop primary | feat/m6a-performance-evaluation | c3926c68fd14f270faa8b55234311071947fa080 | 8 |
| .codex/worktrees/61dd | detached | b521c5e33c4fef1a37f33a63586bb6a201520c5a | 0 |
| BMW16 task worktree | codex/bmw16-derived-select-4r | c3926c68fd14f270faa8b55234311071947fa080 | 0 before work |
| .codex/worktrees/e4c9 | codex/m6a-p2-i-e18-delivery | 57e239ee820a4208ac606663d877803f0b692206 | 0 |
| .codex/worktrees/f516 | codex/m6a-p2-iii-e2 | 6c98ff32497e4024394a2174548c80363ec985f1 | 2 |
| M6A E19 | codex/m6a-p2-i-e19-resource-matrix | 5caec909830daa881d42a4b0e9a7f3e097c7e88c | 0 |
| M6A E20 | codex/m6a-p2-i-e20-streamed-material | 8986a40175f5f8eef56d765fef0cfa863e205985 | 0 |
| M6A E21 | codex/m6a-p2-i-e21-unified-matrix | 93d96f6c9f9d5c758e5471ea38158f05f1c7dea4 | 9 |
| .codex/worktrees/m6a-p2-iii-e3 | codex/m6a-p2-iii-e3 | b8ca19379c21962674535e035f31320b3ca39631 | 0 |
| Desktop all-pairs experiment | codex/m6a-p2-i-allpairs-experiment | ac8af47a03031585f1848a6f4b7749d797b9ae93 | 15 |
| Desktop E11 benchmark worktree | codex/m6a-p2-i-e14-metrics | b521c5e33c4fef1a37f33a63586bb6a201520c5a | 0 |
| Desktop E15 worktree | codex/m6a-p2-i-e15 | 6a9ef8447cfd6b74c17f9a5645eedebcb8a6ea72 | 0 |

The Desktop primary worktree’s pre-existing eight paths were: modified PROJECT.md, docs/IMPLEMENTATION_PLAN.md, docs/PAPERS.sha256, docs/REFERENCE_MANIFEST.md; untracked M6A decision docs dated 2026-09-27 and 2026-09-28; untracked M6A source inventory dated 2026-09-27; and untracked siamjdiscrmath.pdf. Those changes remain untouched.

E21’s pre-existing nine paths were PROJECT.md, docs/BENCHMARK_VALIDATION_PLAN.md, docs/IMPLEMENTATION_PLAN.md, its E21 decision and delivery report, and four E21 TEST_ONLY scripts. The all-pairs desktop worktree’s 15 pre-existing paths remain untouched. The E20 worktree was clean. PR #28 was observed as Draft on branch codex/m6a-p2-i-e18-delivery and was not modified. No existing worktree or PR was checked out for this task.

## 3. Paper identity and inspection

| PDF | Pages | SHA-256 | Role |
| --- | ---: | --- | --- |
| Papers/1603.04941v1.pdf | 44 | F46F83CBA279F37E9E3AAA0D64B145FDFCFBC52C0C9C8B79BACEC1259CA9CA23 | Primary 2016 paper source |
| Papers/017.pdf | 249 | 3EAE3A93F12AB71EEA8AC040167F21351FCF3FF54DC423985A04087E3E90CB15 | 2018 Jieming Mao dissertation; related-author cross-check only |

The primary source pages checked were PDF p.7 for model/notation, p.11 for Theorem 8, p.29 for Algorithm 5, pp.33–34 for Algorithm 7 and its proof, and p.16 for Appendix A Lemma 1. Source-page raster checks confirmed the equations and the epsilon endpoint. The auxiliary dissertation’s title page identifies Jieming Mao and September 2018; Chapter 6 gives a high-level four-round theorem, but it is not independent evidence and does not resolve the numbered pseudocode mismatch.

The local paper parser status check found Docling and Marker available. Docling parsing ended with std::bad_alloc after partial extraction; Marker had processed 2 of 44 pages after roughly two minutes and was stopped. The cited lines were therefore checked using PyMuPDF text extraction and rendered source-page images. The source PDFs were not modified or added to Git. Their local-only status is confirmed by the repository Papers ignore rule; only the SHA entries were added to docs/PAPERS.sha256.

## 4. Environment, files, commands, and raw result hashes

Environment: Windows 11, Python 3.13.7, 64-bit CPython.

Added and modified files:

- docs/decisions/BMW16_DERIVED_SELECT_4R_SOURCE_AND_COMPOSITION_DECISION_2026-10-05.md — evidence tables, rank algebra, four-round conditional schedule, finite-size gaps, conditional cost, stable key, and Protocol I design draft.
- docs/reproduction/BMW16_DERIVED_SELECT_4R_VALIDATION_2026-10-05.md — this report.
- experiments/TEST_ONLY_BMW16_DERIVED_SELECT_4R/README.md — explicit audit-only scope.
- experiments/TEST_ONLY_BMW16_DERIVED_SELECT_4R/oracle.py — frozen TEST_ONLY stable kth/top-k oracle.
- experiments/TEST_ONLY_BMW16_DERIVED_SELECT_4R/audit_reference.py — deterministic source-parameter audit; no comparisons or Select output.
- experiments/TEST_ONLY_BMW16_DERIVED_SELECT_4R/audit_matrix.py — five-request parameter matrix.
- experiments/TEST_ONLY_BMW16_DERIVED_SELECT_4R/test_audit.py — tests for oracle semantics, arbitrary-K algebra, endpoints, and the two paper-literal witnesses.
- experiments/TEST_ONLY_BMW16_DERIVED_SELECT_4R/results/source_audit_matrix_2026-10-05.jsonl — raw five-record parameter audit.
- experiments/TEST_ONLY_BMW16_DERIVED_SELECT_4R/results/test_audit_2026-10-05.txt — raw unit-test output.
- experiments/TEST_ONLY_BMW16_DERIVED_SELECT_4R/results/smallest_input_static_audit_2026-10-05.json — n=1,K=1 finite-structure diagnostic.
- PROJECT.md, docs/IMPLEMENTATION_PLAN.md, docs/REFERENCE_MANIFEST.md — dated scope/source-status notes only.
- docs/PAPERS.sha256 — the two local PDF hashes.

Commands, run from experiments/TEST_ONLY_BMW16_DERIVED_SELECT_4R:

    python -B audit_matrix.py | Tee-Object -FilePath .\results\source_audit_matrix_2026-10-05.jsonl
    python -B -m unittest -v test_audit.py 2>&1 | Tee-Object -FilePath .\results\test_audit_2026-10-05.txt
    python -B audit_reference.py --n 1 --k 1 | Tee-Object -FilePath .\results\smallest_input_static_audit_2026-10-05.json

All 9 source-algebra/oracle unit tests passed. These tests do not run or certify BMW16 Select. They exhaustively check the two-sentinel padding identity for every 1≤n<32 and 1≤K≤n; test stable ties and signed int32 endpoints; and assert both the Algorithm 5 undefined-pivot witness and the Algorithm 7 negative-rank witness. The n=1 audit reports a negative idealized base-set cardinality and refuses to produce a candidate V size.

The matrix covers (128,2), (128,8), (256,2), (256,8), and (1000,80), all with ε=1/36. For example, (128,2) maps to ascending rank 127, high padding 125, and median input M=254; D=47 makes the A5 input 348, where literal k1=174 but |A1|=50. Algorithm 7 then has U size 94 and literal line-11 rank −47 on the exact-partition cardinalities. The matrix’s edge counts are explicitly labeled conditional logical requests; actual and duplicate edge counts are NOT_MEASURED because no algorithm-generated edge list exists.

Raw result SHA-256 values:

| Result file | SHA-256 |
| --- | --- |
| source_audit_matrix_2026-10-05.jsonl | 9EA07195C8960F0CDBCCF37256BE69DE88654B70516DAB99929D07ED612B83E8 |
| test_audit_2026-10-05.txt | F72CECD343981CD95378C111A9222418D799CA92C0CDB70DCF38EE2D177C169D |
| smallest_input_static_audit_2026-10-05.json | A7C5BE59E26EA243F210B8FD5E97FAD7A551F05B9FBE45ABEB7BA90B54D5F56F |

No official performance matrix, random-seed Select run, independent edge-list causality checker, oracle differential for an algorithm result, Protocol I runtime, DCF, or secure mask was run. No measured comparison count is inferred from a complexity formula.

## 5. Conditional complexity and four-round claim

Source claim: Theorem 8 gives Algorithm 7 four comparison rounds and O(m) comparisons for median Partition with probability at least 1−exp(−Ω(m^ε)). The Appendix conversion creates M≤2n and two parallel Partition calls. Thus, **if** Algorithm 7 is implemented according to a corrected, fully specified proof, the derived Select has four shared comparison rounds, two copies of the Partition comparison work, and O(n) asymptotic comparison complexity. Its two success events give at least p² with independent algorithm tapes.

This is not an implementation result. The current source-literal control flow does not define all pivots/ranks and the paper leaves finite-size integerization implicit. No four-round execution or actual O(n) counter has been observed.

## 6. Seven gates

| Gate | Status | Evidence |
| --- | --- | --- |
| PAPER_SOURCE_AND_REDUCTION | PARTIAL / NO-GO overall | Source pages and two-sentinel / arbitrary-K algebra are traced and checked; Algorithm 5/7 source steps remain unresolved. |
| FOUR_ROUND_SCHEDULE | CONDITIONAL_ONLY | R1–R4 causality and parallel instances are laid out; no runnable Select schedule exists. |
| LINEAR_COMPARISON_BOUND | CONDITIONAL_PROOF_ONLY | Theorem 8 plus M≤2n and two calls gives O(n) if a repaired A7 is realized. Per-round actual counts and unique edges are NOT_MEASURED. |
| FINITE_N_RUNNABLE_REFERENCE | NO_GO | Integer rules and several branches are undefined; no Select implementation. |
| STABLE_KTH_ORACLE | ORACLE_TESTS_PASS / ALGORITHM_DIFFERENTIAL_NOT_RUN | The isolated oracle passes its tests; there is no algorithm output to compare. |
| PROJECT_EXACT_MASK | NOT_IMPLEMENTED | No mask generator or secure route was added. |
| SECURE_PROTOCOL_I_COMPOSITION | DESIGN_DRAFT / NOT_REVIEWED | Candidate key, inclusive DCF predicate, original-index preservation, inverse route, leakage, adaptive preprocessing, and separate round fields are documented. No message-level safety proof/runtime exists. |

## 7. Repository boundary check

The task worktree started at c3926c68fd14f270faa8b55234311071947fa080, equal to local main and origin/main, on codex/bmw16-derived-select-4r. Source PDFs are ignored local references. No changed path is under VFSS-baseline, Papers, key material, build output, logs, or local reference projects. The benchmark-validation plan was not changed because this is a failed source-closure/design stage, not a formal performance run.

Both git diff --check and git diff --cached --check passed after staging only the task files. The branch remains local and separate from main; no merge or push is part of this work.
