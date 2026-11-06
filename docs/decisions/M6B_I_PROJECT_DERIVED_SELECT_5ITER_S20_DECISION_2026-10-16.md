# M6B-I-PROJECT-SELECT-5ITER-S20 决策记录

日期：2026-10-16
路线：Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL

## 决策

1. 复核对象固定为 S18 接收的 S15 默认关闭修复源码快照：72 项，S15 manifest SHA-256 为 4167e24096edc977464c89e407f15bb73ff9f9a9439baedbe31578e015da1816，canonical source SHA-256 为 c4a7df6c9ea67eb258ea9b44c17c9c89cbc0b1e22e82d3471b2a2feb8da17498。S20 对 72 项重新计算 SHA-256，72/72 匹配；没有把当前 main 源码替代被测对象。
2. 本报告作者撰写了 S19，故作者自审不是独立审查。随后 `/root/s18_independent_receipt` 完成非作者只读密码学复核；该审查者此前仅参与 S18 功能接收，未参与 S19/S20 证明工作。审查过程已完成，但密码学结论按范围分别裁决。
3. 非作者审查确认：固定 `(α, β, party)` 后，若目标 roots 条件于 α 与完整 pre-Eval 辅助状态独立，则单次 key-only 逐点界可对辅助状态取期望；同一 key 的两次 Eval 是确定性后处理；由 correctness 决定的 opened bit 可按 `c-u_b` 生成。该单次调用结论是 `CONDITIONAL`，依赖 AES-128 PRP 与理想联合随机 tape，不扩展到 key-dependent continuation。
4. 非作者复核 key-only 池算术：N=128 为 41,024 slots、Σw=1,639,424；N=256 为 163,968 slots、Σw=6,719,104。对应 `Σw·εG + εRNG_joint` 仅为条件性 key-only projection 界，不是自适应整池 transcript 结论；`εRNG_joint` 与现实信道项仍未量化。
5. 非作者指出首个全池证明断点位于 carry→sign continuation：carry DCF 的本地份额不打开，`lift = x - ((carry & 3) << 32)` 进入后续 sign operand frame。S19 关于已打开比较位与地址的后处理论证不能覆盖此依赖。缺少替换 carry key 后继续生成本方 carry shares、sign frames 和相关 masks 的条件分布 sampler/引理。该结论是证明缺口，不是已发现攻击。
6. N=128、N=256 自适应 full-pool view 均 `OPEN/NO_GO`。M2CM 的 `σ_b`、`τ_b` 边缘均匀不能推出共同 `Π/Π⁻¹` 下 Y/Q frames、DCF carrier、output share、lease 与 crash prefix 的联合条件分布；shuffle/inverse/output/crash 仍 `OPEN/NO_GO`。未找到可复现区分器。
7. 运行时无可复现缺陷，本轮不改 runtime、不重跑 S18 功能矩阵、不构建/运行测试、不运行正式性能矩阵。非作者复核是只读分析，同样未运行构建或测试。
8. 继续保持 EXPERIMENTAL；不建 secure alias，不扩大 N>256。M6A V3 仍 OPEN、V4 未获准，正式 LAN/WAN 仍 NOT_RUN。

## 精确后续动作

下一步由证明负责人给出完整的 continuation leakage sampler 与游戏；之后再让非作者审查者复核新证明。当前最小证明义务为：

- 对每个腐化方形式化 carry DCF 的本地输出 share、`lift = x - ((carry & 3) << 32)` 与 sign operand frame 的联合变量；给出只使用本方 input share、理想 output share 和批准泄露 `L` 的 sampler。
- 证明替换 carry key 后，后续 sign frame、mask shares、α/z 与 wire-level masked operand shares 保持正确条件分布；此处不能仅依赖已打开比较位。
- 将 carry→sign sampler 按 N=128 `M6B5/v3 + M2CM/v1 + M6P2/v1` 与 N=256 `M6S2/v2` 的固定 slot 顺序扩展到已用、未用、拒绝和 crash/restart 槽，重算逐槽 hybrid。
- 在上述 pool hybrid 闭合后，再将共同 `Π/Π⁻¹`、Y/Q、membership、inverse route、输出 share、lease 与 crash-prefix 视图逐方组合并证明条件分布。
- 量化或继续显式保留 `epsilon_RNG_joint` 与真实信道优势项；未量化前不报告具体安全位数。
- 对新写出的 simulator/game 再由非作者审查；崩溃合同仍只主张指定测试范围的 no-complete-mask，允许孤立 XOR share，不要求原子发布。
独立审查逐项裁决见 [S20 报告第 6.1 节](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S20_REPORT_2026-10-16.md)、[门禁 JSON](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S20_GATES_2026-10-16.json) 和 [source/evidence manifest](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S20_SOURCE_EVIDENCE_MANIFEST_2026-10-16.json)。平台负责人仍需实例化或明确保留 `epsilon_RNG_joint`；在未量化前不得报告具体安全位数。
