# BMW16 S31：conditional-v2 独立接收报告

日期：2026-10-10  
方案身份：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED**  
接收范围：S30 `conditional_secure_v2`（`1≤n≤1000`）源码/入口接收、最终候选有界功能验证及服务器实验准备资产。  
明确排除：BB90 原算法复现、BMW16 Algorithm 7 逐字复现、恶意安全、生产 PKI 运维、V4 正式 LAN/WAN 性能矩阵。

## 1. 接收结论

本轮从 S30 报告提交建立隔离工作树，独立复核 v2 wrapper 并在该源码上重新构建。源码差分没有发现 v2 admission/TLS/调用边界缺陷。新构建完成后，按 conformance → common-tape differential → 三进程 E2E 顺序验证；n=1000/K=80 使用 fresh material、T/P0/P1 不同 UID、本机 mTLS，成功输出由隔离控制器核验为原序、长度 1000、二值且 weight=80 的冻结 oracle mask；重领材料被双方拒绝且无 mask。

因此本报告判定：**Protocol I BMW16-derived 的 n≤1000 条件安全代码候选已完成接收**，限下文逐项列出的条件模型、泄露函数和 v2 的 `1≤n≤1000` admission。此结论表示条件安全设计和 wrapper/功能接收完成；不表示无条件安全、恶意安全、跨主机生产部署已获接收，亦不代表性能验收。S30 的 v1 `1≤n≤256` 门保持 `CONDITIONALLY_ACCEPTED / CLOSED`，本轮没有重开。

S31 未改运行时代码；本轮测试所用 party runtime 源码 revision 是 S30 的 `eac5151e96f3cc67d2bc664c0d300b51ef672e10`。S31 报告提交与被测源码、二进制分别登记，不能混为一个 revision。服务器实验包为 `READY_FOR_FUTURE_V4`；M6A/V3 前置未关闭，V4 正式矩阵 `NOT_RUN`。

## 2. 接收对象、工作区和远端状态

| 对象 | 核验值 | 说明 |
|---|---|---|
| S29 最终接收基线 | `871b6683d15a34d117eb24353611d8ef48434aca` | PR #29 原远端 head。 |
| S30 被测 runtime | `eac5151e96f3cc67d2bc664c0d300b51ef672e10` | v2 wrapper、CMake、CLI 与共用 runtime 的源码版本。 |
| S30 报告/本轮起点 | `d1e8d8dc0a4f594af6e7d6cdb4ce469a4d96797a` | 文档提交；不是 S30 被测二进制源码 revision。 |
| S31 分支 | `codex/m6b-i-bmw16-s31` | 起始工作树干净；所有修改限于本报告、决策、计划、基准准备文档和 schema。 |
| S31 起始 main / origin/main / merge-base | `c3926c68fd14f270faa8b55234311071947fa080` | 起始时三者一致；实验链未在该 main 中。 |
| 桌面原工作树 | `feat/m6a-performance-evaluation`，有既存差异 | 保留，未在其中工作或覆盖。 |
| PR #29（本轮起始核验） | Open，base `main`，head `codex/m6b-i-bmw16-s26`=`871b668…` | 未合入 main。GitHub 页面仍显示 S26 旧标题与模板正文；没有实际 CI 通过证据。S31 最终 fast-forward 结果见本报告最后一节。 |
| PR #28 / `VFSS-baseline/` / `Papers/` / 本地参考树 | 未改动 | 禁止路径未进入差异。 |

本轮使用 protocol-reproduction skill 和仓库 `AGENTS.md` 约束。证据分类为：论文/既有安全决策、S30 项目派生 wrapper、S31 源码检查、本轮新执行结果和仍未实测项目；功能测试不替代密码学论证。

## 3. S30 来源身份与哈希复核

S30 决策/报告的 runtime commit 为 `eac5151…`，报告提交为 `d1e8d8d…`。S31 从报告 head 检查其父链并针对 runtime commit 的 Git tree/blob 重新计算文件 SHA-256。S30 报告所列 7 个源文件摘要中，v2 header、v2 wrapper cpp、v2 conformance 三个可由该 commit 复现；CMake、party-node、E2E harness、v1 conformance 四个不能复现。下面列出 7 个文件的**本轮按被测 Git blob 复算值**：

| 被测源码文件 | Git blob 字节 SHA-256（S30 runtime commit `eac5151…`） |
|---|---|
| `VFSS/CMakeLists.txt` | `8d998ee901248cf6dc758e3cab855d90d865dd7bc59484f79640a54cb7ecfbef` |
| `VFSS/include/moe_topk/protocol_i_bmw16_conditional_secure_v2.h` | `4173d57590b9e1f95d5379a3727505ea3e49172125fc4d6c52701c492868c5a8` |
| `VFSS/src/moe_topk/protocol_i_bmw16_conditional_secure_v2.cpp` | `7b1088371d441b835069de9491d17527e1c89bc1ef3f2d578ee1863d4d810b96` |
| `VFSS/src/apps/moe_topk_bmw16_experimental_party_node.cpp` | `d63faad73b655a604c631bd0b0c65cb7b7a173db5d28e20ae5a39f40ae3c7af9` |
| `VFSS/tests/moe_topk/experimental_bmw16_party_node_e2e_test.cpp` | `b5240ec4a244c9f7e9e6f1b97a8c76fef46b8a0c16bce373c5e297b8ff479d06` |
| `VFSS/tests/moe_topk/protocol_i_bmw16_conditional_secure_v1_conformance_test.cpp` | `5c068eb8a5755a78c72e9fae7808c1d0b65b40f501bfafe7ecf8e752a65fd997` |
| `VFSS/tests/moe_topk/protocol_i_bmw16_conditional_secure_v2_conformance_test.cpp` | `370ff3d9d11ad8a01f61ce5a78c624250756cb169a4eb58c444b67fbbea695ca` |

这是 S30 报告的文件级摘要索引错误，不是 runtime diff 或攻击证据。S31 由 `git show eac5151…:<path>` 对照文件内容并从该 revision 新建 Release；本轮运行的 binary 与该构建目录对应。S31 没有改这些 runtime 文件，故本轮发现不改变 ABI、泄露、消息、材料或 runtime hash。Windows checkout 的行尾转换会使工作目录原始文件哈希不同；上表固定为 Git tree 中被测提交的 blob 字节身份。

## 4. v1/v2 入口差分与条件安全范围

### 4.1 源码审查结果

审查定位（行号对应被测 revision `eac5151…`）：`VFSS/CMakeLists.txt:24–43,102–128,148–156,741–751` 定义 default-OFF 及显式依赖/目标注册；`VFSS/include/moe_topk/protocol_i_bmw16_conditional_secure_v2.h` 只为已存在 party config/material/output 提供版本化 API；`VFSS/src/moe_topk/protocol_i_bmw16_conditional_secure_v2.cpp:8–29` 强制 authenticated transport、`1≤n≤1000`、拒绝 TEST_ONLY fault，随后委托唯一核心 party 函数；`VFSS/src/apps/moe_topk_bmw16_experimental_party_node.cpp:676–691,743–757` 接入 v1/v2 CLI 并选用对应 wrapper；E2E harness 的 v2 实例在 `VFSS/tests/moe_topk/experimental_bmw16_party_node_e2e_test.cpp:740–755,915–932`。源码审查结果和本轮执行结果分开记录。

| 项目 | 复核结论 |
|---|---|
| 默认构建 | 实验适配器、conditional-v1、conditional-v2、TEST_ONLY failpoint 均默认 OFF。v1/v2 目标要求适配器依赖显式开启；默认配置没有注册 BMW16 conditional target。 |
| v1/v2 admission | v1 强制 authenticated TLS 且 `1≤n≤256`；v2 强制 authenticated TLS 且 `1≤n≤1000`。n=0、n=1001 被拒；n=1 走双方本地 shortcut。 |
| v2 CLI/API | `party-conditional-secure-v2` 调用版本化 public API；拒绝 TEST_ONLY fault/tape 参数。API 校验 TLS/authenticated stream、输入和会话参数后才进入核心路径。 |
| 通道 | 复用已注册的 12 条 authenticated TLS streams，以 party、session、fingerprint 和 channel 绑定；缺 stream、错身份/参数、已消费 stream fail closed。没有裸 FD 自动回退。 |
| 核心调用 | v2 只改变 admission cap，调用唯一 `protocol_i_bmw16_experimental_raw_score_mask_party`。Select、前/逆 shuffle、DCF/uCMP、membership、材料 ABI、边生成和状态合同未复制或改写。 |
| 输出与失败 | 成功只发布每方原序 XOR mask share；隔离 TEST_ONLY 控制器在 parties 退出后核对完整输出。正常算法 abort 要双方 agreement 且不出 mask；材料/TLS/通信/不变量错误分型，无 ACK 时不声称共同确认。 |
| 缺陷 | 未发现 v2 新增代码或参数的可复现 runtime 缺陷；没有为了制造代码变更而重写 runtime。 |

### 4.2 假设、泄露和已关闭的条件证明

角色及腐化模型：T 可信、离线发材、在线静默、不与任一在线方合谋；P0/P1 中至多一方半诚实腐化。理论分析以理想私有认证信道为前提；本机 mTLS 只验证本地工程路径，不能替代生产证书注册、轮换、远程主机管理或管理员/快照回滚安全。

密码学前提延续 S27/S28 经异会话复核的完整条件证明：M2UC v1 完整序列化 DCF party key 的 source-specific `G126`/受限 126-bit AES-key 子族辅助输入 PRG/PRP 假设；cryptoTools AES-CTR root-stream PRG；OS CSPRNG；离线 KeyGen 独立随机带；理想私有认证信道。普通 AES-128 PRP 假设或 BGI15 Theorem 6 本身不直接证明该序列化 ABI。Sampler 界以理想无放回抽样为数学对象；接到双方公开 XOR coin + SHA-256 counter 实现时依赖 ROM 且至少一方提供诚实、不可预测的 OS 熵。标准模型下实际 SHA-256 sampler 的可量化抽样优势不主张。

许可泄露函数 `L` 包括：public masked operands 与 forward/inverse `public_z`；匿名比较端点及打开 bit；pivot、U/V/W 位置；两 Select task 共用的匿名 handle；selected anonymous handle；key/slot ID、访问顺序与次数；比较调用数；阶段、frame 长度/相位；双方可见的 sampler coin；统一算法 ABORT；腐化方自己的 output share。`L` 不含 raw score、`original_index` 到匿名 handle 的映射、明文阈值/完整 mask 或对方 share。错误的本地诊断与工程/通信失败状态不冒充正常概率 abort。

S28 独立复核 S27 的单 key、相关整池/自适应查询、同 key 两次 Eval、未使用 key、forward/inverse shuffle 及 output-share 条件模拟。S30 已按这些明确假设关闭 v1 `n≤256` 条件安全设计门。v2 wrapper 不改被证明的 runtime、安全游戏、材料 ABI 或 L；S31 接收的是新增 n≤1000 admission wrapper 和该范围参数重算/功能路径，不是把条件结论变成无条件保证。定义各方完整 view、hybrid 和证明依据见 S27/S28 原始文档；S31 不将有限枚举或成功样例另称为一般证明。

### 4.3 n=1000 所有 K 的范围审计

对 `n=1000` 任意 `1≤K≤1000`，两哨兵归约的偶数有效规模 `M=2·max(K,n+1−K)` 遍历 `1002,1004,…,2000`。本轮用 checked 整数范围复算参数端点：

| 项 | 实际参数规则 | 全 K 范围 | 对界的作用 |
|---|---|---:|---|
| `p` | `next_power_of_two(n)` | 1024 | padded 域。 |
| `index_bits / comparison_bits` | `ceil(log2 n)` / `33+index_bits` | 10 / 43 | 实际稳定键比较宽度。 |
| `C(n,2)` | `n(n−1)/2` | 499,500 | 真实无序端点对数。 |
| `N_DCF` | `2p+9C` | 4,497,548 | DCF KeyGen 次数；不是每方全部 slot 数。 |
| slots/party | `2p+2+9C` | 4,497,550 | 上式另含两个 shuffle slots。 |
| `s` | `min(M,ceil(sqrt(64M)))` | 254–358 | 小于 M；bracket 项为 `2e^-31`。 |
| `U_cap` | `min(M,ceil(8M^(3/4)))` | 1002–2000 (=M) | `U` 超限分支不可达，预算为 0。 |
| `W_cap` | `2ceil(sqrt(4M))+1` | 129–181 | 小于 M；窗口项为 `M e^-32`。 |

两个并行 task 使用 union bound，不假设 task 独立：
`p_abort,ideal(M) ≤ 2·(2e^-31 + M·e^-32)`。M=1002 时为 `2.55166868447235×10^-11`；n=1000 全 K 最坏值在 M=2000，为 `5.07943612807155×10^-11`。这仅是项目既定 S16 参数在理想均匀无放回抽样模型下的推导；接到实际 sampler 还需上述 ROM/诚实熵条件，不是 BMW16 Theorem 8 数值，且 0/1 次自然失败观察不构成概率证明。

资源/条件优势项仍采用 S28 的符号表达，不制造安全位数：对 n=1000，`N_DCF=4,497,548`，每方槽 `4,497,550`，布局推导的 T root-stream blocks `q0=2N_DCF=8,995,096`；`b=43`，碰撞项按 `C(2N_DCF·b,2)·2^-126`；`Adv_G126_aux` 与 `Adv_root((q_t)_t)` 保留符号。sidecar 解析布局约 `4,967,527,664 bytes/party`。

## 5. 本轮独立验证与命令

环境：WSL Ubuntu 24.04、Linux `6.6.87.2-microsoft-standard-WSL2`、x86_64、GCC 13.3.0、CMake 3.28.3、OpenSSL 3.0.13。构建目录位于仓库外 `/tmp/moe_bmw16_s31_conditional_release`。配置为 Release、`BUILD_TESTING=ON`、实验适配器/v1/v2 ON、failpoints/DCF PRG counters OFF。

### 5.1 构建和定向测试

配置命令：

```bash
cmake -S VFSS -B /tmp/moe_bmw16_s31_conditional_release \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON \
  -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON \
  -DMOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V1=ON \
  -DMOE_TOPK_ENABLE_CONDITIONAL_BMW16_SECURITY_V2=ON \
  -DMOE_TOPK_ENABLE_TEST_ONLY_BMW16_FAILPOINTS=OFF \
  -DMOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=OFF
cmake --build /tmp/moe_bmw16_s31_conditional_release --target \
  moe_topk_bmw16_experimental_party_node \
  moe_topk_bmw16_experimental_party_node_e2e_test \
  moe_topk_bmw16_experimental_party_test \
  moe_topk_bmw16_conditional_secure_v1_conformance_test \
  moe_topk_bmw16_conditional_secure_v2_conformance_test --parallel 2
```

定向 CTest 分两组实际运行，各 9/9 PASS（总计 18 个被选测试 PASS，不声称全仓 CTest）：

```bash
ctest --test-dir /tmp/moe_bmw16_s31_conditional_release --output-on-failure \
  -R 'moe_topk_(bmw16_experimental_party_test|bmw16_conditional_secure_v1_conformance_test|bmw16_conditional_secure_v2_conformance_test|m3_protocol_iii_three_process_e2e_test|m1_dcf_conformance_test|m2_ucmp_conformance_test|m2_transport_conformance_test|m2_score_input_conformance_test|m2_parallel_shuffle_conformance_test)'
ctest --test-dir /tmp/moe_bmw16_s31_conditional_release --output-on-failure \
  -R 'moe_topk_(m3_secure_combine_test|m3_raw_score_pipeline_test|m3_raw_score_three_process_e2e_test|m2_priority_key_test|m2_priority_dcf_conformance_test|m2_reverse_shuffle_model_test|m2_cmpagg_conformance_test|m2_cmpagg_process_e2e_test|m2_paper_core_alignment_test)'
```

默认 OFF 配置 `/tmp/moe_bmw16_s31_default_off`：`sytorch` 目标构建成功；实验适配器/v1/v2/failpoint cache 均 OFF，target help 未注册 BMW16 目标。全仓登记的 44 项 CTest 本轮 `NOT_RUN`；并未把缺少构建目标的历史全仓记录算成失败或通过。

共同随机 tape 命令：

```bash
py -3 experiments/TEST_ONLY_BMW16_S12/run_common_tape_matrix.py \
  --wsl-distro Ubuntu-24.04 \
  --cpp-test /tmp/moe_bmw16_s31_conditional_release/moe_topk_bmw16_experimental_party_test \
  --s4 experiments/TEST_ONLY_BMW16_S7_COMPOSITION/select4r_s4_source.py \
  --out C:\Users\28641\.codex\artifacts\BMW16_S31_20261010\common_tape
```

结果 39/39 PASS，逐轨迹比较 S4 Python 与 C++ 的抽样、pivot、U/V/W、边集、结果、自然 abort 类别及 selected handle。输入随机 tape 相同，不将不同 PRNG 的“相同 seed”误作共同随机性。矩阵 SHA-256 为 `FA030858053B1AB421309C3466BBFF90A9C819F065E67432CCCB100DE6435E88`。

### 5.2 独立进程 E2E

```bash
wsl.exe -d Ubuntu-24.04 -u root -- \
  /tmp/moe_bmw16_s31_conditional_release/moe_topk_bmw16_experimental_party_node_e2e_test \
  /tmp/moe_bmw16_s31_conditional_release/moe_topk_bmw16_experimental_party_node \
  --conditional-secure-v1
wsl.exe -d Ubuntu-24.04 -u root -- \
  /tmp/moe_bmw16_s31_conditional_release/moe_topk_bmw16_experimental_party_node_e2e_test \
  /tmp/moe_bmw16_s31_conditional_release/moe_topk_bmw16_experimental_party_node \
  --conditional-secure-v2-n1000
```

| 运行 | 本轮结果 | 限制 |
|---|---|---|
| v1 | fresh-material 47/47 oracle success；47/47 durable re-claim 双方 exit 20/no-mask；不同 UID 隔离通过；错 peer/认证错误无 mask；自然算法 abort 0/47。 | 是接收 v1 历史已关闭条件门的代表复跑，不扩大 v1 上限。 |
| v2 `(1000,80)` | 新 fresh-material T/P0/P1；T 和双方 receive/online exit 0；mutual TLS 1.3；T `T_online=false`；oracle success、original-order、weight=80；replay 双方 exit 20/no-mask；UID 隔离通过。自然 abort 0/1。 | 本机功能 E2E，不是性能样本；仅直接跑此 n/K，不推广成 n1000 全 K 实测。 |

v2 本轮记录：session `138245308544`；测试配置标识 `0x2030100080` 只标识测试输入/session，party 在线随机币和材料 KeyGen 仍为 OS CSPRNG。sidecar 日志观察为 `4,967,527,664 bytes/party`、shell `2,013,901 bytes/party`；双方 TLS sent aggregate `9,941,055,454 bytes`，receiver 各记录 `4,970,527,727 bytes`。T 交付记录 `425,564,551 µs`、离线 elapsed `531,602,269 µs`。这些是这次功能日志原值，仅供运行身份/容量核对，九指标正式性能字段仍 `NOT_MEASURED`。

原始日志未写入仓库：v1 `v1-independent-e2e.log` SHA-256 `82DD12909136939A7B1F1C5A294D37FD20E489832FCA28E0B784F1CA68F49142`；v2 `v2-n1000-e2e.log` SHA-256 `DD1AA8B7D3EF4A144BC58B12F58865F90893ABB8DD8B645B7849A7977813E28C`。

### 5.3 构建二进制身份

被测源码 revision=`eac5151e96f3cc67d2bc664c0d300b51ef672e10`；构建选项如本节 5.1。SHA-256：

| Release binary | SHA-256 |
|---|---|
| `moe_topk_bmw16_experimental_party_node` | `1484bcb891fba3ba70648e10e742f11c636b83924b90577e2fc81317f51b010a` |
| `moe_topk_bmw16_experimental_party_node_e2e_test` | `54ea64b1502f230d0a2414b9177c4d23272a5eb94b3568d8b2c1062dbac2062c` |
| `moe_topk_bmw16_experimental_party_test` | `53d95cc8be561307540724320d472d05af63d53ecc6793044b5112128844109b` |
| `moe_topk_bmw16_conditional_secure_v1_conformance_test` | `4d549a540abb13b78ee85c6bb45ef6287afe26588fda8f81890d12ef01f220bc` |
| `moe_topk_bmw16_conditional_secure_v2_conformance_test` | `e8923636463904fcce067eaff743c6eb91e1148bb42af8770b080b5f096c178f` |

构建物留在 `/tmp/moe_bmw16_s31_conditional_release`，未纳入 Git。构建与定向 CTest 原始日志留在外部 artifact root。逐文件外部 SHA-256 索引（含 common-tape 文件）位于 `C:\Users\28641\.codex\artifacts\BMW16_S31_20261010\sha256-index.json`，索引 SHA-256 `8BCE620870E916751FD8524DBC25FC1073CACC66EE853CC881C8056E751651B8`。artifact root 为 `C:\Users\28641\.codex\artifacts\BMW16_S31_20261010\`；材料、party shares、证书及私钥不进入仓库或本报告。

## 6. 服务器实验包与指标状态

服务器 runbook 与逐次 JSON schema 在本轮交付。schema `$id` 使用本地稳定 URN；随机性字段允许 `NOT_USED_n1_shortcut`，以免 n=1 无随机性的合法执行无法记录。schema 是结构校验，不替代跨字段约束检查（例如必须保证 `k≤n` 和 `SUCCESS` 才能设置 oracle mask 发布标记）。

实验包规定 source/build/binary/input/oracle/hash/session/fresh material 绑定，证书和身份预置合同、1 次预热+5 次正式运行、失败尝试纳入分母、LAN/WAN 配置与资源预检。九项主指标各自列出边界和采集点：`offline_time_ms`、`offline_material_total_bits`、`online_time_ms`、`online_comm_total_bits`、`online_comm_per_party_bits`、`online_rounds`、`online_prg_calls_total`、`comparison_edges_total`、`total_time_ms`。在线因果层按完整入口 DAG 分列；当前布局可推导 12 个应用消息相位，不把 Select 的 R1–R4 写成总协议四轮。AES 调用、AES block、DCF Eval、sampler counter word 与 PRG stream call 独立计数。

本轮 loopback E2E 日志没有提供可比 LAN/WAN 九指标：正式九指标均不从该单次日志填值；wire byte 和阶段时长只作为 E2E 原始记录保留。S31 package 状态为 `READY_FOR_FUTURE_V4`，不是 `V4_ACCEPTED`。n=10,000 及更大规模按全池容量被拒绝生成，状态保留 `PRECHECK_REJECTED / NOT_RUN`，不外推 n=1000 性能。

## 7. 修改文件

S31 最终文档提交中的修改文件仅为：

1. `PROJECT.md` — 同步 S31 v2 独立接收、条件安全范围、服务器包状态及 PR/main 时间点。
2. `docs/IMPLEMENTATION_PLAN.md` — 同步 v2 wrapper 已接收，保持 V3→V4 正式顺序，并将更新日期改为 S31。
3. `docs/BENCHMARK_VALIDATION_PLAN.md` — 加入 S31 server-package 和指标边界说明。
4. `docs/decisions/BMW16_S31_CONDITIONAL_V2_ACCEPTANCE_AND_SERVER_PACKAGE_2026-10-10.md` — 新增条件安全/范围决策；纠正采样整数取整范围为 `s=254..358`、`W_cap=129..181`。
5. `docs/reproduction/BMW16_S31_INDEPENDENT_ACCEPTANCE_2026-10-10.md` — 本报告。
6. `docs/reproduction/BMW16_S31_SERVER_BENCHMARK_PACKAGE_2026-10-10.md` — 新增未来 V4 runbook 与九指标合同。
7. `docs/reproduction/BMW16_S31_RUN_RECORD_SCHEMA.json` — 新增逐次记录 schema，含 n=1 随机性枚举。

无 VFSS runtime/test 源文件改动；此次接收改正了 S30 报告的源文件 SHA 索引，不改 S30 历史提交。

## 8. 分项门禁

| Gate | S31 判定 | 证据/限制 |
|---|---|---|
| `V1_CONDITIONAL_SECURITY` | `CONDITIONALLY_ACCEPTED / CLOSED` | S30+S28 已关闭，`n≤256`；本轮不重开。 |
| `V2_WRAPPER_INDEPENDENT_ACCEPTANCE` | `PASS` | v1/v2 源码差分、默认 OFF/TLS/admission/fail-closed、conformance 和 v2 n1000 独立 E2E。 |
| `N1000_FUNCTIONAL_E2E` | `PASS (1/1, K=80)` | 本轮 fresh-material 本机 mTLS；仅这一配置是本轮 n1000 直接 E2E。 |
| `N1000_SECURITY_PARAMETER_SCOPE` | `CONDITIONAL / n≤1000, 1≤K≤n` | S28 条件证明和本报告全 K 理想/ROM abort 计算；G126/root PRG/OS CSPRNG/T/channel/L 前提均保留。 |
| `PR_AND_MAIN_STATUS` | `PENDING_S31_FAST_FORWARD` | 起始远端 PR head `871b668…`、base main、未合入；S31 本地提交后再记录 push/PR 操作结果。 |
| `BENCHMARK_PACKAGE_READY` | `READY_FOR_FUTURE_V4` | runbook/schema/预检资产已交付；服务器完整指标未测。 |
| `FORMAL_V4_LAN_WAN` | `NOT_RUN` | V3 前置未关闭；本轮未运行 1+5。 |
| `FULL_REPOSITORY_CTEST` | `NOT_RUN` | 本轮两组定向 9/9；全仓 44 项未运行。 |

## 9. 下一阶段执行清单

在租赁服务器任务开始前：先关闭并记录 V3 正式门；读取本报告与 server package，冻结 PR 接收后的最终 commit；准备 ≥8 vCPU/16 GiB RAM 的起始实例、≥30 GB scratch 的串行单次配置（保留六份重复包时建议 ≥80 GB）；按实际 sidecar preflight 保留至少 20% 空间余量；由受信任运维预置非 TEST_ONLY 的 T/P0/P1 证书与独立身份；从最终 commit 重建 default-OFF 与条件入口 Release，保存 cache、binary、input/oracle、manifest 和原始索引 hash；同一输入运行全对全 I、I+AAV86 与本候选，先预热 1 次，再正式 5 次，全部失败尝试入库；分别测 LAN/WAN RTT、带宽、分方线速字节、九指标、RSS 与 PRG/AES/DCF 各自计数；不得把 S31 loopback 记录拼入该矩阵。

若服务器/指标采集齐全且 V3 通过，再进入正式 V4；若 n=10k+，先重做 checked 资源准入，不自动生成全池材料。Protocol III 派生路线和六方案总验收仍按各自真实进度另行判定。
