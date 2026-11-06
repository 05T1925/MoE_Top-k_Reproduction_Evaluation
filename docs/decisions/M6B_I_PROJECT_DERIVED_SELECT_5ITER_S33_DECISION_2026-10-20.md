# M6B-I PROJECT_DERIVED_SELECT_5ITER S33 决策记录

日期：2026-10-20
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 决策

S33 接收并复建 S32 的精确 GMW/Beaver-v3 运行时，按本地冻结 oracle 复跑功能证据，并独立审查通信、材料消费和在线 escrow 提交/恢复。修复一个可复现的 SIGPIPE 进程终止缺陷。修复后所列功能和通信故障案例通过；另有不注入故障的观测路径留存双端日志和帧 trace。允许该实现继续以显式 opt-in 实验状态存在。

此结论不表示 BB90 五迭代算法复现、BMW16 实现、密码学安全通过、掉电耐久保证或正式性能接收。不得建立 `secure` alias；v3 默认仍为 OFF。

## 接收身份与关联

- S32 来源按其记录的 S25 74 文件清单与 15 个增量文件重建，84 个唯一文件的规范哈希为 `fbb4b2474b5c4594d8f10a91cdbc669cd1a1efcc8865bf565fa2fbd8552dd38e`；S25 清单的 74/74 文件核对通过，原 S32 worktree 的 84/84 文件也与清单匹配。
- S33 使用隔离 worktree；不清理、重置、覆盖、删除或提交 S32 原 worktree 中继承的历史文件。独立复核之前，S33 未参与 S32 runtime 实现。
- S31 是路线规格和 TEST_ONLY 参考行为审查；它不是 S32 的实现审查或密码学证明。S27 的历史 `PASS_WITH_SCOPE` 不在本轮重跑，也不扩大证据边界。
- 最终 S33 复建集有 84 个文件；相对 S32 有 4 个有意变化：`PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md` 记录本里程碑，`VFSS/src/moe_topk/protocol_i_transport.cpp` 修复 SIGPIPE 错误路径，`VFSS/tests/moe_topk/score_to_mask_v3_process_e2e_test.cpp` 增加 TEST_ONLY 无注入帧观测与故障代理/定向案例。其余 80 个文件逐项匹配 S32；最终规范哈希为 `7e0f2ab6ae03ae402a739e2f5e97a09aa44a4fb3a373294f860ebb6f441c3df9`。

## 代码变更

READY 内容错误注入使 TLS 代理关闭连接时 P0/P1 都在 best-effort ABORT 写入附近以 SIGPIPE 退出 141，而没有返回 API `ABORTED`。将 framed transport 的线程级 SIGPIPE 屏蔽、pending 状态前后检查、仅消费本次 socket/TLS 操作产生的 SIGPIPE、恢复调用线程原 signal mask 置于公共精确 I/O 路径。TLS `SSL_read` 也受保护，因为 TLS 读取在处理突然断开的 peer 时可能写 fatal alert；TLS `SSL_write` 与明文 framed `write` 同样受保护。底层写失败仍走既有错误路径，由上层返回 abort/recovery 状态。

TEST_ONLY 进程 E2E 驱动新增 mTLS 终止代理和三项定向案例：READY body `1→0`、重放 phase-1 READY（外层 sequence 增加但 phase 保留），以及在 phase-2 Beaver opening response 到达代理后、转发至 P0 前 SIGKILL P1。测试进程的 stdout/stderr 和逐帧 proxy trace 输出到仓库外证据目录。运行时仅 transport 修复；GMW/Beaver 电路、材料 ABI、escrow 算法未改。

## 接收结果

- Fresh Release v3 ON-only 构建：v3 runtime/process CTest 2/2 通过，195/195 独立进程 case 对照冻结 oracle 通过；N=128 与 N=256 的实际 package、最大 frame、party peak RSS、gate schedule、exchange phase、发送字节与输出核验记录在逐案 JSON。
- 修复后通信故障案例三项通过。READY mismatch 和重复帧时双方 API 返回 `ABORTED/no-share`；P1 退出案例中 P0 返回 `ABORTED/no-share`、P1 被信号杀死而未写 API 结果。三项故障都先烧毁双方 one-shot lease，A 的 transaction/share 记录数均为 0，A 无法恢复 share。失败方不能复用同一包，必须使用新 session/material。
- Fresh Release default-OFF 构建未发现 v3 target 或 v3 symbols；未启用 opt-in 宏时包含 v3 header 得到预期 compile-time rejection。修复后 generic M2 framed transport CTest 1/1 通过。
- 无注入成功路径本轮未观察到自然读写错误。人工注入用于检查明示故障边界，不能用来推断自然错误曾发生。

## 输出与 escrow 决策

功能目标仍是 `F_TOPK-U`：两方得到本方均匀 XOR share，异或重构为原序稳定 Top-K bit-mask；signed Q20.12 分数降序、同分按输入原索引升序、权重恰为 K。失败门控仍采用 `F_TOPK-ATOMIC`：A 在同一 SQLite transaction 中提交两份收件人密文与 receipt，durable commit 是逻辑线性化点；服务发送 receipt 和两台 Party 实际返回 API share 是后续事件，返回时刻可以不同。

A 没有 recipient share 的解密私钥，但可观察 S32 `L_A` 所列 session/invocation、N/K、party 身份、密文长度/hash、提交/abort/fetch 状态和访问顺序/时序。该在线 A 扩大了系统参与方和元数据观察边界，不能默认为原“两方在线、Dealer 离线”架构。

## 未关闭的门

P0/P1 完整 view simulator、A 的视图与提交/恢复/崩溃 simulator、T 的 triple/permutation 联合预处理分布、只公开批准比较结果时的组合证明，以及 RNG/签名/收件人加密/TLS 的优势项量化均为 `OPEN` 或 `NOT_QUANTIFIED`。SQLite WAL/FULL 和 process-restart 测试不证明掉电、文件系统或硬件耐久性。正式 LAN/WAN 性能矩阵 `NOT_RUN`；诊断时间和 exchange phase 不是 RTT/正式性能结果。

## 后续准入条件

保持实验身份和默认关闭。任何安全或性能提升必须另有独立证据：逐方 output-conditioned 全视图 simulator/hybrid、T 联合分布分析、A 元数据/故障模拟与 storage contract、量化优势项；性能则须另开正式 LAN/WAN 任务，按 revision、环境、种子、命令、重复次数和原始计数记录。
