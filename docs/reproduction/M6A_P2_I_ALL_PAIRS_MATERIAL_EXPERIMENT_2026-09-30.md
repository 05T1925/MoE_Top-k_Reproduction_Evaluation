# M6A-P2-I-E1：全两两离线材料池隔离实验与 AAV86 逐轮比较边统计

日期：2026-09-30
实验 worktree：`C:\Users\28641\Desktop\MoE_Top-k_Reproduction_Evaluation__m6a_p2_i_allpairs_experiment`
实验分支：`codex/m6a-p2-i-allpairs-experiment`
起始 revision：`c3926c68fd14f270faa8b55234311071947fa080`
Git 状态：**未提交、未推送、未合并；实验分支保留供复核。**

## 1. 结论与范围

- `P2-I EXPERIMENT`：**小规模真实 uCMP 材料池生成、动态子集消费与 AAV86 图计数均通过本轮 TEST_ONLY 核验**。真实 FSS keygen 样本为 `n=5, r=2`，不等于协议集成或安全证明。
- `P2-I SECURE RUNTIME`：**NO-GO，保持原结论**。没有实现完整 AAV86 secure sorting、Protocol I 生产入口、独立 party 进程、完整 raw-score→原顺序 Top-K mask 或安全组合。
- 算法对象仍是 AAV86 §3.1 / Theorem 3.1 的固定轮数随机**完整排序**，输出前 K 的原顺序 mask 属于项目适配。没有宣称 AAV86 论文直接给出本项目的 Top-K-mask 安全协议。
- Protocol III 按用户决定继续 `DEFERRED BY USER DECISION`；本轮未实现或扩展审查它，也未从 M6A 目标中删除它。
- 历史限制 `AUTHOR_EXACT = NOT_PROVEN` 保持不变。

本轮批准方向的可核验来源是用户于 2026-09-30 提供的任务文本（附件 `b4c3c4b8-849a-4c9b-8807-3c72eceea48a`）中对用户/学姐决定的记录。没有单独附上学姐原始回复，因此本文不补写其原话、日期或更宽的批准范围。

## 2. Git 基线与原工作区保护

开始时原工作区与任务给定状态一致：

- 分支 `feat/m6a-performance-evaluation`，HEAD `c3926c68fd14f270faa8b55234311071947fa080`，与 `main`/`origin/main` 基线相同。
- 原工作区已有的 tracked 文档差异仍是 `PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md`、`docs/PAPERS.sha256`、`docs/REFERENCE_MANIFEST.md`；已有未跟踪内容包括 P0/P1/P2 文档及根目录 `siamjdiscrmath.pdf`。这些都是任务开始前的内容，本轮未改动、暂存、移动或删除。
- P0/P1/P2 依据仍保留在原工作区。只读记录的源文档 SHA-256：P0 `B637F31009F329391D1CC92CC959E251E64A0AAC45C5FBB96CFEE0D471B93EFE`；P1 `21DE1BE89CFCC7B66005B8E4F34BCAFFBC7556CCF22FAE7F517FEC91681456C0`；P2 `C19DF3632C411BF7BD8DEFC6C003CCDC7FC90963D6C46C871FE12D91AD551BDC`；实施计划 `F5C2D1DCD6587D763EFBD8DBF3D6CF8F9A7014EDB93145A7678ADF6876DD49E8`。
- 既有 P2 文档结论：`P2-I DESIGN REVIEW = READY_FOR_REVIEW`、`P2-I SECURE RUNTIME = NO-GO`。M5 G3 仍只是历史工程接收，不能改写成作者精确复现证明。
- 新 worktree 从核验过的 `c3926c6…` 创建在分支 `codex/m6a-p2-i-allpairs-experiment`。所有本轮新增物均在该 worktree 的 `experiments/m6a_p2_i_allpairs/` 和本报告；`VFSS/`、`VFSS-baseline/`、参考目录与论文正文未改。

## 3. 实现前接口可行性判断

底层真实接口允许按端点掩码对独立生成 DCF key：`ProtocolIUcmpMaterial(bits, mask_left, mask_right)` 计算掩码差并调用仓库 `keyGenDCF`；`export_party_material(0|1)` 可分别导出 move-only party-local 材料；`ProtocolIUcmpPartyMaterial` 可序列化/反序列化，且一次求值后拒绝重放。因而，隔离原型能够为候选 pair 生成真实的一次性两方材料，不需要 mock key、拼接旧 key 或在线 Dealer。

生产边界不同：`protocol_i_cmpagg_eval_party` 要求材料条目数严格等于 `n(n−1)/2`，并按固定 clique 遍历；现有 `ProtocolIPartyPackage` 没有 session/iteration/graph-digest 动态子集查表契约。故本轮没有调用或更改生产 CmpAgg/package，而是在新实验命名空间内直接使用低层 uCMP party material。**“底层可逐 pair 生成”不等于“生产 ABI 已支持 AAV86 图”。**

## 4. AAV86 图计数方法

`PAPER_DIRECT` 来源为 Alon、Azar、Vishkin，*Tight Complexity Bounds for Parallel Comparison Sorting*，FOCS 1986，§3.1 / Theorem 3.1，印刷页 506–507（本地作者稿 PDF 第 5–6 页）。P0 清单记载的 ignored 本地作者稿为 9 页，SHA-256 `322f1bd761a987fd09e6b59b3a3ae77d6e4b1ca9dcb1c2765e45ac4a2a7e2b83`；P1 已对该页段作原文核对。

TEST_ONLY 计数器对一个大小为 `m`、剩余 `d` 轮的递归子问题执行下列图构造：令 `t=ceil(m^(1/d))`、`q=t−1`，均匀无放回抽取 q 个 pivot；本轮边为 pivot clique 加 pivot 到 non-pivot 的完全二部边，因此该子问题贡献

```text
C(q,2) + q·(m−q)
```

按 pivot 的稳定全序把其余点分入 `q+1` 个桶，桶只进入下一轮递归；桶内没有额外的同轮比较。`d=1` 时 `q=m−1`，当前子问题图覆盖所有无序 pair。计数器用整数 nth-root 避免浮点边界误差。小规模图显式生成边并由独立的穷举 oracle 逐 pair 检查“当且仅当至少一端是本轮 pivot”；大规模不分配边列表，按已抽 pivot 集合计算实际 pivot-clique / pivot-element 边数。

稳定排序键按当前项目契约作 TEST_ONLY 映射：signed Q20.12 raw word 降序、`original_index` 升序。分数 `/2^12` 不改变 raw 整数比较次序。图查表只用单独的 shuffled handle；测试 fixture 使用非恒等 handle→original 映射 `[3,0,4,1,2]`。这只是明文模拟，不证明真实 shuffle 或 handle 隐私。K 不进入图生成器；对 `K=1`、中间 K、`K=n` 的输出 mask 只用作稳定 oracle 检查。

### 4.1 预注册矩阵和保存结果

矩阵在执行前固定于脚本：

- `n = 1,2,3,4,5,8,16,32,128,256,1000,10000,100000`；覆盖计划中的 `(128,2/5)`、`(256,2/5)` 等小/代表规模，并增加若干探索点。
- `r = 2,3,4,5`。
- 三组输入/pivot 种子：`(1,1001)`、`(7,1007)`、`(42,1042)`；handle 洗牌种子由输入种子与固定常数派生。
- 共 **156 个 run、546 条逐轮原始记录**，全部保存在 [aav86_graph_counts.csv](../experiments/m6a_p2_i_allpairs/aav86_graph_counts.csv)。CSV 保存每轮 active/nontrivial 子问题、空桶、逻辑节点、singleton carry、pivot 数、pivot-pivot 与 pivot-element 边、同轮递归边数、`e_i`、run 总边数、`M_observed`、pool slot 数与使用率、去重核验类型及上界。

独立穷举 oracle 对 `n=1,2,3,5,8,13,32` 的小规模图逐 pair 核对；还检查同轮/跨轮 pair 无重复、排序结果与稳定明文 oracle 一致。覆盖全相等、重复值、`INT32_MIN/INT32_MAX` 原始值、非二次幂 n、singleton、`K=1`/中间 K/`K=n`。自检总结果：`SELF_TEST PASS`。

下表列出代表 run 的**每轮实际边数**；全部种子的全部轮次在 CSV 中，未只保留较低样本：

| n | r | 输入种子 / pivot 种子 | 每轮 `e_i` | `M_observed` 样本最大值 | `N_pool` 逻辑 slot | 实际使用 / 未用 |
|---:|---:|---:|---|---:|---:|---:|
| 5 | 2 | 固定边界/重复值 fixture / 20260930 | 7, 1 | 7 | 20 | 8 / 12 |
| 32 | 3 | 42 / 1042 | 90, 78, 24 | 90 | 1,488 | 192 / 1,296 |
| 256 | 4 | 7 / 1007 | 762, 1,065, 1,027, 694 | 1,065 | 130,560 | 3,548 / 127,012 |
| 10,000 | 5 | 1 / 1001 | 59,979; 59,718; 62,989; 72,260; 54,001 | 72,260 | 249,975,000 | 308,947 / 249,666,053 |
| 100,000 | 2 | 42 / 1042 | 31,549,914; 33,896,022 | 33,896,022 | 9,999,900,000 | 65,445,936 / 9,934,454,064 |

`M_observed = max_i e_i` 是所列固定种子样本最大值，**不是理论界**。由算法构造可得每轮活跃子问题互不相交，且边是原逻辑域无序 pair 的子集，故有一个宽松但可证明的逐轮上界 `e_i ≤ C(n,2)`。本轮没有推导更紧的最坏情况界；更紧的 per-round bound 记为 `UNKNOWN`。AAV86 论文的期望总比较复杂度结论不是这组样本的上界，也不等于安全协议在线时间。

## 5. 全两两离线材料契约与实际原型

### 5.1 容量和标识

本轮实验契约为：无 padding 的逻辑域恰有 n 个 shuffled handles；每个 AAV86 iteration 为每个 canonical 无序 pair `a<b` 预留 `invocation_id=0` 一项；一个 pair 在每轮图中最多比较一次；不同 iteration 使用独立材料。因此

```text
N_pool(n,r) = Σ_i C(n,2) = r·C(n,2) logical slots
party-local key count = N_pool(n,r) for P0 + N_pool(n,r) for P1
```

每项由公共 pool 头和槽位/party-local key 共同绑定 `(protocol_version, session, iteration, shuffled_endpoint_a, shuffled_endpoint_b, invocation_id, party_id, comparison_bits)`。没有使用 `original_index` 作为材料查表编号。AAV86 的 pivot 被本轮比较后移出递归桶，未来轮次的子图不再含该 pivot；同轮子问题又互不相交，因此相同端点 pair 不会跨轮或同轮重现。该结构证明允许此实验按轮开槽，但不证明共享节点掩码下的 FSS key 相关性安全。

若以后加入 padding，公式中的 n 必须替换成明确的 padded domain；若同一端点对每轮多次调用，则需把 invocation multiplicity 加进求和。单凭 `M` 大于在线比较数不能把某 M 个 key 重标号给任意端点。

### 5.2 生成、查表与测试边界

实验生成器在读取 AAV86 活跃边 CSV 之前，先为 `n=5,r=2` 的每轮十个 pair 调用真实 `ProtocolIUcmpMaterial`，导出并序列化两方 key。每轮生成各自的 full node masks，并向两方 pool 放入 additive mask shares；同轮不同邻边共享对应节点掩码，跨轮使用 fresh mask 与不同 FSS PRNG 输出。PRNG 种子固定以便复核，**这不是密码学随机数安全性声明**。此外，为逐一核对每个 pair/iteration 的 key lookup 和真实比较函数，测试另建了不同 session、不同 seed 的全对 validation pool，并一次性消费全部 20 个 pair/iteration key；这些 key 不与下面的 AAV 活跃图池复用。测试代码中的 T-like 生成器、P0、P1 都在一个进程中，生成器仍可见完整掩码和两方材料；没有实现或验证三方进程隔离/非合谋。

生成完成后才加载 AAV86 图边 fixture，并分别对每轮 party-local pool 绑定图标签和允许的边集合。查表拒绝错误版本、session、轮次、非 canonical endpoint 次序、invocation、party、比较位宽、非当前图边、错误图 digest、缺槽位与重复消费。图标签目前是 TEST_ONLY FNV64，只用于本地请求一致性检查，**不是密码学 digest、提交或防篡改证明**。测试边集合没有发给材料生成函数/T。

### 5.3 真实 key 消费样本

固定 fixture 的 shuffled handle→original index 映射为 `[3,0,4,1,2]`，原始 score 为 `[INT32_MAX,7,7,-4,INT32_MIN]`。AAV86 图计数器实际产生：

| iteration | active 非空子问题 / 空桶 | 逻辑节点 | pivots | pivot-pivot 边 | pivot-element 边 | `e_i` |
|---:|---:|---:|---:|---:|---:|---:|
| 1 | 1 / 0 | 5 | 2 | 1 | 6 | 7 |
| 2 | 2 / 1 | 2 | 1 | 0 | 1 | 1 |

两轮图共 8 条比较边。原型在每轮 10 个候选 pair 中只消费本轮这 7 条/1 条实际边；P0、P1 各自都消费 8 个一次性 party key，12 个逻辑 slot / 每方未使用。被消费材料经仓库 DCF/uCMP 原语对稳定 priority key masked value 运行，TEST_ONLY 重构两方比较 share，与明文 stable oracle 对照通过。该样本不执行完整 AAV86 secure sorter 或 Top-K mask。

拒绝检查通过：错误 session、错误 iteration、反向端点、错误 party、缺失 invocation slot、错误 graph digest、错误参数位宽、当前图外的 pair、重复消费、截断的底层序列化 key。独立 validation pool 另对 20 个候选 pair/iteration 全部执行真实 uCMP 比较并逐个查表，确认槽位返回的 party key 对应其端点掩码和优先键结果；它使用新 pool/session，未重复使用活跃图池的 one-shot keys。跨轮相同端点 key 的序列化字节不同。字节不相同只是本种子样本的非复用检查，不能替代随机性/密钥独立性证明。

## 6. 成本和未测边界

### 6.1 小规模实测（一次 TEST_ONLY 运行）

| 项目 | 实测值 | 解释 |
|---|---:|---|
| 逻辑池容量 | 20 slots | `r=2 × C(5,2)`；每方 20 个 party-local key，共 40 个 party key 对象 |
| 活跃图边 / 在线消耗 | 8 / 20 slots | 每方各消费 8 项，逻辑未用 12 项 |
| uCMP key 序列化 | 921 bytes / party key | comparison bits=36；key 内部 payload 为 `24B+24=888` bytes，即 7,104 bits；serialized key 另含 33-byte primitive header/length fields |
| party pool envelope | P0 18,963 bytes；P1 18,963 bytes；共 37,926 bytes | 实测本实验 serializer，含 pool header、参数、每方 10 个 mask shares、20 项槽位元数据及 party key；不含网络/TLS/framing |
| 全池生成时间 | 0.240075 ms、0.138949 ms（两次单次运行） | 每次覆盖测试 mask-share 生成、20 个真实 uCMP keygen 和序列化；deterministic TEST_ONLY PRNG。保留两次值，不求平均、不外推 |
| 本地 wrapper + 活跃图 uCMP eval | 0.065826 ms、0.073332 ms（两次） | 单进程 8 条边、查表/检查/比较路径的 TEST_ONLY 计时，不是线上延迟或网络测量 |
| FSS eval 调用数 | 112 次（按源码调用结构计） | 活跃图 8 edges × 2 parties × 2 次 `evalDCF` = 32；独立 20-slot conformance pool 再做 80 次；没有 profiler hook |

独立 validation pool 的 keygen 时间与 wire bytes没有纳入上表；均记为 `NOT_MEASURED`。初版 harness 首次失败发生在 test-only masked-share 重构检查：测试误把同一个 full priority key 同时当成 P0、P1 两份输入 share；更正为随机 additive split 后，最终 fixture 使用 `comparison_bits=key_bits+1=36` 并通过。由于初次失败发生在 uCMP 比较之前，本文不把它归因为 primitive 位宽错误，也没有声称做了 35/36 位对照实验。36 位只覆盖当前 `n=5` 样本；所有 `n` 的 range/conformance 仍待证明。

### 6.2 大规模只算容量，不生成/分配

| n | `N_pool(n,2)` | `N_pool(n,5)` | 实验动作 |
|---:|---:|---:|---|
| 128 | 16,256 | 40,640 | 只跑明文图计数；无 key pool 分配 |
| 256 | 65,280 | 163,200 | 只跑明文图计数；无 key pool 分配 |
| 1,000 | 999,000 | 2,497,500 | 只跑明文图计数；无 key pool 分配 |
| 10,000 | 99,990,000 | 249,975,000 | 只跑明文图计数；无 key pool 分配 |
| 100,000 | 9,999,900,000 | 24,999,750,000 | 只跑明文图计数；无 key pool 分配 |
| 1,000,000 | 999,999,000,000 | 2,499,997,500,000 | 只算理论 slot，不跑图矩阵、不生成 key、不分配 pool |

以 `n=10^6,r=5`、priority key bits=52、实验规则 comparison bits=53 为例，当前 party-pool serializer 的**理论序列化长度**为每方 `3,372,496,667,500,063` bytes（约 3.37 PB，含每轮 node-mask shares 和槽位 metadata）；没有实际创建这些对象。物理 RSS、allocator overhead、盘上容量均 `NOT_MEASURED`。上述是全对池项目扩展的容量/序列化推导，不能用 AAV86 论文按实际 `e_A` 个比较生成材料的 offline 成本替代。

真实线上通信、独立 P0/P1 交付、跨进程运行、网络 framing、部署内存、全量 AAV86 secure latency、raw-score→原顺序 mask 适配成本均为 `NOT_MEASURED`。离线 T 在本轮未接收真实在线输入/rank，但单进程 test harness 的控制器能看到测试 oracle 和两方状态，不能将其描述成安全部署。

## 7. 安全与后续路线边界

全两两材料避免了“先知道 exact adaptive edge list 才能给该边发材”的直接时序冲突：在本试验里 T-like 生成器先为每轮全部 endpoint pair 制材，图随后才确定/加载，在线模拟调用只选择图内 edge。成本是 `Θ(rn²)` party-local 材料。以下仍未证明或未实现：

1. 同轮节点掩码跨邻边复用时，各差分 DCF key 的联合分布、组合隐私与自适应图选择安全性；跨轮 fresh mask/key 的正式随机性与 domain separation。
2. T 的实际交付、擦除、非合谋模型以及对 P0/P1 的可见视图；当前 test 是单进程，不是 2+1 secure execution。
3. pivot/bucket/图结构、endpoint lookup 和失败行为的 transcript leakage；FNV graph label 不是安全绑定。
4. Protocol I 的 masked input、local-rank/分桶依赖、hidden shuffle 与 inverse mapping；如何从完整排序得到原输入顺序 Top-K XOR mask，以及端到端轮数/通信。
5. 不同 `n` 下 priority key、uCMP 比较位宽和 padding/range 的完整 conformance 与证明。
6. option 2 的 `M` 个材料方案。`M ≥ M_observed` 只描述计数，不能让 edge-bound keys 服务任意端点；端点绑定、掩码相关性、每材料 one-shot 与查询泄露需另行设计/证明。

因此，本次全两两路线只能标记为 `EXPERIMENTAL_ALL_PAIRS_OFFLINE_POOL`。P2-I secure runtime 仍是 NO-GO；不宣称 exact-edge compact preprocessing 已解决，不改项目总纲/benchmark 矩阵，不改变路线范围。

## 8. 执行和验证记录

本轮仅运行实验专属验证，未运行仓库全量测试或性能验收：

1. `python experiments/m6a_p2_i_allpairs/aav86_graph_counter.py --self-test` → `SELF_TEST PASS`。
2. `python experiments/m6a_p2_i_allpairs/aav86_graph_counter.py --matrix experiments/m6a_p2_i_allpairs/aav86_graph_counts.csv` → 156 runs、546 rows；随后逐 run 校验行数、`M_observed`、总边数、capacity/unused 公式与 `e_i≤C(n,2)`，`MATRIX_VALIDATION PASS`。
3. `python experiments/m6a_p2_i_allpairs/aav86_graph_counter.py --material-fixture experiments/m6a_p2_i_allpairs/aav86_n5_r2_test_edges.csv` → fixture `e=[7,1]`。
4. WSL 下用 `g++ -std=c++17 -O1 -ffunction-sections -fdata-sections -maes -mssse3 -msse4.1 -fopenmp -pthread`，链接仓库 `VFSS/src/moe_topk/protocol_i_ucmp.cpp`、`VFSS/ext/FSS/dcf.cpp`、FSS config/PRNG/utils 与 cryptoTools Defines/AES/PRNG，直接编译实验 C++ harness 并立即运行 fixture → `MATERIAL_POOL_TEST PASS`，输出上述真实 key 数、字节、计时和 fail-closed 检查结果。生成的可执行文件位于 WSL 临时 `/tmp`，未进入仓库。
5. Windows shell 未安装 cmake；WSL 提供 CMake 3.16.3，而仓库要求至少 3.17，所以 standalone CMake target 未通过 CMake 配置。真实 keygen 测试已通过上述隔离的直接 g++ 构建运行；没有伪报 CMake/项目全量构建通过。
6. 对实验 worktree 执行 `git diff --check` 和新增文本 trailing-whitespace 检查；并再次核对原工作区分支、HEAD、Git 状态、tracked diff 名称与任务开始时一致。未提交、未推送、未合并。

## 9. 新增文件

1. `docs/reproduction/M6A_P2_I_ALL_PAIRS_MATERIAL_EXPERIMENT_2026-09-30.md`：本报告。
2. `experiments/m6a_p2_i_allpairs/CMakeLists.txt`：可在满足仓库 CMake 版本时单独配置的实验 target；只依赖现有 VFSS library。
3. `experiments/m6a_p2_i_allpairs/aav86_graph_counter.py`：TEST_ONLY 图统计器、独立小规模边 oracle、稳定 mask 自检与预注册矩阵生成器。
4. `experiments/m6a_p2_i_allpairs/aav86_graph_counts.csv`：全部 156 个 run 的 546 条逐轮结果。
5. `experiments/m6a_p2_i_allpairs/aav86_n5_r2_test_edges.csv`：由图计数器生成、供真实材料池消费测试的 n=5/r=2 活跃边 fixture。
6. `experiments/m6a_p2_i_allpairs/material_pool_test.cpp`：隔离 all-pairs pool wrapper、真实 party-local uCMP keygen/序列化/消费和 fail-closed 测试。

仅以上 6 个文件是本轮新增/修改的 Git 工作区内容。生产运行时代码、测试目录、项目计划、参考树与论文正文均未改动。
