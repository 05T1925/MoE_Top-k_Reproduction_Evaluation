# M6B-I PROJECT_DERIVED_SELECT_5ITER S10 independent receipt and joint-view review

Date: 2026-10-15  
Route: `Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / EXPERIMENTAL`  
Decision: **S8 exact snapshot independently received for bounded functionality; corrected S8 count summary; seed timing matches the approved contract; actual DCF/full-pool/shuffle-output joint-view proof remains OPEN. No runtime code defect found and no runtime change made.**

## 1. Scope, receiver and frozen source

Function contract: each input is a signed Q20.12 score; the total order is score descending, then `original_index` ascending; success returns a binary mask in original input order with length N and exactly K one-bits. The scope here is the S8 received source and N≤256 tested configurations, not a security or asymptotic-cost claim.

The independent receiver is task-scoped reviewer `/root/s8_independent_receipt`. It reports no participation in S4–S8 implementation, decisions or prior receipts. It is not the S9 implementer. It also reports no S1/S2/S5 implementation or receiver role. S7's prior S6 receipt remains scoped to S6 and does not cover S8's seed-publication modification.

S8 was an uncommitted selective reconstruction, not merged to `main`. The source base is `c3926c68fd14f270faa8b55234311071947fa080`; branch label `codex/m6b-project-derived-5iter-s8` is descriptive, not the source identity. The S8 manifest SHA-256 is `c5d29679aeaf1be3cc71da1ffc8053b33d589fd15313031137b5b79655e1dffb`. Its 55 inherited entries plus 12 S8 delta entries resolve to **58 unique source paths** (nine overlap); all 58 were rehashed against the manifest and copied into a separate receipt worktree at the same base. The S8 manifest itself was separately hash-checked. The receipt CSV, transcript and exact selected file list are included/referenced by the S10 source manifest. No source file was changed by S10; only documentation and audit scripts/results were added.

S10's analysis checkout is `C:\Users\28641\.codex\worktrees\m6b-project-derived-5iter-s10`, branch `codex/m6b-project-derived-5iter-s10`, HEAD `c3926c68fd14f270faa8b55234311071947fa080`. The independent receiver built from its own selective receipt worktree `C:\Users\28641\.codex\worktrees\m6b-project-derived-5iter-s10-independent-receipt`; the test binary therefore binds that exact path-bearing build as well as the manifest-selected source hashes. S8 remains not merged to main.

## 2. Independent build and replay

Environment: Ubuntu 24.04.4 WSL2 x86_64, Linux kernel `6.6.87.2-microsoft-standard-WSL2`, CMake 3.28.3, GCC 13.3.0, OpenSSL 3.0.13, Release, `BUILD_TESTING=ON`, EMP OT OFF, Select5 opt-in ON. CPU observed: Intel Core i9-13980HX, 32 logical CPUs. These are functional test facts, not performance results.

Fresh configure/build command:

```sh
cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-project-derived-5iter-s10-independent-receipt/VFSS \
  -B /tmp/m6b-project-derived-5iter-s10-independent-receipt-release \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_EMP_OT=OFF \
  -DMOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=ON
cmake --build /tmp/m6b-project-derived-5iter-s10-independent-receipt-release --target \
  moe_topk_project_derived_select5_conformance_test \
  moe_topk_project_derived_select5_material_store_test \
  moe_topk_streamed_material_slot_store_test \
  moe_topk_tls13_transport_conformance_test \
  moe_topk_project_derived_select5_process_e2e_test \
  moe_topk_m2_ucmp_conformance_test \
  moe_topk_m2_parallel_shuffle_conformance_test -j2
```

All seven targets built and exited 0. Their SHA-256 hashes are:

| Target | Exit | SHA-256 |
|---|---:|---|
| `moe_topk_project_derived_select5_conformance_test` | 0 | `7ad847e5d97a5d57d5bfe20e2fc1c4b1950bf5aad16d01edc1c930ee06321285` |
| `moe_topk_project_derived_select5_material_store_test` | 0 | `6768e60d7dc77ff706ce5b04f9019ac2894f535070bd02753adada37079fc53e` |
| `moe_topk_streamed_material_slot_store_test` | 0 | `19e4fb9a975cffa95812426ab27ea4cb03fc5622d7502c41ad5fb53d95303129` |
| `moe_topk_tls13_transport_conformance_test` | 0 | `069b9ffd089fb04c38d133d4634e1d0962fb6a5ac119563ec0d5e203c9d8cdc4` |
| `moe_topk_project_derived_select5_process_e2e_test` | 0 | `5758db8c1950474eba4b97e98d65c1fb598214f9d8fe9f627ed53c6409d8d3ab` |
| `moe_topk_m2_ucmp_conformance_test` | 0 | `022495c58b967e2fe07d4cd1be3228579f0398b44d017316f6c1be25bba20525` |
| `moe_topk_m2_parallel_shuffle_conformance_test` | 0 | `41777dd7807a7145b14f928bf6f162504233a167099eabe9c2236d79d9de57f7` |

`CMakeCache.txt` SHA-256: `58e4ceb67affec77c66610df099615255687fea12468e48baa6919535ba483d1`.

A second fresh Release configuration used `MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=OFF` with the same source root and `-DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EMP_OT=OFF`; its `target help` contained no Select5, streamed-store or TLS-conformance target. The feature-OFF runner then built and ran the two ordinary baseline targets with `cmake --build <feature-off-build> --target moe_topk_m2_ucmp_conformance_test moe_topk_m2_parallel_shuffle_conformance_test -j2`. Its two ordinary baseline targets built and passed: `moe_topk_m2_ucmp_conformance_test` (`022495c58b967e2fe07d4cd1be3228579f0398b44d017316f6c1be25bba20525`) and `moe_topk_m2_parallel_shuffle_conformance_test` (`41777dd7807a7145b14f928bf6f162504233a167099eabe9c2236d79d9de57f7`). Feature-OFF `CMakeCache.txt` SHA-256: `ec6dc668d76a5614a23f8f3326bf89d9b0f98cfbd88eb0e587f6bac184597649`.

### E2E case matrix and oracle

The fresh three-process `T/P0/P1` E2E target emitted 24 rows and exited 0: **18 `ORACLE_PASS`, 6 `UNIFORM_ABORT_NO_MASK`**. For every successful case the target XORed P0/P1 output mask shares in its isolated test controller, compared every bit with frozen C++ `top_k_mask()`, and checked original order, output length, binary bits and `weight=K`. This is a functional test-controller reconstruction only; no secure runtime disclosure was added. The receiver-authored, per-case JSON is [S10 receipt cases](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S10_RECEIPT_CASES_2026-10-15.json), SHA-256 `61ef6b6b8bd98fb9ec5a097c717220df7472e54175ade912bc369875091e315c`; each record binds to its raw CASE_JSON line and the fixed S8 case spec.

N=128 covers K `1,2,8,64,65,128` and a repeated K=64 row. N=256 covers K `1,2,8,64,128,255,256` and a repeated K=128 row. The six fault rows cover tampered package, peer disconnect, wrong party K, wrong TLS identity, T failure before ready, and malformed sample-seed magic after forward shuffle. All fault rows end in uniform ABORT/no-mask. Directed targets additionally exercise sample-frame shape/version/iteration/session/replay/order and pre-shuffle state rejection, durable monolithic/streamed store semantics, uCMP/DCF and parallel shuffle conformance. The prior N=257 streamed preflight rejection is covered by the directed Select5 controller regression; no N>256 run is inferred from N=256.

The replay's observed comparison calls are newly drawn functional diagnostics under fresh OS sample seeds. They are separate from both historic S8 JSON and S9 implementer reruns:

| Logical N | successful-row calls in this S10 replay | min / median / max |
|---:|---|---:|
| 128 | 298, 325, 580, 884, 2036, 2073, 2454 | 298 / 884 / 2454 |
| 256 | 601, 607, 665, 948, 1202, 11330, 14685, 18473 | 601 / 1075 / 18473 |

These finite rows do not establish expected cost, a high-probability bound or time/communication performance. The algorithm still has five algorithm iterations, up to nine plaintext comparison-dependency layers, and the full protocol's observed exchanges vary by case (e.g. 31 at nine active comparator layers); these are separate counts, not a claim of a three-round Protocol I implementation.

## 3. S8 counter erratum

S10 independently parsed the frozen S8 case JSON with `experiments/PROJECT_DERIVED_SELECT_5ITER_S10/recompute_s8_counters.py`; it checked 24 rows, 18 success rows and six no-mask abort rows. The immutable input JSON hash is `82f89ea34821a96f8653f4f80449162d0cdb8b5dfb10f8e69c6fee1f5b8d1b53`.

Correct statistics over successful rows only:

| Padded N | Sorted observations | min / median / max |
|---:|---|---:|
| 128 | 298, 299, 303, 368, 747, 2094, 2479 | 298 / 368 / 2479 |
| 256 | 600, 605, 607, 626, 765, 10967, 12271, 14723 | 600 / 695.5 / 14723 |

The S8 report and manifest list N=128 median 747, which is the fifth sorted value rather than the median of seven. S8 decision numbers (`299/1825/5340` at N=128 and `600/676/18408` at N=256) cannot be recomputed from the named 24-row JSON; their alternate data/filter is not identified and remains unexplained. S9's same-implementer run is not used as an independent receiver result. The dated [erratum](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S10_COUNTER_ERRATUM_2026-10-15.md) adds the corrected summary without editing S8's report, decision or raw JSON.

## 4. S8/S9 build provenance reconciliation

S10 compared the S8 and S9 worktrees: 356 C/C++/header/CMakeLists path entries across active, baseline and experiment trees had zero hash mismatches; the active VFSS C/C++/header subset was 234 paths, also with zero mismatches; recorded compiler/toolchain, Release flags (`-O3 -DNDEBUG`, `-std=gnu++17`, OpenMP and the project flags) and link inputs match. The source worktree paths differ. The detailed row hashes and normalizer output are in the S10 machine-readable build-provenance ledger. The original S8/S9 binary hashes differ for five FSS-linked targets; `moe_topk_streamed_material_slot_store_test` and `moe_topk_tls13_transport_conformance_test` are byte-identical by SHA-256. For each of the other five targets, normalizing the equal-length embedded absolute worktree component `...-s8`→`...-s9` makes all bytes match except the 20-byte `.note.gnu.build-id`. Thus the observed differences are fully accounted for at byte level by embedded build/source path strings and the derived ELF build ID. Original executable bytes and hashes are still different; this does not assert binary interchangeability.

| Target | S8 SHA-256 | S9 SHA-256 | Byte size | Result |
|---|---|---|---:|---|
| Select5 control | `db90b8aa40b2b871771cd775587d62ef10d4d9b4e5539eacfe84f0b612784921` | `57d1dbe03ad224a5759e2d61ac364095aea9a2fab748a5d346c7eefef8e7f7bf` | 321,760 | path/build-id only after normalization |
| Select5 material store | `2c0f27c254e1058a37629dd1daf63cd55fd4e889d4264962ef8c0c70e4ed0af6` | `a5f08ee2e7942a95d1216da5e100f3f0b98aa2ae56c0a2b96a2ec999149db648` | 558,792 | path/build-id only after normalization |
| Streamed slot store | `19e4fb9a975cffa95812426ab27ea4cb03fc5622d7502c41ad5fb53d95303129` | same | 93,240 | identical |
| TLS 1.3 transport | `069b9ffd089fb04c38d133d4634e1d0962fb6a5ac119563ec0d5e203c9d8cdc4` | same | 63,144 | identical |
| Select5 process E2E | `b72bc6a9df4e389abfe4d67d450ac40ecdc77a0dfe36d7a22f3e60957af8c1d6` | `92a4483c61c9861017e6d5f90e402fb5172f91dc0488695d26ad07087efcaa1e` | 608,792 | path/build-id only after normalization |
| uCMP DCF conformance | `73574e88208cea2e93d30de72468b7f3ad3c47ef12cf07e15fafd03b7b74cdf6` | `ef805eeef41fc2eba6c69240070626b3719de5dcb6d636f83160d3c2c78e0f11` | 202,752 | path/build-id only after normalization |
| Parallel shuffle | `059b7272cb752ec147411105c2764830dc1b4c5bbf9fbee9cc4067ab0aac0192` | `944886f71ebd901b4e2bb7243ae187a9a1ecfd2957b9e7dbda15106426e4560a` | 333,048 | path/build-id only after normalization |

S8's initial Ubuntu 20.04/CMake 3.16 configure failure produced no binary; its first stale E2E attempt had incomplete output and no binary hash. That old failed attempt's exact executable identity remains unresolved and is excluded. The S10 receiver's first Git-inspection wrapper failed before configure because Linux Git could not parse the Windows worktree `.git` pointer; no test ran in that attempt. A second script avoided that Git call and completed fresh configure/build and tests. The first wrapper failure was not replaced or counted as a test failure/pass.

## 5. Seed timing and approved leakage

The runtime source at the received hash generates a fresh 32-byte seed with `getrandom()` only after the two forward-shuffle exchanges (phases 20/21) have completed and only when iteration `i` becomes active. P0 sends it in bound phase `40+i`; P1 validates the seed frame and returns the iteration ACK. The frame/header binds the session, fingerprint, padded N, K, comparison width, direction, phase, frame type and one-shot material ID. The raw seed is not present in offline M6B5/v2 or M6S2 material, slot manifest, ready/start metadata, the initial binding or the S10 case/log records; those records contain `input_share_seed_hex`, which is a test fixture for input sharing and is not the sample seed. Future iteration seed bytes do not exist until that iteration is entered. Seed publication is approved leakage; after release it makes the current sample order/clique derivable from the public current candidate vector. It does not disclose DCF α, DCF keys, mask shares, raw score, permutation mapping, pivot value, threshold or full mask.

The independent raw run does not contain a full packet/frame capture: the precise phase `40+i` is source-derived, while test evidence covers pre-shuffle/order/replay rejection and malformed-seed abort after shuffle. The leakage L used for this review includes the active-round seed; shuffled-coordinate current endpoints and pivot positions; opened comparison bits/call counts; public slot/address labels; masked operands; message phases/lengths/counts; and uniform ABORT/no-mask. It excludes raw values, DCF internal material and original-index/shuffle correspondence. Endpoint+bit transcript plus public algorithm rules determine pivot ranks, candidate/residual-rank state and later addresses. This public adaptation is part of L and the security simulator must account for it; it is not evidence of privacy by itself.

## 6. Actual DCF ABI, pool, and joint-view proof status

The review is source-bound to the actual `VFSS/src/moe_topk/protocol_i_ucmp.cpp`, `VFSS/ext/FSS/dcf.cpp`, and S8's Select5 package/runtime sources. `ProtocolIUcmpMaterial(b, ml, mr)` validates `34≤b≤53`, defines the ring mask, computes `alpha=(ml-mr) mod 2^b`, and calls `keyGenDCF(b,64,alpha,1)`. In the selected size range, the Select stable-key width is 40 bits at padded N=128 and 41 bits at N=256. `keyGenDCF` draws the initial two blocks from the configured FSS PRNG, generates party seed-chain blocks `k[0..b]`, and AES-expands MSB-first tree steps into correction values `g` and `v`. This ABI returns different `k0/k1` chains but places the same generated `g0/v0` correction arrays into both DCFKeyPack records (source line 244); each party serializes its local record as `M2UC/v1`, including party id, Bin/Bout/groupSize, and the lengths and contents of `k`, `g`, and `v`. The key-generation loop performs two four-block AES expansions per input bit, so a proof must account for `Q_keygen = 8·Σ_j Bin_j` AES block evaluations over all DCF slot generations (plus any other construction PRG calls); any PRG hybrid must state `Adv_AES(Q)` for the actual query count and parameters, not merely invoke “AES security”. `eval_strict_lt` forms `x=(z_left-z_right) mod 2^b`, `y=(x-2^(b-1)) mod 2^b`, evaluates the local key at both, applies the party-specific wrap correction, and returns an additive share of strict `<`. The Select runtime opens the approved masked operands, evaluates each local material, and opens the resulting comparison bit. Membership uses `1−strict_lt(tau,key_i)` for the stable unique packed key and then inverse-routes the output shares.

The dealer makes independent per-comparison `mask_left,mask_right` values, shares each into party package fields, and derives the DCF threshold from their difference. The opened `z` values are sums of each party's key-share and corresponding local mask share. Thus public masked operands and local DCF/mask package fields occur jointly in each party view; “each z is individually masked” does not establish a simulator for that conditional joint distribution. In addition, public comparison bits select later endpoint/address accesses and candidate sizes. One-shot slot binding and TLS authenticate the intended material/message; neither proves that full adaptive pool view is simulatable.

Two material schemas are audited separately:

| Schema | Full local logical inventory | Security-specific concern |
|---|---|---|
| M6B5/v2 monolithic, padded N=128 | 57,280 DCF key-pair slots: 40,640 Select (`5*C(128,2)`), 16,256 retained unused all-pairs M2CP shuffle pool (`2*C(128,2)`), 384 input-conversion/membership slots (`3N`). | The unused M2CP pools use a shared `r` vector. For a triangle, `alpha_ij=r_i−r_j`, `alpha_jk=r_j−r_k`, `alpha_ki=r_k−r_i`, so the latent sum is exactly zero modulo the ring. This is a correlation witness in the generated threshold vector, not a party-view distinguisher because α is not sent. A simulator must cover the entire monolithic serialized pool, including unused slots and mask/shuffle fields. |
| M6S2 streamed, padded N=256 | 163,968 DCF slots (`5*C(256,2)+3N`) plus 2,560 shuffle metadata slot addresses (`10N`) = 166,528 addresses per party; 11 segments and manifest/segment digests/access/lease state. | No retained all-pairs M2CP DCF pool; requires a separate simulation of the streamed manifest, digests, per-slot material, unused entries, adaptive access and durable lease transcript. |

Threat model: T is trusted, input-independent, prepares both packages (and knows its own permutation/material-generation randomness) and exits before online execution; T does not collude with either online party. P0/P1 communicate over the authenticated private transport. At most one online party is semi-honest corrupted. Malicious parties, T-party collusion, host compromise, admin rollback/deletion, production filesystem semantics and atomic external publication under crash are outside this proof claim.

For either corrupted party `P_j`, a valid simulator may use only that party's ideal input/output and approved `L`. A complete proof needs to sample jointly: its input-conversion and shuffle shares; entire serialized local DCF key and mask pool (used and unused); the adaptive address sequence induced by public endpoints/bits; observed masked operands; its local output mask share; and abort/restart/timeout prefixes. The natural hybrid would idealize TLS/OS randomness, replace AES expansion with its PRG security game, then replace the real local DCF pool and correlated masks, simulate adaptive opened operands/bits, and finally compose forward shuffle, membership, inverse route, output shares and ABORT. Required losses would include an actual AES PRP/PRG advantage for the total query count, OS entropy assumption, channel authentication/privacy advantage and a schema-specific pool-composition term. No reduction or bound for the latter is established.

The first unclosed step is a concrete missing lemma, not a claimed attack: for each party and each package schema, show that the serialized local `M2UC` key shares and mask shares for the complete correlated pool are simulatable from the party's ideal input/output plus L, conditioned jointly on opened `z` operands, comparison bits and adaptively selected slot addresses, including unused slots and abort prefixes. Single-key correctness/conformance is not that lemma; AES pseudorandomness alone does not address key-generation correlations or the conditioning. Without this starting distribution, the shuffle/inverse/membership/output/ABORT hybrids cannot be completed. **No distinguisher was found, but the proof is not closed.**

`DCF_SINGLE_KEY_PRIVACY=OPEN`, `ADAPTIVE_FULL_POOL_VIEW=OPEN`, `SHUFFLE_OUTPUT_COMPOSITION=OPEN`, `JOINT_PARTY_VIEW=OPEN`; do not claim PASS or conditional security. `SECURE_ALIAS_READY=NO`.

## 7. Gates and project state

| Gate | S10 result |
|---|---|
| `S8_SOURCE_RECEIPT` | `PASS_EXACT_MANIFEST_SNAPSHOT` |
| `FUNCTIONAL_E2E_N256` | `PASS_BOUNDED_ORACLE_CHECKED` |
| `SAMPLE_SEED_AFTER_SHUFFLE` | `PASS_SOURCE_AND_PHASE_REVIEW` |
| `S8_COUNTER_RECONCILIATION` | `PASS_ERRATUM_ISSUED` |
| `BUILD_PROVENANCE` | `PARTIAL_PATH_NORMALIZED; ORIGINAL_HASHES_DIFFER` |
| `DCF_SINGLE_KEY_PRIVACY` | `OPEN` |
| `ADAPTIVE_FULL_POOL_VIEW` | `OPEN` |
| `SHUFFLE_OUTPUT_COMPOSITION` | `OPEN` |
| `JOINT_PARTY_VIEW` | `OPEN_NO_CONCRETE_COUNTEREXAMPLE_FOUND` |
| `SECURE_ALIAS_READY` | `NO` |
| `N_GT_256_ADMISSION` | `NO_GO` |
| `V3_V4_PERFORMANCE_STATUS` | `V3=OPEN; V4=NOT_ALLOWED/NOT_RUN` |

No code defect was reproduced. Runtime changes: **none**. BB90 source status remains S1 `NO_GO_FOR_IMPLEMENTATION`; this functional project-derived route is not called BB90 and does not replace its comparison item. BMW16-derived Select remains a separate candidate. N>256 remains rejected. The official project performance gates are unchanged: V3 is OPEN; V4 requires the existing V3 completion plus the plan's fixed BB90 source/version/parameter/probability and complete combination prerequisites. Therefore formal LAN/WAN is `NOT_ALLOWED/NOT_RUN`; no formal matrix was launched and no timing conclusions are made.

## 8. Evidence paths and hashes

Independent receipt evidence directory: `C:\Users\28641\.codex\evidence\S10_INDEPENDENT_RECEIPT_2026-10-15`.

- Manifest-copy ledger `S10_RECONSTRUCTED_FILE_HASHES.csv`: SHA-256 `d72e16eeacc5fcc1501827060718a28c5f6eaacae6c3547826e39ceb4d2a5a40`.
- Reconstruction transcript: `S10_RECONSTRUCTION_TRANSCRIPT.txt`, SHA-256 `a548cd55214b1f9fbc6ed9760f163765fd70f44f9ec23757a07067485dc8ee0e`.
- Successful seven-target replay: `S10_RELEASE_REPLAY_ATTEMPT2_RAW.log`, SHA-256 `9d654defbd3016fe655066e9506b082bb744856a79241bd1ec4407f4a89560ff`; replay script SHA-256 `20ea2d892116366b1d80d7396cdaca5d6e5c24ed8aa196c177b069f0a08a9943`.
- First release wrapper failure was retained as `S10_RELEASE_REPLAY_RAW.log`, SHA-256 `c45ba2f2da1cfc35bd44ed39b8c1e6e7056c1b9a3b092a58605ef9a8fef8e1d6`; it stopped at Git worktree inspection before configure.
- Feature-OFF replay log `S10_FEATURE_OFF_REPLAY_RAW.log`, SHA-256 `671aa75c0c5ea7c6d5c6010f32131ed564c65e2755fff5a70ebc84744c0b5067`; target-help capture SHA-256 `99b3577f26fd7860a9c7cbe6dbb20a0ba25d2c3263af10808062f0a1a427e79d`. OFF build script: `D4260A1BE248FFF71A9A236EDFD4C676D644696373A622BA827F13E5EA882CCF`.
- S8/S9 normalized binary provenance is documented in [the machine-readable provenance ledger](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S10_BUILD_PROVENANCE_2026-10-15.json). S8 case JSON and recomputation output are pinned by the erratum and source manifest. The independent case JSON additionally records oracle source SHA-256 `270ed8e3c3c8eef3fb143641d376d4fae7726e14621cc1274c717c2620a12e22` and E2E source SHA-256 `862c0ca04c251c1d286bc63e3de60d420fa72120f220734e30a3334803c13407`.

The erroneous initial manifest-hash script invocation in the S10 reconstruction wrapper was followed by a successful exact hash ledger; the WSL Git-pointer failure above was retained. There were no failed functional cases in the final independent replay. Historical S8 initial Ubuntu 20.04/CMake 3.16 configure, malformed test-hook attempt, and stale incomplete E2E remain as recorded in S8; S10 did not erase or relabel them.






Source review entry points in the received tree: `VFSS/src/moe_topk/protocol_i_ucmp.cpp:6-14` (alpha construction, local x/y eval, M2UC serialization), `VFSS/ext/FSS/dcf.cpp:121-244` (actual key generation), `VFSS/src/moe_topk/protocol_i_project_derived_select5.cpp:1174-1204,1325-1353` (fresh mask/key slots), `VFSS/src/moe_topk/protocol_i_project_derived_select5.cpp:1677-1822` (shuffle-complete state, per-active-round seed and adaptive Select calls), and `VFSS/src/moe_topk/protocol_i_project_derived_select5.cpp:1828-1854` (membership operands and `1-strict_lt` composition).
