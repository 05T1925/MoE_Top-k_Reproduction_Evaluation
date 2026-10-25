# M6B-I-PROJECT-SELECT-5ITER-S53 详细复核报告

日期：2026-10-25
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

本路线实现项目自有的五迭代稳定 Select：signed Q20.12 分数降序、同分按原始索引升序，并输出原始输入顺序下恰有 K 个 1 的 Top-K XOR mask shares。它借鉴 BB90 研究思路，但不是 BB90 原算法、BMW16 或 BB90+DCF 复现。本报告只裁决理想 SuccessFence 与实际有限超时成功筛选的安全组合范围；不改变输入、输出、角色、托管流程或泄露合同。

## 1. 结论摘要

| 项目 | 状态 | 裁决 |
|---|---|---|
| S52 精确来源接收 | `PASS_WITH_SCOPE` | 固定 commit/tree/parent、9 个 Git blob 和规范化 digest 核验通过。Windows checkout 下两个 Markdown raw SHA 受 autocrlf 影响，但 Git blob 一致。 |
| 外生 `SuccessFence` 下 S50 `Kpre_3` + S51 `Kpost_3` | `PASS_WITH_SCOPE`（继承） | 只组合理想模型中的成功应用消息视图；S53 不重证继承引理，也不声称真实时钟或成功率。 |
| 实际有限 deadline 条件化 party-view | `OPEN` | 首个未闭合项为 `Law(Vpre_b | Q_b,cap_success,S_actual) ≈ Law(Vpre_b | Q_b,cap_success)`，两方都需要成立或有权限正确 sampler。 |
| S52 功能接收 | `PASS_WITH_SCOPE`（继承） | 7 个 API-v2/托管状态案例，N=1000 的 K={1,8,500,999,1000} 五案及 N=1/128/256 smoke；本轮没有重跑。不得外推为全输入正确性或安全证明。 |
| 自然业务 I/O 故障 | `NOT_REPRODUCED`（继承） | S52 timeout 与缺行是 TEST_ONLY 注入；服务结束码 143 是 harness 受控停止。 |
| BUILD/TEST 与性能 | `NOT_RUN` / `NOT_MEASURED` | 本轮仅源码、manifest 和证明模型审查。 |

项目正常交付合同保持不变：离线可信发材方结束后，双方各自在进程内形成并加密自己的份额；在线托管方只存密文，收到双方密文后共同提交；双方随后各取回自己的 share，API 实际返回时刻可不同。本报告不把“双方成功”解释成物理同时返回。

## 2. 固定对象、来源和证据层级

S53 在新的隔离 worktree `C:/Users/28641/.codex/worktrees/m6b-i-project-select5-s53/MoE_Top-k_Reproduction_Evaluation` 从精确 S52 commit 建立分支 `codex/m6b-i-project-select5-s53`。基线为：

| 项 | 值 |
|---|---|
| S52 commit | `578ecadfe807748a3e2fada54ca11efe1b00d28c` |
| S52 tree | `e81d8e201fcd69c262f4d18360249a748bdc1490` |
| S52 parent | `1cc1e5202b3a603b55aaae985704933d70db0794` |
| latest `origin/main` | `c3926c68fd14f270faa8b55234311071947fa080` |
| ancestry | S52 是该 `origin/main` 的后代；S53 分支从固定 S52 candidate 开始，以保留用户要求的精确源码 |
| S52 manifest canonical delivery SHA-256 | `0c7ff49958df42086f20f69fb7a5936092ef356876959e3fe049d4f908300ad9` |
| S52 manifest 文件 raw SHA-256（本次 checkout） | `547bf0a9db112779a3571da032d37859c2ef87c18a086d40e76a9268b4c90c23` |
| S52 worktree 初始状态 | clean；本轮在新 worktree，不改 S52 原 worktree |
| `VFSS-baseline/` | diff empty |

复核 S52 manifest 的 9 项 `delivery_files`：通过 `git rev-parse <S52 commit>:<path>` 对固定 S52 tree 做 9/9 Git blob 核验，均与清单相符；以清单记载 raw SHA 按 ordinal path 排序重算 canonical digest，结果与 manifest 完全相符。当前 Git 使用 `core.autocrlf=true`。S53 已在 `docs/IMPLEMENTATION_PLAN.md` 追加本轮摘要，因此当前工作树该文件的 blob/raw hash 不再等于 S52 旧行；此外 S52 决策和报告两个 Markdown 的 checkout raw SHA 因 CRLF 与 S52 记录不同，但固定 S52 tree 中的 blob 均匹配。故当前工作树 8/9 行仍等于旧 Git blob，基线 S52 tree 为 9/9；这三项差异分别是本轮计划更新和可逆换行转换，不是被测源码替代或来源冲突。清单 canonical digest 使用其记录的 raw SHA 行计算，结果匹配；没有从桌面主工作区取文件。准确逐项列表见 S53 source/evidence manifest。

来源关系：S48/S49/S50/S51/S52 证据均按相应历史 manifest 和提交链读取。S50 的 `Kpre_3` 结论只限 N≤256、理想 IID FTv3、`cap_success`、静态半诚实、无故障 GMW pre-escrow view；S51 的 `Kpost_3` 只限理想 A token RNG、诚实 peer 真实因果 deposit 和外生成功调度下的 escrow/API 后缀；S52 收录了两者的组合边界以及功能接收。本轮将这些作为带范围的历史证据，不把其本地测试摘要反向写成论文结论。固定 S52 tree 中未跟踪 S50 `CASES` JSON；S50 manifest 的交付是决策、报告、gates、计划和 manifest，符合证明/文档审查不运行案例矩阵的记录。S51 的 4 个 cases 是人工注入/正常 API 顺序验证；本轮只继承其状态与审查结论。

证据层级分开如下：

1. **论文定义**：BB90 等只作为研究思路参照，本轮未裁决论文等价性。
2. **本地代码事实**：超时参数、deadline 创建位置、连接和 SQLite 状态机来自固定 S52 Git blobs。
3. **项目派生证明**：SuccessFence 下组合是 S50/S51 的范围限定引理，经 S52 非作者复核后记录；不是 BB90 定理。
4. **待验证假设**：实际部署网络、scheduler、OS、TLS、DB 和工作负载延迟联合律，以及成功筛选不依赖秘密视图的条件关系，仍未提供/证明。

## 3. 两个不同的安全命题

### 3.1 命题 A：外生 SuccessFence

定义理想环境 `E_fence`：调用开始时提供已经认证并可用的双方 peer endpoint 与 A endpoint；对 S50/S51 成功消息执行中的每次 TLS 握手、frame 发送/接收、A 的第二份 deposit 等待、COMMITTED 响应、authenticated fetch 和 API 完成，环境外生保证操作在该操作配置的 deadline 前完成。若建模到连接建立，也由该理想环境保证 connect/accept 在使用通道前完成。对固定公开调用参数和 `L_S29`，此环境不以消息明文/秘密状态或实际时长来筛选转录；成功是模型前提，而不是运行时事件的条件化样本。

在此模型中 S50 的 `Kpre_3` 先生成单方前缀；S51 的 `Kpost_3` 随后由本方 simulator、使用自己材料的诚实 peer endpoint 和 A 的理想成功状态机按协议因果执行：本方 authenticated submit → PENDING → peer authenticated deposit → COMMITTED/token → 本方响应 → 本方 authenticated fetch/decrypt/核验/API share。理想接口不向本方 simulator 交付 peer package/share/凭据、A-only context 或 T 私钥。由已审前缀 kernel 接续后缀 kernel，得到既有成功**应用消息**转录分布。

因此可继承写作 `PASS_WITH_SCOPE`。条件包括：ideal IID `F_T/v3` 与 `cap_success`；A 理想独立 token RNG；一方静态半诚实；A/T 受信且不与被腐化方合谋；诚实对端按自身同次调用材料和输出进行真实因果提交；N≤256（因为 Kpre3 的安全证明范围）；无 I/O/进程故障；外生 `E_fence`。结论不包含 wall-clock 延迟分布、真实 timeout 结果、真实失败概率、可用性、故障前缀隐私、A-only 腐化、掉电耐久或 N>256 安全扩域。

`E_fence` 不等于代码内的有限 timeout 全部安全，也不等于连接必然成功。现有 transport 对 DNS/connect/accept 没有统一截止时间，所以 fence 对这些阶段的“端点已就绪/及时可用”是理想外部假设，不是源码保证。

### 3.2 命题 B：真实有限超时下的成功样本

固定一方 `b∈{0,1}`，令：

- `Q_b=(x_b,z_b,L_S29,pkg_b,static_b,P)`：S50 定义的本方输入、理想输出 share、批准泄露、本方材料包/静态状态与公开参数；
- `cap_success`：理想 FTv3 发材未触及拒绝上限；
- `Vpre_b`：本方完整 pre-escrow GMW/Select application-message transcript；
- `W`：实际网络排队/时延、OS 调度、线程可运行时序、peer 与 A 处理、DNS/TCP、TLS/DB 服务状态及其他外生或部署随机变量；
- `S_actual`：本次真实执行的双方 peer 消息、两份 authenticated deposit、托管提交响应、fetch/decrypt 和 API 成功都完成，且所有对应有限 deadline 均未先到期。

令在固定 `(Q_b,cap_success)` 下
`μ(v)=Pr[Vpre_b=v | Q_b,cap_success]`，并令
`w(v)=Pr[S_actual | Vpre_b=v,Q_b,cap_success]`，其中概率对尚未抽样的 peer/A 隐藏状态、部署环境及 W 求平均。则真实成功条件分布满足 Bayes 重加权：

```text
Pr[Vpre_b=v | Q_b,cap_success,S_actual]
  = μ(v) · w(v) / E_{u←μ}[w(u)]
```

若 `Pr[S_actual | Q_b,cap_success] > 0`，相应统计距离可写为：

```text
TV(Law(Vpre_b | Q_b,cap_success,S_actual), μ)
  = 1/2 · E_{v←μ}[ | w(v)/Eμ[w] − 1 | ].
```

计算不可区分版本还需一个 PPT 归约把上述密度偏差界住，或直接给出权限正确的条件 sampler。一个充分但未由本轮证据建立的条件是 `w(v)` 对所有合法 `v` 在固定 Q 下相同（或具有经证明的可忽略相对变化），且成功概率不被压到使条件化放大不可控；更一般条件可以是直接采样/耦合该条件 law。

S52 未给出 `W` 的部署分布、`W` 与 `Vpre_b` 的条件独立关系、实时计算/消息处理的常数时间性质，亦没有 `w(v)` 的界或条件 sampler。帧长度/phase 的公开确定性只说明消息形状可由公开参数和既有 `L_S29` 描述；它不蕴含 frame 延迟、线程运行时长、TLS/DB 处理时长或成功事件概率与 `Vpre_b` 独立。S52 成功 E2E 是少量已成功样本，不能证明上述密度比接近 1；S52 timeout 注入是一个人工故障路径，也不估计真实成功样本分布。故命题 B 保持 `OPEN`，不判断有无攻击或该等式是否不可能。

## 4. 固定源码中的时限、调度边界和 party 可见结果

| 阶段 | 固定 S52 源码位置与实际计时 | timeout/成功路径可观察结果 | S52 相关证据覆盖 |
|---|---|---|---|
| Protocol-I peer channel 配置 | `VFSS/include/moe_topk/protocol_i_transport.h:60-62` 默认 `timeout_ms=2000`；Select5 runtime 由调用者传入 `config.timeout_ms`，在 `protocol_i_project_derived_select5.cpp:495-517,520-529,2161-2169` 创建 channel；config 默认 0 并在 `protocol_i_project_derived_select5.cpp:679,795,1395` 等入口要求正值。 | 实际执行期限是调用方参数，不能一概称生产固定 2000ms。E2E 普通目标曾传 15000ms，超时注入 probe 传 1200ms（S52 测试源 `score_to_mask_v3_process_e2e_test.cpp:730,929,1776-1780`）。 | 成功的 S52 N=1000/兼容 E2E 覆盖一组 15000ms 参数；没有自然延迟分布测量。 |
| Peer TLS 握手 | `protocol_i_transport.cpp:308-311,337-375`；显式 `complete_authenticated_handshake()` 用 `now+timeout_` 单独期限。若第一次 send/receive 懒触发握手，`exact()` 将其放在该 frame deadline 内（`379-380`）。 | 握手失败/timeout 作为异常；本方 party v3 顶层失败路径调用 `abort_best_effort()`，返回 `kAborted/ABORT_V3`、清除 API 返回 share，但不声称临时内存已擦除（`score_to_mask_v3_party.cpp:903-918`）。 | 没有在真实部署中测 TLS 握手延迟分布；S52 功能案只覆盖成功握手或 TEST_ONLY 路径。 |
| Peer frame send/receive | `protocol_i_transport.cpp:378-445,480-533`。每次 `send`/`receive` 开始时独立构造 `steady_clock now+timeout_`；同帧 header 和 payload 共享同一个 deadline；`exact` 通过循环处理 partial I/O、TLS WANT_READ/WANT_WRITE 和 `poll`。成功一帧后下一帧重新计时。 | timeout、EOF、poll/TLS 错误或 frame 校验异常进入 party 的 fail/abort 返回；消息阶段数与已完成 frame/byte 计数在 output diagnostics 可见。失败时的 prefix counter/phase 属失败路径观察，不因此纳入 `L_S29`，且 crash/abort-prefix privacy 未证明。 | S52 正常案例完成 88–106 个 phase/同量级 frame；没有自然 partial-frame stall/EOF 故障复现。 |
| A server accept / connection | `score_to_mask_v3_escrow.cpp:626-635` 使用阻塞 `accept4`，接受后 detached thread 处理；未见 accept deadline。 | A 进程/监听/调度状态影响请求何时进入服务；A-only 内部状态不交给 party。 | 未测试自然 accept 阻塞，也未给部署调度界。 |
| A server TLS handshake / READY / request / response frames | `score_to_mask_v3_escrow.cpp:566-586,600-613`；server channel 使用签名 context 中 `transaction_wait_timeout_ms`。`complete_authenticated_handshake()` 是独立 handshake deadline；后续 `send/receive` 分别建 frame deadline。 | 服务端 request 错误只写 stderr 并关闭/结束连接（`615-622`）；客户端看到 TLS/EOF/timeout/parser 错误。party submit callback 错误一般回 `COMMIT_STATUS_UNKNOWN`；不能据此推定 A 未提交。 | S52 API callback loss/failure 案是 TEST_ONLY callback throw，不是自然 TLS/网络延迟。 |
| A 的 PENDING 等第二次 deposit | `score_to_mask_v3_gmw.h:92` 缺省 30000ms；signed context 序列化绑定该字段见 `score_to_mask_v3_gmw.cpp:257`，范围检查 `855-859,1408-1412` 限 1..60000ms。A 在第一份密文 `submit_ciphertext` 后从 `590-595` 开始单调时钟 deadline，每 20ms 查状态；到期仍 PENDING 时 `597-599` 写 durable ABORT 并重查。 | 只有经 TLS 身份、frame、transaction ID、空密文/零 receipt 校验的持久 ABORT 才让提交方看到 `ESCROW_ABORTED`；若响应丢失或 timeout 发生在不确定边界，party 结果为 `COMMIT_STATUS_UNKNOWN`。A context 与 DB state 属 A 视图，不是 `L_S29`。 | 一个 S52 server-deadline TEST_ONLY 案：P0 明确 ABORT、P1 recovery/unknown、两方都无 API share、重启状态 `ABORTED_NO_FETCH`。这证明该注入合同路径，不证明自然 timeout 采样。 |
| A client DNS/TCP connect | `score_to_mask_v3_escrow.cpp:515-534,678-699`：同步 `getaddrinfo`、blocking `connect` 后才建 framed channel；未设置 resolver/connect 总 deadline。 | DNS 失败或连接错误是 submit 前/submit 阶段异常；连接阻塞时间可超出后续每帧 timeout。失败由 API 映射为 unknown/recovery 或中止，视处于哪个 API 阶段。 | S52 无慢 DNS/连接注入、无部署网络测试。 |
| A client TLS / READY / submit / commit response | `score_to_mask_v3_escrow.cpp:691-717`：client channel 每 frame 60000ms；显式 handshake 独立 60000ms，welcome、request send、response receive 分别调用 framed I/O 并各有自己的 frame deadline。 | 第一份密文经 submit 可能进入 PENDING，也可能因 peer 已提交直接 COMMITTED。response 异常或丢失时 API 不能判定 A 是否已 commit，输出 `COMMIT_STATUS_UNKNOWN`、不返回 share。 | S52 有丢 receipt/callback 异常注入，不是自然响应延迟。 |
| A committed fetch / recovery | API v2 的调用与返回顺序见 `score_to_mask_v3_party.cpp:831-902`；每次 fetch 通过 `request_service` 新建连接和 TLS，client handshake/frame 仍各自 60s。先核对 receipt，再解密 recipient ciphertext 并与本地计算 share 比较。 | 已知 COMMITTED 后 fetch、解密或比对失败返回 `COMMITTED_FETCH_REQUIRED`，保留 transaction/receipt，不返回 share；成功时只返回本方 `fetched_share`。retry/recovery 是额外 API 行为，不由单个 global deadline 约束。 | S52 有 post-commit fetch callback 异常并经 restart recovery 的 TEST_ONLY 案；没有自然网络恢复分布。 |
| 整次 Select/API | 仓内 API 有多个顺序 frame、submit、fetch；本轮未在该调用链发现单一全局 wall-clock deadline。 | 成功/失败可由多个局部 timeout、connect/accept、scheduler 和 service wait 共同决定；两方 API 返回时间允许不同。 | 成功 E2E 只证其固定环境样例能完成，不是 global deadline 或线上可用性证据。 |

上述表格是源码行为审查，不是安全或可用性证明。成功返回 `kCommitted` 表示本方取得并核验本方 share；并不表示双方 API 在物理同一时刻返回。发生异常且 API 未返回 share，也不能写成“本方进程从未在内存中形成局部 share”。

## 5. S52 功能/工程证据如何使用

本轮不重跑 S52 的案例，不把其运行记录当作新 S53 测试。准确继承范围：

- API-v2 七个 N=128/K=64 状态案例：正常；提交后响应丢失；fetch callback 标准异常；submit callback 非标准异常；fetch callback 非标准异常；A 等第二份 deposit 的显式 transaction timeout；删除 transaction state row。每案有不同的 party status/share、A 持久状态及恢复观察。
- 目标功能八案：N=1000、K={1,8,500,999,1000}；另有 N=1/128/256 smoke。所列成功样例均与冻结 oracle 匹配，两方只取回本方 share。案例不是全输入或全 `N,K` 穷尽证明。
- S52 final E2E 二进制 SHA-256：`81a5f05e6a9c75d85ea19c2e59729cd2cc0c9e724b68fae77c0c4333668043b9`；这是 S52 已记录二进制，本轮没有重建或运行。

S52 报告记录人工 transaction timeout 和缺行注入；S52 没有观察到自然业务读写故障，仍为 `NOT_REPRODUCED`。服务器结束码 143 是 harness 的受控停止。测试成功和注入响应分别属于功能/工程可靠性证据，不估计 `S_actual` 条件分布，不构成密码学攻击或安全证明。

## 6. 证明停止点与最小下一步

当前停止于真实有限超时的首个条件 law，不继续把已审后缀机械拼成完整真实成功样本。要保持现有 `L_S29`，最短选择是二选一：

1. **定理范围继续限定为 SuccessFence。**明确将安全陈述限定为理想外生成功调度下的应用消息视图；实际 wall-clock、timeout 选择和 availability 另行开放。无需改输出、角色或泄露；这就是当前可继承的有限范围结论。
2. **若要覆盖实际 finite-deadline 成功样本，先版本化环境模型，再证明条件 sampler。**需要明确 scheduler/OS/TLS/网络/DB 的随机变量、对手/环境能看见和控制什么、连接与 frame deadline、服务 context timeout、重试和取消语义，并证明 `w(v)` 对合法 pre-view 近似常数，或构造 `Law(Vpre|Q,cap_success,S_actual)` 的 PPT sampler。若只依赖部署测量，测量本身仍不能替代针对声明分布的理论条件；若需要把时间/timeout/status 作为泄露，须单独变更版本化泄露合同并获负责人批准，本轮不增列 `L_S29`。

不臆造 scheduler 分布，不把当前本机 phase、elapsed 或 S52 测试成功率作为其替代。当前没有可复核攻击样例，也没有可声称的不可行性结论。

## 7. 安全、故障、规模和性能边界

- ideal FTv3 发材与 `cap_success`、真实 OpenSSL DRBG/RNG 联合流、Dealer 签名/材料发行：`OPEN`。
- A token RNG、TLS/mTLS 部署桥、RSA-OAEP/AES-GCM 收件人加密及 hash binding 优势：`OPEN`。
- S50/S51 pre/post 子引理外的完整 GMW/Select/shuffle/output/escrow party-view 组合：`OPEN`。
- `LOW_PROBABILITY_ISSUANCE_ABORT_ACCEPTANCE=OPEN_CONFIRMATION`；未取得新增负责人记录。
- 自然业务读写/等待故障：`NOT_REPRODUCED`；S52 人工 timeout 与 callback/DB 注入单列为 TEST_ONLY 工程案例。
- N≤1000 仅为 S48/S52 有限功能案例；N>256 的安全扩域仍 `NOT_IN_SCOPE`，N>1000 未准入。
- 正式 LAN/WAN、RTT、AAV86、统一性能矩阵：`NOT_RUN/NOT_MEASURED`。算法迭代数、协议比较依赖层、应用交换 phase 和网络 RTT 不混用。
- v3/FTv3/API-v2 保持默认关闭并显式 opt-in；不创建 `secure` alias，不推送/合并。

唯一仍待确认的低概率产品合同问题：**合法输入下，离线材料准备若以极低概率达到拒绝采样上限并返回发材中止、本次调用作废且需用新 invocation 重试，是否可接受？**本轮不询问已经确认的 escrow、正常交付或人工故障边界。

## 8. 本轮执行、复核和变更

- 执行者延续 S50–S52 的来源/证明文档审查工作；本轮不是非作者独立复核。
- S52 记录的 `/root/s51_independent_review` 未参与 S51/S52 实现或推导，并 fresh Release 构建、运行 7 个 API-v2 案和 8 个目标/兼容功能案；该 reviewer 独立重推过 SuccessFence 与实际 finite-timeout 条件 law 的缺口，但未重证 S50/S51 子引理。S53 未声称 reviewer 复核本轮新增文字或代码。
- S53 没有可复现 runtime/合同缺陷；无 runtime 或测试代码修改。
- `BUILD=NOT_RUN`；`TEST=NOT_RUN`；没有 S35/S40 历史矩阵复跑；没有正式性能/RTT/LAN/WAN 测量。
- 交付只有 `docs/IMPLEMENTATION_PLAN.md` 与 S53 decision/report/gates/source-evidence manifest；`VFSS-baseline/`、S52 源码和历史 evidence 均未修改。

### 本轮可写声明

可写：**“在 ideal F_T/v3、cap-success、理想 A token RNG 和外生 SuccessFence 下，N≤256 的已复核 Kpre3 与 Kpost3 成功应用消息视图可按既有条件组合；项目自有五迭代 Select 的 N≤1000 指定功能样例仅继承 S48/S52 的有限 oracle 收据。”**

不可写：真实有限超时成功视图已安全；已完成全 GMW/Select/shuffle/output/escrow 模拟；已完成完整安全证明；论文 BB90 原算法已复现；功能或性能门已证明安全。
