# 阶段三O决策：M2 Protocol I paper-compatible 设计门

日期：2026-09-15
当前基准 revision：阶段三N clean closeout `bcb5dd3a4ea144817d5b7f5e7e673d3f2d214d9a`；
阶段三O documentation-only closeout 的完整 hash 以最终 `git rev-parse HEAD` 为准。
决策：`DESIGN_BLOCKED`
实现状态：`IMPLEMENTATION_NO_GO`
论文状态：`M2_PAPER_EXACT_BLOCKED / NOT_VERIFIED`

## 决策范围

本记录只决定是否允许新建独立 paper-compatible Protocol I 设计/实现阶段，不改变：

- `m2_protocol_i_raw_score_input_modular_8round_mask_output`；
- `m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate`；
- `m2_protocol_i_dealer_preprocessed_rank_reveal_6round_mask_output`；
- `m2_protocol_i_raw_score_dealer_preprocessed_rank_reveal_8round_mask_output`；
- `agarwal_protocol_iii_modular_3round`；
- `moe_topk_protocol_iii_raw_score_modular_5round`。

## 论文事实边界

本地唯一 Agarwal 论文基线是 15 页 CCS 2024 会议版，SHA256
`18faf63eaa7923eef715a6eb9d5d526fe04dc69700b133c3e94de935f68c01c`。它明确给出：

- Table 1（p.3）：Protocol I = 2+1、Shuffle、CmpAgg、3 online rounds；
- §2.4（p.6）：secure shuffle 输出 secret-shared shuffled list 以及公开
  `π(x)+r`，其中 r 是单方未知随机 mask；
- §4.1（p.8-9）：先 shuffle，再计算并公开 shuffled stable ranks，routing 可在 clear
  对 secret-shared values 执行；
- Theorem 4.1（p.9）：3-round Protocol I 的高层成本和安全目标；proof/完整 instantiation
  指向 full version。

未确认：逐消息 sender/receiver、party view、π/r correlated material、original-index
binding、padding/dummy、原生 mask output、adapter 是否计入 3 轮和 exact leakage simulator。
这些字段统一写 `UNKNOWN_FROM_AVAILABLE_PAPER`，不得依靠本地代码猜测。

## Gate 结论

| Gate | 状态 |
|---|---|
| 论文来源充分性 | BLOCKED / `PAPER_SOURCE_INCOMPLETE` |
| 输入输出兼容性 | BLOCKED |
| same-permutation binding | BLOCKED |
| correlated material r | BLOCKED |
| party view | BLOCKED |
| offline Dealer | PASS-C / PAPER UNKNOWN |
| rank privacy | BLOCKED |
| leakage equivalence | BLOCKED |
| stable ties/padding | BLOCKED |
| causal round accounting | BLOCKED |
| paper-native output | BLOCKED |
| equivalent production primitive | BLOCKED |
| primitive conformance | PROJECT PASS / exact NOT_REMEASURED |
| oracle differential | PROJECT PASS / exact NOT_REMEASURED |
| independent-process E2E | PROJECT PASS / paper-native NOT_REMEASURED |
| formal leakage evidence | BLOCKED |

必要 gate 未全部通过，故不得创建 paper-compatible C++，不得创建
`agarwal_protocol_i_exact_mask_output` executable 或把 candidate 三轮改名。

## 允许的下一步

只有取得作者/导师对以下问题的明确答复后，才可将决策升级为 `DESIGN_GO`：三轮 transcript、
public masked-list、r 生成/相关性、party view、native output、adapter round boundary、
stable ties/padding、Dealer model、leakage set 和 full-version proof。

获答复后必须先写独立 design record，冻结 material/package/frame、failure semantics、
causal graph、leakage simulator 和验证矩阵，再实现代码。验证顺序固定为：primitive
conformance → paper-native oracle differential → independent Dealer/P0/P1 E2E → actual
barrier trace → formal leakage simulator。任何新增 adapter round 必须单列。

## 禁止事项

不得复用 `RA6M`/`RA8M` 作为 paper transcript；不得复用 M3 DPF material；不得把 reverse
shuffle 计为免费；不得修改 M3、formal baseline、candidate、Route A、`VFSS-baseline/`、
`Papers/` 或本地参考工程；不得把工程 PASS 写成论文 exact PASS。
