# 项目全局进度与后续阶段

日期：2026-09-19
状态：规划口径；不是代码覆盖率、性能结果或论文一致性声明。

## 总体结论

按下表的里程碑权重，本项目当前约为 **55%** 完成。这个数字衡量的是“统一安全 Top-K
项目”从基础设施到可比较实验/工作负载接入的全流程，不把已有提交数或代码行数当作
进度。它包含已完成的 M0、M1、M2 C 类工程基线和 M3 模块化基线，也显式扣除了未完成的
M2 paper-exact、M4--M7 与 CryptoMoE。

该估计的可复算值为 `20 + 15 + 3 + 15 + 1.5 = 54.5`，四舍五入为 55%。其中 M2
paper-exact 的 3 分和 M5 的 1.5 分只代表已完成的资料/设计工作，**不代表可执行的
论文精确实现**。

## 阶段总览

| 阶段 | 权重 | 状态 | 已完成的可审计交付物 | 未完成的退出条件 |
|---|---:|---|---|---|
| M0：证据、命名与冻结基线 | 5 | 完成 | 引用边界、PDF/参考资料清单、`VFSS-baseline` 冻结 | 无；后续只做维护性复检 |
| M1/M1.1：公共正确性底座 | 15 | 完成 | Q20.12 语义、stable tie、oracle、CmpAgg、metrics、DCF conformance、Ubuntu 验收 | 性能/LAN/WAN 仍不属于该阶段的已测结论 |
| M2：Protocol I C 类工程基线 | 15 | 完成 | 8-round raw-score mask baseline、shuffle/transport/material、独立进程与 Ubuntu/EMP 记录 | 不升级为论文 exact |
| M2-exact：Protocol I 论文校准 | 15 | 阻塞中的研究收尾（20%） | 会议版的原生输出、Dealer 模型、stable tie、`π(x)+r` 高层证据已定位 | 逐轮 transcript、party view、`r` material、same-permutation、padding/dummy、full version/proof |
| M3：Protocol III 模块化接口适配 | 15 | 完成 | 3-round priority-key 与 5-round raw-score路径、DPF routing、secure combine、三方 E2E | 2-round 压缩不属于 M3，转入 M5 |
| M4：CipherGPT 原生基线 | 10 | 未开始 | 已有范围、风险和验收计划 | source/revision/license 审计、终止/错误语义、index 绑定、mask adapter、差分/E2E |
| M5：Protocol III 论文精确 2-round | 10 | 设计准备（15%） | paper-evidence、field contract 与预实现检查清单 | 域与非零 payload 前置条件、2-round transcript、实现、泄露/轮数审计、E2E |
| M6：AAV86 / Direct Top-K | 7.5 | 未开始 | 已识别自适应 exact-edge 预处理门 | offline-only 预处理、安全模型、runtime、差分/E2E |
| M7：统一性能报告 | 5 | 未开始 | 指标口径和比较矩阵已定义 | 在同一环境实际采样、原始记录、可比较报告 |
| CryptoMoE 工作负载接入 | 2.5 | 未开始 | 已定义为 M7 之后的独立层 | eligibility/dummy/capacity/leakage 契约与已验收后端 |

## 已完成与未完成的边界

已完成不等于论文精确复现：M2、M3 当前均是名称和轮数已冻结的工程基线/扩展；其中
M2 的论文精确 Protocol I 被资料与 material/transcript 证据阻塞，M3 的论文精确
2-round 压缩是独立的 M5。所有性能字段若未来自已保存的实际运行，仍为
`NOT_MEASURED`。

M2 的证据收尾详见
`docs/decisions/M2_PROTOCOL_I_PAPER_EVIDENCE_SUPPLEMENT_2026-09-19.md`；M2/M3
代码与测试交接详见 `docs/M2_M3_TEAMMATE_HANDOFF_2026-09-19.md` 和
`docs/reproduction/M2_M3_STAGE3N_FINAL_CLOSEOUT_2026-09-15.md`。

## 建议执行顺序

1. **M2-exact 只取证，不补猜代码。** 向作者/导师取得 full version 或逐轮算法；先完成
   sender/receiver、字段、公开值、party view、`π/r` 生命周期和 adapter 计量表。
2. **M4 启动 CipherGPT native baseline。** 先解决 license/source、错误传播、终止性和
   stable/index/mask 语义，再做差分和独立进程测试。
3. **M5 仅在 field contract 可满足后进入实现。** 不将 `Z_(2^b)` 当 field，不为零
   payload 或逆元失败增加隐式降级路径。
4. **M6 先解决自适应预处理。** 禁止 online Dealer 或以完整图预分配冒充 AAV86 的
   exact-edge 节省。
5. **M7 最后采集统一性能。** 保留 revision、命令、环境、输入/种子、重复次数和原始
   计数；CryptoMoE 只在此后接入。

## 分支命名规则

新分支使用功能前缀：`docs/`、`design/`、`feat/`、`fix/`、`test/`、`perf/`、`review/`。
不再创建 `codex/` 前缀分支；历史同类 ref 的迁移见 `docs/BRANCH_MAP.md`。一个分支只承载
一个里程碑或明确治理变更，所有实现分支从最新 `origin/main` 开始。
