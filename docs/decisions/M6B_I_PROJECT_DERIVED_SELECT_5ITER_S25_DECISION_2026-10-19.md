# M6B-I-PROJECT-SELECT-5ITER-S25 决策

日期：2026-10-19
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / PROJECT_DERIVED / EXPERIMENTAL`
接收对象：S24 74-file candidate，声明 canonical SHA-256 `585d4fd60f8cc04ad4b14bf02730882084cfbb2bd7045d9b2ca36c956fc51438`。
S25 测试扩展快照 canonical SHA-256：`64f32993b12bcd8692ed089f5dcdb8ce1be9f181ff217b508b8b0470eb8e440d`。

## 决定

1. S24 清单中的 74 个候选文件均在新的 S25 worktree 中重算；S24 源集合摘要可复现。相对 S24 恰有 3 个测试源文件变化，其余 71 个不变。S25 的三项差异仅加测试拒收与 E2E 覆盖，运行时代码未变。
2. 当前执行者参与了 S19–S24 的设计/证明推导，故 `INDEPENDENT_RECEIPT=NOT_SATISFIED`。本轮结果是新的 fresh Release 构建与技术复跑，不是异会话独立接收，也不是非作者密码学签核。
3. 在 DCF correctness 下，v1 完整 key share 满足 `Q0+Q1=κ` 于 `R_c`；其局部 `q_b=Q_b>>h` 可组合依赖 v1 的低位余数为 `i` 与 `0`。当前后续路径消费完整 key record，而非局部 q。
4. v2 的 `rho_0←U(R_c^N)`、`rho_1=−rho_0`、`Q′_b=Q_b+rho_b` 保持完整键重构。不得把局部 `q′_b=Q′_b>>h` 当作可组合 share；局部移位反例仍有效，且现有 Select5 消费完整 `(Q′_b,0,0)` record。
5. S24 的均匀刷新等式只在 `V_pre` 不含 `rho` 且刷新随机量独立于被条件化的 `V_pre,L_prefix` 时成立。它说明 `Q′_b` 对该**缩小前缀视图**边缘均匀；不会单独生成完整 party package、未用/已用 DCF 池、store/lease 状态、shuffle 本地状态或后续协议 transcript。模拟器若不能生成 `V_pre | x_b,L_prefix`，输出条件模拟仍然开放。
6. 对理想独立刷新随机量，完整三元组的正确条件分布为：

   ```text
   Pr[V=v, rho=r, Q′=q | x_b,L_prefix]
     = Pr[V=v | x_b,L_prefix] · 2^(−cN)
       · 1[q = Q_b(v)+r mod 2^c].
   ```

   给定理想输出 `q` 的候选 sampler 应先从已证明的基准条件分布采样 `V`，再令 `rho=q−Q_b(V) mod 2^c`。但 S24 的 `V_pre` 没有覆盖 party 持有的完整材料包及其它共享随机带；该完整 sampler 尚未给出。
7. N=128 `M6B5/v4` 与 N=256 `M6S2/v3` 分开保留。功能回归覆盖其声明配置，不构成自适应整池、inverse/membership/output/crash 的联合隐私证明。
8. 没发现可复现的运行时缺陷或攻击；只发现证明范围不足。未改运行时代码。保持 `EXPERIMENTAL`，不创建 `secure` alias。M6A V3 仍 OPEN，V4 未获准；正式 LAN/WAN 矩阵未运行且不得启动。

## 门禁摘要

- `S24_SOURCE_IDENTITY`: `PASS_WITH_SCOPE`（74 文件复核；S24 摘要重算一致）。
- `INDEPENDENT_RECEIPT`: `NOT_SATISFIED`（同一执行者参与 S19–S24）。
- `V1_SHARE_RECORD_SEMANTICS`: `PASS_WITH_SCOPE`（源码与 DCF correctness 条件下代数）。
- `V2_FULL_KEY_RECONSTRUCTION`: `PASS`（`R_c` 全环，包括回绕）。
- `V2_LOCAL_Q_RECOMPOSITION`: `OPEN/NO_GO`（已知局部移位反例；runtime 不消费）。
- `V2_PACKAGE_AND_REFRESH_BINDING`: `PASS_WITH_SCOPE`（定向单元、store、E2E；仅测到的 ABI/平台范围）。
- `V2_FUNCTIONAL_E2E`: `PASS_WITH_SCOPE`（本轮 16 个成功执行记录含 1 个 v1 对照、15 个 v2；6 个统一失败、6 个 crash-prefix；精确范围见报告）。
- `DCF_SINGLE_KEY_PRIVACY`: `CONDITIONAL/UNQUANTIFIED`（实际 ABI 到 BCG21 的归约未作非作者独立签核，AES 与联合 RNG 优势未实例化）。
- `V2_OUTPUT_CONDITIONAL_SIMULATION`: `CONDITIONAL/UNQUANTIFIED`（仅缩小前缀 law；需理想联合随机带与未完成的前缀 sampler）。
- `ADAPTIVE_POOL_AND_SHUFFLE_OUTPUT`: `OPEN/NO_GO`。
- `JOINT_PARTY_VIEW / SECURE_ALIAS_READY`: `OPEN/NO_GO`。
- `FORMAL_PERFORMANCE`: `NOT_ALLOWED/NOT_RUN`。

详见 `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S25_REPORT_2026-10-19.md`、对应 gates、case JSON 与 source/build/evidence manifest。
