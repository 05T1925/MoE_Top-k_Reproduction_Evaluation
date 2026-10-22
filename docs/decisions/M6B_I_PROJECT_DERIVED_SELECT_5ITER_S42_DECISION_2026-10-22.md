# M6B-I-PROJECT-SELECT-5ITER-S42 决策记录

日期：2026-10-22

路线：**Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL**

本轮状态：**`CONDITIONAL_PASS_REVIEWED`，仅针对受限的本方可见成功应用/API transcript projection；完整 GMW 安全仍 `OPEN`。**

## 裁决

1. 被审提交固定为 S41 `486026ba425a2d17ca15173d056db6eb309747d1`，父提交为 S40 `b04618117b11cffe0e899947726ab1c66807ffdd`，runtime 源码仍以 S39 `8028c0fece69fe0c648b1e0f1e12ad7fca723f20` 为基线。S41 的 85 项 manifest 逐项核对：S42 开工时的本机 checkout 有 83 项 raw SHA-256 相同；`PROJECT.md` 与 `docs/IMPLEMENTATION_PLAN.md` 是已知 CRLF/LF 工作树差异，之后仅为追加本轮治理记录而有意修改。Git 提交树与固定提交一致、工作树起始干净；从 S40 原始条目重算 canonical SHA-256 为 `10c508b3d0d16a06f826bbdfd0ada97b9fd4b99c2abdcd2336cd5a5605b804f6`。同时发现 S41 manifest 的 per-entry `sha256_s41_checkout` canonical 重算值与其汇总字段不同（`e0bb…` 对 `bfa8…`）；作为历史 manifest 的 metadata divergence 保留为 OPEN，不改写 S41。两项换行差异不改变固定提交对象或 VFSS 源码。
2. 本轮构造**本方可见成功路径转录**的直接联合 sampler。它组合 S36 已独立复核的 H4 本方前缀模拟、同一固定调用的 `F_T/v1` 与 `F_TOPK`、诚实对端的真实格式加密/认证提交，以及 A 的 receipt/v2 状态机。诚实端点实际触发第二次 deposit；没有给 `Sim_b` 对端材料、A context、T 私钥或 reduction-only `J_b`，也没有使用 peer-success oracle。
3. 模拟器只输出腐化方可见的应用/API 转录。`Law_real(H_b|Q_b)=Law_ideal(H_b|Q_b)` **未主张、也非该投影视图模拟所需条件**。闭合依据是成功路径下，服务后缀对 `Sim_b` 的可见投影只依赖本方已提交密文、公开 transaction、固定 COMMITTED 状态及独立 v2 token；peer ciphertext、context、package-set hash 和 PENDING 状态不返回给该 party。固定完整 `S_pre` 的条件续接 kernel 本身仍不足以推出视图相等；本轮另行给出前缀—本方密文—服务响应的投影因子分解。
4. `/root/s40_independent_receiver` 未参与 S41 推导，已只读复核 sampler，结论为范围限定的 **`CONDITIONAL_PASS_REVIEWED`**。独立审查确认本方可见 projection 的因子分解不要求完整 `Law(H_b|Q_b)` 相等，且诚实 endpoint/A 信息路由与响应字段符合代码。适用范围限于一方静态半诚实、条件受信且不合谋的 A/T、理想 `F_T/v1`、固定 route、`1≤K≤N≤256`、理想独立随机带、fresh transaction/未消费 one-shot package、双方 deposit 在 A 配置期限内成功、无 I/O/进程故障应用/API transcript，以及一次已 COMMITTED 后的本方 fetch。原始 TLS wire/timing、任意故障前缀、真实 Dealer/RNG/TLS 桥和完整 GMW 安全不在通过范围。
5. route 在 `F_TOPK` 输入中显式标记；固定 S42 route 在 F_T/context 一侧通过 v3 protocol id 与 circuit/gate-schedule hashes 绑定，在 package 一侧通过 v3 magic/package-format version 与 circuit/schedule hashes识别。runtime package/context 当前没有单独的 route 字符串，transaction id 只编码 session 与 invocation；A 的签名 context 与 transaction-row 字段再绑定 N/K/circuit/schedule/package-set，one-shot lease 的 package hash 覆盖完整 package。该隐式 route 映射仅用于本固定路线；多路线共用格式时需新版本显式编码 route，本轮不改 runtime。
6. S39/S40 的 receipt/v2 局部结论只沿用“理想独立 A RNG 下 32-byte token 条件均匀”。真实 `RAND_bytes` 桥未量化。A 的 `L_A` 与 party `L_S29` 分开；不新增任何 `L_S29` 字段。
7. 本轮只改证明/治理文档，不改 runtime，不重跑 S40 的 211 案，不构建、不测试。完整 GMW 安全、`F_T/v1` 到真实 Dealer/签名/RNG/TLS 桥、A-only 腐化、任意 crash/abort-prefix、物理掉电耐久、正式 LAN/WAN、RTT、N>256 均未通过或未启动；没有创建 `secure` alias。

## 协议和证明约定

- 功能输出保持原序、signed Q20.12 降序、同分按原始 index 稳定排序、恰 K 个 1 的两份 XOR mask shares。
- `F_TOPK-ATOMIC` 的逻辑 commit 是提交状态与 token 可恢复的共同可见点；两个 API 可在不同物理时刻返回。PENDING 是 A 内部状态；API `⊥` 不表示 party 内存中从未形成中间 share。
- 成功路径模拟不是 A-only 模拟；A 仍被排除在腐化集合之外。任意 crash-prefix 隐私不在定理范围，S40 的故障注入和自然 I/O 状态继续作为独立工程证据。
- S40 的 N≤256 `PASS_WITH_SCOPE` 功能证据继承，不代表本轮重跑或安全证明。

详见 S42 报告、gates 和 source/evidence manifest。未推送或合并。
