# M6B-I PROJECT_DERIVED_SELECT_5ITER S8 decision

Date: 2026-10-15  
Route: `PROJECT_DERIVED_SELECT_5ITER`  
Composition: `Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / EXPERIMENTAL`  
Decision: **Adopt version-3 public sampling disclosure after forward shuffle; bounded functionality passes this maintainer's S8 regression; S8 is not independently received; the joint single-party view remains OPEN.**

## 1. Identity and source receipt

This is the project-derived five-iteration stable Select. It is not a BB90 or BMW16 algorithm and inherits none of their algorithm, probability, comparison-cost, round or security results. It does not replace a BB90 row in the six-variant comparison.

S7's S6 receipt remains the independent baseline receipt. S8 reconstructed a separate worktree from S7's exact selected snapshot: base `c3926c68fd14f270faa8b55234311071947fa080`, S7 manifest SHA-256 `0b1118c038cca4b1deadb4f522ff22d8358aed43d3b9f6b8367b72b21096e907`. Fifty-five inherited/overlay/S7 record paths matched their manifest hashes. The S8 author then modified the runtime and test sources and ran the S8 tests; this is maintainer verification, not independent acceptance of S8. The source manifest gives the exact file list and hashes for a later nonparticipant receiver.

S7 had five of seven FSS-linked target binary hashes differ from S6 despite matching recorded CMake target flags and Ubuntu 24.04.4/CMake 3.28.3/GCC 13.3/OpenSSL 3.0.13 toolchain. Embedded absolute source paths differ between the S6 worktree and S7 snapshot; that explains path strings but not every differing byte. S8 does not claim those historical binaries were equivalent. Its evidence is tied to its own selected source snapshot, fresh configuration, and S8 binary hashes.

## 2. Version-3 sampling disclosure decision

The S8 online binding contract is version 3 and supersedes S7's no-seed-disclosure rule only for this new runtime version. S7 remains accurate for its exact S6 source snapshot, whose runtime did not send the seed. Existing M6B5/v2 monolithic and M6S2 streamed party material schemas are unchanged. Neither package, slot manifest, ready marker, startup metadata nor phase-3 binding includes an actual sampling seed.

After both forward-shuffle exchanges (phases 20 and 21) return, the P0 runtime creates a new 32-byte seed using `getrandom()` only when the controller enters an active reduction iteration. P0 sends this at phase `40+i` over the existing authenticated framed channel. Payload bytes are exactly:

```text
u32be(0x53355352) || u32be(3) || u8(iteration) || seed[32]
```

The payload is 41 bytes. P1 checks exact size, `S5SR` magic, version 3 and the current expected iteration before acknowledging with the 9-byte payload:

```text
u32be(0x5335414b) || u32be(3) || u8(iteration)
```

The authenticated frame header binds session, fingerprint, padded N, K, bit width, sender, receiver, phase, frame type and one-shot material ID. The message frame does not carry a candidate set or plaintext score. Because session/iteration and channel are already part of the existing frame binding, no new message field is needed for those values.

For the shared sample map, both parties use the same current publicly derived candidate vector and `s=min(m,ceil(sqrt(m)))`. Partial Fisher–Yates selects the first `s` entries without replacement. Its stream input is the ASCII bytes (no terminating NUL) of `PROJECT_DERIVED_SELECT_5ITER/public-sample-prng/v3`, then the seed bytes, `session` as 8-byte big-endian, `fingerprint` as 8-byte big-endian, `iteration` as one byte, and a 64-bit big-endian counter. SHA-256 emits 32-byte blocks; each consecutive eight bytes form a big-endian word. For bound `b`, reject words below `((-b mod 2^64) mod b)`, otherwise use `word mod b`. Counter exhaustion, digest failure, malformed frame, or mismatched session/phase takes the error/ABORT path; it cannot yield a successful mask.

The raw 32-byte seed is now explicitly in `L_S8` for both execution parties. Once phase `40+i` occurs, either party can immediately compute the current sample order and sample-clique from the public current candidate set. This may reveal those current positions before their comparison endpoints are otherwise transmitted. The seed does **not** remain secret after publication. A future seed is not pre-generated and cannot be derived from the current seed; a later active iteration obtains fresh OS entropy only after its own preceding state is complete. If the target was already selected, no later seed is drawn or disclosed. The sample seed is unrelated to DCF α and does not authorize disclosure of keys, mask shares, input scores, permutation mapping, pivot values, thresholds or output mask.

## 3. Verification and algorithm contract

The fixed sample vector, seed frame roundtrip, wrong version/iteration, truncated/trailing payload, cross-session frame, prior-phase replay, duplicate/out-of-order iteration and pre-shuffle state rejection are checked by the conformance target. Both parties use the same public-seed mapping. The controller's four reduction iterations retain the existing state/rank transitions and `s=min(m,ceil(sqrt(m)))`; the fifth iteration remains exact selection. The top-level frozen stable order remains signed Q20.12 descending and original index ascending, with original-order mask of exactly K bits.

Fresh Ubuntu 24.04 Release reran the seven S7 targets with exit 0. The three-process E2E returned 24 case rows: 18 frozen-oracle successes and six `UNIFORM_ABORT_NO_MASK` fault cases. Its N=128 successes cover K `1,2,8,64,65,128` and a repeated K=64 case; N=256 covers K `1,2,8,64,128,255,256` and repeated K=128. N=257 streamed preflight rejects. The E2E checks exact mask bits, length and weight K, plus durable-material replay rejection after successful output. A new `sample-seed-invalid-magic` case injects a malformed seed payload through the actual TLS runtime after forward shuffle; both parties abort without an output mask. S8 E2E case JSON records no actual sample seed.

The algorithm output remains correct for every valid random tape: seed choice changes sample/work only; exact rank invariants and fifth-layer selection determine the selected key. An invalid sample message or resource/error path fails closed/no-mask. The SHA-256 expansion is a deterministic specified map; its random-looking distribution is not a proved finite-scale uniform sampler. Therefore S2's expectation calculation under ideal independent uniform samples is not an implementation guarantee. S8 records only observed comparison-call diagnostics: N=128, 7 success cases, min/median/max `299/1825/5340`; N=256, 8 success cases, `600/676/18408`. These counts are not time performance, a tail bound, or an expected-cost proof.

The full path still distinguishes five algorithm iterations, up to nine plaintext comparison dependency layers, conditional CA core formula `2k+1` (19 if its assumptions held at k=9), Protocol I paper-core count 3, and end-to-end framed exchanges `9+2L+A≤31` where `L≤9` is the number of nonempty comparison layers and `A≤4` active seed exchanges. Publishing the seed replaces the old sample-position payload in the same exchange; it adds no causal exchange.

## 4. Security decision

The model remains at most one semi-honest execution-party corruption, trusted offline T, no T/party collusion, no active corruptions and authenticated P0/P1 transport. T exits before online execution and cannot generate these seeds. For a P0 simulator, the current public seed is in L and can be placed in its local view; for P1, the simulator receives that same L field and derives the current sample from L and public candidate state. No simulator may use the real transcript as an input.

This handles only the sampling-randomness field. The first unresolved hybrid remains the actual party-local serialized DCF key and local mask-share distribution conditioned on the party input share, full correlated DCF pool, opened masked operands and comparison bits, and adaptively chosen slot addresses. M6B5/v2's unused M2CP DCF pools and common-r-vector threshold relations differ from M6S2's metadata-only streamed layout; the two complete package views still require separate proofs. Forward/inverse shuffle state, membership, output-share and successful/ABORT/restart views also remain uncomposed. No concrete distinguisher was found; no full simulator/reduction or advantage bound is established.

Accordingly: `JOINT_VIEW_SECURITY=OPEN`, `SECURE_ALIAS_READY=NO`, and the route remains opt-in `EXPERIMENTAL`. Do not describe it as secure or conditionally secure.

## 5. Gate decisions

| Gate | S8 decision | Evidence / limit |
|---|---|---|
| `S7_SOURCE_RECEIPT` | `PASS_FOR_EXACT_S6_SNAPSHOT` | S7 receipt is separate and remains valid; S8 reverified 55 selected paths. |
| `S8_INDEPENDENT_ACCEPTANCE` | `NOT_YET_RECEIVED` | S8 author made changes and ran regressions; next receiver must not have participated in S4–S8. |
| `SAMPLE_RANDOMNESS_PUBLIC_AFTER_SHUFFLE` | `PASS_BOUNDED` | v3 runtime guard and phase ordering; no future seed pre-generation. |
| `SAMPLE_RECONSTRUCTION_CONSISTENCY` | `PASS_FUNCTIONAL` | Fixed vector, bound rejection tests, and 18 oracle E2E successes. |
| `FUNCTIONAL_E2E_N256` | `PASS_BOUNDED` | Exact 24 cases, including K matrix and six uniform ABORT/no-mask faults. |
| `DCF_SINGLE_KEY_PRIVACY` | `OPEN` | Functional ABI does not prove privacy. |
| `ADAPTIVE_FULL_POOL_VIEW` | `OPEN` | No simulator for whole used/unused correlated pool under adaptive access. |
| `SHUFFLE_OUTPUT_COMPOSITION` | `OPEN` | No joint proof for forward/inverse shuffle through output share and ABORT. |
| `JOINT_PARTY_VIEW` | `OPEN / NO_GO_FOR_SECURE_CLAIM` | No complete P0 or P1 simulator/hybrid. |
| `SECURE_ALIAS_READY` | `NO` | No security pass. |
| `N_GT_256_ADMISSION` | `NO_GO` | N=257 preflight rejects; no extrapolation. |
| `V3_V4_PERFORMANCE_STATUS` | `V3=OPEN; V4=NOT_ALLOWED/NOT_RUN` | S8 does not bypass M6A prerequisites. |
| `FORMAL_LAN_WAN` | `NOT_ALLOWED/NOT_RUN` | No formal performance matrix launched. |

## 6. Follow-up

An S8 nonparticipant receiver should select only the paths in the S8 source manifest, rebuild with its recorded Release command and flags, rerun the seven targets plus all 24 fixed case parameters, verify that the 41-byte seed frame appears only after phases 20/21, independently compare output to the frozen stable oracle, and audit the exact modified-source and binary hashes. Security review remains a distinct follow-up: prove or refute the actual complete DCF-pool and shuffle/output joint view for each material schema without treating S8 functional tests as a simulator.
