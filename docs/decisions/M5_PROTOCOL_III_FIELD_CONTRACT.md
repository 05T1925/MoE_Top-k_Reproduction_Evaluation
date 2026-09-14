## 2. 新增 `docs/decisions/M5_PROTOCOL_III_FIELD_CONTRACT.md`

```markdown
# M5 Protocol III 有限域、非零 payload 与输出契约

状态：**待评审；合并后冻结 M5 field contract**

本文冻结 M5 Protocol III 两轮候选实现使用的有限域、payload 编码、DPF 输出群、
消息表示和输出身份。

这些选择属于项目决策，不是 Agarwal 会议版指定的工程参数。

## 1. 适用范围

本契约适用于：

```text
Protocol III round-compressed payload-selection core
K-rank repeated selection extension
selected-record to original-order mask adapter boundary
本契约不修改：
M1 score/tie/oracle
M2 Protocol I
M3 modular three-round implementation
VFSS-baseline
现有 M3 的 ring DPF、ring multiplication和最低位 mask 转换继续保持原语义，
不能通过类型转换进入 M5 field 路径。
2. 有限域选择
M5 选择二元扩域：
H = GF(2^64)
域元素表示为一个 uint64_t 多项式系数向量：
bit i = x^i 的系数
模多项式冻结为：
f(x) = x^64 + x^4 + x^3 + x + 1
低 64 位 reduction constant 为：
0x1B
该多项式必须在 conformance test 中使用独立的 Rabin irreducibility check
或等价的可信验证固定下来。实现不得只因为常量可运行就假设其不可约。
2.1 运算
addition(a,b)    = a XOR b
subtraction(a,b) = a XOR b
zero             = 0x0000000000000000
one              = 0x0000000000000001
multiplication   = carry-less polynomial multiplication mod f(x)
inverse(a)       = a^(2^64-2), for a != 0
inverse(0) 必须硬失败。
2.2 序列化
每个 field element 使用固定 8 字节 big-endian 编码。
GF(2^64) 的每个 64 位串都是 canonical field element，因此不存在 prime-field
式的非 canonical residue。
wire package 仍必须绑定：
field_id
field_version
irreducible_polynomial_id
byte_order
冻结值建议为：
field_id = "gf2_64_poly_1b"
field_version = 1
byte_order = "big-endian"
3. 类型隔离
必须新增独立类型，例如：
class ProtocolIIIBinaryField64;
不得使用隐式构造把以下类型当作 field element：
GroupElement
uint64_t ring share
rank share
priority-key additive share
允许显式 API：
from_canonical_bits()
to_canonical_bits()
add()
multiply()
inverse()
serialize()
deserialize()
禁止：
- 直接调用普通整数乘法实现 field multiplication；
- 使用 + 表示 field addition；
- 对 ring share 逐方 reinterpret 为 field share；
- 复用 M3 MaskedMulMaterial 而不经过 field conformance；
- 将 field share 传入现有 ring evalDPF_Payload()。
4. Field sharing
M5 field additive sharing使用 GF(2^64) 的加法，即 XOR sharing：
z = z_0 XOR z_1
split 操作为：
z_0 <- uniform GF(2^64)
z_1 = z XOR z_0
field share 与 M3 Z_(2^64) arithmetic share 是不同类型和不同协议，
即使二者底层都占用 64 位，也不得混用。
5. Payload 合同
两轮压缩核心接收每个输入位置的非零 field payload shares：
[z_i]^H
z_i != 0
通用核心不得假设 payload 等于 1，也不得只实现单位 payload 特化。
Dealer 为每个位置采样：
s_i <- H*
并生成：
[s_i]
s_i^-1
field multiplication correlation for z_i*s_i
field-output DPF key with beta_i=s_i^-1
以下情况必须硬失败：
z_i encoding is zero
s_i is zero
inverse requested for zero
field id mismatch
invalid material count
material reuse
session/fingerprint mismatch
secure runtime 不得通过重构 z_i 检查其是否为零。合法非零性由受信输入编码边界、
材料生成边界和 conformance test 保证。
6. 项目 selected-record 编码
为同时携带稳定排序 key 和原始位置，定义：
index_bits = protocol_i_index_bits(logical_n)
priority_key_bits = 32 + index_bits
priority_key =
    protocol_i_priority_key(raw_score, original_index, logical_n)
冻结的 field record 为：
tag_bit = 1 << priority_key_bits
encoded_record = tag_bit | priority_key
约束：
1 <= logical_n <= 1,000,000
index_bits <= 20
priority_key_bits <= 52
encoded_record uses at most 53 bits
encoded_record != 0
所有高于 tag_bit 的位必须为 0，tag_bit 必须为 1。
该编码的目的包括：
- 保证 payload 非零；
- 保留 score 和 original index 的稳定绑定；
- 选中后能够恢复同一个 priority record；
- 低 index_bits 继续表示 original index。
编码属于 PROJECT_DECISION。
6.1 输入适配边界
当前 raw-score 和 priority-key 接口产生 ring arithmetic shares，不能逐方
直接转换为 GF(2^64) XOR shares。
因此 M5 必须区分：
paper-core input:
    priority-key shares for GRank
    field payload shares for compressed routing

project input adapter:
    raw-score shares
    -> priority-key ring shares
    -> encoded-record field shares
ring-to-field 转换若需要额外通信，其轮数、时间和字节必须计入 input adapter，
不得隐藏在 preprocessing 或 paper core 中。
在该转换实现完成前，测试可以由 TEST_ONLY 输入生成器直接产生一致的：
priority-key shares
encoded-record field shares
但这种测试输入生成不能被描述为已完成的 raw-score secure adapter。
7. Field multiplication correlation
M5 需要 field 上的 share-preserving multiplication：
[z_i]
[s_i]
    -> [z_i*s_i]
建议使用 field Beaver triple：
a_i, b_i <- H
c_i = a_i*b_i
Round 1 打开：
d_i = z_i + a_i
e_i = s_i + b_i
其中 field 加减均为 XOR。
各方本地计算 product share。约定由 Party 0 加入公开交叉项：
q_0 = c_0 + d*b_0 + e*a_0 + d*e
q_1 = c_1 + d*b_1 + e*a_1
从而：
q_0 + q_1 = z_i*s_i
这里所有加法和乘法均在 GF(2^64) 中。
Round 2 公开：
z_tilde_i = q_0 + q_1
每个 triple、s_i 和对应 inverse 只能消费一次。
8. Field-output DPF 合同
M5 的 DPF 为：
domain:
    rank domain

output group:
    GF(2^64)

alpha_i:
    r_i

beta_i:
    s_i^-1
命中语义：
share_0(x) + share_1(x) =
    s_i^-1, if x = r_i
    0, otherwise
field 加法为 XOR。
现有：
evalDPF_Payload()
返回 Z_(2^bout) ring arithmetic share，不能直接复用为 field output。
实现应在 VFSS 现有 FSS 代码中增加最小的 output-group 扩展，复用同一 DPF tree
遍历和 correction-word 结构。不得复制整份 DPF 实现到 moe_topk。
建议使用独立 API 和 key wrapper，例如：
FieldDPFKeyPack
keyGenFieldDPF(...)
evalFieldDPF(...)
key 或外围 material 必须绑定 output group，防止 ring key 与 field key 混用。
最终 leaf 到 field element 的扩展必须产生完整 64 位 field element。不得直接把
已清除控制位的 tree seed 当作均匀 64 位 field leaf。若增加 domain-separated PRG
leaf expansion，其 AES/PRG 调用必须进入指标。
9. Rank domain
GRank 输出仍位于 rank additive group：
Z_(2^rank_bits)
rank_bits = max(1, ceil(log2(logical_n)))
field 仅用于 payload masking、DPF output 和 selected payload aggregation。
因此 M5 同时存在：
rank group: Z_(2^rank_bits)
payload field: GF(2^64)
两者必须使用不同类型。
当 logical_n 不是二次幂时，DPF domain 使用最小二次幂嵌入。合法 rank 仍仅为：
0..logical_n-1
padding domain 上的点不能成为合法 selected rank。
该嵌入是 PROJECT_DECISION，成本报告必须与论文记号 Z_n 区分。
10. Paper-core 输出身份
单 target rank 的 paper-core 输出为：
一个 selected encoded-record field share
K-rank 项目扩展输出为：
K 个 selected encoded-record field shares
target ranks = 0..K-1
该输出保持秘密共享，不公开 selected index。
实现候选标签：
protocol_iii_exact_2round_candidate
protocol_iii_repeated_k_payload_selection_candidate
在以下条件满足前，不启用最终 exact mask 标签：
field DPF conformance
nonzero encoding conformance
two-round causal audit
independent-process E2E
ring/field input adapter
original-order mask adapter
communication validation
cross-review
11. Mask adapter 边界
统一项目输出为：
logical_n 个 original-order XOR mask shares
从 K 个 selected-record shares 到 mask 的转换必须：
- 不公开 selected index；
- 输出每位 0/1；
- 恰有 K 位为 1；
- 保持原始输入顺序；
- 覆盖重复 score 和全相等输入；
- 单独记录通信、时间和轮数。
GF(2^64) sharing 本身是 XOR sharing，因此每方可以局部提取 selected record
的低 index_bits，得到 selected index 的 XOR shares。但是将 K 个秘密 index
转换为 n-bit membership mask 仍需要安全 equality 或等价 routing，不能通过测试
重构完成。
该 mask adapter 可以在 M2 接口交接后接入。它不属于本 field-contract 分支。
12. 指标边界
M5 至少分别记录：
input_adapter_rounds
paper_core_rounds
mask_adapter_rounds
total_online_rounds

field_mul_calls
field_inverse_calls
field_dpf_eval_calls
field_prg_calls

round1_sent/received
round2_sent/received
mask_adapter_sent/received

offline_field_material_bytes
offline_dpf_material_bytes
offline_total_bytes
paper core 固定目标：
paper_core_rounds = 2
端到端总轮数必须由真实依赖相加，不预先写死。
13. 明确非目标
本分支不实施：
- field arithmetic runtime；
- field DPF；
- ring-to-field adapter；
- 两轮在线 runtime；
- mask adapter；
- 三进程 E2E；
- 性能数字。
本分支只冻结合同和证明义务。所有未测指标继续记为 NOT_MEASURED。
