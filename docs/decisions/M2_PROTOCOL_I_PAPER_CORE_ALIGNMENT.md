# M2.15 Protocol I paper-core alignment

Status: **completed as a C-level alignment audit; 3-round candidate not achieved.**
The current implementation remains
`m2_protocol_i_raw_score_input_modular_8round_mask_output`. This document does not
promote it to `agarwal_protocol_i_exact`, `agarwal_protocol_i_exact_mask_output`,
or `paper_3_round_exact`.

## Evidence boundary

| Evidence | Finding | Classification |
| --- | --- | --- |
| Agarwal CCS 2024, §4.1, PDF p.8 | The shuffle functionality used by shuffle-then-reveal outputs a public masked shuffled list in addition to secret-shared shuffled output. | A |
| Agarwal CCS 2024, Table 1 and §4.1, PDF pp.8–9 | Protocol I is reported as a 2+1 construction with 3 online rounds at the paper-core boundary. | A |
| Chase et al., §6.1, PDF p.6 | Secret-shared shuffle outputs additive shares of a shuffled dataset; the public masked-list extension is not part of the base functionality stated there. | A |
| Chase et al., §6.2–§6.3, PDF pp.19–22 | Permute+Share and its two sequential composition expose only party-local shares and use the data-owner message `m`, intermediate `delta` values and fresh `w`. | A |
| `Agarwal_TopK/protocol1/runtime` | B0 names `SHUFFLE_OPEN`, `MASKED_OPEN`, and `RANK_OPEN`; its public masked keys are produced after the shuffle call. | B |
| VFSS `protocol_i_secret_shared_shuffle.*` | The forward shuffle API returns only `ProtocolIBlock192` shares and counters; no public masked list, list fingerprint, phase or sequence is returned. | B |
| VFSS `protocol_i_pipeline.cpp` | The current public masked-key vector is formed after forward shuffle and exchanged in a separate framed phase. | B/C |

Paper statements are not inferred from the local reference or from the VFSS
implementation. The paper's public-list functionality is an A-level requirement;
the missing VFSS composition and transcript are D-level gaps.

## Stage table

| Stage | Roles and input | Preprocessing and messages | Opened values | Output and leakage | Causal rounds |
| --- | --- | --- | --- | --- | ---: |
| O0 offline package | P2 receives only public shape/configuration. P0/P1 receive party-local materials. | P2 sends bound package frames and exits. PS/DCF/score materials are one-shot and input-independent. | None. | Party-local material only. | offline |
| R1 forward shuffle | P0/P1 hold secret-shared key records, including the project index binding. | Two sequential role-switched Permute+Share calls. Current VFSS returns only secret shares. | None. | Secret-shared shuffled payload; no permutation or mapping. | 2 |
| R1′ public masked list candidate | Same shuffled key input and the same composed permutation. | A paper-compatible shuffle functionality would have to produce a public masked list in the same online causal stage, with list binding and materials defined. Current VFSS has no such output or message. | The masked shuffled list, if a future approved functionality supplies it. | Only the paper-approved masked list; no unmasked key, original mapping or permutation. | included in candidate shuffle boundary |
| R2 CmpAgg | Shuffled key shares plus party-local uCMP/DCF material. | Current VFSS forms local masked shares and performs a separate P0↔P1 framed exchange. | Reconstructed masked-key vector. | Current C-level leakage is the public masked-key vector. | 1 |
| R3 rank reveal | Shuffled rank shares. | P0/P1 exchange rank shares. | Controlled shuffled-domain `(slot, rank_P)` only, per D1. | No original mapping or final mask. | 1 |
| R4 reverse mask adapter | Shuffled carrier shares created after rank reveal. | Fresh role-switched inverse two-pass PS. | None. | Original-order XOR mask shares. | 2 |

The R1′ row is a required functionality for the paper-core candidate, not an
implementation claim. It must use the same composition as the secret-shared
payload; a separately generated public permutation or controller-side list would
not be equivalent.

## Exact source of the public masked list

The list is not derivable from the current P2 package. P2 is input-independent
and exits before online inputs are released. A masked list containing the shuffled
input therefore must be produced by the shuffle functionality from the online
secret-shared input and fresh correlated masks. The list must be aligned with the
same hidden composed permutation as the secret-shared payload. The current PS
interface produces party-local shares of the permuted payload; it does not return a
public value or a public-list share pair, and its material does not declare a list
phase, list fingerprint, comparison width, or sequence.

## Message graph and round accounting

Current VFSS message graph:

```text
P2 -> P0/P1 package (offline)
P0/P1: forward PS pass 1
P0/P1: forward PS pass 2
P0/P1: masked-key frame exchange       [current independent phase]
P0/P1: rank-share frame exchange       [D1 controlled reveal]
P0/P1: reverse PS pass 1
P0/P1: reverse PS pass 2
```

The current paper-core-shaped accounting is `2 + 1 + 1 = 4` rounds. The raw
Q20.12 carry/lift/sign adapter is two separate rounds, and the reverse mask
adapter is two separate rounds, so the current total is `2 + 4 + 2 = 8`.

The candidate requested by M2.15 is `forward shuffle/public masked list 2 + rank
reveal 1 = 3` paper-core rounds, with total `2 + 3 + 2 = 7`. This remains a
candidate number only. It cannot be recorded as achieved while the independent
masked-key exchange remains in the message graph.

## Why the current B1 API cannot express the candidate

The two-pass PS implementation has the following interface boundary:

- the permutation owner supplies a private permutation only to its local API;
- the data owner supplies its local record share only to its local API;
- the data owner sends the final payload and masks to the permutation owner;
- the permutation owner reconstructs its local output share using private delta/material;
- each party receives only its own secret share.

That interface is sufficient for the Chase secret-shared shuffle functionality.
It is not sufficient to return a public masked shuffled list to both parties.
Adding the list after the call requires a new message or a new primitive output;
computing it at P2 is impossible because P2 does not know the input; computing it
at the controller is prohibited; and exposing either composed permutation is
prohibited. Renaming the current masked-key exchange as a shuffle output would
change neither the causal graph nor the bytes/barrier count.

The missing capability is therefore primarily an **API/functionality and
preprocessing contract gap**, with a corresponding paper-transcript gap. It is
not fixed by a counter change. The current record-level PS randomness is also not
specified to generate a jointly public list, so the required correlated
randomness cannot be assumed from existing material.

## Public and prohibited values

Public under the current project contract:

- logical and padded `n`, `K`, ring/comparison widths;
- session, fingerprint, phase, role and sequence bindings;
- the current C-level masked-key opening;
- the D1-controlled shuffled `(slot, rank_P)` reveal;
- final test-only reconstruction results, only inside the test harness.

Never public in the secure path:

- raw score words, carry/sign bits, unmasked priority keys;
- either party's permutation or the composed permutation;
- original-index mapping, selected original index, complete rank shares,
  comparison bits and final mask;
- oracle data, debug transcript or a controller-precomputed masked list.

A future public masked-list adapter would need to add only the approved masked list
and its binding metadata. It must not widen the leakage boundary above.

## Failure semantics and material binding

The current fail-closed rules remain: wrong party, slot, stage, session,
fingerprint, sequence, width or count; missing/duplicate material; truncation,
trailing bytes, EOF, timeout, replay or material reuse are hard failures. No retry,
fallback, fixed sleep, online Dealer, file polling or partial output is allowed.

Any future public-list functionality must bind at least
`session`, `fingerprint`, `phase`, `sequence`, `n`, `comparison_bits` and list
length. Its material must be fresh, input-independent at P2, consumed once, and
bound to the same composed permutation as the payload. The current package does
not satisfy that additional contract, so no such interface was added in M2.15.

## Test and evidence status

The new `moe_topk_m2_paper_core_alignment_test` freezes the current `2+1+1+2`
core accounting and the separate `2+4+2` total decomposition. Existing M2 tests
continue to cover PS payload alignment, material one-shot behavior, transport
failure matrices, raw-score differential behavior and independent-process E2E.
No test is allowed to reconstruct secure rank, selected index or final mask outside
the existing TEST_ONLY harnesses.

Evidence levels for the alignment decision:

- **A:** paper-defined public masked-list shuffle functionality and paper 3-round
  core statement;
- **B:** local B0/B1 behavior and source-level VFSS API behavior;
- **C:** project Q20.12 input, priority rank, D1 leakage, original-order XOR mask,
  round decomposition and metric boundaries;
- **D:** a VFSS paper-compatible public-list primitive, its combined security
  argument, exact message transcript and evidence that it preserves the paper's
  leakage model.

## Current decision

M2.15 does **not** achieve the 3-round candidate. The current baseline remains the
accurate C-level `m2_protocol_i_raw_score_input_modular_8round_mask_output` path,
with current core 4 rounds and total 8 rounds. The paper's 3-round number is retained
as A-level context and a D-level future target, not as an observed implementation
result.

M2.15 even when completed can only establish the current VFSS Protocol I
paper-core alignment candidate. Unless the paper message graph, shuffle output,
leakage boundary, preprocessing model and round count all obtain independent
evidence, do not use `agarwal_protocol_i_exact` or `paper_3_round_exact` labels.
