# M6A-P2-I-E10：Protocol I+AAV86 全两两候选、计量与 D128 资源门

日期：2026-10-03。范围仅为可信、非合谋离线 T 预发每轮全部 canonical pair，P0/P1 在线只取活跃边的独立 Protocol I+AAV86 项目实例。固定 M、在线补料、Protocol III+AAV86 与正式 V3 LAN/WAN 矩阵不在本次范围。`AUTHOR_EXACT=NOT_PROVEN`。

## 1. 接收起点与 E9 本地检查点

隔离 worktree `C:\Users\28641\Desktop\MoE_Top-k_Reproduction_Evaluation__m6a_p2_i_allpairs_experiment` 起始为 `codex/m6a-p2-i-allpairs-experiment@c3926c68fd14f270faa8b55234311071947fa080`，完整 `git status` 显示 E1–E9 源码/文档多为未跟踪、`VFSS/CMakeLists.txt` 与实施计划已修改。修改前，E9 异会话接收报告登记的六个源码 SHA-256 与工作区逐项一致；其中接口头为 `A7A4642B...747C53`、实现源码为 `DA2537FF...E95AC`，完整六项见 [E9 接收报告](M6A_P2_I_E9_CROSS_CHAT_RECEIVER_AUDIT_2026-10-03.md)。本阶段未把 E10 修改后的源码误当成 E9 旧签收对象。

先显式暂存 28 个既有文件：`VFSS/CMakeLists.txt`、E9 六个接口/源码/测试、实施计划、两份 M6A 决策、E1 复现材料文档、E1/E4/E5/E7/E8/E9 历史报告及异会话接收、七个隔离 TEST_ONLY 源码/说明。`git diff --cached --name-status` 的 28 项均属这些路径。三处历史文件末尾多余空行经最小修整后，`git diff --cached --check` 通过；原有相关测试重新运行 25/25 PASS。建立**仅本地** E9 检查点提交 `142db65776b6f7334dac02845109308a887d3995`。没有使用 `git add .`。

未纳入提交的 E9 原始记录：`experiments/m6a_p2_i_allpairs/TEST_ONLY_E9_2026-10-03/e9_per_configuration_raw.csv`，SHA-256 `373DE72235A0EBAFA4E7231058F12BD0A747785E2A780E5079750D836CE57C3F`；同目录 `m6a9-d64-e2e.log`，SHA-256 `FF036E7E95AF058D2CBB988C947BA15806414C87A5819D0643FEDF956155A44A`。E5/E6 CSV/JSON、E7/E8 原始日志也保持未跟踪。`VFSS-baseline/`、`Papers/`、参考树、构建物、密钥与主工作区原有差异均不在检查点。未推送、合并或创建 PR。

E9 D128 口径按接收 finding 保留原报告：**当时首先由 `D≤64` 硬上限拒绝；388.61 MiB 是解析工程预算，不是 E9 API 的实际 D128 拒绝原因。**

证据层级：AAV86 原文和 Agarwal CCS'24 CA 是论文定义；E5 Python 是明文 `TEST_ONLY` 参考行为；全两两预发、统一 D、signed Q20.12、同分原下标优先、原序 XOR mask 及同 π 逆路由是项目扩展；整池 DCF/FSS hybrid、AES/PRG、可信非合谋 T 和完整私有通道仍是条件性假设。源码测试不构成密码学证明。

## 2. PRG 与图节点计数合同

`VFSS/ext/FSS/dcf.cpp::traverseOneDCF` 的传统 DCF Eval 每层对当前 seed 执行一次 `AES::ecbEncTwoBlocks`，得到两个 128-bit block；在**完成该两块扩展后**，线程本地计数器加一。计数单位是一次 seed→两块的长度倍增扩展，既不是两个 AES block，也不是一条边或一次 DCF Eval。计数器在 party 已持久 claim、开始 score adapter 前重置，单次 party 返回时读取。该入口的两次 score uCMP/端点调用传统 DCF，CA 每条活跃边的一次 uCMP 调用传统 DCF；当前前向/逆向路由不执行该扩展，公开 pivot 的 AES 流 PRNG 不属于此“长度倍增 PRG”指标。未把其他 FSS 变体、未来多线程子任务或系统 AES 调用宣称为已覆盖。key 格式和 FSS 返回值未改变。

小 fixture 直接核对：3 层单次 DCF Eval 记录 3 次；34 位 uCMP 的两次 DCF Eval 每方记录 68 次；另一个线程起始计数为 0，未改写当前线程计数。随后 differential 对每方核对 `score_prg=4D·34`、各轮 `ca_prg=2e_t·(33+log2 D)`，这些公式**仅作为独立断言**，正式测量值来自原语计数器。E2E 独立进程逐方输出 score、CA、inverse、total；`inverse_prg=0` 是已覆盖路径的实际读数。其他 PRG/AES 细分指标保持 `NOT_MEASURED`。

`e_A` 是每轮去重后的无序边数之和；`v_A` 是**每轮**活跃边端点的去重数，再跨轮求和，不能直接填 D。同一端点在不同轮出现按各轮分别计入。secure 函数只从已经公开的图计算，未打开额外输入。differential 从 `edge_trace` 独立复算每轮端点集合并与双方 metric 比较；E5 明文 CA 图的 14 个逐轮样例中，端点去重数与其 `logical_vertices_in_graph` 一致。E5 与当前 C++ 的 pivot PRNG 不同，该检查验证计数**定义**，不冒充逐次随机图完全相同。

通信来自实际 `message_trace`：每方 `score_sent+core_sent+inverse_sent=online_sent`，每方发送与对方接收相等；`online_comm_total` 只加 P0/P1 发送量。帧计数层为应用层实际发送字节，未测 TCP/IP 抓包字节。E2E 验证实际 phase 顺序与收发累计：score 两轮，CA 初始 shuffle 加每次 masked-open/rank-reveal 为 `2r+1`，同 π inverse 一轮，因此仅本独立 raw-score→原序 XOR mask 入口为 `2r+4` 因果轮。

离线包围时间从 parent 开始等待 T，到 T 完成生成、序列化和两份分发、退出，且 P0/P1 都发出完成反序列化的 ready 信号为止；包含 T fork/exec 启动和双方接收屏障。T 的 `generate/serialize/distribute` 分段时间另列，两者之和不强求等于包围时间。在线每方时间从收到输入 shares 后调用 secure party 起，到返回 XOR shares 为止；包括持久 claim、协议计算与通信，排除测试输入分发、oracle 重构、结果回报。报告字段和重构仅在 TEST_ONLY parent。

## 3. D128 公开形状、准入及运行

纯公开形状接口以 checked arithmetic 对 `1≤n≤10^6`、`1≤r≤5` 计算 `D=max(2,next_power_of_two(n))`、`C(D,2)`、`rC(D,2)`、`b=33+log2 D` 和每方序列化字节 `B=114+D(1844+8r)+rC(D,2)(81+24b)`，不分配随 D 增长的对象。独立 assessment 列出硬上限、64 MiB 包、生成预算、可用内存、material ID 与 D128 进程限制；dealer 在任何全池 reserve/keygen 前调用准入。D128/r5 的形状为每方 40,640 槽、42,547,506 包字节、`8B+64 MiB=388.61 MiB` 工程预算。该预算是保守工程额度，**不是可移植堆内存定理**。

生成生存期复核：T 同时持有两份 key/mask/路由对象，每个 keygen 临时 pair 在边循环本次迭代结束释放；之后顺序构造双方包，包缓冲与双份材料短时共存。`8B+64 MiB` 给材料对象、vector/allocator、临时 DCF 和序列化增长留余量；D128/r5 Debug E2E 实测 T 峰值 227,580 KiB，低于 388.61 MiB 工程预算。准入还要求单方包 ≤64 MiB、预算 ≤512 MiB、有效可用内存至少 3 倍预算，并检查可读的 cgroup v2/v1 limit-current；若无 cgroup 限制文件，本机取 `sysconf` 可用页。D128 **必须**在 `RLIMIT_AS` 640–768 MiB 的进程中运行，否则预检拒绝。本次 WSL2 设 `ulimit -v 786432`（768 MiB），可用内存约 7.2 GiB、`/tmp` 可用磁盘约 924 GiB。T/P0/P1 由同一 bounded shell fork+exec，均继承进程限制；任一内存分配失败直接报错，材料不补发、不复用。该环境检查不能泛化到其他主机。

在不生成 key 的 D128/r5 assessment 中，五个资源拒绝标志均为 false；未设置 RLIMIT_AS 时 `process_limit=true` 且 preflight 明确拒绝。之后依序运行 D128 的 conformance → frozen oracle differential → T/P0/P1 fork+exec E2E，**各 6/6 PASS**。覆盖 n=127/128、r=2/5、K=1/中间/n、全同分、重复、负数与 INT32_MIN/MAX。各例双方包长相等、原序 XOR mask 重构恰 K 位且与 oracle 一致；同一 π 逆路由与一次性 claim 保持原合同。D16/32/64 在最终源码上同顺序各 6/6 PASS。既有错误包、持久 replay、静默 peer 超时与关闭通道用例在原回归中继续运行。

| D | 最大实际 r | Debug 三阶段 | E2E 单方包（r=5） | T 峰值最大 | P0/P1 峰值最大 | 最大离线包围 | 最大在线单方 |
|---:|---:|---|---:|---:|---:|---:|---:|
| 16 | 5 | 6/6 各项 PASS | 611,658 B | 7,548 KiB | 5,120 KiB | 27.61 ms | 3.97 ms |
| 32 | 5 | 6/6 各项 PASS | 2,523,042 B | 17,560 KiB | 9,216 KiB | 117.88 ms | 7.89 ms |
| 64 | 5 | 6/6 各项 PASS | 10,372,050 B | 57,872 KiB | 25,088 KiB | 480.53 ms | 16.64 ms |
| 128 | 5 | 6/6 各项 PASS | 42,547,506 B | 227,580 KiB | 90,624 KiB | 2,018.36 ms | 48.95 ms |

D128/r5 一个完整例（n=128,K=128）的实际逐轮 `e_A=[253,333,255,233,135]`、`v_A=[128,126,119,104,65]`，`e_A=1209`，`v_A=542`，每方 score PRG 17,408、CA PRG 96,720、inverse PRG 0，双方在线 PRG 总计 228,256。每方在线发送 score 4,192、CA 11,792、inverse 1,072，总计 17,056 字节；双方合计发送 34,112 字节，各自接收 17,056 字节。上述为**单次随机图观察**，不当作复杂度上界或平均值。

Release 构建在相同本机 socketpair 环境、相同 768 MiB 进程限制下再跑 D128 六个独立 fresh-material 配置，6/6 PASS，仅验证记录格式与边界；预热 0、重复 1，不是 V3 要求的 1 次预热+5 次正式重复，也没有 LAN/WAN。Debug 与 Release 数字不合并统计。

## 4. 正式矩阵容量与真实状态

[24 行 checked 容量 CSV](../../experiments/m6a_p2_i_allpairs/TEST_ONLY_E10_2026-10-03/e10_matrix_capacity_checked.csv)由 C++ 公开 shape/assessment 实际输出生成。表内每行的 `slots_per_party`、`party_package_bytes` 为**解析值**；`actual_status` 单独区分已试运行、只算容量和预检拒绝。下表按 `r=2/3/4/5` 顺序列每方值，覆盖要求的全部 24 配置：

| n | D | 每方槽数，r=2/3/4/5 | 每方包字节，r=2/3/4/5 | 真实状态 |
|---:|---:|---|---|---|
| 128 | 128 | 16,256 / 24,384 / 32,512 / 40,640 | 17,160,690 / 25,622,962 / 34,085,234 / 42,547,506 | r2、r5 Debug+Release E2E PASS；r3、r4 容量计算，仅未运行 |
| 256 | 256 | 65,280 / 97,920 / 130,560 / 163,200 | 69,999,474 / 104,763,122 / 139,526,770 / 174,290,418 | 全部 HARD_CAP+PACKAGE_LIMIT，未 keygen |
| 1,000 | 1,024 | 1,047,552 / 1,571,328 / 2,095,104 / 2,618,880 | 1,167,830,130 / 1,750,801,010 / 2,333,771,890 / 2,916,742,770 | 全部预检拒绝，未 keygen |
| 10,000 | 16,384 | 268,419,072 / 402,628,608 / 536,838,144 / 671,047,680 | 324,549,132,402 / 486,808,592,498 / 649,068,052,594 / 811,327,512,690 | 全部预检拒绝，未 keygen |
| 100,000 | 131,072 | 17,179,738,112 / 25,769,607,168 / 34,359,476,224 / 42,949,345,280 | 22,007,488,315,506 / 33,011,111,624,818 / 44,014,734,934,130 / 55,018,358,243,442 | 全部预检拒绝，未 keygen |
| 1,000,000 | 1,048,576 | 1,099,510,579,200 / 1,649,265,868,800 / 2,199,021,158,400 / 2,748,776,448,000 | 1,487,639,764,009,074 / 2,231,458,679,226,482 / 2,975,277,594,443,890 / 3,719,096,509,661,298 | 全部预检拒绝，未 keygen |

特别地，n=256、r=2 的单方包已经是 69,999,474 字节，**超过**现行 64 MiB (=67,108,864 字节)；r=5 达 174,290,418 字节。不能通过隐式固定 M、在线 T、稀疏预发或缩小 n 来记为原配置通过。最大**实际成功** D=128、r=5；最大**仅完成公开容量计算** D=1,048,576（n=10^6），无对应 keygen 或运行。

## 5. 复跑与证据索引

E10 源码基于 E9 本地检查点 `142db65776b6f7334dac02845109308a887d3995`，在隔离 worktree 修改，最终精确文件哈希见下表；E10 候选仍需下一次异会话独立接收。Debug：Ubuntu-24.04 WSL2、GCC 13.3.0、CMake 3.28.3、`-DCMAKE_BUILD_TYPE=Debug -DMOE_TOPK_ENABLE_EMP_OT=OFF`；Release 仅将 Build Type 换为 `Release`。CPU 为 Intel Core i9-13980HX（32 逻辑 CPU），内存 7,746 MiB。原始日志不进入 Git。

```text
cmake -S VFSS -B /tmp/m6a10-clean-20261003 -DCMAKE_BUILD_TYPE=Debug -DMOE_TOPK_ENABLE_EMP_OT=OFF
cmake --build /tmp/m6a10-clean-20261003 -j8
ulimit -v 786432
MOE_TOPK_M6A_E9_D=128 /tmp/m6a10-clean-20261003/moe_topk_m6a7_aav86_small_conformance_test
MOE_TOPK_M6A_E9_D=128 /tmp/m6a10-clean-20261003/moe_topk_m6a7_aav86_small_differential_test
MOE_TOPK_M6A_E9_D=128 /tmp/m6a10-clean-20261003/moe_topk_m6a7_aav86_small_e2e_test
ctest --test-dir /tmp/m6a10-clean-20261003 --output-on-failure -j4 -R 'moe_topk_(m6a10|m6a7|m2_|m3_|m5_fix_f1)'
```

`ulimit` 在同一 WSL shell 内设置，子进程继承；单独运行上面每条命令时须在对应 shell 重新设置。现有分档测试仍通过历史命名的 `MOE_TOPK_M6A_E9_D` 选择 D；日志 `E9_*` 是测试程序旧打印前缀，不能据此把本次运行冒称 E9 原始结果。`ctest` 在最终源码上 **26/26 PASS**；`verify_e10_vertices_test.py` 为 **14/14 逐轮定义检查 PASS**；相关错误包/重放/超时用例包含在回归内。`git diff --check` 与未跟踪新增文件空白检查见执行记录。

| 文件 | E10 SHA-256 |
|---|---|
| `VFSS/include/moe_topk/protocol_i_aav86_small.h` | `A903519F9C7380C55297FFC77920FCDEE394A79E46807F9AD6A68347B5747C53` |
| `VFSS/src/moe_topk/protocol_i_aav86_small.cpp` | `008643138F48EACAA12C8026C44BF84CF448740CF93975A9788C97A7ED1E95AC` |
| `VFSS/ext/FSS/include/FSS/dcf.h` | `E20A0C7A3E998EA00D7747FC863FFD5D949ADDBEF7392D4BB18F4D1FB3C56F00` |
| `VFSS/ext/FSS/dcf.cpp` | `D53CDE848945784CBB98AD36B845435D858AB364CA857B2924E329F78F02C764` |
| `VFSS/tests/moe_topk/protocol_i_aav86_small_e2e_test.cpp` | `5DB916E9B702E62CE77382F6F0C4A8D33B7C55AAD974B28FFABC2E738E9B20BC` |
| `VFSS/tests/moe_topk/protocol_i_aav86_e10_metrics_conformance_test.cpp` | `82120A3F044B26120203DAFCA9CCD2C0D9A1C17C50C20393E34EE451A59FEAC0` |
| `experiments/m6a_p2_i_allpairs/TEST_ONLY/verify_e10_vertices_test.py` | `51BEEE8E6B1E561506EB8A283F89BEB3632BC24B03B61945FD46D2287D907042` |

原始索引：`experiments/m6a_p2_i_allpairs/TEST_ONLY_E10_2026-10-03/`，含 Debug 的 16/32/64/128 三阶段日志、D128 bounded assessment、Release D128 E2E、构建/回归、E5 图定义和两个 CSV。[24 行 Debug 原始配置](../../experiments/m6a_p2_i_allpairs/TEST_ONLY_E10_2026-10-03/e10_debug_per_configuration_raw.csv) SHA-256 `619B857CF195C53C2B1565E4AC51C40E0695B68A9C9A6E63288AED8FC639BC94`；[24 行容量矩阵](../../experiments/m6a_p2_i_allpairs/TEST_ONLY_E10_2026-10-03/e10_matrix_capacity_checked.csv) SHA-256 `EE4ABD772CB417874AAC7D7AFA58C97731D0E157210B29F3502A411025E8BEA6`；D128 Debug E2E 日志 `1C917CA484CE6E3879059AF7144009CF26612B8EB619EEE1B5C5532C12B56F4D`，Release E2E 日志 `9E8735422A2115C27D81722A56100DF4DF9810F24D429955FF41619E50ED0E0D`。所有日志包含公开 pivot seed，不含密码材料随机币或 key blob。

## 6. 退出判定与剩余范围

| 门 | 结论 |
|---|---|
| E9 可审代码检查点 | GO：28 个显式文件，本地 `142db657...`；原始记录未提交 |
| 传统 DCF 长度倍增 PRG 与 v_A | GO：小 fixture、差分、独立进程逐轮计数通过；覆盖范围限于本入口实际传统 DCF 路径 |
| D128 当前机器受控探测 | GO：需 640–768 MiB `RLIMIT_AS`，本次 768 MiB；Debug/Release 六例各 PASS |
| n≥256 当前完整全池包 | NO-GO：至少硬上限和 64 MiB 单方包上限；没有生成这些配置的材料 |
| 正式 V3 | NOT_DONE：未做 LAN/WAN、1 次预热+5 次重复、n≥256 成功运行或 Protocol III+AAV86 |

允许泄露仍包括公开 masked list、shuffled local ranks、pivot、bucket、活跃图、帧长度及 abort 时点。安全归约仍以 E7/E8 的选阈值单密钥 DCF/FSS、独立随机币、AES/PRG、可信非合谋离线 T 和完整私有通道为条件；测试只验证功能与计数。后续性能工作应在异会话接收本次新源码后再推进，不能用本机小规模时间外推到 n=10^5/10^6。
