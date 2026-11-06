# M6B-I-PROJECT-SELECT-5ITER-S22 决策记录

日期：2026-10-16
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL`

## 决策

1. 安全对象固定为 S18 manifest 锁定的 S15 默认关闭修复源码，72 项 canonical SHA-256 `c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498`；S18 功能接收不扩展为 S22 安全接收。
2. S15 `protocol_i_score_input.cpp` 实际为 R34；carry share 未打开，其低 2 bit 进入 `lift_b`，并进入 phase-5 sign frame；phase 4/5使用独立 fresh mask shares。R34公式及消息次序见 S22 report §3。
3. 独立审查者 `/root/independent_dcf_review` 已披露未参与 S19–S21 证明撰写、推导或决策；其对 P0/P1 的 R34 carry→sign **local pre-output view** 给出 `CONDITIONAL PASS`。在 ideal joint RNG tape 及 fixed `(alpha,beta=1,party)` key-only theorem 下，先替换 carry key，再替换 sign key；同一 key 的双 Eval 与本地计算为确定性后处理。候选损失 `68*epsilon_G + epsilon_RNG_joint`，`epsilon_G ≤ Adv_AES128^PRP(T′,q=5)+10/2^128`；`T′` 与 RNG 联合优势均未量化。
4. 该独立签核不包括给定 ideal final output share 的模拟。源码把 `q_b` 变成 `keys[i]`，再于 `protocol_i_project_derived_select5.cpp:1901–1906` 放入 `key_records[i].word0` 并送进 forward shuffle。未定义 `F_scoreconv` 输出 share 约定，也没有 `Sim_b(x_b,Q_b,L)` 生成 `q_b/key_records` 和 shuffle 本地视图的条件分布；所以 P0/P1 的 output-conditioned continuation 为 `OPEN/NO_GO`。S22没有发现攻击；phase-5相关式及 q-to-shuffle 是模拟义务，不是区分器。
5. 本轮独立性门为 `SATISFIED_WITH_SCOPE`，仅涵盖 local pre-output continuation。`/root/s18_independent_receipt` 与 `/root/s12_receipt` 不被作为本轮新审查者。
6. 沿源码识别到首个未闭合后续状态 `q_b → key_records → forward shuffle` 后停止；没有继续审查后续自适应 Select slot。两种材料池的 key-only projection只引用历史条件结果，自适应全池及 M2CM shuffle/inverse/output/crash仍OPEN。N=128、N=256分别保留各自schema。
7. 未发现可复现运行时缺陷；不改runtime，不跑测试；不创建`secure` alias，不扩大N，不跑正式LAN/WAN。V3 OPEN、V4 NOT_ALLOWED。

## 门禁

见 `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S22_GATES_2026-10-16.json`。

## 下一任务

先形式化 `F_scoreconv` 的 additively shared output约定，证明 `Sim_b(x_b,Q_b,L)` 能在固定理想 output share 条件下生成 carry/sign key、mask shares、frames、`q_b/key_records` 与 M2CM forward-shuffle 本地视图，并说明模拟器能否选择腐化方 share，或给出条件采样的支持/接受率证明。由未参与 S19–S22 的密码学审查者独立复核现有 `68*epsilon_G + epsilon_RNG_joint` 界及条件化损失。该门未关闭前不得进入后续自适应 slot、整池或 shuffle/output 组合结论。

