可以。论文证据足以冻结 Protocol III 的核心结构，但会议版没有给出具体有限域模数、完整消息编码和 Dealer 序列化格式；这些不能自行写成“论文规定”。

先建立符合你命名规则的分支：

```bash
cd ~/projects/MoE_Top-k_Reproduction_Evaluation

git switch design/m5-protocol-iii-exact-2round
git status --short

git switch -c M5.0.1-paper-evidence

touch docs/decisions/M5_PROTOCOL_III_PAPER_EVIDENCE.md
nano docs/decisions/M5_PROTOCOL_III_PAPER_EVIDENCE.md
```

将下面完整内容写入该文件。

```markdown
# M5 Protocol III 论文证据记录

状态：**M5.0.1 论文证据已核对，尚未授权实现合并**

本文记录 Agarwal 等人在 CCS 2024 论文
*Secure Sorting and Selection via Function Secret Sharing*
中与 Protocol III 精确两轮核心直接相关的事实。

本文只冻结论文明确说明的内容。论文会议版没有给出的工程参数、消息编码、
有限域实例、序列化格式和异常处理策略，不得标记为“论文要求”。

---

## 1. 论文身份

- 标题：Secure Sorting and Selection via Function Secret Sharing
- 作者：Amit Agarwal、Elette Boyle、Nishanth Chandran、Niv Gilboa、
  Divya Gupta、Yuval Ishai、Mahimna Kelkar、Yiping Ma
- 会议：ACM CCS 2024
- 会议时间：2024-10-14 至 2024-10-18
- DOI：10.1145/3658644.3690359
- 论文印刷页：3023–3037
- 本地 PDF 页数：15
- 本地 PDF SHA-256：

```text
18FAF63EAA7923EEF715A6EB9D5D526FE04DCB69700B133C3E94DE935F68C01C
```

本文中的页码同时给出：

```text
PDF 页码 / 论文印刷页码
```

---

## 2. 证据强度标签

本文使用以下标签。

### PAPER_EXPLICIT

论文正文、图、表或定理直接明确说明。

### PAPER_DERIVED

由论文明确给出的公式、协议顺序或安全条件直接推导，
但不是论文中的原句。

### PROJECT_DECISION

为了在本仓库落地而作出的工程决定，不属于论文结论。

### NOT_SPEC_SPECIFIED

当前 15 页 CCS 会议版本没有给出；不能自行声称为论文规定。

---

## 3. Protocol III 身份

### 3.1 Table 1 中的 Protocol III

证据位置：

- PDF 第 3 页
- 印刷页 3025
- Table 1
- §1.1.1

论文将 Protocol III 定义为：

| 属性 | 论文内容 | 证据 |
| --- | --- | --- |
| routing | standard DPF routing | PAPER_EXPLICIT |
| ranking | complete-graph Compare-Aggregate，即 \(\binom{n}{2}\)-CmpAgg | PAPER_EXPLICIT |
| party model | \(2+1\) | PAPER_EXPLICIT |
| online rounds | 2 | PAPER_EXPLICIT |
| security | 单方静态半诚实腐化 | PAPER_EXPLICIT |
| online communication | Table 1 中为 \(4n\ell+2n\log n\) bits | PAPER_EXPLICIT |
| online computation | \(n^2\ell\) 个长度扩展 PRG 调用，忽略低阶项 | PAPER_EXPLICIT |
| offline communication | \(\lambda n^2\ell\) bits，忽略低阶项 | PAPER_EXPLICIT |
| theorem | Theorem 4.2 | PAPER_EXPLICIT |

因此，M5 的论文核心候选实现必须使用以下身份：

```text
ranking = binom(n,2)-CmpAgg
routing = DPF
party model = 2 online parties + 1 offline dealer
online rounds = 2
```

不能将下列实现标为 Protocol III exact：

```text
shuffle routing
mult-DPF ranking
3 online parties
3-round modular routing
raw-score adapter + 2-round core 的完整外层路径
```

---

## 4. 安全模型与角色边界

证据位置：

- PDF 第 3 页 / 印刷页 3025，Table 1 caption
- PDF 第 4–5 页 / 印刷页 3026–3027，§2.1

### 4.1 腐化模型

论文证明的是：

```text
static semi-honest security
single-party corruption
```

状态：PAPER_EXPLICIT。

当前 M5 不得宣称：

```text
malicious security
active security
security under collusion of two parties
authenticated-channel security
adaptive security
```

除非后续另行增加证明或机制。

### 4.2 2+1 模型

Protocol III 的角色为：

```text
P2 / Dealer:
    输入到达之前生成并发送相关随机性；
    在线阶段保持静默；
    不持有在线输入；
    不接收在线输出。

P0、P1:
    持有输入的加法份额；
    执行两轮在线协议；
    最终得到输出的加法份额。
```

状态：PAPER_EXPLICIT。

Dealer 可以通过安全两方协议替代，但这会产生额外成本。当前 M5
使用真实或模拟 Dealer 均可，但必须保持：

```text
Dealer preprocessing 完成
        ↓
Dealer 不再参与
        ↓
P0/P1 开始在线两轮
```

---

## 5. 功能与输入输出契约

证据位置：

- PDF 第 4 页 / 印刷页 3026，Figure 1
- PDF 第 5 页 / 印刷页 3027，Figure 2
- PDF 第 8–10 页 / 印刷页 3030–3032，§4.2

### 5.1 Fselect 输入

论文的选择功能公开参数包括：

```text
n              元素数量
L              key 的明文域大小
G              key 所在的有序阿贝尔群
k              目标 rank
H              payload group
```

P0、P1 分别持有：

```text
x_b ∈ G^n       key additive shares
y_b ∈ H^n       payload additive shares
```

满足：

```text
x_0 + x_1 ∈ [L]^n
```

状态：PAPER_EXPLICIT。

### 5.2 Fselect 输出

论文功能输出：

```text
rank-k key 的加法份额
对应 payload 的加法份额
```

状态：PAPER_EXPLICIT。

M5 第一阶段可以只验证 payload selection，但若使用
`agarwal_protocol_iii_exact_2round` 最终标签，必须明确它实现的是：

```text
Fselect key and payload
```

还是：

```text
payload-only project adapter
```

payload-only 版本不能不加说明地等同完整 Fselect。

### 5.3 排序扩展

论文说明 DPF routing 可以扩展到完整排序：

```text
对每个目标 rank 计算输出；
复用同一组 n 个 DPF key；
形成 n × n 的秘密共享置换矩阵。
```

这不增加在线通信或离线材料，但 DPF 求值从单点 Eval
变为 FullEval，在线计算量升高。

状态：PAPER_EXPLICIT。

M5 当前建议先实现：

```text
Fselect / single target rank
```

完整 Fsort 作为后续扩展。

---

## 6. Stable rank 语义

证据位置：

- PDF 第 6 页 / 印刷页 3028，§3
- PDF 第 7 页 / 印刷页 3029，Figure 3、Lemma 3.1、
  Corollary 3.1.1

论文定义 stable rank 为：

```text
比 x_i 小的元素数量
+
在 x_i 之前且与 x_i 相等的元素数量
```

因此：

```text
数值较小者 rank 更小；
相同数值时原始索引较小者 rank 更小；
最小元素 rank = 0；
最大元素 rank = n - 1。
```

状态：PAPER_EXPLICIT。

完整图上的 Compare-Aggregate 等价于稳定排名：

```text
H = clique([n])
edge set = all (i,j), i < j
number of edges = n(n-1)/2
```

每条边通过 uCMP FSS gate 产生比较结果份额，然后在各节点本地聚合为
rank 份额。

状态：PAPER_EXPLICIT。

### 6.1 k 的编号歧义

论文正文将 stable rank 描述为：

```text
0, ..., n-1
```

但 Fselect 功能又写作：

```text
k ∈ [n]
```

而论文记号中：

```text
[n] = {1, ..., n}
```

因此，15 页会议版本在目标 rank 的接口编号上存在表面上的
0-based/1-based 记号不一致。

状态：PAPER_EXPLICIT_AMBIGUITY。

仓库必须单独冻结转换，例如：

```text
外部 selection_k: 0-based
DPF rank domain: Z_n
target rank: selection_k
```

或者：

```text
外部 k: 1-based
内部 target_rank = k - 1
```

在冻结前，不得让不同模块各自解释 k。

---

## 7. CmpAgg ranking

### 7.1 输入域

Corollary 3.1.1 给出的 Compare-Aggregate ranking 参数为：

```text
plaintext keys in [L]
FSS input group Z_{L'}
L' ≥ 2L
rank output group Z_n
```

状态：PAPER_EXPLICIT。

\(L' \ge 2L\) 与 uCMP 的比较输入差值范围条件有关。

### 7.2 离线材料

Dealer 为每个输入位置生成随机输入 mask，并为完整图的每条边生成
uCMP/DCF FSS key shares。

对于边：

```text
e = (i,j), i < j
```

FSS gate 的秘密偏移参数绑定到相应的两个输入 mask。

状态：PAPER_EXPLICIT。

### 7.3 第一轮 ranking 通信

在线双方公开重构每个 key 的加法掩码形式：

```text
x'_i = x_i + r_i
```

每方发送 \(n\) 个 masked key 元素。之后双方使用公开 masked keys
和各自的 DCF/FSS key，在本地计算所有比较份额并聚合为 rank
加法份额。

状态：PAPER_EXPLICIT。

未经掩码的 key、比较结果和 rank 均不得公开。

---

## 8. 模块化 DPF routing

证据位置：

- PDF 第 9 页 / 印刷页 3031，§4.2

论文先给出一个独立的两轮 routing 模块。其输入为：

```text
rank shares y_i ∈ Z_n
payload shares z_i ∈ G_payload
```

Dealer 为每个位置 \(i\) 提供：

```text
rank mask additive shares r_i ∈ Z_n

DPF key shares for:
    f_{α_i,β_i}: Z_n → G_payload
    α_i = r_i
    β_i = 1

one Beaver multiplication triple:
    (a_i,b_i,c_i=a_i b_i)
```

状态：PAPER_EXPLICIT。

模块化 routing：

```text
Routing Round 1:
    双方公开 masked rank
    ŷ_i = y_i + r_i

    对目标 rank k，本地求值：
    f_{r_i,1}(ŷ_i - k)

    得到秘密共享 indicator。

Routing Round 2:
    公开 Beaver-masked payload 和 indicator；
    本地完成 indicator 与 payload 的乘积及求和；
    得到目标 payload 的加法份额。
```

状态：PAPER_EXPLICIT。

该 routing 自身需要两轮。与一轮 GRank 直接串联时得到三轮模块化协议，
这正是当前仓库 M3 的核心来源。

---

## 9. 两轮 round compression

证据位置：

- PDF 第 9–10 页 / 印刷页 3031–3032
- §4.2 “Round-compressed ranking + routing”

M5 的目标不是删除安全计算，而是把模块化 routing 的最后一轮
压入前面的通信中。

### 9.1 压缩前

```text
R1: GRank masked-key opening
R2: masked-rank opening and DPF evaluation
R3: Beaver-based indicator × payload combine
```

状态：PAPER_DERIVED。

### 9.2 压缩原则

论文把原 routing Round 2 的通信拆成：

```text
a-masked payload vector
b-masked indicator vector
```

payload 相关值在协议开始时已经存在，因此可以提前到先前轮次。

indicator 只有得到 rank 后才能计算，不能直接提前。论文通过非零乘法
mask 和带逆元 payload 的 DPF 消除这部分在线乘法轮次。

状态：PAPER_EXPLICIT。

### 9.3 压缩后的两轮因果关系

#### Round 1

并行执行：

```text
A. GRank 第一轮
   - 打开 masked keys；
   - 本地执行 CmpAgg；
   - 产生 rank additive shares。

B. payload masking
   - 对 payload z_i 与秘密共享非零 mask s_i 做逐项安全乘法；
   - 公开：
       z_tilde_i = z_i * s_i
```

Round 1 结束时：

```text
rank 仍为秘密共享；
z_tilde 为公开乘法掩码 payload；
原始 payload 不公开。
```

#### Round 2

```text
双方公开：
    ŷ_i = rank_i + r_i

使用修改后的 DPF：
    alpha_i = r_i
    beta_i = s_i^{-1}

对目标 rank k 求值：
    f_{r_i,s_i^{-1}}(ŷ_i - k)

得到：
    rank_i == k 时为 s_i^{-1}
    否则为 0
```

最后，本地计算：

```text
sum_i z_tilde_i *
      f_{r_i,s_i^{-1}}(ŷ_i - k)
```

唯一目标位置的乘法为：

```text
(z_i * s_i) * s_i^{-1} = z_i
```

因此双方得到目标 payload 的加法份额，且不再需要第三轮。

状态：PAPER_EXPLICIT 与 PAPER_DERIVED。

### 9.4 两轮消息图

```text
Preprocessing / Dealer
    |
    |-- CmpAgg masks and DCF/FSS keys
    |-- rank-mask shares r_i
    |-- DPF keys with alpha_i=r_i
    |                 beta_i=s_i^{-1}
    |-- nonzero multiplicative-mask material s_i
    |-- secure multiplication correlation for z_i*s_i
    |
    v
Dealer exits before online input

Online Round 1
P0 <----------------------------------------> P1
    open masked ranking inputs
    open the values required to obtain z_tilde_i=z_i*s_i
    locally evaluate CmpAgg
    locally obtain rank shares

Online Round 2
P0 <----------------------------------------> P1
    open masked ranks ŷ_i=rank_i+r_i
    locally evaluate DPF at ŷ_i-k
    locally inner-product DPF shares with public z_tilde
    output additive shares

No third online message.
```

---

## 10. Field requirement

证据位置：

- PDF 第 3 页 / 印刷页 3025，§1.1.1
- PDF 第 9–10 页 / 印刷页 3031–3032，§4.2
- Theorem 4.2

论文明确说明 DPF routing 的乘法掩码技术将 Protocol III 限制在
field 上。

Theorem 4.2 的条件是：

```text
H is a field
```

其中 H 编码 key 与 payload。

状态：PAPER_EXPLICIT。

因此以下结构不能直接作为 M5 exact 两轮的 payload algebra：

```text
Z_(2^b), b > 1
```

原因是它不是域，非零元素也不一定存在乘法逆元。

状态：PAPER_DERIVED。

### 10.1 论文没有冻结的内容

当前会议版本没有指定：

```text
具体素数 p
具体有限域实现
Montgomery/Barrett reduction
字节序
canonical field encoding
C++ field element 类型
```

状态：NOT_SPECIFIED。

这些必须在后续项目决策中明确，并标记为 PROJECT_DECISION。

---

## 11. 非零 payload 与乘法 mask

论文两轮压缩要求：

```text
payload plaintext z_i 非零
s_i 从 H* 中均匀取样
s_i 必须非零
s_i^{-1} 必须存在
DPF payload beta_i = s_i^{-1}
```

状态：PAPER_EXPLICIT。

### 11.1 为什么 payload 必须非零

如果：

```text
z_i = 0
```

则公开：

```text
z_tilde_i = z_i * s_i = 0
```

会直接暴露该 payload 为零，不能提供论文所依赖的乘法掩码隐藏性。

状态：PAPER_DERIVED。

### 11.2 论文建议的零值处理

论文脚注 7 说明，可以通过非零编码处理 payload，例如：

```text
输入时对所有 payload 加固定常量；
协议结束后移除该常量。
```

状态：PAPER_EXPLICIT。

但会议版本没有规定：

```text
固定常量具体是多少
允许的明文 payload 范围
怎样证明编码后永不为零
溢出处理
解码 API
非法编码时的错误类型
```

状态：NOT_SPECIFIED。

因此仓库后续必须冻结：

```text
encode_payload()
decode_payload()
is_valid_nonzero_encoding()
```

并证明有效输入不会编码为域中的零。

### 11.3 mask 逆元失败

对合法 preprocessing：

```text
s_i ∈ H*
```

所以逆元总是存在。出现 \(s_i=0\) 不是正常协议分支，而是 Dealer
材料无效。

项目实现必须拒绝零 mask，不能：

```text
把 inverse(0) 当成 0
重采样但不记录
回退到环乘法
公开原始 payload
```

状态：

- 非零 mask：PAPER_EXPLICIT
- 工程拒绝策略：PROJECT_DECISION

---

## 12. DPF 语义

两轮 Protocol III 的第 \(i\) 个 DPF 为：

```text
domain: Z_n
output: H
alpha_i = r_i
beta_i = s_i^{-1}
```

在线输入为：

```text
ŷ_i - k
```

正确性：

```text
if rank_i == k:
    ŷ_i - k = r_i
    DPF output sum = s_i^{-1}
else:
    DPF output sum = 0
```

状态：PAPER_EXPLICIT。

这不同于 M3 模块化 routing：

```text
beta_i = 1
```

因此不能直接把 M3 的 unit-payload DPF 材料标为 M5 exact 材料。

### 12.1 选择与排序的 DPF 次数

对于单一 rank 的 selection：

```text
每个输入位置一次 DPF Eval
总计 n 次/party
```

对于 sorting：

```text
同一 DPF key 在 n 个连续目标 rank 上求值
形成 n×n routing matrix
使用 FullEval
```

状态：PAPER_EXPLICIT。

---

## 13. 公开值与泄露边界

### 13.1 论文允许公开的中间值

两轮核心中允许公开：

```text
masked ranking inputs
masked ranks ŷ_i
multiplicatively masked payloads z_tilde_i
```

状态：PAPER_EXPLICIT。

### 13.2 不允许公开

不得公开：

```text
raw keys
raw payloads
comparison bits
rank values
rank indicator vector
multiplicative masks s_i
inverse masks s_i^{-1}
DPF alpha or beta
最终选择结果的明文
```

状态：由功能、安全模型和协议结构导出，PAPER_DERIVED。

### 13.3 rank 不公开

§4.2 明确指出 DPF routing 的目标之一是使秘密共享的 rank
永不公开。

因此任何实现若直接 reconstruct rank，均不能称为论文 Protocol III。

状态：PAPER_EXPLICIT。

---

## 14. 离线材料清单

M5 的 Dealer bundle 至少需要表达：

```text
1. CmpAgg ranking
   - input mask shares for n keys
   - one uCMP/DCF FSS key share per clique edge

2. compressed routing
   - n rank-mask shares r_i
   - n DPF key shares
       domain Z_n
       alpha_i = r_i
       beta_i = s_i^{-1} in H

3. payload masking
   - n nonzero multiplicative masks s_i in H*
   - secure multiplication correlation required to obtain
     public z_tilde_i = z_i*s_i

4. public binding
   - implementation version
   - party id
   - n
   - L or key-domain identifier
   - target rank convention
   - field identifier
   - session identifier
   - preprocessing fingerprint
```

第 1–3 项的密码学关系来自论文。

第 4 项是仓库防止错配和复用的 PROJECT_DECISION。

### 14.1 会议版未给出的离线细节

论文将完整结构的细节指向 full version，没有冻结：

```text
bundle wire format
session/fingerprint 算法
material version number
one-shot 状态机
崩溃恢复
部分消费后的处理
跨进程序列化
```

状态：NOT_SPECIFIED。

---

## 15. 材料复用规则

论文说明在完整 sorting 中，同一个位置对应的 DPF key 可以对不同目标
rank 重复求值，从而形成一行置换矩阵。

状态：PAPER_EXPLICIT。

这不意味着 preprocessing 可以跨不同协议执行复用。

对于两个不同输入执行，若复用同一个加法 mask 并公开：

```text
x + r
x' + r
```

两者相减会泄露：

```text
x - x'
```

乘法 mask、Beaver correlation 和其他一次性材料也不能默认跨会话复用。

状态：PAPER_DERIVED。

仓库规则应为：

```text
允许：
    在同一次 Fsort 执行中，
    按论文方式对不同目标 rank 复用对应 DPF key。

禁止：
    跨 session 复用输入 mask；
    跨 session 复用 rank mask；
    跨 session 复用乘法 mask；
    跨 session 复用 Beaver/乘法相关材料；
    把已部分消费 bundle 重新作为 fresh bundle 使用。
```

---

## 16. 轮数口径

### 16.1 论文核心

Protocol III 的论文核心：

```text
online rounds = 2
```

状态：PAPER_EXPLICIT。

### 16.2 M3 模块化基线

当前 M3：

```text
GRank
→ DPF routing
→ secure combine
```

为：

```text
online rounds = 3
```

这是论文 §4.2 在 round compression 之前描述的模块化组合，
不是 Theorem 4.2 的最终两轮结构。

### 16.3 raw-score 外层

如果项目从 raw-score shares 开始，且 raw-score 适配器需要两轮，
输出 mask 转换又需要额外轮次，则完整工程路径不能标成论文的两轮
Protocol III core。

必须分别记录：

```text
input adapter rounds
paper core rounds
output adapter rounds
total engineering rounds
```

例如：

```text
2 + 2 + 2 = 6
```

或其他实际测量值。

具体总数以真实实现和测试为准，不得通过修改标签隐藏外层轮次。

---

## 17. Theorem 4.2 成本

证据位置：

- PDF 第 10 页
- 印刷页 3032
- Theorem 4.2

设：

```text
ell' = ceil(log L')
ell' + p = ceil(log |H|)
```

H 同时编码 key 和 payload。

对于 Compare-Aggregate ranking、2+1 party 的 Fselect：

### 17.1 在线通信

论文定理给出两名在线参与方合计：

```text
6 n ell'
+ 2 n log n
+ 4 n p
bits
```

状态：PAPER_EXPLICIT。

脚注 8 说明，若 Beaver triple 与 ranking gate 使用共同 mask，可优化为：

```text
4 n ell'
+ 2 n log n
+ 4 n p
bits
```

状态：PAPER_EXPLICIT。

Table 1 中 Protocol III 的：

```text
4 n ell + 2 n log n
```

是忽略低阶项、采用相应优化并省略额外 payload 项后的摘要口径，
不能与 Theorem 4.2 的未优化公式混写。

### 17.2 在线计算

两名在线参与方合计：

```text
2 * C(n,2) * DCF.Eval[Z_L', Z_n]
+
2n * DPF.Eval[Z_n, H]
```

状态：PAPER_EXPLICIT。

### 17.3 离线通信

Dealer 向两名在线参与方发送的离线通信主要为：

```text
2 * C(n,2) * DCF.keysize[Z_L', Z_n]
+
2n * DPF.keysize[Z_n, H]
+
8np
bits
```

状态：PAPER_EXPLICIT。

Table 1 将其概括为：

```text
lambda * n^2 * ell
```

并忽略低阶加法项。

### 17.4 sorting 扩展

Fsort 使用：

```text
DPF.FullEval[Z_n,H]
```

替换 selection 中的：

```text
DPF.Eval[Z_n,H]
```

状态：PAPER_EXPLICIT。

---

## 18. 会议版本未指定的工程参数

以下内容必须保持为项目选择，不能写成论文精确值：

```text
具体有限域和模数
field element 的内存表示
字段序列化与字节序
固定 payload 编码偏移
session/fingerprint 计算
socket frame 类型
网络 timeout
最大消息大小
随机数 API
seed 记录格式
异常类型
退出码
CTest target 名称
metrics JSON schema
Ubuntu/GCC/CMake 版本
```

后续选择这些参数时，文档必须标为：

```text
PROJECT_DECISION
```

---

## 19. M5 实现前必须证明的性质

### 19.1 Algebra conformance

必须证明：

```text
field addition/subtraction/multiplication 正确
每个非零元素存在逆元
a * inverse(a) = 1 for a != 0
zero inverse 被拒绝
序列化往返保持 canonical value
```

### 19.2 Nonzero encoding

必须证明：

```text
所有允许的 key/payload 输入编码后均非零
decode(encode(x)) = x
公开 z_tilde 不直接暴露零值位置
非法输入在在线阶段前被拒绝
```

### 19.3 DPF payload

必须证明：

```text
DPF output group 与 field H 一致
beta=s^{-1}
target point 输出重构为 beta
非 target point 输出重构为 0
传输前后 DPF key 语义不变
```

### 19.4 Two-round causality

必须用消息图或独立进程测试证明：

```text
Round 1:
    masked keys + payload masking messages

local:
    CmpAgg produces rank shares

Round 2:
    masked rank messages

local:
    inverse-payload DPF Eval
    public-vector inner product
    output shares
```

不得出现隐藏的第三次在线 exchange。

### 19.5 Privacy boundary

测试和 runtime 不得：

```text
reconstruct raw keys
reconstruct raw payloads
reconstruct ranks
reconstruct indicator vector
send s_i or s_i^{-1}
让 Dealer 在 online 阶段继续工作
```

---

## 20. 测试矩阵

至少覆盖：

```text
n=1, k=minimum
n=3，重复 key
n=5，非二次幂
n=7，重复 key
n=8，k 为边界
n=127
n=128
n=129
n=256
```

语义覆盖：

```text
严格递增
严格递减
重复值
全相等
最小 rank
最大 rank
非零 payload 编码边界
field 最大 canonical value
```

错误覆盖：

```text
zero payload encoding
zero multiplicative mask
wrong field id
wrong session
wrong fingerprint
wrong party
wrong n
wrong target rank
truncated DPF key
truncated multiplication material
material reuse
partially consumed material
Dealer 在 online 阶段仍持有不应持有的 fd
```

---

## 21. 标签规则

在以下条件全部完成之前，只能使用：

```text
moe_topk_protocol_iii_exact_2round_candidate
```

不能使用：

```text
agarwal_protocol_iii_exact_2round
```

完成标签要求：

```text
field algebra 已冻结并验证
nonzero payload encoding 已冻结并验证
DPF beta=s^{-1} 已验证
完整两轮因果关系已通过独立进程测试
Dealer 在 online 阶段退出
rank 从未重构
真实消息计数证明只有两轮
Theorem 4.2 成本口径可映射到 metrics
oracle differential 全部通过
M4 完成或仓库路线已明确授权提前合并
```

这是 PROJECT_DECISION，用于避免论文标签被提前使用。

---

## 22. M5.0 结论

根据 CCS 2024 会议版，可以冻结：

```text
Protocol III
=
2+1 parties
+
complete-graph CmpAgg ranking
+
standard DPF routing
+
field-valued nonzero payload encoding
+
nonzero multiplicative masks
+
DPF payload beta=s^{-1}
+
two online rounds
+
static semi-honest single corruption
```

不能从会议版冻结：

```text
具体 field modulus
具体 nonzero encoding constant
完整 Dealer bundle wire format
具体 secure multiplication实现
具体异常和恢复机制
具体 session/fingerprint 方案
``` 
后面这些必须作为独立项目决定和 conformance 工作完成。
