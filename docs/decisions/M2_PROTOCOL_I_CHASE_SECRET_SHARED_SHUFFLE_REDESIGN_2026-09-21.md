# M2 Protocol I Chase secret-shared shuffle redesign

Date: 2026-09-21

Status: **C1 CHASE CONFORMANCE COMPLETE; EXPERIMENTAL THREE-ROUND
C-INSTANTIATION IMPLEMENTED; AUTHOR-EXACT C3 REMAINS BLOCKED.**

This decision stops the independent Dealer-DPF Candidate-B route for Protocol
I. The active route follows Chase, Ghosh, and Poburinnaya's Secret-Shared
Shuffle, which Agarwal et al. cite in §2.4. DPF routing remains a Protocol III
primitive and is forbidden in the Protocol I shuffle path.

This is a paper-aligned `C-INSTANTIATION`, not an author-exact promotion. The
repository contains the 15-page CCS 2024 conference paper, not the omitted
full-version adaptation transcript. Strict G1 remains blocked.

## Repository and evidence state

The initial inventory was performed at
`dd313a95017d1d92bf1596049312329fcbfbf527`. During R0 preparation, PR #21
reverted the three Dealer-DPF documentation merges and advanced main to
`d73bb973abdba1dd38da357b2287fa47875d33eb`. This patch is now on
`docs/m2-chase-shuffle-migration` at that revision. The prior commits remain in
Git history; they are not active-tree contracts.

The only initial worktree entries were four untracked Candidate-B-only files:

```text
VFSS/include/moe_topk/protocol_i_candidate_b_dpf.h
VFSS/src/moe_topk/protocol_i_candidate_b_dpf.cpp
VFSS/src/moe_topk/protocol_i_candidate_b_kdf.cpp
VFSS/src/moe_topk/protocol_i_candidate_b_kdf_internal.h
```

They had no Git history, CMake target, tracked consumer, Protocol III
dependency, or general ABI and were removed. The local reference-code trees
`Agarwal_TopK/`, `ADSMPC/`, and `CipherGPT/` are absent. The verified papers
are `Papers/Agarwal 等 - 2024 - Secure Sorting and Selection .pdf` and
`Papers/Secret-Shared Shuffle.pdf`.

Evidence labels are non-interchangeable:

| Label | Meaning |
| --- | --- |
| `AGARWAL-DIRECT` | Statement in the target Agarwal conference paper. |
| `CHASE-DIRECT` | Definition/construction in Chase supporting literature. |
| `C-INSTANTIATION` | Project adapter, representation, preprocessing compilation, packaging, or output mapping. |
| `UNRESOLVED` | Required fact or construction not established by available evidence. |

## Paper evidence chain

1. `AGARWAL-DIRECT`: §2.4 requires secret shares of `pi(x)` and public
   `pi(x)+r`, where `r` is a set of random private masks **unknown to any
   single party**. In the `(2+1)` single-corruption model, this includes P2; it
   is not limited to online parties.
2. `AGARWAL-DIRECT`: the conference paper says the formal functionalities and
   concrete instantiation are in the full version and adapted from [21].
3. `AGARWAL-DIRECT`: [21] is Chase, Ghosh, and Poburinnaya, *Secret-Shared
   Shuffle*, ASIACRYPT 2020.
4. `AGARWAL-DIRECT`: §4.1 binds `r` to the GRank FSS gate; Theorem 4.1 states
   a three-online-round Protocol I core.
5. `CHASE-DIRECT`: §3--§6.3 supplies OPV, Share Translation,
   `(T,d)` Beneš decomposition, Permute+Share, and SecretSharedShuffle from two
   sequential role-swapped Permute+Share calls.

The chain does not supply Agarwal's concrete adaptation or party views.

## Current Chase implementation inventory

The tracked VFSS tree already contains and must reuse:

```text
VFSS/include/moe_topk/protocol_i_chosen_ot.h
VFSS/src/moe_topk/protocol_i_chosen_ot_emp.cpp
VFSS/tests/moe_topk/protocol_i_chosen_ot_conformance_test.cpp
VFSS/include/moe_topk/protocol_i_opv.h
VFSS/src/moe_topk/protocol_i_opv_emp.cpp
VFSS/tests/moe_topk/protocol_i_opv_conformance_test.cpp
VFSS/include/moe_topk/protocol_i_share_translation.h
VFSS/src/moe_topk/protocol_i_share_translation.cpp
VFSS/tests/moe_topk/protocol_i_share_translation_conformance_test.cpp
VFSS/include/moe_topk/protocol_i_permutation.h
VFSS/src/moe_topk/protocol_i_permutation.cpp
VFSS/include/moe_topk/protocol_i_benes.h
VFSS/src/moe_topk/protocol_i_benes.cpp
VFSS/include/moe_topk/protocol_i_permute_share.h
VFSS/src/moe_topk/protocol_i_permute_share.cpp
VFSS/tests/moe_topk/protocol_i_permute_share_conformance_test.cpp
VFSS/include/moe_topk/protocol_i_secret_shared_shuffle.h
VFSS/src/moe_topk/protocol_i_secret_shared_shuffle.cpp
VFSS/tests/moe_topk/protocol_i_secret_shared_shuffle_conformance_test.cpp
VFSS/include/moe_topk/protocol_i_party_package.h
VFSS/src/moe_topk/protocol_i_party_package.cpp
```

Current gaps include typed group boundaries, general Beneš shape policy,
moving input-independent `w` out of the online API, standalone forward-shuffle
oracles, separating the reverse-mask extension, and any proved P2 compilation.
No second OPV/Beneš/Permute+Share/SecretSharedShuffle stack is authorized.

## Artifact classification

`DELETE`: the four untracked Candidate-B files listed above. No tracked tests
or CMake targets exist.

`SUPERSEDE`: the NON-EXACT design, Dealer-DPF remediation contract, and
M2CBKDF1 entropy contract remain auditable in their original commits. PR #21
removed them from the active tree by reverting their merges, so there are no
current files to banner in this branch. They must not be restored as active
Protocol I contracts.

`KEEP`: `protocol_i_ucmp.*`, `protocol_i_cmpagg.*`, DCF, priority-key and
score-input adapters, transport/framing, metrics, independent-process harness,
the legacy Protocol I pipeline, Protocol III DPF routing, generic DPF, and
`VFSS-baseline/`.

`REUSE`: the concrete Chase-oriented files listed in the implementation
inventory and their existing CMake targets.

## Architecture

```text
secret input shares
  -> Chase OPV
  -> Share Translation
  -> (T,d) Beneš decomposition
  -> Permute+Share
  -> two sequential role-swapped Permute+Share calls
  -> SecretSharedShuffle shares of pi(record)
  -> Agarwal public-mask adapter (C-INSTANTIATION)
       shares of pi(record) + public pi(priority_key)+r
  -> existing uCMP/DCF CmpAgg
  -> shuffled-rank open
  -> local routing
  -> original-order secret Top-K mask adapter
```

Protocol I shuffle must not call a DPF selector, `evalAll_reduce` routing,
Dealer-DPF routing, or the Protocol III routing primitive.

## Direct components and party views

### CHASE-DIRECT

- OPV and Share Translation, including `Delta=b-pi(a)`.
- `(T,d)` representation and additive-group Permute+Share.
- In §6.3 P0 and P1 each choose their own random permutation `pi0` and `pi1`.
  P0 retains `pi0`, P1 retains `pi1`, and neither receives the composed
  permutation. P2 is absent from this party view.
- The two sequential calls produce shares of
  `(pi1 o pi0)(x0+x1)` under the original two-party static semi-honest model.

### AGARWAL-DIRECT

- `(2+1)` topology with an inputless offline P2.
- Secret shares of `pi(x)` plus public `pi(x)+r` under the same permutation.
- Full `r` is unknown to **any single party**, including P2.
- The same `r` is the secret parameter of the GRank FSS gate.
- Three online rounds are the Theorem 4.1 paper-core target.

The conference version does not reveal who chooses component permutations or
how r-private GRank material is generated.

### C-INSTANTIATION

- Q20.12, stable priority key, padding, payload, original-order mask, framing,
  serialization, one-shot material and metrics.
- Any movement of Chase correlations into P2 preprocessing.
- Any compiler in which P2 learns both `pi0` and `pi1`; this is not the
  CHASE-DIRECT party view.
- The P2-full-`r` four-round functional baseline described below. It is not
  proved to realize the AGARWAL-DIRECT modified-shuffle functionality and
  cannot unblock strict G1.

## Arithmetic group mapping

Chase's main construction uses an arbitrary additive group, not only the XOR
notation used for exposition:

```text
G_key     = Z_(2^ell')
G_payload = explicitly selected finite additive payload group
G_record  = G_key x G_payload
```

Operations are componentwise and permutation acts only on positions:

```text
Delta     = b - pi(a)
delta^(i) = a^(i+1) - b^(i)
m         = x + a^(1)
w         = fresh uniform element of G_record^N
```

`ProtocolIBlock192` is storage, not the mathematical group contract. Width
reduction and canonical representation require typed tests. Only the masked
priority-key component may be opened; payload remains secret under the same
permutation.

## Three distinct preprocessing/adaptation problems

### 1. CHASE-DIRECT preprocessing

P0/P1 select and retain `pi0/pi1`, then run the permutation-dependent OPV/ST
work. P2 does not participate and no party learns the composed permutation.

### 2. Project preprocessing compilation

A conservative candidate lets P2 distribute only permutation-independent base
correlations or seeds. P0/P1 retain their permutations and finish the
permutation-dependent offline work. This may retain P0/P1 offline interaction
and still needs a distribution/party-view proof.

A centralized compiler that lets P2 choose or learn both permutations is a
separate C-INSTANTIATION. Being inputless and offline does not turn that view
into CHASE-DIRECT evidence.

### 3. Agarwal public-mask adapter

The more conservative raw-mask candidate is:

```text
P0 samples uniform r0
P1 independently samples uniform r1
r = r0 + r1
```

P0, P1, and P2 then each lack full `r`. However, the repository has no
distributed/blind FSS KeyGen that produces the two GRank keys bound to hidden
`r` without revealing `r` to P2 or either online party. The available Agarwal
paper omits the concrete mechanism. This candidate is therefore incomplete;
ordinary centralized DCF/FSS KeyGen must not be assumed to accept secret shares
of its parameter.

The symbol `r` in Chase's ideal output `(r,pi(x)-r)` is an output share. It is
not automatically Agarwal's public-list mask with the stronger any-party and
GRank-binding contract.

## Functional four-round C-baseline

For functional and causal reference only:

1. P2 samples full `r`, then distributes `r0,r1` and r-bound GRank material.
2. Chase shuffle produces `z0+z1=pi(key)` and same-permutation payload shares.
3. P0/P1 exchange `q_b=z_b+r_b` and reconstruct
   `y=q0+q1=pi(key)+r`.
4. Both evaluate GRank and later open shuffled rank shares.

Additive sharing after P2 sampled `r` does not erase P2's knowledge. A
corrupted P2 knows full `r`, so this path is a functional C-INSTANTIATION
baseline only. It cannot satisfy the stated Agarwal any-single-party contract
or unblock strict G1.

## Causal transcript

```text
CHASE-DIRECT reference:
  P0 chooses/retains pi0; P1 chooses/retains pi1
  P0/P1 run permutation-dependent OPV/ST preprocessing

knowledge-preserving P2 candidate:
  P2 distributes only permutation-independent base correlations
  P0/P1 privately finish permutation-dependent work
  P0/P1 independently contribute r0/r1
  BLOCKED: generate r-bound GRank keys without revealing full r

functional C-baseline:
  P2 may know both permutations in a centralized compiler
  P2 knows full r and distributes shares and r-bound keys
  neither party view is author-exact evidence

R1: P1(data owner) -> P0(pi0 owner), first PS input-dependent message
R2: P0(data owner) -> P1(pi1 owner), depends on R1
R3: P0 <-> P1 exchange q_b=z_b+r_b and reconstruct public y
R4: P0 <-> P1 open shuffled rank shares; routing is local
```

Same-round outbound messages may not depend on a peer message received in that
round.

## Three-round feasibility

**BLOCKED.** The second Permute+Share output depends on the R2 data-owner
message. Returning a public masked list is therefore later, and the other party
cannot form its rank share before receiving that list. The conservative
composition has four core rounds.

A three-round claim requires the authoritative Agarwal adaptation transcript
or a separately proved fused C-INSTANTIATION that gives both parties the
same-permutation public list by the end of R2, preserves any-party `r` secrecy
and the required permutation views, and does not use DPF routing.

## Security model and knowledge contracts

```text
P0/P1: online input/output parties
P2: inputless offline dealer, silent online
adversary: static semi-honest
corruption: at most one of P0/P1/P2
collusion: excluded
```

- `pi`, CHASE-DIRECT: P0 knows `pi0`, P1 knows `pi1`, no party is given full
  `pi`; P2 is absent.
- `pi`, project compilation: P2 learning both is C-INSTANTIATION only and has
  no current author-exact claim.
- Agarwal's conference wording does not resolve whether any single party
  includes the inputless dealer during generation; this is `D-UNRESOLVED`.
- P2 knowing full `r` remains an explicit project C-INSTANTIATION assumption,
  not an author-exact party-view claim.

Inputlessness and online disconnection do not make P2's full-`r` or full-`pi`
knowledge author-exact evidence.

## Implementation stages

| Stage | Required result |
| --- | --- |
| R0 | Cleanup, evidence correction, file classification, arithmetic and knowledge contracts. |
| R1 | OPV conformance and freshness/process boundaries. |
| R2 | Share Translation correctness with `Delta=b-pi(a)`. |
| R3 | Beneš composition, disjointness and shape policy. |
| R4 | Permute+Share oracle; distinguish direct preprocessing from P2 assistance. |
| R5 | Standalone forward SecretSharedShuffle and CHASE-DIRECT permutation views. |
| R6 | Same-permutation public-mask adapter; P2-full-r path only a functional C-baseline; investigate distributed r and blind GRank KeyGen without assuming feasibility. |
| R7 | Integrate uCMP/DCF, CmpAgg, rank open and local routing without DPF shuffle routing. |
| R8 | Frozen-oracle differential and independent P2/P0/P1 E2E. |
| R9 | Account rounds, bytes, OT/OPV/ST, PRG, DCF and wall-clock; unknowns stay `NOT_MEASURED`. |

## C1/C3 checkpoint (2026-09-21)

On `feat/m2-chase-c1-c3` at base `5d3f36a5bdf5be821ba86bcd959e4c1b5adae9b1`,
R1--R5 conformance is implemented and the 15-test M2 matrix passes. Independent
clear oracles establish `pi(x)[i] = x[pi[i]]`, composition `pi1(pi0(x))`,
componentwise arithmetic in `(Z_(2^64))^3`, and direct forward-shuffle
correctness before the separate reverse adapter is invoked. The current Beneš
implementation deliberately rejects shapes where `log2(N) % log2(T) != 0`.

Fresh searches of the local Papers tree, all Git refs, the authors' publication
pages, MIT's final-version deposit, ACM metadata, IACR ePrint indexing, and
public code results found only the same 15-page conference version and no
author artifact or concrete modified-shuffle transcript. With the verified
Chase transcript, public `y=pi(x)+r` requires a post-shuffle opening before both
parties can evaluate GRank. The repository's centralized DCF KeyGen requires
the complete mask difference and supplies no distributed/blind replacement.
The result is `PATH_C = STILL_BLOCKED`; no three-round production path is
authorized by this checkpoint.

## Experimental correlated-parallel checkpoint (2026-09-23)

A senior-student construction and independent Python reference motivated an
isolated `C-INSTANTIATION`: P2 samples one hidden final permutation `Pi` and
independent factors `sigma0,sigma1`, then distributes
`tau0=Pi∘sigma1^{-1}` and `tau1=Pi∘sigma0^{-1}` with correlated additive
masks. P0/P1 simultaneously exchange their independently permuted input
shares in R1, simultaneously open the same masked shuffled list in R2, locally
run the existing CmpAgg material bound to the same key-mask projection, and
open rank shares in R3. P2 is inputless and exits before online input, but
knows full `Pi` and `r` while generating the packages.

The isolated C++ implementation and independent-process E2E establish
functional correctness, three causal rounds, stable priority-key handling,
and logical costs `4n(ell'+p)+2n ceil(log n)` online and
`6n(ell'+p)+4n ceil(log n)` non-DCF offline. Exact cost agreement is not
evidence of the omitted Agarwal transcript. A formal simulation proof and a
durable cross-process material-consumption store remain review obligations.

## Online communication benchmark (2026-09-24)

Real independent-process measurements over `n=2,4,8,16,20,32,64,128`, with
five fresh formal runs per size, found logical delta `0` and logical ratio
`1.0` against Theorem 4.1 for identical `(n,ell',p)` parameters. Application
wire bytes were measured separately and exceed the logical formula only by the
current 48-byte frames, fixed 192-bit record storage, and rank-byte packing.
See the
[online communication record](../reproduction/M2_PROTOCOL_I_3ROUND_ONLINE_COMMUNICATION_UBUNTU_2026-09-24.md).
This cost match is C-INSTANTIATION evidence, not author-exact transcript proof;
author-exact remains **NO / D-UNRESOLVED** and strict G1/G2/G3 remain blocked.

## Exactness and unresolved items

| Identity or gate | Status |
| --- | --- |
| Dealer-DPF Candidate B | **STOPPED / SUPERSEDED** |
| Chase base-stack reuse | **C1 CONFORMANCE COMPLETE (NON-EXACT)** |
| P2-full-`r` four-round path | **FUNCTIONAL C-BASELINE ONLY** |
| Correlated-parallel three-round path | **EXPERIMENTAL C-INSTANTIATION IMPLEMENTED** |
| Any-party-private `r` plus r-bound GRank keys | **BLOCKED** |
| Author-exact Theorem 4.1 transcript | **BLOCKED** |
| Strict G1/G2/G3 | **BLOCKED** |

Unresolved:

1. Agarwal full-version adaptation transcript is absent.
2. The correlated-parallel construction has correctness and single-party
   sanity tests, but not a complete simulation proof.
3. Durable cross-process/restart prevention of material reuse remains the
   material-pool owner's responsibility.
4. Distributed/blind GRank FSS KeyGen for hidden `r=r0+r1` is absent.
5. General Beneš shapes remain unsupported; the current restricted shape is
   fail-closed and covered by rejection tests.
6. Reverse original-order mask routing remains a separately counted project
   adapter.
7. New-route application-level online communication is measured; runtime,
   LAN/WAN, and full performance remain `NOT_MEASURED`.

Current implementation branch: `feat/m2-chase-c1-c3`.

`READY_FOR_ONE_FINAL_INDEPENDENT_REVIEW = YES` for C1 and the experimental
three-round C-INSTANTIATION. Author-exact C3 remains blocked on the missing
transcript and unresolved dealer/full-`r` semantics.
