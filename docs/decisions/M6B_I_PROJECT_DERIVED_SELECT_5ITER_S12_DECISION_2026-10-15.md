# M6B-I PROJECT_DERIVED_SELECT_5ITER S12 decision

Date: 2026-10-15  
Route: `Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / EXPERIMENTAL`  
Base: `c3926c68fd14f270faa8b55234311071947fa080`  
Decision: keep the route opt-in and EXPERIMENTAL; do not create a `secure` alias.

## Independent review of S11's DCF bridge

The S11 bridge is **not independently confirmed**. The actual S8 `dcf.cpp` is the unchanged base blob `b85fc93d854f5a3c9875501acb05145c5a9dff32` (SHA-256 recorded in the S12 source manifest). For `Bout=64, groupSize=1`, `convert` copies the first 8 bytes of its input block. In key generation/evaluation, the terminal conversion is called on `s & notThreeBlock`; `notThreeBlock` clears the low two bits of the low 64-bit lane. On the x86_64 little-endian target used for the build, the projected 64-bit group element therefore has two fixed low zero bits. BCG+21's `Convert_G` for `G=Z/(2^64)` and `k=64<=lambda` returns the first 64 bits of a pseudorandom string, whose output is uniform over all 64-bit elements when its input is uniform. The implementation projection is supported on only `2^62` values.

For the **claimed conversion-output distribution**, the predicate `y & 3 == 0` has probability 1 under the implementation input projection and 1/4 for a uniform 64-bit group element, a statistical gap of 3/4. This is a concrete theorem-mapping witness, **not** a demonstrated distinguisher on the serialized party key and not evidence that the DCF is broken. The first missing result is a reduction showing that the terminal conversion's restricted input distribution is covered by the theorem (or a new proof for the biased conversion). Until then `DCF_SINGLE_KEY_PRIVACY=OPEN`; correlated-pool and adaptive-key hybrid claims that depend on this bridge also remain OPEN.

The cited construction is Boyle et al., EUROCRYPT 2021, Figure 1 and Theorem 2 (main-version pp. 12–13; Appendix F.1, pp. 42–45), [official full version](https://www.microsoft.com/en-us/research/wp-content/uploads/2021/01/FSS.pdf). The paper theorem, S8 code behavior, S12 source-to-theorem check, and unresolved proof extension are kept as separate evidence classes in the S12 report. No DCF ABI was silently changed.

## Runtime and storage decisions

1. N=128 `M6B5/v2` party packages and N=256 `M6S2` segment stores use separate OS-owned directories. Party processes run under distinct UIDs in the E2E harness. Shared-directory writer/publisher overloads were removed from the Select5 API.
2. Monolithic ready metadata uses `M6P2/v1`: an 88-byte recipient-bound marker containing only that party's package digest. The underlying package remains `M6B5/v2`; the N=256 streamed `M6S2` manifest/segments remain a separate schema.
3. Party store opens require final directory ownership by the effective UID, mode `0700` or stricter, and `O_NOFOLLOW`; files are one-shot and fsync-backed. This is a local POSIX contract under a trusted supervisor, not a cross-host or hostile-root guarantee.
4. Output uses durable `.run`, `.prepared`, `.commit`, and `.published` markers and one TLS final-status transaction with PREPARE and COMMIT phases. The API returns only the caller's XOR share after the handshake. A crash can leave at most one orphan share; two independent processes do not atomically publish together. All-or-nothing publication is not claimed.

## S12 reproduced defects and regression

- Initial ASAN `n=1` T/P0/P1 E2E reproduced a double-free/use-after-free in TLS wrapper destruction when the final-status transaction recreated/reused channel ownership across phases. The runtime now keeps one channel and advances its authenticated frame phase. The preserved post-fix ASAN `n=1` trace returns `ORACLE_PASS`; the final Release six-target CTest run also passes.
- The prior monolithic ready record exposed both party package digests to a reader of either shared marker. `M6P2/v1` replaces it with an 88-byte recipient-bound record containing only that recipient's digest. The shared two-package staging entry points were removed. Store and distinct-UID process tests verify owner-only reads and reject cross-party file access.
- An earlier pre-fix process E2E invocation exited 1 with only a generic faulted-process/no-mask assertion and no per-case child-status record. It remains a failed attempt with unlocalized cause. The later E2E emits per-fault JSON and separately checks output absence; the final six fault cases all return `UNIFORM_ABORT_NO_MASK`.
- Final Release CTest: 6/6 selected tests pass. The process E2E reports 18 frozen-oracle successes, 6 uniform abort/no-mask cases, and 6 separate crash-prefix no-complete-mask cases. Additional streamed-store and TLS tests pass; the feature-OFF CTest filter registers zero Select5 tests.
- The reproduced ASAN failure and its post-fix log are retained outside the repository and hashed in the S12 source manifest. The ASAN executable itself was temporary under WSL `/tmp` and was not retained; its binary SHA-256 is therefore `NOT_RETAINED`. Final Release test-binary hashes are recorded separately.

## Preserved scope

S10 remains the independent functional receipt for the exact S8 source manifest; it does not cover S12. S12's own tests are implementation-author regression, not an independent S12 receipt. The route remains project-derived, not BB90 or BMW16. M6A V3 remains OPEN, V4 remains NOT_ALLOWED/NOT_RUN, and formal LAN/WAN remains NOT_ALLOWED/NOT_RUN. `N>256` remains rejected. See the [S12 report](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S12_REPORT_2026-10-15.md), [source manifest](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S12_SOURCE_MANIFEST_2026-10-15.json), and [gate summary](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S12_GATES_2026-10-15.json).
