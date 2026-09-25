# M5-E Protocol III two-round Fselect candidate closeout

Implementation: `VFSS/include/moe_topk/protocol_iii_two_round.h` and
`VFSS/src/moe_topk/protocol_iii_two_round.cpp`. Test:
`VFSS/tests/moe_topk/protocol_iii_two_round_fselect_test.cpp`.
Base: `main@883da9e`; branch checkpoint before this uncommitted stage:
`a768c23`. This is an independent candidate, not author-exact and not the
final M5 process or cost closeout.

## Historical read-only design audit

The read-only branches `origin/docs/m5-protocol-iii-2round-plan` and
`origin/design/m5-protocol-iii-exact-2round` contained plans and causal gates
from `main@2a83b19`, not an implemented two-round transcript. The older
`origin/M5.0.1-paper-evidence` and `origin/m5-protocol-iii-field-contract`
were used as references, with the latter's proposed `GF(2^64)` replaced by
the current, tested M5-D field. No file was imported from these branches.

| Old design item | Audit result |
| --- | --- |
| Independent path alongside M3, two exchanges, offline dealer, no opened rank | REUSE_AS_IS as constraints |
| Party-local state, session binding, framed transport, oracle differential | REUSE_WITH_ADAPTATION to current ABIs |
| Old R1/R2 schema and causal proof | NEEDS_REDERIVATION; old draft deliberately left exact fields open |
| Ranking masks, rank masks, payload masks, DPF `alpha/beta`, Beaver material | REUSE_AS_REFERENCE_ONLY; derived again from paper and current code |
| Historical `GF(2^64)` field choice and payload packing | ALREADY_SUPERSEDED by M5-D `F_(2^127−1)` |
| Old M4 merge gate and `main@2a83b19` base | DO_NOT_REUSE; current roadmap cancelled M4 and branch has advanced |
| Old proposed author-exact naming and anticipated cost | DO_NOT_REUSE without independent proof/review |

## Function and algebra

The minimal core is one-rank Fselect. Each logical item has a project priority
key (score descending, original index ascending) and a `uint64` payload. The
M5-D field encoder packs them as `z_i=((key_i<<64)|payload_i)+1`, a nonzero
value in `H=F_(2^127−1)`. The API accepts additive field shares of `z_i`;
it does not convert old ring payload shares. The embedded field-record key
must match the comparison-key shares; this consistency is established by the
test input controller, not checked by opening secrets in production. P2 samples independent
`r_cmp[i]` in the comparison ring, `r_rank[i]` in the rank ring, and
`s_payload[i]∈H*`, generating a CmpAgg clique, field DPF at
`alpha_i=r_rank[i]` with `beta_i=s_payload[i]⁻¹`, and field multiplication
triples. Each party obtains only its own shares and DPF key.

For parties `p∈{0,1}` and logical index `i`:

- R1 sends `[key_i+r_cmp[i]]_p` and Beaver openings
  `[d_i]_p=[z_i-a_i]_p`, `[e_i]_p=[s_payload[i]-b_i]_p`.
- After R1, opening these masked values gives public masked comparison keys,
  `d_i`, `e_i`. Shared `protocol_i_cmpagg_eval_party` gives `[rank_i]_p` in
  `Z_(2^rank_bits)`. The field Beaver finish gives `[q_i]_p` for
  `q_i=z_i·s_payload[i]`, without opening `z_i` or `s_payload[i]`.
- R2 sends `[rank_i+r_rank[i]]_p` and `[q_i]_p`. Opening gives
  `m_i=rank_i+r_rank[i]` in the rank ring and `z̃_i=q_i` in `H`.
- For target rank `t`, each party locally evaluates its field DPF key at
  `x_i=m_i-t`; additive DPF shares reconstruct to `s_payload[i]⁻¹` iff
  `rank_i=t`, otherwise zero. It outputs
  `[selected]_p=Σ_i z̃_i·DPF_p(x_i)` in `H`. Exactly one logical item has
  rank `t`, so the reconstructed result is its encoded field record.

The target rank and `k` are public. `rank_bits=max(1,ceil(log2 n))` remains
the project's rank ring: `n=3→Z4`, `n=5→Z8`. These are project
C-INSTANTIATION domains, not paper-exact `Z_n`; honest ranks remain in
`[0,n)`. No padded position receives CmpAgg or routing material.

## Causal dependency DAG and messages

| Value | Earliest availability | View |
| --- | --- | --- |
| CmpAgg keys, `r_cmp`, `r_rank`, `s_payload` shares, field DPF, Beaver triples | offline | P2 full; P0/P1 local material only |
| Priority-key and encoded field-payload shares | before R1 | party-local secret |
| Masked comparison-key shares, `d/e` shares | before R1 peer input | R1 outbound |
| Public masked keys and public `d/e` | after R1 peer input | public masked |
| Additive rank shares and `q=z·s` shares | after R1 peer input | party-local secret |
| Masked-rank shares and `q` shares | before R2 peer input | R2 outbound |
| Public masked rank `m` and public masked payload `z̃` | after R2 peer input | public masked |
| DPF indicator shares and selected field share | after R2 peer input | local secret; no send |

Every party prepares the *whole* outbound message before consuming its peer
message for that round. The test invokes both `prepare_round1()` calls before
either `consume_round1()`, and both `prepare_round2()` calls before either
`consume_round2()`. One case also exchanges each prepared byte vector over
actual framed socket channels. The owning `protocol_iii_two_round_party`
entry is separately tested over two framed socketpairs for `n=3,5,8` and
returns per-round wire counters. P1 may physically send after a receive to
avoid blocking; its bytes were fixed before the receive. The state machine
has no send path after `consume_round2()`. A failed exchange enters a terminal
failed state and its material cannot be retried.

Application message header (44 canonical big-endian bytes): magic `M5TR`,
version 1, round, sender, reserved, session, fingerprint, logical_n,
padded_n, k, target_rank, comparison_bits, rank_bits, reserved. The existing
framed transport adds 48 bytes per message and checks its own sender,
receiver, phase, type, sequence, lengths, session, and fingerprint.

| Round | Body per party, logical order | Application bytes per party | Logical bits, both parties |
| --- | --- | ---: | ---: |
| R1 | `n` comparison masked-key shares (8 bytes each); `n` field `d/e` pairs (16+16 bytes each) | `44+40n` | `2n(comparison_bits+2·127)` |
| R2 | `n` masked-rank shares (8 bytes each); `n` field `q` shares (16 bytes each) | `44+24n` | `2n(rank_bits+127)` |

For the measured `n=2,k=1` socket case, R1 logical = 1152 bits, R2 logical =
512 bits, total logical = 1664 bits; R1 wire = 344 bytes, R2 wire = 280
bytes, total wire = 624 bytes across both parties. Serialized offline
component content for both parties = 2690 bytes, excluding dealer transport
framing. Logical counts exclude serialization headers and ring storage
padding. These are candidate measurements, not a Theorem 4.2 cost match.
Command: `build-vfss-debug/moe_topk_m5e_two_round_fselect_test` on
`a768c23` plus the uncommitted M5-E diff, fixed deterministic seed, one
sample; repeated benchmarking is NOT_MEASURED.

## Party views, lifecycle, and security boundary

P0/P1 each see only local priority/payload shares, local material, their own
rank and field output shares, public masked comparison keys and Beaver `d/e`
(R1), and public masked ranks and `z̃` (R2). Neither sees full
`s_payload`, scalar `s_payload⁻¹`, clear payload, raw rank, or selected index
in the secure path. P2 owns the full input-independent preprocessing view
and is not called by either online state after construction. M5-E does not
yet prove independent P2/P0/P1 process isolation; that is M5-F.

CmpAgg uCMP keys, field multiplication triples, rank masks and DPF keys are
owned by one state instance. R1 consumes comparison masks and starts the
triples before online I/O; R1 receive finishes triples and consumes uCMP;
R2 consumes rank-mask shares and evaluates field DPF locally. State phases
reject repeat calls. Malformed R1/R2 consumes the state even after partial
processing. The owning transport entry moves the whole bundle into a local
state and owns both round descriptors; a partial receive or other I/O failure
destroys both, and a moved-from bundle cannot be retried. Session, fingerprint, party, shape, widths, edge order, DPF slot,
triple slot and target rank are checked. Message decoding rejects wrong
version/round/party, metadata mismatches, wrong length, out-of-ring word,
noncanonical field element, and trailing bytes.

The field DPF wrapper reuses the native DPF tree security assumption and
applies a field correction only at the leaf. Separate party keys carry no
clear `beta` or complete `s_payload`. This is an implementation-level
structural review, **not** a formal proof or independent cryptographic audit.
The conference paper does not fix the project field/encoding/transport.

## Verification and remaining work

The new test executes 117 deterministic cases across `n=2,3,4,5,8`,
`k=1,n,floor(n/2)` where distinct, strict descending/ascending, all equal,
mixed duplicates, boundary scores, signed negative/zero/positive encodings,
and fixed-seed randomized scores. Payloads include zero, one, `UINT64_MAX`,
duplicates and random values. Every case checks reconstructed CmpAgg rank
against `stable_ranks_cmpagg` and reconstructed field Fselect record against
the same clear priority oracle. Three same-input cases (`n=2,k=1`; all-equal `n=3,k=1`; mixed-duplicate
`n=5,k=2`) directly execute the frozen M3 three-round core and compare
selected membership. The separate
M5-B/C tests continue to exercise 112 routing/full-modular cases each.

One-shot, wrong target/session/party, malformed headers, truncation,
additional bytes, noncanonical field encoding, failure replay, and a partial
R1 socket receive are tested.
The new target passes Debug and the existing ASan/UBSan build. Full Debug
CTest: 33/33 passed; old M3 and Protocol I tests passed. No production
Protocol I, M3, native FSS, or `VFSS-baseline/` file changed.

Remaining M5-F: independent executable P2/P0/P1 distribution and online
process closeout, more complete material transport binding and causal/wire
audit. Remaining M5-H: independent Theorem 4.2 logical communication review.
Secure ring-to-field input conversion and original-order XOR Top-K mask
output adapter are separate project boundaries. Fsort/FullEval and
common-mask optimization remain unimplemented.
