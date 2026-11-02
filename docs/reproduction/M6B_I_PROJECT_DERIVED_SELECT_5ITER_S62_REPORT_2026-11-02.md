# M6B-I PROJECT_DERIVED_SELECT_5ITER S62 详细报告

日期：2026-11-02
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

本路线实现项目自有五迭代稳定 Select，借鉴 BB90 研究思路；不是 BB90 原算法、BMW16 或 BB90+DCF 复现。功能合同仍为 signed Q20.12 分数降序、同分按原始索引升序、输出原序恰 K 个 1 的 Top-K XOR mask shares。算法迭代数不等于网络轮数或 RTT。

## 1. 结论摘要

| 分支 | 状态 | 精确范围/原因 |
|---|---|---|
| A. 正常成功执行及 S61 固定事件 cut | `PASS_WITH_SCOPE`（继承整理） | 理想 FTv3 cap-success、理想 output refresh/A token RNG、一方静态半诚实、honest peer、外生 SuccessFence 与固定外生 event cut，`1≤K≤N≤256`。S62 没有重证 S50/S51/S53/S61 基础证明。 |
| B. FTv3 cap-abort 到 package/context、双方启动/通知 | `OPEN`；候选核 `CONDITIONAL` | 当前 F_T/v3 只输出 issuer-local `ISSUANCE_ABORT`，未定义 party delivery/start/notice joint kernel。测试控制器的 all-or-none 启动不能代表生产路由。 |
| B. A 的 ABORT/UNKNOWN/fetch/recovery failure | `OPEN_WITH_EXACT_GAP` | 现有理想 Kpost 仅成功核。首缺的是首份 deposit 后“第二份是否在 wait deadline 前到达、A 状态及提交方实际收到的响应”的条件联合核。 |
| C. signer、文件/marker、RAND/provider、SQLite、TLS/机器及真实 timeout | `OPEN` | 部署/可用性模型未提供；不并入理想 cap-abort 事件或 S61 SuccessFence。 |
| 本轮 runtime/build/test/performance | `NOT_RUN` | 仅固定源码与证明/合同审查，无运行时改动。 |

因此可保留 S61 范围的正常成功应用消息视图结论；不能宣称协议级中止模拟、完整 T_AB、真实部署安全或完整协议安全通过。

## 2. 固定来源和证据层级

### 2.1 基线核验

| 项目 | 值 |
|---|---|
| S61 commit | `d843b45f29ab87d75520b28aac12c82608ec9678` |
| S61 tree | `d7dbc1f107ded4fe0527e55a40c24965b7b01a60` |
| S61 parent | `07325b076dc31f83439d55d7a59ad2239e0d5fcf` |
| S61 manifest 的 S60 canonical delivery | `9b6b35befdb2138ce15ee8750d154a5881e1f4aa3f6392dbbbe75e5d7efc1024` |
| 本轮 worktree | `C:/Users/28641/.codex/worktrees/m6b-i-project-select5-s62/MoE_Top-k_Reproduction_Evaluation` |
| 本轮分支 | `codex/m6b-i-project-select5-s62`，从固定 S61 commit 建立 |

新 worktree 从 S61 精确提交创建，S61 原 worktree 和未跟踪文件未修改。S61 manifest 的 canonical delivery 复算一致；八项固定源码映射的 Git blob、内容 SHA-256 与长度均匹配。S61 manifest 的 `inherited_evidence` 中 `PROJECT.md` 和 `docs/IMPLEMENTATION_PLAN.md` 两行是 S60 父版本 blob；它们不匹配 S61 最终版本。这是证据清单的快照语义差异，非运行时来源冲突。本报告与 S62 manifest 分列：S61 原记录值、S60 父版本值、S61 当前 HEAD 的实际值；不改写 S61 历史清单。

### 2.2 证据分类

- **论文定义：**没有把项目 Select5 或其失败合同反写为 BB90 论文保证。
- **源码事实：**以固定 S61/S60 Git blobs 中的 FTv3 issuer、party API、escrow state machine 和 TEST_ONLY E2E 控制器为准。
- **项目派生证明：**A 分支只组合既有理想模型下的引理；B 的 all-or-none/notice 是尚待决定的理想接口候选。
- **待验证假设：**真实部署随机源、签名/文件发布、数据库/信道/调度及失败联合分布不由本地源码或功能测试证明。

## 3. 负责人决定核查

从固定 S61 仓库的 PROJECT/实施计划、决策、gates 与 S50–S61 相关记录中，未找到更新的负责人书面决定。三项仍为 `OPEN_CONFIRMATION`：

1. cap-abort 烧掉 `τ` 后是否允许用新 invocation 重试；
2. 发材失败时是否要求两方同获一般失败通知且都不启动；若通知送达失败，是否允许不同方看到 timeout/UNKNOWN；
3. 安全声明是否覆盖协议级 abort，还是只覆盖初始化/发材成功后的正常隐私。

未重问正常托管/回执/取回和人工 kill 含义。已暂缓的算法随机失败数值界与 FTv3 bounded sampler 的 cap-abort 合同是两个不同决定。

### 3.1 安全声明范围 A/B/C

| 类别 | 事件 | 本轮定理归属 | 本轮裁决 |
|---|---|---|---|
| A. 正常成功执行 | 初始化/理想发材成功；双方因果执行；A 共同提交；双方各自取回自己的 share | 限定理想成功执行隐私与正确性范围 | `PASS_WITH_SCOPE` 仅继承 S50/S51/S53/S61 假设；不包括真实 timeout 或实现桥。 |
| B. 协议级中止 | ideal cap-abort、材料/协议验证拒绝、对端不 deposit、A 持久化 ABORT、提交响应未达导致客户端 UNKNOWN、后续 fetch/恢复 | 只有负责人选择 abort-aware 定理且理想功能定义可观察状态转移后才纳入 | 当前 `OPEN_CONFIRMATION` / `OPEN`；本文仅构造候选分支并停在首个缺失 escrow 条件核，不把失败视图当作已证明。 |
| C. 部署/可用性故障 | signer/key、RAND/provider、marker/文件发布、SQLite、TLS/网络、服务/机器/介质重启及真实 deadline | 当前作为可靠性/部署边界，不纳入 A/B 理想定理 | `OPEN`；若日后纳入，需目标部署联合故障模型、实际可见响应投影及独立证明，不能并入 `δ_cap`。 |

A/B/C 是不同结论层：本地源码给出状态转移事实，但不单独证明对应概率或模拟器。尤其 API 未返回 share 不推出进程此前未形成本地暂存 share；客户端 `COMMIT_STATUS_UNKNOWN` 也不等于 A DB 中某一确定状态。

## 4. 冻结模型、状态与权限

完整 invocation 继续使用 S61 的公开绑定元组 `τ`：

```text
τ = (session, invocation, N, K, route, circuit_hash, gate_schedule_hash,
     FT/sampler/package/context/receipt versions, party/A/T identities,
     recipient-key identities, escrow state parameters)
```

腐化方最多一方静态半诚实。模拟器不得取得 peer package/input/output/credential、A-only context 或 DB、T 私钥/Dealer tape、测试控制器状态或新增 `L_S29`。诚实 peer 只用自己的同次输入、ideal output share、package 和凭据因果执行。`z_b` 只在本方 `OUTPUT_SHARE_FORMED` 后进入本方 simulator 的本地状态；这不将它交给 peer，也不增加泄露。

以下状态不能合并：

```text
party local share formed -> ciphertext prepared -> A receives ciphertext ->
A database state -> response delivered/unknown -> party fetches own ciphertext ->
party API returns its share
```

`COMMIT_STATUS_UNKNOWN` 是客户端响应观察，不是 A 的 `PENDING`/`COMMITTED`/`ABORTED` 数据库状态；API 未返回 share 也不表示本地 share 从未形成。

## 5. A 分支：限定正常成功定理

### 5.1 定理陈述

对任一 `b∈{0,1}`、`1≤K≤N≤256`，以同一次成功发材 invocation 为条件，假设：

- ideal FTv3 材料发行成功并给每方仅自己的有效签名 package；
- `F_TOPK`/output refresh 提供目标本方 share；
- A token RNG 是 S39/S40 引理中的独立理想均匀源；
- A/T 受信且不与腐化 party 合谋，另一 party 按诚实 endpoint 因果执行；
- 外生 `SuccessFence` 保证合法应用消息与服务事件按约定完成，执行不按真实耗时或成功事件筛选；
- 对 prefix 子结论，使用 S61 已列举的固定 source-event 切点 `j∈E_cut`，由理想外生调度指定，不把 `j` 暴露为 `L_S29`。

则 S50/S51/S53/S61 的有限范围证明支持成功应用视图及每个已枚举事件 projection 的 simulator。简写为：

```text
Law(V_b^success | x_b,z_b,L_S29,pkg_b,static_b,public(τ),ISSUED,SuccessFence)
  = Sim_b^success(...)

Law(V_b^{≤j}, state_b^{≤j}, share_formed_b | Q_b, ISSUED,
    SuccessFence, σ, J=j)
  = Sim_b^{≤j}(...),  j∈E_cut
```

第二式中 share 形成前不条件于也不输入 `z_b`；仅 share 形成事件以后把该方自己的 staged `z_b` 交给本方模拟器。S61 的 cut-specific kernel 是因果采样，并未把含未来输出的完整成功 transcript 机械截断。

### 5.2 继承引理与 S62 工作

| 引理/层 | S62 状态 | 范围 |
|---|---|---|
| S36 H4、FTv3 Beaver/比较材料条件联合核 | `INHERITED_PASS_WITH_SCOPE` | S50 复核范围；S62 未重推。 |
| S50 `Kpre₃`，N≤256 pre-escrow | `INHERITED_PASS_WITH_SCOPE` | ideal IID FTv3 且 `cap_success`。 |
| S51 `Kpost₃` 与理想 A token 条件 law | `INHERITED_PASS_WITH_SCOPE` | honest peer 因果 deposit、双份密文成功、A token RNG 理想。 |
| S53 `SuccessFence` | `INHERITED_PASS_WITH_SCOPE` | 外生及时成功执行，不是 runtime timeout 保证。 |
| S61 `Kpre-prefix^{b,j}` | `INHERITED_PASS_WITH_SCOPE` | 已枚举 source-event cut 的增量 sampler，含 output-share 前后不同权限。 |
| A→S61 的范围整理 | `PASS_WITH_SCOPE` 待本轮独立 reviewer 确认 | 只确认继承条件无误；未重证基础引理或实现桥。 |

不包括 cap-abort/失败响应、真实有限 timeout 条件样本、A-only corruption、N>256 安全扩域，也不包括真实 T/A RNG、签名、recipient encryption、TLS/mTLS、SQLite 持久性或完整 GMW/Select/shuffle/output/escrow 组合。

## 6. B 分支：发材中止到材料交付、启动与通知

### 6.1 实际 FTv3 源码顺序

固定源码 `VFSS/src/moe_topk/score_to_mask_v3_gmw.cpp`：

| 事件 | 源码位置 | 状态含义与可见性 |
|---|---|---|
| 校验参数/身份、读取签名 key | `dealer_issue_packages_impl` 840–874，`read_key` 862–863 | signer read/parse 是部署失败，不是 cap 事件；先于 burn marker。只由 T 调用路径观察。 |
| 注册 FTv3 invocation burn marker | `register_ft_v3_invocation_once` 472–544 | `O_EXCL` EEXIST 表示同 invocation 已签发/烧掉，是 replay refusal；marker 写/fsync 失败可能留 marker，区别于采样耗尽。 |
| 生成材料及 bounded Fisher–Yates | v3 path 875–943；cap throw 923–925（v2 cap throw 在 912） | triples/randomness、permutation shares 后按 `i` 抽样；达到 FTv3 retry limit 时抛 `F_T/v3 ISSUANCE_ABORT`。RAND_bytes error 是另一异常路径，不是 cap-abort。 |
| 构造/签 package 与 context | 约 944–1004 | 只有随机与签名成功后才构造包和 context。 |
| 顺序发布文件 | 约 1005–1007 | `durable_create(P0)`、再 `durable_create(P1)`、再 `durable_create(context)`；发布失败不等于 cap-abort，前置文件可能已存在。 |

理想 F_T/v3 对新鲜、有效 `τ` 的 IID bounded sampler cap exhaustion 定义 issuer-local `ISSUANCE_ABORT`、私有 burn、无 party package/context/online lease。它没有规定这个结果如何给 P0/P1 投影、是否创建 A transaction、是否调用 party API、谁发送 notice 及送达顺序。

理想 cap-abort 的拒绝概率是单独的语义材料概率。沿用 S60 保守界 `δ_cap≤255·2^-154<2^-146`（理想 IID tape，N≤256，固定 FTv3 profile）；它不是 marker EEXIST、RAND/provider error、signer/filesystem error、A commit error 或 timeout 的概率，也不直接界定包括 notice/启动/签名 transcript 的总视图距离。S62 不从 E2E 次数推导 `p_min`。

### 6.2 当前源码是否提供双方启动/通知核

固定运行时公开入口是独立的 Dealer package issuer 和 `score_to_mask_v3_execute_party_v2`。Dealer 成功后返回，不暴露一个生产 API 将三份发行产物认证交付并对 P0/P1 作共同 start gating；两方分别收到 package 路径后可独立进入 party 函数。party 在 `score_to_mask_v3_party.cpp:547–595` 打开/校验本方 package、检查输入及 channel，然后分别消费 lease 并交换 READY。固定源码未找到生产 `τ` 级的双端 launcher/notice barrier。

TEST_ONLY E2E 控制器在 `VFSS/tests/moe_topk/score_to_mask_v3_process_e2e_test.cpp:1106–1110` 等待 Dealer 子进程成功并验证两个 package，随后才创建 escrow 服务并在约 `1147–1182` 启动双方。这证明的是测试控制器的次序；不能把 controller 的状态/文件查询权限交给 party simulator，也不能称为生产路径保证。FTv3 forced-abort case 的控制器只核验无 package/context/lease 和 burn record，不证明两方通知或生产双方都未启动。

因此不能由“issuer 没发布 package”推出“两方均未启动或收到同一个错误”。若 party 被外部直接调用且包缺失，`open_material` 在 party 入口失败，外层 API 返回本地 `ABORT_V3`；这也不是带原因的 `ISSUANCE_ABORT` 双方通知。

### 6.3 proof-only 候选分支

未批准的 `F_TOPK-ISSUE-ESCROW/v1` 可对**纯理想** cap 分支采用以下候选转移：

```text
bind τ once; call ideal F_T/v3 once
if ISSUED:
    expose pkg_b only to P_b and ctx_A only to A
    require both package deliveries/preflight before either party starts
    continue with S61 Kpre-prefix event kernel
if CAP_ABORT:
    burn τ; expose no package/context; create no A transaction; start_0=start_1=0
    choose one owner-approved party projection:
      NONE_NO_CALL, or PAIRED_GENERIC_ABORT, or an explicit per-party notice kernel
```

若两方均未被调用，party transcript 可为空；若采用 `PAIRED_GENERIC_ABORT`，仅在负责人确认该发材中止观察已包含于获准的 `L_S29` abort 字段时，才可让两方看到统一 abort 类别而不新增泄露。否则该输出/泄露映射仍未获准，不能假定现有 `L_S29` 已包含它。此策略不暴露 cap 原因/采样状态，理想 notice 仍需定义原子/送达规则。若采用 per-party notice，则必须给出 `(notice_0,notice_1,start_0,start_1)` 联合核；不能假定两方自动同时收到。以上各策略下，给定策略后，理想 cap 事件到 policy 输出可直接按该确定/明示核采样，不需 peer secret、A context、T 私钥或 future share。

这只表明理想接口**可以**补足 cap event 的模型转移；不表示当前源码有 barrier，也不表示负责人选定了策略。B 的当前状态因此仍 `OPEN`。把它与 `S61 Kpre-prefix` 组合只在 `ISSUED` 分支可用；cap-abort 分支不进入 Select、没有 `z_b`，不能向 S61 success cut 核套用未来 share。

### 6.4 错误必须分开建模

| 事件 | 与理想 cap-abort 的关系 | 当前 source 观察/缺口 |
|---|---|---|
| bounded sampler 达上限 | 协议级 ideal `CAP_ABORT` | issuer exception、invocation 已 burn；party notice/start 未定义。 |
| marker EEXIST | replay refusal | 错误前缀可能含 `ISSUANCE_ABORT` 文本，但原因是 registry 已有记录；不计入 `δ_cap`。 |
| marker open/write/fsync 失败 | 部署发行故障 | 可能保留部分 marker；无联合 party 通知/启动核。 |
| RAND_bytes error | 部署随机源故障 | 无重试，抛异常；不是 sampler 所定义的全部拒绝事件。 |
| signer/key parse 或 package签名错误 | 部署/密码服务故障 | signer read 在 marker 前；签名失败在 package 发布前。 |
| package/context durable create 失败 | 部署文件故障 | P0→P1→context 顺序，可能部分可见；不属于 cap bound。 |

## 7. B 分支：A ABORT、UNKNOWN 与恢复失败

### 7.1 源码状态链

固定源码 `VFSS/src/moe_topk/score_to_mask_v3_party.cpp` 与 `score_to_mask_v3_escrow.cpp` 给出下列本地行为；它们是源码事实，不自动构成 ideal failure law。

1. party 在 output refresh 后构造本方 `local_output`，再加密并调用 submit（party 约 799–836）。`z_b`/本地 share 此时已在进程中形成，即使函数最终不返回 share。
2. A 收到首份有效密文后 SQL 行进入 `PENDING`；第二份密文有效后在同一提交路径内生成 token、写 COMMITTED 与 token 并提交（escrow 411–437；相关请求处理 314–442）。
3. 只有提交方收到并解析有效 COMMITTED receipt，party 才知道 commit 成功；若 callback/连接抛异常，函数返回 `kRecoveryRequired / COMMIT_STATUS_UNKNOWN`，没有 API share、也没有 receipt（party 843–856）。这时 A 可能是 PENDING、COMMITTED 或其它状态，party 不知道。
4. 若收到 authenticated ABORT response，submitter 将其标成 definite abort，party 返回 `kAborted / ESCROW_ABORTED`，无 API share；本地 share 已先形成（party 837–842；escrow response parse 648–675）。
5. A 在第一份 deposit 后等待第二份，至 signed context deadline 仍 PENDING 时将状态改为 ABORTED（escrow 592–606）。服务重启还会把 PENDING 行改为 ABORTED（537–542）。这些是 A 内部状态；不能从 `COMMIT_STATUS_UNKNOWN` 推断其值。
6. A fetch 仅允许 COMMITTED，返回请求方自己的 ciphertext 与同一个 receipt；未提交/已 ABORT 会在服务端异常路径关闭连接，不是统一的状态查询响应（445–473、566–624）。
7. 若 party 已知 commit 但 fetch/decrypt/receipt-match 失败，API 返回 `COMMITTED_FETCH_REQUIRED`，保留 transaction/receipt 而不返回 share（party 875–894）。最终只有 fetch 密文解密匹配本地计算值后才把 share 作为 `kCommitted` 结果返回（party 897–902）。

### 7.2 第一个未闭合条件核

若 cap-abort issue/start policy 已由理想 barrier 固定，且同 invocation 已发材、双方已启动，则本轮向 escrow failure 扩展的第一个失败切点是：

```text
J_A = first valid deposit stored; second valid deposit has not yet arrived
     at the A wait-expiry event
```

所需的联合核不是 `A state` 的边缘分布，而是：

```text
K_A^b = Law( second_deposit_before_expiry,
             A_state_after_event,
             response_b_delivered_and_parsed,
             client_observation_b,
             share_formed_b,
             V_b^post-submit
           | x_b, public(τ), pkg_b, static_b, L_S29,
             V_b^pre, ISSUED, J_A )
```

现有 Kpost/S51 只定义在诚实对端第二份密文因果到达、A COMMITTED、理想 token 生成且 SuccessFence 成功的核；S61 的 cut 集合是成功应用事件，不含 `A_WAIT_EXPIRED`/ABORT/响应丢失。当前 `F_TOPK`/A ideal API 未提供 `K_A^b` 的 event schedule、提交响应投影或失败投影。没有经负责人批准的外生失败 event selector，也没有证明对手/调度不会按 `V_b^pre` 选择第二份 deposit/响应是否交付。因此此处是 `OPEN_WITH_EXACT_GAP`，不是攻击或数学上不存在 sampler。

要模拟 `UNKNOWN`，不能把它等同于 A 的 ABORTED：它要求模型同时保留 A 隐藏状态并仅给 party 其实际收到的响应/无响应。重复 submit、服务重启及 fetch 失败也各需按因果 transition 明示状态和客户端可见响应。对于已知 ABORT 的后续 fetch，源码只给 recipient fetch 成功于 COMMITTED 的路径；对 ABORTED/PENDING 请求，服务异常关闭连接，party 看到的是 callback/transport failure，不是 DB 状态。已知 COMMITTED 但 fetch response 丢失则与“原 submit 未知”不同，party 保存 receipt 并返回 `COMMITTED_FETCH_REQUIRED`。

### 7.3 最小版本化修正候选（未批准/未实现）

proof-only `F_TOPK-ISSUE-ESCROW/v1` 至少需要为每个 `τ` 增加以下**功能状态和权限边界**，之后仍要独立证明每个核：

1. `ISSUE=(ISSUED,CAP_ABORT,REPLAY_REFUSED,DEPLOYMENT_ERROR)` 分型；只有 `CAP_ABORT` 使用理想 cap 概率，replay 与部署错误不归入该概率。
2. `delivery=(pkg_0,pkg_1,ctx_A)` 的角色隔离和 party start barrier；通知策略显式选为 none、paired generic-abort，或联合 per-party delivery kernel。
3. party-local `share_formed_b`、ciphertext prepared/submitted、fetch received、API returned 独立位；output share 只在 share formation 后交本方 simulator。
4. A 的隐藏状态 `NO_TX/PENDING/COMMITTED/ABORTED`、token 只在成功提交时抽一次并复用；party 输出仅来自实际收到的 status/receipt/fetch response，A DB 不进 party simulator。
5. 明示 `A_WAIT_EXPIRED`、服务重启 abort、响应送达/丢失、重复请求和 recovery fetch 等事件转移；外生环境/调度者不得按隐藏输入或 `V_b` 选择，除非合同另定义并提供其条件 sampler。真实 finite-timeout law 不因理想 event selector 而通过。
6. honest peer 在成功路径只用本方资料因果 deposit；不允许 peer-success oracle、controller 查询权限或对端秘密进入被腐化方 simulator。

这份候选不新增 `L_S29`，不改变输出/角色，也不桥接 SQLite、真实 timeout、TLS、token RNG、签名或持久性。未获负责人确认和非作者审查前不得当作项目合同或 `PASS_WITH_SCOPE`。

## 8. 定理范围—引理—首缺口—责任方

| 定理范围 | 已证/继承引理 | 继承假设 | 首个缺口 | 下一证据责任方 |
|---|---|---|---|---|
| A：成功执行、固定 event cut | S50 Kpre₃；S51 Kpost₃；S53 SuccessFence；S61 Kpre-prefix | Ideal FTv3 cap-success、理想输出刷新/A token、honest peer、`N≤256`、固定 L、外生成功 schedule/cut | 真实实现桥、真实 timeout 与完整协议 composition 不在此定理 | 密码学 reviewer + 之后的 T/A/transport 实现证据负责人 |
| B：ideal FT cap-abort | S49 的 local issuer abort/burn | iid ideal sampler；错误类别不合并 | same-τ package/context 到 party start/notice joint projection 未定；生产无 barrier API | 项目负责人定合同；runtime/launcher owner 出实现映射 |
| B：A ABORT/UNKNOWN/fetch recovery failure | S51 只含 success suffix | 若扩展需理想 A 状态机、explicit event cut/response policy、honest endpoint 因果路由 | 首核 `K_A^b`：第二份 deposit 时序/到达、A 状态、response delivery 与 party observation 条件 law | 项目负责人定 abort theorem；密码学 reviewer给 sampler；A service owner给状态 refinement |
| C：部署及真实有限超时 | 无 | 目标环境 provider/存储/网络/调度模型 | 配置和联合故障/延迟 kernel缺失 | T/A SRE、PKI/构建发布、服务运维 |

## 9. 负责人待确认问题

只保留尚未决定、会改变合同的事项：

1. 合法输入下 FTv3 bounded sampler 达上限并烧掉本次 invocation 后，是否接受仅用新的 invocation 重试？
2. 发材失败时采用哪种通知/启动规则？是否要求双方在启动前得到同一一般失败结果；通知不可达时是否接受一方 timeout/UNKNOWN？
3. 隐私定理覆盖完整协议级 abort，还是仅覆盖成功初始化及发材后的正常协议？

候选最小表述：

> 项目是否接受：合法输入下，FTv3 拒绝采样若达到固定上限，本次 invocation 被 burn 并中止，后续只能以新 invocation 重试？

## 10. Reviewer、验证和状态

本轮执行者参与过 S50–S61 相关文档和证明工作，不把自审称为独立复核。非作者 `/root/s62_readonly_review` 未参与 S50–S62 推导，已只读复核 A 分支组合范围、S61 继承条件、cap-abort 到 launch/notice 候选核、错误类型分离、A 的首个 `PENDING→ABORTED` 条件核及 P0/P1 权限；审查者认为没有重大问题，并要求明确 `PAIRED_GENERIC_ABORT` 只有在负责人确认相应观察已属获准 `L_S29` abort 字段时才可使用。该审查不是功能接收或部署安全签核；reviewer 未编辑、未构建、未测试。

- 来源核验：S61 commit/tree/parent、原 manifest、固定 runtime blobs 和 delivery canonical digest已检查；仅 S61 inherited evidence 中两份状态文档使用父提交 blob，已显式记录。
- 本轮 BUILD/TEST=`NOT_RUN`；不重跑 S35/S40/S52 历史矩阵，不做 fault injection。
- 自然业务 I/O 本轮 `NOT_RUN`，继承 `NOT_REPRODUCED`；人工 kill 证据仍只属工程注入。
- LAN/WAN、RTT、AAV86、性能 `NOT_RUN/NOT_MEASURED`；N≤1000 功能收据只作为 S48/S52 继承证据，N>256 安全 `NOT_IN_SCOPE`。
- 不修改 `VFSS/` 或 `VFSS-baseline/`，不创建 `secure` alias，不推送、合并或创建 PR。

## 11. 最终声明

在 S61 的受限理想模型下，项目已有经复核的成功应用视图与单个外生 source-event prefix 结论。它不覆盖 cap-abort 到两方启动/通知的实际路由，也不覆盖 A ABORT/UNKNOWN/recovery failures 的完整联合 law。候选 ideal wrapper 可把这些事件写进功能，但写规格不能替代负责人批准、可执行 simulator、源码 refine 或部署证据。真实 T/A DRBG、签名与文件持久性、recipient encryption、TLS/mTLS、SQLite、真实有限 timeout、N>256 安全扩域和正式性能仍分别过门。
