# M6B-I-PROJECT-SELECT-5ITER-S6：S5 异会话接收、sample-seed 泄露裁决与 N=256 有界 E2E

日期：2026-10-14  
路线：`PROJECT_DERIVED_SELECT_5ITER`  
组合：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + DCF / EXPERIMENTAL`  
结论：**S5 基线由非参与者独立接收通过；S6 的 N≤256 功能候选通过；`JOINT_VIEW_SECURITY=OPEN`；N>256 不支持；正式 LAN/WAN 不准入。**

本报告只记录项目派生稳定 Top-K 功能路线。它不是 BB90 五迭代复现、不是 BMW16 算法实现，也不继承二者论文中的复杂度、概率或安全结论。S1 BB90 来源闭合、BMW16-derived 方案、S34 接收和 M6A V3/V4 门分别保持原身份。

## 1. 被测对象和工作区接收

| 项目 | 实际身份 |
|---|---|
| S6 主 checkout | `C:\Users\28641\.codex\worktrees\m6b-project-derived-5iter-s6`，分支 `codex/m6b-project-derived-5iter-s6` |
| S6 HEAD/base | `c3926c68fd14f270faa8b55234311071947fa080`；任务变更未提交，无新的 Git revision |
| S5 source worktree | `C:\Users\28641\.codex\worktrees\m6b-project-derived-5iter-s5`，分支 `codex/m6b-project-derived-5iter-s5`，HEAD 同为 c3926；存在 dirty/untracked 内容 |
| main/origin/main | 本次源基点均为 c3926；S5/S6 worktree 的未提交源并未因此视为已合入 main |
| 桌面主 checkout | `feat/m6a-performance-evaluation`；开始时有用户修改，S6 未修改该 checkout，也未覆盖或移走其差异 |
| S4/S5 实现者身份 | 本报告作者参与过 S4/S5；因此 S6 源码改动本身不是异人独立接收 |

S6 使用选择性冻结的源快照，而不是复制 S5 worktree 的全部 dirty 状态。输入文件路径/哈希、S4/S5 来源状态、排除路径和 S5 manifest mismatch 见 [S6 input freeze](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S6_INPUT_FREEZE_2026-10-14.json)。其中 S5 source manifest 36 项有 35 项匹配，18 个 S5 报告源哈希全部匹配。唯一不同项是 `docs/REFERENCE_MANIFEST.md`：manifest 期望 `3023056355c55df5992facd457e256dabc7d6ca2196c1a795c7ec557b7246d6e`，实际 S5 文件为 `6b9bd2e1cc6fcd6212ca8159cd12cc08c29419125c3003ad85d21c78d1876b99`，原因为 manifest 后追加的 S5 文档说明。它不是运行时代码。S4 用户的 `docs/PAPERS.sha256` 差异被排除，没有被 S6 覆盖。

### 1.1 非参与者接收 S5

审查者只参与过 S1/S2 source review，没有参与 S4/S5 的实现或审查。其在独立 review build 中使用 Ubuntu 20.04.6 WSL2、CMake 3.28.4、GCC 9.4、OpenSSL 1.1.1f、Release、EMP OT OFF、Select5 ON，重新 configure/build 并运行以下七个目标，均退出 0：

```text
moe_topk_project_derived_select5_conformance_test
moe_topk_project_derived_select5_material_store_test
moe_topk_streamed_material_slot_store_test
moe_topk_tls13_transport_conformance_test
moe_topk_project_derived_select5_process_e2e_test
moe_topk_m2_ucmp_conformance_test
moe_topk_m2_parallel_shuffle_conformance_test
```

独立复跑的 E2E 为 10 个 frozen-oracle success 和 5 个 `UNIFORM_ABORT_NO_MASK` 故障 case；S5 中的 M6S2 仍只是存储测试，没有 Select5 runtime 调用。该接收复核确认的是**S5 输入快照**，不接收 S6 后续修改。外部证据目录为 `C:\Users\28641\.codex\evidence\S6_S5_RECEIPT_2026-10-14`；`receipt_summary.json` SHA-256=`46e9225981d820ec5f18a388ad314b0d0f112a2df396a7c4b8d3b7e940d63b10`，`run2_console.txt` SHA-256=`9ac3c10a608126cb40360259de91c19c94c048c37e884bb15c536ab786e001f0`，`binary_sha256.txt` SHA-256=`d9ccab345b2240a203693f13b96c1853eb2155c774a66c765f80581284656220`。`INDEPENDENT_ACCEPTANCE` 因此分解为 `S5_BASELINE=PASS_BY_NONPARTICIPANT`、`S6_DELTA=NOT_YET_RECEIVED`。

## 2. 核心结果摘要

| Gate | 结果 | 证据边界 |
|---|---|---|
| `S5_RECEIPT` | `PASS_BY_NONPARTICIPANT` | source manifest、七目标和 10+5 既有 S5 case 独立复跑 |
| `S6_DELTA_INDEPENDENT_ACCEPTANCE` | `NOT_YET_RECEIVED` | 实现者参与过 S4/S5 |
| `ALGORITHM_FUNCTIONALITY` | `PASS_BOUNDED` | 所有合法 sample tape 均由 residual-rank 不变量和 exact 第五层保证答案；比较数/耗时没有因此取得期望保证 |
| `N≤128_FUNCTIONAL` | `PASS_BOUNDED` | S6 final E2E 10 成功/5 故障，另有 S5 独立接收结果 |
| `N=256_FUNCTIONAL` | `PASS_BOUNDED` | 8 个 streamed 成功 case，与冻结稳定 oracle 逐位一致 |
| `LEAKAGE_L_MAPPING` | `PASS_NO_RAW_SEED / CONDITIONAL` | raw seed 与由公开 metadata 派生未来图的 context 均已移除；只发送当前 shuffled sample positions |
| `DCF_MEMBERSHIP` | `PASS_FUNCTIONAL` | 实际 uCMP/DCF C++ ABI、stable key、`1−strict_lt(τ,key_i)`、exact-K mask |
| `DURABLE_MATERIAL_LEASE` | `PASS_LOCAL_POSIX_CONTRACT` | one-shot lease/claim-before-read、并发/重启/篡改测试；管理员删除/回滚被排除 |
| `AUTHENTICATED_TRANSPORT` | `PASS_FUNCTIONAL_TLS13_MTLS` | local TLS E2E 与身份/帧 conformance；证书运营未测 |
| `JOINT_VIEW_SECURITY` | `OPEN / NO_GO_FOR_SECURE_CLAIM` | 缺实际 P0/P1 full-view simulator/hybrid；没有把未发现攻击写成通过 |
| `N>256` | `NO_GO` | N=257 streamed preflight 拒绝 |
| `FORMAL_PERFORMANCE_READY` | `NOT_ALLOWED / NOT_RUN` | V3/V4 门未关闭；没有运行 LAN/WAN 矩阵 |

## 3. Sample seed、公开 L 与自适应控制

### 3.1 S5 泄露问题与 S6 修复

S5 的 `public_sample_seed` 由 T 的 `getrandom` 生成，写入 M6B5/v1 party package 和 binding，P0/P1 收到后用来决定采样集合与后续图。它与输入无关，但用户此前确认的 L 只列出端点/pivots、slot、比较 bit、次数/长度、masked operands 和统一 ABORT；raw seed 并未单独得到公开许可。因此 seed 一到 party 手中，完整未来随机 tape 就被提前公开。

S6 最初尝试把 seed 删除后，用公开 session/fingerprint/material ID 派生 deterministic context。复核发现 parties 仍能从这些公开字段提前推算未来图，等价于 seed 泄露；该实现未作为最终版本接受。最终 S6 移除派生 context：P0 在每个实际活动的 reduction iteration 当场从 `getrandom()` 取得 bounded random words，按拒绝采样 Fisher–Yates 从当前 candidate 无放回取样，再经独立 `sample_schedule[iteration]` endpoint 和 phase `40+iteration` 发送当前样本位置。P1 收到后，在控制器校验样本长度、唯一性及其属于本轮 candidate，再产生当前样本 clique；下一轮尚未生成的 sample 不可从 session metadata 推导。

P0 与 P1 在半诚实模型中按此流程执行；P0 是随机样本的发送方，消息不发送 RNG state/seed。运行时 E2E JSON 只存公开 binding IDs、case 及计数，不存 sample seed。当前样本位置属于 shuffled-coordinate endpoint 公开内容；消息长度和发布轮数属于已确认可见的通信长度/计数。没有公开 score、threshold、original index→shuffle position 映射或明文 mask。该 mapping 说明 transcript 字段没有超出已确认 L，不是 joint-view 模拟证明。

### 3.2 L 字段/视图表

| 字段/视图 | 产生方→接收方 | 坐标/保护 | 时机与可推导性 |
|---|---|---|---|
| session、fingerprint、material ID、N/K/width、protocol version/party/direction | T/package publisher/P0/P1 binding | 参数和身份；不含 score | binding 阶段公开，绑定每个 frame/材料；S6 schedule 不由这些字段派生 |
| 当前 iteration 的 sample positions | P0→P1 | shuffled coordinates；TLS 应用 payload 对 peer 可见，网络外部加密 | 当前轮开始时发送；等价 sample-clique 顶点/端点。下一轮 sample 未生成 |
| sample clique / pivot-star / exact-clique endpoints | P0/P1 由当前状态导出 | shuffled coordinates | 每层执行时冻结；sample positions、先前 opened bits 和算法公开状态决定边集。pivot 位置可由 sample order 和 bits 导出，不发送 rank/pivot 值 |
| DCF slot ID/address（purpose、iteration/substage、direction、endpoint pair） | party-local reader；对方可从公开图推导 | shuffled coordinates；密钥内容不可见给对方 | 访问时公开/可推导；每个 key lease 一次消费，不跨迭代复用 |
| `z_left/z_right` 及 score conversion / membership masked operands | P0↔P1 | `b`-bit ring 中 masked operands；TLS 外部加密、协议对方可见 | 各 uCMP 操作交换时；不是 unmasked score/key/tau |
| strict comparison bit | 双方由 DCF output share 求和 | 每 bit 为 0/1，协议公开 | comparator result phase；sample order、pivot rank、candidate/residual、后续 edge 和 slot order 可由 transcript 确定性更新 |
| 比较次数、sample exchange 次数、通信长度、active compare layers | 双方/传输观察者可见 | 计数/长度 | 由 frame 数和 payload/framing 长度观察；已在 L 中 |
| 最终 XOR mask shares | P0/P1 各自输出 | 每方本地 share；只有 TEST_ONLY controller 重构 | final success barrier 后；ABORT 时无 party API mask 返回 |
| ABORT、断连、timeout、重启状态 | P0/P1；外部可见 timing/长度 | 统一协议状态，TLS 对网络内容加密 | 出错阶段。不同本地错误日志/耗时不是协议报文，但仍需在真实部署 side-channel 论证中覆盖 |
| T 离线视图 | T | 知道 `Pi` 和两方 preprocessing；不看 score/rank/online threshold/output | T 生成材料期间；T 完成后退出。T+party 串通不在模型中 |

端点/bit transcript 足以让两方按同一公开排序键和 residual-rank 规则更新控制状态，不需要额外发送 secret rank mux、threshold 或原始位置。相同 n/K、不同输入的本轮对照由 final E2E 中固定输入夹具的不同 binding IDs、随机样本和 oracle 结果构成；case JSON 对外仅呈现获准的 endpoint/长度派生计数，不记录输入值或阈值。该对照是 transcript 投影检查，**不是**密码 party-view simulator。

**Seed 泄露决策已确认**：用户在 S6 明确要求原始 `public_sample_seed` 保持不公开，尤其不在任何对应 endpoint/pivot/比较 bit 按协议时序公开之前发送给 P0/P1。S6 的 seed 不进入 package、binding、party 消息或 E2E case JSON；当前 L 不包含原始 seed，也没有待用户确认的 L 扩展项。若未来提出 seed 公开，必须另行版本化材料/协议合同并重新审查提前推导未来图的影响。

## 4. 样本分布、稳定语义与比较成本

### 4.1 sampler 关系和正确性

实际 runtime 的 P0 每次 bounded draw 从 OS `getrandom()` 得到 64-bit word；`random_bounded(bound)` 拒绝低于 `(-bound mod bound)` 的区域后取 `%bound`，再用 Fisher–Yates 对当前 candidate 做无放回抽样。若把 OS CSPRNG 输出视为理想独立均匀 64-bit word，该过程给出均匀子集；这是假设，不是 S2 定理对运行时 CSPRNG/系统实现的形式证明。P1 不抽另一份样本，而是验证和消费 P0 当前轮发送的 endpoint 集。

算法精确性不依赖样本命中概率：每次更新都由完整样本 clique 与两组 pivot-star 比较恢复 pivot 在 candidate 的精确 rank，并维护“目标仍在 candidate、residual 是其 candidate 内一基 rank”的不变量；若前四轮没有选中目标，第五轮对剩余 candidate 的 complete clique 恢复严格全序并精确选择 residual 第 K 项。因此任意**合法**随机样本 tape 都输出冻结稳定第 K 项。抽样只影响比较工作。输入无效、抽样接口返回非法集合、材料/DCF/transport 错误、timeout 或资源中止为统一失败且 no-mask；不将错误答案当作概率失败，也不靠重试掩盖。

项目负责人暂缓了数值化随机失败率要求。S2 的有限矩阵期望比较量结论仅在其理想 fresh uniform-without-replacement 模型下成立；S6 不把它转写成 concrete `getrandom()` runtime 的期望/高概率成本保证。当前实现最坏比较调用仍为 `12N+C(N,2)`，O(N²)。

### 4.2 E2E 观察到的比较调用

下面是 S6 final binary 的固定功能矩阵观测，不是用时、网络性能、期望值或 tail bound。全部 18 个成功 case 逐位匹配冻结稳定 oracle；machine records 在 [S6 E2E JSON](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S6_E2E_CASES_2026-10-14.json)。

| logical/padded N | 成功 case 数 | K 覆盖 | 比较调用 min/median/max | 最坏界 |
|---:|---:|---|---:|---:|
| 128/128 | 7 | 1、2、8、64、65、128、重复 64 | 299 / 415 / 3,942 | 9,664 |
| 256/256 | 8 | 1、2、8、64、128、255、256、重复 128 | 618 / 692.5 / 20,650 | 35,712 |

N=256 per-K 观测：

| K | comparisons | `L` 比较层 | `A` sample exchange | 完整 exchanges `9+2L+A` |
|---:|---:|---:|---:|---:|
| 1 | 618 | 6 | 3 | 24 |
| 2 | 660 | 8 | 4 | 29 |
| 8 | 723 | 9 | 4 | 31 |
| 64 | 20,650 | 9 | 4 | 31 |
| 128 | 13,953 | 9 | 4 | 31 |
| 255 | 662 | 6 | 3 | 24 |
| 256 | 629 | 6 | 3 | 24 |
| 128（重复运行） | 6,503 | 9 | 4 | 31 |

比较调用是一条有序调用，对应一个逻辑无序端点关系和一对 64-bit masked operands；每个 party 对实际 DCF 调用两次 `evalDCF`，双方总计四次。跨迭代重复 endpoint 对使用不同 iteration-bound slot，故不会重用 key；S6 runtime metrics 不计算跨迭代 unique-edge union，记 `NOT_MEASURED`。sample clique 和同轮 pivot stars 端点集合不相交；比较调用/slot count 可从 CASE JSON 与图定义重算。

## 5. 实际 DCF pool、材料 ABI 与容量

### 5.1 ABI/密钥绑定

S6 对象是 S5 的 `protocol_i_project_derived_select5`、`protocol_i_ucmp`、`keyGenDCF/evalDCF`、parallel shuffle 和 `M6S2` material slot store。对每个比较槽，T 用 `getrandom()` 产生环内 `m_left,m_right`，将每个 mask 分成两方 additive shares，并按实际代码计算 `alpha=(m_left−m_right) mod 2^b`，生成 `keyGenDCF(b,64,alpha,1)` 双方 party-local material。每方比较槽绑定 session/material binding、purpose、iteration、substage scope、方向、端点。在线各方发送 `z_left=(x_left_share+m_left_share) mod 2^b`、`z_right=(x_right_share+m_right_share) mod 2^b`；接收后共同可见的两个 z 是加 mask 后的 ring words。`ProtocolIUcmpPartyMaterial::eval_strict_lt` 计算 `x=(z_left−z_right) mod 2^b`、`y=(x−2^(b−1)) mod 2^b`，每方对 `x` 和 `y` 各执行一次 `evalDCF`，交换 local result share 并打开严格比较 bit。membership 以同 ABI 比较 `τ` 与 `key_i`，稳定 key 唯一时输出 `1−strict_lt(τ,key_i)`；padding 受稳定顺序约束。S6 uCMP C++ conformance、membership E2E 均通过；这证明 ABI/方向/功能，不证明 key 的单方视图可模拟。`getrandom()` 的输出分布和 DCF security 都仍是实现/密码假设，本文不把边际 masked z 均匀性升级成联合安全结论。

更精确地，对环 `R=Z/(2^b)`，每个 full mask `m` 由 `getrandom()` 取字并掩成 `b` 位；`share_mask(m)` 取独立随机 `s_0←R`，置 `s_1=m−s_0 (mod 2^b)`。T 把 `s_0,s_1` 分别写入 P0/P1 槽，并为 `(m_left,m_right)` 生成阈值 `alpha=m_left−m_right (mod 2^b)` 的 DCF key pair。设比较双方已有的输入加法份额为 `x_{L,0}+x_{L,1}=x_L`、`x_{R,0}+x_{R,1}=x_R`。party `j` 发出的两个局部 payload word 是 `z_{L,j}=x_{L,j}+s_{L,j}`、`z_{R,j}=x_{R,j}+s_{R,j}`（均在 `R` 中）；双方相加打开 `z_L=x_L+m_left`、`z_R=x_R+m_right`。代码构造 `d=z_L−z_R=(x_L−x_R)+alpha`，并在 `d` 与 `d−2^(b−1)` 上运行两次 Eval，以按 ABI 还原 signed strict comparison。对每个槽这是精确的代数关联，不是“独立均匀字段”：T 同时持有 full masks 和 key pair，单方持有各自 mask shares/key share，双方看到的 z 与阈值及打开 bit 在同一 transcript 中相关。若假设每个 `getrandom()` word 独立均匀，则单个 share 的理想分布为均匀 `R`，但该结论本身不覆盖 DCF correction words、全池 key、adaptive 地址访问或 shuffle/output 的联合视图；S6 未证明这些视图可模拟。

### 5.2 N=128 固定包与 N=256 stream

| padded N | comparison bits | Select `5C(N,2)` | 全流式 slot `5C+13N`/party | storage result | support |
|---:|---:|---:|---:|---:|---|
| 128 | 40 | 40,640 | 42,304 | M6B5/v2 exact serialized package `60,008,572 B/party` | E2E PASS |
| 256 | 41 | 163,200 | 166,528 | `185,925,312 B/party` staged data+manifest；11 个 ≤16 MiB segments/party；双方 `371,850,624 B` | streamed E2E PASS |
| 257 | — | — | — | preflight rejects before staging | NO-GO |

N=256 每方流式 payload 由 score input 512 slots、Select 163,200、membership 256、shuffle metadata 2,560 组成，合计 166,528 槽。source/layout 公式得数据 `173,269,120 B/party`；manifest `64+76×166,528=12,656,192 B/party`。E2E parent 在 T staged 后观察到 per-party `185,925,312 B`，这是**本地 POSIX 文件观察**（data+manifest），不含 ready/lease、不是峰值 RSS 或 T→party network delivery；没有把 N=128 成功外推给 N=256。固定 monolithic N=256 package preflight 按已知字段/64 MiB cap 在生成前拒绝，N=257 stream 也拒绝。N>256 记 NO-GO。

最大比较层按 padded N=256 complete clique `C(256,2)=32,640` 计算，当前 payload 为 `16×32,640=522,240 B` masked operands 与 `8×32,640=261,120 B` result words，低于 1 MiB 单帧 payload 上限；sample clique/pivot stars 更小。因此 N=256 本次比较帧不需要 chunk splitting。它不是 T 到 parties 的离线材料传输测量。实际磁盘峰值含 OS/lease/ready、allocator/RSS 峰值、网络传输 bytes、N=257 完整生成成本均 `NOT_MEASURED`。

Segment manifest 和每槽 digest/绑定、ready 和 per-slot durable lease 一次消费。claim 在读出材料前经 `O_EXCL`/fsync burn；领取后崩溃需放弃该 material ID，不复用旧槽。POSIX durability 假设目录 owner/mode 与 fsync 语义可靠；管理员删除/回滚整个 store 或旧快照排除在合同外。ready API 不自行证明 caller 就是 supervisor、也不验证 T 已退出；E2E supervisor 确实 wait 到 T exit code 0 后才发布。生产部署必须保留该编排且隔离 P0/P1 的目录和宿主身份。

TLS 1.3 mTLS 应用帧 v2 绑定 session/fingerprint/material ID/N/K/width/party direction/type/phase/sequence；runtime 无 plaintext fallback。错误 peer identity、篡改 package、错 party K、ready 未发布、断连等 E2E 失败均无 mask。证书签发/轮换/撤销及真实网络路径未测。E2E 三方为本机进程/socketpair，同 OS identity 能访问对方本地测试文件；它不是独立宿主隔离证明。

## 6. 消息因果 DAG 和轮数

| 子路径 | 在线 request-response exchange |
|---|---:|
| package/binding | 1 |
| raw signed Q20.12 → stable packed key 输入适配 | 2 |
| forward shuffle | 2 |
| 当前样本位置发布 | `A`，每个实际活动 sampled iteration 1 次，`A≤4` |
| Select comparator | `2L`，每个非空冻结比较层先 exchange masked operands 再 exchange local DCF result shares/打开 bit，`L≤9` |
| threshold membership | 1 |
| inverse shuffle/原序 routing | 2 |
| final status/mask barrier | 1 |
| **完整 raw-score → original-order mask** | **`9 + 2L + A`，最多 31** |

消息因果：`binding → score conversion(2) → forward shuffle(2) → [sample schedule(A) → current comparator operand/result(2 per layer), comparator bits update public C/r → next sample only after previous update] → stable τ membership(1) → inverse route(2) → joint success status(1) → local XOR-share output`。控制器的第五层 exact clique 仅在前四轮均未确定目标时运行。Algorithm iteration=5、plaintext comparator dependency layers≤9、current online exchange count、CA formula 与 Protocol I paper core 不能互换。

若一个特定 CA compiler 的适用假设单独证明成立，比较层 `k=9` 可以代入通用 `2k+1=19` **比较核心条件值**。S6 没有实现 CA 编译，且当前 sample schedule exchange `A` 未由该条件式消除；不将完整协议报成 19 轮。Protocol I 文献的三轮 paper core 也是不同功能边界。

## 7. 半诚实单方联合视图安全审查

信任模型：每次只腐化 P0 或 P1；T 是 trusted、离线且不合谋；Party 间用认证 TLS 1.3 通道；T 生成时知道 `Pi` 和双方材料，之后退出，不看到 score/rank/online threshold/output。T+party 合谋、恶意方对采样/消息偏离、主机入侵不在该模型。

复核的视图字段：本方输入 shares；本方完整 package 或 streamed manifest/segments 的 used 与 unused DCF/uCMP keys、mask shares 和 shuffle state；公开 masked `z` operands 与 opened bits；由先前 bits 决定的 candidate/pivot/endpoints/地址访问顺序；forward shuffle 和 inverse routing local state；最终 XOR mask share；ABORT、断连、timeout、restart。N≤128 monolithic shuffle package 保留 M2CP 当前双轮路径不消费的全对比 DCF pool；N=256 streamed 路径用 metadata-only 两轮 shuffle layout，没有生成该未用比较 key 池。证明需绑定到准确 schema，不可混并两种 pool。

**未闭合点是具体的组合分布，而非笼统“需要 simulator”：**每个 DCF key 的 threshold/correction words 与 alpha=`mask_left−mask_right`、local mask shares、party 可见的 z operands、opened comparator bit 相关。之后 opened bits 决定下一轮 candidate、sample graph 和材料地址；P0/P1 同时持有全池或 manifest 中其余 unused slot。要模拟的对象还要串起 shuffle 的 `sigma/tau`、masked records、输出 inverse route 和 abort/restart。S6 没有对实际 `keyGenDCF/evalDCF` key generation 构造 P0/P1 simulator，也没有逐 hybrid 假设消掉此相关/自适应访问；“单个 z 边际均匀”或“端点已获准公开”不能封闭它。未发现可复现 distinguisher，不等于通过安全审查。BGI15 不同 key 定理、BMW16/S31/S32 条件证明均未继承。

故 `JOINT_VIEW_SECURITY=OPEN / NO_GO_FOR_SECURE_CLAIM`；代码入口只保留 opt-in EXPERIMENTAL，没有创建 secure alias。若要提升，应提交以准确 DCF key ABI、M6B5/v2/M6S2 两种 party package、实际 L、T trust boundary 和 abort model 为对象的证明，并在跨主机 party-storage 隔离下接收；功能 E2E 不是安全门替代品。

## 8. Release 构建、命令与回归

S6 最终 Release 环境：Ubuntu 24.04 WSL2 x86_64、CMake 3.28.3、GCC 13.3.0、OpenSSL 3.0.13、`BUILD_TESTING=ON`、`MOE_TOPK_ENABLE_EMP_OT=OFF`、`MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=ON`。Build directory 为 `/tmp/m6b-select5-s6-build`，不在 repo。

```bash
cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-project-derived-5iter-s6/VFSS \
  -B /tmp/m6b-select5-s6-build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_EMP_OT=OFF \
  -DMOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=ON

cmake --build /tmp/m6b-select5-s6-build --target \
  moe_topk_project_derived_select5_conformance_test \
  moe_topk_project_derived_select5_material_store_test \
  moe_topk_streamed_material_slot_store_test \
  moe_topk_tls13_transport_conformance_test \
  moe_topk_project_derived_select5_process_e2e_test \
  moe_topk_m2_ucmp_conformance_test \
  moe_topk_m2_parallel_shuffle_conformance_test -j2

/tmp/m6b-select5-s6-build/moe_topk_project_derived_select5_conformance_test
/tmp/m6b-select5-s6-build/moe_topk_project_derived_select5_material_store_test
/tmp/m6b-select5-s6-build/moe_topk_streamed_material_slot_store_test
/tmp/m6b-select5-s6-build/moe_topk_tls13_transport_conformance_test
/tmp/m6b-select5-s6-build/moe_topk_project_derived_select5_process_e2e_test
/tmp/m6b-select5-s6-build/moe_topk_m2_ucmp_conformance_test
/tmp/m6b-select5-s6-build/moe_topk_m2_parallel_shuffle_conformance_test
```

All seven targets built. Controller, package lease, streamed slot store, TLS, uCMP/DCF, shuffle conformance each ran and exited 0; the full final T/P0/P1 E2E exited 0. A separate Release configure with `MOE_TOPK_ENABLE_PROJECT_DERIVED_SELECT5_EXPERIMENTAL=OFF` confirmed Select5, streamed slot store and TLS conformance targets absent from default target help (`DEFAULT_OFF_TARGETS_ABSENT=PASS`). Final binary hashes and external logs are in the [S6 source manifest](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S6_SOURCE_MANIFEST_2026-10-14.json); per-case runtime evidence is in the [case JSON](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S6_E2E_CASES_2026-10-14.json).

### 8.1 保留的失败尝试和修复

- 首次接收 S5 时的 PowerShell/WSL wrapper 因 CRLF 把最后的 exit 参数解析失败，wrapper exit=1；七个底层程序实际均 exit=0，改为 LF wrapper 后完整接收命令 exit=0。首次记录保留于独立证据目录。
- 首次 S6 配置误用 Ubuntu 20.04 的系统 CMake 3.16（低于仓库 minimum），另一次 distro 名称错误；改用已安装 CMake 3.28.3 的 Ubuntu 24.04 完成 Release configure/build。
- 首个 N=256 E2E 暴露 runtime 已改上限、Select controller 仍限 N≤128 的接口不一致；S6 同步 controller 上限为 256 并修复失败时重复 abort 路径，之后 N=256 全矩阵通过。旧失败日志没有覆盖。
- 旧“tamper package seed”用例实际修改了 M6B5 包 offset 32 的逻辑 N，使 T staging 在 parties 启动前拒绝，并没有命中所声称的 party 阶段；S6 换成显式 wrong-party-K no-mask case，不把原失败改写为协议覆盖。
- 即时 sample schedule 接线后的首个完整运行被 E2E harness 的计数断言挡下：P0 的 schedule-sent bytes 与 P1 schedule-received bytes 是对应方向，不应要求两方相同位置相等。断言改为交叉核对 send/peer-receive 并分别比较公共工作计数后，完整 E2E 两次复跑 exit=0。
- 另一次证据捕获脚本受 PowerShell→WSL 变量展开影响，没有执行目标；随后改为逐个可执行文件直接调用，六个 conformance 均 exit=0。以上 wrapper/断言问题都保留在外部尝试日志；未以修改 oracle 期望、丢弃样本或重试算法 case 隐藏失败。

## 9. 性能、外推和历史门界

本阶段没有启动 LAN/WAN、server matrix、warmup/repetition、统一性能指标或正式比较报告。E2E 里的 bytes 是 local TLS frame/测试存储观察，不能写入正式性能结果；CPU/RSS、完整 T→party network package bytes、segment fsync 的 production storage 行为、跨宿主延迟和证书运维都为 `NOT_MEASURED`。

S34 的诊断范围和未异会话独立接收状态保持原样；S6 的非参与者 S5 receipt 不补签 S34。M6A V3 保持 `OPEN`，V4 和 LAN/WAN 为 `NOT_ALLOWED/NOT_RUN`。S1 BB90 来源保持 `NO_GO_FOR_IMPLEMENTATION`；现有 BMW16-derived Select 是另一独立路线；S6 不把 PROJECT_DERIVED 纳入六方案 BB90 行。

## 10. 文件修改范围

S6 相对冻结 S5 源快照的 runtime/test delta：

- `VFSS/include/moe_topk/project_derived_select5.h`
- `VFSS/include/moe_topk/protocol_i_material_slot_store.h`
- `VFSS/include/moe_topk/protocol_i_parallel_shuffle.h`
- `VFSS/include/moe_topk/protocol_i_project_derived_select5.h`
- `VFSS/src/moe_topk/project_derived_select5.cpp`
- `VFSS/src/moe_topk/protocol_i_material_slot_store.cpp`
- `VFSS/src/moe_topk/protocol_i_parallel_shuffle.cpp`
- `VFSS/src/moe_topk/protocol_i_project_derived_select5.cpp`
- `VFSS/tests/moe_topk/protocol_i_project_derived_select5_three_process_e2e_test.cpp`

S6 同步的当前治理文档为 `PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md` 和 `docs/REFERENCE_MANIFEST.md`。本报告同目录另交付 S6 decision、逐案 E2E JSON、source manifest 和 input-freeze JSON。其余 S1–S5 reports/decisions/tests 在 S6 worktree 中是经 manifest 选择性承接的历史输入，不是 S6 修改；S5 对 `VFSS/CMakeLists.txt`、score-input/TLS transport 的变更也只是继承。未修改 `VFSS-baseline/`、论文、本地参考工程、密钥、证书、package/sidecar、build output 或原始运行日志。
