# BMW16 S17 独立接收、party-node 与流式侧车验证报告

日期：2026-10-07
方案：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL**
工作分支：`codex/m6b-i-bmw16-s17`
工作起点：`f3f20cc7afb742ecfb43acd972ff8d8d37664ffd`
被测代码为本分支修改后的工作树；最终源码 SHA、二进制 SHA 及结果 SHA 见外部证据索引。文档提交/最终本地提交不代表被测源码之前的 revision。

## 1. 基线和证据分类

本轮在独立 S17 worktree 开工；开始时 branch HEAD 为 S16 文档提交、工作树 clean。最新 main 与 origin/main 为 `c3926c68fd14f270faa8b55234311071947fa080`，S16 核心被测源码为 `a95d7cec8105da0c418a465d96f314830a873630`。S15 历史 pilot 的源码绑定状态仍是 S16 所判 `UNVERIFIED`；S16 数学界、61 例和 counter 结果是历史结论，不改写为 S17 新数据。E20 (`8986a401…`) 仅作为流式/发材工程参考。

证据层次分为：

- **论文/协议历史**：BMW16 只提供派生 Select 的算法来源，不给本组合继承 Theorem 8 概率或 BB90 常数；Protocol I 的既有 raw adapter、shuffle/uCMP/DCF 是仓库代码事实。
- **S16 已有推导**：SUCCESS soundness 和 ideal uniform-without-replacement abort 界见 S16 决策；S17 不重写这些数学推导，也不以测试替代它们。
- **S17 项目实现**：party-node CLI、root-bound v3 bundle、随机访问侧车与本轮 E2E 是 PROJECT_DERIVED/EXPERIMENTAL。
- **未证明/未测**：整池 adaptive DCF 安全、shuffle/inverse 联合 simulator、生产 T→party 私有认证传输、n=1000 streaming KeyGen、V4 网络性能。

## 2. 本轮实现与源码复核

### 2.1 Party-node 调用入口

新增 `moe_topk_bmw16_experimental_party_node`（只在 `MOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON` 时编译）。T 子命令只带公开参数与预置的双方 credential 路径/UID，运行 `protocol_i_bmw16_experimental_material_generate()`、分方 AES-GCM 封包与本地受限交付，然后退出。Party 子命令只读取本方 raw share、bundle、recipient key 与 claim directory；校验文件 owner/mode；open v3 bundle；在进 Protocol I API 前先 durable-claim 整包；随后以 12 个既有协议 FD 执行 API，并另用一个 framed READY FD 同步两边已准备完成。

节点不实现 TLS/远程 authenticated transport；在线 FD 是调用方注入且被假设为私有认证通道。T 的本地 bundle 写入也不是跨主机传输。party 测试控制器是 root，负责 fixture/启动/最终 oracle，但该权限不属于 party 单方 view。T 退出后才生成 raw shares；P0/P1 分别以 UID 22012/22013 `fork+exec`。因此本轮证明的是隔离测试环境中的本地文件/UID 路径，不是生产 credentials/网络发材。

party API 成功时只写本方 mask share；算法 abort 无 mask，材料错误、通信错误及意外错误使用不同返回状态/退出码。节点测试成功配置输出 `SUCCESS=0`；现有三进程异常矩阵检查 algorithm abort=10、material=20、communication=30、unexpected=70。无对端 ACK 时只能证明本地 abort。

### 2.2 Root-bound bundle 和一次性

bundle v3 头部新增 claim-root device/inode，进入 AES-GCM AAD 并与 session/party/n/K/fingerprint/padding/width/manifest 同步校验；persistent claim store 核当前 root 身份；v2 memory fixture 在持久 root 场景被拒绝。此前 parser 把 root bytes 交给持有引用的 Reader 时引用了临时 vector；S17 源码使用具名 `root_bytes` 生命周期，避免悬空引用。

这一身份只识别 root inode，不实现 rollback-resistant hardware/ledger。接收范围为合同 A：同 bundle 只在同一受保护且不可回滚的 party claim root 使用。S16 的 3proc fault matrix 在当前源码重建后重跑：wrong recipient/party/session/width、truncated tag、AEAD tamper、不同 claim root copy、wrong root、duplicate claim、duplicate slot、burn 后重启、peer close/silent 均拒绝。该测试不覆盖 root snapshot/backup 回滚或 root 管理员。

### 2.3 流式侧车

`ProtocolIBmw16StreamedUcmpSlotStore` 在固定 132-byte header 后存放规范排序的密封 uCMP 记录；一条记录含 `57+24·comparison_bits` 字节 serialized key 与 16-byte GCM tag。每条 AAD 再绑完整配置/root prefix 与 `slot id,stage,task,round,left,right`。读取经 `pread` 随机寻址，检查 exact file size、UID/mode、root 与参数，AEAD 失败或重复槽读取报错。进程内用 mutex + loaded-id set 实现测试路径的单次 `load_once`；正式 bundle claim 仍是整包 durability 边界。

逐槽对照覆盖 9·C(n,2) Select+membership keys，并检查其序列化字节和内存源完全相同；wrong root、改首槽 ciphertext、truncated last byte 与 duplicate read 全部拒绝。`n=5` 侧车例中 90 槽/party，密封文件 84,462 B/party。该量只计侧车文件，不计原始 score/shuffle bundle。

限制很关键：源 KeyGen 仍先把所有 raw、shuffle、Select 和 membership 材料同时驻留在 `ProtocolIBmw16ExperimentalMaterialPair`。sidecar 只涵盖 Select/membership，raw/shuffle 仍走内存对象。因此这不降低 KeyGen peak RAM、不降低 O(n²) 离线总字节、也不是 n=1000 实现。实测的是随机访问存储/懒加载，不是流式生成。

## 3. S16 独立复跑和 S4 差分

构建基于 Windows 11 上 WSL2 Ubuntu 24.04，GNU C/C++ 13.3，Release，OpenSSL 3，BMW16 adapter ON，FSS DCF PRG research counter ON，EMP OT OFF。临时构建目录 `/tmp/bmw16-s17-build`；证据材料位于 `C:\Users\28641\.codex\artifacts\BMW16_S17_20261007`。构建二进制均在仓库外。

另用相同源码配置 `MOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=OFF` 的 `/tmp/bmw16-s17-default-off`；检查 CMake target help 未列出任何 BMW16 target，确认新增入口仍由 opt-in 开关控制。

S17 当前源码下重跑既有 3proc TEST_ONLY matrix：61 行；57 SUCCESS、1 显式注入算法 abort、1 bundle/material abort、1 unexpected exception (exit 70)、1 peer-close/communication abort。每个 SUCCESS 由隔离控制器重构后与仓库 C++ oracle 核对。失败行不计入成功率；算法失败在此 matrix 是注入的 `TEST_ONLY_FORCED_ABORT_AFTER_SELECT`，不是自然抽样失败。负例：wrong key、wrong party/session/width、AEAD tamper/truncation、异 root copy、wrong store root、duplicate bundle claim、duplicate in-process slot、burn-after-crash、peer close、peer silent 均为 reject/local abort。

同一 Release build 的 `moe_topk_bmw16_experimental_party_test` 11 行：singleton、n=2/3/5/8、K 边界/重复与极值、n=64/K=8、forced abort，以及 n=5/K=2 sidecar 路径。所有 SUCCESS 与 `top_k_mask` oracle 相同且 weight=K；注入 abort 的双方输出 share 为空。最终重跑 n=64 实测逻辑调用 17,550、unique Select slots 5,421、membership slots 63、每方 uCMP 5,612 eval、DCF 11,224 eval、两方汇总 DCF 22,448 `evalDCF` calls；hook 记录 ordinary DCF eval 的 AES node expansions 872,912、对应 1,745,824 blocks。每次运行会重新生成 shuffle/material，因此该处计数以外部最终日志为准。该 PRG 字段不覆盖 KeyGen、shuffle 或全库随机调用。

直接 party-node fork/exec E2E 基础模式覆盖 n=1、2、5、8；`--extended` 模式另外运行 n=64/K=8、n=128/K=8、n=256/K=2。最终扩展复跑七例全部 `SUCCESS_ORACLE_CHECKED`，party exit 均为 0；输入覆盖 singleton、INT32_MIN/MAX、重复分数和确定性大规模混合分数。测试控制器按 session 固定输入与 raw-share split；T/party 材料与协议随机币仍由 OS 熵新鲜产生，没有把测试 tape 传给 party。每例先运行 T，T 退出后才生成 shares；P0/P1 是两个 UID。控制器在双方退出后重构每个 mask 并与冻结 oracle 逐位核验。四个大规模 case 的 T bundle 实际字节/方分别为 n=64: 18,654,137、n=128: 76,701,593、n=256: 314,544,473；party 两侧分别记录逻辑比较 17,932/40,404/81,348，单方 uCMP 6,085/11,453/18,428 eval，单方 DCF 12,170/22,906/36,856 eval。n=8/K=3 时双方各 `ready_sent=58 B/ready_recv=58 B`，各 180 logical comparisons、109 uCMP eval、218 DCF eval；T 报告 270 slots/party、v3 root-bound、离线后退出，bundle 各 255,701 B。n=1 明确走 `SINGLETON_NO_MATERIAL`。这些 loopback/socketpair 数字是有界功能诊断，不是网络性能数据。

复跑 S4/C++ n=256 common-tape：使用 S4 frozen `select4r.py` SHA-256 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab`，K=2/8 两条中间 trace 均 PASS；最终复跑矩阵位于 `s4_cpp_n256_s17_final2/matrix.json`，SHA=`d0c0675f1be2c185c4b15c3693f38d925d1826eb2b2205d06fde368289d3b539`。第一次脚本调用错误地把 WSL 路径交给 Windows-path 参数，脚本在生成 C++ case 前拒绝；随后使用 Windows 路径重新运行，记录为最终结果。没有以该启动参数错误计算法失败。

与 BMW16 改动无关的入口回归也在同一构建中运行：Protocol I score input、uCMP、parallel shuffle conformance/process E2E、reverse-shuffle model 五项均 exit 0；Protocol III three-process、raw-score pipeline、raw-score three-process、raw-score secure executable、secure combine、metrics record 六项均由 CTest 报告 1/1 passed。两组原始输出分别保存在外部 `protocol_i_regressions_s17.log`、`m3_regressions_s17.log`。这只是所列目标的回归，不是全量 CTest。

## 4. 安全门审查

对 P0/P1 采用相同 view 清单：本方 raw share；forward/inverse shuffle 局部状态及消息；`public_z`；同置换两 task 的真实匿名 handle；9 个 uCMP key 子池的已用/未用槽；由共同 shuffle node masks 导出的相关阈值；本方 Eval share 与已打开 compare bit；四层 adaptive edge/pivot/U/V/W/slot trace；选中匿名 handle；abort、inverse transcript、本方 mask share。proposal leakage 与“不额外泄露真实 score/index/rank/threshold/完整 mask”见 S17 决策文档；公开图/bit 并未被视为用户已批准泄露。

**相关阈值不是自动的反例。** 设 key pool `Q=9·C(n,2)`。若每次 KeyGen tape 独立，且某个一方单-key定理确实对任意与 threshold correlated 的 auxiliary view、party key + eval share、自适应公开 query/bit leakage 安全，则逐 key hybrid 在固定相关 threshold 向量上可走，代价形式为 `Q·ε_one-key + Σ_tid Adv_PRG(Q_tid)`；适应性可以由把已公开 transcript 纳入 auxiliary/leakage 来表达。但仓库没有该 aux-input/output-aware 定理；普通单 key 序列化 key 隐私不说明条件于 public masked operands、本地 shuffle helper、完整未用池及双方 Eval shares/opened bit 的 view。因此首缺口精确在 H1→H2，而不是“阈值相关所以不可能”。

之后 H2→H3 还需要实际 Protocol I forward/inverse shuffle 与该自适应 transcript 的联合模拟器。`seed_fss_once()` 使用每个 OpenMP PRNG 流的 OS 种子，实际 `keyGenDCF()` 从线程 PRNG 取两个 seed blocks 后再运行 AES expansion；当前 counter 不能给所有 KeyGen/shuffle PRG 用量。正确性 conformance、AES-GCM 和文件权限没有关闭联合视图安全证明。

## 5. n=1000 准入与性能结论

checked capacity script 用 `slots/party=2p+2+9·C(n,2)`、key `57+24·b` 及 manifest/raw overhead：n=256 解析下界 312,136,740 B/party；n=1000 下界 4,987,338,528 B/party（4.99 GB），超当前 512 MiB package cap，且 T generator、bundle reader、API 显式限制 n≤256。n=10⁴/10⁵/10⁶ 下界分别 542,225,036,988 B、57,464,659,444,756 B、6,070,495,802,256,900 B。所有容量均是 checked 公式，不是实测。

未来 writer 应独立以新标签实施：T 按规范端点顺序生成一对 uCMP party keys 后立即分方 AEAD 落盘；manifest 绑定 session/party/n/K/width/stage/task/round/端点/id；只在全部槽完成并 durable finalize 后发布；party 在输入前一次性 claim bundle，常数 RAM 随机读取，坏块无 mask abort。raw-score 和 shuffle 材料需要另定随机访问布局。该设计需在 n≤256 做同材料/同随机带差分、重启和坏块验证，并另行重审全池安全/一次性；当前接口不具备。

S17 不运行正式 V4 LAN/WAN 五次矩阵，也不做三方案同源性能拼表。未实测字段统一 `NOT_MEASURED`；本报告保留的微秒/字节/PRG 数仅用于复核功能与代码路径，不具备跨路线或网络比较资格。

## 6. 复跑命令

```powershell
# 配置与 build（Ubuntu-24.04 WSL2）
wsl.exe -d Ubuntu-24.04 -u root -- cmake -S /mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s17/MoE_Top-k_Reproduction_Evaluation/VFSS -B /tmp/bmw16-s17-build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON -DMOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=ON -DMOE_TOPK_ENABLE_EMP_OT=OFF
wsl.exe -d Ubuntu-24.04 -u root -- cmake --build /tmp/bmw16-s17-build --target moe_topk_bmw16_experimental_party_test moe_topk_bmw16_experimental_party_node moe_topk_bmw16_experimental_party_node_e2e_test moe_topk_bmw16_experimental_3proc_test moe_topk_m2_score_input_conformance_test moe_topk_m2_ucmp_conformance_test moe_topk_m2_parallel_shuffle_conformance_test moe_topk_m2_parallel_shuffle_process_e2e_test moe_topk_m2_reverse_shuffle_model_test -j2

# Tests (all raw result files are external to Git)
wsl.exe -d Ubuntu-24.04 -u root -- ctest --test-dir /tmp/bmw16-s17-build --output-on-failure -R bmw16_experimental_party_test
wsl.exe -d Ubuntu-24.04 -u root -- /tmp/bmw16-s17-build/moe_topk_bmw16_experimental_3proc_test
wsl.exe -d Ubuntu-24.04 -u root -- /tmp/bmw16-s17-build/moe_topk_bmw16_experimental_party_node_e2e_test /tmp/bmw16-s17-build/moe_topk_bmw16_experimental_party_node
wsl.exe -d Ubuntu-24.04 -u root -- /tmp/bmw16-s17-build/moe_topk_bmw16_experimental_party_node_e2e_test /tmp/bmw16-s17-build/moe_topk_bmw16_experimental_party_node --extended
wsl.exe -d Ubuntu-24.04 -u root -- /tmp/bmw16-s17-build/moe_topk_m2_score_input_conformance_test
wsl.exe -d Ubuntu-24.04 -u root -- /tmp/bmw16-s17-build/moe_topk_m2_ucmp_conformance_test
wsl.exe -d Ubuntu-24.04 -u root -- /tmp/bmw16-s17-build/moe_topk_m2_parallel_shuffle_conformance_test
wsl.exe -d Ubuntu-24.04 -u root -- /tmp/bmw16-s17-build/moe_topk_m2_parallel_shuffle_process_e2e_test
wsl.exe -d Ubuntu-24.04 -u root -- /tmp/bmw16-s17-build/moe_topk_m2_reverse_shuffle_model_test
wsl.exe -d Ubuntu-24.04 -u root -- ctest --test-dir /tmp/bmw16-s17-build --output-on-failure -R moe_topk_m3_protocol_iii_three_process_e2e_test
wsl.exe -d Ubuntu-24.04 -u root -- ctest --test-dir /tmp/bmw16-s17-build --output-on-failure -R moe_topk_m3_raw_score_pipeline_test
wsl.exe -d Ubuntu-24.04 -u root -- ctest --test-dir /tmp/bmw16-s17-build --output-on-failure -R moe_topk_m3_raw_score_three_process_e2e_test
wsl.exe -d Ubuntu-24.04 -u root -- ctest --test-dir /tmp/bmw16-s17-build --output-on-failure -R moe_topk_m3_raw_score_secure_executable_test
wsl.exe -d Ubuntu-24.04 -u root -- ctest --test-dir /tmp/bmw16-s17-build --output-on-failure -R moe_topk_m3_secure_combine_test
wsl.exe -d Ubuntu-24.04 -u root -- ctest --test-dir /tmp/bmw16-s17-build --output-on-failure -R moe_topk_m3_metrics_record_test

# Exact frozen S4 common-tape differential; invoke from Windows Python
python experiments/TEST_ONLY_BMW16_S15/run_n256_common_tape.py --cpp-test /tmp/bmw16-s17-build/moe_topk_bmw16_experimental_party_test --s4 C:\Users\28641\.codex\artifacts\BMW16_S17_20261007\select4r_frozen_s4.py --out C:\Users\28641\.codex\artifacts\BMW16_S17_20261007\s4_cpp_n256_s17_verified2
python experiments/TEST_ONLY_BMW16_S15/capacity_preflight.py
```

## 7. 门禁与限制

| Gate | 结果 | 适用范围 |
|---|---|---|
| `FUNCTIONAL` | `PASS_FOR_REBUILT_TESTED_DOMAIN` | 61-case 3proc matrix; party-node T/P0/P1 E2E n=1/2/5/8/64/128/256; n=256 S4 common-tape K=2/8; listed Protocol I/III regressions. |
| `CONDITIONAL_SECURITY` | `NO_GO` | H1→H2 aux-input adaptive full-pool DCF game missing; shuffle/inverse combination unproved. |
| `PRIVATE_DELIVERY` | `EXPERIMENTAL_LOCAL_ONLY` | Local credentials/owner directories; no remote authenticated transport. |
| `ONE_SHOT_SCOPE` | `CONDITIONAL_FIXED_ROOT_ONLY` | requires protected nonrollbackable claim-root operations. |
| `STREAM_STORE` | `PASS_LAZY_SELECT_MEMBERSHIP_ONLY` | Exact slot bytes and bounded oracle path; no streaming KeyGen. |
| `N=1000` | `PRECHECK_REJECTED / NOT_RUN` | 4.99 GB/party lower bound >512 MiB cap, source/API n≤256. |
| `PERFORMANCE_READY` | `NO_GO` | No V4 network matrix; no same-revision all-pairs/I+AAV86 comparison. |
| `PRODUCTION_SECURE_ALIAS` | `NOT_CREATED` | EXPERIMENTAL option remains default OFF. |

## 8. 修改文件清单

实现：

- `VFSS/CMakeLists.txt`
- `VFSS/include/moe_topk/experimental_bmw16_material_bundle.h`
- `VFSS/include/moe_topk/experimental_bmw16_select_party.h`
- `VFSS/include/moe_topk/experimental_bmw16_stream_store.h`（新增）
- `VFSS/src/apps/moe_topk_bmw16_experimental_party_node.cpp`（新增）
- `VFSS/src/moe_topk/experimental_bmw16_material_bundle.cpp`
- `VFSS/src/moe_topk/experimental_bmw16_select_party.cpp`
- `VFSS/src/moe_topk/experimental_bmw16_stream_store.cpp`（新增）
- `VFSS/tests/moe_topk/experimental_bmw16_3proc_test.cpp`
- `VFSS/tests/moe_topk/experimental_bmw16_party_test.cpp`
- `VFSS/tests/moe_topk/experimental_bmw16_party_node_e2e_test.cpp`（新增）

决策与证据：

- `docs/decisions/BMW16_S17_INDEPENDENT_COMPOSITION_SECURITY_AND_STREAM_STORE_2026-10-07.md`
- `docs/reproduction/BMW16_S17_INDEPENDENT_ACCEPTANCE_AND_STREAM_STORE_2026-10-07.md`
- `docs/reproduction/evidence/BMW16_S17_EXTERNAL_EVIDENCE_INDEX_2026-10-07.md`
- `docs/IMPLEMENTATION_PLAN.md`
- `docs/BENCHMARK_VALIDATION_PLAN.md`
