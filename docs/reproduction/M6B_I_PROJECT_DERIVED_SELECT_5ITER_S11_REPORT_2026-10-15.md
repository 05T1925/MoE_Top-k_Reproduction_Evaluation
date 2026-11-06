# M6B-I PROJECT_DERIVED_SELECT_5ITER S11 DCF bridge and joint-view review

Date: 2026-10-15  
Route: `Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / EXPERIMENTAL`  
Decision: **The S8 DCF key generator can be bridged to the optimized DCF FSS theorem under a precise restricted-key AES PRG assumption and a small, stated initial-control-bit extension. A correlated-pool/adaptive-address hybrid is available for the DCF-key layer. The complete Protocol I party-view proof remains OPEN because the S8 package-delivery boundary and the exact contents of the approved leakage function are not closed, and crash-time output publication is not atomic. No alternative DCF or runtime change is justified by this review.**

## 1. Scope and receipt identity

This is a source-level security review of the **same 58-path S8 snapshot independently received in S10**. It is not a new functional receipt and does not claim that this reviewer was uninvolved in S4–S8. The S10 receiver `/root/s8_independent_receipt` reported no S4–S8 implementation or decision participation and performed the fresh Release build, seven directed target runs, default-OFF check, and 24 T/P0/P1 cases. Those results remain S10 evidence; S11 did not rerun them.

The S8 code was an uncommitted snapshot based on `c3926c68fd14f270faa8b55234311071947fa080`, not a mainline merge. S10's S8 manifest SHA-256 is `c5d29679aeaf1be3cc71da1ffc8053b33d589fd15313031137b5b79655e1dffb`. The S11 receipt ledger records the 58-file SHA-256 set and canonical sorted path/hash digest `17f1827077bc3f9dbf6038688bbda466162c4fe53e6436d34abb09703725f957`. At closeout, a read-only revalidation against the preserved S8 worktree matched all 58 entries. S11 then appended status/reference addenda to `PROJECT.md`, `docs/IMPLEMENTATION_PLAN.md`, and `docs/REFERENCE_MANIFEST.md`; their S8 receipt hashes remain in the manifest and their post-addendum hashes are separately recorded there. The full file ledger is [S11 source manifest](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S11_SOURCE_MANIFEST_2026-10-15.json).

S10's corrected counter erratum is preserved and cited, not rewritten. Its successful functional evidence covers the S8 seed-publication change. S10 reports the tested E2E binary SHA-256 `5758db8c1950474eba4b97e98d65c1fb598214f9d8fe9f627ed53c6409d8d3ab`; S11 did not rebuild it or claim that binary hash as an S11 build. The S10 build-provenance differences and its scope limitations remain as recorded there.

S11 worktree: `C:\Users\28641\.codex\worktrees\m6b-project-derived-5iter-s11`, branch `codex/m6b-project-derived-5iter-s11`, base `c3926c68fd14f270faa8b55234311071947fa080`. S11 made no C++ or build-system changes and ran no build or tests. The exact S10 report, source manifest, counter erratum, and erratum JSON hashes are recorded in the S11 source manifest.

## 2. Evidence classes and model

The review separates:

1. **Paper theorem:** Boyle, Chandran, Gilboa, Gupta, Ishai, Kumar, and Rathee, “Function Secret Sharing for Mixed-Mode and Fixed-Point Secure Computation,” EUROCRYPT 2021, full version. Definition 2 (FSS security) and Definition 3 (DCF) are on pp. 10–11; optimized DCF Figure 1 and Theorem 2 are on pp. 12–13; the proof is Appendix F.1, pp. 42–45. The official full version is [Microsoft Research PDF](https://www.microsoft.com/en-us/research/wp-content/uploads/2021/01/FSS.pdf); the publication record is [Microsoft Research / EUROCRYPT 2021](https://www.microsoft.com/en-us/research/publication/function-secret-sharing-for-mixed-mode-and-fixed-point-secure-computation/).
2. **S8 local behavior:** the checked-in `dcf.cpp`, `protocol_i_ucmp.cpp`, `protocol_i_score_input.cpp`, `protocol_i_parallel_shuffle.cpp`, Select5 runtime, serializers, and material stores at the S8 hashes in the manifest.
3. **S11 derivation:** the source-to-Figure-1 map, random initial control-bit extension, multi-key hybrid, and shuffle equations below. These are this report's proof steps, not claims made by the paper about this repository.
4. **Still unverified deployment assumptions:** one-party package isolation, store rollback policy, process/fork behavior of the dealer PRNG, approved coverage of every observed masked frame, and atomic publication of two-party outputs.

Threat model: trusted offline T generates input-independent correlated material and exits; T does not collude with either online party. At most one of P0/P1 is semi-honest corrupted. P0/P1 use private authenticated channels. Active corruption, T-plus-party collusion, host/admin compromise, and a malicious dealer are excluded. These exclusions do not establish that the current filesystem actually enforces party package isolation.

## 3. Actual DCF and uCMP ABI

### 3.1 Function and state

For a comparison width `b` (34–53 in the adapter), the uCMP constructor computes

~~~text
R = Z/(2^b),  alpha = (mask_left - mask_right) mod 2^b,
DCF = keyGenDCF(Bin=b, Bout=64, alpha, beta=1, groupSize=1).
~~~

The DCF function is `1` when its `b`-bit input is strictly below `alpha`, and `0` otherwise, represented in the additive group `Z/(2^64)`. `eval_strict_lt` forms `x=(z_left-z_right) mod 2^b` and `y=(x-2^(b-1)) mod 2^b`, evaluates the same local DCF key at both public points, applies the party-specific wrap correction, and returns additive shares of the strict comparison. The party ABI therefore exposes one key per logical uCMP slot but makes two DCF Eval calls per logical comparison. Score conversion uses the same key family at 34 input bits; Select uses 40 bits at padded N=128 and 41 bits at padded N=256; membership uses the Select width.

Each logical mask is sampled uniformly with OS `getrandom()`. `share_mask` samples party 0's additive share uniformly and sets party 1's share to `mask-share0 mod 2^b`. For a slot, `alpha=m_left-m_right`; the local party sees only its two mask shares, not either full mask or `alpha`. The transmitted operand shares sum to `input_left+m_left` and `input_right+m_right`. The opened comparison bit and operands are in the current approved leakage contract; a mask share is not itself sent.

### 3.2 Key generation/evaluation map to the theorem

`VFSS/ext/FSS/dcf.cpp::keyGenDCF` implements the binary-tree correction-word construction in Figure 1:

| Paper object | S8 implementation | Review result |
|---|---|---|
| Input domain `{0,1}^b` | `Bin=b`; index bits are consumed MSB first | Match |
| Output group `G` | `GroupElement=uint64_t`, `Bout=64`, `groupSize=1` | `Z/(2^64)` Abelian group |
| Threshold/payload | `idx=alpha`, `payload=1` | DCF `1[x<alpha]` |
| Node expansion | AES under `state & ~3`, encrypt fixed blocks 0,1,2,3 | A fixed bit projection of a four-block PRG expansion |
| State and control | two `si` children, two `vi` blocks, LSB control bits | Figure-1 `(sL,vL,tL,sR,vR,tR)` fields, up to fixed encoding/projection |
| Correction words | `k[i+1]` packs seed correction and two control corrections; `v[i]` carries group correction | Figure-1 shared `sCW || VCW || tL_CW || tR_CW` |
| Final correction | `g[0]` shared by the two key packs | Figure-1 `CW(b+1)` |
| Evaluation | `traversePathDCF`, correction words, final `g`, party-1 sign | Figure-1 `Eval`, additive share in `Z/(2^64)` |
| Wire format | `M2UC/v1`, party/bits/Bout/group size, `b+1` 16-byte blocks, one 64-bit `g`, `b` 64-bit `v` values | Serialization of the full local FSS key and public parameters; no `alpha` field |

The runtime uses `FSSConfig::prngs[omp_get_thread_num()]`. Before S8 dealer generation, `seed_fss_once()` obtains 256 independent 128-bit seeds from `getrandom()` and seeds cryptoTools AES-counter PRNGs. Each DCF key generation consumes two 128-bit root blocks from that stream. The DCF expansion masks the low two state bits before AES; its node seed is therefore a 126-bit value. With `lambda=126`, define the concrete FSS PRG assumption as the distinguishing advantage for the implementation's fixed four-counter AES expansion under a uniformly random 126-bit key embedded with the low two key bits fixed to zero. `convert(Bout=64,groupSize=1)` takes a fixed 64-bit projection from the AES output block; a fixed byte/bit projection does not change the PRG assumption. The stream that supplies root blocks is a separate AES-counter PRG seeded by OS entropy.

This is **not** a proof from the ordinary full-key AES-128 PRP assumption alone: the DCF expansion keys are restricted to the `2^126` subset with two fixed low bits. The theorem bridge is parameterized by that exact restricted-key PRG assumption, plus the separate AES-counter-generator and OS-CSPRNG assumptions. No concrete advantage number is claimed.

### 3.3 Initial control-bit extension

The paper's Figure 1 writes the initial control bits as `(t0,t1)=(0,1)`. The S8 generator instead samples a random root block for party 1 and forces the party-0 LSB to its complement, so `(t0,t1)=(1-theta,theta)` for uniform `theta`; one additional root bit is unused by AES because the state is masked by `~3`.

For each fixed `theta`, the source equations are Figure 1 with the initial complementary control pair `(1-theta,theta)`: the `sign=(-1)^t1` term appears in the same VCW, V-alpha, and final-CW locations; the keep/lose branch, MSB-first walk, and two correction bits are unchanged. The Appendix F.1 hybrid proof uses the complementary-control invariant and PRG indistinguishability at each level. Replacing its fixed initial values by a fixed complementary pair leaves those hybrids unchanged; averaging over the independent `theta` gives the same key simulator, with `theta` included as an independent root field. The ignored root bit is independent randomness.

This last paragraph is the S11 proof extension from the published fixed-bit description to the actual randomized complementary bits. It is a short algebraic/generalization step, not a literal statement of Theorem 2. An independent cryptographic reviewer should verify this extension before any security-status upgrade.

## 4. Single-key simulator and advantage terms

Let `S_b(1^lambda,b,Gin,Gout)` be the DCF key simulator for party `b` from Definition 2 / Theorem 2. It receives only domain/output parameters, not `alpha`, `beta`, `z`, masks, or the other party's key. For this adapter, `Gin={0,1}^b`, `Gout=Z/(2^64)`. The theorem's key contains all root/correction/output material; the S8 serializer adds only fixed ABI metadata and field encoding.

For a corrupted P_b, the single-slot hybrid is:

1. Fix the dealer-selected `alpha` and `beta=1`; sample all local mask shares independently as in S8.
2. Replace the actual serialized key `K_b` by `S_b(1^126,b,Gin,Gout)`. The key distribution no longer depends on `alpha`.
3. At each actual public `z_left,z_right`, locally evaluate the simulated key at `x` and `y`. The corrupted party's own Eval outputs are deterministic post-processing of the simulated key and public operands. The peer Eval share it receives is reconstructed in the simulator as `opened_bit - local_share mod 2^64`; DCF correctness makes this exactly the real message relation.

The simulator does not need `alpha` or the honest party's secret input. For the score adapter's unopened intermediate carry/sign shares, it computes the corrupt party's local shares from the simulated key and the opened `z` operands; the resulting local `lift` and next-stage message are then deterministic. For opened Select comparisons it uses the approved bit in `L` as above. Membership results remain shares; their final output-share contribution is handled with the inverse-route simulator and ideal output share.

Write `epsilon_G` for the distinguishing advantage of the exact 126-bit restricted-key, four-counter AES expansion, and `epsilon_CTR(B)` for replacing the `B` bytes of the dealer's AES-counter stream with independent uniform bytes. For a single key at width `b`, Appendix F.1 yields a hybrid loss proportional to the `b+1` correction-word hybrids; conservatively record it as `(b+1)*epsilon_G` (up to the paper's single-step reduction convention). This is conditional on the source mapping above. `epsilon_OS` covers the OS CSPRNG words used for root seeds/masks, and `epsilon_TLS` covers the authenticated private-channel idealization. These remain symbolic assumptions, not measured probabilities.

## 5. Full pool and adaptive Select queries

The two package schemas remain separate security objects:

| S8 schema | DCF key inventory per party | Used/unused distinction |
|---|---:|---|
| N=128 `M6B5/v2` monolithic | Select `5*C(128,2)=40,640`; score carry/sign `2N=256`; membership `N=128`; unused forward+inverse M2CP edge keys `2*C(128,2)=16,256`; total `57,280` DCF keys | All 384 score/membership keys are used. Select keys used equal that E2E case's `comparison_calls`; the remaining `40,640-comparison_calls` are unused. All 16,256 M2CP keys are unused by Select5's two-round shuffle path. |
| N=256 `M6S2` streamed | Select `5*C(256,2)=163,200`; score `512`; membership `256`; total `163,968` DCF slots, plus `10N=2,560` shuffle metadata slots | All score/membership slots are used. Select used slots equal that case's `comparison_calls`; all other pre-generated select slots remain unused. There is no legacy M2CP DCF edge pool in this schema. |

The N=128 unused M2CP keys have the documented common-vector relation. If `R_i=r0_i+r1_i mod 2^b`, the edge threshold is `alpha_ij=R_i-R_j`, hence for `i<j<k`, `alpha_ij+alpha_jk-alpha_ik=0`. This is an algebraic relation among hidden dealer parameters, not a one-party distinguisher by itself. Each corrupted party receives only its own `r_share` vector and its own key for each edge. The one-key simulator is independent of every threshold value; it therefore simulates the entire vector for **arbitrarily correlated** thresholds, provided the per-key root randomness is independent/pseudorandom as specified. The other party's `r_share` and all `alpha_ij` are absent from the corrupt view.

The pool hybrid for P_b first replaces the dealer's AES-counter root stream with uniform blocks (loss `epsilon_CTR(B)` for the exact stream length), then replaces each of the `Q` local DCF keys, including every unused slot, with `S_b`. Since the FSS simulator's distribution depends only on input/output domain, not on the function parameters, the threshold vector may be correlated with mask shares, `r_share`, and other thresholds. A conservative symbolic pool loss is

~~~text
epsilon_pool <= epsilon_CTR(B) + Q*(b_max+1)*epsilon_G
                 + epsilon_OS + epsilon_TLS,
~~~

with `Q=57,280` for M6B5/v2 and `Q=163,968` for M6S2, and `b_max=40` or `41` respectively for those pools (score keys use b=34). This is a theorem-shaped hybrid bound, not an instantiated numeric security level; the exact restricted-AES and AES-counter advantages have not been bounded for the actual environment.

Adaptive addresses do not require a new DCF theorem in this model. The entire one-shot key pool is generated and bound before online input. Given the approved active-round seed, public endpoint/pivot coordinates, comparator bits, and the fixed controller rule, both parties derive the candidate set, residual rank, next edge addresses, and slot order as deterministic functions of `L`. The simulator replaces the full key pool before evaluating this deterministic post-processing. A key is used at most once per slot; a repeated endpoint in a later Select iteration has a different slot/key. The durable lease prevents local re-claim after a process restart under the tested POSIX no-rollback contract. M6S2's own manifest and per-slot digests can be recomputed from the simulated **local** bytes. This closes the *DCF-key/adaptive-address layer conditionally*; it does not close material delivery or whole Protocol I output security.

The active sample seed is generated by P0 only after both forward-shuffle exchanges and only when that iteration becomes active. It is in `L_S8`; it is not the DCF alpha or a key seed. The simulator receives that current seed, derives the same sample from the public candidate vector, and does not need future seeds. S10's independent S8 receipt and counter erratum remain the source for the functional and counter facts; S11 did not rerun the sample tests.

## 6. Shuffle, inverse route, membership, and outputs

For each two-round metadata shuffle, let `pi` be T's secret permutation; `sigma0,sigma1` are independent uniform permutations and `tau0=pi o sigma1^{-1}`, `tau1=pi o sigma0^{-1}`. Let `a0,a1,h,r0,r1` be independent uniform 192-bit record vectors, and `e0=-tau0(a1)-h`, `e1=-tau1(a0)+h`. For input shares `x0,x1`, the actual frames are

~~~text
m0 = sigma0(x0)+a0                 m1 = sigma1(x1)+a1
d0 = tau0(m1)+e0+r0 = pi(x1)-h+r0
d1 = tau1(m0)+e1+r1 = pi(x0)+h+r1
d0+d1 = pi(x0+x1)+r0+r1.
~~~

P0's `(sigma0,tau0)` and P1's `(sigma1,tau1)` are individually distributed as independent uniform permutations, independent of `pi`; each local `e_b` is uniform conditioned on the fields visible to that one party because `h` is fresh uniform. Thus the algebra supports a one-party simulator: take that party's input share and the masked shuffle frames from `L`; sample its local uniform permutation/r-mask fields conditional on its own outbound frames; derive `a_b/e_b` from the displayed equations; compute its local shuffled share as `d_b-r_b`. The inverse route is the same construction with `pi^{-1}` and the secret-shared membership carrier. The final local mask share is the low bit of that inverse-route local record and is supplied by the ideal output-share interface.

This is a candidate simulation, not yet a closed proof for the S8 implementation, for three precise reasons:

1. **Leakage field completeness:** S8's written `L_S8` says “approved masked operands” but does not enumerate the actual 24-byte record arrays on forward/inverse shuffle phases (first and second exchange), nor all per-party store/ready digests and lease records. The runtime sends those fields. The algebra above simulates them as masked records, but this report does not silently add them to the user-approved `L`. `SHUFFLE_OUTPUT_COMPOSITION` stays OPEN until the leakage contract explicitly includes exactly those observable fields or the runtime stops exposing them.
2. **Party-store isolation:** the N=128 staging API writes both `_p0.pkg` and `_p1.pkg` in one directory and the `M6B5/v2` ready marker contains both package hashes. The directory is mode 0700 and packages are mode 0400, but those modes do not isolate processes running under the same UID; `claim_party_package` receives the directory path and selects only its own file by convention. A P0 process with that same-UID directory access can open the `_p1.pkg` path and read P1's full DCF key pool. This is a concrete filesystem-view witness if the parties share that directory/UID; it is not a break of the DCF theorem. Production deployment must deliver party-specific stores under distinct principals/ACLs, and the S8 runtime API does not enforce that distribution. S10 already recorded the same-UID limitation; S11 binds it to the full-view proof.
3. **Output/abort atomicity:** the runtime returns a local mask share only after receiving final success, and E2E abort cases return no mask. It cannot atomically guarantee that both callers publish their shares together if one process crashes after its final-status exchange but before the peer completes. That crash prefix and the external caller's publication policy are not covered by the current simulator or a tested two-party atomic-output primitive.

These are the first whole-view stopping points. No concrete cryptographic distinguisher against the isolated DCF key distributions was found; the full `L_S8` party-view proof is not closed. **未发现反例，但证明未闭合。** A DCF replacement would not repair a party that can read the other party's package or an underspecified leakage/output boundary.

## 7. Gate decisions and alternative DCF decision

| Gate | S11 decision | Scope |
|---|---|---|
| `DCF_SINGLE_KEY_PRIVACY` | `CONDITIONAL_BRIDGE_ESTABLISHED` | BCG+21 optimized DCF theorem under the exact restricted-key AES PRG, AES-counter PRNG, OS-CSPRNG, and initial-control-bit extension above. Not independently reviewed. |
| `CORRELATED_POOL_SIMULATION` | `CONDITIONAL_PASS_DCF_KEY_LAYER` | Product hybrid covers all used/unused local keys and arbitrarily correlated thresholds, including M6B5's common-r cycles, after replacing the root PRNG stream. Does not simulate non-DCF package fields by itself. |
| `ADAPTIVE_QUERY_SIMULATION` | `CONDITIONAL_PASS_DCF_KEY_LAYER` | Full pre-generated pool is simulated before addresses/bits are post-processed from approved L. One-shot/no-rollback assumptions apply. |
| `SHUFFLE_OUTPUT_COMPOSITION` | `OPEN` | Masked-record L coverage, per-party package isolation, and crash-time output publication are not closed by current interfaces. |
| `JOINT_PARTY_VIEW` | `OPEN / NO_GO_FOR_SECURE_CLAIM` | No full S8 P0/P1 simulator under only confirmed L and ideal output is accepted; no secure alias. |

The S11 condition for evaluating an alternate is **not met**: actual DCF has an effective theorem bridge. No second DCF ABI, package, runtime, or candidate algorithm was created. The shortest next step is not to swap DCF: specify and enforce party-scoped M6B5/v3 material delivery (each party receives only its own package and its own ready digest), enumerate the exact shuffle-frame/manifest fields in the approved L without revealing any prohibited values, and define the abort/output-share publication interface. Then have a separate reviewer instantiate the two simulators and recheck the randomized-control-bit extension and concrete AES advantage mapping. Until then the route stays EXPERIMENTAL.

## 8. Functional, performance, and route status

S10 independently received the S8 functional snapshot: 7 directed Release targets, default-OFF target check, 24 T/P0/P1 cases (18 frozen-oracle successes, 6 abort/no-mask), with N≤256 coverage. Its report and erratum remain the functional/counter sources; S11 did not rerun them. These demonstrate bounded function behavior, not the simulator above.

No secure alias was created. No code or tests changed. Formal LAN/WAN remains `NOT_ALLOWED/NOT_RUN` because M6A V3 remains OPEN and the existing V3→V4 prerequisites remain unchanged. `N>256` remains `NO_GO`. The route remains project-derived and does not become BB90 or BMW16.

## 9. Reproduction and limits

The S11 review used source inspection of the 58 manifest files, the S10 report/erratum/build-provenance files, and the official BCG+21 full version. There is no S11 build command or binary hash because no code was changed and no tests were run. To reproduce the source receipt, validate each `files[].sha256` in the S11 manifest against this worktree; to reproduce the inherited functional receipt, follow the commands and evidence hashes in the S10 report/manifest, without labeling that rerun as an S11 rerun.

Closeout validation had one tooling-only failed attempt: the first PowerShell hash-check script treated a JSON `PSCustomObject` as a dictionary and stopped at `GetEnumerator()`. The check was corrected to iterate `PSObject.Properties`; JSON parsing, all 58 S8 hashes, the three documented overlay hashes, artifact hashes, and `git diff --check` then passed. This did not alter source or evidence files. No build, CTest, E2E, or performance command was attempted in S11.

All S8/S10 evidence stays untouched. No paper, secret key, certificate, material package, build artifact, log, `VFSS-baseline/`, or local reference project was modified. Formal performance was not run.

Machine-readable gate summary: [S11 gates](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S11_GATES_2026-10-15.json). The same dispositions are recorded in [S11 decision](../decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S11_DECISION_2026-10-15.md). The S11 document addenda do not alter the S8 source receipt or S10 test results.

