# Protocol III CmpAgg Reuse — New Chat Prompt

Copy the text below into a new Sol High implementation chat.

---

继续 MoE_Top-k_Reproduction_Evaluation 的 Protocol III 主实现工作。

仓库：

`/home/fss/projects/MoE_Top-k_Reproduction_Evaluation`

使用 Sol High。先读取并遵守仓库 `AGENTS.md`。这是 Protocol III 编程新对话，
但不要把 Protocol III 当成一套从零开始的 ranking 实现。

## 0. 起点和证据边界

Protocol I 和 Protocol III 的论文架构是：

```text
                 shared clique CmpAgg / GRank
                              |
              +---------------+---------------+
              |                               |
      Protocol I shuffle routing      Protocol III DPF routing
```

论文 Table 1 中二者都使用 `(n choose 2)-CmpAgg`。Protocol I 用 shuffle
routing、2+1 parties、3 online rounds；Protocol III 用 DPF routing、2+1
parties、2 online rounds。

Protocol III is NOT a fresh implementation of ranking.

Before writing Protocol III ranking code, inspect and reuse the existing
validated CmpAgg/uCMP/DCF stack. 不要把 `protocol_i_cmpagg_eval_party` 复制成
`protocol_iii_cmpagg_eval_party`，除非实际 ABI 证据证明无法共享，并先报告。

保持四类证据分离：论文定义、本地参考行为、项目 C-INSTANTIATION、待验证设想。
不得把现有 M3 ring adapter 或新候选自动称为 author-exact。

先确认 Git 真实状态。Protocol I PR #23 已于 2026-09-24 合并，远端 merge
commit 为 `ac7f0ed2def54f5c90bb8b9f297792c323dd2f72`。先 fetch 并确认 updated
`main` 包含该提交，再从 updated `main` 建短生命周期 Protocol III 分支；如果真实
状态不同，先报告，不要猜。不要从当前任务中陈旧的 local main 开始。不要 commit、
push，除非用户后续明确要求。

## 1. 必读材料

先实际读取：

- `PROJECT.md`
- `docs/IMPLEMENTATION_PLAN.md`
- `docs/TEAM_WORK_PLAN.md`
- `docs/M3_ONWARD_TEAM_WORK_PLAN.md`
- `docs/handoffs/M2_PROTOCOL_I_TO_III_CMPAGG_REUSE_HANDOFF_2026-09-24.md`
- `docs/decisions/PROTOCOL_III_MODULAR_3ROUND_DESIGN.md`
- `docs/reproduction/M3_REVIEW_CLOSEOUT_UBUNTU_2026-09-10.md`
- `Papers/Agarwal 等 - 2024 - Secure Sorting and Selection .pdf`

论文重点重读：§1.1、§3.1、Figure 3、Corollary 3.1.1、§4.1、§4.2、
Theorem 4.1、Theorem 4.2、Table 1。

## 2. 第一轮只做 inventory + tests + reuse plan

在大规模编码之前必须完成：

A. repo inventory；

B. 读取全部 `protocol_iii_*` headers/sources/tests/CMake targets；

C. 找出 exact shared CmpAgg symbols 及所有 callers；

D. 运行已有 uCMP/CmpAgg tests；

E. 运行全部现有 Protocol III/M3 tests；

F. 输出 reuse plan，逐项分类：

`REUSE_DIRECTLY`、`REUSE_WITH_THIN_ADAPTER`、
`SHOULD_EXTRACT_SHARED_LATER`、`PROTOCOL_I_ONLY`、
`PROTOCOL_III_ONLY`、`SEMANTIC_CONFLICT`、`UNKNOWN`。

在 A–F 完成前不要大规模编码，不要同时重写 CmpAgg、DPF routing、transport
和 record schema。

## 3. 当前已知真实实现边界（必须重新核验）

当前共享 ranking stack 包括：

- `ProtocolIUcmpMaterial` / `ProtocolIUcmpPartyMaterial`：DCF-backed uCMP；
- `protocol_i_mask_priority_key_share`；
- `protocol_i_cmpagg_eval_party`：clique CmpAgg Eval；
- `ProtocolIPartyPackage` 的 `node_mask_shares` 和 `edge_materials`；
- `protocol_i_priority_key`：score DESC、original index ASC；
- `stable_ranks_cmpagg`：仅测试 oracle；
- `protocol_iii_grank_party`：Protocol III 的 masked-input/domain thin adapter。

当前 `protocol_iii_grank_party` 已直接调用
`protocol_i_cmpagg_eval_party`。它不是第二份 rank algorithm。

当前 M3 基线：

```text
R1 GRank
 -> R2 masked-rank DPF indicator routing
 -> R3 secure combine
 -> original-order XOR Top-K mask shares
```

priority-key core 为 3 rounds；raw-score pipeline 为 5 rounds。它们是冻结回归
资产，不是 Theorem 4.2 的两轮精确实现，不能通过改名或删测试升级身份。

## 4. 必须冻结的 shared contracts

### Shared ranking input

- logical comparison-key vector；
- key domain `Z_(2^comparison_bits)`；
- each party 持有 additive shares；
- lower encoded key = higher project priority；
- stable rule = score DESC, original index ASC；
- Protocol I/III 各自负责产生 CmpAgg 所需 public masked inputs。

### CmpAgg preprocessing

- one mask share per logical node；
- one one-shot uCMP/DCF material per clique edge；
- edge order lexicographic `(left,right)`；
- party/session/fingerprint/shape/width bindings fail closed。

### CmpAgg Eval

输入：party id、comparison width、public masked key vector、party edge
material。输出：party-local additive rank-share vector。CmpAgg 不负责 routing，
也不打开 rank。

### CmpAgg rank output -> DPF routing

当前 concrete contract 是：

`ProtocolIIIGrankOutput::rank_additive_shares`

- length = `logical_n`；
- same order as original logical inputs；
- storage = canonical `uint64_t`；
- current domain = `Z_(2^rank_bits)`；
- `rank_bits = max(1, ceil(log2 logical_n))`；
- reconstruct 后 rank 在 `[0,n)`；
- rank 0 = highest project priority；
- equal score 按 original index ascending；
- 不含 padded slots；
- 不得打开 raw ranks；
- record association 通过相同 index 保持。

论文写 `Z_n`，当前 non-power-of-two 情况使用 `Z_(2^rank_bits)`；这是必须显式
处理的 C-INSTANTIATION/domain gap，不能悄悄等价。

## 5. 明确禁止跨协议复用的 runtime state

Protocol III 不得依赖：

- `Pi`、`sigma`、`tau`；
- Chase OPV/ST/Beneš/Permute+Share/SecretSharedShuffle；
- Protocol I parallel shuffle material 或 `s0/s1`；
- Protocol I public `Pi(x)+r_cmp`；
- Protocol I public shuffled ranks；
- Protocol I R3 rank opening/local routing。

共享边界到 additive rank-share production 为止。

Protocol I：CmpAgg -> rank shares -> open shuffled ranks -> local routing。

Protocol III：CmpAgg -> rank shares -> DO NOT OPEN RAW RANKS -> DPF routing
-> secret output shares。

不要把 DPF routing 塞回 Protocol I，也不要让 Protocol III 吃 Protocol I
runtime output。

## 6. Protocol III 特有目标

重点开发：

```text
CmpAgg rank-share output
  -> DPF routing
  -> two-round composition
```

需要逐项解决：

- rank masks `r_rank[i]`；
- DPF alpha/beta and payload semantics；
- masked-rank reconstruction；
- general Fselect payload routing；
- field payload representation；
- nonzero encoding and multiplicative masks；
- inversion failure semantics；
- DPF payload compatibility；
- cross-stage round compression；
- Fsort/FullEval extension（与 Fselect 分开）；
- Theorem 4.2 communication accounting；
- independent-process 2-round causal state machine。

Theorem 4.2 requires payload `H` to be a field. 当前
`ProtocolIBlock192 = Z_(2^comparison_bits) x Z_(2^64) x Z_(2^64)` 是 product
ring/storage，不是 field，不能直接当 Protocol III payload。不要修改 Protocol I
record；定义并验证最小 `ProtocolIIIFieldRecord` 或等价 adapter。

## 7. 随机量命名必须分离

统一使用：

- `r_cmp[i]`：CmpAgg comparison-input mask；
- `r_rank[i]`：Protocol III DPF rank mask；
- `s_payload[i]`：Protocol III nonzero multiplicative payload mask；
- `a_payload`, `b_indicator`, `c_mul`：modular Beaver components。

不要都写成 `r`。不要默认共用 masks。Theorem 4.2 脚注中的 common-mask
optimization 必须在 base two-round correctness 闭合之后单独实现/证明/计量。
当前状态为 `NOT_IMPLEMENTED`。

## 8. 实现顺序

FIRST：复跑 shared uCMP/DCF/CmpAgg，确认 Protocol III 继续调用同一排名核心。

SECOND：冻结并 differential-test
`ProtocolIIIGrankOutput::rank_additive_shares -> protocol_iii_dpf_routing_party`。

THIRD：保持当前 M3 三轮实现不变，完成 modular DPF routing 的 correctness、
domain、one-shot、serialization、party-view 和 process E2E 审计。

FOURTH：选择 field representation，先写 field/encoding/DPF payload conformance。

FIFTH：实现独立的两轮 compressed candidate，检查每轮 outbound 在读取本轮
peer inbound 前已冻结；与 M3 同输入/种子/oracle differential。

SIXTH：完成 Fselect core 后再做 Fsort/FullEval 和 optional common-mask
optimization。

不要一开始同时重写 shared ranking、DPF、transport、schema。若现有 ABI 只需 thin
adapter，就不要移动/rename shared primitives。

## 9. Protocol I regression boundary

Protocol I three-round C-INSTANTIATION 已通过 independent review。任何共享提取
必须是行为不变的 alias/adapter，并运行 Protocol I 与 Protocol III 回归。

如果改动涉及 Protocol I R1/R2/R3、parallel shuffle material、public-y semantics、
causal ordering、rank opening 或 local routing：

`PROTOCOL_I_REAUDIT_REQUIRED = YES`

遇到这种情况先 STOP 报告；优先用 adapter 避免触碰。

禁止修改 `VFSS-baseline/`，禁止在线 Dealer，禁止 clear oracle 进入 secure path，
禁止文件轮询 key transport，禁止把 `Z_(2^b)` 冒充 field。

## 10. 第一轮输出

第一轮先只输出：

`REPOSITORY_BASE`

`PAPER_ARCHITECTURE_CONFIRMATION`

`EXISTING_PROTOCOL_III_MAP`

`CURRENT_SHARED_CMPAGG_SYMBOLS`

`CURRENT_CALL_GRAPH`

`CMPAGG_TESTS_RUN`

`PROTOCOL_III_TESTS_RUN`

`RANK_SHARE_CONTRACT`

`PAYLOAD_FIELD_GAP`

`REUSE_DIRECTLY`

`REUSE_WITH_THIN_ADAPTER`

`PROTOCOL_I_ONLY`

`PROTOCOL_III_ONLY`

`SEMANTIC_CONFLICTS`

`FIRST_IMPLEMENTATION_TARGET`

`PROTOCOL_I_REAUDIT_REQUIRED`

`STOP_REASON`

完成 inventory/tests/reuse plan 后，在同一新对话继续最小实现；普通测试失败直接
定位和修复。只有必须破坏 Protocol I baseline、修改 Protocol III 之外的冻结模块、
field/DPF 代数不闭合、或真实两轮 causal schedule 不成立时才 STOP。

不要 commit，不要 push，直到用户明确要求。每个实现阶段结束运行相关测试和
`git diff --check`，确认 `VFSS-baseline/` 未修改。
