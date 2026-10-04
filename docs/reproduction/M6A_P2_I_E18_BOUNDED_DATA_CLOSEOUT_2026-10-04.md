# M6A-P2-I-E18：Protocol I+AAV86 有界数据关闭与六方案对比口径

日期：2026-10-04。状态：**E17 独立接收的有界项目实现与数据包可供合入审查**；本记录不是六方案完成、全规模测量或作者精确复现。被测协议源码是 E15 `346a92326e81aa3ab573c162439968503e792354` 与 E16 `7515aac64e8c7779895017d7c735285cb44dc336`；E16 事后报告 `267cb6924f061d376dbce7986fc0093b52ec2512` 和 E17 接收 `01f3c04fdbe5bcc8de6722bd72cd63bf3bad0b31` 不充当被测源码。E18 只合并来源/审查文档，未改 secure、测试输入、材料布局或计时合同。

## 可比较的数据

| 范围 | 采用的证据 | 可作的结论 |
|---|---|---|
| n=128，K=2/8，r=2–5 | E17 `E17_E16_REVISION_N128` 桥接，使用 E16 `7515aac` 原二进制；LAN/WAN 各配置 1 预热+5 正式，120 次运行、100 次正式正确，20 组九指标的 540 个统计值复核通过 | 与 E16 n=256 按**同 revision**、同定义分栏观察；输入长度不同，不能把不同 n 当重复样本 |
| n=128，K=2/8，r=2–5 | E15 `346a923` 修正计时后的独立接收批次，151 项索引 | 独立保留，不与 E16/E17 拼接为同 revision 趋势 |
| n=256，K=2/8，r=2–5 | E16 `7515aac` 最终 `final_v2`，E17 异会话 `PASS_WITH_EXPLICIT_LIMITS`；LAN/WAN 的 AAV86 96 次及 EMP-ON 全对全 24 次，100 次正式正确，20 组九指标的 540 个统计值复核通过 | 两条 Protocol I 项目路线在该同主机模拟网络、输入计划、资源门与计量合同下的配对观察 |
| n≥1000，K=80，r=2–5 | checked 形状公式与实际 preflight；先命中 `D>256` | `PRECHECK_REJECTED`；九指标、keygen、运行时间与峰值均 `NOT_MEASURED` |

完整 min/median/max 不在此重抄：n=128 桥接看仓库外 `E17_receiver_7515aac/bridge_final_v2/unified_nine_metrics_n128_bridge.csv`；n=256 看 E16 原目录 `TEST_ONLY_E16_RAW/final_v2/unified_nine_metrics.csv`；E15 看其 `TEST_ONLY_E15_RAW/corrected/` 独立记录。索引与路径、批次排除项及核验见 [E17 接收](../reviews/M6A_P2_I_E17_INDEPENDENT_RECEIVER_2026-10-04.md)、[E16 报告](../reviews/M6A_P2_I_E16_D256_PAIRED_PERFORMANCE_2026-10-04.md)、[E15 重测](../reviews/M6A_P2_I_E15_PROTOCOL_I_UNIFIED_REMEASUREMENT_2026-10-03.md)。原始日志留在实验工作树或仓库外副本，不纳入 Git。

## 九指标与同口径条件

两路线接收相同的 signed Q20.12 score；按 score 降序、原下标升序稳定处理同分；输出原输入顺序的 XOR 秘密共享 Top-K bit-mask，恰 K 个位置为 1。TEST_ONLY 同 profile/K/repetition 使用相同的输入 seed 与输入摘要；T 看不到在线输入。AAV86 的公开 pivot/bucket/局部 rank/活跃图及两路线的公开帧长按条件性泄露合同记录。比较对象是全两两预发 Protocol I+AAV86 与 **EMP-ON** 全对全 Protocol I；后者不是论文作者 Protocol I 的精确复现。

LAN/WAN 是同主机 WSL2 network namespace 的真实 TCP 连接加 `netem` 校准，通信计 framed application bytes，TCP/IP 线缆字节 `NOT_MEASURED`。每组 1 次预热排除、5 次正式分别取 min/median/max；离线从启动角色前到 T 退出及双方 ready，在线从双方输入接收后的 secure raw-score 入口到原序 mask 输出，`online_time_ms=max(P0,P1)`，逐次 `total=offline+online` 再统计，遵守 [E15 计时合同](../decisions/M6A_P2_I_E15_PROTOCOL_I_TIMING_CONTRACT_2026-10-03.md)。九指标依次为：离线时间、ready 时两方留存材料有效载荷 bit、在线时间、总时间、在线总通信 bit、在线每方通信 bit、raw-score 到原序 mask 的因果轮、在线传统 DCF 长度倍增 PRG 调用、在线实际消费比较边。在线每方通信分方实测；表中的双方相等时也不把包长或预算替代材料。

材料量使用 E15 已逐字段诊断的固定布局公式，并经 E16 实际包长复核；这是**由验证过的布局推导**的留存有效载荷，不是逐 key 正式计时内重序列化测量。每方全池预留 `r·C(D,2)` 是离线槽数，实际比较边是在线 `Σe_A`，两者分列。在线 PRG 只数传统 DCF 长度倍增调用，不等于 AES 总次数；比较边不把未消费的预发槽算入。`D=max(2,next_pow2(n))`，`b=33+log2(D)`，每方包 `114+D(1844+8r)+r·C(D,2)(81+24b)` B，Dealer **准入预算** `8·包+64 MiB`；包长和预算都不是实际峰值或离线留存材料。n=256 的真实运行峰值另见 E16 原报告；n≥1000 没有峰值实测。

## 六方案状态

| 方案 | 本阶段可引用状态 |
|---|---|
| Protocol I | 既有 M2 路线与本阶段 EMP-ON 全对全**项目基线**各保留自身标签；本表只用后者作同定义性能对照 |
| Protocol III | M5 既有 G3 回归基线；未与本次 E16/E17 数据做同口径性能拼接 |
| Protocol I+AAV86 | 上述全两两预发、n=128/256 的有界数据包已获 E17 `PASS_WITH_EXPLICIT_LIMITS` |
| Protocol III+AAV86 | 独立设计门仍 NO-GO；secure runtime 与性能 `NOT_MEASURED` |
| Protocol I+BB90+DCF | 本阶段无实现或同口径性能，`NOT_MEASURED` |
| Protocol III+BB90+DCF | 本阶段无实现或同口径性能，`NOT_MEASURED` |

论文中的算法与轮数定义、本地参考/实测、全两两预发和原序 mask 等项目扩展、待验证的高 n/联合安全/异机网络必须分别引用。当前安全结论只在可信且非合谋离线 T、私有完整交付通道、独立密钥随机性、单份 DCF 隐私及所列 PRG/泄露假设下成立；多 key 联合模拟没有在此证明。`AUTHOR_EXACT=NOT_PROVEN`，M6A/V3 总验收仍未完成。
