# M6B-I PROJECT_DERIVED_SELECT_5ITER S56 决策

日期：2026-10-25
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 裁决

- **固定来源：`PASS_WITH_SCOPE`。**本轮使用隔离 worktree 中精确 S55 commit `0c4a35fb7bdf5d234e98eb1616cb65aae03ab3fd`，tree `a37fe6edc5cb0f5d1cb702347bee534c6d84e200`，parent `4695d70a4bb9a7f2bbce5c6fc7e163cb3f93d978`。S55 manifest 记录的 15 个源码 raw SHA-256 与 Git blob 均匹配；S55 四份交付文件的 Git blob 与 manifest 一致，规范交付摘要重算为 `cfe208960db5a26c545f0bf04aea020e34a72d900d76e4ed03fda3754ce85b81`。三份文本交付文件在 Windows checkout 中 raw hash 因 CRLF 与 S55 LF 值不同，Git blob 相同；不据此判为来源变化。
- **receipt/v2 源码路径与状态映射：`PASS_WITH_SCOPE`。**S55 固定 escrow 源码在两个 TLS 身份/context/请求格式检查通过、两份密文行齐备且原状态为 PENDING 后，于同一 `BEGIN IMMEDIATE` 数据库事务中调用一次 `RAND_bytes(token,32)`，接着写入 `state=COMMITTED, receipt=token` 并执行 `COMMIT`。A 检查请求字段、长度和 envelope magic，不解密或验证密文内的明文 share。COMMITTED 后重复同密文 submit 与 authenticated fetch 复用数据库中同一个 token；fetch 仅返回请求方自己的密文。该结论是源码审查，不是新运行测试或掉电耐久证明。
- **理想独立 token 随机源的状态感知条件律：`PASS_WITH_SCOPE`。**令 `W` 包含 token 生成前的本方前缀、双方密文/哈希、签名 context、通过 A 身份/context/请求/envelope 格式检查的两个 deposit 及数据库前置状态。若 `U` 在两份密文行齐备后独立均匀采样，且 COMMIT/响应/fetch 的成功条件不按 `U` 的值筛选，则 `Law(U | W, S_COMMIT=1, S_FETCH=1)=Unif({0,1}^256)`。A 不解密或验证密文内 share 明文。提交响应、fetch、重复请求是同一持久化 token 的确定性状态机后处理。此局部引理不提供诚实对端 deposit 路由，也不闭合 `L_S29` 下的完整 party view。
- **从实际 OpenSSL token API 到理想随机源的条件桥：`CONDITIONAL`。**可写成状态感知的联合优势 `ε_A=Adv_DRBG_A(Q_A,total,C_A,total;E_A,P)`，必须包含请求边界/长度、返回字节、成功/错误状态、A 状态转移、对外 submit/receipt/fetch 响应及成功事件。对同一联合游戏中的成功事件 `S`，只有当 `p_I=Pr_I[S]` 有依据且 `ε_A<p_I` 时，才可得 `Pr_R[S]≥p_I-ε_A` 及 `Adv_cond≤2ε_A/(p_I-ε_A)`。单笔 token finalize 是 32 字节、1 次 `RAND_bytes` 调用；重试、并发交易及同随机上下文其他使用者会扩大总量。
- **目标 A 部署实例与聚合随机量：`OPEN / NOT_AVAILABLE`。**未找到目标服务二进制/镜像、实际 libcrypto/provider/`OSSL_LIB_CTX`、熵与 reseed 策略、进程和并发边界、A/T 是否共进程/共享 RNG、调用及错误状态总量。不得用本机 OpenSSL、S55 的 T 部署缺口或构建依赖代替 A 的部署证据。
- **实际 commit/fetch 成功条件：`OPEN`。**源代码定义了状态机，但没有目标服务的 `Pr[S_COMMIT]`、`Pr[S_FETCH]`、`Pr[S_FULL]` 下界或环境错误模型。历史功能通过不能作为 `p_min`。SQLite 回滚调用的返回值未检查；COMMIT/查询/响应失败会让调用方状态未知，不能推断一定未提交。
- **有限 timeout、完整 party view 与其他安全门：继续 `OPEN`。**S53 的外生 SuccessFence 仅覆盖理想 deadline 前成功执行，不说明真实 wall-clock 成功筛选。T 的真实 DRBG、签名/文件发布、TLS/mTLS、recipient encryption、完整 GMW/Select/shuffle/output/escrow composition、N>256 安全扩域继续分别开放。
- **产品合同：未改变。**`L_S29`、输入/输出、角色、正常托管提交/取回不变；低概率 `ISSUANCE_ABORT` 接受仍为 `OPEN_CONFIRMATION`。
- **运行时与测试：未修改，`BUILD/TEST=NOT_RUN`。**未发现可复现实现缺陷；不重跑 S35/S40/S52 历史矩阵，不运行正式 LAN/WAN、RTT、AAV86 或性能矩阵。`VFSS-baseline/` 未改；未创建 `secure` alias；不推送、不合并。

## 独立复核

非作者 `/root/s56_token_independent_review` 未参与 S39–S56 token 设计或证明推导。其只读重核固定 S55 来源、token 生成与事务次序、失败状态分类、理想 token 条件分布及成功条件化公式；确认状态感知的条件裁决与源码相符。审查者未编辑文件、未构建、未测试，也未实例化目标 A 部署配置；其结论不是功能接收或整体安全通过。

## 下一步证据要求

由 A 服务维护/SRE、构建发布、OpenSSL/密码服务及数据库运维负责人提供脱敏目标 profile：实际服务与 libcrypto/provider 身份，实际 RNG context/熵/reseed/lifecycle/共享使用者及 `Q_A,total,C_A,total` 口径，RAND 错误状态，重试并发上限，以及 SQLite/文件系统与 commit/fetch 状态证据。不得提供 token、seed/熵内容、私钥、证书私密部分或生产凭据。完整字段列于 S56 报告。
