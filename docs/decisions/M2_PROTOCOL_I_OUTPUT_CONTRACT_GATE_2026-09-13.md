# M2 Protocol I Output Contract Gate

Date: 2026-09-13  
Evidence revision: `b44a1f8281ce0cd4bd41d65d193e808a41df4874`  
Status: `M2_OUTPUT_CONTRACT_DESIGN_REVIEW_COMPLETE`, `M2_ADAPTER_ENTRY_BLOCKED`, `M2_PAPER_EXACT_BLOCKED`

## 1. Fixed identities

| Identity | Label | Input | Output | Rounds | Status |
|---|---|---|---|---:|---|
| M2 formal baseline | `m2_protocol_i_raw_score_input_modular_8round_mask_output` | raw-score shares | original-order XOR mask | 8 | project baseline, unchanged |
| M2 candidate | `m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate` | padded priority-key additive shares | shuffled-domain additive rank shares plus public masked list | 3 | `CORE_RUNTIME_GO` for candidate gate |
| M3 modular baseline | `agarwal_protocol_iii_modular_3round` | padded priority-key shares | original-order XOR mask | 3 | `M3_CALIBRATED` |
| M3 raw-score extension | `moe_topk_protocol_iii_raw_score_modular_5round` | raw-score adapter input | original-order XOR mask | 5 | project extension |

The candidate is a project extension, not paper-exact Protocol I, not a complete mask path, and not a seven-round implementation. Its exact core is R1 first forward Permute+Share, R2 second forward Permute+Share, R3 masked shuffled-list opening, with local DCF/CmpAgg evaluation.

## 2. Evidence classes

- **A, paper definition:** Protocol I's 2+1 topology and reported three-round target; high-level secure shuffle/ranking behavior and paper output semantics where explicitly stated.
- **B, local reference behavior:** Agarwal/Chase reference code and historical message/key behavior. These are not proof of the paper transcript.
- **C, project extension:** Q20.12 signed score semantics, stable priority-key, padding, original-order XOR mask, VFSS transport, candidate output-slot `r`, and M3 modular routing.
- **D, not verified:** exact modified-shuffle transcript, correlated public-list/GRank material proof, formal Dealer view, leakage simulator, secure rank-to-selection, and seven-round path.

## 3. Current candidate versus complete output

| Property | Candidate | Complete unified output |
|---|---|---|
| Input | padded priority-key additive shares | raw-score shares or an explicitly approved adapter |
| Domain | shuffled slots | original input order |
| Output fields | `public_masked_list`, `shuffled_rank_share`, metrics, trace | logical-length XOR bit shares |
| Selection carrier | absent | required secure input to output adapter |
| Reverse route | absent | required, with fresh inverse-shuffle material |
| Rank reveal | absent | not allowed by default; requires disclosure approval |
| Paper status | C project extension | undecided |

`shuffled_rank_share` is neither `selection_bit_share` nor `original_order_topk_bit_share`.

## 4. Frozen rank-to-selection contract gap

The intended relation is `selection_bit[i] = [rank[i] < K]`. The current candidate does not define or implement this conversion. A production contract must still specify the rank ring and valid range, public/secret `K`, shuffled versus original domain, additive versus XOR output shares, party ownership, whether any masked rank is opened, secure primitive and dealer materials, message direction/framing, causal barriers, carrier/index binding, inverse-shuffle share type, leakage, metrics, replay/EOF/timeout behavior, and fail-closed semantics. Until all are independently reviewed, no adapter source or target may be created.

The conversion is nonlinear. Local tests cannot compare each party's share with `K`; opening rank without counting a real transport barrier is invalid; reverse shuffle only changes slot order and cannot perform the conversion.

## 5. Route comparison

| Route | Complete output | Additional causal work | Leakage | Engineering | Paper fidelity | State |
|---|---|---|---|---|---|---|
| A: public shuffled rank then reverse | yes, if disclosure approved | rank reveal/exchange plus two reverse PS rounds; 8-round raw-score project path (`2+3+1+2`) | public shuffled rank/order | low/medium | unverified | candidate direction, not approved |
| B: secure rank-to-bit then reverse | yes, if primitive is proven | secure comparison/material stage (at least one real barrier, possibly more) plus two reverse PS rounds | lower than A, but primitive-dependent | high | unverified | not designed |
| C: true 7-round research path | target only | no hidden extra barrier beyond redesigned 3-round core and two-round output path | to be proved | very high | target route | blocked |

Route A is the shortest auditable project path, but it changes leakage and is not paper exact. Route B has no frozen primitive, material, or transcript. Route C requires a new paper-compatible core that produces a legal carrier within three rounds; current candidate cannot be relabeled to achieve this.

## 6. Stage tables

### Candidate

| Stage | Roles | Message/open | Output/share | Barrier |
|---|---|---|---|---:|
| O0 | P2 to P0/P1 | framed packages, no open | `r` shares and edge material | 0 |
| R1 | P0/P1 | forward PS, no open | intermediate shares | 1 |
| R2 | P0/P1 | forward PS, no open | shuffled key shares | 1 |
| R3 | P0/P1 | masked-share exchange; opens public `y` | public masked list | 1 |
| Local | P0/P1 | no message | shuffled rank shares | 0 |

### Route A

Adds a real rank-share exchange/reveal and selection construction, then two fresh reverse PS calls. The rank reveal is public leakage and must be separately counted; the reverse calls output original-order arithmetic carrier shares, followed by a local bit-share conversion.

### Route B

Adds a secure rank-to-bit protocol over shuffled rank shares, with separately generated one-shot material and a measured transport trace, then two fresh reverse PS calls. No exact barrier count is assumed before primitive design and E2E evidence.

### Route C

Requires redesigning R1/R2/R3 so the same paper-compatible transcript simultaneously binds payload, mask and ranking material and ends with a legal selection carrier. It must prove no hidden rank reveal or duplicate R3 work. This is a research design gate, not an implementation plan.

## 7. Paper Protocol I gap matrix

| Item | Evidence/status |
|---|---|
| 2+1 topology / three-round target | `PAPER_CONFIRMED` (A) |
| input/output exact mapping to project priority-key and XOR mask | `PROJECT_EXTENSION` / `NOT_VERIFIED` |
| input-independent Dealer view | `NOT_VERIFIED` |
| modified shuffle per-message transcript | `NOT_VERIFIED` |
| public masked-list timing and visibility | high-level paper requirement, implementation `NOT_VERIFIED` |
| same permutation and correlated `r`/GRank material | `BLOCKED` |
| rank visibility and selection semantics | `NOT_VERIFIED` |
| original-order XOR mask output | project-only, `NOT_VERIFIED` as paper claim |
| exact leakage simulator and performance | `BLOCKED` / `NOT_MEASURED` |

The existing decision records remain historical evidence; none are rewritten to imply paper proof. In particular, `M2_PROTOCOL_I_PAPER_EXACT_3ROUND_DESIGN.md` and `M2_PROTOCOL_I_REVERSE_SHUFFLE_SPEC.md` already identify the public-list/correlation and reverse-adapter gaps.

## 8. Adapter entry gate

Required conditions include a secure selection carrier, carrier/domain/index binding, proven inverse direction, explicit logical/padded/dummy semantics, logical-length XOR output, no secure reconstruction, measured adapter rounds/material/messages, negative cases, conformance/oracle/E2E plans, and independence from M3 and the formal M2 baseline. These conditions are not all frozen. Result: `ADAPTER_ENTRY_BLOCKED`; no production adapter, reverse-shuffle target, seven-round executable, or paper-exact label is authorized.

## 9. Next-stage gate

The next stage must choose between an explicitly disclosed 8-round project path and a separately reviewed secure rank-to-bit design. A true seven-round path remains research-only until a formal transcript, leakage argument, carrier contract, and independent-process evidence exist.
