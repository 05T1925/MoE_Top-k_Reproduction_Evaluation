# M6B-I-BMW16-S25：实验功能候选复核与运行报告

日期：2026-10-08

方案身份：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL**

候选分支：`codex/m6b-i-bmw16-s25`

实现提交：`112cd8a6d641ded1be5244b317ffb5c42f3b86d1`
报告前 HEAD：`ae803851996e866365b2a97b75f327dab127eb55`（仅修正决策文档格式）

本报告是当前执行聊天对隔离候选的工程与功能检查，不是另一聊天/接收者的独立签收。协议身份保持 EXPERIMENTAL；成功样本与稳定 Top-K oracle 一致不构成 DCF、整池联合视图或 shuffle 的安全证明。BMW16 Theorem 8 的概率和比较常数不适用于本项目实现。

## 1. 基线、代码来源与隔离

- 最新 `main`、`origin/main` 和本分支 merge-base 均为 `c3926c68fd14f270faa8b55234311071947fa080`。候选从该点建立，worktree 为 `C:\Users\28641\.codex\worktrees\m6b-i-bmw16-s25\MoE_Top-k_Reproduction_Evaluation`。
- S24 精确提交是 `efafcd69f9d73b4386e8147df7fb5395afc78720`；S22 runtime 来源是 `9210c11a7a877f8fe3687e0509c1722440cf63fe`。逐路径差异复核显示 S24 没有改变 `VFSS/` runtime。S24 的首个 DCF 变量映射断点仍有效：它否定将压缩 VFSS correction 字段直接等同 BGI15 随机变量的推导；这不是对完整序列化 key 的攻击，也没有证明其函数隐私。
- 以 `origin/main..S22` 的 29 个必要 `VFSS/` 路径为依赖集合，导入实验 party、材料、TLS、启动、Select、DCF/uCMP、shuffle/score/transport 及相关测试；没有 cherry-pick S1–S24 的历史诊断提交或大日志。保留 S4 Python Select 为 TEST_ONLY 对照，并纳入 S12 common-tape 工具。最终 C++ party 仍调用唯一的 `protocol_i_bmw16_experimental_raw_score_mask_party`，没有第二套 Select。
- 主工作区已有的未提交文档与 `siamjdiscrmath.pdf` 保持原样。本分支没有改 `VFSS-baseline/`、`Papers/`、参考工程、E20/E21 或 PR #28。PR #28 仍是 Draft，未改动：[PR #28](https://github.com/05T1925/MoE_Top-k_Reproduction_Evaluation/pull/28)。
- 源码提交 `112cd8a` 含 37 个路径；之后 `ae80385` 仅修改该决策文档的空白格式。所有最终重跑均在该代码已提交且后续无运行时代码变化的树上完成。S24/S22 历史运行数字没有混入本轮测量。

## 2. 接口及执行路径复核

完整 party 路径位于 `VFSS/src/moe_topk/experimental_bmw16_select_party.cpp`，party/T 命令入口位于 `VFSS/src/apps/moe_topk_bmw16_experimental_party_node.cpp`。

1. T 仅接收公开的 `n, K, session, fingerprint` 和角色/凭据配置。它流式生成 raw-score、shuffle、两份 Select task 四轮及 membership 所需材料，分 P0/P1 认证加密交付。两个 receiver 均提交后才发布 pair-ready；启动门要求 T 与 P0/P1 接收进程均退出 0，且绑定的 session/参数匹配，party 才能创建/消费输入。在线阶段 T 不在场、不补发材料。
2. P0/P1 输入为 signed 32-bit Q20.12 原始分数加法份额。`experimental_bmw16_select_adapter.cpp` 调用正式 `protocol_i_raw_score_input_party` 产生带 `original_index` 的稳定 priority key 份额；再将高分优先键作有界反向映射，使 S4-derived Select 使用的序与项目优先级一致。配置要求 `comparison_bits = 33 + index_bits`，C++ API 上限 53 bit；测试覆盖 signed 极值与同分。
3. 两任务共用同一真实记录的 forward shuffle 匿名 handle；dummy/哨兵按材料与算法内的专用类别处理。Select 在同一个四层依赖时钟上运行两任务，每轮只使用此前已确定的图和比较结果。比较 transcript 对 party 可见，这是当前 EXPERIMENTAL 协议行为，不代表它已被证明符合任何更强隐私目标。
4. 成功的两个 Select 输出交集选中一个匿名 handle。membership 对未选中的每条真实记录执行预发 uCMP/DCF key，方向与稳定优先键约定一致；选中项本地产生 membership=1 share。之后用同一置换的 inverse shuffle 将 bit share 恢复到原输入顺序。仅 TEST_ONLY 控制器重构 XOR shares 并逐位核对冻结 oracle；party 只输出自己的原序 bit share。
5. 在线 mTLS 模式将实际 Protocol I framed bytes 交给注册的 TLS stream；缺失/不匹配的 authenticated stream fail closed，不回退到裸 FD。非 TLS FD 模式只作为显式选择的本地测试/旧调用模式存在。

| 阶段 | 发起方/消息 | 本轮可核对的合同 |
|---|---|---|
| 离线 KeyGen/交付 | T→P0/P1：认证 manifest、分块 sidecar/shell、PREPARE/COMMIT/ACK | session、party、参数、stream、摘要和 claim-root 绑定；部分发布不允许启动 |
| 在线启动 | P0↔P1：双向认证 TLS 和 ready 屏障 | 对端身份、session、`n/K/width/fingerprint` 绑定；没有双方最终 ACK 时只可报本地 abort |
| raw-score | P0↔P1：carry/sign adapter 的两组 Protocol I 消息 | 原始 score 不被 party 重构 |
| forward shuffle | P0↔P1：R1、R2 两个 shuffle frame | 建立同一隐藏置换，实际在线轮数另计 |
| sampler + Select | P0↔P1 合币消息；两个 Select task 的 R1–R4 比较边/结果 | Select 有四层比较依赖；它们不是完整入口的四轮 |
| membership/inverse | 预发 uCMP/DCF membership；inverse shuffle 两个 frame | membership 方向对应稳定 Top-K；inverse 输出回原序 share |
| 完成状态 | P0↔P1 最终状态 frame | SUCCESS 需双方状态一致；概率 abort 不返回 mask；不一致/断连无一致性虚称 |

party 状态码合同在实现中区分 `SUCCESS=0`、正常算法 abort `10`、材料错误 `20`、通信错误 `30`、未预期/工程错误 `70`。协议正常抽样失败只在最终双方状态一致时记 `PEER_AGREED`，mask 为空；断连、TLS 认证失败、静默和工程不一致按本地或 peer-observed 状态记录，不能冒充双方一致 abort。

## 3. 构建和验证

### 环境与构建

在 WSL2 Ubuntu 24.04、x86_64 上运行：CMake 3.28.3、GCC 13.3.0、OpenSSL 3.0.13；Windows Python 3.13.7 / NumPy 2.3.5。运行时 n=1000 预检时 WSL 可用内存约 7.0 GiB，`/tmp` 可用空间约 922 GiB。

以下在候选 worktree 中由 WSL2 Ubuntu 24.04 执行；每条 `cmake` 配置命令使用该 worktree 的 WSL 挂载路径，调用形式为 `wsl.exe -d Ubuntu-24.04 -u moeaudit -- <command>`（TEST_ONLY E2E 用 `-u root` 启动隔离控制器）：

```text
cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s25/MoE_Top-k_Reproduction_Evaluation/VFSS -B /tmp/moe_bmw16_s25_off -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=OFF -DMOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=OFF -DMOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=OFF
cmake --build /tmp/moe_bmw16_s25_off --target moe_topk_m1_oracle_test moe_topk_m2_transport_conformance_test moe_topk_m3_protocol_iii_three_process_e2e_test --parallel 2
cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s25/MoE_Top-k_Reproduction_Evaluation/VFSS -B /tmp/moe_bmw16_s25_on -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON -DMOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=OFF -DMOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=OFF
cmake --build /tmp/moe_bmw16_s25_on --target moe_topk_bmw16_experimental_party_node moe_topk_bmw16_experimental_party_node_e2e_test moe_topk_bmw16_experimental_party_test moe_topk_m1_dcf_conformance_test moe_topk_m1_oracle_test moe_topk_m2_priority_dcf_conformance_test moe_topk_m2_ucmp_conformance_test moe_topk_m2_transport_conformance_test moe_topk_m2_score_input_conformance_test moe_topk_m2_cmpagg_conformance_test moe_topk_m2_paper_core_alignment_test moe_topk_m2_parallel_shuffle_conformance_test moe_topk_m2_cmpagg_process_e2e_test moe_topk_m2_parallel_shuffle_process_e2e_test moe_topk_m3_protocol_iii_three_process_e2e_test moe_topk_m3_raw_score_three_process_e2e_test moe_topk_m3_raw_score_secure_executable_test moe_topk_m3_secure_executable_test --parallel 2
cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s25/MoE_Top-k_Reproduction_Evaluation/VFSS -B /tmp/moe_bmw16_s25_test -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON -DMOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=ON -DMOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=OFF
cmake --build /tmp/moe_bmw16_s25_test --target moe_topk_bmw16_experimental_party_node moe_topk_bmw16_experimental_party_node_e2e_test moe_topk_bmw16_experimental_party_test moe_topk_bmw16_experimental_3proc_test --parallel 2
```

三个 Release 配置和相应构建均成功。OFF 配置的 `cmake --build ... --target help` 没有任何 BMW16 目标；正常 ON Release 将 `MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=OFF`，TEST_ONLY 专用构建才将其置 ON。全仓 CTest 未运行，状态为 `NOT_RUN`。

最终源码上的定向回归命令为：

```text
ctest --test-dir /tmp/moe_bmw16_s25_on --output-on-failure -R "moe_topk_(bmw16_experimental_party_test|m1_dcf_conformance_test|m1_oracle_test|m2_priority_dcf_conformance_test|m2_ucmp_conformance_test|m2_transport_conformance_test|m2_score_input_conformance_test|m2_cmpagg_conformance_test|m2_paper_core_alignment_test|m2_parallel_shuffle_conformance_test|m2_cmpagg_process_e2e_test|m2_parallel_shuffle_process_e2e_test|m3_protocol_iii_three_process_e2e_test|m3_raw_score_three_process_e2e_test|m3_raw_score_secure_executable_test|m3_secure_executable_test)"
```

结果：`16/16 PASS`，10.36 秒。包括当前 BMW16 C++ 对照以及相关 Protocol I/III 定向回归；不表示 41 项完整仓库 CTest 已执行。

### Select differential 与独立进程 E2E

TEST_ONLY common-tape 差分使用冻结 S4 Python 源码与 C++ `party_test`，逐项复用外部 tape，并对中间选择 trace 做差分，不是仅比最终 mask：

```text
py experiments/TEST_ONLY_BMW16_S12/run_common_tape_matrix.py --wsl-distro Ubuntu-24.04 --cpp-test /tmp/moe_bmw16_s25_test/moe_topk_bmw16_experimental_party_test --s4 experiments/TEST_ONLY_BMW16_S7_COMPOSITION/select4r_s4_source.py --out C:\Users\28641\.codex\artifacts\BMW16_S25_20261008\common_tape_final
```

39/39 cases PASS：n=2..8 的共同随机带用例和四个 n=64 profile；样本、pivot、中间集合、逐轮边、比较结果、abort 类别和 selected handle 由 harness 核查。

最终候选树上的独立 UID/进程 E2E 命令：

```text
/tmp/moe_bmw16_s25_test/moe_topk_bmw16_experimental_party_node_e2e_test /tmp/moe_bmw16_s25_test/moe_topk_bmw16_experimental_party_node --small-ranks
/tmp/moe_bmw16_s25_test/moe_topk_bmw16_experimental_party_node_e2e_test /tmp/moe_bmw16_s25_test/moe_topk_bmw16_experimental_party_node --extended
/tmp/moe_bmw16_s25_test/moe_topk_bmw16_experimental_party_node_e2e_test /tmp/moe_bmw16_s25_test/moe_topk_bmw16_experimental_party_node --tls
/tmp/moe_bmw16_s25_on/moe_topk_bmw16_experimental_party_node_e2e_test /tmp/moe_bmw16_s25_on/moe_topk_bmw16_experimental_party_node --tls-n1000
```

| 当前候选上的运行 | 结果和范围 |
|---|---|
| `--small-ranks` | 39 个 SUCCESS 均经冻结 C++ oracle 核验。含 n=2..8 全部 K（35 个 rank）及 n=1/固定边界输入；另执行缺材料/截断/篡改负例。输入含重复、负数、`INT32_MIN/MAX`。 |
| `--extended` | 14 个 SUCCESS oracle 核验。n=64 的 K=1/8/n；n=128 的 K=1/8/80/n；n=256 的 K=1/2/n；附重复与 seeded 输入。跨规模成功 mask 均长度 n、bit 值合法、weight=K、逐位吻合 oracle。 |
| `--tls` | 8 个 mTLS TCP loopback SUCCESS oracle 核验，覆盖 n=2/3/5/8/64/128/256。不同 UID 检查双方目录隔离；包括 durable replay、错 T/party/session/stream、receiver publish/fsync/silent、chunk 重排/截断/篡改、旧/错身份 TLS、缺 stream、断连/静默、算法 abort、工程错误和最终状态不一致。 |
| `--tls-n1000` | 最终候选正常 Release（failpoint OFF）上 fresh-material、mTLS、独立 P0/P1 UID 的 n=1000、K=80 全链运行 SUCCESS，oracle 核验通过。不是 S22 历史结果。 |

`--tls` 的故障矩阵中，测试正常算法 abort 得到双方 exit 10、`PEER_AGREED`、无 mask；材料 replay 为双方 exit 20、无 mask；身份/传输/peer 失败为材料或通信 abort；意外/注入 invariant 错误与最终状态不一致 exit 70，且无 mask。无 ACK 的失败只作为 `LOCAL_ONLY` 或 `PEER_OBSERVED`。本轮合法随机执行样本未观察到自然概率 abort；这个有限样本观察不提供理论成功率。注入 abort 未计入抽样概率。

n=1000 记录（单次功能执行，不是性能基准）：

- `slots_per_party=4,497,550`；sidecar 每方 `4,967,527,664` bytes，shell 每方 `2,013,901` bytes；TLS 总发送 `9,941,055,454` bytes。
- T 日志的 `offline_elapsed_us=520,355,012`、`delivery_elapsed_us=322,110,399`；P0/P1 接收计时分别 `520,349,546/520,338,641` 微秒。各字段沿用 harness 计时边界，不相加当作端到端时间。
- T、receiver 和两 party 的退出码为 0；party 输出为秘密 share，只有 TEST_ONLY harness 重构核验。双 UID 访问隔离检查 PASS，材料重领双方 exit 20 且 `mask=NONE`。
- 这只是 loopback、单次、默认样本的功能诊断；非 LAN/WAN，不用于比较排序或正式九指标。

## 4. 二进制与证据身份

最终运行时源码提交为 `112cd8a6d641ded1be5244b317ffb5c42f3b86d1`。构建在 `ae803851996e866365b2a97b75f327dab127eb55` 之后重新配置/构建；该提交与实现提交之间只有上述文档空白修正，没有 C++/Python 运行时差异。由于 WSL 对此 Windows worktree 的 `.git` 文件指向 Windows 绝对路径，WSL 内嵌入式 Git revision 查询不可用；因此 provenance 由 Windows Git revision、明确的构建命令、源码 SHA-256 和二进制 SHA-256 外部绑定，不声称二进制自带 revision 字符串。

| 对象 | SHA-256 |
|---|---|
| `VFSS/src/moe_topk/experimental_bmw16_select_party.cpp` | `a5f61e04f0be8a6722a579f918bce38c94069fd231b235916f1d4ab0a61301ce` |
| `VFSS/src/apps/moe_topk_bmw16_experimental_party_node.cpp` | `ac74c39ebe18081df5be1520ed1436babbca19cc88fbd7803f8843bd896bcd82` |
| `VFSS/src/moe_topk/experimental_bmw16_material_bundle.cpp` | `174c3e91551aa4b8b896f6ca3af42c57a8be13234a5bbad5197839000a6fbe4c` |
| `VFSS/src/moe_topk/experimental_bmw16_material_delivery.cpp` | `0491bda0aec58e9608624bcc9286c692f1d3838651fb553462c6a7b8d8e33d99` |
| `VFSS/src/moe_topk/experimental_bmw16_online_tls.cpp` | `0578b7179673ecb7eb3b10b551f4d7b3d17f015d7aa69778af3aa80c5db45461` |
| TEST_ONLY S4 source copy | `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab` |
| normal opt-in Release `party_node` | `8b80cb68fe5fde9386255b35379ba42dd49fcceb42225976ad52c84a9e5a06d0` |
| TEST_ONLY failpoint `party_node` | `7c485120d6200dc57a1a2f5cb590735bf85576b51fad272df40f93d05de9d638` |
| `party_node_e2e_test` | `d1f342718aa2c6b3091b5d4272c0acd037f99b5c3c81000b4e50daa5579a4638` |
| `party_test` | `dddd9acae4e87479ee30f7e4762cceed971edea8fd3d4a4e313bab96969a4da5` |

逐次 logs、tapes、traces、临时材料与测试证书均位于仓库外：`C:\Users\28641\.codex\artifacts\BMW16_S25_20261008` 及 WSL 临时目录。临时 n=1000 package 在 harness 退出后清理；密钥/凭据/sidecar 没有进 Git。原始证据哈希：

| 外部证据 | SHA-256 |
|---|---|
| `ctest_targeted_final.log` | `e9c0239e487e049222322c4f9bc62562bebf8eeff8343393d97eda6eec41147d` |
| `common_tape_final/matrix.json` | `b354496705086f249e9c645795674be365a36d7e9cc0e1fb514577ea857fffd0` |
| `e2e_small_all_ranks.log` | `ee3d94d094181149b1ea54e08cf5cadcf66374581574f191b131a85f982e8274` |
| `e2e_extended_n64_128_256.log` | `bb8e5a6a92ea3d7b744e023d458087612500fc9c9e0a7f621093f53f45af0240` |
| `e2e_tls_fault_matrix.log` | `d587eabd9dbd956cdb4b2b018d0a4c76b1cb5077f487374d11296585d2bf7f86` |
| `e2e_tls_n1000.log` | `e3d6349a3ad125d7df4f4e963c9c77dfef0ecde84543b4a8c8212919c408f676` |

## 5. 指标状态、门禁与限制

| 门 | 判定 | 依据/边界 |
|---|---|---|
| `EXPERIMENTAL_FUNCTIONAL_ACCEPTANCE` | **PASS（有界候选）** | 最终运行时代码提交上的 common-tape、CTest、不同 UID 三进程、mTLS 和 n=1000 SUCCESS oracle 检查均完成。自然概率 abort 样本数有限，概率界未在此门验收。 |
| `DEFAULT_OFF_AND_BASELINE_ISOLATION` | **PASS** | OFF 配置成功构建既有目标且 BMW16 party-node target 不存在；正常 Release opt-in 与 TEST_ONLY failpoint 构建分开；冻结 baseline 零差异。主工作区未提交差异保留。 |
| `MATERIAL_AND_TRANSPORT_CONTRACT` | **PASS（当前单机实验范围）** | fresh 分方材料、TLS 1.3 loopback、独立 UID、pair-ready/T-exit gate、持久一次性 claim、错身份/损坏/截断/重放/缺 stream 和 abort 负例已执行。**不代表生产证书注册/轮换、跨主机运维或管理员/快照回滚防护。** |
| `SOURCE_TO_BINARY_PROVENANCE` | **PASS（外部可追溯）** | 运行时 commit、构建配置/命令、重点源码和二进制哈希完整记录；binary 不嵌入可读 revision，WSL 内 Git rev 查询受 worktree `.git` Windows 路径影响。 |
| `DCF_SINGLE_KEY_PRIVACY` | **NO-GO / UNPROVEN** | S24 已指出与 BGI15 随机字段逐项映射的分布断点；没有针对当前完整 serialized key 的攻击，但本报告不提供完整函数隐私证明。 |
| `FULL_POOL_AND_SHUFFLE_VIEW` | **NO-GO / UNPROVEN** | 相关整池 key、打开比较位下本地 Eval share 的自适应模拟，以及真实 forward/inverse shuffle 到输出 share 的联合模拟仍没有独立可审查证明。测试未关闭该门。 |
| `SECURE_ALIAS_READY` | **NO-GO** | 保持无 secure alias；不把公开 transcript 扩大为“已经证明安全”的结论。 |
| `FORMAL_PERFORMANCE_READY` | **NO-GO / NOT_READY** | 没有 V4 正式 LAN/WAN 1+5 矩阵、同口径基线配对或完整九指标。n=1000 单次 loopback 仅功能验证。 |

统一九指标预留：逻辑比较调用、unique FSS/uCMP 槽、DCF Eval、party 收发字节、材料槽/有效载荷/密文包长、离线与在线时间、因果消息轮数、PRG 调用与 AES block 数、总时间/资源/abort 成本。n=1000 原始输出只给出材料/T 交付的一部分计时与字节；完整在线 PRG、收发、消息 phase、端到端计时及配对网络性能均为 `NOT_MEASURED`。n=64–256 日志中的逻辑比较/uCMP/DCF 和 RSS 是实验诊断字段，不等于已冻结的正式九指标矩阵。

## 6. 修改文件

S25 runtime candidate commit `112cd8a` 纳入 37 个路径：

- `VFSS/CMakeLists.txt`
- `VFSS/ext/FSS/dcf.cpp`、`VFSS/ext/FSS/include/FSS/dcf.h`
- `VFSS/include/moe_topk/experimental_bmw16_material_bundle.h`、`experimental_bmw16_material_delivery.h`、`experimental_bmw16_online_tls.h`、`experimental_bmw16_select_adapter.h`、`experimental_bmw16_select_party.h`、`experimental_bmw16_startup_gate.h`、`experimental_bmw16_stream_store.h`
- `VFSS/include/moe_topk/protocol_i_parallel_shuffle.h`、`protocol_i_score_input.h`、`protocol_i_transport.h`
- `VFSS/src/apps/moe_topk_bmw16_experimental_party_node.cpp`
- `VFSS/src/moe_topk/experimental_bmw16_material_bundle.cpp`、`experimental_bmw16_material_delivery.cpp`、`experimental_bmw16_online_tls.cpp`、`experimental_bmw16_select_adapter.cpp`、`experimental_bmw16_select_party.cpp`、`experimental_bmw16_startup_gate.cpp`、`experimental_bmw16_stream_store.cpp`
- `VFSS/src/moe_topk/protocol_i_parallel_shuffle.cpp`、`protocol_i_score_input.cpp`、`protocol_i_transport.cpp`
- `VFSS/tests/moe_topk/experimental_bmw16_3proc_test.cpp`、`experimental_bmw16_party_node_e2e_test.cpp`、`experimental_bmw16_party_test.cpp`、`protocol_i_score_input_conformance_test.cpp`、`protocol_i_transport_conformance_test.cpp`
- `experiments/TEST_ONLY_BMW16_S12/README.md`、`compare_common_tape.py`、`run_common_tape_matrix.py`、`experiments/TEST_ONLY_BMW16_S7_COMPOSITION/select4r_s4_source.py`
- `PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md`、`docs/BENCHMARK_VALIDATION_PLAN.md`、`docs/decisions/BMW16_S25_EXPERIMENTAL_FUNCTIONAL_CANDIDATE_DECISION_2026-10-08.md`

报告本身新增：`docs/reproduction/BMW16_S25_FUNCTIONAL_CANDIDATE_ACCEPTANCE_2026-10-08.md`。报告完成后运行 `git diff --check` 并检查最终差异不包含 baseline、论文、密钥/证书、sidecar、构建物或逐次大日志。下个接收者应从本地候选的最终 HEAD 独立构建和复核；本报告作者不自称已完成异会话接收。
