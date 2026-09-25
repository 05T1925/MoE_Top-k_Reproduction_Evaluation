# M5-G Protocol III Fsort / field FullEval decision

Status: accepted M5-G project extension on
`feat/m5-protocol-iii-two-round`, HEAD `77d4938` plus uncommitted M5-G diff.
This decision does not close M5-H or establish author-exact cost matching.

## Paper and project semantics

**PAPER_DIRECT** (Agarwal et al., CCS 2024, Figure 1 and §4.2): Fsort
outputs additive shares of the sorted key vector and correspondingly ordered
payload vector. Protocol III forms an additive-share permutation matrix using
the same per-item DPF keys, evaluating each at `masked_rank_i−target`; replacing
single-point Eval by full-domain evaluation adds local computation, with no
further offline or online communication. Theorem 4.2 states the two-round
structure under a field payload domain.

**C-INSTANTIATION:** this project packs the priority key and `uint64` payload
into one nonzero `F_(2^127−1)` record and returns `logical_n` field shares in
project priority order: score descending, original index ascending, rank zero
highest priority. The paper's order statistic is phrased as kth smallest; the
project's priority-key encoding maps that convention explicitly. Fsort uses
the existing two-round configuration with `k=logical_n,target_rank=0` as a
canonical bound header. No online frame or dealer package layout changes.

For per-item DPF key `f_{r_i,s_i^-1}`, public masked rank `m_i`, and public
masked record `z̃_i=z_i s_i`, the field FullEval vector is indexed by native
DPF input `x`. Output rank `t` uses `x=(m_i−t) mod 2^rank_bits` and local
share `Σ_i z̃_i FullEval_i[x]`. Exactly one honest item contributes to each
logical `t`. The wrapper traverses the native seed/correction tree once per
item and computes field leaves; it never casts the native `Z_(2^64)`
`evalAll` output to field elements. Native `evalAll` was separately confirmed
to put input `x` at output index `(x+rightShift) mod 2^bits`.

The full field domain has `2^rank_bits` entries. The production Fsort vector
contains only `logical_n` entries; honest CmpAgg ranks occupy `[0,n)`. For
`n=3/5`, tests verify reconstructed padded target slots 3 and 5–7 are zero.
This rank domain remains a **C-INSTANTIATION** relative to paper `Z_n`.

## Boundaries

One CmpAgg pass, one R1 exchange, one R2 exchange, and one field DPF FullEval
per item per party produce the whole sorted vector. Offline material has the
same shape as Fselect, but each one-shot bundle is consumed by one execution;
there is no per-target network loop or repeated CmpAgg. R2 receive is followed
only by local field operations. P2 remains offline and exits before online
input release in the process test.

The field wrapper follows the native DPF tree's seed/correction structure and
visits every leaf without alpha-based pruning. Party keys are separate; neither
key exposes full beta or alpha directly. This is an implementation-level
structural review, **not** a formal proof. The conference paper does not fix
the field, serialization, precise transcript or this wrapper. Secure
ring-to-field conversion, proof of consistency between comparison-key and
encoded-record shares, original-order Top-K bit-mask adaptation, common-mask
optimization, and Theorem 4.2 independent cost review remain deferred.
