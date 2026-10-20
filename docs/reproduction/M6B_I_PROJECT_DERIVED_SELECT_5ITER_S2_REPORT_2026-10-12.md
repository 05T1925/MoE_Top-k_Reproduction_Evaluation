# M6B-I S2 工作报告：项目派生五迭代 Select 独立接收、比较成本与 Protocol I 设计门

日期：2026-10-12（Asia/Shanghai）  
任务：`M6B-I-PROJECT-SELECT-5ITER-S2`  
算法：`PROJECT_DERIVED_SELECT_5ITER`，plaintext label `PROJECT_DERIVED_SELECT_5ITER_TEST_ONLY` v1  
结果：明文功能及独立 trace 接收 PASS；比较成本上界闭合；Protocol I 设计门 `NO_GO_FOR_INTEGRATION`。没有实现 secure/experimental runtime，没有创建 alias，也没有运行正式 LAN/WAN。

## 1. 工作区、来源身份与保护范围

| 项目 | S2 核验 |
|---|---|
| 工作树 | `C:\Users\28641\.codex\worktrees\m6b-bb90-source-closure-s1\MoE_Top-k_Reproduction_Evaluation` |
| 分支 / HEAD | `codex/m6b-project-derived-5iter` / `c3926c68fd14f270faa8b55234311071947fa080` |
| base/local main/origin/main/live origin main | 全为 `c3926c68fd14f270faa8b55234311071947fa080`；`git ls-remote origin refs/heads/main` 返回相同 revision |
| S1 源码 revision | S1 没有提交；从本工作树未提交文件以精确文件 hash 接收，见决策文档 §2 |
| S31/S32 refs | S31 `a1a90cb2a568deb5e5078ddec10136ce9a743411`；S32 `32cd7952cccdacc5a1a5b4fcca245f7acf594049`。这是 BMW16-derived 路线的分支材料，不是本项目算法的接收证明 |

此 worktree 在 S2 开始前已承载 S1 的文档、算法代码及 `docs/PAPERS.sha256` 修改。S2 保留这些内容；未改 S1 报告、S1 决策、S1 测试、`docs/PAPERS.sha256`、`VFSS/`、`VFSS-baseline/`、论文或本地参考目录。桌面 primary checkout 中的用户差异也未被本任务写入。所有 S2 新代码和报告都在当前隔离 worktree。

接收源码 hashes：`select5iter.py` `8b8d3dbd…a204606b`；`audit_edges.py` `b84591c4…bf2fc0`；`frozen_oracle.cpp` `a30fe76a…e48a457`；`test_select5iter.py` `fe933e6d…79c4cf56`；冻结 `topk_oracle.h` `270ed8e3…0a12e22`。完整 hash 在 [S2 决策](../decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S2_DECISION_2026-10-12.md)。源标签/身份仍是 PROJECT_DERIVED；不称为 BB90 或 BMW16 实现。

S34 schema-v3 技术诊断仍不是异会话接收；M6A V3 仍 OPEN，正式 V4 仍 NOT_ALLOWED/NOT_RUN；BMW16-derived 条件安全路线仍独立；BB90 S1 来源仍 NO-GO。上述状态没有合并。

## 2. S1 明文算法的独立接收

S2 增加 `experiments/PROJECT_DERIVED_SELECT_5ITER_S2/independent_accept.py`。该 verifier 不调用 S1 auditor；它使用 S1 trace 和边 outcome，自行验证样本无重复且属于 C、样本 clique 的全序、qL/qH、pivot identity、所有 edge endpoints、pivot rank、候选/RANK 更新、第五层 clique、跨轮边重复、最终 stable mask，再逐例对照编译后的冻结 C++ oracle。

| 复核范围 | 结果 |
|---|---:|
| 全部 `n≤7` 严格顺序排列与所有 K | 40,319 例通过 |
| S1 对抗、随机、边界和稳定同分扩展 | 包含在 40,490 个有效案例内，全部通过 |
| 有效案例总数 | 40,490/40,490 |
| 五层均有非空 edge 的案例 | 60 |
| 固定输入改变算法 seed | 32 个 seed 产生 32 个不同首轮 sample |
| 非法输入拒绝 | 5/5 |
| 最终失败记录 | 0；空 `independent_failures.jsonl` SHA-256 `e3b0c442…b855` |

这确认 S1 对任意合法采样带的结果正确，重复分数由 index 稳定打破，边集足以给 pivots 精确 rank。它不是秘密共享正确性证据。

独立审计还更正了一项 round accounting：S1 明文程序直接从 scores 排 sample，随后才记录 clique outcomes。明文功能没有错，比较数量也把 clique 计入；但协议算法必须先执行 sample clique 才能知道 pivots，再执行 pivot stars。实际比较依赖是四组 `(sample clique, pivot stars)` 加第五层 candidate clique，共 9 层。S1 所称“五轮”是算法迭代，不是五个并行比较层，也不是五轮协议。

验证迭代曾先后出现独立 verifier 实现错误：run1 误将三字段 outcome 解成二字段；run2 未处理自比较恒为 false；run3 对第五轮单例 clique 的零边 trace 错误要求非空。三个 run 的原始失败文件和 summary 都留在外置 evidence，未覆盖。run4 在修正后初步通过；随后 verifier 又增加冻结边数组、边摘要及 edge outcome 对 comparator 的逐条核对，最终 run7 为 40,490 PASS/0 failures。这些是 checker 开发失败，不是被隐藏的算法失败。

| 尝试 | verifier 错误 | failure JSONL SHA-256 | summary SHA-256 |
|---|---|---|---|
| run1 | outcome row 解包错误，5 个 singleton/invalid 案例外均被 checker 拒绝 | `616654432ae63538f3a71566bff9c4e6d1dee4705daf99e3c08f0648c871cec7` | `216642ae8006ded44225916d90b7289176c0911604cc0317058db31a76654c1a` |
| run2 | rank 更新对 `x==x` 查缺省边 | `41e0bdd68a7ea02e22a915fd1c72e2eb66383970d1ff6804b552c7e686c56f98` | `ebdf9a0de655d56a0e6c7f1ce16e2386886fad72c17e34688155653e41f0919a` |
| run3 | 将合法的最终单例零边 clique 误报为缺层 | `66a7d6199f8dfdff5c884ebb7132fec5dd0087b5b1eabafef36bee53a1b9a975` | `e6ead2b3e1119d7d903932fb632db3343670b071899e4460997e5179ee60789a` |
| run4 | 初步通过，尚未增加最终 edge digest/outcome 核对 | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` | `b9718b5e358d6ec81618aab7f4408cb99ee1d36b7b6040d82d8283af3e3dc8a1` |
| run7 | 最终独立重跑，含冻结边数组/digest 和 comparator outcome 核对 | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` | `d75f5d0e09e04f953080e9effa08c26c33a1eedfc8784e86f4b8530181defc76` |

## 3. 目标规模比较成本

新建精确成本程序 `comparison_cost_bounds.py`：用状态 `(stage,m,r)` 和样本子集转移计算精确期望；并用不带 memoization 的路径枚举复核 `n=2..6` 的所有 K，共 20 个点逐个给出相同的有理数。有限矩阵没有能力在可行时间内枚举所有大状态，所以没有伪称矩阵精确均值；而是用次序统计量间距二阶矩、边界 spacing 和单侧 hypergeometric Hoeffding 尾界形成逐点可复算的严格有限期望上界。目标矩阵和最坏界见决策文档 §4；cost JSON 的 SHA-256 为 `d133364f…20e3807`。

主要数值（逻辑 comparison calls，上界而非实测）：

| n,K | R1–R4 样本 clique edge | R1–R4 pivot star edge 上界 | 第五候选/全图上界 | 总期望比较数上界 | 最坏总量上界 |
|---|---|---|---:|---:|---:|
| 128,2/8 | 66/轮 | 232,228,224,220 | 120 / 7,140 | 5,152 | 8,308 |
| 256,2/8 | 120/轮 | 480,476,472,468 | 248 / 30,628 | 23,042 | 33,004 |
| 1,000,80 | 496/轮 | 1,936,1,932,1,928,1,924 | 992 / 491,536 | 390,499 | 501,240 |
| 10,000,80 | 4,950/轮 | 19,800,19,796,19,792,19,788 | 9,992 / 49,915,036 | 46,229,117 | 50,014,012 |
| 100,000,80 | 50,086/轮 | 199,366,199,362,199,358,199,354 | 99,992 / 4,999,150,036 | 586,733,340 | 5,000,147,820 |
| 1,000,000,80 | 499,500/轮 | 1,998,000,1,997,996,1,997,992,1,997,988 | 999,992 / 499,991,500,036 | 3,843,728,886 | 500,001,490,012 |

有限上界较松，尤其 R5；故我们只主张最坏 `O(n²)` 和理想 sampler 下的期望 `O(n)` 逻辑比较数，不给高概率工作量界，也不声称任何大规模实际加速。随机模拟没有进入定理或表值。所有随机 tape 都输出正确；实际随机源偏离理想 uniform-without-replacement 模型时，期望成本证明不自动适用。

按比较调用量纲区分：每轮 `e(m)` 里的 sample clique 与 stars 是不同无序边；跨轮同一 endpoint pair 再次出现则重计一次逻辑 call/边执行，并要 fresh one-shot material slot；唯一无序 identity pair 另计；通过端点规范只发单向 `<` 调用，反向由 complement 得出。没有协议运行，所以有向 DCF Eval、key slot、实际 DCF Eval、AES expansion、材料字节和在线字节全部 NOT_MEASURED；决策中的 DCF 数仅是按现有 uCMP 源码 ABI 的条件算式。

## 4. Protocol I 设计复核及 NO-GO 的准确原因

阅读了主线 raw-score、shuffle、uCMP、package 和 pipeline 源码，以及 S31/S32 分支决策/计量文档。

**现有接口事实**：当前 `ProtocolIPartyPackage` 固定 `C(N,2)` 个按端点字典序排列的材料槽，没有 iteration/substage 字段；包限制 1,000,000 edges 和 64 MiB。当前 pipeline 做 2 轮 forward shuffle，打开 masked priority 列表，运行固定全对全 CmpAgg，再打开 shuffled rank，最后 2 轮 reverse shuffle；工程总 core 6 轮，接入当前两轮 raw Q20.12 adapter 后是现有 raw-score→mask 8 轮。现有 `ProtocolIUcmpPartyMaterial` one-shot，strict `<` 每方运行两个 DCF Eval；序列化 party key 为 `D(b)=57+24b` bytes。没有自适应 comparator、active-candidate scheduler 或共享 threshold membership API。

**数据流草图**：原始 Q20.12 share → 34-bit carry/sign 转换成带 original_index 的唯一优先级 key share → 隐藏 shuffle → 4 次 `(active-C sample → sample clique → qL/qH oblivious pick → pivot stars → secret rank/C/r update)` → 第五候选 clique → 得到共享 K-th key τ → 对每个 padded key 计算 `1−[τ<key_i]` → 同一个隐藏逆排列 → 原序 XOR mask shares。唯一 key 保证 membership 恰 K 位；此处只是函数组合推导，未执行安全协议。

NO-GO 不是笼统缺“安全证明”：

1. 不能对秘密 active C 直接抽取精确均匀 sample；也没有 sample order statistic、pivot rank、residual rank、秘密候选 compact/update 的接口。公开这些状态需要扩展 L，保持隐藏则需要 oblivious control。
2. 自适应 slot 地址由前轮比较决定。已有固定一 pair 一 key 包不包含 round ID，且不能安全重用 one-shot key；如直接公开端点和读槽顺序，图随输入次序变化。最小 prototype 保持 n/K/算法 seed 与首轮 sample identity 相同，但改用固定种子打乱的测试分数顺序，五轮 edge set 对称差为 `[100,105,38,31,45]`。这是接口可见字段见证，不是匿名 shuffle 后的密码学攻击。
3. 按 round 分配每个可能 pair 的 fresh key，在 padded 目标矩阵的 Select 池需要 `5C(N,2)` slots；以现有 M2UC key size 计算，128 点约 42,998,400 bytes/party（即使容量可容纳也不兼容旧 ABI），256 点约 175,853,760 bytes/party，1,000 点已需 2,618,880 slots、2.94 GB/party；1M 点下界约 3.74 PB/party。没有 ORAM/oblivious fetch 或压缩 key service。
4. 阈值 τ 要保持秘密。虽可把 stable key 唯一性下的 `key_i≤τ` 写成 `1−[τ<key_i]`，当前只确认源级 strict `<` ABI，没有新路线的阈值 share encoding、方向/boundary conformance 和 simulator；不能借其它路线的成员选择测试。
5. S31 的 L 允许某些匿名 handles/endpoints/slot schedule 字段，但其证明游戏覆盖另一个算法的图、材料池、shuffle 与 output。S31 还列明 `p_x,p_y,r'`/active candidate size 是否公开未在本路线确认。S32 是计量决策，不会自动批准 L。不得转移 S31 whole-pool/adaptive-query/shuffle simulator 或概率界。
6. 没有新路线的成功/失败消息 DAG、双方 ABORT/no-mask barrier 或完整 party-view simulator；单方返回 output share 前如何确认双方成功也未设计。

**条件计量**：`N=next_power_of_two(max(2,n))`、`w=33+log2 N`。若每次调用使用同 ABI uCMP，Select 实际 trace E 次，则每方新 one-shot slots 为 `2N` 个 34-bit raw adapter + E 个 w-bit Select + N 个 w-bit membership；每方 DCF Eval `2(E+3N)`，两方总 Eval `4(E+3N)`。隐藏调用时 input-independent Select pool 最坏为 `5C(N,2)`。这些数字是材料设计算式，不是已生成 key/Eval。

在线方面只可复用当前已知 adapter payload：raw carry/sign 每方每阶段 `16N` bytes，两阶段送出 `32N` bytes（未计 frame）；每个已有 PS pass 每方发 `56+24N(d+1)` bytes。新核心、hidden state、membership 和 ABORT 没有 frame ABI，故完整在线字节仅有表达式 `known+select+membership+state+abort`，后四项未设计/未测。

轮数映射：算法迭代数5；明文比较 DAG 9层；若将9层算法完全放进符合相应 CA theorem 的构造，`2k+1` 条件数为19，但 S2 未验证该 theorem 的适用前提。Agarwal Protocol I paper core 3轮只是论文参考；现有 priority pipeline 6轮、现有 raw-score path 8轮不是本算法结果。新协议 core 和完整 path 仍 NOT_CLOSED，不填 11/14 或其他总数。

## 5. 门禁、证据与命令

| Gate | S2 状态 | 证据边界 |
|---|---|---|
| `ALGORITHM_FUNCTIONALITY` | PASS | 40,490 个合法用例与冻结 oracle 一致；所有采样带的正确性来自 rank 不变量，不来自抽样成功率 |
| `INDEPENDENT_ACCEPTANCE` | PASS_WITH_SCOPE | S2 独立 verifier 重建 edge/cause；同会话复核，不冒称异会话接收 |
| `COMPARISON_COST_BOUND` | PASS | exact finite worst + strict expected matrix upper；不是大矩阵精确均值 |
| `EXPECTED_COST_CLAIM` | PROJECT_DERIVED `O(n)` | 仅理想 fresh uniform sample 模型；无高概率成本结论 |
| `PROTOCOL_I_INTERFACE` | NO_GO | active C/sampler/pivot/state/τ 没有 runtime API |
| `LEAKAGE_MAPPING` | NO_GO/PENDING | 端点字段类型可映射到 S31 L，新增自适应分布、rank/control 和模拟未审 |
| `MATERIAL_COVERAGE` | NO_GO | 固定 ABI 无轮次；pool 超 package cap，需新服务和证明 |
| `ONLINE_DAG` | NO_GO | 仅有比较子层和已有 adapter；secret control、membership、ABORT 未闭合 |
| `SECURE_COMPOSITION` | NO_GO | 没有新路线 joint simulator / party-view proof |
| `FORMAL_PERFORMANCE_READY` | NO | V3 OPEN；没有 runtime 或正式矩阵 |

命令（输出放仓库外）：

```powershell
python -B experiments/PROJECT_DERIVED_SELECT_5ITER_S2/independent_accept.py --out-dir C:\Users\28641\.codex\evidence\PROJECT_DERIVED_SELECT_5ITER_S2_2026-10-12\run7
python -B experiments/PROJECT_DERIVED_SELECT_5ITER_S2/comparison_cost_bounds.py --out C:\Users\28641\.codex\evidence\PROJECT_DERIVED_SELECT_5ITER_S2_2026-10-12\run1\comparison_cost_bounds.json
python -B experiments/PROJECT_DERIVED_SELECT_5ITER_S2/material_access_prototype.py --trace C:\Users\28641\.codex\evidence\PROJECT_DERIVED_SELECT_5ITER_2026-10-12\final\representative_trace.json --matrix --paired-data-dependent-demo --out C:\Users\28641\.codex\evidence\PROJECT_DERIVED_SELECT_5ITER_S2_2026-10-12\run4\material_access_prototype_final.json
```

环境：Windows 11 `10.0.26200-SP0`、Python 3.13.7、MinGW g++ 8.1.0。最终 run7 oracle executable SHA-256 `694c54f1e2b6200a3fc54cd721e3cbfbad9f15cf3d711fc173d0306f2f76c36a`；results `bdf47fc66a1a17b9912b7b4fbc1efbac2b39a089e550455a4de0ec9b3e910751`；empty failures `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`；summary `d75f5d0e09e04f953080e9effa08c26c33a1eedfc8784e86f4b8530181defc76`。cost bounds JSON SHA-256 `d133364f63d37285ef9c99e488d988e7400aeff549c5d74415f79126c20e3807`；最终 material interface witness SHA-256 `b9a04297d643889a84b9ca71fb901fec82cb6fae7bd724f5e725f88816ba733c`。全长哈希及每次开发 run 记录均保存在 `C:\Users\28641\.codex\evidence\PROJECT_DERIVED_SELECT_5ITER_S2_2026-10-12\`；没有将 exe、trace、keys、sidecar 或 raw logs 提交进仓库。

S2 源文件 SHA-256：`independent_accept.py` `6dbd5b14a01e6de2796afde4624a953e5456b75c42e11a00ebe6de1f76490883`；`comparison_cost_bounds.py` `3810a095f7552ef233da17d15458ef4f24d976b3f024b94acb975f9942e6b40e`；`material_access_prototype.py` `3444405e0b1a782619bb4f9c9338e7d0b095f006eb2e6105f9aba06b6bf56a2d`。未构建/运行 secure runtime，也未运行独立进程协议 E2E，因为 Stage B 设计门未通过。

## 6. 本任务新增/修改文件

新增：

1. `docs/decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S2_DECISION_2026-10-12.md`
2. `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S2_REPORT_2026-10-12.md`
3. `experiments/PROJECT_DERIVED_SELECT_5ITER_S2/README.md`
4. `experiments/PROJECT_DERIVED_SELECT_5ITER_S2/independent_accept.py`
5. `experiments/PROJECT_DERIVED_SELECT_5ITER_S2/comparison_cost_bounds.py`
6. `experiments/PROJECT_DERIVED_SELECT_5ITER_S2/material_access_prototype.py`

修改：

7. `PROJECT.md` — 追加 S2 接收/成本与 Stage B 门禁状态。
8. `docs/IMPLEMENTATION_PLAN.md` — 追加 S2 状态、独立接受边界及下一准入条件。
9. `docs/REFERENCE_MANIFEST.md` — 追加 S2 成本/Protocol I interface supplement，保持 BB90/S1 历史。

没有修改 S1 源码和历史报告、S34、S31/S32 原件或安全运行时。
