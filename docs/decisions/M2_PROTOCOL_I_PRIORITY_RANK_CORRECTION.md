# M2.2 Protocol I priority-rank 语义纠错与 reverse-shuffle 规格审计

状态：**`m2_protocol_i_design_blocked`**。本记录纠正 M2.0/M2.1 的 rank 映射和 bit-share 结论；不修改协议代码，也不构成 M2 实现授权。

证据分类：**A** 为论文明确内容；**B** 为本地参考/VFSS 行为；**C** 为本项目工程契约；**D** 为缺失或未验证证据。不得以 B/C/D 反写论文结论。

## 1. rank 的三个不同概念

| 名称 | 定义 | 证据 |
| --- | --- | --- |
| `rank_A` | Agarwal 的原始 score 升序 stable rank：较小 score 在前；同分时较小原始下标在前。 | A，Agarwal PDF p.6，§2.4/§3.1 |
| `rank_P` | 项目 priority rank：score 降序；同分时较小 `original_index` 在前；`rank_P=0` 为 Top-1。 | C；M1 score 语义和 oracle |
| priority-key 升序 rank | 对 `priority_key` 做升序 stable CmpAgg 的结果。 | C；候选 M2 输入编码 |

`rank_A` 和 `rank_P` **不是**普遍逐项满足 `rank_P=n-1-rank_A`。例如三个同 score 的原始位置 `0,1,2`，论文稳定 rank 为 `(0,1,2)`，项目 priority rank 也为 `(0,1,2)`，而 `n-1-rank_A` 是 `(2,1,0)`，反转了同分组内已经冻结的“较小 index 优先”规则。此前出现的全局反转等式因此错误，必须删除。

候选项目编码为：

```text
ordered_score = raw ^ 0x80000000
priority_key = ((UINT32_MAX - ordered_score) << index_bits) | original_index
index_bits = max(1, ceil(log2(n)))
```

较小 key 表示较高项目优先级；相同 score 时较小 index 仍给出较小 key。因此若对该 key 的升序 total order 运行 stable CmpAgg，结果**定义为** `rank_P`，直接选择 `selected=(rank_P<K)`。这是 C 类输入编码/rank 契约，不是论文对原始 score rank 的定义。

若 M2 选择公开 rank，公开的只能是 `(shuffled_slot, rank_P)`：它泄露的是项目 priority-key order 的 shuffled-domain total order。它与公开原始 score 上的 `rank_A` 在非重复 score 时可由反转对应，但在存在重复 score 时不是相同信息，不能混称。D1 仍等待用户确认其泄露策略、受众和零敏感转录保留规则。

M3 若复用 project priority rank，使用 `rank_P` 的环/范围/party 契约；不得再使用 `paper_rank=n-1-priority_rank`。原始 score 的论文 stable rank 与项目 priority rank 在有重复值时没有全局逐项反转关系。

## 2. arithmetic bit share 到 XOR bit share

令两方在同一环 `Z_(2^w)` 中持有 additive shares，且 secure routing 已保证重构值是 bit：

```text
a0 + a1 = b (mod 2^w),  b in {0,1}.
```

模 2 取该等式得 `(a0 mod 2)+(a1 mod 2)=b (mod 2)`；在 bit 上的模 2 加法等于 XOR，故：

```text
(a0 & 1) XOR (a1 & 1) = b.
```

双方可局部取最低位；不需要额外 correlated randomness、通信或 `mask_adapter_rounds`。secure runtime 不得重构或验证 `b`；“输出确为 0/1”的验证只属于 conformance/test。此 C 类转换与 M3 的最低位规则一致，但 M3 DPF conformance 不是 M2 inverse-routing 证据。该纠正不解决 D2。

## 3. reverse Permute+Share 审计：D2 仍阻塞

Chase 的 `Permute+Share` 功能是：permutation owner 输入 `pi`，data owner 输入 `x`，输出随机 additive shares `(r, pi(x)-r)`（A，`协议1shuffle.pdf` §6.2，pp.19--21）。其 forward secret-shared shuffle 固定为：P0 选 `pi0`，先作用于 P1 share；P0 局部加入 `pi0(x0)`；P1 选 `pi1`，再作用于该中间 share；P1 局部加入 `pi1` 作用后的另一 share；输出合成置换 `pi=pi1 o pi0`（A，§6.3，pp.21--23）。论文证明该**正向**两次 sequential `Permute+Share` 的 static semi-honest secret-shared shuffle。

对 carrier 的数学逆需要 `pi^-1=pi0^-1 o pi1^-1`，所以所需局部 permutation 顺序与 forward 相反：先使用 P1 的 `pi1^-1`，再使用 P0 的 `pi0^-1`。但是论文没有为“从已经 secret-shared shuffle 输出回到原顺序”的独立 reverse functionality 给出消息级协议、fresh preprocessing 分配、role-swapped transcript、opened values、round accounting 或组合安全证明。它也没有授予复用 forward masks/OT/Share Translation materials 的权限。

本地 B1 有 `inverse_permutation`，并有 `run_permutation_owner_pass`/`run_data_owner_pass` 与 `owner_direction`（B，`runtime/src/b1/algebra.cpp`、`real_two_pass_shuffle*.cpp`）。但 B1 使用 OPV/Share Translation、CryptoTools transport、helper process、私有 artifact、路径、端口和旧 ABI；它没有 VFSS reverse primitive API，也不能成为 VFSS 的实现前提。特别是“有 inverse_permutation 函数”只证明本地置换数组可逆，不证明安全 inverse routing。

因此本审计选择结论 **B**：D2 没有消息级、可实现、可测试的 secure reverse primitive 规格，继续为 D 阻塞。不得称“论文支持 reverse 两遍”。未来要解除 D2，必须先提交并评审一个独立 primitive 规格，至少包含：两次调用是否 role-swapped、每次的 owner/data owner/输入输出 share、全量离线 OT/translation/randomness、每条 online message 与唯一 opened value、freshness/non-reuse、错误/EOF 语义、`mask_adapter_rounds`，以及 record-binding、oracle differential、两方 hiding、三进程 E2E 验收。不得用 B0、单方 permutation、明文 post-processing、文件、helper 或旧 ABI 补位。

## 4. D3：半环 promise 与待验证范围

Agarwal 的 uCMP 输入是 `(x,y) in Z_N x Z_N`，且只在 `|x-y|<N/2` 时承诺输出 `[x>y]`；超出时无保证（A，Agarwal PDF p.6，§2.3）。对未掩码合法 priority key，选择候选 `N=2^(key_bits+1)` 可使最大差 `2^key_bits-1 < N/2`。这只证明 key 明文代表元的数学半环界；FSS gate 的实际 masked-difference、mask sampling/offset、DCF threshold、shift、序列化和非法输入仍须独立 conformance。

| n | index_bits | key_bits | `N` 候选 | 候选 DCF/uCMP `Bin` | key 端点 | 未掩码前提 | VFSS 已证明？ |
| ---: | ---: | ---: | --- | ---: | --- | --- | --- |
| 1 | 1 | 33 | `2^34` | 34 | `[0,2^33-1]` | `|x-y|<=2^33-1<2^33` | 否 |
| 128 | 7 | 39 | `2^40` | 40 | `[0,2^39-1]` | `<2^39=N/2` | 否 |
| 256 | 8 | 40 | `2^41` | 41 | `[0,2^40-1]` | `<2^40=N/2` | 否 |
| 1,000 | 10 | 42 | `2^43` | 43 | `[0,2^42-1]` | `<2^42=N/2` | 否 |
| 10,000 | 14 | 46 | `2^47` | 47 | `[0,2^46-1]` | `<2^46=N/2` | 否 |
| 100,000 | 17 | 49 | `2^50` | 50 | `[0,2^49-1]` | `<2^49=N/2` | 否 |
| 1,000,000 | 20 | 52 | `2^53` | 53 | `[0,2^52-1]` | `<2^52=N/2` | 否 |

M1 的 raw 32-bit DCF conformance 不覆盖这些 bins。VFSS 的 `GroupElement=uint64_t` 和 `keyGenDCF(int Bin,...)` API（B）不等价于对 34--53 bit uCMP 的安全支持。未来 conformance 必须覆盖所有表点、两端 key、最小/最大 mask difference、masked gate input、comparison direction、padding/unused index、`K=0/K>n`、越界 key 与非法/危险 `Bin`；所有失败均硬错误。候选 `Bin=key_bits+1` 是 D3 的待验证工程要求，不是已批准或已验证的 VFSS 行为。

## 5. 实施状态

| 项目 | 状态 | 是否可以批准为设计契约 | 是否已经可以实现 |
| --- | --- | --- | --- |
| D1 priority-rank 泄露 | C：精确定义为公开 shuffled `(slot,rank_P)`；仍待用户确认泄露 | 仅可作为待用户确认策略 | 否 |
| D2 reverse routing | D：缺消息级 secure primitive | 否 | 否 |
| D3 uCMP/DCF 范围 | C/D：有未掩码半环候选，缺 VFSS conformance | 仅可作为待 conformance 要求 | 否 |
| D4 transport/metrics | C/D：已有最小 fail-closed contract，未实现/E2E | 仅可作为待 conformance 要求 | 否 |

在 D2 未闭合前，不建议创建 `codex/m2-protocol-i`。即使未来允许原语 conformance，也不得提前使用 `agarwal_protocol_i_exact_mask_output`。

**M2 仍处于 design_blocked；priority-rank 语义和 reverse-routing 规格未经批准及验证前，不授权开始 M2 代码修改。**
