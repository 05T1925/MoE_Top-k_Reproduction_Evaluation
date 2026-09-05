# M2.1 Protocol I 可实现性审计与决策闭合

状态：**`m2_protocol_i_design_blocked`**（2026-09-05）。本记录只把 M2.0 的建议拆成可批准或否决的条款；它不是代码授权，也不声称 Protocol I 已实现或已复现。

本审计的证据标签不可互换：**A** 为 15 页 Agarwal CCS 2024 会议论文的明确内容；**B** 为本地参考或现有 VFSS 的静态行为；**C** 为本项目为 Q20.12、原顺序 XOR mask、可审计计量而增加的工程契约；**D** 为缺失或未验证的证据。会议论文是唯一论文基线；没有假定 full version 或补写其未公开的逐消息细节。

## 1. 已核验的边界

| 项目 | 审计结论 | 证据 |
| --- | --- | --- |
| Paper core | Protocol I 是 `2+1`、安全 shuffle routing、`binom(n,2)` CmpAgg，表中为 3 个 online rounds。论文 §4.1 使用 shuffle-then-reveal：shuffle 后公开 rank，清晰域 routing。 | A，Table 1、§3.1、§4.1 |
| 论文 rank | 论文稳定 rank 是升序：较小元素数；同分时较小原始下标在前。因此最小项为 `rank_A=0`，最大项为 `n-1`。 | A，§3.1/Figure 2 |
| 项目语义 | 项目选择最大 score、同分较小 `original_index`；其 priority rank 记为 `rank_P=0` 最高优先级。它须由 priority key 的 stable ascending rank 直接定义，不能以 `n-1-rank_A` 推导。 | C，`M1_SCORE_SEMANTICS.md`、M2.2 纠错；B，`topk_oracle.h` |
| B1 参考 | B1 是两次 Permute+Share/OPV 方向的本地材料；其 adapter fork helper、写读私有 artifact，并传递路径、端口和 session。它不是可迁移 VFSS primitive。 | B，`protocol1/runtime/src/protocol1_b1_shuffle_adapter.cpp`、`src/b1/real_*` |
| VFSS 现状 | `GroupElement` 为 `uint64_t`；DCF API 接受 `int Bin/Bout`，本审计未发现对 M2 priority-key/uCMP 的端点 conformance。`SocketBuf` 无限 connect retry（`usleep(1000)`）且固定 `sleep(1)`。 | B，`group_element.h`、`dcf.h`/`dcf.cpp`、`comms.cpp` |
| 已通过的测试 | M1 raw 32-bit DCF conformance 只覆盖 VFSS raw unsigned `x < threshold`；M1 oracle/CmpAgg 是明文/测试层语义证据。它们不是 secure Protocol I、priority-key 或 uCMP conformance。 | B/C，M1 tests 与 M1.1 记录 |

下文把论文 paper-core rounds 与项目 mask adapter 分开：未实测的时间、通信、PRG 与轮数一律为 `NOT_MEASURED`，不得由表中设计推算为性能结果。

## 2. D1：受控 shuffled-domain rank 泄露契约

### 拟批准条款（C，基于 A 的高层范式）

在 R3，P0 与 P1 **仅**重构每一个公开 shuffled slot `s in [0,n)` 的 project priority rank `rank_P[s] in [0,n)`；二者在 priority-key CmpAgg 输出的 rank shares 完整就绪后、任何 clear-domain selection 前同时收到同一向量 `(s, rank_P[s])`。它不是单独一方获得的值，也不发给 P2 或日志/监控系统。每个 slot 恰有一个 rank，向量因此泄露 shuffled domain 中按 project priority key 排列的完整严格总序。

项目选择直接以公开 `rank_P[s] < K` 决定该 **shuffled slot** 是否被选择。`rank_P` 是 priority key 的 stable ascending rank；它不可由论文 raw-score ascending `rank_A` 作 `n-1-rank_A` 转换得到，特别是在同分时该映射会反转 index tie-break。该编码是 C 项目扩展，不是论文规定。

严禁打开、发送、持久化或调试打印：合成 permutation 或任一方的 permutation、任何 `original_index`、原顺序 rank、score/raw score/priority key、单边 comparison bit、selected index、最终原顺序 mask，以及能够把 slot 关联回原始位置的表。仅可记录公共 run metadata（revision、`n`、`K`、实现标签、阶段成功/失败、计数）；rank 向量、slot-rank 对、permutation、消息 payload 和密钥均不得进入 transcript。运行结束后只保留该 metadata；内存中的敏感向量按实现语言可行的最短生命周期释放，持久化期限为零。

shuffle 隐藏的是 shuffled slot 与 original position 的映射，而不是 rank 总序本身。此条款的必要前提是两遍独立随机 Permute+Share 的合成 permutation 对 P0、P1 任一单方未知，且没有不受本契约约束的侧信道/转录保存。Chase shuffle 的静态半诚实、secret-shared shuffle 描述可支撑这一类前提（A）；B1 的两次 pass 只是本地调用方向证据（B），尚未在 VFSS 中端到端证明该前提（D）。

若团队拒绝这项泄露，停止 R3/R4 设计，不以“私有 routing demo”替代；准确标签仍为 `m2_protocol_i_design_blocked`，不能称 shuffle-based Protocol I exact。

## 3. D2：inverse-routing / mask adapter 可实现性

### 所需的目标构造（C）

这是**规范化的待审计目标**，不是已可调用的构造。M2.3 已给出 reverse 的 ideal-functionality 候选；每条 record 从输入绑定到结束，须在同一置换下携带：

| 字段 | 共享类型与创建时机 | 允许用途 |
| --- | --- | --- |
| `score` | P0/P1 的 32-bit arithmetic shares；R1 前 | R2 secure CmpAgg；不得打开 |
| `original_index` | P0/P1 arithmetic shares（或经批准的等价秘密 carrier）；R1 前 | record binding 与 inverse correctness；不得打开 |
| `carrier_A` | P0/P1 的 1-bit arithmetic shares；R3 后由 public `rank_P` 产生：P0 取 `1{rank_P<K}`，P1 取 0（或相反的固定公共 convention） | 唯一进入 inverse 的 selection carrier；公开 rank 不使 carrier 的原始位置公开 |
| `mask_X` | P0/P1 原顺序 1-bit arithmetic shares；R4 输出 | 只在 share 形式存在 |
| `mask_B` | P0/P1 原顺序 XOR-bit shares；R4 最终输出 | 统一 Top-K 输出 |

正向 R1 必须以同一个未知合成 permutation `pi` 作用于 score 和必要的 original-index binding；`carrier_A` 在 R1 不存在。R3 后才在 shuffled domain 由公开 `rank_P` 创建 carrier shares，R4 对其执行 `pi^{-1}`，输出原顺序 arithmetic bit shares。carrier 的第 `j` 个原顺序输出只来自原 record `j`；`score`、`original_index` 不因 selection 而打开。精确 reverse 代数、freshness 和边界见 `M2_PROTOCOL_I_REVERSE_SHUFFLE_SPEC.md`。

若得到了 arithmetic bit shares `(a0,a1)`，先验证其承诺为 `a0+a1 mod 2^w=b in {0,1}`。此时 `((a0&1) xor (a1&1))=b`：模 2 的加法正是 XOR，因此不需要相关随机性、通信或额外 adapter round。此结论仅适用于已经由 routing 产生的 0/1 arithmetic shares；它不允许重构，也不适用于一般 `Z_(2^w)` 值，secure path 必须保留 shares。它不解决 D2 inverse-routing 的缺口。

### 缺失的实现性证据（D，阻塞）

现有材料不足以把上面的目标变成可审计的 secure inverse-routing 构造：

| 必需项目 | 需要精确给出 | 现有证据与结论 |
| --- | --- | --- |
| inverse 的性质 | 两次 role-swapped、fresh Permute+Share 调用的 owner/data-owner、输入输出 shares 与局部相加项 | M2.3 已在 arbitrary-permutation ideal functionality 边界给出代数候选（C）；Chase 未给 VFSS 可迁移 reverse transcript，故组合安全与 adapter 仍未获证（D）。 |
| 在线打开值 | 每条消息的 phase、sender/receiver、长度、内容，及唯一允许的 opened value | 无满足目标边界的 VFSS adapter；B1 的 helper/artifact/ports 是禁止依赖。**未知，不能以伪代码填补。** |
| 预处理 | inverse permutation/translation、OT/OPV 或 related correlation 的生成与 P2→P0/P1 发放 | B1 方向包含本地 OT/OPV 类材料（B），但 ABI、artifact 和独立 runtime 不可复用；VFSS 没有审计过的替代物（D）。 |
| arithmetic→XOR | 已有 0/1 arithmetic shares 的逐 share LSB 映射与其 0/1 precondition | 模 2 恒等式足够；无需独立原语、消息或预处理。仍需在未来 conformance 中验证 routing 输出满足该 precondition（C）。 |

因而本审计**没有**获得可审计的 D2 secure inverse-routing 构造。明确拒绝 B0 permutation matrix、单方 permutation、打开 index/mask、明文后处理、文件/共享目录以及 "先做 demo"。未来实现必须先用文件级最小 primitive 边界证明上表四项，再写代码。

若将来 D2 被补齐，`mask_adapter_rounds` 精确定义为 R4（inverse scatter/reverse primitive 加 arithmetic→XOR conversion）中，发生在 R3 rank reveal 之后、因果上不能与 R1/R2/R3 合并的 P0↔P1 交互 barrier 数。它不是论文 Table 1 的 3 rounds；实测总数应分报 `paper_core_rounds` 与 `mask_adapter_rounds`。offline material 包含 P2 到两方的全部 inverse/translation/conversion correlations；online communication 包含 R4 的完整 framed bytes。二者都不得混入测试后处理。

## 4. D3：priority-key、DCF/uCMP 域宽与范围承诺

候选（C，非论文编码）为：

```text
ordered_score = raw_score ^ 0x80000000
priority_key = ((UINT32_MAX - ordered_score) << index_bits) | original_index
index_bits = max(1, ceil(log2(n)))
```

它使较大的 signed score 具有较小 key，tie 时较小 index 具有较小 key；priority key 的 stable ascending rank 直接就是 `rank_P`，无需也不得转换为 `rank_A`。不能仅凭 key 小于 64 位推断正确性。

| n | index_bits | key_bits | 合法 `original_index` | 64-bit 拼接的纸面上界 |
| ---: | ---: | ---: | --- | --- |
| 1 | 1 | 33 | `[0, 0]` | `< 2^33` |
| 128 | 7 | 39 | `[0, 127]` | `< 2^39` |
| 256 | 8 | 40 | `[0, 255]` | `< 2^40` |
| 1,000 | 10 | 42 | `[0, 999]` | `< 2^42` |
| 10,000 | 14 | 46 | `[0, 9,999]` | `< 2^46` |
| 100,000 | 17 | 49 | `[0, 99,999]` | `< 2^49` |
| 1,000,000 | 20 | 52 | `[0, 999,999]` | `< 2^52` |

对于此表，`0 <= UINT32_MAX-ordered_score <= 2^32-1`、`index_bits <= 20`，故左移在 `uint64_t` 中不丢位；`original_index < 2^index_bits`，所以 OR 不重叠且最终 `<2^(32+index_bits) <= 2^52`。实现必须在移位前验证 `1 <= n <= 1,000,000`（当前候选审计上界）、`1 <= K <= n`、`index < n`、`key < 2^key_bits`，并把任何非法 `n/K/index/key` 作为硬错误。`n` 非二次幂时 `[n,2^index_bits)` 是未使用 index 域点：不得生成 record、DCF key 或 carrier；任何对它们的输入/输出均为硬错误，不能静默映射到真实 slot。

这只是**位宽表和 C++ 无溢出前提，不是完整 range proof**。未 mask key 的候选 `key_bits` 要满足 Agarwal uCMP 的 `|x-y|<N/2` 前提，故候选比较环为 `N=2^(key_bits+1)`，候选 DCF/uCMP `Bin=key_bits+1`（本表为 34、40、41、43、47、50、53）。现有 VFSS DCF 的签名是 `keyGenDCF(int Bin, int Bout, ...)`/`evalDCF(...)`（B）；`GroupElement` 为 `uint64_t`（B），但未见对这些候选参数、阈值端点、masked difference、payload/ring 宽度或非法 bit-width 的 conformance。B1 `eval_ucmp_gt` 的本地实现不能代替 VFSS 证明。必须明确比较方向为“较小 priority key 胜出”，并证明 mask difference、阈值、`Bin`、shift 不触及未定义行为。上述前提尚未被证明或在 VFSS conformance 中验证（D）。

结论：M1 raw 32-bit DCF conformance 已通过；priority-key DCF/uCMP conformance **未验证**；纸面 `key_bits <= 52` **不等于**当前 VFSS 可安全调用。D3 仍阻塞，直到有完整 range proof、所有表点及端点/unused-domain/invalid-input conformance，并审计比较方向。

## 5. D4：独立进程 transport 与真实计量契约

静态审计表明 `KeyBuf` 维护 `bytesSent/bytesReceived`，`Peer` 能读/清零它们；但原始 `SocketBuf` 用两个 sockets，客户端 connect loop 无上限并 `usleep(1000)`，连接后固定 `sleep(1)`，`read/write` 依赖 `MSG_WAITALL`/`always_assert`。`waitForPeer` 的 accept 也无 timeout（B，`comms.h`/`comms.cpp`）。因此原样保留无界 retry 或 sleep 时，**不得**进入 M2 secure runtime。`Dealer`/`Peer` 提供 key-buffer 传输能力，但没有 M2 的 phase/sequence/EOF/timeout contract（B）。

### 最小 M2 transport contract（C；尚待批准和实现）

每条 P0↔P1 online message 必须带固定编码 header：protocol/version、session id、sender、receiver、phase、monotonic sequence、公开 `n/K/key_bits` digest、payload length、message type；先验证 header 与长度上限，再读 payload。EOF、连接/accept/read/write timeout、短读、重复/跳序、角色/phase/digest/长度不符、非法公开参数均立即关闭连接、清除未完成输出并以可辨识错误退出；不得 retry、sleep、重连补发、部分输出或明文 fallback。

| 角色/阶段 | 因果消息和计量归属 |
| --- | --- |
| P2 offline | 仅在输入前向 P0/P1 发经批准的 input-independent packages；每份材料字节记入 `offline_material_total_bits`，离线时间另记。发放成功后 P2 关闭连接并退出；online 不连接、不接收、不补发。 |
| P0/P1 online start | 建立/验证一次会话，完成公开配置同步；连接建立和启动同步不增加 causal online round，但真实时间单列，不能用 fixed sleep 隐藏。online 计数开始点是同步完成、第一条 R1 protocol message 前，双方将 counters reset 为零。 |
| R1 shuffle | 仅允许经 D1/D2 批准的 shuffle primitive 所定义的 framed messages；其因果 barrier 计入实际 paper-core audit。当前消息集未知（D），不得借 B1 helper 的 ports/artifacts 填充。 |
| R2 CmpAgg | 每个方向的 masked FSS gate/input exchange 均以 phase/sequence 独立框定；仅原语允许的 masked values 可打开。该阶段的依赖 barrier 记入实际 paper-core audit。 |
| R3 rank reveal | P0/P1 交换 rank shares并只重构 D1 指定的 `(slot,rank_P)`；交换属于 R3 的一个因果 barrier。 |
| R4 mask adapter | 仅 D2 获证后出现；其所有消息、打开值和 barriers 分别计入 `mask_adapter_rounds`。当前不得假定消息或轮数。 |
| online end | 完成 XOR mask shares后读取双方 `sent_bits/received_bits`，先保存每方原始值，再导出 `online_comm_total_bits=(P0.sent_bits+P1.sent_bits)*8`、per-party 为 total/2。所有未接通字段保持 `NOT_MEASURED`。 |

offline material、连接建立、启动同步、online messages 和 R4 adapter 均要分边界记录；只有 P0/P1 online sent bytes 进入主 `online_comm_total_bits`。M1 `MetricsRecord` 的 `derive_metrics` 从 `party_communication.sent_bits` 求和（B），故 M2 需要在上述 reset/read 点采集真实 counter，而不能写入文件大小、估计值或零。

这不是通用网络框架设计：它明确排除文件、共享目录、helper process、固定等待和明文 fallback。D4 已有**可审查的最小契约**，但没有实现/独立进程 E2E 验证，故仍是 D 阻塞而非可用 transport。

## 6. 实施就绪判定

| 门 | 状态 | 所需批准/证据 | 是否允许 M2 代码 |
| --- | --- | --- | --- |
| D1 rank 泄露 | 推荐条款已精确定义，未获批准；VFSS permutation-hiding E2E 未证明 | 团队批准受众/值/零持久化规则；审计两方均不知合成 permutation；泄露与 transcript test | 否 |
| D2 inverse-routing | **D：没有可审计构造** | 选定 inverse/reinvoke/scatter 之一的完整 primitive、消息/离线材料、arithmetic→XOR proof；record-binding conformance、oracle differential、独立进程 E2E | 否 |
| D3 域宽/range | 仅有位宽表，非完整 proof | 32+index_bits DCF/uCMP range/direction proof、`n=1..10^6` 表点与端点/unused-domain/error conformance | 否 |
| D4 transport/metrics | 最小 contract 已写，未批准、未实现验证 | 有界 fail-closed transport、真实 Peer counter reset/read、P2 静默与三进程 E2E | 否 |

只有 D1--D4 全部同时具备可批准精确定义、无已知安全语义冲突、文件级最小实现边界，以及可执行的 conformance → differential → independent-process E2E 验收条件，才可建议用户创建 `codex/m2-protocol-i`。当前不满足，且没有任何 M2 代码、CMake 或测试修改。

**M2.1 只闭合可实现性与决策证据；未经用户明确批准 D1–D4，不授权开始 M2 代码修改。**

Update 2026-09-06: M2.12 has explicit user approval for the D1 restricted
shuffled-domain `rank_P` reveal and validates a bounded P2/P0/P1 E2E with the
M2.11 reverse composition. This does not supply a paper-exact proof, padding,
or raw-score-share input adapter.
