# M6A-P2-I-E3：Protocol I+AAV86 全两两预处理组合设计门

日期：2026-10-02
审查 worktree：C:\Users\28641\Desktop\MoE_Top-k_Reproduction_Evaluation__m6a_p2_i_allpairs_experiment
分支：codex/m6a-p2-i-allpairs-experiment
审查起点：c3926c68fd14f270faa8b55234311071947fa080
文档属性：设计与可行性审查；不构成运行时规格批准。

## 0. 结论摘要

本稿审查用户锁定的全两两离线材料路线：T 在输入到达前，为每个算法 iteration、每个候选无序 endpoint pair 生成独立 party-local uCMP/DCF 比较材料；在线图确定后，双方只消费本轮活跃图中的槽位。本文把材料可用性、协议组合闭合、安全证明和 secure runtime 分开评判。

| 设计门 | 判定 | 依据与限制 |
| --- | --- | --- |
| OPTION1 ALL-PAIRS MATERIAL AVAILABILITY | PASS_WITH_FINDINGS（研究容量模型） | 在固定 endpoint 域 D、固定轮数 r、每轮每节点一份 mask、每轮每无序 pair 各有一个 endpoint-mask-difference 对应的 key 槽、每槽最多消费一次的前提下，r·C(D,2) 个 party-local 槽覆盖该轮任何简单无向候选图。E1 对 n=5、r=2 实际生成真实 uCMP party key 并消费 8/20 个槽；E2 独立复核 fixture。没有证明整池 view 安全，也没有生产 ABI。 |
| PROTOCOL I+AAV86 COMPOSITION DESIGN | NO-GO（接口和因果设计未闭合） | score adapter、部分 shuffle、全 clique CmpAgg 与 mask pipeline 的接口存在；AAV86 自适应图、活跃子集 local-rank、每轮线上 mask/open、rank/bucket 可见性、与同一 shuffle 的输出逆映射没有已实现/已证明的组合。CA 每轮发送方向与消息依赖可以列出候选 DAG，但初始 shuffle 和输出映射仍无可复用等价证明。 |
| FULL-POOL PARTY-VIEW SECURITY | NO-GO / UNPROVEN | 无针对共享 endpoint masks 导致的相关 DCF key 全池分布、按前轮状态自适应选择 key、查询/消费顺序及 shuffle/rank transcript 的单方视图模拟证明。单个 uCMP key 的功能正确或底层 DCF 安全不能推出整池组合安全。 |
| P2-I SECURE RUNTIME | NO-GO（保持既有结论） | 存在安全与因果 blocker；本稿不批准 runtime、生产接口或 benchmark。 |

项目契约仍为 signed Q20.12 分数份额输入，分数降序、同分按原始输入下标升序，输出原输入顺序的长度 n XOR bit-mask 且恰有 K 个 1。候选算法是 AAV86 全排序后取前 K，不能称为论文直接给出的 Top-K mask 协议。Protocol III 仍按本任务延期到 Protocol I 路线完成之后；固定 M 仍是延期备选，本阶段不分析或使用它。历史证据限制 AUTHOR_EXACT = NOT_PROVEN 保持不变。

## 1. Git 基线和工作区保护记录

### 1.1 起点状态

E3 开始时两个 worktree 均位于同一个 HEAD：

| worktree | branch | HEAD | 起始状态 |
| --- | --- | --- | --- |
| 主工作区 C:\Users\28641\Desktop\MoE_Top-k_Reproduction_Evaluation | feat/m6a-performance-evaluation | c3926c68fd14f270faa8b55234311071947fa080 | 有4个既存tracked文档修改及4项未跟踪内容；全部保留，本文未触碰。 |
| 隔离实验 worktree C:\Users\28641\Desktop\MoE_Top-k_Reproduction_Evaluation__m6a_p2_i_allpairs_experiment | codex/m6a-p2-i-allpairs-experiment | c3926c68fd14f270faa8b55234311071947fa080 | 已有E1报告、E2报告及experiments/未跟踪内容；均保留，本文只新增E3文档。 |

主工作区起始 dirty 内容为：修改 PROJECT.md、docs/IMPLEMENTATION_PLAN.md、docs/PAPERS.sha256、docs/REFERENCE_MANIFEST.md；未跟踪 docs/decisions/M6A_AAV86_ALGORITHM_AND_STABLE_ORDER_DECISION_2026-09-27.md、docs/decisions/M6A_PROTOCOL_I_AAV86_PREPROCESSING_AND_SECURITY_DESIGN_2026-09-28.md、docs/reproduction/M6A_BASELINE_AND_SOURCE_INVENTORY_2026-09-27.md 和根目录 siamjdiscrmath.pdf。E1/E2 已记录这些主工作区变更存在于审查前；不能把它们归入本阶段差异。主工作区及实验 worktree 的 branch、HEAD、原有状态均未改变。

### 1.2 M5 G3 与 M6 起点

- G3 接收报告记录 G3_ACCEPTANCE = PASS，接收检查对象为 clean main@9b3ce3747b1734602e3edf4c644ae1b6da52e8c1；接收工作记录为 2026-09-26。
- M5 closeout PR #27 已合入当前 c3926c68 主线；M6 分支从该主线 revision 建立。此处沿用 P0 盘点及 M6A handoff，不重跑 M5 验收。
- M5 的工程接收不构成作者精确复现证明。AUTHOR_EXACT = NOT_PROVEN、EXACT_THEOREM_4_2_COST_MATCH = NO、common-mask optimization 未实现、field DPF formal proof 未完成、通用 ring-to-field / field-to-XOR 转换未实现等限制保持原文状态。
- M5 的 Protocol I 三轮对象是带 C-INSTANTIATION 标签的三轮 shuffle/CmpAgg 核心，不是 raw-score→mask 完整入口；历史 raw-score→mask modular engineering path 为两轮 score input adapter 加六轮 fixed-clique pipeline，且其历史 E2E 有可选 EMP 依赖。不得用三轮替代八轮完整入口，也不得把任一计数挪作 AAV86 CA 的轮数。

## 2. 证据标签和范围

本文用以下标签分开证据类型：

- PAPER_DIRECT：论文页面直接陈述的定义、算法或定理。
- PAPER_DERIVED：明确列出前提后，从论文算法/定理作出的推导。
- PROJECT_CONTRACT：仓库冻结的输入、stable tie、输出和接口语义。
- E1_EXPERIMENT_OBSERVED：E1 隔离 TEST_ONLY 实际运行记录的观察值。
- E2_INDEPENDENTLY_RECHECKED：E2 对 E1 保存数据、源码逻辑或小规模结果的独立复核。
- ANALYTICAL_BOUND：由本文列出的域和公式可复算的容量/消息上界。
- UNPROVEN：缺少构造、证明或适配证据。
- NOT_MEASURED：无实际测量。

本稿没有运行仓库测试、E1 实验 harness、生产 benchmark 或参考实现。E1/E2 输出只用于审查它们实际覆盖的窄结论，不作安全协议、独立 party、生产延迟或作者复现证据。

## 3. 全两两路线的精确定义

### 3.1 端点域、活跃图和槽位

令 n 为逻辑输入数，K 为输出选择数，r 为 AAV86 算法最多迭代轮数。令 D 为 AAV86 比较图使用的 shuffled endpoint 域大小；某 iteration i 的活跃图写作 G_i=(V_i,E_i)，其中每个节点 handle 属于固定域 [D]，E_i 是活动子问题内本轮 AAV86 比较边的简单无向边并集。

一条活跃边 {a,b} 表示本轮用同一稳定全序上的 endpoint 值比较一次；canonical 槽按 a<b 索引。此处的 endpoint 是 shuffle 后的图节点 handle，不是原始输入下标。AAV86 的 pivot-pivot 边及 pivot-to-nonpivot 边构成本轮边；各桶仅在下一轮形成子问题。对于大小 m、剩余 d 轮的子问题，E1 统计器使用 t=ceil(m^(1/d))、q=t−1 个 pivot，贡献 C(q,2)+q·(m−q) 条边；不同活动桶在该轮分别生成图。

全两两池按 iteration 隔离，不跨 iteration 重用同一槽：

- 每轮 i，为域内每个 canonical 无序 pair {a,b} 预留一个 party 0 槽和一个 party 1 槽；
- 每轮每个 endpoint a 有一份节点 mask R_i,a；同轮所有 incident edge 共用 R_i,a；
- 跨轮 mask 新鲜，且各 iteration 的 key 槽互相隔离；
- 活跃池容量每方是 C(D,2)，全协议每方是 r·C(D,2)，双方 DCF party key 对象合计 2r·C(D,2)；
- 活跃比较数为 A=Σ_i |E_i|=Σ_i e_i；每方对应消费 A 个 party-local key，不活跃槽仍占离线存储。

在每个子问题按简单图比较且同轮子问题互不相交的前提下，单轮宽松上界为 e_i≤C(D,2)；全池容量是任何该域内简单图的充分候选集合。E2 没有给出比 C(D,2) 更紧的统一 per-round worst-case bound，因此更紧上界记为 UNKNOWN。AAV86 的期望比较数 E[e_A(n,r)]≤c(r)n^(1+1/r) 不是任一轮的最坏情况界，也不是全两两材料容量界。

### 3.2 逻辑 n 与 padding 域必须分开

E1 的 pool fixture 使用无 padding 的 D=n endpoint 域，因此记录的物理材料池为 r·C(n,2)。它只证明 E1 约定下的逻辑槽位数，不自动成为 Protocol I 生产包长度。

当前 Protocol I input layout 对逻辑 n 生成 N=max(2,next_power_of_two(n)) 的 padded_n；raw-score adapter 对 padding 槽放 signed 最小值，再产出 padded_n 个 priority-key shares。AAV86 图究竟只含 n 个逻辑节点，还是含 N 个 padded 节点，现有设计没有闭合：

- 若 CA 图仅含逻辑节点，candidate edge 池按每方 r·C(n,2) 计；还需单独定义从 padded score adapter 到 logical CA domain 的选择/稳定下标关系。
- 若 CA 图含 padding，则池容量按每方 r·C(N,2) 计，并须证明 padding 不会被选作 pivot、不改变 pivot 排序与桶划分、不会影响排序功能或输出。现有 signed-min sentinel 与 fixed-clique padding 行为不能替代这一 AAV86 CA 证明。
- 例如 n=5 时，N=8；两轮池分别为逻辑域20槽/方或padded域56槽/方。E1 的20槽物理样本属于前者。n=1 的项目输入合法，但当前 I layout 将其 pad 到2；AAV86 singleton/空图终止及mask路径仍须定义。

因此下面成本公式都以显式域大小 D 表示；除 E1 n=5 D=5 测量外，不把 D=n 当成生产已定规则。

### 3.3 槽字段及低层 key 实际绑定方式

目标 pool 的逻辑槽建议逐项携带下列可审计索引字段，以便唯一定位一次材料调用：

| 字段 | 值/用途 | 现有证据 |
| --- | --- | --- |
| protocol_version | Protocol I+AAV86 pool schema 版本 | E1 有 TEST_ONLY wrapper version 字段；生产 schema 未定义。 |
| session | 单次协议实例标识 | E1 wrapper/pool envelope 检查；生产动态池 ABI 未定义。 |
| iteration | AAV86 轮 i | E1 wrapper lookup key 包含 iteration；当前 ProtocolIPartyPackage 的 fixed clique edge item 没有迭代字段。 |
| endpoint_a, endpoint_b | shuffled handle 的 canonical pair a<b | E1 wrapper 检查 canonical 顺序；底层 DCF key 自身不存 endpoint 标签。 |
| invocation_id | 同轮此端点对的调用序号；E1固定为0 | E1 wrapper查表参数；不是DCF中的密码学字段。 |
| party_id | 0或1 | DCF party-local key序列化可报告party/bits；E1 wrapper同时校验party。 |
| comparison_bits | 进行uCMP的环位宽b | DCF key header/party material及E1 wrapper参数可核对。 |
| mask relation | α_i,a,b=(R_i,a−R_i,b) mod 2^b | DCF keygen的数学输入；E1 keygen使用端点mask差。未见序列化DCF key暴露/认证可读endpoint元数据。 |
| graph/one-shot state | 本轮图允许边、消费次序和是否已用 | E1为单进程内存wrapper状态，未持久化为安全transcript/认证状态。 |

实际 ProtocolIUcmpMaterial 由 bits、mask_left、mask_right 生成 α=(mask_left−mask_right) mod 2^b，再调用仓库 keyGenDCF；分别导出 party 0/1 key。这个低层 key 的密码学计算参数对应 α 与 DCF key 本身，不在 key 本体内认证 session、iteration、endpoint pair、graph digest 或 invocation。上表是目标 pool 的索引/审计契约，不应误读为已实现的密码学绑定。

E1 的 C++ wrapper 把槽位路由标签、图 allowlist 和当前实例消费标志放在内存 PartyPool 中。E2 核实图绑定使用 FNV64，pool serializer 不包含 graph_bindings_ 或 consumed_，且没有完整 pool restore；uCMP key used 状态重新反序列化后不提供跨进程耐久 one-shot。它们是 TEST_ONLY 误用检查，不是抗碰撞认证、跨重启防重放、party 隔离或自适应 transcript 安全。

## 4. 正确性代数、接口数据流和未闭合组合

### 4.1 稳定 priority key 与输入 adapter

PROJECT_CONTRACT：score 是 32-bit 二补码 signed Q20.12；全序按分数降序、original_index 升序；rank 0 为最高优先级。AAV86 排序层需要一个严格全序，项目使用复合 priority key 表达“高分在前，同分小下标在前”。key 中的 original_index 分量属于项目扩展，不能写成 AAV86 或 Agarwal 论文编码。

当前 protocol_i_raw_score_input_party 接收本方 logical_n 个 raw score additive shares 及 party package，按 carry、sign 两个先后 exchange 阶段将输入转换为 padded_n 个 uint64 priority-key shares；在线计2轮。adapter的tie slot在hidden shuffle之前取原输入槽位；它将每方只应持有的raw shares转成key shares，不输出明文score或mask。此接口可以作为候选输入阶段，但其padding/ring适配是否能作为CA的逻辑域输入尚未证明。

current priority pipeline 的代码只为每槽创建记录 {key_share,0,0} 并前向 shuffle；original_index 没有作为独立 payload field 经过 record group。稳定 index 因为被编码进比较 key，能参与 fixed-clique 比较，但这仍不同于一个同时保护 key 与 index/payload 的 AAV86 record-routing 证明。AAV86 目标必须维持 stable component 与其 score 同 record，并在递归、桶、最终成员路由期间保留“该节点对应哪个原始位置”的秘密映射。

### 4.2 每轮 mask 与 uCMP 的代数关系

对某轮 i 的 endpoint a，设其稳定 key 为 K_i,a（b位环中的正确编码），两方持有加法份额 K_i,a^(0)、K_i,a^(1)，满足模2^b重构 K_i,a。T离线抽取该轮节点mask R_i,a，并分给双方份额 R_i,a^(0)、R_i,a^(1)，满足模2^b重构 R_i,a。对每个候选pair {a,b}，T用mask差

α_i,a,b=(R_i,a−R_i,b) mod 2^b

生成uCMP的DCF key pair，再分别把party-local key发给P0/P1。

在线双方需要先让每方发送本方逐节点的masked-key share：

Z_i,a^(p)=K_i,a^(p)+R_i,a^(p) mod 2^b。

两份相加并打开后得到 Z_i,a=K_i,a+R_i,a mod 2^b。活跃边 {a,b} 将同一对 masked operands Z_i,a、Z_i,b 输入对应 key。差值扣除 key 内隐含的 mask difference 后满足：

(Z_i,a−Z_i,b)−α_i,a,b = K_i,a−K_i,b mod 2^b。

若编码、符号/环绕边界与uCMP比较语义对整个可取key域均满足范围条件，则该比较恢复的是稳定priority key顺序位。protocol_i_ucmp 的 eval_strict_lt 返回两方分别持有的uint64 additive shares，重构值为0/1；它不是直接给两方一个公开明文compare bit，也不能仅凭接口把这些additive shares称作XOR shares。E1 n=5/b=36样本核验了oracle下真实uCMP比较，不等于所有n、padding和位宽均完成证明。

E1没有实现上述真实线上Z形成及开值。E1测试wrapper在单个进程直接拿到了全掩码、两方key和明文masked operands，然后调用真实uCMP key。测试中的z不是线上消息记录，不能用来声称生产协议具备每轮mask refresh/open。

### 4.3 从活跃边到 CA local ranks、bucket 与图

对单条canonical edge {a,b}，party-local比较输出为加法份额。假设priority keys在b位域内严格唯一，则反向顺序位可由1−[K_a<K_b]得到；各方按AAV86结构将相关边份额累加到endpoint local-rank / pivot-order / bucket-position shares。对同一活动子问题，pivot彼此比较以确定其顺序，non-pivot比较pivot以确定其所在桶；下一iteration只在相应桶内形成新的子问题和边图。

这里是目标算法的数据依赖，不是现有E1池代码的实现：E1 test wrapper对图列表逐边取key并验证活动比较，未实现AAV86 secure local-rank accumulator、pivot shares、masked input exchange、桶的安全更新或生产图builder。当前 protocol_i_cmpagg_eval_party 只接受固定点集的一整套C(n,2) edge material，并在全clique上遍历；它不是上述按G_i消费的AAV86 local-rank接口。

从revealed local ranks到公开pivot order、bucket labels、下一层图的具体映射，必须跟随论文CA结构并注明谁获得这些值。若两方均重构shuffled-domain local rank，双方也可能从rank、pivot和bucket sizes获得输入相关信息；公开active graph、节点数、边数、空桶和查表顺序也可能形成泄露。当前项目没有针对这些值的允许泄露清单/模拟器证明。不能把“图两方都能生成”当作零泄露事实，也不能在没有确认同步语义时把所有graph update记为零轮。

### 4.4 完整排序到原输入顺序 XOR bit-mask

AAV86候选完成的是全排序。目标项目需要安全地产生长度n的原序mask，且恰K项选中。逻辑上需要完成：

1. 对最终full order的每个逻辑节点确定其全局order rank；
2. 为rank<K的节点构造membership bit的双方shares，而不是公开rank/order或membership；
3. 将shuffled endpoint的membership shares逆映射到输入original_index；
4. 输出原序XOR shares，证明长度为n且XOR重构恰有K个1。

本项目fixed-clique pipeline的代码会重构整个shuffled-domain rank vector，对rank<K在party 0侧构造carrier，再使用独立reverse shuffle将bit carrier返回输入顺序。这是当前fixed-clique路径的具体代码行为；其公开rank、carrier表示和逆shuffle不自动兼容AAV86 CA的递归全排序。AAV86 CA的shuffled handle、递归子问题位置、原始输入位置和最终排序位置不是同一个index namespace。新组合没有同π的payload/key/correlation证明、没有安全membership-share生成方案，也没有output inverse-map API。因此不得复用现有pipeline输出描述来填补目标组合的这一阶段。

整个排序中不可公开full order、score、original_index或selected index，除非单独的目标安全定义证明该opening可接受；当前仓库没有这种批准。masked/open ranks的泄露则需在目标协议视图里单独说明，不能仅因分数未直接打开就视作没有输入相关泄露。

### 4.5 逐阶段证据矩阵

| 阶段 | 输入 → 输出 | 现有可复用事实 | 证据状态 / blocker |
| --- | --- | --- | --- |
| Q20.12 → stable priority shares | raw-score additive shares → padded stable-key additive shares | protocol_i_raw_score_input_party；两次stage exchange，输出padded key shares；priority tie slot在shuffle前编码 | 已有fixed input adapter；如何将N padded域映射到logical CA domain未闭合。 |
| 初始hidden shuffle | input record/key shares → shuffled shared records及可供CA使用的masked-list | Protocol I存在两轮forward SecretSharedShuffle和三轮C-INSTANTIATION core；三轮core输出masked record/rank fields | 各自是不同协议边界；没有证明目标shuffle与Agarwal §5.4的π、masked list、rank reveal construction等价。 |
| 每轮online remask/open | key share + round mask share → 双方共同可见Z_i | fixed pipeline有masked-key share send/open的一轮；E1无线上执行 | AAV86每轮mask shares如何传送/消费、mask fresh与对应整池key一致、打开边界和T静默均未闭合。 |
| 活跃边比较 / CA local rank | G_i、opened Z_i、party key subset → local rank shares | uCMP真实party-key接口；fixed CmpAgg有全clique accumulator；E1 active subset wrapper有n=5真key观察 | 缺production graph-subset API、masked-open传输与AAV86 local-rank正确性实现/证明。 |
| rank reveal / buckets / next G | rank shares → graph control state | Agarwal会议版CA给出相关论文结构的高层描述；E1 CSV保存图计数，不实现CA transcript | 谁看到哪些rank/桶/图及泄露尚未冻结；next graph同步因果和pivot randomness协议未闭合。 |
| final sort → rank<K → inverse map | final ranking shares → n-bit original-order XOR shares | fixed-clique pipeline有本路线自身的rank<K carrier与2轮reverse adapter | 无AAV86输出membership或同π逆映射；通信和轮数未知。 |

## 5. T、P0、P1 的完整视图及全池安全

### 5.1 T（trusted offline third party）

用户已确认的模型边界：T可以看见全部离线随机量、密钥、置换等离线材料，默认不与P0或P1任一方合谋。按全两两方案，T的离线视图至少包括：

- session、protocol version、n/K/r/域大小D、比较位宽、材料槽索引及交付元数据；
- 每轮每节点的完整mask R_i,a、每个端点mask差；
- 两份party-local DCF/uCMP keys，全部候选pair与iteration槽；
- hidden shuffle的生成随机性、置换或完整置换相关材料；
- 如果用padded domain，sentinel/逻辑节点对应表；
- 交付确认、超时/失败状态（具体是否会返回T尚未定义）。

T不获取原始score、P0/P1输入份额、online masked opens、CA rank/bucket/graph transcript、rank<K结果或输出shares；这些是为了避免T借助完整R/置换解除online opening掩码的必要边界。若T取得某轮Z_i,a且知道R_i,a，就可以直接计算对应K_i,a；若再获得rank transcript和shuffle map，输入顺序泄露会进一步增加。因此T在线静默必须是实际进程/网络边界，而非文档措辞。其材料在离线分发后是否擦除、保留多久以及错误/ACK是否回传T，当前尚未规定，记为UNPROVEN/open。用户指定的“不与在线方合谋”作为本阶段固定信任前提，不在此扩展恶意T方案。

E1的TEST_ONLY generator在单进程持有完整mask、party 0/1的全部材料和harness数据；它与上述独立T/P0/P1部署完全不同，未验证T退出、隔离或non-collusion。

### 5.2 P0 和 P1（online parties）

| 视图字段 | P0实际/目标视图 | P1实际/目标视图 | 安全边界 |
| --- | --- | --- | --- |
| 输入 | 自己的Q20.12 raw-score additive shares；不得获得对方输入份额 | 自己的Q20.12 raw-score additive shares | E1 harness在同一进程持有双方input shares/oracle，不是隔离证据。 |
| key与original position | 自己整池party-local keys、每个endpoint/iteration的mask shares和索引元数据；协议要保持stable-key与输入位置绑定 | 对称地持另一份材料 | 各方查看所有候选pair，而不只是活跃边，视图比单key查询更大；key之间有关联。 |
| shuffle | 自己的shuffle party material/相关permutation shares；不应从单方视图恢复完整π或original_index map | 对称地持有另一份shuffle material | 当前不同shuffle API的复合π关系未证明；不能推定任一方恰好不知道π。 |
| online opens | 若候选每轮masked-opening成立，双方重构本轮所有需参与图计算的Z值 | 同样看到这些openings | Z应与score/index无关联泄露的simulatable view；目前没有目标CA opening transcript证明。 |
| graph/query | 可见或本地算出的图、pivot顺序、bucket标签、active edge lookup/消费次序及报错 | 对称可见或本地算出 | 如果图/局部rank作为公开值，属于数据相关输出，须明确允许泄露；当前未冻结。 |
| 输出与失败 | 自己的最终XOR mask shares、输入/图/包不匹配和abort状态 | 对应share与状态 | 只能联合XOR恢复mask；错误行为、部分材料消费后abort、可重试语义未定义。 |

### 5.3 整池相关 key 和自适应查询

全两两方案能解决的只是offline availability：在线自适应图只要落在已知固定endpoint域内，本轮任意简单边{a,b}都能找到该pair/iteration槽。它不自动解决以下联合安全问题：

1. 同一轮多个edge key共用节点mask R_i,a，故α值在图上有关联；一方持有该轮全部party-local key，而不是只获得某条独立查询key。
2. 前轮rank/bucket结果确定下一轮图，活跃pair子集是自适应的；被消费槽和查询次序可能泄漏rank、bucket或pivot结构。
3. 证明须覆盖整套correlated DCF key与所有允许的adaptive query transcript，且模拟器视图包括离线接收的完整材料、online openings、revealed local ranks和错误行为。
4. T看见完整预处理信息但不合谋；普通两方单key DCF安全声明不能直接覆盖该2+1 view，更不能覆盖T能否接收online transcript。
5. 若查表顺序依赖私有状态，内存lookup pattern、时序、流量或错误差异也可能是party view的一部分；当前没有恒定访问或leakage contract。

当前材料没有针对这个full-pool party-view的模拟证明、归约或经审查theorem-to-code映射。这是独立NO-GO / UNPROVEN blocker，不因slot覆盖公式成立或E1比较正确而下降。

## 6. 消息因果图、轮数和成本

### 6.1 候选消息 DAG（只记录依赖，不宣布项目总轮数）

    离线阶段（T，在线输入未知）
      T生成score-adapter material、shuffle material、每轮节点mask shares、
      每iteration × 每无序candidate pair的两份DCF key
      ├─ P0只收P0-local package
      └─ P1只收P1-local package
      T不接收在线消息；离线分发耗时/字节另记，不算在线因果轮数

    在线阶段
      raw score shares
        → [score carry exchange] → [score sign exchange]             2轮，当前adapter
        → [兼容CA的初始hidden shuffle]                              R_shuffle未知
        → 对每个算法iteration i：
             P0、P1发送本方逐节点key-share + mask-share
             → 重构/打开Z_i（masked values）                       1轮/iteration
             → 两方本地对活跃E_i查表并评估DCF、累积local ranks      0轮
             → P0、P1交换rank shares并按目标泄露契约重构local ranks 1轮/iteration（候选）
             → 双方本地生成pivot/bucket/next graph                 0轮仅当共同随机数、
                                                                 确定性映射和状态完全一致；
                                                                 未证实，额外同步轮数R_graph_sync未知
        → full order转membership shares                            R_membership未知
        → 以对应hidden shuffle逆映射回original-order XOR mask       R_inverse未知
        → 输出shares / abort传播                                     R_output未知

计数前提是每一条双向exchange都必须有独立sender、receiver、payload、依赖和同步边界。候选CA iteration内，masked-open消息依赖当前stable-key shares与当轮mask shares；local-rank exchange依赖活跃边比较/累加完成；next graph依赖已得到的本轮bucket state。不能把API调用顺序或线程交错直接合并成网络因果轮数。

若上述两条逐轮同步都成立，CA iteration本身的项目候选消息数为2r；它还不包含已知的raw adapter、兼容shuffle、pivot coin同步（如需要）、Top-K membership、逆映射或输出转换。仅作为未闭合账本表达式，可写为：

    项目端到端在线轮数候选 = 2 + R_shuffle + 2r + R_graph_sync
                                + R_membership + R_inverse + R_output

其中 R_shuffle、R_graph_sync、R_membership、R_inverse、R_output 目前均未定义或未证明，故总数为NOT_PROVEN，不能把未知项填0。若图和pivot randomness由两方预先共享并能从共同revealed state无通信地确定，某些graph update可以本地执行；这一条件本身尚未落实为目标协议设计。

PAPER_DIRECT：Agarwal CCS’24 Theorem 5.1 对其 CA sort/select 给出2r+1的在线轮数界。PAPER_DERIVED：在论文相应构造下，可将其读作初始shuffle层加每轮masked-open与局部rank reveal两层同步依赖。该计数属于论文CA安全编译器模型，不能直接替代本项目的raw-score adapter、当前P-I shuffle、T全知离线视图或original-order mask adapter。项目fixed pipeline的2+6=8轮、M5三轮C-INSTANTIATION core和theorem的2r+1是三个不同计数对象；不将其相加、替换或压缩成M6A总轮数。

### 6.2 离线材料、在线打开和输出成本

| 资源 | 符号口径 | 状态 / 样本 |
| --- | --- | --- |
| 每轮每方候选key数 | C(D,2) | ANALYTICAL_BOUND；取决于逻辑/填充域D。 |
| 全协议每方party-local key数 | r·C(D,2) | ANALYTICAL_BOUND；两方合计2r·C(D,2)个party key blobs。 |
| 活跃比较调用 | A=Σ_i e_i；每方消费A个材料 | E1 n=5,r=2 fixture为e=[7,1]、A=8；全两方key eval物理评估成本不能外推。 |
| 节点masks | 每轮D个full R_i,a；T视图有rD个full masks；每方持有rD个additive mask shares | E1自定义envelope含每方8rD bytes的mask-share项。shuffle/CA额外随机性未计入。 |
| 实验key字节 | L_key(b)=57+24b bytes/key；pool envelope=63+8rD+r·C(D,2)·(20+L_key(b)) bytes/party | E2从E1 serializer源码复算；只代表当前TEST_ONLY serializer，不含传输framing、TLS、ACK、图binding/消费状态。 |
| 实测key / pool字节 | b=36时921 bytes/party key；n=5,r=2,D=5时18,963 bytes/party、双方37,926 bytes | E1_EXPERIMENT_OBSERVED；这是唯一当前物理字节样本。 |
| online masked opens | 每轮每方发送D个b-bit masked-key contributions；r轮为每方rDb bits，双方总逻辑send量2rDb bits | ANALYTICAL_BOUND，假设所有域节点每轮都打开。真实CA是否打开全部endpoint及编码/包头均未冻结。 |
| rank-share消息 | 令b_rank=ceil(log2 D)或实现证明所需更宽位宽；候选为每方rD·b_rank bits，双方总逻辑send量2rD·b_rank | ANALYTICAL_BOUND条件式；local-rank总域、复合范围和发送编码未闭合。 |
| graph/pivot/bucket控制信息 | 记每轮实际传输为B_graph,i bytes、可能同步轮数R_graph_sync | NOT_MEASURED/UNRESOLVED；若由已公开rank及共同随机带本地导出，可无显式payload，但泄露仍在视图内。 |
| inverse-map/output | 记B_inverse、R_inverse、B_output；最终结果每方至少表达逻辑n个bit shares | NOT_MEASURED/UNRESOLVED；mask share表示/转换和同一π逆映射接口未定。 |
| CPU / RSS / network / E2E | offline keygen+serialize、分发、存储、online critical path和total cost分开 | 除n=5,b=36的E1两次本地计时外均NOT_MEASURED。正式性能验收必须分别报offline、online、total，不能只报online。 |

E1对n=5、r=2、b=36报告两次全池生成/序列化0.240075 ms、0.138949 ms；另报告8条活跃边单进程wrapper+uCMP 0.065826 ms、0.073332 ms。E2未重跑harness，并指出没有原始stdout/log artifact。以上保留为有限TEST_ONLY报告值，不求平均，不外推到网络或生产。其他位宽下key bytes、RSS、pool实际分配、网络传输、在线CA及full-sort→mask总延迟均NOT_MEASURED。大n的PB序列化长度是解析容量估算，不是分配/RSS实测。

## 7. 论文、实验、源码与目标组合对照

| 对象 | 功能 / 输入输出 | 图何时生成 | mask/key绑定及视图 | 轮数 | 成本与实现状态 | 证据级别 |
| --- | --- | --- | --- | --- | --- | --- |
| AAV86 §3.1 / Thm 3.1 | 随机并行比较排序；元素全序→完整排序 | 由pivot和递归桶逐轮产生；活跃边自适应 | 原论文是明文比较算法，无项目Q20.12 shares、T、FSS pool、mask output | 固定算法round count参数不等于消息同步轮数 | 期望比较复杂度界；不提供本项目全两两party pool容量或mask适配 | PAPER_DIRECT |
| Agarwal CCS’24 CA / Theorem 5.1 | 将比较图算法表达为CA安全sort/select | CA iteration中结合masked input与FSS/local-rank；图状态根据算法数据依赖更新 | 会议版给CA/2+1角色模型；本项目T全知离线材料、shuffle view、party pool correlation和完整mask边界须单独对齐 | 定理对论文construction给2r+1；不等于项目总轮数 | 按论文图边/顶点等模型量；会议版未给足本项目自适应全两两发材/输出逆映射工厂 | PAPER_DIRECT + 需边界映射 |
| E1 all-pairs wrapper | TEST_ONLY为n=5,r=2所有候选pair生成真实party-local DCF key；fixture后消费每轮活跃边 | key池先生成；随后加载图并由wrapper绑定/查表 | 同进程控制器看全mask、两方key与测试oracle；标签在wrapper；不作安全隔离 | 不实现线上masked open、rank reveal或CA轮次 | 20槽/方；实际两方各消费8槽；921 bytes/key、18,963 bytes/方，仅一组位宽/域的物理样本 | E1_EXPERIMENT_OBSERVED |
| 目标Protocol I+AAV86 | raw Q20.12 shares→stable full sort→原顺序n位、恰K个1的XOR shares | AAV86每轮adaptive G_i；T必须在输入前制备每轮全pair池 | 目标T/P0/P1 view尚无完整安全模拟；stable key、handle与original index逆映射未闭合 | CA每轮masked open+local-rank reveal是候选因果层；项目总轮数未知 | 每方r·C(D,2)池；生产runtime未实现，在线/offline/total性能未测 | PROJECT_CONTRACT + PROJECT_EXTENSION + UNPROVEN |

AAV86论文原文包含的是排序算法；稳定项目key、三方材料、adaptive FSS pool、Top-K membership和原序XOR mask均属项目组合扩展。Agarwal会议版的CA模型/定理不是对当前VFSS接口的自动证明。E1仅证明材料候选域小样本可用；不证明protocol integration、安全、Theorem 5.1成本匹配或CA rank开值边界。

## 8. E1/E2 的精确 errata 与证据范围

本节只补充历史边界；不覆盖或修改E1/E2报告与产物。

1. snapshot oracle：E1 的 graph_oracle_edges 接收同一 AAV86GraphCounter 预先生成的 snapshots，再穷举snapshot上的pair并检查 pivot-edge predicate。精确措辞应为“snapshot条件下的边枚举oracle”，不是独立验证pivot抽样和完整递归。E2 对矩阵中 n≤32 的96个run另行重建递归并枚举了336条逐轮记录，补强小样本范围；大n未在E2重新生成。
2. same_round_recursive_edges：Python dataclass将其默认成0且没有后续赋值。E2指出它是结构上的预设零值，不是实测计数；应读作“当前递归结构不在同一轮比较子桶边”，不能当测量指标。
3. empty_buckets：统计实际递归访问到的空子问题，不含终止层创建但不再递归的空桶；不影响边数与pool容量，但不等于所有迭代创建的空桶数。
4. 重复检查字段：CSV的ENUMERATED_0 / PROVED_BY_PIVOT_REMOVAL是检查方法或结构论证标签，并非每一条矩阵记录都执行了逐run重复扫描。E2的小规模独立枚举支持n≤32样本；大规模结构唯一性仍靠pivot removal论证。
5. 大n / timing：E2验证了全部546行/156个run的CSV算术关系及n≤32样本递归；没有重新生成大于32的随机图。E1两组timing没有配套stdout/log artifact，E2没有重跑二进制。大n逐轮边数仍按E1样本观察，非E2复现或统计尾界。
6. key与graph binding：精确说法是TEST_ONLY wrapper以session/iteration/endpoints/party/bits/FNV64图标签/内存allowlist做请求路由和拒绝检查；底层DCF key不含这些认证字段。FNV64不是密码学图认证；pool序列化未包含图binding和consumed状态；one-shot只在当前内存PartyPool实例中成立。
7. all-pairs材料可行性的限缩：n=5 fixture真实比较keys覆盖其候选域且只消费当前活跃边，证明功能可用性的样本事实；大域任意边覆盖来自槽位枚举公式，不是端到端自适应查询安全性、各位宽conformance或生产package证据。E1不执行完整AAV86 secure sort或Top-K mask。

## 9. 设计 blocker 与需补证事实

进入后续实现设计前至少要闭合以下blocker；任何一项缺失都不能将本稿的材料覆盖PASS扩展为安全运行时批准：

1. domain/padding contract：逐输入规模确定CA节点域是n还是N；说明n=1、非二次幂、全最小分、空桶/单例子问题和sentinel，给出stable key位宽/ring范围证明。
2. offline key package construction：定义T生成、party-local交付、pool验证、每轮mask share一致性、全pair key的准确index schema；证明不同iteration独立/域分离。补齐从离线发材到secure erase/retention/failed delivery/ACK的生命周期。
3. 真实每轮masked opening：明确sender、receiver、消息字段、node mask share的装载与消耗、Z的开值范围，以及为何打开这些Z不泄露score/index；证明T不收到opening或party transcript。
4. CA rank/bucket semantics：逐AAV86算法步骤列出edge compare shares如何累加为pivot order/nonpivot bucket ranks、local ranks何时揭示、是否公开endpoint handle/子问题大小/空桶/active edge count、谁依赖哪些字段生成下一图；提供论文具体页段到项目映射与泄露证明。
5. full-pool party-view security：对每个单方（含其整池相关key）构造可审查模拟/归约，覆盖共享节点mask造成的跨edge key相关性、自适应edge查询、查表访问pattern、失败和重放。单key DCF安全不能替代本项。
6. compatible hidden shuffle：给出同一个π如何作用到key、稳定下标信息及CA所需masked-list；对现有两轮SecretSharedShuffle、三轮C-INSTANTIATION及目标CA shuffle分别列清功能、相关性、打开值、单方view；没有证明前不得复用计数。
7. final original-order mask：定义full order到rank<K membership XOR shares，再到原始位置的无泄露逆映射；包含mask share转换、output/error消息和逐轮材料消费；不公开完整排序、score或original_index。
8. message and resource evidence：用sender/receiver/dependency transcript重算在线轮数；分别实测离线生成与发放、存储/RSS、在线关键路径和总成本、双方logical/wire communication，附revision、seed、环境、重复次数与原始计数。缺数写NOT_MEASURED。

建议下一阶段先由论文/协议owner提供可逐页核查的CA rank揭示与shuffle构造出处，再由安全审查给出目标单方view/泄露模拟要求；接口/输出adapter必须在任何secure runtime代码之前闭合。此处不提出或冻结新的AAV86算法版本、stable tie编码变化、CA协议变体或runtime API。

## 10. 资料与源码核验登记

以下SHA-256均在E3审查时针对当前只读来源计算。主工作区的文档有部分是本地dirty或untracked内容；其hash表示当前文件，不表示它们已提交到c3926c68。实验材料为E1/E2 worktree中已存在的报告/隔离实验文件。论文PDF正文只作为本地只读来源，没有加入Git差异。

### 10.1 主工作区项目文档

| 路径（相对主工作区） | 章节/用途 | SHA-256 |
| --- | --- | --- |
| PROJECT.md | 项目范围和冻结契约；主工作区tracked modified | 33B66C6BD6CCE8FCEAB817FB68C17BEF2A4E58194A39B41AA4D48AD79D2C05D5 |
| docs/IMPLEMENTATION_PLAN.md | M5关闭、M6A目标和里程碑；tracked modified | F5C2D1DCD6587D763EFBD8DBF3D6CF8F9A7014EDB93145A7678ADF6876DD49E8 |
| docs/BENCHMARK_VALIDATION_PLAN.md | V3计量边界与offline/online/total；base文档 | DBFAA8A850541A4A8BFA5DAE708804CF2DB4C44F82A9D4E3402E87AB10198756 |
| docs/decisions/M1_SCORE_SEMANTICS.md | 全文：Q20.12、signed映射、tie和mask语义 | 5920A00EAA41CC04712E0338F82305EE255730E944AB38293E9C65C70FBA426D |
| docs/decisions/M6A_AAV86_ALGORITHM_AND_STABLE_ORDER_DECISION_2026-09-27.md | §§5–10；AAV86、稳定key、Protocol I接口与轮数边界；untracked | 21DE1BE89CFCC7B66005B8E4F34BCAFFBC7556CCF22FAE7F517FEC91681456C0 |
| docs/decisions/M6A_PROTOCOL_I_AAV86_PREPROCESSING_AND_SECURITY_DESIGN_2026-09-28.md | §§4–10；T/P0/P1、mask、CA DAG、P2 gate；untracked | C19DF3632C411BF7BD8DEFC6C003CCDC7FC90963D6C46C871FE12D91AD551BDC |
| docs/handoffs/M6A_NEW_CHAT_PROMPT_2026-09-25.md | §§M5 closeout、Assets and entrypoints、M6A target | A7535B18A68FBF464135ED85814BB3FDCFE22F11AD4241512DE9A1FF753ED440 |
| docs/reviews/M5_TO_M6A_G3_RECEIVER_ACCEPTANCE_2026-09-25.md | G3接受结论、准确revision和证据限制 | C3598C62EDC0D431E5C1159A979683602157175050963F1E3F2F20A905562DD6 |
| docs/reproduction/M6A_BASELINE_AND_SOURCE_INVENTORY_2026-09-27.md | §§1–4：c392基线、M5 G3、当前入口；untracked | B637F31009F329391D1CC92CC959E251E64A0AAC45C5FBB96CFEE0D471B93EFE |

### 10.2 Protocol I 代码入口（主工作区静态读取）

| 路径 | 核验对象 | SHA-256 |
| --- | --- | --- |
| VFSS/include/moe_topk/protocol_i_score_input.h | raw-score adapter配置、2轮metrics及函数签名 | 8103866D657721F604DE3CB7A475824830581FE1D64A243A21B17972C6CEB0D1 |
| VFSS/src/moe_topk/protocol_i_score_input.cpp | carry/sign两阶段、padding和stable priority输出 | 282D44D2632D92BD22D70FF64495FF7B65630D1CC15ACCB959A1E9D0642E9AD8 |
| VFSS/include/moe_topk/protocol_i_priority_key.h | priority key项目编码 | 8C345675877FDD101E0B5918250FF7E7B6879031161CD38B7FE141CE96D0E338 |
| VFSS/include/moe_topk/score_semantics.h | 32-bit signed ordering helper | DF9E2400429432FC0BE998FE6ADCA3BB774D98BE44EAE97535E0B463DA6C9801 |
| VFSS/include/moe_topk/protocol_i_pipeline.h | fixed pipeline输入输出、轮数counter | 4871BD423ABD71758661B6668249D4D1268EFA240C8894658F804DC411DCAC6F |
| VFSS/src/moe_topk/protocol_i_pipeline.cpp | padded clique、masked open、rank reveal、rank<K carrier和reverse路径 | 0FD331AD19637415CD83307DE96AE6DD3C4D3DE21D0DD34858B5143847E94D06 |
| VFSS/include/moe_topk/protocol_i_parallel_shuffle.h | 三轮C-INSTANTIATION core输入输出 | A88FFF48B9B2F1F423EFDAECEA9B8F89C01F25C91BFE1A0AEA4F358604DA672D |
| VFSS/src/moe_topk/protocol_i_parallel_shuffle.cpp | dealer full mask/key factory、三轮masked/rank输出 | 8D0E488B6348CAA6BC55D8F6652198E97FC6ED69381D5B367091D71B77DCD75D |
| VFSS/include/moe_topk/protocol_i_secret_shared_shuffle.h | 两轮forward/两轮reverse候选shuffle接口 | 7C739B2B7DE07791AE746C1E5B942C267F332D7058F465FBE9CBD04A1EDCF197 |
| VFSS/src/moe_topk/protocol_i_secret_shared_shuffle.cpp | 上述shuffle实现边界 | 4B83CED81802CD1C1EECD484C96FA1866F8601131D052C34A672A8223C6121C |
| VFSS/include/moe_topk/protocol_i_ucmp.h | primitive interface和party material | 7E444F0188AFA2FAF0CE8A1FA97AF7D821C54FE82FB46CEFD1999AEA4C798D33 |
| VFSS/src/moe_topk/protocol_i_ucmp.cpp | alpha、DCF keygen和strict-LT share实现 | B1CC25814E772E0B62E71CA97465FECB16B9010F896D062BEA591E899743FB23 |
| VFSS/include/moe_topk/protocol_i_cmpagg.h | CmpAgg接口 | D50B7B84A3EEF299D1F1C23A4D342C7F95003C28DD0F72CD8BD56FD0E3878390 |
| VFSS/src/moe_topk/protocol_i_cmpagg.cpp | 固定全对边数校验与rank累加 | 5183E371ADC7D06DF6E93097921B2C303B1AC5A8C24B883CA2330855521FCA4F |
| VFSS/include/moe_topk/protocol_i_party_package.h | package字段与edge/carry/sign material结构 | 9258F0F141E8D7F0346559458D2B280255633AE5BECBA41A00BCFE0D83867AA9 |
| VFSS/include/moe_topk/protocol_i_permutation.h | permutation namespace与apply语义 | DE94DC9DC6FECB1C1D0E62EF09469462564D7FC8BE62C993BBAB62D8BD5D3E2F |

### 10.3 论文与E1/E2隔离材料

| 路径 | 页/节和用途 | SHA-256 |
| --- | --- | --- |
| Papers/Alon_Azar_Vishkin_1986_Tight_Complexity_Bounds_for_Parallel_Comparison_Sorting_FOCS_AuthorHosted.pdf | Author-hosted [PDF](https://web.math.princeton.edu/~nalon/PDFS/Publications2/Tight%20complexity%20bounds%20for%20parallel%20comparison%20sorting.pdf), [IEEE DOI](https://doi.org/10.1109/SFCS.1986.57); 9 pages; Sec. 3.1/Theorem 3.1, printed pp. 506-507 / PDF pp. 5-6 | 322F1BD761A987FD09E6B59B3A3AE77D6E4B1CA9DCB1C2765E45AC4A2A7E2B83 |
| Papers/Agarwal 等 - 2024 - Secure Sorting and Selection via Function Secret Sharing.pdf | CCS 2024 conference version; [MIT DSpace](https://dspace.mit.edu/entities/publication/d70bd196-cf7c-46e1-a0a9-e3dfd08f379b), [ACM DOI](https://doi.org/10.1145/3658644.3690359); 15 pages; Sec. 5.1-5.4/Theorem 5.1, printed pp. 3033-3035 / PDF pp. 11-13 | 18FAF63EAA7923EEF715A6EB9D5D526FE04DCB69700B133C3E94DE935F68C01C |
| docs/reproduction/M6A_P2_I_ALL_PAIRS_MATERIAL_EXPERIMENT_2026-09-30.md | E1隔离实验报告，包含n=5/r=2物理样本与限定 | 0F54A121DC2CFFA70719C735F577C66022C2ED019706415CBC5D9A87347537A4 |
| docs/reviews/M6A_P2_I_E1_INDEPENDENT_AUDIT_2026-10-02.md | E2独立审计、errata、n≤32重建范围 | 307B80ABC789F2C1EB0614992A4E2C4938A7AB51BB3BCB089951D713C6C23ED9 |
| experiments/m6a_p2_i_allpairs/aav86_graph_counter.py | TEST_ONLY图统计逻辑与字段实现 | 09DCC72DF6BC68C83046CBB3F7C9AF8B13642BEA720036B92804F6BE5423B402 |
| experiments/m6a_p2_i_allpairs/aav86_graph_counts.csv | 156 runs/546行保存数据；E2对数值算术审计 | 5346AE011201C1F88F3B7199BB86DE7C1B4F761151F3FB269C86F9E99AF9F622 |
| experiments/m6a_p2_i_allpairs/aav86_n5_r2_test_edges.csv | n=5/r=2 e=[7,1]活跃edge fixture | 87A660C55B47FBCD1AAAE6C4E025798E8E4B6853968DC6D27DF635A704B46D78 |
| experiments/m6a_p2_i_allpairs/material_pool_test.cpp | TEST_ONLY party pool wrapper、mask与key生成/消费 | 8F6F36C5408A55A09CB9754C7076C2C27EADD0DDD539B7D5297949A9DEF0E159 |
| experiments/m6a_p2_i_allpairs/CMakeLists.txt | 隔离实验target配置 | B99BCA22E068E462FB222E83AD5A79BE9E4A92E79447E46EB6880BD317C77195 |

AAV86本地PDF用于§3.1原文页段；Agarwal只登记本地CCS’24会议版，未据此宣称获得或核验完整作者稿。纸面结论限于上述PDF和引用页。资料哈希不代表论文正文已加入Git。

## 11. 最终判定与执行记录

- OPTION1 ALL-PAIRS MATERIAL AVAILABILITY：PASS_WITH_FINDINGS，仅对固定域、iteration隔离槽位的功能覆盖及其容量公式成立；不是生产材料包、安全证明或实际大域物理容量。
- PROTOCOL I+AAV86 COMPOSITION DESIGN：NO-GO；仍缺CA图/局部rank到下一图的具体接口及泄露说明、真实每轮mask/open消息、稳定record/同一π证明、full-sort到原序mask逆映射和闭合的消息轮数。
- FULL-POOL PARTY-VIEW SECURITY：NO-GO / UNPROVEN；缺少整池相关key和adaptive query的单方view模拟及T隔离证明。
- P2-I SECURE RUNTIME：NO-GO。
- 固定 M：仍为延期备选；本稿没有审查、实现或建议转向它。
- Protocol III：延期；本报告没有把Protocol I结论外推到Protocol III或其2r目标。
- 要求学姐/团队补充的具体事实/来源见第9节；优先提供CA local-rank揭示与shuffle构造出处、兼容Protocol I初始shuffle及同π论证、T/P0/P1完整view与泄露证明、final mask inverse-map出处。
- 本轮只静态阅读并新增本文；没有运行测试或生产benchmark，没有修改生产代码、测试、VFSS-baseline、论文、参考树或基线目录，没有修改E1/E2报告，没有暂存、提交、推送、合并、创建PR或切换分支。

本文创建前主工作区dirty状态与本节1.1所列完全相同；实验worktree原有E1/E2/experiments未跟踪内容保持原样，仅追加本设计稿。
