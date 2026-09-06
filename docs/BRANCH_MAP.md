# M2 分支阶段映射

更新时间：2026-09-06。本文记录 M2 历史分支的阶段归属和新名称，便于从
`main` 追溯阶段证据。分支重命名只改变 Git ref，不改变提交内容或论文结论。

| 原名称 | 新名称 | 阶段归属 |
| --- | --- | --- |
| `m2-protocol-i-design` | `m2.0-design-protocol-i-design` | M2.0 设计门 |
| `m2-protocol-i-feasibility` | `m2.1-feasibility-protocol-i-feasibility` | M2.1 可实现性审计 |
| `m2-protocol-i-semantics-audit` | `m2.2-rank-semantics-protocol-i-semantics-audit` | M2.2 rank 语义 |
| `m2-protocol-i-reverse-spec` | `m2.3-reverse-spec-protocol-i-reverse-spec` | M2.3 reverse shuffle 规格 |
| `m2-protocol-i-primitive-conformance` | `m2.4-priority-dcf-protocol-i-primitive-conformance` | M2.4 priority/DCF |
| `m2-protocol-i-ucmp-conformance` | `m2.5-ucmp-protocol-i-ucmp-conformance` | M2.5 uCMP |
| `m2-protocol-i-cmpagg-conformance` | `m2.6-cmpagg-protocol-i-cmpagg-conformance` | M2.6 CmpAgg |
| `m2-protocol-i-cmpagg-process-e2e` | `m2.7-transport-protocol-i-cmpagg-process-e2e` | M2.7 bounded transport |
| `m2-protocol-i-cmpagg-three-process-e2e` | `m2.7-three-process-protocol-i-cmpagg-three-process-e2e` | M2.7 三进程 E2E |
| `m2-protocol-i-permute-share-conformance` | `m2.10-permute-share-protocol-i-permute-share-conformance` | M2.10 Permute+Share |
| `m2-protocol-i-secret-shared-shuffle-conformance` | `m2.11-secret-shuffle-protocol-i-secret-shared-shuffle-conformance` | M2.11 两遍 shuffle |
| `m2-protocol-i-small-e2e` | `m2.12-small-e2e-protocol-i-small-e2e` | M2.12 priority-key E2E |
| `m2-protocol-i-modular-6round` | `m2.13-six-round-protocol-i-modular-6round` | M2.13 六轮候选 |
| `m2-protocol-i-raw-score-input` | `m2.15-closeout-test-protocol-i-raw-score-input` | M2.14 raw score + M2.15 收尾 |

M2.15 收尾分支对应的实际 closeout 提交为 `25f4cff8f62818aff3968e4bb8f33f0e636d9443`，
并已通过 merge commit `9c1e9a657f6a573c0159ed8e230f34ab2517b20e` 合入 `main`。
当前实现仍是 C 级模块化 8-round 路径；3-round paper core 和 7-round candidate
不因分支重命名而变为已实现。

M2.16 从 `main` 的 `6c72ec8a18a44c1d3d441758017b9807fe1dc090` 建立为
`m2.16-paper-exact-3round-protocol-i`。本阶段只完成 paper-exact feasibility 和
leakage audit；由于同置换 public `pi(x)+r`、相关 `r`/GRank material 与 3-round
message transcript 仍缺乏可审计实现，未修改 VFSS 生产代码，当前 8-round C 级标签保持。

当前 Git 集成状态：M2.16 审计已通过 `f800f96` 进入 `main`。其后两个独立修复分支
`fix/m2-chosen-ot-pollhup` 和 `fix/m2-modular-e2e-fd-lifecycle` 分别修复 chosen-OT
的 readable-HUP 行为和 modular E2E 的每-case FD 生命周期；它们不改变协议标签或
paper-exact 结论。Ubuntu 24.04.4 的 soft `RLIMIT_NOFILE=1024` 组合验证为 EMP-ON
19/19、EMP-OFF 13/13。M3 实现应从当前 `main` 建立，而不是从 exact 目标或旧 M2
分支派生。

M1.1、M3 设计和 Protocol III DPF 测试分支不属于本次 M2 重命名范围。
