# M2.4 primitive conformance — Ubuntu 2026-09-05

Revision: `bfcee9bc7bf83c8749d44a51578b902b1051489b`; Debug build in Ubuntu 24.04.4 LTS / WSL2 / x86_64, GCC/G++ 13.3.0, CMake 3.28.3, OpenMP 4.5, `-j1`. Build directory: `/tmp/moe_topk_m2_4_final.9FGrFQ`.

Commands: `cmake -S VFSS -B "$b" -DCMAKE_BUILD_TYPE=Debug`; build the four `moe_topk_m1_*` targets and three `moe_topk_m2_*` targets with `-j1`; then `ctest -N`, M1 regex, M2 regex, and full `ctest --output-on-failure`.

`ctest -N` found 7 tests. M1 passed 4/4; M2.4 passed 3/3; full CTest passed 7/7. Raw DCF coverage used Bin 34, 40, 41, 43, 47, 50, 53 and asserts only raw `x < threshold`. The reverse test is a TEST_ONLY ideal Permute+Share algebra model.

An earlier new-directory `-j2` link of `libsytorch.a` ended with a Bus error; retrying from a new directory with `-j1` passed. This is not classified as a protocol-test failure. Secure shuffle, masked uCMP/range proof, transport, E2E, communication, rounds, PRG, network, and performance are `NOT_MEASURED`.
