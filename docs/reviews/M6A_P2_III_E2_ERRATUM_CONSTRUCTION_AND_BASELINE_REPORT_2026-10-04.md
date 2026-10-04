# M6A-P2-III-E2：padding 勘误、III+AAV86 构造门与全对全基线准备报告

日期：2026-10-04。工作分支 `codex/m6a-p2-iii-e2`；E1 起点 `67739472c4409a62a676823c9e786fe4aa40bb57`，E2 勘误检查点 `ae100e7c7df9d11437aadcd5be40d5b5d5bdcf73`，E2 构造/基线被审查源码检查点 `bdc195804e673c1a25a44ee424867c67db1977ea`。独立 worktree 为 `C:\Users\28641\.codex\worktrees\f516\MoE_Top-k_Reproduction_Evaluation`；未进入主工作区，也未修改 E15/E16/E17 原始目录或 `VFSS-baseline/`。本报告是检查点之后的说明，不代替被测源码身份。

## 1. 判定摘要与证据类别

| 工作线 | E2 结论 | 证据身份与边界 |
| --- | --- | --- |
| E1 padding 勘误 | **E1 关于现有 adapter 的零值 dummy 断言撤回**；正式 Protocol III 入口没有该已指控缺陷。 | `main@c3926c68` 与 E1 的同一源码 blob、真实 adapter 定向实调和冻结 oracle；零值 dummy 反例保留为反事实错误接线。 |
| III+AAV86 功能代数 | **TEST_ONLY CONDITIONAL**。D 个 priority-key 份额经隐藏 handle、AAV86 rank-offset、ring DPF 和同 π 逆路由可写出候选功能合同。 | E1 明文模型 255 个差分子例，E2 增逆路由联合视图的 `Z4` 条件穷举；不等于真实 secure 入口。 |
| III+AAV86 泄露 | **项目扩展许可已给出**。每轮 masked list、前 `r−1` 轮 local rank、pivot/bucket/活跃图/edge 访问/帧长/abort 可公开；末轮 rank 与原槽明文选择不公开。 | 项目所有者本任务明确答复“允许上述公开字段，继续审查稀疏图方案”；不能借 Protocol I 的许可或证明。 |
| III+AAV86 安全/运行 | **SECURE_RUNTIME_GATE = NO-GO**。第一个未闭合 hybrid 是 native 64-bit ring DPF 单份目标隐私在公开 `m`、自有 rank-mask share、早期图和其余 key 等联合辅助信息下的转换。新组合包、帧、一次性领取未实现。 | [构造门](../decisions/M6A_P2_III_E2_HIDDEN_HANDLE_RING_DPF_CONSTRUCTION_GATE_2026-10-04.md)给出假设、模拟器可见量和逐步归约；不把 conformance 或“半诚实”写成证明。 |
| 现有 III 全对全完整入口 | **四组容量/功能/计数 TCP 冒烟 PASS；正式九指标 NOT_MEASURED**。 | n=128/256、K=2/8，独立 T/P0/P1、四条 TCP loopback 在线连接、oracle/恰 K、真实包长/DCF PRG/分阶段帧；没有 E17 LAN/WAN 1+5 原始批次。 |

论文定义、本地参考行为、项目扩展与待验证设想分开使用：AAV86 pivot 图及 Agarwal 会议版 §5 的 shuffle-based CA compiler 属论文定义；E7/E17 与当前仓库运行属本地行为；可信非合谋离线 T、完整 raw-score→原序 bit-mask、获许可的扩展泄露属项目合同；III `2r` 核心、DPF 联合安全、正式性能和作者精确复现属待验证。`AUTHOR_EXACT=NOT_PROVEN`，M6A/V3 未总验收。

## 2. E1 padding 事实核查及历史保留

对 `main@c3926c68` 和 E1 检查点逐 blob 核对：`protocol_i_score_input.cpp` 同为 `04d750044ba80b36df1b8a7a8280129d6e3a6882`，padding 份额 **P0=`0x80000000`、P1=`0`**，重构 signed `INT32_MIN`；`protocol_iii_grank.cpp` 同为 `39017015834db4830ad2ca35fd29fea4c13cc518`，GRank 只处理 `logical_n` 真实槽。严格 priority key 同分时按原下标，dummy 下标大于真实槽。新增 n=3/K=3 的全 `−1` 与全 `INT32_MIN` 定向例，既调用正式 `protocol_iii_raw_score_mask_party`，又直接调用真实 `protocol_i_raw_score_input_party` 检查 D=4 key，再比冻结 oracle/M3 参考；142/142 cases PASS。详情见 [勘误](../decisions/M6A_P2_III_E2_E1_PADDING_ERRATUM_2026-10-04.md)。

E1 设计门、E1 报告及实施计划保留可见的历史反例和修正：故意把 dummy 错接为 score 0 时，`[-1,-1,-1]`、K=3 只选中两个真实槽；此测试不调用现有 adapter。它不能证明已签收的 F1 入口有 padding 缺陷，也不能作为继续判 III+AAV86 NO-GO 的理由。

## 3. III+AAV86 候选构造及审查进展

[独立构造门](../decisions/M6A_P2_III_E2_HIDDEN_HANDLE_RING_DPF_CONSTRUCTION_GATE_2026-10-04.md)从 D 个真实 adapter priority-key 加法份额出发，逐阶段固定：T 输入前预发每轮 `r·C(D,2)` 全池 uCMP/DCF keys 和 D 个共享节点 mask、同一 π 的独立前/逆置换因子、每 handle 独立 64-bit ring DPF key/目标与 rank mask；P0/P1 先前向隐藏布局，逐轮打开 D-word masked list，对公开活跃图取 local rank，前 `r−1` 轮公开 local rank 并确定 pivot/bucket/offset，末轮保持 rank 份额，打开均匀 masked rank，DPF Eval 后逆路由到原槽 XOR mask。现有 score adapter、uCMP/DCF 单边 Eval、F1 ring DPF 算法和 I 的两遍线性 shuffle 代数可复用；新组合 bundle、handle rank-offset adapter、final rank-share 接口、同 π 绑定、真实帧/材料 claim 必须新增。不能把现有原槽 GRank 或 I 的公开最终 carrier 原样接入。

完整逐方泄露表明确写出自己的 raw shares、离线 keys/掩码/置换因子、每轮 masked list、早期 local rank、pivot/bucket/活跃图/edge trace/帧长/abort、最终均匀 masked rank、逆路由 transcript 和自己的输出份额。相比正式 F1 的固定原槽全两两流量，它额外公开 shuffled-domain 部分名次及自适应工作图。隐藏控制图的备选需 oblivious 图更新/secure mux/ORAM 或每轮全池求值，现有接口无实现/证明；全池 Eval 至少失去稀疏在线边优势。项目所有者选择公开控制图后，选项 A 获泄露许可，安全证明仍单独审查。

E7 全池 hybrid 迁移逐步检查共享 R 相关阈值、未使用 key 全池替换、公开早期 `L` 的 `L−q_p` peer-share patch、自适应访问后处理、末轮 `m−q_p−u_p` patch、DPF key 目标替换及逆路由。所需 `FSS-IND/PRG-IND`、独立 keygen 币、私有完整通道和可信非合谋 T 已列出；`DPF-IND-64` 需覆盖真实 native serialization、固定 payload 1、公开 Eval 点向量和其余联合辅助信息。**第一个无现有证据支撑的转换是 DPF key 目标替换**；F1 的 DPF 功能测试只证明正确性。逆路由进一步根据源码 `e_p=-τ_p(a_{1-p})±h'` 推得：fresh 均匀 h' 使固定本方 e 后 peer a 仍均匀，故即使 peer carrier 是未知 DPF bit share，peer 首帧与本方环输出 share 仍均匀；模拟器可给定理想 XOR 输出低位、抽其余环位并反解 peer 帧。E2 的两 handle `Z4` 条件穷举通过，但真实新包/帧与随机币独立性仍待审，不把此夹具写成 secure 证明。

独立消息 DAG 为 score carry→sign 两层、前向隐藏布局一层、每轮 masked-list open 和前 `r−1` 轮 local-rank open、最后 masked rank/DPF 一层、逆路由一层。CA/DPF **`2r` 仅候选核心目标**，加 score 和前/逆路由得候选完整 `2r+4`；新 secure 帧尚未实现，端到端轮数 `NOT_PROVEN`。当前门停在 TEST_ONLY；只有 DPF 联合假设/证明、组合材料与帧审查通过后，才按 D≤8 conformance→冻结 oracle differential→独立 T/P0/P1 E2E 推进。

## 4. 全对全 Protocol III 基线的实际进展与缺口

[九指标合同](../decisions/M6A_P2_III_E2_FULL_CLIQUE_BASELINE_CONTRACT_2026-10-04.md)与 E17 对齐：离线时间到 T 退出且两方 ready，在线时间取两方完整 secure 调用较大值，总时间先逐行相加再取五次统计；ready 时在线留存材料有效载荷、双方 framed sent/received、真实因果轮、实际传统 DCF 长度倍增 PRG 和无序 GRank 边均独立记录。正式输入应与 E17 同 `(K,rep)` 计划配对且与 T 可见 ID 独立，每配置 LAN/WAN 1 预热+5 正式，并保存校准/原始行/源码和二进制哈希。当前 TEST_ONLY harness 没有 ready 时钟或 netem 原始批次；全部正式九指标列保持 `NOT_MEASURED`。

容量工具由 F1 当前 bundle/native DPF 布局推导，并用 M5 G3 已观察的小包和本次大包校验。每方包 n128 为 8,711,978 B、n256 为 35,267,050 B，均低于 64 MiB 单包门；两方有效载荷仅**推导**为 131,805,184/534,159,360 bit。冒烟输入/份额 seed 固定用于复查，不能作为输入隐私证据；正式计划另用独立 OS seed。真实 TCP loopback 三进程对四组均 oracle/恰 K PASS，四因果轮，T 先退且无在线 T：

| 仅 TEST_ONLY TCP loopback | n128/K2 | n128/K8 | n256/K2 | n256/K8 |
| --- | ---: | ---: | ---: | ---: |
| 双方实际包传输 B，含两个 8 B 长度头 | 17,423,972 | 17,423,972 | 70,534,116 | 70,534,116 |
| 双方在线 framed sent B | 12,672 | 12,672 | 24,960 | 24,960 |
| 真实 DCF 长度倍增 PRG | 1,335,296 | 1,335,296 | 5,422,592 | 5,422,592 |
| native DPF Eval 次数 | 512 | 2,048 | 1,024 | 4,096 |
| 无序 GRank 边 | 8,128 | 8,128 | 32,640 | 32,640 |
| E17 LAN/WAN 九指标与 RSS | `NOT_MEASURED` | `NOT_MEASURED` | `NOT_MEASURED` | `NOT_MEASURED` |

分阶段双方在线字节 n128 为 carry/sign/GRank/DPF=`4192/4192/2144/2144`，n256 为 `8288/8288/4192/4192`；实际计数经 A.sent=B.received 和阶段求和守恒。PRG 在每方 secure 调用前 reset、返回后 read；n128 每方 667,648、n256 每方 2,711,296，与 `4D·34+2C(n,2)·(33+log₂D)` 的独立形状断言相符。该计数不含 DPF Eval/AES；在线时间、offline 时间、RSS 和正式材料留存计数未从这些形状或旧项目数字估算。下一执行门是补 ready barrier/时钟/材料正式计数及网络校准与原始索引，再冻结 revision 跑 E17 同口径批次。

## 5. 执行和差异审计

环境：WSL Ubuntu 24.04，CMake 3.28，Debug/EMP OFF，隔离构建目录 `/tmp/m6a-p2-iii-e2-debug`；正式 LAN/WAN 批次预期需另冻 Release/所需依赖。已运行：

1. `cmake --build /tmp/m6a-p2-iii-e2-debug --target moe_topk_m5_fix_f1_process_e2e_test -j4`，PASS；先前 `moe_topk_m5_fix_f1_raw_score_mask_test` 构建 PASS。
2. `ctest --test-dir /tmp/m6a-p2-iii-e2-debug --output-on-failure -R "moe_topk_m5_fix_f1_(raw_score_mask|process_e2e)_test"`，2/2 PASS；定向真实 adapter 共 142 cases PASS。
3. `python -m unittest VFSS.tests.moe_topk.protocol_iii_aav86_test_only_gate -v`，5/5 PASS；包括反事实零值 dummy、原槽逆路由反例、odd-prime 奇偶反例、255 差分子例和新 `Z4` 条件穷举。
4. `python VFSS/tests/moe_topk/protocol_iii_e2_baseline_capacity.py`，n128/256、K2/8 四行布局检查 PASS。
5. `moe_topk_m5_fix_f1_process_e2e_test --e2-tcp-{n128,n128-k8,n256,n256-k8}-smoke`，四组各 PASS；`--failure-smoke` 的 peer early close 两方均失败且无有效 mask，`--cost-smoke` 六形状 PASS。
6. `git diff --cached --check` / `git diff --check`，PASS；检查 E2 新提交只在 `VFSS/tests/moe_topk/`、文档/计划，未改活动 secure 实现、冻结基线、密钥、论文、本地参考工程或 E15/E16/E17 原始目录。未推送、未合并。

## 6. 本次 E2 修改文件清单

下列 12 个唯一文件覆盖 `ae100e7` 勘误检查点、`bdc1958` 构造/基线检查点及本报告；E1 历史文本保留且附勘误，不静默抹除：

| 文件 | 修改 |
| --- | --- |
| `VFSS/tests/moe_topk/protocol_iii_raw_score_mask_test.cpp` | 实调正式 adapter 的 n3/K3 全负和 `INT32_MIN` 定向边界。 |
| `VFSS/tests/moe_topk/protocol_iii_aav86_test_only_gate.py` | 反事实零值 dummy 标签；新增逆路由条件均匀性穷举。 |
| `VFSS/tests/moe_topk/protocol_iii_raw_score_mask_process_test.cpp` | 四条真实 TCP loopback 在线连接、n128/256×K2/8 CLI 冒烟、实际 DCF PRG 进程计数及守恒。 |
| `VFSS/tests/moe_topk/protocol_iii_e2_baseline_capacity.py` | 新增 F1 包/有效载荷的 allocation-free 布局预检。 |
| `docs/decisions/M6A_P2_III_E1_AAV86_RING_MASK_DESIGN_GATE_2026-10-04.md` | 对历史 padding 结论加显式勘误。 |
| `docs/decisions/M6A_P2_III_E2_E1_PADDING_ERRATUM_2026-10-04.md` | 新增源码 blob、真实测试和撤回范围。 |
| `docs/decisions/M6A_P2_III_E2_HIDDEN_HANDLE_RING_DPF_CONSTRUCTION_GATE_2026-10-04.md` | 新增逐阶段构造、泄露选择、E7 hybrid、消息 DAG/门项。 |
| `docs/decisions/M6A_P2_III_E2_FULL_CLIQUE_BASELINE_CONTRACT_2026-10-04.md` | 新增正式入口九指标、输入/网络/计时计划与容量实证边界。 |
| `docs/reviews/M6A_P2_III_E1_DESIGN_GATE_AND_TEST_ONLY_EVIDENCE_2026-10-04.md` | E1 报告加勘误，不删除旧反例。 |
| `docs/reviews/M6A_P2_III_E2_ERRATUM_CONSTRUCTION_AND_BASELINE_REPORT_2026-10-04.md` | 本完整 E2 工作报告。 |
| `docs/IMPLEMENTATION_PLAN.md` | 追加 E1 勘误、E2 获许可泄露/NO-GO 与全对全基线进展。 |
| `docs/BENCHMARK_VALIDATION_PLAN.md` | 追加 III 正式入口基线计量计划和未测边界。 |

E2 结束边界：padding 事实已纠正，TEST_ONLY 构造及真实 F1 容量/计数门已可审；III+AAV86 secure 仍 NO-GO，正式 III 全对全 LAN/WAN 九指标仍 `NOT_MEASURED`。下一阶段需先关闭 DPF 联合安全转换与新包/帧审计，再决定 III+AAV86 D≤8 secure 三层验证；全对全基线的正式 runner 可独立推进。
