# M6A-P2-I-E17 异会话独立接收记录

日期：2026-10-04。此记录先于 E17 的任何修正或复跑保存；起始待核清单保留为历史审计顺序，最终判定为 `PASS_WITH_EXPLICIT_LIMITS`（见第 1 节）。

## 起始封存

- 独立 worktree：`C:\Users\28641\.codex\worktrees\e4c9\MoE_Top-k_Reproduction_Evaluation`；起始 `HEAD=267cb6924f061d376dbce7986fc0093b52ec2512`，起始工作树干净。E17 分支为 `codex/m6a-p2-i-e17-independent-receiver`。该 checkout 与 E15/E16 原工作树分离。
- 审查对象：E16 被测源码 `7515aac64e8c7779895017d7c735285cb44dc336`，事后报告 `267cb6924f061d376dbce7986fc0093b52ec2512`；二者不能互代。
- E16 原始目录位于 E16 工作树的 `experiments/m6a_p2_i_allpairs/TEST_ONLY_E16_RAW/`。起始索引 `e16_raw_complete_index.csv` 的 SHA-256 为 `326BB0A2F490A79EE709A09726B59808F4BEBC754BCAB7E8E75FFD7645F3C0EE`。
- E15 原始目录位于 E15 工作树的 `experiments/m6a_p2_i_allpairs/TEST_ONLY_E15_RAW/corrected/`。起始索引 `e15_raw_complete_index.sha256` 的 SHA-256 为 `E997A85D974EA498CE0999045451C5A29D3521440CB46FEFEAE9047AFAFB70D4`。
- `VFSS-baseline/` 起始无工作树差异；E15、E16 工作树与各自原始证据仅供只读核对。后续 E17 记录与工具仅写入本独立 worktree。

## 待独立核验

起始时保留此清单作为审计顺序；下文记录已完成的核验和限制。

## 1. 接收结论与证据身份

`E16_INDEPENDENT_RECEIVER = PASS_WITH_EXPLICIT_LIMITS`。接收对象是**全两两预发方案的有界项目实现及性能数据包**：E16 被测源码 `7515aac64e8c7779895017d7c735285cb44dc336` 的 n=256、K=2/8、r=2–5 Protocol I+AAV86，与同 revision EMP-ON 全对全 Protocol I，按 LAN/WAN 同主机模拟 TCP 网络分别完成 1 预热+5 正式。E16 的 `267cb6924f061d376dbce7986fc0093b52ec2512` 只是事后报告提交；两提交间 VFSS 源码、被测二进制和原始批次不混用。E17 未修改被测协议源码、计时合同或 E15/E16 原始证据。

本接收另在独立原始目录完成同一 E16 **原二进制**的 n=128 桥接，标签 `E17_E16_REVISION_N128`。E15 的 n=128 接收仍独立有效；桥接提供可在同 revision 分栏查看 n=128 与 n=256 的依据，不把 E15 的不同源码拼入跨规模曲线。n≥1000 只有真实首门拒绝与解析容量，没有 keygen、在线、耗时、峰值实测。

E17 是对尚未合并的 E16 证据的接收依赖，因此独立分支从 E16 报告提交建立；本分支只含接收文档和 TEST_ONLY 工具，不把审计改动带回主工作区或 E16 分支。

证据分层：AAV86 原论文与 Agarwal 会议版的算法/成本定义属于**论文定义**；E15 接收和 E16 原始运行属于**本地参考行为**；可信非合谋离线 T、全两两密钥预发、signed Q20.12 稳定键、原序 XOR Top-K mask 与本次 D256 准入属于**项目扩展**；n≥1000 可运行性、跨机器网络、底层联合安全证明和 Protocol III+AAV86 属于**待验证**。`AUTHOR_EXACT=NOT_PROVEN`，不宣称 M6A/V3 总验收。

## 2. 独立容量、材料与首门

不调用 E16 自带审计函数，E17 脚本逐行复算 32 个 `(n,K,r)`：`D=max(2,next_pow2(n))`、`b=33+log2(D)`、每方预留槽 `r·D(D−1)/2`、每方序列化包 `114+D(1844+8r)+r·D(D−1)/2·(81+24b)` B、Dealer **准入预算** `8·包+64 MiB`。预算是保守接入规则，不是 RSS。n=128 八行在 E16 原矩阵为 `NOT_RUN_E16_NO_BRIDGE`，现有独立 E17 桥接；n=256 八行 `MEASURED`；n≥1000 十六行 `PRECHECK_REJECTED`。两种 K 在同 n/r 有相同容量，完整 32 行复算对象为 E16 `final_v2/configuration_status_32.csv`。

| n | D | r | 每方预留槽 | 每方包 B | Dealer 预算 B | 实际 E16 首门 |
|---:|---:|---:|---:|---:|---:|---|
| 256 | 256 | 2 | 65,280 | 69,999,474 | 627,104,656 | 接受，3 GiB `RLIMIT_AS` |
| 256 | 256 | 3 | 97,920 | 104,763,122 | 905,213,840 | 接受，3 GiB |
| 256 | 256 | 4 | 130,560 | 139,526,770 | 1,183,323,024 | 接受，3 GiB |
| 256 | 256 | 5 | 163,200 | 174,290,418 | 1,461,432,208 | 接受，3 GiB |
| 1,000 | 1,024 | 2 | 1,047,552 | 1,167,830,130 | 9,409,749,904 | `D>256` |
| 10,000 | 16,384 | 2 | 268,419,072 | 324,549,132,402 | 2,596,460,168,080 | `D>256` |
| 100,000 | 131,072 | 2 | 17,179,738,112 | 22,007,488,315,506 | 176,059,973,632,912 | `D>256` |
| 1,000,000 | 1,048,576 | 2 | 1,099,510,579,200 | 1,487,639,764,009,074 | 11,901,118,179,181,456 | `D>256` |

n≥1000 的 r=3–5 亦逐行复算；最小被拒的 n1000/r2 同时超过 192 MiB 单方包与 2 GiB Dealer 预算，但源码真实检查次序先命中 `D>256`。旧 E15 n≥256 先命中 `D>128`。E16 的原始 `gate/e16_final_preflight_3g.log` 与 E17 自建 Release/EMP-ON 目标的 `e17_independent_preflight_3g.log` 均实际记录 n256/r2–5 接受、n≥1000/r2–5 以 `HARD_CAP_D_GT_256` 拒绝；这些拒绝发生在 keygen 前。独立 768 MiB 预检另确认 n256 因进程限额拒绝，n128 桥接在 D128 的 640–768 MiB 允许区间执行。`PRECHECK_REJECTED` 不等于资源运行失败，九指标均为 `NOT_MEASURED`。

固定留存材料公式另以真实包核对：DCF key 的有效载荷为 `24b+24` B（33 B 序列化头不计入有效载荷）；score key 840 B。n256/r2 每方 T 留存有效载荷为 66,256,896 B，两方为 1,060,110,336 bit；真实单方传输包为 69,999,474 B。全对全 n256 每方 T 留存 33,341,440 B，本地 shuffle 留存 600,064 B，两方总材料 543,064,064 bit。两路线的材料口径均为在线 ready 时留存的有效载荷，未把包头、传输包或 Dealer 预算代入材料指标。E16 `final_v2` 的 96 条 AAV86 行逐次真实包长与 r 对应公式一致。

每方预留 `r·C(D,2)` 是离线槽数，`e_A` 是在线实际比较边，不能互称。E17 对 120 条原始行逐轮核对 `Σe_A`、`Σv_A`、`e_A≤C(v_A,2)` 及 CA DCF 长度倍增 PRG `2·e_A·b`；例如 n256/r2 预热原行 `e_A=[3720,2831]`、`v_A=[256,240]`、每方 CA PRG `[305040,232142]`，`b=41`。在线总 PRG 指传统 DCF `traverseOneDCF` 的长度倍增调用，含 score/CA 阶段两方总和；其他 AES/PRG 细分仍为 `NOT_MEASURED`。原始 JSONL 不保存每条活跃边全文，故 E17 从 JSONL 独立核对的是逐轮向量与摘要，具体边合法性另由冻结 differential/E2E 测试及其原始日志覆盖。

## 3. 原始数据、九指标与网络

独立只读工具 `e17_independent_receiver.py` 逐项 SHA-256 核对 E16 376/376 个索引文件，且没有未索引原始文件；读取 120 条 JSONL 及对应 120 份日志，核对日志中的 n/K/D、时钟和分方字节，100/100 条正式行为成功退出及冻结 oracle/恰 K 标记，20 条预热排除。24 组 `(网络,K,重复)` 的 AAV86 四个 r 与基线输入 seed、输入摘要一致。逐次验证 `total=offline+online`、在线时间等于两方 secure 时钟较大值、双向 sent/received、各阶段 sent/received 之和、在线总/分方 bit、PRG 定义、round 与槽/边；再从五条正式行重算 20 组×九指标×median/min/max 共 **540 项**，全部匹配 `final_v2/unified_nine_metrics.csv`。分项中位数不相加伪造 total 中位数。日志及 source SHA 对应 `7515aac`；AAV86/基线二进制 SHA 分别为 `A1648F24DA0C6DDD88A3A6B561700A2907ECE57125068D25A9F210E6E013AF79`、`A07737585DC14ABABCDD8DF915FBC69FC11C26F61C75C89D52B5B4399B8C3A69`。

九指标为离线时间、离线留存材料、在线时间、总时间、在线总通信、在线每方通信、因果轮、在线 DCF 长度倍增 PRG、在线实际比较边。E16 n256 每组五次的完整 min/median/max 留在上述 CSV；20 组全部接收，没有用 0 填未测指标。n256 AAV86 r2 的中位在线总通信 40.75 KiB，基线 408.59 KiB；LAN AAV86 r2/K2 总时间 879.08 ms，基线 576.85 ms；WAN 对应 1,188.97 ms 与 1,248.15 ms。仅为本机样本观察，不能推成显著性或异机结论。

E16 校准逐批记录同主机 WSL2 namespace loopback 的 `netem`：LAN RTT 中位 1.119–1.201 ms，WAN 50.393–50.543 ms；吞吐原件和 qdisc before/after 均索引保存。计量是 framed application bytes，TCP/IP 线上开销 `NOT_MEASURED`。E17 另在临时 network namespace 真实建立 TCP 连接、双向发送并校验字节，`e17_tcp_probe.json` 为 `TCP_CONNECT_SEND_RECEIVE_PASS`。E17 桥接自身校准 LAN 1.147 ms/864.67 Mbit/s、WAN 50.502 ms/63.12 Mbit/s；仍属同主机模拟网络。

E15 原目录/副本 151/151、E16 原目录/副本 376/376 经独立 `e17_verify_preservation.py` 再验，两个索引 SHA 分别保持 `E997A85D974EA498CE0999045451C5A29D3521440CB46FEFEAE9047AFAFB70D4`、`326BB0A2F490A79EE709A09726B59808F4BEBC754BCAB7E8E75FFD7645F3C0EE`。E15、E16 工作树仍无 tracked 差异；`VFSS-baseline/` 未进入 E17 差异，主工作区亦未改动。

## 4. 失败留存与受控独立复跑

初版 `0cb011b` 的 n256/r5 在 2 GiB `RLIMIT_AS` 下已**通过预检**，conformance 两次 `std::bad_alloc`；复测最高 RSS 1,601,016 KiB。这是实际分配失败，不是公式包超限，也不代表在线协议失败。3 GiB 重测六例通过，最高 conformance RSS 1,771,288 KiB。E16 `gate/` 保留全部日志，初版 `final/` 的 r2–4 批次不与第二版 `final_v2/` 拼接。初版 r2 WAN 因共享 Git `packed-refs` 读取错误中断，三条尝试行及日志保存在 `final/r2/aav86/attempts/WAN_git_ref_read_error_1/`；最终统计只取第二版完整批次。第一版 TEST_ONLY D256 篡改夹具和 edge trace 上界修正也留在源码历史，修正后全档重跑。

在本独立 worktree 从 `7515aac` 内容重新配置 Release/EMP-ON；首次 CMake 缺少 emp-tool 前缀，按 E16 环境快照中的现有 `/tmp/moe_m28_emp.ok9WzQ/prefix` 重配并构建成功，没有安装依赖。受控资源预检当时可用内存约 7.56 GB、C 盘余量约 91.8 GB，满足 n256/r2 的 3×预算与 8 GiB 磁盘门；每目标 3 GiB `RLIMIT_AS`、120 s `timeout`。随后严格按顺序复跑 n256/r2：conformance 六例 PASS（最高 RSS 714,352 KiB）→冻结 oracle differential 六例 PASS（579,552 KiB）→独立 T/P0/P1 E2E 六例 PASS（348,932 KiB）。原始日志置于 E17 目录，未覆盖 E16 冻结五次统计。这是一个形状的独立复跑，不冒充 E16 全 120 次重复。

## 5. 同 revision n128 桥接

E16 原二进制已支持 n128，D128 准入要求 640–768 MiB。E17 在 **768 MiB** 下复用 E16 的 12 个输入种子计划，使用新的 T-visible serial 区段、逐次 fresh 材料和独立 `bridge_final_v2/` 原始目录。两路线在同一 profile/K/repetition 输入摘要完全配对；LAN/WAN 各 60 条，其中 10 组各 1 预热+5 正式。合计 120/120 次成功、100/100 正式正确、20 组九指标 min/median/max 的独立复核 540/540 PASS。桥接原始 JSONL 的 SHA 为 LAN `EA88A91FB841BC8A8C2ACE9D8B43A0F98FD2E1883E4785AF2DCCF8E4FEBF7C2A`、WAN `732E0A96DB220F94D33BAC88752C4465633888BBBB482C1769F43E2CEA5CA084`；完整九指标见 `bridge_final_v2/unified_nine_metrics_n128_bridge.csv`。

| E16 revision 上 n128 桥接 | K | LAN 总时间中位 ms | WAN 总时间中位 ms | 材料 MiB | 在线总通信 KiB | 因果轮 |
|---|---:|---:|---:|---:|---:|---:|
| AAV86 r2 | 2 | 188.08 | 596.38 | 30.94 | 20.75 | 8 |
| AAV86 r2 | 8 | 179.23 | 596.93 | 30.94 | 20.75 | 8 |
| AAV86 r3 | 2 | 252.13 | 746.37 | 46.20 | 24.94 | 10 |
| AAV86 r3 | 8 | 253.32 | 765.31 | 46.20 | 24.94 | 10 |
| AAV86 r4 | 2 | 336.04 | 990.16 | 61.46 | 29.12 | 12 |
| AAV86 r4 | 8 | 327.74 | 985.66 | 61.46 | 29.12 | 12 |
| AAV86 r5 | 2 | 412.09 | 1,173.46 | 76.71 | 33.31 | 14 |
| AAV86 r5 | 8 | 407.61 | 1,106.97 | 76.71 | 33.31 | 14 |
| 全对全 | 2 | 200.39 | 652.30 | 16.17 | 180.59 | 8 |
| 全对全 | 8 | 198.46 | 656.69 | 16.17 | 180.59 | 8 |

这批桥接无需改 E16 被测协议二进制或 E15 计时合同，可以与 E16 n256 **同 revision 分栏**研究规模变化；同网络、同输入生成规则下的两种 n 有不同输入长度，且每组仅五次，不作泛化速度断言。E17 启动时两次工具路径适配失败只留下独立校准文件：一次是 WSL 对 Windows `.git` worktree 指针解析，另一次是 Git blob LF 与 Windows checkout CRLF 比较；均在任何桥接 JSONL 前终止，校准原件保存在 `bridge_attempt1/` 与 `bridge_final/`，错误输出见本聊天工具记录，未进入 `bridge_final_v2/`。E17 最终证据目录索引 v2 覆盖 146 个文件，SHA-256 `3763F3AC2F8A3074E0511A9F5A960B53A8C472F9BDDAD69D123832CAC15D56C3`；先生成的 144 项索引原样保留并纳入 v2 索引。

## 6. 源码边界、安全与分项判定

`git diff 6a9ef84..7515aac` 的 secure 改动仅在 `VFSS/include/moe_topk/protocol_i_aav86_small.h` 与 `VFSS/src/moe_topk/protocol_i_aav86_small.cpp`：D 上限 128→256、单方包 64→192 MiB、Dealer 预算 512→2048 MiB、D256 精确 3 GiB 地址空间门和对应内存检查；没有改全池材料布局、在线消息/角色、稳定同分、oracle 或计时起止。TEST_ONLY 差异为 D256 夹具、按位篡改修正、实际 padded D 的 edge 上界、n256 benchmark 入口与资源/网络 runner；基线仍 EMP-ON。`267cb692` 事后提交只改报告与审计工具。secure 路径没有 TEST_ONLY 的明文 rank、选中下标或 mask 重构；测试重构仍在独立测试入口。

| 分项 | E17 判定 | 边界 |
|---|---|---|
| 功能 | **PASS** | E16 100/100 正式标记与日志，另独立 n256/r2 三层六例复跑；E17 n128 桥接 100/100 正式。 |
| 材料与资源门 | **PASS** | 32 行公式、D256 四档真实包及首门；2 GiB/r5 实际失败如实留存，3 GiB 修复重测；大规模仅准入拒绝。 |
| 九指标 | **PASS** | E16 n256 与 E17 n128 各 20 组×九项×三统计，均独立复算；AES 细分及大规模指标 `NOT_MEASURED`。 |
| 网络 | **PASS_WITH_SCOPE** | 真实 TCP 连接及模拟 LAN/WAN 校准通过；没有异机/线速字节结论。 |
| 数据完整性 | **PASS** | E15 151/151、E16 376/376 原件/副本；E17 v2 索引 146/146；作废与失败批次不混入。 |
| 安全证据边界 | **CONDITIONAL** | E8 异会话接收既定的单份 DCF 隐私、独立随机性、AES/PRG、可信非合谋离线 T、完整私有通道等假设不因性能接收而升级；公开 masked list、local rank、pivot/bucket/活跃图、帧长和 abort 时点仍属泄露口径。多 key 联合模拟及作者精确复现未证明。 |
| E16 异会话独立接收 | **PASS_WITH_EXPLICIT_LIMITS** | 本聊天与独立 worktree 完成只读审计、受控复跑和桥接；不代表 Protocol III+AAV86 或 M6A/V3 总验收。 |

## 7. Protocol I+AAV86 阶段关闭表

| 覆盖 | 结果 | 被测源码 / 实现标签 | 证据目录 |
|---|---|---|---|
| n=128、K=2/8、r=2–5 | E15 已独立接收；E17 同 revision 桥接另 PASS | E15 `346a92326e81aa3ab573c162439968503e792354` / `protocol_i_aav86_allpairs_e15_post_timing_accounting`；桥接 E16 `7515aac64e8c7779895017d7c735285cb44dc336` / `E17_E16_REVISION_N128` | `TEST_ONLY_E15_RAW/corrected/`；仓库外 `E17_receiver_7515aac/bridge_final_v2/` |
| n=256、K=2/8、r=2–5 | E16 九指标同规模接收 PASS | E16 `7515aac64e8c7779895017d7c735285cb44dc336` / `protocol_i_aav86_allpairs_e16_post_timing_accounting`；基线 `protocol_i_full_clique_emp_e16_post_timing_accounting` | `TEST_ONLY_E16_RAW/final_v2/`，E17 独立复跑 `E17_receiver_7515aac/` |
| n≥1000、K=80、r=2–5 | `PRECHECK_REJECTED`；性能/峰值 `NOT_MEASURED` | `7515aac64e8c7779895017d7c735285cb44dc336` / E16 有界准入 | `TEST_ONLY_E16_RAW/gate/e16_final_preflight_3g.log`；E17 `e17_independent_preflight_3g.log`；32 行容量 CSV |

因此关闭的是 Protocol I+AAV86 **全两两预发方案的有界项目实现及性能数据包**，不是全规模运行、论文作者精确复现、Protocol III+AAV86 或 M6A/V3 总验收。

## 8. E17 修改文件、验证命令与差异边界

本 E17 worktree 的全部修改文件如下；接收记录和九个 TEST_ONLY 工具为新建，两份既有计划只追加 E17 状态，`.gitattributes` 固定桥接 shell 的 LF 换行，没有活动协议源码改动：

1. `docs/reviews/M6A_P2_I_E17_INDEPENDENT_RECEIVER_2026-10-04.md`：先写起始封存，再补全本报告。
2. `e17_independent_receiver.py`：E16 原始文件、32 形状、首门、逐行守恒、配对及统计只读复算。
3. `e17_verify_preservation.py`：E15/E16 原目录与副本双份哈希核验。
4. `e17_bridge_n128.py`：E16 原二进制同 revision n128 桥接 runner，新鲜材料与 120 s 上限。
5. `e17_bridge_namespace.sh`：隔离 TCP `netem`、校准、768 MiB 地址空间门。
6. `e17_verify_bridge.py`：桥接日志、输入配对与九指标复核。
7. `e17_export_bridge.py`：单独桥接统一九指标 CSV 导出。
8. `e17_tcp_probe.py`：真实 network namespace TCP 双向诊断。
9. `e17_index_evidence.py`：一次性冻结 E17 目录索引，拒绝覆盖。
10. `e17_verify_index.py`：E17 证据索引只读核验。
11. `docs/IMPLEMENTATION_PLAN.md`：追加 E17 独立接收、桥接和阶段关闭边界。
12. `docs/BENCHMARK_VALIDATION_PLAN.md`：追加同 revision n128 桥接的原始口径与未测范围。
13. `.gitattributes`：固定 E17 network namespace shell 脚本的 LF 换行，确保 Windows checkout 后仍能在 WSL 运行。

执行过：两个只读原始审计工具、桥接独立核验、E17 146 项索引复验、独立 Release/EMP-ON 三目标构建及 n256/r2 conformance→oracle differential→独立进程 E2E、3 GiB/768 MiB 实际 preflight、TCP 双向探测、Python `py_compile`、Bash `bash -n`、`git diff --check`。提交前再检查最终工作树差异与冻结基线。构建物、原始日志、密钥、论文和本地参考工程不进入 Git；不推送、不合并。
