# M2 Protocol I current evidence gate

Date: 2026-09-19

Status: **canonical current M2 evidence gate.** This record supersedes current
gate references in active planning and handoff documents; it does not rewrite
historical implementation, test, performance, or decision records.

## Decision

| Identity or gate | Current status | Basis |
| --- | --- | --- |
| Protocol I message/material/party-view/round-exact | **BLOCKED** | Required target-paper transcript and concrete material details are unavailable. |
| Protocol I paper-aligned experimental reproduction | **READY only as NON-EXACT** | The conference paper supports a high-level functionality and cost target, but not the missing exact transcript/material claim. |
| M2 G2 communication verification | **BLOCKED** | Strict M2 G1 has not passed. |
| M2 G3 interface/evidence handoff | **BLOCKED** | Strict M2 G1 and G2 have not passed. |
| M5 runtime implementation | **BLOCKED** | The required M2 G3 handoff has not passed. Design and evidence preparation remain separate, non-runtime work. |
| Dealer-DPF BoundPublicMaskShuffle | **VALID design; NON-EXACT C-INSTANTIATION** | Security-contract remediation complete; implementation absent; tests/benchmarks not run; measurements NOT_MEASURED. |

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

Supporting literature, including SIGMA, is separate from A--D. It may provide
general background but cannot be promoted to A or used to infer the missing
Protocol I transcript, material, party view, or round-exact construction.
Neither local references, current VFSS code, nor project extensions may supply
those missing target-paper facts.

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
and dependencies; concrete shuffle and correlated r/GRank material generation,
distribution, binding, and one-shot consumption; and each party's view and
leakage boundary. Until then, do not invent those details or label any path
paper-exact.
