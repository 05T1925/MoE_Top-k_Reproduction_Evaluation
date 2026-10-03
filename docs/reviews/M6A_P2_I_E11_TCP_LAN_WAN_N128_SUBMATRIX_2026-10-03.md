# M6A-P2-I-E11：Protocol I+AAV86 全两两 TCP LAN/WAN 子矩阵

日期：2026-10-03。结论限于 **可信、非合谋、在线静默的离线 T** 为每轮全部 canonical pair 预发材料，P0/P1 在线只消费活跃边的项目实例。`AUTHOR_EXACT=NOT_PROVEN`；这不是完整 V3 或 Protocol III 验收。

## 1. 被测身份与证据类别

起点为隔离 worktree 的 E10 提交 `ac8af47a03031585f1848a6f4b7749d797b9ae93`。原工作区的 [E10 异会话接收](M6A_P2_I_E10_CROSS_CHAT_RECEIVER_AUDIT_2026-10-03.md)当时未跟踪，原件与独立干净 worktree 中的副本 SHA-256 均为 `BBF1BD0C8CA91890DF35164DE2DBBFCBC7A94C3651103D878F765AB6920BC88F`，现已显式纳入本地基础设施提交。六个关键 E10 源码哈希与该接收报告一致；原 E5–E10 CSV/日志未转入新 checkout，也没有将 E10 工程试运行重标为正式数据。恢复用原隔离 worktree 未修改。

新 checkout 为 `codex/m6a-p2-i-e11-benchmark`。基础设施提交 `9a4ee588719292a0a0e579f07ba0c80a4aab140a`；WSL 读取 Windows Git worktree 路径的修正提交 `103b76d863e68c2c318998db01073591e6a8fce8`。**两种网络的全部正式运行当刻 HEAD 均为 `103b76d863e68c2c318998db01073591e6a8fce8`，tracked 状态逐次检查为 `CLEAN`。**运行二进制 SHA-256 为 `07169745C6306F02EA1CC41D8280109B12069C71AF8AB3BA460125B338D4FF40`。协议接口、实现和 E2E 源码 SHA-256 分别为 `D42DAB3F9779295FB53BA557F6E65ED3CA827CC00F07E9395B89B62158E63FED`、`BE52BBCDD5A93A231B06E3879D2AFFA09C9196430C9152A0479B1E9F1593968E`、`67D9292C9EDB3894A891A35A8DF614A54A2C0F63B87E6DCB420EE4F05568DCA1`。每条原始记录另含完整相关源码哈希和二进制哈希。

正式运行**之后**才增加容量拒绝打印、同材料网络 A/B 夹具、独立原始审计和本报告；这些复核文件不改变已测 secure 入口、正式 runner 脚本或上述二进制。最终文档提交 HEAD 因此不能反写成 96 次运行时的 HEAD。

论文定义仅涉及 AAV86 比较图、Agarwal CCS'24 CA 的算法和核心轮数。全两两离线池、统一 D、signed Q20.12、稳定同分、原序 XOR mask、同一置换逆路由是项目扩展；E11 的 TCP harness、公开 pivot 种子覆盖、oracle 和网络模拟均是隔离的 `TEST_ONLY` 行为。密码学安全仍依赖既有条件性 DCF/FSS 混合归约、独立密钥随机币、AES/PRG、可信 T、半诚实 P0/P1 和完整私有通道；运行与测试不能证明这些原语本身安全。

## 2. 网络通道与门禁

E10 的在线通信使用 AF_UNIX `socketpair`。E11 在 **TEST_ONLY harness** 中为两轮 score、`2r+1` 个 CA phase 和一轮 inverse 分别建立真实 `AF_INET/SOCK_STREAM` TCP loopback 连接，随后把文件描述符交给原有 `ProtocolIFramedChannel`；原有安全协议消息、key 格式和 T→party 离线 Unix 分发没有改。TCP 连接在输入分发之前建立，设置 `TCP_NODELAY`；连接建立时间不计入在线主指标。T 独立 fork+exec，仅拥有两份离线 package 描述符和离线 telemetry 描述符，发材退出后 parent 才发送输入；在线 TCP 仅连接 P0/P1。测试 parent 的 oracle 重构不进入 secure 路径或在线时间。

两档均在 Ubuntu 24.04 WSL2 **同一主机** 的独立 Linux network namespace 内执行。`tc netem` 仅配置该 namespace 的 `lo`，实际 P0/P1 TCP 和校准 TCP 都经过该 qdisc；namespace 退出时清理。它们是**模拟 LAN/WAN**，不是两台物理主机的真实链路。校准为 20 次 TCP echo RTT 与 8 MiB TCP 应用负载吞吐，预设门为 LAN RTT 0.5–5 ms、吞吐 200–1200 Mbit/s；WAN RTT 35–70 ms、吞吐 20–110 Mbit/s。超门则 runner 在生成正式材料前停止。

实验结束后 `ip netns list` 无 E11 namespace，宿主默认 `lo` 为 `qdisc noqueue`，未遗留网络整形。

| 标签 | `tc netem` 目标 | 校准 RTT 中位数 | 校准应用吞吐 | 校准后的协议 qdisc 增量 |
|---|---|---:|---:|---:|
| LAN | 单向 0.5 ms、1 Gbit/s | 1.230 ms | 800.230 Mbit/s | 1,825,026 B / 7,393 packets |
| WAN | 单向 25 ms、100 Mbit/s | 50.428 ms | 63.238 Mbit/s | 1,825,026 B / 7,393 packets |

目标带宽是 qdisc 速率上限，实测单流吞吐更低，应以实测值解释延迟。qdisc 的 packet/byte 是内核队列诊断量，含 TCP 控制和封装，**不等于**应用层在线通信量，也不是独立抓包的物理线速字节。校准前、校准后和协议后 `tc -s` 均保留原文。`NETWORK_GATE=GO` 只适用于上述同主机可复跑模拟环境。

独立 frame fixture 对 phase `4/5/9/10/11/40` 使用相同 payload，Unix/TCP 两种 fd 上的双向收发 payload、相位和应用层帧字节逐项相同。TCP peer 已关闭、静默、截断帧分别在 0、119、0 ms 内失败，均小于 1 s 的测试门。另有**隔离的同材料 TEST_ONLY 等价夹具**：仅为网络 A/B 测试，在内存中对 n=8、r=3 的同一份 dealer package 和相同分数份额分别执行 Unix/TCP，使用不同 claim 目录；两方 XOR shares、oracle mask、每条公开边、逐轮 `e_t/v_t`、实际 phase/发送接收字节、DCF Eval、PRG 和 10 轮均逐项相同，`E11_SAME_MATERIAL_TCP_EQ_PASS`。该夹具故意重用材料，不参与安全声明和正式性能。常规完整协议试跑使用冻结输入/公开 pivot 种子但重新随机材料和秘密置换，因而两次的边数/PRG 实数可能不同；两者仍均通过 oracle、恰 K 位和消息 DAG。**正式 96 次运行每次都新生材料**，没有筛选小图种子；前向/逆向仍绑定同一 π。

## 3. 环境、种子与计量边界

环境：[原始环境清单](../../experiments/m6a_p2_i_allpairs/TEST_ONLY_E11_RAW/e11_environment.txt)：Ubuntu 24.04.4 WSL2，Linux 6.6.87.2，GCC/G++ 13.3.0，CMake 3.28.3，Intel i9-13980HX 32 逻辑 CPU，7.6 GiB 内存。Release 编译 `-O3 -DNDEBUG`，`MOE_TOPK_ENABLE_EMP_OT=OFF`；两个在线方各单线程执行此入口，T/P0/P1 为同主机独立 fork+exec。D128 每个子进程在 `prlimit --as=805306368:805306368` 下运行；原有 D128 资源、64 MiB 单方包和内存预检未删除或放宽。构建目录 `/tmp/m6a11-release`，原始记录目录 `experiments/m6a_p2_i_allpairs/TEST_ONLY_E11_RAW/` 被局部 `.gitignore` 排除。

主输入为端点包含的整数均匀分布 `[-32·2^12,32·2^12]`，以 `std::mt19937_64` 和 `std::uniform_int_distribution<int32_t>` 生成后按 signed Q20.12 的 `Z_(2^32)` 原始分数份额输入。每个 `(K,repetition)` 的输入 seed 固定为 `0xE110000+100K+repetition`，跨 r、LAN/WAN 共用输入，原始行记录 seed 和输入摘要。公开 pivot seed 固定为 `0xA110000+1000r+repetition`，同 r/repetition 的 K 与网络成组对应；仅 TEST_ONLY dealer 在重新随机生成秘密材料后覆盖**公开且与 key 无关**的 pivot seed。秘密 π、mask、DCF key 和加法份额每次由系统随机性新生，随机币不写入日志。每配置 1 次预热、正式 5 次；预热不计统计。每次拥有独立 session/material ID、T 生成、序列化、分发和持久 claim。

离线包围时间从 parent 等待 T 到 T 已退出且 P0/P1 均完成包反序列化 ready 屏障，含启动、生成、序列化、分发和接收；T 的三段单独计时，`receive_barrier_ms` 从 T 退出到双方 ready，余下启动/调度残差不冒充任何一段。在线完整时间取同主机单调时钟下 P0/P1 **各自 secure 调用耗时的较大值**，记录两方原值；调用始于 party 接收输入份额之后，含持久 claim、score、CA、inverse，排除测试输入发送、oracle 和结果汇报。score/core/inverse 三段在 secure 入口内计时，三段之和可小于完整在线时间（入口绑定和 claim 开销）；每段两方原值均保存。`total_time_ms=offline_time_ms+online_time_ms`。不把两方耗时相加。

通信是 framed channel 实际应用层发送/接收字节，每方 score/core/inverse 发送和、接收和及双方交叉对账均逐次核验；`online_comm_total_bits=8·(P0_sent+P1_sent)`，接收不再相加。因果 DAG 为两轮 score、`2r+1` 轮 CA、一轮 inverse，**仅此 raw-score→原序 XOR mask 入口**为 `2r+4`；测试检查实际 message trace 的 phase 顺序，不靠预填数字单独作证。每轮 `e_t` 是去重无序边数、`v_t` 是该轮边端点去重数，跨轮求和分别为 `e_A/v_A`；party 测试编码再次从公开 edge trace 复算二者。PRG 仅计此入口传统 DCF 的 seed→两块长度倍增调用，score/每轮 CA/逆路由分列；其他 AES/PRG 细分为 `NOT_MEASURED`，不将 DCF Eval 或比较边误称 PRG。

## 4. n=128 正式可运行子矩阵

下表每行是**五次正式运行**；时间为中位 `[最小,最大]` 毫秒，边/节点与通信列为五次中位。完整逐次字段、各项统计的 median/min/max、失败记录和逐次日志见原始索引。每次原序 mask 对冻结 oracle PASS、恰 K 位、T/P0/P1 退出 0。

| 网络 | K | r | 在线 ms | 离线 ms | e_A | v_A | 在线总 bits |
|---|---:|---:|---:|---:|---:|---:|---:|
| LAN | 2 | 2 | 22.31 [21.57,25.20] | 171.08 [161.53,181.44] | 2279 | 244 | 169984 |
| LAN | 2 | 3 | 21.89 [20.57,54.14] | 268.98 [236.97,277.18] | 1606 | 348 | 204288 |
| LAN | 2 | 4 | 24.11 [22.73,43.93] | 380.00 [357.36,386.86] | 1286 | 447 | 238592 |
| LAN | 2 | 5 | 24.80 [23.93,26.65] | 440.92 [428.39,444.77] | 1229 | 546 | 272896 |
| LAN | 8 | 2 | 21.33 [20.37,22.55] | 189.21 [188.95,197.13] | 2200 | 244 | 169984 |
| LAN | 8 | 3 | 21.18 [20.85,22.44] | 258.95 [228.82,282.56] | 1599 | 348 | 204288 |
| LAN | 8 | 4 | 23.89 [22.21,24.33] | 391.54 [355.09,449.54] | 1343 | 448 | 238592 |
| LAN | 8 | 5 | 24.17 [22.85,25.81] | 472.01 [464.37,563.22] | 1150 | 541 | 272896 |
| WAN | 2 | 2 | 422.65 [416.13,428.73] | 197.74 [183.66,228.88] | 2537 | 245 | 169984 |
| WAN | 2 | 3 | 517.15 [512.45,521.92] | 271.58 [266.11,284.76] | 1440 | 349 | 204288 |
| WAN | 2 | 4 | 616.64 [613.08,619.15] | 388.24 [374.38,413.06] | 1261 | 448 | 238592 |
| WAN | 2 | 5 | 716.34 [712.09,721.55] | 491.95 [451.41,509.78] | 1174 | 548 | 272896 |
| WAN | 8 | 2 | 421.25 [417.58,425.96] | 199.03 [191.01,203.90] | 2267 | 245 | 169984 |
| WAN | 8 | 3 | 517.79 [512.22,548.52] | 276.33 [273.46,289.49] | 1763 | 348 | 204288 |
| WAN | 8 | 4 | 617.14 [612.84,625.43] | 377.53 [360.76,386.82] | 1238 | 445 | 238592 |
| WAN | 8 | 5 | 716.64 [715.70,735.81] | 467.55 [460.71,498.74] | 1184 | 545 | 272896 |

LAN 个别运行的最大值高于中位（如 K2/r3 54.14 ms），原值保留，未按最快值删样本。WAN 在线时间随 `2r+4` 因果轮增加；这仅是当前模拟条件下的观察，不外推到其他 RTT、主机或 n。全两两预留离线材料与活跃比较边分列；例如 D128/r5 每方预留 40,640 槽和 42,547,506 B 包，在线 e_A 以各次真实图计，绝不把后者代替预留量。

## 5. V3 容量与同路线基线

[32 行逐点状态](../../experiments/m6a_p2_i_allpairs/TEST_ONLY_E11_RAW/e11_v3_status.csv)由当前 checked C++ 容量输出和**实际调用的 preflight** 合并而成：n128、K2/8、r2..5 的 8 点均为两网络 `FORMAL_PASS_5_PER_NETWORK`；n256、K2/8 的 8 点及 n=10³/10⁴/10⁵/10⁶、K80 的 16 点均为 `PRECHECK_REJECTED_NO_KEYGEN`。对这 20 种大 n/r 形状，当前 preflight 第一拒绝原因均为 `HARD_CAP_D_GT_128`，assessment 另列 64 MiB 包、预算和内存标志；n256/r2 每方包的解析值 69,999,474 B 已超过 67,108,864 B 限制。没有扩大上限、生成被拒绝全池、缩小 n、改固定 M 或在线补料。预测字段、实际 preflight 和运行状态分列。

同路线全对全入口为 `protocol_i_raw_score_input_party` 加 `protocol_i_priority_pipeline_party`，接受相同 32 位分数份额并输出原序 XOR mask；其独立进程样例在 `VFSS/tests/moe_topk/protocol_i_small_e2e_test.cpp`。当前 Release 构建使用 `MOE_TOPK_ENABLE_EMP_OT=OFF`，该样例目标只在 `ON` 时编译；样例还用固定 FSS 种子和 AF_UNIX socketpair，不具备本任务的 fresh 随机材料、同一输入分布、TCP LAN/WAN、时间和 PRG 完整计数。因此**本次没有可比的全对全数值**，没有用 AAV86 核心时间对照旧端到端 Debug 数字。后续需为全对全路径补同一 Release 构建条件、fresh T 材料、TCP runner 和相同指标边界后再做匹配比较。

## 6. 命令、原始索引与结果核验

Release 构建：

```bash
cmake -S /mnt/c/Users/28641/Desktop/MoE_Top-k_Reproduction_Evaluation__m6a_p2_i_e11_benchmark/VFSS -B /tmp/m6a11-release -DCMAKE_BUILD_TYPE=Release -DMOE_TOPK_ENABLE_EMP_OT=OFF
cmake --build /tmp/m6a11-release -j8 --target moe_topk_m6a7_aav86_small_e2e_test moe_topk_m6a7_aav86_small_conformance_test moe_topk_m6a7_aav86_small_differential_test moe_topk_m6a10_aav86_metrics_conformance_test
ulimit -v 786432
MOE_TOPK_M6A_E9_D=128 /tmp/m6a11-release/moe_topk_m6a7_aav86_small_conformance_test
MOE_TOPK_M6A_E9_D=128 /tmp/m6a11-release/moe_topk_m6a7_aav86_small_differential_test
MOE_TOPK_M6A_E9_D=128 /tmp/m6a11-release/moe_topk_m6a7_aav86_small_e2e_test
/tmp/m6a11-release/moe_topk_m6a7_aav86_small_e2e_test transport-conformance
/tmp/m6a11-release/moe_topk_m6a7_aav86_small_e2e_test transport-negative
/tmp/m6a11-release/moe_topk_m6a11_tcp_material_equivalence_test
ctest --test-dir /tmp/m6a11-release --output-on-failure -j4 -R 'moe_topk_(m6a11|m6a10|m6a7|m2_|m3_|m5_fix_f1)'
```

同一 Ubuntu-24.04 WSL2 以 root 为**网络命名空间设置**执行下面两个命令；脚本立即降为 `moeaudit` 身份并以 `prlimit` 限制校准与 runner，退出时清理 namespace。两个配置在测量前已固定、校准、过门；输出目录不得覆盖旧批次。

```bash
bash experiments/m6a_p2_i_allpairs/TEST_ONLY/e11_shaped_namespace.sh LAN /tmp/m6a11-release/moe_topk_m6a7_aav86_small_e2e_test experiments/m6a_p2_i_allpairs/TEST_ONLY_E11_RAW "$PWD"
bash experiments/m6a_p2_i_allpairs/TEST_ONLY/e11_shaped_namespace.sh WAN /tmp/m6a11-release/moe_topk_m6a7_aav86_small_e2e_test experiments/m6a_p2_i_allpairs/TEST_ONLY_E11_RAW "$PWD"
python3 experiments/m6a_p2_i_allpairs/TEST_ONLY/e11_audit_results.py --raw-dir experiments/m6a_p2_i_allpairs/TEST_ONLY_E11_RAW --head 103b76d863e68c2c318998db01073591e6a8fce8
python3 experiments/m6a_p2_i_allpairs/TEST_ONLY/e11_capacity_status.py --preflight-log experiments/m6a_p2_i_allpairs/TEST_ONLY_E11_RAW/e11_capacity_preflight.log --output experiments/m6a_p2_i_allpairs/TEST_ONLY_E11_RAW/e11_v3_status.csv
```

正式 runner 依次输出 `LAN_runs.jsonl` / `WAN_runs.jsonl`（各 48 行，含预热标志、逐次命令/退出/时长/峰值/阶段字节/e_t/v_t/DCF/PRG/轮数/日志哈希），再输出 `LAN_summary.json` / `WAN_summary.json`（各 8 配置所有数值指标的 median/min/max）。独立审计 `E11_RAW_AUDIT_PASS 96 runs, 80 formal, 16 warmup, 16 summaries`；容量/实际 preflight 为 24/20 行，状态 CSV 32 行。原始 SHA-256：

| 忽略目录内文件 | SHA-256 |
|---|---|
| `LAN_runs.jsonl` | `2DD8E9036F34553308E11A711B5468966458DC44E5E66C39046E8094031E2522` |
| `WAN_runs.jsonl` | `B35B3DCB849980420060B8F2F02F57170CF9A94CC33DB5155712A67DA7AAD056` |
| `LAN_summary.json` | `83EB6310502E2526620A0CDC631722DE80C2CB626D060AED960B3E68ABB3AA78` |
| `WAN_summary.json` | `346C2D67024C5943D5CF2D5B2EC1EAF3FE3212B704B26B7A831F38C8001B2C26` |
| `LAN_calibration.json` | `9287D02358F6275E2FC0B6712A30C37FA6058A0695BC9C1F314B70C6C409FD38` |
| `WAN_calibration.json` | `95B3B7E6209AF6EA5D104973EC3450D2A0A4FE26F538BC566153C0F269F1E84F` |
| `e11_audit.log` | `BEFF599229823A54CA4C121D357DEC701F05C7BE06E891B7E5A5666C8CAFA3F5` |
| `e11_capacity_preflight.log` | `D2939EA77A7E0BA55F157038BF610617EBFB23B6A6C819F295B68416A26B72C4` |
| `e11_v3_status.csv` | `49127F82E321B8DF42317D2209CEE711052D0C056B99D491DB9824F516EC169E` |
| `e11_same_material_transport_equivalence.log` | `7DC5F0ACB4B4997BBBCC03F3F3151C2FDAB20498423A6B654AD5AC39C2C1E5C6` |
| `e11_final_related_ctest_27.log` | `961AB9F879F069F597CFD37B90713C1270259BE49DBA0BE485BE244D54E90283` |
| `e11_environment.txt` | `C28219E3203DC46ABE7B52AD08256AC7335AE157AD586544ED60077B403B1D93` |
| `LAN_qdisc_after_calibration.txt` / `LAN_qdisc_after.txt` | `873298DA89993574A14BE17AD2F1C2A8DDFE2D937E573412CA2081222E4C21CF` / `F0CE85D2C316ABEFC2AEF243C065E61287B0E711128762BC90C170DE314E44E1` |
| `WAN_qdisc_after_calibration.txt` / `WAN_qdisc_after.txt` | `A1A63327FE261FDFFA1DD42FC8069E5B8962544BCB94B38118C0AA8496891C3D` / `EB7467CB821DDE296B5049F023FB1A0809DBDC89E67E580B0BEF88B44129D8F1` |

第一次 LAN 尝试在网络校准后被 Windows `.git` 路径的 WSL Git 解析问题阻断，**没有协议测量行**；校准和 qdisc 记录保留于 `TEST_ONLY_E11_RAW/failed_windows_gitdir_attempt/`，修正后从新提交完整重新运行，没有接续、挑选或覆盖失败批次。完整 Release `all` 构建曾在与本路径无关的 `ext/bitpack/bitpack_test` 链接阶段失败（`bitpack::mod` 未定义）；本任务所需目标单独编译并通过上述验证，未将全量构建失败写成全量 PASS。

最终源码顺序验证为 D128 conformance 6/6、冻结 oracle differential 6/6、T/P0/P1 独立进程 E2E 6/6，然后相关旧路径回归 CTest **27/27 PASS**（含 E11 同材料 TCP 等价夹具）；Git 差异与空白另行核对。

## 7. 判定与未完成项

| 门 | E11 结果 |
|---|---|
| TCP 传输与模拟网络 | GO：真实 P0/P1 TCP、namespace qdisc 校准、协议 qdisc 增量和有界失败验证 |
| n128 Protocol I+AAV86 子矩阵 | GO：16 网络×配置单元，各 1 预热+5 正式；80/80 正式正确，原始与五次汇总保留 |
| 更大 n 的全两两运行 | PRECHECK_REJECTED：当前 D>128 硬门先触发，n256/r2 包本身也超 64 MiB |
| 同路线全对全数值对照 | NOT_MEASURED：当前基线构建/随机性/网络/计量边界未对齐 |
| V3 总验收 | NOT_DONE：另一 AAV86 路线、较大 n 成功运行和异会话接收均未完成 |

E11 新代码、模拟网络记录和结果需下一次异会话独立接收。未提交原始日志/CSV、密钥、掩码、论文、构建物、参考工程或 `VFSS-baseline/`；未推送、合并或创建 PR。
