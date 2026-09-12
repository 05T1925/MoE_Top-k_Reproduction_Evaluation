# M2 Protocol I Dealer-preprocessed 3-round candidate Windows validation

日期：2026-09-12

## 1. 最终状态

- `ENVIRONMENT_BLOCKED`
- `BUILD_BLOCKED`
- `CORE_RUNTIME_BLOCKED`
- `ADAPTER_ENTRY_BLOCKED`

本记录是阶段三D在当前 Windows 工作站上的资格化结果。目标候选仍为
`m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate`。本机不是 Ubuntu，且没有
CMake/CTest 或 EMP-ON (`emp-tool`/`emp-ot`) 配置包，因此没有进入真实 candidate build graph。
不得把本轮结果表述为 candidate 编译、链接、conformance、E2E 或三轮运行通过。

## 2. Git identity

- repository: `git@github.com:05T1925/MoE_Top-k_Reproduction_Evaluation.git`
- initial HEAD: `58dda3118d337aed8f536762338fe6f4dda7151c`
- final HEAD: `58dda3118d337aed8f536762338fe6f4dda7151c`
- branch: `codex/m2-candidate-ubuntu-validation`
- upstream: none (local validation branch)
- worktree: clean after documentation change

交接 HEAD 与阶段三D要求一致；未修改 main、未 push、未 merge、未创建 PR。

## 3. Environment qualification

检查命令在 PowerShell 中执行：

```text
Get-CimInstance Win32_OperatingSystem
Get-CimInstance Win32_Processor
Get-Command cmake,ctest,openssl
Get-ChildItem C:\ -Recurse -Include emp-toolConfig.cmake,emp-tool-config.cmake,emp-otConfig.cmake,emp-ot-config.cmake
```

结果：

- OS: Windows 11 Home 10.0.26200, x64（不是 Ubuntu/Linux）
- CPU: 13th Gen Intel(R) Core(TM) i9-13980HX, 24 cores / 32 logical processors
- compiler available: MinGW-w64 `c++` 8.1.0
- CMake: `NOT_FOUND` in PATH and local searched locations
- CTest: `NOT_FOUND` in PATH and local searched locations
- Eigen3 CMake config: `NOT_FOUND`
- emp-tool CMake config: `NOT_FOUND`
- emp-ot CMake config: `NOT_FOUND`
- OpenSSL: Git MinGW OpenSSL 3.5.7 executable and headers/libs were found, but this does not provide the missing CMake/EMP stack

WSL probe (`wsl.exe -e bash -lc ...`) failed because no usable WSL distribution is installed.
No package manager, system compiler, OpenSSL, CMake, EMP package, or global dependency was installed or modified.

## 4. First blocking command and root cause

The required configure command cannot be executed because the host does not provide `cmake`:

```text
cmake -S VFSS -B <fresh-build> -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EMP_OT=ON
```

PowerShell result: `The term 'cmake' is not recognized ...`.

Even if CMake were supplied, the repository requires
`find_package(emp-tool 1.0 CONFIG REQUIRED)` and
`find_package(emp-ot 1.0 CONFIG REQUIRED)` when `MOE_TOPK_ENABLE_EMP_OT=ON`; neither package
was found on this host. The blocker is therefore environment qualification, not a candidate source
failure. Per stage three D, no speculative source or CMake fallback was added.

## 5. Source and security audit

No candidate source files were modified. The existing package/core/app were inspected for the required
secure-path hazards. Static inspection found no candidate-side rank reconstruction, rank reveal, input
file polling, `sleep`, or online Dealer fallback. This is static evidence only; it does not establish
runtime behavior.

The candidate remains the documented rank-share-only path:

- output: `public_masked_list`, `shuffled_rank_share`
- no `selection_bit_share`
- no original-order mask
- no reverse/output adapter

## 6. Build and test evidence

| Evidence | Status | Reason |
| --- | --- | --- |
| EMP-ON configure | `NOT_MEASURED` | CMake unavailable; EMP packages absent |
| Candidate compile/link | `NOT_MEASURED` | No configure graph |
| Package/material conformance | `NOT_MEASURED` | Candidate test cannot be built |
| Public-y / same-r differential | `NOT_MEASURED` | Candidate test cannot run |
| Rank-share oracle differential | `NOT_MEASURED` | Candidate test cannot run |
| Fork+exec Dealer/P0/P1 E2E | `NOT_MEASURED` | Candidate executable cannot be built |
| FD lifecycle, peer-exit, timeout | `NOT_MEASURED` | No runtime |
| R1/R2/R3 causal trace | `NOT_MEASURED` | No runtime counters |
| M2 candidate regression | `NOT_MEASURED` | Candidate EMP-ON graph absent |
| M3 regression | `NOT_MEASURED` in this fresh run | No CMake/CTest |
| Full CTest | `NOT_MEASURED` in this fresh run | No CMake/CTest |
| `BUILD_TESTING=OFF` | `NOT_MEASURED` in this fresh run | No CMake |

Historical EMP-OFF/EMP-ON results in earlier records were not reused as current-revision evidence.
No performance or communication number was estimated.

## 7. Adapter entry review

`ADAPTER_ENTRY_BLOCKED` remains unchanged. The candidate exposes additive shuffled-domain rank
shares, while the proposed adapter requires secure selection carrier shares
`selection_bit_share = [rank < K]`. Rank-to-selection is nonlinear and cannot be obtained from
the two reverse-shuffle rounds. No output adapter, seven-round label, rank reveal, or test-only
carrier bridge was added.

## 8. Paper-exact boundary

This result does not change the existing paper-exact design gate. The candidate is still only a
project extension pending real Ubuntu/EMP-ON evidence; it is not Agarwal Protocol I exact, not a
paper-exact three-round implementation, not a complete Protocol I reproduction, and not an
exact-leakage-equivalent secure shuffle.

## 9. Remaining action

Run the prescribed fresh EMP-ON configure/build/test sequence on an Ubuntu 24.04 environment with
working CMake/CTest, Eigen 3.3+, OpenSSL development files, and pinned `emp-tool` 1.0 plus `emp-ot`
1.0 CMake packages. Preserve the resulting command output and runtime counters before reopening
`CORE_RUNTIME_GO` or the adapter gate.

