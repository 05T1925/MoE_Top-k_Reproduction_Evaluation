# M5-G Protocol III Fsort / FullEval closeout

Status: M5-G Fsort extension complete on
`feat/m5-protocol-iii-two-round`, HEAD `77d4938` plus uncommitted M5-G diff.
The frozen M5-E Fselect and M5-F P2/P0/P1 process baseline remain available.
This is a C-INSTANTIATION, not an author-exact reproduction or M5-H cost
verdict. Decision: [M5-G Fsort / FullEval](../decisions/M5_PROTOCOL_III_FSORT_FULLEVAL_DECISION_2026-09-25.md).

## Source mapping and contract

Figure 1 of the supplied CCS 2024 paper defines Fsort as sorted keys with
associated payloads, both additively shared. §4.2 states that the same `n`
per-item DPF keys can locally yield an `n×n` permutation matrix and that
FullEval replaces single Eval without extra offline or online communication.
Theorem 4.2 attributes the sorting extension's added work to
`DPF.FullEval[Z_n,H]`. The concrete field, packing and transcript are project
choices; the conference paper refers detailed implementation to a full
version not present in the repository.

Input: `padded_n` additive shares of priority keys plus `logical_n` already
field-shared, nonzero encoded `(priority key,uint64 payload)` records.
Consistency of these two secret representations is a caller precondition, not
a secure proof or ring-to-field conversion. Output: `logical_n` additive
`F_(2^127−1)` shares, in project rank order (score DESC, original index ASC,
rank 0 highest priority). Each field result unpacks to the key and payload of
the corresponding original record. The canonical Fsort bound configuration
sets `k=logical_n,target_rank=0`; target is unused in the local Fsort finalizer.

## FullEval derivation and padded slots

The native DPF's `evalAll` returns `Z_(2^64)` shares; its `rightShift` places
native input `x` at output index `(x+rightShift) mod 2^bits`. A small-domain
experiment at bits 1, 2, 3 confirmed both local share ordering and ring
reconstruction against `evalDPF_Payload`. Those ring shares cannot be
reinterpreted as field shares. The new Protocol-III-only field FullEval walks
the same native seed/correction tree, derives the M5-D field leaf, and returns
field shares indexed by input `x=0,...,2^rank_bits−1`. It allocates at most
`2^20` entries and validates party, session, fingerprint, slot and domain.
It neither changes `VFSS/ext/FSS/` nor exposes alpha or beta.

For item `i`, with public masked rank `m_i`, public masked encoded record
`z̃_i=z_i s_i`, and DPF at `alpha_i=r_rank_i`, `beta_i=s_i⁻¹`, target rank `t`
uses `x=(m_i−t) mod 2^rank_bits`. Its two FullEval shares reconstruct to
`beta_i` iff `rank_i=t`, otherwise zero. Each party computes locally:

```
out_share[t] = Σ_i z̃_i · FullEval_party_i[(m_i−t) mod 2^rank_bits]
```

The same one-shot `n` DPF keys, rank masks, CmpAgg edge materials and field
multiplication materials serve the entire vector. CmpAgg runs once; there is
no per-target online call. Only `logical_n` slots are returned. In honest
runs, `n=3` padded slot 3 and `n=5` padded slots 5–7 reconstruct to zero;
these are tested separately. The rank storage domain remains `Z4/Z8`, a
C-INSTANTIATION rather than paper-exact `Z_n`.

## Causal process path and measured communication

M5-G adds `consume_round2_sort()` to the existing one-shot party state. The
R1 and R2 preparation, messages, framed transport, offline bundle and
P2 lifecycle are unchanged. P0/P1 each freeze complete R1 outbound before
peer R1 receive and R2 outbound before peer R2 receive. After R2 receive,
FullEval, field multiplication and accumulation are entirely local. Each party
sends its completed `n`-field-share vector to the **test controller** on a
separate `M5SR` result channel; this carries no value back into protocol
computation. It is not a third protocol round. P2 exits before online input
release. No raw rank, raw payload, full multiplicative mask or scalar inverse
is sent.

The Fsort process test uses 14 independent fork+exec P2/P0/P1 executions over
`n=2,3,5,8`; strict scores, all equal, mixed duplicates, boundary and
fixed-seed random scores; payload zero, one, `UINT64_MAX`, duplicates and
random values. It checks all output slots against the stable clear oracle,
permutation completeness, both parties' causal events and measured frames.
Three process faults (bad offline field tag, truncated R1 and truncated R2)
fail closed. The result vector decoder also rejects wrong tag/shape,
truncation, extra bytes and noncanonical field values. Existing M5-F Fselect
process tests remain unchanged in behavior and continue to run.

Measured sample: `n=5,k=5,target_rank=0` (canonical Fsort header), fixed seed
from the process test, Linux WSL2 x86_64, GCC 11.4.0, Debug build, HEAD
`77d4938` plus uncommitted M5-G diff. Command:
`build-vfss-debug/moe_topk_m5f_two_round_process_e2e_test --sort-controller`;
one direct invocation for the sample below.

| Category | Result |
| --- | ---: |
| R1 logical, both parties | 2,900 bits |
| R2 logical, both parties | 1,300 bits |
| Total logical online | 4,200 bits |
| R1 framed wire, each party | 292 bytes |
| R2 framed wire, each party | 212 bytes |
| Total online framed wire, both parties | 1,008 bytes |
| Serialized offline bundle, each party | 10,766 bytes |
| Serialized offline bundles, both parties | 21,532 bytes |

The two 8-byte outer dealer length prefixes are excluded from bundle sizes.
Controller result collection is excluded from online protocol bytes. The
logical count excludes application metadata, storage padding and transport
frames. Fsort adds no online protocol bytes relative to Fselect for the same
shape. These are communication counts, not latency benchmarks or a Theorem 4.2
cost match.

By loop structure, Fsort calls field FullEval once per input item per party
(`2n` total), traversing `2^rank_bits` leaves per call. The local routing
aggregation executes `n²` field multiplications and `n²` accumulations per
party; additional field operations within FullEval and masked payload opening
are excluded from those counts. A comprehensive runtime field-operation
counter is `NOT_MEASURED`.

## Conformance and evidence boundary

The field FullEval test checks per-party share equality with single Eval at
every `x`, field reconstruction at alpha and zero elsewhere, beta zero/one/
`p−1`/random, widths 1/2/3/5/8, and rejected party/session/fingerprint/slot/
domain, truncated/extra key, noncanonical correction and oversized domain.
The in-memory Fsort differential runs 40 deterministic cases across
`n=2,3,4,5,8`: descending, ascending, all equal, mixed duplicates, signed
boundaries and randomized scores. It checks the actual shared CmpAgg rank,
all sorted key/payload records, uniqueness, padded slots, success replay and
malformed-R2 failure replay. For selected cases, targets 0/middle/last match
fresh frozen M5-E Fselect runs. The first `k` members match a same-input
execution of the frozen M3 secure core, not merely a clear mask formula.

**PAPER_DIRECT:** Figure 1 Fsort keys/payloads; §4.2 same DPFs yield a local
permutation matrix; Theorem 4.2 replaces Eval by FullEval for sorting.
**PAPER_DERIVED:** `x=m_i−t` maps native DPF input to project rank slot `t`,
and multiplying by public `z̃_i` then summing recovers the record share.
**C-INSTANTIATION:** field, key/payload packing, rank ring, native-tree field
leaf wrapper, bounded domain, target-zero Fsort header, same-build bundle,
framed IPC and result vector encoding.
**D-UNRESOLVED:** author-exact transcript, field DPF formal security proof,
conference full-version details and independent Theorem 4.2 cost match.
Secure ring-to-field input conversion, input consistency proof, original-order
Top-K mask output adapter and common-mask optimization remain deferred.

## Final verification (2026-09-25)

- `ctest --test-dir build-vfss-debug --output-on-failure`: 38/38 PASS,
  including frozen M3, M5-B/C/D/E/F and Protocol I regressions.
- `env ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir /tmp/m5d-vfss-sanitized --output-on-failure -R 'moe_topk_m5g_'`: 3/3 PASS.
- `git diff --check`: PASS. `VFSS-baseline/` and `VFSS/ext/FSS/` have no changes.

These runs establish candidate functional and causal conformance. The field DPF
wrapper has implementation-level structural review; no formal security proof
or author-exact cost claim is made.
