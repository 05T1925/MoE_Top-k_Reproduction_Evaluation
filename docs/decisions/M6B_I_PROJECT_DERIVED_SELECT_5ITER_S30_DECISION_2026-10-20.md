# M6B-I-PROJECT-SELECT-5ITER-S30 决策

日期：2026-10-20
路线：PROJECT_DERIVED_SELECT_5ITER（项目派生五迭代 Select；不是 BB90/BMW16 原算法复现）
审查对象：S25 manifest 固定的 74-file candidate；canonical SHA-256 64f32993b12bcd8692ed089f5dcdb8ce1be9f181ff217b508b8b0470eb8e440d。

## 决议

1. S25 identity 为 PASS：按 S25 manifest 对 74 个文件逐一重算，全部匹配；manifest SHA-256 为 c96450eb62b186e866291b6c4415441e0ef3216703612314542e1d2ee25372eb。被审对象是 S25 worktree 快照，不以基础提交 c3926c68fd14f270faa8b55234311071947fa080 代替。
2. S25 功能接收为 PASS_WITH_SCOPE，只继承 S27 对相同 manifest 的历史收据；S30 不重跑矩阵，不把该功能接收称作完整安全证明。
3. 从 S25 的 inverse-shuffle e0/e1 随机 pad ±h 和输出 word1 LSB，可在理想独立随机带下推出逐字节 share law：每方 share 均匀，XOR 重构为代码实际生成的 M_runtime；不把完整 mask 给任何一方。该 law 与 F_TOPK-U 的输出分布形式相同，但 M_runtime 对所有允许输入/随机带等于理想稳定 Top-K M 尚未证明；S27 有限 E2E 收据不足以替代全域正确性证明。getrandom/OpenSSL RAND_bytes 的联合优势 epsilon_rng_joint 未量化。
4. 输出前错误的抽象理想语义是统一 ABORT，但具体异常/校验映射待冻结。Crash/abort 输出合同标记 NEEDS_OWNER_DECISION：候选为逐方 API 返回语义 F_TOPK-PREFIX，或需要额外原子发布机制的 F_TOPK-ATOMIC。S25 的 NO_COMPLETE_MASK 探针不能推出原子无输出或 all-or-nothing；本决议不擅自选择。
5. P0、P1 的 Sim_b(x_b,z_b,pp,L) 完整 party-view 模拟均为 OPEN/NO_GO。Dealer package generator 可从新随机带无条件重采样；首个停止点是不能把此 package law 联到固定 (x_b,z_b,pp,L) 下的线上 carry/sign DCF key+mask、消息 frames 与 Eval transcript 条件联合分布，边界位于 protocol_i_score_input.cpp:90-139。已找到的是证明缺口和未量化条件分布；没有可复现区分器，也没有 v2 不可能性证明。
6. 模块组合仍 OPEN/NO_GO。若 F_scoreconv/v2 把 Q′_b 交给腐化方，组合后的功能扩大了可见输出；若隐藏 Q′_b，则完整最终输出条件 sampler 仍未提供。不得用新增子功能泄露制造原外层功能 PASS。
7. 后续路线选择为转入版本化 score-to-mask-v3 设计规格。候选后端是两方半诚实 GMW 与离线可信 T 直接生成的一次性 XOR-shared Boolean Beaver triples；候选方案不使用 OT。若具体协议实现固定并完成 GMW transcript、逐 gate preprocessing/package、declassification、used/unused 材料和 crash-prefix simulator/hybrid 证明，新的协议证明才可能覆盖旧 v2 条件分布断点。当前尚未实例化定理，不宣称断点已消除。
8. v3 仅达到 DESIGN_SPEC_ONLY。它不是已经证明可安全的实现。本轮不改 VFSS runtime，不准入 v3 代码；也不宣布 v2 数学上不可能安全。
9. v3 实现前有两项明确 owner gate：
   - 冻结 F_TOPK-PREFIX 或 F_TOPK-ATOMIC，并定义错误、材料消费和每个 crash 前缀的理想输出；
   - 确认新 GMW 消息 DAG 下通信长度及既有 declassification 字段仍满足已批准 L；若不满足，先更新决策与理想功能。
   随后须给出 P0/P1 simulator、preprocessing/package/crash 组合证明及量化 preprocessing、PRG/DRBG、联合 RNG、TLS 和 hybrid 优势项。当前可信 T 直接生成 Beaver triples，候选不计 epsilon_OT。未来密码学证明应由未参与 S19–S30 的审查者独立复核。
10. S25 N=128 M6B5/v4 与 N=256 M6S2/v3 整池、自适应 DCF 地址、shuffle/inverse、membership、output/crash 组合均未由本轮模拟，状态为 NOT_ATTEMPTED_AFTER_STOP_CONDITION。没有运行正式 LAN/WAN 性能矩阵；性能为 NOT_MEASURED。没有创建 secure alias。

## 独立审查范围

独立接收者 /root/s30_independent_receiver 未参与 S19–S29 的证明撰写、决策或实现，并对本轮 S30 做了只读审查。其独立复核覆盖 S25 identity、输出 share law 与全输入正确性边界、crash probe 长度头范围、v2 条件分布停止点、v3 preprocessing/GMW 假设及泄露边界；修正已回写本报告、gates 和决策；接收者已对修订版完成最终只读复核，确认指定边界准确且未发现新缺陷。该只读审查不替代密码学证明。

## 本轮修改

仅新增治理文档：

- docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S30_REPORT_2026-10-20.md
- docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S30_GATES_2026-10-20.json
- docs/decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S30_DECISION_2026-10-20.md

运行时代码与 VFSS-baseline/ 均未修改。
