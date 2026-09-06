# M2.12 priority-key-input small E2E

Implementation label: `m2_protocol_i_priority_key_input_small_e2e`.

This is a C project-engineering E2E candidate, not `agarwal_protocol_i_exact`,
`agarwal_protocol_i_exact_mask_output`, `paper_3_round_exact`, or
`secure_shuffle_complete`.

P2 creates only input-independent CmpAgg packages and exits before TEST_ONLY
priority-key shares are released. P0/P1 preprocess four distinct Permute+Share
materials, then execute forward shuffle, one masked-key CmpAgg exchange, one
controlled rank-share reveal, and fresh reverse carrier shuffle. D1 approval
for this milestone permits only shuffled `(slot, rank_P)`; score, key,
permutation, original mapping, selected index and original mask are not opened
or persisted by production code. The carrier is created only after rank reveal:
P0 holds `rank_P<K`, P1 holds zero. Reverse output is converted locally by LSB
to each party's XOR mask share.

Actual causal counters are forward shuffle 2, CmpAgg masked-open 1, rank reveal
1, reverse carrier 2, total 6. Package bytes, framed bytes, timing and formal
performance baselines are not yet emitted by this small E2E harness and remain
`NOT_MEASURED`. Non-power-of-two padding and raw score-share to priority-key
production adaptation are outside this milestone.
