# M2.16 Protocol I exact leakage audit

Status: **blocked D-level audit; no exact approval.**

This audit records what the paper-compatible public masked-list functionality
would require and why the current VFSS path cannot claim it. It is an audit of
the candidate, not evidence that the candidate exists.

## Findings

| Question | Finding | Evidence |
| --- | --- | --- |
| Who sees the public list? | Agarwal's secure shuffle makes `pi(x)+r` available to all online parties. | A: §2.4, §4.1 |
| What is in the list? | A masked shuffled array, not an unmasked score list or index list. | A: §2.4 |
| Who knows `r`? | `r` is unknown to any single party and is used as the FSS secret parameter. | A: §2.4 |
| Is `r` one-shot? | The candidate must consume it once with the corresponding GRank material; the paper excerpt does not specify a VFSS serialization. | A/D |
| Is the list linked to the payload? | It must use the same hidden `pi`; current VFSS has no proof or binding field for this public output. | A requirement / B current gap |
| Can one party recover `pi`? | The intended secure shuffle hides the composed permutation from one party. Current PS evidence only covers local secret-share output. | A/B |
| Does the current masked-key frame equal the list? | No. It is formed after forward PS and is a separate current phase. Renaming it cannot remove its causal barrier. | B/C |
| Does P2 participate online? | It must not. P2 must be input-independent, send packages, and exit. | C frozen contract |
| Does D1 rank reveal remain? | Yes, only the approved shuffled `(slot, rank_P)` may be exposed; original mapping remains secret. | C frozen contract |
| Does D2 inverse routing change leakage? | It must not expose mapping or selected index; current two-pass adapter remains a separate +2 rounds. | C/B |
| Does D3 widening change paper identity? | Yes, Q20.12 arithmetic-share widening and 34-bit uCMP are project extensions and need separate conformance. | C |
| Does D4 transport change the paper claim? | The paper's 3-round claim cannot absorb an undisclosed transport barrier; all causal online messages must be counted. | A/C |
| Is there test reconstruction in secure runtime? | Prohibited. Final mask reconstruction is test-harness only. | C |

## Current prohibited leakage

The following must remain unavailable in secure execution:

- raw score, carry, sign, and unmasked priority key;
- comparison bits, complete rank shares, original-index mapping, selected
  original index, either local permutation, or composed permutation;
- original-order mask, oracle input, debug transcript, or test reconstruction.

The only currently documented public values are shape/configuration metadata,
the D1-controlled shuffled `(slot, rank_P)` reveal, and the current C-level
masked-key opening. The latter is a project extension and is not the same as
the paper's shuffle output.

## Same-permutation security obligation

It is not sufficient that two arrays have the same length or that a test-side
oracle observes the same order. A future implementation must bind the public
list slot and the secret payload slot to the same hidden composed permutation,
and bind the exact mask `r_i` used by the GRank material. A separately sampled
public permutation, controller-side list, P2-generated input-dependent list, or
post-shuffle list exchange fails this obligation.

The current VFSS PS return type contains only local secret shares and counters.
The P2 package has no input-dependent data and no declared public-list/GRank
mask correlation. This is a functionality and preprocessing gap, not a missing
counter or label.

## Verdict

No paper-exact leakage approval is granted. The current implementation remains
the C-level 4-round core / 8-round total path. `agarwal_protocol_i_exact`,
`agarwal_protocol_i_exact_mask_output`, `paper_3_round_exact`, and
`secure_shuffle_complete` remain prohibited.
