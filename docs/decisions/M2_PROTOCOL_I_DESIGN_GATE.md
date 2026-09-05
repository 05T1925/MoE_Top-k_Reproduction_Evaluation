# M2 Protocol I 实施前设计门

状态：**待团队批准；本文件不是 M2 代码授权。**

M2.1 对 D1--D4 的可实现性、精确泄露条款、域宽表、inverse-routing 证据缺口和
transport/metrics contract 见
[`M2_PROTOCOL_I_FEASIBILITY_AUDIT.md`](M2_PROTOCOL_I_FEASIBILITY_AUDIT.md)。该审计的
结论仍是 `m2_protocol_i_design_blocked`；链接不构成任何实现授权。

M2.2 对 priority-rank 语义和 reverse-shuffle 规格的纠错审计见
[`M2_PROTOCOL_I_PRIORITY_RANK_CORRECTION.md`](M2_PROTOCOL_I_PRIORITY_RANK_CORRECTION.md)。
M2.3 的 reverse Permute+Share ideal-functionality 候选见
[`M2_PROTOCOL_I_REVERSE_SHUFFLE_SPEC.md`](M2_PROTOCOL_I_REVERSE_SHUFFLE_SPEC.md)。

实现候选标签为 `agarwal_protocol_i_exact_mask_output`。在本文件的四项决策均获明确
批准、其最小原语经审计且 secure mask adapter 可验证之前，任何实现只能使用
`m2_protocol_i_design_blocked`，不得声称 Protocol I exact。

## 1. 入口、证据纪律与不变量

M1.1 已完成并合入 `main`：Ubuntu 24.04.4 的干净 Debug 构建中，四个 M1 CTest 为
4/4 通过。它只闭合 CTest/provenance 门；LAN/WAN、性能、网络 RTT、带宽、协议通信
均为 `NOT_MEASURED`。2026-09-04 的 DPF conformance 仅是 M3 前置原语证据，不是
M3 实现或完成证据。

本文件使用以下证据标签，且不相互替代：

| 标签 | 含义 |
| --- | --- |
| A | 15 页 Agarwal CCS 2024 会议论文明确规定。它是唯一论文基线；不假定 full version。 |
| B | 本地 `Agarwal_TopK/` 或 VFSS 的实测/静态代码行为；不是论文结论。 |
| C | 为满足仓库 Q20.12、原顺序 XOR mask、审计与计量契约提出的项目工程扩展。 |
| D | 尚无可审计原语、端到端证明或实测支持的假设；它是阻塞项而非实现许可。 |

固定不变量（C，除非另有 A 引用）：输入为 Q20.12 signed 32-bit 算术 shares；最大
score 优先、相等时较小 original index 优先；输出只能是原始顺序、长度 `n` 的 XOR
Top-K bit-mask share；test 重构与 secure 接口隔离。P2 是输入无关离线 Dealer，在线
绝对静默；不得使用旧 ABI、文件轮询、共享目录、固定 sleep、在线 Dealer、B0 矩阵
shuffle 或 `MockShuffle`。

M2 可以进行本设计冻结；M2 代码必须等待本决策记录及四个阻塞决策获得团队批准。

## 2. 精确 Protocol I 阶段表

论文 A：Table 1 将 Protocol I 定义为 2+1、Shuffle routing 加 `binom(n,2)`-CmpAgg
ranking，合计 3 个 online rounds；§4.1 说明先 shuffle 后可公开 shuffled-domain 的
stable rank 并在清晰域 routing。论文只给该总轮数和高层构造，未给可迁移的逐消息
shuffle transcript，故下表不虚构论文逐消息轮次。

论文原始分数升序 stable rank `rank_A=0` 是最小 score；项目 Top-K priority rank
`rank_P=0` 是最大 score，二者同分时均按原始 index 升序。它们不是全局互补：三个相同
分数、原始 index 为 `(0,1,2)` 时，二者都为 `(0,1,2)`，而 `n-1-rank_A` 为 `(2,1,0)`。
因此不得使用 `rank_P = n-1-rank_A` 或任何等价映射。M2 的候选是以 priority key 的
升序稳定 CmpAgg rank 直接定义 `rank_P`，再以 `rank_P<K` 选择；这仍是 C/D 设计门，
不是已获批接口或论文结论。

| 阶段 | 输入/输出与角色 | 原语、发送与打开值 | 禁止打开/轮次/泄露 | 位置与证据 |
| --- | --- | --- | --- | --- |
| O0 离线包 | P2 对 P0/P1；输入未知。输出仅为各方的 input-independent correlated randomness。 | 候选：经批准的两方 secret-shared shuffle、DCF/uCMP keys、掩码及 adapter 预处理，P2→P0/P1。 | P2 在线不连接、不收输入、不补发材料；不计 online round。离线材料大小必须计量。 | A：2+1 Dealer 模型、Table 1；C：VFSS 重新生成/传输；D：具体包布局未批准。 |
| R1 shuffle | P0/P1 的 score 与必要 secret original-index binding shares → 同一随机秘密置换下的 record shares，及论文 §4.1 允许的公开 masked shuffled list。 | A：secret-shared shuffle；C：record binding。selection carrier 在 R3 后才于 shuffled domain 创建，不存在于 R1。P0↔P1 消息由批准 shuffle primitive 决定。 | 不开 permutation、原始 index、原顺序 rank、selected index 或 mask。是否把该 primitive 的在线交互折入论文总 3 轮，尚无会议版逐消息证据。 | A：§4.1、[21]；B：B1 是两次 Permute+Share 方向；C/D：M2.3 reverse candidate、VFSS 实例缺失。 |
| R2 CmpAgg | shuffled score/key shares → shuffled-domain stable rank shares。 | `binom(n,2)` DCF/uCMP evaluations；两方按 M1 已冻结比较语义交换并打开该 FSS gate 所需 masked values。 | 不开 score、comparison bit、original index 或 rank shares。M1 CmpAgg 仅是测试/明文 oracle 证据，不是 secure M2 runtime。 | A：§3.1、§4.1；C：priority-rank mapping；D：uCMP range proof 未完成。 |
| R3 controlled rank reveal + clear-domain selection | shuffled rank shares → public shuffled-domain ranks；选中 record carrier 仍为 shares。 | P0↔P1 仅在批准泄露契约下重构 `rank_P`；按公开 rank 在 shuffled domain 路由。 | 仅可向 P0/P1 公开 `(shuffled position, priority rank)`；不公开 permutation、original mapping、scores、selected/original index、最终 mask。论文表总计 3 rounds；本阶段与 R1/R2 的精确消息归属待 primitive 审计。 | A：§4.1 的 shuffle-then-reveal；C：精确定义泄露；D：未获团队批准。 |
| R4 inverse routing + mask adapter | selected shuffled carriers → original-order arithmetic mask shares → XOR mask shares。 | 仅可用批准的 inverse secret-shared shuffle / secure scatter；算术到 XOR 转换必须明确并计入主路径。 | 不开 selected index、original index、mask、rank 或 oracle。若需要额外因果消息，单列 `mask_adapter_rounds`，不得伪称论文的 3 轮。 | C：统一输出契约；D：当前无已审计 VFSS primitive。 |

因此，论文的“3 rounds”只能报告为 A 的 paper-core 声明；项目实际总轮数应为经实测
的 `paper_core_rounds + mask_adapter_rounds`，并保留因果审计。没有 R4 安全方案时不能用
`agarwal_protocol_i_exact_mask_output`。

## 3. 四项必须批准的决策

### D1：shuffled-domain rank 泄露

| 项目 | 决策内容 |
| --- | --- |
| 可选方案 | (a) 受控公开 shuffled-domain priority rank；(b) 不公开 rank，改走尚未设计的私有 routing。 |
| 泄露/安全 | A 支持 pre-shuffle 后 reveal rank 的高层范式。方案 (a) 只允许 P0/P1 看到 `(shuffled slot, rank_P)`；随机 permutation 对任一单方未知。它不允许公开 original index、selected index、原顺序 rank 或最终 mask。方案 (b) 不再是论文 §4.1 所述 shuffle routing。 |
| 正确性 | 必须由 priority-key stable rank 的排列性证明每个公开 shuffled slot 恰有一个 `rank_P`；tie 使用绑定的秘密 original index。 |
| 轮次/指标 | 记录该重构所在因果轮；公开 rank 后的本地 shuffled-domain routing 不另增交互。所有发送计入 online sent_bits。 |
| 标签 | 方案 (a) 在其泄露契约获批准且其他门通过后仍可使用 exact 候选标签；方案 (b) 不可称 shuffle-based Protocol I exact。 |
| 推荐与缺证 | **推荐 (a)，但当前未批准，故阻塞。**缺失：团队对该精确泄露、受众和 transcript 保留规则的批准，以及 shuffle 对单方 permutation hiding 的端到端论证。 |

### D2：原顺序 XOR mask 的 inverse-routing

| 项目 | 决策内容 |
| --- | --- |
| 可选方案 | (a) 以同一双方秘密置换的可逆、record-preserving secure scatter 逆路由选择 carrier；(b) 明文后处理/打开 index/B0 矩阵/单方 permutation。 |
| 泄露/安全 | 仅 (a) 符合 C 的统一 secure 输出；(b) 会暴露 mapping、selected index 或依赖禁止实现，全部拒绝。 |
| 正确性 | record 至少绑定 score share、original-index share、selection carrier share。shuffle 与 inverse 路由对三字段使用同一置换；carrier 是 `rank_P < K` 的秘密或由公开 shuffled rank 派生的 share。inverse scatter 输出每个 original slot 的 arithmetic bit share。仅在此后转换为 XOR bit share。 |
| 轮次/指标 | offline：inverse permutation/translation 材料；online：其所有消息与 `mask_adapter_rounds` 单列。shuffle、index binding、inverse mapping、share conversion、mask generation 全部进入主时间/通信，另报 paper core。 |
| 标签 | 只有 (a) 有经审计 primitive、conformance、differential、E2E 后才可使用 `agarwal_protocol_i_exact_mask_output`。 |
| 推荐与缺证 | M2.3 给出以任意置换 Permute+Share ideal functionality 为边界的两次 role-swapped reverse 候选；**仍为 D 阻塞。**现有 B1 helper/artifact/path 不可作为 VFSS 方案，且缺独立组合安全审查、VFSS 内部消息/预处理、conformance 与 E2E。 |

### D3：priority key、公开 `n` 上界与 uCMP/DCF 域宽

| 项目 | 决策内容 |
| --- | --- |
| 可选方案 | (a) 先为一组公开 `n_max`、key width、uCMP range commitment 建立证明与 conformance；(b) 因 `uint64_t` 可容纳而隐式截断。 |
| 泄露/安全 | `n`、`K`、位宽和实现配置可公开；越界必须硬错误。静默截断会改变比较/稳定性，拒绝。 |
| 正确性 | C 的候选（非论文结论）：`ordered_score=raw_score ^ 0x80000000`，`priority_key=((UINT32_MAX-ordered_score)<<index_bits)|original_index`。`index_bits=max(1,ceil(log2(n)))`，要求 `n <= 2^index_bits` 且 `original_index<n`；n=1 仍为 1。未 mask key 的候选宽度为 `key_bits=32+index_bits`，uCMP 候选 ring 是 `N=2^(key_bits+1)`（候选 `Bin=key_bits+1`）；VFSS 对该参数、掩码差和端点尚未 conformance，故仍须证明。 |
| 轮次/指标 | 域宽影响 DCF key/material 与消息 bits，必须进入 offline material、online sent/received、PRG calls；不改变经证明的因果轮数。 |
| 标签 | 没有对每一测试矩阵点（128、256、10^3、10^4、10^5、10^6）适用的 range proof 和 conformance 时不可 exact。 |
| 推荐与缺证 | **推荐 (a)，当前为 D 阻塞。**M1 DCF conformance 仅验证 raw unsigned `x<threshold`，没有证明 32+index_bits priority key 或 uCMP 掩码差范围；当前 VFSS DCF 的可用 `Bin`/端点须以实现 conformance 确认。 |

### D4：在线传输与计量边界

| 项目 | 决策内容 |
| --- | --- |
| 可选方案 | (a) P2/P0/P1 独立进程，P2 离线发包后退出，P0/P1 使用有界、fail-closed 的 VFSS socket binding；(b) 复用当前无界重试/固定 sleep，或文件同步/在线 Dealer。 |
| 泄露/安全 | (a) 不持久化秘密材料、不把 P2 带入 online transcript；(b) 违反仓库约束。静态审计 B 显示 `SocketBuf` 当前有无界 connect retry 与 `sleep(1)`，故不能原样视为 M2 运行时。 |
| 正确性 | 每消息有角色、长度、phase/sequence、失败语义；连接失败、长度/轮次不一致、非法 `n/K` 都硬失败。不得提供明文或单方 fallback。 |
| 轮次/指标 | 每方在 online start 后将 Peer `bytesSent/bytesReceived` 置零，结束读取原始计数；`online_comm_total_bits=sum(P0.sent_bits,P1.sent_bits)*8`，per-party 除以 2。连接建立、P2 离线输送、线程启动不计 causal online round，但其时间/材料须单列，且 fixed sleep 绝不可被计时排除。 |
| 标签 | 仅 (a) 经独立进程审计后才可能 exact；现状不能。 |
| 推荐与缺证 | **推荐 (a)，当前为 D 阻塞。**缺失：最小有界 socket/role binding 的设计批准、真实字节采集点验证、P2 静默 E2E。不得借用 B1 `Channel`（50ms retry）或 VFSS 当前 `SocketBuf` 行为。 |

## 4. 最小实施计划（批准后；本轮不创建代码）

| 未来路径 | 单一职责与现有依赖 | secure/test 边界 | 前置批准 |
| --- | --- | --- | --- |
| `VFSS/include/moe_topk/protocol_i.h` | 仅公开请求、角色、错误与 metrics 边界；复用 `score_semantics.h`、`metrics.h`。 | 不暴露 rank/index/reconstruct API。 | D1-D4 |
| `VFSS/src/moe_topk/protocol_i_transport.cpp` | P2/P0/P1 的最小消息绑定与 Peer counters；复用 VFSS `Peer`/Dealer，不能复制 B1 ABI。 | 无文件、无 sleep、无 fallback。 | D4 |
| `VFSS/src/moe_topk/protocol_i_shuffle.cpp` | 经批准的两次 Permute+Share / inverse scatter 最小适配。 | record fields 同置换；secure 不重构。 | D1、D2 |
| `VFSS/src/moe_topk/protocol_i_rank.cpp` | priority key、uCMP/DCF CmpAgg；复用 VFSS DCF。 | 测试 oracle 不链接 secure runtime。 | D3 |
| `VFSS/tests/moe_topk/protocol_i_*.cpp` | 下节严格顺序的 test-only conformance/differential/E2E。 | 仅 test 二进制可以重构。 | D1-D4 |

这不是“迁移 B1 代码”的计划：B1 只提供 B 级调用次序和失败模式参考。不得预建
M3/DPF routing 通用框架。

## 5. 未来验收矩阵

按顺序，不得跳过：

1. score / priority-key / DCF-uCMP conformance；
2. shuffle record binding、两方独立随机性、payload alignment；
3. CmpAgg 与冻结 M1 oracle differential；
4. secure inverse-routing 与 original-order XOR mask adapter；
5. P2/P0/P1 独立进程 E2E；
6. provenance 与真实 sent/received 计数；
7. 泄露、P2 静默、因果轮数审计。

每层覆盖随机、重复、全相等、负值、`INT32_MIN/MAX`、`K=1/K=n`、非二次幂 `n`；
空输入、`K=0`、`K>n` 和每一种域/消息越界必须硬错误。test 才能重构；secure 不得
重构 rank、comparison bit、original/selected index、最终 mask 或 oracle 数据。

## 6. 指标与标签门

未来每条实测记录须有 `implementation_label`、revision、seed/distribution、runtime、
2+1 topology、compiler/flags、CPU/OS、network、warmup/repetitions、offline time/
material、online time、每方 sent/received、total/per-party communication、online rounds、
PRG calls、comparison edges、total time 与 correctness status。没有实际采集点的字段为
`NOT_MEASURED`，绝不填 0、估算或借用 B1 历史数。

只有 D1 受控泄露、D2 secure inverse routing、D3 宽度/range proof、D4 独立进程传输
与计量均批准并通过上述验收，才允许 `agarwal_protocol_i_exact_mask_output`。否则唯一
准确状态是 `m2_protocol_i_design_blocked`。
