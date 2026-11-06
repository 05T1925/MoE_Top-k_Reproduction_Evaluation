# M6B-I-PROJECT-SELECT-5ITER-S23 决策记录

日期：2026-10-16
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL`

## 决策

1. 运行时对象仍为 S18 锁定的 S15 默认关闭修复快照（72 文件，canonical SHA-256 `c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498`）。S23 未修改 `VFSS/`，未重新构建或运行功能测试；S18 `PASS_WITH_SCOPE` 仅是历史功能接收，不是 S23 新测试，也不构成安全接收。
2. S23 执行者参与了 S19–S22 的推导/报告工作，故 `INDEPENDENT_CRYPTO_REVIEW=NOT_SATISFIED`。本文是范围受限的源码与代数复核，不是非作者签核。
3. 实际 `protocol_i_raw_score_input_party` 返回的是 R_(2^(33+h)) 中的优先级键加法份额 `Q_b`，不是随机均匀份额，也不是最终 Top-K mask share。源代码没有名为 `F_scoreconv` 的理想功能或明确 share-law API。S23 将“协议诱导输出分布”定义为候选语义，不能冒称已冻结的理想合同。
4. 在 DCF correctness 前提下，`Q_0+Q_1` 等于 stable priority key；给定本方 `Q_b`，`q_b=Q_b>>h`（33 位）及 `key_records[i]=(Q_b[i],0,0)` 是确定的。由源码还可知 `Q_0[i] mod 2^h=i`、`Q_1[i] mod 2^h=0`，且每个 `Q_b[i]` 的低 32 个 q 位由本方 `x_b[i]` 确定。它不服从均匀 R_(2^(33+h)) share law。
5. 实际 M2CM/v1 前向 shuffle 的单方视图可在“dealer 理想独立均匀 tape”下，给定固定 `Q_b` 和 L 条件模拟：本地 `(sigma_b,tau_b,a_b,e_b,r_b)` 边缘为独立均匀；peer 首帧由 `a_peer` 一次性均匀掩码，第二帧/聚合 `Y` 由 `r_peer` 均匀掩码。该仅限 shuffle 子协议的结论不包括此前 score-conversion key、mask 和 phase-4/5 transcript；实现级 `getrandom`/FSS RNG 距离未量化。
6. 完整目标 `Sim_b(x_b,Q_b,L)` 仍 `OPEN/NO_GO`。首个断点在 carry/sign pre-output view 条件化到固定 `Q_b`：S22 的 sampler 生成 `(V_b,Q_b)`，未证明它能对给定 `Q_b` 采样 `V_b | x_b,Q_b,L`；单 key/key-only replacement 不能自动保留这个条件分布。未发现攻击，也没有找到可复现区分器；这是缺失的 conditional-sampler 引理/API law，不是攻破结论。
7. 未通过上述完整 `F_scoreconv` 条件模拟门，不外推到 N=128 `M6B5/v3 + M2CM/v1 + M6P2/v1`、N=256 `M6S2/v2` 的自适应整池、inverse/membership、最终 output share 或 crash/abort 联合安全。两种 schema 未在 S23 重算或测试。
8. 仅提出设计备选 `F_scoreconv/v2`：在优先级键份额上增加新鲜一次性零和 refresh `rho_0←U(R_c), rho_1=−rho_0`，输出 `Q'_b=Q_b+rho_b`。它会使单方 share 在理想均匀模型下均匀，并与 refresh 前 transcript 独立；需新增版本化材料槽/包格式，未实现，不能作为当前代码结论。
9. 不改 runtime、不创建 `secure` alias、不扩大 N，不运行正式 LAN/WAN。M6A V3 保持 OPEN、V4 未获准、正式矩阵 NOT_RUN。

## 门禁

详见 `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S23_GATES_2026-10-16.json`。最小后续任务：先定义 `F_scoreconv/v1` 的精确输出 share law 与输入/输出接口，再为 P0、P1分别给出 `V_scoreconv | x_b,Q_b,L_score` 的可执行/可证明 sampler；由未参与 S19–S23 的密码学审查者复核。该门通过后再继续整池与 shuffle/output/crash 组合。
