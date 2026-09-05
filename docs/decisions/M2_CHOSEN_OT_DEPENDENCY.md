# M2.8 EMP chosen-OT dependency qualification

Status: **not admitted; `m2_emp_iknp_chosen_ot_conformance` remains blocked.**

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

## Required stage gate before source is admitted

| stage | required evidence | present result |
| --- | --- | --- |
| dependency configure/build/install | fresh Linux/Darwin build from the pinned archives | **blocked**: this host has no registered Ubuntu WSL distribution and emp-tool rejects Windows/MSYS. |
| upstream primitive smoke | run the upstream base-OT and IKNP two-party CTest cases from that build | `NOT_RUN` (no supported host). |
| isolated backend compile | C++20-only private static `moe_topk_emp_ot_backend`; public C++17 headers contain only `std::array<uint8_t,16>` | `NOT_RUN`; source intentionally not added before its dependency can be compiled. |
| chosen-OT conformance/differential/E2E | fresh `socketpair` + `fork` + `execv` role processes; real EMP IKNP bytes and no choice disclosure to sender | `NOT_RUN`; no simulated/direct-selection substitute exists. |
| project acceptance | default-off fresh Debug 11/11 and enabled fresh Debug 12/12 | default-off remains M2.7 evidence only; enabled result is `NOT_RUN`. |

The intended enabled CMake boundary remains explicit and non-discovering:
`MOE_TOPK_ENABLE_EMP_OT=ON` with `EMP_TOOL_DIR` and `EMP_OT_DIR` pointing to
the installed package-config directories. It must fail at configure time if
either is missing. The backend must be the only target including EMP/OpenSSL;
the public API must use `ProtocolIBlock128 = std::array<std::uint8_t, 16>`.

## Admission condition

Resume only on Ubuntu or another supported POSIX host with CMake, a supported
compiler and OpenSSL 3. Build/install the exact archives in a fresh temporary
directory, preserve the link-dependency output, run upstream `test_base_ot`
and `test_iknp`, then implement and test the bounded-fd adapter. Until that
evidence exists, adding a plausible but uncompiled EMP backend would violate
the repository's conformance and provenance rules.
