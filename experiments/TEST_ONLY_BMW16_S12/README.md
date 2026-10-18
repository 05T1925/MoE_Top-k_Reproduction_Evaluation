# BMW16 common-tape differential harness (TEST_ONLY)

This directory contains the common-tape harness originally added in S12 and
reused to verify the S25 opt-in VFSS candidate. It is not a secure protocol
package. The C++ party API lives under `VFSS/`; Python provisions deterministic
TEST_ONLY tapes and compares C++ traces with the frozen S4 source at
`experiments/TEST_ONLY_BMW16_S7_COMPOSITION/select4r_s4_source.py`.

## Build and run

From the repository root, configure the opt-in Release build and run the
differential harness. Replace `<repo>` and `<outside-repository>` with the
matching WSL and Windows paths. All generated tapes, traces and logs must stay
outside the Git worktree.

```powershell
wsl.exe -d Ubuntu-24.04 -- bash -lc 'cmake -S <repo>/VFSS -B /tmp/moe_bmw16_s25_on -DCMAKE_BUILD_TYPE=Release -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON -DMOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=OFF'
wsl.exe -d Ubuntu-24.04 -- cmake --build /tmp/moe_bmw16_s25_on --target moe_topk_bmw16_experimental_party_test -j2
py experiments/TEST_ONLY_BMW16_S12/run_common_tape_matrix.py --wsl-distro Ubuntu-24.04 --cpp-test /tmp/moe_bmw16_s25_on/moe_topk_bmw16_experimental_party_test --s4 experiments/TEST_ONLY_BMW16_S7_COMPOSITION/select4r_s4_source.py --out <outside-repository>/BMW16_S25/common_tape_all_ranks
```

The harness compares both Select tasks' R1/R3 sampling, each round's edge plan
and result bits, intermediate state, abort class and selected anonymous handle.
It covers every K for n=2..8 and four n=64 profiles. The common tape is test
input only; the party runtime uses the OS-CSPRNG coin exchange.

The independent-process runner `moe_topk_bmw16_experimental_party_node_e2e_test`
must be run by a TEST_ONLY root controller because its fixtures create distinct
UIDs 22012/22013. It starts T separately, waits for T exit, then creates input
shares and launches P0/P1. The controller's oracle reconstruction is outside
party processes. The S25 acceptance report records the exact commands and
outputs; no generated evidence is stored in this source directory.

The S4 Python file is a byte-for-byte frozen TEST_ONLY copy pinned by SHA-256
in `compare_common_tape.py`; it is not compiled into a party binary or queried
by the protocol.
