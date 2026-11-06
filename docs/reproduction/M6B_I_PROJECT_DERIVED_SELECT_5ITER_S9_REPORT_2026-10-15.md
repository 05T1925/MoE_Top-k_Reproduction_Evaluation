# M6B-I PROJECT_DERIVED_SELECT_5ITER S9 report

Date: 2026-10-15

Scope: fresh same-executor reconstruction and replay of S8 public-sample-seed version; actual DCF-pool and shuffle/output party-view audit.

Route identity: `Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / EXPERIMENTAL`.

## 1. Result

The S8-selected source snapshot was rebuilt in a new isolated worktree and every selected file hash matched the S8 manifest. A fresh Release build passed seven directed targets, a default-OFF target-registration check, and all 24 T/P0/P1 E2E cases (18 frozen-oracle matches, six uniform ABORT/no-mask). This is a fresh reproducibility run, **not independent receipt**: this executor participated in the S8 implementation/decision and earlier S4–S7 work. Therefore `S8_SOURCE_RECEIPT=NOT_INDEPENDENT` even though the replay passes.

No reproducible code defect or concrete distinguisher was found. The S8 sampling seed timing and bindings match the approved contract. The actual DCF full-pool and shuffle/output joint party-view proof remains open for both M6B5/v2 and M6S2. Keep the feature opt-in EXPERIMENTAL; do not create a secure alias. No formal LAN/WAN matrix was started.

## 2. Frozen source and workspace identity

| Item | Identity |
| --- | --- |
| S8 source base | `c3926c68fd14f270faa8b55234311071947fa080` |
| S8 worktree | `C:\Users\28641\.codex\worktrees\m6b-project-derived-5iter-s8`, branch `codex/m6b-project-derived-5iter-s8`, dirty/uncommitted |
| S8 manifest | `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S8_SOURCE_MANIFEST_2026-10-15.json`, SHA-256 `c5d29679aeaf1be3cc71da1ffc8053b33d589fd15313031137b5b79655e1dffb` |
| S7 manifest reference in S8 manifest | SHA-256 `0b1118c038cca4b1deadb4f522ff22d8358aed43d3b9f6b8367b72b21096e907` |
| S9 worktree | `C:\Users\28641\.codex\worktrees\m6b-project-derived-5iter-s9`, detached at `c3926c68fd14f270faa8b55234311071947fa080` |
| Selected copy | 59 unique files; every SHA-256 matched the S8 manifest-selected values |
| S8 commit/main status | S8 has no task commit and was not merged to main; the source identity is base revision plus hashes |
| Receipt independence | Not independent; same executor participated in S8 and S4–S7 |

The 59 copied paths comprise 55 inherited selected paths (nine of them modified by S8), three S8 report/decision/case files, and the S8 manifest. The S8 source manifest is retained unchanged. The complete per-file snapshot and S9 delta hashes are in [S9 source manifest](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S9_SOURCE_MANIFEST_2026-10-15.json).

The primary desktop checkout remained outside the S9 worktree. Its base is `c3926c68fd14f270faa8b55234311071947fa080` on `feat/m6a-performance-evaluation`; user-owned dirty files were not copied wholesale or edited. S9 edits are restricted to route governance/reporting and S9 machine records. `VFSS-baseline/`, papers, keys, certificates, and local reference projects were not modified.

## 3. Build and replay procedure

Environment: Ubuntu 24.04.4 LTS under WSL2, x86_64; CMake 3.28.3; GCC/G++ 13.3.0; OpenSSL 3.0.13. The S9 Release cache records `-O3 -DNDEBUG`, `BUILD_TESTING=ON`, `MOE_TOPK_ENABLE_EMP_OT=OFF`, `MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=ON`.

Fresh opt-in configure:

```sh
cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-project-derived-5iter-s9/VFSS \
  -B /tmp/m6b-project-derived-5iter-s9-release \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_EMP_OT=OFF \
  -DMOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=ON
```

Fresh seven-target build:

```sh
cmake --build /tmp/m6b-project-derived-5iter-s9-release --target \
  moe_topk_project_derived_select5_conformance_test \
  moe_topk_project_derived_select5_material_store_test \
  moe_topk_streamed_material_slot_store_test \
  moe_topk_tls13_transport_conformance_test \
  moe_topk_project_derived_select5_process_e2e_test \
  moe_topk_m2_ucmp_conformance_test \
  moe_topk_m2_parallel_shuffle_conformance_test -j2
```

The six conformance/store/transport binaries were run serially, followed by:

```sh
/tmp/m6b-project-derived-5iter-s9-release/moe_topk_project_derived_select5_process_e2e_test
```

A separate Release configuration set `MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=OFF`; `cmake --build /tmp/m6b-project-derived-5iter-s9-off --target help` contained no Select5, streamed-slot-store, or TLS-conformance targets. S9 external logs, exit files, hashes, and the environment are listed in the source manifest and stored under `C:\Users\28641\.codex\evidence\S9_SELECT5_RECEIPT_2026-10-15`. This directory is separate from all S8 evidence directories.

## 4. Build targets and binary provenance

All seven opt-in targets built and ran with exit code 0. S8/S9 hash comparison:

| Target | S8 SHA-256 | S9 SHA-256 | Same bytes? |
| --- | --- | --- | --- |
| Select5 control/conformance | `db90b8aa40b2b871771cd775587d62ef10d4d9b4e5539eacfe84f0b612784921` | `57d1dbe03ad224a5759e2d61ac364095aea9a2fab748a5d346c7eefef8e7f7bf` | No |
| M6B5 package store | `2c0f27c254e1058a37629dd1daf63cd55fd4e889d4264962ef8c0c70e4ed0af6` | `a5f08ee2e7942a95d1216da5e100f3f0b98aa2ae56c0a2b96a2ec999149db648` | No |
| M6S2 streamed slot store | `19e4fb9a975cffa95812426ab27ea4cb03fc5622d7502c41ad5fb53d95303129` | `19e4fb9a975cffa95812426ab27ea4cb03fc5622d7502c41ad5fb53d95303129` | Yes |
| TLS 1.3 transport | `069b9ffd089fb04c38d133d4634e1d0962fb6a5ac119563ec0d5e203c9d8cdc4` | `069b9ffd089fb04c38d133d4634e1d0962fb6a5ac119563ec0d5e203c9d8cdc4` | Yes |
| Three-process Select5 E2E | `b72bc6a9df4e389abfe4d67d450ac40ecdc77a0dfe36d7a22f3e60957af8c1d6` | `92a4483c61c9861017e6d5f90e402fb5172f91dc0488695d26ad07087efcaa1e` | No |
| uCMP/DCF conformance | `73574e88208cea2e93d30de72468b7f3ad3c47ef12cf07e15fafd03b7b74cdf6` | `ef805eeef41fc2eba6c69240070626b3719de5dcb6d636f83160d3c2c78e0f11` | No |
| Parallel shuffle conformance | `059b7272cb752ec147411105c2764830dc1b4c5bbf9fbee9cc4067ab0aac0192` | `944886f71ebd901b4e2bb7243ae187a9a1ecfd2957b9e7dbda15106426e4560a` | No |

Toolchain version, Release mode and recorded options match the S8 configuration. S9 binary strings contain the S9 absolute source path. Five of seven hashes differ; two are byte-identical. S9 did not establish that absolute path substitution explains every byte difference, so test claims are bound to the exact S9 binaries above, not to binary equivalence with S8. S8's stale first E2E binary has no preserved hash or complete log; the earlier Ubuntu 20/CMake 3.16 failure happened before compiling any executable.

## 5. Directed tests and 24-case oracle E2E

| Target | Exit | Evidence checked |
| --- | ---: | --- |
| `moe_topk_project_derived_select5_conformance_test` | 0 | Stable Select control, frozen edges, sample vector/reconstruction, seed payload, invalid/replayed/misordered seed and phase/session binding |
| `moe_topk_project_derived_select5_material_store_test` | 0 | M6B5 durable package claim, binding/integrity, restart and concurrent replay rejection |
| `moe_topk_streamed_material_slot_store_test` | 0 | M6S2 segment/manifest integrity, slot binding, one-shot lease, crash/race rejection |
| `moe_topk_tls13_transport_conformance_test` | 0 | TLS 1.3 mutual identity, authenticated frame binding and invalid peer rejection |
| `moe_topk_m2_ucmp_conformance_test` | 0 | Actual C++ DCF/uCMP ABI and strict comparison behavior |
| `moe_topk_m2_parallel_shuffle_conformance_test` | 0 | Existing parallel shuffle component behavior |
| `moe_topk_project_derived_select5_process_e2e_test` | 0 | 24 independent T/P0/P1 process cases and resource preflights |

The E2E controller creates scores after the Dealer and parties have forked. It deterministically builds the frozen signed Q20.12 bit-pattern fixture, shares the values, collects the two output shares, reconstructs only in the TEST_ONLY controller, and compares every output bit with `top_k_mask()`. Successful cases check mask shape, original order, binary values, exact Hamming weight K, bidirectional wire-counter agreement, and post-success package re-claim rejection for both parties. This oracle procedure is performed by the executable and is not merely inferred from party status strings.

Success matrix:

| N | K values | Storage schema |
| ---: | --- | --- |
| 1 | 1 | M6B5/v2 monolithic |
| 7 | 1, 7 | M6B5/v2 monolithic |
| 128 | 1, 2, 8, 64, 65, 128, repeated 64 | M6B5/v2 monolithic |
| 256 | 1, 2, 8, 64, 128, 255, 256, repeated 128 | M6S2 streamed |

Fixture scores include `INT32_MIN`, `INT32_MAX`, negative/positive neighbors, equal signed scores, and repeated zero; for positions `i≥8`, the encoded value is `(i mod 7)*4096`. Ties are resolved by original index through the packed stable key. Successful result rows are 18/18 oracle matches. Six failed cases each produce bilateral uniform ABORT/no-mask: tampered package, disconnect/timeout, mismatched party K, wrong TLS peer identity, T exits before ready, and invalid seed magic. The invalid-seed E2E fault injects the malformed frame at the first seed exchange over the live TLS-bound test endpoint, after the two forward-shuffle phases. Additional conformance negatives include wrong seed version/iteration, truncation/trailing bytes, cross-session binding, replay/skip/out-of-order iteration, pre-shuffle state rejection, and sample-size/candidate errors.

The runtime preflight row reports `N=256 monolithic=REJECTED_BEFORE_GENERATION` and `N=257 streamed=REJECTED`. These are fail-closed capacity outcomes; they do not generalize support above N=256.

### Functional comparison counts (not performance)

| Padded N | Case calls (sorted) | Min / median / max |
| ---: | --- | ---: |
| 128 | `303, 306, 308, 349, 2632, 3053, 4339` | `303 / 349 / 4339` |
| 256 | `620, 637, 666, 674, 878, 4959, 16778, 17260` | `620 / 776 / 17260` |

All counts are observed from these fresh-seed functional cases. They are not execution time, resource/performance metrics, proof of expected linear work, or a high-probability work bound. The earlier S2 expectation derivation assumes ideal independent uniform-without-replacement samples and is not inherited by the SHA-256-expanded S8 implementation.

## 6. Sample seed phase and leakage review

The runtime first completes raw-score conversion and forward shuffle phases 20/21. Only after `run_two_round_shuffle` returns does it mark the shuffle complete and enter the Select controller. The active iteration's sample callback then obtains exactly 32 bytes with `getrandom()` on P0; P1 does not generate another seed. A sample frame at phase `40+i` contains the fixed 41-byte payload `u32be(magic) || u32be(version=3) || u8(i) || seed[32]`. P1 checks exact size, magic, version and iteration and responds with a 9-byte iteration ACK. The framed header binds session, fingerprint, padded N, K, bit width, sender/receiver, phase, frame type and one-shot material ID.

There is no sample seed in M6B5/v2 or M6S2 package serialization, the M6S2 manifest, ready marker, start metadata, or initial binding. Test JSON/console output records the source label, active sample phases and frame byte counts, but not seed bytes. The field `input_share_seed_hex` in E2E case records is a deterministic TEST_ONLY input-sharing fixture seed; it is not a sample seed.

After phase `40+i`, both parties can immediately derive the current sample order and clique from the public seed and their identically updated public candidate vector. The algorithm samples `s=min(m,ceil(sqrt(m)))` entries without replacement, using the exact v3 SHA-256 stream and rejection rule specified in S8. This is the approved leakage. It does not keep the current sample hidden. At that time later iterations' seed values do not exist; later samples become predictable only after their own fresh entropy draw and publication. If an earlier iteration finds the selected item, later seeds are never generated.

The observed 41-byte seed and 9-byte ACK plus their frame headers contribute one causal exchange per active iteration. Five algorithm iterations, up to nine plaintext comparator dependency layers, conditional CA `2k+1`, Protocol I's paper-core three-round count, and the end-to-end frame exchange count remain separate quantities. For the route's current message DAG, full raw-score-to-mask exchanges are `9 + 2L + A ≤31`, where `L≤9` is the number of nonempty comparator dependency layers and `A≤4` is active sample-seed exchanges. This is a project-specific candidate count, not a Protocol I paper-core claim.

## 7. Material and actual DCF audit

### M6B5/v2 monolithic, N≤128

The existing Dealer serializes one M6B5/v2 package for each party and an authenticated ready record binds package size/hash plus session, fingerprint, material ID, logical/padded N, K and comparison width. The N=128 E2E package measurement is 60,008,572 bytes per party, with both party packages totaling 120,017,144 bytes. The source-derived DCF inventory is:

- `5*C(128,2)=40,640` Select edge DCF key pairs across five iteration pools;
- `2*C(128,2)=16,256` forward/inverse M2CP shuffle edge DCF key pairs retained in the monolithic materials but not evaluated by the Select5 two-round metadata-only route;
- `2N+N=384` raw-score conversion and membership DCF key pairs;
- total `57,280` local key-pair slots per party view by layout accounting.

The two shuffle pools are still part of a party's complete local package view even if unused. Their thresholds are derived from the common per-shuffle `r` vector, so their local keys and r shares contain cross-edge cycle relations. No simulation argument currently replaces this full view.

### M6S2 streamed, N≤256

The streamed Dealer writes/finalizes per-party segments and manifest, publishes ready only after successful staging, and exits. P0/P1 lease exactly the addressed slot; stream claims are one-shot and fail closed on digest/binding mismatch, replay, concurrency or tested crash/restart. The N=256 E2E records 185,925,312 staged bytes per party, including a 12,656,192-byte manifest, 11 segments per party, and 371,850,624 staged bytes for both parties. This is disk/staging evidence, not memory or wire size.

For padded N=256, the Select5 pool has `5*C(256,2)=163,200` edge DCF key slots. Input conversion and membership contribute `3N=768`, for `163,968` DCF slots. Metadata-only forward/inverse shuffle contributes `10N=2,560` typed addresses, giving 166,528 total addresses. This schema does not serialize the two monolithic M2CP all-pairs DCF pools. The party still sees its whole local manifest, per-slot digests, all slot names and whether each address was opened/leased, including unused addresses.

The two schema views are not equivalent: M6B5/v2 is a monolithic package with additional unused correlated DCF keys; M6S2 is a streamed namespace with per-slot lease and digest state but no such all-pairs DCF pool. A security proof for one cannot be carried to the other.

### uCMP equations and use

In `VFSS/src/moe_topk/protocol_i_ucmp.cpp`, dealer masks are `m_l,m_r∈Z_(2^b)` and the actual key generation call is `keyGenDCF(b,64,(m_l-m_r) mod 2^b,1)`. Each party material contains its local `(k,g,v)` key and mask shares. At runtime each party sends the masked local values `key_l+m_l_share` and `key_r+m_r_share`; the peer's values are received and added, giving the agreed masked operands `z_l=key_l+m_l`, `z_r=key_r+m_r`. The local DCF wrapper evaluates the actual party key at `x=(z_l-z_r) mod 2^b` and `y=(x-2^(b-1)) mod 2^b`, combines the two shares with the party-1 correction and returns one strict-`<` share. The two parties open the bit by adding output shares; values outside `{0,1}` abort.

The DCF tree reads input bits MSB first. The comparison bit from Select is public. Stable keys are unique because the score ordering includes original index. Membership computes `1-strict_lt(tau,key_i)`, equivalent to `key_i≤tau`, then the inverse route returns the output-share bits in original order. The tester only reconstructs the two XOR shares after the protocol to check the oracle.

## 8. Leakage, trust and proof-gap analysis

The active online leakage observed in the v3 runtime is:

| Field | Producer/observer | Coordinate/time | Security treatment |
| --- | --- | --- | --- |
| 32-byte sample seed | P0 generates and sends; P0/P1 know it | At active phase `40+i`, after shuffle phase 20/21 | Included in `L_S8`; current sample graph is immediately derivable |
| Endpoints/pivots/slot IDs | Deterministic controller state from seed, candidate vector and opened bits | Shuffled coordinates, as each graph/layer is used | Included in L; no original-index permutation map sent |
| Masked operands | Both parties send local additive masked shares and receive peer share | Comparator or membership exchange; each party reconstructs `z` in the ring | Included in L by user approval; z is not the unmasked key |
| Comparator output bit | Each party sends local DCF output share; both add it | End of comparator result exchange | Public result bit included in L and drives next adaptive state/address |
| Key-slot/address labels and counts | Runtime/material reader | At each used address; unused schema metadata remains in local view | Included in L; key bytes remain private |
| Phase, frame length and status | Authenticated framed transport | Per exchange and termination | Length/phase/uniform ABORT included in L; wall-clock metadata not proved hidden |
| Local input, key/mask/shuffle shares and output share | Each party locally | Entire run | Private one-party view; simulator must generate from input/output shares and L |
| T's Pi and both-side dealer material | Trusted offline T only | Dealer generation/staging before ready; then T exits | Excluded from either party's view under no-collusion model |

The approved L does not include raw scores, stable threshold, pivot values, original-index-to-shuffled-position map, DCF α or key bytes, mask shares, or the complete output mask. T/party collusion and active faults are not covered by the stated semi-honest model. TLS 1.3 mTLS provides an authenticated private-channel engineering premise; it is not a proof of the FSS transcript.

### P0 simulator attempt

Intended simulator inputs are P0's ideal input share, P0's ideal output share, and `L_S8`. It can sample public seed values and derive the disclosed current sample from the public candidate state in L. It can emit public phase/length/address fields. It cannot currently sample P0's real package/key shares and mask shares jointly conditioned on the local input share, all used/unused DCF pool entries, the opened masked values/bits, adaptive addresses, local shuffle state and output share without using a real transcript. No applicable whole-pool DCF lemma is identified for either serialization. Therefore the next hybrid step is unproved.

### P1 simulator attempt

Intended inputs are P1's ideal input share, P1's ideal output share, and the same L. The public seed and prior public bits determine P1's current candidate/sample schedule. The local M6B5/v2 package still has unused correlated shuffle DCF keys and threshold/r-share relations; the M6S2 view contains the manifest, all slot digests and access/lease history. The actual local key/mask/operand/bit conditional distribution and full adaptive access trace cannot be generated from only P1 inputs/output and L using an established lemma. As for P0, the first replacement hybrid is missing.

The remaining composition would have to cover the actual two forward messages, Select input/bit exchanges, threshold membership, inverse route, final local XOR output share, success status, bilateral abort, timeout and durable restart prefix. Since the DCF full-pool distribution is not simulated, no later shuffle/output hybrid is claimed. AES-based FSS PRG/PRF security, OS CSPRNG behavior and TLS channel security are assumptions that would enter a future reduction; S9 establishes no numeric `Adv_AES(Q)`, `ε_OS`, `ε_TLS`, `ε_pool` or `ε_shuffle/output` bound. **No concrete attack was found; the proof is not closed.**

## 9. One-shot lease, authenticated transport and supported range

M6B5/v2 package claim and M6S2 per-slot lease tests both passed. Covered local POSIX behavior includes wrong session/party/N/K/width or purpose bindings, integrity/digest mismatch, duplicate/concurrent claims, crash/restart lease persistence and fail-closed behavior. E2E success cases attempt a second claim from both parties and verify rejection. Faulted runs do not release any mask. Store administrator deletion/rollback, production filesystem/network shares, cross-host durability and T supervisor operational reliability remain excluded or not measured.

The TLS 1.3 mTLS conformance and end-to-end wrong-identity case passed. Test evidence covers frame binding and wrong identity; certificate issuance, rotation, revocation, deployed host isolation and production TLS operations are not measured. No plaintext fallback is enabled in the runtime configuration. N≤128 is the M6B5/v2 monolithic layout; N=256 is the M6S2 streamed layout. N=257 preflight rejects; `N>256=NO_GO`.

## 10. S8 prior failures, S9 provenance limits and evidence integrity

The S8 report already records three failed attempts. S9 preserves them:

1. Unqualified WSL selected Ubuntu 20.04/CMake 3.16.3 and failed repository CMake-minimum configuration before compilation.
2. An early malformed-seed hook mixed a plaintext-test flag with TLS; config validation rejected it before seed publication.
3. An E2E attempt used a stale binary and did not produce a complete case result. Its partial raw stdout was lost when the S8 evidence directory was reused; no stale-binary SHA/source/config identity is available.

The final S8 source/build and S9 replay both use the S8 manifest-selected code and recorded flags. S9 newly records hashes for its binaries and raw stdout, but five target binaries differ from S8's final hashes and two match. Strings in S9 binaries show the S9 absolute source path. S9 did not demonstrate that path replacement alone accounts for every different byte, so all claims stop at the S9 artifact identity. The failed stale S8 binary cannot be compared.

S9 also had four non-test tooling errors, all corrected without changing runtime or fixtures: (1) the first copy script treated S8 manifest's prose `manifest_self_hash` sentinel as a digest; it stopped at that check after creating the isolated worktree and partially copying selected files, then recopied/verified all 59 paths against the actual manifest SHA-256; (2) the first multi-binary `strings` command passed empty operands through PowerShell/native quoting, so it was rerun as one direct probe and then per binary; (3) the first final whitespace-check regex accidentally treated the letter `t` as trailing whitespace, and a follow-up parser expression used an invalid `$f:$n` interpolation; both checks were corrected. These did not affect compilation or test runs. Successful copy hashes, the corrected binary-path probe, target logs and E2E log are the retained evidence.

The S8 N=128 records are inconsistent. Its case JSON gives `298,299,303,368,747,2094,2479`, whose recomputed min/median/max are `298/368/2479`. The S8 report and source manifest instead state `298/747/2479`, while the decision states `299/1825/5340`. S8 historical files are not changed here. S9 counts are parsed from S9's raw E2E `CASE_JSON` lines and saved in its own case JSON.

External evidence directory: `C:\Users\28641\.codex\evidence\S9_SELECT5_RECEIPT_2026-10-15`. It contains fresh configure/build output, six target logs and exit codes, default-OFF target list/check, full E2E raw output/exit code, and target hashes. The S9 report/case/source artifacts contain hashes but no key, package, certificate, sample seed or build binary.

## 11. Gate rulings

| Gate | Ruling | Scope |
| --- | --- | --- |
| `S8_SOURCE_RECEIPT` | `NOT_INDEPENDENT` | Exact source reconstructed and run, but this executor participated in S8 |
| `SAMPLE_SEED_AFTER_SHUFFLE` | `PASS_BOUNDED` | Source order, active-iteration guard, seed frame and malformed-frame E2E checked |
| `PER_ROUND_SEED_BINDING` | `PASS_BOUNDED` | Session/phase/iteration/parameters/channel/material binding and negatives checked |
| `FUNCTIONAL_E2E_N256` | `PASS_BOUNDED_SAME_EXECUTOR` | All specified N=256 K cases matched frozen stable C++ oracle |
| `MATERIAL_ONE_SHOT_AND_RESTART` | `PASS_BOUNDED_LOCAL_POSIX` | Package and slot leases, replay/crash/race/integrity tests passed |
| `DCF_SINGLE_KEY_PRIVACY` | `OPEN` | No applicable actual ABI privacy proof established |
| `ADAPTIVE_FULL_POOL_VIEW` | `OPEN` | Used/unused pool and adaptive address views unsimulated |
| `SHUFFLE_OUTPUT_COMPOSITION` | `OPEN` | No forward/inverse/membership/output/abort composition proof |
| `JOINT_PARTY_VIEW` | `OPEN` | P0 and P1 simulators not closed |
| `SECURE_ALIAS_READY` | `NO` | Route remains opt-in EXPERIMENTAL |
| `N_GT_256_ADMISSION` | `NO_GO` | N=257 fails resource preflight |
| `V3_V4_PERFORMANCE_STATUS` | `V3=OPEN; V4=NOT_ALLOWED/NOT_RUN` | Formal LAN/WAN is `NOT_ALLOWED/NOT_RUN` |

The existing M6A V3→V4 prerequisites remain exactly as recorded in the implementation plan; S9 creates no alternate gate. V3 remains OPEN pending its existing V1/V2/base-interface, both AAV86 combinations, Dealer/adaptive-material boundary, leakage/round audits and required counters. V4 remains unavailable until V3 is complete and its existing fixed BB90/version/parameter/probability, two-combination and stable-threshold/DCF requirements are satisfied.

S1 still records `BB90_5ITER_TEST_ONLY_GATE=NO_GO`; this project-derived Select does not become a BB90 or BMW16 implementation and does not replace the BB90 row in the six-route comparison. No protocol runtime code changed in S9.

## 12. Files changed by S9

S9 changed or added exactly these seven repository files:

1. `PROJECT.md` — dated S9 status and receipt/security/performance boundaries.
2. `docs/IMPLEMENTATION_PLAN.md` — S9 gate row and replay status.
3. `docs/REFERENCE_MANIFEST.md` — S9 source, functional, seed, and security record.
4. `docs/decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S9_DECISION_2026-10-15.md` — decision and gate rulings.
5. `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S9_REPORT_2026-10-15.md` — this detailed audit report.
6. `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S9_E2E_CASES_2026-10-15.json` — 24 fresh machine-readable case rows.
7. `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S9_SOURCE_MANIFEST_2026-10-15.json` — source receipt, build/binary/evidence identity, and gates.

No C++ runtime, FSS/DCF, test source, package, or transport file was modified in S9. All S1–S8 inherited dirty files in the isolated worktree remain as received. The desktop primary worktree and `VFSS-baseline/` were not changed.
