# MoE Top-K 详细实施计划

本文是 `PROJECT.md` 的执行版。`PROJECT.md` 定义项目范围、论文边界和长期统一
指标；本文把工作拆成可分配、可验收的里程碑。若两者冲突，以更新后的
`PROJECT.md` 和团队明确决定为准，并同步修正文档，禁止仅在代码中形成隐含规则。

## 1. 当前结论

- M0：本地复检完成，等待所有者提交并推送 GitHub；
- 当前开发主线：M1 → M2 → M3 → M4；
- 近期比较对象：Agarwal Protocol I 与 CipherGPT 原生 Top-K；
- Protocol III：暂缓，保留为 M5；
- CryptoMoE：在安全 Top-K 基线和统一实验稳定后进入 M6；
- 任何 AAV86/Direct Top-K 原型在解决自适应预处理前不得标为论文定理实现。

## 2. 全程不变的统一契约

### 2.1 输入和输出

- 输入：`m` 个 32 位 score 的算术共享 `[x_j]^A` 和公开 Top-K 数量 `K`；
- 顺序：选择最大的 `K` 个 score，同分按原始下标打破；
- 输出：原始输入顺序下 `m` 个布尔共享 `[z_j]^B`；
- 正确性：每个 `z_j` 是 0/1，`Σz_j=K`，掩码为 1 的位置精确对应 Top-K；
- 不要求 Top-K 集合内部排序；
- selected values/payload 只能是附加诊断，不能代替 bit-mask。

若原生论文接口不是 bit-mask，必须增加明确的 `mask_output` 适配层。元素与原始
索引绑定、shuffle、逆映射、共享转换和 mask 生成都属于被测协议路径，时间和通信
不得从主结果中扣除。

### 2.2 测试矩阵

| `n` | `K` | AAV86 迭代 `r` |
| --- | --- | --- |
| 128、256 | 2、8 | 2、3、4、5 |
| `10^3`、`10^4`、`10^5`、`10^6` | 80 | 2、3、4、5 |

同一比较组必须使用相同输入文件/种子、位宽、score 解释、网络环境和 oracle。
无法运行的大规模点标记 `NOT_MEASURED` 并保存失败原因，禁止外推填表。

### 2.3 必须输出的指标

- `offline_time_ms`；
- `offline_material_total_bits`；
- `online_time_ms`；
- `online_comm_total_bits`；
- `online_comm_per_party_bits`，作为主表通信字段；
- 每方 `sent_bits`、`received_bits`；
- `online_rounds`；
- `online_prg_calls_total`；
- `comparison_edges_total`；
- `total_time_ms = offline_time_ms + online_time_ms`；
- 运行时、拓扑、`n/K/r`、位宽、线程、网络、revision 和 correctness status。

每个配置预热 1 次、正式运行 5 次，保存原始运行记录并报告 median/min/max。

## 3.0 M0：仓库与规范基线

### 目标

建立可以安全上传和协作的最小远端仓库，不混入大型参考工程、论文和生成物。

### 已完成

- 冻结 `VFSS-baseline/`；
- 建立提交 `993696e` 和标签 `vfss-baseline-2026-09-03`；
- 确认 `VFSS/` 与冻结树的 131 个源文件一致；
- 中文化项目总纲；
- 固定协议命名、证据层级、bit-mask 输出、测试矩阵和性能字段；
- 忽略论文、参考工程、构建产物、密钥、日志和临时输出；
- 建立根 README、来源清单和本实施计划。

### 待所有者完成

- 确认仓库可见性和许可证；
- 配置 GitHub remote；
- 复核 staged diff，提交并推送 `main` 与冻结标签。

### 退出条件

以 `docs/M0_REVIEW.md` 第 6 节为准。

## 3.1 M1：统一 oracle、数据和计量底座

### 输入

- `PROJECT.md` 第 5 节契约；
- VFSS 现有 GroupElement、DCF、通信和统计接口；
- Protocol I 本地测试向量与论文稳定 rank 定义。

### 任务

1. 明确 32 位 score 是 unsigned、signed two's complement 还是定点数，并记录
   scale；未决定前不写比较适配代码。
2. 实现最小明文 oracle：稳定 Top-K → 原始位置 bit-mask。
3. 建立固定测试数据格式和种子记录；覆盖随机、重复、全相等、负值、`K=1`、
   `K=n`、非 2 次幂 `n` 和位宽边界。
4. 为 Protocol I 需要的 VFSS DCF 比较语义建立 conformance test。
5. 定义统一运行记录，字段严格对应第 2.3 节；不先抽象尚未出现的协议能力。
6. 建立计时边界和通信计数验证，确认 total 与 per-party 能从同一原始记录得到。
7. 实现明文全对全 CmpAgg rank，并与 oracle 差分。

### 交付物

- oracle 源码和单元测试；
- 统一输入向量及生成说明；
- DCF conformance test；
- 指标记录结构和一个明文示例结果；
- score 编码与 tie rule 决策记录。

### 退出条件

- 所有边界向量得到长度为 `m`、二值、和为 `K` 的正确 mask；
- DCF 测试覆盖等于阈值和位宽边界；
- 同一原始通信记录可复算 total/per-party；
- 没有测试专用明文值进入 secure 接口。

## 3.2 M2：Agarwal Protocol I 精确核心与统一输出

### 输入

- M1 oracle、测试数据和计量；
- Chase secret-shared shuffle 论文；
- `Agarwal_TopK/protocol1/` B0 功能参考和 `protocol1_ca/` B1 shuffle 材料；
- VFSS DCF 与通信接口。

### 任务

1. 先画清 Dealer、Party 0、Party 1 的离线材料和三轮在线消息，不照搬旧文件协议。
2. 迁移最小 B1 两次 Permute+Share shuffle；每个元素绑定 score、original index
   和必要 payload。
3. 接入全对全 CmpAgg，计算稳定 rank。
4. 生成原始顺序下的布尔共享 Top-K mask；单独记录 paper core 和 mask adapter
   开销，主结果使用两者总和。
5. 删除对 B0 置换矩阵、`MockShuffle`、文件轮询和明文 rank 的运行依赖。
6. 用独立进程运行 Dealer + 两个在线方。

### 测试顺序

1. shuffle permutation/payload 对齐；
2. 两方各自置换角色和随机性；
3. CmpAgg stable rank；
4. mask adapter；
5. 小规模完整 E2E；
6. `(128,2/8)`、`(256,2/8)` 正式基线。

### 交付物

- `agarwal_protocol_i_exact_mask_output` 可执行目标；
- secure/test 两种明确模式；
- 独立进程 runner；
- 原始指标记录和测试报告；
- 阶段/消息/公开值审计文档。

### 退出条件

- 与 oracle 全部差分通过；
- secure 模式不重构 rank、comparison bits 或 selected indices；
- 角色为 2+1，在线轮数和额外 mask 适配轮数分别可解释；
- 所有统一指标来自实际计数，不使用模拟 shuffle 成本。

## 3.3 M3：CipherGPT 原生基线与 Protocol I 对比

### 输入

- M1 统一契约、输入和指标；
- CipherGPT 论文及原生仓库；
- `Top_K_paper`、`Top_K_paper_test` 和原生 shuffle。

### 任务

1. 修正输入错误传播，禁止只打印错误后返回不完整结果。
2. 给 QuickSelect 建立对所有输入都会收缩的算法不变量；解决全相等和重复值，
   不用轮数上限掩盖不终止。
3. 与项目统一 tie rule 对齐，并明确这是输出语义适配还是原论文行为。
4. shuffle 时绑定 original index，最终输出秘密共享 bit-mask。
5. 将验证模式重构与 secure 输出分离。
6. 保留 CipherGPT 原生两方密码学栈，不为了“统一框架”提前改写成 VFSS。

### 对比方法

- 同一批 32 位输入；
- 先完成 `n=128,256` 与 `K=2,8`；
- 再尝试 `n=10^3..10^6, K=80`；
- Protocol I 记录 2+1，CipherGPT 记录两方，不用 per-party 归一化隐藏拓扑差异；
- 输出、正确性和指标字段统一，运行时和安全模型分栏展示。

### 交付物

- `ciphergpt_native_mask_output` 基线；
- Protocol I 与 CipherGPT 的统一小规模结果表；
- 原始 5 次运行记录；
- 重复值、全相等和错误输入回归测试。

### 退出条件

- 不依赖启发式轮数上限也能终止并返回恰好 `K` 个位置；
- 统一 mask 与 oracle 一致；
- 两个实现的计时边界都覆盖索引恢复和 mask 生成；
- 结果表明确 topology、runtime 和历史/当前实测状态。

## 3.4 M4：Protocol I + AAV86 和 Direct Top-K 实验

### 前置条件

M2 的安全 shuffle、比较原语、mask 输出和计量均已冻结。

### 首要研究问题

后续比较图由已公开局部 rank 决定，而 uCMP/DCF 密钥绑定具体 mask difference。
必须先给出输入无关、Dealer 在线静默的自适应预处理算法。在线 Dealer 原型只能
作为不同安全模型的实验，不得标为 Theorem 5.1 实现。

### 任务

1. 写明每轮公开值、bucket 状态、比较图生成时机和 Dealer 行为。
2. 解决一般性的 exact-edge 自适应预处理，不采用固定完整图预留等局部补丁。
3. 对 `r=2,3,4,5` 运行统一矩阵。
4. 实测 `e_A(n,r)`、`v_A(n,r)`、PRG 调用和 `2r+1` 在线轮数。
5. 在 LAN/WAN 分别比较轮数增加与比较量下降的拐点。
6. Direct Top-K 使用独立标签和安全说明，不与 AAV86 full-sort 混名。

### 交付物与退出条件

- 预处理与泄露说明；
- `aav86_ca_experimental` 和必要时 `direct_topk_experimental` 目标；
- 固定矩阵的原始与汇总结果；
- 只有 offline-only、安全审查和轮数验证通过后，才升级论文一致性标签。

## 3.5 M5：暂缓的 Protocol III

### 恢复条件

Protocol I 与 CipherGPT 统一对比完成，团队重新确认资源投入和 payload 域策略。

### 两步路线

1. `agarwal_protocol_iii_modular_3round`：全对全 CmpAgg + 标准 DPF + Beaver
   路由；移除旧原型的明文 `true_rank`、文件同步和调试重构。
2. `agarwal_protocol_iii_exact_2round`：解决域表示、非零 payload 编码及跨阶段
   压缩后，验证论文 Theorem 4.2。

Protocol III + AAV86 的 `2r` 是团队目标，不是当前会议版已给出的独立 theorem。
必须另行完成协议组合、预处理时序和泄露证明。

## 3.6 M6：CryptoMoE 工作负载

### 前置决策

- 模型的 `m/n/k/t`；
- score 编码、eligibility 和 dummy 的严格顺序；
- 允许公开的 routing transcript；
- 容量溢出 drop 是否属于模型语义。

### 顺序

1. 单 expert 候选 Top-t；
2. 多 expert dispatch；
3. one-hot retrieval 与 combine；
4. 最后连接 router 和 HE MatMul。

CryptoMoE 容量 Top-t 与精确 Top-K oracle 分层实现；不得用 score=0 的 dummy
启发式处理零分合法候选。

## 3.7 M7：统一性能报告

### 比较组

1. B0 vs B1：只比较 shuffle backend；
2. Protocol I B1 全对全 vs AAV86 full-sort：比较排名算法；
3. AAV86 full-sort vs Direct Top-K：相同输入、计划和 transcript 口径；
4. Protocol I vs CipherGPT native：统一输入/输出/指标，拓扑与运行时分栏；
5. CipherGPT native vs 未来 VFSS adapter：不同实现身份，不合并结果。

### 报告规则

- 数字只标记为理论值、历史记录、当前实测或 `NOT_MEASURED`；
- 主通信字段为 per-party，保留 total 和分方原始值；
- 图表必须能追溯到原始运行文件、revision、环境和命令；
- 不把不同安全模型、输出功能或计时边界放进同一速度排名。

## 4. 并行协作建议

在不制造重复抽象的前提下，可以并行以下工作：

- 契约/测试负责人：M1 oracle、向量、指标验证；
- Protocol I 负责人：M2 shuffle 与 CmpAgg；
- CipherGPT 负责人：M3 终止、tie rule 和 mask adapter；
- 实验负责人：固定输入、环境记录和 runner，但不提前虚构尚未产生的字段；
- 安全复核负责人：逐阶段记录公开值、Dealer 行为和测试专用重构。

每项代码任务只修改其当前需要的最小接口。跨协议共用代码只有在第二个真实调用方
出现后再提取；不得为了计划中的未来协议一次性建立大而全框架。

## 5. 每个合并请求的检查项

- 说明对应里程碑和协议阶段；
- 列出输入、输出、公开值和 party 角色变化；
- 列出新增/修改测试及实际运行结果；
- 说明是否影响统一输出、泄露、轮数或指标边界；
- 不修改 `VFSS-baseline/`；
- 不加入参考仓库源码、论文、构建产物、密钥或日志；
- 不吞错误，不加入启发式兜底和仅对当前样例有效的后处理；
- 文档状态必须与代码实际状态同步。

## 6. 立即下一步

1. 所有者完成 M0 的 GitHub 提交与推送；
2. 团队决定 32 位 score 的 signedness、定点 scale 和输入分布；
3. 在 M1 只建立 oracle、测试向量、DCF conformance 和指标最小结构；
4. M1 退出条件全部通过后，再开始 Protocol I 的真实安全 shuffle 迁移。
