# 双人实施分工与交接计划

状态：**已采纳；当前状态按 2026-09-25 更新**。

本文依据 `PROJECT.md` 和 `docs/IMPLEMENTATION_PLAN.md`，明确双人职责、并行边界、公共文件所有权及交接条件。

本次修订保留已完成的 M1/M1.1、M2 工程基线和 M3 模块化基线，不改变冻结的 score、tie-break、rank 和原顺序 Top-K mask 语义。

当前主线为 **M2 COMPLETED → M5 IN PROGRESS**。M2 已完成 current engineering acceptance：three-round C-INSTANTIATION implemented、independent three-round review PASS、online logical communication matches Theorem 4.1、Protocol I PR merged；`AUTHOR_EXACT = NOT_PROVEN` 保留为证据边界。dated strict-gate 记录属于历史，不再阻塞 M5 runtime。
M5-B–G 已在工作分支完成；M5-H1 通信按数量级门槛通过，精确 Theorem 4.2 逻辑式仍差 `254n` bits。M5-H2 independent review 是下一阶段；M5 整体仍 IN PROGRESS。见 [M5-H1 evidence](reproduction/M5_PROTOCOL_III_2ROUND_ONLINE_COMMUNICATION_UBUNTU_2026-09-25.md)。

## 1. 当前分工与执行顺序

### 1.1 固定角色

| 角色 | 当前主责 | 后续主责 |
| --- | --- | --- |
| 角色 A（搭档） | 维护 M2 已完成资产并评审 CmpAgg/rank-share 复用 | Protocol I 路线的升级实现与性能测试；公共比较材料和预处理 |
| 角色 B（Protocol III 负责人） | M5 Protocol III two-round path | 基于交接接口推进通信核验及对应升级 |
| 双方共同 | 交叉评审、交接复跑、公共契约核对 | 图算法设计、计量审计、阶段验收和最终报告 |

角色 A 不再承担 CipherGPT 实施或性能测试任务。角色 B 不重新实现已经完成的 M3 三轮基线。

### 1.2 当前路线

```text
M2 Protocol I：COMPLETED
  → 角色 B：M5 Protocol III two-round path（IN PROGRESS）
  → 实现与正确性验收
  → M5-H2 独立评审与精确成本差额审计
  → 双方冻结基础接口与计量结果
  → AAV86 两种升级实现及完整性能验收
  → BB90+DCF 两种升级实现及完整性能验收
  → 六种方案统一报告
```

Protocol I 的论文核心目标为三轮，Protocol III 为两轮。输入适配和原顺序 mask 输出适配的轮数、通信及时间单独记录，并计入端到端主结果。

### 1.3 里程碑边界

- M1/M1.1：已完成，保持冻结。
- M2：**COMPLETED / 已完成**；three-round C-INSTANTIATION implemented、independent review PASS、Theorem 4.1 online logical communication match、Protocol I PR merged；`AUTHOR_EXACT = NOT_PROVEN`。
- M3：三轮模块化核心及 raw-score 五轮扩展已完成。
- M4：取消，不再作为任何后续阶段的前置条件。
- M5：**IN PROGRESS / 正在进行**；shared clique CmpAgg / GRank → DPF routing → two-round composition，复用既有 ranking core。
- M6A：Protocol I、Protocol III 的 AAV86 升级及完整性能验收。
- M6B：Protocol I、Protocol III 的 BB90+DCF 升级及完整性能验收。
- M7：六种方案统一汇总与报告。

历史 M2 状态记录保留；当前 M5 可复用已完成 M2 的 CmpAgg/rank-share handoff，不重新实现 ranking core。

## 2. 角色 A：Protocol I 精确核心与通信核验

### 2.1 future gated strict 实现责任（当前不得执行）

角色 A 在 strict G1 evidence available 后负责：

1. 固定当前开发 revision、候选标签和实际实现范围。
2. 基于 M2.15/M2.16 审计，闭合 paper-compatible public masked-list shuffle 功能。
3. 验证秘密共享 `pi(x)`、公开 `pi(x)+r` 与后续 GRank 材料的同置换和掩码关联。
4. 保证 r 对任一单方未知，预处理输入无关，Dealer 在线静默。
5. 接入全对全 CmpAgg、稳定 rank 和 payload 路由。
6. 保留 raw-score 输入与原顺序 XOR Top-K mask 输出。
7. 实现并审计三轮论文核心的因果消息依赖。
8. 完成 conformance、oracle differential、独立进程 E2E 和相关异常测试。
9. 保留现有四轮核心、八轮总路径工程基线用于回归。

不得使用模拟 shuffle、公开置换、测试端构造 masked list 或额外事后交换，冒充论文所需功能。

### 2.2 通信核验职责

实现与正确性验收通过后，角色 A 负责：

- 冻结被测 revision 和实现标签；
- 测量各方、各阶段实际发送与接收量；
- 区分论文核心、raw-score adapter、mask adapter 和传输封装；
- 对照 Theorem 4.1 的功能、参数与成本公式；
- 检查 logical/padded 规模、比较位宽、rank 域、payload 和序列化差异；
- 解释论文公式、实现消息推导与实际通信之间的偏差；
- 提供复跑命令、原始计数及核验报告。

核验至少覆盖小规模边界、`(128,2/8)`、`(256,2/8)`，并使用可行规模检查增长趋势。不能只用单个配置的数量级作为通过依据。

### 2.3 向角色 B 交付

角色 A 提供：

- 可复跑 revision、构建配置和依赖说明；
- 公共接口与最小调用示例；
- 输入、输出、rank 和 layout 契约；
- 材料生成、分发、绑定与消费规则；
- transport、session、fingerprint 和错误语义；
- 分阶段计量接口及字段含义；
- 正确性、轮数、泄露与通信核验报告；
- 与 M3/M5 的兼容范围；
- Protocol I 专用接口及不可直接复用的假设。

交接不要求 Protocol III 使用 Protocol I 的 shuffle。必须区分公共基础能力与 Protocol I 专用路由。

### 2.4 主要修改边界

角色 A 主写：

```text
VFSS/include/moe_topk/      Protocol I 专用接口与当前 M2 公共基础
VFSS/src/moe_topk/          Protocol I、shuffle、比较材料和相关运行绑定
VFSS/tests/moe_topk/        Protocol I 及对应公共部件测试
docs/decisions/             Protocol I 设计与安全边界
docs/reproduction/          Protocol I 测试和通信核验记录
```

角色 A 不顺带修改 Protocol III 路由和域适配；涉及公共契约的变更按第 5 节执行。

## 3. 角色 B：计划修订、接收交接与 Protocol III

### 3.1 可立即开展的工作

在角色 A 推进 Protocol I 的同时，角色 B 负责：

1. 修订 `PROJECT.md`、详细实施计划和两份分工计划。
2. 同步首页、路线决策及配套资料说明，清除现行 M4 前置要求。
3. 整理通信核验模板和接口交接清单。
4. 核对论文版本、协议映射、输入输出和轮数口径。
5. 准备 Protocol III 域表示、非零编码、DPF 兼容性和两轮消息设计。
6. 梳理 M3 已完成接口与 M5 所需新增能力。
7. 准备边界向量、失败用例和验收方案。
8. 评审角色 A 的接口、消息表及通信核验方法。

这些工作不要求等待 Protocol I 全部完成，但不能修改任何尚未解锁的 strict shuffle 或公共材料实现，也不能基于未冻结接口宣称 M5 已完成。

### 3.2 接收 M2 交接

角色 B 负责：

1. 检查交接资料是否完整。
2. 在交接 revision 上构建并运行最小调用示例。
3. 复跑代表性正确性和通信核验配置。
4. 核对 input/rank/mask、材料、transport 和计量契约。
5. 明确哪些部件直接复用、哪些需要适配、哪些不属于 Protocol III。
6. 记录复跑结果、差异和接收日期。
7. 在交接问题解决后，基于冻结 revision 推进 M5。

不能仅凭“接口已合并”认定交接完成，也不能复制一份公共代码绕开接口问题。

### 3.3 M5 实现职责

交接完成后，角色 B 负责：

- 选择并记录满足论文条件的域表示；
- 定义非零 payload、乘法掩码、零值编码和逆元失败语义；
- 核对 VFSS DPF 接口与目标代数表示的兼容性；
- 实现 GRank 与 DPF routing 的跨阶段压缩；
- 验证论文核心两轮消息依赖；
- 保留统一 raw-score 输入和原顺序 mask 输出；
- 与 M3 使用同一输入、K、种子和 oracle 做差分；
- 完成 conformance、独立进程 E2E、泄露和材料审计。

不能把 `Z_(2^b)` 直接视为域。仅针对单位 payload 或 mask 输出的删轮特化，不能直接标为 Theorem 4.2 精确复现。

### 3.4 M5 通信核验与交接

角色 B 负责：

1. 对照 Theorem 4.2，注明共用掩码等优化是否采用。
2. 分别测量核心、输入适配、表示转换和 mask 输出。
3. 比较 M3 与 M5 的轮数、通信及适配开销。
4. 解释论文公式、实现消息和实测计数之间的差异。
5. 提供原始结果与可复跑命令。
6. 向角色 A 交付用于交叉核验的配置。
7. 在核验完成后，与角色 A 共同冻结进入 M6A 的基础接口。

### 3.5 主要修改边界

角色 B 主写：

```text
VFSS/include/moe_topk/      Protocol III、域与 DPF routing 适配接口
VFSS/src/moe_topk/          Protocol III 及对应运行绑定
VFSS/tests/moe_topk/        Protocol III、域适配、差分和 E2E
docs/decisions/             Protocol III 代数、消息、泄露和轮数设计
docs/reproduction/          Protocol III 测试和通信核验记录
```

角色 B 不顺带修改 Protocol I shuffle，也不为推进样例修改冻结 oracle、score 或 tie rule。

## 4. 当前 M2 → M5 交接契约

### 4.1 三个阶段门

当前交接严格执行：

```text
G1：协议实现与正确性完成
  → G2：通信测量及差异解释完成
  → G3：公共接口及证据交接完成
```

| 阶段门 | 主责 | 交叉核验 | 完成条件 |
| --- | --- | --- | --- |
| G1 | 角色 A | 角色 B | 正确性、消息、材料和安全边界通过 |
| G2 | 角色 A | 角色 B | 实际计数可信，关键差异已解释 |
| G3 | 角色 A 交付、角色 B 接收 | 双方 | 接收方可复跑并确认接口 |

代码可以分批合并，但“已合并”“能运行”“通信已打印”不分别等于 G1、G2、G3 已通过。

M5 完成后沿用同样三阶段结构，由角色 B 主责交付、角色 A 交叉核验，再进入 M6A。

M2 已完成当前工程验收并完成 M2→M5 handoff；M5 runtime 为当前活动工作。`AUTHOR_EXACT = NOT_PROVEN` 仍保留为 M2 证据边界。

### 4.2 输入、rank 和输出

保持以下契约：

- Q20.12 signed score 算术共享；
- `raw=(x0+x1) mod 2^32`；
- score 降序、original index 升序；
- 最高优先级 rank 为 0；
- 有效 rank 范围 `0..n-1`；
- 原始输入顺序下长度 n 的 XOR Top-K mask shares；
- 重构后每位为 0/1，且恰有 K 位为 1。

交接必须注明实际接口接收 raw-score shares 还是 priority-key shares，不能用同一个名称混淆两种入口。

同样需要区分 `logical_n`、`padded_n`、比较图覆盖范围和 DPF 域。

### 4.3 材料与运行接口

逐接口说明：

- 输入和输出类型；
- 生成方与调用方；
- 所需公开参数；
- 材料生成时机；
- 材料与 session、party、位宽、位置和阶段的绑定；
- 一次性消费和重复使用拒绝方式；
- Dealer 退出条件；
- transport 和错误传播；
- 超时、截断、peer 提前退出等失败语义。

既有全对全 CmpAgg 材料不能直接描述为自适应图材料。ring-only 接口也不能直接描述为满足 M5 的 field 接口。

### 4.4 计量接口

交接必须提供：

- 每方发送和接收计数；
- 分阶段通信与总量的对应关系；
- total/per-party 派生方式；
- core、raw adapter、mask adapter 和封装边界；
- 在线因果轮数的消息依据；
- 离线时间与材料定义；
- 在线时间起止位置；
- PRG 与比较边计数的含义及可信范围；
- 未测字段和原因。

total 只累加在线方发送量，received 用于核验，不再次计入总通信。

理论成本、历史记录和当前实测分别保留。数量级相符不替代正确性、安全性和消息审计。

### 4.5 交接记录格式

| 项目 | 必填内容 |
| --- | --- |
| 基准 | revision、实现标签、构建配置 |
| 功能 | 输入、输出、公开参数、错误语义 |
| 表示 | score、rank、payload、logical/padded layout |
| 材料 | 生成、绑定、分发和一次性消费 |
| 通信 | transport、session、消息和阶段 |
| 计量 | 核心与适配、total/per-party、计时边界 |
| 验证 | conformance、差分、E2E、轮数和通信核验 |
| 使用 | 最小调用示例与复跑命令 |
| 限制 | 已知缺口和不可复用的假设 |
| 接收 | 接收方结果、差异处理和日期 |

有未解决的关键接口、消息或成本差异时，记录具体问题，暂不标记交接完成。独立资料研究和设计准备可以继续。

## 5. 公共接口、计量接口与共享文档所有权

### 5.1 所有权含义

“主写负责人”负责协调该文件或接口的实际修改、相关测试和兼容说明；另一方负责交叉评审。

所有权用于避免并行冲突，不代表负责人可以单方面修改冻结语义。涉及输入、输出、安全模型或指标定义的变化，必须由双方复核并同步决策。

### 5.2 当前主写与评审安排

| 资产 | 主写负责人 | 评审负责人 | 修改规则 |
| --- | --- | --- | --- |
| Protocol I 专用接口、shuffle 和 runtime | A | B | B 不直接并行修改 |
| Protocol III 专用接口、DPF routing 和域适配 | B | A | A 不直接并行修改 |
| M2 来源的 CmpAgg、材料、transport、session 基础 | A | B | B 提交需求，由 A 协调公共修改 |
| `score_semantics.h`、`topk_oracle.h`、冻结输入语义 | 保持冻结 | 双方 | 无普通开发修改任务；必要变更独立决策 |
| `metrics.h` 和公共计量契约 | A | B | 当前由 A 统一写入，跨阶段不自动转移所有权 |
| Protocol I 阶段计数与 report | A | B | 遵循公共计量契约 |
| Protocol III 阶段计数与 report | B | A | 遵循公共计量契约 |
| 通用结果格式、汇总规则及通信核验模板 | B | A | 与公共 metrics 同步，不定义第二套字段含义 |
| 总纲、实施计划、两份分工计划、README、路线决策 | B | A | 本轮由 B 统一修订和汇总状态 |
| Protocol I 设计及复现记录 | A | B | 保留历史记录，新结果绑定新 revision |
| Protocol III 设计及复现记录 | B | A | 保留历史记录，新结果绑定新 revision |
| 公共构建、CTest 注册和公共实验脚本 | 按具体 PR 指定一人 | 另一方 | 不同时修改同一区域 |

现有文件：

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

表中的通用模板或脚本若尚不存在，按实际需要新增；不能将规划中的文件写成已有实现。

### 5.3 公共接口变更流程

1. 使用方说明需求、受影响调用点及必要性。
2. 主写负责人评估能否通过现有接口或最小适配解决。
3. 双方明确是否影响另一条协议线、材料或计量。
4. 由一人提交最小公共变更，附兼容说明及受影响测试。
5. 另一方复核后再接入协议实现。
6. 更新交接 revision 和调用示例。

不得通过复制公共代码、局部改 tie rule 或修改旧 ABI 绕开流程。

交接之后，公共接口变化仍需同步双方；“已交接”不意味着后续可以无说明地破坏兼容性。

### 5.4 计量接口变更流程

- 公共字段含义保持与 `PROJECT.md` 一致。
- 新增指标必须注明单位、统计对象、计数方和阶段边界。
- 单方计数与全局汇总区分，不能在每方 report 中重复填全局总数再相加。
- 无可信观测的字段保持 `NOT_MEASURED`。
- PRG 调用、DCF 调用和比较边数不得互相替代。
- 改变计时或通信边界时，使用明确的结果版本或标签说明。
- 不静默重解释历史数据。

角色 B 可以提出 M5 所需字段并修改 III 专用 report，但公共 `metrics.h` 的写入由角色 A 协调；若需要转交主写职责，应先在双方分工记录中明确。

### 5.5 共享文档变更流程

本轮由角色 B 统一修订共享计划，角色 A 提供 Protocol I 当前状态、接口和测试证据。

实现 PR 如需更新共享文档：

1. 主责方提供准确的状态与证据。
2. 与角色 B 确认当前是否存在同区域文档修改。
3. 指定一个实际写入者。
4. 合并后同步其他入口，避免“当前状态”和“立即下一步”冲突。

角色 B 不能根据口头进度将搭档分支写成 main 已完成。角色 A 也不应在实现 PR 中顺带恢复旧的 M4 前置关系。

### 5.6 禁止修改和提交的内容

- 不修改 `VFSS-baseline/`。
- 不提交论文、生成密钥、日志、构建产物和完整参考工程。
- 不从旧工程复制 FSS ABI 或密钥内存布局。
- 不把测试重构接入 secure runtime。
- 不覆盖历史实验数字或将其改成新阶段结果。

## 6. 后续两条升级路线的双人安排

### 6.1 M6A：AAV86

M2、M5 的实现、通信核验和交接完成后，先做：

```text
Protocol I + AAV86
Protocol III + AAV86
```

默认分工：

| 工作 | 主责 | 评审 |
| --- | --- | --- |
| 公共图接口、比较材料与自适应预处理 | A | B |
| Protocol I 组合实现和对应实验 | A | B |
| Protocol III 组合实现和对应实验 | B | A |
| 统一输出、结果汇总和对照报告 | B | A |
| 算法来源、泄露、轮数与阶段退出 | 双方 | 交叉核验 |

公共图算法只保留一份明确语义。AAV86 的 Protocol III 组合必须单独说明消息与安全条件，不能直接套用 shuffle-based compiler 结论。

M6A 包含两种实现的完整性能验收：统一矩阵、`r=2..5`、LAN/WAN、全部指标、逐次结果和失败配置记录。不能只完成算法就转入 M6B。

### 6.2 M6B：BB90+DCF

M6A 完整性能验收完成后，推进：

```text
Protocol I + BB90+DCF
Protocol III + BB90+DCF
```

默认分工：

| 工作 | 主责 | 评审 |
| --- | --- | --- |
| BB90 公共图/选择接口、比较材料及 DCF 衔接 | A | B |
| Protocol I 组合实现和对应实验 | A | B |
| Protocol III 组合实现和对应实验 | B | A |
| 稳定阈值、原顺序 mask 和结果汇总 | B | A |
| 算法版本、概率保证、材料与泄露审计 | 双方 | 交叉核验 |

必须完成“稳定第 K 大阈值→DCF 成员选择→原顺序 mask”的整个路径。

重复值情况下不能仅比较 `score >= 第 K 大 score`，必须保证与冻结 tie rule 一致且恰选 K 个位置。旧 Direct Top-K 原型不能直接改名为 BB90。

BB90 参数单独定义，不直接沿用 AAV86 的 r。完整性能测试必须包含阈值选择、DCF 和输出适配成本。

### 6.3 M7：联合报告

双方分别对自己负责的三种方案提供可追溯结果：

- A：Protocol I、I+AAV86、I+BB90+DCF。
- B：Protocol III、III+AAV86、III+BB90+DCF。

B 负责统一汇总，A 复核参数、材料、通信和比较范围。双方共同确认六种方案的功能、安全边界和计量口径可比。

M7 不补充 CipherGPT 实施任务，也不把 M6A/M6B 的全部性能测试推迟至最终报告阶段。

## 7. 并行与合并规则

### 7.1 允许提前并行的工作

- 论文和参考来源核对；
- 阶段、消息和接口设计；
- 明文算法 oracle 与失败用例准备；
- 实验矩阵、指标模板和报告设计；
- 不改变未冻结 runtime 的独立验证。

提前设计不等于绕过前置验收，也不表示后续协议已经实现。

### 7.2 需要等待交接的工作

- 基于未冻结公共接口的 M5 核心接入；
- 依赖 M2/M5 精确身份的升级实现验收；
- 使用尚未核验计数器的正式性能结论；
- M6A 性能验收前的 M6B 依赖实现和正式实验。

### 7.3 分支与合并

- 两人从最新 main 建独立短生命周期分支。
- 一个 PR 只覆盖一个可验证步骤或明确治理变更。
- 公共改动与专用协议改动尽量按依赖拆分。
- 不同时修改共享文件同一区域。
- 提交前运行相关测试和 `git diff --check`。
- 文档状态只在证据支持时更新。

当前合并顺序：

```text
计划与分工修订
  → M2 精确实现
  → M2 通信核验
  → M2→M5 交接
  → M5 精确实现
  → M5 通信核验与基础接口交接
  → M6A 实现与完整性能验收
  → M6B 实现与完整性能验收
  → M7 报告
```

M4 不再出现在合并门中。

## 8. 联合验收

### 8.1 正确性

双方共同确认：

- 输出长度为 n；
- 每位为 0/1；
- 恰有 K 位为 1；
- 与冻结 oracle 一致；
- 稳定同分、原始位置和 payload 对齐正确；
- 随机、重复、全相等、负值、K 边界和非二次幂输入通过；
- secure runtime 不依赖测试重构。

### 8.2 角色与安全边界

- Dealer 输入无关并退出在线路径。
- 材料绑定和一次性消费可审计。
- 没有在线明文 `true_rank`。
- 公开值遵循具体协议说明。
- 不能把 I 路线允许的 shuffled rank 公开继承为 III 的公开权限。
- 图升级的局部 rank、bucket 和阈值处理单独审计。

### 8.3 轮数与通信

- Protocol I 精确核心三轮，Protocol III 精确核心两轮。
- 适配轮数和端到端轮数分别记录。
- 通信能够从分方原始计数复算。
- 两次通信核验均有差异解释。
- 未解决问题不被数量级接近掩盖。
- 不用通信结果单独证明论文或代码正确。

### 8.4 性能

保留全部统一指标：

- offline time；
- offline material；
- online time；
- total/per-party/分方通信；
- online rounds；
- online PRG calls；
- comparison edges；
- total time；
- 完整运行 provenance。

每配置预热一次、正式五次，保留 median/min/max 和逐次结果。AAV86 另记实际边数与节点复杂度，BB90+DCF 分开记录选择和成员判断成本。

大规模失败配置如实记录原因；成功配置缺少适用指标时，不能标为“全部指标完成”。不得借用理论值或旧实验填充实测字段。

## 9. 已完成的 M1.1 与 M2 → M3 历史记录

本节仅保留过去交接与验收事实，不作为当前 M2→M5 已完成的证据。

### 9.1 M1.1 公共底座收尾

M1.1 已在 Ubuntu 24.04.4 LTS、WSL2 的全新 Debug 构建中通过 CTest 4/4。

测试代码 revision：

```text
a2efe5e3d2d22bb3c031fb24dc3246c37d442fad
```

已完成：

- 四项 M1 测试注册 CTest；
- seed、输入分布、编译器、flags、CPU、内存、OS 和重复次数等 provenance；
- 分方 sent/received 与 total/per-party 派生；
- Ubuntu 干净复现；
- 冻结基线复检。

详见：

`docs/M1_1_UBUNTU_HANDOFF.md`

该记录闭合的是当时 M1.1→M2 公共基础门，不单独证明 M3 或精确核心已实现。

### 9.2 M2.0–M2.16 工程关闭与历史交接

M2 已完成 C 级模块化工程基线：

```text
m2_protocol_i_raw_score_input_modular_8round_mask_output
```

其实际路径为：

```text
2 轮 raw-score adapter
+ 4 轮 Protocol-I-shaped core
+ 2 轮 reverse mask adapter
= 8 轮
```

当时没有达到论文三轮核心，因为 VFSS PS 接口尚不能提供同置换的 public masked shuffled list。

M2.16 验证论文所需公开列表为 `pi(x)+r`，其中 r 对任一单方未知，并用于后续 FSS gate；该阶段没有新增可审计的精确功能或关联材料实现。

文档随 `f800f96` 合入 main，但没有改变工程标签，也没有将候选升级为精确实现。

相关决策：

- `docs/decisions/M2_PROTOCOL_I_PAPER_EXACT_3ROUND_DESIGN.md`
- `docs/decisions/M2_PROTOCOL_I_EXACT_LEAKAGE_AUDIT.md`

当时的决定是：该论文差距保留为后续研究问题，不阻塞 M2 工程接口交给 M3。角色 B 基于已有 M1/M2 公共契约推进 M3，不复制第二套输入、rank、mask 和计量语义。

这次历史交接已经完成，不能继续写成“M3 现在可以开始”。

### 9.3 M2 验证可靠性记录

后续 chosen-OT readable-hangup 和 modular E2E FD-lifecycle 修复没有改变 M2 协议图。

记录的 Ubuntu 24.04.4、soft `RLIMIT_NOFILE=1024` 环境下：

- EMP-ON：19/19；
- EMP-OFF：13/13；
- 相关基线和重复矩阵通过。

这些结果不改变八轮标签，也不是 paper-exact 或正式网络性能证据。

详见：

- `docs/reproduction/M2_CHOSEN_OT_POLLHUP_UBUNTU_2026-09-06.md`
- `docs/reproduction/M2_MODULAR_E2E_FD_LIFECYCLE_UBUNTU_2026-09-06.md`

### 9.4 历史 M2 → M3 冻结契约

当时交接并继续保留的语义为：

- Q20.12 signed score arithmetic shares；
- `raw=(x0+x1) mod 2^32`；
- controller 不重构 raw 或预计算 priority key 代替安全输入适配；
- score 降序、original index 升序；
- rank 0 为最高优先级；
- 原顺序、长度 n、恰有 K 个 1 的秘密共享 mask；
- Dealer 离线提供材料后退出；
- 复用 transport、session、fingerprint 和 metrics；
- 重构只在测试层。

旧 M2 C 级路径的公开元数据包括：

```text
logical_n、padded_n、K、ring/comparison width
session、fingerprint、phase、sequence
D1 允许的 shuffled (slot, rank_P)
masked-key opening
```

这些公开值属于当时 M2 路径的具体边界，不表示 M3 可以公开 shuffled rank，也不授权未来组合公开原顺序 rank。

禁止公开 raw score、carry、sign、unmasked priority key、comparison bits、original-index mapping、完整置换、selected original index、原顺序 mask 和 oracle 数据。

### 9.5 M3 已完成状态

M3 已在 `main@bb0d0e8` 完成整改并冻结。

| 实现 | 输入 | 在线轮数 |
| --- | --- | --- |
| `agarwal_protocol_iii_modular_3round` | padded priority-key shares | 3 |
| `moe_topk_protocol_iii_raw_score_modular_5round` | Q20.12 raw-score shares | 5 |

已完成：

- DPF conformance；
- logical-n GRank；
- masked-rank DPF routing；
- secure combine；
- raw-score 安全入口；
- Dealer/P0/P1 独立测试进程；
- 两个正式 Party-role executable；
- 控制器汇总分方 report 形成 MetricsRecord；
- 11-test M3 验证矩阵。

完整记录：

`docs/reproduction/M3_REVIEW_CLOSEOUT_UBUNTU_2026-09-10.md`

M3 继续作为 M5 的正确性和开销对照。原始测试来源、计时边界和未测字段保持原样，不将本次文档修订写成重新执行测试。

## 10. 当前立即任务

### 角色 A

维护 M2 已完成资产，并参与 M5 对 CmpAgg/rank-share contract 的复用评审。

### 角色 B

1. 推进 M5 Protocol III two-round path。
2. 复用现有 uCMP、DCF、CmpAgg Gen/Eval、priority-key semantics 与 rank-share contract。
3. 不重新实现第二份 ranking core。

### 双方

1. 保留 M2/M3 已完成工程记录。
2. 不恢复 M4 CipherGPT 任务。
3. 不混淆分支进度与 main 状态。
4. 不以接口合并代替交接验收。
5. 按 AAV86 完整验收在前、BB90+DCF 完整验收在后的顺序推进。
6. 对所有完成声明提供 revision、测试、计量与复跑证据。
