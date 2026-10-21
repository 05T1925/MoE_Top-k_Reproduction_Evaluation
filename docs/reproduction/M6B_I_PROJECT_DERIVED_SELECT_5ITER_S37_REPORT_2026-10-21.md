# M6B-I-PROJECT-SELECT-5ITER-S37 条件模拟与恢复审查

日期：2026-10-21
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 1. 裁决摘要

S37 沿 S36 的 H4 条件证明草案，把成功消息转录从输出刷新推进到 escrow receipt、提交响应和身份绑定 fetch。本文定义 proof-only 的 `F_A-ATOMIC/v1` 服务接口：T 的 metadata-only context 只进入诚实 A 角色；P_b 模拟器仅持本方 signed package、公开参数、获准泄露和本方理想 share，并通过 A 服务获得真实 party 接口本会收到的响应。P_b 不得到 A context、peer package 或 T 私钥。

本轮把 P0/P1 的 receipt 与 committed fetch 模拟推进到一个精确缺口，但未闭合：状态为 `OPEN_WITH_EXACT_GAP`。`F_A-ATOMIC/v1` 已把 A context 与 party simulator 隔离，并足以规定 A 如何从 peer ciphertext 生成本方可见 receipt/fetch 响应；但把真实 peer ciphertext `Enc(pk_{1-b},z_{1-b})` 换成 `Enc(pk_{1-b},0^N)` 的 IND-CPA 归约还缺一个只供归约者使用的高效联合 challenge sampler。原稿“逐个固定明文取最坏优势再平均”没有给统一 PPT 归约者生成与本方 pre-cipher view、A context、理想材料和 `z_{1-b}` 相关的联合样本。独立 reviewer 指出此处并确认应降级。缺口不要求把 peer package/share/context 交给 `Sim_b`，也不扩张 `L_S29`；最小补正见第 9 节。

这是证明接口/归约缺项，不是攻击或运行时代码缺陷。当前源码的 party call 只接收本方 recipient public-key path，session/package 中只有两方 public-key SHA-256；因此形式证明还需将两把 public key 明确作为公开 setup 输入。A-only 腐化仍是独立加强模型，任意 crash/abort prefix 隐私明确不在本轮定理范围。S37 按停止条件停在 receipt hybrid 的联合 challenge sampler，不向后组合；真实 T 随机 tape 到理想 `F_T/v1` 的实现桥仍是独立未关闭事项。

S37 没有运行时改动、构建、测试或性能测量。整体安全状态与 receipt/fetch 子引理均为 `OPEN_WITH_EXACT_GAP`；只有 F_A 字段/状态机后处理在 challenge ciphertext 已提供的条件下为 `PASS_WITH_SCOPE`。这些状态不能解释为 `SECURE`、BB90 五次算法复现、BMW16 实现或 BB90+DCF 结果。

## 2. 固定基线与身份

- S37 隔离 worktree 从 S36 commit `ca5eb001e72c971a7dbaf87d789d4a7fbcba1b9a` 建立，其父提交精确为 S35 `77f9684e1a50794004fd31f3e5ee7f193e9b12ec`。创建时 HEAD 精确匹配 S36，工作树干净；没有从 `main` 或其他快照替代。
- S35 的 85 项 manifest 是 `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S35_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-21.json`，文件 SHA-256 `62be2127ae4eb28eb2c6d5d5b1dc2a5d46d068d6a68a62f2c0835708abdc6c07`，canonical source SHA-256 `af693eb61183a606c64a1f1b16ee5c3ad6e4b382ec5456e541c7915b8f95faa0`。在保留的 S35 worktree 中，85/85 个原始文件 SHA-256 与清单匹配。
- S36 相对其 S35 父提交的 Git tree 差异仅为 S36 新增的决策/报告/gates/manifest 和对 `docs/IMPLEMENTATION_PLAN.md` 的 S36 记录；没有 runtime 源码差异。由于 Windows checkout 行尾策略，S37 工作树文本对齐后 84/85 个 manifest 路径与 S35 内容相同；唯一语义差异是 S36 已追加的 `docs/IMPLEMENTATION_PLAN.md`。`docs/REFERENCE_MANIFEST.md` 与历史 S14 报告的工作树原始字节哈希不同，但换行归一化后内容一致。S37 审查的 runtime 对象仍是 S35 精确父提交中的代码；没有静默替换基线。
- S31 是规格/TEST_ONLY 技术审查；S32 是作者实现与功能记录；S33 的原样版本后来由 S34 复现 SIGPIPE 缺陷，不能视为独立 runtime 接收；S34 修复版通过所列定向回归；S35 才对 metadata-only A 与签名域隔离完成非作者功能接收；S36 独立核验 H4 的 ideal `F_T/v1` 成功路径条件化步骤。S37 仅延续 S36 证明边界，不改写此前结果。
- S35 final binary SHA-256 `f511f262e574ddb430317894f63a97e725dd0a62e3f475e21bd44f0da94984ba` 仅作为历史 manifest 记录引用；S37 未重新核验或重建该二进制。

证据类型保持分开：论文定义不被本项目源码反推；S29/S31 是项目规格；GMW/Beaver-v3、Select5 与在线 A 是项目派生实现；195 案 E2E 是有限测试观察；`F_T`、签名/哈希/加密/TLS 优势和服务耐久性仍是证明假设或待验证项。

## 3. 角色、理想输出和获准泄露

| 角色 | 基线持有/可见值 | S37 模型 |
|---|---|---|
| T | 全局 Beaver tape、shuffle permutation、T 签名私钥；离线生成两个 party packages 和 escrow context | 可信离线发材功能 `F_T/v1`，排除腐化集合；不与任一在线方合谋。签名由 oracle 按固定 verifier key 生成，在线模拟器不取得 T 私钥。 |
| P0/P1 | 本方输入 share、signed party package/材料、本方输出 share、本方 TLS 身份和 recipient 私钥；peer 消息、A 响应 | 逐个静态半诚实腐化一个 party。模拟器只取本方 package，不取 peer package 或 A context。 |
| A | signed metadata-only context、TLS 身份、请求、两份密文及摘要、事务状态/receipt、请求顺序/时序；无 share 解密私钥 | 条件受信、诚实且与 T/P0/P1 不合谋。A-only 腐化是单独加强模型，不属于基线 P_b 定理。 |
| 环境/网络 | 消息顺序、传输结果和时钟 | 基线成功转录使用消息级顺序/长度；A 的 arrival order/timing 按既有 `L_A`。`L_S29` 没有增加 party wall-clock time 字段。 |

目标功能仍是 S31 冻结的 `F_TOPK-ATOMIC` 与 `F_TOPK-U`：稳定 signed Q20.12 降序、同分按原始 index 升序，输出恢复原序且恰有 K 个 1 的两份 XOR mask shares。commit 前 Party API 输出 `⊥`；最终 `COMMITTED` state/receipt 是双方可恢复各自 share 的共同逻辑可见点。两个进程/两个 API 的回执到达及返回时刻不保证物理同时。逻辑 commit 不表示尚未返回 API 的 party 进程内从未形成本地 share。

`L_S29` 逐项沿用 S29 §3.3：公开参数/协议绑定；获准公开的 shuffle endpoints/positions；pivot 与活动比较结果 bit；仅在 forward shuffle 后按活动轮公开的 sample seed；原规格中的 approved masked values 类别；comparison count、通信长度、统一 ABORT。S37 不把 receipt、context hash、peer package hash 或密文哈希加入 `L_S29`。这些值若 party 可见，仍是需模拟的 view 字段。

`L_A` 独立沿用 S35：signed context 元数据、session/invocation、N/K、身份与证书绑定、circuit/schedule 与 package-set/context 摘要、双方 ciphertext bytes/hash/length、到达顺序和时序、PENDING/COMMITTED/ABORTED、receipt、认证 fetch 与 restart 状态。`package_set_sha256`、`context_sha256` 和双方密文哈希仅直接出现在 A 的视图；P_b 看到自己的 package hash/请求及 receipt，但不直接收到 peer ciphertext、peer package hash、context 或 package-set digest。A 不持解密私钥。

## 4. 消息 DAG 与源码观察字段

源码位置以下均以固定 S35 tree 为准。

| 顺序 | 源码行为 | party b 实际可见的应用值 |
|---|---|---|
| 1 | escrow TLS handshake；客户端验证 A 证书 pin，服务端验证连接方 party 证书 pin，见 `score_to_mask_v3_escrow.cpp:551-570,651-679` | TLS 记录与固定长度 `AREADY\x01` 应用 payload（7 bytes）。pin/handshake 失败时没有 AREADY 或结构化拒绝响应。 |
| 2 | `make_request` / `parse_request`，同文件 `:119-209` | submit payload 字段顺序：magic u32、envelope version u8、op u8、party u8、reserved u8、session 16、invocation 16、circuit hash 32、schedule hash 32、本方 package hash 32、transaction length u16、ciphertext length u32、transaction bytes、ciphertext bytes。固定头 142 bytes；transaction 为 `s2m-v3-<session-hex>-<invocation-hex>`，长度 72 bytes。RSA-2048 时 share envelope 为 `298+N` bytes，因此 submit 应用 payload 为 `512+N` bytes；fetch request 无 ciphertext，payload 为 214 bytes。 |
| 3 | A `submit_ciphertext`，同文件 `:300-427` | party 提交的是本方收件人密文。A 第一份 deposit 可成为 PENDING；第二份在最终 SQLite transaction 内产生 receipt、改为 COMMITTED。party 不收到 peer 密文或显式 `PENDING` 状态。重复同一密文幂等；同交易更换密文被拒绝。 |
| 4 | `make_response` 与 `parse_response`，同文件 `:460-470,630-649` | response 应用字段顺序：status u8、transaction length u16、ciphertext length u32、receipt 32 bytes、transaction bytes、ciphertext bytes。submit commit 响应无 ciphertext，长度为 111 bytes；party 收到 receipt 后封装层转为 64 个小写 hex 字符，见 `:834-844` 与 `score_to_mask_v3_party.cpp:725-765`。外层 Protocol-I frame/TLS 记录另有 framing 开销，本文不把 payload 长度当作完整 wire length。 |
| 5 | committed fetch，escrow 同文件 `:430-457,591-596,846-856` | FETCH request payload 为 214 bytes；响应含同一 receipt、transaction 与**仅请求 party 自己**的原 recipient ciphertext。RSA-2048 下响应 payload 为 `409+N` bytes。恢复方用本方 private key 解密；A 不解密。重复 fetch 返回相同 ciphertext 与 receipt。 |

Receipt 输入与源码 `make_receipt` (`score_to_mask_v3_escrow.cpp:245-266`) 一致：

```text
SHA256(
  "score-to-mask-v3/escrow/commit/1" || transaction ||
  session[16] || invocation[16] || u32(N) || u32(K) ||
  circuit_sha256[32] || gate_schedule_sha256[32] ||
  context_sha256[32] || package_set_sha256[32] ||
  SHA256(C0)[32] || SHA256(C1)[32])
```

其中 `C0/C1` 按 party index 排序；每一份 `cipher_hash` 在 deposit 时由 `SHA256(request.ciphertext)` 得到 (`:359-385,396-412`)。源码的 `append_u32` 编码和拼接顺序是规范输入；receipt 没有额外签名字段。响应状态是 COMMITTED/ABORT 状态字节，不额外发送 SQL `state` 列或 peer 密文。

## 5. `F_A-ATOMIC/v1` 与两方 success-path sampler

### 5.1 角色路由和接口

`F_T/v1` 将 signed party package `pkg_b` 只交给 P_b 角色，把 signed metadata-only context `ctx_A` 只交给 A 角色。P_b 模拟器接口写为：

```text
Sim_b^success(x_b, z_b^commit, public, L_S29;
              F_T/v1, F_A-ATOMIC/v1),  b ∈ {0,1}
```

这里 `z_b^commit` 是逻辑 commit 时本方理想 share；它不等价于 Party API 一定已经返回 share。`F_A` 单独给出本方收到的 submit/fetch 响应或投递状态。若采用 `⊥` 表示“API 尚未返回”而不区分已 commit，则无法生成该 party 在提交前已经形成并加密的本地 share；S37 不用这种含混解释覆盖故障前缀。正常成功路径中 `z_b^commit` 与最终 API share 相同。

`F_A-ATOMIC/v1` 是证明用的版本化诚实服务接口，不声称 S35 runtime 导出了此理想 API。其状态为 `EMPTY → PENDING(one ciphertext) → COMMITTED(two ciphertexts + receipt)`，或 PENDING 经超时/服务进程 restart 进入 ABORTED。接口要求：

1. 只接收由 T 签名并绑定 session/invocation、身份、N/K、circuit/schedule、两个 package hashes/package-set hash、服务状态参数的 metadata-only context；该值只传给 A 模拟器/服务。
2. A 的 party-b endpoint 认证证书 fingerprint 与 party 字段；校验 context/session/transaction/package digest。P_b 模拟器不读 `ctx_A`；“honest peer deposit”由服务模拟器内部调用，A 用 context 中对应 peer package digest 完成校验，调用方不取得 peer package。
3. 对同一交易同一 ciphertext hash 的重复 submit 保持幂等；同 party 不同 hash 拒绝。两份收件人 ciphertext 存在后，A 按源码 receipt 编码计算 receipt，final state 与 receipt 原子地转换为 COMMITTED。服务只将该 party 在线接口可见的状态/receipt 响应交给 Sim_b。
4. COMMITTED fetch 只返回当前请求 party 的原 ciphertext 和 receipt；ABORTED/PENDING fetch 拒绝；wrong TLS identity/错误 party 绑定在 AREADY 前或 request 校验处拒绝。服务重启后仅保留已提交记录的行为以逻辑状态建模，不推出物理掉电耐久。
5. `public` 中显式含 `pk_recipient[0], pk_recipient[1]`（或可验证的公开 key-registry oracle），其 DER SHA-256 与 session 中双方指纹相符。公钥本来是公开 setup 数据，不并入 `L_S29`。S35 `ScoreToMaskV3Session` 只保存这两把 key 的 SHA-256 (`score_to_mask_v3_gmw.h:30-38`)，而 `score_to_mask_v3_execute_party` 只收本方 PEM path (`:203-213`)，E2E 为两方分别配置公钥 (`score_to_mask_v3_process_e2e_test.cpp:253-264`)；因此该公开双 key registry 是 S37 明确补入 simulator/public setup 的最小字段。若调用环境实际只给指纹、不提供 peer public key，此接口条件未满足，receipt hybrid 仍为 OPEN_WITH_EXACT_GAP；这不是新泄露字段。

服务状态、A context、A 看到的到达顺序/时序都留在 A 角色。P_b 模拟器只收自己的 `pkg_b` 和通过 A endpoint 对 party b 可见的应用响应；F_A 不将 context、peer package、peer ciphertext/hash、T 私钥或服务数据库状态回传给 Sim_b。

### 5.2 Receipt hybrid

沿用 S36 已独立复核的 H4 sampler，生成直到本方逻辑输出 share 的 party view。模拟器对本方真实消息和密文作同分布采样：

1. 用本方 package、`x_b` 和 H4 sampler 得到本方本地状态、公开活动结果与固定的 `z_b^commit`。此处仍以 S36 ideal `F_T/v1`、获准 `L_S29` 及成功路径为前提。
2. 生成 `C_b = Enc(pk_b, z_b^commit; r_b)`，格式/AAD/随机字节按 `score_to_mask_v3_party.cpp:370-440`：AES-256 key 32 bytes、nonce 12 bytes、RSA-OAEP SHA-256/MGF1-SHA-256 包装 AES key；AAD 为 session、invocation、party、circuit hash、schedule hash；envelope 为 magic `S2MENC3\x01`、RSA 长度 u16、share 长度 u32、nonce、GCM tag、RSA ciphertext、GCM ciphertext。`z_b` 是 N 个 byte 编码的 bit share。
3. 以 `pk_{1-b}` 对 `0^N` 生成格式相同的 `C^*_{1-b}`，AAD 中 party 为 `1-b`。固定 recipient key 后 RSA ciphertext 长度固定；GCM ciphertext 长度等于 N。因此真实 peer share ciphertext 与 dummy ciphertext 长度相同。
4. 由 F_A 接收本方模拟 submit 与内部模拟的 honest-peer submit。`ctx_A` 内的 package-set/context digest、服务 transaction state 和两份密文摘要仅由 A 使用。A 按真实状态机计算 `receipt^*` 并回给 party b 的 response channel。P_b simulator 只看到 AREADY、COMMITTED response 的 receipt/transaction/status 和本方之后 fetch 可见的 ciphertext/receipt。
5. 从真实 `Enc(pk_{1-b},z_{1-b};r)` 到 `Enc(pk_{1-b},0^N;r*)` 的目标 hybrid 是正确的候选，但当前不能从已写的 `Sim_b` 输入直接构造统一 IND-CPA reduction。对每个具体明文取最坏优势再平均不足以定义 reduction，因为尚无高效程序生成该明文与 `V_b^pre`、`ctx_A`、`pkg_b`、`z_b` 和 `L_S29` 的联合条件分布。
6. 最小 proof-only 修正是定义 `J_b-ENC-CHALLENGE/v1`，与 `Sim_b` 分离：对环境提供的输入对和公开 setup，它从理想 `F_T/v1` joint tape 及诚实 peer 执行高效联合采样 `(z_{1-b}, z_b^commit, L_S29, V_b^pre, pkg_b, ctx_A)`；将 `(z_{1-b},0^N)` 交给 IND-CPA challenger；只把 challenge ciphertext 路由进 A 的状态机；A context 只交给 A；`Sim_b` 只接收其真实接口可见的 AREADY/receipt/status/本方 fetch。此 oracle 不把 peer package、peer share、A context 或 T 私钥交给 `Sim_b`，不作为新增泄露，也不修改 `L_S29`。需要证明 J_b 高效且确实保持真实 joint correlation；S37 尚未定义或验证它。
7. 若上述 sampler 成立，则 peer ciphertext 的 SHA-256、receipt SHA-256、数据库状态转换、receipt response 与之后的 own-ciphertext fetch 都是同一 F_A 状态机对挑战 ciphertext 的有效率后处理；不需要假设 SHA-256 输出均匀或随机。碰撞/绑定性质另保留 `Adv_hash`，不用于把 receipt hash 伪装成随机 oracle。recipient envelope 的 IND-CPA 还依赖 RSA-OAEP key-wrap 与 fresh-key AES-256-GCM DEM 的组合安全、随机 AES key/nonce 和固定 AAD。

因为 `z_{1-b}` 与 `z_b`、`L_S29`、活动比较结果可以相关，hybrid 必须覆盖完整 joint execution。H4 的局部边缘式或密文无条件均匀性都不足以替代 J_b。若真实输出 share 可由获准 L 直接计算，模拟泄露本身已经允许该结果；receipt hybrid 不新增这类 leakage。

成功路径条件等式写为：

```text
Law(V_b^succ, receipt, fetch_b | x_b, z_b^commit, public, L_S29; F_T/v1, real A)
  ?≈_c
Law(Sim_b^success(x_b,z_b^commit,public,L_S29; F_T/v1,F_A-ATOMIC/v1))
```

缺少 J_b 时上述等式没有闭合。若 J_b 被形式化并由 reviewer 核实，两方分别以 b=0 和 b=1 运行同一加密 hybrid，每次只替换未腐化方 ciphertext。候选总优势账本为：

```text
Adv_b ≤ ε_H4 + ε_FT + Adv_DRBG(q) + Adv_signature
        + Adv_RSA-OAEP/AES-GCM-IND-CPA(1) + Adv_TLS
        + Adv_hash(binding) + ε_F_A-model + ε_J_b + ε_timing
```

符号均未量化。当前 `ε_J_b` 不是已界定的密码学优势，而是缺失的 joint-sampler correctness/indistinguishability 条件，故 receipt hybrid 不能判 PASS。`Adv_hash(binding)` 指上下文/交易/收据绑定或碰撞目标；单纯对已替换密文取 SHA-256 并作为有效率后处理不需要额外“hash looks random”项。`ε_timing=0` 仅适用于明确不把 party wall-clock latency 放入 view 的消息级定理；A 的 timing 仍属于 `L_A`。SQLite/WAL 不证明物理 durability。

## 6. receipt、lost receipt、fetch 与重复请求

| 执行点 | A 逻辑状态/Party API 可见结果 | S37 模拟范围 |
|---|---|---|
| 正常双 submit 与 receipt | 最终 transaction commit 已写 state=COMMITTED 与 receipt；两个 service thread 各发送相同 receipt；客户端收到时间可不同，Party API 各自返回 `COMMITTED` 和本方 share | F_A 状态机字段与 hash 后处理可描述；真实 peer ciphertext 到 dummy 的统一 IND-CPA 归约因缺 `J_b` 暂为 `OPEN_WITH_EXACT_GAP`。 |
| commit 后 receipt 被丢弃 | A 已 COMMITTED；失去响应的一方 catch submit error，返回 `RECOVERY_REQUIRED/COMMIT_STATUS_UNKNOWN`、transaction ID，不把 share 放进 API output；但本地 `local_output` 已形成且密文已生成，不能称为“内存中从未有本方 share”。未丢响应的一方可已返回自己的 share | 故障事件本身不在主定理。F_A 可在已 COMMITTED state 上模拟指定 delivery-loss 之后的服务调用；理想接口必须将 `z_b^commit` 与 API delivery status 分开。 |
| 已提交后的 authenticated fetch/restart | fetch 返回存储的本方密文与共同 receipt；重启不撤销 COMMITTED。测试复跑曾验证两方解密到各自 share | 从已提交状态开始的本方密文/receipt 是 A 状态机后处理；它与真实协议的计算方模拟连接仍待 `J_b`。不证明产生 COMMITTED 前的任意崩溃前缀，也不证明断电耐久。 |
| 重复 fetch/重复同密文 submit | 返回相同 ciphertext/receipt；同交易同 party 改 ciphertext 被拒绝 | F_A 状态机可确定性重放；模拟器必须缓存并复用 `C_b`，不能每次重新加密。 |
| 错误 party 身份 | 服务端 party endpoint 先校验证书 pin；错误身份在 AREADY 或请求处理前断开/拒绝，无 share ciphertext 给错误身份。测试只验证客户端调用抛出拒绝 | 作为被指定的服务拒绝调用可模拟；真实 TLS close/alert 时序不作密码学 privacy 结论。 |

源码恢复观察：submit ambiguous catch 在 `score_to_mask_v3_party.cpp:729-744`；receipt 格式检查及 API share release 在 `:746-773`；通用 abort 会清空返回字段但不证明栈上从未有局部 share，在 `:774-789`。A startup 将 PENDING 改 ABORTED (`score_to_mask_v3_escrow.cpp:522-527,804-825`)，fetch 只允许 COMMITTED (`:430-457`)。S35 E2E 的丢 receipt/重启/fetch/重复 fetch/错误身份案例位于 `score_to_mask_v3_process_e2e_test.cpp:960-1029,1060-1144`；这是历史故障注入证据，S37 没有重跑。

## 7. A-only 加强模型

A 在本轮基线被排除于腐化集合，因此**A-only simulator 不是 P0/P1 基线定理的必要 gate**。它仍是单列的加强模型，不能和 `L_S29` 合并。

候选 A-only view/泄露函数 `L_A` 记录：T 签名的 metadata-only context 与其签名；session/invocation、N/K、身份/证书指纹、circuit/schedule、两个 package digest/package-set/context hash；两份 recipient ciphertext 的 bytes/hash/length；transaction/PENDING/COMMITTED/ABORTED、receipt、到达顺序/时序、TLS peer identity、restart/fetch 请求和返回值。它不包括明文 share、triple、permutation share、输入 score 或解密私钥。

候选 `Sim_A(public, L_A^events; F_T/v1)` 由 F_T 将 signed context 只交给 A 角色；模拟器以公开 `pk0,pk1` 各自生成 `Enc(pk_i,0^N)`，计算密文哈希与源码同格式 receipt，再依事件序列输出 PENDING、COMMITTED、receipt、重启和 own-fetch 记录。替换顺序为先 C0 再 C1，分别用 recipient IND-CPA/AEAD；package-set/context hashes 是 F_T 提供给 A 的签名 metadata，receipt/state/fetch 是密文替换的后处理。候选界含 `Adv_enc(pk0)+Adv_enc(pk1)+Adv_TLS+Adv_hash+Adv_DRBG+Adv_signature+ε_timing`，不填数值。

这里 `L_A` 的 ciphertext/hash 字段是 A transcript 中由模拟器生成的 view 输出；如果未来把“真实执行所得的具体 ciphertext/hash 值”当作 simulator 的外部输入，dummy replacement 就不再是待证明分布，而需由理想功能明确这些字段的生成权限。该理想输入/输出拆分尚未由 S35/S36 冻结，故加强模型继续标为 `OPEN`。基线条件受信 A 模型的 P_b receipt hybrid 也因 `J_b` 缺项而开放；这不是 A-only 攻击。

## 8. Abort/crash 定理边界

主定理范围明确限于**无 I/O、Party 进程、A 进程故障的成功消息转录**。另可条件地模拟“已知 A 状态为 COMMITTED 后的一次成功 authenticated fetch”服务子调用；这不是对其之前丢包、SIGKILL 或服务崩溃前缀的隐私定理。此范围保留 S31/S35 的 `F_TOPK-ATOMIC` 原合同，不用测试观察收窄合同。

| 故障/切点 | 已有功能合同/历史证据 | S37 安全证明范围 |
|---|---|---|
| PENDING 一方密文已落库，peer 未提交；超时或 A 进程 restart | A 源码只把 state=0 改为 ABORTED；fetch 拒绝。S35 定向测试观察双方 API 不返回 share，lease 已烧毁 | 任意已交付 byte/frame prefix、party 栈内局部 share、异常/超时联合 view 不模拟。`⊥` 代表 API 未返回，不代表 local share 未形成。 |
| 第二次 deposit/最终 commit 边界 | 第二 ciphertext row 与 `state=COMMITTED`、receipt 在第二次 submit 的最终 SQL transaction 内写；首个 PENDING row 已可更早单独提交 | 只在成功消息路径及从确定 COMMITTED 状态开始的 fetch 子调用中使用该状态机。SQLite WAL/FULL 和进程重启不构成掉电或介质耐久证明。 |
| commit 后 receipt response 丢失 | S35 注入后 P0 是 `RECOVERY_REQUIRED/COMMIT_STATUS_UNKNOWN`、API share 字段为空；P1 可已 COMMITTED；之后两方可各自 fetch | 仅 F_A 状态接口可模拟已提交后的 recovery 调用。丢响应前缀、P1 比 P0 先返回等整体故障转录 out of scope。 |
| P1 在提交前被杀 | S35 人工 SIGKILL 案例是有限功能观察；P1 无 API return，A pending 行后被终止/abort，P0 无 API share | 不证明该具体或其他 crash-prefix 的视图隐私；不将 “NO complete mask” 写成双方都没形成自己的 share。 |
| P0/P1 重复 fetch、错误身份；A restart 后 fetch | S35 指定案例通过；错误身份拒绝；相同 fetch 返回同一 cipher/receipt。仅进程级 restart | 操作在指定服务状态上的结果可作为功能观察；攻击者控制的任意身份/OS/磁盘/物理断电不纳入 theorem。 |

若未来把故障前缀纳入 theorem，最小版本化规格需在 `F_TOPK-ATOMIC/v2` 中显式输入环境 cut：已交付 frame/byte prefix、PENDING/COMMITTED/ABORTED、receipt 是否生成/到达、每方 API 是否返回、被腐化方可见的栈内 share 状态与后续 recovery capability。当前不把这些新增字段加入 `L_S29`，也不把它们冒充已证明泄露。S25 历史中止语义仍按 S31 决策保持独立，不追溯修改。

## 9. 沿消息链之后的精确剩余断点

### 9.1 首个精确断点：IND-CPA reduction 缺少联合 challenge sampler

真实变量是诚实 peer 的最终输出份额 `z_{1-b}`，由 `score_to_mask_v3_party.cpp:693-744` 的协议输出/提交路径形成，再传给 `encrypt_share` (`:370-440`)；该份额会与本方 pre-cipher view、共同输出 mask、活动比较结果、`L_S29`、T package/context metadata 有相关性。模拟器现有输入只有 `(x_b,z_b^commit,public,L_S29,pkg_b)`，没有 `z_{1-b}`、peer package、`ctx_A` 或 T 私钥。

当前缺失的条件分布/采样接口为：

```text
J_b-ENC-CHALLENGE/v1(x_0,x_1,public; F_T/v1) ->
  (z_{1-b}, z_b^commit, L_S29, V_b^pre, pkg_b, ctx_A)
```

它必须高效联合采样并保持真实相关性；`ctx_A` 仅路由给 A 状态机，其余 reduction 内部值仅给 IND-CPA reduction。归约将 `(z_{1-b},0^N)` 交给挑战者；只有 challenge ciphertext 进入 A，A 只将真实服务响应交给 `Sim_b`。缺少 J_b 时，单靠“对每个固定明文取最坏优势再对条件分布平均”不能证明存在统一 PPT reduction；也无法从 `Sim_b` 不可获得的 peer 值自行构造挑战对。独立 reviewer 将这一点判为首个精确缺口。它是**证明接口/归约缺项**，不是攻击、代码缺陷或新增泄露。

最小修正是补充 proof-only `J_b-ENC-CHALLENGE/v1`（或等价的 F_A challenge hook），由理想 `F_T/v1` 的 joint setup 与诚实 peer transition 提供挑战明文和 correlated view，但严格不把 peer package/share/context/T key 交给 `Sim_b`；证明 sampler 高效且保留 package-set/context hash 和本地输出间的条件相关性，再由非作者复核两方 reduction。该修正不改变运行时 API、`F_TOPK-ATOMIC`、`L_S29` 或实际角色。现阶段 S37 停止，不沿假设中的加密 hybrid 继续组合。

### 针对 receipt 后处理的有限概率核查（辅助）

为检查“密文替换后，hash/receipt 后处理是否会单独破坏条件分布”这一点，取一位明文 `M`、任意相关 side view `W`，包括 `W` 与 `M` 完全相关的情形；以独立均匀 `R∈{0,1}` 构造理想 OTP toy ciphertext `C=M⊕R`，dummy 为 `C*=0⊕R'`。对任意联合分布 `p(w,m)` 和每个 `w,c`：

```text
Pr[W=w,C=c] = Σ_m p(w,m)·Pr[R=c⊕m] = Pr[W=w]/2
Pr[W=w,C*=c] = Pr[W=w]·Pr[R'=c] = Pr[W=w]/2
```

故这组两明文/两密文值可精确枚举，联合分布全相等；任何确定性 receipt/hash/state 后处理仍全相等。该小核查只验证“在加密挑战已隐藏明文时，相关 side view 与确定性后处理可被一并带过”的概率结构；它不验证 RSA-OAEP/AES-GCM、不模拟 S35 bytes，也不关闭真实 `RAND_bytes`/签名到 `F_T/v1` 的计算桥。这里未运行构建或测试。后续若要把真实混合正式化，最小接口改造方向是版本化 `F_T/v2` 的带标签 tape/随机性 oracle（按 session、invocation、gate/material type 做独立派生）和签名 oracle，再由真实生成器的完整调用序列给出 `q` 与优势 reduction；Party simulator 仍只得到本方 package，context 仍只路由给 A，不扩大 `L_S29`。

该 toy 核查不替代真实 reduction，也没有发现反例；它只验证后处理性质，不能弥补缺失的 J_b。若 J_b 后续闭合，仍需另行审查实现级 T 随机带、package/context 签名分布及 DRBG 优势；这些后续步骤不属于 S37 已闭合结果。

wall-clock timing 另为明确范围边界：A 的到达顺序/时间已列在 `L_A`，A-only 模型处理它；party elapsed latency 不在 S29 原始 `L_S29` 字段中。若把外部可精确计时的 party 时延纳入 view，需要冻结单独的时序功能/泄露与网络调度假设，不在 S37 暗中加到 L。

## 10. 独立复核、验证和门禁

非作者 reviewer 独立复核 receipt 输入顺序、提交/fetch 返回形状、F_T 到 A 的角色路由、双 recipient key 公开输入、hash/receipt 后处理和 abort/crash 范围；同时发现当前 IND-CPA hybrid 缺少 reduction-only 联合 challenge sampler，要求该子引理维持 `OPEN_WITH_EXACT_GAP`。复核身份和精确缺口写在本报告末尾与 S37 gates/manifest。reviewer 未编写本证明、未修改代码或运行测试。

本轮验证：仅只读审源和证明文档；**未构建、未运行测试、未重跑 195 案、未测量资源/性能**。因此自然 I/O 故障沿用 S35 记录为 `NOT_REPRODUCED`，S37 本轮重跑状态 `NOT_RUN`；不能声称错误不存在。N≤256 仅保留 S35 指定 oracle/E2E 样本的历史 `PASS_WITH_SCOPE`；N>256 `NOT_ADMITTED`。正式 LAN/WAN 与 RTT `NOT_RUN/NOT_MEASURED`。算法迭代五次、GMW 比较依赖层数、exchange phase 与网络 RTT 分别报告，不互相替代。v3 默认 OFF，未建 `secure` alias，未修改 `VFSS-baseline/`。

| Gate | S37 状态 | 边界 |
|---|---|---|
| S36/S35 固定源身份 | `PASS_WITH_SCOPE` | S35 保留 worktree 85/85 原始哈希通过；S37 当前文本仅 S36 已改的实施计划不同，其余 84 项换行规范化一致。 |
| S36 H4 | `CONDITIONAL_PASS_REVIEWED` | 继承其 ideal F_T、单方静态半诚实、成功路径条件证明。 |
| P0/P1 receipt + committed fetch 子引理 | `OPEN_WITH_EXACT_GAP` | `F_A-ATOMIC/v1` 状态机与后处理已核对；缺 reduction-only `J_b-ENC-CHALLENGE/v1` 以联合采样 peer plaintext 与本方 pre-cipher view/context。 |
| 确切 source 的 F_A API / public key registry 绑定 | `CONDITIONAL` | 作为 proof interface 定义；当前 party API 只收本方 key path，真实 public registry 接线未实现/未重跑。 |
| A-only simulator | `OPEN` | 非基线目标；L_A 字段由 ideal functionality 生成/输入的精确拆分待冻结。 |
| crash/abort-prefix privacy | `OUT_OF_SCOPE_FOR_S37_THEOREM` | 原 F_TOPK-ATOMIC 合同与历史 no-share/recovery 测试保留，不声称故障前缀密码学安全。 |
| receipt IND-CPA reduction sampler | `OPEN_WITH_EXACT_GAP` | 首个停止点；缺少 `z_peer` 与 `V_b^pre,ctx_A` 的高效联合采样权限。 |
| later real F_T distribution / DRBG / signature / hash bridge | `OPEN_NOT_QUANTIFIED` | 尚未沿过首个断点继续；后续需审查 gmw.cpp:515-585 的联合材料与 package/context 生成律。 |
| natural I/O fault | `NOT_REPRODUCED` | S35 历史观察；S37 `NOT_RUN`。 |
| build / tests / N=128/256 rerun | `NOT_RUN` | 本轮 proof/documentation only。 |
| N>256 | `NOT_ADMITTED` | 无变化。 |
| formal LAN/WAN / RTT | `NOT_RUN / NOT_MEASURED` | 正式 V3→V4 前置门仍在。 |
| v3 default / secure alias | `OFF / NOT_CREATED` | 无变化。 |

### 独立复核记录

reviewer：`s37_receipt_hybrid_review`（未参与 S37 证明撰写）。结论：F_A 状态机/字段与后处理 `PASS_WITH_SCOPE`，receipt hybrid 整体 `OPEN_WITH_EXACT_GAP`。reviewer 指出“固定明文最坏优势再平均”没有给统一 IND-CPA reduction 一个高效联合 sampler，使其生成 `z_peer` 与 `V_b^pre,ctx_A` 的真实相关分布；最小补正为 reduction-only `J_b-ENC-CHALLENGE/v1`，只将 challenge ciphertext 路由进 A，不向 `Sim_b` 暴露 peer package/share/context/T 私钥。其余已复核项：receipt 输入与顺序、response 长度、F_T context 角色路由、双 public key setup 不扩大 `L_S29`、逻辑 commit/API 返回区别及 crash-prefix 边界。未运行测试、未改源码。reviewer 明确建议将 receipt/fetch gate 保持 OPEN，未批准完整安全定理。
