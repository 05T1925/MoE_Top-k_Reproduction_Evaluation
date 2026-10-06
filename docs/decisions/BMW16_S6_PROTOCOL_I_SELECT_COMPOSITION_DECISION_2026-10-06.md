# M6B-I-BMW16-S6：Protocol I + BMW16-derived Select + DCF 组合决策

日期：2026-10-06
候选身份：`PROJECT_DERIVED`
明文算法来源标签：`BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R_TEST_ONLY`
审计标签：`BMW16_DERIVED_SELECT_PROTOCOL_I_COMPOSITION_REVIEW_TEST_ONLY`
设计门结论：**NO-GO_FOR_SECURE_IMPLEMENTATION**；键表示与离线覆盖算术门通过，联合隐私和安全接口门未通过。

## 1. 决策摘要

S6 对 S5 指出的组合缺口作了可执行收紧：以 S4 的严格全序和两哨兵中位数归约为输入，证明 signed Q20.12 分数与 `original_index` 可编码进当前 uCMP 支持的 34–53 位比较域；明确按 S4 实际使用的双实例 dummy 身份复算公共句柄池；加入独立的容量/位宽/边覆盖审计器，并用仓库冻结 C++ oracle 核验所选阈值方向。

上述正面结果**不足以通过 Protocol I 安全组合门**。S4 的 R2/R3/R4 比较边由先前秘密比较结果决定；现有 Protocol I pipeline 要求公开边列表并打开 masked priority values/ranks。现有 SecretSharedShuffle 只给出共享 payload，并无可与共享返回值及两实例共同 handle 绑定的公开 masked list；E20 的固定偏移发材会暴露材料地址访问。若打开比较结果或自适应 handle，可由本地输入相同但对方秘密顺序不同的执行区分；若隐藏图，当前缺少 secret-index gather/compact、边图控制和 oblivious key retrieval 原语。全对全扫描虽覆盖任意边，却把在线比较和 DCF Eval 推至二次量，不能作为未计入 fallback。

因此本提交只增加隔离的 TEST_ONLY 数学审计，不修改 `VFSS/`，不新增名为 `secure` 的入口，不复用/移动 E20 源码，也不运行 LAN/WAN 性能矩阵。审计结果没有证明密码学安全、BMW16/BB90 一致性，或完整入口轮数。

## 2. 起点、隔离与来源身份

| 对象 | revision / 身份 | S6 用途和保护 |
|---|---|---|
| main / `origin/main` | `c3926c68fd14f270faa8b55234311071947fa080` | S6 分支基点；在线 `git ls-remote origin refs/heads/main` 与该值相同 |
| S6 worktree | `codex/m6b-i-bmw16-s6`, HEAD `c3926c68fd14f270faa8b55234311071947fa080` | 最新 main 的独立干净 worktree；本决策与 TEST_ONLY 审计仅写于此 |
| 桌面主工作区 | 当前 checkout `feat/m6a-performance-evaluation`，HEAD 与 main 相同；原有 4 项 tracked 修改、4 项未跟踪资料 | 未改动；差异保留在桌面 checkout |
| S4 | `04ed6f8a352ab2277434344094471dff82f83e4b`, `codex/bmw16-s4` | 只读输入。S4 commit 不在 main 祖先；源码和报告均未 cherry-pick 或修改 |
| S4 Select 源码 | `select4r.py` SHA-256 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab` | 仅核读算法和计数；S6 程序不 import S4 模块 |
| S5 接收 | `67c3f302a8564b68616f7d7247c8cee3415ace31`, `codex/m6b-i-bmw16-s5` | 只读审查起点；保留其 S4 接收结论及 Protocol I NO-GO，不改写 S1–S5 历史 |
| E20 streamed materials | `8986a40175f5f8eef56d765fef0cfa863e205985`; secure-code-changing commit `a8f0ac47f9b3ebc4276d1aa0445f178acc1ef56f` | 只复核输入无关预发、分方 key、固定槽位/一次领取的适用边界；没有复制代码 |
| E20 接收证据 | external checkout `C:\Users\28641\Desktop\MoE_Top-k_Reproduction_Evaluation_Evidence\M6A\E20_receiver_8986a40`, detached HEAD `8986a401...`; report SHA-256 `0BB83D44A058CF5282B59A16724D7EB288048CFFDAEFB8997D8EE090B1353CA0` | `PASS_WITH_EXPLICIT_LIMITS`；multi-key joint simulation 仍 UNPROVEN，不作为 BMW16 组合证明 |
| E21 | `93d96f6c9f9d5c758e5471ea38158f05f1c7dea4` | 仅记录为未接收隔离候选；不借用其数据、标签或源码 |

S4、S5、E20/E21 与 S6 均是 main 之外独立 revision。S5 不包含 S4 程序；S6 明确把 S4 当成只读来源而非可链接依赖。论文 PDF 和本地参考树没有进入 S6 Git worktree 或差异。主工作区现有差异未复制到 S6。

## 3. 来源与事实分类

| 命题 | 分类 | 依据 / 限定 |
|---|---|---|
| 主算法论文为 `Papers/1603.04941v1.pdf`，44 PDF 页，SHA-256 `F46F83CBA279F37E9E3AAA0D64B145FDFCFBC52C0C8B79BACEC1259CA9CA23` | PAPER_DIRECT / 文件身份 | 论文原件从桌面只读路径核验；PDF 不进 Git |
| `Papers/017.pdf`，249 PDF 页，SHA-256 `3EAE3A93F12AB71EEA8AC040167F21351FCF3FF54DC423985A04087E3E90CB15` | 辅助来源 | 只作复核；与主论文内容重叠不构成独立证明 |
| noiseless comparison、Algorithm 5/7、Theorem 8、Appendix A Lemma 1 的页码与原文差异 | PAPER_DIRECT（由 S4/S5 原页审计继承） | PDF 页 7、28–29、34、11/33–34、16；S4 已记录 Algorithm 7 与 Theorem 8 对 epsilon 端点的不同写法，S4 sample-bracket 并不采用 epsilon，也不继承 Theorem 8 |
| 四层 `R1`–`R4` sample-bracket 代码、随机失败状态和计数界 | PROJECT_DERIVED（S4 已独立接收） | S4 不是逐字 Algorithm 7；只把其已接收的有限样本构造作为本阶段来源 |
| `M=2 max(n-K+1,K)`、两哨兵交集、键方向映射 | PAPER_DERIVED + PROJECT_DERIVED | 两哨兵中位数归约由 S4/S5 纸面审计；稳定 Q20.12 键和其偏移表示是项目映射 |
| 本文共同 dummy-handle 账本、离线全对池、容量式、TEST_ONLY key checker | PROJECT_EXTENSION | 只证明组合计数/编码/有限枚举，不是 BMW16 论文结论或安全协议 |
| 隐藏自适应图、共同 shuffle 视图、FSS/DCF 多 key 联合模拟、≤10^-6 secure abort | UNRESOLVED | 下文逐项列出缺少的理想功能、模拟定理或接口；不由通过的 arithmetic checker 推断 |

S4/S5 页面证据：主论文 noiseless model PDF p.7；Algorithm 5 p.29 和证明 pp.28–29；Algorithm 7 p.34；Theorem 8 pp.11、33–34；Appendix A Lemma 1 p.16。S6 未把 S4 行为倒写成论文 Algorithm 7 的步骤或 Theorem 8 的概率。

## 4. S4 双实例与稳定阈值接口

### 4.1 两哨兵算术

令原始输入有 `n≥1` 条记录，`1≤K≤n`，稳定 Top-K 次序为 score 降序、`original_index` 升序。S4 明文比较器的真实记录低到高顺序恰为 score 升序、同分 `original_index` 降序。其目标真实 rank（低到高、一基）为

```tex
r = n - K + 1
M = 2 * max(r, K)
h = M / 2
L = h - r
H = h - K
```

有 `L,H≥0` 且 `L+H=M-n-1`。low-sentinel 实例的有序列表是 `L` 个 low pads、low sentinel、n 个真实记录、`H` 个 high pads；真实 rank `r` 位于 `h+1`。high-sentinel 实例是 `L` 个 low pads、n 个真实记录、high sentinel、`H` 个 high pads；真实 rank `r` 位于 `h`。因此在两实例都正确 partition 时，`Accept_low ∩ Reject_high` 的真实记录唯一为低到高第 `r` 个，也就是稳定 Top-K 的边界记录。S4 实际过滤非真实 dummy，交集长度不是 1 时状态为 `COMPLETED_WRONG`；不会 oracle 修正。

边界公式覆盖 `K=1` 与 `K=n`（两端均 `M=2n`）、偶数 `n,K=n/2`、奇数 `n,K=(n+1)/2`、`K<n/2` 与 `K>n/2`。S6 的审计器逐一枚举 `n≤16`、所有合法 K，检查 `L/H`、两个真实 target rank、每实例长度及唯一 Top-K 阈值映射。S4 的实际 Select 仍在每个 `M` 项的列表运行；用于秘密 handle 的 power-of-two `D` 只是容量/寻址包络，不将额外存储地址当作新的 Select 项，也不重算或继承改变了的 Select 概率。

### 4.2 真实键、dummy 与比较方向

设 `raw` 是 signed Q20.12 的 32 位二补码位型，`i=original_index`，`b=max(1,ceil(log2 n))`，`o=raw XOR 0x80000000`。整数 `o` 随 signed score 升序严格递增。定义

```tex
R(score,i) = (o << b) + (n - 1 - i)
Rmax       = (2^32 - 1) * 2^b + (n - 1)
P(score,i) = ((2^32 - 1 - o) << b) + i
R          = Rmax - P
```

`P` 是现有 Protocol I priority-key 的数学形式：高 score 优先、同分低 index 优先。`R` 因而精确反转到 S4 的低到高顺序。`0≤n-1-i<2^b`，相邻 score 桶相差 `2^b`，所以 score/index 组合键无碰撞，且合法 `INT32_MIN/MAX` 都处于普通真实键域，不与 dummy 依靠保留 score 值区分。

对 S4 的 `L/H`，候选共同数值编码为：

```tex
low pads j                 -> j                    (0 <= j < L)
low sentinel               -> L
real record i              -> L + 1 + R(score,i)
high sentinel              -> L + Rmax + 2
high pads j                -> L + Rmax + 3 + j      (0 <= j < H)
```

各实例只激活自己的 sentinel；不激活 sentinel 的另一实例仍使用同一真实键。重复 score 不冲突，dummy class 不靠“挪用 3 个 tag 位”区分。最大可能编码值为 `Rmax+M-n+1 = 2^(32+b)-2^b+M`。因为 `M≤2n≤2^(b+1)` 且 `b≥1`，最大值严格小于 `2^(33+b)`；所需 `Bin=33+b`。于是 n=128/256/1000/10^4/10^5/10^6 的 Bin 分别为 40/41/43/47/50/53，均在当前 uCMP `34..53` 范围。n=1 用 `b=1`、Bin=34；当前 Protocol I 项目输入上限 n=10^6，范围内不溢出。若将来 n>2^20，当前比较器宽度不够，接口必须拒绝或另开比较器版本，不能截断。

成功选择后阈值为 `R_selected`，真实成员规则是 `R_i ≥ R_selected`，恰有 `n-r+1=K` 项。相等只可能是同一稳定复合键；因此可把自比较的成员值设为 1，其他 n−1 项需要比较。该方向与 S5 用 `P_i≤P_selected` 等价。shuffle 后 `i` 仍须随共享记录携带，不能换成 shuffled handle。上述仅是明文数学编码/接口提议；secure sign-lift、carry、常数偏移和原 index shares 的生成/绑定没有实现。

审计没有把 dummy 类别公开化。若公开 category、真实/dummy 标识或不同实例 sentinel 所在 handle，可能把记录身份和 source index 与已知的本地 shuffle 状态关联；只证明整数编码互异并不允许公开该字段。明文 TEST_ONLY 可检查清晰标签，secure 输入必须让类别和激活位留在份额/固定控制电路中。

## 5. 比较请求、FSS Eval 与材料槽不是同一计数

| 指标 | 定义 | S6 结论 |
|---|---|---|
| 逻辑比较调用 | S4 一次请求一个 noiseless strict comparison；包含有序请求重复 | 按 S4 上界推导；本 S6 未运行 Select |
| 唯一无序关系 | 忽略方向后的 canonical handle pair | 仅作图结构统计；不能据此把一次有向调用当成另一次调用或省掉材料 |
| 同轮/跨轮重复调用 | 重复请求仍是算法调用；同一 pair 在不同 stage 必须分 domain | 新代码输出离线池覆盖，未执行真实 S4 图，实际重复数 `NOT_RUN` |
| real-real / dummy 相关边 | 按端点类别分类的请求 | 若类别/edge 可见会带来额外泄露；S6 未生成请求，实际两项均 `NOT_RUN` |
| uCMP Eval | 当前 `ProtocolIUcmpPartyMaterial::eval_strict_lt` 每方调用底层 `evalDCF` 两次 | 假设复用该比较器时，逻辑边 e 对 P0/P1 分别 2e、合计 4e 次 DCF Eval；不是密钥数 |
| DCF membership Eval | 选中阈值对每条 real key 的最终成员函数 | S6 没有 secure key binder 或 DCF membership runtime；实际数 `NOT_IMPLEMENTED` |
| 离线 key slot | `(session,party,stage,round,instance,ordered endpoints,domain,material ID)` 一次性 party key record | 不同方向、轮次、实例各占独立槽；只有经证明的 share-complement 转换才可按无序边复用 |
| 公共结果 | 对任何合法秘密字段显式开放的数据 | S6 secure 候选应为 0 个公开比较位、selected handle、index、mask；当前不实施，无法报告运行值 |

当前 uCMP 代码在每个 party 的 strict compare 内做两次 `evalDCF`。S4 的 R2 对两个 pivot 分别发出 `item→pivot`，当 item 与 pivot 都属于 sample 时，同一无序 pivot-pair 会以两个方向出现；这是同一轮内的方向重复，仍是两个逻辑调用。当前材料合同没有证明反向共享比较位可通过局部取补得到，也没有安全的一次性 key 缓存合同。因此容量主表按有序端点为每次可能请求预留独立槽：每个实例、每轮全池为 `D(D−1)`；两实例四轮加真实 membership 的最坏全池为 `8D(D−1)+n(n−1)`。无序公式 `8 C(D,2)+C(n,2)` 只能作为明确标注的条件优化，前提是 comparator share representation 证明反向结果变换正确、无需额外 key/Eval，且缓存不会改变可模拟视图。即使该代数优化成立，也不解决隐藏自适应地址访问。

### 5.1 S4 项目的线性比较上界

每个 `M` 项 median Select 的 S4 项目上界为 `R1≤42M`、`R2≤2M`、`R4≤25M`；`R3≤2048M`（`M≤4096`），大规模时由 `|U|` cap 与 V 规则得到 `R3≤657M`。两个 sentinel Partition→Select 实例在同一层并行，所以：

| 有效 M | 两实例 R1 | 两实例 R2 | 两实例 R3 | 两实例 R4 | 合计上界 |
|---|---:|---:|---:|---:|---:|
| `M≤4096` | `84M` | `4M` | `4096M` | `50M` | `4234M≤8468n` |
| `M>4096` | `84M` | `4M` | `1314M` | `50M` | `1452M≤2904n` |

这里 `M=2 max(n-K+1,K)≤2n`；两个实例共享四个比较层，不串行为八层。D 只用于离线/地址覆盖，所以算法比较成本按 M 而非 D 计算。若每个边请求交给当前 uCMP，两方总 `evalDCF` 次数上界为上述逻辑调用总数的 4 倍；实际成本没有运行。该界是 S4 `PROJECT_DERIVED` 构造的线性界，不是 BMW16 Algorithm 7、BB90 定理或 BB90 `c·n` 常数。

## 6. S5 容量复算与 S6 方案账本

S5 报告的全池公式按两实例、每实例四阶段各保留完整 M-clique，再加 final real membership 无序 pair 集：`8 C(M,2)+C(n,2)`。这是 canonical 无序边估计；因 S4 会产生反向有序请求且反向共享材料转换未证明，不能称为方向安全的一次性槽数。保守的有向复算为 `8M(M−1)+n(n−1)`。固定样本 canonical 候选是

```tex
2 * ( C(s,2) + M*s - C(s+1,2) + 2*C(M,2) ) + C(n,2)
s = min(M, ceil(8*sqrt(M)))
```

`M*s-C(s+1,2)` 是固定 sample set S 上所有可能 pivot 对在 R2 形成的无序边并集；其方向安全版本是 `s(M−1)`，因为对每个 sample pivot 都可能发出 `item→pivot`，sample-sample 对出现两个方向。R1 无序 pool 为 `C(s,2)`，方向安全 pool 为 `s(s−1)`；R3/R4 无序 pool 每轮 `C(M,2)`、方向安全 pool 每轮 `M(M−1)`。固定样本公式还要求 sample handle 位置可输入无关固定且不会从单方视图/材料访问中泄露。S6 脚本逐子集、逐 pivot pair 枚举小 D，同时验证无序边并集和有序调用并集。两者都只是离线覆盖算术；有向版本避免了未经证明的反向 key 复用，但不解决自适应访问隐私。

因此 S6 固定样本有序 pool 的完整条件公式为 `2*(s(s−1)+s(D−1)+2D(D−1))+n(n−1)`；若未来 comparator complement sharing 证明成立，才可评估 canonical 版本 `2*(C(s,2)+D*s−C(s+1,2)+2*C(D,2))+C(n,2)`。`s=min(M,ceil(8√M))`。两个公式都是存储覆盖候选，不是实际请求数、DCF Eval 数或可泄露访问方案。

S4 当前明文构造共享两实例的真实记录 ID，但每个实例拥有自己独立的 pads 与 sentinel。忠实保留身份时两实例句柄并集为 `H= n + 2(M-n)=2M-n`；为现有 Protocol I power-of-two shuffle/存储作保守包络 `D=next_power_of_two(H)`。S6 比 S5 多列一个 PROJECT_DERIVED 选择：若能证明两个实例共用同一身份的 L/H pad handles，则并集可缩到 `M+1`。S6 未把该压缩身份方案用作保守容量，也没有安全视图证明。

| 配置 | M / D | Bin S5→S6 | S5 报告无序 slots（条件） | S5 有序保守 slots | S6 D 全池有序 slots | S6 D 无序（条件） | S6 固定样本有序（条件） |
|---|---:|---:|---:|---:|---:|---:|---:|
| n128 K2 | 254 / 512 | 43→40 | 265,176 | 530,352 | 2,109,312 | 1,054,656 | 1,226,112 |
| n128 K8 | 242 / 512 | 43→40 | 241,416 | 482,832 | 2,109,312 | 1,054,656 | 1,221,534 |
| n256 K2 | 510 / 1,024 | 44→41 | 1,071,000 | 2,142,000 | 8,445,696 | 4,222,848 | 4,690,974 |
| n256 K8 | 498 / 1,024 | 44→41 | 1,022,664 | 2,045,328 | 8,445,696 | 4,222,848 | 4,685,446 |
| n1000 K80 | 1,842 / 4,096 | 46→43 | 14,063,988 | 28,127,976 | 135,183,960 | 67,591,980 | 71,144,824 |
| n10^4 K1 | 20,000 / 32,768 | 50→47 | 1,649,915,000 | 3,299,830,000 | 8,689,662,448 | 4,344,831,224 | 4,471,571,296 |
| n10^5 K1 | 200,000 / 524,288 | 53→50 | 164,999,150,000 | 329,998,300,000 | 2,209,018,961,248 | 1,104,509,480,624 | 1,113,286,825,408 |
| n10^6 K1 | 2,000,000 / 4,194,304 | 56→53 | 16,499,991,500,000 | 32,999,983,000,000 | 141,737,453,800,896 | 70,868,726,900,448 | 71,463,891,079,296 |

按 `97+24Bin` 字节/party/slot 的下界估算，S5 报告无序 slots / 修正后的有序 slots / S6 D 全池有序 slots 对应：n128 K2 `299,383,704 / 598,767,408 / 2,229,542,784 B`；n256 K2 `1,234,863,000 / 2,469,726,000 / 9,129,797,376 B`；n1000 K80 `16,890,849,588 / 33,781,699,176 / 152,622,690,840 B`；n10^4 K1 `2,139,939,755,000 / 4,279,879,510,000 / 10,644,836,498,800 B`；n10^5 K1 `225,883,836,350,000 / 451,767,672,700,000 / 2,865,097,593,273,856 B`。n=10^6 的旧 56-bit S5 比较器不受当前 uCMP 支持，故旧格式容量记 `NOT_SUPPORTED`；S6 的 53-bit 有序全池字节下界为 `194,038,574,253,426,624 B`。固定样本有向 pool 每方下界及各配置原值见 JSON；无序固定样本容量仍需额外反向 share-transform 证明。

规范机器结果在 [S6 capacity audit JSON](../reproduction/evidence/BMW16_S6_KEY_CAPACITY_AUDIT.json) 中逐配置列出 M、原始 S4 sample s、源码身份并集、D、S5 报告无序估值、有序保守 pool、条件无序/固定样本 pool、当前 package/domain 检查、Bin 及 byte lower bound。每方 slot 字节按 S5 accounting 模型 `24B edge envelope + 16B endpoint masks + (57+24Bin)B 当前 Protocol I 序列化 key = 97+24Bin` 计算。它是格式下界/预检，不是 E20 序列化格式，未含 AEAD、索引、文件系统、Dealer transient 和传输缓存。当前 `ProtocolIUcmpPartyMaterial::serialize()` 的 key 本体是 `57+24Bin` 字节；E20 `24Bin+24` 属于另一材料格式，不能替换本表。

在 S6 目标 `(n=1000,K=80)`，S4 `M=1842`，源码忠实 union `H=2684`，存储 `D=4096`，每个 D-clique 为 `8,386,560` pair，超过当前 `protocol_i_party_package.cpp` 的 1,000,000 edge/package 检查。S5 的 M=1842 clique 本身也是 1,695,561 edges，已超过该上限。n=10^6 的 `D=4,194,304` 还超过当前 Protocol I layout 的最大 padded domain `1,048,576`；即使比较位宽可容纳，也需要新的 handle-domain API。单独 E20 streaming 可解决落盘/传输形态问题，但不能证明多阶段动态 key 访问、联合隐私或一次性 key 绑定。固定样本 pool 即使代数上缩小，也未越过访问模式和 active-set 证明门。

脚本区分：逻辑请求数（本阶段 Select 未运行）、有序单次材料槽、无序关系去重数、每方 serialized key records（每个有序槽 P0/P1 各一份）、同轮方向重复及跨轮/实例重复（实际 S4 图未运行）、real-real/dummy 实际边数（未运行）、uCMP DCF Eval 假设数、final membership DCF Eval（未实现）。方向安全全池是覆盖上界，不代表 S4 实际会调用所有有向边；实际生成/落盘、离线/在线时间、双方收发、PRG 和 abort 率均 `NOT_MEASURED`。

## 7. Protocol I 组合接口草案与联合视图门

候选成功接口目标：

```tex
request = (session, fingerprint, n, K,
           additive shares of raw signed Q20.12 scores,
           secret-bound original_index for each input record)
result  = SUCCESS(mask_share_P0, mask_share_P1)
        | ABORT(reason_class, no mask)
```

成功分支应等于冻结 C++ oracle 的原序 Top-K mask，每次恰 K 个 1；不公开 kth、`original_index`、比较位或 mask。抽样中止、材料错误、通信错误要分 reason class；双方完成绑定的一致中止后才返回 ABORT，禁止以全零 mask 表示无输出、重试、排序或 oracle 修正。只要每个比较、共享选择、DCF、shuffle 与逆路由原语精确，Select 成功分支可条件推出精确 mask；这不是当前已实现 API。

| 阶段 | 输入 → 期望输出 | 本次可复用事实 / 必须新增 |
|---|---|---|
| raw input | 32-bit score arithmetic shares + 原序 index binding → R-key shares | 当前窄 Protocol I raw-score/priority adapter 能作局部参考；需改为秘密 index carry、符号拓展、偏移加法和 key width 绑定，不能用 shuffled slot |
| common hidden handles | 同一真实 record、R key、class、两个实例 dummy IDs → shared payload under common hidden real permutation | 现有 SecretSharedShuffle 输出 shared payload；S4 两实例真实 ID 相同。需要证明跨实例 real handle 相同且不公开 index/class；现有 API 不产出与 shares 同置换绑定的 public masked list |
| R1..R4 compare | 每层冻结秘密端点 → 每条边 strict comparison bit shares → 下一层 U/V/W/pivot shares | 比较层数 4；需要 secret-index uCMP selection、oblivious graph builder、固定长度 transcript。当前 pipeline 的 graph/edge 顺序公开且 masked keys open，不可接用 |
| two-sentinel Select | secret partition relation → secret one-hot selected real/key | S4 数学集合交集为唯一 rank r record；secure intersection/one-hot gather 与两实例 key/handle 对齐缺失 |
| DCF membership | selected R threshold + every real R → membership shares (`R_i≥R_selected`) | logical star 为 n−1 个真实比较；潜在有序阈值/record 地址池为 `n(n−1)`。`C(n,2)` 只在反向 share complement 和单次材料复用均有证明时适用。secret selected-handle lookup、one-hot mux、材料访问模拟未实现 |
| inverse route | shuffled membership shares + same inverse shuffle → original order | 已有 reverse Permute+Share 构件，组合 ID/score/index/两实例的正确性和模拟未证 |
| bit output | additive shares of 0/1 membership → XOR bit shares | 若份额确在 `Z_(2^w)` 且重构值严格为 0/1，双方各取 LSB 的 XOR 等于 membership；carrier 与 abort gating 尚未 conformance |

### 7.1 公开字段与可复现泄露反例

安全模型目标暂定：T 可信、非合谋，只在输入未知时离线生成 input-independent 材料并交付 P0/P1；T 在线静默、不要求擦除。P0/P1 在线半诚实。任意材料必须绑定 `(session,fingerprint,party,endpoint,round,instance,comparator-domain,mask-ID,key-ID)`，一条 party key 只领取一次。S6 没有证明材料分发方/OS、party 本地 shuffle coin、两实例关联、abort transcript 的联合 simulator。

| 字段/动作 | 产生时刻 | 若公开的风险 / 当前状态 |
|---|---|---|
| R1 sample handles | R1 开始前；sample tape 固定后 | 样本成员是输入顺序上的随机 rank 子集；party 结合本地 shuffle、active sentinel/dummy 或 key slot 可关联记录 |
| pivot handles、R2 edges | R1 结果后 | pivot 是样本 rank 统计量；edge address 依赖秘密结果；E20 固定偏移读写暴露 endpoint slot |
| compare bit / U,V,W / q,qW | 比较边结果后 | 是次序和阈值信息；无法从“随机置换下裸图形状”模拟完整 view |
| selected handle/key | R4 或两实例交集后 | 直接暴露第 K 个记录、index 或稳定 key；必须保持 secret one-hot |
| failure flag/reason/time | 各 cap/bracket 分支后 | S4 abort 事件与 input rank 相关；需给 reason-class 的可模拟分布并达成双方一致 abort |
| material ID、加载 offset、未读 key | 每次比较前 | 可能泄露活跃图；固定地址文件访问可观察；key cache 会改变一次性/联合分布 |

反例：取 `n=2,K=1`，让被腐化 P0 的 raw input additive shares 在两次执行中都为 `(0,0)`；P1 持有 `[0,1]` 或 `[1,0]` 的相应份额。P0 的本地输入视图一致，理想输出只应给出对其随机输出 share 的模拟，但若任一真实比较结果/端点对应关系被公开，哪条记录优先会翻转，区分两输入。S4 实际选择数组有 dummy 扩张；在其中两真实 key 的比较边上同样成立。若 P0 本地置换或材料地址还能关联 handle，则暴露更直接。当前代码没有 hidden graph-control functionality 可阻止该公开结果。

保持边、U/V/W 和 material address 隐藏的候选必须以秘密 bit 运算生成定长节点/端点选择并通过 oblivious gather/MUX 获取 key，或做全池 scan。无方向复用证明时，全池 scan 的 uCMP 在线调用是 `D(D−1)`/阶段/实例；即使以无序关系计也是 `C(D,2)`，均为二次量并破坏线性 online comparison 目标；D 级 ORAM/oblivious circuit 的具体成本和视图证明目前没有。S6 不把“读隐藏 pair key”写成零通信、零轮操作。

## 8. 离线覆盖、概率和在线消息 DAG

| 因果阶段 | 输入/结果 | Select 比较层 | 在线安全消息轮数 |
|---|---|---:|---|
| O0：T offline | input-independent stage/round/instance/pair uCMP/DCF、shuffle、membership、inverse route 材料；分方封装后 T 静默 | 0 | offline-only；生成量/时间未测 |
| raw score lift + R key shares | raw arithmetic shares、原始 index shares → R shares | 0 | 现有窄 raw adapter 有自身交互，但 BMW16 接口未定，完整轮数未推导 |
| common hidden shuffle | R、record payload、original_index、secret class | 0 | 当前 forward share shuffle 构件为 2 rounds；与两实例共同 handle 的 composition 未证明 |
| R1 | frozen R1 edge batch → comparison shares | 1 | secure edge 输入/输出消息未实现 |
| R2 | secret R1 sample ranks/pivots → R2 edge batch → U/prefix/suffix shares | 1 | 依赖 R1；不得与 R1 合并 |
| R3 | R2 U/q → V×U relation → W/qW shares | 1 | 依赖 R2；越界须统一 abort |
| R4 | R3 W/qW → strict ranks → partitions | 1 | 依赖 R3；不得公开 `W` 或结果 bits |
| selected record/key | `Accept_low∩Reject_high` secret intersection → one-hot key | 0 comparison layer (but secure mux/routing work remains) | `UNRESOLVED` |
| DCF membership | selected R star relation → shuffled mask shares | 0 Select layer | star lookup/circuit/message rounds `UNRESOLVED`; full-pair scan quadratic |
| inverse route | mask shares → original order | 0 | 当前 reverse share shuffle 构件为 2 rounds；组合条件未证 |
| conversion + agreed output/abort | membership shares → XOR mask shares or agreed ABORT | 0 | LSB conversion可局部完成的前提见上；一致 abort handshake 未设计 |

“四轮”是明文 Select 的比较依赖层数，不是 raw input 到原序 mask 的 secure 在线轮数。可复用子构件的 2-round forward/reverse 只描述现有接口自身；不得相加成最终轮数或称为完整 secure endpoint 轮数。在线总轮数、消息 bytes、offline keygen、PRG、abort 频率目前均 `NOT_DERIVED` / `NOT_MEASURED`。

S4 已独立接收的理想抽样失败界：目标官方最大 `M=1842` 时，两份独立随机 tape 的完整 Select 失败上界 `<4.50140648×10^-7`；成功 partition 的两哨兵交集唯一且精确。该数依赖理想独立均匀无放回抽样/S4 证明，不是 BMW16 Theorem 8，也不是密码学 PRG/shuffle 统计界。与用户工程目标 `10^-6` 比，最多只余约 `5.49859352×10^-7` 给采样偏差、PRG、shuffle/控制分布等误差，但 S6 没有分配或证明各项；不能直接把它们当独立统计概率相加。TEST_ONLY Python RNG 不是密码学 tape。理想构造条件下可要求 `Pr[wrong output | SUCCESS]=0`，但当前接口并不存在，secure 错误/abort API 也未实现。

## 9. 门禁与精确后续条件

| Gate | S6 判定 | 通过条件/缺口 |
|---|---|---|
| stable real key and dummy total order | **GO for arithmetic/test-only specification** | 已给出无溢出严格编码并与冻结 oracle 对照；secure share-lift/offset conformance 仍待做 |
| common handle and two Select intersection | **NO-GO** | 固定 ideal functionality：同一 real item 在两个实例共用 handle；dummy/sentinel active set 秘密；两方只能获得 secret one-hot selected key；证明输入共享和本地 shuffle view |
| secret adaptive graph control | **NO-GO** | 需四层定长 secret edge scheduler：从 share comparison bits 产生 pivot/U/V/W、secret gather/compact、R2–R4 endpoints 不公开，给出 rounds/material/communication bounds |
| T material coverage and retrieval | **Arithmetic coverage GO; secure retrieval NO-GO** | S6 公式和小图枚举通过；实现 session/party/stage/round/instance/pair/domain/key ID 一次性绑定，并提供不泄露 active offset 的读取功能。当前 package 对 n1000/K80 每 R3/R4 D-clique 超 1M 上限 |
| one-party joint view simulation | **NO-GO** | 对 P0、P1 分别模拟本地 shuffle state、input shares、两个实例 linkability、FSS key pool/access、公开 transcript、abort 与输出 share；S5/E20 均未给该证明 |
| DCF threshold binding and exact membership | **NO-GO** | 需要 secret selected one-hot/key gather 和所有 i 的 `R_i≥R_selected` 精确 bits；证明 star-key 绑定/一次领取/访问模式。全 pair scan 是 quadratic，不是解决方案 |
| inverse route, XOR output, agreed abort | **NO-GO** | 绑定同一个 permutation 和 mask/index carrier；成功恰 K、与 oracle 逐位相同；双方通信失败共同 abort 且不释放部分/全零 mask |
| secure abort ≤10^-6 | **NO-GO** | 仅有理想 S4 抽样失败界；安全随机/PRG/shuffle 统计误差、实际抽样与 abort simulator 未给 |
| Protocol I secure implementation | **NO-GO_FOR_SECURE_IMPLEMENTATION** | 在以上缺口闭合并独立审查前不创建入口 |

本轮独立 checker 与 C++ oracle 的通过，只证明列明有限枚举和计数公式。它们没有运行 S4 Select，不测试 FSS keys、shuffle、DCF、网络或多进程，所以不能转写为 secure correctness/security PASS。

## 10. 后续开始安全实现前的最小审查包

1. 一份 protocol-agnostic ideal functionality/消息 DAG，明确定义原序输入 index shares、同一真实记录跨两实例 stable handle、secret dummy active set、secret select、mask 与双方 abort。
2. 可审查的 P0/P1 联合 simulator 或混合归约，逐字段覆盖两实例、本地 permutation/inverse、材料池、offset/未读槽、公开次数/时序和最后输出 share；明确 E20 T 不在线、OS 文件访问面是否纳入模型。
3. 一个可调用的 secret-index oblivious comparison scheduler：固定四个 Select barriers，不公开活动 endpoint、比较位、U/V/W 或 selected handle；含跨轮单次 material binding 和 worst-case slot/online Eval 预算。若改用全池计算，必须重新接受二次复杂度，不得声称线性在线比较。
4. 对 S6 `R` 编码做 C++ share conversion/uCMP 全边界 conformance（n=1、n=10^6、INT32_MIN/MAX、tie index、low/high sentinel、L/H 0 和非零）；拒绝超 53-bit 配置。
5. 离线 DCF membership binder：用同一 record handle + R shares + secret threshold one-hot 生成逐项精确 `R_i≥R_selected`，不可让 T 看到阈值/输入；明确 star access 的 oblivious 实现和 key slots。
6. 同置换 inverse route 和 additive-bit LSB→XOR share的组合 proof/conformance，以及正式 API `SUCCESS(mask shares)|ABORT(reason_class,no mask)` 的一致 abort 握手。
7. 将 S4 ideal abort `4.50140648e−7` 与安全 PRG/shuffle/sampling-distance 各自定理参数分配在 ≤`10^-6` 内；不能直接假设统计独立。
8. 上述设计独立 review 通过后，才在 `VFSS/` 新建明确 `PROJECT_DERIVED` 标签；按 conformance → 冻结 oracle differential → 独立 T/P0/P1 E2E 验证 n≤8，然后再决定扩大规模。
