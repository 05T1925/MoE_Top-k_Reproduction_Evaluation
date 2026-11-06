# M6B-I PROJECT_DERIVED_SELECT_5ITER S33 独立接收报告

日期：2026-10-20
被测路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 1. 结论与范围

S33 按 manifest 复建 S32 的源码输入，fresh Release default-OFF / explicit-v3-ON 重建，重跑两项 v3 CTest 和 195 个独立进程 E2E case，并审查数据转换、GMW gate/material、通信、escrow 和恢复逻辑。发现一个可复现的异常关闭处理缺陷：READY 错误后 TLS `SSL_read`/ABORT 写入路径可因 SIGPIPE 直接杀死双方进程。已在公共 framed transport 做最小修复，再运行定向故障矩阵和完整功能矩阵。

修复后 v3 定向 CTest 2/2、E2E 195/195、generic framed transport 回归 1/1、补充通信故障案例 3/3 通过。没有创建 `secure` alias，v3 默认保持关闭。**接收范围仅为实现/测试层面的功能与所列通信故障行为；密码学安全结论、掉电耐久性和正式 LAN/WAN 性能仍是独立门。**

无注入正常多进程路径本轮未出现自然 I/O 异常；没有证据可判断此前提到的读写等待现象是否曾发生于相同条件，结论仅为“本轮未复现”。定向注入是有意制造的协议故障，单独记录，不作为自然错误报告。

## 2. 证据分类与独立关系

| 证据类 | 本轮实际依据 | 结论边界 |
|---|---|---|
| 论文定义 | 本地论文/来源边界和 S31 已接收规格 | 不将这一路线说成 BB90 原五迭代算法或 BMW16。论文中不存在本项目这套完整 v3 路径的证明。 |
| 本地参考行为 | S25 manifest 的 74 个文件及冻结 `top_k_mask` oracle；S31 TEST_ONLY 规格和哈希 | 只用来固定 signed Q20.12、stable order、原序输出、路由/输出合同；不把本地实现反写成论文结论。 |
| 项目扩展 | S32 15 个增量文件、84 文件重建集、当前 v3 runtime | `PROJECT_DERIVED_SELECT_5ITER`、GMW/Beaver-v3、在线 escrow A 都是项目派生实验设计。 |
| 待验证设想 | simulator、预处理联合分布、leakage composition、密码学优势、物理耐久性和 LAN/WAN | 功能测试、SQLite flags、TLS 或 Beaver 公式本身均不关闭这些门。 |

S31 是规格/TEST_ONLY 行为审查，不是 S32 runtime 审查。S33 在开始时没有参与 S32 runtime 实现，独立核对 manifest 后才审阅代码；本轮新增一项最小 runtime 修复和 TEST_ONLY 故障驱动。S27 历史 `PASS_WITH_SCOPE` 只作历史背景，本轮未重跑。S32 原 worktree 有继承的大量未提交/未跟踪文件，本轮未在其中运行 clean/reset/覆盖/删除或提交；S33 使用从 manifest 复建的隔离 worktree。

## 3. 源码身份重建

复建顺序是 S25 记录的 74 文件 + S32 明确列出的 15 个增量文件，按 S32 manifest 中的 sha256 逐项校验，使用相同的 `path<TAB>sha256<LF>` ordinal canonicalization：

- S25 manifest SHA-256：`c96450eb62b186e866291b6c4415441e0ef3216703612314542e1d2ee25372eb`；S25 工作树 74/74 文件匹配，canonical `64f32993b12bcd8692ed089f5dcdb8ce1be9f181ff217b508b8b0470eb8e440d`。
- S32 的 15 个增量文件哈希均与其 manifest 相同。重叠文件去重后共 84 个唯一源码/证据输入；S32 原 worktree 和首次 S33 复建在 S33 修改前均为 84/84 精确匹配，canonical `fbb4b2474b5c4594d8f10a91cdbc669cd1a1efcc8865bf565fa2fbd8552dd38e`。
- 本轮相对 S32 有 4 个有意文件变化：`PROJECT.md` 与 `docs/IMPLEMENTATION_PLAN.md` 记录 S33 状态；`VFSS/src/moe_topk/protocol_i_transport.cpp` 修复 SIGPIPE 错误路径；`VFSS/tests/moe_topk/score_to_mask_v3_process_e2e_test.cpp` 增加 TEST_ONLY mTLS 故障代理、进程日志和 3 个定向案例。其余 80 个文件仍逐项匹配 S32。最终 84 文件 canonical `7e0f2ab6ae03ae402a739e2f5e97a09aa44a4fb3a373294f860ebb6f441c3df9`。
- S31/S32 的 decisions、reports、gates、case records、source/build/evidence manifests 均先从 S32 原 worktree 核对，再复制到仓库外的 S33 输入证据目录；其 SHA-256 记录在 machine-readable manifest。

## 4. 构建隔离与开关

环境：Ubuntu 24.04.4 LTS / WSL2 x86_64，GCC 13.3.0，CMake/CTest 3.28.3，OpenSSL 3.0.13，SQLite runtime 3.45.1；`libsqlite3-dev` 3.45.1-1ubuntu2.9 从已下载 deb 解包到 `/tmp`，未安装/未放入仓库。修复后的构建目录和日志都在仓库外。

| 配置 | 结果 |
|---|---|
| Fresh Release，v2 OFF / v3 OFF | `sytorch` build PASS；cache 显示 v3 `OFF`；`libsytorch.a` 无 `ScoreToMaskV3` symbols；target help 中无 v3 target；无 opt-in macro include v3 header 按预期 `#error` 拒绝。 |
| Fresh Release，v2 OFF / v3 ON | 只开启 `MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_GMW_EXPERIMENTAL`；显式构建 v3 runtime/process target，不选择 v2/DCF route，不存在 fallback。v3 conformance/process E2E PASS。 |
| v3 ON build 修复后 hash | `libsytorch.a` `53acd750a1c3d0f0ec10f4f96fe9641f86470510de1acbcb41ef89c3dc9a7500`；runtime conformance binary `6d638c75aa020ecd5840f745ef8e0cec44c27ca0f8ad79c9a1f11b5d268a3fff`；process E2E binary `1f5223b5aa0e7fd27a7ea026692cf72a344732cd2687f2caf5bc095ebc01bc16`。 |

原始 S32 runtime conformance binary 与本轮相同哈希 `6d638c…a3fff`。独立开启只验证代码 opt-in 隔离，不构成安全配置建议。

## 5. 正常功能复跑

最终测试二进制实际运行 targeted ctest，2/2 通过，总计 86.42 秒；process binary SHA-256 为 1f5223b5aa0e7fd27a7ea026692cf72a344732cd2687f2caf5bc095ebc01bc16。命令和原始日志哈希见逐案 JSON 与 source/build/evidence manifest。

195 个逐案记录全部为 oracle PASS；distribution：N=1:1，2:2，3:86，4:101，5:1，8:2，128:1，256:1。N=3 覆盖三值分数全集和所有 K；N=4 覆盖分数排列和所有 K；另含范围边界、同分、N=8、N=128/256 资源点。结果是这 195 个 TEST_ONLY 进程 case 的功能证据，不是对整个 N 域的形式证明。

此外单独运行一条不注入故障的 N=3,K=2 多进程观察路径：TEST_ONLY 本地 mTLS 终止代理仅转发、不改帧，分别保存 Dealer、A、P0、P1 的 stdout/stderr，并记录每帧 phase、sequence、tag。P0/P1 均 exit 0、API 为 COMMITTED，A 有 1 条 transaction 和 2 行 recipient shares；双方各 76 phase/frame，序号 0–75，tag 1–6 的含义见逐案 JSON。其余正常 195 case 通过 CTest 保存 parent stdout/stderr 与逐案 counters；运行时没有生产级逐消息日志。观察代理在双方发送完最后一帧并关闭 socket 后读到 TLS unexpected EOF，party 均正常 exit 0 且 A 已提交；这是测试代理收尾现象，不归类为产品路径自然 I/O fault。

### N=128/256 实际观测

| 量 | N=128,K=64 | N=256,K=128 | 说明 |
|---|---:|---:|---|
| package gate count | 1,289,280 | 5,281,664 | 成功路径 `all_gates_consumed()` 检查全部 gate marker 均为 1；同 gate id 重复消费会拒绝。 |
| P0 / P1 package bytes | 485,964 / 485,964 | 1,989,252 / 1,989,252 | 本轮文件实测，含实际 manifest/signature。 |
| 最大在线 payload frame | 159,852 B | 655,468 B | 本轮过程计数；低于 S31 1 MiB 设计上限。 |
| 两 party 中最大 peak RSS | 25,360 KiB | 72,468 KiB | `wait4` 测 party processes，不含完整生产部署/A 主机开销。 |
| exchange phase / 每方向 frame | 88 / 88 | 89 / 89 | 应用计数；不是 RTT 测量。 |
| P0 / P1 发送字节 | 337,363 / 337,231 B | 1,337,114 / 1,336,854 B | 本机 loopback/Unix harness 诊断计数。 |
| 端到端诊断耗时 | 484 ms | 832 ms | 非正式性能数据。 |
| output oracle / recovery | PASS / PASS | PASS / PASS | 每项异或重构为冻结 oracle 的原序 K-bit mask；A restart 后各自恢复 share。 |

S31 是预算/上界：`N≤256`、`G≤5,281,664`、每方 package ≤2 MiB、manifest/signature 等元数据 ≤64 KiB、单 gate-batch frame ≤1 MiB；消息 DAG 给 N=128/256 分别 90/91 RTT 上界。实测 bytes/frame/RSS/phase 与设计预算分开记录，phase 88/89 不能直接解释为测得 RTT。S31 还给 N=256 packed payload 1,988,816 B + 65,536 B 元数据的最大界；不是本轮包长，也不是实测网络结果。正式 LAN/WAN `NOT_RUN`。

## 6. 源码审查发现

- **算术 share → Boolean share / 模 (2^{32}) 加法：**逐项从 uint32 additive shares 的 bit shares 开始；以 Beaver AND 层生成 bitwise generate/carry，再用 XOR propagate/carry 求 32-bit modulo sum。padding 输入固定为 S31 sentinel share。运行时路径没有先对 rank、比较位、selected index 或明文校验值重构。
- **priority-key 与 comparator：**输入分数按 signed 32-bit/Q20.12 解释；符号位翻转把 signed 顺序映射到 unsigned，随后按该实现的 priority key 对 score 降序和原始 index 升序编码。全体 pair comparator 以逐位 borrow recurrence 工作。N=3/N=4 exhaustive oracle cases、边界分数和 tie cases 支持已测范围；不因此宣称论文算法等价。
- **shuffle / 输出：**T 生成 permutation 的 XOR-shared one-hot matrix；party secret-shared keys 经过前向路由、比较/Select5 active public outcomes、membership threshold 和逆路由，最终恢复到原输入顺序的 mask shares。没有发现 output index 明文返回 API；成功重构仅在 TEST_ONLY oracle/controller。
- **Beaver materials：**每个 package 的 gate schedule 与电路 hash、session、invocation、party、identity、payload 长度/摘要和签名验证绑定。`and_layer` 在交换前标记 gate id 已消费，重复/越界 id 拒绝；成功末尾要求所有 gate marker 消耗完。每方在 READY 前持久 burn one-shot lease；通信失败不会释放或复用 package。异常故障用掉整份 package lease，即使实际只执行了 gate 前缀。
- **frames / waits：**外层 frame 绑定 n/k、session/fingerprint、材料 invocation id、phase、sequence、type、sender/receiver、length；TLS 端点要求 TLS 1.3 mutual authentication 和 name/certificate binding；内层 v3 context 104-byte prefix 再绑定 phase、party、tag、session/invocation、circuit 和 gate-schedule hash。读写按 chunk/partial completion 循环、poll/TLS WANT_READ/WANT_WRITE 使用 deadline，header 长度/phase/sequence 不符或 EOF/截断转异常。正常 195 案例的生产路径只输出 aggregate phase/frame counters；补充的无注入 TEST_ONLY mTLS observer 对一条成功路径保存逐消息 phase/seq/tag 和四个子进程的独立 stdout/stderr，故障代理另保存三条注入路径 trace。generic transport conformance 1/1 复跑通过，覆盖 header/sequence、截断、短 I/O 和超时拒绝；专门 v2 TLS13 transport target 在本次 v2-OFF ON-only CMake 配置中未注册，未声称单独运行通过。
- **本轮新发现：**TLS abrupt close 的 `SSL_read` 错误处理可能发 TLS fatal alert；若触发 `SIGPIPE`，默认 signal action 会直接终止 Party 进程，绕开 `score_to_mask_v3_execute_party` 的 `ABORTED/no-share` 返回合同。S33 将 SIGPIPE block/drain/restore 仅作用于当前线程和 socket I/O 操作；不是全进程 `SIG_IGN`，没有全局改变 signal policy。底层 TLS I/O、上层异常清理、正常流程分别留有独立证据。
- **escrow A：**服务端 `BEGIN IMMEDIATE` 后存 transaction metadata 与收件人密文；第二份有效密文到齐时更新 committed state/receipt 并在同一 SQLite transaction `COMMIT`。同交易相同密文可幂等重试，冲突密文/错交易拒绝；按认证 party 只 fetch 对应 recipient ciphertext，A 侧源码无 recipient decrypt private key。pending 单密文在重启或 timeout 时被标 abort。当前 WAL + `synchronous=FULL` 和本轮 process restart case 支持应用级 transaction/crash 行为；没有验证掉电、磁盘/文件系统故障、SQLite/硬件实际 durability。
- **提交与返回时序：**逻辑提交点是 A SQLite commit；service receipt 是 commit 后的响应；P0/P1 各自收到并验证 receipt 后才从 Party API 得到 share。两台客户端收到 receipt/return 的实际时刻可不同，不能写作“物理同时输出”。丢回执会是 `RECOVERY_REQUIRED`，不是 ABORT；重启后按身份 fetch 各自密文。A 观察边界按 S32 `L_A` 记录，具体包括 session/invocation、N/K、party 身份、密文长度/hash、transaction 状态及提交/访问顺序和时序。

## 7. 通信故障矩阵

三项均用 fresh session、fresh package/lease/A DB 和显式 TLS-terminating TEST_ONLY proxy。API 的 no-share 只表示未返回本方 mask share；不能推导 party 内存里从未存在 input share、wire 或部分中间值。

| 案例/注入点 | 修复后双方 API / 进程 | 材料 | A 持久状态与恢复 | 注释 |
|---|---|---|---|---|
| READY body：phase 1，P1 发 `{1}` 后 proxy 改成 `{0}` 送 P0 | P0 exit 10、P1 exit 10；双方 `ABORTED/no-share` | 两份 package lease 均 burned；此时尚未调用 Beaver AND layer，gate triple 使用数 0 | transactions=0、shares=0；A 无双方密文，双方都不能从 A fetch/恢复本次 share | 修复前双方均 SIGPIPE exit 141；说明异常处理路径曾被绕过。 |
| 重复旧 READY：proxy 已向 P1 发 outer sequence 0 后，再发相同 inner phase-1 READY，outer sequence 1 | P0/P1 exit 10；双方 `ABORTED/no-share` | 两份 lease burned。双方开始第一 Beaver layer 前缀；gate marker 在 exchange 前设置。N=3（padded N=4）首批 128 个 gate id/party 被标记；不复用整包。 | transactions=0、shares=0；A 无本次份额可恢复 | P1 在期望 phase 2 时看到 outer phase 1，transport header 校验拒绝。 |
| 第一 Beaver response：phase 2 P1 response 到达 proxy 后、转发 P0 前对 P1 `SIGKILL` | P0 exit 10 `ABORTED/no-share`；P1 exit 137，进程死亡、没有 API result file | 两份 lease burned，至少首批 128 gate marker 已消费。P1 可能在被杀前进入下一 gate layer；异常退出无 gate marker 总计数，因此额外前缀计数 `NOT_MEASURED`。 | transactions=0、shares=0；A 没有 ciphertext/commit，不能从 A 恢复 P0/P1 的本次输出 share | 不声称 P1 从未持有本地中间值；只确认没有完整 `F_TOPK-U` API share 返回。 |

最终三项故障复跑的所有进程 stdout/stderr、proxy frame trace 和每案例 case-result.json 均保存在 C:\Users\28641\.codex\evidence\M6B_I_S33_20261020_01\communication-faults-final-audited\；早期修复确认日志也保留在 communication-faults-fixed 子目录。原无注入 CTest 成功路径覆盖 A normal commit、restart/fetch、提交前单密文 A 崩溃/超时，以及提交后 P0 lost receipt、A restart/fetch。修复前失败与修复尝试失败也保留在 evidence 子目录/日志，详见 manifest。

## 8. 安全、输出和性能 gates

- P0/P1 完整 output-conditioned party-view simulators：`OPEN`。首个精确证明断点不是功能 oracle，而是需条件化在本方输入、理想本方输出、公开参数和批准 leakage 后，模拟该方实际 transcript 中所有 Beaver `d/e` openings、先前 wire shares、Select5 允许的自适应活动比较结果、shuffle 路由、输出刷新、A deposit/receipt/crash prefix。现有材料只定义了目标和协议代码，没有给出这些变量在该条件下的联合分布 sampler/hybrid。
- Dealer `T` 的 triple 与 permutation-share 联合预处理分布：`OPEN`。party-local 频数/抽样代码不证明两份全包相对于 T 的联合条件分布；需精确说明 T 信任/非合谋假设、跨所有 gate 的 `c=a∧b` 关联和 one-hot permutation shares，与 online view/output/leakage 的条件关系，并给出模拟/证明。
- A 的 view/提交/崩溃/恢复 simulator：`OPEN`。现有实现没有给出对 A 可见密文、交易阶段、服务回执、fetch、崩溃边界和顺序/时序的 joint sampler。`L_A` 是观察范围清单，不是模拟证明。
- leakage composition：`OPEN`。还需将每轮唯一获准的活动比较 output 与最终稳定 mask 的条件分布组合起来，证明没有额外 disclosure（含 transcript/abort prefix）；不能从 195 个测试推出组合安全。
- OpenSSL CSPRNG/DRBG、Ed25519 signature、RSA-OAEP/AES-GCM recipient encryption、TLS 1.3 mTLS advantage：`NOT_MEASURED/NOT_QUANTIFIED`。没有运行时 query bound/失败概率或具体安全参数优势估算。
- WAL/FULL：应用级同事务/process restart 有功能证据；突然断电、介质损坏、硬件缓存/文件系统耐久性 `NOT_RUN`。不得用 SQLite pragma 宣称 power-loss proof。
- 正式 LAN/WAN 性能与 RTT：`NOT_RUN`。本报告所有 `diagnostic_elapsed_ms` 和 `exchange phase` 均非正式性能结果，不能作为 RTT 或跨机时延。

## 9. 失败、超时与重复记录

修复前 dedicated failure binary `57668ef37ecbc6d4bd612ce53b8d6e9b004ee2a9e081e07647257b9490170a58`，链接 S32 原 `libsytorch.a` hash `c207203f6c5e6325024ae525e50e7d88bf4111f3794473d5a9938fe58406ceae`，READY fault 复现 P0=P1 exit 141。最终修复 binary `1f5223b5aa0e7fd27a7ea026692cf72a344732cd2687f2caf5bc095ebc01bc16`，修复后 `libsytorch.a` hash `53acd750a1c3d0f0ec10f4f96fe9641f86470510de1acbcb41ef89c3dc9a7500`，三故障通过。

此外，证据目录保留并在 manifest 标注：最终主矩阵 2/2 CTest 和三项 fault suite 均用最终 TEST_ONLY trace 二进制复跑；无注入单案的四个 worker 日志与逐帧 trace 也已保存。初次 OFF probe 的 shell quoting 失误后修正；初次 fault proxy 配错接收 party config，未达到 injection；只保护 SSL write 时 P1 在 abrupt TLS read/alert 仍以 141 退出，随后把保护覆盖到 SSL read；root-owned 旧 build 目录写入 permission denied，改用新的独立 build directory；ON-only 配置未注册 v2 专属 TLS13 test target。最终功能 CTest 无失败、无超时、无重试，只有第 5 节列明的 195 个 PASS case。

## 10. 修改文件

S33 的 9 个仓库内修改文件如下：

1. `VFSS/src/moe_topk/protocol_i_transport.cpp` — runtime SIGPIPE 异常路径修复。
2. `VFSS/tests/moe_topk/score_to_mask_v3_process_e2e_test.cpp` — TEST_ONLY 无注入 observer trace、角色日志和通信故障案例。
3. `PROJECT.md` — S33 项目状态记录。
4. `docs/IMPLEMENTATION_PLAN.md` — S33 里程碑结果与未关闭门。
5. `docs/decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S33_DECISION_2026-10-20.md` — 接收决策。
6. `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S33_REPORT_2026-10-20.md` — 本独立接收报告。
7. `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S33_E2E_CASES_2026-10-20.json` — 195 案逐案结果、正常 observer trace 和 3 项故障结果。
8. `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S33_GATES_2026-10-20.json` — 机器可读 gates。
9. `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S33_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-20.json` — 84 文件源身份、构建产物和 189 项仓外证据哈希。

未修改 `VFSS-baseline/`、论文、密钥/certificate/package、参考实现；构建和 raw logs 均留在仓库外。未提交 commit/PR。

## 11. 最终状态

功能和列明故障行为 `PASS`；manifest/source identity `PASS`；default-OFF/explicit-ON 边界 `PASS`；自然读写问题 `NOT_REPRODUCED`；单独 v2 TLS13 test target `NOT_RUN`；安全证明 `OPEN`；密码学优势 `NOT_QUANTIFIED`；掉电耐久性 `NOT_RUN`；正式 LAN/WAN 与 RTT `NOT_RUN`。该路线保持 `EXPERIMENTAL`、v3 默认 OFF、无 `secure` alias。
