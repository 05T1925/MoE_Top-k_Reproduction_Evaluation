# M2 Protocol I current evidence gate

Date: 2026-09-19

Status: **canonical current M2 evidence gate.** This record supersedes current
gate references in active planning and handoff documents; it does not rewrite
historical implementation, test, performance, or decision records.

## Route update (2026-09-21)

Dealer-DPF Candidate B is **STOPPED / SUPERSEDED**. The active design is
[M2_PROTOCOL_I_CHASE_SECRET_SHARED_SHUFFLE_REDESIGN_2026-09-21.md](M2_PROTOCOL_I_CHASE_SECRET_SHARED_SHUFFLE_REDESIGN_2026-09-21.md).
It authorizes only an accurately labeled Chase-based C-INSTANTIATION and does
not unblock strict author-exact G1.

## Decision

| Identity or gate | Current status | Basis |
| --- | --- | --- |
| Protocol I message/material/party-view/round-exact | **BLOCKED** | Required target-paper transcript and concrete material details are unavailable. |
| Protocol I paper-aligned experimental reproduction | **READY only as NON-EXACT** | The conference paper supports a high-level functionality and cost target, but not the missing exact transcript/material claim. |
| M2 G2 communication verification | **BLOCKED** | Strict M2 G1 has not passed. |
| M2 G3 interface/evidence handoff | **BLOCKED** | Strict M2 G1 and G2 have not passed. |
| M5 runtime implementation | **BLOCKED** | The required M2 G3 handoff has not passed. |
| Chase R0/R1--R5 migration | **READY only as NON-EXACT** | Chase supplies the two-party base stack; P2 compilation is a separate C-INSTANTIATION. |
| P2-full-`r` four-round adapter | **FUNCTIONAL C-BASELINE ONLY** | A separately corruptible P2 knows full `r`; strict G1 remains blocked. |
| Dealer-DPF Candidate B | **STOPPED / SUPERSEDED** | It is not the Chase route cited by Agarwal §2.4. |

Strict M2 G1 requires a target-paper-supported, auditable Protocol I
transcript that is exact at the message, material, party-view, and causal-round
levels. No available evidence closes that gate. A non-exact experimental
reproduction must retain a distinct identity and must not be used to pass
strict M2 G1, G2, or G3.

## Evidence taxonomy and non-inference rule

| Class | Meaning | Permitted use here |
| --- | --- | --- |
| A | Target-paper evidence | Only direct Agarwal conference-paper definitions and statements. |
| B | Local-reference behavior | Observed behavior only; never a target-paper conclusion. |
| C | Project extension | Q20.12 input, stable priority mapping, adapters, original-order mask, metrics, and engineering contracts. |
| D | Unverified | Missing transcript, concrete material layout/distribution/consumption, party views, and exact causal message dependencies. |

Supporting literature is separate from A--D. Chase §3--§6.3 supports the base
two-party OPV, Share Translation, Beneš, Permute+Share and SecretSharedShuffle.
CHASE-DIRECT has P0/P1 separately choose and retain `pi0/pi1`; P2 is absent. A
project compiler in which P2 learns both permutations is C-INSTANTIATION only.
Agarwal's citation of Chase is target-paper evidence, but the omitted concrete
adaptation remains unavailable.

Agarwal §2.4 says full `r` is unknown to any single party. In the `(2+1)`
single-corruption model this includes P2. P2 sampling full `r` and later
distributing shares is therefore only a functional C-baseline. The more
conservative independent `r0/r1` candidate is blocked on distributed/blind
generation of r-bound GRank/FSS keys.

The conservative composition derives four causal rounds: two sequential
Permute+Share rounds, one public-mask opening, and one rank opening. Theorem
4.1 three-round feasibility remains **BLOCKED** pending authoritative evidence
or a strict fused construction proof.

## Preserved implementation facts

The current C-level Protocol I implementation remains
m2_protocol_i_raw_score_input_modular_8round_mask_output: **four core rounds**
and **eight raw-score-to-original-order-mask end-to-end rounds**. This is a
preserved C-level engineering baseline, not a strict paper-exact implementation.
Existing identities, historical tests/results, and historical performance
records remain unchanged.

## Required execution order

M2 Protocol I exact
-> Protocol I communication verification
-> interface handoff
-> M5 Protocol III exact
-> Protocol III communication verification
-> interface handoff
-> M6A AAV86
-> M6B BB90+DCF
-> M7 unified six-scheme report

M4 remains cancelled. The sequence is a dependency order, not evidence that a
blocked strict gate is currently runnable.

## Evidence needed to unblock strict M2 G1

Obtain target-paper or equivalently authoritative primary evidence for the
complete three-round causal transcript; senders, receivers, fields, openings,
and dependencies; concrete shuffle and correlated r/GRank material generation
that keeps full `r` unknown to P0, P1, and P2; distribution, binding, one-shot
consumption; permutation knowledge for every party; and each party's view and
leakage boundary. Until then, do not invent those details or label any path
paper-exact.
