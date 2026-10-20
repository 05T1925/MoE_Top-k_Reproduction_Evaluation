# M6B-I-PROJECT-SELECT-5ITER-S32 实施与验证报告

日期：2026-10-20
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 1. 结论摘要

依照 S31 已接收的规格，在隔离 worktree 增加了默认关闭、明确版本化的 GMW/Beaver v3 API、T 的一次性材料签发、P0/P1 电路执行和 SQLite escrow A。ON-only Release 构建中的两个 v3 专项 CTest 通过；195 个真实进程 E2E 案例通过，覆盖所有 K 的 N=3 三值输入全集、N=4 的全部 24 种分数排列与所有 K、N=128/N=256 实际资源点及三种 escrow 故障/恢复前缀。OFF 构建未包含 v3 符号/目标，未启用宏时直接包含 v3 头文件会被编译拒绝。

**准入结论仅为 opt-in 实验功能通过所列功能与故障检查。**P0/P1/A 完整视图模拟、Dealer preprocessing 联合分布、CSPRNG/TLS 优势量化、外部存储系统故障安全和 S32 实现的独立审查均未完成；不得称为密码学安全通过、生产安全或 BB90/BMW16 复现。正式 LAN/WAN 性能矩阵 `NOT_RUN`。

## 2. 身份与证据分类

S25 原始 worktree `C:\Users\28641\.codex\worktrees\m6b-project-select5-s25\MoE_Top-k_Reproduction_Evaluation` 按其 S25 manifest 重新计算 74 个文件，74/74 哈希匹配、0 个不匹配，canonical SHA-256=`64f32993b12bcd8692ed089f5dcdb8ce1be9f181ff217b508b8b0470eb8e440d`。S32 在新的 `codex/m6b-i-project-select5-s32` worktree、base HEAD `c3926c68fd14f270faa8b55234311071947fa080` 工作；没有把另一个活动分支当成 S25 被审对象。S32 worktree 中逐项重算这 74 个清单成员后，69 个哈希仍与 S25 相同，5 个是本轮有意增量：`PROJECT.md`、`VFSS/CMakeLists.txt`、`VFSS/include/moe_topk/protocol_i_transport.h`、`VFSS/src/moe_topk/protocol_i_transport.cpp`、`docs/IMPLEMENTATION_PLAN.md`。原 S25 worktree 保持只读。S31 source/test/evidence manifest 与其中列出的 TEST_ONLY 源码、决策、报告、gates 哈希均匹配。

证据严格分层：

| 类别 | S32 使用方式 |
| --- | --- |
| 论文定义 | 此路线是项目派生的 Select，不是 BB90 原算法复现；不继承 BB90/BMW16 的步骤、比较常数或安全定理。 |
| 本地代码行为 | S25 精确快照及 S31 TEST_ONLY 模型只说明其各自版本的本地行为；S27 `PASS_WITH_SCOPE` 是历史功能接收，不是本轮测试。 |
| 项目派生设计 | 五次稳定 Select、Q20.12 signed 分数、GMW/Beaver-v3、`F_TOPK-U` 与 escrow `F_TOPK-ATOMIC` 按 S31 合同实现。 |
| 待验证设想 | 完整模拟、T 的联合材料分布、安全归约、优势项和远程 A 的生产运维安全均未验证。 |

## 3. 成功输出与失败合同

对输入 signed raw Q20.12 分数 `x`，理想 Top-K mask `M` 是长度 N 的原输入序列：分数降序，分数相同按原始索引升序，恰有 K 个 1。v3 成功输出沿用 S31 `F_TOPK-U`：P0 的 share 是均匀随机 `R∈{0,1}^N`，P1 的 share 是 `R XOR M`。重构得到 M；任一 Party 仅有本方均匀 share。

`score_to_mask_v3_party.cpp` 最终 output-refresh 阶段由 P0 用 OpenSSL `RAND_bytes` 取 R，并经认证 peer channel 将 `R XOR local_mask_share_0` 发给 P1；P1 与自己的 circuit share 合成 `R XOR M`。API 只在收到 A 的 32-byte commit receipt 后返回 `kCommitted` 和本地 share。

SQLite A 把两份收件人密文、摘要、session/invocation、N/K、电路与 gate-schedule hash，以及 `state=COMMITTED`/共同 SHA-256 receipt 放入一个 `BEGIN IMMEDIATE`/`COMMIT` 事务；数据库启用 WAL 与 `synchronous=FULL`。正常路径证明的是单进程崩溃/重启下的 SQLite 事务边界，不是掉电、文件系统损坏或硬件持久化保证。提交时刻由 A 的耐久事务线性化；两个 OS 进程读取 receipt 并返回 share 的时刻可能不同，系统不声称物理同时返回。

| 前缀 | API 可见状态 | A 状态与恢复 |
| --- | --- | --- |
| 运行时/提交前错误，A 尚无两份密文 | 两边不返回 share；测试中的恢复不确定状态是 `RECOVERY_REQUIRED`。内部 wire/share、已发送密文及 lease 已消费，不宣称“本机没有 share”。 | 若 A 只收到一份，重启或事务等待超时将其耐久标记 ABORTED；双方 fetch 拒绝。 |
| 两份密文已暂存但尚未耐久提交 | 两边 API 均没有 share。 | 本轮注入 A `SIGKILL` 后重启，确认 pending→ABORTED；timeout 路径也单独验证 ABORTED。 |
| A 已耐久提交但执行方丢失 receipt | 该方返回 `RECOVERY_REQUIRED` 和事务 ID，不返回 share、不谎报 ABORT；另一方可已收到并返回自己的 share。 | A 重启后两个对应收件方均能 fetch 密文与相同 receipt，再仅由自己的私钥解密。 |
| 已提交并完成正常返回 | 双方各自得到一个 share。 | A 重启后仍能仅向对应身份提供密文；双方都恢复出与 API share 一致的数据。 |

## 4. v3 接口与实现范围

新增 [score_to_mask_v3_gmw.h](../../VFSS/include/moe_topk/score_to_mask_v3_gmw.h)、`score_to_mask_v3_gmw.cpp`、`score_to_mask_v3_party.cpp` 和 `score_to_mask_v3_escrow.cpp`。头文件未定义显式 CMake opt-in 宏时以 `#error` 拒绝使用。

- **T / 离线材料：**`score_to_mask_v3_dealer_issue_packages` 为 session、invocation、party 身份、收件人 RSA 公钥身份、A TLS 身份、N/K、协议版本、circuit hash、gate-schedule hash、triple 数及材料摘要生成 Ed25519 签名的 P0/P1 包；T 输入不含 score。T 直接生成 GF(2) Beaver triples 的两方 XOR shares 与随机置换矩阵 shares，发布后退出。在线 API 不联系 T，不补材、不重领。测试的 T、P0、P1、A 由 TEST_ONLY driver fork/exec 为互相独立 OS 进程；控制器只产生 additive input shares 并检查输出 XOR 与 frozen oracle。
- **包与 lease：**定长 v3 头、签名、session/invocation、N/K、Q20.12、party、收件人/escrow identity、circuit/schedule hash、gate 数、材料摘要严格校验。每个 Party 在发 online message 前以本地 SQLite WAL + FULL durability 消费 package-hash one-shot lease；重复 claim、错党、错 session/identity、篡改、截断与非法 gate id 被拒绝。失败的材料不可重用。
- **在线 GMW：**本方 32-bit additive score share 转换为本地 Boolean input share；GMW circuit 做 32-bit modular add、signed priority key（分数降序、原索引升序）、秘密 forward permutation routing、全部 pairwise lexicographic comparator borrow recurrence、Select5 自适应候选更新、membership、秘密 inverse routing。仅 Select5 活动边比较结果按 S31 批次公开；raw score、inactive comparator、permutation、membership 与原序 mask 留在秘密 wire 中。输出阶段刷新成 `F_TOPK-U`。
- **Beaver gate：**每 gate 由唯一 gate ID 绑定三份 Boolean shares `(a,b,c)` 且重构 `c=a∧b`。Party 打开 `d=x⊕a` 与 `e=y⊕b`，按 `c⊕(d∧b)⊕(e∧a)` 计算本地输出，并只在 P0 share 上加 `d∧e`。实现按层批处理并检查 gate id 不越界、所有预分配 gate 恰用一次。每批 frame 有 1 MiB 上限。
- **通信与 escrow：**已有 Protocol I framed transport 承担 mTLS/TLS 1.3、对端证书 pin、sequence/phase/session/party/type 绑定与长度上限；v3 envelope 再绑定协议/circuit/schedule 身份。收件人密文采用 OpenSSL RSA-OAEP-SHA256 包装 AES-256-GCM share，AAD 绑定 session、invocation、party 和电路 hash。A 没有私钥。提交状态与响应只有在该 API 收到正确交易的 receipt 后才暴露 share。

本轮新增 SQLite3 开发依赖是因为原运行时依赖没有提供这一 v3 escrow 的持久线性化事务；没有使用内存 map 冒充耐久服务。CMake 仅在 v3 开启时 `find_package(SQLite3 REQUIRED)`。本机 Ubuntu 24.04 WSL 有 SQLite runtime 3.45.1 但缺 dev headers；构建用从 Ubuntu package 提取到 `/tmp` 的 `libsqlite3-dev_3.45.1-1ubuntu2.9_amd64.deb`，SHA-256=`f5d920c94cd4729145547eeefb07e6269f78dc68f9a83e1a087baba47dd33429`，未安装系统包、未写进仓库。

## 5. 预算与实际资源结果

固定五次**算法迭代**不等于五轮网络通信。实现由依赖层组成；S31 给出的比较依赖层上界为 9，测试从实际 framed exchanges 记录 S31 `W+51` 上界，没有从算法迭代数推导在线 RTT。N=128/256 本机 Party 使用 socketpair 上的 mTLS frame 测试；A 使用本地 TCP mTLS。它们不是 LAN/WAN RTT 测量。

精确材料预算使用补齐后的维数 `P=max(2,next_power_of_two(N))`、`W=32+log2(P)` 和 `G=63P + P²W + binom(P,2)(2W-1) + P²` 个 AND gate。每个 AND gate 分配一个 GF(2) Beaver triple `(a,b,c)`；每方持有三份各 1 bit 的 share，所以 triple share 占 `ceil(3G/8)` bytes/party。每方的随机置换矩阵 share 占 `ceil(P²/8)` bytes。固定 header 为 372 bytes、Ed25519 签名为 64 bytes，因此签名包预算是 `372 + 64 + ceil(3G/8) + ceil(P²/8)` bytes/party。`N=128` 时 G=1,289,280、包 485,964 bytes；`N=256` 时 G=5,281,664、包 1,989,252 bytes；每个 gate id 必须恰好消费一次。

Party 预检峰值内存估算为 `B_package + G + Lmax*sizeof(GateInput) + Lmax + 2P² + 3PW`，其中 `Lmax=max(P²W, binom(P,2)W, P², 63P)`。运行时拒绝 `N` 超出 `[1,256]`、非法 K、G 超过 5,281,664、包超过 2 MiB、内存估算超过 256 MiB、单 gate-batch frame 超过 1 MiB 或材料头超出固定元数据上限的会话；测试实测包、frame 与 RSS 均在上限内。五个算法迭代、至多九个比较依赖层、`W+51` 在线 exchange-phase 上界和实测 phase 数是不同指标：`N=128` 的 W+51=90、实测 88；`N=256` 的 W+51=91、实测 89。每个 phase 的帧/字节统计由测试记录，`W+51` 不等于网络 RTT，也不是由五次迭代推出来的轮数。

| 实测点 | N=128, K=64 | N=256, K=128 |
| --- | ---: | ---: |
| `W`、`W+51` | 39、90 | 40、91 |
| G | 1,289,280 | 5,281,664 |
| Party package bytes | 485,964 | 1,989,252 |
| metadata header + signature | 436 bytes | 436 bytes |
| online exchange phases / 每方向 frames | 88 | 89 |
| 最大观察 payload | 159,852 bytes | 655,468 bytes |
| Party 最大 `ru_maxrss` | 25,360 KiB | 72,324 KiB |
| 完整本机 E2E 诊断 elapsed | 458 ms | 745 ms |

Package 与实际 frame 均低于 S31 的 2 MiB/package、64 KiB metadata、1 MiB gate-batch limits；Party runtime 的 preflight memory cap 为 256 MiB，观察到 RSS 低于该值。诊断 elapsed 包含本地进程启动/材料/escrow 服务操作，不是正式性能结果。最大值、逐 session 与逐包摘要在 [195 案 E2E 记录](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S32_E2E_CASES_2026-10-20.json)。

## 6. 功能和故障结果

| 检查 | 结果与边界 |
| --- | --- |
| package signature、triple reconstruct、one-hot permutation、out-of-range gate id、wrong party/session/identity、截断/篡改、重复 lease | `PASS`，v3 runtime conformance。 |
| 小规模排列/重复值/Q20.12 正负边界 | `PASS`；N=3 输入值 `{-1,0,1}` 的 27 个三元组对每个 K 都跑真实 T/P0/P1/A 进程；N=4 `{INT32_MIN,-1,1,INT32_MAX}` 的 24 种排列对每个 K 都跑。N=1/2/5/8 还覆盖相等分数、padding、K=1/K=N。 |
| N=128/256 E2E | 均新 session、新 T 包、新 lease、新 A database，输出 share XOR 对照 frozen `top_k_mask`；两方 mask 均二值、原序、恰 K 位。 |
| escrow 正常提交并重启 | `PASS`；双方 receipt 相同，A 重启后各自 fetch/decrypt 回本方 API share。 |
| 提交后回执丢失 | `PASS`；TEST_ONLY callback 丢弃已经从 A 收到的回执。P0 API `RECOVERY_REQUIRED` 且无 share，P1 已提交；A 重启后 P0/P1 分别恢复并重构 oracle。该 failpoint 模拟提交边界上的应用层回执丢失，不是自然网络故障报告。 |
| 提交前 A 崩溃 | `PASS`；T/P0/P1/A 独立进程中 P0 已耐久暂存唯一密文，P1 在 escrow submit 前退出 failpoint；A `SIGKILL` 后重启，状态 pending→ABORTED，双方 API 无 share、fetch 均拒绝、两边 lease 已烧毁。 |
| 提交前 A 等待超时 | `PASS`；唯一密文等待 2 秒，A 在 `BEGIN IMMEDIATE` 中将 pending→ABORTED 后回 abort；A 重启仍拒绝两边 fetch，双方 API 无 share。 |
| frame/身份 | TLS peer identity mismatch 定向测试通过；generic Protocol I transport sequence/replay-reject conformance 通过。它们不是 v3 runtime 在任意阶段断连/重排的完整故障注入。 |
| 尚未覆盖的故障矩阵 | `NOT_RUN`：v3 任意 message reorder、各 GMW phase 逐点 Party crash、READY payload mismatch、多轮重放/错 gate 在线帧和真实自然 I/O 错误。未出现这些用例的自然失败。存储掉电/硬件损坏也 `NOT_TESTED`。 |

## 7. 安全门与审查

本轮没有给任一安全 gate 标 PASS：

- P0、P1 的 `Sim_b(input_share_b, ideal_output_share_b, public_params, L)` 完整 view simulator 为 `OPEN/NO_GO`。
- A 的完整服务视图、提交/abort/crash/recovery simulator 为 `OPEN/NO_GO`；A 新增元数据 L_A 只定义/绑定，不自动满足原两方 leakage L。
- T 知道生成的 Beaver triple 与 permutation material；Party 的单点均匀边缘不替代整包、全 gate、输出/泄露条件分布证明。Dealer preprocessing 与在线 transcript 的联合证明为 `OPEN/NO_GO`。
- 已知 GMW/Beaver 结论未被视为自动证明。需要对该电路的输入 share 转换、opening transcript、仅批准比较结果的 declassification、Select5 自适应候选、inverse routing、输出刷新和 A 提交点逐 hybrid 实例化。该模型当前为项目设定的 semi-honest parties + trusted offline T + authenticated A；T/Party 合谋和 A 恶意行为未获证明。
- OpenSSL `RAND_bytes`/DRBG、Ed25519 package signatures、RSA-OAEP/AES-GCM 与 TLS 1.3 的实现级优势项、熵/证书配置、轮换及部署信任均 `NOT_QUANTIFIED`。不存在本轮可复现区分器；准确状态是证明与量化缺口。
- S31 未参与前序推导的技术接收者审查的是规格，不是 S32 代码。没有另一个未参与 S32 实现的 reviewer；独立代码审查与密码学证明复核均 `NOT_RUN`。

## 8. 实际构建、命令与错误尝试

环境：Ubuntu 24.04 WSL2 x86_64；G++ 13.3.0；CMake/CTest 3.28.3；OpenSSL 3.0.13；SQLite 3.45.1。构建物和 SQLite dev package 均在 `/tmp`。

主要命令（完整日志摘要和哈希见 source/build/evidence manifest）：

1. 原 S25 worktree 74-file manifest 重算：Python SHA-256 清单脚本；`PASS 74/74`，canonical 与请求值一致。
2. OFF fresh configure（不传 v3 选项）、target help、OFF static library build、`nm -C libsytorch.a` 和 no-macro header compile probe：目标/符号 absent，probe 被预期 `#error` 拒绝，`PASS`。
3. ON-only configure：`cmake -S VFSS -B /tmp/s32-v3-final-on-only-build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=OFF -DMOE_TOPK_ENABLE_SCORE_TO_MASK_V3_GMW_EXPERIMENTAL=ON -DSQLite3_INCLUDE_DIR=/tmp/s32-sqlite-deps/extracted/usr/include -DSQLite3_LIBRARY=/usr/lib/x86_64-linux-gnu/libsqlite3.so.0`；两个 v3 targets build `PASS`。
4. `ctest --test-dir /tmp/s32-v3-final-on-only-build -V -R "moe_topk_score_to_mask_v3_(runtime_conformance|process_e2e)_test"`：2/2，0 failed；process E2E 195/195。
5. v2 regression build 中 `moe_topk_project_derived_select5_conformance_test`、`moe_topk_score_key_refresh_v2_test` CTest 2/2；TLS 1.3 transport conformance binary 直接运行通过；generic sequence/replay transport conformance binary直接运行通过。
6. `git diff --check` 与新文件空白检查在本报告生成后执行，最终结果见 manifest。无运行时代码变更涉及 baseline；不跑全仓测试。

开发过程里首个 ON CMake configure 暴露原 target 的 `target_link_libraries` plain/keyword 混用，本轮把新 SQLite 链接调用改为 plain 后重新 configure/build。早期 OAEP 解密输出 buffer 按密钥明文长度分配不足，改为 `EVP_PKEY_get_size` 后修复。一次完整 timeout E2E 曾因 harness 没给 P1 传入 pre-submit failpoint 而错误等待 pending row，且旧 harness 未在异常时清理 A 子进程；修正 flag 并加入 child RAII cleanup 后，fault-only 与最终 195-case CTest 均通过。一次 OFF header 检查脚本匹配了错误的诊断文本，改为匹配头文件实际 `#error` 消息后通过。这些失败尝试不作为最终通过证据，亦未保留后台进程。

没有运行正式 LAN/WAN 矩阵。正式性能、CSPRNG/TLS 优势、SQLite 掉电安全、全阶段 fault matrix 与 S32 独立实现审查均明确为 `NOT_RUN`/`NOT_MEASURED`/`NOT_QUANTIFIED`，不借用 S25/S27/S31 数字代填。

## 9. 修改文件

本轮编写/修改的 S32 文件清单见同目录 source/build/evidence manifest。运行时代码只改 `VFSS/` 和共享 Protocol I transport；新增治理/状态文档为本报告、决策、gates、E2E 逐案记录以及 `PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md` 的当前状态补充。S25/S31 精确对象中的继承文件不算本轮新编辑；其清单成员与 S32 有意产生的文件增量在 manifest 中逐项列出。

## 10. 最终裁决

**转入“默认关闭的 v3 opt-in runtime 功能实现”并通过本轮列出的功能/进程/escrow 故障验证；安全裁决仍为 OPEN。**只允许继续实验性实现和安全证明工作。未经独立源码审查、完整 simulator/hybrid、A crash/recovery 证明及优势量化，不得把它提升为安全通过、默认运行或正式性能候选。
