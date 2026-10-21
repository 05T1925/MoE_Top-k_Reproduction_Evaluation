# M6B-I-PROJECT-SELECT-5ITER-S36 条件模拟裁决

日期：2026-10-21
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`
裁决范围：S35 精确提交 `77f9684e1a50794004fd31f3e5ee7f193e9b12ec` 的真实源码；本轮仅证明/规格审查，没有源码、构建或测试改动。

## 1. 裁决摘要

**首个活动比较 `kTagOpenedCompare` 的成功路径条件分布在理想独立预处理带下可闭合。** 对 P0、P1 分别成立：固定本方完整 package 和此前 view 后，每个未消费 Beaver 门的全局掩码仍独立均匀；真实本方 partial opening 可由本方 triple 与采样的 D/E 重算；对于已包含在 `L_S29` 的活动 XOR outcome，peer bit 唯一等于 `outcome XOR local_bit`。sample seed 和过往 outcome 固定后，Select5 下一活动边是确定的，故可沿归纳继续。

这把 S35 所记“首个 `kTagOpenedCompare` 的 full prefix 条件等式未证明”改为**经非作者逐项复核的条件证明草案**，不是安全 PASS。模拟归纳可覆盖成功路径的 GMW gates、Select5 控制、membership/inverse route，及固定最终 share 下的输出刷新前缀。五次算法迭代不等于 GMW 比较依赖层或应用消息阶段；本轮没有新的计量。

按 `Sim_b(x_b,z_b 或 ⊥,public,L_S29;F_T/v1)` 的字面接口，继续走到 A submit 时遇到下一个未闭合点：客户端实际收到的 commit receipt 依赖 A-only context 和另一方密文摘要，而 P_b package 不含这些值，模拟器也没有被明确赋予接收 A-side context 并调用诚实 A emulator 的接口。这个缺口可由单独的 `F_A-ATOMIC/v1` 服务模拟接口补足，且不把 A context 交给 P_b、不扩张 `L_S29`；但本轮没有把它当成已存在功能或已证明步骤。

后续的任意 abort/crash prefix 仍未闭合：`F_TOPK-ATOMIC` 固定了逻辑输出/commit 语义，却没有逐个网络、submit、receipt 和进程崩溃切点定义完整 view 条件分布。本轮停止向后外推。未发现可复现攻击或确定性运行时缺陷；证明/接口缺口不等同于攻击。

## 2. 固定基线与证据分类

- 本轮隔离 checkout 的 HEAD 精确为 S35 commit `77f9684e1a50794004fd31f3e5ee7f193e9b12ec`；开始时工作树干净。没有从 main、S36 之外的分支或现有脏工作树替代基线。
- S35 manifest 的 85 项规范源码快照 canonical SHA-256 为 `af693eb61183a606c64a1f1b16ee5c3ad6e4b382ec5456e541c7915b8f95faa0`；S35 manifest 文件 SHA-256 为 `62be2127ae4eb28eb2c6d5d5b1dc2a5d46d068d6a68a62f2c0835708abdc6c07`。85 项逐文件哈希在 S35 原 worktree 已核对；S36 HEAD 与该提交一致。Windows checkout 的行尾过滤不被拿来冒充新的规范源码快照。
- S35 binary SHA-256 `f511f262e574ddb430317894f63a97e725dd0a62e3f475e21bd44f0da94984ba` 是 S35 历史 manifest 中的身份。本轮没有重建、运行或重新验证该二进制；不产生新的 build identity。
- S29/S31 规格文件来自用户工作区中已存在的历史资料，本轮只读使用，记录在 S36 manifest 的 supplemental references 中；没有修改其所在 worktree。

| 证据类型 | 本轮采用内容 | 结论边界 |
|---|---|---|
| 论文定义 | Protocol I 文献定义 | 不推出本项目电路是 BB90 五迭代、BMW16 或 BB90+DCF 复现。 |
| 项目规格 | S29/S31 的 `F_TOPK-U`、`F_TOPK-ATOMIC`、`L_S29`；用户确认的 A 条件信任 | 作为待证明合同，不将合同写成源码已证明性质。 |
| 源码事实 | S35 GMW/Beaver-v3、Select5、signed packages、escrow 状态机 | 只描述固定 commit 的实现。 |
| 测试观察 | S35 manifest/report 记录的历史 E2E、故障与资源点 | S36 未重跑；不作为 S36 新观察或安全证明。 |
| 待验证假设 | 理想独立 Dealer tape、CSPRNG/签名/密码学优势、诚实 A 模拟、abort/crash 功能 | 保留为条件或 OPEN，不填数值。 |

S31 是协议规格/TEST_ONLY 审查；S32 是作者 runtime 实现与功能记录；S33 的原样版本后来被 S34 发现 SIGPIPE 缺陷，不能作为独立接收通过版；S34 对修复作定向复测；S35 才以 metadata-only A、签名域隔离、S34 修复为对象提交并接收限定功能证据。本轮不改写这些历史记录，负责审查 S35 提交后的第一个条件模拟节点。

## 3. 角色、理想功能、材料及泄露合同

### 3.1 角色与腐化模型

| 角色 | 持有或可见内容 | 本轮基线模型 |
|---|---|---|
| T：离线可信发材方 | 全局 triple/permutation 采样带、签名私钥；产生 P0/P1 各自 package 和独立的 signed escrow context | `F_T/v1` setup/signing oracle；T 离线后不收输入或在线 transcript；T 排除腐化集合且不与 P_b 合谋。模拟器用 oracle 取得与固定 verifier key 相容的签名材料，不持 T 私钥。 |
| P0/P1：在线计算方 | 本方 additive score share、本方 package/triple/permutation shares、本方 recipient 私钥和 TLS 身份、本方最终 XOR share、与 peer/A 的 view | 分别考虑单个静态半诚实腐化；另一 party、T、A 诚实且不合谋。恶意 party/双方合谋不覆盖。 |
| A：在线 escrow | metadata-only context、ciphertext/hash、提交状态、receipt、恢复请求；没有 share 解密密钥，也不持 party package | 用户已确认的条件受信、与 T/P0/P1 不合谋，负责原子可见性和身份受限恢复。它不是原“两方在线、Dealer 离线”角色模型中的透明旁观者。 |
| A-only 加强模型 | A 的全部服务元数据、ciphertext、状态和时序 | 单独的半诚实腐化 A 模型；`Sim_A(public,L_A)` 不与 P_b simulator 合并，本轮仍 OPEN。恶意 A 或 A 与任何角色合谋不在模型中。 |

本轮采用的 F_T/v1 setup 输入为固定 session/invocation、N/K、双方/A 身份与 recipient-key fingerprints、circuit/gate-schedule hashes、状态机参数及固定 verifier key。功能内部采样全局相关 Beaver/permutation 材料，只把 P_b 自己的 signed package 和获准公共绑定元数据交给该 party；另把 metadata-only signed escrow context 交给 A。签名通过固定 T verifier key 的理想 signing oracle 产生，模拟器不取得 T 私钥，也不取得 peer package body。真实材料 generator 的 CSPRNG 与签名实现优势仍另计。
理想成功输出是稳定 Top-K 的原序 XOR mask shares：signed Q20.12 score 降序、分数相同时原始索引升序，恰 K 个 1；`F_TOPK-U` 令 `z0←U({0,1}^N)`、`z1=z0 XOR M`，每方只得到自己的 share。v3 另用负责人冻结的 `F_TOPK-ATOMIC`：提交前双方 API 输出 `⊥`；最终 COMMITTED/receipt 是双方都可恢复各自密文 share 的逻辑可见点。第一份密文可以先是 PENDING；第二份密文及最终 COMMITTED state/receipt 在第二次提交路径的 SQL transaction 中完成。两个客户端实际收到回执/返回 share 的时刻不保证物理同时。WAL/FULL 或进程 SIGKILL 不构成掉电耐久证明。

### 3.2 材料账本和消息 DAG

真实顺序为：

1. T 产生固定 gate schedule 的 Boolean Beaver triples、秘密 one-hot permutation matrix shares、本方签名 package，以及仅含服务元数据的独立 signed context。P0/P1 各自只收自己的 package；A 只收 context。
2. P0/P1 预检并烧毁 one-shot lease，然后 READY。源码先调用 `score_to_mask_v3_consume_package_lease` 再交换 READY，见 `score_to_mask_v3_party.cpp:497-502`。
3. 对本方 additive score share 转 Boolean 位 share；按 63N 个 GMW 门执行模 2^32 加法，见同文件 `:504-545`。
4. 用 T 的 permutation shares 做 forward route；为所有 priority-key 对构造 borrow/comparator table，见 `:550-626`。
5. Select5 每轮在 forward 阶段之后交换 seed/ack；随后只公开获准活动边的 local comparator bit share，并交换 `kTagOpenedCompare` frame。下一活动图依此前 seed/outcomes 确定，见 `:628-667`。
6. 从 `selected_position` 得到秘密 membership，再用逆 permutation share route 回原始索引，见 `:670-691`。
7. P0 采样刷新向量 R 并发固定 N 长度的 `R XOR m0`，P1 用本地 mask share 得到 z1；见 `:693-723`。
8. 双方各自用 recipient RSA-OAEP + AES-256-GCM 加密本方输出并提交 A。A 可先落 PENDING；第二份到达后更新 COMMITTED/receipt。receipt/fetch 恢复受身份与交易绑定；P0/P1 API return 可因回执到达时间不同而异。

源码的 permutation-share 生成另取随机矩阵 share0，并令 share1=Pi XOR share0（score_to_mask_v3_gmw.cpp:531-545）；固定 Pi 后任一 party 的本地矩阵 share 仍均匀。此局部 law 支持同一模拟器继续处理 forward/inverse route，但不取代对 T 完整生成带及真实随机源的优势分析。材料账本：每个 gate id 对应一份 `(a_b,b_b,c_b)`，one-shot 消费状态在调用 I/O 前写入内存；固定总门数公式继承 S31 `G=63N+N²W+C(N,2)(2W−1)+N²`、`W=32+log2(N)`。`G` 和 W 是实现预算/算法依赖结构，不是本轮性能测量。P0 的输出 refresh R 是在线本地随机材料。A 的接口不接收 triples、permutation shares、输入/输出 share 或 T 私钥；S35 的 OS 文件权限负测只覆盖其测试进程边界，不构成任意部署环境下对恶意 A 的密码学约束。

### 3.3 `L_S29` 与 `L_A` 分开

S29 §3.3 的既有 `L_S29` 字段是：公开参数和协议绑定；shuffle 后批准的 endpoint/位置；pivot 与比较结果 bit；forward shuffle 完成后、仅按活动迭代公开的 sample seed（不得预公开未来 seed）；批准的 masked values；comparison count、通信长度和统一 ABORT。S29 将“approved masked values”作为类别，没有为本 GMW-v3 写出每个 frame 字段的新增 schema。故 S36 不把所有 Beaver D/E opening 追加为新 leakage 输入：它们是 party 完整 view 的字段，模拟器须在内部采样/重建；如其某字段原已属于已批准 masked-value 项，也不得因此把其他中间位加进 L。

明确不新增原始 score/rank、原索引映射、selected original index、inactive comparator bits、内部 wire/triple/permutation、party package、T 密钥或完整 mask。活动 seed/edge/outcome 仅限已经由 `L_S29` 批准的活动前缀。A 的 `L_A` 单独记载 context 签名元数据、交易和 package-set/circuit/schedule 哈希、ciphertext/hash/长度、到达顺序/时序、pending/commit/abort、receipt、重启和 fetch。不得将 `L_A` 混成 P0/P1 的 `L_S29`。

## 4. 首个活动比较的条件分布证明草案

目标分别为：
```
Sim_b(x_b, z_b 或 ⊥, public, L_S29; F_T/v1),  b ∈ {0,1}
```
以下只证明无 I/O 故障的成功转录前缀，且以 F_T/v1 的理想独立随机 tape 和签名 oracle 为条件。

### 4.1 package-conditioned 单门 opening

S35 `score_to_mask_v3_gmw.cpp:511-529` 对每个 gate 独立均匀采样全局 A_g、B_g 及 share bits A0_g、B0_g、C0_g，并令 C_g=A_g AND B_g；P0 package 存 \((A0,B0,C0)\)，P1 存 \((A XOR A0,B XOR B0,C XOR C0)\)。

对任一固定 \((A_g,B_g)\)，P0 triple share 三个位独立均匀；P1 triple share 是 P0 三元组的固定平移/双射，故同样均匀：
```
Pr[T_{b,g}=t | A_g=a, B_g=b] = 2^-3.
```
因此，即使条件于腐化方整个 triple package（而不是只看单门边缘）和已经完成的历史，只要 gate id 未消费，当前 \((A_g,B_g)\) 仍为均匀且独立。跨门随机位独立，故逐门归纳可用。

设真实 AND 输入份额为 \(X_g=x_{0,g} XOR x_{1,g}\)、\(Y_g=y_{0,g} XOR y_{1,g}\)。开值
```
D_g = X_g XOR A_g;   E_g = Y_g XOR B_g
```
对固定全局 wires 是对均匀 \((A_g,B_g)\) 的平移，故即使条件于完整此前 view、本方 package 和真实 wires，仍均匀于 \(\{0,1\}^2\)。源码本方 partial 为 `d_b=gate.x XOR triple.a`、`e_b=gate.y XOR triple.b`；模拟器抽均匀 D/E 并输出 peer partial `D XOR d_b, E XOR e_b`，这与实收 frame 的逐字节 bit payload 分布相同。输出 share 按源码公式重算：
```
c_b XOR (D AND b_b) XOR (E AND a_b) XOR ([b=0] AND D AND E)
```
来源：`score_to_mask_v3_party.cpp:210-257`。这证明的是条件于本方 package/完整先前 view 的联合替换，不是只凭无条件“Beaver 开值均匀”。

### 4.2 活动比较、条件化及自适应续接

源码发送本方 bit vector 后以 XOR 得活动 outcome，见 `score_to_mask_v3_party.cpp:638-663`。令本方模拟出来的 bit 为 q_b，`L_S29` 已提供该活动比较 outcome o，则 peer bit 唯一确定为 `q_{1-b}=o XOR q_b`；不需要 peer 输入、peer package 或全局 Dealer tape。若 o 不与公开输入/先前 leakage 兼容，则该 leakage 点属于不可能的理想记录，不纳入条件分布等式。

sample seed 由 P0 的 CSPRNG 生成后发送、P1 返回 SHA-256 acknowledgement，见同文件 `:160-194`；seed 按 S29 只在活动轮次公开。给定已有 seed、匿名活动 endpoints/pivot 和 outcomes 后，Select5 控制器决定后继活动边；此处没有额外向模拟器提供 inactive comparator table。固定下一层 gate 的 wire inputs 后，继续用 4.1 的新鲜门归纳。于是 P0、P1 的完整成功路径本地 package/opening/活动消息前缀具有同一条件 law：
```
Law(V_b^prefix, local bits, peer bits | x_b, z_b, public, L_S29; ideal F_T/v1)
 = Law(Sim_b(...)).
```
被模拟变量包括 package 内随机材料、门 partials、opening 和比较 peer message；并未把这些变量从 view 中删掉或升级为公共泄露。

### 4.3 固定理想输出 share 与后续成功路径

P0 输出 z0=R；发送 `u=R XOR m0`。P1 得 `z1=u XOR m1=R XOR M`，来源 `score_to_mask_v3_party.cpp:693-723`。理想随机 R 与先前计算 transcript 独立，因此给定任一固定 z_b 不改变计算前缀 law：P0 模拟输出刷新时置 R=z0；P1 将接收的刷新值置为 `u=z1 XOR m1`。这支持沿同一 Beaver sampler 经过自适应 Select5、membership 和逆路由直到本地输出 share；不单独证明所有输入下的 Top-K 电路正确性，也不把有限 E2E 当成证明。

### 4.4 独立复核结果

非作者密码学审查者 `s36_conditional_crypto_review` 未改代码或运行测试，逐项核验了两方 local triple 条件均匀性、固定 package/previous view 下的 D/E 条件分布、已批准 outcome 对 peer bit 的唯一补值、seed/outcome 决定自适应边、以及固定 z_b 下刷新关系。审查结论：**H4 在 ideal independent tape + 单方静态半诚实 + 成功路径下条件闭合**；真实 RNG、签名/摘要、TLS 和后续加密优势必须仍以符号保留。该 reviewer 同时指出 receipt 接口是更早于任意 crash-prefix 的下一个缺口，详见下一节。此项是条件证明草案的独立推导复核，不是密码学准入委员会的最终认证。

## 5. 沿真实转录的下一个断点：A receipt

真实 receipt 由 `score_to_mask_v3_escrow.cpp:245-266` 计算，输入包含 transaction、session/invocation/N/K、circuit/schedule hash、context hash、package-set hash 以及两份 ciphertext 的 SHA-256。只有第二份 share 插入后状态仍为 PENDING 时，服务在同一最终 SQL transaction 算 receipt 并置 COMMITTED，见 `:359-422`。P_b 收到 receipt 后在 party 侧只检查 64 位十六进制形式并返回本地 share，见 `score_to_mask_v3_party.cpp:725-765`。

`Sim_b` 的字面输入只有本方 `x_b,z_b,public,L_S29` 与本方 `F_T/v1` package oracle 结果。腐化方 package 不含 A context hash、package-set hash 和 peer package hash；P_b 也看不到 peer ciphertext。receipt 是 P_b 完整 view 的字段但不是 `L_S29`。因此，不额外获得服务模拟能力时，模拟器不能直接算出与真实 A 相同的 receipt。仅以“SHA-256 输出看似随机”替代计算不成立。精确缺失的是 `Law(R_b=H(domain,tx,context_hash,package_set_hash,H(C_b),H(C_{1-b})) | x_b,z_b,public,L_S29,V_b^pre)` 的 sampler：字面接口没有 `context_hash/package_set_hash` 和 peer ciphertext；不能由本方 package 推出该条件分布。

**最小接口补充建议（未实施/未证明）：** 将 `F_T/v1` 的角色路由与 `F_A-ATOMIC/v1` 模拟 oracle 明文分开：T 的 signed context 只交给 A emulator；P_b simulator 仍只拿本方 package。A emulator 使用 context 中已签名的 peer package hash 和 session/transaction 绑定，接收本方模拟密文以及一个 dummy peer recipient ciphertext，按真实 A 状态机计算 receipt 并只把相应服务返回值交给 P_b。若真实 RSA 公钥材料不在 public 输入，须为模拟器明确公开 recipient public key 或使用经过定义的 A-side deposit oracle；不能索取 peer package 或 T 私钥。固定 dummy share 的有效 RSA-OAEP/AES-GCM envelope 替代 peer share ciphertext，需要对应 IND-CPA hybrid；receipt 的 SHA-256 只是该不可区分 ciphertext 的有效率后处理。真实部署 mTLS 另用 TLS hybrid；不能假定模拟器掌握 peer TLS 私钥。该接口不扩展 `L_S29`，也不把 context 给 P_b，但 S35 的 `F_T/v1` 只定义 context 的 A-side recipient，未定义它如何作为 `Sim_b` 内部诚实服务模拟器调用，因此 receipt hybrid 状态仍为 **OPEN**。

这是**模拟器/理想接口缺口**，不是攻击、可复现代码缺陷或 A 恶意行为。未运行区分器或 toy 枚举；最小可检验探针是 N=2,K=1：固定本方输入、输出 share、seed/outcomes，比较真实 receipt 与获得 context-to-A 路由的模拟 receipt，同时改变 peer share 明文但保持 envelope 和调用长度不变；记录 receipt 的全 256-bit 输入前像和 A state 请求。该探针将检验模型/实现连线，不会替代 IND-CPA 或条件证明。

之后还有一个更晚的**abort/crash-prefix 模型缺口**：S29 没冻结每个 transport/submit/receipt/crash 切点的逐方输出/前缀分布；S35 `F_TOPK-ATOMIC` 规定 precommit 双 `⊥` 与 commit 后双方可恢复，但不提供切点输入。party catch 会返回 `ABORT_V3` 并清理 API 输出，submit 响应不确定则返回 `RECOVERY_REQUIRED/COMMIT_STATUS_UNKNOWN`，源码 `score_to_mask_v3_party.cpp:729-789`；已形成但尚未 API 返回的局部 share 仍在腐化方内存/view。若要证明该范围，最小模型改造是版本化 `F_TOPK-ATOMIC/v2` 加环境指定的故障切点、已交付 byte/frame 前缀、A linearization/receipt 是否发生及是否送达；不扩张 `L_S29`。或将当前证明明确限制为无故障成功路径并把 crash 安全保持 OPEN。它不是 S35 的 runtime bug。

## 6. A 的模拟范围与安全优势边界

P0/P1 的基线模拟以 A 诚实、条件受信且不合谋为前提；A context/receipt 的角色路由须按第 5 节补全。单独半诚实腐化 A 时，`L_A` 必须覆盖 signed metadata、package-set/circuit/schedule/context hashes、ciphertext 与 hash/长度、到达顺序/时序、pending/commit/abort、receipt、restart/fetch。A 无解密密钥；用 dummy ciphertext 替换真实密文须 IND-CPA/AEAD 与上下文绑定的正式 hybrid。SQLite/WAL/FULL 和进程级 SIGKILL 不证明掉电耐久性。A-only 视图模拟及耐久/时序模型仍 OPEN。

未来任何完整定理界都需保留符号项，至少包括：
```
Adv_total ≤ ε_FT + Adv_DRBG(q) + Adv_sign + Adv_hash
          + Adv_TLS + Adv_RSA-OAEP/AES-GCM
          + ε_GMW-conditioned-L + ε_A-model + ε_abort/timing
```
该式是未量化的候选账本，不是本轮已证界。GMW/Beaver 的常用定理不能代替这里对实际 package-conditioned transcript、活动公开位、回执和 escrow 状态的组合证明。

## 7. 本轮执行与状态

本轮只做源码只读与文档/条件分布推导，没有动态复跑、编译、benchmark 或故障注入；无运行时修改，因此不重复 S35 的 195 案矩阵。S35 记录仍只能作为历史证据：

| Gate | S36 状态 | 边界 |
|---|---|---|
| S35 commit 与 85-file manifest identity | `PASS` | 精确 commit 与既有规范 hash；不创建新 build。 |
| H4 首个活动比较条件 law | `CONDITIONAL_PASS_REVIEWED` | ideal F_T 独立 tape、成功路径、静态半诚实；符号优势未量化。 |
| P0/P1 完整成功转录至 receipt | `OPEN` | 缺 F_T role-routed context + A emulator / receipt sampler 接口。 |
| P0/P1 arbitrary abort/crash prefix | `OPEN` | 缺环境切点与完整逐方 prefix 功能/分布。 |
| F_T joint real CSPRNG/signing bridge | `CONDITIONAL / NOT_QUANTIFIED` | 只证明理想随机带下的 sharing law；未量化实现优势。 |
| A-only simulator | `OPEN` | 与 base trusted-A P_b 模型分开。 |
| 自然读写故障 | `NOT_REPRODUCED` | S35 正常无注入路径记录；S36 未运行新通信路径，不宣称不存在。 |
| N≤256 功能 | `PASS_WITH_SCOPE`（历史 S35） | 不作 S36 新功能接收或安全外推。 |
| N>256 | `NOT_ADMITTED` | 未扩大准入范围。 |
| 正式 LAN/WAN / RTT | `NOT_RUN / NOT_MEASURED` | 无正式性能测试；S35 phases 不是 RTT。 |
| secure alias / v3 default | `NOT_CREATED / OFF` | 不创建别名，不默认启用。 |
| S35 runtime source/build | 未改/未重建 | 原 commit 保留，binary hash 只援引 S35 manifest。 |

S36 不修改 `VFSS-baseline/`，不清理或覆盖 S35/历史 worktree；不加入密钥、材料、证书、构建物或原始大日志。route 保持项目派生实验，不写作 BB90/BMW16/BB90+DCF 复现或安全通过。
