# M6A-P2-I：Protocol I + AAV86 预处理与安全设计审查

日期：2026-09-28
状态：**设计材料完成，待独立评审；exact-edge 离线预处理未闭合，secure runtime 不得启动。**
范围：只审查 AAV86 全排序、Protocol I/CA 组合、离线发材、参与方视图、消息依赖、泄露和原顺序 mask 适配。本文不是运行时设计批准，也不扩展 Protocol III。

## 1. 结论摘要

| 状态项 | 本轮结论 | 含义 |
| --- | --- | --- |
| `P2-I DESIGN REVIEW` | **READY_FOR_REVIEW** | 本文把角色、阶段、消息、材料候选、泄露目标和缺口写成了可独立审阅材料；不代表安全结论通过。 |
| `P2-I SECURE RUNTIME` | **NO-GO** | 没有经证明的输入无关 adaptive exact-edge 离线发材机制；隐藏 shuffle、party-view 模拟和最终 mask 逆映射也尚未闭合。 |
| `Protocol III` | **DEFERRED BY USER DECISION** | 先完成 Protocol I+AAV86 的设计与可行性评审；本文不判 Protocol III 的 GO/NO-GO，不讨论其组合方案。Protocol III 仍在 M6A 最终范围内。 |

本轮接受的项目决策为：使用 AAV86 固定轮数随机**完整排序**，之后取前 K 并生成原始输入顺序的 mask；存在可信、离线、全知预处理方 T，T 不接触输入或在线排名，发材后退出且默认不与任一在线方合谋；exact-edge 预处理仍未解决；本轮只先审 Protocol I。T 可以看到全部离线随机量、密钥、置换和相关性。该 T 模型是项目扩展和更强信任假设，不能冒称 Agarwal Theorem 5.1 的原论文参与方安全模型或 `AUTHOR_EXACT`。

**核心发现：** AAV86 后续子问题和图边依赖先前比较所得的 rank/bucket。T 在离线发材时看不到这些在线值，因而不能按未来实际图提前生成“恰好是该图”的逐边材料；若预生成覆盖所有端点对的材料，目标离线发材量退化为至少 `Θ(n²)` 级别（每轮独立 fresh material 时为 `Θ(r n²)`），不再代表 AAV86 稀疏比较边的材料成本。紧凑、可组合且对任意后续 edge/mask-difference 正确绑定的离线构造尚无可核验来源或仓库证明。此结论是**未找到可行构造**，不是“不可能性证明”。

## 2. 基线、工作区与证据标签

### 2.1 本轮起点

本轮开始时静态核对得到：

| 检查 | 值 |
| --- | --- |
| 当前分支 | `feat/m6a-performance-evaluation` |
| `HEAD` | `c3926c68fd14f270faa8b55234311071947fa080` |
| `main` / `origin/main` | 均为 `c3926c68fd14f270faa8b55234311071947fa080` |
| `merge-base(HEAD, main)` | `c3926c68fd14f270faa8b55234311071947fa080` |
| `git status --short` | `M PROJECT.md`；`M docs/PAPERS.sha256`；`M docs/REFERENCE_MANIFEST.md`；未跟踪 P1 决策稿、P0 盘点报告和根目录 `siamjdiscrmath.pdf` |
| 最近主线状态 | PR #27 已合并；M5 G3 接收审查 revision 为 `9b3ce3747b1734602e3edf4c644ae1b6da52e8c1`，不是本轮重跑结果 |

M5 G3 的历史接收结论为 PASS，但它只适用于对应接收 revision 和当时验收范围；`AUTHOR_EXACT = NOT_PROVEN` 等限制保持不变，不外推成 AAV86 或本轮安全证明。上述 P0/P1 改动、未跟踪文档和根目录 PDF 均在本轮前存在。本轮不覆盖、不暂存、不移动它们。`siamjdiscrmath.pdf` 仍是 535 页 SIAM 书目，不是 BB90 正文。`VFSS/`、`VFSS-baseline/`、测试、论文及 ignored 本地参考树不作修改。

### 2.2 证据标签

| 标签 | 用法 |
| --- | --- |
| `PAPER_DIRECT` | AAV86 原文或 Agarwal CCS’24 会议版直接写明的算法、定义、定理或成本。 |
| `PAPER_DERIVED` | 从论文定义直接推出的映射或因果解释；要写明前提，不扩大定理范围。 |
| `PROJECT_CONTRACT` | 本项目已冻结的 score、tie、rank、K 和 mask 语义。 |
| `PROJECT_EXTENSION` | T 的全知信任、Q20.12 输入转换、stable index 编码、输出逆映射等项目额外设计。 |
| `CODE_OBSERVATION` | 当前 tracked VFSS 代码确实提供的接口、字段和消息步骤；不能反向当成论文定义。 |
| `LOCAL_REFERENCE` | ignored 参考树或用户历史材料的行为；不证明来源归属、正确性或安全。 |
| `OPEN / UNRESOLVED` | 当前来源与代码无法证明的性质。 |

### 2.3 依据

- AAV86：Alon、Azar、Vishkin，*Tight Complexity Bounds for Parallel Comparison Sorting*，FOCS 1986，§2、§2.3、§3.1 / Theorem 3.1，printed pp. 505–507（本地 PDF pp. 4–6）；9 页作者托管版。来源：[Princeton 作者托管 PDF](https://web.math.princeton.edu/~nalon/PDFS/Publications2/Tight%20complexity%20bounds%20for%20parallel%20comparison%20sorting.pdf)，[IEEE DOI](https://doi.org/10.1109/SFCS.1986.57)。本机副本与 SHA-256 见 [P0 来源盘点](../reproduction/M6A_BASELINE_AND_SOURCE_INVENTORY_2026-09-27.md)。
- Agarwal 等：*Secure Sorting and Selection via Function Secret Sharing*，ACM CCS’24 **会议版**，§1、§2.4、§4.1、§5.1–§5.4、Theorem 5.1，printed pp. 3024–3035（PDF pp. 2–13）；15 页会议版。来源：[MIT DSpace](https://dspace.mit.edu/entities/publication/d70bd196-cf7c-46e1-a0a9-e3dfd08f379b)，[ACM DOI](https://doi.org/10.1145/3658644.3690359)。本次不把未取得的 full version 内容推定为已知。
- 项目决策和已有边界：[M6A P0 盘点](../reproduction/M6A_BASELINE_AND_SOURCE_INVENTORY_2026-09-27.md)、[M6A P1 决策稿](M6A_AAV86_ALGORITHM_AND_STABLE_ORDER_DECISION_2026-09-27.md)、[M2 泄露审计](M2_PROTOCOL_I_EXACT_LEAKAGE_AUDIT.md)、[M2 三轮设计规格](M2_PROTOCOL_I_PAPER_EXACT_3ROUND_DESIGN.md)、[M2 Chase shuffle 决策](M2_PROTOCOL_I_CHASE_SECRET_SHARED_SHUFFLE_REDESIGN_2026-09-21.md)、[M2 独立评审](../reviews/M2_PROTOCOL_I_3ROUND_INDEPENDENT_REVIEW_2026-09-24.md)、[M5→M6A handoff](../handoffs/M5_PROTOCOL_III_TO_M6A_HANDOFF_2026-09-25.md)。
- 本轮静态查看的代码入口：`VFSS/include/moe_topk/protocol_i_parallel_shuffle.h`、`protocol_i_secret_shared_shuffle.h`、`protocol_i_score_input.h`、`protocol_i_pipeline.h`，以及相应 `VFSS/src/moe_topk/protocol_i_*.cpp`。本轮没有执行任何程序。

## 3. 冻结算法、功能和威胁模型

### 3.1 算法对象与最终功能

`PAPER_DIRECT`：AAV86 §3.1 / Theorem 3.1 给出固定轮数的随机比较**排序**算法。对于固定迭代数 `r`（原文记号为 `k`，与项目 Top-K 参数 K 无关），每层从当前集合随机无放回选择 `t−1` 个 pivot，`t=⌈n^(1/r)⌉`；构造 pivot clique 及非 pivot 与 pivot 之间的比较边，根据比较把非 pivot 分桶，再并行递归完整排序。其比较数满足期望界 `E[e_A(n,r)] ≤ c(r)n^(1+1/r)`。这是期望比较数，不是高概率工作界、网络延迟或实测值。

`PROJECT_CONTRACT + PROJECT_EXTENSION` 的目标功能为：

```text
P0/P1 持有 n 个 signed Q20.12 分数加法份额
  → 对 (score DESC, original_index ASC) 形成稳定全序
  → 在隐藏置换域执行 AAV86 固定轮完整排序
  → 取全序前 K 个成员
  → 安全逆映射到原输入位置
  → 两方输出长度 n 的 XOR 共享 bit-mask；恰有 K 位为 1
```

不把 AAV86 的排序算法改称 Top-K 图；K 只影响最终成员提取和 mask，不使该排序图成为 K-sensitive。Theorem 5.1 的通用 CA `select` 结论不是 AAV86 的 Top-K mask 算法，也不替代上面已确认的路线。

### 3.2 参与方、安全目标及边界

在线双方为 P0、P1；T 是独立的可信离线预处理方。

| 属性 | 当前项目决策 / 证据 |
| --- | --- |
| T 信任 | `PROJECT_EXTENSION`：T 可见完整预处理随机性、双方所有相关密钥/材料、置换及相关性；T 不接触输入和在线排名，发材后退出，不参与在线消息；默认不与 P0 或 P1 合谋。 |
| 在线腐化目标 | 目标审查至少分别考虑单独半诚实 P0、单独半诚实 P1；两者不能看对方输入份额或本地密钥。具体在线 adversary 定义及恶意安全尚未冻结。 |
| T 腐化/合谋 | T 完全可信且不合谋是模型前提；T 与 P0/P1 合谋、T 事后泄露、T 状态恢复/擦除保证均不在当前承诺内。对这些情形**不宣称安全**。擦除和留存策略仍 `OPEN`。 |
| 论文比较 | Agarwal Theorem 5.1 的会议版叙述是 `(2+1)` 角色的半诚实安全，dealer 在线静默；§2.4 的 mask `r` 不应被任一单方（包括论文中的第三方角色）知晓。项目 T 明确知道完整材料，故不能直接继承该单方腐化声明，亦不能称 `AUTHOR_EXACT`。 |
| 中止/篡改 | 恶意偏离、拒绝服务、错误包篡改的安全性未证明。运行时若未来实现仍需 fail-closed 与完整性/认证策略；本报告不设计实现。 |

区分“可信 T 看见所有预处理内容”和“任何在线方看见完整 mask/permutation”是必要的：用户决策只赋予 T 全知权，不授权 T 获取在线 transcript 或在发材后协助恢复排名。

## 4. 角色视图和目标模拟条件

### 4.1 参与方视图表

| 角色 | 离线视图 | 在线视图 / 消息 | 结束状态与当前证明状态 |
| --- | --- | --- | --- |
| T（可信离线） | 配置 `(n,K,r,domain,session)`；AAV86 采样随机量；初始置换及 pivot 随机性；为全部候选边生成的掩码、mask differences、FSS/DCF/uCMP keys；P0/P1 各自完整 package；按用户决策可见这些量的明文。 | 按模型**不接收**输入、masked opens、local ranks、bucket/rank transcript、图更新或 output shares；不发在线补件。实际分发是否看到收件确认、错误码、发送大小、网络元数据，及是否记录/擦除 package，尚无 transport 设计，标 `OPEN`。 | 发材完成后退出；在线静默是用户约束，不是已有实现证据。若未来从 ranks 定制材料、根据在线状态补件或接收 transcript，即违反本目标边界。 |
| P0（在线） | 自己的 Q20.12 加法份额；T 交付的 P0 专属一次性材料；公共参数。目标 package 不含 P1 package，也不应单独给出能恢复 P1 秘密的量。当前未来 AAV86 package 尚无结构。 | 与 P1 交换协议规定的本地消息；按论文 CA 构造打开 masked values `x'_v=x_v+r_v`；执行自身 FSS/CA 求值；若采用 shuffled local-rank reveal，则收到/发送并可见公开 rank 与 shuffled slot 标签；按可见 rank 更新 bucket/下一图；输出自己的 mask share。 | 可以保留自己输入份额、自己 package、transcript 和输出份额。允许泄露集合（形状、K、图/bucket 信息等）必须由安全证明明示，目前多数项 `OPEN`。 |
| P1（在线） | 与 P0 对称，持有自己的输入份额及 P1 专属一次性 package。 | 看到相同在线 openings、公开 rank/图更新和 peer transcript；只执行 P1 本地 FSS/CA 求值；不持有 P0 的完整 package。 | 与 P0 对称。不得默认两方会把 rank、原下标、完整排序结果或输出 mask 在安全执行中重构。 |

### 4.2 安全论证草图（尚非证明）

可供独立审查的目标不是“先 shuffle 所以安全”，而是分别定义以下模拟器：

1. **腐化 P0 的模拟目标：**仅给模拟器 P0 自己的输入份额、公共 `(n,K,r,session)`、明确允许泄露和其最终输出份额；模拟 P0 package、公开 masked list、local-rank reveal、图/pivot/bucket transcript、消息及本地 FSS 视图。P1 对称。若协议公开 rank，则模拟器必须依据秘密原始顺序而不是单靠 `n,K` 才能生成的内容，恰好是必须查明的泄露边界。
2. **masked opening 子目标：**对每个节点/轮，若 `r_v` 是对在线单方未知的 fresh uniform mask，则 `x_v+r_v` 可望与未掩分数独立；但还要证明该 `r_v` 与边 FSS key 的相关性、mask reuse/重排和所有公开索引不破坏该性质。T 知道 `r_v` 不等于 P0/P1 知道它；T 若看到 `x'_v` 则可去掩，因此模型必须保证 T 不见在线 transcript。
3. **rank reveal 子目标：**全序 `rank` 与输入值相关。只有在原始元素到公开 shuffled slot 的复合置换对被腐化在线方隐藏，且其视图中的本地置换、package、masked opens、图标签联合分布可由模拟器生成时，公开 shuffled-domain rank 才可能不增加原位置泄露。需要证明 stable `original_index` 未作为公开 tag 暴露，且已公开的 rank array 与任何输出/消息长度/桶大小的组合不泄露额外次序。
4. **T 的模拟目标：**T 的视图由公共参数、其采样的全部离线随机性/密钥/置换及发送元数据组成；因为按模型不见输入或 transcript，且所有离线选择应在看输入前完成，T 单独视图应可与输入独立。此性质依赖离线材料/发送量不能编码任何在线自适应图信息。T 与在线 party 合谋不在保证范围内。

以上是需补出的 proof obligations，不是已完成的 simulation proof。特别是公开 rank 对原输入索引的脱钩、全知 T 的信任边界、图尺寸和流量泄露尚未闭合。

## 5. Protocol I+AAV86 的目标阶段表

| 阶段 | 功能/数据 | 公开值与坐标系 | 材料及下一阶段依赖 | 轮数/当前状态 |
| --- | --- | --- | --- | --- |
| O：T 离线生成并分发 | 在输入到达前生成初始隐藏置换、AAV86 随机种子/pivot 计划（如允许预采样）、节点 masks、边用 uCMP/DCF correlated material、会话/参数绑定；只向 P0/P1 发各自 package。 | 仅公开运行 shape/config；不能包含输入或 input-dependent rank。T 看到全部生成内容及双方 package。 | 第一轮固定图可按初始节点标签准备；未来图是否能预先覆盖是本 P2 blocker。T 退出且不得补发。 | 离线，不计在线轮；真实 generation、分发和材料容量均未测。 |
| I：raw Q20.12 输入适配 | 输入 P0/P1 各自 32-bit raw score 加法份额；映射成 signed Q20.12 稳定 priority-key shares；original index 作为记录语义的一部分。 | 不公开 score、carry、sign、完整 key 或 original index。 | 当前 `protocol_i_raw_score_input_party` 两轮 carry/sign adapter 产生 padded priority-key shares；它的本地 slot 编码在进入 shuffle 前表达 tie 次序。是否直接适配 CA 所用域/位宽未证。 | 现有窄 adapter 为 2 轮；AAV86 适配可复用性 `OPEN`。 |
| S：初始隐藏 shuffle 与 mask correlation | 同一隐藏复合置换 `π` 作用到比较 key 和绑定 payload/original-index 的秘密记录；形成论文需要的 shuffled shares 和公开 masked list `y=π(x)+r`，且相同 `r` 绑定 CA FSS。 | `y` 对 P0/P1 公开；不公开 `π`、`r`、原位置与索引。 | 需要证明同一 π、mask、record 和 FSS key 的 correlation，及任一在线方看不出复合置换。T 知 π/r/key；按模型不在线见 y。 | Theorem 5.1 总轮数含其论文 shuffle/初始化边界；具体项目接口映射尚未证明。当前两遍 shuffle没有 public `y`；三轮 C-INSTANTIATION 不能满足此安全结论（见 §7）。 |
| CA-1：首层 AAV86 图 | 从随机 pivot 及当前全体节点构造 pivot clique 和 pivot-to-nonpivot 比较边。若初始 shuffled node IDs 固定且随机选择可离线确定，第一层 endpoints 可能输入无关。 | 打开 masked key/value（位置应是 shuffled slot）；公开图及 pivot labels 是否公开、labels 是何种坐标尚须冻结。 | 对每一有向/无向逻辑边生成与该端点对 `r_u-r_v`、比较方向、参数域相符的 party FSS material。 | 按 §5.4，高层 masked open 是一轮依赖，FSS local eval 本地进行；与 graph/material 绑定未有目标实现。 |
| CA-i：local-rank 与图更新，`i>1` | 从前轮打开的 local ranks 确定各桶/递归子问题；按算法随机选择该子问题的 pivots 并生成下一轮比较图。 | 公开 ranks 位于 shuffled node IDs，bucket 和边标签可能成为公开值；bucket size/空桶等由这些 ranks 导出。 | 当前准确子问题节点/边要等前轮 rank 打开后才知道。T 已退出，故不能为这些 exact edges 在线造 key。复用所有可能 endpoint pair 的材料会进入 §6 全候选图对照。 | 需要 masked-open、FSS/rank share、rank reveal 的真实因果次序；单轮 CA 的图更新若完全由双方已有公开 transcript 确定可本地做，否则同步/确认需另计。定理总界 `2r+1` 不消除该依赖。 |
| SORT：最后一层输出 | 安全地获得完整排序结果 shares 或等价的 shuffled membership shares；不公开全排序/原位置映射。 | full order、rank permutation 等公开到何种程度不能默认；功能输出是排序结果 shares 而非 Top-K mask。 | 论文 CA sort result 与项目 Top-K mask 不同；要先从 rank `<K` 形成 membership，再保留秘密 original-index 关系。 | 在 CA 定理功能内的部分受论文轮数覆盖；项目 membership/output 处理 `OPEN`。 |
| INV：逆映射 | 把 shuffled-order 的 K 个成员位安全映射回原始 logical input slots。 | 不公开 inverse π、selected original index、映射表或原顺序 mask。 | 需与初始 π、record 和输出标签使用同一绑定；材料应在 O 阶段离线备齐并 one-shot。 | 当前固定 pipeline 的 reverse shuffle 是独立项目适配（2 轮）；不能直接假定它适配 theorem 的 hidden shuffle，也不能假定无额外轮。目标轮数 `OPEN`。 |
| MASK：位表示与完成 | 两方返回长度 n 的 XOR mask shares；逻辑 0/1 且总共有 K 个 1。 | 安全执行不得重构最终 mask、oracle 或明文 selected index。 | 若 membership 已是模 2 加法 share，提 LSB 可逐方本地映为 XOR share；若来自更大环/域，必须设计并计入 share conversion。 | 无法从论文 `F_sort` 直接推出本项目 mask 转换为 0 轮；依赖具体输出表示，当前 `OPEN`。 |

### 5.1 逐轮图依赖与 exact-edge 时点

`PAPER_DIRECT`：Agarwal §5.3 的 Algorithm 1/2 中，第一层 pivot 选择独立于 score 值，但 pivot 对应哪个 shuffled record 取决于初始置换；其后各层只在前层 rank 导出的桶内递归。下一层的候选节点集因而依赖在线比较/local-rank。随机选择未来 pivot 的随机 coins 可以提前抽取，不会让未来 bucket 内的**具体 record endpoints**变成输入无关。

`PAPER_DERIVED`：第一层 exact edge set 至多在初始置换及可见节点标识已固定后才能确切命名；第 `i+1` 层 exact endpoints 要在第 `i` 层的 rank/bucket 已产生并按算法可用之后才确定。若公开 rank 后双方分别按相同公开 transcript 本地算图，可能不需新增“图同步”消息；但每轮材料已需匹配这些端点，T 不能在离线阶段等待这一步。

`CODE_OBSERVATION`：跟踪 VFSS 中没有 AAV86 graph runtime、递归 bucket 状态或逐轮图 package API。已实现的 edge package 均为按固定 edge list 生成/校验；它不能证明自适应 edge material 可离线完成。

## 6. 输入无关离线 exact-edge 候选审查

每条边的目标材料至少要和逻辑端点 `(u,v)`、比较方向/稳定 tie 规则、双方 mask difference（例如与 gate 使用的 `r_u-r_v` 对应）、CA iteration、父 transcript/graph fingerprint、party、session、域/位宽及 one-shot sequence 绑定。实际 uCMP/DCF 表示由安全设计确定；本报告不发明 key encoding。错端点、错轮、错会话、缺包、重复消费或失败重试不得让材料被重用。

| 候选 | T 预先知道什么 / 双方收到什么 | exact graph、在线依赖与消息 | binding、one-shot 与安全/轮数依据 | 数量级与判定 |
| --- | --- | --- | --- | --- |
| A. 只预生成第一层 exact graph 材料 | T 可知道初始置换、随机 pivot slots、首层所有端点及全部 masks；双方各收到首层每边的 party key、node mask share 和其余本轮初始 shuffle 材料。 | 首层若 pivot slots 与 shuffled IDs 输入无关则 exact edges 离线确定；T 无需等输入。第二层开始依赖前层 rank 得到的 buckets，材料在后续层缺失；不能在线问 T 或补发。 | 首层材料可按全绑定字段发放且一次性使用，但尚无首层整体安全证明，也不能推出后续边正确性。纯首层不增加在线轮；它不能闭合完整排序。 | 首层边数为 pivot clique 加 pivot/non-pivot 边，量级约 `O(n·t)`；仅部分预处理，**不满足完整目标**。可作为研究步骤，runtime 仍 NO-GO。 |
| B. 为每一轮每一候选端点对准备完整图材料（完整图控制） | T 对固定 n、轮数、vertex namespace 与每轮 masks 生成全部 `n(n−1)/2` 对应的两方密钥；双方各收全量材料和 edge-ID 索引。T 不见输入或之后 active graph。 | 实际 active graph 在在线 rank 后确定，双方本地取用其子集；T 不等、不发消息。图选择仍可自适应，但预处理不再稀疏。 | 需证明每对 key 与对应 mask difference 匹配，未使用 key 的存在和选择不泄露额外信息；每轮/会话 keys 必须隔离，未使用项也不能跨 run 重用。离线全量分发自身不增加在线 round；AAV86 正确性只来自算法及每条边 gate，而不是该预留策略。 | fresh per-round packages 为 `Θ(r n²)` 条 edge material（不含 key bytes 常数/深度）；即使在线只评 active `e_A`，离线时间/材料已不是 AAV86 `e_A` 级。若实际 eval 全边，在线计算也为 `Θ(r n²)`。**仅完整图对照/控制，不是目标通过。** |
| C. 为全部可能未来图形逐图预生成 | T 可预先列举不同 pivot/bucket graph，并生成图专属 edge packages。双方收到整个图族或其指针。 | 在线 ranks 选出其中一个图；T 不在线参与。但下一图属于哪个家族成员直到 ranks 后才知。 | 每份仍须按 graph/transcript/endpoints/mask/round 绑定；若共享边 key，必须证明跨图/轮复用不泄露并与 fresh-mask 语义相容；材料选择可见性也需模拟。无此证明不能算可组合 key。 | 显式图族最坏数量随可能分桶状态指数增长；若按 edge 原子复用，其压缩终点是候选端点全图 B。未发现能避免此成本的已有构造，**研究候选，未解决**。 |
| D. 通用/可编程 FSS 或紧凑相关性种子 | T 生成紧凑 root/通用相关材料，双方持有各自份额；运行期根据公开 shuffled vertex IDs、轮次/图 digest 和既有 mask share 本地导出准确 edge key。 | 图在前轮公开 rank 后确定；无需 T 等待或在线消息。此是唯一可能同时保持 offline-only 与按实际边发材成本的概念方向。 | 必须给出 key-generation 分布等价性/功能证明：任意被选择 endpoint 的 DCF/uCMP key 对应恰当 mask difference；party-local 导出不可恢复对方秘密；选择性打开、图 transcript binding、伪随机扩展、失败和 one-shot 全部可证明。 | 仓库未发现该构造；Agarwal 会议版离线成本按 `e_A` 记 key 数，却未在可见内容中给出面向后续自适应 exact-edge 的生成/绑定算法。**没有证据不得实现；最重要的只读研究方向。** |
| E. P0/P1 各自本地 PRG 派生 | 双方各拿独立 seed 和 party-local root。 | 任意 edge 可本地计算，不用 T 在线等。 | 不能仅靠各自 PRG 证明两边 key 是同一个 DCF/uCMP 功能的正确 key pair，尤其参数含秘密 mask difference；尚无共享相关性的生成证明，也无安全/one-shot 证明。 | 看似紧凑但目前是 `OPEN` 设想，不能进入 target。 |
| F. 前一轮后由 T 按 rank 补发 key | T 等待上一轮 ranks/active graph 后为 exact edges 生成并发给双方。 | T 必须在线收到 ranks/图并追加在线通信。 | 改变 T 的用户确认边界、暴露 rank/图给 T，增加因果 round/在线 Dealer；不符合离线静默目标。 | 只能作为明确不同威胁模型的诊断对照，**排除出目标**。 |

因此：`ADAPTIVE_EXACT_EDGE_PREPROCESSING = UNRESOLVED / NO-GO FOR SECURE RUNTIME`。现有证据不支持“不可能”结论；继续研究 D 类 compact composable FSS/相关性构造、并寻求作者 full version/正式 artifact 说明。不得用 B 完整图预留或 F 在线补发伪装成目标解。

## 7. 当前 Protocol I 实现路径之间的边界

| 当前对象 | 可核验实现行为 | 不应推定的性质 |
| --- | --- | --- |
| 两遍 `protocol_i_shuffle_forward_party`（`VFSS/include/moe_topk/protocol_i_secret_shared_shuffle.h`、`VFSS/src/moe_topk/protocol_i_secret_shared_shuffle.cpp`） | Chase OPV/Share Translation/Permute+Share 组成：两次先后在线调用，共 2 轮，返回共享的 `π(x)`；每方持有自己的 permutation/inverse 和本地 one-shot PS material。来源记录将其限定为两方 Chase shuffle 的项目 C1 conformance 组件。 | 不输出论文 `public y=π(x)+r`；没有同时绑定的 GRank mask；reverse carrier 是另行 C-INSTANTIATION；接口存在不证明它满足 Agarwal `(2+1)` 视图。 |
| 两遍 reverse carrier | 使用另外两份材料与 fresh reverse stages 把 shuffled carrier 映回输入位置，2 轮。 | 它不是初始 hidden shuffle 的同义词，也不自动适用于 AAV86 排序结果或证明 inverse mapping 不泄露。 |
| `protocol_i_parallel_shuffle_three_round_party`（`VFSS/include/moe_topk/protocol_i_parallel_shuffle.h`） | 输入 party config/material、`ProtocolIBlock192` shares、3 个 round fd；输出含 `shuffled_share`、`public_masked_records`、`public_ranks`、`sorted_share`。R1 两方准备置换 share；R2 形成公开 masked record；本地对其做 fixed-clique CmpAgg；R3 交换 rank shares 并双方获得公开 ranks。 | Header 明确为独立 `C-INSTANTIATION`。Dealer factory 当前直接采样复合置换构件、双方 permutation/correlation、`r0/r1` 并在本地形成 `full_r=r0+r1` 后生成 edge keys；T/生成者因此看到 full r 和完整构件，不满足 Agarwal §2.4 “任一单方未知 r”的直接安全要求。edge 数固定为 `n(n−1)/2`。三轮功能不等于 CA 自适应 AAV86。 |
| `protocol_i_priority_pipeline_party`（`VFSS/include/moe_topk/protocol_i_pipeline.h`） | 输入 padded priority-key shares、party package、shuffle material、各阶段 fd；返回 `xor_mask_share` 和 metrics。前向两轮 shuffle；R3 顺序打开固定 padded key shares 加 node masks；本地计算全对全 CmpAgg；R4 公开 rank；逆向两轮 shuffle carrier；总 6 轮。package 校验固定 `padded_n(padded_n−1)/2` edge materials，随后基于公开 rank 产生 rank `<K` 的 carrier。`protocol_i_raw_score_input_party`（`protocol_i_score_input.h`）另收 `logical_n` 个 raw Q20.12 shares 和 2 个 fd，输出 `padded_n` 个 `uint64_t` priority-key shares，另加 2 轮，总入口 8 轮。 | 是 fixed-clique 工程基线，不包含 AAV86 pivots/buckets/recursion；公开的是其当前 shuffled-domain rank 结果。不能把 8 轮或 3 轮 core 重新标成 AAV86，也不能认为这些结构证明了安全 local-rank reveal。 |

这些路径共享部分原语，但 shuffle 输出、置换知识、输入表示、公开值、图形和输出均不同。函数名里有 `shuffle` 或 `parallel` 不等于论文的 hidden shuffle/local-rank reveal 组合。

## 8. 公开值、关联泄露和稳定记录审查

### 8.1 公开值逐项

| 值 | 预期接收方 / 坐标 | 潜在信息 | 当前状态 |
| --- | --- | --- | --- |
| `x'_v = x_v+r_v` | CA 两在线方；shuffled vertex/slot 坐标 | fresh uniform `r_v` 且 mask/key 相关性正确时，单值可独立于 `x_v`；重复使用、mask share 泄露、T 看见 transcript 会破坏简单论证。 | `PAPER_DIRECT` 说明 open 与 offline r/FSS；本项目实际 mask/FSS 生成与复用未闭合。 |
| local-rank shares | 各方本地 CA/FSS 输出 | 单份 share 应隐藏 rank；不能由代码的 additive vector 类型直接推出。 | 论文概念存在；动态图 package 及 shares 无当前接口。 |
| 重构 local rank | P0/P1；rank 数组下标必须是 shuffled vertex ID | 暴露每个公开槽的局部次序和 bucket；若 π 对单方安全隐藏，原索引泄露可受限，但仍关联 score 次序及公开输出。 | Agarwal §5.4 允许在初始 shuffle 前提下 reveal；当前具体 I shuffle 与此 shuffle 的等价性、slot 标签证明、图 metadata 额外泄露均 `OPEN`。 |
| pivot、bucket、graph、edge/node count | 若两方据 rank 本地更新，则当前两方可见 | 图本身/节点集合可以泄露局部 rank 的函数、桶大小、空桶结构、数据依赖的控制流或流量。 | 可见性需随 round contract 冻结；不宣称只有 rank vector 造成泄露。 |
| original index / 复合 π / 原序 mask / selected index | secure transcript 中不应公开 | 一旦公开，可能直接揭示输入位置及选择结果。 | `PROJECT_CONTRACT` 禁止公开；未来动态路径需通过记录与接口证明保持隐藏。 |

T 能看到完整 `π` 与 `r`。若 T 另外看到 `y`，便能去掩；若 T 看到 ranks 并知道 π，便能把顺序映回原输入身份。因此“不接触在线 opens/ranks”是 T 模型的安全必要条件，而不仅是效率要求。T 与任一在线方合谋则至少会把完整置换和离线 key 交给该方，可能将 shuffled-domain rank 与原位置关联；该情形明确排除，不作合谋安全承诺。

### 8.2 完整稳定记录契约

每个逻辑记录在比较、shuffle、分桶、递归和最终逆映射期间必须保持以下字段的秘密绑定：

```text
(Q20.12 signed score share,
 original_index share or injective stable-priority component,
 logical/padding status,
 current shuffled vertex handle,
 any payload needed by the final mask adapter)
```

`PROJECT_CONTRACT`：`raw` 是 32-bit 二补码 signed Q20.12；顺序为 score 降序、原始下标升序；项目 rank 0 表示最高优先级。当前清晰语义映射 `raw XOR 0x80000000` 把 signed 数值序映为 unsigned 序，已有 priority-key helper 再把 score 方向和 index tie 编入比较键。这是项目编码，不是 AAV86/Agarwal 编码。

`CODE_OBSERVATION`：当前 `protocol_i_raw_score_input_party` 对原始槽位构造 padded priority-key shares，低位 tie 分量使用输入 slot；它在隐藏 shuffle 之前可以表达原始 index 次序。目标仍须证明比较 payload 与该 key 同一置换、每轮 graph node handle 不替代 original index、selected shuffled slot 经逆映射后恰对应输入位置。不能只排序 score 后用 shuffle 后 slot 打破 tie。

位宽/代数：目前 score adapter 在 comparison ring 里做窄 Q20.12 转换；priority comparison width 与 index bits 相关，固定 pipeline 限制最大 53 bits。Theorem 5.1 的域要求（`G=Z_(L')`、`L'≥2L`）和 FSS key 位宽/非零性质须逐项映射，不能由当前 ring helper 的测试或接口推定满足。

### 8.3 边界用例必须在未来规格中明示

| 情形 | 必须保持的功能 | 当前未闭合点 |
| --- | --- | --- |
| `n=1` | 唯一逻辑输入被选中（合法 K=1），mask 长度 1。 | 项目算法需定义 singleton 的递归终止/空图；当前 I layout 会 pad 到 2，不能让 padding 进入 pivot/排序而改变语义。 |
| `K=1` | 仍走已确认的完整排序路线，唯一最高稳定优先项置 1。 | K 不改变 AAV86 全排序图；输出位与逆映射待闭合。 |
| `K=n` | 所有逻辑输入选中，pad 不选。 | 可否做合法的功能等价快捷输出须单独证明/批准；不在本轮冻结优化。 |
| 重复 score / 全相等 | 以较小 `original_index` 先，精确恰 K 位。 | tie 分量须穿 shuffle 和每个 bucket；rank 数组索引不得用作 tie index。 |
| 非二次幂 n | 输出只覆盖 logical n 项，恰 K 位。 | 优先把 padding 节点排除在 AAV86 graph；若保留 padding，需证明 sentinel 比任何合法优先级低、不会被选作 pivot/影响 bucket/local rank，也绝不进入输出。 |
| 正负与 INT32 边界 | 按 signed Q20.12 解释并保持稳定全序。 | raw→ordered mapping、拓宽、模环、mask difference 与 DCF 语义需给无溢出/无环绕证明。 |
| 空递归子问题 / 大小 1 | 算法安全终止，不产生非法 pivot 或材料索引。 | CA 图 builder、预处理材料消费和 round/frame 形状需要定义 empty/no-op 规则；不能在线异常后补发 key。 |

## 9. 因果轮数和计量边界

### 9.1 论文核心的条件性因果图

`PAPER_DIRECT`：Agarwal Theorem 5.1 报告 CA `sort/select` 对应安全分享的在线轮数为 `2r+1`。§5.4 给出 masked input open、local-rank FSS 求值，以及“先 hidden shuffle 时可再 reveal local ranks 并据此更新”的构造说明。按该高层依赖可画出**条件性**轮次：

```text
论文 shuffle / 初始化的额外因果层 (+1)
  → 对每一 CA iteration i：
       打开本轮 masked input x'+r                 (1 round)
       本地 FSS 求 local-rank shares              (0 round)
       打开/揭示本轮 shuffled-domain local ranks  (1 round)
       两方本地据 rank/pivots 生成下一层图       (0 round，若无未计同步消息)
  → 共 1 + 2r = 2r+1（论文聚合边界）
```

这只是会议版 theorem 的高层 causal reading。它不提供项目 wire transcript、T 全知视图、未来 exact-edge key 生成方式或原顺序 mask adapter。任何多一条依赖上一轮 rank 的同步/确认、独立 masked-key exchange 或 output routing 都需重画 DAG 并新增相应轮；不能因代码调用合并而删轮。

### 9.2 项目端到端账本

| 阶段 | 轮数口径 | 本轮结果 |
| --- | --- | --- |
| Q20.12 score → stable key shares | 当前窄 adapter 2 轮 | 若复用则先按 2 轮计；能否与 theorem 输入边界融合未证。 |
| hidden shuffle + CA sort | 论文 theorem 聚合为 `2r+1` | 只有 exact theorem construction 和相同模型/功能时可引用；当前 I shuffle 不等价证明缺失。 |
| rank/bucket/graph update | 高层可在 reveal 后本地算；不同步时 0 轮 | 图一致性、labels、size/termination 的 message contract 未实现；若需同步消息必须单加。 |
| 从 full sort 到 shuffled K-mask | 输出功能/本地操作/安全选择 | 需要基于排序 rank 建 membership shares；输出表示及 mask share 变换轮数未定。 |
| inverse map 到 original-order | 当前固定 shuffle reverse adapter 是 2 轮项目扩展 | 不能直接套给 CA theorem 的 π；新路线所需材料和实际轮数 `OPEN`。 |
| bit-mask share output | 若输入已是 mod-2 shares 可本地表示；其他表示可能要安全转换 | 具体 output field/ring 未确定，不能先填 0。 |

在“score adapter 完全独立、CA theorem core 可用、inverse adapter 另行采用且确为两轮、bit 转换无需通信、没有阶段重叠”的假设下，账面表达式会是 `2+(2r+1)+2 = 2r+5`。**这不是当前实现轮数、不是已证明的上界或承诺**：逆向 adapter 的 permutation/binding 尚未接到 CA hidden shuffle，输入输出材料也未闭合。当前可审计结论是论文 CA core 条件式 `2r+1`；项目 raw-score→原序 mask 总轮数 `NOT_PROVEN / NOT_MEASURED`。历史固定 clique 总 8 轮和独立三轮 C-INSTANTIATION 均不能替代此总账。

### 9.3 理论成本与实测状态

- `PAPER_DIRECT`：AAV86 全排序期望比较数 `E[e_A(n,r)]≤c(r)n^(1+1/r)`；比较图 node/edge 随随机采样而变化。该结论不是每次运行边数，也不是通信延迟或高概率成本保证。
- `PAPER_DIRECT`：Theorem 5.1 给 CA key/computation/communication 公式（成本依赖 `e_A,v_A,n,r,L',p`），其离线项按实际图的 `e_A` 记 FSS keys。会议版没有给本项目所需 adaptive exact-edge key factory，不能只凭公式认定可实现。
- `PROJECT_EXTENSION`：raw input adapter、原下标编码、padding、Top-K membership、逆映射及最终 XOR mask 的时间、通信、材料、轮数都另列，不能计入 theorem core 后又省略。
- 实测：本轮没有 build、test、benchmark 或执行本地参考程序；所有 M5/M2 测试数据只属于各自历史报告。M6A 本轮 runtime 与性能字段保持 `NOT_MEASURED`。

## 10. P2-I 设计门清单

| 门 | 当前状态 | 允许进入 runtime 前必须有的证据 |
| --- | --- | --- |
| 算法身份 | **已由用户确认；来源对应 AAV86 Theorem 3.1 完整排序** | 固定实现参数、随机性和终止规格；不改称 Top-K 原生算法。 |
| T trust model | **已由用户确认的项目扩展** | 在项目安全说明中明确只保证何种在线单方安全；不得继承 dealer 可腐化/合谋的论文保证；明确 retention/erasure 和 distribution metadata。 |
| adaptive exact-edge offline material | **OPEN / NO-GO blocker** | 可复核生成算法、正确 key 分布、endpoint/mask/round/session binding、T 离线视图与 parties 收包、one-shot/replay 规则及正确性/隐私证明；无解时保持 NO-GO。 |
| shuffle + masked list + rank reveal | **OPEN / NO-GO blocker** | 同一 π 对 payload 和 masked-list 的代数证明；任一在线方不能恢复 composed π；local rank reveal 及公开 graph 的模拟证明；T 不接收 transcript。 |
| stable full-order record | **语义方向已定；接口和证明 OPEN** | score/index/padding/status/payload 同记录贯穿 CA；原 index 不公开、不由 shuffled slot 替代；位宽/domain 与 theorem 假设逐一匹配。 |
| Top-K mask/inverse mapping | **OPEN / NO-GO blocker** | full-sort shares→K-membership shares→original-order XOR mask 的 secure 功能证明、所需离线材料、失败规则及因果轮数。 |
| 恶意安全/中止/状态擦除 | **OPEN，当前不宣称** | 若项目需要则另行定 threat model、认证与状态生命周期；不得默认为已有。 |
| Protocol III+AAV86 | **DEFERRED BY USER DECISION** | Protocol I 设计/可行性审查结论形成并关闭后，另立设计审查；不可继承本报告中的 Protocol I 证明。 |

## 11. 未核实证据与后续只读任务

1. 继续查找经作者、作者机构、出版方或正式 artifact 来源确认的 Agarwal full version 或实现说明；目前 P0/P1 仅能确认会议版，会议版明确把若干实现/转换细节引向 full version。没有找到不是公开世界不存在的证明。
2. 查找针对 `(2+1)` 输入无关、在线静默 T 的 **compact composable per-edge FSS/DCF preprocessing** 原始构造或正式证明；须能覆盖 CA 自适应 bucket、mask-difference correlation 与选择性使用。若只有 abstract theorem cost，要求来源给出 key factory/party view。
3. 由安全评审给出 T/P0/P1 的完整腐化集合、允许公开值清单、图/流量/桶尺寸 leakage、状态保留/擦除、分发确认和中止语义。用户确认的“不合谋、无在线 rank/input”边界保持，不重复作为待选方案。
4. 给 Protocol I 初始 shuffle 建立与 CA masked-list 相同 π、r 和 key 的功能/模拟证明，逐 party 对照当前 Chase 两轮、三轮 C-INSTANTIATION 与固定 clique pipeline；任何一条路径未证明前不贴论文等价标签。
5. 单独设计并审查 `full sort → rank<K → inverse map → n-bit XOR shares`；明确 logical/padded vertex namespace、空桶/单元素递归、stable original_index、域宽和原序不泄漏。
6. 把消息 transcript 展开到 sender、receiver、payload、depends-on edge、打开值、round id 和材料消耗；据此重新推导 core 与端到端轮数，保留理论、实测分栏。
7. 上述 Protocol I 设计评审形成明确结论后，才进入用户要求的 Protocol III+AAV86 单独设计审查。Protocol III 仍保留在 M6A 总目标中；此处不提前展开。

### 尚需学姐/团队给定的非既定决策

- 安全目标是否只覆盖半诚实在线单方且将 T 固定为诚实可信、不可合谋方；是否需要恶意/中止安全。
- T 的本地材料保留期限、擦除是否属于安全要求，以及分发 ACK/失败元数据是否进入 T view。
- 公开 graph labels、bucket sizes、active edge counts、消息长度是否全都允许泄露；若不允许，需隐藏控制流及流量模式。
- 如果 full version/正式构造仍不能给出 compact exact-edge 预处理，团队是否只保留完整图作为明确标注的控制实验，还是终止该目标 runtime。此问题影响路线继续性，不改变本文当前 NO-GO。

以上是未来项目模型要冻结的问题。不会回退本文开头已确认的 AAV86 full-sort、T 离线信任、exact-edge 未解决和 Protocol I 优先决策。

## 12. 最终判定与本轮执行记录

- `P2-I DESIGN REVIEW = READY_FOR_REVIEW`：阶段材料已完成，欢迎独立审阅；评审发现问题应追加修订并保留审计记录。
- `P2-I SECURE RUNTIME = NO-GO`：adaptive exact-edge offline material、具体 hidden shuffle/rank reveal simulation、原顺序 mask/inverse mapping 及实际消息 DAG 仍缺核心证明。不得据本文开始 `secure` runtime。
- `Protocol III = DEFERRED BY USER DECISION`：没有在本轮设计、评审或判定其 AAV86 组合；不删除 M6A 最终范围。
- 本轮只静态阅读文档、当前 headers 和 implementations，并新增本设计稿、按用户指定顺序最小更新 `docs/IMPLEMENTATION_PLAN.md`。**没有**执行 build/test/参考程序/benchmark；没有修改运行时代码、测试、`VFSS-baseline/`、论文或本地参考树；没有提交、推送或创建 PR。
- 本轮允许的文档差异仅为本文与实施计划中的 M6A 顺序澄清；此前未提交文件保持原状。
