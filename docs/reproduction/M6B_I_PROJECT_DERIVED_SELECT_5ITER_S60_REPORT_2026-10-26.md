# M6B-I PROJECT_DERIVED_SELECT_5ITER S60 详细报告

日期：2026-10-26
审查基线：S59 本地提交 `0f67aec45d767c4bf530b12e4d548a82d68b6800`
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 1. 结论摘要

本轮尝试为完整单方视图构造不按 `cap_success`、`S_issue`、`S_commit` 或 `S_fetch` 筛选的模拟器。结论是：当前 `F_T/v3`、`F_TOPK` 和 escrow 接口尚不能支持该模拟；这是一处精确的联合功能/证明接口缺口，不是攻击，也不是已证明的不可能性。

若安全定理只声明正常成功执行，可在原范围内继承 S50/S51/S53 的 `PASS_WITH_SCOPE`：理想 FTv3 发材成功、理想 A token 随机源、至多一方静态半诚实、诚实端点按因果提交、`1≤K≤N≤256`，以及外生 `SuccessFence` 保证合法应用消息按期完成。该结论排除发材中止、协议中途 cut、escrow 状态不确定和真实 wall-clock 成功筛选。

若把协议级中止纳入声明，当前首个接口断点是 `ISSUANCE_ABORT → 同一 τ 的 package/context 交付、P0/P1 start 与 notice` 没有定义联合路由。即便临时采用全/无启动门，在线任意消息边界中止仍缺 `Kpre-prefix^{b,j}`。Escrow 的 `PENDING/COMMITTED/ABORTED`、提交响应丢失、`COMMIT_STATUS_UNKNOWN`、恢复 fetch 可以给出候选状态机，但不能据此补出缺失的在线前缀分布。

部署故障和真实有限超时属于另外的范围：签名器/provider、RAND 错误、marker/材料文件、SQLite/rollback、服务重启、TLS/网络、机器/介质及 wall-clock deadline 的目标环境联合核尚未实例化。它们不由候选理想协议功能自动覆盖。没有运行 build、tests、故障实验或性能测量；本轮没有改运行时代码。

## 2. 对象、来源与证据分类

### 2.1 固定对象

在新隔离 worktree `C:/Users/28641/.codex/worktrees/m6b-i-project-select5-s60/MoE_Top-k_Reproduction_Evaluation` 从固定 S59 commit 创建分支 `codex/m6b-i-project-select5-s60`。审查开始时 HEAD=`0f67aec45d767c4bf530b12e4d548a82d68b6800`，tree=`1a8615481103eb7c50cf2c0c1ecf2556ac6b7c8d`，parent=`4ae16569317f4526699ccb0e55a156eafcd03a1e`，初始工作树干净。没有使用桌面主工作区或近似源码替换被审对象。

核验 S59 source/evidence manifest：

- S59 五项交付文件在固定 S59 Git 对象下逐项匹配；规范化交付摘要复算为 `003b768fd2866dec29367904fbef05521124305d91b49cbd225d5458b653db74`。
- manifest 的 `baseline` 指向被 S59 审查的 S58 commit `4ae165...`，不是 S59 交付提交；本轮按此语义检查，未把两个身份混为一谈。
- 从 S59 固定的 S58 manifest 重新核验 7 项主要源码、9 项证据输入、15 项固定 S55 runtime 源码的 Git blob 与 SHA-256；来源对应固定 S59 tree。
- S59 历史交付及其引用不改写。S60 新文档另列在本轮 source/evidence manifest 中。

### 2.2 仓库要求与范围

遵循仓库 `AGENTS.md`、`PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md`。`VFSS/` 是活动实现，`VFSS-baseline/` 保持不变。输入及输出语义保持：signed Q20.12 降序、原始索引升序稳定同分，原始顺序下恰 K 个 1 的 Top-K XOR-mask shares。角色、正常 escrow 流程和 `L_S29` 不变。不称为 BB90/BMW16 原协议复现。

本轮将证据分开标记：

| 证据类别 | S60 使用方式 |
|---|---|
| 论文定义 | 只作为研究动机/参照；不由本地功能或源码审计推导论文等价。 |
| 本地参考行为 | 以 S59 固定 Git blob 的源码状态顺序与字段为依据。 |
| 项目派生设计 | 五迭代稳定 Select、FTv3、F_TOPK 与 atomic escrow 组合属于项目自有合同。 |
| 待验证假设 | 理想 IID FTv3/A token、端点因果诚实性、SuccessFence、抽象故障 cut 及任何真实部署 kernel 均逐项标记。 |

本轮没有改变 `L_S29`，没有将测试控制器的数据库、文件或日志查询权给任一 party simulator。

## 3. 定理范围与合同分支

### 3.1 A：只声明正常成功执行

定义 `T_A`：公开调用元组为同一次 `τ=(session,invocation,N,K,route,circuit,gate_schedule)`；ideal `F_T/v3` 成功发材；双方依其有效 package 诚实运行；至多一方静态半诚实；T/A 受信且不与腐化方合谋；A 仅存密文并按共同提交点工作；理想 A token RNG；外生 SuccessFence 保证双方合法应用消息在本模型的截止点前完成；`1≤K≤N≤256`。

在此模型，沿用 `Kpre₃` 的成功前缀核及 `Kpost₃` 的成功托管后缀核：

```text
Law(Vpre_b | x_b,z_b,L_S29,pkg_b,static_b,P,cap_success)
    = Kpre_3^b(...)

Law(Vpost_b | Q_b,Vpre_b,cap_success,SuccessFence)
    = Kpost_3^b(...; A, HonestEndpoint_(1-b), SuccessFence)
```

其中诚实对端使用自身同次 `x_(1-b), z_(1-b), pkg_(1-b)` 和凭据产生其 deposit；`Sim_b` 不获得 peer 秘密、A context、T 私钥或 `J_b`。在既有 SuccessFence 假设下，这两条核可按因果顺序组合为成功应用消息视图 `PASS_WITH_SCOPE`。它不表示失败分支为空、不表示部署 timeout 视图安全，也不证明真实 T/A RNG、加密、签名或 TLS。

### 3.2 A+B：将协议级中止纳入未筛选完整视图

目标联合对象应是同一 `τ` 下不按任何成功事件筛选的分布：

```text
Law_real(V_b, issue_outcome, delivery_b, start_b, share_formed_b,
        escrow_observation_b, fetch_b, api_b
        | x_b, public(τ), L_S29)
≈ Law_ideal(Sim_b(x_b, public(τ), L_S29; F_joint), same ideal outputs)
```

模拟器必须按同一个联合执行核采样所有终态概率、每方观察以及它们与已见 prefix 的相关性；不能只提供成功分布再把其余概率质量设成一个无定义的 `⊥`。当前接口无此完整联合核，故 `A+B=OPEN`。

### 3.3 C：部署可用性故障

签名 key 读取/解析失败、RAND/provider error、marker/文件部分写入、SQLite 错误或回滚失败、服务重启、TLS/网络断开、机器/存储故障和真实 deadline 均列为部署/可靠性边界。若未来将它们并入定理，须有目标部署状态感知故障核，并区分 A 的隐藏持久状态与 party 可见响应。当前无目标环境核，`C=OPEN`。外生 SuccessFence 不等于部署保证；也不等于真实 timeout 下的成功条件分布证明。

## 4. 因果状态链与各方可见性

`τ` 的一次调用按以下顺序审查。表中的“观察”指 party 实际应用/API view；A 的内部状态和测试控制器观察分开。

| 阶段 | T / A 内部 | 诚实 peer | 腐化 party `P_b` 可见 | 不应交给 `Sim_b` 的内容 |
|---|---|---|---|---|
| 初始化/签名配置 | 实际代码先读 signer key；尚未有 party 材料。 | 无在线行为。 | 通常只看调用启动、最终发材返回或进程错误。 | T 的私钥、文件系统状态、测试控制器日志。 |
| invocation burn | T 通过 invocation marker 防重复；marker 操作有创建、写、fsync、close、目录 fsync 等阶段。 | 未启动。 | 可能未收到任何通知；是否收到一致错误尚未形成 owner 合同。 | marker 内容、目录状态、部分文件副作用。 |
| FTv3 sampler | ideal FTv3 cap-exhaustion 时 issuer caller 收到 `ISSUANCE_ABORT`、invocation burned、无 package/context/online lease；实际 RAND/provider error 不是该理想 cap 事件。 | 若无 package，则不应因理想功能自行启动。 | party 如何获知 cap abort/不启动，在当前跨功能接口中未规定。 | ideal/真实随机带、peer 包、T caller 私有结果。 |
| package/context 发布 | 成功路径顺序写多个 package/context；实际文件发布不是一个经证实的跨文件原子事务。A 仅持 metadata-only context。 | 只从自身合法路由取得自己的包/凭据。 | 自己收到的 package/profile、公开参数和调用结果。 | peer package/share/credential、A-only context、package-set digest、T key。 |
| READY/GMW/Select | 双方在有效 package 下运行多层比较与后续门；A 尚无 escrow transaction。 | 按真实协议消息因果交互。 | 本方 package、共享协议 transcript、公开参数、获准泄露和本方本地状态。 | peer 份额、完整 peer package/随机带、A 状态。 |
| 本地输出 share 形成 | 在线 party 可在进程内先形成 `local_output`，然后加密 envelope。 | 形成并加密 peer 自己的份额。 | 本地 `local_output` 及本方 envelope，即使以后 API 不返回。 | 测试 harness 控制器所持另一方重构结果。 |
| 第一个 submit/PENDING | A 可在第一笔 deposit 后有 PENDING transaction。 | 其后按时序发送第二笔 authenticated deposit。 | 自己 submit 及随后 response/wait 状态；没有 A 的 DB PENDING 查询权。 | peer ciphertext/hash/到达顺序、A context 和 DB row。 |
| 第二 deposit/COMMITTED | A 验证双方 deposit 后生成 token 并更新 COMMITTED；两者在服务事务里有顺序联系。 | 真实因果发送第二份材料，不由腐化方模拟器伪造。 | 仅其收到的 receipt/status。 | A token 随机带/peer ciphertext 和内部提交观察。 |
| response / fetch | A 可能已 COMMITTED，即使 party 没收到响应。fetch 对身份返回本方密文/receipt。 | 以自己的身份取得自己的密文。 | `ESCROW_ABORTED`、`COMMIT_STATUS_UNKNOWN`、有效 receipt 或 fetch 恢复状态；本方 ciphertext/share。 | 对方 fetch 内容、DB 实际状态（除响应可推知部分）、控制器 SQLite 查询。 |
| API 最终返回 | 无变化。 | 只返回自己的 API 结果。 | 只有成功 fetch、receipt 校验、解密及本地一致性检查后才得到 API share；可能是无 share 错误/可恢复状态。 | 不得将“API 无 share”解释为“本地 share 未形成”。 |

上述源码锚点均来自固定 S59 Git blob（manifest 中列出 blob SHA-1 与 blob 内容 SHA-256），不是从当前主工作区抽取：

- Dealer `VFSS/src/moe_topk/score_to_mask_v3_gmw.cpp`：signer key read/parse `862–870`；O_EXCL marker 与 write/fsync/close/parent fsync `472–543`（EEXIST replay 拒绝 `510–513`）；FTv3 cap exhaustion `916–927`；成功路径 package/context 顺序发布 `1005–1007`。
- Party `VFSS/src/moe_topk/score_to_mask_v3_party.cpp`：本地 share 及 envelope/submit `799–836`；明确 escrow abort `837–842`；其它 submit 异常→`COMMIT_STATUS_UNKNOWN` `843–856`；receipt 错误 `858–872`；commit 后 fetch 错误→`COMMITTED_FETCH_REQUIRED` `875–894`；取回/比较成功后 API 返回 `897–902`；外层错误映射 `903–919`。
- Escrow `VFSS/src/moe_topk/score_to_mask_v3_escrow.cpp`：提交事务 `314–443`；第二份 deposit 触发 token 与 COMMITTED 更新 `411–437`，rollback 尝试 `438–440`；本方 fetch `445–473`；启动恢复 PENDING→ABORTED `537–542`；wait timeout 状态转移 `544–564,590–606`；handler 异常记录/关闭连接 `615–623`。
- TEST_ONLY controller 的 SQLite/file/log 检查位于 `VFSS/tests/moe_topk/score_to_mask_v3_process_e2e_test.cpp:786–812,1910–2017`。这些是测试观察权限，不是协议方或 simulator 权限。

数据库状态变量 `S_A∈{NO_TX,PENDING,COMMITTED,ABORTED}` 与客户端观察 `O_b` 不同。特别地，`COMMIT_STATUS_UNKNOWN` 是客户端观察，不是 A 的数据库行状态；发生它时 A 可能尚无事务、仍 PENDING、已 ABORTED，或已经 COMMITTED 但响应丢失。`share_formed_b`、`ciphertext_submitted_b`、`share_fetched_b`、`api_returned_share_b` 是四个不同布尔量，不能合并。

## 5. 不筛选模拟尝试与首个断点

令 `E` 表示理想合同规定的 issue/start/online-cut/A-response/fetch 事件。未筛选视图必须满足整条链式分解：

```text
Law(V_b,E | Q_b) = Law(E | Q_b) · Law(V_b | Q_b,E)
```

只有两项均能由正确权限的 simulator 生成，且二者保留相关性，才得到联合模拟。仅知道成功条件核 `Law(V_b | Q_b,E=success)` 不给出 `Law(E|Q_b)`，也不产生 `E≠success` 分支。历史成功测试样本数不构成 `Pr[E=success]` 下界。

### 5.1 理想 FTv3 `ISSUANCE_ABORT`

已知理想 cap-abort 本身可写为：

```text
δ_cap := Pr[cap_abort] = 1 - ∏_{i=2}^{m}(1 - ρ_i^7), ρ_i=(2^32 mod i)/2^32
state_T(τ) := BURNED
pkg_0 = pkg_1 = ctx_A = online_lease = ⊥
```

等式仅在理想 IID tape、每个 Fisher–Yates 位置使用互不重叠独立抽样块时表示精确 cap-abort 概率。用 `ρ_i<2^-22` 的保守界得 `δ_cap≤(m−1)·2^-154≤255·2^-154<2^-146`。这是理想随机带上的 cap 失败界；它既不是目标环境 RAND error 概率，也不直接界定含版本、签名、notice、失败响应和部署状态的完整协议 view。若拿“永不 cap abort”功能和有界 FTv3 比较，该概率质量只按 S49 所述语义材料投影使用；本轮不把它移植成完整转录的总变差界。

这还不足以推出 party view，因为接口没有定义这个 issuer-only outcome 如何映射到同一 `τ` 的 `deliver_0,deliver_1,start_0,start_1,notice_0,notice_1`。S59 首断点仍有效。负责人未决定通知是否一致、是否允许一方 unknown/timeout，也没有经实现的 all-or-none launch barrier。

若 proof-only 候选经批准规定确定性全/无启动：cap abort 时两方均不收 package、不启动；A 不创建交易；无本地 share。只要 party 的视图确实没有其它可观察的发材查询/通知消息，这一**候选**失败分支可由空协议转录采样，发行端私有 burn 不泄给 party。若要求两方收到 `ISSUANCE_ABORT`，则需 notice channel 的消息和送达核；若允许单独丢通知，则需联合送达向量核，不能用“双方同错”替代。以上不是当前合同或 runtime 已具备的保证。

### 5.2 预启动材料验证拒绝

对静态半诚实方，ideal FT 发出的本方 package 有效；公开参数不兼容、调用参数非法等可在验证前由公开输入决定并以确定性拒绝核模拟。若拒绝检查仅读取本方 package 和公开 tuple，其本方观察是该函数的确定输出。任何依赖 peer package、隐藏 A context、在线 transcript 或失败后已经交换的帧的拒绝，都不能套用此简化；当前接口未给相应 joint kernel。此处为源码/模型分类，未运行验证拒绝测试。

### 5.3 在线协议中途 abort：全 A+B 定理的第一个剩余断点

若候选启动门解决 5.1，在线 cut `j` 的本方 view 必须至少生成：

```text
Kpre_prefix^{b,j} = Law(V_b^{≤j}, share_formed_b=0 |
    x_b, public(τ), L_S29, ISSUED, E_cut=j)
```

对于 cut 晚于本方输出形成，核还须采样 `share_formed_b=1` 及其本地 share/加密材料，同时对 API 保持未返回。S50 的 `Kpre₃` 则是到 output-share 形成的成功 transcript，条件输入含理想本方输出 share `z_b` 与 `cap_success`。把一条已采样完整成功转录截断，得到 `Law(V_b^{≤j}|z_b,success)`；这不是目标 `Law(V_b^{≤j}|E_cut=j)`，除非另证相同条件核或 prefix 与未来输出条件独立。S50 没有给这条等式、prefix kernel 或该独立性证明。

因此完整协议级 abort 定理停止于此，不向后组合。断点是未证明的条件分布，不是具体反例、攻击或不可能性。模拟器不得通过收取完整成功包的未来 `z_b`、peer 材料或测试控制器状态来绕过它。

### 5.4 A/escrow 中止、UNKNOWN 与恢复

在**额外假定**此前完整 pre-escrow 视图已由一个合法 simulator 生成、且本方 `local_output` 已形成后，托管候选后缀可以这样描述：

1. 诚实 peer 使用本方同次输出、package、身份凭据及 envelope，因果执行第二次 deposit；模拟器不伪造 peer deposit。
2. A 在第一份有效 deposit 后为 `PENDING`；第二份验证通过后生成一次 token、持久化 token 与 `COMMITTED`；成功 response/fetch 对每方只给自己的字段。
3. 抽象 event cut 可以发生于“第一份 deposit 后”“第二份 deposit 前”“commit 后 receipt response 前”“fetch 请求/响应之间”。commit 前 A 可处于 `PENDING` 或按功能规则进入 `ABORTED`；commit 后响应丢失时 `S_A=COMMITTED` 仍可对应 `O_b=COMMIT_STATUS_UNKNOWN`，之后以身份认证 recovery fetch 返回本方 share。
4. 明确 ABORT 响应映射到 `ESCROW_ABORTED`，不返回 API share；但若此前已越过在线 output 阶段，party 内存可已形成本地 share。fetch 未完成/校验失败可返回 `COMMITTED_FETCH_REQUIRED` 或 unknown；只有成功 authenticated fetch、receipt/decrypt/local-share 对照完成才 `api_returned_share_b=1`。

上述是所提 ideal state kernel 的可执行抽象顺序，不是本轮证明完成：S51 `Kpost₃` 只为成功路径提供核；现有 A 服务异常处理、SQLite 提交/rollback、不确定结果和重启实际分布尚未以部署 kernel 对齐。全 A+B 证明也先被 5.3 的 pre-escrow cut 阻断。

## 6. proof-only `F_TOPK-ISSUE-ESCROW/v1` 候选规格

下述仅作为下一步讨论的最小候选；`CONDITIONAL / NOT_APPROVED / NOT_IMPLEMENTED`。该规格既不改变已确认的正常份额托管流程，也不暗中替负责人选择失败语义。

### 6.1 调用与发材层

- 调用绑定完整 tuple `τ=(session,invocation,N,K,route,circuit,gate_schedule,party identities,A service identity,certificate identities,recipient-key hashes,signer-key identity,FT version,sampler profile,package version,receipt version,escrow-state parameters)`；版本字段分别绑定，不混为一个版本。每份 package、metadata-only context、启动授权、deposit、receipt 和 recovery request 都校验同一 tuple 中其适用字段；公钥/证书使用公开身份或指纹，不把私钥给 simulator。
- ideal `F_T` 对每次 `τ` 只消耗一次材料。状态为 `UNSEEN→BURNED→ISSUED` 或 `UNSEEN→BURNED→CAP_ABORT`；重复 `τ` 的调用得到 `REPLAY_REFUSED`，不得静默重发。
- `ISSUED` 原子逻辑事件把各自 package 仅交付给各方，把 metadata-only context 仅交给 A。此逻辑交付与实际多文件发布之间的 refinement 另行证明。
- `CAP_ABORT` 先 burn `τ`，不生成/交付 package/context/online lease，不启动 A transaction。是否通过统一可信通知分别告知双方，是 `notice_policy` 的 owner 决定；候选必须择一：`NO_NOTICE_AND_NO_START`、`BOTH_NOTICE_BEFORE_START`、或显式 per-party delivery cut。不得将失败发送抽象成“双方自动知道”。
- 只有双方均收到同次 `ISSUED` 的有效启动授权后，才可进入在线阶段（all-or-none gate）；若不同步通知被允许，则须增加对应状态核并证明泄露。

### 6.2 在线协议与 abort/cut

- party simulator 的外部输入限该方输入、公开调用元组与 `L_S29`，以及功能在相应事件前已交付的本方 package/功能输出；它必须自行采样中间状态，不能把真实执行的本方实际 prefix 或 cut 前局部状态作为“输入”直接交给 simulator。不得得对端材料/输出/凭据、T 私钥、A context、A 内部状态、controller DB/file/log 访问或 reduction-only oracle。
- 若 cut 发生在本方 `local_output` 已形成但 API 尚未返回，候选功能还须明确：corrupted-party simulator 可在该本地形成事件之后、且仅针对被模拟方，取得与本方内部状态一致的 staged `z_b`；party API 仍保持未返回，诚实对端及其他角色不获此状态。cut 早于形成事件时不得提前给 `z_b`。这是把真实腐化方自身内存视图纳入模拟接口，不加入 `L_S29`，也不把该值交给 honest party；当前 `F_TOPK` 未定义这个分阶段接口，仍是额外的 proof obligation。
- 若定理覆盖中途在线 abort，功能必须先冻结一个有限的 operation-indexed `E_cut` 集合及逐 cut 转移表：cut 在哪个逻辑消息前/后、哪些已发送帧已交付、诚实端点何时停止、A 是否已被接触、各方观察哪个确定状态/响应。当前报告只给接口骨架，尚未枚举全部允许 cuts 或为每个 cut 给出转移核，故它不是闭合的可执行 functionality。基线候选只容许与消息内容无关的外生 cut；不包含真实 wall-clock 或 secret-dependent scheduler。
- 对每个合法 cut `j` 要提供权限正确的 `Kpre_prefix^{b,j}`；若 share 在 cut 前形成，功能分别记录本地 `share_formed_b=1` 与 `api_returned_share_b=0`。不将本地变量清零或“从未形成”作为抽象语义。
- 该 prefix 核目前缺失，是完整 A+B 合同可以实现前的证明义务；版本号本身不会自动产生它。

### 6.3 Escrow 和恢复层

- 隐藏服务状态：`NO_TX → PENDING → COMMITTED` 或 `PENDING → ABORTED`；第二份合法 authenticated deposit 才可触发 COMMITTED/token。服务按每个 identity fetch 本方 ciphertext/token。
- 每方观察是另一个变量：收到的 `COMMITTED/ABORTED` response、`UNKNOWN`、fetch 可重试、`COMMITTED_FETCH_REQUIRED`、最终 API share。commit response 丢失时允许 `S_A=COMMITTED, O_b=UNKNOWN`；恢复沿 transaction identity fetch，不把 UNKNOWN 当 DB 状态。
- duplicate submit/fetch 幂等复用状态和同一 token；恢复不得改变本方身份绑定。transaction 内随机 token 的理想生成只在提交核中一步采样，真实 A RNG 仍单独开放。
- 对每个 `E_cut` 要有联合分布 `Law(S_A, delivery_0,delivery_1,O_0,O_1,recovery_0,recovery_1 | V_pre,E_cut)`；不能把 A DB 状态泄给 party simulator，也不扩大 `L_S29`。

### 6.4 排除范围与待决语义

候选抽象 cut 不包括 signer/provider/RNG 错误、文件/marker 部分持久化、SQLite/rollback/物理断电、TLS/真实网络、DNS/connect deadline、进程 crash 或机器/介质错误。这些 C 类需要目标部署联合 kernel。候选也不等于现实服务实现、也不证明成功概率、可用性、掉电耐久性或 wall-clock timeout 隐私。

## 7. 确认事项

以下均为 `OPEN_CONFIRMATION`，不因既有“算法随机失败数值上限”暂缓决策而默认接受：

1. FTv3 sampler 达上限后烧掉 `τ`，是否合法输入下允许以新的 invocation 重试？
2. 发材/启动失败是否要求双方都收到明确失败且均不得启动？通知不可达时，是否接受一方观察 `UNKNOWN`/超时？
3. 安全定理只覆盖初始化和发材成功后的正常隐私，还是还覆盖协议级中止和恢复状态？
4. 当前安全阶段是否明确限制 `N≤256`？S52 `N≤1000` 是有限功能案例，不是安全扩域证明。

不重复询问已确认的正常密文提交/托管/各自取回合同，也不把人工 kill 可靠性测试含义重开。

## 8. 门禁、审查和验证状态

| Gate | 状态 | 本轮说明 |
|---|---|---|
| 固定 S59 来源复核 | `PASS_WITH_SCOPE` | 精确 commit/tree/parent、S59 delivery canonical、引用 source/evidence Git blob。 |
| `T_A` 成功消息视图 | `PASS_WITH_SCOPE`（继承） | 理想 FTv3 cap-success、ideal A token RNG、SuccessFence、N≤256。 |
| cap-abort 同次调用 party 路由 | `OPEN` | 当前无 `deliver/start/notice` 联合核。 |
| pre-start 本地公开验证拒绝 | `CONDITIONAL` | 只在拒绝由本方材料/公开参数确定时。未测试。 |
| 在线中途 abort prefix | `OPEN` | 无 `Kpre-prefix^{b,j}`；为 A+B 的首个剩余断点。 |
| escrow 中止/UNKNOWN/recovery 核 | `CONDITIONAL` | 可按候选 event-index 状态机规格化；未与真实服务联合分布等价。 |
| `F_TOPK-ISSUE-ESCROW/v1` | `CONDITIONAL` | proof-only、待批准/实现/独立证明。 |
| C 类部署故障联合模拟 | `OPEN` | 无部署 signer/RNG/storage/DB/TLS/OS kernel。 |
| 真实有限 timeout/wall-clock 条件视图 | `OPEN` | 不外推 SuccessFence。 |
| `LOW_PROBABILITY_ISSUANCE_ABORT_ACCEPTANCE` | `OPEN_CONFIRMATION` | 等待负责人决定。 |
| runtime/测试修改 | `NOT_RUN` | 无发现可复现代码缺陷；本轮静态审查未做运行实验。 |
| BUILD / TEST / 历史矩阵 | `NOT_RUN` | 按证明任务范围未运行。 |
| 自然业务 I/O 故障 | `NOT_REPRODUCED` | 继承项目状态，本轮未重测。 |
| 正式 LAN/WAN、RTT、AAV86/性能 | `NOT_RUN / NOT_MEASURED` | 不属本轮。 |
| `VFSS-baseline/` / secure alias / push/merge/PR | `UNCHANGED / NOT_CREATED / NOT_PERFORMED` | 本轮无相应操作。 |

独立审查由 `/root/s60_independent_review` 完成；其未参与 S50–S60 推导，复核了本报告的 theorem 分支、联合分布停止点、A/party 状态分离和源码状态映射。Reviewer 要求并确认两处修订：将 cap-abort 精确概率与并集上界分开；补全候选 `τ` 绑定字段并明确 cut 枚举和逐 cut 转移核仍未完成。Reviewer 随后也复核 staged-own-`z_b` simulator 输入只在本方 share 形成后提供给该腐化方 simulator、不属于 `L_S29`，且早期 cut 不获未来 `z_b`。Reviewer 接受候选仅作为 proof-only 讨论方案，不批准 `T_AB`、runtime refinement 或部署故障/timeout 范围。只读审查不等于功能接收或完整密码学证明；其未编辑、未构建、未测试，也未运行故障实验。

## 9. 本轮执行与下一步

- 固定来源：S59 commit/tree/parent 与引用 Git blob；创建隔离分支，无主工作区变更。
- 阅读：AGENTS、PROJECT、实施计划、S59 决策/报告/gates/manifest，S50/S51/S53/S57 的依赖证明与状态说明。
- 源码：只读核对 signer load、burn marker、FTv3 cap、顺序发布、party local share/submit/abort/unknown/fetch、A submit/commit/token/restart/timeout/fetch；无运行时差异。
- 不构建、不跑测试、不注入异常、不测自然 I/O、不运行历史矩阵或性能。
- 下一步：先由负责人裁定第 7 节合同问题；若选择 abort-aware 模型，先冻结每个 `E_cut` 和 notice/launch policy，再独立构造 `Kpre-prefix^{b,j}`。没有该核时，不继续把 post-output escrow 状态机组装成完整协议证明。随后才可审查 ideal cut 与真实 A/T/transport 的 refinement；部署故障和真实 deadline 仍单独过门。

### 实际命令与结果

| 命令/操作 | 结果 |
|---|---|
| `git rev-parse HEAD` | `0f67aec45d767c4bf530b12e4d548a82d68b6800` |
| `git rev-parse HEAD^` | `4ae16569317f4526699ccb0e55a156eafcd03a1e` |
| `git rev-parse 'HEAD^{tree}'` | `1a8615481103eb7c50cf2c0c1ecf2556ac6b7c8d` |
| `git status --short --branch`（创建隔离分支后、写文档前） | `codex/m6b-i-project-select5-s60`，clean。 |
| Python 只读读取 S59 manifest，并对固定 commit 的 Git blob 做 SHA-1/SHA-256 复算及 S59 canonical 复算 | S59 五项 delivery canonical=`003b768fd2866dec29367904fbef05521124305d91b49cbd225d5458b653db74`，匹配；S58 primary/evidence/S55 runtime source 的 `7+9+15` 行均与预期 blob 匹配。 |
| `rg -n` 对 S50/S51/S53/S57 报告及固定 runtime 源做条件/状态/代码路径定位 | 读取证明条件和来源锚点；没有运行代码。 |
| `git diff --cached --check` | 无 whitespace 错误。 |
| `git diff --cached --quiet -- VFSS VFSS-baseline` | 无 staged runtime/test/基线修改。 |
| Python `json.load` + manifest Git-index hash/canonical verifier | gates 与 source/evidence JSON 均解析；S60 五项交付 Git blob 与清单匹配，规范摘要=`a823db063ebf9d769e173963a07b49819512d4eff0616bc8b45e9fc040b86bb5`；S59 旧 canonical 仍匹配。 |

`cmake`、编译、CTest、功能/故障 E2E、网络抓包、历史功能矩阵和性能命令均未执行；不将来源核验或 JSON 校验称为运行测试。
