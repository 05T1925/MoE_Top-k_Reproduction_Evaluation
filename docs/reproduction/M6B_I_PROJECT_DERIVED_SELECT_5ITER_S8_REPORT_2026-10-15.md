# M6B-I PROJECT_DERIVED_SELECT_5ITER S8 report

Date: 2026-10-15  
Implementation label: `Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / EXPERIMENTAL`  
Source base: `c3926c68fd14f270faa8b55234311071947fa080`  
Worktree: `C:\Users\28641\.codex\worktrees\m6b-project-derived-5iter-s8`  
Branch: `codex/m6b-project-derived-5iter-s8`  
Decision: **S7's exact S6 snapshot remains independently received; S8 v3 sample disclosure and bounded function pass this author's regression; S8 itself is not independently received; joint party-view security is OPEN.**

## 1. Scope and receipt

S8 implements the already approved disclosure rule for the sampling randomness: publish each active iteration's random seed only after forward shuffle has completed. This updates the route's leakage function and message contract. It does not change the selection algorithm parameters or package formats. It is a project-derived construction; it is not BB90 or BMW16 and does not inherit their algorithm, complexity, probability, round or security results.

S7's S6 receipt is scoped to the exact dirty S6 snapshot identified by S7's manifest. S8 did not copy the S7 worktree wholesale. It rebuilt a selected tree from the S7 receipt and S7 source manifest. The S7 manifest SHA-256 is `0b1118c038cca4b1deadb4f522ff22d8358aed43d3b9f6b8367b72b21096e907`. The selected reconstruction comprised 55 file paths, each checked against the receipt at reconstruction. The S8 manifest records the original receipt entries and the current S8 hashes separately; nine inherited paths differ now because S8 edited them.

S8's executor modified runtime and test code and ran the S8 regression. Therefore `S8_INDEPENDENT_ACCEPTANCE=NOT_YET_RECEIVED`. A nonparticipant must replay the file manifest and commands in a separate worktree. The existing independent S7 receipt remains valid for S6 only; it does not extend to S8.

The S7 report's binary mismatch remains unresolved: five of seven FSS-linked binary hashes differed between S6 and S7 despite matching recorded Ubuntu 24.04.4, CMake 3.28.3, GCC 13.3, OpenSSL 3.0.13, Release configuration and CMake flags. Embedded absolute build-source paths differed and account for path strings, but not every byte difference. S8 binds its evidence to the exact S8 source snapshot and its own hashes; it does not claim S6 and S7 binaries are equivalent.

The desktop primary checkout and `VFSS-baseline/` were not edited. S5/S6/S7 historical source records, failed-run records and BB90 source findings remain unchanged.

## 2. Version-3 public sampling contract

The online binding contract is version 3. It supersedes S7's no-seed-disclosure rule only for this new runtime version; S7 remains accurate for the exact S6 snapshot it received, which did not send a seed. Offline package formats stay M6B5/v2 for monolithic `N≤128` and M6S2 for streamed `N≤256`. No sample seed is placed in either package, the slot manifest, ready/start metadata or phase-3 binding.

After both forward-shuffle exchanges (phases 20 and 21) complete, P0 obtains a fresh 32-byte seed from `getrandom()` only when a reduction iteration is active. P0 sends the seed in phase `40+i`; the existing authenticated framed channel binds session, fingerprint, padded N, K, comparison width, party direction, phase, frame type and one-shot material ID. P1 verifies the exact message and acknowledges. The payloads are:

```text
seed frame:  u32be(0x53355352) || u32be(3) || u8(iteration) || seed[32]  # 41 bytes
ack frame:   u32be(0x5335414b) || u32be(3) || u8(iteration)              # 9 bytes
```

The sample is reconstructed over the same public current-candidate vector. Its size remains `s=min(m,ceil(sqrt(m)))`. Partial Fisher–Yates selects the first `s` entries. Stream blocks are SHA-256 of the exact ASCII domain (no trailing NUL) `PROJECT_DERIVED_SELECT_5ITER/public-sample-prng/v3`, followed by seed, session as BE64, fingerprint as BE64, iteration as one byte, and counter as BE64. Each eight-byte word is read big-endian. For bound `b`, reject `x < ((-b mod 2^64) mod b)` and use `x mod b` otherwise. Counter exhaustion, digest failure, a malformed frame, or binding mismatch aborts; the runtime does not return a mask.

This specifies the same deterministic sample to both parties, but SHA-256 expansion is not a proof of exact finite-scale uniformity. `getrandom()` is the OS entropy source; its computational/statistical assumptions are not proved here. The ideal independent uniform-without-replacement assumption used in the earlier S2 expectation derivation does not automatically apply to this implementation. No expected-linear or high-probability comparison claim is made. For every valid seed, sample choice changes work only; the rank invariant and exact fifth candidate selection give the stable selected key. Invalid input/message, implementation error and resource failure must abort without mask.

### Updated leakage function `L_S8`

| Field | Source / receiver | Coordinate and timing | Derivable consequence / limit |
|---|---|---|---|
| Public 32-byte sample seed | P0 generates; both P0 and P1 know it | Published in authenticated phase `40+i`, after phases 20/21 and immediately before that active iteration's sample use | Both can derive the current sample order and sample-clique as soon as the message arrives. The current sample is not limited to positions disclosed later. |
| Current shuffled endpoint positions and pivot positions | Current runtime transcript; both parties | Shuffled coordinates, at the comparison phase where used | They can be replayed from the approved transcript. No original index or permutation mapping is sent. |
| Comparison bits and public comparison-call count | uCMP exchange; both parties | Current comparison layer | Determines pivot ranks, candidate/residual-rank updates and subsequent adaptive address schedule, together with the public graph and algorithm rules. |
| One-shot slot IDs, iteration/substage/purpose, direction, endpoint binding | Runtime/material ABI; both parties see the public address transcript | Slot/address labels at each call | Reveals which approved slot was consumed. It does not reveal the DCF key bytes and does not authorize key reuse. |
| Masked operands approved by the project | Protocol messages; both parties | Public masked values at the applicable protocol phase | These are included as actual values in `L_S8`; “masked” alone is not a proof that their joint distribution with local keys is simulatable. |
| Message lengths, phase order, online exchange count, success/ABORT status | Framed transport; both parties | At each exchange and termination | Includes uniform abort/no-mask. Time, OS scheduling and production network metadata are not claimed hidden by this report. |
| Raw score, pivot value, threshold, original-index↔shuffled-position map, DCF α/key, mask shares, complete output mask | Not sent | N/A | Not in `L_S8`; the sample-seed approval does not authorize these fields. |

After phase `40+i`, the sample order and clique are deterministically derivable. The pivot **values** remain hidden; a pivot's rank is public from the opened comparison bits and public shuffled positions, which is distinct from disclosing its underlying key. Current candidate set and residual rank are computed identically from public prior edges/bits. A future iteration's seed is neither generated nor preloaded until that iteration becomes active. Once all prior bits and candidate updates are public, the future candidate set may be known, but the future random sample remains unknown until its own seed publication. The seed is unrelated to DCF α.

## 3. Runtime changes and causal schedule

The Select5 controller retains four possible reduction iterations and an exact fifth selection iteration. It does not derive samples from a pre-shuffle binding ID. The public disclosure-state guard rejects sampling before forward-shuffle completion and rejects replay, skip or out-of-order active iterations. The P0 seed is generated on demand, then both parties derive the same sample. Version 3 changes the online binding/disclosure contract; M6B5/v2 and M6S2 offline materials remain unchanged.

The cause graph remains distinct across layers:

| Layer | Count / rule |
|---|---|
| Algorithm iterations | Five slots: up to four reductions, then exact selection; early completion leaves later sample seeds undisclosed. |
| Plaintext comparator dependencies | At most nine nonempty layers: two per active reduction plus the fifth exact candidate comparison layer. |
| CA compiler core | Conditional `2k+1`, equal to 19 for `k=9` only if CA preconditions hold; this is not the complete Protocol I path. |
| Protocol I paper core | Three rounds at the paper's stated primitive boundary; not a claim about this full runtime. |
| Full raw-score→original-order mask exchanges | `9 + 2L + A ≤ 31`, where `L≤9` comparator layers each use masked-operands/open-bit phases and `A≤4` active seed/ack exchanges. |

The fixed nine exchanges are binding, input conversion, two forward shuffle phases, threshold membership, two inverse-route phases and final status. Each active seed exchange is one P0 seed plus P1 acknowledgment. Each comparison dependency layer is two exchanges. The seed replaces the prior schedule payload within the iteration's existing causal slot; the algorithm still has five iterations and the full protocol does not become a five-round protocol. A crash after one party locally sees final success remains outside an atomic output-publication proof.

## 4. Fresh build and functional regression

Environment: Ubuntu 24.04.4 WSL2 x86_64, CMake 3.28.3, GCC 13.3.0, OpenSSL 3.0.13; Release, `BUILD_TESTING=ON`, EMP OT disabled, Select5 opt-in enabled. Build directory: `/tmp/m6b-project-derived-5iter-s8-release`.

```sh
cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-project-derived-5iter-s8/VFSS \
  -B /tmp/m6b-project-derived-5iter-s8-release \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_EMP_OT=OFF \
  -DMOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=ON
cmake --build /tmp/m6b-project-derived-5iter-s8-release --target \
  moe_topk_project_derived_select5_conformance_test \
  moe_topk_project_derived_select5_material_store_test \
  moe_topk_streamed_material_slot_store_test \
  moe_topk_tls13_transport_conformance_test \
  moe_topk_project_derived_select5_process_e2e_test \
  moe_topk_m2_ucmp_conformance_test \
  moe_topk_m2_parallel_shuffle_conformance_test -j2
```

All seven freshly linked targets exited 0:

| Target | Exit | Purpose |
|---|---:|---|
| `moe_topk_project_derived_select5_conformance_test` | 0 | Fixed sample vector; seed message parsing; cross-party sample agreement; invalid size/version/iteration; pre-shuffle, replay, skip and ordering guards; frame session/phase binding. |
| `moe_topk_project_derived_select5_material_store_test` | 0 | Monolithic package binding, durable one-shot claims and failure behavior. |
| `moe_topk_streamed_material_slot_store_test` | 0 | Streamed slot manifest, read and lease behavior. |
| `moe_topk_tls13_transport_conformance_test` | 0 | TLS 1.3 mTLS identity and frame binding. |
| `moe_topk_project_derived_select5_process_e2e_test` | 0 | 24 T/P0/P1 cases, independent frozen oracle and no-mask faults. |
| `moe_topk_m2_ucmp_conformance_test` | 0 | Actual uCMP/DCF functional conformance. |
| `moe_topk_m2_parallel_shuffle_conformance_test` | 0 | Parallel shuffle conformance. |

The experimental option is default OFF. A separate Release configuration with `MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=OFF` produced no Select5, streamed-store or TLS-conformance targets in `cmake --build --target help`.

The frozen output contract remains signed Q20.12 score descending with `original_index` ascending for equal scores; output mask bits are returned in original input order and must contain exactly K ones. The 24 E2E rows comprise 18 independent frozen-C++-oracle matches and six uniform ABORT/no-mask cases. Oracle successes check original order, exact mask length, bit values and weight `K`. N=128 successful K values were `1,2,8,64,65,128`, plus repeated K=64. N=256 successful K values were `1,2,8,64,128,255,256`, plus repeated K=128. Every successful row also checks post-success material re-claim rejection. Faults cover tampered package, disconnect, wrong party K, wrong TLS peer identity, T exit before ready, and the new `sample-seed-invalid-magic` case. The new case sends a malformed seed frame after forward shuffle over the actual authenticated runtime and checks both parties exit without a mask.

| Logical N | Successful rows | Comparison-call samples | min / median / max |
|---:|---:|---|---:|
| 128 | 7 | `298,299,303,368,747,2094,2479` | `298 / 747 / 2479` |
| 256 | 8 | `600,605,607,626,765,10967,12271,14723` | `600 / 695.5 / 14723` |

These are observed functional-test counts, not timing, expected complexity, high-probability bounds or formal performance results. The S2 expectation result assumes ideal independent uniform samples; S8's SHA-256-expanded stream has no such implementation-specific proof. Worst-case comparisons remain the previously proved `12N+C(N,2)` bound, hence `O(N²)`; S8 does not claim worst-case linear work.

The E2E preflight reports `N=256 monolithic=REJECTED_BEFORE_GENERATION` and `N=257 streamed=REJECTED`. For observed N=128 monolithic runs the serialized package was 60,008,572 bytes per party. For observed N=256 streamed runs, staged store was 185,925,312 bytes per party, 12,656,192 manifest bytes per party and 11 segments; total for both parties was 371,850,624 staged bytes. These are serialized/staged file-size observations only, not peak RSS, network package transfer or a performance metric. N>256 remains unsupported.

The monolithic durable-store target reports pass for lease binding, integrity, crash and concurrent claims. The streamed-store target reports pass for segment/slot binding, one-shot use, crash, race and integrity. E2E also reclaims a successful package and verifies rejection. This is local POSIX behavior; OS reboot/power-loss durability, administrator rollback/deletion, production filesystem and cross-host store semantics, supervisor availability and certificate operations were not tested by S8.

## 5. Actual party-view security review

### Model and view

The intended model is at most one semi-honest execution-party corruption, with trusted offline T. T may know the hidden permutation Π and both parties' preprocessed materials; it is input-independent, publishes packages/ready, then exits before online execution. P0/P1 use authenticated private TLS 1.3 channels. T+party collusion, active corruption, host compromise, administrator deletion/rollback, cross-host store behavior, certificate issuance/revocation operations and atomic output publication under crash are excluded. Functional E2E does not prove this model.

For each party `P_b`, its view includes its input share; the entire local package or streamed manifest/slot namespace including unused material; local DCF and shuffle material; local randomness and permutation shares; sent/received frames and their lengths/phases; public sample seeds, masked operands, endpoints, opened bits and slot addresses; access, abort, timeout and restart state; and its output mask share. T's offline view is separate and excluded from either execution party's view under the no-collusion assumption.

The runtime's uCMP call uses the actual `keyGenDCF(b,64,alpha,1)` and `evalDCF` ABI in `protocol_i_ucmp.cpp`. For ring width `b`, the two opened masked values form `x=(z_left-z_right) mod 2^b` and `y=(x-2^(b-1)) mod 2^b`; each party evaluates its local DCF key at both x and y, then applies the existing party-specific correction to obtain the strict-less output share. The runtime opens approved masked operands and the comparison bit, consumes a bound one-shot slot and uses revealed bits for later candidate/address selection. This review does not replace that ABI or convert the functional DCF test into a privacy theorem.

M6B5/v2 and M6S2 remain different security objects. M6B5/v2 contains the monolithic used Select pool plus unused M2CP DCF pools whose thresholds are correlated through the common `r` vector. M6S2 is the streamed metadata-only shuffle profile, with its own per-slot data, manifest, digest, access trace and durable lease; it does not contain that M2CP comparator pool. Each schema needs its own whole-view argument, including unused slots, failed calls and adaptive access. The new public seed is additional approved leakage and does not make either pool independent.

For the source-derived inventories carried forward from S7, M6B5/v2 at padded N=128 has `5C+2C+3N=57,280` logical DCF-key pairs per party: five Select pools, two unused all-pairs shuffle pools, `2N` conversion keys and N membership keys. Its observed serialized package is 60,008,572 bytes per party. M6S2 at N=256 has `5C+3N=163,968` logical DCF-key pairs plus `10N=2,560` shuffle metadata slots, for 166,528 one-use addresses; the 11-segment staged store is 185,925,312 bytes per party including the 12,656,192-byte manifest. These are package/layout counts and local staged-size observations, not RAM peak or network delivery measurements. Security review is separately bound to each format.

### P0/P1 simulator attempt and exact gap

For P0, an ideal simulator could receive P0's input share, its ideal output share and `L_S8`. It can sample the public seed at each active point, derive the same current sample and emit the approved lengths/phases. It must also synthesize P0's actual local package, all used and unused DCF keys and masks, opened masked operands/bits, adaptive addresses, shuffle/inverse shares and abort/output state jointly. For P1, the simulator uses the same approved seed field from `L_S8`, derives the same sample, receives the P0 messages and must synthesize P1's local correlated pool, local masks, shuffle state, output share and all observable failure prefixes. Neither simulator is given the real transcript as an input; the intended inputs are only the party's ideal input/output and `L_S8`.

The candidate hybrid sequence would (1) replace TLS with authenticated ideal channels, (2) replace `getrandom()` with ideal random words, (3) replace FSS/DCF AES PRG/PRF calls with ideal responses, (4) simulate the full party-local DCF pool and mask shares conditioned on its input share, `L_S8`, adaptive addresses and ideal output, (5) simulate the forward/inverse shuffle and membership composition, then (6) simulate the output share and success/abort/restart prefix. Symbolic losses would include `ε_TLS`, `ε_OS`, an AES distinguishing term parameterized by actual query count `Q`, and a pool/composition term `ε_pool`; none is established here and no numeric bound is claimed.

The first unclosed step is (4): there is no applicable lemma showing that the *actual serialized local DCF key shares and mask shares*, conditioned jointly on public masked operands and comparison bits, are simulatable from only that party's input/output and `L_S8` across all used and unused keys under adaptive slot selection. For M6B5/v2 the proof must additionally reproduce the unused common-r-derived threshold cycles. For M6S2 it must reproduce the entire manifest, per-slot values, digests and lease/access behavior. A single-key `keyGenDCF/evalDCF` correctness/conformance result cannot establish this full-pool distribution. Since step (4) is missing, later shuffle/inverse/output hybrids have no proven starting distribution. No concrete distinguisher was found; **the proof is not closed**. `DCF_SINGLE_KEY_PRIVACY`, `ADAPTIVE_FULL_POOL_VIEW`, `SHUFFLE_OUTPUT_COMPOSITION` and `JOINT_PARTY_VIEW` remain OPEN. No conditional-security claim or `secure` alias is warranted.

## 6. Attempts, provenance and limits

Failed attempts are recorded rather than counted as passes:

1. An unqualified WSL invocation selected Ubuntu 20.04/CMake 3.16.3 and CMake configuration failed the repository's CMake ≥3.17 requirement. No build or test ran in that attempt. The run was repeated with explicit Ubuntu 24.04.4/CMake 3.28.3.
2. The first malformed-seed test-hook draft set the plaintext-transport flag while using the TLS-bound runtime; config validation rejected that mixed configuration before the seed phase. The hook was narrowed to explicit TEST_ONLY direct-material injection over an already authenticated TLS endpoint with the expected peer identity. Production party entry still requires durable material storage and TLS.
3. One E2E run used a binary built before that test-hook correction and did not yield a complete case record. It was stopped and excluded. The final binary was rebuilt from the corrected source and the full 24-case run passed. The same temporary evidence directory was reused for the final run, so the first attempt's partial raw stdout was not retained; its incomplete status is recorded here and it is not cited as a pass.

External evidence directory: `C:\Users\28641\.codex\evidence\S8_SELECT5_V3_2026-10-15-final`. It contains target stdout, target exit records, the E2E raw stdout and external hashes. The repository carries the structured 24-case JSON and source manifest, not raw keys, packages or build outputs.

The C++ binaries and case JSON are pinned in the source manifest. `git diff --check` is run on the final worktree; inherited S1–S7 files remain present because the selected source snapshot is intentionally uncommitted. The path-level S8 delta is separately listed in the manifest. No formal LAN/WAN matrix or benchmark was run. V3 remains OPEN; V4 and formal LAN/WAN remain `NOT_ALLOWED/NOT_RUN` under the existing project plan.

## 7. Gate decisions

| Gate | S8 result | Scope |
|---|---|---|
| `S7_SOURCE_RECEIPT` | `PASS_FOR_EXACT_S6_SNAPSHOT` | Preserved prior independent S7 result only. |
| `S8_INDEPENDENT_ACCEPTANCE` | `NOT_YET_RECEIVED` | This report's author implemented and ran the S8 changes. |
| `SAMPLE_RANDOMNESS_PUBLIC_AFTER_SHUFFLE` | `PASS_BOUNDED` | v3 seed generated after forward shuffle, active iteration only; current sample is immediately derivable. |
| `SAMPLE_RECONSTRUCTION_CONSISTENCY` | `PASS_FUNCTIONAL` | Fixed vector, both-party derivation, plus oracle E2E. SHA stream distribution is not proved exact-uniform. |
| `FUNCTIONAL_E2E_N256` | `PASS_BOUNDED` | 24 cases, N=128 and N=256 K matrices; N=257 preflight rejects. |
| `DCF_SINGLE_KEY_PRIVACY` | `OPEN` | ABI/function conformance only; no local-key share simulator. |
| `ADAPTIVE_FULL_POOL_VIEW` | `OPEN` | Both package schemas and correlated unused pool not simulated. |
| `SHUFFLE_OUTPUT_COMPOSITION` | `OPEN` | No joint forward/inverse/membership/output/abort proof. |
| `JOINT_PARTY_VIEW` | `OPEN / NO_GO_FOR_SECURE_CLAIM` | No complete P0 or P1 simulator/hybrid. |
| `SECURE_ALIAS_READY` | `NO` | Route remains opt-in `EXPERIMENTAL`. |
| `N_GT_256_ADMISSION` | `NO_GO` | N=257 preflight rejects; no extrapolation. |
| `V3_V4_PERFORMANCE_STATUS` | `V3=OPEN; V4=NOT_ALLOWED/NOT_RUN` | Formal LAN/WAN remains prohibited and not run. |

BB90's five-iteration source gate remains `NO_GO_FOR_IMPLEMENTATION`; BMW16-derived remains separate. This route may be described only as a bounded, project-derived stable Top-K **functional** candidate for the tested source/schema range. It is not a secure protocol claim and does not replace the BB90 comparison item.

## 8. S8-authored file delta

The exact S8 delta relative to the S7 receipt, with SHA-256, is listed in [the S8 source manifest](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S8_SOURCE_MANIFEST_2026-10-15.json). S8-authored paths are:

- `VFSS/include/moe_topk/project_derived_select5.h`
- `VFSS/include/moe_topk/protocol_i_project_derived_select5.h`
- `VFSS/src/moe_topk/project_derived_select5.cpp`
- `VFSS/src/moe_topk/protocol_i_project_derived_select5.cpp`
- `VFSS/tests/moe_topk/project_derived_select5_control_test.cpp`
- `VFSS/tests/moe_topk/protocol_i_project_derived_select5_three_process_e2e_test.cpp`
- `PROJECT.md`
- `docs/IMPLEMENTATION_PLAN.md`
- `docs/REFERENCE_MANIFEST.md`
- `docs/decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S8_DECISION_2026-10-15.md`
- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S8_E2E_CASES_2026-10-15.json`
- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S8_REPORT_2026-10-15.md`
- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S8_SOURCE_MANIFEST_2026-10-15.json`

The manifest separately records the 55 selected inherited files. No `VFSS-baseline/`, paper, key, certificate, build output or raw large log is included.
