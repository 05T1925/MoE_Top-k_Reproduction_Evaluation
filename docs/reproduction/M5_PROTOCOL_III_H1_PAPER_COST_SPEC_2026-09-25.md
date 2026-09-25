# M5-H1 pre-benchmark paper cost specification

Frozen before inspecting or running M5-H1 multi-scale measurements on 2026-09-25. Source: the repository CCS 2024 conference PDF, Table 1, §3.1, §4.2, Theorem 4.2 and footnote 8. This is a paper-cost specification, **not** a communication verification result.

## PAPER_DIRECT

For Protocol III's 2+1-party CmpAgg route, Theorem 4.2 assumes a field H encoding key and payload with `ell_prime = ceil(log2 L_prime)` and `ell_prime + p = ceil(log2 |H|)`. Across **both online parties**, its base (no common mask) Fselect cost is:

```
rounds = 2
online_bits = 6*n*ell_prime + 2*n*ceil(log2 n) + 4*n*p
online_compute = 2*C(n,2)*DCF.Eval[Z_Lprime,Z_n]
               + 2*n*DPF.Eval[Z_n,H]
```

Footnote 8 gives a **separate optimized** online formula `4*n*ell_prime + 2*n*ceil(log2 n) + 4*n*p` if a common mask is used for the Beaver triple and ranking gate. The current project has not implemented that optimization.

Theorem 4.2 extends Fselect to Fsort by replacing DPF.Eval with DPF.FullEval. Section 4.2 says the same n DPFs produce an n-by-n permutation matrix locally with no further offline or online communication. Thus Fsort has the same base online-bit formula and two online rounds; the routing local-compute term becomes `2*n*DPF.FullEval[Z_n,H]` across both parties. The comparison-ranking term remains unchanged. Table 1's Protocol III communication expression abbreviates widths and must be read together with the theorem assumptions.

The theorem gives offline communication as **dominated by** `2*C(n,2)*DCF.KeySize[Z_Lprime,Z_n] + 2*n*DPF.KeySize[Z_n,H] + 8*n*p` bits. This is an asymptotic/dominating expression, not a byte-exact party-bundle serialization contract.

## PAPER_DERIVED / comparison convention

`C(n,2)=n*(n-1)/2`. The theorem's communication is the sum of both parties' online **sends**; it excludes P2 offline material, local DCF/DPF evaluations, framing, metadata and result collection. For a project field with `ceil(log2|H|)=127`, an assumption-compatible substitution is `p=127-ell_prime` when the packed encoding is injective at that width. This is a parameter substitution, not a claim that the project transcript equals the authors'.

## C_INSTANTIATION and D_UNRESOLVED

The current project uses `H=F_(2^127-1)` and a padded rank domain `Z_(2^rank_bits)` rather than paper `Z_n`. Its implementation may send additional values; those will be counted without changing the paper formula. The conference paper does not give the author-exact serialized transcript or formal security proof of this project's field DPF wrapper. The input and output adapters are outside the two-round core and must be reported separately.
