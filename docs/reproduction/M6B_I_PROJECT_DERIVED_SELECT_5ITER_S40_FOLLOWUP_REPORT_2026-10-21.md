# M6B-I-PROJECT-SELECT-5ITER-S40-FOLLOWUP 复核报告

日期：2026-10-21
路线：**Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL**

## 1. 裁决摘要

S40 在指定 S39 runtime receipt/v2 实现上完成独立目标规模功能接收：显式 opt-in Release 构建的 211/211 个进程 E2E 案例均完成记录；207 个有 share 的案例与冻结稳定 Top-K oracle 一致，4 个预提交中止案例无 share，因此标为 **NOT_APPLICABLE_NO_SHARE**，并在重启后拒绝 fetch。N=128 与 N=256 都覆盖 K={1,2,8,N/2,N−1,N}。default-OFF、v2 依赖门、未知 receipt version 拒绝、v1 兼容路径也已核验。

功能结果只证明所列实现与输入/故障集合的行为。**完整计算方成功路径模拟仍为 OPEN_WITH_EXACT_GAP**，没有安全通过结论。精确断点是本方 submit 之后，必须由诚实对端完成第二次 authenticated deposit 才能让 A 将 PENDING 交易推进为 COMMITTED。S39 的 v2 token 局部条件均匀结论不提供这条诚实对端路由，也不授予 party simulator 对端秘密或 S38 的 proof-only J_b 权限。

本轮没有发现可复现的运行时缺陷，因此没有修改 runtime，也没有创建 secure alias。协议业务阶段内自然读写错误为 **NOT_REPRODUCED**；正常 trace 保留了一条在最后业务帧成功转发后的代理额外读取 EOF，单独记录如下，不将其隐藏或误报为业务阶段失败。

## 2. 接收对象、来源和变更边界

S40 建立隔离 worktree：C:\Users\28641\.codex\worktrees\s40-followup\MoE_Top-k_Reproduction_Evaluation，分支 codex/m6b-i-project-select5-s40-followup。起始 HEAD 为 S39 文档接收头 b3b04dd835b355193b83d689c698a688267605bc，父级 runtime commit 为 8028c0fece69fe0c648b1e0f1e12ad7fca723f20。8028 到 b3 的 VFSS 差异为空；S39 85 项文件集合和记录 canonical hash 09444abd50c41357eb63e9050b7e39c3b2948588205d9d70f02561d7d8ec49d9 已重算核对。S40 当前 85 项 raw hash 与 S39 manifest 的差异已逐项归因：TEST_ONLY 驱动文件由本轮更改；IMPLEMENTATION_PLAN 包含 b3 审查更新和本轮追加；PROJECT.md 包含本轮追加。没有其他未解释差异，也没有用 main、主工作区或 S35 快照替换 runtime。

S40 唯一运行时代码范围外的源码差异是 TEST_ONLY 文件 VFSS/tests/moe_topk/score_to_mask_v3_process_e2e_test.cpp。其 SHA-256 为 5118c5c0db733ba783dfc2d13823df720fede805b32f53df57fad475a586ad6e。三个关键 runtime 文件与 8028 commit 对应 blob 相同：

- escrow：4814860cd38de345c6ea7a49e618742c1da43f0720dddaa7e0522da7b71ab829；
- GMW：8b27c1957b5b6d2a70d34d9edb6edbea77099c83b9218379911ee155616f6f26；
- party：3921884e0a7d99a0fba21fe5dc19f97a54ba50e5107603ec371cb5b30bb05996。

TEST_ONLY 驱动新增逐案例 ID、完整输入分数和状态 JSON、Dealer/P0/P1/A stdout 与 stderr 保存、退出状态，以及显式 receipt/v2 的 195 矩阵重跑和 N=128/256 边界案例。它没有改 secure/runtime 路径。最终源码差异和报告不包含密钥、材料、证书、构建物或原始大日志。

## 3. 构建和定向验证

环境为 WSL Ubuntu 24.04、CMake 3.28.3、Ninja 1.11.1、GCC 13.3.0、OpenSSL 3.0.13、SQLite runtime 3.45.1。构建目录和 SQLite 开发头文件/静态库解包都在 /tmp；未安装或修改系统包。

| 检查 | 结果 | 证据 |
|---|---|---|
| 无任何 v3/v2 选项的 fresh Release 默认构建 | PASS；两个 CMake cache 选项均 OFF | libsytorch.a SHA-256 214230eab20b67979d2429b651864b23bc12fba89909e2352c39589a16693a18 |
| default-OFF 隔离 | PASS；target help 无 v3 target，nm 无 v3 符号 | 同一 OFF 构建目录 |
| receipt/v2 ON、GMW v3 OFF 配置 | 按预期失败 | 明确错误：Receipt/v2 requires MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_GMW_EXPERIMENTAL=ON |
| 显式 GMW v3 ON + receipt/v2 ON fresh Release 构建 | PASS | libsytorch.a SHA-256 96107d58e85ca7ddc16ee240b5931ac2780d3e613f75f411e6ddd4cd681f3c26 |
| v3 runtime conformance | PASS | package signature、triple shares、permutation shares、one-shot lease、package faults；N=128/256 gate formula 为 1,289,280 / 5,281,664 |
| 未知 receipt version 3、v2 默认值、N=257 等非法边界 | PASS；拒绝符合预期 | 同一 conformance；默认 receipt 仍为 v1 |
| receipt/v1 兼容 smoke | PASS | N=4, K=2，双方提交、oracle 与 restart/fetch recovery PASS |
| receipt/v2 matrix + N=128/256 | PASS_WITH_SCOPE | 211 条逐案结果，case_id 唯一，全部 version=2 |

首次 receipt/v2 CMake 配置因环境缺 SQLite 开发头文件失败；没有系统性安装，改用本地解包的兼容开发头/静态库后 fresh configure/build 通过。首轮 211 案测试输出曾把无 share 的中止案标成 oracle PASS；检查后只修正 TEST_ONLY 结果标签为 NOT_APPLICABLE_NO_SHARE，并以最终二进制重跑完整 211 案。该标签修正没有运行时改动。首次配置失败和旧日志均留在仓库外，并在 manifest 登记。

测试命令与确切 cache/binary/log 哈希见 source/build/evidence manifest。最终 process E2E binary SHA-256 为 d2b14ced64a82dbf984bef899e81665c479c505fe9d9bac6f025ea686ccb599a；最终 runtime conformance binary SHA-256 为 e34f73403718c3c566c5eec43153de011d546f018bc9be1594df8897907c39c5。

## 4. v2 功能矩阵、oracle 和目标规模

195 个原矩阵案例均在 receipt/v2 配置和最终二进制下重跑：15 个边界/资源 fixtures、81 个三值 N=3 全枚举组合、96 个 N=4 全排列与 K 组合、1 个提交后丢回执恢复、1 个预提交服务 kill、1 个预提交超时。其后新增 16 个目标规模案例：N=128 与 N=256 各自覆盖 K={1,2,8,N/2,N−1,N}，并各跑一次提交后丢回执恢复及一次预提交中止。最终为 211 个独立 session、Dealer package、one-shot lease 和 escrow DB；测试身份在该进程的套件内复用，不能把它误写为每案新证书。每个案例保存了真实 int32/Q20.12 codepoint 输入、session/invocation、两份 package hash、退出码、oracle/API 状态、材料 gate 计数、包长、exchange phase、payload、RSS、诊断时间和外部日志目录。

结果计数：

- 204 案正常提交，双方 API 返回各自 share，重启后重复 fetch 成功；覆盖重复相同 ciphertext submit 复用相同 receipt、变更 ciphertext 重复提交拒绝以及错误 TLS 身份拒绝。
- 3 案人工丢弃 P0 已提交响应，P0 以 recovery-required 退出、P1 已提交；服务重启后各自 authenticated fetch 返回自身密文/相同 receipt，恢复 share XOR 通过 oracle。
- 3 案人工在一个密文 pending 后 SIGKILL A；P0/P1 均 recovery-required，无 share，重启时该 transaction 进入 ABORTED，双方 fetch 被拒绝。
- 1 案人工预提交 timeout；双方无 share，重启后 ABORTED 且不能 fetch。
- 因而 207/211 oracle PASS，4/211 为 NOT_APPLICABLE_NO_SHARE；无未预期失败、资源中止或失败重试。

N=128 六个目标 K 的 gate count 是 1,289,280，每方 package 为 485,964 bytes，测得最大 payload 字段 159,852 bytes，peak RSS 25,076–25,332 KiB，应用 exchange phases 81–88，本机诊断时间 823–886 ms。N=256 六个目标 K 的 gate count 是 5,281,664，每方 package 为 1,989,252 bytes，最大 payload 字段 655,468 bytes，peak RSS 72,368 KiB，phase 79–89，本机诊断时间 1,118–1,291 ms。它们仅是本机功能驱动记录，不属于正式性能或 RTT 结果；phase、比较依赖层和五次算法迭代分别计数。

## 5. 无注入正常多进程通信和 EOF

另运行 receipt/v2、N=128、K=64 的无注入多进程路径，双方进程经 TEST_ONLY TLS frame proxy 完整双向转发。双方各发送/接收 88 帧、88 个应用 phase；P0 与 P1 exit 0，API 都是 committed/share-returned，one-shot lease 均已 burn；A 数据库为 1 个 transaction、2 份 ciphertext，state=COMMITTED。两份 share XOR 与原序稳定 Top-K oracle 相同。超时配置为 15,000 ms。

逐帧记录最后两组是 phase 87 成功往返，然后 phase 88 的 P0 tag 6、237-byte payload 与 P1 tag 6、105-byte payload 都 read=OK/write=OK 并成功转发。之后 proxy 的循环又尝试接收下一 phase，记录 TLS unexpected EOF。源码循环因此下一次将读取 phase 89；若收到 frame，其零起始接收序号将是 88。这两个期望值是从 phase 1..255 循环、phase 88 完成及当前 frame counter 推得，日志没有伪称为已收到的 frame。证据中必须保留这一 EOF；从最后一对成功业务帧、双方 phase/frame 计数与退出状态看，它发生在业务转录结束后的代理额外读取，不是 P0/P1 API 的读写错误，也不是“干净 TLS close”。因此业务阶段自然 I/O fault 为 NOT_REPRODUCED，单独附注 POST_FINAL_PROXY_READ_EOF_OBSERVED。

A stderr 中两个 TLS handshake unexpected EOF 对应测试 readiness helper：VFSS/tests/moe_topk/score_to_mask_v3_process_e2e_test.cpp 的 wait_port_ready 只执行 TCP connect 后立刻 close，不完成 TLS handshake。矩阵服务重启 stderr 中 changed duplicate ciphertext 和错误身份拒绝为有意的负测。无注入 trace 全量子进程 stdout/stderr 文件均保留；Party stderr 为空、成功退出。服务进程以测试控制器 SIGTERM 结束（exit 143），这是套件清理，不是故障注入。

## 6. 源码审查：服务响应、token 与成功路径因果顺序

固定 S35/S39 runtime 的 escrow 代码显示：

1. P_b 以自己签名 package 派生 transaction id，构造请求，TLS 连接时绑定服务证书身份，提交自己的 recipient ciphertext。请求含 operation、party、session、invocation、circuit/gate-schedule hash、本方 package hash、transaction id 与密文。
2. A 验证 context 和 party identity。第一份密文通过一个 transaction 内的 BEGIN IMMEDIATE/COMMIT 写到 shares，transaction state 留为 0/PENDING。该状态是 A 数据库内的状态；成功 API 不会先收到一个 PENDING response。
3. 第二个已认证 party deposit 到达；同一提交事务内插入/核验第二份密文。两份 share row 齐备且原状态为 0 后，A 生成 receipt/v2 token，更新交易为 state=1 并保存 receipt，随后 COMMIT。v2 token 由 escrow.cpp 的 make_receipt 调 RAND_bytes 得 32 bytes；它是 opaque token，不是 party 校验的 MAC。
4. 两个 submit handler 各自读取状态；只有状态为 COMMITTED 才向其连接发送 status、transaction id 与 token。两次服务响应与 party API 返回并不物理同时。party 没有直接看见 A context、package-set digest 或对端 ciphertext。
5. committed fetch 依本方 TLS identity/package 认证，只返回该 party 自己的 ciphertext 与同一个持久 receipt；重复 fetch/submit 复用，不改变 token。A 持有数据库但没有 RSA recipient share 解密私钥。

响应结构包括 status、transaction length、ciphertext length、32-byte receipt、transaction 字符串和可选本方 ciphertext；submit 不含 ciphertext，fetch 含本方 ciphertext。receipt API 层以 64-char lowercase-hex 格式承载 32-byte token，只检查格式与 TLS service identity；它不做 receipt MAC/签名验证。A 的 L_A 与 party L_S29 分列：A 可见 context/package-set/circuit/gate hashes、密文、摘要/长度、transaction status、抵达顺序及时序；不得把这些 A-only 字段偷偷塞进 L_S29。

'源码锚点（均为 S39 runtime commit 8028 的文件内容）：

- VFSS/src/moe_topk/score_to_mask_v3_escrow.cpp:245-260 生成 v2 32-byte token；:315-434 写入 PENDING row、核验两份密文并在最终事务设置 COMMITTED/receipt；:442-469 取回指定 party 的密文；:472-482 编码状态/长度/receipt/transaction/ciphertext 响应；:583-608 处理 submit 等待与 fetch；:837 启动时将遗留 PENDING 标为 aborted；:851-867 是客户端 submit/fetch API。
- VFSS/src/moe_topk/score_to_mask_v3_party.cpp:746-765 检查 receipt 表示并发布 committed output。
- VFSS/tests/moe_topk/score_to_mask_v3_process_e2e_test.cpp:727-740 的 wait_port_ready 只做 TCP connect 后 close；:1138-1178 覆盖重启 fetch、重复 fetch、相同密文重复 submit、改密文拒绝和错误 TLS 身份拒绝；:1383-1469 是逐帧 TLS proxy；:1697-1875 注册 receipt/v2 matrix 与目标规模案例。

### 角色、输出和可见范围

输出合同保持为：对 signed Q20.12 codepoint 按 score 降序、同分按原始索引升序，恢复为原始输入顺序的秘密共享 bit-mask，恰有 K 个 1；P0 与 P1 的本地 share XOR 为该 mask。项目功能观察不构成论文算法复现或安全证明。

| 角色 | 真实持有/执行 | 真实可见范围 |
|---|---|---|
| 离线 T / 理想 F_T/v1 | 相关 Beaver triples、shuffle permutation shares、双方签名 package、metadata-only escrow context；T 签名私钥留在该功能内 | 不参与在线输入或 A 的提交恢复；本轮把生成分布视作理想前提 |
| P_b（被模拟的一方） | 本方输入份额、pkg_b、one-shot lease、本地 wire/triple 状态、本方提交的密文 | 公开 setup、L_S29、本方 API 最终收到的状态/receipt及从 COMMITTED 开始的本方 fetch；看不到 peer package/share/credentials 或 A context |
| P_(1-b)（诚实对端） | 自己的输入份额、理想输出份额、pkg_(1-b)、TLS credential 和发给 A 的 recipient ciphertext | 自身 package/协议状态和 A 对自身 endpoint 的响应；此角色的因果消息目前没有被明确路由进 party simulator 接口 |
| 在线 A | metadata-only context、transaction/package-set/context hashes、双方加密 share ciphertext/hash/length、状态、token、到达顺序和时序 | 单独记为 L_A；无 RSA recipient share 私钥。A/T 与任一腐化方不合谋是已确认的条件信任假设 |
| TEST_ONLY 控制器 | 为生成输入、重启服务、检查数据库并计算 XOR oracle，可在测试内部读取双方测试输出 | 仅属功能测试权限，不属于 P_b simulator，不可用来补条件采样证明 |

L_S29 保持原样；不把 A 的 context/package-set digest、对端 share、rank、完整 mask、内部随机带或额外比较位加入 party leakage。F_TOPK-ATOMIC 也保持：PENDING 是 A 内部状态；最终 COMMITTED state/token 是双方可恢复的逻辑提交点，两个 API 的实际返回时刻可不同。

## 7. 条件模拟裁决与首个精确断点'

本轮固定：任一方 P_b 静态半诚实腐化；A/T 是条件受信、诚实且互不与该腐化方合谋；理想 F_T/v1；成功无 I/O/进程故障转录，另只包含从既 COMMITTED 状态开始的一次本方成功 fetch。任意 crash/abort prefix 隐私、A-only 腐化、物理断电/介质损坏不纳入主定理。F_TOPK-ATOMIC、输出定义、L_S29、角色均保持 S39 合同。

P_b 的 party simulator 许可输入仅包括本方输入 x_b、本方理想输出份额 z_b、公开 setup、本方 package pkg_b、静态本方状态 Vpre_b、获准 L_S29 与真实接口中本方应收到的 A 响应。它不能得到对端 z、package、凭据、A context、T 私钥，也不能调用 reduction-only J_b。S40 测试控制器为核对 XOR oracle 而能读取双方 test share，这是测试权限；它不属于安全 simulator，不能拿这项能力填证明。

S39 的 ideal 独立 A 随机源下 token 局部条件均匀性沿用为 CONDITIONAL_PASS_REVIEWED；它说明给定前缀/hidden context 时随机 token 分布可模拟，但不产生使真实 A 到达 COMMITTED 的因果事件。因此停止在本方 authenticated submit 以后、诚实 peer authenticated deposit 以前。缺失的目标关系为：

    Law(D_peer, response_b, U, fetch_b | x_b, z_b, L_S29, pkg_b, static_b, Vpre_b, C_b)_real
    ≈ Law(Sim_b(x_b,z_b,L_S29,pkg_b,static_b,Vpre_b,C_b;F_T/v1))_ideal

其中 D_peer 至少含有效的 peer TLS 身份/认证请求、peer package 绑定字段和 peer ciphertext。源码要求它真实到达 A 后才使 state 0→1、生成/持久 U 并放开响应。现有 F_T/v1 和 party simulator 接口没有一个可执行的诚实 peer endpoint：可以作为诚实方的 P_(1-b) 取得自己的理想输出与自己的 package、构造密文并向 A 发 authenticated deposit；其效果再按真实协议对 P_b 可见的方式触发 response。若 party simulator直接构造 D_peer 就越权；若给它 peer package/share/context/T 私钥或 J_b 也越权；若给“peer deposit success oracle”则跳过了需要证明的真实因果步骤。故这一步条件分布未被建立。它是证明/接口模型缺项，不是攻击、运行时故障或不可能性证明。

可执行下一步候选（必须新版本化，且不改 L_S29）：

- 理想接口路径：为 F_A-ATOMIC/v2 增加有身份与一次性状态的 honest-peer endpoint transition。它只由真实诚实 endpoint 持有自己的 F_T package 和理想 z_peer，可向 A 提交经过协议认证的 D_peer；Sim_b 只能收到其 party 接口会返回的 COMMITTED receipt/fetch 字段。将 endpoint 与本方前缀一起定义统一 PPT 条件联合 sampler，并由未参与推导者复核。party 可见字段不变，A 仍持有既有 L_A；F_T 和 A 保持条件受信/不合谋。
- 实现接口路径：若理想接口之外还要有可执行 role route，新增版本化 P_peer→A 受认证 relay/process，relay 只搬运对端 endpoint 产生的密文与服务必要元数据，不持有解密 key；A 看到新增 relay 身份、到达顺序和时序，需明确登记在 L_A；party response 固定保持现合同格式，L_S29 不扩张。该设计仍必须定义 honest endpoint 如何取得自己的理想/真实 package 与输出并建立前述 joint law；relay 本身不自动闭合证明。两种都不改变算法或输出，除非另立版本化合同，否则不改运行时。

## 8. 状态、未完成事项和下一步

| 项目 | S40 状态 |
|---|---|
| S39 receipt/v2 独立 target-scale 功能接收 | PASS_WITH_SCOPE；N=128/256 案例已跑 |
| default-OFF / explicit v3+v2 ON / v2 without v3 guard / v1 compatibility | PASS |
| receipt/v2 status/submit/commit/restart/fetch、duplicate、identity、lost receipt、precommit abort | PASS_WITH_SCOPE |
| 自然协议阶段读写故障 | NOT_REPRODUCED；最后业务帧后的代理 EOF 单独披露 |
| 诚实对端 deposit + party commit/fetch 成功路径 simulator | OPEN_WITH_EXACT_GAP |
| ideal F_T/v1 → real offline Dealer/material/signature/DRBG bridge | NOT_REACHED；后续义务，不是通过项 |
| A-only corruption / 任意 crash-prefix privacy / 物理掉电耐久性 | OUT_OF_SCOPE / NOT_PROVED |
| N>256 | NOT_ADMITTED |
| 正式 LAN/WAN / RTT | NOT_RUN / NOT_MEASURED |
| secure alias / v3 default | NOT_CREATED / OFF |

若 role route 后经证明并独立复核闭合，后续才审 ideal F_T/v1 与实际离线发材生成的联合分布：Beaver triples 与 permutation shares 的完整联合分布/独立性、一次性 package 与 session/invocation/gate-schedule/signature 绑定、真实 OpenSSL RNG/DRBG 重启/reseed/并发观察、TLS/认证传输和材料消费映射及各自的符号优势界。当前这些为 NOT_REACHED/OPEN；真实 A RNG 项 Adv_DRBG_A(q)、签名、TLS、hash binding 和 envelope encryption 的优势均未量化，不填安全位数。五次算法迭代、比较依赖层、应用 phase 和 RTT 是不同指标，正式性能仍遵守项目的 V3→V4 前置门。

S40 未改 VFSS-baseline、未提交到 main、未推送或合并。构建产物、SQLite 开发包、key/material/certificate 与原始大日志均留在仓库外。一个只读审查命令误生成的 3-byte 临时文件 proxy（内容仅“88”）已从本轮隔离 worktree 删除；它不属于源码差异或交付证据。

## 9. 本轮修改文件

- VFSS/tests/moe_topk/score_to_mask_v3_process_e2e_test.cpp — 只在 TEST_ONLY harness 添加 case id、子进程 stdout/stderr 保存、退出码/逐案 JSON、receipt/v2 195 案重跑和 N=128/256 边界/恢复案例。
- docs/decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S40_FOLLOWUP_DECISION_2026-10-21.md — 新增决策。
- docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S40_FOLLOWUP_REPORT_2026-10-21.md — 本详细报告。
- docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S40_FOLLOWUP_GATES_2026-10-21.json — 机器门禁。
- docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S40_FOLLOWUP_CASES_2026-10-21.json — 211 条可核验 case 结果和输入。
- docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S40_FOLLOWUP_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-21.json — 来源、构建、日志/二进制哈希。
- PROJECT.md 与 docs/IMPLEMENTATION_PLAN.md — 各追加一段 S40 状态摘要，未改写 S39 历史。