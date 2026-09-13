# Stage 3K M2 Protocol I Route A Report

Status: `PRIORITY_ROUTE_A_RUNTIME_GO; APP_ROUTE_A_INTEGRATION_PENDING`.

Implementation label: `m2_protocol_i_dealer_preprocessed_rank_reveal_6round_mask_output`.
This is a project extension. It is not paper-exact and does not rename the formal
`m2_protocol_i_raw_score_input_modular_8round_mask_output` baseline or the
three-round candidate label.

## Composition

The adapter composes the existing dealer-preprocessed candidate (two forward
Permute+Share barriers and one masked-list opening), performs one framed P0/P1
rank-share exchange, derives `rank < K` in the shuffled domain, and sends the
carrier through the existing two-pass role-swapped reverse shuffle. The output is
cropped to `logical_n`; each party returns the local parity of its arithmetic
share as an XOR bit share.

The carrier convention is frozen as P0=`public selection`, P1=`0`. Since
`(a+b) mod 2 = (a mod 2) XOR (b mod 2)`, parity extraction is local and adds no
causal barrier. Rank reveal is framed with session, fingerprint, dimensions,
party direction, phase and sequence, and malformed rank permutations fail closed.

## Files changed

- `VFSS/include/moe_topk/protocol_i_dealer_candidate_route_a.h`
- `VFSS/src/moe_topk/protocol_i_dealer_candidate_route_a.cpp`
- `VFSS/src/apps/m2_protocol_i_dealer_preprocessed_3round_candidate.cpp`
- `VFSS/tests/moe_topk/protocol_i_dealer_candidate_route_a_test.cpp`
- `VFSS/CMakeLists.txt`
- `docs/decisions/M2_PROTOCOL_I_ROUTE_A_RANK_REVEAL_OUTPUT.md`
- `docs/reproduction/M2_PROTOCOL_I_ROUTE_A_STAGE3K_REPORT_2026-09-13.md`

`VFSS/tests/moe_topk/protocol_i_small_e2e_test.cpp` was updated to invoke the
explicit Route-A priority adapter; its existing process harness remains the
independent-process oracle/controller path.

## Verification boundary

`git diff --check` passes. Ubuntu 24.04 EMP-ON verification used
`/tmp/moe-stage3k-on`, Eigen3 CMake configuration, and the local emp-tool/emp-ot
prefix. The Route-A target and the modular independent-process E2E both built;
the full M2 CTest selection passed 18/18, including the six-round Route-A target
and the candidate process E2E. The fresh production app also builds. No
communication-performance claim is made: byte totals are collected by the
existing metrics path but were not benchmarked across repeated runs. The
raw-score eight-round Route-A label remains `BLOCKED / NOT_IMPLEMENTED`, and
the paper-exact gate remains `BLOCKED / NOT_VERIFIED`.

Commands and evidence:

- Configure: `cmake -S VFSS -B /tmp/moe-stage3k-on -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EMP_OT=ON -DCMAKE_PREFIX_PATH=/tmp/moe_m28_emp.ok9WzQ/prefix -DEigen3_DIR=/usr/share/eigen3/cmake`.
- Build: Route-A target, modular E2E target, and the complete build all succeeded.
- Test: `ctest --test-dir /tmp/moe-stage3k-on -R 'moe_topk_m2_' --output-on-failure` reported `18/18` passed in 10.56 seconds.
- Production: a fresh `BUILD_TESTING=OFF` EMP-ON build completed through `moe_topk_m2_dealer_preprocessed_3round_candidate`.
- Static: `git diff --check` passed; no frozen baseline or reference project was changed.

The modular E2E uses the explicit Route-A priority adapter and exercises P2,
P0/P1, forward barriers, rank reveal, reverse barriers, original-order mask
reconstruction, and oracle comparison. The standalone candidate executable's
new `--route-a` result-frame branch is compile-checked but has no dedicated
CTest process harness yet; therefore that app branch is not included in the
`PRIORITY_ROUTE_A_RUNTIME_GO` claim.
