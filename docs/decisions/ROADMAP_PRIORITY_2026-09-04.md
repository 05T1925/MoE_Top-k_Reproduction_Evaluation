# 路线优先级决策（2026-09-04）

状态：**已采纳**。

## 决策

后续开发严格按以下依赖顺序推进：

1. M1 公共底座及 M1.1 收尾；
2. M2 Agarwal Protocol I 精确基线；
3. M3 Protocol III 模块化 3 轮基线；
4. M4 CipherGPT 原生基线；
5. M5 Protocol III 精确 2 轮压缩；
6. M6 AAV86 / Direct Top-K；
7. M7 统一实验与报告。

## 理由

- Protocol I 先固定真实 shuffle、全对全 CmpAgg、统一 mask 和 2+1 E2E，为后续
  Protocol III 复用公共执行与计量边界。
- Protocol III 先实现“GRank 1 轮 + 标准 DPF 路由 2 轮”的模块化 3 轮基线，能先
  验证接口、正确性和泄露边界，不把域表示和跨阶段压缩风险混入首个实现。
- CipherGPT 原码已有 Top-K 路径，但存在终止、同分、错误传播和 source-only
  分发问题；把它放在 3 轮基线之后不会阻塞 Agarwal 主线，也能减少过早修补参考
  原码对公共设计的影响。
- Protocol III 精确 2 轮必须在 3 轮基线正确后再处理域表示、非零 payload 和跨阶段
  压缩，避免把“可运行”误写成论文 Theorem 4.2 的复现。
- AAV86 的自适应 exact-edge 离线预处理仍未闭合，放在精确基线之后，保持基线与
  优化路线可区分。

## 阶段门

- M1.1 → M2：CTest 可统一运行，正式 metrics 能记录复现所需 provenance。
- M2 → M3：Protocol I 的真实 shuffle、mask 输出和独立进程 E2E 通过。
- M3 → M4：3 轮 Protocol III 与 oracle 差分通过，secure 路径无明文 rank/索引重构。
- M4 → M5：CipherGPT 原生基线身份、许可证边界和统一输出完成审查。
- M5 → M6：2 轮实现满足论文所需代数条件并完成消息、轮数和泄露审计。

不同人的资料研究和失败用例整理可以提前并行，但不得绕过上述合并门或改变主线
状态。CryptoMoE 暂作为未来工作负载接入，不进入当前六步实现主线。
