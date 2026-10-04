# AAV86 / CA 明文参考（TEST_ONLY）

该目录中的 Python 程序是为 M6A-P2-I-E5 编写的算法参考和图计数工具。输入 score、原始下标、endpoint handle、pivot、local rank、bucket 和执行轨迹全部以明文处理。它不是安全协议、FSS 实现或生产接口。

## 来源和语义

- AAV86 原始排序：Alon、Azar、Vishkin，FOCS 1986，§3.1 / Theorem 3.1，印刷页 506–507。对于当前大小为 \(m\)、剩余迭代数为 \(d\) 的子问题，\(t=\lceil m^{1/d}\rceil\)，无放回均匀抽 \(q=t-1\) 个 pivot；pivot 之间构成 clique，pivot 与所有 nonpivot 相连；nonpivot 被排入 \(t\) 个有序桶，剩余深度 \(d-1\) 时递归。
- Agarwal 等 CCS 2024，§5.2 的 CA local rank 和 non-distinct stable rank、§5.3 的 Algorithms 1–2：CA 图只揭示邻居局部 rank，而算法的递归控制流保持不变。会议版没有把“rank 数值等于 bucket 下标”单独印成一条公式；对 AAV86 这个图，由邻接关系和 local-rank 定义可直接推出：nonpivot 的 LRank 是排在它前面的 pivots 数，因而就是 0 到 q 的桶下标；pivot 连接子问题内所有其他节点，LRank 是它在全子问题中的位置。
- 会议版 §5.2 对 canonical edge \(i<j\) 的 local-stable-rank 原式，第二项是 \(x_j\le x_i\)。E4 报告曾将该不等式方向转录为“大于等于”；应以会议版 PDF 和 P1 决策文档的原式为准。E5 只记录该转录更正，不改写 E4 历史报告。
- 输入优先序 \((-score, original\_index)\) 是项目的明文 TEST_ONLY 契约；只要 original_index 唯一，它就给出唯一全序。它没有定义 Q20.12 字节编码、稳定字段的安全搬运或原顺序 mask 适配器。
- \(m=1\) 时不建图并原样携带，空桶只计数、不递归；这两项是本参考程序的 TEST_ONLY 终止约定。论文伪代码没有专门列出这些工程分支。\(d=1,m>1\) 时仍建立最终图和桶，\(t=m,q=m-1\)，此图为完整图；此层不再调用子问题。

本目录不决定 secure CA 消息、稳定键编码、padding slot 的参与方式、在线视图或材料池安全性。

## 命令

从仓库或 worktree 根目录运行：

    python experiments/m6a_p2_i_allpairs/TEST_ONLY/aav86_ca_reference_TEST_ONLY.py self-test
    python experiments/m6a_p2_i_allpairs/TEST_ONLY/aav86_ca_reference_TEST_ONLY.py fixture
    python experiments/m6a_p2_i_allpairs/TEST_ONLY/aav86_ca_reference_TEST_ONLY.py matrix --compare-e1

self-test 在小域穷举排列、多 seed 随机输入和稳定 tie fixtures 上，把参考排序与独立 Python sorted 全序 oracle 对照；另以逐条边的 local-rank 公式复核优化计算的 rank，并检查图、桶覆盖、递归缩小、边去重和明文 Top-K mask。

fixture 复用 E1 的 \(n=5,r=2\) 映射和 pivot seed，输出每个子问题的顶点、pivots、LRank、bucket、边及结果轨迹。

matrix --compare-e1 用 E1 的 n、r、输入 seed 和 pivot seed 重建 156 个 run / 546 个轮记录，输出 v2 两份计数 CSV，并把可比计数与 E1 旧 CSV 对照。该命令只做图和递归计数，不做性能基准。

## v2 计数口径

- 活跃图边数 \(e_i\) 为每个非平凡子问题中 pivot clique 和 pivot/nonpivot 边的去重并集；同轮子问题按不相交桶产生。小域矩阵行实际枚举边并去重；大域矩阵行不保存完整边清单，使用 pivot 移除后的结构论证作为 audit basis。
- empty_bucket_slots 记录本轮生成桶中的空槽；nonterminal_empty_bucket_slots 和 terminal_empty_bucket_slots 分别标出是否还会进入后续递归。空桶本身不调用递归。E1 的 empty_buckets 记在子调用所在行，比较器按轮次偏移核验，不把两种口径误认为同一行定义。
- v2 移除 E1 中默认值为 0、没有计算赋值的 same_round_recursive_edges。该指标在旧数据里是未测状态，不被改写成实测零。
- max_active_edges_observed 是本组有限 seed/输入样本的观测最大值，不是 worst-case bound。
- 主容量域是 logical_n：每方每轮 \(C(logical_n,2)\)，全协议 \(rC(logical_n,2)\)。padded_n_informational 仅记现有 pipeline 的 pad 尺寸。padded pool 容量列是条件性算术值，不表示 AAV86 将 padding dummy 加入随机排序域。
- edge_audit_basis 对 \(n\le32\) 表示本矩阵 run 已枚举并检查去重；较大 n 表示使用结构论证，不代表逐条大域边枚举。

## 文件

- aav86_ca_reference_TEST_ONLY.py：递归排序、CA 图与 rank/bucket 推导、计数器、明文自检。
- aav86_ca_counts_E5_v2_TEST_ONLY.csv：每轮图/递归/桶计数。
- aav86_ca_runs_E5_v2_TEST_ONLY.csv：每个 run 的总边、sample observed maximum 和分域容量。
- aav86_ca_fixture_trace_E5_v2_TEST_ONLY.csv：E1 n=5 fixture 的完整子问题轨迹。
- 审查结论见 docs/reviews/M6A_P2_I_E5_AAV86_CA_SPEC_AND_REFERENCE_2026-10-02.md。

本程序不生成 FSS key，不访问或序列化材料池，不连接网络，不公开 rank 给协议方，不接入 VFSS/，也不证明 shuffle、材料相关性或任何 party view 的安全性。
