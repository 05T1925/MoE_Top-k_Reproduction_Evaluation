# BMW16 S31：conditional-v2 独立接收与服务器实验包决策

日期：2026-10-10  
方案身份：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED**  
决策范围：接收 S30 `conditional_secure_v2`（`1≤n≤1000`）并冻结实验运行合同；不重新开启 v1 安全门，不接受正式 LAN/WAN 性能。

## 1. 决定摘要

`conditional_secure_v2` **通过本轮独立 wrapper 接收与 n=1000 功能 E2E**。S31 没有发现需要修改运行时代码的问题。v2 限定 admission 至 `1≤n≤1000`、强制 authenticated TLS、拒绝 TEST_ONLY fault/tape 输入，并调用 S30/S26 唯一 Select/shuffle/DCF/membership/inverse party runtime 与材料 ABI。n=1000/K=80 使用 fresh streamed material、T/P0/P1 独立进程/UID、本机 mTLS；T 退出后双方在线完成，隔离 TEST_ONLY 控制器检查输出与冻结 oracle 一致、原序且 weight=K。重领被双方拒绝且 no-mask。

据此，**Protocol I BMW16-derived 的 n≤1000 条件安全代码候选已完成接收**，仅限本决策列出的假设和泄露。该句不表示无条件安全、恶意安全、正式生产部署或 V4 性能通过。

v1 `1≤n≤256` 的条件安全设计门沿用 S30 的 `CONDITIONALLY_ACCEPTED / CLOSED` 结论；本轮没有重开其 DCF、相关全池、shuffle/output 或 sampler 证明，也没有只因“条件安全”再要求重复审查。n≤1000 的组合优势和失败界按同一实际参数式重新计算；v2 没有新增算法或材料分支。

正式顺序保持 M6A/V3 → M6B/V4 → M7。V3 前置尚未完成，服务器材料只作为 `READY_FOR_FUTURE_V4` 包，不是 V4 已启动或已通过。正式 1+5 LAN/WAN、九指标完整实测、六方案排名均为 `NOT_RUN / NOT_MEASURED`。

## 2. 接收对象、分支和身份

| 对象 | Revision/状态 | 含义 |
|---|---|---|
| S29 最终接收基线 | `871b6683d15a34d117eb24353611d8ef48434aca` | PR #29 原 head；尚未合入 main。 |
| S30 被测 v2 runtime | `eac5151e96f3cc67d2bc664c0d300b51ef672e10` | 本轮复核的 v2 wrapper/CMake/CLI/source。 |
| S30 报告提交 | `d1e8d8dc0a4f594af6e7d6cdb4ce469a4d96797a` | S31 独立 worktree 起点；不是被测 binary 的 runtime revision。 |
| main/origin/main | `c3926c68fd14f270faa8b55234311071947fa080` | S31 起始核验相同；S30/S31 均未在该起点上合并。 |
| PR #29 检查 | 开放；base `main`；检查时 head=`871b6683…`，页面仍用 S26 旧标题/模板正文 | S30/S31 变更仍需以非强推 FF 更新候选；最终远端结果见 S31 报告。 |

S31 worktree：`codex/m6b-i-bmw16-s31`，起始 `d1e8d8dc…`，起始干净。runtime 文件与 S30 `eac5151…` 的 Git blob 内容一致；S31 只作文档、schema 和实验包更新。桌面主工作区及 VFSS-baseline、Papers、参考树、PR #28 未改动。

### 2.1 S30 源哈希表复核注记

S30 报告中 7 项“source SHA-256”有 3 项与 `git show eac5151…:<path>` 的原始提交 blob 一致；另外 4 项不能从所列 commit 复现。S31 对实际 runtime source commit 与对应 Git blob 重算并在独立接收报告列出校正值。此发现是 S30 报告的文件级 hash-index 差异：Git tree/source revision 明确、S31 fresh Release build 与本轮测试可复现，未发现 runtime 内容跨 revision 或二进制错配。S31 不把 S30 的四个错误摘要沿用为证据。

## 3. v1/v2 源码差分裁决

| 审查项 | 源码结论 |
|---|---|
| CMake/build | 实验适配器、v1、v2、TEST_ONLY failpoints 默认 OFF。v1/v2 各自要求实验适配器开启；v2 单独源、API、CLI 与 conformance target 只在 opt-in build 注册。正常 Release failpoints OFF；v2 明确拒绝测试 fault/tape 参数。 |
| v1/v2 admission | v1 要求 TLS 且 `1≤n≤256`；v2 要求 TLS 且 `1≤n≤1000`。n=0 与 n=1001 拒绝；n=1 走同一双方本地 shortcut。 |
| CLI/TLS | v2 CLI 进入 `party-conditional-secure-v2`，要求认证 TLS party 模式；12 条既有注册 stream 仍以 session/fingerprint/party/channel 绑定。缺 stream、错绑定或提前消费 fail closed，不把裸 FD 当降级路线。 |
| runtime/ABI | v2 wrapper 唯一实质变化是 admission 上限。随后调用相同 `protocol_i_bmw16_experimental_raw_score_mask_party`；Select、shuffle、DCF/uCMP、membership、inverse、材料/stream ABI、错误状态和消息顺序均未复制或改写。 |
| no-mask/输出 | 正常成功时仅输出本方原序 XOR mask share；抽样 ABORT 双方协议状态确认后无 mask。材料、TLS/通信及不变量错误仍分型；无 peer ACK 不标 PEER_AGREED。测试 oracle 重构仅在隔离 harness。 |
| 测试缺陷 | 未发现 v2 新增路径的 deterministic defect。S30 的 v1/n=1 harness 处理已在 S30 修正；本轮不重复改测试或 runtime。 |

v2 不是新算法，不更换 S4/S26 Select 逻辑或比较常数。n≤1000 功能结果不延伸到 `n>1000`。四层 R1–R4 仅为 Select 比较依赖层，不等于完整入口只有四轮。

## 4. 条件安全合同与适用域

以下是本项目在 v1/v2 中采用的条件模型，不作无条件安全声明：

- T 可信、离线、在线静默，不与任一在线方合谋；至多一个 P0/P1 半诚实腐化。
- 理论使用理想私有认证通道；本机 mTLS 是工程实现试验，不等于生产凭据注册、跨主机部署审计或对管理员/快照回滚的保证。
- DCF 完整序列化 M2UC v1 party key 的 source-specific G126/受限 126-bit AES key 子族辅助输入扩展假设；cryptoTools AES-CTR 根流 PRG、OS CSPRNG、独立材料 KeyGen 随机带。普通 AES-128 PRP 或 BGI15 Theorem 6 不直接替代 G126 假设。
- sampler 概率使用理想无放回模型；要将实际双方 OS 随机贡献 XOR 后的公开 SHA-256 counter sampler 接到该界，采用 ROM 假设并要求至少一方诚实、不可预测 OS 熵贡献。标准模型 SHA-256 sampler 界不主张。
- 已由 S28 对 S27 独立复核的 DCF single-key、相关全池/自适应 transcript、shuffle/output 条件论证继续适用于同一 runtime。v2 只扩展规模 admission，未改变安全游戏对象、key ABI、泄露 L 或状态转移；因此本决策不重开已关闭的 v1 引理。

允许泄露 L 包括 `public_z`/masked operands、匿名比较端点和打开 bit、pivot/U/V/W 匿名位置、共享匿名 handle、selected anonymous handle、key/slot ID 与访问顺序/次数、比较次数、frame phase/length、双方 sampler coin、统一算法 ABORT、本方 output share。L 不含 raw score、original_index 到匿名 handle 的映射、明文阈值/完整 mask 或另一方 shares。允许公开只是协议规格，并非把真实 transcript 直接交给模拟器；S28 条件 simulator 承担实际组合证明。

排除恶意方、两方合谋、T 合谋、标准模型 sampler 界、管理员读取双方文件、VM snapshot 回滚及生产证书/密钥生命周期。TLS/material/OS 故障是工程错误合同，断连时只给存活方 LOCAL_ONLY。

## 5. n=1000/K 全域参数与界

对任意合法 `1≤K≤1000`，`n=1000` 的两哨兵归约有效规模参数
`M=2·max(K,n+1−K)`，故 `M∈{1002,1004,…,2000}`；中位附近最小 `M=1002`，两端 `K=1/1000` 最大 `M=2000`。对全部这些偶数 M 复核 S16 的有限表达式：

- `s=min(M,ceil(sqrt(64M)))`：范围内为 254–358，严格小于 M，因此 bracket 项为 `2e^-31`。
- `U_cap=min(M,ceil(8M^(3/4)))`：对 `1002≤M≤2000` 均达到 M，U 超限项为 0。
- `W_cap=2ceil(sqrt(4M))+1`：范围内为 129–181，小于 M，W 项为 `M e^-32`。
- 对两个并行 Select task 用 union bound（不要求任务独立）：
  `p_abort,ideal(M) ≤ 2·(2e^-31 + M e^-32)`。

在 M=1002 时为 `2.5516686844723526×10^-11`；对 `n=1000` 所有 K 的最大理想上界出现在 M=2000，为 `5.0794361280715506×10^-11`。此为 S16 项目推导表达式按本次 n/K 可达域重新计算，不是 BMW16 Theorem 8，也不是实际样本成功率。仅在理想无放回采样、ROM 和至少一方诚实 OS 熵贡献前提下应用于现有 sampler；标准模型实际失败概率仍未证明。测试中的自然 ABORT 计数单列。

全池资源/优势范围：

| 参数 | n=1000 |
|---|---:|
| `p=next_power_of_two(n)` | 1,024 |
| `index_bits / comparison_bits` | 10 / 43 |
| `C=n(n−1)/2` | 499,500 |
| `N_DCF=2p+9C` KeyGen 数 | 4,497,548 |
| 每方材料槽 `2p+2+9C` | 4,497,550 |
| `q0=2N_DCF` root-stream blocks（串行 T 布局推导） | 8,995,096，未实测 |
| `N_DCF·b`，b=43 | 193,394,564 |
| `2N_DCF·b` 碰撞事件上界中的状态数 | 386,789,128 |
| 碰撞项 | `C(386,789,128,2)·2^-126`；不换算安全位数 |
| sidecar 文件布局 | 4,967,527,664 bytes/party（解析布局/文件观测需区分） |

条件优势沿用 S28 的符号项：
`Adv_total ≤ Adv_root((q_t)_t) + N_DCF·b·Adv_G126_aux + C(2·N_DCF·b,2)·2^-126 + Adv_sampler_ROM`，
另带理想通道/诚实 T/OS 假设。`Adv_G126_aux` 与 `Adv_root` 不是本轮测量值，也不替换成标准 AES-128 的无条件结论。S28 已条件接收的视图模拟涵盖同 key 两次 uCMP DCF Eval、未用池、打开 bit、自适应 key ID、两个 Select task 共用 handle、forward/inverse 与 output share。S31 核对 v2 wrapper 不改变其前提或字段，不重演该证明。

## 6. S31 新构建和验证

构建环境：WSL2 Ubuntu 24.04、Linux 6.6.87.2-microsoft-standard-WSL2、GCC 13.3.0、CMake 3.28.3、OpenSSL 3.0.13。Release adapter/v1/v2 ON，failpoints 与 FSS DCF counters OFF。另一个 default-off 配置将 `sytorch` 成功构建，缓存中的 adapter/v1/v2/failpoint 均 OFF，target help 没有 BMW16 target。

测试顺序和结论：

1. **Conformance / targeted CTest**：第一组 9/9，覆盖实验 party、v1/v2 wrapper、DCF、uCMP、transport、score input、Protocol I parallel shuffle、Protocol III 三进程 E2E；第二组 9/9，扩展覆盖 Protocol I priority/DCF、reverse shuffle、CmpAgg/三进程、paper alignment、Protocol III raw-score pipeline/三进程。两组分开记录，不能合称全仓 CTest。
2. **冻结 S4/C++ common tape**：39/39 轨迹差分通过；本轮使用同一共同随机 tape、不是“相同 seed”假设。
3. **v1 fresh-material mTLS E2E**：47 个实例 SUCCESS，原序 oracle mask/weight K 通过；47/47 durable re-claim 被拒，双方 exit 20/no-mask。不同 UID 文件隔离检查通过。错在线 peer 与认证失败各有有界 failure/no-mask；自然算法 ABORT 为 0/47。v1 注入测试控制在 API 入口被拒，未把注入样本计入自然概率。
4. **v2 `(1000,80)` fresh-material E2E**：本轮唯一新 n1000 E2E；T/P0/P1 exit 0，三进程独立 UID，本机 mutual TLS；T `T_online=false`，两方 oracle success、原序 mask weight 80；重领双方 exit 20/`DURABLE_CLAIM_REJECTED`/mask none；UID 隔离检查 PASS。自然算法 ABORT 为 0/1，不可据此估算概率。在线总时间/通信九指标未建立，不作为 performance 数据。
5. **未运行**：V4 1+5 LAN/WAN；全仓所有 44 项 CTest（本轮只运行两组各 9 个有针对性目标）；n1000 其他 K 的独立进程 E2E；n≥10,000 材料生成。

S31 运行命令、二进制 SHA、raw record SHA 与失败分型列在 [S31 独立接收报告](../reproduction/BMW16_S31_INDEPENDENT_ACCEPTANCE_2026-10-10.md)。原始日志、tape/trace、证书和临时材料均在仓库外 artifact root。

## 7. 服务器实验准入决策

- **代码准入**：v2 对 n≤1000 默认关闭/显式 opt-in，可调用并独立接收；n=1000/K=80 全链功能 E2E 在本轮通过。
- **规模准入**：n=1000 每方 sidecar 约 4.97 GB，T 需要流式发材，单次两方材料投递约 9.94 GB；本机 S31 容量足以完成一个功能 E2E。正式 1+5 需约 6 倍网络材料量，若保留 6 次 party 包建议至少 80 GB scratch；按次串行并验证销毁后建议至少 30 GB scratch。8 vCPU/16 GiB RAM 是 S30 建议配置，需服务器实测 T/party RSS；网络带宽/RTT 未测。
- **更大规模**：10k 起 sidecar 约 540 GB/party；100k/1M 约 57.3 TB/6.05 PB 每方。当前全池材料路线 `PRECHECK_REJECTED/NOT_RUN`，不因租赁主机而自动开放。
- **性能状态**：准备资产 `READY_FOR_FUTURE_V4`；V3 前置仍未关闭，正式 V4 `NOT_RUN`。没有九指标正式测量，不做六方案排名。

## 8. 分项门禁

| Gate | S31 结论 | 依据/限制 |
|---|---|---|
| `V1_CONDITIONAL_SECURITY` | `CONDITIONALLY_ACCEPTED / CLOSED` | S30+S28 已接收的 n≤256 条件模型；本轮不重开。 |
| `V2_WRAPPER_INDEPENDENT_ACCEPTANCE` | `PASS` | 源码差分、默认 OFF/TLS/admission/negative conformance、v1/core E2E 与 v2 fresh n1000 E2E。 |
| `N1000_FUNCTIONAL_E2E` | `PASS (1/1, K=80)` | 新 fresh-material mTLS oracle E2E；不代表其它 K 实跑。 |
| `N1000_SECURITY_PARAMETER_SCOPE` | `CONDITIONAL / n≤1000, 1≤K≤n` | 全 K 的理想/ROM 抽样失败界与 S28 条件优势表达适用；源/G126/PRG/CSPRNG/T/channel 前提不可省略。 |
| `PR_AND_MAIN_STATUS` | `PENDING_S31_FAST_FORWARD` | S31 检查时 PR #29 仍指向 S29 head，未合入 main；GitHub connector 不可用。推送/PR metadata 结果待 S31 报告最后一节。 |
| `BENCHMARK_PACKAGE_READY` | `READY_FOR_FUTURE_V4`（准备资产） | runbook+schema+资源表已交付；完整线上 wire/全路径 PRG 计数和九指标仍需服务器 instrumentation/运行，V3 未完成。 |
| `FORMAL_V4_LAN_WAN` | `NOT_RUN` | 无预热+5 LAN/WAN。 |

下一步：租机后按 runbook 校验镜像和磁盘，再为同一冻结 input/oracle 建全对全 I、I+AAV86 和本条件入口的逐方案 run manifest；先完成 V3 和九指标采集，再决定是否进入正式 V4。`conditional_secure_v2` 不需要为同一个 wrapper 再开一轮泛化安全证明。
