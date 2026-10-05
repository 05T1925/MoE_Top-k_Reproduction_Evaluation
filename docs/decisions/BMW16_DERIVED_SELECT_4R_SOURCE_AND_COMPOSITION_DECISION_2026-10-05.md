# BMW16-derived four-round Select: source and Protocol I composition decision

Date: 2026-10-05

Status: **NO-GO for a runnable Select implementation; source audit and composition draft only**

Test-only label: **BMW16_DERIVED_SELECT_4R_TEST_ONLY** (reserved for a future repaired implementation).

Current code label: **BMW16_DERIVED_SELECT_4R_TEST_ONLY_AUDIT_NOT_IMPLEMENTATION**.

## 1. Scope and decision

The user changed the current research target from a BB90 reproduction attempt to a candidate derived from Braverman, Mao, and Weinberg (BMW), “Parallel Algorithms for Select and Partition with Noisy Comparisons,” arXiv:1603.04941v1. The requested objective is to derive a four-round, linear-comparison, high-probability Select algorithm and then design its possible Protocol I composition.

The source audit closes the Appendix A median-padding algebra and the two-sentinel set-intersection reduction. It does **not** close the literal Algorithm 5 / Algorithm 7 control flow. In Algorithm 5, the initialized k1=n/2 can exceed every attainable rank in A1, leaving b1 undefined. In Algorithm 7, line 11 can request a negative order statistic on the exact-partition outcome that its own first two rounds are meant to produce. These are not rounding differences. The paper’s theorem statement remains a direct paper claim; this audit does not disprove the theorem. It finds that the typeset pseudocode and a runnable finite-input implementation cannot be reconciled from the cited pages alone.

Therefore this change does not implement Algorithm 5, Algorithm 7, a Select comparison scheduler, a mask, or a secure protocol. The TEST_ONLY code provides the stable oracle and deterministic source/algebra diagnostics only. No comparison algorithm was run, no Select successes or failures were sampled, and no comparison totals were measured. A repair requires a separately reviewed derivation and proof; it must not be presented as paper-direct.

This scoped direction does not rename BB90 evidence, alter E20/E21 or PR #28, change the frozen baseline, or authorize a production Protocol I change. The older M6A-before-M6B formal performance sequence remains a historical/formal-plan gate; this source audit is research/design work and does not start the V4 performance matrix.

## 2. Sources and evidence identity

| File | Identity / role | Pages | SHA-256 | Source status |
| --- | --- | ---: | --- | --- |
| Papers/1603.04941v1.pdf | Mark Braverman, Jieming Mao, S. Matthew Weinberg, arXiv:1603.04941v1, submitted 2016-03-16; primary source | 44 PDF pages | F46F83CBA279F37E9E3AAA0D64B145FDFCFBC52C0C9C8B79BACEC1259CA9CA23 | Local identity verified; official record: [arXiv:1603.04941](https://arxiv.org/abs/1603.04941) |
| Papers/017.pdf | Jieming Mao, Algorithms in Strategic or Noisy Environments, Princeton dissertation, September 2018; secondary cross-check | 249 PDF pages | 3EAE3A93F12AB71EEA8AC040167F21351FCF3FF54DC423985A04087E3E90CB15 | Metadata/title page verified; Chapter 6 restates the high-level four-round result as Theorem 6.4.4, but it is a related-author dissertation and not independent proof. It does not contain the same numbered Algorithm 5/7 or Appendix A Lemma 1 text in the searched PDF text. |

PDF page numbers below are one-based physical PDF pages. The paper’s printed page number is shown separately where useful. The PDFs remain local references and are not committed.

## 3. Comparison model and source notation

The primary paper’s model is a strict ground-truth total order and pairwise comparison queries. A noiseless query returns the true order. Round complexity counts adaptive batches, with each batch chosen before that batch’s results are available. In the paper’s notation, A is the accepted/top side of a median Partition and R is the rejected/bottom side; Select returns the element at the target rank. See PDF p.7 (printed p.6), Section 2, and PDF pp.10–11 (printed pp.9–10), Section 4.1 and Theorem 8.

The paper says its inputs are a priori indistinguishable or are randomly permuted before comparisons. For this project, any comparison-slot permutation must carry original_index with its record; the index used for stable ties is not the permuted slot.

The theorem’s direct statement is: for 0 < ε < 1/18, Algorithm 7 uses O(n) comparisons in four rounds and outputs an exact median Partition with probability at least 1 − exp(−Ω(n^ε)). Algorithm 7 line 1 instead prints ε ∈ [0,1/18]. The endpoint is not covered by Theorem 8; this project uses only the open interval, for example ε=1/36. The asymptotic Ω constant and a finite-n numerical failure bound are not supplied here.

The paper’s result is for **Partition**, not a direct unique-Select return. Algorithm 7 labels a partition. A Select result is obtained by a reduction and must be reported separately.

## 4. Source and derivation table

| Item | Evidence class | Source location | Paper statement / project interpretation |
| --- | --- | --- | --- |
| Noiseless strict-order pairwise comparison model and adaptive rounds | PAPER_DIRECT | PDF p.7, Section 2 | Comparisons reveal the true strict order; comparisons in a round are one batch. Project scores need a strict stable composite order. |
| Algorithm 5 | PAPER_DIRECT | PDF p.29, printed p.28, lines 1–15 | Two-round nested-skeleton method. At r=1, it samples S1 and T1, compares S1×T1 in round 1, chooses a1,b1, then compares each item to a selected x in round 2. |
| Algorithm 5 line 4 versus line 11 | UNRESOLVED | PDF p.29, lines 4, 6–7, 11 | Line 4 literally initializes k0=k1=n/2; at r=1, A1=S1, whose size is about n^(2/3). Line 11 then requires a pivot beating at least k1 elements of A1. For the concrete even size n=348, the audit uses ceil for the sample size, so |A1|=50, k1=174, and no such b1 exists. The nearby prose on PDF p.10 describes finding pivots around the median of S1, suggesting a likely intended value, but does not authorize a unique correction to the pseudocode. |
| Theorem 8 | PAPER_DIRECT | PDF p.11, printed p.10 | Four rounds, O(n) queries, exact median Partition with probability at least 1−exp(−Ω(n^ε)), for 0<ε<1/18. This theorem claim is not a finite-n algorithm listing. |
| Algorithm 7 | PAPER_DIRECT | PDF p.34, printed p.33, lines 1–20 | Uses t=n^(2/3+ε), two parallel Algorithm 5 calls with lower/upper dummy items in rounds 1–2, then U, a sampled V and V×U in round 3, followed by a complete comparison on W in round 4. Its outputs are Accept/Reject labels. |
| Algorithm 7 line 11 rank | UNRESOLVED | PDF p.34, line 11 | It uses n/2−|R(2)|. For an even base size m=254, D=ceil(m^(2/3+1/36))=47, and exact median outcomes from the first two padded partitions, the real-element cardinalities are |R1|=80, |A1|=174, |A2|=80, |R2|=174; U=R2∩A1 has 94 items. The literal requested rank is 127−174=−47, while the residual rank from the rejected-prefix identity is 127−80=47=|U|/2. This is an exact cardinality witness, not a randomized execution. |
| Appendix A Lemma 1(i) | PAPER_DIRECT | PDF p.16, printed p.15 | Median-only Select/Partition reduces to an arbitrary ascending rank r: if r<n/2, add n−2r items below all real items; the symmetric high-side case follows by order reversal. The added items preserve the median-only solver’s success probability. |
| Appendix A Lemma 1(iii) | PAPER_DIRECT | PDF p.16, printed p.15 | From Partition on n+1, add one low sentinel and one high sentinel in two parallel runs; return the element accepted in the low-sentinel run and rejected in the high-sentinel run. The paper states twice the comparison count and success probability at least p². Independent algorithm coins make the two success events independent conditional on the fixed input. |
| General (n,K) padding with even median instances | PAPER_DERIVED + PROJECT_EXTENSION | Derived from Lemma 1(i),(iii) | For project Kth-highest, set the paper’s ascending rank r=n−K+1 and N=n+1. In both sentinel runs use common padding L=max(N−2r,0) below and H=max(2r−N,0) above. Then M=N+L+H=2 max(r,N−r), the median cut is h=M/2=r+L, and M≤2n. In the low-sentinel run the target is at h+1 and is accepted; in the high-sentinel run it is at h and is rejected. Their accepted/rejected intersection contains exactly the target real item. This explicit even-size convention is a project derivation; it is not text printed in the paper. |
| Stable score/index order | PROJECT_EXTENSION | Frozen project input contract | Real items compare by score descending, original_index ascending. The index stays bound to its input record and is never replaced by a post-shuffle slot. |
| Tagged dummy order | PROJECT_EXTENSION | Required to instantiate a strict total order | Use distinct tagged low dummies, real composite keys, and tagged high dummies. Tags keep legal INT32_MIN/INT32_MAX real scores from colliding with sentinels. If dummy-to-dummy comparisons are needed, give dummy identities a deterministic strict tie order. |
| Secure threshold DCF and original-order mask | PROJECT_EXTENSION / future design | Not in BMW paper | Requires an inclusive Top-K membership predicate over the selected stable key, secret threshold handling, and inverse routing if data was shuffled. No secure code is supplied here. |

### 4.1 Arbitrary-K algebra, with the project’s descending rank made explicit

Project K is one-based in the order score descending, original_index ascending. The Appendix reduction is most simply written in ascending order, so its rank is r=n−K+1 (one-based). This conversion handles ties because it is applied after defining the strict composite order.

Let N=n+1, representing the real input plus one outer sentinel in either Partition run. Set

    L = max(N - 2r, 0)
    H = max(2r - N, 0)
    M = N + L + H = 2 max(r, N-r)
    h = M/2 = r + L

In each run, add the same L low and H high reduction dummies. Add the outer low sentinel in run 1 and the outer high sentinel in run 2. On the ascending real order, the target has rank r+1+L=h+1 in run 1 and rank r+L=h in run 2. Thus the median Partition’s accepted upper side contains it in run 1, while its rejected lower side contains it in run 2. Any other real item lies on the same side of both cuts, so the real intersection is exactly the target.

| Case | Ascending rank r | Padding | Result |
| --- | ---: | --- | --- |
| K=1 (highest) | n | H=n−1, L=0 | M=2n; the greatest real item is the unique intersection. |
| K=n (lowest) | 1 | L=n−1, H=0 | M=2n; the least real item is the unique intersection. |
| K<n/2 | r=n−K+1 is on the high side | H=2r−(n+1) | M=2r; works for odd and even n. |
| even n, K=n/2 | r=n/2+1 | H=1 | M=n+2; the upper of the two central ranks is selected. |
| odd n, central K=(n+1)/2 | r=(n+1)/2 | L=H=0 | M=n+1; no half-integer median is used. |
| K>n/2 | r=n−K+1 is on the low side (except the even central boundary above) | L=(n+1)−2r when nonnegative | M=2((n+1)−r); works for odd and even n. |

For even n, K=n/2+1 gives r=n/2, L=1, and M=n; the formula covers this adjacent central rank as well. The endpoint and parity identities are exercised exhaustively for 1≤n<32 in the algebra tests. The maximum expanded median input is M=2n. This proves linear blow-up in the input size, not the missing algorithm steps.

### 4.2 Two-instance probability

Suppose a repaired median Partition instance at size M succeeds with probability at least p. Conditional on the same fixed strict-order input, run the low-sentinel and high-sentinel instances with independent algorithm tapes. Then both succeed with probability at least p²; on that event the real-item intersection above is the unique selected element. With p≥1−δ, the combined failure probability is at most 1−(1−δ)²≤2δ. This is the use of the paper’s p² claim. It does not turn the current literal pseudocode into a working instance.

### 4.3 Paper-literal algorithm skeleton

The following is a transcription in prose of the paper’s control flow, not corrected pseudocode. It records the exact operations whose finite parameters are audited above.

**Algorithm 5, PDF p.29, r=1 specialization**

1. Set S0 to the full augmented input, define Ai as the interval between the previous pivots intersected with Si, and literally initialize k0=k1=N5/2.
2. Sample S1 without replacement from S0 with nominal size N5^(2/3); sample T1 without replacement from S1 with nominal size N5^(1/3).
3. In R1 compare every element of S1 with every element of T1.
4. From those results choose a1 as the largest T1 item beating at most k1 items of A1, and b1 as the smallest T1 item beating at least k1 items of A1. Update the interval and target quantile as printed in line 12.
5. Choose an arbitrary x in [a1,b1]. In R2 compare every augmented input item to x; accept items that beat x and reject the others.

Step 4 has no b1 in the N5=348 witness. The paper’s nearby explanatory text describes bracketing the median of S1, but does not replace the printed initialization with a uniquely specified finite rule.

**Algorithm 7, PDF p.34**

1. The printed line chooses ε∈[0,1/18] and t=m^(2/3+ε).
2. In two parallel A5 calls with r=1, add 2t low dummies in call 1 and 2t high dummies in call 2. Name their accepted/rejected sets A1,R1 and A2,R2.
3. Accept A2, reject R1, and let U=R2∩A1.
4. If |U|>4t, make random decisions on U. Otherwise sample V⊂U with |V|=m/|U|; R3 compares every item in V to every item in U.
5. Use the literal line-11 target m/2−|R2| to choose x,y from V. Reject U items beaten by x; accept items that beat y; let W be the remainder.
6. If |W|>m^(1/3+3ε), make random decisions on W. Otherwise R4 compares all pairs in W, forms R* from prior rejects, and uses the literal rank m/2−|R*| to choose z and label the remaining items.

Algorithm 7 directly produces median Partition labels. For Select, apply the two-sentinel intersection reduction with the explicit arbitrary-K median padding in §4.1. With two outer instances, execute both copies’ Algorithm 7 steps concurrently in R1, R2, R3, and R4.

## 5. Conditional four-round control-flow table

This table describes the intended Algorithm 7 dependency graph and its parallel Select reduction. It is **conditional** on having a defined Algorithm 5 and repairing the Algorithm 7 rank inconsistency. Each edge set must be frozen before its round’s comparison outcomes arrive.

| Round | Information known before the round | Samples / edges for one Algorithm 7 | Results available after the round | Next dependency |
| --- | --- | --- | --- | --- |
| R1 | Public m, ε, dummy tags, and independent random tapes. No comparison results. | Each of Algorithm 7’s two Algorithm 5 invocations samples S1 and T1 without replacement; submit all S1×T1 comparisons. For the two parallel Algorithm 7 instances needed by Select, all four such bipartite graphs are fixed before any R1 result. | R1 comparisons within each A5 call. | Determine a1,b1 and A5’s selected second-round pivot x. |
| R2 | R1 results and each call’s A5 state. | Each A5 call compares its augmented input to its chosen x. The four calls for the two Select instances are sent in parallel. | A/R labels for both A5 calls in each A7 instance; then derive U=R2∩A1 and any |U|>4t branch. | If continuing, sample V⊂U; do not use R3 results to create R3 edges. |
| R3 | R2 labels, U, and an independent sample tape. | If the paper’s |U|≤4t path is taken, sample V and compare every v∈V to every u∈U. The two Select instances’ R3 graphs run in parallel. | Candidate brackets x,y, rejections/acceptances, and residual W. | Determine the |W| branch and the round-4 graph. |
| R4 | R3 results, W, and prior reject labels. | If |W|≤m^(1/3+3ε), compare each unordered pair in W once (a directed “every other” reading may request both orientations; the paper does not specify a call-count convention). Parallelize the two Select instances. | Final Partition labels, then the two-sentinel intersection yields a Select item. | Terminate. |

Algorithm 7’s |U|>4t and |W| cutoff branches make random decisions and do not add later comparison rounds. The algorithm’s maximum comparison depth is four. The present audit cannot claim a runnable four-round Select schedule because Algorithm 5 can stop before R2 and Algorithm 7’s R3 pivot rank can be undefined.

## 6. Finite-size parameters and unresolved branches

The paper uses real powers and expressions such as m/|U|; it does not specify a general integerization policy. Any rounding below marked “candidate” is only for a diagnostic calculation, not an implementation decision.

| Quantity / case | Paper text | Audit treatment / status |
| --- | --- | --- |
| ε | Algorithm 7: [0,1/18]; Theorem 8: (0,1/18) | Use theorem-valid ε=1/36 for diagnostics. Endpoint 0 and 1/18 are not claimed under Theorem 8. |
| t=m^(2/3+ε); 2t dummies | Real exponent; no rounding | Diagnostic uses D=ceil(m^(2/3+ε)), so 2D is an integer. This is a project convention only; no proof transfers the exact finite failure probability. |
| Algorithm 5 |S1|=N5^(2/3), |T1|=N5^(1/3) | Real powers; no rounding | Diagnostic uses ceilings. For N5=348, any conventional floor/ceil/nearest rule still gives a sample far smaller than literal k1=174; rounding does not fix the undefined pivot. |
| U=∅ | Then m/|U| is undefined; no special case is printed | UNRESOLVED; no silent early return or extra comparison is added. |
| m/|U| noninteger | Algorithm 7 requires |V|=m/|U| | UNRESOLVED; no floor/ceil is selected as paper-direct. |
| Requested |V|>|U| | Sampling is without replacement by ordinary subset notation, but no cap is printed | UNRESOLVED; min(|U|,ceil(m/|U|)) is one possible project rule, not proved as the paper’s rule. |
| |U|>4t | Random decisions for every item in U | Preserve as a potential probability-failure branch; it does not guarantee the exact partition. No algorithm run exercised it. |
| Missing x or y bracket pivot | Algorithm 7 assumes both exist; no failure handler is printed | UNRESOLVED; an implementation must report MISSING_R3_BRACKET, not query more edges or use an oracle. |
| |W|>m^(1/3+3ε) | Random decisions for every item in W | Potential probability-failure branch. Any incorrect completed partition remains an algorithm error and must be recorded separately from an explicit reference rejection. |
| R4 rank (m/2−|R*|) missing/out of range | No integerization or range handler | UNRESOLVED; no clamp or fallback. |
| Odd original n; endpoint K | Algorithm 7 median formulas use n/2; Lemma 1 states arbitrary ranks | The derived outer padding always gives an even M and covers K=1,n, even/odd n; this does not define the internal real-power samples. |
| Dummy comparisons / collision | Algorithm 7 says dummy values are outside the real values and dummy-dummy comparisons can be arbitrary | Project uses tagged strict sentinels. Numeric INT32_MIN/INT32_MAX are not reserved. |

Two paper-literal failures are retained as distinct classifications:

1. **Undefined paper step:** no b1 exists when Algorithm 5 line 11 asks for a pivot beating at least n/2 elements of A1 even though |A1|=50 in the N5=348 witness.
2. **Paper-permitted random-decision branch:** Algorithm 7 may choose random labels for U or W; these paths can finish with an incorrect partition. If an eventual implementation returns an answer, oracle disagreement is recorded as COMPLETED_WRONG; a predeclared undefined operation is REFERENCE_REJECTED; the random branch is PAPER_RANDOM_FAILURE_PATH. These states must not be merged.

No hidden oracle, full sort, retry, or extra comparison is permitted as a repair.

## 7. Conditional comparison-cost derivation

Let m=M≤2n be the median-only input after the two-sentinel arbitrary-K reduction, and use ε∈(0,1/18). Under a repaired Algorithm 5/7 and explicit sample integerization:

| Layer / round | Conditional comparison requests | Asymptotic bound |
| --- | --- | --- |
| A5 R1 | |S1||T1| = O(N5^(2/3)N5^(1/3))=O(N5) per A5 call, where N5=m+2D=O(m) | O(m) |
| A5 R2 | At most one comparison of each augmented element to pivot x: O(N5) per A5 call | O(m) |
| A7 R3 | v·u; with a candidate v=min(u,ceil(m/u)), at most m+u, and on the theorem’s first-two-round success event u≤4D=O(m^(2/3+ε)) | O(m) conditional; paper’s fractional sample size remains unresolved |
| A7 R4 | At most w(w−1) ordered-pair requests, or w(w−1)/2 unique unordered edges, where w≤ceil(m^(1/3+3ε)) | O(m^(2/3+6ε))=o(m) because ε<1/18 |
| Two A7 calls for Select | Two median Partition instances in parallel per round; constant-factor comparison work | 2Q_A7(m)=O(m)=O(n) if Theorem 8’s algorithm is realized |

At the call level, Partition→Select has four A5 invocations total: each of the two parallel A7 instances has two parallel A5 invocations. Therefore R1 contains four S1×T1 graph requests; R2 contains four A5 pivot-comparison batches; R3 and R4 each contain at most two A7 batches. Rounds are shared, not serialized.

The theorem and lemma yield an asymptotic O(n) argument after the constant-factor padding and duplication. The above concrete integer formulas are conservative **logical request bounds**, not measured unique comparisons. A repeated real-real edge across instances could be reissued or reused; there is no emitted edge list from which to count it. Dummy-related versus real-real calls, repeated calls, per-round actual counts, and finite constants are NOT_MEASURED. No equality with a BB90 per-round constant is claimed.

## 8. Stable-key and Protocol I composition draft

### 8.1 Plaintext test key

The TEST_ONLY oracle orders record i by the Python tuple (-signed_score_i, original_index_i). Here signed_score_i is the signed 32-bit Q20.12 integer; scaling by 2^-12 does not affect rank. original_index_i is the input’s original index and is unique, so the real keys form a strict total order even when all scores are equal.

For a future fixed-width design, a non-wrapping equivalent can be expressed as the pair (UINT32_MAX − (raw_uint32 XOR 0x80000000), original_index), compared lexicographically. Do not pack into 32 bits: score and index together need a wider domain. A tagged sentinel variant (LOW, tag) < (REAL, priority_pair) < (HIGH, tag) avoids collisions with legal boundary scores. This is an interface proposal, not a secure encoding.

### 8.2 Candidate secure dataflow and fields

| Stage | Candidate value / predicate | Required property and open work |
| --- | --- | --- |
| Input records | (score_share, original_index, route_identity) | Score and original index remain attached through every permutation. The index is a stable tie key, never a shuffled slot. Fix whether index is public or shared for the selected threat model. |
| BMW comparison adapter | Secret comparison of stable priority pairs | A paper comparison is one strict-order oracle call. A pairwise secure comparator may require multiple primitive comparisons or wider arithmetic; cost it explicitly. The derived graph and all branch decisions must remain hidden. |
| Selected threshold | Secret (selected_score_rank, selected_original_index) or the complete selected record | Do not open selected rank, pivots, |U|, |W|, or comparison outcomes. Specify where record routing occurs. |
| DCF membership | b_i = [priority_i ≤ selected_priority] in descending-priority representation | Define inclusive boundary, domain width, signed transform, and output share type. If the existing DCF only implements <, prove an exact non-overflowing inclusive construction; do not assume threshold+1 is safe. |
| Mask order | One shared bit for each original input index | If selection ran after a secret shuffle, route membership bits back through the inverse permutation. Count that route and share conversion. |

Protocol I is a separate protocol layer. The repository’s Protocol I core target is three online rounds; existing raw-score/application adapters have distinct histories and are not interchangeable with that core. Keep separate fields algorithm_comparison_rounds=4, protocol_I_core_online_rounds=<selected API evidence>, protocol_I_full_online_rounds=<dependency-derived>, preprocessing boundary, and output-adaptation rounds. Do not infer 4+3 or 4×3.

The secure schedule has unresolved leakage and preprocessing questions: R1 samples can be public-random, but R2 pivots depend on secret R1 results; R3 needs an oblivious sample from secret U; R4 depends on secret W; branch sizes and random-label paths can reveal secret cardinalities if exposed. A fixed padded circuit may add substantial work and has no established linear bound. Adaptive edge endpoints, endpoint routing, edge-graph leakage, P2 material for later edges, and inverse routing need a message-level Protocol I proof. No runtime, DCF call, XOR mask, or security claim is made.

## 9. Exact follow-up proof obligations

Before implementing the four-round Select reference, a reviewed derivation must:

1. Resolve Algorithm 5’s k1 so a1,b1 exist, and prove that the corrected update in line 12 tracks the target rank. Candidate readings include initializing k1 to the median rank of A1, or representing a normalized target quantile and rewriting the pivot predicates; neither is chosen here.
2. Resolve Algorithm 7 line 11. The exact-prefix identity suggests replacing |R2| by |R1|, while a different candidate is to select a center rank directly in U; each needs a proof under approximate A5 outputs, not just on exact partitions. Current proof text does not close that general case.
3. Specify finite integerization, empty U, fractional/oversized V, missing brackets, odd sizes, rank bounds, duplicate-edge accounting, and output states. Prove that the choices preserve four rounds and the theorem’s probability bound.
4. Then implement explicit edge lists and a causality checker, retain every random failure, run differential checks against an isolated oracle, and only afterward review secure composition.

Until then, the result is **NO-GO** for the requested complete BMW16-derived Select algorithm. The theorem statement, the arbitrary-K set algebra, the stable-key interface, and the conditional cost skeleton remain useful and traceable partial results.
