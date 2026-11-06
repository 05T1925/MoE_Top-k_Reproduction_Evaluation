# M6B-I-PROJECT-SELECT-5ITER-S18 决策记录

日期：2026-10-16
路线：Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL

## 裁决

- S18 由未参与 S15–S17 实现、证明或决策的隔离接收者代理，按 S15 的 72 项来源清单完成了来源校验、fresh Release ON/OFF 构建、default-OFF 检查和定向功能复核。该接收只覆盖 S15 默认关闭修复后的精确源码快照，不覆盖 S16/S17 文档、S18 新改文档或任何新 runtime revision。
- S18 接收不是 S17 全部 25 个 E2E case 的复跑。独立复跑为 12 个 BCG21 smoke success、一个 N=128 monolithic success、六个 BCG21 uniform-abort 负例，以及一个旧 M2UCv1 crash-step 5 用例；后者仅支持历史 no-complete-mask 合同，不是 BCG21 专用 crash 证据。
- 未发现可复现 runtime/API/material/output 合同缺陷；本轮未修改 VFSS 代码或测试，也未修改 S15–S17 原始报告、case JSON 或证据。
- S15 实际 DCF 的单 key 结论保持 CONDITIONAL：它仅是 BCG+21 Figure 1/Theorem 2 条件下的 party-key-only 投影，依赖 AES-128 PRP 及未量化的 epsilon_RNG_joint。它不自动模拟 UCMP transcript。
- 首个联合证明缺口是 key-only 定理到真实 UCMP 调用视图的条件替换：尚未证明，在给定腐化方自己的输入 share、同一调用的两个已打开 masked operands、该方自己的相关 mask/material 辅助信息和比较 bit 后，真实 party key/Eval-share 联合分布可由仅使用获准 L 的模拟器替换。即使该单调用引理成立，还需分别证明两种材料 schema 的 adaptive full-pool hybrid，再证明共同 forward/inverse shuffle、membership carrier、输出 share 和 crash/lease 的联合条件分布。
- S17 的 M2CM/v1 P0/P1 simulator 是候选，不是证明。其双向 shuffle、membership 与输出 share 联合条件分布未经非作者解析证明；未发现可复现攻击，但 SHUFFLE_OUTPUT_COMPOSITION 与 JOINT_PARTY_VIEW 仍 OPEN。
- no-complete-mask 继续是唯一已测试的崩溃输出合同。step 5 可留下 P1 的本地 8-byte XOR share；不声称原子发布或 all-or-nothing。
- 保持 EXPERIMENTAL，不创建 secure alias，不把该路线改记为 BB90/BMW16，也不替代六方案计划里的 BB90 项。N>256 为 NO_GO。M6A V3 仍 OPEN，V4 仍 NOT_ALLOWED/NOT_RUN；正式性能矩阵未运行。

## 最小后续任务

由未参与 S15–S18 的证明作者与独立 reviewer，先为单个真实 M2B1/v1 UCMP 调用证明或反驳：在实际 alpha 与两方 mask-share 生成关系下，party key、两次 Eval、opened masked operands 和 opened comparison bit 的条件联合分布，能否仅用腐化方自身 input share、理想 output share 与获准泄露 L 模拟。若该引理成立，再对 N=128 的 M6B5/v3 + M2CM/v1 + M6P2/v1 与 N=256 的 M6S2/v2 分别扩展到已用、未用、拒绝槽、自适应地址及完整 shuffle/inverse/output/crash transcript。先用 n=2、3 枚举作反例筛查，最终必须提交解析证明，不能以测试替代。

详见 S18 复核报告、gate JSON 与 source/build/evidence manifest。S18 仅接收 S15 精确运行时，不表示 S15 的安全证明已通过。
