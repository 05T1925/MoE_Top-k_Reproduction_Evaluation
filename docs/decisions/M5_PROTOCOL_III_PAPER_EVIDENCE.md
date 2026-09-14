# M5 Protocol III 论文证据记录

状态：**待评审；合并后冻结为 M5 论文证据基线**

本文记录 Agarwal 等人在 CCS 2024 论文
*Secure Sorting and Selection via Function Secret Sharing*
中与 Protocol III 精确两轮核心直接相关的事实。

本文只把会议版论文明确给出的内容标记为论文证据。会议版未指定的有限域、
消息编码、序列化、状态机和工程错误处理属于项目决策，不得反写为论文要求。

## 1. 论文身份

- 标题：Secure Sorting and Selection via Function Secret Sharing
- 会议：ACM CCS 2024
- DOI：10.1145/3658644.3690359
- 印刷页：3023--3037
- 本地 PDF 页数：15
- 本地 PDF SHA-256：

```text
18FAF63EAA7923EEF715A6EB9D5D526FE04DCB69700B133C3E94DE935F68C01C
本文使用以下证据标签：
标签	含义
PAPER_EXPLICIT	论文正文、图、表或定理直接说明
PAPER_DERIVED	由论文公式或消息依赖直接推导
PROJECT_DECISION	本仓库为实现协议作出的决定
NOT_SPECIFIED	15 页会议版没有给出


2. Protocol III 身份
证据位置：
- PDF 第 3 页，Table 1；
- PDF 第 9--10 页，Section 4.2；
- Theorem 4.2。
论文中的 Protocol III 为：
属性	内容	证据
ranking	完整图 C(n,2)-CmpAgg	PAPER_EXPLICIT
routing	standard DPF routing with round compression	PAPER_EXPLICIT
party model	两个在线方和一个离线 Dealer，即 2+1	PAPER_EXPLICIT
security	单方静态半诚实腐化	PAPER_EXPLICIT
paper-core online rounds	2	PAPER_EXPLICIT
theorem	Theorem 4.2	PAPER_EXPLICIT


以下路径不能标记为 Protocol III exact：
- shuffle-based routing；
- multiplicative-DPF ranking；
- 三个在线方；
- M3 的三轮模块化 routing；
- raw-score adapter、paper core 和 mask adapter 的总轮数被统称为两轮；
- 只删除 M3 第三轮而没有实现 field masking 和 inverse-DPF payload。
3. 角色和安全边界
Dealer 在输入未知的离线阶段生成相关随机性并发送给 P0、P1。在线输入到达前，
Dealer 必须退出或保持完全静默。
P0、P1 持有 key 和 payload 的秘密份额，执行在线协议，最终得到输出份额。
状态：PAPER_EXPLICIT。
本项目据此冻结：
Dealer preprocessing complete
    -> Dealer channel closed
    -> online input released
    -> P0/P1 execute Round 1
    -> P0/P1 execute Round 2
    -> local output
以下行为不属于目标安全模型：
- Dealer 根据在线 rank、图或输入补发材料；
- Dealer 接收在线输入或输出；
- secure runtime 重构 score、rank、indicator 或 selected index；
- 跨 session 复用输入 mask、rank mask、乘法 mask 或 DPF key。
4. 排名语义映射
论文的 stable rank 定义为：
小于 x_i 的元素数量
+
位于 x_i 之前且等于 x_i 的元素数量
因此论文中最小元素的 stable rank 为 0。
状态：PAPER_EXPLICIT。
本项目冻结的 priority rank 为：
score 降序
original_index 升序
最高优先级 rank = 0
该方向转换属于 PROJECT_DECISION。M5 不重新实现另一套 rank 语义，而是复用
项目已冻结的 priority-key 和 priority-rank。只要 rank 仍是 0..n-1 的唯一排列，
DPF routing 的等值选择关系保持成立。
文档和指标必须明确区分：
paper ascending stable rank
project descending priority rank
不得把项目 rank 原样称为论文的 ascending stable rank。
5. 模块化 DPF routing
Section 4.2 首先给出两轮模块化 routing。其输入为：
rank shares y_i
payload shares z_i
target rank k
Dealer 对每个位置提供：
rank mask shares r_i
DPF key shares for f_(r_i,1)
Beaver multiplication material
模块化 routing 为：
Routing Round 1:
    open y_i + r_i
    evaluate DPF locally
    obtain indicator shares

Routing Round 2:
    open Beaver-masked indicator and payload inputs
    multiply indicator by payload
    aggregate selected payload
状态：PAPER_EXPLICIT。
将一轮 CmpAgg ranking 与该 routing 串联得到三轮协议。这对应仓库现有 M3：
R1 GRank
R2 masked-rank DPF routing
R3 secure combine
M3 是 M5 的正确性和开销对照，不得覆盖、改名或原地删轮。
6. 两轮压缩原理
论文将 modular routing 最后一轮涉及的内容分成：
payload 相关值
indicator 相关值
payload 在协议开始时已经存在，因此相关计算可以与 ranking 并行。indicator
必须在 rank 产生后才能得到，不能直接提前。
论文使用 field 上的非零乘法 mask 消除 indicator 与 payload 的在线乘法：
s_i <- H*
z_tilde_i = z_i * s_i
DPF alpha_i = r_i
DPF beta_i = s_i^-1
当 rank_i = k 时：
DPF_i((rank_i + r_i) - k) = s_i^-1
否则输出 0。因此：
sum_i z_tilde_i *
      DPF_i((rank_i + r_i) - k)
= z_selected
状态：PAPER_EXPLICIT 与 PAPER_DERIVED。
7. 本项目两轮消息 DAG
会议版没有给出完整 wire message 和 Dealer bundle。以下 DAG 是满足论文压缩关系的
项目实例化，标记为 PROJECT_DECISION。
Offline
Dealer 生成：
CmpAgg input masks and edge FSS keys
rank-mask shares r_i
nonzero field-mask shares s_i
field multiplication correlation for z_i * s_i
field-output DPF keys:
    alpha_i = r_i
    beta_i = s_i^-1
session/fingerprint/party/size/field bindings
Dealer 发送完成后退出。
Online Round 1
P0、P1 在同一因果轮次中并行交换：
A. GRank 所需的 masked priority-key shares

B. field multiplication 所需的 masked inputs，
   用于得到 additive field shares [z_i * s_i]
Round 1 本地计算结束后，每方持有：
[rank_i]
[z_i * s_i]
本轮不得公开 rank、payload 或 field mask。
Online Round 2
P0、P1 在同一因果轮次中交换：
[rank_i + r_i]
[z_i * s_i]
双方得到允许公开的：
hat_rank_i = rank_i + r_i
z_tilde_i = z_i * s_i
然后本地执行：
for target_rank in [0,K):
    d_i = FieldDPF_i(hat_rank_i - target_rank)
    selected[target_rank] =
        field_sum_i(z_tilde_i * d_i)
Round 2 后只允许本地计算，不得再发送 paper-core 消息。
因果关系为：
Offline material
      |
      v
Round 1 messages
      |
      v
rank shares and masked-product shares
      |
      v
Round 2 messages
      |
      v
opened masked ranks and masked payloads
      |
      v
local DPF evaluation and selected payload shares
线程并行、多个 socket 或提前缓冲消息不能改变上述因果轮数。
8. Field 和非零条件
Theorem 4.2 要求 payload group H 是 field。
状态：PAPER_EXPLICIT。
论文压缩还要求：
z_i != 0
s_i != 0
s_i^-1 exists
DPF beta_i = s_i^-1
状态：PAPER_EXPLICIT。
Z_(2^b) 在 b>1 时不是 field，因此现有 M3 的 uint64_t ring payload
不能直接用于 M5。
状态：PAPER_DERIVED。
会议版没有指定：
- 具体有限域；
- 不可约多项式或素数；
- C++ 类型；
- 字节序；
- field DPF 的 API；
- 零值编码和错误类型。
状态：NOT_SPECIFIED。
这些内容由 M5_PROTOCOL_III_FIELD_CONTRACT.md 冻结为项目决策。
9. 允许公开和禁止公开的值
两轮 paper core 允许公开：
masked ranking inputs
hat_rank_i = rank_i + r_i
z_tilde_i = z_i * s_i
禁止公开：
raw score
priority key
unmasked payload
comparison bit
rank
r_i
s_i
s_i^-1
DPF alpha
DPF beta
indicator
selected index
final plaintext mask
测试控制器可以在明确的 TEST_ONLY 边界重构最终输出，但 secure runtime
不得调用或包含该路径。
10. Selection、Top-K 和 mask 的身份
论文 Theorem 4.2 直接给出单个目标 rank 的 selection。状态：
PAPER_EXPLICIT。
本项目对 target_rank=0..K-1 使用同一轮公开值和同一组 DPF keys，得到 K 个
selected payload shares。状态：PROJECT_DECISION。
该扩展：
- 不增加 paper-core 在线轮数；
- DPF Eval 数量由每方 n 增加为每方 nK；
- 输出 K 个 selected payload shares；
- 尚未直接产生原顺序 n-bit mask。
从 K 个 selected payload shares 转换为原顺序 XOR Top-K mask 属于独立
mask adapter。其时间、通信和在线轮数必须单独记录，并计入端到端主结果。
在 mask adapter 完成前，只能使用：
protocol_iii_exact_2round_candidate
protocol_iii_repeated_k_payload_selection_candidate
不得使用完整完成态标签：
agarwal_protocol_iii_exact_2round_mask_output
11. 成本核验边界
Theorem 4.2 对 Compare-Aggregate ranking 的单次 selection 给出：
online communication:
    6*n*ell' + 2*n*log(n) + 4*n*p bits
    across both online parties
脚注 8 给出采用共用 mask 时的优化：
4*n*ell' + 2*n*log(n) + 4*n*p bits
本项目通信报告必须说明：
- 是否采用脚注 8 的共用 mask；
- logical_n 和 padded_n；
- rank domain 是 Z_n 还是二次幂嵌入；
- field element 和 payload 的实际位宽；
- 单 rank selection 与 K-rank 扩展；
- framing、session 和错误检测开销；
- paper core 与 mask adapter 的分界。
论文公式、实现消息推导和实测 socket 计数必须分别保留，不能相互替代。
