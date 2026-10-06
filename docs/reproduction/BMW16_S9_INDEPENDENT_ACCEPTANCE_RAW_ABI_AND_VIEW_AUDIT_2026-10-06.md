# BMW16 S9 独立接收、raw-score differential 与隐私边界报告

- 日期：2026-10-06
- 隔离分支：`codex/m6b-i-bmw16-s9`
- 初始 HEAD：`0ac6d5622fc60847f119672456d785ef4700e180`（S8）
- 标签：`Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL_TEST_ONLY`
- 报告结论：**Select 端到端实验真正运行；raw-score 移植实验与冻结 oracle 一致；OS 同身份文件攻击可复现；不创建 secure 入口，party-view/私有离线发材仍 NO-GO。**

## 1. 本轮实际做了什么

本轮从 S8 冻结提交建立单独 worktree 和 `codex/` 分支，复查 S8 源码、报告、实验 trace、正式 Protocol I shuffle/uCMP/score-input 接口及 E20 发材合同。随后在同一 TEST_ONLY 路径上增加：

- 从正式 `uint32_t` signed-Q20.12 加法 shares 走 carry/sign lift 的实验端口，输出与 S8 Select 输入同序兼容的 stable key shares；
- 两方各提供一个 OS CSPRNG contribution、XOR 与域分离、HMAC-SHA256 counter stream、rejection sampler 和无放回 Fisher–Yates 的 TEST_ONLY algorithm-randomness 模式；
- 强制 probability-abort 夹具、party 子进程尝试读取 sibling share/package 的 OS-access probe；
- 独立边清单/阶段计数复算器对 raw-score 与 joint-random 增量的支持；
- 两世界相同腐化方 shares 的可区分事件夹具。

S4 Select 源仅复用 S8/S7 带来的唯一 `select4r_s4_source.py`；未复制第二份算法。没有写 `VFSS/` runtime 或 secure API；只扩展既有 raw-score conformance test 以调用现有适配器。没有更换旧 Protocol I/AAV86 入口，没有调用在线 Dealer，没有改动 `VFSS-baseline/`、E20/E21、论文、参考树、PR #28 或桌面主工作区差异。S9 没有继承 BMW16 Theorem 8 或 BB90 的失败概率/比较常数。

## 2. 起点、来源和工作区保护

| 项 | 本轮核对结果 |
|---|---|
| `main` / `origin/main` | `c3926c68fd14f270faa8b55234311071947fa080`（S8 分支共同祖先） |
| 用户指定接收起点 | S8 `0ac6d5622fc60847f119672456d785ef4700e180`；S9 worktree 初始 HEAD 正是该 revision |
| S9 工作路径 | `C:\Users\28641\.codex\worktrees\m6b-i-bmw16-s9\MoE_Top-k_Reproduction_Evaluation` |
| S9 分支 | `codex/m6b-i-bmw16-s9` |
| S4 原始源 | `04ed6f8a352ab2277434344094471dff82f83e4b:experiments/TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R/select4r.py`；SHA-256 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab` |
| S7/S8 复用副本 | Git blob SHA-256 `d9a4bba123cfdccf01b9ff3451c6622b41d63bcf90cf1709c66a3af7c4fcd3bb`；本 S9 Windows worktree checkout file-byte SHA-256 `39757567f0107be82859e1123d06c370f9f2e057e1d175242ae2cfbcae06476c`（CRLF checkout） |
| S5 / S6 / S7 | `67c3f302a8564b68616f7d7247c8cee3415ace31` / `c4f34e7e8f9fd954637573fe62b8d9b567362298` / `0dbfc8d99b3e896329611793ef95ea822e241a67` |
| E20 | `8986a40175f5f8eef56d765fef0cfa863e205985`，代码身份 `a8f0ac47f9b3ebc4276d1aa0445f178acc1ef56f` |
| 2016 BMW16 PDF | `Papers/1603.04941v1.pdf`，44 页，SHA-256 `F46F83CBA279F37E9E3AAA0D64B145FDFCFBC52C0C9C8B79BACEC1259CA9CA23` |
| 辅助 PDF | `Papers/017.pdf`，249 页，SHA-256 `3EAE3A93F12AB71EEA8C040167F21351FCF3FF54DC423985A04087E3E90CB15`；与 `docs/PAPERS.sha256`、桌面只读 PowerShell 和 certutil 输出一致 |

S8 报告把辅助 PDF hash 写成 `...71EE8C040...`；S9 将此记录差异订正为实测/manifest 的 `...71EEA8C040...`。文件保持只读且未进入 Git。fitz 复核 PDF 页数分别为44、249。

S4 源也保留三层来源标识：原文件 Git object (`8644...`)、S7/S8 复用副本 Git blob (`d9a4...`)、S9 Windows checkout 的 CRLF 文件字节 (`3975...`)。S9 实际 import 后者；`git show S8:path` 与 `git show S7:path` 的 blob hash 相同，S9 未改动该源文件。

论文文件由既有 S8 来源登记核对；PDF 未放入 S9 worktree/Git。E20 的 AAV86 一次性领取和私有渠道只作为工程边界参考，没有把其安全接收移植到本项目。主工作区已存在的文档差异、其他 worktree 和证据目录均保持原样。

## 3. S8 独立接收结果

在 S9 worktree 中从 S8 源码重新编译 `bmw16_s8_backend`，再跑 S8 原始 `TEST_ONLY` 路径。T 后端先生成双方完整候选 key pool 并退出，随后 runner 创建 input share 文件并启动 P0/P1；trace 标注 `offline_T_exited_before_parties=true`、`online_keygen=false`。这个顺序是代码运行事实，不是 OS 身份隔离证明。

| 独立复跑 | 输入 | 结果 | 证据等级 |
|---|---|---|---|
| S8 固定矩阵 | n=1,2,3,5,8；极值/重复/全相等；K=1、中间值、K=n；11 cases | 11/11 `SUCCESS` 且 frozen C++ oracle PASS | 本 worktree 真正重跑 |
| S8 采样路径 | n=64,K=2，fixture `n64_seed20261006.json` | `SUCCESS` / oracle PASS；实走非全量 R1 样本 | 本 worktree 真正重跑 |
| 边/阶段因果复算 | 上述 11 cases+n64，共 12 | 独立 auditor 12/12 PASS | 不导入 Select 实现的独立 trace auditor |
| 匿名图枚举 | n=2..5 的全部 `n!` handle permutations、反向输入序、共同两任务 handle、两个 seed、R1–R4/结果/selected handle/abort | 所枚举 transcript multiset 一致 | 仅有限公开 transcript 枚举，不含真实完整 party state/pool |

此复跑确认为实验功能证据，不等于 `PARTY_VIEW_SECURITY`。S8 中旧 S7 失败记录继续保留；S9 不以本轮 success 样本覆盖历史失败。

## 4. raw-score ABI 和冻结 oracle differential

正式 ABI 是 `raw_share[i]` 的 32 位 unsigned word，满足加法 shares 在 `Z/(2^32)` 的和，按 signed Q20.12 解释。正式转换的两个 uCMP 阶段是 carry 和 sign，随后构造原 index priority key。S9 的隔离 C++ worker/Python Select 组合把这些等式**移植**后接入；它没有把正式 `protocol_i_raw_score_input_party` runtime 接入 Select 组合。为单独完成 ABI conformance，本轮另扩展官方 adapter 测试目标并真实调用该函数，详见下文。

独立构建并运行仓库现有 `moe_topk_m2_score_input_conformance_test`，退出码 0。测试仍覆盖 `n=1,2,3,5,17,31,128`、INT32 边界编码、priority-key 布局、party package 错方和尾随字节拒绝；本轮再增加真实正式 `protocol_i_raw_score_input_party` 的两方 socketpair conformance，对 n=1/2/3/5/8、signed 边界和重复值实际输出的 additive key shares 与 `protocol_i_priority_key` 对照。此测试以测试专用固定 FSS seed 在同进程两方线程运行；不等于安全多进程协议。随后 S9 移植实验以统一 `top_k_mask` 冻结 C++ oracle 为参照。

测试第一次接入时失败并打印 `PRNG has not been keyed`：测试用例在调用 raw adapter 前尚未设置 DCF generator seed。修正为在 conformance harness 中显式调用固定 TEST_ONLY `seed()` 后，目标 build、直接执行和 `ctest` 均通过。该失败用于检查测试初始化；它不是生产 runtime 失败种子。另对隔离 backend 做了源码顺序检查：其 keygen 前调用的官方 shuffle dealer generator 使用 OS `getrandom` 对 256 条 FSS PRNG 流播种。

| 用例组 | 实际覆盖 | 结果 |
|---|---|---|
| raw-score 固定矩阵 | 11 cases：n=1/2/3/5/8，K 两端和中间，INT32_MIN/MAX、全相等、重复值、正负值 | 11/11 SUCCESS + oracle PASS |
| signed 极值固定 fixture | n=5，`[INT32_MAX, INT32_MIN, 0, -1, INT32_MAX]`，K=3 | SUCCESS + oracle PASS；比较位宽36，T package 119,630 bytes/party |
| 两方 CSPRNG 合币 | n=5,K=3；n=64,K=2，seed contributors 来自 `secrets.token_bytes(32)` | 均 SUCCESS + oracle PASS；分别 16/16 message phases |
| n=64 | fixed n64 score fixture、K=2、S4 非全量抽样分支 | SUCCESS + oracle PASS；Select 比较宽39 |
| 算法 abort 输出夹具 | n=5,K=3，选择后注入 abort | 双方 `ABORT_ALGORITHM_PROBABILITY`，退出码 `[2,2]`，no mask=true；oracle `NOT_RUN_ABORT` |

这 11 个矩阵样本、固定 seed n=5 边界 run、joint-CSPRNG n=5 和 joint-CSPRNG n=64 共 14 个 raw-score 成功输入；自然算法失败观察为 0。注入 abort 不算随机种子失败样本，也不估计随机失败率。该项目推导算法没有已证明的 `≤10^-6` 上界；0/14 只是有限样本观察。

### 4.1 S9 n=64 逐轮和材料实际账本

| 轮/阶段 | 逻辑比较调用 | 真实—真实 | dummy 相关 | 实际 Select uCMP key slots/party |
|---|---:|---:|---:|---:|
| R1 | 8,010 | 2,118 | 5,892 | 2,118 |
| R2 | 500 | 126 | 374 | 126 |
| R3 | 11,362 | 3,104 | 8,258 | 3,104 |
| R4 | 2 | 2 | 0 | 2 |
| **Select total** | **19,874** | **5,350** | **14,524** | **5,350** |
| membership | n−1=63 | 63 | 0 | 63 |
| raw carry + sign input | 两阶段 × padded n=64 comparator requests | 128 slot | — | 128 |

两实例共享真实 handle。实际离线 pool/party：Select `8·C(64,2)=16,128`，membership `C(64,2)=2,016`，forward/inverse shuffle 中未用 CmpAgg `2·C(64,2)=4,032`，raw lift 两阶段各覆盖 padded `P=64` 个槽，所以 `2P=128` 个 raw uCMP slots。总槽为 `16,128+2,016+4,032+128=22,304`/party。表内 raw 行计两阶段合计，而非每阶段 128。

实际 package 为 22,723,604 bytes/party。每个 uCMP slot 中每方 2 次 DCF Eval，两方总 4 次：Select+membership `4·(5,350+63)=21,652` 两方 DCF Eval；raw 输入 `4·128=512`；合计22,164。`uCMP_calls_per_party=5,413` 只计在线 Select+membership 的实际槽消费，不含 raw lift；raw 另列。S9 样本实际无跨轮 FSS 槽复用；逻辑边、唯一规范材料槽、重复逻辑请求、底层 DCF Eval 不能互相替代。

同一次 loopback n=64 run 的应用层 payload 计数：P0 发/收 `772,462 / 772,654` bytes，P1 发/收 `772,654 / 772,462` bytes；party 进程经过时间约 `1.2252445 / 1.2214382` 秒（诊断计时，非性能结果）。该计数包含 harness framing，不是物理网络字节。T 离线耗时、阶段分解、生产链路 RTT/带宽、总正式时间均为 `NOT_MEASURED`。

这些是单次 n64 trace 与离线文件实测，不证明 worst-case `O(n)`，不代表材料/T 生成时间，也不与 BB90 常数比较。包实际保留未用 CmpAgg keys，因此字节统计没有漏掉其费用。正式生产路径是否能按该边界预发和认证整个自适应 pool 仍未证明。

## 5. 随机币、selected handle 和输出

固定 seed 模式用于复现，不是安全随机。joint mode 中两方 `secrets.token_bytes(32)`，同轮交换贡献，XOR 后 SHA-256 域分离形成采样种子；HMAC-SHA256 counter DRBG 提供域分离流，rejection `randbelow` 无模偏差，无放回采样。半诚实且至少一方按协议提供均匀 CSPRNG 贡献时，合币种子为均匀；无 commit/reveal 时不防恶意选择/中止偏置，TCP loopback 也没有认证。实际抽样 PRF 的计算安全假设与算法的概率错误事件分开；没有用 BMW16 Theorem 8 转写抽样中止率。

successful runner 将 selected anonymous handle 公开给双方，并用 `n−1` 星形 membership uCMP 得到每条记录是否 `R_i≥R_selected`，对自比较本地置1。该 uCMP 实现由每个 key 的两次 DCF Eval 支撑；本报告称 “uCMP / DCF Eval”，不把它误标成每条成员的单次直接 DCF。两阶段 inverse shuffle 在同一个隐藏 π 下返回原输入位置 mask shares。party 只输出自身份额；只有外部 TEST_ONLY harness 重构完整 mask 并比较冻结 C++ oracle。

算法四层 R1–R4不是端到端网络轮数。对 n≥2，S8 功能 DAG loopback 为13 phases；S9 raw lift 另加两次 carry/sign message phase，joint coin 再加一 phase，因此 raw+n64+joint run 为16 phases。包括输入转换、前置 shuffle、Select、membership、逆 shuffle、final status；没有将 Select 四层偷换为整个入口四轮。offline/online 时间、WAN/LAN、生产 T→party bytes、PRG、正式失败率均为 `NOT_MEASURED`。

逐阶段因果 DAG 与相位计数（phase 指一次双方 `Peer.exchange` request/response；不是单向 socket write）：

```text
T: input-independent shuffle + complete uCMP/DCF pools + raw carry/sign pools
   └─ writes P0/P1 experiment packages and exits (before online input-share files)
P0/P1 local raw-score shares
   ├─ optional joint random coin exchange                         1 phase
   ├─ raw carry masked operands exchange → local DCF/uCMP eval     1 phase
   ├─ raw sign masked operands exchange → local DCF/uCMP eval      1 phase
   ├─ forward shuffle FWD1 exchange → FWD2 exchange                2 phases
   ├─ Select R1 freeze/compare result exchange                     2 phases
   ├─ Select R2 depends on R1; compare result exchange              2 phases
   ├─ Select R3 depends on R2; compare result exchange              2 phases
   ├─ Select R4 depends on R3; compare result exchange              2 phases
   ├─ selected anonymous handle public; n−1 membership evals       0 phases
   ├─ inverse shuffle INV1 exchange → INV2 exchange                2 phases
   └─ agreed final status / handle exchange                         1 phase
                                                                  ─────────
raw + joint total                                                  16 phases
```

若不使用 joint coin，raw path 为15 phases；不使用 raw lift 与 joint coin 的 S8 direct path 为13 phases；n=1 是单例 shortcut，仅一次 final status exchange。此实现调用 forward/inverse shuffle 的两轮测试子协议，不是 Protocol I 完整 CmpAgg/reorder secure entry；shuffle package 中的 CmpAgg keys 实际生成但未 eval，已经纳入全池 bytes/slots。每个 Select comparison layer 含两次 peer exchange，所以“4 个 Select 依赖层”对应8个 phase；不能将 16 phases 报作“安全协议四轮”。

## 6. 单方 view 审查与具体阻塞

### 6.1 同一 OS token 反例

S9 运行真实 P0/P1 party 进程的 probe flag：在同一 Windows/WSL OS token、同一个临时目录中，P0/P1 均可读 sibling input share 和对方离线 package。n=5 raw mode 中每方确认可从 peer share 文件重构 n 个 signed int32 值；只向报告输出布尔标志和计数，不打印清晰 scores。两世界 n=2 witness 固定 P0 share 字节、交换原 score 顺序 `[0,1]`/`[1,0]`；P0 读取另一 share 后以概率1区分。详细 witness 见 [BMW16_S9_TWO_WORLD_WITNESS.json](evidence/BMW16_S9_TWO_WORLD_WITNESS.json)。

此结果足以判当前同主机/同身份运行路径不满足单方私密输入/材料要求。它不是远端攻击结论；不同 OS 主体或主机的私有传输可移除该直接攻击，但需要另外证明协议视图。

### 6.2 远端独立身份仍缺的引理

将同身份攻击拿掉后，仍缺 `BMW16-S9-JOINT-SHUFFLE-ADAPTIVE-FSS-POOL` 单方 hybrid：需联合覆盖 local shuffle input/state/messages、共同真实匿名 handle、同节点 mask 派生的整套 party DCF key、公开自适应边/比较 bit/selected handle、已/未领取槽、随机 coin、abort 及逆路由 output share。S8 n≤5 的枚举只包含 public graph transcript；不触及 pool/key correlation。S9 没有依据“单 DCF 安全”推出全池自适应安全，也没有调用 ORAM/隐藏访问假装开销免费。

### 6.3 一致 abort 边界

选择后强制注入的概率失败两方均回报相同 abort 类别且无 mask。错误 party package、截断、错误 round/endpoint、重复 key slot、重复领取和单方 peer silence 夹具保留在 runner；材料错误双端共识仅是当前实验 loopback 协议行为。peer 已静默/断开时没有可达 ACK，不能声称双方都确认中止；没有对所有 transport/auth/partial-package 错误做生产可靠的 abort 协议。材料和通信状态仍可能通过错误类型分支影响公开 abort view，需要并入上述模拟。

具体 package fixture 结果：wrong-party package、截断包、错 R1/R2 round、端点越界均被本地拒绝；一个已连接 party 收到错方包时，两端结果为 `ABORT_MATERIAL`、双方 `no_mask=true`；同一材料再次领取与重复 eval 都失败。peer silent 测试只确认存活 P0 返回 `ABORT_COMMUNICATION/no_mask` 并退出，不存在 P1 ack。joint raw+select 的强制 abort 测试是在 Select 执行结束后注入，双方 final `ABORT_ALGORITHM_PROBABILITY` 一致、`selected_handle=null` 且无 mask；它没有触发自然采样失败。

## 7. 实际命令、环境与证据

环境：Windows 11 host（PowerShell）+ WSL Ubuntu 24.04.4。Python runner 使用 Windows Python 3.13.7 + NumPy 2.3.5；C++ build/test 使用 WSL Python 3.12.3、GCC 13.3.0、CMake 3.28.3。S9 backend 编译目录 `/tmp/m6b_i_bmw16_s9_build`；raw-score conformance 编译目录 `/tmp/m6b_i_bmw16_s9_vfss_build`，二者都不在仓库。

```powershell
wsl.exe -d Ubuntu-24.04 -- cmake -S '/mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s9/MoE_Top-k_Reproduction_Evaluation/VFSS' -B /tmp/m6b_i_bmw16_s9_vfss_build -DCMAKE_BUILD_TYPE=Release
wsl.exe -d Ubuntu-24.04 -- cmake --build /tmp/m6b_i_bmw16_s9_vfss_build --target moe_topk_m2_score_input_conformance_test -j2
wsl.exe -d Ubuntu-24.04 -- /tmp/m6b_i_bmw16_s9_vfss_build/moe_topk_m2_score_input_conformance_test
wsl.exe -d Ubuntu-24.04 -- ctest --test-dir /tmp/m6b_i_bmw16_s9_vfss_build -R moe_topk_m2_score_input_conformance_test --output-on-failure

python experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/run_s8.py --backend 'wsl:/tmp/m6b_i_bmw16_s9_build/bmw16_s8_backend' --matrix --output "$env:TEMP\BMW16_S9_INDEPENDENT_MATRIX.json"
python experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/run_s8.py --backend 'wsl:/tmp/m6b_i_bmw16_s9_build/bmw16_s8_backend' --scores-json experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/vectors/n64_seed20261006.json --k 2 --case-id n64_k2_seed2026 --input-seed 67 --algorithm-seed 101 --output "$env:TEMP\BMW16_S9_N64_K2.json"
python experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/audit_party_transcripts.py "$env:TEMP\BMW16_S9_INDEPENDENT_MATRIX.json" "$env:TEMP\BMW16_S9_N64_K2.json" --output "$env:TEMP\BMW16_S9_EDGE_AUDIT_RECHECK.json"

python experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/run_s8.py --backend 'wsl:/tmp/m6b_i_bmw16_s9_build/bmw16_s8_backend' --matrix --raw-score-abi --output "$env:TEMP\BMW16_S9_RAW_SCORE_MATRIX.json"
python experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/run_s8.py --backend 'wsl:/tmp/m6b_i_bmw16_s9_build/bmw16_s8_backend' --scores-json experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/vectors/s9_raw_abi_n5.json --k 3 --case-id s9_raw_abi_n5_signed_edges --input-seed 109 --algorithm-seed 211 --raw-score-abi --joint-csprng-coins --output "$env:TEMP\BMW16_S9_RAW_ABI_N5_JOINT.json"
python experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/run_s8.py --backend 'wsl:/tmp/m6b_i_bmw16_s9_build/bmw16_s8_backend' --scores-json experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/vectors/n64_seed20261006.json --k 2 --case-id s9_raw_n64_k2_joint_csprng --input-seed 67 --raw-score-abi --joint-csprng-coins --output "$env:TEMP\BMW16_S9_RAW_N64_K2_JOINT.json"
python experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/run_s8.py --backend 'wsl:/tmp/m6b_i_bmw16_s9_build/bmw16_s8_backend' --scores-json experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/vectors/s9_raw_abi_n5.json --k 3 --case-id s9_force_abort_and_same_uid_probe --input-seed 109 --algorithm-seed 211 --raw-score-abi --test-only-force-algorithm-abort --test-only-probe-peer-files --output "$env:TEMP\BMW16_S9_FORCE_ABORT_AND_FILE_PROBE.json"
python experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/audit_party_transcripts.py "$env:TEMP\BMW16_S9_RAW_SCORE_MATRIX.json" "$env:TEMP\BMW16_S9_RAW_N64_K2_JOINT.json" --output "$env:TEMP\BMW16_S9_RAW_EDGE_AUDIT.json"
python experiments/EXPERIMENTAL_TEST_ONLY_BMW16_S8_THREE_PROCESS/audit_os_shared_worlds.py > "$env:TEMP\BMW16_S9_TWO_WORLD_WITNESS.json"
```

Earlier bring-up failures were retained in the S9 run summary: first raw matrix run exposed a list/string parsing mismatch in the harness, fixed before the accepted rerun；独立 auditor 的首次 phase equation 不含 S9 两个 raw 和一个 joint-coin phase，修正后 raw/direct trace 均 12/12 PASS；backend 在接线初期的编译错误均修正并最终成功构建。这些是 harness/audit/build 修复，不是被删掉的 algorithm failure seed。S9 正式抽样矩阵中自然 `PAPER_RANDOM_FAILURE_PATH` 观察为 0；forced abort 另行记录。

生成的 full run JSON 在本地 `$env:TEMP`，不作为原始大日志提交；提交的结构化证据只保存摘要、哈希、结论及合成两世界 witness。backend 二进制 SHA-256 为 `0989efa4b2458267f330237ef0027a87812356473b028de64c2c6a14ca3bc3cf`；本轮扩展后的正式 raw input conformance test 二进制 SHA-256 在最终 `BMW16_S9_SHA256SUMS.txt` 中记录。对应 run/evidence hashes 列在该 manifest。

## 8. 门禁结论

| 门禁 | 结论 | 证据/限制 |
|---|---|---|
| `S8_INDEPENDENT_ACCEPTANCE` | `PASS_EXPERIMENTAL` | S8 matrix 11/11 + n64 1/1 + edge audit 12/12 |
| `RAW_SCORE_ABI` | `PASS_FORMAL_ADAPTER_CONFORMANCE_AND_EXPERIMENTAL_FUNCTIONAL; SECURE_COMPOSITION_NOT_ACCEPTED` | 正式 adapter 真实 socketpair conformance PASS；移植路径 raw differential PASS；不接 Select 生产/secure runtime |
| `OFFLINE_PRIVATE_MATERIAL` | `NO_GO` | 当前同身份可读对方文件，且无 E20 类私有 AEAD 全量交付 |
| `ADAPTIVE_TRANSCRIPT_VIEW` | `FINITE_PUBLIC_VIEW_PASS; JOINT_VIEW_NO_GO` | 公开 transcript n≤5 枚举与置换等变，不覆盖 state/pool |
| `FULL_POOL_KEY_VIEW` | `NO_GO` | shared node masks 相关的 full adaptive key-pool hybrid 未给出 |
| `SAME_PI_INVERSE_MASK` | `PASS_EXPERIMENTAL_FUNCTIONAL` | 成功差分都原序恰 K；验证器是唯一重构位置 |
| `AGREED_ABORT` | `PASS_TEST_ONLY_INJECTED; PRODUCTION_NOT_ACCEPTED` | 强制算法 abort 双方同类、无 mask；peer disconnect 无双方 ACK 保证 |
| `SMALL_N_SECURE_RUNTIME` | `NOT_IMPLEMENTED / NO_GO` | 不符合创建 secure 入口条件，`VFSS/` 无 S9 差异 |
| `PERFORMANCE_READY` | `NO` | 非安全 loopback 测试，正式 LAN/WAN、offline T、PRG、正式 latency 均未测 |

具体下一步不是改 Select 概率结论，而是修正执行/发材威胁面并完成缺失 hybrid：分离 party OS 身份或主机；实现 T→P 私有完整 AEAD 分发与 package 数学绑定；重新跑 file-access/T-exit/replay 篡改/单次领取；然后证明整池 uCMP/DCF key、shuffle state、adaptive graph 和 output shares 的一个腐化方 joint simulation。否则保留为实验候选，不允许进入 secure API 或 V4 正式性能矩阵。
