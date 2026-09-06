# M2.8 EMP chosen-OT dependency qualification

Status: **implemented and validated as `m2_emp_iknp_chosen_ot_conformance` (C).**

This is an engineering dependency investigation (**C**), not an Agarwal
Protocol I, OPV, Share Translation, Permute+Share, or shuffle implementation
(**A/D**).  No Protocol I claim follows from the result.

## Evidence and fixed candidate

| component | requested revision | inspected archive SHA-256 | license / notice result |
| --- | --- | --- | --- |
| `emp-tool` | `v1.0.0-alpha.1`, `97f335927dd7d38caaf5e80d93fca70edddd5423` | `7f4a2cb169ba0b7fc48ffe89b7615288c41f9377bb0a4d56a3178fe20b66ab46` | Apache-2.0; notice lists optional BLAKE3 (CC0/Apache-2.0), sse2neon (MIT), ThreadPool (zlib) and generated SoftFloat assets (BSD-3-Clause). |
| `emp-ot` | `v1.0.0-alpha.1`, `03acb042b98e82fd5fd0da33babd44801f8ec082` | `8cfffc340a2014e5ac3b90c6659c0a812a6748e7e24c6ea2b0f5b3e58ba66183` | Apache-2.0; notice identifies vendored ML-KEM/Kyber reference code (CC0 or Apache-2.0). |

The archives were fetched from the exact GitHub codeload commit URLs on
2026-09-05. `emp-tool`'s own CMake source requires OpenSSL 3, Threads, C++20,
GNU >=10 / Clang >=12 / AppleClang >=14, and explicitly rejects every system
except Linux or Darwin. `emp-ot` finds `emp-tool` as `emp-tool::emp-tool`,
publishes `emp-ot::emp-ot`, requires C++20 transitively, and contains upstream
two-party `test_base_ot` and `test_iknp` tests. The planned dependency set is
therefore limited to emp-tool, emp-ot, OpenSSL 3, Threads and the notices above;
it has no approved libOTe, coproto, macoro, function2, second cryptoTools,
FetchContent, vendored source, online Dealer, or file-polling component.

The earlier libOTe/coproto/macoro route is rejected and was not re-run. Its
unresolved macoro licensing/packaging boundary is not carried into this route.

## Implemented stage contract

| stage | roles / input | messages and opened values | output / allowed leakage |
| --- | --- | --- |
| preamble | Sender has two 128-bit message vectors; receiver has only 0/1 choices. | Each writes then verifies a fixed 48-byte preamble: magic/version, role, session, fingerprint, material ID, item count and `IKNP` protocol ID. No values are opened. | Role/config mismatch, EOF, timeout and replay hard-fail before EMP. |
| base OT + extension | Sender/receiver use a controller-created connected Unix fd. | EMP semi-honest IKNP performs its base OT and chosen-message COT wrapper over `EmpBoundedFdIO`, with one absolute poll deadline. | Sender returns counters only; receiver returns exactly selected blocks and counters. |
| conformance | TEST_ONLY controller independently supplies role inputs and checks receiver output. | `socketpair` + `fork` + `execv`; no TCP port, file, sleep, retry, online Dealer or direct selection path. | Controller-only reconstruction checks selected block equality and child exit codes. |

`EmpBoundedFdIO` handles short read/write, `EINTR`, EOF, `POLLERR`, `POLLHUP`,
`POLLNVAL` and timeout as hard errors.  Every `EINTR` poll retry recomputes
the remaining time against one absolute steady-clock deadline. It uses no
`emp::*` or OpenSSL type in the public header. A process-local
`(session,fingerprint,material_id,role)` consumption set is a **process-local
duplicate invocation guard** after a valid preamble; it is neither persistent
nor cross-process replay protection. Runtime uniqueness instead requires the
controller to allocate fresh IDs, both roles to bind them in the preamble, and
secure code never to reuse material IDs.

## Ubuntu 24.04.4 qualification result

The initial WSL observation was a name error: `wsl.exe -d Ubuntu` returned
`WSL_E_DISTRO_NOT_FOUND`. The actual installed distribution is
`Ubuntu-24.04`, which was subsequently used successfully; the earlier record
is historical and was not an EMP build failure.

- Host: Ubuntu 24.04.4 LTS, x86_64, GCC 13.3.0, CMake 3.28.3, OpenSSL
  3.0.13 (with `libssl-dev` installed as the only missing build package).
- Both pinned archives passed the hashes above; `emp-tool` and `emp-ot` built,
  installed to a fresh `/tmp/moe_m28_emp.ok9WzQ/prefix`, and upstream
  `test_base_ot` plus `test_iknp` passed 2/2 in 0.22 seconds.
- `ldd` for the upstream and project chosen-OT executables lists OpenSSL,
  libc and C++ runtime only; it has no libOTe, coproto, macoro, function2 or
  cryptoTools dynamic dependency.
- A fresh enabled Debug configuration discovers exactly 12 tests and passes
  12/12 in 4.28 seconds. The new conformance covers `n=1,2,17,128`, all-zero/all-one/
  alternating/seeded choices, distinct/equal/zero/`0xff`/boundary/seeded
  messages, invalid vector/choice/config inputs, session/fingerprint/material
  mismatch, role mismatch, truncated preamble EOF, timeout, replay and fresh
  material batches.
- One actual `n=1` trace counted EMP-only bytes (preamble excluded): sender
  sent 8843 / received 38694; receiver sent 38694 / received 8843. These are
  a conformance observation, not a benchmark.

The enabled CMake boundary is explicit and non-discovering:
`MOE_TOPK_ENABLE_EMP_OT=ON` calls `find_package(emp-tool 1.0 CONFIG REQUIRED)`
and `find_package(emp-ot 1.0 CONFIG REQUIRED)`. Qualification used only
`-DCMAKE_PREFIX_PATH=<prefix>`; absent packages hard-fail at configure time.
Only static target `moe_topk_emp_ot_backend` uses C++20 and includes EMP.
The public C++17 header uses `ProtocolIBlock128 = std::array<std::uint8_t,16>`.

## Admission condition

This closes the standalone chosen-OT boundary. M2.9 independently consumes it
for OPV/Share Translation conformance; see `M2_OPV_SHARE_TRANSLATION.md`.
Performance, LAN/WAN measurements, Permute+Share, secure shuffle, inverse
routing and complete Protocol I remain `NOT_MEASURED` or unimplemented.
