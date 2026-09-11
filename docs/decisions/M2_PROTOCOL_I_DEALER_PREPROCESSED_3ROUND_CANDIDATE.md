# M2 Protocol I Dealer-preprocessed 3-round rank-share candidate

状态：**IMPLEMENTATION_BLOCKED（代码与测试已隔离实现；EMP-ON 运行证据缺失）**。

日期：2026-09-11。实现标签固定为
`m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate`。
该标签表示本项目的 Dealer-preprocessed、最多单角色腐化且不串谋的 secure
candidate，不表示 Agarwal 会议论文的 exact Dealer view、paper-exact、exact
leakage equivalence 或 Protocol I reproduction complete。

## 1. 安全模型和证据边界

本候选明确采用以下项目扩展安全模型：

- P0、P1 是 online 计算方；P2 是 input-independent offline Dealer；
- 最多腐化一个角色，且不允许角色串谋；
- P2 可以在 offline 阶段采样完整随机 `r`，短暂持有完整 `r`，并用完整
  `r_i/r_j` 生成 edge DCF/uCMP material；
- P2 只能看到公开 shape、material id 和随机生成状态；
- P2 不接收 score、priority-key、任何 input share、P0/P1 permutation 或
  composite permutation；
- P2 不参与 online，不看到 `y`、shuffled payload、rank share、重构 rank 或输出，
  并在 online role 启动前退出；
- P0/P1 各自只获得自己的 `r` share、party-local edge material、自己的输入 share
  和自己的局部 permutation。

“P2 在 offline 知道完整 `r`”是本项目候选模型的显式假设，不得反写成 Agarwal
会议论文规定的 exact Dealer view。论文 Gate C 仍独立保持 BLOCKED。

## 2. 冻结的真实协议表

| 阶段 | 角色 | 输入 | 消息 | 打开值 | 输出 | Online barrier |
| --- | --- | --- | --- | --- | --- | --- |
| O0 | P2 → P0/P1 | public shape、随机 `r`、DCF material | framed party-local package | 无 | `r0/r1` share、party edge keys | 0 |
| O1 | P0 ↔ P1 | local permutation、PS offline material | existing PS preprocessing traffic | 无 | one-shot forward PS material | 0 |
| R1 | P0 ↔ P1 | input shares、first PS material | existing first PS transcript | 无 | first-pass shares | 1 |
| R2 | P0 ↔ P1 | first-pass shares、second PS material | existing second PS transcript | 无 | shuffled priority-key shares | 1 |
| R3 | P0 ↔ P1 | shuffled share、local `r` share | one framed masked-share exchange | `y = pi(x) + r` | public masked shuffled list `y` | 1 |
| Local | P0/P1 | public `y`、party-local DCF keys | 无 | 无新增打开值 | shuffled-domain rank shares | 0 |

固定结论：online causal barriers 为 `R1 + R2 + R3 = 3`。DCF/CmpAgg evaluation
不发送 rank-share bundle，不打开 rank，不执行 reverse shuffle，不生成
original-order XOR mask。O0/O1 的 offline cost 必须单独计量，不计入 online rounds。

## 3. 独立 package/material contract

candidate 不扩展或重解释 `ProtocolIPartyPackage`。每个 party-local package 绑定：

- format version 和固定 implementation label；
- session、fingerprint、material id、party；
- logical_n、padded_n、k、comparison_bits、rank_bits；
- `r_share[padded_n]`；
- 每条 `(left,right)` comparison edge 的 stage、slot、endpoint 和 party DCF material；
- package 的 one-shot consumption state。

candidate package 和每条 edge material 都是 move-only；核心入口消费 package，重复
消费或已消费 DCF material 必须 fail closed。package framing 使用现有
`ProtocolIFramedChannel`，因此绑定 sender/receiver、phase、type、sequence、length
和 chunk offset。

## 4. P2 offline preprocessing

P2 只接收 public config，先通过 `protocol_i_make_input_layout` 验证
logical/padded/k/width。随后：

1. 在 comparison ring 中生成每个 shuffled slot 的随机 `r_i`；
2. 生成 `r0_i`，令 `r1_i = r_i - r0_i mod 2^comparison_bits`；
3. 对每条 edge `(i,j)` 用完整 `r_i/r_j` 构造 `ProtocolIUcmpMaterial`；
4. 导出 party 0/1 的 edge material；
5. 分别序列化并通过独立 offline framed channel 分发 package；
6. 清理完整 dealer mask、关闭 package channel，并正常退出。

P2 不生成 input-dependent permutation，也没有 input FD。正式路径使用 OpenSSL
随机源；确定性 permutation seed 只由 TEST_ONLY controller 显式注入，用于可复现
测试。完整 `r` 不进入 P0/P1 package，P0/P1 只看到本地 share。

## 5. Online core

`protocol_i_dealer_candidate_core_party` 的 secure 入口只接受一个 party 的：

- candidate package；
- forward-only 使用的现有 `ProtocolIShufflePartyMaterial`；
- padded priority-key additive shares；
- 两个现有 forward online FD 和一个 R3 masked-open FD。

执行顺序固定为：验证 package/layout/material；执行现有两遍 forward
Permute+Share；逐槽计算

```text
local_masked_share[i] = shuffled_share[i] + r_share[i]
public_y[i] = local_masked_share[i] + peer_masked_share[i]
```

全部加法按 comparison ring 取模。随后 P0/P1 各自在本地对 public `y` 和自己的
edge material 调用 `protocol_i_cmpagg_eval_party`，返回 shuffled-domain additive
rank shares。核心中没有 rank reveal FD、peer rank-share消息、rank reconstruction、
reverse carrier、Top-K oracle 或 original-order mask。

## 6. 域、padding 和 dummy

candidate 遵循当前项目真实 layout：

- `1 <= logical_n <= 1,000,000`，`padded_n` 从 2 开始取不小于 logical_n 的 2 次幂；
- `index_bits` 采用 `protocol_i_make_input_layout` 的 padded 定义；
- `comparison_bits >= minimum_comparison_bits` 且 `34 <= comparison_bits <= 53`；
- public `y`、`r` share、masked share 使用 comparison ring；
- `rank_bits` 记录有效 padded rank 宽度，但 rank share 仍以 comparison-ring word 返回；
- 所有 padded slots 进入 secure comparison graph；TEST_ONLY oracle 才在逻辑结果判断
  时排除 dummy；
- 本阶段只处理 padded priority-key shares，不实现 raw-score adapter。

## 7. 计量和 trace

候选的 run record（不把 provenance 塞入 secure party payload）必须保留：

- P2 package bytes 和 O1 offline material bytes；
- R1、R2、R3 每个实际 framed channel 的 sent/received bytes；
- 每条实际 barrier 的 phase、sequence、sender、receiver 和完成状态；
- comparison edge 数、DCF evaluation 次数和由 `forward counters + R3 exchange`
  推导出的 online rounds；
- revision、implementation label、环境、输入/seed、重复次数和 correctness。

没有完整 PRG 计数器或未执行环境的项目写 `NOT_MEASURED`，禁止用常量或历史数据
填充。

## 8. 与论文 exact 模型的保留差异

本候选解决的是一个明确的工程安全模型：P2 被授权短暂知道完整 offline `r`，并
生成相关 DCF material。该模型与此前“任何单方都不得知道完整 `r`”的审计前提不同，
也不等于论文 full version 未提供的正式 Dealer transcript。

即使 candidate 的三轮功能、独立进程、泄露审计和回归全部通过，仍只能使用
`m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate`、
`paper-compatible candidate` 或 `Agarwal-inspired project extension` 等名称。
不得修改 Gate C 为 PASS，不得实现或声称 original-order mask、7-round unified
path 或论文精确复现。

## 9. 验收门

Gate B 只有在以下证据全部通过后才能标记 `SECURE_CANDIDATE_GO`：

- package serialization、mismatch/replay/one-shot conformance；
- 两遍真实 forward shuffle 和一遍 R3 masked-list opening；
- public `y` 方程与同一 `r` 的 differential evidence；
- 本地 DCF/CmpAgg rank-share oracle differential；
- 实际三 barrier transport trace；
- P2/P0/P1 fork+exec、P2 online 前退出、FD/EOF/POLLHUP/timeout 生命周期；
- secure source 无明文重构、无 rank reveal、无文件同步、无在线 Dealer；
- 当前 M2/M3 回归、完整 CTest、`BUILD_TESTING=OFF` 隔离；
- EMP-OFF/EMP-ON 分开记录，未执行项为 `NOT_MEASURED`。

在上述证据完成前，本文件状态只能从“设计冻结”更新为 `IMPLEMENTATION_BLOCKED`
或保持 pending，不能提前写成 `SECURE_CANDIDATE_GO`。

## 10. 2026-09-11 实施验收状态

候选 package、core、独立角色 executable 和 TEST_ONLY fork+exec controller 已加入
隔离路径，生产代码的语法边界检查通过。当前主机的 EMP-ON 配置在
`find_package(emp-tool 1.0 CONFIG REQUIRED)` 处失败，因而真实 OT/DCF、两遍
Permute+Share、候选 executable、候选 CTest 和 independent-process E2E 均不能在本
机标记为 PASS；这些项目保持 `NOT_MEASURED`。EMP-OFF 的现有 25 项 M2/M3/full
CTest 回归通过，但不覆盖本候选。故本阶段结论为 `IMPLEMENTATION_BLOCKED`，不是
`SECURE_CANDIDATE_GO`。完整命令和输出边界见
`docs/reproduction/M2_PROTOCOL_I_DEALER_PREPROCESSED_3ROUND_CANDIDATE_2026-09-11.md`。
