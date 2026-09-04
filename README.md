# MoE Top-K 安全协议统一项目

本仓库以 VFSS 为唯一活动实现框架，目标是建立可复现、可横向比较的安全 Top-K
基线。近期先完成 Agarwal Protocol I 与 CipherGPT 原生 Top-K 的统一输出和实验
对比；Protocol III 暂缓，AAV86、Direct Top-K 与 CryptoMoE 集成作为后续独立路线。

## 当前状态

- M0 仓库与规范基线已经建立；
- `VFSS/` 与 `VFSS-baseline/` 尚未分叉；
- 尚未开始在 VFSS 中迁移协议代码；
- 统一输出固定为原始输入顺序下的秘密共享 Top-K bit-mask；
- 论文和大型参考工程不进入首版远端 Git 历史。

## 文档入口

- [项目范围、论文映射与统一指标](PROJECT.md)
- [详细实施计划](docs/IMPLEMENTATION_PLAN.md)
- [M1 统一 score 语义（已冻结）](docs/decisions/M1_SCORE_SEMANTICS.md)
- [M0 上传前复检](docs/M0_REVIEW.md)
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
`docs/REFERENCE_MANIFEST.md` 选择经过许可证审查的方式。

## 协作起点

1. 先阅读 `PROJECT.md` 和 `AGENTS.md`；
2. 确认工作基于 `vfss-baseline-2026-09-03` 之后的最新主分支；
3. 新协议代码只修改 `VFSS/`，不得修改 `VFSS-baseline/`；
4. 每项工作按 `docs/IMPLEMENTATION_PLAN.md` 的输入、交付物和退出条件验收；
5. 性能结果必须使用统一测试矩阵和字段，不提交生成密钥、日志或本地构建产物。

## 环境说明

VFSS 当前 README 记录的基础依赖包括 Eigen3、CMake、支持 OpenMP 的 C++ 编译器。
M0 只建立仓库和规范基线，尚未在目标 Linux/LAN/WAN 环境完成新的协议构建验证；
首次实现任务必须把可复现构建命令和实际环境补入文档。
