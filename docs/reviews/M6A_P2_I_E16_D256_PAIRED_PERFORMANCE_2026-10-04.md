# M6A-P2-I-E16：D256 全链路容量门与 Protocol I 同口径性能报告

日期：2026-10-04。状态：**E16 本地执行及原始数据独立脚本审计 PASS；异会话接收待完成**。被测源码 `7515aac64e8c7779895017d7c735285cb44dc336`，隔离分支 `codex/m6a-p2-i-e16`；最终报告提交在该被测提交之后，不改写被测源码或二进制。起点是已独立接收的 E15 `6a9ef8447cfd6b74c17f9a5645eedebcb8a6ea72`。本报告仅对本机 Release/EMP-ON、同主机 WSL2 TCP 模拟网络和所列输入分布负责，不把项目扩展反写成论文结论，`AUTHOR_EXACT=NOT_PROVEN`。

## 1. 冻结来源、证据分层与边界

- **论文定义**：本次未增加论文一致性声明。AAV86 的条件性设计及可信 T、公开图形状、泄露假设仍依 E7/E8 决策；Protocol III+AAV86 的设计门仍为 NO-GO。
- **本地参考行为**：E15 的计时合同、固定材料布局诊断、n=128 重测与异会话接收构成本地比较口径，不是 n=256 的性能样本。E15 被测源码 `346a923`、原始目录 `TEST_ONLY_E15_RAW/corrected/`、151/151 索引 SHA-256 `E997A85D974EA498CE0999045451C5A29D3521440CB46FEFEAE9047AFAFB70D4`、仓库外 `E15_corrected_346a923/` 未修改；E15 工作树仍在 `6a9ef84` 且干净。
- **项目扩展**：E16 只把现有全两两 Protocol I+AAV86 入口在有界资源门下扩至 D256，并适配 TEST_ONLY 夹具和基线 n256 入口。secure 路径不重构 rank、比较位、selected index 或明文校验数据；T 仍只预发完整材料且不接触在线输入。两路线输出均为原输入顺序 XOR Top-K mask，使用 signed Q20.12 与稳定同分 oracle。
- **待验证设想**：n≥1000 的可运行性、跨机器网络行为、跨 n 的同 revision 趋势及 Protocol III+AAV86 均未测。不得由 n=128 或本次 n=256 外推。

E16 第一版 `0cb011b` 在 2 GiB `RLIMIT_AS` 下已完成 r=2–4，但 r=5 conformance 连续两次以 `std::bad_alloc` 失败，复测最高 RSS 1,601,016 KiB。失败及第一版正式批次保存在 `TEST_ONLY_E16_RAW/gate/` 与 `TEST_ONLY_E16_RAW/final/`，没有并入最终统计。第二版只将 D256 每进程地址空间硬限额升为**精确 3 GiB**；r=5 conformance 六例通过，最高 RSS 1,771,288 KiB。源码/runner 重新冻结后，全部 r=2–5 和基线在 `TEST_ONLY_E16_RAW/final_v2/` 全量重测。E15 与两版 E16 也不拼接。

## 2. 32 配置资源门

公式推导：`D=max(2,next_pow2(n))`、比较宽度 `b=33+log2(D)`、每方全池槽 `r·C(D,2)`、单方包 `114+D(1844+8r)+r·C(D,2)(81+24b)` B、Dealer 准入预算 `8·package+64 MiB`。这是已由 E15 固定布局诊断核对过的形状公式，**不是**实际 RSS 或运行时间。E16 未改布局；D256 的 conformance、独立 E2E 与逐次真实包长再次核对公式。旧预检真实首门依次为 `D>128`、64 MiB 包、512 MiB Dealer 预算、可用内存、D128 `RLIMIT_AS`、材料 ID；新预检真实首门为 `D>256`、192 MiB 包、2 GiB Dealer 预算、可用内存、D256 精确 3 GiB `RLIMIT_AS`、材料 ID。`e16_prechange_resource_gate.csv` 将全部 32 行公式与旧二进制 24 个唯一形状的实际预检逐项核对；新二进制在 `e16_final_preflight_3g.log` 中对 n256/r2–5 接受，对 n≥1000 先报 `HARD_CAP_D_GT_256`，均发生在 keygen 前。

本机逐档可用内存约 7.52–7.55 GB，始终高于各档 `3×` Dealer 预算；C 盘余量约 92 GB，高于 8 GiB 门；包格式 `u32` 长度可覆盖 192 MiB；每个正式子进程上限 120 s，T/P0/P1 各有 3 GiB 地址空间上限。n256 分档预算与观测峰值如下。预算是 admission 规则，峰值是运行观测，二者不可互换。

| r | 每方槽 | 每方包 B | Dealer 预算 B | 3×预算 B | 最高 T/P0/P1 RSS KiB | 实际门 |
|---:|---:|---:|---:|---:|---|---|
| 2 | 65,280 | 69,999,474 | 627,104,656 | 1,881,313,968 | 348,932 / 146,176 / 146,176 | ACCEPTED |
| 3 | 97,920 | 104,763,122 | 905,213,840 | 2,715,641,520 | 452,276 / 216,320 / 216,320 | ACCEPTED |
| 4 | 130,560 | 139,526,770 | 1,183,323,024 | 3,549,969,072 | 685,052 / 286,720 / 286,720 | ACCEPTED |
| 5 | 163,200 | 174,290,418 | 1,461,432,208 | 4,384,296,624 | 798,948 / 357,120 / 357,120 | ACCEPTED |

下面为**逐配置**状态；每方包及 Dealer 预算的完整 32 行数值在 `final_v2/configuration_status_32.csv`。`MEASURED` 指 AAV86 的 LAN/WAN 均完成 1+5，且相同 n/K 的 r 无关全对全基线可配对；`NOT_RUN_E16_NO_BRIDGE` 的九指标均为 `NOT_MEASURED`，仅有分栏的 E15 结果；`PRECHECK_REJECTED` 的九指标均为 `NOT_MEASURED`，仅有公式容量与实际预检结论。

| n | K | r | D | 每方槽 | E16 预检/状态 | 证据范围 |
|---:|---:|---:|---:|---:|---|---|
| 128 | 2 | 2 | 128 | 16,256 | NOT_RUN_E16_NO_BRIDGE | E15 分栏 |
| 128 | 2 | 3 | 128 | 24,384 | NOT_RUN_E16_NO_BRIDGE | E15 分栏 |
| 128 | 2 | 4 | 128 | 32,512 | NOT_RUN_E16_NO_BRIDGE | E15 分栏 |
| 128 | 2 | 5 | 128 | 40,640 | NOT_RUN_E16_NO_BRIDGE | E15 分栏 |
| 128 | 8 | 2 | 128 | 16,256 | NOT_RUN_E16_NO_BRIDGE | E15 分栏 |
| 128 | 8 | 3 | 128 | 24,384 | NOT_RUN_E16_NO_BRIDGE | E15 分栏 |
| 128 | 8 | 4 | 128 | 32,512 | NOT_RUN_E16_NO_BRIDGE | E15 分栏 |
| 128 | 8 | 5 | 128 | 40,640 | NOT_RUN_E16_NO_BRIDGE | E15 分栏 |
| 256 | 2 | 2 | 256 | 65,280 | ACCEPTED / MEASURED | 配对九指标 |
| 256 | 2 | 3 | 256 | 97,920 | ACCEPTED / MEASURED | 配对九指标 |
| 256 | 2 | 4 | 256 | 130,560 | ACCEPTED / MEASURED | 配对九指标 |
| 256 | 2 | 5 | 256 | 163,200 | ACCEPTED / MEASURED | 配对九指标 |
| 256 | 8 | 2 | 256 | 65,280 | ACCEPTED / MEASURED | 配对九指标 |
| 256 | 8 | 3 | 256 | 97,920 | ACCEPTED / MEASURED | 配对九指标 |
| 256 | 8 | 4 | 256 | 130,560 | ACCEPTED / MEASURED | 配对九指标 |
| 256 | 8 | 5 | 256 | 163,200 | ACCEPTED / MEASURED | 配对九指标 |
| 1,000 | 80 | 2 | 1,024 | 1,047,552 | PRECHECK_REJECTED | D>256 首门、仅容量 |
| 1,000 | 80 | 3 | 1,024 | 1,571,328 | PRECHECK_REJECTED | D>256 首门、仅容量 |
| 1,000 | 80 | 4 | 1,024 | 2,095,104 | PRECHECK_REJECTED | D>256 首门、仅容量 |
| 1,000 | 80 | 5 | 1,024 | 2,618,880 | PRECHECK_REJECTED | D>256 首门、仅容量 |
| 10,000 | 80 | 2 | 16,384 | 268,419,072 | PRECHECK_REJECTED | D>256 首门、仅容量 |
| 10,000 | 80 | 3 | 16,384 | 402,628,608 | PRECHECK_REJECTED | D>256 首门、仅容量 |
| 10,000 | 80 | 4 | 16,384 | 536,838,144 | PRECHECK_REJECTED | D>256 首门、仅容量 |
| 10,000 | 80 | 5 | 16,384 | 671,047,680 | PRECHECK_REJECTED | D>256 首门、仅容量 |
| 100,000 | 80 | 2 | 131,072 | 17,179,738,112 | PRECHECK_REJECTED | D>256 首门、仅容量 |
| 100,000 | 80 | 3 | 131,072 | 25,769,607,168 | PRECHECK_REJECTED | D>256 首门、仅容量 |
| 100,000 | 80 | 4 | 131,072 | 34,359,476,224 | PRECHECK_REJECTED | D>256 首门、仅容量 |
| 100,000 | 80 | 5 | 131,072 | 42,949,345,280 | PRECHECK_REJECTED | D>256 首门、仅容量 |
| 1,000,000 | 80 | 2 | 1,048,576 | 1,099,510,579,200 | PRECHECK_REJECTED | D>256 首门、仅容量 |
| 1,000,000 | 80 | 3 | 1,048,576 | 1,649,265,868,800 | PRECHECK_REJECTED | D>256 首门、仅容量 |
| 1,000,000 | 80 | 4 | 1,048,576 | 2,199,021,158,400 | PRECHECK_REJECTED | D>256 首门、仅容量 |
| 1,000,000 | 80 | 5 | 1,048,576 | 2,748,776,448,000 | PRECHECK_REJECTED | D>256 首门、仅容量 |

计数：8 `MEASURED`、16 `PRECHECK_REJECTED`、8 `NOT_RUN_E16_NO_BRIDGE`。最小被拒 n=1000/r2 的单方包公式已达 1,167,830,130 B，Dealer 预算 9,409,749,904 B，亦超过当前包与预算门；实际**首个**拒绝理由仍是 `D>256`，不能称材料生成资源失败。

## 3. 完整路径、计时口径与运行环境

每档先单独记录 `r*_v2_prerun_resources.log`，复核当时内存、磁盘、预检与上一档实际峰值，然后顺序执行六例 conformance → 六例冻结 oracle differential → 六例独立 T/P0/P1 进程 E2E，全部通过后才启动该档正式批次。r=5 的 2 GiB 失败与 3 GiB 修复各有原始日志，不作为正式性能样本。全对全 n256 非正式诊断确认实际固定材料形状；AAV86 各档 E2E 核对真实序列化包。最终正常 CTest 相关 5/5 PASS；`git diff --check` PASS。

环境：Ubuntu 24.04 WSL2，Linux 6.6.87.2-microsoft-standard-WSL2，Intel i9-13980HX、32 逻辑 CPU，GCC 13.3、CMake 3.28.3，`Release` (`-O3 -DNDEBUG`)，`MOE_TOPK_ENABLE_EMP_OT=ON`。AAV86 二进制 SHA-256 `A1648F24DA0C6DDD88A3A6B561700A2907ECE57125068D25A9F210E6E013AF79`，基线 `A07737585DC14ABABCDD8DF915FBC69FC11C26F61C75C89D52B5B4399B8C3A69`。每批独立 network namespace 的 loopback `netem`：LAN 目标 RTT 1 ms/1000 Mbit/s，实校准中位 RTT 1.119–1.201 ms、吞吐 693–868 Mbit/s；WAN 目标 50 ms/100 Mbit/s，实校准 50.393–50.543 ms、62.87–63.04 Mbit/s。这是同主机**模拟**网络，不是跨主机 LAN/WAN。量的是 framed application bytes，TCP/IP 线速开销 `NOT_MEASURED`。

输入为含端点的整数 `[-131072,131072]` 对应 signed Q20.12 raw score。`final_v2/input_plan.json` 的 12 个独立 uint64 输入种子按 K=2/8、每组重复 0–5 配对给两路线及所有 r；每次运行新生 Dealer/EMP 材料和份额。AAV86 公开算法种子和真实 `e_A/v_A` 向量逐行留在 JSONL；T 的材料 claim、包和密钥不落盘。旧名 `MOE_TOPK_M6A_E15_BENCH=1` 是沿用的 TEST_ONLY 开关，控制 fresh share/关闭计时内诊断，**不是 E15 数据标签**。正式 runner 检查工作树 clean、HEAD 不变、输入计划、校准和子进程退出；逐行保存命令、源码及二进制哈希、输入摘要、阶段收发/时间、reserved slots、在线 PRG、实际比较边与 T/P 峰值。

计时沿 E15 修正合同：TCP 建连单列；离线从角色启动前至 T 退出且两方 ready；在线取两个 secure 入口时钟的较大值；每次 `total=offline+online` 后再统计。有效载荷计数在在线时钟停止后读取；AAV86 `offline_material_total_bits` 是已核验固定布局下两方在线留存有效载荷公式，基线加计双方本地 shuffle 留存材料；不把 T package 头/传输字节混入材料。在线 PRG 为传统 DCF `traverseOneDCF` 的**长度倍增调用**，不是所有 AES/PRG 操作；其他 AES/PRG 细分为 `NOT_MEASURED`。在线轮数为因果轮，AAV86 `2r+4`，基线 8；实际比较边为在线启用的边，不是预留全池槽位。

可复现命令入口：`e16_shaped_namespace.sh <LAN|WAN> <AAV86 E2E binary> <final_v2/rX/aav86> <E16 root> <input_plan.json> <r>`；基线 `e16_clique_shaped_namespace.sh <LAN|WAN> <clique benchmark binary> <final_v2/r2/baseline> <E16 root> <对应 AAV86 r2 JSONL> <input_plan.json>`。两者由 Ubuntu WSL root 启动 network namespace，内部 `runuser moeaudit` 和精确 3 GiB `prlimit`；JSONL 的 `command` 保存每次 n/K/r/seed/serial。校准 JSON 和 qdisc before/after 原件亦在各目录。

## 4. 九指标同口径结果

最终 `final_v2/unified_nine_metrics.csv` 含 **20 组 × 9 指标 × median/min/max**。下表仅列每组五次正式运行的中位数，详细 min/max、分方在线通信、阶段计时和峰值以 CSV/JSONL 为准。材料为 MiB、在线通信为总 KiB；分方在线通信在本次对称实现中为总量一半，原始分方收发仍单列核对。基线 r 无关，按相同 n/K/输入计划复用于 r=2–5 的每个配对比较。

| 网络 | 路线 | K | r | 离线 ms | 在线 ms | 总 ms | 材料 MiB | 在线总 KiB | 轮 | 在线 PRG | 实际边 |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| LAN | AAV86 | 2 | 2 | 820.09 | 45.83 | 879.08 | 126.38 | 40.75 | 8 | 1,201,232 | 6,900 |
| LAN | AAV86 | 8 | 2 | 723.44 | 45.86 | 771.24 | 126.38 | 40.75 | 8 | 1,267,488 | 7,304 |
| LAN | AAV86 | 2 | 3 | 1,087.35 | 91.38 | 1,270.99 | 189.13 | 48.94 | 10 | 800,580 | 4,457 |
| LAN | AAV86 | 8 | 3 | 1,071.80 | 233.01 | 1,323.07 | 189.13 | 48.94 | 10 | 799,268 | 4,449 |
| LAN | AAV86 | 2 | 4 | 1,503.23 | 36.21 | 1,585.12 | 251.89 | 57.12 | 12 | 647,076 | 3,521 |
| LAN | AAV86 | 8 | 4 | 1,527.45 | 35.11 | 1,724.91 | 251.89 | 57.12 | 12 | 617,556 | 3,341 |
| LAN | AAV86 | 2 | 5 | 1,679.40 | 34.52 | 1,713.92 | 314.65 | 65.31 | 14 | 559,992 | 2,990 |
| LAN | AAV86 | 8 | 5 | 1,917.38 | 163.72 | 2,130.13 | 314.65 | 65.31 | 14 | 556,056 | 2,966 |
| LAN | 全对全 | 2 | — | 430.32 | 147.80 | 576.85 | 64.74 | 408.59 | 8 | 5,422,592 | 32,640 |
| LAN | 全对全 | 8 | — | 436.85 | 146.88 | 585.38 | 64.74 | 408.59 | 8 | 5,422,592 | 32,640 |
| WAN | AAV86 | 2 | 2 | 720.66 | 449.88 | 1,188.97 | 126.38 | 40.75 | 8 | 1,281,920 | 7,392 |
| WAN | AAV86 | 8 | 2 | 709.95 | 455.02 | 1,173.14 | 126.38 | 40.75 | 8 | 1,179,748 | 6,769 |
| WAN | AAV86 | 2 | 3 | 1,127.37 | 637.69 | 1,790.14 | 189.13 | 48.94 | 10 | 723,828 | 3,989 |
| WAN | AAV86 | 8 | 3 | 1,117.24 | 549.38 | 1,700.47 | 189.13 | 48.94 | 10 | 744,492 | 4,115 |
| WAN | AAV86 | 2 | 4 | 1,450.30 | 651.18 | 2,274.44 | 251.89 | 57.12 | 12 | 675,120 | 3,692 |
| WAN | AAV86 | 8 | 4 | 1,452.80 | 656.86 | 2,104.63 | 251.89 | 57.12 | 12 | 615,260 | 3,327 |
| WAN | AAV86 | 2 | 5 | 1,874.27 | 751.01 | 2,659.47 | 314.65 | 65.31 | 14 | 548,512 | 2,920 |
| WAN | AAV86 | 8 | 5 | 2,101.63 | 738.05 | 2,828.80 | 314.65 | 65.31 | 14 | 534,736 | 2,836 |
| WAN | 全对全 | 2 | — | 437.96 | 814.93 | 1,248.15 | 64.74 | 408.59 | 8 | 5,422,592 | 32,640 |
| WAN | 全对全 | 8 | — | 517.44 | 810.92 | 1,323.22 | 64.74 | 408.59 | 8 | 5,422,592 | 32,640 |

本机样本中 AAV86 以较多离线材料和预处理时间换取明显更少的在线通信、在线 DCF PRG 与实际比较边。r=2 的 AAV86 在线总通信为基线约十分之一，离线材料约 1.95 倍；r=5 为约六分之一通信、约 4.86 倍材料。LAN 四档的总时间中位均高于基线；WAN 的 r=2 两个 K 总时间中位低于基线，r=3–5 高于基线。五次样本有明显尾值（例如 LAN r=4/K=2 总时间 1,440.05–8,709.16 ms），故这些仅是所列环境的中位观察，不作统计显著性或真实异机网络结论。九指标的每组 min/max 均在统一 CSV 中，不能把分指标中位数相加当成总时间中位数。

## 5. 独立审计、完整性与失败留存

最终 10 个 profile/route/r 批次共 120 行，其中 100 正式、20 预热；20 个五次组、24 个 K×重复×网络输入配对组。`e16_audit_results.py` 从原始 JSONL 重算每组全部统计，并核对各行 n/D/K/r、干净 HEAD、源码和二进制哈希、输入计划与摘要、原始日志 SHA、T/P 退出、oracle/exact-K 通过标记、总时间恒等式、两方收发及阶段守恒、轮数、预留/活跃边、材料和 PRG 定义；结果为 `final_v2/independent_audit.json` 的 `PASS`。oracle 本身由冻结差分夹具及两个独立进程入口逐次按原序 XOR mask 与稳定同分 `top_k_mask` 比对；审计不把这个 TEST_ONLY 明文重构移入 secure 路径。

`e16_raw_complete_index.csv` 列出 **376 个** E16 原始文件及 SHA-256，索引自身 SHA-256 `326BB0A2F490A79EE709A09726B59808F4BEBC754BCAB7E8E75FFD7645F3C0EE`，全部 376 项已逐文件复验。仓库外副本 `C:\Users\28641\Desktop\MoE_Top-k_Reproduction_Evaluation_Evidence\M6A\E16_7515aac_final_v2` 再验 376/376 与原件一致。E16 原始目录被 `.gitignore` 排除，未把日志、生成材料或构建物提交进 Git。

失败与修复留存：第一版 r=2 conformance 的 D256 槽数低字节为零，旧 TEST_ONLY 篡改夹具未真正改变值；修为按位翻转后常规/D256 均通过。第一版 E2E 的 TEST_ONLY edge trace 仍写死 `<128`，改为实际 padded D 后通过。第一版 r=2 WAN 在三次运行后因共享 Git `packed-refs` 瞬时读取错误中断；三条原样放在 `final/r2/aav86/attempts/WAN_git_ref_read_error_1/`，整批从预热重新开始，未选取部分样本。第一版 r=5 的 2 GiB `std::bad_alloc` 两次记录与 3 GiB 通过记录均在 gate 目录。所有失败都保留，不以零代替未测指标。

## 6. 修改文件与交付判定

- 资源门/secure：`VFSS/include/moe_topk/protocol_i_aav86_small.h`、`VFSS/src/moe_topk/protocol_i_aav86_small.cpp`；只扩大有界 D256/包/预算/地址空间准入，不改变算法、材料布局、在线角色或计时边界。
- TEST_ONLY 夹具及入口：`VFSS/tests/moe_topk/protocol_i_aav86_e10_metrics_conformance_test.cpp`、`protocol_i_aav86_e9_fixtures.h`、`protocol_i_aav86_small_conformance_test.cpp`、`protocol_i_aav86_small_differential_test.cpp`、`protocol_i_aav86_small_e2e_test.cpp`、`protocol_i_e12_baseline_bench_test.cpp`。
- E16 独立工具：`experiments/m6a_p2_i_allpairs/TEST_ONLY/e16_resource_gate.py`、`e16_input_plan.py`、`e16_run_matrix.py`、`e16_run_clique_baseline.py`、`e16_shaped_namespace.sh`、`e16_clique_shaped_namespace.sh`、`e16_audit_results.py`、`e16_export_unified.py`、`e16_finalize_status.py`。
- 治理与文档：`.gitignore`、`.gitattributes`、`docs/decisions/M6A_P2_I_E16_STAGED_RESOURCE_GATE_2026-10-03.md`、`docs/IMPLEMENTATION_PLAN.md`、`docs/BENCHMARK_VALIDATION_PLAN.md`、本报告。

**交付判定**：n=256 的八个 `(K,r)` 形状具备两路线同 revision、同输入、同计时合同的九指标性能结论；n≥1000 的十六形状仅具备容量/实际 preflight 拒绝结论；n=128 的八形状未做 E16 桥接，E15 单独报告。E16 分支提交供异会话接收，不合并、不改写 E15。Protocol III+AAV86、M6A 总验收和论文作者精确复现仍未完成。
