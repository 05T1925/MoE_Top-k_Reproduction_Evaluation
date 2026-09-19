# 项目全局进度与后续阶段

日期：2026-09-19

状态：规划与审计口径；不是代码覆盖率、性能结果或论文一致性声明。

## 总体结论

当前项目已经完成公共正确性底座、Protocol I 的 C 类工程基线，以及与 Protocol III
相关的模块化工程基线。尚未完成的核心工作是 Protocol I 与 Protocol III 的
paper-exact 实现、相应通信核验、AAV86、BB90+DCF，以及最终六方案统一报告。

本文不再给出单一“完成百分比”。旧的约 55% 估计依赖已经取消的 CipherGPT
里程碑及其权重，无法与当前路线自洽。后续进度以各阶段的可审计退出条件为准，不以提交数、
代码行数或未经重新定义的加权百分比衡量。

当前冻结执行路线为：

```text
M2 Protocol I paper-exact
→ Protocol I 通信核验
→ Protocol I 接口交接
→ M5 Protocol III paper-exact
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
| M1/M1.1：公共正确性底座 | 完成 | Q20.12 语义、stable tie、oracle、CmpAgg、metrics、DCF conformance、Ubuntu 验收 | 性能、LAN/WAN 结果不属于该阶段的已测结论 |
| M2：Protocol I C 类工程基线 | 完成 | 8-round raw-score mask baseline、shuffle/transport/material、独立进程与 Ubuntu/EMP 记录 | 不得升级命名为 paper-exact |
| M2-exact：Protocol I 论文精确核心 | 证据阻塞 | 会议版的高层协议结构、原生输出、Dealer 模型、stable rank、`π(x)+r` 等证据已经定位 | 逐轮 transcript、party view、`r` material、concrete same-permutation、padding/dummy、full version/proof 等仍需审计 |
| Protocol I 通信核验 | 未开始 | 已有 metrics 和计量规则可复用 | paper core 与项目 adapter 的轮数、通信量、公开值和泄露必须分别核验 |
| Protocol I 接口交接 | 未开始 | 已知项目目标输出为 original-order XOR Top-K mask | 冻结输入、输出、rank 方向、index/payload 绑定、adapter 和错误语义 |
| M3：Protocol III 模块化工程基线 | 完成 | 3-round priority-key 路径、5-round raw-score 路径、DPF routing、secure combine、三方 E2E | 这些实现不等于论文精确 2-round Protocol III |
| M5：Protocol III paper-exact | 设计准备 | paper evidence、field contract 和预实现检查清单 | 域与非零 payload 前置条件、2-round transcript、实现、泄露/轮数审计、E2E |
| Protocol III 通信核验 | 未开始 | 可复用公共 metrics 与测试框架 | paper core 与 raw-score/index/mask adapter 分项计量 |
| Protocol III 接口交接 | 未开始 | 已有模块化接口经验 | 冻结可供 M6A/M6B 和统一报告使用的语义与测量接口 |
| M6A：AAV86 | 未开始 | 已识别自适应 exact-edge 预处理门 | 一手算法依据、offline-only 预处理、安全模型、runtime、差分测试和 E2E |
| M6B：BB90+DCF | 资料准备 | 已确定为 AAV86 之后的独立阶段 | 原始算法依据、目标顺序统计量、DCF 衔接、实现来源、差分测试和 E2E |
| M7：六方案统一报告 | 未开始 | 指标口径和比较矩阵已有基础 | 同一环境实际采样、原始记录、版本固定和可比较报告 |

## 已完成与未完成的边界

“工程基线完成”不等于“论文精确复现完成”。

M2 当前完成的是名称、语义和测试记录可追溯的 C 类工程基线。Protocol I 的论文精确核心
仍受 material、逐轮 transcript 和 party-view 证据不足的限制。

M3 当前完成的是与 Route A 隔离的模块化工程基线。论文声称的 Protocol III 2-round
核心属于 M5，不能用现有 3-round 或 5-round 工程路径代替。

论文中的 rank 方向和项目统一的 priority-rank 方向不同：

```text
论文 rank：
minimum = 0
maximum = n - 1
相等元素中，原序列更早者 rank 更小

项目 priority-rank：
最高优先级 = 0
Top-K largest
score 相同时，original index 更小者优先
```

因此，从论文 rank 到项目 priority-rank 必须存在显式的方向映射。不能仅凭两者都具有
stable tie，就宣称 rank 编码完全相同。

所有没有来自已保存实际运行记录的性能字段均为 `NOT_MEASURED`。仅有历史测试记录、
设计表或论文成本公式时，不得改写为本项目已经测得的性能结果。

M2 的证据收尾详见：

```text
docs/decisions/M2_PROTOCOL_I_PAPER_EVIDENCE_SUPPLEMENT_2026-09-19.md
```

M2/M3 的实现身份与交接边界详见：

```text
docs/M2_M3_TEAMMATE_HANDOFF_2026-09-19.md
docs/reproduction/M2_M3_STAGE3N_FINAL_CLOSEOUT_2026-09-15.md
```

## 后续执行顺序

### 1. M2 Protocol I paper-exact

在本项目的 paper-exact 验收标准下，先取得或确认能够审计以下内容的一手资料：

- 三个在线轮次的 sender、receiver、字段及 causal dependency；
- 各方在每个阶段可见的公开值和私有 material；
- shuffle 关联的私有随机 mask `r` 的生成、分发和消费；
- `π`、`r` 与公开值 `π(x)+r` 的生命周期；
- key 与 corresponding payload 的同一 permutation 绑定；
- original index 作为项目新增 payload/adapter 时的绑定方式；
- padding 和 dummy 语义；
- paper-native key/payload shares 到项目 original-order mask 的适配边界。

缺少上述证据时，不补猜 message-level 或 material-level 实现，也不把已有 Route A、
M3 material 或测试层明文重构重新命名为 paper-exact。

### 2. Protocol I 通信核验

paper core 和项目 adapter 必须分别计量。Theorem 4.1 的 3-round 声明不能直接覆盖：

- Q20.12 raw-score 输入转换；
- rank 方向映射；
- original-index 绑定；
- reverse routing；
- original-order XOR Top-K mask 生成。

上述 adapter 是否产生额外 causal communication round，必须依据实际设计审计，不能未经
分析直接记为零，也不能预先断言一定增加轮数。

### 3. Protocol I 接口交接

只有在 paper core、通信计量和 adapter 边界冻结后，才进行 Protocol I 接口交接。交接至少
包括：

- 输入域和定点数语义；
- Top-K largest 与 stable tie 语义；
- paper rank 到 project priority-rank 的方向映射；
- key、payload 与 original index 的绑定；
- paper-native 输出与项目 mask 输出的边界；
- 公开值、泄露面和错误语义；
- offline/online material 身份和消费点；
- 可复现命令、测试和 metrics 输出。

### 4. M5 Protocol III paper-exact

仅在 field contract 和非零 payload 等前置条件得到明确满足后进入实现。不将
`Z_(2^b)` 自动当作 field，也不为零 payload 或逆元失败添加未记录的隐式降级路径。

Protocol III 的 2-round paper core 与 raw-score、index、mask adapter 必须分别审计。

### 5. Protocol III 通信核验与接口交接

按照与 Protocol I 相同的证据纪律，冻结：

- 两轮 causal transcript；
- DPF/DCF material；
- 每方 view 和公开值；
- paper-native 输入输出；
- 项目新增 adapter；
- 通信量、轮数和泄露；
- 可供 M6A、M6B 与统一报告使用的稳定接口。

### 6. M6A AAV86

先解决一手算法依据与自适应预处理问题。不得引入 online Dealer，也不得用完整图预分配
冒充 AAV86 的 exact-edge 节省。

完成来源登记、安全模型、runtime、oracle differential 和独立进程 E2E 后，才进入 M6B。

### 7. M6B BB90+DCF

BB90+DCF 是 AAV86 之后的独立阶段。必须登记算法来源、固定版本、目标顺序统计量、随机性、
概率成本、正确性条件以及与 DCF 预处理的连接方式。

AAV86、QuickSelect、Direct Top-K 或 pivot-pruning 原型不能仅因能够选择第 K 大而登记为
BB90。

### 8. M7 六方案统一报告

统一报告最后执行。所有方案必须在同一可复现环境下采样，并保存：

- 精确 revision；
- 构建和运行命令；
- 编译器、依赖与机器环境；
- 输入规模、K、种子和重复次数；
- 原始日志和原始计数；
- offline/online、paper core/adapter 的拆分；
- 正确性、轮数、通信量与泄露口径。

未经实际运行的字段继续写 `NOT_MEASURED`。

## M4 CipherGPT 的治理状态

M4 CipherGPT 已取消。CipherGPT 不再是当前路线中的独立实现阶段，也不占用任何里程碑权重。

如果未来需要把 CipherGPT 作为外部对照重新引入，必须通过新的治理决定单独定义范围、
证据来源、许可证、验收条件和报告位置；不能通过修改本文中的阶段名称使其隐式恢复。

## 分支命名规则

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

不再创建 `codex/` 前缀分支。历史同类 ref 的迁移见 `docs/BRANCH_MAP.md`。

一个分支只承载一个里程碑或一项明确的治理变更。所有实现分支从最新
`origin/main` 开始，并在合并前重新核对基线、测试记录和冻结目录。
