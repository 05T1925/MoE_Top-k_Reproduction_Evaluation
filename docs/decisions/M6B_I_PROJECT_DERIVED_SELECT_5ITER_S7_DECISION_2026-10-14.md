# M6B-I PROJECT_DERIVED_SELECT_5ITER S7 decision

Date: 2026-10-14
Route: `PROJECT_DERIVED_SELECT_5ITER`
Composition: `Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / EXPERIMENTAL`
Source base: `c3926c68fd14f270faa8b55234311071947fa080`; S6 was an uncommitted, hash-frozen delta.
Decision: **S6 source snapshot independently received within the recorded test scope; bounded N≤256 functionality is accepted; raw sample seed is not disclosed by the reviewed S6 runtime; full joint party-view security remains OPEN; formal performance remains NOT_ALLOWED.**

## Decisions

1. **Keep the route identity.** This is a project-derived stable Select construction. It is neither a BB90 reproduction nor a BMW16 algorithm. S1's BB90 source gate remains `NO_GO_FOR_IMPLEMENTATION`; the existing BMW16-derived candidate and the six-variant comparison table remain separate.
2. **Freeze S6 as files, not as a commit.** S6 had no task commit: worktree `codex/m6b-project-derived-5iter-s6` remained dirty on base `c3926c68fd14f270faa8b55234311071947fa080`. The S7 receipt uses the S6 source manifest and input-freeze list. In the S7 isolated reconstruction, all 48 inherited/frozen files matched the expected hashes, and the 9 runtime/test plus 3 governance S6 deltas matched the S6 manifest. The exact receiver build/test record is linked from the S7 report and machine-readable manifest.
3. **Separate S5 historical seed records from S6 behavior.** The frozen S5 E2E JSON contains `sample_seed` values from the pre-fix S5 test runs. Those rows are preserved historical evidence and are not S6 online output. In the S6 runtime, party package/binding schema, current S6 E2E case JSON, and production transport fields, no raw `public_sample_seed` is found. S6 generates each active sample on P0 from `getrandom()` immediately before that iteration's schedule message and sends only the current shuffled-coordinate positions. The message disclosure is within the already confirmed current-round position/endpoint schedule; it does not reveal a reusable seed or a future sample tape. The deterministic seeded sampler in the controller is a TEST_ONLY path, not used by the runtime overload.
4. **Accept the bounded functionality only at its verified scope.** The acceptance applies to the exact S6 snapshot and only `1≤logical_n≤256`, `1≤K≤logical_n`, the frozen stable Q20.12 key semantics, and the specific M6B5/v2 monolithic (`N≤128`) or M6S2 streamed (`N≤256`) package profile. It establishes functionality, not a secret-view simulation, production host isolation, or a BB90 algorithm claim.
5. **Do not merge the two material schemas in the security argument.** M6B5/v2 N≤128 contains the used five Select pools and unused M2CP forward/inverse all-pairs DCF pools. M6S2 N=256 uses metadata-only two-round shuffle material and has no such unused M2CP comparator pools. Both require a separate party-view argument tied to their exact package and addressing ABI.
6. **Leave the secure gate open.** The first missing proof step is a simulator/reduction for the actual single-party serialized DCF key share, correlated threshold `alpha`, local mask shares, revealed masked operands and opened comparison bit, across the complete used/unused pool and adaptive slot schedule. The monolithic unused pool additionally contains the cycle relations `alpha_ij + alpha_jk = alpha_ik (mod 2^b)` induced by a shared `r` vector. No applicable multi-key adaptive theorem or simulator is supplied by the repository. No concrete distinguisher was found; that is not a PASS.
7. **Leave M6A gates untouched.** S34 remains a technical schema-v3 diagnosis rather than an independent acceptance; M6A V3 remains OPEN. V4 and formal LAN/WAN remain `NOT_ALLOWED / NOT_RUN`. The maintained benchmark plan requires V1, V2 and interface handoff, both AAV86 implementations, their leakage/round audits and validated counters before V3; V4 further requires completed V3, fixed BB90 source/parameters/probability guarantees, both complete combinations and stable-threshold/DCF audit.

## Gate summary

| Gate | S7 decision | Scope |
|---|---|---|
| `S6_SOURCE_RECEIPT` | `PASS_BY_NONPARTICIPANT` | Only the exact hash-frozen S6 snapshot and the rerun items listed in the S7 receipt. The receiver did not participate in S4/S5/S6. |
| `FUNCTIONAL_E2E_N256` | `PASS_BOUNDED` | 23 S6 cases re-executed; oracle/no-mask assertions and N=256 K matrix are recorded in the S7 machine record. |
| `SEED_NONDISCLOSURE` | `PASS_FOR_S6_RUNTIME; HISTORICAL_S5_SEEDS_PRESERVED` | No raw runtime sample seed in S6 package, binding, peer frames or final S6 case record. S5 archive still has old `sample_seed` rows. |
| `MATERIAL_ONE_SHOT_AND_RESTART` | `PASS_LOCAL_POSIX_CROSS_PROCESS; HOST_REBOOT_NOT_TESTED` | Child exit/reclaim and crash-before/after-claim behavior were rerun. OS reboot/power loss was not tested; administrator rollback/deletion, network filesystem semantics and production supervisor operations are excluded. |
| `DCF_SINGLE_KEY` | `PASS_FUNCTIONAL_ABI_ONLY` | `keyGenDCF/evalDCF` ABI, direction and one-call consumption checked; this is not a key-share privacy proof. |
| `ADAPTIVE_FULL_POOL` | `OPEN` | Full correlated used/unused party key pool has no S7 simulator/hybrid. |
| `SHUFFLE_OUTPUT_COMPOSITION` | `OPEN` | Forward/inverse shuffle, endpoint transcript, membership, output shares and abort are not jointly simulated. |
| `JOINT_PARTY_VIEW` | `OPEN / NO_GO_FOR_SECURE_CLAIM` | No proof pass; no concrete attack found. |
| `CONDITIONAL_SECURITY_STATUS` | `NOT_ESTABLISHED` | Do not describe the route as conditionally secure. |
| `N_GT_256_ADMISSION` | `NO_GO` | N=257 preflight rejects; no extrapolation. |
| `V3_V4_PERFORMANCE_STATUS` | `V3=OPEN; V4=NOT_ALLOWED/NOT_RUN` | Formal LAN/WAN matrix remains prohibited at this stage. |

The S7 report is a separate technical receipt of S6 files and specified runs. The receiver had prior S1/S2 research involvement and performed an S5 baseline receipt, but did not participate in S4/S5/S6 implementation or decisions. It does not retroactively convert S4/S5/S6 author-side records into independent reviews or claim that functional acceptance is a security certification.

