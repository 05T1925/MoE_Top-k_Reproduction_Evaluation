# Stage 3L M2 Protocol I Route A Report

## Status

`M2_ROUTE_A_PRIORITY_MASK_GO` and `M2_ROUTE_A_RAW_SCORE_8ROUND_MASK_GO`.
`M2_PAPER_EXACT_BLOCKED / NOT_VERIFIED` remains unchanged. Both Route A paths
are project extensions and do not rename the formal M2 baseline, the three-round
candidate, or M3.

## Provenance

- Branch: `codex/m2-candidate-ubuntu-validation`
- Pre-implementation HEAD: `98be22fe52a251f9a6824b59fd35aa91fc349412`.
- Final verification revision: `f752a77` (`feat(m2): complete Route A mask output paths`).
- Worktree after commit: clean. The clean-revision results below were rerun
  from `/tmp/moe-stage3l-committed`.
- Host: Ubuntu 24.04 WSL2, x86_64; CMake; Eigen3 CMake package;
  EMP-Tool/EMP-OT 1.0 local prefix `/tmp/moe_m28_emp.ok9WzQ/prefix`.
- Build flags: `RelWithDebInfo`, `MOE_TOPK_ENABLE_EMP_OT=ON`.
- Formal performance benchmark: `NOT_MEASURED`.

## Implemented Paths

| Path | Input | Secure path | Output | Causal rounds | Label |
| --- | --- | --- | --- | ---: | --- |
| M2 formal baseline | raw-score shares | existing modular baseline | original-order XOR mask | 8 | `m2_protocol_i_raw_score_input_modular_8round_mask_output` |
| M2 candidate | padded priority-key shares | candidate core | shuffled rank shares | 3 | `m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate` |
| M2 Route A priority | padded priority-key shares | candidate + rank reveal + reverse shuffle | original-order XOR mask | 6 | `m2_protocol_i_dealer_preprocessed_rank_reveal_6round_mask_output` |
| M2 Route A raw | signed Q20.12 raw-score shares | 2-round score adapter + Route A priority | original-order XOR mask | 8 | `m2_protocol_i_raw_score_dealer_preprocessed_rank_reveal_8round_mask_output` |
| M3 modular | padded priority-key shares | GRank + DPF/routing + combine | original-order XOR mask | 3 | `agarwal_protocol_iii_modular_3round` |

## Priority-Key Independent Process

CTest target: `moe_topk_m2_dealer_preprocessed_rank_reveal_6round_mask_output_test`.
It launches the production candidate executable three times as independent
`--role dealer`, `--role party0 --route-a`, and `--role party1 --route-a`
processes. P2 sends framed candidate packages and exits before the controller
sends priority-key shares. P0/P1 use separate rank-reveal and reverse-shuffle
socketpairs; the controller sends no carrier or final mask. The actual route
result frame is `RA6M` version 1, phase 7, sequence 1, and is parsed by the
TEST_ONLY harness.

The route test covers identity, reverse and random permutations, duplicate and
all-equal scores, signed extreme values, dummy padding, non-power-of-two sizes,
`K=1`, `K=n`, and stable ties. It checks logical output length, XOR bit shares,
exactly `K` reconstructed ones, and nonzero rank/reverse byte counters. Result
identity/truncation negatives and existing transport EOF/timeout/package
negative cases are exercised. Result: **PASS**.

The causal trace is: R1/R2 candidate forward shuffle, R3 masked-list opening,
local CmpAgg rank-share evaluation, R4 rank-share reveal, local public carrier
derivation, R5/R6 reverse shuffle. No selected original index or permutation is
opened.

## Raw-Score Independent Process

CTest target: `moe_topk_m2_raw_score_route_a_8round_mask_output_test`.
The same production executable is launched as independent Dealer/P0/P1
processes with `--route-a --raw-score-route-a`. The Dealer sends serialized
`ProtocolIPartyPackage` material containing candidate edge material plus separate
carry and sign stages. The controller sends only additive raw-score shares in
the input frame; P0/P1 exchange carry and sign material over dedicated framed
channels. The actual output frame is `RA8M` version 1, phase 8, sequence 1, and
carries the configured `material_id` before its metric fields.

The parser verifies output length, route identity, nonzero carry/sign/candidate/
rank/reverse counters, and total rounds equal to 8. The final XOR mask is
reconstructed only in the test controller and compared with the signed int32
oracle. The matrix covers
`n={1,2,3,5,7,8,11,16,31,127,128,129,256}`, `K=1`, `K=2`, `K=8` where valid,
`ceil(n/2)`, and `K=n`; styles include random, all-equal, alternating
`INT32_MIN/MAX`, all `INT32_MIN`, and monotone values. Result: **PASS**.

The causal trace is: R0 carry adapter, R0b sign adapter, R1/R2/R3 candidate,
R4 rank reveal, local carrier derivation, R5/R6 reverse shuffle. Adapter bytes,
candidate bytes, rank bytes, reverse bytes, and total rounds are carried in the
RA8M result frame. Network/performance repetitions are `NOT_MEASURED`.

## Regression and Build Evidence

Fresh configure/build directory before commit: `/tmp/moe-stage3l-final`.

Clean-revision configure/build directory: `/tmp/moe-stage3l-committed`.

- Full `ctest --test-dir /tmp/moe-stage3l-final --output-on-failure`: **34/34 PASS**.
- Clean-revision full `ctest --test-dir /tmp/moe-stage3l-committed --output-on-failure`:
  **34/34 PASS**.
- Priority Route A and raw Route A focused rerun: **2/2 PASS**.
- `ctest -N`: 34 tests, including both Route A independent-process targets.
- Fresh `BUILD_TESTING=OFF` production build directory: `/tmp/moe-stage3l-prod`;
  target `moe_topk_m2_dealer_preprocessed_3round_candidate`: **PASS**.
  Test-only Route A executable is absent from the production build.
- `git diff --check`: **PASS** before commit; final worktree is clean.
- Frozen `VFSS-baseline/` and reference trees were not intentionally modified.

## Changed Files

Stage 3L and the uncommitted Stage 3K worktree together contain:

- `VFSS/CMakeLists.txt`
- `VFSS/include/moe_topk/protocol_i_dealer_candidate_route_a.h`
- `VFSS/include/moe_topk/protocol_i_raw_score_route_a.h`
- `VFSS/src/apps/m2_protocol_i_dealer_preprocessed_3round_candidate.cpp`
- `VFSS/src/moe_topk/protocol_i_dealer_candidate_route_a.cpp`
- `VFSS/src/moe_topk/protocol_i_raw_score_route_a.cpp`
- `VFSS/tests/moe_topk/protocol_i_dealer_candidate_route_a_test.cpp`
- `VFSS/tests/moe_topk/protocol_i_dealer_candidate_test.cpp`
- `VFSS/tests/moe_topk/protocol_i_raw_score_route_a_test.cpp`
- `VFSS/tests/moe_topk/protocol_i_small_e2e_test.cpp` (pre-existing Stage 3K
  explicit Route A adapter change retained)
- `docs/decisions/M2_PROTOCOL_I_ROUTE_A_RANK_REVEAL_OUTPUT.md`
- `docs/decisions/M2_PROTOCOL_I_RAW_SCORE_ROUTE_A.md`
- `docs/reproduction/M2_PROTOCOL_I_ROUTE_A_STAGE3K_REPORT_2026-09-13.md`
- `docs/reproduction/M2_PROTOCOL_I_ROUTE_A_STAGE3L_REPORT_2026-09-13.md`

No push, PR, baseline edit, reference copy, key, log, or build artifact was
added to the repository.
