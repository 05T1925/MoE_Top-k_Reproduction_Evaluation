# M6B-I S2：PROJECT_DERIVED_SELECT_5ITER 独立接收与 Protocol I 设计门

日期：2026-10-12（Asia/Shanghai）  
任务：`M6B-I-PROJECT-SELECT-5ITER-S2`  
算法身份：`PROJECT_DERIVED_SELECT_5ITER`  
明文测试标签：`PROJECT_DERIVED_SELECT_5ITER_TEST_ONLY`，revision `v1`

## 1. 裁决

- S1 明文 Stage A：S2 独立编写的 edge/trace replay 与冻结 oracle 重跑 `PASS`。这是同一执行会话中的独立接收复核，不是第二位接收方或异会话签收。
- 比较成本：最坏有限上界闭合；冻结矩阵每点给出严格有限期望上界；小规模精确 DP 与无记忆路径枚举在 20 个 `(n,K)` 点相等。对任意固定输入、理想均匀无放回新鲜样本，证明期望逻辑比较量 `O(n)`；最坏仍为二次。
- Protocol I 设计门：`NO_GO_FOR_INTEGRATION`。未实现安全路径、未创建 opt-in alias；活动候选集采样、图/材料访问、阈值选择、泄露映射、ABORT barrier 和联合视图模拟未闭合。
- 正式 LAN/WAN 未运行；M6A V3 仍 OPEN，V4 仍 `NOT_ALLOWED / NOT_RUN`。

本算法是项目推导方案，不使用 BB90 或 BMW16 的算法、成本、随机顺序假设、概率界或安全结论。BB90 S1 来源门继续 NO-GO；S34 状态不变。

## 2. 接收对象与稳定顺序

S1 源码没有 Git commit，接收对象以文件 SHA-256 固定：

| 文件 | SHA-256 |
|---|---|
| `experiments/TEST_ONLY_PROJECT_DERIVED_SELECT_5ITER/select5iter.py` | `8b8d3dbd087d601a4962fe61c7b35546aa834cebd0ed4736b4e6d5dca204606b` |
| `experiments/TEST_ONLY_PROJECT_DERIVED_SELECT_5ITER/audit_edges.py` | `b84591c49ec9f67484258011b0685ce9f6cbcebb56685457a80a571bfbcf2fc0` |
| `experiments/TEST_ONLY_PROJECT_DERIVED_SELECT_5ITER/frozen_oracle.cpp` | `a30fe76ab09552ea532ca0e732a19eb925e3d35db0eb99e1728a087f5e48a457` |
| `experiments/TEST_ONLY_PROJECT_DERIVED_SELECT_5ITER/test_select5iter.py` | `fe933e6d1d3e298e8b9144e8764468693b000cd6b1c12d3a1eb643e179c4cf56` |
| `VFSS/include/moe_topk/topk_oracle.h` | `270ed8e3c3c8eef3fb143641d376d4fae7726e14621cc1274c717c2620a12e22` |

输入为 `1≤n≤1,000,000`、`1≤K≤n`、signed Q20.12 原始 int32。稳定全序是 `(-score, original_index)`：score 降序，同分 index 升序。重复及全相等分数合法；oracle 语义来自项目冻结代码。

S2 verifier 只从 sample clique 的比较结果重建样本全序，再核对 qL/qH pivots、clique/star 边集、pivot 精确 rank、候选与 residual-rank 转移、第五轮全图、跨轮重复、mask 和冻结 C++ oracle。结果为 `40,490/40,490` 有效案例通过，含 `40,319` 个 `n≤7` 全排列/全部 K；60 个案例五个比较层均非空；固定输入的 32 个算法种子给出 32 个不同第一轮样本；5 个非法输入被拒绝。

因果修正：S1 的 plaintext harness 在记录 edges 前直接用分数排序 sample 并选 pivots。比较成本仍计入 clique，正确性不受影响，但不能据此说 pivot 在 clique 比较前已知。比较算法应先执行 sample clique，再从结果选 pivots，随后执行 pivot stars。因此每个收窄迭代有两个比较依赖层，最后全候选 clique 一层，总计 9 个明文比较依赖层，而不是五层并行比较。

## 3. 正确性、随机性和终止

状态 `(C,r)` 表示全局第 K 项在候选 C 中的 1-based rank 为 r。对四次收窄，`m=|C|`、`s=min(m,ceil√m)`，均匀无放回采样；`b=bit_length(n)`、`a=ceil√(2sb)`、`μ=s(r−1)/m`：

```text
qL=clamp(floor(μ)−a,1,s−1)
qH=clamp(ceil(μ)+a+1,qL+1,s)
```

sample clique 给出排序和 pivots `x=S[qL]`,`y=S[qH]`；两 pivot 对所有非样本候选各有一条星边，因此 pivot rank 在 C 中精确。若 pivot 等于 rank r 的项则精确终止；否则按其 rank 保留 `(x,y)`、y 后缀或 x 前缀，并准确更新 r。第五迭代比较剩余 C 的全 clique，返回 rank r。所有更新严格保留 rank 不变量，因此每种合法采样带均输出精确稳定第 K 项：`Pr[错误输出]=0`、合法输入的随机 abort 概率为 0。随机性只影响成本；固定输入次序不要求随机。无效输入预先拒绝，资源耗尽或内部不变量错误返回错误/无答案；不重试、不回退、不产生阈值或 mask。

Python seedable PRNG 只供 TEST_ONLY 重放，不是密码学 sampler。期望成本结论假设每轮对当前 C 条件均匀、新鲜、输入无关地采样；没有高概率工作量界。

## 4. 成本推导

每个活动收窄迭代逻辑无序比较调用数：

```text
e(m)=C(s,2)+2(m−s),  s=ceil√m (capped by m).
```

同层无重复；跨层再次出现的身份对是新逻辑调用，应使用新的一次性材料槽。若 `E=Σ|E_i|`，跨层唯一无序关系 `U=|∪E_i|`，重复调用 `E−U`。每条关系可按端点身份规范成一个 `<` 方向，反向取补，不需第二比较。逻辑请求、唯一材料槽、DCF Eval、字节均分开。

每次非终止状态移除两个不同 pivot，故第 i 轮后 `m_i≤max(1,n−2i)`。四轮每层至多 `e(n−2(i−1))`，最终 clique 至多 `C(n−8,2)`；另有 `e(m)≤3m`，故最坏 `12n+C(n−8,2)=O(n²)`。

有限期望上界用均匀子集次序统计量和间距分布。令 `X_j` 是 s 元均匀子集第 j 个次序统计量，`d=qH−qL`：

```text
P(X_qL=x,X_qH=y)=C(x−1,qL−1)C(y−x−1,d−1)C(m−y,s−qH)/C(m,s)
E[G]=d(m+1)/(s+1)−1,  G=X_qH−X_qL−1
Var(G)=(m−s)d(s+1−d)(m+1)/((s+1)^2(s+2))
```

首尾 endpoint gap 的均值为 `(m−s)/(s+1)`，方差为 `(m−s)s(m+1)/((s+1)^2(s+2))`。未触及样本边界时，低/高侧失括概率各由单侧 hypergeometric Hoeffding 界 `exp(−2a²/s)≤n^-4` 控制；触及边界则用相应 endpoint 二阶矩。独立脚本取 `dmax=min(s−1,2a+3)`，对每个 `3≤m≤n` 精确计算保守 envelope `g(m,n)=E[G²]+2E[endpoint²]+2m²/n⁴`，并取精确有理数 `C_n=max g(m,n)/(m floor√m)`。于是

```text
E[M_(i+1)^2 | M_i=m] ≤ C_n m^(3/2)
B0=n²; B_(i+1)=min(n², ceil(C_n B_i^(3/4))).
```

R2–R4 的期望边数上界为 `min(3 ceil√B_(i−1), e(n−2(i−1)))`，R5 上界为 `min(ceil(B4/2), C(n−8,2))`。又 `d≤min(s,2ceil√(2s bit_length(n))+3)`，所以 `g=O(m^(3/2)log n)`；固定四次更新后 `E[M4²]=O(n^(81/128) polylog n)`，最终 clique 次线性，前四个 sample/pivot 层总期望 `O(n)`，整个期望逻辑比较数 `O(n)`。这只是本项目推导，不是 BB90/BMW16 结果，也不是实测性能。

冻结矩阵的严格数值边界如下；expected 列为各轮期望上界，不是精确均值：

| `(n,K)` | clique max / 每轮 | pivot stars max / R1–R4 | R5 `m_max / C(m,2)` | 期望各轮比较上界 `[R1..R5]` | 期望总量上下界 | 最坏总量上界 |
|---|---:|---|---:|---|---:|---:|
| `(128,2),(128,8)` | 66 | 232,228,224,220 | 120 / 7,140 | 298,294,290,285,3,985 | 298–5,152 | 8,308 |
| `(256,2),(256,8)` | 120 | 480,476,472,468 | 248 / 30,628 | 600,596,592,588,20,666 | 600–23,042 | 33,004 |
| `(1,000,80)` | 496 | 1,936,1,932,1,928,1,924 | 992 / 491,536 | 2,432,2,428,2,424,2,420,380,795 | 2,432–390,499 | 501,240 |
| `(10,000,80)` | 4,950 | 19,800,19,796,19,792,19,788 | 9,992 / 49,915,036 | 24,750,24,746,24,742,24,738,46,130,141 | 24,750–46,229,117 | 50,014,012 |
| `(100,000,80)` | 50,086 | 199,366,199,362,199,358,199,354 | 99,992 / 4,999,150,036 | 249,452,202,710,151,071,121,176,586,008,931 | 249,452–586,733,340 | 5,000,147,820 |
| `(1,000,000,80)` | 499,500 | 1,998,000,1,997,996,1,997,992,1,997,988 | 999,992 / 499,991,500,036 | 2,497,500,1,231,470,631,539,382,722,3,838,985,655 | 2,497,500–3,843,728,886 | 500,001,490,012 |

精确 `C_n` 分别为 n=128: 8.6897，256: 13.5154，1,000: 28.6247，10,000: 97.0969，100,000: 144.3763，1,000,000: 168.5018。n=2..6 的 20 个 `(n,K)` 精确 DP 值都与无记忆路径枚举相等；未用模拟代替证明。

## 5. Protocol I 设计门和数据流

输入 shares 仍按当前 raw-score adapter 转成唯一升序 key `((raw XOR 0x7fffffff)<<index_bits)+original_index`；original_index 与 key share 一起隐藏 shuffle。协议不得直接使用 plaintext trace 的原始身份。活动 Select 流程需要：对秘密 active C 精确均匀采样；比较 sample clique；秘密选取 qL/qH key；比较两 pivot stars；以不泄露的 rank/state 更新候选 C；第五轮 clique；输出共享 threshold τ。随后对每个 padded record 用 `1−[τ<key_i]` 获得 `key_i≤τ` membership。稳定 key 唯一，所以恰 K 项。每方 membership 低位放进反路由 carrier，经 inverse shuffle 输出原序 XOR shares；TEST_ONLY controller 才能重构检查重量 K。

前三个算法步骤和阈值/路由没有现成的秘密状态 API。若公开 rank、pivot identity 或 active C，必须先将字段纳入泄露函数并提供 simulator；否则必须安全计算 sample、rank 比较、候选 compact 和 material lookup。现有 pipeline 不能做这些工作。

### 5.1 角色、在线依赖 DAG 与消息状态

角色是离线 Dealer T 和在线 P0/P1。P0/P1 输入 raw-score arithmetic shares，不将 score、rank、τ 或原序 mask 发给 T；T 只可按公开 `(session,N,K,w)` 和 input-independent randomness 预发 share/key packages，退出后不得参与在线阶段。新 round-aware pool 的 sidecar format 和安全生成尚未实现，因此这一 T contract 是设计要求，不是代码行为。

```mermaid
flowchart TD
  A[原始 score shares，本地] --> B[carry exchange，2N uint64/party]
  B --> C[sign exchange，2N uint64/party]
  C --> D[forward hidden shuffle，2 passes]
  D --> S{四次自适应迭代}
  S --> S1[对 secret active C 取均匀样本：API/轮数未设计]
  S1 --> S2[sample clique comparison layer]
  S2 --> S3[从 clique shares 选 qL/qH pivots：oblivious mux 未设计]
  S3 --> S4[pivot-star comparison layer]
  S4 --> S5[精确 pivot rank 与 secret (C,r) 更新/compact：未设计]
  S5 --> S
  S --> E[第五候选 clique comparison layer]
  E --> T[选择并保留秘密 K-th key τ：share API 未设计]
  T --> M[所有 padded record 做 key_i≤τ membership：DCF adapter 未设计]
  M --> R[inverse hidden shuffle，2 passes]
  R --> G[P0/P1 final success/ABORT agreement：未设计]
  G --> O[只在双方成功后释放原序 XOR mask shares]
```

这里是因果依赖图，不是已定义的 wire transcript。消息/公开字段逐项如下：

| 阶段 | 当前或拟议消息 | 在线公开/本方视图 | 状态 |
|---|---|---|---|
| 输入及 carry/sign | P0/P1 各交换长度 `2N` 的 masked 64-bit words，carry 后再 sign；每方 payload `16N` bytes/阶段 | session、N、K、width、phase/length；不公开 raw share | 当前 raw-score adapter 有实现 |
| forward shuffle | P0/P1 按 PermuteShare 两阶段发送 masked 192-bit records；每 pass 每方 `56+24N(d+1)` bytes | frame binding、长度；原序映射应保密 | 当前 shuffle 可复用，但 key/origin 同载绑定须验证 |
| 四次 Select | clique / pivot-star 所需边端点、masked operands、比较 share 或 open bit | 若公开 edge/pivot/slot/address 则自适应图对该方可见；若不公开需 oblivious key retrieval | 没有消息 ABI/公开字段决策 |
| sample 与状态控制 | sampler coin、qL/qH 选择、pivot rank、residual r、active C 更新 | 这些值本身可能泄露输入秩/候选集 | 没有 party message 或 secret-control circuit |
| 第五层与 τ | 候选 clique结果、selected key share | τ 与 selected 原序身份必须隐藏 | 没有 threshold extraction API |
| DCF membership | 每个 padded key 与共享 τ 的 masked operand material/Eval；输出 shares 不打开 | 不公开 membership bits、τ、mask | 没有本路线调用约定或 frame |
| inverse shuffle | P0/P1 两阶段反路由 carrier shares | 不公开 shuffle/index 映射 | 当前 reverse shuffle 可复用，carrier 适配待 conformance |
| 结果/失败确认 | 双方 final READY/ABORT barrier；任何一方失败双方无 mask | 统一 ABORT；不公开失败 rank/threshold | 当前本路线没有 agreement/no-mask API |

新路线不声明 online Dealer。在线字节只能写为已知 adapter/shuffle 项加未知 Select、state、membership、agreement 项；完整消息长度和轮数须待这些阶段有代码及 frame 后重新计算。

当前 Protocol I pipeline 固定 `C(N,2)` 边材、公开 masked priority 列表、全对全 CmpAgg 后 reveal shuffled ranks，再 inverse shuffle；没有自适应图或 threshold membership。`ProtocolIUcmpPartyMaterial` 一次性，strict `<` 一次调用两次 DCF Eval/party。party-key serialized bytes 精确为 `D(b)=57+24b`；这些是代码行为。对新路线没有实测材料或 Eval 数。

若继续复用 uCMP，令 `N=next_power_of_two(max(2,n))`、`w=33+log2(N)`、实际调用 E：每方条件槽数 `2N` 个 34-bit 输入转换 + E 个 w-bit Select + N 个 w-bit membership；DCF Eval 条件数 `2(E+3N)/party`、`4(E+3N)` 两方总和。跨轮重复需要 fresh key。隐藏数据且无 oblivious key service 时，输入无关的五迭代全池需 `5C(N,2)` Select key slots。

round-aware DCF sidecar/party 的最低序列化字节推导（不含 shuffle、oblivious access、transport、索引分配）为：

```text
40+8N+8
+2N(40+D(34))
+8+5C(N,2)(32+D(w))
+8+N(16+D(w)).
```

| logical n | N | w | Select 全池槽 | 最低 DCF sidecar bytes/party |
|---:|---:|---:|---:|---:|
| 128 | 128 | 40 | 40,640 | 42,998,400 |
| 256 | 256 | 41 | 163,200 | 175,853,760 |
| 1,000 | 1,024 | 43 | 2,618,880 | 2,938,774,080 |
| 10,000 | 16,384 | 47 | 671,047,680 | 816,714,752,064 |
| 100,000 | 131,072 | 50 | 42,949,345,280 | 55,362,113,306,688 |
| 1,000,000 | 1,048,576 | 53 | 2,748,776,448,000 | 3,741,088,079,151,168 |

当前 ABI 固定一对一 edge 与 `1,000,000` edge / `64 MiB` package cap，全部目标都不兼容；n≥256 的估算 sidecar 已大于 cap，n≥1,000 还超过 edge cap。数值是静态容量下界，不是实测。

已知在线 payload 只有复用适配项：raw carry/sign 为每方每阶段 `16N` bytes、两阶段每方送出 `32N` bytes（不含 frame）；forward/reverse 每个 PermuteShare pass 每方发送 `56+24N(d+1)` bytes，d 为 Beneš depth。新 Select、threshold membership、隐藏 state control 与 ABORT 帧没有 ABI，完整字节表达式只能记作 `B_known+B_select+B_membership+B_state+B_abort`，后四项 `NOT_DESIGNED/NOT_MEASURED`。

### 5.2 轮数严格分栏

| 口径 | 轮数/状态 |
|---|---|
| 算法迭代 | 5 |
| 明文比较依赖层 | 9：四组 clique→stars + 最终 candidate clique；秘密 sampling/state control 另有未闭合依赖 |
| CA 编译核心 | 若该状态式 sampler 完全满足适用 CA theorem 的隐藏图、预处理及输出前提，则条件值 `2k+1=19`（k=9）；尚未证明适用 |
| Protocol I 论文核心 | 3，Theorem 4.1 参考值，不等于本算法路线 |
| 当前 priority pipeline 工程核心 | 6 |
| 当前 raw-score→mask 工程路径 | 8（raw adapter 2 + 当前 pipeline 6），非本算法测量 |
| 新方案 Protocol I 核心和完整 Top-K | `NOT_CLOSED`，不报 11/14/其他总数 |

Known phase terms: raw adapter 2 + forward shuffle 2 + conditional CA 19 + threshold membership + inverse shuffle 2 + final agreement；秘密 candidate/sampler 控制、membership CA、ABORT phase 都须落实进实际 DAG，故不求和冒充端到端轮数。

## 6. 泄露函数与 S31/S32 复用界

S31 revision `a1a90cb2a568deb5e5078ddec10136ce9a743411` 的路线特定 L 包含 `public_z`/masked operands、匿名比较端点和 opened bit、pivot/U/V/W 匿名位置、匿名 handle、selected handle、key/slot ID 与访问次序/次数、比较次数、frame phase/length、sampler coin、统一 ABORT、本方 output share；排除 raw score、original-index 到匿名 handle 映射、明文阈值/full mask/另一方 shares。S32 revision `32cd7952cccdacc5a1a5b4fcca245f7acf594049` 主要变更计量口径，不新建安全证明。

| 本路线数据 | L 对照 |
|---|---|
| n,K,width/session/frame | 可能属于公开配置；需新 transcript 核对 |
| sample/pivot handles、edge endpoints、iteration、slot IDs/order/count | 字段家族像 S31 的 anonymous endpoint/handle/slot schedule；分布和 adaptive path 不同，必须新 simulator，不能继承 |
| `p_x,p_y,r'`、候选大小/bitmap | L 未明确授权这些 rank/control 值；公开需改 L，秘密化需 oblivious state update；PENDING |
| score、original_index 映射、τ、membership bits、full mask | 保持隐藏，只输出共享 mask |
| ABORT/error | 统一 ABORT 且双方确认前 no-mask；本路线 barrier 未设计 |

可复用部分仅是候选底层假设：main uCMP 序列化 `M2UC v1`、Bin 34–53、Bout 64、groupSize 1、strict `<` 和两 eval/party 的源结构与候选接口；membership 的 operand 方向、阈值编码及 boundary conformance 未建立。S31 single-key DCF lemma 可在 ABI/假设完全一致时重用；whole-pool/adaptive-query/shuffle-output simulator、优势计数、S31 ABORT composition不转移。S32 可复用指标定义，不可借其 measured fields 或安全结论；新路线所有材料、DCF Eval、AES、在线轮数和字节均 `NOT_MEASURED`。

## 7. 下一阶段精确准入条件与方案清单

Stage B 前必须冻结并评审：

1. 对秘密 active C 的均匀 sampler、样本排序/分位点选择、pivot rank 和 `(C,r)` oblivious update/compaction API；说明哪些 handle/state 可公开。
2. Round-aware one-shot 材料 ABI `(iteration,substage,left,right)`、重复边 fresh-slot 规则、输入无关池或 oblivious retrieval 的完整访问语义及规模准入。
3. shared τ 上 `key_i≤τ` 的方向、宽度、mask/DCF binding、边界与 count=K proof/conformance；稳定键/逆路由保留原序语义。
4. P0/P1 party-view 的 L 映射与联合 simulator，覆盖 hidden sample/graph、slot reads、未使用材料、membership、inverse、断连/ABORT/no-mask。
5. 完整消息 DAG、online byte expression、离线容量、party agreement barrier；设计评审通过后再 opt-in 实现，并顺序运行 conformance → frozen oracle differential → 独立进程 E2E。

此算法作为额外 `PROJECT_DERIVED` 研究线，不替代六方案里的 Protocol I+BB90+DCF 或 Protocol III+BB90+DCF，不修改正式方案名称和 M6A/V3→V4 顺序。
