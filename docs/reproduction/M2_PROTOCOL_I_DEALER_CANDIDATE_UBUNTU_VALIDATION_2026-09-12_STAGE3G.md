# Stage 3G: M2 Dealer Candidate Ubuntu Validation

## Executive conclusion

Stage 3G completed the build-isolation work and the rank differential root-cause audit. Ubuntu 24.04 EMP-ON configure and candidate compile/link succeed. The candidate independent-process test remains blocked at rank differential for non-identity permutations; this is a real permutation/record-binding defect, not an oracle-direction defect. Status remains `PARTIAL_RUNTIME_GO`, `CORE_RUNTIME_BLOCKED`, and `ADAPTER_ENTRY_BLOCKED`.

## Identity and scope

- Branch: `codex/m2-candidate-ubuntu-validation`
- HEAD at start: `2492eca978c1c3989576673aaad2dc16b4fbb4d9`
- Candidate label: `m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate`
- Formal M2 label unchanged: `m2_protocol_i_raw_score_input_modular_8round_mask_output`
- Candidate output remains only `public_masked_list` and `shuffled_rank_share`.
- No output adapter, selection carrier, reverse shuffle, M3 runtime change, or paper-exact claim was added.

## Environment

Fresh Ubuntu 24.04 WSL build directories were used. Observed tools: CMake/CTest 3.28.3, GCC/G++ 13.3.0, Eigen 3.4 CMake config, OpenSSL 3.0.13. Pinned EMP packages were discovered from `/tmp/moe_m28_emp.ok9WzQ/prefix` with `MOE_TOPK_ENABLE_EMP_OT=ON`.

## Rank analysis and root cause

`protocol_i_priority_key` encodes lower keys as higher Top-K priority. The frozen oracle assigns rank equal to the number of strictly smaller keys. `ProtocolIUcmpMaterial::eval_strict_lt(left,right)` returns one for `left < right`; CmpAgg accumulates that result into the right endpoint and the complementary share into the left endpoint. Therefore `[10,20,30]` reconstructs `[0,1,2]`, not `[2,1,0]`. The candidate test expected-rank helper was corrected to use `<`, based on this independent chain and existing CmpAgg conformance.

The remaining failure is observed after non-identity local shuffles (`index=5 got=7 expected=5`). Dealer `r_share` and edge materials are indexed in the original domain, while only priority-key shares are passed through the composed secret shuffle. Consequently the opened `y = shuffled_key_share + original_slot_r` and the DCF edge masks are no longer position-aligned. This is category C/D permutation/record binding, not a reason to alter the oracle or CmpAgg direction. A secure fix requires carrying/rebinding correlated `r` and edge material through the shuffle design; no unsafe local reorder was introduced.

## Build isolation

`VFSS/ext/bitpack/CMakeLists.txt` now creates `bitpack_test` only under `BUILD_TESTING`. Its link is explicitly `PRIVATE bitpack`. `bitpack::mod` is externally defined so the test link resolves. The top-level VFSS CMake now separates tests/benchmarks from production targets; EMP candidate core/executable remain available with `BUILD_TESTING=OFF`, while candidate and conformance tests are gated.

Fresh `BUILD_TESTING=OFF`, EMP-OFF configure succeeded and target help contained only `sytorch` among test/bench/candidate patterns. `sytorch` built successfully. Fresh EMP-ON configure succeeded; candidate core and executable built with `BUILD_TESTING=OFF`. With testing enabled, `bitpack_test` and candidate test built, and the candidate test had an explicit dependency on the candidate executable.

## Evidence matrix

| Area | Result |
|---|---|
| package serialization/material negative checks | PASS before E2E rank gate |
| bitpack test build | PASS in `/tmp/moe-stage3g-on`; runtime result not re-measured in the final build |
| candidate configure/build/link | PASS on Ubuntu 24.04 EMP-ON |
| candidate process startup/FD framing/trace structural checks | PASS before rank gate |
| public-y/same-r differential | BLOCKED by shuffled-domain binding failure |
| rank-share differential | FAIL (`index=5 got=7 expected=5`) |
| independent Dealer/P0/P1 E2E | FAIL at rank differential |
| actual R1/R2/R3 causal barriers | STATIC_ONLY; trace structure reached, full E2E not passing |
| M1/M2/M3/full CTest | NOT_MEASURED in a complete built test graph; no blanket pass claimed |
| BUILD_TESTING=OFF target isolation | PASS for configure/target graph and `sytorch` production build |
| CORE_RUNTIME_GO | BLOCKED |
| Adapter/paper-exact gate | `ADAPTER_ENTRY_BLOCKED`; paper-exact claims prohibited |

## Static security audit

FSS PRNG initialization remains fail-closed in the candidate executable; deterministic test seeds remain TEST_ONLY. Transport HUP handling and trace field ordering fixes from Stage 3F were retained. No online Dealer, file polling, fixed sleep, plaintext secure reconstruction, output adapter, or M3 modification was added. Metrics are sourced from protocol counters. Full runtime validation remains blocked by the binding defect.

## Modified files

- `VFSS/CMakeLists.txt`
- `VFSS/ext/bitpack/CMakeLists.txt`
- `VFSS/ext/bitpack/src/bitpack/bitpack.cpp`
- `VFSS/tests/moe_topk/protocol_i_dealer_candidate_test.cpp`
- `docs/reproduction/M2_PROTOCOL_I_DEALER_CANDIDATE_UBUNTU_VALIDATION_2026-09-12_STAGE3G.md`

No files under `VFSS-baseline/`, `Papers/`, `Agarwal_TopK/`, `ADSMPC/`, or `CipherGPT/` were modified. No commit was created in Stage 3G; the worktree contains the changes above.

## Commands and evidence paths

Key commands were Ubuntu 24.04 WSL invocations of `cmake -S VFSS -B ...`, `cmake --build ... --target sytorch`, `cmake --build ... --target moe_topk_m2_dealer_preprocessed_3round_candidate_test -j1`, and `ctest --test-dir ... -R moe_topk_m2_dealer_preprocessed_3round_candidate_test --output-on-failure`. Evidence builds: `/tmp/moe-stage3g-off3`, `/tmp/moe-stage3g-emp-off`, `/tmp/moe-stage3g-on2`.

## A-J answers

A. Root cause: original-domain `r`/edge material is not rebound to shuffled slots (category C/D).
B. Expected rank uses frozen smaller-key rank semantics: PASS.
C. Candidate compile/link: PASS.
D. Package/material conformance: PASS up to E2E gate.
E. Public-y/same-r differential: BLOCKED.
F. Independent-process E2E: FAIL at rank differential.
G. Three causal barriers: STATIC_ONLY, not fully passing E2E.
H. `BUILD_TESTING=OFF` excludes tests/benchmarks and keeps production target: PASS.
I. `CORE_RUNTIME_GO`: NO, BLOCKED.
J. Adapter entry and paper-exact gate: `ADAPTER_ENTRY_BLOCKED`; paper-exact status remains unavailable.

## Next stage

Implement and prove a protocol-consistent correlated-record binding for shuffled `r` shares and edge materials, with n=2/n=3, identity/reverse/random, duplicate/all-equal, and dummy-padding differential cases. Only after that should candidate E2E, M2/M3 regression, and full CTest be rerun.
