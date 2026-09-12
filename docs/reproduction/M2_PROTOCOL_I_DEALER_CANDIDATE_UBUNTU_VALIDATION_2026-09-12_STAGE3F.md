# Stage 3F: M2 Protocol I Dealer-preprocessed Candidate Ubuntu Validation

Date: 2026-09-12  
Branch: `codex/m2-candidate-ubuntu-validation`  
Initial HEAD: `f733d4266fbd493140b898872c45e03cb53fb80f`  
Final HEAD at report time: working tree changes after `c3b7daceecd2ce58a822917b0a9d2c9e4f527fab`

## Scope and boundary

This stage validates the project candidate label `m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate` with real EMP-ON configuration. The candidate outputs only `public_masked_list` and `shuffled_rank_share`. It does not implement an output adapter, reverse shuffle, selection carrier, original-order Top-K mask, or a paper-exact Agarwal Protocol I claim.

## Environment

- Ubuntu 24.04.4 LTS under WSL2, kernel `6.6.87.2-microsoft-standard-WSL2`, x86_64.
- GCC/G++ 13.3.0; CMake 3.28.3; CTest 3.28.3; Ninja 1.11.1.
- Eigen 3.4.0 via `/usr/share/eigen3/cmake`; OpenSSL 3.0.13 with `libcrypto.so.3`.
- Real pinned EMP prefix: `/tmp/moe_m28_emp.ok9WzQ/prefix`.
- emp-tool `v1.0.0-alpha.1`, commit `97f335927dd7d38caaf5e80d93fca70edddd5423`, archive SHA256 `7f4a2cb169ba0b7fc48ffe89b7615288c41f9377bb0a4d56a3178fe20b66ab46`.
- emp-ot `v1.0.0-alpha.1`, commit `03acb042b98e82fd5fd0da33babd44801f8ec082`, archive SHA256 `8cfffc340a2014e5ac3b90c6659c0a812a6748e7e24c6ea2b0f5b3e58ba66183`.

## Commands and results

EMP-ON configure passed with:

```text
cmake -S /mnt/c/Users/28641/Desktop/MoE_Top-k_Reproduction_Evaluation/VFSS \
  -B /tmp/moe-stage3f-build-20260912 -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EMP_OT=ON \
  -DCMAKE_PREFIX_PATH=/tmp/moe_m28_emp.ok9WzQ/prefix \
  -DEigen3_DIR=/usr/share/eigen3/cmake
```

Candidate core, executable, and test targets configured and built with `-j1`. A parallel `-j2` attempt first hit a `/mnt/c` static-archive visibility race (`ranlib: libsytorch.a: No such file`); the serial retry passed.

The candidate test initially failed because exec-isolated candidate processes had unkeyed FSS PRNGs. The executable now initializes 256 FSS PRNG slots from OpenSSL randomness. The test also received a TEST_ONLY deterministic PRNG initialization.

After those fixes, the candidate test progressed through package conformance, transport negative cases, process startup, framed transport, and trace validation, but currently fails at rank differential (`candidate rank differential index=0 got=0 expected=1`). This is a real FAIL, not a pass or an adapter result.

The full EMP-ON build reached 99% but failed in the pre-existing `ext/bitpack` test link: `undefined reference to bitpack::mod(unsigned long, int)`. This is STATIC_ONLY/BUILD_GOVERNANCE failure outside the candidate implementation.

The fresh `BUILD_TESTING=OFF`, `MOE_TOPK_ENABLE_EMP_OT=OFF` configure succeeded, but the build still included test/bench targets and reproduced the same bitpack test link failure. Therefore production OFF isolation is NOT_MEASURED/PENDING CMake target-graph correction.

CTest registration reported 32 tests, including candidate, M2, M3, and DPF groups. M2/M3/full regression was not run to completion because the candidate gate is FAIL and the full build is blocked by bitpack linking.

## Changes made

1. `VFSS/tests/moe_topk/protocol_i_dealer_candidate_test.cpp`: deterministic initialization of all 256 FSS PRNG slots for TEST_ONLY local checks; rank-oracle direction was aligned with the implemented CmpAgg convention during diagnosis.
2. `VFSS/src/apps/m2_protocol_i_dealer_preprocessed_3round_candidate.cpp`: initialize all 256 FSS PRNG slots per exec-isolated process using OpenSSL `RAND_bytes`.
3. `VFSS/src/moe_topk/protocol_i_transport.cpp`: accept `POLLHUP|POLLIN` while buffered data remains readable; treat HUP without I/O readiness as an error.
4. `VFSS/src/moe_topk/protocol_i_dealer_candidate_core.cpp`: correct trace event aggregate field order so `completed`, sequence, and byte counters serialize according to the declared struct.

No changes were made to `VFSS-baseline/`, papers, reference projects, formal M2/M3 labels, or output adapters.

## Commits

- `c3b7daceecd2ce58a822917b0a9d2c9e4f527fab` test(m2): initialize three-round candidate FSS test PRNGs.
- Remaining Stage 3F fixes are currently uncommitted pending resolution of the rank differential and transport/core review.

## Gate answers (A-J)

- A Environment: PASS for Ubuntu 24.04, CMake/CTest, Eigen, OpenSSL, and real pinned EMP package discovery.
- B Candidate configure/build: PARTIAL; target build passes serially, full graph blocked by bitpack test link.
- C Package/transport conformance: PASS up to candidate process execution; negative cases execute before rank gate.
- D Independent-process E2E: FAIL at rank differential.
- E Three-round trace: STATIC_ONLY/PARTIAL; trace is produced and structurally validated before rank gate, but no final passing E2E artifact.
- F M2/M3/full regression: NOT_MEASURED due candidate gate and full-build blocker.
- G BUILD_TESTING=OFF production isolation: NOT_MEASURED; current target graph still builds tests.
- H Metrics provenance: candidate metrics are generated by the implementation; no performance benchmark is claimed (`NOT_MEASURED`).
- I Adapter gate: `CORE_RUNTIME_GO` is not granted because rank differential is failing; `ADAPTER_ENTRY_BLOCKED` remains.
- J Paper boundary: no paper-exact, seven-round, original-order mask, or exact leakage-equivalence claim is made.

## Final assessment

Stage 3F is INCOMPLETE. Real EMP dependencies are present and CMake discovery works. Candidate startup, transport HUP handling, and trace serialization defects were exposed and fixed, but the independent-process candidate still fails rank differential, and the repository-wide build is blocked by the existing bitpack test link. Further work must first resolve the rank/permutation semantic mismatch and isolate test targets for `BUILD_TESTING=OFF`.
