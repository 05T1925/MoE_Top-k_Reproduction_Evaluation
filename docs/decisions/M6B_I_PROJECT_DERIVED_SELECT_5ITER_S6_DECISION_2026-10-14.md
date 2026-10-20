# M6B-I PROJECT_DERIVED_SELECT_5ITER S6 决策记录

日期：2026-10-14  
路线：`PROJECT_DERIVED_SELECT_5ITER`  
组合：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / EXPERIMENTAL`  
基线：S5 dirty worktree 的选择性冻结快照，base `c3926c68fd14f270faa8b55234311071947fa080`  
当前状态：**S5 基线异会话接收通过；S6 N≤256 有界功能通过；联合单方视图安全 OPEN；正式性能不准入。**

## 决策

1. **路线身份不变。** S6 是项目派生、稳定 Top-K 功能候选；不是 BB90 五迭代复现，也不继承 BB90/BMW16 的算法、比较界、概率或安全结论。六方案表中的 BB90 行保持原样，S1 `BB90_SOURCE=NO_GO_FOR_IMPLEMENTATION` 不变。
2. **接收身份分开记录。** 独立接收者未参与 S4/S5，在 fresh Release worktree 对 S5 source manifest 的 36 项逐项核对，35 项匹配；唯一差异是 S5 manifest 生成后又追加的 `docs/REFERENCE_MANIFEST.md`。S5 报告声明的 18 个源哈希全部匹配；S5 七个目标及十个成功/五个 no-mask 进程案例退出码均为 0。故 `S5_BASELINE_RECEIPT=PASS_BY_NONPARTICIPANT`。本 S6 实现者参与过 S4/S5；`S6_DELTA_INDEPENDENT_ACCEPTANCE=NOT_YET_RECEIVED`。
3. **移除 S5 的 raw `public_sample_seed`。** 仅从包或 peer binding 删除该字段仍不够：若双方以公开 session metadata 确定采样上下文，就能在比较时序前推导后续图。S6 删除这种派生 schedule context。P0 只在某个 reduction iteration 实际活动时，从 OS 熵源执行拒绝采样 Fisher–Yates，并通过该 iteration/phase 绑定的 TLS 消息发送当前 shuffled sample positions；P1 先验证样本大小、唯一性和当前 candidate 成员关系再继续。下一轮样本尚未生成。该集合等价于当前 sample-clique 的顶点/端点集合，不含 score、threshold、原下标映射或随机 seed。
4. **样本发布增加实际消息阶段。** 完整候选交换数改为 `9 + 2L + A`：固定适配路径 9 个 request-response exchange；每个非空比较依赖层 2 个；每个活跃样本集合发布 1 个；`L≤9`、`A≤4`，故最大 31。样本消息阶段不能被算法迭代数、CA 公式或 Protocol I 论文核心轮数吞并。
5. **M6S2 已接到 N=256 Select5 路径。** Dealer 逐槽生成、写 segment 和 manifest，只有 T 正常退出后由父 supervisor 发布 ready；party 按需读取并先 fsync durable per-slot lease，再释放 key bytes。N=256 全部指定 K 及重复 K=128 的 local TLS T/P0/P1 frozen-oracle E2E 通过。固定 M6B5 monolithic N=256 预检拒绝，N=257 streamed 预检拒绝；不得外推到 N>256。
6. **安全和性能门保持关闭。** S6 复核了实际 DCF key/threshold/mask/z 联系、全池未用 key、adaptive endpoint/地址、shuffle/逆路由、输出 share 和 abort/restart，但没有构造 P0/P1 的完整 simulator 或 hybrid，也没有发现可报告为已确认的具体攻击。准确结论仍是 `JOINT_VIEW_SECURITY=OPEN`；功能通过不构成条件安全。V3 仍 OPEN；V4 和正式 LAN/WAN 仍 `NOT_ALLOWED/NOT_RUN`。

用户在 S6 明确确认原始 `public_sample_seed` 保持不公开，特别是不在对应 endpoint/pivot/比较 bit 按时序发布前提前发送给 P0/P1。S6 不把 seed 写入 package、binding、party 消息或 E2E case JSON；当前泄露合同无 seed 扩展待确认项。若未来提出 seed 公开，须新版本合同并重审提前推导未来图的影响。

## 门禁裁决

| Gate | 裁决 | 边界 |
|---|---|---|
| `INDEPENDENT_ACCEPTANCE` | `S5_BASELINE=PASS`; `S6_DELTA=NOT_YET_RECEIVED` | 非参与者复跑 S5；S6 新增代码由 S4/S5 参与者编写 |
| `ALGORITHM_FUNCTIONALITY` | `PASS_BOUNDED` | `1≤K≤n≤256`；每个合法样本集合都保持 residual-rank 不变量，第五次 exact clique 闭合；资源/输入/通信异常统一 no-mask |
| `N≤128_FUNCTIONAL` | `PASS_BOUNDED` | S5 独立接收 10 成功/5 故障；S6 final E2E 另有 10 成功/5 故障 |
| `N=256_FUNCTIONAL` | `PASS_BOUNDED` | K=`1,2,8,64,128,255,256` 及重复 128；通过冻结 C++ stable oracle |
| `LEAKAGE_L_MAPPING` | `PASS_NO_RAW_SEED / CONDITIONAL` | 不再暴露可推导未来图的 seed/context；当前 sample positions、endpoint/bits、slot、长度/次数、masked operands 和统一 ABORT 留在已确认 L 内；这不是 simulator |
| `DCF_MEMBERSHIP` | `PASS_FUNCTIONAL` | 实际 uCMP/DCF ABI；stable packed key 唯一时 `1−strict_lt(τ,key_i)`，原序输出恰 K |
| `JOINT_VIEW_SECURITY` | `OPEN / NO_GO_FOR_SECURE_CLAIM` | 没有覆盖 correlated key/mask/z、全池、adaptive transcript、shuffle/output/abort 的组合模拟 |
| `DURABLE_MATERIAL_LEASE` | `PASS_LOCAL_POSIX_CONTRACT` | ready digest、一次性 durable lease、重启烧毁和并发单赢家；排除管理员删除/回滚，supervisor 必须按 T 成功退出屏障调用 ready |
| `AUTHENTICATED_TRANSPORT` | `PASS_FUNCTIONAL_TLS13_MTLS` | 错身份和帧错误 fail-closed；生产证书 provisioning/rotation/revocation 未测 |
| `MATERIAL_COVERAGE` | `PASS_N256_FUNCTIONAL` | streamed N=256 逐槽覆盖并按需 claim；same-UID 本地目录不是 party 隔离证据 |
| `N>256` | `NO_GO` | N=257 preflight 明确拒绝 |
| `EXPECTED_COST_CLAIM` | `NOT_ESTABLISHED_FOR_RUNTIME` | S2 理想 fresh uniform-without-replacement 结论保留为历史数学推导，不转成 getrandom/当前实现期望界 |
| `FORMAL_PERFORMANCE_READY` | `NOT_ALLOWED / NOT_RUN` | M6A V3 未关闭；V4/LAN/WAN 不启动 |

## 轮次与成本合同

- 算法 trace 为五个迭代槽；前四次最多两个比较依赖层/次，第五次 exact clique；最多九个非空比较层。
- 运行时完整 raw-score→原序 mask 消息 exchange 数为 `9+2L+A`；`A` 是即时样本发布轮数。N=256 final cases 实测为 24、29、31 等，按 `(L,A)` 逐例见 S6 E2E JSON。
- CA 通用比较编译关系 `2k+1` 若各自假设成立，针对 `k=9` 的**比较核心条件值**是 19；它未证明可吸收本实现的 `A` 次样本发布，也未由 CA compiler 实现。Protocol I 论文核心 3 轮是另一口径。
- N=256 逐方槽数 `5*C(256,2)+13*256=166,528`；观测 staging bytes 为 `185,925,312 B/party`（数据+manifest，未含 ready/lease 文件），manifest `12,656,192 B`，11 个 segment/party。完整 storage bytes、理论 worst comparison bound 和 E2E 观察计数分栏报告。
- `12N+C(N,2)` 仍是项目算法的最坏比较界：N=128 为 9,664，N=256 为 35,712，属于 O(N²)。N=128/256 E2E 的 min/median/max 是样本诊断，不是时间性能、期望界或高概率界。

## 实际联合视图安全结论

模型限定为每次只腐化 P0 或 P1 的半诚实观察者、可信且不合谋的离线 T、TLS 1.3 mTLS 私有认证通道。T 在生成阶段知道 permutation `Pi` 与双方离线材料，之后退出；T 与任何 party 串通不在范围内。Party 视图包括本方输入 share、全池本方 DCF key/mask share、流式未用槽清单、对方发来的 masked operands、打开的 comparator bits、当前/后续由这些 bits 选择的 endpoints/slot address、两轮前向 shuffle、逆路由和输出 mask share，以及 ABORT/重启状态。

实际 uCMP 生成阈值关系 `alpha=(m_left−m_right) mod 2^b`，再调用 `keyGenDCF(b,64,alpha,1)`；party 持有相应 local key/correction material 与 mask shares。在线每个逻辑比较交换 masked `z_left/z_right`，本地执行两个 `evalDCF`（输入与 sign-shifted 输入），交换 local result shares 并打开严格比较 bit。S6 对不同 active edge 请求按绑定地址领取 key；开出的 bits 决定之后候选和访问顺序。

首个未闭合的模拟项是上述 `alpha`、local DCF `k/g/v`、mask share 和可见 z/bit 的**联合分布**；证明必须继续覆盖 full used/unused key pool、adaptive 地址选择、shuffle 的 party-local `sigma/tau` 与 record mask、inverse/output 及 failure views。边际 `z` 均匀或 endpoints 已获准公开均不足以推出组合模拟。未找到 concrete distinguisher 只表示没有攻击证据，不是安全通过。N≤128 monolithic M2CP package 仍含当前两轮调用不消费的 DCF pool；N=256 streamed 的 metadata-only shuffle layout 不生成该未用比较池。两者实际视图不同，后续证明必须精确绑定版本/schema。测试子进程共享宿主 UID，可访问彼方测试材料目录；功能 E2E 不建立生产主机隔离。

## 下一阶段入口

1. 先由未参与 S4/S5/S6 的接收者按 S6 source manifest 在干净隔离 worktree 独立复建，重跑七目标及 23 条 E2E case；审核本决策中的 `9+2L+A`、N=256 slot/storage 和 sampler 口径。
2. 在任何 `secure` 命名或性能矩阵之前，为**M6B5/v2 + M6S2 streaming**分别给出 P0/P1 完整视图模拟/逐 hybrid；至少闭合 DCF key 与 z 的相关性、adaptive full-pool queries、shuffle/inverse/output 与 ABORT/restart。
3. 为生产部署定义彼方材料目录隔离、可信 supervisor 的持久 ready 发布、磁盘删除/备份回滚策略和证书运维；当前功能 harness 不验证这些运营边界。
4. M6A V3/V4 前置门按项目计划关闭后，重新申请正式性能准入；其前不运行正式 LAN/WAN。
5. BB90 来源工作独立保持 S1 `NO_GO_FOR_IMPLEMENTATION`；若以后取得 BB90 完整来源，另建来源/算法路线审查，不得将本 S6 代码改名。

证据入口：[S6 详细复核报告](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S6_REPORT_2026-10-14.md)、[逐案 JSON](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S6_E2E_CASES_2026-10-14.json)、[输入冻结记录](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S6_INPUT_FREEZE_2026-10-14.json)、[源码/二进制/日志哈希清单](../reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S6_SOURCE_MANIFEST_2026-10-14.json)。
