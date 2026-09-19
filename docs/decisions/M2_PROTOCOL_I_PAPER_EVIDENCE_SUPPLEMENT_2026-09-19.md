# M2 Protocol I 论文证据与实验复现验收标准

日期：2026-09-19

适用代码基准：

```text
30df4f09836a4ff38c83e87e04e29048f405022c
```

本次只更新证据边界、复现目标和验收标准，不修改现有协议实现。

前置决策：

```text
docs/decisions/M2_PROTOCOL_I_STAGE3O_DECISION_2026-09-15.md
```

## 1. 当前目标

M2 的目标调整为：

```text
Protocol I paper-aligned experimental reproduction
```

验收重点为：

1. 功能结果正确；
2. 核心协议保持三个在线 causal rounds；
3. Dealer 仅参与输入无关的离线预处理，在线阶段静默；
4. 在线通信量及其随 `n`、输入位宽和 payload 位宽的增长数量级与 Theorem 4.1 一致；
5. 项目新增的 raw-score、index、rank-direction 和 mask adapter 单独记录；
6. 实验环境、revision、输入、随机种子、原始日志和计数可复现。

本阶段不再把以下材料作为实验复现的前置阻塞项：

- 作者 full version；
- Theorem 4.1 的完整证明；
- 作者原始逐消息 frame 格式；
- secure shuffle 的作者具体代码；
- 与作者实现逐字节一致的 preprocessing package；
- 作者内部 benchmark 脚本。

缺少这些材料意味着项目不能宣称：

```text
author-implementation-exact
message-level paper-exact
material-level paper-exact
```

但不阻止项目完成具有正确功能、相同在线轮数和相同通信数量级的独立实验复现。

## 2. 证据分类

继续使用以下证据分类：

- A — `TARGET_PAPER`：Agarwal CCS 2024 目标论文直接定义或明确陈述。
- B — `LOCAL_REFERENCE`：`Agarwal_TopK/`、`ADSMPC/` 等本地参考工程的实际行为。
- C — `PROJECT_EXTENSION`：本项目增加的 ABI、adapter、mask 输出、metrics 或工程约定。
- D — `UNVERIFIED`：尚未得到目标论文直接支持或尚未完成实验验证的假设。

其他 FSS 论文单列为：

- `SUPPORTING_LITERATURE`：用于解释通用 FSS preprocessing、Gen/Eval 和 masked-value
  执行模型的正式文献。

SIGMA 是 `SUPPORTING_LITERATURE`，不是 B 类本地参考，也不能替代 Agarwal Protocol I
的具体 shuffle transcript。

## 3. 已核验来源

| 来源 | 身份与核验 | 用途 |
| --- | --- | --- |
| Agarwal 等，*Secure Sorting and Selection via Function Secret Sharing*，CCS 2024 会议版 | `Papers/Agarwal 等 - 2024 - Secure Sorting and Selection .pdf`；SHA256 `18faf63eaa7923eef715a6eb9d5d526fe04dcb69700b133c3e94de935f68c01c`；15 页，论文页 3023–3037 | Protocol I 的 A 类目标论文来源 |
| Gupta 等，*SIGMA: Secure GPT Inference with Function Secret Sharing* | SHA256 `3a2989f1da36bda9e2e9b4b6a2d482e33aeff15b223e18a2d1aa5ebcbf1fea0d` | 通用 FSS preprocessing、Gen/Eval 和 masked-value 模型旁证 |
| `Agarwal_TopK/`、`ADSMPC/` 等 | 仅记录固定 revision 下的实际行为 | B 类本地参考；不能反推为论文主张 |

Agarwal 文件路径必须与 `docs/PAPERS.sha256` 完全一致：

```text
Papers/Agarwal 等 - 2024 - Secure Sorting and Selection .pdf
```

## 4. Agarwal 论文已经直接回答的内容

### 4.1 协议角色

Agarwal §1、§1.2 和 §2.1 明确给出 `(2+1)` 模型：

- `P0`、`P1` 是在线双方；
- `P2` 是 offline party/dealer；
- Dealer 在离线阶段发送 correlated randomness；
- Dealer 在线阶段保持静默；
- 输入由 `P0`、`P1` 分享；
- 安全模型为单个静态半诚实腐化。

因此：

```text
Dealer online silence = A / VERIFIED
```

这不是仅由 FSS 惯例推出的结论，也不需要依赖 SIGMA 才成立。

### 4.2 secure shuffle 的功能接口

Agarwal §2.4 明确给出 secure shuffle 的高层功能：

- 输入是 secret-shared list `x`；
- 输出包括 secret-shared shuffled list；
- 在线双方还得到公开的 masked shuffled list `π(x)+r`；
- `π` 是一个 permutation；
- `r=(r1,...,rn)` 是随机私有 mask；
- `r` 对任一单方均未知；
- 后续 FSS gate 使用 `r` 作为秘密参数；
- 公开的 `π(x)+r` 可直接作为 FSS gate 输入。

因此，正确表述为：

```text
r 是与 secure shuffle 关联的随机私有 mask；
公开值是 π(x)+r；
r 本身不是公开输出。
```

论文同时明确说明，secure-shuffle functionality 的正式描述和 concrete protocol
instantiation 位于 full version。因此，会议版没有给出 `r` 的具体 share layout 或消息格式。

### 4.3 rank 与 stable tie

Agarwal §3 明确定义：

```text
Rank(x_i)
= 小于 x_i 的元素数量
+ 位于 i 之前且等于 x_i 的元素数量
```

因此：

```text
minimum element rank = 0
maximum element rank = n - 1
```

相等元素中，原序列位置更早者取得更小的论文 rank。

这是 A 类直接证据。

### 4.4 key 与 payload 的关联

Figure 1 的 `Fsort` ideal functionality 明确要求：

```text
使用 x 作为 keys；
使用 y 作为 corresponding payloads；
对 x、y 保持对应关系地排序；
输出排序后的 key shares 和 payload shares。
```

Figure 2 的 `Fselect` 则输出 rank-`k` 元素及其 corresponding payload 的 shares。

§4 进一步说明，ranking 只作用于 keys，payload 在 routing 阶段随对应 key 移动。

因此：

```text
key/payload association = A / VERIFIED
```

会议版没有给出 concrete shuffle material 如何实现该绑定，但实验复现可以选择任一满足
该 ideal functionality 和安全模型的 secure shuffle 实现。

### 4.5 Protocol I 的轮数和通信量

Table 1、§4.1 和 Theorem 4.1 直接支持：

```text
Protocol I:
routing = Shuffle
ranking = all-pairs Compare-Aggregate
parties = 2+1
online rounds = 3
```

对于 `G = Z_L'`、`L' >= 2L`、`ell' = ceil(log2 L')` 和 `p` bit payload，
Theorem 4.1 给出的总在线通信量为：

```text
4n(ell' + p) + 2n ceil(log2 n) bits
```

在线计算量为：

```text
2 * C(n,2) * DCF.Eval[G, Z_n]
```

Dealer 向两个在线方发送的离线通信量为：

```text
6n(ell' + p)
+ 4n ceil(log2 n)
+ 2 * C(n,2)
  * (DCF.KeySize[G, Z_n] + ceil(log2 n))
bits
```

对于实验复现，核心在线通信数量级应为：

```text
O(n(ell' + p + log n))
```

核心在线计算和 DCF preprocessing 主项随全对比较数量增长：

```text
O(n^2)
```

实现可以存在序列化、frame header、连接初始化和 metrics 元数据等固定开销，但必须将这些
工程开销与 protocol payload 分开报告。

## 5. SIGMA 能补充的 FSS 标准模型

SIGMA §2.2–§2.4 给出标准的 2PC-with-preprocessing FSS 示例：

### 离线阶段

- preprocessing 与在线输入独立；
- Dealer 或其他 preprocessing mechanism 生成 correlated randomness；
- 为 wire 采样随机 mask；
- `Gen` 根据 offset function 生成两份 FSS keys；
- `P0`、`P1` 分别取得自己的 key 和所需 preprocessing material。

### 在线阶段

- `P0`、`P1` 使用 masked input 和各自 FSS key；
- 两方分别执行 `Eval`；
- Eval 输出组合为相应函数值的 share 或 masked-output share；
- 协议需要时重构 masked intermediate；
- output owner 最终使用相应 mask 得到输出；
- Dealer 不执行 online Eval。

这可支持本项目采用以下通用结构：

```text
offline:
Dealer/Gen → material_0, material_1

online:
P0/P1 → masked input
P0/P1 → local Eval
P0/P1 → reconstruct required masked intermediates
P0/P1 → output shares
```

但 SIGMA 不提供 Agarwal secure shuffle 的 `π/r` concrete transcript，不能用于声称复现了
作者的具体 shuffle 实例化。

## 6. 原十个问题的处理结论

| 原问题 | 处理结论 | 是否阻塞实验复现 |
| --- | --- | --- |
| full version、附录和三轮 transcript | 会议版没有完整 transcript；Theorem 4.1 proof 和 shuffle 具体实例化被放到 full version | 否 |
| `r` 的具体生成和分发 | Agarwal 给出功能边界；具体 material 未给出。可采用符合该功能的独立实现 | 否 |
| key/payload same permutation | Fsort ideal functionality 已明确要求保持 correspondence；具体 material 可由所选 shuffle 实现负责 | 否 |
| original index 和逆路由 | Original index 可作为项目 payload 实例化；逆路由和 original-order mask 属于 C 类 adapter | 否 |
| stable tie 和 priority-rank | 论文 rank 定义已明确；项目必须单独定义 descending Top-K 的方向及 tie 映射 | 否 |
| padding/dummy | 会议版未定义 Protocol I padding/dummy；如果实现需要，登记为 C 类工程约定 | 否 |
| key/payload shares 到 mask | 不属于 Fsort/Fselect 原生输出；属于 C 类 adapter | 否 |
| mask adapter 是否属于论文计数 | 不属于 Theorem 4.1 已证明的原生输出边界；必须单独计量 | 否 |
| Theorem 4.1 的准确边界 | 覆盖 `Fsort`、3 online rounds、payload 和列出的计算/通信成本；不自动覆盖项目 adapter | 否 |
| proof、版本和稳定链接 | 会议版 DOI 和公开出版记录可稳定引用；完整 proof 仍指向尚未纳入仓库的 full version | 否 |

因此，原“给作者或导师的十个问题”不再是 M2 实验复现的强制前置条件。

## 7. rank 与 Top-K 项目语义

论文 rank 是升序 rank：

```text
最小值 → 0
最大值 → n - 1
```

如果只考虑互异元素，Top-K largest 对应论文 rank：

```text
n-K, ..., n-1
```

但需要特别注意相等元素。

论文 stable rank 对相等值采用：

```text
原位置更早者 → 更小的论文 rank
```

如果直接用：

```text
priority_rank = n - 1 - paper_rank
```

会同时反转数值顺序和相等元素的先后顺序，不能自动保证项目要求的：

```text
score 相同 → original index 更小者优先
```

因此，本项目必须显式冻结 descending Top-K 的 tie 语义。允许的工程方式包括先构造已经编码
项目优先级的 composite priority key，再调用排序/选择核心；但其编码、位宽和输入转换属于
C 类 adapter，不属于 Agarwal 原生 rank 定义。

验收测试必须至少覆盖：

- 所有分数互异；
- Top-K 边界处存在两个相等分数；
- 多个元素全部同分；
- `K=1`；
- `K=n`；
- 同分时 original index 的确定性选择。

## 8. original index 和 original-order mask

Agarwal 的 `Fsort` 支持 corresponding payload；Theorem 4.1 也明确计入 `p` bit payload。

因此，将 original index 编码为 payload 是与论文接口兼容的项目实例化。不过论文没有直接
定义以下项目输出：

```text
original-order XOR Top-K mask
```

下列步骤均为 C 类 adapter：

1. 将 original index 编码为 payload；
2. 取得排序或选择后的 index shares；
3. 判断哪些 index 属于 Top-K；
4. 将结果路由回原始位置；
5. 产生 original-order mask；
6. 转换成项目要求的 XOR shares。

实验报告必须把这些步骤与 Protocol I paper core 分开：

```text
paper core:
shuffle + ranking + routing

project adapter:
raw-score conversion
priority-key construction
index payload
rank-direction/tie mapping
reverse routing
original-order mask
XOR-share conversion
```

如果 adapter 增加在线通信或 causal rounds，必须单独报告，不能合并后仍将整个系统称为
“3-round Protocol I”。

## 9. padding 和 dummy

当前 Agarwal 会议版中没有给出 Protocol I 的 padding/dummy 规则。

Protocol I 的公开参数直接包含元素数量 `n`，Theorem 4.1 的成本也直接以 `n` 表示。因此，
不能在没有额外来源的情况下宣称论文要求将输入 padding 到二次幂或固定容量。

如果本项目的 shuffle、network frame 或 batch implementation 需要 padding/dummy，则应将其
登记为 C 类工程约定，并记录：

- 真实元素数；
- padding 后元素数；
- dummy key 和 payload 的语义；
- dummy 是否可能进入 Top-K；
- 过滤 dummy 的位置；
- padding 对通信量和运行时间的影响；
- 是否泄露真实 `n`。

实验通信量应同时报告 logical `n` 和 physical padded `n`，避免把 padding 开销误记为论文
公式偏差。

## 10. 实验复现验收门

M2 可在没有 full version 的情况下进入实现和实验。最终验收分为以下五项。

### 10.1 功能正确性

必须通过：

```text
conformance
→ cleartext oracle differential
→ independent-process E2E
```

至少检查：

- 排序结果；
- selection 或 Top-K 结果；
- payload/index correspondence；
- stable tie；
- original-order mask；
- share reconstruction；
- 边界输入和重复值。

### 10.2 在线轮数

Protocol I paper core 必须为：

```text
3 causal online rounds
```

连接建立、握手、日志同步等 transport event 不得伪装成协议轮次，也不得忽略真正存在数据依赖
的额外通信。

项目 adapter 的轮数单独报告：

```text
paper_core_online_rounds
adapter_online_rounds
total_online_rounds
```

### 10.3 Dealer 边界

必须满足：

- preprocessing 与在线输入独立；
- Dealer 只在 offline phase 产生和分发 material；
- Dealer 不读取在线输入；
- Dealer 不接收在线消息；
- Dealer 不参与 online Eval；
- 在线阶段仅由 `P0`、`P1` 执行。

如果实验使用预生成文件模拟 Dealer，应记录生成命令、随机种子策略、material identity 和每方
读取的文件。

### 10.4 通信量

至少分别记录：

```text
offline_bytes_total
online_bytes_p0_to_p1
online_bytes_p1_to_p0
online_bytes_total
paper_core_online_bytes
adapter_online_bytes
transport_overhead_bytes
```

Protocol I paper core 的理论目标为：

```text
4n(ell' + p) + 2n ceil(log2 n) bits
```

实验不强制逐 bit 完全相等，但必须满足：

1. 将固定 frame/header 开销单列；
2. 扣除 transport overhead 后，核心通信量与理论公式处于可解释的常数因子内；
3. 固定 `ell'`、`p` 改变 `n` 时，在线通信呈近线性增长；
4. 固定 `n` 改变 `ell'` 或 `p` 时，在线通信呈近线性增长；
5. 不出现无法解释的 `O(n^2)` 在线通信；
6. `O(n^2)` DCF 成本主要体现在离线 key material 和计算，而不是核心在线通信。

### 10.5 可复现性

每组实验必须保存：

- Git revision；
- 构建命令；
- 运行命令；
- 编译器和依赖版本；
- 操作系统和硬件；
- `n`、`K`、`L`、`L'`、`ell'`、`p`；
- 输入文件或随机生成方式；
- 随机种子；
- 重复次数；
- 原始 stdout/stderr；
- 原始 metrics；
- 理论通信量；
- 实测通信量；
- paper core 与 adapter 的拆分。

没有实际运行记录的字段继续写：

```text
NOT_MEASURED
```

## 11. 允许的命名

通过上述验收后，可以使用：

```text
Protocol I paper-aligned experimental reproduction
Agarwal Protocol I functionality-aligned implementation
Protocol I 3-round paper-core reproduction
```

在没有作者 full version 和 concrete transcript 的情况下，不使用：

```text
author-code reproduction
author-implementation-exact
message-level paper-exact
material-level paper-exact
byte-for-byte reproduction
```

项目输出为 original-order XOR Top-K mask 时，应在名称或报告中明确：

```text
paper core + project mask adapter
```

## 12. 后续执行顺序

1. 冻结 Protocol I paper core 与 C 类 adapter 的接口边界。
2. 实现或选用满足 §2.4 功能要求的 secure shuffle。
3. 保证 key 与 payload 经过同一逻辑 permutation。
4. 实现 all-pairs Compare-Aggregate ranking。
5. 将 paper core 控制在三个 causal online rounds。
6. 独立实现并计量 raw-score、index、tie、reverse-routing 和 mask adapter。
7. 运行 conformance、oracle differential 和独立进程 E2E。
8. 对多个 `n`、位宽和 payload 位宽采集通信量。
9. 比较实测通信与 Theorem 4.1 的公式和数量级。
10. 完成 Protocol I 通信核验与接口交接。
11. 进入 M5 Protocol III paper-exact/paper-aligned 实验复现。

## 13. 非阻塞的可选作者问题

如果以后能够联系作者，只需补充询问：

1. 是否存在公开 full version 或 Protocol I secure-shuffle 的正式 transcript；
2. Theorem 4.1 实验使用的 concrete shuffle 实例化、代码 revision 和 benchmark 配置。

这些问题用于提高论文精确度，不阻塞当前实验复现。

## 14. 当前结论

```text
Protocol I high-level construction:
VERIFIED / A

Dealer offline-only and online silence:
VERIFIED / A

public π(x)+r:
VERIFIED / A

r as private shuffle-associated mask:
VERIFIED / A

key/payload correspondence:
VERIFIED AT FUNCTIONALITY LEVEL / A

paper rank and stable rank:
VERIFIED / A

Protocol I online rounds:
3 / VERIFIED / A

Protocol I online communication:
4n(ell' + p) + 2n ceil(log2 n) bits / VERIFIED / A

concrete secure-shuffle transcript:
NOT AVAILABLE IN CONFERENCE VERSION / NON-BLOCKING

full Theorem 4.1 proof:
REFERRED TO FULL VERSION / NON-BLOCKING

original-index payload:
PROJECT INSTANTIATION / C

descending priority and tie mapping:
PROJECT ADAPTER / C

original-order XOR Top-K mask:
PROJECT ADAPTER / C

padding/dummy:
NOT DEFINED BY CONFERENCE VERSION;
PROJECT EXTENSION IF USED / C

experimental reproduction:
IMPLEMENTATION_GO

message/material-level paper-exact claim:
NO-GO UNTIL ADDITIONAL EVIDENCE
```
