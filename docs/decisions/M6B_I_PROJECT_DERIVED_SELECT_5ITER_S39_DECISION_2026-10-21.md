# M6B-I-PROJECT-SELECT-5ITER-S39 决策记录

日期：2026-10-21
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 决策

1. 本轮从 S38 commit `2f350b6d79fc88df381347b597cbead4398a7902` 的隔离 worktree 开始；其父级为 S37 `5c31ebfeaf6eb072421badf31d7354431b984d35`，祖先链含 S36 `ca5eb001e72c971a7dbaf87d789d4a7fbcba1b9a` 与 S35 `77f9684e1a50794004fd31f3e5ee7f193e9b12ec`。S35 的 85 项 source manifest canonical SHA-256 为 `af693eb61183a606c64a1f1b16ee5c3ad6e4b382ec5456e541c7915b8f95faa0`；清单身份与 S38 记录一致。本轮未从 main 或其他源码快照替换基线。
2. S38 的 reduction-only `J_b` 及 peer envelope G0→G1 结论作为历史前提引用，不在 S39 重报为新证明。本轮裁决：receipt/v1 的 H4→party simulator 联合步骤在既有假设下仍未闭合。v1 摘要把仅 A 持有的 `context_sha256`、`package_set_sha256` 和两份密文摘要确定性地映入 party 可见 receipt；碰撞抗性或 S38 的边缘/密文替换结论都没有给出所需条件联合分布。
3. 增加默认关闭且显式 opt-in 的 receipt/v2：A 在最终 `COMMITTED` SQL 事务中生成 32-byte 随机 token，并与提交状态一起保存。party 不验证 receipt 的密码学内容，只检查 32-byte 小写十六进制编码；TLS 服务身份和 context 身份验证仍承担服务认证。token 不冒充 MAC 或独立签名凭证。
4. v2 的理想收据条件式在理想 A 独立均匀随机源下成立：给定 H4 的任意本方前缀、输出 share、L_S29、本方 package、本方 envelope、隐藏 A context/package-set 摘要及对端 envelope，`U` 仍均匀。party simulator 使用 role-routed A 响应，不读取 context；已提交 fetch 与重复请求复用同一 `U`。这只消除 v1 receipt 这条相关性，不完成完整计算方联合证明，也不关闭理想 F_T/v1 到真实 Dealer/签名/DRBG 的实现桥。
5. 运行时将 receipt 字段由 `commit_receipt_sha256` 改为 `commit_receipt`，准确反映 v1 是摘要、v2 是不透明随机 token。v1 默认和 wire 长度不变。v2 由双重显式门启用：CMake 的 `MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_RECEIPT_V2_EXPERIMENTAL=ON` 且 `MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_GMW_EXPERIMENTAL=ON`；无 opt-in 时拒绝签发/接受 v2 context，且新鲜 CMake 配置实测拒绝 v2 ON/GMW OFF 组合。签名 escrow context 中已有的 receipt-format 字段选择该版本，未知版本拒绝。
6. 不修改 `F_TOPK-ATOMIC`、L_S29、输出、角色、活动泄露或故障范围。最终 SQL 事务写第二份密文、`COMMITTED` 与 receipt；第一份 PENDING 存储可在更早的独立事务中完成。提交状态是双方可恢复的共同逻辑点，不代表两个客户端同时收到返回。任务定理仍只针对一方静态半诚实腐化、A/T 条件受信且不合谋、ideal F_T/v1、无故障成功转录及一次从已 COMMITTED 开始的本方成功 fetch；任意 crash/abort-prefix 隐私、A-only 腐化和物理掉电耐久性不在证明范围。
7. v2 定向 conformance、v1 兼容 smoke 和独立进程状态/E2E 已通过。一次首轮 v2 conformance/E2E 因 context 解码器仍硬编码 receipt format 1 而失败，定位后将解码检查改为 `receipt_format_supported`，修复后全部相关运行通过；失败记录保留在外部日志哈希中。此为本轮发现并修复的版本接线缺陷，不是安全攻击或自然 I/O 故障。
8. 安全门维持 `OPEN_WITH_EXACT_GAP`，receipt/v2 相关条件引理为 `CONDITIONAL / PENDING_INDEPENDENT_CRYPTO_REVIEW`。未参与 S39 推导者的独立密码学复核尚未完成；不得写成 `CONDITIONAL_PASS_REVIEWED` 或完整安全 PASS。下一处范围外的证明桥为 ideal F_T/v1 的材料/签名联合生成律到真实离线 Dealer、OpenSSL DRBG 与签名实现。
9. 未重跑 S35 的 195 案矩阵，未运行 N=128/256 的 receipt/v2 E2E；N≤256 功能准入仍仅继承 S35 已接收的 v3 功能证据。N>256 `NOT_ADMITTED`。自然 I/O 故障在 S39 `NOT_RUN`（历史为 `NOT_REPRODUCED`）；正式 LAN/WAN、物理 RTT `NOT_RUN/NOT_MEASURED`。构建诊断时间、exchange phase 不作为性能或 RTT。v3 默认关闭，无 `secure` alias；不推送、不合并，不改 `VFSS-baseline/`。

## 门禁

| Gate | 状态 |
|---|---|
| S38/S37/S35 提交祖先链与精确对象 | `PASS` |
| S35 85 项 canonical identity | `PASS` |
| S38 J_b 与 G0→G1 peer-envelope 替换 | `INHERITED_CONDITIONAL_PASS_REVIEWED; NOT_REPEATED` |
| receipt/v1 H4→Sim_b 联合模拟 | `OPEN_WITH_EXACT_GAP` |
| receipt/v2 opaque-token 局部条件分布（理想 A RNG） | `CONDITIONAL; PENDING_INDEPENDENT_CRYPTO_REVIEW` |
| receipt/v2 真实 A `RAND_bytes` 桥 | `OPEN; Adv_DRBG_A(q) UNQUANTIFIED` |
| party 不验证 receipt 内容 / TLS 身份绑定 | `SOURCE_CONFIRMED; functional tests PASS` |
| receipt/status/fetch/duplicate/restart/identity 目标测试 | `PASS_WITH_SCOPE` |
| v1 compatibility | `PASS_WITH_SCOPE` |
| 首轮版本接线失败 | `REPRODUCED_AND_FIXED; LOG_RETAINED` |
| 理想 F_T/v1 → 真实 Dealer/signature/DRBG 桥 | `OPEN_NOT_REACHED` |
| full party-view theorem | `OPEN_WITH_EXACT_GAP` |
| N≤256 功能范围 | `S35_ACCEPTED_SCOPE; S39 v2 tests only N=3/4` |
| N>256 | `NOT_ADMITTED` |
| 自然 I/O 故障 | `NOT_RUN_S39; NOT_REPRODUCED_HISTORICALLY` |
| 正式 LAN/WAN / RTT | `NOT_RUN / NOT_MEASURED` |
| v3 default / secure alias | `OFF / NOT_CREATED` |

详见 [S39 复核报告](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S39_REPORT_2026-10-21.md)、[gates](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S39_GATES_2026-10-21.json) 与 [source/build/evidence manifest](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S39_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-21.json)。
