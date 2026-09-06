# MoE Top-K 详细实施计划

本文是 `PROJECT.md` 的执行版。`PROJECT.md` 定义项目范围、论文边界和长期统一
指标；本文把工作拆成可分配、可验收的里程碑。若两者冲突，以更新后的
`PROJECT.md` 和团队明确决定为准，并同步修正文档，禁止仅在代码中形成隐含规则。

## 1. 当前结论

- M0：已在远端闭环；
- M1：核心已完成并同步远端，四项测试在 macOS 与 Ubuntu 24.04 通过；
- 当前开发主线：M1.1 已完成；M2.7 CmpAgg 三进程运行基础已作为项目扩展完成，
  但完整 Protocol I 仍须通过设计门、真实 shuffle、inverse routing 和统一 mask；
  M2.8 的项目扩展 `m2_emp_iknp_chosen_ot_conformance` 已在 Ubuntu-24.04 上完成固定
  EMP 依赖构建、upstream base-OT/IKNP smoke、C++20 隔离的 connected-fd adapter 和
  sender/receiver 独立进程 conformance；M2.9 项目扩展
  `m2_emp_opv_share_translation_conformance` 已在其上完成真实 GGM OPV 与 Share
  Translation 的独立进程 conformance；M2.10 项目扩展
  `m2_emp_single_pass_permute_share_conformance` 完成单遍 Permute+Share
  conformance；M2.11 项目扩展
  `m2_emp_two_pass_shuffle_roundtrip_conformance` 随后在独立 P0/P1 exec 进程中完成
  两遍前向/逆向 carrier roundtrip conformance。它仍不是完整 Protocol I，状态与
  M2.12 项目 E2E `m2_protocol_i_priority_key_input_small_e2e` 已将 priority-key
  additive shares、两遍 shuffle、CmpAgg、受控 shuffled rank 泄露和 reverse carrier
  串联为原顺序 XOR mask shares；M2.13 的 C 级候选
  `m2_protocol_i_modular_6round_mask_output` 闭合 P2 包先收后预处理的离线屏障、
  有界分片帧、逻辑/填充布局、全 rank-permutation 审计和 6 轮模块化 mask 输出；
  它仍不是 raw-score-share input 或论文 exact 基线，状态与
  可复现命令见
  `docs/decisions/M2_CHOSEN_OT_DEPENDENCY.md` 和
  `docs/decisions/M2_OPV_SHARE_TRANSLATION.md`、
  `docs/decisions/M2_PERMUTE_SHARE.md`、
  `docs/decisions/M2_SECRET_SHARED_SHUFFLE.md`、
  `docs/decisions/M2_PROTOCOL_I_SMALL_E2E.md`、
  `docs/reproduction/M2_PERMUTE_SHARE_UBUNTU_2026-09-06.md` 和
  `docs/reproduction/M2_PROTOCOL_I_SMALL_E2E_UBUNTU_2026-09-06.md`、
  `docs/reproduction/M2_SECRET_SHARED_SHUFFLE_UBUNTU_2026-09-06.md`；
  随后才是 M3 Protocol III 3 轮 → M4 CipherGPT → M5 Protocol III 2 轮 → M6 AAV86；
- 双人职责、并行边界和 M2 → M3 交接条件见 `docs/TEAM_WORK_PLAN.md`；
- CryptoMoE：移到 M7 统一实验之后，作为独立工作负载接入；
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
- 配置 SSH 远端并推送 `main` 与冻结标签；
- 在 GitHub 核验公开仓库与文档入口。

### 尚未决定

- 项目级 LICENSE 尚未添加；
- 论文和大型参考工程的公开再分发未获逐项确认，因此继续保持本地；
- 队友按 `docs/LOCAL_REFERENCES_SETUP.md` 自行准备获准使用的副本。

### 退出条件

以 `docs/M0_REVIEW.md` 第 6 节为准。

## 3.1 M1：统一 oracle、数据和计量底座

### 当前状态

核心已完成（2026-09-04）。团队已冻结 32-bit 二补码 fixed-point（scale=12）、测试量化整数均匀范围
`[-32*2^12, 32*2^12]`（端点包含）、数值降序及同分 original_index 升序。Oracle、
全对全 CmpAgg、测试向量、统一比较适配和计量记录均以此语义实现。现有 VFSS CMake
目标已实际构建并运行 `moe_topk_m1_oracle_test`、`moe_topk_m1_cmpagg_test`、
`moe_topk_m1_metrics_test` 与 `moe_topk_m1_dcf_conformance_test`；四者在 macOS 与
Ubuntu 24.04 均通过。

### M1.1 当前状态

已在 Ubuntu 24.04.4 LTS（WSL2、x86_64）的新 `/tmp` Debug 构建目录验收测试代码 revision
`a2efe5e3d2d22bb3c031fb24dc3246c37d442fad`：`ctest -N` 恰发现四项，
`ctest --output-on-failure` 为 4/4 通过。正式 metrics 已补齐输入 seed/分布、
编译器/flags、构建类型、CPU/内存/OS、网络、warmup 和 repetitions provenance；未测网络
和性能字段仍是 `NOT_MEASURED`。完整环境、命令、警告检查和 baseline 复检见
`docs/M1_1_UBUNTU_HANDOFF.md`。本验收只闭合 M1.1，不授权开始 M2，也不将 DPF
conformance 前置原语测试写成完整 M3 实现证据。

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

### M1.1 收尾门（已满足）

进入 M2 实现前完成：

1. 为四个 M1 可执行测试注册 CTest，确保 `ctest --output-on-failure` 是统一入口；
2. 给正式 metrics 记录补齐 seed、输入分布、编译器/flags、CPU/内存/OS、网络环境、
   warmup 和 repetitions；
3. 保留 raw per-party sent/received 计数，并验证派生 total/per-party 字段；
4. 在 Ubuntu 24.04 上执行干净 configure/build/ctest，并把命令和结果写入复现记录；
5. 再次确认 `VFSS-baseline/` 与冻结标签无差异。

M1.1 只闭合测试入口和复现元数据，不重新讨论已冻结的 score/tie/output 契约。该门已
满足；M2 代码仍受 `docs/decisions/M2_PROTOCOL_I_DESIGN_GATE.md` 的四项批准决策约束。

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

### M2.7 CmpAgg 三进程运行基础（已完成）

实现标签固定为 `m2_priority_cmpagg_three_process_e2e`，属于项目扩展（C），不等同于
`agarwal_protocol_i_exact_mask_output`。P2 在离线阶段为公开 canonical comparison
graph 生成 node-mask shares 和 party-separated VFSS uCMP/DCF edge material，分别发送
给 P0/P1 后退出；controller 仅在该 barrier 后发送 TEST_ONLY priority-key shares；
P0/P1 通过一个有界 framed masked-key exchange 计算 additive rank shares，controller
才在测试层重构并与 oracle 比较。

M2.7 的十个独立进程用例覆盖 `n=1,2,5,7,11`、`K=1`、`K=n`、`K=2 (n=11)`、非二次幂、
随机/重复/全相等值及 `INT32_MIN/MAX`，并通过 package/transport 错误矩阵。Ubuntu
24.04.4 全新 Debug 构建中 CTest 发现 11 项且全量 11/11 通过；实际命令、字节计数、
退出码和未测字段见
`docs/reproduction/M2_CMPAGG_PROCESS_E2E_UBUNTU_2026-09-05.md`。

该交付只闭合 CmpAgg process foundation：uCMP 是项目 two-evaluation adapter；secure
shuffle、inverse routing、最终原始顺序 bit-mask、完整 Protocol I 论文轮数/泄露与网络
性能仍未实现或为 `NOT_MEASURED`。因此不改变 M2 设计门的阻塞状态，也不提前进入 M3。

## 3.3 M3：Protocol III 模块化 3 轮基线

阶段、消息、表示、泄露、统一 mask 适配和实现门以
`docs/decisions/PROTOCOL_III_MODULAR_3ROUND_DESIGN.md` 为准。

### 输入

- M1 全对全 CmpAgg、oracle、测试输入与指标；
- M2 已冻结的运行、通信和统一 mask 边界；
- Agarwal Protocol III 论文定义；
- ADSMPC 旧原型仅作调用次序和失败模式参考。

### 任务

1. 画清 GRank 1 轮与标准 DPF 路由 2 轮的角色、预处理和消息依赖。
2. 复用 M1 CmpAgg，以 VFSS DPF + 秘密共享乘法实现模块化路由。
3. 删除明文 `true_rank` Dealer、文件轮询、固定 `sleep` 和调试重构依赖。
4. 把 payload 恢复到原始输入位置，输出统一秘密共享 Top-K bit-mask。
5. 分离 `test`/`secure` 模式并接入统一 metrics。
6. 使用独立进程完成 Dealer、Party 0、Party 1 的小规模 E2E。

### 交付物

- `agarwal_protocol_iii_modular_3round` 可执行目标；
- GRank/DPF routing conformance 与 oracle differential 测试；
- 阶段、消息、打开值、泄露和 3 轮因果关系审计；
- 小规模原始运行记录。

### 退出条件

- 重复值、全相等、负值、`K=1/K=n` 和非 2 次幂输入均与 oracle 一致；
- secure 模式不重构 rank、DPF index 或 selected indices；
- 在线因果轮数可复算为 3，额外 mask adapter 轮数单独记录；
- 不使用 `agarwal_protocol_iii_exact_2round` 名称或相关性能声明。

## 3.4 M4：CipherGPT 原生基线

### 输入

- M1 统一契约、输入和指标；
- CipherGPT 论文及原生仓库；
- `Top_K_paper`、`Top_K_paper_test` 和原生 shuffle。

### 任务

1. 先确认原始代码来源、revision、许可证和 source-only 可审查边界。
2. 修正输入错误传播，禁止只打印错误后返回不完整结果。
3. 建立对所有输入都会收缩的 QuickSelect 不变量；解决全相等和重复值，不用轮数
   上限掩盖不终止。
4. 与项目统一 tie rule 对齐，并说明它是输出语义适配还是论文原生行为。
5. shuffle 时绑定 original index，最终输出秘密共享 bit-mask。
6. 将验证模式重构与 secure 输出分离，保留原生两方密码学栈。

### 交付物与退出条件

- `ciphergpt_native_mask_output` 基线；
- 重复值、全相等、错误输入和 `K` 边界回归测试；
- 小规模原始运行记录及明确的 runtime/topology 字段；
- 不依赖启发式轮数上限也能终止并返回恰好 `K` 个位置；
- 统一 mask 与 oracle 一致，计时覆盖索引恢复和 mask 生成。

## 3.5 M5：Protocol III 精确 2 轮压缩

### 前置条件

M3 的 3 轮实现通过差分、独立进程 E2E、消息流和泄露审计。

### 任务

1. 选择并记录满足论文要求的域表示，不能继续把 `Z_(2^b)` 当作域。
2. 定义非零 payload 编码、零值处理和逆元失败语义。
3. 实现 GRank 与 DPF 路由的跨阶段压缩，证明并实测在线因果轮数为 2。
4. 分别计量 paper core 和统一 mask adapter，主结果报告两者总和。
5. 与 M3 使用同一输入和 oracle 做逐项差分，并运行独立进程 E2E。

### 交付物与退出条件

- `agarwal_protocol_iii_exact_2round` 可执行目标；
- 域、非零 payload、消息流、泄露和轮数决策记录；
- M3/M5 正确性与开销对照；
- 只有论文前置条件和 2 轮审计均通过后，才能标为 Theorem 4.2 复现。

## 3.6 M6：AAV86 / Direct Top-K 实验

### 首要研究问题

后续比较图由已公开局部 rank 决定，而 uCMP/DCF 密钥绑定具体 mask difference。
必须先给出输入无关、Dealer 在线静默的自适应预处理算法。在线 Dealer 原型只能
作为不同安全模型的实验，不得标为 Theorem 5.1 实现。

### 任务

1. 写明每轮公开值、bucket 状态、比较图生成时机和 Dealer 行为。
2. 解决一般性的 exact-edge 自适应预处理，不采用固定完整图预留等局部补丁。
3. 对 `r=2,3,4,5` 运行统一矩阵。
4. 实测 `e_A(n,r)`、`v_A(n,r)`、PRG 调用和在线轮数。
5. 在 LAN/WAN 分别比较轮数增加与比较量下降的拐点。
6. Direct Top-K 使用独立标签和安全说明，不与 AAV86 full-sort 混名。

### 交付物与退出条件

- 预处理与泄露说明；
- `aav86_ca_experimental` 和必要时 `direct_topk_experimental` 目标；
- 固定矩阵的原始与汇总结果；
- 只有 offline-only、安全审查和轮数验证通过后，才升级论文一致性标签。

## 3.7 M7：统一性能报告

### 比较组

1. B0 vs B1：只比较 shuffle backend；
2. Protocol I vs Protocol III 3 轮：统一 CmpAgg，比较 shuffle 与 DPF routing；
3. Protocol III 3 轮 vs 2 轮：统一功能，比较压缩前后的轮数与代数代价；
4. Protocol I/III vs CipherGPT native：统一输入/输出/指标，拓扑与运行时分栏；
5. Protocol I B1 全对全 vs AAV86 full-sort：比较排名算法；
6. AAV86 full-sort vs Direct Top-K：相同输入、计划和 transcript 口径；
7. CipherGPT native vs 未来 VFSS adapter：不同实现身份，不合并结果。

### 报告规则

- 数字只标记为理论值、历史记录、当前实测或 `NOT_MEASURED`；
- 主通信字段为 per-party，保留 total 和分方原始值；
- 图表必须能追溯到原始运行文件、revision、环境和命令；
- 不把不同安全模型、输出功能或计时边界放进同一速度排名。

CryptoMoE 位于 M7 之后：先冻结 eligibility、dummy、容量 `t` 和允许公开的 routing
transcript，再从已经验收的精确 Top-K 后端开始接入。

## 4. 两人协作方案

两人从同一个最新 `main` 建立独立分支，不直接在 `main` 上并行修改。先自行选择
角色 A/B，选择结果记录在首个 PR 描述中，不在代码里写个人姓名。

### 4.1 角色 A：公共底座收尾与 Protocol I（M1.1/M2）

负责：

- 完成 CTest 注册、metrics provenance 和 Ubuntu 干净复现；
- 依据 M2 顺序梳理 Dealer、Party 0、Party 1 的离线材料和三轮在线消息；
- 在 `VFSS/` 中实现真实 B1 secret-shared shuffle 的最小路径；
- 接入全对全 CmpAgg、稳定 rank 和统一 bit-mask 输出；
- 增加 Protocol I 单元、差分和独立进程 E2E 测试；
- 记录每个阶段的公开值、轮数、通信和 mask adapter 开销。

主要写入边界：

```text
VFSS/include/moe_topk/       仅新增 M2 实际需要的接口
VFSS/src/moe_topk/           Protocol I 与必要运行绑定
VFSS/tests/moe_topk/         Protocol I 测试
docs/decisions/              Protocol I 消息流和安全边界
```

不得修改 `VFSS-baseline/`，不得把 `Agarwal_TopK/` 代码树或旧 FSS ABI 复制进来。

### 4.2 角色 B：Protocol III 设计与实现（M3/M5）

M2 合并前可先做只读设计与测试准备；M2 的公共运行边界冻结后再合并实现：

- 把论文 Protocol III 拆成 GRank、标准 DPF 路由和跨阶段压缩三个可审计阶段；
- 为 M3 写消息流、3 轮因果关系、打开值/泄露和独立进程 E2E 方案；
- 复用公共 CmpAgg、oracle 与 metrics，不复制另一套 score/tie/output 语义；
- 先交付 `agarwal_protocol_iii_modular_3round`，通过后再研究 M5 的域表示、非零
  payload 和 2 轮压缩；
- 对 ADSMPC 只做证据映射，不迁移明文 Dealer、文件同步或旧 key layout；
- CipherGPT 的资料、许可证和失败用例可以提前整理，但实现合并服从 M4 顺序。

主要写入边界：

```text
VFSS/include/moe_topk/       仅新增 M3/M5 实际需要的 DPF/routing 接口
VFSS/src/moe_topk/           Protocol III 与必要运行绑定
VFSS/tests/moe_topk/         DPF routing、差分和 E2E 测试
docs/decisions/              Protocol III 阶段、代数条件、轮数与泄露
```

角色 B 不在 M3 中提前实现 2 轮压缩，也不为了推进样例而修改 M1 冻结语义。

### 4.3 共享资产与所有权

以下 M1 文件已经冻结，任何一方需要修改都必须先说明对另一条线的影响并由双方
复核：

```text
VFSS/include/moe_topk/score_semantics.h
VFSS/include/moe_topk/topk_oracle.h
VFSS/include/moe_topk/metrics.h
docs/decisions/M1_SCORE_SEMANTICS.md
PROJECT.md
```

- 共享输入、种子和 oracle 只保留一份；禁止两条线复制后各自改变 tie rule；
- 角色 A 不改 Protocol III 路由实现，角色 B 不改 Protocol I shuffle 实现；
- 两个 PR 不同时修改 `README.md` 或 `IMPLEMENTATION_PLAN.md` 的同一区域；状态汇总由
  后合并者在代码 PR 通过后单独更新；
- 原始性能输出留在被忽略目录，进入文档的数字必须带 revision、环境和运行命令；
- 跨协议共用代码只有第二个真实调用方出现后再提取，不提前建立大而全抽象。

### 4.4 合并顺序

1. 先合并本次路线、协作边界和 PR 模板；
2. M1.1 已合并；角色 A 先完成并获批准的 M2 实施前设计门，角色 B 可并行提交不改代码的 M3 消息流设计；
3. 角色 A 的 M2 实现满足退出条件后合并；
4. 角色 B 基于冻结的 M2 公共边界合并 M3 模块化 3 轮实现；
5. M4 CipherGPT 原生基线完成后，再进入 M5 2 轮压缩；
6. M5 通过后才合并 M6 AAV86/Direct Top-K，实现 PR 不顺带改另一协议。

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

1. 评审并批准 M2 Protocol I 实施前设计门的四项决策；
2. 批准后，角色 A 才可创建 M2 代码；首个 M2 PR 先提交已批准的消息流、依赖映射和最小测试骨架；
3. M2 通过阶段门后，角色 B 基于最新 `main` 开始 M3 模块化 3 轮实现；
4. 两人每次合并前确认共享输入、统一 mask、metrics 和冻结 baseline 没有分叉。
