# M6B-I PROJECT_DERIVED_SELECT_5ITER S62 决策

日期：2026-11-02
固定基线：S61 commit `d843b45f29ab87d75520b28aac12c82608ec9678`
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 裁决

- **A：正常成功定理，`PASS_WITH_SCOPE`（继承并在本轮整理）**。只在 `1≤K≤N≤256`、一方静态半诚实、理想 FTv3 发材成功、理想输出刷新与 A token RNG、诚实对端因果执行、A/T 与腐化方不合谋、外生 `SuccessFence` 及 S61 固定 source-event cut 条件下，继承 S50 `Kpre₃`、S51 `Kpost₃`、S53 fence 范围并加上 S61 `Kpre-prefix`。本轮没有重证各基础引理，也没有把 cut 当作实际 abort。
- **B：包含协议级中止，`OPEN`**。当前没有负责人书面决定选取 abort-aware 定理；理想 FTv3 只定义 cap-abort 的局部 issuer 结果，尚无它到同 invocation package/context delivery、双方 start 与 party notice 的联合转移。可用 proof-only `F_TOPK-ISSUE-ESCROW/v1` 的两种通知策略作条件分析，但均非已批准合同或已实现路由。即便暂按全/无启动门，首个后续缺口在 A 等待第二份 deposit 时的 `PENDING→ABORTED`/响应送达条件核，后续 UNKNOWN、fetch 与 recovery failure 亦未闭合。
- **C：部署/可用性故障，`OPEN`，不纳入 A/B 理想定理**。signer、marker、RAND/provider、签名/文件发布、SQLite、机器故障、TLS/网络及真实有限 deadline 需分别建立部署模型；不得并入 cap-abort 概率。

未参与 S50–S62 推导的 `/root/s62_readonly_review` 已只读复核本轮 A 分支继承范围、S61 来源差异、cap-abort 的交付/启动/通知区分，以及 escrow 首个失败条件核；未编辑、构建或测试。审查者要求 `PAIRED_GENERIC_ABORT` 只有在负责人确认该观察已包含于获准 `L_S29` abort 字段时才可作为候选，不构成功能接收或部署安全签核。

## 已核验的合同状态

固定 S61 文档和门禁仍把以下事项列为 `OPEN_CONFIRMATION`，本轮没有收到或找到更新的负责人决定：

1. 合法输入下 FTv3 cap-abort 烧掉本次 invocation 后，是否接受只用新 invocation 重试；
2. 发材失败的通知与启动门规则，以及通知不可达时是否接受双方不同的 timeout/UNKNOWN 观察；
3. 隐私定理只覆盖成功初始化/发材后的正常协议，还是还纳入协议级中止。

既有“算法随机失败数值界暂缓”不是 FTv3 cap-abort 接受决定。已确认的正常密文托管、共同提交、各方取回以及人工 kill 的测试含义均不重开。

## 证明及运行边界

- 不改 `L_S29`、角色、输入、输出、默认开关或运行时代码。
- S61 `E_cut` 是外生选定的有限完整事件边界；不覆盖真实时钟筛选、partial-byte/TLS-record cut、进程任意指令点或失败响应。
- 不把 S51 成功后缀直接截断为 ABORT/UNKNOWN sampler；A 的隐藏 DB state 不交给 party simulator，`COMMIT_STATUS_UNKNOWN` 仅是客户端观察。
- N≤1000 仅有有限功能收据，N>256 安全扩域不在本轮结论内。
- BUILD/TEST=`NOT_RUN`；历史功能矩阵、正式 LAN/WAN/RTT/AAV86/性能均未运行。

完整状态链、候选接口分支、首个联合核缺口、review 与来源细节见 [S62 报告](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S62_REPORT_2026-11-02.md)、[gates](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S62_GATES_2026-11-02.json) 和 [source/evidence manifest](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S62_SOURCE_EVIDENCE_MANIFEST_2026-11-02.json)。
