# BMW16 S5：Protocol I + BMW16-derived Select + DCF 组合门

日期：2026-10-06
状态：**NO-GO_FOR_SECURE_IMPLEMENTATION**
候选标签：`Protocol I + BMW16-derived Select + DCF`
明文参考标签：`BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R_TEST_ONLY`

## 1. 决策摘要

S5 已独立接收 S4 的 TEST_ONLY 算法和冻结结果，接受范围限于：严格全序输入、理想独立均匀无放回抽样、S4 固定有限参数、成功路径精确分割、失败时无候选输出。S4 不是 BMW16 Algorithm 7 的逐字实现，也不继承 Theorem 8 的概率数值；它是受该论文四轮/线性比较目标启发、另行给出失败界的项目衍生构造。

Protocol I 组合门不通过。主线 VFSS 的 shuffle API 只输出 secret-shared payload，没有与该输出绑定的同置换 public masked list `pi(x)+r`。E20/E21 提供的流式全池材料行为和性能属于其他隔离 revision；它们不能填补联合视图证明，也不能被本任务假设已合入。自适应比较边、两实例共享真实记录身份、tagged stable-key 范围、阈值 DCF 取材、概率 abort 传播和原序逆路由的组合都没有足够证据。因此本提交**不改 VFSS secure runtime，不创建“secure”入口**。

## 2. 概率性 API 决策

用户已批准允许少量失败。本项目的候选 API 合同据此固定如下：

```text
request = (session, fingerprint, n, K, raw signed-Q20.12 score shares,
           record/original-index binding)
result  = SUCCESS(mask_share_P0, mask_share_P1)
        | ABORT(reason_class, no mask share)
```

`SUCCESS` 必须重构为原输入顺序、长度 `n`、二值且恰有 `K` 个 1 的 mask，并与稳定 Top-K oracle 一致；在比较、key 编码、路由原语均满足其精确功能性的条件下，算法本身满足 `Pr[wrong output | SUCCESS]=0`。任何样本 bracket、U/W 上限、材料/消息校验或传输阶段失败均不得输出可用 mask；不允许明文 oracle 检错、重试、在线 Dealer、未计入的全对全 fallback 或安全路径明文重构。

工程目标暂定为目标配置下 `Pr[ABORT] ≤ 10^-6`。S4 的独立理想随机带界为 `4.50140648×10^-7`，仅给安全随机生成与 shuffle 统计距离留下小于 `5.49859352×10^-7` 的联合余量。该余量尚未分配，也没有 PRG/shuffle 定理或参数可代入；因此密码学实现的 `10^-6` abort 合同为 **UNPROVEN**。最终合同须满足
`epsilon_ideal_abort + epsilon_PRG + epsilon_shuffle + epsilon_sampling_bias ≤ 10^-6`，其中各项不能用运行观察替代。

Abort 状态应带 session/phase/reason-class 绑定；只有两方完成相同的终止握手后才返回共同 `ABORT`，失败的连接方本地 fail-closed 且不释放 mask。如何避免一方断连后另一方挂起或释放部分 mask、reason-class 是否公开、abort 与对方输入份额的联合独立性，都需写入具体 transport/simulator 证明。不能把一次本地异常当作安全的双方一致 abort。

## 3. 组合接口候选与证据状态

| 阶段 | 候选输入 → 输出 | 本阶段判定 | 未闭合条件 |
|---|---|---|---|
| raw input | 双方 signed Q20.12 raw-word shares → 每条记录的 raw score shares | 主线有窄适配器 `protocol_i_raw_score_input_party`，但它不是 BMW16 接口 | 输入份额布局、记录 ID/original-index 共享方式、padding contract 与 S4 两个不同哨兵数组的绑定 |
| stable priority key | `(signed_score DESC, original_index ASC)` → secret-shared strict key | 主线有 `protocol_i_priority_key` 和 34-bit score-to-priority adapter 的局部实现 | 证明 score shares 的 lift/sign 与 index 注入对 INT32 全域精确；S4 另需 5 类 dummy/sentinel 的总序类别 tag。若 tag 与 real tuple 拼接，比较宽度增加 3 bit；`n=10^6` 的候选 Bin 为 56，超出现有 uCMP `34..53`。若 dummy 类型公开后本地分支，需重新审查泄露 |
| same hidden shuffle | key shares、real-record identity、分类 tag → 两个 median 实例共同使用的 hidden shuffled handles | 当前 SecretSharedShuffle 为两遍 Permute+Share 的 share 输出 | 没有 `pi(x)+r` 输出及其与 share、DCF gate、两个 Select 实例的同置换绑定；没有联合 view simulator |
| R1–R4 Select | 每轮依赖之前结果的比较图 → 两份 median partition | S4 明文比较深度为 4 | 公布图、边地址、结果、pivot、U/V/W 或失败分支对完整单方 view 的模拟尚未证明；隐藏图则需要 oblivious address/material 方案和额外在线交互 |
| 两哨兵交集 | low-sentinel Accept ∩ high-sentinel Reject → 唯一 selected record/key shares | 数学归约通过 | 两数组真实记录句柄必须稳定关联；不得公开 selected original index 或用不同 shuffle 后的数组槽号求交 |
| DCF membership | selected priority key 与每条 real priority key → shuffled membership bit shares | 明确目标方向为 `priority_key(record) ≤ selected_key`，因为升序编码下前 K 个 key 最小 | 阈值在线输入相关，T 不能提前为其重新 keygen；候选方案是事先生成阶段隔离的所有 real-real pair 材料并读取选中 handle 的 star 边。需证明 pair-to-key 绑定、一次领取、访问模式、与其他阶段 key 的联合安全及精确 comparator range |
| inverse route | shuffled membership shares → 原序 membership shares | 主线有 inverse Permute+Share 组件 | 与两哨兵共同 handle、score/index shares、membership carrier 使用同一全局置换的组合正确性和 simulator 未证明 |
| output | 原序 arithmetic membership shares → XOR mask shares | 仓库契约要求原序 XOR mask | 算术/XOR 分享转换、abort gating、恰 K 和安全过程中不重构都需在新完整入口逐项 conformance/E2E |

`original_index` 必须是输入记录的稳定 tie-break 字段；洗牌后的 handle/slot 不得代替它。主线窄 score adapter 把源 slot 注入 priority key，可能为 real-record tie-break 提供可复用的局部构件，但不会自动提供 dummy 标签、跨实例 identity join 或 BMW16 阈值选择。

## 4. 可见图与单方联合视图

候选设计设想用隐藏均匀 shuffle 将固定严格 rank 顺序随机映射到 handles，再公开每轮的抽样/比较边和 noiseless 比较结果。只考虑该 public transcript 的边际分布时，每个输入 strict order 经独立均匀 handle permutation 后具有同一分布；这只是一个候选论证，不能替代所需证明。

实际模拟对象必须对每个腐化方分别覆盖完整联合视图：该方输入 shares、其已知局部 permutation/inverse、shuffle 消息与本地状态、public masked list、DCF/uCMP key shares、共享节点或逐边 masks、每个阶段读取/未读取的 material ID、边图和结果、abort 状态、最终输出 shares。特别需要解释两个 median 数组如何用相同 real handles 而保持各自 padding/sentinel 次序；若各自独立 shuffle，交集没有可直接使用的 handle；若共享 real handles，跨实例 linkability 和 material ownership 要纳入 simulator。

一条无法通过 review 的泄露反例是：若 public graph/比较结果能按该方已知局部 shuffle 或材料 slot 对回某些源记录，则完整视图就能区分具有不同 stable rank 布局的输入，即使单独观察未标记的 edge-count 序列只依赖随机 permutation。当前没有证明这种关联不发生，也没有一个可调用的 ideal functionality 覆盖它。故本报告不把“均匀隐藏置换让 rank handles 随机”提升为通过结论。

## 5. T 离线材料覆盖和容量门

T 在输入未知前可为每个可能阶段和无序 handle pair 生成一次性材料，在线后保持静默。若两哨兵实例使用独立 key，阶段身份必须包含 `(session, instance, round, canonical handle pair, comparator domain, material_id)`。每条 uCMP logical comparison 在现有 adapter 中调用底层 DCF 两次；不得将封装为一次 uCMP 解释为一次 DCF Eval。为避免尚未证明的共享节点 mask/多 key 联合安全，本次低界估算采用每边两个独立 endpoint mask shares，并分别 keygen 各阶段边；这不是安全证明。

容量预检脚本只用 uint64 checked arithmetic，不分配池。表内“optimistic”假定离线能固定、公开 shuffled-handle 样本集合 S，且有证明保证其输入无关；R1 为 `C(s,2)`，R2 为覆盖所有候选 pivot∈S 的 `M*s−C(s+1,2)`。当前 Protocol I 尚无所需 same-permutation shuffle/view proof，所以不能把这个缩小域当成已批准部署容量。“conservative”对两份实例的 R1/R2/R3/R4 全部各预留 `C(M,2)`，再加最终 membership `C(n,2)`，即 `8C(M,2)+C(n,2)` logical material slots。

按 tagged 5-class comparator 假设 `Bin=32+index_bits+3+1`，每方每 slot 下界为 `24 + 16 + (57+24*Bin)` bytes（边身份/长度 envelope、独立 endpoint mask shares、当前 party uCMP 序列化）；未含 AEAD、session/phase/material-ID 头、索引、传输缓存、T keygen transient、落盘校验和。全体 payload 是解析预检下界，不是实测，不是 E20 格式。

| 尺度/配置 | M | optimistic slots | conservative slots | Bin | 每方 optimistic 下界 | 每方 conservative 下界 | 宽度 |
|---|---:|---:|---:|---:|---:|---:|---|
| n128 K2 | 254 | 201,420 | 265,176 | 43 | 227,403,180 B | 299,383,704 B | 支持范围内 |
| n128 K8 | 242 | 185,022 | 241,416 | 43 | 208,889,838 B | 272,558,664 B | 支持范围内 |
| n256 K2 | 510 | 736,078 | 1,071,000 | 44 | 848,697,934 B | 1,234,863,000 B | 支持范围内 |
| n256 K8 | 498 | 705,578 | 1,022,664 | 44 | 813,531,434 B | 1,179,131,592 B | 支持范围内 |
| n1000 K80 | 1,842 | 8,548,352 | 14,063,988 | 46 | 10,266,570,752 B | 16,890,849,588 B | 单个 R3/R4 median pair pool 已有 `C(M,2)=1,695,561`，超过当前 party package 的 1,000,000-edge 上限 |
| n10,000 K1（worst M=2n） | 20,000 | 895,232,736 | 1,649,915,000 | 50 | 1,161,116,858,592 B | 2,139,939,755,000 B | 每方 optimistic 下界约 1.16 TB |
| n100,000 K1（worst M=2n） | 200,000 | 86,430,742,844 | 164,999,150,000 | 53 | 118,323,686,953,436 B | 225,883,836,350,000 B | 恰到当前 Bin 上限 |
| n1,000,000 K1（worst M=2n） | 2,000,000 | 8,545,251,477,372 | 16,499,991,500,000 | 56 | NOT_COMPUTABLE | NOT_COMPUTABLE | tagged comparator 超出现有 Bin≤53 |

表内整数由随附容量审计器以 uint64 checked arithmetic 重新计算并与审计 JSON 核对；未发生溢出时列出精确槽数与字节下界，`Bin=56` 一行按当前 53-bit comparator 上限明确标成 `NOT_COMPUTABLE`。这些是解析预检下界，全部字节/生成时间均为 `NOT_MEASURED`。n≥10⁴ 的旧 E20 容量数据来自不同实现标签/材料结构，不在此与本表拼接。

即使允许 streaming，寻址也不能让材料 slot 暴露为 secret-adaptive access trace；E20 的逐边 AEAD/流式驻盘测试不证明 BMW16 各轮多 key joint simulation。若将图隐藏，需 oblivious material selection/逐轮控制方案，另计交互、材料、网络和泄露，而不能保留明文四轮成本直接作为协议轮数。

## 6. 消息 DAG 与计量合同

| 顺序 | 候选 phase | 依赖/输出 | 轮数状态 |
|---:|---|---|---|
| O0 | T 离线生成、分发 shuffle、share-translation、各 `(instance,round,pair)` DCF/uCMP、membership、逆路由材料；T 停止在线参与 | 只允许 input-independent 完整材料；要冻结 slot 覆盖/领取合同 | T 时间、材料、峰值/磁盘 `NOT_MEASURED` |
| 1 | score lift/sign 与 stable key shares | 原始 score word shares + 原始 index binding | 主线窄 adapter 本身有 2 个消息阶段；BMW16 full entry 因果总轮数 `NOT_RUN` |
| 2 | 同一 hidden shuffle 两个 median views | 保留 score/key、原 index tie-break、真实记录句柄、tag shares | 主线 share shuffle 前向有 2 个 round；public `pi(x)+r` 与该返回值结合 `UNPROVEN` |
| 3 | R1 比较并完成后计算 pivots | 仅本轮开始前冻结 sample pair 图；结果生成 R2 pivots | 逻辑 comparison depth R1=1；在线交互 `NOT_MEASURED` |
| 4 | R2 比较并完成后得到 P/U/q | 仅由 R1 pivots 形成 edges | depth R2=1；online barrier/bytes `NOT_MEASURED` |
| 5 | R3 V×U 比较并完成后得到 ranks/W/qW | 仅由 R2 state 形成图；超界变 abort | depth R3=1；`UNPROVEN` |
| 6 | R4 W pair comparisons and exact ranks | R3 state 决定 W；成功后形成两份 partition | depth R4=1；`UNPROVEN` |
| 7 | 两哨兵 shared-handle 交集 | 不打开 selected original index | 组合 identity join `UNPROVEN` |
| 8 | final DCF membership star | 每个 real handle 与 selected handle 在 project-priority ascending key 下比较 `key_i≤key_selected` | all-pair pool可覆盖候选，实际消息与 DCF Eval `NOT_MEASURED` |
| 9 | inverse route + arithmetic-to-XOR output + abort gate | 输出原序 mask shares；abort 时零 mask | 主线逆向 shuffle组件有 2 rounds；整条消息 DAG和共同 abort `UNPROVEN` |

必须分别报告明文 Select 比较层数 `4`、secure full-entry因果在线轮数 `NOT_RUN`、`R1..R4` logical edges、uCMP calls、底层 DCF Eval、T离线 keygen/传输/ready bytes、在线每方 bytes、重试（禁止）和 abort。不得把 `4` 填进 `online_rounds`。

## 7. 条件门及下一步授权边界

| Gate | 结论 |
|---|---|
| S4 独立算法接收 | `PASS_WITH_EXPLICIT_LIMITS` |
| 隐藏统一 shuffle 的完整联合视图 simulator | `NO-GO / UNPROVEN` |
| public adaptive graph 泄露与 abort simulator | `NO-GO / UNPROVEN` |
| 离线材料 slot 覆盖、一次性绑定、streamed access simulator | `NO-GO / UNPROVEN` |
| stable tagged key 全域 comparator / uCMP range | `NO-GO / UNPROVEN` |
| 两哨兵句柄交集 / DCF threshold membership | `NO-GO / UNPROVEN` |
| 逆路由、原序 XOR mask、双方 abort gating | `NO-GO / UNPROVEN` |
| 组合后消息 DAG与在线 rounds | `NOT_RUN / UNPROVEN` |
| `Protocol I + BMW16-derived Select + DCF` secure runtime | **`NO-GO_FOR_IMPLEMENTATION`** |

在以上核心门未独立 review 通过前，不写入 `VFSS/`。未来门通过后按 conformance → frozen oracle differential → 独立 T/P0/P1 E2E 实施；然后做接口绑定/篡改/重领/静默/断连/超时和全量计量。Protocol III BB90 槽位、PR #28、E20/E21 原数据与 E16/E17 接收结论均不因本决策改名或改变状态。
