# M6A-P2-I-E15：Protocol III+AAV86 设计门（2026-10-03）

## 结论和证据边界

**DESIGN_GATE = NO-GO for secure runtime；2r = PROJECT_TARGET / NOT_PROVEN。** 本阶段只读审查，不修改 Protocol III secure runtime，不把 I+AAV86 的 shuffle、泄露或轮数直接移植。最小阻塞是：（1）自适应边与每轮 masked-key/DCF 材料的可信离线 T 预处理和单方联合视图未闭合；（2）III 的 field Fselect/Fsort 固定全两两 core 与 AAV86 的每轮公开 local-rank 控制没有可证明的压缩组合；（3）最终 shuffled handle 到原序 XOR mask 的安全逆映射及计量缺接口。任一缺口均足以阻止 2r 或完整性能验收。

已读证据：M5 G3 接收与 F1 修复、正式 `protocol_iii_raw_score_mask_party`、M5 field payload/DPF/Fselect/Fsort 决策及源文件；Agarwal CCS 2024 会议版 §4.2、§5.2–5.4/Theorems 4.2、5.1，AAV86 FOCS 1986 原文 pp. 506–507。原始 AAV86 给出 Valiant 比较轮与期望 `O(n^(1+1/r))` 比较，按 pivot 比较递归；Agarwal Algorithm 2 的 CA 变换仅公开每轮 local rank 来更新桶。Theorem 5.1 是 **shuffle-based CA compiler** 的 `2r+1` 轮及边/节点成本，不是 III+AAV86 定理。会议版未给 III+AAV86 的 2r 构造。下列所有 III+AAV 组合、mask、Q20.12 规则均属项目扩展或待验证设想；`AUTHOR_EXACT=NOT_PROVEN`。

## M5 可复用边界

M5 G3 已独立接受 `main` 的四轮原始分数份额→原序 XOR mask C-INSTANTIATION（score adapter 2 + GRank 1 + DPF routing 1）；`ProtocolIIIField=F_(2^127−1)`、非零 payload 编码、field DPF、两轮 Fselect/Fsort 分别有功能与进程证据。Fselect 输出一个 field-shared record，Fsort 输出 rank-order field-shared records；两者输入是已 field-shared 且 key/record 一致的 payload，不是 Q20.12 raw-score，也不直接给原序 mask。F1 的四轮 mask 路径依靠 ring indicator 专用适配，并不建立通用 field-to-XOR 或 arbitrary ring-to-field 转换。M5 H1 的数量级通信通过但 Theorem 4.2 精确成本差 `254n` bits；field DPF 正式安全证明与 common-mask 优化仍未完成。现有 `protocol_iii_raw_score_mask_party` 的全两两 GRank、掩码 rank DPF routing 是功能/计量基线，不能改名为 AAV86。

## 候选接口表与单方视图

| 阶段 | T / P0 / P1 输入与材料 | 在线公开/输出；待证明点 |
| --- | --- | --- |
| 统一输入 | P0/P1 各持长度 n 的 Q20.12 `Z_(2^32)` score 加法份额；公开 n,K,r,D、版本/session。T 不见分数、rank 或输入 seed。 | 必须把 `score DESC, original_index ASC` 注入严格 priority key，padding 永后于真实元素。score adapter 的两轮及字节单列。 |
| 初始隐藏布局 | 离线 T 或已批准的 shuffle 功能准备 same-π 的前向材料与逆路由；每方只得自己的 permutation/mask shares。 | P0/P1 得 shuffled record shares 与对应 masked list；任一单方、包括 T，允许视图须重新论证。不能把 I 的具体 π/逆路由当现成 III 证明。 |
| 每轮 t 的图 | 公开活跃 shuffled handles 和前轮 local rank/bucket；图 `G_t=(V_t,E_t)`，canonical edge (a<b)。T 在输入到来前仅按 session,t,a,b 生成所有允许候选边的两份独立 DCF/uCMP keys，以及节点 fresh mask shares；不能在线查询/补发。 | 真正消费 `E_t`，其余预留槽不求值；公开 `y_t`、local rank、pivot/bucket、图/流量/abort 均需明确列入泄露或隐藏。 |
| 图后压缩路由 | III field record、非零乘法掩码、Beaver、field DPF 的目标/索引绑定当前 handle 与最终 rank；T 的 package 一次性、按 round/edge/role/session 绑定。 | 需说明 masked field record `z·s` 的公开时点、rank share 与 DPF key 的依赖、何时可 FullEval；不得把 field 元素与 `Z_(2^b)` 混算。 |
| 终局输出 | 公开 shuffled 全序只在明确允许的泄露合同下使用；安全路由材料把 handle 上选中位逆送回原槽。 | 输出 P0/P1 各自长度 n XOR bit shares；secure 方不得得到原槽 selected index、原序明文 mask、完整 π。TEST_ONLY 控制器单独重构并检查恰 K。 |

单方 P0/P1 视图至少包含：自己的 raw shares、自己的初始/逆路由因子、按轮节点掩码 share 与全预发 key 池、自己的 field/DPF/Beaver 材料、所有公开 y/local-rank/pivot/bucket/活跃图、己方协议帧、输出份额。T 视图至少包含其采样的全部预处理随机币、完整候选 key manifest、分发字节及公开 shape/session；T 在在线输入释放前退出。必须对 **共享节点 mask + 多 key 相关阈值 + 自适应选边 + local-rank reveal + DPF/field 公开乘法掩码** 的联合视图给出单方模拟或适用的组合定理。当前 DCF 功能测试、会计守恒及 M5 固定图证据均不足以证明此性质；尤其 T 知完整 π 时其 party-view 条件与 I 的 Chase 路线不同。

## 因果 DAG 与轮数

候选核心的必要依赖为：
`G_t`（由上一轮公开 local ranks 决定）→ `y_t` masked-key opening → 本地 `DCF.Eval(E_t)`/聚合 → local-rank share exchange → 公开 local ranks → `G_(t+1)`。即使每轮两次在线交换可执行，`R_rank(t)` 必须先于 `R_open(t+1)`；这给图控制的候选 `2r` 交换链。它**没有**说明初始隐藏 shuffle 与首轮 opening 可并入哪一轮，也没有说明 III field Fsort/Fselect 的 R1/R2 乘法 opening、masked rank opening及最终逆路由能与这条链重叠。当前固定图 III 两轮 core 的 R1 收 masked keys 和 field multiplication openings，随后才能 DCF/Beaver；R2 收 masked ranks 和 masked payloads，随后才能 DPF Eval/FullEval。AAV 下一轮图依赖本轮 rank 公开，未证明固定 core 的 R2 可以同时承载 CA 控制而不增加泄露/轮数。因此 **完整 raw-score→mask 轮数 = NOT_PROVEN**；2r 只作研究目标，不能作为实现标签。score adapter、初始 shuffle、每轮 CA、field routing、逆映射必须逐帧画 DAG 后才能定总数。

## 原序 mask 代数义务

设初始隐藏置换为 `π: original→shuffled`，公开图控制所致布局变换为 `ρ_t`，终局 handle `h=ρ_r∘…∘ρ_1∘π(j)`。排序后 handle 上的 `m[h]=1[rank(h)<K]` 必须以共享值保留；若全序公开，公开范围仅为 shuffled-domain local/final rank，不能公开 j↔h。逆适配必须在共享域计算 `c[j]=m[ρ_r∘…∘ρ_1∘π(j)]`，再输出 `[c[j]]_0 XOR [c[j]]_1=c[j]`，截去 padding。可考虑一次性 DPF/secret-permutation routing，但现有 III Fselect 输出单点 field record、Fsort 输出 rank-order field records，不提供该逆算子。若用 field 份额，需证明 field→bit XOR 转换与恰 K；若沿用 III ring indicator，则需证明其对隐藏 handle→原槽路由有效并计成本。不能由 TEST_ONLY 明文重排补缺。

## 容量与统一指标接法

若选输入无关的 full candidate pool，每方至少保留 `r·C(D,2)` 个 edge key 槽，双方合计 `2r·C(D,2)`；实际 `e_A=Σ_t|E_t|`、`v_A=Σ_t|V_t|` 是在线消费/参加节点，不等于预留容量。D=128,r=5 已达每方 40,640 槽；再加 III field DPF、乘法、rank mask 与逆映射材料，包长度和峰值 **NOT_MEASURED**，不能借 I 的字节数代填。若主张 exact-edge 离线材料，则前轮在线 rank 决定后轮边，必须提出不让 T 在线参与的盲生成/通用可实例化 key 机制；当前无此接口。64 MiB 包、768 MiB RSS 和 D≤128 仍是当前控制，放宽前不可计划正式大点。

下一阶段只建议三个最小接口：`ProtocolIIIAav86StageMaterial(session,t,D,edge-id,mask,key,one-shot)`；`protocol_iii_aav86_local_rank_party(public G_t, masked handles, stage material) -> local-rank shares + exact counters`；`protocol_iii_aav86_inverse_mask_party(hidden layout material, shared handle bits) -> original-order XOR bits`。先做 n=2/5、同分/极值/非二次幂的 conformance、冻结 oracle differential 和独立 T/P0/P1 E2E；随后证明单方视图和帧 DAG，最后才接统一九指标。接法为同一 `offline_time_ms`、`offline_material_total_bits`、`online_time_ms`、分方 sent/received、因果轮、实际 DCF 长度倍增 PRG/field DPF AES 等分栏、`e_t/v_t`、总时间逐次派生；与同功能的 III 全两两四轮完整入口同输入/种子/网络比较。设计门未闭合时上述性能一律 `NOT_MEASURED`。
