# M3 及后续双人分工计划

状态：**M3 已关闭；M2 COMPLETED；M5 IN PROGRESS**。

更新日期：2026-09-25

本文承接 `PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md` 和 `docs/TEAM_WORK_PLAN.md`，保留 M3 已完成的接口契约与证据，细化当前任务、后续分工、交叉评审、分支建议及合并顺序。

当前执行序列：

```text
M2 Protocol I：COMPLETED
  → M5 Protocol III two-round path：IN PROGRESS
  → M5-H2 独立评审完成，最终关闭 FAIL（F1/F2）
  → M5-FIX / 接收方 G3 复跑与基础接口交接
  → M6A AAV86 两种升级及完整性能验收
  → M6B BB90+DCF 两种升级及完整性能验收
  → M7 六种方案统一报告
```

M3 是已完成前置基础，不重新安排实现。M4 CipherGPT 已取消，不再作为 M5 或后续阶段的前置条件。

M2 已完成 current engineering acceptance：three-round C-INSTANTIATION implemented、independent three-round review PASS、online logical communication matches Theorem 4.1、Protocol I PR merged。`AUTHOR_EXACT = NOT_PROVEN` 保留为证据边界；dated strict-gate 记录属于历史，不阻塞当前 M5 runtime。

M5-B–G 已在工作分支完成；M5-H1 通信按用户指定数量级门槛通过（n=2–128，Fselect/Fsort），但 Theorem 4.2 精确逻辑式仍差 `254n` bits。M5-H2 [独立评审](reviews/M5_PROTOCOL_III_INDEPENDENT_FINAL_REVIEW_2026-09-25.md)已完成，M5 最终关闭 FAIL（F1/F2），仍为 IN PROGRESS。下一步为 M5-FIX 和接收方 G3 复跑；见 [暂定交接](handoffs/M5_PROTOCOL_III_TO_M6A_HANDOFF_2026-09-25.md)。

## 1. 当前基线与复用边界

### 1.1 已冻结语义

M1/M1.1 已完成并冻结：

- 32 位二补码 signed fixed-point score，小数位数为 12；
- score 降序、original index 升序；
- 最高优先级 rank 为 0；
- 原始输入顺序下的秘密共享 Top-K bit-mask；
- 每位为 0/1，且恰有 K 位为 1；
- 公共 oracle、基础输入分布和计量语义。

六种方案必须复用这些契约，不建立另一套 score、tie rule、rank 或 mask 定义。

### 1.2 M2 工程基础

M2 已完成并合入 main 的身份是：

```text
M2 Protocol I C-level modular engineering baseline
```

当前标签：

```text
m2_protocol_i_raw_score_input_modular_8round_mask_output
```

当前路径：

```text
2 轮 raw-score adapter
+ 4 轮工程核心
+ 2 轮 reverse mask adapter
= 8 轮
```

已有可复用组件包括：

- priority-key 表示与 logical/padded layout；
- DCF-backed uCMP；
- 全对全 CmpAgg；
- session/fingerprint/package binding；
- 有界 framed transport；
- Dealer→Party 0/Party 1 材料传输；
- EMP chosen OT；
- OPV 与 Share Translation；
- Permute+Share；
- 两遍 secret-shared shuffle；
- raw Q20.12 score share 输入适配；
- 原顺序 XOR Top-K mask 输出；
- 已有通信、原语调用和轮数计数；
- 两方及 Dealer/P0/P1 独立进程测试基础。

这些能力不能自动证明精确核心、图升级或完整性能计量已经完成。

特别需要保留以下边界：

- M2 的完整图材料生成不等于自适应图预处理。
- M2.16 只完成 paper-exact 可行性和泄露审计，没有新增精确原语。
- 当前精确三轮目标需要闭合同置换 public masked-list 与 GRank 材料契约。
- 已有部分调用计数不等于完整可信的在线 PRG 计数。
- 正式 LAN/WAN 性能仍需在对应阶段完成。

### 1.3 M3 基础

M3 已在 `main@bb0d0e8` 完成整改并冻结：

```text
agarwal_protocol_iii_modular_3round
moe_topk_protocol_iii_raw_score_modular_5round
```

三轮入口接收 priority-key shares；五轮入口接收 raw-score shares。两者不能混用名称、输入或轮数。

M3 保留为 M5 的正确性、接口和开销对照。完整契约与证据见第 3 节。

### 1.4 当前目标身份

Protocol I 的论文精确核心为三轮，Protocol III 为两轮。输入适配、表示转换和 mask 输出适配单独记录，并计入端到端主结果。

最终六种方案为：

1. Protocol I。
2. Protocol III。
3. Protocol I + AAV86。
4. Protocol III + AAV86。
5. Protocol I + BB90+DCF。
6. Protocol III + BB90+DCF。

旧工程基线是额外对照，不替代上述精确目标。历史 Direct Top-K 原型不直接改名为 BB90+DCF。

## 2. 总体分工与并行边界

### 2.1 双人职责

| 成员 | 当前主任务 | 后续主任务 |
| --- | --- | --- |
| 角色 A（搭档） | 维护 M2 已完成资产并评审 CmpAgg/rank-share 复用 | 公共比较材料与预处理；Protocol I 两种升级及实验 |
| 角色 B（Protocol III 负责人） | M5 Protocol III two-round path | M5 通信核验；Protocol III 两种升级及实验 |
| 双方共同 | 交叉评审、通信复跑、契约核对 | 图算法与安全设计、完整性能验收、最终报告 |

角色 A 不再承担 CipherGPT 修复、适配或性能任务。角色 B 不重新实现已关闭的 M3。

### 2.2 基础协议阶段门

M2 和 M5 均执行：

```text
G1：协议实现与正确性完成
  → G2：通信测量及差异解释完成
  → G3：接口、示例和证据交接完成
```

M2 由 A 交付、B 接收；M5 由 B 交付、A 交叉复跑，双方共同冻结进入 M6A 的基础接口。

代码可分多个 PR 合并，但代码合并不等于通信核验和接口交接已经完成。

### 2.3 可提前并行的工作

允许提前进行：

- 来源与论文核对；
- 域、编码和消息设计；
- 失败用例与明文 oracle 准备；
- 计量模板和实验配置设计；
- 不依赖未冻结接口的独立验证。

前置条件未满足时：

- 不合并依赖未冻结接口的 runtime；
- 不把原型标为论文精确复现；
- 不复制公共语义绕开接口问题；
- 不把后续实验目标写成已完成能力。

M6A 完成两种升级的完整性能验收后，再进入 M6B 的依赖实现和正式实验。BB90 资料研究可提前进行。

## 3. M3 已完成契约与证据

本节保留 M3 的历史实现和冻结行为，不作为新 M5 已完成的声明。

### 3.1 固定协议路径

M3 三轮核心为：

```text
R1：GRank
R2：masked-rank DPF routing
R3：share-preserving masked multiplication/combine
```

输出为：

```text
原始输入顺序下长度为 logical_n 的 XOR Top-K mask shares
```

M3 是模块化三轮工程基线，不是 Theorem 4.2 的精确两轮实现。

### 3.2 已完成子阶段

| 子阶段 | 内容 | 状态 |
| --- | --- | --- |
| M3.0 | Ubuntu 环境与构建基线 | 已完成 |
| M3.1a | `keyGenDPF`、`evalDPF_Payload` conformance | 已合入并通过复检 |
| M3.1b | DPF key 经 Peer 传输后求值不变 | 已合入并通过复检 |
| M3.1c | share-preserving multiplication adapter | 已合入并通过复检 |
| M3.2 | Protocol III GRank runtime | 已完成并通过复检 |
| M3.3 | masked-rank DPF routing | 已完成并通过复检 |
| M3.4 | secure combine 与原顺序 mask | 已完成并通过复检 |
| M3.5 | Dealer/P0/P1 独立进程 E2E | 已完成并通过复检 |

上述实现和测试已合入 `main@bb0d0e8`。

### 3.3 M3.2：一轮 GRank

输入契约：

```text
padded priority-key additive shares
ProtocolIPartyPackage
GRank framed fd
```

冻结执行行为：

1. 校验 session、fingerprint、party、n、K 和位宽。
2. 校验 node mask、完整比较图材料的数量和绑定。
3. 对本地 priority-key share 加 node mask share。
4. 通过 `ProtocolIFramedChannel` 交换 masked keys。
5. 只打开协议允许的 masked keys。
6. 调用 `protocol_i_cmpagg_eval_party()`。
7. 将逻辑位置的 rank share 约化到 `Z_(2^rank_bits)`。
8. 返回 additive priority-rank shares。
9. 记录对应通信、比较边及原语调用。

复检后的 GRank 图、材料、在线传输向量和比较边计数按 `logical_n` 覆盖真实位置。priority-key 输入仍保留 padded layout 契约。

禁止：

- 重构 priority key 或 rank；
- 返回 padded slot 作为真实输出；
- 调用 Protocol I shuffle 或 rank reveal；
- 将测试 oracle 引入 secure runtime；
- 静默修改 M2 公共接口的既有语义。

已验收条件：

- 双方 socket E2E 与 oracle 差分通过；
- 重复值、全相等、负值和边界输入通过；
- `logical_n=1`、非二次幂和 padding 通过；
- package binding 错误显式拒绝；
- one-shot 材料按契约消费；
- GRank 因果路径为一轮；
- 相关测试与复现记录齐备。

### 3.4 M3.3：DPF routing

输入契约：

```text
rank additive shares
rank-mask shares r_i
DPF party keys
公开目标 rank k
```

冻结行为：

1. 在 `Z_(2^rank_bits)` 中计算 `[rank_i]+[r_i]`。
2. 交换并打开 `hat_rank_i`。
3. 对逻辑位置 i 和目标 `k ∈ [0,K)` 计算：

   ```text
   evalDPF_Payload(key_i, hat_rank_i - k)
   ```

4. 输出 additive indicator shares。
5. 记录 DPF key、payload 位宽、材料和 R2 通信。

复用接口：

- `keyGenDPF`；
- `evalDPF_Payload`；
- 已验证的 Peer/Dealer key transport。

禁止：

- 将 `evalDPF_EQ` 的 XOR share 直接用于算术乘法；
- 重构 rank、DPF point 或 indicator；
- 读取旧 `.bin` key 文件；
- 直接传输 `DPFKeyPack` 内存布局。

M3 继续保留 GRank、routing、combine 三轮身份。跨阶段压缩在 M5 中单独实现，不能覆盖本基线。

### 3.5 M3.4：secure combine

输入契约：

```text
indicator additive shares
unit payload additive shares
fresh multiplication material
```

冻结行为：

1. 对每个 `(i,k)` 使用独立乘法材料。
2. 通过 masked multiplication adapter 生成 product additive shares。
3. 不公开 product。
4. 沿目标 rank 聚合每个原位置的 mask arithmetic share。
5. 使用 `Z_(2^64)` 加法 share 的最低位生成 XOR mask share。
6. 只返回前 `logical_n` 个结果。

该最低位转换属于既有环上实现，不能直接推广为 M5 任意域上的免费转换。

已验收条件：

- 重构后每位为 0/1；
- 恰有 K 位为 1；
- 与 M1 oracle 一致；
- secure runtime 不存在 product 重构；
- R3 材料新鲜且一次性使用；
- adapter 通信和材料进入结果。

### 3.6 M3.5：独立进程 E2E

冻结测试路径：

```text
Dealer 离线生成并发送材料后退出
  → P0/P1 接收输入 shares
  → GRank
  → DPF routing
  → secure combine
  → 输出 XOR mask shares
```

测试覆盖：

- 随机、重复 score、全相等；
- 正负边界；
- `K=1`、`K=n`；
- 非二次幂 n；
- 错误 session/fingerprint；
- 截断 package；
- 材料重复使用；
- socket 分片、超时和 peer 提前退出。

重构和 oracle 检查只在 TEST_ONLY 控制路径发生。

### 3.7 Raw-score 五轮扩展

冻结实现：

```text
moe_topk_protocol_iii_raw_score_modular_5round
```

输入为 Q20.12 raw-score shares，路径为：

```text
carry
  → sign
  → GRank
  → DPF routing
  → secure combine
```

不能将五轮 raw-score 扩展写成三轮 raw-score 协议，也不能把 priority-key 入口写成原始 score 入口。

### 3.8 正式入口与计量证据

已完成能力包括：

- Dealer/Party fork+exec 角色隔离；
- logical-n GRank；
- DPF conformance CTest 注册；
- raw-score 独立进程 E2E；
- 三轮与五轮正式 Party-role executable；
- TEST_ONLY 控制器汇总两方 report 生成 MetricsRecord；
- Ubuntu 构建与验证记录。

Dealer 是既有 TEST_ONLY E2E harness 的独立角色，不应写成第三个正式 runtime executable。

正式 Party 生成各自 report，完整 MetricsRecord 由控制器汇总；不能写成每方各自生成全局计量结果。

历史记录中网络 bandwidth/RTT 和完整可信在线 PRG 计数仍未测。计时边界包含已披露的 Dealer 生命周期、输入分发和 report 收集，后续不能静默改写。

主要证据：

- `docs/decisions/PROTOCOL_III_MODULAR_3ROUND_DESIGN.md`
- `docs/decisions/PROTOCOL_III_MASKED_MUL_ADAPTER.md`
- `docs/reproduction/DPF_CONFORMANCE_UBUNTU_2026-09-04.md`
- `docs/reproduction/MASKED_MUL_ADAPTER_UBUNTU_2026-09-05.md`
- `docs/reproduction/M3_ENV_BASELINE_UBUNTU_2026-09-07.md`
- `docs/reproduction/M3_REVIEW_CLOSEOUT_UBUNTU_2026-09-10.md`

关闭记录保留其 11-test 验证口径、结果来源和限制，不将本次计划修订写成重新运行测试。

## 4. M2 已完成资产与 M5 复用交接

### 4.1 实现任务

M2 已完成交接资产包括：

1. 固定当前 candidate revision 与实现范围。
2. 闭合同置换的秘密共享 `pi(x)` 和公开 `pi(x)+r` 输出。
3. 验证 r 对任一单方未知，且与后续 GRank 材料关联。
4. 保证 Dealer 输入无关、在线静默。
5. 接入全对全 CmpAgg、稳定 rank 和 payload 路由。
6. 保留 raw-score 输入与原顺序 mask 输出。
7. 完成三轮论文核心的因果消息审计。
8. 运行 conformance、差分、独立进程 E2E 和异常测试。

现有四轮核心、八轮总路径保持原标签，不直接覆盖为精确身份。

### 4.2 通信核验任务

实现验收后：

- 冻结被测 revision；
- 测量分方、分阶段发送和接收量；
- 对照 Theorem 4.1；
- 明确 logical/padded 规模、位宽、rank 域与 payload；
- 分离核心、输入适配、输出适配和封装；
- 形成论文公式、实现消息推导和实测计数三层对照；
- 解释关键差异，修正后重跑受影响配置。

至少覆盖小规模边界、`(128,2/8)`、`(256,2/8)`，并使用可行增长点检查趋势。

不以单点数量级相近代替成本解释，也不以通信不符直接断言论文错误。

### 4.3 M2→M5 交付物

角色 A 交付、角色 B 接收：

- revision、构建配置和依赖；
- 公共接口与最小调用示例；
- raw/priority-key 输入边界；
- score、rank、payload 和 layout；
- 材料绑定、分发与一次性消费；
- transport、session 和错误语义；
- core/adapter 计量说明；
- 正确性、轮数、安全与通信报告；
- 不可直接复用的假设。

角色 B 复跑示例及代表性核验，记录接收结果。只合并接口而未完成复跑，不算交接完成。

Protocol I 专用 shuffle 不作为 Protocol III 必须调用的部件。

## 5. 当前角色 B：M5 IN PROGRESS

### 5.1 交接前任务

角色 B 当前负责：

1. 同步总体计划、实施计划和两份分工计划。
2. 更新 README、路线决策和配套说明。
3. 清除旧 M4 前置要求。
4. 准备两次通信核验和交接模板。
5. 设计 M5 域、非零编码、DPF 兼容性和消息依赖。
6. 整理 M3 复用矩阵与失败用例。
7. 评审角色 A 的接口和通信核验方法。

不修改任何尚未解锁的 strict shuffle 或公共材料实现，不基于未冻结接口宣称 M5 已完成。

### 5.2 M5 实现任务

前置条件为 M3 已完成，以及 M2 已完成的 CmpAgg/rank-share handoff 可复用。

角色 B 负责：

1. 选择并记录满足论文条件的域表示。
2. 定义非零 payload、乘法掩码、零值编码及逆元失败语义。
3. 核对 VFSS DPF 与目标表示的兼容性。
4. 审计 M2 ring-only 接口的复用限制。
5. 实现 GRank 与 DPF routing 的跨阶段压缩。
6. 验证两轮论文核心。
7. 明确 raw-score、域转换和 XOR mask 输出成本。
8. 与 M3 使用同一输入、K、种子和 oracle 差分。
9. 完成独立进程 E2E、材料和泄露审计。

单位 payload 特化产生的删轮方案不能直接作为一般压缩路由的精确复现证据。

### 5.3 M5 通信核验

角色 B 负责：

- 对照 Theorem 4.2；
- 注明共用掩码等优化是否采用；
- 区分单个 order statistic、sorting 与项目 Top-K mask；
- 分解 GRank、压缩路由、域转换和输出适配；
- 保留 total、per-party、sent 和 received；
- 与 M3 做可比的开销对照；
- 提供三层通信对账及差异解释。

角色 A 复跑代表性配置，核查通信、代数和消息依赖。

### 5.4 M5 退出与交接

只有以下条件满足，才更新 `agarwal_protocol_iii_exact_2round` 精确身份：

- 论文代数与非零条件满足；
- 两轮消息依赖成立；
- oracle、conformance 和 E2E 通过；
- 安全边界明确；
- 输入输出适配完整计量；
- 通信差异已解释；
- 公共接口和结果可复跑。

之后双方冻结进入 M6A 的基线 revision、复用矩阵、材料契约和计量说明。全对全接口不能直接写成已支持自适应图。

## 6. M4 取消与范围清理

M4 CipherGPT 原生基线取消。

不再安排：

- source/revision/license 实施前审计任务；
- QuickSelect 修复；
- 原生 mask adapter；
- CipherGPT-style VFSS adapter；
- CipherGPT 性能实验；
- 以 CipherGPT 完成为条件的 M5 合并门。

保留历史资料和目录忽略规则，不删除历史证据，也不复用 M4 编号承载新任务。

CryptoMoE 位于 M7 之后，另行定义工作负载。它不构成继续实施 CipherGPT 的理由。

## 7. M6A：AAV86 两种升级的分工

### 7.1 前置条件与目标

前置条件：

- M2 精确核心与通信核验完成；
- M5 精确核心与通信核验完成；
- 基础接口交接完成。

交付：

```text
Protocol I + AAV86
Protocol III + AAV86
```

两种实现及完整性能验收都属于 M6A，不只交付明文图算法或单一路线原型。

### 7.2 公共复用矩阵

| 基础部件 | M6A 处理 |
| --- | --- |
| score、tie rule、oracle | 直接沿用冻结语义 |
| priority-key 与 layout | 沿用契约，明确实际表示 |
| uCMP/DCF | 审计后复用 |
| CmpAgg 聚合 | 复用适用逻辑，不继承固定完整图假设 |
| party package | 按动态图材料绑定需求审计扩展 |
| transport/session/fingerprint | 复用并验证新阶段边界 |
| metrics | 补齐 edge、vertex、PRG 和逐轮计数 |
| shuffle、chosen OT、share translation | 仅在对应组合实际需要时复用 |
| DPF、域和压缩路由 | 由 III 路线单独审计 |
| raw-score 与 mask 适配 | 复用或明确扩展，成本计入总路径 |

### 7.3 角色 A 主责

- 公共图接口与比较材料；
- 自适应 exact-edge 预处理；
- edge、round、session 和掩码绑定；
- Protocol I+AAV86 runtime；
- Protocol I 路线的性能实验；
- 材料、Dealer 和通信审计。

A 负责公共图执行与材料的主写协调，B 参与算法语义和安全评审，不另建第二份图语义。

### 7.4 角色 B 主责

- Protocol III+AAV86 组合设计；
- 域、DPF routing 与图执行的衔接；
- 原顺序 mask 和表示转换；
- Protocol III 路线的性能实验；
- 统一结果汇总和对照报告；
- 复核公共图输出与冻结 oracle 的一致性。

### 7.5 双方设计门

必须明确：

- 每轮公开值、bucket 和局部 rank；
- 图生成依赖；
- 材料生成与分发时机；
- Dealer 是否在线；
- one-shot 消费边界；
- 两种组合各自的泄露和消息依赖。

Protocol I 对应 CA compiler 的核心目标为 `2r+1`。Protocol III 的 `2r` 是团队组合目标，需独立推导，不视为会议版已给出的定理。

在线 Dealer 原型、完整图预留对照和明文图 oracle 应分别标记。完整图预留不能冒充已解决自适应 exact-edge 预处理。

### 7.6 完整性能任务

A、B 分别运行各自路线，统一：

- `(n,K)` 矩阵；
- `r=2,3,4,5`；
- 输入和算法种子；
- LAN/WAN 配置；
- 预热一次、正式五次；
- median/min/max；
- 全部统一指标。

每次运行另记录实际 `e_A(n,r)`、`v_A(n,r)`、DCF/uCMP 调用、PRG、逐轮通信和材料。

### 7.7 退出条件

- 两种路线均完成正确性与安全验收；
- 自适应预处理和 Dealer 边界明确；
- 轮数由实际消息依赖核验；
- 计量能力可信；
- 成功配置全部适用指标齐全；
- 全矩阵配置有成功或明确失败记录；
- 报告披露覆盖范围和限制；
- 完成交叉复核及 M6B 接口交接。

M6A 完整性能验收未完成时，不把后续主线改为 M6B。

## 8. M6B：BB90+DCF 两种升级的分工

### 8.1 前置条件与目标

M6A 两种升级及完整性能验收完成后，交付：

```text
Protocol I + BB90+DCF
Protocol III + BB90+DCF
```

完整路径为：

```text
BB90 选择稳定第 K 大阈值
  → DCF 生成成员指示共享
  → 必要路由、逆映射和共享转换
  → 原顺序 XOR Top-K mask
```

“投票”表示秘密共享成员指示，不增加明文投票公开步骤。

### 8.2 角色 A 主责

- BB90 公共图/选择接口；
- 比较材料及预处理；
- 在线阈值与 DCF 的安全衔接；
- Protocol I 组合实现；
- 对应 conformance、E2E 和性能实验；
- 材料与通信分解。

### 8.3 角色 B 主责

- 稳定第 K 大阈值与项目 rank 语义映射；
- Protocol III 组合及域/DPF 适配；
- 原顺序 mask 与共享转换；
- 对应 conformance、E2E 和性能实验；
- 统一汇总及六种方案的数据对齐。

阈值表示、公共 DCF 契约和稳定同分处理由双方共同评审，公共文件按所有权规则由一人写入。

### 8.4 算法与安全设计门

必须说明：

- BB90 具体版本、适用 K 范围和迭代参数；
- 随机性、概率成本界及失败事件；
- 输出正确性与随机成本保证的区别；
- 比较图如何转换为 CA；
- 阈值的共享表示与允许公开值；
- 在线阈值相关预处理如何保持输入无关；
- 两种组合的消息与材料时序。

不能只用 `score >= 第 K 大 score` 处理重复值。必须保持 original index 的稳定规则，保证恰选 K 个位置。

轮数从完整组合推导，不直接套用 AAV86 公式或预设“BB90 加一轮”。

### 8.5 完整性能任务

分别测量：

1. BB90 选择；
2. DCF 成员判断；
3. 输入、路由、逆映射和 mask 适配；
4. 完整端到端路径。

记录迭代数、节点/边计数、DCF/uCMP/PRG、材料、通信、轮数和时间。

运行统一 `(n,K)` 矩阵与已确定的 BB90 参数，完成 LAN/WAN 和全部统一指标。不能只报告 BB90 内部比较量而省略 DCF 与输出成本。

### 8.6 退出条件

- 两种完整 Top-K 路径均实现；
- 阈值和最终 mask 与 oracle 一致；
- 重复值、全相等、K 边界及非二次幂通过；
- 预处理、泄露和概率保证明确；
- 成功配置全部适用指标齐全；
- 失败配置及限制完整记录；
- 两种路线完成交叉复核后进入 M7。

## 9. M7：双方共同完成六种方案报告

### 9.1 结果责任

角色 A 对以下结果负责：

- Protocol I；
- Protocol I+AAV86；
- Protocol I+BB90+DCF；
- Protocol I 工程基线对照。

角色 B 对以下结果负责：

- Protocol III；
- Protocol III+AAV86；
- Protocol III+BB90+DCF；
- M3 三轮工程对照；
- 统一汇总与图表。

每方负责自己结果的输入、命令、revision、原始计数和解释，另一方交叉检查。

### 9.2 统一报告要求

保留：

- offline time 和 material；
- online time；
- total/per-party/分方通信；
- 因果轮数；
- PRG 调用；
- 比较边数；
- total time；
- 环境、参数、种子、命令和重复次数；
- correctness 与失败配置状态。

主比较按相同路线比较全对全、AAV86、BB90+DCF，再比较相同算法下的 I/III 两种路线。

历史工程对照另列，不把不同功能、安全模型或计时边界混入无说明的速度排名。

M7 汇总前面已经完成的实验；仅在 revision、环境或计量变化时补跑必要配置。CipherGPT 不进入本轮比较组。

### 9.3 未完成配置

资源不足、超时、执行失败、正确性失败和指标缺失分别记录。

无法完成的大规模点可保留原因及 `NOT_MEASURED`，但不能宣称该配置成功。成功运行而缺少 PRG 等适用指标时，不能标为全部指标完成。

不得静默丢弃失败种子、外推填表或用旧数据替代当前实测。

## 10. 共享文件与修改所有权

本节沿用 `docs/TEAM_WORK_PLAN.md` 的所有权，不另建冲突规则。

| 资产 | 主写 | 评审 |
| --- | --- | --- |
| I 专用接口、shuffle、runtime | A | B |
| III 专用接口、域、DPF routing | B | A |
| M2 来源的公共比较材料、transport、session | A | B |
| 公共 `metrics.h` 和计量契约 | A | B |
| I 分阶段 report | A | B |
| III 分阶段 report | B | A |
| 通用汇总、核验模板与结果格式 | B | A |
| 总纲、实施计划、两份分工、README、路线决策 | B | A |
| 公共构建、CTest 和实验脚本 | 每个 PR 指定一人 | 另一方 |
| score、tie rule、oracle | 保持冻结 | 双方 |

共享文件包括：

```text
VFSS/include/moe_topk/score_semantics.h
VFSS/include/moe_topk/topk_oracle.h
VFSS/include/moe_topk/metrics.h
docs/decisions/M1_SCORE_SEMANTICS.md
README.md
PROJECT.md
docs/IMPLEMENTATION_PLAN.md
docs/TEAM_WORK_PLAN.md
docs/M3_ONWARD_TEAM_WORK_PLAN.md
```

规则：

- 同一共享文件同一区域只由一人写入。
- 使用方提出需求，主写方协调最小公共修改。
- 公共变更说明另一条路线的影响并运行相关回归。
- 交接后改变 ABI、材料或计量边界，需更新契约和示例。
- 不复制另一套 oracle、rank、输入生成或指标定义。
- 不静默重解释历史结果。
- 不把分支进度写成 main 已完成。
- 不修改 `VFSS-baseline/`。
- 不提交论文、密钥、日志、构建物和完整参考工程。

## 11. 交叉评审重点

### 11.1 B 评审 M2

检查：

- 同置换 public masked-list 与 payload 是否绑定；
- r 是否对单方未知并与 GRank 材料一致；
- 是否真的减少因果阶段，而非改函数或计数标签；
- Dealer 是否输入无关并在线退出；
- 稳定同分和原始位置是否保持；
- 核心、输入和输出适配成本是否完整；
- 通信差异是否逐项解释；
- 接口示例能否在交接 revision 上复跑。

### 11.2 A 评审 M5

检查：

- 域与非零编码条件；
- DPF 及公共 ring-only 接口的复用边界；
- 两轮消息依赖；
- 是否存在 rank、indicator 或 product 重构；
- mask 转换是否错误沿用 M3 环上最低位结论；
- 是否遗漏域转换和输出成本；
- 是否以单位 payload 特化冒充论文完整压缩；
- 通信报告与实际 revision 是否一致。

### 11.3 双方评审 M6A

检查：

- 自适应图是否依赖在线公开值；
- 材料如何在 Dealer 静默条件下生成和消费；
- 完整图预留是否被误称为 exact-edge；
- I/III 组合是否分别论证；
- edge、vertex、DCF 和 PRG 计数是否区分；
- 比较量下降是否包含额外材料、通信和路由成本；
- 两种路线是否完成完整性能验收。

### 11.4 双方评审 M6B

检查：

- 是否采用明确的 BB90 算法版本；
- 稳定第 K 大是否与冻结顺序一致；
- 重复值是否恰选 K 个；
- 在线阈值与 DCF 材料如何安全衔接；
- 是否公开阈值、selected index 或原始映射；
- 是否省略最终 DCF 和 mask 成本；
- 随机成本界与正确性保证是否混写；
- 是否错误地将其他 Direct Top-K 原型改名。

### 11.5 双方评审性能报告

检查：

- total 只累加发送量，received 不重复计入；
- 论文 total 与 per-party 是否正确映射；
- bits、bytes、KB/KiB 是否注明；
- core 与端到端路径是否分开；
- 成功、失败和未测配置是否完整；
- 所有数字是否可追溯到原始记录；
- 理论估算是否混入实测字段。

## 12. 分支记录与建议

以下新分支名仅为建议，不表示已经创建或完成。若当前任务已有有效分支，继续使用并记录，不为符合名称重复建立分支。

### 12.1 M3 历史分支

原计划记录的以下分支已合入：

```text
m3-grank-runtime
m3-dpf-routing
m3-masked-combine
m3-protocol-iii-3round-e2e
```

保留历史记录，不重新创建同名任务。

### 12.2 当前文档修订

建议：

```text
docs-topk-roadmap-20260913
```

用于同步本次范围、阶段门和分工，不混入协议实现。

### 12.3 M2 精确核心与交接

建议按独立目标拆分：

```text
m2-paper-shuffle-contract
m2-paper-shuffle-runtime
m2-protocol-i-3round-e2e
m2-protocol-i-comm-validation
m2-to-m5-interface-handoff
```

不复用 M2.15/M2.16 编号覆盖旧审计结果。

### 12.4 M5

```text
m5-protocol-iii-field-contract
m5-protocol-iii-field-adapters
m5-protocol-iii-2round-runtime
m5-protocol-iii-2round-e2e
m5-protocol-iii-comm-validation
m5-to-m6a-interface-handoff
```

### 12.5 M6A

```text
m6a-aav86-reuse-and-security-design
m6a-aav86-adaptive-preprocessing
m6a-aav86-shared-graph-runtime
m6a-protocol-i-aav86
m6a-protocol-iii-aav86
m6a-aav86-metrics
m6a-aav86-performance-report
```

公共 metrics 修改仍由指定主写方协调，不能因分支独立而同时改同一区域。

### 12.6 M6B

```text
m6b-bb90-algorithm-contract
m6b-bb90-shared-selection-runtime
m6b-bb90-dcf-threshold-interface
m6b-protocol-i-bb90-dcf
m6b-protocol-iii-bb90-dcf
m6b-bb90-dcf-metrics
m6b-bb90-dcf-performance-report
```

### 12.7 M7

```text
m7-six-variant-evaluation-report
```

不再创建或推进 `m4-ciphergpt-*` 任务分支。已有历史分支是否保留不影响本轮任务取消。

## 13. 合并与验收顺序

M3 已按依赖完成并关闭，不重新安排其合并。

后续顺序：

1. 合并本次总体路线、实施计划和两份分工修订。
2. 按依赖合并 M2 精确功能、相关测试和三轮核心。
3. 完成 M2 通信核验及必要修正。
4. B 复跑并接收 M2→M5 接口。
5. 合并 M5 域与适配、两轮核心和 E2E。
6. 完成 M5 通信核验及 A 的交叉复跑。
7. 双方完成 M5→M6A 基础接口交接。
8. 合并 M6A 公共设计、材料及两种组合实现。
9. 完成 M6A 计量与完整性能验收。
10. 合并 M6B 公共选择、DCF 衔接及两种组合实现。
11. 完成 M6B 计量与完整性能验收。
12. 完成 M7 汇总及必要补跑。

各阶段可以有多个 PR。设计文档和独立测试准备可以提前合并，但不能据此跳过实现、通信或交接门。

M4 不再是任何阶段的前置条件。

## 14. 当前立即任务

### 角色 A

维护 M2 已完成资产，并参与 M5 CmpAgg/rank-share contract 的复用评审。

### 角色 B

1. 推进 M5 Protocol III two-round path。
2. 复用 uCMP、DCF、CmpAgg Gen/Eval、priority-key semantics 与 rank-share contract。
3. 不重新实现第二份 ranking core。

### 双方

1. 保留 M3 的三轮和五轮入口、契约及历史证据。
2. 不恢复 CipherGPT 实施任务。
3. 不用数量级接近代替正确性、安全性或成本解释。
4. 不混淆候选分支与 main 完成状态。
5. 保持 AAV86 完整验收在前、BB90+DCF 完整验收在后的顺序。
6. 为所有完成结论提供 revision、测试、计量和复跑证据。

## 15. 完成原则

- 每项工作有明确主写方和交叉评审方。
- 公共接口、指标和文档由指定负责人协调修改。
- 不复制或分叉冻结语义。
- 不覆盖 M2/M3 历史身份。
- 不把测试重构带入 secure runtime。
- 不把工程原型当作论文证明。
- 不把代码合并当作通信核验或交接完成。
- 不以未测、外推或历史数据填充当前实测。
- 不修改冻结基线，不提交生成物和本地参考工程。
- 阶段完成状态与实际实现、验收和性能覆盖一致。
