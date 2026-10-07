# BMW16 S18 — streamed KeyGen, party reads, and bounded E2E report

**Implementation label:** `Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL`
**Source under test:** S17 base `6f0ac1bfdccc27b43877104807f87e1faffb131e` plus S18 working-tree changes (the test binary is built from this tree).
**Date/environment:** 2026-10-07; Windows 11 host, WSL2 Ubuntu 24.04, GNU C++ 13.3, CMake Release, OpenSSL 3, `MOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=OFF`, EMP OT OFF.
**Build directory:** `/tmp/bmw16-s18-build` in WSL.
**Raw evidence directory (outside Git):** `C:\Users\28641\.codex\artifacts\BMW16_S18_20261007`.

## Summary

S18 fixes the two source-path gaps identified in S17: trusted offline T now
generates and seals each fresh uCMP/DCF slot as it is produced, and party
processes fetch authenticated records by random access rather than retaining
nine quadratic in-memory key pools. The existing C++ Select, raw-score adapter,
shuffle, membership, inverse routing, and output-share implementation are
reused. A common generator path supports byte-for-byte slot comparison against
the legacy materialized path.

Small and bounded extended T/P0/P1 process tests completed successfully for
n=1,2,5,8,64,128,256,1000. The first n=1000 attempt completed streamed
generation but hit the existing 1 MiB single-frame transport cap. The exact
failure was preserved, a chunked comparison-layer exchange was added using
the repository's bounded framed-chunk transport, and a second run with fresh
material passed.
The research path remains EXPERIMENTAL because the correlated full-pool
adaptive DCF and actual shuffle joint simulator are not proved. No formal
LAN/WAN matrix was run.

## Baseline and source audit

- S17 commit and clean initial worktree: `6f0ac1bfdccc27b43877104807f87e1faffb131e`.
- S17 prototype artifacts remain attributable to that tree. Historical pilot
  rows previously marked `UNVERIFIED` for source/binary binding are not reused
  as S18 measurements or acceptance evidence.
- Branch: `codex/m6b-i-bmw16-s18`; isolated managed worktree under
  `C:\Users\28641\.codex\worktrees\m6b-i-bmw16-s18\...`.
- S17's test-only stream-store writer accepted a fully materialized party
  object. The node's T command called `material_generate()` first. The party
  then decoded that full pool before assigning a lazy lookup function. This
  was not streamed generation and not a no-full-key-pool party path.
- S18 adds a generator callback that writes each key slot during KeyGen; the
  memory generator is retained as the byte/differential reference. The new
  stream shell stores O(n) score/shuffle state and a compact implicit manifest.
- The app's S18 T/P0/P1 flow remains explicitly opt-in/EXPERIMENTAL. T accepts
  public dimensions only and exits before raw input generation; P0/P1 run as
  separate UIDs in the test harness. The harness/root user is not counted as a
  party view.

## Implemented data and claim contract

The fixed slot order is: raw/shuffle prefix (`2*p+2` slots), eight
task/layer Select pair pools, then one membership pool. Each of the nine pools
contains `C(n,2)` canonical unordered endpoints; direction conversion is done
from the one slot using the tested Z/(2^64) complement convention. There is no
cross-task, cross-round, or membership key reuse. Logical comparisons remain
distinct from actual unique key slots and Eval calls.

Forward and inverse shuffle generation requests the repository's shuffle-only
mode; the two unused CmpAgg pools are not generated or serialized. At n=64 this
omits 4,032 CmpAgg pair-key slots per party, rather than marking them unused
after generation.

The sidecar header is 164 bytes; each encrypted record contains the serialized
key plus the 16-byte GCM tag. The compact shell binds one party's sidecar hash
and the common stream ID. T finalizes and fsyncs exact-length sidecars before
publishing shells. Party opens the shell, burns the bundle identity in its
fixed protected claim root, then validates the complete sidecar hash with a
64 KiB buffer before socket/Eval. Each accessed slot is random-read, metadata
checked, GCM authenticated, and marked consumed within that process. The
contract does not support copying to a new claim store or snapshot rollback.

The durable claim intentionally happens before sidecar verification: a bad
sidecar aborts and burns the bundle; recovery uses fresh offline material.
An interrupted T write must not publish a ready shell. Any surviving `.tmp`
file after power loss is not claimable and requires offline cleanup. A party
that detects peer loss reports only local abort unless it receives protocol
evidence that the other party reached the same state.

## Key-slot differential

`moe_topk_bmw16_experimental_party_test` invokes a TEST_ONLY tee on the shared
slot-generation code. For one fixed config and one KeyGen result, the memory
path and the stream writer serialize each P0/P1 Select+membership slot and
compare bytes. It also verifies final stream digest/length, tampering,
truncation, metadata mismatch, and duplicate local read rejection.

Observed fixture: n=5, 90 slots per party, sidecar length 84,494 bytes each.
The initial run exposed a nonce parsing defect (`BMW16 stream authentication
failed`); after preserving the parsed base nonce, the unit test passed. This
was an implementation defect and is retained in the raw evidence, not counted
as an algorithm failure.

## E2E evidence

The runner forks/execs the same C++ party-node binary as T, P0 and P1, prepares
fresh materials per case, checks exit codes and status, reconstructs only
inside the TEST_ONLY harness, then verifies mask length, bits, Hamming weight
K, and equality to the frozen stable-key oracle. Each party writes only its
share. It records both parties' status and reports missing ACK as local-only,
not joint abort.

### Current observed runs

| Cases | Result | Notes |
|---|---|---|
| n=1,2,5,8 basic | PASS | all SUCCESS and oracle checked; n=1 singleton shortcut |
| n=64 fixed and random inputs, K=1/8/64 | PASS | non-full-sampling branch exercised |
| n=128 fixed/random, K=8/80 | PASS | two independent process E2E cases |
| n=256, K=2 | PASS | full process E2E; per-party sidecar about 310,504,484 bytes in observed run |
| n=1000, K=80 | PASS after transport fix | fresh stream, both party exit 0, oracle checked, mask weight 80 |

The byte-identical frozen S4 source was extracted from Git commit
`04ed6f8a352ab2277434344094471dff82f83e4b` and has SHA-256
`8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab`.
Common-tape differential against the same S18-built C++ selector passed for
three n=128 cases (K=2/8/80; final matrix hash
`a064c09ba5c26350c92beccef3c3364ac727ce2166217a23ebc310637836581f`) and two
n=256 cases (K=2/8; final matrix hash
`a7ae70f31dcf3e67ca75f3183b95caed733cf7424545596028b67c46cdd6e43f`). The
first attempt passed the checked-in S7 copy (`39757567…`) to the frozen-source
comparator; the comparator correctly rejected its hash before trace
comparison. That identity failure is retained and was resolved by using the
exact S4 Git object, not by changing the comparator.

For the successful n=1000 run, T generated 4,495,500 uCMP slots per party;
each encrypted sidecar was exactly 4,967,527,664 bytes and each shell was
2,013,901 bytes. The T phase reported 107,356,765 microseconds and peak RSS
22,976 KiB. P0/P1 each reported 345,991 logical comparisons, 114,573 uCMP
party evaluations and 229,146 DCF party evaluations; sidecar validate scans
were 55,094,845 and 55,115,370 microseconds, and peak RSS was 40,888 and
40,572 KiB. Those diagnostic timings depend on this WSL/host/storage setup and
are not formal performance data. Total E2E wall time, bytes sent/received for
all Select phases, and whole-path PRG calls are `NOT_MEASURED`.

S18 implementation debugging failures retained outside Git:

1. `party_test.log`: initial stream reader failed AEAD due to nonce parsing.
2. `node_e2e_basic.log`: bundle reader held a temporary serialization vector,
   giving a truncated zero-byte span; the lifetime is now explicit.
3. `node_e2e_n1000.log`: T streamed and published both bundles, then a large
   comparison-layer share vector exceeded `ProtocolIFramedChannel`'s 1 MiB
   single-frame payload cap. One side reported `ERROR_UNEXPECTED / PROCESS_ERROR`
   with `frame length`; the peer reported `ABORT_COMMUNICATION / LOCAL_ONLY`
   and emitted no mask. Before this run the party-side progress was 337,904
   logical comparisons; it is a communication failure, not a natural algorithm
   abort. `node_e2e_basic_rerun.log` contains the intermediate independent shell
   digest mismatch when one party's package was bound to the other party's
   sidecar digest; the code now binds each shell to its own sidecar SHA-256.
4. `node_e2e_n1000_chunkfix.log`: after chunked layer exchange, the same n/K
   and input-seed case with fresh offline keys ended in `SUCCESS_ORACLE_CHECKED`
   for both parties, exit code 0, exact oracle mask and weight K.
5. `s4_copy_identity_attempt.txt`: two common-tape runner invocations used the
   transformed S7 copy instead of the frozen S4 source. The expected SHA check
   rejected before algorithm trace comparison; final runs used the byte-exact
   Git S4 object and passed.

These are corrected serialization/binding defects, not algorithmic false
successes. The first n=1000 attempt is still reported as a failed trial and is
not dropped from the attempt history. All attempts remain in the external
evidence directory.

## n=1000 checked preflight

Command:

```powershell
wsl.exe -d Ubuntu-24.04 -u root -- python3 \
  /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s18/MoE_Top-k_Reproduction_Evaluation/experiments/TEST_ONLY_BMW16_S18/capacity_preflight.py \
  --n 1000 --k 80 --directory /tmp
```

The checked calculation returned `CAPACITY_OK` with observed `/tmp` free space
990,198,984,704 bytes and `MemAvailable` 7,568,912,384 bytes. It calculates
4,495,500 uCMP slots per party, 1,089 plaintext key bytes and 1,105 encrypted
record bytes per slot, 4,967,527,664 bytes per sidecar and 9,935,055,328 bytes
for both sidecars. The 1 GiB disk and 2 GiB memory reserves are policy inputs.
This is a format/resource preflight, not measured output size, T RSS, or a
performance result. Shell, raw/shuffle generator memory, and exact T time were
NOT_MEASURED by preflight.

The same checked calculation was run for larger sizes (K=80) without
generating material:

| n | comparison bits | slots/party | sidecar bytes/party (formula) | preflight |
|---:|---:|---:|---:|---|
| 10,000 | 47 | 449,955,000 | 540,395,955,164 | PRECHECK_REJECTED; both sidecars exceed observed free disk |
| 100,000 | 50 | 44,999,550,000 | 57,284,427,150,164 | PRECHECK_REJECTED |
| 1,000,000 | 53 | 4,499,995,500,000 | 6,052,493,947,500,164 | PRECHECK_REJECTED |

These are checked layout calculations, not files written. The stream KeyGen
accepts a representational domain through n=2^20, but full party execution is
also constrained by the compact shell reader's 512 MiB file limit, O(n)
raw/shuffle memory, available disk, and the quadratic slot pool. Thus n=1000
clears the current shell limit; the API's representational maximum does not
claim every size is runnable on this host.

### Functional and failure-path coverage

The final targeted CTest set was rerun after the S18 source/test edits. All 12
selected tests passed: BMW16 party, Protocol III combine/process, raw-score
pipeline/process, reverse shuffle, uCMP, transport, raw-score input, and
parallel shuffle regressions. The exact test names and raw output are in the
external evidence index. A separate unfiltered `ctest` invocation discovered
42 registered tests; 12 built tests passed, while 30 were `Not Run` because
their executables were not built in this targeted Release configuration. CTest
therefore returned 8 for the incomplete full suite. This is a build-coverage
limitation, not 30 failing test executions. The BMW16 party test
also races two separate processes for the same durable claim and verifies
exactly one winner. It forks a child that durably claims a bundle and exits
without cleanup, then reopens the same claim root and verifies replay rejection.
It verifies that an
incomplete streamed writer cannot finalize or publish a ready sidecar and that
its pending file is removed on ordinary destruction. Endpoint, party, root,
truncation, tamper and duplicate-read negatives are included.

Not exercised: forced ENOSPC, power loss between each file/dir fsync and publish
boundary, a full party-node kill after claim followed by process restart, an
open-but-silent peer, and a natural S4 algorithm abort. A crash after the
durable claim is tested at the claim-store level; a crash during T publication
is represented by writer finalization failure, not by a power-cut fault
injector. A peer-close/local-abort result is not reported as a joint abort
unless an acknowledgement is present. No mask is emitted on the first n=1000
communication failure or on material/claim errors.

The local multi-UID test does not prove isolation against a host administrator,
root test orchestrator, filesystem snapshot rollback, a copied bundle plus a
new claim root, or a compromised T. The stated one-shot guarantee is scoped to
one fixed protected, non-rollbackable claim root. The current party test proves
the restart replay behavior for the exercised local claim implementation, not
that root/snapshot rollback is prevented by the host.

### Larger checked configurations

The current stream API's representational bound is `n<=2^20`; this does not
remove resource and shell-size constraints. For K=80, the checked calculations
above yield:

| n | slots per party | sidecar bytes per party | status on this host |
|---:|---:|---:|---|
| 10,000 | 449,955,000 | 540,395,955,164 | PRECHECK_REJECTED |
| 100,000 | 44,999,550,000 | 57,284,427,150,164 | PRECHECK_REJECTED |
| 1,000,000 | 4,499,995,500,000 | 6,052,493,947,500,164 | PRECHECK_REJECTED |

The S18 preflight command reports current available disk/RAM and applies explicit
reserve policies. These sizes were not generated. No capacity estimate is
presented as a measured material file or runtime.

## Security and delivery ruling

**Party-view security: NO-GO.** `ProtocolIUcmpMaterial` in
`VFSS/src/moe_topk/protocol_i_ucmp.cpp` forms
`alpha_ij=(r_i-r_j) mod 2^b`, invokes `keyGenDCF(b,64,alpha_ij,1)`, and each
`eval_strict_lt` calls `evalDCF` twice. The implementation is
`VFSS/ext/FSS/dcf.cpp`; declarations are in
`VFSS/ext/FSS/include/FSS/dcf.h`. `keyGenDCF` consumes two initial blocks from
thread-indexed `FSSConfig::prngs`; the Protocol I shuffle generator seeds that
PRNG once from OS entropy, after which KeyGen draws sequentially. S18 leaves
this schedule unchanged.

The hybrid attempt is H0→H1, replacing the hidden sequential AES-PRNG stream
with uniform tape under a multi-output cryptoTools PRG premise; H1→H2,
replacing the `N=9*C(n,2)` corrupt-party key shares one at a time, including
unused slots; then H2→H3, simulating the actual forward/inverse shuffle state
and final mask share. H1→H2 needs DCF privacy in the presence of arbitrary
auxiliary input containing other correlated key shares and threshold-family
information, adaptive query inputs selected from earlier opened comparison
bits, and the corrupt party's local Eval-share distribution conditioned on
each opened result. If this property is available, the hybrid advantage can
be bounded by `N` times the single-key advantage, plus H0's PRG advantage, for
polynomial `N`. The repository does not state/prove that exact
output-conditioned adaptive auxiliary-input theorem for this ABI. Ordinary
single-key key-only privacy is not the required game. This is the first
unresolved step; S18 supplies neither an attack nor an impossibility claim.
The next hybrid still requires a simulator for actual Protocol I shuffle
local material/messages, shared anonymous handles, abort, and output shares.

Candidate public leakage fields (anonymous graph, comparison bits, pivots,
U/V/W, slot-access order, selected anonymous handle, abort class) are listed as
candidate leakage only; none is newly approved by S18. The AES-GCM/UID local
test supports bytes/context confidentiality within the local experimental
contract. It does not establish production remote credentials or private
authenticated T-to-party transport. No secure alias and no official V4
qualification are created.

## Metrics and boundaries

For each field below, the run record must say `MEASURED`, `DERIVED`, or
`NOT_MEASURED`:

- logical comparisons, real-real/dummy edges, unique endpoints, duplicate
  requests;
- generated slots, stream ciphertext bytes, shell bytes, consumed/unused
  slots, DCF Eval calls;
- T keygen/seal/fsync time and peak RSS; party shell-open, durable claim,
  sidecar checksum scan, per-slot decrypt/Eval, socket send/receive, total
  online time and peak RSS;
- causal phases/messages and PRG calls. This build disables the repository's
  optional DCF PRG counter; do not infer whole-path PRG counts from DCF Eval.

The whole-sidecar hash scan is O(file size), uses bounded memory, and is part
of pre-protocol startup. The T and party timing fields emitted by the node are
experimental diagnostics only. No LAN/WAN matrix, no official five-run matrix,
and no six-scheme rank comparison were run.

## Gate decisions

| Gate | Decision | Evidence / limit |
|---|---|---|
| Streamed offline key generation | PASS for implemented experimental API | T invokes the common slot generator with a sink; it does not first materialize the nine quadratic key pools. n=1000 fresh generation completed. |
| Party random access without quadratic resident key pools | PASS for tested path | Compact O(n) shell plus authenticated on-demand records; n=1000 P0/P1 RSS observations are about 40 MiB each. This does not measure every host peak or exclude O(n) shuffle state. |
| Byte/trace compatibility | PASS for exercised evidence | TEST_ONLY memory/stream key-slot bytes match on the fixed fixture; frozen S4 common-tape differential passed n=128 (3 cases) and n=256 (2 cases). |
| Independent party function | PASS on bounded matrix | T exits before input creation; separate P0/P1 UIDs; n=1/2/5/8/64/128/256/1000 observed successes were oracle checked. First n=1000 transport failure is retained. No natural algorithm abort was observed. |
| Claim and authenticated slot reads | PASS within fixed claim-root contract | Durable bundle burn before online traffic, restart replay rejection at claim-store test level, per-slot metadata/GCM and duplicate-read negatives. No cross-store, administrator, or snapshot rollback guarantee. |
| Private production T delivery | NOT READY | Current T fixture uses local test credentials/filesystem delivery. It is not a deployed remote authenticated private transport. |
| Joint party-view security | NO-GO | Output-conditioned, adaptive, auxiliary-input multi-key DCF view and actual shuffle/output composition lack the required simulator/theorem. Candidate transcript leakage is not approved. |
| Natural abort bound | NOT established by S18 | The observed finite sample is not an upper bound; S18 does not inherit BMW16 Theorem 8. |
| Complete registered CTest suite | NOT RUN completely | 12 relevant built tests pass; 30 of 42 registered tests have no executable in this targeted build and were reported `Not Run`. |
| Formal V4 / LAN-WAN performance | NOT RUN | Experimental function diagnostics only; no official performance status. |

Thus the S18 function and streamed storage work is reusable for further
EXPERIMENTAL research, while the next security blocker remains the precise
output-conditioned adaptive multi-key DCF simulation and its composition with
the real shuffle state machine. Production delivery credentials are a separate
engineering gate; they are not the reason the conditional cryptographic proof
is NO-GO.

## Build and commands

```powershell
wsl.exe -d Ubuntu-24.04 -u root -- cmake -S <worktree>/VFSS -B /tmp/bmw16-s18-build `
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON `
  -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON `
  -DMOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=OFF -DMOE_TOPK_ENABLE_EMP_OT=OFF
wsl.exe -d Ubuntu-24.04 -u root -- cmake --build /tmp/bmw16-s18-build --target `
  moe_topk_bmw16_experimental_party_test `
  moe_topk_bmw16_experimental_party_node `
  moe_topk_bmw16_experimental_party_node_e2e_test `
  moe_topk_bmw16_experimental_3proc_test -j2
wsl.exe -d Ubuntu-24.04 -u root -- bash -lc "ctest --test-dir /tmp/bmw16-s18-build --output-on-failure -R '^(moe_topk_bmw16_experimental_party_test|moe_topk_m3_secure_combine_test|moe_topk_m3_protocol_iii_three_process_e2e_test|moe_topk_m3_raw_score_pipeline_test|moe_topk_m3_raw_score_three_process_e2e_test|moe_topk_m3_raw_score_secure_executable_test|moe_topk_m2_reverse_shuffle_model_test|moe_topk_m2_ucmp_conformance_test|moe_topk_m2_transport_conformance_test|moe_topk_m2_score_input_conformance_test|moe_topk_m2_parallel_shuffle_conformance_test|moe_topk_m2_parallel_shuffle_process_e2e_test)$'"
wsl.exe -d Ubuntu-24.04 -u root -- /tmp/bmw16-s18-build/moe_topk_bmw16_experimental_party_node_e2e_test /tmp/bmw16-s18-build/moe_topk_bmw16_experimental_party_node --extended
wsl.exe -d Ubuntu-24.04 -u root -- /tmp/bmw16-s18-build/moe_topk_bmw16_experimental_party_node_e2e_test /tmp/bmw16-s18-build/moe_topk_bmw16_experimental_party_node --n1000
wsl.exe -d Ubuntu-24.04 -u root -- /tmp/bmw16-s18-build/moe_topk_bmw16_experimental_party_test
wsl.exe -d Ubuntu-24.04 -u root -- python3 <worktree>/experiments/TEST_ONLY_BMW16_S18/capacity_preflight.py --n 1000 --k 80 --directory /tmp
wsl.exe -d Ubuntu-24.04 -u root -- python3 <worktree>/experiments/TEST_ONLY_BMW16_S18/capacity_preflight.py --n 10000 --k 80 --directory /tmp
```

The S18 party API remains behind the opt-in BMW16 experimental CMake option.
The raw logs are outside Git and indexed by SHA-256 in the final evidence
index at
[`BMW16_S18_EXTERNAL_EVIDENCE_INDEX_2026-10-07.md`](evidence/BMW16_S18_EXTERNAL_EVIDENCE_INDEX_2026-10-07.md).
The measured byte/timing observations above do not qualify as a
LAN/WAN pilot or official V4 result.

## Files changed

| File | Change |
|---|---|
| `VFSS/include/moe_topk/experimental_bmw16_material_bundle.h` | Streaming bundle binding and party-open result declarations. |
| `VFSS/include/moe_topk/experimental_bmw16_select_party.h` | Shared streamed/materialized generator API and slot sink contract. |
| `VFSS/include/moe_topk/experimental_bmw16_stream_store.h` | Incremental writer and random-access encrypted slot reader interface. |
| `VFSS/src/moe_topk/experimental_bmw16_select_party.cpp` | Common KeyGen path, per-slot callback emission, and lazy party slot loading. |
| `VFSS/src/moe_topk/experimental_bmw16_stream_store.cpp` | Incremental sealing, finalization/publication, authenticated random-access reads, and bounded sidecar verification. |
| `VFSS/src/moe_topk/experimental_bmw16_material_bundle.cpp` | Compact stream manifest/shell encoding and identity/configuration binding. |
| `VFSS/src/apps/moe_topk_bmw16_experimental_party_node.cpp` | T streaming generation and P0/P1 shell/sidecar party path, capacity-domain update, and run metrics. |
| `VFSS/tests/moe_topk/experimental_bmw16_party_test.cpp` | Slot-byte equality, interrupted-writer, wrong metadata, tamper, truncation, duplicate, and durable-claim replay tests. |
| `VFSS/tests/moe_topk/experimental_bmw16_party_node_e2e_test.cpp` | Independent-UID multi-size runner and larger framed comparison-layer coverage. |
| `experiments/TEST_ONLY_BMW16_S18/capacity_preflight.py` | Checked n/K slot, sidecar, disk-reserve, and memory-reserve preflight. |
| `docs/decisions/BMW16_S18_STREAMED_KEYGEN_PARTY_READ_AND_SECURITY_REVIEW_2026-10-07.md` | S18 implementation contract, fixed-root scope, and conditional-security decision. |
| `docs/reproduction/BMW16_S18_STREAMED_KEYGEN_E2E_AND_SECURITY_REVIEW_2026-10-07.md` | This implementation, validation, failure, cost, and gate report. |
| `docs/reproduction/evidence/BMW16_S18_EXTERNAL_EVIDENCE_INDEX_2026-10-07.md` | SHA-256 identities for external run evidence and final source/binary files. |
| `docs/IMPLEMENTATION_PLAN.md` | Records authorized early bounded EXPERIMENTAL work without advancing V3/V4 acceptance. |
