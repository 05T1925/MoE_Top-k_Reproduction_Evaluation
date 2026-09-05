# M2.3 Protocol I reverse secret-shared shuffle / mask-carrier 规格

状态：**候选设计契约，待独立审查与用户批准；`m2_protocol_i_design_blocked`。**本文不创建代码或主张论文已有 VFSS reverse 实现。

证据：**A** 论文明确内容；**B** 本地参考/VFSS 行为；**C** 项目扩展；**D** 缺失证据。

## 1. 边界、记号与 Permute+Share 理想接口

令 `PS(owner, rho, data, v)` 为 Chase 的 Permute+Share 理想功能（A，Chase §6.2，pp.19--21）：permutation owner 私有输入置换 `rho`，data owner 私有输入向量 `v`；输出随机 additive shares `(q_owner,q_data)`，满足 `q_owner+q_data=T_rho(v)`。只有各自输出 share 到达相应方；不打开 `rho`、`v`、重构结果或 mask。安全论证是静态半诚实模型下的底层 OPV/Share Translation 构造，且每个调用的相关材料须新鲜。

为消除数组记法歧义，本规格用抽象值变换 `T_rho`，并要求未来 API 选择一种可测试约定。B1 的数组约定是 `apply(rho,x)[i]=x[rho[i]]`，其 `compose_apply(outer,inner)[i]=inner[outer[i]]`；它与“旧位置映射到新位置”的函数复合方向相反。未来 conformance 必须验证 `T_inverse(rho)(T_rho(x))=x`，不得只比较置换数组名称。

论文给出正向两次 sequential PS（A，§6.3，pp.21--23）。B1 提供同类 `run_*_pass`、OPV/Share Translation 和 `inverse_permutation`（B），但依赖旧 ABI、CryptoTools、helper、私有 artifact、路径/端口；这些均禁止进入 VFSS 候选接口。

## 2. 正向代数（A 的功能，C 的记录绑定）

输入为 `x=x0+x1`。P0 私有 `rho0`，P1 私有 `rho1`，抽象合成记为 `Pi=T_rho1 o T_rho0`。

1. `PS(P0,rho0,P1,x1)` 输出 P0 的 `a0`、P1 的 `a1`，且 `a0+a1=T_rho0(x1)`；P0 局部置 `b0=T_rho0(x0)+a0`。
2. `PS(P1,rho1,P0,b0)` 输出 P1 的 `c1`、P0 的 `c0`，且 `c0+c1=T_rho1(b0)`；P1 局部置 `d1=T_rho1(a1)+c1`。

输出 `(c0,d1)` 重构为 `Pi(x)`。两次 PS 的 OT/translation/random masks 均是一次性离线材料；任一方仅知自己的 `rho`，故不知道合成 mapping（A 的正向构造；将其绑定到项目 record 是 C）。

## 3. carrier 生命周期（C）

R1 仅正向 shuffle score、秘密 original-index binding 和所需 record 数据；**carrier 不存在于 R1**。R2 产生 shuffled `rank_P` shares，R3 仅按获批 D1 公开 `(shuffled_slot,rank_P)`。双方以固定公开 convention 创建 carrier arithmetic shares：例如 P0 持 `1{rank_P<K}`，P1 持 0。公开 rank 已使 shuffled slot 的选择可推导；该 convention 不额外关联原始位置。R4 只逆路由 carrier，输出原顺序 arithmetic bit shares；双方局部取 LSB 得 XOR mask shares，无消息且不增 round。

## 4. reverse 两次 PS 候选（C；代数正确，未实现）

设 R4 输入 `z0+z1=Pi(x)`，目标 `Pi^-1=T_rho0^-1 o T_rho1^-1`。使用**新鲜**两份 PS 离线材料：

1. P1 是 owner，调用 `PS(P1,rho1^-1,P0,z0)`，得 P1 `e1`、P0 `e0`，`e0+e1=T_rho1^-1(z0)`；P1 局部置 `f1=T_rho1^-1(z1)+e1`。
2. P0 是 owner，调用 `PS(P0,rho0^-1,P1,f1)`，得 P0 `g0`、P1 `g1`，`g0+g1=T_rho0^-1(f1)`；P0 局部置 `h0=T_rho0^-1(e0)+g0`。

输出 `(h0,g1)`，其和为 `Pi^-1(z)`。这证明“若 PS 对任意置换均以该 ideal interface 可用”，两次 role-swapped 调用在代数上足够。它**不**指定 VFSS 内部 OT/translation transcript；正向材料绝不可复用。缺材料、长度/角色/phase 不符、EOF 或任何 PS 失败均 hard-fail、清除未完成输出且不重试/降级。

R4 的唯一抽象在线操作是上述两个 PS invocations；其实际 P0↔P1 causal barriers、opened values、sent/received bytes 在 VFSS adapter 尚不存在前均为 `NOT_MEASURED`。新鲜 reverse 材料计入 offline material；R4 barriers 单列为 `mask_adapter_rounds`，不得计作论文 Table 1 的 3 rounds。

## 5. record-preserving 与退出门

reverse 仅作用于 R3 后创建的 carrier：由正向 `Pi` 下 slot 得到的 carrier 经 `Pi^-1` 回到同一原始 slot，不需要打开 original index。score/index 仅用于 R1/R2 的绑定与正确性证明；它们不随 R4 发送或打开。该 project-output 论证为 C；PS 单方隐藏与正向构造为 A；对 VFSS adapter 的 transcript、安全性和 E2E 为 D。

| 条件 | 状态 | 证据 | 可进入最小 conformance 代码？ |
| --- | --- | --- | --- |
| forward PS 接口与代数 | 已定义 | A/C | 仅获批后 |
| reverse 两次调用代数 | 已定义，依赖任意置换 ideal PS | C | 仅获批后 |
| reverse 安全/泄露 | 需独立组合审查 | D | 否 |
| 新鲜预处理、不复用 | 明确要求；VFSS 未实现 | A/C/D | 否 |
| 消息、打开值、错误、轮次 | 顶层调用/错误已定；内部 transcript 缺失 | C/D | 否 |
| record-binding 与原顺序 mask | 代数目标已定；缺 conformance/E2E | C/D | 否 |

结论：D2 从“无候选”推进为**可审查的 ideal-functionality / minimal-adapter 候选**，但未满足进入代码的全部退出门。D1 泄露策略仍须用户批准；D3 需 range conformance；D4 需 bounded transport、真实计量和三进程 E2E。

**D2 未经独立审查与用户批准前，M2 仍为 design_blocked；不授权开始完整 Protocol I 实现。**
