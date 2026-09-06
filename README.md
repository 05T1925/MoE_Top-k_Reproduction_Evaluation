# MoE Top-K 安全协议统一项目

本仓库以 VFSS 为唯一活动实现框架，目标是建立可复现、可横向比较的安全 Top-K
基线。长期顺序已经冻结为：公共底座 → Protocol I → Protocol III 模块化 3 轮
基线 → CipherGPT 原生基线 → Protocol III 精确 2 轮压缩 → AAV86。

## 当前状态

- M2.16 的首次审计提交为 `5fbc24f9dd67c1275bf52c1c9701c01859951df9`，其文档已在
  分支 `m2.16-paper-exact-3round-protocol-i` 推送；截至 2026-09-06，
  `origin/main` 仍为 `6c72ec8a18a44c1d3d441758017b9807fe1dc090`，M2.16 尚未合入
  `main`，需经文档 PR 审核后再合并；
- M0 已在远端闭环，`main` 与冻结标签均已推送；
- M1 核心已完成：score 语义、oracle、CmpAgg、metrics 和真实 VFSS DCF
  conformance 测试均通过；
- M1.1 已在 Ubuntu 24.04.4 的干净 Debug 构建中验收：`ctest -N` 恰发现四项，
  `ctest --output-on-failure` 为 4/4 通过；验收记录见
  `docs/M1_1_UBUNTU_HANDOFF.md`；M1/M1.1 的语义、oracle、metrics 和冻结基线
  已完成并保持不变；
- `VFSS/` 已产生 M1 的预期改动，`VFSS-baseline/` 仍保持冻结标签内容；
- M2.0--M2.15 已完成当前 C 级模块化 Protocol I 工程基线，并已合入 `main`；
  当前实现标签为 `m2_protocol_i_raw_score_input_modular_8round_mask_output`，
  core 为 4 轮、总路径为 8 轮；
- M2 未达成 Agarwal paper-exact 3-round core 或 7-round total candidate，这些
  仍是明确记录的后续研究目标，不得反写成当前实现能力；
- M2.16 已完成 paper-exact 3-round feasibility/leakage audit：论文要求的同置换
  public `pi(x)+r` 与当前 VFSS PS/GRank material 绑定仍无可审计实现，因此保留
  8-round C 级基线；
- M3 Protocol III 模块化 3 轮是当前实现主线，可从最新 `main` 开始；
- M2.16 合并后不改变 M3 入口契约；在合并前，队友可继续进行 M3 只读设计和测试准备，
  但实现分支应以合并后的 `main` 为起点；
- 统一输出固定为原始输入顺序下的秘密共享 Top-K bit-mask；
- 论文和大型参考工程不进入普通远端 Git 历史，需要队友在本地自行补齐。

## 文档入口

- [项目范围、论文映射与统一指标](PROJECT.md)
- [详细实施计划](docs/IMPLEMENTATION_PLAN.md)
- [双人实施分工与交接计划](docs/TEAM_WORK_PLAN.md)
- [M2 分支阶段映射与重命名记录](docs/BRANCH_MAP.md)
- [路线优先级决策](docs/decisions/ROADMAP_PRIORITY_2026-09-04.md)
- [Protocol III 模块化 3 轮设计](docs/decisions/PROTOCOL_III_MODULAR_3ROUND_DESIGN.md)
- [M1 统一 score 语义（已冻结）](docs/decisions/M1_SCORE_SEMANTICS.md)
- [M1.1 Ubuntu 24.04 验收记录](docs/M1_1_UBUNTU_HANDOFF.md)
- [M2 Protocol I 实施前设计门](docs/decisions/M2_PROTOCOL_I_DESIGN_GATE.md)
- [M2.16 paper-exact 3-round 阻塞规格](docs/decisions/M2_PROTOCOL_I_PAPER_EXACT_3ROUND_DESIGN.md)
- [M2.16 paper-exact 泄露审计](docs/decisions/M2_PROTOCOL_I_EXACT_LEAKAGE_AUDIT.md)
- [M2.16 Ubuntu 研究记录](docs/reproduction/M2_PROTOCOL_I_PAPER_EXACT_3ROUND_UBUNTU_2026-09-06.md)
- [本地论文与参考仓库配置](docs/LOCAL_REFERENCES_SETUP.md)
- [M0/M1 仓库复检](docs/M0_REVIEW.md)
- [本地参考资料边界](docs/REFERENCE_MANIFEST.md)
- [项目实现约束](AGENTS.md)
- [协议复现工作流](.agents/skills/protocol-reproduction/SKILL.md)

## 目录说明

```text
VFSS/                 唯一活动实现目录
VFSS-baseline/        冻结恢复与回归基线
docs/                 计划、决策和来源记录
.agents/skills/       仓库级 Codex 工作流
```

本机另有 `Papers/`、`ADSMPC/`、`Agarwal_TopK/` 和 `CipherGPT/` 作为只读参考。
这些目录被 `.gitignore` 排除；新环境不会自动拥有它们。需要共享时按
`docs/LOCAL_REFERENCES_SETUP.md` 放到固定位置并校验；需要重新分发时按
`docs/REFERENCE_MANIFEST.md` 选择经过许可证审查的方式。

## 协作起点

1. 先阅读 `PROJECT.md`、`AGENTS.md` 和当前里程碑的决策记录；
2. 按 `docs/LOCAL_REFERENCES_SETUP.md` 准备自己分工所需的本地参考资料；
3. 确认工作基于最新 `main`，并知道 `vfss-baseline-2026-09-03` 只用于恢复比较；
4. 新协议代码只修改 `VFSS/`，不得修改 `VFSS-baseline/`；
5. 每项工作按 `docs/IMPLEMENTATION_PLAN.md` 的输入、交付物和退出条件验收；
6. PR 使用仓库模板，性能结果使用统一测试矩阵和字段，不提交生成密钥、日志或
   本地构建产物。

## 已验证环境

- Apple Clang、CMake、Homebrew `libomp` 与 `eigen@3`：M1 四个目标通过；
- Ubuntu 24.04.4、GCC 13.3、CMake、Eigen 3.4、OpenMP 4.5：M1 四个目标通过；
- LAN/WAN 性能尚未测量，不能从本机功能测试推导通信或时延结论。

复现命令见 `docs/decisions/M1_SCORE_SEMANTICS.md`。
