# M5 Protocol III → M6A interface and reproducibility handoff (2026-09-25)

## 1. Identity, status and receiving gate

Source branch: `feat/m5-protocol-iii-two-round`. Frozen code/review revision for this handoff: `3e8089d` (`docs(m5): add Protocol III independent final review`); this handoff and current-status corrections are uncommitted documentation on top of it. Confirm the actual receiving revision before a cross-party rerun. The source worktree was clean before this documentation task.

**M5 = IN PROGRESS; M5_READY_TO_CLOSE = NO; H2 REVIEW_VERDICT = FAIL for final M5 closeout.** The H2 review is [the controlling finding](../reviews/M5_PROTOCOL_III_INDEPENDENT_FINAL_REVIEW_2026-09-25.md). The M5-B–G core correctness, two-round Fselect/Fsort process causality, P2 online silence, and H1 order-of-magnitude communication acceptance remain valid within their stated boundaries. This document exposes reusable assets and a repeatable **candidate** interface. It is not the receiving party's rerun record and does not close G3 or authorize formal M6A acceptance.

Evidence classes: **PAPER_DIRECT** covers the cited paper definitions; **PAPER_DERIVED** covers direct algebraic consequences; **C-INSTANTIATION** covers the project's field, padded rank ring, encoding, transcript and process layout; **D-UNRESOLVED** covers author-exact transcript and unproved security/adapter properties. No project implementation detail is relabeled as paper-direct.

## 2. H2 closeout blockers: first-class handoff constraints

| ID | Description and why M5 is blocked | Current code status / missing work | Owner stage | Can M6A proceed without it? |
| --- | --- | --- | --- | --- |
| F1 | Current [project](../../PROJECT.md) and [implementation plan](../IMPLEMENTATION_PLAN.md) require a secure input/output and representation path, measured adapters, and the original-order shared Top-K bit-mask. A two-round selected or sorted field-record core is a different endpoint. | `ProtocolIIITwoRoundParty` takes already-field-shared encoded records alongside priority-key shares; no secure ring-to-field adapter or standard-output mask adapter exists for this path. H1 excludes their cost. H2 found no arithmetic/two-round core bug. | **M5-FIX** before M5 closure and formal M6A baseline acceptance. | **Infrastructure preparation only.** No final end-to-end I/III comparison or M5 completed label. |
| F2 | M5 G3 requires a frozen revision, minimal invocation, material/metric contracts and an independent receiving-party rerun. A completed M5→M6A handoff was absent. | This document supplies a provisional contract and commands. A receiving party still must build/rerun on the frozen revision, record results, accept the resolved F1 interface and sign off. | **M5-FIX/G3**, after F1's interface/cost boundary is fixed. | **Read-only design and benchmark harness preparation only.** This document alone does not satisfy G3. |

H2 **F3 is a documented limitation, not an additional blocking finding by itself**: no secure key/record consistency proof, formal field-DPF wrapper proof, persistent replay database, portable native FSS-key format, common-mask optimization, or author-exact transcript. The exact Theorem 4.2 cost mismatch (`254n` logical bits and 2× shared-uCMP DCF-call term) is nonblocking under the explicit H1 quantity-scale criterion, provided it is never described as exact. The deferred ring-to-field **adapter and cost** are part of F1; do not misclassify them as merely F3.

**Gate classification:** F1 and F2 = `MUST_FIX_BEFORE_ANY_FORMAL_M6A` under the current plan, and certainly before final M6A comparison. Separately, benchmark schema/runner preparation can occur now with no M6A implementation or formal result claim. Exact-cost reduction and F3's formal proof are not prerequisites created by H2.

## 3. M6A scope and fixed project semantics

Current [M6A plan](../IMPLEMENTATION_PLAN.md) and [benchmark plan](../BENCHMARK_VALIDATION_PLAN.md) require **both** Protocol I+AAV86 and Protocol III+AAV86, including correctness, independent-process security/causal review and complete performance acceptance. The unified application input is `n` additive shares of signed Q20.12 32-bit scores; output is a length-`n`, original-input-order secret-shared Top-K bit-mask with exactly K ones. Priority is score DESC then original index ASC; rank 0 is highest priority. A selected payload or rank-order sorted record is not that output.

AAV86 uses adaptive comparison graphs, not the fixed clique. An offline-only P2, exact-edge material timing, stage/edge binding and disclosed local-rank/bucket semantics need new arguments. Protocol I's `2r+1` theorem route does not prove Protocol III's team `2r` combination target. The full M6A matrix is `n=128,256; K=2,8; r=2..5` plus `n=10^3..10^6; K=80; r=2..5`, subject to explicit per-configuration success/failure/resource records. LAN/WAN, one warmup and five formal repetitions, median/min/max and all applicable metrics belong to later M6A acceptance, **not this handoff**.

## 4. Frozen reusable assets and exact reuse status

`REUSE_DIRECTLY` means the stated frozen behavior can be called as-is at its existing boundary; it does **not** mean it already supports an adaptive graph. `REUSE_WITH_ADAPTER` requires a separately reviewed new boundary. `REFERENCE_ONLY` is a correctness/communication control, not the M6A algorithm. `BLOCKED` cannot be used as a final accepted M6A route until F1/F2 are resolved.

| Symbol / file | Purpose; input → output contract | Evidence / tests | M6A reuse status |
| --- | --- | --- | --- |
| `protocol_i_cmpagg_eval_party`, `ProtocolIUcmpPartyMaterial`; `VFSS/include/moe_topk/protocol_i_cmpagg.h`, `protocol_i_ucmp.h` | Shared clique DCF/uCMP evaluator: party ID, public masked keys and ordered one-shot edge material → logical original-order rank shares. | M2 uCMP/CmpAgg, M5-B differential, H2 call-chain audit; raw rank stays secret. | `REUSE_WITH_ADAPTER` for adaptive graph/edge material; `REUSE_DIRECTLY` for fixed-clique controls. Do not copy ranking. |
| `ProtocolIPartyPackage`; `protocol_i_party_package.h` | Per-party node mask shares and edge materials, session/fingerprint/shape/width bound. | M2 package and M5-B/C/E/F tests; native FSS-key ABI has same-build/architecture limit. | `REUSE_WITH_ADAPTER` for dynamic graph, edge/stage binding. |
| `protocol_iii_grank_party`, `ProtocolIIIGrankOutput::rank_additive_shares`; `protocol_iii_grank.h` | Priority-key shares and local package → `logical_n` original-item-order shares in `Z_(2^rank_bits)`; no padded ranks. | M3 GRank and M5-B/C; H2 shared-evaluator review PASS. | `REUSE_WITH_ADAPTER` if graph rank aggregation changes; existing fixed-clique path `REFERENCE_ONLY`. |
| `protocol_iii_secure_core_party`; `protocol_iii_secure_core.h` | Padded priority-key shares → original-order XOR Top-K mask, three causal core rounds. | M3/M5-C modular differential and three-process E2E. | `REFERENCE_ONLY` for mask correctness/round-cost control; not two-round AAV86. |
| `protocol_iii_raw_score_pipeline_party`; `protocol_iii_raw_score_pipeline.h` | Raw Q20.12 score shares → original-order XOR mask in five total rounds. | Historical M3 raw-score tests/process E2E. | `REFERENCE_ONLY`; its ring conversion does not implement the new field adapter. |
| `ProtocolIIIField`; `protocol_iii_field.h` | Canonical `F_(2^127-1)` element, add/sub/mul/inv, 16-byte big-endian encoding. | M5-D arithmetic and sanitizer; H2 arithmetic review PASS. | `REUSE_DIRECTLY` for field algebra. |
| `protocol_iii_encode_payload_nonzero`, `protocol_iii_pack_key_payload_nonzero`; `protocol_iii_field_payload.h` | Dealer/controller-level uint64→nonzero field encoding; packed key width 1–62 plus full uint64 payload; no share-domain conversion. | M5-D payload tests, zero/max/random. | `REUSE_DIRECTLY` for already-field-shared records; `BLOCKED` as a secure ring-share adapter. |
| `protocol_iii_field_dpf_generate/eval/full_eval`; `protocol_iii_field_dpf.h` | Party-local tree key and field correction: point→beta, otherwise 0; FullEval returns `2^rank_bits` local leaves. | M5-D/G conformance and H2 implementation-level structural review; **no formal wrapper proof**. | `REUSE_WITH_ADAPTER` for an AAV86 routing proof/domain; fixed-clique control can reuse directly. |
| `protocol_iii_two_round_preprocess`, `ProtocolIIITwoRoundParty`; `protocol_iii_two_round.h` | P2 offline pair → each party's local state; priority-key and field-record shares → one Fselect field-record share after R1/R2. | M5-E/F, H2 process/causal review PASS; caller enforces key/record consistency. | `REFERENCE_ONLY` as fixed-clique two-round core; AAV86 composition needs independent design. |
| `ProtocolIIITwoRoundParty::consume_round2_sort`; same header | Same R1/R2 input/material → `logical_n` rank-order field-record shares by local FullEval. | M5-G unit/process differential; no extra online frame. | `REFERENCE_ONLY` as full-sort core control and possible adapter input; not a Top-K mask API. |
| `protocol_iii_two_round_serialize_bundle/deserialize_bundle`; `protocol_iii_two_round_package.h` | Per-party offline one-shot material envelope, session/fingerprint/party/material ID/shape/width bound. | M5-F bundle/process malformed tests; embedded FSS keys not cross-platform canonical. | `REUSE_WITH_ADAPTER` for AAV86 stage/graph binding; direct for current control reruns. |
| `protocol_iii_two_round_party`, process controller; `protocol_iii_two_round_process_e2e_test.cpp` | Two framed P0/P1 exchanges with P2 exited before input release; test controller alone checks oracle. | M5-F/G process E2E, H2 causal review PASS. | `REUSE_WITH_ADAPTER` for M6A process harness; keep controller input preparation marked TEST_ONLY. |
| `protocol_iii_communication_benchmark.py` and `--comm-benchmark` process mode | Ten sizes, two repetitions, Fselect/Fsort logical/wire/offline consistency; no latency/PRG benchmark. | H1 80 rows; H2 16-row independent spot check; exact cost gap recorded. | `REUSE_DIRECTLY` for fixed-core communication regression; `REUSE_WITH_ADAPTER` for M6A measurement fields. |
| `ProtocolIIITwoRoundMetrics`, `ProtocolIIIMetricsObservation`; respective headers | Per-round sent/received bytes and logical bits; M3 record schema has NOT_MEASURED fields. | Process accounting and M3 metrics tests; online timing/PRG/peak memory incomplete. | `REUSE_WITH_ADAPTER` to add validated time, PRG, graph and adapter-stage counters. |
| M5 decisions/reproduction notes/H2 review | Paper mapping, process and cost provenance, limitations. | H2 independent review at `2f595cc`; current handoff base `3e8089d`. | `REFERENCE_ONLY` as evidence; receiving-party rerun remains missing. |

**Protocol I frozen entrypoint for this handoff:** `protocol_i_parallel_shuffle_three_round_party` in `protocol_i_parallel_shuffle.h`, exercised by `moe_topk_m2_parallel_shuffle_process_e2e_test` (also `--comm-benchmark-128`) and documented in [M2 communication evidence](../reproduction/M2_PROTOCOL_I_3ROUND_ONLINE_COMMUNICATION_UBUNTU_2026-09-24.md). It is a three-round **C-INSTANTIATION**, not the unpublished author transcript. The older raw-score→original-mask eight-round engineering reference is `moe_topk_m2_protocol_i_modular_e2e_test`, gated by `MOE_TOPK_ENABLE_EMP_OT`; the current Debug and Release CMake caches set that option OFF, so that executable is **not available in these builds**. Do not label either current Protocol I core test as a complete unified M6A benchmark.

## 5. Protocol III primary and reference paths for M6A

- **Available core controls (both required, different functions):** Fselect through `ProtocolIIITwoRoundParty::consume_round2` / `moe_topk_m5f_two_round_process_e2e_test`; Fsort through `consume_round2_sort` / the same executable's `--sort-controller`. Fsort is the nearer full-order core control; Fselect is a single target/order-statistic control. Measure and label them separately. Neither is the final original-order Top-K mask or an AAV86 graph implementation.
- **Reference paths:** `agarwal_protocol_iii_modular_3round` and `moe_topk_protocol_iii_raw_score_modular_5round` executables, plus the M5-C tests. They yield the frozen original-order mask and are regression/semantic controls, not substitutes for a compressed field route.
- **Formal M6A primary path:** **NOT_AVAILABLE YET**. It must be a separately audited Protocol III+AAV86 composition with adaptive graph, secure representation/output adapters, original-order mask, and full-path cost. Do not silently benchmark the fixed-clique Fsort/Fselect core as if it were that path.

## 6. Input, output, and adapter contracts

| Boundary | Current contract | M6A interpretation |
| --- | --- | --- |
| Two-round core input | `padded_n` additive priority-key shares plus `logical_n` already-field-shared nonzero encoded `(key,uint64 payload)` records; key meanings agree by caller precondition. Public session, fingerprint, party, `logical_n`, `padded_n`, K, target, comparison/rank widths. | `RING_TO_FIELD_ADAPTER=DEFERRED`; `INPUT_CONSISTENCY_PROOF=NOT_IMPLEMENTED`. Test-controller clear-to-share generation is **BENCHMARK_INPUT_PREPARATION**, never a secure raw-score/ring-share conversion. |
| Fselect output | One additive `F_(2^127-1)` share of selected encoded record at public target rank. | Single order-statistic core only; decode/reconstruct solely in TEST_ONLY controller. |
| Fsort output | `logical_n` additive field-record shares in rank order, highest priority first; no padded slots. | Full-sort core only; membership of first K can be checked in TEST_ONLY, but this is not a secure original-order mask adapter. |
| M3 three-/five-round reference output | `logical_n` original-input-order XOR Top-K mask shares. | Regression oracle-compatible reference with different representation and round boundary. |
| Unified M6A output | `logical_n` original-order secret-shared Top-K bits, exactly K ones, from raw signed Q20.12 score shares. | **OUTPUT_ADAPTER_GAP=OPEN** for current field two-round path. Its secure cost and causal rounds are `NOT_MEASURED`. |

Minimal semantic example for a receiving-party rerun: take five equal signed Q20.12 scores in original order and public `K=2`. Stable project ranks are `[0,1,2,3,4]`; Fselect at public target rank 2 must reconstruct the encoded record of original item 2, while Fsort must reconstruct the five records in index order. The **unified** original-order Top-K mask oracle is `[1,1,0,0,0]`. Only the isolated test controller may reconstruct these values; the current field core does not securely emit that mask. The process executable above runs fixed internal cases rather than accepting arbitrary clear scores on its CLI.

Priority-key comparison widths, packed field key widths, `Z_(2^rank_bits)` rank-domain padding, and 16-byte canonical field serialization must be explicit in any adapter. Raw rank, raw payload, selected index and final mask are not opened by production parties. P2 remains input-independent and online silent. Material is one-shot, party/session/shape/fingerprint bound; same-build FSS-key serialization and controller-managed replay uniqueness remain explicit limitations.

## 7. Benchmark reuse and future measurement contract

The H1 [long-term report](../reproduction/M5_PROTOCOL_III_2ROUND_ONLINE_COMMUNICATION_UBUNTU_2026-09-25.md) and driver cover `n=2,3,4,5,8,16,20,32,64,128`, two deterministic repetitions per mode/target, 80 independent-process rows. For the fixed-clique two-round core, across both online parties: `R1=2n(ell_prime+254)`, `R2=2n(rank_bits+127)` logical bits; wire `368+128n` bytes; offline serialized party bundles measured separately. Fsort adds zero online communication to Fselect at the same n. Implementation/paper unoptimized ratio is 1.42–1.44; exact delta `254n` bits. `ORDER_OF_MAGNITUDE_COMM_MATCH=YES`; `EXACT_THEOREM_4_2_COST_MATCH=NO`.

**These are communication counts, not runtime performance results.** M6A needs distinct offline generation/distribution time, per-phase online critical-path latency (R1/R2 and adapters), total time, actual online PRG calls, DCF/uCMP/DPF/FullEval local work and time, graph `e_A(n,r)` and `v_A(n,r)`, wire bytes, logical bits, offline material, causal rounds, peak memory if collected, seeds, environment and correctness. Protocol I and III stage definitions differ; do not force their timings into identical phase names without a mapping. `NOT_MEASURED` is required until a verified counter exists. Do not copy H1 wire tables into a runtime field.

For comparable results, use the same machine, compiler/build type, n/K/payload width, transport/network conditions, process isolation, input distributions/seeds, repetition/warmup policy and metric boundaries. Compare I vs I+AAV86 and III vs III+AAV86 within the same full input/output functionality before cross-route I+AAV86 vs III+AAV86. Fselect vs Protocol I full Fsort or raw-score→mask is **not** a whole-protocol comparison. Split input adapter, graph/core, output adapter, serialization and framing; retain per-party sends/receives, count total as sends only. For each configuration, keep success, failure, resource-limited and not-run distinct.

## 8. Minimum reproducibility commands and regression gate

Existing local builds use `MOE_TOPK_ENABLE_EMP_OT=OFF`; report that optional Protocol I target limitation. At source revision `3e8089d`, from the repository root:

```bash
cmake --build build-vfss-debug --target moe_topk_m2_parallel_shuffle_process_e2e_test moe_topk_m5f_two_round_process_e2e_test agarwal_protocol_iii_modular_3round moe_topk_protocol_iii_raw_score_modular_5round -j4
ctest --test-dir build-vfss-debug --output-on-failure
./build-vfss-debug/moe_topk_m2_parallel_shuffle_process_e2e_test
./build-vfss-debug/moe_topk_m5f_two_round_process_e2e_test
./build-vfss-debug/moe_topk_m5f_two_round_process_e2e_test --sort-controller
ctest --test-dir build-vfss-debug --output-on-failure -R 'moe_topk_m5h1_communication_benchmark_smoke_test'
```

For a fresh **Release communication** rerun (not a performance latency run):

```bash
cmake --build build-vfss --target moe_topk_m5f_two_round_process_e2e_test -j4
python3 VFSS/tests/moe_topk/protocol_iii_communication_benchmark.py ./build-vfss/moe_topk_m5f_two_round_process_e2e_test /tmp/m5h1-protocol-iii-comm-release-verified.csv
```

The H2 [independent review](../reviews/M5_PROTOCOL_III_INDEPENDENT_FINAL_REVIEW_2026-09-25.md) reports **39/39 Debug CTest PASS**, **10/10 ASan/UBSan PASS** (`detect_leaks=0`; memory/UB checks active) and a four-scale, 16-row independent communication spot check. Those are H2 observations at reviewed commit `2f595cc`. On this documentation task at base `3e8089d`, the four-target Debug build command above exited 0 (`ninja: no work to do`), and a local `ctest --test-dir build-vfss-debug --output-on-failure` rerun passed **39/39** (exit 0, 30.68 s). This self-rerun is not a receiving-party G3 sign-off. The M6A precondition regression suite is the full CTest gate: Protocol I frozen tests, M3 GRank/routing/secure combine/raw-score/process, M5-B/C interface/modular, M5-D field/DPF, M5-E/F Fselect/package/process, M5-G FullEval/Fsort/process, and the H1 smoke target. Optional EMP-dependent Protocol I tests must be reported as unavailable in the current build, not passed. Add the relevant sanitizer subset when the environment supports it.

**Receiving-party G3 record still required:** record checkout SHA, build flags/dependencies, exact commands and exit codes, one small input/target example with expected output, party-local package validation, one-shot and session binding, metric boundaries, H2 F1 resolution revision, and any failures. A self-rerun by the delivering agent is not that record.

## 9. M6A allowed work and forbidden assumptions

Allowed **now as preparation only**: read-only AAV86 source/paper audit; compare graph and CA contracts; design/adapt benchmark schema and harness without publishing performance conclusions; prepare fixed-seed test matrices, failure-state reporting, reusable transport counters and new-stage material-binding tests. Do not begin formal AAV86 runtime/benchmark acceptance until the current plan's M5/G3 prerequisites are met.

Do not infer that a complete-clique preprocessing generator supports adaptive exact-edge graphs, that Protocol I shuffle belongs in Protocol III, that M3 ring low-bit mask conversion works in the field, that test-controller clear sharing is a secure input adapter, or that a rank-order record vector is an original-order mask. Do not treat H1's order-of-magnitude acceptance as exact cost, or the project two-round candidate as an author binary. Do not silently change stable ties, output semantics, roles, leakage, timing boundary or metric units.

## 10. M5-FIX recommendation and next ownership

`M5_FIX_STAGE_REQUIRED=YES` under the current accepted plans. Minimum scope: (1) secure raw-score/ring-share-to-field input path with explicit key/record consistency contract and isolated test oracle; (2) secure field Fselect/Fsort-to-original-order shared Top-K mask adapter, preserving ties and exactly K bits; (3) full-path rounds, logical/wire/offline/time accounting with input/output adapter stages separate; (4) same-input differential against M3 and Protocol I plus independent P2/P0/P1 E2E and failure/material tests; (5) update current plan/status if the accepted boundary changes; (6) after F1 is resolved, a genuine receiving-party rerun and sign-off for F2. Do not implement these items in this handoff task.

Next branch recommendation: **do not invent or create a new M6A branch yet**. Keep the current M5 branch for the F1/F2 resolution or open a short-lived M5-FIX branch from a reviewed checkpoint according to the repo workflow; once M5 G3 is actually accepted, start a separate M6A branch from the then-current main. This handoff plus [new-chat prompt](M6A_NEW_CHAT_PROMPT_2026-09-25.md) prepares that work but does not itself change Git or start M6A.

