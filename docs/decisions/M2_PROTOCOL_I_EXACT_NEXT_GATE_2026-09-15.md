# M2 Protocol I 论文精确三轮下一道门

状态：`M2_PAPER_EXACT_BLOCKED / NOT_VERIFIED`
日期：2026-09-15
当前 revision：`5bd879e8c4d053b14aae61f19bbaf53529446025`

本记录将当前工程扩展与 `agarwal_protocol_i_exact_mask_output` 的进入条件分开。只有下表
所有论文必要条件都有清晰来源、实现契约和实际证据时，才允许创建独立的
paper-compatible implementation stage。当前 Route A、M2 candidate、M3 DPF material 均
不能直接满足或替代该门。

## 1. Exact gate 表

| Gate | 论文要求 | 当前证据 | 状态 | 最小缺口 |
|---|---|---|---|---|
| 三轮 core transcript | 每轮 sender/receiver、消息和依赖完整可复核 | 会议版没有足够逐消息 transcript | BLOCKED/UNKNOWN | 论文/作者材料 |
| public masked-list | 公开 list 的确切构造、域和语义 | 当前 Route A 是项目 shuffled masked list | BLOCKED | 冻结 paper-native semantics |
| same-permutation binding | score/payload/index 与同一隐藏置换绑定 | 工程 permutation material 存在，论文 binding 无证明 | BLOCKED | 形式化 binding 与 differential |
| hidden permutation party view | 每方看到什么、什么保持 secret | 当前工程 view 不能反推论文 view | BLOCKED/UNKNOWN | party-view transcript |
| correlated material `r` | `r` 生成、分发、消费及其与 permutation 的相关性 | material id/share contract 有，相关性证明无 | BLOCKED | correlation proof/fixture |
| offline Dealer | 输入无关、离线、静默、无 online fallback | 当前项目 Dealer 满足工程约束 | PROJECT PASS / PAPER UNKNOWN | paper model confirmation |
| secure rank computation | 不重构 rank/index 完成 core | candidate secure runtime 通过，Route A R4 会公开 rank | BLOCKED | paper-compatible rank path |
| stable ties | 同分的原始 index 语义 | 项目 stable oracle 已冻结 | PROJECT PASS / PAPER UNKNOWN | paper boundary |
| padded dummy | dummy 是否存在、如何排除、输出边界 | 项目内部 padded，输出 logical_n | PROJECT PASS / PAPER UNKNOWN | paper boundary |
| output semantics | 论文原生输出与 project mask adapter 的区别 | candidate 只有 rank share；Route A 有额外 reverse rounds | BLOCKED | native output definition |
| leakage equivalence | 当前公开对象与论文 leakage simulator 等价 | Route A rank reveal 是新增公开对象 | BLOCKED | exact leakage simulator |
| primitive conformance | shuffle/compare/material ABI 与 paper primitive 对齐 | VFSS project primitives conformance PASS | PROJECT PASS | paper mapping |
| oracle differential | stable signed oracle、padding、边界 K/n | M2 current tests PASS | PROJECT PASS | exact core oracle |
| independent-process E2E | Dealer/P0/P1 的 paper-native 3-round path | candidate/Route A E2E PASS，但不是 native core | BLOCKED | new executable/harness |
| metrics/provenance | label、revision、round、输入、原始 counters | Stage3N provenance and logs PASS | PROJECT PASS | paper transcript and bit-level trace |

## 2. 当前实现不能用作 exact 证据的部分

- `m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate` 只产生 shuffled-domain
  rank shares，不能改名为完整 mask output；
- priority-key Route A 的 R4 rank reveal 改变了泄露，R5/R6 reverse shuffle 不是免费步骤；
- raw-score Route A 另外有 R0/R0b carry/sign adapter，8 轮不能压成论文三轮或项目 7 轮；
- `RA6M`/`RA8M` frame、public carrier、M3 DPF/triple material 和 local reference code 都
  不能作为论文 transcript 或安全证明；
- TEST_ONLY oracle/reconstruction 只能证明当前项目输出正确，不能证明论文泄露等价。

## 3. Blocker closure

### 必须从论文/作者材料取得

1. 三轮准确阶段划分和每轮消息 transcript；
2. public masked-list 的数学定义及允许公开字段；
3. hidden permutation 的 party view 与 same-permutation binding；
4. correlated `r` 的生成、分发、消费、失败语义；
5. stable ties、padding、原始输出和安全模型的边界；
6. 当前会议版缺失细节的权威补充或作者确认。

### 可以由 VFSS 设计补齐

1. 与论文 transcript 一一对应的新 material/package state machine；
2. 不重构 rank/index 的 paper-compatible secure core；
3. primitive conformance、oracle differential 和独立 Dealer/P0/P1 E2E；
4. 每个 causal barrier 的 sender/receiver/frame/phase/material id/payload/opened value/
   sent_bits/received_bits 原始 trace；
5. exact leakage simulator 和 paper-vs-project separation；
6. negative tests、stable ties、dummy padding、K/n 边界和 reproducible provenance。

### 预期受影响与禁止修改的文件

预计会新增独立 M2 decision/design record、paper-compatible primitive/header/source、
conformance/differential/E2E tests、metrics/trace schema 和 reproduction report。不得修改
`VFSS-baseline/`、`Papers/`、`Agarwal_TopK/`、`ADSMPC/`、`CipherGPT/`，不得覆盖 formal
8-round baseline、candidate、Route A 或 M3 标签，也不得复用 M3 DPF material 代替论文材料。

## 4. 进入下一阶段的验收顺序

1. 先在 decision record 中写完整三轮 transcript、party view、材料契约、公开/秘密字段和
   失败语义；
2. 通过设计审查后实现最小 primitive；
3. 依次通过 primitive conformance、oracle differential、independent-process E2E；
4. 由实际 transport/barrier trace 核对三轮，不从函数名或标签推断；
5. 完成 leakage simulator 与 paper/local-reference separation；
6. 形成新的 revision-bound reproduction report，再决定是否将 exact gate 改为 GO。

在这些条件完成前，下一步建议是保持 `M2_PAPER_EXACT_BLOCKED / NOT_VERIFIED`，维护当前
M2/M3 工程基线并转入 M3/后续任务，而不是从 Route A 复制代码后改名。
