# M6A-P2-III-E1：隐藏 handle AAV86 与 ring mask 设计门

日期：2026-10-04。状态：**NO-GO for secure runtime**；仅允许隔离的 `TEST_ONLY` 明文模型。`2r` 是项目候选核心目标，`AUTHOR_EXACT=NOT_PROVEN`。本门先于任何本任务 secure runtime 修改保存。E15 的正式被测源码是 `346a92326e81aa3ab573c162439968503e792354`，事后报告是 `40a1ccb450447c00ce604a08b058cbe5c0255e88`，本任务起点为独立接收检查点 `6a9ef8447cfd6b74c17f9a5645eedebcb8a6ea72`；三者不能合并成一个 revision。起点干净且 detached，`main=origin/main=merge-base(HEAD,main)=c3926c68fd14f270faa8b55234311071947fa080`，本任务分支 `codex/m6a-p2-iii-e1` 从指定检查点建立。E15 原始记录和主工作区差异不属于本分支。

## 证据身份和接口

论文定义：Agarwal CCS 2024 §5 的 shuffle-based CA compiler / Theorem 5.1 给出 `2r+1` 核心边界；会议版没有 III+AAV86 的 `2r` 构造。AAV86 原文的 pivot 递归是比较图来源。本地参考行为：`protocol_i_aav86_small_party` 在隐藏 shuffled handle 上运行图，逐轮重建 local rank，末轮 `flatten` 出公开全序并由 P0 写 carrier；这不是 III。项目已接收的 F1 C-INSTANTIATION：`protocol_iii_raw_score_mask_party` 是 signed Q20.12 `Z_(2^32)` 加法份额到原序 XOR Top-K mask 的四轮路径，调用 score adapter、全两两 GRank、`Z_(2^64)` DPF 指示份额。下文把 AAV86 局部 rank 份额接 F1 ring DPF、再逆路由的组合均为**待验证项目候选**。M5 两轮 odd-prime field Fselect/Fsort 只接受一致的 field-shared record，既非本 bit-mask 入口，也不构成本入口通用 field→XOR 阻塞；取其份额低位无效。

| 阶段 | 现有对象和实际合同 | E1 判断 |
| --- | --- | --- |
| score / stable key | `protocol_i_raw_score_input_party` 两轮；`(UINT32_MAX-(raw^0x80000000))<<index_bits | original_index`，小 key 高优先级 | 真正复用 carry/sign uCMP；须在本候选调用边界给 D 个槽构造 dummy `INT32_MIN` 加法份额并保留原下标，不可直接把现有 n→D 输出用于图。原 score adapter 默认补齐原始份额为 `0`。 |
| 隐藏布局 | I+AAV86 两遍 share shuffle，有同一秘密 π 的前向/逆向材料；P0/P1 单方不持完整 π，T 当前生成时知道 π | 仅可最小适配，必须证明组合单方视图；不能复用 I 的公开全序 carrier 或把 I rank reveal 当 III 授权。 |
| GRank / CA | M5 `protocol_iii_grank_party` 固定原序 n 全两两、一轮 masked key；I+AAV86 用 D handle、每轮 fresh 节点 mask、全两两预发边 key、在线只求值动态图边 | 复用 `protocol_i_cmpagg_eval_party`/uCMP 的比较约定和单边原语；不能直接调用固定图 GRank。须新建按 `(session,party,t,a,b,parameters,one-shot ID)` 绑定的图适配器。 |
| rank→位 | F1 每**原槽**一个 rank mask 与输入宽 `ceil(log2 n)`、输出 `Z_(2^64)` 的 DPF key，`rank<K` 后逐方取低位 | 可复用 DPF 算法；必须为每个**隐藏 handle**（含 dummy）生成独立 key/mask，位宽改为 `ceil(log2 D)`，目标点和 mask 与同一 handle 绑定，不能复用原槽 key 向量。 |
| 原序输出 | I 的逆 shuffle 是同 π 的加法份额线性置换；F1 本地低位转 XOR | 须证明并绑定 D 个 handle 的 ring bit 份额经逆 π 后取低位、截 dummy；不得在 secure 路径重构原槽选中下标或明文 mask。 |
| field Fselect/Fsort | `F_(2^127-1)` 两轮一般 payload 路由 | 可选独立研究列；不能当本 ring mask 入口依赖，亦不能取 field 低位。 |

角色/材料：T 仅接收公开 `shape/session/fingerprint/material-id`，输入份额释放前完成所有材料分发并退出；P0/P1 各持 Q20.12 原始分数加法份额和自己的包、在线互通；测试控制器独占原始分数、输入份额种子和 oracle。保持可信、不合谋、无需擦除的 T 模型。固定 M 延期，不以在线 Dealer、补料、文件轮询或模拟 shuffle 替代。新候选的每轮 key 必须预发 `r·C(D,2)` 槽/方，实际 `e_A=Σ|E_t|` 次求值；节点 mask 为 `rD` 份额，另有 D 个 handle 的 rank mask/DPF key 及前向/逆向 shuffle 材料。未消费槽仍占离线材料，不计实际比较边。单个 package 的 session、party、轮次、端点、handle、宽度、K、一次性 ID/重放与截断失败合同尚未实现。

## 功能代数：条件性成立，直接接线有反例

定义 `D=2^ceil(log2 n)`（至少 2）、严格 priority key，分数降序、原下标升序；dummy 为 `INT32_MIN` 且下标 `n..D-1`。即使真实分数为 `INT32_MIN`，dummy 也恒后于真实元素。**直接复用**现有 score adapter 的补齐结果不满足此条件：它把未给出的 raw score 份额置零。最小反例 `n=3,D=4,K=3,scores=[-1,-1,-1]`：dummy 的 score `0` 排第一，三个真实 key 只占 rank 1..3；`rank<K` 选两个真实槽，原序 mask 基数为 2。修复候选须在新调用边界显式提供 D 个输入份额，把每个 dummy 设为 `0x80000000`，且保持索引 `n..D-1`；冻结 F1/I 入口不改。该修复目前只有 TEST_ONLY 代数模型，没有 secure 证明。

对递归节点 `B` 定义公开全局偏移 `o(B)`、严格 key 全序下的局部 rank。前 `r-1` 层公开每个参与 handle 的 local rank，故 pivot 以公开 local rank 排序，非 pivot 按其前面 pivot 的个数进入桶。若 pivot local rank 为 `p_j`，它的全局 rank 是 `o(B)+p_j`；第 `j` 桶的公开偏移是 `o(B)+p_j+1`（`j=0` 用 `o(B)`，空桶无子节点），大小由公开 membership 给出。归纳到叶：先前产生的 singleton 与 pivot 的 rank 已由公开偏移确定；末轮每个活跃节点 depth=1，`q=|B|-1`，图是该节点完整 clique，uCMP 聚合产生所有 handle 的**局部加法 rank 份额**。给一方加公开 `o(B)` 后得到全局份额。`B` 中至多 `D` 个位置，选 `b=ceil(log2 D)`，`0≤rank<D≤2^b`，所以真 rank 无模回绕；逐方在 `Z_(2^b)` 约减保持和。`D=2`、空桶、最后 pivot、同分及非二次幂 n 均由严格 key、递归偏移和 D padding 规则覆盖。此归纳只证明功能；它依赖早期 local-rank/bucket 的公开和最后 clique 的 share-only 输出，不能把 I 的公开末轮 `flatten` 重命名。

对 handle `h`，T 选独立 `u_h∈Z_(2^b)` 并为函数 `f_(u_h,1)` 分发两把 `Z_(2^64)` 加法输出 DPF key。双方只打开 `m_h=(rank_h+u_h) mod 2^b`，分别在 `(m_h-t) mod 2^b` 对 `t=0..K-1` 求值，得到和为 `[rank_h=t]` 的输出份额。每方先在 `Z_(2^64)` 累加，再取低位：`Z_(2^64)→Z_2` 为加法同态，故两方 XOR 是 `[rank_h<K]`。任意较窄 2 幂环 `Z_(2^w)` 的逐方模约减也保持重构和及低位；奇素数 field 不成立。DPF 必须覆盖 D 个 handle，且 key/mask 不能按原槽索引重排后错配。最小错配反例：两槽 π 交换、原槽 0 为最高优先级，则 handle mask `[0,1]`；直接截取作原序 mask 得 `[0,1]`，正确是 `[1,0]`。

逆 π 的加法 share 置换若作用在 D 个 0/1 XOR 份额（视为 ring 元素），两方原槽重构的低位等于 handle XOR bit，因为线性置换与模 2 投影可交换；截掉 `n..D-1` dummy 后恰 K。前向/逆向必须使用同 π，并且任何一方不取得完整 π 或原槽 selected index。这里只给功能方程，现有逆材料与新增 DPF 材料的**联合**单方视图仍待证明。

## 泄露、单方视图与失败引理

| 观察者 | M5 F1 全两两基线 | E1 候选新增公开/持有量 |
| --- | --- | --- |
| P0 | 自己输入/包、masked comparison list、每原槽均匀 masked rank、原序 XOR mask 份额、固定全图/流量 | 自己的 share-shuffle 因子、每轮 masked list、早期 local rank、pivot 与 bucket/子图、访问/帧长度、每 handle masked final rank、DPF 输出份额、逆路由材料、原序 XOR 份额；末轮 rank 不打开。 |
| P1 | 同 P0 | 同 P0，持另一因子和 key；不得由单方联合材料恢复 π。 |
| T | 输入无关 GRank/DPF 随机币及公开 shape/session；在线前退出 | 还知完整 π、各轮全部节点 mask、同一 mask 上全对全相关阈值 key、handle DPF key/rank mask、逆路由相关材料；不知道输入、rank、在线图。 |

**失败引理 1（泄露合同）**：`m_h=rank_h+u_h` 对独立均匀 `u_h` 为均匀；早期公开 local rank、pivot、bucket 与后续图不是这个 masked-rank transcript 的字段。例 `D=2,r=2` 首轮选一个 pivot、两 handle 比较并公开 local rank，即公开 shuffled handle 全序；F1 只公开均匀 masked ranks。隐藏均匀 π 是否足以在 P0/P1 的**联合本地视图**中模拟此新增信息，当前无证明，不能靠“handle 被打乱”口头判断为零泄露。若合同明确许可，字段为各轮 `y_t`、局部名次、pivot 身份、桶成员/大小、活跃图、访问/流量和 abort；它们能给出 shuffled-domain 的递归相对序、桶边界与工作量。两种代价：许可扩大公开合同并承受这些可观测量，或另设计 oblivious graph/control 隐藏它们并计额外交互/材料。所有者许可 **PENDING**，不能从 I 的许可自动转入 III。

**失败引理 2（联合材料归约）**：现有单边 uCMP/DCF conformance 与 I 的条件性安全论证没有给出以下联合分布的单方模拟器：同一轮节点 mask 被 `C(D,2)` 个预发 key 重用；自适应选择其中一部分；公开 masked list/local rank；同一 handle 的 DPF rank mask/key 与后续 masked-rank opening；前向及逆向相关置换材料。把独立单 key 安全直接乘起来不能推出共享 mask 下的多 key 安全。尤其未用 edge key 留在包里，不能当成不存在。需要明确 hybrid：先替换 P0/P1 的 share-shuffle transcript；再在共享节点掩码与**整池**相关 DCF keys 下模拟 adaptive selected edges 及 local-rank reveal；接着在给定此 transcript 时替换每 handle DPF key/masked final rank；最后联合逆路由材料模拟原序 XOR 输出份额。每步须说明条件分布、T 的独立视图及允许泄露。当前没有这样的证明或适用定理。I 的测试不能充当 III 安全证明。

以上任一失败足以判 NO-GO。功能上的修复方向及 TEST_ONLY 差分通过不会消除失败引理。

## 真实候选消息 DAG、容量和研究次序

候选在线 DAG（`→` 为值依赖；可否合并必须按实际帧验证）：

```text
R_score1(carry) → R_score2(sign) → priority-key shares
    → R_shuffle(forward two-pass exchange) → hidden D-handle shares
    → R_open(1): y_1 → local DCF/share eval → R_local(1): open local rank/bucket
    → G_2 → R_open(2): y_2 → ... → R_local(r-1): open local rank/bucket
    → G_r → R_open(r): y_r → final local rank SHARES + public offsets
    → R_DPF: open masked final ranks → local DPF sum/low-bit
    → R_inverse: secret-shared inverse π → original-order XOR bits
```

CA/DPF 核心在此特定候选 DAG 上的**条件性**交换数为 `r+(r-1)+1=2r`；这是按待实现帧推导的项目目标，不是论文定理或实测。包括 score/shuffle/inverse 的候选上界式为 `2+1+2r+1=2r+4`，但复用接口的实际合并/中止路径未实现，故**实际 raw-score→原序 mask 轮数 = NOT_PROVEN**。P0/P1 每方每轮出站必须在同轮对端读取前可计算；下一轮图必须等待前轮 local-rank 公开。不得把最终 rank-share 或逆路由假设为零轮。若 `r=1`，无早期公开，仍需单独检查末轮完整图和帧。

容量门：D≤8、1≤r≤5 候选每方全边预留至多 `5·C(8,2)=140` key 槽，另 `rD≤40` 节点 mask、D 个 rank mask/DPF key 和前/逆置换材料；这只是 checked shape，**包字节、峰值、时间均 NOT_MEASURED**。D=128,r=5 则每方 40,640 key 槽；III 新 key/逆向材料必须重新做 64 MiB 包与 768 MiB RSS 预检，不可借 E15 的 I 字节数。`e_t/v_t` 是实耗边/顶点；预留 `r·C(D,2)` 与实耗分列。固定 M 不接入。

门项：功能代数 = **CONDITIONAL**（直接接线反例，显式 dummy 修复和最终份额偏移有模型）；离线材料 = **NO-GO**（联合视图与绑定未验证）；单方视图/泄露 = **NO-GO**（新增公开字段待许可及混合证明）；同一隐藏布局/逆映射 = **CONDITIONAL**（功能线性，联合隐私未证）；消息 DAG = **CONDITIONAL / ACTUAL NOT_PROVEN**；容量 = **SHAPE ONLY / ACTUAL NOT_MEASURED**。总门为 **NO-GO**，本任务不写伪 secure 入口。

下一研究步骤：先在独立隐私审查中固定新增字段许可或选择 oblivious 控制；给共享节点 mask + 全预发相关 key + adaptive use + DPF + 双向置换的单方 hybrid；在通过后冻结 per-party package 和帧，做 D≤8 conformance→冻结 oracle differential→独立 T/P0/P1 进程 E2E。覆盖 n=2/5/8，K=1/中间/n，全等/重复/signed 极值、非二次幂、不同 pivot seed、错误材料/重放/peer abort/截断；逐次记录输入和算法种子、每轮 e_t/v_t、分方帧及因果轮。九指标原始列：离线时间、离线材料 bit、在线时间、总时间、总通信 bit、每方通信 bit、因果轮、实际 DCF 长度倍增 PRG 调用、实际比较边；另列 DPF Eval、DCF Eval、AES、预留槽、阶段及分方计数。此门未实测者一律 `NOT_MEASURED`。

后续 n128 匹配性能方案：仅在上述门关闭且容量预检通过后，以同功能的 `protocol_iii_raw_score_mask_party` 全对全 raw-score→mask 完整入口为基线，同 n/K/输入 seed/算法 seed 记录/网络/重复次数，分开报告论文核心、输入和输出适配；按 V3 计划 LAN/WAN、1 预热+5 正式、逐次原始行和九指标 median/min/max。n≥256 保留容量状态，不外推。六方案最终汇总分共同可运行规模与各路线容量上限；本 E1 没有跑正式性能矩阵。
