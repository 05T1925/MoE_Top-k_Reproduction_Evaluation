# 双人实施分工与交接计划

状态：**已采纳**。

本文在 `docs/IMPLEMENTATION_PLAN.md` 的里程碑顺序下明确双人职责、并行边界和
交接条件，不改变已冻结的 score、tie-break、输出语义、协议身份或路线优先级。

## 1. 分工与并行规则

- 角色 A（搭档）：负责 M1.1 公共底座收尾，并在其合并后负责 M2 Protocol I
  精确基线；
- 角色 B（Protocol III 负责人）：负责 M3 的协议设计、DPF 适配验证和模块化
  3 轮实现，后续负责 M5 精确 2 轮压缩；
- 两人可以并行进行论文核对、设计和失败用例整理，不要求角色 B 等待 M2 全部完成
  才开始工作；
- 代码合并门保持 `M1.1 → M2 Protocol I → M3 Protocol III 模块化 3 轮`；
- 每项工作从最新 `main` 建立短生命周期分支，一个 PR 只覆盖一个里程碑或一个明确
  的治理变更，不直接向 `main` 提交。

## 2. 角色 A：M1.1 与 Protocol I

### 2.1 M1.1 公共底座收尾

当前状态：M1.1 已在 Ubuntu 24.04.4 LTS（WSL2）的全新 Debug 构建目录通过 CTest 4/4；
测试代码 revision 为 `a2efe5e3d2d22bb3c031fb24dc3246c37d442fad`，详细记录见
`docs/M1_1_UBUNTU_HANDOFF.md`。这只闭合 M1.1 → M2 的公共测试/provenance 门，
不改变 M2 → M3 阶段门，亦不构成 M3 实现、路由或性能证据。

1. 为四个 M1 测试注册 CTest，使 `ctest --output-on-failure` 成为统一测试入口；
2. 给正式 metrics 补齐 seed、输入分布、编译器/flags、CPU、内存、操作系统、
   网络环境、warmup 和 repetitions；
3. 保留每方 sent/received 原始计数，并验证 total/per-party 派生字段；
4. 在 Ubuntu 24.04 上完成干净 configure、build、ctest，并记录复现命令和结果；
5. 确认 `VFSS-baseline/` 与 `vfss-baseline-2026-09-03` 冻结标签无差异。

M1.1 只完成测试入口和复现元数据，不重新讨论或修改已冻结语义。

### 2.2 M2 Protocol I 精确基线

1. 先提交 Dealer、Party 0、Party 1 的离线材料、在线消息和公开值设计；
2. 实现真实的秘密共享 shuffle，不使用 `MockShuffle` 或 B0 置换矩阵代替；
3. 实现全对全 CmpAgg、稳定 rank 和原始顺序下的秘密共享 Top-K mask；
4. 提供独立进程 Dealer、Party 0、Party 1 端到端运行；
5. 依次通过 shuffle payload 对齐、角色随机性、stable rank、mask adapter、
   小规模 E2E，以及 `(128,2/8)`、`(256,2/8)` 基线测试。

交付目标为 `agarwal_protocol_i_exact_mask_output`。secure 路径不得使用文件轮询、
固定 sleep、明文 rank、selected index 或调试重构。

## 3. 角色 B：Protocol III

### 3.1 可立即开展的设计

1. 只使用通过 `docs/PAPERS.sha256` 校验的论文和测试标准；哈希不匹配的本地资料
   不作为规范来源；
2. 画清 GRank 1 轮与标准 DPF 路由 2 轮的角色、离线材料、消息依赖、公开值和
   泄露边界；
3. 明确离线 Dealer 负责生成 DPF keys、掩码和乘法相关随机材料，且不依赖在线
   Dealer；
4. 记录 rank 语义映射：仓库规定最高优先级 `rank=0`，因此 Top-K 目标为
   `0..K-1`，不得直接照搬旧 ADSMPC 的 `n-K..n-1`；
5. 把旧原型中的明文 `true_rank`、文件交换、固定 sleep 和结果重构列为禁止复用
   模式。

### 3.2 M1.1 合并后的 DPF 适配验证

1. 基于 VFSS 的 `keyGenDPF`、`evalDPF_EQ` 和 `evalDPF_Payload` 建立最小适配层；
2. rank 域覆盖 `0..n-1`，并明确非 2 次幂输入的域宽与填充规则；
3. 覆盖索引 `0`、`n-1`、非 2 次幂规模，以及 payload 为 `0`、`1` 和一般非零值；
4. 只允许测试代码重构双方输出，以验证 share 合并后等于期望结果；
5. 在 M2 接口冻结前不修改其公共接口或共享 metrics 结构。

### 3.3 M2 通过后的 M3 实现

1. 复用 M1 CmpAgg、oracle、测试输入和统一计量；
2. 使用标准 VFSS DPF 与项目采用的秘密共享乘法实现模块化路由；
3. 输出长度为 `n`、保持原始输入顺序的秘密共享 Top-K bit-mask；
4. 提供独立进程 Dealer、Party 0、Party 1 小规模 E2E；
5. 输出标签固定为 `agarwal_protocol_iii_modular_3round`，本阶段不实现论文精确
   2 轮压缩。

## 4. M2 → M3 交接契约

Protocol I 在进入 M3 前必须冻结以下行为边界：

- 输入为仓库规定的 Q20.12 signed score 算术共享；
- stable rank 按 score 降序、同分 original index 升序，最高优先级 rank 为 0；
- rank 的有效范围是 `0..n-1`；
- 最终输出是原始输入顺序下、长度为 `n` 的秘密共享 Top-K bit-mask；
- 运行、通信、轮数和 metrics 接口足以被 M3 复用，额外 mask adapter 成本可单独
  解释但不得从主结果扣除。

角色 A 在 M2 PR 合并前提供接口说明和最小调用示例；角色 B 在其基础上 rebase，
不得复制或分叉出第二套 rank、mask 或计量语义。

## 5. 联合验收

- 正确性覆盖重复值、全相等、负值、`K=1`、`K=n` 和非 2 次幂输入；
- 协议输出与 M1 oracle 差分一致，每个 mask 位为 0/1 且总和为 K；
- Dealer、Party 0、Party 1 以独立进程运行；
- M3 在线因果轮数可复算为 3，额外适配轮数必须单独披露；
- secure 路径中不出现明文 rank、DPF index、selected index、文件通信、固定 sleep
  或调试重构；
- 所有性能字段来自实际记录；未测量项目写 `NOT_MEASURED`，不得估算。

文档设计可以提前并行合并；实现代码仍严格通过 M1.1、M2、M3 的阶段门。
