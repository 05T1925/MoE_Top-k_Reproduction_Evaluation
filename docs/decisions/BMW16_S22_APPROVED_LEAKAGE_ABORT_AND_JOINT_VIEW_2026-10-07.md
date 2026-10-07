# S22：批准的公开信息、统一算法中止与联合视图审查

日期：2026-10-07
基础源码：S21 `c231329f8c952b80df5cee201b056b461b5701b0`
候选身份：Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL

## 决策范围

本决策记录本轮用户转述的学姐答复和据此实施的 S22 修正。学姐答复只批准在 P0/P1 之间公开下表字段；它不是密码学安全证明，也不把 S21 历史上“尚未获批准”的判断追认为当时已批准。S21 原文保留，S21 的时间线只由本决策和验证报告追加说明。

### 公开字段合同

| 字段 | 接收者与产生时机 | 代码承载位置 | 错误路径与边界 |
|---|---|---|---|
| 会话、n、K、参数指纹、实现标签、通道/阶段 | T、P0、P1；发材和在线握手 | S20 bundle manifest；S21 TLS channel binding | 配置或身份不匹配时拒绝启动；此字段不是秘密输入 |
| 置换后比较端点、逐条比较结果、pivot 位置、U/V/W 与抽样位置 | P0、P1；对应 Select 层结束后 | 比较帧中双方 Eval share 的逐项和；任务状态按打开 bit 确定 | 正常算法失败统一状态；通信中断仅存活方 LOCAL_ONLY |
| masked operands / public_z | P0、P1；raw-score 转换及 forward/inverse shuffle 各阶段 | raw-score carry/sign 两个帧中实际交换的 masked word 向量；shuffle round 1/2 的三 word 记录帧及 round-2 两方和向量 | 将实际帧值作为获准的 masked operands；禁止公开原始分数或原始索引到 shuffle 位置映射 |
| 被选中的置换后位置/匿名 handle | P0、P1；两个 Partition 交集可计算时 | 由双方已有公开比较 transcript 本地导出，不另发原索引 | 不是原始位置；不得解码或记录对应 original_index |
| 实际读取的 key ID/slot ID、次序、比较次数、通信长度 | P0、P1；每个材料读取/每一在线帧 | manifest slot、两方帧顺序和长度 | ID 仅编码 session、用途、task、round、置换后规范端点；不得含原始索引或分数；获准公开不代表允许复用 key |
| 统一算法中止状态 | P0、P1；最终 agreement 帧 | final agreement 的 0=SUCCESS、1=正常概率 ABORT、2=工程/不变量错误 | 1 才是正常算法失败；2 保留工程故障与退出码 70，不得吞成概率失败 |
| 原输入顺序的各自 mask share | 各方仅本地；成功返回时 | party API 输出文件/调用方 | 仅本方份额；不得向对端发送完整明文 mask |

以下字段明确禁止公开：原始 signed Q20.12 分数、original_index、原始输入位置与匿名 handle 的映射、selected original_index、完整明文 mask。T 只看公开配置及离线材料，不接收在线输入、排名、比较 transcript 或输出。

以上是“允许公开”的合同；它没有自动说明这些公开值的联合分布满足半诚实模拟安全。是否符合项目最终隐私目标还需所有者审查。尤其比较结果和自适应轨迹可让双方推断某些失败大致发生于哪个阶段，虽 final 状态只区分成功/概率失败/工程错误，不能宣称失败原因在统计上不可区分。

## key ID 与材料新鲜性

S21 的匿名 pair slot 用规范化的 shuffled handle `lo < hi` 编号。令 C=n(n−1)/2、base=2·padded_n+2、pair_index 为字典序无序对索引：

- Select：`base + (task·4 + round−1)·C + pair_index`，task∈{0,1}、round∈{1,2,3,4}；
- membership：`base + 8·C + pair_index`；
- raw carry/sign slot 是 party 本地原输入数组 slot；它不是在线公开的 shuffled endpoint/key ID；
- forward/inverse shuffle 使用各自独立的材料 ID 与固定记录材料，不编码原始 score/index。

每个 Select task/round 及 membership 都独立 KeyGen。只在同一 task、同一 round 对同一无序端点的重复逻辑请求使用轮内缓存；不会跨轮、跨 task 或跨 membership 缓存/复用 key。规范方向的同一 key 支持本地反向 share 转换，避免对同一个数学边再次 Eval。上述检查来自 `find_slot_id`、`compare_round` 的 cache key 及材料生成循环；它们证明 ID 的代码形状和新鲜性规则，不证明整池 key 隐私。

## 算法状态与一致中止合同

最终 agreement 使用三值协议状态：0=SUCCESS，1=ABORT_ALGORITHM_PROBABILITY，2=ABORT_ALGORITHM_INVALID。双方都得到 1 时返回 PEER_AGREED、无 mask、party-node 退出码 10。以下是算法允许的正常概率分支：R2 目标未被 pivot bracket、U 超上限、R3 的 W 超上限；任何一方报告概率失败都会在 final agreement 归一为 1。受控本地 reason 可记录细节，但不进入协议级状态码。

rank 不在有效范围、R1 样本 rank 不唯一、R3 rank invariant 失败、R4 总序/划分基数失败、两 Partition 交集不是唯一元素、状态码不一致都属于工程/不变量错误。代码返回 ABORT_ALGORITHM_INVALID、清除 mask、记录 `engineering_failure=true`，party-node 退出码 70。码 2 不能按正常抽样失败计数。材料错误仍是 20；断连/静默是 30 与 LOCAL_ONLY，除非 final agreement 的对端确认已实际收到。任何非 SUCCESS 分支不留下 mask 文件。

这保证双方在同一连接上成功交换 final agreement 且各自状态为 1 时统一中止；它不保证网络分区情况下双方都知道对方状态。已公开的边、结果和位置仍可能泄露大致失败阶段。

## TLS fail-closed 修正

增加显式传输模式：旧接口默认 CallerOwnedFd，保留调用方明确选择的裸 FD 旧协议行为；要求认证流的 BMW16/TLS 路径使用 RequireAuthenticatedStream。构造 framed channel 前若 TLS stream 未注册、已被消费、注册竞态失败，立即抛 ProtocolITransportError，不进行裸 FD read/write。相反，已注册 TLS stream 却选 CallerOwnedFd 也被拒绝。channel 只销毁/关闭它实际拥有的 stream；注册前失败时 FD 仍归 caller，防止双重关闭。

本轮将严格模式传播到 raw-score adapter、forward/inverse shuffle、Select 四层、sampling coin 和 final agreement。定向测试覆盖缺流、已领取/已消费、错误模式和正常旧 FD 路径。安全性质是“指定认证流缺失即失败”，不是所有 caller FD 本身都认证。

## 联合单方视图与归约尝试

腐化方 P0 或 P1 的完整实际 view 包含：本方 raw share；本方 forward/inverse shuffle 的 sigma/tau/a/e/r_share 与全部发送/接收消息；本方 sidecar 中已使用和未使用的完整 key；由共享节点 mask 差得到的相关阈值；每次本地 DCF Eval share；public_z 和 opened comparison bits；两 task 共用的匿名句柄、全部自适应边、pivot/U/V/W、sample、slot ID/读取顺序；selected anonymous handle；final status、输出 share、TLS 长度/终止状态。本方看不到对方原始输入份额或对方 sidecar（依赖各方文件/凭据隔离）；T 在线静默且不与 party 合谋。

### H0→H1：随机流

生产 DCF KeyGen 的根 seed 在 `seed_fss_once()` 中由 OS `getrandom` 取 128 bit，写入 256 个 thread-local cryptoTools PRNG。每个 `keyGenDCF` 调用取两块 root-stream 输出，并由 AES 四块扩展产生每一深度的两个 seed/value 分支。把这些 PRG stream 替换为同长度独立均匀随机 tape，损失为 AES/cryptoTools PRG 的区分优势乘以实际扩展查询数；本轮没有测出全路径 DCF 节点扩展总量，故定量优势参数记作 `Adv_PRG(Q)`、Q=`NOT_MEASURED`。采样 PRF words 使用另一字段，不能与 FSS 节点扩展相加后声称精确安全参数。OS 随机源失效时实现应失败，不回退固定 seed。

随机采样的 abort 界另行处理。在线两方各发一段 32-byte `getrandom` contribution 并 XOR；双方都知道该 combined seed。之后当前 sampler 用 `SHA256(seed || task || domain || counter)` 生成任意多 word，再用 rejection 形成有界整数。S16 的 union bound 是对理想独立均匀无放回抽样的数学界；当前 seed 是公开的，故不能把它称为“秘密密钥 PRF”，也不能直接把标准 PRF advantage 加到理想界来宣称严格保证。当前实际分布需要额外的随机 oracle/random-expander 假设或独立统计/可证明采样构造；不受本次功能样本零 abort 证明。理想概率界、OS 熵模型和哈希扩展假设分栏记录。

### H1→H2：可审查的条件性多 key 归约

固定任一 party c 和整次公开 transcript L。FSS 单 key game 必须对实际 serialized party key `(k,g,v)` 的完整分布成立，并允许攻击者在提交挑战函数前选择任意辅助 state。对 N 个 DCF slots 按固定顺序定义 hybrids H_j：前 j 个本方 slot 使用阈值 0 的比较 DCF key，剩余 slot 使用真实阈值 α_i；所有 α_i 来自同一批节点 mask 的相关差，其他槽与本地 shuffle 辅助状态在该 hybrid 中作为辅助 state 生成。对第 j 与 j+1 个相邻 hybrid，归约者知道真实 α 向量，独立生成所有其它 slots（它们可任意依赖 α_j），把 real 函数 `x<α_j` 与 dummy 函数 `x<0` 交给单 key FSS challenger，并将 challenger 返回的腐化方完整 key 用于本地 Eval。FSS key privacy 的 game-state 能包含其余完整 pool、本地随机 shuffle 状态和相关 α 的任意函数；要求 challenge key 的随机性在给定这些量后独立。

每条比较的本地 share 是 key 的确定函数（uCMP 对同一 key 在 x/y 两个公开 masked operands Eval，再按源码做 Z/(2^64) 线性组合）。泄露 L 给出真实 opened bit b，腐化方收到的 peer share 在实际 transcript 里就是 `b−s mod 2^64`，其中 s 是本方 share；所以 hybrid simulator 用当前 challenge key 算 s，并直接构造 peer share 为 `b−s`，得到相同的 opened bit。边、下一层 pivot/U/V/W、slot 顺序均为已给定 L 的确定后处理。未访问 keys 仍在完整 key view 中，由对应 key hybrid 逐个处理。每个 pair key 在两个 task/四层/membership 的用途是不同的独立 KeyGen slot；同一逻辑重复边仅轮内缓存，不产生额外查询。

在 BMW16 stream store 的当前材料格式中，DCF key 槽数是 N=`2·padded_n + 9·C(n,2)`：每个 padded slot 各有一个 carry 和 sign key，再加 8 个 Select pair pool 与 1 个 membership pool；shuffle-only forward/inverse 保存记录/置换材料，不另含 CmpAgg DCF key。KeyGen 顺序固定，slot 间不复用 root-stream 字节；S22 不要求每槽额外 domain separation。

因此在“实际 `keyGenDCF/evalDCF` ABI 对应一个输出群为 Z/(2^64)、域宽 34..53 的标准二方 comparison FSS，完整 party key 满足函数隐私；PRG 随机带可替换；T/两方发材可信且无篡改”的条件下，H1→H2 有标准多项式混合，优势至多 N·ε_DCF + Adv_ROOT_PRG(Q)。外部原语来源可对照 Boyle–Gilboa–Ishai, EUROCRYPT 2015, Definition 2/Theorem 6；该定理定义的单 key function privacy覆盖完整 key，Eval 是确定后处理。该引用不自动证明仓库 `dcf.cpp` 与定理构造逐字同构。需由独立密码学审查者确认根扩展、位序、负号、纠错字及 `g/v` 序列化确实实现同一 DCF family；DCF construction 自身在该假设下的 PRG 损失计入 ε_DCF，根 AES-CTR tape 替换另计 `Adv_ROOT_PRG(Q)`，Q 目前 NOT_MEASURED。未获得对应关系接收前，多 key 门为 CONDITIONAL，不记无条件安全 PASS。

### H2→H3：forward/inverse shuffle 与输出 share 的条件模拟

以实际 dealer/party 方程为准。对单个 shuffle，输入 share 为 x0,x1，消息/状态为：

- m0=σ0(x0)+a0，m1=σ1(x1)+a1；
- τ0=π∘σ1⁻¹，τ1=π∘σ0⁻¹；e0=−τ0(a1)−h，e1=−τ1(a0)+h；
- y0=τ0(m1)+e0=π(x1)−h，y1=τ1(m0)+e1=π(x0)+h；
- d0=y0+r0，d1=y1+r1，z=d0+d1=π(x0+x1)+r0+r1。

腐化 P0 的本地置换 `(σ0,τ0)` 条件于 π 是独立均匀置换对：σ0 均匀独立，σ1 均匀使 τ0=π∘σ1⁻¹ 对每个固定 π 均匀；因此联合分布不依赖 π。P1 对称。逆向 shuffle 独立生成置换因子、使用 π⁻¹，同一论证说明腐化方的逆向局部置换因子边际不含 π；在给定 π 下 forward/inverse 的 dealer 随机 tape 独立，所以本方两套局部因子联合也不依赖 π。不能只凭这个边际结论结束证明，下面还要条件化所有消息和输出 share。

对一方 c，给定其输入 share、L 中双方 round-1/round-2 masked frame payload 与本方局部 shuffle share 状态，模拟本地状态可按条件分布直接采样：抽 σc、τc 均匀；由本方 round-1 message `mc` 解出 `ac=mc−σc(xc)`；抽本地 e_c 均匀，并由本方 round-2 message `dc` 设 `r_c=dc−τc(m_peer)−e_c`。真实生成中 a/e/r 的独立均匀性给出同一条件分布；已知 peer m 和 d 后，两个加法等式就是全部约束。计算 yc=τc(m_peer)+e_c 与 public masked vector 即重现本地状态。另一方的 frame 已属于 L，不需要知道对方输入 share、σ 或 π。

inverse 的 membership 输入 share vc 由 H1→H2 模拟器给出。输出本方 XOR bit share `s_out` 是 inverse 本地 `yc.word0` 的最低位。给定 L 的 inverse 两轮消息及理想功能给的 s_out，模拟器从记录环中均匀抽取满足 `LSB(yc.word0)=s_out` 的 yc，置 `e_c=yc−τc(m_peer)`、`r_c=d_c−yc`；这与真实均匀独立 e/r 在已知 round-2 frame 和输出 share 条件下的分布相同。word1/word2 的内部 share 按全环均匀抽取。这样同时重现本地 inverse 状态与原序输出 share，不解码 π、原始 index 或完整明文 mask。inverse round-2 公共 masked vector 和输出 share 一并进入 L/理想输出。

Select 和 membership 的端点、两个 task 共同 handle、比较 bit、key IDs/访问顺序、pivot/U/V/W 与抽样结果都是 L 的显式分量。比较边仅由之前的 L bits 和公开 coin 决定。腐化方拿到自身 Eval share s 与对方帧 share t；已打开比较 bit b 满足 `t=b−s mod 2^64`，故在 dummy-key hybrid 中可用当前 challenge key 计算 s 并构造 t，后续 transcript/state 是 L 的确定后处理。raw adapter carry/sign 输出不公开；它们进入下一阶段的本方 masked operand 消息。每个阶段将本方密钥和 Eval 作为 view，消息中受新鲜均匀 mask 遮蔽的 peer component 直接按 L 给定；本方 mask share由本方输入 share、此前本地 Eval 与自己已发送的 masked operand 确定。该步骤以原始 mask shares 由 T 独立均匀生成、且未被同一 party 另外获得完整对方 key/share 为前提。

于是可给出条件性定理：若 raw/Select/membership 所有 DCF key 都满足上节完整 key function privacy，OS/PRG 假设成立，shuffle 的置换与 mask 由可信 T 按源码独立均匀生成，channel 是私有认证通道，则对 c∈{0,1} 的上述 simulator，除 DCF/根 PRG hybrid 优势外，shuffle masked-frame 与 inverse output-share 条件分布是完美模拟；两 task 共用 π 由同一 L 和同一 mask-pool关系保留，不把两实例错误地拆成独立置换。该定理只保护未获准字段的输入隐私，明确泄露整个 L。端到端优势上界写作 `N·ε_DCF + Adv_ROOT_PRG(Q) + Adv_OS/coin`；在 OS `getrandom` 理想建模下后项为 0，否则未量化。实际应用仍需独立确认 raw score 的两个 masked operand 帧、inverse frame 和函数返回 share 与这里的 L 精确一致，并复核 shuffle 输入输出方程；故本轮裁决 `CONDITIONAL`，不是无条件 secure PASS。无反例不等于安全。

## 依赖与门禁

| 子门 | 状态 | 结论 |
|---|---|---|
| 允许公开字段 | USER_APPROVED_IN_S22 | 只表示项目许可，不等于安全证明 |
| ID 不编码原索引/分数 | CODE_REVIEW_PASS | shuffled endpoint pair 与用途域分离；raw local slot 不公开 |
| 正常概率 abort 统一为 1 | IMPLEMENTED_AND_E2E_TESTED | 双方 peer-agreed，无 mask；自然失败未必在有限测试触发 |
| 工程/不变量错误 | IMPLEMENTED_AND_E2E_TESTED | code 2、退出 70、engineer flag，不归入概率 abort |
| TLS 缺流降级 | IMPLEMENTED_FAIL_CLOSED | Strict 模式无 raw FD fallback；legacy FD 仍为显式模式 |
| H1→H2 相关自适应多 key | CONDITIONAL | 给出 N·ε hybrid；依赖实现等价于单 key DCF 函数隐私的独立审查 |
| H2→H3 shuffle/output | CONDITIONAL | 给出基于实际 shuffle 等式、公开帧 L 与输出 share 的模拟器草案；须独立核验材料采样分布、所有帧确实纳入 L、以及 inverse 输出份额的条件分布。未发现攻击不等于证明 |
| SECURE_ALIAS | NOT_CREATED | 维持 EXPERIMENTAL |
| 正式性能 | NOT_RUN | 本阶段不运行正式 LAN/WAN 矩阵 |

后续最小安全评审动作：独立审查者须逐行核对 H1→H2 中 BGI15 comparison-FSS 定理与 `keyGenDCF/evalDCF` 的 serialized key、纠错字和位序；再检查 H2→H3 模拟器确实覆盖 raw adapter 的 carry/sign masked frame、forward/inverse 两轮记录帧、公开和、共同 `pi`、两 Select task 关联、概率/工程 abort 和每方最终输出 share。对应的最小未证引理是：给定本方输入 share、已批准泄露 L 和理想功能输出 share，实际代码产生的本方 shuffle state、全池 key、Eval/访问轨迹及完整输出视图可由 PPT simulator 产生，分布误差仅为上述 FSS/PRG 优势。此为可审查的条件性引理草案，不是已由独立审查关闭的结论；未通过独立签收前不能创建 secure alias。
