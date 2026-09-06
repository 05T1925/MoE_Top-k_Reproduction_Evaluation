# M2.16 Protocol I paper-exact 3-round research record

Status: blocked feasibility/design audit; no paper-exact implementation.

Branch: `m2.16-paper-exact-3round-protocol-i`

Starting code revision: `6c72ec8a18a44c1d3d441758017b9807fe1dc090`
(`origin/main`, including the M2 closeout merge).

Current implementation label remains
`m2_protocol_i_raw_score_input_modular_8round_mask_output`.

## Evidence reviewed

- Agarwal CCS 2024 PDF, Section 2.4, Section 4.1, Theorem 4.1 and Table 1.
- Local B1 Route-B interface and two-pass reference source, used as B-level
  behavior only.
- VFSS PS, pipeline, transport, package, raw-score and existing alignment tests.
- M2.15 alignment record and M2 closeout/handoff records.

The local PDF engine extracted all 15 pages and confirmed the paper definition
`y = pi(x) + r`; Docling rejected the PDF page metadata (`15 != -1`) and Marker
was stopped during slow layout recognition. The direct local extraction was used
only to verify the cited text, not to infer missing transcript details.

## Current audit result

The paper requires a shuffle functionality that emits both secret-shared
`pi(x)` and public masked `pi(x)+r`, with `r` unknown to any single party and
available as the subsequent FSS secret parameter. Current VFSS two-pass PS emits
only party-local secret shares. Its current masked-key frame is a separate phase
after PS and cannot be reclassified as the paper shuffle output.

No new runtime/API was added. The required same-permutation public-list,
correlated `r`/GRank material, 3-round causal transcript, formal leakage proof,
and exact transport accounting remain D-level blockers.

## Round result

| Path | Result |
| --- | ---: |
| Requested paper core | 3 rounds, not achieved |
| Current paper-core-shaped graph | 4 rounds |
| Requested unified total | 7 rounds, not achieved |
| Current unified total | 8 rounds |

Current total remains:

```text
raw score adapter 2 + forward shuffle 2 + masked-key exchange 1
+ rank reveal 1 + reverse mask adapter 2 = 8
```

## Existing implementation validation carried forward

The M2 closeout validation remains the applicable C-level evidence:

- EMP-OFF fresh build: 13 discovered, 13/13 passed;
- EMP-ON fresh build: 19 discovered, 19/19 passed;
- independent E2E `(128,2)`, `(128,8)`, `(256,2)`, `(256,8)`: all exit 0;
- score-adapter communication: 4,192 bytes per party at n=128 and 8,288 bytes
  per party at n=256;
- CmpAgg edges: `n_padded*(n_padded-1)/2`; score adapter: `2*n_padded` uCMP
  and `4*n_padded` raw DCF calls per party.

These are carried-forward M2 baseline results, not M2.16 public-list evidence.
M2.16 added no code, so no new conformance, differential, or independent-process
run can establish the missing primitive. Paper-compatible public-list bytes,
correlated-mask material, PRG calls, phase timings, formal leakage proof,
LAN/WAN, repetitions, and large-scale performance are `NOT_MEASURED`.

## Decision and next step

M2.16 is complete as a feasibility/design audit and fails the exact-primitive
entry condition. Preserve the current 8-round label and let M3 proceed from the
frozen M2 contract. Any future attempt must first pass the design and leakage
obligations in the two M2.16 decision records before modifying VFSS runtime.
