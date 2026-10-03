# M6A-P2-I-E15：Protocol I 两路线统一重测（2026-10-03）

## 结论与证据身份

本报告只使用修正后的 `TEST_ONLY_E15_RAW/corrected/` 批次。被测源码提交 `346a92326e81aa3ab573c162439968503e792354`，工作树四批运行期间 tracked 状态为 CLEAN；AAV86 与 EMP-ON 全对全二进制 SHA-256 分别为 `E0963613D767FEAE6197B0512979891EF0F20A4326DF6C397A979E6A31F38F3E`、`0468912949F3469C12C65FFFF77ECEE03298DBE58874A0124BAB3AEE13565ADB`。实现标签分别是 `protocol_i_aav86_allpairs_e15_post_timing_accounting`、`protocol_i_full_clique_emp_e15_post_timing_accounting`。原始索引 SHA-256 为 `E997A85D974EA498CE0999045451C5A29D3521440CB46FEFEAE9047AFAFB70D4`。

E14 接收结论见 `M6A_P2_I_E15_E14_PRE_FIX_INDEPENDENT_ACCEPTANCE_2026-10-03.md`：功能及非时间计数有条件成立，offline/total 时间被 ready 前整池诊断污染，原速度结论撤回。E15 第一候选 `fdcdbe5` 虽移除整池诊断，但仍在 ready 前计算形状计数，按严格计时合同作废；其 `TEST_ONLY_E15_RAW/final/` 与仓库外完整副本保留，不混入本报告。本次修正在双方 ready 前的正式路径完全不运行材料统计；AAV86 的常量形状计数及全对全的八个 OT 计数读取、常量形状计数均置于在线计时结束之后。诊断路径仍把真实序列化材料与公式逐项比对，且不进入正式统计。secure runtime、消息、输出和泄露合同未改动；源码只改 TEST_ONLY party 入口、共享指标头和 E15 runner，另同步合同与计划。

此结论是当前聊天的技术审计与重测报告，**不冒充另一独立会话对 E15 的最终接收**。AAV86 是 Agarwal Protocol I 比较图加项目原序 mask 扩展；EMP-ON 全对全是项目比较路线。`AUTHOR_EXACT=NOT_PROVEN`。两条路线在 signed Q20.12、稳定原下标 tie、原序 XOR Top-K bit-mask、可信离线且在线静默 T 的同一功能口径下比较。

## 计量合同、运行条件与验证

合同在 `docs/decisions/M6A_P2_I_E15_PROTOCOL_I_TIMING_CONTRACT_2026-10-03.md`。`offline_time_ms` 从控制器启动角色前至 T 退出且双方收到材料、做完本地预处理并 ready；`online_time_ms=max(P0,P1)`，从输入接收后 secure raw-score 入口至原序 mask 输出；每次 `total=offline+online` 后再按五次取 median/min/max。在线通信为两方应用层 sent 之和，per-party 为分方量；材料为 T 包有效载荷加双方本地 shuffle 留存，不把 OT 传输重复计入留存。轮数是本入口因果 DAG；PRG 是 DCF 长度倍增计数，不等于 Eval/AES 次数；比较边是在线实际消费的无序边。原始行保留阶段、分方、OT、active vertices、reserved slots、峰值及命令。

环境为 Windows 11 / Ubuntu 24.04 WSL2，内核 `6.6.87.2-microsoft-standard-WSL2`，i9-13980HX（16 核/32 逻辑线程），WSL 可见 RAM 8,122,396,672 B、swap 2,147,483,648 B，GCC 13.3.0，CMake Release、`MOE_TOPK_ENABLE_EMP_OT=ON`，EMP prefix `/tmp/moe_m28_emp.ok9WzQ/prefix`。每角色单线程，`RLIMIT_AS=805306368 B`。同主机独立 network namespace 中 TCP loopback 的 netem LAN 为 0.5 ms/1000 mbit、WAN 为 25 ms/100 mbit；实际校准 JSON、qdisc before/after 均在原始目录。LAN RTT 中位约 1.19–1.21 ms；WAN 约 50.44–50.74 ms。这是同主机模拟网络，不是物理双机结果。

输入计划 SHA-256 `11430F861182471BCE27B25452C42B7E8444C9AB9B0CC71F45891CE6B53925D9`，由 OS 随机源产生 12 个不同 64-bit seed。相同 K/rep 的输入 seed 与 digest 在四个 AAV86 r 和全对全基线中配对，T 可见 serial 分离，逐次重生 T 材料、份额与一次性身份。n=128，K=2/8，AAV86 r=2..5，LAN/WAN 各配置 1 预热+5 正式：AAV86 48+48 行、基线 12+12 行，共 120 行、100 正式、20 个五次组和 24 个配对组。预热原始行保留但不进统计。

修正后的 Release 二进制顺序通过 AAV86 conformance、冻结 oracle differential、独立进程 E2E、材料指标 conformance、全对全 E2E；TCP 同材料等价、关闭/静默/截断有界失败、旧 F1 raw-mask/process、旧模块化 I 回归亦通过。另以非正式模式对 n=2、D=16、n=128 的实际生成材料执行形状对序列化诊断；n=128 AAV86 r=5 与全对全入口均成功，诊断没有加入 120 行。

独立审计脚本重新读取四份 JSONL，逐行核对 revision、源码与二进制哈希、status/exit、oracle 原序恰 K 位、输入配对、分阶段与分方 sent/received 守恒、在线 PRG 与边、offline+online 恒等式、五次统计摘要；结果 `PASS`。运行日志、审计输出及统一 CSV 可在 `TEST_ONLY_E15_RAW/corrected/` 查询。`sha256sum -c e15_raw_complete_index.sha256` 为 151/151；152 文件仓库外副本 `C:/Users/28641/Desktop/MoE_Top-k_Reproduction_Evaluation_Evidence/M6A/E15_corrected_346a923/` 与原始目录 `diff -qr` 完全相同。

## 九指标：五次正式样本中位数

下表时间单位 ms，材料/通信单位 bit；`comm/P` 是每方在线发送量。CSV 保存全部九指标的每组 median/min/max，原始 JSONL 保存每次值。AAV86 的 reserved slots 是每方 `r·C(128,2)`，表中 edges 为在线实际边，两者不能混用。

| 网络 | 路线 | K | r | offline | material | online | total | comm | comm/P | rounds | PRG | edges |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| LAN | AAV86 | 2 | 2 | 182.9 | 259,571,712 | 21.5 | 203.9 | 169,984 | 84,992 | 8 | 432,896 | 2,488 |
| LAN | AAV86 | 2 | 3 | 254.0 | 387,555,328 | 20.1 | 272.8 | 204,288 | 102,144 | 10 | 287,776 | 1,581 |
| LAN | AAV86 | 2 | 4 | 347.9 | 515,538,944 | 48.2 | 396.0 | 238,592 | 119,296 | 12 | 236,896 | 1,263 |
| LAN | AAV86 | 2 | 5 | 445.9 | 643,522,560 | 23.3 | 469.1 | 272,896 | 136,448 | 14 | 228,416 | 1,210 |
| LAN | AAV86 | 8 | 2 | 170.2 | 259,571,712 | 21.2 | 193.0 | 169,984 | 84,992 | 8 | 410,496 | 2,348 |
| LAN | AAV86 | 8 | 3 | 239.1 | 387,555,328 | 20.7 | 261.2 | 204,288 | 102,144 | 10 | 278,976 | 1,526 |
| LAN | AAV86 | 8 | 4 | 353.7 | 515,538,944 | 21.8 | 375.6 | 238,592 | 119,296 | 12 | 223,776 | 1,181 |
| LAN | AAV86 | 8 | 5 | 428.8 | 643,522,560 | 23.0 | 461.0 | 272,896 | 136,448 | 14 | 210,336 | 1,097 |
| LAN | full clique EMP-ON | 2 | — | 150.6 | 135,667,712 | 42.2 | 192.8 | 1,479,424 | 739,712 | 8 | 1,335,296 | 8,128 |
| LAN | full clique EMP-ON | 8 | — | 155.7 | 135,667,712 | 41.7 | 198.1 | 1,479,424 | 739,712 | 8 | 1,335,296 | 8,128 |
| WAN | AAV86 | 2 | 2 | 177.7 | 259,571,712 | 430.5 | 612.1 | 169,984 | 84,992 | 8 | 385,056 | 2,189 |
| WAN | AAV86 | 2 | 3 | 256.0 | 387,555,328 | 524.0 | 779.5 | 204,288 | 102,144 | 10 | 276,576 | 1,511 |
| WAN | AAV86 | 2 | 4 | 348.2 | 515,538,944 | 625.3 | 972.7 | 238,592 | 119,296 | 12 | 216,736 | 1,137 |
| WAN | AAV86 | 2 | 5 | 422.8 | 643,522,560 | 724.3 | 1,147.5 | 272,896 | 136,448 | 14 | 225,536 | 1,192 |
| WAN | AAV86 | 8 | 2 | 182.5 | 259,571,712 | 424.0 | 607.3 | 169,984 | 84,992 | 8 | 427,456 | 2,454 |
| WAN | AAV86 | 8 | 3 | 251.5 | 387,555,328 | 525.3 | 776.4 | 204,288 | 102,144 | 10 | 287,456 | 1,579 |
| WAN | AAV86 | 8 | 4 | 335.3 | 515,538,944 | 627.0 | 964.4 | 238,592 | 119,296 | 12 | 253,376 | 1,366 |
| WAN | AAV86 | 8 | 5 | 413.4 | 643,522,560 | 725.5 | 1,142.6 | 272,896 | 136,448 | 14 | 201,696 | 1,043 |
| WAN | full clique EMP-ON | 2 | — | 163.0 | 135,667,712 | 503.2 | 666.1 | 1,479,424 | 739,712 | 8 | 1,335,296 | 8,128 |
| WAN | full clique EMP-ON | 8 | — | 159.4 | 135,667,712 | 500.6 | 660.6 | 1,479,424 | 739,712 | 8 | 1,335,296 | 8,128 |

两路线在全部配置通过同一原序 mask oracle。AAV86 r=2 的在线通信是全对全的约 11.5%，实际边在这些样本为 2,189–2,488 对，低于 8,128；代价是预留离线材料约 1.91 倍。r 从 2 增至 5 时本实现保留材料线性增、完整入口因果轮由 8 到 14，虽然实际边通常下降，总时间仍上升。LAN 的总时间中位数比较：K=2,r=2 的 AAV86 203.9 对全对全 192.8 ms，K=8,r=2 为 193.0 对 198.1 ms；WAN 的 r=2 分别为 612.1 对 666.1、607.3 对 660.6 ms。r=3..5 在两档网络均比本次全对全基线慢。LAN K=2,r=4 的在线中位 48.2 ms 相对邻组偏高；五次范围和逐次日志保留，未删除离群值或事后修正。以上仅是这台主机、这些样本及该计时边界下的观察，不给统计总体或跨机器速度保证，也不从 E14 作废时间估计“修复提升”。

## 可复现路径、容量与边界

源码工具均在 `experiments/m6a_p2_i_allpairs/TEST_ONLY/`：`e15_input_plan.py` 生成计划；`e15_shaped_namespace.sh LAN|WAN <aav-binary> <output/aav86> <source-root> <input-plan>` 与 `e15_clique_shaped_namespace.sh LAN|WAN <baseline-binary> <output/baseline> <source-root> <matching-aav-jsonl> <input-plan>` 以 root 设置隔离网络后由 `moeaudit` 身份运行受 768 MiB 限制的 runner；`e15_audit_results.py --raw-root ... --head 346a923... --source-root ... --aav-binary ... --baseline-binary ...` 验证；`e15_export_unified.py` 导出统一 CSV。原始行存完整实际命令。全路径、配置、校准与构建信息见 `corrected/e15_environment.txt`。

`moe_topk_m6a10_aav86_metrics_conformance_test` 在同一 768 MiB 限制下输出 `corrected/e15_capacity_preflight.log`；`e11_capacity_status.py` 生成 32 行 V3 容量状态表。n=128 八个形状有正式计量；n≥256 的 24 个形状为 `HARD_CAP_D_GT_128` / `PRECHECK_REJECTED_NO_KEYGEN`，其时间、峰值、在线量一律 `NOT_MEASURED`。n=256,r=2 解析包已为 69,999,474 B，超过 64 MiB 包上限；不借旧结果或公式充作实测。Protocol III+AAV86 本阶段只有独立设计门文档，secure runtime 未改、性能 `NOT_MEASURED`。
