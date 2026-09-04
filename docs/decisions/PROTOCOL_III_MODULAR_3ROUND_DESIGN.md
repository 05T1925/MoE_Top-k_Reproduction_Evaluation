# Protocol III 模块化 3 轮设计

状态：**M3 实现规范；代码实现等待 M2 阶段门**。

实现标签固定为 `agarwal_protocol_iii_modular_3round`。本文描述的是 Agarwal
Protocol III 的模块化 3 轮中间基线，不是论文 Theorem 4.2 的精确 2 轮实现。

## 1. 证据层级与边界

### 论文定义

- Agarwal 等 CCS 2024 论文 §3.1：全对全 Compare-Aggregate 可在 1 个在线轮次
  输出 stable rank 的加法共享；
- 论文 §4.2（PDF 页 9--10，论文页 3031--3032）：标准 DPF 路由先公开加法掩码
  rank，再使用 DPF 得到指示向量共享，最后通过 Beaver 乘法完成 payload 内积，
  模块化路由共 2 个在线轮次；
- 1 轮 GRank 与 2 轮模块化路由串联得到 3 轮协议；
- Theorem 4.2 的整体 2 轮压缩要求 payload 群是域且 payload 非零，不属于 M3。

论文基线文件的 SHA-256 必须为：

```text
18faf63eaa7923eef715a6eb9d5d526fe04dcb69700b133c3e94de935f68c01c
```

### 项目扩展

- 项目使用 Q20.12 signed score、降序优先级 rank 和原顺序 Top-K XOR-shared
  bit-mask；
- VFSS 标准 DPF 的输入域是 `2^rank_bits`，因此非 2 次幂的 `n` 嵌入最小二进制
  域，而不是直接实现论文记号中的 `Z_n`；
- 论文的第 `k` 个 payload 输出扩展为原位置 mask，适配开销进入主结果。

### 本地参考行为

`ADSMPC/src/protocol3.cpp`、`RankingPhase.h` 和 `routing_dpf.h` 只用于检查调用顺序
和失败模式。它们不是安全性、轮数或论文一致性的证据。

## 2. 固定表示与接口契约

- 公开参数：`n`、`K`、score 位宽 32、scale 12、`rank_bits`、routing ring 位宽、
  实现标签和实验模式；
- 合法请求：非空输入且 `1 <= K <= n`；其他情况显式失败；
- `rank_bits = max(1, ceil(log2(n)))`，`rank_modulus = 2^rank_bits`；
- GRank 输出 `n` 个位于 `Z_(2^rank_bits)` 的加法共享 `y_i`；合法明文 rank
  始终位于 `0..n-1`；
- 仓库 rank 是降序优先级 rank：最大 score 的 rank 为 0，同分时较小原始下标
  优先；Top-K 目标集合固定为 `0..K-1`；
- 论文 rank 是“更小元素数量”，最大值的 rank 为 `n-1`。两者关系为
  `paper_rank = n - 1 - priority_rank`，实现和报告中不得混用；
- DPF 必须调用返回加法 share 的 `evalDPF_Payload`，不得把 XOR-share 接口
  `evalDPF_EQ` 直接送入算术乘法；
- M3 的 DPF payload 与乘法适配首先使用 64-bit `GroupElement` 环。所有中间结果
  按 `Z_(2^64)` 解释并计量实际 64-bit 通信；
- secure 输出仅包含原顺序长度 `n` 的 XOR mask share。rank、DPF index、selected
  index 和明文校验数据不属于接口。

## 3. 参与方与离线材料

参与方固定为 Dealer、Party 0、Party 1。Dealer 仅在输入未知的离线阶段运行，在线
阶段不得连接、接收输入或根据中间结果补发材料。

Dealer 为每次执行生成：

1. GRank/CmpAgg 所需的 DCF/FSS 相关随机材料；
2. 对每个位置 `i`，均匀采样 `r_i <- Z_(2^rank_bits)`，向两方发送其加法 share；
3. 对每个位置 `i`，生成标准 DPF key pair：
   `f_(r_i,1): Z_(2^rank_bits) -> Z_(2^64)`；同一 key 可在不同公开目标 rank 上
   重复求值；
4. 对每个 `(i,k)`，其中 `i in [0,n)`、`k in [0,K)`，独立采样 indicator 输入
   mask `a_i,k`、单位标记输入 mask `b_i,k` 和乘积输出 mask `c_i,k`，调用
   `MultGen(a_i,k,b_i,k,c_i,k)` 生成两方 multiplication keys；
5. 向两方分别发送 `a_i,k`、`b_i,k`、`c_i,k` 的加法 shares。在线方用前两者
   打开随机掩码输入，用最后一个从 `MultEval` 的局部结果中移除输出 mask；
6. 单位标记 `u_i=1` 的随机加法 shares。它们用于统一 mask 适配，不表示原始下标
   是秘密。

DPF keys 和乘法材料必须通过 VFSS `Peer`/Dealer 通道传输；不得自行序列化旧
`DPFKeyPack` 内存布局或读取参考工程的 `.bin` key 文件。离线材料数量和实际字节数
进入 `offline_material_total_bits`。

## 4. 三个在线因果轮次

| 在线轮次 | 输入 | 通信与公开值 | 本地计算 | 输出 |
| --- | --- | --- | --- | --- |
| R1：GRank | score arithmetic shares | 使用 M2 冻结的 CmpAgg/FSS 消息；只打开该原语规定的随机掩码值 | 全对全稳定比较并聚合 | priority-rank additive shares `[y_i]` |
| R2：DPF 路由第一轮 | `[y_i]`、`[r_i]`、DPF keys | 两方交换 `[y_i]+[r_i]` 并公开 `hat_y_i=(y_i+r_i) mod 2^rank_bits` | 对每个 `k in [0,K)` 计算 `I_i,k = DPF_i(hat_y_i-k)` 的 additive shares | 每个原位置和目标 rank 的 indicator shares |
| R3：DPF 路由第二轮 | `[I_i,k]`、`[u_i=1]`、独立乘法材料 | 打开 `I_i,k+a_i,k` 与 `u_i+b_i,k`，不打开乘积 | 每方调用 `MultEval`，再减去自己的 `[c_i,k]`，得到 `[P_i,k]=[I_i,k*u_i]`；按位置求和并转为 XOR bit share | 原顺序 Top-K mask share |

所有减法在 `Z_(2^rank_bits)` 中进行。因为 `hat_y_i = y_i+r_i` 且 DPF 点为
`r_i`，所以：

```text
DPF_i(hat_y_i - k) = 1  <=>  y_i = k.
```

R2 依赖 R1 的 rank shares，R3 需要先得到 R2 的 indicator shares 才能形成公开的
masked multiplication inputs，因此在线因果轮数严格为 3。线程同步、socket 建连
和 Dealer 离线传输不重复计为在线轮次。

## 5. 原顺序 bit-mask 适配

论文模块化 Fselect 在 R3 后对 `I_i,k * payload_i` 沿 `i` 求和，得到第 `k` 个
payload 的共享。项目统一输出改为保留每个原位置的乘积，并沿 `k` 求和：

```text
mask_arith_share[i] = sum(k=0..K-1, P_i,k)
mask_xor_share[i]   = mask_arith_share[i] & 1
```

stable rank 是 `0..n-1` 的排列，所以每个位置最多命中一个目标 rank；重构后的
`mask_arith[i]` 必为 0 或 1。对于 `Z_(2^64)` 加法 shares，双方最低位的 XOR 等于
重构值的最低位，因此最后一步无需额外通信。

共享单位标记乘法在数学上可被局部 DPF indicator 省略，但 M3 有意保留它，以复现
论文模块化路由的第二轮并为后续任意秘密 payload conformance 提供同一路径。任何
删除 R3 的 mask 特化必须使用不同实现标签，不得覆盖本基线。

## 6. VFSS 适配约束

- DPF：复用 `keyGenDPF`、`evalDPF_Payload` 和 `Peer::send_dpf_keypack`；先验证
  key 传输前后的求值一致；
- 乘法：论文 §4.2 使用 Beaver triples；M3 在 VFSS 中用 `MultGen`/`MultEval` 的
  一轮 masked multiplication 实例化相同的乘法阶段，并明确标为 VFSS 工程实例；
- 对公开 masked inputs `L=I+a`、`R=u+b`，两方 `MultEval` 结果的和是
  `I*u+c`。Dealer 必须离线提供 `c` 的加法 shares，各方输出
  `P_party=MultEval_party-c_party`，从而使两方结果之和为 `I*u`；
- 当前 `ElemWiseMul` 会在在线方内部调用 `reconstruct(...)` 并返回公开的 masked
  product，虽然随机输出 mask 可保护乘积，但它不满足 M3 所需的两方 additive-share
  输出接口，不能直接调用；
- M3 routing adapter 不调用 `reconstruct(...)`，只交换 R3 的 masked inputs，并
  保留 `MultEval` 的局部结果；
- 通信：只使用 VFSS socket/`Peer` 通道，不使用文件轮询、共享目录或固定 sleep；
- 模式隔离：仅测试二进制可重构输出；secure runtime 不编译或调用测试重构路径。

M2 必须在交接时提供 rank share 的环、party 编号、通信计数和错误语义。M3 可以
增加薄适配器，但不得复制第二套 score、rank、mask 或 metrics 语义。

## 7. 公开值、泄露与错误

在线允许公开：

- `n`、`K`、位宽、实现标签和实验配置；
- R1 的 GRank 原语规范中允许的随机掩码值；
- R2 的均匀掩码 rank `hat_y_i`；
- R3 的独立随机掩码输入 `I_i,k+a_i,k` 和 `u_i+b_i,k`。

不得公开：原始 score、完整 rank、`r_i`、DPF 点、indicator、selected index、最终
mask 或任意调试 oracle。随机性复用、key/triple 数量不足、party 消息长度不一致、
越界 rank、非法 `K` 和连接失败均为硬错误；不得降级到明文或返回部分结果。

## 8. 验证顺序

### 8.1 Primitive conformance

1. DPF additive reconstruction：命中点重构为 payload，其他点为 0；
2. 覆盖 `n=1,3,5,127,128,129`、索引 0/`n-1`、域填充位置和 0/1/一般非零 payload；
3. DPF key 经 `Peer` 发送和接收后行为不变；
4. 乘法 adapter 覆盖 0、1、最大 ring word 和随机值，输出只以 shares 存在；
5. 验证加法 bit share 的最低位转换为 XOR bit share。

截至 2026-09-04，前 3 项中的标准 DPF 本地重构和 key 传输已按
`docs/reproduction/DPF_CONFORMANCE_UBUNTU_2026-09-04.md` 完成首轮验证。传输验证
使用 VFSS `Peer`/Dealer 的内存通道，覆盖与 socket 通道相同的 DPF key
序列化函数；独立进程 socket E2E 仍保留在 8.3 节，不视为已经完成。

### 8.2 Oracle differential

- 重复值、全相等、负值、`INT32_MIN/MAX`、`K=1`、`K=n` 和非 2 次幂 `n`；
- 重构仅发生在测试层；所得 mask 长度为 `n`、值域为 0/1、和为 `K`，并与 M1
  oracle 完全一致；
- 对任意秘密 scalar payload 的单个 rank 路由结果与明文选择一致。

### 8.3 独立进程 E2E

- Dealer、Party 0、Party 1 为三个独立进程；
- Dealer 在在线阶段无连接和消息；
- 网络抓取或 Peer counters 能复算每方 sent/received、total 和 per-party；
- 在线 barrier 可复算为 3；无 `.bin` 同步、固定 sleep 或 secure 路径重构；
- 先通过小规模固定向量，再运行 `(128,2/8)` 和 `(256,2/8)`。

## 9. 指标与实现门

正式记录使用 `MetricsRecord`，实现标签为
`agarwal_protocol_iii_modular_3round`，并完整记录 revision、输入/seed、编译环境、
网络、重复次数、correctness、offline/online 时间、离线材料、分方通信、3 个在线
轮次、PRG calls 和 `n(n-1)/2` comparison edges。尚未接通的字段写
`NOT_MEASURED`，不得从论文公式或旧日志估算。

开始 M3 代码前必须满足：

1. M1.1 的 CTest 和 metrics provenance 已合并；
2. M2 的真实 shuffle、统一 mask 和独立进程 E2E 已通过；
3. M2 提供可复用的 rank share 与运行/通信接口；
4. DPF 和无重构乘法 adapter 的 conformance 设计经过评审。

## 10. 明确禁止与非目标

- 不使用旧原型的明文 `true_rank` Dealer；
- 不重构 rank 后再生成 masked rank；
- 不使用 `.bin` 文件、文件大小模拟通信量或固定 sleep；
- 不直接复制旧 DPF key 内存布局或修改 VFSS 原语语义；
- 不使用 AAV86 图代替全对全 CmpAgg；
- 不在 M3 中加入域逆元、非零 payload 编码或跨阶段压缩；
- 不使用 `agarwal_protocol_iii_exact_2round` 或论文 Theorem 4.2 复现标签。
