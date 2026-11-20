# BMW16-derived Select S2: r=1 repair, four-round reference, and Protocol I gate

Date: 2026-10-05
Status: **Four-round plaintext TEST_ONLY candidate implemented; secure composition is not approved.**
Implementation label: **`BMW16_DERIVED_SELECT_4R_TEST_ONLY`**.

## 1. Decision and source boundary

S2 continues S1 commit `0a0593deaccb55225566da6af3aac661814e717c` in a new local worktree and branch `codex/bmw16-bmw16-s2`. S1's NO-GO source audit and counterexamples remain unchanged. S2 adds a runnable project-derived repair of the A5 `r=1` median case and A7's residual rank, finite boundaries, and omitted threshold membership. It does not claim that the repaired pseudocode is verbatim or author-exact.

The 2016 primary source is `Papers/1603.04941v1.pdf` (44 PDF pages, SHA-256 `F46F83CBA279F37E9E3AAA0D64B145FDFBC52C0C9C8B79BACEC1259CA9CA23`). `Papers/017.pdf` (249 pages, SHA-256 `3EAE3A93F12AB71EEA8AC040167F21351FCF3FF54DC423985A04087E3E90CB15`) is Jieming Mao's 2018 dissertation and is only a related-author cross-check, not independent proof. Neither PDF is in the Git change.

The comparison model is a strict total order and noiseless pairwise comparison calls (primary PDF p.7, printed p.6, Section 2). A round is a batch whose entire edge set is fixed before any result in that round is consumed. Theorem 8 (PDF p.11, printed p.10) states four rounds, `O(n)` comparisons, and exact median Partition with probability at least `1−exp(−Ω(n^ε))` for `0<ε<1/18`. Algorithm 7 (PDF p.34, printed p.33) typesets `[0,1/18]`; this work uses `ε7=1/36`, strictly inside the theorem's open interval.

### Evidence and derivation table

| Claim or object | Evidence class | Source location | S2 decision |
|---|---|---|---|
| Noiseless strict-order pairwise comparison and adaptive batches | `PAPER_DIRECT` | PDF p.7, Section 2 | One comparison call reveals the true total-order relation; each frozen batch is one comparison round. |
| Algorithm 5 nested skeleton; `S_i`, `T_i`, `A_i`; arbitrary `x∈[a_r,b_r]` | `PAPER_DIRECT` | PDF p.10, Section 4.1; PDF p.29, printed p.28, Algorithm 5 | Implement only `r=1`, as Algorithm 7 invokes it. No general-r recursion is implemented. |
| Algorithm 5 proof that `S_i×T_i` reveals each pivot's rank within the sampled skeleton | `PAPER_DIRECT` | PDF p.29–30, printed pp.28–29, proof of Theorem 5 | Each `T_i` element is compared with every `S_i` element; at `r=1` this suffices to bracket the chosen `S1` quantile. |
| `k1` is a zero-based lower-count threshold within `S1`, not `N/2` | `PROJECT_DERIVED` | Literal line 4 and line 11, PDF p.29; median-of-`S1` description, PDF p.10 | Set `s=|S1|`, `k1=floor(s/2)`, so the one-based target in `S1` is `q1=k1+1`. |
| Outward A5 endpoint choice for the two dummy-shifted runs | `PROJECT_DERIVED` | Algorithm 5 line 14 allows arbitrary point in `[a1,b1]`, PDF p.29 | Use `a1` with low dummies and `b1` with high dummies. This preserves Algorithm 5's allowed interval and makes the cuts face away from the median. |
| Theorem 8 and its epsilon interval | `PAPER_DIRECT` | PDF p.11 | Its guarantee applies to `0<ε<1/18`; S2 does not use either endpoint. |
| Algorithm 7's two A5 calls, U/V/W stages, and random-label branches | `PAPER_DIRECT` | PDF p.34, Algorithm 7 lines 1–20 and proof below it | Both A5 calls within each A7 are concurrent; the two outer Select partitions also share the same four-round clock. |
| Residual R3 reject count `q=h−|R1_real|` | `PROJECT_DERIVED` | Algorithm 7 line 11, PDF p.34 | Correct when the low and high cut prefixes bracket `h`; source's `h−|R2|` is negative on the S1 exact-cut witness. |
| R3 sample factor 32 and dense-U all-pairs cutoff `u²≤32m` | `PROJECT_DERIVED` | Algorithm 7 line 9 and proof, PDF p.34 | Preserves four rounds and `O(m)` comparisons; raises the finite sample hit exponent. It is not the paper's sample constant. |
| Assign z itself to Reject; count only current A7 items in `R*` | `PROJECT_DERIVED` | Algorithm 7 lines 17–18, PDF p.34 | Makes the output have exactly `h` real A7-input rejects. Scratch dummies are not output bits or `R*` members. |
| Two-sentinel arbitrary-rank Select reduction | `PAPER_DIRECT` | Appendix A Lemma 1, PDF p.16, printed p.15 | Run two independent median partitions in parallel and intersect Accept(low-sentinel) with Reject(high-sentinel); success is at least `p²`. |
| Signed-Q20.12 stable-key mapping, dummy tags, original-order selected index | `PROJECT_DERIVED` | Frozen project semantics in `PROJECT.md` and `topk_oracle.h` | Real order is score descending, then original index ascending; tags put dummies outside every legal score. |
| Secret-shared stable key, adaptive graph hiding, DCF binding, inverse route | `UNRESOLVED` | Protocol I / DCF interface analysis in §8 | Design gate only; no secure implementation or security claim. |

## 2. A5 `r=1`: one rank convention and a defined pivot interval

For this proof, order every A5 input by a strict ascending relation `≺`, from lowest priority to highest. For a pivot `v`, define `lower_S(v)=|{s∈S1:s≺v}|`; this is exactly the number of sample items that `v` beats. The pivot itself contributes zero and needs no self-comparison.

Let `N` be the A5 input size. S2 fixes `s=ceil(N^(2/3))`, `p=ceil(N^(1/3))`, samples `S1` uniformly without replacement, then samples `T1⊂S1` uniformly without replacement. The code uses exact integer ceil roots. Put `k1=floor(s/2)` and one-based sample target `q1=k1+1`. This is a central sample order statistic: for `s=2j`, it is rank `j+1`; for `s=2j+1`, it is rank `j+1`.

The literal Algorithm 5 initialization says `k0=k1=N/2` even though at `r=1`, `A1=S1` and `|S1|≈N^(2/3)`. The S1 witness `N=348`, `|S1|=50` makes literal `k1=174` unattainable. The body of Section 4.1 instead says the first pivot interval brackets the median of `S1` (PDF p.10). S2 therefore interprets `k1` as the lower-count threshold in `S1`; it never compares that count with the full `N/2`.

Round 1 contains every non-self unordered edge requested by `S1×T1`. For each pivot, those outcomes determine `lower_S(v)`. Define:

```text
a1 = pivot of maximum lower_S(v) among T1 with lower_S(v) ≤ k1
b1 = pivot of minimum lower_S(v) among T1 with lower_S(v) ≥ k1
```

If the target pivot is in `T1`, then `a1=b1` at its exact sample rank. Otherwise, if both sides exist, `a1≺q1≺b1`. The interval endpoints have known relative ranks because every `T1` pivot is also in `S1`, so its rank comparison with the other pivots is already present in the R1 batch. If either side is absent, S2 reports `UNDEFINED/INVALID_FINITE_CASE / A5_R1_MISSING_ONE_SIDE_OF_S1_TARGET_BRACKET`; it does not add a sentinel, retry, or ask another comparison.

Once `[a1,b1]` is fixed, R2 compares every A5 input element once against the chosen `x`. Algorithm 5 permits any `x∈[a1,b1]`. The project chooses `a1` in the low-dummy call and `b1` in the high-dummy call. This outward choice does not require an extra comparison. Theorem 5's `r=1` analysis (PDF pp.10, 29–30) applies to a point from that interval and bounds the cut's distance from the augmented median with high probability.

For S2's proof, Algorithm 7 uses `ε7=1/36`, while the A5 Theorem 5 slack is fixed at `η5=1/72`. Ceil sample sizes remain within constant factors of `N^(2/3)` and `N^(1/3)`, so the hypergeometric/Chernoff exponents and `O(N^(2/3+η5))` cut-error order are unchanged. This establishes an asymptotic bound, not an exact finite-n failure probability or a numeric minimum n; the paper itself provides no such constant.

## 3. A7 R1/R2 cut invariants and R3 target rank

Let the current A7 input size be even `m`, `h=m/2`, and `D=ceil(m^(25/36))`. The two inner A5 inputs each contain the same `m` current items plus `2D` scratch dummies: low dummies in the first call, high dummies in the second. Scratch dummies use tagged kinds and never collide with signed score values.

Restrict A5 outputs to the current A7 items. Let `c1=|R1_real|` be the low-run rejected prefix length, `c2=|R2_real|` the high-run rejected prefix length, and `A2_real` the high-run accepted suffix. Under the A5 good event, the low-dummy run's ideal real cut is `h−D` and the high-dummy run's ideal real cut is `h+D`. Its two error radii are `e1,e2=O(m^(2/3+η5))=o(D)`. Thus for sufficiently large m:

```text
c1 = h−D ± e1 < h
c2 = h+D ± e2 > h
c1 < c2
```

Each A5 output is a threshold partition, so `R1_real` and `R2_real` are low prefixes, and `A1_real` and `A2_real` are high suffixes. When `c1≤c2`, `R1_real` and `A2_real` are disjoint and:

```text
U = R2_real ∩ A1_real = positions c1+1 through c2
|U| = c2−c1
q = h−c1        # number of the lowest U elements that must be Reject
```

The identity is independent of whether either A5 cut equals its ideal position: the cuts may be approximate, provided the observed prefixes satisfy `c1≤h≤c2`. The code checks that condition, checks for `R1_real∩A2_real`, and checks `|U|=c2−c1`. A crossed cut or target outside the observed U is retained as `PAPER_RANDOM_FAILURE_PATH`; it is not repaired with another comparison.

The literal R3 expression `h−|R2|` is incompatible with this invariant. On S1's exact-cut witness `m=254`, `D=47`, `h=127`, the real cut sizes are `c1=80`, `c2=174`, and `|U|=94`; the literal `127−174=−47` is impossible, while the residual count is `127−80=47`, inside U. The corrected formula remains valid for approximate cuts whenever `c1≤h≤c2`; if that premise fails, the program reports the failure state.

Boundary rules:

- `u=0` with `q=0` is already an exact median partition.
- `q=0` assigns all U to Accept; `q=u` assigns all U to Reject. Both are exact without R3/R4 comparisons when `u≤4D`.
- The source's `u>4t` random-label branch is applied before these endpoint shortcuts, preserving its stated branch order.
- A missing R3 left/right sample pivot is explicitly `UNDEFINED/INVALID_FINITE_CASE / A7_R3_MISSING_X_OR_Y_SAMPLE_BRACKET`.
- A direct odd-sized median A7 input is invalid; the arbitrary-K wrapper always creates even M.

## 4. R3/R4 boundaries, exact cardinality, and finite rules

The implementation fixes `ε7=1/36`, so `a=2/3+ε7=25/36` and `b=1/3+3ε7=5/12`. All root/power integerization is exact:

| Quantity | S2 finite rule | Evidence class |
|---|---|---|
| A7 pad exponent | `D=ceil(m^(25/36))`, with `2D` tagged low/high scratch elements in each A5 input | `PROJECT_DERIVED`; ceil changes size by <1 |
| A5 sample sizes | `s=min(N,ceil(N^(2/3)))`; `p=min(s,ceil(N^(1/3)))`; both samples without replacement | `PROJECT_DERIVED` integerization of Algorithm 5 |
| A5 target | `k1=floor(s/2)` lower-count threshold; target is one-based rank `k1+1` within S1 | `PROJECT_DERIVED` correction to Algorithm 5 line 4 |
| A7 U branch | if `u>4D`, fair independent label bits for U, no later comparison | `PAPER_DIRECT` branch with finite integerized threshold |
| Dense R3 | if `u²≤32m`, set `V=U`, compare every unordered pair once in R3; edge count `u(u−1)/2≤16m` | `PROJECT_DERIVED`; includes the user-proposed `u²≤m` case |
| Sampled R3 | otherwise `v=min(u,ceil(32m/u))`, uniform without replacement; compare unique non-self edges in `V×U` | `PROJECT_DERIVED`; the paper uses `m/u`, S2 multiplies by 32 |
| R3 target | `q=h−c1`, one-based count of U elements to reject; x is the highest sampled item with rank `≤q`, y the lowest sampled item with rank `>q` | `PROJECT_DERIVED`, from the prefix invariant |
| W cutoff | `B=floor(m^(5/12))`; `|W|>B` is the source random-label branch | `PROJECT_DERIVED` floor exactly matches integer `|W|>m^(5/12)` |
| R4 graph | all `w(w−1)/2` unordered W pairs, one round | `PAPER_DIRECT` comparison requirement with duplicate-edge accounting |
| R4 target | `qW=h−|R*|`, where R* counts previously rejected current A7-input items only; z is rank qW in W; z itself is assigned Reject | `PROJECT_DERIVED` completion of source's omitted equality case |

For an interior q, R3 leaves x and y in W because only elements strictly below x are rejected and only elements strictly above y are accepted. Let `r_<x` be the number of U items below x. Then `qW=q−r_<x`; x's rank is at most q and y's rank is above q, so `1≤qW≤|W|`. The all-pairs R4 result gives every W rank. Rejecting the first qW W items, including z, gives:

```text
|Reject| = |R1_real| + r_<x + qW
          = c1 + r_<x + (h−c1−r_<x)
          = h.
```

All remaining items, including the high suffix and the items above z, are Accept; exactly `m−h` are accepted. The A7 scratch dummies never enter this count or output. The outer rank-reduction padding and sentinels are current inputs to A7 and are counted there, then filtered from the final Select intersection.

The R3 factor 32 is not a sample-specific patch. When `u²>32m`, `v≤u` and `vu≤32m+u=O(m)`. On the good first-two-round event, `u≤4D=O(m^(25/36))`; a q-side window of `Θ(m^(5/12))` positions is hit with miss probability at most `exp(−vL/u)`. Since `v≥32m/u`, the exponent is `Ω(m·m^(5/12)/m^(50/36))=Ω(m^(1/36))`. When `u²≤32m`, the all-pairs path costs at most `16m` and determines q exactly. If W passes the cutoff, R4 costs at most `B²/2=O(m^(5/6))=o(m)`; if it does not, the source random-label branch ends without additional comparisons.

## 5. Four shared rounds and total comparison bound

Let `M` be the arbitrary-K median expansion, `M≤2n`, and `D=ceil(M^(25/36))`. Each of the two parallel outer A7 partitions invokes two A5 instances, for **four A5 instances total**. They all freeze R1 edges before any R1 result, then all valid pivot batches freeze R2 after R1. The two outer A7 calls freeze their R3 batches after R2 and R4 batches after R3. No same-round result is used to add another edge to that round.

| Round | Frozen work for the full Select | Worst-case comparison bound |
|---|---|---:|
| R1 | Four A5 `S1×T1` graphs, including low/high dummy calls for both outer partitions | `4·O(sp)=O(M)`; more explicitly each A5 has `sp=O(N)` |
| R2 | Up to four A5 all-input-to-pivot batches | `4·O(N)=O(M)` |
| R3 | Up to two A7 graphs: dense U all-pairs `≤16M`, or `v·u≤32M+u`; random branches issue none | `O(M)` |
| R4 | Up to two W all-pairs graphs with `w≤floor(M^(5/12))` | `O(M^(5/6))=o(M)` |

Here `N=M+2D=Θ(M)`. Including dummy edges, padding, both Select partitions, and duplicate requests gives `O(M)=O(n)` total comparison calls. The outer instances are parallel: the four A5 calls are not serialized into eight rounds. The asymptotic bound is not a BB90 comparison constant or per-round match.

The code counts calls, real-real calls, any-dummy calls, same-round duplicate calls, cross-task duplicates, repeats from earlier rounds, and unique unordered edges separately. Edge de-duplication occurs within each subproblem; repeated calls between separate subproblems or rounds are counted as real comparison calls. Self-comparisons are omitted.

## 6. Any-K conversion to a unique Select answer

The paper's Appendix A Lemma 1 converts a median-only Partition into an arbitrary-rank Select using two parallel sentinel instances. Project ranks are zero-based only at the API boundary: the requested K is the K-th highest item; internally let `r=n−K+1` be its one-based ascending rank and `N=n+1`.

```text
L = max(N−2r, 0)
H = max(2r−N, 0)
M = N+L+H = 2·max(r,N−r) ≤ 2n
h = M/2
```

Both median inputs contain the same L tagged-low and H tagged-high rank-padding items. The first also contains one tagged-low Select sentinel; the second contains one tagged-high Select sentinel. Their sizes are both M and even. In ascending order the desired real item has rank `r+L+1=h+1` in the low-sentinel run, so it is the first Accept item, and rank `r+L=h` in the high-sentinel run, so it is the last Reject item. For any other real rank s, the two membership conditions cannot both hold. Therefore:

```text
unique selected real = Accept(low-sentinel partition) ∩ Reject(high-sentinel partition)
```

This covers odd/even n and `K=1`/`K=n`. For even n, `K=n/2` uses high padding; for odd n, `K=(n+1)/2` yields no padding. `K<n/2` lies on the high-padding branch; `K>n/2` lies on the low-padding branch, with the exact middle handled by the same formula. At the endpoints, the expansion still has `M=2n` and the target ranks are `h+1` and `h`. If each median partition succeeds with probability p and their ideal algorithm tapes are independent conditional on the fixed input, Select succeeds with probability at least `p²`. S2 uses domain-separated deterministic SplitMix64 streams only for replay; this is not a cryptographic or ideal-randomness claim.

## 7. Stable project order and output contract

For a real record with signed raw Q20.12 score `s` and original position `i`, the descending project key is `(-s,i)`. An equivalent low-to-high comparison key in this TEST_ONLY program is `(s,-i)`. `original_index` remains attached to the record; it is never replaced by a sample or shuffled slot. Tagged categories order every internal scratch low dummy below all A7-current items and every scratch high dummy above them. Rank-padding and Select sentinels have independent tags outside all legal real keys, including `INT32_MIN` and `INT32_MAX`.

The only successful program result is a single `selected_original_index`. A missing A5 or R3 bracket, invalid residual rank, crossed cuts, or non-unique intersection returns an explicit non-success status. A7's two random-label branches continue exactly as independent project-defined coin labels and are marked `PAPER_RANDOM_FAILURE_PATH`. The algorithm does not call the S1 oracle, the C++ oracle, a sort routine, retry, or an extra comparison. Oracle checks run in separate processes/tools after the result is frozen.

The ideal-tape analysis gives each repaired A7 a high-probability bound with A5 slack `η5=1/72`; the resulting S2 guarantee is `1−exp(−Ω(m^(1/72)))` for median Partition, hence at least `p²` for Select. This is a weaker exponent than Theorem 8's direct statement and is presented as a project-derived proof, not a claim that the repaired construction inherits the theorem's exact exponent. Neither the paper nor this proof gives a finite-n numeric failure probability. Observed test rates below are empirical.

## 8. Protocol I design gate (no runtime)

| Stage | Candidate value/interface | Required Protocol I proof before implementation |
|---|---|---|
| Input | Signed Q20.12 arithmetic shares, public n/K, original-index metadata | Decide whether original index is public or shared under the threat model; preserve score/index binding through hidden shuffle. |
| Stable priority | Secret `(ordered_signed_score, reverse_original_index)` composite key | Prove bit width, signed transform, lexicographic comparison, and no wraparound. A shuffled slot cannot replace original index. |
| BMW comparison oracle | Secret comparison bit for a selected pair | Map every comparison edge to secure work and cost; hide pivot identities, outcomes, graph endpoints, and access pattern. |
| Adaptive control | Secret S/T, R1/R2 pivots, U/V/W membership and size branches | Give oblivious sampling/set filtering and fixed or hidden branch handling. Public U/W cardinalities or edge schedules leak input order information. |
| Offline T/material | Input-independent shares/correlations | Explain how material for R2 pivots and R3/R4 graphs can be prepared before their secret adaptive endpoints exist; bound padded material without making the O(n) claim false. |
| K-th threshold | Shared complete selected stable key or routed selected record | Prove uniqueness, selected-record routing, and consistency with the median-sentinel reduction. |
| DCF membership | Inclusive predicate `priority_i ≥ threshold` (or equivalent low-to-high `key_i≤tau`) | Bind threshold, domain, and per-record key; prove exact equality handling, one-shot material use, and no score-only tie ambiguity. Input-independent offline DCF material for a data-dependent secret threshold is unresolved. |
| Original-order mask | XOR shares of one membership bit per original index | Invert the hidden shuffle safely and count routing, conversion, and mask output work. |
| Metrics | `select_comparison_rounds=4` and separate Protocol I core/full fields | Derive full causal online rounds from the message graph; do not assume `4+3` or `4×3`. Count offline time/material, traffic, comparison edges, and output adaptation separately. |

The project's current exactness contract expects every run to return an oracle-correct mask with exactly K bits. BMW16-derived selection is probabilistic and this TEST_ONLY matrix includes retained random-failure and undefined seeds. A secure integration cannot claim the deterministic contract without either changing that contract or proving a separately authorized exact completion mechanism that preserves the claimed privacy, rounds, and costs. This issue, adaptive offline material, stable-key DCF binding, graph leakage, and inverse routing block a secure implementation. `SECURE_PROTOCOL_I_DESIGN_GATE=READY_FOR_REVIEW` only means this interface analysis is reviewable; it is not a security approval.
