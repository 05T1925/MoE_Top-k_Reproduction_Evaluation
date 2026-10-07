# BMW16 S18 — streamed KeyGen and random-access party reads

**Status:** local decision for `PROJECT_DERIVED / EXPERIMENTAL` research code.
**Base:** S17 commit `6f0ac1bfdccc27b43877104807f87e1faffb131e`.
**Branch:** `codex/m6b-i-bmw16-s18`.
**Date:** 2026-10-07.

## Decision

Continue the existing VFSS Protocol I + BMW16-derived Select + DCF path. S17's
stream store did not stream key generation: the T command first called the
full-pool `material_generate()`, and the party bundle loader materialized all
nine uCMP/DCF pools before installing a lazy lookup. S18 changes those two
points. T now generates one pair-key slot at a time, immediately seals each
party's copy into a sequential encrypted sidecar, and releases the slot before
moving to the next one. The party bundle contains only the O(n) raw/shuffle
prefix plus a compact manifest; parties authenticate and claim that shell
before network traffic, then random-read one authenticated sidecar record per
requested comparison.

This is a functional/storage decision only. The candidate remains
`Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL`.
No `secure` alias is created. The construction is not BB90 reproduction, does
not inherit BMW16 Theorem 8, and the Select's four comparison layers are not
the complete protocol's online round count.

## Frozen baseline and scope

- S17 source/base: `6f0ac1bfdccc27b43877104807f87e1faffb131e`.
- S16 decision/report and S17 decision/report were read from this tree before
  edits. S17's security conclusion was NO-GO; its stream store was only a
  serializer for an already materialized pool.
- The worktree was created from the exact S17 commit and was clean at start.
  Main desktop changes and other worktrees were left untouched.
- `VFSS-baseline/`, `Papers/`, local reference trees, E20/E21, and PR #28 are
  outside this change. No paper PDF or key material is added to Git.
- The user's authorization is for early bounded EXPERIMENTAL development. It
  does not satisfy V3, authorize the formal V4 matrix, or change six-scheme
  acceptance order.

## S17 audit and S18 source changes

The S17 T path called `protocol_i_bmw16_experimental_material_generate()`
before sealing; this retained the quadratic pool in memory. S17's party path
opened an in-memory party bundle containing those keys and only then installed
the sidecar callback. S17 therefore did not meet a streamed-KeyGen or
no-full-key-pool resident-memory contract.

S18 factors the existing generator so the legacy in-memory path and the new
streamed path use the same slot-generation logic. The legacy path remains
available for differential compatibility and retains its `n<=256` bound. The
new T API has `2<=n<=2^20`, `comparison_bits<=53`; its test-only `n=1` party
case retains the existing singleton shortcut (no pair-key pool). The stream
path generates the O(n) raw-score and shuffle material, then emits each of
`9*C(n,2)` uCMP slots per party. A generated pair-key object is held only while
the two party records are appended; it is not accumulated into a quadratic
array.

For padded size `p`, slot IDs are deterministic and injective over the fixed
manifest order: `2*p+2` raw/shuffle prefix slots, then eight Select task/layer
pools of `C(n,2)` pairs, then one membership pool of `C(n,2)` pairs. The slot
identifier encodes the phase/task/round and canonical unordered endpoints;
the uCMP key's requested comparison orientation remains a party-side
conversion, not a second slot. Cross-round and cross-task key reuse is not
introduced.

The stream v2 record has a 164-byte authenticated header and a fixed-size
encrypted record per slot. AES-256-GCM AAD binds the stream prefix and slot
metadata (session/fingerprint, n/K, padding, index and comparison widths,
party, stage/task/round, canonical endpoints and slot ID); the nonce is
derived injectively from the per-file base nonce and slot ID. The writer
checks the exact emitted slot count and exact file length, updates a whole-file
SHA-256, fsyncs the completed temporary file, publishes the final name without
replacement, and fsyncs the directory. Only after both sidecars are complete
does T publish the v4 compact shell bundle. A crash before shell publication
does not create a claimable ready bundle. Incomplete temporary data is cleaned
on ordinary exceptions; power loss can leave an ignored temporary file, which
must be quarantined/removed by the offline operator before retry.

Each party shell binds its own sidecar hash and common stream ID. The party
opens the compact shell and durably burns the bundle identity in the fixed
protected claim root before opening the network or evaluating any key. It
then checks the whole sidecar SHA using a bounded 64 KiB buffer and verifies
each consumed slot's GCM tag and requested metadata on random access. A
process-local set rejects repeated slot reads. Existing S17 one-shot scope is
preserved: it assumes one protected, non-rollbackable claim root. Copying a
bundle to a different empty store or rolling back a snapshot is outside this
contract; no administrator/snapshot rollback resistance is claimed.

The legacy memory API remains capped at n<=256 and its 512 MiB decoded-bundle
limit remains in place. The streamed T/party API no longer uses that memory
bundle shape cap, subject to the explicit `2^20` representational domain,
checked arithmetic, and host storage/resources. Only Select/membership keys
are streamed; raw-score and shuffle generation objects remain O(n) in T, and
the compact party shell retains the O(n) raw/shuffle package. Their measured
RSS is reported separately from the quadratic sidecar bytes.

## Failure and abort ordering

The stream reader rejects a wrong party/configuration, stream ID/hash,
record endpoint/round/stage/slot, malformed length, duplicate local read,
truncation, or authentication failure. A durable claim precedes sidecar
validation and all online traffic; consequently a corrupt sidecar burns that
bundle and requires fresh offline material. Once the claim is durable, crash
or restart cannot re-use that bundle within the declared claim-root scope.
Peer loss reports only the surviving party's local communication abort unless
an acknowledgement proves a joint abort. No partial mask is published on an
error.

The output is an XOR-shared bit mask in original input order. The party API
does not reconstruct score, rank, comparison bits, selected original index, or
the complete mask; only the isolated E2E oracle harness reconstructs output
shares. The success criterion remains the frozen stable-key oracle and exact
weight K.

## Security review and remaining proof gap

The only privacy claim made here is the execution/storage boundary tested by
the local multi-UID harness under the fixed-root assumption. AES-GCM and the
whole-file hash authenticate bytes and context; they do not prove that the
wrapped uCMP keys encode the mathematically intended comparison relation.
That relation depends on trusted T invoking the existing KeyGen correctly.
The local T/UID setup is not a production remote credential-registration or
T-to-party delivery system.

The concrete interfaces are `VFSS/src/moe_topk/protocol_i_ucmp.cpp`:
`ProtocolIUcmpMaterial::ProtocolIUcmpMaterial` computes
`alpha=(mask_left-mask_right) mod 2^b` and calls
`keyGenDCF(b,64,alpha,1)`, while `ProtocolIUcmpPartyMaterial::eval_strict_lt`
calls `evalDCF` twice (at `x` and `x-2^(b-1)`) and returns an arithmetic share
of strict `<`. The underlying declaration is
`VFSS/ext/FSS/include/FSS/dcf.h`; key generation is
`VFSS/ext/FSS/dcf.cpp::keyGenDCF` and evaluation is `::evalDCF`. `keyGenDCF`
draws two initial AES-PRNG blocks from `FSSConfig::prngs[omp_get_thread_num()]`
per key and deterministically expands its tree. The generator is seeded once
from OS entropy by `protocol_i_parallel_shuffle_dealer_generate`'s
`seed_fss_once`. Thus slots consume a sequential AES-PRNG stream, not
separately domain-separated per-slot seeds. S18 did not change this schedule.

The proof attempt is:

1. **H0→H1, KeyGen random source.** Replace the T-only seeded
   `FSSConfig` AES-PRNG stream by a uniform tape. The required premise is a
   multi-output PRG indistinguishability theorem for this exact cryptoTools
   AES-PRNG interface, with hidden seeds and polynomially many sequential
   outputs. The replacement must preserve the exact number/order of 128-bit
   blocks and surrounding public randomness. S18 records this as an
   assumption, not a repository-proved statement. Per-slot domain separation
   is not required if this stream-PRG premise holds.
2. **H1→H2, complete DCF pool.** Let `N=9*C(n,2)` and
   `alpha_ij=(r_i-r_j) mod 2^b` for every canonical pair and each of nine
   tasks. Replace each corrupt-party DCF key share one by one, including
   unused keys. The reduction needs DCF privacy that allows auxiliary input
   containing the other key shares, the correlated threshold-family
   description, prior opened comparison bits, adaptively selected later query
   inputs, and local Eval-share distributions conditioned on opened outputs.
   With that game, the hybrid loss could telescope at most `N` single-key
   advantages plus the H0 PRG advantage, while carrying the explicitly
   allowed opened-bit leakage. The repository ABI/implementation exposes
   KeyGen/Eval but does not state or prove this auxiliary-input,
   output-conditioned adaptive multi-key property for `keyGenDCF/evalDCF`.
   Ordinary single-key key-only privacy does not provide the conditioned
   Eval-share law needed to simulate public bits. This is the first unresolved
   hybrid: a missing theorem/interface guarantee, not a demonstrated attack
   and not a proof that such a theorem is impossible.
3. **H2→H3, shuffle and output.** Even granting H2, a simulator must combine
   actual forward-shuffle local `sigma/tau/a/e/r_share` material and messages,
   public masked operands, common real handles in both tasks, inverse-shuffle
   shares, abort coordination, and final XOR-mask shares. S18 has no such
   simulator for the actual Protocol I shuffle state machine.

The stream format changes neither `alpha_ij`, key randomness, nor Eval
semantics. It removes quadratic resident-memory use but closes none of these
hybrids. The public anonymous graph, opened comparison bits, pivots/U/V/W,
selected anonymous handle, access order, and abort category remain candidate
leakage fields, not approved leakage. No general simulator or concrete
distinguishing attack is claimed in S18. Therefore `PARTY_VIEW_SECURITY=NO-GO`;
no secure API or formal performance qualification follows.

## Capacity formula

For `n>1`, padded `p`, `b=33+log2(p)` comparison bits, and
`C=n(n-1)/2`:

```
uCMP slots/party       = 9*C
serialized key bytes   = 57 + 24*b
encrypted record bytes = serialized key bytes + 16-byte GCM tag
sidecar bytes/party    = 164 + 9*C*encrypted record bytes
```

These are checked layout calculations. They exclude shell bundle bytes,
filesystem metadata, temporary duplicate files during publication, raw/shuffle
O(n) objects, and other process memory. The preflight reports available RAM
and disk as observations, not as measured peak usage. See the S18 report for
actual n=64/128/256/n=1000 data and its evidence hashes.

## Gate

| Gate | S18 decision |
|---|---|
| Streamed offline Select/membership KeyGen | PASS for implemented bounded API; n=1000/K=80 fresh T/P0/P1 run completed |
| No quadratic uCMP key arrays in party path | PASS by source path and bounded-RSS E2E; raw/shuffle shell remains O(n) |
| Existing S4 Select behavior retained | PASS on reported TEST_ONLY frozen oracle E2E; this is not a paper theorem extension |
| Fresh material / durable claim / per-slot authentication | PASS within fixed local protected-root contract; no cross-store rollback protection |
| Protocol functionality | bounded functional evidence only; not secure acceptance |
| Joint party-view security | NO-GO: correlated adaptive full-pool DCF hybrid and shuffle composition unproved |
| Production T-to-party delivery | NOT READY: local UID/file harness only, not a deployed credential/transport setup |
| Official V4 performance / six-scheme comparison | NOT RUN / NOT READY |
