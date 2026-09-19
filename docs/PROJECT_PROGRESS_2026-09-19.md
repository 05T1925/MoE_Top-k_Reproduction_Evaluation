# 项目全局进度与后续阶段

日期：2026-09-19

状态：规划与审计口径；不是代码覆盖率、性能结果或严格论文一致性声明。

## 总体结论

当前项目已经完成公共正确性底座、Protocol I 的 C 类工程基线，以及与 Protocol III
相关的模块化工程基线。

根据当前复现目标，M2 不再以取得作者 full version、逐消息 transcript 或作者具体
secure-shuffle 实例化作为进入实现的强制前置条件。M2 当前允许进入：

```text
Protocol I paper-aligned experimental reproduction
```

该阶段的验收重点是：

- 功能结果正确；
- Protocol I paper core 保持三个 causal online rounds；
- Dealer 仅参与输入无关的离线预处理，在线阶段静默；
- 核心在线通信量及其增长数量级与 Theorem 4.1 一致；
- 项目新增 adapter 与 paper core 分开记录和计量；
- 实验环境、revision、输入、随机种子、日志和原始计数可复现。

由于当前路线已经取消 M4 CipherGPT，旧的约 55% 估计及其权重体系不再有效。本文不再提供
单一完成百分比。后续进度以各阶段可审计的退出条件为准，不以提交数、代码行数或未经重新
定义的加权百分比衡量。

当前冻结执行路线为：

```text
M2 Protocol I 论文对齐实验复现
→ Protocol I 通信核验
→ Protocol I 接口交接
→ M5 Protocol III 论文对齐实验复现
→ Protocol III 通信核验
→ Protocol III 接口交接
→ M6A AAV86
→ M6B BB90+DCF
→ M7 六方案统一报告
```

M4 CipherGPT 已取消，不属于当前执行路线，也不应重新计入项目进度或后续优先级。

## 阶段总览

| 阶段 | 状态 | 已完成的可审计交付物 | 未完成的退出条件 |
| --- | --- | --- | --- |
| M0：证据、命名与冻结基线 | 完成 | 引用边界、PDF/参考资料清单、`VFSS-baseline` 冻结 | 无；后续仅做维护性复检 |
| M1/M1.1：公共正确性底座 | 完成 | Q20.12 语义、stable tie、oracle、CmpAgg、metrics、DCF conformance、Ubuntu 验收 | 性能和 LAN/WAN 结果不属于该阶段的已测结论 |
| M2：Protocol I C 类工程基线 | 完成 | 8-round raw-score mask baseline、shuffle/transport/material、独立进程与 Ubuntu/EMP 记录 | 不得将其重新命名为论文原生 3-round 实现 |
| M2-reproduction：Protocol I 论文对齐实验复现 | `IMPLEMENTATION_GO` | 会议版的高层协议结构、Dealer 模型、stable rank、`π(x)+r`、原生输出、3-round 声明和通信公式已经核验 | 实现 3-round paper core；完成正确性、轮数、Dealer 边界和通信数量级验收 |
| Protocol I 通信核验 | 待 M2-reproduction 实现后执行 | 已冻结 Theorem 4.1 理论公式和项目 metrics 要求 | 分别记录 paper core、adapter 和 transport overhead；完成多组参数的增长趋势核验 |
| Protocol I 接口交接 | 未开始 | 已知项目目标输出为 original-order XOR Top-K mask | 冻结输入、输出、rank/tie 映射、index/payload 绑定、adapter、泄露和错误语义 |
| M3：Protocol III 模块化工程基线 | 完成 | 3-round priority-key 路径、5-round raw-score 路径、DPF routing、secure combine、三方 E2E | 这些实现不等于论文声明的 2-round Protocol III |
| M5：Protocol III 论文对齐实验复现 | 设计准备 | paper evidence、field contract 和预实现检查清单 | 满足域与非零 payload 前置条件；实现 2-round paper core；完成正确性、轮数、通信和 E2E 验收 |
| Protocol III 通信核验 | 未开始 | 可复用公共 metrics 和测试框架 | paper core 与 raw-score/index/mask adapter 分项计量 |
| Protocol III 接口交接 | 未开始 | 已有模块化接口经验 | 冻结可供 M6A、M6B 与统一报告使用的语义和测量接口 |
| M6A：AAV86 | 未开始 | 已识别自适应 exact-edge 预处理门 | 一手算法依据、offline-only 预处理、安全模型、runtime、差分测试和 E2E |
| M6B：BB90+DCF | 资料准备 | 已确定为 AAV86 之后的独立阶段 | 原始算法依据、目标顺序统计量、DCF 衔接、实现来源、差分测试和 E2E |
| M7：六方案统一报告 | 未开始 | 指标口径和比较矩阵已有基础 | 在同一环境实际采样，保存原始记录、固定版本并形成可比较报告 |

## 复现身份与证据边界

“工程基线完成”不等于“论文对齐实验复现完成”；“论文对齐实验复现完成”也不等于
“作者实现逐消息精确复现”。

当前使用以下三个层级：

### 1. 工程基线

现有 M2/M3 路径提供可运行、可测试的项目工程基线，但其轮数、输出和泄露边界不等于论文
原生协议。

### 2. 论文对齐实验复现

满足以下条件后，可以登记为论文对齐实验复现：

- 实现论文描述的高层 protocol core；
- 功能结果通过 oracle differential；
- 核心 causal online rounds 与论文一致；
- Dealer/offline/online 角色与论文一致；
- 核心通信量的数量级和参数增长趋势与论文公式一致；
- 项目 adapter 与 paper core 分开标记和计量；
- 实验可以从固定 revision 和保存的命令重复运行。

允许使用的名称包括：

```text
Protocol I paper-aligned experimental reproduction
Agarwal Protocol I functionality-aligned implementation
Protocol I 3-round paper-core reproduction
```

### 3. message/material-level paper-exact

以下名称仍需要 full version、作者 transcript 或等价的一手 material 证据：

```text
author-implementation-exact
message-level paper-exact
material-level paper-exact
byte-for-byte reproduction
```

缺少这些资料只限制上述严格命名，不再阻塞论文对齐实验复现。

## Protocol I 已核验依据

当前已经核验：

- Protocol I 使用 `(2+1)` 模型；
- `P0`、`P1` 是在线双方；
- `P2` 是 offline party/dealer；
- Dealer 在离线阶段发送 correlated randomness，在线阶段保持静默；
- Protocol I 使用 secure shuffle 和 all-pairs Compare-Aggregate ranking；
- Protocol I paper core 为三个在线轮次；
- secure shuffle 公开 masked shuffled list `π(x)+r`；
- `r` 是与 shuffle 关联的私有随机 mask，不是公开输出；
- Fsort/Fselect 的原生输出是 key shares 和 corresponding payload shares；
- original-order XOR Top-K mask 是项目 adapter；
- 论文 rank 为 minimum `0`、maximum `n-1`；
- stable rank 中，相等元素的原位置越早，论文 rank 越小；
- Theorem 4.1 给出了带 `p` bit payload 的 Protocol I 在线通信公式。

详细证据和实验验收规则见：

```text
docs/decisions/M2_PROTOCOL_I_PAPER_EVIDENCE_SUPPLEMENT_2026-09-19.md
```

## rank 与项目 priority 语义

论文 rank 采用升序方向：

```text
minimum element → rank 0
maximum element → rank n - 1
```

本项目采用：

```text
Top-K largest
最高优先级 → priority-rank 0
score 相同 → original index 更小者优先
```

两者不能只通过一句“反转 rank”完成映射。

直接使用：

```text
priority_rank = n - 1 - paper_rank
```

会同时反转数值顺序和论文 stable tie 的顺序，不能自动保证项目要求的同分语义。

因此，descending Top-K 的 rank-direction 和 tie mapping 属于 C 类 adapter，必须显式定义并
用以下情况测试：

- 所有 score 互异；
- Top-K 边界存在同分；
- 多个元素全部同分；
- `K=1`；
- `K=n`；
- 同分时 original index 更小者被优先选择。

## paper core 与项目 adapter

Protocol I paper core 包含：

```text
secure shuffle
all-pairs Compare-Aggregate ranking
paper-native routing/selection output
```

以下内容属于项目 adapter：

```text
Q20.12 raw-score conversion
composite priority-key construction
paper-rank / priority-rank mapping
original-index payload
reverse routing
original-order Top-K mask
XOR-share conversion
project metrics and error semantics
```

Theorem 4.1 的 3-round 声明不能未经分析直接覆盖这些 adapter。

报告必须分别记录：

```text
paper_core_online_rounds
adapter_online_rounds
total_online_rounds

paper_core_online_bytes
adapter_online_bytes
transport_overhead_bytes
online_bytes_total
```

如果 adapter 增加 causal communication round，必须单独报告，不能将整个含 adapter 的系统
继续称为“3-round Protocol I”。

## Protocol I 通信目标

对于：

```text
G = Z_L'
L' >= 2L
ell' = ceil(log2 L')
p = payload bit width
```

Theorem 4.1 给出的核心在线通信目标为：

```text
4n(ell' + p) + 2n ceil(log2 n) bits
```

对应在线通信数量级为：

```text
O(n(ell' + p + log n))
```

全对 Compare-Aggregate 的计算和 DCF preprocessing 主项为：

```text
O(n^2)
```

因此，实验通信核验至少应确认：

1. 固定 `ell'` 和 `p`、增大 `n` 时，核心在线通信近似线性增长；
2. 固定 `n`、增大 `ell'` 或 `p` 时，核心在线通信近似线性增长；
3. 核心在线通信中不出现无法解释的 `O(n^2)` 项；
4. DCF 的二次主项主要体现在计算和离线 key material；
5. frame/header、连接建立和 metrics 元数据作为 transport overhead 单列；
6. 理论值与实测值的差异能够由 share 数量、序列化和固定工程开销解释。

实验不要求与论文逐 bit 完全相等，但必须保持可解释的常数因子和相同增长数量级。

## Protocol I 实验验收门

M2-reproduction 当前状态为：

```text
IMPLEMENTATION_GO
```

最终退出条件如下。

### 功能正确性

验证顺序：

```text
conformance
→ cleartext oracle differential
→ independent-process E2E
```

至少覆盖：

- 排序或选择结果；
- payload/index correspondence；
- stable tie；
- original-order mask；
- share reconstruction；
- 边界输入和重复值。

### 在线轮数

Protocol I paper core 必须保持：

```text
3 causal online rounds
```

连接建立、日志同步等 transport event 与协议 causal rounds 分开记录。

### Dealer 边界

必须满足：

- preprocessing 与在线输入独立；
- Dealer 只在 offline phase 产生和分发 material；
- Dealer 不读取在线输入；
- Dealer 不接收在线消息；
- Dealer 不参与 online Eval；
- online phase 仅由 `P0`、`P1` 执行。

### 通信量

至少记录：

```text
offline_bytes_total
online_bytes_p0_to_p1
online_bytes_p1_to_p0
paper_core_online_bytes
adapter_online_bytes
transport_overhead_bytes
online_bytes_total
```

### 可复现性

每组实验保存：

- Git revision；
- 构建和运行命令；
- 编译器、依赖和机器环境；
- `n`、`K`、`L`、`L'`、`ell'` 和 `p`；
- 输入文件或输入生成方式；
- 随机种子；
- 重复次数；
- 原始 stdout/stderr；
- 原始 metrics；
- 理论通信量与实测通信量；
- paper core 与 adapter 的拆分。

未实际测量的字段继续写：

```text
NOT_MEASURED
```

## 后续执行顺序

### 1. M2 Protocol I 论文对齐实验复现

1. 冻结 paper core 与项目 adapter 的接口。
2. 实现或选用符合论文 §2.4 高层功能的 secure shuffle。
3. 保证 key 与 corresponding payload 使用同一逻辑 permutation。
4. 实现 all-pairs Compare-Aggregate ranking。
5. 将 paper core 控制在三个 causal online rounds。
6. 单独实现 raw-score、index、tie、reverse-routing 和 mask adapter。
7. 完成 conformance、oracle differential 和独立进程 E2E。

Full version、作者 frame 和作者具体 preprocessing package 不再是实现前置条件。

### 2. Protocol I 通信核验

对多个 `n`、位宽和 payload 位宽采集数据，并分别比较：

```text
理论 paper-core 通信量
实测 paper-core 通信量
adapter 通信量
transport overhead
总通信量
```

确认数量级、增长趋势和常数差异均可解释。

### 3. Protocol I 接口交接

冻结：

- 输入域和定点数语义；
- Top-K largest 与 stable tie；
- rank-direction/tie mapping；
- key、payload 与 original index 的绑定；
- paper-native 输出与项目 mask 输出的边界；
- offline/online material；
- 公开值和泄露边界；
- 错误语义；
- 可复现命令、测试和 metrics。

### 4. M5 Protocol III 论文对齐实验复现

仅在 field contract 和非零 payload 等前置条件明确满足后进入实现。

不将 `Z_(2^b)` 自动当作 field，也不为零 payload 或逆元失败添加未记录的隐式降级路径。

目标是完成 2-round paper core 的正确性、轮数、通信量和 E2E 验收。作者逐消息实现证据继续
作为更严格命名所需的补充资料，而不是实验实现的绝对前置门。

### 5. Protocol III 通信核验和接口交接

分别计量：

- 2-round paper core；
- raw-score adapter；
- original-index adapter；
- mask adapter；
- transport overhead。

完成后冻结供 M6A、M6B 和统一报告使用的接口。

### 6. M6A AAV86

先取得并登记必要算法依据，解决自适应预处理问题。

不得引入 online Dealer，也不得用完整图预分配冒充 AAV86 的 exact-edge 节省。

完成安全模型、runtime、oracle differential 和独立进程 E2E 后，进入 M6B。

### 7. M6B BB90+DCF

登记：

- 原始算法依据；
- 目标顺序统计量；
- 随机性和概率成本；
- 正确性条件；
- DCF preprocessing 的连接方式；
- 实现来源和固定 revision。

AAV86、QuickSelect、Direct Top-K 或 pivot-pruning 原型不能仅因能够选择第 K 大而登记为
BB90。

### 8. M7 六方案统一报告

所有方案必须在同一可复现环境下采样，并保存：

- 精确 revision；
- 构建和运行命令；
- 环境及依赖；
- 输入规模、K、种子和重复次数；
- 原始日志和原始计数；
- offline/online 拆分；
- paper core/adapter 拆分；
- 正确性、轮数、通信量和泄露口径。

未经实际运行的字段继续写 `NOT_MEASURED`。

## M4 CipherGPT 的治理状态

M4 CipherGPT 已取消。

CipherGPT 不再是当前路线中的独立实现阶段，也不占用任何里程碑权重。

`CipherGPT/`、相关论文或历史实验资料可以继续作为只读背景材料保留，但不能据此恢复：

```text
M4 CipherGPT native baseline
```

如果未来需要将 CipherGPT 作为外部对照重新引入，必须通过新的治理决定单独定义：

- 使用目的；
- 来源和固定 revision；
- 许可证；
- 实现身份；
- 输入输出契约；
- 验收条件；
- 在六方案报告中的位置。

不得通过修改路线名称或复用旧进度权重使其隐式恢复。

## 分支和合并规则

新分支使用功能前缀：

```text
docs/
design/
feat/
fix/
test/
perf/
review/
```

不再创建 `codex/` 前缀分支。历史同类 ref 的迁移见：

```text
docs/BRANCH_MAP.md
```

一个分支只承载一个里程碑或一项明确治理变更。所有新实现分支从最新
`origin/main` 开始。

本次 `docs/m2-paper-evidence-handoff` 与 `main` 已有较长分叉历史，因此不得直接整体合并。
正确方法是从最新 `main` 建立新的短生命周期文档整合分支，只提取最终确认的文档内容，
避免带入旧源码、测试和历史决策差异。

合并前至少执行：

```powershell
git fetch origin --prune
git status --short --branch
git diff --check
git diff --name-status origin/main...HEAD
```

最终 PR 的文件范围必须与声明的文档任务一致。
