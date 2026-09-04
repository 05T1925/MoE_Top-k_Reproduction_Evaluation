# MoE Top-K 安全协议统一项目

本仓库以 VFSS 为唯一活动实现框架，目标是建立可复现、可横向比较的安全 Top-K
基线。近期先完成 Agarwal Protocol I 与 CipherGPT 原生 Top-K 的统一输出和实验
对比；Protocol III 暂缓，AAV86、Direct Top-K 与 CryptoMoE 集成作为后续独立路线。

## 当前状态

- M0 已在远端闭环，`main` 与冻结标签均已推送；
- M1 已完成：统一 score 语义、oracle、CmpAgg、metrics 和真实 VFSS DCF
  conformance 测试均已通过；
- `VFSS/` 已产生 M1 的预期改动，`VFSS-baseline/` 仍保持冻结标签内容；
- M2 尚未开始，当前准备由两名队友分别推进 Protocol I/VFSS 与 CipherGPT 原生线；
- 统一输出固定为原始输入顺序下的秘密共享 Top-K bit-mask；
- 论文和大型参考工程不进入普通远端 Git 历史，需要队友在本地自行补齐。

## 文档入口

- [项目范围、论文映射与统一指标](PROJECT.md)
- [详细实施计划](docs/IMPLEMENTATION_PLAN.md)
- [M1 统一 score 语义（已冻结）](docs/decisions/M1_SCORE_SEMANTICS.md)
- [本地论文与参考仓库配置](docs/LOCAL_REFERENCES_SETUP.md)
- [M0/M1 仓库复检](docs/M0_REVIEW.md)
- [本地参考资料边界](docs/REFERENCE_MANIFEST.md)
- [项目实现约束](AGENTS.md)

## 目录说明

```text
VFSS/                 唯一活动实现目录
VFSS-baseline/        冻结恢复与回归基线
docs/                 计划、决策和来源记录
```

本机另有 `Papers/`、`ADSMPC/`、`Agarwal_TopK/` 和 `CipherGPT/` 作为只读参考。
这些目录被 `.gitignore` 排除；新环境不会自动拥有它们。需要共享时按
`docs/LOCAL_REFERENCES_SETUP.md` 放到固定位置并校验；需要重新分发时按
`docs/REFERENCE_MANIFEST.md` 选择经过许可证审查的方式。

## 协作起点

1. 先阅读 `PROJECT.md` 和 `AGENTS.md`；
2. 按 `docs/LOCAL_REFERENCES_SETUP.md` 准备自己分工所需的本地参考资料；
3. 确认工作基于最新 `main`，并知道 `vfss-baseline-2026-09-03` 只用于恢复比较；
4. 新协议代码只修改 `VFSS/`，不得修改 `VFSS-baseline/`；CipherGPT 原生线按实施
   计划先解决 source-only 可审查边界；
5. 每项工作按 `docs/IMPLEMENTATION_PLAN.md` 的输入、交付物和退出条件验收；
6. 性能结果必须使用统一测试矩阵和字段，不提交生成密钥、日志或本地构建产物。

## 环境说明

本机已使用 Apple Clang、CMake、Homebrew `libomp` 和 `eigen@3` 构建并通过 M1 四项
测试，复现命令见 `docs/decisions/M1_SCORE_SEMANTICS.md`。Linux/LAN/WAN 尚未验证；
每条协议实现必须继续记录实际编译器、依赖和网络环境。
