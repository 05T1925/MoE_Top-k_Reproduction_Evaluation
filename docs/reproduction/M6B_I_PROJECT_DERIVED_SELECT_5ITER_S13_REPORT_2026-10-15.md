# M6B-I PROJECT_DERIVED_SELECT_5ITER S13 report

Date: 2026-10-15
Route: `Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / EXPERIMENTAL`
Decision: preserve the opt-in experimental route; no `secure` alias. This remains a project-derived stable Top-K implementation, not BB90 or BMW16.

## 1. Outcome

S12's exact dirty-worktree source set was reconstructed and checked against its 58-file manifest. The selected Release targets and six directed CTest cases passed in a fresh build, the TEST_ONLY three-process harness reported 18 oracle-matching cases, six uniform abort/no-mask cases and six crash-prefix cases without a complete mask, and the feature-OFF build registered no Select5 tests. These results are a **same-session technical replay**, not an independent receipt: this report's author participated in the S12 runtime implementation. S10's independent receipt covered S8 only.

The current DCF cannot be bridged directly to Boyle et al.'s optimized DCF theorem. For `Bout=64, groupSize=1`, the actual terminal `Convert_G` receives a 128-bit state with two low bits fixed, so its output lies in a `2^62`-element subset of the `2^64` group (the actual distribution may have smaller support). Figure 1's conversion for this group returns the first 64 bits of a uniform 128-bit input, which is exactly uniform. The `y mod 4` probability gap is a theorem-to-code mapping witness, not a demonstrated distinguisher on a serialized party key and not an attack on DCF. The separate restricted-AES-key and initial-control-bit differences also remain unproved. Accordingly, `DCF_SINGLE_KEY_PRIVACY`, the correlated-pool/adaptive hybrids, and the full shuffle/output party-view simulation remain OPEN.

The report specifies a versioned replacement design candidate, `DCF_BCG21_FIG1_AES128_V1`, that would port Figure 1 with a full-domain AES-PRP-based PRG, exact group conversion, separate root seeds and theorem-prescribed initial control bits. It is a design only: no ABI or runtime code was changed, no party material was regenerated, and it has not passed conformance, integration, or independent security review. The current route therefore stays EXPERIMENTAL.

## 2. Receipt object, independence, and provenance

S12's worktree was `C:\Users\28641\.codex\worktrees\m6b-project-select5-s12\MoE_Top-k_Reproduction_Evaluation`, branch `codex/m6b-project-select5-s12`, based on `c3926c68fd14f270faa8b55234311071947fa080`, with no task commit and no merge to `main`. The S12 source/build manifest is [M6B_I_PROJECT_DERIVED_SELECT_5ITER_S12_SOURCE_MANIFEST_2026-10-15.json](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S12_SOURCE_MANIFEST_2026-10-15.json), SHA-256 `e333b5a7bda6b067ca3a626946c8bfa34c53fdade64f8bd4cb61bb626d3a78e2`. Its selected 58-file set was reconstructed in the isolated S13 worktree and matched canonical snapshot digest `dcfd9bccda57edb0cfbc4c953ae5fbd640535cd67dfba59756c2aae6949e4188`; the seven-file S12 runtime delta digest is `ac783df2c822a2d157cd0a24c5351c41969a9bc020bcaae1e582e023ee8be3f3`. The manifest distinguishes 48 files byte-identical to S8, three inherited S10/S11 document overlays, and seven S12 runtime/store/test deltas.

The current reviewer implemented S12 and participated in the S4–S12 work. This fails the task's independent-recipient condition. S13 therefore records `S12_SOURCE_RECEIPT=HASH_RECONSTRUCTED` and `INDEPENDENT_RECEIPT=NOT_SATISFIED`; no result below is described as independent S12 acceptance. The S12 source set is not presumed to be in `main`.

The actual `VFSS/ext/FSS/dcf.cpp` dependency is outside the selected 58 paths, is Git base blob `b85fc93d854f5a3c9875501acb05145c5a9dff32`, and has SHA-256 `1e62cf39a5985e761f5bb7e18e20884835dfe6ac08a8bf55f69e34b1b1b3e0e8`. The selected source manifest, DCF dependency, build identities and logs are captured by [the S13 source/build manifest](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S13_SOURCE_BUILD_MANIFEST_2026-10-15.json), SHA-256 `393615f686786244cd9e87172e9b824b39bdc2a33369ca6b40e50e909dbd166c`.

The replay ran in a separate worktree at `C:\Users\28641\.codex\worktrees\m6b-project-select5-s13\MoE_Top-k_Reproduction_Evaluation`, branch `codex/m6b-project-select5-s13`, HEAD `c3926c68fd14f270faa8b55234311071947fa080`. No S13 runtime source was modified. The desktop main checkout and `VFSS-baseline/` were not changed.

## 3. Build and replay

Environment: Ubuntu 20.04.6 under WSL; GCC/G++ 9.4.0; OpenSSL 1.1.1f; CMake/CTest 3.31.6; Python 3.8.10. The Release cache SHA-256 is `1edfd43fd5f69645fa4734d6f5471c5881e26f76d911ac96f3ac3ab0c327f5d2`; the fresh feature-OFF cache SHA-256 is `ff0282595c3c1a328248212a5074a542c07fe6536ed8f68c1b53bb774cdef531`.

Commands executed:

```text
cmake -S <repo>/VFSS -B <evidence>/release-build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EMP_OT=OFF -DMOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=ON
cmake --build <evidence>/release-build --parallel 2
cmake --build <evidence>/release-build --parallel 2 --target moe_topk_project_derived_select5_conformance_test moe_topk_project_derived_select5_material_store_test moe_topk_project_derived_select5_process_e2e_test moe_topk_m2_ucmp_conformance_test moe_topk_m2_parallel_shuffle_conformance_test moe_topk_m2_parallel_shuffle_process_e2e_test moe_topk_streamed_material_slot_store_test moe_topk_tls13_transport_conformance_test
wsl.exe -u root -e /home/lwt2024212936/.local/lib/python3.8/site-packages/cmake/data/bin/ctest --test-dir /mnt/c/Users/28641/.codex/evidence/M6B_I_S13_20261015_RUN01/release-build -R "moe_topk_project_derived_select5_conformance_test|moe_topk_project_derived_select5_material_store_test|moe_topk_project_derived_select5_process_e2e_test|moe_topk_m2_ucmp_conformance_test|moe_topk_m2_parallel_shuffle_conformance_test|moe_topk_m2_parallel_shuffle_process_e2e_test" --output-on-failure -V
cmake -S <repo>/VFSS -B <evidence>/feature-off-build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EMP_OT=OFF -DMOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=OFF
ctest --test-dir <feature-off-build> -R project_derived_select5 -N
<release-build>/moe_topk_streamed_material_slot_store_test
<release-build>/moe_topk_tls13_transport_conformance_test
```

The first all-target build attempt addressed a directory without a Makefile and failed; the corrected all-target invocation then failed at the unrelated `bitpack_test` link (`undefined reference to bitpack::mod(unsigned long,int)`). Both logs are retained. This does not invalidate the individually built Select5 targets, but the report does not claim the whole repository built. The eight requested binaries built successfully. The first non-root directed CTest run skipped the Select5 process E2E because its trusted supervisor must create distinct party UIDs; the later root-supervised run with the configured CTest 3.31.6 binary passed all six targets in 418.06 seconds. Direct streamed-store and TLS 1.3 mTLS tests passed. Feature-OFF CTest listing returned `Total Tests: 0`. A system CTest 3.16 invocation misused `--test-dir`; its result was discarded and not counted.

The selected test executable hashes are:

| Binary | SHA-256 | Bytes |
|---|---|---:|
| `moe_topk_project_derived_select5_conformance_test` | `86d9c922bf8966ec5865b845ab49750b78cb84c01a0cbd8ab4f73b76f296e964` | 297,512 |
| `moe_topk_project_derived_select5_material_store_test` | `0e79806bfa727f0e63377697cc3329ecabb49777d579863430974ce2902f6971` | 537,608 |
| `moe_topk_project_derived_select5_process_e2e_test` | `27c258ab49138daa4892aaf720b01c8a5797c64760a97b04f9b820e97f27a5dc` | 632,016 |
| `moe_topk_m2_ucmp_conformance_test` | `6d8909ec00bd53652fcdb8e229b3965d4f6653d5cf85db72538b91745556f089` | 187,280 |
| `moe_topk_m2_parallel_shuffle_conformance_test` | `af3dd24f7bf3565140f2a3f56677d52a8621d3a4b5bd0c7c954f419be06a4933` | 309,232 |
| `moe_topk_m2_parallel_shuffle_process_e2e_test` | `da79a480dc4821b71eecb78b44dff8419b19b741bc24bf143c4c0a767884245c` | 344,304 |
| `moe_topk_streamed_material_slot_store_test` | `0ea3fe45a99ae2f96e33065241297c0184ee8e687406fe72196ca65be3c1c7c8` | 98,392 |
| `moe_topk_tls13_transport_conformance_test` | `94a237fdc948f9eae1c5eb9cd26a95f7a0c08d85f50fdcc7f1e8b6fe149acdc8` | 68,104 |

All fresh logs are under the unique, external evidence directory `C:\Users\28641\.codex\evidence\M6B_I_S13_20261015_RUN01`. The directed CTest log SHA-256 is `11f81bea0cff18f7e2f9d22ae74ec17c428328b4bd476607a76e1c8b0573afbc`; every build/configuration/direct-test log hash is in the S13 manifest. The extracted, machine-readable case set is [S13 replay cases](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S13_REPLAY_CASES_2026-10-15.json), SHA-256 `e1afd5e91374bf8fbada5ff06b5adb54d550c4f63a9b154e45a2db3fa0de6339`.

## 4. Functional evidence and limits

The TEST_ONLY T/P0/P1 harness emitted 24 functional cases: 18 `ORACLE_PASS` and six `UNIFORM_ABORT_NO_MASK`. The harness's trusted TEST_ONLY supervisor combines P0/P1 XOR shares and compares every success bit-for-bit with the frozen C++ `top_k_mask()` oracle, while checking original order, output length, binary values and weight `K`. The production party API itself returns a party-local XOR share, not a plaintext mask. The harness and oracle check were source-reviewed and rerun here, but the current author's S12 involvement means this is not an independent acceptance.

Successful cases covered `N=1,K=1`; `N=7,K=1/7`; seven `N=128` configurations (`K=1,2,8,64,65,128`, with a second `K=64` sample); and eight `N=256` configurations (`K=1,2,8,64,128,255,256`, with a second `K=128` sample). They preserve the signed Q20.12 descending score order, `original_index` ascending tie rule and original-input-order mask. Six fault cases cover package tampering, peer disconnect, party/K binding mismatch, wrong TLS peer identity, T exit before ready, and invalid sample-seed magic; each returns no mask. The runtime's seed-order trace asserts forward-shuffle completion before phase `40+i` seed send/receive; this is the instrumented test trace, not a passive packet capture.

Six crash-prefix cases cover termination after run lease, prepared marker, PREPARE exchange, commit marker, COMMIT exchange and published marker. All are `NO_COMPLETE_MASK`. Prefixes 1–4 produced no output bytes. Prefixes 5–6 left one 8-byte P1 local XOR share after P0 crashed; no complete mask was delivered. This supports only the tested **no-complete-mask** property under the TEST_ONLY trusted supervisor and local POSIX setup. It does not claim atomic two-party publication or all-or-nothing output under arbitrary crashes.

Preflight continued to reject `N=256` monolithic generation and `N=257` streamed use. The tested N=128 package schema is `M6B5/v2` with an 88-byte party-bound `M6P2/v1` marker; it stores 57,280 DCF slots per party: 40,640 Select, 256 score conversion, 128 membership and a 16,256-slot unused M2CP pool. The tested N=256 layout is streamed `M6S2`: 166,528 slot records per party (163,968 DCF records plus 2,560 shuffle metadata records), 11 segments, 12,656,192-byte manifest and 185,925,312 bytes total staged storage per party. These schemas have distinct serialization, addressing and lease state; no argument below treats them as one package.

Logical comparison-call counts are diagnostics for this fresh run, not time/performance measurements or asymptotic bounds. For the seven successful N=128 cases: min/median/max = `309/383/3,978`; for eight successful N=256 cases: `603/628/14,709`. Each logical `eval_strict_lt` performs two DCF evaluations per party at shifted inputs; E2E comparison-call count is not the DCF Eval count. Five algorithm iterations, up to nine public comparison-dependency layers, and complete path message exchanges are distinct quantities. This replay observed 13–32 online exchanges across its cases; it does not claim the three-round Protocol I paper core. The varying calls/exchanges reflect a new runtime seed/tape and are not a correction to S12's earlier run.

## 5. Actual DCF versus the theorem

The audited source is the actual dependency `VFSS/ext/FSS/dcf.cpp`, unchanged at the S12 source base; uCMP is `VFSS/src/moe_topk/protocol_i_ucmp.cpp`. The project wrapper constructs `alpha=(m_left-m_right) mod 2^b`, `beta=1`, with input ring width `b=34..53`, output `Z/(2^64)` and `groupSize=1`. It evaluates the same party key at `x=(z_left-z_right) mod 2^b` and `y=x-2^(b-1) mod 2^b`, then applies a wrap correction to realize the strict comparison share. The key serializes the local root/correction blocks and group words but not a plaintext `alpha` field. This establishes function direction and source behavior only, not privacy.

Boyle, Chandran, Gilboa, Gupta, Ishai, Kumar and Rathee, *Function Secret Sharing for Mixed-Mode and Fixed-Point Secure Computation*, EUROCRYPT 2021 full version, is the primary theorem source: Figure 1 and Theorem 2 are on PDF pp. 12–13; its hybrid proof is Appendix F.1, pp. 42–45 ([official full version](https://www.microsoft.com/en-us/research/wp-content/uploads/2021/01/FSS.pdf)). For `|G|=2^k` with `k≤λ`, `Convert_G` returns the first `k` bits of its λ-bit input. Figure 1 samples two independent λ-bit root seeds and fixes root controls to `t_0=0,t_1=1`. The proof replaces correction words level-by-level using the assumed PRG; its per-key result is pseudorandomness of one party's DCF key for fixed `alpha,beta`.

The current code does not map to that exact distribution:

1. `convert()` at the beginning of `dcf.cpp` reads the first eight bytes as a host-endian `uint64_t` when `Bout=64, groupSize=1`. The terminal call in key generation and evaluation passes `s & notThreeBlock`, where `notThreeBlock=toBlock(~0,~3)` fixes two bits of the 128-bit input block. On the tested x86_64 little-endian platform those are the low two bits of the low lane copied by `convert()`. The masked input therefore lies in a `2^126`-element subset of the 128-bit domain, and the group output lies in the subset with `y mod 4 = 0`, whose cardinality is `2^62`; the actual internal distribution may have smaller support. This event has probability 1 under the code mapping and probability 1/4 for a uniform element of `Z/(2^64)`. This is the first theorem-mapping break: the direct uniform-input premise for `Convert_G` is not met. It is not a party-key distinguisher because the `s` value is an internal/correlated terminal state, not directly given as a field to the other party.
2. The current code creates AES input keys as `(state & notThreeBlock)`, restricting the AES-key family to two low zero bits. The generic theorem's PRG input is a uniform λ-bit string; ordinary AES-128 PRP security for uniform keys does not alone state the needed restricted-key PRG property. A reduction for this key subfamily and its correlation with the in-block control bits is missing.
3. Current root state construction makes the two embedded low control bits complementary, with a random orientation, rather than storing the paper's separate fixed `(0,1)` controls. That may admit a clean relabeling proof, but no such mapping is included here. It cannot be assumed by matching DCF function outputs.
4. `FSSConfig::prngs[thread].get<array<block,2>>()` supplies roots. The mapping from this generator's actual seed/expansion to independent uniform per-slot roots and a concrete AES/PRG advantage has not been quantified in the S12 ABI proof.

The event in item 1 proves that the **direct input-distribution precondition used for the theorem's group conversion is false** on the measured platform. It does not prove a break of DCF, does not show a key distinguisher, and is not a correctness failure. No concrete party-view attack was found; **未发现反例，但证明未闭合**. A valid current-ABI proof would need (i) a lemma for the restricted terminal conversion and its dependence on prior correction words, (ii) a restricted-AES-key PRG/reduction, (iii) a reduction handling embedded/complementary initial controls, (iv) quantified root PRNG security, and then an independent check of the resulting joint distribution. S13 does not claim A.

## 6. Versioned replacement candidate and cost impact

The shortest auditable alternative is a separate exact port of the theorem construction, labeled `DCF_BCG21_FIG1_AES128_V1`. It is **design-only / NOT_IMPLEMENTED / NOT_INTEGRATED / NOT_SECURITY_ACCEPTED**.

| Field | Candidate contract |
|---|---|
| Source | Boyle et al., official full version Figure 1, Theorem 2 and Appendix F.1, not the current CryptoTools key ABI |
| λ / input / output | λ=128; MSB-first `b`-bit domain (current N≤256 Select widths 34/40/41); `Z/(2^64)` output; `beta=1` for `x<alpha` |
| PRG | `G_AES128_5BLOCK`: uniform 128-bit AES key, encrypt five fixed distinct 128-bit counters 0…4, concatenate, take exactly the first 514 bits MSB-first |
| Convert | For `Z/(2^64)`, first 64 bits of an independent uniform 128-bit input; exact uniform conversion (`epsilon_Convert=0`) |
| Root | Independent full-width uniform root seeds; separate control bits set exactly `t0=0,t1=1` |
| ABI | New magic/version (proposed `M2UC/v2`), explicit endianness, widths/counts, root seed, `Bin` correction words (`16-byte seedCW + 8-byte valueCW + two control bits packed in one byte`), final 8-byte group correction; old `M2UC/v1` is rejected |

For the proposed PRG, AES-128 PRP security at five distinct queries yields the design bound `epsilon_G ≤ Adv_PRP_AES(T+poly(λ),5) + 10/2^128`: the second term bounds replacing five outputs of a random permutation with five independent uniform blocks; truncation to 514 bits cannot increase distinguishing advantage. Figure 1/Theorem 2's hybrid has `b` PRG transitions and one `Convert_G` transition. For a distinguisher within the theorem's runtime loss, the candidate key-only bound is `b * epsilon_G + epsilon_Convert`, where exact uniform truncation gives `epsilon_Convert=0`; the reduction's runtime overhead is absorbed in the PRG advantage's time argument. Exact constants still require review against the eventual port. A conservative whole-pool budget is `sum_j b_j * epsilon_G`, where `j` ranges over every issued DCF key, **including unused slots**. Using max `b=53` gives factors `3,035,840 * epsilon_G` for the N=128 57,280-key pool and `8,690,304 * epsilon_G` for the N=256 163,968-key pool. This is a key-pseudorandomness hybrid budget, not yet a simulator for opened comparisons or shuffle/output.

The intended key-level-to-pool argument for this candidate is conditional but precise: for every fixed `alpha_j,beta_j`, Figure 1's per-party key pseudorandomness is uniform in those values. If each slot uses independent ideal root coins, one may condition on the *entire* possibly correlated threshold vector and on a party's local additive mask-share vector, then replace each key in turn. This suggests that shared-mask algebraic relations do not invalidate the key-only hybrid by themselves; the loss is `Σ_j b_j ε_G` plus the root-coin source term `epsilon_root(T, Q_root)`, with `Q_root=2N_DCF` 128-bit root samples. A real OS/DRBG root source must supply a bound for that whole output stream. Replacing the whole pool before online queries is the right order for adaptive addresses because the accepted addresses are functions of L. To complete adaptive transcript simulation, a proof still must show how simulated local Eval shares and peer contributions reproduce every opened bit in L and remain consistent with the ideal output share. The pool observation here is a candidate proof outline, **not** a proof for S12 or a security acceptance for an unimplemented v2 ABI.

The theorem key-size expression is `b(λ+ell+2)+λ+ell` bits, with `ell=64`, i.e. `194b+192` bits at λ=128. If the current 33-byte `M2UC` header/count envelope is retained, the proposed byte-aligned key record is `57+25b` bytes: 33 bytes of envelope, a 16-byte root seed, `b` correction words of 25 bytes each (`16+8+1`), and an 8-byte final group correction. The current v1 record is `57+24b`, so this specific layout adds `b` bytes per DCF slot. For the known active slots only, that is +1,639,424 bytes/party at N=128 (`40,640` Select + `256` score + `128` membership keys; widths 40/34) and +6,719,104 bytes/party at N=256 (`163,200` Select + `512` score + `256` membership keys; widths 41/34). The N=128 unused M2CP pool, any other party-key records, and changed package/manifest/segment framing are not included; whole-package delta and capacity are therefore `NOT_MEASURED`. These formulas are not serialized package measurements or a capacity certification. The candidate remains design-only, so neither schema has been regenerated.

Before implementation, freeze the field-by-field Figure 1 mapping and reduction, then build an opt-in `M2UC/v2` conformance fixture. Required follow-on work is: DCF Gen/Eval conformance; exhaustive/frozen oracle differential for the current strict-less uCMP wrapper; regeneration of every N=128 M6P2/v1 and N=256 M6S2 DCF slot; package/segment and lease version bump; one-shot store and wrong-key/reuse/corruption tests; N=1/2/small all-K plus N=128/256 T/P0/P1 E2E; then non-author independent security review. S13 does not silently change the old ABI.

## 7. Pool, adaptive access, leakage and joint view

### 7.1 Approved online leakage and local-only state

The current route's peer-visible fields are the authenticated frame binding and phase/length; masked shuffle record arrays; after forward shuffle, the active iteration's 32-byte public sample seed; shuffled-coordinate endpoints and pivot positions; logical slot/address identifiers and access order; masked `z_left/z_right` operands; opened comparison bits and call count; fixed PREPARE/COMMIT/ABORT statuses; and public traffic lengths. Seed is generated only for the active iteration after forward-shuffle completion, sent in phase `40+i` and acknowledged by P1. No future-iteration seed is pre-generated in the package or ready/start binding. The sample seed is not DCF α or a DCF key.

Party-owned packages/manifests/segments, durable lease markers, unused local slots, local DCF key fields, local mask shares, local shuffle `sigma/tau`/record shares, local inverse route state, and each party's output XOR share are local to that party's protected store/API; they are not sent to the peer as raw files. The trusted offline T sees both material generations, masking state and shuffle permutation `Pi`, then exits. T does not receive scores, online ranks, pivots or output mask. T+party collusion, malicious T, malicious/active party behavior, hostile root/admin rollback, and a public-network traffic-analysis adversary are excluded. The local TEST_ONLY supervisor sees both output shares to check the oracle.

The one-byte PREPARE and COMMIT statuses are fixed values at fixed protocol phases; the ABORT status is uniform. No rank, selected position, threshold, α, DCF key, plaintext score or raw-index mapping is encoded there. They are therefore representable in the approved public transcript L as fixed control statuses, not new data-dependent disclosures. Current iteration seed and comparison results already fall within the user's approved L.

| Field/state | P0 | P1 | Trusted T / supervisor | External network observer |
|---|---|---|---|---|
| Own input share, package, used/unused DCF key fields and mask shares | Local P0 view | Local P1 view | T saw both generated packages and mask/key generation inputs before exit; supervisor handles test lifecycle | Not visible through TLS |
| Peer package, manifest, lease and ready digest | OS-denied in tested distinct-UID local POSIX model | OS-denied in tested distinct-UID local POSIX model | Trusted publisher/supervisor can administer both party stores | Not visible through TLS |
| Forward/inverse 24-byte shuffle record payload arrays | Receives peer TLS plaintext record values | Receives peer TLS plaintext record values | T generated corresponding local record shares and knows `Pi` | Record length, direction and timing only |
| Active-iteration seed, endpoints/pivot coordinates, opened comparison bits | Visible after the iteration is activated | Same | T is out of online transcript; it does not learn online scores/ranks | TLS-protected payload hidden; traffic pattern remains visible |
| Masked `z_left/z_right`, logical slot/address, comparison count, frame lengths | Visible as approved L fields | Same | No online T role | Length/direction/timing only |
| Lease marker and per-slot claim/used state | Own local file/system state only | Own local file/system state only | Supervisor may observe lifecycle state | Not visible except resulting protocol traffic |
| PREPARE/COMMIT/ABORT | Fixed phase status visible | Fixed phase status visible | Supervisor may observe process state | Frame lengths/timing only |
| Final XOR output share or no-output | Own local API result | Own local API result | TEST_ONLY supervisor can collect both shares for oracle verification; production peers do not receive the other's raw share | Not visible through TLS |
| Runtime test trace | Local/test harness trace records event, phase, iteration and payload length; seed bytes are omitted | Same | TEST_ONLY supervisor may archive trace | Not visible unless separately exported |

Frame bindings expose public session/fingerprint, logical/padded N, K, bit width, sender/receiver, phase, frame type and material ID. The online party-view claim is for one semi-honest corrupt party over private authenticated TLS channels. T+party collusion, malicious T/party, hostile administrator/root and external traffic-analysis privacy are excluded.

### 7.2 N=128 `M6B5/v2` plus `M6P2/v1`

Each party package contains its raw-score conversion shares/keys, forward and inverse shuffle party material, 40,640 Select slots across five iteration namespaces, 128 membership slots, and the 16,256-key unused M2CP pool. Each Select slot serializes two local mask shares and one party's uCMP DCF key, bound by session/purpose/iteration/direction/endpoints. M6P2/v1 publishes an 88-byte party-recipient marker with only that party's package digest. Local lease is one-shot. The entire pool is available in the party's own package view before online queries; its adaptive use trajectory is also determined by approved public bits/seed/coordinates. No proof currently simulates the complete package jointly with the local score/mask shares and public `z` values.

The unused M2CP pool has threshold relations induced by a shared vector of masks. This algebraic relation is not by itself a party distinguisher; S13 finds no executable distinguisher. Yet the required statement is stronger: show that the entire package distribution, conditioned on that party's package-local shuffle/mask shares and all exposed `z`/bits/adaptive addresses, can be generated from L and ideal output. No such conditional multi-key lemma is established. The 57,280-key pool must be bounded as a whole, including never claimed slots and crash/abort lease history.

N=128 package ledger, per party:

| Slot class | Count | Current online use / party visibility |
|---|---:|---|
| Select DCF | 40,640 | Full five unordered-edge pools are packaged; only the adaptive graph's selected slots are evaluated/leased; all unused entries remain in the party package view |
| Raw-score conversion DCF | 256 | Used by the two conversion stages for 128 padded records |
| Membership DCF | 128 | One per padded item |
| Unused M2CP DCF | 16,256 | Not consumed by this Select5 shuffle path; still present in each party package and included in the privacy obligation |
| Total DCF key slots | 57,280 | Monolithic `M6B5/v2`; 60,008,572 serialized bytes per party in the S12 case |

### 7.3 N=256 `M6S2` streamed schema

Each party's 12,656,192-byte manifest and 11 segments carry 166,528 slot records: 163,200 Select keys (five full unordered-edge pools), 512 score-conversion keys, 256 membership keys, and 2,560 shuffle metadata records. The streamed DCF pool is 163,968 keys. Per-slot durable lease binds session, material ID, party, purpose, iteration/substage, direction, endpoints or item, and digest. The peer sees slot IDs/access order through the accepted transcript, but not the other party's manifest, segment bytes or lease files under the tested distinct-UID POSIX setup. No cross-host production store, malicious root/admin, snapshot rollback, or distributed filesystem persistence was tested.

M6S2 must be simulated separately from the monolithic package. A candidate replacement would require a new manifest/segment version and regeneration of all keys; the key-level sum bound above only handles an idealized independent candidate key pool, not actual old M6S2 bytes or existing local shuffle record keys.

N=256 package ledger, per party:

| Slot class | Count | Current online use / party visibility |
|---|---:|---|
| Select DCF | 163,200 | Five full unordered-edge pools are packaged; only active adaptive endpoints are evaluated; unselected slots remain in this party's streamed store |
| Raw-score conversion DCF | 512 | Two conversion stages across 256 padded records |
| Membership DCF | 256 | One per padded item |
| Shuffle metadata records | 2,560 | Forward/inverse metadata; local shares and one-shot addresses stay in party-local M6S2 store |
| Total slots / DCF keys | 166,528 / 163,968 | 11 segments; manifest and lease are distinct M6S2 objects |

### 7.4 Why the composition proof is still open

For P0 and P1 separately, the intended simulator input is only the corrupted party's original input share, approved L and its ideal output share. Neither S12 nor S13 constructs that simulator. The first missing hybrid is the party-local DCF package distribution conditioned on the actual restricted-Convert terminal states and the local mask shares. Without that single-key bridge, replacing a chosen used key by uniform is unjustified. Even after a corrected DCF port, the proof must (i) hybrid all `N_DCF` keys including unused keys, with per-key input widths `b_j` and additive loss `Σ_j b_j ε_G`; (ii) expose only transcript-driven adaptive queries and the already-public comparison bits; (iii) prove M2CP shuffle/forward/inverse party materials separately; (iv) compose raw-score conversion, Select, membership, inverse routing, local output share, fixed status frames and crash/restart lease state. A one-key theorem alone does not establish these steps.

Current security classification: `DCF_SINGLE_KEY_PRIVACY=OPEN`; `CORRELATED_POOL_SIMULATION=OPEN`; `ADAPTIVE_QUERY_SIMULATION=OPEN`; `SHUFFLE_OUTPUT_COMPOSITION=OPEN`; `JOINT_PARTY_VIEW=OPEN`. There is no known concrete distinguishing attack in this review. The mathematical Convert_G support gap is a proof-mapping counterexample, not evidence of an attack. No conditional-security acceptance is recorded.

## 8. Gates and next actions

| Gate | S13 status | Evidence / remaining condition |
|---|---|---|
| `S12_SOURCE_RECEIPT` | `HASH_RECONSTRUCTED` | Exact 58-file manifest matched; not yet independent acceptance |
| `INDEPENDENT_RECEIPT` | `NOT_SATISFIED` | Reviewer authored S12; an eligible reviewer must rebuild and replay the manifest-frozen snapshot |
| `FUNCTIONAL_E2E_N256` | `PASS_SAME_SESSION_REPLAY` | Eight N=256 oracle successes and the full 24 functional/6 crash-prefix harness records; not independent |
| `DEFAULT_OFF` | `PASS` | Fresh OFF configuration listed zero Select5 tests |
| `DCF_SINGLE_KEY_PRIVACY` | `OPEN` | Restricted terminal Convert_G distribution breaks direct Theorem 2 mapping; other root/AES conditions also differ |
| `CORRELATED_POOL_SIMULATION` | `OPEN` | N=128 and N=256 schemas separately inventoried; no conditional whole-package simulator |
| `ADAPTIVE_QUERY_SIMULATION` | `OPEN` | Addresses are transcript-determined; no justified key-pool hybrid for actual ABI |
| `SHUFFLE_OUTPUT_COMPOSITION` | `OPEN` | Real M2CP, inverse, output and restart view not simulated |
| `CRASH_OUTPUT` | `PASS_TESTED_NO_COMPLETE_MASK` | Six prefixes; two leave one orphan 8-byte share; no all-or-nothing claim |
| `N_GT_256_ADMISSION` | `NO_GO` | N=257 preflight rejects |
| `SECURE_ALIAS_READY` | `NO` | Single-key, full-pool and composition proofs not closed or independently reviewed |
| `FORMAL_PERFORMANCE_READY` | `NOT_ALLOWED_NOT_RUN` | M6A V3 remains OPEN; V4 is NOT_ALLOWED/NOT_RUN under the existing plan |

Next reviewer should be someone who did not implement S12 or write this S13 report. The exact replay entrypoint is the S13 manifest's Release configure/build/test commands; the six-test CTest regular expression and binary names are listed in §3. First follow-on engineering candidate is the isolated `DCF_BCG21_FIG1_AES128_V1` conformance port, not a patch to current DCF or reuse of v1 packages. Only after that port has a reviewed theorem mapping and fresh key/material schemas should the correlated-pool and route-wide simulators be attempted.

No formal LAN/WAN performance matrix ran. M6A V3 stays OPEN and V4 remains NOT_ALLOWED/NOT_RUN under the existing V3→V4 prerequisites. BB90 source closure stays NO-GO; BMW16-derived remains a separate candidate. This route can be called a project-derived stable Top-K **functional** path over the tested N≤256 cases, but not a BB90 implementation, a BMW16 implementation, or a secure protocol.
