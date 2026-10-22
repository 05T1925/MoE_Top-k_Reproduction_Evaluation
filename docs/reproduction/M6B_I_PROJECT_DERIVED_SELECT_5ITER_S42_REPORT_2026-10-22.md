# M6B-I-PROJECT-SELECT-5ITER-S42 联合模拟复核报告

日期：2026-10-22

路线：**Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL**

## 1. 结论与范围

本轮针对 S41 的首个断点构造了一个权限正确的**本方可见成功转录 sampler**。核心观察不是隐藏状态 `H_b` 与本方前缀具有相同条件分布，而是：在固定的无故障成功执行中，peer deposit 只决定 A 的逻辑状态何时能变为 COMMITTED；本方 submit/fetch 的应用层响应不携带 peer ciphertext、A context、package-set/context 摘要或到达顺序。提交响应只含固定 COMMITTED 状态、本次交易 ID、空 ciphertext 长度和 32-byte receipt/v2 token；一次已提交后的 authenticated fetch 只返回本方 ciphertext 与同一 token。token 在理想独立 A 随机源下沿用 S39/S40 的局部条件均匀结论。

这使成功路径的**party-visible projection**可以直接采样，而无需模拟完整 `H_b`，也无需证明 `Law_real(H_b|Q_b)=Law_ideal(H_b|Q_b)`。关键仍是因果关系：P_(1-b) endpoint 必须从同一次 `F_T/v1`、`F_TOPK` 调用取得自己的 package/share，按协议加密并 authenticated submit；A 收到该消息后执行真实状态机，`Sim_b` 才从自己的 A 通道看到 response/fetch。并未把成功事件交给模拟器伪造。

未参与 S41 推导的独立接收者 `/root/s40_independent_receiver` 已只读复核 sampler 与因子分解，结论是范围限定的 **`CONDITIONAL_PASS_REVIEWED`**。审查者确认该投影模拟不要求完整 `Law(H_b|Q_b)` 相等、没有隐藏 target-law oracle，且诚实 endpoint/A 信息路由与响应字段符合源码。该结论仅适用：一方静态半诚实腐化 party；A/T 条件受信且不与该 party 合谋；理想 `F_T/v1` 与 `F_TOPK` 的一次同次调用；固定 route、`1≤K≤N≤256`；理想独立随机带；该交易 ID 与双方 one-shot package 均新鲜/未消费；两份有效 authenticated deposit 均在 A 配置等待期限内被接受；无 I/O/进程故障的成功应用/API 转录；以及一次从已 COMMITTED 状态开始的本方 authenticated fetch。原始 TLS wire transcript、时序、任意延时/超时和失败前缀均不在本引理中。此结果不等于完整 GMW 安全或 `SECURE`。

论文定义、项目规格、源码事实、测试观察、证明假设分列如下：Protocol I 的论文内容不推出本项目 Select5 是论文算法；稳定 Top-K 和 `L_S29` 是项目规格；交易、envelope、响应字段和提交顺序是固定 runtime 源码事实；S40 的 211 案是历史功能观察；ideal `F_T/v1`、独立随机带、S36 H4 和 S39/S40 token 条件引理是有范围的证明前提，不是本轮新测得性质。

## 2. 固定源码对象和来源核验

- S42 隔离 worktree 从 S41 提交 `486026ba425a2d17ca15173d056db6eb309747d1` 开始；S41 父级为 S40 `b04618117b11cffe0e899947726ab1c66807ffdd`。S40 runtime commit 为 `8028c0fece69fe0c648b1e0f1e12ad7fca723f20`。
- S41 manifest SHA-256：`2d7c453b62232093981864358f98563e0a985f06b0d363872b7bca588ecf0ae5`。按 S41 manifest 的 declared canonicalization，从其中 85 条 `sha256_s40_original_worktree` 重算得到 `10c508b3d0d16a06f826bbdfd0ada97b9fd4b99c2abdcd2336cd5a5605b804f6`，与 S40 记录相同。另从 S41 manifest 的 85 条 `sha256_s41_checkout` 重算为 `e0bb4d2507a2cf2536c5d28d53c5c2c6aa19f927359482c1e236c5b19b5f8a69`，与该 manifest 汇总字段 `sha256_s41_checkout=bfa8ba9c051ca69c43a22130fa9d1cefc8885cbed10284ed0506762667ab726c` 不同。该 S41 历史 manifest 汇总差异未能从本轮资料解释；原 S41 文件不改写。固定 S41 commit/tree 精确、S40 原始快照 canonical 可复算，S42 开始前的 checkout 有 83/85 raw entry 匹配，因此源码对象仍由精确 commit/tree 确定；此历史 aggregate provenance 作为独立 `OPEN_METADATA_DIVERGENCE` 记录。
- S42 完成文档更新后的 85 项来源集合 canonical SHA-256 为 `2ea2896cce7dd284f3d317a7462dde528c188cc15cb32af16c744dad4e5e6837`。此值反映 S42 明确治理追加后的当前工作树，不替代 S41 固定提交身份或其 manifest 汇总字段。
- 在 S42 开始前，将 S41 `sha256_s41_checkout` 与 85 个工作树文件 raw SHA-256 对照：83 项逐字节一致；仅 `PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md` 不同，分别为 `9acd5b084237a0bca0142c5d3be009f30a8b7232473841f7f5887bb619bcbd62`→`3c72c1fe433351163f4705e1d9d6d8608884a042c5859b9c5861c977df3a9207` 和 `84fba36475bddbddcd3927cfe19ec79c724d49a1bae3f904d579e14b8e2d3b82`→`f3478e67a3328aa604a3da34308bd7114d1e1039a66a35495be82b5a1f5c35f8`。这是 S42 起始 checkout 的 Windows 换行差异；之后这两份文件被有意追加 S42 状态。HEAD tree `f37604525a67562a9a3fe3f36237d92502ade758` 与 S41 commit 一致，起始 `git status` 干净。没有以 main 或相似源码代替。
- S42 runtime 未改。相对 S39 runtime commit 的唯一 VFSS 差异是 S40 增加的 `TEST_ONLY` process E2E harness；S41 的 runtime 源码与 S39 runtime commit 相同。`VFSS-baseline/` 未动。

## 3. 冻结角色、同次调用绑定和泄露

完整调用元组固定为

```text
τ = (session, invocation, N, K, route, circuit, gate_schedule)
```

加上公开身份/证书指纹、双方 recipient public-key 指纹、A identity 与 T verification key。证明只使用这一条固定 route；调用协调器按下面的已定义输入核对同一 `τ`，它只做公开一致性检查并路由现有 ideal-functionality 输出，不采样或隐藏重述目标联合 law。

| 项/值 | 生成、持有和绑定位置 | party 与 A 的可见范围 |
|---|---|---|
| `F_T/v1` setup | 可信离线 T 对 `(protocol=v3, session, invocation, N, K, identities, recipient-key hashes, circuit hash, gate-schedule hash, resource/state params, verifier key)` 生成相关 packages/context。材料随机带独立于输入；T 私钥仅留在 T。 | P_i 只收 `pkg_i`；A 只收签名 metadata-only `ctx_A`。party 模拟器没有 peer package 或 context 权限。 |
| `F_TOPK` | 对 `(session, invocation, N, K, route, x0, x1)` 重构输入的 stable Top-K 功能，输出 `z0=R`、`z1=R XOR M`，其中 `M` 是原序稳定 mask。 | party i/其 ideal endpoint 只收 `z_i`。输出份额在协议内部可供密文提交；`F_TOPK-ATOMIC` 仍控制 party API 何时返回 share。 |
| 双方 package | 签名 package header 显式带 session/invocation/N/K、双方身份与 recipient key hashes、A identity、party index、gate/material counts、circuit/schedule hash、材料摘要和 signer fingerprint。 | 每 party 只持本包；A 通过 `ctx_A` 中签名 package hashes/package-set 绑定它们。 |
| `ctx_A` | 仅 A 持有；签名 body 显式带 session/invocation/N/K、party/A identities、circuit/schedule、两 package hashes/set digest、签名者与状态参数，包括 receipt format。 | 属 `L_A`，不放进 `L_S29`，不交给 `Sim_b` 或 peer endpoint。 |
| transaction | runtime `transaction_id = "s2m-v3-" || hex(session) || "-" || hex(invocation)`。A DB 同时保存 session/invocation/N/K/circuit/schedule/context/package-set 元数据；同 ID 的参数不一致会被拒绝。 | transaction ID 本身只编码 session/invocation；上下文其余绑定由 signed context、package hash、request 校验和 DB metadata 完成。 |
| route/circuit/schedule | `F_TOPK` 显式收到 `route`。固定 runtime route 是 PROJECT_DERIVED_SELECT_5ITER 的 v3 实现；F_T 输入显式带 protocol v3 和 circuit/schedule hashes；escrow context 显式带 protocol v3 及 hashes；party package 用 v3 magic/package-format version 与 circuit/schedule hashes识别该固定路线，但没有独立 protocol-version 或 route 字符串。S42 公开绑定器使用固定的 `route → (protocol v3, circuit hash, schedule hash)` 映射并拒绝不匹配。 | 该映射对单路线证明足够；不能推成未来多 route 共用格式已绑定，未来需要版本化显式 route 字段。 |
| ciphertext `C_i` | 收件人 RSA public key 的指纹与 package 一致；AES-GCM AAD 为 session/invocation/party/circuit/schedule。A 请求还携带 transaction、party/package context 与 ciphertext；A 不解密。one-shot lease 以 package hash 为主键并记录 session/invocation/party，package hash 覆盖其全部签名字段与材料。 | P_i 知本方密文；A 知两份密文/长度/hash（`L_A`）；P_i 不收 peer ciphertext。 |
| `L_S29` / `L_A` | `L_S29` 仍是批准的 public setup/protocol binding、获准活动 endpoints/位置、pivot/活动比较 outcome、按活动前缀公开的 sample seed、已批准 masked value 类别、比较数、通信长度与统一 ABORT。没有添加项目字段。 | A 的 context/hash、package-set、ciphertext/hash/length、PENDING/COMMITTED、到达顺序/时序、receipt 与恢复状态只在独立 `L_A`。 |

不以“同一个 session”替代同次调用约束：协调器要求 F_T 的 session/invocation/N/K 与 F_TOPK 的这些字段一致；将 F_TOPK 的 route 映射到 v3 固定 circuit/schedule，再核对 packages、context、请求和 transaction row。交易字符串虽不直接编码 N/K/route，A 对已有 transaction 的 metadata 冲突会拒绝。源码无显式 route 字段这一范围限制已公开记录，不改历史包格式。

## 4. 源码中的本方可见 escrow 续接

固定对象的关键事实（均在 S40 的 v3 receipt/v2 runtime）：

1. party 在线末尾形成 `local_output`，以 signed package 绑定的本方 recipient key 加密：RSA-OAEP-SHA256/MGF1-SHA256 包 AES-256-GCM，AAD 为 `(session, invocation, party, circuit, gate_schedule)`，输出 envelope 含 magic/version、wrapped-key length、plaintext length、nonce、tag、wrapped key 和 GCM body。`score_to_mask_v3_party.cpp:370-440,725-730`。
2. party 以 `tx=session+invocation` 发 authenticated submit。A 在 mTLS READY 后验证 party certificate pin、请求 session/invocation/circuit/schedule/transaction、package hash 与大小。第一 deposit 可建立 PENDING；对端合法请求到达后，最终 SQL transaction 写第二份密文、采样 v2 token、更新 COMMITTED 与 token，然后提交。`score_to_mask_v3_escrow.cpp:87-99,152-209,311-434,569-608`。
3. PENDING/DB row/peer hash 不作为 submit response 返回。COMMITTED wire response 结构是 `status || u16(tx_len) || u32(ciphertext_len) || receipt[32] || tx || optional_ciphertext`；submit 的 optional ciphertext 为空。party client 核对 response length/status/transaction，将 receipt bytes 编为 64 位小写 hex；party API 只有在格式符合且提交返回后才放出本方 share。`score_to_mask_v3_escrow.cpp:472-482,642-660,675-701,849-857`；`score_to_mask_v3_party.cpp:746-773`。
4. A 的第一次 submit handler 可以等待另一请求；在成功执行中，诚实 peer endpoint 实际产生 authenticated deposit 才使状态成为 COMMITTED。A 的两个连接都能读到同一逻辑 token，但客户端物理返回时间可以不同。逻辑可领取点不等于物理同时返回。
5. 一次从已 COMMITTED 开始的本方 authenticated fetch 只按 party index 返回本方密文及已存 token；响应字段相同，ciphertext length/bytes 为 `C_b`，token 与 submit 响应相同。`score_to_mask_v3_escrow.cpp:442-469,584-608,861-867`。A 不返回 peer 密文给该 party。
6. token 是固定 32 bytes。理想独立 A 随机源下 `U←$ {0,1}^256` 局部条件均匀结论沿用 S39/S40；真实 `RAND_bytes` 的 `Adv_DRBG_A(q)` 仍 OPEN。party 不验证 token MAC/签名，当前条件受信 A 模型无需新增 token 认证假设。

首份 deposit 与最终提交不是“两份密文同一 SQL transaction 同时插入”：第一份可已处于 PENDING；第二份、COMMITTED 和 receipt 在最终事务中共同落盘。WAL/FULL 不是物理断电耐久性证明。

## 5. 模拟接口与可执行 sampler

### 5.1 权限

`Sim_b` 的输入是本方 `x_b,z_b,L_S29,pkg_b,static_b` 和公开设置（固定 `τ`、身份、recipient public keys、服务地址/公钥与被批准的密码参数）。它可从理想 `F_T/v1` 取得**仅本方** signed package；T 的签名服务/验证密钥用于验证该包，不给模拟器 T 私钥。它不取得 `pkg_(1-b),z_(1-b),ctx_A`、A 内部状态、peer TLS credential、另一方加密随机带或 reduction-only `J_b`。

理想系统另外将同一次 `F_T/v1` 的 peer package 交给诚实 P_(1-b)，将 `ctx_A` 只交给 A；同一次 `F_TOPK` 的 peer output share 只交给该 endpoint。诚实端点仅按自己的包、份额、凭据、密钥和公开服务设置执行标准加密/submit 客户端。A 只运行源中规定的服务状态机。`Sim_b` 的 submit/fetch 请求与响应在 A 通道上发生；它只读回本方实际 API 响应。

### 5.2 sampler 伪代码

```text
PublicBind(τ, identities, recipient_pks, A_config):
    require route == PROJECT_DERIVED_SELECT_5ITER
    require route_registry(route) == (protocol=v3, circuit, gate_schedule)
    require fresh tx(τ) and both one-shot package leases unused
    register same explicit τ with F_T/v1 and F_TOPK; reject tuple disagreement
    call F_T/v1(protocol=v3, session, invocation, N, K, identities,
                recipient_pk_hashes, circuit, gate_schedule, state_params,
                T_verify_key)
    call F_TOPK(session, invocation, N, K, route, x0, x1)
    F_T internally samples its correlated packages/context and routes pkg_i
      directly to party/endpoint i and ctx_A directly to A
    F_TOPK routes z_i directly to protocol continuation i
    each recipient validates its own fields; no router/simulator receives
      the full tuple of secret outputs; F_TOPK-ATOMIC gates API release

Sim_b(x_b, z_b, L_S29, pkg_b, static_b, public; F_T/v1):
    Vpre_b <- S36_H4_Sim_b(x_b, z_b, L_S29, public, pkg_b, static_b)
    C_b <- EncryptShare(z_b, pkg_b, party=b, recipient_pk_b; fresh encryption tape)
    start/continue the real-format authenticated A client using only b credentials
    receive the fixed AREADY/1 welcome after the pinned TLS server handshake
    submit_b(tx(τ), C_b); wait for A's response caused by two accepted deposits
    R_b <- exact COMMITTED response visible on b's channel
    from already COMMITTED state, request authenticated fetch_b(tx(τ))
    F_b <- exact b-only ciphertext/token response; verify local format as real client
    output (Vpre_b, C_b, W_A, R_b, F_b, API share z_b after COMMITTED)

HonestEndpoint_(1-b)(x_(1-b), z_(1-b), pkg_(1-b), credential_(1-b), public):
    C_(1-b) <- EncryptShare(z_(1-b), pkg_(1-b), party=1-b, pk_(1-b); own tape)
    execute the actual authenticated submit to A
    receive only its own ordinary response; reveal nothing to Sim_b

A(ctx_A, public):
    execute parser/auth checks and database state machine from source
    first accepted deposit -> PENDING (internal to A)
    only after second actual authenticated request, persist second row,
      sample U once from independent ideal A tape, and commit state+U
    return each party's source-format COMMITTED response; fetch_i returns C_i,U
```

`PublicBind` is a deterministic consistency layer, not a joint-law oracle: it makes the existing F_T/F_TOPK calls with their declared sampling algorithms, validates their public tuple fields, and routes outputs directly to their intended recipients. It does not inspect or retain the secret outputs. The two functionalities use their specified independent random tapes for preprocessing and output-share refreshing; no target distribution is restated as an opaque oracle. The peer event is produced by the honest endpoint's encryption and authenticated network request, not by `Sim_b`.

### 5.3 Party-view factorization and hybrid

令 `q_b=(x_b,z_b,L_S29,pkg_b,static_b,public,τ)`，`V_b=(Vpre_b,C_b,W_A,R_b,F_b,API_b)` 为完整的离散应用/API transcript；`W_A` 是 TLS READY 后收到的应用欢迎帧，不包括原始 TLS record、握手 transcript 或 wall-clock 时间。`H_b` 可包含 `z_peer,pkg_peer,ctx_A`、两份密文与 A 的 PENDING 状态。仅针对本轮成功路径，目标是 `Law(V_b | q_b)`，不要求输出或匹配 `H_b`。

定义（`W_A` 是 TLS READY 后客户端实际读到的固定应用欢迎帧）：

```text
Kpre_b(v | q_b)   = S36 已复核 H4 的本方前缀 law
Enc_b(c | q_b)    = 源中 EncryptShare(z_b,pkg_b,pk_b) 的密文分布
U                 <- uniform {0,1}^256, 独立于此前 transcript 与 H_b
tx(τ)             = "s2m-v3-" || hex(session) || "-" || hex(invocation)

W_A = ('A','R','E','A','D','Y',1)
R_b = (status=COMMITTED, tx_len=|tx|, ciphertext_len=0, receipt=U, tx)
F_b = (status=COMMITTED, tx_len=|tx|, ciphertext_len=|C_b|,
       receipt=U, tx, ciphertext=C_b)
API_b = z_b after COMMITTED
```

在理想独立随机带、所有 endpoint 正常完成、两份认证 deposit 均在 A 配置等待期限内被接受且本轮只取成功 transcript 时，源码响应 projection 给出：

```text
Law(Vpre_b,C_b,W_A,R_b,F_b,API_b | q_b, successful execution)
 = Kpre_b(Vpre_b | q_b)
   × Enc_b(C_b | q_b)
   × Unif_256(U)
   × δ[W_A = source_welcome]
   × δ[R_b = source_submit_response(tx,U)]
   × δ[F_b = source_fetch_response(tx,C_b,U)]
   × δ[API_b = z_b] .
```

更直接地，对 S41 的固定条件 `Q_b=(x_b,z_b,L_S29,pkg_b,static_b,Vpre_b,C_b)`，令 `q'_b=(Q_b,public,τ)`。在理想独立 A 随机带且限定成功执行时，服务后缀满足

```text
Law(R_b,F_b,API_b | q'_b, successful execution)
 = Unif_256(U)
   × δ[R_b = source_submit_response(tx(τ),U)]
   × δ[F_b = source_fetch_response(tx(τ),C_b,U)]
   × δ[API_b = z_b] .
```

该条件式的 sampler 只需已获准的 `Q_b`、公开 `tx(τ)` 和理想独立 A token tape；不需抽样 peer hidden state，也不需要让 party simulator 取得 `ctx_A`。它说的是 party-visible 后缀条件 law，不是 `Law(H_b|Q_b)`。

这个等式逐项由本方可见字段决定：`C_b` 的明文为本方已知理想 share；它的新鲜加密带独立于已结束的 GMW tape；A 的 `AREADY/1` 是固定欢迎帧；submit 的 transaction/长度为固定公开值、status 在成功执行里固定为 COMMITTED、receipt 是理想独立 `U`；fetch 按身份只重放 `C_b,U`。API 将 token bytes `U` 编为 64 个小写十六进制字符。诚实 peer 的密文与 context 只影响隐藏服务内部以及“是否存在成功事件”。在当前定理中 peer 是诚实且无故障，A 收到正确 context/package 请求，且请求在配置期限内到达，故该成功事件恒成立；它不是对 `Sim_b` 发出的可选择 oracle。token 局部独立结论依赖 S39/S40，未推导真实 RNG。

**Hybrid 0 → 1：** 将真实 GMW 本方前缀替换为 `S36_H4_Sim_b`。S36 的非作者条件审查在 ideal `F_T/v1`、固定本方 `(x_b,z_b,L_S29,pkg_b)`、单方静态半诚实、成功 transcript 范围内复核了 Beaver 开值、获准比较 outcome、活动 seed/边与最终固定输出 share 的条件模拟。该替换只要求相同 `Kpre_b`，不是对隐藏 peer package/context 的耦合断言。

**Hybrid 1 → 2：** 由同一 `z_b,pkg_b,public pk_b` 执行本方 `EncryptShare`。模拟器与真实 party 用同算法、同 envelope 字段、同分布的新鲜加密 tape；这是本方已知明文的直接重采样，不替换 peer ciphertext，也不声称 A-only 加密安全。

**Hybrid 2 → 3：** 保持诚实 endpoint 和 A 状态机。peer endpoint 以自己的理想输出/package/身份完成认证 deposit；A 按源格式由 PENDING 进入 COMMITTED，采样一次 v2 token 并同时持久化 state/token，再向本方回真实格式响应。服务及 fetch 是 `C_b,U,tx` 的确定性后处理，除一次 token 抽样外不再引入 party 可见随机字段。重复或丢回执、任意重启/abort-prefix 不在这一步的定理范围。

**隐藏状态为何无需相同：** 固定完整 `S_pre` 的续接 kernel 等式仍不推出全视图相等，因为那会把被给定的隐藏联合 law 当成共同事实。本轮不用这条错误推理。对每个固定本方条件 `q_b`，先将真实后缀投影到 party-visible `(R_b,F_b)`；在成功子空间中该投影仅由本方 `C_b`、公开 `tx` 与独立 `U` 决定。因此可将 `H_b` 积分掉而得到上面的显式因子分解。这里不主张、也不需要 `Law_real(H_b|q_b)=Law_ideal(H_b|q_b)`；不从边缘 token law 推导其余字段。若把 A 的 wall-clock 到达顺序、网络旁路元数据、失败/中止前缀纳入 party view，上述投影等式不适用。

## 6. 信任、泄露、优势项和限制

- party simulator 不接收 `pkg_peer,z_peer,credential_peer,ctx_A,T_secret,J_b`。诚实 endpoint 和 A 是 theorem 中排除腐化集合的既有角色，且不与被腐化方合谋；这不是 A-only corruption 证明。
- PENDING、peer ciphertext/hash/length、context/package-set、事务顺序/时间属于 `L_A`。party `L_S29` 未新增字段；party response 本身实际含本方 transaction、状态、长度与 token，这些是代码公开消息字段，不等于把 A 内部元数据增列为泄露。
- A 的 submit response 不返回 peer hash/ciphertext；fetch 依认证身份返回本方记录。本方 share 只在 COMMITTED 之后成为 API 输出；逻辑 commit 与进程的物理返回时刻仍可不同。
- 理想模型中的 sampler 等式是精确的投影 law。它不闭合真实实现桥。后续必须分别保留符号项：`Adv_DRBG_T(q)`、`Adv_DRBG_A(q)`、party envelope RNG `Adv_DRBG_P(q)`、签名不可伪造优势、RSA-OAEP/AES-GCM envelope 优势、TLS 1.3/证书认证优势和 hash-binding 假设。S42 不赋予具体安全位数，不用“哈希看起来随机”证明隐私。
- `J_b-ENC-CHALLENGE/v1` 是 S38 的 reduction-only 对象；S42 没有用它。对端密文并未替换为 dummy，因而本轮不产生新的 IND-CPA hybrid。
- 仅适用应用/API 离散消息 view。S40 正常多进程功能证据作为历史证据继承；本轮未重跑自然通信实验。自然业务读写错误仍 `NOT_REPRODUCED`，S40 proxy 在最终应用帧后的 EOF 不计业务失败；人为故障注入与自然 I/O 证据分离。

## 7. 继承证据与未关闭事项

| 事项 | S42 状态 | 范围 |
|---|---|---|
| S40 receipt/v2 N≤256 功能接收 | `INHERITED_PASS_WITH_SCOPE` | 211 案中 207 个有 share 案例匹配 oracle，4 个预提交中止无 share；本轮不重跑。 |
| S36 H4 本方前缀 | `INHERITED_CONDITIONAL_PASS_REVIEWED` | ideal `F_T/v1`、一方静态半诚实、成功路径；不覆盖真实 Dealer/RNG 或 A suffix。 |
| S39/S40 opaque v2 token | `INHERITED_CONDITIONAL_PASS_REVIEWED` | 仅理想独立 A RNG 的 32-byte token 局部条件均匀性。 |
| S41 hidden-state conditional-law gate | `CONDITIONAL_PASS_REVIEWED_FOR_PARTY_PROJECTION; H LAW NOT_CLAIMED` | 独立复核通过本方可见转录投影 sampler；完整 joint hidden law 仍未知且非此引理目标。 |
| `F_T/v1` → 实际 Dealer | `OPEN_NEXT_PROOF_OBLIGATION` | 本轮若经复核，仅为下一证明义务。 |
| 完整 GMW/Beaver security | `OPEN` | 不从局部 sampler 推导完整定理。 |
| 任意 crash/abort-prefix、A-only 腐化、物理掉电 | `OUT_OF_SCOPE / NOT_PROVED` | 保持原合同，不收窄也不扩张。 |

该条件模拟已由未参与 S41 推导的 reviewer 复核通过其限定范围。下一步只推进理想 `F_T/v1` 到真实离线 Dealer 的桥：Beaver triple 与 permutation shares 的联合分布/独立性、one-hot permutation share 构造、材料 slot/一次性 package 映射、package/context 的签名用途域和同次调用绑定、真实 RNG/DRBG 重启/reseed/并发观察、TLS/认证传输，以及每个替换的符号优势量化。当前尚未做这些证明。

## 8. 验证、运行和文件状态

- `BUILD/TEST = NOT_RUN`：本轮是证明与文档更新，没有 runtime/API/test 变化；不重跑 S40 211 案。
- 独立复核：`/root/s40_independent_receiver` 未参与 S41 推导，只读检查本报告 §3、§5 的条件 law、同次调用、endpoint/A 权限与源码字段；没有改文件或运行测试。其结论为范围限定的 `CONDITIONAL_PASS_REVIEWED`：支持 party-visible success projection sampler，不支持完整安全结论。
- S40 功能证据继承，不计作 S42 新运行；`N>256 = NOT_ADMITTED`；正式 LAN/WAN 与 RTT `NOT_RUN/NOT_MEASURED`；安全 alias 未创建。
- S42 只新增报告、决策、machine-readable gates、source/evidence manifest，并在 `PROJECT.md` 和实施计划追加状态。原 S35–S41 记录、主工作区、S40/S41 worktree、`VFSS-baseline/` 均未覆盖；没有把密钥、材料、证书、构建物或原始大日志加入仓库；未推送或合并。
