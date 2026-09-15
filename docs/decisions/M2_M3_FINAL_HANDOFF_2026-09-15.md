# M2/M3 最终交接契约冻结

日期：2026-09-15
适用 revision：`5bd879e8c4d053b14aae61f19bbaf53529446025`
代码/测试证据提交：`fc149c2e33df53a272208622da925d0047303121`

本文冻结阶段三N之后供规划与后续实现读取的标签、输入输出和轮数契约。标签不得因为
测试通过而重命名；任何改变输入语义、输出、角色、轮数、公开值、预处理边界或泄露的
工作必须新建 decision record。

## 1. 固定标签与证据边界

| 路径 | 固定 implementation label | 输入 | 输出 | 在线因果轮数 | 身份 |
|---|---|---|---|---:|---|
| M2 formal baseline | `m2_protocol_i_raw_score_input_modular_8round_mask_output` | signed Q20.12 raw-score shares | original-order XOR Top-K mask | 8 | 工程基线，不是论文三轮 |
| M2 candidate | `m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate` | padded priority-key additive shares | shuffled-domain rank shares | 3 | candidate，不是完整 mask |
| M2 priority Route A | `m2_protocol_i_dealer_preprocessed_rank_reveal_6round_mask_output` | padded priority-key additive shares | original-order XOR Top-K mask | 6 | 项目扩展 |
| M2 raw Route A | `m2_protocol_i_raw_score_dealer_preprocessed_rank_reveal_8round_mask_output` | signed Q20.12 raw-score shares | original-order XOR Top-K mask | 8 | 项目扩展 |
| M3 modular | `agarwal_protocol_iii_modular_3round` | padded priority-key shares | original-order XOR Top-K mask | 3 | 模块化工程基线 |
| M3 raw extension | `moe_topk_protocol_iii_raw_score_modular_5round` | signed Q20.12 raw-score shares | original-order XOR Top-K mask | 5 | M3 项目扩展 |

M2 Route A 不是 M3；M3 不调用 M2 Route A symbols，不解析 `RA6M`/`RA8M`，不使用 M2
public carrier，不是 Protocol III exact 2-round。不得使用
`agarwal_protocol_i_exact_mask_output` 或 `agarwal_protocol_iii_exact_2round` 标签描述
当前实现。

## 2. 共享输入/输出契约

### M2

- score 表示为 signed Q20.12 int32；priority key 在比较域内绑定原始 index 以定义稳定同分；
- candidate/priority Route A 的 secure 输入域是 `padded_n`；dummy 只存在内部 padded 域，
  不进入最终输出；
- candidate 的公开对象是 shuffled masked list，candidate 输出仅为 shuffled-domain rank
  shares；
- Route A 在 candidate 后公开 shuffled-domain rank 形成 public selection carrier，再执行
  两个 reverse `Permute+Share` stages；rank reveal 和 reverse shuffle 都计入轮数；
- Route A 输出长度为 `logical_n`，是原始输入顺序的 XOR-shared Top-K bit-mask，重构只在
  TEST_ONLY harness；
- stable ties 使用较小 original index 优先；支持 K=1、K=n、合法 K=2/K=8、非 2 次幂 n、
  negative 和 int32 extremes。

### M3

- priority-key 输入长度为 `padded_n`；GRank comparison graph 只处理 `logical_n` 真实位置；
- DPF routing domain 固定为 `2^rank_bits`；
- online core 固定 `GRank -> DPF routing -> secure combine` 三轮；raw-score 入口额外为
  carry、sign 两轮，因此总计 5 轮；
- 输出为原始顺序长度 `logical_n` 的 XOR-shared Top-K bit-mask；rank、DPF index、selected
  index、明文校验数据和 final mask 不在 secure runtime 重构；
- M3 使用自身 package/material/serialization；不复用 M2 Route A rank reveal/reverse adapter。

## 3. 角色、预处理与 frame 边界

Dealer 只在输入未知的 offline 阶段生成并交付 input-independent material；在线阶段只运行
P0/P1。Dealer handoff、socket 建连、local carrier derivation、TEST_ONLY result reconstruction
不计入 causal online rounds。controller 不注入 carrier、不重排数组、不提供明文 rank/index。

M2 Route A 的 `RA6M` 与 `RA8M` 是项目 frame identity，不是论文 transcript。任何新 frame
必须有新的 identity、material id、phase/sequence 检查和 decision record。M3 frame 与 M2
Route A frame 完全分离。

## 4. provenance 与当前状态

当前 HEAD 为 `5bd879e8c4d053b14aae61f19bbaf53529446025`。阶段三M的代码/测试证据绑定
`fc149c2e33df53a272208622da925d0047303121`，当前 HEAD 是其文档-only 直接子提交；阶段三L
实现基准为 `71c161290d6627e8a9521a8733e6061b9afed861`。`f752a77` 仅为 amend 前 dangling
commit，不能用于新的最终证据标签。

阶段三N fresh Ubuntu/EMP 结果：EMP-ON `ctest -N`=34；full CTest 34/34；M2 focused 19/19；
M3 focused 11/11；candidate 1/1；priority Route A 1/1；raw Route A 1/1；M3 raw-score
5-round focused 3/3。EMP-OFF production build 与 EMP-ON production candidate build 均通过。

## 5. 交接规则

1. 后续报告必须沿用上表标签和轮数，不得把 Route A 写成论文原生三轮或把 M3 写成 exact
   2-round。
2. 任何新增 round、公开值、adapter、payload、rank/index 语义或 Dealer 依赖都必须显式
   出现在 metrics、trace、leakage table 和 decision record 中。
3. 性能、RTT、带宽、PRG calls、逐 barrier bit counters 在尚未实际采集前写
   `NOT_MEASURED`，不得借用历史 CTest 用时。
4. paper-exact Protocol I 保持 `BLOCKED / NOT_VERIFIED`，直到独立 exact gate 的 transcript、
   same-permutation、correlated material、leakage equivalence 和 E2E 证据全部通过。
5. 不修改 `VFSS-baseline/`、`Papers/`、`Agarwal_TopK/`、`ADSMPC/`、`CipherGPT/` 来满足
   M2/M3 交接；不通过改名或隐藏 adapter 解锁 exact。
