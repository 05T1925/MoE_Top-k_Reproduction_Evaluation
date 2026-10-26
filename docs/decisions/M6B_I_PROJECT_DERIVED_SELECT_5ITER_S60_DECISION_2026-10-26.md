# M6B-I PROJECT_DERIVED_SELECT_5ITER S60 决策

日期：2026-10-26
固定审查对象：S59 本地提交 `0f67aec45d767c4bf530b12e4d548a82d68b6800`
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 裁决

- **S59 固定来源：`PASS_WITH_SCOPE`。**从准确提交建立隔离分支；父提交、tree、S59 manifest 及其交付文件哈希匹配。S59 manifest 固定的 S58 源文件/证据来源继续作为本轮被审对象；没有从主工作区替换材料。
- **成功执行定理 `T_A`：`PASS_WITH_SCOPE`（继承）。**范围是理想 FTv3 且 cap-success、理想 A token RNG、单方静态半诚实、T/A 条件受信且不合谋、诚实 peer 因果执行、`1≤K≤N≤256`、外生 SuccessFence。它不是未筛选的 A+B 完整视图定理，也不涵盖部署故障或真实有限超时。
- **现有接口下 A+B 无条件单方完整视图：`OPEN`。**S59 首个缺口 `ISSUANCE_ABORT → package/context delivery → 双方 start/notice` 仍然存在。按候选全/无启动门可以为 cap-abort 写出显式分支采样器，但那是新合同，尚未批准、实现或证明其对应真实启动器。
- **即使采纳候选启动门，任意在线中途协议中止仍 `OPEN`。**S50 `Kpre₃` 是成功运行至 output-share 形成的条件采样器。它不提供在第 j 个比较层/消息边界切断、且不先给出未来 output share 条件下的 `Kpre-prefix^{b,j}`。截断一个给定 `z_b` 的完整成功 transcript 只生成 `Law(prefix | z_b)`，不能自动等同未筛选的 `Law(prefix)`。这是首个剩余证明断点，不表示存在攻击或已证明不可模拟。
- **output-share 已形成但 API 未返回的 cut 另需 staged own-state 接口。**模拟实际 corrupted-party 内存 view，需要仅在本方 output 形成后、且仅向其 simulator 提供本方 staged `z_b`；更早 cut 不得提前提供。该状态属于被腐化 party 的局部 view，不并入 `L_S29`，也不交给 peer。现有 `F_TOPK` 未定义此阶段接口；此补充义务尚未由 reviewer 独立确认。
- **发材成功后的指定托管 cut：`CONDITIONAL`。**若新理想组合功能显式固定 event-index cut，完整 pre-escrow 已由 `Kpre₃` 生成，A/PENDING/COMMITTED/ABORTED、响应丢失、`UNKNOWN` 与恢复可由版本化状态机按因果关系逐步采样；但 S51 `Kpost₃` 只覆盖成功后缀，当前 runtime 也未证明能实现全部 abstract cut。该条件后缀不填补前述中途 GMW prefix 缺口。
- **SuccessFence 以外的真实部署故障与时间筛选：`OPEN`。**签名器/provider、文件/marker、SQLite/服务重启、TLS/网络/真实 deadline 归部署/可用性类；不被当作协议随机 abort。真实 wall-clock 成功条件分布没有部署 kernel。
- **`F_TOPK-ISSUE-ESCROW/v1`：`CONDITIONAL / NOT_APPROVED / NOT_IMPLEMENTED`。**报告给出 proof-only 状态接口骨架、完整调用绑定字段和权限限制，区分全/无启动与通知策略、A 的隐藏状态和 party 观察、share 形成/提交/fetch/API 返回。所有允许的 `E_cut` 尚未枚举成封闭集合，也未为每个 cut 给出转移核；该草案不是闭合的可执行 functionality。完整 abort-aware simulator 仍待定义/证明所有 prefix samplers。
- **负责人事项：`OPEN_CONFIRMATION`。**低概率 FTv3 cap-abort 后以新 invocation 重试；发行/启动失败时的共同失败与通知不可达规则；安全定理是否纳入协议级 abort；N≤256 是否是本阶段安全声明上限。数值算法随机失败上限的暂缓要求不视为 cap-abort 获批。
- **执行状态：**不改 runtime、测试、材料格式或 `L_S29`；`BUILD=NOT_RUN`、`TEST=NOT_RUN`；历史矩阵与正式 LAN/WAN/RTT/AAV86/性能均未运行；`PERFORMANCE=NOT_MEASURED`。自然业务 I/O 沿用 `NOT_REPRODUCED`（本轮未重测）。

## 两个合同分支

1. **只覆盖正常成功 `T_A`：**继承 S50/S51/S53 的 SuccessFence 条件应用消息模拟。cap-abort/协议中止/恢复状态留在安全定理外；这不声称它们实际部署安全，只是定理范围排除。
2. **覆盖正常执行与协议中止 `T_AB`：**须先批准并实现 `F_TOPK-ISSUE-ESCROW/v1`，明确 CAP_ABORT 的同调用通知/全无启动、抽象 cut 规则、honest-peer 因果提交、A 状态响应核、每方 recovery 输出；再对每个 cut 前缀构造 PPT sampler。当前首个剩余缺口为中途在线 prefix law。真实 C 类环境故障仍需另行模型化。

## 关键公式与首个剩余断点

理想 IID 下令每个 Fisher–Yates 位置 `i` 的单次拒绝率为 `ρ_i=(2^32 mod i)/2^32`。在相互独立的理想抽样块和重试上限 7 下，精确 cap-abort 概率为 `δ_cap=1−∏_{i=2}^{m}(1−ρ_i^7)`；对 `m≤256` 有 `δ_cap≤255·2^-154<2^-146`。这只描述 ideal sampler 失败概率，不给出 party 路由核或完整 view 距离。

未筛选目标必须比较完整 joint view：

```text
Law_real(V_b, issue, start_b, share_formed_b, A-state-observations_b,
        fetch_b, api_b | x_b, public(τ), L_S29)
≈ Law_ideal(Sim_b(x_b, public(τ), L_S29; F_joint), same outputs).
```

A+B 候选下，CAP_ABORT 若规定“无 package/context/lease、双方均不 start”，其 party 投影可由确定的 no-start 分支生成；若规定双方都收到同一错误，需另有实际 notice route；若通知可能分别丢失，必须把 per-party delivery cut 显式放进模型。成功发材后任意第 `j` 个在线 cut 仍缺：

```text
Kpre-prefix^{b,j} = Law_real(V_b^{≤j}, share_formed_b=0 |
                              x_b, P, L_S29, ISSUED, E_cut=j).
```

S50 只给 `Law(V_pre | x_b,z_b,L_S29,pkg_b,cap_success)` 至完整输出 share。没有已证明的条件分布等式可从它推出上式；不得以 `truncate(Kpre₃(x,z))` 代替。

## 独立复核与未执行项

本轮非作者 `/root/s60_independent_review` 未参与 S50–S60 推导；只读复核 S59 精确来源、S60 的合同分支、状态机、条件 law 和 prefix 停止点。其指出 cap-abort 等式须与上界分开，并要求补全候选绑定字段、明确尚未枚举 E_cut；这些已并入报告并经其再次确认。Reviewer 只接受 proof-only 讨论规格，不批准 `T_AB` 或部署范围；没有确认完整 simulator。Reviewer 也确认 staged-own-`z_b` 仅在本方 share 形成后向该腐化方 simulator 提供以复现其 local view，不属于 `L_S29` 泄露；更早 cut 不获未来值。该复核不构成完整密码学证明或功能接收。未构建、未测试、未启动故障实验。

详细条件分布、分支采样器和候选状态机见 S60 report；逐项门禁见 S60 gates/source-evidence manifest。
