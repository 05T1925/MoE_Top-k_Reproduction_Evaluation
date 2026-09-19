# M2/M3 队友接手说明（论文证据收尾）

日期：2026-09-19

交接分支：`docs/m2-paper-evidence-handoff`

范围：证据与文档收尾；本次没有新增或修改协议实现。

## 一句话状态

M2 的 C 类工程基线和 M3 的模块化工程基线已有可追溯的实现与测试记录；M2 的
paper-exact Protocol I 在本项目验收标准下仍受逐轮 transcript、`π/r` material 和
party-view 证据不足的限制。

下一步不是补猜实现，而是先取得可以审计 Protocol I 三轮 causal transcript、shuffle
material、各方 view，以及 paper-native 输出到项目 mask 输出之间适配边界的一手资料。

当前后续路线为：

```text
M2 Protocol I paper-exact
→ Protocol I 通信核验
→ Protocol I 接口交接
→ M5 Protocol III paper-exact
→ Protocol III 通信核验
→ Protocol III 接口交接
→ M6A AAV86
→ M6B BB90+DCF
→ M7 六方案统一报告
```

M4 CipherGPT 已取消，不属于当前路线。

## 先读什么

按以下顺序阅读，避免把工程适配或 supporting literature 写成目标论文结论：

1. `PROJECT.md` 与 `docs/IMPLEMENTATION_PLAN.md`：总边界和计量规则。
2. `docs/decisions/M2_M3_FINAL_HANDOFF_2026-09-15.md`：既有标签、输入输出、角色与隔离。
3. `docs/decisions/M2_PROTOCOL_I_STAGE3O_DECISION_2026-09-15.md`：本项目的 paper-exact gate。
4. `docs/decisions/M2_PROTOCOL_I_PAPER_EVIDENCE_SUPPLEMENT_2026-09-19.md`：本次证据矩阵。
5. `docs/reproduction/M2_M3_STAGE3N_FINAL_CLOSEOUT_2026-09-15.md`：最后一次 Ubuntu/EMP 实测记录。
6. `docs/decisions/PROTOCOL_III_MODULAR_3ROUND_DESIGN.md`：M3 独立路径。
7. `docs/PROJECT_PROGRESS_2026-09-19.md`：当前全局路线和阶段边界。

## 证据分类

本交接继续使用以下 A/B/C/D 体系：

- A — `TARGET_PAPER`：Agarwal CCS 2024 目标论文直接定义或明确陈述。
- B — `LOCAL_REFERENCE`：`Agarwal_TopK/`、`ADSMPC/` 等本地参考工程的实际行为。
- C — `PROJECT_EXTENSION`：本项目增加的 ABI、adapter、original-order mask、metrics 或工程约定。
- D — `UNVERIFIED`：缺少目标论文直接证据、尚未完成审计的设想或映射。

SIGMA 等其他论文单列为 `SUPPORTING_LITERATURE`。它们可用于解释通用 FSS
preprocessing/Gen/Eval 模型，但不属于 B 类本地参考，也不能替代 Agarwal Protocol I 的
逐轮 transcript。

## 现有实现身份：不要改名

| 名称 | 标签 | 输入 → 输出 | 在线轮数 | 证据身份 |
| --- | --- | --- | ---: | --- |
| M2 formal baseline | `m2_protocol_i_raw_score_input_modular_8round_mask_output` | Q20.12 raw-score shares → 原顺序 XOR Top-K mask | 8 | C 类工程基线 |
| M2 candidate | `m2_protocol_i_dealer_preprocessed_3round_rank_share_candidate` | padded priority-key shares → shuffled-domain rank shares | 3 | candidate；不是完整 mask |
| M2 Route A | `m2_protocol_i_dealer_preprocessed_rank_reveal_6round_mask_output` / `m2_protocol_i_raw_score_dealer_preprocessed_rank_reveal_8round_mask_output` | priority/raw shares → 原顺序 XOR mask | 6 / 8 | C 类扩展；包含额外 rank reveal/adapter |
| M3 modular | `agarwal_protocol_iii_modular_3round` | padded priority-key shares → 原顺序 XOR mask | 3 | 工程模块化基线 |
| M3 raw | `moe_topk_protocol_iii_raw_score_modular_5round` | raw-score shares → 原顺序 XOR mask | 5 | C 类项目扩展 |

M2 Route A 不是 M3。M3 不得解析或复用 `RA6M`、`RA8M`、Route A material 或其
public carrier。

现有 3-round M2 candidate 只输出 shuffled-domain rank shares，不是项目要求的
original-order XOR Top-K mask，因此不得因为轮数相同而升级为完整 Protocol I
paper-exact 实现。

## 当前取证结论

### 已有 A 类直接证据

Agarwal CCS 2024 会议版直接支持：

- Protocol I 采用 `(2+1)` 模型；
- `P0`、`P1` 是在线双方，`P2` 是 offline party/dealer；
- Dealer 在离线阶段发送 correlated randomness，在线阶段保持静默；
- Protocol I 由 secure shuffle 与 Compare-Aggregate ranking 组合；
- Protocol I 的高层声明为三个在线轮次；
- secure shuffle 产生 secret-shared shuffled list，并公开 masked shuffled list
  `π(x)+r`；
- `r` 是与 shuffle 关联的私有随机 mask，而不是公开输出本身；
- Fsort/Fselect 的原生功能输出是 key shares 与对应 payload shares，而不是本项目的
  original-order XOR Top-K mask；
- stable rank 中，相等元素按原序列顺序确定先后。

Dealer online silence 因而属于 A 类目标论文直接证据，不需要依赖 SIGMA 推导。

### rank 方向必须显式转换

目标论文 rank 的方向为：

```text
minimum rank = 0
maximum rank = n - 1
```

稳定同分规则为：相等元素中，原序列更早者 rank 更小。

本项目 priority-rank 的统一方向为：

```text
最高优先级 = 0
Top-K largest
score 相同时 original index 更小者优先
```

两者方向相反。后续实现、测试和接口文档必须显式登记 rank-direction mapping，不能只写
“stable rank 一致”。

### 仍阻塞 paper-exact 的内容

当前会议版尚不足以审计：

- 三轮中每一轮的 sender、receiver、消息字段及 causal dependency；
- 每方在各阶段持有的 private material 和可见的 public value；
- `r` 的具体生成、分发、份额形式和消费点；
- concrete secure-shuffle material 如何确保 key 与 corresponding payload 使用同一
  secret permutation；
- 本项目新增 original index 后的绑定和逆路由方式；
- padding/dummy 的完整协议语义；
- paper-native key/payload shares 到 original-order XOR Top-K mask 的完整适配；
- full version、proof 或作者提供的逐轮 transcript。

论文功能定义已经要求 key 与 corresponding payload 保持对应关系；尚未取得的是 concrete
secure-shuffle material 如何实现该绑定。Original index 则属于本项目新增 payload/adapter，
必须作为 C 类单独定义和审计。

### SIGMA 的限定用途

SIGMA 是 `SUPPORTING_LITERATURE`，可作为以下通用背景的旁证：

```text
preprocessing Gen
→ 生成与在线阶段配合使用的 correlated material
→ online Eval 由 P0/P1 执行
```

SIGMA 不能证明 Agarwal Protocol I 的三个 causal round、`π/r` material、party view、
same-permutation 实例化或 inverse routing。

## paper-native 输出与项目输出

Agarwal 的 Fsort/Fselect 原生功能输出是 key shares 与 corresponding payload shares。
它不是：

```text
原始顺序 XOR Top-K mask
```

本项目要求的 original-order XOR Top-K mask 属于 C 类 adapter。以下内容必须与 paper core
分开描述和计量：

- Q20.12 raw-score 输入转换；
- paper rank 到 project priority-rank 的方向映射；
- original-index payload；
- reverse routing；
- original-order mask；
- XOR share 输出；
- 项目 metrics 和错误语义。

Theorem 4.1 的 3-round 声明不能未经审计直接覆盖这些 adapter。某个 adapter 是否增加通信
轮次，应根据实际 causal communication 判断；既不能默认免费计入，也不能预先断言一定增加。

## 本项目 paper-exact 验收门

`IMPLEMENTATION_NO_GO` 是本项目针对 paper-exact 验收定义的工程门，不是“Protocol I
在一般意义上不可实现”的论文结论。

会议版已经提供高层功能、secure shuffle 接口、`π(x)+r`、FSS gates、rank、三轮声明和成本
公式。其他实现者可以依据这些信息构造符合高层 functionality 的实现。

但本项目要求验证的是：

```text
paper-exact
message-level
material-level
party-view-level
round-exact
```

因此，在缺失 transcript/material 证据得到补足之前，本项目不得把新实现验收或命名为
paper-exact。

## Git 接手与合并安全

交接前快照中，本地 M2 线与 `origin/main` 已分叉。记录的基准为：

```text
local base:
30df4f09836a4ff38c83e87e04e29048f405022c

origin/main:
e34ff9016874450b244c75a4e44ecf5a48d895dc

divergence:
local-only 22
remote-only 13
```

这些数字是交接时快照，不是永久状态。接手者必须在操作前重新执行：

```powershell
git fetch origin --prune
git status --short --branch
git rev-list --left-right --count HEAD...origin/main
git log --left-right --cherry-pick --oneline HEAD...origin/main
```

然后在新的短生命周期整合分支上审阅 `origin/main` 独有提交，并进行受控 rebase 或 merge。
解决文档和代码冲突后重新运行相关测试。

不要直接 force-push，不要假定本交接分支可以无审阅地合入 `main`，也不要在 `main` 上直接
编辑。

## 后续工作边界

- 活动实现目录仅为 `VFSS/`。
- 不修改 `VFSS-baseline/`、`Papers/`、`Agarwal_TopK/` 或 `ADSMPC/`。
- CipherGPT 已退出当前路线，不再把 `CipherGPT/` 作为后续实现阶段。
- 没有足够的一手 transcript/material 证据前，只做资料获取、设计表和测试计划。
- 不创建声称 paper-exact 的 secure package、frame 或 material identity。
- 不得用 `RA6M`、`RA8M`、M3 DPF material、测试层明文重构或本地参考行为填补目标论文
  transcript 空白。
- 获得资料后，先完成 message/material/view/leakage 表，再实现最小 isolated primitive。
- 验证顺序固定为 conformance → oracle differential → 独立进程 E2E。
- 新增 adapter、公开值、轮数、预处理时机或输出语义时，同步更新 decision record、
  implementation plan、trace/leakage table 和 metrics。
- 未实际测量的字段写 `NOT_MEASURED`。

## Protocol I 完成后的交接要求

Protocol I 进入接口交接前，至少应冻结：

1. target-paper 证据表；
2. 三轮 causal transcript；
3. `π`、`r` 与 `π(x)+r` 生命周期；
4. material producer、recipient 和 consumption point；
5. 各方 view 与公开值；
6. key/payload same-permutation 绑定；
7. original-index C 类 adapter；
8. paper rank 到 priority-rank 的方向映射；
9. paper-native 输出到 project mask 的 adapter；
10. paper core 与 adapter 的轮数、通信和泄露拆分；
11. conformance、oracle differential 与独立进程 E2E；
12. 可复现 revision、命令、环境和原始日志。

完成该交接后，路线进入 M5 Protocol III paper-exact，而不是恢复 CipherGPT。

## 本次复检范围

本次交接只更新文档，没有重新运行 C++、CTest 或性能实验：

```text
runtime verification: NOT_REMEASURED
performance fields: NOT_MEASURED
```

提交前应复检：

- 文档内部链接；
- `git diff --check`；
- 工作树状态；
- 当前分支与 `origin/main` 的真实分叉；
- 冻结目录无差异；
- Agarwal 路径与 SHA256；
- 远端分支推送结果；
- 全局路线中不存在已取消的 M4 CipherGPT。
