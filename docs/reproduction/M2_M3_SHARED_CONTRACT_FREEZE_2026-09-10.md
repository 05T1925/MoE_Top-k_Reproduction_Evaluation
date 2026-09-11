# M2/M3 共享契约冻结：复现与验证记录

- 日期：2026-09-10
- 结论：`PARTIAL PASS`
- 任务边界：冻结现状、审计依赖、建立 fresh validation baseline；不实现 M2 三轮修复。
- 代码变更：无；本次提交只包含本记录和对应决策文档。

## 1. Git 与仓库基线

| 项目 | 结果 |
| --- | --- |
| repository root | `/Users/wentao.liu/Desktop/moe_plan` |
| branch | `codex/m2-m3-contract-freeze` |
| validation/base revision | `2a83b19dead1230a416fa093c8ca2983cdec0f8e` |
| documentation commit | `f5450cd45fdb32afe1201863b6f38898463f738e` |
| origin/main | `2a83b19dead1230a416fa093c8ca2983cdec0f8e` |
| remote | `git@github.com:05T1925/MoE_Top-k_Reproduction_Evaluation.git` |
| 初始 worktree/index | clean；`VFSS-baseline/` 无差异 |
| fetch | 已执行 `git fetch origin`；未 push、merge、rebase、reset 或覆盖用户改动 |

工作从已核对的 `origin/main` 创建短生命周期分支；复核时只允许本次两份 Markdown 进入差异。

## 2. 文档输入审计

已读取项目约束、项目说明、实施计划、团队工作计划、M2 决策文档、Protocol III modular design 和当前 M3 closeout。上一阶段任务输入误写了 `docs/decisions/M3_RAW_SCORE_SECURE_ENTRY.md`；它不是仓库缺失交付物。M3 raw-score 的实际依据是 `docs/decisions/PROTOCOL_III_MODULAR_3ROUND_DESIGN.md`、`docs/reproduction/M3_REVIEW_CLOSEOUT_UBUNTU_2026-09-10.md`、`VFSS/src/moe_topk/protocol_iii_raw_score_pipeline.cpp`、formal raw executable 和对应测试源码。

相关当前记录：

- [M2/M3 共享契约决策](../decisions/M2_M3_SHARED_CONTRACT_FREEZE.md)
- [当前 M3 Ubuntu closeout](M3_REVIEW_CLOSEOUT_UBUNTU_2026-09-10.md)

历史 M2/M3 复现记录只作为历史证据，不替代本次 fresh macOS baseline，也不被本次文档批量改写。阶段一的 `24/24`、EMP-ON `NOT_MEASURED` 和依赖错误事实保持不变。

## 3. 环境与依赖

- OS：macOS 15.7.9，Darwin 24.6.0，x86_64；主机 MacBookPro16,1；内存 16 GiB。
- CMake：4.4.3；Apple clang：17.0.0.17000013。
- OpenMP：配置阶段找到 `/usr/local/opt/libomp/include`，version 5.1。
- CMake 默认 `find_package(Eigen3 3.3 REQUIRED NO_MODULE)` 没有接受主机默认的 Eigen 5.0.1。
- 只读发现现有兼容配置：`/usr/local/Cellar/eigen@3/3.4.1/share/eigen3/cmake`；未安装、修改系统或借用旧构建目录。

## 4. Fresh EMP-OFF 验证

### 4.1 默认配置的失败事实

在全新临时目录 `/tmp/moe-m2-m3-contract-freeze-off-default.6giSNu` 执行默认配置：

```text
cmake -S VFSS -B /tmp/moe-m2-m3-contract-freeze-off-default.6giSNu \
  -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_EMP_OT=OFF
```

退出码为 `1`。原始错误为：

```text
Could not find a configuration file for package "Eigen3" that is compatible
with requested version "3.3".
/usr/local/share/eigen3/cmake/Eigen3Config.cmake, version: 5.0.1
The version found is not compatible with the version requested.
```

该失败是依赖选择问题，不是源代码修复证据；没有通过改 CMake 版本约束或安装新依赖来掩盖它。

### 4.2 现有兼容 Eigen 配置下的 fresh baseline

在另一个全新目录 `/tmp/moe-m2-m3-contract-freeze-off-compat.nDsV2g` 执行：

```text
cmake -S VFSS -B /tmp/moe-m2-m3-contract-freeze-off-compat.nDsV2g \
  -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_EMP_OT=OFF \
  -DEigen3_DIR=/usr/local/Cellar/eigen@3/3.4.1/share/eigen3/cmake
cmake --build /tmp/moe-m2-m3-contract-freeze-off-compat.nDsV2g -j2
ctest --test-dir /tmp/moe-m2-m3-contract-freeze-off-compat.nDsV2g -N
ctest --test-dir /tmp/moe-m2-m3-contract-freeze-off-compat.nDsV2g --output-on-failure
```

配置、构建、CTest discovery 和完整 CTest 均退出 `0`。CTest 发现并通过 `24/24`，总测试时间约 `224.08 s`，失败 `0`。构建确认存在两个正式入口：

- `agarwal_protocol_iii_modular_3round`
- `moe_topk_protocol_iii_raw_score_modular_5round`

通过的测试清单：

```text
moe_topk_dpf_conformance_test
moe_topk_m3_grank_test
moe_topk_m3_dpf_routing_test
moe_topk_masked_mul_adapter_test
moe_topk_m3_secure_combine_test
moe_topk_m3_protocol_iii_three_process_e2e_test
moe_topk_m3_raw_score_pipeline_test
moe_topk_m3_raw_score_three_process_e2e_test
moe_topk_m3_secure_executable_test
moe_topk_m3_raw_score_secure_executable_test
moe_topk_m3_metrics_record_test
moe_topk_m1_metrics_test
moe_topk_m1_dcf_conformance_test
moe_topk_m1_oracle_test
moe_topk_m1_cmpagg_test
moe_topk_m2_priority_key_test
moe_topk_m2_priority_dcf_conformance_test
moe_topk_m2_reverse_shuffle_model_test
moe_topk_m2_ucmp_conformance_test
moe_topk_m2_cmpagg_conformance_test
moe_topk_m2_transport_conformance_test
moe_topk_m2_cmpagg_process_e2e_test
moe_topk_m2_score_input_conformance_test
moe_topk_m2_paper_core_alignment_test
```

分类计数为 M1 `4`、M2 `9`、M3 `11`。另以 fresh DPF binary 执行的 conformance 输出为 `DPF local and Peer/Dealer transport conformance passed: 44 cases`，退出码 `0`。

## 5. EMP-ON 状态

EMP-ON 只做了只读依赖检查和全新 configure 尝试。当前主机没有 pinned `emp-tool`/`emp-ot` CMake config，也没有可用的 brew formula；`pkg-config` 命令本身也不可用。

在 `/tmp/moe-m2-m3-contract-freeze-on-missing.BY1CZs` 使用已有 Eigen 3.4.1 执行：

```text
cmake -S VFSS -B /tmp/moe-m2-m3-contract-freeze-on-missing.BY1CZs \
  -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_EMP_OT=ON \
  -DEigen3_DIR=/usr/local/Cellar/eigen@3/3.4.1/share/eigen3/cmake
```

退出码为 `1`，原始错误为：

```text
Could not find a package configuration file provided by "emp-tool"
(requested version 1.0)
```

因此 EMP-ON build/discovery/test 为 `NOT_MEASURED`；没有安装依赖、改系统配置或借用历史 Ubuntu 输出。历史 Ubuntu 记录仍可作为历史证据，但不填充当前主机指标。

## 6. M3 依赖静态审计

对 M3 formal/raw 入口、`protocol_iii_*` 源码/头文件及 M3 测试执行禁止调用搜索，未发现：

```text
protocol_i_shuffle_forward_party
protocol_i_shuffle_reverse_party
protocol_i_priority_pipeline_party
protocol_i_secret_shared_shuffle
protocol_i_pipeline
```

当前调用链为：

```text
raw_score_pipeline
  -> protocol_i_raw_score_input_party (2 rounds)
  -> protocol_iii_grank_party (1 round)
  -> protocol_iii_dpf_routing_party (1 round)
  -> protocol_iii_secure_combine_party (1 round)
```

对应代码证据：

- [raw score pipeline](../../VFSS/src/moe_topk/protocol_iii_raw_score_pipeline.cpp:156) 依次调用输入适配、GRank、DPF routing、secure combine，并强制 `2 + 3 = 5`。
- [GRank](../../VFSS/src/moe_topk/protocol_iii_grank.cpp:250) 使用独立的 masked CmpAgg exchange；没有 M2 forward shuffle 或 rank reveal。
- [DPF routing](../../VFSS/src/moe_topk/protocol_iii_dpf_routing.cpp:271) 只有一个 masked-rank exchange，并清理 one-shot material。
- [secure combine](../../VFSS/src/moe_topk/protocol_iii_secure_combine.cpp:210) 在原始顺序生成 XOR mask share；不重构最终 mask。
- [secure core](../../VFSS/src/moe_topk/protocol_iii_secure_core.cpp:199) 只编排上述三阶段，并检查输出 bit 和材料清理。

M3 production source 中未发现 `true_rank`、`sleep`、明文重构调用或 selected-index 输出；出现的 `key.bin` 仅是 DPF key 字段，不是文件轮询同步机制。静态搜索与测试通过不等于论文 exact leakage 已证明。

## 7. 共享契约验证结论

- 已冻结 signed Q20.12、modulo-`2^32` raw additive shares、stable tie、`logical_n`/`padded_n`、原始顺序 XOR bit-mask、P0/P1/P2 角色和 frame/material binding；frame/header 与 material record 的字段边界见决策文档。
- `ProtocolIPartyPackage` 类型和部分 material 类型可以作为共享记录表示，但 M2 的 padded comparison graph/package 实例不得直接交给按 logical graph 构造的 M3；两者的 node masks、edge materials、count 和序列化内容必须独立生成。
- 已确认 M3 不依赖 M2 shuffle、reverse carrier、shuffled-rank reconstruction/rank reveal 或 M2 pipeline。
- M2 当前 C-level 的 `2 + 4 + 2 = 8` 与 M2 predecessor 的 `4 + 2 = 6` 历史标签继续保留；不被 M3 的 `3`/`5` 标签覆盖。
- M3 有自己的 `protocol_iii_metrics_record.*` factory；当前测试证明 schema/record 行为，不能扩写为 EMP-ON 或生产性能证明。

## 8. 剩余门槛

本次达到“契约冻结 + EMP-OFF fresh baseline + M3 静态依赖审计”，但整体为 `PARTIAL PASS`，原因只有明确可复核的环境门槛：当前主机 EMP-ON 为 `NOT_MEASURED`，默认 Eigen 配置不能直接 configure，必须显式选择现有兼容 Eigen 3.4.1 配置。M2 exact 3-round、论文精确泄露和生产部署不在本次阶段一完成范围。
