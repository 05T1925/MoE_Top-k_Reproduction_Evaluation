# M6B-I PROJECT_DERIVED_SELECT_5ITER S7: independent S6 receipt and full-view security review

Date: 2026-10-14
Route: `PROJECT_DERIVED_SELECT_5ITER`
Composition: `Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / EXPERIMENTAL`
Decision record: [S7 decision](../decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S7_DECISION_2026-10-14.md)
Machine-readable receipt/gates: [S7 source and gate manifest](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S7_SOURCE_MANIFEST_2026-10-14.json)

## 1. Executive result

S7 receives the **exact, uncommitted S6 file snapshot** on base `c3926c68fd14f270faa8b55234311071947fa080`, not a branch name or the S6 report by itself. The receiving reviewer participated in S1/S2 research and previously received the S5 baseline, but did not participate in S4/S5/S6 implementation or decisions. The independent receipt scope, fresh build identity, each test exit code and N=256 case results are in the machine-readable manifest; this report does not substitute S6's original run for the S7 rerun.

The S6 project-derived stable Top-K candidate is functionally accepted at `1≤n≤256`, `1≤K≤n`, for the exact stable signed Q20.12 input contract and the two package profiles inspected. It remains **EXPERIMENTAL**. We found no reproducible deterministic answer, slot-reuse, seed-forwarding or output-mask defect in the reviewed S6 snapshot. The full P0/P1 joint-view proof does not close: first missing is a simulator/reduction for the actual DCF party key share and its correlation with mask shares, opened masked operands and comparison bits, compositionally across the adaptive full pool. `JOINT_VIEW_SECURITY=OPEN`, not conditional PASS.

S6 runtime, package, binding and current run artifacts do not expose a reusable `public_sample_seed`. The older S5 E2E JSON does contain seed fields from S5's pre-fix runs. Those historic records remain unchanged and are not S6 outputs. No formal performance matrix was run or admitted. V3 remains OPEN; V4 and formal LAN/WAN remain `NOT_ALLOWED/NOT_RUN`.

## 2. Source object, worktree isolation and provenance

| Object | Observed identity |
|---|---|
| S7 review/decision worktree | `C:\Users\28641\.codex\worktrees\m6b-project-derived-5iter-s7`, branch `codex/m6b-project-derived-5iter-s7` |
| Git base / `HEAD` at start | `c3926c68fd14f270faa8b55234311071947fa080` |
| Local `main` and cached `origin/main` refs | both `c3926c68fd14f270faa8b55234311071947fa080` at inspection; no fetch performed |
| S6 worktree | `C:\Users\28641\.codex\worktrees\m6b-project-derived-5iter-s6`, branch `codex/m6b-project-derived-5iter-s6`, same base; dirty/uncommitted |
| S6 manifest | `M6B_I_PROJECT_DERIVED_SELECT_5ITER_S6_SOURCE_MANIFEST_2026-10-14.json`, SHA-256 `fbf9c6adb7aa4caf543c86a4db317c5abaf3e09f0aa8a523cbb1f42e25902812` |
| S6 input freeze | `M6B_I_PROJECT_DERIVED_SELECT_5ITER_S6_INPUT_FREEZE_2026-10-14.json`, SHA-256 `4c6a8c3dee3085e23fe1ebb432f52d3764731813c594cca4a6e8e1e174447724` |
| S6 final E2E cases | `M6B_I_PROJECT_DERIVED_SELECT_5ITER_S6_E2E_CASES_2026-10-14.json`, SHA-256 `90d6850bda111ccef3ab16bb6ec73ac8b70d4c97d0bf77c4ff3ae55e71fac879` |

S6 did not create a task commit. Its manifest enumerates 9 runtime/test source deltas and 3 governance deltas over S5. S7 rebuilt a selective snapshot from the S6 input-freeze entries and overlaid only those declared S6 deltas and S6 decision/report/machine evidence. It did not copy the entire dirty S5 or S6 worktree. An independent hash check found 48 copied/frozen rows and **zero mismatches** against the expected inherited or S6-overlaid hashes. The exact path/hash list, S6 delta list, and S5/S6 artifact hashes are in the manifest.

The direct S6 `git status --short` inventory was also inspected rather than inferred from its change manifest. It showed modified tracked paths `PROJECT.md`, `VFSS/CMakeLists.txt`, `VFSS/include/moe_topk/protocol_i_parallel_shuffle.h`, `VFSS/include/moe_topk/protocol_i_score_input.h`, `VFSS/include/moe_topk/protocol_i_transport.h`, `VFSS/src/moe_topk/protocol_i_parallel_shuffle.cpp`, `VFSS/src/moe_topk/protocol_i_score_input.cpp`, `VFSS/src/moe_topk/protocol_i_transport.cpp`, `docs/IMPLEMENTATION_PLAN.md`, and `docs/REFERENCE_MANIFEST.md`; untracked files included six Select5/material-store headers/sources, five test sources, S1–S6 decisions and reproduction records, plus `experiments/`. S7 compared the full inventory with the S6 manifest and S5 input freeze: S6's own source delta is the 9 listed runtime/test paths, the 3 governance files are its declared doc deltas, and the remaining files are inherited S1–S5 evidence/source. The S4 `docs/PAPERS.sha256` user difference was not in the frozen copy and was left untouched.

The desktop primary checkout `C:\Users\28641\Desktop\MoE_Top-k_Reproduction_Evaluation` remained on `feat/m6a-performance-evaluation` at the same base commit. Its pre-existing modified/untracked paths were observed read-only and not changed by S7: `.agents/skills/protocol-reproduction/SKILL.md`, `PROJECT.md`, `docs/IMPLEMENTATION_PLAN.md`, `docs/PAPERS.sha256`, two M6A decision documents, one M6A reproduction report and `siamjdiscrmath.pdf`. `docs/PAPERS.sha256` was not copied as an S6 input and was not edited. `VFSS-baseline/`, paper/reference trees, credentials, generated materials, build artifacts and raw logs were not modified or added to the repository.

S6's own source delta is the 9 file paths listed in the S6 manifest; S7 did not modify those code files. It updated only governance/reporting files in the isolated S7 worktree (listed in §11). S4/S5/S6 remain uncommitted snapshots; none is described as merged into main.

## 3. Independent functional receipt

The receiver's independence is specific: the receiver had prior S1/S2 research involvement and performed an S5 baseline receipt, but did not implement or decide S4/S5/S6. The receiver reconstructed the S6 snapshot from the frozen file list, checked hashes, configured a fresh Release build, checked the feature-OFF target registry, and reran the seven S6 targets and the fixed 23-case S6 E2E matrix. The exact environment, configure/build/test commands, process exits, executable SHA-256s, log SHA-256s and case records appear in the S7 manifest. A later auxiliary rerun mistakenly used default WSL Ubuntu 20.04, where CMake 3.16.3 failed the project's 3.17 minimum during configure and overwrote `environment.txt` and `configure.log` in that evidence directory. Those overwritten files are not evidence for the successful Ubuntu 24.04 receipt. A repeated target run was interrupted during E2E; its partial capture is not substituted for the completed run. Both attempts are listed in the manifest as non-passing evidence events.

### 3.1 Seven directed test targets

| Target | Purpose | S7 exit |
|---|---|---:|
| `moe_topk_project_derived_select5_conformance_test` | public controller/rank and trace conformance | 0 |
| `moe_topk_project_derived_select5_material_store_test` | monolithic package binding, integrity and durable lease | 0 |
| `moe_topk_streamed_material_slot_store_test` | M6S2 manifest/segment, per-slot lease, replay, race and tamper | 0 |
| `moe_topk_tls13_transport_conformance_test` | TLS 1.3 mutual identity and v2 frame binding | 0 |
| `moe_topk_project_derived_select5_process_e2e_test` | T/P0/P1, stable oracle, masks and failure path | 0 |
| `moe_topk_m2_ucmp_conformance_test` | actual uCMP/DCF functional ABI | 0 |
| `moe_topk_m2_parallel_shuffle_conformance_test` | forward/inverse shuffle functional checks | 0 |

The default-OFF configure excluded Select5, streamed store and TLS conformance targets; no default alias or `secure` alias was registered. The complete independent command transcript and per-executable binary identity, rather than this summary table, is the receipt evidence.

### 3.2 E2E matrix and oracle contract

The frozen S6 case list has 23 entries: 18 success cases and 5 uniform-ABORT/no-mask fault cases. The independent rerun rechecked output length, original order, bit values, exact weight K and bitwise equality against the frozen C++ `top_k_mask()` oracle in the TEST_ONLY controller. It did not accept party exit status as an oracle.

Successful logical/padded configurations include `n=1`, `n=7`, `n=128`, and `n=256`. N=256 success ranks are `K=1,2,8,64,128,255,256` plus an independent repeated `K=128`; fixtures include duplicate scores and signed Q20.12 edge values. Five injected faults cover package tampering, peer disconnect, wrong party K, wrong TLS peer identity, and T failing before ready. Each case returns no complete mask to the controller. N=256 monolithic preflight and `n=257` streamed preflight reject before a run is treated as supported.

The fixed N=256 successful run's historical diagnostic comparisons were `618, 660, 723, 20,650, 13,953, 662, 629, 6,503` (min/median/max `618/692.5/20,650`). The independent rerun generates new OS-random samples, so its comparison counts are separately recorded and need not equal the S6 list. These are comparison-count diagnostics, not runtime, LAN/WAN, expectation or tail-bound evidence.

### 3.3 Material lease failure matrix: exact tested scope

The fresh conformance targets exercise durable POSIX behavior at the package or slot level:

- wrong session/K binding and pre-ready rejection;
- tamper detection against committed package/segment digest;
- concurrent cross-process claim with exactly one winner;
- package or slot replay after claim;
- crash before package claim leaves it claimable;
- crash after durable package/slot claim burns it;
- N=257 preflight rejection and S6 E2E no-mask fault behavior.

The target sources `protocol_i_project_derived_select5_material_store_test.cpp` and `protocol_i_material_slot_store_test.cpp` identify the named cases. The receiver re-executed package/slot binding, tamper, replay, cross-process concurrent one-winner claim, child exit before claim (later claim succeeds), and child exit after durable claim (later claim is denied); E2E re-executed N=257 rejection and the five listed no-mask faults. **NOT_RERUN:** actual OS reboot or power loss, and the narrow crash point after lease durability but before key bytes are returned. S7 machine records distinguish tested cases from source-only coverage. Evidence supports only the local POSIX store and tested process model. It excludes administrator deletion/rollback, backup restoration, network filesystem durability, production host/UID separation, supervisor identity hardening and production certificate issuance/rotation/revocation.

### 3.4 Evidence-preserving same-task reconfirmation

The independent receiver's complete run returned the stated 7/7 target and 23-case E2E success, but a later erroneous default-distro auxiliary configure and an interrupted duplicate run overwrote its `environment.txt`/`configure.log` and left only a partial `stage_exit_codes.txt` plus incomplete raw E2E stdout in that folder. To avoid treating those altered files as a complete receipt, this task also performed a **separate same-task (not independent-receiver) reconfirmation** from the S7 worktree after confirming its runtime/test hashes matched the S6 manifest. Ubuntu 24.04.4 WSL2, CMake 3.28.3, GCC 13.3.0 and OpenSSL 3.0.13; fresh Release/TESTING ON/EMP OT OFF/Select5 ON configure, build and direct execution of the same seven targets all exited 0. A separate default-OFF build again omitted the opt-in targets. Its E2E stdout has 23 case rows; a checker matched `n`, padded N, K, result and fault against the frozen S6 JSON, verified the specified N=256 K set, and passed the executable's frozen-oracle/no-mask assertions. This second run supplies the retained per-case machine rows and binary/log hashes; it corroborates the independent receipt but is not mislabeled as the nonparticipant's run. The Ubuntu 20.04 auxiliary configure and two initial shell-wrapper invocation errors are retained as failed evidence events, not code/test failures.

## 4. Algorithm/state and deterministic correctness review

The inspected entry points are `VFSS/src/moe_topk/project_derived_select5.cpp::protocol_i_project_derived_select5_control_with_sampler` and `VFSS/src/moe_topk/protocol_i_project_derived_select5.cpp::run_party_with_material`.

The adapter contract is the repository's signed 32-bit Q20.12 raw-score word; the selected order is descending score followed by ascending `original_index`, making each logical stable key unique. The existing carry/sign adapter produces that priority-key sharing before the forward shuffle; padded records rank after logical records and are excluded from the returned logical-length mask. Select finds the K-th stable key; membership uses the conformance-tested `1 - strict_lt(tau,key_i)` direction; inverse routing restores the original slot order. E2E reconstructs only inside its TEST_ONLY controller and verifies the XOR of party shares has length `n`, values in `{0,1}`, Hamming weight exactly `K`, and bitwise equality to the frozen stable `top_k_mask()` oracle. This is functional evidence, not a party-view privacy statement.

For each active reduction iteration, the controller freezes the sample clique before comparing it. The returned sample must have the prescribed size, contain distinct vertices and be a subset of the current candidate C. It sorts that sample from the opened comparator bits, takes two distinct ordered pivots `x<y` at the parameterized sample ranks, freezes both stars from each pivot to `C\sample`, and explicitly rejects overlap between clique and star edge sets. Since x and y are distinct and star endpoints exclude all sample vertices, the two stars also have no duplicate undirected edge. A `set<EdgeKey>` canonicalizes within-wave unordered relations. The rank update uses the exact opened comparisons to recover each pivot's rank in C and then either selects the target pivot or keeps the unique interval/lower/upper subcandidate while updating residual rank. If no earlier iteration selected the answer, the final active iteration freezes the full clique on C and selects exactly the residual-rank vertex.

Thus, for valid stable-key comparisons and **every legal sample tape**, the correctness invariant is deterministic: the target remains in C and its one-based C-rank remains `residual`; the sample tape affects work and trace length only. Invalid sample shape, invalid rank interval, bad material, comparator/transport error or resource failure throws and follows no-mask failure handling; an incorrect threshold is not an accepted probabilistic failure. The controller pads inactive trace slots after a target is selected so an algorithm trace still has five iteration entries; those padding entries do not cause new comparison or sample exchanges.

The runtime maps slots by iteration, purpose, disjoint-sublayer scope, direction and canonical shuffled endpoints. The monolithic in-memory `consumed` bit-vector rejects duplicate address consumption; the streamed reader durably claims one addressed slot before returning bytes. The same edge in two different iterations is a different purpose/iteration slot and not reuse of one key. The runtime comparison layer index drives separate operand and result-share phases; no extra rank, pivot value, stable key or original index is sent.

This review establishes consistency between code paths and the declared project-derived control contract, not a proof of DCF privacy or ideal shuffle security. No code behavior is used as a BB90 or BMW16 theorem.

## 5. `public_sample_seed` and the S6 transcript

The S6 runtime overload has no `sample_seed` argument. `random_bytes` repeatedly calls Linux `getrandom()`, handles `EINTR`, and errors on a failed/empty read; `random_bounded` uses rejection sampling; `random_sample` uses partial Fisher–Yates without replacement. The runtime calls it only on P0, only after entering an active iteration, and sends the encoded current sample positions on the `sample_schedule[iteration]` endpoint with phase `40+iteration`. P1 checks length, uniqueness and membership in its current candidate before progressing. There is no persistent RNG state or reusable future schedule in the wire payload. Session/fingerprint/material IDs are not used as a seed.

The sample set is the current sample-clique vertex set and thus reveals the current round's shuffled endpoints. The draw order is also present in the current sample payload; the C++ graph canonicalizes clique edges and the pivot choice is determined by opened comparisons, so this draw order does not select a secret pivot value. The independent source scan found no raw runtime sample seed in S6 source, package serialization, binding, transport frame, S6 final case JSON or receiver logs. It is useful to separate **input-share test fixture seeds** (S6 JSON has `input_share_seed_hex`) from a Select sampling seed; they are TEST_ONLY harness values and do not drive runtime sample selection.

The S5 archival E2E JSON is different: all 15 historical case rows include numeric `sample_seed` values from the pre-fix runs (file SHA-256 `56142221b7acfe35f784109fea763864489b71237659e299cef42708ede5207d`). S5 had sent a seed to parties before the approved timing; the archived JSON preserves that historical issue. S7 did not erase or rewrite those records. Consequently the precise finding is “no raw sample seed in the reviewed S6 runtime and S6 current run artifacts,” not “no sample seed was ever logged in the project's history.”

For the actual runtime sampler, unbiasedness is conditional on uniform independent 64-bit words from the operating-system CSPRNG. Given that model, rejection sampling has an acceptance interval whose size is divisible by the bound, and partial Fisher–Yates gives an exact uniform sample without replacement. The project has not proved an implementation-specific statistical guarantee for Linux `getrandom()` nor applied S2's ideal sampler expectation theorem as a runtime theorem. No numeric random abort threshold is imposed by project decision; this does not relax exact correctness for legal tapes or abort/no-mask requirements.

## 6. Leakage function `L_S6` and public control derivation

The table records content visible to an online party; TLS protects the channel against outside observers, not against its authenticated peer. Current shuffled positions/endpoints are in the confirmed disclosure scope. Original input position ↔ shuffled position, raw score, stable threshold and compared plaintext values remain hidden.

| Field | Producer → recipient | Coordinate/domain/protection | Time and deterministic consequences |
|---|---|---|---|
| Public binding: session, fingerprint, material ID, logical/padded N, K, bit width, party/direction, protocol/schema, phase/sequence | T/supervisor and each party → P0/P1 | public identifiers and parameters; no score or sample RNG state | Package/frame setup; fixes ABI and slot namespace but does not determine sample tape. |
| Current sample positions | P0 → P1 | shuffled coordinates, 32-bit indices; sent inside TLS application payload | Only immediately before an active iteration's graph is needed. Reveals current sample-clique vertices/order, not a raw index mapping or future sample. |
| Frozen comparison endpoints | generated by public controller and sent/used by both parties | shuffled coordinates; pair is canonical unordered relation | Current wave before masked operands. Reveals edge set and comparison-call count. |
| Pivot positions | derived from current sample order and opened bits | shuffled coordinates; no pivot value | After the sample comparison results needed to identify pivots; pivot edges and control state follow. |
| Slot identifiers/address | party slot reader, reconstructible by both from graph | purpose, iteration, substage scope, direction and shuffled endpoint pair; slot contents private | At each DCF claim/evaluation. Prior bits/edges determine the next candidate/address trace. One-shot claim; no key reuse. |
| Masked operands `z_left,z_right` and score/membership masked operands | each party sends local additive-share word; both add received words | ring `Z/2^b`; local shares are sent, their sum is visible to both protocol participants | At each uCMP exchange. Reveals the mask-added operands permitted by the user, not directly score/key/threshold. Algebraically correlated with the DCF threshold. |
| Comparator result bit and DCF output shares | each party evaluates its local DCF share, then both exchange shares | local result words then opened 0/1 | After each comparison wave. The bits determine sample order, exact pivot ranks, active C, residual rank, future edges and key addresses. |
| Counts, phase/sequence, payload/frame lengths, active-layer/sample exchanges | both parties and transport observers | public integers/lengths; outside observer sees TLS record sizes/timing | During each exchange; endpoints plus bits/counts and lengths are already allowed. Exact wall-clock timing remains unmeasured and is not silently added as a proven leakage field. |
| Forward shuffle and inverse route local state | generated by trusted T then local to P0/P1 | local `sigma`, `tau`, `a/e/r_share`; peer messages include first/second-round masked records | T knows Pi; no Pi field sent to online parties. Party sees its local share and public masked sum. Cross-stage permutation/mask correlations need proof. |
| Output mask share | each P_i → its own caller | local XOR share in original order, length n | Returned only after final success exchange. TEST_ONLY controller alone reconstructs for frozen oracle comparison. A single share is not the complete mask. |
| ABORT, disconnect, timeout, restart | party protocol status to peer; process supervisor observes local exit | common status byte for explicit peer abort; failure stage and duration can be locally visible | No full mask is returned on covered E2E failure. Crash/timeout local timing and a post-status crash are not demonstrated to have atomic two-party output publication. |
| T's offline view | T only; T exits before ready | T knows Pi and both party packages/material/masks during generation | Trusted, input-independent and non-colluding. T+P0/P1 collusion is explicitly outside model. |

Given initial public N/K and the allowed current-round sample, public endpoints and bits suffice for both parties to update `C` and `residual` deterministically. The implementation does not send secret rank muxes or pivot values. Candidate size and later access sequence are deterministic functions of the previously opened bits, sample positions and algorithm parameters; that is part of the actual leakage, not hidden by a claim that only endpoint labels are public. The schedule contains no future graph field derivable before its sample-generation event.

## 7. Material and DCF ABI: two distinct party views

### 7.1 Actual comparison ABI

`ProtocolIUcmpMaterial(b,m_left,m_right)` sets `alpha=(m_left-m_right) mod 2^b` and invokes `keyGenDCF(b,64,alpha,1)`. For each active comparison, each party opens the sums `z_left = x_left + m_left` and `z_right = x_right + m_right` (mod `2^b`), forms `x=z_left-z_right` and `y=x-2^(b-1)`, evaluates its one party-local DCF key on x and y, exchanges local result shares, and opens the strict comparison bit. The DCF traversal reads input bits MSB-first. Each logical compare consumes one key slot; each party performs two DCF evaluations and both parties perform four evaluations together. Membership uses the same ABI and the tested direction `1 - strict_lt(tau,key_i)` with stable keys, followed by inverse routing.

The low-level DCF C++ code uses AES calls over correction-tree blocks and stores party material as `k`, `g`, `v` vectors. S6 seeds a pool of FSS PRNGs from OS randomness. Source inspection identifies these mechanisms but supplies no reduction from the precise `DCFKeyPack` serialization and its joint inputs to a party simulator. `DCF_SINGLE_KEY=PASS_FUNCTIONAL_ABI_ONLY`; do not read it as key privacy proof.

### 7.2 M6B5/v2 monolithic, logical/padded N≤128

The monolithic dealer calls the general `protocol_i_parallel_shuffle_dealer_generate_for_permutation` for both forward and inverse shuffles. That profile creates `C(N,2)` edge DCF material for each shuffle even though Select5's two-round path uses only sigma/tau/a/e/r metadata and does not call the shuffle CmpAgg round. Those are **unused but serialized party keys**. Select5 has five iteration-bound complete-pair pools. Score conversion contributes `2N` keys and membership N keys. Total logical DCF-key pairs/party: `5C + 2C + 3N = 7C+3N`; at N=128 this is 57,280. M6B5/v2 serialized party package observed in S6 was 60,008,572 bytes (the older S4/S5 v1 measurements were 60,008,580 bytes and remain historical v1 numbers).

Each unused shuffle edge threshold is a difference of entries in a common full vector `r`: `alpha_ij=r_i-r_j mod 2^b`. Thus its pool is not a set of unrelated uniform thresholds; it contains cycle identities such as `alpha_ij+alpha_jk=alpha_ik`. The party also has its local `r_share` and complete set of its local DCF key shares. Although runtime never evaluates those unused edges, the package view still includes them and a simulator must explain them. The package is claimed once under a durable per-party lease; in-memory Select slot accounting additionally rejects duplicate addresses.

### 7.3 M6S2 streamed, N=256

The streamed dealer calls the metadata-only shuffle generator, so it does not make M2CP's unused C(N,2) shuffle comparator pools. Logical DCF-key pairs/party are `5C+3N=163,968`; O(N) shuffle metadata contributes `10N=2,560` more slots for total 166,528 one-use addresses. At N=256, the source-derived data is 173,269,120 bytes/party, manifest is `64+76·166,528 = 12,656,192` bytes/party, 11 segments are used, and S6 staging observed 185,925,312 bytes/party (data+manifest). That observation is local staged file size, not network delivery bytes or peak RSS.

Each address binds session, fingerprint, material ID, N/K/bit width/party and purpose/iteration/substage/direction/endpoints. The reader checks manifest ordering/ranges, segment owner/type/mode, slot SHA-256, creates an exclusive fsync-backed lease before returning bytes, then rechecks the slot digest. Restart, duplicate and concurrent claims fail closed in the local POSIX model. S6 did not test a production network filesystem or cross-host store.

### 7.4 The proof obligation each layout creates

For P_b, the party view contains its complete package/manifest and local material, not merely the keys later queried. For monolithic, the simulator must additionally produce all unused M2CP keys correlated through the common r vector. For streamed, it must produce every manifest slot, digest, lease/access trace and unconsumed address while preserving independently generated per-slot mask/DCF correlations and adaptive selection. In both profiles the address trace is selected from prior opened bits. `ADAPTIVE_FULL_POOL=OPEN`; functional fresh-slot coverage is not a multi-key privacy theorem.

## 8. Full P0/P1 simulator/hybrid review

### 8.1 Model and view

The candidate model is semi-honest corruption of at most one of P0/P1. T is trusted, input-independent, runs offline, may know the hidden permutation Pi and both offline material sets, publishes them, and exits before online readiness. The local supervisor publishes ready only after successful offline generation; the E2E fault case covers T exiting before ready. Supervisor isolation, cross-host ready agreement, T+party collusion, active deviations, host compromise, storage-administrator rollback/deletion and production certificate lifecycle are excluded. P0/P1 use TLS 1.3 mutual authentication. The full view is not only network payload: `View_b` includes P_b input shares; its whole schema-specific package/manifest and keys/masks; local shuffle/inverse state; local randomness; all sent/received application frames; opened z operands/bits; adaptive endpoint/address state; status/abort/timeout/restart observations; and final local output share.

### 8.2 Candidate hybrids and explicit terms

For either b=0 or b=1, start with the real view `H0_b`. A proof would need the following ordered transformations:

1. Replace the authenticated private transport by ideal authenticated channels while leaking only allowed message lengths and approved peer identity. This needs a TLS 1.3 channel-security statement for the actual certificate/endpoint model and a term `ε_TLS`; certificate issuance, revocation, transcript binding and external timing are not fully measured in S6/S7.
2. Replace all sample/mask/permutation entropy calls with independent uniform words. This needs an explicit OS-CSPRNG assumption or statistical statement `ε_OS`; the code's `getrandom()` call is not that proof. Under the ideal-word hybrid, rejection sampling and partial Fisher–Yates produce a uniform permutation/sample exactly.
3. Replace FSS AES counter-stream outputs and DCF correction-tree AES calls with ideal independent PRP/PRF responses, bounded by the actual key/query schedule. A term would have form `ε_AES(Q)` under the selected AES PRP/PRF assumption; an audited global query count Q, domain separation proof, and relation between the two AES uses are not included in S6.
4. Replace each party's actual serialized DCF key share plus local mask shares by a simulator conditioned on its prescribed input share, current opened z values and bits, and the complete adaptive address transcript. This is the **first unclosed hybrid**. No theorem is supplied for this exact `keyGenDCF(b,64,alpha,1)` ABI with `alpha=m_left-m_right`, the two x/y evaluations, arithmetic result shares, complete local key pool, rejected/unused keys and opened adaptive queries. The monolithic profile additionally has the correlated unused-alpha cycle identities. A generic assertion of “DCF security” does not state whether its adversarial view includes the entire key pool, auxiliary mask shares, dependent thresholds and adaptive queries.
5. After a DCF pool simulator, simulate the correlated forward and inverse shuffle local permutations, masks, public masked records, current endpoints and selected-output carrier. This needs a joint composition argument for `sigma/tau/a/e/r_share` under one hidden Pi and its inverse, including the fact that trusted T knows Pi but a party does not.
6. Simulate the output shares and all successful/failing execution prefixes. A simulator needs the real output-share distribution (or party output from the ideal functionality) and a failure/abort functionality that includes connection loss, timeout and restart. A final success exchange provides a barrier against tested earlier faults, but does not prove atomic publication under a crash immediately after one side sends/receives success.

No numerical advantage bound is claimed: `ε_TLS`, `ε_OS`, `ε_AES(Q)` and, most importantly, an applicable `ε_DCF-pool` term are not established for this source revision and these package schemas. It would be misleading to sum symbolic labels as a completed reduction. The S6/S7 code and transcript enumeration establish functionality and field locations only.

### 8.3 P0 and P1 simulator status

For P0, an ideal simulator would sample P0's input share and ideal output share, simulate P0's local packages (including unused/correlated keys in M6B5/v2 or the complete M6S2 slot index), its `sigma_0/tau_0/a_0/e_0/r_0` view, and then answer each local DCF evaluation and opened-z/bit message consistently with the output. Its sample choice is generated by P0 itself from fresh entropy, but the simulator still must produce the same current sample disclosure and adaptive later endpoints as the functionality transcript. It cannot receive the real peer's internal ranks, full threshold or future sample seed.

For P1, the simulator must generate the corresponding other party key shares, masks and shuffle components, accept the sample positions and each P0 message as public transcript, and produce the same opened bits and future C/slot access sequence. The P0 and P1 simulators differ in DCF output-share correction and local shuffle shares, but encounter the same unproved multi-key joint distribution at step 4.

There is no valid completed simulator for either party in the current evidence. The precise distribution break is: conditioned on public masked operands and opened comparison output, the party-local DCF correction/key bytes and mask shares are not shown computationally indistinguishable from a distribution generated using only that party's input share and L; moreover this must hold jointly for all used and unused slots under adaptive query/address selection. Without this lemma, later shuffle/output hybrids have no starting distribution. We found no concrete distinguisher; lack of a counterexample is not proof.

## 9. Causal rounds, comparison costs and scope

| Count | S6 algorithm/implementation value | Meaning |
|---|---:|---|
| Algorithm iteration slots | 5 | four potential sample/pivot candidate reductions then exact fifth candidate selection; early answer pads inactive trace slots |
| Maximum plaintext comparator dependency layers | 9 | at most two per first four active reductions plus final exact clique; this is a graph dependency count |
| Conditional CA comparison-core expression | `2k+1`; for `k=9`, 19 | applies only if that CA compiler's assumptions are proven; does not absorb sample-schedule messages |
| Protocol I literature core | 3 | paper-core number for its stated primitive boundary; not this complete implementation |
| Full S6 raw-score→original-order mask exchanges | `9+2L+A`, maximum 31 | L nonempty comparison layers, A active sample schedule messages, fixed adapter/shuffle/membership/inverse/final phases |

The concrete phase DAG for the reviewed runtime is:

```text
package binding (phase 3)
  → raw-score carry (4) → sign/lift (5)
  → forward shuffle metadata (20 → 21)
  → for each active Select iteration i:
       publish current shuffled sample only (40+i)
       → each causal compare layer j: masked operands (10+2j) → opened DCF bit (11+2j)
  → threshold membership operands (60)
  → inverse route (22 → 23)
  → final success status (61)
```

This is a partial order: the sample exchange for an iteration occurs only when that iteration's state is ready; later sample phases cannot precede earlier opened-bit dependencies. `L` counts nonempty comparator layers, and each contributes the two select exchanges shown. The fixed nine exchanges are phases `3,4,5,20,21,60,22,23,61`; `A` is the number of active schedule exchanges. On failure, local exceptions attempt the same phase-61 ABORT/no-mask status, while peer timeout/disconnect exits without publishing the complete mask. An atomic output publication protocol against a crash after one party's final success exchange is not proved by this DAG.

The five iteration slots are not five online rounds. The observed S6 N=256 cases have `(comparison layers, active sample exchanges, total exchanges)` `(6,3,24)`, `(8,4,29)`, `(9,4,31)`, `(9,4,31)`, `(9,4,31)`, `(6,3,24)`, `(6,3,24)`, `(9,4,31)`. These exact counts and independently rerun values are carried in the machine case record. No complete path is labeled 11 or 14 rounds.

Worst logical comparisons remain `12N + C(N,2)`: 9,664 at N=128 and 35,712 at N=256, i.e. O(N²). Historical S6 fixed-matrix diagnostics are min/median/max 299/415/3,942 for N=128 and 618/692.5/20,650 for N=256. S2's expected-cost proof assumes ideal fresh uniform samples without replacement; S6's OS CSPRNG implementation has no implementation-specific expected/high-probability theorem in this report. E2E comparison counts are not time or a formal bound.

The separate S7 same-task reconfirmation observed N=128 comparison counts `[298,616,309,2131,3114,299,2402]` (7 successes; min/median/max `298/616/3114`) and N=256 counts `[615,601,723,21341,13881,764,609,19204]` (8 successes; `601/743.5/21341`). These differ from S6's OS-random sample run, are diagnostic counts only, and are not an expected-cost estimate, tail bound, or time-performance result.

## 10. Gate decisions

| Gate | Result | Explanation |
|---|---|---|
| `S6_SOURCE_RECEIPT` | `PASS_BY_NONPARTICIPANT` | exact file-hash snapshot; receiver not involved in S4/S5/S6; only claimed rerun scope in manifest |
| `FUNCTIONAL_E2E_N256` | `PASS_BOUNDED` | all fixed 23 S6 cases rerun with the frozen oracle/no-mask assertions; only N≤256 |
| `SEED_NONDISCLOSURE` | `PASS_FOR_S6_RUNTIME; HISTORICAL_S5_SEEDS_PRESERVED` | no S6 runtime/final evidence raw seed; old S5 archive has seed fields |
| `MATERIAL_ONE_SHOT_AND_RESTART` | `PASS_LOCAL_POSIX_CROSS_PROCESS; HOST_REBOOT_NOT_TESTED` | one-use, race, child-exit-before/after-claim, replay and digest cases; no OS reboot/power-loss test |
| `DCF_SINGLE_KEY` | `PASS_FUNCTIONAL_ABI_ONLY` | conformance of direction/width/strictness; not DCF share-privacy |
| `ADAPTIVE_FULL_POOL` | `OPEN` | full multi-key correlated pool cannot currently be simulated |
| `SHUFFLE_OUTPUT_COMPOSITION` | `OPEN` | no composition simulator for forward/inverse route, membership and output/abort |
| `JOINT_PARTY_VIEW` | `OPEN / NO_GO_FOR_SECURE_CLAIM` | no complete P0/P1 proof, and no attack identified |
| `CONDITIONAL_SECURITY_STATUS` | `NOT_ESTABLISHED` | no basis for “conditionally secure” wording or secure alias |
| `N_GT_256_ADMISSION` | `NO_GO` | N=257 preflight rejection; larger sizes untested and unsupported |
| `V3_V4_PERFORMANCE_STATUS` | `V3=OPEN; V4=NOT_ALLOWED/NOT_RUN` | no formal LAN/WAN matrix; existing plan prerequisites remain |

`PROJECT_DERIVED_SELECT_5ITER` can be described as a **bounded project-derived stable Top-K functionality candidate**, conditional on the tested source/schema/platform and the formal output contract. It cannot be called secure, conditionally secure, BB90, BMW16 or production-ready. The route does not replace or rename the BB90 row in the six-variant plan.

## 11. Files modified in S7

No C++ runtime or test source was changed; no reproducible code defect required a patch. S7 changes in its isolated worktree are limited to:

- `PROJECT.md` — append dated S7 receipt/security status, preserving S6 history;
- `docs/IMPLEMENTATION_PLAN.md` — add S7 gate status and independent receipt scope;
- `docs/REFERENCE_MANIFEST.md` — add S7 evidence index and correct seed-history distinction;
- `docs/decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S7_DECISION_2026-10-14.md` — current stage decisions;
- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S7_REPORT_2026-10-14.md` — this detailed audit;
- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S7_SOURCE_MANIFEST_2026-10-14.json` — receiver/source/binary/log hashes and gates.

S6 source files, S5 historical evidence and main checkout differences remain unchanged. The final S7 `git diff --check` and changed-path inventory are recorded in the source manifest/report update.

