# M2 Protocol I Route A Rank-Reveal Mask Output

Status: `PROJECT_EXTENSION`; paper-exact gate remains `BLOCKED / NOT_VERIFIED`.

Priority-key label: `m2_protocol_i_dealer_preprocessed_rank_reveal_6round_mask_output`.

Route A composes the validated candidate without altering its three-round core:

| Stage | Online action | Opened value | Barrier |
|---|---|---|---:|
| O0 | P2 distributes candidate and fresh reverse PS material, then exits | none | 0 |
| R1/R2/R3 | candidate forward PS, forward PS, masked-list opening | public shuffled `y` | 3 |
| R4 | framed P0/P1 rank-share exchange | public shuffled rank | 1 |
| Local | both parties derive public `rank < K`; P0 uses arithmetic carrier bit and P1 zero | public shuffled carrier | 0 |
| R5/R6 | fresh role-swapped inverse PS calls | none | 2 |

The rank reveal is deliberately an additional Route-A leakage: both P0 and P1 learn the shuffled-domain rank permutation and carrier, but not scores, priority keys, original indices, either local permutation, selected original indices, or original-order mask. P2 does not receive online FDs or observe R4--R6.

The carrier is an additive `ProtocolIBlock192` value: P0 holds the public 0/1 value and P1 holds zero. The existing reverse PS preserves additive sharing. Taking each party's final word-0 parity is a local arithmetic-to-XOR conversion because `(a+b) mod 2` equals `(a mod 2) XOR (b mod 2)`; it introduces no communication barrier.

The public rank must be a permutation over `0..padded_n-1`; malformed/replayed/truncated rank frames, wrong framing identity, out-of-range or duplicate ranks, material reuse, EOF and timeout fail closed. Logical output is cropped to `logical_n`; dummy slots remain inside candidate/reverse domains but never appear in output.

This is a six-round priority-key project path (`3+1+2`). The existing raw-score adapter is a separate two-round project component; a future raw-score integration must emit a new eight-round Route-A label and independently validate its actual trace. This decision does not rename or modify `m2_protocol_i_raw_score_input_modular_8round_mask_output`, M3, or the three-round candidate.
