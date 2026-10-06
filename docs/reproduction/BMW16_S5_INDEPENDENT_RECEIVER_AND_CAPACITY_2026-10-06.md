# BMW16 S5：S4 独立接收、Protocol I 组合门与容量预检

日期：2026-10-06
接收对象：`BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R_TEST_ONLY`
S4 revision：`04ed6f8a352ab2277434344094471dff82f83e4b`
S5 review branch：`codex/m6b-i-bmw16-s5`
判定：**S4_ALGORITHM_ACCEPTANCE = PASS_WITH_EXPLICIT_LIMITS**；**Protocol I secure composition = NO-GO**。

## 1. 接收对象、基线和证据身份

| 项 | 独立核验值 |
|---|---|
| main / origin/main / S5 起点 | `c3926c68fd14f270faa8b55234311071947fa080` |
| S5 分支 | `codex/m6b-i-bmw16-s5`，隔离 worktree `C:\Users\28641\.codex\worktrees\m6b-i-bmw16-s5\MoE_Top-k_Reproduction_Evaluation` |
| S5 修改前接收凭证提交 | `c1d96252c1d9f2bfd79a031ae046181e1b691808`，新增 `docs/reproduction/BMW16_S5_PRECHANGE_RECEIPT_2026-10-06.md` |
| S4 branch / parent | `codex/bmw16-s4` / `59c75ff1ef21784bd54f266ab9bd6e631444ccae`；S4 工作树当时干净 |
| S4 状态 | S4 commit 不在 main 祖先链；S5 没 cherry-pick、修改或覆盖 S4 |
| M6A E17/E20/E21 | E17 `01f3c04...`、E20 `8986a401...`、E21 `93d96f6...` 均非 main 祖先；各自实验、标签和原始证据独立 |
| 禁止目录 | S5 差异未改 `VFSS/`、`VFSS-baseline/`、`Papers/`、三个本地参考工程、E20/E21 worktree 或桌面主工作区 |

主要论文 `Papers/1603.04941v1.pdf`：44 PDF 页，SHA-256 `F46F83CBA279F37E9E3AAA0D64B145FDFCFBC52C0C8B79BACEC1259CA9CA23`；辅助交叉核对 `Papers/017.pdf`：249 页，SHA-256 `3EAE3A93F12AB71EEA8C040167F21351FCF3FF54DC423985A04087E3E90CB15`。前者是主要算法来源；后者只用于交叉核查，不能作为独立证明。两 PDF 在桌面只读，均未进入 Git。S4 的来源页码表保留于其决策文件；与 S5 接收相关的定位为：noiseless comparison model PDF p.7；Algorithm 5 PDF p.29 与相邻证明 p.28–29；Theorem 8 PDF pp.11、33–34；Algorithm 7 PDF p.34；Appendix A Lemma 1 PDF p.16。S4 另将 Algorithm 7 的 epsilon 闭区间与 Theorem 8 的严格开区间差异列为原文事实。S4 的 sample-bracket 算法**不使用 epsilon，也不继承 Theorem 8 的概率定理**。

S4 源码 manifest 的 `revision` 记为 S3 base（S4 文件在完成实验后提交）。S5 对官方 run 使用的 `official_matrix_final_source/frozen_manifest.json`，逐文件比对后确认它的五个源码哈希精确对应 S4 receiver commit：

| 源文件 | SHA-256 |
|---|---|
| `select4r.py` | `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab` |
| `run_matrix.py` | `35122d8c75e89d97f25e7ecf3036164e16f9f0a683d58d80d10ca186f9b4f3ab` |
| `validate_results.py` | `16dcd96673d736b92b76b81fe444e13fbb5cee1a125f60c1b283dac732490ea1` |
| `audit_edges.py` | `390f89c199c66e8899549a4710bd6c853e5709c0fde0f795b1ad1dc789ae346f` |
| `test_select4r.py` | `576de4edadebd908959c56fc4fc9dce10c946d09dbdfa110effdf6aff6114586` |

外部证据还有相邻目录 `official_matrix_final_v2`，其 manifest 绑定不同 `select4r.py` SHA `664e480e...` 和不同 results SHA `f982bea4...`。S5 不把两个结果集合混合；下面 10,000 行接收明确对应源码哈希 `8644...` 的 `official_matrix_final_source`。

## 2. 算法逐项复核

S4 的算法身份是**PROJECT_DERIVED sample-bracket Select**。它使用 BMW16 的 noiseless comparisons、严格全序、任意 K 两哨兵归约以及四轮/线性比较的研究目标；其 R1–R4 采样参数和 finite abort cutoff 由 S4 项目推导决定。不得将其写为论文 Algorithm 7、BMW16 作者精确复现或 BB90 复现。

对偶数输入 `M` 与目标 median reject count `h=M/2`：

1. R1 对大小 `s=min(M,ceil(8√M))` 的无放回样本做所有无序对比较。比较结果为样本构造一基全序 rank，按固定的 `qL/qH` 取 `x<y`。R2 的 pivot 由已经完成的 R1 产生；R1 不读取后轮信息。
2. R2 对每项与 `x/y` 比较，得到 `P={z:z<x}`、`U=[x,y]`、`Q={z:y<z}`。`c=|P|`、`q=h-c`。只有 `c≤h≤# {z:z<y}` 且 `|U|≤Ucap` 才进入 R3。此 bracket 使 `0≤q<|U|`。`q=0` 可直接按 `[x,y]` 把 cut 右侧全接受，R3/R4 留空；不添加比较。
3. R3 取 `v=min(u,ceil(8u/√M))` 的 `V⊆U`。冻结图为 `V` 内部无序对和 `V×(U\V)`；V 内部只请求一次。其结果可给每个 V 元素相对 U 的确切 rank。取最近的 `i≤q`、`j>q` sample rank；缺边界 sample 时分别用虚拟 rank `0` 或 `u+1`。真实端点 `x/y` 自身属于 W；W 为端点间包含边界的连续区间。`qW=q−(i−1)`（若 i 是虚拟 0 则 `qW=q`）。若 `W` 大于 `2ceil(2√M)+1`，明确 abort，不做精确 fallback。
4. R4 比较 W 全部无序对，精确排名后拒绝 W 中前 `qW` 项。真实/填充数组共拒绝 `c+(i−1)+qW=h` 项；虚拟边界不计为输入记录。另一侧对称保留，因此 R4 完成精确 median partition。S5 trace auditor 独立重建了每轮边表、outcome 哈希、R1 pivots、R2 interval/q、R3 rank/W/qW，并确认每个 edge batch 在其结果 barrier 之前已冻结。

任意 `K` 使用 S4/Appx A 的两哨兵归约：令升序 real rank `r=n−K+1`、`N=n+1`、`Lpad=max(N−2r,0)`、`Hpad=max(2r−N,0)`；两个 median 实例各含 n 个 real、一枚低/高 sentinel 和相同数量的 rank padding，`M=2max(r,K)≤2n`。低 sentinel 实例目标 rank 为 `h+1`，高 sentinel 实例目标 rank 为 `h`；在两个 median instances 都成功的事件上，唯一 real 输出是 `Accept_low ∩ Reject_high`。`K=1/K=n`、奇偶中心和同分都由 S4 的代数和 strict key 处理。两个实例同钟 R1/R2/R3/R4 并行；它们的理想 random tapes 必须条件独立。

Project 明文全序从低到高是 `(signed_score, −original_index)`，等价于项目 priority `(score DESC, original_index ASC)` 的反向序；内部类别次序给 low rank padding、low sentinel、real、high sentinel、high rank padding 分配不同类别，故 `INT32_MIN/MAX` 不与哨兵碰撞。original index 留在复合键，不得用洗牌 slot 取代。本结论仅适用于 TEST_ONLY 明文编码。秘密份额 tagged key 的位宽、类型 tag 与 DCF 比较是另一个未证明接口。

S4 对随机分支采用显式失败合同：pivot bracket 失败、U cap 越界、W cap 越界会输出 `PROJECT_RANDOM_FAILURE_PATH` 且没有 candidate；违反结构性 rank/cardinality 不变量输出 `COMPLETED_WRONG`。不存在通过 oracle、排序、重试或追加轮次的恢复路径。

## 3. 概率、成本和有限参数

理想随机模型中，R1 bracket 失败为 `δbracket=Pr[H<qL or H≥qH]`，`H~Hypergeom(M,M/2,s)`；S5 用 Python 大整数组合数重新求精确有理数，再以高精度 Decimal 输出。R3 若有一侧至少 L 个 rank，V 未命中该侧固定 L 长窗口的概率不超过 `exp(−vL/u)≤exp(−16)`；两侧 union bound 为 `2exp(−16)`。当 `M≤4096`，`Ucap=M` 无 U cap abort；当 `M>4096`，S5 复核 `qH−qL≤24M^1/4`、`μ≥64M^1/4`，并对不超过 M 个区间位置 union bound，得 `δcap≤Mexp(−12M^1/4)`。算法每次成功输出都是精确 median partition；概率损失来自 bracket/cap 触发 abort，不来自以错 rank 标为成功。

| M（n,K 配置顺序 n128/K2、n128/K8、n256/K2、n256/K8、n1000/K80） | s | qL/qH | 精确 R1 bracket fail | Ucap | `2e^-16` window 项 | 两实例 Select ideal abort 上界 |
|---:|---:|---:|---:|---:|---:|---:|
| 254 | 128 | 18 / 111 | `6.19946424e-35` | 254 | `2.25070349e-7` | `<4.50140648e-7` |
| 242 | 125 | 18 / 108 | `1.39298770e-34` | 242 | `2.25070349e-7` | `<4.50140648e-7` |
| 510 | 181 | 37 / 145 | `1.21384453e-24` | 510 | `2.25070349e-7` | `<4.50140648e-7` |
| 498 | 179 | 36 / 144 | `4.27213223e-25` | 498 | `2.25070349e-7` | `<4.50140648e-7` |
| 1842 | 344 | 97 / 248 | `5.44075270e-20` | 1842 | `2.25070349e-7` | `<4.50140648e-7` |

这不是 BMW16 Theorem 8 的误差率，也不包括密码学 PRG/shuffle 统计距离。Python `random.Random` 与 SHA-256 派生 seed 是可重放 TEST_ONLY tape，不是密码学随机源。

逐层线性界（一个 median instance）：R1 `≤42M`，R2 `≤2M`，R3 对 `M≤4096` 用 `C(M,2)≤2048M`，对更大 M 的 U cap `U≤9M^3/4` 和 V 规则给 `vU≤657M`，R4 W cap 给 `C(|W|,2)≤25M`。两实例同轮求和 R1≤84M、R2≤4M、R3≤4096M、R4≤50M，总计 `≤4234M≤8468n`。该是 S4 项目常数，不是 BB90 的常数。

在正式 10,000 行文件上，S5 独立重新生成 signed-int32 输入并按 `(-score,index)` 排序：10,000/10,000 `SUCCESS` 一致；每个 family/config 1,000 条、零失败，单组一侧 95% Clopper–Pearson success 下界 `0.9970087504549047`，不能替代小于 `10^-6` 的概率证明。按记录相加总比较调用 `1,212,728,441`，最高实测 `408.554n`；单条上界检查仍为 `8468n`。这不是运行时间/性能结论。

S5 还从只读 S4 commit `04ed6f8a352ab2277434344094471dff82f83e4b` 新运行了同一 10,000 行矩阵。重跑行数、种子 schedule、状态、答案、逐轮/总比较计数均与权威归档逐行相同；S5 独立审计逐行比对所有 JSON 字段，仅移除嵌入行的 `frozen_manifest_sha256`。重跑的 results 文件 SHA 与旧归档不同，是因为旧归档的 manifest revision 早于 S4 commit，而本次重跑 manifest 绑定精确 S4 revision；两份文件的 provenance 字段不同，其余字段完全相同。新重跑仍是 10,000/10,000 SUCCESS、0 failures，不把它作为额外独立随机样本计入成功率。

40,567 条小规模/对抗记录经独立生成所有 `n≤7` 全序排列、同分和 INT32 边界输入核验，`SUCCESS=40,567`、失败文件 0 byte；总比较调用 `15,009,334`，最高 `405.214n`。独立实跑仓库 C++ `topk_oracle.h` 对 13 个边界/同分/正式固定输入用例：C++ 选中 index 和完整 Top-K mask 都逐项与独立 sort 结果一致，mask 恰 K。此处的测试只证明这些冻结样本。

修复前 raw archive 保留 40,543 行，其中 40,343 `SUCCESS`、200 `COMPLETED_WRONG`；S5 对这 200 行确认全部没有 candidate 输出（`failed_rows_emitting_candidate=0`）。代表样本：`n=65,K=8,algorithm_seed=8320890985398024223`，输出空 candidate，oracle index 23，状态为 R4 coverage/cardinality invariant failure。该档案完整 SHA 为 `3682b81c63d5e09ab124df1c9a2176853c7dd9910d7cd3daf8cc2d3fb8fb7835`，保留失败，不计入修复后 success rate。

接收时也记录了 S4 small-run harness 的退出码局限：它对 `COMPLETED_WRONG` 返回非零，但对 `PROJECT_RANDOM_FAILURE_PATH` 不保证非零。因此 S5 不以退出码单独作为通过依据；独立 verifier 逐行重新分类状态、校验失败文件长度/哈希、输出与逐轮调用，并确认修复后正式两份 run 均无失败行。

## 4. 逐轮边审计小实例

冻结 trace：`n=8,K=4,input_seed=101,algorithm_seed=202`；稳定选择 index 6。S5 不读取 S4 edge-auditor 结果生成自己的结论，而从 trace item keys 重算 outcome，再独立重建边次序和摘要。

| 轮 | comparison calls | real-real | dummy-related | same-round repeats | same-task repeats | cross-task repeats | previous-round repeated calls |
|---:|---:|---:|---:|---:|---:|---:|---:|
| R1 | 90 | 56 | 34 | 28 | 0 | 28 | 0 |
| R2 | 36 | 28 | 8 | 10 | 2 | 8 | 36 |
| R3 | 2 | 2 | 0 | 0 | 0 | 0 | 2 |
| R4 | 2 | 2 | 0 | 0 | 0 | 0 | 2 |
| 合计 | **130** | 88 | 42 | **38** | 2 | 36 | 40 |

全轮 unique unordered edges 为 62，总重复调用 `130−62=68`（包括同轮与跨轮）；每轮重算的 canonical edge digest 与原 trace 相同，且结果摘要分别是 `d20c9003...`、`3dc94637...`、`47dc540c...`、`96a296d2...`。R2 plan 依赖 R1；R3 plan 依赖 R2；R4 plan 依赖 R3。S5 auditor 重建四轮全部 130 条边并验证结果，不允许同轮结果反向改变同轮边图。计数口径注意：same-task 与 cross-task 两列恰好划分同轮重复（合计 38）；previous-round repeated calls（40）是另一条诊断统计，与同轮重复分类可能重叠，不与 38 相加。全局的 68 是按实际调用减去全轮 unique unordered edges 得到的总重复数。

## 5. Protocol I 安全组合门与具体阻塞

主线 Protocol I label 仍为 `m2_protocol_i_raw_score_input_modular_8round_mask_output`。当前 M2 文档记录 Protocol I secure candidate 的前向 share shuffle 两轮、all-pairs CmpAgg 一轮、public rank reveal 一轮、reverse routing 两轮，再加 raw score input lift/sign 两阶段，实际 C-level raw-score full-entry label 为 8 rounds；这不等同于论文 exact 功能。现有 `protocol_i_secret_shared_shuffle` 返回 `ProtocolIBlock192` shares/counters，不返回与其同置换的 public masked list。精确公开输出 `pi(x)+r` 和材料同置换绑定为 M2.16 明示 blocker。S5 没把 4 个 Select comparison layers 加到 8 得出候选 secure rounds；它们可能需要更多因果交换，当前 `NOT_RUN`。

| 安全/工程门 | 结论 | 证据或缺口 |
|---|---|---|
| 单方完整 joint simulator（包括 shares、局部 permutation、masked list、图、DCF key shares、文件 access、abort、output shares） | `UNPROVEN` | 只有 public transcript 的均匀 rank-to-handle 边际直觉；没有 joint simulator。两 Select 实例还必须保留真实 handle 跨实例关联 |
| 固定 sample slots 及 R1/R2 offline address | `UNPROVEN` | 可令隐藏 uniform permutation 后固定样本位置以覆盖 uniform sample，但当前 shuffle public-list contract 和关联证明缺失；所以同时报告保守全 pair 池 |
| 多阶段、多 key、一次性读取 | `UNPROVEN` | E20 `E20_STREAM_AEAD_V1`、E21 `E21_CLIQUE_MINIMAL_SEALED_V1` 属于非 main 分支；它们逐 key/store 功能证据不能证明本协议多阶段自适应读取的 joint simulation |
| stable tuple/dummy comparison width | `UNPROVEN` | 主线 score adapter 能形成 real priority key candidate；5 个 item class 额外 tag 和 secure comparator range 未实现。uCMP `Bin=34..53`；tagged 1e6 候选 Bin56 |
| selected key → DCF membership | `UNPROVEN` | 最终 star 边池可避免为在线 unknown threshold 单独 keygen，但 selected-handle address、key identity、阶段 domain 与 DCF shares 未绑定 |
| inverse route 与输出合同 | `UNPROVEN` | 当前 reverse PS 组件不是两 Select shared identity 与 BMW16 selected threshold 的组合证明；XOR conversion/abort zero-mask 未接入 |
| full entry online rounds/communication/PRG/keygen | `NOT_RUN / NOT_MEASURED` | 不得引用 S4 明文耗时、AAV86 E17/E20/E21 数值或现有 8-round 路径数据为候选估计 |

E17 的 `PASS_WITH_EXPLICIT_LIMITS` 只接收 bounded AAV86/all-pairs 数据；E20 覆盖 streamed n1000 AAV86 package 与独立对照，明示 multi-key joint simulation 未证明；E21 的 clique/material matrix 仍明示 `AUTHOR_EXACT=NOT_PROVEN`。三者 revision 均不在 main ancestor chain，本报告只引用其边界，不 cherry-pick 代码或搬运计量。

### 后续接入前必须交付的证据

1. 纸面定义同一个 hidden permutation 的 ideal functionality：real record、原始 index、S4 的两实例 dummy/sentinel views、masked list `pi(x)+r`、DCF gate 参数和 inverse carrier 必须共享同一可验证 binding。
2. P0/P1 各自的完整 joint simulator：输入 shares、各自局部 permutation、T 包、每个可读/未读材料 ID、公开边与输出/abort；证明 public graph、rank handles、dummy 暴露和 abort 在允许泄露内。
3. 可信离线 T 的一键一边一阶段 domain、per-edge masks/key material、覆盖完整的 fixed/precomputed/oblivious edge addressing、一次性领取与重放/坏包合同；不得让 T 知道 online input/未来 pivot，不得在线补料。
4. 32-bit signed score + original-index stable key + 五类 tagged dummy 的 exact range proof 与 DCF/uCMP conformance；解释 `n=10^6` 宽度超过当前 53 位接口如何处理。
5. 两哨兵交集不打开 selected original index 的共享表示，selected threshold 与每个 real record DCF membership pair pool 一次绑定，最终 inverse route 对应原输入顺序。
6. 完整因果消息 DAG、双方 abort agreement/no-mask 释放规则、PRG/shuffle 统计误差预算，使目标 `Pr[ABORT]≤10^-6` 可计算，并在 `Pr[wrong|SUCCESS]=0` 的理想功能性条件下闭合。
7. checked capacity + 一次性材料/每方字节/Dealer 峰值/磁盘预检通过之后，才开始 VFSS 小规模新入口；按 conformance → frozen oracle differential → T/P0/P1 独立进程 E2E 顺序验收。

## 6. 独立审计器、命令和原始结果

独立审计源码：`experiments/TEST_ONLY_BMW16_S5_RECEIVER_REVIEW/audit_s4_independent.py`。它不导入 S4/S1 Python 算法或 oracle；从冻结 seed schedule 重建 inputs，以独立 `sorted((-score,index))` 计算 kth 和 Top-K mask；重算仓库 C++ oracle 的 13 个 cases；重建小 trace R1–R4 edge lists、每条 noiseless 比较结果、边顺序/hash、R1–R3 state 与 barriers；用高精度 Decimal/整数组合数重新算有限概率 bound；用 uint64 checked add/multiply 预检离线池，不分配材料。

复跑命令（在 S5 worktree 根目录，Python 3）：

```powershell
python -m py_compile experiments/TEST_ONLY_BMW16_S5_RECEIVER_REVIEW/audit_s4_independent.py
python experiments/TEST_ONLY_BMW16_S5_RECEIVER_REVIEW/audit_s4_independent.py `
  --evidence-root C:\Users\28641\.codex\evidence\BMW16_S4_2026-10-05 `
  --s4-source-root C:\Users\28641\.codex\worktrees\bmw16-s4\MoE_Top-k_Reproduction_Evaluation\experiments\TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R `
  --replay-root C:\Users\28641\.codex\evidence\BMW16_S5_2026-10-06\replay_official `
  --out C:\Users\28641\.codex\evidence\BMW16_S5_2026-10-06\independent_s4_audit.json
```

S5 从 S4 提交重跑的命令（分别只写入仓库外的新证据目录）：

```powershell
python C:\Users\28641\.codex\worktrees\bmw16-s4\MoE_Top-k_Reproduction_Evaluation\experiments\TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R\test_select4r.py `
  --out-dir C:\Users\28641\.codex\evidence\BMW16_S5_2026-10-06\replay_small
python C:\Users\28641\.codex\worktrees\bmw16-s4\MoE_Top-k_Reproduction_Evaluation\experiments\TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R\run_matrix.py `
  --out-dir C:\Users\28641\.codex\evidence\BMW16_S5_2026-10-06\replay_official --workers 8 `
  --root C:\Users\28641\.codex\worktrees\bmw16-s4\MoE_Top-k_Reproduction_Evaluation
```

当前环境：Windows 11 x64；本机 Python `3.13.7`。审计结果放在仓库外，不提交。S4 原始证据目录为 `C:\Users\28641\.codex\evidence\BMW16_S4_2026-10-05`；S5 审计 JSON 为 `C:\Users\28641\.codex\evidence\BMW16_S5_2026-10-06\independent_s4_audit.json`。

| 审计证据 | SHA-256 |
|---|---|
| S5 独立审计结果 JSON | `9a17b94874d9c420ace9f8e289cb44aa0802551d099e13e7b4ffd1e300e130cb`；结果文件不入 Git |
| S5 独立审计器源码 | `5c63739256d02b4e720a47662e9e73bb23736b8b868546ddd8ca29d2114d8ce6` |
| S4 official seed schedule（10,000） | `3af287fcafcf82712004c941c3c176340b15b34bc9889ef08aa5b45435c42325` |
| S4 authoritative official results | `288ba4bb5882def10f463c174e15893aed074741a3c08b448320c64c4d8c70c1` |
| S4 authoritative frozen manifest | `3db6f1ea81234923bdf4413aa6174a30ff4adbab320f5197a379bb911d5189e4` |
| S4 final 40,567 case JSONL | `cea6debee86575c9495724f43808c295d323d722bd748d539a1ad53275442319` |
| S4 repair前 200 failure records 的完整集合 | `3682b81c63d5e09ab124df1c9a2176853c7dd9910d7cd3daf8cc2d3fb8fb7835` |
| S4 n8 trace | `c58bf7ee54c278e13ec165e5508db526da7388ed0d1f5ede9d0fff76cd3f07af` |
| Python/C++ oracle crosscheck JSON / exe | `0e703334f46a509d9182f1ab1b3ab0e4ed58cc5a7d9da4f65c0d7c547f64e3ac` / `c2f33216c369e2d7df62209caef5e1a6b9da7a30a82d202e623d00cfb7f59290` |
| S5 fresh official replay results / schedule / manifest | `1bed58b4e5f9acc91d89c7c799b5c3c24b74950d4050b66f27ce8d4eb21d6c14` / `3af287fcafcf82712004c941c3c176340b15b34bc9889ef08aa5b45435c42325` / `911841a686eeafa6df407eea2d73159692113ce1827dd0e9d285459c020ab87d` |
| S5 fresh official replay failure JSONL | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` (0 bytes) |
| S5 fresh small replay results / failures / summary | `cea6debee86575c9495724f43808c295d323d722bd748d539a1ad53275442319` / `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` / `15ce13c4beb87fdf2f39dfed46dd2aa4390c634ec40303c65c514c479633bd5f` |

原始 S4/S5 JSONL、binary、PDF、key 和构建物都在仓库外；本阶段未跑网络或正式 LAN/WAN benchmark。S5 性能指标全部 `NOT_MEASURED`。
