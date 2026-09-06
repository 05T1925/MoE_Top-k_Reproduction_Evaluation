# M2.14 raw Q20.12 arithmetic-share input

Implementation label: `m2_protocol_i_raw_score_input_modular_8round_mask_output`.

This is a C-level project extension, not `agarwal_protocol_i_exact`,
`agarwal_protocol_i_exact_mask_output`, or `paper_3_round_exact`.

## Contract and algebra

Each online party receives only `logical_n` `uint32_t` shares, with
`raw=(x0+x1) mod 2^32`.  No controller computes a priority key.  For each
padded slot, P0/P1 use independent, P2-generated 34-bit uCMP material:

```text
carry = [UINT32_MAX - x1 < x0]
raw_lift_0 = x0 - carry_0 * 2^32              (mod 2^34)
raw_lift_1 = x1 - carry_1 * 2^32              (mod 2^34)
sign = [INT32_MAX < raw_lift]
q_0 = INT32_MAX - raw_lift_0 + sign_0 * 2^32
q_1 =             - raw_lift_1 + sign_1 * 2^32
key_0 = (q_0 << b) + original_index
key_1 =  q_1 << b
```

Here `b=index_bits`, `W=33+b`, and every final-key operation is modulo
`2^W`.  Thus the reconstructed key is `(raw XOR 0x7fffffff)<<b + index`, the
frozen descending-score/stable-index key.  Padded slots are public legal
shares of `INT32_MIN` (P0 has `0x80000000`, P1 has zero), with dummy original
indices `logical_n..padded_n-1`. Consequently, a real `INT32_MIN` at any
logical original index precedes every padded dummy under the frozen stable
original-index tie rule.

P2 sees no raw share: it creates per-slot carry/sign mask shares and party
uCMP material, bound by package session, fingerprint, party, canonical slot,
stage, and 34 comparison bits.  Deserialization rejects wrong party, missing
or duplicate slot/stage, truncation and trailing bytes; materials are
move-only and cleared after their one evaluation.

Public values are `logical_n`, padded `n`, `K`, ring widths, frame binding and
the D1-approved shuffled `(slot, rank_P)` reveal.  Raw words, carry/sign bits,
priority keys, original mapping, selected indexes and original-order mask are
not opened in the production path; final mask reconstruction is test-only.

## Rounds and evidence

| Component | Causal rounds | Evidence |
| --- | ---: | --- |
| Agarwal Protocol-I paper core | 3 (paper statement) | A: Agarwal §3.1/§4.1/Table 1 |
| Current shuffle/CmpAgg/D1 core | 4 | C implementation evidence |
| Raw input adapter | 2 (carry then sign) | C implementation evidence |
| Reverse mask adapter | 2 | C implementation evidence |
| Current total | 8 | C message graph |

The paper's 3-round claim is not aligned with this 8-round modular path.
D1 is approved only for the controlled shuffled rank reveal; D2/D3/D4 have
implementation evidence but no paper-exact proof.  PRG calls, LAN/WAN,
per-stage timings, formal performance and repetitions are `NOT_MEASURED`.
