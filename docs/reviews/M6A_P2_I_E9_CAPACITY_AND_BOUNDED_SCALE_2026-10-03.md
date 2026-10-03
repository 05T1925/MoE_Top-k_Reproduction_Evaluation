# M6A-P2-I-E9：全两两容量预检与有界规模试运行

日期：2026-10-03。状态：**D=16/32/64 有界功能试运行 PASS；D=128 PRECHECK_LIMITED；正式性能与作者精确复现均未完成。**

## 1. 起点、来源和变更边界

仅在隔离 worktree `C:\Users\28641\Desktop\MoE_Top-k_Reproduction_Evaluation__m6a_p2_i_allpairs_experiment` 工作。起始分支 `codex/m6a-p2-i-allpairs-experiment`，HEAD `c3926c68fd14f270faa8b55234311071947fa080`。开始时 `VFSS/CMakeLists.txt` 与 `docs/IMPLEMENTATION_PLAN.md` 已修改，E1–E8 报告、五个 E7/E8 源码文件及 `experiments/` 均未跟踪；完整 `git status --short` 已在执行聊天记录中保存。没有 checkout、reset、stash、暂存、提交、推送或创建 PR。主工作区、`VFSS-baseline/`、`Papers/`、参考树、已有 E1–E8 文档和日志保持不动。

开始前核对 [E8 异会话接收报告](M6A_P2_I_E8_CROSS_CHAT_RECEIVER_ACCEPTANCE_2026-10-03.md) 的五个 SHA-256，**全部一致**：头文件 `92ABFFED...50836`、源码 `296C25CA...BF85B`、conformance `D4DB5D01...E120`、differential `218E3DD7...AD4B`、E2E `344CB6DE...56DD`。因此本次从 E8 `PASS_WITH_EXPLICIT_LIMITS` 的精确源码状态开始；该签收不自动覆盖下述 E9 改动。

依据 `AGENTS.md`、`PROJECT.md`、实施计划 M6A 门、E6/E7/E8 报告和当前源码，证据分类如下：AAV86/CA 原文是论文定义；E5 明文程序是 `TEST_ONLY` 参考行为；可信 T 每轮预发所有 canonical pair、在线只取活跃边、统一 D、signed Q20.12 稳定 Top-K 与同 π 逆路由是项目扩展；整池 DCF hybrid、AES/PRG 及私有完整通道是条件性安全假设，测试并不证明密码学安全。`AUTHOR_EXACT = NOT_PROVEN`。

## 2. 从实际编码复算的容量与准入

`protocol_i_aav86_small.cpp` 的 writer 对每方写 114 字节固定字段和段计数、每个端点 `1844+8r` 字节、每个 canonical pair 槽 `81+24b` 字节。后者含 24 字节边标签/长度与 `57+24b` 字节 uCMP key blob，`b=33+log2(D)`。因此：

`pairs=C(D,2)`，`slots=r·pairs`，`B(D,r)=114+D(1844+8r)+slots(81+24b)`。

公式经 conformance 对 **D=16/32/64、r=2/5** 的双方实际序列化长度逐例相等验证。`B` 是**单方 wire package**，不等于两份材料的生成期堆内存。当前工程准入预算为 `8B+64 MiB`，包含两份 party 对象、vector/allocator、临时 DCF pair、顺序序列化缓冲和进程余量；本 GNU/glibc Debug 构建的准入判据是预算 ≤256 MiB、可用物理页字节 ≥2×预算、单方包 ≤64 MiB、D≤64、r≤5。8 倍加固定余量是保守工程额度，**不是可移植的 C++ allocator 峰值证明**；超出已测范围维持禁用。D64/r5 的 T 实测峰值 57,612 KiB，低于 143.13 MiB 工程额度。

| D | r | 每轮槽 | 每方总槽 | 每方包字节 | 双方包字节 | 生成预算 |
|---:|---:|---:|---:|---:|---:|---:|
| 16 | 5 | 120 | 600 | 611,658 | 1,223,316 | 68.67 MiB |
| 32 | 5 | 496 | 2,480 | 2,523,042 | 5,046,084 | 83.25 MiB |
| 64 | 5 | 2,016 | 10,080 | 10,372,050 | 20,744,100 | 143.13 MiB |
| 128 | 5 | 8,128 | 40,640 | 42,547,506 | 85,095,012 | 388.61 MiB |

`dealer_generate` 在加锁、随机数初始化、全池 reserve、keygen 和大块分配前调用预检；D/r、pair/slot 乘法、material ID 范围、包和预算均用 checked arithmetic。D>64、r>5、material ID 溢出在测试中预先拒绝。D128 即使单方包未达 64 MiB，也超出 256 MiB 生成预算，故为 **PRECHECK_LIMITED**；没有生成 D128 key，也没有 D128 运行时实测。当前预检实际最大准入 D=64。

本阶段精确修改文件：`VFSS/include/moe_topk/protocol_i_aav86_small.h`（容量结果与接口注释）、`VFSS/src/moe_topk/protocol_i_aav86_small.cpp`（checked 预检、D64 准入及幂运算）、`VFSS/tests/moe_topk/protocol_i_aav86_small_conformance_test.cpp`（包公式/拒绝用例/规模场景）、`VFSS/tests/moe_topk/protocol_i_aav86_small_differential_test.cpp`（规模场景）、`VFSS/tests/moe_topk/protocol_i_aav86_small_e2e_test.cpp`（规模场景和离线/在线/内存/逐轮计量）、新增 `VFSS/tests/moe_topk/protocol_i_aav86_e9_fixtures.h`（TEST_ONLY 定向输入）、`docs/IMPLEMENTATION_PLAN.md`（阶段门）及本报告。`VFSS/CMakeLists.txt` 的已存修改属于 E7，不是 E9 新改动。TEST_ONLY 原始日志和 CSV 是未跟踪本地证据，绝不暂存为源码差异。

## 3. 验证顺序、入口和实际结果

新 WSL Ubuntu-24.04 Debug 构建目录 `/tmp/m6a9-clean-20261003`，CMake 3.28.3，GCC 13.3.0，x86_64 WSL2 6.6.87.2，Intel Core i9-13980HX，32 逻辑 CPU，内存约 7,746 MiB。CMakeCache 记录 `CMAKE_CXX_FLAGS=""`、`CMAKE_CXX_FLAGS_DEBUG="-g"`、`MOE_TOPK_ENABLE_EMP_OT=OFF`；目标另受仓库 CMake 的 OpenMP 等链接/编译配置影响。配置命令 `cmake -S VFSS -B /tmp/m6a9-clean-20261003 -DCMAKE_BUILD_TYPE=Debug`；随后 `cmake --build /tmp/m6a9-clean-20261003 -j4 --target moe_topk_m6a7_aav86_small_conformance_test moe_topk_m6a7_aav86_small_differential_test moe_topk_m6a7_aav86_small_e2e_test`。预热 **0**、重复 **1**；本表是 Debug 工程试运行，不是 LAN/WAN 五次矩阵。

每档按 conformance → frozen oracle differential → T/P0/P1 fork+exec E2E 执行；环境变量 `MOE_TOPK_M6A_E9_D=16`，再依序改为 `32`、`64`，后接上述各可执行文件。每档各有六个场景：`n=D-1` 与 `n=D`；`r=2/5`；`K=1/中间/n`；同分、重复值、负数、INT32_MIN/MAX 和固定输入种子 `0x9e3779b97f4a7c15 XOR D` 的 xorshift 序列。份额拆分种子为 `0x770000+serial`，仅作用于 TEST_ONLY harness。密码材料随机币不记录。pivot seed 为公开协议币，见原始 E2E 日志。

第一次构建命令误写了不存在的 `protocol_i_aav86_small_conformance_test` 目标，CMake 回报 `No rule to make target`，未进入编译；改用 CMakeLists 中真实的 `moe_topk_m6a7_aav86_small_*_test` 目标后干净编译通过。初轮 E2E 通过后增加 T 分段/RSS 与逐轮计量字段，重新构建并依序重跑 D16/32/64 的 E2E，下面的表和原始文件只取最后一次结果。

| D | conformance | differential | fork+exec E2E | GO/NO-GO |
|---:|---:|---:|---:|---|
| 16 | 6/6 PASS | 6/6 PASS | 6/6 PASS | GO |
| 32 | 6/6 PASS | 6/6 PASS | 6/6 PASS | GO |
| 64 | 6/6 PASS | 6/6 PASS | 6/6 PASS | GO |
| 128 | 未运行 | 未运行 | 未运行 | PRECHECK_LIMITED |

下面是最终 E2E 一次运行的逐配置原始摘要；时间单位 ms，RSS 单位 KiB。逐轮活跃边、P0/P1 各自时间/RSS、dealer 生成/序列化/分发时间、双方收发字节、公开 pivot seed 及 edge digest 见 [18 行原始 CSV](../../experiments/m6a_p2_i_allpairs/TEST_ONLY_E9_2026-10-03/e9_per_configuration_raw.csv) 与同目录逐档日志。通信 total 只合计双方发送量；接收量仅对账。

| D | n | K | r | 每方预留 | 活跃边 | 每方包 B | 每方在线发送 | T 离线 | 在线 max | T 峰值 |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
|16|15|1|2|240|120|262,434|1,664|15.98|5.89|5,376|
|16|16|16|5|600|52|611,658|2,720|28.55|5.87|7,292|
|16|15|7|5|600|47|611,658|2,720|27.35|4.59|7,288|
|16|16|1|5|600|62|611,658|2,720|27.29|5.78|7,292|
|16|15|15|2|240|57|262,434|1,664|12.59|3.01|5,376|
|16|16|8|2|240|87|262,434|1,664|12.36|3.55|5,376|
|32|31|1|2|992|239|1,044,690|2,944|49.43|7.67|9,944|
|32|32|32|5|2,480|163|2,523,042|4,768|106.69|6.28|17,308|
|32|31|15|5|2,480|158|2,523,042|4,768|105.94|6.69|17,304|
|32|32|1|5|2,480|170|2,523,042|4,768|112.06|7.96|17,056|
|32|31|31|2|992|261|1,044,690|2,944|49.06|7.64|9,944|
|32|32|16|2|992|234|1,044,690|2,944|47.27|7.89|9,944|
|64|63|1|2|4,032|747|4,219,698|5,504|189.32|17.93|27,764|
|64|64|64|5|10,080|567|10,372,050|8,864|447.70|21.60|57,612|
|64|63|31|5|10,080|501|10,372,050|8,864|442.67|18.47|57,516|
|64|64|1|5|10,080|403|10,372,050|8,864|439.83|18.90|57,612|
|64|63|63|2|4,032|785|4,219,698|5,504|187.72|18.72|27,768|
|64|64|32|2|4,032|821|4,219,698|5,504|180.47|18.12|27,764|

每例 E2E 均检查 TEST_ONLY 重构后原序 bit-mask 恰有 K 个 1 并与冻结 oracle 一致。P0/P1 secure 函数仅返回 XOR shares。E2E 逐次检查实际 `message_trace` 的 phase 顺序与 sent/received 累计，双方 trace 与逐轮 active edge 一致，不只相信 `causal_rounds` 值。`score` 两轮，CA 核心 `2r+1`（初始 shuffle，随后每迭代 masked open/rank reveal），同 π inverse 一轮；因此**仅此 raw-score→原序 XOR mask 独立入口**为 `2r+4` 因果轮。T fork+exec 后只保留双方离线 package fd 和向 TEST_ONLY parent 回报计量的离线 telemetry fd；T 退出后 parent 才发送 score shares，T 不收在线输入、rank 或输出。测试保留错方、错参数、截断/尾随、同规格 blob 互换的 dealer 双份一致性检测、持久重启 replay、静默 peer 超时和关闭通道失败关闭用例。完整通道及可信 T 是模型前提；结构字段校验不是恶意方认证。

相关旧 Protocol I/III 回归：`cmake --build /tmp/m6a9-clean-20261003 -j8` 全量编译通过；`ctest --test-dir /tmp/m6a9-clean-20261003 --output-on-failure -j4 -R 'moe_topk_(m6a7|m2_|m3_|m5_fix_f1)'` **25/25 PASS**。原始 build/ctest 日志留在同一 `TEST_ONLY_E9_2026-10-03/` 目录，不纳入拟提交差异。

## 4. 来源身份、边界与下一门

E9 六个源码/测试文件的最终 SHA-256：

| 文件 | SHA-256 |
|---|---|
| `protocol_i_aav86_small.h` | `A7A4642BBA588A343A99D9B89FFA4EB79A5E1DC0FA99AFA5530128DEAB790785` |
| `protocol_i_aav86_small.cpp` | `DA2537FF9007B64C18CBB91374A5EEC1BD621549218E467A974F8449658DFAFC` |
| `protocol_i_aav86_small_conformance_test.cpp` | `AE0815470FE62F0B4E8C748AAE3F9DC979BD5F9520EDF75D5A4797B405934045` |
| `protocol_i_aav86_small_differential_test.cpp` | `855325D2FC53F79520FC0F6EC4C523ABF18D7E7323E8FD6C71A2CF144F862502` |
| `protocol_i_aav86_small_e2e_test.cpp` | `FB72FC5F69F08B31A8E47311A8F082FAEF857CF5F53DBA5FB6A146862B29E831` |
| `protocol_i_aav86_e9_fixtures.h` | `4C0CCF9EF57F91C7700A76D13084F98227A35EB54011A628A198552D0CB2EF87` |

条件性单方安全仍依赖 E7/E8 明列的选阈值单密钥 DCF/FSS 隐私、每边独立随机币、AES/PRG、可信非合谋离线 T 和私有完整通道。允许泄露包含公开 masked list、shuffled local ranks、pivot、bucket、活跃图、消息长度及 abort 时点；逐轮图和材料查表顺序不可另称为秘密。小 D 接收的安全归约没有因为 D=64 功能测试而变为形式证明。`AUTHOR_EXACT = NOT_PROVEN`，Protocol III 和正式大规模性能矩阵未覆盖。

PRG/AES 内部调用次数、系统性 CPU 计数、网络 LAN/WAN 延迟及正式重复分布为 `NOT_MEASURED`。下一步若开始正式 Protocol I 性能矩阵，需在 FSS/PRG 内部增加可信计数或明确标记 `NOT_MEASURED`，固定网络与硬件环境、Release 构建、预热及五次重复，并由异会话接收方重新核对 E9 精确源码。D128 需先独立修订并论证生成期内存准入，不能以 D64 活跃边上界外推或直接解除当前限制。固定 M、在线补料、Protocol III 均未进入本次实施。
