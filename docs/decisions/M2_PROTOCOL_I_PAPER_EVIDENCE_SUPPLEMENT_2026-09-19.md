# M2 Protocol I 论文证据补充与交接门

日期：2026-09-19

适用代码基准：

```text
30df4f09836a4ff38c83e87e04e29048f405022c
```

本次只更新文档。最终交接 revision 以本分支提交为准。

前置决策：

```text
docs/decisions/M2_PROTOCOL_I_STAGE3O_DECISION_2026-09-15.md
```

本项目当前门状态：

```text
DESIGN_BLOCKED
IMPLEMENTATION_NO_GO
M2_PAPER_EXACT_BLOCKED / NOT_VERIFIED
```

其中 `IMPLEMENTATION_NO_GO` 仅指本项目的 paper-exact 验收门，不表示 Protocol I 在一般意义上
无法依据会议版的高层 functionality 实现。

## 1. 范围与不变项

本补充整理：

- 已定位的目标论文证据；
- supporting literature 的限定用途；
- 当前资料缺口；
- paper rank 与 project priority-rank 的方向映射；
- paper-native 输出与项目 adapter 的边界；
- 下一位执行者的取证及验收顺序。

本补充不修改任何 `VFSS/` 源码、既有实现标签、在线轮数、测试记录或泄露结论。它不是实现
授权，也不将项目扩展重命名为论文原生协议。

## 2. 证据分类

证据类别保持为：

- A — `TARGET_PAPER`：Agarwal CCS 2024 目标论文直接定义或明确陈述。
- B — `LOCAL_REFERENCE`：`Agarwal_TopK/`、`ADSMPC/` 等本地参考工程的实际行为。
- C — `PROJECT_EXTENSION`：本项目增加的 ABI、adapter、mask 输出、metrics 或工程约定。
- D — `UNVERIFIED`：目标论文中尚未取得直接证据、尚未完成审计的设想或映射。

其他论文不并入 A/B/C/D，而是单列：

- `SUPPORTING_LITERATURE`：SIGMA 等用于解释通用模型、原语或背景的正式文献。

“FSS 通常如此”“本地参考实现如此”或“另一篇论文如此”均不能补写为 Agarwal Protocol I 的
A 类逐轮 transcript。

## 3. 本次已核验来源

| 来源 | 身份与核验 | 可用范围 |
| --- | --- | --- |
| Agarwal 等，*Secure Sorting and Selection via Function Secret Sharing*，CCS 2024 会议版 | `Papers/Agarwal 等 - 2024 - Secure Sorting and Selection .pdf`；SHA256 `18faf63eaa7923eef715a6eb9d5d526fe04dcb69700b133c3e94de935f68c01c`；15 页，论文页 3023–3037 | Protocol I 的 A 类目标论文来源 |
| Gupta 等，*SIGMA: Secure GPT Inference with Function Secret Sharing* | SHA256 `3a2989f1da36bda9e2e9b4b6a2d482e33aeff15b223e18a2d1aa5ebcbf1fea0d` | `SUPPORTING_LITERATURE`；通用 FSS preprocessing/Gen/Eval 背景，不是 Protocol I transcript |
| `Agarwal_TopK/`、`ADSMPC/` 等本地参考 | 仅在固定来源、revision、许可证和入口后记录实际行为 | B 类本地参考；不能反推为目标论文主张 |

Agarwal 文件路径必须与 `docs/PAPERS.sha256` 保持完全一致，包括 `Selection` 与 `.pdf`
之间的空格：

```text
Papers/Agarwal 等 - 2024 - Secure Sorting and Selection .pdf
```

## 4. 九项证据矩阵

| 编号 | 问题 | 当前结论 | 分类与边界 |
| ---: | --- | --- | --- |
| 1 | Protocol I 是否为 `(2+1)`、secure shuffle + CmpAgg、3 online rounds | 会议版明确给出 Protocol I 的高层组合和三轮在线声明；会议版没有给出足以完成 message-level 审计的逐轮 sender/receiver/frame/causal transcript | 高层结构与三轮声明为 A；concrete transcript 为 D |
| 2 | `π(x)+r` 与 `r` 分别是什么 | secure shuffle 的公开 masked shuffled list 是 `π(x)+r`；`r` 是与 shuffle 关联的私有随机 mask，对任一单方均未知，并作为后续 FSS gate 的秘密参数 | 高层功能为 A；`r` 的具体生成、分发、份额格式和消费点仍为 D |
| 3 | P0/P1/P2 的角色及 Dealer 是否在线参与 | `P0`、`P1` 是在线双方，`P2` 是 offline party/dealer；Dealer 离线发送 correlated randomness，在线保持静默 | Dealer online silence 是 A 类直接证据 |
| 4 | key、payload 与 original index 是否使用同一 permutation | 论文功能定义明确要求 key 与 corresponding payload 保持关联；会议版未给出 concrete shuffle material 如何实现该绑定；original index 是项目新增 payload/adapter | key/payload 对应关系为 A；concrete material 为 D；original index 为 C |
| 5 | 论文原生输出是否为 original-order Top-K mask | Fsort/Fselect 原生输出是 key shares 与对应 payload shares；不是 original-order XOR Top-K mask | 原生输出为 A；项目 mask 为 C |
| 6 | 论文三轮是否覆盖项目 adapter | Theorem 4.1 的三轮声明不能直接覆盖 raw-score 转换、rank 方向转换、original-index 绑定、reverse routing 和 mask 生成 | 论文核心三轮为 A；adapter 为 C；adapter causal rounds 必须独立审计 |
| 7 | stable rank 与项目 priority-rank 是否同方向 | 论文 rank 为 minimum `0`、maximum `n-1`；相等元素中原序列更早者 rank 更小。项目语义为 Top-K largest、最高优先级 `0`，同分时 original index 更小者优先 | 论文 rank/stable tie 为 A；方向转换为 C，且必须显式实现和测试 |
| 8 | preprocessing 与 online Dealer 的边界 | Agarwal 的 `(2+1)` 模型直接支持 Dealer 离线发送 correlated randomness、在线静默；通用的 Gen/Eval 背景可由 SIGMA 辅助解释 | Agarwal online silence 为 A；SIGMA 仅为 supporting literature |
| 9 | 是否已取得 full version、proof 或逐轮 transcript | 本仓库和本次已记录资料中尚未取得可审计的 full version、proof 或逐轮 transcript；会议版多处将证明指向 full version | `PAPER_SOURCE_INCOMPLETE`；这是检索状态，不表示 full version 不存在 |

## 5. Protocol I 的已确认边界

### 5.1 三轮声明与逐轮 transcript

Agarwal §4.1 和 Theorem 4.1 支持 Protocol I 的高层三轮在线声明。该声明不等同于：

- 已知三个具体 frame；
- 已知每轮 sender 与 receiver；
- 已知每条消息的字段；
- 已知每方在每轮的完整 view；
- 已知 `r` 的 material layout；
- 已知本项目 adapter 可免费包含在三轮内。

因此：

```text
3 online rounds: A / VERIFIED AT HIGH LEVEL
message-level transcript: D / NOT VERIFIED
```

### 5.2 `r` 与 `π(x)+r`

正确表述为：

```text
r 是与 secure shuffle 关联的私有随机 mask 向量；
公开输出之一是 π(x)+r；
r 本身不是公开输出；
r 对任一单方均未知；
r 作为后续 FSS gate 的秘密参数使用。
```

不得写成“`r` 是公开输出”或“公开了 `r`”。

仍需取得或确认：

- `r` 的生成者；
- P0/P1 分别取得什么 material；
- `r` 是以何种份额或 key material 表示；
- `r` 与 `π` 如何组合；
- 哪一个后续 gate 在何时消费相应秘密参数。

### 5.3 Dealer online silence

Dealer online silence 是 Agarwal 目标论文的 A 类直接证据。

目标论文对 `(2+1)` 模型的描述明确限定 Dealer 在 offline phase 发送 correlated
randomness，随后在 online phase 保持静默；§2.1 也将 `P0`、`P1` 定义为在线 parties，
将 `P2` 定义为 offline party/dealer。

因此，本文不再使用 SIGMA 来“推导”Dealer online silence。SIGMA 仅用于补充说明通用 FSS
preprocessing `Gen` 与 online `Eval` 的背景结构。

### 5.4 same-permutation 边界

论文的 Fsort functionality 已经要求：

```text
使用 x 作为 keys；
y 是与 keys 对应的 payloads；
输出排序后的 key shares 和对应 payload shares。
```

所以 key 与 payload 的对应关系是 A 类要求，不应标成完全未知。

当前未知的是 concrete secure-shuffle material 如何确保 key 与 payload 使用同一 secret
permutation。

Original index 并不是自动获得的目标论文输出。本项目如果把 original index 作为额外 payload
绑定到 key，属于 C 类 adapter，必须单独定义和审计：

- index 的编码；
- index 与 key/payload 的同一 permutation；
- padding/dummy index；
- reverse routing；
- 泄露；
- causal communication rounds。

### 5.5 paper-native 输出与项目 mask

Agarwal 的 Fsort/Fselect functionality 输出 key shares 与 corresponding payload shares。
Fselect 返回所选元素及相应 payload 的 shares，而不是整条原始顺序 mask。

项目目标：

```text
original-order XOR Top-K mask
```

属于 C 类 adapter。不得将 mask 输出反写为论文原生功能，也不得把 adapter 隐式计入论文的
三轮声明。

### 5.6 rank 方向映射

论文 rank：

```text
minimum element → rank 0
maximum element → rank n - 1
```

相等元素中，原序列更早者 rank 更小。

项目 priority-rank：

```text
maximum / highest priority → rank 0
Top-K largest
score 相同 → original index 更小者优先
```

因此两种 rank 的数值方向相反。项目必须定义显式转换，例如由已冻结的输入规模和排名域决定
方向映射；具体公式应在实现设计中登记并由 oracle 测试确认。

本补充只冻结“必须显式转换”这一边界，不在缺少最终接口设计时新增具体实现公式。

## 6. SIGMA 对照的正确用法

SIGMA §2.2–§2.4 对通用 FSS 预处理模型的说明可支持以下背景理解：

- correlated material 可以在在线输入处理前生成；
- `Gen` 生成后续计算使用的 material；
- online `Eval` 由两方使用各自 material 执行。

SIGMA 属于 `SUPPORTING_LITERATURE`，不是 B 类 `LOCAL_REFERENCE`，也不是 Agarwal
Protocol I 的 A 类来源。

SIGMA 不能证明：

- Protocol I 三个 causal round 的具体内容；
- Protocol I 的 sender/receiver；
- `π/r` material 的具体格式；
- Protocol I party view；
- concrete same-permutation 实例化；
- inverse routing；
- original-order mask adapter。

## 7. 对现有 M2/M3 的影响

下列标签、实现和历史测试状态均不因本补充改变：

| 路径 | 固定标签 | 当前身份 |
| --- | --- | --- |
| M2 formal baseline | `m2_protocol_i_raw_score_input_modular_8round_mask_output` | C 类工程基线 |
| M2 candidate | `m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate` | candidate；不是完整 mask |
| M2 priority/raw Route A | `m2_protocol_i_dealer_preprocessed_rank_reveal_6round_mask_output` / `m2_protocol_i_raw_score_dealer_preprocessed_rank_reveal_8round_mask_output` | C 类扩展；包含额外泄露/adapter |
| M3 modular/raw | `agarwal_protocol_iii_modular_3round` / `moe_topk_protocol_iii_raw_score_modular_5round` | 与 Route A 隔离的工程基线/扩展 |

阶段三N的 Ubuntu/EMP 测试记录仍是祖先代码的历史证据，详见：

```text
docs/reproduction/M2_M3_STAGE3N_FINAL_CLOSEOUT_2026-09-15.md
```

本次没有运行 C++、CTest 或性能测试：

```text
runtime verification: NOT_REMEASURED
performance fields: NOT_MEASURED
```

## 8. 本项目 paper-exact 验收门

会议版已经给出：

- ideal functionality；
- secure shuffle 的高层接口；
- public `π(x)+r`；
- FSS gates；
- rank 定义；
- 三轮总结；
- 成本公式。

这些内容足以支持高层 functionality 研究，也可能支持独立构造
Protocol-I-like implementation。

但是，本项目的 paper-exact 验收要求进一步覆盖：

- message-level；
- material-level；
- party-view-level；
- round-exact；
- paper core 与 project adapter 的可审计分离。

因此：

```text
Under this project's paper-exact acceptance criterion,
implementation remains NO-GO until the missing
transcript/material evidence is obtained.
```

`IMPLEMENTATION_NO_GO` 不能被解释为论文证明了 Protocol I 无法实现，也不能被推广为其他
项目必须采用的门槛。

## 9. 唯一允许的后续顺序

1. 获取 full version、作者提供的算法或附录，或可校验的一手逐轮 transcript。
2. 对新增资料记录来源、URL、获取日期、SHA256、页码、算法号或定理号。
3. 更新 A/B/C/D evidence table，逐轮列出：
   - sender → receiver；
   - 消息字段；
   - causal dependency；
   - 公开值；
   - 每方 view；
   - material producer、recipient 和 consumption；
   - `π/r` 生命周期；
   - key/payload concrete same-permutation；
   - padding/dummy。
4. 单独设计 paper-native key/payload shares 到项目 original-order mask 的 C 类 adapter。
5. 显式定义论文 rank 到项目 priority-rank 的方向映射。
6. 分别审计 paper core 与 adapter 的轮数、通信量和泄露。
7. 仅在上述内容可审计后，新建独立 package、frame 和 material identity。
8. 验证顺序固定为：
   - conformance；
   - oracle differential；
   - 独立进程 E2E；
   - leakage/round audit；
   - 通信核验；
   - 接口交接。

在此之前，不得使用以下内容填补 paper transcript 空白：

- `RA6M` 或 `RA8M`；
- M3 DPF material；
- 测试层明文重构；
- 本地参考工程行为；
- SIGMA 的通用 Gen/Eval 模型；
- 未经来源核验的 FSS 惯例。

## 10. 给作者或导师的最小问题集

请索取或确认：

1. Protocol I 的 full version、附录或逐轮 transcript，包括三轮的 sender 和 receiver。
2. Secure shuffle 中 `r` 的生成者、P0/P1 各自获得的 material、`r` 与 `π` 的组合及消费点。
3. Key 和 corresponding payload 如何通过 concrete material 使用同一 secret permutation。
4. 如果加入 original index，正确的组合与逆路由方式是什么；若论文未定义，应确认它属于项目
   adapter。
5. Stable tie 的完整编码，以及论文 rank 到 Top-K-largest priority-rank 的推荐映射。
6. Padding/dummy 是否由 Protocol I 定义，以及其对正确性、轮数和泄露的影响。
7. Fsort/Fselect 原生 key/payload 输出转成 original-order Top-K mask 是否属于论文计数。
8. 如果不属于论文计数，推荐的额外协议、轮数和泄露口径是什么。
9. Theorem 4.1 三轮声明所覆盖的准确协议边界。
10. 可公开引用的 proof、算法编号、版本和稳定下载位置。

## 11. 交接门结论

当前结论保持：

```text
Protocol I high-level construction: VERIFIED
3-online-round claim: VERIFIED AT HIGH LEVEL
Dealer online silence: VERIFIED / A
public π(x)+r: VERIFIED / A
r as private shuffle-associated mask: VERIFIED / A
paper-native key/payload output: VERIFIED / A
stable rank: VERIFIED / A
paper-rank → project-priority-rank mapping: REQUIRED / C
message-level transcript: NOT VERIFIED / D
concrete r material: NOT VERIFIED / D
concrete key/payload same-permutation material: NOT VERIFIED / D
original-index and original-order mask adapter: PROJECT EXTENSION / C
full version/proof/transcript: PAPER_SOURCE_INCOMPLETE
project paper-exact implementation gate: IMPLEMENTATION_NO_GO
```

该门解除后，执行顺序为：

```text
Protocol I paper-exact
→ Protocol I 通信核验
→ Protocol I 接口交接
→ M5 Protocol III paper-exact
```

不得跳过 Protocol I 通信核验和接口交接，也不得恢复已经取消的 M4 CipherGPT 路线。
