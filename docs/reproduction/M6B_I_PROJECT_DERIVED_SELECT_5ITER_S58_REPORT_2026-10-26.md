# M6B-I PROJECT_DERIVED_SELECT_5ITER S58 详细复核报告

日期：2026-10-26
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 1. 结论

本轮裁决：**现有 `F_T/v3`、`F_TOPK` 与托管恢复合同不足以支持不按成功事件筛选的单方完整视图模拟。**缺少的不是某一个已证明的攻击，也不是 cap-abort 事件本身的抽样定义，而是同一调用中“发材结果 → 包/上下文交付 → party 启动或通知 → `F_TOPK` 输出 → A 的交易状态 → 每方 API/恢复观察”的联合状态转移接口。

成功模型与失败模型必须分开陈述：

| 范围 | S58 裁决 | 精确适用范围 |
|---|---|---|
| 正常成功应用消息视图 | `PASS_WITH_SCOPE`（继承） | 理想 IID `F_T/v3` 且 `cap_success`、理想 A token RNG、一方静态半诚实、T/A 按既定模型条件受信且不与腐化方合谋、诚实 peer 按自身输出/package/凭据因果提交、`1≤K≤N≤256`，以及外生 `SuccessFence` 保证双方合法消息在截止前完成。 |
| 协议级中止的无条件 party view | `OPEN` | 理想 FTv3 cap-abort 已定义；但同一 invocation 是否交付、是否启动 `F_TOPK`、各 party 是否获知失败及 API 结果没有组合规则。其他协议拒绝/主动中止/连接失败也未形成同一状态核。 |
| 底层部署故障的无条件 party view | `OPEN` | 实际 signer/marker/RAND/package 发布、A token RNG/SQLite、TLS/timeout/重启等的联合故障分布和 party-visible 投影未建立。它们可被排除在一个明确的成功执行隐私定理之外，但不能据此声称失败视图隐私已证明。 |
| 不筛选成功/失败的完整单方模拟 | `OPEN` | 没有可从当前功能接口调用的无条件完整 transcript/status sampler；也没有目标部署概率 kernel 或成功概率下界。 |

所有结论仍是项目派生路线的范围审查，不是 BB90/BMW16 原协议复现或完整安全通过。S58 没有改 runtime/test、没有跑构建或测试，未重跑历史矩阵和性能测试。

## 2. 固定对象、来源核验与清单勘误

### 2.1 精确对象

S58 在独立 worktree `C:/Users/28641/.codex/worktrees/m6b-i-project-select5-s58/MoE_Top-k_Reproduction_Evaluation` 的短分支 `codex/m6b-i-project-select5-s58` 开始。固定 HEAD 为 `79f61f6298e0da65b7c64ea4e748bee1afd1ebb1`，tree 为 `e18037236c7c58089a8ac58b792adf9716b56b29`，父提交为 `66a3db485ffa9ac752a49fd0f8a0f100fcef4cbe`。本轮开始时工作树干净；不从桌面主工作区复制文件，不改 S57 历史对象。

S57 manifest `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S57_SOURCE_EVIDENCE_MANIFEST_2026-10-25.json` 在 S58 检出中的 SHA-256 为 `6c6417a80708e1683b56b8cda290b334d26f328998f81099eaf204284db617fe`。重核结果：

- S57 manifest `primary_source_files`：7/7 checkout SHA-256 与 Git blob 全匹配。
- S57 manifest `evidence_inputs`：9/9 checkout SHA-256 与 Git blob 全匹配。
- manifest 继承的 S55 固定运行时源文件：15/15 checkout SHA-256 与 Git blob 全匹配。
- S56 历史交付摘要：记录值和按原 manifest 文件行重算值均为 `22534d20fbe5e178d495e6b0326334021351379fafd40ac3c8ea602016418be6`。
- S57 `canonical_delivery.sha256`：按其定义对 manifest 记录的四个 delivery hash 重算为 `11dec51d82ec56ee73193b39f5f7e205f04ee2b681e5cc7426b508527bdd0389`，与历史记录相符。该复算验证的是历史 manifest 所记录的 hash 行，不替代逐文件对 Git HEAD 的核验。

### 2.2 S57 manifest 的 delivery 元数据勘误

S57 交付内容按固定 HEAD 的 Git blob 核对。`docs/IMPLEMENTATION_PLAN.md` 的记录 raw SHA `0692c223…d218f` 和 blob `66c2c99…7351e` 与当前检出一致。S57 decision、report 的 manifest `checkout_raw_sha256` 分别为 `9f3815ec…38d2a0`、`2151e4a5…58366`；两值等于固定 HEAD blob 内容的 SHA-256，但不等于本机 CRLF 检出的原始字节 SHA（分别为 `91c55d3f…a63059`、`acd2ec9b…981ea3`）。其 manifest 所记 Git blob ID 也有误：decision 记 `ce04b803…03d6`，实际 HEAD blob 为 `1056f3be…21e4`；report 记 `374644f7…1cf1`，实际 HEAD blob 为 `80b8ce60…a243`。对两份当前 checkout 去除 CRLF 差异后，其 SHA-256 都与 S57 manifest 记录的 hash 及 HEAD blob 内容 SHA-256 相同。

S57 gates 的 raw hash 有 CRLF 检出差异，但其 manifest blob ID 与 HEAD blob 匹配。由此将 S57 情况分类为 **delivery manifest 的两项 blob ID 元数据错误和 Markdown raw-byte/CRLF 记法差异**；没有发现固定 S57 runtime/source/evidence blob 被替换。S58 manifest 逐项保存历史记录和本轮实际值；S57 历史文件保持原样。

### 2.3 证据类别与承继边界

- **论文定义：**该路线借鉴 BB90 五迭代研究思路；论文没有定义本项目的 FTv3、五迭代实现图、A 托管接口或这些失败状态合同。
- **本地代码事实：**以下消息次序、文件/数据库调用、异常投影和本地 share 暂存来自被固定 S57 源文件的静态核查。
- **项目派生接口：**`F_T/v3`、`F_TOPK`、`F_TOPK-ATOMIC` 与 `SuccessFence` 是项目的功能/证明模型，不自动等同代码故障语义。
- **待验证设想：**`F_TOPK-ISSUE-ESCROW/v1` 是本轮提出的 proof-only 候选，不是已实现、批准或与部署映射完成的功能。

`F_TOPK-U/F_TOPK-ATOMIC` 的历史描述依据 S32 report 摘要；S31 原始决策源未包含在固定 S57 证据集合中。因此本报告不声称重新找回或独立核验了 S31 原文，也不扩张 `L_S29`。S50 `Kpre₃` 的范围是理想 IID FTv3、`cap_success`、N≤256 的 pre-escrow 成功前缀；S51 `Kpost₃` 是理想 A token RNG、诚实 peer 因果提交及成功调度下的后缀；S53 `SuccessFence` 不覆盖真实 wall-clock/有限 timeout 筛选。S57 的无条件视图缺口作为本轮起点，不把早期成功引理重写为完整证明。

## 3. 冻结的角色、声明对象与模拟权限

功能继续为 signed Q20.12 分数降序、同分按原始索引升序稳定排序，并输出原始输入顺序、恰 K 个 1 的 Top-K XOR mask shares。正常托管约定仍是双方各自加密并提交本方 share，A 不解密；两份密文齐备后逻辑提交，双方各自 authenticated fetch。本轮不改变正常 API 合同、角色或获准泄露。

固定调用元组写为：

```text
τ = (session, invocation, N, K, route, circuit, gate_schedule,
     FT_profile, sampler_version, package_format, receipt_format)
```

完整的单方目标必须同时表示事件、消息和本地暂存状态，而不是只比较成功 transcript：

```text
Law_real(V_b^all, O_b, api_b, share_formed_b, share_returned_b
        | x_b, public, L_S29)
```

其中 `V_b^all` 包括本方 package/进程状态、本方输入和获准 transcript、已收到的 peer/A 响应、submit/receipt/fetch、失败/恢复状态；`share_formed_b` 与 `share_returned_b` 是不同变量。A 内部 DB 状态和 T 私有随机带/marker 不属于 party view；它们只能作为生成 party-view 的隐藏状态核。`L_A` 仍与 `L_S29` 分开。

`Sim_b` 只能使用本方输入、同次理想输出份额（只在本地计算到该阶段后进入本方内部状态）、公开参数、已批准 `L_S29`、本方 package 和真实接口会让本方看到的消息/API 状态。不能取得 peer package/share/credentials、A-only context、A DB/controller 数据、T 私钥、未授权随机带或 proof-only 联合 sampler 权限。诚实 peer 必须使用它自己的本次输入、输出、package 和凭据发第二份 deposit；本轮没有假设无条件 peer-success oracle。

## 4. 固定源码中的全调用状态链

|阶段 | 固定源码/状态 | T、A、P_b、控制器可见内容 | 证明/模拟含义 |
|---|---|---|---|
|0. 参数与配置预检 | `score_to_mask_v3_gmw.cpp:848–861`；检查 N/K、身份绑定、输出路径唯一性、escrow 参数和 receipt version | 本地参数/path 错误可在发材前失败；执行器可见自己的异常。控制器可能收集进程结果 | 不是密码学 cap abort。无条件模型需明确拒绝是否影响两方 launch；不把私有错误交给 party simulator。 |
|1. signer key 读取 | 同文件 `:862–870` 调用 `read_key`；发生在 `before_material()`/marker 前 | T 观察文件读/PEM/密钥类型结果；party 尚未得到材料。是否有人通知 party 由启动器/外层流程决定 | 在有效公开参数后，这是最早实际发行失败类。现有理想 FTv3 将签名能力理想化，没有 `E_T_key → delivery/start/party-result` 联合投影。 |
|2. one-shot burn marker | `:472–544,874`；基于 session/invocation 建 marker，`O_EXCL`，写记录及 fsync/close/dir-fsync | T/文件系统可见 marker 阶段和私有 artifact；party 不应获知 registry 内部状态；测试 controller 若有权限可读文件 | 对 key 可用的假设成立时，这是首个可能改变持久状态的失败点。open/stat/create/write/fsync/close/dir-fsync 可有不同部分 marker/burn 结果；理想 private burn 不等于这些 outcome law。 |
|3. 材料随机及协议 cap abort | `random_bytes :280–286`，`RAND_bytes` 失败即异常不重试；FTv3 bounded sampler `:916–927` | RAND 调用/字节与采样状态由 T 持有；party 无 package；controller 可观察退出码/测试文件 | 理想 `F_T/v3` 已定义 IID tape 下 cap 耗尽→发材 caller 收到 `ISSUANCE_ABORT`、保留 private burn、不发 packages/context/online lease。真实 RAND/provider 错误不是 cap abort；`F_TOPK` 对同 invocation 是否启动仍未绑定。 |
|4. 签名与包/上下文发布 | `:944–1007` 构建并签 P0、P1 package 和 A context；随后 `durable_create` 依次 P0、P1、context（`durable_create :361–385`） | T 见密钥/材料/签名与文件操作；A 在收到 context 后见 `L_A`；party 只可能访问本方包。控制器权限不能转给模拟器 | 签名/序列化错误和多文件部分发布没有统一事务。某文件创建/写入/fsync 失败时之前文件不会由该函数回滚/删除；不能写作“无包痕迹”。 |
|5. 交付/启动与 GMW/Select | 外层启动/交付路由不是 F_T 与 F_TOPK 的组合规范；party 代码在 `party.cpp:799–830` 形成 `local_output`，`831–833` 加密并准备 submit | P_b 作为静态腐化进程可在本地看到自己的 `local_output`；peer 真实因果运行。A 可见自己的 context/消息但不解密 | 要模拟中止前缀，必须知道 `O_T` 后 P0/P1 是否取到包、启动、收到何种通知。现有分离功能没有这条 same-`τ` kernel。 |
|6. 首份 authenticated deposit | `escrow.cpp:314–443` 以 `BEGIN IMMEDIATE` 写 transaction/share；第一份可单独成为 PENDING 并提交 | A 见 metadata/ciphertext/hash 和内部状态；该状态不向 party 报告，除非具体响应包含它。party 只见其通道可见的响应 | A 状态要隐藏；模拟器不能由 controller 查询 SQLite。两个 share row 不是同时在最初 transaction 中插入。 |
|7. 第二份 deposit、token、COMMITTED | `:411–437` 第二份到达后读两份 cipher hash，生成 token (`:245–260`)，更新 state/receipt 并 commit；异常调用 `ROLLBACK`，返回值不检查 (`:438–442`) | A 见密文及交易；token 存入 state；peer 以自身 package/output/credential 真实提交 | 理想 token 局部均匀不提供 RAND 错误、SQL commit/rollback/响应成功 law。提交结果可能未知，不能把 rollback 调用本身当成状态已回滚。 |
|8. 状态响应、ABORT、UNKNOWN | `query_state :281–311`；restart/timeout 可把 PENDING 改为 ABORTED `:537–564,590–607`；服务异常 catch 后记录 stderr 并关闭连接 `:615–623` | `PENDING/COMMITTED/ABORTED` 是 A DB 状态；`COMMIT_STATUS_UNKNOWN` 是 party API 的认识状态。A/controller 的观测不属于 party simulator | 缺少不同故障切点和送达/丢失响应的联合状态机。UNKNOWN 不等于数据库 UNKNOWN，也不等于 PENDING/ABORTED。 |
|9. fetch/恢复与 API share | fetch 只对 COMMITTED 取本方 ciphertext/token (`:445–473,475–485`)；party 校验 receipt、decrypt、比较 local output 后在 `:897–902` 返回；错误映射见 `:835–894` | party 看到本方 fetch 密文/token/返回码；另一方响应时刻可能不同。controller 可读测试数据库，但无模拟权限 | 只有验证成功才将本方 share 放入最终 API 输出；失败时无 API share 不等于之前没有 `local_output`。外层 catch 清空输出字段也不证明内存已安全清零。 |

### 4.1 API/数据库状态不能混为一谈

固定源将经过认证的明确 ABORT 映射为 `ESCROW_ABORTED`（`party.cpp:837–842`）；submit 的其他异常映射为 `COMMIT_STATUS_UNKNOWN`（`:843–856`）；无效 receipt 为 `COMMIT_RECEIPT_INVALID`（`:858–872`）；已经知道 COMMITTED 但 fetch/decrypt/receipt/share 比对失败时为 `COMMITTED_FETCH_REQUIRED`（`:875–894`）。只有 fetch share 与本地计算 share 一致才返回 `COMMITTED` 及本方 mask share（`:897–902`）。

A 内部状态变量 `state∈{PENDING,COMMITTED,ABORTED}` 与客户端观察 `api_b∈{COMMITTED,ESCROW_ABORTED,COMMIT_STATUS_UNKNOWN,COMMIT_RECEIPT_INVALID,COMMITTED_FETCH_REQUIRED,ABORT_V3,…}` 是不同随机变量。A 错误 handler 可能没有 structured error response；客户端看到未知结果不决定 DB 实际状态。测试控制器的进程日志、exit code、文件检查、SQLite 查询是 TEST_ONLY 控制面信息，不得作为 `Sim_b` 的输入。

## 5. 无条件模拟尝试及精确断点

### 5.1 现有功能实际给出的 law

S49 的理想 `F_T/v3` 对有效的新 invocation 在理想 IID tape 上产生语义材料，bounded sampler 耗尽时返回 `ISSUANCE_ABORT` 并私下 burn；成功时按 profile 发布有签名的 package 与 A-only context。S50 `Kpre₃` 的 transcript kernel 额外以 `cap_success` 为条件。S51 `Kpost₃` 继续以成功应用执行、诚实 peer 因果 deposit、理想 A token RNG 与 `SuccessFence` 为前提。`F_TOPK` 和 escrow 的既有成功合同没有规定与发行失败共用的 launch/delivery 门，也没有将 T/A/deployment error 联合路由进 party output view。

将一次无筛选执行分解为隐藏阶段向量：

```text
H = (O_T, marker_artifact, package/context publication,
     D_0, D_1, D_A, start_0, start_1,
     online transcript, share_formed_0/1,
     A_DB_state, token_state, response_delivery,
     fetch_result_0/1, API_result_0/1)
```

party simulator 输出的应是 `V_b^all` 的投影，不是 H 全量值。无条件比较为：

```text
Law_real(V_b^all, status_b, share_formed_b, share_returned_b
        | x_b, public, L_S29)
  ≈ Law_ideal(V_b^all, status_b, share_formed_b, share_returned_b
              | x_b, public, L_S29).
```

**首个未闭合投影：**在有效公开参数下，真实 T 可于 signer key load/parse 失败；现有理想接口没有该 `O_T` 结果，也没有规定该失败对 `D_b/start_b` 的投影。缺少的条件核可写为：

```text
K_issue(D_0,D_1,D_A,start_0,start_1,notify_0,notify_1,
        artifact_visible_b | O_T, τ, actual_launcher_state)
```

该 kernel 不是 `L_S29`，也不是让 party simulator 直接获得 `O_T` 私密细节。即便对 cap exhaustion 有 `Pr_I[cap_abort]≤δ_reject` 的理想 IID 界，当前 `F_T/v3` 也未规定同一调用下 `F_TOPK` 是否被禁止启动以及 parties 看见的结果。不能假设自然没有通知、不能编造统一错误响应，也不能把两方都没收到 package 当成已被代码保证。

如果安全范围把 signer key 可用设为理想前提，则下一个精确缺口是 marker 的分阶段 I/O 结果与 package publication cut；若模型再把 T 文件系统成功也外生化，才进入在线后缀。在线阶段随后仍需要 A 的 RAND/SQLite/response/fetch 联合核以及实际 finite-timeout scheduler 条件。S57/S58 没有给出这些真实联合 law 或成功概率下界。

### 5.2 为什么条件成功模拟不能给出无条件模拟

S50/S51/S53 可支持的范围性等式是在理想成功分支内：

```text
Law_ideal(V_b | cap_success, issued, both parties run, SuccessFence)
  = Kpre₃_b ∘ Kpost₃_b.
```

目标未筛选 law 还必须包括每个失败 outcome 及其 party-visible prefix：

```text
Law(V_b, O, API_b) = Σ_o Pr[O=o] · Law(V_b, API_b | O=o).
```

当前只有理想 cap-abort 的部分定义和成功条件 kernel；没有实际 key/marker/RAND/sign/file publication outcome 到启动器行为的核，也没有 A/database/network 故障的完整概率 law。于是不能实例化该 mixture。功能 E2E 样本不提供 `Pr[S_issue]`、`Pr[S_commit]`、`Pr[S_fetch]` 的可证明下界，也不能替代失败分支分布。

若改用条件成功定理，需要先定义成功事件 S 并证明完整联合游戏。对无条件总变差距离 `ε`，只有给出理想 `p=Pr_I[S]>ε` 才可使用常见界 `TV(R|S,I|S)≤2ε/(p−ε)`；计算型 DRBG/签名优势不能未经条件成功归约就代入统计 `ε`。S58 没有这样的真实部署 `p_min`，也没有将错误、commit outcome unknown、timeout 和恢复结果放入同一个部署游戏。因此本轮不作任何完整成功条件化估计。

### 5.3 SuccessFence 的边界

外生 `SuccessFence` 是理想调度前提，直接保证两方合法消息在协议规定期限内完成；在该模型下可沿用 S50/S51 对应用消息视图的成功结论。它不生成真实延迟分布，也不证明实际连接、TLS 握手、逐帧 deadline、A 等待第二份 deposit、响应丢失、restart 和恢复的成功样本不会依赖前缀/隐藏状态。实际 `Success_actual` 条件式仍未证明：

```text
Law(Vpre_b | Q_b, cap_success, Success_actual)
  ≈ Law(Vpre_b | Q_b, cap_success).
```

S58 不把 timeout、wall-clock、活动消息形状或新状态加入 `L_S29`。无故障业务 I/O 故障是否自然复现未在本轮测试；继承历史状态 `NOT_REPRODUCED`，不声称不存在。

## 6. 三类范围的细分裁决

### A. 正常协议执行

以理想发行成功、双方按协议执行、A 正常收到双方密文并 commit/fetch 为前提，可复用 `Kpre₃+Kpost₃` 的受限理想成功视图结论。但需要保留所有条件：N≤256、`cap_success`、理想 token、诚实对端真实因果 deposit、无 I/O/进程故障和外生 fence。它不是运行时真实错误率、所有 API 状态或超时筛选的证明。S52 的 N≤1000 功能用例只是既有 oracle 功能观察，非本轮测试，也非概率证据。

### B. 协议级中止

| 事件 | 现有理想/源码事实 | 完整视图状态 |
|---|---|---|
| FTv3 bounded sampler 达上限 | 理想 F_T/v3 定义 `ISSUANCE_ABORT` 与 private burn、无包/上下文/在线 lease；源码将 retry cap 单独报错 | cap outcome 局部有定义；与同次 F_TOPK launch、两方通知/响应的联合 law `OPEN`。低概率产品接受另为 `OPEN_CONFIRMATION`。 |
| 重复 invocation/已 burn | marker `O_EXCL` 的 `EEXIST` 会映射为 `ISSUANCE_ABORT`；理想功能要求一次性绑定/重复拒绝 | 对真实持久 registry 的状态范围依赖目录/回滚/部署；不等于跨目录或备份回滚全局防重。party 对该情况的通知仍需启动器映射。 |
| 输入/包/协议验证拒绝 | 代码各阶段可抛异常/返回特定状态；部分错误由 outer API 归为 `ABORT_V3` | 没有统一的跨 T、P0/P1、A 状态核。模拟器需知道实际可见错误投影与本地阶段，而不能只用“无 share”。 |
| party 主动 abort / peer channel failure | GMW engine 可 abort；外层 transport/escrow 可能断连；local output 可能早已形成 | 不构成自然故障安全证明。无论 API 是否返回 share，腐化进程 view 应保留此前的本地 share、中间 wire/message 及错误前缀。完整中止隐私 `OPEN`。 |

### C. 底层部署故障

Signer key 不可读/解析、registry 权限或写入失败、`RAND_bytes` 错误、签名/serialization 失败、P0/P1/context 顺序发布部分失败、A token RNG 错误、SQLite statement/commit/rollback 不确定、机器重启/持久化介质故障、TLS/连接/超时均不是理想 bounded cap abort。它们可被一个经负责人确认的成功-only 隐私声明排除在定理之外，并单列为 availability/reliability 范围；若要求隐私定理覆盖它们，必须有明确 fault cuts 或目标部署联合 kernel，并生成与真实 API 一致的失败 transcript。来源代码调用 `ROLLBACK` 不证明 rollback 成功；`fsync`/SQLite 选项不自动证明物理掉电耐久。

## 7. 最小版本化理想接口候选（未批准、未实现）

建议后续定义 proof-only 组合功能 `F_TOPK-ISSUE-ESCROW/v1`。它用于补齐理想接口和证明对象，不替代真实实现桥，也不直接把目标联合 law 包装成一个 party simulator oracle。

### 7.1 绑定与角色

接受唯一 `τ=(session,invocation,N,K,route,circuit,gate_schedule,FT_profile,sampler_version,package_format,receipt_format)`。T 持有签发 key capability、随机 tape、burn/文件 artifact；A 持有 metadata-only context、双端密文及 escrow 状态；P_b 只持本方输入/package/credential/计算状态及自己实际收到的 protocol/API 消息；控制器数据不进入任何密码学角色接口。A 的可见字段仍归 `L_A`，`L_S29` 不改。

### 7.2 发行结果与交付/启动

功能内部为 T 记录阶段化 outcome：

```text
ISSUED
CAP_ABORT
REPLAY_OR_ALREADY_BURNED
KEY_LOAD_ERROR
MARKER_IO_ERROR(stage, private_artifact_state)
RAND_ERROR(call_index)
SIGN_OR_SERIALIZE_ERROR(stage)
PUBLISH_ERROR(file_index, stage, artifact_state)
```

`stage/artifact_state` 是 T/环境侧状态，不能直接传给 party simulator。功能另有与实际 launcher 对齐的 delivery/launch projection `D=(package_delivered_0, package_delivered_1, context_delivered_A, start_0, start_1, party_notice_0, party_notice_1)`。该投影须由项目确认的启动合同或部署证据确定；功能不得虚构 parties 都会收到新的统一错误响应。`ISSUED` 才允许同一 `τ` 的 `F_TOPK` 在线执行；失败结果下是否双端一致失败、是否完全不启动，正是待负责人决定的问题。部分文件已发布时，不能用一个“无 package”布尔值压掉实际可读 artifact。

### 7.3 分享形成、escrow 和可见输出

`F_TOPK` 在在线执行实际到达输出阶段时产生理想本方 share；对被腐化 party，模拟器可将其作为其本地 `share_formed_b`/输入加密和 submit 的状态。对外 API `share_returned_b` 仍仅在 A 逻辑 COMMITTED、authenticated 本方 fetch 成功、receipt 一致、recipient decrypt 和本地 share 比对成功时变为 true。失败或未知时 API 不返回 share，但功能不得声称 corrupted process 未在本地形成/持有 share。

A 作为隐藏状态机分别记录 `NO_CONTEXT/NO_TX/PENDING/COMMITTED/ABORTED`、token `NOT_GENERATED/GENERATED_IN_TX/PERSISTED`、commit result `NOT_ATTEMPTED/COMMITTED/ABORTED/UNKNOWN_TO_CALLER`、响应是否抵达、每方 fetch 与恢复结果。`COMMIT_STATUS_UNKNOWN` 是 party 对服务状态的认识/API 结果，不是 A 数据库状态。A 的随机源、事务/持久化和恢复行为使用显式确定 fault cut，或由真实部署资料实例化的 `Γ_A`；不得把 A 内部状态、token 前值或 peer ciphertext 提供给 P_b。

### 7.4 对手、失败与定理范围

基础证明对手仍为至多一方静态半诚实；T/A 依冻结模型条件受信且不与腐化方合谋。先给确定性 fault-cut 语义，不声称概率；如要声明实际部署无条件分布，需证明 `Γ_T, Γ_A, Γ_storage, Γ_network, Γ_launcher` 与被审环境匹配，并纳入共享 RNG/并发/重启与 response selection。该功能不扩大 leakage，不把控制器权限给模拟器，也不纳入物理掉电/恶意 OS，除非另行定义。

这份规格只解决“理想功能要描述什么”和后续证明的采样接口。它不会自动证明 `F_T/v3` 与真实 Dealer/DRBG、签名器、文件系统、SQLite、A token RNG、TLS/mTLS、recipient encryption 或真实有限 timeout 相符。未经负责人确认和独立复核，保持 `CONDITIONAL / NOT_IMPLEMENTED`。

## 8. 独立复核、执行记录和状态

Reviewer `/root/s58_independent_review` 未参与 S50–S58 的实现或证明推导，独立只读检查了固定源码中的发材顺序、cap-abort 与部署故障的区分、party/A/controller 可见性、API share release、首个状态核缺口和 `F_TOPK-ISSUE-ESCROW/v1` 最小字段。Reviewer 未编辑、构建或测试；该复核不是独立功能接收。

本轮确实执行：固定 HEAD/tree/parent 查询；读取 AGENTS/PROJECT/实施计划、S57/S49/S50/S51/S53/S56 证据；通过哈希程序逐项检查 S57 manifest 中 7 个 primary sources、9 个 evidence inputs、15 个 S55 runtime rows；重算 S56/S57 历史 canonical delivery；用 `rg` 与行号抽取核查 Dealer、party、A 和测试控制器代码。S57 Manifest 的字段勘误数值列入 S58 manifest。未执行 build/test、历史矩阵、自然 I/O 实验或正式网络/性能测试。

| 门 | 状态 | 范围/限制 |
|---|---|---|
| `S57_FIXED_SOURCE_RECEIPT` | `PASS_WITH_SCOPE` | 固定 S57 HEAD 和源/证据身份通过；S57 两个 Markdown blob metadata erratum 已保留。 |
| `NORMAL_SUCCESS_SUCCESSFENCE_VIEW` | `PASS_WITH_SCOPE`（继承） | 仅理想 FTv3 cap-success、理想 A token RNG、N≤256、诚实 peer、外生 fence 的应用消息 view。 |
| `FTV3_CAP_ABORT_LOCAL_FUNCTION` | `PASS_WITH_SCOPE` | 理想 IID bounded sampler cap-abort 局部定义；未定义与 F_TOPK/party launch 的无条件组合。 |
| `UNCONDITIONAL_FULL_SINGLE_PARTY_VIEW` | `OPEN` | 缺同次 issue outcome → delivery/start/notice → online/A/API 联合核。 |
| `FIRST_RUNTIME_FAILURE_PROJECTION` | `OPEN` | key read/parse 可先于 marker 失败；理想功能无此 outcome 到 party projection。 |
| `MARKER_AND_PARTIAL_PUBLICATION_FAILURES` | `OPEN` | marker 分阶段持久状态、文件发布 artifact cut 没有相应 joint law。 |
| `A_STATUS_AND_PARTY_API_SOURCE_MAP` | `PASS_WITH_SCOPE` | 源码映射 PENDING/COMMITTED/ABORTED、UNKNOWN 及 share 形成/返回差异；不等于失败证明。 |
| `PROTOCOL_ABORT_VIEW` | `OPEN` | 只有 cap-abort 局部定义；验证拒绝/主动 abort/peer failure 与 party-visible prefix 未组合。 |
| `DEPLOYMENT_FAILURE_VIEW` | `OPEN` | 真实 T/A RNG、签名/文件、DB、TLS、重启/超时联合分布与环境未实例化。 |
| `ACTUAL_FINITE_TIMEOUT_VIEW` | `OPEN`（继承） | SuccessFence 不代表实际有限 timeout 成功筛选。 |
| `F_TOPK_ISSUE_ESCROW_V1_SPEC` | `CONDITIONAL / NOT_IMPLEMENTED` | proof-only 最小候选；启动/错误投影、fault kernel 和负责人合同待定。 |
| `L_S29_AND_NORMAL_DELIVERY_CONTRACT` | `UNCHANGED` | 不扩张 leakage，不改变已确认正常流程。 |
| `LOW_PROBABILITY_ISSUANCE_ABORT_ACCEPTANCE` | `OPEN_CONFIRMATION` | 无新增负责人决定。 |
| `BUILD_TEST` | `NOT_RUN` | 文档/证明审查，没有运行时代码变化。 |
| `NATURAL_BUSINESS_IO_FAILURE` | `NOT_RUN`（继承 `NOT_REPRODUCED`） | 本轮无自然 I/O 运行。 |
| `FORMAL_LAN_WAN_RTT_AAV86_PERFORMANCE` | `NOT_RUN / NOT_MEASURED` | 本轮未启动。 |
| `SECURE_ALIAS_READY` | `NOT_READY` | 未创建 alias。 |

本轮没有发现或复现运行时缺陷；没有修改 `VFSS/`、测试、`VFSS-baseline/`、密钥、材料、证书、构建物或原始日志。最终变更仅为本轮治理与证据文档。未推送、未合并、未创建 PR。

## 9. 下一步最短路径

1. 项目负责人回答上节三项合同问题，确定 cap-abort 重试、其他发行失败的 party-visible/启动语义、以及隐私定理范围。
2. 以确定性 fault cut 先正式写出 `F_TOPK-ISSUE-ESCROW/v1`，尤其规定同一 `τ` 的 package delivery/start/notice、部分 publication、local share formation 和 A 的隐藏提交状态，不新增 `L_S29`。
3. 让未参与该接口规格的密码学 reviewer 独立构造 P0/P1 的无条件 joint simulator；若只证明 success-only 分支，明确留下所有失败 branch 的范围边界。
4. 后续分别取得 T/A 部署 DRBG、签名和文件系统、SQLite/恢复、TLS/mTLS、recipient encryption、真实超时/调度模型证据。没有这些资料时对应部署桥继续 OPEN；不以 S58 理想接口关闭之。
