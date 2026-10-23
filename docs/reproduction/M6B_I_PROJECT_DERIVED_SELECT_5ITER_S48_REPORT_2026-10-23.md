# M6B-I-PROJECT-SELECT-5ITER S48 独立接收报告

日期：2026-10-23
路线：**Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL**

目标功能：输入原始顺序的 signed Q20.12 分数，按分数降序、原始索引升序稳定打破同分，输出原序下恰有 K 个 1 的两方 XOR 共享 Top-K mask。这是项目派生的五迭代 Select，不是 BB90 原算法复现、BMW16 实现或完整安全通过。五次算法迭代、比较依赖层、应用 exchange phase 和网络 RTT 是不同量。

## 1. 裁决摘要

- 从指定 S47 提交独立复建并接收，没有从主工作区或近似分支替换源码。初始 HEAD/tree/parent 为 `feb5e6677568d649906f46d0e611d5d52349c07f` / `7c494b5cfcc67843410e683556a1792074d0ca1e` / `d48a41062ba2f86edbac3bcb3663692a641e95bd`。S47 原 worktree 最终只读 `git status` 为 clean。桌面主 worktree 终检存在未提交/未跟踪历史内容；S48 的构建和编辑只在隔离 S48 worktree 执行，没有在主工作区或 S47 worktree 写入。`VFSS-baseline/` 无差异。
- 本轮 fresh S48 Release E2E 二进制重新运行了 S47 定义的 10 个 case，再加 3 个固定 seed/结构的 N=1000 case。13/13 oracle、双方提交与恢复通过，范围为 `PASS_WITH_SCOPE`。N=1000 请求的 K 集合 `{1,8,500,999,1000}` 全覆盖；另有 N=1、FTv3 N=128/256、FTv1 和 FTv2 兼容 smoke。不是全 N/K 矩阵。
- default-OFF Release 库没有 v3/Select5 target/symbol，头文件/API 在未 opt-in 时拒绝；显式 opt-in Release 成功，production archive 不含 TEST_ONLY 随机源 hook。`DEFAULT_OFF_ISOLATION=PASS_WITH_SCOPE`。
- F_T/v3 达重试 cap 的确定性发行失败验证通过：发行输出路径中 package/context/escrow DB/party lease DB 未创建，私有 burn registry 留有一个 marker，同 invocation 重放被拒且 marker 字节不变。因 A/P0/P1 没有启动，在线数据库表行状态为 `NOT_RUN`，不把“服务未启动”改写成“在线状态机查表为零行”。
- N=1000 目标本机内存、磁盘、材料包及最大 payload 检查通过。所有这些数值是功能/资源诊断；不是正式性能或 RTT 数据。
- 低概率 `ISSUANCE_ABORT` 的产品接受仍为 `OPEN_CONFIRMATION`；目标部署 DRBG、认证私有信道及完整 party-view/composition 仍 OPEN。N>1000 `NOT_ADMITTED`；正式 LAN/WAN、RTT、性能及 AAV86 对比均未运行/测量；未创建 `secure` alias。

## 2. 来源、工作区与独立性

S48 在隔离 worktree `C:\Users\28641\.codex\worktrees\m6b-i-project-select5-s48\MoE_Top-k_Reproduction_Evaluation` 的分支 `codex/m6b-i-project-select5-s48` 工作。起始工作区 clean，HEAD/tree/parent 如上。S47 source/build/evidence manifest 当前 checkout raw SHA-256 为 `3e4be30fdbd0c0463b39ad1ec8dcd1893a096f7bc92d35c6f86b850a936480ef`；其中 19 项 S47 source-delta 记录值的 canonical 摘要重算为 `5e70ff463f6e8593604fef320953e0df59ab2762bae1b39fb32c8a7fb1f70f7c`，与 S47 manifest 一致。建 worktree 后、修改测试源前，19/19 source-delta Git blob 均与 S47 manifest 相符。5 份新增 S47 文档的当前 checkout raw SHA 与历史 raw SHA 不同，是 Windows 换行字节转换；对应 Git blob 身份相同。S48 修改两个测试文件后，它们自然不再是 S47 文件哈希，S48 清单另外记录新身份。

S47 原 worktree `C:\Users\28641\.codex\worktrees\m6b-i-project-select5-s47\MoE_Top-k_Reproduction_Evaluation` 的最终只读 `git status --porcelain -uall` 为空。桌面主 worktree 终检有 24 条未提交/未跟踪状态项（5 个 tracked modifications、19 个 untracked paths）；S48 未在该 worktree 执行写入，且由于没有保存任务开始时的逐项 status 快照，不推断其具体创建时间。两次 S48 只读检查之间，状态数量仍为 24，但 `.agents/skills/protocol-reproduction/SKILL.md` 的 porcelain 标记从暂存修改变为未暂存修改，其余条目一致；来源无法判定，S48 未尝试恢复或暂存该文件。该差异和两次状态摘要均记录在 manifest。S48 未修改 `VFSS-baseline/`、S47 decision/report/gates/cases/manifest，也没有提交构建物、密钥、材料、证书或原始大日志。

执行者参与过 S45/S46 的项目推进，不将自测称为独立接收。非作者 `/root/s40_independent_receiver` 未参与 S47 实现或 S47 证据生成；此前只做过 S40–S42 只读审查。它从 S48 最终测试源 fresh-build 并独立运行更新后的 bounded conformance、FTv3 定向 E2E（其中包含 5 个指定 N=1000/K 案）、3 个新增案和无注入透明 trace；独立 reviewer 没有运行 runtime conformance，也没有重跑 FTv1/v2 兼容 smoke。其独立 Release 二进制 SHA-256 为 bounded `726258065bbad1a6818ad83881efa9d137a9530307168d1febb044bfcd9ccf5d`、process E2E `21a6cb4aefddb4ee71b1d29e78ece8b6e408f0098906aa9e38bb64330ed1774e`。Reviewer 确认失败用例只应支持 Dealer 发行前边界，不应声称 A/party 数据库已被运行或查询。

## 3. 构建与默认关闭隔离

本轮执行环境为 WSL2 Ubuntu 24.04，Linux kernel `6.6.87.2-microsoft-standard-WSL2`，CMake 3.28.3、Ninja 1.11.1、GCC 13.3.0、OpenSSL 3.0.13。该构建主机配置不等于项目目标部署的 provider/熵源配置。

### 3.1 Release OFF

全新目录 `/tmp/m6b-s48-default-off` 配置为 Release、`BUILD_TESTING=OFF`，四个 GMW/receipt/FTv2/FTv3 cache 选项均为 OFF；构建目标 `sytorch` 成功。`ninja -t targets all` 中 v3/Select5 target 数为 0；`nm -g --defined-only libsytorch.a` 匹配 `score_to_mask_v3|ProjectDerivedSelect5` 的符号数为 0。编译单头 API probe 的预期结果是 `score-to-mask-v3 is opt-in and must be built explicitly`。

### 3.2 显式 FTv3

- Release production 配置同时显式开启 GMW/v3、receipt/v2 与 F_T/v3，`BUILD_TESTING=OFF`；`sytorch` 构建成功，archive 含 FTv3/Five-Select 符号，不含 `with_test_random` 随机源 hook。
- F_T/v3 开而 GMW 关闭、或未开 receipt/v2 的 CMake 负配置均按预期拒绝，错误分别为 `Receipt/v2 requires ... GMW...=ON` 与 `F_T/v3 requires GMW/v3 and receipt/v2 opt-ins`。
- TEST_ONLY Release 配置另建，`BUILD_TESTING=ON`，启用兼容 smoke 所需的 FTv2 和 F_T/v3。只构建 bounded conformance、v3 runtime conformance 与 FTv3 process E2E 三个目标；本轮没有跑全仓测试。
- FTv3 package/sampler/context 字段和 receipt-format version 由 conformance 直接核对：F_T=3、sampler=2、λ=128、retry cap=7、package format=6、context format=2、receipt format=2；错误 sampler/receipt 字段被拒绝。FTv1/receipt-v1 与 FTv2/receipt-v2 兼容 smoke 分开执行。

本轮 production archive SHA-256：OFF `7691eae29478bbb2fa7462aff6690558b81ee73de3dd70b4d4e9eb30e7610127`；显式 FTv3 production `76b999f7bc5400da9c72f1c35eaaa44cb46cef6a8d4f32b5ea27407a6e2d76e1`。TEST_ONLY `libsytorch.a` 为 `fb6c6d5fe5321441af2f7779ab73fe68700fab3dffa33842bb7b0c056dc8b8c0`。最终执行者本地二进制：bounded conformance `fd2d51990bc0bf5c024574cb0c3c58ac6073eda7dd955e8abad4d0fc0ed496db`；runtime conformance `5dac078ce3f6634009f79b3510e84fb6026072b188bdcd73a35228557731b660`；process E2E `bc29468dadfd344985c6f2138cdd4a7242578da7aed5a4ad63023a2083de0060`。独立 reviewer 使用单独 build path/命令生成的二进制哈希另列，不与本机二进制身份混用。

## 4. N=1000 功能复跑

所有 13 个案例均由 S48 的 FTv3/receipt-v2 Release TEST_ONLY 二进制新跑；详细 input hash、case log tree hash、package、退出码和资源数据见 [逐案例 JSON](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S48_CASES_2026-10-23.json)。

| 来源 | 案例 | 结果 |
|---|---:|---|
| S47 固定案例定义，S48 新执行 | N=1000，K={1,8,500,999,1000}（5 案） | 5/5 oracle PASS；Dealer/P0/P1 exit 0；双方 COMMITTED、各自返回 share、重启恢复 PASS |
| S47 固定案例定义，S48 新执行 | FTv3 N=1/K=1、N=128/K=64、N=256/K=128（3 案） | 3/3 PASS |
| S47 固定案例定义，S48 新执行 | FTv1/receipt-v1 N=128/K=64；FTv2/receipt-v2 N=256/K=128（2 案） | 2/2 兼容 smoke PASS |
| S48 新增 | 固定 `mt19937` seed `0x48c0ffee`/K=500 混合极值、随机值和重复片段；交错排名含端点极值/K=1；42/-17 大量同分/K=999 | 3/3 oracle PASS，双方 API/恢复 PASS |

所有功能案例的 scores 是有符号 int32 表示的 Q20.12 固定点值；测试覆盖 `INT32_MIN`、`INT32_MAX`、负值、零、重复分数、大量 tie 及首尾排名交错。测试 oracle 按 signed 分数降序、索引升序稳定排序；运行器核对重构 XOR mask 与冻结 C++ oracle、mask 长度 N、元素为 bit、恰 K 个 1 及原始顺序。每方只通过 API 返回自己的 share，不能把重构 mask 当作任一方输出。

这是一组有代表性的固定案例，不是全分布或穷尽测试。S40 的 211 案、S35 的 195 案和 S46 的旧配置 10 案没有重跑；旧结果没有重标成 S48/F_Tv3 结果。

## 5. 有界发材失败状态与持久副作用

有界 sampler conformance 使用 TEST_ONLY 字节源，对 N=1000/M=1024 的首个 Fisher–Yates 拒绝位置强制连续达到 profile cap=7。测试断言随机调用量和字节量不超过该失败执行的预期边界：88,757,792 字节、94 次随机请求、最大单次 1,048,576 字节；成功路径总上界仍是 `Qmax=88,786,404 B`、`Cmax=7,247`。失败的 summary 行是：

固定 cap 的源码映射也做了直接核对：生产发材入口 `score_to_mask_v3_dealer_issue_packages_ft_v3_bounded` 与 TEST_ONLY 发材入口 `score_to_mask_v3_dealer_issue_packages_ft_v3_with_test_random` 都把公开常量 `kScoreToMaskV3FtV3MaxRetryLimit`（值 7）写入 `PackageProfile`；package/context 校验继续要求该固定字段，且验证 `minimum_retry <= cap`。TEST_ONLY 用可控字节源强制达到 cap，不使用小于生产规格的重试次数。该实现映射检查不证明真实 OpenSSL DRBG 输出流安全。

```text
S48_FT_V3_BOUNDED_CONFORMANCE PASS gates=88626688 Qmax=88786404 Cmax=7247 issuance_abort=PASS
S48_ABORT_STATE escrow_db=NOT_CREATED p0_lease_db=NOT_CREATED p1_lease_db=NOT_CREATED online_services=NOT_STARTED package_files=0 context_files=0 burn_markers=1 replay_rejected=true
```

此处直接观察到：发行输出根目录没有 party package、signed context、escrow DB 或 P0/P1 lease 目录；私有 `0700` registry 有且只有一个可读 burn marker；同一 session/invocation 重放返回 `already issued or burned`，没有 package/context，marker 字节保持不变。源码顺序是先登记 burn marker，再采样/构材；package/context 的 durable create 仅在之后的成功路径发生。这个证据说明失败调用的离线 T 没有发布 package/context，且同 registry 下 invocation 被烧毁。

A、P0、P1 进程没有在这项发材失败测试中启动；因此没有 A transaction、ciphertext、receipt 或 party lease 表可供该用例查询。测试通过检查这些数据库/目录路径未创建来核实未发生在线持久化，不声称观察了实际在线数据库的零行，也不证明临时内存清零、跨库回滚或掉电持久性。正常成功路径 A/party 持久状态另由下面的无注入 E2E trace 验证。低概率合法输入 `ISSUANCE_ABORT` 是否可接受仍 `OPEN_CONFIRMATION`，不改变已确认的成功交付合同。

## 6. 正常通信与 frame/资源边界

使用透明代理、无 failpoint、N=1000/K=500 的正常 FTv3/receipt-v2 流程重新运行。Dealer/P0/P1 exit 0；双方 API 为 `committed/share-returned`，A 有一笔 COMMITTED transaction 与两条密文 share row；P0/P1 lease 都 burned，成功后两方各自 fetch/recovery 路径可用。escrow test server exit 143 是 harness 在成功后发送 SIGTERM 的清理结果，不是业务进程异常退出。

代理 trace 共 424 条记录（P0/P1 与代理两个方向各覆盖 106 次 frame write/read），phase 1–106、sequence 0–105。每条 trace 都满足 Protocol-I frame header 56 B，complete protocol frame = payload + 56 B；最大 payload `1,048,572 B`，最大 Protocol-I frame `1,048,628 B`。这是 TLS 前 Protocol-I 帧长，TLS record 封装开销不在此计数。

transport 源码的 `kMaxPayload=1<<20` 对 payload 生效，frame header 是单独写出的 56 B。FTv3 固定批量最多 4,193,824 gates：内层上下文 104 B，batch metadata 12 B，opening bits `2×4,193,824/8=1,048,456 B`，合计 payload `1,048,572 B`，比 1 MiB payload cap 少 4 B。该 4 B 留空来自 gate batch 边界与 bit packing；它不是外层 framing header 造成的。加上 56 B outer header 得到上述 1,048,628 B。其他 N=1000 用例观察到 payload 上限相同；源码 send/receive 对大于 1 MiB payload 明确拒绝，batch cap 同时阻止 FTv3 合法最大批次越过此约束。

| 资源项 | N=1000 实测/推导 |
|---|---:|
| padded `m` / gate count | 1024 / 88,626,688 |
| F_T/v3 材料 payload / party package | 33,366,080 B / 33,366,516 B |
| 两个 party package 合计 | 66,733,032 B |
| 每个 N=1000 case temporary directory | 66,794,349 B |
| normal proxy trace case directory | 66,826,470 B |
| 最大 party RSS（测试过程观测） | P0 986,228 KiB；P1 986,236 KiB |
| 该 case 两方 RSS 合计峰值 | 1,972,464 KiB |
| observed party exchange phase | 本机 96–106；normal trace 双方各 106 |

随机消耗从 S47 的 N=1000 profile 重新核对：

```text
G      = 88,626,688
Qmax   = G + ceil(1024^2/8) + 4*7*(1024-1) = 88,786,404 bytes
Cmax   = ceil(G/2^20) + 1 + 7*(1024-1) = 7,247 calls
request size <= 1 MiB
delta_reject <= 1023*2^-154 < 2^-144 < 2^-128
```

最后一项仅适用于 S47 定义的理想 IID tape 下语义材料投影；不适用于带版本、签名、编码的完整 package/transcript，也不能替代 `Adv_DRBG_T(Qmax,Cmax;E)`。本机 preflight 曾观察到总 RAM 约 8.12 GB、可用 RAM 约 7.50 GB、swap 2.00 GiB、`/tmp` 可用空间约 983.7 GB；最终重跑后的再检查为 32 vCPU、可用 RAM 约 7.46 GB、swap 空闲 2.00 GiB、`/tmp` 可用空间约 982.4 GB。N=1000 的目标案例未触及这些本机资源限制。

RSS、package/file/frame 字节和 phase 是本机功能/资源诊断。phase 不是算法五次迭代数，也不是比较依赖深度或 RTT；wall-clock 诊断时间没有作为性能结果发布。

## 7. 证据分类与安全/性能边界

- **论文定义：** 本轮未通过本地实现或测试反向声称论文等价；不宣称复现 BB90 原算法或实现 BMW16。
- **本地参考行为：** 冻结 C++ oracle 提供本项目的 signed Q20.12 稳定 Top-K mask 比较基准。
- **项目派生设计与本地代码事实：** FTv3/Beaver-v3 扩展只在 opt-in 下支持到 N=1000；五迭代 Select、同分稳定规则、原序输出、party share API 与双密文共同提交合同保持 S47 决策。13 个定向案例通过。
- **待验证假设：** 真实部署的 OpenSSL provider/熵源/重播种/并发/fork/restart；整条 RAND_bytes 输出流与理想随机带的联合优势；认证私有信道；GMW/Beaver 两方完整视图模拟；secret shuffle/inverse、输出刷新与 escrow 的组合；有效输入下低概率发材中止合同接受。

| 门 | 状态 | 说明 |
|---|---|---|
| `S47_SOURCE_RECEIPT` | `PASS_WITH_SCOPE` | 精确 S47 commit/tree/manifest 已核对；S48 source delta 另记 |
| `FTV3_N1000_FUNCTIONAL_RECEIPT` | `PASS_WITH_SCOPE` | 13 个目标/兼容定向案例，非全矩阵 |
| `DEFAULT_OFF_ISOLATION` | `PASS_WITH_SCOPE` | 本轮 OFF Release 库、symbols、targets 与 header probe |
| `FTV3_ISSUANCE_ABORT_PREPUBLICATION` | `PASS_WITH_SCOPE` | T-only prepublication; A/P0/P1 未启动 |
| `ONLINE_DB_ABORT_ROW_STATE` | `NOT_RUN` | 失败案例没有启动在线服务，因此没有表可查 |
| `N1000_RESOURCE_FRAME_DIAGNOSTIC` | `PASS_WITH_SCOPE` | 同机 RSS、目录、package 和 payload/frame trace |
| `LOW_PROBABILITY_ISSUANCE_ABORT_ACCEPTANCE` | `OPEN_CONFIRMATION` | 未见负责人对合法输入偶发中止的正式接受记录 |
| `DRBG_DEPLOYMENT_PROFILE` / `JOINT_RNG_STREAM_BRIDGE` | `OPEN` | 部署 provider/熵源/lifecycle 与联合流优势缺证 |
| `PRIVATE_CHANNEL_BRIDGE` / `PARTY_VIEW_COMPOSITION` | `OPEN` | 不由功能或 trace 测试关闭 |
| `SHUFFLE_OUTPUT_ESCROW_COMPOSITION` | `OPEN` | 完整条件模拟未完成 |
| `N_GT_1000` | `NOT_ADMITTED` | 不外推本轮数据 |
| `NATURAL_BUSINESS_IO_FAULT` | `NOT_REPRODUCED` | 正常路径通过；尾部 EOF 是成功后的 teardown |
| `FORMAL_LAN_WAN` / `RTT` / `AAV86` | `NOT_RUN` / `NOT_MEASURED` | 正式性能比较后置 |
| `SECURE_ALIAS_READY` / `PERFORMANCE_READY` | `NOT_READY` | v3 保持默认关闭；无 secure alias |

## 8. 失败尝试、继承项与下一步

本轮保留的非源码错误/修正记录：

1. 第一次 PowerShell 嵌套命令展开了 `$SRC`，CMake 得到错误 source path；改为固定 WSL worktree path 后重跑成功。
2. 首次默认 OFF 构建误用全量 target；在 180 个 target 的 65/180 处主动终止，然后只构建接收所需的 `sytorch`，不是编译失败。
3. 一个包装命令没有正确读取 shell pipeline exit status；随后用直接重定向及返回码重跑所有目标。
4. 首次代理 trace 把正常成功后读取下一业务帧时的 TLS EOF 放进一般 proxy error；扩充 TEST_ONLY harness，仅在两方 exit 0 且 A 已有 COMMITTED/两条 share row 时，单列为 `post_success_teardown_eof`。最终 trace 的业务 proxy error 为空。
5. 初稿 bounded test 预先建了空 SQLite 表，再报告零行。这不能证明运行时在线 DB 状态，已删除该误导性断言，改为检查发行失败实际输出目录与 registry。独立 reviewer 复核改后边界，确认它不声称在线数据库已启动/查询。
6. expected negative controls：OFF header probe 编译失败；FTv3 缺 GMW 或缺 receipt/v2 的 CMake 配置失败。这些是合同预期，不是构建缺陷。

S35 195、S40 211、S46 旧 FTv2 十案例矩阵本轮未运行；历史证据只保持其原始配置。自然业务 I/O fault 仍 `NOT_REPRODUCED`。人工 SIGKILL/进程 kill 注入本轮未运行，也不作为正常算法崩溃证据。

最短后续路线：先由项目负责人明确 `OPEN_CONFIRMATION` 的低概率预处理 abort 接受状态；为真实部署提供 OpenSSL 版本/provider/`OSSL_LIB_CTX` 配置、熵源和 seed/reseed 行为、线程/并发/fork/restart 生命周期以及失败选择模型，才能审查 DRBG 联合流桥；之后继续认证私有信道和两方完整视图/ shuffle-output-escrow composition。仅当安全和接收门另有明确状态后，再统一服务器、构建、输入、seed、重复次数和计量边界，启动正式 LAN/WAN 与 AAV86 比较。

## 9. 本轮变更文件

本轮只增加 TEST_ONLY 诊断与证据文档，没有改 `VFSS/src/` runtime：

- `VFSS/tests/moe_topk/score_to_mask_v3_ft_v3_bounded_conformance_test.cpp`
- `VFSS/tests/moe_topk/score_to_mask_v3_process_e2e_test.cpp`
- `docs/decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S48_DECISION_2026-10-23.md`
- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S48_REPORT_2026-10-23.md`
- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S48_GATES_2026-10-23.json`
- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S48_CASES_2026-10-23.json`
- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S48_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-23.json`
- `PROJECT.md`
- `docs/IMPLEMENTATION_PLAN.md`
- `docs/REFERENCE_MANIFEST.md`

只创建本地隔离提交，不推送、不合并、不创建 PR。
