# M6B-I PROJECT_DERIVED_SELECT_5ITER S14 report

Date: 2026-10-15  
Route: `Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL`  
Branch/worktree: `codex/m6b-project-select5-s14`, `C:\Users\28641\.codex\worktrees\m6b-project-select5-s14\MoE_Top-k_Reproduction_Evaluation`  
Base HEAD: `c3926c68fd14f270faa8b55234311071947fa080`; task changes remain uncommitted and are not merged.

## 1. Result summary

S12's exact source snapshot was independently received by a receiver who did not participate in S12 or S4–S12 implementation/route decisions. That receipt covers S12 only. S14 then added `DCF_BCG21_FIG1_AES128_V1` as a new ABI and integrated it into the existing Select5 route without changing the legacy DCF code or old package ABI.

The final S14 Release candidate passes BCG DCF conformance, Select control, material-profile guard, default-OFF inspection, 13 successful BCG Select5 T/P0/P1 oracle cases, and six BCG fault/no-mask cases. `DCF_SINGLE_KEY_PRIVACY` and static key-only pools are conditionally supported by a paper/code review under stated AES-PRP and OS-CSPRNG assumptions. The first full-view hybrid for opened masked operands/comparison bits and shuffle/output is not proved. `JOINT_PARTY_VIEW=OPEN`; no secure alias.

No formal LAN/WAN matrix was run. M6A V3 remains OPEN, V4 remains NOT_ALLOWED/NOT_RUN, and this route is not registered as the historical BB90 alternative.

## 2. S12 independent receipt and exact scope

The receiver's relationship declaration: no participation in S12 implementation and no participation in S4–S12 implementation or route decisions. The receiver reconstructed the S12 manifest-selected 58-file snapshot in a detached review worktree, verified all 58 SHA-256 values, and recomputed canonical digest `dcfd9bccda57edb0cfbc4c953ae5fbd640535cd67dfba59756c2aae6949e4188`. S12 manifest SHA-256 is `e333b5a7bda6b067ca3a626946c8bfa34c53fdade64f8bd4cb61bb626d3a78e`. S12 was based on `c3926c68fd14f270faa8b55234311071947fa080`, uncommitted, not merged.

The independent receiver used Ubuntu 20.04.6 WSL, GCC 9.4.0, OpenSSL 1.1.1f and CMake/CTest 3.31.6. Six CTest tests and two direct executables passed: Select5 control, monolithic material store, Select5 three-process E2E, uCMP, shuffle conformance, shuffle process E2E, streamed slot store and TLS 1.3. The 30 controller records were 18 oracle successes, six uniform abort/no-mask and six crash-prefix no-complete-mask. The controller XOR-combined only in TEST_ONLY and checked each successful mask against the frozen C++ oracle (stable signed Q20.12 descending score, original-index ascending ties, original order, length/bits/weight K). Feature-OFF excluded Select5. The report and hashes are preserved in the S12 evidence directory and copied S12 source manifest; S10's S8 receipt is not used as a substitute.

Known receipt limits: the independent receiver reported a pre-existing whole-build `bitpack_test` undefined reference; the directed targets did build and pass. Six of eight rebuilt binary hashes differed from S12's earlier recorded binaries due embedded absolute dependency paths; the receiver bound the result to its own fresh binary hashes and did not claim binary equivalence. The current S14 author did not independently receive S14.

## 3. S14 source/build identity

S14 begins from the same base commit but is a separate uncommitted worktree. Exact per-file S12-to-S14 classifications and final SHA-256 values are in the [S14 source/build manifest](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S14_SOURCE_MANIFEST_2026-10-15.json). Its canonical selected source digest is computed from manifest-listed repository-relative path and final file hash pairs; the manifest itself is excluded from its own digest.

Build environment: WSL Ubuntu 20.04.6; GCC/G++ 9.4.0; OpenSSL 1.1.1f; CMake/CTest 3.31.6; Release; `BUILD_TESTING=ON`; EMP OT OFF; Select5 experimental ON. External evidence directory: `C:\Users\28641\.codex\evidence\M6B_I_S14_DCF_BUILD_20261015_01`. The compile cache and binary SHA-256 values are in the source/build manifest.

Build commands (paths shortened only here):

```text
cmake -S <S14-worktree>/VFSS -B <evidence>/build-release -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EMP_OT=OFF -DMOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=ON
cmake --build <evidence>/build-release --target moe_topk_dcf_bcg21_fig1_aes128_v1_conformance_test moe_topk_project_derived_select5_conformance_test moe_topk_project_derived_select5_material_store_test moe_topk_project_derived_select5_process_e2e_test moe_topk_m2_ucmp_conformance_test moe_topk_m2_parallel_shuffle_conformance_test moe_topk_streamed_material_slot_store_test moe_topk_tls13_transport_conformance_test -j2
<evidence>/build-release/moe_topk_project_derived_select5_process_e2e_test --bcg21-smoke
sudo <same executable> --bcg21-faults
```

A separate feature-OFF build configured `MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=OFF`; CTest reported zero matching Select5 cases and target help contained no Select5 runtime/test target. The common library still compiles the BCG dispatcher/object; this is not an executable registration or callable route. See raw feature-OFF logs.

## 4. Versioned DCF and package changes

`DCF_BCG21_FIG1_AES128_V1` has independent ABI id 1 and key magic/version `M2B1/v1`. It uses the Figure 1 binary tree, `Bin` 34–53 in conformance, output group `Z/(2^64)`, `beta=1` in uCMP, MSB-first input order, fixed complementary roots `(0,1)`, a 514-bit expander, and first-64-bit terminal `Convert_G`. Serialization is 57+25·Bin bytes and rejects wrong party/version/count/padding/truncation. `ProtocolIUcmpMaterial` branches on the explicit DCF ABI. The old `M2UC/v1` route and `dcf.cpp` generator/evaluator remain unchanged.

The route-specific material ABI is also separate:

| Range | Outer/store package | Shuffle encoding | DCF key |
|---|---|---|---|
| N≤128 BCG | `M6B5/v3`; ready marker `M6P2/v1` | `M2CM/v1` metadata-only; BCG runtime rejects nonempty legacy edge-key vectors | `M2B1/v1` |
| N=129..256 BCG | `M6S2/v2`, ABI id 1, segmented per-party store | streamed O(N) metadata slots; no legacy edge-key pool | `M2B1/v1` |
| legacy route | `M6B5/v2` and `M6S2/v1` | `M2CP/v1` legacy full material | `M2UC/v1` |

For N=128 the BCG pool has 41,024 DCF key-pair slots per party: 40,640 Select + 256 score-conversion + 128 membership. Width split is Q40=40,768 (Select+membership) and Q34=256 (score). The 1,280 shuffle metadata records (`10N`) replace the unused legacy shuffle DCF pool. Observed monolithic package size is 44,985,596 B/party (89,971,192 B for both), below the 64 MiB per-party cap.

For N=256 there are 163,968 DCF key-pair slots per party: 163,200 Select + 512 score + 256 membership, plus 2,560 shuffle metadata records, total 166,528 records. Width split is Q41=163,456 and Q34=512. The final E2E staging reported 192,644,416 B/party, 12,656,192 B manifest/party, 11 segments, and 385,288,832 B across both parties. This is local functionality/storage evidence, not a network package timing or performance result. Source-derived keygen work is 67,191,040 AES block calls and 327,936 fresh 16-byte root draws per N=256 dealer generation; not wall-clock measured.

## 5. Correctness and E2E evidence

The DCF conformance target passed the AES FIPS-197 known-answer test, exhaustive domains for Bin 1–5 and all alpha values through Bin 8 with strict boundary checks, and target Bin 34–53 endpoint/near-endpoint alpha values, four beta values, serialization round trips, malformed party/version/truncation rejection, uCMP wrap correction and one-shot replay rejection. This is functional evidence; it does not replace the theorem proof.

The final BCG Select5 successful T/P0/P1 rows use the frozen C++ oracle in the TEST_ONLY controller. Fixture values contain duplicate scores, signed Q20.12 extrema/neighbor values, zero and small repeated values. The controller verifies mask length, bits, original order and exactly K selected positions. There are 13 successful BCG rows: N=1/K=1; N=2/K=1,2; N=128 streamed/K=1,64,128; N=128 monolithic/K=64; N=256 streamed/K=1,2,8,128,255,256. Exact per-case sessions, material state, sample rounds, comparisons, comparison DAG layers, exchanges, byte counts and result are in the [S14 E2E case file](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S14_E2E_CASES_2026-10-15.json).

Six BCG fault cases independently report `UNIFORM_ABORT_NO_MASK`: package tamper, peer disconnect, wrong party K, wrong mTLS peer identity, T exit before ready, and invalid sample-seed message magic. N=256 monolithic resource preflight rejects before generation; N=257 streamed preflight rejects. The test harness must run as a trusted supervisor to launch distinct party UIDs. An initial non-root invocation returned SKIP (77); the final fault run used WSL root supervisor, and the final Release output contains all six no-mask rows. These are no-mask fault contracts, not a universal crash/host-failure guarantee.

For the final smoke sample, observed comparison-call diagnostics were N=128 min/median/max 311/370/2,938 over three streamed cases, and N=256 605/619/12,055 over six cases. They describe only these function runs, not an expectation theorem, high-probability bound, timing result, or formal benchmark. The route has five algorithm iterations, up to nine plain comparison dependency layers, and observed end-to-end message exchanges from 13 to 32 in these cases. They are distinct quantities and are not the Protocol I paper's three-round core.

## 6. Failures and fixes retained

- The first BCG conformance run failed at `Bin=1, alpha=0, x=0`: correction-word generation used the wrong child quantity. The implementation was fixed to compute the group correction from the parsed `v_L/v_R` values; the original failed log remains alongside the passing final conformance log.
- The first BCG monolithic layout included the legacy `M2CP` full-edge shuffle DCF pool, bringing the package to about 61.6 MB/party. Independent review identified that this was an unused material pool in the BCG candidate. S14 added `M2CM/v1` metadata-only serialization and a runtime check rejecting injected legacy edge keys. The final package is 44,985,596 B/party; material-store tests cover the ABI split and rejection guard.
- An initial fault E2E launched without the trusted supervisor and returned skip code 77; the final fault case run used `wsl.exe -u root` and passed all six cases.
- One directed CTest invocation used the system CTest in the desktop source cwd and found zero tests. This was preserved and corrected by using the configured native CMake/CTest 3.31.6 on the isolated evidence build. After the final BCG injection guard, the six directed registered tests (BCG DCF, Select5 control, material store, three-process Select5 E2E, uCMP and parallel shuffle) passed 6/6 in 406.90 s. The separate BCG N≤256 smoke and six fault/no-mask runs also passed. The initial invocation and the corrected final invocation have separate immutable logs.
- A full default build still encounters the previously observed unrelated `bitpack_test` undefined reference `bitpack::mod(unsigned long,int)`. This task uses explicitly built directed targets and does not claim a whole-repository build pass.

All first failures remain in the unique external evidence directory; no historical S12/S13 JSON or report was rewritten.

## 7. Security review and gates

Two read-only reviewers (`/root/independent_dcf_review`, `/root/s12_receipt`) who did not implement S14 DCF code compared it with BCG+21 Figure 1, Theorem 2 and Appendix F.1 and found no substantive mismatch in the semantic single-key mapping. They checked source `dcf_bcg21_fig1_aes128_v1.cpp` at SHA-256 `b2bc67cbe26bd11b2fded7a3f10002970947694e001cbf16f2bbbd45ae83982f`; the second reviewer also checked the revised joint RNG assumption and pool terms. Single-key privacy is **CONDITIONAL**: `delta_w ≤ w·epsilon_G + epsilon_RNG_joint(T;slot)`, where `epsilon_G ≤ Adv_AES128^PRP(T,q=5)+10/2^128`; the 64-bit group Convert loss is zero. `epsilon_RNG_joint(T;X)` is the computational advantage for the actual joint OpenSSL `RAND_bytes`/Linux `getrandom` output in context X versus mutually independent uniform bits, including any side information in the same material. For N=128 the key-only pool bound is `1,639,424·epsilon_G + epsilon_RNG_joint(T;M6B5/v3,N128)`; for N=256 it is `6,719,104·epsilon_G + epsilon_RNG_joint(T;M6S2/v2,N256)`. Their DCF-root portions are 1,312,768 and 5,246,976 bytes, respectively; the joint terms also include getrandom-derived package masks/metadata. These bounds apply only to the party's DCF-key projection and canonical serialization/digest, with fixed slot parameters; they are symbolic, conditional assumptions rather than concrete numeric security levels. The final PRG byte retains four unused bits that the parser ignores; only 514 bits are consumed, so this does not change the mapping. Reviewers ran no S14 tests; functional evidence is implementation-run and separate.

The first unproved whole-view step is replacing the joint conditional distribution of a party's own mask-share material, opened `z_left/z_right`, opened compare bit, DCF eval share/key slot, adaptive future addresses and shuffle/output state using only approved leakage L, ideal Top-K output and that party's own input share. The DCF theorem does not provide that UCMP/transcript simulator. **No concrete attack was found, but the full proof is not closed.** Adaptive query simulation and shuffle/inverse/output/abort/restart composition remain OPEN for each schema; no secure alias.

| Gate | S14 result |
|---|---|
| `S12_INDEPENDENT_RECEIPT` | PASS, exact 58-file S12 snapshot only |
| `DCF_BCG21_FIG1_CORRECTNESS` | PASS for tested domains/parameters |
| `SELECT5_ORACLE_E2E_N_LE_256` | PASS for 13 enumerated BCG cases; this is not an independent S14 receipt |
| `MATERIAL_SCHEMA_ISOLATION` | PASS for tested M6B5/v3+M2CM/v1 and M6S2/v2 profiles; production multi-host operations NOT_MEASURED |
| `DCF_SINGLE_KEY_PRIVACY` | CONDITIONAL PASS under paper PRG theorem, AES-PRP, CSPRNG and exact S14 ABI mapping |
| `CORRELATED_POOL_SIMULATION` | CONDITIONAL for the key-only static pool hybrid; full related protocol pool view OPEN |
| `ADAPTIVE_QUERY_SIMULATION` | OPEN beyond deterministic address post-processing of an already simulated static key pool |
| `SHUFFLE_OUTPUT_COMPOSITION` | OPEN; actual opened masked operands/comparison bits and inverse/output shares lack a joint simulator |
| `JOINT_PARTY_VIEW` | OPEN; no concrete attack found, proof incomplete |
| `SECURE_ALIAS_READY` | NO |
| `N_GT_256_ADMISSION` | NO_GO; N=257 preflight rejects |
| `V3_V4_PERFORMANCE_STATUS` | V3 OPEN; V4 NOT_ALLOWED/NOT_RUN; formal LAN/WAN NOT_ALLOWED/NOT_RUN |

## 8. Files changed by S14

The exhaustive list is in the S14 source manifest. Relative to the S12 58-file snapshot, S14 changes the DCF/uCMP/Select5/material/shuffle/score-input/transport source set, adds the versioned DCF and tests, updates project state, and adds the S14 decision/report/security/gate/case/manifest documents. Inherited S1–S12 files present because the source snapshot is uncommitted are not described as S14 edits unless listed in the manifest's `s14_modified` or `s14_added` section.

