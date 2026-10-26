# M6B-I PROJECT_DERIVED_SELECT_5ITER S59 详细复核报告

日期：2026-10-26
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`
被审对象：S58 精确提交 `4ae16569317f4526699ccb0e55a156eafcd03a1e`

## 1. 结论摘要

本轮把要证明的对象固定为**未筛选的完整单方视图**，同时将正常执行、协议级中止、部署故障分开。源码和历史理想功能只支持一条有限的成功路径结论：理想发材成功、理想 A token 随机源、诚实对端按自己的材料与输出因果提交，且外生 `SuccessFence` 保证应用消息按期完成时，N≤256 的既有 `Kpre₃/Kpost₃` 可在原有条件下讨论。它不提供失败路径分布。

**本轮裁决：当前 `F_T/v3 + F_TOPK + escrow` 不足以构造覆盖所有协议中止及状态结果的无条件单方完整视图模拟器。**首个协议级接口断点是：FTv3 的 `ISSUANCE_ABORT` 只定义给离线发行调用者的结果及私有 burn；没有同一 invocation 下包是否交付、P0/P1 是否启动、各方收到什么失败通知的 joint kernel。不能从“没发包”推出两方未被启动、未观察到其他状态，也不能把 TEST_ONLY 控制器的退出码、文件检查或 SQLite 查询当作 party simulator 的输入。

真实实现中更早发生的部署错误是 signer key 读取/解析失败 (`score_to_mask_v3_gmw.cpp:862–870`)；若将签名器成功作为外生前提，首个持久发行分叉是 marker 创建、写入和持久化 (`:472–543`)。此外，marker 耐久失败的错误字符串也包含 `ISSUANCE_ABORT` (`:542`)，与 bounded sampler cap 耗尽 (`:925`) 不能仅按错误文字归为同一协议事件。必须按故障原因分类。

针对下一轮，报告提出 proof-only、未批准且未实现的 `F_TOPK-ISSUE-ESCROW/v1` 状态规格。它把发材 outcome、启动门、协议中止、本地 share 形成、A 隐藏状态、响应抵达和恢复结果放进一个联合功能；它不向 party simulator 泄露 peer package/share/credential、A context/数据库状态、T 私钥、随机带或测试控制器数据，不扩展 `L_S29`。这个候选仍需负责人确定失败合同，并需独立证明其状态转移和真实接口映射。

本轮没有改运行时代码或测试，没有构建、运行 E2E、重跑历史矩阵或测量性能。自然业务读写故障仍按既有证据标为 `NOT_REPRODUCED`；人工终止仍是注入故障证据，不是自然崩溃。完整安全论证、目标部署桥、真实有限 timeout 视图和正式性能准入继续分别保持开放。

## 2. 固定对象、来源核验与证据层级

### 2.1 S58 对象身份

在隔离工作树 `C:/Users/28641/.codex/worktrees/m6b-i-project-select5-s59/MoE_Top-k_Reproduction_Evaluation` 开始工作；初始状态干净，建立本地短分支 `codex/m6b-i-project-select5-s59`，固定起点为 S58，而非以桌面主工作区或近似快照替代。

| 对象 | 复核值 | 状态 |
|---|---|---|
| S58 commit | `4ae16569317f4526699ccb0e55a156eafcd03a1e` | 匹配 |
| S58 tree | `2f34000e549964c3bfbd1dce9487fd28073d5767` | 匹配 |
| S58 parent | `79f61f6298e0da65b7c64ea4e748bee1afd1ebb1` | 匹配 |
| S58 初始工作树 | clean | 匹配 |
| S58 delivery canonical SHA-256 | `fca3d40df2339379ed967017a6c96698fe4be6f05ecf11bb0e1a61e9ef8b3826` | 从五个 S58 交付文件重算匹配 |

按 S58 manifest 重算并比较 Git blob / checkout 内容：7/7 主源码、9/9 证据输入、15/15 固定 S55 runtime 源文件通过。S57 manifest 本身和其固定 source rows 也在 S58 中记录并复核。S58 manifest 的 `reviewed_governance_files` 是 S58 修改前的治理输入快照；S58 交付清单中的 PROJECT/IMPLEMENTATION_PLAN 行是 S58 修改后的交付 blob，二者分属前后两种清单用途，不能错误比较成同一文件状态。S57 中曾记录的两项 blob ID 勘误由 S58 明确保留，本轮不改历史记录。

S58 五个交付文件 canonical 计算规则为：按路径 ordinal 排序，以 `<path><TAB><LF-normalized Git blob SHA-256><LF>` 形成文本后 SHA-256；manifest 自身不纳入以避免自引用循环。重算值匹配 S58 记录。S59 source/evidence manifest 进一步记录本轮固定源码和交付哈希。

### 2.2 证据分类

- **论文定义：**研究 BB90 的思路不等于实现 BB90 原算法；本轮不依据本地行为反推论文保证。
- **本地源码事实：**FTv3 发材函数顺序、marker 和文件副作用、A 的 SQLite/receipt 状态、party API 映射及测试控制器访问来自 S58 固定 Git blobs 的静态读取。
- **项目派生合同：**`F_T/v3`、`F_TOPK`、`F_TOPK-ATOMIC`、receipt/v2 和 `SuccessFence` 是项目功能/证明模型，不能自动等同实现异常、数据库或部署故障语义。
- **待验证设想：**`F_TOPK-ISSUE-ESCROW/v1` 是本轮 proof-only 候选；未获负责人批准、未实现，也没有被本轮当作真实系统的理想功能。

输入/输出继续保持 signed Q20.12 score 降序、原始索引升序稳定同分，并输出原序下恰有 K 个 1 的 Top-K XOR mask shares。协议角色、在线托管成功合同及 `L_S29` 不变。

## 3. 冻结的证明对象与模拟权限

同次 invocation 绑定元组记为：

```text
τ = (session, invocation, N, K, route, circuit_hash, gate_schedule_hash,
     FT_version, sampler_version, package_format_version,
     receipt_format_version, party/A identities, recipient-key hashes)
```

对 `b∈{0,1}`，完整单方视图不仅是最终 API 输出，而是：

```text
V_b = (x_b, pkg_b, static_b, all sent/received frames,
       local wire/material state visible to P_b,
       share_formed_b, envelope_b, submit observations,
       receipt/status observations, fetch/recovery observations,
       api_state_b, share_returned_b)
```

`share_formed_b`、`share_submitted_b`、`share_fetched_b`、`share_returned_b` 是四个不同事件。A 的数据库/token/context/package-set state 和 T 的密钥、材料随机带、marker 是隐藏变量；测试控制器的日志、退出码和 SQL 查询不属于腐化 party view。

目标是针对完整未筛选分布证明：

```text
Law_real(V_b, status_b, share_formed_b, share_returned_b
        | x_b, public(τ), L_S29)
  ≈ Law_ideal(Sim_b(x_b, public(τ), L_S29; F_joint), same_outputs)
```

不在式子外条件化 `cap_success`、`S_issue`、`S_commit`、`S_fetch` 或真实 `Success_actual`。模拟器不得取得对端 package/share/credential、A-only context/DB state、T 私钥、未授权随机带、`J_b` proof-only 权限或测试控制器能力。诚实对端必须用自己的同次输入、理想输出、package 和 credential，按因果顺序参与计算与第二次 deposit。

若实际或理想 view 有结果分支 `O`，完整分布应分解为：

```text
Law(V_b | x_b,P,L_S29)
  = Σ_o Pr[O=o | x_b,P,L_S29]
      · Law(V_b | O=o,x_b,P,L_S29).
```

S50/S51 提供的是若干成功条件核，不提供所有 `o` 的概率与失败条件核。不能用一个 `Law(V_b | success)` 代替右侧总和，也不能用 E2E 成功次数推断 `Pr[S_issue]`、`Pr[S_commit]` 或任何 `p_min`。

## 4. 安全声明范围裁决

| 范围 | 本轮分类 | 可覆盖内容与限制 |
|---|---|---|
| A. 正常成功执行 | `PASS_WITH_SCOPE`（只继承 S50/S51 条件视图） | 理想 FTv3 IID tape 且 cap-success、理想 A token RNG、至多一方静态半诚实、T/A 条件受信且不合谋、诚实 peer 因果提交、`1≤K≤N≤256`，外生 SuccessFence 保证规定应用消息在期限内处理。只对应成功应用消息视图；不覆盖发材中止、真实部署故障、超时筛选、故障前缀或 N>256 安全扩域。 |
| B. 协议级中止 | `OPEN`；具体局部事实 `PASS_WITH_SCOPE` | 理想 sampler cap-abort 有局部定义；但同调用交付/启动/通知未定义。协议定义的包验证拒绝、诚实 endpoint 的协议拒绝，以及由明确定义的抽象 fault cut 引发的 peer 不提交/A 状态结果，可作为候选协议中止；其联合视图未完成。实际 TLS/网络断连、超时或机器故障不因 party 最终返回 ABORT/UNKNOWN 就成为 B。半诚实腐化模型不包含腐化 party 恶意中止；若要证明恶意 abort，须另改对手模型。 |
| C. 底层部署/可用性故障 | `OPEN`（可由经确认的成功-only 定理排除） | signer key、RAND/provider、marker/文件、SQLite/rollback、服务重启/介质、TLS/实际网络连接/真实 timeout 等缺部署 kernel。A 的实际重启/定时器将 PENDING 变 ABORTED 也属于 C；只有另行定义的理想 fault cut 可作为 B 的抽象状态分支。它们是部署可靠性故障或成功-only 定理边界；若纳入隐私定理，需将故障点、可见错误和联合分布明确定义。 |

### 4.1 负责人尚未决定的合同

在没有明确负责人记录时，下面三项均是 `OPEN_CONFIRMATION`；本轮继续做源码映射，不自行选择合同：

1. FTv3 有界 Fisher–Yates sampler 达到上限、以理想模型下极低概率触发 `ISSUANCE_ABORT` 并烧掉本次 invocation 后，是否接受使用**新的 invocation**重试？这不是五迭代算法数值随机失败上限已有豁免的同一事项。
2. signer/marker/文件发布等发行故障时，是否要求双方拿到相同且不能误认为成功的失败结果、任何一方均不得启动？若一方未收到通知，是否允许它只观察到等待、超时或 `COMMIT_STATUS_UNKNOWN`？
3. 隐私目标是否限定为初始化与发材成功后的正常协议隐私、将底层故障作为可靠性边界；还是要求理想功能和定理覆盖协议级 abort 状态？

不再询问已确认的正常份额托管/取回流程，也不重复人工 kill 仅是注入故障的既有含义。

## 5. 固定源码状态链与可见性

### 5.1 Dealer / T 发材

| 顺序/状态 | 固定源码事实 | T/A/party 可见性与模拟影响 |
|---|---|---|
| 输入检查、签名器准备 | `score_to_mask_v3_gmw.cpp:839–870` 检查参数/路径；`:862` 读取 signer 私钥；FTv3 要求 receipt format v2。 | 私钥路径、解析结果、异常首先由 T/issuer caller 观察；party 只看到自己后续是否被交付/启动。理想 FT 的理想签名能力没有该部署失败。 |
| invocation burn marker | `:472–543` 依据 session/invocation 建 marker；O_EXCL、写记录、fsync、close、parent fsync 分步执行；错误路径不保证清除已创建 marker。调用点 `:874` 在随机材料分配前。EEXIST 重放拒绝在 `:510–513`。 | marker/burn 是 T 私有状态。EEXIST replay refusal 与 marker durability failure (`:542`) 都使用 `ISSUANCE_ABORT` 状态前缀，但各自成因不同，均不等同 sampler cap exhaustion (`:925`)。 |
| 有界随机发材/重放 | `:280–286` `RAND_bytes` 分块（≤1 MiB）并在失败时 throw；FTv3 bounded sampler `:916–927`，重试耗尽返回 cap `ISSUANCE_ABORT`；marker EEXIST 重放拒绝也在 `:510–513` 使用该前缀。 | 理想 `F_T/v3` 仅规定 IID tape 的 sampler cap abort、private burn、无 party package/context/online lease。EEXIST 是已 burn invocation 的重放拒绝，不是 cap 失败；真实 RAND 错误属于 provider/部署失败。测试里控制的随机带不提供生产分布。 |
| 签名、编码和发布 | 材料签名后，`:1005–1007` 顺序创建 P0 package、P1 package、A context；`durable_create :361–385` 不会将三个文件作为一个原子事务一起回滚。 | T 能看到全部生成过程和私有 artifact；每方只该得到本方 package，A 得 metadata-only context。半发布的真实错误 law 未定义。 |

**最早分叉需要分两种模型陈述：**实际部署源下最早可见内部失败为 signer key read/parse；在“signer 一定可用”的协议抽象下，最早实际持久状态操作为 marker I/O。当前 ideal protocol 缺口先出现于 cap-abort 到 party delivery/start 的映射。另有 EEXIST 重放拒绝与 cap abort 共用错误前缀。必须按原因分类：cap exhaustion 是理想随机协议中止，EEXIST 是 one-shot/replay 状态，marker durability failure 与 RAND/sign/file I/O failure 是部署故障；同一个错误字符串不让这些事件同分布。

### 5.2 在线 party 与 share 阶段

`score_to_mask_v3_party.cpp:799–830` 形成/检查本地 `local_output`。P0 生成 refresh；P1 由协议响应组合本地 share。`831–836` 加密并调用 submit。因此在第一次 A 响应之前，腐化 party 的本地进程已经形成并加密了自己的 share。

外层 API 的实际映射：

| 事件 | 源码位置 | Party 观察/API 状态 | 对模拟器的要求 |
|---|---|---|---|
| 已形成 local share 并提交 | `party.cpp:799–836` | 本地状态包含 share、envelope、transaction；A 仅持加密 envelope。 | 若后续中止，腐化 view 必须保留此前本地状态，不能将它抹成“从未形成”。 |
| 明确认证 escrow ABORT | `:837–842` | `ESCROW_ABORTED`，无 API share。 | 需有确定的 ABORT frame 到达条件；不证明本地 share 不存在。 |
| submit callback 其他异常 | `:843–856` | `COMMIT_STATUS_UNKNOWN` 并保留 transaction id；这表示客户端不能确定，不是 A 的 DB state。 | Simulator 不可从 UNKNOWN 推断 A 是 PENDING/COMMITTED/ABORTED；分布须来自隐藏状态与 response-delivery 联合核。 |
| receipt 格式错误 | `:858–872` | `COMMIT_RECEIPT_INVALID`，无 share。 | 真实消息/本地临时状态仍进 view；不能仅输出最终 API 码。 |
| 确认提交后 fetch/decrypt/比对失败 | `:875–894` | `COMMITTED_FETCH_REQUIRED`，保留 transaction 与 receipt 供外部恢复；不返回预提交内存 share。 | 本地 share 已形成但 API 未返回。当前恢复调度/再次 fetch 的整体功能不由该函数独自定义。 |
| 成功 fetch 与检查 | `:897–902` | `COMMITTED` 并返回解密后的本方 share。 | 只返回本方 share；不保证 P0/P1 物理同时返回。 |
| 其他异常 | `:903–919` | `ABORT_V3`，清空输出字段；不构成内存零化证明。 | view 仍包含失败前已生成的临时状态、消息和异常观察。 |

### 5.3 A escrow 状态与 response

`score_to_mask_v3_escrow.cpp:314–443` 在每个 submit 期间使用 SQLite `BEGIN IMMEDIATE`。首次有效 deposit 可写 transaction/share row 并提交成 PENDING。第二个合法 deposit 到达后，服务在 `:411–437` 读取两份 ciphertext hash、生成 receipt/token、更新 `state=COMMITTED` 并执行 SQL COMMIT。receipt/v2 token 在 `:245–260` 由 A 的 32-byte `RAND_bytes` 生成；重复提交/恢复时应由已持久 receipt复用。fetch 在 `:445–473` 仅返回请求方自己的 ciphertext 和 token，格式见 `:475–485`。

| 隐藏 A 状态 | 可能的 party 观察 | 必须区分的事实 |
|---|---|---|
| `NO_TX` | 未收到响应、等待或错误 | party 不可查询 controller SQL。 |
| `PENDING`，第一份密文已持久 | 第一方 submit 仍在等待；第二方可能尚未提交 | PENDING 是 DB state，不会自动以同名字段给 party。 |
| token 未生成 | 第二份 deposit 尚未有效/处理到 finalize | 不等于没有第一份 PENDING 密文。 |
| token 已在当前事务生成、COMMIT 未确认 | caller 可能看到连接错误/UNKNOWN；DB 最终可能 PENDING/ABORTED/COMMITTED，依真实结果而定 | `:438–440` 尝试 ROLLBACK 但不检查返回值，不能由调用回滚推断状态。 |
| `COMMITTED`，response 已送达 | party 收到 COMMITTED response/receipt frame 后仍需 authenticated fetch、解密与比较；API `COMMITTED` 尚未返回 | DB commit、response 到达、fetch 成功及 API share 返回是不同事件。 |
| `COMMITTED`，response 丢失 | `COMMIT_STATUS_UNKNOWN`；可能用 package+transaction 再次 authenticated fetch | UNKNOWN 是本方观察，不是 A DB 的第四个状态。 |
| `ABORTED` | 经验证的 ABORT 到达才为 `ESCROW_ABORTED`；连接异常仍可为 UNKNOWN | startup 将 PENDING 更新为 ABORTED (`:537–542`)；timeout handler尝试终态更新 (`:544–564,590–606`)。更新执行与通知到达是分离事件。 |
| fetch/恢复失败 | `COMMITTED_FETCH_REQUIRED` 或 UNKNOWN | API 尚未返回 share；本地计算 share 可能早已存在。 |

服务异常处理在 `:615–623` 记录 stderr 并关闭，不保证 structured error response；测试 harness 能读日志、exit code、测试文件和 SQLite（测试源码如 `score_to_mask_v3_process_e2e_test.cpp:786–812,1910–2017`），这些 controller 权限绝不可赋给 `Sim_b`。

## 6. 无条件模拟尝试与首次精确断点

### 6.1 当前已有的可模拟成功分支

条件于：`cap_success`、有效 package 与固定公开 `τ`、理想 IID FTv3 语义材料、S50 的 N≤256 `Kpre₃`、理想 A token RNG、诚实 peer 使用自己的输入/输出/package/credential 第二次 authenticated deposit，以及 `SuccessFence` 外生保证成功处理，可以沿用：

```text
Law(V_b^success | x_b,z_b,L_S29,pkg_b,static_b,τ,
    cap_success, peer follows, A ideal, SuccessFence)
   ≈ Kpre₃^b ; Kpost₃^b.
```

该式按原范围只支持成功应用消息视图。`z_b` 在真实执行中仅在对应协议计算阶段形成；它不作为“发行前已存在”的 simulator 输入。S50/S51 的局部结论不覆盖 cap-abort、protocol abort prefix、A commit uncertain、response loss、startup ABORTED 或实际有限 timeout。

### 6.2 首个未定义联合核：cap-abort 与双方启动

理想 `F_T/v3` 对同一 τ 的 bounded sampling cap 耗尽，定义 T/issuer 一侧 `ISSUANCE_ABORT`、私有 burn 且不发布 package/context/online lease。当前 `F_T` 和 `F_TOPK` 分开：没有 `F_TOPK` 应否调用、何时调用、P0/P1 是否启动或各自是否收到了失败通知的统一规则。故对任一 `b` 缺少：

```text
K_abort^b := Law_real(D_b, V_b^issuer/party, start_b,
                      notice_b, api_b, share_formed_b
                      | cap_abort, τ, x_b, L_S29),
```

其中 `D_b` 是本方 package 交付/启动/通知投影。现有 ideal side 没有同变量或可采样核。`cap_abort` 发生概率在 S49 的理想 IID bounded sampler 范围内有界，但**它的概率界没有定义启动/通知响应**；也不能从无 package 推出“双方均未启动”，因为固定对象未包含一个可核验的生产 launcher 全/无启动门。模拟器若读取 T 的异常、文件目录或 TEST_ONLY controller 结果补足该变量，就越权。

因此第一个不成立的是“现有合同可推出完整模拟”这一证明步骤；不是已知存在偏差的反例，也不是不可模拟的证明。要求的缺失对象是同 invocation 下的 `F_T outcome → package/context delivery → party launch/notice → F_TOPK invocation` 条件联合规则。

### 6.3 后续 escrow 核为何也不能用边缘量补齐

即使补上 issuance routing，后续仍需对每个允许 cut 的整条核证明：

```text
Law_real(V_b^escrow, share_formed_b, share_returned_b
        | τ, V_b^pre, honest_peer_endpoint, A hidden state, cut)
  ≈ Law_ideal(Sim_b(...), same public/corrupt view).
```

仅固定正常成功 `Kpost₃` continuation kernel，不推出混合 PENDING、ABORTED、COMMITTED-but-response-lost 与 fetch recovery 的未筛选 view 相等；需要 `Pr[outcome]` 与响应抵达/状态的联合核。token 局部均匀性也不生成 SQL/网络失败选择分布。当前没有 `p_min`，历史成功 E2E 次数不能充当真实部署完整成功概率下界。

### 6.4 状态分支逐项模拟状态

| 分支 | 当前模型/源行为 | 当前 sampler 裁决 |
|---|---|---|
| 理想 cap success + SuccessFence | S50/S51 原条件模型 | `PASS_WITH_SCOPE`（继承，不是 S59 新建完整证明） |
| 理想 cap abort，未定义双方 launch/notice | F_T 返回发行侧 abort、不发材料 | `OPEN`，首个断点 `K_abort^b` 缺失 |
| T key/签名/RAND/文件失败 | 真实 Dealer 异常/副作用可能在发行前或半发布；marker I/O failure 与 sampler exhaustion 有相似错误文字 | `OPEN` 部署 kernel；不纳入 protocol-only 定理，除非负责人选择并给出模型 |
| party package/在线协议验证拒绝 | 真实 API 通常归为 `ABORT_V3`；前缀可能已发送/接收消息 | `OPEN`，需要按各拒绝切点模拟完整已发生前缀；不能从最终无 share 简化 |
| 一方完成在线计算并已有 local share，但 peer 未 deposit | 本方 share/envelope 已形成，A 可 PENDING；抽象协议 cut 可给协议中止结果；当前服务 restart/deadline 导致的 ABORTED 属于部署 C | `OPEN`，需 honest peer 因果步骤及显式区分的协议 cut / deployment scheduler 模型 |
| A definite ABORT 送达 | `ESCROW_ABORTED`，无 API share | Source map `PASS_WITH_SCOPE`；未证明整个先前 view/到达事件联合分布 |
| A commit 已执行但 response 不明 | 可能真实 COMMITTED，party 观察 `COMMIT_STATUS_UNKNOWN` | `OPEN`，未知为客户端认知，不代表 DB state；缺提交/发送 joint kernel |
| response COMMITTED，fetch 成功 | 逻辑共同提交后本方单独取回 | 仅在理想 A/token 与 SuccessFence 下沿用 Kpost₃ |
| response/receipt 后 fetch 失败、之后恢复 | `COMMITTED_FETCH_REQUIRED` 或其他 UNKNOWN；本地 share 先前已形成 | `OPEN`，无 S59 当前功能性测试或完整理想状态核 |
| signer/DB/机器/网络/真实 deadline 故障 | deployment/availability | `OPEN`；需要环境分布，不能当成理想协议 cap-abort |

## 7. Proof-only 候选 `F_TOPK-ISSUE-ESCROW/v1`

此规格用于解决当前 ideal side 未规定的联合变量，不是已批准合同，不是 runtime 改造，也不扩大 `L_S29`。后续若采纳，至少具备以下接口与状态。

### 7.1 调用绑定与发行

将同一调用唯一绑定到 `τ`；对同一 `(session,invocation)` 只发行一次，明确 burn/replay 结果。签名、sampler、package 和 receipt 版本分别保持字段空间独立。功能将 T 侧 outcome 分离为：

```text
ISSUED
CAP_ABORT                 # 理想 IID bounded sampler cap exhausted
PROTOCOL_REJECT(stage)    # 项目明确定义的协议输入/包验证拒绝
DEPLOYMENT_ERROR(kind, stage) # 仅部署故障扩展模式允许，不能与 CAP_ABORT 混同
```

在 **protocol-only theorem profile** 下只把 `CAP_ABORT` 与定义好的协议拒绝纳入；T 签名能力可用、marker/发布操作按 ideal 功能原子成功，不讨论真实 provider/file error。若选择 `deployment-inclusive`，则必须给 `Γ_T` 明确 issuer 可见 outcome、部分 marker/package artifact 及概率分布；没有部署证据时不实例化。

### 7.2 Package/context 交付和启动门

功能维护 `delivery_b`、`delivery_A`、`start_b`、`notice_b` 的显式转移。提案的最小安全可验证版本采用**同次 invocation 全/无启动门**：只有 `ISSUED`、P0/P1 各自的 τ 绑定 package 可用、A 的 context 已正确交付，才允许双方在线 `F_TOPK` 启动；`CAP_ABORT` 时不交付任何 party package/context、不创建 online lease，两方 `start_b=0`，`ISSUANCE_ABORT` 仅返回给离线 issuer/orchestrator，双方 party API 不产生结果，因为未启动。

这是一项提案的合同选择，不是 S58 runtime 已验证的部署行为；若项目希望 parties 已启动后获得共同显式失败，必须另定义对应消息/通知及不可达时的状态，不能仅在 ideal function 写“双方都知道失败”。真实 launcher 需要提供可核验的全/无启动保证或与实际观察一致的 `delivery/start/notice` 联合 kernel。

### 7.3 在线 share 与 A 状态机

只有 `start_0=start_1=1` 时才运行理想 `F_TOPK`。每方仅在真实协议到达输出阶段时获得自己的 `z_b`，并记录以下内部时间戳/布尔状态：

```text
share_formed_b → envelope_created_b → deposit_sent_b → deposit_accepted_b
                → share_fetched_b → share_returned_b
```

状态不是把中间值添加到 `L_S29`；这些只用于腐化方完整进程 view。功能不向 party simulator提供对端明文份额。

A 的数据库状态、服务事务动作和 party 可见响应应分开建模。A 数据库及 token 的隐藏状态至少为：

```text
db_state ∈ {NO_TX, PENDING, COMMITTED, ABORTED}
token_state ∈ {NOT_GENERATED, GENERATED_IN_TX, PERSISTED}
```

A 服务内部提交动作/结果另记为：

```text
commit_attempt ∈ {NOT_ATTEMPTED, COMMIT_OK, COMMIT_ERROR, ROLLBACK_ATTEMPTED}
```

响应送达、fetch 与客户端观察属于交互视图，不是 A 数据库状态：

```text
delivery_b ∈ {NOT_SENT, SENT, DELIVERED, LOST, MALFORMED}
fetch_observation_b ∈ {NOT_REQUESTED, REJECTED, OWN_CIPHERTEXT_RECEIVED, DELIVERY_UNKNOWN}
api_observation_b ∈ {NOT_STARTED, COMMIT_STATUS_UNKNOWN, ESCROW_ABORTED,
                     COMMITTED_FETCH_REQUIRED, COMMITTED, ABORT_V3, ...}
```

`commit_attempt` 是 A service 的动作/结果，不是 party 可见值；`delivery_b`、`fetch_observation_b` 与 `api_observation_b` 是响应传递和客户端观察，不是 A 的数据库状态。`COMMIT_STATUS_UNKNOWN` 仅是本方 API/客户端认识状态，不是 A DB state，也不表示 A 自身“不知道数据库状态”。第一份合法 deposit 可导致 PENDING；第二份合法 deposit 加 receipt/token 与 COMMITTED；重复 submit/fetch 对已经持久 COMMITTED 交易复用同一 token/recipient ciphertext。A 不解密 share。token 在正常 commit 中于共同事务内生成并随 state 保存；token RNG/SQL error 的失败分支使用确定 fault cut 或经证据实例化的 `Γ_A`，不得当作 token 随机性的推论。

### 7.4 状态 cut、消息可见性与权限

候选功能以版本化 `E_cut` 或明确 kernel 描述协议中止/消息响应交付的具体位置：例如在线开始前、某个已完成 gate frame 后、share 形成后但 deposit 前、第一 deposit 后、commit 前/后、response 送达/丢失、fetch 送达/丢失。切点表示动作，不直接把 A/T 隐藏状态交给 party simulator。协议错误/显式理想 cut 可列入候选 B；真实 TLS/网络断连、DNS/connect、wall-clock deadline、服务重启和机器故障仍列为 C，只有部署-inclusive profile 提供目标 kernel 后才可能纳入。模拟器可与诚实 peer/A 按协议互动，并只收到真实协议会给本方的响应。`E_cut` 在 theorem 中的权限必须固定；若 cut 可以依据秘密计时/网络延迟选择，真实有限 timeout 条件模拟仍 OPEN。

该功能必须提供显式可执行转移算法，而不是把目标 `Law_real(V_b)` 包装成一个 oracle。每个 cut 的 party-visible output 必须是消息 DAG 的确定投影；A 的 `L_A` 与 party `L_S29` 分开；不新增 rank、索引、完整 mask、活动比较位或时间字段。

### 7.5 适用范围与排除项

- 半诚实腐化：一方静态半诚实；T/A 受既定信任条件且不与被腐化方合谋。恶意 party 主动选择 abort 不在该腐化模型内；若要覆盖必须单独改变对手能力。
- 输出合同：原序稳定 Top-K XOR mask，成功时 A 收齐密文共同提交，各方仅可恢复本方 share；任何 API share 返回时点不要求物理同时。
- 可尝试范围：理想发材/理想 A token RNG 和 `N≤256` 的成功前缀继承；protocol abort 分支须有 S59 之后对每个 Kpre prefix、escrow cut 的独立模拟证明。
- 不自动纳入：真实 DRBG/provider、签名器、文件系统/marker 原子性、SQLite rollback/durability、TLS/mTLS、RSA-OAEP/AES-GCM 组合、机器重启/介质损坏、DNS/connect、真实有限 timeout/wall-clock 选择。要纳入须再给 `Γ_T, Γ_A, Γ_storage, Γ_network` 及各项优势/成功条件证据。

### 7.6 候选规格能解决什么、不能解决什么

若负责人批准并由独立审查确认实现投影，`F_TOPK-ISSUE-ESCROW/v1` 可解决“当前理想世界没有同 invocation 发行结果→delivery/start→online `F_TOPK`→A 状态→party API 输出联合 law”的**接口定义缺口**。它不会证明这些理想转换匹配真实部署；不会自动关闭 issuer/A RNG、文件发布、签名、SQLite、recipient encryption、TLS、物理耐久、有限 timeout 或完整 GMW/Select/shuffle/output composition。当前仍未获得成功定理外的 protocol-abort全视图 PASS。

## 8. SuccessFence 与真实有限时间

### 8.1 可继承的理想成功范围

S50 `Kpre₃` 仅是理想 IID FTv3 发材 cap-success 下、`1≤K≤N≤256`、单方静态半诚实的 pre-escrow transcript kernel。S51 `Kpost₃` 延伸到理想 A token RNG、诚实 peer 因果 deposit、A 成功逻辑提交和本方成功 fetch。S53 的 SuccessFence 是**外生理想调度假设**：对定理要求的每条合法应用消息保证按时处理。该模型下可沿用成功应用消息结论；不包括 cap-abort、失败前缀和真实部署时长/超时筛选。

### 8.2 未关闭的实际模型

真实有限 deadline 下，成功事件 `S_actual` 由 DNS/connect、TLS handshake、frame read/write deadline、peer 消息活动、A 等第二份 deposit deadline、SQLite/receipt 工作、response delivery、恢复 fetch 共同决定。需要：

```text
Law(V_pre_b | x_b,P,L_S29,cap_success,S_actual)
```

与目标 simulator 的可采样条件核，或对任意合法前缀证明成功权重不改变其分布。S58/S59 没有目标调度器/网络/服务延迟分布，也没有可用来假设 `S_actual` 与秘密相关前缀独立的依据。不能新增 wall-clock/timeout/活动形状到 `L_S29` 来制造通过。因此 `ACTUAL_FINITE_TIMEOUT_VIEW=OPEN`；SuccessFence 不是部署保证。

## 9. 门禁与待补证据

| 门/主张 | 状态 | 责任来源 / 最短补项 |
|---|---|---|
| S58 exact source receipt | `PASS_WITH_SCOPE` | 本轮重算 S58 manifest 与 Git blobs。 |
| S50/S51 ideal SuccessFence successful-message view | `PASS_WITH_SCOPE`（继承） | 限 N≤256 与原理想假设；本轮独立 reviewer 仅复核映射/边界，不扩展成安全证明。 |
| Full unfiltered single-party view | `OPEN` | 密码学作者提供每状态的联合模拟；接口先由负责人决定 scope。 |
| FTv3 cap-abort to party delivery/start/notice | `OPEN` | 项目 protocol/API owner 决定启动和通知合同；launcher owner 证明源行为。 |
| Protocol validation/peer-abort prefix view | `OPEN` | 逐 cut 的消息 prefix simulator；是否纳入恶意主动 abort 需明确 adversary model。 |
| Escrow PENDING/COMMITTED/ABORTED/UNKNOWN/recovery view | `OPEN` | A service owner 定义 fault cut、response delivery 和恢复契约；独立证明联合核。 |
| `F_TOPK-ISSUE-ESCROW/v1` specification | `CONDITIONAL / NOT_APPROVED / NOT_IMPLEMENTED` | 负责人确认 3 项合同；protocol owner 与非作者 reviewer 再审 executable transition rules。 |
| T signer/RAND/marker/package publish | `OPEN` | T/SRE、签名设施、文件系统/发布 owner 提供脱敏部署 profile 与副作用/失败语义。 |
| A RNG/SQLite/rollback/restart durability | `OPEN` | A service/DB owner 提供 provider 配置、事务故障模型、备份/恢复边界；不声称掉电耐久。 |
| TLS/mTLS / recipient encryption | `OPEN` | 部署 PKI/network/security owner 与密码学分析分别提供 profile 和 advantage proof。 |
| `ACTUAL_FINITE_TIMEOUT_VIEW` | `OPEN` | SRE/network owner 提供调度权限、deadline、重试/取消/响应交付模型；证明 success-conditioned kernel。 |
| `L_S29` changes | `NOT_CHANGED` | 本轮未增加任何字段。 |
| Low-probability `ISSUANCE_ABORT` acceptance | `OPEN_CONFIRMATION` | 负责人回答负责人问题 1。 |
| N≤1000 functionality | `PASS_WITH_SCOPE`（仅继承 S52 个案） | 不是 S59 新测；不能推为安全/全输入正确性。 |
| N>256 security | `OPEN / NOT_IN_SCOPE` | 不外推 Kpre₃/Kpost₃。 |
| Build / tests / historical matrices | `NOT_RUN` | 无 runtime/test 变更。 |
| Natural I/O failures | `NOT_REPRODUCED`（继承） | 本轮未实验；不能说故障不存在。 |
| LAN/WAN/RTT/AAV86/formal performance | `NOT_RUN / NOT_MEASURED` | 正式性能仍后置。 |
| Secure alias / push / merge / PR | `NOT_CREATED / NOT_PERFORMED` | 保持实验默认关闭与治理边界。 |

### 部署方证据清单（不收集秘密）

- **T service/SRE：**实际 OpenSSL/libcrypto 精确版本、provider/`OSSL_LIB_CTX`/配置、熵源/DRBG/reseed 生命周期、线程/fork/restart/并发、共享 DRBG 使用者与聚合请求数、错误处理和进程重试/新 invocation 行为；只提交脱敏配置，不提交 seed、私钥或熵材料。
- **发行/签名 owner：**key load 失败投影、marker 每个 I/O cut 的 artifact/burn 可见性、三文件发布及同 invocation 重放规则、sign/serialize/publish 失败如何路由给两端。
- **A service/DB owner：**RAND token 失败、transaction begin/update/commit/rollback/unknown、PENDING timeout/restart、COMMITTED 恢复和响应送达的状态机；SQLite/文件系统/备份/恢复配置及其证据，不以 WAL/FULL 配置直接声称掉电耐久。
- **Protocol/API owner：**cap abort 时 parties 是否启动、失败通知/无通知、有效 package 的全/无交付门、validation reject 和 protocol abort 的 party-visible错误投影、恢复调用的 API contract。
- **Network/PKI/SRE：**TLS/mTLS profile、connect/DNS/frame/TLS deadlines、重试/取消/服务端等待期限、调度权限与 success-selection 模型；不得由本机配置替代目标部署证据。
- **独立密码学 reviewer：**复核候选 `F_TOPK-ISSUE-ESCROW/v1` 转移、逐 cut 条件 law、模拟器权限与优势分解；不能把合同审核当作功能接收。

## 10. 独立复核、执行和未运行项

执行者沿 S58 固定对象开展静态证据审查。本轮 reviewer `/root/s59_independent_review` 是新委派且未参与 S50–S59 推导的非作者；其独立复算 S58 manifest 的 7/7 主源码、9/9 证据输入、15/15 固定 S55 runtime 源码及五文件 canonical 摘要，并只读核对 S59 状态链/候选接口，不编辑、不构建、不运行测试。Reviewer 确认应按原因区分 marker durability failure、EEXIST replay refusal 与理想 sampler `ISSUANCE_ABORT`，实际 TLS/network/deadline/restart 应留在部署环境类，且 candidate 仍需 issue→delivery/start 联合核。该审查是边界/合同复核，不是完整密码学证明或独立功能接收。

实际执行包括：读取 `AGENTS.md`、`PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md`、S58 decision/report/gates/source manifest，以及 S50/S51/S53/S57 相关报告；核对 Git HEAD/tree/parent/status；按 S58 manifest 重算内容哈希和 Git blobs；静态检索并抽取 FTv3 issuer、party output/submit/fetch、A receipt/transaction/restart 源码行。没有执行任何构建、测试或系统级运行。

| 项目 | 状态 |
|---|---|
| Runtime / tests 修改 | `NONE` |
| BUILD / TEST | `NOT_RUN` |
| S35/S40/S52 历史矩阵 | `NOT_RUN` |
| 自然 I/O 故障实验 | `NOT_RUN`；既有 `NOT_REPRODUCED` 继续沿用 |
| 人工 kill / crash 注入 | `NOT_RUN` |
| LAN/WAN / RTT / AAV86 / 正式性能 | `NOT_RUN / NOT_MEASURED` |
| `VFSS-baseline/` | 未修改 |
| secrets/materials/certs/build products/raw logs | 未新增到仓库 |
| `secure` alias / push / merge / PR | 未创建 / 未执行 |

## 11. 最终可用措辞与最短下一步

当前可写：**“该项目在理想 FTv3/理想 A token RNG 和外生 SuccessFence 条件下，沿用 N≤256 成功应用消息视图的有范围 Kpre₃/Kpost₃ 结论；S59 静态确认 escrow 与 party API 的多个状态映射。”**

当前不可写：**“中止/失败分支已被完整模拟”“真实有限 timeout 成功视图已安全”“完整部署安全证明已完成”“S59 功能/性能通过”“项目复现了 BB90/BMW16 原协议”。**

下一步最短路线：先让负责人答复 §4.1 三个合同问题；Protocol/API owner 冻结 `F_TOPK-ISSUE-ESCROW/v1` 的启动/通知/abort/recovery 联合状态机；用固定 cut 列表写出每个失败前缀的权限正确 sampler，并由新的非作者密码学审查者复核。之后再分别实例化 T/A 的真实 RNG、签名/发布、SQLite、TLS 与 finite-timeout 部署核。性能工作继续等待安全声明和准入门，不在本轮启动。
