# M6B-I PROJECT_DERIVED_SELECT_5ITER S57 决策

日期：2026-10-25
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 裁决

- **固定来源：`PASS_WITH_SCOPE`。**本轮在隔离 worktree 审查 S56 commit `66a3db485ffa9ac752a49fd0f8a0f100fcef4cbe`，tree `8cd2c7e77c9577091bbba9405a44640e6bc446bd`，parent `0c4a35fb7bdf5d234e98eb1616cb65aae03ab3fd`。初始状态 clean。S56 manifest 的固定源码 Git blob 与 checkout 逐项相符；四项 S56 交付文件的 Git blob 及规范化交付摘要相符；S55 的 15 项运行时来源身份也继承并复核。S56 文本文件在 Windows checkout 的 raw hash 有 CRLF 差异时，以固定 Git blob 判断来源，不改写历史记录。
- **无成功筛选的完整单方视图模拟：`OPEN`。**现有 `F_T/v3` 仅明确理想 bounded sampler 的 cap-exhaustion `ISSUANCE_ABORT`；S50 `Kpre₃` 以 `cap_success` 为前提，S51 `Kpost₃` 以成功执行/SuccessFence 为前提。二者未给出包含所有发材错误、A 状态和客户端恢复结果的无条件联合状态核。
- **首个缺口按阶段区分。**在所有公开参数、路径与签名 key 均有效的调用上，签名 key 文件读取/解析失败仍可能先于 `before_material()`，而 `F_T/v3` 将签名能力理想化且没有该错误输出；这是完整发行视图最早的未建模运行时失败。若 key 已成功读取，`before_material()` 的 FTv3 marker open/stat/create/write/fsync/close/dir-fsync 是首个有持久 burn 状态的失败边界，当前理想功能也没有相应的结果/持久性 law。前者没有发行，后者可能留下 marker 或部分状态。缺口是模型/接口未定义，**不是攻击或不可能性结论**。
- **托管状态源码映射：`PASS_WITH_SCOPE`。**PENDING、COMMITTED、ABORTED 是 A 内部数据库态；`COMMIT_STATUS_UNKNOWN` 是客户端对结果不确定的 API 投影，不是 A 的 DB 状态。只有经验证的 committed fetch/decrypt/share 对照完成后才返回本方 share；该事实不表示失败路径中本地临时 share 从未形成。
- **最小理想接口提案：`CONDITIONAL / NOT_IMPLEMENTED`。**后续可定义组合版 `F_TOPK-ISSUE-ESCROW/v1`，对同一调用元组显式路由发材 outcome、在线输出启用、A 内状态及各方可见响应；T/A 私有错误状态不直接交给 party simulator。该提案不扩大 `L_S29`、不变更角色和成功输出合同，也不提供目标部署失败概率。
- **真实状态概率与部署桥：`OPEN`。**S56 源码可确定调用次序，但目标 T/A provider、RAND 错误、签名/序列化、文件系统、SQLite commit/rollback、TLS、调度和超时联合 law 均无目标部署证据。S52 功能通过次数不是任何成功事件的 `p_min`；本轮不做事后条件化。
- **继承边界：`PASS_WITH_SCOPE`（仅继承）。**S53 外生 SuccessFence 下成功应用消息视图仍限理想 IID `F_T/v3`、cap-success、理想 A token RNG、诚实 peer 因果提交及 fence 保证及时完成。真实有限 timeout/wall-clock 成功筛选仍 `OPEN`。
- **其他门：继续 `OPEN`。**真实 T/A DRBG、签名与文件发布、TLS/mTLS、recipient encryption、完整 GMW/Select/shuffle/output/escrow 全视图、N>256 安全扩域各自独立开放。`LOW_PROBABILITY_ISSUANCE_ABORT_ACCEPTANCE=OPEN_CONFIRMATION`。
- **运行时与验证：未修改，`BUILD/TEST=NOT_RUN`。**未发现本轮可复现的 runtime 缺陷；未重跑功能案例、历史矩阵或自然 I/O 场景。正式 LAN/WAN、RTT、AAV86 与性能矩阵 `NOT_RUN/NOT_MEASURED`。`VFSS-baseline/` 未改；未创建 `secure` alias；不推送、不合并。

## 独立复核

非作者 `/root/s57_independent_review` 未参与 S50–S57 的实现或证明推导。本轮只读重建 `F_T/v3` 的适用条件、发行状态先后、A 的状态机、party-visible 错误映射及现有理想功能缺口，并确认 marker I/O 是首个持久发行状态边界。复核者未编辑、构建或测试；该审查不是功能接收，也不是整体密码学安全通过。执行者与审查者意见均不替代目标部署资料。

## 下一步

提出 proof-only 的 `F_TOPK-ISSUE-ESCROW/v1` 接口草案，先由协议负责人确认错误结果的角色可见范围和同 invocation 启动规则，再由非作者复核完整无条件 joint kernel。目标部署 DRBG/错误、签名、文件发布、A token RNG、数据库、真实 timeout 和 transport 的环境证据仍需分别取得；不得以理想接口提案关闭这些门。
