# BMW16 S30：独立接收、条件入口与规模验证报告

日期：2026-10-10
标签：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL**
隔离分支：`codex/m6b-i-bmw16-s30`
S29 接收基线：`871b6683d15a34d117eb24353611d8ef48434aca`
S30 被测源码 commit：`eac5151e96f3cc67d2bc664c0d300b51ef672e10`
S30 最终文档提交：见当前分支最终 HEAD；它不替代上述被测 runtime revision。

## 1. 摘要

S29 `conditional_secure_v1` 的入口检查、独立复跑和 n≤256 mTLS E2E 通过。S30 增加默认关闭的 `conditional_secure_v2`，只把 admission scope 扩到 n≤1000，仍调用唯一的 S26/S28 party runtime 和材料 ABI。S30 通过 n=1000/K=80 fresh-material 三进程 E2E；新 v2 wrapper 尚需异会话独立接收。条件安全结论继承 S28 对原 runtime 的独立审查，只适用于第 4 节所列假设，不是无条件/恶意安全，也不是生产部署接收。

S30 没有找到原 v1 的运行时缺陷。发现并修正了两个复核/证据问题：S29 文档把中间 ref `915fbb…` 写在最终 ref 说明附近；E2E 重领子进程的 stderr 偶尔与成功摘要交叉。另一个 n=1 首次测试失败来自 harness 错把无材料 shortcut 当成需在线发材路径；已改为调用 v1/v2 conformance 中的 n=1 shortcut 测试。三个原始记录均保留在仓库外。

在 v1 1≤n≤256 范围内，S28 的条件安全设计已异会话接收，S30 又独立接收了 S29 最终 v1 入口并完成本轮代表性 E2E；**该范围的代码与条件安全设计门已关闭**。此结论只在第 4 节明确列出的理想私有认证通道、密码学/随机性假设与许可泄露函数 L 下成立，不表示无条件安全、恶意安全或已完成生产部署。v2 将同一 admission 规则扩至 n≤1000，S30 实际 n=1000 E2E 通过，但 v2 wrapper 是本轮新增源码，仍需下一接收者复核。

## 2. Git、接收链与来源核对

### 2.1 起始状态

在隔离 worktree 检查 `AGENTS.md`、`PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md`、`docs/BENCHMARK_VALIDATION_PLAN.md`、S27/S28/S29 决策与报告，以及当前 party、conditional wrapper、DCF/uCMP、材料、TLS 和 shuffle 源码。原工作区的未提交差异未复制或修改。隔离 worktree 从 S29 最终文档 head `871b6683d15a34d117eb24353611d8ef48434aca` 开始；起始干净。开工时 main/origin/main/merge-base=`c3926c68fd14f270faa8b55234311071947fa080`。

| 对象 | revision | 说明 |
|---|---|---|
| S26 最终功能 runtime | `2818bce20f30719eaccf0bc5df586cce4fe78c84` | 已接收核心 runtime 身份 |
| S26 runtime 修复 | `ded433d636a3df929380bdce0ff22f4ce6121453` | 同一功能路径的修复 |
| S26 异会话功能接收报告 | `32f7f0ba3f80083ff282c9b5ebd0a827bd366898` | 接收者报告，不是被测 binary revision |
| S27 条件证明文档 | `8bbf7119470bbc37f6950a3b425255ed993eaa8d` | S28 独立复核对象 |
| S28 独立安全复核 | `27bee47de4356a87af6a1f6ff06be6c7e0df2c9b` | 未改 DCF/party runtime |
| S29 被测 runtime | `08e96d393da6f102bb047d230e8845da4509814d` | 新增 v1 wrapper 的源码 revision |
| S29 最终报告/远端 head | `871b6683d15a34d117eb24353611d8ef48434aca` | 本轮接收基线 |
| S30 被测 runtime | `eac5151e96f3cc67d2bc664c0d300b51ef672e10` | 加入 v2、E2E 证据修正和 v1 全矩阵 harness 的源码 |

S29 的 `915fbbdd7e96bdd34f97f4d414b335f3b3951935` 是同步时的中间 ref，不是最终提交。本轮 `git ls-remote` 核对 `origin/codex/m6b-i-bmw16-s26` 与 `refs/pull/29/head` 都是 `871b6683d15a34d117eb24353611d8ef48434aca`；PR #29 Open、目标 main、非 Draft、未合并。页面标题仍是泛化的 S26 候选标题，正文为模板占位。GitHub connector 返回 `USER_NOT_LOGGED_IN`，本机无可用 `gh`，S30 未改远端 PR 或 main。PR #28 未触及。

PR #29 当前仍只包含 S29 v1，不包含本地 S30 v2。取得写权限后可将标题更新为 `Conditional Protocol I + BMW16-derived Select + DCF candidate (PROJECT_DERIVED)`，正文应说明 v1 n≤256、非 BB90、默认 OFF、S28 条件证明假设和 L、47 个 S30 v1 E2E、正式性能 NOT_RUN；不要把本地 v2 n=1000 写入未推送的 PR diff。若提交 v2，应以经审查的显式分支/PR 更新处理。

远端维护者可直接使用的 PR #29 标题：

`Conditional Protocol I + BMW16-derived Select + DCF candidate (PROJECT_DERIVED)`

建议正文：

> This PR adds the opt-in Protocol I + BMW16-derived Select + DCF project-derived candidate. It is not a BB90 implementation and does not claim BMW16 Algorithm 7 verbatim, its theorem bounds, or a four-round secure entry protocol. The conditional v1 entry is limited to 1≤n≤256 and assumes a trusted offline non-colluding T, at most one semi-honest online party corruption, ideal private authenticated channels, the source-specific G126/restricted-key and AES-CTR PRG assumptions, OS CSPRNG, and ROM sampling with an honest entropy contribution. The allowed leakage L and proof boundaries are in the dated S29/S30 decision documents. Targeted conformance is 9/9, common-tape differential 39/39, and S30 v1 independent-process E2E 47/47; default build remains OFF. Formal LAN/WAN performance, full V4 qualification, production credential operations, and unconditional/malicious security are not included. The local S30 n≤1000 v2 extension is not part of this PR.
### 2.2 S29 入口源码审查结果

| 检查项 | S29 源码事实 / 结果 |
|---|---|
| 默认构建 | 实验适配器、v1、v2、TEST_ONLY failpoints 均默认 OFF。v1/v2 不能绕开实验适配器依赖。 |
| CLI 与公共 API | v1 CLI 强制 TLS party 模式；API 要求 authenticated transport、限制 `1≤n≤256`，拒绝 test fault/tape 标志。 |
| 协议调用关系 | wrapper 最终调用唯一 `protocol_i_bmw16_experimental_raw_score_mask_party`；Select、shuffle、DCF/uCMP、membership、inverse、bundle/sidecar ABI 均未复制。 |
| 暗中降级 | `RequireAuthenticatedStream` 缺 stream、错绑定或提前消费均 fail closed；协议 frame bytes 走 TLS data path，不把裸 FD 作为 TLS 握手后的旁路。 |
| 明文/Dealer | party 不重构 score、rank、selected original index、比较位或完整 mask；没有在线 T、排序 fallback、retry。oracle 重构仅在隔离 E2E 控制器。 |
| 失败合同 | 成功后输出原序 mask shares；概率 abort、材料、通信和工程异常使用不同状态；未确认 peer 时不冒称两方同意。 |

## 3. S30 修改前发现、修改和影响

| 发现 | 位置 | 影响及处理 | 改动身份 |
|---|---|---|---|
| `915fbb…` 出现在 S29 当前 ref 叙述 | S29 PROJECT/plan/decision/report | 该哈希是中间 ref；S30 更新成 `871b668…` 最终 head 及 PR ref 的本轮核验值。历史时间点保留。 | 文档，未改 S29 runtime |
| replay 子进程继承 runner stdout/stderr | `experimental_bmw16_party_node_e2e_test.cpp` replay fork | 负例的 `ABORT_MATERIAL` 有机会拼入成功摘要。S30 为 P0/P1 replay 分别收集诊断，父进程核验退出码、日志状态和无 mask 后打印汇总。仅 TEST_ONLY harness。 | 测试证据修复；不改协议帧/材料/泄露 |
| n=1 shortcut 被当作需 TLS 发材实例 | S29 原 E2E 驱动 | T 正确拒绝 singleton 材料，导致 harness 误报启动门失败。S30 将 n=1 移到 v1/v2 API conformance，网络 E2E 从 n=2 开始。 | 测试范围修正；不改 API 行为 |
| v1 n≤256 无通向 1000 的版本入口 | S29 v1 header/source | 不放宽 v1；新增 v2 明确 cap=1000、独立 build option/CLI/test，通过后委托同一核心 runtime。 | S30 新 runtime wrapper/CLI version |

改动没有改变 DCF/uCMP ABI、材料格式、协议消息、允许泄露、核心因果层、错误状态或正式指标口径。v2 只增加基于同一证明公式/假设且经 n=1000 功能 E2E 的范围 admission；它并未把 S28 的证明条件去除或加强成标准模型结论。

## 4. 条件安全声明与范围

S28 独立复核覆盖实际完整序列化 M2UC v1 DCF key 的 source-specific G126 归纳、相关阈值全池（含未使用 key）、opened comparison bit、自适应访问和同一 key 两次 Eval，以及实际 forward/inverse shuffle、两 task 共享句柄与 output-share 的条件 simulator。S30 逐项检查 S29 wrapper 只是输入范围/TLS约束，不改核心状态转移，因此把 S28 结论限定延伸到 v1/v2；不把证明报告提交当作 S30 新 runtime 的来源 revision。

**事实、假设和排除项分开：**

- 事实：稳定键语义为 signed Q20.12 score 降序、`original_index` 升序；v1/v2 使用相同 key/slot、uCMP、shuffle、membership 与输出路径。成功分支 oracle 对照检查恰 K 位。
- 假设：可信离线静默且非合谋 T；最多一个半诚实腐化 P0/P1；抽象理想私有认证通道；source-specific G126 与受限 126-bit AES 子族 PRG/PRP（含证明要求的辅助输入）；cryptoTools AES-CTR root stream PRG；OS CSPRNG；独立 KeyGen 随机带；ROM sampler 与至少一方诚实 OS 熵贡献。
- 不宣称：普通 AES-128 PRP 定理直接覆盖受限 key 子族；BGI15 Theorem 6 直接证明该 serialization；标准模型 SHA-256 sampler 界；恶意安全、两方合谋、T 合谋、生产证书/密钥注册运维、管理员读取双方存储、快照/VM 回滚；或“BMW16/BB90 精确实现”。

L 包含 masked operands 与 `public_z`、匿名端点/打开比较 bit、pivot/U/V/W、两任务共用匿名 handle、selected anonymous handle、key ID/访问次序/次数、比较次数、帧长与 phase、公开 sampler XOR coin、双方共同算法 ABORT、本方输出 share。L 不包含 raw score、original index 到匿名 handle 映射、明文阈值/全 mask/对方 share。断连为 LOCAL_ONLY，属于工程异常，不冒充 PEER_AGREED。

优势式保持为 S29/S28 决策中的符号形式。按 `N_DCF=2p+9C`、`slots/party=2p+2+9C`、`b=33+index_bits`。n=1000 有 `N_DCF=4,497,548`、每方 `4,497,550` 槽、`q0=8,995,096` 个根流 block（源布局推导）。不填臆造的 `Adv_G126`、root-stream 安全位数或 sampler 标准模型优势。n≤256 的理想/ROM abort 最坏界 `1.3105804606611235×10^-11`；n≤1000 所用理想失败预算使用 S16 推导适用式（上界低于 n≤10^6 记录的 `5.065679989546104×10^-8`），需要同样的 ROM/诚实随机贡献假设。S30 实际 47/47 无自然 abort 不推成统计保证。

## 5. 构建与验证

### 5.1 环境和二进制

- WSL Ubuntu 24.04，GCC/G++ 13.3.0，CMake Release，CTest enabled。
- 启用：`MOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON`、`MOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V1=ON`、`MOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V2=ON`。
- 关闭：`MOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=OFF`、`MOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=OFF`。v1/v2 API 仍拒绝 TEST_ONLY 控制。
- Release build: `/tmp/moe_bmw16_s30_conditional_release`。默认关闭 build: `/tmp/moe_bmw16_s30_default_off`；目标清单中不存在 conditional 或 BMW16 实验 target，相关 cache 值均为 OFF。

S30 runtime commit `eac5151e96f3cc67d2bc664c0d300b51ef672e10` 下 Release 二进制：

| Binary | SHA-256 |
|---|---|
| `moe_topk_bmw16_experimental_party_node` | `46f2aca7202df691bc42e1fba542f47a1a75c5d22a232c213c7f250deba93eff` |
| `moe_topk_bmw16_experimental_party_node_e2e_test`（capture fix 后） | `54ea64b1502f230d0a2414b9177c4d23272a5eb94b3568d8b2c1062dbac2062c` |
| `moe_topk_bmw16_experimental_party_test` | `a64841acc77bf1f2201b3dd397f7a3e3e126efc10a380eeb3f130d38dd4a9c2e` |
| `moe_topk_bmw16_conditional_secure_v1_conformance_test` | `a1549a950a965e4dce92432d37e790cd2333e0a7c4f66e1a9c14d37e9e8be823` |
| `moe_topk_bmw16_conditional_secure_v2_conformance_test` | `755a1d546e43aa3ce29b5c59dea17e84dc491fb224c4c422c9b4991090d32bf6` |

被测 runtime 源文件 SHA-256（对应 commit `eac5151e96f3cc67d2bc664c0d300b51ef672e10`）：

| Source | SHA-256 |
|---|---|
| `VFSS/CMakeLists.txt` | `142869BF76816CBE2A0817ED6E818E16CF1DED3725BC1860CF9286E689E9DCE7` |
| `VFSS/include/moe_topk/protocol_i_bmw16_conditional_secure_v2.h` | `4173D57590B9E1F95D5379A3727505EA3E49172125FC4D6C52701C492868C5A8` |
| `VFSS/src/moe_topk/protocol_i_bmw16_conditional_secure_v2.cpp` | `7B1088371D441B835069DE9491D17527E1C89BC1EF3F2D578EE1863D4D810B96` |
| `VFSS/src/apps/moe_topk_bmw16_experimental_party_node.cpp` | `BBAA7FA5635FA8C1C4A718A37D7B73EA06C7FE0B134D262A2308F5C181DBD105` |
| `VFSS/tests/moe_topk/experimental_bmw16_party_node_e2e_test.cpp` | `1C4F58E2C947A35B892D3B2E705DA2D452C3141F395DD912A714D2509F1862BC` |
| `VFSS/tests/moe_topk/protocol_i_bmw16_conditional_secure_v1_conformance_test.cpp` | `9F69547ECBCEFBBC912573F93CF0EC7675AF799032386A2D3B14FB4210DF5629` |
| `VFSS/tests/moe_topk/protocol_i_bmw16_conditional_secure_v2_conformance_test.cpp` | `370FF3D9D11AD8A01F61CE5A78C624250756CB169A4EB58C444B67FBBEA695CA` |
### 5.2 执行顺序和结果

1. **Conformance 定向 CTest**：构建后对 BMW16 party、conditional-v1/v2、Protocol III 3-process E2E、DCF、uCMP、transport、raw-score、Protocol I shuffle 运行 9 个注册测试；9/9 PASS，3.49 s。该命令不等于全仓 CTest。
2. **共同 tape differential**：S4 Python 与同一 C++ TEST_ONLY oracle 的 39 条 common-tape 对照 PASS；逐项比较 S4/C++ 随机决策，测试 binary 是上述 `moe_topk_bmw16_experimental_party_test`。矩阵文件 SHA-256 `DC1FB7A88143F28E985F961C84B3AEC740B58C3E4C59EE6F89EA4B9539D85947`，S4 source SHA-256 `abebab9b710dc4f763924230674617f3fdf11756dada4d9782a6273750e82042`。
3. **conditional-v1 独立进程 E2E**：重新构建 capture-fix harness 后，47 个 fresh-material case 全部 T/P0/P1 exit 0，并逐位比对冻结 oracle；覆盖 n=2..8 每个 K，n=64/128/256 的 K={1,mid,n}，n=8 全相等 tie K={1,4,8}。47 次持久 bundle 重领均 P0/P1 exit 20、no-mask；OS UID 隔离检查 48 次 PASS；错在线 peer 1 例双方 exit 30、no-mask、identity rejected；对端静默 1 例有界退出、no-mask。自然算法 abort 0/47，注入算法 abort 0（conditional API 拒绝 TEST_ONLY 控制），不作概率外推。
4. **n=1 shortcut**：v1/v2 各自 conformance 实际调用 API 两 party shares 并核对 XOR 得 1，无 T、材料或 peer。最初误把 n=1 放进 TLS 发材 harness，测试失败 `T=70/P0=30/P1=30`；失败日志保留，测试被修正后重跑通过。它不是 runtime 缺陷。
5. **conditional-v2 n=1000/K=80 E2E**：S30 在 n=1000 最终代码上用 fresh material 运行 T、P0、P1 不同进程、不同 UID、本机 mutual TLS。首次运行和 replay-capture 修改后的最终复跑均保留。两次主路径均为 T/P0/P1 receive/online exit 0，oracle 正确、weight=80；P0/P1 重领分别 exit 20、no-mask；本地 UID 互不可读 PASS，T 不访问在线输入 PASS。最终 capture-fix 复跑的 T 生成 `546766326 µs`、TLS delivery `389400667 µs`，双方 sent bytes 总和 `9,941,055,454`（每方接收 `4,970,527,727`）；首轮记录 T 生成 `612129097 µs`、TLS delivery `451672497 µs`。这些是单机 loopback 功能/规模观察，不进入性能排名。

测试输入和随机性标识：v1 `n=2..8` 使用固定 `extended_scores(n)`；会话为 `0x20300000+n*256+K`。n=64/128/256 的 TEST_ONLY 输入由 `std::mt19937_64` 种子 `0x2030000000+n*16+j` 生成（j=0/1/2 对应 K=低/中/高），会话为该 seed XOR `0x43415345`；n=8 全相等用分数 7、会话 `0x20300800+K`。v2 n=1000 的 TEST_ONLY score 和 session 均以 `0x2030100080` 作种子/标识（日志十进制 session=`138245308544`），K=80。TEST_ONLY share split 由 session 派生。party 在线抽样与 KeyGen 不使用固定测试 seed，使用本次进程的 OS CSPRNG；故原始 E2E 日志保留 session、case、状态而不声称可重放相同协议随机 transcript。common-tape differential 使用独立保存的公共测试 tape。
6. **全仓 CTest 与正式性能**：全仓 CTest 未运行；本次只声明 9 个有针对性测试通过。V4 LAN/WAN 1+5、九指标和六方案性能排名 NOT_RUN/NOT_MEASURED。

### 5.3 主要复跑命令

Release configure 关键参数：

```bash
cmake -S VFSS -B /tmp/moe_bmw16_s30_conditional_release \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON \
  -DMOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V1=ON \
  -DMOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V2=ON \
  -DMOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=OFF \
  -DMOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=OFF
```

定向回归：

```bash
ctest --test-dir /tmp/moe_bmw16_s30_conditional_release --output-on-failure \
  -R 'moe_topk_(bmw16_experimental_party_test|bmw16_conditional_secure_v1_conformance_test|bmw16_conditional_secure_v2_conformance_test|m3_protocol_iii_three_process_e2e_test|m1_dcf_conformance_test|m2_ucmp_conformance_test|m2_transport_conformance_test|m2_score_input_conformance_test|m2_parallel_shuffle_conformance_test)'
```

三进程 E2E：

```bash
moe_topk_bmw16_experimental_party_node_e2e_test \
  moe_topk_bmw16_experimental_party_node --conditional-secure-v1
moe_topk_bmw16_experimental_party_node_e2e_test \
  moe_topk_bmw16_experimental_party_node --conditional-secure-v2-n1000
```

在本机实际运行时，两条 E2E 都由 WSL Ubuntu 24.04 root 测试控制器启动；控制器仅准备不同 UID/材料/测试 CA，并在 party 退出后重构 oracle。三方本身是独立进程。测试控制器权限不计入 P0/P1 隐私声明。

### 5.4 仓库外原始证据

原始日志、common-tape trace 与构建目录未进入 Git：
`C:\Users\28641\.codex\artifacts\BMW16_S30_20261010\`。

| Evidence | SHA-256 | 内容 |
|---|---|---|
| `conformance_final.log` | `5820DB69DD7253E32303CDA27C776CD9139AF25CA81C14A9DBA984C3BB912E36` | 9/9 定向 CTest |
| `common_tape_final/matrix.json` | `DC1FB7A88143F28E985F961C84B3AEC740B58C3E4C59EE6F89EA4B9539D85947` | 39 common tape |
| `conditional_v1_e2e_capturefix.log` | `78FCB67445F431E0305B770D5AE2D0F8674FD3F60B40FB794ACCFC08B9928ACC` | capture-fix 后 47/47 v1 |
| `conditional_v2_n1000_e2e.log` | `2D222B776D6E00B405B01B92924C41CB136540230C11A6EBAB16CA43E181EB09` | v2 n1000 首次 fresh-material E2E，保留作历史运行 |
| `conditional_v2_n1000_capturefix_e2e.log` | `28389C4476D12D294F4334D345345FF178A46AB4905B1CFDFE11366CA3255D5C` | v2 n1000 最终 capture-fix fresh-material E2E |
| `conditional_v1_n1_harness_failure.log` | `3E80D86A3398F9E7DA827CF4980218425AB485C4A1645205486F15D1DAB98C3A` | 原 n=1 harness setup 失败，保留 |
| `default_off_config.log` | `F95D1C764B2CB39D4BC89A743BEEBF2E2145C2946A7C675FDFD075E46B645ED2` | 默认关闭配置 |

其它首次构建日志保留在同目录；第一次构建曾误写不存在的目标名，修正后目标构建成功。日志和副本不包含 key/share/secret payload。运行时临时材料和证书放在 WSL `/tmp` 并由测试 harness 删除；本报告不收录其字节内容。

## 6. 容量门与服务器准备

n=1000/K=80 v2 fresh-material E2E 两次成功，最终 capture-fix 运行作为 S30 `VERSIONED_ENTRY_E2E` 功能证据；这不是大规模性能资格，也没有独立接收 S30 v2 wrapper。对应公式为：p=1024、index_bits=10、comparison_bits=43、`C=499500`、`N_DCF=2p+9C=4,497,548`。每方总 `slots=2p+2+9C=4,497,550`；其中流式 sidecar 是 `9C=4,495,500` 个比较 key slot，大小为 `164 + 4,495,500×(1089+16)=4,967,527,664` bytes；其余 `2p+2=2,050` 个 raw/shuffle slot 由 shell 材料承载。key record 为 1089 bytes，AEAD tag 16 bytes。两份 sidecar payload 约 9.94 GB，最终运行 T 输出 TLS sent 实测 9.941 GB。解析量与实测输送量分列。

- **n≤256/v1**：入口/条件安全接收范围关闭；不重开同一门。
- **n≤1000/v2**：源码门、构建门和一个 n=1000/K=80 功能 E2E 通过；v2 wrapper 本轮实现，异会话复核待办。
- **n=10,000**：约 540.4 GB/party 解析 sidecar，本机不生成，`PRECHECK_REJECTED/RESOURCE_INFEASIBLE`。
- **n=100,000 / 1,000,000**：分别约 57.3 TB / 6.05 PB 每方，当前全池路线不准入，不运行。

后续服务器建议：至少 8 vCPU、16 GiB RAM、30 GB 临时盘/单一 n=1000 fresh session，分方独立 UID/存储及受控 claim-root，mTLS/TLS证书作为预置配置；运行前核对两份实际 sidecar 总量和输出空间，试运行后采集 T/party RSS。后续正式矩阵按统一 schema 1 次预热+5 次；每配置新材料，保留 SUCCESS/算法 ABORT/工程失败的尝试数、阶段耗时和成本。网络带宽、RTT、计量器、正式在线轮数及 PRG 九指标本轮未测，均 NOT_MEASURED。

租赁服务器正式任务的输入清单：先冻结一份 signed Q20.12 原始输入与 stable-oracle 输出，复用到所有对照实现；至少列入已由计划冻结的 n=128/256 配置和显式 v2 的 n=1000/K=80 功能规模，K 两端与中间值按统一矩阵登记。每个正式重复使用 fresh T 材料、独立 session 与算法种子；保留预热和 5 次正式尝试，包括算法 ABORT、材料/通信错误及其成本。记录同一主机镜像、编译器/commit/binary SHA、CPU/RAM/磁盘、链路 RTT/带宽/netem 参数、计时边界与九指标 schema；未完成 V3 前置或独立接收 v2 wrapper 前，不把 n=1000 v2 纳入正式 V4 对照。

## 7. 门禁

| Gate | S30 判定 | 依据 |
|---|---|---|
| `S29_FINAL_ENTRY_INDEPENDENT_ACCEPTANCE` | PASS（v1） | 独立源码审查 + 47/47 v1 E2E；v1 n≤256。 |
| `DCF_SINGLE_KEY_PRIVACY_CONDITIONAL` | CONDITIONAL / S28 独立接收 | 受限 G126/AES key family、AES-CTR root PRG、OS CSPRNG。 |
| `ADAPTIVE_FULL_POOL_VIEW_CONDITIONAL` | CONDITIONAL / S28 独立接收 | 相关全池、未用槽、公开 transcript 和自适应请求进入 hybrid；优势项符号化。 |
| `SHUFFLE_OUTPUT_COMPOSITION_CONDITIONAL` | CONDITIONAL / S28 独立接收 | 可信不合谋 T、理想私有认证信道、完整 L。 |
| `SAMPLER_ABORT_BOUND_CONDITIONAL` | CONDITIONAL | ROM + 理想无放回和诚实 OS 熵贡献；有限样本自然 abort 为 0，不替代理论。 |
| `CONDITIONAL_SECURITY_DESIGN_ACCEPTANCE` | CONDITIONALLY_ACCEPTED（v1 n≤256 范围已关闭） | S28 条件证明异会话接收 + S30 v1 最终入口功能接收。v2 scope依同假设，待下一接收者确认 admission wrapper。 |
| `N256_FINAL_ENTRY_FUNCTIONAL_E2E` | PASS | 47/47 v1，K 边界及中间值，oracle 逐位匹配、weight K。 |
| `N1000_SCALE_GATE / VERSIONED_ENTRY_E2E` | PASS（功能试运行）/ independent v2 receipt pending | 两次 fresh material；最终复跑不同 UID、T 先退出、mTLS、oracle 正确；仅功能可行性，不是性能结论。 |
| `PR_AND_PROVENANCE` | CONDITIONAL | PR #29 仍指向 S29 `871b668…`、title/body陈旧；连接器未登录，未改远端。S30 本地源 commit `eac5151…`。 |
| `FORMAL_LAN_WAN_PERFORMANCE` | NOT_RUN | 无正式 1+5，无九指标，无速度排名。 |

## 8. 修改文件

运行时代码/测试（S30 源 commit `eac5151…`）：

- `VFSS/CMakeLists.txt`
- `VFSS/include/moe_topk/protocol_i_bmw16_conditional_secure_v2.h`（新增）
- `VFSS/src/moe_topk/protocol_i_bmw16_conditional_secure_v2.cpp`（新增）
- `VFSS/src/apps/moe_topk_bmw16_experimental_party_node.cpp`
- `VFSS/tests/moe_topk/experimental_bmw16_party_node_e2e_test.cpp`
- `VFSS/tests/moe_topk/protocol_i_bmw16_conditional_secure_v1_conformance_test.cpp`
- `VFSS/tests/moe_topk/protocol_i_bmw16_conditional_secure_v2_conformance_test.cpp`（新增）

文档（S30 报告提交）：

- `PROJECT.md`
- `docs/IMPLEMENTATION_PLAN.md`
- `docs/BENCHMARK_VALIDATION_PLAN.md`
- `docs/decisions/BMW16_S29_CONDITIONAL_SECURITY_DECISION_2026-10-09.md`
- `docs/reproduction/BMW16_S29_CONDITIONAL_SECURITY_VALIDATION_2026-10-09.md`
- `docs/decisions/BMW16_S30_CONDITIONAL_SECURITY_AND_SCALE_DECISION_2026-10-10.md`（新增）
- `docs/reproduction/BMW16_S30_INDEPENDENT_ACCEPTANCE_2026-10-10.md`（本文件）

禁止路径没有修改：`VFSS-baseline/`、`Papers/`、Agarwal/ADSMPC/CipherGPT 参考树。没有密钥、证书私钥、bundle/sidecar、构建物或逐次原始大日志入 Git。
