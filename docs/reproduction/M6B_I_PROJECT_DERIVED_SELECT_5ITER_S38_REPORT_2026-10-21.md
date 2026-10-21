# M6B-I-PROJECT-SELECT-5ITER-S38 独立裁决报告

日期：2026-10-21
路线：Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL
范围：S37 的无故障成功转录证明对象；本轮只审 reduction-only J_b-ENC-CHALLENGE/v1、recipient envelope 替换及其与合法 party simulator 的连接。

## 1. 裁决摘要

S38 得到两个不同层次的结果：

1. **对端挑战明文有逐点公式；在明确的 proof-only 生成权限下，可以构造联合归约采样器。** 在功能正确性前提下，对固定输入 share 对令 M=M_TOPK(x_0,x_1)，S35 输出刷新满足 z_0 XOR z_1=M，因此 z_(1-b)=M XOR z_b。J_b 不是给 party simulator 的输入，也不是一个直接吐出目标 tuple 的 opaque oracle；它按本报告列明的 Beaver/permutation 联合生成律和签名 oracle 构造 F_T/v1 状态，再执行两方应用层转移。该构造依赖 reduction-only 的 F_T 生成律/签名 oracle 权限、recipient-key 游戏匹配及置换截断误差；这些不是 party-facing F_T API。归约器保留 joint sample，Sim_b 只取得自己的 package 和正常 A 响应。

   但是，S36 H4 只给出了 party pre-cipher view 的条件 sampler；它没有证明该 sampler 与同一 F_T/v1 session 中仅交给 A 的 context/package-set commitment **联合条件分布相等**。context 与 package-set hash 被纳入实际 party 可见 receipt；真实 peer gate messages 又来自与该 package 相关的材料带。由 H4 的 party-view 边缘式、或由 A 的确定性后处理，推不出这个 joint equality。不能把 SHA-256 笼统当成随机数来补此缺口。

因此，**在 reduction-only F_T 联合生成权限、recipient key-generation 前提和精确 envelope IND-CPA 前提下，可以证明“真实本方前缀 + 真实对端 envelope”与“同一真实前缀 + dummy 对端 envelope”的密文替换子引理；它不闭合真实本方前缀到受限 Sim_b 前缀的替换。** 完整 party simulator 到 commit receipt/fetch 的成功路径仍为 OPEN。这是证明接口/相关性缺口，不是攻击、可复现代码缺陷或不可能性结论。S38 在该处停止，没有继续证明实际离线 Dealer 生成到理想 F_T/v1 的桥接。未参与本轮推导的审查者已确认 J 仅在列明的 reduction-only 权限及密码假设下有效，并确认 G1→Sim_b 的联合 receipt/fetch 条件 law 仍是首个断点；该审查没有授予完整协议安全 PASS。

本轮未改运行时代码；BUILD/TEST = NOT_RUN。未跑 S35 的 195 案矩阵、性能、RTT 或自然 I/O 复现。未创建 secure alias。

## 2. 固定基线与来源核验

- S38 隔离 worktree：C:\Users\28641\.codex\worktrees\m6b-i-project-select5-s38\MoE_Top-k_Reproduction_Evaluation，分支 codex/m6b-i-project-select5-s38。
- 起始 HEAD 精确为 S37 5c31ebfeaf6eb072421badf31d7354431b984d35，父级为 S36 ca5eb001e72c971a7dbaf87d789d4a7fbcba1b9a。S35 runtime commit 为 77f9684e1a50794004fd31f3e5ee7f193e9b12ec。没有以 main 或相似工作树替代。
- S35 source/build/evidence manifest 记录 85 项源文件及 canonical SHA-256 af693eb61183a606c64a1f1b16ee5c3ad6e4b382ec5456e541c7915b8f95faa0。本轮重新读取仍保留的 S35 worktree，并对 manifest 的 85 项逐文件计算原始文件 SHA-256：**85/85 匹配**；canonical path/hash 文本摘要也复算一致。该工作树 HEAD 是 S35 commit，状态干净。
- S35 manifest 的工作树文件摘要与直接对 Git blob 计算的摘要有 23 项不同；核对确认这些差异全是 Windows checkout 的 CRLF/LF 行尾转换。没有把 blob 字节摘要冒充成 manifest 的文件摘要。
- S37 commit 相对 S35 的 VFSS/ 文件差异为空；因此本轮被审运行时仍是 S35 85 项规范快照。S37 的新增内容只涉及文档、gates、manifest 和计划记录。
- S35 历史 Release binary SHA-256 f511f262e574ddb430317894f63a97e725dd0a62e3f475e21bd44f0da94984ba 仅作为历史记录引用；S38 未重新构建，未验证该二进制。
- S37 决策、报告、gates 与 manifest 是只读输入；S38 未覆盖或改写 S37 记录。未改 VFSS-baseline/，未纳入密钥、材料、证书、二进制或原始大日志。

本轮实际来源核验命令/操作：`git rev-parse HEAD`（S38 起始 HEAD=`5c31ebfeaf6eb072421badf31d7354431b984d35`）；`git status --short --branch`（S35 原 worktree clean）；`git diff --quiet 77f9684e1a50794004fd31f3e5ee7f193e9b12ec 5c31ebfeaf6eb072421badf31d7354431b984d35 -- VFSS/`（exit 0）；PowerShell `Get-FileHash -Algorithm SHA256` 对 S35 manifest 85 个列项逐个比较并按 ordinal path 重算 canonical 摘要（85 matches、0 mismatch、canonical 如上）。

证据分类保持分离：论文 Protocol I 定义不证明此项目电路是 BB90、BMW16 或 BB90+DCF；Select5 和稳定 mask 是项目派生规格；以下字段与计算顺序来自 S35 源码；历史 E2E 只作为历史功能观察，本轮没有复跑；F_T/v1、密码优势和 A 服务功能是证明假设，不是实现级安全结论。

## 3. 模拟游戏和权限

### 3.1 Party simulator

对固定 b∈{0,1}，party simulator 接口为：

    Sim_b(x_b, z_b, public, L_S29, pkg_b;
          F_T/v1 role route, F_A-ATOMIC/v1)

输入包括：

- 腐化方自己的 additive score share x_b；
- F_TOPK-U / F_TOPK-ATOMIC 给该方的 committed output share z_b；
- 公开 setup：N、K、session/invocation、circuit/gate-schedule、两把 recipient RSA 公钥及身份/公钥指纹绑定；
- 腐化方本来持有的静态辅助状态，包括自己的 recipient RSA 私钥、TLS 客户端凭证/私钥和本方服务身份；这些是本方状态，不包括对端私钥；
- 既有 L_S29；
- T 签发且只交给该 party 的 pkg_b；
- 由证明实验维护的 opaque F_T/A 角色路由句柄。句柄只让模拟器把本方请求送到对应服务会话；它不编码或返回 `ctx_A`。F_T 将 context 内容只交 A，服务响应只按真实 party endpoint 返回。

模拟器不拿 x_(1-b)、pkg_(1-b)、z_(1-b)、ctx_A 内容、T 私钥、peer recipient/TLS 私钥或 peer 的 Beaver/permutation/random tape；也不将这些量加入 L_S29。A/T 与腐化 party 不合谋。A-only corruption、party+A 合谋不属于本轮。

### 3.2 证明归约器与 J_b 权限

J_b 是与 party simulator 分离的 IND-CPA reduction/sampler，不是 simulator 的新增输入口。其证明专用输入允许：

- 固定实验输入对 (x_0,x_1)；这是外层安全环境的 proof-only 输入，不送给 Sim_b；输入可由环境在挑战 bit 产生前固定，或由归约器先运行环境取得；
- 两把公开 recipient RSA key、两方 key fingerprint、session/invocation 和 OAEP/GCM 所需公开 AAD；归约不取得 peer 解密私钥；
- 本地执行下述显式 `J_b` 算法所需的理想独立 F_T tape 与 T 签名 oracle。签名 oracle 只对 J 生成的 package/context 消息签名；归约器不读取或持有 T 私钥。`J_b` 在本地生成完整相关状态并按 F_T 角色规则路由；它不调用一个直接返回目标 tuple 的抽象联合采样 oracle；
- 归约内部生成的 z_b、z_(1-b)、L_S29、V_b^pre、pkg_0/pkg_1、ctx_A 和 A 状态。对照的 party simulator 只看到 pkg_b 及获准输入/服务响应。

这个 proof-only sampler 权限是本轮 IND-CPA 归约的明确前提，不是 S35 运行时 API，也不是 party-facing F_T/v1 额外返回的对象。签名 oracle 以固定公开验证钥对应的签名接口为前提，只签 J_b 新生成的 package/context 消息；这不向 Sim_b 或在线 party 暴露 T 私钥。形式化游戏若只允许归约器使用 role-routed F_T/v1、不给它在本地执行同一 joint generation law 的能力或不给兼容的签名 oracle，则 J_b 条件未满足，密文替换子引理也仍标 OPEN；不能默默把 peer package 或 ctx_A 塞给 Sim_b。

### 3.3 比较对象：联合分布与点条件

对每个固定输入对 (x_0,x_1)，采用 standard simulation 的 **joint distribution** 判定，比较理想功能产生 z_b、L_S29 及协议/服务随机性的整体分布：

    (x_b, z_b, L_S29, pkg_b, static_b, V_b^pre, A response, fetch_b)_real
      ≈c
    (x_b, z_b, L_S29, pkg_b, static_b, Sim_b view, A response, fetch_b)_ideal

这里 `static_b` 是腐化方自己的密钥/凭证状态。安全定义要求一个统一 PPT simulator 处理理想功能实际产生的 z_b、L_S29 联合分布；它不另行要求对每一个低概率固定值逐点证明独立 conditional equality。若外层环境按某个有效输入分布抽样，比较整个联合分布并要求归约对所有 PPT 环境成立。S36 H4 声称了更强的逐支持点条件采样，但该结论只涉及 party 前缀 sampler。S38 的 **peer encryption plaintext 等式** 对每个支持点逐点成立；这不把 party simulator 与归约器的权限混合。

成功路径定理的观察对象是应用层 messages/phase/sequence/length、P_b 本方 package、内部本方状态、A 返回的 transaction/status/receipt 和一次 COMMITTED 后 fetch。wall-clock latency、网络调度时间和故障前缀不在本轮 party leakage/security view；A 自己看到的到达顺序/时间只属 L_A，A-only 腐化不在目标集合。

L_S29 沿用 S29/S36/S37 的既有字段，不新增本轮定义：公开参数/协议绑定；获准 shuffle 后 endpoints/位置、pivot 与比较 outcome；仅当前活动轮次公开的 sample seed；既有批准的 masked values、比较次数、通信长度和统一 ABORT。未来 seed 不提前公开；不加入原始 score/rank、selected index、完整 mask、peer package、Beaver 中间 opening 的新类目或内部随机带。L_A 单独包括 A 收到的 metadata-only context、session/transaction 与 package-set/context digest、密文及其摘要/长度、到达顺序/时序、PENDING/COMMITTED/ABORTED、receipt、restart 和 fetch。A 的 L_A 不并入 party 的 L_S29；party 收到的 receipt 是 F_TOPK-ATOMIC/A 服务输出，不以新增泄露字段的名义扩大 L_S29。

## 4. 输出 share 关系和 J_b-ENC-CHALLENGE/v1

### 4.1 逐点 plaintext 等式

理想功能的稳定 mask 为：

    M = M_TOPK(reconstruct_Q20.12(x_0, x_1), N, K)

其中 additive score share 先按源码的模 2^32 语义相加，再解释为 signed Q20.12；按分数降序、同分按原始索引升序，输出原序且恰 K 个 1。

S35 的输出刷新在 `score_to_mask_v3_party.cpp:693-723`：

- P0 请求 N 个随机字节并逐位取低位形成 R，自身 share z_0=R，发送 R XOR m_0；在 ideal-uniform coin 模型下 R 均匀。运行时调用 `RAND_bytes`，由 DRBG/随机带桥接承担其实现差异；
- P1 收到该向量后与自己的原序 mask share m_1 XOR，得到 z_1=R XOR m_0 XOR m_1；
- m_0 XOR m_1 是运行时实际产生的原序 mask；将该 mask 等同于理想 M_TOPK 还需实现正确性前提，有限 E2E 观察不是全域证明。

源码 share-refresh 本身无条件给出 z_0 XOR z_1=m_0 XOR m_1。在 S35 算法正确性前提下，才可连接为：

    z_0 XOR z_1 = M
    z_(1-b) = M XOR z_b

在该正确性前提下，对任一固定输入对及支持上的任一 z_b、L_S29、pre-cipher view、pkg_b、F_T setup，peer plaintext 的 conditional law 都是点质量：

    Pr[Z_(1-b)=w | x_0,x_1,z_b,L_S29,V_b^pre,pkg_b,ctx_A]
      = 1[w = M_TOPK(x_0,x_1) XOR z_b]

归约器从外层输入对计算 M，使用 z_b 得到 challenge plaintext；它不把 M 或 peer share交给 Sim_b。此构造是 O(N log N) 的稳定排序加线性 mask 计算，或用项目同一稳定 oracle 的多项式实现；**对端明文的点条件抽样不需要 rejection sampling**，也不依赖 H4 的无条件边缘分布。F_T 的 Fisher–Yates 置换在 S35 源码中使用 rejection sampling，其严格 PPT 截断及误差单独计入 4.2/4.3。

### 4.2 完整 joint sampler 的可执行定义

`J_b-ENC-CHALLENGE/v1` 是 reduction-only 的显式 joint-execution sampler。它产生相关真实执行供 IND-CPA reduction 使用；不会作为 party simulator 的输入，也不直接调用“返回目标 tuple”的 opaque oracle：

```text
J_b(1^λ, x0, x1, public, pk0, pk1, Sign_T):
  1. 在 proof-only F_T 生成过程开始前按安全参数 λ 初始化彼此独立的 ideal
     uniform-byte tape。对每个 gate g，按源码从一个 fresh uniform byte 取出
     5 个相互独立的 bit a_g,b_g,a0_g,b0_g,c0_g；令
       c_g = a_g AND b_g
       P0_g=(a0_g,b0_g,c0_g)
       P1_g=(a_g XOR a0_g,b_g XOR b0_g,c_g XOR c0_g).
     这是 score_to_mask_v3_gmw.cpp:511-530 的理想独立 tape 生成律。
  2. 按 source 的 Fisher–Yates 顺序生成 Π：每个位置至多尝试 λ 个独立
     32-bit uniform draw，按 next_uniform(i) 拒绝规则接受并映射到 [0,i)；在 λ
     次都拒绝时返回 `⊥_J`。对每个 one-hot
     Π[row,original] 独立采样 share0，令 share1=Π XOR share0。按 S35 的
     package header、packed triples/permutation、材料哈希、domain/version
     签名消息和 context 编码，调用 Sign_T 得到 pkg0、pkg1 与 ctx_A；context
     按源码绑定两把 recipient public-key fingerprints、party/A identities、
     circuit/schedule、两个 package digests 和 package-set digest
     (gmw.cpp:306-338,484-585)。Sign_T 仅是 reduction-only 签名 oracle。
  3. 以 x0、x1、本地生成的 pkg0/pkg1 和独立理想在线 coins 建立两个本地
     party-state。逐项执行 source-defined READY 参数检查。对每个实际 AND gate i，
     P_j 读取 triple share (a_j,b_j,c_j)，发出 d_j=x_j XOR a_j 与 e_j=y_j XOR b_j；
     双方打开 d=d_0 XOR d_1、e=e_0 XOR e_1，并输出
       w_j=c_j XOR (d AND b_j) XOR (e AND a_j) XOR ([j=0] AND d AND e)
     作为本方 AND-share（party.cpp:210-256）。严格按 gate_id 检查/消费并执行
     每个 layer 的 frame/phase 序列；继而按源码顺序执行 forward routing、
     当前活动比较结果/seed 所决定的 Select 边、membership 与 inverse routing。
     逐条记录 P_b 可见的 peer frame/phase/sequence/length 和获准 L_S29。最后
     按 party.cpp:693-723 采样 P0 refresh R，令 z0=R、z1=R XOR m0 XOR m1。
     这是应用层确定状态机上的本地 proof execution，不经过 socket/TLS record
     或故障路径；协议实际 gate/message DAG 必须与固定 S35 代码完全相同。
  4. 在电路 correctness 前提下检查 z0 XOR z1=M_TOPK(x0,x1)。不对不匹配
     样本做条件拒绝或重抽；若前提不成立，J 所称的 ideal-output 接口失效，
     将其记为功能正确性缺口，而非丢弃该样本。
  5. 令 m_real = z_(1-b)，m_dummy = 0^N。由步骤 4，逐点
       m_real = M_TOPK(x0,x1) XOR z_b.
     返回 reduction-only state
       W=(z_b,z_(1-b),L_S29,V_b^pre,pkg_b,ctx_A,pk0,pk1,static_b)
     及 (m_real,m_dummy)。ctx_A 和 peer package/share 留在 reduction/A
     证明状态；从不交给 Sim_b。所有 share plaintext 均编码为 N 个 byte bit。
```

`next_uniform(i)` 在 S35 源码 `gmw.cpp:246-253` 用 32-bit rejection sampling；对本轮 `N≤256`，单次尝试拒绝概率小于 `256/2^32=2^-24`。严格 PPT 版本为每个 Fisher–Yates 位置至多尝试 λ 次，超过即返回 `⊥_J`；最多 255 个位置，故截断概率 `ε_perm ≤ 255·2^(-24λ)`。这不是对 z_b、L_S29 或 view 的条件 rejection sampling。未截断源算法是 expected-PPT 且分布精确；有界算法是 strict-PPT，和精确理想置换的统计距离至多 `ε_perm`。若 IND-CPA 归约在 `⊥_J` 时输出独立随机 bit，则有界游戏两边在该分支完全相同；相对精确理想置换的两边差异上界由三角不等式增加至多 `2ε_perm`。

generation 与 source transition 对 gate count/输入大小多项式；bounded Fisher–Yates 工作 `O(Nλ)`；稳定 Top-K mask 计算为多项式；Sign_T 次数固定为两份 package 加 context。采用精确 expected-PPT 置换时，J 与理想联合生成律同分布；采用 strict-PPT 置换时，J 对该分布的距离至多 `ε_perm`。它允许 reduction 保持本地 joint sample 中 view、package 和 A context 的相关性，但并不构造 H4 simulator 与 context 的耦合。该 proof-only 访问不等于 F_T 对 P_b 的输出，也不授权 party simulator 直接取 peer package/share/context；T 私钥仍不进入归约器。若 F_T/v1 仅被形式化为 opaque role-routed oracle、没有上述可执行生成律，则 J 仍不成立，必须将这一采样能力作为 proof-only 前提明确加入证明游戏。

### 4.3 统一 IND-CPA reduction 和 joint game

对固定 b，定义：

- G0_b：J_b 的真实 source-defined pre-cipher transcript `V_b^pre`、同一 F_T package/context 与本方 envelope `C_b=Enc(pk_b,z_b)`，加上真实 peer envelope `Enc(pk_(1-b),z_(1-b))`，进入同一 F_A-ATOMIC/v1 状态机完成成功 commit；然后从已 COMMITTED 状态执行一次 P_b authenticated fetch。
- G1_b：保持 G0_b 的真实本方 pre-cipher transcript、setup/context、本方 envelope 和服务状态机，将唯一变化设为 peer envelope `Enc(pk_(1-b),0^N)`；对 A 的解析、hash、receipt、COMMITTED 响应和该本方 fetch 均继续运行同一源码定义状态机。

G0_b→G1_b 是真实本方前缀中的密文替换。它**没有**把 `V_b^pre` 换成 `Sim_b`。完整目标还需要 G1_b 与 `Sim_b+F_A-ATOMIC/v1` 所生成视图不可区分；此为第 6 节列明的未闭合步骤。

严格的挑战密钥嵌入：在通常 key-generation 游戏中，IND-CPA challenger 的 `pk*` 放在 peer slot `(1-b)`，归约器本地按同一 keygen 生成 corrupted party 的 `(pk_b,sk_b)`，并把 `pk0,pk1` 一并交给 F_T setup；目标安全命题须采用相同且独立的 key-generation 分布。归约器保留本方 `sk_b`/TLS 静态状态供 D 的 view，完全不取得 `sk_(1-b)`。若部署把两个 recipient public keys 固定为外部常量，不能直接借用标准随机 keygen IND-CPA 游戏；需另有对指定目标公钥、带允许 auxiliary input 的 key-conditioned IND-CPA 假设。S35 package/context 对两把 key 的 fingerprint 绑定与这些 public key 同属 setup，不新增 `L_S29`。

统一归约 `B_b^D` 的可执行顺序为：

```text
B_b^D(pk*):
  1. 令 pk_(1-b)=pk*；本地生成 (pk_b,sk_b)，完成 public setup/key binding；
     在挑战 bit 尚未产生时取得固定 (x0,x1) 或运行输入环境得到该输入对。
  2. 运行 strict-PPT J_b，持有其 reduction-only W。若返回 `⊥_J`，输出
     独立随机 bit 并停止；否则设置 challenge pair
       (m0,m1)=(z_(1-b),0^N)。
  3. 向 IND-CPA challenger 提交 (m0,m1)，取得精确 envelope C*。
     用本方 fresh coins 生成 C_b=Enc(pk_b,z_b)。
  4. 将 C_b 与 C* 按 W 中固定的成功路径到达顺序送入 F_A-ATOMIC/v1；
     将 ctx_A 仅送给 A。按真实服务代码完成 hash/receipt/COMMITTED；
     从已 COMMITTED state 发一次 b 身份 fetch，得到本方 C_b 和 receipt。
  5. 形成 D 所见 party view=(x_b,z_b,public,L_S29,pkg_b,static_b,
     V_b^pre,C_b,A application responses,fetch_b)，运行 D 并返回其 bit。
     不把 W 的 peer package/share/context/T tape 交给 D/Sim_b。
```

`pk*` 与本地 keypair 的嵌入要求是构造条件而非当前部署已证明事实。challenge pair 在 challenge bit 产生前从一次 joint sample 得到；因此普通 IND-CPA theorem 比较完整联合 `(W,C)` 分布，不要求对每个低概率 `(z_b,L_S29,V_b^pre,ctx_A)` 单独取最坏优势再平均。若环境自适应选择输入，B 先运行它取得 `(x0,x1)`，再按同一 J 算法采样，所有计算仍独立于 challenger bit。

J_b 将 (z_(1-b),0^N) 作为 IND-CPA 两条消息，将 challenge ciphertext 路由给 A。对任意 PPT distinguisher D，在给定 key-setup 模型下：

    |Pr[D(G0_b)=1] - Pr[D(G1_b)=1]|
      ≤ Adv^IND-CPA_Env(B_b) + 2·ε_perm

这里 B_b 是一个统一 strict-PPT reduction；对 bounded-J 两个游戏的归约界是 `Adv^IND-CPA_Env(B_b)`，将其分别桥接回精确 ideal Fisher–Yates 游戏时由两个 TV 距离增加至多 `2ε_perm`。挑战消息不是按每个固定明文分别选取的最坏攻击，再对条件分布平均。A 的 context、pkg_b、V_b^pre、L_S29 与 z_(1-b) 同属同一 joint execution；挑战 bit 只选择 envelope 的两条消息。G0/G1 的差异只有 challenger 生成的一个 peer envelope。

对 b=0 与 b=1 分别建立上述归约，不能把两个腐化模型合并。

## 5. 实际 envelope、receipt 和状态机

### 5.1 S35 加密封装源码事实

score_to_mask_v3_party.cpp:370-440 的 envelope：

- plaintext 是本方 N 个 byte 编码的 bit；
- 每份 envelope 产生新的 32-byte AES key 和 12-byte nonce (RAND_bytes)；
- RSA 公钥需与签名 package 中对应 recipient 公钥 SHA-256 指纹匹配；
- AES key 用 RSA-OAEP 包装，OAEP digest 与 MGF1 digest 都是 SHA-256；
- body 用 AES-256-GCM；AAD 顺序为 session || invocation || party || circuit_sha256 || gate_schedule_sha256；GCM tag 为 16 bytes；
- serialized 顺序为 magic S2MENC3\\x01、encrypted-key length(u16)、plaintext size(u32)、nonce[12]、tag[16]、RSA ciphertext、GCM ciphertext。对于固定 key/N，z_(1-b) 与 0^N 两个候选长度一致；GCM body length 都是 N。

将该精确 envelope 的 IND-CPA 作为一个整体假设记为 Adv_Env-IND-CPA。可作的组件分解只能在额外确立相应定理条件后写成：

    Adv_Env-IND-CPA
      ≤ Adv_RSA-OAEP-KEM-KI(指定 OAEP/RSA 模型、密钥生成和多用户界)
       + Adv_AES-256-GCM-IND-CPA(新鲜 key/nonce、固定 AAD、单信封用法)
       + ε_nonce/key-collision

一个可复核的组件混合应分两步而不是只罗列算法名称：H0 使用 RSA-OAEP 包装出的 K_real 作为 GCM key；先以精确 OAEP-KEM key-indistinguishability game 将 body key 换成独立均匀 K while 保留相同 OAEP ciphertext/AAD/nonces，代价记 Adv_OAEP-KEM-KI；再在均匀 K 下，以新鲜 key/nonce、相同 AAD 的 GCM-IND-CPA 游戏替换 N-byte plaintext，代价记 Adv_GCM-IND-CPA。该分解只有在所选 RSA-OAEP/KEM 模型、key generation、multi-user/query 范围及 GCM nonce regime 被正式写明时成立；也可不分解而直接把 exact envelope 的 IND-CPA 作为符号假设。本轮没有为 OAEP-KEM 的模型条件、GCM 优势或碰撞项给出数值；不能仅因组件名是 RSA-OAEP/AES-GCM 就声称 envelope 已获证明。GCM tag 的完整性不替代本任务的机密性归约；本成功路径是诚实 A 服务和诚实提交，不覆盖恶意密文 oracle。

### 5.2 A 的 receipt/fetch 后处理

实际 `score_to_mask_v3_escrow.cpp:152-209` 的 `parse_request` 检查请求身份、版本、transaction、固定字段、密文长度与 envelope 头；它不会解密或验证 AES-GCM 明文。`submit_ciphertext` 再按 context 检查 session/invocation、party/package/circuit/schedule 绑定及长度，记录密文摘要。实际 `score_to_mask_v3_escrow.cpp:245-266` receipt SHA-256 的有序输入为：

    domain("score-to-mask-v3/escrow/commit/1")
    || transaction
    || session[16] || invocation[16]
    || BE32(N) || BE32(K)
    || circuit_sha256[32] || gate_schedule_sha256[32]
    || context_sha256[32] || package_set_sha256[32]
    || SHA256(C0)[32] || SHA256(C1)[32]

`transaction` 为固定格式 `s2m-v3-<session-hex>-<invocation-hex>`（72 ASCII bytes），按原字节直接拼入该 preimage；C0/C1 按 party index 0/1 排序。`context_sha256` 是已验证 signed context 文件的字节摘要。源码没有把 context 或 package-set digest 作为新增独立 party 字段发送；它们只经这个 receipt 的确定性哈希关系影响 party 响应。

第一次 submit 在一个 `BEGIN IMMEDIATE` SQL 事务中建立 state=0 transaction row 并插入第一份 recipient ciphertext，然后提交该事务；该 share 此时是 PENDING，party submit handler 等待 COMMITTED 或超时，不返回 pending receipt。第二次 submit 使用另一个事务；它插入第二份 share、读取按 party 排序的两份 ciphertext hash、计算 receipt、把 state 更新到 COMMITTED，再在同一个该次 SQL transaction 内 `COMMIT` (:299-300, :359-422)。因此“两份 ciphertext 已存在、receipt 已写入、state=COMMITTED”在第二次 deposit 的事务提交点逻辑可见；第一份 pending deposit 本身发生在更早的事务中。成功 submit handler 返回 status、transaction 和 32-byte receipt (:572-590，response 编码 :460-470)，客户端校验状态/transaction 后以 64 个小写 hex 字符交给 party (:660-689, :834-843)。从已 COMMITTED 状态的 fetch 只取当前 TLS party 身份的 ciphertext，并返回同一 receipt (:430-457, :592-597)；重复 fetch 返回同一 ciphertext/receipt。相同 party 对同一 ciphertext hash 重复 submit 幂等，不同 ciphertext hash 被拒绝 (:330-368)。RSA-2048 key 下 envelope 长度为 `298+N` bytes，submit 应用 payload 为 `512+N` bytes，READY 为 7 bytes，commit response 为 111 bytes，fetch response 为 `409+N` bytes；这些是源码字段长度，不含外层 protocol frame/TLS 记录开销。成功请求/fetch 长度与两个等长明文相同。

在 **G0→G1 peer envelope 替换** 这一个混合中，密文 SHA-256、receipt 字段、SQL state transition、COMMITTED 响应及后续本方 fetch 都是收到挑战 envelope 后由同一 PPT A 状态机执行的后处理。由 IND-CPA 闭包可直接得出 G0/G1 区分优势界；不需要假设 SHA-256 输出均匀，不需要“hash looks random”。receipt hash 的绑定/碰撞安全是另一命题，单独保留 Adv_hash-binding。

这只说明替换挑战密文时后处理不会增加区分优势；不自动证明 S36 H4 party prefix sampler 与 A-only context joint law 相同，见下一节。A 的 package-set/context、ciphertext hash/长度、事务状态、到达次序和时序属于 L_A。本轮基线排除 A 腐化；A-only 加强模型不在此处混入。逻辑 commit 可供两方恢复，不表示两份 API 物理同时返回。SQLite WAL/FULL 不证明掉电耐久。

## 6. 当前完整模拟的首个精确断点

### 6.1 缺失条件分布

密文替换后，下一步要把真实 party 前缀换为 S36 H4 sampler。令 `A_b^dummy` 表示：F_T 将 `ctx_A` 只交给 A；party simulator 用自己的 `z_b` 和公开 key 生成 `C_b`，并经 A 的真实接口提交本方密文与用公开 peer key 加密的 `0^N`；A 按 S35 服务状态机生成真实响应和 fetch 结果。则需要证明下面这个 **joint distribution** 等式（比较固定输入对下的整体分布；不要求把每个低概率条件取值拆成独立定理）：

    Law(V_b^pre, z_b, L_S29, pkg_b, static_b, C_b,
        receipt_b, COMMITTED_b, fetch_b
        | x_0,x_1, public; F_T/v1, A_b^dummy)
      ?=
    Law(Sim_b^H4(x_b,z_b,public,L_S29,pkg_b), z_b, L_S29,
        pkg_b, static_b, C_b, receipt_b, COMMITTED_b, fetch_b
        | x_0,x_1, public; F_T/v1, A_b^dummy)

上式的 `ctx_A` 在两边均由同一次 F_T setup 产生并且只交 A；它不是 `Sim_b` 的输入。`receipt_b`、`COMMITTED_b` 与 `fetch_b` 是 A 根据 `ctx_A`、本方 envelope、dummy peer envelope 和真实状态机生成后再按 party endpoint 返回的值。party simulator可得到本方 keypair/TLS 状态、public setup、本方签名 package、本方 `z_b`、`L_S29` 和正常服务响应；不得拿 `ctx_A` 内容、peer package/share 或 T 私钥。S36 H4 只给出 `Law(V_b^pre | x_b,z_b,L_S29,pkg_b)` 的 sampler，没有建立它与同一 F_T setup 下隐藏 `ctx_A`、package-set digest 及 A 返回 receipt 的联合关系，因此不足以推出上式。密文 IND-CPA 只闭合 G0→G1；它不补足 G1→该 ideal simulator 分布。

### 6.2 可复核 toy 例与结论边界

抽象 toy setup 足以证明“相同边缘不推出相同联合分布”：

- 令隐藏 peer package bit U←{0,1}；
- 固定 peer input bit x_peer，peer opening 为 q=x_peer XOR U；
- toy A context/receipt 暴露 c=U；
- 真实联合分布总满足 q XOR x_peer=c；
- 若 H4 只给 q 的边缘 sampler，则 q 均匀；party simulator若将其与从 A 单独给出的均匀 c 独立组合，关系测试通过率仅 1/2，而真实为 1。

这不是对 S35 SHA-256 context/receipt 的攻击，也没有声称真实代码以概率 1/2 泄露。它只否定从 H4 边缘 law 推出 context 联合 law 的推理。真实 source 用 SHA-256 对完整 package bytes、context 和 receipt 做摘要；碰撞抗性并不等价于隐藏性/伪随机性。若要依赖随机预言机、摘要隐藏性或高 min-entropy，还必须给出精确游戏、条件熵界、查询/优势项；S35/S36/S37 未提供，本轮不代填。

因此分类为 **证明接口/相关性缺口**：没有已知可复现攻击，不是 runtime bug，也不是不可能性证明。

### 6.3 最小版本化方向

不扩张 L_S29、不把 peer package/share/context 给 party simulator。可选的最小后续修正：

1. 定义并实现可复核的 F_T/v2 + F_A-ATOMIC/v2 proof interface，使模拟器可用其本方签名 package、公开参数和当前获准泄露，按实际材料生成式高效地产生与 A context/receipt **联合相容**的本方 pre-cipher view；接口只把 context 交 A，且必须给出具体采样伪代码和条件分布证明。不得把“返回目标联合分布样本”的 opaque oracle 当成构造。
2. 若实现证明难以闭合，版本化 A receipt 格式改为 opaque random transaction receipt（或使用独立 A-key 的 PRF/MAC receipt），保持服务内部绑定 transaction、两份 ciphertext hash 和 COMMITTED state，但不把 ctx_sha256/package_set_sha256 原样混入 party 可见 digest。随机 receipt 需同一交易跨两 party/fetch 持久一致；PRF/MAC 方案需单列密钥与优势假设。此方案需另作 runtime/功能决策及 E2E，S38 未实施。
3. 不推荐将 ctx_A、package-set hash、peer share 或额外内部位追加到 L_S29；本轮用户边界禁止这样补证明。

## 7. 后续门与范围状态

| 项目 | S38 状态 | 依据 |
|---|---|---|
| S37/S35 固定源码身份 | PASS | S35 原 worktree 85/85 SHA-256 和 canonical 摘要匹配；S37 VFSS/ 无差异。 |
| z_peer=M XOR z_b 挑战明文条件 law | CONDITIONAL_PASS_REVIEWED | reviewer 核对 :693-723；仍以 S35 电路计算冻结稳定 mask 为前提，历史 E2E 不是全域正确性证明。 |
| reduction-only full-joint J_b | CONDITIONAL_PASS_REVIEWED | 非作者确认采样算法作为 reduction-only 子引理有效；需明确理想 F_T 联合生成权限、Sign_T oracle、recipient key setup、功能正确性及 exact-envelope IND-CPA 假设。strict-PPT 置换截断误差 `ε_perm≤255·2^(-24λ)`。 |
| peer envelope G0→G1 IND-CPA 替换 | CONDITIONAL_PASS_REVIEWED | 非作者核对 reduction-only J 与后处理；依赖精确 envelope IND-CPA 与 key setup，两个精确置换游戏相对有界归约另加 `2ε_perm`。哈希/receipt/state/fetch 是该密文替换的后处理。 |
| H4 与 A-only context/receipt 的联合连接 | OPEN_WITH_EXACT_GAP | 上述 conditional law 未在 S36 建立；不能用边缘 law 或 SHA-256 随机外观代替。 |
| party receipt/committed-fetch 完整成功路径模拟 | OPEN_WITH_EXACT_GAP | 因 joint context gap，不判 PASS。 |
| 实际离线生成到 ideal F_T/v1 桥 | NOT_REACHED / OPEN | 下一阶段：真实 triples/permutation shares、签名和 OpenSSL DRBG 的联合分布。 |
| A-only 模型 | OUT_OF_SCOPE_FOR_S38 | 基线中 A 条件受信且排除腐化集合；未定义其 simulator。 |
| crash/abort-prefix 隐私 | OUT_OF_SCOPE_FOR_S37/S38_THEOREM | S37 成功路径边界保留；不得从 no-share 功能合同外推密码学隐私。 |
| runtime defect / code change | NOT_REPRODUCED / NONE | 本轮只读源代码和证明接口。 |
| build/tests | NOT_RUN | 未改 runtime 或 tests。 |
| natural I/O fault | NOT_REPRODUCED | S38 没有执行 I/O；S37/S35 历史记录未冒充本轮复跑。 |
| N≤256 功能范围 | S35_HISTORICAL_FUNCTION_EVIDENCE_ONLY | 本轮未复跑；不作为安全证明。 |
| N>256 | NOT_ADMITTED | 保持原门。 |
| formal LAN/WAN, RTT | NOT_RUN / NOT_MEASURED | 无性能实验。 |
| S38 非作者复核 | CONDITIONAL_PASS_REVIEWED (J 子引理) | reviewer 独立核对 J 的权限、源分布、ε_perm 和 stop point；完整 receipt/fetch 仍 OPEN。 |
| secure alias / merge / push | NOT_CREATED / NOT_DONE | 没有安全通过或合并结论。 |

### 本轮修改文件

- docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S38_REPORT_2026-10-21.md
- docs/decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S38_DECISION_2026-10-21.md
- docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S38_GATES_2026-10-21.json
- docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S38_SOURCE_EVIDENCE_MANIFEST_2026-10-21.json
- docs/IMPLEMENTATION_PLAN.md

无运行时、测试、baseline 或历史 S37 文件改动。

### 非作者复核记录

reviewer：`s38_independent_review`，未参与 S37/S38 推导；只读核查，无改文件、无 build/test。其确认 S35 `next_uniform` rejection、最多 255 个 Fisher–Yates 位置、triple/permutation share law 与 output refresh；认为 J 是**有条件有效的 reduction-only peer-envelope sampler**，条件包括 proof-only F_T tape/signing 权限、兼容 recipient-key 游戏、S35 电路正确性和 envelope IND-CPA。reviewer确认 party simulator 没有取得对端 package/share、`ctx_A` 或 T 私钥；receipt 输入和 commit/fetch 后处理引用源码位置正确。独立意见同意：密文替换只闭合 G0→G1；H4 的 party 前缀 sampler 没有给出与同次隐藏 context/package-set commitment 及 A receipt/fetch 的联合分布等式，所以 G1→Sim_b 仍是首个精确断点。reviewer发现的 manifest 旧字段已在本轮修正。
