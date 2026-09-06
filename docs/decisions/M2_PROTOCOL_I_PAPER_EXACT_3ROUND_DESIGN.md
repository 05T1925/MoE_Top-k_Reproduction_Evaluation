# M2.16 Protocol I paper-exact 3-round design specification

Status: **D-level research specification; blocked, not implemented.**

This record investigates the missing functionality required to move the current
C-level baseline toward Agarwal Protocol I's paper-core claim. It does not
authorize an exact label, change the current runtime, or replace the validated
`m2_protocol_i_raw_score_input_modular_8round_mask_output` path.

## 1. Source boundary

The following are paper-defined observations (A):

- Agarwal CCS 2024, Section 2.4, defines a secure shuffle that outputs the usual
  secret-shared shuffled list and a public masked shuffled list
  `y = pi(x) + r`; `r` is a vector of random private masks unknown to any single
  party.
- Agarwal CCS 2024, Section 4.1, uses that public `y` as input to FSS ranking
  gates without additional communication.
- Agarwal CCS 2024, Theorem 4.1 and Table 1 report Protocol I as a 2+1
  shuffle plus compare-aggregate construction with 3 online rounds.

The following are local or project observations, not paper claims (B/C):

- The local B1-oriented reference describes public output as `pi(x)+r`, but its
  current efficient implementation boundary is not an independent proof of the
  paper functionality.
- VFSS `protocol_i_secret_shared_shuffle` returns party-local secret shares and
  counters. It has no public-list output, list binding, or material binding for
  the FSS masks `r`.
- VFSS currently forms and exchanges a masked-key vector after the forward PS
  calls. This is an independent online phase and is counted in the current
  4-round core / 8-round total.

## 2. Frozen project contract

The candidate must preserve the existing M1/M2 contract:

- P0/P1 hold raw signed Q20.12 32-bit arithmetic shares;
- `raw = (x0 + x1) mod 2^32`; the controller does not reconstruct raw or build
  priority keys;
- scores sort descending and ties sort by original index ascending;
- priority rank 0 is highest priority and ranks cover `0..n-1`;
- output is a logical-length, original-order XOR Top-K bit-mask share;
- only the test harness may reconstruct the final mask;
- P2 is input-independent offline material provider, sends packages, then exits;
- P2 does not participate in online score or rank computation.

## 3. Candidate functionality

For input records `x_i` (score plus the index-binding fields required by the
project), the proposed paper-compatible functionality would sample a hidden
permutation `pi` and fresh masks `r_i`, then return:

```text
secret shares of pi(x)
public y_i = pi(x)_i + r_i
```

The same `r_i` must be the secret parameter of the subsequent GRank/DCF gate.
The public list is therefore a masked value, not a public permutation, a public
original-index list, a selected-index list, or a controller-generated test
vector. The list length is `padded_n`, and every public element is bound to its
shuffled slot, session, phase, sequence, comparison width, and list fingerprint.

The record payload must include the score and the secret original-index binding
needed for stable rank. A public list carrying an invertible score transform or
an index encoding would violate the project leakage contract.

## 4. Roles and material

P2 would receive only public shape, ring, phase, seed-domain and protocol
configuration. P2 would produce party-specific PS material and the correlated
material needed by the FSS gate to evaluate against the same `r`. P2 must not
receive raw shares, reconstruct scores, select a permutation from data, or
remain connected after package delivery.

P0/P1 would receive local PS/GRank material and their raw input shares. The
online functionality would need to make the `y` values available to both
online parties while retaining only local secret shares of `pi(x)` and hiding
each party's local permutation. The material must be one-shot and bound to
`session`, `fingerprint`, `phase`, `sequence`, `party`, `slot`, `width`, and
`count`.

## 5. Candidate stage table

| Stage | Input/output | Messages and opening | Intended causal rounds |
| --- | --- | --- | ---: |
| O0 | P2 packages PS and GRank correlated material | P2 to P0/P1; P2 exits | offline |
| R1 | secret-shared record shares to secret-shared `pi(x)` and public `y=pi(x)+r` | the paper-compatible shuffle transcript must produce both outputs under one hidden `pi` | 2 |
| R2 | public `y` plus FSS masks `r` to additive stable-rank shares | local GRank/DCF evaluation; no extra public-list exchange | included in R1/R2 boundary as specified by the paper construction |
| R3 | shuffled rank shares to controlled shuffled `(slot, rank_P)` and local carrier shares | one approved rank-reveal message; no original mapping opened | 1 |
| A | selected carrier shares to original-order XOR mask shares | current inverse routing adapter is two causal PS rounds | +2 |

For the project candidate, the target arithmetic is `paper core = 3` and
`total = 2 raw-score adapter + 3 paper core + 2 mask adapter = 7`. This table is
a target specification only. The current graph remains 8 because its separate
masked-key exchange is still present.

## 6. Required new interface, if the blocker is resolved

No interface is added by M2.16. A future implementation would need a minimal
functionality with an output contract equivalent to:

```text
paper_shuffle_preprocess(config, party_material, same_permutation_grank_material)
paper_shuffle_online(party, input_share, one_shot_material)
  -> {secret_shuffled_share, public_masked_list, list_binding, counters}
```

The actual wire/API shape must be decided only after an independent security
review. In particular, a post-hoc exchange of `public_masked_list` is not an
implementation of this interface, and a separately sampled public permutation
does not satisfy same-permutation binding.

The interface must hard-fail on wrong role, phase, sequence, session,
fingerprint, slot, width, count, truncation, trailing bytes, duplicate or
replayed material, EOF, timeout, invalid shape, or material reuse. It must not
retry, fall back, return partial output, use an online Dealer, use file
synchronization, or expose a secure reconstruction API.

## 7. Binding and proof obligations

Before any production implementation can be considered, it must provide:

1. an algebraic proof that the public list and secret payload use exactly the
   same composed hidden permutation;
2. a proof that no single party learns the composed permutation or the other
   party's local permutation;
3. a construction showing how the same `r` is correlated with the GRank/DCF
   material without making `r` known to one party;
4. a complete online message transcript whose causal barriers sum to 3;
5. a proof that no masked-key exchange remains as an undisclosed fourth core
   round;
6. a leakage proof covering the public list, shuffled rank reveal, D1, inverse
   routing and the project's original-order mask;
7. conformance, oracle differential, and independent P2/P0/P1 E2E evidence.

The current PS API can prove only the secret-shared payload output. It has no
return value or material contract for items 1, 3, 4, or 6. Therefore extending
the C++ return struct or renaming the current masked-key frame would not close
the proof obligations.

## 8. Decision

M2.16 does not implement a paper-exact primitive. The current VFSS API and
available local evidence are insufficient to prove a public `pi(x)+r` output
with same-permutation binding and a 3-round transcript. The accurate result is
to keep the C-level 8-round label and carry this D-level specification forward
as a research blocker. M3 remains the implementation mainline after the M2
closeout merge.

An exact label may be considered only after the design, leakage audit, complete
transcript, all conformance/differential/E2E tests, and reproduction evidence
are independently reviewed and passed.
