# M6B-I-PROJECT-SELECT-5ITER-S36 决策记录

日期：2026-10-21
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 决策

1. 被审对象固定为 S35 提交 `77f9684e1a50794004fd31f3e5ee7f193e9b12ec`，S35 85 项规范源码 hash 为 `af693eb61183a606c64a1f1b16ee5c3ad6e4b382ec5456e541c7915b8f95faa0`。本轮只写治理/证明文件，没有 runtime 变化、构建或测试。
2. **H4 有条件闭合。** 在 `F_T/v1` 理想独立 tape、party package/signing oracle、单方静态半诚实及无故障成功路径下，P0/P1 的 package-conditioned Beaver openings 可逐门模拟；`L_S29` 允许的活动 comparator XOR outcome 决定 peer local bit；给定 seed/outcomes 后自适应活动图确定。固定本方最终输出 share 不改变前缀分布。非作者 `s36_conditional_crypto_review` 完成核心条件化复核。
3. 此结果是 **条件证明草案**，不是 GMW 通用定理套用，不是安全 PASS。真实 CSPRNG、签名/摘要、TLS 和加密的优势项未量化，完整 P0/P1 view 未闭合。
4. 沿成功路径到 escrow submit，严格的 `Sim_b(x_b,z_b 或 ⊥,public,L_S29;F_T/v1)` 首个下游缺口是 receipt：A 的回执依赖 signed context/package-set 与 peer ciphertext hash；P_b 模拟接口没有把 A-side context 与诚实 escrow emulator 耦合进 `Sim_b` 的显式接口。 建议 `F_A-ATOMIC/v1` 最小化补足服务调用，不向 P_b 泄露 context，不扩张 `L_S29`，但本轮不把其当成已存在/已证明接口。
5. 任意 abort/crash-prefix 仍另有模型缺口；若覆盖它，需版本化 `F_TOPK-ATOMIC/v2` 指定 environment cut、已交付 frame/byte 前缀和 A commit/receipt 状态。否则当前证明必须限于成功路径。该缺口不是攻击或 runtime defect。
6. A-only corruption 保持为独立加强模型，`L_A` 不与 `L_S29` 合并。A 按用户确认条件受信、与 T/P0/P1 不合谋、无 share 解密钥；SQLite WAL/FULL 不作掉电证明。
7. 自然读写故障仍记录为 `NOT_REPRODUCED`（S35 历史无注入证据；S36 未新运行）；N>256 不准入；正式 LAN/WAN/RTT 未运行/未测；没有创建 secure alias，v3 默认 OFF。
8. 保持历史 worktree/提交与 `VFSS-baseline/` 不变。S36 变更限于报告、决策、机器可读 gates、source/evidence manifest 和实施计划的状态追加。

## Gate 摘要

| Gate | 状态 |
|---|---|
| S35 exact source identity | `PASS` |
| H4 activity-open conditional proof draft | `CONDITIONAL_PASS_REVIEWED` |
| P0/P1 complete view through receipt | `OPEN` |
| P0/P1 abort/crash prefix | `OPEN` |
| F_T real RNG/signature bridge | `CONDITIONAL / NOT_QUANTIFIED` |
| A-only simulator | `OPEN` |
| Natural I/O failure | `NOT_REPRODUCED` |
| N≤256 functional admission | `PASS_WITH_SCOPE` (S35 historical only) |
| N>256 | `NOT_ADMITTED` |
| formal LAN/WAN / RTT | `NOT_RUN / NOT_MEASURED` |
| secure alias / default v3 | `NOT_CREATED / OFF` |

详见 [S36 报告](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S36_REPORT_2026-10-21.md)、[机器门禁](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S36_GATES_2026-10-21.json) 和 [来源/证据 manifest](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S36_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-21.json)。
