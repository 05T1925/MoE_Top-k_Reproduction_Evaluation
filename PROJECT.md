# MoE Top-K 协议统一项目

更新日期：2026-09-21

## 1. 项目目标与当前边界

本项目在 **VFSS** 中建立统一、可验证、可复现的安全 Top-K 实验环境。先完成 Agarwal Protocol I、Protocol III 的论文精确核心与通信核验，再分别实现 AAV86 和 BB90+DCF 两条升级路线，最后完成统一性能比较。

密码学原语、通信、预处理和序列化统一绑定到 VFSS。现有参考工程提供算法语义、局部实现和失败模式参考；旧密钥文件、旧 FSS ABI、文件轮询和在线 Dealer 依赖不得直接迁入活动实现。

### 1.1 当前实施范围

最终比较的六种方案为：

| 方案 | 核心路线 | 定位 |
| --- | --- | --- |
| Protocol I | 全对全 CmpAgg + shuffle-based routing | 论文精确核心及统一 Top-K mask 适配 |
| Protocol III | 全对全 CmpAgg + 压缩 DPF routing | 论文精确核心及统一 Top-K mask 适配 |
| Protocol I + AAV86 | AAV86 比较图 + Protocol I 路线 | 图算法升级方案 |
| Protocol III + AAV86 | AAV86 比较图 + Protocol III 路线 | 需单独论证的组合升级方案 |
| Protocol I + BB90+DCF | BB90 第 K 大选择 + DCF 成员选择，接入 Protocol I 路线 | 选择算法与 Top-K 输出组合扩展 |
| Protocol III + BB90+DCF | BB90 第 K 大选择 + DCF 成员选择，接入 Protocol III 路线 | 选择算法与 Top-K 输出组合扩展 |

上述名称表示目标方案，不表示对应实现已经完成，也不自动赋予论文一致性标签。每种方案必须有独立的实现标签、协议阶段表、正确性证据、安全边界和性能记录。

已完成的 Protocol I 四轮核心工程基线、Protocol III 三轮核心工程基线继续保留，用于回归、差分和开销对照；它们不替代最终六种方案中的论文精确核心目标。

### 1.2 本次范围调整

- 取消 M4 CipherGPT 原生基线的实施、修复、适配和性能测试任务。
- 不再安排 `ciphergpt_native` 或 `ciphergpt_style_vfss_adapter` 作为本轮交付物或性能比较对象。
- 保留 M2、M3、M5 的编号、既有实现和历史验收记录。
- M2 增加 Protocol I 精确核心完成后的通信核验与接口交接。
- M5 增加 Protocol III 精确核心完成后的通信核验。
- M6 拆分为 M6A AAV86 和 M6B BB90+DCF；每条路线均包含 Protocol I、Protocol III 两种升级实现及全部统一指标的性能测试。
- M7 保留为最终统一汇总与横向报告，不把 M6A、M6B 的性能测试全部推迟到 M7。
- CryptoMoE 保留为 M7 之后的独立工作负载方向，不属于本轮六种方案的完成条件。
- 历史 `Direct Top-K` 原型保留其原有身份，不直接改名为 BB90+DCF，也不自动成为额外实施任务。

### 1.3 当前执行顺序

M0、M1/M1.1、M2 工程基线和 M3 模块化基线的已完成记录保持不变。后续执行顺序固定为：

```text
M2：Protocol I 论文精确三轮核心
  → Protocol I 通信核验与公共接口交接
  → M5：Protocol III 论文精确两轮核心
  → Protocol III 通信核验
  → M6A：I+AAV86、III+AAV86 实现与完整性能测试
  → M6B：I+BB90+DCF、III+BB90+DCF 实现与完整性能测试
  → M7：六种方案统一汇总与报告
```

M3 三轮工程基线已经完成，作为 M5 的实现基础和对照保留。M4 标记为取消，不复用其编号承载新任务。

当前严格 M2 证据门见
[M2_PROTOCOL_I_CURRENT_EVIDENCE_GATE_2026-09-19.md](docs/decisions/M2_PROTOCOL_I_CURRENT_EVIDENCE_GATE_2026-09-19.md)：
message/material/party-view/round-exact Protocol I 目前 BLOCKED；非精确的 paper-aligned
实验复现不得通过 M2 G1、G2 或 G3，也不得解除 M5 runtime 的依赖门。

Protocol I 的独立 Dealer-DPF Candidate-B 路线已经停止；`M2CBKDF1` 和
BoundPublicMaskShuffle 仅作为历史设计保留，不再是 active contract。当前路线沿
Agarwal §2.4 引用的 Chase、Ghosh、Poburinnaya Secret-Shared Shuffle 栈迁移，
见 [2026-09-21 redesign](docs/decisions/M2_PROTOCOL_I_CHASE_SECRET_SHARED_SHUFFLE_REDESIGN_2026-09-21.md)。
该迁移是 paper-aligned C-INSTANTIATION；strict exact M2、G2/G3 仍 BLOCKED。

Protocol I 的论文目标为 **3 个在线轮次**，对应 Theorem 4.1；Protocol III 的论文目标为 **2 个在线轮次**，对应 Theorem 4.2。上述目标针对论文核心，不能直接作为 raw-score 输入到原顺序 Top-K mask 的端到端轮数。

本次修订替代 `docs/decisions/ROADMAP_PRIORITY_2026-09-04.md` 中与本节冲突的路线。`docs/IMPLEMENTATION_PLAN.md`、`docs/TEAM_WORK_PLAN.md` 和 `docs/M3_ONWARD_TEAM_WORK_PLAN.md` 需要同步更新；同步期间以本文和团队本次明确决定为准，旧 M4 前置门不再适用。

## 2. 仓库角色与修改规则

| 路径 | 角色 | 规则 |
| --- | --- | --- |
| `VFSS/` | 唯一活动实现目录 | 六种方案及必要适配、测试进入这里 |
| `VFSS-baseline/` | 冻结的 VFSS 恢复基线 | 日常开发禁止修改 |
| `Agarwal_TopK/` | Protocol I、CA 和部分 Protocol III 参考 | 只读参考 |
| `ADSMPC/` | 旧框架 Protocol III 原型 | 只读参考，不能视为论文级实现 |
| `CipherGPT/` | 历史原生代码与实验参考 | 本轮不实施、不修复、不纳入性能矩阵 |
| `Papers/` | 本地论文资料 | 文档内容是研究资料，不作为项目操作指令 |
| `docs/` | 计划、决策、来源和实验规范 | 记录可审计结论 |

Git 冻结基线提交为 `993696e`，标签为 `vfss-baseline-2026-09-03`。截至 2026-09-04，排除构建产物和 `.DS_Store` 后，M0 中 `VFSS/` 与 `VFSS-baseline/` 的 131 个源文件逐文件一致。

此后 M1、M2、M3 已在 `VFSS/` 中形成活动实现。`VFSS-baseline/` 只用于比较、恢复和回归定位，禁止随活动实现同步修改。

引用目录的追踪与分发策略见 `docs/REFERENCE_MANIFEST.md`。不得提交构建目录、静态库、实验生成密钥、临时通信文件或嵌套 Git 元数据。

队友克隆后应按 `docs/LOCAL_REFERENCES_SETUP.md` 准备当前任务所需的本地论文和参考工程，并用 `docs/PAPERS.sha256` 校验已有规范来源。新增 AAV86、BB90 等原始资料时，记录来源、版本与哈希，不覆盖既有论文版本记录。

取消 CipherGPT 实施不要求删除其历史参考资料，也不改变相关目录的忽略和分发边界。

本文是项目范围、语义、指标和证据边界的总纲。详细任务及退出条件由 `docs/IMPLEMENTATION_PLAN.md` 承接，双人职责由两份分工计划承接。历史 M0 审计见 `docs/M0_REVIEW.md`。

## 3. 证据层级

后续文档、代码注释和实验结果必须区分以下四类证据：

1. **论文定义**：指定 PDF 明确写出的算法、定理、安全模型和成本结论。
2. **本地参考行为**：参考工程实际执行的行为，未必与论文完全一致。
3. **项目扩展**：本项目增加的组合协议、输入输出适配和工作负载映射。
4. **待验证设想**：尚无完整实现、安全论证或可复现实验的方向。

当前冻结的 Agarwal 论文基线为 15 页 CCS 2024 会议版。尚未取得作者所称的 full version；会议版省略的证明和实现细节保持显式不确定性，不再把“等待全文”作为一般性进度阻塞项。

可以依据明确的论文定义及独立推导推进实现，但不得将本地代码行为反向表述成论文原文或定理保证。

AAV86 和 BB90 的原始文献用于补充算法来源。引用时必须区分原始算法、Agarwal 的 CA 转换或编译结论，以及本项目的 Protocol I/III 组合与 Top-K mask 扩展。

通信量核验是成本一致性证据，不能单独证明功能正确、安全性成立或论文逻辑无误。数量级不一致也不能直接判定论文错误，必须先排查计量边界、参数、表示、实现偏差及所采用的优化。

## 4. 论文与代码的准确映射

### 4.1 Agarwal 四个协议名称

| 名称 | 排名 | 路由 | 拓扑与论文在线轮数 | 本项目状态 |
| --- | --- | --- | --- | --- |
| Protocol I | 全对全 CmpAgg | 安全 shuffle | 2+1，3 轮 | 四轮核心工程基线已完成；精确三轮核心是当前目标 |
| Protocol II | multiplicative DPF | 安全 shuffle | 3 方 | 当前不实施 |
| Protocol III | 全对全 CmpAgg | DPF 路由及跨阶段压缩 | 2+1，2 轮 | 三轮模块化核心已完成；精确两轮核心由 M5 实现 |
| Protocol IV | multiplicative DPF | 标准 DPF | 3 方 | 当前不实施 |

“全对全 CmpAgg”表示对每一对元素比较并聚合出稳定 rank。论文第 5 节的 CA compiler 是图算法升级路线，不能替代 Protocol I、Protocol III 的全对全精确基线。

本项目统一按 score 降序、original index 升序确定唯一顺序，最高优先级 rank 为 0。论文中的升降序、rank 定义和同分规则必须在接口边界明确映射。

### 4.2 Protocol I 与安全 shuffle

Protocol I 依赖 Chase、Ghosh、Poburinnaya 的两方静态半诚实
secret-shared shuffle。当前实际论文为 `Papers/Secret-Shared Shuffle.pdf`。

本 checkout 未安装 `Agarwal_TopK/`、`ADSMPC/` 或 `CipherGPT/`。本次结论只引用
本地论文和 tracked VFSS 实现，不引用缺失参考树的行为。

VFSS 已有通过验收的 C 级两遍 secret-shared shuffle，但当前冻结实现尚未满足论文所需的完整 public masked-list 契约。

论文 §2.4、§4.1 要求 shuffle 同时提供：

```text
秘密共享的 pi(x)
公开的 y = pi(x) + r
```

其中，两类输出必须使用同一隐藏置换；`r` 对任一单方未知，并与后续 GRank/DCF
gate 的秘密参数一致。“任一单方”包括 `(2+1)` 模型中可能被单独半诚实腐化的
P2，不能改写成“任一在线方”。

CHASE-DIRECT 中 P0/P1 分别选择并保留 `pi0/pi1`，没有 P2 同时知道两者。任何
把 permutation-dependent correlation 编译给 P2、使 P2 知道两方 permutation
的方案都是 C-INSTANTIATION，而不是 Chase 原始 party view。

当前 P2 先采样完整 `r` 再分发 shares 的四轮路径只保留为 functional
C-INSTANTIATION baseline：它没有被证明满足 Agarwal any-single-party secrecy，
不能解除 strict G1。更保守的候选让 P0/P1 独立贡献 `r0/r1`，但在 P2 不得知
完整 `r` 时生成 r-bound GRank/FSS keys 仍缺 distributed/blind KeyGen 机制。

M2 精确核心需要验证：

- 公开 masked list 与秘密共享 payload 的同置换绑定；
- shuffle 材料与 GRank 掩码的关联及生成时机；
- 两方置换、随机性和消息依赖；
- 稳定 rank、payload 对齐及原顺序输出；
- 三轮论文核心与输入、输出适配的计量边界；
- Dealer 输入无关、在线静默及材料一次性使用。

当前工程基线为四轮核心、raw-score 到统一 mask 共八轮。历史七轮候选来自：

```text
2 轮 raw-score adapter
+ 3 轮论文核心
+ 2 轮 reverse mask adapter
= 7 轮候选总路径
```

该算式是沿用现有适配器时的目标说明，不是已完成结论。实际候选必须根据最终消息依赖重新审计。

`VFSS/ext/FSS/api.cpp` 中的 `MockShuffle` 只是交换首尾元素的 Graphiti 模拟代码，禁止作为 Protocol I shuffle 或性能数据来源。

### 4.3 Protocol III 与 DPF 路由

`Agarwal_TopK/protocol3_ca/` 提供 DCF conformance 和明文 AAV86 图测试，不是完整 Protocol III。

`ADSMPC/src/protocol3.cpp`、`RankingPhase.h` 和 `routing_dpf.h` 提供旧框架参考，但存在以下非目标行为：

- Dealer 用明文输入计算 `true_rank`；
- 多轮图在 Dealer 端按明文结果更新；
- 文件交换、固定 `sleep(1)` 和临时二进制文件同步；
- 密钥文件格式绑定旧 FSS 结构；
- 默认选择 AAV86，而不是全对全 Protocol III 排名。

这些代码只能提供局部算法和失败模式参考，不能作为端到端安全性或两轮复现的证据。

论文先给出两轮模块化 DPF 路由，与一轮 GRank 组合后形成三轮协议。最终 Protocol III 通过跨阶段压缩降为两轮，相关论证要求 payload 群为域，并使用非零 payload、非零乘法掩码及逆元。

VFSS 当前 `GroupElement` 使用 `uint64_t`，按位宽在 `Z_(2^b)` 上取模，不能直接视为域。M5 必须明确域表示、非零编码、运算适配和逆元条件，并验证与现有 DPF 接口的兼容性。

M3 已完成标准 DPF 与秘密共享乘法的三轮工程基线。它继续保留，不能通过修改标签或直接删除某一轮来宣称完成 M5。

仅针对单位 payload 或 mask 输出的简化，即使能够减少交互，也应标为独立的功能特化；不能据此宣称复现了满足论文完整前提的压缩路由。

### 4.4 AAV86 / Compare-Aggregate 升级

AAV86 升级依据论文 §5 的比较图与 CA 转换。算法在初始 shuffle 后，根据允许公开的局部 rank 自适应地产生后续比较图。

M6A 必须分别实现并测试：

1. Protocol I + AAV86。
2. Protocol III + AAV86。

两种方案共享统一输入、oracle 和计量契约，但分别记录路由、公开值、预处理、消息依赖和输出适配。

会议版 Theorem 5.1 给出 shuffle-based CA compiler 的 `2r+1` 轮结论。该结论不能直接转移为 Protocol III + AAV86 的定理。

Protocol III + AAV86 的 `2r` 保留为团队组合目标；实现前必须补齐组合协议、代数前提、预处理时序和泄露论证，未完成前不能写成已证明或已实现结论。

当前仍需解决的关键问题是：uCMP/DCF 材料绑定具体边的 mask difference，而后续比较边可能依赖前一轮在线公开状态。

现有参考材料仅支持以下边界：

- 图在离线包生成前已知时，可生成对应边的 CA 材料；
- B1 全对全路径及部分 AAV86 原型已有局部测试；
- 根据在线状态补发材料的 Dealer 路径改变了目标安全模型；
- 尚未闭合完整的 offline-only、自适应 exact-edge 预处理实现。

必须单独审计动态图材料的生成、绑定和消费。不得隐藏在线 Dealer，也不得用完整图预留冒充已经解决自适应 exact-edge 预处理。

完整图预留若作为独立对照，必须如实记录其离线材料与计算成本，不得混入目标方案的优化结论。

### 4.5 BB90+DCF 升级

M6B 在 M6A 两种升级实现及完整性能验收完成后推进，分别交付：

1. Protocol I + BB90+DCF。
2. Protocol III + BB90+DCF。

目标流程为：

```text
BB90 选择第 K 大元素对应的稳定优先级阈值
  → DCF 安全比较并生成 Top-K 成员指示共享
  → 必要的路由、逆映射和共享转换
  → 原始输入顺序下的秘密共享 Top-K bit-mask
```

“投票”在此表示每个输入位置是否属于 Top-K 的秘密共享成员指示。最终必须满足每位为 0/1 且恰好 K 位为 1，不引入近似选择或明文投票公开步骤。

必须遵循已冻结的稳定顺序。仅将原始 score 与第 K 大 score 作 `>=` 比较，在重复值情况下可能选择超过 K 个位置，不能作为完整实现。阈值表示及比较应包含正确的同分处理，并与原始下标语义一致。

需要明确：

- BB90 采用的具体算法版本、适用范围、迭代参数及随机性；
- 从目标顺序统计量到本项目最高优先级 rank 为 0 的映射；
- BB90 比较图如何转换为 CA 步骤；
- 第 K 大阈值的共享表示、允许公开值及与 DCF 的衔接；
- 阈值在在线阶段产生时，DCF 预处理如何保持输入无关；
- 原顺序 mask 的恢复与共享转换；
- Protocol I、Protocol III 两种路线分别保留或调整哪些阶段；
- 随机算法的正确性保证、成本保证和失败事件，不能将概率成本界与输出正确性混写。

Agarwal 会议版讨论 BB90 用于 selection，但没有给出本项目这两种 BB90+DCF Top-K mask 组合的完整协议。因此，两种组合初始均属于项目扩展或待验证候选。

其在线轮数必须从完整消息依赖推导并验证，不预先套用 `2r+1`、`2r` 或“BB90 轮数加一”。DCF 阈值比较、必要的表示转换和 mask 输出成本全部计入。

旧 `Direct Top-K`、pivot-pruning 或其他选择原型不能仅因目标相似而改名为 BB90。

### 4.6 CipherGPT 与 CryptoMoE 的范围边界

CipherGPT 的原生代码、历史测试和已知问题保留为参考资料。本轮不再安排：

- 原生 Top-K 修复；
- 原生 mask adapter；
- CipherGPT-style VFSS adapter；
- CipherGPT 性能实验；
- CipherGPT 与六种方案之间的正式性能比较。

CryptoMoE 继续作为未来应用工作负载来源，包括 router Top-k 和单 expert 的候选 Top-t dispatch。它不是本轮新增的 Top-K 密码学原语，也不构成继续实施 CipherGPT 的前置理由。

若后续接入 CryptoMoE，应先明确：

- eligibility、dummy 和有效候选不足时的顺序；
- 容量 `t`、drop 和 padding 语义；
- 允许公开的 routing transcript；
- 应用近似或容量限制与精确 Top-K oracle 的边界。

不得简单把 ineligible score 设为 0，因为合法 score 也可能量化为 0。不得将历史 Direct Top-K 原型的比较量泛化为一般性的 `O(Kn)`。

### 4.7 基础和条件性资料

- `FSS基础.pdf`：用于理解 FSS、DPF、DCF、correction word 和 full-domain evaluation；属于学习笔记，落地时以原论文和 VFSS conformance 为准。
- Boneh 等 2023：private heavy hitters、incremental/extractable DPF 和恶意客户端检查的背景资料，不是当前六种方案的直接实施任务。
- `协议2shuffle.pdf`（Ruffle）：不同安全模型下的条件性路线，不直接替换当前 2+1 半诚实 shuffle；未来采用时需要重新确认正式版本和模型。
- AAV86、BB90 原始文献：补充具体算法及复杂度来源，必须注明采用版本；不把会议版省略的细节表述为已经给出的完整实现。

## 5. 统一语义、测试与计量契约

### 5.1 已冻结的输入输出语义

统一语义保持不变：

- 输入为 32 位二补码 signed fixed-point score 的算术共享。
- 小数位数为 12，数值解释为量化整数除以 `2^12`。
- 两方 raw score shares 满足 `raw = (x0 + x1) mod 2^32`。
- 选择最大的 K 个 score，同分按 original index 升序。
- 最高优先级 rank 为 0，有效 rank 范围为 `0..n-1`。
- 统一输出为原始输入顺序下长度 n 的秘密共享 Top-K bit-mask。
- 每个输出位为 0/1，且 `Σz_i = K`。
- 不要求 Top-K 集合内部排序。
- score、rank、单边比较位、selected index 和最终 mask 不得通过测试辅助路径在 secure runtime 中公开。
- 具体协议允许的 shuffled rank 或局部 rank 公开必须有明确的接收方、置换隐藏条件和泄露说明，不能扩大为原顺序 rank 公开。
- 重构与 oracle 检查只在隔离的 `test` 路径发生。
- secure 模式使用新鲜随机性及一次性预处理材料。

统一接口为：

```text
([z_1]^B, ..., [z_n]^B)
    <- TopK(([x_1]^A, ..., [x_n]^A), K)
```

例如：

```text
X = (0.2, 0.9, 0.4, 0.8)
K = 2
Z = (0, 1, 0, 1)
```

本文的 n 表示一次 Top-K 调用的输入数量；应用文档中的 token 数 m 应在工作负载层映射，不能与单次协议规模混用。

六种方案必须使用同一输入语义、同分规则、oracle 和输出契约。selected payload 可以作为额外诊断或内部中间值，不能替代统一 mask。

所有方案均应维护原始位置绑定。输入编码、shuffle、路由、逆映射、共享转换及 mask 生成的实际成本全部计入相应阶段。

### 5.2 长期统一测试矩阵

固定用 K 表示 Top-K 数量，用 r 表示 AAV86 迭代次数。

| 输入数量 n | Top-K 数量 K | AAV86 迭代 r |
| --- | --- | --- |
| 128、256 | 2、8 | 2、3、4、5 |
| `10^3`、`10^4`、`10^5`、`10^6` | 80 | 2、3、4、5 |

要求：

- 六种方案沿用同一 `(n,K)` 矩阵。
- AAV86 两种路线均覆盖 `r=2,3,4,5`。
- BB90 迭代参数依据采用算法单独定义和记录，不直接复用 AAV86 字段。
- 同一比较组使用相同输入、输入种子、位宽、网络环境和 oracle。
- 算法随机种子与输入种子分别记录。
- 保留已冻结的基础随机量化整数分布 `[-32*2^12, 32*2^12]`，端点包含；其他分布单独标注。
- 正确性额外覆盖重复值、全相等、负数、正负边界、`K=1`、`K=n`、非二次幂 n 和非法参数。
- 正式性能实验分别覆盖 LAN/WAN，记录实际带宽、RTT、线程和机器配置。
- 每个配置预热 1 次、正式运行 5 次，保留逐次结果并报告 median/min/max。
- 论文理论值与历史结果可辅助核验，不能替代当前实现的实测。
- 无法运行的大规模点记录资源限制、失败阶段和原因，不用缩小参数、外推或旧数据填充原配置。

随机图导致实际工作量变化时，必须将每次运行的图参数、随机种子、边数、节点复杂度和性能结果关联保存。

### 5.3 长期统一指标与输出字段

每次正式运行必须记录以下指标：

| 字段 | 统一含义 | 单位/口径 |
| --- | --- | --- |
| `offline_time_ms` | 在线所需全部预处理的耗时 | ms；明确生成、序列化和分发边界 |
| `offline_material_total_bits` | 提供给在线执行的全部预处理材料之和 | bits |
| `online_time_ms` | 输入就绪至统一 Top-K mask 完成 | ms |
| `online_comm_total_bits` | 在线方实际发送字节折算后的总和 | bits，原始审计字段 |
| `online_comm_per_party_bits` | `online_comm_total_bits / 在线方数量` | bits，统一主报告字段 |
| `online_rounds` | 有因果依赖的在线交互轮数 | rounds |
| `online_prg_calls_total` | 所有在线方长度倍增 PRG 调用总数 | calls |
| `comparison_edges_total` | 实际执行的无序比较边总数 | comparisons |
| `total_time_ms` | `offline_time_ms + online_time_ms` | ms |

同时保留：

- 每方 `sent_bits`、`received_bits`；
- Git revision、实现标签和证据层级；
- runtime、party topology 和在线方数量；
- n、K、算法迭代参数；
- score 位宽、比较位宽、rank 域、payload 表示及 padding；
- 输入分布、输入种子、算法种子；
- CPU、内存、OS、编译器、flags、build type、线程数；
- 网络配置、带宽、RTT；
- 命令、预热次数、正式重复次数；
- correctness status、失败原因和原始结果定位信息。

统一通信表使用 per-party 展示，同时保留 total 与分方原始计数。total 只累加发送量；received 用于核验，不再加入 total，以免重复计算。

论文公式标为 “across both online parties” 时，应先映射到 total，再派生 per-party。bits、bytes、KB、KiB 必须注明转换方式。

离线材料和在线通信分开统计；密钥文件大小、内存对象大小和实际序列化材料大小不能混用。

同一条无序比较边由两方分别执行，不因此重复记为两条边；同一位置对在不同算法步骤实际重新比较时，按实际执行次数记录。DCF 调用数与比较边数分别保留。

AAV86 还必须记录实际：

- `e_A(n,r)`：各迭代比较边数之和；
- `v_A(n,r)`：各迭代参与节点数之和；
- 每轮通信、DCF/uCMP 调用及 PRG 调用。

满足 Theorem 5.1 对应构造条件时，将上述计数与定理成本项并列核验。

BB90+DCF 必须分别记录 BB90 选择阶段和最终 DCF 成员选择阶段的成本，包括迭代数、节点与边计数、DCF 调用、材料、通信、轮数和输出适配；不得只报告 BB90 图内部成本。

当前没有可信计数的指标写 `NOT_MEASURED`。理论换算只能出现在独立理论字段，不能填入实测 PRG 或通信字段。

### 5.4 论文核心与端到端边界

每种方案至少区分：

1. raw-score 输入适配；
2. 论文核心或明确标记的组合核心；
3. 原顺序 mask 输出适配；
4. 测试控制、报告传输和 oracle 验证等辅助工作。

主比较使用 raw-score 输入到统一 mask 完成的完整安全路径。论文核验另取功能及边界一致的核心数据，不把项目新增适配成本直接与论文原生成本混比。

不同阶段发生重叠或合并通信时，以真实消息依赖记录端到端轮数；不能机械相加重复计算，也不能把有因果依赖的消息合并称作一轮。

轮数目标分别为：

| 方案 | 核心轮数口径 |
| --- | --- |
| Protocol I | Theorem 4.1 的三轮目标 |
| Protocol III | Theorem 4.2 的两轮目标 |
| Protocol I + AAV86 | 对应 Theorem 5.1 构造的 `2r+1` 目标 |
| Protocol III + AAV86 | 团队 `2r` 组合目标，需独立论证 |
| Protocol I + BB90+DCF | 按完整组合消息依赖推导并实测 |
| Protocol III + BB90+DCF | 按完整组合消息依赖推导并实测 |

历史结果保持原计量边界。若历史 online time 包含输入分发或报告收集，应如实标明；新正式实验应分别记录可分离的辅助成本，不能静默重解释旧数字。

### 5.5 两次通信核验

新增两个必须完成的核验节点：

| 节点 | 时机 | 作用 |
| --- | --- | --- |
| Protocol I 通信核验 | M2 精确核心正确性和轮数审计完成后 | 核对论文核心成本，形成 M2→M5 交接证据 |
| Protocol III 通信核验 | M5 精确核心正确性和轮数审计完成后 | 核对压缩核心成本，形成进入图升级的基线证据 |

每次核验均需形成以下三层对照：

```text
论文成本公式
  ↔ 按实际参数、表示和消息推导的实现成本
  ↔ 独立进程执行得到的实际发送量
```

核验内容包括：

1. 明确比较的是 sorting、单个 order statistic，还是项目 Top-K mask。
2. 明确 n、逻辑/填充规模、输入域、rank 域、payload 和安全参数。
3. 列出论文公式、节号、脚注及采用的优化。
4. 逐阶段记录两方发送/接收量及因果轮数。
5. 将论文核心、输入适配、输出适配和传输封装分开解释。
6. 检查 total/per-party、bits/bytes 和发送/接收是否混淆。
7. 检查位宽提升、字节对齐、帧头、材料序列化和重复发送。
8. 检查随 n、K 变化的趋势，不只比较单个配置的数量级。
9. 配套运行 oracle differential 和独立进程 E2E，防止少做必要步骤导致通信“更优”。

Protocol I 以 Theorem 4.1 为主要核心成本来源。Protocol III 以 Theorem 4.2 为主要来源，并明确是否实现脚注中共用掩码的通信优化。

Table 2、Table 3 的参数、功能和归一化方式需逐项对齐；其中的估算数据不能当作本项目环境下的实测基准。

核验交付物至少包括：

- revision、实现标签、环境、输入和命令；
- 论文公式及参数代入说明；
- 实现消息清单与预计有效载荷；
- 原始分方、分阶段通信计数；
- 差异分解和未解决问题；
- 正确性、轮数与安全边界审计结果；
- 可复跑的核验入口。

核验通过表示计数可信、对应关系明确、关键差异有解释，不要求实际传输字节与理论 bit 数逐位相等。

尚有无法解释的协议成本或消息依赖差异时，不将其标为论文精确验收完成，也不使用该结果支持后续优化结论。资料研究和独立设计可以继续。

### 5.6 图升级的完整性能验收

M6A、M6B 各自包含完整性能验收，均覆盖对应的 Protocol I、Protocol III 两种实现。

验收要求：

- 统一矩阵中的全部配置均有处理记录；
- 可完成配置提供全部适用指标；
- 不能完成的配置记录失败或资源原因及未测字段；
- 正确性失败的配置不能计入成功性能比较；
- PRG、网络和阶段计量能力的缺口需要显式补齐或记录，不能用已有功能测试代替；
- 理论值、历史记录、当前实测和未测项明确区分；
- 原始结果可追溯到具体 revision、输入、种子和命令。

保留全部配置的失败记录不等于所有配置均成功完成。阶段报告必须明确实际覆盖率、缺失项及其对结论的限制。

## 6. VFSS 迁移原则

1. `VFSS/ext/FSS` 是统一 FSS 实现来源，不复制旧工程原语或密钥 ABI。
2. 通过最小适配使用 DCF、DPF、乘法和 shuffle；需要不同代数表示时先记录契约，不静默改变既有原语语义。
3. 新原语使用方式或适配器依次通过 conformance、oracle differential 和独立进程 E2E。
4. 所有预处理在当前实现下重新生成，不读取 ADSMPC、Agarwal_TopK 或 CipherGPT 的旧 key struct。
5. 复用已验证的 VFSS 通信与 framed transport；不迁移文件轮询、固定等待和明文 Dealer。
6. 不加入降级路径、启发式终止、特殊输入补丁或事后修正。
7. 统一 score、oracle、输入生成与指标定义只保留一份，不能为不同方案复制另一套语义。
8. 预处理材料必须记录 session、party、参数、阶段及一次性消费边界。
9. 图升级不能通过在线补发材料悄悄改变 Dealer 模型。
10. 只在实际需要时增加接口与文件，不预先建立大而全的抽象。

目录约定：

```text
VFSS/include/moe_topk/      稳定的最小公开接口
VFSS/src/moe_topk/          协议实现与必要适配
VFSS/tests/moe_topk/        conformance、differential、E2E
docs/decisions/             语义、安全、轮数和计量决策
docs/reproduction/         验收与复现记录
scripts/                   可复现构建和实验入口
artifacts/                 被忽略的本地产物
```

生成物目录本身不是验收证据。正式结论需有文档中的来源、命令、结果说明及可定位的原始记录。

## 7. 修订后的实施顺序

### M0：证据与命名基线

状态：已完成。

保留论文版本、来源边界、协议命名、Git 基线及历史审计。`VFSS-baseline/` 继续冻结。

### M1/M1.1：公共正确性与计量底座

状态：已完成并合入 main。

统一语义为 32 位二补码 signed fixed-point、小数位 12、数值降序、同分 original index 升序。

已完成 oracle、测试向量、CmpAgg、DCF conformance 和基础 metrics provenance。M1.1 在 Ubuntu 24.04.4 干净 Debug 构建中四项 CTest 通过，详见：

`docs/M1_1_UBUNTU_HANDOFF.md`

保留既有语义和测试。后续补充计量能力不应改变 oracle 或输入输出契约，也不反向修改历史未测字段。

### M2：Protocol I 精确核心与通信核验

状态：C 级工程基线已完成；Dealer-DPF 路线已停止；Chase-based R0
documentation/cleanup 已形成待评审 patch。严格 M2 G1、G2/G3 和 M5 runtime
仍为 gated sequence。

当前仅允许：明确标为 NON-EXACT 的 paper-aligned 实验复现、证据准备，以及 canonical gate 明确允许的 design-only 工作。严格 G1 仍等待 authoritative transcript/material/party-view/causal-round 证据；G2 仅在严格 G1 后，G3 仅在官方 G2 后，M5 runtime 仅在严格 M2 handoff 后。

严格 G1 解除后，未来任务：

1. 闭合 paper-compatible public masked-list shuffle 契约。
2. 验证同置换 payload、公开 masked list 与 GRank 材料绑定。
3. 接入全对全 CmpAgg、稳定 rank 和论文路由。
4. 审计并验证论文核心三轮消息依赖。
5. 保留 raw-score 输入与原顺序 mask 输出适配。
6. 通过 conformance、oracle differential 和独立进程 E2E。
7. 完成第 5.5 节 Protocol I 通信核验。
8. 提供可供 M5 复用的公共接口和计量交接材料。

交接至少包含接口版本、输入输出、材料生成与消费、调用示例、失败语义、阶段计数和核验报告。

只有论文条件、正确性、轮数、安全边界和通信核验均完成，才能将候选升级为精确基线身份。既有 C 级标签保持不变。

### M3：Protocol III 模块化三轮基线

状态：已完成并冻结。

保留：

```text
R1：GRank
R2：masked-rank DPF routing
R3：secure combine
```

以及 raw-score 两轮输入适配后的五轮扩展。

M3 继续作为 M5 的实现基础、正确性对照和压缩前基线，不重新安排为待实现任务，不用新目标覆盖历史三轮身份。

### M4：CipherGPT 原生基线

状态：取消。

本轮不实施、不修复、不适配、不运行 CipherGPT 性能实验。保留编号与历史资料，不作为 M5 或 M6 的前置条件。

### M5：Protocol III 精确两轮核心与通信核验

前置条件：

- M3 三轮基线及其验收记录稳定；
- M2 精确核心完成通信核验，所需公共接口完成交接。

允许提前进行域表示、非零编码、消息依赖和失败用例设计；依赖未冻结接口的核心实现与正式验收遵循上述顺序。

任务：

1. 明确满足论文条件的域及编码。
2. 定义非零 payload、乘法掩码和逆元失败语义。
3. 审计 M2 交接接口的可复用部分，不错误继承 ring-only 假设。
4. 实现 GRank 与 DPF routing 的跨阶段压缩。
5. 验证论文核心两轮，分别计量输入和 mask 适配。
6. 与 M3 使用同一输入、种子和 oracle 做差分。
7. 完成独立进程 E2E、泄露与材料时序审计。
8. 完成第 5.5 节 Protocol III 通信核验。

只有上述条件满足后，才使用 `agarwal_protocol_iii_exact_2round` 的精确身份，并进入依赖该基线的升级验收。

### M6A：AAV86 两种升级实现与完整性能测试

前置条件：M2、M5 精确基线及两次通信核验完成。

任务：

1. 固定 AAV86 算法、CA 转换、图生成及随机性规则。
2. 闭合输入无关、Dealer 在线静默的自适应预处理方案。
3. 分别实现 Protocol I + AAV86、Protocol III + AAV86。
4. 分别审计两种组合的公开值、轮数和输出适配。
5. 使用统一 oracle 验证精确 Top-K mask。
6. 对 `r=2,3,4,5` 执行统一矩阵。
7. 在 LAN/WAN 下测量全部统一指标及 `e_A`、`v_A`。
8. 与对应全对全基线比较计算、通信、轮数及离线成本。
9. 形成完整性能与未测配置报告。

M6A 完整验收后，再进入 M6B 的依赖实现和正式性能测试。BB90 资料研究可以提前进行。

### M6B：BB90+DCF 两种升级实现与完整性能测试

任务：

1. 固定 BB90 具体版本、参数、正确性和概率成本条件。
2. 明确稳定第 K 大阈值的表示与输出契约。
3. 设计并实现阈值到 DCF 成员选择的安全衔接。
4. 分别接入 Protocol I、Protocol III 路线。
5. 验证重复值、全相等、边界和原始下标语义。
6. 确保输出精确选择 K 个位置。
7. 审计动态图与在线阈值相关的预处理、泄露和消息依赖。
8. 执行统一 `(n,K)` 矩阵与算法适用的迭代参数。
9. 在 LAN/WAN 下测量全部统一指标。
10. 分别报告 BB90、DCF 和 mask 适配成本，并与全对全及 AAV86 方案比较。

两种组合均需独立实现标签和验收证据。不得将仅完成 BB90 第 K 大选择写成已完成整个 Top-K mask 协议。

### M7：六种方案统一汇总与报告

M7 汇总前述阶段已经完成的结果，并在实现、环境或计量边界变化时补跑必要配置。

主要比较组：

| 比较组 | 目的 |
| --- | --- |
| Protocol I vs Protocol III | 全对全条件下比较两条核心路线 |
| I vs I+AAV86 | 评估 Protocol I 路线的图排序升级 |
| III vs III+AAV86 | 评估 Protocol III 路线的图排序升级 |
| I vs I+BB90+DCF | 评估 Protocol I 路线的选择升级 |
| III vs III+BB90+DCF | 评估 Protocol III 路线的选择升级 |
| I+AAV86 vs I+BB90+DCF | 相同路线下比较排序后选择与阈值选择 |
| III+AAV86 vs III+BB90+DCF | 相同路线下比较两种升级算法 |
| I+AAV86 vs III+AAV86 | 相同图算法下比较两种组合路线 |
| I+BB90+DCF vs III+BB90+DCF | 相同选择算法下比较两种组合路线 |

工程对照另列：

- Protocol I 四轮核心工程基线与精确三轮核心；
- Protocol III 三轮模块化核心与精确两轮核心；
- 输入适配、mask 适配及传输封装的成本分解；
- 必要的 shuffle backend 历史或专项对照。

工程对照不增加六种主方案数量，不把 B0、模拟 shuffle 或不同安全模型结果混入正式速度排名。

报告规则：

- 统一功能、输入、输出和环境后再比较；
- 不同安全模型或泄露边界分栏说明；
- 主通信字段为 per-party，同时保留 total 和分方值；
- 分开报告 offline、online、core 和适配成本；
- 保留 median/min/max 和逐次结果；
- 每个数字标记为理论、历史、当前实测或未测；
- 不再包含 CipherGPT 实施或性能比较任务。

CryptoMoE 位于本阶段之后，另行确定工作负载和完成条件。

## 8. 验收门槛

### 8.1 正确性

- 原语使用方式和适配器 conformance 通过。
- 与冻结 oracle 差分一致。
- 独立进程 E2E 通过。
- 输出长度、二值性、`Σz_i=K`、稳定同分、原始位置及 payload 对齐正确。
- 覆盖重复值、全相等、负值、位宽边界、非二次幂和 K 边界。
- secure runtime 不依赖测试重构。
- 随机算法的正确性条件与随机成本条件分别说明。

### 8.2 安全性与材料边界

逐阶段记录输入、输出、接收方、公开元数据、秘密共享值、打开值、随机性、材料生成时机和 Dealer 行为。

改变 party 模型、泄露、代数表示或预处理时机时，必须同步决策与计划，不能仅更换实现标签。

材料应满足 session/party/参数绑定及一次性消费。自适应图和在线阈值不得引入未声明的在线 Dealer 或明文中间值。

### 8.3 轮数与通信

- 因果轮数可由消息依赖复算。
- 区分论文核心与端到端路径。
- total/per-party/分方计数可相互核验。
- 两次基础协议通信核验有完整报告。
- 未解释的偏差不能被“数量级接近”覆盖。
- 输出正确、成本一致和安全性是分别验收的条件。

### 8.4 性能与复现

正式记录包含第 5.3 节全部指标及 provenance。

M6A、M6B 分别完成两种实现的完整性能验收。未测数据不能用零、估算或旧实验结果替代。

资源不足、超时、崩溃和正确性失败应分别记录。含未测配置的阶段报告必须披露覆盖范围，不能宣称全矩阵成功。

## 9. 当前已确认状态

截至本次计划修订，已合入主线的状态为：

- M0、M1/M1.1 已完成，冻结基线继续保留。
- M1 score、tie rule、oracle、输入分布和基础 metrics 已冻结。
- M2 已完成 C 级 Protocol I 工程基线，当前核心四轮、raw-score 到 mask 共八轮。
- M2 精确三轮核心及相关 public masked-list 契约尚未在既有关闭记录中完成。
- M3 三轮模块化核心与 raw-score 五轮扩展已完成整改并冻结。
- M5 精确两轮核心及其通信核验是后续目标。
- VFSS 已有 DCF、DPF、CmpAgg、通信、两遍 shuffle 和独立进程测试基础。
- M3 已有分方 report 与结构化 MetricsRecord 汇总。
- 完整可信的在线 PRG 计数及正式 LAN/WAN 性能测量仍需补齐。
- AAV86 的自适应预处理与组合安全边界尚未闭合。
- BB90+DCF 两种组合尚不能视为已有完整实现。
- M4 CipherGPT 已从本轮实施与性能范围取消。

团队未来 gated 责任归属（当前不得执行 strict G1、官方 G2、G3 或 M5 runtime）为：

- 角色 A（搭档）：未来 gated 负责 strict Protocol I G1、官方 G2 与 G3；当前仅可进行 NON-EXACT 实验、证据准备和允许的 design-only 工作。
- 角色 B（你）：当前负责计划修订和 M5 design-only 准备；M5 runtime 是 strict M2 G3 后的 future gated 责任。
- 双方：交叉评审公共接口、语义、轮数与成本证据；后续分工按更新后的详细计划执行。

上述安排仅保留责任归属，不表示 strict 工作已解锁或当前可执行。主线状态只随代码、测试和验收证据更新。

## 10. 历史实现与验收记录

### 10.1 M2 工程基线关闭记录

M2.0–M2.16 documentation/validation closeout 完成了当前 C 级工程基线：

```text
m2_protocol_i_raw_score_input_modular_8round_mask_output
```

该路径为：

```text
2 轮 raw-score adapter
+ 4 轮当前核心
+ 2 轮 reverse mask adapter
= 8 轮
```

M2.16 完成的是精确目标的 feasibility/leakage audit，没有新增 paper-exact runtime 原语。

会议论文 §2.4/§4.1 所需的同置换 public `pi(x)+r`、任一单方未知且与后续 FSS gate 关联的 r，在该次审计的 VFSS PS/P2 API 中尚无完整可审计契约。

相关记录：

- `docs/decisions/M2_PROTOCOL_I_PAPER_EXACT_3ROUND_DESIGN.md`
- `docs/decisions/M2_PROTOCOL_I_EXACT_LEAKAGE_AUDIT.md`
- `docs/decisions/M2_PROTOCOL_I_PAPER_CORE_ALIGNMENT.md`

M2.16 文档随 `f800f96` 合入 main。该历史集成没有改变 C 级实现身份。

本次修订重新把精确核心推进和通信核验列为近期任务，但不反写历史关闭结论，也不把旧候选标签改成已完成。

### 10.2 M2 验证可靠性修复

后续合入的修复包括：

- chosen-OT adapter 在 `POLLIN|POLLHUP` 时先排空合法缓冲数据；
- modular E2E harness 在每个 case 关闭相关 socketpair 端点并回收 P0/P1/P2。

记录的 Ubuntu 24.04.4 fresh 构建在显式 soft `RLIMIT_NOFILE=1024` 下：

- EMP-ON：19/19 CTest 通过；
- EMP-OFF：13/13 CTest 通过；
- `(128,2/8)`、`(256,2/8)` 及重复 E2E 矩阵通过。

这些是工程验证证据，不是 paper-exact 或正式网络性能证据。相关记录：

- `docs/reproduction/M2_CHOSEN_OT_POLLHUP_UBUNTU_2026-09-06.md`
- `docs/reproduction/M2_MODULAR_E2E_FD_LIFECYCLE_UBUNTU_2026-09-06.md`

### 10.3 M3 复检关闭状态

M3 已在 `main@bb0d0e8` 完成代码整改。

| 正式实现 | 输入 | 在线轮数 |
| --- | --- | --- |
| `agarwal_protocol_iii_modular_3round` | padded priority-key shares | 3 |
| `moe_topk_protocol_iii_raw_score_modular_5round` | Q20.12 raw-score shares | 5 |

已完成能力包括：

- TEST_ONLY E2E 控制器通过 fork+exec 隔离 Dealer、Party 0、Party 1；
- logical-n GRank；
- DPF routing；
- secure combine；
- raw-score 安全入口；
- 两个正式 Party-role executable；
- 控制器汇总两方 report 并生成 MetricsRecord JSON；
- 11-test M3 validation matrix。

Dealer 是既有 TEST_ONLY E2E harness 中的独立角色，不应写成第三个正式 runtime executable。

详细证据见：

`docs/reproduction/M3_REVIEW_CLOSEOUT_UBUNTU_2026-09-10.md`

该记录中的测试结果、来源和计时边界保持原样。不能将其改写为本次计划修订重新运行的结果，也不能用后续目标覆盖三轮和五轮入口的不同身份。

### 10.4 历史计量边界

既有 M3 关闭记录已经披露：

- offline 时间包含 Dealer 启动、生成、序列化、发送和退出；
- online 时间使用控制器输入分发至两方 report 收集的边界；
- 网络 bandwidth/RTT 和完整在线 PRG 计数未测。

正式性能阶段应按第 5 节补充必要的分阶段计量。历史数字保留原标签与边界，不静默转换为论文核心或纯 runtime 时间。

## 11. 后续需要明确的具体输入

以下事项需要在对应阶段实施或正式实验前明确，不重新讨论已经冻结的 Top-K 语义：

1. Protocol I 当前开发分支、交接 revision、接口版本及通信核验入口。
2. Protocol I 核心与适配层的实际边界、位宽、payload 和材料格式。
3. Protocol III 精确压缩采用的域、非零编码、DPF 适配及安全条件。
4. 论文成本核验的具体功能、参数点和优化版本。
5. 正式实验机器、线程配置、LAN/WAN 条件、带宽和 RTT 测量方法。
6. AAV86 自适应预处理方案及 Protocol III 组合的消息和泄露论证。
7. BB90 原始资料版本、适用 K 范围、迭代参数、随机性及概率保证。
8. BB90 阈值到 DCF 的共享表示、预处理衔接和稳定同分处理。
9. 正式结果的保存位置、复跑命令和报告生成方式。
10. 大规模配置的资源上限、超时规则及失败记录方式。

Protocol III 精确两轮已是明确目标，不再将“是否只保留三轮基线”作为未决路线选择。若论文条件暂时无法满足，应记录具体缺口并保留准确的中间身份。

CryptoMoE 的模型参数、eligibility、容量、dummy、drop 和 transcript 需求留待 M7 之后单独确定。Ruffle、恶意安全 3PC 及 CipherGPT 不作为当前六种方案的前置任务。

总体计划、详细实施计划、两份分工计划及路线决策应保持一致。修改计划不等于完成实现；所有完成状态、论文一致性和性能结论必须对应可审计证据。
