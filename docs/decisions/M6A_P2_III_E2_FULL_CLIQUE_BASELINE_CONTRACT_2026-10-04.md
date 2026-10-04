# M6A-P2-III-E2：正式全对全 Protocol III 九指标基线准备

日期：2026-10-04。对象是现有 `protocol_iii_raw_score_mask_party` 的 **signed Q20.12 原始份额→原序 XOR Top-K mask 四轮完整入口**，不使用 M5 两轮 odd-prime field core，也不使用 Protocol I 数字。当前为 TEST_ONLY 容量/功能/计数预检，尚无 E17 口径的 LAN/WAN 正式批次。III+AAV86 的构造门单独记录于 [E2 设计门](M6A_P2_III_E2_HIDDEN_HANDLE_RING_DPF_CONSTRUCTION_GATE_2026-10-04.md)，它的 NO-GO 不阻止本基线准备。

## 1. 身份、输入和网络

正式批次只取同一个冻结 revision/二进制/实现标签，逐行记录源码及二进制 SHA-256、命令、主机/WSL/编译器/CMake/EMP 设置、CPU/内存限制、TCP qdisc、校准和时间戳。输入形状 `n=128/256`、`K=2/8`，与 E17 使用相同的 12 个 `(K,rep)` 输入 seed 计划语义：每个 K 有 1 次预热加 5 次正式，LAN/WAN 与所有对照路线对同一 `(n,K,rep)` 使用相同原始分数/摘要；不同 n 的向量长度不同。seed 由 OS 随机生成并独立保存，不能从 T 可见 session、serial、fingerprint 或 material ID 推回；两方加法份额用另一个独立 seed/OS 熵。每次重新运行可信不合谋 T，待其材料一次性分发完成并退出后才释放输入。测试控制器独占明文 oracle，secure 双方不见原始分数或明文 mask。

在线四条 carry/sign/GRank/DPF 帧通道必须是真 TCP；正式 LAN/WAN 用 E17 的同主机 network namespace + `netem` 语义，逐批保存双向校准、qdisc 前后和 TCP 探针。离线/控制 IPC 可独立于在线 TCP，但须在日志中分栏。当前 E2 harness 仅用 `127.0.0.1` TCP loopback，未加 E17 的 RTT/吞吐 profile；它的输出只证实连接、容量、功能及计数，不能填正式九指标或与 E17 I 数字比较。正式每 `(n,K,profile)` 跑 1 预热+5 正式，预热排除统计；每一组从五条正式原始行独立重算九指标 min/median/max。失败、作废批次及重跑分开索引，不能拼接。

## 2. E17 同口径九指标采集点

| 指标 | E2/后续正式采集合同 | 当前 E2 状态 |
| --- | --- | --- |
| `offline_time_ms` | 控制器启动 T/P0/P1 前起，到 T 成功退出、两方收齐材料、完成本地协议预处理且均发送 ready 止；TCP 建连/校准单列。ready 前不得全池 `serialize()` 诊断或遍历 key 计材料。 | `NOT_MEASURED`；当前进程 harness 没有独立 ready/计时帧。 |
| `offline_material_total_bits` | ready 时双方在线实际留存并读取的有效载荷总 bit；score/GRank/DPF share/key 分栏，去除包头/IPC 长度；用冻结布局的 checked 形状公式计算，并在独立诊断测试与真实包逐字段核对。 | `NOT_MEASURED`；本次给出布局推导值和真实包长度，尚未冻结正式 material 计数器。 |
| `online_time_ms` | P0/P1 各自只围住完整 secure `protocol_iii_raw_score_mask_party` 调用，取两方较大者；输入生成、oracle、报告编码与诊断不在内。 | `NOT_MEASURED`。 |
| `total_time_ms` | 每条正式原始行先算该行 offline+online，再对五条 total 独立取统计，不能把分项 median 相加。 | `NOT_MEASURED`。 |
| `online_total_communication` | 两方 framed application `sent_bytes` 相加，并交叉核对 A.sent=B.received、B.sent=A.received；保留 carry/sign/GRank/DPF 分阶段原始计数。TCP/IP 线速字节另列。 | TCP 冒烟计数已观察；正式 LAN/WAN `NOT_MEASURED`。 |
| `online_per_party_communication` | 各方 sent、received、logical bit 及四阶段分项，逐行保存，方向守恒。 | 冒烟计数已观察；正式 `NOT_MEASURED`。 |
| `causal_rounds` | 真实 secure 帧 DAG：score carry→sign→原槽 GRank→ring DPF；应为 4，逐行审计实际帧，不从 `2r` 候选借用。 | 四轮 TCP 冒烟 PASS；正式批次 `NOT_MEASURED`。 |
| `online_dcf_prg` | 在线前 `resetDCFOnlinePrgCalls()`，secure 返回后 `readDCFOnlinePrgCalls()`，两方总和；含 score 和 GRank 的传统 DCF 长度倍增，不含 native DPF Eval/AES，后二者单列。 | TEST_ONLY 进程总计数已覆盖并与形状核对；正式 `NOT_MEASURED`。 |
| `online_actual_comparison_edges` | 只数 `GRank` 无序真实槽 pair，为 `C(n,2)`；score 的 per-slot uCMP 次数与 DPF Eval 次数单列，不混成边。 | n128=8,128、n256=32,640，形状及入口调用核对；正式 `NOT_MEASURED`。 |

正式原始行另记 `raw_dcf_calls`、`dpf_eval_calls`、DCF PRG score/GRank 分项、网络 RTT/吞吐、P0/P1 RSS 峰值、T RSS 峰值和失败/abort；未做的细分写 `NOT_MEASURED`。PRG 是 `VFSS/ext/FSS/dcf.cpp` 当前线程计数器的实际调用次数，不能用 `raw_dcf_calls × bits` 公式伪装为实测。当前 E2 TEST_ONLY 在独立进程 secure 调用前后实际读取计数器，并以公式作守恒断言；正式 runner 仍需保留真实原始计数和源码身份。

## 3. 容量和已运行 TCP 冒烟

`protocol_iii_e2_baseline_capacity.py` 从现有序列化布局推导每方包、各阶段字节及材料有效载荷，先用 M5 G3 n2/5/8 的真实包记录检验公式。正式 F1 package 上限 64 MiB **每方**；n128/K2 或 K8 每方 8,711,978 B，n256 每方 35,267,050 B，均低于上限。n128 两方有效材料推导为 131,805,184 bit，n256 为 534,159,360 bit；这不是 RSS，也不是已经签收的九指标材料实测。当前小型进程 harness 的 package bytes 包含双方各 8 B IPC 长度头，故观察总量分别为 17,423,972 B 和 70,534,116 B。未测得固定 RSS 峰值或资源门；“包不过限”不代表 n256 的正式五次统计已经可运行。

| E2 TEST_ONLY TCP loopback | n128/K2 | n256/K2 | n128/K8 | n256/K8 |
| --- | ---: | ---: | ---: | ---: |
| 三进程 oracle/恰 K | PASS | PASS | PASS | PASS |
| 四轮真实在线 TCP | PASS | PASS | PASS | PASS |
| 双方包含 IPC 长度头 B | 17,423,972 | 70,534,116 | 17,423,972 | 70,534,116 |
| 双方在线 framed sent B | 12,672 | 24,960 | 12,672 | 24,960 |
| 双方实际 DCF PRG | 1,335,296 | 5,422,592 | 1,335,296 | 5,422,592 |
| LAN/WAN 九指标 | `NOT_MEASURED` | `NOT_MEASURED` | `NOT_MEASURED` | `NOT_MEASURED` |

本次容量式为 `party_bundle=106+1796D+16n+C(n,2)(24b+81)+n·dpf_key_bytes`，`b=33+log₂D`，`dpf_key_bytes=16(rank_bits+1)+2·point_bytes+16`；严格只适用于当前 native 64-bit ring DPF 和 F1 包版本。在线 PRG 期望核对式是**每方** `4D·34 + 2C(n,2)·b`，n128 为 667,648、n256 为 2,711,296；测试比较的是 runtime counter 而非用此式填指标。新版本改变布局、DPF 序列化或位宽时，先重做独立真实包校验。

## 4. 后续执行门

冻结 source/计量合同后，先做 conformance→冻结 oracle differential→独立 T/P0/P1 E2E，再做 network namespace TCP LAN/WAN 1+5 正式批次；任何失败留下原始证据并修复后新标签全档重跑。正式 runner 需补：输入 seed 计划及摘要、ready barrier、两方 secure 单独时钟、T/party 资源观测、材料 payload 诊断独立校验、四阶段 PRG 分项、网络校准和不可覆盖的原始 JSONL/日志/哈希索引。此前不会从 M5 field-core 通信核验、I+AAV86 E17 数字或当前 loopback 冒烟推算九指标。
