# M6B-I-PROJECT-SELECT-5ITER-S37 决策记录

日期：2026-10-21
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 决策

1. 固定基线为 S36 commit `ca5eb001e72c971a7dbaf87d789d4a7fbcba1b9a`，父级 S35 commit `77f9684e1a50794004fd31f3e5ee7f193e9b12ec`。S35 保留 worktree 中 85 项 source manifest 原始哈希 85/85 匹配，canonical SHA-256=`af693eb61183a606c64a1f1b16ee5c3ad6e4b382ec5456e541c7915b8f95faa0`。S36 相对 S35 的 runtime Git tree 无变化；S36 已追加实施计划，因此 S37 当前计划文件不是 S35 原始版本。未从 main 或其他快照替换被审源。
2. 以 `F_A-ATOMIC/v1` 补足 S36 缺失的角色路由：T context 只进 A 服务；P_b 模拟器只能获得本方 `F_T/v1` package、输入/逻辑输出 share、公开参数和 `L_S29`；peer package、A context 与 T 私钥不交给 P_b。A 内部按真实 transaction 状态机产生 receipt/fetch 响应。
3. 明确将双 recipient public key registry 纳入 `public` setup 输入，其指纹必须等于签名 session 中的 key digests。该公开输入不增加 `L_S29`。如果证明环境只有 key fingerprints、拿不到 peer public key，则 dummy peer ciphertext hybrid 不能运行，receipt gate 回到 `OPEN_WITH_EXACT_GAP`。
4. P0/P1 receipt 与 committed-fetch 子引理为 `OPEN_WITH_EXACT_GAP`。非作者 reviewer 发现当前“固定 challenge 明文的最坏 IND-CPA 优势再平均”没有为统一归约给出生成 `z_{1-b}` 与本方 `V_b^pre`、`ctx_A`、材料及 `L_S29` 相关联合样本的高效 sampler。`F_A-ATOMIC/v1` 的 receipt/hash/state/fetch 后处理本身成立，但不能替代缺失的挑战消息生成器。最小 proof-only 补充是 reduction-only `J_b-ENC-CHALLENGE/v1`；它只将 `(z_peer,0^N)` 送入 IND-CPA game，并将 challenge ciphertext 路由给 A，绝不把 peer package/share/context/T 私钥交给 `Sim_b`，不扩大 `L_S29`。在 J_b 正式定义并复核前，不标记 receipt hybrid 通过。
5. `F_TOPK-ATOMIC` 逻辑合同不变：precommit Party API 为 `⊥`；最终 COMMITTED state/receipt 使双方 share 逻辑上可恢复；物理 API return 可不同步。S37 theorem 限于无故障成功消息路径，以及给定 COMMITTED state 的成功 recovery fetch 子调用。commit-after-lost-receipt 仅模拟服务已提交后的 recovery 请求语义，不证明造成 lost receipt 的 crash/network 前缀。任何 API `⊥` 都不表示腐化进程内从未形成中间 share。
6. A 是条件受信、与 T/P0/P1 不合谋且不持 recipient decryption key。A-only 半诚实腐化是分离的加强模型，仍为 OPEN；恶意 A、合谋、物理断电/介质损坏/恶意 OS 不在 S37 theorem。
7. S37 在 receipt hybrid 的 peer plaintext `z_{1-b}` 处停止。精确未模拟条件是 `z_{1-b}` 与 `(V_b^pre,ctx_A,pkg_b,z_b,L_S29)` 的高效联合分布 sampler；`score_to_mask_v3_party.cpp:693-744` 形成本地输出/提交 share，`:370-440` 将其收件人加密。该缺口是证明接口问题，不是攻击或源码缺陷。真实 Dealer 到 ideal `F_T/v1` 的实现桥仍开放，但 S37 没有越过当前停止点组合它。
8. S37 是 proof/documentation-only：不改 runtime、未 build/test、未重跑 195 案。自然 I/O fault 沿用历史 `NOT_REPRODUCED`，本轮 `NOT_RUN`；N≤256 只有历史功能样本 `PASS_WITH_SCOPE`；N>256 `NOT_ADMITTED`；正式 LAN/WAN/RTT `NOT_RUN/NOT_MEASURED`。v3 默认 OFF、无 secure alias、未改 `VFSS-baseline/`、不推送或合并。

## 状态

| Gate | 状态 |
|---|---|
| Receipt/fetch success-path sublemma | `OPEN_WITH_EXACT_GAP` |
| F_A receipt/hash/state/fetch post-processing | `PASS_WITH_SCOPE` |
| Reduction-only joint IND-CPA sampler `J_b` | `OPEN_WITH_EXACT_GAP` |
| Overall security theorem | `OPEN_WITH_EXACT_GAP` |
| A-only strengthening model | `OPEN` |
| Arbitrary crash/abort-prefix privacy | `OUT_OF_SCOPE_FOR_S37_THEOREM` |
| Later real `F_T/v1` joint setup bridge | `OPEN_NOT_QUANTIFIED; not reached past current stop point` |
| Runtime defect reproduced | `NO` |
| Build/tests/performance | `NOT_RUN / NOT_MEASURED` |

详见 [S37 报告](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S37_REPORT_2026-10-21.md)、[机器 gates](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S37_GATES_2026-10-21.json) 和 [source/evidence manifest](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S37_SOURCE_EVIDENCE_MANIFEST_2026-10-21.json)。
