# Stage 3E: M2 Protocol I three-round candidate validation

日期：2026-09-12

## Executive conclusion

当前候选：`m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate`。

- `ENVIRONMENT_BLOCKED`
- `BUILD_BLOCKED`
- `CORE_RUNTIME_BLOCKED`
- `ADAPTER_ENTRY_BLOCKED`

本轮无法获得 candidate runtime 证据。当前执行主机是 Windows 11，不是要求的 Ubuntu
24.04；且没有可执行的 CMake/CTest、Eigen3 CMake 配置、`emp-tool` 或 `emp-ot` 包。
没有安装或修改任何依赖，也没有使用 EMP-OFF、历史结果、声明桩或 fake header 绕过门槛。

## Git identity and revision

- repository: `git@github.com:05T1925/MoE_Top-k_Reproduction_Evaluation.git`
- initial HEAD: `77f3b09b87afb3dc91138a5137c2f5d87ed73eca`
- final HEAD: recorded by the commit containing this document
- branch: `codex/m2-candidate-ubuntu-validation`
- upstream: none
- worktree: clean after the documentation commit
- expected Stage 3E HEAD: `77f3b09b87afb3dc91138a5137c2f5d87ed73eca` (matched at start)

## Environment qualification

Read-only checks:

```text
Get-CimInstance Win32_OperatingSystem
Get-CimInstance Win32_Processor
where.exe cmake
where.exe ctest
where.exe ninja
where.exe g++
where.exe clang++
where.exe openssl
Get-ChildItem C:\ -Recurse -Include Eigen3Config.cmake,emp-toolConfig.cmake,emp-otConfig.cmake
```

Observed:

- OS: Windows 11 Home 10.0.26200, x64; Ubuntu 24.04 unavailable
- CPU: Intel Core i9-13980HX, 24 cores / 32 logical processors
- compiler: MinGW-w64 `g++` 8.1.0
- CMake: `NOT_FOUND`
- CTest: `NOT_FOUND`
- Ninja: found under the Python installation, but unusable without CMake and not a substitute
- Eigen3: `Eigen3Config.cmake` not found
- emp-tool: `emp-toolConfig.cmake` / `emp-tool-config.cmake` not found
- emp-ot: `emp-otConfig.cmake` / `emp-ot-config.cmake` not found
- OpenSSL: Git MinGW OpenSSL 3.5.7 executable and MinGW OpenSSL headers/libs exist
- WSL: probe did not provide a usable Ubuntu distribution
- dependency installation/modification: none

## Dependency matrix

| Dependency | Required | Current evidence | Status |
| --- | --- | --- | --- |
| Ubuntu 24.04 x86_64 | yes | Windows host | `BLOCKED` |
| CMake / CTest | yes | executables absent | `BLOCKED` |
| Eigen3 CMake package | yes | config absent | `BLOCKED` |
| emp-tool 1.0 CONFIG | yes | config absent | `BLOCKED` |
| emp-ot 1.0 CONFIG | yes | config absent | `BLOCKED` |
| OpenSSL development files | yes | headers/libs present, CMake path unavailable | `PARTIAL` |

## CMake configure result

Fresh directory:

```text
C:\Users\28641\Desktop\MoE_Top-k_Reproduction_Evaluation\.tmp-stage3e-build-20260912
```

Command:

```text
cmake -S VFSS -B C:\Users\28641\Desktop\MoE_Top-k_Reproduction_Evaluation\.tmp-stage3e-build-20260912 -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EMP_OT=ON
```

Result: `FAIL / ENVIRONMENT_BLOCKED` before CMake execution:

```text
The term 'cmake' is not recognized as a name of a cmdlet, function, script file, or executable program.
```

The repository's next required checks (`find_package(emp-tool 1.0 CONFIG REQUIRED)` and
`find_package(emp-ot 1.0 CONFIG REQUIRED)`) could not be reached. No build graph was generated.

## Compile and link result

`NOT_MEASURED`: no configure graph, candidate targets, compile commands, or link commands.
No fake headers, OpenSSL declaration stubs, manual ABI, or hand-written linker invocation was used.

## Conformance and differential results

All runtime items below are `NOT_MEASURED` because the candidate test could not be configured:

- package/material serialization and negative conformance;
- material binding and one-shot/replay checks;
- public-y and same-r differential;
- two real forward Permute+Share passes;
- local DCF/CmpAgg shuffled rank-share differential;
- independent-process Dealer/P0/P1 fork+exec E2E;
- peer EOF/POLLHUP, timeout, child exit and FD allowlist cases;
- actual R1/R2/R3 transport trace and causal barrier count;
- M2/M3/full CTest;
- `BUILD_TESTING=OFF` production build.

Historical results were not reused as current-revision evidence.

## Secure-path static audit

`STATIC_ONLY`: existing candidate package/core/app were inspected for reconstruction, rank reveal,
file polling, `sleep`, online Dealer fallback, and output-adapter insertion. No such candidate-side
path was identified in the inspected source. This does not prove runtime behavior or promote any
runtime gate.

The candidate output remains `public_masked_list` plus `shuffled_rank_share`; it does not provide
`selection_bit_share`, reverse shuffle, or original-order mask.

## Current status and blockers

The first blocking assumption is environment qualification, not a candidate source error. The
required next stage is a clean Ubuntu 24.04 x86_64 host with CMake/CTest, Eigen3, OpenSSL
development files, and pinned `emp-tool` 1.0 plus `emp-ot` 1.0 CMake packages. Only then may the
prescribed configure, candidate build, conformance, differential, E2E, and trace sequence run.

The formal M2 label remains `m2_protocol_i_raw_score_input_modular_8round_mask_output`. The
three-round candidate is a project extension, Dealer-preprocessed, at most one-corrupt-party
non-colluding model, and shuffled-domain rank-share candidate. It is not Agarwal Protocol I
paper-exact, a complete Protocol I reproduction, a seven-round unified path, an original-order
Top-K mask output, or an exact-leakage-equivalent implementation.

`ADAPTER_ENTRY_BLOCKED` remains unchanged because rank-share to selection-carrier conversion is
nonlinear; reverse shuffle cannot perform it for free. The paper-exact Gate remains
`BLOCKED / NOT_VERIFIED`.

## Modified files

Only this Stage 3E evidence document was added. No `VFSS/` source, formal M2/M3 runtime,
`VFSS-baseline/`, `Papers/`, `Agarwal_TopK/`, `ADSMPC/`, or `CipherGPT/` files were modified.

## Exact commands executed

```text
git status --short --branch
git rev-parse HEAD
git rev-parse --abbrev-ref HEAD
git rev-parse --abbrev-ref --symbolic-full-name '@{upstream}'
git log --oneline --decorate -12
git diff --check
Get-CimInstance Win32_OperatingSystem
Get-CimInstance Win32_Processor
where.exe cmake
where.exe ctest
where.exe ninja
where.exe g++
where.exe clang++
where.exe openssl
Get-ChildItem C:\ -Recurse -Include Eigen3Config.cmake,emp-toolConfig.cmake,emp-otConfig.cmake
cmake -S VFSS -B <fresh-stage3e-build> -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EMP_OT=ON
git diff --check
git status --short --branch
```

## Recommended next stage

在具备全部 pinned Ubuntu/EMP 依赖的环境中重新执行阶段三E；保存 configure/build/test 原始
输出、实际 child 生命周期、R1/R2/R3 counters 和失败矩阵后，再重新审查 `CORE_RUNTIME_GO`。
在此之前不得实现 output adapter 或修改七轮、paper-exact、最终 mask 结论。

