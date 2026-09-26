# M5 → M6A G3 receiver acceptance

Acceptance date: 2026-09-26
Receiver: Codex, independent receiving run
G3_REVIEWED_MAIN_COMMIT: 9b3ce3747b1734602e3edf4c644ae1b6da52e8c1

## Verdict

G3_ACCEPTANCE = PASS. The clean receiver checkout at main@9b3ce3747b1734602e3edf4c644ae1b6da52e8c1 builds, passes all 41 CTest targets, and independently exercises the standard raw-score-to-mask API, independent P2/P0/P1 processes, cost smoke, and fail-closed behavior. The receiver required no developer-local patch, build directory, or generated key.

This closes the M5 receiving gate at the project's documented engineering boundary:

- M5 = COMPLETED
- M5-FIX-F1 = COMPLETED
- F2 / G3 = PASS
- M5_READY_TO_CLOSE = YES

It does not establish an author-exact reproduction or change the documented paper-cost limitations.

## Reviewed revision and clean checkout

The authoritative remote main was cloned from GitHub into a new Windows temporary directory, then copied with git clone --no-hardlinks into a separate Linux receiver checkout. The WSL HTTPS fetch attempt ended with a TLS recv error -110; the Windows Git remote clone succeeded. Both checkouts reported the same exact main commit, and the Linux receiver checkout was clean before build:

- Remote clone: main@9b3ce3747b1734602e3edf4c644ae1b6da52e8c1
- Linux receiver clone: main@9b3ce3747b1734602e3edf4c644ae1b6da52e8c1
- git status -sb: ## main...origin/main
- git status --porcelain=v1: empty
- git diff --check: PASS

The reviewed commit contains M5 merge commit 2db7dbf (PR #25) and the receiver checklist merge (PR #26).

## Environment and build

- Ubuntu 24.04.4 LTS in WSL, x86_64
- GCC 13.3.0, CMake 3.28.3, Ninja 1.11.1, Eigen 3.4.0
- Debug build; EMP OT disabled

Commands:

    cmake -S VFSS -B build-vfss-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DMOE_TOPK_ENABLE_EMP_OT=OFF
    cmake --build build-vfss-debug -j4

Configuration succeeded and all 180 build steps completed successfully, including the standard API implementation and F1 test targets.

## Regression and F1 end-to-end

Command:

    ctest --test-dir build-vfss-debug --output-on-failure

Result: 41/41 passed. This includes the F1 differential and process E2E targets, M5 field/Fselect/Fsort regressions, H1 communication smoke, M3 controls, and Protocol I regressions.

The standard production API is protocol_iii_raw_score_mask_party in VFSS/include/moe_topk/protocol_iii_raw_score_mask.h. P0/P1 provide their own std::vector<std::uint32_t> additive shares in Z_(2^32) for signed two's-complement Q20.12 scores, with public logical_n and K. Each returns a length-logical_n std::vector<std::uint8_t> XOR share. Reconstruction is the original-order stable Top-K mask, ordered by score descending then original index ascending; constraints are n >= 2 and 1 <= K <= n.

The receiver ran:

- moe_topk_m5_fix_f1_raw_score_mask_test: 140/140 differential cases passed, including n=2/3/4/5/8, K=1/K=n/middle K, ascending/descending/all-equal/mixed duplicates, negative/zero/positive and signed boundary words, distinct Q20.12 fractions, and deterministic random cases. Reconstructed masks matched the clear test oracle, had exactly K ones, and had logical length only.
- moe_topk_m5_fix_f1_process_e2e_test: 3/3 process cases passed at n=3/5/8, covering tie, signed-boundary/duplicate, and K=n cases. Output shares were formed by the party processes before the test controller collected and reconstructed them.

The standard full pipeline is 4 online rounds: 2 input-adapter rounds, 2 Protocol III mask GRank/routing rounds, and 0 extra output-adapter rounds. The generic field-valued Fselect/Fsort core remains separately labeled as 2 rounds.

## Process isolation and security boundary

The TEST_ONLY harness launches P2, P0, and P1 as separate OS processes using fork+exec. P2 receives only offline-generation channels, distributes party-bound material, and is waited for to exit before the controller releases either online input share. No online channel is assigned to P2. P0/P1 receive only their own score shares and party-local bundle; result collection occurs after their local mask shares are formed.

The tested standard entrypoint has no plaintext reconstruction bridge:

- P2_ONLINE_SILENT = YES
- RAW_SCORE_PUBLIC = NO
- RAW_RANK_PUBLIC = NO
- RAW_MASK_PUBLIC = NO
- PLAINTEXT_RECONSTRUCTION_BRIDGE = NO

Clear scores and the reconstructed oracle mask remain in TEST_ONLY controller code. The runtime opens masked comparison keys and masked ranks as documented; it does not open raw scores, raw ranks, or the mask.

## Cost smoke

Command:

    ./build-vfss-debug/moe_topk_m5_fix_f1_process_e2e_test --cost-smoke

All six smoke rows passed. Counts below are this receiver run, one process execution per configuration, and are not latency measurements. Logical bits and wire sends are totals across P0/P1; offline bytes are the serialized party bundles and their length prefixes.

| n | K | padded n | logical online bits | wire bytes | offline bytes | full rounds |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2 | 1 | 2 | 684 | 576 | 9,470 | 4 |
| 5 | 2 | 8 | 2,566 | 1,056 | 48,844 | 4 |
| 8 | 8 | 8 | 2,800 | 1,152 | 83,452 | 4 |

The logical-bit formula 272p + 2n(ell' + r) gives 684, 2,566, and 2,800 for these configurations; framed wire counts follow 384 + 64p + 32n and agree with 576, 1,056, and 1,152 bytes. Stage totals matched the raw counters. This is only the required smoke check, not M6 performance acceptance.

## Failure and one-shot smoke

moe_topk_m5_fix_f1_process_e2e_test --failure-smoke injected an early carry-channel close. Both parties exited nonzero, neither emitted a valid mask, and the harness reported F1_FAILURE_SMOKE_PASS early_close no_valid_mask=1.

The passing F1 test target also rejected wrong party/session/fingerprint/material/shape/width bindings, altered headers and section length, truncated and trailing bundle bytes. Reuse after successful consumption and reuse after a partial failed exchange were rejected. This includes one-shot material behavior in the frozen runtime, not persistent cross-process replay prevention.

SANITIZER_RESULT = NOT_RUN. Sanitizers are not a G3 acceptance criterion; the developer-side sanitizer record was not substituted for receiver evidence.

## Preserved evidence limits

- AUTHOR_EXACT = NOT_PROVEN
- EXACT_THEOREM_4_2_COST_MATCH = NO; the H1 implementation-to-paper logical ratio remains about 1.42–1.44, with a 254n-bit difference under the documented accounting.
- ORDER_OF_MAGNITUDE_COMM_MATCH = YES
- COMMON_MASK_OPTIMIZATION = NOT_IMPLEMENTED
- FIELD_DPF_FORMAL_PROOF = NOT_DONE
- PERSISTENT_REPLAY_DATABASE = NOT_IMPLEMENTED
- Native FSS-key serialization retains its documented same-build/same-architecture constraint.
- F1 is a four-round bit-mask C-INSTANTIATION. General arbitrary-payload ring-to-field and field-to-XOR conversion remain unimplemented; the two-round field Fselect/Fsort core and this standard mask entrypoint are distinct interfaces.

## Gate result

| Gate | Receiver result |
| --- | --- |
| Clean main checkout and build | PASS |
| Full regression | PASS, 41/41 |
| Standard raw-score-to-mask E2E | PASS, 140/140 differential plus independent-process cases |
| Stable tie, exact K, padding exclusion | PASS |
| Independent P2/P0/P1; P2 exits before input release and stays online-silent | PASS |
| No plaintext reconstruction bridge; raw score/rank/mask remain private | PASS |
| Cost smoke n=2/5/8 | PASS |
| Malformed binding/material, transport failure, and one-shot behavior | PASS |
| Developer-local artifact required | NO |

G3_ACCEPTANCE = PASS; M5 is ready to close at this project evidence boundary. The M6A performance matrix has not been run.
