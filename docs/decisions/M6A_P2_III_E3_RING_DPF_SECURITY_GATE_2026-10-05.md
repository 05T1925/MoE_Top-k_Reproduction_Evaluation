# M6A-P2-III-E3：native ring DPF 安全门与 III+AAV86 组合合同

日期：2026-10-05。源码起点 `6c98ff32497e4024394a2174548c80363ec985f1`，独立分支 `codex/m6a-p2-iii-e3`。本记录关闭 E2 的“native ring DPF 单份 key 目标隐私”审查项，随后冻结小 D 项目组合的安全条件与消息合同。它不是作者精确 Protocol III+AAV86 结论；没有把 Protocol I+AAV86 的 carrier、许可或证明转给 Protocol III。

## 1. 判定摘要

| 门 | 结果 | 边界 |
| --- | --- | --- |
| native `DPF(b,64,u,1)` 单份 key 目标隐私 | **CONDITIONAL PASS** | 依赖 §3 明列的受限密钥 AES 展开 PRG、OS 熵与一次性独立流假设；不是从功能 conformance 推出的结论。 |
| E7 工具对 III 的联合单方视图 | **CONDITIONAL DESIGN PASS** | §3/§5 明列 DCF-key chosen-target 隐私和完整 FSS stream 混合假设，再逐步固定挑战无关旁信息；需要真实实现满足材料独立性、逐轮 fresh mask、逆路由方程与已许可泄露。 |
| 新组合包、真实 secure 帧、失败/重放、`D≤8` conformance→冻结 oracle differential→T/P0/P1 E2E | **D≤8 FUNCTIONAL GATE PASS** | E3 已实现并通过 native package conformance、signed-score oracle differential、独立 T/P0/P1 进程帧 E2E、错误包/重放/peer-abort 检查与 768 MiB/进程资源门。该项不等于大 D 容量或正式性能门通过；安全仍限于 §3 条件假设。 |
| 全对全 Protocol III 正式性能批次 | 独立推进 | 不依赖以上设计门，仍只测 `protocol_iii_raw_score_mask_party`。 |

## 2. 被审计的本地原语

审计对象是 `VFSS/ext/FSS/dpf.cpp` 的 `keyGenDPF`、`evalDPF_Payload`，`VFSS/ext/FSS/include/FSS/keypack.h` 的 `DPFKeyPack`，`VFSS/ext/FSS/src/FSS/comms.cpp` 的 `Peer::send_dpf_keypack`，以及 F1 的 `encode_dpf`/`decode_dpf`。F1 正式 raw-score→mask 运行时实际调用 `evalDPF_Payload`；本组合候选固定 `bin=b=log2(D)≤3`、`bout=64`、payload `1`，目标为每个 hidden handle 的独立 `u_h∈Z_(2^b)`。

代码与经典二叉树 DPF 的对应关系：

1. `keyGenDPF` 从调用方 FSS PRNG 取两份 128-bit 根种子；`notOneBlock=toBlock(~0,~1)` 将子树种子的低控制位清零，故每个树 seed 有 127 个可变位，另一个低位存 control bit `t`。
2. 每层对父 seed 用 AES 加密两个不同常量块 `(0,0)`、`(0,1)`。高位 seed 部分是两个孩子的 PRG 输出，最低位是左右 control bit。
3. 生成方按目标路径 `idx` 取 `keep`，把 loose child 的两个 seed XOR 作为共享 seed correction word；左右 control correction word 分别再 XOR `keep` 或 `keep^1`，使 loose 分支收敛、目标分支 control XOR 保持 1。两份 key 收到相同 correction words。
4. 末层通过 `payload = β - low(s0) + low(s1)` 并按 party-1 control 位取负，编码到任意加法群；`evalDPF_Payload` 沿输入位走树，输出 `low(s)+payload*t`，party 1 取负。由此两方重构为 `β·[x=idx]`。对本组合 `β=1`，再在 `Z_(2^64)` 累加后取低位。
5. native wire 是每 key `bin+1` 个 block、左右 control 字、payload 和 64-bit sentinel；domain/output 宽度由外层带绑定。F1 的 bundle codec 逐 key 使用相同 Peer codec 并检查长度。序列化/反序列化是 key 的确定性编码与解析，不增加随机数或协议输出；任一额外 metadata 在新组合包中另行绑定。

这不是对 BGI15 算法 1/2 的逐字节同构声明。BGI15 的正式 FSS 定义先对任意两函数、相同输入域的挑战 key 给出逐方 indistinguishability；其 PRG-based point-function 构造把纠错字写入二叉树并证明单份 key 隐私。GI14 的 DPF 定义更直接要求单份 key 可由仅知道 party id、输入长度和输出长度的 simulator 生成。native 代码采用同一类 tree correction invariant，但使用 AES 两点展开及 ring terminal share；本记录单独给出此实现对应的群输出归约，而不声称只因论文名称相似就自动适用论文定理。[GI14 DPF 定义与 PRG 安全结果](https://www.iacr.org/archive/eurocrypt2014/84410245/84410245.pdf)；[BGI15 FSS 游戏与 PRG-based DPF](https://www.iacr.org/archive/eurocrypt2015/90560300/90560300.pdf)。

## 3. 隐私游戏、假设与归约

### 3.1 本组合所需游戏

固定公开输入域宽度 `b≤3`、输出群 `G=Z_(2^64)`、payload `β=1` 和腐化方 `p∈{0,1}`。对手在 challenger 抽样前联合选择两个目标 `u0,u1∈{0,1}^b` 与状态 `z`。`z` 可依赖候选目标、真实 rank、rank-mask share 和其他候选固定值，但必须在替换位 `c` 抽出后逐字相同，并且不能依赖被挑战 keygen 的随机币。Challenger 抛 `c←{0,1}`，运行 native `keyGenDPF(b,64,uc,1)`，只交给对手 party `p` 的 key（包括 wire 编码）。要求对手猜 `c` 的优势可忽略。该游戏允许对所得 key 在任意多已固定公开点执行确定性 `evalDPF_Payload`；它是 key 的后处理。

组合 hybrid 的具体挑战对是 `(u0,u1)=(u_h,0)`：`u_h`、两方 rank-mask shares、公开 `m_h=rank_h+u_h` 和全部 Eval 点 `m_h-j` 均取自真实世界并固定，只替换这一份 DPF key 的函数目标。中间 hybrid 因而故意让替换后的 key 与保留的 rank-mask share 不一致；这不是新的协议执行，而是标准目标隐私游戏中的 key-only 替换。`rank_h` 只作为 reduction 的隐藏状态，不交给对手；最终模拟须单独证明 `(m_h,u_p[h])` 的联合分布可采样。公开 `m_h` 虽然在线生成晚于 DPF keygen，但不读取 DPF keygen 随机带；在独立随机带下可以交换采样顺序，作为两挑战世界相同的辅助状态，不能在替换目标后重新计算。

该游戏是 GI14 “只泄露 party id 与输入/输出长度”的单份 key 目标隐私在显式 state 下的条件化形式，也是 BGI15 两函数挑战游戏只交付被腐化 key 的 specialization。`u0`、`u1`、Eval 点和辅助视图可以由对手知道；关键约束是它们在挑战 bit 改变时保持固定，不能重算以追随 `uc`。

### 3.2 假设

| 假设 | 精确定义/用途 | 本地实现状态 |
| --- | --- | --- |
| `AES-Tree-PRG-127` | 对均匀 `s` 取自 `{k∈{0,1}^128 : k_0=0}`，`G(s)=(AES_s(0), AES_s(1))`；比较 `G(s)` 与均匀 256-bit string，优势可忽略。两块各自最低位作为 `t`，其余 127 位作为 child seed。普通均匀 128-bit AES-PRP 安全本身不自动推出这个受限 key 子空间版本；这里明确把它列为组合假设。 | native `dpf.cpp` 满足受限 key 编码。 |
| `AES-CTR-Stream` | OS 随机 128-bit key 驱动 cryptoTools AES-CTR `PRNG` 的整段输出，在计算上与同长度独立均匀串不可区分。用于所有 DPF keygen 根 seed；替换整段 stream 后，分给不同 handle 的随机带才可视为独立辅助 key。 | native `PRNG::SetSeed/refillBuffer` 使用 AES counter；生产 T 必须以 `getrandom`/等价 OS CSPRNG 取 seed。 |
| `AES-CTR-FSS-Stream` | base Dealer 对 `FSSConfig::prngs[0]` 的整段 AES-CTR 输出与等长独立均匀串计算不可区分。源码在一次 `SetSeed` 后顺序生成全部 score carry/sign 和全部 edge DCF keys；先替换完整随机带，才可把每个 keygen 的随机带看成相互独立。 | `protocol_i_aav86_small_dealer_generate` 以 OS 熵设种；III dealer 必须保持单线程并在独立 DPF stream 之前完成该 pool。 |
| `DCF-Key-IND-64` | 对固定公开 `b∈[34,53]`，任选两阈值 `α0,α1`，challenger 仅给出 `keyGenDCF(b,64,αc,1)` 的腐化方完整 key；挑战前固定的辅助 state 可包含相关的其它 pool keys、共享 node-mask 本方 share、所有公开点和泄露值，只要这些 state 在 c=0/1 两世界逐字相同且不含 challenged keygen 随机币；允许任意确定性 DCF Eval/比较后处理。 | 这是 E7 全池 hybrid 需要的阈值 key 隐私假设。此记录没有把 native DCF 代码仅凭 conformance 宣称为定理；若本地正式原语定义/安全证明不覆盖该 chosen-target 游戏，该组合门应回退 NO-GO。 |
| 分域与新鲜性 | DPF stream、DCF stream、shuffle/mask randomness 使用彼此独立的 OS seed；每包/handle/轮次只分配一次，不跨任务或轮次复用。单份材料不含 peer 的 key、seed 或全部随机币。 | 新 III 包/Dealer 必须落实；E2 TEST_ONLY 固定 seed 不构成生产实现。 |
| 环与正确性 | `Z_(2^64)` 是有限阿贝尔加法群；native DPF 正确性按 §2 的两个 additive outputs 直接得出。点函数 payload 恒为 1。 | 已有 F1 conformance 覆盖加法重构/codec；还需新 `D` 个 handle 的包与路由 conformance。 |

`AES-Tree-PRG-127` 是本决策的实质密码学前提。若项目不接受对 AES 固定位密钥分布的 PRG 假设，DPF 门立即回退为 NO-GO；不能用“标准 AES”一语省略该分布差异。测试只能证明功能和格式，不验证该假设。

### 3.3 目标独立辅助信息的 hybrid

将本地 `keyGenDPF` 看作 binary-tree DPF：根 seed 为均匀 127-bit seed；每层扩展为两个 child seeds 与两个 control bits；correction seed/control 保证非目标分支合并；目标叶的 ring value correction 由独立未知的 peer leaf state 掩码。对被腐化方：

1. 先将 `seed_dpf_stream()` 给 `FSSConfig::prngs[0]` 的**整段** AES-CTR 输出替换为同长度独立均匀字节串。当前 `keyGenDPF` 每个 `DPFKeyPack` 从该流取两个 128-bit 根 seed；不能先把一个 handle 的 key 替换后就把同流产生的其他 DPF keys 当成与挑战币独立的辅助信息。整段替换后，不同 handle 的 32-byte 随机带彼此独立。目标 `u_h` 和 share 由独立 OS `getrandom` 产生。
2. 在整段随机流 hybrid 中，对从根到叶的 tree 扩展逐层把 `AES-Tree-PRG-127` 输出换为真正均匀输出。每份 key 有 `b` 层，生成时扩展两棵 party seed tree，故最多 `2b` 次 PRG 替换；D 个 key 总计最多 `2bD` 次。
3. 在均匀树 hybrid 中，目标位只决定每层哪一侧叫 keep/loose。该层被给出的单方 key 中，loose-child seed correction 是两份独立均匀 child seed 的 XOR，因而均匀；左右 control correction 是一个均匀 xor bit 与目标位的 XOR，仍均匀。未揭示的 peer target-path state 维持独立均匀 pad。
4. 末层 ring payload share 是 `±(1-low(s0)+low(s1))`。受限 child seed 的 `low(s)` 均匀分布在偶数子群 `2Z_(2^64)`，差值也是该子群均匀；所以 payload 均匀落在由公开 payload `β` 决定的奇数陪集（`β=1` 时是所有奇数），与目标 `u` 无关。该输出 share 连同均匀根 key、每层均匀 correction words 的模拟器只需 `(p,b,64,β)`，不需 `u`。
5. 对两个挑战目标 `u0,u1`，上述单份 key 的理想分布相同；归约上界为 `2bD·Adv_AES-Tree-PRG-127 + Adv_AES-CTR-Stream`，另加实现/参数绑定错误概率；这里 `D≤8,b≤3`。Eval 点、key serialization 与 key 的任意后处理不扩大区分优势。

因此 native ring DPF 子门是“给定上述明确受限 key PRG 假设时可归约”，不是从 AES 理论或 conformance 无条件推出。组合层还需接受 `AES-CTR-FSS-Stream` 与 `DCF-Key-IND-64`；本地 repo 没有把这两个假设自动升级为密码学定理。native serializer 不压缩、重排或丢弃一个额外的 secret field；它发送 key 中已有 seeds/correction/payload，所以 wire view 同样受 key 游戏覆盖。截断、错宽或恶意改包是完整性/拒绝服务问题，由包验证和 fail-closed 测试处理，不属于半诚实隐私游戏。

## 4. E2 项目泄露合同与全部新增公开字段

E2 原始项目所有者确认已记录：允许每轮 hidden-handle masked list、前 `r−1` 轮 local rank、pivot、bucket、活跃图、edge 访问/帧长和 abort 时点公开；末轮 rank、原槽 selected index、原序明文 mask 仍不公开。此确认仅授权泄露函数，不授权把 I 的证明搬到 III，也不放宽本节之外的输出。

| 每次/阶段公开 | 数值内容 | 可见关系/影响 |
| --- | --- | --- |
| 参数和帧头 | `session/version/fingerprint/n/D/K/r/ell/b/phase/round/endpoint-or-handle/width/one-shot-id` | shape 与执行阶段；身份/长度绑定，不含 key bytes。 |
| 前向 hidden layout | 双方各自 share-shuffle 首帧及完整 D 个输出 | 只有相加后隐藏置换的 key layout；原槽到 handle 的 π 不公开。 |
| 每轮 masked list `y_t` | D 个 `π(priority_key)+R_t` 的 2-share 打开 | 向双方公开；逐轮 fresh 均匀节点 mask，揭示长度与固定槽位数，不揭示 π 或 key。 |
| 早期 local-rank `L_t`, `t<r` | hidden handle 上的真实本地 rank 总和 | 揭示本轮相对名次；与 pivot seed/身份、bucket 成员、bucket 大小、公开 offset 及下一活跃图共同泄露递归分区和未来工作。 |
| pivot/bucket/活跃图 | handle 身份、节点树、桶边界和 canonical 活跃 endpoint pairs | 直接公开自适应控制图；公开访问次序、实际比较边、活跃顶点以及导致该图的早期 ranks。 |
| edge 访问、帧长、abort | 本轮访问了哪些预发槽、交换 word 数/字节、终止轮/失败位置 | 泄露实际工作量和失败进度；不能用“消息内容加密”掩盖流量/控制流。 |
| 最终 masked rank `m_h` | D 个 `rank_h+u_h` ring 点 | 双方公开；末轮 rank 不开，点相对最终 rank 由 fresh `u_h` 隐藏。 |
| 不公开 | 末轮 rank share/明文 rank、`u_h` 两份合值、原槽 selected index、原序明文 bit-mask、π、全排序、T 全部材料对任一在线方 | 输出只有各自原序 XOR share；完整 π 只在不合谋 T。 |

相对现有四轮全对全入口，本扩展额外公开每轮 `r` 个 D-word 的 hidden shuffled masked list、`r−1` 轮 hidden local rank 与 pivot/bucket/offset、实际活跃图/边顺序/帧长度、按轮 abort 位置，以及 hidden-domain 最终 masked rank。F1 则在原槽公开一份固定图上的 masked comparison list、均匀 masked rank 与固定长度消息；F1 不公开相对名次或图控制。已许可的 A 方案会泄露 shuffled-domain 的阶段名次与自适应工作；B 方案需 oblivious control / ORAM 或固定全池访问，当前无接口、证明、资源测量，且失去稀疏边收益，交互/材料/轮数为 `NOT_MEASURED/NOT_PROVEN`。

## 5. E7 全池 hybrid 对 III 的逐步联合审计

单方模拟器可见值固定为公开 shape、获许可的上述早期 `L_t`/图/帧和 abort、被腐化方自己的 raw-score 输入 share/score 与 shuffle/edge/DPF 包 share、其输出 mask share；模拟器不见完整 π、明文输入、最终 rank 或原序明文 mask。T 不与 P0/P1 合谋，也不看到任何在线消息。相邻 hybrid 中的挑战函数、公开值和辅助 state 均在挑战 key 随机币之外固定。

挑战 DPF key 的旁信息逐项如下；表中“固定”指比较 `DPF(u_h,1)` 与 `DPF(0,1)` 两世界时只改该 key 的目标，不重新执行在线协议：

| 旁信息 | 对 challenged DPF coins 的依赖 | key-only 替换时的处理 |
| --- | --- | --- |
| `m_h=rank_h+u_h` 与查询点 `m_h-j` | 依赖真实候选 `u_h`、秘密 rank 和公开 K；不调用 DPF keygen PRG。在线出现晚于 keygen，但它与 key coins 独立，可在证明中交换采样顺序。 | 用真实世界候选 `u_h` 生成后固定；换成 challenge 的 `0` 时不重算 `m` 或 Eval 点。 |
| 腐化方自己的 `u_p[h]` share | 依赖候选 target 与另一份随机 share；由 OS `getrandom` 生成，独立于 DPF stream。 | 固定真实 world share；challenge key 改变不会同步改 mask share。 |
| 早期 masked lists、`L_t`、pivot/bucket/活跃图、abort/帧长 | 由输入、score/DCF/置换随机带与允许公开控制流决定；这些材料在 DPF stream 单独设新种子前已生成，不含 DPF Eval 输出。 | 全部逐字固定；早期开值按获许可的泄露函数处理。 |
| 所有已用/未用 DCF score/edge keys、node-mask share | score 和 edge DCF keygen 顺序消费同一个 `FSSConfig::prngs[0]` AES-CTR stream；同一轮不同边的阈值还因复用节点 mask 而相关。node-mask shares 本身由独立 OS draws 生成。未使用 keys 也完整在腐化方 package 中。 | 先整体替换完整 FSS PRNG 输出流为独立均匀串，再做全 pool key hybrid；由此独立的是 keygen coins，不是 edge targets。逐 key 挑战 `(α_e,0)` 时，真实 `R_t`、所有 threshold targets、node-mask shares、`y_t`、其余 key packages 及早期公开量均固定，明确纳入 `DCF-Key-IND-64` 的 auxiliary state。公开 graph/访问是早期开值的确定性后处理；未用 key 同样逐项替换。 |
| 其他 `D−1` 个 DPF keys | 同一 AES-CTR PRNG 依次产生，**在真实流中与 challenged key coin 相关**。 | 先做 §3.3 的 full-stream AES-CTR→uniform hybrid。此后每个 handle 占独立 32-byte 块，挑战块与其余 DPF keys 条件独立。 |
| 前向/逆向 share-shuffle、同一 π、输入/输出 share | 在 base Dealer 阶段由独立随机带生成；逆路由输出是 challenged key Eval 的确定性后处理。 | 保留腐化方本地材料及完整前后向联合关系固定；输出/逆路由由 E7 条件方程与 ideal XOR output shares 模拟。 |

| 步 | hybrid 操作与保持固定的值 | 旁信息依赖审查 / 必要假设 |
| --- | --- | --- |
| 0 | 真实 H0。 | score adapter 输出 D 个 priority-key additive shares；n<D 时复用其真实 `INT32_MIN` dummy。GRank 不复用，全部 D handles 按项目组合做 CA。 |
| 1 | 先同时随机替换同 π 的前向/逆向置换材料。保持腐化方自己的 `σ_p,τ_p,γ_p,δ_p`、masked list、公开 ranks/graph、DPF message 与输出固定。 | 对给定 π 与腐化方材料，peer 侧独立均匀 permutation 存在唯一解；两条置换分别新鲜，peer 侧 permutation 同时是均匀 one-time pad。故固定 π 时本方 permutation material 分布不依赖 π，前/逆 pair 的联合分布也不依赖 π。mask vectors `a_p,e_p` 需独立 uniform；此项从 E7 I 代数重推，未调用公开全序/carrier。 |
| 2 | 先替换 base Dealer 一次生成全部 score/edge DCF materials 的完整 `FSSConfig::prngs[0]` AES-CTR stream；再对全池每个 score/edge DCF key 按 canonical 顺序换为 dummy-threshold key。 | 完整流替换依赖 `AES-CTR-FSS-Stream`。共享 `R_t` 导致 edge targets 相关，但单 key hybrid 每一步把整组真实 targets、所有未挑战 key packages、node-mask shares、input shares 固定，并只改变本 key 的 target；需要 §3 的 chosen-target `DCF-Key-IND-64`，不能拿“每把 key 的 conformance”代替。key coins 在完整流 hybrid 后相互独立；所有实际和未使用 keys 均纳入 hybrid。每轮 `R_t` fresh，避免跨轮相减消除 mask。 |
| 3 | 对 `t<r` 的早期开图轮，交换/替换单个 DCF key 后令 peer rank-share 消息取 `L_t-q_p`，保持打开的 `L_t`、pivot/bucket、offset、下一轮图、edge 顺序、帧宽完全不变。 | 这是 E7 对本方 key-share变动的 rank-opening 模拟。被允许公开的 `L_t` 让模拟器持有总和；图为它与公开 pivot seed 的确定性后处理。若实际帧另暴露未汇总的中间值，此步不覆盖它，需实现中禁止额外字段。 |
| 4 | 末轮不打开 local rank；保持 `m_h` 与 DPF Eval 点逐字固定，peer rank-share 帧取 `m_h-q_p-u_p[h]`。 | challenger 两目标下 `m_h` 和点向量不重新计算。T 预发 `u_h=u0_h+u1_h` 的两份加法 mask；一方自己的 `u_p[h]` 保留，peer share 独立均匀，所以给定公开 `m_h`、本方 share 时最终 rank仍被隐藏。该步不能套用早期 `L-q_p`，也不能把 `m_h` 改到随挑战目标漂移。 |
| 5 | 对 D 个 handle 各自逐 key 替换腐化方 `DPF(u_h,1)` key 为 `DPF(0,1)`。 | 目标对是 `(u_h,0)`；固定 `m_h-j mod 2^b`（全部 `0≤j<K`）、本方 `u_p[h]`、早期泄露、全部其他 DPF/DCF keys、共享 node mask 与前/逆置换材料。除被挑战 key 外，这些量不依赖被挑战 keygen 随机币；`m`/points 可依赖真实候选 `u_h`，但在两个挑战世界相同。先替换整条 DPF AES-CTR 随机流，避免把共流生成的其余 keys 当成独立旁信息，再用 native `DPF-IND-64` 逐 key hybrid；K 次 Eval、ring 累加和低位投影是 key 的确定性后处理。§3 给出该 native game 在 `AES-Tree-PRG-127` 与 `AES-CTR-Stream` 下的归约。 |
| 6 | 联合模拟 DPF 输出后的逆路由和双方公开流量。 | 对每个 party，前/逆 share shuffle 形如 `m_p=σ_p(x_p)+a_p`、`z_p=τ_p(m_{1-p})+e_p`，dealer 令 `τ_p∘σ_{1-p}=π`，且 `e_0=−τ_0(a_1)−h`,`e_1=−τ_1(a_0)+h`，`h` fresh uniform。腐化方收到的 peer 首帧因 peer `a` 均匀而均匀；其 inverse 输出为 `π(x_peer)±h`，给定整个前序视图和 ideal XOR output share 仍均匀。模拟器可抽取它，再将低 `b−1` 环位任选并反解 peer ring share；低位投影保 XOR。此处要求真实代码按该方程、同 π、新鲜独立 h 实现。 |
| 7 | 用均匀 masked lists/final masked ranks、许可的 early graph、理想输出 shares 生成终态。 | `y_t=π(key)+R_t` 与 `m=rank+u` 在 fresh peer mask share 下均匀；隐藏均匀 π 只给观察者随机 hidden labels，早期 ranks/graph 按明确泄露函数回放。公开 abort/帧长按真实控制事件生成；不得把 fail-closed 次序当作免费常数时间。 |

以上给出了在 §3 明示的 DPF、DCF 和 PRG 假设下，从材料池到组合视图的条件归约。第一个密码学条件是项目是否接受受限 AES tree-seed PRG 游戏；第一个基础原语映射条件是 `DCF-Key-IND-64` 是否由本地所用 DCF 定义/证明覆盖。随后还要求真实 T/party 满足完整 FSS/DPF stream 替换前提、域分离、独立 fresh masks、`L−q_p`/`m−q_p−u_p` 帧方程和逆路由方程。若这些条件有任一项没有被实现或原语定义覆盖，则**联合组合门仍为 NO-GO**，即使 ring DPF 子门条件通过。逆路由必须使用 DPF ring accumulator 映射到 `Z_(2^b)` 的低 b 位；“逐方先取 parity 再当 ring share”有最小反例 `1 XOR 1=0` 但 `1+1 mod 4=2`。单 key conformance、经验随机性或“半诚实”字样不能替代 PRG hybrid。

## 6. 完整协议合同与实际消息 DAG

所有离线数据均由 T 在任何 score share 进入之前生成和分发；T 不接在线 fd、不读输入/排序结果、不在线补料。分方 package 绑定 `(version,session,fingerprint,material_id,party,n,D,K,r,comparison_bits,b,phase,round,canonical edge/handle,element width)`，领取在第一条输入相关消息前 durable one-shot claim。超时、断连、短包、错 party/阶段/宽度、重放、T 不退出或任一方 abort 均 terminal failure，不允许同 ID 重跑。每轮保留全部 `r·C(D,2)` keys，但仅访问公开图中的边；unused 与 used key 布局各轮/各边彼此独立。

| DAG 层 | 输入和消息 | 重构/公开 | 因果轮的当前分类 |
| --- | --- | --- | --- |
| C0 T pre-input | D 个 priority compare 的 score material；双向 share-shuffle `σ/τ/a/e`（前/逆，使用同 π）；每轮 D node-mask shares 与全部 canonical edge `uCMP` keys；每 handle 两份 `u_h` shares 与 `DPF(b,64,u_h,1)` key；元数据与 fresh IDs。 | T 完整知道 π 与两方 offline 材料；然后 ACK/退出。 | 离线，无在线轮。 |
| C1 score adapter | carry open、sign open 两条真实 framed exchange。 | P0/P1 各得到 D 个加法 priority-key shares；dummy 由原 adapter 固定到 signed `INT32_MIN`。 | 现有 Protocol III 输入适配，2 causal rounds。 |
| C2 forward hidden layout | P0/P1 各发送 `σ_p(key_share_p)+a_p` 向量，接收 peer 向量后本地 `τ_p(·)+e_p`。 | 双方得到同 π 的隐藏 layout shares，不开 π 或原槽对应。 | D≤8 真实 framed-process E2E 观察到 1 causal exchange；socketpair 不作为 LAN/WAN 测量。 |
| C3 CA 第 t 轮 | 先各发送 D 个 `π(key)+R_t` additive shares并公开 `y_t`；本地只 Eval 活跃图 `E_t`；在 `t<r` 时双方发送 local rank shares并公开 `L_t`。 | 前 `r−1` 轮公开 `L_t/pivot/bucket/offset/G_(t+1)`；末轮保留 rank shares，只加公开 offset。 | D≤8 E2E 实测每轮 masked-list exchange 与前 `r−1` 个早期-rank exchange，合计 `2r−1` 帧。 |
| C4 final rank mask + ring DPF | 双方发送 `rank_h+u_{p,h}`；公开 `m_h`。每 key 在 `m_h-j mod 2^b`、`0≤j<K` 做 `Z_(2^64)` Eval 并对 K 个 payload share 累加。 | 不公开 `rank_h`、被选下标或明文 bit。各方保留完整 64-bit additive accumulator。 | D≤8 E2E 实测 1 次 masked-rank exchange，末轮明文 rank 未写入 secure view。 |
| C5 inverse hidden layout | 各方将完整 DPF accumulator 约化到 `Z_(2^b)`，作为 additive shares 用 inverse same-π 材料对 D 个 handle 做一次 ring share shuffle。 | 仅在原序结果上逐方取低位，裁掉 dummy，得到 XOR output shares；不打开明文 mask。 | D≤8 E2E 实测 1 次 inverse exchange；完整端到端 causal frames 为 `2r+4`。 |

消息图必须分开统计：`score adapter 2`、`forward hidden layout 1`、每轮 masked-list `r`、早期 rank `r−1`、`final masked-rank 1`、`inverse routing 1`。D≤8 独立进程测试为每个阶段分配真实 framed channel，核对 phase、发送/接收守恒与 `trace.size()==2r+4`；因此当前候选实现的 `online_rounds=2r+4`，其中 CA/DPF 子图 `2r` 是实际帧数。该计数不包含 TCP 网络校准，也不构成作者精确实现或端到端性能数据。

## 7. 实施和后续验证门

E3 已按此顺序实现并验证：native package/DPF key codec 与 ring share equation conformance → 固定输入生成器、signed Top-K 稳定 tie oracle 与恰 K differential → 独立 T/P0/P1 进程 framed E2E。矩阵覆盖 `n=2/5/8`、`K=1/中间/n`、全等/重复/signed 极值/非二次幂、`r=1..5`；每个 `(n,K)` 覆盖四种输入风格，T 的 pivot seed 在整组运行中均不同。错误包测试覆盖版本/session/party/n/D/K/r/handle/ID/宽度/截断/尾随字节/交换；另外验证 durable replay 拒绝和 peer abort 无有效 mask。逐次记录每方消息/接收字节、因果帧、每轮 `e_t/v_t`、DCF/DPF Eval、分阶段 DCF PRG、fresh material ID、pivot seed、oracle mask 与恰 K。T/P0/P1 `wait4` 峰值 RSS 均低于 768 MiB/进程门。secure path 不重构 rank/selected index/明文 mask；只有 TEST_ONLY 控制器核对两方输出 share 的 XOR 与 oracle。

`D≤8` 运行逻辑门通过后可讨论更大 D 的容量和资源门；本文未测任何 III+AAV86 九指标，全部仍为 `NOT_MEASURED`。Protocol III 全对全基线单独按 E2/E3 合同执行，其测量不能填 III+AAV86 指标。

生产准入边界：E3 通过的是小 D 组合功能门；环 DPF 安全是条件通过，整池 DCF/PRG 与双向视图安全也仅在 §3 的显式假设及 §5 的实现不变量成立时条件通过。该候选尚未接入产品调用路径，也没有独立密码学评审签收；不得据此标为无条件 `SECURE_RUNTIME=GO`。若 `DCF-Key-IND-64` 或完整 FSS stream 假设缺乏项目可接受的定理映射，组合安全状态回到 `NO-GO`，但已测的小 D 功能结果仍有效。
