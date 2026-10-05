# BMW16 S4：四轮项目衍生 Select 验证报告

日期：2026-10-05

实现标签：`BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R_TEST_ONLY`
结论范围：明文 TEST_ONLY 排序选择；**不是** BB90 复现、BMW16 Algorithm 7 修复版、Protocol I secure runtime 或完整 Top-K mask。

## 1. 执行结论

S4 保留 S1/S2/S3 的 NO-GO 与失败证据，另行实现四轮 sample-bracket Select。该构造使用 BMW16 论文的 noiseless comparison 模型及 Appendix A 两哨兵任意 rank 归约，但 R1–R4 的抽样窗口、端点构造、有限规模规则和概率界均为 `PROJECT_DERIVED`。它**不继承** Theorem 8 的概率或常数。

最终源码经独立边审计、小规模全排列与重复值验证、冻结的正式规模种子矩阵、Python/C++ oracle 交叉核对。未实现 secure Protocol I、DCF、shuffle 逆路由或原序秘密共享 mask；也未运行 LAN/WAN 性能矩阵。

| 门禁 | 结论 | 依据和限制 |
|---|---|---|
| S2_FAILURE_REPLAY | PASS | S4 worktree 原样重放 776 条；四份复跑归档文件哈希与 S3 归档逐一相同。 |
| BMW16_ALGORITHM_5_7_REPAIR | NO-GO / NOT CLAIMED | 纸面 Algorithm 7 的残余 rank 行仍有内部不一致；新实现明确是另一套项目推导。 |
| PROJECT_DERIVED_SELECT_PROOF | PASS（理想均匀抽样模型） | R1/R2 bracket、R2 `U` cutoff、R3 窗口、R4 精确基数与两哨兵交集均有单独不变量。 |
| FOUR_CAUSAL_COMPARISON_LAYERS | PASS | R1→R2→R3→R4 每层先冻结全边清单，再消费比较结果；两份中位 Partition 同步调度。 |
| FINITE_N_REFERENCE | PASS | 任意 `n≥1, 1≤K≤n` 定义扩张、整数量化和显式随机中止；不排序补答案或重试。 |
| O_N_COMPARISONS | PASS（项目常数） | 两份中位实例合计 `≤4234M≤8468n`；不声称与 BB90 常数相同。 |
| STABLE_KTH_DIFFERENTIAL | PASS（被测样本） | 40,567 个小规模/边界样本、10,000 个正式矩阵样本与冻结 Python oracle 一致；不是对所有输入的经验穷举。 |
| PROTOCOL_I_SECURE_IMPLEMENTATION | NO-GO | 接口与阻塞项已列出；适应性图隐藏、离线材料、在线阈值 DCF、精确失败契约和逆路由均未闭合。 |

## 2. 工作区与来源身份

工作在独立 worktree `C:\Users\28641\.codex\worktrees\bmw16-s4\MoE_Top-k_Reproduction_Evaluation`、分支 `codex/bmw16-s4` 完成。起点为 S3 revision `59c75ff1ef21784bd54f266ab9bd6e631444ccae`；其历史父链保留 S2 `691260833206533882e042ca37e8a21c7b9ecc24` 和 S1 `0a0593deaccb55225566da6af3aac661814e717c`。S4 没有改写这些提交。

桌面主工作区的活动分支为 `feat/m6a-performance-evaluation`，HEAD `c3926c68fd14f270faa8b55234311071947fa080`；本次检查时 `main` 与 `origin/main` 均指向同一 revision。主工作区已有 `PROJECT.md`、实施计划、来源清单和本地 PDF 差异；S4 未触碰或暂存这些内容。`VFSS/`、`VFSS-baseline/`、E20/E21、PR #28、本地参考工程和 PDF 均未修改。

| 本地来源 | 页数 | SHA-256 | 来源状态 |
|---|---:|---|---|
| `Papers/1603.04941v1.pdf` | 44 | `F46F83CBA279F37E9E3AAA0D64B145FDFCFBC52C0C9C8B79BACEC1259CA9CA23` | BMW16 主要算法来源，PDF 原件只读，不入 Git。 |
| `Papers/017.pdf` | 249 | `3EAE3A93F12AB71EEA8AC040167F21351FCF3FF54DC423985A04087E3E90CB15` | 作者相关学位论文的交叉核对；不是独立证明，也不替代 2016 论文原页。 |

BB90 原文没有用于填补 BMW16 步骤，也没有从其常数推导本算法。

## 3. S2 失败诊断与复跑

S2/S3 记录的 776 条样本结论保持不变：`SUCCESS=570`、`PAPER_RANDOM_FAILURE_PATH=47`、`UNDEFINED/INVALID_FINITE_CASE=159`；候选结果中 oracle 匹配 571、无候选 204、错误候选 1。40 条正式规模样本只有 16 条成功；`n=256,K=8`、算法种子 `2707453233` 的失败分支曾输出 index 244，而 oracle 为 169。

主要根因是把渐近证明的 slack 以 unit leading constants 用在小规模参数上；S2 的 `D=ceil(M^(25/36))` 没有数值吸收 Theorem 5 proof 中的常数。S2 的 U/W 随机标签分支也不保证稳定 rank 精确。S4 在 S2 worktree 运行 S3 冻结命令并重现全部 776 条，以下归档 SHA-256 与 S3 manifest 相同：

| 文件 | SHA-256 | 与 S3 归档 |
|---|---|---|
| `select4r_edge_traces.jsonl.gz` | `4c635eb600b2d5be9d336a52d157ae29a0975fe5415e95119535c080430d8b73` | identical |
| `select4r_failed_or_non-success_runs.jsonl` | `c8f9478f3c921880111f993ba0d41e97139048e6c961ff6886de01c761358936` | identical |
| `select4r_run_matrix_meta.json` | `cbdaeb851ea3ef7b166a5869edb4b5311005c68797f205fa872d824e84b949e2` | identical |
| `select4r_run_summaries.jsonl` | `ceaec6f180efb9729f24f459924d5701e58b7ede87817f1b2827d7683e55b53e` | identical |

S2 复跑的原始文件位于 `C:\Users\28641\.codex\evidence\BMW16_S4_2026-10-05\s2_replay`；S3 原归档仍只读保留在 `C:\Users\28641\.codex\evidence\BMW16_S2_691260833206533882e042ca37e8a21c7b9ecc24`。

S2 原样复跑命令（在 S2 worktree 根目录执行）：

```powershell
python -B run_matrix.py --seeds 8 --small-max-n 5 `
  --out-dir 'C:\Users\28641\.codex\evidence\BMW16_S4_2026-10-05\s2_replay'
```

S4 开发中另发现并修复一个**S4 新代码**的窗口构造缺陷：曾用 `U` 的输入槽位切片充当 R3 rank 区间 `W`，但输入槽位不等于比较顺序。重复值压力样本触发 200 条 `COMPLETED_WRONG`，没有被当作随机失败或成功候选。精确失败种子、输入哈希和计数保存在外部证据 `small_and_adversarial_pre_r3_window_fix/small_and_adversarial_failures.jsonl`（SHA-256 `5b0d40fae496cb9cf8a9a531275338d7df3b10ed0e4f70e4c57d83aee7593af8`）；例如 `n=65,K=8`、算法种子 `8320890985398024223`。修复后 `W` 由已经执行的端点样本比较行筛为 `x≤u≤y`，不新增边或轮次。修复后全部 40,567 条小规模与对抗记录成功。

## 4. 论文证据与实现身份

| 事实 | 证据分类 | 2016 PDF 位置 | 本项目结论 |
|---|---|---|---|
| noiseless comparison rounds 与严格序模型 | `PAPER_DIRECT` | p.7，§2 | 一层中的比较边必须在该层开始前已确定。 |
| A5 二层 skeleton / `r=1` 源步骤 | `PAPER_DIRECT` | p.29，Algorithm 5；证明 pp.29–30 | 供 S2 纸面审计使用；S4 新构造不声称实现此算法。 |
| Theorem 8 四轮、`O(n)`、高概率精确 median partition | `PAPER_DIRECT` | p.11，Theorem 8 | 定理只写 `0<epsilon<1/18`；S4 不继承该概率或比较常数。 |
| Algorithm 7 参数写法及输出对象 | `PAPER_DIRECT` | p.34，Algorithm 7 | 算法行写 `epsilon∈[0,1/18]`，与 Theorem 8 的严格开区间不同；直接输出 Partition，不直接返回唯一 Select。任意 K 另用 Appendix A 归约。 |
| A7 R3 目标 rank `n/2−|R(2)|` | `PAPER_DIRECT`（伪代码与定义存在不一致） | p.34，Algorithm 7 line 11 及证明 | `R(2)` 是高 cut prefix；S2 所用低 prefix 修正属于项目推导且要求 cut 真正 bracket。S4 不沿用该步骤。 |
| 两哨兵、两份 Partition 并行、任意 rank 归约与 `p²` | `PAPER_DIRECT` | p.16，Appendix A Lemma 1 | 两份调用使用独立理想随机带；有限实现的 PRNG 不构成密码学随机数声明。 |
| S4 R1–R4 的 sample、窗口及 cutoff 公式 | `PROJECT_DERIVED` | 本决策 §4–6 | 独立 finite rule 与显式失败状态。 |
| S4 成功率、oracle 差分和边数 | `TEST_ONLY_OBSERVED` | 本报告 §6–8 | 只适用于冻结被测样本，不代替概率证明。 |

S2 `r=1` 的 A5 推导把 `S1` 的 sample size 记作 `s`，把 `k1=floor(s/2)` 解释成一基目标秩 `k1+1`。这是对 Algorithm 5 字面 `k1=N/2` 的 `PROJECT_DERIVED` 修正：PDF p.10 §4.1 正文要求 bracket `S1` 自己的中位数，而不是 full input 的 `N/2`；S1 记录的 `N=348, |S1|=50` 已显示字面秩不可达。R1 的 `S1×T1` 比较可算出 T1 pivot 在 S1 的 lower-count，但只有两边 sample pivot 同时存在才有区间，抽样不保证总有 bracket。S4 新实现不把 S2 A5 调用纳入自身证明。

Algorithm 7 PDF p.34 直接输出 Partition。其 R3 文字 `h−|R(2)|` 中，`R(2)` 是高 cut 的拒绝前缀；在 A7 中位 `h` 的目标落在两 cut 之间时，这个量可能为负。项目推导需使用低 cut 大小 `c_low`，残余 rank 为 `h−c_low`，并以两个观察 cut 真正 bracket `h` 为先决条件。S4 选择另一项目构造，不把修正写回论文算法，也不沿用 Algorithm 7 的 `U/W` 随机标记分支。

## 5. 可运行算法与正确性要点

一个中位 Partition 输入严格升序、偶数规模 `M≥2` 的全序集合，目标是拒绝最低的 `h=M/2` 项。项目参数全部使用整数规则：

```text
s = min(M, ceil(8 sqrt(M)))
if s == M: qL=h; qH=h+1
else: a=ceil(4 sqrt(s)); qL=max(1,floor((s+1)/2)-a); qH=min(s,ceil((s+1)/2)+a)
Ucap = min(M,ceil(8 M^(3/4)))
L = ceil(2 sqrt(M)); B=2L+1
```

R1 均匀无放回取 `s` 项并比较样本全部无序对；pairwise ranks 得 `x=S[qL]`,`y=S[qH]`。R2 把每个输入项比较到两个 pivot，省去自比较；令 `P={u:u<x}`、`Q={u:y<u}`、`U=[x,y]`、`c=|P|`、`q=h−c`。只有当 `c≤h≤|{u:u<y}|` 且 `|U|≤Ucap` 时继续。若 `q=0`，R2 已给出精确 cut，直接完成该 Partition。

R3 取 `v=min(|U|,ceil(8|U|/sqrt(M)))` 个 `V⊆U`，比较 `V×U` 中每个无序边一次；样本 rank 因而在 `U` 中精确可知。设 `i` 是最大 `≤q` 样本 rank，`j` 是最小 `>q` rank；空侧用已知虚拟 rank 0 或 `|U|+1`。`W` 是两个 endpoint 之间的**元素集合**，通过对应 endpoint 与 U 的已有 R3 比较结果确定，不使用输入槽位排序。若 `|W|>B`，显式中止。R4 比较 W 全部无序对，精确排序 W 并拒绝其中前 `q−(i−1)` 项（虚拟 lower endpoint 的 `i−1` 定义为 0）。于是精确拒绝量为 `c+(i−1)+(q−(i−1))=h`。

任意 `(n,K)`：项目优先级是 score 降序、`original_index` 升序。S4 明文实现将全序反向写作 `(signed_score,−original_index)` 升序；目标 rank `r=n−K+1`。令 `N=n+1`、`Lpad=max(N−2r,0)`、`Hpad=max(2r−N,0)`，则 `M=N+Lpad+Hpad=2·max(r,K)` 为偶数。低 sentinel 实例输出接受集合，高 sentinel 实例输出拒绝集合；其真实项分别满足 `r'≥r` 与 `r'≤r`，交集恰为唯一第 K 优先项。K=1、K=n、奇偶 n、中位两侧的充填推导见决策文档 §6。

测试 dummy 使用独立 category tag，位于全部真实键之外，因此合法 `INT32_MIN/MAX` 不与 sentinel 冲突。`original_index` 从输入携带，不以洗牌 slot 替代。所有隔离检查/比较都有状态或边计数；算法无 oracle、全排序、重试和未计入 fallback。

## 6. 有限规模概率与成本

本构造没有 `epsilon` 参数。主论文 PDF p.34 的 Algorithm 7 行写 `epsilon∈[0,1/18]`，而 PDF p.11 的 Theorem 8 只保证 `0<epsilon<1/18`；引用定理时采用严格开区间，不能把端点写法差异抹平。S4 方案不继承 Theorem 8 概率。对固定全序，R1 样本中最低 h 项数 `H~Hypergeometric(M,h,s)`，精确 bracket 失败率为 `Pr[H<qL or H≥qH]`。R2 复核 pivot bracket，故 R1 bracket 外的运行会显式 abort 而不产生随机标签答案。

当 `M≤4096`，`Ucap=M` 不会触发；更大时，hypergeometric Chernoff + M 个 rank 区间的 union bound 给 `δcap≤M exp(−12 M^(1/4))`。R3 每个需要命中的完整 L 长 rank 窗未被 V 命中的概率至多 `exp(−vL/u)≤exp(−16)`；上下两侧合计 `δwindow≤2e^(−16)`。一份中位 Partition 的成功率至少 `pM=1−δbracket−δcap−2e^(−16)`；两份独立理想随机带的 Select 成功率至少 `pM²`。抽样失败输出 `PROJECT_RANDOM_FAILURE_PATH`、无候选；rank/cardinality 不变量被违反为 `COMPLETED_WRONG`；有效 `(n,K)` 没有未定义有限参数。实现使用 SHA-256 domain-separated 固定种子与 Python `random.Random`，仅供可复跑测试，并不声称独立理想带或安全随机性。

每份 Partition 的 R1/R2/R3/R4 最坏比较调用分别 `≤42M,2M,2048M,25M`。两份并行实例因此为 `≤84M,4M,4096M,50M`，合计 `≤4234M`。任意 K 有 `M=2·max(n−K+1,K)≤2n`，总比较调用 `≤8468n`。U 大时采样截断 `ceil(8M^(3/4))`；考虑 ceil 后 `vU≤657M<2048M`。R1 ceil、R2 自比较、省略、R3 内部 V–V 去重、R4 W cutoff 均已计入上界。跨 round 和两个外层实例重复调用按实际调用再次计数。此为显式的大项目常数，不是 BB90 比较常数，也不含安全协议离线材料成本。

## 7. 验证结果

### 7.1 全排列、稳定顺序及边界样本

最终 TEST_ONLY 验证脚本穷举了 `n=1…7` 的全部严格序排列，且每种排列测所有 `K`，合计 40,319 个 case；另测重复/全相等/负值/INT32 极值、奇偶和非二次幂规模，以及 `n=128,256,1000` 的全相等/重复/极值压力输入和 K 边界。总计 40,567 条。最终状态及 oracle 均为 `SUCCESS`；失败文件为空。修复前 200 个 `COMPLETED_WRONG` 仍保留在前述 external evidence。

### 7.2 冻结正式规模矩阵

输入 seed 与算法 seed 按 `BMW16-S4-2026-10-05-v1` 的 SHA-256 domain 分离规则冻结。固定输入组每个 `(n,K)` 重用一个输入，改变 1,000 个算法种子；随机输入组各自生成独立 signed-int32 输入和算法种子。全部由独立 validator 对冻结 S1 Python oracle 差分。每格单侧 95% Clopper–Pearson 下界为 0.9970087504549047；它是这 1,000 个固定种子试次的经验区间，不替代理想抽样概率证明。

| `(n,K)` | Appendix A `M` | 固定输入成功 | 随机输入成功 | 每组 CP 下界 |
|---|---:|---:|---:|---:|
| `(128,2)` | 254 | 1000/1000 | 1000/1000 | 0.99700875 |
| `(128,8)` | 242 | 1000/1000 | 1000/1000 | 0.99700875 |
| `(256,2)` | 510 | 1000/1000 | 1000/1000 | 0.99700875 |
| `(256,8)` | 498 | 1000/1000 | 1000/1000 | 0.99700875 |
| `(1000,80)` | 1842 | 1000/1000 | 1000/1000 | 0.99700875 |

合计 10,000/10,000 与 oracle 一致，观察失败 0。对这五个 M，精确 hypergeometric `δbracket` 依次为：`6.20e−35, 1.39e−34, 1.21e−24, 4.27e−25, 5.44e−20`（按上表配置顺序）；均小于 R3 窗口项。两份 Partition 的理想模型联合失败界 `<4.51e−7`。

每组实际 comparison calls（最小 / 中位 / 最大）：

| `(n,K)` | 固定输入组 | 随机输入组 |
|---|---:|---:|
| `(128,2)` | 38,667 / 42,941 / 47,197 | 38,720 / 42,874 / 48,092 |
| `(128,8)` | 35,840 / 39,753 / 43,943 | 35,380 / 39,869 / 43,727 |
| `(256,2)` | 75,872 / 88,443 / 100,085 | 77,449 / 88,567 / 99,981 |
| `(256,8)` | 76,222 / 87,291 / 98,051 | 76,256 / 87,302 / 100,162 |
| `(1000,80)` | 289,216 / 347,709 / 408,554 | 301,360 / 347,127 / 398,754 |

10,000 个试次累积的逐层实测调用为 R1 `428,380,000`、R2 `26,728,000`、R3 `757,124,530`、R4 `495,911`。R1/R2/R3/R4 的真实—真实调用分别为 `119,600,355 / 7,052,000 / 244,073,561 / 441,647`，dummy 相关调用分别为 `308,779,645 / 19,676,000 / 513,050,969 / 54,264`。跨双 Partition 的同轮重复请求分别为 `5,484,399 / 73,656 / 49,621,229 / 61,629`；R2 另有每份任务各一次的 pivot-pivot 重复，共 20,000 次。这些是 10,000 次重复运行合计，不作为单次 `O(n)` 证明；单次最坏界见 §6。

每例和每轮的真实—真实边、dummy 相关边、同轮重复、跨外层重复、跨轮重复均由程序计数；逐轮总和与独立审计结果见本报告 external matrix manifest。实际比较调用只验证量级和计数实现，不替代上面的 worst-case 推导。

### 7.3 C++ oracle 与独立边清单复算

冻结 Python oracle 与 `VFSS/include/moe_topk/topk_oracle.h` 在 13 个相同 raw signed Q20.12 测例上交叉核对，含全相等、同分、负分、INT32 边界、官方规模固定输入和 K 两端，selected index 与 mask 全部一致。C++ 文件只读。

完整小实例：scores `[9,1,8,2,7,3,6,4]`，`n=8,K=4`，input seed 101、algorithm seed 202；选出 `original_index=6`。独立边审计读取完整 R1–R4 edge list，校验边归属、因果轮号、结果哈希、pivot 是否由 R1 决定、U/q 是否由 R2 决定、V/W 是否由 R3 决定、以及算法/审计器计数一致：PASS。R1/R2/R3/R4 调用分别为 `90/36/2/2`，总计 `130`，唯一无序边 `62`、重复调用 `68`；各轮真实—真实/ dummy 相关调用分别 `56/34, 28/8, 2/0, 2/0`。完整原始边迹与 audit JSON 在 external evidence。

## 8. Protocol I 接入设计门

候选接口为：signed Q20.12 arithmetic shares → 稳定复合优先键 shares（建议高优先级先行的 tuple `(~sign_biased_score, original_index)`）→ 四层秘密比较 → 共享的第 K 阈值 key → 每条 shuffled 记录与阈值的 DCF 成员判断（key `≤` threshold）→ 用秘密逆路由回原始输入顺序的 XOR mask shares。

尚缺：

1. 隐藏样本/pivot/U/V/W、比较图地址和失败状态的无泄露方案；若公开将泄露自适应输入结构。
2. T 在未知输入与未来边图下提供输入无关、一次性、静默在线材料的方法；禁止在线 Dealer。
3. 输入相关比较材料的 oblivious addressing、容量、复用防护及联合 `T/P0/P1` transcript 证明。
4. signed score 与 original index 的秘密稳定 tuple 比较、share conversion、位宽、越界证明；shuffle 后 original index 必须随记录安全路由。
5. 在线产生阈值时，离线 DCF key 如何输入无关生成、阈值绑定每一条记录并一次性领取。
6. 秘密 shuffle/inverse route 与 DCF mask 的 payload 对齐及回原序证明。
7. 区分 plaintext comparison depth 4 与完整入口在线轮数；另测离线时间/material、在线通信、比较边、DCF/PRG 和 mask 路由成本。
8. 仓库现有“每次恰 K 且 oracle 正确”契约与 `≤4.51e−7` 理想模型 abort 概率的兼容方式。retry、精确 fallback 或放宽 API 合同都会改变轮数、材料和成本，不能默认为免费。

设计草稿可以交独立评审，但当前 gate 为 `NO-GO_FOR_SECURE_IMPLEMENTATION`。Protocol III 的 BB90+DCF 目标未替换；Protocol I 的候选标签更新不构成批准或实现。

## 9. 复跑命令、环境与证据

以下命令使用仓库 worktree 根目录与本机 bundled Python 3/NumPy；完整原始输出统一写到仓库外 `C:\Users\28641\.codex\evidence\BMW16_S4_2026-10-05`。

实测环境：Windows 11 `10.0.26200-SP0`、Python `3.12.14`、NumPy `2.3.5`、g++ MinGW-W64 `8.1.0`。官方矩阵用 8 个 worker；未测 secure 网络、在线轮数或 LAN/WAN。

```powershell
$py='C:\Users\28641\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $py experiments/TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R/select4r.py `
  --scores-json 'C:\Users\28641\.codex\evidence\BMW16_S4_2026-10-05\small_scores.json' `
  --k 4 --input-seed 101 --algorithm-seed 202 `
  --trace 'C:\Users\28641\.codex\evidence\BMW16_S4_2026-10-05\small_trace.json'
& $py experiments/TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R/audit_edges.py `
  'C:\Users\28641\.codex\evidence\BMW16_S4_2026-10-05\small_trace.json' `
  --out 'C:\Users\28641\.codex\evidence\BMW16_S4_2026-10-05\small_trace_audit.json'
& $py experiments/TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R/test_select4r.py `
  --out-dir 'C:\Users\28641\.codex\evidence\BMW16_S4_2026-10-05\small_and_adversarial_final'
& $py experiments/TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R/run_matrix.py `
  --out-dir 'C:\Users\28641\.codex\evidence\BMW16_S4_2026-10-05\official_matrix_final_source' --workers 8
& $py experiments/TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R/validate_results.py `
  --out-dir 'C:\Users\28641\.codex\evidence\BMW16_S4_2026-10-05\official_matrix_final_source'
& $py experiments/TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R/crosscheck_oracles.py `
  --out-dir 'C:\Users\28641\.codex\evidence\BMW16_S4_2026-10-05\oracle_crosscheck'
```

外部证据包含冻结 seed schedule、每次输入 hash/seed/status/逐轮计数、完整 small trace、独立 edge audit、所有失败记录（最终正式矩阵失败文件为空）、oracle validation、PDF 身份复核、S2 replay 和 C++ binary。源码、schedule 与结果 SHA-256 如下；原始矩阵和生成物留在仓库外。

官方矩阵的冻结 manifest 记录 Git revision 为 S3 起始提交 `59c75ff1ef21784bd54f266ab9bd6e631444ccae`，因为矩阵在 S4 本地提交前生成；manifest 中五个算法/验证源码哈希与下表及最终提交逐字节一致。矩阵后只改了文档，没有更改算法源码。

| Artifact | SHA-256 |
|---|---|
| final-source `select4r.py` | `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab` |
| final-source `run_matrix.py` | `35122d8c75e89d97f25e7ecf3036164e16f9f0a683d58d80d10ca186f9b4f3ab` |
| final-source `validate_results.py` | `16dcd96673d736b92b76b81fe444e13fbb5cee1a125f60c1b283dac732490ea1` |
| final-source `audit_edges.py` | `390f89c199c66e8899549a4710bd6c853e5709c0fde0f795b1ad1dc789ae346f` |
| final-source `test_select4r.py` | `576de4edadebd908959c56fc4fc9dce10c946d09dbdfa110effdf6aff6114586` |
| final-source `crosscheck_oracles.py` | `6a9c9e48baa5664a29a473fdf43e492d960d223f128ff59aba114406b5bd4c4b` |
| final-source `oracle_harness.cpp` | `30fda9ece72fee463a40300f41165fc4056879847d661482e7924266a171adc9` |
| final-source `README.md` | `17e308c7bdf9a394ae10c015a09d3a6f96ec4997223772b127dff0a11b25fbae` |
| frozen S1 Python stable-order oracle `oracle.py` | `9c3e3ee5fa61fbb21b934bb5c40b342d60e2ee68e803224d44cc7b8d0bc4f1be` |
| official frozen seed schedule | `3af287fcafcf82712004c941c3c176340b15b34bc9889ef08aa5b45435c42325` |
| official algorithm results JSONL | `288ba4bb5882def10f463c174e15893aed074741a3c08b448320c64c4d8c70c1` |
| official oracle validation JSONL | `b146981b2fcfed350f3054f1b4d1b52ac112b144bc00cd028f0dd7abc11a016b` |
| official frozen manifest | `3db6f1ea81234923bdf4413aa6174a30ff4adbab320f5197a379bb911d5189e4` |
| small/adversarial result JSONL | `cea6debee86575c9495724f43808c295d323d722bd748d539a1ad53275442319` |
| small/adversarial summary JSON | `15ce13c4beb87fdf2f39dfed46dd2aa4390c634ec40303c65c514c479633bd5f` |
| full example edge trace JSON | `c58bf7ee54c278e13ec165e5508db526da7388ed0d1f5ede9d0fff76cd3f07af` |
| independent edge audit JSON | `df7f36592ac898445deb30cb91346ff1cf05fe7c81a81ad676fa596f568a9f4d` |
| Python/C++ oracle cross-check JSON | `0e703334f46a509d9182f1ab1b3ab0e4ed58cc5a7d9da4f65c0d7c547f64e3ac` |
| compiled C++ oracle harness | `c2f33216c369e2d7df62209caef5e1a6b9da7a30a82d202e623d00cfb7f59290` |
