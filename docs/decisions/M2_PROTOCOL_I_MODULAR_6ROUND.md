# M2.13 Protocol I modular 6-round mask-output

## Current status (2026-09-06)

M2.13 is the frozen padded-priority-key predecessor of the successor
`m2_protocol_i_raw_score_input_modular_8round_mask_output` baseline. The
successor adds the raw Q20.12 arithmetic-share carry/lift/sign adapter in two
online rounds. This document preserves the M2.13 historical input boundary.

Implementation label: `m2_protocol_i_modular_6round_mask_output`.

Status: C-level project-engineering baseline. It is not
`agarwal_protocol_i_exact`, `agarwal_protocol_i_exact_mask_output`,
`paper_3_round_exact`, or `secure_shuffle_complete`.

## Evidence boundary

| Item | Evidence | Meaning |
| --- | --- | --- |
| Protocol I is 2+1, shuffle routing plus all-pairs CmpAgg, and Table 1 reports 3 online rounds | A | Agarwal CCS 2024, Table 1 and §4.1. The conference paper does not specify this implementation's packet transcript. |
| Two-pass Permute+Share sequencing | B | Local B1-oriented reference behaviour only; no old ABI or code is imported. |
| padded priority-key layout, framed P2 package transport, original-order XOR mask and counters | C | This repository's auditable engineering extension. |
| paper-exact transcript, leakage proof for all components and raw 32-bit arithmetic-share widening | D | Still unresolved and explicitly not claimed. |

## Stage table

| Stage | Roles and input | Message/opening | Output | Causal rounds |
| --- | --- | --- | --- | --- |
| O0 package | P2, input-independent materials | P2→P0/P1 framed chunks bound to session/fingerprint/role/phase/sequence; P2 exits | per-party CmpAgg package | offline |
| O1 preprocessing | P0/P1 after deserialize | four offline Permute+Share material exchanges | one-shot forward/reverse material | offline |
| R1 forward | P0/P1 padded key shares | two Permute+Share messages | shuffled key shares | 2 |
| R2 CmpAgg | P0/P1 masked shuffled keys | one framed masked-key exchange; no score/key opening | rank shares | 1 |
| R3 reveal | P0/P1 rank shares | one framed rank-share exchange | shuffled rank reconstruction only in the test harness | 1 |
| R4 reverse | P0/P1 shuffled carrier shares | two Permute+Share messages | padded original carrier, cropped logical XOR-bit shares | 2 |

The implementation validates logical/padded dimensions, package identity and
all canonical edges, comparison-ring range, rank length/range/full-permutation,
and exact one-time package material movement. The controller releases input
only after P2 has exited and both online parties acknowledged package receipt
and all offline preprocessing.

## Input, output and metrics

`logical_n` is 1..1,000,000 and `padded_n=max(2,next_power_of_two(logical_n))`,
capped at 1,048,576. Padded slots are `(INT32_MIN, logical_n...padded_n-1)`,
so real `INT32_MIN` entries precede dummies. The runtime accepts caller-prepared
additive comparison-ring priority-key shares. Legacy 32-bit arithmetic score
share conversion was not implemented in that milestone; it is implemented by
the dated M2.14 successor decision and reproduction record, not by this
historical baseline.

Output is cropped to `logical_n` and uses only word0's LSB. Test-only code
reconstructs final shares for the frozen oracle. The E2E emits actual P2 package
frame bytes and per-phase P0/P1 bytes, excluding controller traffic, plus padded
comparison edges, two raw DCF evaluations per edge and rounds `2/1/1/2=6`.
Timing, PRG-call counts, WAN/LAN and benchmark repetitions are `NOT_MEASURED`.

## Remaining gates

This closes neither a paper-exact 3-round claim nor a raw-score-share input
adapter. The composed shuffle proof and rank-reveal leakage policy remain
project-review gates.
