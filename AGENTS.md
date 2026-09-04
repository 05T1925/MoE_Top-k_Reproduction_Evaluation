# MoE Top-K 仓库执行约束

## 开始工作前

- 先读 `PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md` 和当前里程碑对应的
  `docs/decisions/`；发现冲突时先暴露不确定性，不自行猜测论文含义。
- 区分四类证据：论文定义、本地参考行为、项目扩展、待验证设想。代码行为不能
  反向写成论文结论。

## 实现边界

- `VFSS/` 是唯一活动实现目录；`VFSS-baseline/` 是冻结恢复基线，禁止修改。
- `Agarwal_TopK/`、`ADSMPC/`、`CipherGPT/` 和 `Papers/` 只作本地参考；不复制旧
  FSS ABI、生成密钥、文件轮询或在线 Dealer 依赖。
- 只实现当前里程碑需要的最小接口。禁止吞错误、一次性抽象、降级路径、启发式
  兜底、特殊样例补丁和事后修正。
- `secure` 路径不得重构 rank、比较位、selected index 或明文校验数据；重构只在
  明确隔离的 `test` 路径发生。

## 正确性与计量

- 新原语/适配器依次通过 conformance、oracle differential 和独立进程 E2E。
- 统一输出必须是原始输入顺序下的秘密共享 Top-K bit-mask，并遵循已冻结的
  Q20.12 signed score 与稳定同分语义。
- 性能数字必须能追溯到 revision、实现标签、输入/种子、环境、命令、重复次数和
  原始计数；未实测写 `NOT_MEASURED`，不得估算或借用旧结果填充。
- 改变输入语义、输出、角色、轮数、泄露、预处理时机或指标边界时，同步更新决策
  文档与实施计划。

## Git 与评审

- 从最新 `main` 建短生命周期分支；一个 PR 只处理一个里程碑或一个明确的治理变更。
- 提交前运行相关测试与 `git diff --check`，并确认 `VFSS-baseline/`、密钥、论文、
  构建物、日志和本地参考工程未进入差异。
- 评审优先拦截：冻结基线变化、无证据的论文一致性声明、模拟 shuffle、文件同步、
  在线 Dealer、测试重构进入 secure 路径，以及由零值代替的未测指标。
