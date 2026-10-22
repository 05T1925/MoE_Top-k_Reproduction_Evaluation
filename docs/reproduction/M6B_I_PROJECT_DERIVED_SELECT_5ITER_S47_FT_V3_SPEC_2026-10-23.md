# S47 F_T/v3 specification and implementation record: N=1000

Status: implemented as an explicitly opted-in experimental candidate. A
non-author read-only review of the final specification and implementation
mapping identified a fixed-retry-profile mismatch; the correction was
re-reviewed. This is not a product contract or security result.

## Fixed source and scope

- Runtime base: S46 commit `d48a41062ba2f86edbac3bcb3663692a641e95bd`, tree
  `3f70ae5999400781f37f871420a26988afa146c9`, parent
  `6fd6bed0718be28d677892ca40125d0f52323f0e`; the new S47 worktree began clean.
- S46 carries the S45 85-file source set and one S46 `TEST_ONLY` E2E target-K
  change. Its manifest's S46 delta identifies
  `VFSS/tests/moe_topk/score_to_mask_v3_process_e2e_test.cpp`; the S39 runtime
  baseline and S35/S40 source provenance remain those recorded by S46. The
  checked-out source is resolved from the exact S46 Git tree, not from another
  worktree. Text SHA-256 values in older manifests were produced under
  different checkout line-ending conventions; Git blob identity is the source
  authority where raw checkout hashes differ.
- `VFSS-baseline/` is clean and remains outside the change scope. No historic
  S35, S40, or S46 full matrix will be repeated.
- Route name remains `Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3
  / EXPERIMENTAL`. Input semantics and the original-order, stable-tie,
  exactly-K XOR mask output do not change.
- S46 `LOW_PROBABILITY_ISSUANCE_ABORT_PRODUCT_ACCEPTANCE` remains
  `OPEN_CONFIRMATION`. The candidate can be tested as an experimental
  pre-issuance failure path, but it is not accepted as a product contract.

## Preflight source facts and resource arithmetic

The S46 source currently caps `kScoreToMaskV3MaxN` at 256 and the GMW budget
at 5,281,664 gates; `score_to_mask_v3_gate_budget` rejects above either the
N/package bound. The F_T/v2 sampler caps padded `m` at 256. Party AND opening
frames are limited to 1 MiB and `GmwParty::and_layer` currently rejects a layer
that does not fit in one frame. The process E2E harness supplies a 256 MiB
party cap. These are distinct source limits, not one generic “memory limit”.

For actual `N=1000`, padding gives `m=1024`, `index_bits=10`, `W=42`, and
`pairs=1024*1023/2=523,776`. From the current gate equation

```text
G = 63m + m^2 W + pairs(2W-1) + m^2
  = 88,626,688 AND gates.
```

The current package layout is a 372-byte header plus a 64-byte Ed25519
signature, `ceil(3G/8)=33,235,008` triple-share bytes per party, and
`ceil(m^2/8)=131,072` permutation-share bytes per party. The projected package
is 33,366,516 bytes per party (66,733,032 bytes for both), excluding context,
SQLite sidecars, filesystem allocation, and temporary build/test outputs.
The schema's gate count is u64 and the material lengths are u32, so these
values fit their integer fields. The S46 2 MiB package ceiling and 5,281,664
gate ceiling do not.

Largest single layer is the forward route: `m^2 W=44,040,192` gates. With the
current 104-byte inner frame prefix, 12 bytes of explicit batch/layer metadata,
4 bytes of packed-gate count and 2 bits per gate, the proposed deterministic
maximum is `floor((1,048,576-104-12-4)*4)=4,193,824` gates per frame, so route
requires 11 frames. Compare generation has 21,998,592 gates and requires 6
frames. The 41 borrow layers and inverse layer each fit one frame. Input
conversion/carry contributes 32 frames. A Beaver frame is one call to
`exchange_phase`, consumes one monotonically increasing phase, and carries one
transport frame. The fixed GMW core including READY therefore uses:

```text
READY 1 + input generate 1 + input carry 31 + route batches 11
      + compare-generate batches 6 + borrow recurrence 41 = 91 phases.
```

The Select5 control has at most nine opened-comparison waves: two each in
iterations 0–3 and one exact-rank wave in iteration 4. At most four seed
exchanges occur, since iteration 4 is exact-rank fallback. Thus Select5 adds at
most 13 phases. Inverse routing and output refresh add one phase each, so the
worst-case successful path is at most 106 phases, below the existing 255 limit
by 149 phases. Early selection may reduce this count; actual per-case counts
must still be reported. Five algorithm iterations, the maximum nine comparison
dependency waves, application exchange phases, and RTT are distinct metrics.
Batch boundaries, frame counts, and the phase pattern are externally visible
communication metadata and are recorded as an FTv3 schedule-profile change;
this work does not claim that the old leakage description already covers this
new schedule. The FTv3 schedule digest must bind the layer/batch order, 1 MiB
frame profile, phase allocation, and nine-wave/four-seed Select5 schedule.
Legacy package schedule digests must remain unchanged.

The existing `estimated_peak_bytes` expression evaluates to 872,902,644 bytes
(832.5 MiB) for N=1000. Its source terms are package (33,366,516), consumed
gate markers (88,626,688), maximum `GateInput` layer storage
(44,040,192 × `sizeof(GateInput)`; 16 bytes on the current ABI), one result
vector (44,040,192), comparator plus membership arrays (2 × 1024²), and
three key-share arrays (1024 × 42 × 3). The expression does not include all
simultaneously live buffers: notably the temporary Beaver-share vector
(conservatively 3 × 44,040,192 bytes), a second full maximum-layer result
vector (44,040,192), and one 1 MiB frame. Adding those and an explicit
allocator/nested-vector reserve of 25% of the maximum layer
(11,010,048 bytes) gives about 1,061,122,036 bytes (1,012 MiB), before
executable mappings and any allocator overhead beyond that reserve. The 25%
reserve is a planning allowance, not a proved bound. This is not measured RSS;
record actual per-process and aggregate RSS during E2E. Proposed test cap is 2
GiB per party; the host has
about 7.36 GB available RAM, 2 GiB unused swap, 1,009 GB free `/tmp`, and 82.9
GB free on `/mnt/c`, with 32 visible CPUs and no finite `ulimit -v`/file-size
limit. Do one case at a time and stop if measured aggregate pressure or an
actual process limit is reached. A reproducible server recommendation is at
least 8 GiB RAM and 10 GB free workspace, with 4 or more CPUs; this is a
resource-safety recommendation, not a performance claim.

The S47 final build/test host was WSL Ubuntu 24.04.4, OpenSSL 3.0.13,
GCC 13.3.0 and CMake 3.28.3. A preliminary attempt in Ubuntu 20.04 stopped
before configuration because its CMake was 3.16.3 (<3.17); it is not the
reported build. The selected host configuration is a build-host fact, not a
target deployment profile and not evidence for the S46 DRBG bridge.

## Candidate F_T/v3 contract (reviewed and implemented)

1. Add a separate explicit CMake opt-in
   `MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL`, default OFF,
   requiring GMW and receipt/v2. It does not imply F_T/v1 or F_T/v2 changes.
2. F_T/v3 supports actual `N<=1000`, padded `m<=1024`; F_T/v1 and F_T/v2
   remain capped at actual `N<=256` even when this option is enabled. `K`,
   score interpretation, stable tie rule, five Select iterations, and output
   are unchanged. The existing shared Select5 public API remains capped at
   256. Add FTv3-only sampler/control entry points accepting padded
   `m<=1024`; route only an FTv3 package through them. Legacy entry points
   continue rejecting `m>256`, even in a build where FTv3 is enabled.
3. Use the exact package profile tuple
   `(ft_version=3, sampler_version=2, sampler_lambda_bits=128,
   sampler_retry_limit=7, package_format_version=6)` and escrow profile
   `(context_format_version=2, receipt_format_version=2)`. These are separate
   fields and version spaces. The FTv3-only signed context format 2 carries
   the five package-profile fields above, while `receipt_format_version=2`
   remains in its existing escrow state field. F_T/v1/v2 retain context
   format 1 byte-for-byte. A context-v2 parser rejects a missing or mismatched
   package profile. The FTv3 package maximum is 64 MiB **per party package**;
   the aggregate two-package set is not bounded by that per-file limit. Keep
   a separate 2 MiB escrow ciphertext maximum: output-share envelopes are
   independent of Dealer package size, and widening the package loader must
   not widen the escrow request limit.
   Package signatures use the domain
   `ProtocolI/PROJECT_DERIVED_SELECT_5ITER/GMW-Beaver-v3/party-package/F_T-v3/bounded-rejection-v2`,
   followed by NUL, big-endian u32 package-format version 6, and the exact
   unsigned package bytes. The unsigned body contains the tuple above except
   receipt v2, which is instead in the signed escrow context. The FTv3 schedule
   hash uses a separately versioned domain
   `ProtocolI/PROJECT_DERIVED_SELECT_5ITER/GMW-Beaver-v3/gate-schedule/F_T-v3/batched-AND-frame-1MiB/v1`.
   For FTv3, compute the schedule digest as SHA-256 over the existing package
   magic, that exact UTF-8 domain, one NUL byte, the existing fixed schedule
   description, and big-endian u32 encodings of (batch gate cap=4,193,824,
   maximum frame payload=1,048,576, fixed core phase count=91, maximum
   Select5 phase count=13, inverse phase count=1, output phase count=1).
   Preserve the pre-S47 `schedule_hash()` byte-for-byte for FTv1/v2. The
   FTv3 schedule digest is checked against the context and each package before
   the lease insertion, so the signed profile and runtime batch interpretation
   agree.
   Context-v2 unsigned body field order is exactly:

   ```text
   magic[8] || BE32(context_format=2) || BE32(protocol_version=3) ||
   ft_version:u8 || sampler_version:u8 || lambda_bits:u8 || retry_limit:u8 ||
   BE32(package_format=6) || session[16] || invocation[16] ||
   BE32(N) || BE32(K) || party_identity_sha256[2][32] ||
   escrow_identity_sha256[32] || circuit_sha256[32] ||
   gate_schedule_sha256[32] || party_package_sha256[2][32] ||
   package_set_sha256[32] || signer_identity_sha256[32] ||
   BE32(wait_timeout_ms) || BE32(max_ciphertext_bytes=2 MiB) ||
   pending_restart_policy:u8 || receipt_format_version:u8 || reserved[2]=0
   ```

   `package_set_sha256` is exactly SHA-256 of
   `UTF8("ProtocolI/PROJECT_DERIVED_SELECT_5ITER/GMW-Beaver-v3/package-set/v1") ||
   0x00 || party_package_sha256[0] || party_package_sha256[1]`; it commits only
   to the two package hashes. The context signature authenticates this digest
   together with the other unsigned body fields. The complete Ed25519 signing
   input is
   `UTF8("ProtocolI/PROJECT_DERIVED_SELECT_5ITER/GMW-Beaver-v3/escrow-context") ||
   0x00 || BE32(2) || context_v2_unsigned_body`; the context file is that body
   followed by its 64-byte signature. F_T/v1/v2 context-format-1 bytes remain
   unchanged. An FT/profile/schedule/receipt tuple mismatch must be rejected
   by package/context validation before a party lease is consumed. No implicit
   fallback is permitted. Package-loader size selection, gate-budget selection,
   and context validation dispatch only after parsing the explicit
   package/context format; the legacy F_T/v1/v2 path retains its 2 MiB and
   N<=256 limits.
4. For a uniform 32-bit word and Fisher–Yates bound `i<=1024`, rejection
   probability is `(2^32 mod i)/2^32 <= 1023/2^32 < 2^-22`. At most 1023
   positions are sampled. The mathematical minimum for a chosen domain is

   ```text
   r_min(lambda,m) = ceil((lambda + log2(m-1))/22), m>1; r_min=0 for m=1.
   ```

   Since `r_min(128,1024)=7`, F_T/v3 fixes a profile-wide maximum
   `sampler_retry_limit=7` for every supported padded domain `m<=1024`; the
   package/context field is always 7, including `m=1` where there are no
   Fisher–Yates draws. The sampler may therefore use more attempts than the
   mathematical minimum for small domains. For any `m<=1024`, a union bound
   gives `delta_reject <= (m-1)*2^(-22*7) <= 1023*2^-154 < 2^-144 < 2^-128`.
   Successful samples remain exactly uniform under IID 32-bit words. This
   statistical bound applies only to the semantic-material projection with
   version fields, encodings and signatures removed; it does not compare full
   package transcripts and does not close the real DRBG bridge.
5. Exhaustion returns the existing explicit `ISSUANCE_ABORT` class. The new
   FTv3 marker is profile-scoped and keyed by `(session, invocation)` using
   `filename = "ft-v3-" || hex(SHA256(UTF8(key_domain) || 0x00 || session[16] || invocation[16]))`,
   where `key_domain` is
   `ProtocolI/PROJECT_DERIVED_SELECT_5ITER/GMW-Beaver-v3/F_T-v3/issuance-registry-key/v1`.
   The mode-0600 marker record is
   `UTF8(record_domain) || 0x00 || BE32(1) || session[16] || invocation[16] ||
   BE32(N) || BE32(K) || ft_version:u8 || sampler_version:u8 || lambda_bits:u8 ||
   retry_limit:u8 || BE32(package_format=6) || circuit_sha256[32] ||
   gate_schedule_sha256[32]`, with `record_domain` equal to
   `ProtocolI/PROJECT_DERIVED_SELECT_5ITER/GMW-Beaver-v3/F_T-v3/issuance-burn-record/v1`.
   T creates it atomically with create-if-absent in its owner-only 0700
   registry and fsyncs the file and containing directory before allocating or
   sampling material. Failure before marker creation aborts before material
   allocation. If write/fsync fails after marker creation, retain the partial
   marker as a burn and abort before package publication. Bounded-sampler
   exhaustion retains the marker. A repeated FTv3 call for the same
   session/invocation is rejected, and the
   FTv3 issuer contains no automatic retry or dispatch to FTv1/v2. This
   profile-specific marker does not make the historical FTv1 API or FTv2
   registry recognize the FTv3 tombstone; a separate explicit call to a legacy
   issuer with reused identifiers is outside this marker's enforcement scope.
   N=1000 cannot use either legacy issuer because those paths retain the
   N<=256 limit. On a
   fresh invocation the one-shot private registry burn marker is retained;
   package, signed context, party lease, and READY are not published. Material
   vectors have already been formed before the permutation sampler runs and
   are not explicitly zeroized in S46; the candidate must report this, not
   claim that no sensitive temporary existed or that no trace was written.
6. Set an FTv3-specific package maximum of 64 MiB and gate ceiling above the
   exact 88,626,688 value (proposed 100,000,000). Leave the 2 MiB escrow
   ciphertext limit unchanged: output-share envelopes are independent of the
   Dealer material package size. Reject truncation, overlong u32 fields,
   wrong version tuples, and cross-version packages before lease consumption.
7. For FTv3 only, deterministically split an AND layer into batches of at most
   4,193,824 gates. Each Beaver frame contains layer ordinal, batch ordinal,
   batch gate count and packed openings; the ordinary framed transport still
   caps each inner frame at 1 MiB and sequence/phase. New schedule hash commits
   to the frame profile and externally visible batch schedule. FTv1/v2 retain
   their existing one-frame schedule and hash. If exact dual-version schedule
   binding cannot be preserved, stop and version the schedule before modifying
   runtime.
8. Raise the TEST_ONLY N=1000 party cap to 2 GiB explicitly. Production code
   continues to enforce the computed cap passed by its caller; no allocation
   fallback or reduced-N substitution is allowed.

The new bounds must be profile-dispatched, not global edits to legacy limits:
keep `score_to_mask_v3_gate_budget` at N<=256 and add an FTv3-only budget
entry point for N<=1000; keep the legacy Select5 sampling/control entry points
at 256 and add FTv3-only entry points for padded m<=1024. The package reader
must select a 2 MiB or 64 MiB cap only after explicit format dispatch. The
escrow context format-2 path dispatches its own profile and budget; the escrow
service's ciphertext cap stays 2 MiB. Preserve context-format-1, FTv1/v2
schedule, package, signature and gate-budget behavior byte-for-byte. Cross-
version and cross-profile tuple validation occurs before inserting the local
one-shot package lease.

## Proposed S47 validation and stop rules

- Before runtime edits, reviewer must accept the probability bound, failure
  side-effects and marker scope, exact version tuple/signature bytes, 1 MiB
  batching and externally visible schedule accounting, compatibility boundary,
  full phase upper bound, and itemized memory estimates or return a precise
  correction.
- Fresh Release default-OFF must omit all new FTv3 targets/symbols; explicit
  FTv3 opt-in must build only with required GMW/receipt-v2 options. TEST_ONLY
  random hooks stay out of `BUILD_TESTING=OFF`.
- Run FTv3 sampler tests for `m=1,2,3,1024`, first-attempt success,
  reject-then-accept, exactly seven rejected words, maximum bytes/call counts,
  version mismatch, and registry/artifact state. Run only minimal F_T/v1/v2
  N<=256 compatibility smokes under the new binary.
- Run five N=1000 multiprocess cases with `K={1,8,500,999,1000}`, frozen
  signed Q20.12 oracle and negative/boundary/tied scores; verify original-order
  mask and exactly K ones. Capture actual package/frame sizes, RSS, disk, exits,
  phase count and logs. If the first measured resource boundary prevents a
  complete run, stop there and report it without claiming N=1000 passed.
- Preserve `ISSUANCE_ABORT_PRODUCT_ACCEPTANCE=OPEN_CONFIRMATION`,
  `DRBG_DEPLOYMENT_PROFILE=OPEN`, `JOINT_RNG_STREAM_BRIDGE=OPEN`,
  `PRIVATE_CHANNEL_BRIDGE=OPEN`, `PARTY_VIEW_COMPOSITION=OPEN`,
  `SECURE_ALIAS_READY=NOT_READY`, and `PERFORMANCE_READY=NOT_READY`.
  No formal LAN/WAN, RTT, security, or AAV86 comparison is in scope.

## S47 implementation and review result

The experimental implementation is in the S47 source delta listed in the
source/build/evidence manifest. `F_T/v3` uses a fixed signed retry cap of 7
for every supported padded size, including a package issued for padded `m=2`;
the direct sampler still treats `m=1` as zero draws. Both production and
TEST_ONLY issuers serialize cap 7, while package and context validation require
cap 7 and separately check that the mathematical `r_min(lambda,m)` does not
exceed it. The `receipt_format_version` remains an independent escrow field.
The final reviewer follow-up checked these bindings, the N=1 assertions, the
`BUILD_TESTING=OFF` exclusion and the documented probability bound. That
reviewer did not build or run tests; the build and tests below were run by the
S47 implementer and are recorded separately in the S47 report.

The final target environment was WSL Ubuntu 24.04.4 with OpenSSL 3.0.13,
GCC 13.3 and CMake 3.28.3. A preliminary Ubuntu 20.04 configure did not start
because its CMake was 3.16.3 while the repository requires at least 3.17; this
was an environment/toolchain failure, not a source or resource failure. Fresh
default-OFF and explicit production Release builds succeeded, as did the
FTv3 sampler/runtime conformance and ten-case targeted multiprocess oracle
run. See the S47 report and manifests for exact hashes, commands and observed
resources.

Observed N=1000 per-party package size was 33,366,516 bytes, maximum frame
payload 1,048,572 bytes, and peak party RSS ranged from 942,312 to 981,868
KiB. All five target K cases completed and matched the frozen oracle. These
are local functional/resource observations only. They do not close the
low-probability issuance-abort product acceptance, target DRBG deployment,
joint RNG stream, private channel, party-view security or formal performance
gates. The S47 acceptance is `PASS_WITH_SCOPE` for the exercised N=1000
cases; N>1000 remains unadmitted.
