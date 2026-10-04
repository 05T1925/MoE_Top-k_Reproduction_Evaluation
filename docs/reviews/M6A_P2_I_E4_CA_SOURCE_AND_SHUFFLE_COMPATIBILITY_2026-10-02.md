# M6A-P2-I-E4：Agarwal CA 原文证据与 Protocol I Shuffle 兼容性审查

日期：2026-10-02
审查 worktree：C:/Users/28641/Desktop/MoE_Top-k_Reproduction_Evaluation__m6a_p2_i_allpairs_experiment
分支：codex/m6a-p2-i-allpairs-experiment
审查基线：c3926c68fd14f270faa8b55234311071947fa080
文档属性：来源、论文语义与接口组合审查；不构成实现规格、协议证明或运行时批准。

## 0. 范围与结论摘要

本报告只核对 Agarwal CCS’24 会议版、AAV86 原文、E1–E3 已有证据和当前 Protocol I 源码接口。论文陈述、项目契约、实验观察和未证明的组合分别标记；任何代码接口都不反向充当论文结论。

本轮发现：

- 作者主页实际链接的 PDF 是 15 页 CCS’24 会议版。它与仓库现有 PDF 的逐页抽取文本一致，但文件字节和 SHA-256 不同。
- 会议版明确给出安全 shuffle 的理想输出、CA local-rank、AAV86 的 CA 算法框架和 2k+1 轮的定理；它把 shuffle 的正式功能描述/具体实例化、CA 转换更多讨论和 Fselect 完整结构指向 full version。
- MIT 仓储列出 final published version PDF 及自动沉积 ZIP；当前下载端点触发验证/405，未检查 ZIP 内容。因此 full version 或 supplement 状态为 UNRESOLVED，不能写成不存在。
- 本轮核实的作者主页和作者机构页面未找到可归属本文的作者代码；状态为 NOT_FOUND_IN_SOURCES_CHECKED。
- 当前两轮 SecretSharedShuffle、三轮 C-INSTANTIATION core 和 fixed-clique priority pipeline 都不能单凭名称或相似输出证明与论文 CA 使用相同的 π、同一组 endpoint masks/key 及最终 inverse route。
- E1 全两两材料仍只是实验候选。整池单方视图安全、自适应查表/端点泄露和 AAV86 到原输入顺序 mask 的组合未证明；secure runtime 保持 NO-GO。

## 1. Git 基线、工作区与已读文件

### 1.1 起点状态

| 工作区 | branch | HEAD | E4 开始时状态 |
|---|---|---|---|
| 主工作区 C:/Users/28641/Desktop/MoE_Top-k_Reproduction_Evaluation | feat/m6a-performance-evaluation | c3926c68fd14f270faa8b55234311071947fa080 | 有既存修改与未跟踪资料，逐项见下表；E4 未触碰 |
| 隔离实验 worktree | codex/m6a-p2-i-allpairs-experiment | c3926c68fd14f270faa8b55234311071947fa080 | 已有 E1、E2、E3 报告及 experiments/ 未跟踪内容；E4 仅新增本报告 |

主工作区起点的既存状态：

- 修改：PROJECT.md、docs/IMPLEMENTATION_PLAN.md、docs/PAPERS.sha256、docs/REFERENCE_MANIFEST.md。
- 未跟踪：docs/decisions/M6A_AAV86_ALGORITHM_AND_STABLE_ORDER_DECISION_2026-09-27.md、docs/decisions/M6A_PROTOCOL_I_AAV86_PREPROCESSING_AND_SECURITY_DESIGN_2026-09-28.md、docs/reproduction/M6A_BASELINE_AND_SOURCE_INVENTORY_2026-09-27.md、根目录 siamjdiscrmath.pdf。
- siamjdiscrmath.pdf 作为既有用户资料保持原样；本报告没有把它当作本文或任何论文正文。

E4 开始时主工作区和隔离 worktree 都在同一 c3926c68 基线。主线日志顶部为 c3926c6（Merge pull request #27 from 05T1925/docs/m5-g3-closeout），其前为 ba7efd3、c72039c、89147bc、9b3ce37。E4 未切换分支，也没有暂存、提交、推送、合并或创建 PR。

### 1.2 读取材料与 SHA-256

以下哈希是在对应当前工作区文件上实测；主工作区中已修改或未跟踪文件的哈希表示 E4 审查时读取的内容，不表示基线提交内容。

| 文件 | 核对用途 | SHA-256 |
|---|---|---|
| 主工作区 PROJECT.md | 项目约束、输入输出契约及 M6A 状态 | 33B66C6BD6CCE8FCEAB817FB68C17BEF2A4E58194A39B41AA4D48AD79D2C05D5 |
| 主工作区 docs/IMPLEMENTATION_PLAN.md | 当前里程碑和路径限制 | F5C2D1DCD6587D763EFBD8DBF3D6CF8F9A7014EDB93145A7678ADF6876DD49E8 |
| 主工作区 docs/BENCHMARK_VALIDATION_PLAN.md | 离线、在线、总成本计量边界 | DBFAA8A850541A4A8BFA5DAE708804CF2DB4C44F82A9D4E3402E87AB10198756 |
| 主工作区 docs/decisions/M1_SCORE_SEMANTICS.md | Q20.12、stable tie 和 mask 语义 | 5920A00EAA41CC04712E0338F82305EE255730E944AB38293E9C65C70FBA426D |
| 主工作区 docs/decisions/M6A_AAV86_ALGORITHM_AND_STABLE_ORDER_DECISION_2026-09-27.md | §§5–10：AAV86、stable key、Protocol I 边界 | 21DE1BE89CFCC7B66005B8E4F34BCAFFBC7556CCF22FAE7F517FEC91681456C0 |
| 主工作区 docs/decisions/M6A_PROTOCOL_I_AAV86_PREPROCESSING_AND_SECURITY_DESIGN_2026-09-28.md | §§4–10：T、party view、mask、CA 因果边界 | C19DF3632C411BF7BD8DEFC6C003CCDC7FC90963D6C46C871FE12D91AD551BDC |
| 主工作区 docs/reproduction/M6A_BASELINE_AND_SOURCE_INVENTORY_2026-09-27.md | 早期基线和来源盘点 | B637F31009F329391D1CC92CC959E251E64A0AAC45C5FBB96CFEE0D471B93EFE |
| 隔离 worktree docs/reproduction/M6A_P2_I_ALL_PAIRS_MATERIAL_EXPERIMENT_2026-09-30.md | E1 实验约定、n=5/r=2 观察值和限制 | 0F54A121DC2CFFA70719C735F577C66022C2ED019706415CBC5D9A87347537A4 |
| 隔离 worktree docs/reviews/M6A_P2_I_E1_INDEPENDENT_AUDIT_2026-10-02.md | E2 独立复核和 errata | 307B80ABC789F2C1EB0614992A4E2C4938A7AB51BB3BCB089951D713C6C23ED9 |
| 隔离 worktree docs/decisions/M6A_PROTOCOL_I_AAV86_ALL_PAIRS_DESIGN_GATE_2026-10-02.md | E3 设计门结论和源码清单 | CE43F664BADC664A918C919EEDA889B6586C8EA224B0CF0D512712A914906965 |
| 隔离 worktree experiments/m6a_p2_i_allpairs/aav86_graph_counter.py | 明文 AAV86 图计数器；非 secure runtime | 09DCC72DF6BC68C83046CBB3F7C9AF8B13642BEA720036B92804F6BE5423B402 |
| 隔离 worktree experiments/m6a_p2_i_allpairs/aav86_graph_counts.csv | E1 图计数输出；E2 有限定范围的独立审计 | 5346AE011201C1F88F3B7199BB86DE7C1B4F761151F3FB269C86F9E99AF9F622 |
| 隔离 worktree experiments/m6a_p2_i_allpairs/material_pool_test.cpp | E1 TEST_ONLY 真实 DCF key pool harness | 8F6F36C5408A55A09CB9754C7076C2C27EADD0DDD539B7D5297949A9DEF0E159 |

Protocol I 源码读取范围与哈希：

| 文件 | 本轮核对内容 | SHA-256 |
|---|---|---|
| VFSS/include/moe_topk/protocol_i_score_input.h | raw score 输入 adapter 接口 | 8103866D657721F604DE3CB7A475824830581FE1D64A243A21B17972C6CEB0D1 |
| VFSS/src/moe_topk/protocol_i_score_input.cpp | 两轮 score→priority-key 路径 | 282D44D2632D92BD22D70FF64495FF7B65630D1CC15ACCB959A1E9D0642E9AD8 |
| VFSS/include/moe_topk/protocol_i_priority_key.h | 项目稳定 priority-key 语义 | 8C345675877FDD101E0B5918250FF7E7B6879031161CD38B7FE141CE96D0E338 |
| VFSS/include/moe_topk/protocol_i_pipeline.h | 输入 layout、pipeline 接口和轮数计数器 | 4871BD423ABD71758661B6668249D4D1268EFA240C8894658F804DC411DCAC6F |
| VFSS/src/moe_topk/protocol_i_pipeline.cpp | fixed-clique、rank reveal、rank<K carrier 和 reverse | 0FD331AD19637415CD83307DE96AE6DD3C4D3DE21D0DD34858B5143847E94D06 |
| VFSS/include/moe_topk/protocol_i_secret_shared_shuffle.h | 两轮 forward、两轮 reverse 的 party-local API | 7C739B2B7DE07791AE746C1E5B942C267F332D7058F465FBE9CBD04A1EDCF197 |
| VFSS/src/moe_topk/protocol_i_secret_shared_shuffle.cpp | Permute+Share 组合与逆置换调用顺序 | 4B83CED81802CD1C1EECD484C96CFA1866F8601131D052C34A672A8223C6121C |
| VFSS/include/moe_topk/protocol_i_parallel_shuffle.h | 三轮 C-INSTANTIATION core 输入输出 | A88FFF48B9B2F1F423EFDAECEA9B8F89C01F25C91BFE1A0AEA4F358604DA672D |
| VFSS/src/moe_topk/protocol_i_parallel_shuffle.cpp | 三轮消息阶段、masked record 和公开 rank | 8D0E488B6348CAA6BC55D8F6652198E97FC6ED69381D5B367091D71B77DCD75D |
| VFSS/include/moe_topk/protocol_i_cmpagg.h | party-local CmpAgg 声明 | D50B7B84A3EEF299D1F1C23A4D342C7F95003C28DD0F72CD8BD56FD0E3878390 |
| VFSS/src/moe_topk/protocol_i_cmpagg.cpp | 固定 clique 边材料检查与 rank 计算 | 5183E371ADC7D06DF6E93097921B2C303B1AC5A8C24B883CA2330855521FCA4F |
| VFSS/include/moe_topk/protocol_i_ucmp.h | uCMP party material 接口 | 7E444F0188AFA2FAF0CE8A1FA97AF7D821C54FE82FB46CEFD1999AEA4C798D33 |
| VFSS/src/moe_topk/protocol_i_ucmp.cpp | mask-difference DCF keygen/eval | B1CC25814E772E0B62E71CA97465FECB16B9010F896D062BEA591E899743FB23 |
| VFSS/include/moe_topk/protocol_i_permutation.h | apply/compose 置换约定 | DE94DC9DC6FECB1C1D0E62EF09469462564D7FC8BE62C993BBAB62D8BD5D3E2F |
| VFSS/src/moe_topk/protocol_i_permutation.cpp | permutation / inverse helper | C92CA4F70C25F88C8D23F2311A72CD9B63D36B28C84CF894EFAE2C9EE8169DF1 |
| VFSS/include/moe_topk/protocol_i_permute_share.h | Permute+Share 子协议接口 | CC2AEA70E6F3FB65026B10DCC2AC690A9B056A1DC1C149BDCC3416F0A2AD228D |
| VFSS/src/moe_topk/protocol_i_permute_share.cpp | Permute+Share 子协议实现 | E53FE7FB173729B5C342251338026E2EE42CC589808315B7B5D9F3612A4E4778 |
| VFSS/include/moe_topk/protocol_i_party_package.h | party package 材料字段 | 9258F0F141E8D7F0346559458D2B280255633AE5BECBA41A00BCFE0D83867AA9 |
| VFSS/src/moe_topk/protocol_i_party_package.cpp | party package 校验与消费路径 | E3D31C04DCD979A882E8FA37C75C80C243494A75848CA4B00946842A0088E216 |

## 2. 正式来源、版本和本地副本状态

核验日期：2026-10-02。网络取得的论文副本只保存在系统临时目录，没有写入仓库、Papers/、manifest 或 Git。

| 来源 | 核验结果 | 文件/版本/页数/哈希 |
|---|---|---|
| Elette Boyle 作者主页：https://cs.runi.ac.il/~elette/ | 页面列出本文、八位作者，并提供 PDF 链接。条目属于 CCS 2024。该条目没有作者代码/artifact 链接。 | 作者链接 PDF 下载到 C:/Users/28641/AppData/Local/Temp/m6a-p2-i-e4-source-check/Secure-kth-stats.pdf；15 页，1,387,175 bytes；SHA-256 CE6C50A395B303EB4BE295BC782068BE6C6DDE47300537FBF3E47DDA27097811。PDF 内 CCS’24 页眉可核。 |
| 作者 PDF：https://cs.runi.ac.il/~elette/Secure-kth-stats.pdf | 本次下载成功；定位为 CCS’24 会议版，而不是从文件名推断版本。与本地 15 页文件逐页做规范化文本抽取比较，15/15 页一致、未发现文本页差异。 | 会议版 PDF bytes 与本地文件不同，SHA-256 不同；逐页文本一致不等于版面或字节相同。 |
| 仓库本地 Agarwal PDF：Papers/Agarwal 等 - 2024 - Secure Sorting and Selection via Function Secret Sharing.pdf | 已存在；题名和页眉与作者页所列 CCS’24 版本一致。 | 15 页，1,249,281 bytes；SHA-256 18FAF63EAA7923EEF715A6EB9D5D526FE04DCB69700B133C3E94DE935F68C01C。 |
| MIT DSpace：https://dspace.mit.edu/entities/publication/d70bd196-cf7c-46e1-a0a9-e3dfd08f379b | 记录给出出版者 ACM、Proceedings of 2024 ACM SIGSAC CCS、Issued 2024-12-02、DOI 10.1145/3658644.3690359，版本字段为 Final published version；PDF 记录名 3658644.3690359.pdf，1.16 MB，仓储元数据 MD5 79bf74a6ce514109e5e09e1cfcaed386。记录也列出 1.13 MB ZIP，说明为 automated deposit 提交文件集合。 | 直接取 PDF/ZIP 时站点返回验证页/HTTP 405；本轮没有得到文件字节，故无实测 SHA-256、ZIP 内容和 PDF 页数。未尝试绕过站点验证。仓储所示 MD5 只是其元数据，不能当成本地实测哈希。 |
| ACM DOI：https://doi.org/10.1145/3658644.3690359；出版页：https://dl.acm.org/doi/10.1145/3658644.3690359 | DOI 与 DSpace 记录相互对应。浏览核验工具打开 dl.acm.org 返回 Internal Error；未能核查 ACM 页面上可能存在的 supplement/artifact 文件。 | ACM 直接下载/页面附件状态 UNRESOLVED。 |
| Microsoft Research 作者机构页面：https://www.microsoft.com/en-us/research/publication/secure-sorting-and-selection-via-function-secret-sharing/ | 机构页面列出作者、CCS 2024、ACM 出版者和论文摘要；没有在该页面找到代码或 full-version 附件链接。 | 网页来源，不是另一个 PDF 版本；不产生论文文件哈希。 |
| AAV86 Princeton 作者托管 PDF：https://web.math.princeton.edu/~nalon/PDFS/Publications2/Tight%20complexity%20bounds%20for%20parallel%20comparison%20sorting.pdf；IEEE DOI：https://doi.org/10.1109/SFCS.1986.57 | Princeton 作者托管链接可读；本地参考副本为 9 页，FOCS 1986 版本。按 E1–E3 已核页段，§3.1 / Theorem 3.1 在印刷 pp.506–507、PDF pp.5–6。 | Papers/Alon_Azar_Vishkin_1986_Tight_Complexity_Bounds_for_Parallel_Comparison_Sorting_FOCS_AuthorHosted.pdf，1,063,055 bytes；SHA-256 322F1BD761A987FD09E6B59B3A3AE77D6E4B1CA9DCB1C2765E45AC4A2A7E2B83。E4 未下载另一副本。 |

### 2.1 Full version、supplement 和作者代码状态

作者主页 PDF 是 15 页 CCS’24 会议版。论文在 §2.4 明说 full version 会给出 shuffle functionalities 的正式描述和一种改编自先前工作的具体协议；§5.3 指向 full version 进一步讨论 CA 转换；§4.2 / Fselect 段落也有 full-version 细节指引。这些指引本身说明会议版没有把所有实现决定所需细节都放在当前正文，不能把定理成本记号当作完整的预处理工厂说明。

本次核查了 Elette 作者主页及其本文条目、Microsoft Research 作者机构页面、MIT DSpace 条目和 ACM DOI/出版页入口，并查询了精确题名与 supplementary/full version/code 的作者归属结果。观察到 MIT DSpace 确有一个自动沉积 ZIP，但由于下载端点拒绝取件，里面是否含 full version、代码或附录仍是 UNRESOLVED。未找到可归属作者、作者机构、ACM 或正式 artifact 的对应实现仓库。准确状态是：

- AUTHOR_SOURCE = VERIFIED：作者主页直接提供并识别 CCS’24 会议版。
- FULL_VERSION = UNRESOLVED：会议版明确引用 full version；仓储 ZIP 存在但未能检查，不能断言找不到或不存在。
- CODE = NOT_FOUND_IN_SOURCES_CHECKED：已检查的作者/机构/出版方来源中未核实到可对应本文 CA/AAV86 构造的作者代码；不代表公开世界不存在。

没有把搜索摘要、非归属转载或 E1/E2/E3 的测试代码登记为作者实现。

## 3. Agarwal CCS’24 中 CA 与 AAV86 的可核语义

以下为论文直接定义或算法文字，页码按会议版印刷页 / PDF 页同时标出。PDF p.1 对应印刷 p.3023。

### 3.1 §2.4：shuffle 输出什么、为什么需要

印刷 p.3028 / PDF p.6，§2.4 将 shuffle 定义为：输入列表 x，除了输出 secret-shared 的 π(x) 外，还把公开 masked shuffled list π(x)+r 输出给所有 online parties；r 是 n 个随机 mask，且对任意单一 party 保持未知。文中说它用于在无额外通信下，将已公开的 masked values 作为多个 FSS gate 的输入，FSS gate 的 secret parameter 是 r。

同节说明 (2+1) 与三方协议各自有对应 shuffle functionality；full version 提供正式功能描述以及改编自先前工作 [21] 的具体协议实例化。由此可直接确认“论文需要此功能”，不能直接确认“仓库现有两轮 shuffle 就是该功能的实例”：现有 SecretSharedShuffle API 只返回 secret shares，没有 π(x)+r 公开输出。

### 3.2 §5.1–5.2：图、local-rank 和安全排序器看到的信息

印刷 pp.3033–3034 / PDF pp.11–12：

- CA 模型在每轮指定比较图 H；协议只公开每个节点相对其图邻居的 local rank，并在后续迭代使用它。作者说明开始时需要 secure shuffle，以免透露原始输入之间的相对次序。
- Figure 3 / CmpAgg gate（印刷 p.3029 / PDF p.7）按图边为每个端点 mask 对生成 uCMP FSS key；在线对图边求比较 share，再按入边/出边约定对邻接比较 share 求和，得到每个节点的 local-rank share。论文允许 H 为所有可能边的任意子集，不局限于 clique。
- 对 ordered node set 和规范边方向 i<j，论文 local stable rank 可写作：

  LRank(x_i) = |{x_j : (i,j)∈E 且 x_i>x_j}| + |{x_j : (j,i)∈E 且 x_j≥x_i}|。

  等值时由 ordered vertex index 决定谁排前。这个 tie 顺序是论文图节点顺序定义，不等于仓库的原始 index，除非项目显式证明两者如何对应。
- 安全执行会让 online parties 看到 masked values 和每轮揭示的 local ranks；rank 的用途是按算法更新块/子问题，不需要把所有 pairwise comparison bit 单独公开。论文定理的单方半诚实安全主张不自动适用于把完整离线材料交给可信 T、并要求 T 永不看到 online openings 的项目三方视图。

### 3.3 §5.3：AAV86 Algorithm 1 和 CA Algorithm 2 的轮间控制

印刷 p.3034 / PDF p.12，Algorithms 1–2：

1. 算法参数为 item 数 n 和迭代数 k，列表写 p=n^(1/k)。
2. 均匀无放回取 pivot 集 P，|P|=p−1；A=[n]−P 为非 pivot，B=P 为 pivot。
3. 构造 H：A 与 B 形成完全二部图，B 自身形成 clique。每轮该 CA/AAV86 图只比较 pivot-pivot 和 pivot-nonpivot。
4. Algorithm 1 求这些边的比较结果，并按结果把 A 分到 p 个有序、不相交块 x_A^1,...,x_A^p，使较低编号块内的元素不大于较高编号块内的元素。
5. Algorithm 2 将第 4 步替换为 Compare-Aggregate 求图上的 LRank，再依据这些 ranks 将 A 分桶；其余递归控制流与 Algorithm 1 相同。若 k>1，对每个块以 k−1 递归。
6. 作者解释，正确分桶只需非 pivot 相对 pivot 的 rank、以及 pivot 之间的 rank；不必公开所有边比较结果。作者声称对该例，Valiant 模型与 CA 转换的 edge 和 node complexity 保持相同。
7. §5.3 明确把一般 CA 转换的更多讨论留给 full version。会议版算法列出“based on local ranks partition into p ordered blocks”，但没有在本文完整给出适用于项目稳定 key/endpoint handle 的精确 bucket-index 函数、消息字段绑定或逆映射数据结构。因而 CA RANK/BUCKET EXTRACTION 只能判 PARTIAL。

这里的算法目标是固定轮数随机完整排序。AAV86 §3.1 / Theorem 3.1 的期望比较复杂度不能写成项目 Top-K bit-mask 安全协议，也不证明 E1 的 FSS pool 安全。

### 3.4 §5.4 / Theorem 5.1：online 阶段和轮数边界

印刷 p.3035 / PDF p.13：

- 每个 CA iteration，online parties 公布加了离线 mask 的 x′=x+r（一次通信），调用 CmpAgg FSS gate，得到 LRank(x) 的 secret shares。
- 再用一次通信揭示 local ranks；因协议开始先 shuffle，论文称此时可以安全揭示，并允许双方按 CA 算法局部重排元素 shares，产生下一轮所需子问题。
- Theorem 5.1 对论文的 (2+1)-party 模型给出 online rounds=2k+1。定理功能为论文的 F_sort 或 F_select share 输出，不是本项目的原顺序 XOR Top-K mask。
- 论文离线通信式中与比较边直接相关的项为：

  2 e_A(n,k) · (DCF.KeySize[G,Z_n] + ceil(log n))。

  该式属于论文构造和符号模型的效率结论。正文未因此给出本项目 T 可在输入未知时，为每轮全部可能 endpoint pair 生成、关联共享节点 masks、并在图确定后认证查表的完整工厂。不能把 e_A 项直接改写为 E1 的全池成本，也不能声称全池已继承 Theorem 5.1 的安全证明和通信边界。

论文 2+1 模型中的 dealer 提供 offline correlated randomness 后不参与 online。项目已确认的模型是 T 可信、可见全部离线材料且不与 online 方合谋；仍需证明 T 不获取在线 openings/transcript，且在线单方 view 与材料相关性满足项目安全目标。模型之间不能只凭角色名视为相同。

## 4. 论文变量与项目变量的身份映射

| 身份/阶段 | 论文对象 | 当前项目对象 | E4 判断 |
|---|---|---|---|
| 原始输入位置 | 初始输入列表中的有序位置 i | raw score input slot / original_index，范围 0..logical_n−1 | 项目输出 mask 必须回到这里；这是项目输入身份 |
| 稳定 priority key | 论文对 ordered set 的稳定顺序；CCS 论文没有项目 Q20.12 编码 | score 降序并以 original_index 升序稳定 tie；raw-score adapter 把 slot index 编入 priority key 低位 | 项目契约；不是论文给出的编码 |
| 初始 shuffle 后 endpoint handle | 论文图 H 的 ordered node index | 现有 SecretSharedShuffle 后向量 slot；接口没有返回公开 handle→original_index 映射 | 不能把 handle 当作 original_index；若比较 key 唯一，则还须证明该稳定字段随记录同置换 |
| 每轮 pivot / 子问题 | P、A、每个递归块及图端点 | E1 明文计数器中的 shuffled handle 和递归 snapshots | E1 是 TEST_ONLY 明文图，不是 CA runtime 图或秘密安全 bucket 状态 |
| bucket index | Algorithm 1/2 中由 pivot 次序及比较/local ranks 所定的有序块编号 1..p | 当前生产 Protocol I 没有 AAV86 bucket API；pipeline 是单个固定 padded clique | 精确的项目 bucket 更新尚未映射 |
| final sorted position | F_sort 中完整排序位置 | 现有固定 pipeline 以公开全局 rank 构造 rank<K carrier；C-INSTANTIATION core 输出 sorted shares | 这些路径是固定 clique 排名，不是递归 CA sorter |
| 原顺序输出 mask 位置 | 论文没有给出本项目 bit-mask 适配器 | 每个 original_index 对应一个 XOR bit，恰 K 个 1 | 需要 final membership carrier 与同一初始 shuffle 的 inverse permutation；当前 AAV86 适配证明缺失 |

E1 的 n=5 fixture 用了非恒等的 handle→original 映射，但该映射处于明文 TEST_ONLY 生成器，不证明 secure shuffle 的 party view、映射不泄露或项目 pipeline 的 payload 路由。

## 5. Protocol I 实际接口与 CA 目标边界

| 路径 | 当前源码可确认的输入/输出 | 置换、打开值与轮数 | 与 CA 的差别 |
|---|---|---|---|
| 两轮 SecretSharedShuffle | 每方传入 ProtocolIBlock192 shares 和本方 party material，输出被置换的 shares。Block192 可以承载三个字段，但调用者决定字段语义。 | forward 2 轮，reverse carrier 2 轮；party material 保存 own_permutation、own_inverse_permutation 及四组一次性 Permute+Share 材料。该接口没有返回论文要求的公开 π(x)+r。 | 可以作为同记录搬运的工程构件；不等于论文 §2.4 secure-shuffle functionality，也无目标 CA、edge-mask/key pool 和输出 adapter 的联合证明。 |
| 三轮 C-INSTANTIATION shuffle/CmpAgg core | 输入 Block192 secret shares；阶段输出包括 shuffled_share、public_masked_records、public_ranks、sorted_share。CmpAgg 实际比较 word0。 | round 1/2 置换/份额消息，round 2 得 public masked records，round 3 exchange rank shares 后重构 public ranks 并产生 sorted_share；计数为 3 轮。party material 含 sigma/tau、mask/share 和 edge materials；dealer factory 生成 correlated material。 | 当前 CmpAgg 收齐并按 padded domain 全 clique 遍历 edge materials。它不是每个 AAV86 活跃子图的 local-rank/bucket API；C-INSTANTIATION public masked records 也没有证明等同于论文同一 π 下的 π(x)+r。 |
| fixed-clique priority pipeline | 输入 padded priority-key additive shares、package、shuffle material、forward/CmpAgg/rank-reveal/reverse channels；输出 logical_n 长度 XOR mask shares。 | 先把每条记录构造成 {key_share,0,0}；forward shuffle 2 轮；交换 masked key share 并做 clique CmpAgg 1 轮；公开重构每个 shuffled slot rank 1 轮；生成 party 0 的 rank<K carrier 后 reverse shuffle 2 轮。pipeline 为 6 轮，连同 raw-score adapter 2 轮共 8 轮。 | 已有 output carrier 回原序路径只对应本 pipeline 固定 clique 的完整置换材料。它没有 CA 的每轮 pivot/bucket/子问题状态，也未携带项目额外 payload 字段；不能将 6/8 轮数转记为 CA 或 Theorem 5.1 的项目轮数。 |
| 目标 CA + E1 全两两池 | 期望输入为稳定 priority record shares、初始 hidden shuffle、每轮 CA 图端点/掩码和 party-local DCF slots，输出应能形成排序 membership 并回原始位置。 | 论文因果候选为初始 shuffle + 每轮 masked open / CmpAgg / rank reveal / 局部 reordering；E1 只在单进程 TEST_ONLY 环境按给定图查真实 key。 | 生产 Protocol I 没有此完整 API、在线 transcript 或端到端 inverse map。必须单独证明图、key、mask、同一 π 和 T/P0/P1 view 的绑定。 |

源码可确认的细节：

- raw-score adapter protocol_i_raw_score_input_party 接收 logical raw score additive shares、package、两条 stage fds，输出 padded_n priority-key shares；两轮 carry/sign exchange。padding 由 party 0 注入 signed INT32_MIN、party 1 注入 0。
- pipeline 的 protocol_i_make_input_layout 将 logical_n pad 到 max(2,next_power_of_two(logical_n))；pipeline 使用 padded_n，而 E1 fixture 是无 padding logical domain。目标 CA 使用 n 还是 padded_n 仍须定义并证明。
- fixed pipeline 将 original slot tie component 编入稳定 key；它的记录载体 word1/word2 均置零。因此该调用没有额外传递原始 payload 或独立 index 字段。
- CmpAgg 源码要求 edge_materials.size() 精确等于 C(padded_n,2)，再按所有 left<right pair 遍历；它不接收 AAV86 active-edge list 或 pivot/bucket 状态。
- 两轮 shuffle header 将 forward 与 reverse 描述为四组 Permute+Share one-shot material；当前函数各方持有自己的置换和逆置换。置换工具约定 apply(pi,x)[i]=x[pi[i]]。这些事实不能替代跨协议相关随机性、同一全局 π、masked list 和 view 的证明。

## 6. “同一个 π”与 final inverse-map 审查

要复用 Protocol I 的 shuffle，需要证明至少这些对象处于同一记录/同一映射：

1. score 与 project stable original_index 组成的 priority key；
2. CA 每轮 endpoint handle 和每轮对应的节点 mask share；
3. E1 pool 中按 unordered endpoint pair、iteration 绑定的 DCF party key；
4. 论文 shuffle 对外公布的 masked shuffled list；
5. CA 局部排序、桶更新后能返回 membership 的 carrier；
6. 将 membership 经同一初始置换的 inverse 映回 original_index 的输出 bit-mask。

当前证据只分别覆盖了较窄的接口事实：SecretSharedShuffle 能搬运 Block192 shares；C-INSTANTIATION 可返回一个固定流程的 public masked record/rank；fixed-clique pipeline 能对它自己的 rank<K carrier 调用 reverse shuffle；E1 在单进程中为 n=5、r=2 的明文 shuffled handles 生成/消费真实底层 key。

未找到一份把上述六项连在一起的代数证明或源自论文的实例化，尤其没有证明：

- 两轮 SecretSharedShuffle 的复合置换与论文 §2.4 的 π 及 public masked list 具有同一联合分布；
- E1 每轮节点 masks 与该 shuffle 的 endpoint handle 一一对应，且所有 incident-edge DCF keys 的相关分布对单个 online party 可模拟；
- priority-key、任何 payload 和 rank/bucket carrier 在每次局部 reorder 后保留可逆的原始身份；
- C-INSTANTIATION 的 masked records/ranks 能用于论文 CA 算法的 adaptive H_t，而不是固定 clique；
- inverse route 将前 K membership shares 恰好放回项目原始槽位，且不公开排序/原始 index。

因此判定为 UNPROVEN；当前协议组合实现门为 NO-GO。这里的 NO-GO 是“现有证据不足以批准组合实现”，并非证明该组合不可能。

## 7. E1 全两两池与论文 e_A 离线成本不是同一资源口径

| 口径 | Agarwal Theorem 5.1 | E1 / Option 1 全两两实验 |
|---|---|---|
| 比较边数 | e_A(n,k)：给定 CA 算法在其实际输入/随机运行中的 edge complexity；定理离线比较材料项按 e_A 写出 | 固定 endpoint 域 D、每轮预留任意简单图的全部 C(D,2) 无序 pair，共 r·C(D,2) 个 party-local key slots |
| key 数 | 论文式中的边相关材料项为 2·e_A·(DCF.KeySize+ceil(log n)) bits，另有 masks、payload、shuffle 和其他离线项 | 每方 r·C(D,2) 槽，双方合计 2r·C(D,2) 个 party key blobs；E1 serializer 还包含每轮 mask shares、槽位字段和 wrapper header |
| 生成时机/图依赖 | 会议版 theorem 给符号成本，但没有在可核页面中展示项目所需的、输入/递归桶图未知时精确构造整套自适应 edge pool 的工厂 | pool 在加载 active graph fixture 前生成；后按 iteration/endpoint 查表。该“预备所有 pair 再稀疏消费”是项目扩展策略 |
| 空槽成本 | 不由 e_A 的活动边项表达为全 pair 预留容量 | 所有未使用 pair 仍占离线 key 存储/生成/发放成本 |
| 安全边界 | theorem 的 security proof 与论文 shuffle/FSS 构造相连 | E1 wrapper 在单进程持有双方密钥和 oracle；没有全池单方 view、共享 mask 多边相关性或 adaptive lookup 模拟 |

E1/E2 报告中的实验 serializer 长度公式为每方 63 + 8rD + r·C(D,2)·(20 + L_key(b)) bytes；其 921 bytes/key、18,963 bytes/party、双方 37,926 bytes 仅是 n=5、r=2、b=36 样本。该数值不含网络 framing、TLS、完整 CA transcript 或生产 package；不得作为普遍成本。

如把 all-pairs 只表述为“项目为节省在线比较与计算而预先提供候选材料”的扩展，正式计量至少并列列出：

- Offline：材料生成 CPU/时间、每方峰值内存与保存字节、T 向 P0/P1 的实际交付 bytes、离线随机性和失败/重发成本。标明 D、r、bits、材料生成版本和输入 seed。
- Online：每轮实际 active edges、端点/节点数、打开值 bytes、rank reveal bytes、party 双向通信、CA reordering 工作、在线 rounds 和 wall time。
- Total：对实际单次执行说明 offline 与 online 的字节/计算/时延合计；若给多次执行均摊，必须报告同一池可用次数和 one-shot 消耗证据。E1 keys 为一次性消费，未经证明不得跨执行摊销。
- 论文列与项目扩展列分开；论文理论 e_A 项不得替代实测/推导全池成本。任何未测值写 NOT_MEASURED，不能用 E1 计数器或旧性能结果补齐。

## 8. 逐门判定

| Gate | 状态 | 理由 |
|---|---|---|
| AUTHOR_SOURCE | VERIFIED | Elette 作者主页直接列出本文及 CCS’24 PDF；本地会议版内容核对完成。 |
| FULL_VERSION | UNRESOLVED | 正文多处指向 full version，MIT DSpace 列出 ZIP 但无法取件；附件内容和完整作者稿是否可获得未核验。 |
| CODE | NOT_FOUND_IN_SOURCES_CHECKED | 本次查过的作者主页/本文条目、作者机构页面、MIT/ACM 页面没有找到可归属的对应作者实现。 |
| CA RANK/BUCKET EXTRACTION | PARTIAL | local-rank 公式、pivot graph 和高层 rank→分桶因果已核；具体 bucket-index 更新细节和适用完整构造留有 full-version 缺口。 |
| PROTOCOL I SHUFFLE COMPATIBILITY | UNPROVEN | 没有 same-π、共享 mask/edge key、公开 masked list、CA reorder 与 inverse-map 的联合证明。当前证据不足以批准组合实现。 |
| OPTION1 ALL-PAIRS | EXPERIMENTAL CANDIDATE | E1/E2/E3 支持固定域小样本功能及槽位可用性研究；尚非 design-ready、安全材料方案或生产接口。 |
| FULL-POOL PARTY-VIEW SECURITY | UNPROVEN | 缺少共享节点 mask 导致的多边 key 相关性、自适应 endpoint 查询/访问 pattern、错误与重放视图的单方模拟证明；单个 DCF key 的功能正确不推出整池安全。 |
| P2-I SECURE RUNTIME | NO-GO | 论文原文和 E1 实验均未补齐组合证明、T/P0/P1 view、在线 CA 与原顺序 mask adapter；保持 E3 结论。 |

## 9. 需要论文/协议 owner 补充的精确问题

以下问题整理给用户转交，本轮没有发送给任何人。

1. 能否提供作者或正式仓储可访问的 full-version PDF/supplement 原始 URL、版本说明和 SHA-256？它是否包含 §2.4 两个 shuffle functionality 的定义/具体实例化，以及 §5.3 bucket transformation / §5.4 Fsort 完整协议？
2. Algorithm 2 中从 pivot local ranks 和 nonpivot local ranks 到 p 个 bucket 的完整规则是什么？请给出具体定理、公式、页码或完整算法，尤其包括重复值、端点顺序和非整数 p 时的约定。
3. 论文的 π(x)+r shuffle 实例中，两方分别持有哪些 π 份额、r 份额及 payload shares？公开 masked list 的发送者/观察者和安全模拟分别是什么？
4. 是否存在可引用的证明说明仓库两轮 Protocol I SecretSharedShuffle 与该论文 functionality 同分布，包括同一 π 同时作用于 priority key/payload、公开 π(x)+r 和 final inverse mapping？若存在请提供 construction/proof 的文件与章节；若不存在，明确需要新协议。
5. 若使用项目的 all-pairs DCF pool，论文中 CA 每轮 node mask 与每条 endpoint-pair key 如何共同生成和相关？哪些 endpoint、图访问、pivot/bucket 和 rank transcript 对 P0、P1、可信 T 分别可见？
6. 论文 Theorem 5.1 的 e_A 离线项对应何种生成调用和图边可用时间？是否存在正文之外的实现级 adaptive preprocessing 步骤，能供本项目引用？不要只给复杂度公式；需要具体算法/接口及对应安全证明。
7. 能否提供与本文版本对应、由作者账号或正式 artifact 归属确认的源代码仓库/commit，并指出 CA/AAV86/secure shuffle 的实现入口？
8. 对本项目的目标函数，是否有可证明的 full sort 输出 membership-share → 原输入 index XOR bit-mask 适配器？请给出不打开 score、stable index 或全排序结果的身份追踪和 inverse-permutation 证明。

以上不要求改变已确认项目模型：T 可信并可见全部离线材料，且默认不与 online party 合谋。待补的是该项目 view 与论文 view 的映射、T 不接触 online transcript 的协议边界及具体证明。

## 10. 下一步建议和停止条件

建议先取得第 9 节中正式来源和 CA/shuffle 细节，再完成逐阶段映射审查。进入任何 secure runtime 之前，应有可独立审查的证据覆盖：精确定义的 CA bucket 更新、论文 shuffle 到 Protocol I 的同一 π 兼容性、全池单方 view 安全、T 的离线/在线隔离、stable key 与 payload 的身份跟踪、以及 final membership 到原顺序 XOR mask 的逆映射。重新计算 online rounds 时须从 sender/receiver/依赖 transcript 出发；不得直接沿用 2k+1、3 或 6/8 的其他边界。

停止条件：

- full version 或 supplement 仍无法取得且其中细节影响接口时，相关语义保持 PARTIAL/UNRESOLVED；
- 未证明 same π 和 mask/key 联合分布前，不能复用现有 shuffle 并进入协议组合实现；
- 未证明全池 party view 和 T 不接收 online masked openings/ranks 前，安全门保持 UNPROVEN；
- 未定义 padding domain、stable tie 和原序 inverse mask 的一致性前，功能输出门不通过；
- 在所有上述 blocker 关闭并经独立安全审查前，P2-I SECURE RUNTIME 继续 NO-GO。

## 11. 文件差异与执行记录

本轮仅新增：

- docs/reviews/M6A_P2_I_E4_CA_SOURCE_AND_SHUFFLE_COMPATIBILITY_2026-10-02.md（本报告）

没有修改主工作区；没有修改 VFSS/、VFSS-baseline/、测试、E1/E2/E3 历史报告、E1 实验文件、Papers/、参考工程、项目计划或基线目录。作者 PDF 的临时核验副本保存在系统 Temp，不属于仓库差异；根目录 siamjdiscrmath.pdf 保持不动。

本轮只做静态源码阅读、PDF 文本抽取和官方网页来源核对；未运行测试、build、runtime harness、benchmark；未暂存、提交、推送、合并、创建 PR 或切换分支。主工作区既存 dirty 状态与 E4 开始时相同。
