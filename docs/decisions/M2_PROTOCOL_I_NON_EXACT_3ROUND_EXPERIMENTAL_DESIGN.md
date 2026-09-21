# M2 Protocol I NON-EXACT three-round experimental design

## Status, identity, and evidence boundary

**DESIGN ONLY — NON-EXACT.** There is no implementation and no measurement in
this record. Strict message/material/party-view/round-exact M2 G1 remains
**BLOCKED**; official G2, G3, and M5 runtime remain blocked. The unchanged
engineering baseline is
`m2_protocol_i_raw_score_input_modular_8round_mask_output`, with 2 raw-adapter
+ 4 core + 2 reverse-mask rounds.

Candidate identity: **Agarwal Protocol I functionality-aligned 3-round
experimental candidate (NON-EXACT), using an independent Dealer-DPF shuffle construction**. Its concrete C-INSTANTIATION remediation contract is [the 2026-09-20 contract](M2_PROTOCOL_I_DEALER_DPF_BOUND_PUBLIC_MASK_SHUFFLE_REMEDIATION_CONTRACT_2026-09-20.md), not author behavior.
Machine label:
`agarwal_protocol_i_functionality_aligned_3round_NON_EXACT_experimental`.

Its normative byte-level entropy derivation is [M2CBKDF1](M2_PROTOCOL_I_DEALER_DPF_ENTROPY_DERIVATION_CONTRACT_2026-09-21.md), also a C-INSTANTIATION rather than author behavior.

This document preserves four canonical, non-interchangeable evidence classes.
Explanatory sublabels map back to those classes: **A-DIRECT** and
**A-DERIVED** are A, **C-INSTANTIATION** is C, and **D-UNRESOLVED** is D.
**SUPPORTING_FSS** is supporting literature outside A/B/C/D.

| Class | Boundary |
| --- | --- |
| A | Target-paper evidence. **A-DIRECT:** P0/P1 are online in the (2+1) setting; P2 supplies correlated randomness offline and is silent online; preprocessing is input-independent; shuffle outputs secret-shared shuffled data and public `y=π(x)+r` to online parties; `r` is private random masks unknown to a single online party; the public array is directly usable as FSS input without added communication; FSS/GRank uses `r` as secret parameter; Fsort preserves key/payload correspondence; stable ranks are revealed after shuffle for local routing; Fsort has three online rounds; Theorem 4.1 gives aggregate `4N(ell'+p)+2N ceil(log2 N)` online bits. **A-DERIVED (not verbatim transcript facts):** two shuffle rounds yield public `y` plus secret shuffled shares, then local GRank, one shuffled-rank opening, and local routing; no extra GRank masked-input opening follows shuffle; key/payload follow one hidden permutation; `r` in `y` is the slot-mask vector encoded by corresponding GRank/FSS material; an added post-shuffle `y` round makes the core at least four rounds; shuffled ranks remain unlinkable to original positions while the hidden permutation remains secret. |
| B | Observed local-reference/current-code behavior only; never a paper conclusion. |
| C | Project extensions, including **C-INSTANTIATION**: concrete messages/material layout, Q20.12 adaptation, composite priority key, padding, original-index payload, inverse routing, XOR mask, framing, counters, identifiers, replay protection, and the RANK_OPEN frame/layout/validation/logging/failure handling. |
| D | **D-UNRESOLVED:** author-exact transcript/material/party views, exact wire layout, author-specific π/r representation/ownership, author-specific joint material construction, complete author simulation views, and Theorem 4.1's per-round shuffle split. **PRE-REMEDIATION STATUS / HISTORICAL T2 STATUS:** this row formerly also listed the concrete two-round backend and its project security review as unresolved. For Dealer-DPF Candidate B only, that project-level issue is superseded by the later C-INSTANTIATION remediation contract; this does not resolve author-exact evidence. |

**SUPPORTING_FSS (supporting literature, not A/B/C/D):** SIGMA / standard FSS
preprocessing supports generic input-independent mask generation, trusted-dealer
preprocessing, separate P0/P1 FSS keys/material, and online local evaluation on
masked inputs. It does not establish Agarwal-specific shuffle messages, π/r
ownership, or material layout.

## Roles and offline phase

P2 receives only public configuration, atomically generates correlated packages,
sends them offline, receives readiness acknowledgements, erases ephemeral master
secrets, closes all descriptors, and exits before score shares are released. P2
never receives inputs, ranks, selected indices, or outputs and has no online
channel. There is no input-dependent preprocessing.

P0 and P1 receive separate bound packages, hold only their local input/output
shares, and execute the online protocol. A **TEST_ONLY** controller may know
clear inputs and preprocessing witnesses and reconstruct final results only after
the secure processes finish.

## Parameters and one-shot preprocessing

Let `N` be the padded element count, `b = ceil(log2 N)`, `w = 32+b` the
composite-key value bits, `ell' = 33+b` the comparison-ring bits (the current
safe adapter boundary), `p = b` the default original-index payload bits, and
`E = N(N-1)/2`.

Each one-shot package is bound to session, party, shape, width, and stage, and
contains the following.

1. A public session manifest: protocol/version, session, fingerprint,
   material-set ID, logical/padded sizes, K, widths, roles, phases, and counts.
2. **PRE-REMEDIATION STATUS / HISTORICAL T2 STATUS:** exact `π/r` ownership and
   representation were D-UNRESOLVED at the original T2 design point. For the
   later Dealer-DPF Candidate B C-INSTANTIATION, the project-level ownership,
   atomic construction, and security contract are superseded by
   [the remediation contract](M2_PROTOCOL_I_DEALER_DPF_BOUND_PUBLIC_MASK_SHUFFLE_REMEDIATION_CONTRACT_2026-09-20.md): backend VALID, remediation COMPLETE,
   implementation NOT PRESENT, tests NOT RUN, measurements NOT_MEASURED. This
   does not assert author behavior or resolve author-exact ownership evidence.
3. Per-party `BoundPublicMaskShuffle` R1/R2 material for complete key/payload
   records.
4. Per-party public-list/GRank linkage tokens ensuring the R2 list uses the
   exact `r` used by GRank material.
5. DCF/FSS party keys for every unordered comparison edge: `E = N(N-1)/2` party
   keys per online party and `2E` party keys total across P0/P1, unless a future
   concrete C-level backend explicitly requires additional directional material.
   Such material is a project extension, not an attribution to Theorem 4.1.
6. Per-party rank-aggregation masks/tokens.
7. Existing raw-score carry/sign materials, adapter-only.
8. Existing inverse-routing materials, adapter-only.
9. Non-secret random binding IDs and ordered material IDs.
10. A local consumption ledger with terminal `unused → in_progress → consumed`
    semantics; any success or failure makes the attempted slice unusable.

Binding IDs prevent mix-ups but are not malicious-security proofs. Never hash
low-entropy secret permutations or masks into public identifiers.

## BoundPublicMaskShuffle: required contract and blocker

**PRE-REMEDIATION STATUS / HISTORICAL T2 STATUS:** no current VFSS primitive
implemented this contract, and runtime was blocked pending a concrete backend and
security review. For the later Dealer-DPF Candidate B C-INSTANTIATION, that
project-level backend/security-contract issue is superseded by
[the remediation contract](M2_PROTOCOL_I_DEALER_DPF_BOUND_PUBLIC_MASK_SHUFFLE_REMEDIATION_CONTRACT_2026-09-20.md): VALID design and COMPLETE remediation,
with NOT PRESENT implementation, NOT RUN testing, and NOT_MEASURED results. This
does not resolve author-exact transcript/material/party-view evidence or strict
G1, official G2/G3, and M5 runtime BLOCKED status.

`BoundPublicMaskShuffle` must shuffle the entire key/payload record under one
hidden permutation; output each party's secret shares of `π(key,payload)`; and
output the identical public list `y=π(key)+r` to P0 and P1. It must use the exact
same `π` for key and payload, bind the exact same `r` to subsequent GRank/FSS
material, reveal neither full `r` nor any hidden permutation to either online
party, and complete in two causal online rounds. Every R2 outbound frame must be
fully determined before receipt of the peer's R2 frame.

The selected implementation must cryptographically enforce the same hidden
permutation for key/payload and the same `r` relation between public `y` and
GRank material. A single atomic factory with shared internal `π/r` handles is
one possible C-INSTANTIATION, not an Agarwal requirement. Metadata IDs alone do
not prove the relation.

### Canonical GRank/DCF offset-orientation invariant

For every comparison edge use the canonical orientation `e=(i,j)` with `i<j`.
Let `y_i=key_i+r_i` and `y_j=key_j+r_j`. The uCMP/DCF material for `e` must be
generated for the ordered mask pair `(r_i,r_j)` and evaluated on the ordered
public inputs `(y_i,y_j)`. It must preserve:

```
(y_i - r_i) - (y_j - r_j)
= (y_i - y_j) - (r_i - r_j)
= key_i - key_j
```

Thus the GRank/DCF offset convention preserves the exact `r_i-r_j`
sign/orientation for edge `(i,j)`. A package using `r_j-r_i`, or evaluating the
same key on `(y_j,y_i)`, is not equivalent and must fail correctness/validation.
The comparison meaning remains `uCMP(A_i,A_j) = 1` iff `A_i < A_j`,
subject to the comparison-domain promise.

**A-DIRECT (canonical A):** the target paper's native uCMP/GenCmpAgg semantics
and ordering facts; these are not silently rewritten as repository adapter
semantics. **A-DERIVED (canonical A):** the equality above connecting public
`y=key+r` to the required edge offset. The current repository strict-less adapter
`L_ij=[A_i<A_j]` and its CmpAgg integration are **C-INSTANTIATION/current
implementation semantics**, not an A-level transcription of the paper's native
uCMP convention. Concrete author key serialization/layout remains
**D-UNRESOLVED**. Edge IDs/orientation metadata enforcing the Candidate-B
relation are C-INSTANTIATION; no internal DCF key encoding is specified here.

## Abstract A-DERIVED causal transcript

The following A-DERIVED skeleton is an inference from the A-DIRECT
functionality/round statements, not an author transcript:

1. Two secure-shuffle rounds.
2. At completion, secret shuffled key/payload shares and public
   `y=π(key)+r` exist.
3. GRank evaluates locally with no new round.
4. One shuffled-rank opening round.
5. Routing is local.

The shuffled-rank disclosure is A-DIRECT; only its frame, field layout, binding
metadata, validation, logging, and failure behavior are project choices.

## Conditional C-INSTANTIATION: independent three-round framed transcript

This is a conditional C-INSTANTIATION, explicitly not the author's transcript.
All fields, directions, framing, correction terms, identifiers, and validation
metadata are project choices.

| Round | Concurrent traffic and local work |
| --- | --- |
| R1 | P0→P1 and P1→P0 send framed `SHUFFLE_R1`: binding metadata and `N` opaque masked key/payload record terms. Each frame depends only on local input and unused R1 material. Nothing is opened. Receipt produces intermediate hidden-permutation state. |
| R2 | P0→P1 and P1→P0 send framed `SHUFFLE_R2`: final record-correction terms, `N` public-list opening contributions, and permutation/mask binding IDs. Outbound data may depend on R1, never peer R2. After receipt, both hold secret shuffled record shares and identical public `y`; each locally performs `E` FSS evaluations and produces additive rank shares. |
| R3 | P0→P1 and P1→P0 send framed `RANK_OPEN`: `N` rank shares of `b` logical bits and binding metadata. They reconstruct only shuffled-domain `(slot,priority_rank)` labels, validate a permutation of `0..N-1`, and locally scatter secret-shared records into rank order. |

If a backend needs a peer-R2-dependent response or a post-shuffle masked-list
exchange, the core is at least four rounds. Rank shares and ranks linked to
original items remain secret. The underlying shuffled-rank reveal is A-DIRECT;
the explicit C-level shuffled-domain D1 label representation is a project extension.

## Ranking and padding semantics

For raw two's-complement word `u` and original index `i`:

```
ordered_signed(u) = u XOR 0x80000000
descending_code(u) = 0xffffffff - ordered_signed(u)
priority_key(u,i) = (descending_code(u) << b) | i
project_priority_rank = ascending_rank(priority_key)
```

This places larger scores first, uses ascending original index on ties, and gives
priority rank 0 highest priority. Do not use
`priority_rank=N-1-paper_rank(raw_score)`: it reverses equal-item order. For
`[7,7,7,7], K=2`, the required mask is `[1,1,0,0]`; naive reversal selects
indices 2 and 3.

Padding is C-level: `N=max(2,next_power_of_two(logical_n))`; dummies have
`INT32_MIN` and indices `logical_n..N-1`; real minimum-valued items precede
dummies; and `K<=logical_n`.

## Core boundary and adapters

The core input is padded composite-key shares and corresponding payload shares;
the default payload is a secret-shared original index. Core work is bound
shuffle/public list → local all-pairs GRank → shuffled-rank opening → local rank
routing. The core output is secret shares of rank-ordered keys/payloads: an
Fsort-shaped paper-native boundary.

Separate adapters are raw Q20.12 share conversion; signed direction/tie-key
construction; padding/dummy handling; original-index payload encoding; rank `<K`
carrier creation; inverse routing; required share conversion; and original-order
XOR Top-K mask. If current adapters are reused, derive rather than hard-code the
conditional total `2 raw + 3 core + 2 reverse = 7` rounds.

## Future interfaces (proposal only)

Without changing ABI, propose:

- `ProtocolINonExactCoreConfig`: session, fingerprint, material-set ID,
  logical/padded N, K, key/comparison/payload/rank widths, party, timeout.
- `ProtocolINonExactPreprocessingPackage`: config, binding metadata,
  bound-shuffle material, GRank material, aggregation material, one-shot state.
- `ProtocolINonExactOnlineInput`: key-share and payload-share vectors.
- `ProtocolINonExactBindingMetadata`: exact label, session, fingerprint,
  material set, permutation/mask binding IDs, N, widths.
- `ProtocolINonExactSecretShuffledShares`.
- `ProtocolINonExactPublicMaskedList`.
- `ProtocolINonExactShuffleOutput`.
- `ProtocolINonExactStageTraffic`: stage, causal round, payload sent/received
  bits, framing sent/received bytes.
- `ProtocolINonExactCoreOutput`: sorted key/payload shares and trace.
- `ProtocolINonExactAdapterOutput`: original-order XOR mask share and trace.

Reuse `ProtocolIInputLayout`, raw-score semantics, `ProtocolIFramedChannel`,
the frozen oracle, and top-level `MetricsRecord` where safe. Do not reuse
`ProtocolIShufflePartyMaterial` as the new contract: it lacks public-list and
GRank-mask binding.

## Communication accounting

Record per party and stage: payload sent/received bits, serialization bytes,
framing/control bytes, application bytes, causal round, and diagnostic frame
count. Separately record offline material generation, serialization, and
distribution. Record logical `n`, padded `N`, K, `w`, `ell'`, `p`, `b`, `E`,
per-party DCF/FSS evaluations `E`, total evaluations across P0/P1 `2E`, actual
PRG calls, and per-primitive offline material.

Total is the sum of party sends only; received counts are cross-checks.
**PRE-REMEDIATION STATUS / HISTORICAL T2 STATUS:** the original T2 discussion
used Agarwal Theorem 4.1, `4N(ell'+p)+2Nb`, as target-paper A evidence. For the
later Dealer-DPF Candidate B C-INSTANTIATION, frozen logical payload is R1
`2N(ell'+p)`, R2 `2Nell'`, RANK_OPEN `2Nb`, total `4Nell'+2Np+2Nb`. The `2Np`
difference is a resolved structural difference of the independent construction,
not missing accounting. Future Candidate-B measurements first check that formula,
then may compare separately with Theorem 4.1 as paper-vs-project structure;
Candidate B is not paper-native communication. Logical `n` remains a labeled
reference. All T2 empirical fields remain NOT_MEASURED; NON-EXACT measurements
cannot satisfy official G2.

## Leakage, failures, and safety

The secure runtime must not reconstruct score, key, comparison, original-rank,
selected-index, or final-mask data; disclose full `r`, permutations, peer
package, or peer input share; or break key/payload and list/GRank binding
invariants. Scope is single-static-semi-honest, non-colluding only.

Validate exactly session, fingerprint, role, sender/receiver, phase, type,
sequence, counts, widths, material IDs, binding IDs, and lengths. Reject
truncation, trailing bytes, replay, duplicate/out-of-order phases, wrong package,
timeout, EOF, rank non-permutation, peer failure, and reuse. Fail closed: no
retry using the same material, fallback, partial output, fixed sleep, file
polling, or online material request.

## Future T3 tests

1. Bound-shuffle conformance: reconstructed shares equal one permutation of
   complete records; public list equals shuffled key plus TEST_ONLY witness `r`;
   both parties get the same list; individual packages expose neither full `π`
   nor `r`.
2. Same-permutation key/payload binding, including rejection of swapped
   payload/permutation/session material.
3. Masked-list/GRank consistency and rejection of altered lists, slots,
   bindings, or material from another session.
4. Priority mapping: largest-first, ascending-index ties, duplicates at K, all
   equal, and explicit failure of `N-1-paper_rank`.
5. Three-round trace: no hidden fourth masked-list exchange and R2 dependency
   rule enforced.
6. Frozen-oracle differential tests for random, duplicate, all-equal, negative
   Q20.12, `INT32_MIN/MAX`, mixed boundaries, `K=1`, `K=n`, and non-power-of-two
   logical n.
7. Full adapter differential against the frozen oracle and existing C-level
   baseline; verify length, binary values, exact cardinality K, and original
   order.
8. Malformed material/frame matrix: truncation, trailing bytes, wrong
   party/session/fingerprint/material set/phase/sequence/N/K/width/binding,
   replay, one-shot reuse after success or failure, EOF, timeout, and peer exit.
9. Independent fork/exec P2/P0/P1 E2E proving P2 exits before input release and
   has no online descriptor; only TEST_ONLY controller reconstructs.
10. Counter self-tests reconciling sends/receives/stages/totals and distinguishing
    edges, FSS calls, PRG calls, payload, and framing.
11. Candidate-B uCMP orientation differential: for canonical `e=(i,j), i<j`,
    form `alpha=r_A[i]-r_A[j]`, evaluate `(y_i,y_j)`, and reconstruct
    `L_ij=[A_i<A_j]`; verify CmpAgg `rank_i+=1-L_ij; rank_j+=L_ij`. Include
    positive strict-less and strict-not-less cases. Negative oracle cases must
    fail for reversed `r_A[j]-r_A[i]`, swapped `(y_j,y_i)`, or replacement of
    strict-less with obsolete strict-greater; no inversion or argument swap.
12. INT32_MIN cross-boundary padding: use non-power-of-two `logical_n`, every
    real score and dummy score `INT32_MIN`, real indices `0..logical_n-1`, and
    dummy indices `logical_n..N-1`; run `K=1` and `K=logical_n`. Verify every
    real record ranks before every dummy, no dummy enters the selected prefix,
    and the reconstructed original-order mask has exactly K real selections.

## Open risks and T2 acceptance

**PRE-REMEDIATION STATUS / HISTORICAL T2 STATUS:** at the original T2 design
point, the missing concrete two-round backend, its project π/r ownership, joint
material construction, and security review were runtime blockers. For later
Dealer-DPF Candidate B, those project-level issues are superseded/resolved by
[the remediation contract](M2_PROTOCOL_I_DEALER_DPF_BOUND_PUBLIC_MASK_SHUFFLE_REMEDIATION_CONTRACT_2026-09-20.md): VALID design and COMPLETE remediation,
but NOT PRESENT implementation, NOT RUN testing, and NOT_MEASURED results.
This does not resolve author-exact transcript/material layout/party views,
strict G1, official G2/G3, or M5 runtime; those remain D-UNRESOLVED/BLOCKED.
Other T2 risks retain their historical status.

**HISTORICAL T2 STATUS:** when this design was originally written, acceptance
of this NON-EXACT direction was conditioned on resolving a concrete two-round
BoundPublicMaskShuffle backend. **CURRENT CANDIDATE-B STATUS:** for the
Dealer-DPF C-INSTANTIATION, that project-level condition is superseded by
[the remediation contract](M2_PROTOCOL_I_DEALER_DPF_BOUND_PUBLIC_MASK_SHUFFLE_REMEDIATION_CONTRACT_2026-09-20.md), which records VALID backend design and COMPLETE
security-contract remediation. This does not mean implementation exists or is
authorized: it remains NOT PRESENT and authorization remains NO pending the
separate post-merge implementation-authorization gate. It also does not resolve
author-exact evidence: strict G1, official G2, official G3, and M5 runtime
remain BLOCKED. Historical T2 acceptance otherwise required this exact NON-EXACT
identity, A/B/C/D labels, the contracts above, tie-reversal rejection,
paper-native versus adapter separation, NOT_MEASURED metrics, unchanged gates,
and no implementation or benchmark claim.
