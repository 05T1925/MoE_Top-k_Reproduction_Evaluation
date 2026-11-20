# M6B-I-PROJECT-SELECT-5ITER-S29 工作报告

日期：2026-10-20
路线：`PROJECT_DERIVED_SELECT_5ITER`；S29 为理想功能/party-view 安全架构审查，不是 BB90/BMW16 复现、密码学攻击报告或性能验收。

## 1. 摘要裁决

| 审查项 | 状态 | 本轮结论 |
|---|---|---|
| S25 对象身份 | `PASS` | 对指定 S25 工作树逐一重算 74 个文件哈希，0 个不匹配；canonical SHA-256 与 S25 manifest 相同。 |
| S25 功能收据 | `PASS_WITH_SCOPE` | S27 对相同源快照做过限定范围独立接收；S29 只继承该证据，不重跑测试。 |
| P0 final-mask-only `END_TO_END_SIM` | `OPEN/NO_GO` | 缺少从 `(x_0,z_0,public_params,L)` 生成完整本方视图的模拟器。 |
| P1 final-mask-only `END_TO_END_SIM` | `OPEN/NO_GO` | 缺少从 `(x_1,z_1,public_params,L)` 生成完整本方视图的模拟器。 |
| `MODULAR_COMPOSITION_VALID` | `OPEN/NO_GO` | 单独证明输出 `Q′_b` 的 score-conversion 子功能，不能自动证明最终 mask-only 外层功能。 |
| `CURRENT_INTERFACE_SECURITY` | `OPEN/NO_GO` | 当前 v2 接口没有完整安全证明；这表示缺少证明，不表示已构造攻击。 |
| `VERSIONED_REDESIGN_REQUIRED` | `CONDITIONAL` | 若 final-output-conditioned proof-only 路线不能闭合，采用本报告 §8 的 v3 fused 接口；此状态不声称 v2 不可能安全。 |
| 泄露字段变化 | `NO` | 未把 `Q′_b`、rho 或其他内部状态加入 L。 |
| 安全范围 | `N≤256: OPEN; N>256: NOT_RUN` | N≤256 只继承限定功能结果，不表示任何 N 的隐私通过。 |
| secure alias | `NOT_CREATED` | 安全门未关闭。 |
| 正式 LAN/WAN | `NOT_RUN / NOT_MEASURED` | 后置；本轮没有测量。 |

结论是**证明义务尚未闭合**。本轮没有发现可复现区分器、攻击或运行时代码缺陷。首个外层断点是：模块候选模拟器以 `Q′_b` 为条件，但 `F_TOPK` 只给腐化方最终 mask share `z_b`；尚无证明可从 `(x_b,z_b,L)` 采样 `Q′_b` 与 score-conversion 前缀的正确联合分布。若模拟器自行抽取 `Q′_b`，还须证明它与最终输出 share、L 及后续 transcript 的条件联合 law。均匀单方边缘、刷新零和和功能正确性都不能替代这一步。

## 2. 被审对象与证据边界

### 2.1 S25 精确身份

- 对象：`C:\Users\28641\.codex\worktrees\m6b-project-select5-s25\MoE_Top-k_Reproduction_Evaluation` 下 S25 manifest 固定的候选快照。
- 分支：`codex/m6b-project-select5-s25`；base HEAD：`c3926c68fd14f270faa8b55234311071947fa080`。
- manifest：`docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S25_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-19.json`；SHA-256 `c96450eb62b186e866291b6c4415441e0ef3216703612314542e1d2ee25372eb`。
- 候选 canonical SHA-256：`64f32993b12bcd8692ed089f5dcdb8ce1be9f181ff217b508b8b0470eb8e440d`；74/74 个文件本轮复算匹配。
- 这是建立在上述 base commit 上的 manifest 固定工作树快照，`candidate_committed=false`。base commit 本身不含该 74 文件改动，不能取 main 或别的分支代替候选。

S25 与 S27/S28 证据的区分：S27 的报告和 manifest 记录了相同 74 文件对象的功能复核、独立接收和限定结果；S28 明确记录它的执行者与 S27 相同，所以 S28 是新的技术分析，不是新的独立人员签核。S29 的接收者另行检查 S25 manifest 与 S27/S28 的理想功能、组合论证和安全状态；其未参与 S19–S28，且没有运行测试。该复核是独立接收意见，不是组织签字或完整密码学证明。

### 2.2 四类证据

| 证据类别 | S29 使用范围 |
|---|---|
| 论文定义 | Boyle et al. 的 DCF Theorem 2 / Appendix F.1 只作为具体 DCF 构造的安全依据候选；论文按 PRG 混合证明单方 key 隐私，不直接证明本项目 score conversion、同 key 双次 Eval、整包辅助状态、Select5 自适应消息或最终 mask 协议。 |
| 本地代码行为 | S25 manifest 固定的 `VFSS/` 源码决定实际材料、调用、消息、输出和崩溃前缀。代码行为不反向成为论文结论。 |
| 项目派生推导 | `F_TOPK`、`F_scoreconv/v2`、允许泄露 L、下文模拟器和 hybrid 是本项目的功能/安全定义。Select5 五层不是 BB90/BMW16 算法。 |
| 待验证设想 | rho/DCF 联合随机性界、carry/sign 条件 sampler、由 `z_b` 条件化的 `Q′_b` 分布、自适应整池及最终输出/中止视图模拟均未证明或未量化。 |

论文参考为 Boyle et al., *Function Secret Sharing for Mixed-Mode and Fixed-Point Secure Computation*, Theorem 2 / Appendix F.1。它给 DCF key pseudorandomness 的单原语证明框架；将其移植到本 ABI 及完整协议 view 仍需要本地 reduction 和条件混合。

## 3. P0/P1 理想功能、泄露与模拟器接口

### 3.1 外层 `F_TOPK` 成功语义

令 `b∈{0,1}`。P0、P1 私有输入分别为长度 `n` 的 Q20.12 signed score shares `x_b`，重构满足 `raw[i]=(x_0[i]+x_1[i]) mod 2^32`。公开参数包括 `n,K,padded_n,h,c`、score/tie 语义、party/session/fingerprint、协议和 DCF ABI。功能按 signed score 降序、original index 升序决定稳定 Top-K bit-mask `M`，要求 `M∈{0,1}^n` 且 `Σ_i M_i=K`。

现有项目文字只要求输出 secret-shared mask，没有在本轮所核对的合同中冻结单方 share law。为避免把一种分布猜成原合同，保留两个精确候选：

| 成功输出合同 | 定义 | 安全含义 |
|---|---|---|
| `F_TOPK-U`（均匀 XOR） | 采样 `z_0←{0,1}^n` 均匀，置 `z_1=z_0 XOR M`；P_b 仅获 `z_b`。 | 两方单方 share 边缘均匀；分别模拟器的接口为 `(x_b,z_b,pp,L)`。这是本报告用于陈述输出条件模拟义务的候选，不声称已证明它就是 S25 adapter 的原始输出 law。 |
| `F_TOPK-C`（canonical share） | 固定 `z_0=0^n`、`z_1=M`；P_b 仅获 `z_b`。 | P1 直接获知完整 M，明显增加其可见信息；不能暗中用作较弱的原协议合同或安全证明。只有项目明确批准该输出语义变化时才能采用。 |

后续采用 `F_TOPK-U` 作为**待确认的证明目标候选**，而非裁定原协议必然如此。进入安全准入前需从 S25 adapter/已批准功能合同确认真实 share law 与其中一个合同一致；若实际 law 既非 U 也非 C，应精确写出第三个合同再重做模拟接口。对于 U，外层腐化方模拟器接口是：

```text
Sim_b(x_b, z_b, public_params, L)
```

该输入**不含** `Q_b`、`Q′_b`、`rho_b`、carry/sign share、DCF/uCMP key、对方输入、Dealer tape 或任何本地材料明文。若项目原合同并未冻结上述均匀输出-share law，应在后续准入前确认它与最终 adapter 的实际分布一致；S29 不以任何内部值补足这个接口。

在候选 `F_TOPK-U` 下，逐方合同明确如下（`F_TOPK-C` 的精确替代输出已列于上表）：

| 腐化方 | 理想输入 | 成功输出 | 模拟器可得内容 |
|---|---|---|---|
| P0 | 私有 `x_0`；功能还接收 P1 的 `x_1` 和公开参数 `pp` | 仅 `z_0`，且 `z_0←{0,1}^n` 均匀 | `(x_0,z_0,pp,L)` |
| P1 | 私有 `x_1`；功能还接收 P0 的 `x_0` 和公开参数 `pp` | 仅 `z_1=z_0 XOR M` | `(x_1,z_1,pp,L)` |

两方各自的 ideal input 包含其本方输入；功能内部取得双方输入后计算 M，但腐化方模拟器不会因此得到对方输入。公开参数无效、维度/取值越界或协议绑定不匹配时，功能应在任何 share 输出前返回统一 `ABORT`；目前具体校验集合及异常映射还没有一份冻结的理想功能表，故这是**待冻结的错误合同**，不是 S25 已证明的行为。通信错误、缺失/损坏材料和 crash 则适用下节列出的两个候选前缀合同；S29 不把实际异常文本或内部阶段自动列入 L。

### 3.1.1 score-conversion 子功能候选合同

为比较路线 B，可定义项目派生的模块候选 `F_scoreconv/v2`：输入 `(x_0,x_1,pp)`；输出完整 priority-key shares `(Q′_0,Q′_1)`，满足两方按协议重构规则重构出与固定点 score 和稳定 tie-break 一致的完整键。它在在线调用期间返回/暴露给各 party 的本地 `Q′_b`，且错误或材料失败以统一 `ABORT` 返回。此处精确键编码、错误时已消耗材料及输出是否在 crash 前已被 party API 返回，仍须在子功能规范中冻结。`F_scoreconv/v2` 只是项目模块合同候选，不是论文定义。

若此模块按半诚实腐化模型把 `Q′_b` 给 P_b 的模拟器，模块证明最多覆盖带中间 priority-key share 的扩展功能。组合时若希望外层 `F_TOPK` 仍只有 `(x_b,z_b,pp,L)`，必须另证 bridge：从该外层接口采样 `Q′_b` 和全部 score-conversion local view 的真实条件联合分布。

### 3.2 中止合同的两个候选

现有资料冻结了最终功能和 `NO_COMPLETE_MASK` 测试谓词，但没有完整定义任意 abort/crash 时每方的理想输出。不能选择一个较弱合同并称其证明了原协议。

| 候选 | 合同 | 安全含义与现有证据关系 |
|---|---|---|
| `F_TOPK-ATOMIC` | 只有双方都完成 `PUBLISH` 才给双方各自 `z_b`；任一方在此前中止/崩溃则双方均为 `⊥`。 | 要求 all-or-nothing。S25/S27 没有冻结此语义；源码在最终状态阶段前已形成本地 `output_share`，并在各方 API 返回后分别向子进程写结果。该候选不能从 `NO_COMPLETE_MASK` 推出。 |
| `F_TOPK-PREFIX` | 成功时同上；每方若自己的 API 返回则得到本方 share，若在返回前终止则该方输出为 `⊥`。另一方可在其 API 已返回时单独拿到自己的 share。进程内已形成但未返回的局部值属于 view，不自动成为理想输出。 | 允许非原子、逐方的输出前缀；只保证没有两方都完成并交付完整 mask 的情况时，不能声称原子无输出。它更贴近 party API/进程故障模型，但 S25 的探针不足以冻结所有前缀，因此仍是候选。 |

两个候选都需进一步指定：错误参数、错误/缺失材料、transport failure、统一 ABORT、crash stage、已经 claim/消耗的材料，以及 party API 是否返回输出。当前只继承 S25/S27 明确运行的故障前缀与统一 ABORT 结果；不把它们外推为通用 crash 定理。

### 3.3 当前获准泄露 `L`

沿用 S27/S28 记录的既有泄露字段，不增加字段：

- 公开参数和协议绑定信息；
- shuffle 后允许公开的 endpoint/位置；
- pivot 与比较结果 bit；
- forward shuffle 完成后，按活动迭代才公开的 sample seed（不提前公开未来 seed）；
- 已批准的 masked values；
- comparison count、通信长度和统一 ABORT。

`Q_b/Q′_b`、rho、carry/sign 本地 share、DCF/uCMP key、Dealer tape、party-local package/manifest/lease、shuffle 本地置换状态和内部输出前缀都是腐化方 **view 的待模拟部分**，不是新增的公开泄露函数输入。manifest/hash/ready/lease 如本方可见，模拟器须按模拟材料重算并生成一致状态；不能把它们从 view 中删掉，也不能把它们直接添加到 L。

### 3.4 分方真实 view `View_b`

分别对 P0、P1，完整腐化方 view 至少包括：

1. 本方输入 `x_b`、公开参数、session/fingerprint/party/ABI 绑定和错误/终止状态。
2. ready 前交付给本方的全部材料：carry/sign masks 与 DCF party keys、rho、Select DCF pool、membership pool、forward/inverse shuffle metadata；streamed 版本还包括 slot manifest、segment 及地址。
3. ready/package digest、claim/lease/replay/one-shot 状态、被读取与未读取 slot 地址和内容；party 本地状态文件、失败前缀和异常类别（以真实可见程度为准）。
4. phase 4 carry、phase 5 sign 的发送/接收 frames、同一 DCF key 的两次 Eval、本地 carry/lift/sign 值、旧 `Q_b` 及完整 `Q′_b`。
5. `key_records`、forward shuffle 的本地状态和收发 wire；Select 每轮活动端点、pivot、公开 seed、比较结果、使用/未使用 DCF 槽及其地址；membership、inverse shuffle、mask share 和最终状态交易。
6. 成功时本方 `z_b`；失败时按最终选定的 abort contract 表示的输出与前缀。crash 前已经形成本地但 API 尚未返回的 `output_share` 仍属于 view，不能据此视为 ideal output。

Dealer tape 本身不是 party 的可见输入，但它产生的相关材料、package digest 和 party-local状态共同决定 view 分布。理想模拟器不得访问真实 Dealer tape；必须自行采样到等价 joint law。至多单方腐化、可信离线 T 生成材料后退出且不与腐化方合谋；P0 与 P1 要分别模拟，不能拼接两个单方模拟器，也不能把 T+party 合谋当作本威胁模型。

## 4. S25 实际 score-conversion 输出与可见时序

源码事实（不是论文结论）：

1. `protocol_i_raw_score_input_party` 在 online 阶段先做 phase 4 carry exchange，按本地 carry DCF key 计算 carry/lift；再做 phase 5 sign exchange，并按本地 sign key 计算 sign 和旧 priority-key share `Q_b`。见 `VFSS/src/moe_topk/protocol_i_score_input.cpp:79–139`。
2. v2 refresh 为 `Q′_b=(Q_b+rho_b) mod 2^c`；helper 对完整 `c` 位 key 执行加法并清理越界检查，见同文件 `:155–170`。`rho_0←U(R_c^N)`、`rho_1=−rho_0` 是零和刷新关系，保证完整键重构不变；不意味着两方 rho 独立。
3. rho 在 ready 前已放入本方 package/slot：monolithic v4 在 package 内携带 refresh vector；streamed v3 以 purpose=5 slot 进入 ready/manifest。source 生成 ready、digest 和 durable claim/lease 见 `protocol_i_project_derived_select5.cpp:1600–1885` 与 `protocol_i_material_slot_store.cpp:368–416`。本方 package 在 claim 后供 runtime 使用；ready 本身不是 rho 的生成时刻。
4. `protocol_i_project_derived_select5.cpp:2022–2030` 调用 raw-score conversion、执行 v2 refresh、随后清除已消费 rho vector；`:2040–2045` 将完整 `Q′_b` 填入 `{Q′_b,0,0}` 记录并直接送入 forward shuffle。故 `Q′_b` 是实际 party-local 状态；它没有作为最终 mask 输出，但不能因“内部”而排除在半诚实腐化方 view 之外。
5. v2 下 Select5 消费完整 priority-key record。S25 对本地 `q_b`/局部右移的分析并未找到当前 v2 下游消费者；不应把错误的 `Q′_b>>h` 解释成已存在接口。carry/sign key 的同一 party material 会对 `d` 和 `d−2^33` 做两次 Eval，见 `protocol_i_ucmp.cpp:100–132,165–190`。
6. `getrandom` 路径负责 Select5 随机字节/sample seed（`protocol_i_project_derived_select5.cpp:390–403,2152–2202`）；BCG DCF root 使用 OpenSSL `RAND_bytes`（`dcf_bcg21_fig1_aes128_v1.cpp:253–259`）。两个 API 的调用事实不构成跨 API、跨槽或与 dealer tape 其余字段的联合独立性证明。

因此必须区分：代码运行时的本地可见性（`Q_b/Q′_b` 确实由该 party process 计算并消费）和理想功能允许模拟器拿到的内容（当前只有 `x_b,z_b,public_params,L`）。

## 5. 路线 A：当前外层功能的端到端模拟

目标对 `b∈{0,1}` 是构造：

```text
Sim_b(x_b, z_b, public_params, L) ≈ View_b
```

计算界应覆盖 DCF/AES PRG reduction、Convert 项、联合随机带、TLS channel 项及协议混合；目前这些总项未量化，不可给安全位数或 PASS。逐阶段义务如下：

| 阶段 | 真实分布必须包含 | 模拟器需要生成 | 当前状态 |
|---|---|---|---|
| Offline T / ready | 完整 party package、全部 DCF key/mask、rho、shuffle/inverse metadata、ready/digest、manifest/segment、claim/lease；它们共享实际 dealer 随机带与格式关系。 | 仅用公开参数和可模拟的公开 L 内部采样单方完整 package joint law，并重算 digest/manifest/ready/lease；不可读真实 tape 或对方 package。 | 旧材料为 input-independent 是有利条件；完整 joint RNG/跨槽/序列化 view 的界未量化。不是因“离线输入无关”即可判 PASS。 |
| Carry/sign | party-local DCF keys 与 masks、phase 4/5 frames、收到的 peer frames、同 key 双 Eval、本地 carry/lift/sign、`Q_b`，全部条件于 `x_b`、L 和隐含对方输入。 | 产生与真实条件 law 一致的 key、frame、Eval 结果和旧 key share；若调用 DCF theorem，需把实际 ABI、alpha/beta 取值、两次 Eval、辅助 frame 及其它材料放在同一 hybrid 中。 | 无完整条件 sampler。S28 的 BCG 单 key bridge 仍 `CONDITIONAL/UNQUANTIFIED`；论文 Theorem 2/Appendix F.1 不单独证明该 joint transcript。 |
| rho / `Q′_b` | rho 在 ready 前已存在于 party package/slot；完整 `Q_b` 与 `rho_b` 形成 `Q′_b`。 | 若已采得不含 rho 的 `V_pre` 且 rho 对 `(V_pre,x_b,L)` 条件均匀，给定目标 `q′` 后可取 `rho_b=q′−Q_b(V_pre) mod 2^c`。 | 代数条件式成立需要上述独立前提；没有 `V_pre|x_b,L` sampler，更没有 `Q′_b` 对 `(x_b,z_b,L)` 的 outer sampler。S24/S25 的缩小视图等式不能覆盖完整 package。 |
| 首次 `key_records` / forward shuffle | 本地完整 `{Q′_b,0,0}` records、forward shuffle local sigma/tau/a/e/r 等状态、交换 masked frame 和输出 shares。 | 先模拟 `Q′_b` 与最终 `z_b` 的正确联合分布，再按同一隐藏置换/材料分布生成 party-local shuffle view 和获准的 masked values。 | 已有条件 forward-wire 推导范围有限，未覆盖完整 shuffle state 与 package joint law。 |
| adaptive Select pool | 5·C(N,2) 候选 DCF 池、实际 key/address 访问、pivot/endpoint/比较 bit、活动轮次 seed、未使用 key、消费/lease 状态；后续查询由先前 transcript 自适应决定。 | 每轮在已模拟的前缀条件下联合替换真实活动 key/query/输出，保持未活动池与 future address 的条件分布，并正确按批准时机产生 seed 和 L。 | 未给出完整 adaptive-pool hybrid；`Q′` 前置门和单 key bridge 尚未关闭，S27 槽位/访问记录只是功能事实。 |
| membership / inverse / output | membership key、逆置换材料、最终 mask share、PREPARE/COMMIT/PUBLISH、错误/abort、lease burn、restart/crash prefix。 | 在最终选定的输出与 abort 合同下采样完整 prefix，并证明两方 mask share 的分布与外层功能一致。 | `NOT_RUN`；没有把功能成功、`NO_COMPLETE_MASK` 或本地 share 边缘当作安全证明。 |

### 首个精确断点

外层模拟器接口最早不匹配的位置是：S28 的模块候选 `Sim_sc,b(x_b,Q′_b,L_prefix)` 可以把 `Q′_b` 当成明确的 score-conversion 子功能输出；但外层 `F_TOPK` 只交给模拟器 `z_b`。因此首先需要证明：

```text
Law(Q′_b, V_score,b | x_b, z_b, public_params, L)
```

可由外层允许的输入采样，而且其边缘/联合关系与真实 transcript 相同。S25/S28 没有给出此 law 的 sampler。源码在 `protocol_i_project_derived_select5.cpp:2022–2045` 明确把 `Q_b → Q′_b → key_records` 连起来；`protocol_i_score_input.cpp:90–139,155–169` 给出前置变量与刷新。

若尝试在模拟器内部先独立抽一个均匀 `Q′_b`，这只证明边缘分布候选；仍须证明其与 `(z_b,L)`、carry/sign、rho、package/lease 以及未来 transcript 的联合 law。若给定 `Q′_b`，下一个断点是 carry/sign 单方前缀分布和同 key 双 Eval 辅助 view。两处都是**缺少证明/条件未量化**，没有已构造区分器，不能称攻击或安全 PASS。

## 6. 路线 B：分层理想功能与顺序组合

可以把结构写成：

```text
F_scoreconv/v2:  (x_0,x_1, pp) -> Q′_0,Q′_1
F_Select5/DCF:   (Q′_0,Q′_1, material) -> stable selected membership shares
F_route/mask:   (membership shares, inverse material) -> z_0,z_1
F_TOPK:          最终只输出 z_b
```

模块化证明可用于分离局部职责，但合同必须说明 `F_scoreconv/v2` 的腐化方输出：`Q′_b` 是该方实际可见的子功能输出。按当前半诚实腐化模型，攻击者控制 party process，看到该本地状态。于是：

- 若组合理想功能把 `Q′_b` 交给腐化方，证明到的是“最终 mask + 中间 priority-key share”的扩展功能；泄露函数增加了 `Q′_b`，不再是原 `F_TOPK`。
- 若组合 ideal 把 `Q′_b` 仅作为内部线连到下一子功能、不给腐化方，但真实 party view仍可观察该局部状态，就必须另给从 `(x_b,z_b,L)` 模拟它的 bridge；这正是路线 A 的缺失条件分布。
- 因此，“scoreconv 子协议安全”与“Select 子协议安全”不能单独推出原 final-mask-only 功能；一般顺序/UC 组合定理要求明确子功能 I/O 和腐化方输出，不能抹去真实腐化方可见的中间输出。

裁决：路线 B 对**扩展功能**可以是有用的证明结构；对现有 `F_TOPK` 与 L，`MODULAR_COMPOSITION_VALID=OPEN/NO_GO`。不因定义 `F_scoreconv/v2` 而添加新的泄露字段。

## 7. 路线 C：版本化方案比较

| v3 选项 | 改变的变量/依赖 | 材料、通信影响 | 泄露字段 | 必须新增的证明 | 兼容与回滚 |
|---|---|---|---|---|---|
| C1. 分离 rho 随机带或改变生成时机 | 将 `rho` 从共用随机调用中分到明确的 refresh domain；或在其余 offline package 固定后再采样。仍需 `rho_0` 与 `rho_1` 零和相关。若改为 online 生成，须设计双方如何取得零和份额。 | 仅 offline domain separation 可不加在线轮；online refresh 通常需要额外交互/相关材料，角色、round 和失败前缀需重推。刷新 slot、manifest/ready digest 必须版本化。 | 理论目标可保持 L 不变；不能把“独立 rho”增列为泄露。 | 证明 CSPRNG/HKDF domain separation 对实际联合随机带的优势；覆盖 rho 与 score masks、DCF roots、shuffle tape、package digest、lease 的 joint law。即使 rho 对 `V_pre` 条件均匀，仍未解决 `Q′_b|(x_b,z_b,L)` 及 DCF frame 条件模拟。 | 新 `schema-v3`/`score_share_abi-v3`；拒绝与 v2 混用。失败时回到 v2 仅作功能实验，不继承安全结论。 |
| C2. 改造 score-conversion 输出/Select 输入接口 | 真正有意义的改造不是把 `vector<Q′_b>` 改名为 handle；半诚实 view 仍含本地值。需新增 fused `score-to-mask-v3`，令 Select 直接消费 score-share continuation，并把外部理想输出固定为最终 `z_b`。 | 建立新的 carry/sign、refresh、shuffle、Select、membership/inverse 材料类型和 phase DAG。若仍离线产生相关材料，通信可不变但 key layout 变；若在线 refresh 或换 primitive，轮数/字节必须重新推导，未测不得估计。 | 目标合同要求 L 与外层输出不变。若需要公开/授权 `Q′_b`，就不再满足本任务范围。 | 给 fused 运行时 `View_b` 的 simulator，仍须模拟本地临时 key/continuation 状态；证明 score conversion 到输出 mask 的正确性、材料联合分布、adaptive query、shuffle、membership、abort-prefix，以及 DCF/PRG/TLS advantage。只有接口不 materialize `Q′` 不够。 | 新 runtime entry、schema 和 version binding；禁止跨 v2 package/ABI。v2 可保留为 `EXPERIMENTAL` 功能回归，不能作为 v3 rollback 的安全继承项。 |
| C3. 保留 v2 功能，显式声明中间子功能合同 | 将 `F_scoreconv/v2` 输出 `Q′_b` 及 Select/route 子功能的输入/输出/失败都写进正式接口。变量本身不变。 | 文档化可复用现有材料和消息 DAG；代码/性能可以不变。需要记录每个模块输出在 corruption view 中出现的位置。 | 若承认 Q′ 是腐化方输出，则 `L′=L∪Q′_b`，改变泄露；若不加到 L，仍要证明它从 final-only simulator 可生成，回到 A。 | 子协议功能正确性、各自条件安全证明、组合接口对应关系和 exact abort contract；不能只引用一般组合定理。 | 有利于审查但不兼容“外层只泄露最终 mask”的原目标；须新功能标签和新批准，不能静默替换。 |

### 推荐与最小可执行版本规格

本轮推荐次序：先在现有 v2 上做 proof-only 的 A 路线，因为只有它可能在不改功能/泄露下关闭；此时它尚未通过。如果该 sampler 经审查无法闭合，则不要用 B 的 intermediate-output 合同替代；启动 C2 的新版本设计。C1 可作为 C2 材料子设计，但独立 rho 不是完整方案。当前不能把“存在证明缺口”写成数学上证明必须改架构；版本化改造是**准入上的条件后备路线**。

可执行最小 `score-to-mask-v3` 设计合同：

```text
external ideal function: F_TOPK-U (same inputs, same stable mask, same z_b law)
simulator:               Sim_b(x_b, z_b, public_params, L; abort_status_if_granted_by_frozen_contract)
party corruption:        at most one semi-honest online party; T remains trusted/offline
runtime boundary:        one fused score-share -> original-order-mask API; no Q′ output API
material schema:         new v3 schema; typed carry/sign, refresh, forward/inverse,
                         adaptive Select, membership, output and slot-manifest domains
randomness:              explicit labeled substreams; actual joint reduction required
DAG:                     offline T ready/package -> carry -> sign -> refresh ->
                         forward shuffle -> active 5-iteration Select waves ->
                         membership -> inverse route -> local output prepare/commit/publish
leakage:                 unchanged L; Q′, rho, local keys/frames/package state remain view
abort:                   choose and specify one exact per-party prefix contract before build;
                         do not assert atomic no-output from NO_COMPLETE_MASK
admission:                no secure alias until P0/P1 simulators and all stage hybrids pass
```

分号后的内容只在最终选定的理想功能明确把该方 abort/输出状态作为理想输出时提供；不得把任意实际 transcript prefix、异常细节或内部阶段加入模拟器输入。当前 abort 合同尚未冻结，故 v3 的 abort 条件模拟仍是准入前置 gate，而非新增泄露字段。这个接口把需要交付的证明对象和材料边界明确下来，但它本身**不是安全实现，也不自动关闭模拟器义务**。若 v3 仍在 party-local state 中计算完整 `Q′_b`，fused API 也必须把该值纳入 `View_b` 模拟；真正的准入取决于新 primitive/协议是否给出所需 joint law。新版本需要证明：P0/P1 输出条件 sampler、cross-domain RNG/DCF reduction、每一适应轮 DCF pool hybrid、shuffle/inverse local-state law、membership/output-share law、所有 abort/crash 前缀 law；优势项量化或继续 gate 为条件，不能标安全 PASS。

## 8. 逐方裁决与后续准入

| Party | `END_TO_END_SIM` | `MODULAR_COMPOSITION_VALID` | `CURRENT_INTERFACE_SECURITY` | `VERSIONED_REDESIGN_REQUIRED` | 依据 |
|---|---|---|---|---|---|
| P0 | `OPEN/NO_GO` | `OPEN/NO_GO` | `OPEN/NO_GO` | `CONDITIONAL` | 模拟器仅有 `x_0,z_0,pp,L`；没有输出条件 `Q′_0` 与完整 package/score-prefix 联合 sampler。模块输出 Q′ 会增加腐化方可见中间值。 |
| P1 | `OPEN/NO_GO` | `OPEN/NO_GO` | `OPEN/NO_GO` | `CONDITIONAL` | 模拟器仅有 `x_1,z_1,pp,L`；`rho_1=−rho_0` 只给单方边缘关系，不提供 `Q′_1` 或 package 的外层条件分布。 |

状态含义：`OPEN/NO_GO` 表示证明尚未提供或现有接口准入拒绝，不代表发现攻击；`CONDITIONAL` 表示列明条件未完成/未量化；没有任一方 `END_TO_END_SIM` 或当前接口安全标为 PASS。没有可复现区分器。

### 逐项证明义务

1. 冻结成功时 output-share law 与上述候选 `F_TOPK-U` 一致；给定 corruption input/output/share 可运行模拟器。
2. 对每个 `b`，提供完整 `P_b` package + ready/digest/manifest/lease/claim/restart 前缀 sampler；联合涵盖未使用 DCF pool、shuffle 与 membership slots。
3. 把 S25 BCG ABI 映射到论文 DCF：固定 `alpha/beta`、PRG 替换、AES 五块 514-bit 扩展、Convert、两次 Eval 的辅助 frame 视图及 query 次数，给出可审查 reduction 和具体优势参数。
4. 证明 `rho_b` 与完整非 rho view 的条件分布；若以独立 domain-separated randomness 替代，量化全部材料联合 hybrid，不得只证明 rho 的边缘均匀。
5. 给出 `Q′_b` 与 `(x_b,z_b,L)` 的联合 sampler；由此延伸到首次 shuffle 和每个自适应 Select wave，包含未使用 pool/lease 状态。
6. 延伸至 membership、inverse shuffle、output shares 及所选 abort/crash 合同；各阶段真实/模拟 trace 逐轮可核对。
7. 汇总 DCF/PRG、RNG、TLS 和组合项优势，明确 N 依赖；不量化时保持 `CONDITIONAL/UNQUANTIFIED`。

## 9. 功能、crash 与性能证据的准确范围

- S25 功能证据经 S27 同源对象独立复核，状态为 `PASS_WITH_SCOPE`。它只说明对应 N≤256 case 的功能/错误路径与各自 oracle 检查，不证明 party-view privacy。
- S28 的执行者与 S27 相同；S28 没有重新运行构建、测试或性能矩阵，也没有新的人员独立性签核。
- crash case `NO_COMPLETE_MASK` 是测试谓词。测试在 `three_process_e2e_test.cpp:762–786` 只读取结果 pipe 的最初 8 字节并将其解析为长度头；party child 在 `:389–391` 才写长度头和 mask share payload。该探针没有读取/核对 share payload。因此 `P1_output_bytes=8` 表示 probe 收到 8 字节头，不是“收到 8-byte share”；S27 报告中对应表述不应被扩展为 payload 证据。
- runtime 在 `protocol_i_project_derived_select5.cpp:2254–2277` 先形成本地 `output_share`，再经 PREPARE/COMMIT/PUBLISH；注释明确可能有孤立本地 share。由此更不能把测试谓词改写为 atomic no-output/all-or-nothing。
- 本轮复跑：仅 S25 manifest/hash 对象身份核验。构建、CTest、oracle differential、独立进程 E2E、N 扩展、材料资源、LAN/WAN 均 `NOT_RUN`；正式性能 `NOT_MEASURED`。S25/S27 历史结果标为继承证据，不归入 S29 本轮运行。

## 10. 独立接收复核

接收者是本轮新安排的独立协作审查者，明确未参与 S19–S28 的证明撰写或决策。其检查了 S25 manifest 固定对象与 S27/S28 相关证据，并复核本报告的理想输出接口、`Q′_b` 外层条件分布断点、组合泄露后果及 crash probe 含义。结论同意将 P0/P1 四项安全门保持 `OPEN/NO_GO`/`CONDITIONAL`，并指出 `P1_output_bytes=8` 是被测试读到的长度头，不是 mask-share 负载。本轮未请该接收者复跑任何测试；这不是组织签核或密码学定理证明。

## 11. 本轮修改文件与命令

本轮新增 4 个审查文档；运行时代码改动为 0：

1. `docs/decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S29_DECISION_2026-10-20.md`
2. `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S29_REPORT_2026-10-20.md`
3. `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S29_GATES_2026-10-20.json`
4. `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S29_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-20.json`

验证命令（PowerShell，在 S25 精确工作树只读执行）：

```powershell
$m = Get-Content -Raw $manifestPath | ConvertFrom-Json
foreach ($f in $m.files) {
  (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $root $f.path)).Hash
}
# 将 path<TAB>lowercase hash<LF> 按 UTF-8、相对路径 Ordinal 排序后求 SHA-256
```

结果：`file_count=74`、`mismatch_count=0`、manifest SHA-256 `c96450eb62b186e866291b6c4415441e0ef3216703612314542e1d2ee25372eb`、canonical SHA-256 `64f32993b12bcd8692ed089f5dcdb8ce1be9f181ff217b508b8b0470eb8e440d`。S29 没有在 S25/S27/S28 worktree 写文件。

| 项目 | S29 状态 |
|---|---|
| Release/Debug builds | `NOT_RUN` |
| conformance / CTest | `NOT_RUN` |
| oracle differential | `NOT_RUN` |
| independent-process E2E | `NOT_RUN` |
| S25/S27 functional matrices | `INHERITED_NOT_RERUN` |
| N>256 | `NOT_RUN / OUT_OF_SCOPE` |
| formal LAN/WAN performance | `NOT_RUN / NOT_MEASURED` |
| binary hashes | `NOT_BUILT_THIS_TURN` |
| secure alias | `NOT_CREATED` |
| S29 runtime source changes | `0` |

machine-readable gates 和来源/构建状态见同目录的 S29 JSON。`VFSS-baseline/`、论文、密钥、证书、材料包、构建物、原始日志和本地参考工程均未修改。
