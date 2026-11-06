# M6B-I PROJECT_DERIVED_SELECT_5ITER S13 decision

Date: 2026-10-15
Route: `Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / EXPERIMENTAL`
Receipt object: S12 exact 58-file hash snapshot at base `c3926c68fd14f270faa8b55234311071947fa080`; not merged to main.
Decision: keep EXPERIMENTAL; do not create a `secure` alias.

## Decisions

1. S12 is hash-reconstructed and replayed in a fresh isolated worktree, but not independently accepted. The current reviewer authored S12 runtime work and therefore reports a same-session technical replay only. S10's independent receipt remains scoped to S8.
2. Fresh Release selected-target build and six directed CTest cases pass. The full all-target build still fails at the unrelated `bitpack_test` link. Feature-OFF registers zero Select5 tests. Functional harness evidence is 18 frozen-oracle successes, six uniform abort/no-mask cases, and six crash-prefix cases with no complete mask; this is not independent S12 receipt.
3. Do not claim the actual `VFSS/ext/FSS/dcf.cpp` party-key privacy from Boyle et al. The first concrete bridge failure is `Convert_G`: current terminal 128-bit input has two low bits fixed, so the first-64-bit conversion output lies in a `2^62`-element subset of the `2^64` values on the tested x86_64 little-endian target (the actual internal distribution may have smaller support). `Pr[y mod 4=0]` is 1 instead of 1/4; Figure 1's conversion maps a uniform 128-bit input to a uniform 64-bit group element. This is a theorem-mapping witness, not a party-key distinguisher or DCF break.
4. Also leave the restricted AES-key family, complementary random root-control orientation, and CryptoTools root-PRNG mapping unproved. Current single-key privacy, correlated full-pool simulation, adaptive access, shuffle/output composition and joint party view are OPEN.
5. Select `DCF_BCG21_FIG1_AES128_V1` as the next versioned candidate: a separate port of the published Figure 1 construction with uniform 128-bit AES PRG keys, exact 514-bit PRG stream, exact `Z/(2^64)` Convert, independent root seeds and fixed `t0=0,t1=1`. It is design-only and not implemented or integrated; no old key/package is reused. Require new ABI/material schema and independent review before any security claim.
   With the current 33-byte `M2UC` header/count envelope retained, the candidate's byte-aligned record would be `57+25b` bytes (versus current `57+24b`). This gives key-only increases of 1,639,424 bytes/party for the enumerated active N=128 slots and 6,719,104 bytes/party for N=256. Unused N=128 M2CP slots and new package/segment framing are excluded; whole-package size/capacity is `NOT_MEASURED` until the versioned schema is specified and generated.
6. Preserve no-complete-mask as the output crash guarantee. The six test prefixes include two single orphan output shares; they do not prove atomic all-or-nothing publication.
7. N>256 remains rejected. Formal performance remains NOT_ALLOWED/NOT_RUN because V3 is OPEN and V4 is not authorized by the existing plan.

## Gate summary

| Gate | Status |
|---|---|
| `S12_SOURCE_RECEIPT` | `HASH_RECONSTRUCTED` |
| `INDEPENDENT_RECEIPT` | `NOT_SATISFIED` |
| `FUNCTIONAL_E2E_N256` | `PASS_SAME_SESSION_REPLAY` |
| `DCF_SINGLE_KEY_PRIVACY` | `OPEN` |
| `CORRELATED_POOL_SIMULATION` | `OPEN` |
| `ADAPTIVE_QUERY_SIMULATION` | `OPEN` |
| `SHUFFLE_OUTPUT_COMPOSITION` | `OPEN` |
| `JOINT_PARTY_VIEW` | `OPEN` |
| `SECURE_ALIAS_READY` | `NO` |
| `N_GT_256_ADMISSION` | `NO_GO` |
| `V3_V4_PERFORMANCE_STATUS` | `V3_OPEN; V4_NOT_ALLOWED/NOT_RUN; FORMAL_LAN_WAN_NOT_ALLOWED/NOT_RUN` |

Detailed evidence, theorem mapping, schema ledgers, command line and hashes are in the [S13 report](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S13_REPORT_2026-10-15.md), [case records](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S13_REPLAY_CASES_2026-10-15.json), and [source/build manifest](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S13_SOURCE_BUILD_MANIFEST_2026-10-15.json). The immutable input identity is the [S12 source/build manifest](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S12_SOURCE_MANIFEST_2026-10-15.json).
