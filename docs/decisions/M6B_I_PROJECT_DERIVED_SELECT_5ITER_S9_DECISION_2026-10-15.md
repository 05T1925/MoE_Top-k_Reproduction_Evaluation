# M6B-I PROJECT_DERIVED_SELECT_5ITER S9 decision

Date: 2026-10-15

Route: `Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / EXPERIMENTAL`

## Decision

The S8 seed-v3 snapshot was reconstructed and freshly rebuilt in a new isolated worktree. All 59 selected files matched the S8 source manifest before S9 documentation changes. The same executor performed S8 implementation/decisions and earlier S4–S7 work, so this is **not an independent receipt**: `S8_SOURCE_RECEIPT=NOT_INDEPENDENT`. The fresh same-executor replay passed seven directed targets, default-OFF target inspection, and all 24 T/P0/P1 E2E cases.

The bounded functional evidence supports the project's stable Top-K output behavior for the tested padded `N≤256` matrix. It does not prove security. No reproducible code defect or concrete distinguisher was found. `JOINT_PARTY_VIEW=OPEN`; keep the route opt-in and EXPERIMENTAL. Do not create a `secure` alias.

## Source and build identity

- S8 source base: `c3926c68fd14f270faa8b55234311071947fa080`, detached in S9; S8 was not merged into main.
- S8 manifest SHA-256: `c5d29679aeaf1be3cc71da1ffc8053b33d589fd15313031137b5b79655e1dffb`.
- Reconstructed source set: 55 S8-selected inherited paths, including 9 paths overwritten by S8; three new S8 report/decision/case files; and the S8 manifest: 59 unique files. All hashes matched.
- Build: Ubuntu 24.04.4 under WSL2, x86_64, CMake 3.28.3, GCC 13.3.0, OpenSSL 3.0.13; Release, `BUILD_TESTING=ON`, EMP OT OFF, Select5 experimental ON.
- Build directory: `/tmp/m6b-project-derived-5iter-s9-release`.
- Separate Release opt-in OFF configuration has no Select5, streamed slot-store, or TLS conformance targets.
- This S9 executor participated in S8 and S4–S7. This report is a same-executor replay and must not be labeled independent acceptance.

The complete source, target-binary, external-evidence, and per-file hash records are in [S9 source manifest](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S9_SOURCE_MANIFEST_2026-10-15.json). The 24 machine-readable case rows are in [S9 E2E cases](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S9_E2E_CASES_2026-10-15.json).

## Functional decision

All seven S8 directed targets were rebuilt from the S8-selected source snapshot and exited 0:

1. Select5 control/conformance;
2. monolithic durable package store;
3. M6S2 streamed slot store;
4. TLS 1.3 mTLS transport conformance;
5. three-process T/P0/P1 Select5 E2E;
6. uCMP/DCF conformance;
7. parallel shuffle conformance.

The E2E emitted 24 rows: 18 oracle successes and six bilateral ABORT/no-mask cases. Every success compares `mask0 XOR mask1` bit-for-bit with the frozen C++ `top_k_mask()` oracle in the TEST_ONLY controller and checks original input order, `n` mask length, binary bits, weight `K`, bidirectional counter agreement, and rejection of a second material claim by each party. The controller is the only location that reconstructs the XOR mask for the oracle check.

| Padded N | K values (success rows) | Comparison-call diagnostics min / median / max | Result |
| ---: | --- | ---: | --- |
| 2 | 1 | 1 / 1 / 1 | Oracle pass |
| 8 | 1, 7 | 13 / 16.5 / 20 | Oracle pass |
| 128 | 1, 2, 8, 64, 65, 128, repeated 64 | 303 / 349 / 4,339 | 7 oracle passes |
| 256 | 1, 2, 8, 64, 128, 255, 256, repeated 128 | 620 / 776 / 17,260 | 8 oracle passes |

The six no-mask fault rows cover package tampering, peer disconnect/timeout, mismatched party K, wrong TLS peer identity, T exiting before ready publication, and malformed sample-seed magic on the actual TLS runtime after forward shuffle. Preflight rejects N=256 monolithic and N=257 streamed configurations. The conformance target also checks wrong seed version/iteration, truncation/trailing bytes, cross-session frame binding, replay/skip/out-of-order iteration, invalid sizes, and pre-shuffle disclosure-state rejection.

Comparison counts vary because each E2E execution uses fresh OS sample randomness. They are diagnostic counts from functional cases, not timing, a bound on likely work, an expected-cost result, or formal performance data. The S2 ideal uniform-without-replacement expected-cost derivation does not automatically apply to the S8 SHA-256-expanded stream.

## Seed disclosure and leakage contract

The S8 v3 online binding contract is preserved. Both forward-shuffle exchanges (phases 20 and 21) complete before the controller can invoke the sample source. At entry to each active reduction iteration only, P0 obtains 32 bytes via `getrandom()` and sends a 41-byte payload at phase `40+i`; P1 validates the exact payload and active iteration and returns a 9-byte acknowledgement. The framed channel binds session, fingerprint, padded N, K, bit width, sender/receiver, phase, frame type, and one-shot material ID. A future iteration's seed is not pre-generated, stored, or derivable from the current seed.

The seed is part of the approved `L_S8` after disclosure. Both parties can derive the current sample order and sample clique as soon as phase `40+i` is received. This exposes the current shuffled sample positions before their endpoint messages; it does not reveal raw score, stable key, pivot value, threshold, original-index mapping, DCF α/key, mask shares, or complete mask. The seed is not placed in either offline package format, M6S2 manifest, ready/start metadata, initial binding, or S9 E2E logs. S9 case records expose only active phase numbers and sent/received byte counts, never seed bytes.

The fixed sample size remains `s=min(m,ceil(sqrt(m)))`. Both parties run the same partial Fisher–Yates mapping over the same public candidate vector. The deterministic stream is SHA-256 over the specified v3 domain, seed, session BE64, fingerprint BE64, iteration byte, and counter BE64, with BE64 words and rejection sampling. This checks reproducibility and unbiased reduction conditional on uniform stream words; it does not prove that SHA-256 expansion or OS randomness has an exact finite-scale uniform distribution.

## Materials and actual DCF execution

These are two distinct party-view schemas and are not combined for security claims.

| Schema | Padded N | DCF slots by layout | Other slots | Observed serialized/staged bytes |
| --- | ---: | ---: | ---: | ---: |
| M6B5/v2 monolithic | 128 | `5*C(128,2)+2*C(128,2)+3N = 57,280` key pairs/party. The `2C` forward/inverse shuffle pools are retained in the package but the Select5 two-round route does not evaluate them. | Forward/inverse shuffle metadata and per-slot local mask shares | 60,008,572 B/package/party |
| M6S2 streamed | 256 | `5*C(256,2)+3N = 163,968` DCF key slots/party | `10N=2,560` purpose/direction-bound shuffle metadata addresses; 166,528 total addresses, 11 segments | 185,925,312 B staged/party, including a 12,656,192 B manifest |

These byte figures are local serialization/staging evidence. They are not peak RSS, network package sizes, or benchmark results. Select slots bind iteration, purpose, scope, direction and shuffled endpoints. The streamed store has per-slot integrity and durable one-shot lease; a failed/replayed claim fails closed. The monolithic package binds session, fingerprint, party, N/K/width and material ID and is protected by a durable package claim. The local POSIX tests cover integrity, wrong bindings, concurrent claims and crash/restart behavior; production cross-host storage semantics, administrator rollback/deletion, supervisor reliability, and certificate lifecycle remain outside this evidence.

For each uCMP slot, the dealer samples masks `m_l,m_r` in the comparison ring and invokes the actual `keyGenDCF(b,64,alpha=m_l-m_r mod 2^b,groupSize=1)`. Each party receives its serialized local DCF key `(k,g,v)` plus its own additive mask shares. At runtime both parties exchange their local masked operands and reconstruct the approved masked values `z_l=key_l+m_l`, `z_r=key_r+m_r`; the party wrapper evaluates the local DCF share at `x=z_l-z_r` and `y=x-2^(b-1)`, applies the party-1 correction, and returns strict-`<` output shares. Their sum opens one comparison bit. Membership uses `1-strict_lt(tau,key_i)` to test `key_i≤tau` for unique stable composite keys. The DCF input bits are traversed MSB first in `VFSS/ext/FSS/dcf.cpp`.

The code confirms the hidden correlation: the per-party serialized key, that party's mask shares, the dealer-chosen DCF threshold, the public masked operands and the opened result bit belong to one joint distribution. A standalone masked value being uniform does not establish that distribution. In M6B5/v2, each unused all-pairs shuffle pool derives thresholds from a common vector of `r` values and has within-pool cycle relations. M6S2 omits those DCF pools and instead exposes a party-local manifest, segment digests, slot names, lease state and adaptive access trace. A proof for one format cannot be reused for the other.

## Joint party-view security decision

Model: trusted offline T generates both parties' material, knows the paired shuffle permutation `Pi` and full masks, publishes party materials, then exits before online execution. P0/P1 use private authenticated TLS 1.3 channels. At most one online party is semi-honest corrupted. T/party collusion, active corruptions, malicious Dealer behavior, administrator rollback, and timing/scheduling side channels are excluded. T's view is not part of either P0/P1 view under the explicit no-collusion assumption; E2E controller reconstruction exists only in TEST_ONLY.

Approved leakage includes the active phase-`40+i` seed, current shuffled endpoint and pivot positions, key-slot/address labels, comparison bits/call counts, masked operands, lengths/phases, and uniform ABORT/no-mask status. Each party's local view also contains its entire local package or streamed manifest and any unconsumed slots, its local input share, DCF key shares/mask shares, shuffle/inverse state, frames, durable lease/restart status, and its output mask share. The other party's private shares and full mask are not given to a simulator.

For P0, a simulator given only P0's ideal input/output share and `L_S8` can sample disclosed sample seeds and reproduce public sample/edge scheduling. It still lacks a proven way to sample P0's actual full local DCF pool, including unused M6B5/v2 keys and adaptive M6S2 slot/lease view, jointly conditioned on P0's input share, local mask shares, opened masked operands, comparison bits, adaptive addresses and final output share. The P1 simulator has the symmetric missing distribution. A candidate hybrid can replace TLS with an authenticated ideal channel, `getrandom()` with ideal random words, and the FSS AES PRG/PRF with ideal responses; the next hybrid would require an applicable whole-pool lemma. No such lemma or reduction is established for either serialized schema. Therefore no advantage expression beyond symbolic `ε_TLS + ε_OS + Adv_AES(Q) + ε_pool + ε_shuffle/output` is justified; the latter terms are unbounded/unproved here.

This is the first missing step for both parties. Since it is not closed, subsequent forward/inverse shuffle, threshold membership, XOR output share, abort, restart, and failure-prefix hybrids also lack a proven starting distribution. **No concrete distinguisher was found, but the proof is not closed.**

## S8 failure provenance and inconsistencies

S8's documented Ubuntu 20/CMake 3.16 configuration failed before compilation; it has no test binary. The initial malformed-seed hook was rejected by runtime config validation before reaching the seed phase. S8's first E2E attempt used a stale binary but did not retain that failed binary's hash or complete raw log, so S9 cannot compare its exact code/configuration/binary identity with the final fresh build. Those failures stay documented in S8 and are not overwritten.

S9 matches S8's recorded toolchain and Release flags, but five target binary hashes differ and two match. The five differing S9 binaries contain the S9 absolute source path; because complete byte-level normalization was not established, S9 results are bound only to the recorded S9 hashes and are not called byte-identical to S8 binaries. S8's N=128 values in its case JSON are `298,299,303,368,747,2094,2479`, whose recomputed min/median/max are `298/368/2479`. The S8 report and source manifest state `298/747/2479`, while the S8 decision states `299/1825/5340`. S9 does not rewrite that history and uses its own 24 parsed case records for its numbers.

## Gates

| Gate | S9 ruling |
| --- | --- |
| `S8_SOURCE_RECEIPT` | `NOT_INDEPENDENT`; same executor participated in S8 and S4–S7. Fresh same-executor reconstruction/replay passed. |
| `SAMPLE_SEED_AFTER_SHUFFLE` | `PASS_BOUNDED_SOURCE_AND_E2E`; generated after phases 20/21, active iteration only. |
| `PER_ROUND_SEED_BINDING` | `PASS_BOUNDED`; phase/session/parameter/direction/material binding and malformed/replay checks passed. |
| `FUNCTIONAL_E2E_N256` | `PASS_BOUNDED_SAME_EXECUTOR`; exact stable-oracle matches across specified K matrix. |
| `MATERIAL_ONE_SHOT_AND_RESTART` | `PASS_BOUNDED_LOCAL_POSIX`; production storage and admin rollback are not measured. |
| `DCF_SINGLE_KEY_PRIVACY` | `OPEN`; actual local key and mask correlation has no established applicable privacy proof here. |
| `ADAPTIVE_FULL_POOL_VIEW` | `OPEN`; full used/unused pool view under adaptive address selection is unsimulated. |
| `SHUFFLE_OUTPUT_COMPOSITION` | `OPEN`; no forward/inverse/membership/output/abort simulator. |
| `JOINT_PARTY_VIEW` | `OPEN`; no P0 or P1 simulator/complete hybrid. |
| `SECURE_ALIAS_READY` | `NO`. |
| `N_GT_256_ADMISSION` | `NO_GO`; N=257 preflight rejects. |
| `V3_V4_PERFORMANCE_STATUS` | V3 `OPEN`; V4/formal LAN-WAN `NOT_ALLOWED/NOT_RUN`. |

M6A gates are unchanged: V3 still requires V1/V2 and base-interface handoff, correct Protocol I+AAV86 and Protocol III+AAV86 implementations, defined adaptive material/Dealer boundary, both leakage/round audits, and required counters; V4 additionally requires V3 complete, a fixed BB90 version/parameters/probability statement, both complete combinations, and the stable-threshold/DCF audit. S9 neither satisfies nor bypasses these prerequisites.

## Next review action

An S4–S8 nonparticipant must independently reconstruct these same 59 source files from the S8 manifest, use a fresh build/evidence directory, and rerun the recorded tests before any independent-acceptance label. Separate security work must supply the missing DCF local-key/mask full-pool conditional-distribution lemma for **each** package schema, then a P0/P1 simulator for adaptive access and shuffle/inverse/membership/output/abort/restart composition. No code or naming change can substitute for those proofs.
