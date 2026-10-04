# M6A-P2-I-E14：Protocol I 两路线统一性能数据包与有界收口

日期：2026-10-03。范围：Protocol I+AAV86 全两两项目实例与同路线 EMP-ON 全对全工程基线。Protocol III+AAV86、固定 M、在线补料、物理双机网络和作者精确复现均未在本阶段实施。`AUTHOR_EXACT=NOT_PROVEN`。

## 1. 起点、隔离和版本

- E12 正式被测源码 `8ad0725c73716ac37c96d417136958f932233d61`、事后报告 `c86f5bd50a938be8550d41ca99712d5859f9a1ae` 保持区分。E12 异会话数据接收为 `PASS_WITH_FINDINGS`；其 151 项索引 SHA-256 `110C445F288539FF0AB5E4FB78451FAEF504C58ED164F33B37CC3EA1AFC12C88`。E14 开始前，E12 `accepted/` 与仓库外副本各 152 文件逐 SHA-256 相同，未改写。
- 隔离 worktree 为 `C:/Users/28641/Desktop/MoE_Top-k_Reproduction_Evaluation__m6a_p2_i_e11_benchmark`。开工时 HEAD `c86f5bd...`，merge-base `main` 为 `c3926c68fd14f270faa8b55234311071947fa080`；E13 的三份已有修改、两份未跟踪接收/交接文档及审计脚本经显式暂存和 `git diff --cached --check` 后，保存本地文档检查点 `2b48ef6c9ae3d4a25e0812a08a85c1d6d6b2dd56`。新分支 `codex/m6a-p2-i-e14-metrics`。
- E14 第一代码检查点 `e3d4ad36580b14fe6682d6108f17859fb4b627fb` 建立材料与 PRG 观测点。独立审计发现基线 JSONL 少三项角色退出码，初版 `TEST_ONLY_E14_RAW/accepted/` 全部保留但**作废、不纳入本报告统计**。修正 runner 后的正式运行 HEAD 为 **`b0e474cfbbcae433405ac61c5240068878ba43ca`**。`final/` 是唯一用于下表的 E14 批次；E11/E12/初版 E14 不混用。
- 没有修改 `VFSS/src/` secure runtime、DCF/FSS 实现、旧 Protocol I/III 对外接口、`VFSS-baseline/`、`Papers/`、本地参考树或主工作区；没有提交密钥、PDF、日志或构建物。

## 2. 指标来源与两个缺口的关闭方式

[E14 指标来源决策](../decisions/M6A_P2_I_E14_UNIFIED_METRIC_PROVENANCE_2026-10-03.md)先于 TEST_ONLY 代码修改写成，逐项列出观测点与初始缺口。本次统一列为 `offline_time_ms`、`offline_material_total_bits`、`online_time_ms`、`total_time_ms`、`online_comm_total_bits`、`online_comm_per_party_bits`、每方发送/接收、`online_rounds`、`online_prg_calls_total`、`comparison_edges_total`。后续六方案可以直接读取忽略目录的 `e14_unified_protocol_i.csv`：20 组，每组五次正式运行的 median/min/max，另有 route、K、r、HEAD、网络标签与预热次数。

| 计量对象 | AAV86 | EMP-ON 全对全 | 状态与边界 |
| --- | --- | --- | --- |
| 离线时间 | T/P0/P1 启动前至 T 退出且两方 ready | 同左；双方离线 EMP OT 在屏障内 | **MEASURED**；TCP 建连另列，不含测试输入发送、oracle、报告收集 |
| 在线时间 | score + CA/carrier + inverse 的各方 secure 入口原值，主值取两方较大者 | score + forward/CmpAgg/reveal/reverse 的各方 secure 入口原值 | 各方 **MEASURED**，主值 **DERIVED**；非双机时钟相减 |
| 总时间 | 每次 offline + online，再取五次统计 | 同左 | **DERIVED**；不能把两个中位数相加当总时间中位数 |
| 在线持有材料 | 每方四组置换、forward/inverse words、每轮节点 mask、两阶段 score mask/DCF、全部预留边 DCF key 的实际有效载荷 | 每方 T package 的节点 mask、score mask/DCF、全对全边 DCF；另加本地双向 shuffle 的 own/own-inverse permutation、PO permutation/layers/delta、DO a/b/w | 实际 vector 逐项遍历、固定宽度规范编码和实际 key `k/g/v` 序列化有效载荷；两方求和 **MEASURED/DERIVED**。容器 capacity、C++ padding、包/DCF 标签和已销毁的 OT 中间状态不入主字段 |
| T package 与离线 OT | package 实际序列化/分发字节另列；无 EMP OT | T package 实际字节、party 本地材料、EMP OT `IOChannel` 发送/接收各自另列 | **MEASURED**；package 文件长不冒充全部材料。离线 OT `IOChannel` 计数不含四次/方、每次 48 B 的自定义 preamble；两方发送总量若需含 preamble，须再加 384 B，标 DERIVED |
| 在线通信 | 本方 score/core/inverse message trace | 本方 score/forward/cmpagg/reveal/reverse 计数 | 应用层字节 **MEASURED**；总 bits 只加双方发送，接收用于阶段及跨方守恒；TCP/IP 报文未计 |
| PRG | score 和各轮 CA 的 `evalDCF→traverseOneDCF`，本方 thread-local 在 secure 入口重置；inverse 为 0 | score 与 CmpAgg 的同一 DCF 扩展点，本方在两阶段前后读取 | 现有两条在线完整入口的 **seed→双子 seed 长度倍增**调用均 **MEASURED**，两方求和 DERIVED；不是 DCF Eval、AES 总调用或边数。公开 pivot 使用独立流式随机数；其他 AES/PRG 细分 `NOT_MEASURED` |
| 边、槽与轮数 | 每轮活跃无序边 `e_t`、端点去重 `v_t` 和跨轮和；每方 `r·C(D,2)` 预留槽；完整入口 `2r+4` | 实际 `C(D,2)` 无序边；完整工程入口 8 轮 | 图工作量 **MEASURED**；槽数、因果轮为 checked 形状/执行 trace **DERIVED/核验**。双方 Eval 不重复计边 |

材料小实例的独立手算：n=2 全对全每方 T 有效载荷 `4×(16+840)+840+16=4,280 B`，本地双向 shuffle 留存 `432 B`；运行时 score/pipeline DCF 长度倍增分别 `272/68` 次。AAV86 的 D=2、r=1 每方有效载荷 `4,376 B`。两条测试入口均断言这些值，n=128 独立进程试跑又核对每方发送/接收、材料、DCF/PRG 和原序恰 K 位 mask。Baseline n=128 每方 T 有效载荷 `8,218,112 B`、本地 shuffle `261,120 B`，统一两方材料 `135,667,712 bits`；每方 package 文件 `8,696,624 B`，两者单位与含义不同。每方离线 EMP OT 内部发送 `201,920 B`，与对方内部接收相等，未计入材料主字段。

## 3. 正式运行合同、身份及原始保存

- 同主机 WSL2 Ubuntu 24.04.4，Linux 6.6.87.2；Intel i9-13980HX、32 逻辑 CPU；WSL 可见内存 7.6 GiB。GCC 13.3.0，CMake 3.28.3，Release `-O3 -DNDEBUG`，`MOE_TOPK_ENABLE_EMP_OT=ON`，emp-tool/emp-ot 前缀 `/tmp/moe_m28_emp.ok9WzQ/prefix`，OpenSSL 由构建链接；每方单线程。D128 运行保留 768 MiB `RLIMIT_AS` 与预检。
- 二进制 SHA-256：AAV86 E2E `E0B8807B1AD9AA47BFAE6272AD2AA02BE375027483736590CC19CB4E9696E3C3`；EMP-ON baseline `CA0F355DE77705EEE1F004B427872D1A6034ABE7D44CB011AFBF0A475C3CA0B2`。关键 TEST_ONLY 源码 SHA-256 分别为 AAV86 `1A4A3C2BD9FA460CF6FE336068B9B5C353137A556F5E81011E6EC7270F2856D1`、baseline `A114DA38336079D4E8E4B2CF7085BF51BCAAF11511FF44AA90D6770B87F08FE9`、共享材料观测头 `8CF07DD88F3448AFAE39AF889951C820C82A045B938E1494DBEFEBD3546AE1CA`。每行另存完整相关源码哈希映射、运行时 HEAD、tracked 状态、二进制哈希及原始日志哈希。
- 输入为端点包含的均匀整数 `[-32·2^12,32·2^12]`。`final/input_plan.json` 用 OS 熵生成 12 个不同 64-bit seed，SHA-256 `B239A26B07CA6C36CFA7E2CFECB601B4A5495165E790A55DA7A218DCB4DE77BD`；同 K/重复编号的两路线与 AAV86 四个 r 读取同一 seed，T 的 serial/公开参数由另一日程构造且不接收 seed。AAV86 的 TEST_ONLY 加法份额改用 `getrandom`，baseline 已用 OS 熵；每次 fresh T 材料和 ID。计划与合成输入在实验记录中可复现，**不是输入隐私实验**；secure 入口未重构 score、rank 或 mask。
- LAN/WAN 都是单 WSL2 主机、独立 network namespace 的 TCP loopback，qdisc 实际作用于所有在线 P0/P1 TCP fd；T 离线分发，在线无输入/rank/output 通道。目标 LAN 1 ms/1000 Mbit/s、WAN 50 ms/100 Mbit/s。本次校准 RTT 中位数与应用层吞吐分别为 AAV86 LAN `1.121 ms / 840.182 Mbit/s`、WAN `50.446 ms / 62.281 Mbit/s`，baseline LAN `1.255 ms / 829.737 Mbit/s`、WAN `50.384 ms / 62.141 Mbit/s`。逐次校准与 qdisc 前/校准后/协议后原文均在原始目录；不称物理双机 LAN/WAN。
- 每网络 AAV86 8 配置×(1 预热+5 正式)=48 次，baseline 2 配置×(1+5)=12 次。两网络共 **120 次、100 次正式，全部正确**；预热与失败行另存、不入 median/min/max。AAV86 实际每轮 `e_t/v_t`、score/CA DCF、逐轮 PRG、T/P0/P1 峰值、阶段时间/字节和材料槽均在逐次行。baseline 无 r，对相同 K/repetition 的一组 fresh baseline 同时匹配四个 r。

最终原始目录：`C:/Users/28641/Desktop/MoE_Top-k_Reproduction_Evaluation__m6a_p2_i_e11_benchmark/experiments/m6a_p2_i_allpairs/TEST_ONLY_E14_RAW/final/`（Git 忽略）。仓库外完整副本：`C:/Users/28641/Desktop/MoE_Top-k_Reproduction_Evaluation_Evidence/M6A/E14_final_complete_b0e474c/`。两处各 **152 文件，逐文件 SHA-256 0 差异**；`e14_raw_complete_index.sha256` 收录 151 项，索引自身 SHA-256 `7363D64EA6794C49DA563D39FB02DED528E2FABB3C211BC295EFC092037ED796`。`sha256sum -c` 151/151 通过。初版 E14 `accepted/` 和其早期副本保留为历史失败批次，不用于数据包。

| 原始索引项 | SHA-256 |
| --- | --- |
| `aav86/LAN_runs.jsonl` | `115E43A88AB6021789167462255FD20B3E5FE8876DC6AFE490D3B1FA3FE8F415` |
| `aav86/WAN_runs.jsonl` | `44C0C496E4A65E48FC31A9158FDD9DC59B6007AA4FA3372361D4D7AD24236ED2` |
| `baseline/LAN_baseline_runs.jsonl` | `72D91A5A70AE3761F4DB60BC434138B0D9A03956B9D720A29A1B4ED4621883B2` |
| `baseline/WAN_baseline_runs.jsonl` | `6898EDFB9CAC93FE17D7B5170EC8989D0C0061C8C4A30B369B417CA9EADB4642` |
| `e14_unified_protocol_i.csv`（20 组、9 主指标三统计） | `ECD8DC82495A841AE5B31A560F1BB63E96384A0EAB4548369710374054EE0E01` |
| `e14_v3_capacity_status.csv`（32 配置行，含 24 个 n≥256 拒绝点） | `49127F82E321B8DF42317D2209CEE711052D0C056B99D491DB9824F516EC169E` |
| `e14_capacity_preflight.log` | `D2939EA77A7E0BA55F157038BF610617EBFB23B6A6C819F295B68416A26B72C4` |
| `e14_audit_result.json` | `8FCBDBF0BE17835FB4D472C3A96DF293A97D8CF34D519665EE4F2CC0DD85CA07` |

## 4. n=128 完整入口结果

下表每行的在线/总时间是五次正式值的 **median [min,max]**，离线列为五次中位数；单位 ms。总时间先逐次 offline+online 再统计。所有其余主字段的 median/min/max 在 `e14_unified_protocol_i.csv`，逐次值在四份 JSONL。AAV86 的 CA 局部时间保留在逐次记录，不与 baseline 完整入口做交叉比值。

| 网络 | 路线 | K | r | 离线中位 | 在线中位 [min,max] | 总中位 [min,max] |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| LAN | AAV86 | 2 | 2 | 202.6 | 22.0 [20.2,24.1] | 223.4 [222.8,261.9] |
| LAN | AAV86 | 2 | 3 | 294.3 | 21.1 [20.4,34.0] | 318.5 [305.2,322.6] |
| LAN | AAV86 | 2 | 4 | 399.8 | 25.6 [23.6,33.7] | 425.4 [394.0,439.4] |
| LAN | AAV86 | 2 | 5 | 480.1 | 23.8 [22.3,25.8] | 503.6 [496.3,509.5] |
| LAN | AAV86 | 8 | 2 | 195.3 | 21.6 [21.2,23.0] | 216.8 [203.6,220.3] |
| LAN | AAV86 | 8 | 3 | 262.2 | 20.6 [20.0,21.7] | 282.2 [279.3,296.1] |
| LAN | AAV86 | 8 | 4 | 385.8 | 22.8 [21.9,25.2] | 409.6 [395.7,415.1] |
| LAN | AAV86 | 8 | 5 | 490.5 | 24.1 [23.4,24.9] | 513.9 [465.9,521.3] |
| LAN | 全对全 | 2 | — | 163.5 | 43.2 [42.1,45.3] | 208.6 [197.4,214.6] |
| LAN | 全对全 | 8 | — | 164.8 | 43.3 [41.8,44.5] | 208.7 [206.3,213.3] |
| WAN | AAV86 | 2 | 2 | 191.9 | 424.8 [420.7,432.8] | 614.8 [603.7,633.8] |
| WAN | AAV86 | 2 | 3 | 260.5 | 527.2 [525.9,530.0] | 787.8 [782.7,798.3] |
| WAN | AAV86 | 2 | 4 | 376.8 | 624.2 [622.9,625.8] | 1002.6 [990.1,1034.1] |
| WAN | AAV86 | 2 | 5 | 523.7 | 723.1 [721.0,727.7] | 1244.7 [1173.9,1260.5] |
| WAN | AAV86 | 8 | 2 | 195.8 | 424.7 [417.4,434.6] | 620.5 [596.6,644.1] |
| WAN | AAV86 | 8 | 3 | 274.9 | 519.8 [518.2,526.9] | 794.7 [779.5,854.9] |
| WAN | AAV86 | 8 | 4 | 386.5 | 623.3 [617.6,630.1] | 1009.7 [996.2,1058.4] |
| WAN | AAV86 | 8 | 5 | 521.6 | 723.3 [721.0,728.2] | 1249.8 [1153.2,1276.7] |
| WAN | 全对全 | 2 | — | 168.7 | 501.2 [500.6,503.1] | 671.4 [659.7,677.9] |
| WAN | 全对全 | 8 | — | 165.8 | 501.1 [500.5,507.3] | 666.9 [656.1,677.3] |

共同工作量：全对全的每方预留/消费 `8,128` 无序边、DCF Eval `16,768`、两方长度倍增 PRG `1,335,296`、在线应用层总通信 `1,479,424 bits`、8 因果轮。AAV86 r=2/3/4/5 每方预留分别 `16,256/24,384/32,512/40,640` 槽；两方在线通信分别 `169,984/204,288/238,592/272,896 bits`，完整入口轮数 `8/10/12/14`。两方在线材料有效载荷分别 `259,571,712/387,555,328/515,538,944/643,522,560 bits`，全对全为 `135,667,712 bits`。AAV86 活跃 `e_A/v_A` 与 PRG 随固定公开图种子和实际运行而变，所有五次的中位/min/max 见 CSV；不把预留槽当消费边。

这些数据说明此 n=128 环境里 AAV86 的在线通信较少，LAN 在线时间约 20.6–25.6 ms（全对全约 43.2–43.3 ms）；同时全两两材料与离线时间随 r 增长。WAN 的 r=2 总时间中位数低于匹配全对全，r≥3 则高于全对全；LAN 全部 r 的总时间中位数高于匹配全对全。只用同定义、同入口的实测项比较；不把小 D 外推到 n=10^5/10^6。

## 5. 容量、限制与证据等级

`e14_v3_capacity_status.csv` 有 32 个按 K 展开的配置行：n=128 的 8 行各为 LAN/WAN 5/5 正式 PASS；n=256、10^3、10^4、10^5、10^6 的 24 行均为 `PRECHECK_REJECTED_NO_KEYGEN`，实际首个理由 `HARD_CAP_D_GT_128`。checked 容量/预检程序在 768 MiB 进程限制下重跑，24 个大点未 keygen。n=256、r=2 每方解析包 `69,999,474 B > 64 MiB`，硬上限之外另触及包限制；更大点的槽数、包字节及预算/资源布尔值均在 CSV 原始行。最大实际成功 `D=128,r=5`；最大仅完成 checked 形状计算的 `D=1,048,576`。本次没有放宽 64 MiB、改为固定 M、在线 T 或稀疏预发。

| 分项门 | E14 判定 | 证据边界 |
| --- | --- | --- |
| Protocol I+AAV86 有界功能 | **PASS_WITH_FINDINGS** | D≤128 的既有 secure 实现保持不变；新 TEST_ONLY 观测点的 conformance、oracle differential、独立进程 E2E 与旧路径回归通过。新计量源码待下次异会话接收。 |
| n=128 两条路线九项统一指标 | **PASS_WITH_FINDINGS** | 20 组、100 次正式全部正确，material/PRG 观测闭合并有手算与运行时对账；同主机模拟网络与离线 OT preamble 范围须随数据携带。 |
| n≥256 资源限制记录 | **PASS（预检证据）** | 24/24 checked 形状加真实 preflight 拒绝，无性能测量。 |
| Protocol I 路线性能收口 | **有界 n=128 数据包就绪，待异会话接收** | 不能称完整 V3 规模覆盖或生产环境性能；全两两 `O(rD²)` 预留继续限制扩展。 |
| M6A/V3 总验收 | **NOT_COMPLETE** | Protocol III+AAV86 未实现与计量；n≥256 未运行；作者精确复现未证明。 |

条件性安全论证继续依赖 DCF/FSS 选阈值隐私及整池多 key 混合归约、独立 keygen 随机币、PRG/AES 安全、可信不合谋且在线静默 T、单方半诚实 P0/P1 和私有完整通道。公开 masked vectors、局部 rank、pivot/bucket/图、访问顺序、消息长度与 abort 时点沿既有项目允许泄露边界；TEST_ONLY 计数或成功测试不证明密码学安全。Protocol I 的 `2r+1` CA 核心和 `2r+4` raw-score→原序 mask 完整入口轮数只适用于本实现，不转写 Protocol III。

## 6. 复跑命令、检查与下一步交接

```text
cmake -S <隔离 worktree>/VFSS -B /tmp/m6a14-emp-release -DCMAKE_BUILD_TYPE=Release -DMOE_TOPK_ENABLE_EMP_OT=ON -DCMAKE_PREFIX_PATH=/tmp/moe_m28_emp.ok9WzQ/prefix
cmake --build /tmp/m6a14-emp-release --target moe_topk_m6a7_aav86_small_e2e_test moe_topk_m6a12_protocol_i_clique_benchmark_test -j 4
ctest --test-dir /tmp/m6a14-emp-release -R 'moe_topk_m6a7_aav86_small_conformance_test|moe_topk_m6a10_aav86_metrics_conformance_test' --output-on-failure
ctest --test-dir /tmp/m6a14-emp-release -R 'moe_topk_m6a7_aav86_small_differential_test' --output-on-failure
ctest --test-dir /tmp/m6a14-emp-release -R 'moe_topk_m6a7_aav86_small_e2e_test|moe_topk_m6a12_protocol_i_clique_benchmark_test' --output-on-failure
python experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_input_plan.py --output experiments/m6a_p2_i_allpairs/TEST_ONLY_E14_RAW/<new>/input_plan.json
# root in Ubuntu-24.04 WSL2; full absolute paths required:
bash experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_shaped_namespace.sh LAN|WAN <AAV86 binary> <new>/aav86 <source root> <new>/input_plan.json
bash experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_clique_shaped_namespace.sh LAN|WAN <baseline binary> <new>/baseline <source root> <new>/aav86/<profile>_runs.jsonl <new>/input_plan.json
python experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_audit_results.py --raw-root <new> --head <frozen HEAD>
python experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_export_unified.py --raw-root <new> --output <new>/e14_unified_protocol_i.csv
git diff --check
```

上述矩阵实际命令中的根目录与参数如下，均在 Ubuntu-24.04 WSL2 中执行；两个 wrapper 以 root 建立临时 network namespace，内部切到 `moeaudit` 并设置 `RLIMIT_AS`。原始目录已存在时 runner 拒绝覆盖，因此复跑须给新的被忽略目录与输入计划。

```bash
ROOT=/mnt/c/Users/28641/Desktop/MoE_Top-k_Reproduction_Evaluation__m6a_p2_i_e11_benchmark
RAW=$ROOT/experiments/m6a_p2_i_allpairs/TEST_ONLY_E14_RAW/final
AAV=/tmp/m6a14-emp-release/moe_topk_m6a7_aav86_small_e2e_test
BASE=/tmp/m6a14-emp-release/moe_topk_m6a12_protocol_i_clique_benchmark_test
bash "$ROOT/experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_shaped_namespace.sh" LAN "$AAV" "$RAW/aav86" "$ROOT" "$RAW/input_plan.json"
bash "$ROOT/experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_shaped_namespace.sh" WAN "$AAV" "$RAW/aav86" "$ROOT" "$RAW/input_plan.json"
bash "$ROOT/experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_clique_shaped_namespace.sh" LAN "$BASE" "$RAW/baseline" "$ROOT" "$RAW/aav86/LAN_runs.jsonl" "$RAW/input_plan.json"
bash "$ROOT/experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_clique_shaped_namespace.sh" WAN "$BASE" "$RAW/baseline" "$ROOT" "$RAW/aav86/WAN_runs.jsonl" "$RAW/input_plan.json"
python3 "$ROOT/experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_audit_results.py" --raw-root "$RAW" --head b0e474cfbbcae433405ac61c5240068878ba43ca --source-root "$ROOT" --aav-binary "$AAV" --baseline-binary "$BASE"
```

实际新构建先在未配置 emp 前缀时被 CMake 明确拒绝；使用仓库现有真实 emp-tool/emp-ot 前缀后 Release 目标成功。顺序验证：AAV86 conformance/metrics 2/2、冻结 oracle differential 1/1、AAV86 与 baseline 独立进程 E2E 2/2；旧 Protocol I/III 与 TCP 等价回归 6/6；额外 TCP phase 等价六项、关闭/静默/截断三项有界失败和 D16 六夹具均通过。小实例还断言手工有效载荷与实际 DCF 扩展次数。全量 `all` 目标未作为本次协议目标构建门，历史 `bitpack_test` 独立链接问题不被计入上述通过数。正式 120 行由新 source/binary 冻结运行，独立审计在 `b0e474c` 下复算 120/120 行、20/20 组五次统计、24/24 输入配对；新原始索引 151/151 与外部副本 152/152 一致。构建临时文件 `/tmp/m6a14-emp-release`、`/tmp/m6a14-*.log` 不在 Git 差异。

本阶段实际改动文件见 Git 差异：`.gitignore`；两份 TEST_ONLY 入口 `protocol_i_aav86_small_e2e_test.cpp`、`protocol_i_e12_baseline_bench_test.cpp`；新增 `protocol_i_e14_material_metrics.h`；新增 E14 的输入计划、AAV86/基线 runner、两个 network namespace wrapper、独立审计和统一 CSV 导出脚本；更新 `docs/BENCHMARK_VALIDATION_PLAN.md`、`docs/IMPLEMENTATION_PLAN.md`、E14 指标来源决策；新增本报告。E13 六份文档/脚本在单独 `2b48ef6` 检查点保存，非 E14 性能二进制差异。最终仅建立本地提交，不推送、不合并、不创建 PR。

E14 **逐文件**变更：

| 路径 | 作用 |
| --- | --- |
| `.gitignore` | 排除 E14 原始目录 |
| `VFSS/tests/moe_topk/protocol_i_aav86_small_e2e_test.cpp` | AAV86 测试入口的有效载荷观测与 OS 随机输入份额 |
| `VFSS/tests/moe_topk/protocol_i_e12_baseline_bench_test.cpp` | 基线测试入口的本地 shuffle、离线 OT、分阶段 DCF PRG 观测 |
| `VFSS/tests/moe_topk/protocol_i_e14_material_metrics.h` | 两路线共用 TEST_ONLY 逐字段有效载荷计数 |
| `experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_input_plan.py` | 与 T serial 独立的匹配输入计划 |
| `experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_run_matrix.py` | AAV86 新批次 JSONL/五次统计 |
| `experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_run_clique_baseline.py` | baseline 新批次 JSONL/五次统计、角色退出码 |
| `experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_shaped_namespace.sh` | AAV86 TCP 网络校准与运行 |
| `experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_clique_shaped_namespace.sh` | baseline TCP 网络校准与运行 |
| `experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_audit_results.py` | 独立复核原始行、日志哈希、配对和统计 |
| `experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_export_unified.py` | 导出后续六方案可复用的九主指标 CSV |
| `docs/decisions/M6A_P2_I_E14_UNIFIED_METRIC_PROVENANCE_2026-10-03.md` | 先行来源表与执行后判定 |
| `docs/BENCHMARK_VALIDATION_PLAN.md` | 统一材料与 PRG 口径 |
| `docs/IMPLEMENTATION_PLAN.md` | E14 进度和未完成门 |
| `docs/reviews/M6A_P2_I_E14_PROTOCOL_I_UNIFIED_PERFORMANCE_CLOSEOUT_2026-10-03.md` | 本报告 |

进入 Protocol III+AAV86 **设计审查**可交接：E5 明文 AAV86 控制器及 `e_t/v_t` 图定义、E1–E6 全两两材料账本与容量公式、本报告的计量 schema/输入计划/校准/原始索引流程、稳定 signed Q20.12 原序 XOR mask oracle。必须另证 Protocol III 的 field 代数与预处理时序、公开 rank/自适应图泄露、原序 mask 路由及完整因果轮数；Protocol I 的 shuffle 联合视图与 `2r+1` 不能直接移植。规划聊天复检后再启动下一阶段。
