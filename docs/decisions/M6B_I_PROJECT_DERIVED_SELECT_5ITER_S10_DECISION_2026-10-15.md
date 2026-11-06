# M6B-I PROJECT_DERIVED_SELECT_5ITER S10 decision

Date: 2026-10-15  
Route: `PROJECT_DERIVED_SELECT_5ITER`  
Composition: `Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / EXPERIMENTAL`  
Decision: **Accept S8's exact manifest-selected snapshot for bounded functional evidence; issue a counter erratum; keep DCF single-key privacy, adaptive full-pool simulation and shuffle/output composition OPEN. No runtime changes.**

## Decision basis

1. The independent receiver `/root/s8_independent_receipt` did not participate in S4–S8 implementation or decisions. It reconstructed the 58 unique S8 source paths from the 55 inherited and 12 delta records in S8 manifest SHA-256 `c5d29679aeaf1be3cc71da1ffc8053b33d589fd15313031137b5b79655e1dffb`, verified every source hash, and separately checked the manifest. S8 remains unmerged from main; identity is source hashes plus base `c3926c68fd14f270faa8b55234311071947fa080`, not branch name.
2. Fresh Ubuntu 24.04.4 Release replay passed the seven requested test targets, 24 T/P0/P1 cases, and a feature-OFF registration/build check. All 18 success masks were reconstructed only in the TEST_ONLY controller and matched frozen C++ oracle bit-for-bit, with exact-K, bit and original-order checks; six faults produced ABORT/no-mask.
3. S10 independently recomputed S8's historical 24-row comparison counters. N=128 is `298/368/2479`; N=256 is `600/695.5/14723` (min/median/max). S8 report/manifest median 747 is wrong; the S8 decision's alternative triples are unsupported by the named JSON. The dated addendum preserves all historical evidence unchanged.
4. S8's 32-byte sample seed is generated and published only after forward shuffle for an active iteration, as approved. No seed is preloaded or logged in material/ready metadata; later-round seeds are not generated until activation. This approves only sample randomness in L.
5. Actual DCF ABI and serialized key pool were reviewed. No complete simulator/conditional distribution lemma exists for local DCF key+mask packages given opened masked operands/bits and adaptive addresses. Thus the joint security claim remains OPEN even though no concrete distinguisher was found.

## Gates

| Gate | Decision |
|---|---|
| `S8_SOURCE_RECEIPT` | `PASS_EXACT_MANIFEST_SNAPSHOT` |
| `FUNCTIONAL_E2E_N256` | `PASS_BOUNDED_ORACLE_CHECKED` |
| `SAMPLE_SEED_AFTER_SHUFFLE` | `PASS` |
| `S8_COUNTER_RECONCILIATION` | `PASS_ERRATUM_ISSUED` |
| `BUILD_PROVENANCE` | `PARTIAL_PATH_NORMALIZED; ORIGINAL_HASHES_DIFFER` |
| `DCF_SINGLE_KEY_PRIVACY` | `OPEN` |
| `ADAPTIVE_FULL_POOL_VIEW` | `OPEN` |
| `SHUFFLE_OUTPUT_COMPOSITION` | `OPEN` |
| `JOINT_PARTY_VIEW` | `OPEN; NO CONCRETE COUNTEREXAMPLE FOUND` |
| `SECURE_ALIAS_READY` | `NO` |
| `N_GT_256_ADMISSION` | `NO_GO` |
| `V3_V4_PERFORMANCE_STATUS` | `V3=OPEN; V4=NOT_ALLOWED/NOT_RUN` |

The route may be described as bounded project-derived stable Top-K functional behavior for the received code/configuration. It is not BB90, BMW16 or secure. The BB90 S1 source gate and six-scheme comparison remain unchanged. No code defect was reproduced, so runtime files were not changed. Formal LAN/WAN stays NOT_ALLOWED/NOT_RUN under the existing V3→V4 prerequisites.

## Follow-up proof obligation

For each actual material schema and each corrupted party, provide a simulator that sees only the party's ideal input/output and approved leakage L, and jointly generates local package keys/masks (including unused slots), adaptive slot transcript, masked operands, opened bits, forward/inverse shuffle view, membership, output share and ABORT/restart prefixes. The first missing lemma is the conditional simulability of the serialized local DCF key shares plus masks under the opened operand/bit transcript and adaptive slot choices. Prove it or produce a concrete distinguishing witness before changing the security gate or adding any secure alias.
