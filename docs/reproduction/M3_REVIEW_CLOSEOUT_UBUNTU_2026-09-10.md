# M3 复检整改关闭记录

日期：2026-09-10

状态：代码整改已合入 `main`；本文记录最终复检口径

代码基准：`main@bb0d0e84ce63b0560db822aa2fcc45b7d811571c`

## 1. 关闭结论

M3 已关闭 raw-score 安全入口、角色/进程隔离、logical-n GRank、DPF CTest 注册、
正式 Party executable 和可复现实验记录等复检项。整改未修改 `VFSS-baseline/`，
也未改变冻结的 M1 score semantics、M2 Protocol I 或 VFSS DPF 原语语义。

M3 保留两个不同的入口，名称、输入和轮数不得混用。

## 2. 三轮正式实现

`agarwal_protocol_iii_modular_3round` 接收 `padded_n` 个 priority-key additive
shares。GRank 的比较图只覆盖前 `logical_n` 个真实位置，随后执行 DPF routing 和
secure combine，在线因果路径共 3 轮：

```text
GRank → DPF routing → secure combine
```

该工程基线从已生成的 priority-key shares 开始，不包含 raw-score 输入适配。

## 3. Raw-score 五轮扩展

`moe_topk_protocol_iii_raw_score_modular_5round` 接收 `logical_n` 个 Q20.12
raw-score additive shares。它先执行 carry、sign 两轮安全输入适配，再进入相同的
三轮安全核心，共 5 轮：

```text
carry → sign → GRank → DPF routing → secure combine
```

这是仓库项目扩展，不是论文原生三轮入口。

## 4. 角色、进程与离线边界

TEST_ONLY E2E 控制器通过 `fork()` + `exec()` 启动独立 Dealer、Party 0 和 Party 1
进程。Dealer 仅生成、序列化和发送输入未知的离线材料，并在控制器发送输入 shares
前成功退出；在线阶段只有 Party 0 和 Party 1。父控制器关闭无关 FD，不保留会掩盖
Party 退出或 socket HUP 的协议端点副本。

正式 executable 提供 Party 角色；Dealer 是 TEST_ONLY E2E harness 角色，不能写成
第三个正式 runtime executable。12-case 验证记录了 12 次 Dealer exec 和 24 次
Party exec。测试专用 completion acknowledgement 位于协议执行和计量之外。

## 5. GRank、输入布局与 DPF 域

GRank 的 node-mask shares、comparison-edge materials、在线传输向量和
comparison-edge metrics 均按 `logical_n` 构造。priority-key 输入契约仍为
`padded_n`，DPF routing 域仍为 `2^rank_bits`。该优化没有改变输入 ABI 或 DPF
原语语义。

DPF 本地及 Peer/Dealer transport conformance 共 44 组，并已正式注册进 CTest。

## 6. Secure runtime 边界

secure runtime 不重构 raw score、priority key、rank、indicator、selected index、
最终 mask 或其他明文校验数据。输出保持为原始输入顺序下长度 `logical_n` 的 XOR
Top-K bit-mask shares。重构与 oracle 检查只存在于隔离的 TEST_ONLY 控制路径。

## 7. MetricsRecord 口径

正式 Party executable 生成各自的运行 report；TEST_ONLY E2E 控制器汇总两方 report
并输出结构化 `MetricsRecord` JSON。因此不能写成“两个正式 executable 各自生成完整
MetricsRecord”。

以下字段来自实际执行或运行配置：revision、implementation label、`n`、`K`、输入
seed、correctness、Dealer preprocessing time、online execution time、offline
material bytes、两方实际发送/接收字节、comparison-edge count、compiler、build
type、OS、CPU 和 system memory。网络 bandwidth/RTT 未测量；runtime 尚无完整可信的
online PRG 计数器，因此均保持 `NOT_MEASURED`，不作估算。

时间边界为：

- offline：Dealer launch 前至 Dealer 完成生成、序列化、发送并成功退出；
- online：控制器发送第一份输入 share 前至收到两方完整 report。

oracle 验证、测试控制 acknowledgement 和子进程清理不计入 online time。

## 8. 验收矩阵

队友提供的 Ubuntu 24.04.4、GCC 13.3.0、CMake 3.28.3、Debug、EMP-OFF 全新构建
记录覆盖以下 11 个 CTest：

1. `moe_topk_dpf_conformance_test`
2. `moe_topk_m3_grank_test`
3. `moe_topk_m3_dpf_routing_test`
4. `moe_topk_masked_mul_adapter_test`
5. `moe_topk_m3_secure_combine_test`
6. `moe_topk_m3_protocol_iii_three_process_e2e_test`
7. `moe_topk_m3_raw_score_pipeline_test`
8. `moe_topk_m3_raw_score_three_process_e2e_test`
9. `moe_topk_m3_secure_executable_test`
10. `moe_topk_m3_raw_score_secure_executable_test`
11. `moe_topk_m3_metrics_record_test`

```text
100% tests passed, 0 tests failed out of 11
Total Test time (real) = 193.59 sec
```

该 Ubuntu 结果来自队友提供的最终记录，本次文档复检没有把它伪装成独立重跑结果；
原始逐字 shell 历史未保存。可复跑命令见
[M3 Ubuntu 环境基线](M3_ENV_BASELINE_UBUNTU_2026-09-07.md)。

## 9. 后续边界

后续工作不得把五轮 raw-score 扩展称为论文原生三轮实现，不得把 priority-key
入口写成 raw-score 入口，不得修改 M1/M2 冻结语义或 `VFSS-baseline/`，也不得用
估算值填写 PRG、带宽或 RTT。M4 CipherGPT 原生基线和 M5 Protocol III 精确两轮
压缩须在独立里程碑中推进。
