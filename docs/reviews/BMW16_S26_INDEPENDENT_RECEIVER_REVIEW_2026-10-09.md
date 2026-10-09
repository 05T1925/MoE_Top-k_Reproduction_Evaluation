# S26 BMW16 实验候选：独立接收复核

日期：2026-10-09。接收者未参与 S25/S26 编写、运行或自签收。本报告只审查 S26 最终功能候选是否可作为 Draft PR 供代码评审；不评判 S27 安全证明。

## 身份与裁决

- 实现标签：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL**。这是项目扩展，不是 BB90 原算法、BMW16 Algorithm 7 逐字实现或论文安全定理实例。
- 最新 `origin/main`、merge-base：`c3926c68fd14f270faa8b55234311071947fa080`。S26 最终树：`2818bce20f30719eaccf0bc5df586cce4fe78c84`。实际运行时修复：`ded433d636a3df929380bdce0ff22f4ce6121453`；此后提交只改文档。
- 独立集成分支从该 `main` 建立并无冲突快进到 S26 最终树。相对 `main` 的 40 个源/文档/TEST_ONLY 路径中，无 `VFSS-baseline/`、`Papers/`、参考树、密钥、证书、sidecar、构建物或日志；`git diff --check origin/main HEAD` 通过。桌面主工作区、S25/S26/S27 worktree 与 PR #28 未修改。
- **`DRAFT_PR_READY=PASS`，仅限默认关闭的实验功能候选。** 源码、证据身份、定向正确性、隔离三进程与发布回滚具有可审查性。没有创建 secure alias，没有宣称安全证明或正式性能验收，也不改变 M6A→M6B 正式门。

## 源码、二进制与旧证据身份

原 S26 worktree 的 `sha256_index.txt` 中 7 个 `repo/*` 和 14 个 `external/*` 对象逐一重算全部匹配；原构建目录四个二进制哈希也匹配 S26 报告，包括 Release party-node `ba605cbb9970f0365320099b5a5e006cbb6b2fcb489d085d440bcce91a44ee1d` 与 TEST_ONLY party-node `b025214d7498df6aa78989e5a62ea66341a9ba006eb6057bc6e1fa9f70f4e279`。这核对了原证据，不能当成本接收者新运行。

本接收 worktree 检出的文本由 Windows `core.autocrlf=true` 展开换行，因此物理文件 SHA-256 与 S26 原混合换行文件不同；`HEAD` 的 Git blob 与最终提交一致，没有源码差异。接收者在 WSL2 Ubuntu 24.04、GCC 13.3.0、CMake 3.28.3、OpenSSL 3.0.13、Release 配置下独立重建。新二进制身份如下，不能混写为 S26 原二进制：

| 接收者构建 | SHA-256 |
| --- | --- |
| ON Release party-node；failpoints OFF，DCF counters OFF | `207fa371f2c4f26cfece3562b2af505719828cea2c7f1c56c0d6f493573b2aac` |
| ON Release E2E harness | `0780004c368cedd2503b87051ac572b3dcc7eceea2fbe82977d867c86481968a` |
| TEST_ONLY party-node；failpoints ON，DCF counters ON | `fa00face340b087a3bfff42dc4f1db13609d635bac3ec5e9b2e1c5eb489082a2` |
| TEST_ONLY Select party/conformance binary | `ecaeaf3e34d3d1533524e3599d29ead823bdc83f68b88adb482feb0afb4e965c` |

默认 OFF 配置的 target help 不含 BMW16 目标；独立 Release party-node 的 `strings` 无发布 failpoint 名，TEST_ONLY party-node 包含该名。S26 记录的全仓 OFF 构建被既有 `bitpack_test` 链接错误截断及 38 个 CTest 未运行这一边界，经原日志哈希核对；本轮没有把它记为全仓通过。

## 源码与协议边界复核

唯一完整入口是 `protocol_i_bmw16_experimental_raw_score_mask_party`：signed Q20.12 原始分数加法 shares → 稳定 priority key → 同置换 forward shuffle → 两份 Select task 的四个比较依赖层 → DCF/uCMP membership → 同置换 inverse shuffle → 原序 XOR mask shares。T 按公开 `n/K/session/fingerprint` 离线发材并退出；P0/P1 在输入创建后在线执行。输入 score、原始 index、selected key 和完整明文 mask 未在 party 路径重构；冻结 oracle 的 shares 重构位于 TEST_ONLY 控制器。匿名端点、比较 bit、pivot/集合位置、masked operand、selected anonymous handle 等为此实验候选已声明的可见 transcript，不能据此推出可模拟性。

S26 的 `write_mask_exclusive` 先同步暂存数据，no-clobber 发布，清理暂存名并同步目录；发布后的中间错误回滚本次最终路径和暂存路径，保留调用前已有目的文件。故障注入验证了两处发布后失败均 exit 70 且无 mask。若设备故障导致回滚 `unlink/fsync` 自身失败，仍须由外层拒收非零退出的尝试；此处没有证明跨设备/管理员故障安全。

## 接收者实际执行：conformance → oracle differential → 独立进程 E2E

构建目录均在 WSL `/tmp/`，逐次日志和 common-tape 输出均在仓库外 `C:\Users\28641\.codex\artifacts\BMW16_S26_RECEIVER_20261009\`。主要命令：

```text
cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s26-independent-review/MoE_Top-k_Reproduction_Evaluation/VFSS -B /tmp/moe_bmw16_s26_receiver_on -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON -DMOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=OFF -DMOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=OFF
cmake --build /tmp/moe_bmw16_s26_receiver_on --target moe_topk_bmw16_experimental_party_node moe_topk_bmw16_experimental_party_node_e2e_test moe_topk_bmw16_experimental_party_test moe_topk_m1_dcf_conformance_test moe_topk_m2_score_input_conformance_test moe_topk_m2_transport_conformance_test moe_topk_m2_parallel_shuffle_conformance_test moe_topk_m3_protocol_iii_three_process_e2e_test --parallel 2
ctest --test-dir /tmp/moe_bmw16_s26_receiver_on --output-on-failure -R moe_topk_m1_dcf_conformance_test
ctest --test-dir /tmp/moe_bmw16_s26_receiver_on --output-on-failure -R moe_topk_m2_score_input_conformance_test
ctest --test-dir /tmp/moe_bmw16_s26_receiver_on --output-on-failure -R moe_topk_m2_transport_conformance_test
ctest --test-dir /tmp/moe_bmw16_s26_receiver_on --output-on-failure -R moe_topk_m2_parallel_shuffle_conformance_test
ctest --test-dir /tmp/moe_bmw16_s26_receiver_on --output-on-failure -R moe_topk_bmw16_experimental_party_test
py -3 experiments/TEST_ONLY_BMW16_S12/run_common_tape_matrix.py --wsl-distro Ubuntu-24.04 --cpp-test /tmp/moe_bmw16_s26_receiver_on/moe_topk_bmw16_experimental_party_test --s4 experiments/TEST_ONLY_BMW16_S7_COMPOSITION/select4r_s4_source.py --out C:\Users\28641\.codex\artifacts\BMW16_S26_RECEIVER_20261009\common_tape
cmake -S <same VFSS source> -B /tmp/moe_bmw16_s26_receiver_test -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON -DMOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=ON -DMOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=ON
cmake --build /tmp/moe_bmw16_s26_receiver_test --target moe_topk_bmw16_experimental_party_node moe_topk_bmw16_experimental_party_node_e2e_test moe_topk_bmw16_experimental_party_test --parallel 2
/tmp/moe_bmw16_s26_receiver_test/moe_topk_bmw16_experimental_party_node_e2e_test /tmp/moe_bmw16_s26_receiver_test/moe_topk_bmw16_experimental_party_node --small-ranks
/tmp/moe_bmw16_s26_receiver_test/moe_topk_bmw16_experimental_party_node_e2e_test /tmp/moe_bmw16_s26_receiver_test/moe_topk_bmw16_experimental_party_node --tls
ctest --test-dir /tmp/moe_bmw16_s26_receiver_on --output-on-failure -R moe_topk_m3_protocol_iii_three_process_e2e_test
```

| 门 | 接收者结果 |
| --- | --- |
| Conformance | 5/5：M1 DCF、M2 score input、transport、parallel shuffle、BMW16 party test。`ctest_conformance.log` SHA-256 `94656cf0bf9cf07e8b3a69e051092f18f1ee08e37ba5f5a3de9b41c728ef9907`。 |
| 冻结 oracle differential | S4 common-tape 39/39 PASS，含 n=2..8 所有 K 与四个 n=64 profile。`common_tape/matrix.json` SHA-256 `36e103791d2be14562fe139d40e6aa1ce92784c08fa59623de3629fdf26fa5ec`。 |
| 独立 T/P0/P1，小规模全 K | TEST_ONLY 构建 `--small-ranks` exit 0；39 个 `SUCCESS_ORACLE_CHECKED`，P0/P1 各 39 个成功退出，包括 n=2..8 所有 35 个 K。日志 SHA-256 `98954760971c811b9a7b7ffcbf0791f7cd5fa8f95e698d3844d2f7c15a7e55e6`。 |
| 独立 T/P0/P1，mTLS 与故障 | TEST_ONLY `--tls` exit 0；7 个三角色 mTLS oracle 成功，另有 n=1 shortcut；15 个 UID 隔离断言，两种 mask 发布后故障均 exit 70、mask=NONE。日志 SHA-256 `4078741bed2452908be69731d6f8072e69a42fe186e4e233066116d05dee5c72`。仅本机不同 UID 与 TCP loopback。 |
| 相关旧路线回归 | M3 Protocol III three-process E2E 1/1 PASS。全仓 CTest、本轮 n=1000 重跑和正式 LAN/WAN 矩阵均未运行。 |

一次接收者命令选择错误也保留：以正常 Release node 执行 `--small-ranks` 时，harness 所需 `t-local-test-only` 命令未编译，离线 T 即拒绝启动，exit 1；此尝试无在线协议。错误日志 SHA-256 `c80036fbeafa64f6e155aa38ad0572b6c6ddf73b17253e9b6694fa736e4bf643`。之后使用匹配 TEST_ONLY 构建成功重跑。S26 原 n=1000/K=80 loopback 日志哈希被核对，但不是本轮独立重跑，也不进入性能表。

## 逐门结论和未解决项

| 门 | 结论 |
| --- | --- |
| `DEFAULT_OFF_AND_BASELINE_ISOLATION` | PASS：默认 target 隔离、目录差异和 PR #28 未改。 |
| `EXPERIMENTAL_FUNCTIONAL_ACCEPTANCE` | PASS（上述有界配置）；不外推大规模或随机失败概率保证。 |
| `MATERIAL_AND_TRANSPORT_CONTRACT` | PASS（本地受测范围）；生产凭据注册、跨主机服务管理与管理员/快照回滚未验。 |
| `DCF_SINGLE_KEY_PRIVACY`、`FULL_POOL_AND_SHUFFLE_VIEW` | UNPROVEN；本报告不审 S27，也不把功能测试作为安全证明。 |
| `SECURE_ALIAS_READY` | NO-GO。 |
| `FORMAL_PERFORMANCE_READY` | NO-GO；统一九指标与正式 LAN/WAN 矩阵 `NOT_MEASURED`。 |
| `DRAFT_PR_READY` | PASS：只供实验功能候选的 Draft 代码评审，不合并或转 Ready for Review。 |

本 PR 的文件范围仅为 S26 候选的 40 个原路径及本独立接收报告；完整清单以 `git diff --name-only origin/main HEAD` 为准。

## 文件清单

```text
PROJECT.md
VFSS/CMakeLists.txt
VFSS/ext/FSS/dcf.cpp
VFSS/ext/FSS/include/FSS/dcf.h
VFSS/include/moe_topk/experimental_bmw16_material_bundle.h
VFSS/include/moe_topk/experimental_bmw16_material_delivery.h
VFSS/include/moe_topk/experimental_bmw16_online_tls.h
VFSS/include/moe_topk/experimental_bmw16_select_adapter.h
VFSS/include/moe_topk/experimental_bmw16_select_party.h
VFSS/include/moe_topk/experimental_bmw16_startup_gate.h
VFSS/include/moe_topk/experimental_bmw16_stream_store.h
VFSS/include/moe_topk/protocol_i_parallel_shuffle.h
VFSS/include/moe_topk/protocol_i_score_input.h
VFSS/include/moe_topk/protocol_i_transport.h
VFSS/src/apps/moe_topk_bmw16_experimental_party_node.cpp
VFSS/src/moe_topk/experimental_bmw16_material_bundle.cpp
VFSS/src/moe_topk/experimental_bmw16_material_delivery.cpp
VFSS/src/moe_topk/experimental_bmw16_online_tls.cpp
VFSS/src/moe_topk/experimental_bmw16_select_adapter.cpp
VFSS/src/moe_topk/experimental_bmw16_select_party.cpp
VFSS/src/moe_topk/experimental_bmw16_startup_gate.cpp
VFSS/src/moe_topk/experimental_bmw16_stream_store.cpp
VFSS/src/moe_topk/protocol_i_parallel_shuffle.cpp
VFSS/src/moe_topk/protocol_i_score_input.cpp
VFSS/src/moe_topk/protocol_i_transport.cpp
VFSS/tests/moe_topk/experimental_bmw16_3proc_test.cpp
VFSS/tests/moe_topk/experimental_bmw16_party_node_e2e_test.cpp
VFSS/tests/moe_topk/experimental_bmw16_party_test.cpp
VFSS/tests/moe_topk/protocol_i_score_input_conformance_test.cpp
VFSS/tests/moe_topk/protocol_i_transport_conformance_test.cpp
docs/BENCHMARK_VALIDATION_PLAN.md
docs/IMPLEMENTATION_PLAN.md
docs/decisions/BMW16_S25_EXPERIMENTAL_FUNCTIONAL_CANDIDATE_DECISION_2026-10-08.md
docs/decisions/BMW16_S26_POST_CANDIDATE_REVIEW_DECISION_2026-10-09.md
docs/reproduction/BMW16_S25_FUNCTIONAL_CANDIDATE_ACCEPTANCE_2026-10-08.md
docs/reproduction/BMW16_S26_POST_CANDIDATE_REVIEW_2026-10-09.md
experiments/TEST_ONLY_BMW16_S12/README.md
experiments/TEST_ONLY_BMW16_S12/compare_common_tape.py
experiments/TEST_ONLY_BMW16_S12/run_common_tape_matrix.py
experiments/TEST_ONLY_BMW16_S7_COMPOSITION/select4r_s4_source.py
docs/reviews/BMW16_S26_INDEPENDENT_RECEIVER_REVIEW_2026-10-09.md
```
