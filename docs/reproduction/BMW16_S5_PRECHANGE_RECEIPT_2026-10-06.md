# BMW16 S5：修改前接收记录

日期：2026-10-06

本记录先冻结 S4 接收对象、来源 revision、主要源码哈希与原始证据身份。记录完成前未修改 S4 工作树或 S4 源码；后续分析、接受/拒绝结论另行记录。

## 1. S5 基线与工作区保护

| 项 | 检查结果 |
|---|---|
| 最新 `origin/main`（执行 `git fetch --no-tags origin main` 后） | `c3926c68fd14f270faa8b55234311071947fa080` |
| S5 隔离 worktree / branch | `C:\Users\28641\.codex\worktrees\m6b-i-bmw16-s5\MoE_Top-k_Reproduction_Evaluation` / `codex/m6b-i-bmw16-s5` |
| S5 基线 HEAD | `c3926c68fd14f270faa8b55234311071947fa080`，与已核验 `origin/main` 相同 |
| S4 指定接收 revision | `04ed6f8a352ab2277434344094471dff82f83e4b`，parent `59c75ff1ef21784bd54f266ab9bd6e631444ccae` |
| S4 分支与 main 关系 | S4 commit 当时在 `codex/bmw16-s4`；不是 `main` 祖先，不能视为已合并 |
| S4 工作树 | 对应 worktree HEAD 与指定 revision 相同，`git status --short` 为空 |
| 主桌面工作区 | `feat/m6a-performance-evaluation`，HEAD 与 main 相同；已有差异保留且未修改 |

桌面主工作区原有差异包括 `PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md`、
`docs/PAPERS.sha256`、`docs/REFERENCE_MANIFEST.md`、两份 M6A 决策文档、
`docs/reproduction/M6A_BASELINE_AND_SOURCE_INVENTORY_2026-09-27.md` 与本地
`siamjdiscrmath.pdf`。S5 不在该 checkout 上写入文件。

S5 worktree 从核验后的 `origin/main` 建立；没有 cherry-pick S4 或 M6A 实验提交。
S4 代码和实验结果只作为独立评审对象；任何后续移入都须先过各自门禁。

## 2. S4 代码身份与 SHA-256

实现标签：`BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R_TEST_ONLY`。
目录：`experiments/TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R/`。

| 文件 | SHA-256 |
|---|---|
| `README.md` | `17e308c7bdf9a394ae10c015a09d3a6f96ec4997223772b127dff0a11b25fbae` |
| `select4r.py` | `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab` |
| `run_matrix.py` | `35122d8c75e89d97f25e7ecf3036164e16f9f0a683d58d80d10ca186f9b4f3ab` |
| `validate_results.py` | `16dcd96673d736b92b76b81fe444e13fbb5cee1a125f60c1b283dac732490ea1` |
| `audit_edges.py` | `390f89c199c66e8899549a4710bd6c853e5709c0fde0f795b1ad1dc789ae346f` |
| `test_select4r.py` | `576de4edadebd908959c56fc4fc9dce10c946d09dbdfa110effdf6aff6114586` |
| `crosscheck_oracles.py` | `6a9c9e48baa5664a29a473fdf43e492d960d223f128ff59aba114406b5bd4c4b` |
| `oracle_harness.cpp` | `30fda9ece72fee463a40300f41165fc4056879847d661482e7924266a171adc9` |

## 3. 原始证据身份

来源于 S4 外部证据目录
`C:\Users\28641\.codex\evidence\BMW16_S4_2026-10-05`；此处只登记哈希，不将原始矩阵、日志或二进制复制进仓库。

| 证据 | SHA-256 | S4 所报范围 |
|---|---|---|
| 官方冻结 seed schedule | `3af287fcafcf82712004c941c3c176340b15b34bc9889ef08aa5b45435c42325` | 10,000 行 |
| 官方 algorithm results JSONL | `288ba4bb5882def10f463c174e15893aed074741a3c08b448320c64c4d8c70c1` | 10,000 次运行 |
| 官方 frozen manifest | `3db6f1ea81234923bdf4413aa6174a30ff4adbab320f5197a379bb911d5189e4` | 实验配置及源哈希 |
| 小型/对抗用例 JSONL | `cea6debee86575c9495724f43808c295d323d722bd748d539a1ad53275442319` | 40,567 条 |
| 小型/对抗失败 JSONL | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` | 空文件 |
| 完整小实例 trace | `c58bf7ee54c278e13ec165e5508db526da7388ed0d1f5ede9d0fff76cd3f07af` | n=8,K=4 |
| S4 独立 edge auditor 输出 | `df7f36592ac898445deb30cb91346ff1cf05fe7c81a81ad676fa596f568a9f4d` | S4 自带 audit 输出 |
| Python/C++ oracle cross-check JSON | `0e703334f46a509d9182f1ab1b3ab0e4ed58cc5a7d9da4f65c0d7c547f64e3ac` | 13 个用例 |

主论文 `Papers/1603.04941v1.pdf`：44 页，SHA-256
`F46F83CBA279F37E9E3AAA0D64B145FDFCFBC52C0C9C8B79BACEC1259CA9CA23`；交叉核对
`Papers/017.pdf`：249 页，SHA-256
`3EAE3A93F12AB71EEA8AC040167F21351FCF3FF54DC423985A04087E3E90CB15`。PDF 保持只读，
不进入 S5 差异。

## 4. 相关 Protocol I / AAV86 / E20 / E21 来源状态

| 来源 | 精确 revision / 标签 | 与当前 main 关系及使用边界 |
|---|---|---|
| Protocol I 当前 main 路线 | `c3926c68fd14f270faa8b55234311071947fa080`；`m2_protocol_i_raw_score_input_modular_8round_mask_output` | mainline C-INSTANTIATION；不是 paper-exact。M2 设计记录明确指出现有 SecretSharedShuffle 不输出同置换 public `π(x)+r` 与相应绑定，不能假设满足 BMW16 自适应图构造。 |
| M6A E17 receiver evidence | `01f3c04fdbe5bcc8de6722bd72cd63bf3bad0b31` | 不是 `origin/main` 祖先；报告为 `PASS_WITH_EXPLICIT_LIMITS`，不得当作 mainline 合并资产。 |
| M6A E20 streamed material | `8986a40175f5f8eef56d765fef0cfa863e205985`；`E20_STREAM_AEAD_V1` | 不是 `origin/main` 祖先。其合同称多 key 联合模拟未证明；单独逐边 AEAD/存储一致性不能替代 Protocol I 泄露证明。 |
| M6A E21 minimal clique/material matrix | `93d96f6c9f9d5c758e5471ea38158f05f1c7dea4` | 不是 `origin/main` 祖先。属于独立 Protocol I+AAV86 实验分支，需分别接收，不直接并入 S5 secure route。 |

本记录只确认 Git 关系和文档/代码标签，不预先认可算法、安全性、E20/E21 runtime 或其性能结果。S4 数学、比较层因果性、失败合同、协议联合视图、材料和容量仍须独立复核。
