# BMW16 S12 common-tape and process tests (TEST_ONLY)

This directory supports the opt-in `BMW16_S12_CPP_TP0P1_EXPERIMENTAL_TEST_ONLY`
candidate. It is not a secure protocol package. The C++ party API lives under
`VFSS/`; the Python code here only provisions a deterministic test tape and
compares C++ trace files with the frozen S4 Python source.

## Build and run

From the repository root in PowerShell:

```powershell
wsl.exe -d Ubuntu-24.04 -- bash -lc 'cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s12/MoE_Top-k_Reproduction_Evaluation/VFSS -B /tmp/bmw16-s12-build -DCMAKE_BUILD_TYPE=Release -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON'
wsl.exe -d Ubuntu-24.04 -- cmake --build /tmp/bmw16-s12-build --target moe_topk_bmw16_experimental_3proc_test moe_topk_bmw16_experimental_party_test -j2
wsl.exe -d Ubuntu-24.04 -- ctest --test-dir /tmp/bmw16-s12-build -R '^moe_topk_bmw16_experimental_party_test$' --output-on-failure
wsl.exe -d Ubuntu-24.04 -u root -- /tmp/bmw16-s12-build/moe_topk_bmw16_experimental_3proc_test
```

The three-process TEST_ONLY harness needs root only to create separate private
fixture directories and drop P0/P1 to UIDs 22012/22013. It starts a separate T
process, waits for T to exit, and only then creates the two raw input shares.
The root controller provisions test credentials and performs the final oracle
check; it is not part of either party view.

## S4/C++ common random tape

`run_common_tape_matrix.py` creates a `BMW16_S12_TAPE_V1` file outside the C++
process. Both S4 Python and the C++ TEST_ONLY trace consume the same 64-bit words
for both tasks' R1/R3 sampling. The matrix runs every K for n=2..8 plus four
n=64 profiles. A Windows Python with NumPy runs the S4 comparator; C++ executes
in WSL:

```powershell
py experiments/TEST_ONLY_BMW16_S12/run_common_tape_matrix.py --wsl-distro Ubuntu-24.04 --cpp-test /tmp/bmw16-s12-build/moe_topk_bmw16_experimental_party_test --s4 C:/Users/28641/.codex/worktrees/bmw16-s4/MoE_Top-k_Reproduction_Evaluation/experiments/TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R/select4r.py --out experiments/TEST_ONLY_BMW16_S12/evidence/common_tape_all_ranks
```

The per-case tape, C++ trace, exact differential JSON and aggregate manifest are
written under `evidence/`. These traces deliberately contain test-only mappings
needed for differential checking. Do not connect them to party code or use the
fixed test tape as protocol randomness. The runtime party API obtains fresh
sampling coins from the OS and combines the two party contributions.

The S4 source path above is an external read-only S4 worktree and is pinned by
SHA-256 in `compare_common_tape.py`. n=128 is outside the currently implemented
finite domain (`n<=64`) and is not simulated or reported as run.
