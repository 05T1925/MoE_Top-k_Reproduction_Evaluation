# M6B-I-PROJECT-SELECT-5ITER S46 详细报告

日期：2026-10-22
路线：**Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL**
功能目标：Q20.12 signed score 降序，原始索引升序稳定打破同分，输出原始顺序且恰有 K 个 1 的秘密共享 Top-K mask。

本路线是项目派生的五迭代稳定 Select。它不是 BB90 原算法复现、BMW16 实现或 BB90+DCF 性能结果。五次算法迭代、比较依赖层、应用 exchange phase 和网络 RTT 是不同量。

## 1. 范围、来源与接收角色

本轮从隔离 worktree 的 S45 commit `6fd6bed0718be28d677892ca40125d0f52323f0e` 开始，父提交为 `aaa7f96095c350e8236246163d48c54a7fb8d9ba`，tree 为 `a77ba90ccce3ac18c7a735f6617e47ab8f0aaa2f`。S45 runtime 来源仍是其固定提交对象；没有从 main、桌面主工作区或其他历史 worktree 拼接源码。S45 source/build/evidence manifest 的原始文件 SHA-256 与 S46 checkout 哈希、Git blob 身份和本轮 TEST_ONLY 差异见 manifest。

S45 manifest 的七个源增量逐项与 S45 Git tree 匹配。由于 Windows checkout 的 `core.autocrlf=true`，三个 S45 新增 LF 文本文件的 checkout raw hash 与当时 Linux raw SHA 不同，但 `git hash-object --path` 与 S45 commit blob 一致。其余未改源文件 raw hash 也匹配；E2E test harness 的 S45 基线 blob 与提交一致，S46 仅在该文件增加 48 行 TEST_ONLY 分支。S45 decision/report/gates/cases/manifest 的原记录哈希均在 S46 worktree 重算匹配。`VFSS-baseline/` 状态为空，没有改动。

执行者参与过 S43–S45 的审查/实现，不把自身审查称为独立接收。非作者 `/root/s45_non_author_review` 没有参与 S45 实现或采样规格推导，执行了全新 Release build 与测试；其本轮结论为 `PASS_WITH_SCOPE`。该 reviewer 对 S45 曾做只读检查，但此前没有运行构建或测试；S46 功能接收是本轮新建构建目录的直接观察。该 reviewer 不是对实际目标部署 DRBG 或完整协议 party-view 的密码学复核人。

证据类别：论文定义仅提供研究背景；F_T/v2 版本化 sampler 和五迭代 Select 属项目扩展；本报告的构建、源码、测试结果是本地观察；目标部署 RNG profile 和 DRBG 优势属于待证明假设。

## 2. S45 源码接收与本轮唯一源码差异

S45 固定 F_T/v2 的显式 opt-in 路径：package format 5，`ft_version=2`、`sampler_version=1`、`lambda=128`、`retry_limit=6`；F_T/v1 format 4 与旧 sampler 单独保留。escrow `receipt_format_version` 是独立签名 context 字段，可为 1 或 2。S46 没有修改 Dealer/runtime、版本字段、签名、序列化或材料格式。

本轮为覆盖目标规模 K 边界，仅在 `VFSS/tests/moe_topk/score_to_mask_v3_process_e2e_test.cpp` 增加 `--ft-v2-target-k-cases` TEST_ONLY 入口。入口在正确的 `moe_topk_score_to_mask_v3_ft_v2_process_e2e_test` target 中编译：每个 case 通过现有独立进程驱动创建新 session/invocation、fresh Dealer package 和 one-shot lease，并在 receipt v1/v2 间交替。每案以冻结 `top_k_mask` oracle 核对 XOR 重构、原序 mask 和恰好 K 个 1。

## 3. 独立 Release 构建与功能验证

接收者在 `/tmp/m6b-s46-review` 重新配置构建，没有复用 S45 二进制：

| 构建/探针 | 配置和断言 | 结果 / SHA-256 |
|---|---|---|
| default-OFF Release | `BUILD_TESTING=ON`，GMW、receipt/v2、F_T/v2 均 OFF；target 列表与 `libsytorch.a` 无 v3/F_Tv2 注册或符号 | PASS；archive `20afe504e050f37294352b5377fe4aa4a52904eeed3782d3e0149ffa79501351` |
| public API 关闭探针 | 无 GMW 宏时头文件不可用；只有 GMW 宏而无 FTv2 宏时 FTv2 issuance API 不声明 | 两项均按预期编译拒绝，exit 1 |
| 显式 FTv2 Release/TESTING | GMW、receipt/v2、FTv2 显式开启；sampler 与 runtime conformance 构建 | PASS；二进制见 cases/manifest |
| Release production | `BUILD_TESTING=OFF` 且显式开启 GMW、receipt/v2、FTv2；FTv2 正式 API 存在，`test_only`/`with_test_random`/`TEST_ONLY` 符号和 TESTING 宏不存在 | PASS；archive `094429a86d037b227f1670aaf7b6bd4c1cfb8422d8509c83a82611296c98edbd` |
| sampler + runtime conformance | bounded sampler 边界及 v3 package/triple/permutation/lease conformance | 2/2 PASS |
| F_T/v1 兼容 | N=4、K=2、F_T/v1 + receipt/v1；oracle、提交和重启恢复 | PASS |

Conformance 覆盖 S45 的 `m=1/2/3/255/256`、拒绝后接受、连续 6 次拒绝、尝试/字节/call cap、版本错配、receipt 版本分离及重复发材 burn marker。S46 重新运行了该定向 suite，但不声称这是完整算法矩阵。

### N=128/256 F_T/v2 多进程 E2E

正确的 S46 E2E 二进制 SHA-256 为 `c0512470a0fed791810ef4207c24f3181340fa8154e91e09094d6462069d8ef3`。10 案结果为：

| N | K | receipt | Dealer/P0/P1 | Oracle | escrow restart fetch |
|---:|---:|---|---:|---|---|
| 128 | 1 | v1 | 0/0/0 | PASS | PASS |
| 128 | 8 | v2 | 0/0/0 | PASS | PASS |
| 128 | 64 | v1 | 0/0/0 | PASS | PASS |
| 128 | 127 | v2 | 0/0/0 | PASS | PASS |
| 128 | 128 | v1 | 0/0/0 | PASS | PASS |
| 256 | 1 | v2 | 0/0/0 | PASS | PASS |
| 256 | 8 | v1 | 0/0/0 | PASS | PASS |
| 256 | 128 | v2 | 0/0/0 | PASS | PASS |
| 256 | 255 | v1 | 0/0/0 | PASS | PASS |
| 256 | 256 | v2 | 0/0/0 | PASS | PASS |

所有 case 都使用含重复同分、`INT32_MIN`、`INT32_MAX`、负值、0 和正值的确定性周期输入。每案独立 session/invocation、package 和 lease。服务及重启服务 exit 143 是 E2E harness 在 commit/fetch 检查完成后的预期 SIGTERM 清理（128+15），不是协议失败。每案唯一输出判断是 oracle、原序和恰 K 个 1；phase/RSS/elapsed 只留作原始诊断，不在此报告作性能结果。

受控 sampler exhaustion 另测 N=256、K=128：TEST_ONLY 字节流令一位置连续 6 次拒绝；Dealer exit 1，收到 `ISSUANCE_ABORT`，P0/P1 不启动，没有 package/context/party lease，T 私有 burn marker 留存。它是人工随机源故障注入，不是自然 I/O 错误。

### 一次误选 target 的失败尝试

接收者最初误用普通 `moe_topk_score_to_mask_v3_process_e2e_test --ft-v2-target-k-cases`。该目标没有 FTv2 TESTING hook，忽略新参数并启动旧 S40 风格 v1 suite。观察到第 73 条 `S40_V3_PROCESS_E2E` 行后立即 Ctrl-C，shell 调用退出码为 1；日志没有 S46 summary。普通 target SHA-256 为 `4889dde795876e9813c9ff5d52ef5de4e1c492ff70dc574cccd66735cd23b1a3`，中止日志 SHA-256 为 `c981d562c04bed477f571d94ebf87ad120b106b45577c6d636fddc14646d1453`。这 73 条只是被中止尝试，不计入 S46；没有再次启动普通 target，完整 S35 195 案和 S40 211 案均未运行。错误日志与正确日志都只保留在 `/tmp`。结束时没有残留协议子进程。

## 3.1 可复跑命令

接收者使用 source worktree `/mnt/c/Users/28641/.codex/worktrees/m6b-i-project-select5-s46/MoE_Top-k_Reproduction_Evaluation`，所有 build/log 留在 `/tmp/m6b-s46-review`。完整 argv、退出码和 command-results index hash 见 manifest。核心调用如下：

```bash
cmake -S "$SRC/VFSS" -B /tmp/m6b-s46-review/default-off -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_SCORE_TO_MASK_V3_GMW_EXPERIMENTAL=OFF -DMOE_TOPK_ENABLE_SCORE_TO_MASK_V3_RECEIPT_V2_EXPERIMENTAL=OFF -DMOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL=OFF
cmake --build /tmp/m6b-s46-review/default-off --target sytorch --parallel 4
cmake -S "$SRC/VFSS" -B /tmp/m6b-s46-review/ft-v2-tests -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DMOE_TOPK_ENABLE_SCORE_TO_MASK_V3_GMW_EXPERIMENTAL=ON -DMOE_TOPK_ENABLE_SCORE_TO_MASK_V3_RECEIPT_V2_EXPERIMENTAL=ON -DMOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL=ON -DSQLite3_INCLUDE_DIR=/tmp/m6b-s45-sqlite -DSQLite3_LIBRARY=/tmp/m6b-s45-sqlite/libsqlite3
cmake --build /tmp/m6b-s46-review/ft-v2-tests --target moe_topk_score_to_mask_v3_ft_v2_bounded_conformance_test moe_topk_score_to_mask_v3_runtime_conformance_test moe_topk_score_to_mask_v3_process_e2e_test --parallel 4
ctest --test-dir /tmp/m6b-s46-review/ft-v2-tests -R 'moe_topk_score_to_mask_v3_(ft_v2_bounded|runtime)_conformance_test' --output-on-failure --verbose
TMPDIR=/tmp/m6b-s46-review/tmp S35_KEEP_TEST_TMP=1 /tmp/m6b-s46-review/ft-v2-tests/moe_topk_score_to_mask_v3_ft_v2_process_e2e_test --ft-v2-target-k-cases
TMPDIR=/tmp/m6b-s46-review/tmp S35_KEEP_TEST_TMP=1 /tmp/m6b-s46-review/ft-v2-tests/moe_topk_score_to_mask_v3_ft_v2_process_e2e_test --receipt-v1-smoke
```

生产构建命令：

```bash
cmake -S "$SRC/VFSS" -B /tmp/m6b-s46-review/ft-v2-prod -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DMOE_TOPK_ENABLE_SCORE_TO_MASK_V3_GMW_EXPERIMENTAL=ON -DMOE_TOPK_ENABLE_SCORE_TO_MASK_V3_RECEIPT_V2_EXPERIMENTAL=ON -DMOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL=ON -DSQLite3_INCLUDE_DIR=/tmp/m6b-s45-sqlite -DSQLite3_LIBRARY=/tmp/m6b-s45-sqlite/libsqlite3
cmake --build /tmp/m6b-s46-review/ft-v2-prod --target sytorch --parallel 4
```
Production `BUILD_TESTING=OFF` 的 Release 配置/构建和两个预期编译拒绝 probe 也已运行；命令行、退出码及产物哈希列于 manifest。完整测试日志不入库，仅保留其 `/tmp` 路径与 SHA-256。
## 4. 有限随机请求账本

固定源码位置：`VFSS/src/moe_topk/score_to_mask_v3_gmw.cpp:248-255` 的 `random_bytes` 将请求拆成至多 `2^20` 字节的 `RAND_bytes` call；返回值不等于 1 时立即失败，无回退。`:268-276` 保留 F_T/v1 无界 `next_uniform`；`:628-700` 的 `dealer_issue_packages_impl` 对 FTv2 先取 `g=gate_count` 个字节派生 triples，再取 `ceil(m^2/8)` 个字节作为 permutation-share tape，最后按 Fisher–Yates 位置每次取一个 4-byte word、最多 `r=6` 次。bounded sampler 的实现位于 `VFSS/src/moe_topk/score_to_mask_v3_bounded_sampler.h:10-50`，精确 integer retry 计算在 `:34-49`；版本化包签名域和 header 参数在 `score_to_mask_v3_gmw.cpp:45-60,424-450,720-735`。CMake 显式开关及 TEST_ONLY target 注册在 `VFSS/CMakeLists.txt:31-41,351-380,766-790`。随机请求顺序可自适应拒绝结果，但总查询/字节数有上界。

```text
Qmax_bytes = g + ceil(m^2/8) + 4*r*(m-1)
Cmax_calls = ceil(g/2^20) + 1 + r*(m-1)
each request <= 2^20 bytes
```

| N=m | g bytes | permutation bytes | max rejection words | Qmax bytes | Cmax `RAND_bytes` calls |
|---:|---:|---:|---:|---:|---:|
| 128 | 1,289,280 | 2,048 | 762 | 1,294,376 | 765 |
| 256 | 5,281,664 | 8,192 | 1,530 | 5,295,976 | 1,537 |

调用数重算：N=128 的 gate tape 分 2 次请求，加 1 次 permutation 请求和 762 次 4-byte sampler 请求，合计 765；N=256 的 gate tape分 6 次请求，加 1 次 permutation 请求和 1,530 次 sampler 请求，合计 1,537。总字节数同上式逐项相加。`Qmax/Cmax` 仅是一次 F_T/v2 发材的本地需求，不含 T 进程其他 RAND API 使用者、其他并发 session、密钥生成、服务 receipt 或 party 加密。部署层总量须按共享同一 DRBG 的所有调用聚合。

阈值 `rho_i=(2^32 mod i)/2^32 < 2^-24`，最多 `m−1` 个位置和每位 6 次 word，因此理想 IID 下

```text
delta_reject <= 1 - product(1-rho_i^6)
             <= sum rho_i^6
             < (m-1) * 2^-144
             <= 255 * 2^-144 = (255/256) * 2^-136 < 2^-136 < 2^-128.
```

接受时每个 residue 仍均匀；到上限时返回统一发材中止。这是基于 IID 32-bit 输入的低概率预处理失败，不是 mask 错误概率。该统计项只界定去版本、签名、编码元数据后的语义材料投影 `pi_mat`，不界定不同 F_T 版本间的完整签名 transcript。它与真实 DRBG 计算优势、签名、TLS 优势保持分开。

## 5. OpenSSL / DRBG 联合流桥接裁决

源码调用 `RAND_bytes` 而不是项目指定的可版本化随机接口；S45 代码没有设置 provider、`OSSL_LIB_CTX`、seed/reseed 来源或 DRBG 参数，也没有自定义 `RAND_METHOD`、`RAND_add`、`RAND_seed`、`RAND_set_rand_method` 的路径。CMake 对 OpenSSL 的最低版本是 1.1.1，未固定目标使用 1.1.1、3.x 还是更高版本。单次 F_T/v2 的有序请求长度和总查询数有限，只解决了循环的无界性，不能推出调用返回的数据是 IID 或计算不可区分。

所需对象应是完整有序随机接口，而非每次调用的边缘：

```text
W_T = ((len_j, return_status_j, bytes_j))_{j=1..c}
```

其中请求边界、长度由前缀拒绝结果自适应决定；`P` 是固定公开参数/版本/key profile，`E` 是实际 libcrypto/provider/context、熵源配置和进程生命周期策略；隐藏 `S` 仅代表初始和 reseed 熵输入，不是 DRBG 演化状态。需对 `S` 求平均，并提供 `Law(S | P, sk_T, E)=Law(S | E)` 或清楚的熵源替代定理。还需定义整个流的计算安全游戏，计入成功/错误返回状态、每个请求的返回字节和失败后的统一发材 abort。若只证明成功调用上的条件视图，必须单独给出 `Pr[all RAND_bytes succeed]` 的条件化损失界；不能把成功条件当作免费假设。

若有针对整个自适应请求流的 `Adv_DRBG_T(Qmax,Cmax;E)`，发材材料、哈希、包和签名是对同一随机流及固定 key 的后处理；计算不可区分性可沿确定性后处理继承。这个 hybrid 仍要求实际 `E`/熵源和成功状态处理可证明，不能从 deterministic post-processing 本身建立 DRBG 假设。`delta_reject` 仍独立，仅用于 `pi_mat`。

本轮观测的本机 build host 为 WSL Ubuntu 24.04.4、GCC 13.3.0、CMake 3.28.3、Ninja 1.11.1、OpenSSL 3.0.13。OpenSSL CLI 的当前进程配置显示 `default` provider active；`OPENSSL_CONF`、`OPENSSL_MODULES`、`OPENSSL_ENGINES` 未设置，`/etc/ssl/openssl.cnf` SHA-256 为 `529815b0dd4bd6608bafeeb3d410b0683374e61aef792b3e3f38b3767d26f747`，`libcrypto.so.3` SHA-256 为 `6a66c3ba6b3749aacc9497973fff00f6ba61c703ba7015d0d7ab3fd9510974b6`。这是 build/CLI 主机观察；不证明 Dealer 目标进程实际加载相同库、使用相同 `OSSL_LIB_CTX`/provider 或配置，也不代表生产 deployment profile。OpenSSL 通用 API 文档也只说明 API 的约定，不替代部署熵源和联合 DRBG 优势：[RAND_bytes](https://docs.openssl.org/3.0/man3/RAND_bytes/)、[EVP_RAND](https://docs.openssl.org/3.0/man7/EVP_RAND/)、[OSSL_LIB_CTX](https://docs.openssl.org/3.0/man3/OSSL_LIB_CTX/)。

### 关闭 DRBG profile OPEN 前必须补齐的环境资料

1. 目标 Dealer 的精确可执行文件/容器 digest、revision、实际动态链接路径或静态 libcrypto 版本/哈希；运行时加载器实际解析结果。
2. OpenSSL build/version/options、provider 列表与模块文件哈希、`OSSL_LIB_CTX` 创建/配置代码、自动加载配置路径、`OPENSSL_CONF`/`OPENSSL_MODULES`/`OPENSSL_ENGINES`、FIPS/engine/custom RAND 设置。
3. OpenSSL build seed-source 选项、运行平台 OS 熵接口及目标环境可用性/失败语义；只需安全模型和配置元数据，不提交或披露实际 seed、私钥或材料。
4. DRBG 类型、参数、primary/public/private 实例策略、首次播种和自动/显式 reseed 策略、阈值、失败返回路径及应用启动/初始化顺序。
5. T 进程的线程/并发 issuance、fork/exec、容器或 VM snapshot/restore、重启、reseed、崩溃恢复、回滚策略；同一上下文的其他调用方和总并发请求量。
6. 固定的 `P/E` 与隐藏 entropy input `S` 的条件模型；要么证明 `Law(S | P,sk_T,E)=Law(S|E)`，要么提供替代的熵源/DRBG 定理。
7. 对包括请求长度/边界、成功/错误状态和失败 abort 的自适应全流安全假设/可审查界 `Adv_DRBG_T(Q,C;E)`，并说明本轮单 session `Qmax/Cmax` 如何汇总成部署总 `Q/C`。

若没有上述目标配置与生命周期证据，本轮停止于 `DRBG_DEPLOYMENT_PROFILE=OPEN`；不声称实际 OpenSSL 输出流符合 IID tape。只有测试主机 profile，不应把“S46 fresh build 是 OpenSSL 3.0.13”写成部署结论。

## 6. 故障、信任和非目标范围

有界拒绝注入属于人工 TEST_ONLY 随机字输入控制。S40 的正常路径和自然业务 I/O fault `NOT_REPRODUCED` 按历史范围继承；S46 未重新尝试自然读写故障，也未将旧 E2E 误启动归类为协议故障。跨 DB 回滚/物理掉电、任意 crash-prefix 隐私、A-only 腐化仍为各自独立 OPEN/未测事项。

本轮未运行 S35 195 案、S40 211 案、完整 N≤256 组合矩阵、N>256、正式 LAN/WAN、RTT 或性能矩阵。N>256 继续 `NOT_ADMITTED`；同机 `phase`、RSS 和耗时不是正式性能数据；不创建 `secure` alias，v3/F_Tv2 继续默认关闭。S40 的 211 案仅是其当时 F_T/receipt 配置的历史 PASS_WITH_SCOPE，不转写为 F_T/v2 案例结果。

## 7. 状态、审查与后续

机器门禁、case list、源/build/evidence hashes 见同名前缀 JSON。分别记录：功能接收 `PASS_WITH_SCOPE`；默认关闭 `PASS_WITH_SCOPE`；有界失败语义 `PASS_WITH_SCOPE`（并保留产品级合同确认项）；有限随机请求界 `PASS_WITH_SCOPE`；实际部署 profile 与联合流桥 `OPEN`；private-channel 和 party-view `OPEN`；secure alias/performance `NOT_READY`。优势项没有合并成单一安全数字。

reviewer `/root/s45_non_author_review` 的报告记录其构建/测试、二进制、日志和错误 target 尝试。reviewer 确认没有遗留协议进程。原始日志、临时密钥、证书、材料和构建均保留在 `/tmp/m6b-s46-review`，不进入仓库。

后续若继续证明，应先由部署负责人提供上一节的目标环境资料，再在固定 `E` 下证明全序列 RAND stream 的条件计算界以及成功/失败选择效应；否则转做 N=1000 的功能、内存与材料资源准入（不声明正式性能）。S35 195/S40 211 历史矩阵不重复运行。
