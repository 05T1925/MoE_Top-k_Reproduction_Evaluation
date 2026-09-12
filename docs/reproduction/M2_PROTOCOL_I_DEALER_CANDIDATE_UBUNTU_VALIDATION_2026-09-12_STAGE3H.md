# M2 Protocol I Dealer Candidate Ubuntu Validation - Stage 3H

Date: 2026-09-12  
Revision: `2492eca978c1c3989576673aaad2dc16b4fbb4d9`  
Branch: `codex/m2-candidate-ubuntu-validation`

## Executive conclusion

The candidate now passes its independent-process rank differential and the full configured Ubuntu EMP-ON CTest set. The Stage 3G failure was a TEST_ONLY oracle error for equal padded dummy keys, not a secure shuffled-slot or edge-material binding failure. The candidate remains a project extension and outputs shuffled-domain rank shares only.

## Slot lineage and model

| Field | Domain/slot | Permutation | Owner |
|---|---|---|---|
| priority-key share | original slot before R1 | payload enters two forward shuffles | P0/P1 |
| forward output | final shuffled slot after R2 | yes, composite secret permutation | P0/P1 |
| `r_share[index]` | final output slot | no; output-slot randomness | P2 package to P0/P1 |
| public `y[index]` | final shuffled slot | no after R2 | jointly opened |
| edge `(left,right)` | final shuffled comparison slots | no | P2 package to P0/P1 |
| rank share | final shuffled slot | no | P0/P1 |

`protocol_i_apply_permutation(pi,x)` is `out[i]=x[pi[i]]`; the two-party shuffle composition is consistent with the candidate test's `compose(p1,p0)`. This is Model A, output-slot `r`, not record-bound `r`. P2 does not need the permutation because it defines material by final slot index.

## Root cause and correction

The previous expected rank counted only strictly smaller keys. Padded dummy slots intentionally share one maximum priority key, while CmpAgg's stable tie behavior assigns equal keys in descending slot order. The TEST_ONLY expected-rank helper now models `other < current || (other == current && other_slot > current_slot)`. No secure implementation, permutation, package, or material code was changed for this correction.

## Validation

- Candidate package/core/executable: compiled and linked under Ubuntu 24.04 EMP-ON.
- Candidate independent-process Dealer/P0/P1 E2E: PASS; P2 package barrier precedes online execution and P2 exits before online input delivery.
- Candidate test: PASS, including identity, reverse, fixed-random permutations, duplicate/all-equal scores, logical/padded and non-power-of-two cases.
- M1/M2 directed regression: 21/21 PASS.
- M3/DPF/masked-multiply directed regression: 11/11 PASS.
- Full configured CTest: 32/32 PASS.
- BUILD_TESTING=OFF, EMP-OFF fresh graph: production `sytorch` built; target help contains no test/bench/candidate-test targets.
- `git diff --check`: PASS.
- Performance: `NOT_MEASURED`.

Actual candidate barriers remain R1 first forward Permute+Share, R2 second forward Permute+Share, and R3 masked shuffled-list opening: causal online barriers = 3. Trace validation and byte accounting pass through the candidate result schema.

## Security/protocol boundary

Static audit: no score/key/rank reconstruction in secure code, no rank-share exchange, selected-index opening, reverse shuffle, original-order mask, selection carrier, file polling, fixed sleep, online Dealer, or formal M2/M3 runtime modification. The candidate is not paper-exact Protocol I, not the complete Protocol I reproduction, and not an output-adapter implementation.

## Status labels and A-J answers

A. `r` domain: final shuffled output slot (PASS, static/code evidence).  
B. Edge material endpoints: final shuffled slots (PASS, static/code evidence).  
C. Failure root cause: padded-dummy tie oracle mismatch (PASS, differential evidence).  
D. Compile/link: PASS.  
E. Public-y/same-r: PASS by candidate E2E and slot contract.  
F. Identity/reverse/random: PASS in candidate matrix.  
G. Rank differential: PASS.  
H. Independent Dealer/P0/P1 E2E: PASS.  
I. Actual causal barriers: 3 (PASS).  
J. `CORE_RUNTIME_GO`: PASS for this candidate gate; `ADAPTER_ENTRY_GO`: BLOCKED because no selection-carrier/original-order mask; paper-exact gate: BLOCKED.

## Modified files

- `VFSS/CMakeLists.txt` (Stage 3G target/test isolation changes retained).
- `VFSS/ext/bitpack/CMakeLists.txt` (Stage 3G test gating/link changes retained).
- `VFSS/ext/bitpack/src/bitpack/bitpack.cpp` (Stage 3G build fix retained).
- `VFSS/tests/moe_topk/protocol_i_dealer_candidate_test.cpp` (frozen rank direction, padded-dummy tie oracle, TEST_ONLY failure diagnostics).
- `docs/reproduction/M2_PROTOCOL_I_DEALER_CANDIDATE_UBUNTU_VALIDATION_2026-09-12_STAGE3G.md` (pre-existing Stage 3G report, not rewritten).
- This Stage 3H report.

No new commit was created. No baseline, reference, paper, key, build, or log files were modified or added to the repository.

## Commands/evidence

Ubuntu commands used: EMP-ON build of all targets in `/tmp/moe-stage3g-on2`; candidate CTest; M1/M2 regex CTest; M3 regex CTest; full CTest; and fresh EMP-OFF `/tmp/moe-stage3h-off` configure/build/target-help checks. Raw command logs were kept under `/tmp/stage3h-*.log` during validation and are not repository artifacts.

## Remaining blockers / next stage

The candidate's rank-share output still requires a separately designed nonlinear rank-share-to-selection-carrier conversion and reverse/original-order mask adapter. That work must not be inferred from this candidate pass and should be handled as a separate design-gated stage.
