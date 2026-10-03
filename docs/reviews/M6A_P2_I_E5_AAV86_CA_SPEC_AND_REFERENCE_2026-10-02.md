# M6A-P2-I-E5：AAV86/CA 算法规范闭合与 TEST_ONLY 参考实现

日期：2026-10-02
执行位置：隔离实验 worktree
分支：codex/m6a-p2-i-allpairs-experiment
基线 / 起始及结束 HEAD：c3926c68fd14f270faa8b55234311071947fa080
起点核对：符合任务预期。
本报告只记录算法证据、项目决策、TEST_ONLY 明文参考和 E1/v2 图计数；不批准安全协议组合或 secure runtime。

## 1. 执行范围与工作区保护

开始和结束时，实验 worktree 均位于预期分支和 HEAD；本次未 checkout、reset、stash、清理或覆盖文件。实验 worktree 中已有 E1–E4 报告、设计稿、代码与 CSV 均保留。本次只新增本报告及下列 TEST_ONLY 文件，没有改写 E1、E2、E3、E4、旧计数器或旧 CSV。

主工作区仍位于 feat/m6a-performance-evaluation，HEAD 也是 c3926c68fd14f270faa8b55234311071947fa080。开始和结束时，主工作区保持原有差异：PROJECT.md、docs/IMPLEMENTATION_PLAN.md、docs/PAPERS.sha256、docs/REFERENCE_MANIFEST.md 四个 tracked 修改；P1/P2 决策文档、P0 盘点文档和根目录 siamjdiscrmath.pdf 仍为未跟踪文件。没有在主工作区写文件。Agarwal 与 AAV86 论文 PDF 只读自 Papers/；没有把论文加入本次差异。根目录 siamjdiscrmath.pdf 保持原样，未作为 BB90 论文正文或本阶段材料使用。

实验 worktree 起止 Git 状态均只显示此前已有的未跟踪 E1–E4 材料与 experiments/；本次没有 tracked 文件修改。未暂存、提交、推送、合并或创建 PR。

## 2. 输入材料及 SHA-256

本轮的项目状态依据仓库决策/设计记录、E1/E4 审查、原始论文和当前 E1 文件。用户给出的任务文本记录学姐答复；未提供独立聊天原文、答复时间戳或截图，因此只登记为本轮收到的项目确认，不补造其原始出处时间。

| 材料 | SHA-256 |
|---|---|
| 用户 E5 任务文本（记录项目模型与路线） | 13B73F24B41BF7A6106C810563A6FA6A3FCC4397AC35CB1CB00E3DB9E0126117B |
| M6A P1 算法与稳定顺序决策（主工作区，只读） | 21DE1BE89CFCC7B66005B8E4F34BCAFFBC7556CCF22FAE7F517FEC91681456C0 |
| E3 全两两设计门（实验 worktree，只读） | CE43F664BADC664A918C919EEDA889B6586C8EA224B0CF0D512712A914906965 |
| E1 独立复审报告（只读） | 307B80ABC789F2C1EB0614992A4E2C4938A7AB51BB3BCB089951D713C6C23ED9 |
| E4 CA 来源与 shuffle 兼容性审查（只读） | 3312ED84FD5EDEE2F3766C791186CABF297BB73E0565BD06E93E2FCA588E35FD |
| E1 原 Python 计数器（未修改） | 09DCC72DF6BC68C83046CBB3F7C9AF8B13642BEA720036B92804F6BE5423B402 |
| E1 原计数 CSV（未修改） | 5346AE011201C1F88F3B7199BB86DE7C1B4F761151F3FB269C86F9E99AF9F622 |
| E1 n=5,r=2 fixture CSV（未修改） | 87A660C55B47FBCD1AAAE6C4E025798E8E4B6853968DC6D27DF635A704B46D78 |
| Agarwal CCS’24 本地会议版 PDF，15 页 | 18FAF63EAA7923EEF715A6EB9D5D526FE04DCB69700B133C3E94DE935F68C01C |
| AAV86 作者托管本地 PDF，9 页 | 322F1BD761A987FD09E6B59B3A3AE77D6E4B1CA9DCB1C2765E45AC4A2A7E2B83 |

Agarwal 本地会议版来源登记继续沿用 E4：DOI [10.1145/3658644.3690359](https://doi.org/10.1145/3658644.3690359)，MIT DSpace [publication record](https://dspace.mit.edu/entities/publication/d70bd196-cf7c-46e1-a0a9-e3dfd08f379b)。AAV86 书目来源为 IEEE DOI [10.1109/SFCS.1986.57](https://doi.org/10.1109/SFCS.1986.57)；作者托管 PDF：[Tight Complexity Bounds for Parallel Comparison Sorting](https://web.math.princeton.edu/~nalon/PDFS/Publications2/Tight%20complexity%20bounds%20for%20parallel%20comparison%20sorting.pdf)。

此前 E4 的正式来源核对状态在本阶段沿用：Agarwal full version/supplement 仍为 UNRESOLVED；已检查来源中未核实到可归属作者的对应代码。E5 没有重复扩大检索。主机上的 Agarwal_TopK/ 与 ADSMPC/ 参考树存在，但其当前 Git remote 指向项目仓库 05T1925/MoE_Top-k_Reproduction_Evaluation.git；仅凭目录存在不能把它们认定为 Agarwal 作者发布实现。

## 3. 学姐确认的项目模型与材料路线

以下为项目设计/威胁模型确认，来源是用户本轮转交的任务文本，记录日期为 2026-10-02；不是 AAV86 或 Agarwal 论文结论：

1. 当前 Protocol I 方向采用每轮固定 endpoint domain 的 all-pairs 离线材料池：为所有无序端点对预生成候选材料，在线只消费活跃图边对应槽位。同轮多条边可引用同一端点 mask；边 key 与对应端点 mask 差相关。
2. 固定 M 仍为延期备选；E5 不实现、不切换、不重新评价固定 M。
3. P0/P1 在线方按半诚实模型。
4. T 为可信第三方，能够看到全部离线随机量、密钥、置换和两方材料；T 只负责离线生成与分发，之后退出，不接触在线输入、排名或在线 transcript。默认 T 不与任一在线方合谋；不要求擦除 T 保留的状态。

该模型既不是全池相关 key 的联合安全证明，也不自动继承论文的 (2+1)-party 安全结论。继续保留：

- FULL-POOL PARTY-VIEW SECURITY = UNPROVEN
- PROTOCOL I SHUFFLE COMPATIBILITY = UNPROVEN
- P2-I SECURE RUNTIME = NO-GO

## 4. 论文算法规范与适用边界

### 4.1 AAV86 原始固定轮排序

来源：Alon、Azar、Vishkin，FOCS 1986，§3.1 / Theorem 3.1，印刷 pp.506–507 / 本地 PDF pp.5–6。定理输入是 n 个全序元素；固定 k≥1 轮，输出完整排序，期望比较数至多 c(k)n^(1+1/k)。该期望界不是高概率界、最坏情况界或项目协议性能数字。

每个当前活动子问题的算法骨架可精确写成：

    SORT(V, d):
      m = |V|
      t = ceil(m^(1/d))
      q = t - 1
      均匀无放回抽 q 个 pivot P
      建图：P 内 clique；P 与 V\P 之间 complete bipartite
      比较图上所有边
      将 V\P 按 pivot 全序划分为 t 个有序、不相交桶 B[0..q]
      若 d > 1：并行 SORT(B[i], d-1)，跳过空桶
      按 B[0], P[0], B[1], ..., P[q-1], B[q] 合并

参数约定：对 m>1,d≥1，t=ceil(m^(1/d))，q=t−1；q 是当前子问题的 pivot 数，不是原始 logical_n 的全局常数。d=1 时 t=m、q=m−1，图是该子问题上的完整图；仍可按 pivots 建立最终桶，但不再递归。论文把 k=1 的全排序视为平凡基例。m=1 时的直接携带、空桶不进入递归，是本 TEST_ONLY 实现采用的无操作边界规则；论文伪代码没有单独展开这些调用约定。合法入口为 logical_n≥1、r≥1；空输入不在此参考实现的输入域内。

### 4.2 Agarwal CCS’24 的 CA 图和 local rank

来源：Agarwal 等，CCS’24 会议版，§5.1–5.4，Algorithms 1–2，印刷 pp.3033–3035 / PDF pp.11–13。会议版 §5.3 的 Algorithm 1 给出 AAV86 的 pivot 图与分桶控制流；Algorithm 2 把边比较结果替换成 Compare-Aggregate local ranks，并保留相同递归控制流。Algorithm 2 文字称根据 local ranks 把 nonpivot 分成 p 个有序块，没有把适用于本例的桶下标单独写成公式。

§5.2 对有序节点与 canonical edge i<j 给出的 non-distinct local stable rank 为：

    LRank(x_i)
      = |{x_j : (i,j)∈E 且 x_i > x_j}|
        + |{x_j : (j,i)∈E 且 x_j ≤ x_i}|.

这第二项方向以会议版 PDF 原文为准。E4 报告中的 “x_j ≥ x_i” 是转录错误；E5 将公式更正为 “x_j ≤ x_i”。P1 决策文档已经记载正确方向。本轮没有改写 E4 历史审查文件。

### 4.3 CA rank 到桶编号的来源推导

在 AAV86 图中，每个 nonpivot 的邻居集合恰好是本子问题的 q 个 pivot。因此它的 CA local rank 是排在它前面的 pivots 数，唯一确定为 bucket index 0..q；不需要所有 pivot/nonpivot 比较位单独公开。bucket i 的元素位于 pivot i−1 与 pivot i 之间（端点桶省略缺失的左/右 pivot）。

每个 pivot 与子问题里其余全部顶点相连，因此它的 LRank 是该 pivot 在当前子问题稳定全序中的精确位置；pivot 相互之间的 rank 给出 pivot 顺序。将各 bucket 与 pivot 按序交错即可恢复本层顺序，再对非空 bucket 递归。

这是从 AAV86 的 pivot 图、Agarwal §5.2 的 local-rank 定义和 Algorithm 1/2 的有序分块目标得出的 PAPER_DERIVED 规则。会议版没有印出 “bucket index = LRank(nonpivot)” 这句独立公式；推导成立的条件是子问题比较对象具有唯一全序。它闭合了 TEST_ONLY 算法参考的 rank/bucket 语义，不等于作者给出的项目级稳定键编码，也不弥补 full version 所含一般 CA 转换讨论的来源缺口。

### 4.4 项目稳定序、endpoint handle、padding 和输出

P1 决策冻结的项目顺序是 signed Q20.12 score 降序、original_index 升序。E5 只在明文参考中用 tuple priority (-score, original_index) 表示这个全序；original_index 唯一时不存在相等 priority。它属于 PROJECT_CONTRACT/TEST_ONLY oracle，不是 AAV86 的 score 编码、CA secure field 或论文定理。

TEST_ONLY 中的 endpoint handle 是当前 shuffled vector 的局部整数 ID；它与 original_index 是分开的字段。图端点只从当前逻辑活动子问题的 handles 中选取。Top-K mask 检查把排序结果映射回 original_index，只用于明文 oracle；没有实现安全逆置换或生产 membership-share adapter。

当前 Protocol I pipeline 的 padding 规则在 E4/源码记录中为 max(2,next_power_of_two(logical_n))。该工程 padding 不代表 dummy slot 应加入 AAV86 的随机排序域。v2 将 logical_n 作为主算法域，另写 padded_n_informational 和条件性的 padded pool 容量；后者是容量算术对照，不是算法或生产配置决定。

## 5. 明文 TEST_ONLY 参考程序

新增 experiments/m6a_p2_i_allpairs/TEST_ONLY/aav86_ca_reference_TEST_ONLY.py 和 README_TEST_ONLY.md。代码定义：

- Record：signed integer score 与 original_index；handle 作为 records 字典键。
- RunContext：固定 r、pivot_seed、每轮统计、可选边枚举及子问题轨迹。
- 子问题轨迹：轮次、剩余深度、父子路径 ID、顶点、pivot/nonpivot、local rank、pivot 顺序、桶、图边和终止状态。
- 纯逻辑函数：整数精确 ceil_nth_root、pivot 图边生成、图结构优化的 local-rank 求值、完整递归排序及明文 Top-K mask。

对于矩阵大 n，代码按已推出的邻接结构优化计算 local rank：pivot 的 rank 由完整当前子问题全序确定；nonpivot 的 rank 通过有序 pivot 序列的 lower-bound 得到。self-test 在小图中另用逐边定义独立重算 local ranks，与优化路径逐个节点对照。所有这些值都在一个明文 Python 进程内可见。

程序没有 FSS keygen、party material/pool、网络协议、Dealer、shuffle 替代、rank reveal 消息、secure bucket update、原序安全 mask adapter，也没有接入 VFSS/。它不证明算法协议组合、隐藏 shuffle、材料相关性或 party-view 安全。

## 6. E1 计数器的 v2 修订

E1 原文件保持不变。v2 新增每轮文件 aav86_ca_counts_E5_v2_TEST_ONLY.csv、每 run 文件 aav86_ca_runs_E5_v2_TEST_ONLY.csv 和 n=5 trace 文件 aav86_ca_fixture_trace_E5_v2_TEST_ONLY.csv。

### 6.1 字段与口径更正

| E1 旧项 | E5/v2 对应 | 修正说明 |
|---|---|---|
| graph_oracle_edges(snapshots, r) | trace 中按 pivot predicate 枚举小图边，并逐边校验 CA rank | E1 oracle 只针对既有 snapshots；v2 self-test 独立检查完整小规模递归轨迹。大 n 不保存全部边。 |
| active_subproblems | subproblems_entered | 都是该轮真实进入的非空子问题调用，包含 singleton carry。 |
| nontrivial_subproblems | graph_subproblems | 大小至少 2、实际建立比较图的子问题。 |
| logical_nodes | logical_vertices_in_graph | 本轮所有比较图中的逻辑顶点数之和。 |
| empty_buckets | nonterminal_empty_bucket_slots / terminal_empty_bucket_slots | v2 在桶被创建的轮次记数，并分开非终止层与最终层；空桶不递归。E1 将空子调用记在下一轮行，v2 比较器按这个轮次偏移核对。 |
| same_round_recursive_edges | 已从 v2 移除 | E1 旧代码从未给它赋值，默认 0 不是观测结果。 |
| duplicate_edge_check | edge_audit_basis | n≤32 的每个矩阵 run 实际枚举并检查同轮/跨轮重复；大 n 使用结构论证标签，不伪装成逐边执行检查。 |
| M_observed_sample_max | max_active_edges_observed | 仍是每个有限 seed run 的各轮最大观测值，不是 worst-case bound。 |
| pool_capacity_formula_slots | logical_pool_slots_per_party_full_protocol | 每方容量 r·C(logical_n,2)，每轮 C(logical_n,2)。活跃边只计入 used slots。 |

对结构论证：同轮各非空桶两两不相交，因此其图端点不交；每轮图边至少有一端是该轮 pivot，而 pivot 不进入后续桶，所以任何先前边的 pivot 端点不会留在后续活动子问题中，跨轮边不重复。该论证只说明边唯一性和一般性容量关系，不把样本最大值升级为更紧的最坏情况比较数上界。

### 6.2 复跑矩阵

使用 E1 同一组 n=[1,2,3,4,5,8,16,32,128,256,1000,10000,100000]、r=[2,3,4,5]、(input_seed,pivot_seed)=[(1,1001),(7,1007),(42,1042)]。保留 E1 的 Python random 采样和 depth-first 非空桶消费顺序。v2 输出 156 个 run、546 个逐轮行；与 E1 旧 CSV 对照 PASS：所有可比轮边数、pivot 数、图顶点数、子问题数、singleton carry、空桶偏移、run 总边数、样本 observed maximum 和容量/已用/未用槽均一致。v2 计数不是性能 benchmark。

n≤32 的 96 个 run 实际保留并枚举逐边记录、做同轮及跨轮重复检查；更大 n 使用递归结构论证，不保存二次级图边表。fixture 重跑得到 n=5,r=2 每轮边数 [7,1]，每方逻辑容量 20、活跃槽 8、未用槽 12。固定 M 没有评估或实现。

## 7. 验证与复现记录

环境：Python 3.13.7；worktree revision c3926c68fd14f270faa8b55234311071947fa080。只运行了用户明确允许的 TEST_ONLY 定向检查和矩阵计数，没有运行仓库测试或完整性能矩阵。

    python experiments/m6a_p2_i_allpairs/TEST_ONLY/aav86_ca_reference_TEST_ONLY.py self-test
    SELF_TEST PASS exhaustive_cases=1224 random_cases=120 tie_fixture_runs=12

该检查覆盖所有 n≤5 的 rank permutation、r=1..4 与两组 pivot seeds；另覆盖 n∈{2,3,5,8,13,32}、4 组 input seeds、r=1..5 的多 seed 样本，以及同分、signed int32 边界值夹具。seed 记录：穷举 pivot_seed 为 17+n、9001+n；随机部分 input_seed 为 1、7、42、20260930，pivot_seed 为 50000+101*n+7*input_seed+r；同分夹具 pivot_seed 为 771+n。失败配置为无；任一断言失败会非零退出。每例比较完整排序与独立明文 stable-sort oracle；验证图端点属于当前逻辑子问题、pivot/nonpivot 图完整、边无重复、逐边 LRank 与优化 LRank 相同、桶不重叠且覆盖全部 nonpivots、递归子问题严格缩小、同轮子问题不交，以及 K=1/中间 K/n 的明文 mask 数量与 membership。

    python experiments/m6a_p2_i_allpairs/TEST_ONLY/aav86_ca_reference_TEST_ONLY.py fixture
    FIXTURE PASS logical_n=5 r=2 edges=[7, 1] stable_sort/mask=PASS trace_rows=3

    python experiments/m6a_p2_i_allpairs/TEST_ONLY/aav86_ca_reference_TEST_ONLY.py matrix --compare-e1
    E1_COMPARE PASS rows=546 runs=156

fixture 记录 stable_sorted_handles 与原序 K=1 mask，但仅为明文 oracle。验证通过只说明该参考算法和已定义计数器在上述域内相符；不代表 FSS key 安全、随机 shuffle 隐藏、T/P0/P1 视图安全、生产消息轮数闭合或完整协议正确。

## 8. 泄露与安全组合边界

论文 §5.4 / Theorem 5.1（印刷 pp.3034–3035 / PDF pp.12–13）描述其 shuffle 条件下的 masked-input 打开、每轮 CA local-rank 分享与揭示、以及后续本地重排；论文模型中的揭示许可不能直接移植成当前项目 T/P0/P1 安全证明。

若本项目在线方或可观察方获得 local ranks、bucket membership、pivot 标识或活动边列表，这些数据会描述当前节点相对 pivots/邻居的位置，揭示递归分区以及随输入变化的访问图模式。endpoint/材料查表模式也可能可见；其精确可见者取决于后续协议接口，当前没有为它定义完整视图。论文的安全分析没有在本阶段被重述为“已覆盖共享节点 mask 的跨边相关 DCF key、all-pairs pool 自适应查询、项目 T 保留材料的视图或项目原序输出逆映射”。

还未闭合的实现/安全问题包括：full-pool party-view 模拟（含跨边 key 相关性和自适应访问模式）、论文 shuffle 与当前 Protocol I 是否具有同一 π/同一记录搬运分布、T/P0/P1 对离线与在线 transcript 的分离证明、padding 参与语义、稳定字段和 payload 在局部重排中的可逆身份跟踪、full-sort membership 到原始槽位 XOR mask 的安全逆映射、消息/材料消费顺序与错误终止、项目端到端轮数。以上均阻止 secure runtime。

## 9. 结论状态与 P3/P4 门槛

- AAV86 原始 pivot 图、参数和递归排序：PAPER_DIRECT 闭合（§3.1 / Theorem 3.1）。
- Agarwal CA 对 AAV86 图的 local-rank/bucket 参考语义：PASS（§5.2/§5.3 原式加唯一全序前提下的 PAPER_DERIVED 桶映射）；一般 CA 转换、作者 full version 和作者代码状态仍 PARTIAL/UNRESOLVED。
- TEST_ONLY REFERENCE = PASS（明文功能/计数范围内）。
- E1 COUNT SEMANTICS = CORRECTED（E1 原件保留，v2 独立输出）。
- FULL-POOL PARTY-VIEW SECURITY = UNPROVEN。
- PROTOCOL I SHUFFLE COMPATIBILITY = UNPROVEN。
- P2-I SECURE RUNTIME = NO-GO。

后续只有在安全视图/泄露模型、all-pairs 相关材料证明、shuffle 与同一 π 的组合论证、rank/bucket 消息顺序与错误行为、padding 域、稳定身份搬运、原序 mask 逆映射和端到端计量边界均可审查之后，才重新进入 secure runtime 设计门。E5 不建议或冻结新的 CA 协议、AAV86 版本、稳定键编码或固定 M 方案。

## 10. 新增文件与结果校验

本次新增 6 个文件，无既有文件修改：

1. docs/reviews/M6A_P2_I_E5_AAV86_CA_SPEC_AND_REFERENCE_2026-10-02.md（本报告）
2. experiments/m6a_p2_i_allpairs/TEST_ONLY/aav86_ca_reference_TEST_ONLY.py
3. experiments/m6a_p2_i_allpairs/TEST_ONLY/README_TEST_ONLY.md
4. experiments/m6a_p2_i_allpairs/TEST_ONLY/aav86_ca_counts_E5_v2_TEST_ONLY.csv
5. experiments/m6a_p2_i_allpairs/TEST_ONLY/aav86_ca_runs_E5_v2_TEST_ONLY.csv
6. experiments/m6a_p2_i_allpairs/TEST_ONLY/aav86_ca_fixture_trace_E5_v2_TEST_ONLY.csv

代码 SHA-256：6FA205CDD1067C58203103F85F4C0F90781345FBC68559C92EBB6F6FB808F073
逐轮计数 CSV SHA-256：F7451C956CCCB7F5E68CC1919BDC488FF6ACE4B78C7EE08F2A63B3B016C054A9
run 汇总 CSV SHA-256：75400E10300D985FB19EAB9CCEFC20AAE687CCA9A44160AE9E7D96A5F662DC58
fixture trace CSV SHA-256：CC73AF0CC615938AE5852D58CAD3070960186F41F56501BC0B9C07B5FCE22256


E1 上游材料池实验报告 docs/reproduction/M6A_P2_I_ALL_PAIRS_MATERIAL_EXPERIMENT_2026-09-30.md SHA-256：0F54A121DC2CFFA70719C735F577C66022C2ED019706415CBC5D9A87347537A4。README_TEST_ONLY.md SHA-256：0828F6AC7D0B16E1AC4C8E170F8F64FFED2A5CD117442692CBEB86E8BBF13910。

最终校验：git diff --check 退出码为 0，tracked diff 路径为空；另对本次 6 个新增文件逐行扫描，未发现行尾空格或 tab。起止主工作区 status 列表一致，未触碰 VFSS/、VFSS-baseline/、Papers/、参考树或根目录 siamjdiscrmath.pdf。未运行完整仓库测试、性能矩阵或安全协议实验。
