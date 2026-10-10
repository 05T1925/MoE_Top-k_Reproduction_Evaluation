# S32 计量实现与研究诊断报告（2026-10-10）

## 范围与身份

路线：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED**。它是六方案规划中原
BB90 比较位置的项目衍生替代候选，不是 BB90 原算法复现或 BMW16 Algorithm 7 逐字实现；不继承
BMW16 的概率界或比较常数。S31 已独立接收的 `conditional_secure_v2` 条件安全范围保持不变；本报告
不重开该证明门、不创建新安全 alias。四个 Select 比较依赖层不是完整 raw-score→mask 的四轮。
本报告记录的性能数只属于 `EXPERIMENTAL_DIAGNOSTIC`，不是 V4 验收数据。

## 基线、提交和被测对象

- S31 接收文档基线：`a1a90cb2a568deb5e5078ddec10136ce9a743411`。
- 开始时本地 `main=origin/main=c3926c68fd14f270faa8b55234311071947fa080`；S31 chain 与 main
  尚未合并。开始时 PR #29 source/ref 为 `codex/m6b-i-bmw16-s26@a1a90cb2...`，
  `refs/pull/29/head` 同值，PR 目标 main，未合并。
- S32 运行时代码提交：`f2db098578b1cc096298ee39d990ff668d8dd6f7`
  (`Instrument BMW16 diagnostic metrics and run records`)。本次所有新诊断均用此 revision 构建。
- S32 报告/决策在运行时提交之后提交；不得将文档提交称作被测 runtime。
- 当前源码二进制 SHA-256：party-node
  `e895cff05e85a18d314b1fcbc2c41226fbc99342952de380253863afbcf56624`；E2E harness
  `19ae7cf76ebd28d8e15747d6ec9a26560f98fffe4a2444c513afe831bfc56290`；party test
  `a84765722a9ee6d2732b25e5359503adb333bdb10560e21917af31a9c0535a20`。
- 构建目录 `/tmp/m6b-s32-build`，Release；adapter/v2/DCF counters ON，v1/failpoints OFF，
  `BUILD_TESTING=ON`。默认 OFF 构建为 `/tmp/m6b-s32-default-off`，Release、所有 BMW16 选项及测试 OFF。

## 指标定义对照与采集点

| 主指标 | 仓库统一定义 | S31 schema/runbook 差异 | S32 实际计数与审计 | 结论 |
|---|---|---|---|---|
| `offline_time_ms` | 本次执行必需预处理边界明确 | S31 计划描述完整，但未将测试输入 share setup 清楚分栏 | E2E harness wall clock：receiver/T 启动前至 T/双方 receiver exit 0、pair-ready、raw shares 写好；包括测试 share fixture；同时保存 party/T 子阶段。 | Measured diagnostic envelope，不是纯 T time |
| `offline_material_total_bits` | 在线持有的全部预处理 | S31 主口径容易与 package/TLS bytes 混同 | T writer 实际汇报每方 shell 内预处理 payload + 实际 DCF key plaintext bytes，两方相加×8；不含 fixed shell wrapper、manifest、AEAD tag、TLS。package/sidecar 文件字节及 TLS app delivery 另列。 | Measured payload；其他封装字段辅助 |
| `online_time_ms` | 输入/离线材料 ready 至两方 mask 完成 | S31 runner 未提供统一同次分解 | 同次 E2E wall clock：pair-ready+raw shares 到两个 party exit/output publish；含 party spawn、mTLS setup 和 harness 协调；另存 party API critical path。 | Measured diagnostic envelope |
| `online_comm_total_bits` | 所有在线方实际 sent bits 之和 | S31 曾将 TLS wire 层作为主口径候选 | `ProtocolIFramedChannel` app framed `sent_bytes(P0)+sent_bytes(P1)`×8；接收只作双向守恒。TLS handshake/records/TCP/IP/retransmit 未测。 | Measured application layer |
| `online_comm_per_party_bits` | `total/2` 标量 | S31 schema v1 错写成 party-object | S32 schema v2 为 bits 标量 `total/2`；P0/P1 实际 sent/received bytes 在 counts 另列。 | 修正 schema 表示 |
| `online_rounds` | 因果依赖在线轮数 | “12 phases”不应与 Select 四层混称 | 双方 runtime phase counter 相等并对应 DAG：raw 2 + forward 2 + coin 1 + Select 4 + inverse 2 + final agreement 1。TLS handshake/frames 不计协议 round。 | Measured counter + DAG audit |
| `online_prg_calls_total` | 在线长度倍增 PRG 调用 | S31 只覆盖普通 DCF扩展调用 | DCF 实际 AES node-expansion 调用点计数，party counter 与运行时 DCF Eval 及宽度公式核对；调用数、DCF Eval、AES blocks、SHA sampler words 分别记录。在线适配/Select/shuffle/inverse 源码审计未发现第二个长度倍增协议 PRG 调用点。 | 本实现的在线协议 PRG measured；TLS 内部 RNG 排除。T/root PRG stream calls 是离线且 NOT_MEASURED |
| `comparison_edges_total` | 实际执行的无序比较边，一边不因两方求值记两次 | S31 logical requests、dummy 与实际边混用风险 | raw adapter 实际 uCMP calls（含 padding，真实/padding 分解）、两 task 四轮去重真实 secure slots、membership secure edges 求和。dummy 本地请求、重复逻辑请求、logical calls、材料槽、两方 DCF Eval 分别另存。跨 task/round 重新执行按一次新边计。 | Measured、计数恒等式 audited |
| `total_time_ms` | 同一次 attempt 的 offline+online | 不应相加独立阶段 median | 同一 outer E2E wall interval；auditor 逐条验证总时间 = offline + online（2 μs timer resolution）。 | Measured same-attempt |

九主指标有实际值的 SUCCESS 记录均标 `MEASURED`。未测的 TLS wire/handshake、TCP/IP、root PRG stream、
T RSS、网络 RTT/bandwidth、协议纯 payload（去 frame）保持 JSON `null` 与 `NOT_MEASURED`。
解析容量不写入 measured fields。`online_prg_calls_total` 是协议层长度扩展调用口径，不包括 TLS 库内部
随机数/加密运算；其 AES blocks、DCF Eval 与 SHA sampler words 有各自独立字段。

## 验证顺序与执行结果

环境：WSL2 Ubuntu 24.04；Linux kernel `6.6.87.2-microsoft-standard-WSL2`；13th Gen Intel
Core i9-13980HX，报告 32 vCPU、约 7.6 GiB RAM；GCC 13.3.0、CMake 3.28.3、OpenSSL 3.0.13；
Release，`OMP_NUM_THREADS=1`。E2E harness 使用不同 UID 的 T/P0/P1 进程与真实 loopback mTLS，
不是多主机网络测试。

命令骨架：

```bash
cmake -S VFSS -B /tmp/m6b-s32-build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON \
  -DMOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V1=OFF \
  -DMOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V2=ON \
  -DMOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=ON \
  -DMOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=OFF
python3 VFSS/tests/bench/bmw16_run_record.py collect \
  --e2e-test /tmp/m6b-s32-build/moe_topk_bmw16_experimental_party_node_e2e_test \
  --party-node /tmp/m6b-s32-build/moe_topk_bmw16_experimental_party_node \
  --source-commit f2db098578b1cc096298ee39d990ff668d8dd6f7 \
  --input-seed <seed> --session <fresh-session> --n <n> --k <K> --threads 1 \
  --out-dir <outside-repo-evidence-dir>
```

在 WSL 中由隔离 root TEST_ONLY collector 启动不同 UID 进程；正常 party runtime 不使用 root 控制器、
不重构明文。收集器在子进程结束后重构并调用冻结 C++ oracle。每个配置 fresh session/material。

| n,K | 输入 seed / session | 状态与正确性 | offline ms | payload bits | package bytes | online ms | app comm total / per party bits | rounds | online PRG | comparison edges | total ms | evidence manifest SHA-256 | run_record SHA-256 |
|---|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---|
| 8,3 | 603201 / 6033002 | SUCCESS, oracle exact, weight K | 55.514 | 3,967,968 | 505,018 | 1,115.812 | 50,192 / 25,096 | 12 | 15,568 | 109 | 1,171.326 | `04e620cd8b6b72ebcfeb466aa4d9052c162a1b634dc2e66d1afa7ac7d780c4a9` | `82c04163458f0602a9712387acd84e56ba999615270ba20a1a2448f716accdb5` |
| 128,8 | 603202 / 6033003 | SUCCESS, oracle exact, weight K | 1,356.148 | 1,194,366,240 | 151,637,602 | 1,224.713 | 5,432,720 / 2,716,360 | 12 | 1,928,736 | 12,093 | 2,580.862 | `49d9e18672a1f87c028ac04a809366f214c42dea103a12af85b4650447116733` | `3495086a82608dbd11f92eece867cd9425dafcb0bb83a9d0a130248a25ef05f4` |
| 256,80 | 603203 / 6033004 | SUCCESS, oracle exact, weight K | 5,392.165 | 4,900,938,016 | 622,018,530 | 1,781.751 | 8,718,864 / 4,359,432 | 12 | 6,624,548 | 40,481 | 7,173.917 | `7671c796794fdb38d56b282b76c020392a5eff71afbda3bccd4870d9df112f05` | `25d1fcd73b7c2c75d44ce78a13d2f9d09f59abce9b8616c413075c8a4739e58c` |
| 1000,80 | 603204 / 6033005 | SUCCESS, oracle exact, weight K | 462,953.841 | 78,361,809,376 | 9,939,083,130 | 79,360.450 | 49,577,872 / 24,788,936 | 12 | 23,011,768 | 134,218 | 542,314.291 | `6fd32688c838d49aa6f37145ed781d6f55a62845c31920548328af38b3260f5a` | `95c96f3998d811a9c2071d8695bffaf53fa99111a6a578251a3ebfc0bf34edd1` |

这组归档复跑记录位于仓库外 `C:\Users\28641\.codex\evidence\m6b-s32-final-diagnostics\{n8_k3,n128_k8,n256_k80,n1000_k80}`；每目录含 `run_record.json`、`evidence_manifest.json` 和脱敏进程日志。独立 `audit` 命令对四条 JSON 均返回 PASS；manifest SHA 与对应记录的 `raw_record_sha256` 一致。临时 shares、key、证书、sidecar 已由 TEST_ONLY collector 清理。运行种子复用首轮报告，session 与 OS CSPRNG 材料均新鲜，因此结果/随机轨迹和首轮可能不同。首轮数值及 hash 留在先前 S32 报告提交 `d5489ca` 的 Git 历史；其 WSL `/tmp` 原始目录现已清理，本表改以当前持久归档复跑作为可直接复核的数据源。

说明：package bytes = 两方 sidecar ciphertext 文件 + 两方 shell 文件；TLS app delivery 为辅助值，不等于 TLS wire。
这四次为每配置单次诊断，不是预热+5重复；自然算法 abort 为 `0/4`、注入 abort 为 `0/4`（四条 E2E 均为正常执行），不据此推导理论失败率。n=1000 本次全链约 9.04 分钟，其中 offline 7.72 分钟；不得当 LAN/WAN 结论。

另有一次非 root harness 启动尝试在创建输入 fixture/party 进程前被 `distinct OS identities` 前置条件拒绝；它没有产生协议消息、材料或输出，不计入 E2E/abort 分母。该启动拒绝的原始终端输出未写入上述持久证据目录，本报告保留其分类和限制，不把它伪装成协议失败或成功样本。

独立边/计数对账：当前归档 n=1000：raw 2,048 uCMP calls（2,000 real + 48 pad）+ Select 131,171
 unique real edges + membership 999 = 134,218；DCF Eval 两方合计 536,872，PRG node expansions
两方合计 23,011,768，AES blocks 辅助数 46,023,536。上述量纲相互独立，不可互换。

## 回归、ON/OFF 与默认关闭

- 39/39 S4/C++ common-random-tape trace differential PASS，S4/C++ 中间控制流和边迹一致。
- S32 相关 CTest `moe_topk_bmw16`：3/3 PASS（record/schema auditor、party tests、conditional v2 conformance）。
- 旧 Protocol I/III 相关定向回归：13/13 PASS（DCF/uCMP、raw score、shuffle、transport、Protocol III routing/combine 等）。
- Python run-record unittest：10/10 PASS；schema JSON parse PASS；`git diff --check` PASS。
- 默认关闭配置 Release 构建 `sytorch` 成功；BMW16 party/conditional 目标未注册，所有 feature flags OFF。
- 默认关闭构建核查：`sytorch` Release build PASS，`cmake --build --target help` 中无 BMW16 party/E2E/conditional 目标；target listing SHA-256 `0f003b827f2402224529d14c7a3ad930d2b26437f46d34b0f77a4012042337b6`，default-off `libsytorch.a` SHA-256 `a2a9bb816a1cd7a82e7ddf92c42148e113de91eacb9951ca1021771479b47be7`。
- 全仓 CTest 未运行：没有构建全部注册目标；这不是 PASS，也不报告为功能失败。历史 `bitpack_test` 全量构建障碍不用于替代本轮定向结果。
- 计数器 ON/OFF 校准：同一 TEST_ONLY 输入 `(n=64,K=8,score=0×64)`、algorithm tape `alternating`、seed `603299`、OMP=1；外置 deterministic `getrandom` shim seed `603299` 固定 shuffle/material 随机带，三次每组 `edge_fnv64`、逻辑/唯一边、发送字节、DCF Eval 和 PRG 次数逐次完全一致。按每次 P0/P1 party API critical path 的较大值统计，ON 为 `39.116/42.219/39.514 ms`，median `39.514 ms`；OFF 为 `36.364/35.970/34.789 ms`，median `35.970 ms`；该小样本观测差为 `+9.85%`。此夹具是单进程测试驱动、进程内存材料与 Unix socketpair，不含 T/mTLS/磁盘/网络，计数开销数值仅作研究诊断，不可外推正式 runtime。ON/OFF 日志 SHA-256 分别为 `5aeab4fd196edbe936583e923f5da3b03574785d7d099e69412cb9733c45a662`、`905be5991f518dec7ff84422c50312a913bf3de4ff723dcc0a7f6b83f6d92fc9`；随机 shim 源/so SHA-256 为 `8f6dad1009d9eeb982f67a1c26b7b8ecdd7c2969c1fbe7a036cf8935f4f515e6`、`dbdac74f0a957a2c4fea1a4f6e3e582dbeda30292314b4f341b6e688ac44d182d`。counter ON/OFF test binary SHA-256 分别为 `a84765722a9ee6d2732b25e5359503adb333bdb10560e21917af31a9c0535a20` 和 `b29d2ec79024399c2892a3e100027368188b3364f106f49d3ad7e5f7fc6ed1bc`。

本次实际 binary hashes：party-node `e895cff05e85a18d314b1fcbc2c41226fbc99342952de380253863afbcf56624`；
E2E harness `19ae7cf76ebd28d8e15747d6ec9a26560f98fffe4a2444c513afe831bfc56290`；party test
`a84765722a9ee6d2732b25e5359503adb333bdb10560e21917af31a9c0535a20`。计量构建 `CMakeCache.txt`
SHA-256：counter ON `3f1b5c1657521d15b304f5c796558bf88860153199f4fec4dcb929dee991fc81`，counter OFF
`5bab91fc0b52f9ecfa2c57666759fcff5266651d2d405533988f490d78077ac7`；default-off
`8345185a7020e9aefc3d57979fa5504a40458ac6de3598ee482ab5492d8f949d`。所有二进制、输入、share、密钥、证书、
sidecar、逐次 stdout/stderr 与 JSON records 均在仓库外。报告/决策/hash 索引通过后续提交记录。

最终文档整理后的复核：Ubuntu-24.04 WSL 中重新配置 Release 计量构建到
`/tmp/m6b-s32-final-build`，按上方 CMake feature flags 构建 party-node、BMW16 E2E/party/v2
conformance、DCF/priority-DCF conformance 与 Protocol III 三进程目标；构建成功。随后命令
`ctest --test-dir /tmp/m6b-s32-final-build -R "moe_topk_bmw16|moe_topk_m1_dcf|moe_topk_m2_priority_dcf|moe_topk_m3_protocol_iii_three_process" --output-on-failure`
结果 **6/6 PASS**；Python auditor `python3 -m unittest VFSS.tests.bench.test_bmw16_run_record -v`
结果 **10/10 PASS**；schema JSON parse 与 `git diff --check` 通过。重建 party-node、E2E harness、party test
SHA-256 分别仍为 `e895cff05e85a18d314b1fcbc2c41226fbc99342952de380253863afbcf56624`、
`19ae7cf76ebd28d8e15747d6ec9a26560f98fffe4a2444c513afe831bfc56290`、
`a84765722a9ee6d2732b25e5359503adb333bdb10560e21917af31a9c0535a20`。该次复核没有重新生成 n=1000 材料；
该复核 CMakeCache SHA-256 为 `cb0848a35e5f57b1fe02855e0c5de55df0b77f7f2b21c6ec25104b8fcdab92f7`。
前表 n=1000 仍是 runtime commit `f2db098...` 上本任务此前保存的独立 fresh-material 诊断。全仓 CTest 仍 NOT_RUN。

最终复核时第一次从 PowerShell 直接调用 WSL，含竖线的 CTest regex 被外层 shell 拆成多个命令，测试未启动；随后改用显式 `bash -lc` 引号重跑，才得到上述 6/6。该次调用错误不是测试失败，也未纳入测试分母。

另以所有 BMW16 选项与 `BUILD_TESTING` OFF 在 `/tmp/m6b-s32-final-off` 重新配置并构建 `sytorch`，
构建成功；`cmake --build /tmp/m6b-s32-final-off --target help` 未列出 BMW16 party、E2E 或 conditional
入口。该次 `libsytorch.a` SHA-256 为 `a2a9bb816a1cd7a82e7ddf92c42148e113de91eacb9951ca1021771479b47be7`；
当前构建目录专属 CMakeCache SHA-256 为 `441f4af60b442bea62da02845bdf30ba53d3b896f90a3c5a8ee0e6c7750eea51`。

## 同路线可比性、PR 与下一阶段

| 路线 | S32 schema-v2 同定义计数器 | 同输入/同网络/同边界可对照 | 当前状态 |
|---|---|---|---|
| Protocol I 全对全 | 未在本 revision/runtime 接入 | 否 | NOT_MEASURED / NOT_COMPARABLE |
| Protocol I + AAV86 | E15/E17/E20 历史数据口径/源码 revision 不同 | 否 | 历史数字不得拼接；需 schema-v2 同组重跑 |
| Protocol I + BMW16-derived Select + DCF | 本报告工具实际采集 | 尚无同期全对全/AAV86 配对运行 | EXPERIMENTAL_DIAGNOSTIC only |

Git 远端状态：S32 首次文档提交后，通过普通非强制快进将 PR 源分支
`codex/m6b-i-bmw16-s26` 从 `a1a90cb2...` 推至 `d5489ca1be132399e7420bfce4b067bda9feb871`；修正 PR 状态说明后，
又以非强制快进推送报告专用提交。S32 归档复跑前最近一次 `git ls-remote` 核对时该分支与
`refs/pull/29/head` 均为 `b446061478b76b32342964a61ce672dc7195e0c3`，`main` 仍为
`c3926c68fd14f270faa8b55234311071947fa080`。PR #29 仍 Open、base `main`，公开页显示 26 commits；标题仍是旧 S26
标题，正文仍为未填写 S32 结果的仓库模板。Checks 页没有显示已配置/已运行的 GitHub Actions 检查。
GitHub PR 元数据连接器返回 `USER_NOT_LOGGED_IN`，GitHub CLI 未安装，浏览器页面也未登录；本轮结束前再次调用 `github_fetch_pr`，连接器仍返回 `USER_NOT_LOGGED_IN`，因此未改 PR 标题/正文。
分支代码和 S32 文档已推送，但 PR 元数据未更新，也没有合入。取得 GitHub 元数据编辑登录后，只需更新标题/正文，
不需要重写提交历史或再推送；不 force-push、不合并：

建议标题：`Protocol I BMW16-derived Select + DCF: conditional n≤1000 candidate and S32 metrics`

以下正文可直接用于 PR #29：

```markdown
## Summary

This PR packages the **Protocol I + BMW16-derived Select + DCF** route as a
`PROJECT_DERIVED` candidate and adds schema-v2 instrumentation for its
diagnostic runs. It is not a reproduction of the original BB90 algorithm or a
line-by-line implementation of BMW16 Algorithm 7, and it does not inherit
BMW16's comparison constants or probability theorem.

The conditional-security design accepted in S31 applies only under its stated
assumptions: the source-specific G126/restricted AES-key PRG/PRP assumption,
AES-CTR root streams, OS CSPRNG, ROM sampler, a trusted offline and non-colluding
T, at most one semi-honest online-party corruption, an ideal private
authenticated channel, and the documented leakage function L. This is a
conditional candidate, not an unconditional or malicious-secure claim and not
a claim that the complete raw-score-to-mask protocol takes four rounds.

## S32 diagnostic evidence

The current S32 report records one fresh-material loopback mTLS end-to-end run
for each of n=8/K=3, n=128/K=8, n=256/K=80, and n=1000/K=80. Each SUCCESS
reconstructed in the isolated TEST_ONLY collector matched the frozen oracle,
had output length n, and had mask weight K. These are diagnostic observations,
not the formal V4 LAN/WAN matrix. The observed natural-abort count was 0/4;
this is not a probability bound.

Schema-v2 records the nine planned metrics with provenance and separates
application bytes, package bytes, payload bits, DCF Eval, PRG expansions, AES
blocks, logical comparison calls, and unordered comparison edges. The report
includes per-attempt hashes and the audit command.

## Validation

- BMW16 run-record auditor, party path, conditional-v2 conformance, DCF and
  priority-DCF conformance, and Protocol III three-process target: 6/6 CTest
  tests passed in the targeted build.
- Python run-record tests: 10/10 passed.
- S4/C++ common-tape differential: 39/39 passed.
- Related Protocol I/III targeted regressions: 13/13 passed.
- Default-off build: passed; BMW16 experiment targets were absent.
- Full-repository CTest: NOT RUN (not all registered targets were built).
- Same-run Protocol I all-pairs and Protocol I+AAV86 comparison: NOT RUN.
- Formal V4 LAN/WAN warm-up plus five repetitions and six-scheme ranking: NOT
  RUN; V3 prerequisite remains in force.

The PR remains `PROJECT_DERIVED`; this diagnostic report does not upgrade the
security scope or mark formal performance acceptance complete.
```

S32 不运行正式 V4 1+5 LAN/WAN 矩阵。

后续服务器任务只有在 V3 前置获正式关闭后启动：冻结 schema-v2、最终源码/二进制、相同输入/oracle、线程和
计时屏障；三条 Protocol I 路线按同一 metric layer 重跑，fresh materials，LAN/WAN 每配置 1 warm-up + 5
formal repetitions，所有 abort/失败保留。租机前重新 checked 总磁盘（含至少 6 次约 10 GB 双方包的空间及证据
余量）、RAM/CPU 与时限；本机 WSL 诊断不替代网络校准。n≥10^4 继续仅做 checked 容量评估。
