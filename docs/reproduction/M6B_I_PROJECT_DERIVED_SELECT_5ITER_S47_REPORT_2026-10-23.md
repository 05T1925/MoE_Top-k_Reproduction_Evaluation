# M6B-I-PROJECT-SELECT-5ITER S47 详细报告

日期：2026-10-23
路线：**Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL**

目标功能：输入原始顺序的 signed Q20.12 score；按分数降序、同分按原索引升序稳定排序；输出原序、恰有 K 个 1 的两方 XOR 共享 Top-K mask。

这是一条项目派生的五迭代 Select 路线。它不是 BB90 原算法复现、BMW16 实现或 BB90+DCF 性能结果。五次算法迭代、比较依赖层、应用 exchange phase 和真实网络 RTT 是不同指标。

## 1. 结论摘要

- S46 固定源码上的 F_T/v3 显式 opt-in 候选已实现。默认关闭；旧 F_T/v1/v2 的 N≤256 限制、包格式及采样路径不自动升级，也不发生隐式回退。
- 对 padded `m=1024` 的资源推导和本机预检通过。fresh Release 多进程 E2E 的 N=1000、K={1,8,500,999,1000} 五案全部退出成功并通过冻结 oracle；另有 N=1 profile、FTv3 N=128/256、FTv1 N=128 和 FTv2 N=256 兼容 smoke。功能接收只为这些指定案例 `PASS_WITH_SCOPE`，不是全 K/全输入矩阵。
- N=1000 的每方 package 为 33,366,516 B；最大实测 frame payload 为 1,048,572 B；每方峰值 RSS 942,312–981,868 KiB。每个保留的 N=1000 E2E 临时目录占用 66,794,349 B，其中 package 双份共 66,733,032 B，其余是上下文、escrow 数据及运行时文件。该目录大小是本地临时文件观察，不是磁盘性能指标。
- retry cap/profile mismatch 经非作者审查后已统一：生产和 TEST_ONLY issuer 均固定签入 cap 7，package/context parser 要求 7 并验证 `r_min<=7`。非作者只读复核通过范围为规格、概率、版本绑定和实现映射；审查者没有构建或测试。
- 低概率 `ISSUANCE_ABORT` 产品接受仍为 `OPEN_CONFIRMATION`；目标部署 DRBG、联合随机流、私有信道、完整 party-view 安全仍为 OPEN。没有 secure alias；N>1000 未准入；正式 LAN/WAN、RTT、性能和 AAV86 对比未运行。

## 2. 固定对象、来源和参与关系

本轮从隔离 worktree `C:\Users\28641\.codex\worktrees\m6b-i-project-select5-s47\MoE_Top-k_Reproduction_Evaluation` 开始，branch `codex/m6b-i-project-select5-s47`。基线 commit/tree/parent 分别为：

```text
commit d48a41062ba2f86edbac3bcb3663692a641e95bd
tree   3f70ae5999400781f37f871420a26988afa146c9
parent 6fd6bed0718be28d677892ca40125d0f52323f0e
```

起始 worktree clean。S46 manifest 是本轮来源门：其 raw SHA-256 为 `f19f8be05b590b8c3e4c888a513e9b37540d80ff3de6f712f99e8f575d0b8438`。S46 报告、决策、gates、cases 的 Git blob 与 S46 manifest 中列出的身份匹配；Windows checkout 下 S46 报告/决策的 raw SHA 与旧记录不同，但 `git hash-object --path` 对应 blob 一致。S46 已记载的 S45 85 项来源身份与唯一 S46 E2E harness 增量继续沿用；本轮 source manifest 另固定 S46 exact Git tree 和所有 S47 工作树差异哈希，没有从桌面主工作区、main 或旧 S35 快照拼接代码。S46 的原始记录未改写。

本轮执行者参与此前 S45/S46 路线实现、审查或推进，不将自己称为非作者接收者。非作者 `/root/s45_non_author_review` 未参与 S47 实现或推导；它只读核对 FTv3 retry cap、reject bound、package/context 字段及解析、receipt 版本分离、N=1 测试和 TEST_ONLY gate。首轮指出 TEST_ONLY issuer 仍写入动态 `r_min` 的不一致；改为固定 cap 7 后完成第二次只读复核，结论 `PASS_WITH_SCOPE`。Reviewer 未运行构建、测试或资源实验，因此独立运行证据由本轮执行者的日志、二进制和 case records 支撑，而不是 reviewer 自测。

S46 收据只覆盖 FTv2 的 N=128/256 十个案例；S40 的 211 案是更早 FT/receipt 配置。本轮不把任一历史结果改写为 FTv3 或 N=1000 证据，也没有重跑历史矩阵。

## 3. 资源模型与首个边界检查

### 3.1 源码层上限与分层变更

S46 原路径在算法 admission、bounded sampler、package ceiling 和 gate budget 上都限制到 N≤256 / padded `m≤256`；其 package ceiling 为每包 2 MiB、gate ceiling 为 5,281,664。普通 AND opening frame 也不能容纳 N=1000 的最大路由层。S47 没有把这些常量全局放大，而是增加 FTv3 专用路径：

- 仅 FTv3 budget/sampler/control 接受实际 N≤1000、padded `m≤1024`；FTv1/v2 的 budget 与入口仍拒绝 N>256。
- FTv3 package format 6、context format 2、receipt format 2 分字段绑定。FTv3 每方 package cap 为 64 MiB，仍小于此处 33.4 MB 的包；escrow 输出密文 cap 保持 2 MiB，因为它存的是加密输出份额，不是 Dealer package。
- 单个协议 transport frame 仍不超过 1 MiB。FTv3 将大 AND layer 按固定最大 gate 数拆成多批，schedule digest 绑定 batch 顺序、frame profile 和 phase 上界；FTv1/v2 旧 schedule/hash 不变。
- Gate 数和材料长度适配到 64-bit 计数/u32 字节字段；N=1000 的实际数值均未越过字段范围。SQLite/lease 流程没有遇到本轮目标大小瓶颈。

因此，原来的算法/采样/包/单帧上限是本轮需要版本化绕过的源码边界，不是本机 RAM、磁盘或地址空间故障。修正限定于 FTv3 版本，未静默改变旧版本承诺。

### 3.2 公式推导

`N=1000` 按源码 padding 到 `m=1024`，索引宽 `h=10`，key 宽 `W=42`，全 pair 数 `P=m(m−1)/2=523,776`。门数由实际预算公式得：

```text
G = 63m + m²W + P(2W−1) + m²
  = 88,626,688 AND gates.
```

每方预处理包材料字节：

```text
triple shares       = ceil(3G/8) = 33,235,008 B
permutation shares = ceil(m²/8) =    131,072 B
payload/party                       33,366,080 B
header + signature                    436 B
package/party                     33,366,516 B
both packages                     66,733,032 B
```

双包加起来低于单包 64 MiB 上限的两倍。计算不包括 context、SQLite WAL/SHM、目录分配簇和诊断输出。FTv3 总随机请求上界为：

```text
rho_i = (2^32 mod i)/2^32 <= 1023/2^32 < 2^-22
r=7, positions<=1023
delta_reject <= 1023 * (2^-22)^7
             = 1023 * 2^-154 < 2^-144 < 2^-128
Qmax = G + ceil(m²/8) + 4r(m−1)
     = 88,786,404 B
Cmax = ceil(G/2^20) + 1 + r(m−1)
     = 7,247 RAND_bytes calls
single request <= 1,048,576 B
```

`delta_reject` 是理想 IID 32-bit words 下的发材 abort 概率界，只应用于去除版本/编码/签名字段后的语义材料投影；它不是完整 package 或协议 view 的距离界，也不替代 `Adv_DRBG_T(Qmax,Cmax;E)`。

### 3.3 内存、frame 与 exchange phase 预算

最大的前向路由层为 `m²W=44,040,192` gates，比较生成层为 `21,998,592` gates。最大每帧 gate batch 为 4,193,824 gates；连同 104 B inner prefix、12 B batch metadata、4 B packed count 和 2-bit/gate opening payload 后，frame 上限为 1 MiB，每次 E2E 最大观察 payload 为 1,048,572 B。路由层分成最多 11 批；比较生成层分成最多 6 批。

按源码层对象合计的规划峰值估算约 1,061,122,036 B（约 1,012 MiB/party），其中最大 gate-input vector 是主项，并另加一层临时 Beaver/result buffers、1 MiB frame 和 25% 最大层嵌套 vector reserve。25% 是规划余量，不是数学证明的分配上界。最大成功路径为 106 个应用 exchange phases：固定 GMW core 91（含 READY）、Select5 最多 13、inverse 1、output 1；五次算法迭代不等于 13 个 Select phase。所有这些是 phase/frame/resource 记账，不是 RTT 或正式性能数据。

## 4. 实验环境、构建与版本隔离

S47 final run 使用 WSL Ubuntu 24.04.4、Linux kernel `6.6.87.2-microsoft-standard-WSL2`、GCC 13.3.0、CMake 3.28.3、Ninja 1.11.1、OpenSSL 3.0.13。SQLite 开发头从 `libsqlite3-dev=3.45.1-1ubuntu2` 临时解包到 `/tmp`；链接 system SQLite 3.45.1。依赖辅助文件、build、二进制、日志、测试密钥/证书/材料均留在 `/tmp`，没有入库。

预检时 WSL 可用 RAM 约 7.5 GB、swap 2 GiB、`/tmp` 空闲约 983.9 GB、Windows 盘空闲约 80.8 GB，32 个逻辑 CPU。没有有限 `ulimit -v` 或 file-size ceiling。完整 N=1000 测试时每方 2 GiB TEST_ONLY RSS cap 未触发。结果没有因本机资源不足停止，也没有缩小 N 冒充目标测试。

本机资源满足本轮运行要求，所以没有转交统一服务器复测。若后续在统一服务器复现，建议预留至少 8 GiB RAM、10 GB 空闲工作盘和 4 个逻辑 CPU；这是减少并发/allocator 压力的规划建议，不是源码最低需求或性能准入标准。

| 验证 | 配置/结果 |
|---|---|
| fresh Release default-OFF | `BUILD_TESTING=OFF`，target help 无 v3/FTv3 target，`libsytorch.a` 无 v3/FTv3 symbol；SHA-256 `4da54879f18f681ccef5ef93329b68138391bbdda20473faa19e345838f6c920` |
| 头文件无 opt-in probe | `score_to_mask_v3_gmw.h` 按预期以 `#error "score-to-mask-v3 is opt-in and must be built explicitly"` 拒绝 |
| invalid opt-in: FTv3 without GMW | CMake 以 `F_T/v3 requires GMW/v3 and receipt/v2 opt-ins` 拒绝 |
| invalid opt-in: FTv3 without receipt/v2 | 相同规格错误，按预期拒绝 |
| fresh Release TEST_ONLY | GMW、receipt/v2、FTv2、FTv3 显式 ON，`BUILD_TESTING=ON`；sampler、runtime 和 FTv3/compat E2E targets 成功构建 |
| fresh Release production FTv3 | GMW、receipt/v2、FTv3 ON，FTv2 OFF，`BUILD_TESTING=OFF`；生产 archive SHA-256 `3de5d97d0f9460a27d6a7afa999a93ca28d501e3f13105d3192d60d4c63afdf1`；无 `with_test_random` symbol |

default-OFF 检查是构建隔离证据，不是安全证明。FTv3 TEST_ONLY 随机源只在 testing build target 中注册；生产构建没有该 hook。

## 5. 定向 conformance 与独立进程结果

最终 FTv3 sampler conformance 输出：

```text
S47_FT_V3_BOUNDED_CONFORMANCE PASS gates=88626688 Qmax=88786404 Cmax=7247 issuance_abort=PASS
```

它覆盖直接 sampler 的 m=1 零调用、m=2/3/1023/1024 接受/拒绝边界、拒绝后接受、最多 7 次后统一 abort、随机 bytes/call/max-request 上限、m=1025 拒绝、N=1000 精确资源公式、legacy N=1000 拒绝、包/context 的固定 cap=7 与 FT/sampler/package/receipt 分离，以及错误版本 tuple 拒绝。强制中止只保留 T 私有 burn marker，测试直接断言 party package/context 不存在，重复 invocation 被拒绝。该测试不会启动双方进程，因此没有单独读取 escrow DB 来断言 lease 表状态；由于 package 未发布，party/READY/lease 流程没有被触发。该证据不声称观察到自然故障。

运行时 conformance 的最终输出：

```text
S32_V3_RUNTIME_CONFORMANCE {"package_signature":"PASS","triple_shares":"PASS","permutation_shares":"PASS","one_shot_lease":"PASS","package_faults":"PASS","N128_formula":1289280,"N256_formula":5281664}
```

最终 FTv3 E2E 二进制 SHA-256 为 `b01c46dd787bca7d310f60b1de3ae75f1dfdf1f97f754d6115182a9a17ce4c97`。最终运行输出中 `S47_FT_V3_TARGETED_CASES PASS cases=10`，进程结束码 0。每案新 session、invocation、Dealer packages、one-shot lease 和 escrow 状态；全 10 案 oracle PASS，双方进程 exit 0，提交后恢复 PASS。

| Case | F_T / receipt | N/K | Gates | Package / party | Max payload | Phases | Peak RSS P0/P1 KiB |
|---|---|---:|---:|---:|---:|---:|---:|
| `s47-ft-v3-n1000-k1` | 3 / 2 | 1000/1 | 88,626,688 | 33,366,516 B | 1,048,572 B | 96 | 942,956 / 942,700 |
| `s47-ft-v3-n1000-k8` | 3 / 2 | 1000/8 | 88,626,688 | 33,366,516 B | 1,048,572 B | 106 | 942,956 / 942,444 |
| `s47-ft-v3-n1000-k500` | 3 / 2 | 1000/500 | 88,626,688 | 33,366,516 B | 1,048,572 B | 106 | 981,868 / 981,612 |
| `s47-ft-v3-n1000-k999` | 3 / 2 | 1000/999 | 88,626,688 | 33,366,516 B | 1,048,572 B | 106 | 942,956 / 942,700 |
| `s47-ft-v3-n1000-k1000` | 3 / 2 | 1000/1000 | 88,626,688 | 33,366,516 B | 1,048,572 B | 106 | 942,312 / 942,080 |
| `s47-ft-v3-n1-k1` | 3 / 2 | 1/1 | 327 | 560 B | 149 B | 72 | 34,400 / 34,400 |
| `s47-ft-v3-n128-k64` | 3 / 2 | 128/64 | 1,289,280 | 485,964 B | 159,860 B | 88 | 34,552 / 34,552 |
| `s47-ft-v3-n256-k128` | 3 / 2 | 256/128 | 5,281,664 | 1,989,252 B | 655,476 B | 89 | 72,372 / 72,376 |
| `s47-legacy-ft-v1-n128-k64` | 1 / 1 | 128/64 | 1,289,280 | 485,964 B | 159,852 B | 88 | 34,552 / 34,552 |
| `s47-legacy-ft-v2-n256-k128` | 2 / 2 | 256/128 | 5,281,664 | 1,989,252 B | 655,468 B | 89 | 72,376 / 72,372 |

The deterministic E2E input repeats every 10 scores:
`[INT32_MAX, INT32_MIN, -1, 0, 17, INT32_MAX, -17, 4096, -4096, 17]`.
It exercises signed extremes, negative/zero/positive values and stable ties.
The harness compares reconstructed shares to the frozen oracle and asserts
original-order output with exactly K ones. A phase count is an application
exchange count only; it is not the number of Select iterations or measured
network RTT. `diagnostic_elapsed_ms` is retained in cases but is not a
performance result.

For each retained N=1000 case directory, `du -sb` observed 66,794,349 B total;
this includes package pair 66,733,032 B and approximately 61,317 B of local
context/database/runtime files. The E2E harness stops service processes after
checks, so service exit 143 is expected SIGTERM cleanup; Dealer/P0/P1 all
reported exit 0. Raw full stdout/stderr and process logs remain in `/tmp` and
are excluded from git; case JSON contains hashes, compact results and raw-log
paths.

## 6. 命令与失败尝试

构建和运行使用 `/tmp/m6b-s47-*` 下的隔离目录。核心命令：

```bash
cmake -S "$SRC/VFSS" -B /tmp/m6b-s47-default-off -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build /tmp/m6b-s47-default-off --target sytorch --parallel 4

cmake -S "$SRC/VFSS" -B /tmp/m6b-s47-ftv3-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_SCORE_TO_MASK_V3_GMW_EXPERIMENTAL=ON \
  -DMOE_TOPK_ENABLE_SCORE_TO_MASK_V3_RECEIPT_V2_EXPERIMENTAL=ON \
  -DMOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL=ON \
  -DMOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V3_BOUNDED_EXPERIMENTAL=ON \
  -DSQLite3_INCLUDE_DIR=/tmp/m6b-s47-sqlite-dev/usr/include \
  -DSQLite3_LIBRARY=/usr/lib/x86_64-linux-gnu/libsqlite3.so.0
cmake --build /tmp/m6b-s47-ftv3-build --target \
  moe_topk_score_to_mask_v3_ft_v3_bounded_conformance_test \
  moe_topk_score_to_mask_v3_runtime_conformance_test \
  moe_topk_score_to_mask_v3_process_e2e_test \
  moe_topk_score_to_mask_v3_ft_v3_process_e2e_test \
  moe_topk_score_to_mask_v3_ft_v2_process_e2e_test --parallel 4
./moe_topk_score_to_mask_v3_ft_v3_bounded_conformance_test
./moe_topk_score_to_mask_v3_runtime_conformance_test
S35_KEEP_TEST_TMP=1 ./moe_topk_score_to_mask_v3_ft_v3_process_e2e_test --ft-v3-n1000
```

此外另做 `BUILD_TESTING=OFF`、FTv3=ON、FTv2=OFF 的生产 Release 构建，确认无 TEST_ONLY hook；做了 FTv3 without GMW 和 without receipt/v2 两个 expected-negative CMake configure；无宏头文件 probe 按预期编译拒绝。命令结果、exit 和 archive/binary hashes 见 manifest。

保留的非代码缺陷尝试：Ubuntu 20.04 的 CMake 3.16.3 低于项目要求 3.17，fresh configure 在环境阶段停止；换到满足要求的 Ubuntu 24.04 构建。第一次 build 指向不存在的临时目标名，列举 CMake targets 后改用真实 target 名称。首次 FTv3 conformance compile 使用了错误 private-header include，修为公开 include-root 下的规范路径后通过。这些均不是 N=1000 资源或算法失败。非作者审查发现 TEST_ONLY issuer profile 与生产 issuer 不一致；统一为固定 cap 7 后由同一 reviewer 复核，最终 conformance 和十案 E2E 已在修正后的二进制上重跑。没有隐瞒或丢弃已观测失败。

## 7. 安全、合同和性能边界

F_T/v3 成功验证只证明所测运行可构建、发材、执行并匹配冻结功能 oracle。`ISSUANCE_ABORT` 耗尽时 party package/context 不存在，T 私有 burn marker 保留；因为发材没有发布包，party、READY 和 party lease 请求不会开始，但本轮没有单独启动 party 并查询其 lease 数据库。临时材料向量并无零化保证。合法输入的低概率预处理中止尚未被项目负责人明确接受，故 `LOW_PROBABILITY_ISSUANCE_ABORT_ACCEPTANCE=OPEN_CONFIRMATION`；没有私下补充产品决定。

S46 的 provider/熵源/DRBG lifecycle 信息只描述当时构建机。S47 的 OpenSSL 3.0.13 也是 build-host observation，不是目标部署 profile。有限 `Qmax/Cmax` 只给 API 调用量/每次请求字节上界，不是返回时间界；成功条件化和真实 provider 的随机流仍需 `Adv_DRBG_T(Qmax,Cmax;E)`。`delta_reject`、DRBG 优势、签名优势和 TLS/私有信道优势保持为不同项；本轮没有合并成“安全位数”。

正常多进程本地 E2E 成功；自然业务读写故障沿用 S40 `NOT_REPRODUCED`，本轮未注入也未重新尝试。人工拒绝采样 exhaustion 是定向随机源故障，和自然 I/O 错误分开。任意 crash-prefix 隐私、跨 DB 掉电耐久、A-only 腐化和完整 party-view 均未获证明。

正式 LAN/WAN 矩阵、RTT、吞吐、延迟基准和 AAV86 对比均 `NOT_RUN/NOT_MEASURED`。本机 RSS、包大小、帧数和 phase 只用于本任务的资源/功能准入，不是正式性能结果。M6A 的 V3→V4 前置和仓库性能准入规则仍适用。

## 8. 门禁状态与后续工作

本轮机器门禁分开记录在 S47 gates JSON。重点如下：

| Gate | 状态 |
|---|---|
| `N1000_RESOURCE_PREFLIGHT` | `PASS_WITH_SCOPE` |
| `FT_VERSIONED_SAMPLER_SPEC` | `PASS_WITH_SCOPE` |
| `FT1000_FUNCTIONAL_E2E` | `PASS_WITH_SCOPE` |
| `LOW_PROBABILITY_ISSUANCE_ABORT_ACCEPTANCE` | `OPEN_CONFIRMATION` |
| `DRBG_DEPLOYMENT_PROFILE` / `JOINT_RNG_STREAM_BRIDGE` | `OPEN` |
| `PRIVATE_CHANNEL_BRIDGE` / `PARTY_VIEW_COMPOSITION` | `OPEN` |
| `SECURE_ALIAS_READY` | `NOT_READY` |
| `PERFORMANCE_READY` | `NOT_READY` |

下一步若继续准入，应先记录合法输入在随机发材 abort 下的项目合同接受，再由目标部署提供 libcrypto/provider、熵源、`OSSL_LIB_CTX`、线程/fork/restart/reseed 和全局调用量资料。正式性能需要另按统一服务器和 V3→V4 前置门申请。N>1000、secure alias 和正式性能未由 S47 批准。
