# M6B-I 项目自有五迭代 Select 决策

日期：2026-10-12（Asia/Shanghai）

身份：`PROJECT_DERIVED`；不属于 BB90 或 BMW16 论文算法复现

实现标签：`PROJECT_DERIVED_SELECT_5ITER_TEST_ONLY`

安全候选标签（未实现）：`Protocol_I_PROJECT_DERIVED_SELECT_5ITER_DCF_EXPERIMENTAL`

## 1. 决策与既有状态

用户明确允许推导项目自有五迭代稳定 Select。因此本设计不以 BB90 正文或 2024 论文未公开的五轮步骤为来源前提。S1 的 `SOURCE_SPEC=NO-GO_FOR_IMPLEMENTATION` 与 `BB90_5ITER_TEST_ONLY_GATE=NO_GO` 继续有效；本决策不把它们改成 BB90 来源闭合。

BMW16（Braverman–Mao–Weinberg, 2016）及其项目派生四层候选仍是独立路线。该候选的概率界、比较常数和安全结论均不转移到此算法。本设计不是在 S4 源码末尾机械附加一层：S4 是两 sentinel median-partition 归约，四层依次为 sample clique、全输入到两个 pivot、`V×U`、`W` 精确图；本算法直接维护任意 K 的一个 `(C,r)` 状态，四次使用同一套新定义的 sample-clique-plus-two-pivot-stars 转移，第五层再精确闭合，且没有双实例、dummy/sentinel 或 S4 参数。未导入或改写 S4 代码。

实现按“先冻结数学规格、再建 TEST_ONLY”的顺序完成，当前明文 Stage A = PASS。它不自动准入 Protocol I、DCF、正式性能矩阵或生产安全结论。S34、M6A V3/V4、BMW16-derived 安全门、S1 BB90 来源门分别保持原状态。

## 2. 四类证据

| 类别 | 本决策中的内容 |
|---|---|
| 论文定义 | BB90、BMW16 和 2024 论文的历史来源边界见 S1 报告；它们不定义本算法。Agarwal CCS 2024 §5 / Theorem 5.1 只可在其 CA 编译前提逐项满足后作条件轮数参考。 |
| 本地参考行为 | VFSS 稳定 Top-K oracle 使用 signed Q20.12 原始整数、score 降序、original index 升序；当前 Protocol I pipeline 是固定全对全 CmpAgg，不是本算法实现。 |
| 项目推导 | 下述五层图、采样 pivot、候选集合更新、精确终止和成本递推。 |
| 待验证设想 | 本算法新图的 CA 编译、安全模拟、离线材料布局、DCF 阈值成员接口和完整 Protocol I 消息 DAG。 |

## 3. 输入、顺序与随机模型

输入为 `n` 个 raw signed Q20.12 `int32` 分数，且 `1 ≤ n ≤ 1,000,000`；`1 ≤ K ≤ n`。每项身份为其原始 `original_index ∈ [0,n)`。比较全序为

```text
a precedes b iff score[a] > score[b],
  or score[a] == score[b] and original_index[a] < original_index[b].
```

即按 `(-signed_score, original_index)` 升序选择第 `K` 项。固定输入允许任意分布、重复值和全相等；不要求输入次序是随机排列。每个采样轮在当前候选项身份上均匀无放回采样，且给定当前状态后使用新鲜、相互独立的理想随机带。实现中的 seedable PRNG 仅用于 TEST_ONLY 可重放；不作密码学随机性声明。

## 4. 五迭代算法规格

状态为 `(C,r)`：`C` 是原输入身份的非空子集，按稳定全序诱导；`r` 是目标在 `C` 中的 1-based rank。初始状态 `C={0,…,n−1}`、`r=K`。归纳不变量是全输入第 K 项恰为 `C` 中第 r 项。

令 `choose(a,b)=binom(a,b)`（越界为 0）。对最多四个采样迭代 `i=1,…,4`：

1. 若前轮已确定答案，记录空 `H_i` 并保持终止状态；否则令 `m=|C|`。取 `s=min(m,ceil(sqrt(m)))` 项的均匀无放回样本 `S⊆C`，按本算法稳定全序排序为 `S[1..s]`。若 `m=1`，该项即精确答案。
2. 令 `b=bit_length(n)`、`a=ceil_sqrt(2 s b)`（精确整数实现：`a=isqrt(2sb)+[isqrt(2sb)^2<2sb]`）、`μ=s(r−1)/m`，并定义

   ```text
   qL = clamp(floor(μ−a), 1, s−1)
   qH = clamp(ceil(μ+a)+1, qL+1, s)
   x = S[qL], y = S[qH].
   ```

   `clamp(v,lo,hi)=min(hi,max(lo,v))`。`floor(μ)`、`ceil(μ)` 用整数商计算。因为 `m>1` 时 `s≥2`，两个 sample pivot 不同。取整均如上，不用浮点运算。
3. 在读本轮比较结果之前冻结简单无向图：`V_i=C`，

   ```text
   E_i = { {u,v} : u,v∈S, u≠v }
         ∪ { {x,z} : z∈C\S }
         ∪ { {y,z} : z∈C\S }.
   ```

   每条无序边只出现一次；sample clique 已包含 pivot-pivot 及 pivot-to-other-sample 边。端点是输入身份，不是秘密 rank。全轮所有边在轮开始时一次冻结；本轮后继图只依赖前轮结果、当前候选状态和本轮独立样本。
4. 用 `E_i` 的比较结果精确计算 `x,y` 在 `C` 中的 rank `p_x,p_y`。若 `p_x=r` 或 `p_y=r`，返回相应输入身份。否则按以下互斥情况更新：

   ```text
   p_x < r < p_y: C'={z∈C : x<z<y}; r'=r−p_x
   p_y < r:       C'={z∈C : y<z};   r'=r−p_y
   p_x > r:       C'={z∈C : z<x};   r'=r
   ```

   次序比较使用完整稳定 key。`p_x<p_y` 恒成立。任何违反 `1≤r'≤|C'|` 的状态是内部不变量错误，抛出错误且不产生答案；不将其记作概率失败，不重试。

第 5 迭代在剩余 `C` 上冻结完整无向图 `H_5=(C, {{u,v}:u<v})`，由全图比较计数每项在 `C` 内的精确 rank，返回第 `r` 项。若前轮已经确定答案，`H_5` 是明确记录的空终止轮。合法输入下不会有随机 abort；无效 n/K/score 在计算前拒绝。内部不变量错误或资源耗尽（例如内存不足）会传播为错误并且不产生结果；无 fallback、oracle 参与、重试或错误阈值输出。

### 正确性

每条采样 pivot 到 `C` 每项的关系均存在于 sample clique 或 pivot 星边，故两 pivot 在 `C` 中的 rank 是精确的。若目标 rank 落在二 pivot 之间，保留严格开区间并减去 `p_x` 个已知前驱；若目标位于两 pivot 同一侧，仅删除已知不含目标的一侧并按删除量更新 rank。因此不变量逐轮保持。第 5 轮完整图给出全序中每项的精确 rank；所选身份必为原输入第 K 项。该证明对任意抽样结果、任意固定输入成立，所以理想抽样下 `Pr[错误输出]=0`、`Pr[概率 abort]=0`。随机性只影响候选大小和比较量。

## 5. 有限规模成本与概率

### 每轮边数与复杂度

采样轮当状态大小 `m>1`、`s=min(m,ceil√m)` 时，

```text
|E_i| = binom(s,2) + 2(m−s).
```

若 `s=m`，此式即完整 sample clique。第 5 轮为 `binom(m_4,2)`。每个采样轮节点数 `m_i≤n`，样本节点数 `s_i`；最终轮节点数 `m_4≤n`。因 `s≤√m+1`，每个采样轮 `|E_i|≤3m`；四轮最多 `12n` 次逻辑比较，最终轮最多 `n(n−1)/2`。故最坏总量严格受

```text
12n + n(n−1)/2 = O(n²)
```

约束；没有宣称线性最坏或期望比较数。超轮重复关系照常再次计为逻辑比较。跨轮唯一无序关系为 `|∪E_i|`，重复调用为 `Σ|E_i|−|∪E_i|`，由冻结边迹计算；不把二者混写。明文每无序边执行一次比较。安全材料槽数、方向和 DCF Eval 数目前 `NOT_MEASURED`，因未接入协议。

### 精确有限 n 成本分布

给定状态 `(m,r)`，均匀样本按 rank 表示成 `[m]` 中的 s 元子集。若样本第 `qL,qH` 个次序统计量为 `x<y`，则精确概率为

```text
P(X_qL=x, X_qH=y | m,r)
 = C(x−1,qL−1) C(y−x−1,qH−qL−1) C(m−y,s−qH) / C(m,s).
```

将此分布逐层递推四轮：若 `x=r` 或 `y=r`，转入零后续代价的吸收态；否则按上述三种状态更新转移。每个非终止采样状态加入 `binom(s,2)+2(m−s)` 边，第 5 轮按剩余状态加入 `binom(m,2)`。该有限求和给出每个固定 `(n,K)` 的**精确**期望比较数以及整个比较数分布，可逐状态用整数二项式系数复算。未对完整 benchmark `(n,K)` 矩阵运行此递推，故这些实例的数值期望/分位数为 `NOT_COMPUTED`；不以测试观察代替理论值，也不主张比最坏 `O(n²)` 更强的渐近期望界。

样本中目标前驱数 `H~Hypergeom(m,r−1,s)`。若未触发 q 边界截断，事件 `|H−μ|≤a` 足以保证两个 pivots bracket 目标。Hoeffding 无放回界及 `b=bit_length(n)≥log₂n` 给 `Pr(|H−μ|>a)≤2 exp(−2a²/s)≤2 exp(−4b)≤n^(−4)`（`n≥2`）。边界截断时使用上面的精确次序统计量递推计算成本分布。无论 bracket 事件是否发生，候选更新仍然精确，故这个尾界不是成功率/正确率条件。输入分布、随机排列假设和成功率门槛均不影响正确性。

## 6. 比较深度、CA 与 Protocol I 轮数

| 指标 | 本算法裁决 |
|---|---|
| 算法迭代 | 最多五层，四次随机窗口缩减加一次精确闭合；终止后的空层显式记录。 |
| 明文并行比较依赖层 | 五层；每层内部全部边并行，`H_(i+1)` 必须等到第 i 层结果。 |
| CA 编译核心轮 | 若且仅若 Agarwal CCS 2024 Theorem 5.1 对此自适应图、公开量、shuffle 和输入条件逐项成立，通用 `2k+1` 代入 `k=5` 得 11；当前 `NOT_PROVEN`。 |
| Protocol I 论文核心轮 | Agarwal Theorem 4.1 为 3 轮核心；不因算法比较深度为 5 而变成新算法实现轮数。 |
| 当前仓库 Protocol I 工程 pipeline | main 中既有优先级 CmpAgg pipeline 固定全对全，记录 2 forward +1 CmpAgg +1 rank-reveal +2 reverse =6 在线轮；不是本算法，也不是论文三轮核心。 |
| 新算法 Protocol I 核心轮 | `NOT_DERIVED`；没有接入消息 DAG。 |
| raw-score→原序 Top-K mask 完整轮 | `NOT_DERIVED / NOT_MEASURED`。需要 raw-score→稳定 key、秘密 pivot/rank 更新、最终成员判断、reverse route 和 mask 输出的完整 DAG。 |

## 7. Protocol I 接口与安全门

预期功能流为：两方 signed-score shares → 稳定 composite-priority-key shares → 隐藏于任何单方的 sample/pivot 状态及比较 → 共享第 K 阈值或其等价成员位 → DCF membership → 沿输入位置映射逆路由 → 原序 XOR Top-K mask。具体角色分工、阈值隐藏、错误/abort 处理和消息 DAG尚未证明。

本 checkout 的 `protocol_i_priority_pipeline_party` 只接受预处理**全对全** `ProtocolIPartyPackage`：每对唯一 `ProtocolIUcmpPartyMaterial`，固定 CmpAgg，之后向双方揭示 shuffled-domain 全部 rank，再逆 shuffle。当前 uCMP 构件在 `protocol_i_ucmp.cpp` 以 `keyGenDCF(Bin,64,alpha,1)` 生成两方材料，其中 `alpha=(mask_left−mask_right) mod 2^Bin`；一次 `eval_strict_lt` 在每方以两个 DCF 点计算严格 `<` 份额，party key 为 one-shot，比较位宽限 34–53。它不是可直接调用的 DCF membership API。若未来用它比较选中 key 与记录 key，须证明同一 key 编码/位宽/掩码 ABI、函数方向、严格/非严格边界与不回绕条件；当前代码不支持逐轮稀疏/自适应图材料、秘密 rank 更新、单一 threshold 的 DCF membership 或本新算法独立进程端到端路径。

因此 Stage A 通过只批准明文 TEST_ONLY。Stage B 仍 `NO_GO_FOR_INTEGRATION`，直到：新消息级协议与自适应图安全论证完成；可输入无关离线预生成且一次性绑定的材料池实现；DCF 函数方向/阈值/表示 ABI 经 conformance；阈值、样本、图地址及失败 transcript 不泄露；输出 exact-K 原序 mask；新路径经过 conformance、冻结 oracle differential、独立进程 E2E。不得复制协议栈、创建 secure alias 或将既有 all-pairs/安全结论移植。

