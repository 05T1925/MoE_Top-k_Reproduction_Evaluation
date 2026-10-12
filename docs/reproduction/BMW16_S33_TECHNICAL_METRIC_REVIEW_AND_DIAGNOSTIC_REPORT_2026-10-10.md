# M6B-I-BMW16-S33：S32 计量技术复核与三路线配对诊断报告

日期：2026-10-10

结论范围：复核 S32 计量实现；冻结 Protocol I 三路线 schema-v3 九指标合同；完成小规模本机配对诊断和服务器前准入检查。S33 的实施者也参与本次接收审查，因此 S33/S34 的结论均称为**技术复核**，不称为独立接收。**本报告不表示正式 V4 完成，也不扩大 S31 安全主张。**

## 1. 阶段结论

| 阶段门 | 结论 | 证据范围 |
| --- | --- | --- |
| S26 独立功能签收 | **PASS（历史接收）** | 独立接收报告的 `DRAFT_PR_READY=PASS` 仅覆盖默认关闭的实验功能候选，不代表安全接收或正式性能通过 |
| S31 `conditional_secure_v2` 条件安全接收 | **CONDITIONAL_ACCEPTED（沿用）** | S28–S31 假设、泄露函数 L 和 n≤1000 范围维持原文；S33 未重开安全证明 |
| S32 计量源码技术复核 | **PASS** | S31→S32 runtime 差异逐文件审阅；计数点和审计恒等式复核 |
| S32 历史 schema-v2 记录 | **PASS，4/4** | 调用原 S32 auditor；记录和 manifest SHA-256 与 S32 报告逐一相符 |
| 计数器 ON/OFF 固定 tape 等价 | **PASS** | 新 S33 TEST_ONLY 对照：3 次成功 + forced-abort；观察到 +15.29% 计时开销，仅本机小样本诊断 |
| schema-v3 及本地三路线诊断/审计 | **PASS（选定最终记录）** | 四个最终配对 campaign 均通过；S34 又核验了 22 个历史 campaign 的 schema/auditor 状态，并加强身份与 fresh-session 检查 |
| conformance → oracle differential → 独立进程 E2E | **PASS** | v2 conformance 1/1，冻结 oracle party test 1/1，之后三路线配对 spot-check 3/3；E16 AAV86 conformance+differential 2/2 |
| 配对诊断矩阵 n=128/256、K=2/8 | **PASS（诊断）** | 4 配置 × 3 路线，12/12 SUCCESS；不作为 V4 正式数据或排名 |
| n=1000/K=80 三路线准入 | **A/B PRECHECK_REJECTED；C RESOURCE_REVIEW_REQUIRED / NOT_RUN** | all-pairs 与 AAV86 在协议启动前拒绝；C 满足 conditional_secure_v2 参数界，但当前主机未通过资源/网络准入 |
| 正式 LAN/WAN V4 | **READY/NOT_RUN** | V3 未关闭；未运行正式服务器矩阵 |
| PR #29 的 S32 内容已推送 | **YES** | 当前 PR head 为 `32cd7952cccdacc5a1a5b4fcca245f7acf594049`；S33 仍未提交/未推送 |
| PR #29 reviewer approval | **NO / NOT_APPROVED** | 页面只列出 Copilot review 请求；评论明确因 requester quota limit 未完成 review，没有批准结论 |
| PR #29 已合入 main | **NO / NOT_MERGED** | PR 状态仍为 Open；`main` 未包含该 head |

全仓 CTest **未运行**。以上测试是本次相关的定向 CTest、显式 E2E 和逐次记录审计，不代表全仓测试通过。

## 2. 开工身份、工作区与来源哈希

### 2.1 Git 状态

- S33 主 worktree：`C:\Users\28641\.codex\worktrees\m6b-i-bmw16-s33-metrics\MoE_Top-k_Reproduction_Evaluation`，分支 `codex/m6b-i-bmw16-s33-metrics`。开始时工作树 clean，HEAD=`32cd7952cccdacc5a1a5b4fcca245f7acf594049`；merge-base=`c3926c68fd14f270faa8b55234311071947fa080`。
- local `main`=`origin/main`=`c3926c68fd14f270faa8b55234311071947fa080`。S33 主 worktree 没有覆盖桌面主 checkout。
- 桌面主 checkout 在开工前已有 dirty/untracked 文件；本轮未在该工作区写入。结束时仍保留原状态：`PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md`、`docs/PAPERS.sha256`、`docs/REFERENCE_MANIFEST.md` 有修改；三份 M6A 文档和 `siamjdiscrmath.pdf` 为未跟踪文件。
- 另一个 E16 bridge worktree：`C:\Users\28641\.codex\worktrees\m6a-e16-s33-diagnostic-bridge\MoE_Top-k_Reproduction_Evaluation`，HEAD=`267cb6924f061d376dbce7986fc0093b52ec2512`；只改两个 TEST_ONLY benchmark harness 文件，未改其 runtime/secure 实现。
- S31 接收提交：`a1a90cb2a568deb5e5078ddec10136ce9a743411`。S32 被测 runtime：`f2db098578b1cc096298ee39d990ff668d8dd6f7`。S32 最终报告提交及 S33 开工 HEAD：`32cd7952cccdacc5a1a5b4fcca245f7acf594049`。
- 远端 `refs/pull/29/head` 与 `refs/heads/codex/m6b-i-bmw16-s26` 均为 `32cd7952cccdacc5a1a5b4fcca245f7acf594049`；`refs/heads/main` 为 `c3926c68fd14f270faa8b55234311071947fa080`。`refs/pull/29/merge` 的 `6f8e8d3d091211b738e6d273ca6609b9cec85d9c` 是 GitHub 生成的 merge ref，不表示已合并。远端 `codex/m6b-i-bmw16-s26-independent-draft`=`32f7f0ba3f80083ff282c9b5ebd0a827bd366898`，与 PR #29 head 不同；浏览器 compare URL 中的这个分支不能代替 PR #29 当前 head。

### 2.2 审阅来源 SHA-256

| 来源 | SHA-256 |
| --- | --- |
| `AGENTS.md` | `a260f872d0a9058fe9f0b2b94dc78c06272dcbba7c0d28488cccee7bf34897aa` |
| S31 independent acceptance report | `59acd6ca730375f07489d557e2379afa92fdf88144fb1eebab8c95d6b4b3f814` |
| S31 server benchmark package/runbook | `af1f8259d232edf08d2d9a87a2e956017a955e6dc67c32387917d5d6924091c4` |
| S31 conditional-v2 decision | `45e2953febc8144c8dd4df533bb2c83c70770c9413ba0e2b92ce209f2e209030` |
| S32 decision | `ef5f303f8381b98a249482d70162fdcedd5ad387d2b4a8d8ae63594e8458f8ee` |
| S32 final instrumentation/report | `331fd95e912b2af75142cf545d23cb94cb0611c128199ea3f43324d690155695` |
| S32 schema-v2 | `d36172fd1628815b711722d589f043e7118d98e83aa759abc4efe9636bf9332f` |
| S33 schema-v3 | `ad342ee442857ed0677dd4e13cd16af210e5ebf2dd950896e37e217e3d6e923a` |
| S33 unified collector/auditor final source | `c920b635ec2884b624aeceb7ab95a82ab133a4345b6dbe3d5b9544a7da69c7df` |
| S33 BMW16 TEST_ONLY E2E harness | `c775146047ba39fb53cb5d06e92ac22ae0b87a50704553557f693debcefe67c6` |
| S26 independent functional receiver report | `554F421B7CD7061DCEC7F9E5B27B1863E551C00463F6227FB6CB4B220295C09D` |

S31/S32 source materials were read from the starting S33 revision, not rewritten. S32 source history remains: `a1a90cb…` → runtime `f2db098…` → report `32cd795…`.

## 3. S32 独立技术接收

### 3.1 变更差异和安全/功能影响

`git diff a1a90cb2… f2db0985…` 涉及 8 个文件，约 957 insertions / 4 deletions：CMake 计数开关、BMW16 metrics header、T/party-node diagnostics、Select adapter 计数、run-record collector/auditor、测试 harness 和 schema-v2。没有修改 DCF/Select secure 算法、消息内容/依赖、一次性材料状态、输入/输出语义或安全证明。新增 runtime instrumentation 在 T KeyGen 和在线调用前后 reset/read DCF counters，并把已存在的 `score_metrics`/Select/runtime 计数写入 TEST_ONLY 日志；T payload 拆解只新增计量输出。没有把明文 score、rank、比较位或 selected index 引入 secure 路径。

### 3.2 计数点到字段与恒等式

| 实际代码/采集点 | 记录字段 | 复核关系 |
| --- | --- | --- |
| `t_tls_offline()` 在 streaming material generation 前 reset，结束后读取 KeyGen 计数 | offline DCF KeyGen calls、KeyGen node expansions | 只归离线；不得加进 `online_prg_calls_total` |
| T shell plaintext 减固定 44-byte wrapper 和 manifest，再加两方 DCF plaintext key payload | `material_payload_bytes_p0/p1`、`offline_material_total_bits` | `8×(payload_P0+payload_P1)`；sidecar ciphertext、shell/package、manifest、TLS bytes 分列 |
| `party_online()` 在算法入口前后 reset/read DCF counter；`dcf_eval_counter_matches_runtime` | P0/P1 DCF Eval 和 node expansions、online PRG | counter 开启时计数等于 runtime DCF Eval；online PRG 是双方长度倍增 node expansions 总和 |
| raw adapter `score_metrics.ucmp_calls/raw_dcf_calls`；Select 每轮 `unique_select_slots`；membership slot | `comparison_edges_total` 与明细数组 | BMW16：raw adapter uCMP + `sum(unique_select_slots_r1_r4)` + membership edges；dummy/repeated/logical/材料槽不是边数 |
| `ProtocolIFramedChannel` P0/P1 send/receive 累加 | online total/per-party、分方收发 | `P0_send=P1_recv`、`P1_send=P0_recv`；`total=8×(P0_send+P1_send)`；per-party=`total/2` |
| 两方 `online_message_phases` 与实际传输依赖 | `online_rounds` | 两方阶段值相同；完整 BMW16-derived DAG 12 阶段，不把 Select 四层误报为完整 4 轮 |
| 同一 attempt 外层 offline/online timer | `total_time_ms` | 同次 `total=offline+online`，S32 auditor 容许 0.002 ms 的 1 μs 粒度差 |

DCF Eval、AES block、SHA-256 sampler counter word 和 PRG stream call 是不同单位。代码审阅确认 online PRG 字段不应把离线 KeyGen、sampler words、TLS 内部 RNG 或 DCF Eval 次数当作相同调用。`online_rounds` 依赖消息 DAG；C++ `TEST_ONLY` 固定 sampler tape 会跳过 coin exchange，因此 ON/OFF 校准夹具出现 11 个 phase counter；正常随机 coin 的完整入口与 DAG 是 12 阶段。此校准差异没有改变正式主轮数定义。

### 3.3 S32 历史记录及哈希复核

对 S32 的四个原始 `run_record.json` 重新运行 `bmw16_run_record.py audit`，4/4 PASS。文件存在且 SHA-256 与 S32 报告一致。它们仍是 schema-v2 的历史实验身份，S33 不批量改成 FORMAL。

| 配置 | 原始 record SHA-256 | 原始 evidence manifest SHA-256 |
| --- | --- | --- |
| n=8, K=3 | `82c04163458f0602a9712387acd84e56ba999615270ba20a1a2448f716accdb5` | `04e620cd8b6b72ebcfeb466aa4d9052c162a1b634dc2e66d1afa7ac7d780c4a9` |
| n=128, K=8 | `3495086a82608dbd11f92eece867cd9425dafcb0bb83a9d0a130248a25ef05f4` | `49d9e18672a1f87c028ac04a809366f214c42dea103a12af85b4650447116733` |
| n=256, K=80 | `25d1fcd73b7c2c75d44ce78a13d2f9d09f59abce9b8616c413075c8a4739e58c` | `7671c796794fdb38d56b282b76c020392a5eff71afbda3bccd4870d9df112f05` |
| n=1000, K=80 | `95c96f3998d811a9c2071d8695bffaf53fa99111a6a578251a3ebfc0bf34edd1` | `6fd32688c838d49aa6f37145ed781d6f55a62845c31920548328af38b3260f5a` |

S32 `offline_time_ms` 把 TEST_ONLY raw-share fixture 创建纳入 offline 外壳；schema-v3 修订后将输入生成/份额夹具明确移到 auxiliary。原始记录及 hash 保持不变。

### 3.4 计数器 ON/OFF 功能、失败状态与时间开销

在同一 TEST_ONLY 随机带 `(n=64,K=8,score=0×64)`、algorithm tape `alternating`、seed `603299`、OMP=1、S32 deterministic `getrandom` shim seed `603299` 下，重新构建计数器 ON/OFF。3 次成功样本的 `edge_fnv64`、logical/unique/dummy/repeated Select 计数、DCF Eval、应用帧 bytes 和消息阶段逐次一致；计量开启的 DCF expansion/AES 计数是额外观察列，不改变协议输出。完整 party test 的规范化输出在移除时间和仅计数器字段后逐字节一致。注入 `n=5,K=2` forced abort after Select 时，两方均同意 abort reason，双方无 mask；ON/OFF 失败状态一致。

以每次 `max(P0_online_us,P1_online_us)` 比较，ON `39.162/41.139/44.169 ms`，median `41.139 ms`；OFF `34.992/35.682/38.072 ms`，median `35.682 ms`，观察差 `+15.29%`。这只是 3 次小样本 socketpair/进程内 TEST_ONLY 对照；不含 T、mTLS、磁盘或真实网络，不能外推生产运行时。S32 历史同类校准 `39.514 ms` vs `35.970 ms`（+9.85%）仍按当时二进制/样本保留，不能用 S33 数值覆盖。

S33 counter 证据目录：`C:\Users\28641\.codex\evidence\m6b-s33-counter-calibration-fixed-tape\`。索引 `comparison_summary.json` SHA-256：`d461c09f5fa29767d280b12991131eeab1a3cce775ab3ad615b2da3137c43368`；它包含八个逐次日志哈希。ON/OFF binaries：party test `f3d6206b3581bc052fde89f56cb430802bb414e7c8494c2f82d161833e4e8dda` / `6ca273c63462f879d53a82c5d3e498d18d1be4a2fc71538e7df1c4c717305bbb`；固定 getrandom shim source/binary `8f6dad1009d9eeb982f67a1c26b7b8ecdd7c2969c1fbe7a036cf8935f4f515e6` / `dbdac74f0a957a2c4fea1a4f6e3e582dbeda30292314b4f341b6e688ac44d182d`。

最初一次 ON/OFF 尝试只固定 score/tape，没有固定 OS `getrandom` 熵，因而随机轨迹/计数不同；它作为未配对探索记录留在 `m6b-s33-counter-toggle-on-off`，不被算作语义差异、PASS 或正式计时样本。之后使用 S32 固定 entropy shim 重跑，才得到上述可比校准。

## 4. schema-v3 三路线合同与冲突裁决

| 指标 | S31 runbook 原边界/差异 | S32 代码和 collector 实际行为 | S33 冻结裁决 |
| --- | --- | --- | --- |
| offline time | T KeyGen start 至双方 durable-ready，排除输入生成；T→party delivery 被描述为在线方 ready 过程 | 外层计时从 T/receiver 启动到材料 ready 与 raw-share fixture 写完；TEST_ONLY fixture 混入 | 主值到双方材料 ready；输入生成与份额 fixture 作为 auxiliary 同次计时，不能混入主值 |
| offline material | runbook 候选为 sidecar+shell 精确交付文件字节（封装/manifest 按文件计） | S32 已拆出实际持有的 shell material 与 DCF plaintext key payload；另存包/sidecar/TLS app bytes | 主值只计在线方持有/消费的 payload×8；package、shell wrapper、sidecar ciphertext、AEAD/tag、manifest、TLS application/wire 独立列 |
| online time | 输入就绪至双方最终原序 mask | party E2E 从 ready barrier 至双方退出/发布，含进程启动和 mTLS/setup/controller | 主值是输入与材料 ready 到双方最终 mask 的完整关键路径；controller/oracle/清理单列，不能只取一方或 party API 代替 |
| communication | S31 schema 曾考虑 TLS wire 为主计数 | S32 收集 `ProtocolIFramedChannel` 应用帧 sent bytes；接收只核对，不重复加总 | 固定应用帧主口径；P0/P1 sent/received 分列核对；TLS wire 与 IP/TCP wire 可空缺，禁止跨层加总 |
| rounds | DAG 推导 12，Select 四层只是子协议 | S32 采双方 causal phase，并与 raw2+forward2+coin1+Select4+inverse2+final1 DAG 对齐 | 按实际消息依赖；BMW16-derived=12，A/B 本次实现=8；不按帧数或 Select 四层计 |
| online PRG | runbook 要求覆盖 PRG stream 调用，同时承认原计数不足 | S32 在线 DCF 实际扩展计数点已存在；DCF Eval、AES block、sampler words 可分解 | 在线长度倍增调用单列；DCF Eval、AES blocks、SHA sampler words、离线 KeyGen/root stream 分开报告 |
| comparisons | S31 runbook 候选公式把 Select 逻辑请求和 n−1 membership 请求相加 | S32 collector 采用真实 raw adapter uCMP + 唯一真实 Select edges + membership；dummy/repeat/slots 分列 | 只记实际执行的无序安全比较边；不同 task/round 重新执行另计；logical/dummy/repeat/材料槽分栏 |
| total | 同次 overall wall time | S32 同一 attempt timer，auditor 检查 offline+online | 同一 attempt 逐行对账；不相加不同配置 median |

路由映射：A 全对全为 raw-score input uCMP + all-pairs 核心边；B AAV86 为 raw signed-score 比较 + 当前 active graph unique edges，`r=2`，逻辑保留材料槽另列；C BMW16-derived 为 raw adapter uCMP + 每个 task/round 唯一 Select 真实边 + membership。三路线各自保留实现标签、revision、工作树 diff、二进制/build-options SHA、完整入口计数、材料边界、应用通信、PRG/比较边和失败状态。E15/E17/E20 的旧 revision/材料边界不纳入 S33 排名。

## 5. 三路线实现身份与执行环境

| 路线 | 标签 / 源 | binary SHA-256 | build-options SHA-256 | 材料/轮次说明 |
| --- | --- | --- | --- | --- |
| A | `Protocol_I_all_pairs`，E16 source `267cb6924f061d376dbce7986fc0093b52ec2512`；TEST_ONLY bridge diff `78cee254c7de71a247bfb439b0d31c3d22434b3421416bd766346a6f68696e18` | `eb9539973e4d69450633ae88e382c5f1b723d213d8f486887811bb88aa78265c` | `a167d9f3c00acd3da1883fbd21645c9471e35efc527f751414cb213f8cd35f57` | E16 clique benchmark 的实际 pair materials；当前端到端 8 应用阶段 |
| B | `AAV86/r=2`，相同 E16 source；仅修改 TEST_ONLY timer/input/oracle identity bridge | `238bfe1c5362572f7f3ebc576c25380eb8a4aebc77dc0b1529edfd696f6bcf8b` | `a167d9f3c00acd3da1883fbd21645c9471e35efc527f751414cb213f8cd35f57` | `protocol_i_aav86_small_*` 的 r=2 party material/keygen/package；本诊断不是 AAV86 新安全接收；当前端到端 8 应用阶段 |
| C | `conditional_secure_v2`，base source `32cd7952cccdacc5a1a5b4fcca245f7acf594049`；S33 TEST_ONLY harness diff `941ef94a99ae3de4f5566d108a98812f2df58ead01b41f85bcdcb88a0628c7de` | E2E harness `f0dfeca2875dfb57a8ef9900a031496b0a0b7b135fb958c42582804a3728d529`；runtime party-node `08f1df9204a9593a8daf5163051d4ad18ff949846d86aa98df80ea8bba6c197e` | `3d5c2d6769a40237f0cbfb74c692f5bf04223467384c806e3aec7879ece4c581` | S31 条件安全范围；fresh T material/session；完整 DAG 12 阶段 |

环境：WSL2 Ubuntu 24.04，Linux 6.6.87.2-microsoft-standard-WSL2，GCC 13.3.0，CMake 3.28.3，OpenSSL 3.0.13，i9-13980HX，32 vCPU，约 8.12 GB RAM，Release，OMP=1。BMW16 build 使用 adapter/v2/DCF counters ON、v1/failpoints OFF。E16 bridge 为同机独立进程/TEST_ONLY 输入口径，非双机 LAN/WAN。

## 6. 三路线小规模配对诊断结果

每配置三路共用以下相同 score 向量与冻结 oracle（FNV64 与 SHA-256 双重比对）；每次 route 单独申请 session/material。signed Q20.12 生成器为 `mt19937_64_uniform_signed_Q20.12_v1`。

| n/K | input seed | input SHA-256 | oracle SHA-256 | oracle FNV64 |
| --- | ---: | --- | --- | --- |
| 128/2 | 771337 | `e6e179d28130a03ed912c0c4c5216662ca5f3e934ab858b87dd16a614e596a7b` | `ed11adfda5bdd2e8e19bc097d211be0dd38dfadf839c0df9e05dc4678f02aeaa` | `7721330113122150475` |
| 128/8 | 20261010 | `e7b574da348eabc66decaaa9fbf28f8d922daa2d292a859cb828e44aa26c38f2` | `248e9d83aaca24db86375768679df0f2fab3239618283c3e48d0d586eb8ed640` | `341159367908390313` |
| 256/2 | 20261011 | `127e8d6e540c3492e8a8a669d5c9d2e1473c7a3083521788c2ea1d9e8d94cf38` | `ae921b1289f136eef18f17586f5ad3172be0087242270983021f965628a175c0` | `7509538231265104939` |
| 256/8 | 20261012 | `a086165afa4be5d49dc2926fe4ceadd5952cd3cf752d23003f4c20d97fa0f862` | `ff6d6e032b276979dd170d4e82ffa3ebf99ac9507589f27f4411251f7e7099aa` | `4281867390158351461` |

以下为每一 attempt 的九项主指标。`comm total/per` 单位 bits，材料单位 bits，时间单位 ms；边数是实测无序比较边。数据是单机 WSL loopback/test transport 的 DIAGNOSTIC 行；不计算跨配置 median，不作性能排名。

| n/K | 路线 | offline | material | online | comm total/per | rounds | online PRG calls | edges | total | run_record SHA-256 | evidence manifest SHA-256 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |
| 128/2 | A all-pairs | 155.027504 | 135,667,712 | 35.587591 | 1,479,424 / 739,712 | 8 | 1,335,296 | 8,384 | 190.615095 | `6dcf337920c4dfbbe9c6121e568672b71c033eb3037e94ae2d9a9f8fb6aadee6` | `301634ab8a101c81292f1c0443232dabda416e28187a3692f76eb8ad3bf87c5a` |
| 128/2 | B AAV86/r=2 | 187.114367 | 259,571,712 | 18.650982 | 169,984 / 84,992 | 8 | 399,136 | 2,533 | 205.765349 | `4d235eaa89720eaf815b556b99ec074f9c715ed7a7415d4517a8671efe0c3bb6` | `d77a81ab0d408f1537e2590915fb4f3cc2d47f840be8fe62fa7b98dcac6eeb6d` |
| 128/2 | C BMW16-derived | 2,507.578000 | 1,194,366,240 | 1,313.392000 | 5,942,288 / 2,971,144 | 12 | 1,766,976 | 11,082 | 3,820.970000 | `c6a357f8f2939af82714ba60522aa6d65e5aaa9006bf157d572da83fa48e32c6` | `96bb5845e2674efc22c831e5e9e307296244499c97c333c3e67aa15749d9d0b6` |
| 128/8 | A all-pairs | 152.188405 | 135,667,712 | 34.093552 | 1,479,424 / 739,712 | 8 | 1,335,296 | 8,384 | 186.281957 | `29ef147dc8287f679db89de74d997809925fe8d1a6c17171096de92dd08c8db3` | `b762ff3d70443d0f13e69958acaac6b9f4861dff160f332fe45ff34b98e7b4fc` |
| 128/8 | B AAV86/r=2 | 169.490553 | 259,571,712 | 20.158925 | 169,984 / 84,992 | 8 | 425,216 | 2,696 | 189.649478 | `10cafb8be0654de6c6b557dfa076e9043a8dabd7af0802900d0b904e36308fdb` | `c641413b9e7a6a2fe2e462c337f19b046e4e4e852203db5a9a84c6f051f28756` |
| 128/8 | C BMW16-derived | 1,260.357000 | 1,194,366,240 | 1,207.861000 | 5,484,304 / 2,742,152 | 12 | 1,827,616 | 11,461 | 2,468.218000 | `3d3f431429715d4f753912659f49a960d3518d5062dc87e8a5c1a51668dd7fb1` | `91ab59b04d5a18bf50d630d99964cc8838936a0f2421a681a328d6ac86fd7d7e` |
| 256/2 | A all-pairs | 497.099241 | 543,064,064 | 139.304333 | 3,347,200 / 1,673,600 | 8 | 5,422,592 | 33,152 | 636.403574 | `06953156aadaa10c177e7638f672716f4c0fbfd47271fdfa102f5a96f6bba946` | `606e9ce7f0169bf130c10811fcd52d15e9a06f11477ed8eccd6562bfaae8d68a` |
| 256/2 | B AAV86/r=2 | 687.976350 | 1,060,110,336 | 53.659385 | 333,824 / 166,912 | 8 | 1,478,228 | 9,101 | 741.635735 | `95ee618276af66a6fd3ac0e426e97cf6c1d6d1ab0c78c9c86b90a91e47775f64` | `309439784853454edb8ee8d2824a666759744c79d68b87eb3032226508aea80f` |
| 256/2 | C BMW16-derived | 5,454.381000 | 4,900,938,016 | 1,653.833000 | 11,655,312 / 5,827,656 | 12 | 3,432,780 | 21,019 | 7,108.214000 | `cfd00e7bca9c26bd6cfad01ef0004a45d498771ca9f1e08e25a8f1337f2d0cb6` | `cc1c0491411ccdfa92ffb1def870dc7e56c9d82a0005e487d65c116dd6692051` |
| 256/8 | A all-pairs | 500.676961 | 543,064,064 | 147.689534 | 3,347,200 / 1,673,600 | 8 | 5,422,592 | 33,152 | 648.366495 | `b7f7180682f46ec12476f9ef0e5e39b4d454ac592a3a04927aa8eb4356b58aeb` | `169437c8a3e38566cc30ce4478b1278c406e874384cbd56d89d4720506ea750a` |
| 256/8 | B AAV86/r=2 | 703.568505 | 1,060,110,336 | 49.911799 | 333,824 / 166,912 | 8 | 1,286,676 | 7,933 | 753.480304 | `21a94e7fade9175d53f2934e863ab290a0cb7e10434732e56a16ee7a39d73e08` | `ee732021a2871e6c485a2305259e16f55758ff88906b99c5590372332d51bd94` |
| 256/8 | C BMW16-derived | 6,088.472000 | 4,900,938,016 | 1,688.810000 | 11,793,808 / 5,896,904 | 12 | 4,445,316 | 27,193 | 7,777.282000 | `822bf4af806a5218b05a80ae2dae21cb710b854613c1e92a7dfebc9d3112226b` | `ac8f1559f9b1082bbab71e51b232a60c4e3d41c8a47c605b570ca87f9a2c3c31` |

四个 campaign record SHA-256（每个 campaign 内含三条 attempt 引用及其 SHA）为：

| campaign | SHA-256 |
| --- | --- |
| `pi-v3-128-2-diagnostic-4aa945f3-f566-48dd-8bdf-2cb3d0747396` | `e7888cfb029b75c2a5235e96cb9dff917756de0f7eb27e6cb1e18c26c72522bc` |
| `pi-v3-128-8-diagnostic-5ba0a284-9f8e-4c86-a166-c163a4f7b57f` | `f7a8bbcbf511ea8bb5154454fb75be1f7172cc72bcf1730545a85ca71c02e4df` |
| `pi-v3-256-2-diagnostic-e510cf15-79c5-4fc8-aa6c-960b1ec48bf4` | `2f3edebca9e812c457672e98f819ac197e282efd0370cad7eaf6de5fcd21a8a1` |
| `pi-v3-256-8-diagnostic-1e8b02b6-de24-4555-a49b-70b3d68463c7` | `9e2fb69a8a7b1cccc487cbb9dcce60b888c07d0bace0496791e3482f613360d7` |

四个主 campaign 的 run record 均记录当时 collector SHA-256 `d5ec2eac533327431865b052c51996b03f46f8d4a368d5a5594671a1b5493abf`。其后 gate spot-check 使用 collector SHA-256 `cbb030e8921728ee5877f500a963cf906beb82e4454618b5096c838e6ea04331`；当前最终 collector 增补了共享输入身份失败时的 record 生成，SHA-256 为 `c920b635ec2884b624aeceb7ab95a82ab133a4345b6dbe3d5b9544a7da69c7df`。三个版本均保留 source hash 字段；主数据由其采集时版本生成，随后版本用同一审计公式复核。

collector 的 campaign audit 会递归审计每一 attempt 的九字段、null/provenance、mask/no-mask、通信对账、总时间、实际边恒等式、共享 oracle FNV64 和 manifest/raw 文件哈希。上述四个 campaign 均 `audit=PASS`。每条成功 attempt 的冻结 oracle 相同；原序 mask 长度 n、二进制位和恰 K 位断言均为 true。TLS wire、RTT、吞吐、root PRG stream 和控制器/网络辅助层中未实测的字段保持 null/`NOT_MEASURED`。

四组配对诊断原始目录：`C:\Users\28641\.codex\evidence\m6b-s33-paired-diagnostics\<campaign_id>\`。额外的 gate 后同输入 spot-check（不属于上表四个配置重复）在 `C:\Users\28641\.codex\evidence\m6b-s33-validation\paired-spot-check-after-gates\pi-v3-128-2-diagnostic-fb42d177-1f1f-449c-bf49-c257155ce8de\`，3/3 SUCCESS、campaign audit PASS，campaign SHA-256=`1923b4423c244922a102c1eeb7186ec9d042348e128f88abcaecf4f36bd2ad76`。

## 7. 失败保留与 n=1000/K=80 预检

所有实际 route failure 均保留了 `run_record.json` 和 manifest，不从尝试分母中删除：

| 诊断 campaign | 路线/状态 | 记录 SHA-256 / manifest SHA-256 | 原因 |
| --- | --- | --- | --- |
| `pi-v3-128-2-diagnostic-18d7955b-d4c1-445c-a6c8-f9b1921d3486` | AAV86 `ENGINEERING_ERROR` | `4bcd02718a7ff9f264f0d1f1139c5406c844e318299f258a907709b85726daed` / `260e42e772b7f48653beece18d230c664ff39de02da20d53317cfa82405f2145` | E16 T 阶段 D128 资源/`RLIMIT_AS` gate 失败；未成为安全/正确性失败 |
| `pi-v3-128-2-diagnostic-deb7768f-d286-4246-a340-55345c379c80` | AAV86 `ENGINEERING_ERROR` | `3ccf2a28c438f753d823c3250d9febed7cfeacfeb4bcccf58d9946d30ef960f3` / `4e8e6651e4931660854e737c48d230c664ff39de02da20d53317cfa82405f2145` | 同类 E16 T 资源 gate；该 campaign BMW 路线另有 parser failure |
| 同上 | BMW16 `MATERIAL_ERROR` | `887518d38bb713a59e19f2f18fec626d45a4e03925398aa8ba3ed74491488a6b` / `c4c78031864fcfd1d2c474988c73a00401e79e08fa2f0136c4681df37c30cede` | 早期 collector 解析器期待 `sidecar_bytes_p0`，而 T marker 提供 per-party 字段；后续 schema-v3 parser 同时接受 `sidecar_bytes_per_party`，最终四组通过 |
| `pi-v3-128-2-diagnostic-4dfbd555-cf53-49ad-8ad7-c9655baadffa` | BMW16 `ENGINEERING_ERROR` | `078aca018747863f4877050b2e48b55f57e35d98e40900dc989ab34a901d8d15` / `807355af955e9ae4ae86f622a09f5bc05bb20e6a1672c62428fbc2052eb963a2` | 非 root harness 无法 provision T/P0/P1 distinct OS identities；未启动协议。随后 root harness 成功 |

另对“共享 TEST_ONLY 输入身份工具无法启动/缺少 marker”故障路径做了主动注入：S33 collector 仍生成 3 条 `ENGINEERING_ERROR` attempt、完整 failure_reason、空指标的 `NOT_MEASURED` provenance、collector source SHA、manifest 和 campaign record，并通过 auditor；三条协议 route 均注明 `protocol_attempts_started=false`。该 recorder-only 验证 campaign 为 `pi-v3-128-2-diagnostic-373096b7-4aa2-4253-94e8-dc534c50b9df`，campaign SHA-256=`808a6c597c9e81262b2ff61dec036e145d983f3c32e4ea48f78abbf6814cb6b4`；三条 run-record SHA-256 依次为 AAV86 `fbe44c7be058dd785b5ece0bc75b805c4026530d3ffca29342f751b4a19f9c00`、all-pairs `97bdca03a7a2797fe7348989bc82729b51cc7ce6383b7af394da085bcd277627`、BMW16-derived `deb78fbf029405393cb5457198e41d226d1e6a7ab38e78795fb54f802a18dadc`。原始文件位于 `C:\Users\28641\.codex\evidence\m6b-s33-validation\identity-failure-test-final\`。这不是协议运行或性能样本。

`n=1000,K=80` checked preflight：D=1024；总 RAM `8,122,368,000` bytes、当时可用 RAM `7,308,337,152` bytes、scratch 可用 `985,966,764,032` bytes。结果文件 `n1000-k80-preflight.json` SHA-256=`22a1a60abd70dd5000094f356bb09fa7398f7ef60f824da9ad48d40e6df2244c`。

- all-pairs：`PRECHECK_REJECTED`，E16 benchmark entry 只接收 n∈{128,256}、K∈{2,8}，n=1000 没有 paired bridge binary。
- AAV86：`PRECHECK_REJECTED`，D=1024>256 是 T/keygen 前硬门。
- BMW16-derived：n/K 在 `conditional_secure_v2` 参数范围内；S33 原 runner 的 `READY_FOR_DIAGNOSTIC` 只代表形状检查，却被误写成准入状态。S34 已修正为 `RESOURCE_REVIEW_REQUIRED`。当前主机总 RAM 约 8.12 GB，低于 S31 建议的至少 16 GiB；约 9.94 GB 的历史 package 规模、峰值 RSS、网络及时限也未完成当前环境准入。因此未单独启动，九指标为 `NOT_MEASURED`。
- 网络流量、RTT/吞吐和 runtime 均为 null/`NOT_MEASURED`；没有把 S32 n=1000 结果外推到 A/B 或当前 S33 revision。

## 8. 命令与验证

主 BMW16 build：`/tmp/m6b-s33-build`，Release，`BUILD_TESTING=ON`，adapter/conditional-v2/DCF counters ON，v1/failpoints OFF。

按指定顺序重跑的关键命令：

```bash
ctest --test-dir /tmp/m6b-s33-build --output-on-failure \
  -R '^moe_topk_bmw16_conditional_secure_v2_conformance_test$'
# 1/1 PASS

ctest --test-dir /tmp/m6b-s33-build --output-on-failure \
  -R '^moe_topk_bmw16_experimental_party_test$'
# 1/1 PASS; includes frozen top_k_mask oracle assertions

/tmp/m6b-s33-build/moe_topk_bmw16_experimental_party_node_e2e_test \
  /tmp/m6b-s33-build/moe_topk_bmw16_experimental_party_node \
  --conditional-secure-v2-paired-case 128 2 771339 2026101012813
# T/P0/P1 distinct UID; SUCCESS_ORACLE_CHECKED; original-order mask weight K;
# durable material replay rejected by both parties with no mask.

ctest --test-dir /tmp/m6a-s33-bridge-build --output-on-failure \
  -R 'moe_topk_m6a7_aav86_small_(conformance|differential)_test'
# 2/2 PASS (conformance, frozen oracle differential)
```

之后以统一 collector 重跑一个三路 128/2 gate spot-check，A/B/C 均 SUCCESS，campaign audit PASS。四个主矩阵 campaign 的复核命令：

```bash
python3 VFSS/tests/bench/protocol_i_unified_diagnostic.py audit <campaign_record.json>
```

上述四次均 `audit=PASS`，递归覆盖 12 条 attempt 和全部证据 manifest。S32 历史四条用：

```bash
python3 VFSS/tests/bench/bmw16_run_record.py audit <run_record.json>
```

结果 4/4 PASS。另通过 schema-v3 JSON parse、Python collector syntax compile、S32 BMW16 定向 CTest 3/3、E16 AAV86 conformance/differential 2/2、`git diff --check`（主 worktree 与 bridge worktree）。schema-v3 built-in audit 检查九指标字段集合、单位、null/provenance、通信守恒、attempt 时间、无序边公式、mask/no-mask、输入/oracle FNV 及文件 SHA；没有依赖把未测项写成 0。全仓 CTest 未启动。

BUILD binary SHA-256：BMW16 party-node `08f1df9204a9593a8daf5163051d4ad18ff949846d86aa98df80ea8bba6c197e`；S33 E2E test binary `f0dfeca2875dfb57a8ef9900a031496b0a0b7b135fb958c42582804a3728d529`；party test `f3d6206b3581bc052fde89f56cb430802bb414e7c8494c2f82d161833e4e8dda`；CMakeCache `3d5c2d6769a40237f0cbfb74c692f5bf04223467384c806e3aec7879ece4c581`。计数器 OFF party-node/E2E/party-test SHA-256 分别为 `89303bb8765c05d05690760379bc0e8afe30954691cfa9dd99023378e2637bf4`、`04effbd73ca14a3e2c28296fc7843531bb497b1ebef08346cd3ef9a4ae819641`、`6ca273c63462f879d53a82c5d3e498d18d1be4a2fc71538e7df1c4c717305bbb`；对应 CMakeCache `d06f5ad96b048565069935c27162ebadf012038979765cde0025c585775bb247`。全部 build/output 均在仓库外。

gate 后 representative E2E 的外部日志 `C:\Users\28641\.codex\evidence\m6b-s33-validation\representative-independent-e2e.log` SHA-256=`80ad36b51a17c425ffb5139583450e5dde070054726e7042575b6c66fccf8db9`。

## 9. PR 状态与剩余工作

截至 2026-10-12，GitHub PR #29 仍为 Open，标题为旧 S26 标题“m6b i bmw16 s26-Add opt-in BMW16-derived Protocol I Select candidate”，base 是 `main`，head 分支 `codex/m6b-i-bmw16-s26`，远端 `refs/pull/29/head` 与该分支均为 `32cd7952cccdacc5a1a5b4fcca245f7acf594049`；`main` 为 `c3926c68fd14f270faa8b55234311071947fa080`。正文仍是通用模板。页面列有 Copilot review 请求，但公开评论明确因 requester quota limit 未完成 review；没有 reviewer approval。故“功能独立签收”指 S26 报告中的实验候选 `DRAFT_PR_READY=PASS`；它不等于 PR reviewer 批准。S32 source/report 已在 PR head；S33/S34 本地工作尚未推送，也不属于 PR。GitHub CLI 未安装；以 `git ls-remote` 和公开 PR 页面复核远端身份及状态。没有合并 PR。

正式 V4 的最短后续路径：

1. 先按 M6A/V3 计划关闭 V3 门并留存 machine-readable PASS；S33 不改变该顺序。
2. 服务器上冻结三路线源码/二进制/build options、共同输入/oracle 和执行环境；先 checked 磁盘、RAM、网络流量及时限，再确认每路线可执行 fresh material/session。
3. 用服务器侧 attempt collector 记录 LAN/WAN RTT、吞吐、拓扑/测量方法和失败原因；每路线每配置 1 warmup + 5 formal records。收集 TLS/wire 若可用但不得与应用层混加。
4. 使用 `seal-campaign` 对三路线 18 条 attempt、共享输入/oracle、各自 hash 和 V3 gate 进行封存；只有通过之后才形成 V4 正式结果。n=1000 A/B 准入仍须先解决各自入口/容量边界。

截至 S33 原始报告形成时，报告和决策未提交、未推送；S34 之后仅整理为本地可审查提交，没有推送或合并。没有更改 PR #28/#29、`main`、冻结基线、论文、密钥/证书、sidecar 或仓库内构建产物。

## 10. S34 技术复核更正（2026-10-12）

S34 对本报告列出的源码、记录和外部原始证据作了复核。审查者参与过 S33 的实现与运行，故状态为技术复核；S26 的历史独立功能接收、S31 的条件安全接收分别保持原身份。

- S33 选定的 4 个配对 campaign 共 12 条 attempt 均通过当前 schema-v3 JSON Schema 与增强后的 auditor。S34 对其 `input_sha256`、`oracle_sha256`、n/K、command identity 和每路 session/material serial 作二次核验，未发现最终矩阵数据不一致；campaign 与 attempt 原始 JSON、manifest 和 SHA-256 均未改写。
- 原始目录共检出 22 个 campaign（88 个 campaign/attempt JSON 文件）：7 个 campaign（28 个 JSON 文件）通过 schema-v3 与 auditor；另外 15 个早期探索或失败 campaign 保持原状并排除为最终矩阵。8 个旧成功 campaign 缺 null/`NOT_MEASURED` provenance，4 个旧成功 campaign 缺共同 oracle FNV 身份，3 个失败 campaign 使用早期不完整失败记录合同。S33 曾遗漏索引一个更早的身份 fixture 失败 campaign `388b64e5-56c0-4605-952d-4bc7963ef6d7`（SHA-256 `a701675545ebb4ea5ed657b21ec684acbf5f35f214fb17cffaa68fb9c5842f4a`）；该记录 audit/schema-v3 PASS，仍是失败记录，不是协议运行。
- 发现并修复两项 collector 缺陷：n>1000 的 BMW 路线不再错误显示为 READY；campaign auditor 现在对比配置与每条 attempt 的 shape、输入/oracle SHA、seed 和 command identity，并校验 command.json 中 hash-bound session/material serial 在 campaign 内唯一。正式 `seal-campaign` 使用同一身份审计。新增 8 项 TEST_ONLY Python 测试通过。
- 重新执行当前 schema-v3 auditor 后，22 个 campaign 中 7 PASS / 15 FAIL；7 条通过记录分别是 4 个正式选定诊断 campaign、gate spot-check、最终身份失败记录和更早身份失败记录。其余旧记录没有被编辑或改标。
- 当前 checked n=1000/K=80 的单路线 C 状态为 `RESOURCE_REVIEW_REQUIRED`，不是已准入。当前 Ubuntu 24.04 WSL 主机约 8.12 GB RAM，低于 S31 建议的 16 GiB；当前峰值 RSS、网络流量/RTT/吞吐、时限未测。n≥10000 受 C 的 n≤1000 包装界、B 的 D≤256 硬门、A 当前桥接形状范围和磁盘容量估算共同阻断；没有生成全池材料。
- 本次定向构建与验证结果、原始 campaign inventory/preflight SHA、当前 PR 和 V3/V4 门状态见 [S34 技术复核报告](BMW16_S34_SCHEMA_V3_TECHNICAL_REVIEW_AND_ADMISSION_REPORT_2026-10-12.md)。
