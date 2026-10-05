# BMW16 S3 独立复审与 Protocol I 接入门

日期：2026-10-05

复审标签：`BMW16_DERIVED_SELECT_4R_TEST_ONLY`

复审分支：`codex/bmw16-s3`

被测 revision：`691260833206533882e042ca37e8a21c7b9ecc24`

父 revision（S1）：`0a0593deaccb55225566da6af3aac661814e717c`

## 1. 决定

S2 的四层明文调度和线性比较调用上界可由代码与计数规则复核；其 A5/A7 修正式排名恒等式在注明的括号前提下成立。它仍不是一个对所有有限输入/随机带都定义并满足项目输出契约的 Select：复现到 159/776 个 `UNDEFINED/INVALID_FINITE_CASE`，40 个正式规模向量仅 16 个返回 `SUCCESS`，另有一个非成功分支产生了错误候选。

因此，本报告不把 S2 的高概率结论提升成已验证的有限规模成功率，不接受 S2 原报告中 `FINITE_N_SPEC_COMPLETE=PASS`、`SELECT_4R_RUNNABLE=PASS` 和 `SECURE_PROTOCOL_I_DESIGN_GATE=READY_FOR_REVIEW` 作为本次门禁结论。按 S3 的独立门禁，有限规模完整性与项目 Select 契约为 **NO-GO**；Protocol I 组合设计门也为 **NO-GO**。这里的 NO-GO 是当前 revision 和冻结项目契约的决定，不是对 BMW16 Theorem 8 的反证。

未实现 secure selector，未修改 `VFSS/`，未运行 secure E2E 或 LAN/WAN 矩阵。S2 提交及其报告没有改写；原始逐次数据在逐字节归档并复核后，由本 S3 普通提交从 Git 工作树移除，仓库保留摘要和完整哈希清单。

## 2. 被测对象、证据来源与工作区

### 2.1 revision 和隔离

| 对象 | S3 复核结果 |
|---|---|
| S2 被测 commit | `691260833206533882e042ca37e8a21c7b9ecc24`，父为 S1 `0a0593deaccb55225566da6af3aac661814e717c` |
| S2 分支/worktree | `codex/bmw16-bmw16-s2`，复核时干净；保留原提交与原始报告 |
| S3 分支/worktree | `codex/bmw16-s3`，从 S2 精确 commit 创建；本报告只在此工作树形成 |
| 仓库基线 refs | `main` 与 `origin/main` 均为 `c3926c68fd14f270faa8b55234311071947fa080` |
| 桌面主工作区 | `feat/m6a-performance-evaluation`，起始 HEAD `c3926c68fd14f270faa8b55234311071947fa080`；已有差异保留 |
| 保护范围 | 未修改 E20/E21、PR #28、`VFSS-baseline/`、论文 PDF、参考工程或安全协议源码；未推送、合并或建 PR |

S2 commit 的 22 个文件清单由 `git show --stat` 核实，其中包含本报告第 7 节列出的逐次 JSONL、边轨迹和完整小轨迹。S2 的摘要文件仍留在 S3 工作树。原始逐次证据先复制至 `C:\Users\28641\.codex\evidence\BMW16_S2_691260833206533882e042ca37e8a21c7b9ecc24`，再从 S3 的 Git index 删除；没有重写 S2 历史。

### 2.2 论文与证据层级

主来源是本地 `Papers/1603.04941v1.pdf`，44 页，SHA-256：

```text
F46F83CBA279F37E9E3AAA0D64B145FDFBC52C0C9C8B79BACEC1259CA9CA23
```

`Papers/017.pdf` 是相关作者 2018 年论文/学位论文的复述资料，249 页，SHA-256：

```text
3EAE3A93F12AB71EEA8AC040167F21351FCF3FF54DC423985A04087E3E90CB15
```

它只作交叉核对，不当成独立证明。PDF 保持本地只读且不入 Git。论文页码均指 PDF 物理页；括号内为页面印刷页码。

| 来源位置 | PAPER_DIRECT 内容 | S2/项目解释边界 |
|---|---|---|
| §2，PDF p.7（印刷 p.6） | noiseless pairwise comparison 模型、每轮批量查询以及输入不可区分/随机置换背景 | 不直接定义项目稳定键、秘密共享、泄露或 Top-K mask |
| §4.1，PDF p.10（印刷 p.9） | 第一阶段从 `S1` 找包围其中位数的 pivots | 支持把 A5 `r=1` 的目标解释为 `S1` 的中心分位；不是 Algorithm 5 排版初始化值的逐字修正 |
| Theorem 8，PDF p.11（印刷 p.10） | `0<ε<1/18`；四轮、`O(n)` 比较；以至少 `1−exp(−Ω(n^ε))` 概率得到无错误 median Partition | 渐近定理，不给具体常数、`n₀` 或有限 `n` 数值失败上界 |
| Appendix A, Lemma 1，PDF p.16（印刷 p.15） | median 到任意 rank 的 padding；Partition→Select 的两个哨兵并行归约；成功概率至少 `p²` | `p²` 需要固定输入条件下两份算法随机带独立；项目的整数扩张式是明示推导 |
| Algorithm 5 及证明，PDF pp.29–30（印刷 pp.28–29） | 两层 comparison schedule：`S1×T1`，随后输入对选定 pivot 的比较；Theorem 5 的近似切点论证 | S2 将排版中的 `k1=N/2` 改为 `floor(|S1|/2)`，属 `PROJECT_DERIVED`，只审查 `r=1` |
| Algorithm 7 及其证明，PDF p.34（印刷 p.33） | 两个带低/高 dummy 的 A5 调用、`U/V/W`、R3 抽样比较、R4 对 W 的全比较、随机标记分支 | S2 对 R3 rank、样本放大、整数化、失败状态和 z 的归属均属项目规则；不能把 Theorem 8 自动套到这些改写 |

处理方法：检查 S2 引用的原页，并直接渲染复核 PDF p.16、p.29、p.30、p.34。PDF 主体 OCR 可以检索；本地解析器没有完整提取 Appendix，故 Appendix 结论按原 PDF 页面而不是二手解析文本登记。

### 2.3 论文陈述与 S2 项目构造的区分

1. **A5 `r=1`**：Algorithm 5 的字面 `k0=k1=N/2` 在 `A1=S1`、`|S1|≈N^(2/3)` 时可能超出可达 rank。S2 用 `s=|S1|`、`k1=floor(s/2)`，把 `k1+1` 当作 S1 的一基中心秩。这与 §4.1 文字一致，是合理的 `PROJECT_DERIVED` 修正，不是原伪代码原样实现。R1 的 `S1×T1` 能给每个 pivot 的 S1 lower-count；只有采样 pivot 同时落在目标两侧时，才定义 `a1,b1`。有限样本并不保证这一事件。
2. **A5 bracket**：R2 只在 R1 bracket 已产生后冻结。缺任一侧时，S2 返回 `UNDEFINED/INVALID_FINITE_CASE`，没有暗中加入比较或 fallback。失败符合抽样可能性，代码状态明确；它同时证明 S2 尚未给所有合法随机带一个有限算法输出合同。
3. **低/高 dummy 外端点**：S2 在低 dummy 实例选 `a1`、高 dummy 实例选 `b1`；这是 Algorithm 5 允许的区间内选择。Theorem 5 对任意区间内 pivot 的概率论证是否覆盖 S2 这一对相关端点，需要将整套采样/定理前提联立证明；有限运行本身不证明该点。
4. **A7 R3 秩**：在严格递增全序上，设 `m` 为外层 A7 输入大小、`h=m/2`；低 dummy A5 在真实 A7 输入上的拒绝集合是长度 `c1` 的前缀，高 dummy A5 的拒绝集合是长度 `c2` 的前缀。若 `c1≤h≤c2`，则两 prefix cuts 不交，`U` 正是位置 `c1+1..c2`，`|U|=c2−c1`，且 U 内要拒绝 `q=h−c1` 个最小元素。该等式不要求两切点精确，只要求观察到的括号成立。S2 对精确/近似切点使用此式的集合代数正确。字面 `h−c2` 不能作为 U 的 residual rank；S1 的 exact-cut 反例是其中一个见证。若 `c1>h`、`c2<h` 或两个 cut 交叉，前提失败，不能继续套该等式。
5. **A7 R4 归属与计数**：令 `R*` 只计当前外层 A7 输入里已被拒绝的记录。它包含任意 K padding 和选择哨兵，因为这些此时是 median 输入；它不包含仅为 A5 比较临时加入的 `2D` scratch dummy。R3 取 x/y 的全 U 秩；x、y 本身未被严格“小于 x/大于 y”的过滤删除，因此仍在 W。R4 对 W 完整成对比较，若 W 内第 `qW=h−|R*|` 个元素 z 及其自身归入 Reject，则拒绝数为 `|R*|+qW=h`。论文 Algorithm 7 的严格两侧描述没有给 z 的相等归属，S2 将 z 分给 Reject 是必要且代数正确的 `PROJECT_DERIVED` 补全。若 R3 bracket/`qW` 前提不成立，状态失败而不是套用该证明。
6. **两哨兵 Select 归约**：对 API 的 K-th highest，令原始 `n`、一基升序秩 `r=n−K+1`、Lemma 1(ii) 前的 `N=n+1`，并令 `L=max(N−2r,0)`、`H=max(2r−N,0)`。共同 padding 后 `M=N+L+H=2 max(r,N−r)≤2n` 且为偶数。median 拒绝数 `h=M/2=r+L`。低 sentinel 实例的目标 real item 是第 `h+1` 个、落入 Accept；高 sentinel 实例是第 `h` 个、落入 Reject；只在原始 real items 上求 `Accept_low ∩ Reject_high`，结果唯一。K=1 时 `(L,H)=(0,n−1), M=2n`；K=n 时 `(L,H)=(n−1,0), M=2n`。偶数 n 且 K=n/2 时得到 `H=1,L=0,M=n+2`；奇数 n 的中间 K=(n+1)/2 时无 rank padding 且 `M=n+1`。端点与两种奇偶都由同一公式覆盖。
   分区式可展开检查为：`K<n/2` 时 `(L,H)=(0,n−2K+1)`；`K>n/2` 时 `(L,H)=(2K−n−1,0)`；偶数中位 `K=n/2` 有一个 high padding；奇数中位 `K=(n+1)/2` 无 padding。外层两个 Select sentinel 每份各一个，padding 项在两份中相同；交集后过滤所有 padding/sentinel，仅 real input item 能作为输出。
7. **`p²` 前提**：两份 Partition 是同步执行在四个比较层里，并不意味着随机带相同。Appendix 的下界从每份在同一固定输入上成功率至少 p，且其理想随机带独立推出联合成功至少 p²。S2 的 domain-separated SplitMix64 只给可复跑的 TEST_ONLY 种子流；它不是安全 PRG 或理想独立随机带的安全论证。
8. **有限参数与论文定理**：S2 固定 `ε7=1/36`（在 Theorem 8 开区间内）、`D=ceil(m^(25/36))`、A5 样本 `s=min(N,ceil(N^(2/3)))`、`p=min(s,ceil(N^(1/3)))`、`k1=floor(s/2)`、R3 若 `u²≤32m` 则全 U 成对比较，否则 `v=min(u,ceil(32m/u))` 无放回抽样、W 阈值 `floor(m^(5/12))`。这些规则使程序可执行，但 `32`、取整和 bracket abort 不能仅凭 Theorem 8 变成已获证明的有限算法。S2 声称改写版有 `1−exp(−Ω(m^(1/72)))` 的渐近成功率；S3 只确认该 exponent 的幂次代数与抽样方案相容，未确认完整 bad-event 上界、Theorem 5 前提到改写 A5 端点的适用以及常数闭合，故不把它当作已通过的项目定理。S2 对抽样 miss 的指数级论证是渐近 sketch；没有把常数、阈值和全部 bad events 收拢成可计算的有限 n 上界。结论为 `NO_FINITE_BOUND_ESTABLISHED`。

## 3. 独立重放：失败由什么组成

### 3.1 运行内容与种子

本报告按 S2 同一 `run_matrix.py` 重建 776 个固定用例，未按 oracle 选择参数或更换种子：719 个 `n=1..5` 全序排列、40 个正式配置、17 个 adversarial 用例。正式输入/算法种子由 S2 固定公式生成：`input_seed=0xB1600000+config_index×10000+seed`，`algorithm_seed=0xA1600000+config_index×10000+seed`。排列用例及 adversarial 种子按被测源码固定表生成。SplitMix64 是 TEST_ONLY 可复现 PRNG，不是密码学随机数。

运行环境：Windows 11 10.0.26200 x64，Python 3.13.7，MinGW-w64 g++ 8.1.0；C++ oracle harness 使用 `g++ -std=c++17 -O2`，可执行文件放临时目录并退出清理。选择器不导入任何 oracle；验证器在运行结束后独立调用冻结 Python oracle 和 `VFSS/include/moe_topk/topk_oracle.h`。

### 3.2 所有用例分类

| 运行状态/核对 | 次数 | 复审解释 |
|---|---:|---|
| `SUCCESS` | 570 | 570 个输出均与 Python、C++ stable K-th oracle 相同 |
| `PAPER_RANDOM_FAILURE_PATH`（S2 状态名） | 47 | 包含论文 U/W 随机标记事件，也包含 S2 项目检测的 crossed-cut abort；不能整体都称为论文明示随机分支 |
| `UNDEFINED/INVALID_FINITE_CASE` | 159 | 由 172 个 A5 实例 bracket 缺失事件分布在这些运行中，部分运行有两个 A5 子调用失败；没有 R1 后补比较 |
| 被声明 `SUCCESS` 但 oracle 错 | 0 | 未观察到名义成功的错误输出 |
| 所有候选与 oracle 的比较结果 | `MATCH=571`, `NO_CANDIDATE=204`, `MISMATCH=1` | 571 个 MATCH 包含一个来自失败路径但碰巧正确的候选；它不能算成功状态 |
| Python/C++ oracle 一致 | 776/776 | 对每个同分/原始 index/K 向量，期望 index 与完整 mask 一致 |

S2 `PAPER_RANDOM_FAILURE_PATH` 的根因需再拆开统计，按外层 Partition 子实例计（同一 Select 可触发两次）：

| 子实例触发 | 子实例次数 | 是否是论文 Algorithm 7 明示分支 |
|---|---:|---|
| `|U|>4D` 后对 U 随机标记 | 26 | 是，Algorithm 7 相应分支；输出可错，计入算法坏事件 |
| `|W|>floor(m^(5/12))` 后对 W 随机标记 | 6 | 是，Algorithm 7 相应分支；输出可错，计入算法坏事件 |
| `c1≤h≤c2` 不成立或 cuts crossing | 22 | 否。S2 将其复用 `PAPER_RANDOM_FAILURE_PATH` 状态名，但代码没有执行论文的 U/W 随机标记，只报告项目检测的坏事件/停止 |
| A5 R1 缺一侧 sample bracket | 172 个 A5 调用；影响 159 个完整 Select 运行 | A5 的有限抽样事件；S2 作为 undefined 返回，没有原文定量 fallback |

子实例次数和运行数不同，且以上事件会在同一运行中叠加。正式 40 个样本的状态为 16 `SUCCESS`、24 S2 失败状态、0 A5 undefined。按子实例计，正式矩阵中 U 随机标记 22 次、W 随机标记 6 次、cut-cross abort 1 次。各配置成功数：`(128,2)=5/8`，`(128,8)=3/8`，`(256,2)=2/8`，`(256,8)=3/8`，`(1000,80)=3/8`。

唯一错误候选：`official_n256_k8_seed1`，输入 seed `2975888689`、算法 seed `2707453233`，输入 SHA-256 `e158e66224702e372f99f5fb1af1dc3bdfa162633239ad898730cb9bcaacaee7`，程序候选 `original_index=244`，两个 oracle 均为 `169`。它的一个外层 Partition 进入 U 随机标记事件；交集中恰有一个 real item，并不说明该项是目标秩。程序把它保留为失败分支的候选，validator 判 `MISMATCH`，没有 oracle 修正。

### 3.3 原因判断

- **未观察到** comparator/计数器让 `SUCCESS` 与 oracle 不同的证据；四轮边结果与明文 strict key 一致。
- **已确认的算法级失败**：U/W 论文随机标记分支可以产生错误选择；一个具体错误实例已复现。该分支本来就是高概率定理的失败事件，不是单例代码修补问题。
- **已确认的有限样本未定义**：A5 对随机子样本缺 bracket 时，没有选定 pivot，因而不能构造 R2。172 个 A5 调用失败；S2 正确地没有偷偷重试，但其“有限 n 完整”声明不成立。
- **已确认的 S2 分类缺陷**：22 个 crossed-cut 子实例被置为与论文随机分支相同的状态。应单独分类为 `PROJECT_DERIVED_BAD_EVENT_ABORT` 或同等状态。该修正属于状态语义复核；为保护被测版本，本提交不改 S2 源码。
- **没有有限概率证明**：Theorem 8 隐藏 `Ω` 常数，不能给 128、256、1000 的数值 δ。40 个样本的 40% 成功率是有限观察，不是估计定理下界，也不证明理论被反驳。当前分类为 `NO_FINITE_BOUND_ESTABLISHED`。
- **随机币不满足安全结论**：可复跑 SplitMix64 seeded streams 不等于两份秘密、条件独立、均匀随机 tape。S2 的 p² 只能按理想独立带解释，不能直接用来承诺 secure runtime 的错误率。

因此问题不归结成“论文允许错误”：S2 有合法概率失败输出，也有未定义抽样执行；项目当前验收要求每次选中正确稳定阈值并输出恰 K mask。两者契约不同。

## 4. 四轮、边审计与比较预算

### 4.1 四轮因果调度

S2 源码为两个外层 sentinel Partition 共四份 A5 子调用先冻结全部 R1 边，收齐 R1 后再冻结有效 R2 pivot-star 边；然后两个 A7 在同一个 R3 时钟冻结 `V×U`/U-all-pairs，结果到达后才选 x/y、形成 W，R4 才冻结 W 的全无序对。若上游 bracket 失败，对应下游任务为空/停止；没有同轮收到结果再追加边。由源代码和 trace 可复核“比较层不超过 R1/R2/R3/R4”，不是 secure 网络 round 结论，也不代表所有坏状态都跑满有用边。

独立 auditor 未 import `select4r.py`，重建 serialized task descriptor 的期待边、验证 plan SHA、edge 所标 round / `max_info_round` 和 `requires_results_through_round`、严格 key 的比较位以及计数。它对 776/776 trace 返回 PASS。但 descriptor 中的 `U_ids/V_ids/W_ids/pivot_x`、比较轮标记由被测运行时生成并提供；auditor 不从前两轮结果独立推导 A5 cut、A7 U/W 或抽样分布。因此它证明 trace 在声明的计划内一致，不是独立数学证明，也不是任意安全执行中的视图模拟证明。

### 4.2 逐轮实计数

| 层 | 调用 | Real–real | 至少一端 dummy/sentinel | 当层重复边 | 先前层已出现的重复调用 |
|---|---:|---:|---:|---:|---:|
| R1 | 207,672 | 32,992 | 174,680 | 1,369 | 0 |
| R2 | 207,790 | 33,915 | 173,875 | 487 | 33,568 |
| R3 | 1,172,921 | 415,749 | 757,172 | 148,090 | 30,930 |
| R4 | 1,074 | 897 | 177 | 50 | 580 |
| **合计** | **1,589,457** | **483,553** | **1,105,904** | **149,996** | **65,078** |

`unique unordered edges summed per run=1,374,383`；全部 repeated calls 为 `215,074`。重复调用仍计入 comparison calls。Observed max run 是 `adversarial_boundaries_n1000_k80_seed86` 的 124,661 次；最大观测 `calls/n=134.96875`，来自 `official_n256_k2_seed6`（34,552/256）。各 R1/R2/R3/R4 最大单次层分别 9,100/8,852/106,673/171。以上仅是固定 776 个明文用例的实测。

### 4.3 S2 规则下的渐近最坏界

令原输入 n、任意 K 扩张后偶数 M，其中 `M≤2n`；`D=ceil(M^(25/36))≤M`；每个内层 A5 规模 `N=M+2D≤3M`。A5 取 `s=ceil(N^(2/3))`、`p=ceil(N^(1/3))`（按 N/s 截断），故 R1 `sp≤4N`；R2 一个 pivot 对至多 `N−1` 项。总共四份 A5，R1+R2 合计至多 `4×5N≤60M`。每个 A7 的 R3：当 `u²≤32M` 时 `C(u,2)≤16M`；否则 `v=ceil(32M/u)≤u` 且 `vu≤32M+u≤33M`。两份并行 A7 共至多 `66M`。若进入 R4，则每份 `w≤floor(M^(5/12))`，总两份 R4 不超过 `M^(5/6)≤M`。于是该 S2 整数规则下总调用数 `≤127M≤254n`，含 scratch dummy、Sentinel/rank padding、各阶段重复请求；这不是与 BB90 每轮常数相等的结论。

上述 O(n) 账本只数明文比较调用。它没有界定秘密自适应端点下的离线 DCF key pool；那可能是二次材料，见第 6 节。

## 5. 正确性契约判定

稳定顺序统计量不是只返回任意同分记录。项目顺序是 signed Q20.12 score 降序、`original_index` 升序；唯一复合键可以编码为高分优先的升序整数 `(UINT32_MAX−(raw_score XOR 0x80000000), original_index)`。未打包时也可按这两个字段字典序比较。随机置换后的槽号不能替代原 index。

一个正确且唯一的 K-th 复合阈值可使 `key_i≤τ` 的成员集合恰好有 K 项；这只给出阈值到 mask 的代数路线，不会让一个错误阈值变正确。两个 Select/Partition 哨兵的接受拒绝交集如为空、多项或含错元素，也不能作为可靠阈值。A5/A7 scratch dummy 需要额外 tag/guard 表示，确保合法 `INT32_MIN/MAX` 复合键不与 dummy 相等；明文对象类型可做到互异，现有 secure priority key helper 没有给 BMW16 dummy 编码接口或证明。

当前冻结契约仍为每次输出一个与 oracle 一致、原输入顺序的秘密共享 Top-K XOR mask，恰有 K 个 1。项目负责人可以另外审议概率性契约，但至少需要批准：δ 的量化来源/适用 n、允许错误阈值的语义、是否允许 abort、abort/失败如何保密以及如何计入失败率。论文没有有限 δ，现有 40% 正式样本成功率不能当作建议 δ 或替代批准。没有批准时不放宽现行契约。

## 6. Protocol I 安全组合门

### 6.1 目标接口（尚未实现）

候选数据流为：

```text
Q20.12 signed arithmetic shares + original_index binding
    → stable composite priority-key shares
    → hidden forward permutation of key and record identity
    → 4 adaptive comparison batches R1–R4 with private graph control
    → shared unique K-th threshold key/record
    → DCF membership: include i iff priority_key_i ≤ threshold
    → inverse route membership bits through the same hidden permutation
    → original-order XOR-mask shares (length n, exactly K selected)
```

每个箭头都有在线消息、预处理、校验与错误语义。BMW16 的四个 **comparison layers** 不能直接填进 Protocol I 的 online-round counter。

### 6.2 三方职责、消息和可见性

| 参与方 | 输入/离线材料 | 在线动作候选 | 允许/禁止可见内容 |
|---|---|---|---|
| T（离线 Dealer） | 只应获知公开形状/会话配置；为 P0/P1 生成各自 one-shot 份额、比较材料、shuffle/逆路由材料 | 预发后退出，不接触输入 share、明文分数、rank、随机 pivot 或在线自适应边 | 不得知道输入/排序/所选阈值；需要证明对材料联合生成和所有自适应 query pool 的模拟安全 |
| P0 | 自己的 signed score shares、原输入位置绑定、仅自己的预处理份额 | 与 P1 做安全转换、同一隐藏 shuffle、每轮被授权的比较/材料消费、私有选择、DCF membership 和反路由 | 不能单独看到 score、sign/carry、复合 key、rank、原 index 映射、pivot、U/V/W、活跃边、选中 index 或原序 mask |
| P1 | 同 P0 的另一份输入与预处理材料 | 和 P0 同步执行同一消息因果 DAG；不依赖 Dealer 在线补材料 | 同上；不能看到另一方 share 或由访问模式恢复的数据关系 |

目前文档允许的是固定 shape/session 元数据、受控的 shuffled `(slot, rank_P)` 打开，以及记录为 C-level 扩展的 masked-key opening；具体批准边界见 `docs/decisions/M2_PROTOCOL_I_EXACT_LEAKAGE_AUDIT.md` 和 `M2_PROTOCOL_I_SMALL_E2E.md`。这些批准不包含 BMW16 的 pivot、cut、U/V/W、active edge set、随机失败状态或 selected threshold。若图不同导致 key slot、消息长度、访问顺序、文件块、运行时长或 abort 结果不同，应当视为潜在泄露，不能靠“数据在密文材料文件里”排除。

### 6.3 现有接口事实与缺口

| 环节 | 可从仓库源码确认的事实 | 未闭合点 |
|---|---|---|
| signed score→key | `protocol_i_raw_score_input_party` 有 2 个 carry/sign exchange rounds；priority 编码 helper 为 signed score 映射为高分优先 key 并拼 slot/index。 | score adapter、原 index 绑定与新 selector 尚未组合；当前 adapter 用输入 slot 作为 index，必须证明 score/index 共置换、padding/tag 不溢出并保留 original_index。 |
| shuffle | 现有模块有 forward 与 reverse `Permute+Share` 路径及材料。 | 必须将 score、original_index/carrier 和选择 bit 绑定同一隐藏 permutation；端点/访问顺序、BMW scratch dummy 的路由语义未设计。 |
| comparison / DCF | `ProtocolIUcmpMaterial` 从 mask-difference 生成 one-shot DCF comparator；每 party material 结构是 2..53 bits 校验范围内，实际 uCMP adapter 限 34..53；每次 comparator 调用两次 `evalDCF`。 | S2 活跃边是在线结果决定的。现有 CmpAgg 只接受全体 `N choose 2` edge materials 并聚合完整 ranks，不接受私密动态稀疏边与隐藏的下一层选择。 |
| offline package | `ProtocolIPartyPackage` 按公开 endpoint pair 绑定材料；当前 selector pipeline 要求每条全图边一份 party material。 | 一次性 DCF key 不能让 T 在未知 endpoint 时“现场补发”；pool 的端点对覆盖、每个 round/实例的 domain separation 与分槽、消息 padding 和联合视图均无证明。现有 edge record 不含 BMW 阶段/实例槽号。 |
| selected threshold | S2 只在明文返回 `selected_original_index`；没有秘密阈值输出。 | 要秘密路由复合 key/record；阈值必须与每条输入记录的 one-shot DCF membership key 正确绑定。若选错则即使 mask 有 K 位也违反 oracle。 |
| 输出顺序 | 当前候选路径有 reverse carrier 机制。 | 必须将 membership bit 用同一个 shuffle 逆路由；不得公开 original-index map 或 selected index；稳定 tie field 必须始终是 original_index。 |

以上代码事实对应：`VFSS/include/moe_topk/protocol_i_priority_key.h:20`；raw score adapter 的 2-round 接口 `VFSS/include/moe_topk/protocol_i_score_input.h:24-27` 及转换实现 `VFSS/src/moe_topk/protocol_i_score_input.cpp:58-110`；CmpAgg 全图边数约束 `VFSS/src/moe_topk/protocol_i_cmpagg.cpp:35`；selector pipeline 的 all-pairs package、masked-key exchange、rank reveal/reverse-route 和 6 轮计数 `VFSS/src/moe_topk/protocol_i_pipeline.cpp:25-130`、`VFSS/include/moe_topk/protocol_i_pipeline.h:35-36`；package 的 64 MiB/1,000,000-edge 限制及序列化 `VFSS/src/moe_topk/protocol_i_party_package.cpp:11-12,45,143-176`；uCMP one-shot/两次 DCF Eval/序列化 `VFSS/src/moe_topk/protocol_i_ucmp.cpp:10-14`。这些是接口/代码核查，不代表 S3 实测了材料包。

`uCMP` party share 的序列化长度按源码为 `24b+57` 字节；在 b=43 时是 1,089 字节，不含 edge record。当前 package 每条 edge 另外序列化两个 endpoint 和 length 三个 `u64`（24 字节），故现有格式每 edge/party 是 `24b+81=1,113` 字节。两方各持一个 share。对一个保守的全端点覆盖示例，令扩张中 `M=1842,D=186,N=M+2D=2214`（对应 n=1000,K=80），每阶段每可能端点对都预备一份 one-shot key：四份 A5 在 R1、R2 各需至多 `4·C(N,2)` 槽；两份 A7 R3 与两份 A7 R4 各需至多 `2·C(M,2)` 槽；所选 threshold 对 M 个记录的候选 DCF membership 至多 `M²` 槽。因 comparator material 是 one-shot，不跨 R1/R2 或 R3/R4 复用。该保守池为 29,773,536 slots；按当前 b=43 的 package edge record 格式仅 edge payload 约 33,137,945,568 bytes，即每方约 30.86 GiB，未含输入转换 key、node masks、容器/加密、双边临时 Dealer 峰值或交付帧。单个扩张 A5 的全 endpoint universe 已有 `C(2214,2)=2,449,791` 对，超过当前 `kMaxEdges=1,000,000`。这些是容量风险推演，不是已构建材料或实测占用；实际构造可能选另一种协议，但须先证明并预检。

即便只套当前 Protocol I 全图比较：n=1000 pad 至 N=1024，需 `C(1024,2)=523,776` 条边；当前 package 的 `kMaxEdges=1,000,000` 允许这个边数，但 `kMaxPackageBytes=64 MiB` 与每边 1,113 bytes 的 b=43 格式不兼容：仅边记录约 582,962,688 bytes/party。尚未生成/序列化此包，实际峰值和压缩后驻盘/交付均 `NOT_MEASURED`。这说明调用数的 O(n) 上界不表示离线材料也 O(n)，当前全图序列化容量门对 n=1000 已不通过。

### 6.4 消息轮次、材料与性能指标边界

| 计量字段 | S3 结论 |
|---|---|
| BMW16 plaintext comparison depth | 四层 R1–R4，由 S2 trace 复核；不是网络轮 |
| 现有 priority-key Protocol I 候选 | 代码计数器声明 forward 2 + masked CmpAgg 1 + rank reveal 1 + reverse 2 = 6；它是不同的 all-pairs/rank 输出路线，不是 BMW16 Select 接入实测 |
| 现有 raw-score adapter | 单独源码声明 carry/sign 两轮；与 BMW16 selector 的完整合并消息 DAG 未实现 |
| raw Q20.12 shares→BMW16→DCF membership→原序 mask 的 online rounds/bytes/time | `NOT_DERIVED` / `NOT_MEASURED` |
| BMW16 活跃 edge calls | plaintext 实测与 O(n) 上界见第 4 节 |
| 离线 edge/threshold key slots、DCF Eval/PRG 总量、Dealer 峰值、各方驻盘/交付 bytes、网络 bytes、离线时间 | `NOT_MEASURED`；容量下界/格式推演见第 6.3 节 |
| secure leak / single-party joint-view proof | `UNPROVEN` |

完整 selector 网络图必须为每一 comparison batch 给出双方 outbound 冻结时点、peer read barrier、结果如何保持秘密并驱动下一批边，以及随机抽样/随机标记如何由双方独立共同生成。不能直接用 `4`、历史 `2r+1` 或现有 6/8 轮数字填完整 online path。

## 7. S2 原始证据整理

S2 的大逐次运行摘要、边轨迹、逐次 oracle 验证和完整 trace 被普通 Git 跟踪，与 `docs/REFERENCE_MANIFEST.md` §8 对原始日志/benchmark 原始输出的排除规则及 `docs/BENCHMARK_VALIDATION_PLAN.md` §12.2 的存储边界不符。

删除之前，S3 从精确 S2 Git objects 导出 10 个原始/小轨迹文件，逐一核对外部副本的 SHA-256 和字节数，合计 14,083,145 bytes。又用 `git cat-file blob <S2 revision>:<path>` 对两份 S2 报告及全部 8 个 TEST_ONLY 源文件计算 SHA-256。清单存于 `experiments/TEST_ONLY_BMW16_S2_SELECT_4R/results/archived_s2_evidence_manifest_2026-10-05.json`；其中同时列源文件 Git blob ID、SHA-256、字节数及原始结果哈希。外部副本位于上节路径，不被 Git 跟踪。精确 S2 Git blob 字节作为基准，避免 `core.autocrlf=true` 导致工作树文本哈希与 commit 对象字节差异。

复审核心源码哈希（SHA-256，基于精确 S2 commit blob）：

| S2 文件 | SHA-256 |
|---|---|
| `docs/decisions/BMW16_S2_DERIVED_SELECT_4R_DECISION_2026-10-05.md` | `2d6bf9193b410f0e4d37a556a30e70219ba3c3bd663e3823470225558b8774c6` |
| `docs/reproduction/BMW16_S2_DERIVED_SELECT_4R_VALIDATION_2026-10-05.md` | `60f9c286d445f878779ef0eaf9ea72c8730586621a4a5d9cfbfe100bc79d7c1e` |
| `experiments/TEST_ONLY_BMW16_S2_SELECT_4R/select4r.py` | `491be2865d2c29ed40fb33877dd73485890b069bc09c37fc9d38eaa824dc8d67` |
| `experiments/TEST_ONLY_BMW16_S2_SELECT_4R/run_matrix.py` | `5a364df5d5de0d8986f837e9c2472be2070e92ae5750e998ef2e82556987a7a9` |
| `experiments/TEST_ONLY_BMW16_S2_SELECT_4R/audit_edges.py` | `66d56b320f45b5d6eccd7ad0b0c6efc9a47d9558b9f5e35f7896d180171de0a4` |
| `experiments/TEST_ONLY_BMW16_S2_SELECT_4R/validate_results.py` | `ffede20bddfbff57eac2deb09ec635975d9b33e5c5ac25eb5dbbc323db1e8abf` |
| `experiments/TEST_ONLY_BMW16_S2_SELECT_4R/oracle_harness.cpp` | `2ce9086f416a6e94ddbbf80fef7a195741c7152d90d32b3f3a6d3b7b235e1de5` |

README、单例 CLI 和 unit source 的 SHA-256 也在同一 JSON manifest 中。

差分校验依赖也固定到 S2 base tree：S1 的 `experiments/TEST_ONLY_BMW16_DERIVED_SELECT_4R/oracle.py` SHA-256 为 `c70c847f175fb000bac97eab932922612d5504960c6f12f6326a43962ee7fbf3`；C++ `VFSS/include/moe_topk/topk_oracle.h` SHA-256 为 `5b5de74fbb8fd965846f864875f1b6c10c80ac47b76392cccfadd614836ff766`。它们是只读验证依赖，本 S3 没有修改。

S3 另用原始 S2 程序重跑同一 776 个固定向量，输出不写入仓库，独立复算证据也保存在该外部目录的 `independent_replay/`。7 个文件共 13,936,336 bytes。manifest 收录路径、SHA-256、字节数、复跑命令和分类总数。保留在 Git 的只有旧 `select4r_run_matrix_meta.json`、`select4r_oracle_validation_summary.json` 两个短汇总及 S3 哈希 manifest；S2 历史 revision 仍保留全部旧树对象。

### S3 独立复算文件哈希

| 文件 | bytes | SHA-256 |
|---|---:|---|
| `independent_edge_audit.jsonl` | 922,654 | `fcb4039b6d1eaf0cfec9eab90144a2e62a7ccb9a7ca9e41e8c25a590ea76b8d4` |
| `oracle_validation.jsonl` | 378,903 | `a62e559d881f1c920373ec7e791287a38572837f0995cb5c571b2f1f7303153f` |
| `oracle_validation_summary.json` | 1,131 | `e9f74cc1aaabd6412d64b75c6b7685ce86585bd46c34286dc2960f066804f9dc` |
| `select4r_edge_traces.jsonl.gz` | 9,221,112 | `4c635eb600b2d5be9d336a52d157ae29a0975fe5415e95119535c080430d8b73` |
| `select4r_failed_or_non-success_runs.jsonl` | 734,237 | `c8f9478f3c921880111f993ba0d41e97139048e6c961ff6886de01c761358936` |
| `select4r_run_matrix_meta.json` | 701 | `cbdaeb851ea3ef7b166a5869edb4b5311005c68797f205fa872d824e84b949e2` |
| `select4r_run_summaries.jsonl` | 2,677,598 | `ceaec6f180efb9729f24f459924d5701e58b7ede87817f1b2827d7683e55b53e` |

原始 S2 对象文件的完整哈希/大小表见 manifest；其中 gzip trace 的 repo blob SHA-256 为 `4c635e…d8b73`。S2 报告曾展示部分 text 文件的 CRLF 工作树哈希，S3 以上述精确 commit blob 导出与外部副本实测字节 hash 为准。

## 8. 复跑命令与边界

本次只复跑 TEST_ONLY Select、边审计和隔离 oracle differential；没有运行生产协议测试、secure E2E、构建协议包、密钥生成、网络或性能矩阵。

实际 cwd 为 `experiments/TEST_ONLY_BMW16_S2_SELECT_4R`，执行：

```powershell
python -B run_matrix.py --seeds 8 --small-max-n 5 `
  --out-dir 'C:\Users\28641\.codex\evidence\BMW16_S2_691260833206533882e042ca37e8a21c7b9ecc24\independent_replay'
```

边审计和 differential 的实际命令：

```powershell
python -B audit_edges.py `
  'C:\Users\28641\.codex\evidence\BMW16_S2_691260833206533882e042ca37e8a21c7b9ecc24\independent_replay\select4r_edge_traces.jsonl.gz' `
  --out 'C:\Users\28641\.codex\evidence\BMW16_S2_691260833206533882e042ca37e8a21c7b9ecc24\independent_replay\independent_edge_audit.jsonl'
python -B validate_results.py `
  --summaries 'C:\Users\28641\.codex\evidence\BMW16_S2_691260833206533882e042ca37e8a21c7b9ecc24\independent_replay\select4r_run_summaries.jsonl' `
  --traces 'C:\Users\28641\.codex\evidence\BMW16_S2_691260833206533882e042ca37e8a21c7b9ecc24\independent_replay\select4r_edge_traces.jsonl.gz' `
  --out 'C:\Users\28641\.codex\evidence\BMW16_S2_691260833206533882e042ca37e8a21c7b9ecc24\independent_replay\oracle_validation.jsonl' `
  --summary-out 'C:\Users\28641\.codex\evidence\BMW16_S2_691260833206533882e042ca37e8a21c7b9ecc24\independent_replay\oracle_validation_summary.json'
```

三个执行结果为：776 个计划/边审计通过；Python 与 C++ oracle 在 776/776 同意；候选结果 571 MATCH、204 NO_CANDIDATE、1 MISMATCH。复算输入与各输出 hashes 在 S3 manifest 与外部目录。

独立 replay 全量输出在 S3 开始时不存在；复跑严格沿用 S2 source/seed schedule。它复现的是 S2，被测源码 Git blob：`select4r.py=a9222a9309be210cc2ba7cd40572317308ce6c29`、`run_matrix.py=fff6d767b8c0389fb8254bcde6cc128f9e323c70`、`audit_edges.py=783351c345fdbef06d4fc7c34ed404f461647751`、`validate_results.py=bd88e28aed2cb04a38ef0c9ca0d9cab8b5d5ea65`。完整来源和对象身份以 `6912608…9ecc24` 为准。

## 9. 门禁

| 门禁 | S3 状态 | 依据 |
|---|---|---|
| S2 algorithm source/control and derivation | `CONDITIONAL` | A5/A7 项目恒等式有注明前提；修正规则不是论文逐字步骤，S2 概率推导没有给 finite bound，随机 coins 仅 TEST_ONLY |
| `S2_FAILURE_CLASSIFICATION` | `PASS`（独立分类） | 固定 776 replay；区分 paper U/W random labels、project cut abort、A5 undefined、错误候选；S2 原 taxonomy 需勘误 |
| `FOUR_ROUND_SCHEDULE` | `PASS`（plaintext comparison layers） | R1–R4 frozen batches/causal metadata 与代码审查；不表示 secure online round |
| `LINEAR_COMPARISON_BOUND` | `PASS`（给定 S2 project rules） | `≤127M≤254n` 推导与 1,589,457 实际调用核对；不表示预处理线性或 BB90 常数 |
| `FINITE_N_RUNNABLE_REFERENCE` | `NO-GO` | 159 个完整运行未定义；S2 失败状态虽显式，但不构成全部随机带有输出的完整算法 |
| `STABLE_KTH_DIFFERENTIAL` | `CONDITIONAL` | 570 `SUCCESS` 全通过，1 个失败分支输出错误候选；没有每次正确契约 |
| `PAPER_FINITE_FAILURE_BOUND` | `NO_FINITE_BOUND_ESTABLISHED` | Theorem 8 隐藏常数，S2 无数值 δ/n₀；观测样本不能代替 |
| `CURRENT_TOPK_CONTRACT` | `NO-GO`（与此候选组合） | 冻结契约要求每次 oracle-correct、exact-K、原序秘密 bit mask；当前候选不是完整 mask 且有概率错/undefined |
| `SECURE_PROTOCOL_I_DESIGN_GATE` | `NO-GO` | 概率契约未获负责人批准；joint-view、动态离线覆盖、图访问泄露、阈值 DCF、原序逆路由、完整 message DAG 未证明 |
| `PROJECT_EXACT_MASK` | `NOT_IMPLEMENTED` | 本阶段不改 secure runtime |
| `SECURE_E2E / FORMAL_PERFORMANCE_MATRIX` | `NOT_RUN` | secure 接入门未过，不生成猜测/零值指标 |
| `CROSS_ROUTE_PERFORMANCE_COMPARABILITY` | `NO-GO / NOT_MEASURED` | 本阶段没有 secure BMW16 计量，也没有在同口径复跑 Protocol I+AAV86 或 all-pairs；不能作性能比较 |

## 10. 后续最小可执行阶段

1. **项目负责人作契约决策**：保持每次 oracle-correct 现状并要求 exact completion 证明；或书面批准独立 probabilistic contract，确定有限 n 的 δ、输出/abort 语义、失败统计和泄露约束。当前 S2 证据不足以给负责人一个满足目标 δ 的数字。
2. **算法复核人闭合参考规格**：保留 S2 原版作为证据；另起隔离版本，将 `PAPER_RANDOM_FAILURE_PATH` 与 `PROJECT_DERIVED_BAD_EVENT_ABORT` 分开；对 A5 bracket 缺失给出完整概率性失败输出合同，或给无额外轮且保持比较上界的已证明处理；独立完成 S2 改写 A5/A7 的联合概率证明并说明 randomized output cardinality。未有 finite 数字时仍须保留 `NO_FINITE_BOUND_ESTABLISHED`。
3. **密码协议设计人闭合材料与视图**：给出 P0/P1/T 可模拟视图，端点隐藏的动态 comparator pool、one-shot/session/index binding、重边槽、访问 pattern/payload padding、联合 DCF key 相关性证明及可运行容量预检；证明 signed-key+original_index+dummy tag 在统一比较环中互异且无溢出。
4. **组合审查人闭合输出和消息 DAG**：证明 selected threshold 的秘密路由、`≤τ` DCF 成员 mask 恰 K、同一 hidden permutation 的 inverse route 和原序输出；逐消息计算从原始 score share 到 mask share 的 round/bytes，列出允许打开项并得到安全审查批准。
5. 只有上述条件满足才建立 `Protocol I + BMW16-derived Select + DCF` secure 候选，先 D≤8 conformance→冻结 oracle differential→独立 P0/P1/T E2E；容量门先于 keygen/大分配。通过后才申请完整 LAN/WAN 矩阵。BB90 替代方案身份另需治理决策，本 S3 不改写 BB90 目标。
