# MoE Top-K 安全协议统一项目

本仓库以 **VFSS** 为唯一活动实现框架，建立统一语义、可复现、可横向比较的安全 Top-K 实验环境。

M2 Protocol I 已完成当前工程验收；当前活动里程碑为 M5 Protocol III two-round path，之后依次完成 AAV86、BB90+DCF 两条升级路线及完整性能测试。

M4 CipherGPT 实施和性能任务已取消。其历史资料继续保留为参考，不作为后续阶段的前置条件。

## 当前状态

| 里程碑 | 状态 | 实现与验收边界 |
| --- | --- | --- |
| M0 | 已完成 | 远端仓库及冻结基线已建立 |
| M1/M1.1 | 已完成 | score、tie rule、oracle、基础适配、metrics 和 CTest 入口已冻结 |
| M2 工程基线 | 已完成并合入 main | Protocol I 四轮核心；raw-score 到原顺序 mask 共八轮 |
| M2 | 已完成 | three-round C-INSTANTIATION implemented；independent three-round review PASS；online logical communication matches Theorem 4.1；Protocol I PR merged；`AUTHOR_EXACT = NOT_PROVEN` |
| M3 | 已完成并冻结 | priority-key 三轮入口，以及 raw-score 五轮扩展 |
| M4 | 已取消 | 不实施 CipherGPT，不安排其性能实验 |
| M5 | 正在进行 | shared clique CmpAgg / GRank → DPF routing → two-round composition；复用既有 ranking core |
| M6A | 后续目标 | I+AAV86、III+AAV86 实现及完整性能验收 |
| M6B | 后续目标 | I+BB90+DCF、III+BB90+DCF 实现及完整性能验收 |
| M7 | 后续目标 | 六种方案统一汇总与报告 |

已完成的工程实现为：

| 实现标签 | 输入 | 核心轮数 | 完整入口轮数 |
| --- | --- | --- | --- |
| `m2_protocol_i_raw_score_input_modular_8round_mask_output` | raw Q20.12 score shares | 4 | 8 |
| `agarwal_protocol_iii_modular_3round` | padded priority-key shares | 3 | 3，不含 raw-score 适配 |
| `moe_topk_protocol_iii_raw_score_modular_5round` | raw Q20.12 score shares | 3 | 5 |

需要明确：

- M2 的完成边界为当前工程验收，不是作者未公开实现的逐转录复现；`AUTHOR_EXACT = NOT_PROVEN`。
- M5 复用 uCMP、DCF、CmpAgg Gen/Eval、priority-key semantics 与 rank-share contract，不建立第二份 ranking core。
- M3 已在 `main@bb0d0e8` 完成整改，不再是“可开始”的待实现阶段。
- M3 三轮和五轮入口保留为 M5 的正确性及开销对照，不通过改名升级为论文两轮实现。
- `VFSS/` 已包含 M1、M2、M3 的活动实现；`VFSS-baseline/` 继续保持冻结。
- 当前任务安排不等于分支工作已经验收或合入 main，完成状态必须对应代码和证据。

## 后续路线

```text
M2 Protocol I：COMPLETED
  → M5 Protocol III two-round path：IN PROGRESS
  → 通信测量及差异解释
  → 基础接口交接
  → M6A AAV86 两种升级实现及完整性能验收
  → M6B BB90+DCF 两种升级实现及完整性能验收
  → M7 六种方案统一报告
```

Protocol I 的论文核心为三轮，Protocol III 为两轮。raw-score 输入适配、表示转换和原顺序 mask 输出适配单独记录，并计入端到端主结果。

两次基础协议验收均执行：

```text
协议实现与正确性完成
  → 通信核验及差异解释完成
  → 接口、示例和证据交接完成
```

通信核验需要对齐论文公式、实际参数和消息结构、分方实测计数。数量级相符不能单独证明正确性或安全性；数量级不符也不能未经差异分析直接归因于论文错误。

AAV86 和 BB90+DCF 各自包含两种实现及完整性能验收，不把性能测试全部推迟到 M7。资料研究和设计可提前并行，依赖实现和正式验收遵循上述顺序。

## 六种目标方案

| 方案 | 目标 |
| --- | --- |
| Protocol I | 全对全 CmpAgg、shuffle-based routing、统一 mask 适配 |
| Protocol III | 全对全 CmpAgg、压缩 DPF routing、统一 mask 适配 |
| Protocol I + AAV86 | Protocol I 路线的图算法升级 |
| Protocol III + AAV86 | Protocol III 路线的图算法组合升级 |
| Protocol I + BB90+DCF | 第 K 大阈值选择与 DCF 成员判断，接入 Protocol I 路线 |
| Protocol III + BB90+DCF | 第 K 大阈值选择与 DCF 成员判断，接入 Protocol III 路线 |

上述是目标方案，不表示已经全部实现。

图升级需要单独解决自适应预处理、Dealer 在线静默、材料绑定、泄露和消息依赖。Protocol III 的图算法组合不能直接继承 shuffle-based compiler 的论文结论。

BB90+DCF 必须保持稳定同分规则并恰好选出 K 个位置；历史 Direct Top-K 原型不直接改名为 BB90。

## 统一语义与实验要求

统一输入为 32 位二补码 signed fixed-point score 算术共享，小数位数为 12。选择 score 最大的 K 个元素，同分按 original index 升序，最高优先级 rank 为 0。

统一输出为：

```text
原始输入顺序下长度 n 的秘密共享 Top-K bit-mask
每位为 0/1，且恰好 K 位为 1
```

六种方案复用同一 oracle、输入语义和输出契约。重构与明文校验只在隔离的测试路径发生。

统一测试矩阵：

| n | K | AAV86 迭代 r |
| --- | --- | --- |
| 128、256 | 2、8 | 2、3、4、5 |
| `10^3`、`10^4`、`10^5`、`10^6` | 80 | 2、3、4、5 |

BB90 迭代参数按采用算法单独定义。

正式实验保留全部统一指标：离线时间与材料、在线时间、total/per-party/分方通信、因果轮数、PRG 调用、比较边数和总时间，以及 revision、输入、种子、环境和命令。

每配置预热一次、正式运行五次，报告 median/min/max。AAV86、BB90+DCF 的完整性能阶段分别覆盖 LAN/WAN。无法完成的配置保留失败原因和未测字段，不外推填表。

详细定义以 `PROJECT.md` 和 `docs/IMPLEMENTATION_PLAN.md` 为准。

## 文档入口

### 当前规范与分工

- [项目范围、六种方案、论文映射与统一指标](PROJECT.md)
- [详细实施计划与阶段门](docs/IMPLEMENTATION_PLAN.md)
- [双人职责、公共文件所有权与交接契约](docs/TEAM_WORK_PLAN.md)
- [M3 冻结契约及后续双人执行计划](docs/M3_ONWARD_TEAM_WORK_PLAN.md)
- [项目实现约束](AGENTS.md)
- [协议复现工作流](.agents/skills/protocol-reproduction/SKILL.md)
- [M1 统一 score 语义（已冻结）](docs/decisions/M1_SCORE_SEMANTICS.md)

### 已完成基线与验收证据

- [M1.1 Ubuntu 24.04 验收记录](docs/M1_1_UBUNTU_HANDOFF.md)
- [M2 分支阶段映射与重命名记录](docs/BRANCH_MAP.md)
- [M2 Protocol I 历史实施前设计门](docs/decisions/M2_PROTOCOL_I_DESIGN_GATE.md)
- [M2.16 精确三轮候选规格与历史阻塞](docs/decisions/M2_PROTOCOL_I_PAPER_EXACT_3ROUND_DESIGN.md)
- [M2.16 paper-exact 泄露审计](docs/decisions/M2_PROTOCOL_I_EXACT_LEAKAGE_AUDIT.md)
- [M2.16 Ubuntu 研究记录](docs/reproduction/M2_PROTOCOL_I_PAPER_EXACT_3ROUND_UBUNTU_2026-09-06.md)
- [M2 chosen-OT POLLIN/HUP 修复记录](docs/reproduction/M2_CHOSEN_OT_POLLHUP_UBUNTU_2026-09-06.md)
- [M2 modular E2E FD 生命周期修复记录](docs/reproduction/M2_MODULAR_E2E_FD_LIFECYCLE_UBUNTU_2026-09-06.md)
- [Protocol III 模块化三轮设计](docs/decisions/PROTOCOL_III_MODULAR_3ROUND_DESIGN.md)
- [M3 复检整改关闭记录](docs/reproduction/M3_REVIEW_CLOSEOUT_UBUNTU_2026-09-10.md)
- [M3 Ubuntu 环境基线](docs/reproduction/M3_ENV_BASELINE_UBUNTU_2026-09-07.md)
- [M2→Protocol III CmpAgg reuse handoff](docs/handoffs/M2_PROTOCOL_I_TO_III_CMPAGG_REUSE_HANDOFF_2026-09-24.md)
- [Protocol III CmpAgg reuse new-chat prompt](docs/handoffs/PROTOCOL_III_CMPAGG_REUSE_NEW_CHAT_PROMPT_2026-09-24.md)

### 历史路线与本地资料

- [2026-09-04 历史路线决策](docs/decisions/ROADMAP_PRIORITY_2026-09-04.md)
- [本地论文与参考仓库配置](docs/LOCAL_REFERENCES_SETUP.md)
- [本地参考资料边界](docs/REFERENCE_MANIFEST.md)
- [M0/M1 仓库复检](docs/M0_REVIEW.md)

旧路线中的 CipherGPT 实施和 M4→M5 前置关系已被本次修订替代。当前执行顺序以更新后的项目总纲、实施计划和分工文档为准。

## 目录说明

```text
VFSS/                 唯一活动实现目录
VFSS-baseline/        冻结恢复与回归基线
docs/                 计划、决策、来源与验收记录
.agents/skills/       仓库级协议复现工作流
```

本地可按任务需要准备 `Papers/`、`ADSMPC/`、`Agarwal_TopK/` 和 `CipherGPT/`。这些目录被 `.gitignore` 排除，新环境不会自动拥有它们。

`CipherGPT/` 仅保留为历史参考，本轮不要求准备其工作包以推进实施。论文和大型参考工程不进入普通远端 Git 历史。

资料准备与版本校验见 `docs/LOCAL_REFERENCES_SETUP.md`；新增来源和再分发边界见 `docs/REFERENCE_MANIFEST.md`。

## 当前协作起点

- **角色 A（搭档）**：维护 Protocol I 已完成资产，并评审 M5 的 CmpAgg/rank-share 复用。
- **角色 B（Protocol III 负责人）**：推进 M5 Protocol III two-round path。
- **双方**：交叉复跑、维护公共契约，随后分别推进两条协议路线的 AAV86 和 BB90+DCF 升级。

开始工作前：

1. 阅读 `PROJECT.md`、`AGENTS.md` 和当前阶段决策。
2. 按分工准备必要资料并校验版本。
3. 从最新 main 建短生命周期分支。
4. 按 `docs/TEAM_WORK_PLAN.md` 确认共享接口、metrics 和文档的主写负责人。
5. 新协议实现进入 `VFSS/`，不修改 `VFSS-baseline/`。
6. 按实施计划的实现、通信核验和交接门验收。
7. PR 使用仓库模板，结果保留 revision、环境、输入、种子、命令和原始计数。
8. 不提交生成密钥、日志、论文、参考工程和本地构建产物。

`vfss-baseline-2026-09-03` 只用于恢复与回归比较，不作为新开发起点。

## 已记录的验证与性能边界

- M1 四项测试已在 Apple Clang、CMake、Homebrew `libomp`/`eigen@3` 环境通过。
- M1/M1.1 已在 Ubuntu 24.04.4 的干净 Debug 构建中通过 4/4 CTest。
- M2 验证可靠性修复记录：Ubuntu 24.04.4、soft `RLIMIT_NOFILE=1024` 下，EMP-ON 19/19、EMP-OFF 13/13 通过。
- M3 关闭记录保留三轮/五轮入口及 11-test 验证口径，具体环境、结果来源和限制以原记录为准。
- 已有工程路径具备部分实际通信、材料和时间记录；这些记录不等于完成论文核心通信核验。
- 正式 LAN/WAN 性能和完整可信在线 PRG 计数仍需补齐，未测项保持 `NOT_MEASURED`。

历史验证不因修改 README 而成为重新运行的结果，也不能直接推导新精确核心或图升级方案的性能。

复现命令分别见 M1.1、M2 修复和 M3 关闭记录。
