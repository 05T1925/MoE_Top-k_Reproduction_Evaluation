# M2/M3 队友接手说明（论文证据收尾）

日期：2026-09-19

交接分支：`docs/m2-paper-evidence-handoff`

范围：证据与文档收尾；本次没有新增或修改协议实现。

## 一句话状态

M2 的 C 类工程基线和 M3 的模块化工程基线已有可追溯实现/测试记录；M2 的
paper-exact Protocol I 仍被会议版资料不足阻塞。下一步不是补猜实现，而是先取得能审计
三轮 transcript、`π/r` material 和 party view 的一手资料。

## 先读什么

按此顺序阅读，避免把工程适配误写成论文结论：

1. `PROJECT.md` 与 `docs/IMPLEMENTATION_PLAN.md`（总边界和计量规则）。
2. `docs/decisions/M2_M3_FINAL_HANDOFF_2026-09-15.md`（既有标签、输入输出、角色与隔离）。
3. `docs/decisions/M2_PROTOCOL_I_STAGE3O_DECISION_2026-09-15.md`（exact gate 仍为 NO-GO）。
4. `docs/decisions/M2_PROTOCOL_I_PAPER_EVIDENCE_SUPPLEMENT_2026-09-19.md`（本次九项证据矩阵）。
5. `docs/reproduction/M2_M3_STAGE3N_FINAL_CLOSEOUT_2026-09-15.md`（最后一次 Ubuntu/EMP 实测）。
6. `docs/decisions/PROTOCOL_III_MODULAR_3ROUND_DESIGN.md`（M3 独立路径）。

## 现有实现身份：不要改名

| 名称 | 标签 | 输入 → 输出 | 在线轮数 | 证据身份 |
|---|---|---|---:|---|
| M2 formal baseline | `m2_protocol_i_raw_score_input_modular_8round_mask_output` | Q20.12 raw-score shares → 原顺序 XOR Top-K mask | 8 | C 工程基线 |
| M2 candidate | `m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate` | padded priority-key shares → shuffled-domain rank shares | 3 | candidate，不是完整 mask |
| M2 Route A | `m2_protocol_i_dealer_preprocessed_rank_reveal_6round_mask_output` / `m2_protocol_i_raw_score_dealer_preprocessed_rank_reveal_8round_mask_output` | priority/raw shares → 原顺序 XOR mask | 6 / 8 | C 扩展，额外 rank reveal/adapter |
| M3 modular | `agarwal_protocol_iii_modular_3round` | padded priority-key shares → 原顺序 XOR mask | 3 | 工程模块化基线 |
| M3 raw | `moe_topk_protocol_iii_raw_score_modular_5round` | raw-score shares → 原顺序 XOR mask | 5 | M3 项目扩展 |

M2 Route A 不是 M3；M3 不得解析或复用 `RA6M`/`RA8M`、Route A material 或 public carrier。

## 当前取证结论

- 已有 A 类证据：Protocol I 的 2+1/offline Dealer 模型、三轮高层声明、stable-rank 的
  同分顺序，以及 Fsort/Fselect 的原生 key+payload shares 输出。
- 仍阻塞 exact：逐轮消息/party view、`r` 的精确生产和分发、同一 permutation 对
  score/payload/index 的绑定、padding/dummy 语义、full version/proof/transcript。
- SIGMA 是合理的 FSS Gen/Eval 背景对照，并有公开代码；它不是 Protocol I transcript。
- 原始分数的 Q20.12 转换、original-index 绑定、reverse routing 和 original-order mask 都
  必须作为项目 adapter 单独计量，不能自动塞进论文三轮。

完整页码、哈希、检索边界和作者问题见证据补充文档。

## Git 接手与合并安全

交接前快照中，本地 M2 线和 `origin/main` 已分叉：基准
`30df4f09836a4ff38c83e87e04e29048f405022c` 相对
`origin/main@e34ff9016874450b244c75a4e44ecf5a48d895dc` 为本地独有 22、远端独有 13 个
提交。该分支保留完整本地 M2/M3 审计谱系，不能假定其可直接合并。

接手者应先执行：

```powershell
git fetch origin --prune
git status --short --branch
git rev-list --left-right --count HEAD...origin/main
git log --left-right --cherry-pick --oneline HEAD...origin/main
```

然后在一个新的短生命周期整合分支上，审阅远端 `main` 独有提交并进行受控 rebase 或 merge，
解决文档/代码冲突后重新跑相关测试。不要直接 force-push、不要把本交接分支合入 `main`，也
不要在 `main` 上直接编辑。

## 后续工作边界

- 活动实现目录仅为 `VFSS/`；绝不修改 `VFSS-baseline/`、`Papers/`、
  `Agarwal_TopK/`、`ADSMPC/` 或 `CipherGPT/`。
- 没有 A 类 transcript 前，只做资料获取、设计表和测试计划；不创建声称 paper-exact 的
  secure package/frame。
- 一旦获得资料，先完成 message/material/view/leakage 表，再做最小 isolated primitive；
  验证顺序固定为 conformance → oracle differential → 独立进程 E2E。
- 任何新 adapter、公开值、轮数、预处理时机或输出语义都要同步更新 decision record、
  implementation plan、trace/leakage table 和 metrics。未实测项写 `NOT_MEASURED`。

## 本次复检范围

本次交接只新增文档，因此没有重跑 C++/CTest 或性能实验（`NOT_REMEASURED`）。交接提交前应
复检文档链接、`git diff --check`、工作树、分支基线、冻结目录无差异，以及远端分支推送结果。

全局项目阶段、约 55% 的规划口径和其余里程碑见
`docs/PROJECT_PROGRESS_2026-09-19.md`。新分支必须使用功能前缀，禁止再使用 `codex/`。
