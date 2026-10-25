# S52 详细报告：S51 API-v2 接收与有限超时组合裁决

日期：2026-10-25
路线：Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL
结果：功能独立接收 PASS_WITH_SCOPE；外生 SuccessFence 下理想成功视图组合 PASS_WITH_SCOPE（继承 S51 非作者复核范围）；实际有限超时条件视图 OPEN。

## 1. 接收对象与来源

被测对象是 S51 commit 1cc1e5202b3a603b55aaae985704933d70db0794，tree 6b1ce4bbcd6a962edeeaa8a5b862dfd15e786021，父提交 S50 ff77b9694a1259c3f314f145e06fcb980cda0bfb。使用新隔离 worktree，初始 git 状态 clean。没有从主工作区复制文件；没有清理、重写 S51/S50 工件。VFSS-baseline/ 无差异。

S51 source/evidence manifest 的 Git blob 与固定提交匹配：10/10 delivery blobs；其中 4 个受本次构建/审计直接涉及的 runtime/test 文件的 S51 原始 SHA-256 和 Git blob 也逐项核验。Windows checkout 对 markdown/JSON 的换行转换为 CRLF，因此部分历史文档的工作树 raw SHA 与 manifest raw SHA 不同；Git blob SHA 一致。S51 manifest 自身 raw SHA 为 f5a61f0998a17f751a51d0114dbb79b310e090a62711a6f36a2d61c0f33df97b，Git blob 为 76b11b4c322b70fe4e55d39b809df1a0e35b14fc。按 manifest 定义，从所列 deliverables 的 path-sorted path<TAB>raw_sha256<LF>重算 canonical delivery 摘要，得到 1c3433d326379e69a982511207fad2f0ae0cf858213b951ddc783ba1609daf90，与 S51 记录相符。

工作区保护记录：文档补丁工具曾将四份已有 S52 文件写入桌面主工作区；已从既有 s52-receiver worktree 恢复四份原文件并逐个核对 SHA-256 相同。主工作区其余预存改动未触碰；本轮新稿只留在本隔离 worktree。

S51 的 S29 来源说明继续保留 provenance 限定：报告 snapshot commit 502c9cc68a23b06b3347d34d60e679d98d3f60a4，source manifest 明记 candidate_committed=false；本轮未把它升级成正式批准或扩大 L_S29。S47/S48/S49/S50/S51 的继承关系以 S51 manifest 提交链和 blob 记录为准，不用当前主工作区快照替代。

## 2. 实现和输出合同

此路线是项目派生的稳定五迭代 Select。目标语义为 signed Q20.12 score 降序、同分按原输入索引升序，输出原输入顺序下恰有 K 个 1 的 Top-K XOR shares。它借鉴 BB90 研究思路，但不等同于 BB90 原算法、BMW16 或 BB90+DCF。

正常 escrow 合同保持不变：双方在各自进程中产生本方 share 并加密提交；A 仅保存密文，双方密文齐备后进入共同 COMMITTED 状态；双方之后各取回自己的密文/share。客户端 API 返回时间允许不同。API-v2 只有在收到有效提交响应、fetch 回执与提交回执一致、recipient 解密成功且取回 share 与本地计算 share 一致后才返回本方 share。超时或 callback 异常会产生恢复状态，不代表此前内存从未形成中间 share；本轮定理不覆盖任意 crash-prefix。

## 3. 构建与 default-OFF 隔离

环境：WSL2 Ubuntu 20.04、GCC 9.4.0、CMake 3.31.6、OpenSSL 1.1.1f、SQLite runtime 3.31.1。SQLite development headers 从已有外部 sibling 临时解包目录提供；没有安装系统包或将其复制到仓库。构建物和原始日志均在 worktree 外。

### OFF Release

全新 build-off-fresh：Release、BUILD_TESTING=OFF；GMW、receipt-v2、FTv2 bounded、FTv3 bounded 与 Select5 experimental 均 OFF。sytorch 静态库目标构建通过，SHA-256 ee8116b0660a8e0c907a010a8898c4717aec7c6cd53b8180ee9c2f01cc4f1669。nm 检查没有 v3/score_to_mask 或 bounded_sampler 符号，CMake target-help 没有对应实验目标；无 opt-in 宏 include v3 header 的 probe 按 header #error 被拒。

尝试构建 OFF 的全部 targets 时失败在 unrelated ext/bitpack 测试链接，报 undefined reference bitpack::mod(unsigned long,int)。这不是 v3 目标失败，但因此“全仓 OFF 全目标构建”未通过；本次只验证成功的活动库目标及 OFF 隔离，不扩展整改无关依赖。

### 显式 opt-in Release

全新 build-on-prod：Release、BUILD_TESTING=OFF，GMW/Beaver-v3、receipt-v2、FTv3 bounded 显式 ON，FTv2 bounded OFF，Select5 legacy experimental option 保持 OFF。sytorch 构建成功，SHA-256 266a764f9586bf36c2d94133078eb53f063856971d2576e9f3dc33a4448f92bd。nm 可见 score_to_mask_v3_execute_party_v2；未见 with_test_random 测试 hook。

### TEST_ONLY Release

全新 build-test：Release、BUILD_TESTING=ON，显式 GMW/receipt-v2/FTv3 bounded ON，FTv2 bounded OFF。相关四个测试目标完成 fresh build。TEST_ONLY dealer hook 符号仅出现在该测试配置的测试静态库中；production OFF/ON 静态库均未导出该 hook。最终独立进程二进制 SHA-256 为 48355b3b3cae05ba4e8d4d9160a020fd8980c57652c9bf9fda41608b9a66dc3c。

构建尝试记录：初次非 login shell 暴露系统 CMake 3.16.3 低于项目要求 3.17，改由 login shell 使用 CMake 3.31.6 后完成 fresh 配置；早先 TEST_ONLY 临时目录在 WSL 会话间消失，未用其运行测试，改在持久 build-test 重新构建并执行。另有 OFF all-target 的 bitpack 链接失败，保留为环境/无关目标限制。没有因为这些问题修改源码。

## 4. 定向功能接收

最终被测 FTv3 process E2E 二进制及运行记录见 cases JSON。核心 conformance、bounded sampler、API-v2 状态路径和 N=1000 目标样例均使用新 build-test 下生成的二进制。运行命令不包含 S35 的 195 案、S40 的 211 案或 S48 的完整案例组。

### API-v2 提交/取回/恢复

- 正常 FTv3/receipt-v2，N=128、K=64：Dealer/P0/P1 成功退出；A 在用例结束时由测试 harness 清理信号退出；API BOTH_COMMITTED；oracle PASS；两方 fetch/recovery PASS，各自只取回本方 recipient share。
- 标准 fetch callback 抛出：P0 exit 11、P1 exit 0；P0 不返回 share，API P0_FETCH_RECOVERY_REQUIRED_P1_COMMITTED；服务重启后可按本方身份恢复，oracle 重构 PASS。
- 非标准 fetch callback 抛整数：P0 exit 11、P1 exit 0；P0 不返回 share，状态 COMMITTED_FETCH_REQUIRED；保留 transaction/receipt，重启 fetch 后恢复 PASS。
- 非标准 submit callback 抛整数（发生在 A 可能已提交后）：P0 exit 11、P1 exit 0；P0 不返回 share，报告 COMMIT_STATUS_UNKNOWN；对端已提交；服务重启后按交易恢复，双方 shares 重构 oracle PASS。
- 旧 receipt-format v2 API-only smoke（FTv1 package）：正常 commit/fetch 成功；注入 submit 响应丢失后 API 进入 recovery-required、无 share，重启 fetch 成功；这是 v2 API callback/conformance smoke，不是 FTv3 目标规模证据。
- 人工在 precommit 时 SIGKILL escrow：P0/P1 exit 11、无 share，A exit 137；restart 检查为 ABORTED_NO_FETCH。该路径是故障注入，不是自然崩溃、自然读写故障或密码学攻击证据。

标准/非标准 callback 异常和状态不确定均是 TEST_ONLY 注入。自然业务 I/O 故障本轮没有复现，沿用 NOT_REPRODUCED。没有把人工注入写成超时自然发生。

### N=1000 oracle 案例与资源诊断

在最终 FTv3 E2E 二进制上顺序运行 N=1000、K={1,8,500,999,1000} 五案；另外同一批诊断运行含 N=1、N=128/K64、N=256/K128 三个 smoke。每个调用内 P0/P1 必须同时运行以完成协议；K 案例逐次执行以减少峰值内存。每个 N=1000 案例都记录 oracle PASS、双方进程 exit 0、BOTH_COMMITTED、服务重启恢复 PASS、package format 6/context format 2。冻结 oracle 检查输出长度为 N、每位为 bit、恰 K 个 1、XOR 重构与 signed Q20.12 稳定排序 mask 一致；测试覆盖极值、负数、重复值和同分稳定次序。

N=1000/padded 1024 的公式/观察分开：gate count 88,626,688；每方 package 33,366,516 bytes；观测最大 application payload 1,048,572 bytes。协议应用 payload 上限为 1 MiB；外层 framing header 不计入 payload，因此 4-byte 差距不是 frame header 计入方式。每方峰值 RSS（KiB，按 K 顺序）为：P0 940,432 / 940,456 / 968,084 / 940,428 / 940,684；P1 940,424 / 940,468 / 967,972 / 940,428 / 940,684。未将这些诊断量写成性能、RTT 或网络结论。N=128 smoke 包为 485,964 bytes/方、最大 payload 159,860；N=256 smoke 包为 1,989,252 bytes/方、最大 payload 655,476。

这些是有限目标用例的独立功能接收，不支持任意输入/所有 N、K 的正确性证明，也不扩展 N≤256 的条件安全证明到 N=1000。

## 5. API-v2 源码合同与调用点

S51 新 API 声明位于 VFSS/include/moe_topk/score_to_mask_v3_gmw.h:279，定义位于 VFSS/src/moe_topk/score_to_mask_v3_party.cpp:522。全仓只有这一份 production declaration/definition；调用点仅在 TEST_ONLY process E2E（score_to_mask_v3_process_e2e_test.cpp:918），没有生产仓内调用者。关闭 opt-in 时头文件拒绝使用；没有添加旧 API 降级路径。外部消费者和 ABI 兼容未由仓内证据覆盖。

party.cpp:835-895 先产生/加密本地 share、提交；submit callback 任意异常都映射 COMMIT_STATUS_UNKNOWN，返回恢复状态且不交付 share。提交 receipt 格式无效时返回 COMMIT_RECEIPT_INVALID。fetch callback、receipt 不匹配、解密或 share 不一致时返回 COMMITTED_FETCH_REQUIRED，保留 transaction 和 receipt，不返回本地预提交 share。只有 fetch 回执相同、解密内容等于本地 share 时 API 输出本方 fetched share。该实现解决的是客户端输出/恢复合同，不提供物理同时返回。

## 6. 超时/调度源码审计

| 边界 | 源码与语义 | 客户端/对端可观察结果 |
|---|---|---|
| Protocol I 通道默认值 | protocol_i_transport.h:60-62，timeout 默认 2000ms；Select5 config 默认 timeout_ms=0，runtime 要求调用者提供正值（protocol_i_project_derived_select5.h:34、runtime validation）。 | 未设置时配置拒绝；发送/接收异常会中止本地执行并尝试 best-effort ABORT。 |
| 帧读写 | protocol_i_transport.cpp:175-195, 378-447, 480-534。每个 send/receive 分别建 steady_clock deadline；partial I/O 循环、poll、TLS WANT_READ/WANT_WRITE 都用该帧自己的 deadline；header 与 payload 共用该帧 deadline。每个新帧重置 deadline，不是整次调用全局 deadline。 | frame timeout / poll / EOF / TLS record error 抛出；对端可能先见本帧、部分帧或无帧；后续逻辑按异常路径 abort。 |
| TLS handshake | protocol_i_transport.cpp:308-310、337-375；显式 complete_authenticated_handshake() 用当前 channel timeout 建独立握手 deadline；若握手由 read/write 内部触发，则使用该帧 deadline。证书链及主机名校验失败抛错。 | 握手失败/超时以 TLS 或 frame 错误终止；它有局部 deadline，但不是全任务 deadline。 |
| Select5 多阶段调用 | protocol_i_project_derived_select5.cpp:495-517、ordered_exchange 与各 schedule frame call-site、2168、2290-2307；config.timeout_ms 用于每个 frame/channel。 | 一方可能在具体 phase 失败，另一方进入对称的等待/失败路径。未配置跨全流程总 deadline。 |
| escrow client TCP connect | score_to_mask_v3_escrow.cpp:512-529。getaddrinfo 与 blocking connect 未显式 deadline；建立 channel 后 60,000ms 用于其 handshake/各单独 framed receive/send。READY、request 和 response 各是独立操作；fetch 再开新连接。 | DNS/connect 可能等待超出后续 60s channel timeout；响应超时或断链时 callback 失败；submit ambiguity 映射为 recovery-required。 |
| escrow service 等第二提交 | header score_to_mask_v3_gmw.h:90-95 默认 transaction_wait_timeout_ms=30000 且该参数绑定 signed metadata/context；server score_to_mask_v3_escrow.cpp:570-593 用同一值作为服务 channel timeout，单份提交写入 PENDING 后再启动 transaction wait deadline，以约 20ms 间隔查询；到期仍 PENDING 则原子标 ABORT，否则给 COMMITTED/token 响应。 | 服务到期时会写 ABORT 并发回 ABORT 响应；客户端解析器只接受 COMMITTED，解析异常被 API-v2 的 submit catch-all 映射为 COMMIT_STATUS_UNKNOWN（不返回 share，需恢复查询）。该显式 ABORT 响应路径未作为本轮单独案例运行。party 也可能先因自身帧 deadline 观察到失败/状态未知；A 状态依实际到达顺序与等待窗口决定。 |
| 重启与恢复 | score_to_mask_v3_escrow.cpp:534-539 把 restart 遗留 PENDING 标 ABORT；API-v2 fetch 走新 TLS/RPC 并校验同一 receipt/本方密文。 | 已提交可以本方认证恢复；未提交中止无 share；无统一全调用 retry/cancel orchestration。 |

源码只定义了本地 deadline 和状态机，并未定义真实 DNS、OS scheduler、拥塞、TLS record 延迟、party 计算时间、进程重启分布等环境模型。本轮没有实际等到自然 timeout；S52 的成功案例是运行观察，不是对超时条件分布的证明。

## 7. 两种安全主张及首个断点

### A. 外生 SuccessFence

S50 Kpre3 给出 N≤256、理想 IID FTv3、cap_success 下单方 pre-escrow view sampler；S51 Kpost3 以真实因果诚实对端第二 deposit 和 A 的 PENDING→COMMITTED/token/fetch 状态机给出成功后缀 sampler。若理想执行环境外生承诺：本次调用所有需要的合法应用消息都在各自有限 deadline 前得到处理，并以该成功调度为前置条件，而非从真实时间轨迹筛选成功样本，则该 Fence 固定后缀成功支路。于是对每个 b∈{0,1}，在匹配的 ideal FTv3、A token、输入/输出和 L_S29 条件下，可以先运行 Kpre3，再由诚实 peer 与 ideal A 生成 Kpost3，形成应用消息/API 成功视图组合。

这个 PASS_WITH_SCOPE 是理想应用消息视图结论：没有抽样真实时长、超时通知时刻或调度轨迹；不证明 wall-clock 隐私、实际 timeout-selection、可用性、崩溃前缀或部署网络。该组合 N≤256；S48 的 N≤1000 只属于功能证据。

### B. 真实有限超时成功样本

令 Q_b 表示允许给 party simulator 的输入、理想本方输出、本方 package/static state、公开调用参数与 L_S29；令 Vpre_b 是 pre-escrow 应用 transcript；cap_success 是 S49–S50 理想有界发行成功事件；S_actual 是真实通道/服务的每个相关 deadline 均成功、诚实 peer deposit 进入 COMMITTED、响应与本方 fetch 均完成的事件。所要求的主关系可写为：

~~~text
Law(Vpre_b | Q_b, cap_success, S_actual=1)
    ≈ Law(Vpre_b | Q_b, cap_success).
~~~

一般条件化恒等式说明缺少什么。对 support 内的 v，

~~~text
Pr[Vpre_b=v | Q_b,cap_success,S_actual=1]
 = Pr[S_actual=1 | Q_b,cap_success,Vpre_b=v]
   Pr[Vpre_b=v | Q_b,cap_success]
   / Pr[S_actual=1 | Q_b,cap_success].
~~~

因此若不构造受权限约束的真实成功条件 sampler，至少要证明成功核 Pr[S_actual=1 | Q_b,cap_success,Vpre_b=v] 对 Vpre_b 条件常量（或给出可量化的偏差界）。S51 的外生 SuccessFence 是外部保证，而不是该核；本轮没有网络/调度分布、任何关于成功概率下界的模型，也没有成功核与 pre-view 条件独立的等式。且实际 S_actual 由多个独立 frame deadline、无界 connect、A 的 PENDING 等待截止、响应及恢复等共同决定；观察到 E2E 成功只给出若干被采样轨迹，没有给出条件 law。

首个不能继续的证明步骤是把理想成功 Fence 替换为“真实运行后条件于 success”的随机事件时，以上 Vpre 条件联合律的替换。它不等于发现代码攻击、随机崩溃或有具体泄露反例；只是当前理想 sampler/假设未覆盖这个条件变量。没有理由扩大 L_S29：最小下一步是在安全模型里二选一明确声明外生 deadline-safe scheduler/Fence，继续只主张应用消息 view；或由系统所有者提供脱敏部署调度/时延模型，随后证明成功核对 Vpre 条件独立或给出真实条件 sampler/界。不得通过将 wall-clock/timeout/action shape 私自增列 L_S29 来关闭门。

## 8. Reviewer、分层状态与未运行项

执行者未参与 S51 runtime 实现或 S50/S51 推导；本次实际进行了独立 fresh build 与测试。S51 非作者 reviewer /root/s51_independent_review 未参与 S47–S51 实现/推导，先前独立复核了 S29 来源、Kpre/Kpost 条件分布、诚实 peer deposit、A 状态机与 token/fetch、两方对称性及超时选择缺口；其只读审阅，没有运行 build/test。S52 没有新的独立密码学 reviewer，故不把此先前审查说成对本轮报告/新构建证据的第二人复核。功能接收由本轮未参与 S51 实现的接收者完成。

- S52 API-v2 接收与 N≤1000 指定有限案例：PASS_WITH_SCOPE。
- default-OFF 隔离与 opt-in production target：PASS_WITH_SCOPE；OFF 全部测试/外部目标的整仓 build 因无关 bitpack link failure 未通过。
- S50 Kpre3 + S51 Kpost3 under ideal exogenous SuccessFence：PASS_WITH_SCOPE（S51 的非作者理论复核范围）。
- actual finite timeout-conditioned full view：OPEN，首个缺失关系如上。
- low-probability issuance abort acceptance：OPEN_CONFIRMATION。
- natural business I/O fault：NOT_REPRODUCED。
- real Dealer/DRBG、A token RNG deployment、signature/encryption/TLS bridge、full party-view composition：OPEN。
- N>256 security extension：NOT_IN_SCOPE/OPEN。
- formal LAN/WAN, RTT, AAV86, performance: NOT_RUN / NOT_MEASURED。
- source/runtime patch: NONE；本轮无 runtime defect reproduction。

本轮不重复 S35/S40 全矩阵，不执行完整性能基准，也不声称完整安全通过。
