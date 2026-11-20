# M6B-I-PROJECT-SELECT-5ITER-S29 决策

日期：2026-10-20
路线：`PROJECT_DERIVED_SELECT_5ITER`（项目派生，不是 BB90/BMW16）

## 决策

1. 被审对象固定为 S25 manifest 所列的 74 文件工作树快照：canonical SHA-256 `64f32993b12bcd8692ed089f5dcdb8ce1be9f181ff217b508b8b0470eb8e440d`。S25 候选基于 `c3926c68fd14f270faa8b55234311071947fa080`，但尚未提交；不得只用该 base commit 代表候选。
2. 成功输出的单方 share law 尚未从项目合同/adapter 冻结。报告明确列出均匀 XOR 候选 `F_TOPK-U` 与 canonical 候选 `F_TOPK-C`；后者让 P1 直接获知 M，不得暗中当作原合同。当前以 U 作为待确认的证明目标，因此候选模拟器输入写作 `(x_b,z_b,public_params,L)`。`Q_b`、`Q′_b`、`rho_b`、carry/sign share、DCF key、Dealer tape 和材料包都不是该接口输出；它们只能由模拟器内部按真实联合分布生成。准入前必须核对实际 share law，并按需要冻结精确 ideal contract。
3. P0、P1 的 final-mask-only `END_TO_END_SIM`、`MODULAR_COMPOSITION_VALID` 和 `CURRENT_INTERFACE_SECURITY` 均为 `OPEN/NO_GO`。这是缺少输出条件联合模拟及未量化前提，不是可复现区分器或攻击结论。
4. 以 `Q′_b` 为输出的 `F_scoreconv/v2` 可以用于模块级条件分析，但不能把其输出默认为外层泄露。若组合证明将 `Q′_b` 交给腐化方，组合功能就暴露了该中间 share；要维持原有泄露，仍须证明它从 `(x_b,z_b,public_params,L)` 可模拟。
5. 不选择较弱的 crash 合同冒充既有合同。报告列出 `F_TOPK-ATOMIC` 与 `F_TOPK-PREFIX` 两种候选合同；现有证据只支持所列测试的 `NO_COMPLETE_MASK`，不足以裁定原协议的中止输出分布。
6. 首选下一步是 proof-only 尝试：分别构造 P0、P1 的 final-output-conditioned 全视图模拟器。若该证明不能闭合，才启用版本化 `score-to-mask-v3` 设计。新版本必须保持外部 Top-K 输出和原泄露 L，给出完整材料/消息 DAG 与输出条件模拟证明；单独域分离随机源、延后 rho 或改名中间接口均不构成安全闭合。
7. 本轮没有发现可复现运行时缺陷；不改 `VFSS/`，不改 S25 候选，不创建 secure alias，不运行正式 LAN/WAN 性能矩阵。

## 证据状态

| Gate | 状态 | 裁决 |
|---|---|---|
| S25 source identity | `PASS` | 本轮复算 74/74 哈希及 canonical hash 一致。 |
| S25 功能接收 | `PASS_WITH_SCOPE` | 继承 S27 对相同 manifest 对象的限定复跑；本轮未重跑。 |
| P0/P1 `END_TO_END_SIM` | `OPEN/NO_GO` | 无从 `(x_b,z_b,L)` 采样完整腐化方 view 的证明。 |
| P0/P1 `MODULAR_COMPOSITION_VALID` | `OPEN/NO_GO` | 子功能输出 `Q′_b` 会进入腐化方 view；原外层桥接仍缺。 |
| P0/P1 `CURRENT_INTERFACE_SECURITY` | `OPEN/NO_GO` | 缺少完整视图模拟；没有发现区分器。 |
| `VERSIONED_REDESIGN_REQUIRED` | `CONDITIONAL` | 若 final-output-only proof 无法闭合，须采用新版本；目前没有证明 v2 在数学上不可能安全。 |
| N 范围 | `FUNCTIONAL N≤256: PASS_WITH_SCOPE; PRIVACY: OPEN; N>256: NOT_RUN` | 仅为继承的功能范围，不是安全或性能范围。 |
| Leakage changed | `NO` | 不把任何中间 share 加入 L。 |
| Secure alias | `NOT_CREATED` | 隐私门未关闭。 |
| 正式性能 | `NOT_RUN / NOT_MEASURED` | 按用户要求后置。 |

详细规格、分布断点、C 路线版本化接口和审查记录见：
`docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S29_REPORT_2026-10-20.md`。
