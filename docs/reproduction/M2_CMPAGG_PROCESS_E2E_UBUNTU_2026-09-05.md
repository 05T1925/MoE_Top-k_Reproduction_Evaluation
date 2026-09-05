# M2.7 process foundation — Ubuntu 2026-09-05

Fresh Debug build: `/tmp/moe_m27_final`, Ubuntu 24.04.4 WSL2, x86_64,
GCC/G++ 13.3.0, CMake 3.28.3, `-j1`.

Commands: `cmake -S VFSS -B /tmp/moe_m27_final -DCMAKE_BUILD_TYPE=Debug`,
build the four M1 and six M2 targets with `-j1`, then `ctest --test-dir
/tmp/moe_m27_final --output-on-failure`.

Result: discovery 10; M1 4/4; M2 6/6; total 10/10.  The M2 uCMP test covers
Bin 34--53 with complete independent endpoint masks plus fixed-seed random
lower-half operands.  The process test is TEST_ONLY local IPC framing/counter
validation.  It is not secure shuffle, full Protocol I, or a network/
performance measurement; those fields remain `NOT_MEASURED`.
