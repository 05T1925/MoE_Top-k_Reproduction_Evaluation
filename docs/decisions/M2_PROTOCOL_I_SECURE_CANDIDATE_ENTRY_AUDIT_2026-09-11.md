# M2 Protocol I isolated secure candidate entry audit

状态：**ENTRY_BLOCKED（旧安全模型的历史审计）**。

日期：2026-09-11。基准为阶段三A提交 `8f11a4d0aa53ca75dd06536ebc55b0f847750837`。本记录审计的是“P2 不得知道完整 `r`”的旧安全模型，不是当前 Dealer-preprocessed candidate 的最终状态。当前候选的显式模型和实现门见 `M2_PROTOCOL_I_DEALER_PREPROCESSED_3ROUND_CANDIDATE.md`；该候选仍不改变论文 Gate C 的 BLOCKED 结论。

## 1. 审计范围和证据等级

本审计区分：

- A：已校验的 Agarwal Protocol I 与 Chase shuffle 论文定义；
- B：当前 VFSS API、实现和测试的静态/执行行为；
- C：项目冻结的 signed Q20.12、stable tie、logical/padded、transport 和 original-order mask 契约；
- D：阶段三A中心化 TEST_ONLY 模型及其理想化设想。

阶段三A模型只提供 algebraic ideal-function model 证据。本审计不把它升级为 distributed secure construction、executable causal transcript、GRank/FSS correlated-material generation 或安全视图证据。

## 2. 阶段三A证据口径修正

阶段三A的 54 组模型用例和 CTest 只能支持：

| 结论 | 状态 | 边界 |
| --- | --- | --- |
| algebraic ideal-function model | PASS | 模型内 staged/direct permutation、`r` 等式和 payload oracle 成立 |
| distributed secure construction | BLOCKED | 没有真实 P0/P1 message/material construction |
| executable causal transcript | BLOCKED | O0/R1/R2/R3 是目标表，不是实际 transport trace |
| GRank/FSS correlation generation | BLOCKED | 模型直接把 `r0/r1` 作为 fixture material，没有真实可消费的生成协议 |

阶段三A模型还使用了与当前项目不同的域假设：模型在 `logical_n=1` 时使用 `padded_n=1`，而 `protocol_i_make_input_layout` 从 `padded_n=2` 开始；模型把 `key_bits` 作为环宽，而当前 M2 pipeline 的最小 `comparison_bits` 是 `32 + index_bits + 1`。因此阶段三A不能作为项目 width/layout conformance 证据。

阶段三B只修正了阶段三A遗留的构建边界：该 TEST_ONLY 模型 target 现在置于 `BUILD_TESTING` 条件内，`BUILD_TESTING=OFF` 的生产配置不会默认编译它。此修正不新增 secure source、secure target 或正式 M2 接入。

## 3. 当前实际 Permute+Share 能力

### 3.1 可调用接口

`protocol_i_permute_share.h` 暴露的 PO/DO material 是 move-only、one-shot material。PO material 持有自己的完整局部 permutation、Benes layer 和 delta；DO material 持有 `a/b` shares。每次 `protocol_i_permute_share_online_po/do` 的计数器都固定为 `ps_online_rounds=1`。

`protocol_i_secret_shared_shuffle.h` 的 forward 是两个顺序的 Permute+Share 调用，party-local material 只保存 own permutation/inverse 和四组 PS material；其输出类型只有 `ProtocolIBlock192` secret share，没有 public masked list、`r` 或 GRank material 字段。

### 3.2 实际单遍消息代数

对一次 PS，DO 在本地计算：

```text
m = x + a[0]
cv[z] = a[z] - b[z-1]
w <- fresh random share
```

DO 发送固定的 hello 和 `m`、所有 `cv[z]`、`w`；PO 使用自身 `delta`、Benes permutation 和收到的 values 计算 `p(m) + acc - w`。DO 返回 `w - b[last]`。这些消息可以支持单遍 secret-shared Permute+Share，但没有生成 `public_y = pi(x) + r` 的消息表达式，也没有把任意 `r` 绑定进输出。

### 3.3 实际 forward 两轮

当前 `protocol_i_shuffle_forward_party` 的静态因果顺序为：

| 当前 pass | Party 0 | Party 1 | 结果 |
| --- | --- | --- | --- |
| forward pass 1 | PO，接收 P1 input share 的 PS 消息 | DO，发送 P1 input share 的 PS 消息 | 形成第一份 secret share |
| forward pass 2 | DO，发送 `p0(input0)+share` 的 PS 消息 | PO，接收并应用 `p1` 的 PS 消息 | 双方得到 secret-shared `p1(p0(x))` |

这里的两轮只证明当前 secret-shared payload 路径的能力。它没有同时给出 public masked list，也没有给出与 public list 同槽、同 permutation、同 `r` 的 GRank material。因此不能把当前两轮直接当作阶段三B要求的 R1/R2 candidate 消息。

## 4. R1/R2 入口审计

### R1

- sender/receiver：当前为一次 PS 的 DO → PO 消息，双向 hello/transport handshake；
- sender 当时拥有：本方 input share、DO offline `a/b` material；PO 拥有自身局部 permutation、delta 和 PS offline material；
- 逐槽表达式：发送 `m[s] = x_do[s] + a_0[s]`、`cv_z[s] = a_z[s] - b_{z-1}[s]`、随机 `w[s]`；
- receiver 新值：PO 可得到该 pass 的 secret share；
- 安全边界：消息不直接发送 DO 的 permutation，但当前接口也不产生 public `y` 或相关 `r`；
- 结论：**payload-only R1 = PASS；candidate public-list R1 = BLOCKED**。

### R2

- sender/receiver：当前为第二次 PS 的 DO → PO/PO → DO 配对，依赖 R1 输出和本地 permutation；
- sender 当时拥有：第二次 PS 的本地输入 share、对应 offline material 和 R1 state；
- 逐槽表达式：仍是第二次 PS 的 `m/cv/w` 结构，输出为第二次 secret share；
- receiver 新值：双方可得到 `Perm(p1, Perm(p0, x))` 的 additive shares；
- 缺失值：没有任何现有返回值或 frame 表达 `public_y = Perm(pi, x) + r`，也没有 public-list slot 与 GRank material 的 binding；
- 结论：**secret payload R2 = PASS；public list + same-r R2 = BLOCKED**。

因此当前 API 不能满足“R1/R2 两轮结束后双方同时拥有 secret-shared `Perm(pi,x)`、public `y` 和同槽 correlated GRank material”的入口条件。把后续 masked-key frame 重命名为 R2 不构成证明。

## 5. correlated r 与 uCMP/FSS material

当前 `ProtocolIUcmpMaterial` 的唯一构造接口为：

```text
ProtocolIUcmpMaterial(bits, mask_left, mask_right)
alpha = mask_left - mask_right mod 2^bits
keys = keyGenDCF(bits, 64, alpha, 1)
```

该构造在一个调用方同时接收完整 `mask_left` 和 `mask_right`，然后再 export 两份 party material。当前 `ProtocolIPartyPackage` 只序列化 party-local `node_mask_shares` 和已生成的 edge DCF material；它没有 candidate 的 `material_id`、public-list slot/r binding 或同一 `r` 的生成证明。

当前仓库没有可调用的“P0/P1 分布式生成 correlated DCF/uCMP key”的新原语。现有 M2 测试也由 controller/P2 先生成 `full_masks`，再计算两方 shares，并用完整 mask pair 构造 `ProtocolIUcmpMaterial` 后 export party material。该行为是测试/中心化材料生成，不能迁入 secure candidate。

因此三种方案的审计结果为：

| 方案 | 结果 | 原因 |
| --- | --- | --- |
| P2 知道完整 `r` | BLOCKED | 与当前 P2 不得知道完整 `r` 的泄露契约冲突 |
| P0/P1 分布式 offline 生成 | BLOCKED | 当前没有该 material-generation primitive、消息和 conformance |
| 其他已证明构造 | BLOCKED | 没有可消费且经证明的替代材料格式/评估接口 |

Gate B 要求的 correlated r/FSS material generation 未成立，不能继续写 secure source。

## 6. R3 与 GRank 的实际语义

当前 `protocol_i_priority_pipeline_party` 的行为是：

1. forward PS 输出本地 secret share；
2. 本地加上 `package.node_mask_shares`；
3. 通过 phase 2 的 `cmp_channel` 交换两方 masked-key share，并在两方本地计算 `opened[index] = local[index] + peer[index]`；
4. 对共同的 opened masked keys 做本地 DCF/CmpAgg evaluation，得到本地 `ranks` share；
5. 通过 phase 3 的 `rank_reveal_channel` 交换两方 `ranks`，并计算 `rank = ranks + peer_ranks`；
6. 根据公开 shuffled rank 形成 reverse carrier。

这意味着当前 R3 实际会重构 shuffled-domain rank，并不是“发送 rank shares 但保持 rank secret”。当前 API 的本地 DCF evaluation 可以产生 local rank share，但后续 rank reveal 明确把它打开给双方。它不能复用为论文安全 R3。

candidate core 的目标输出应是 shuffled-domain rank shares，并且本阶段不实现 original-order reverse/output mask adapter。要达到该目标，仍需新的真实 GRank/FSS 输入、输出和 material contract；当前实现没有它。

## 7. 实际 causal dependency graph

以下是从当前代码调用顺序得到的静态图，不是阶段三A候选的假设轮数。发送/接收的实际 wall-clock timestamp 未被当前实现记录，因此记为 `NOT_MEASURED`；sequence/header 字段只证明 frame 顺序，不替代 runtime trace。

| Barrier | Phase/sequence | sender/receiver | payload | dependency | send/receive completion |
| --- | --- | --- | --- | --- | --- |
| 1 | forward PS pass 1 | P0/P1 按 PO/DO 角色 | PS hello、`m/cv/w` | input share + PS offline material | 源码顺序可见；timestamp `NOT_MEASURED` |
| 2 | forward PS pass 2 | P0/P1 按 DO/PO 角色 | PS hello、`m/cv/w` | barrier 1 output + second PS material | 源码顺序可见；timestamp `NOT_MEASURED` |
| 3 | phase 2/type 1 | P0 ↔ P1 | padded masked-key words | forward output + node mask share | P0 先 send/receive，P1 先 receive/send；运行 timestamp `NOT_MEASURED` |
| 4 | phase 3/type 1 | P0 ↔ P1 | padded rank-share words | local DCF ranks | P0 先 send/receive，P1 先 receive/send；运行 timestamp `NOT_MEASURED` |
| 5 | reverse PS pass 1 | P0/P1 按 DO/PO 角色 | PS hello、`m/cv/w` | opened rank carrier | 源码顺序可见；timestamp `NOT_MEASURED` |
| 6 | reverse PS pass 2 | P0/P1 按 PO/DO 角色 | PS hello、`m/cv/w` | barrier 5 output + inverse PS material | 源码顺序可见；timestamp `NOT_MEASURED` |

当前 priority-key path 的实际工程 graph 是 `2 + 1 + 1 + 2 = 6` 个 barriers；加上 raw-score carry/sign 两轮后是当前 `8` 轮。candidate 目标 `2 + 1 = 3` 不能从当前 graph 推导，因为它缺少 public-list/r binding 和真实 GRank material path。

## 8. domain/width/padding 复核

当前 M2 项目布局来自 `protocol_i_make_input_layout`：

- `1 <= logical_n <= 1,000,000`，`1 <= k <= logical_n`；
- `padded_n` 从 2 开始并向上取 2 的幂，因此 `logical_n=1` 时 `padded_n=2`；
- `index_bits = bit_width(padded_n - 1)`；
- `minimum_comparison_bits = 32 + index_bits + 1`；
- uCMP 当前支持 `comparison_bits` 34..53；
- `protocol_i_priority_key` 的 priority-key 本身按 `index_bits(logical_n)` 编码为 `32 + index_bits` 位，和 comparison ring 的额外安全位不是同一个字段；
- M2 pipeline 没有独立的 candidate `rank_bits` 契约，而是以 comparison ring 重构并检查 rank；
- 当前 raw-score adapter 的 padding 和 M3 GRank 的 logical/padded 语义不能直接替代 candidate dummy/comparison graph 设计。

阶段三A模型没有覆盖这些差异，故 width、ring、rank_bits、dummy key 和 padding comparison graph 的 candidate contract 仍为 BLOCKED。

## 9. Entry Audit 决策

| Entry 条件 | 结果 | 最小证据/缺口 |
| --- | --- | --- |
| R1/R2 每条消息有 candidate 可执行代数 | BLOCKED | 只有 payload PS 代数，没有 public `y`/same-r/GRank 代数 |
| 不需要中心方掌握禁止对象 | BLOCKED | 当前 DCF 生成需要完整 mask pair；测试由中心 controller 生成 |
| correlated uCMP/FSS material 可真实生成 | BLOCKED | 无分布式生成 primitive/conformance |
| R3 产生契约规定的 secret rank shares | BLOCKED | 当前 rank reveal 会重构 shuffled rank |
| 实际 dependency graph ≤ 3 online barriers | BLOCKED | 当前 priority graph 为 6，raw-score 为 8；候选 3 未从代码 trace 证明 |
| input/padding/ring/comparison width 与契约一致 | BLOCKED | 阶段三A模型与项目 layout/width 不一致 |
| 不需修改 M3 或当前 M2 8-round 路径 | PASS | 本审计未修改这些路径 |

最终结论：**ENTRY_BLOCKED**。

按照停止条件，本阶段不新增 secure candidate header/source、secure 独立 executable、空壳 API 或 secure 测试。仅将阶段三A已有的 TEST_ONLY 模型 target 置于 `BUILD_TESTING` 条件内；没有新增 target。也不运行阶段三B的 conformance、oracle differential、fork+exec E2E 或完整 CTest 作为“通过”证据；阶段三A的 25/25 仅保留为已降级的 algebraic ideal-function model 历史证据。

## 10. 重新进入 Gate B 的最小前置项

1. 独立冻结 candidate package/material schema，不复用现有 M2 package instance；
2. 设计并实现不要求任何一方掌握完整 `r` 的 correlated uCMP/FSS material generation primitive；
3. 对两次真实 PS/OT/Share Translation 消息给出逐槽 public-list、payload、slot binding 代数；
4. 定义真实 GRank 输入、输出和不打开 rank 的 R3 语义；
5. 用实际 transport send/receive trace 证明三个 online barriers；
6. 补齐 project width/layout/dummy/rank_bits conformance 后，才可重新审查 `ENTRY_GO`。

任何后续实现仍必须保持当前 M2 8-round 路径、M3 路径、共享契约、baseline 和参考目录不变，并继续禁止 exact 标签。
