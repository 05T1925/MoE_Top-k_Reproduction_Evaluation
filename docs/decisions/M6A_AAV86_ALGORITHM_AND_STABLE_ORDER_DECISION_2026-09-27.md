# M6A-P1：AAV86 算法证据、稳定 Top-K 语义与路线边界审查

日期：2026-09-27
状态：**P1 证据审查完成；算法对象已识别，安全组合和 P2 实现仍有显式阻塞项。**
范围：只审论文原文、项目冻结语义、当前接口和本地参考行为；不实现协议、不修改运行时、不替 P2 设计自适应离线发材机制。

## 1. 基线、证据类别与审查结论摘要

### 1.1 本轮起点

本轮开始时的 Git 状态与 P0 交付后的状态一致：

```tex
 M docs/PAPERS.sha256
 M docs/REFERENCE_MANIFEST.md
?? docs/reproduction/M6A_BASELINE_AND_SOURCE_INVENTORY_2026-09-27.md
?? siamjdiscrmath.pdf
```

- 当前分支：`feat/m6a-performance-evaluation`。
- HEAD、`main`、`origin/main` 均为 `c3926c68fd14f270faa8b55234311071947fa080`；HEAD 即当前 `main`，merge-base 也相同。
- 最近五条提交与 P0 报告相同；M5 G3 接收结果已由 PR #27 合入 main。G3 接收审查的对象仍是 `9b3ce3747b1734602e3edf4c644ae1b6da52e8c1`，不是本轮重新运行或重验。
- 根目录 `siamjdiscrmath.pdf` 仍是用户要求保留的 535 页书目 PDF；本轮没有改写、移动或登记为 BB90 正文。被忽略的 `Papers/` 和三个本地参考树也未改动。

### 1.2 证据标签

| 标签 | 本记录中的含义 |
| --- | --- |
| `PAPER_DIRECT` | 会议版或 AAV86 原文页面直接写明的定义、步骤、定理和计量式。 |
| `PAPER_DERIVED` | 对原文定义作直接语义映射，例如把较小值优先的 rank 改写成项目的高分优先 rank；必须说明前提。 |
| `PROJECT_CONTRACT` | 已冻结的 M1 分数、tie、rank、K 和输出 mask 契约；不冒充论文内容。 |
| `LOCAL_REFERENCE` | 本机被忽略的参考目录或用户脚本静态表现；不说明代码来源、正确性或安全性。 |
| `PROJECT_EXTENSION` | 为输出项目所需 Top-K mask 而增加的适配、稳定键绑定或组合路线。 |
| `D-UNRESOLVED` | 当前材料不足以确定，须由算法/安全设计门补证；本阶段不填入实现方案。 |

### 1.3 摘要结论

1. Agarwal CCS’24 中标为 AAV86 的具体示例是 **AAV86 1986 论文 §3.1 / Theorem 3.1 的固定轮数随机比较排序算法**，输出对象为完整排序。Agarwal Algorithm 1 给出 Valiant 比较图写法，Algorithm 2 将同一图步骤改写为 CA local-rank 步骤。AAV86 §2.3 的“处理器数—轮数”上界是另一结果，不应混为同一算法。
2. 原 AAV86 论文讨论 median/selection 的比较复杂度界并引用其他选择算法；本次取得的正文没有一个可直接用于本项目、以任意 K 输出 Top-K mask 的完整 AAV86 selection 图。Agarwal Theorem 5.1 对 CA 中 `sort` 和单项 `select` 给出通用编译结论，但该定理本身不是 K 项成员 mask 算法。
3. 当前唯一有直接 AAV86 排序来源的项目候选是 **路线 A：先完整排序，再取前 K 并安全逆映射成原顺序 mask**。这是有来源的算法对象候选，不等于已冻结安全实现或计量边界。路线 B 需要完整、可定位的 selection/Top-K 算法来源；本次未核实到，保留为条件候选。
4. 稳定语义可作比较方向映射：AAV86/CA 的 rank 以较小元素为先，项目 rank 0 为最高优先级；项目须按“signed Q20.12 score 降序、原始下标升序”构成比较全序。原始下标须随秘密记录保留，不能用初始隐藏 shuffle 后的槽号代替。最终 mask、padding 进入动态图、输出逆映射等仍是项目扩展，尚未由论文闭合。
5. Agarwal Theorem 5.1 的 `2k+1` 轮是其 2+1 半诚实安全 CA 编译器的论文总轮数，不自动成为当前 Protocol I raw-score→mask 端到端轮数；更不证明 Protocol III 的团队目标 `2k`。当前 Protocol I 和 III 都没有 AAV86 自适应图 runtime。

## 2. 论文副本、来源和视觉核验

| 文献 | 本地文件与版本 | 页数 / SHA-256 | 视觉核验位置 | 来源状态 |
| --- | --- | --- | --- | --- |
| Agarwal 等，*Secure Sorting and Selection via Function Secret Sharing*，CCS ’24 | `Papers/Agarwal 等 - 2024 - Secure Sorting and Selection via Function Secret Sharing.pdf`；MIT DSpace 标为 final published version，会议版 | 15 页；`18faf63eaa7923eef715a6eb9d5d526fe04dcb69700b133c3e94de935f68c01c` | §5.1–§5.4：printed pp. 3033–3035 / PDF pp. 11–13；printed p.3036 / PDF p.14 用于成本表和 full-version 指引 | SHA-256 与 `docs/PAPERS.sha256` 相同。来源：[MIT DSpace](https://dspace.mit.edu/entities/publication/d70bd196-cf7c-46e1-a0a9-e3dfd08f379b)、[ACM DOI](https://doi.org/10.1145/3658644.3690359)。 |
| Alon、Azar、Vishkin，*Tight Complexity Bounds for Parallel Comparison Sorting*，FOCS 1986 | `Papers/Alon_Azar_Vishkin_1986_Tight_Complexity_Bounds_for_Parallel_Comparison_Sorting_FOCS_AuthorHosted.pdf`；作者托管版 | 9 页；`322f1bd761a987fd09e6b59b3a3ae77d6e4b1ca9dcb1c2765e45ac4a2a7e2b83` | printed pp. 503–509 / PDF pp. 2–8；重点 §2、§2.3、§3.1 / Theorem 3.1 | SHA-256 与 `docs/PAPERS.sha256` 中 P0 登记一致。来源：[Princeton 作者托管 PDF](https://web.math.princeton.edu/~nalon/PDFS/Publications2/Tight%20complexity%20bounds%20for%20parallel%20comparison%20sorting.pdf)、[IEEE DOI](https://doi.org/10.1109/SFCS.1986.57)。 |

以上页码和公式由本地 PDF 页面图像复核；文本抽取只用于定位。AAV86 原始算法步骤/定理和 Agarwal 的 local-rank、Algorithm 1/2、Theorem 5.1 均检查了页面图像。没有把会议版之外的推导、实验细节或作者实现当作已知。

### 2.1 版本与缺失来源

- Agarwal 本地 PDF 是 CCS ’24 会议版，不是已核实的作者 full version。论文 §5.3（printed p.3034 / PDF p.12）明确把部分转换讨论指向 full version；printed p.3036 / PDF p.14 也把 secure median 的完整成本分析留给 full version。P0 和本轮来源核对都**未核实到**来自作者、作者机构、出版方或正式 artifact 页面、且内容可归属的 full version 或对应作者代码。该结论只表示本次未核实，不表示公开世界不存在。
- `Agarwal_TopK/`、`ADSMPC/`、`CipherGPT/` 在本机存在且被忽略，但根目录没有可核验的 `.git` 来源元数据。可读文件只能称本地参考行为；不能登记成作者实现、固定 revision 或许可证已确认的代码。
- 本机 BB90 正文 PDF 仍未核实到；BB90 是 M6B，本阶段不下载、不分析、不实现。根目录书目 PDF 不属于 BB90 正文。
- 用户提供的 `C:\Users\28641\Downloads\parallel_shuffle_3round_reference.py` 可读取，大小 13,291 bytes，SHA-256 `404a90c600d9b9c97b4fc17957c4aa67df027435b7e673cffe9d6c3cead8779e`；文件头称其为新推导参考构造，并否认是找回的 Agarwal 作者代码。归类为历史设计线索，本轮未执行；它不证明 AAV86 图、离线 exact-edge 材料或安全性。
- 本轮消息中没有可访问的聊天截图附件或本地截图路径，截图状态不可核验，未作证据使用。

## 3. AAV86 原始论文：算法对象和保证

### 3.1 比较模型、同分和排序方向

`PAPER_DIRECT`：AAV86 §2（printed p.505 / PDF p.4）把输入视为全序元素；一次比较只返回 `<` 或 `>`。论文说明相等元素的相对顺序可由其下标决定，因此比较模型不返回“相等”。这给出稳定的**全序假设**，不是项目的 Q20.12 字编码，也没有声明 secure Top-K mask。

`PAPER_DIRECT`：论文排序结果按该全序递增。项目的“分数降序、原始下标升序”需要先定义反向分数主序、保持原始下标 tie 次序，再把 rank 0 解释为最高优先级；这是 `PAPER_DERIVED + PROJECT_CONTRACT`，不是论文默认 rank 方向。

### 3.2 不应混淆的两个 AAV86 结果

| 结果 | 原文位置 | 内容与保证 | 对 M6A 的关系 |
| --- | --- | --- | --- |
| 固定处理器数下的轮数上界 | §2.3，printed p.506 / PDF p.5 | 给定 `p ≥ n` 个处理器，在 parallel comparison model 中排序轮数上界为 `O(log n / log(1+p/n))`。 | 处理器数/轮数结果；不是 Agarwal Algorithm 1/2 所展示的固定 `k` 随机排序步骤。 |
| 固定轮数随机排序 | §3.1 / Theorem 3.1，printed pp.506–507 / PDF pp.5–6 | 对任意固定 `k ≥ 1`，论文给出显式 randomized sorting algorithm；期望比较数 `E(n,k) ≤ c(k)·n^(1+1/k)`，`c(k)` 只依赖 k。 | Agarwal §5.3 用于演示 Valiant→CA 转换的 AAV86 对象。期望比较数不是高概率界，也不是网络/安全运行时间。 |

Theorem 3.1 页面图像显示的执行骨架（§3.1，printed pp.506–507 / PDF pp.5–6）：令 `t = ⌈n^(1/k)⌉`；均匀、无放回选取 `t−1` 个 pivot；将每个 pivot 与当前集合元素比较（含 pivot 间比较）；按 pivot 顺序把非 pivot 元素分成 `t` 个区间/子集；在各子集上并行递归调用 `k−1` 轮算法。它是**全排序**递归，定理按比较次数计成本，比较量为随机变量，结论是期望界。

### 3.3 Selection 证据边界

printed p.504 / PDF p.3 的结果综述给出 median/selection 相关的固定轮数比较复杂度下界，并提到 Pippenger 的上界；同页还引述 Reischuk 的 randomized comparison selection algorithm。该处是选择复杂度和既有结果讨论，不是 AAV86 本文给出的、带完整 pivot/graph/recursive procedure 的 Top-K membership 算法。对项目可直接定位的 AAV86 构造仍是 Theorem 3.1 排序算法。

从完整排序功能上可以取前 K，但这不等于 AAV86 论文给出 `K` 敏感 comparison graph、Top-K mask 或 mask-only 安全协议。若路线 B 要称 AAV86 selection/Top-K，须补算法来源、完整步骤、适用 `K`、复杂度保证和 CA 定义。

## 4. Agarwal CCS ’24 §5：CA 改写和定理覆盖

### 4.1 Valiant 模型与 CA local rank

`PAPER_DIRECT`：§5.1（printed p.3033 / PDF p.11）将排序算法表示为 `k` 个可依赖先前轮结果的比较图 `H_i=(V_i,E_i)`。边对应比较元素对；总 edge complexity `e_A(n,k)=Σ_i |E_i|`，node complexity `v_A(n,k)=Σ_i |V_i|`。若算法随机化，两者为随机变量，文中以期望复杂度表述相关成本。

`PAPER_DIRECT`：§5.2 定义 Compare-and-Aggregate（CA）模型。对图 `H=(V,E)`，算法不接收每条边的比较结果，只接收每个节点相对邻居的 local rank。distinct-value 情况：

```tex
LRank(x_i) = |{ x_j : (i,j) ∈ E 且 x_i > x_j }|
```

non-distinct 情况在 §5.2（printed p.3034 / PDF p.12）定义为：

```tex
LRank(x_i) = |{ x_j : (i,j) ∈ E 且 x_i > x_j }|
            + |{ x_j : (j,i) ∈ E 且 x_j ≤ x_i }|
```

定义要求节点集合有序，并约定每条边 `(i,j)` 中 `i<j`。同分时，先前节点的相等元素计入当前节点 rank；较早下标排在相同值之前。这是 CA 图节点顺序下的 stable local rank，不是分数编码。

### 4.2 Algorithm 1、Algorithm 2 与适用范围

`PAPER_DIRECT`：§5.3 / Algorithms 1–2（printed p.3034 / PDF p.12）把 AAV86 randomized sorting algorithm 表示成两种模型的对应版本：

- Algorithm 1 是 Valiant model 的完整排序：`p=⌈n^(1/k)⌉`，无放回均匀选择 `p−1` 个 pivot；比较图由 pivot clique 和非 pivot 到 pivot 的 complete bipartite edges 组成；比较后按 pivot 次序把非 pivot 分成 `p` 个子集并递归排序。
- Algorithm 2 保留图、pivot 和递归控制流，以 CA local-rank 替代揭示全部比较结果，并用相应 rank 信息划分子集。文中说明此转换保持所示算法的 edge/node complexity。
- 两个算法展示对象都是**完整排序**，输出有序集合。论文的算法综述还覆盖 maximum、median/selection、sorting 和 sorted Top-K 等类别；这不把它们变成 Algorithm 1/2 的直接输出。

§5.3 表示并非任意 Valiant 算法都可自动转换；会议版处理的是若干有清楚图和步骤的算法，并把更广泛讨论指向 full version。P1 不将此扩张为“任何 Top-K 算法/图均可编译”。

### 4.3 Theorem 5.1 的功能、模型和计量

`PAPER_DIRECT`：Theorem 5.1（printed p.3035 / PDF p.13）针对 CA 模型中 `P ∈ {sort, select}` 的算法 A，依赖 `e_A(n,k)`、`v_A(n,k)`、`n`、`k`；这里 `k` 是算法 iteration 数，不是项目 Top-K 参数 `K`。定理给出 2+1 party 协议，输出 `F_P` 的安全分享，工作域为 `G=Z_(L')`、`L'≥2L`、`ℓ'=⌈log L'⌉`，并允许 `p`-bit payload。论文 §1 对这类协议的基本安全目标是针对任意一个半诚实 party；(2+1) 中 dealer 仅在 offline 发相关随机材料并在线静默。它不覆盖 dealer 与在线方合谋，也不是默认恶意安全。

页面图像核对的 Theorem 5.1 成本式：

| 定理项 | 论文给出的量 |
| --- | --- |
| 在线轮数 | `2k + 1` |
| 在线通信（两在线 party 合计） | `2 v_A(n,k)·(ℓ' + ⌈log n⌉) + 2n·(ℓ' + 2p)` bits |
| 在线计算（两在线 party 合计） | `2 e_A(n,k)·DCF.Eval[G, Z_n]` |
| 离线通信（offline party → 两在线 party） | `6n(ℓ'+p) + 4n⌈log L'⌉ + 2ℓ'(v_A(n,k)−n) + 2e_A(n,k)·(DCF.KeySize(G,Z_n)+⌈log ℓ'⌉)` bits |

这些公式是论文 CA 协议计量边界；不含本项目尚未设计/测量的 Q20.12 adapter、priority-key 适配、Top-K mask 输出、Protocol I/III 自有 shuffle/routing 或部署网络换算。`e_A`/`v_A` 不得用本地参考代码的一次边数代替理论定义。

### 4.4 Masked opens、shuffle、local-rank reveal 和 offline 材料

`PAPER_DIRECT`：§5.4（printed pp.3034–3035 / PDF pp.12–13）说明编译思路：两在线 party 持有元素 shares；每轮打开 masked input `x'_i=x_i+r_i`（一轮通信）；`r_i` 在 offline 采样并作为 `G_CmpAgg` secret parameters；FSS gate 后得到 local-rank shares。若最初先 shuffle 输入，则可用额外一轮安全揭示 local ranks，再按 CA 算法规则本地重排元素 shares。Theorem 5.1 报告全协议 `2k+1` 轮。

会议版给出了上述 masked-open、FSS gate、shuffle 条件和总成本，但没有逐项展开未来轮自适应 exact-edge DCF 材料如何在输入无关、在线静默的 dealer 模型中生成并绑定具体 edge、mask difference、父 transcript、session 与轮次。Theorem 的 offline communication 式按 `e_A` 计 DCF key 成本，不是该自适应发材机制的构造说明。full version 尚未核实到，因此列为 P2 必须补证项；不据此判定定理错误，也不宣称当前已能实现。

## 5. 并排比较五种算法/接口对象

| 对象 | 功能/输入输出 | 轮数或阶段 | 随机性与图 | tie / rank | 复杂度保证与证据级别 | 等价关系缺口 |
| --- | --- | --- | --- | --- | --- | --- |
| AAV86 原文 §3.1 / Theorem 3.1 | 全排序；n 个全序元素 → 全排序结果 | 固定 k 个 comparison rounds | 每递归层随机无放回采样 `t−1` pivots，pivot clique + pivot/非 pivot 比较，递归分桶 | 相等值以元素下标决定全序；递增排序、较小值 rank 先 | `PAPER_DIRECT`：期望比较数 `≤c(k)n^(1+1/k)`；非高概率声明 | 无项目 Q20.12 编码、Top-K mask、secure preprocessing 或 inverse route |
| Agarwal Algorithm 1（Valiant） | 全排序；AAV86 的 secure-compiler 输入图算法 | k 次模型 iteration | `p−1` uniform pivots、pivot clique + A-B edges、递归排序 p 个块 | 按有序节点及比较结果排序 | `PAPER_DIRECT`：§5.3；AAV86 固定轮排序示例 | 输出不是原顺序 bit-mask |
| Agarwal Algorithm 2（CA） | CA 模型全排序；输入图比较后用各节点 local rank 划块 | k 次 CA iteration；安全定理另给 `2k+1` 在线轮 | pivot / 图步骤与 Algorithm 1 对应；后续子图随 rank/块划分变化 | non-distinct local rank 显式保持节点顺序 tie | `PAPER_DIRECT`：§5.2–5.4；Theorem 5.1 成本按 `e_A,v_A` | 会议版没给 adaptive exact-edge offline 发材细节；没有 mask adapter |
| 本机 `Agarwal_TopK` 参考树 | `protocol1_ca` 有 pivot/recursive plan、rank transcript 和 direct-TopK plan 声明；`protocol3_ca` 有 bucket/pivot 图和状态更新 | CA 轮/递归子问题；不是来源已确认 runtime | `protocol1_ca/src/aav86.cpp` 的 pivot count 为 `p−1`，用 `rng.u64()%i` 表达 shuffle 抽样，再按打开 ranks 派生块/图；`protocol3_ca/src/aav86.cpp` 每 bucket 取 `p` 个 pivot 并建 pivot clique + pivot edges | `protocol1_ca/src/ca.cpp` 本地 key/clear oracle 以 score 降序、original index 升序；按打开 rank 派生计划 | `LOCAL_REFERENCE`：静态观察，未运行、未作正确性/安全/性能评价 | 两子树 pivot 计数不同；目录无 Git provenance，不证明作者版、实现等价或 exact offline preprocessing |
| 本项目 M6A 目标 | raw signed Q20.12 score shares → 长度 n、原始顺序、恰 K 个 1 的秘密共享 mask | Protocol I+AAV86、Protocol III+AAV86 两条独立组合 | 输入无关 P2 offline；自适应图/边材料与打开值待设计门 | score DESC、original index ASC、rank 0 最高；原始下标随 secret record 保留 | `PROJECT_CONTRACT + PROJECT_EXTENSION`；无 AAV86 secure implementation/benchmark | sort/select→mask、shuffle/inverse mapping、padding、泄露与轮数均未闭合 |

`protocol1_ca` 中 direct TopK 命名的 plan API 只表示本地参考行为；不能把原论文改写为 AAV86 Top-K 算法，也不能满足路线 B 的直接来源要求。

## 6. 项目稳定 Top-K 语义与候选映射

### 6.1 冻结契约与现有 helper

`PROJECT_CONTRACT` 见 `docs/decisions/M1_SCORE_SEMANTICS.md`、`VFSS/include/moe_topk/score_semantics.h`、`VFSS/include/moe_topk/topk_oracle.h`：

- score 是 32-bit raw word，按二补码 signed fixed-point 解释，scale=12；即 signed Q20.12。
- 总序为数值 score 降序；相同 score 以 `original_index` 升序打破 tie。
- rank 0 是最高优先级；合法输入 `n≥1, 1≤K≤n`；输出长度 n 的原顺序 bit-mask，恰有 K 个 1。
- `signed_score_to_ordered(raw)=raw XOR 0x80000000` 将 signed 数值序映射为 unsigned 升序。`top_k_precedes` 再反转 score 比较，并以原下标升序处理 tie。
- `protocol_i_priority_key.h` 的 clear helper 构造 `high=UINT32_MAX−ordered`，低位附 `original_index`；按 key 升序表示“高分优先、同分小 index 优先”。文件注释明确它是项目编码，不是 Agarwal 论文输入编码。
- `protocol_i_raw_score_input_party` 经两轮 carry/sign adapter 后产生 padded priority-key **shares**；secure 接口接收本方 raw-score shares，不是重构后的 score。helper 的 clear 算式是语义核对，不授权 secure runtime 解密 score/index。

### 6.2 从 rank 到 mask 的语义闭合项

1. **方向**（`PAPER_DERIVED`）：CA/AAV 的递增 rank 以较小元素为先；项目 rank0 为最高优先级。对 score 主序反向，tie 次序保持原 index 升序，之后选 `rank<K`。该方向关系闭合。
2. **stable index**（`PROJECT_EXTENSION`）：隐藏 shuffle 只可改变图节点位置，不能改变 tie 语义。每个 score 必须与秘密共享的 original index 同记录经过 pivot、bucket、递归和输出路径；不能把 shuffle 后 slot index 当原 index。现有 priority-key 逻辑表达此语义，但 AAV86 动态图接口尚不存在，因此端到端映射未实现。
3. **排序输出到 mask**（`PROJECT_EXTENSION`）：route A 从高优先级排序结果取前 K 成员，再把 membership shares 逆映射到原输入顺序。不能公开完整排序结果，也不能把 shuffled-order mask 当项目输出。论文 `F_sort`/`F_select` 分享输出不是项目 bit-mask API。
4. **重复分数**：只比较 score 会违反稳定次序；必须由 original index 全序决定同分。AAV86 的“按 index 排平局”和 Agarwal stable local rank 在抽象排序层兼容，但项目原始下标的绑定方式仍是适配契约。
5. **n、K 与边界**：项目 oracle 接受 `n≥1, 1≤K≤n`。当前 Protocol III `raw_score_mask` 校验 `logical_n≥2`，所以 `n=1` 不能通过该统一入口；独立 GRank 核心允许 `logical_n≥1`。Protocol I 输入布局可对 `logical_n=1` 取 `padded_n=2`。这是当前代码入口边界，不更改 M1 契约。
6. **padding**：当前 raw-score adapter 在 padding 槽放 signed 最小值，并以槽位构成 priority key；Protocol I 固定 clique pipeline 在 padded domain 上工作，Protocol III GRank 仅处理 logical slots。AAV86 pivot/edge/recursive graph 必须明确只包含 logical nodes，或证明 padding sentinel 不成为 pivot、不影响 local-rank 和递归划分。论文没有本项目 sentinel/padding 契约；此项未闭合。
7. **溢出/位宽**：helper 的 `32+index_bits` 和 pipeline comparison ring 校验是现有项目实现边界。论文的 `[L]`/`G=Z_(L')` 域不能直接证明这些位宽、signed 映射、padding sentinel 或 ring/field 互转在 AAV CA composition 中正确；各阶段须给具体编码和域证明。

## 7. Route A / Route B 决策

| 候选 | 算法对象 / K 对图规模的影响 | full order、mask 与逆映射 | 当前 P1 决策 |
| --- | --- | --- | --- |
| A：完整排序后取 Top-K | 直接对应 AAV86 Theorem 3.1 和 Agarwal Algorithms 1/2。排序图/比较量由 n、轮数 k 和 pivot 随机性决定；K 不在排序算法参数中，不能称 K-sensitive。 | 算法完成完整全序；`F_sort` 输出排序数据的分享。是否公开全序取决于 composition。项目 mask 仍需前 K membership 和 original-order inverse mapping；不能公开 full order。 | **当前唯一 source-backed AAV86 候选。** 不冻结 CA→Protocol I/III composition、轮数、opening 或 material 策略。 |
| B：来源完整的 selection/Top-K 图 | 原始 AAV86 selection 讨论不足以给出执行图；Agarwal Theorem 5.1 支持通用 CA `select`，但 AAV86 示例仍是 full sort，`select` 不等于 Top-K membership。本地 direct-TopK plan 无已核实作者来源。 | 特定来源的图/边复杂度可能随目标 rank/K 变化；单个第 K 项输出仍需成员比较、稳定 tie 和原顺序 mask 路由。 | **条件候选，未选定。** 先取得来源和完整定义；不得把本地代码或 generic theorem 改名为 AAV86 Top-K。 |

## 8. Protocol I：真实接口与 AAV86 路线边界

| 层次 | 当前代码事实 | 对照论文/项目目标 | 边界状态 |
| --- | --- | --- | --- |
| 三轮 shuffle + C-INSTANTIATION 核心 | `protocol_i_parallel_shuffle_three_round_party`（`VFSS/include/moe_topk/protocol_i_parallel_shuffle.h`）输入 `ProtocolIBlock192` record shares、party material 和 3 个 round fd；输出 shuffled shares、public masked records、public ranks、sorted shares。 | P0 handoff 标为 `C-INSTANTIATION` 三轮核心。不是 raw-score vector API，也不是 raw-score→mask 完整入口；`AUTHOR_EXACT` 仍 `NOT_PROVEN`。 | 保持单独核心标签，不把三轮计为完整 M6A route。 |
| 当前 score 输入适配 | `protocol_i_raw_score_input_party` 输入 `logical_n` 个 uint32 Q20.12 加法份额、party package、2 个 stage fd；输出 `padded_n` 个 uint64 priority-key shares。carry/sign 两阶段计 2 轮。 | 项目输入转换，非 AAV86 论文步骤。 | 有代码；不能单独产生 mask。 |
| 当前 fixed-clique priority pipeline | `protocol_i_priority_pipeline_party` 输入 padded key shares、shuffle/offline material、forward/cmpagg/rank-reveal/reverse channels；forward shuffle 2 + CmpAgg 1 + rank reveal 1 + reverse shuffle 2 = 6 轮。输出逻辑 n 原顺序 XOR mask shares。 | 现有 clique 控制路径；CmpAgg 和公开 rank 不等于 CA 自适应 AAV86 graph。 | 有代码；全图材料检查要求 `padded_n(padded_n−1)/2` 条边。 |
| 当前组合 raw-score→mask | 两轮 score adapter + 六轮 pipeline = 8 在线轮；pipeline 反向 shuffle 恢复原顺序。 | 工程完整入口/历史控制，不是三轮 C-INSTANTIATION core，也不是论文 CA 的直接轮数。历史整合 E2E 依赖可选 EMP，EMP OFF 构建中不可用。 | `2k+1` 不可直接套到该 8 轮路径或其未来替换上。 |
| AAV86 CA 每迭代 masked-open、FSS/local-rank 与 reveal | 当前 P-I path 有 fixed graph masked-input exchange、CmpAgg rank-share computation 和 rank-reveal phase；没有 AAV86 pivot/bucket/recursive graph runtime。 | Agarwal §5.4 给 CA gate、初始 shuffle 后 reveal local ranks 和总 `2k+1` 理论界。现有 P-I 消息与 theorem 的 shuffle 是否等价、哪些阶段可复用，尚未证明。 | 需逐消息因果推导，不能先声明 P-I 端到端 `2k+1`。 |
| AAV86 自适应图、pivot、递归子问题 | `VFSS/` 未发现 AAV86 runtime；本地 ignored reference 不能升级为生产入口。 | CA 下一轮 graph 依赖前轮排序结果/打开的 rank。 | 未实现；offline exact-edge material 是 P2 GO/NO-GO。 |
| 最终 mask 和 inverse mapping | 当前固定 pipeline 用 rank<K carrier 再执行 reverse shuffle；AAV mask path 未实现。 | 项目需要 original-order n-bit secret shares。排序/选择结果映射及成本须计入端到端。 | `PROJECT_EXTENSION` 待设计、验证和计量。 |

`ProtocolISecretSharedShuffle` header 的 forward/reverse 各有 2 轮，是项目 shuffle composition；不能仅凭接口名认定它与 Agarwal §5.4 的初始 hidden shuffle 在安全模型、correlation 或 rank reveal 上等价。三轮 `protocol_i_parallel_shuffle_three_round_party` 又是不同的 C-INSTANTIATION 核心。三者分别列账。

**Protocol I 当前边界：** AAV86 CA 算法对象可作候选；当前输入 adapter、shuffle、fixed-clique CmpAgg、rank reveal、reverse carrier 有接口或参考路径。动态图重算、exact-edge 离线发材、轮间 graph binding、AAV 输出 mask 组合均未实现。项目计划中的 P-I `2r+1` 是组合核心目标，不是当前 `raw-score→mask` 的实测/证明总轮数。

## 9. Protocol III：真实接口与不可继承的 CA 论证

| 接口 | 当前输入 → 输出 | 阶段/轮数 | 对 M6A 的边界 |
| --- | --- | --- | --- |
| `protocol_iii_raw_score_mask_party` | Q20.12 raw-score additive shares + score/GRank/routing one-shot material → 原输入顺序 XOR mask shares | score adapter 2 + GRank 1 + DPF routing 1 + 本地 mask conversion 0 = 4 轮 | 标准完整 raw-score-mask 工程入口；不是 AAV86 graph。初始无 hidden shuffle，P2 预分发输入无关材料。 |
| `protocol_iii_grank_party` | padded priority-key shares + package → `logical_n` 个 `Z_(2^rank_bits)` priority-rank additive shares | 1 轮 | GRank 是 logical domain fixed all-pairs CmpAgg；打开 masked key，不重构 key 或 rank；自身不做 route/mask。 |
| `ProtocolIIITwoRoundParty::consume_round2` | key shares + 已 field-share 的 encoded record shares → 一个 selected field record share | R1/R2，共 2 轮 | 固定 clique 的 field-valued Fselect core；调用方承担 key/record consistency。不是 raw-score→mask。 |
| `consume_round2_sort` | 同一两轮 transcript → logical_n 个 rank-order field-record shares | 2 轮；末轮本地 FullEval 不增加通信 | Fsort full-order core/control，不输出原顺序 mask；无 secure ring-to-field 或 mask adapter。 |
| AAV86 + Protocol III | 当前 VFSS 无接口 | `2r` 仅为项目组合目标 | 需独立消息依赖、安全/泄露和 round proof；Theorem 5.1 不是 `2r` 结论。 |

Protocol III 不能直接继承 Protocol I/CA hidden-shuffle 的 local-rank reveal 论证：

- 当前 GRank 按原始 logical positions 聚合 rank shares，不做 initial hidden shuffle；若公开 local ranks，观察者可把 rank 与原位置关联。原文“先 shuffle 后安全 reveal”不能推成“原顺序 reveal 同样安全”。
- 当前 GRank 输出 additive rank shares；rank-based routing 通过 DPF 完成。AAV86 若用已知 rank 生成下一轮 graph，必须另定谁看到哪些 rank、pivot/桶如何表示、两在线方怎样形成相同 node/edge 集合、图选择增加哪些轮和可见元数据。
- fixed all-pairs package、masked-key opening 和 DPF routing 不提供下一轮自适应 exact-edge 材料，也不证明没有 shuffle 时可打开位置关联的局部 rank。
- 若引入 hidden shuffle，需秘密携带 stable `original_index` 并最终逆映射；若不引入，则要有另一套经证明的泄露契约。P1 不设计加密、共享 graph 或 dealer workaround。

因此，Protocol III `2r` 保持为 `PROJECT_EXTENSION / team target`。不得写成 Agarwal 定理已证明轮数、当前两轮 field core 直接升级所得，或标准四轮 raw-mask 接口的推论。

## 10. 轮数、输出与视图的证据对照

### 10.1 轮数/功能表

| 对象 | 在线轮数 | 输出功能 | 计数边界 |
| --- | ---: | --- | --- |
| Agarwal Theorem 5.1 CA secure `sort/select` | `2k+1` | 论文算法 `F_sort` 或 `F_select` 的安全分享 | 定理公式；不是项目 raw-score conversion + original-order Top-K mask 的完整计数。 |
| Protocol I 三轮 C-INSTANTIATION | 3 | `ProtocolIBlock192` 核心输出，包括 shuffled/masked/rank/sorted fields | 核心构造；非 raw-score、非最终 bit-mask。 |
| Protocol I raw-score→mask modular pipeline | 8（2+6） | 原输入顺序 XOR mask | 含输入 adapter、forward/reverse shuffle、CmpAgg、rank reveal；fixed-clique engineering path，不是 AAV86。 |
| Protocol III raw-score→mask | 4（2+1+1） | 原输入顺序 XOR mask | 含输入 adapter、fixed-graph GRank、DPF route。 |
| Protocol III Fselect/Fsort field core | 2 | selected record share 或 rank-order field record shares | 不含 raw-score/ring-to-field 输入 adapter 和 original-order mask adapter。 |
| M6A+AAV86 两路线 | `NOT_PROVEN / NOT_IMPLEMENTED` | raw-score shares → 原顺序 Top-K mask shares | P-I `2r+1` 与 P-III `2r` 是项目目标；causal schedule 和 adapter/inverse mapping 尚未闭合。 |

### 10.2 Theorem 5.1 下的角色视图（论文模型）

会议版可支持的角色边界见 Agarwal §1（printed pp.3024–3025 / PDF pp.2–3）及 §5.4 / Theorem 5.1（printed pp.3034–3035 / PDF pp.12–13）。确切 FSS key 内容、随机数保留/擦除和 adaptive edge key 生成未逐项说明。

| 角色 | Offline 阶段 | 每个在线 iteration | 结束后 | 能确认的安全范围 |
| --- | --- | --- | --- | --- |
| P0（online） | 不持有明文输入；接收 party-local correlated randomness/FSS material。具体自适应 edge key/share/view 未描述。 | 参与 masked `x+r` 打开；执行本地 FSS/CA；持有 local-rank shares。先 hidden shuffle 时，local ranks 可经额外一轮揭示以控制排序/重排。逐轮 pivot、graph labels/ranks 的可见性需落到 transcript。 | 得到 `F_sort`/`F_select` 输出 share，可能持有公开 shuffled-domain local ranks/transcript；会议版没有项目 mask 输出。 | 针对任一单个半诚实 party；不外推到 P0 与 dealer 合谋或恶意 party。 |
| P1（online） | 与 P0 对称，持有另一份 party-local material/input shares；不能默认知道 P0 的单份 mask/key。 | 看到相同 public openings 和 peer transcript，获得另一份 rank/output shares；需界定公开 rank 与 shuffled position 的绑定。 | 得到对应 output shares 和 transcript；公开内容取决于 output functionality。 | 同上。 |
| P2（offline dealer） | 输入未知时提供相关随机材料；论文 2+1 角色要求 online 静默。其随机性、shuffle permutation/r、两份 FSS keys、edge 材料及发放后状态须按具体协议列出；adaptive exact-edge 细节未说明。 | 无在线消息或 input/rank 访问；若 P2 按 rank 生成材料，将改变论文角色/轮数。 | 无在线 transcript 新消息；dealer state 擦除/保留未逐项冻结。 | 单独 P2 属于单个半诚实 party；P2 与 P0/P1 合谋不在该单方腐化声明内。不得把 online 静默说成自动满足安全模型。 |

Project P2 gate 必须逐阶段列出实际视图：离线 masks、DCF keys、shuffle permutation/correlation、发材时间；每轮 masked open、rank/bucket/pivot、公开 graph 和收件方；结束时状态擦除与输出 shares。上述是论文角色边界，不声称当前本地代码与论文 view 完全相同。

## 11. 未决事项与 P2 GO/NO-GO 门

| ID | 具体问题 / 阶段 | 参与方 | 当前已知 | 未知与影响 | 建议负责人 | P2 前必须通过？ |
| --- | --- | --- | --- | --- | --- | --- |
| G1 | full sort→mask 还是 K-sensitive selection/Top-K 图 | 算法设计 | AAV86 direct source 是 randomized full sort；CA theorem 支持通用 select | route B 算法、来源、复杂度、输出未定，影响标签、K 规模、成本对照 | M6A 算法负责人/共同设计 | **是**；或确认 route A |
| G2 | 每轮自适应 edge set 何时确定 | P0、P1、P2 | 下一图可依赖前轮 rank/桶；论文要求 P2 online silent | 输入无关 offline exact-edge material 怎么覆盖在线才确定的边未解释 | 协议/预处理设计负责人 | **是，NO-GO blocker** |
| G3 | uCMP/DCF material 与 edge、mask difference、session、iteration、父 transcript 如何绑定 | Dealer、P0/P1 | tracked fixed-clique 包按固定边序绑定；handoff 要求 dynamic graph adapter | 新图材料如何预先生成、拒绝 relabel/replay 未定 | 材料/接口 owner | **是** |
| G4 | exact-edge 材料不能离线准备时的策略 | P2、P0/P1 | online Dealer 不属于目标模型；完整 clique reserve 只能作 control | theorem-backed preprocessing 是否存在；control 是否只用于对照；不能冒称 target | 设计者 + benchmark owner | **是**；无方案则 target NO-GO |
| G5 | 每轮打开 masked value、pivot、bucket、local rank、位置和接收方 | P0/P1/P2 | §5.4 提 masked `x+r`；先 shuffle 后可 reveal local ranks | rank 数组索引域、消息顺序和 graph metadata 可见性未定 | 安全审查人 + 协议 owner | **是** |
| G6 | P2 corruption view、permutation/mask/r 可见性及保留 | P2、P0、P1 | conference 定义单个半诚实 party；P2 dealer online silent | P2 是否能单独持有完整 permutation/mask/correlation/r；合谋和擦除状态未知 | 安全设计负责人 | **是** |
| G7 | Protocol I hidden shuffle 与 local-rank reveal | P0/P1/P2 | Agarwal 说先 shuffle 可安全 reveal；当前 I 有 forward/reverse 2-round path 和独立 3-round C-INSTANTIATION | 两者数据记录、置换 view 与 theorem 前提等价性；original_index 怎样穿 shuffle | Protocol I owner + reviewer | **是** |
| G8 | Protocol III 是否有 hidden-index/shuffle 或等价泄露契约 | P0/P1/P2 | 当前 III fixed GRank 用原顺序位置、不 shuffle；rank 是 shares，mask 经 DPF route | 动态图依据的公开值、position leakage、一致 edge set、逆映射未知 | Protocol III owner + security reviewer | **是** |
| G9 | 每条路线真实因果轮数及 adapter 账 | P0/P1 | paper theorem `2r+1`；project P-III `2r` 是 team goal；current I/III masks 8/4 轮 | score adapter、shuffle、rank reveal、graph control、routing、mask/inverse map 增量未定 | 两路线负责人各自给出 | **是**；不可移植轮数 |
| G10 | 完整 sort 结果怎样成为 original-order mask | P0/P1 | 项目输出契约有 mask；现有 fixed-clique routes 可出原顺序 mask | CA `F_sort`、前 K membership、hidden permutation 与 inverse map 无接口/证明/成本 | 输出适配 owner | **是** |
| G11 | n=1、K=1/K=n、非 2 次幂、递归空块与 pad | P0/P1 | 项目 `1≤K≤n`；I 可 pad；III raw-mask 当前 n≥2；III GRank 只用 logical_n | AAV pivot 数、pad 排除、sentinel tie 与终止规则未定 | 算法 + adapter owner | **是** |
| G12 | full version、作者代码、参考树 revision | 算法设计 | MIT DSpace 证实会议最终版；参考树本机存在 | 未核实作者 full version/code；树上游/revision/license 不明，限制来源身份 | 资料/算法 owner | source-backed implementation 前 **是** |
| G13 | full-clique 大规模控制资源记录 | benchmark owner | fixed all-pairs controls 存在；V3 要记成功/失败/资源受限/未运行 | `n=10^5,10^6` 每点是否必须尝试 clique 需解释矩阵要求 | benchmark owner/学姐 | P2 设计时确认；不影响算法身份 |

**NO-GO 条件：** G2–G10 任一项没有明确角色、值流、材料时机、因果轮数和泄露/逆映射边界时，不进入 target adaptive secure runtime 实现。可独立保留明确标签的 non-adaptive full-clique control；它不得替代 AAV86 target，也不得冒用其复杂度或安全结论。G1/G11/G12 须在冻结实现范围及复现标签前关闭。

## 12. 供学姐确认的问题（本记录不代为联系）

| 问题 | 已知证据 | 剩余歧义 | 需要对方给出的具体答案/资料 | 类型 |
| --- | --- | --- | --- | --- |
| 1. M6A 是否采用 AAV86 完整排序后输出 mask？若直接 Top-K/selection，具体哪一版？ | AAV86 §3.1、Agarwal Algorithms 1/2 是 full sort；Theorem 5.1 的 select 是通用 CA 算法范围。 | 是否接受 route A 与 K 无关的排序成本，或存在来源完整 route B。 | 算法标题/论文页节/步骤、K 参数、期望或高概率保证和输出对象。 | **设计决定** |
| 2. 2+1 模型下 P2 的腐化和合谋范围是什么？是否可见完整 permutation、mask/correlation 或 r？ | Agarwal 描述任意单个半诚实 party；dealer 只发 offline material，在线静默。 | 项目允许的持久 dealer view、擦除要求及 P2 与在线方合谋是否超出模型。 | corruption set、P2 离线输入/随机状态、材料保留/销毁要求。 | 安全设计决定 |
| 3. CA 每轮 masked opens、local-rank reveal 与 hidden shuffle 怎样对应？full version 是否补充 exact-edge preprocessing？ | §5.4 写明打开 `x+r`、调用 FSS gate、得到 rank shares；先 shuffle 后可 reveal；Theorem 5.1 给 `2k+1`。 | conference 版未逐项给自适应 edge key/material 的生成绑定。 | 可核验 full version URL/页节，或组内消息与材料推导。 | 部分文献核验，组合需设计 |
| 4. Protocol III+AAV86 是否有认可 hidden-index/shuffle/leakage contract？`2r` 是硬目标还是探索目标？ | 当前 III 无初始 shuffle；project plan 把 `2r` 作为团队目标并要求独立推导。 | rank 未 shuffle 时允许公开到什么程度，如何形成一致下一图。 | 设计目标、允许 opens、轮数口径和审查 owner。 | **设计决定** |
| 5. 是否存在 Agarwal 作者发布、revision/许可证可验证的 full version、代码或 artifact？ | MIT DSpace 核对的是 15 页 final published version；本次未核实作者代码/full version。 | 本地参考树没有 provenance 元数据。 | 作者/机构/正式 artifact URL、commit/tag、license 及与 CCS 算法对应说明。 | 来源核验 |
| 6. 是否已有输入无关 offline dealer 准备 adaptive exact-edge material 的组内构造或作者答复？ | local `ADAPTIVE_PREPROCESSING_DECISION.md` 标为 `BLOCKED_MISSING_ADAPTIVE_PREPROCESSING_DETAIL`；tracked packages 为 fixed-clique。 | 后续边依赖已开 local rank，边/DCF key 需 offline 固定。 | 构造、dealer view、edge/mask/session/round binding、安全假设和轮数；若无，确认 NO-GO。 | **设计/安全 gate** |
| 7. V3 在 `n=10^5,10^6` 是否要求每点都尝试 full-clique baseline？资源限制怎样分类？ | `docs/BENCHMARK_VALIDATION_PLAN.md` 要求记录成功、失败、资源受限和未运行，及统一矩阵。 | 巨大 clique 的执行义务和停止阈值需解释。 | 每点 baseline 必试或 `RESOURCE_LIMITED` 规则；不要求估算性能。 | benchmark 口径 |

## 13. P1 判定和进入 P2 的条件

### P1 结论

- **算法身份：通过。** Agarwal CCS’24 的 AAV86 示例对应 AAV86 Theorem 3.1 固定轮数 randomized full sort，不是已证明的 AAV86 Top-K mask 算法。
- **语义方向：概念映射通过，接口闭合未通过。** score DESC / original index ASC / rank0 highest / `rank<K` 可在比较全序上对应；Q20.12、原始 index 随秘密记录穿 shuffle、padding 图节点约束和 inverse mask 路由仍须项目证明/适配。
- **路线状态：A 为唯一 source-backed AAV86 候选；B 保留条件候选。** 不以本地 direct-TopK 命名代替论文来源，也不冻结未核实 selection 算法。
- **Protocol I：未见 AAV86 adaptive runtime；三轮 C-INSTANTIATION、8 轮 raw-score mask pipeline、CA `2r+1` theorem/target 是不同边界。**
- **Protocol III：4 轮 raw-score mask、fixed 1-round GRank、2 轮 field Fselect/Fsort 是不同接口；`2r` 仍是 project target。** 不继承 CA shuffle/reveal 安全结论。
- **复现限制：** `AUTHOR_EXACT = NOT_PROVEN` 等 M5 接收报告限制保持不变；G3 PASS 是工程接收结论，不是作者精确复现、安全证明或本轮测试结果。

### P2 readiness

`P1 = PASS_WITH_EXPLICIT_OPEN_DECISIONS`：证据盘点足以把算法身份、稳定顺序、route A/B 和 I/III 当前边界交付审阅；仍有具名缺口，不以“全部解决”作为阶段完成条件。

`P2 adaptive secure runtime implementation = NO-GO`，直到至少 G1–G12 按表关闭或得到有记录的设计决策，特别是：

1. route A/B 和确切算法版本；
2. offline-only 自适应 exact-edge 材料及 edge/stage binding；
3. P2 单方腐化 view、初始 shuffle 与每轮 openings；
4. Protocol I、Protocol III 各自独立的因果消息/轮数/leakage 证明；
5. signed Q20.12 / original_index / padding / Top-K mask 和 inverse mapping 的端到端接口。

**允许进入 P2 的范围：** 继续安全/算法设计与资料核验，关闭上述决策门；不允许据 P1 直接写 secure runtime，或将 fixed-clique/online-Dealer prototype 标为 AAV86 target。

## 14. 本轮执行记录

- 静态读取项目文档、P0 报告、M5 handoff/接收记录及 Protocol I/III headers/相关实现。
- 复核两份 PDF 文件名、页数、字节数、SHA-256；视觉检查 AAV86 PDF pp.1–8、Agarwal PDF pp.11–14 中相关原文页/公式。关键定义、Theorem 3.1/5.1 和 cost 以页面图像为准。
- 仅静态查看本机 `Agarwal_TopK/protocol1_ca` 和 `protocol3_ca` 的相关 AAV86/CA 源码与 declaration；没有执行参考程序或用户脚本。
- **未运行任何测试**，未改 `VFSS/`、`VFSS-baseline/`、论文正文、本地参考目录或根目录 PDF；未提交、未推送。
- 修改 `PROJECT.md` 仅为澄清 Git 跟踪状态与本机 ignored reference copy 的区别；本决策记录是新增交付物。
