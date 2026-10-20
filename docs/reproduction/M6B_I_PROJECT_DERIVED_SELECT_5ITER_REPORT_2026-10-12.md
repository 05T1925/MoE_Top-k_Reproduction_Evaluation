# M6B-I 项目自有五迭代稳定 Select：规格、实现与门禁报告

日期：2026-10-12（Asia/Shanghai）

任务：`M6B-I-PROJECT-SELECT-5ITER-S1`

算法身份：`PROJECT_DERIVED`；与 BB90、BMW16 论文算法区分

明文实现标签：`PROJECT_DERIVED_SELECT_5ITER_TEST_ONLY`，revision `v1`

结果：Stage A **PASS**；Protocol I Stage B **NO_GO_FOR_INTEGRATION**

性能状态：正式 LAN/WAN 矩阵未运行；M6A V3/V4 状态未改

## 1. 执行摘要与分离的门

本任务按用户新决定，从项目自己的稳定顺序 rank 维护规则推导五迭代 Select，不再等待未闭合的 BB90 五轮来源。四次抽样 pivot-window 迭代都以精确比较 rank 安全缩小候选；第五轮对剩余候选建完整比较图并精确选出第 K 项。抽样影响比较量，不影响正确性。实现仅在独立 TEST_ONLY 目录，未改协议运行时代码。

| 门 / 历史项 | 本次登记 | 说明 |
|---|---|---|
| 新项目算法 Stage A | `PROJECT_DERIVED_SELECT_5ITER_TEST_ONLY_GATE=PASS` | 40,478 个有效案例全与冻结 C++ oracle 一致；独立边审计逐例 PASS；其中 60 个案例实际执行了五个非空比较层。 |
| 新项目算法 Protocol I Stage B | `NO_GO_FOR_INTEGRATION` | 当前 pipeline 固定全对全并公开 shuffled-domain ranks；无该算法的自适应稀疏图、阈值 DCF membership 和联合视图证明。 |
| BB90 五迭代 S1 来源门 | `SOURCE_SPEC=NO-GO_FOR_IMPLEMENTATION`；`BB90_5ITER_TEST_ONLY_GATE=NO_GO` | 历史裁决完整保留；新算法没有找到或声称找到了 BB90 步骤。 |
| BMW16-derived 四层路线 | 保持独立 | 其标签、概率/abort 分析和安全 NO-GO 不转移到新算法。 |
| S34 | 非 main 分支 schema-v3 技术诊断，非异会话独立接收 | 分支 `codex/m6b-i-bmw16-s33-metrics`，文件 `docs/reproduction/BMW16_S34_SCHEMA_V3_TECHNICAL_REVIEW_AND_ADMISSION_REPORT_2026-10-12.md`；tip `fbc4beed11f686f9e0b74ba89d23b7514e93ce24`；报告 SHA-256 `12f846ab69ba946dc3b29604959731d83944294f4a93c75dcd34c14dc7d3e5c2`；原报告记录 7 PASS / 15 FAIL inventory。 |
| M6A V3 / V4 | V3 `OPEN`；V4 LAN/WAN `NOT_ALLOWED / NOT_RUN` | 本任务没有关闭这些门。 |

明文测试 PASS 只证明实现相对规格及冻结稳定 oracle 的有限测试一致。它不证明 Protocol I 安全、DCF threshold membership、概率成本渐近式、CA 假设成立或生产可用。

## 2. 工作区、revision 与用户差异保护

| 项目 | 状态 |
|---|---|
| 隔离 worktree | `C:\Users\28641\.codex\worktrees\m6b-bb90-source-closure-s1\MoE_Top-k_Reproduction_Evaluation` |
| 本任务分支 | `codex/m6b-project-derived-5iter`，由最新本地 `main` 创建；未提交 |
| 基础 HEAD | `c3926c68fd14f270faa8b55234311071947fa080` |
| 本地 `main` / `origin/main` 起始指针 | 均为 `c3926c68fd14f270faa8b55234311071947fa080`；结束时另核验远端指针 |
| 主 checkout | `C:\Users\28641\Desktop\MoE_Top-k_Reproduction_Evaluation`，分支 `feat/m6a-performance-evaluation`；本任务未在此目录写入 |
| 开始时隔离 worktree 状态 | 延续 S1 工作：PROJECT、IMPLEMENTATION_PLAN、PAPERS.sha256、REFERENCE_MANIFEST 已有未提交差异；S1 decision/report 是未跟踪文件。本任务在保留它们的状态下新建分支 |
| 主 checkout 原有差异 | 原 4 个 tracked 用户改动和用户未跟踪的 M6A decisions/inventory 与 `siamjdiscrmath.pdf` 保留；S1 原报告记录的 tracked diff SHA-256 为 `6a212d4a037b337a98d28aa889339357fbe3c2ef3d585e24fd635c003ce5937c`。结束时再检查未被本任务改变 |

`docs/PAPERS.sha256` 是用户原有差异，在本任务未修改。S1 两份报告/决策文件以及 S1 对它们的历史状态均保留。未修改 `VFSS-baseline/`，未添加论文、密钥、证书、sidecar、构建物或大日志。

## 3. 既有来源与候选身份复核

### 3.1 BB90 S1 历史事实保持原样

[S1 来源闭合报告](M6B_I_BB90_5ITER_SOURCE_CLOSURE_S1_REPORT_2026-10-12.md) 中的来源结论仍成立：BB90 1990 正文未取得，SIAM 直连返回 403；2024 CCS 会议版 MIT 附件 ZIP 实际只有 METS 和出版 PDF；出版版 p.3036 只报告中位数场景下“五迭代 BB90”和线性比较量，没有逐轮图、参数和概率定理。该历史来源门不因允许本项目自推算法而转成 PASS。直接出处和文件哈希见 S1 报告及 [REFERENCE_MANIFEST §4.5](../REFERENCE_MANIFEST.md)。

### 3.2 BMW16-derived 四层候选不是本算法

| 项 | 可核验身份 / 限定 |
|---|---|
| 论文来源 | Braverman–Mao–Weinberg, *Parallel Algorithms for Select and Partition with Noisy Comparisons*, arXiv `1603.04941v1`；[arXiv 版本记录](https://arxiv.org/abs/1603.04941v1)。它是独立于 BB90 的论文。 |
| 论文正文核读位置 | Algorithm 5 PDF p.29；Algorithm 7 PDF p.34；Theorem 8 PDF p.11；44 页本地副本 SHA-256 见 S1 报告 §3。 |
| 明文代码 | 标签 `BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R_TEST_ONLY`；`origin/m6b-s4-protocol-i-bmw16-sample-bracket-test-only` tip `04ed6f8a352ab2277434344094471dff82f83e4b`；`select4r.py` SHA-256 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab`。该提交不在当前 `main` 历史中。 |
| 适用范围 | S4 自己构造的严格全序 median partition；任意稳定 K 由两份带 sentinel 的 median partition 归约。此构造不是 Algorithm 7 字面实现，也不继承 Theorem 8 的概率值。 |
| 失败行为 | 抽样 bracket / cap 失败时显式 `PROJECT_RANDOM_FAILURE_PATH`、不给候选；有限参数/状态不变量异常不得作为成功输出。不能假定失败后免费重试。 |
| 概率证据 | S4 报告记载理想均匀无放回抽样下，两份独立 partition 的成功率至少 `p_M²`；官方矩阵最大扩张 `M=1842` 时联合失败界 `<4.50140648×10^-7`。这是项目自己的有限参数分析，不是 BMW16 定理。 |
| 实测观察 | S4 report 记载 40,567 条小规模/对抗样本和 10,000 条冻结矩阵运行；这些是该 revision 的测试证据，不是任意输入的概率证明，也不是新算法测试。 |
| Protocol I 安全边界 | S5（`67c3f302a8564b68616f7d7247c8cee3415ace31`）接收明文算法，但组合门 NO-GO。S6（`c4f34e7e8f9fd954637573fe62b8d9b567362298`）虽通过部分键/容量算术审计，仍因自适应图、联合 shuffle/view、材料访问、DCF threshold binding、逆路由与 abort simulator 缺失而 `NO-GO_FOR_SECURE_IMPLEMENTATION`。 |

本次既未导入 S4 代码，也未将其 `p_M²`、比较界、四层图、abort 语义或安全分析用于新算法。

结构上也不是给 S4 追加一层：S4 是双 sentinel median-partition 归约，四层分别做 sample clique、全输入对两个 pivot、`V×U` 和 `W` 精确图；新算法从任意 K 的单个 `(C,r)` 状态开始，四层反复执行另一种直接 sample-clique-plus-two-pivot-stars rank-safe 转移，再由第五层闭合。没有双实例、dummy/sentinel、S4 参数或 S4 代码依赖。

## 4. 新算法规格摘要

完整参数和状态公式见[项目自有决策文档](../decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_DECISION_2026-10-12.md)。此处列出可供复核的执行摘要。

### 输入及全序

- 支持 `1≤n≤1,000,000`、`1≤K≤n`、raw signed Q20.12 `int32` 分数；`original_index` 为输入槽位 `0..n−1`。
- 全序优先键升序表示为 `(-signed_score, original_index)`，等价于 score 降序、original index 升序。
- 固定输入可任意，重复/全相等允许；算法使用与输入独立的均匀无放回样本。不存在输入随机排列假设。
- 测试的 `algorithm_seed` 与生成输入用的 `input_seed` 分开；seeded Python PRNG 仅支持复跑，不是安全随机源。

### 五个图层及状态

初始候选 `C₀={0,…,n−1}`，残余 rank `r₀=K`。每层维持不变量“全输入第 K 项是 C 中第 r 项”。

对 `i=1..4`，若未终止，令 `m=|C|`、`s=min(m,ceil√m)`；从 C 均匀无放回抽 S，按稳定次序排成 `S[1..s]`。令 `b=bit_length(n)`、`a=ceil_sqrt(2sb)`、`μ=s(r−1)/m`，整数求 `qL=clamp(floor μ−a,1,s−1)`、`qH=clamp(ceil μ+a+1,qL+1,s)`；pivot `x=S[qL]`,`y=S[qH]`。floor/ceil μ 用整数商，`a` 用 `isqrt`，无浮点取整。

本层在读任何本层结果前冻结 `V_i=C` 及简单无向边：

```text
E_i = all unordered pairs inside S
      ∪ edges from x to each C\S
      ∪ edges from y to each C\S
```

样本 clique 加两条 pivot 星边，使 x、y 在 C 中的 rank 精确可知。若 pivot rank 正好是 r，返回 pivot；若 `rank(x)<r<rank(y)`，只保留 `(x,y)` 严格开区间并令新 rank=`r−rank(x)`；若两个 pivot 都低于 r，只保留 `y` 以上并扣除 `rank(y)`；若两个 pivot 都高于 r，只保留 `x` 以下且 rank 不变。这三种更新都保留目标。

第 5 层在 `C₄` 建完整无向图、独立计数每项的全序 rank 并返回第 `r₄` 项。早期已精确命中时，余下计划层作为空终止层记录。非法输入拒绝；内部状态不变量异常返回错误、不输出阈值或成功 mask；没有随机失败、重试或 fallback。

### 正确性与概率

每个 pivot 对当前 C 中所有其他项均有一条冻结比较边，故 pivot rank 精确。按 pivot 与目标 residual rank 的关系删除不含目标的区域并准确调整 residual rank。不变量由归纳保持；第五层完整图给出精确目标。由此对每种样本带都有正确性，`Pr[错误输出]=0`、`Pr[随机 abort]=0`。这不表示比较成本恒定。

目标之前的样本数量 `H` 在给定状态下服从 `Hypergeom(m,r−1,s)`。当 q 窗未触及样本边界时，`|H−μ|≤a` 充分保证选定 pivots bracket 目标；Hoeffding 无放回界给每轮此事件补集概率 `≤2e^(−4 bit_length(n))≤n^-4`。边界截断时按完整有限状态递推计算成本。此尾界只关乎 bracket/成本，不是算法成功率；任何未 bracket 的样本仍安全更新或留到后续精确闭合。

## 5. 比较成本与轮数

四个采样层在候选大小 m、样本 s 时的无序逻辑比较数为 `C(s,2)+2(m−s)`；第五层为 `C(m₄,2)`。各层节点分别是候选集 C，最多 n；样本子图节点 s；全程最多 5n 节点层出现量。每个采样层不超过 `3m`，总 worst-case 为：

```text
12n + n(n−1)/2 = O(n²).
```

没有宣称新算法线性比较数。重复边在不同层重新计逻辑调用。跨层唯一无序关系为 `|∪E_i|`，重复调用为 `Σ|E_i|−|∪E_i|`。安全 key slot、方向请求、DCF Eval、材料字节均因没有集成而 `NOT_MEASURED`。

精确有限成本分布可由决策文档的转移递推复算。状态 `(m,r)` 上，样本第 `qL/qH` 次序统计量为 `x<y` 的概率为：

```text
C(x−1,qL−1) C(y−x−1,qH−qL−1) C(m−y,s−qH) / C(m,s).
```

将四次状态转移的精确有理概率累积，再加第五层 `C(m₄,2)`，即可得每个固定 `(n,K)` 的期望和分位数分布。正式矩阵的这些数值未计算，登记 `NOT_COMPUTED`；不以测试经验宣称渐近期望界。

一条实际五层 trace：31 项交替 `+4096/-4096` raw score，K=16，input seed 1010、algorithm seed 0；独立审计的五层无序边数为 `[65,61,38,30,45]`，总逻辑比较 239，跨层唯一边 225，重复调用 14；选中 index 30、mask 16 个 1。trace SHA-256 `b545a94036b33f8fa99b0016234bc59d732c3d2cd5a3620588d9ac797cc14e04`，audit SHA-256 `c929ac820aaf2eaa886d0f45e0206fe54d6bfa167d77d5bb4e0521b088ee2b20`。这是功能计数，不是性能结果。

### 分层轮数

| 层级 | 轮数 / 状态 |
|---|---|
| 算法迭代 | 5 个计划层（4 个随机窗口更新 + 1 个精确闭合）；若提前 exact hit，剩余层为空并在 trace 中保留。 |
| 明文比较依赖层 | 最多 5，层内边并行；后继边集等待前一层。 |
| CA 编译核心 | 若 Agarwal CCS 2024 Theorem 5.1 的 shuffle、adaptive graph、公开量等前提经独立证明适用，代入 `2k+1` 得条件值 11；当前 `NOT_PROVEN`。 |
| Protocol I 论文核心 | Theorem 4.1 的 3 轮是论文核心口径，不自动适用于本 Select。 |
| 当前仓库 Protocol I priority pipeline | 固定全对全 CmpAgg 工程 pipeline 是 2 forward +1 CmpAgg +1 rank reveal +2 reverse =6 在线阶段轮；这不是本算法，也没有阈值 DCF。 |
| 新 Protocol I 核心；raw score→原序 mask 全路径 | `NOT_DERIVED / NOT_MEASURED`。 |

## 6. Protocol I 接入与安全复用裁决

理想目标功能是 raw score shares → 稳定 key shares → 在秘密状态下完成五层 → 获得等价第 K threshold 或成员位 → DCF membership → 同置换 inverse route → 原序 XOR mask share。角色仍为 P0/P1；离线 T 只可在输入未知时发 input-independent 一次性材料，不能接触 input、rank 或 threshold，在线静默。

当前 main 的 `protocol_i_priority_pipeline_party` 要求 `ProtocolIPartyPackage` 含 padded domain 的固定全对全边材料；随后公开 masked priorities、调用全对全 CmpAgg、双方 reveal 所有 shuffled rank，再 reverse shuffle 成 mask。它没有接收本算法的动态图，也没有 `selected threshold → per-record DCF membership` runtime API。当前 `protocol_i_ucmp.cpp` 以 `keyGenDCF(Bin,64,alpha,1)` 生成两方材料，`alpha=(mask_left−mask_right) mod 2^Bin`；每方一次 `eval_strict_lt` 用两个 DCF 点计算严格 `<`，party key 为 one-shot，比较位宽 34–53。只有当 key 编码/位宽/掩码 ABI、函数方向、阈值边界和无回绕条件完全一致时，uCMP 才是候选组件；它不能视作任意方向或非严格 `≤` membership 的安全合同。

新图会让 pivot、活跃候选、边地址和材料领取受之前比较结果影响；跨层重复关系需要新鲜独立 slot。将图公开可能泄露比较顺序和 pivot 关系；隐藏图则需 secret edge scheduler、oblivious gather/compact 和 key retrieval。全对全池只覆盖任意边，不等于隐藏访问或联合视图模拟。应重新证明输入/分数分享、shuffle view、样本和边 transcript、读取/未读材料槽、abort/断连、DCF membership、逆路由以及与 mask 输出的组合。S4 的 whole-pool、自适应 transcript 或 shuffle-output 条件论证不能套用。

因此 Stage B 暂停在安全/接口门，不创建实验 alias，不修改 `VFSS/`。最小后继工作应先提交新算法专属 Protocol I 消息 DAG、key/material ABI 和 P0/P1 联合视图模拟设计；独立 review 后再建最小 experimental runtime，并按 conformance → oracle differential → 独立进程 E2E。由于本次未接入，不报虚构 edge material、DCF Eval、在线轮数或通信值。

## 7. 测试、命令与结果

| 覆盖 | 最终结果 |
|---|---:|
| `n≤7` 所有严格序全排列、每种输入所有 K | 40,319 / 40,319 oracle PASS |
| 重复/全相等/负数/Q20.12 signed int32 边界及随机输入 | 覆盖 |
| 固定输入改变 algorithm seed | 32 个种子，32 个不同首轮样本，均成功 |
| 实际五层非空 edge graph | 60 个案例；代表 trace 的每层边和跨层重复均由独立 auditor 核验 |
| `K=1`、`K=n`、中位、非二次幂 | 覆盖 |
| 非法输入 | 5/5 拒绝，无成功输出 |
| oracle differential 总数 | 40,478 / 40,478，0 mismatch |
| 独立 edge/cause audit | 40,478 / 40,478 PASS |
| 保留的失败记录 | `failures.jsonl` 为空；没有试次被删除、过滤或重试掩盖 |

环境：Windows 11 `10.0.26200-SP0`、Python 3.13.7、g++ MinGW-W64 8.1.0。冻结 oracle 编译自 `VFSS/include/moe_topk/topk_oracle.h`；原始 score 语义/稳定比较由仓库 C++ oracle 决定。

复跑命令（worktree 根目录）：

```powershell
$env:PYTHONDONTWRITEBYTECODE='1'
python experiments/TEST_ONLY_PROJECT_DERIVED_SELECT_5ITER/test_select5iter.py `
  --out-dir 'C:\Users\28641\.codex\evidence\PROJECT_DERIVED_SELECT_5ITER_2026-10-12\final'
```

脚本会将可执行 oracle、全部成功案例、失败 JSONL、非法输入记录、代表性完整边迹及独立 audit 写入仓库外目录。最终证据哈希：

| Artifact | SHA-256 |
|---|---|
| `results.jsonl`（40,478 cases） | `ac80735543bba5d1e3c6c3507ce802cb3ce415f034abcd64778d07cd4172e9d1` |
| `failures.jsonl`（empty） | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| `invalid_input_checks.jsonl` | `f72a4126a69ad2ce5edaac6b6b585d311af7fabe6ac37360dd3c049f0b8f8c26` |
| `representative_trace.json` | `b545a94036b33f8fa99b0016234bc59d732c3d2cd5a3620588d9ac797cc14e04` |
| `representative_trace_audit.json` | `c929ac820aaf2eaa886d0f45e0206fe54d6bfa167d77d5bb4e0521b088ee2b20` |

执行时 Git HEAD 为 `c3926c68fd14f270faa8b55234311071947fa080`，当前任务源文件未提交；以下 SHA-256 固定本次实际测试的工作树源码：

| Source | SHA-256 |
|---|---|
| `select5iter.py` | `8b8d3dbd087d601a4962fe61c7b35546aa834cebd0ed4736b4e6d5dca204606b` |
| `audit_edges.py` | `b84591c49ec9f67484258011b0685ce9f6cbcebb56685457a80a571bfbcf2fc0` |
| `frozen_oracle.cpp` | `a30fe76ab09552ea532ca0e732a19eb925e3d35db0eb99e1728a087f5e48a457` |
| `test_select5iter.py` | `fe933e6d1d3e298e8b9144e8764468693b000cd6b1c12d3a1eb643e179c4cf56` |

没有运行正式 LAN/WAN 性能矩阵；测试比较数只作功能 trace/audit 证据。

## 8. 修改文件清单

本任务新增或编辑：

1. `PROJECT.md` — 追加项目自有 Select 支线的状态，保留 S1、S34、V3/V4 历史。
2. `docs/IMPLEMENTATION_PLAN.md` — 分别登记 BB90 S1 NO-GO 和项目算法 Stage A PASS / Stage B NO-GO。
3. `docs/REFERENCE_MANIFEST.md` — 新增 §4.6，登记算法标签、来源分类、复跑和哈希。
4. `docs/decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_DECISION_2026-10-12.md` — 完整算法、证明、概率/成本递推、轮数和 Stage B 条件。
5. `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_REPORT_2026-10-12.md` — 本报告。
6. `experiments/TEST_ONLY_PROJECT_DERIVED_SELECT_5ITER/select5iter.py` — 纯明文五层稳定 Select。
7. `experiments/TEST_ONLY_PROJECT_DERIVED_SELECT_5ITER/audit_edges.py` — 不 import Select 的独立 trace/边/因果审计器。
8. `experiments/TEST_ONLY_PROJECT_DERIVED_SELECT_5ITER/frozen_oracle.cpp` — 调用冻结 VFSS C++ 稳定 Top-K oracle 的批量 harness。
9. `experiments/TEST_ONLY_PROJECT_DERIVED_SELECT_5ITER/test_select5iter.py` — 全排列、固定/随机/边界输入的 differential 和外置证据归档。
10. `experiments/TEST_ONLY_PROJECT_DERIVED_SELECT_5ITER/README.md` — 标签、接口、边界与复跑方式。

S1 的 `docs/decisions/M6B_I_BB90_5ITER_SOURCE_CLOSURE_S1_DECISION_2026-10-12.md`、`docs/reproduction/M6B_I_BB90_5ITER_SOURCE_CLOSURE_S1_REPORT_2026-10-12.md` 是进入本任务时已存在的未提交历史证据，本任务保留但未改写。`docs/PAPERS.sha256` 是用户既有差异，本任务未编辑。没有修改 `VFSS/` 或 `VFSS-baseline/`。

## 9. 后续门与未完成事项

1. `BB90_5ITER_TEST_ONLY_GATE` 仍 NO_GO；若未来要恢复 BB90 复现任务，仍需获取正式/作者可核验的逐轮算法、参数、定理条件和失败合同。
2. 本项目自有 Select 明文门已通过，但 Stage B 尚需安全设计评审；至少补齐 secret adaptive graph、输入无关离线材料寻址、DCF threshold membership ABI、原序 mask 逆路由、abort 和 P0/P1 联合视图模拟。
3. 精确有限成本分布可按递推对目标 `(n,K)` 计算；本次没有计算完整矩阵的期望/分位数，不能声称期望线性或高概率比较上界。
4. 新算法的 CA 假设、Protocol I 核心轮数和完整 raw-score→mask 消息 DAG均 `NOT_PROVEN / NOT_DERIVED`。
5. M6A V3 仍 OPEN；V4 LAN/WAN 仍 NOT_ALLOWED / NOT_RUN；本任务没有启动正式性能测试。

