# BMW16 Select S4: finite-sample four-round construction decision

Date: 2026-10-05

Status: **Project-derived TEST_ONLY specification; secure integration is conditional and not approved.**
Implementation label: **`BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R_TEST_ONLY`**.

## 1. Decision

Retain S1/S2/S3 and their failure evidence unchanged. The S2 implementation is an
informative implementation of a project interpretation of BMW16 Algorithm 5/7,
but its finite parameters do not support the requested high-success finite-size
contract at the official input sizes. We will not tune S2's hidden constants by
looking at oracle outcomes. Instead, this decision specifies a separate
four-layer, linear-comparison sample-bracketing algorithm with explicit finite
abort states. It uses BMW16's noiseless comparison model, median-to-arbitrary-rank
reduction, and four-layer/linear-comparison objective, but it is **not** Algorithm
7 and does not inherit Theorem 8's guarantee. Every new algorithmic step below is
`PROJECT_DERIVED`.

The intended secure upgrade slot is named **`Protocol I + BMW16-derived Select +
DCF`** as requested. This document does not authorize a secure implementation,
does not replace the Protocol III BB90 slot, and does not claim BB90 equivalence.

## 2. Source identity and evidence classes

Primary source: `Papers/1603.04941v1.pdf`, 44 PDF pages, SHA-256
`F46F83CBA279F37E9E3AAA0D64B145FDFCFBC52C0C9C8B79BACEC1259CA9CA23`. Cross-check
only: `Papers/017.pdf`, 249 PDF pages, SHA-256
`3EAE3A93F12AB71EEA8AC040167F21351FCF3FF54DC423985A04087E3E90CB15` (a related
author's dissertation, not an independent proof). Both files are local and
read-only; neither belongs in Git.

| Claim | Evidence class | 2016 paper location | Project interpretation |
|---|---|---|---|
| Noiseless comparison rounds and strict ordering | `PAPER_DIRECT` | PDF p.7, printed p.6, §2 | One frozen batch is one comparison layer; an edge cannot depend on a result from the same or a later layer. |
| Nested random-skeleton method and two-round A5 skeleton | `PAPER_DIRECT` | PDF p.10, printed p.9, §4.1; PDF p.29, printed p.28, Algorithm 5 | Source method and proof are reviewed, but S4 does not use literal A5 for the new implementation. |
| Theorem 5 error/probability is asymptotic with hidden constants | `PAPER_DIRECT` | PDF pp.10, 29–30, printed pp.9, 28–29, Theorem 5 proof | It does not itself certify S2's finite choices `D=ceil(M^(25/36))`, `s=ceil(N^(2/3))`, `p=ceil(N^(1/3))`. |
| Four rounds, `O(n)` comparisons, zero-error partition with probability `1−exp(−Ω(n^ε))`; Theorem 8 states `0<ε<1/18` | `PAPER_DIRECT` | PDF p.11 and pp.33–34, printed pp.10 and 32–33, Theorem 8/proof | The hidden constants do not provide a numerical finite-size rate for S2. Algorithm 7 p.34 says `ε∈[0,1/18]`; the theorem guarantee only applies on the strict open interval. |
| Algorithm 7 `U/W` random-decision branches | `PAPER_DIRECT` | PDF p.34, printed p.33, Algorithm 7 lines 6–18 | These are probability-failure branches, not exact fallbacks. |
| Algorithm 7 R3 target `n/2−|R(2)|` | `PAPER_DIRECT`, with a paper inconsistency | PDF p.34, printed p.33, Algorithm 7 line 11; compare its prefix sets and proof | Under the stated definitions, `|R(2)|` is the high-cut prefix and can exceed `n/2`; the residual rank must instead subtract the already-rejected low prefix. S2's correction is project-derived and valid only when the two cut prefixes bracket the median. |
| Median-only to arbitrary Select by two sentinel partitions | `PAPER_DIRECT` | PDF p.16, printed p.15, Appendix A Lemma 1 | Use independent ideal random tapes; success is at least `p²`. |
| S2 observed failure rates and wrong failed-branch candidate | `TEST_ONLY_OBSERVED` | S3 report §3–5 and archived S2 evidence manifest | Evidence about that revision only; not a probability proof. |
| New sample bracket algorithm, boundary sentinels, finite cutoff and probability bounds | `PROJECT_DERIVED` | This document §3–6 | Independent construction; no author-exact or Theorem 8 inheritance claim. |
| Secure adaptive graph hiding, offline material supply, DCF and inverse route | `UNPROVEN` | S3 review and S4 design gate §8 | Remains a blocker to secure code. |

S2/S3 identities preserved: S2 `691260833206533882e042ca37e8a21c7b9ecc24`,
S3 `59c75ff1ef21784bd54f266ab9bd6e631444ccae`, parent S1
`0a0593deaccb55225566da6af3aac661814e717c`. S4 starts at S3 and does not amend
those revisions.

## 3. S2 failure diagnosis

S3 re-ran and independently classified all 776 archived S2 inputs. Results were
570 `SUCCESS`, 47 `PAPER_RANDOM_FAILURE_PATH`, and 159
`UNDEFINED/INVALID_FINITE_CASE`; candidate outcomes were 571 oracle matches, 204
no-candidate results and one wrong candidate. All 570 nominal successes matched.
The 40 official-size cases had 16 successes and 24 failure-path results. One
failed branch emitted a wrong candidate for `(n=256,K=8)`, algorithm seed
`2707453233`: candidate index 244, frozen oracle index 169.

The main finite-size issue is not that the four S2 rounds were serialized. The
round scheduler has four causal batches. The issue is that the asymptotic proof's
slack was used with unit leading constants at finite sizes. Theorem 5's proof
contains constant factors in its cut-error window; with `ε=1/36`, S2 chose
`D=ceil(M^(25/36))` while the exponent gap from its A5 slack `1/72` to `1/36` is
too small to absorb those constants at `M≤1842`. Consequently the nominal
`|U|≤4D` high-probability premise was not established numerically. The S3
official matrix records 22 `U>4D` partition events, 6 `W` threshold events and
one crossed-cut event (events may overlap). S2's A5 missing-sample brackets
occurred 172 times in the complete 776-run replay, mainly among tiny effective
inputs. Those are finite sampling-failure events that require an explicit abort
contract and probability bound; they are not grounds for silently choosing a
pivot.

S2's `U` branch makes random labels and can return a wrong candidate, as the
retained mismatch shows. Its `W` branch also no longer knows an exact cardinality
partition. S2's cutoff constants and A5 error analysis did not establish a
finite rate that makes these branches acceptably rare at the required sizes.
Changing only the status label would not repair the behavior.

## 4. New median-partition algorithm (`PROJECT_DERIVED`)

Input is an even-size `M≥2` set under a strict ascending total order. Let
`h=M/2`; the output is a partition with exactly `h` lowest elements rejected.
Every input item, including dummies, has a unique internal key.

This alternative construction has no `epsilon` parameter. The PDF wording differs
at the upper-level algorithm: Algorithm 7 (PDF p.34, printed p.33) says
`epsilon∈[0,1/18]`, while Theorem 8 (PDF p.11, printed p.10) states the guarantee
for `0<epsilon<1/18`. We use the theorem's strict open interval whenever citing
its guarantee; neither endpoint is treated as covered by Theorem 8. The S4
construction does not silently use that theorem range to parameterize itself.
Valid project inputs are all `n≥1` and
`1≤K≤n`; the two-sentinel mapping below always gives an even `M≥2`. If R2
confirms the median bracket, strict `x<y` implies `U` contains both pivots, so
`|U|≥2` and `0≤q<|U|`. `U=∅`, a missing R1 pivot, or an out-of-range residual
rank is an invariant/input failure, never repaired by sorting. If the bracket
is not confirmed, the run aborts before R3. The `|U|/sqrt(M)` parameter uses
integer `ceil_sqrt_ratio`; no `M/|U|` sample rule is used, so a non-integral
`M/|U|` has no special case. The request for `|V|` is clamped by
`v=min(|U|,ceil(8|U|/sqrt(M)))`, so `|V|>|U|` is never requested and `V=U`
is the defined branch when the ceiling reaches `|U|`.

All roots and powers use integer ceilings/floors. Define:

```text
s = min(M, ceil(8 sqrt(M)))
if s = M:
    qL = h; qH = h+1
else:
    a  = ceil(4 sqrt(s))
    qL = max(1, floor((s+1)/2) - a)
    qH = min(s, ceil((s+1)/2) + a)
Ucap = min(M, ceil(8 M^(3/4)))
L = ceil(2 sqrt(M))
B = 2L+1
```

Sample `S` uniformly without replacement from the `M` input items.

| Layer | Batch frozen before comparisons | Result consumed after the layer |
|---|---|---|
| R1 | All unordered pairs in `S` | Pairwise rank each sample item; choose `x=S[qL]`, `y=S[qH]`. Abort if any pairwise ranks are not unique. The strict total order plus `qL<qH` proves `x<y`; the R1 edge between them is already counted, so no extra validation comparison is issued. |
| R2 | For every input item, compare it to `x` and `y` (omit self-edges) | Let `P={u:u<x}`, `Q={u:y<u}`, `U=[x,y]`, `c=|P|`, `q=h−c`. Abort if `c≤h< c+|U|` is false or `|U|>Ucap`. |
| R3 | Let `v=min(|U|,ceil(8|U|/sqrt(M)))`; sample `V⊆U` uniformly without replacement; compare every unordered pair in `V×U` once, omitting self-edges and duplicate `V×V` requests | Derive each sample rank in `U`. Find nearest sampled ranks `i≤q` and `j>q`; use virtual ranks 0 and `|U|+1` if a side has no sample. Form `W={u∈U:(x absent or x≤u) and (y absent or u≤y)}` from the R3 comparison rows for endpoint samples `x,y`. Abort if `|W|>B`. |
| R4 | All unordered pairs in `W` | Rank `W` exactly; reject the first `q−(i−1)` elements of `W`, including its boundary element, and accept the rest. `i=0` for the virtual lower boundary. |

The lower/upper virtual boundaries are algorithmic sentinels whose ranks are
known without a comparison. They cannot become output records. If `q=0`, label
`U` directly after R2 and leave R3/R4 empty. Under the accepted R2 bracket,
`|U|≥2` and `0≤q<|U|`; `q=|U|` is unreachable and is classified as
`COMPLETED_WRONG` if the invariant is ever violated. An R2/R3 sampling cutoff
returns no candidate with `PROJECT_RANDOM_FAILURE_PATH`. An impossible sample
rank or cardinality invariant is `COMPLETED_WRONG`. There is no retry, extra
edge, oracle lookup, full-input sort, or random label substitution.

Both median partitions for the two-sentinel reduction create their samples
independently before R1 and join each shared R1/R2/R3/R4 batch. The selector's
public comparison-depth bound is four; a branch may stop early with an explicit
abort or exact endpoint case.

## 5. Correctness and finite probability bound

### 5.1 R1/R2 bracket

For any fixed strict order on `M` items, let `H` be the number of sampled items
among the lowest `h=M/2` input items. Under ideal uniform sampling,
`H~Hypergeometric(M,h,s)`. The median cut lies between sample order statistics
`qL` and `qH` exactly when `qL≤H<qH`. Therefore the exact finite R1 bracket
failure probability is:

```text
δbracket(M) = Σ_{j<qL or j≥qH} C(h,j) C(M−h,s−j) / C(M,s).
```

Conditional on this event's complement, `c≤h<c+|U|`, so `0≤q<|U|` and
R2 leaves the cut in the inclusive interval `U`. The endpoint `q=0` is resolved
exactly after R2; otherwise `1≤q<|U|` and R3/R4 find the residual rank.

### 5.2 R2 window cap

If `M≤4096`, `Ucap=M`, so the cap cannot fail. For `M>4096`, an event
`|U|>8M^(3/4)` implies some fixed-start interval of `ceil(8M^(3/4))` ranks
contains at most `d=qH−qL` sample elements. Its hypergeometric count has mean
`μ=s·ceil(8M^(3/4))/M`; a union bound over at most `M` starts and the hypergeometric
lower-tail Chernoff bound give the explicit conservative bound
`δcap≤M·exp(−(μ−d)^2/(2μ))` when `d<μ`, otherwise use the trivial bound 1.
For `M>4096` with the constants above, `μ≥64M^(1/4)` and
`d≤24M^(1/4)`, hence `δcap≤M·exp(−12M^(1/4))`. The exact rank-gap probability
may be smaller; the conservative bound is the theorem used for the asymptotic
claim.

### 5.3 R3/R4 residual window

Condition on any successful R2 state with residual size `u` and target count
`q`. If `q≥L`, a fixed interval of `L` ranks immediately below the cut is hit
by `V` except with probability at most `exp(−vL/u)`; if `u−q≥L`, the same
bound applies above the cut. A side shorter than `L` is covered by its virtual
boundary and contributes at most `L` items itself. Since
`v=min(u,ceil(8u/sqrt(M)))`, `vL/u≥16` whenever a full interval is needed.
Thus `|W|≤2L+1=B` except with conditional probability at most
`δwindow=2exp(−16)`. The bound is uniform in the fixed order and in `q`.

When all stated events hold, R4 gives exact ranks in `W`. The number rejected is
`c+(i−1)+(q−(i−1))=h`, with `i−1` interpreted as zero at the lower virtual
boundary. Hence the partition has exactly `h` real-and-padding items rejected;
the virtual R3 boundaries are not counted as items.

For ideal random tapes, one partition succeeds with probability at least
`pM=1−δbracket−δcap−2e^(−16)`. The two independent-sentinel reduction uses
independent ideal tapes conditional on the fixed input, so its success
probability is at least `pM²`. For the official expansion sizes `M=242,254,498,
510,1842`, `Ucap=M` and the exact hypergeometric values for `δbracket` are
respectively `1.39e−34, 6.20e−35, 4.27e−25, 1.21e−24, 5.44e−20`; therefore the
conservative Select failure bound is less than `4.51e−7` (the R3 term dominates).
These are mathematical ideal-sampling bounds, not empirical PRNG guarantees.
The implementation's seeded Python PRNG is for reproducible TEST_ONLY runs and
is not cryptographic randomness.

## 6. Cost and stable Kth mapping

For one median partition:

| Layer | Worst-case calls | Reason |
|---|---:|---|
| R1 | `C(s,2)≤42M` | `s≤8sqrt(M)+1`; when `s=M`, `M<66` and `C(M,2)≤33M`. |
| R2 | `2M` | Two pivots per item, self-comparisons omitted. |
| R3 | `≤2048M` | For `M≤4096`, `C(U,2)≤2048M`; for `M>4096`, `U≤ceil(8M^(3/4))≤9M^(3/4)` or abort, so `vU≤8U²/sqrt(M)+U≤657M`. |
| R4 | `≤(4sqrt(M)+3)^2/2≤25M` | Reached only when `|W|≤B`. |

The two independent median instances are scheduled on the same clock. Their
combined per-layer bounds are R1 `≤84M`, R2 `≤4M`, R3 `≤4096M`, and R4 `≤50M`;
therefore the combined total is at most `4234M` comparison calls under these
conservative integer bounds. The Appendix A expansion has
`M=2·max(r,n+1−r)≤2n`, so Select uses at most `8468n` calls. Repeated unordered
edges across layers or sentinel instances are counted again as actual calls;
they can only make the unique-edge count smaller. This is a linear bound with a
large project constant, not a BB90 constant and not an offline-material bound.

For top-K API semantics, `r=n−K+1` is the one-based ascending rank. Let
`N=n+1`, `Lpad=max(N−2r,0)`, `Hpad=max(2r−N,0)`, and
`M=N+Lpad+Hpad=2·max(r,N−r)`. Each median instance has the same `Lpad` items
below all real records and `Hpad` items above them. The low-sentinel instance
adds a sentinel below every real record; the high-sentinel instance adds a
sentinel above every real record. The target real record has rank `h+1` in the
low-sentinel instance and rank `h` in the high-sentinel instance. The unique
result is `Accept_low ∩ Reject_high`.

In either padding regime, `h−Lpad=r`. In the low-sentinel instance a real
record is accepted exactly when its ascending real rank `r'` satisfies
`Lpad+1+r'>h`, equivalently `r'≥r`; in the high-sentinel instance it is
rejected exactly when `Lpad+r'≤h`, equivalently `r'≤r`. Their real-record
intersection is therefore the unique real record of rank `r`.

The padding cases follow directly from the sign of `N−2r=2K−n−1`:

| Input/rank case | Padding and expanded size | Median target ranks |
|---|---|---|
| `2K<n+1` (including `K<n/2`; even `K=n/2` has one high pad) | `Lpad=0`, `Hpad=n+1−2K`, `M=2r`, `h=r` | Low-sentinel target `h+1`; high-sentinel target `h` |
| `2K=n+1` (odd `n`, `K=(n+1)/2`) | No pads, `M=n+1`, `h=r` | `h+1` and `h` |
| `2K>n+1` (including `K>n/2`; for odd n this starts above the center) | `Lpad=2K−n−1`, `Hpad=0`, `M=2K`, `h=K` | `h+1` and `h` |
| `K=1` | `r=n`, `Hpad=n−1`, `M=2n` | `n+1` and `n` |
| `K=n` | `r=1`, `Lpad=n−1`, `M=2n` | `n+1` and `n` |

For even `n`, `K=n/2` falls in the first row and `K=n/2+1` in the third.
For odd `n`, the central legal rank is the second row; there is no integer
`K=n/2`. These equations include both endpoints and keep `M` even for every
valid `n≥1, 1≤K≤n`.

Real records are ordered low-to-high by `(signed_score, −original_index)`, which
is the reverse of the project's selected priority `(score descending,
original_index ascending)`. Distinct category tags put all padding and sentinels
outside real keys, including `INT32_MIN` and `INT32_MAX`. A shuffled slot never
replaces `original_index`. This is a plaintext TEST_ONLY encoding; secret
sharing and secure lexicographic comparison remain separate design questions.

## 7. Scope and status

This new construction is project-derived and separate from the S2 code path. It
does not alter the historical S1/S2/S3 conclusions. TEST_ONLY experiments and
finite-size confidence intervals are recorded in the S4 validation report.
Observed rates validate only the frozen sampled seeds. The finite probability
bound above assumes ideal independent uniform samples. Any different finite
parameter or algorithm requires a new dated decision and seed freeze.

## 8. Protocol I integration interface and design gate

| Stage | Candidate input/output | Required comparison and order | Open secure dependency |
|---|---|---|---|
| Raw score | Existing signed Q20.12 arithmetic-share input `(score_i)` | Preserve signed two's-complement order | Share representation/conversion into a comparator domain must be identified for the actual Protocol I API. |
| Stable priority key | Secret-shared tagged tuple `(score descending, original_index ascending)`; proposed priority-ascending key is `(~sign_biased_score, original_index)` | One key for every real record; dummy/sentinel tags remain outside all legal score/index tuples | Sign-bit conversion, tuple comparison width, masking of the original index, and carry through the shuffle are unproved. The original index is metadata, never the shuffled slot. |
| Four Select layers | Each `Rj` consumes a batch of key comparisons; its next edge set depends on prior results | Four is only the TEST_ONLY comparison depth | Publishing samples, pivots, `U`, `V`, `W`, endpoint IDs, failure flags, or edge addresses leaks input-dependent structure. Hiding/adapting the graph while supplying one-shot material remains unresolved. |
| Selected threshold | Two-sentinel partition intersection identifies one selected real record/key | The selected key is in project priority order; do not publish selected index | The shared selected key must remain bound to the original record through shuffle and satisfy the protocol's output-share contract. |
| DCF membership | For each shuffled record, evaluate `priority_key(record) ≤ selected_priority_key` in the stated high-priority-first order | Exactly K membership bits if the selected threshold is correct | The threshold is online and input-dependent. Input-independent offline DCF material, key generation without an online dealer, one-time assignment to all records, and threshold/key binding have no construction or proof here. |
| Original-order mask | Inverse-route the shuffled XOR membership shares to the source order | Preserve one bit per original input row | Requires the exact hidden permutation/inverse correlation and proof that index ties, payload, and mask stay aligned. |

The offline T must not learn the input, future pivots, or the adaptive edge
sequence, and must be silent online. A generic pre-generated per-edge key list
cannot be assigned to secret, future-dependent edge identities without either
revealing that access pattern or defining an oblivious addressing protocol and
its extra material. Any such construction must prove one-time use and the joint
`T/P0/P1` view, including abort paths; online Dealer repair is excluded.

The TEST_ONLY ideal-sampling algorithm aborts with probability at most
`4.51e−7` on the official expanded sizes; it does not emit a wrong candidate on
the proved-success path. The repository's current product acceptance contract
expects exact K and oracle-correct output on every invocation. A probability of
abort is therefore still a contract mismatch. A retry, deterministic exact
fallback, or a changed probabilistic API contract would each change round,
material, or cost/security claims and is not authorized by this design.

Separate fields for any future design are: plaintext comparison depth `4`;
secure full-entry online rounds `NOT_MEASURED`; offline T time and material
`NOT_MEASURED`; online bytes per party and total `NOT_MEASURED`; comparison edge
calls by layer `NOT_MEASURED` until a secure graph schedule exists; DCF/PRG
counts `NOT_MEASURED`; and inverse-route/mask cost `NOT_MEASURED`. The four
plaintext layers do not establish a four-round Protocol I entry.

**Gate: `NO-GO_FOR_SECURE_IMPLEMENTATION` / design draft documented, independent
review still required.** The adaptive graph, offline material, stable-key
comparator, online threshold DCF, exact-failure contract, and inverse route
remain unresolved. No production `VFSS/` source changes are part of this S4
branch.
