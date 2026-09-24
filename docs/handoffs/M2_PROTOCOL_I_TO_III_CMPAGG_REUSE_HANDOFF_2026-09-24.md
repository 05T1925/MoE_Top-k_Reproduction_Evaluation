# Protocol I / Protocol III CmpAgg Reuse and Interface Handoff

Date: 2026-09-24

Scope: Protocol I to Protocol III implementation handoff

Evidence classes: paper definition, inspected local implementation, project adapter, unresolved design

## 1. Architectural relationship

Agarwal Protocol I and Protocol III share one ranking core and differ in routing:

```text
                     shared clique CmpAgg / GRank
                                  |
                  +---------------+---------------+
                  |                               |
        Protocol I shuffle routing      Protocol III DPF routing
```

Protocol III is not a fresh ranking implementation. Reuse stops after additive
rank-share production. Protocol I runtime state is not an input to Protocol III.

## 2. Paper evidence

The local conference paper `Papers/Agarwal 等 - 2024 - Secure Sorting and
Selection .pdf` establishes the following:

- Section 1.1 separates secure ranking from routing.
- Section 3.1 and Figure 3 define `GenCmpAgg`/`EvalCmpAgg`. On a clique, CmpAgg
  realizes GRank (Corollary 3.1.1).
- Table 1 assigns the same `(n choose 2)-CmpAgg` ranking to Protocol I and
  Protocol III. Protocol I uses shuffle routing in three online rounds;
  Protocol III uses DPF routing in two.
- Section 4.1 opens shuffled rank values only after shuffle-based ranking.
- Section 4.2 starts DPF routing from secret-shared ranks and does not open raw
  ranks. Its two-round compression requires a field payload, nonzero payload
  masks, and inverses.
- Theorem 4.2's footnote mentions a common mask for the Beaver triple and the
  ranking gate. The conference text does not authorize arbitrary mask reuse.

These are paper-defined architectural facts. File names and the current ring
adapters below are project implementation facts.

## 3. Shared CmpAgg ranking core

| File | Symbol | Role | Current callers | Classification |
| --- | --- | --- | --- | --- |
| `VFSS/include/moe_topk/protocol_i_ucmp.h` and `VFSS/src/moe_topk/protocol_i_ucmp.cpp` | `ProtocolIUcmpMaterial`, `ProtocolIUcmpPartyMaterial::eval_strict_lt` | One-shot DCF-backed masked strict comparison | Protocol I CmpAgg, Protocol III GRank, score adapter, tests | Generic math with historical Protocol-I name; reuse directly |
| `VFSS/include/moe_topk/protocol_i_cmpagg.h` and `VFSS/src/moe_topk/protocol_i_cmpagg.cpp` | `protocol_i_mask_priority_key_share` | Add one comparison-mask share in the configured comparison ring | Protocol I and Protocol III GRank paths | Generic ranking input helper; reuse directly |
| same | `protocol_i_cmpagg_eval_party` | Clique edge evaluation and linear aggregation into additive rank shares | Protocol I pipeline/parallel shuffle and `protocol_iii_grank_party` | Generic clique CmpAgg with historical Protocol-I name; reuse directly |
| `VFSS/include/moe_topk/protocol_i_party_package.h` and source | `ProtocolIEdgePartyMaterial`, ranking fields of `ProtocolIPartyPackage` | Session-bound node-mask shares and clique uCMP key material | Protocol I and Protocol III GRank | Mixed package; adapt/extract a ranking view rather than copy |
| `VFSS/include/moe_topk/protocol_i_priority_key.h` | `protocol_i_priority_key`, `protocol_i_index_bits` | Project priority encoding: score DESC, original index ASC | Both protocols and tests | Project-generic stable-key helper; reuse directly |
| `VFSS/include/moe_topk/topk_oracle.h` | `stable_ranks_cmpagg`, `top_k_mask_from_stable_ranks` | Clear differential oracle only | Protocol I/III tests | Shared test oracle; never a secure-runtime participant |
| `VFSS/include/moe_topk/protocol_iii_grank.h` and source | `protocol_iii_grank_party` | Protocol-III input opening/domain adapter around the shared CmpAgg evaluator | Protocol III secure core | Thin Protocol-III adapter; keep |

`ProtocolIUcmpMaterial` and `protocol_i_cmpagg_eval_party` are mathematically
generic despite their names. A future rename/extraction is optional cleanup,
not a prerequisite for Protocol III, and must preserve the Protocol I ABI and
behavior.

## 4. Current implementation map

The inspected call path is:

```text
priority-key additive shares
  + ProtocolIPartyPackage.node_mask_shares / edge_materials
  -> protocol_iii_grank_party
  -> open only public masked key vector
  -> protocol_i_cmpagg_eval_party
  -> ProtocolIIIGrankOutput.rank_additive_shares
  -> protocol_iii_dpf_routing_party
  -> indicator shares
  -> protocol_iii_secure_combine_party
  -> original-order XOR Top-K mask shares
```

`protocol_iii_grank_party` already includes and calls the existing Protocol-I
named CmpAgg implementation. It does not duplicate the rank algorithm.

## 5. Reuse matrix

| Component | Protocol I | Protocol III | Reuse decision |
| --- | --- | --- | --- |
| priority-key encoding | score DESC/index ASC | same | REUSE |
| raw Q20.12 score semantics | input adapter | input adapter | REUSE |
| uCMP | DCF-backed edge gate | same edge gate | REUSE |
| low-level DCF Gen/Eval | uCMP | same | REUSE |
| CmpAgg Gen/material | clique node masks and edge keys | same ranking material | ADAPT mixed package; do not duplicate |
| CmpAgg Eval | `protocol_i_cmpagg_eval_party` | called by GRank adapter | REUSE |
| rank shares | shuffled position order | original logical input order | REUSE representation with explicit layout contract |
| rank oracle | clear test oracle | same oracle | REUSE in tests only |
| score input | Protocol-I-named adapter | already reused by raw pipeline | ADAPT naming/package only |
| record schema | `ProtocolIBlock192` | not a field | DO_NOT_REUSE as Theorem 4.2 payload |
| payload schema | two `uint64_t` lanes in product ring | requires field for compressed routing | ADAPT with a new field representation |
| transport | framed channel/session validation | currently reused | REUSE/ADAPT phase identifiers only |
| party package | ranking plus Protocol-I score fields | ranking subset needed | ADAPT; extract a view later if useful |
| metrics | stage-specific structures | stage-specific structures | ADAPT under common accounting conventions |
| shuffle | Chase/parallel shuffle | absent | DO_NOT_REUSE |
| rank opening | Protocol I R3 | forbidden before DPF routing | DO_NOT_REUSE |
| local routing | public shuffled ranks | absent | DO_NOT_REUSE |
| DPF routing | absent | masked-rank DPF | III_ONLY |
| Beaver material | absent in shuffle route | modular secure combine | III_ONLY |
| payload multiplicative mask | absent | required for two-round compression | III_ONLY |
| DPF FullEval | absent | required for general Fsort | III_ONLY |

## 6. Shared contracts

### A. Shared ranking input

- `n`: number of logical inputs; CmpAgg uses the clique on these positions.
- comparison key: project priority key in `Z_(2^comparison_bits)`.
- ordering: lower encoded key means higher project priority.
- stable semantics: score descending, original index ascending.
- each online party holds one additive key-share vector; public masked inputs
  are produced by the protocol-specific input-preparation stage.
- Protocol I and Protocol III may prepare those public masked inputs
  differently. That preparation is not part of shared CmpAgg Eval.

### B. CmpAgg preprocessing

- one comparison-mask share per logical node;
- one one-shot `ProtocolIUcmpPartyMaterial` per lexicographically enumerated
  clique edge `(left,right)`, `left < right`;
- party id is 0 or 1;
- session, fingerprint, shape, bit width, and party bindings fail closed;
- the dealer/compiler may generate the pair, but online parties receive only
  their own material.

### C. CmpAgg evaluation

Input: party id, comparison width, the common public masked-key vector, and
that party's ordered edge materials. Output: one additive rank share per
logical input. The evaluator neither routes records nor opens ranks.

### D. Rank-share contract

The current concrete equivalent of a future `CmpAggRankShares` is
`ProtocolIIIGrankOutput::rank_additive_shares`:

- one vector per party, length `logical_n`;
- vector order equals original logical input order;
- stored as canonical `uint64_t` values in `Z_(2^rank_bits)`;
- current `rank_bits = max(1, ceil(log2(logical_n)))`;
- reconstruction modulo `2^rank_bits` yields a unique value in `[0,n)`;
- rank 0 denotes the highest project priority;
- equal scores are ordered by original index ascending;
- no padded slot is present;
- the vector remains secret-shared and is consumed directly by DPF routing.

This is a project C-INSTANTIATION. The paper states ranks in `Z_n`; for
non-power-of-two `n`, `Z_(2^rank_bits)` is not literally `Z_n`.

### E. Record/payload contract

Record association is by vector position: rank share `i` belongs to record
share `i`. Any Protocol III field adapter must preserve that association,
stable-key semantics, and original-order output mapping.

## 7. Protocol-I-only components

The following must not enter Protocol III runtime state or packages:

- Chase chosen OT, OPV, Share Translation, Beneš, Permute+Share, and
  SecretSharedShuffle;
- parallel-shuffle `Pi`, `sigma`, `tau`, `a`, `e`, `h`, shuffle masks and
  secret shuffled record shares;
- Protocol I public `Pi(x)+r_cmp`;
- Protocol I rank opening and public shuffled ranks;
- Protocol I local routing from opened shuffled ranks.

## 8. Protocol-III-only components

- rank masks `r_rank[i]` and their additive shares;
- DPF point/payload keys and masked-rank reconstruction;
- indicator/permutation-matrix shares;
- payload field representation, nonzero encoding, multiplicative masks and
  inverses for the compressed construction;
- Beaver or equivalent product correlations where the modular baseline needs
  them;
- cross-stage two-round state machine;
- Fselect routing and the Fsort/FullEval extension;
- Theorem 4.2 communication accounting.

## 9. CmpAgg-to-DPF-routing interface

Current producer:

```cpp
ProtocolIIIGrankOutput::rank_additive_shares
```

Current consumer:

```cpp
protocol_iii_dpf_routing_party(config, material, rank_shares, fd)
```

Freeze the semantic interface before changing types:

```text
party-local additive rank shares
length = logical_n
domain = current Z_(2^rank_bits) C-INSTANTIATION
rank 0 = highest priority
order = original logical input order
associated record = record at the same index
visibility = never opened raw
```

The existing type is sufficient for the first differential work. A named
shared wrapper/alias may be introduced later only if it makes domain/layout
validation stronger without touching Protocol I runtime behavior.

## 10. Payload/field compatibility issue

`ProtocolIBlock192` represents three independent lanes:

```text
Z_(2^comparison_bits) x Z_(2^64) x Z_(2^64)
```

It is a product of rings/storage lanes, not a field, and therefore cannot be
used directly as Theorem 4.2's payload field `H`. `ADAPTER_NEEDED`.

The Protocol III task must choose and test a `ProtocolIIIFieldRecord` (or a
minimal equivalent adapter) defining field choice, serialization, nonzero
payload encoding, multiplication/inversion, DPF payload compatibility, and
conversion to the project output. Protocol I records must remain unchanged.

## 11. Random-mask naming and ownership

Use distinct names:

- `r_cmp[i]`: CmpAgg comparison-input mask;
- `r_rank[i]`: Protocol III DPF rank mask/point;
- `s_payload[i]`: nonzero multiplicative payload mask for compressed routing;
- `a_payload`, `b_indicator`, `c_mul`: modular Beaver correlation components.

Do not identify or reuse these masks unless a separately documented proof and
message schedule implements the Theorem 4.2 common-mask optimization. Current
code uses separate masks; the optimization is `NOT_IMPLEMENTED`.

## 12. Existing Protocol III status

| Module | Status | Notes |
| --- | --- | --- |
| `protocol_iii_grank` | IMPLEMENTED_AND_TESTED | One round; thin adapter around shared CmpAgg |
| `protocol_iii_dpf_routing` | IMPLEMENTED_AND_TESTED | One-round `logical_n x k` indicator routing in `Z_(2^rank_bits)` |
| `protocol_iii_secure_combine` | IMPLEMENTED_AND_TESTED | One-round unit-payload specialization over `Z_(2^64)` |
| `protocol_iii_secure_core` | IMPLEMENTED_AND_TESTED | Frozen modular three-round priority-key baseline |
| `protocol_iii_raw_score_pipeline` | IMPLEMENTED_AND_TESTED | Frozen five-round raw-score path |
| general payload Fselect | IMPLEMENTED_PARTIAL | Indicator/unit-payload path exists; general field payload does not |
| Fsort / DPF FullEval | NOT_IMPLEMENTED | Required as a separate extension |
| field-based two-round compression | NOT_IMPLEMENTED | M5 target |
| Theorem 4.2 common-mask optimization | NOT_IMPLEMENTED | Keep separate from base two-round correctness |

Verification on 2026-09-24 at `f993326d1e8305aa885afbcd813c7c6dc5e0d3d2`:

- focused CmpAgg tests: 4/4 passed;
- frozen Protocol III/M3 matrix: 11/11 passed.

The Protocol III core does not duplicate CmpAgg evaluation. It does duplicate
some protocol-specific configuration, validation, framing, and domain-adapter
logic; compare differentially before any refactor.

## 13. Required adapters

1. A ranking-package view that exposes only CmpAgg node masks and clique edge
   material, leaving score-input and Protocol-I-only fields out.
2. An explicit rank-domain adapter/decision for paper `Z_n` versus current
   `Z_(2^rank_bits)`.
3. A field payload record/packing adapter with nonzero encoding and inverse
   failure semantics.
4. A DPF payload adapter compatible with the chosen field representation.
5. A two-round cross-stage state machine that combines ranking preparation
   with compressed DPF routing without opening raw ranks.
6. Stage metrics that keep core communication, representation conversion, and
   project output adaptation separate.

## 14. Regression boundaries

The Protocol I three-round C-INSTANTIATION has passed independent review.
Renaming or moving shared symbols is permitted only as a behavior-preserving
alias/extraction with all Protocol I and Protocol III regressions passing.

`PROTOCOL_I_REAUDIT_REQUIRED = YES` if a change touches Protocol I R1/R2/R3,
parallel-shuffle material, public-y semantics, causal ordering, rank opening,
or routing. Prefer adapters/aliases so this remains `NO` for Protocol III work.

Never modify `VFSS-baseline/`, reintroduce DPF routing into Protocol I, or make
Protocol III depend on Protocol I runtime output.

## 15. Protocol III implementation entry point

The first implementation target is not another ranking implementation. It is:

1. rerun and freeze the existing shared CmpAgg/GRank behavior;
2. freeze `ProtocolIIIGrankOutput::rank_additive_shares` to DPF routing;
3. differential-test the existing modular DPF routing baseline;
4. choose/validate the field payload adapter;
5. implement the two-round compressed candidate while preserving the M3
   three-round and five-round baselines;
6. only then evaluate the common-mask optimization and communication formula.

## 16. Open questions

- Which concrete field and serialization satisfy Theorem 4.2 and the project
  record/payload requirements?
- Is the current `Z_(2^rank_bits)` padded rank domain an accepted
  C-INSTANTIATION, or must the exact path implement `Z_n`?
- How is zero payload encoded as a nonzero field element without changing the
  final project semantics?
- Does the repository DPF payload API support the selected field directly, or
  is a proven encoding/decomposition required?
- What exact message/material correlation realizes the two-round compression?
- Is the optional common-mask communication optimization in scope only after
  the unoptimized two-round construction passes?

## Git/PR handoff state

The inspected checkout remains on `feat/m2-chase-c1-c3` at `f993326d...`.
GitHub PR #23 is merged into remote `main` as merge commit
`ac7f0ed2def54f5c90bb8b9f297792c323dd2f72` (2026-09-24). The local
`main`/`origin/main` refs in this checkout are stale at `5d3f36a...` because
this documentation task did not fetch or switch branches. Protocol III coding
should start from a freshly fetched, updated `main` containing the merge commit,
unless the user explicitly chooses parallel development.
