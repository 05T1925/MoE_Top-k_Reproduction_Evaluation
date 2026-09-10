# M3 复检整改关闭记录

日期：2026-09-10  
状态：已完成  
基准分支：`main`  
基准提交：`bb0d0e84ce63b0560db822aa2fcc45b7d811571c`

## 1. 关闭结论

M3 复检发现的输入契约、实现标签、角色隔离、GRank 图规模、
DPF CTest 注册、正式 executable 和可复现实验记录问题均已整改。

本次整改没有修改 `VFSS-baseline/`，也没有改变已经冻结的
M1 score semantics 或 M2 Protocol I 实现。

M3 现在明确保留两个不同入口，不能混用名称或轮数口径。

## 2. 三轮正式实现

实现标签：

```text
agarwal_protocol_iii_modular_3round
输入契约：
padded_n 个 priority-key additive shares
在线路径：
GRank
  → DPF routing
  → secure combine
在线轮数：
3
该实现是 Protocol III 模块化三轮工程基线。它从已经生成的
priority-key shares 开始，不包含 raw-score 到 priority-key 的输入适配。
3. Raw-score 五轮扩展
实现标签：
moe_topk_protocol_iii_raw_score_modular_5round
输入契约：
logical_n 个 Q20.12 raw-score additive shares
在线路径：
carry
  → sign
  → GRank
  → DPF routing
  → secure combine
在线轮数：
2 轮输入适配 + 3 轮安全核心 = 5 轮
该实现是仓库提供的 raw-score extension，不标记为论文原生三轮入口。
4. 角色与进程隔离
Dealer、Party 0 和 Party 1 使用独立的 fork() + exec() 角色进程。
离线与在线边界为：
Dealer preprocessing
  → Dealer 成功退出
  → 控制器发送输入 shares
  → Party 0 / Party 1 开始在线协议
验证中确认每个 12-case 矩阵产生：
Dealer exec 次数：12
Party exec 次数：24
父控制器不保留会掩盖 Party 退出或 socket HUP 的协议端点副本。
测试专用 completion acknowledgement 位于真实协议执行和计量之外，
不计入在线轮数。
5. GRank 与 DPF 口径
GRank 的以下内容均按 logical_n 构建：
- node-mask shares；
- comparison edge materials；
  -在线传输向量；
- comparison-edge metrics。
priority-key 输入仍保持 padded_n 形状。
DPF routing 域仍保持：
2^rank_bits
没有将 logical-n GRank 优化与输入接口或 DPF 域重构混在同一阶段。
现有 DPF 本地及 Peer/Dealer transport conformance 共 44 组，
已经正式注册进 CTest。
6. MetricsRecord
两个正式 executable 都生成结构化 MetricsRecord。
当前已记录：
- Git revision；
- implementation label；
- n 和 K；
- input seed；
- correctness status；
- Dealer preprocessing time；
- online execution time；
- offline material bytes；
- Party 0/Party 1 的实际发送和接收字节；
- comparison-edge count；
- compiler；
- build type；
- operating system；
- CPU model；
- system memory；
  -本地 AF_UNIX socketpair 环境。
当前 runtime 尚未提供完整、可信的 online PRG 调用计数器，因此：
online_prg_calls_total = NOT_MEASURED
不使用估算值冒充测量结果。
时间边界为：
offline:
  Dealer launch 前
  → Dealer 完成生成、序列化、发送并成功退出

online:
  控制器发送第一份输入 share 前
  → 控制器收到两方完整 report
oracle 验证、测试控制 acknowledgement 和子进程清理不计入在线时间。
7. 最终验证环境
Revision:
bb0d0e84ce63b0560db822aa2fcc45b7d811571c

Operating system:
Ubuntu 24.04.4 LTS

Kernel:
Linux 7.0.0-31-generic x86_64 GNU/Linux

Compiler:
gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0

CMake:
3.28.3

Configured memory:
7.7 GiB

Swap:
3.8 GiB

Logical processors visible to Ubuntu:
4

Build type:
Debug

EMP OT:
OFF

Network:
local AF_UNIX socketpair on one Ubuntu VM
8. 验收矩阵
以下 11 个 M3 相关测试在全新构建目录运行：
1. moe_topk_dpf_conformance_test
2. moe_topk_m3_grank_test
3. moe_topk_m3_dpf_routing_test
4. moe_topk_masked_mul_adapter_test
5. moe_topk_m3_secure_combine_test
6. moe_topk_m3_protocol_iii_three_process_e2e_test
7. moe_topk_m3_raw_score_pipeline_test
8. moe_topk_m3_raw_score_three_process_e2e_test
9. moe_topk_m3_secure_executable_test
10. moe_topk_m3_raw_score_secure_executable_test
11. moe_topk_m3_metrics_record_test
结果：
100% tests passed, 0 tests failed out of 11
Total Test time (real) = 193.59 sec
9. 后续边界
M3 模块化三轮工程基线和 raw-score 五轮扩展至此冻结。
后续工作不得在未单独说明的情况下：
- 把五轮 raw-score 扩展称为论文原生三轮实现；
- 把 priority-key share 入口描述成 raw-score 入口；
- 修改 M1 已冻结 score semantics；
- 重写 M2 Protocol I；
- 将未测量的 PRG、带宽或 RTT 指标填成估算值；
- 修改 VFSS-baseline/。
下一阶段可以独立推进 CipherGPT 原生基线或后续 Protocol III 精确两轮压缩。
