# M5-E Protocol III two-round Fselect candidate decision

Status: M5-E candidate on `feat/m5-protocol-iii-two-round`, based on
`main@883da9e` and the checkpoint `a768c23`. This decision does not mark the
whole M5 protocol or Theorem 4.2 reproduction complete.

## Boundary and evidence

- **PAPER_DIRECT** (Agarwal et al., CCS 2024, §3.1, §4.2, Theorem 4.2):
  CmpAgg produces secret-shared ranks; compressed routing uses a field `H`,
  nonzero field payload `z_i`, independent nonzero `s_i`, public
  `z̃_i=z_i s_i`, and DPF output payload `β_i=s_i⁻¹`. Selection is local after
  masked ranks and masked payloads are opened. Protocol III's online core has
  two causal rounds under these algebraic conditions.
- **PAPER_DERIVED**: field multiplication openings for `z_i s_i` can be
  exchanged with masked comparison inputs in round 1 because neither depends
  on the rank or DPF output. Round 2 can open the masked rank and product
  together; field DPF evaluation and aggregation need no subsequent message.
- **C-INSTANTIATION**: use the M5-D `F_(2^127−1)` field and field-output DPF
  wrapper, the project priority-key representation, a single selected target
  rank, the existing shared `protocol_i_cmpagg_eval_party`, a separate field
  multiplication triple per logical item, `Z_(2^rank_bits)` rank shares,
  44-byte canonical application message headers, and existing 48-byte framed
  transport headers. The candidate begins at already-field-shared encoded
  payloads and ends at one field share per party of the selected record.
- **D-UNRESOLVED**: the conference version does not specify this field, its
  packing, the exact transport transcript, or a proof of this field DPF
  wrapper. Its referenced full version is not substituted by this project
  design. Theorem 4.2 logical-cost matching needs independent M5-H review.

The old M3 `agarwal_protocol_iii_modular_3round` and five-round raw-score
pipeline remain unchanged. An owning two-frame transport entry makes a failed or partial exchange
terminal by destroying the moved one-shot state. The new candidate does not include secure
`Z_(2^64)`-to-field input conversion, original-order XOR Top-K mask output
conversion, Fsort, FullEval, or the common-mask optimization. The M5-F
independent P2/P0/P1 process closeout is separate.

## Frozen candidate contract

For each execution, P2 creates fresh input-independent material and delivers
only one party-local bundle to P0 and P1. Each party receives `padded_n`
priority-key additive shares in the comparison ring and `logical_n` additive
field shares of nonzero encoded `(priority key, uint64 payload)` records. The encoded record key must agree with the ranking-key shares;
this is a caller input-consistency precondition, not a conversion or
zero-knowledge consistency proof provided by this candidate.
`target_rank<k<=logical_n`; the output is one additive `H` share. A caller
needs fresh material and a distinct bound session for each target rank.
Only the test controller reconstructs the selected record or rank. The
production state machine never reconstructs either.

A full raw-score to original-order bit-mask path is not claimed to take two
rounds. The candidate core is exactly two causal online message exchanges;
its output contract intentionally differs from the frozen M3 mask contract.
