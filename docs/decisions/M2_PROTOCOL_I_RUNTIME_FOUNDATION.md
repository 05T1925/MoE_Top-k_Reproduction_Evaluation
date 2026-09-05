# M2.7 CmpAgg process foundation

Status: **implemented and verified as a project extension (C)**. This record does
not claim a complete Agarwal Protocol I implementation.

Implementation label: `m2_priority_cmpagg_three_process_e2e`.
The tested code revision is `119e527`.

## Stage contract

| Stage | Roles and inputs | Preprocessing/messages | Opened values | Output and allowed leakage |
| --- | --- | --- | --- | --- |
| O0 offline | P2 receives public `n`, `K`, comparison width and public comparison graph; it does not receive scores or priority keys. | P2 samples full node masks, additive mask shares, and one fresh VFSS DCF/uCMP material per canonical edge `i<j`; it sends a separate package to P0 and P1, then exits. | None. | Each party receives only its own mask share and party-specific edge material. Package bytes are measured separately from online communication. |
| R1 online input | TEST_ONLY controller holds clear scores and the frozen oracle; P0/P1 receive only their additive priority-key shares. | Controller sends separately bound `session`, fingerprint, `n`, `K`, comparison width and key-share vectors only after `waitpid(P2)` reports normal exit. | No score or priority key is opened by P0/P1. | TEST_ONLY input channels are excluded from online protocol counters. |
| R2 masked-key exchange | P0/P1 hold key shares plus their node-mask shares. | Each party locally forms `key_share + mask_share mod 2^bits`; P0 sends one framed vector and P1 receives then sends one framed vector. | The reconstructed public masked-key vector is the sole online opened value in this foundation. | P0/P1 use party-only material to produce additive rank shares. The frame is one causal online exchange. |
| T validation | TEST_ONLY controller receives one result from each party. | Rank shares are returned over separate TEST_ONLY channels. | Rank, stable oracle data and Top-K mask are reconstructed only by the controller. | Validation evidence only; it is not a secure runtime output contract. |

The project uCMP adapter uses two independent DCF evaluations to evaluate strict
priority-key order. It is a VFSS two-evaluation adapter and must not be described as
the paper's complete Protocol I shuffle or as a paper-level secure implementation.
The production CmpAgg entry point accepts only party-specific uCMP material; the paired
`ProtocolIUcmpMaterial` type remains confined to P2 generation and primitive/test setup.

## Package and transport invariants

`ProtocolIPartyPackage` is move-only at the edge-material level. `M2PK` version 1
serializes fixed-width big-endian fields. Every edge record carries explicit
`left` and `right` identity, followed by its material length and payload. Serialization
and deserialization reject:

- zero/invalid dimensions, uint32 overflow, multiplication overflow, oversize package
  or edge allocation;
- node mask shares outside the comparison ring;
- a non-canonical, missing, duplicate, repeated or out-of-order edge;
- party/comparison-width mismatch, material truncation, and trailing bytes.

The package implementation caps the serialized package at 64 MiB and the edge list
at 1,000,000 entries before allocating. `n(n-1)/2` is computed after widening and
checked before conversion or allocation.

The P0↔P1 framed channel uses a 48-byte versioned wire header: fixed-width big-endian
magic/version, comparison width, roles, phase/type, `n`, `K`, payload length, session,
fingerprint and sequence. The entire header-plus-payload has one absolute deadline;
partial I/O and `EINTR` preserve that deadline. `POLLERR`, `POLLHUP`, `POLLNVAL`, EOF,
timeouts, wrong headers, role/session/sequence mismatches and oversize payloads are
hard failures. A rejected header does not advance the receive sequence. Counters count
actual header and payload bytes. An accepted receive sequence advances only after the
entire payload is received successfully; a truncated or timed-out payload is not accepted.

No file polling, fixed sleep, online Dealer, old FSS ABI, secure shuffle, inverse
routing, final mask adapter or secure-path rank/index reconstruction is used.

## Verification boundary

The independent-process test covers ten cases: `n=1,2,5,7,11`, `K=1`, `K=n`, and
`K=2` for `n=11`; non-powers-of-two, random values, repeated values, all-equal
values, `INT32_MIN` and `INT32_MAX`, and ten deterministic input/P2 seed pairs.
Each case checks the rank oracle, stable ties, rank permutation, Top-K mask, canonical
edge count, package material consumption, P2-before-online barrier and normal exit of
P2/P0/P1. Negative checks cover `n=0`, `K=0`, `K>n`, wrong party/width, missing,
duplicate and unordered edges, wrong material count, truncation, trailing bytes and
child non-zero exit propagation.

The transport conformance target separately covers controlled one-system-call chunks of
`1`, `2`, `3`, and `7` bytes; header-plus-payload counters; bad magic, version and reserved
bytes; wrong session, fingerprint, role and sequence; oversize length; truncated header and
payload; payload absolute-deadline timeout; EOF; and rejected-header sequence preservation.

The completed foundation exposes no final secure Top-K bit-mask. Secure shuffle,
reverse routing, final mask generation, full Protocol I round accounting, PRG metrics,
WAN/network performance and the paper's exact claim remain `NOT_MEASURED` or outside
this milestone. The full M2 design gate remains distinct from this M2.7 foundation.
