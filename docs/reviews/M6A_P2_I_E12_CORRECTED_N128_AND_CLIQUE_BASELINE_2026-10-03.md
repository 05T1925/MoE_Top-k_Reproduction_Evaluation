# M6A-P2-I-E12：计量合同修正、n=128 重测与全对全 Protocol I 基线

日期：2026-10-03。状态：**E12_CORRECTED_N128 = PASS；NETWORK_GATE = GO（同主机模拟网络）；BASELINE_COMPARISON = PASS_WITH_LIMITS；V3_FULL = NOT_COMPLETE。** 本报告是本执行窗口的工程证据，修改后的代码和性能结论仍需下一次异会话接收。`AUTHOR_EXACT=NOT_PROVEN`。

## 1. 起点、证据冻结和版本

只在 `C:/Users/28641/Desktop/MoE_Top-k_Reproduction_Evaluation__m6a_p2_i_e11_benchmark` 工作。起始分支 `codex/m6a-p2-i-e11-benchmark`，HEAD `46009b6b8e10633d114fb00f899879f545c49f48`，`git status --short` 只有未跟踪的 `docs/reviews/M6A_P2_I_E11_CROSS_CHAT_RECEIVER_AUDIT_2026-10-03.md`。该接收原件 SHA-256 为 `04B2C713836EA2D033B79618DA508CDC6C80693D0047E701F995F9F04F75A78D`，内容与字节均保留并在本次显式纳入提交。E11 原始 LAN/WAN JSONL SHA-256 分别为 `2DD8E9036F34553308E11A711B5468966458DC44E5E66C39046E8094031E2522`、`B35B3DCB849980420060B8F2F02F57170CF9A94CC33DB5155712A67DA7AAD056`，本次未覆盖。

E11 **实际被测**提交是 `103b76d863e68c2c318998db01073591e6a8fce8`，`46009b6` 是其后写报告的提交。E11 异会话接收为 `PASS_WITH_METRIC_FINDINGS`，定位离线起表过晚、阶段接收字节由对方发送回填，以及旧 `core_time_ns` 包含 carrier。本次依次建立本地检查点 `d62f437`（计量和基线适配）、`6d2dcec`（测试输入 seed 与 T 参数分离）、`8ad0725c73716ac37c96d417136958f932233d61`（基线逐阶段审计）；**后者是全部 `accepted/` 正式运行当刻的 HEAD**。逐次记录检查 tracked 状态为 `CLEAN`。后续只改审计脚本读取键和本报告，不追认其为被测源码。

证据类别：AAV86/CCS'24 的算法与 CA 轮数是论文定义；旧全对全 Protocol I 源码、E11 测试和本次构建是本地实现事实；可信 T 全两两预发、统一 D、signed Q20.12、稳定同分、原序 XOR mask、同置换逆路由和网络 runner 是项目扩展；整池 FSS/PRG 安全、侧信道及大规模性能仍是条件性或未验证事项。没有改固定 M、在线补料、Protocol III、BB90 或运行时输出语义。

## 2. 修正结果与精确计量合同

| Finding | 源码证据和 E12 修正 | 验收 |
| --- | --- | --- |
| 离线起点晚于 T fork | AAV86 E2E harness 在建立 fd/TCP 后、**首次**启动 P0/P1/T 前起表；T 退出且两方 ready 后止表。TCP 建连单列 `transport_setup_ms`。旧 E11 行原样保留。 | 小实例、D128 与逐次离线子区间审计通过。 |
| 阶段接收量非本方实测 | secure 入口按本方 `message_trace.received_bytes` 聚合 score、CA 组合、inverse；party 结果传回真实三段计数。同方阶段和=本方总接收，本方各阶段接收=对方对应发送。 | 120 次最终双路线记录逐行守恒；AAV86 阶段接收直接源自本方 trace。 |
| `core` 含 carrier | 保留原 `core_time_ns` 作为组合阶段，另在公开 full-order flatten 后、carrier 构造前起相邻边界；直接实测 `ca_time_ns` 与 `carrier_time_ns`，严格满足二者和=组合时间。新 JSONL 字段名为 `combination_p{0,1}_ms`、`ca_p{0,1}_ms`、`carrier_p{0,1}_ms`。 | 两方运行时与独立审计均通过。`ca_time_ns` 包含首轮 shuffle、逐轮 CA 和 flatten，不冒称作者纯 CA 实测。 |
| 基线输入 seed 进入 T 参数 | 在 `d62f437` 初版基线试跑时发现控制器把测试输入 seed 复用为材料标签；`6d2dcec` 后控制器单独持有输入 seed，T exec 参数改用另一 session/运行标签和公开形状。固定测试 FSS seed 改为系统随机种子，score/edge mask 改为 OS 随机，OT 继续使用真实 EMP/OpenSSL。E13 复核发现两个标签仍可由公开 K/重复编号日程互推，因此这里的“分离”仅表示未直接传参，不能作为合成输入隐私证明。 | 初版基线 LAN 整批保留但**作废**；`8ad0725` 下重新测量。T 发材并退出后才发送输入份额。 |

本次 `total_time_ms = offline_time_ms + max(online_p0_ms, online_p1_ms)`。它是两个定义明确的阶段之和，测试输入发送、oracle、报告收集以及另列的连接建立时间不包含在内。T generate/serialize/distribute 和 T 退出后的 party ready barrier 各保留实测原值；不能将它们简单相加充作离线包围时间。决策口径见 `docs/decisions/M6A_P2_I_E12_MEASUREMENT_CONTRACT_2026-10-03.md`，实施与 benchmark 计划已同步。

AAV86 实际 trace 验证两轮 score、`2r+1` 轮 CA 和一轮同置换逆路由，故**仅当前 signed raw-score→原序 XOR mask 入口**为 `2r+4`。全对全基线为两轮 score、两轮 forward、一轮 masked comparison open、一轮 rank reveal、两轮 reverse，完整路径 8 轮。每方阶段字节均由真实 channel 计数，接收用于对账；`online_comm_total_bits=8·(P0_sent+P1_sent)`，不重复加接收。AAV86 的 `e_t` 为当轮去重无序边，`v_t` 为当轮活跃边端点去重数；跨轮分别求和为 `e_A/v_A`。传统 DCF seed→两块长度倍增 PRG 按 party 计数，其他 AES/PRG 细分 `NOT_MEASURED`。基线没有相同可信 PRG 计数，不能以边数或 DCF Eval 替代。

## 3. 实现和验证门

独立 Release 构建使用 Ubuntu 24.04.4 WSL2、GCC 13.3、CMake 3.28.3、Ninja 1.11.1、Intel i9-13980HX（32 逻辑 CPU）、7.6 GiB WSL 内存；两构建均为 `-O3 -DNDEBUG`。AAV86 构建 `/tmp/m6a12-release` 为 `MOE_TOPK_ENABLE_EMP_OT=OFF`；基线构建 `/tmp/m6a12-emp-release` 为 `ON`，真实 emp-tool/emp-ot 安装前缀 `/tmp/moe_m28_emp.ok9WzQ/prefix`、OpenSSL 3.0.13。两者均使用独立 T/P0/P1 fork+exec，协议各方单线程，正式 runner 限制 `RLIMIT_AS=805306368` B；D128 的内存与 64 MiB 单方包检查保持原样。AAV86 二进制 SHA-256 `805E2B5B96653ED6E5C1A0C2428A1A0ECB41A9A1DC5C9D6ABBD2975579578904`，基线二进制 `C767797CD712C2DCDA8D522AC1E9F7AD13E71E4DE35B1A8AE10DE4BB913A6456`。

关键工作树源码 SHA-256：AAV86 头文件 `576F4A683DDC8702F3BB18BBD6933824B07919D7A2F1AC0F8A5845F0B7B9D580`，实现 `FB38C894E0266B2D426BD07CFFAD6AE8365D9E5DC2594BE1A42F7660CCB4BAC0`，E2E harness `7A5994F62C118C3D9E0AAC4A948B9ABFEF48EC9532678736A6F32FAB63E3AFAF`，新基线 harness `F4028304A101CBE82B08F54E4C08E04FD3E8A9869AAF8FEE319F658A72FBB652`。每条原始行另有相关源码及二进制哈希；Git LF blob 与 Windows 工作树字节哈希因 `core.autocrlf` 可不同。

验证顺序和本轮结果：AAV86 conformance/计量 `2/2 PASS` → 冻结 oracle differential `1/1 PASS` → 小 D 计量和 D128（`prlimit` 下）三进程 E2E，D128 `6/6 PASS` → Unix/TCP 同材料夹具 `1/1 PASS`、phase frame 对照和关闭/静默/截断负面用例全部 PASS → Protocol I/III 相关旧测试 `18/18 PASS`。无 `RLIMIT_AS` 的 D128 一次调用在 T 的预检阶段按预期拒绝，随后使用规定的 768 MiB 限制通过；拒绝不算协议失败。EMP-ON 原 `moe_topk_m2_protocol_i_modular_e2e_test` `1/1 PASS`，新基线 fixture 的多尺寸 conformance/oracle/三进程 `1/1 PASS`；最终复跑两者合计 `2/2 PASS`。n128 的 Unix 与 TCP 分别以 fresh 材料、同输入 seed 通过原序 oracle、恰 K 位和阶段守恒。E11 的 `27/27` 是历史回归证据，不能替代上述 E12 实际结果。测试不能证明 AES/DCF 安全。

本轮独立复现 Release `bitpack_test` 链接失败：`bitpack::mod(unsigned long,int)` 在 `VFSS/ext/bitpack/src/bitpack/bitpack.cpp` 被定义为仅本翻译单元的 `inline`，`tests/test.cpp` 外部引用找不到符号。它阻断无选择的 `all` 目标；AAV86 和真实 EMP 基线的选定协议目标均成功构建和运行，相关调用链不依赖该测试目标。未为本计量任务改动 bitpack。

## 4. 网络门与完整子矩阵

同一 WSL2 主机上，两个角色实际在线 TCP loopback fd 经过独立 Linux network namespace 的 `lo` qdisc；T 离线发材与 party OT/ready 使用本地 IPC。每个 profile/路线重新建 namespace、校准 20 次 TCP echo RTT 与 8 MiB TCP 应用吞吐，记录 `tc -s qdisc` 校准前、校准后和协议后原文；退出后 namespace 清理，宿主 `lo` 为 `qdisc noqueue`。它是**模拟 LAN/WAN**，不是物理双机。

| 路线/标签 | qdisc 目标（单向 delay、rate） | 实测 RTT 中位 ms | TCP 应用吞吐 Mbit/s | 协议期 qdisc 增量 |
| --- | --- | ---: | ---: | ---: |
| AAV86 / LAN | 0.5 ms，1000 Mbit/s | 1.230 | 835.419 | 1,825,026 B / 7,393 包 |
| AAV86 / WAN | 25 ms，100 Mbit/s | 50.342 | 62.654 | 1,825,092 B / 7,394 包 |
| 全对全 / LAN | 0.5 ms，1000 Mbit/s | 1.166 | 836.546 | 2,314,440 B / 1,380 包 |
| 全对全 / WAN | 25 ms，100 Mbit/s | 50.504 | 63.064 | 2,317,674 B / 1,429 包 |

qdisc 计数包含 TCP 控制/封装，不能当成协议应用层通信。四次校准均在预先固定的 LAN RTT 0.5–5 ms/吞吐 200–1200 Mbit/s、WAN RTT 35–70 ms/吞吐 20–110 Mbit/s 门内。**每配置** AAV86 在 `n=128,K∈{2,8},r∈{2,3,4,5}` 下每网络 1 次预热加 5 次正式；全对全没有 r，对每个 K/repetition 的**同一输入**只测一组 fresh 基线，再对应四个 r。总计 AAV86 96 次（80 正式）、基线 24 次（20 正式），无失败的 accepted 行。正式输入为端点包含的均匀整数 `[-32·2^12,32·2^12]`，seed `0xE110000+100K+repetition`；AAV86 公开 pivot seed `0xA110000+1000r+repetition`。秘密 π、mask、DCF/OT 随机性和材料 ID 每次新生，未筛选小图。

下表为五次正式运行**分别求出的中位数**，单位 ms；每字段五次 min/max、逐次值和失败记录在被忽略目录的 summary/JSONL 中。总时间列是逐次先求 `offline+online` 再取中位数，不等于两列中位数直接相加。

| 网络 | K | 路线 / r | 离线 | 在线完整入口 | 总时间 | 在线通信 bit |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| LAN | 2 | 全对全 | 167.222 | 45.586 | 214.362 | 1,479,424 |
| LAN | 2 | AAV86 / 2 | 187.014 | 22.222 | 210.428 | 169,984 |
| LAN | 2 | AAV86 / 3 | 256.201 | 21.948 | 279.289 | 204,288 |
| LAN | 2 | AAV86 / 4 | 330.833 | 21.822 | 353.201 | 238,592 |
| LAN | 2 | AAV86 / 5 | 495.390 | 23.900 | 533.520 | 272,896 |
| LAN | 8 | 全对全 | 164.374 | 45.133 | 209.692 | 1,479,424 |
| LAN | 8 | AAV86 / 2 | 193.912 | 22.341 | 216.253 | 169,984 |
| LAN | 8 | AAV86 / 3 | 274.969 | 20.830 | 296.298 | 204,288 |
| LAN | 8 | AAV86 / 4 | 363.249 | 22.090 | 385.339 | 238,592 |
| LAN | 8 | AAV86 / 5 | 451.167 | 24.269 | 489.290 | 272,896 |
| WAN | 2 | 全对全 | 166.875 | 501.507 | 667.265 | 1,479,424 |
| WAN | 2 | AAV86 / 2 | 199.241 | 421.559 | 625.858 | 169,984 |
| WAN | 2 | AAV86 / 3 | 269.438 | 521.745 | 789.816 | 204,288 |
| WAN | 2 | AAV86 / 4 | 374.014 | 616.040 | 990.053 | 238,592 |
| WAN | 2 | AAV86 / 5 | 491.262 | 722.627 | 1213.889 | 272,896 |
| WAN | 8 | 全对全 | 165.433 | 502.958 | 668.757 | 1,479,424 |
| WAN | 8 | AAV86 / 2 | 191.163 | 428.702 | 616.169 | 169,984 |
| WAN | 8 | AAV86 / 3 | 265.800 | 517.110 | 782.920 | 204,288 |
| WAN | 8 | AAV86 / 4 | 370.485 | 622.444 | 992.929 | 238,592 |
| WAN | 8 | AAV86 / 5 | 437.461 | 718.726 | 1155.595 | 272,896 |

这些是本机模拟网络的完整路径对照，不能把 AAV86 CA 局部时间与基线端到端时间混比，也不能将在线较快视为总时间总是较快。全两两离线包随 r 增长：每方预留 `r·C(128,2)=16,256/24,384/32,512/40,640` 槽，实测每方序列化包 `17,160,690/25,622,962/34,085,234/42,547,506` B；在线只 Eval 活跃边。基线每方 8,128 对、T package `8,696,624` B，另有 party 本地 EMP OT 材料，完整离线材料量 `NOT_MEASURED`。AAV86 score DCF Eval 每方 512，CA Eval 每方 `2e_A`；基线 DCF Eval 每方 16,768。AAV86 长度倍增 PRG score/CA 逐轮在原始行；基线 PRG 和其他 AES 细分 `NOT_MEASURED`。所有正式行 T/P0/P1 峰值上界分别为 AAV86 `227,328/90,624/90,624 KiB`，基线 `55,396/23,200/23,200 KiB`，仅限本环境及输入批次。

## 5. 完整 V3 状态与容量

下表的槽数和包字节为 checked 公式/真实 preflight 的**解析值**，仅 `n=128` 有本次 LAN/WAN 五次正式运行。`n≥256` 的实际 preflight 首先拒绝 `D>128`，未生成全池；其 64 MiB 包门及预算门也分别按公式记录，不因首先命中的原因而消失。`n=256,r=2` 每方解析包已是 `69,999,474 B > 67,108,864 B`。不扩上限、不缩 n、不改固定 M 或在线 T。

| n | K | r | D | 每方槽 | 每方包 B | E12 状态 |
| ---: | --- | ---: | ---: | ---: | ---: | --- |
| 128 | 2/8 | 2 | 128 | 16,256 | 17,160,690 | LAN/WAN 5/5 PASS |
| 128 | 2/8 | 3 | 128 | 24,384 | 25,622,962 | LAN/WAN 5/5 PASS |
| 128 | 2/8 | 4 | 128 | 32,512 | 34,085,234 | LAN/WAN 5/5 PASS |
| 128 | 2/8 | 5 | 128 | 40,640 | 42,547,506 | LAN/WAN 5/5 PASS |
| 256 | 2/8 | 2 | 256 | 65,280 | 69,999,474 | HARD_CAP；未 keygen |
| 256 | 2/8 | 3 | 256 | 97,920 | 104,763,122 | HARD_CAP；未 keygen |
| 256 | 2/8 | 4 | 256 | 130,560 | 139,526,770 | HARD_CAP；未 keygen |
| 256 | 2/8 | 5 | 256 | 163,200 | 174,290,418 | HARD_CAP；未 keygen |
| 1,000 | 80 | 2 | 1,024 | 1,047,552 | 1,167,830,130 | HARD_CAP；未 keygen |
| 1,000 | 80 | 3 | 1,024 | 1,571,328 | 1,750,801,010 | HARD_CAP；未 keygen |
| 1,000 | 80 | 4 | 1,024 | 2,095,104 | 2,333,771,890 | HARD_CAP；未 keygen |
| 1,000 | 80 | 5 | 1,024 | 2,618,880 | 2,916,742,770 | HARD_CAP；未 keygen |
| 10,000 | 80 | 2 | 16,384 | 268,419,072 | 324,549,132,402 | HARD_CAP；未 keygen |
| 10,000 | 80 | 3 | 16,384 | 402,628,608 | 486,808,592,498 | HARD_CAP；未 keygen |
| 10,000 | 80 | 4 | 16,384 | 536,838,144 | 649,068,052,594 | HARD_CAP；未 keygen |
| 10,000 | 80 | 5 | 16,384 | 671,047,680 | 811,327,512,690 | HARD_CAP；未 keygen |
| 100,000 | 80 | 2 | 131,072 | 17,179,738,112 | 22,007,488,315,506 | HARD_CAP；未 keygen |
| 100,000 | 80 | 3 | 131,072 | 25,769,607,168 | 33,011,111,624,818 | HARD_CAP；未 keygen |
| 100,000 | 80 | 4 | 131,072 | 34,359,476,224 | 44,014,734,934,130 | HARD_CAP；未 keygen |
| 100,000 | 80 | 5 | 131,072 | 42,949,345,280 | 55,018,358,243,442 | HARD_CAP；未 keygen |
| 1,000,000 | 80 | 2 | 1,048,576 | 1,099,510,579,200 | 1,487,639,764,009,074 | HARD_CAP；未 keygen |
| 1,000,000 | 80 | 3 | 1,048,576 | 1,649,265,868,800 | 2,231,458,679,226,482 | HARD_CAP；未 keygen |
| 1,000,000 | 80 | 4 | 1,048,576 | 2,199,021,158,400 | 2,975,277,594,443,890 | HARD_CAP；未 keygen |
| 1,000,000 | 80 | 5 | 1,048,576 | 2,748,776,448,000 | 3,719,096,509,661,298 | HARD_CAP；未 keygen |

逐点 `K` 展开、硬门/64 MiB/生成预算/内存门的各布尔值和实际首个拒绝原因，见忽略目录的 32 行 `e12_v3_capacity_status.csv`；SHA-256 `49127F82E321B8DF42317D2209CEE711052D0C056B99D491DB9824F516EC169E`。对应 checked C++ 容量与 preflight 原文 SHA-256 `B592956FA732A54B4926C96BD82362960E5E02EFCDC113A925D59CC1AB2C0CFA`。最大实际成功 D=128、r=5；更大 D 仅完成 checked 容量计算并被当前 runtime 预检拒绝。

## 6. 原始证据、复跑和保留的失败

最终有效原始目录为 `experiments/m6a_p2_i_allpairs/TEST_ONLY_E12_RAW/accepted/`（被 `.gitignore` 排除）：`aav86/`、`baseline/` 分别含 LAN/WAN JSONL、五次统计、每次独立日志、网络校准和 qdisc 三时点原文；目录另有环境、容量预检和 151 个原始文件的 SHA-256 索引。索引 `e12_raw_index.sha256` 的 SHA-256 为 `110C445F288539FF0AB5E4FB78451FAEF504C58ED164F33B37CC3EA1AFC12C88`；已运行 `sha256sum -c` 对账。每条 JSONL 自带单次日志 SHA-256、HEAD、tracked 状态、源码/二进制哈希、输入摘要/seed、公开算法 seed、配置、逐阶段时间和字节、`e_t/v_t`、DCF/PRG、T/P0/P1 峰值及退出状态；秘密材料和 transcript 不登记。

| 最终原始文件 | SHA-256 |
| --- | --- |
| AAV86 `aav86/LAN_runs.jsonl` | `2FD7F1D3A6D1C4BBBB82103D6CCE42297EAAEDC06414B9C9DE2F1A90A98B668A` |
| AAV86 `aav86/WAN_runs.jsonl` | `9AC21139A9158ABEC1B53F20141AB3B2145F25E717EA87E07FBC1B53C14CD9D3` |
| 全对全 `baseline/LAN_baseline_runs.jsonl` | `52F038C266A9FEB504D23D08189648B63914572DAD5E350AF8ACA904B2679D19` |
| 全对全 `baseline/WAN_baseline_runs.jsonl` | `478141F4257170D2626C792FC344C7CFEF0E70F0ECEE6D670DC577C7470FEBA4` |
| 环境 `e12_environment.txt` | `9D04D6E2CDD209B6F241CA7E761D2C6FC4186517AAFE2A18BF72B15810E5A2B8` |

独立程序 `e12_audit_results.py` 对 AAV86 `96/96` 行与 16 组 summary 全部 PASS；`e12_audit_clique_baseline.py` 对基线 `24/24` 行、四组 summary、AAV86 输入摘要配对和 qdisc 增量全部 PASS。第一次审计脚本误读 `p0_ca_ms` 键，修为实际字段 `ca_p0_ms` 后重跑；只变审计脚本，原始文件哈希与被测二进制不变。

失败与试跑均保留在忽略目录之外的其他子目录：首次 LAN wrapper 未用 root，`aav86/LAN_driver.log` 记录 `network namespace setup requires root`、无正式数据；`d62f437` 的初版 AAV86 LAN `aav86/LAN_runs.jsonl`、基线 LAN `baseline/LAN_baseline_runs.jsonl` 因发现基线 T 收到输入 seed 而不纳入对照；`6d2dcec` 的 `final/aav86/LAN_runs.jsonl` 因当时有短暂并发回归测试而整批排除。它们均未删除或重标为 accepted。D128 无资源限制调用的预检拒绝保存在 `/tmp/m6a12-d128-e2e.log`，随后有界通过记录在 `/tmp/m6a12-d128-e2e-rlimit.log`。E11 老批次不参与 E12 统计。

复跑命令（在 Ubuntu-24.04 WSL2 中；`M6A_ROOT` 为本隔离 worktree）：

```bash
M6A_ROOT=/mnt/c/Users/28641/Desktop/MoE_Top-k_Reproduction_Evaluation__m6a_p2_i_e11_benchmark
cmake -S "$M6A_ROOT/VFSS" -B /tmp/m6a12-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DMOE_TOPK_ENABLE_EMP_OT=OFF
cmake --build /tmp/m6a12-release --target moe_topk_m6a7_aav86_small_conformance_test moe_topk_m6a7_aav86_small_differential_test moe_topk_m6a7_aav86_small_e2e_test moe_topk_m6a10_aav86_metrics_conformance_test moe_topk_m6a11_tcp_material_equivalence_test -j 4
cmake -S "$M6A_ROOT/VFSS" -B /tmp/m6a12-emp-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DMOE_TOPK_ENABLE_EMP_OT=ON -DCMAKE_PREFIX_PATH=/tmp/moe_m28_emp.ok9WzQ/prefix
cmake --build /tmp/m6a12-emp-release --target moe_topk_m2_protocol_i_modular_e2e_test moe_topk_m6a12_protocol_i_clique_benchmark_test -j 4
ctest --test-dir /tmp/m6a12-release -R 'moe_topk_m6a7_aav86_small_conformance_test|moe_topk_m6a10_aav86_metrics_conformance_test' --output-on-failure
ctest --test-dir /tmp/m6a12-release -R moe_topk_m6a7_aav86_small_differential_test --output-on-failure
prlimit --as=805306368:805306368 -- env MOE_TOPK_M6A_E9_D=128 /tmp/m6a12-release/moe_topk_m6a7_aav86_small_e2e_test
ctest --test-dir /tmp/m6a12-emp-release -R moe_topk_m6a12_protocol_i_clique_benchmark_test --output-on-failure

# 以下四条各在 root WSL shell 串行运行；输出目录必须是新的空目录。
RAW="$M6A_ROOT/experiments/m6a_p2_i_allpairs/TEST_ONLY_E12_RAW/accepted"
bash "$M6A_ROOT/experiments/m6a_p2_i_allpairs/TEST_ONLY/e12_shaped_namespace.sh" LAN /tmp/m6a12-release/moe_topk_m6a7_aav86_small_e2e_test "$RAW/aav86" "$M6A_ROOT"
bash "$M6A_ROOT/experiments/m6a_p2_i_allpairs/TEST_ONLY/e12_shaped_namespace.sh" WAN /tmp/m6a12-release/moe_topk_m6a7_aav86_small_e2e_test "$RAW/aav86" "$M6A_ROOT"
bash "$M6A_ROOT/experiments/m6a_p2_i_allpairs/TEST_ONLY/e12_clique_shaped_namespace.sh" LAN /tmp/m6a12-emp-release/moe_topk_m6a12_protocol_i_clique_benchmark_test "$RAW/baseline" "$M6A_ROOT" "$RAW/aav86/LAN_runs.jsonl"
bash "$M6A_ROOT/experiments/m6a_p2_i_allpairs/TEST_ONLY/e12_clique_shaped_namespace.sh" WAN /tmp/m6a12-emp-release/moe_topk_m6a12_protocol_i_clique_benchmark_test "$RAW/baseline" "$M6A_ROOT" "$RAW/aav86/WAN_runs.jsonl"
python3 "$M6A_ROOT/experiments/m6a_p2_i_allpairs/TEST_ONLY/e12_audit_results.py" --raw-dir "$RAW/aav86" --head 8ad0725c73716ac37c96d417136958f932233d61
python3 "$M6A_ROOT/experiments/m6a_p2_i_allpairs/TEST_ONLY/e12_audit_clique_baseline.py" --raw-dir "$RAW/baseline" --aav86-dir "$RAW/aav86" --head 8ad0725c73716ac37c96d417136958f932233d61
```

上述原始命令实际写入带 `driver.log` 的新目录。脚本拒绝覆盖已有 JSONL/summary；复跑时须换空目录。`git diff --check`、显式暂存清单与缓存差异检查通过；最终 `VFSS-baseline/`、`Papers/`、旧参考树、密钥、原始日志、构建物均未进入提交。主工作区和 E11 原始资料未改；本分支只作本地提交，未推送、合并或建 PR。

## 7. 门禁与后续范围

- **功能**：n128 两路线原序 XOR Top-K mask、signed Q20.12 和原下标稳定同分按 oracle 逐次 PASS；小 D、极值、同分、非二次幂及故障关闭由本轮相关夹具复验。安全路径未重构分数、rank、比较位、选中下标或 mask；重构仅在 TEST_ONLY 控制器。
- **计量**：E11 三项字段口径在新 AAV86 批次闭合；真实 TCP、校准、qdisc、每方阶段收发、工作量、五次统计及原始哈希闭合。基线完整离线 OT 材料量和同定义 PRG 计数仍 `NOT_MEASURED`，基线纯论文核心阶段未单独计时；因此完整时间/通信/轮数可同口径并列，不能宣称所有成本指标或泄露边界完全同构。
- **条件性安全**：全两两 key 池的单方视图仍依赖既有 DCF/FSS 多 key 混合归约、独立 keygen 随机币、AES/PRG、可信且不合谋的离线 T、半诚实 P0/P1、完整私有通道假设；旧基线另依赖真实 EMP IKNP OT 的安全假设。源码测试不是这些假设的密码学证明。基线的 party 离线 EMP OT/本地材料与 AAV86 的 T 预发材料生命周期不同，比较解释必须保留此差异。
- **覆盖**：已实测最大 D=128、r=5；`n≥256` 24 个 V3 点仅有 checked 容量和真实 preflight 拒绝，不是性能实测。此阶段未完成 V3 总矩阵、Protocol III+AAV86、物理 LAN/WAN 或作者精确复现。

## 8. 本次 Git 文件差异

E12 的本地提交只包含下列路径；E11 接收报告是原已存在的未跟踪文件，字节未改、仅显式纳入 Git。原始 `TEST_ONLY_E12_RAW/`、旧 E11 JSONL/日志、PDF、`VFSS-baseline/`、参考工程和构建目录均不在差异中。

| 文件 | 用途 |
| --- | --- |
| `.gitignore` | 忽略独立 E12 原始目录。 |
| `VFSS/CMakeLists.txt` | EMP-ON 时构建和注册 TEST_ONLY 全对全基线目标。 |
| `VFSS/include/moe_topk/protocol_i_aav86_small.h` | 增加真实阶段接收和 CA/carrier 时间字段及边界注释。 |
| `VFSS/src/moe_topk/protocol_i_aav86_small.cpp` | 聚合本方 trace 接收；在 CA/载体相邻时间戳处计时并断言守恒。 |
| `VFSS/tests/moe_topk/protocol_i_aav86_small_e2e_test.cpp` | 提前离线起表，单列连接，编码真实字段并加强跨方断言。 |
| `VFSS/tests/moe_topk/protocol_i_e12_baseline_bench_test.cpp` | 隔离的 fresh EMP OT、TCP、同分 oracle、T/party 时间与通信基线 harness；输入 seed 留在控制器。 |
| `docs/BENCHMARK_VALIDATION_PLAN.md` | 固定 E12 时间/total 组成和 E11 历史字段限制。 |
| `docs/IMPLEMENTATION_PLAN.md` | 登记 E11 findings 与 E12 修正范围。 |
| `docs/decisions/M6A_P2_I_E12_MEASUREMENT_CONTRACT_2026-10-03.md` | 决策计量口径与基线限制。 |
| `docs/reviews/M6A_P2_I_E11_CROSS_CHAT_RECEIVER_AUDIT_2026-10-03.md` | 纳入此前未跟踪的接收原件，未改内容。 |
| `experiments/m6a_p2_i_allpairs/TEST_ONLY/e12_run_matrix.py` | AAV86 正式批次与五次统计。 |
| `experiments/m6a_p2_i_allpairs/TEST_ONLY/e12_shaped_namespace.sh` | AAV86 隔离 TCP network namespace。 |
| `experiments/m6a_p2_i_allpairs/TEST_ONLY/e12_audit_results.py` | AAV86 原始逐行独立审计；正式运行后仅修正审计字段读取键。 |
| `experiments/m6a_p2_i_allpairs/TEST_ONLY/e12_run_clique_baseline.py` | 匹配输入的全对全正式批次与统计。 |
| `experiments/m6a_p2_i_allpairs/TEST_ONLY/e12_clique_shaped_namespace.sh` | 基线隔离 TCP network namespace。 |
| `experiments/m6a_p2_i_allpairs/TEST_ONLY/e12_audit_clique_baseline.py` | 基线逐行审计和 AAV86 输入配对。 |
| `docs/reviews/M6A_P2_I_E12_CORRECTED_N128_AND_CLIQUE_BASELINE_2026-10-03.md` | 本报告。 |
