# M2 Protocol I Dealer-DPF BoundPublicMaskShuffle remediation contract

Date: 2026-09-20

## Status and evidence boundary

**Decision-complete C-INSTANTIATION; DESIGN ONLY.** This is an independent Dealer-DPF construction, not an author transcript, Chase-native behavior, paper-exact Protocol I, or official G2/G3 evidence. Identity: **Agarwal Protocol I functionality-aligned 3-round experimental candidate (NON-EXACT), using an independent Dealer-DPF shuffle construction.** Short form: **Dealer-DPF BoundPublicMaskShuffle** (functionality-aligned independent C-INSTANTIATION, NON-EXACT).

Backend design: **VALID**. Security-contract remediation: **COMPLETE**. Implementation: **NOT PRESENT**. Backend implementation tests: **NOT RUN**. Benchmarks: **NOT RUN**. Empirical communication: **NOT_MEASURED**. Implementation authorization: **NO**, pending independent documentation re-review and later explicit authorization. Strict author-exact G1, official G2/G3, and M5 runtime remain **BLOCKED**. A=`TARGET_PAPER`, B=`LOCAL_REFERENCE`, C=`PROJECT_EXTENSION`, D=`UNRESOLVED`; every concrete Dealer-DPF detail is C-INSTANTIATION, never A-DIRECT/A-DERIVED author behavior. Semantic support through `N<=2^20` does not claim practical quadratic execution at that maximum.

## Parameter, record, priority, and GRank contracts

Freeze `1<=logical_n<=1,000,000`, `1<=K<=logical_n`, `N=max(2,next_power_of_two(logical_n))`, N power-of-two, `2<=N<=2^20`, `b=log2(N)`, `1<=b<=20`, `ell'=33+b`, `34<=ell'<=53`, `p=b`, and `E=N(N-1)/2`. The old generic party-package edge cap is not inherited. `src` has exactly N unique entries in `[0,N)` and `pi(record)[i]=record[src[i]]`. Production record schema is `ell'` mathematical priority-key bits plus `p=b` mathematical original-index bits. `uint64_t` storage is never a mathematical/paper width; no generic production `p<=64` claim; high priority-key bits above `ell'` are zero.

```text
ordered_signed(raw)  = raw XOR 0x80000000
descending_code(raw) = 0xffffffff - ordered_signed(raw)
A(raw,index)         = (descending_code(raw) << b) | index
```

Smaller A is higher project priority and A values are unique. Dummies have raw `INT32_MIN`, index `logical_n..N-1`; real `INT32_MIN` has a smaller index and ranks before dummies. `A <= 2^(32+b)-1 = 2^(ell'-1)-1 < 2^(ell'-1)`: unmasked keys are in the lower half of `Z_(2^ell')`.

Frozen uCMP is strict `L_ij=[A_i<A_j]`, not the obsolete strict-greater predicate. For canonical `e=(i,j), i<j`, `alpha_e=r_A[i]-r_A[j] mod 2^ell'`; evaluate `(y_i,y_j)`, where `y_i=A_i+r_A[i]`. CmpAgg is `rank_i+=1-L_ij; rank_j+=L_ij`, yielding ascending ranks; rank 0 is highest priority. No inversion/argument swap; equality is excluded.

## Sharing, entropy, FSS, and DPF contracts

Over `Z_(2^ell') x Z_(2^b)`: uniform u, fresh uniform u0, u1=u-u0; v=pi(u), fresh v0 independent of u,u0,pi/src,r_A,DPF/DCF randomness,identifiers, then v1=v-v0; uniform r_A, fresh r_A,0, r_A,1=r_A-r_A,0. `v_b=pi(u_b)` is forbidden because it may reveal the permutation.

Linux production entropy is 256-bit `getrandom(2), flags=0`: retry EINTR; fail closed otherwise/incomplete. No timestamps, PID, random_device, fixed seed, env var, partial state. Deterministic seeds are TEST_ONLY; restart gets a new root and forks cannot reuse inherited DRBG state. HKDF-SHA-256 domains: `permutation/src`, `u/full/<field>`, `u/share0/<field>`, `v/share0/<field>`, `rA/full`, `rA/share0`, `DPF/<output-slot>`, `DCF/<left-slot>/<right-slot>`, `session-id`, `material-set-id`, `package-id/<party>`, `ledger-attempt-id/<party>`. Each DPF/DCF has a distinct seed; Fisher-Yates uses rejection sampling; production never calls public-constant `FSSBase::initPrngs()`.

Current fact: keyGenDPF/keyGenDCF use `FSSConfig::prngs[omp_get_thread_num()]` and accept no explicit PRNG. Candidate B requires dedicated P2, serialized outside-OpenMP FSS lane, thread 0, process lock, reseed `prngs[0]` immediately before every DPF/DCF with separately derived seed, and lock spanning SetSeed plus complete call; no concurrent global PRNG user/fixed seed. If impossible, only fallback is minimal explicit-PRNG overload; do not redesign FSS mathematics.

Candidate-B DPF wrapper is move-only/non-copyable/RAII and exposes no mutable/raw owning DPFKeyPack (unsafe owning-pointer semantics). Per output i: `keyGenDPF(bin=b,bout=64,idx=src[i],payload=1)`, requiring b 1..20, N=2^b 2..2^20, src[i]<N, rightShift=0, table length N. For m 1..64 tables are uint64_t; native is evalAll_reduce; m=64 native; m<64 `native & ((1ULL<<m)-1)`; never `1ULL<<64`. Semantics: `sum table[(acc+rightShift) mod 2^b]*DPFShare_party(acc) mod 2^64`; unit payload/zero shift reconstruct table[src[i]].

The sole production operation is conceptually `generate_candidate_b_material(public_config,production_entropy)`: atomically bind `src -> DPF selectors -> u -> pi(u) -> independent u/v shares` and `r_A -> independent r_A shares -> R2 masks -> ordered uCMP/DCF material`. No API independently replaces selector/payload/field selector/v/r_A/GRank material. IDs are defense-in-depth only; binding is private atomic construction.

Every Candidate-B owner of live cryptographic material is move-only and non-copyable: DPF selector owner, applicable uCMP/DCF edge owner, CandidateBPartyBlob, CandidateBPartySession, DealerDistribution, package/material aggregate owners, one-shot live-material containers, and every introduced owning key container. No implicit shallow copy of owning cryptographic memory; deserialization constructs directly into final owner; partial construction/deserialization is RAII-cleaned; moved-from objects are empty/inert; used/consumed material cannot be copied or reserialized as a new live package. Public immutable metadata/config may remain copyable. This does not redesign unrelated repository ownership.

## Security, process, runtime, ledger, serialization

Scope is one static semi-honest corruption among P0/P1/P2, no collusion. For corrupt `P_b`, where `b in {0,1}`, the view accounts for input share,u_b,v_b,r_A,b,N DPF party keys, public d,computed S_b,public y,all E uCMP/DCF party keys, comparison/rank shares, and allowed shuffled-rank disclosure. Assume multi-instance DPF and analogous DCF privacy: independently randomized keys, correlated selectors, polynomial instances, one half, fixed public shape. P2 view includes pi/src, full u/pi(u)/r_A, DPF/DCF seeds+witnesses, IDs/packages, but no online input shares,d,y,rank, or input-dependent original/selected indices during online execution. P2+P0/P2+P1 each break privacy, out of scope. Zeroization is hygiene only.

Controller initially creates only P2 offline package/status descriptors. P2 has no P0<->P1 online/input-release/result descriptors or plaintext input; writes opaque packages, closes/exits, controller verifies success; only then create P0<->P1 descriptors. P0/P1 receive read-only packages, validate, durably enter in_progress, then input release. Production controller is orchestration-only; clear controller/reconstruction TEST_ONLY.

Stages: R1_PREPARE/SEND/RECEIVE, LOCAL_DPF, R2_PREPARE/SEND/RECEIVE, LOCAL_GRANK, RANK_OPEN. R1 `d_b=x_b-u_b`, outbound frozen before peer R1 read, then d=x-u. Local DPF: `T_b[i,field]=DPFWeightedSelect(selector_b[i],d[field])`, `S_b=v_b+T_b`, so S0+S1=pi(x). R2 `q_b=S_b[key]+r_A,b`, outbound frozen before peer R2 read, then y=q0+q1=pi(key)+r_A; no protocol-data response follows R2, then LOCAL_GRANK and RANK_OPEN. `R1+R2+RANK_OPEN=3` NON-EXACT core; large frames full-duplex so incoming cannot change frozen outbound.

Ledger: `unused -> in_progress -> consumed`, key label/version/material_set_id/party. Only unused runs; before input/frame/sensitive evaluation use same-directory temp, fdatasync, atomic rename, directory fsync. Restart allows unused, permanently rejects other states. Success, exception, timeout, malformed/truncated/trailing/replay/EOF/peer exit/partial send/rank failure/crash-after-in_progress consume permanently: no rollback/retry; concurrent second use fails.

Canonical Candidate-B envelope includes magic/version/NON-EXACT label, production/TEST_ONLY flag, party/role/session/material/package IDs, config fingerprint, logical_n/N/K/b/ell'/p/schema, DPF N/edge E counts, stage/sequence, ledger/package digest, section lengths. Fixed-width big-endian, AES blocks exactly 16 bytes, no native structs/pointers/padding/host-size; check before allocation, reject truncation/trailing. Outer format binds bin/bout and party/session/material/output-slot external to raw native DPF.

## Communication, counters, and future matrix

Logical online payload: R1 `2N(ell'+p)`, R2 `2Nell'`, RANK_OPEN `2Nb`, total `4Nell'+2Np+2Nb`, not Theorem 4.1 `4N(ell'+p)+2Nb`; 2Np is structural. Report logical bits, serialized/framing bytes, per-party sends/receives, total sends only, offline material, adapter communication; no efficiency claim from online bits alone.

Counters: per party DPF keys N, dots 2N, leaf terms 2N^2, expansions 2N(N-1), AES 4N(N-1), inner bytes N(16b+40); Dealer selector pairs N/expansions 2Nb/AES 4Nb; GRank E edges/uCMP per party, DCF 2E per party/4E total, E offline pairs. Empirical values NOT_MEASURED.

Mandatory implementation matrix (tests verify implementation properties, not cryptographic multi-instance assumptions):

- DPF reconstruction; weighted evalAll_reduce differential; serialization round-trip; rightShift rejection; exact table length; m<64 reduction; m=64 path; wraparound.
- Entropy failure: injected getrandom/entropy-source failure; incomplete acquisition; production generation fails closed; no package output after entropy failure.
- TEST_ONLY: deterministic seed/material only in test targets; production loader rejects TEST_ONLY_DETERMINISTIC packages; production APIs cannot accept a test seed.
- Parameter/src negatives: logical_n out of range; K=0; K>logical_n; supplied non-power-of-two N where exact padded N is expected; N>2^20; b=0/b>20; invalid ell'/p; duplicate/out-of-range/wrong-count src; wrong DPF table length.
- Domain separation, thread isolation, fork/restart non-repetition; same selector across fields; independent v sharing; pi(record) reconstruction; TEST_ONLY d/y oracles; P0/P1 identical y.
- Canonical i<j uCMP: positive strict-less and strict-not-less; reversed r_A[j]-r_A[i] fails oracle; swapped (y_j,y_i) fails canonical oracle; obsolete strict-greater expectation fails; half-ring boundary/rejection; valid composite keys unique; r_A orientation and DPF/u/v/r_A/GRank swap failures.
- R1/R2 outbound freeze; no post-R2 protocol frame; backpressure/short-write.
- Inspect P2 descriptor set: it inherits no P0<->P1 online descriptors, input-release descriptors, online-result descriptors, clear-input descriptors, or descriptor carrying d/y/rank/output. Independent-process test proves successful P2 exit **before** P0<->P1 descriptor creation, P0/P1 session start, and input-share release.
- Concurrent double-use; partial-send consumption; crash-after-in_progress; restart rejection; success/failure reuse rejection; malformed/truncated/trailing/replay/EOF/timeout.
- Logical communication, wire accounting, offline-material accounting, and unchanged legacy 4-core/8-E2E baseline.
