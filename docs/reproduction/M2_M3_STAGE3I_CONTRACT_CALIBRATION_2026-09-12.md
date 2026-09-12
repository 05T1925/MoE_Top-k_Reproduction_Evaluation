# Stage 3I: M2 Candidate Evidence Freeze and M3 Contract Calibration

Date: 2026-09-12  
Frozen revision: `9de4ac2a8df95dbd894dfd8c0db0d77a6ecb9f58`  
Branch: `codex/m2-candidate-ubuntu-validation`

## Executive conclusion

Stage 3H evidence is now frozen on a clean committed revision. Fresh Ubuntu 24.04 EMP-ON validation reproduced the candidate and complete configured CTest result: 32/32 passed. M3 contract audit and existing conformance/E2E regression confirm the modular 3-round contract without changing M3 source semantics.

## Provenance and commits

Base `2492eca` is the Stage 3F commit. Stage 3G/H validation originally ran on a dirty worktree. Stage 3I split and committed those changes:

1. `511d673 fix(build): isolate test targets from production graph`
2. `f10155d test(m2): correct candidate padded-domain rank oracle`
3. `9de4ac2 docs(m2): freeze Stage 3H candidate runtime evidence`

The worktree is clean at the frozen revision; `git diff --check` passes.

## M2 candidate freeze

Label: `m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate`. It is a project extension with P0/P1 online parties and P2 offline Dealer, output-slot `r`, and shuffled-domain rank-share output. R1/R2 are the two forward Permute+Share passes; R3 opens the masked shuffled list; DCF/CmpAgg rank evaluation is local. Candidate compile/link, package/material conformance, public-y/same-r, identity/reverse/random permutations, duplicate/all-equal/padding, independent-process E2E, 3 causal barriers, M1/M2 regression, M3 regression, full CTest, and BUILD_TESTING=OFF isolation are PASS on the frozen revision. Performance remains `NOT_MEASURED`.

The candidate has no rank reveal, rank-to-selection conversion, reverse shuffle, original-order mask, or selection carrier. Therefore `CORE_RUNTIME_GO` is PASS for the candidate gate; `ADAPTER_ENTRY_GO` and paper-exact Protocol I remain BLOCKED/NOT_VERIFIED.

## M2/M3 separation

| Dimension | M2 formal baseline | M2 candidate | M3 modular baseline |
|---|---|---|---|
| Label | `m2_protocol_i_raw_score_input_modular_8round_mask_output` | `m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate` | `agarwal_protocol_iii_modular_3round` |
| Input | raw-score shares | padded priority-key shares | padded priority-key shares |
| Output | original-order XOR mask | shuffled rank shares | original-order XOR mask |
| Online rounds | 8 | 3 | 3 |
| Paper status | project baseline | project candidate | modular intermediate |

M3 raw-score extension remains `moe_topk_protocol_iii_raw_score_modular_5round`; it is not the native three-round label.

## M3 contract calibration

The source/test mapping is consistent: GRank builds its comparison graph over `logical_n`; priority-key input arrays use `padded_n`; ranks use `rank_bits = max(1, ceil(log2(logical_n)))`; DPF routing evaluates over `2^rank_bits`; secure output is an original-order XOR mask of length `logical_n`. M3 tests cover non-power-of-two sizes, dummy/padding handling, stable ties, invalid dimensions, DPF payload evaluation/transport, one-shot material behavior, masked multiplication, secure combine, metrics, and independent-process execution.

The modular M3 causal sequence remains R1 GRank, R2 DPF routing first stage, and R3 secure combine/masked multiplication. Offline Dealer transfer is not counted as an online causal round. Existing metrics tests distinguish 3-round priority-key and 5-round raw-score paths, preserve labels and logical/padded dimensions, and leave unmeasured PRG/performance fields as `NOT_MEASURED`.

## Clean-revision validation

Fresh build directory: `/tmp/moe-stage3i-on2`  
Ubuntu command used: `wsl.exe -d Ubuntu-24.04` with CMake EMP-ON, `CMAKE_PREFIX_PATH=/tmp/moe_m28_emp.ok9WzQ/prefix`, `Eigen3_DIR=/usr/share/eigen3/cmake`, `RelWithDebInfo`, `BUILD_TESTING=ON`. Full configured CTest: 32/32 PASS in 20.70s. Raw logs: `/tmp/stage3i-config.log`, `/tmp/stage3i-build.log`, `/tmp/stage3i-full.log`.

## Status and remaining blockers

- M2 candidate: `CORE_RUNTIME_GO` PASS; `ADAPTER_ENTRY_BLOCKED`.
- M2 formal 8-round baseline: unchanged project baseline.
- M2 paper-exact gate: BLOCKED / NOT_VERIFIED.
- M3: `M3_CALIBRATED` for the documented modular contract; no M3 source changes were required in Stage 3I.
- Performance/network/PRG measurements: `NOT_MEASURED` where not supplied by the existing records.

Rank-share to selection-carrier conversion is nonlinear; reverse shuffle cannot provide it for free. It requires a separate design-gated stage.

## Modified files in Stage 3I

- `VFSS/CMakeLists.txt`
- `VFSS/ext/bitpack/CMakeLists.txt`
- `VFSS/ext/bitpack/src/bitpack/bitpack.cpp`
- `VFSS/tests/moe_topk/protocol_i_dealer_candidate_test.cpp`
- `docs/reproduction/M2_PROTOCOL_I_DEALER_CANDIDATE_UBUNTU_VALIDATION_2026-09-12_STAGE3G.md`
- `docs/reproduction/M2_PROTOCOL_I_DEALER_CANDIDATE_UBUNTU_VALIDATION_2026-09-12_STAGE3H.md`
- `docs/reproduction/M2_M3_STAGE3I_CONTRACT_CALIBRATION_2026-09-12.md`

No baseline, reference, paper, key, build, or log artifact was modified in the repository.

## A-J answers

A. Clean committed revalidation: PASS, frozen revision `9de4ac2`.  
B. Candidate evidence revision: `9de4ac2` (PASS provenance).  
C. Candidate `CORE_RUNTIME_GO`: PASS for candidate gate.  
D. Candidate output remains shuffled rank shares only: PASS.  
E. M2 output adapter: BLOCKED.  
F. M2 paper-exact gate: BLOCKED / NOT_VERIFIED.  
G. M3 priority-key input length: `padded_n` (PASS).  
H. M3 GRank graph: `logical_n` (PASS).  
I. M3 DPF domain `2^rank_bits`, online rounds 3 (PASS).  
J. M3 status: `M3_CALIBRATED`; unmeasured performance/network/PRG fields remain `NOT_MEASURED`.
