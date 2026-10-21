# M6B-I-PROJECT-SELECT-5ITER-S41 复核报告

日期：2026-10-21

路线：**Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL**

## 1. 结论摘要

S41 的对象是 S40 已接收的 receipt/v2 精确提交 `b04618117b11cffe0e899947726ab1c66807ffdd`。本轮没有重跑 S40 的功能矩阵。S40 85 项来源哈希在原 S40 worktree 中逐项复核通过，canonical SHA-256 为 `10c508b3d0d16a06f826bbdfd0ada97b9fd4b99c2abdcd2336cd5a5605b804f6`。S41 新 worktree 从该提交开始，当前提交树与基线一致。

本轮给出了一个 **proof-only、版本化的诚实 endpoint 因果路由候选**：本方 `Sim_b` 提交自己的 ciphertext 后，另一名既有在线角色 `P_(1-b)` 可只用自身理想输出份额、package、TLS 凭据和公开设置，按真实 authenticated escrow client 路径提交第二份 ciphertext；A 因收到该真实请求而由 PENDING 转为 COMMITTED，`Sim_b` 不伪造 peer-deposit 成功事件。独立接收者 `/root/s40_independent_receiver` 已复核该因果路由，但裁定整体联合模拟为 **`OPEN_WITH_EXACT_GAP`**。

首个断点是：报告原稿把完整 `S_pre`（包含 `Vpre_b`、对端输出/package、A-only context 等）作为共同条件，再把固定状态的 continuation-kernel 等式对这个“共同 law”积分；这并未证明该隐藏状态与 `Sim_b` 可见前缀在真实/理想执行中的条件联合分布相同。未给出的核心式子是 `Law(Vpre_b,z_(1-b),pkg_(1-b),ctx_A,C_b | x_b,z_b,L_S29,pkg_b,static_b)_real` 与理想执行的对应条件 law 相等，或提供一个不泄露这些隐藏值的可执行联合 sampler。该缺口是尚未证明的接口/分布耦合，不是攻击，也不是不可能性结论。

因此不能将 receipt/status/commit/fetch continuation 标为通过，也不能进入 `F_T/v1`→真实 Dealer 桥接。本轮只证明候选 endpoint 状态机在固定完整边界状态下的局部因果转移可与源码逐项对应；**不等于成功路径联合模拟、完整安全证明或 secure。**没有发现 runtime defect；没有运行构建、测试或性能矩阵。

## 2. 固定对象、身份与边界

- S41 隔离 worktree：`C:\Users\28641\.codex\worktrees\s41-peer-deposit\MoE_Top-k_Reproduction_Evaluation`，分支 `codex/m6b-i-project-select5-s41`。
- 起始提交：S40 `b04618117b11cffe0e899947726ab1c66807ffdd`；其父提交为 S40 文档接收头 `b3b04dd835b355193b83d689c698a688267605bc`；记录的 runtime commit 为 `8028c0fece69fe0c648b1e0f1e12ad7fca723f20`。
- S40 manifest 有 85 个 source entries。S40 原 worktree 的 85 个 raw file hash 全部匹配 manifest，重算 canonical 为 `10c508b3d0d16a06f826bbdfd0ada97b9fd4b99c2abdcd2336cd5a5605b804f6`。S41 初始 checkout 中，`PROJECT.md` 与 `docs/IMPLEMENTATION_PLAN.md` 的 raw bytes 与原 S40 worktree 存在换行格式差异；当时新 checkout 的 Git diff 为空。S41 最终只在这两份治理文档追加本轮结论，故最终相对 S40 的 tracked diff 限于文档记录；其 S40 原始哈希与 85 项 manifest 仍由保留的 S40 worktree 复核。详细原始/最终哈希见 S41 manifest；没有静默替换源码。
- 证明模型：一方静态半诚实；A/T 按既定条件受信且不与腐化方合谋；理想 `F_T/v1`；无 I/O/进程故障的成功执行；可包括一次从已 COMMITTED 状态开始的本方 fetch。A-only 腐化、任意 abort/crash-prefix 隐私、物理掉电/介质损坏耐久性仍在范围外。
- 固定功能和角色：Q20.12 signed score 降序、同分按原始索引升序，输出原序且恰 K 个 1 的两份 XOR mask shares；A 存储加密份额、不解密；T 离线发材。A 的实际服务响应和逻辑提交合同不变。

证据分类：论文定义不因本轮而扩展；receipt/escrow 的状态与字段是固定 runtime 的源码事实；`F_A-ATOMIC/v2` 是项目派生的 proof-only endpoint 接口；其与真实 Dealer/RNG 的实现桥接是未验证假设。

## 3. 角色、权限与泄露隔离

| 实体 | 可持有/使用 | 不得获得或输出 |
|---|---|---|
| `Sim_b` / 腐化 party 的理想模拟器 | `x_b,z_b,L_S29,pkg_b`、公开 setup、本方静态状态与本方 pre-submit view；本方实际收到的 A response；本方 recipient public key | peer 输入/输出/package/TLS 凭据、A context、T 私钥、S38 reduction-only `J_b`，或任何 A-only 摘要 |
| 诚实 endpoint `P_(1-b)` | 自己的 `x_(1-b)`、同一 `F_TOPK` session 给它的 `z_(1-b)`、自己从 `F_T/v1` 得到的 `pkg_(1-b)`、自己的 TLS client credential 和公开 service/recipient 设置 | 向 `Sim_b` 返回 peer package、share、credential 或内部随机带 |
| A / `F_A-ATOMIC/v2` | F_T 只交给 A 的 context、两侧已认证请求、密文、context/package-set/cipher hashes、DB state、token 与到达顺序/时序 | 任何 share 的明文或 recipient 私钥 |
| T / `F_T/v1` | 联合采样并分别交付正确接收人的签名 package 与 A-only context；T signing key | 参与在线输入、排名或提交；私钥不给任何 simulator |

`L_S29` 不增加字段；A-only 的 context/package-set/ciphertext/hash/length/state/order/timing 不移入 party leakage。测试控制器在 S40 可读双方测试 share 以算 oracle，这只是历史功能测试权限，不属于本轮任何 simulator。A 的实际时序属于 `L_A`；S41 只证明离散成功 transcript，不把客户端 wall-clock latency 偷渡成既有 `L_S29`。

## 4. 固定源码中的真实状态机

下列位置均是 S40 固定 runtime 源码，S41 未修改它们。

1. P_b 在线计算结束后以本地 `local_output` 生成本方收件人密文，调用 `escrow_submitter(expected_transaction, party, ciphertext)`；仅在收到并校验 64 字符小写 hex receipt 后才把 share 放入 `PartyOutput`。见 `VFSS/src/moe_topk/score_to_mask_v3_party.cpp:725-765`。密文使用签名 package 绑定的 recipient key、RSA-OAEP-SHA256/MGF1-SHA256 包装 AES key、AES-256-GCM 与 session/invocation/party/circuit/gate-schedule AAD，见同文件 `:370-440`。
2. escrow 请求经 TLS 1.3 双向认证后，以固定 session、invocation、circuit、gate-schedule、该 party package hash、transaction、ciphertext length 和 ciphertext 字段解析；party identity 与接收端口绑定，见 `VFSS/src/moe_topk/score_to_mask_v3_escrow.cpp:152-209, 569-583`。party client 只发送自己 package 绑定的 submit/fetch，并核验服务身份、READY、transaction 与状态，见同文件 `:663-701`。
3. 第一份有效 ciphertext 落到 state=0 的 PENDING 交易。第二份已认证 deposit 到达后，`submit_ciphertext` 在一个 SQLite transaction 中保存第二份 share、生成 receipt/v2 token 并把 state 更新到 COMMITTED，最后 commit；对应源码 `:311-434`。S41 不重述 token 的局部均匀性引理；在理想独立 A 随机源下沿用 S39/S40 已审结的局部结论。
4. 每个 submit connection 在 state=0 时轮询；到 state=1 才回 `COMMITTED` 响应，字段为 status、transaction/ciphertext lengths、32-byte token、transaction，以及本次 response 可选 ciphertext，见 `:472-482, 584-608`。两个连接各自观察到同一逻辑 commit/token，但响应与 API 返回不是物理同时。
5. 从已 COMMITTED 状态发起 authenticated fetch 时，A 只按 party index 返回该方自己的 ciphertext 和保存的 token；fetch 再核验 context hash，见 `:442-469, 603-608`。重复请求不重新生成已存 token；本轮不重新测试该行为，沿用 S40 功能证据。

## 5. 诚实 endpoint 交互 sampler

下面是 **`F_A-ATOMIC/v2` 的理想接口规格，不是 runtime 新 API**。它只把真实存在的 P0/P1 endpoint 调用路由写明，不新增计算或服务角色。

```text
Session setup (same invocations, not independent samples):
  (pkg_0, pkg_1, ctx_A) <- F_T/v1(session, invocation, N, K, route,
                                  circuit, gate_schedule, public setup)
  (z_0, z_1)            <- F_TOPK(session, invocation, route, input_0, input_1)
  deliver pkg_i only to P_i; deliver ctx_A only to A
  deliver z_i only to P_i / its endpoint

Sim_b:
  generate/hold only its own valid C_b from z_b, pkg_b, pk_b
  send authenticated Submit(tx, b, pkg-hash_b, C_b) to F_A-ATOMIC/v2
  wait for the actual A response; do not synthesize the peer event

HonestEndpoint_(1-b):
  locally compute C_(1-b) = EncryptShare(z_(1-b), pkg_(1-b), pk_(1-b); fresh coins)
  send authenticated Submit(tx, 1-b, pkg-hash_(1-b), C_(1-b)) to A
  receive only its own normal response; keep all local material private

A / F_A-ATOMIC/v2:
  validate authenticated party identity, context/session/package/circuit binding,
    transaction, envelope header and size exactly as the real service
  persist first deposit as PENDING
  only after the second endpoint's real Submit bytes arrive:
    in the final transaction persist second ciphertext, sample U once,
    persist U and COMMITTED together, then release normal responses
  from COMMITTED, fetch_i returns only C_i and the stored U to authenticated P_i
```

关键权限点：`HonestEndpoint_(1-b)` 是实际既有 party 角色，由环境给它自己的输入、输出和 package；它自己形成 `D_peer` 并真正发起 A 的认证请求。`Sim_b` 不能调用 `DepositPeerSuccess()`，也不能接收 endpoint 的私密返回。A context 不交给 endpoint 或 `Sim_b`；A 使用自己的 F_T 输出完成验证。第二次 deposit 的真实消息到达才触发 COMMITTED，因此这不是无条件 peer-deposit oracle。

## 6. 条件分布与逐步论证

令 `S_pre` 是两方已到达 escrow 边界时的联合状态，至少含两份已生成的输出 share、两份 signed package、各自 TLS 身份、两把 recipient public key、本方 pre-submit view 与 own ciphertext、A-only context、交易 id 和服务状态。令

```text
K_real( D_peer, R_b, U, F_b | S_pre )
K_A/v2  ( D_peer, R_b, U, F_b | S_pre )
```

分别为源码 escrow continuation 与 `F_A-ATOMIC/v2` endpoint/service continuation 的条件核。对每一个合法固定 `S_pre`，两者在成功路径上的应用层字段与状态转移相同：

1. endpoint 使用 `z_(1-b),pkg_(1-b),pk_(1-b)` 及新鲜本地加密随机带，按与真实 party 相同的 `EncryptShare`/submit 编码产生 `D_peer`。这是端点自己的随机过程；不得将它交给 `Sim_b`。若只建模应用 transcript，这一步逐字节同分布；若要把 TLS record bytes 和随机数实现也计入，则还需 TLS/RNG 实现桥接，S41 不假装该桥已经证明。
2. A 收到该请求后按同一身份、session、invocation、circuit、schedule 和 package-hash 约束解析。第一个 deposit 仍只产生 PENDING；只有实际收到另一方合法 deposit，才有两行 share 并执行第二次状态转移。
3. 第二份密文、一次性 token `U` 与 COMMITTED 写入发生在同一个最终 SQL transaction。`U` 此处直接沿用 S39/S40 的理想 A 随机源局部引理；这一步不把局部 token 结论扩写为整体证明，也不推导真实 DRBG 优势。
4. `R_b` 的实际 wire 字段是 status、transaction length、ciphertext length、32-byte receipt/token、transaction，以及可选 ciphertext（源码 `score_to_mask_v3_escrow.cpp:472-482`）。submit 的响应不带 peer ciphertext、peer hash、package-set digest 或 context。对已提交 fetch，`F_b` 返回本方 ciphertext 和同一已存 token；重复 fetch 读取既有记录，不重新抽 token。

本轮尝试的高效交互 sampler 是：保留一个独立诚实 endpoint，由同一组显式参数绑定的理想 `F_T/v1` 与 `F_TOPK` 各自只向它交付 `pkg_(1-b), z_(1-b)`，向 A 单独交付 `ctx_A`；endpoint 按真实加密/认证 submit 算法生成 `D_peer`，A 运行真实状态机，`Sim_b` 只接收其实际 A response。该 endpoint 子程序是多项式时间且不需要 `Sim_b` 取得 peer secrets，也没有无条件 peer-success oracle。然而，S41 没有证明这个执行与 `Vpre_b` 由同一调用产生时所需的条件联合 law，因此它不是完整的 `J_b` sampler，也不能单独证明 party view 模拟。

所以本轮可建立的只有：对固定、完整且已给定的 `S_pre`，源码 escrow continuation 与候选 service continuation 的**应用层条件续接 kernel 等式**。该式没有使用密文伪随机性或 SHA-256 均匀性，也没有执行 IND-CPA 替换。但它不能直接推出本方完整视图的联合模拟。所需变量分解为：

```text
Q_b = (x_b, z_b, L_S29, pkg_b, static_b, Vpre_b, C_b)
H_b = (z_(1-b), pkg_(1-b), ctx_A, package_set/context digests,
       peer credentials/public setup, A state before peer deposit)

Law_real(H_b, D_peer, R_b, U, F_b | Q_b)
       ?= Law_ideal(H_b, D_peer, R_b, U, F_b | Q_b)
```

`H_b` 仍不能作为输入交给 `Sim_b`；此式只是明确联合分布比较对象。另一种足够的闭合方式是给出一个高效 sampler，能在不读取 `H_b` 的条件下生成与该 law 相同的 party-visible projection，并让 `P_(1-b)` 作为独立诚实 endpoint 仅通过其自身权限因果地产生 `D_peer`。当前报告没有证明这两种方式中的任一种。独立 reviewer 指出，S41 把 `Vpre_b` 与 peer/output/package/context 都放入共同条件状态，再称其具有共同 law，正是把待证明的条件耦合当成输入；“F_T 与 F_TOPK 使用 same invocation”也需明确绑定 session、invocation、N、K、route、circuit 与 gate schedule。故不得从固定 `S_pre` kernel 等式积分声称 `Law(R_b,U,F_b | Q_b)` 相等。

缺失的首个条件式可写为：

```text
Law_real(H_b | Q_b) = Law_ideal(H_b | Q_b)
```

或一项可量化的统计/计算距离界及构造它的联合 sampler。该条件无法由边缘分布相等推出；SHA-256 抗碰撞性、receipt/v2 token 的局部均匀引理以及 S38 reduction-only `J_b` 均未补上它。此处是证明/接口缺口，不是已知攻击或“不可能”结论。最小版本化接口候选为 proof-only `F_SESSION-CORRELATED/v1`：所有调用均带同一 `(session, invocation, N, K, route, circuit, gate_schedule)`；功能内部按明确的联合分布生成并分别交付本方 package/output 给各 party endpoint、仅给 A context，随后由诚实 endpoint 实际运行 submit。该接口不改变运行时角色、party 可见消息或 `L_S29`，也不给 `Sim_b` peer/context 权限；若采用，它会把“同次调用相关交付”明确写入理想 setup 接口，属于新增的理想接口假设，不是当前 S41 已有事实。它本身不证明真实分布等价，必须给出联合采样算法及其条件分布，不能把目标 law 作为黑盒 oracle 重述。随后由未参与推导者复核上述条件 law。在此之前，P_b 前缀与成功 A 响应/fetch 的联合模拟保持 OPEN。

对 `P_b` 的可见接口不返回 peer ciphertext、peer package、A context、package-set hash 或 A 内部到达顺序；A 仍单独持有这些 `L_A` 状态。若网络旁路抓包或客户端 wall-clock latency 被纳入 party view，则需要另行定义泄露并证明，本轮不声称 `L_S29` 覆盖它。S41 的比较对象限于离散成功消息及一次从已 COMMITTED 状态开始的 fetch。

## 7. 安全门、剩余假设与下一步

| Gate | S41 状态 | 说明 |
|---|---|---|
| S40 固定源码身份 / 85 项 canonical | PASS | S40 原 worktree raw hashes 全部匹配；S41 从 b046 开始，最终 tracked changes 限于本轮治理文档 |
| `F_A-ATOMIC/v2` honest endpoint causal route | REVIEWED_LOCAL_ROUTE_ONLY | 独立 reviewer 确认真实诚实 endpoint 可由自身材料按因果路径提交；这只确认路由规格，不确认联合模拟 |
| P_b prefix + submit/receipt/commit/fetch 联合模拟 | OPEN_WITH_EXACT_GAP | 缺少 `Law_real(H_b|Q_b)` 与 ideal 的相等/距离界，及不泄露 H_b 的联合 sampler；固定 S_pre 下的 kernel 等式不足以推出它 |
| S40 receipt/v2 target-size functional evidence | INHERITED_PASS_WITH_SCOPE | 本轮不重跑 211 案；详见 S40 cases/log hashes |
| ideal `F_T/v1` → 实际 Dealer 联合生成 | NOT_STARTED / BLOCKED_BY_PREFIX_JOINT_LAW | 在先闭合并独立复核上述 pre-submit 条件联合 law 前，不把 Dealer bridge 标成唯一下一项 |
| 实际 RNG/DRBG、TLS/认证传输与优势量化 | OPEN / NOT_MEASURED | 需处理并发、重启/reseed、调用数/可观察输出和各项 `Adv(·)` 参数 |
| 完整 GMW/Beaver 全视图 / 完整安全 | OPEN | 本报告不扩展局部条件续接为完整协议证明 |
| 任意 crash/abort prefix、A-only、物理掉电 | OUT_OF_SCOPE / NOT_PROVED | 保留既有合同和边界 |
| BUILD / TEST | NOT_RUN | 仅规格/证明文档变更，无 runtime/API 改动 |
| 自然 I/O 故障 / LAN-WAN / RTT / N>256 | NOT_REPRODUCED (沿用 S40) / NOT_RUN / NOT_MEASURED / NOT_ADMITTED | S41 未运行这些实验 |

**本轮后续工作**是先补并复核同一调用标识下 `Vpre_b`、诚实 peer 的 `(z_(1-b),pkg_(1-b))`、A-only `ctx_A` 与本方可见状态之间的条件联合 law，或构造权限正确的世界级 honest endpoint sampler。完成之前不推进更后续的组合。此点通过后，下一阶段才是 ideal `F_T/v1` 到真实离线生成器的证明义务清单：1) Beaver triples 与 permutation shares 的联合分布、独立性和材料槽映射；2) 两份 package/context 签名、用途域、session/invocation/N/K/circuit/gate schedule/package-set 绑定；3) OpenSSL RNG/DRBG 熵来源、重启/reseed、并发观察与 `Adv_DRBG(q)`；4) TLS/身份认证配置、证书 pin 与消息认证桥；5) one-shot lease 的原子消费、失败 burn 和重放映射；6) 每个 hybrid 的条件分布和优势项。以上仍全为 OPEN/未验证义务，不是 S41 通过项。

## 8. 本轮未运行项与交付

- `BUILD/TEST = NOT_RUN`；没有改 runtime 或测试源码，不重跑 S40 211 案。
- 独立接收者 `/root/s40_independent_receiver` 未参与 S41 推导；只读核查 endpoint 权限、条件联合 law、同次调用绑定及服务字段，结论 `OPEN_WITH_EXACT_GAP`。未构造反例/攻击，也未声称不可能性。
- 未启动正式 LAN/WAN，未测 RTT；没有把 S40 同机诊断时间或 phase 当作性能数据。
- 没有新增 `secure` alias；v3 和 receipt/v2 默认仍 OFF；N>256 未准入。
- 未修改 `VFSS-baseline/`；未推送/合并；构建物、密钥、材料、证书和原始日志均未加入仓库。
- S41 source/evidence manifest 记录 S40 85 项实际 hashes、commit/tree、source 关键文件摘要、本轮文档 hashes、审查范围及未运行状态。
