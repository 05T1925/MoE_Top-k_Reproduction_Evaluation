# M6A-P2-I-E9 异会话接收复核

日期：2026-10-03。接收者为本规划与复检聊天，未参与 E9 执行聊天的源码修改。审查对象是隔离 worktree `codex/m6a-p2-i-allpairs-experiment@c3926c68fd14f270faa8b55234311071947fa080` 中下列精确 E9 文件；本报告不代表另一个人的形式化密码学证明。

## 判定

| 门 | 结论 |
|---|---|
| E9 D=16/32/64 功能与容量试运行 | **PASS_WITH_FINDINGS**。每档 conformance、冻结 oracle differential、T/P0/P1 独立进程 E2E 各 6/6 通过；异会话新构建目录复跑结果一致，相关 CTest 25/25 通过。 |
| 生成前预检 | **PASS_WITH_SCOPE_LIMIT**。当前 `D≤64` 范围内，checked 容量、material ID、包字节和工程预算在 dealer 的全池 reserve/keygen/大块分配前检查。工程预算不是可移植的峰值内存证明。 |
| D=128 | **NOT_RUN / ANALYTIC_LIMIT**。公开 `preflight` 现先由 `domain(n)` 的 `n≤64` 硬上限拒绝；388.61 MiB 是套用同一公式的解析预算，不是该接口实际返回的 D128 拒绝原因。不得写成 D128 已按预算门实际试运行。 |
| 正式性能、Protocol III、作者精确复现 | **NOT_DONE**。本次为 Debug 单次工程试运行；PRG、逐轮参与节点 `v_A`、LAN/WAN 与正式重复均未完成。`AUTHOR_EXACT=NOT_PROVEN`。 |

此接收允许下一阶段针对 Protocol I+AAV86 完善计量、冻结实现及审慎评估 D128；不批准放开无界 D、变更为固定 M 或以本次数字宣布 M6A 性能验收。

## 源码身份和独立核验

六份文件的 SHA-256 与 E9 执行报告完全一致：

| 文件 | SHA-256 |
|---|---|
| `protocol_i_aav86_small.h` | `A7A4642BBA588A343A99D9B89FFA4EB79A5E1DC0FA99AFA5530128DEAB790785` |
| `protocol_i_aav86_small.cpp` | `DA2537FF9007B64C18CBB91374A5EEC1BD621549218E467A974F8449658DFAFC` |
| `protocol_i_aav86_small_conformance_test.cpp` | `AE0815470FE62F0B4E8C748AAE3F9DC979BD5F9520EDF75D5A4797B405934045` |
| `protocol_i_aav86_small_differential_test.cpp` | `855325D2FC53F79520FC0F6EC4C523ABF18D7E7323E8FD6C71A2CF144F862502` |
| `protocol_i_aav86_small_e2e_test.cpp` | `FB72FC5F69F08B31A8E47311A8F082FAEF857CF5F53DBA5FB6A146862B29E831` |
| `protocol_i_aav86_e9_fixtures.h` | `4C0CCF9EF57F91C7700A76D13084F98227A35EB54011A628A198552D0CB2EF87` |

逐行独立复算 E9 原始 CSV 的 18 个配置：`reserved=r·C(D,2)`；每方包 `B=114+D(1844+8r)+r·C(D,2)(81+24b)`、`b=33+log2(D)`；`active=sum(active_by_round)`；`ca_eval_per_party=2·active`；`total_eval=score_eval+ca_eval`；`rounds=2r+4`；双方发送与对端接收逐行对账。**18/18 均一致**。原 CSV 每档 6 行，D64 的最大 T 峰值为 57,612 KiB。原始随机图边数是单次观测，复跑改变随机币后无需与历史边数相等。

在新的 WSL `Ubuntu-24.04` Debug 构建目录 `/tmp/m6a9-receiver-20261003` 以 `-DMOE_TOPK_ENABLE_EMP_OT=OFF` 配置并构建三项目标。依次设置 `MOE_TOPK_M6A_E9_D=16,32,64`，逐档直接运行 conformance、differential、E2E，**9/9 可执行文件调用成功，各档各 6 个用例**；然后全量构建并执行 `ctest --test-dir /tmp/m6a9-receiver-20261003 --output-on-failure -j4 -R 'moe_topk_(m6a7|m2_|m3_|m5_fix_f1)'`，**25/25 PASS**。`git diff --check` 通过。没有生成 D128 key，也没有正式 benchmark。

源码复核确认 `protocol_i_aav86_small_dealer_generate` 的第一步为 `protocol_i_aav86_small_preflight`；其后才锁定 dealer、初始化随机币并构造全池。`preflight` 对本阶段容量做 checked arithmetic，仍使用 `D≤64` 的独立硬上限、64 MiB 单方包限制及 `8B+64 MiB≤256 MiB` 工程准入。后者是带余量的工程规则；`sysconf(_SC_AVPHYS_PAGES)` 的可用页信息不能单独证明容器或主机的实际内存余量，扩 D 前还需核对实际进程限制和失败退出。

## 发现及下一步

1. **F1 / 报告口径**：把 D128 记为 `HARD_CAP_AND_ANALYTIC_BUDGET_LIMIT` 更准确。当前代码拒绝 D128 的第一原因是 `n≤64`；预算 388.61 MiB 是离线公式评估，尚无 D128 预检 API 或运行时数据。这不影响 D16–64 的功能验收。
2. **F2 / 内存门**：`8B+64 MiB` 已在 D16–64 的本次环境下得到实际 RSS 佐证，但不构成通用上界。任何 D128 探测须先明确进程/cgroup 可用内存、可控资源上限、失败关闭和独立测试；不能仅调大 `n`/预算常量。D256/r5 的解析单方包约 174,290,418 字节，已经超过现有 64 MiB 包上限；更大正式矩阵须保留 `RESOURCE_LIMITED` 等准确状态。
3. **F3 / 完整计量**：E9 已记录活跃边、包字节、消息、DCF 和 T/P0/P1 峰值；正式 V3 还缺可信在线长度倍增 PRG 调用、各轮参与节点 `v_A`、Release 相同环境、1 次预热+5 次正式重复以及 LAN/WAN。未经测量的指标维持 `NOT_MEASURED`。
4. **F4 / 证据冻结**：HEAD 仍为旧基线而 E1–E9 源码/报告多为未跟踪，正式性能前须在隔离分支选择性提交可审代码与文档并由接收者复跑冻结 revision；不能将论文、密钥、原始日志或构建物混入提交。主工作区原有差异与 `siamjdiscrmath.pdf` 不动。

当前密码学结论依赖 E7/E8 所列单份 DCF 选阈值隐私、独立 keygen 随机币、AES/PRG、可信且不合谋的离线 T 以及完整私有通道。该条件性项目扩展与 Agarwal 作者精确复现分开标记。
