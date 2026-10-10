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

| n,K | 输入 seed / session | 状态与正确性 | offline ms | material payload bits | package bytes | online ms | app comm total / per party bits | rounds | online PRG calls | comparison edges | total ms | raw evidence manifest SHA-256 |
|---|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| 8,3 | 603201 / 6032001 | SUCCESS, oracle exact, weight K | 78.451 | 3,967,968 | 505,018 | 1,141.160 | 50,192 / 25,096 | 12 | 15,568 | 109 | 1,219.611 | `a6a6873a7c90d6b22105bc835e16c4710affffcdc6ab865891de634fbad207c9` |
| 128,8 | 603202 / 6032002 | SUCCESS, oracle exact, weight K | 2,003.618 | 1,194,366,240 | 151,637,602 | 1,261.173 | 5,344,272 / 2,672,136 | 12 | 1,846,816 | 11,581 | 3,264.792 | `5cc89a9349f5884eb14142fac88a984ba34755f82659c768cc33b245728e0834` |
| 256,80 | 603203 / 6032003 | SUCCESS, oracle exact, weight K | 6,909.491 | 4,900,938,016 | 622,018,530 | 1,944.843 | 8,335,120 / 4,167,560 | 12 | 7,071,612 | 43,207 | 8,854.335 | `3be86eb29089fb118b893e9f91ca74037393cf28eb0f698b8c91cd9f3e6dbfe3` |
| 1000,80 | 603204 / 6032004 | SUCCESS, oracle exact, weight K | 581,921.123 | 78,361,809,376 | 9,939,083,130 | 54,753.717 | 48,047,120 / 24,023,560 | 12 | 19,688,900 | 114,899 | 636,674.840 | `d9ae4cbcf127c4ce32b21cede0dc1c199f6680d81460752336a91ee622eea675` |

四个 `run_record.json` 本身 SHA-256（对应上表顺序）为：
`cd88650606c3f6c546779a1fe80cf8aec1a663194a6cbae6f3315eaed84ef787`、
`7a72222d3d36e8d068425cbf71c365e8e12e0f1ba30db0acdf339d72d68b09e6`、
`fe7979b635e90ef6cd73e129dfd365d1e9b14e3251883f23839de861f3920e98`、
`851219d9f55d89f503c4dee8c14eb8513e6b5273c42abd99eaf0454b3375f689`。

说明：package bytes = 两方 sidecar ciphertext 文件 + 两方 shell 文件；TLS app delivery 为辅助值，不等于 TLS wire。
四次是每配置单次诊断，不是预热+5重复；自然算法 abort 观察为 `0/4`，注入 abort 计数也是 `0/4`，不据此
推导理论失败率。n=1000 本机试跑约 10.61 分钟，其中 offline 9.70 分钟；不得当 LAN/WAN 结论。

独立边/计数对账：例如 n=1000：raw 2,048 uCMP calls（2,000 real + 48 pad）+ Select 111,852
 unique real edges + membership 999 = 114,899；DCF Eval 两方合计 459,596，PRG node expansions
 两方合计 19,688,900，AES blocks 辅助数 39,377,800。上述量纲相互独立，不可互换。

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
又以非强制快进推送报告专用提交。最近一次 `git ls-remote` 核对时该分支与 `refs/pull/29/head` 均为
`12eb42ea753a660b9755c1e269ce17619c2b40b4`，`main` 仍为
`c3926c68fd14f270faa8b55234311071947fa080`。PR #29 仍 Open、base `main`，公开页显示 26 commits；标题仍是旧 S26
标题，正文仍为未填写 S32 结果的仓库模板。Checks 页没有显示已配置/已运行的 GitHub Actions 检查。
GitHub PR 元数据连接器返回 `USER_NOT_LOGGED_IN`，GitHub CLI 未安装，浏览器页面也未登录；因此本轮无法改 PR 标题/正文。
分支代码和 S32 文档已推送，但 PR 元数据未更新，也没有合入。取得 GitHub 元数据编辑登录后，只需更新标题/正文，
不需要重写提交历史或再推送；不 force-push、不合并：

建议标题：`Protocol I BMW16-derived Select + DCF: conditional n≤1000 candidate and S32 metrics`

建议正文重点：PROJECT_DERIVED；S31 条件安全接收适用 n≤1000，假设为 G126/restricted AES 子族、AES-CTR
root stream、OS CSPRNG、ROM sampler、可信离线静默 T、至多一个半诚实腐化方、理想私有认证通道及明示
leakage L；非 BB90 原算法、非 BMW16 Algorithm 7 逐字实现，不继承定理常数/概率。报告 S32 schema-v2
九指标和 n=8/128/256/1000 各一次成功 loopback mTLS oracle 诊断；说明 `0/4` 自然 abort 只是观察，
不是概率界。列出本轮 CTest 6/6（含 S32 auditor、party、v2 conformance、DCF 与 Protocol III 三进程目标）、
common tape 39/39、Protocol I/III 定向 13/13；明确全仓 CTest、同期全对全/
I+AAV86 配对、TLS wire/RSS/root PRG、正式 V4 LAN/WAN 1+5 与六方案排名未完成/NOT_MEASURED。V3 前置
保持不变。

S32 不运行正式 V4 1+5 LAN/WAN 矩阵。

后续服务器任务只有在 V3 前置获正式关闭后启动：冻结 schema-v2、最终源码/二进制、相同输入/oracle、线程和
计时屏障；三条 Protocol I 路线按同一 metric layer 重跑，fresh materials，LAN/WAN 每配置 1 warm-up + 5
formal repetitions，所有 abort/失败保留。租机前重新 checked 总磁盘（含至少 6 次约 10 GB 双方包的空间及证据
余量）、RAM/CPU 与时限；本机 WSL 诊断不替代网络校准。n≥10^4 继续仅做 checked 容量评估。
