# M2 Protocol I Output Contract Review - Stage 3J

Date: 2026-09-13  
Reviewed revision: `b44a1f8281ce0cd4bd41d65d193e808a41df4874`  
Environment evidence: Ubuntu 24.04 EMP-ON clean-revision validation in `/tmp/moe-stage3i-on2`; Stage 3I full CTest 32/32 PASS.

## Executive conclusion

Stage 3J freezes the output-contract review without changing secure code. The M2 candidate runtime evidence remains valid on the clean revision, but the candidate ends at shuffled-domain additive rank shares. A complete original-order XOR Top-K mask needs a separately specified secure selection carrier and inverse route. The current evidence does not authorize production adapter implementation or a seven-round claim.

## Four evidence classes

Paper-defined facts are limited to the published Protocol I topology/high-level behavior and reported round target. Local reference repositories provide behavior examples only. Project extensions are the candidate's Dealer-preprocessed output-slot-`r` model, VFSS transport, stable priority-key and mask contract. Exact transcript, formal Dealer view, correlated public-list/GRank material, leakage equivalence, secure rank-to-bit and 7-round path remain unverified design hypotheses.

## Contract and leakage review

The candidate's `public_masked_list` is a public shuffled list, while `shuffled_rank_share` remains private additive shares. No rank, selected index, permutation, original-order mask or selection carrier is opened. P2 is offline-only in the declared project model. The candidate's leakage therefore differs from Route A, which would publicly reveal shuffled ranks, and differs from M3's independently specified routing transcript.

The current reverse-shuffle API is an inverse permutation over a shared carrier. It does not define how a rank share becomes a bit. Reusing M3 DPF routing would change protocol identity and is prohibited. Reconstructing rank, using `rank_share < K` locally, or creating a carrier in TEST_ONLY would violate the secure/test boundary.

## Route decisions

Route A is the shortest complete *project* path only if rank disclosure is explicitly accepted and counted: raw-score adapter 2 + candidate core 3 + rank reveal 1 + reverse PS 2 = 8 online rounds. Route B is potentially less leaky but lacks an approved secure comparison primitive, material contract and measured barrier count. Route C targets 7 rounds but requires a new paper-compatible core; current candidate R3 cannot be silently reused as both masked-list opening and rank reveal.

## Current statuses

- `M2_CANDIDATE_RUNTIME_GO` / `CORE_RUNTIME_GO`: PASS for candidate evidence.
- `M2_OUTPUT_CONTRACT_DESIGN_REVIEW_COMPLETE`: PASS.
- `M2_ADAPTER_ENTRY_BLOCKED`: BLOCKED.
- `M2_PAPER_EXACT_BLOCKED`: BLOCKED / NOT_VERIFIED.
- `M3_CALIBRATED`: PASS for the Stage 3I modular contract; M3 remains non-Theorem-4.2 exact.
- Performance/network/PRG fields not freshly measured: `NOT_MEASURED`.

## A-J answers

A. Clean candidate evidence revision: `b44a1f8` (PASS, Stage 3I clean revision).  
B. Candidate remains `CORE_RUNTIME_GO` (PASS, candidate gate only).  
C. Candidate output fields: `public_masked_list`, `shuffled_rank_share`, metrics, trace (PASS).  
D. Selection carrier exists: NO, `BLOCKED`.  
E. Secure rank-share to selection-carrier contract frozen: NO, `BLOCKED`.  
F. Shortest complete route: Route A as an auditable project extension, not approved for implementation until leakage is accepted.  
G. Route A adds rank reveal/exchange plus two reverse PS rounds; raw-score total is 8 rounds and publicly exposes shuffled rank (PROJECT_EXTENSION, not paper confirmed).  
H. Route B primitive/material/round count: not defined (`BLOCKED`, `NOT_VERIFIED`).  
I. True 7-round Route C has sufficient evidence: NO (`BLOCKED`, research-only).  
J. M2 paper-exact: `BLOCKED / NOT_VERIFIED`; adapter entry: `BLOCKED`; M3: `M3_CALIBRATED`.

## Modified files and commands

Stage 3J adds only these documents:

- `docs/decisions/M2_PROTOCOL_I_OUTPUT_CONTRACT_GATE_2026-09-13.md`
- `docs/reproduction/M2_PROTOCOL_I_OUTPUT_CONTRACT_REVIEW_2026-09-13.md`

No secure C++, M2 formal path, M3 runtime, CMake target, adapter, reverse-shuffle implementation, or label was changed. The review reused the Stage 3I evidence commands and records; no duplicate full M3 regression was run, per scope.

## Entry criteria for the next implementation stage

Before production adapter code, freeze the carrier's domain/share type, rank-to-bit primitive, dealer materials, reverse direction, dummy handling, output length/value domain, causal rounds, bytes/trace, failure matrix, and independent-process conformance/E2E. Keep rank-share conversion separate from reverse shuffle. A seven-round research stage must additionally supply a paper-compatible transcript and leakage/simulator evidence.
