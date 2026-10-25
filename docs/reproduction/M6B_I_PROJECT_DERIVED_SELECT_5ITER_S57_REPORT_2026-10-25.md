# M6B-I PROJECT_DERIVED_SELECT_5ITER S57 详细报告

日期：2026-10-25
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 1. 结论摘要

本轮目标是把发材中止和 escrow 提交/恢复失败一并放进单方完整视图，而不是仅模拟成功 transcript 后再条件化。当前 `F_T/v3`、`F_TOPK` 和 escrow 合同不足以给出这个无条件联合模拟器：`Kpre₃` 的 law 条件于理想发行成功，`Kpost₃` 的 law 条件于成功应用执行与外生 SuccessFence；而真实代码还可能在读签名 key、记录 burn marker、生成材料、签名/序列化和逐个发布文件时失败。现有理想发行功能没有这些失败的 outcome law，也没有把一次调用的发行结果明确路由到同一调用的 `F_TOPK` 启动/输出。

对严格“第一个断点”作如下精确限定：在固定有效输入、路径和配置的前提下，`dealer_issue_packages_impl` 仍在 marker 之前执行 `read_key`（`score_to_mask_v3_gmw.cpp:862–870`）；key 文件读/解析/密钥加载错误没有 `F_T/v3` 输出，因此它是最早未建模的运行时失败类。若将签名 key 的可用性作为固定理想假设，下一处断点是 FTv3 marker 的 open/stat/create/write/fsync/close/目录 fsync（`score_to_mask_v3_gmw.cpp:492–543`）。这是最早可能改变持久 burn/artifact 状态的失败点。两处均属合同/模型缺口，不能写成攻击或已证明不可能。

托管后缀的源码状态机可以精确描述，但源码可见性映射并不等于无条件安全模拟：party 不知道 A 的 PENDING/COMMITTED/ABORTED 内部态；其 `COMMIT_STATUS_UNKNOWN` 只是提交结果不确定时的 API 投影。客户端只有在取得有效 commit 状态、authenticated fetch、receipt 匹配、recipient decrypt 与本地 share 比较成功后才返回本方 mask share。失败路径不返回 share，不证明进程内从未形成临时 share。

因此，本轮结论是 **`OPEN_WITH_EXACT_GAP`**。没有改运行时和理想功能实现；没有构建或测试。独立只读 reviewer 复核了首个缺口、权限区分和最小接口草案。

## 2. 范围、证据类别和固定来源

### 2.1 路线与功能合同

目标是项目自行设计的五轮稳定 Select：signed Q20.12 分数降序，同分按原始索引升序，输出原序且恰有 K 个 1 的 Top-K XOR mask shares。该路线借鉴 BB90 的研究思路，但不是 BB90 原算法、BMW16 或 BB90+DCF 复现。正常交付合同仍是双方各自在计算进程中生成/加密本方 share，A 收齐双方密文后共同提交，之后各方只取回自己的 share；两方 API 返回时点可不同。`L_S29`、输入、输出、角色、A-only `L_A`、正常交付语义均未更改。

### 2.2 固定对象

- S56 commit：`66a3db485ffa9ac752a49fd0f8a0f100fcef4cbe`
- Tree：`8cd2c7e77c9577091bbba9405a44640e6bc446bd`
- Parent：`0c4a35fb7bdf5d234e98eb1616cb65aae03ab3fd`
- Branch：`codex/m6b-i-project-select5-s57`
- Isolated worktree：`C:/Users/28641/.codex/worktrees/m6b-i-project-select5-s57/MoE_Top-k_Reproduction_Evaluation`
- Initial status：clean。

已阅读 `PROJECT.md`、本计划、S50–S56 决策/报告/gates/cases/manifests。S56 manifest 中 S55 的 15 项源文件 raw hash 与 Git blob 均匹配；S56 source-review 锚点与固定 Git blobs 匹配。S56 四项历史交付文件的 Git blob 与 canonical digest 复算一致。其三项文本文件在本 Windows checkout 的 raw hash 因 CRLF 与历史 LF 不同；`docs/IMPLEMENTATION_PLAN.md` 的历史 S56 blob 在 HEAD 中匹配，但本轮已追加 S57 摘要，所以当前工作文件 raw/blob 按 S57 交付重新计量。Git baseline blob 是固定来源判断依据，不改写 S56 历史。S31 原始决策文件不在本固定树中；`F_TOPK-ATOMIC` 的继承条款按 S32 报告记载引用，不冒称恢复了原始 S31 文件。

### 2.3 证据分类

- **论文定义：**本轮不声称目标路线等价于 BB90 原算法或论文安全结论。
- **本地源码事实：**以下文件/行号是本轮静态核对对象，未重跑运行时。
- **项目派生合同：**`F_T/v3`、`F_TOPK`、A atomic escrow、SuccessFence 是项目建立的接口与模型。
- **待验证假设：**真实随机源、文件/数据库、签名、网络/调度、超时和它们的失败联合分布。

## 3. 不筛选的目标视图

对腐化方 `P_b`，目标视图 `V_b^all` 包含它在本次 invocation 中可见的有序消息与本地状态：是否获得本方 package/READY、peer frame/openings/公开比较结果、调用阶段、错误/断连、A 对其可见的 submit/commit/receipt/fetch 响应、API state/abort code、是否返回 share，以及腐化进程内实际形成的本地中间 state（例如计算后的 `local_output` 和加密前后对象）。内部 A 状态、对方 package/share/凭据、T 私钥/随机带和 test controller 的数据库读取权限不属于 `V_b` 输入。

`V_b^all` 需要保留“API 返回 share=false”与“暂存 share 从未形成”之间的区别。真实 party 先在 `score_to_mask_v3_party.cpp:831–833` 得到本地输出并构造加密 envelope，然后调用 submit。随后明确 ABORT 返回 `ESCROW_ABORTED`；submit 其他异常返回 `COMMIT_STATUS_UNKNOWN`；已知 receipt 后 fetch/decrypt/receipt/share 对照异常返回 `COMMITTED_FETCH_REQUIRED`；只有本方 fetched share 解密并等于 `local_output` 才以 committed 状态返回（`:835–902`）。这些结果不表示 `local_output` 被清零或从未在内存中存在。

完整未筛选目标比较应是 `Law_real(V_b^all, status_b, status_T_public, status_A_public | x_b, public, L_S29)` 与权限正确理想执行之联合距离。`S_cap`、`S_RAND`、`S_issue`、`S_commit`、`S_response`、`S_fetch`、`S_API` 及 timeout 事件都留在 sample space 中。不能把比较对象改成只在这些成功事件上的条件 law；也不能以 `S52` 的有限成功案例推导任一真实成功概率下界。

本轮不把 `L_S29` 扩为错误细节、时间、活跃消息形状、数据库状态或恢复结果。可见错误必须由既定 API/网络/launcher 实际投影产生；A-only 内部状态仍只属于 `L_A`/A 视图。

## 4. 固定源码中的因果状态机

|顺序 / 事件|源码事实与分支|T / A / party / controller 各自可见|当前合同覆盖范围|
|---|---|---|---|
|0. 参数、密钥与配置预检|`dealer_issue_packages_impl` 校验预算、K、身份/路径、escrow 参数并调用 `read_key`，见 `score_to_mask_v3_gmw.cpp:848–870`。校验/读 key 错误会在材料与 marker 前抛出。|T/调用者看到错误；此时没有 party package/context。party 是否启动或收到调用失败通知没有被函数接口定义。|`F_T/v3` 把 issuer/签名能力理想化，未规定读 key/配置失败输出及其同次调用的 party 启动投影。|
|1. burn marker|校验通过后 `before_material()` 调用 `register_ft_v3_invocation_once`，再分配/请求材料随机带（`:874–880, 1125–1144`）。注册目录 open/fstat/权限/O_EXCL create/write/file fsync/close/dir fsync 可失败（`:492–543`）；失败会 throw，marker 不清理。|只有 T/调用者可见 marker 操作/异常及其私有 registry 状态；test controller 在专门测试中可能检查文件。party 不获得 marker bytes/errno；package 尚未生成。|理想 F_T/v3 有原子 private burn 概念，但无 marker I/O 错误、部分记录及实际持久性分布。|
|2. 材料随机生成与 cap abort|`random_bytes` 分块至多 1 MiB，每次 `RAND_bytes != 1` 抛异常且无重试（`:280–286`）。triples、permutation share 与 bounded Fisher–Yates 随后生成（`:871–943`）；FTv3 retry cap 用尽抛 `ISSUANCE_ABORT`（`:916–927`）。|T 知道随机调用/局部材料和错误；party 尚无 package。cap abort 是 issuer 私下可见的理想失败；controller 可看进程状态/测试文件但不等于 party。|S49 `F_T/v3` 规定 ideal IID tape 下 cap exhaustion→`ISSUANCE_ABORT` 给发行 caller、burn、无 package/context/online lease。它不建模真实 RAND 错误/状态及其实际概率。|
|3. 包与 context 签名/编码|材料包与签名在 `:944–977`；A context、package hashes/set hash 和签名在 `:978–1004`。异常可在签名、编码、大小检查或 context 构造阶段发生。|T/调用者看到异常和内存状态；party/A 尚未被成功交付 package/context。|S49 理想化签名能力并将签名/编码视为可执行的确定性后处理；没有失败状态 law。签名优势未量化。|
|4. 三文件顺序发布|依次 `durable_create(P0 package)`, `durable_create(P1 package)`, `durable_create(A context)`（`:1004–1007`）。每个文件使用 O_EXCL、write、fsync、close、父目录 fsync（`:361–385`）；失败 catch 关闭 fd，但不 unlink。|T 可见文件操作/部分 artifact；party/A 只有外部发布器交付后才知道收到的内容。controller 可能检查目录。不同发布阶段可能留下 0/1/2 个完整或部分文件；源码不是三文件原子事务。|F_T/v3 只定义全部成功发布的 packages/context 或 cap abort；不定义部分发布 artifact、发布者可见错误或发往各角色的截断/缺失交付。|
|5. 在线 party 前缀与本地输出|仅在 party 包被交付并启动后，双方沿 GMW/Beaver-v3 运行；本地 `local_output` 先形成后加密，party API 有失败也可能已拥有暂存 share（`party.cpp:831–833`）。|P_b 可见本方 package、双端协议消息、公开活动比较结果及自身内部状态；A context 与 peer secrets 不给 P_b。|S50 `Kpre₃` 只覆盖理想 IID FTv3、`cap_success` 的 pre-escrow view，不能单独模拟发材失败前缀。S31 原文件缺失，泄露来源按 S32/S42 摘要继承，不扩充 L。|
|6. 首份 authenticated deposit|A 检查 TLS identity/context/request/envelope 格式并把本方密文/hash 作为 PENDING deposit 写入当前事务（`escrow.cpp:314–411`；首笔可在自己的事务 COMMIT 后留下）。|A 内部可见 context、package-set/context hash、密文/hash、到达方/顺序和 PENDING；P_b 只见本方 submit 和后续等待，不收到 DB PENDING 行；peer 密文不交给 P_b。|F_TOPK-ATOMIC 有逻辑提交/恢复概念，但未定义所有超时/异常 cut 的 joint state kernel。真实有限 timeout 仍 OPEN。|
|7. 第二份 deposit 与 token|当两行齐全且 state=PENDING，A 在同一 submit 事务中读取两份 hash、`RAND_bytes(token,32)`、UPDATE state=COMMITTED+receipt、SQL COMMIT（`escrow.cpp:245–260,411–442`）。RAND 失败、UPDATE/COMMIT/query 失败会 throw；catch 尝试 ROLLBACK 但忽略其返回值。|A 知道随机与事务操作；P_b 无法从异常单独判断事务实际是否回滚/已提交。A handler 对异常只 stderr/关请求，无结构化错误帧（`:566–624`）。|S56 的 ideal independent token law 只在有效 deposit 已齐、token 均匀且后续成功不按 token 值筛选时成立。没有真实 A RNG/SQLite 错误联合分布或成功概率下界。|
|8. COMMITTED/ABORTED/未知|服务会等待第二份 deposit；restart 时 PENDING→ABORTED（`:537–542`）；超时也尝试 PENDING→ABORTED（`:544–564,590–606`）。只有明确且认证的 ABORT response 才归类 definite abort；缺失/无效状态或 connection error 是 caller unknown。|A DB state 仅 A/controller 可见；P_b 收到可验证 ABORT 才知道 ABORT，否则可能 API `COMMIT_STATUS_UNKNOWN`。UNKNOWN 不是独立 A DB state。|`F_TOPK-ATOMIC` 的既有摘要规定某些 PENDING restart/timeout→ABORTED、未知回执需恢复，但实际超时/故障 cut 与对外状态未形成无条件组合功能。|
|9. fetch/恢复及 API 输出|fetch 只在 COMMITTED 返回请求方 ciphertext 与原 token（`:445–473,609–613`）；响应格式包括 status、transaction length、ciphertext length、receipt、transaction、ciphertext（`:475–485`）。Party 校验 receipt、解密并比较本地 share，随后才返回（`party.cpp:875–902`）。|P_b 得到自己的 fetch ciphertext/token 和最终 API 状态；A 持有双方密文/token；不能给 P_b A 的 DB 状态/peer ciphertext。|S51 Kpost₃ 可继承的只是 ideal FTv3、ideal token RNG、诚实 peer 因果提交、外生 SuccessFence 成功范围。失败/恢复前缀仍缺组合 joint kernel。|

**控制器权限隔离：**固定测试源码中的只读 helper 会直接 query SQLite transaction/share row（`VFSS/tests/moe_topk/score_to_mask_v3_process_e2e_test.cpp:786–812,1910–1919`）；发材 abort 测试还会查看 T exit/stderr、package/context/lease 文件和 registry marker（`:1597–1615`）。该测试控制器可见性不是 party simulator 的权限。测试数据库中可直接查询状态，不等于计算方能看到或模拟器有权输入该状态。

## 5. 现有理想合同是否够用

### 5.1 已有理想范围

S49 `F_T/v3` 对有效新 invocation 在 ideal IID tape 上产生相关 triples、permutation shares 和版本化签名 package/context；bounded sampler 达上限时向发行 caller 报 `ISSUANCE_ABORT`、保留 ideal private burn、不给 party package/context/online lease。这个定义足以讨论理想 cap-abort 事件本身，但 S50 `Kpre₃` 进一步条件于 `cap_success`，未给完整联合 `Law(view, issuance outcome)`。

S32 报告摘要里的 `F_TOPK-U` 给出稳定 Top-K mask 和两方 XOR shares；`F_TOPK-ATOMIC` 把共同 `COMMITTED` 设为逻辑可取回点、commit 前不返回 share，双方可在 commit 后分别恢复本方密文。S51 的 Kpost₃ 需诚实 peer 使用自己同次输出/package/凭据沿真实因果路径提交第二份 deposit，且理想执行由 SuccessFence 保证成功调度。它没有把 T 发行失败、A 请求异常/状态未知、数据库失败、timeout/恢复消息的所有状态一并定义成单一无条件状态机。

### 5.2 无条件联合 law 的缺口

令 `τ=(session,invocation,N,K,route,circuit,gate_schedule,profile)`。要证明的应是完整 sample-space 上的联合距离：

```text
Law_real(V_b^all, outcome_T, outcome_A^party, api_b, returned_share_b,
        temp_share_formed_b | x_b, public, L_S29)
≈ Law_ideal(Sim_b(x_b, public, L_S29; F_T/F_TOPK/A), corresponding outputs)
```

不对 `S_RNG/S_issue/S_commit/S_fetch/Success_actual` 条件化。最早未建模的 runtime 失败有两层：

1. **全失败集合最早项：**固定参数有效时，T 的 `read_key` 仍可因 key 路径/PEM/加载失败在 marker 前异常（`:862–870`）。既有 F_T/v3 没有相应的 `outcome_T`，因为它使用理想签名能力/固定签名 key 成功的假设。若错误只到 T caller，则 party 视图可能是“未获 package/未启动”；但 `F_T` 与 `F_TOPK` 没有同 invocation 的 delivery/launch gate，也没有定义 caller 是否通知 party。party simulator 不能读取 T 的 exception 或 key 文件状态来填空。
2. **首个持久状态项：**假定 key 可用，marker I/O 可能在 marker 不存在、已 O_EXCL 创建但部分写入、file fsync/close 成功但 dir fsync 失败、marker 已完整可见等不同 cut 失败。现有 F_T 的抽象 private burn 不给出这些实际 artifact/burn 结果的 law，也不能把 test controller 看到的 marker 作为 party simulator 输入。若外部部署仅以 party projection 为定理对象，仍缺少 `M -> delivery_b/launch_b/notification_b` 的接口映射。

此处的首个断点不是某个已证明的 transcript 区分器，而是 ideal side 缺少变量：

```text
E_T_key = {签名 key load/parse 成功或失败及其 issuer-only 结果}
E_T_marker = {registry / marker 每个 I/O 阶段的结果与私有 artifact/burn 状态}
D_b = {本方 package/context 是否交付、party 是否启动、party 是否收到 issuer 失败通知}
```

真实接口需要 `Law_real(E_T_key,E_T_marker,D_b,V_b^all,...)`；现有理想接口只给 cap-abort/success 轮廓，并未定义该联合 kernel。因此不能写出逐步耦合等式，也不能由 `Law(U)` 的 token/材料边缘均匀或 S52 E2E 成功次数推出此联合 law。

### 5.3 不做事后条件化

cap exhaustion 的 `δ_reject≤1023·2^-154<2^-144` 只继承 S49 对理想 IID tape 与 semantic-material projection 的范围，不是包含版本/签名/错误状态的完整 transcript 距离。真实 RAND error、签名/serialization、marker/三文件发布、token RNG、SQLite commit/rollback/fetch 与 timeout 的概率都没有目标环境分布或下界。因而本轮不以 `ε/p_min` 形式声称任何完整成功条件界；S52 成功样本数不是 `Pr[S_issue]`、`Pr[S_commit]` 或 `Pr[S_full]` 下界。

## 6. 托管状态与可观察结果表

|真实状态/事件|A 内部状态|P_b 可见响应/API|是否返回本方 share|视图注意点|
|---|---|---|---|---|
|尚未收到两份合法 deposit|无 transaction 或 PENDING；可已有单方 share row|party 可能仍等候、连接失败或收到后续状态；A 不发 PENDING 行|否|A 已可见第一份密文，不给 party simulator。|
|token 尚未生成|在 finalize 之前，或第二份 deposit/字段检查失败|无 token；handler 异常路径可能断连而无 structured error|否|“无 token”不自动意味着未曾持有第一方 PENDING。|
|token 已采样但 COMMIT 未知/未完成|事务内可能有临时 U/UPDATE；外部 DB 可仍 PENDING、可能 rollback 成功，或 COMMIT 结果已对外成功但 caller 查询/响应失败|通常连接异常；party 映射 `COMMIT_STATUS_UNKNOWN`|否|UNKNOWN 是 caller knowledge/API state，不是 DB state；不声称 rollback 成功或 token 已清零。|
|明确 COMMITTED|state=COMMITTED, receipt=U，双方密文/hash 可查询|收到认证 COMMITTED response 后看到 token；repeat submit/fetch 可恢复同一个 token 和本方密文|尚未 fetch 成功时否|响应可能丢失，所以 party 不一定知道 A 已 commit。WAL/FULL 不构成掉电证明。|
|明确 ABORTED|state=ABORTED（例如 restart/等待期限将 PENDING→ABORTED）|只有验证后的 ABORT response 映射 definite abort；否则仍可能 unknown|否|重启/超时数据库写的成功与客户端收包是两个事件。|
|提交后 fetch/解密/receipt 对照失败|A 仍可 COMMITTED|`COMMITTED_FETCH_REQUIRED`，保留 transaction/receipt 供恢复|否|不是 share 未在计算进程中形成；local_output 可能已存在于内存。|
|恢复 fetch 完成|A state 已 COMMITTED|authenticated fetch 返回本方 ciphertext 和同 token；API 验证解密一致后给 `COMMITTED`|是|仅本方 share 交给本方 API；另一方返回时刻可不同。|

SQLite `ROLLBACK` 在异常路径被调用但返回值被忽略；该代码事实不支持“异常即事务一定回滚”。服务 handler catch 后写 stderr 并断请求，不发通用错误响应（`escrow.cpp:615–623`）。A 服务状态、party observed response 与 controller SQL 查询三者分开记录。

## 7. 最小版本化理想接口候选（未实现）

建议在后续规格中定义**组合 wrapper** `F_TOPK-ISSUE-ESCROW/v1`，不是改变运行时、角色、成功输出或 `L_S29`。其最低状态空间：

1. 固定绑定 tuple `τ=(session,invocation,N,K,route,circuit,gate_schedule,profile)`；只接受一次同 `τ` 发行/执行，明确重复请求结果与 burn scope。
2. 将发行结果分开列为 `ISSUED`, `CAP_ABORT`, `KEY_LOAD_ERROR`, `MARKER_IO_ERROR(stage,artifact_projection)`, `RAND_ERROR(call_index)`, `SIGN_SERIALIZE_ERROR(stage)`, `PUBLISH_ERROR(file_index,artifact_projection)`。发行方 T 获得其真实调用可见的 outcome；marker、密钥路径、随机带、部分 artifact 只保留在 T/环境侧，不直接交给 party simulator。
3. 同 invocation 的 `F_TOPK` outputs/party endpoint 只在 `ISSUED` 且对应角色 package 已按源协议交付时启动。失败时 party 只获得现有真实 orchestrator/API 会送到它的消息；若当前没有通知路径就保持沉默/未启动。该映射需由真实启动器确认，不能由功能性理想机自造一条错误通知。
4. 发材成功后按协议生成本方理想输出 share与在线应用 view；A 状态单列 `NO_CONTEXT/NO_TX/PENDING/COMMITTED/ABORTED`，token 是否已采样、SQL transaction 是否提交、响应是否送达分别建模。`UNKNOWN` 仅作为某方对状态的观察/API 结果，不能作为 A DB 状态。
5. 对 A 的实际随机、DB/file 操作和网络只可用两种明确模式之一：确定公开/对抗性 fault cut（不作真实概率结论），或由目标部署证据实例化的概率 kernel `Γ_T,Γ_A,Γ_network,Γ_storage`。不能把目标联合 law本身装进 oracle当作模拟器权限，也不能给 P_b peer package/share/credential、A context、T key或 A-only state。
6. party API 仅在 COMMITTED、本方 authenticated fetch、receipt 匹配、解密验证成功后输出本方 share；同时需在腐化方内部 view 中如实表示已经计算/加密但尚未返回的 `local_output`。没有在合同中表示的内存清零不作假设。
7. A 观察单独是 `L_A`；P_b 不接收 A 的 context、package-set hash、peer ciphertext/hash、arrival order 或内部事务态。错误响应字段只复现源协议已有响应；不新增 leakage 字段。

该接口可补齐“哪些失败分布与状态转移属于理想世界”及“同调用是否启动”的规格缺口。它本身不证明 `Γ` 与生产分布匹配，也不关闭真实 T/A DRBG、成功概率、签名/文件持久性、SQLite durability/rollback、TLS/mTLS、recipient encryption、有限 timeout 或完整 GMW/Select/shuffle/output/escrow composition。若不取得目标环境资料，`Γ` 应作为显式待验证假设，而不是 PASS。

## 8. 逐方模拟权限与停止点

|参与者|可见信息|不可给腐化 party simulator 的值|
|---|---|---|
|T issuer|本次参数、签名 key load/签名调用 outcome、私有 marker/burn、随机调用 outcome、构造材料、文件 artifact 与发行错误|T 私钥、另一 party 的 package/share、A-only context 不应无条件作为 party simulator 输入。|
|P_b|本方输入/材料 package，真实协议本方 transcript、peer 的协议消息中该本方已收到的字段、A 的本方 authenticated 响应、API result、本地计算状态|对端 package/share/credential，A context/package-set digest/DB 状态，T 私钥与未授权 tape，controller logs/SQL。|
|A|metadata-only context、双端 authenticated ciphertext/hash、事务状态、receipt token、消息顺序与服务时间|A 无 recipient share decryption key；A 的 view 不与 `L_S29` 合并。|
|测试 controller|专用 E2E harness 可读进程日志、exit code、受限测试文件/SQLite 状态|不属于 T、任一 P_b 或 A 的密码学模拟接口。|

在第一处缺失的发行错误/启动联合核处停止完整 simulator 组合；不继续声称后续材料 transcript、shuffle/output 或 escrow recovery 的完整联合分布已经闭合。现有 S51 `Kpost₃` 和 S56 token 局部 lemma 仅继承其原始理想成功范围，不能越过该停止点。

## 9. 状态、未运行项目与继承证据

|门|状态|说明|
|---|---|---|
|S56 exact source receipt|`PASS_WITH_SCOPE`|仅 provenance/hash 和静态来源对象。|
|S57 full unscreened single-party simulator|`OPEN`|缺少发行错误与同次调用启动/通知联合核；后续 escrow failure kernel 也未闭合。|
|F_T/v3 ideal cap-abort path|`PASS_WITH_SCOPE`|继承 S49；仅理想 IID bounded sampler 的 cap abort/semantic material 范围。|
|T key/RAND/sign/file failure joint law|`OPEN`|目标配置、失败状态与部分 artifact law 未提供。|
|A PENDING/COMMITTED/ABORTED source map|`PASS_WITH_SCOPE`|本轮静态源码映射；不证明 DB error/rollback/durability law。|
|A token/commit/fetch real joint bridge|`OPEN`|继承 S56；无 target profile/p_min。|
|SuccessFence application messages|`PASS_WITH_SCOPE` inherited|仅理想条件成功 schedule。|
|Actual finite timeout/wall-clock view|`OPEN`|不得条件化成功事件或扩张 L_S29。|
|Full party view / GMW / shuffle / output / escrow|`OPEN`|没有端到端完整安全证明。|
|`LOW_PROBABILITY_ISSUANCE_ABORT_ACCEPTANCE`|`OPEN_CONFIRMATION`|无新增负责人明确决定。|
|N>256 security|`NOT_IN_SCOPE / OPEN`|不外推 S50 的 N≤256 条件证明。|
|Build/tests|`NOT_RUN`|本轮没有 runtime/test 修改。|
|Formal LAN/WAN/RTT/AAV86/performance|`NOT_RUN / NOT_MEASURED`|无性能工作。|
|Runtime defect reproduced|`NOT_REPRODUCED`|仅静态审查，没有运行时复现。|

继承 S52 的有限 N≤1000 功能案例不是本轮运行，也不构成成功概率下界或安全证据。自然业务 I/O fault 沿用 `NOT_REPRODUCED`；人工 kill 仍是注入工程场景，不是自然崩溃或安全结论。`VFSS-baseline/` 保持不变；未创建 secure alias、未推送、未合并。

## 10. 独立复核

Reviewer：`/root/s57_independent_review`，本轮新委派；未参与 S50–S57 的实现或相关证明推导。复核者只读检查固定 S56 源码与 S49–S56 理想合同，重新判定 `Kpre₃/Kpost₃` 的成功前提、T key/marker 的发行状态边界、A 状态到 party API 投影及最小版本化接口。其确认：当前合同不足以支持全状态无条件 simulator，属于证明/接口缺口；并建议严格区分 **key-load 这一最早未建模错误** 与 **marker I/O 这一首个持久 burn 状态缺口**。Reviewer 未编辑、构建或测试；没有实施独立功能接收，也没有实例化目标部署概率模型。

## 11. 执行记录

- 固定对象核验命令：`git rev-parse HEAD`、`git show -s --format='%H%n%T%n%P' HEAD`；另按 S56 manifest 核对源文件 Git blob / checkout SHA-256 及 S55 固定源码列表。
- 源码检查：PowerShell 行号抽取 `score_to_mask_v3_gmw.cpp` 的 `random_bytes`, marker, issuance, package publication；`score_to_mask_v3_escrow.cpp` 的 receipt, submit, state, restart/timeout/fetch；`score_to_mask_v3_party.cpp` 的 API result mapping；文本检索 S32 `F_TOPK` 摘要及 S49 ideal `F_T/v3`。
- `BUILD=NOT_RUN`，`TEST=NOT_RUN`；不重跑 S52、S35/S40 矩阵；不运行自然 I/O、LAN/WAN、RTT、AAV86 或性能测试。
- 本轮无运行时、测试、密钥、材料、证书、构建物、日志或 `VFSS-baseline/` 修改。
