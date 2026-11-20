# BMW16-derived Select S2: implementation, differential, and edge audit

Date: 2026-10-05
Result label: **`BMW16_DERIVED_SELECT_4R_TEST_ONLY`**
Decision: **A runnable four-round plaintext Select candidate is implemented and audited. Secure Protocol I composition remains review-only.**

## 1. Worktree, source, and environment

Work was performed only in `C:\Users\28641\.codex\worktrees\bmw16-s2\MoE_Top-k_Reproduction_Evaluation`, on `codex/bmw16-bmw16-s2`. The new worktree was created from S1 revision `0a0593deaccb55225566da6af3aac661814e717c`; its initial checkout was detached and clean before the branch was created. At start, local `main` and `origin/main` both resolved to `c3926c68fd14f270faa8b55234311071947fa080`. No content was imported from other experiment branches.

The S1 worktree and S1 reports remain unchanged. The S2 worktree is separate from the dirty Desktop primary workspace. E20 remained clean. E21's existing nine unrelated paths were observed and left untouched. The Desktop primary's existing eight paths were observed and left untouched. PR #28 was not checked out, changed, or updated; S1's report records its prior Draft state. No file changed under `VFSS/`, `VFSS-baseline/`, `Papers/`, any local reference tree, a key directory, or a build directory.

| Source | Role | Pages | SHA-256 |
|---|---|---:|---|
| `Papers/1603.04941v1.pdf` | Primary 2016 paper; Braverman, Mao, Weinberg | 44 | `F46F83CBA279F37E9E3AAA0D64B145FDFCFBC52C0C9C8B79BACEC1259CA9CA23` |
| `Papers/017.pdf` | Related-author 2018 dissertation; cross-check only | 249 | `3EAE3A93F12AB71EEA8AC040167F21351FCF3FF54DC423985A04087E3E90CB15` |

The theorem/algorithm page checks used physical PDF p.7 (comparison model), p.10 (A5 median-of-sample explanation), p.11 (Theorem 8), p.29–30 (Algorithm 5 and its proof), p.34 (Algorithm 7 and its proof), p.16 (Appendix A Lemma 1), and p.32 (dummy-shift observation). The dissertation is not counted as a separate proof source. The detailed line-by-line derivation, integer rules, and secure design blockers are in the [S2 decision](../decisions/BMW16_S2_DERIVED_SELECT_4R_DECISION_2026-10-05.md).

Environment: Windows 11 Home, build 10.0.26200, 64-bit; Python 3.13.7; MinGW-w64 g++ 8.1.0. The C++ oracle harness was compiled into a temporary directory and removed at process exit.

## 2. What actually ran

This stage ran the Select scheduler, all requested validations, and no formal LAN/WAN or secure benchmark.

| Validation set | Cases |
|---|---:|
| Exhaustive strict-order permutations, all K, n=1..5 | 719 |
| `(128,2)`, `(128,8)`, `(256,2)`, `(256,8)`, `(1000,80)`, eight independent replay seeds each | 40 |
| Repeated-score, all-equal, signed-boundary, odd/even, endpoint-K, non-power-of-two, and fixed trace case | 17 |
| **Total** | **776** |

Observed runtime classifications:

| Status | Runs | Handling |
|---|---:|---|
| `SUCCESS` | 570 | All 570 selected original indices matched the independent stable K-th oracle. |
| `PAPER_RANDOM_FAILURE_PATH` | 47 | All retained. 45 produced no unique candidate, one candidate matched, and one candidate was wrong. The wrong candidate was `official_n256_k8_seed1` (index 244; oracle index 169); it came from the source random-label failure branch and is preserved as a failure, not counted as a successful output. |
| `UNDEFINED/INVALID_FINITE_CASE` | 159 | All retained. Most were missing A5 R1 or R3 sample brackets; there was no retry, clamp, or extra comparison. |
| `COMPLETED_WRONG` on a declared success path | 0 | No nominal `SUCCESS` output disagreed with the oracle. |

The 40 official-matrix classifications were:

| Configuration | Successful and oracle-correct | Random-failure path | Undefined |
|---|---:|---:|---:|
| `(128,2)` | 5/8 | 3/8 | 0/8 |
| `(128,8)` | 3/8 | 5/8 | 0/8 |
| `(256,2)` | 2/8 | 6/8 | 0/8 |
| `(256,8)` | 3/8 | 5/8 | 0/8 |
| `(1000,80)` | 3/8 | 5/8 | 0/8 |

These rates are finite replay observations, not a numerical estimate or proof of the paper's asymptotic probability. In particular, all failed seeds remain in `select4r_failed_or_non-success_runs.jsonl`.

The Python S1 oracle and the C++ `VFSS/include/moe_topk/topk_oracle.h` harness agreed on the expected K-th original index and full Top-K mask for **776/776** inputs. The selector module imports neither oracle. All 570 declared-success outputs agreed with both references. The one wrong candidate from the paper random-failure path remains separately visible.

## 3. Independent round-edge audit and observed costs

`audit_edges.py` independently reconstructed the expected edge set for each frozen task descriptor, checked the plan digest, verified that every R1/R2/R3/R4 task used only results from earlier rounds, re-evaluated each edge against the TEST_ONLY strict key, and recomputed all counts without using the scheduler's count fields. It passed **776/776** traces.

Across all matrix traces:

| Counter | Observed total |
|---|---:|
| Comparison calls | 1,589,457 |
| Real-real calls | 483,553 |
| Calls with at least one dummy/sentinel endpoint | 1,105,904 |
| Unique unordered edges, summed per run | 1,374,383 |
| Repeated calls after a prior occurrence, summed per run | 215,074 |
| R1 / R2 / R3 / R4 calls | 207,672 / 207,790 / 1,172,921 / 1,074 |

The largest run used 124,661 calls (`n=1000`, boundary-score case). The largest observed per-run `calls/n` ratio was 134.96875, at `official_n256_k2_seed6`. Largest per-round observed counts over the matrix were R1 9,100, R2 8,852, R3 106,673, R4 171. These are observed TEST_ONLY counts, not a BB90 constant or a secure cost.

The proof uses the concrete integer rules in the S2 decision. With `M≤2n`, `D=ceil(M^(25/36))≤M`, and each A5 input `N=M+2D≤3M`, one A5 has at most `sp≤4N` R1 calls and `N−1` R2 calls. Four parallel A5 invocations therefore use at most `60M` calls. Each A7's R3 has at most `33M` calls (dense U costs at most `16M`; sampled U has `vu≤32M+u≤33M`), so both use at most `66M`. Both R4 graphs together cost at most M because `floor(M^(5/12))²≤M^(5/6)≤M`. Thus the full Select is bounded by `127M≤254n` comparison calls under these finite rules, including dummy comparisons and repeated requests. This coarse bound is a proof for this project construction, not a claim of BB90-equivalent constants.

## 4. Reproducible small trace

Input scores in original order: `[5,0,7,1,6,2,4,3]`; `K=3`; input seed 17; algorithm seed 1. The result was `original_index=0`, matching both oracle semantics. The outer expansion has `M=12`, `h=6`, and each A7 uses `D=6`. The trace has all four round plans, including an empty R4 batch because R3's all-pairs U rule completed the exact residual partition.

| Round | Calls |
|---|---:|
| R1 | 84 |
| R2 | 92 |
| R3 | 100 |
| R4 | 0 |
| **Total calls / unique unordered edges / repeated calls** | **276 / 179 / 97** |

The complete edge/result trace is `experiments/TEST_ONLY_BMW16_S2_SELECT_4R/results/select4r_small_trace_seed1.json`; its independent audit is `select4r_small_trace_edge_audit.json`. Re-run with:

```powershell
cd experiments/TEST_ONLY_BMW16_S2_SELECT_4R
python -B run_one.py --scores 5,0,7,1,6,2,4,3 --k 3 --input-seed 17 --algo-seed 1 --out results/select4r_small_trace_seed1.json
python -B audit_edges.py results/select4r_small_trace_seed1.json --out results/select4r_small_trace_edge_audit.json
```

The runtime uses a fixed SplitMix64 replay generator and domain-separated seeds for samples, the two sentinel partitions, and random-label branches. That generator is deterministic and not cryptographic; the theorem's independence statement assumes ideal independent random tapes. The validation record therefore treats the replay rates as empirical evidence only.

## 5. Commands and raw result identity

Commands were run from `experiments/TEST_ONLY_BMW16_S2_SELECT_4R`:

```powershell
python -B run_one.py --scores 5,0,7,1,6,2,4,3 --k 3 --input-seed 17 --algo-seed 1 --out results/select4r_small_trace_seed1.json
python -B -m unittest -v test_select4r.py
python -B run_matrix.py --seeds 8 --small-max-n 5 --out-dir results
python -B audit_edges.py results/select4r_edge_traces.jsonl.gz --out results/select4r_independent_edge_audit.jsonl
python -B validate_results.py
```

The C++ validation script compiled a TEST_ONLY harness with `g++ -std=c++17 -O2 -I <repo>/VFSS/include <this-dir>/oracle_harness.cpp -o <TEMP>/oracle_harness.exe`, then ran the 776 vectors in one process. Seven focused unit tests passed after the matrix run.

| Raw evidence | SHA-256 | Bytes |
|---|---|---:|
| `select4r_run_summaries.jsonl` | `ceaec6f180efb9729f24f459924d5701e58b7ede87817f1b2827d7683e55b53e` | 2,677,598 |
| `select4r_failed_or_non-success_runs.jsonl` | `c8f9478f3c921880111f993ba0d41e97139048e6c961ff6886de01c761358936` | 734,237 |
| `select4r_edge_traces.jsonl.gz` | `4c635eb600b2d5be9d336a52d157ae29a0975fe5415e95119535c080430d8b73` | 9,221,112 |
| `select4r_independent_edge_audit.jsonl` | `fcb4039b6d1eaf0cfec9eab90144a2e62a7ccb9a7ca9e41e8c25a590ea76b8d4` | 922,654 |
| `select4r_oracle_validation.jsonl` | `a62e559d881f1c920373ec7e791287a38572837f0995cb5c571b2f1f7303153f` | 378,903 |
| `select4r_oracle_validation_summary.json` | `82788582b285ea583fe16e79a61043091d09ad71964290c9b7dfd183727a666f` | 1,131 |
| `select4r_run_matrix_meta.json` | `cbdaeb851ea3ef7b166a5869edb4b5311005c68797f205fa872d824e84b949e2` | 701 |
| `select4r_small_trace_seed1.json` | `5cb150f025c62f95ee210687eb20d22a254393a6d56a0f8e3fb226855c939f79` | 150,955 |
| `select4r_small_trace_edge_audit.json` | `177fd60b5292dec801965d31c3fdce3035aa89f2ef1763f2c3dfee1c7c9f3b8b` | 1,149 |
| `select4r_unittest_2026-10-05.txt` | `4d853fd19e7d1141d5dabde8a4b00a57a3faca4a53866b83f3ee803e6c4efb90` | 1,084 |

The detailed matrix trace is compressed JSONL and includes every scheduled edge and result; its 9.2 MiB size is retained because it is the input to the independent audit. No compiler output, executable, paper, key, baseline, or local reference project is included.

## 6. Stage gates

| Gate | Result | Evidence / limit |
|---|---|---|
| `A5_R1_DERIVATION_PROVED` | **PASS (project-derived, r=1 only)** | S1 target is fixed to `floor(|S1|/2)` lower-count threshold; R1 reveals pivot ranks; missing sample bracket has an explicit failure state. No general-r A5 code. |
| `A7_R3_RANK_INVARIANT_PROVED` | **PASS under checked bracket premise** | For approximate A5 prefix cuts `c1≤h≤c2`, U is exactly the interval `c1+1..c2`; q=`h−c1`. Crossed cuts and out-of-range cases remain failures. |
| `A7_R4_BOUNDARY_AND_CARDINALITY_PROVED` | **PASS** | R* counts prior rejects among current A7 inputs, excludes A5 scratch dummies, and z is rejected; total rejects are h. |
| `FINITE_N_SPEC_COMPLETE` | **PASS as an execution/status specification** | Ceil/floor, empty U, oversize sample, bracket, random, and invalid paths are defined. This does not provide a finite numerical success probability. |
| `SELECT_4R_RUNNABLE` | **PASS** | CLI ran 776 vectors; frozen R1–R4 batches are present and independently audited. |
| `O_N_COMPARISON_PROVED_AND_COUNTED` | **PASS for this project rule** | Coarse bound `≤254n`; 1,589,457 measured calls across the fixed matrix, with dummy and repeated calls retained. No BB90 constant claim. |
| `STABLE_KTH_DIFFERENTIAL` | **PASS on declared-success outputs; failures retained** | 570/570 `SUCCESS` outputs match both oracles. One random-failure path emitted a wrong candidate and remains recorded. Python and C++ oracles agree 776/776. |
| `SECURE_PROTOCOL_I_DESIGN_GATE` | **READY_FOR_REVIEW only** | Interface and blocker table is in the decision document. No secure composition is implemented or approved. |

Exact mask is `NOT_IMPLEMENTED`. Protocol I security/composition is `NOT_REVIEWED`. The earlier S1 NO-GO remains a correct dated S1 result: at that revision, no runnable Select existed. This S2 result does not claim `BB90`, author-exact BMW16 pseudocode, secure protocol behavior, or every-run correctness on the probabilistic failure paths.
