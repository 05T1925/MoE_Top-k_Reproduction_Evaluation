# M6A-P2-I-E13：E12 被测版本技术复核与 Protocol III+AAV86 设计交接

日期：2026-10-03。复核对象严格为正式运行源码
`8ad0725c73716ac37c96d417136958f932233d61`、事后报告提交
`c86f5bd50a938be8550d41ca99712d5859f9a1ae` 和忽略目录
`experiments/m6a_p2_i_allpairs/TEST_ONLY_E12_RAW/accepted/` 的原始记录。
该报告不更改 E12 的任何原始记录。

**程序性限制：本聊天的既有上下文包含 E12 的编写、运行及报告过程，因此
不满足用户指定的“未参与 E12 的异会话接收方”。本报告是重新计算的技术
复核，不能冒称异会话独立签收。**
`E12_TECHNICAL_RECHECK=PASS_WITH_FINDINGS`；
`E13_CROSS_CHAT_INDEPENDENT_ACCEPTANCE=FAIL`（程序性资格未满足）。
下述有界数据可按明确口径引用，正式接收门仍需另一独立聊天核对。

## 1. 起点、证据身份与保护

复核工作树为
`C:/Users/28641/Desktop/MoE_Top-k_Reproduction_Evaluation__m6a_p2_i_e11_benchmark`，
分支 `codex/m6a-p2-i-e11-benchmark`，开始时 HEAD 为 `c86f5bd...`，
`git status --porcelain=v1 -uall` 为空。E12 正式 JSONL 的 120 行均写
`head=8ad0725...`、`tracked_state=CLEAN`，相关源码从该提交到报告提交
没有差异；不能把 `c86f5bd` 写成被测 revision。主工作树
`feat/m6a-performance-evaluation@c3926c68...` 的原有四个 tracked 修改、
三个未跟踪文档和 `siamjdiscrmath.pdf` 仍在原位，本次未写入。

| 冻结证据 | SHA-256 |
| --- | --- |
| E12 报告原件（本次更正前） | `523D010C7DD3A7B711BC98BFD5E18C6E37407345129C86830DF4BA5AAF5A79EA` |
| `accepted/e12_raw_index.sha256` | `110C445F288539FF0AB5E4FB78451FAEF504C58ED164F33B37CC3EA1AFC12C88` |
| AAV86 `LAN_runs.jsonl` | `2FD7F1D3A6D1C4BBBB82103D6CCE42297EAAEDC06414B9C9DE2F1A90A98B668A` |
| AAV86 `WAN_runs.jsonl` | `9AC21139A9158ABEC1B53F20141AB3B2145F25E717EA87E07FBC1B53C14CD9D3` |
| 全对全 `LAN_baseline_runs.jsonl` | `52F038C266A9FEB504D23D08189648B63914572DAD5E350AF8ACA904B2679D19` |
| 全对全 `WAN_baseline_runs.jsonl` | `478141F4257170D2626C792FC344C7CFEF0E70F0ECEE6D670DC577C7470FEBA4` |
| AAV86 Release 二进制 | `805E2B5B96653ED6E5C1A0C2428A1A0ECB41A9A1DC5C9D6ABBD2975579578904` |
| EMP-ON 全对全 Release 二进制 | `C767797CD712C2DCDA8D522AC1E9F7AD13E71E4DE35B1A8AE10DE4BB913A6456` |

当前工作树的 AAV86 头、实现、E2E harness、全对全 benchmark harness
SHA-256 分别为
`576F4A683DDC8702F3BB18BBD6933824B07919D7A2F1AC0F8A5845F0B7B9D580`、
`FB38C894E0266B2D426BD07CFFAD6AE8365D9E5DC2594BE1A42F7660CCB4BAC0`、
`7A5994F62C118C3D9E0AAC4A948B9ABFEF48EC9532678736A6F32FAB63E3AFAF`、
`F4028304A101CBE82B08F54E4C08E04FD3E8A9869AAF8FEE319F658A72FBB652`。
上述四个哈希与 E12 报告相符；两份二进制在现存 WSL 构建目录重算亦相符。
论文定义、旧实现事实、可信 T 全池项目扩展及密码学假设保持分栏；
`AUTHOR_EXACT=NOT_PROVEN`。

## 2. 不依赖 E12 审计脚本的原始复算

新增只读 `e13_receiver_audit.py` 没有导入 E12 汇总或审计脚本。
它从 151 项索引逐文件重算 SHA-256，再读四份 JSONL、全部 120 条
对应原始日志、四份校准 JSON、十二份 qdisc 原文、四份 summary、
32 行容量 CSV 和记录所指源码。索引 151/151 无缺失或哈希差异；
逐条日志的输入标识、退出行、主要时间/字节/边/DCF 数值与 JSONL 一致。
对每组五次正式记录重新计算全部保存的 median/min/max，20/20 组匹配；
`total` 是**逐条先求 offline+online，再取中位数**。未引用 E11 或
E12 作废批次参加统计。

| 路线/网络 | accepted 行 | 预热 | 正式 | 配置组 | 逐行审计 |
| --- | ---: | ---: | ---: | ---: | --- |
| AAV86/LAN | 48 | 8 | 40 | 8 | PASS |
| AAV86/WAN | 48 | 8 | 40 | 8 | PASS |
| 全对全/LAN | 12 | 2 | 10 | 2 | PASS |
| 全对全/WAN | 12 | 2 | 10 | 2 | PASS |

每条记录均对应 `n=D=128`、K=2/8、正确性 PASS、精确 HEAD 与
源码/二进制哈希。相同网络、K、重复编号下，全对全与四个 r 的
`input_seed` 和 `input_digest` 全部相同。AAV86 每条有 r 个
`e_t/v_t/PRG_t`，`e_A=sum(e_t)`、`v_A=sum(v_t)`；
源码以当轮活跃边端点集合去重计 `v_t`
（`protocol_i_aav86_small.cpp:728–739`），未把 D 填作活跃节点。
每方预留 `r·C(128,2)` 槽，score DCF=512，
CA DCF=`2e_A`；传统 DCF 长度倍增 PRG 的 score 为 17,408，
每轮 CA 为 `80e_t`，96/96 行匹配。

两方逐阶段发送和、接收和分别等于各自总量；各阶段发送与对方
对应接收相等。`online_comm_total_bits=8(P0发送+P1发送)`，未重复
加接收。AAV86 组合阶段 `CA+carrier` 在两方逐条加法守恒；
`online=max(online_p0,online_p1)`。全对全八轮、AAV86 `2r+4`
完整入口以及 AAV86 `2r+1` CA 核心按源码消息相位及 E2E trace
形状核对，不能把后一数值借给 Protocol III。

独立复算的五次正式总时间如下；方括号为逐次总时间的最小/最大值，
单位 ms。离线与在线列各自是五次中位数，不能相加得到总列。

| 网络 | K | 路线 | 离线中位 | 在线中位 | 总中位 [min,max] |
| --- | ---: | --- | ---: | ---: | ---: |
| LAN | 2 | 全对全 | 167.222 | 45.586 | 214.362 [207.778,217.758] |
| LAN | 2 | AAV86 r2 | 187.014 | 22.222 | 210.428 [204.262,213.766] |
| LAN | 2 | AAV86 r3 | 256.201 | 21.948 | 279.289 [244.037,280.843] |
| LAN | 2 | AAV86 r4 | 330.833 | 21.822 | 353.201 [329.900,468.571] |
| LAN | 2 | AAV86 r5 | 495.390 | 23.900 | 533.520 [476.935,591.645] |
| LAN | 8 | 全对全 | 164.374 | 45.133 | 209.692 [205.755,217.637] |
| LAN | 8 | AAV86 r2 | 193.912 | 22.341 | 216.253 [185.031,231.809] |
| LAN | 8 | AAV86 r3 | 274.969 | 20.830 | 296.298 [272.403,298.115] |
| LAN | 8 | AAV86 r4 | 363.249 | 22.090 | 385.339 [382.043,439.791] |
| LAN | 8 | AAV86 r5 | 451.167 | 24.269 | 489.290 [458.928,518.643] |
| WAN | 2 | 全对全 | 166.875 | 501.507 | 667.265 [661.372,678.536] |
| WAN | 2 | AAV86 r2 | 199.241 | 421.559 | 625.858 [608.511,667.512] |
| WAN | 2 | AAV86 r3 | 269.438 | 521.745 | 789.816 [781.933,803.547] |
| WAN | 2 | AAV86 r4 | 374.014 | 616.040 | 990.053 [979.783,1015.025] |
| WAN | 2 | AAV86 r5 | 491.262 | 722.627 | 1213.889 [1181.478,1279.679] |
| WAN | 8 | 全对全 | 165.433 | 502.958 | 668.757 [665.246,677.254] |
| WAN | 8 | AAV86 r2 | 191.163 | 428.702 | 616.169 [601.865,632.493] |
| WAN | 8 | AAV86 r3 | 265.800 | 517.110 | 782.920 [778.326,793.146] |
| WAN | 8 | AAV86 r4 | 370.485 | 622.444 | 992.929 [979.038,1004.859] |
| WAN | 8 | AAV86 r5 | 437.461 | 718.726 | 1155.595 [1147.661,1180.049] |

四份校准的应用层 RTT/吞吐与 JSONL 引用一致：AAV86 LAN
1.230 ms/835.419 Mbit/s、WAN 50.342 ms/62.654 Mbit/s；全对全
LAN 1.166 ms/836.546 Mbit/s、WAN 50.504 ms/63.064 Mbit/s。
校准后到协议结束的 qdisc 字节增量依次为
1,825,026、1,825,092、2,314,440、2,317,674 B；这是实际
P0/P1 TCP 所经同主机 network namespace/qdisc 的流量证据，
不是应用层通信量，也不是物理双机网络结果。

## 3. `8ad0725` 源码计量合同

- AAV86 E2E 在建完 fd/TCP 后、首次 fork P0/P1/T 前设置
  `offline_start`（`protocol_i_aav86_small_e2e_test.cpp:296–319`）；
  等待 T 退出及双方 ready 后才设 `offline_end`（:339–360）。
  全对全对应顺序在
  `protocol_i_e12_baseline_bench_test.cpp:213–264`。建连另列
  `transport_setup_ms`。T 的生成/序列化/分发和退出后 ready
  屏障分别计数，但并行区间不能简单相加取代包围时间。
- 两条路线均在 party 收到本轮输入份额后、调用 secure score
  适配器前起在线表，返回原序共享 mask 后止表；测试控制器
  发输入、oracle 重构和结果收集在在线 party 时间之外。
  `total_time_ms=offline_time_ms+max(online_p0_ms,online_p1_ms)`，
  **不含**另列的 TCP 建连。
- AAV86 在 `protocol_i_aav86_small.cpp:807–820` 用相邻时间戳
  实测 CA 与 carrier；CA 时间含首轮 shuffle、逐轮 CA、公开
  full-order flatten，不等于论文纯 CA 子程序。:674–684 与
  :842–849 从本方 `message_trace.received_bytes` 汇总阶段接收。
  `dcf.cpp:90–98` 仅给传统 DCF 每次 AES seed→两块扩展加
  thread-local 计数；score、逐轮 CA 和 inverse 在入口分别读取。
  公开 pivot PRNG、其他 AES/FSS 路径不在这个 PRG 指标内。
- 全对全经 `MOE_TOPK_ENABLE_EMP_OT=ON` 链接 emp-tool、emp-ot
  和 OpenSSL；`protocol_i_chosen_ot_emp.cpp:202–224` 直接调用
  `emp::IKNP`，不是 mock OT。benchmark harness 以 OS 熵
  初始化 FSS、mask 和 benchmark 输入份额，
  `protocol_i_shuffle_preprocess_party` 在双方离线 ready 前运行，
  P0/P1 在线用 TCP；T 仅有离线包和计量 telemetry fd，退出后才分发输入。
  基线是两轮 score、两轮 forward、一轮 masked comparison open、
  一轮 rank reveal、两轮 reverse 的**八轮工程完整入口**，
  不能称为论文三轮核心的直接性能测量。

## 4. Findings 与指标证据等级

| ID / 严重度 | 发现、复现与影响 | 处理 |
| --- | --- | --- |
| F0 / 接收门 | 本聊天参与 E12 工作，不能签署“未参与 E12”的异会话接收。检查聊天历史和 E12 提交过程即可复现。这不改变原始数值，但阻断程序性签收。 | 本报告明确标记技术复核；请真正独立窗口复核本报告、`8ad0725` 与原始目录后签收。 |
| F1 / 安全表述 | `e12_run_clique_baseline.py:174–178` 使 input seed 与交给 T 的 serial 都由公开 K/repetition 日程决定。基线确实没有**直接**向 T 传 input_seed，然而 T 可按日程推导测试合成输入。AAV86 控制器在 `protocol_i_aav86_small_e2e_test.cpp:365–367` 用已知 serial 播种 `mt19937_64` 生成 P0 的测试份额；P1 若知道 serial 与自身份额，可重建本轮合成分数。因而 TEST_ONLY 夹具不能充当输入隐私的角色视图证明。secure 入口接收外部份额，条件性协议论证另行成立。 | 已收窄 E12 决策和报告文字；不改被测代码、不重写原始值、不重跑 120 次。将此作为后续新基准输入生成器的安全卫生修正。 |
| F2 / 原始字段 | 全对全 JSONL 没有单列 `exit_t/p0/p1`，虽 24 条原始日志逐条均有 `t_exit=0 p0_exit=0 p1_exit=0`，且 runner 在子进程失败时不会产出 accepted PASS。JSONL 单文件不是角色退出状态的完整记录。 | 保留 E12 原始日志；未来新批次可把退出码也写进 JSONL，不追改旧数据。 |
| F3 / 统一成本 | 全对全 `package_only_material_bits` 仅是 T package，缺 party 本地 EMP OT 材料；其完整离线材料、同定义 PRG 与纯论文核心时间均 `NOT_MEASURED`。 | 时间、在线通信和工程轮数可并列；不计算材料或 PRG 加速比。 |

| 指标 | E12 证据等级及范围 |
| --- | --- |
| 正确性、每方 secure 耗时、T/party 离线子区间、每方在线阶段收发字节、序列化包长、e_t/v_t、DCF Eval、AAV86 已覆盖 PRG、RSS、RTT/吞吐 | **MEASURED**：运行时输出或校准原值，原始日志与 JSONL 对账；正确性由 TEST_ONLY 冻结 oracle 比较。 |
| `online_time_ms`、`total_time_ms`、应用层总/每方 bits、e_A/v_A、预留全池槽、五次 median/min/max、解析容量、名义因果轮 | **DERIVED**：从实测原值、固定形状和已执行 trace/阶段结构计算；轮数是预填式并由 trace 相位/条数在 E2E 检查，未直接测物理链路往返。 |
| 全对全完整 EMP OT 离线材料、基线 PRG、其论文纯核心时间、两路线其他 AES/PRG 细分、物理双机表现 | **NOT_MEASURED**。 |
| n≥256 的在线/离线实测和峰值 | **RESOURCE_LIMITED**：仅 checked 容量与实际 preflight 拒绝；无 keygen、无性能记录。 |

容量 CSV 32 行中 n=128 的 8 行有正式记录；n≥256 的 24 行
均为 `HARD_CAP_D_GT_128` / `PRECHECK_REJECTED_NO_KEYGEN`。
源码在 `protocol_i_aav86_small.cpp:283–337` 先做 checked
形状/包/预算/资源判断，dealer 在 preflight 后才开始 keygen。
n256/r2 的每方解析包为 69,999,474 B，另超过 64 MiB；
这些是容量与拒绝证据，不是运行表现。容量日志里 n128 的
`process_limit=1` 是在**未设置**正式 768 MiB 限制时的检查，
正式运行行记录了限制并成功，不应混写成正式批次的拒绝。

## 5. 分项接收与可冻结范围

| 门 | 本次技术判定 | 准确边界 |
| --- | --- | --- |
| E12 n128 正确性及五次正式数据 | **PASS_WITH_FINDINGS** | 120/120 accepted 行、151/151 原始索引、20/20 组统计、源码/二进制身份与日志均匹配；F2 记录形式不完整。 |
| 两路线完整入口时间及在线通信可比 | **PASS_WITH_FINDINGS** | 同 Release 优化、同合成输入、同模拟网络、同 offline/online/total 口径；路线的 shuffle/OT 离线工作不同，不能外推为作者核心对比。 |
| 全部离线材料与 PRG 指标完整性 | **FAIL** | 基线 EMP OT 材料与 PRG 缺失，AAV86 仅定义的传统 DCF 扩展 PRG 已测。 |
| TEST_ONLY 角色视图可作隐私证据 | **FAIL** | F1 的公开日程/确定份额使测试合成输入可推知；secure runtime 安全结论仍是明确假设下的**条件性主张**，未因功能测试证明。 |
| n≥256 资源限制证据 | **PASS** | 24 点为公式加真实 preflight，不是运行测量。 |
| Protocol I+AAV86 有界性能候选 | **PASS_WITH_FINDINGS（技术范围）** | 可冻结 `8ad0725` 下 D≤128、n128/K2/8/r2..5 的同主机模拟 LAN/WAN 五次结果及全两两 `O(rD²)` 离线成本；正式异会话签收仍待独立窗口。 |
| M6A/V3 总完成 | **FAIL / NOT_COMPLETE** | Protocol III+AAV86 尚未实施；n≥256 无成功运行；基线部分统一指标缺失；`AUTHOR_EXACT=NOT_PROVEN`。 |

安全声明继续以单份 DCF/FSS 选阈值隐私及整池多 key
混合归约、独立 keygen 随机币、AES/PRG 安全、可信且不合谋且
在线静默 T、单方半诚实 P0/P1、私有完整通道为条件。全对全
另依赖真实 EMP IKNP OT 的安全假设。公开 masked score、
shuffled local rank、pivot/bucket/图、查表顺序、消息长度及
abort 时点属于原项目泄露边界。这里未重做密码学证明，
亦未将 Protocol I 的单方 shuffle 论证移植到 Protocol III。

## 6. Protocol III+AAV86 紧接设计输入

可复用的**算法/工程资产**：E5/E6 的 AAV86 明文
TEST_ONLY 控制器和图 fixture；E1 全两两材料槽位/容量实验；
已测的 `e_t/v_t` 去重定义、checked 容量、原始索引哈希、
网络 runner 的校准/原始日志/五次统计格式；冻结的 signed
Q20.12、稳定同分、原序 XOR mask oracle。上述均不自动
给 Protocol III 提供安全材料或两轮压缩。

设计前必须另行给出并审查：

1. Protocol III 当前 field Fselect/Fsort 与 raw-score mask
   实际入口的代数条件：域表示、非零 payload、非零乘法
   mask、逆元及 DPF 兼容，不能把环 `Z_(2^b)` 当作域。
2. 动态 AAV86 图在 Protocol III 路由/DCF 参数确定之前
   哪些材料可离线预发；若继续全池，明确端点/轮/掩码
   绑定和 P0/P1/T 单方视图，不套用 Protocol I 的 shuffle 证明。
3. Protocol III 是否允许打开逐轮 local rank、pivot、bucket、
   活跃图、长度和 abort；不能沿用 Protocol I 的公开 rank
   泄露口径而不证明。
4. 从 Protocol III 的共享输出恢复原输入顺序 XOR mask 的
   路由、padding/dummy、稳定同分与恰 K 位证明；
   secure 路径不得重构 index、rank 或 mask。
5. 从真实消息 DAG 分别推导核心与 signed raw-score 完整
   入口的因果轮数；`2r` 仅是项目目标，Protocol I 的
   `2r+1` 和 `2r+4` 不转移。随后再按 conformance、
   冻结 oracle differential、独立进程 E2E 准入 secure runtime。

## 7. 命令、未运行工作和修改范围

主要只读命令：`git branch --show-current`、
`git rev-parse HEAD`、`git status --porcelain=v1 -uall`、
`git diff 8ad0725..HEAD -- VFSS`、
`wsl.exe -d Ubuntu-24.04 -- sha256sum -c <accepted/e12_raw_index.sha256>`、
`wsl.exe -d Ubuntu-24.04 -- sha256sum /tmp/m6a12-release/moe_topk_m6a7_aav86_small_e2e_test /tmp/m6a12-emp-release/moe_topk_m6a12_protocol_i_clique_benchmark_test`，
以及：

```text
python experiments/m6a_p2_i_allpairs/TEST_ONLY/e13_receiver_audit.py --raw-root experiments/m6a_p2_i_allpairs/TEST_ONLY_E12_RAW/accepted --source-root .
```

该脚本输出 `errors=[]`，文件 SHA-256 为
`6149DD2E8AB578E333B6AAE27AAC9487129AE7C5389D326881DEB959553D9CCD`。
另以一次性独立 Python 读取 120 条原始日志，将
offline/online、包长、DCF、阶段字节、逐轮数组逐项对回 JSONL，
并核对 32 行容量状态，结果 120/120、32/32，无差异。
最初的一次审计试写把拒绝码误设为 `HARD_CAP` 而报 24 项；
核对 CSV 的实际码 `HARD_CAP_D_GT_128` 后修正审计谓词，
不是原始数据差异。

本轮**未运行**新的 conformance、oracle differential、进程 E2E、
120 次网络矩阵、全量 `all` 构建或任何 Protocol III runtime。
没有重新生成或消费密钥。E11/作废 E12 批次没有进入统计；
已有 `bitpack_test` 全量链接故障仍按 E12 历史记录保留。

本次只新增本报告与独立只读审计脚本，并收窄 E12 决策/报告的
测试 seed 措辞、在实施计划登记技术复核边界。最终
`git diff --check` 为 exit 0；两份未跟踪新文件均无行尾空白且有
终止换行。未改
`VFSS/` 运行时代码、`VFSS-baseline/`、`Papers/`、
本地参考树、主工作区或 E11/E12 原始数据；未提交、推送、
合并或创建 PR。`git diff --check` 和未跟踪文件空白检查须在
交付前的复核结果如上。
