# M6B-I-PROJECT-SELECT-5ITER-S38 决策记录

日期：2026-10-21
路线：Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL

## 决策

1. 以 S37 commit 5c31ebfeaf6eb072421badf31d7354431b984d35 及其记录的 S35 runtime snapshot 为基线。S35 source manifest 的 85 项 SHA-256 在仍保留的 S35 原 worktree 逐项匹配，canonical SHA-256 为 af693eb61183a606c64a1f1b16ee5c3ad6e4b382ec5456e541c7915b8f95faa0。S37 相对 S35 的 VFSS/ 差异为空。没有替换为 main 或相似工作树。
2. 分开定义 party simulator 与 IND-CPA reduction。Sim_b 只获得自己的输入/output share、本方签名 package、public setup、L_S29 及通过 A 正常可见的服务响应；不获得对端输入、package、output share、A context 或 T 私钥。
3. 在 S35 电路正确性前提下，对固定输入对 x_0,x_1，z_0 XOR z_1=M_TOPK(x_0,x_1)，因此归约器从外层输入对和 z_b 可精确算出 z_(1-b)。该点条件等式消除了 peer plaintext 的条件抽样；它不消除 S35 Fisher–Yates 置换生成器自己的 rejection sampling。
4. J_b 给出 reduction-only 的显式构造：按 gmw.cpp 的 triple/permutation-share 生成律采样、以兼容 Sign_T oracle 签 package/context、执行两方固定 S35 应用层转移，再用 (z_(1-b),0^N) 调用精确 recipient envelope 的 IND-CPA challenger，并把 challenge ciphertext 只送入 A 状态机。该构造依赖 proof-only 联合生成权限和 recipient-key 游戏匹配，不是现有 party-facing F_T/v1 API。strict-PPT 对每个 Fisher–Yates 位置最多尝试 λ 次，`ε_perm≤255·2^(-24λ)`；对照精确置换分布时密文替换界增加至多 `2ε_perm`。归约器权限不扩展到 party simulator。
5. peer ciphertext 的 SHA-256、receipt、COMMITTED response 和已提交本方 fetch 是同一 A 状态机对挑战密文的确定性后处理；该 G0→G1 替换不依赖 SHA-256 输出均匀。它只比较“真实本方前缀 + 真实对端密文”和“同一真实本方前缀 + dummy 对端密文”，没有把真实前缀替换成 H4 simulator。
6. **不将完整 receipt/fetch simulation gate 判 PASS。** S36 H4 的 party pre-cipher transcript 条件 law 未覆盖与 A-only ctx_A/package-set commitment 的 joint law；该 context 摘要进入实际 party 可见 receipt，且 context 与 package 来自同一相关 F_T setup。由 H4 的边缘 law或“SHA-256 看起来随机”不能推出所需联合式。当前状态为 OPEN_WITH_EXACT_GAP，不是攻击或代码缺陷。
7. 为继续闭合，下一版本需提供非 opaque 的具体联合采样证明，或调整版本化 A receipt 使其不向 parties 暴露未建模的 setup-digest correlation；不扩张泄露，不把 peer package/share/context交给 Sim_b。S38 不修改 runtime/API，不重新定义 F_TOPK-ATOMIC。
8. S38 的 theorem scope继承 S37：静态半诚实腐化一方，A/T 条件受信且不合谋，ideal F_T/v1，无 I/O/进程故障成功转录，加一次从既有 COMMITTED 状态开始的本方成功 fetch。任意 crash/abort-prefix privacy、A-only corruption、恶意角色和物理断电不纳入。
9. 非作者 reviewer `s38_independent_review` 已只读复核 J 的权限、bounded sampler、源码分布与首个停点：J 作为 reduction-only peer-envelope 子引理在明示假设下成立；完整 receipt/fetch hybrid 仍 OPEN。reviewer 未改文件、未 build/test；S37 review 不冒充本轮复核。
10. S38 仅改证明文档、gates、manifest 和计划。BUILD/TEST=NOT_RUN；N≤256 仍仅为 S35 历史功能证据；N>256 NOT_ADMITTED；正式 LAN/WAN 和 RTT NOT_RUN/NOT_MEASURED；无安全通过、无 secure alias、无 push/merge。

## 门禁

| Gate | 状态 |
|---|---|
| S35 精确源码身份 / S37 接收基线 | PASS |
| peer output-share challenge plaintext point law | CONDITIONAL_PASS_REVIEWED |
| reduction-only full-joint J_b sampler | CONDITIONAL_PASS_REVIEWED |
| peer recipient-envelope IND-CPA substitution | CONDITIONAL_PASS_REVIEWED + 2ε_perm for exact permutation law |
| H4 party view 与 A context/receipt 的 joint law | OPEN_WITH_EXACT_GAP |
| party receipt/committed-fetch success-path simulation | OPEN_WITH_EXACT_GAP |
| real Dealer/DRBG/signature 到 ideal F_T/v1 bridge | NOT_REACHED / OPEN |
| S38 non-author review of J/stop point | CONDITIONAL_PASS_REVIEWED (J only) |
| code defect reproduced | NO |
| runtime changes | NONE |
| build / tests | NOT_RUN |
| formal LAN/WAN / RTT | NOT_RUN / NOT_MEASURED |
| crash/abort-prefix privacy | OUT_OF_SCOPE |
| secure alias | NOT_CREATED |

详见 S38 报告、gates 与 source/evidence manifest。
