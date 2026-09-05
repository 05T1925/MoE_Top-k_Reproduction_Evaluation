# M2.5/M2.6 conformance — Ubuntu 2026-09-05

Revision `2665816b61c3232758c237574209f87037a9b33c`, branch `m2-protocol-i-cmpagg-conformance`, was tested in a fresh Debug build at `/tmp/moe_topk_m2_final.3og3HR` on Ubuntu 24.04.4 LTS / WSL2 / x86_64 with GCC/G++ 13.3.0, CMake 3.28.3, OpenMP 4.5 and `-j1`.

The build configured with `cmake -S VFSS -B "$b" -DCMAKE_BUILD_TYPE=Debug`, built four M1 and five M2 targets, then ran `ctest -N`, M1 regex, M2 regex, and full CTest. Discovery was 9 tests; M1 passed 4/4, M2 passed 5/5, and full CTest passed 9/9.

M2.5 exercises `comparison_bits=34..53`, raw VFSS DCF-backed strict priority comparison, and test-only reconstruction. M2.6 uses canonical `n(n-1)/2` edges and test-only oracle differential. These are project conformance primitives, not secure shuffle, score-share widening, reverse runtime, independent-process E2E, communication, rounds, PRG, timing, or network measurements; all those fields remain `NOT_MEASURED`.
