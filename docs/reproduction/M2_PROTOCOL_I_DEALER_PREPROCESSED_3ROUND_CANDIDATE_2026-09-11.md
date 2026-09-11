# M2 Protocol I Dealer-preprocessed 3-round candidate implementation record

日期：2026-09-11。仓库：`/Users/wentao.liu/Desktop/moe_plan`。

结论：**IMPLEMENTATION_BLOCKED**。候选实现、包格式、三屏障 core、独立角色和
TEST_ONLY controller 已加入隔离路径，但本机没有 `emp-tool`/`emp-ot` 的 CMake
package，因此无法执行需要 EMP-ON 的真实 shuffle/DCF/OT 和 fork+exec E2E。不得
将以下结果写成 `SECURE_CANDIDATE_GO`、paper-exact 或 Protocol I reproduction
complete。

阶段三C验证将该状态进一步拆分为 `candidate source authored: PASS`、real dependency
compile/link/runtime: `NOT_MEASURED`、`CORE_RUNTIME_BLOCKED`；由于输出仍是
shuffled-domain rank shares 而不是 selection carrier shares，adapter 入口为
`ADAPTER_ENTRY_BLOCKED`。阶段三C没有新增 adapter 源码，详见对应 adapter gate
decision record。

## 1. Revision and scope

- 基准提交：`b88da43eb256a6c609331ca8f68a90a2e638cff4`。
- 当前实现分支：`codex/m2-dealer-preprocessed-3round-candidate`。
- 本记录覆盖固定标签 `m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate`。
- 没有修改当前 M2 8-round 主路径、M3/M4/M5，未修改 `VFSS-baseline/` 或本地参考目录。

## 2. Fresh checks

环境为 macOS AppleClang 17、Eigen 3.4.1、libomp；项目默认选择的 Eigen 5 不满足
仓库的 `find_package(Eigen3 3.3)`，改用本机已安装的 3.4.1 配置目录后继续验证。

### Candidate source boundary

使用临时 OpenSSL 声明桩，仅执行 `-fsyntax-only`，四个新增 C++ 文件在
`-std=c++17 -Wall -Wextra -Wpedantic -Werror` 下通过。该检查不证明 OpenSSL、EMP、
OT、DCF 链接或运行时正确性。

静态人工/文本审计确认生产 candidate package/core/app 没有明文 score、两方 input
share、rank reveal、rank reconstruction、reverse shuffle、Top-K oracle、文件同步、
sleep 或在线 Dealer；`std::reverse` 仅是测试可控的 local permutation mode，不是
reverse protocol adapter。

### EMP-OFF

命令：

```text
cmake -S VFSS -B <build-off> -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=OFF -DMOE_TOPK_ENABLE_EMP_OT=OFF \
  -DEigen3_DIR=/usr/local/Cellar/eigen@3/3.4.1/share/eigen3/cmake
cmake --build <build-off> -j2
```

结果：完整 `BUILD_TESTING=OFF` 生产构建通过（100%）；候选 production/test target
在目标列表中均不存在，证明测试 controller 不会进入该构建。该配置不编译候选，因
候选明确依赖 EMP-ON backend。

另行以 `BUILD_TESTING=ON`、`MOE_TOPK_ENABLE_EMP_OT=OFF` 完成 fresh build 和
`ctest --test-dir <build-onoff> --output-on-failure`：**25/25 passed**，总用时约
204.26 秒，覆盖当前 M2/M3/full CTest；这只是既有路径回归，不是候选 runtime 证据。

### EMP-ON

命令：

```text
cmake -S VFSS -B <build-on> -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EMP_OT=ON \
  -DEigen3_DIR=/usr/local/Cellar/eigen@3/3.4.1/share/eigen3/cmake
```

结果：在 `VFSS/CMakeLists.txt:279` 的
`find_package(emp-tool 1.0 CONFIG REQUIRED)` 失败，未生成 EMP-ON build graph。
候选 package conformance、实际 DCF/CmpAgg、两轮 PS、R3、候选 CTest、
fork+exec E2E、真实 bytes/trace 和运行时 timing 均为 `NOT_MEASURED`。

## 3. Gate B disposition

| Gate B 项目 | 状态 | 证据边界 |
| --- | --- | --- |
| 独立 package/material schema | IMPLEMENTED, NOT_MEASURED at runtime | 新 package 为 move-only，绑定版本、标签、身份、shape、宽度、edge stage/slot/party 和 one-shot；EMP-ON 未链接 |
| P2 full-r preprocessing / exit | IMPLEMENTED, NOT_MEASURED at runtime | P2 只接收 public config，生成并清理完整 offline mask，发送两个 party package 后退出；未运行 executable |
| 两次真实 forward PS | NOT_MEASURED | 调用现有 `protocol_i_shuffle_forward_party`，但依赖 EMP-ON |
| R3 public masked-list opening | NOT_MEASURED | 独立 framed channel 已实现，未运行 |
| public `y` 与同一 `r` differential | NOT_MEASURED | TEST_ONLY oracle 已实现，未运行 |
| local DCF/CmpAgg rank shares | NOT_MEASURED | 使用 party-local edge material，未运行 |
| trace/actual rounds | NOT_MEASURED | 代码从实际 PS counters 和 R3 channel counters 生成，未运行 |
| mismatch/replay/truncation/peer-exit/timeout | PARTIAL | package negative 和 transport negative 已写入 TEST_ONLY；未完成 EMP-ON 编译运行 |
| fork+exec、P2 先退出、FD allowlist | NOT_MEASURED | controller 代码按顺序 fork+exec/waitpid，未运行 |
| secure-source audit | PASS (static) | 新 secure package/core/app 未接入 oracle/reveal/reverse/file polling |
| 当前 M2/M3/full CTest | PASS (EMP-OFF existing suite) | fresh 25/25；候选不在 EMP-OFF graph |
| `BUILD_TESTING=OFF` isolation | PASS (EMP-OFF) | fresh 100% build，候选 target absent |
| EMP-ON dependency/runtime | BLOCKED | 缺少 emp-tool CMake package，见上文 |

## 4. Fixed non-claims

P2 知道完整 offline `r` 是项目候选安全模型的显式授权，最多单角色腐化且不串谋；
它不是论文 exact Dealer view。论文 Gate C 仍为 BLOCKED。候选也没有实现 raw-score
adapter、reverse/output mask、original-order XOR mask、7-round path 或 rank opening。

## 5. Follow-up required for GO

在具备 EMP-ON 依赖的干净环境中，必须重新配置、编译并运行候选 package/conformance、
negative、oracle differential、三进程 fork+exec E2E，以及现有 M2/M3/full CTest；
只有全部证据和本记录的 FD/leakage 检查共同通过，才可将 decision record 更新为
`SECURE_CANDIDATE_GO`。
