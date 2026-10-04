# M6A-P2-I-E18：独立接收后的交付与合入候选审查报告

日期：2026-10-04。结论：**READY_FOR_DRAFT_REVIEW_WITH_EXPLICIT_LIMITS**；交付至 [draft PR #28](https://github.com/05T1925/MoE_Top-k_Reproduction_Evaluation/pull/28)，目标 `main`，未合并。交付范围仅为 E17 已独立接收的 Protocol I+AAV86 **全两两预发有界项目实现**、EMP-ON 全对全基线和对应性能证据；不把它称为 adaptive exact-edge、作者精确复现、Protocol III+AAV86 或 M6A/V3 总完成。六方案分栏与九指标定义见 [E18 数据关闭说明](../reproduction/M6A_P2_I_E18_BOUNDED_DATA_CLOSEOUT_2026-10-04.md)。

## 1. Git 起点、身份和工作区保护

`git fetch origin main` 后，本地 `main`、`origin/main` 和候选 merge-base 均为 `c3926c68fd14f270faa8b55234311071947fa080`。隔离 checkout 从 `codex/m6a-p2-i-e17-independent-receiver@01f3c04fdbe5bcc8de6722bd72cd63bf3bad0b31` 建立 `codex/m6a-p2-i-e18-delivery`；E17 起始工作树干净。主工作区 `C:/Users/28641/Desktop/MoE_Top-k_Reproduction_Evaluation/` 原有 tracked 修改：`PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md`、`docs/PAPERS.sha256`、`docs/REFERENCE_MANIFEST.md`；原有未跟踪：两份 P1/P2 决策、一份 P0 盘点、`siamjdiscrmath.pdf`。该工作区仅只读检查，未 reset、覆盖、移动或清理。E15/E16 工作树也仅只读使用。

`git worktree list --porcelain` 所列八个 checkout 均做只读状态核对：主工作区有上述保留修改；`61dd` 与 E11/E14 工作树为干净的 `b521c5e`；E15 `6a9ef84`、E16 `267cb692` 干净；本 E18 checkout 在最终提交后干净；`f516` 的 Protocol III 工作树有其自身 tracked/未跟踪修改；早期全两两实验工作树 `ac8af47` 有未跟踪历史审计、夹具和原始目录。其他 checkout 的这些内容均未改动、移动或清理。

四个提交身份不可互换：

| 提交 | 角色 |
|---|---|
| `346a92326e81aa3ab573c162439968503e792354` | E15 n=128 修正计时合同后的**被测源码**及独立批次 |
| `7515aac64e8c7779895017d7c735285cb44dc336` | E16 n=256 **被测源码**；E17 n=128 桥接复用其原二进制 |
| `267cb6924f061d376dbce7986fc0093b52ec2512` | E16 **事后报告**，不替换被测源码身份 |
| `01f3c04fdbe5bcc8de6722bd72cd63bf3bad0b31` | E17 异会话独立接收，`PASS_WITH_EXPLICIT_LIMITS`；不产生新协议被测 revision |

`main..候选` 的协议差异包含 DCF 实现/头文件、Protocol I+AAV86 头文件/源码、CMake 接线；测试差异包含 AAV86 conformance/differential/独立进程 E2E、材料/指标/TCP 等价、EMP-ON 基线入口；决策差异包含 E1–E16 的材料、安全、计时和资源门记录；实验差异包含 E11–E17 的 TEST_ONLY runner、审计、导出与 namespace 脚本。下方附录逐文件列出，不把 E18 的文档收束误写成新的 secure 或性能 revision。

## 2. 来源文档与逐段合并

主工作区三份历史文档经重算、审阅后才复制到隔离候选：

| 文档 | 主工作区原 SHA-256 | 本候选处理 |
|---|---|---|
| P0 `M6A_BASELINE_AND_SOURCE_INVENTORY_2026-09-27.md` | `B637F31009F329391D1CC92CC959E251E64A0AAC45C5FBB96CFEE0D471B93EFE` | 解除 E1/E6 和 PROJECT 链接缺口；为 `git diff --check` 仅去掉日期行尾两个空格和末尾多余空行，交付文件 SHA-256 `BF62C0301564B3D8396764B0660F427B27AD290AC0C9A72CA443F00F4C7DF8AF`。历史审计记录中的 B637…仍准确指向原始主工作区字节。 |
| P1 `M6A_AAV86_ALGORITHM_AND_STABLE_ORDER_DECISION_2026-09-27.md` | `21DE1BE89CFCC7B66005B8E4F34BCAFFBC7556CCF22FAE7F517FEC91681456C0` | 原样纳入；早期算法/稳定键决策作为历史设计证据 |
| P2 `M6A_PROTOCOL_I_AAV86_PREPROCESSING_AND_SECURITY_DESIGN_2026-09-28.md` | `C19DF3632C411BF7BD8DEFC6C003CCDC7FC90963D6C46C871FE12D91AD551BDC` | 原样纳入；当时 adaptive exact-edge NO-GO 不被后来全两两预发的条件性运行结论改写 |

`PROJECT.md` 合入本地参考树来源限制，`docs/PAPERS.sha256` 修正 Agarwal PDF 实际名称并登记作者托管 AAV86 PDF SHA，`docs/REFERENCE_MANIFEST.md` 合入书目/副本来源及参考树限制。实测本地忽略 PDF 的两份摘要分别为 Agarwal `18FAF63EAA7923EEF715A6EB9D5D526FE04DCB69700B133C3E94DE935F68C01C`、AAV86 `322F1BD761A987FD09E6B59B3A3AE77D6E4B1CA9DCB1C2765E45AC4A2A7E2B83`；PDF 本身不进 Git。`docs/IMPLEMENTATION_PLAN.md` 只在 §3.4、§4.7 插入有日期的历史设计顺序与 exact-edge/全两两区别，并在原有 E17 段之后追加 E18 交付，完整保留 E15–E17 后续记录；没有用主工作区文件覆盖。基准验证计划只追加 E18 数据引用。E17 接收报告首页的过期 `PENDING` 改为与 §1 一致的最终 `PASS_WITH_EXPLICIT_LIMITS`，起始待核清单和历史审计过程保留。三份来源文件的链接及历史哈希引用已核对；P0 的交付字节差异如上明示。

独立材料池目标的 CTest 需要 `experiments/m6a_p2_i_allpairs/aav86_n5_r2_test_edges.csv`；原候选因 CSV 忽略规则缺失该夹具。E18 从只读实验工作树按既有决策文档 SHA `87A660C55B47FBCD1AAAE6C4E025798E8E4B6853968DC6D27DF635A704B46D78` 核验后补入，仅用于 TEST_ONLY 材料池验证。对七份新旧关键文档的 22 个本地 Markdown 链接逐一解析，缺失为 0。

## 3. 独立构建与验证

在候选工作树干净的 `ac64c8eca9378e49c4949089f24dadb4e172a24a` 文档检查点，使用 WSL Ubuntu 24.04.4、GCC 13.3.0、CMake 3.28.3、自带已固定的 `emp-tool`/`emp-ot` 前缀 `/tmp/moe_m28_emp.ok9WzQ/prefix`，从源码新建 `/tmp/m6a18-delivery-release`。命令（`<repo>` 为本隔离 worktree，所有输出日志在 `/tmp`）：

```bash
cmake -S <repo>/VFSS -B /tmp/m6a18-delivery-release -DCMAKE_BUILD_TYPE=Release -DMOE_TOPK_ENABLE_EMP_OT=ON -DCMAKE_PREFIX_PATH=/tmp/moe_m28_emp.ok9WzQ/prefix
cmake --build /tmp/m6a18-delivery-release --parallel 4 --target moe_topk_m6a7_aav86_small_conformance_test moe_topk_m6a7_aav86_small_differential_test moe_topk_m6a7_aav86_small_e2e_test moe_topk_m6a10_aav86_metrics_conformance_test moe_topk_m6a11_tcp_material_equivalence_test moe_topk_m6a12_protocol_i_clique_benchmark_test
ctest --test-dir /tmp/m6a18-delivery-release --output-on-failure -R '^moe_topk_m6a(7|10|11|12)_' # 6/6 PASS
cmake --build /tmp/m6a18-delivery-release --parallel 4 --target moe_topk_m2_protocol_i_modular_e2e_test moe_topk_m2_chosen_ot_conformance_test moe_topk_m5_fix_f1_raw_score_mask_test moe_topk_m5_fix_f1_process_e2e_test moe_topk_m5g_two_round_fsort_test moe_topk_m5f_two_round_process_e2e_test moe_topk_m3_protocol_iii_three_process_e2e_test
ctest --test-dir /tmp/m6a18-delivery-release --output-on-failure -R '^(moe_topk_m2_(chosen_ot_conformance|protocol_i_modular_e2e)_test|moe_topk_m3_protocol_iii_three_process_e2e_test|moe_topk_m5_fix_f1_(raw_score_mask|process_e2e)_test|moe_topk_m5g_two_round_fsort(_process_e2e)?_test)$' # 7/7 PASS
cmake -S <repo>/experiments/m6a_p2_i_allpairs -B /tmp/m6a18-material-release -DCMAKE_BUILD_TYPE=Release -DMOE_TOPK_ENABLE_EMP_OT=ON -DCMAKE_PREFIX_PATH=/tmp/moe_m28_emp.ok9WzQ/prefix
cmake --build /tmp/m6a18-material-release --parallel 4 --target m6a_p2_i_material_pool_test
ctest --test-dir /tmp/m6a18-material-release --output-on-failure -R '^m6a_p2_i_material_pool_test$' # 1/1 PASS
```

随后按顺序，以 `MOE_TOPK_M6A_E9_D=256 MOE_TOPK_M6A_E16_R=<2|5> timeout 120s prlimit --as=3221225472 -- ./moe_topk_m6a7_aav86_small_<conformance|differential|e2e>_test` 分别复跑两个资源档。r=2、r=5 各为 conformance 6/6、冻结 oracle differential 6/6、独立 T/P0/P1 进程 E2E 6/6 PASS。默认 conformance 包含重复值、非二次幂、同规格材料篡改/重复消费等用例；另运行 `./moe_topk_m6a7_aav86_small_e2e_test transport-conformance` 与 `transport-negative`，TCP/Unix 帧等价和关闭/静默/截断故障均 PASS。三个步骤分别留有 `/tmp/m6a18-n256-r{2,5}-{conformance,differential,e2e}.log`，不加入仓库。

环境准备中一次误用默认 WSL Ubuntu 20.04，其 CMake 3.16.3 因 emp-tool 要求 ≥3.25 而在依赖配置阶段停止；改用仓库已有 Ubuntu 24.04 环境后构建成功。一次旧回归构建命令误把 CTest 名称 `moe_topk_m5g_two_round_fsort_process_e2e_test` 当作构建目标，Make 报 no rule；改为真实可执行目标 `moe_topk_m5f_two_round_process_e2e_test` 后七项回归全过。两者都是环境/命令选择问题，不是协议测试失败，也未写入仓库构建物。

## 4. 冻结证据只读复核

在 Windows Python 下以候选 TEST_ONLY 工具执行：

```text
python experiments/m6a_p2_i_allpairs/TEST_ONLY/e17_verify_preservation.py <E15原目录>/corrected <E15仓库外副本> <E16原目录> <E16仓库外副本>
python experiments/m6a_p2_i_allpairs/TEST_ONLY/e17_verify_bridge.py <E17仓库外目录>/bridge_final_v2 <E16原目录>/final_v2/input_plan.json
python experiments/m6a_p2_i_allpairs/TEST_ONLY/e17_verify_index.py <E17仓库外目录> e17_raw_complete_index_v2.csv
```

E15 原目录/副本 151/151，索引 `E997A85D974EA498CE0999045451C5A29D3521440CB46FEFEAE9047AFAFB70D4`；E16 原目录/副本 376/376，索引 `326BB0A2F490A79EE709A09726B59808F4BEBC754BCAB7E8E75FFD7645F3C0EE`。E17 桥接 120/120 运行、100/100 正式正确、24 配对输入组、20 个五次组及 540/540 统计项 PASS；E17 v2 索引 146/146，SHA-256 `3763F3AC2F8A3074E0511A9F5A960B53A8C472F9BDDAD69D123832CAC15D56C3`。原目录与仓库外副本仍可访问；没有改写冻结样本。E18 只修改文档，不触及 secure、TEST_ONLY 输入计划、材料布局或 E15 计时合同，因此 E15/E16/E17 原被测数据不因 E18 失效，也无须重跑全部网络矩阵。

## 5. 验收边界与待审风险

- 已接收的是 n=128/256 的全两两预发项目路线；n≥1000 的真实首门为 `D>256`，`PRECHECK_REJECTED`，九指标、keygen、耗时和峰值 `NOT_MEASURED`。容量公式及 Dealer 预算是推导/准入，不是峰值实测。
- E17 桥接的 n=128 与 E16 n=256 才可同 `7515aac` revision 分栏；E15 n=128 独立保留。网络为同主机模拟 LAN/WAN；五次正式样本不支持异机或统计显著性推断。
- 安全仍依 E8/E6 所列可信非合谋离线 T、私有完整交付、独立随机性、单份 DCF 隐私及 AES/PRG/公开泄露假设；多 key 联合模拟没有证明。早期 exact-edge NO-GO 不因全两两预发数据解除。
- Protocol III+AAV86 仍独立设计门 NO-GO；BB90+DCF 两路在本阶段没有实现或性能。`AUTHOR_EXACT=NOT_PROVEN`，M6A/V3 总验收未完成。
- 审查需关注 E15/E16 旧批次排除、E16 r5 2 GiB 失败历史、同主机网络限制、固定材料布局推导与 E17 索引/仓库外证据可获取性；不得将零值或预算写成未测指标。

提交前使用 `git diff --check` 和 `git diff --name-status origin/main..HEAD` 审查全部差异，确认 `VFSS-baseline/`、`Papers/`、密钥、构建物、原始日志和 `Agarwal_TopK/`、`ADSMPC/`、`CipherGPT/` 均不在差异。候选已进入 draft PR 审查，未合并 main。

## 附录：main..候选逐文件盘点

完整变更集合以 `git diff --name-status c3926c68fd14f270faa8b55234311071947fa080..HEAD` 为准。以下在最终提交前由该命令生成，按代码/测试、决策和审查文档、实验工具、交付整理分组；新增文件均为文本，不含原始日志或密钥。

### 活动实现与构建接线

- M `VFSS/CMakeLists.txt`
- M `VFSS/ext/FSS/dcf.cpp`
- M `VFSS/ext/FSS/include/FSS/dcf.h`
- A `VFSS/include/moe_topk/protocol_i_aav86_small.h`
- A `VFSS/src/moe_topk/protocol_i_aav86_small.cpp`

### 测试代码

- A `VFSS/tests/moe_topk/protocol_i_aav86_e10_metrics_conformance_test.cpp`
- A `VFSS/tests/moe_topk/protocol_i_aav86_e11_tcp_material_equivalence_test.cpp`
- A `VFSS/tests/moe_topk/protocol_i_aav86_e9_fixtures.h`
- A `VFSS/tests/moe_topk/protocol_i_aav86_small_conformance_test.cpp`
- A `VFSS/tests/moe_topk/protocol_i_aav86_small_differential_test.cpp`
- A `VFSS/tests/moe_topk/protocol_i_aav86_small_e2e_test.cpp`
- A `VFSS/tests/moe_topk/protocol_i_e12_baseline_bench_test.cpp`
- A `VFSS/tests/moe_topk/protocol_i_e14_material_metrics.h`

### 决策文档

- A `docs/decisions/M6A_AAV86_ALGORITHM_AND_STABLE_ORDER_DECISION_2026-09-27.md`
- A `docs/decisions/M6A_P2_I_E12_MEASUREMENT_CONTRACT_2026-10-03.md`
- A `docs/decisions/M6A_P2_I_E14_UNIFIED_METRIC_PROVENANCE_2026-10-03.md`
- A `docs/decisions/M6A_P2_I_E15_PROTOCOL_III_AAV86_DESIGN_GATE_2026-10-03.md`
- A `docs/decisions/M6A_P2_I_E15_PROTOCOL_I_TIMING_CONTRACT_2026-10-03.md`
- A `docs/decisions/M6A_P2_I_E16_STAGED_RESOURCE_GATE_2026-10-03.md`
- A `docs/decisions/M6A_PROTOCOL_I_AAV86_ALL_PAIRS_DESIGN_GATE_2026-10-02.md`
- A `docs/decisions/M6A_PROTOCOL_I_AAV86_E6_COMPOSITION_SECURITY_AND_RUNTIME_CONTRACT_2026-10-02.md`
- A `docs/decisions/M6A_PROTOCOL_I_AAV86_PREPROCESSING_AND_SECURITY_DESIGN_2026-09-28.md`

### 审查文档

- A `docs/reviews/M6A_P2_I_E10_CROSS_CHAT_RECEIVER_AUDIT_2026-10-03.md`
- A `docs/reviews/M6A_P2_I_E10_FREEZE_METRICS_AND_D128_GATE_2026-10-03.md`
- A `docs/reviews/M6A_P2_I_E11_CROSS_CHAT_RECEIVER_AUDIT_2026-10-03.md`
- A `docs/reviews/M6A_P2_I_E11_TCP_LAN_WAN_N128_SUBMATRIX_2026-10-03.md`
- A `docs/reviews/M6A_P2_I_E12_CORRECTED_N128_AND_CLIQUE_BASELINE_2026-10-03.md`
- A `docs/reviews/M6A_P2_I_E12_CROSS_CHAT_DATA_ACCEPTANCE_2026-10-03.md`
- A `docs/reviews/M6A_P2_I_E13_E12_RECEIVER_TECHNICAL_AUDIT_AND_PROTOCOL_III_HANDOFF_2026-10-03.md`
- A `docs/reviews/M6A_P2_I_E14_PROTOCOL_I_UNIFIED_PERFORMANCE_CLOSEOUT_2026-10-03.md`
- A `docs/reviews/M6A_P2_I_E15_CROSS_CHAT_RECEIVER_AUDIT_2026-10-03.md`
- A `docs/reviews/M6A_P2_I_E15_E14_PRE_FIX_INDEPENDENT_ACCEPTANCE_2026-10-03.md`
- A `docs/reviews/M6A_P2_I_E15_PROTOCOL_I_UNIFIED_REMEASUREMENT_2026-10-03.md`
- A `docs/reviews/M6A_P2_I_E16_D256_PAIRED_PERFORMANCE_2026-10-04.md`
- A `docs/reviews/M6A_P2_I_E17_INDEPENDENT_RECEIVER_2026-10-04.md`
- A `docs/reviews/M6A_P2_I_E18_DELIVERY_AND_MERGE_CANDIDATE_2026-10-04.md`
- A `docs/reviews/M6A_P2_I_E1_INDEPENDENT_AUDIT_2026-10-02.md`
- A `docs/reviews/M6A_P2_I_E4_CA_SOURCE_AND_SHUFFLE_COMPATIBILITY_2026-10-02.md`
- A `docs/reviews/M6A_P2_I_E5_AAV86_CA_SPEC_AND_REFERENCE_2026-10-02.md`
- A `docs/reviews/M6A_P2_I_E7_REVIEW_AND_SMALL_D_RUNTIME_GATE_2026-10-03.md`
- A `docs/reviews/M6A_P2_I_E7_SMALL_D_RUNTIME_VALIDATION_2026-10-03.md`
- A `docs/reviews/M6A_P2_I_E8_CROSS_CHAT_RECEIVER_ACCEPTANCE_2026-10-03.md`
- A `docs/reviews/M6A_P2_I_E8_RECEIVER_REVIEW_2026-10-03.md`
- A `docs/reviews/M6A_P2_I_E8_TECHNICAL_FIX_AND_GATE_RESULT_2026-10-03.md`
- A `docs/reviews/M6A_P2_I_E9_CAPACITY_AND_BOUNDED_SCALE_2026-10-03.md`
- A `docs/reviews/M6A_P2_I_E9_CROSS_CHAT_RECEIVER_AUDIT_2026-10-03.md`

### 复现与性能计划

- M `docs/BENCHMARK_VALIDATION_PLAN.md`
- M `docs/IMPLEMENTATION_PLAN.md`
- A `docs/reproduction/M6A_BASELINE_AND_SOURCE_INVENTORY_2026-09-27.md`
- A `docs/reproduction/M6A_P2_I_ALL_PAIRS_MATERIAL_EXPERIMENT_2026-09-30.md`
- A `docs/reproduction/M6A_P2_I_E18_BOUNDED_DATA_CLOSEOUT_2026-10-04.md`

### 实验工具

- A `experiments/m6a_p2_i_allpairs/.gitignore`
- A `experiments/m6a_p2_i_allpairs/CMakeLists.txt`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/README_TEST_ONLY.md`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/aav86_ca_control_E6_TEST_ONLY.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/aav86_ca_reference_TEST_ONLY.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e11_audit_results.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e11_capacity_status.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e11_network_calibrate.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e11_run_matrix.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e11_shaped_namespace.sh`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e12_audit_clique_baseline.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e12_audit_results.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e12_clique_shaped_namespace.sh`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e12_run_clique_baseline.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e12_run_matrix.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e12_shaped_namespace.sh`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e13_receiver_audit.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_audit_results.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_clique_shaped_namespace.sh`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_export_unified.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_input_plan.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_run_clique_baseline.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_run_matrix.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e14_shaped_namespace.sh`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e15_audit_results.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e15_clique_shaped_namespace.sh`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e15_export_unified.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e15_input_plan.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e15_run_clique_baseline.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e15_run_matrix.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e15_shaped_namespace.sh`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e16_audit_results.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e16_clique_shaped_namespace.sh`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e16_export_unified.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e16_finalize_status.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e16_input_plan.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e16_resource_gate.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e16_run_clique_baseline.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e16_run_matrix.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e16_shaped_namespace.sh`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e17_bridge_n128.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e17_bridge_namespace.sh`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e17_export_bridge.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e17_independent_receiver.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e17_index_evidence.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e17_tcp_probe.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e17_verify_bridge.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e17_verify_index.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/e17_verify_preservation.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/verify_aav86_ca_control_E6_TEST_ONLY.py`
- A `experiments/m6a_p2_i_allpairs/TEST_ONLY/verify_e10_vertices_test.py`
- A `experiments/m6a_p2_i_allpairs/aav86_graph_counter.py`
- A `experiments/m6a_p2_i_allpairs/aav86_n5_r2_test_edges.csv`
- A `experiments/m6a_p2_i_allpairs/material_pool_test.cpp`

### 其他仓库配置与来源

- A `.gitattributes`
- M `.gitignore`
- M `PROJECT.md`
- M `docs/PAPERS.sha256`
- M `docs/REFERENCE_MANIFEST.md`
