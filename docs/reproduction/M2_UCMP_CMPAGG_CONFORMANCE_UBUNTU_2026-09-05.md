# M2.5/M2.6 conformance — Ubuntu 2026-09-05

## Scope and revision

The implementation under test was revision
`2665816b61c3232758c237574209f87037a9b33c` on branch
`m2-protocol-i-cmpagg-conformance`.  This record is for project primitive/core
conformance only.  It does not promote local reference behavior or test-only
reconstruction to a paper conclusion or to a complete Protocol I result.

## Actual environment

The commands were run in the explicitly selected `Ubuntu-24.04` WSL2
distribution, not the machine's separate default Ubuntu 20.04 distribution.

| Field | Observed value |
| --- | --- |
| OS | Ubuntu 24.04.4 LTS (Noble Numbat) |
| Kernel | `6.6.87.2-microsoft-standard-WSL2` |
| Architecture | `x86_64` |
| CPU | 13th Gen Intel(R) Core(TM) i9-13980HX; 16 cores / 32 logical CPUs |
| Memory at capture | 8,122,376,192 bytes total; 7,522,246,656 bytes available |
| GCC / G++ | 13.3.0 (`gcc`/`g++` packages 13.2.0-7ubuntu1) |
| CMake | 3.28.3 |
| Eigen | `libeigen3-dev` 3.4.0-4build0.1 |
| OpenMP | `libomp-dev` 1:18.0-59~exp2; OpenMP 4.5 |
| Configuration | fresh `Debug` build; single-job build (`-j1`) |

No compiler warnings were observed in the final target build output.

## Commands and results

The fresh build directory was `/tmp/moe_topk_m2_final.3og3HR`.

```sh
cmake -S VFSS -B /tmp/moe_topk_m2_final.3og3HR -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/moe_topk_m2_final.3og3HR --target \
  moe_topk_m1_oracle_test moe_topk_m1_cmpagg_test \
  moe_topk_m1_metrics_test moe_topk_m1_dcf_conformance_test \
  moe_topk_m2_priority_key_test moe_topk_m2_priority_dcf_conformance_test \
  moe_topk_m2_reverse_shuffle_model_test moe_topk_m2_ucmp_conformance_test \
  moe_topk_m2_cmpagg_conformance_test -j1
ctest --test-dir /tmp/moe_topk_m2_final.3og3HR -N
ctest --test-dir /tmp/moe_topk_m2_final.3og3HR -R '^moe_topk_m1_' --output-on-failure
ctest --test-dir /tmp/moe_topk_m2_final.3og3HR -R '^moe_topk_m2_' --output-on-failure
ctest --test-dir /tmp/moe_topk_m2_final.3og3HR --output-on-failure
```

`ctest -N` discovered exactly nine tests.  The M1 subset passed 4/4, the M2
subset passed 5/5, and the complete suite passed 9/9:

1. `moe_topk_m1_metrics_test`
2. `moe_topk_m1_dcf_conformance_test`
3. `moe_topk_m1_oracle_test`
4. `moe_topk_m1_cmpagg_test`
5. `moe_topk_m2_priority_key_test`
6. `moe_topk_m2_priority_dcf_conformance_test`
7. `moe_topk_m2_reverse_shuffle_model_test`
8. `moe_topk_m2_ucmp_conformance_test`
9. `moe_topk_m2_cmpagg_conformance_test`

## What this establishes

M2.5 exercises the raw VFSS DCF-backed strict priority comparison for
`comparison_bits=34..53` and uses test-only reconstruction for conformance.
M2.6 uses the canonical `n(n-1)/2` edge enumeration, consumes one fresh
uCMP material per edge and party, and differentially checks reconstructed
rank shares against the frozen M1 oracle in its test fixture.

The reverse-shuffle test remains explicitly `TEST_ONLY`: an algebraic ideal
Permute+Share model, not a secure shuffle or runtime primitive.  It must not
be treated as OT/OPV, communication, security, or online-round evidence.

Secure shuffle, score-share-to-wide-key conversion, secure inverse runtime,
independent-process E2E, transport/byte accounting, measured causal rounds,
PRG-call counts, timing, network behavior, and performance are all
`NOT_MEASURED`.  Therefore this result neither completes Protocol I nor
closes its outstanding D1--D4 design/security gates.
