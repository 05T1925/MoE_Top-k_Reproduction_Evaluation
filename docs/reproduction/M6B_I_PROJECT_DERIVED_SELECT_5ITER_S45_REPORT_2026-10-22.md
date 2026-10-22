# M6B-I-PROJECT-SELECT-5ITER S45 详细复核报告

日期：2026-10-22
路线：**Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL**
目标：Q20.12 signed score 按降序排序，同分按原始索引升序稳定处理，输出原始顺序且恰有 K 个 1 的秘密共享 Top-K mask。

本路线是项目派生的五迭代 Select。它不是 BB90 原算法复现、BMW16 实现或 BB90+DCF 性能结果。

## 1. 范围与来源身份

本轮从隔离 worktree 的 S44 提交 `aaa7f96095c350e8236246163d48c54a7fb8d9ba` 开始，父提交为 S43 `a1b02a8133955786e0bb6b047f55b746af66acbf`，S44 tree 为 `7bb41c0d57ed1158aa90764781ad523d83beab58`。S44 起始工作树干净。S44 固定的 S39 Dealer/runtime 提交是 `8028c0fece69fe0c648b1e0f1e12ad7fca723f20`；S35 85 项清单 canonical SHA-256 为 `af693eb61183a606c64a1f1b16ee5c3ad6e4b382ec5456e541c7915b8f95faa0`，S39 85 项清单 canonical SHA-256 为 `09444abd50c41357eb63e9050b7e39c3b2948588205d9d70f02561d7d8ec49d9`。S45 在该 S44 worktree 上只叠加本轮源码/测试与交付文件，没有用 main 或主工作区文件拼接被测对象。

S44 中继承的 S42 治理文档行 `PROJECT.md` 与 `docs/IMPLEMENTATION_PLAN.md` 的 `OPEN_METADATA_DIVERGENCE` 继续保留为来源元数据问题；S44 已独立确认 S39 Dealer 核心源码身份，不把治理哈希差异解释成运行时代码冲突。S41 provenance erratum、S43/S44 历史报告不修改。完整 S45 源码与构建身份逐文件见 [source/build/evidence manifest](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S45_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-22.json)。

证据分类如下：论文定义仍只提供项目研究背景；当前五层 Select、GMW/Beaver-v3、F_T/v2 和 escrow 流程是项目扩展；本轮构建、测试、源码映射是本地观察；真实部署 DRBG 等尚未闭合项仍是待证明假设。功能测试不构成安全证明。

执行者参与 S43/S44 的推导/审查并实现本轮代码，因此不是独立 reviewer。非作者 `/root/s45_non_author_review` 未参与 S45 实现或推导，按要求对采样界、版本规格、失败语义、代码映射和证据解释作只读复核；reviewer 未运行 build/test，也未改文件。

## 2. S45 版本化接口

新增 CMake 选项：

```text
MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_FT_V2_BOUNDED_EXPERIMENTAL=OFF  (默认)
```

只有同时启用现有 `MOE_TOPK_ENABLE_SCORE_TO_MASK_V3_GMW_EXPERIMENTAL=ON` 才能启用 F_T/v2。F_T/v2 具有独立显式 API；它不改变 F_T/v1 的入口、格式或采样器，也没有回退路径。

| 字段 | F_T/v1 | F_T/v2 候选 |
|---|---:|---:|
| package format | 4 | 5 |
| `ft_version` | 1（格式 4 的保留字节语义） | 2 |
| `sampler_version` | 0 / unbounded-v1 | 1 / bounded-rejection-v1 |
| `security_parameter_bits` | 0 | 128 |
| `retry_limit` | 0 | 6 次抽样尝试/位置 |
| escrow `receipt_format_version` | 独立字段 1 或 2 | 独立字段 1 或 2 |

F_T/v2 的 package format 5 header 将 F_T、sampler、安全参数和上限作为已签字节；party-package 使用新的用途域 `.../party-package/F_T-v2/bounded-rejection-v1` 与 package format 版本参数签名。完整 package hash 纳入 escrow context 中两方 package digest 和 package-set 摘要。receipt format 仍单独保存在已签 context 的 state 参数中；测试分别验证 `(F_T/v2, receipt/v1)` 和 `(F_T/v2, receipt/v2)`，没有把两个版本号合并。

F_T/v1 的 package format 4 保持原 7 个保留字节及旧 signature domain；v1 入口继续调用原无界 `next_uniform`。独立 v2 入口使用新的有界 sampler。关闭 FTv2 选项的 validator 仍拒绝格式 5，不会把未知格式解释成 v1。

## 3. 有界采样与概率界

每个 Fisher–Yates 位置 `i` 从一个 32-bit word `u∈[0,2^32)` 抽样。定义 `t_i=2^32 mod i`，当 `u<t_i` 时拒绝，否则输出 `u mod i`。接受区间长度 `2^32−t_i` 可被 `i` 整除，因此给定接受时每个 residue 出现次数相同，严格均匀。由于 `t_i<i≤256`，

```text
rho_i = (2^32 mod i) / 2^32 < 256 / 2^32 = 2^-24.
```

对 `m>1` 的 `m−1` 个位置，每个位置最多取 `r(λ,m)=ceil((λ+log2(m−1))/24)` 个独立 word。所有尝试拒绝的概率为

```text
delta_reject(m,r)
  = 1 - product_{i=2..m}(1-rho_i^r)
  <= sum_{i=2..m} rho_i^r
  < (m-1) 2^(-24r).
```

取 `λ=128` 且 `m≤256`，最大项为 `128+log2(255)<136`，故 `r=ceil(136/24)=6` 对所有 `2≤m≤256` 足够。更紧的统一保守界是

```text
delta_reject <= 255 * 2^-144
             = (255/256) * 2^-136
             < 2^-136 < 2^-128.
```

`m=1` 特判为 0 次抽样、0 次重试且返回 0；测试验证 helper 不消费随机字节。实际 N=1 的 v3 gate budget 将 `padded_n` 至少取 2，因此这是 helper 边界，不是本轮 N=1 Dealer issuance 证据。对有界调用，若所有位置成功，则各位置接受 residue 仍严格均匀；若上限耗尽，则统一发材中止。这个有限失败概率是发材失败概率，不是输出错误概率。

对照“永不失败”的 F_T/v1 时，上述 `delta_reject` 只适用于 `pi_mat`：丢弃 package/sampler version、签名和编码元数据后的语义材料分布。F_T/v2 的版本字段和签名域与 F_T/v1 不同，不能将该统计界直接套到完整签名 package、context 或完整 transcript。真实 `RAND_bytes` 到 ideal IID tape 的替换是另一个计算假设，保留为 `Adv_DRBG_T(Qmax,Cmax;E)`，不与 `delta_reject` 合并。

## 4. 调用量上界和实现映射

`score_to_mask_v3_gmw.cpp::random_bytes` 以最多 1 MiB 为单个 `RAND_bytes` 请求分块；`F_T/v2` 在 Fisher–Yates 每个位置最多发出 6 次 4-byte 请求。设 `g=gate_count`、`m=padded_n`：

```text
Qmax_bytes = g + ceil(m^2/8) + 4*r*(m-1)
Cmax_calls = ceil(g/2^20) + 1 + r*(m-1)
per-call request <= 2^20 bytes.
```

其中第一段是 gate triple 随机带，第二段是 permutation-share tape；其余是 Fisher–Yates 32-bit word。拒绝导致的调用计数只是上界，不含 T 之外的密钥生成、服务 receipt RNG、party envelope AES/nonce、party 输出刷新或其他 OpenSSL API。

| N | m | gate bytes `g` | permutation bytes | Qmax bytes | Cmax `RAND_bytes` calls |
|---:|---:|---:|---:|---:|---:|
| 128 | 128 | 1,289,280 | 2,048 | 1,294,376 | 765 |
| 256 | 256 | 5,281,664 | 8,192 | 5,295,976 | 1,537 |

`STRICT_PPT_BOUND` 仅表示在有限响应的理想随机字带模型中，采样循环、查询次数和单请求大小具有源码级上界。它不证明 `RAND_bytes`、filesystem `fsync` 或操作系统调用有墙钟时间界，也不证明真实 OpenSSL DRBG 或熵源安全。

源码位置映射：

- `score_to_mask_v3_bounded_sampler.h::next_uniform_bounded`：固定最大抽样次数、拒绝边界和 `m=1` 特例；`bounded_retry_limit` 用整数比较实现公式，避免浮点舍入。
- `score_to_mask_v3_gmw.cpp::dealer_issue_packages_impl`：由参数 profile 选择采样器；F_T/v1 与 F_T/v2 复用材料构造主体但保留不同 profile 与 sampler 路径。
- `register_ft_v2_invocation_once`：在材料创建前以 private registry 的 `O_CREAT|O_EXCL|O_NOFOLLOW` 登记 session/invocation burn record，并 fsync 文件与目录。
- `make_header`、`parse_header`、`score_to_mask_v3_validate_package`：写入、拒绝未知/错配 profile 并验证签名后的字段。
- `domain_separated_message` 与 F_T/v2 签名域：把 package format 版本和新用途域纳入签名输入。
- `make_signed_escrow_context` / `serialize_escrow_context_body`：receipt-format 保持单独字段；context digest 绑定两个完整签名 package digest。
- `durable_create` 仅在所有材料、package 与 context 都构造/签名之后调用；Sampler exhaustion 在该处之前抛出。

## 5. 统一失败合同与边界

F_T/v2 issuer 在参数/身份/context 配置校验后、任何材料随机字节请求前先登记 burn marker。达到第 6 次拒绝时抛出 `F_T/v2 ISSUANCE_ABORT`。定向测试观察到：P0/P1 package、签名 escrow context、party lease 文件均不存在；没有执行 READY/在线服务阶段；同 registry 根目录下相同 session/invocation 的再次 F_T/v2 请求在发随机字节之前被拒绝。

这不等于“磁盘从未有任何痕迹”：内部 0600 burn record 会有意持久存在，供 T 阻止同根目录重发。该记录是 T 的发材 burn 状态，不是 party lease，也不向 parties 发布。保证要求部署固定同一受限 registry 根路径；若更换根目录/回滚存储，或通过仍可单独调用的 F_T/v1 入口重发，本轮实现没有全局跨版本防重。部署必须固定允许的 F_T 版本，不能把该局部 ledger 描述为全局一次性证明。

采样中止前可能已在进程内创建 triple/permutation vectors；本轮没有显式清零保证。由于中止发生在 header/context/signature 构造和三个 `durable_create` 写入之前，测试证明的是没有可被 parties 接受的文件包/lease，不是内存中从未出现中间材料，也不是对任意后续磁盘写错误的多文件原子性。若随机采样成功但之后依次写 P0、P1、context 时发生文件系统错误，旧有顺序写路径可能留下部分文件；此故障边界未由本轮 bounded-abort 测试覆盖，且不属于本轮 `delta_reject`。

## 6. 定向验证与运行证据

环境为 WSL Ubuntu 24.04，GCC 13.3.0，CMake 3.28.3，Ninja 1.11.1，OpenSSL 3.0.13；SQLite 开发头在本机镜像缺失，使用官方 SQLite 3.45.1 amalgamation header 和临时系统库链接仅辅助本地构建，未改仓库依赖或提交该文件。完整参数、二进制哈希与命令见 manifest。

| 验证 | 结果 | 观察范围 |
|---|---|---|
| Release default-OFF fresh configure/build | PASS | CMake cache 中 GMW/FTv2 OFF；target help 无 v3/FTv2 target，`libsytorch.a` 无 v3/FTv2 符号 |
| Release 显式 GMW + receipt/v2 + F_T/v2 + BUILD_TESTING=ON | PASS | 建立最终 conformance 与过程 E2E binary |
| `F_T/v2=ON, GMW=OFF` CMake 配置负例 | PASS (expected reject) | CMake 报出 `F_T/v2 requires ... GMW...=ON` |
| Release `BUILD_TESTING=OFF` opt-in library | PASS | bounded API 符号存在；确定性 TEST_ONLY random hook 符号不存在 |
| `moe_topk_score_to_mask_v3_ft_v2_bounded_conformance_test` | PASS | m=1/2/3/255/256，阈值拒绝/接受边界、6 次连续拒绝、attempt/byte/call 上界、N128/256 Qmax、包 profile 篡改、receipt 格式分离、重复发材、abort 后 burn 与无 package/context/lease |
| `moe_topk_score_to_mask_v3_runtime_conformance_test` | PASS | 既有 v3 runtime conformance 相关回归 |
| F_T/v2 independent-process smoke | PASS | N128 K64 与 N256 K128；双方重构 mask 与冻结 `top_k_mask` oracle 一致且恰 K 个 1；原序输出正确；成功路径 Dealer/P0/P1 exit 0；escrow restart fetch recovery PASS |
| 强制发材中止 | PASS | N256 第一个非二次幂范围连续 6 次拒绝；Dealer exit 1；package/context/lease 均不存在，S45 结果记录 `ISSUANCE_ABORT` |
| receipt/v1 + F_T/v1 compatibility smoke | PASS | N4 K2，F_T version 1、receipt format 1，双方 oracle PASS，重启恢复 PASS |
| S35 195 案、S40 211 案 | NOT_RUN | 本轮仅执行必要定向覆盖 |
| 正式 LAN/WAN、RTT、性能矩阵 | NOT_RUN / NOT_MEASURED | 本轮测试输出中的 phase/RSS/本机 elapsed 仅供诊断，未作为性能结论 |

E2E 正常案服务进程和 restart service 的 exit code 为 143，是 TEST_ONLY harness 在完成 COMMITTED/fetch 后以 SIGTERM 清理进程的预期结果；Dealer、P0、P1 均为 0，API 状态为 `BOTH_COMMITTED`，restart recovery PASS。它不是在线阶段失败。故障注入只覆盖本轮为证明有限拒绝而使用的受控 TEST_ONLY 字节带；不把人工采样控制当作自然 I/O 故障。协议业务自然读写故障状态沿用 S40 `NOT_REPRODUCED`，本轮没有重新尝试自然故障复现。

固定 N128/N256 E2E 输入由周期性重复分数构成，包含 `INT32_MIN`、`INT32_MAX`、负值、0、正值和重复 tie；`top_k_mask` 用作冻结 oracle。N128/256 的 K 均为 N/2；这是定向目标规模验证，不替代 S40 211 案，也未覆盖全部 K 值。package SHA、case JSON、输出摘要、构建归档及测试 binary SHA 在 manifest/逐案记录中给出。本机 diagnostics 中 exchange phase、RSS、字节数和 elapsed 不是正式性能结果。

## 7. 安全裁决和未闭合项

有限重试解决 S44 所指出的“采样循环没有 source-level worst-case draw bound”这一工程/证明接口问题，但没有完成以下工作：

1. `JOINT_RNG_STREAM_BRIDGE=OPEN`：需要固定实际 OpenSSL/libcrypto/provider/`OSSL_LIB_CTX`、seed/entropy source、重播种、并发、fork/restart 生命周期和错误选择效应，证明聚合的 whole-stream transcript 与 ideal IID tape 的条件计算不可区分性。保留符号 `Adv_DRBG_T(Qmax,Cmax;E)`；没有可靠部署配置或可量化优势。
2. `PARTY_VIEW_COMPOSITION=OPEN`：F_T/v2 的 bounded sampling 不证明完整 GMW/Beaver party-view simulator，也不推进之前的 Select/输出/escrow 条件模拟。
3. `MATERIAL_FAILURE_CONTRACT=PASS_WITH_SCOPE` 不覆盖内存零化、跨版本/跨路径 global burn ledger、存储回滚、任意文件系统故障的多文件原子性或掉电持久性。
4. `N>256=NOT_ADMITTED`；真实自然业务 I/O fault 仍为继承的 `NOT_REPRODUCED`；任意 crash/abort-prefix 隐私没有新增证明。
5. `SECURE_ALIAS_READY=NOT_READY`，v3/F_T/v2 保持默认关闭；`PERFORMANCE_READY=NOT_READY`，formal LAN/WAN、RTT 与正式性能矩阵没有启动。

因此本轮只将有界采样与本轮目标范围功能记为 `PASS_WITH_SCOPE`；不把 `delta_reject` 与 DRBG、签名、TLS 或协议模拟优势相加为单一结论，不声明密码学安全通过。

## 8. 本轮变更文件

运行时/构建/测试：

- `VFSS/CMakeLists.txt`
- `VFSS/include/moe_topk/score_to_mask_v3_gmw.h`
- `VFSS/src/moe_topk/score_to_mask_v3_gmw.cpp`
- `VFSS/src/moe_topk/score_to_mask_v3_bounded_sampler.h`（新增）
- `VFSS/tests/moe_topk/score_to_mask_v3_ft_v2_bounded_conformance_test.cpp`（新增）
- `VFSS/tests/moe_topk/score_to_mask_v3_ft_v2_test_support.h`（新增，TEST_ONLY）
- `VFSS/tests/moe_topk/score_to_mask_v3_process_e2e_test.cpp`

治理/交付：

- `PROJECT.md`
- `docs/IMPLEMENTATION_PLAN.md`
- `docs/REFERENCE_MANIFEST.md`
- `docs/decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S45_DECISION_2026-10-22.md`
- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S45_REPORT_2026-10-22.md`
- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S45_GATES_2026-10-22.json`
- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S45_CASES_2026-10-22.json`
- `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S45_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-22.json`

`VFSS-baseline/`、用户主工作区、历史 worktree、S43/S44 历史证据未修改；密钥、材料、证书、构建物和原始 E2E 子进程大日志未纳入仓库。

## 9. 非作者复核结论与证据修订

非作者 `/root/s45_non_author_review` 的复核状态为 `PASS_WITH_SCOPE_REVIEWED`，仅覆盖重试概率与上界、profile/receipt 版本绑定、失败点及副作用、源码映射、测试解释和 `delta_reject` 的适用范围。审查者确认重试公式、N=128/256 的 Qmax/Cmax 计算与代码映射无误，且没有把语义材料统计界外推到完整签名 transcript 或真实 DRBG 桥。审查者未运行构建/测试，未修改文件；该复核不等于密码学安全通过。

复核期间的交付修正均已进入最终快照：TEST_ONLY E2E 结构化输出中的格式问题已修正并重跑对应定向 smoke；首轮报告中遗漏 source/build/evidence manifest 的问题已补齐，审查者确认其存在、路径互指一致并核对 7 项源文件哈希；执行者发现并更正 `score_to_mask_v3_ft_v2_test_support.h` 的清单哈希后，审查者再次确认 7 项源文件哈希全部匹配。manifest 所列非自引用交付文件哈希也均匹配。相关结论只修复证据准确性，没有改变运行时代码或安全边界。