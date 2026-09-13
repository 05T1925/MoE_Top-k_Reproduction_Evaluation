# M2 Protocol I Raw-Score Route A

Status: `PROJECT_EXTENSION`; paper-exact status remains `BLOCKED / NOT_VERIFIED`.

Implementation label: `m2_protocol_i_raw_score_dealer_preprocessed_rank_reveal_8round_mask_output`.
This path is separate from the formal
`m2_protocol_i_raw_score_input_modular_8round_mask_output` baseline and from the
priority-key Route A label.

## Contract

The input is a two-party additive share of signed 32-bit Q20.12 raw scores.
The score adapter performs two framed online exchanges: carry (R0) and sign
(R0b). It widens the shares to the padded priority-key ring, preserving signed
two's-complement ordering and original-index stable ties. The candidate core
then contributes R1/R2/R3, Route A reveals the shuffled rank in R4, and the
existing secure reverse shuffle contributes R5/R6. The causal total is eight
online rounds.

The Dealer sends an input-independent `ProtocolIPartyPackage` containing node
mask shares, candidate CmpAgg edge material, and separate carry/sign material.
The online controller sends only raw-score shares. It does not send a selected
index, rank, carrier, or final mask. P2 has no online channel and exits after
the package handoff.

The output frame is bound to session, fingerprint, `material_id`, padded dimensions, party,
phase 8 and sequence 1. Its payload identity is `RA8M` version 1, followed by
`material_id`, logical output count, carry/sign bytes, candidate/reveal/reverse
bytes, total rounds, and the logical-domain XOR bit share. The controller reconstructs the
mask only in `TEST_ONLY` code for oracle checking.

## Leakage

Public values are the configured dimensions, the candidate masked shuffled list,
the shuffled rank and its public selection carrier, and the measured framed
metadata. Raw scores, priority keys, original indices, local permutations,
selected original indices, party-local shares, and the original-order mask are
not reconstructed by the secure runtime.

## Verification

The independent-process CTest starts a separate Dealer, P0 and P1 with
`fork`/`exec`, uses framed package/input/score/candidate/rank/reverse/result
channels, checks child exit status and output identity, and compares the final
XOR reconstruction with the signed-score oracle. The matrix covers
`n={1,2,3,5,7,8,11,16,31,127,128,129,256}`, `K=1`, `K=2`, `K=8` where valid,
`ceil(n/2)`, and `K=n`, with random, equal, alternating extreme, all-minimum,
and monotone inputs. The raw Route A target passed in a fresh Ubuntu 24.04
EMP-ON build; performance benchmarking is not claimed.
