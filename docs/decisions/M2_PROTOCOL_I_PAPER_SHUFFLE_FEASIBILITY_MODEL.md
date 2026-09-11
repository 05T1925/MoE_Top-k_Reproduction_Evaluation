# M2 Protocol I paper-compatible shuffle feasibility model

状态：**FEASIBILITY_GO（仅 Gate A；不代表 secure candidate 已实现）**

日期：2026-09-11。

本文件是阶段三A的设计与验收 owner。它把阶段二的三个 BLOCKED 根因拆成可执行的 TEST_ONLY algebra model：same-permutation、correlated-r/GRank material 和完整 causal transcript。模型不接入正式 M2/M3 runtime，不链接正式生产库，不提供 paper-exact、exact 3-round 或安全三轮实现。

## 1. 范围、证据等级和结论边界

本阶段只判断是否具备进入下一阶段隔离 secure candidate 的条件：

- A：论文定义，来自已校验的 Agarwal Protocol I 和 Chase shuffle PDF；
- B：本地参考行为或当前 VFSS 静态行为；
- C：项目冻结的 Q20.12、stable tie、logical/padded、原顺序 mask 约束；
- D：本模型的候选代数、视图和 transcript，尚未实现为安全协议。

本阶段的最终结论为 FEASIBILITY_GO：Gate A 的代数、三方视图、输入无关性、causal transcript 和 TEST_ONLY 不变量均有设计与执行证据。Gate B（隔离 secure candidate）和 Gate C（论文精确标签）仍未通过。

## 2. 数学对象

### 2.1 环、score 和 priority-key

每个逻辑输入的原始 score 是冻结的 signed Q20.12 32-bit two's-complement word，scale 为 4096。模型先按项目规则将 score 映射到 priority-key：

1. raw word 与 sign bit XOR，得到 unsigned ordered score；
2. 取 UINT32_MAX - ordered_score，使较高 score 具有较小 key；
3. 将 key 左移 index_bits(logical_n)，并 OR 原始 index；
4. priority-key 宽度为 32 + index_bits(logical_n)。

候选 core 在环 R_w = Z/(2^w Z) 上进行 additive shares，其中 w 是当前 logical_n 的 priority-key 宽度，所有加法、减法和 mask 均按 2^w 取模。模型不把这个项目 ring 直接写成 Agarwal 论文的抽象群证明；它只验证冻结项目域中的代数一致性。

### 2.2 logical_n、padded_n 和 payload

真实输入长度为 logical_n，工作向量长度为 padded_n。模型覆盖 logical_n ∈ {1, 2, 3, 5, 7, 8}，其中 padded_n 是不小于 logical_n 的下一个 2 的幂。dummy slot 使用低优先级的最大 key，不进入 logical rank 或 Top-K oracle。secret payload 的每个 slot 包含 priority-key、原始 index 和 dummy marker；public masked list 只包含 masked priority-key，不公开 index 或 marker。

### 2.3 局部 permutation 与复合顺序

定义 Perm(p, v)[s] = v[p[s]]。P0 和 P1 分别持有局部 permutation p0、p1，先应用 p0，再应用 p1：

shuffled = Perm(p1, Perm(p0, v))

因此按 slot-index 表示，复合 permutation 为：

pi[s] = p0[p1[s]]

这里的公式明确固定了数组索引方向，避免把函数复合方向与 wire slot 方向混淆。模型同时验证 staged application 和 direct composite application 逐槽相等。

### 2.4 r、public list、payload 和 GRank material

P0/P1 各自持有等宽随机 mask share：

r[s] = r0[s] + r1[s] mod 2^w

对 priority-key 向量 x，候选 public masked list 为：

public_y[s] = shuffled_key[s] + r[s] mod 2^w

secret shuffled payload 的 shares 重构为：

payload_share_0[s] + payload_share_1[s] = Perm(p1, Perm(p0, payload))[s]

GRank material 的本地 mask share 定义为：

grank_mask_share_0 = r0
grank_mask_share_1 = r1

因此模型检查：

public_y[s] - shuffled_key[s] = grank_mask_share_0[s] + grank_mask_share_1[s]

该等式验证的是同一个代数 r，而不是只验证两个独立随机向量长度相同。

## 3. 三方视图和 input-independent 边界

### P0

P0 可以知道：

- 自己的 input share；
- 自己的局部 permutation p0；
- 自己的 r0 和 GRank mask share；
- P1 返回的 additive shares 和允许的 public list；
- public shape、phase、slot、width、count 和 material id。

P0 不应拥有完整 p1、完整 pi、完整 r、明文 score、priority-key、rank 或 original-index mapping。模型对象只保存 p0、r0 和对应 share，不保存 composed permutation 或 reconstructed r。

### P1

P1 与 P0 对称，只拥有 p1、r1、自己的 input/material share 和收到的 additive shares。P1 不拥有完整 p0、pi、r、明文 score、priority-key、rank 或 original-index mapping。

### P2

P2 的 candidate view 只有：

- logical_n、padded_n、key_bits、rank_bits；
- session/fingerprint/material id；
- per-party package shape 和 one-shot metadata；
- offline/online boundary 和退出状态。

P2 不接收 input share，不选择 input-dependent permutation，不持有完整 p0、p1、pi、r、score、priority-key 或 rank。模型将局部 permutation 和 r share 视为 P0/P1 的 party-local offline material；未来真实 P2 preprocessing 若需生成相关材料，必须保持同样的 P2 view，否则 Gate B 仍应阻塞。

### 输入无关性

所有 p0、p1、r0、r1 和 GRank mask share 在测试输入 vector 构造前完成；P2 view 只包含 shape/handle。模型不接受 score、priority-key、rank 或 permutation execution result 作为 P2 输入。

## 4. Same-permutation 证明义务

对任意向量 v：

Perm(p1, Perm(p0, v))[s] = v[p0[p1[s]]] = Perm(pi, v)[s]

对 priority-key x，模型输出：

public_y[s] = Perm(pi, x)[s] + r[s]

对 secret payload，模型分别分享 Perm(pi, payload)，再逐槽重构。对 GRank material，模型只分享同一 r 的两个局部 shares，并检查 public_y 减去重构后的 shuffled key 后与 GRank correlation 完全一致。

因此本模型覆盖：

- public list 与 secret payload 的复合 permutation 一致；
- p0/p1 的应用顺序一致；
- r 的 slot 对齐独立于单方完整 permutation；
- dummy 不进入 logical rank；
- GRank/FSS 使用的 correlation 与 public list 中的 r 是同一对象。

模型不证明真实网络协议中的密码学模拟安全，也不证明论文 full version 未公开的具体消息实现。

## 5. Causal transcript

候选 core 的轮数按 causal barrier 计数。双向消息可在同一 round 同步发送，但任何依赖对方消息的计算只能进入下一 round。

| Round | Sender | Receiver | Message type/shape | 依赖 | 新 barrier 与可见信息 | Offline 可预生成 |
| --- | --- | --- | --- | --- | --- | --- |
| O0 | P2 | P0 | candidate package shape，constant-size metadata | session、fingerprint、logical/padded、key/rank bits | P0 得到自身 package shape；没有 input 或完整对象 | 是，online 前完成 |
| O0 | P2 | P1 | candidate package shape，constant-size metadata | 同上 | P1 得到自身 package shape；没有 input 或完整对象 | 是，online 前完成 |
| R1 | P0 | P1 | forward local-share bundle，padded_n 个 w-bit words | P0 input share、p0、r0、correlation handle | P1 得到一份 additive forward share，不得到 p0 或 r | 否 |
| R1 | P1 | P0 | forward local-share bundle，padded_n 个 w-bit words | P1 input share、p1、r1、correlation handle | P0 得到一份 additive forward share，不得到 p1 或 r | 否 |
| R2 | P0 | P1 | final public-mask share + shuffled payload share，分别为 padded_n 个 w-bit words | R1 state、local permutation、local r share | 双方合并 public-mask shares 后得到 public_y；payload 仍为 shares | 否 |
| R2 | P1 | P0 | final public-mask share + shuffled payload share，分别为 padded_n 个 w-bit words | R1 state、local permutation、local r share | 双方合并 public-mask shares 后得到 public_y；payload 仍为 shares | 否 |
| R3 | P0 | P1 | GRank rank-share bundle，padded_n 个 rank_bits words | public_y、local GRank material、R2 state | P1 得到 rank share；不打开完整 rank 或 r | 否 |
| R3 | P1 | P0 | GRank rank-share bundle，padded_n 个 rank_bits words | public_y、local GRank material、R2 state | P0 得到 rank share；不打开完整 rank 或 r | 否 |

O0 不计入 online rounds。R1、R2 是候选 shuffle/public-list 两轮；R3 是 GRank/rank 核心一轮。因此 priority-key candidate core 为 3 online rounds，没有隐藏的第四个 masked-key barrier。R3 的 rank-share output 仍不是 secure runtime 的明文 rank。

### Priority-key 路径

O0 + R1 + R2 + R3 = offline + 3 online core rounds。

输出回到 original order 的 mask adapter 仍需单独处理，不能从 core 轮数中删除。

### Raw-score 完整工程路径

| 阶段 | 轮数 |
| --- | ---: |
| carry adapter | 1 |
| sign adapter | 1 |
| candidate Protocol I core | 3 |
| reverse/output mask adapter | 2 |
| 合计 | 7 |

该 7 是 candidate project path 的工程目标，不是论文原生总轮数，也不是当前 8-round runtime 的新标签。

## 6. TEST_ONLY 模型的验收信号

模型必须覆盖：

1. public masked list 与 secret payload 使用相同复合 permutation；
2. 同一 r 与 GRank correlation 的 slot 绑定；
3. P0/P1 只有局部 r share，不能直接拥有完整 r；
4. permutation 复合方向；
5. logical_n/padded_n 边界；
6. duplicate score 的 stable priority-key 语义；
7. 非 2 次幂 logical_n；
8. 错 permutation、错 r share、错 slot metadata、错误 shape、重复消费必须失败。

测试用例覆盖 logical_n 1、2、3、5、7、8；identity/reverse/固定种子随机 permutation；正、负、零、INT32_MIN、INT32_MAX；duplicate scores；多组非零 random masks；padding 有/无。

测试模型允许在 test target 内重构 oracle 值。任何重构代码、完整 r 或完整 permutation 都不得进入 secure library、正式 executable 或 M2/M3 runtime。

`run_candidate` 是测试目标内的 in-process algebra evaluator；它可以从 fixture 读取 clear vector 以计算 oracle，但 `PartyMaterial` 和 `P2View` 不保存这些 clear vector、完整 r 或完整 permutation。该 evaluator 不是 P0/P1/P2 的安全进程实现，也不把测试 harness 的重构视为 party view。

## 7. 三层 Gate 结论

### Gate A：候选设计/可行性门

| 条件 | 结果 | 证据 |
| --- | --- | --- |
| 数学对象和 ring/width | PASS | 本文件第 2 节和 candidate model |
| same-permutation algebra | PASS | 本文件第 4 节和 permutation assertions |
| correlated r/GRank material | PASS | 本文件第 2.4、3 节和 correlation assertions |
| P0/P1/P2 views | PASS | 本文件第 3 节和 view-shape assertions |
| input-independent P2/offline boundary | PASS | 本文件第 3 节和 model construction order |
| complete causal transcript | PASS | 本文件第 5 节 |
| no hidden online barrier | PASS | R1/R2/R3 分离且只有 3 个 online barriers |
| TEST_ONLY key invariants | PASS | candidate model CTest |

Gate A：**PASS，结论 FEASIBILITY_GO**。

### Gate B：secure candidate implementation

本阶段不执行。仍需独立进程 P2/P0/P1、真实 offline/online 边界、primitive conformance、oracle differential、independent-process E2E、secure path 无明文重构、无在线 Dealer、无文件同步。

### Gate C：论文精确标签

本阶段不执行。仍需完整论文依据、输入/输出/轮数/泄露/角色/预处理匹配、fresh build、完整测试和独立安全审查。

## 8. 明确禁止的声明

本阶段即使测试通过，也不得声明：

- 已实现 Agarwal Protocol I；
- 已实现论文精确 3-round core；
- 已完成 secure 3-round protocol；
- 已完成 Protocol I reproduction；
- 已证明论文泄露完全一致；
- 已通过 exact label gate。

允许的名称仅为 paper-compatible candidate、feasibility model、experimental candidate 和 TEST_ONLY algebra model。
