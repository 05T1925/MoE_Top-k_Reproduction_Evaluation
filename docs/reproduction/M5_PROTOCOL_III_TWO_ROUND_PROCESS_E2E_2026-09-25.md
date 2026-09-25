# M5-F Protocol III independent-process two-round Fselect closeout

Status: M5-F functional and causal process E2E complete on
`feat/m5-protocol-iii-two-round`, HEAD `8f18354` plus the uncommitted M5-F
diff. M5-E mathematics and the M3 three-round baseline are unchanged.
This is a project candidate, not an author-exact implementation or a final
Theorem 4.2 cost verdict.

## Process architecture and reuse

The existing M3 process tests supplied the repository pattern: `fork+exec`
roles, socket-pair IPC, a P2 offline distributor, separate controller input
release, framed P0/P1 transport, bounded waits, and child cleanup. M5-F reuses
`ProtocolIFramedChannel`, M5-E's `ProtocolIIITwoRoundParty`, the shared
CmpAgg embedded in its party package, M5-D field/field-DPF/field-mul
serializers, and the M3 process pattern. The process test is a new executable
because its two-round bundle and R1/R2 transcript differ from M3; no M3
runtime or harness source was changed. File polling, Protocol I shuffle
material, and online dealer communication are absent.

The test executable has four roles. Its controller holds clear test input and
the clear oracle; it launches P2, P0 and P1 as three separate `execv` child
processes. P2 receives only public configuration, a deterministic test seed,
and the two offline distribution socket descriptors. It generates the entire
input-independent preprocessing pair, serializes one local bundle to each
party, sends a size receipt to the controller, and exits. The controller
`waitpid`s P2 and receives both `bundle_ready` events **before** constructing
and releasing P0/P1 input shares on separate sockets. P2 has no input,
result, event, R1 or R2 descriptor. P0 and P1 receive only their own offline
bundle and input shares; they have distinct address spaces and one endpoint
each of two P0/P1 protocol socket pairs. The controller reconstructs the
field output only after both parties report completion.

The input boundary remains M5-E's padded additive priority-key shares and
already-field-shared, nonzero encoded records. The controller's clear-to-share
operation is **TEST_INPUT_SHARE_GENERATION**, not a secure conversion from
arbitrary ring payload shares. P0/P1 do not check encoded-record consistency
by reconstructing secrets. The selected target rank and `k` are public.

## Offline bundle and lifecycle

`ProtocolIIITwoRoundOfflineBundle` carries material identity and one party's
CmpAgg package, logical rank-mask shares, field payload mask shares and field
DPF keys, and field multiplication materials. Its `M5FB` version-1 header
binds field ID `F_(2^127-1)`, party, session, fingerprint, one-shot material
ID, logical and padded shape, `k`, target, comparison/rank widths and reserved
bytes. Sections have explicit counts and lengths; the decoder rejects
truncation, excess data, wrong bindings/counts, noncanonical field encodings,
and malformed nested keys/material. The outer offline socket message uses an
8-byte length prefix. Field elements use M5-D's canonical 16-byte big-endian
encoding. The embedded legacy FSS CmpAgg package still has same-build,
same-architecture native ABI constraints; the bundle is **not** claimed to be
cross-platform portable.

Each online process owns exactly one moved bundle and one state object; failed
online exchange destroys both. The controller issues unique material IDs
throughout the test and rejects an attempted duplicate. A failed run is never
retried with its material. There is no persistent replay database across
controller restarts; global uniqueness remains a controller responsibility.

| View | Secret material | Online values |
| --- | --- | --- |
| P2 | Full input-independent preprocessing pair; no online input | None; exits before release |
| P0 | Only P0 CmpAgg/rank-mask/field DPF/field-mul material and P0 input shares | Its own and peer masked protocol messages, P0 output share |
| P1 | Only P1 counterpart and P1 input shares | Its own and peer masked protocol messages, P1 output share |
| Controller (test only) | Clear test input/oracle, both output shares after completion | Release/status/report channels, never a secure party |

## Actual process transcript and causal check

The M5-E application header is 44 canonical bytes. The existing framed
transport adds 48 bytes per message and checks phase, round, sender/receiver,
session, fingerprint, shape, length and sequence. There is one send by each
party on each of two independent protocol channels:

| Round | Complete outbound per party, prepared before peer read | Body bytes | Logical bits, both parties |
| --- | --- | ---: | ---: |
| R1 | `n` masked priority-key shares (8 bytes/item); `n` field Beaver `d/e` opening-share pairs (32 bytes/item) | `44+40n` | `2n(comparison_bits+254)` |
| R2 | `n` masked-rank shares (8 bytes/item); `n` masked-field-payload shares (16 bytes/item) | `44+24n` | `2n(rank_bits+127)` |

The actual per-party sequence is:

- P0: `BUNDLE_READY → R1_PREPARED → R1_SENT → R1_RECEIVED → R1_IMMUTABLE → R2_PREPARED → R2_SENT → R2_RECEIVED → R2_IMMUTABLE → COMPLETE`.
- P1: `BUNDLE_READY → R1_PREPARED → R1_RECEIVED → R1_IMMUTABLE → R1_SENT → R2_PREPARED → R2_RECEIVED → R2_IMMUTABLE → R2_SENT → COMPLETE`.

P1's send syscall follows its peer read to prevent a blocking-send deadlock;
the entire byte vector and a test-only digest are fixed *before* that read,
and the digest is checked unchanged afterward. P0 also checks both buffers.
The controller validates each ordered event, including that both
`BUNDLE_READY` events precede online input release. The result-channel
one-byte controller ACK occurs after both protocol outputs exist and only
keeps descriptors alive until both peers finish; it contains no protocol
value and is not a P0/P1 round. No P0/P1 protocol send follows R2 receive.
The test expects exactly two P0-to-P1 and two P1-to-P0 protocol frames total:
one per direction per round. The result and event channels are test-control
channels and are excluded from online protocol byte counts. Bounded `poll`,
child exit collection and cleanup prevent indefinite waits on failures.

M5-E's algebra is retained: R1 opens only masked comparison keys and field
Beaver differences, then locally yields rank shares and shares of
`z_i·s_payload[i]`. R2 opens masked rank and masked payload; local field DPF
evaluation at `masked_rank_i−target` with `alpha_i=r_rank[i]` and
`beta_i=s_payload[i]⁻¹` yields the selected encoded field-record share. Raw
rank, raw payload, full `s_payload`, and scalar inverse are never sent.

## Verification

The process test runs 25 deterministic correct executions over `n=2,3,4,5,8`,
including `k=1`, `k=n`, and middle `k`; first, last and middle public targets;
strict scores, all-equal scores, duplicates, zero/max signed score patterns,
and fixed-seed random scores. Payloads include zero, one, `UINT64_MAX`,
duplicates and random `uint64`. All-equal `n=5` explicitly tests target ranks
0, 2 and 4, whose stable indices are 0, 2 and 4. `n=3/5` exercise the
project rank rings `Z4/Z8`; this is C-INSTANTIATION relative to paper `Z_n`.
Only the controller calls `stable_ranks_cmpagg`, reconstructs both output
shares in `F_(2^127−1)`, unpacks the record, and compares its key/payload to
the oracle.

Twenty real-process failure cases reject swapped-party, wrong
session/fingerprint/target/field/material-ID/n/padded-n/widths/counts,
truncated and extra offline bundles, wrong R1 round tag, truncated R1/R2
frames and peer closure in R1/R2. Offline faults reject before any online
input release. Online faults produce no pair of valid outputs and at least
one nonzero party exit. The other process is reaped or terminated by bounded
cleanup. There is no recovery exchange or material retry.

Run provenance: fixed source seeds in the test, Linux
`5.15.167.4-microsoft-standard-WSL2 x86_64`, GCC 11.4.0, Debug build,
HEAD `8f18354` plus uncommitted M5-F files, one process-test invocation for
the byte sample below. Command:
`build-vfss-debug/moe_topk_m5f_two_round_process_e2e_test`.
The sample is `n=5,k=5,target=4`, all equal scores. It measured:

| Item | Bytes or bits |
| --- | ---: |
| P0/P1 serialized offline bundle | 10,766 bytes each; 21,532 total |
| Dealer socket length prefixes | 8 bytes each; 16 total |
| P0→P1 / P1→P0 R1 framed wire | 292 bytes each; 584 total |
| P0→P1 / P1→P0 R2 framed wire | 212 bytes each; 424 total |
| Total online framed wire | 1,008 bytes |
| R1 / R2 logical online | 2,900 / 1,300 bits; 4,200 total |
| Application header + frame overhead | 92 bytes per message; 368 bytes across four messages |

Serialized offline bytes include M5-F binding and length fields; the 16
outer dealer prefix bytes are separate. Logical online bits exclude storage
padding and engineering headers. These are communication counts, not latency
benchmarks. Theorem 4.2 cost matching is **NOT_YET_INDEPENDENTLY_VERIFIED**.

`ctest --test-dir build-vfss-debug --output-on-failure`: 35/35 PASS, including
M5-E, M5-D, M5-B/C, historical M3 independent-process and raw-score tests,
and Protocol I regression. Existing ASan/UBSan build:
`ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir /tmp/m5d-vfss-sanitized -R 'moe_topk_m5(e_two_round_fselect|f_two_round_bundle|f_two_round_process_e2e)_test' --output-on-failure`:
3/3 PASS. Leak detection was disabled for this existing sanitizer configuration;
ASan memory checks and UBSan remained enabled.

## Evidence boundary and next stages

**PAPER_DIRECT:** Protocol III uses clique ranking, DPF-based field routing,
nonzero multiplicative payload masks and two online rounds for Fselect.
**PAPER_DERIVED:** sending payload multiplication openings with R1 permits
masked payload shares in R2 and local field DPF selection after R2.
**C-INSTANTIATION:** `F_(2^127−1)`, key/payload packing, field DPF wrapper,
`Z_(2^rank_bits)` rank ring, M5-E two-message schema, framed process IPC,
material binding and same-build CmpAgg serialization.
**D-UNRESOLVED:** conference text does not determine this exact transcript,
wire format or field DPF wrapper proof. The wrapper has an implementation-level
structural review only, not an independent cryptographic proof.

M5-G Fsort/FullEval, M5-H independent Theorem 4.2 cost review, a secure
ring-to-field input adapter, original-order Top-K mask output adapter, and
the common-mask optimization remain outside M5-F. No Protocol I, frozen M3,
FSS core, or `VFSS-baseline/` runtime file was modified.
