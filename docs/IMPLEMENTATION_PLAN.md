# MoE Top-K 详细实施计划

更新日期：2026-09-13

本文是 `PROJECT.md` 的执行版。`PROJECT.md` 定义项目范围、论文边界、统一语义和长期指标；本文将工作拆成可分配、可验证、可交接的阶段。

若本文与项目总纲冲突，以更新后的 `PROJECT.md` 和团队明确决定为准，并同步修正文档。不得仅在代码、分支名称或口头约定中形成新的协议和计量规则。

当前严格 M2 门以
[M2_PROTOCOL_I_CURRENT_EVIDENCE_GATE_2026-09-19.md](decisions/M2_PROTOCOL_I_CURRENT_EVIDENCE_GATE_2026-09-19.md)
为准：message/material/party-view/round-exact G1 为 BLOCKED；只有明确的 NON-EXACT
paper-aligned experimental reproduction READY，且不能进入 G2、G3 或解除 M5 runtime 前置条件。

本次修订取消 M4 CipherGPT 实施及性能任务，保留 M2、M3、M5 的编号和历史记录，将后续图升级分为 M6A AAV86、M6B BB90+DCF。

## 1. 当前结论与执行主线

### 1.1 已完成基础

| 里程碑 | 当前状态 | 保留边界 |
| --- | --- | --- |
| M0 | 仓库、来源和冻结基线已建立 | `VFSS-baseline/` 不修改 |
| M1/M1.1 | 统一语义、oracle、基础适配、metrics 和测试入口已完成 | 不重新定义 score、tie rule 和输出 |
| M2 工程基线 | C 级模块化实现已完成并合入 main | 四轮核心、raw-score 到 mask 共八轮，不是论文精确实现 |
| M3 | 三轮模块化核心及 raw-score 五轮扩展已完成并冻结 | 作为 M5 的基础和对照 |

当前 M2 工程实现标签为：

```text
m2_protocol_i_raw_score_input_modular_8round_mask_output
```

Approved NON-EXACT Candidate-B sequence: 1) documentation remediation decision; 2) independent documentation review; 3) explicit implementation authorization; 4) DPF/uCMP conformance slice; 5) Dealer preprocessing/material factory; 6) BoundPublicMaskShuffle core; 7) Protocol I integration; 8) independent-process E2E/failure tests; 9) communication/complexity instrumentation; 10) final implementation/security review. No implementation stage is complete; strict G1/official G2/G3/M5 do not change. See [the contract](decisions/M2_PROTOCOL_I_DEALER_DPF_BOUND_PUBLIC_MASK_SHUFFLE_REMEDIATION_CONTRACT_2026-09-20.md).

当前 M3 两个入口为：

```text
agarwal_protocol_iii_modular_3round
moe_topk_protocol_iii_raw_score_modular_5round
```

M2.16 完成的是 paper-exact 可行性与泄露审计，没有实现精确核心。M3 已在 `main@bb0d0e8` 完成整改；不得继续将 M3 写成尚待开始。

### 1.2 当前待完成目标

- Protocol I 论文精确三轮核心及通信核验。
- Protocol III 论文精确两轮核心及通信核验。
- Protocol I、Protocol III 各自的 AAV86 升级及完整性能验收。
- Protocol I、Protocol III 各自的 BB90+DCF 升级及完整性能验收。
- 六种目标方案的统一横向报告。

轮数口径固定为：

- Protocol I：Theorem 4.1 的三轮核心。
- Protocol III：Theorem 4.2 的两轮核心。
- raw-score 输入适配和原顺序 mask 输出适配另列，并计入端到端主结果。

不得将 Protocol I 目标写成“论文两轮”，也不得将核心轮数直接用作统一输入输出路径的总轮数。

### 1.3 新执行顺序

```text
M2 精确核心实现完成
  → Protocol I 通信测量及差异解释
  → M2 公共接口交接
  → M5 精确核心实现完成
  → Protocol III 通信测量及差异解释
  → 基础协议接口与计量结果交接
  → M6A 两种 AAV86 升级实现
  → M6A 完整性能验收
  → M6B 两种 BB90+DCF 升级实现
  → M6B 完整性能验收
  → M7 六种方案统一报告
```

M3 作为已完成前置条件保留，不重新安排实现。M4 标记取消，不再作为 M5 的前置条件。

资料研究、设计、失败用例整理可以提前进行；依赖未冻结接口的实现和正式验收必须遵循上述顺序。

### 1.4 最终六种方案

1. Protocol I。
2. Protocol III。
3. Protocol I + AAV86。
4. Protocol III + AAV86。
5. Protocol I + BB90+DCF。
6. Protocol III + BB90+DCF。

已完成的 I 四轮核心和 III 三轮核心作为工程对照另列，不替代六种方案中的精确基线。

历史 `Direct Top-K` 原型不直接改名为 BB90+DCF。CipherGPT native、CipherGPT-style adapter 和 CryptoMoE 接入不属于本轮六种方案的交付范围。

### 1.5 文档同步边界

本次路线替代旧 `docs/decisions/ROADMAP_PRIORITY_2026-09-04.md` 中冲突的顺序与阶段门。

以下文档应同步：

- `PROJECT.md`
- `docs/IMPLEMENTATION_PLAN.md`
- `docs/TEAM_WORK_PLAN.md`
- `docs/M3_ONWARD_TEAM_WORK_PLAN.md`
- `README.md`
- 路线决策及配套计量规范

旧计划中的 M4→M5 前置关系不再执行。历史实现和实验记录保留，不将新目标反写为旧阶段已完成能力。

## 2. 全程不变的统一契约

### 2.1 输入和输出

- 单次调用包含 n 个 32 位二补码 signed fixed-point score 算术共享。
- 小数位数为 12，数值解释为量化整数除以 `2^12`。
- 两方 raw shares 满足 `raw = (x0+x1) mod 2^32`。
- 公开参数包含 n、K、位宽、payload 形状、party 拓扑和模式。
- 选择 score 最大的 K 个元素，同分按 original index 升序。
- 最高优先级 rank 为 0，有效范围为 `0..n-1`。
- 输出为原始输入顺序下长度 n 的秘密共享 Top-K bit-mask。
- 每位为 0/1，且恰好 K 位为 1。
- 不要求 Top-K 集合内部排序。
- selected payload 不能代替统一 mask。

统一接口：

```text
([z_1]^B, ..., [z_n]^B)
    <- TopK(([x_1]^A, ..., [x_n]^A), K)
```

元素与原始位置绑定、输入编码、shuffle、路由、逆映射、共享转换和 mask 生成均属于实际执行路径。主结果不能将这些步骤作为免费后处理扣除。

六种方案复用同一份输入语义、oracle 和输出契约。

### 2.2 安全与测试边界

- P2 为输入无关的离线材料提供方，完成分发后退出在线路径。
- secure runtime 不通过测试辅助逻辑重构 raw score、priority key、原顺序 rank、比较位、selected index 或最终 mask。
- 原始输入和中间值不得交给 Dealer 计算正确答案。
- 具体协议允许公开的 shuffled rank 或局部 rank 必须有单独的泄露说明，不能扩大为任意 rank 公开。
- oracle 重构只在隔离的 TEST_ONLY 路径发生。
- 材料必须绑定 session、party、参数和协议阶段，并按规定一次性消费。
- 不使用文件轮询、固定 sleep、模拟 shuffle 或在线补发材料隐瞒协议依赖。

新适配器按以下顺序验证：

```text
conformance
  → oracle differential
  → 独立进程 E2E
  → 消息、泄露和计量审计
```

### 2.3 测试矩阵

| n | K | AAV86 迭代 r |
| --- | --- | --- |
| 128、256 | 2、8 | 2、3、4、5 |
| `10^3`、`10^4`、`10^5`、`10^6` | 80 | 2、3、4、5 |

执行规则：

- 六种方案沿用同一 `(n,K)` 矩阵。
- AAV86 两种路线均覆盖 `r=2..5`。
- BB90 的迭代参数根据采用版本单独定义，不复用 AAV86 字段冒充已确定配置。
- 同一比较组使用同一输入、输入种子、score 解释、网络和 oracle。
- 输入种子与算法随机种子分别记录。
- 保留基础随机量化整数均匀范围 `[-32*2^12, 32*2^12]`，端点包含；其他分布单独标注。
- 正确性测试另覆盖重复值、全相等、负数、正负边界、`K=1`、`K=n`、非二次幂 n 和非法输入。
- 正式性能实验分别覆盖 LAN/WAN。
- 每个配置预热 1 次、正式运行 5 次，保留逐次结果并报告 median/min/max。
- 随机图每次运行的种子、边数、节点复杂度和性能结果关联保存。
- 无法运行的配置保留失败阶段、资源限制和原因，不缩小参数后冒充原配置。

基础协议的两次通信核验先覆盖小规模边界及 `(128,2/8)`、`(256,2/8)`，并使用可行规模检查增长趋势。它们不要求在进入后续阶段前强行完成全对全 `n=10^6` 实验。

完整长期矩阵及无法完成配置的记录在正式性能阶段处理。

### 2.4 必须输出的指标

| 字段 | 含义 |
| --- | --- |
| `offline_time_ms` | 全部预处理耗时，明确生成、序列化和分发边界 |
| `offline_material_total_bits` | 在线所需全部离线材料之和 |
| `online_time_ms` | 输入就绪至统一 mask 完成的在线耗时 |
| `online_comm_total_bits` | 在线各方实际发送字节折算后的总和 |
| `online_comm_per_party_bits` | total 除以在线方数量，主表通信字段 |
| `online_rounds` | 因果依赖决定的在线轮数 |
| `online_prg_calls_total` | 所有在线方长度倍增 PRG 调用总数 |
| `comparison_edges_total` | 实际执行的无序比较边总数 |
| `total_time_ms` | offline 与 online 时间之和 |

同时保留：

- 每方 `sent_bits`、`received_bits`；
- revision、实现标签、runtime、party topology；
- n、K、算法迭代参数；
- score、comparison、rank、payload 位宽与表示；
- logical/padded layout；
- 输入分布、输入种子、算法种子；
- CPU、内存、OS、编译器、flags、build type、线程数；
- 网络配置、带宽、RTT；
- 命令、预热次数、重复次数；
- correctness status、失败原因和原始结果定位信息。

total 只累加发送量，received 用于核验，不再次加入总量。

同一条比较边由两方分别计算，不因此记为两条边；同一位置对在不同步骤实际重复比较时，按实际次数计数。比较边数、DCF 调用数和 PRG 调用数分别记录。

未知指标使用 `NOT_MEASURED`，不能用零、估算或历史数字填充。完整性能阶段必须补齐适用计数；仅有字段而没有可信观测不算测量完成。

### 2.5 分阶段计量

每种方案至少区分：

1. raw-score 输入适配；
2. 论文核心或明确标记的组合核心；
3. 原顺序 mask 输出适配；
4. 测试控制、报告传输和 oracle 验证等辅助工作。

主比较使用完整安全路径；论文核验使用功能、参数和边界一致的核心数据。

阶段通信应能与总通信对账。并行或合并执行时按实际消息依赖计算总轮数，不机械相加，不把有依赖的两轮消息仅因放入同一函数而称为一轮。

历史计时边界保持原样。若旧记录包含 Dealer 启动、输入分发或报告收集，应继续注明；不能静默改写为纯核心时间。

## 3. 阶段门与验收规则

### 3.1 基础协议统一阶段门

M2 精确核心和 M5 精确核心均按以下顺序推进：

```text
G1：实现与正确性完成
  → G2：通信实测及差异解释完成
  → G3：接口和证据交接完成
```

| 阶段门 | 必须提供的证据 | 通过后的状态 |
| --- | --- | --- |
| G1 实现门 | conformance、oracle differential、独立进程 E2E、消息与泄露审计 | 可进入正式通信核验的候选 |
| G2 通信门 | 论文公式、实现消息推导、实际分方计数及差异解释 | 成本对应关系已核验 |
| G3 交接门 | 冻结 revision、接口契约、最小调用示例、材料及计量说明、接收方复跑记录 | 可作为下一阶段依赖 |

代码可以按依赖拆成多个可验证 PR 合并，但“代码已合并”不等于 G2/G3 已通过。

论文精确身份还要求全部相关论文前提成立；不能只凭 oracle 通过或轮数相同升级标签。

### 3.2 通信核验方法

每次核验形成三层对照：

```text
论文成本公式
  ↔ 实际参数和消息定义下的实现成本
  ↔ 独立进程执行得到的实际发送量
```

必须检查：

1. 比较功能是否一致：sorting、单个 order statistic、Top-K mask 不混用。
2. n、logical/padded 规模、输入域、rank 域和 payload 是否一致。
3. 使用了哪一条定理、脚注和优化。
4. total/per-party、bits/bytes、KB/KiB 是否一致。
5. 核心、输入适配、输出适配和封装是否分离。
6. 是否存在额外位宽、字节对齐、帧头、重复发送或不同材料表示。
7. 发送量与对端接收量是否能在同一统计边界下对应。
8. 随 n、K 增长的趋势是否符合实现消息结构。
9. 是否因遗漏必要步骤或改变泄露而得到较低通信量。

Protocol I 主要对照 Theorem 4.1。Protocol III 主要对照 Theorem 4.2，并注明是否采用其中脚注描述的共用掩码优化。

Table 2、Table 3 的估算参数和归一化方式需单独核对，不能把论文估算值写成本项目实测目标值。

### 3.3 通信门退出条件

- 原始分方和分阶段计数可复算。
- 论文公式与当前实现的对应关系明确。
- 核心及适配、封装成本分别说明。
- 关键差异已经定位并解释。
- 正确性、轮数和安全条件仍然成立。
- 接收方可按命令复跑核验配置。
- 未解决项有明确记录，不被“数量级接近”覆盖。

不要求 wire bytes 与理论 bit 数逐位相等，也不以任意误差百分比代替原因分析。

通信核验通过只支持成本一致性。数量级不符时，先定位实现或计量差异；不能直接判定论文错误。数量级相符时，也不能据此证明功能或安全性正确。

### 3.4 图升级阶段门

M6A、M6B 各自执行：

```text
算法与安全设计
  → 两种路线实现
  → 正确性、消息与材料审计
  → 计量能力验收
  → 完整矩阵性能实验
  → 报告与接口交接
```

M6A 完成两种路线的性能验收后，再进入 M6B 依赖实现及正式实验。

资料核对、明文算法 oracle 和不依赖未冻结接口的设计可以提前开展。

### 3.5 性能状态与失败配置

每个配置区分：

- 成功完成并取得全部适用指标；
- 正确性失败；
- 执行失败或超时；
- 资源不足；
- 指标未测或计数能力缺失；
- 参数不适用，并附算法依据。

资源无法承受的大规模点可按项目总纲记录原因和未测字段，阶段报告必须披露覆盖范围。此类记录不等于该点成功完成。

对于已经成功运行的配置，若 PRG、材料或通信等必需指标缺失，不能标为“全部指标完成”。平台性计量缺口应在正式性能验收前补齐。

不得静默丢弃失败随机种子、只保留最快结果或用重试掩盖正确性问题。

## 4. 里程碑任务与退出条件

### 4.1 M0：仓库与规范基线

状态：已完成。

保留交付物：

- 冻结提交 `993696e` 与标签 `vfss-baseline-2026-09-03`；
- 来源、论文版本、目录和忽略规则；
- 统一输出、测试矩阵、指标和证据层级；
- 远端仓库与文档入口。

历史验收以 `docs/M0_REVIEW.md` 为准。M0 时记录的 LICENSE 和参考资料分发事项保持其证据边界，不作为本次已解决事项。

不重新执行已完成的仓库初始化，不因本次改计划修改冻结树。

### 4.2 M1/M1.1：统一 oracle、数据和基础计量

状态：已完成并冻结。

保留：

- Q20.12 signed score 语义；
- 数值降序、同分 original index 升序；
- 基础输入分布与边界向量；
- 明文 stable-rank/Top-K oracle；
- DCF、CmpAgg conformance；
- 基础 MetricsRecord、provenance 和 CTest 入口。

历史验证包括：

```text
moe_topk_m1_oracle_test
moe_topk_m1_cmpagg_test
moe_topk_m1_metrics_test
moe_topk_m1_dcf_conformance_test
```

M1.1 在 Ubuntu 24.04.4、WSL2、x86_64 干净 Debug 构建中验证 revision：

```text
a2efe5e3d2d22bb3c031fb24dc3246c37d442fad
```

CTest 为 4/4 通过，详见：

`docs/M1_1_UBUNTU_HANDOFF.md`

后续只按实际需要补充计量和适配能力，不重新讨论已冻结语义，不把历史 `NOT_MEASURED` 改成已测。

### 4.3 M2：Protocol I 精确核心、通信核验与交接

状态：C 级工程基线已关闭。严格 M2 G1、官方 G2、G3 和 M5 runtime 是未来 gated sequence，当前不可执行；当前仅允许 NON-EXACT paper-aligned 实验复现、证据准备和 canonical gate 允许的 design-only 工作。

主责：角色 A。角色 B 交叉评审并接收接口。

#### 输入

- M1 语义、oracle、测试输入与 metrics；
- 已完成的 M2 shuffle、CmpAgg、transport、package 和输入输出适配；
- Agarwal §2.4、§4.1、Theorem 4.1；
- Chase secret-shared shuffle；
- M2.15/M2.16 对 public masked-list 和泄露的审计记录。

#### M2 实现阶段

1. 明确当前 PS API 与论文功能之间的差异。
2. 设计并实现同时输出秘密共享 `pi(x)` 与公开 `pi(x)+r` 的所需功能。
3. 保证两种输出使用同一隐藏置换，r 与后续 GRank 材料关联且对任一单方未知。
4. 验证预处理输入无关、Dealer 在线静默和材料一次性使用。
5. 接入全对全 CmpAgg、稳定 rank 和 payload 路由。
6. 保留 raw-score 输入与原顺序 XOR mask 输出。
7. 按实际消息依赖实现并审计三轮论文核心。
8. 运行 conformance、差分、独立进程 E2E 和异常输入/传输测试。
9. 保留当前四轮核心工程实现作为回归对照。

不能通过额外的事后 masked-list 交换、公开置换或测试端构造公开列表冒充论文所需的 shuffle 功能。

当前历史七轮候选是：

```text
2 轮 raw-score adapter
+ 3 轮论文核心
+ 2 轮 reverse mask adapter
= 7 轮候选总路径
```

它不是既成实现；若最终适配改变，应重新审计，不固定填写总轮数。

#### M2 通信阶段

G1 通过后：

1. 冻结被测 candidate revision 和实现标签。
2. 运行小规模边界与 `(128,2/8)`、`(256,2/8)` 核验。
3. 选择可行增长点检查通信趋势。
4. 对照 Theorem 4.1 及实际输入、payload 和 rank 表示。
5. 分别记录 raw adapter、shuffle/core、rank reveal、reverse mask 和封装成本。
6. 输出论文公式、实现推导和实际计数对照。
7. 定位并解释差异，必要时修正实现后重跑受影响配置。

#### M2 交接阶段

G2 通过后向角色 B 提供：

- 可复跑的 revision 与构建配置；
- 公共接口、输入输出和 layout；
- score/rank 域及 payload 表示；
- 预处理材料格式、绑定和消费规则；
- transport/session/fingerprint 契约；
- 最小调用示例与错误语义；
- 分阶段计数说明；
- 正确性、轮数、泄露和通信核验报告；
- 与现有 M3 的兼容范围及不能直接复用的部分。

交接不要求 M5 使用 Protocol I 专用 shuffle。应区分可复用的公共部件和仅属于 Protocol I 的实现。

角色 B 在接收 revision 上复跑最小示例及代表性核验，记录接收结果。

#### 交付物

- Protocol I 精确三轮核心候选与统一 mask 路径；
- 独立进程运行入口；
- 测试和阶段审计；
- 通信核验报告；
- 公共接口交接记录。

`agarwal_protocol_i_exact_mask_output` 为目标身份，不表示当前已有可执行目标。只有精确条件与 G1/G2/G3 均通过后才更新完成状态。

#### 退出条件

- 论文功能、同置换和材料条件成立；
- 核心三轮可由消息依赖复算；
- 原始输入顺序的 mask 与 oracle 一致；
- 没有新增未声明泄露或在线 Dealer；
- 通信计数可信且关键差异已解释；
- 交接方成功复跑并确认接口；
- 原工程基线和冻结语义未被覆盖。

### 4.4 M3：Protocol III 模块化三轮基线

状态：已完成，保留回归与维护。

实现规范见：

`docs/decisions/PROTOCOL_III_MODULAR_3ROUND_DESIGN.md`

保留的三轮路径：

```text
GRank
  → masked-rank DPF routing
  → secure combine
```

保留的 raw-score 五轮扩展：

```text
carry
  → sign
  → GRank
  → DPF routing
  → secure combine
```

已完成交付物：

- 两个不同输入边界的正式 Party executable；
- DPF、GRank、combine 与 raw-score 测试；
- 原顺序 XOR Top-K mask；
- 独立 Dealer/P0/P1 测试角色；
- 结构化 MetricsRecord 汇总；
- 三轮/五轮因果关系和 secure/test 边界记录。

后续维护要求：

- priority-key 三轮入口不能写成 raw-score 三轮入口；
- 不通过改名将 M3 升级为论文两轮；
- 新计量字段不得静默改变旧记录；
- M5 与 M3 使用同一 oracle 做差分；
- 单位 payload 的 mask 特化不能覆盖一般路由基线身份。

关闭证据见：

`docs/reproduction/M3_REVIEW_CLOSEOUT_UBUNTU_2026-09-10.md`

### 4.5 M4：CipherGPT 原生基线

状态：取消。

本轮不安排：

- CipherGPT 原生修复；
- original-index 或 mask 适配；
- CipherGPT-style VFSS adapter；
- CipherGPT 性能测试；
- CipherGPT 与六种方案的正式比较。

保留编号、历史资料与忽略规则。删除其作为 M5 前置条件的执行要求，不删除历史实验来源。

### 4.6 M5：Protocol III 精确两轮、通信核验与交接

主责：角色 B。角色 A 交叉评审。

#### 前置条件

- M3 已完成并保持稳定；
- M2 精确核心的 G1/G2/G3 通过；
- M5 所需公共接口及其代数适用范围明确。

在前置条件完成前，角色 B 可开展论文核对、域与编码设计、消息表和失败用例准备；不把依赖未冻结接口的实现写成已验收结果。

#### M5 实现阶段

1. 固定满足论文要求的域及表示。
2. 定义非零 payload、零值编码、乘法掩码和逆元失败语义。
3. 核对 VFSS DPF 输出与该表示的兼容性，不能把 `Z_(2^b)` 直接视为域。
4. 划分可复用的 M2 公共接口与必须新增的 M5 适配。
5. 实现 GRank 与 DPF routing 的跨阶段压缩。
6. 验证两轮论文核心，不遗漏输入或 mask 适配。
7. 与 M3 对同一输入、K 和种子进行差分。
8. 运行 conformance、独立进程 E2E、材料复用拒绝和传输异常测试。
9. 审计允许公开值、Dealer 行为和消息依赖。

仅因单位 payload 可简化计算而删掉 combine 轮，不能直接作为 Theorem 4.2 精确复现证据。

#### M5 通信阶段

G1 通过后：

1. 冻结 candidate revision 与标签。
2. 运行与 M2 核验功能可比的小规模及正式基线点。
3. 对照 Theorem 4.2，注明共用掩码优化是否采用。
4. 分解 GRank、压缩路由、表示转换和 mask 输出成本。
5. 比较 M3 与 M5 的通信、轮数及代数适配开销。
6. 核验单个 order statistic 与项目 Top-K mask 的区别。
7. 输出三层通信对照、差异解释与未解决项。

#### M5 交接阶段

G2 通过后，为 M6A 提供：

- 冻结的基线 revision；
- M2/M5 公共部件复用矩阵；
- 域、payload、DPF 与输入输出适配契约；
- 材料生成和一次性消费规则；
- 核心及端到端阶段计量；
- 正确性和通信核验报告；
- 图升级必须重新解决的假设；
- 最小调用示例与复跑结果。

不得将全对全材料生成器直接描述为已支持自适应比较图。

#### 交付物

- `agarwal_protocol_iii_exact_2round` 目标核心及统一 mask 路径；
- 域、非零编码、消息和泄露决策；
- M3/M5 差分及开销对照；
- 通信核验报告；
- M6A 接口交接记录。

#### 退出条件

- 论文代数条件满足；
- 两轮核心经消息审计和独立进程执行验证；
- 输出与冻结 oracle 一致；
- 输入、输出及表示转换成本完整记录；
- 通信差异已解释；
- 两条基础路线的交接可复跑；
- 只有上述条件通过后才更新精确身份。

### 4.7 M6A：AAV86 两种升级与完整性能验收

#### 前置条件

- M2、M5 精确核心与两次通信核验完成；
- 基础接口交接完成；
- M3 工程对照仍可运行。

#### 设计阶段

1. 固定 AAV86 算法及 CA 转换来源。
2. 明确图生成、pivot、bucket、局部 rank 和稳定同分语义。
3. 定义输入种子与算法随机种子。
4. 写明每轮公开值、图生成时机、材料和 Dealer 行为。
5. 解决输入无关、Dealer 在线静默的自适应 exact-edge 预处理。
6. 分别给出 Protocol I、Protocol III 组合协议与输出路径。
7. 审计 Protocol III 组合的代数条件和泄露，不能直接套用 shuffle-based compiler 的证明。

Protocol I+AAV86 对应构造的核心目标为 `2r+1`。Protocol III+AAV86 的 `2r` 是团队组合目标，必须独立推导与验证。

#### 实现阶段

分别交付：

```text
Protocol I + AAV86
Protocol III + AAV86
```

每种方案具有独立标签、入口和计量记录。共享算法部分只保留一份明确契约，不复制另一套 score 或 oracle。

不得使用在线 Dealer 原型、完整图预留或明文图测试冒充目标协议。不同模型的对照必须单独标记并完整计量。

#### 正确性与安全验收

- 明文图算法与稳定 Top-K oracle 一致；
- 安全图执行与明文算法差分一致；
- 重复值、全相等和非二次幂输入正确；
- 输出保持原始顺序且恰有 K 位为 1；
- 自适应材料生成、边绑定和跨轮消费可审计；
- 公开 bucket、局部 rank 和位置的泄露有明确说明；
- 两种组合的轮数独立验证。

#### 计量与性能阶段

1. 补齐可信 PRG 计数、每轮通信、离线材料和时间边界。
2. 记录每次运行实际 `e_A(n,r)`、`v_A(n,r)`。
3. 对 `r=2,3,4,5` 运行统一矩阵。
4. 分别完成 LAN/WAN 实验。
5. 每配置预热一次、正式五次，汇总 median/min/max。
6. 与对应全对全基线比较完整端到端成本。
7. 将比较量下降与额外轮数、材料、路由及适配成本一起报告。
8. 保存所有配置状态、失败原因和覆盖范围。

满足 Theorem 5.1 对应构造时，可将实测节点、边及通信与其理论项并列；其他组合使用明确标记的独立推导。

#### 交付物

- 两种 AAV86 升级实现；
- 图算法、预处理、泄露和轮数设计；
- conformance、差分和独立进程 E2E；
- 完整适用指标与逐次结果；
- LAN/WAN 性能报告；
- 未完成配置清单；
- 供 M6B 复用的接口与计量说明。

#### 退出条件

- 两种路线均完成实现与正确性、安全审计；
- 计量能力通过核验；
- 成功运行配置的全部适用指标齐全；
- 统一矩阵各配置均有成功或明确失败记录；
- 比较结论与实际覆盖范围一致；
- 报告经另一方复核；
- M6A 实现与性能验收完成后，才进入 M6B 依赖实现和正式实验。

### 4.8 M6B：BB90+DCF 两种升级与完整性能验收

#### 前置条件

M6A 两种路线的实现、性能验收及交接完成。

BB90 原文研究和明文算法设计可以提前进行，但不能以此跳过 M6A 的性能阶段。

#### 设计阶段

1. 固定 BB90 采用版本、适用范围、迭代参数及概率保证。
2. 明确从算法顺序统计量到本项目第 K 大稳定优先级的映射。
3. 明确随机成本界、失败事件与输出正确性条件。
4. 给出比较图到 CA 的转换。
5. 定义第 K 大阈值的秘密共享表示。
6. 设计在线阈值与预生成 DCF 材料的衔接。
7. 分别设计 Protocol I、Protocol III 路线的完整阶段。
8. 推导消息依赖和总轮数，不预先套用 AAV86 公式。

#### 实现阶段

分别实现：

```text
Protocol I + BB90+DCF
Protocol III + BB90+DCF
```

完整功能为：

```text
BB90 得到稳定第 K 大阈值
  → DCF 生成成员指示共享
  → 必要的路由、逆映射和共享转换
  → 原顺序 Top-K mask
```

“投票”表示每个位置是否属于 Top-K 的秘密共享成员指示，不增加明文投票公开步骤。

仅以原始 score 与第 K 大 score 做 `>=` 比较无法保证重复值情况下恰选 K 个。必须沿用稳定同分语义，使阈值比较与冻结 oracle 完全一致。

不得将选出阈值本身写成完整 Top-K 协议，也不得将历史 Direct Top-K 原型仅改名后作为 BB90 实现。

#### 正确性与安全验收

- BB90 选择结果与稳定顺序统计量 oracle 一致；
- DCF 成员选择与完整 Top-K oracle 一致；
- 覆盖随机、重复值、全相等、边界、非二次幂及 K 边界；
- 阈值、selected index 和最终 mask 不通过测试路径进入 secure transcript；
- 在线阈值相关预处理不引入未声明 Dealer 参与；
- 两种组合各自的材料、公开值和轮数经过审计。

#### 计量与性能阶段

分别计量：

1. BB90 选择；
2. DCF 成员选择；
3. 输入、路由、逆映射和 mask 适配；
4. 完整端到端路径。

记录实际迭代数、节点和边计数、DCF/uCMP/PRG 调用、离线材料、每轮通信和时间。

对统一 `(n,K)` 矩阵及已确定的 BB90 参数进行 LAN/WAN 完整性能实验。遵循预热一次、正式五次及 median/min/max 规则。

与对应全对全和 AAV86 方案比较，不能只报告 BB90 图内部成本而省略最终 DCF 选择。

#### 交付物

- 两种 BB90+DCF 组合实现；
- 算法版本、阈值表示及预处理设计；
- 正确性、泄露和消息审计；
- 全部适用性能指标；
- 逐次结果、汇总报告和失败配置；
- 六种方案共同比较所需的接口与数据说明。

#### 退出条件

- 两种路线均完成完整 Top-K mask 路径；
- 稳定同分和恰好 K 个成员条件通过；
- 安全模型和概率保证明确；
- 成功配置全部适用指标齐全；
- 全矩阵配置状态和限制有记录；
- 阈值、DCF 和适配成本均计入；
- 报告经过交叉复核后进入 M7。

### 4.9 M7：六种方案统一性能报告

M7 汇总前述已完成结果，不把 M6A、M6B 的性能验收推迟到此阶段。

#### 主比较组

| 比较 | 目的 |
| --- | --- |
| I vs III | 比较全对全精确核心路线 |
| I vs I+AAV86 | 评估 I 路线的图排序升级 |
| III vs III+AAV86 | 评估 III 路线的图排序升级 |
| I vs I+BB90+DCF | 评估 I 路线的选择升级 |
| III vs III+BB90+DCF | 评估 III 路线的选择升级 |
| I+AAV86 vs I+BB90+DCF | 相同路线比较两种升级算法 |
| III+AAV86 vs III+BB90+DCF | 相同路线比较两种升级算法 |
| I+AAV86 vs III+AAV86 | 相同图算法比较组合路线 |
| I+BB90+DCF vs III+BB90+DCF | 相同选择算法比较组合路线 |

工程对照另列：

- I 四轮核心与三轮精确核心；
- III 三轮模块化核心与两轮精确核心；
- core、raw adapter、mask adapter 和封装成本；
- 必要且真实执行的 shuffle backend 专项对照。

CipherGPT 不进入本轮报告比较组。

#### 任务

1. 对齐六种方案的 revision、输入、环境、输出和计量边界。
2. 对实现或环境变化导致不可比的配置补跑。
3. 汇总全部长期矩阵配置及失败原因。
4. 展示 offline、online、total、per-party、轮数、PRG、边数和总时间。
5. 分开呈现理论、历史、当前实测和未测。
6. 报告性能拐点、资源限制及结论适用范围。
7. 保留两次基础通信核验的公式与差异解释。
8. 完成交叉审计和可复跑说明。

#### 退出条件

- 六种方案身份与实际完成情况明确；
- 比较功能、参数、环境和统计口径可比；
- 图表与数字可追溯到逐次记录；
- 不将不同安全模型放入无说明的统一速度排名；
- 未完成配置和缺失指标完整披露；
- 不以外推数据填补实测矩阵；
- 不因已写报告而把未实现或未验收方案标为完成。

CryptoMoE 在 M7 之后另行确定 eligibility、dummy、容量和允许公开的 transcript，不属于本轮退出条件。

## 5. 两人协作与接口所有权

### 5.1 角色 A：搭档

未来 gated 主责（当前不得执行 strict G1、官方 G2、G3 或 M5 runtime）：

- M2 精确三轮核心；
- Protocol I 相关测试、通信测量和差异解释；
- 公共接口交接；
- M5 的交叉评审。

后续建议分工：

- M6A/M6B 公共比较材料、预处理及 Protocol I 路线；
- Protocol I 及其两种升级的性能运行；
- 材料生成、Dealer 和通信边界审计。

不再承担 M4 CipherGPT 任务。

### 5.2 角色 B：你

未来 gated 主责（当前不得执行 strict G1、官方 G2、G3 或 M5 runtime）：

- 总体计划、实施计划和分工文档修订；
- M5 域、编码、消息依赖及失败用例准备；
- 接收并复跑 M2 交接；
- M5 精确两轮核心、通信核验与交接。

后续建议分工：

- M6A/M6B 的 Protocol III 路线；
- 统一输出与 Protocol III 适配；
- Protocol III 及其两种升级的性能运行；
- 结构化结果与跨协议对照。

M3 已完成，不重新安排为当前实现任务。

### 5.3 双方共同负责

- 公共图算法、oracle 和材料契约评审；
- 两次通信核验的交叉复跑；
- M6A/M6B 安全设计和阶段退出评审；
- 全部指标的计量口径；
- M7 汇总与结果审计。

图算法实现、公共计量及共享文件在每个阶段指定一名实际写入负责人，另一方评审；不同时维护两份不同实现语义。

详细到文件和 PR 的分工由更新后的 `docs/TEAM_WORK_PLAN.md`、`docs/M3_ONWARD_TEAM_WORK_PLAN.md` 承接。

### 5.4 共享资产

```text
VFSS/include/moe_topk/score_semantics.h
VFSS/include/moe_topk/topk_oracle.h
VFSS/include/moe_topk/metrics.h
docs/decisions/M1_SCORE_SEMANTICS.md
PROJECT.md
docs/IMPLEMENTATION_PLAN.md
docs/TEAM_WORK_PLAN.md
docs/M3_ONWARD_TEAM_WORK_PLAN.md
README.md
```

规则：

- score、tie rule、oracle 和输出语义保持冻结。
- 计量能力可按真实需求扩展，但同步说明影响，不静默改变旧字段。
- 同一共享文件同一区域不同时修改。
- 两条协议线不复制另一套 rank、输入生成或指标定义。
- 跨协议公共接口在真实调用需求出现后提取。
- 角色 A 不顺带修改 III 路由，角色 B 不顺带修改 I shuffle。
- 必要的公共变更独立成 PR，并运行受影响回归。
- 不修改 `VFSS-baseline/`。

### 5.5 交接记录要求

每次 G3 交接至少记录：

| 项目 | 内容 |
| --- | --- |
| 基准 | revision、分支、实现标签、构建配置 |
| 功能 | 输入、输出、公开参数、错误语义 |
| 表示 | score、rank、payload、logical/padded layout |
| 材料 | 生成方、时机、绑定、一次性消费 |
| 通信 | transport、session、阶段及消息顺序 |
| 计量 | core/adapter、total/per-party、计时边界 |
| 验证 | conformance、差分、E2E、轮数、通信报告 |
| 使用 | 最小调用示例与复跑命令 |
| 限制 | 已知缺口和不可直接复用的假设 |
| 接收 | 接收方复跑结果与日期 |

交接记录是可运行证据，不以“接口已经写好”的口头说明代替。

## 6. 分支与合并顺序

### 6.1 基本规则

- 从最新 main 建立短生命周期分支。
- 一个 PR 只处理一个明确实现步骤或治理变更。
- 不在一个 PR 中混合 Protocol I、Protocol III 和下一种升级算法的无关改动。
- 文档与设计可以提前合并，依赖 runtime 的验收服从阶段门。
- 原始密钥、日志、论文、构建产物和参考工程不进入普通 Git 历史。
- 计划中的目标名称不代表已有 executable 或已完成状态。

### 6.2 当前合并顺序

1. 合并本次范围、路线和分工文档。
2. 按依赖合并 M2 精确功能及相关测试。
3. 合并 M2 通信计量、核验报告和必要修正。
4. 完成 M2 G3 接口交接。
5. 合并 M5 所需域与适配，再合并精确核心和 E2E。
6. 合并 M5 通信核验及必要修正。
7. 完成 M5 G3 基础接口交接。
8. 合并 M6A 设计、必要公共接口和两种升级实现。
9. 完成 M6A 计量与完整性能报告。
10. 合并 M6B 设计、两种组合实现及验证。
11. 完成 M6B 完整性能报告。
12. 完成 M7 汇总和必要补跑。

每阶段可以拆多个 PR；前置阶段未通过时，不以“已经合并”绕过验收门。

M4 不再出现在合并顺序中。旧 M2→M3 交接作为已完成历史保留，不与当前 M2 精确核心→M5 交接混淆。

## 7. 每个合并请求的检查项

- 对应哪个里程碑、步骤和阶段门？
- 实现标签及证据层级是什么？
- 输入、输出、party 角色和允许公开值是否变化？
- 是否改变预处理、代数表示、材料绑定或消费方式？
- 核心与端到端轮数如何计算？
- 是否包含 raw-score 和 mask 适配成本？
- 新增或修改了哪些测试，实际命令和结果是什么？
- 通信是否保留 total、per-party 和分方计数？
- 适用阶段是否附通信核验或性能矩阵报告？
- 未测指标与失败配置是否如实记录？
- 是否影响另一条协议线或公共接口？
- 文档状态是否与 main/分支实际状态一致？
- `VFSS-baseline/` 是否保持不变？
- 是否避免提交参考工程、论文、密钥、日志和构建物？
- 是否运行相关回归及 `git diff --check`？

文档 PR 不声称重新执行历史测试。新性能结果必须有自己的 revision、输入、环境、命令和原始计数。

## 8. 立即下一步

### 8.1 角色 B 当前任务

1. 完成 `PROJECT.md`、本文及两份分工计划的同步修订。
2. 更新首页和路线决策，清除仍指向 M4 的现行执行要求。
3. 保留 M2/M3 历史验收和实现标签。
4. 整理 M2→M5 交接表及两次通信核验模板。
5. 准备 M5 域、非零编码、DPF 兼容性和两轮消息设计。
6. 不修改任何尚未解锁的 strict Protocol I 核心或 shuffle 工作。

### 8.2 角色 A future gated 任务（当前不得执行）

strict G1、官方 G2、G3 与 M5 runtime 保持未来 gated 责任；当前仅允许 NON-EXACT 实验、证据准备和 canonical gate 允许的 design-only 工作。

### 8.3 双方立即共同确认

- 本轮目标是 I 三轮核心、III 两轮核心。
- I 四轮工程基线和 III 三轮工程基线继续保留。
- M2、M5 均执行“实现→通信核验→交接”。
- AAV86 两种路线完成完整性能验收后再推进 BB90+DCF。
- 六种方案共用语义和全部指标。
- CipherGPT 实施和性能任务已取消。
- 分支进度不能直接写成 main 已完成。
- 尚未测量的数据继续标记 `NOT_MEASURED`。

## 9. 历史证据索引

本节保留旧计划中的实现基础和验收事实。它们不是本次修改文档重新运行得到的结果，也不构成精确核心已经完成的声明。

### 9.1 M2.7：CmpAgg 三进程基础

实现标签：

```text
m2_priority_cmpagg_three_process_e2e
```

P2 为公开 canonical graph 生成 node-mask shares 和分方 uCMP/DCF 材料，发送后退出。测试控制器在离线屏障后提供 priority-key shares；重构与 oracle 检查只在测试层执行。

历史测试覆盖 `n=1,2,5,7,11`、K 边界、重复值、非二次幂、正负边界及 package/transport 错误矩阵。

记录的 Ubuntu 干净 Debug 构建为 11/11 CTest 通过。见：

`docs/reproduction/M2_CMPAGG_PROCESS_E2E_UBUNTU_2026-09-05.md`

### 9.2 M2.8–M2.14：工程组件与端到端路径

| 阶段 | 已有实现或证据 |
| --- | --- |
| M2.8 | EMP chosen OT、connected-fd adapter 和独立进程 conformance |
| M2.9 | GGM OPV 与 Share Translation |
| M2.10 | 单遍 Permute+Share |
| M2.11 | 两遍前向/逆向 shuffle roundtrip |
| M2.12 | priority-key 输入、shuffle、CmpAgg、受控 shuffled rank 与 reverse carrier 的小规模 E2E |
| M2.13 | 六轮模块化 mask 路径、离线屏障、有界帧、layout 和 rank-permutation 审计 |
| M2.14 | raw-score carry/lift/sign 适配后的八轮路径 |

主要决策：

- `docs/decisions/M2_CHOSEN_OT_DEPENDENCY.md`
- `docs/decisions/M2_OPV_SHARE_TRANSLATION.md`
- `docs/decisions/M2_PERMUTE_SHARE.md`
- `docs/decisions/M2_SECRET_SHARED_SHUFFLE.md`
- `docs/decisions/M2_PROTOCOL_I_SMALL_E2E.md`

主要复现记录：

- `docs/reproduction/M2_PERMUTE_SHARE_UBUNTU_2026-09-06.md`
- `docs/reproduction/M2_SECRET_SHARED_SHUFFLE_UBUNTU_2026-09-06.md`
- `docs/reproduction/M2_PROTOCOL_I_SMALL_E2E_UBUNTU_2026-09-06.md`

这些组件是后续复用基础，不自动证明精确 shuffle 功能或自适应图预处理已经实现。

### 9.3 M2.15/M2.16：精确核心差距审计

M2.15 确认当时的 PS API 无法表达论文需要的同置换 public masked shuffled list，因此保留四轮核心、八轮总路径。

M2.16 进一步审计 `pi(x)+r`、掩码秘密性和 GRank 关联，未新增精确原语。文档随 `f800f96` 合入 main。

三轮核心、七轮总路径在这些记录中仍为未实现候选。相关文件：

- `docs/decisions/M2_PROTOCOL_I_PAPER_CORE_ALIGNMENT.md`
- `docs/decisions/M2_PROTOCOL_I_PAPER_EXACT_3ROUND_DESIGN.md`
- `docs/decisions/M2_PROTOCOL_I_EXACT_LEAKAGE_AUDIT.md`
- `docs/reproduction/M2_PROTOCOL_I_PAPER_CORE_ALIGNMENT_UBUNTU_2026-09-06.md`
- `docs/reproduction/M2_PROTOCOL_I_PAPER_EXACT_3ROUND_UBUNTU_2026-09-06.md`

本次重新推进该目标，不反写这些历史结论。

### 9.4 M2 验证可靠性关闭

chosen-OT readable-hangup 和 modular E2E FD-lifecycle 修复已合入，未改变协议图。

记录的 Ubuntu 24.04.4 fresh 构建在 soft `RLIMIT_NOFILE=1024` 下：

- EMP-ON：19/19；
- EMP-OFF：13/13；
- `(128,2/8)`、`(256,2/8)` 及重复矩阵通过；
- 相关完整矩阵还在 4096 下通过。

这说明对应工程验证不依赖将 FD limit 提高到 4096，不表示论文精确条件或网络性能已验收。

见：

- `docs/reproduction/M2_CHOSEN_OT_POLLHUP_UBUNTU_2026-09-06.md`
- `docs/reproduction/M2_MODULAR_E2E_FD_LIFECYCLE_UBUNTU_2026-09-06.md`

### 9.5 M3 关闭记录

M3 在 `main@bb0d0e8` 完成整改，保留三轮 priority-key 和五轮 raw-score 两个入口。

已完成 logical-n GRank、DPF routing、secure combine、raw-score 安全入口、独立进程测试、两个正式 Party executable 和 MetricsRecord 汇总。

正式 Party 生成各自 report，TEST_ONLY 控制器汇总完整 MetricsRecord；不能写成每个 Party executable 都独立生成全局结果。

完整证据及 11-test 验证口径见：

`docs/reproduction/M3_REVIEW_CLOSEOUT_UBUNTU_2026-09-10.md`

历史计时包括已披露的 Dealer 生命周期、输入分发和 report 收集边界；网络 bandwidth/RTT 及完整在线 PRG 计数仍按原记录保留未测状态。

后续正式实验新增分阶段计量，不静默替换历史数字或证据来源。
