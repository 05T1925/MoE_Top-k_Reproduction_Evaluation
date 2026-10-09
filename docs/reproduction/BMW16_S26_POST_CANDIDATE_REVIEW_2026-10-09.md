# BMW16 S26：S25 实验候选后续技术复核报告

日期：2026-10-09
身份：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL**

## 1. 接收来源与独立性边界

复核目标：S25 最终 HEAD `e8e06c90ebb32cd975afcb582bb25b2962bdad19`；运行时代码提交 `112cd8a6d641ded1be5244b317ffb5c42f3b86d1`。HEAD 的父提交为 `eff03b9b8bdcdc9081b0babb25780016210680b4`；runtime 提交父提交为 `c3926c68fd14f270faa8b55234311071947fa080`。`main`、`origin/main`、merge-base 均为 `c3926c68fd14f270faa8b55234311071947fa080`。复核分支从 S25 HEAD 建立为 `codex/m6b-i-bmw16-s26`，未改写 S25 提交历史。

本轮实际运行时代码及回归修正固定在提交 `ded433d636a3df929380bdce0ff22f4ce6121453`；该提交只包含 party-node、E2E harness 和 common-tape CRLF 校验脚本。本文、决策和状态同步单独提交，避免将文档提交误报为被测 runtime revision。

工作区基线：S25 工作树在复核时干净；桌面主工作区已有的 `PROJECT.md`、实施计划、论文登记、本地参考登记及未跟踪文档/PDF 保持原样。本 S26 分支的 S25→S26 修改只涉及实验实现、TEST_ONLY harness、dated 决策/报告和当前状态注记；没有纳入主工作区差异、VFSS-baseline、论文、参考树、密钥、证书、sidecar、构建物或逐次大日志。相对 `origin/main` 的 S25 候选来源清单为 38 个路径；禁止路径计数为 0。

**独立性限制：**当前聊天上下文此前参与 S25 工作，所以本报告不能满足“未参与 S25 编写或运行的接收聊天”这一资格条件。下面的源码审计与本窗口复跑是真实证据，但不能标成异聊天独立接收 PASS。Draft PR 保留为待外部接收的审查候选。

## 2. 接收对象与功能链检查

对 S25 runtime tree、party API、T 发材、材料 shell/sidecar、TLS 收发和 E2E harness 做源码检查。唯一完整路径仍为：

`signed Q20.12 raw-score add shares → stable priority-key adapter → forward shuffle → 两份并行 Select 的 R1–R4 → DCF/uCMP membership → 同置换 inverse shuffle → 原序 XOR mask shares`。

S25 报告所述设计边界在本轮源码中得到确认：T 只使用公开参数并在在线输入之前发材退出；P0/P1 分方取得材料并调用同一 VFSS party API；party 路径没有重构 score、original index、selected key 或完整 mask；只有隔离 harness 重构输出 shares 对冻结 oracle。样本、pivot/集合位置、匿名比较端点和 bit、public masked operand、匿名 selected handle、key ID/顺序、访问数与通信长度属于项目**允许公开的实验 transcript 草案**，不代表联合单方 view 已经证明安全。

具体共用模块复核：

- `dcf.{h,cpp}` 的可选 counters 是编译期开关，以 atomic 计数 API 的 KeyGen/Eval 与 AES helper 调用；不读取或改变比较输入/输出。计数项“helper 调用数”不等于 AES block 数，也不覆盖所有 cryptoTools PRG 调用。
- shuffle-only material 配置显式省略 CmpAgg 池，默认值仍维持旧路径；序列化版本/标志有解析校验。party 使用 shuffle-only 包；这是材料布局标记，不改变原 shuffle 因果协议。
- raw-score 输入模块仅增加计时字段，没有改变算术关系或异常传播。
- 在线 TLS stream registry 对句柄做受控注册/领取，缺失或错绑的 authenticated stream fail closed；显式旧 FD 模式独立选择，不自动降级。此处审查没有找到可复现的 S26 级确定性漏洞。

## 3. 修复前故障与修正

修复前输出 writer：先写并 `fdatasync` 临时文件，再硬链接 no-clobber 到最终路径，之后删除临时名。**若最终路径发布成功但临时文件删除失败，函数直接抛异常，主流程 exit 70，而最终路径仍可见；同时没有持久化父目录项。**这违反“工程失败不得留下本次 mask share 文件”的局部输出合同。

复现使用仅 TEST_ONLY 的 failpoint `mask_unlink_after_publish`：两方均在发布后强制进入异常。外部日志 `mask_publish_before_fix.log` 显示 `P0_exit=70 P1_exit=70 P0_mask=present P1_mask=present`，之后 E2E 断言失败：`TLS E2E failure published a partial mask share`。该日志 SHA-256 为 `6a5994ce556d2caa69d2864e122c56c295eb1bda2d84c636bf32fa60061de39f`。运行时修复前源文件 SHA-256 为 `ac74c39ebe18081df5be1520ed1436babbca19cc88fbd7803f8843bd896bcd82`；相同原文备份与校验表保存在仓库外 `BMW16_S26_20261008/pre_mask_fix/`。失败 fixture 曾含测试材料，仅在 WSL 临时目录用于检查；没有复制进仓库，复核后清除。

修正后的 `write_mask_exclusive` 使用异常安全清理：写入与 fdatasync、关闭、no-clobber link、临时名 unlink、父目录 fsync 全部成功后才把发布状态视为完成；任何中间异常均关闭 fd、删除本次发布路径和临时路径、尽力同步目录，再原样传播异常。existing output 目标不会被覆盖或当成“本次拥有”误删。增加 `mask_dir_fsync_after_publish` 故障点。二者仅在 TEST_ONLY failpoint build 生效。

修正后 TLS 测试结果：两类注入均 `P0_exit=70 P1_exit=70 P0_mask=none P1_mask=none`，被报告为 `LOCAL_OUTPUT_ERROR`；普通成功、抽样 abort、材料/通信失败、final-status 异常保持原本状态码和无 mask 合同。故障注入覆盖回滚逻辑；它不模拟设备本身的 unlink/fsync 系统调用永久失效，若回滚 syscall 自身失败，外层启动器仍必须按非零退出码拒收，不可将残留文件视为成功输出。

## 4. S25 基线独立检查与 S26 复跑

S25 报告中的文件身份、源码摘要与提交关系经只读核对；S25 历史报告记载的 16 项回归、39 common-tape、small ranks、extended、TLS matrix、n=1000 分别留在 S25 原报告中。本窗口只将以下实际 S26 执行计入新证据：

| 层 | S26 实际命令/配置 | 结果 |
|---|---|---|
| 默认关闭 Release 构建 | `cmake --build /tmp/moe_bmw16_s26_off --parallel 2` | **未通过完整构建**：冻结外部目标 `bitpack_test` 链接报 `undefined reference to bitpack::mod(unsigned long, int)`；不是 BMW16 代码报错。构建器中已选择构建的旧 Protocol I/III 目标此前完成。 |
| 全部 CTest（OFF 配置） | `ctest --test-dir /tmp/moe_bmw16_s26_off --output-on-failure` | 41 项中 3 项运行通过、38 项因未构建可执行文件为 `Not Run`；CTest 汇总 `7% tests passed`。不称全仓通过。 |
| 默认关闭构建覆盖 | `cmake --build ... --target help` | BMW16 party-node / E2E target 未注册；default-off。 |
| 有界历史协议回归 | ON Release 先显式构建 15 项，再按精确 CTest regex 执行 | **15/15 PASS**：M1 DCF/oracle/CmpAgg、M2 priority/DCF/shuffle/uCMP/CmpAgg/transport/score-input/paper-alignment/并行 shuffle，以及 Protocol III 三进程。 |
| common-tape differential | 下方命令 | **39/39 PASS**；包含小 n tapes 与四个 n=64 profile，逐比较样本、pivot、中间集合、边、结果、abort 与 selected handle。 |
| 全 K TEST_ONLY 进程 oracle | `wsl.exe -d Ubuntu-24.04 -u root -- /tmp/moe_bmw16_s26_test/moe_topk_bmw16_experimental_party_node_e2e_test /tmp/moe_bmw16_s26_test/moe_topk_bmw16_experimental_party_node --small-ranks` | 本轮复跑退出 0。测试源码对 n=2..8 各 K=1..n 共 35 个配置执行冻结 oracle 检查；另有 n=1 shortcut 和固定的 n=2/5/8 用例。外部记录显示 39 组 P0/P1 SUCCESS（包括 35 个全 K 配置与固定用例）。另外 3 个 ready marker 缺失/截断/篡改负例均拒绝且无 mask。 |
| Extended TEST_ONLY 进程 oracle | 同上，参数 `--extended` | 14 个 oracle-successful 用例：n=1、2、5、8、64（K=1/8/n）、128（K=1/8/80/n）、256（K=1/2/n）；3 个 marker 负例拒绝。覆盖 K 两端、中间 K、重复分数和 signed 极值。 |
| 三角色 TLS | `wsl.exe -d Ubuntu-24.04 -u root -- /tmp/moe_bmw16_s26_test/moe_topk_bmw16_experimental_party_node_e2e_test /tmp/moe_bmw16_s26_test/moe_topk_bmw16_experimental_party_node --tls`；不同 UID、真实 TCP loopback、TEST_ONLY 临时证书 | 7 个正常 mTLS 成功样本（n=2/3/5/8/64/128/256）逐位核 oracle；13 个发材失败/身份/发布/截断/重排/崩溃样本、8 个在线故障样本，以及 TLS 缺流、最终状态分歧、两种 mask 发布故障均执行。不同 UID 文件隔离断言 15 次通过。这里只是本机隔离 UID 与 loopback，不等同跨主机部署或正式性能测试。 |
| n=1000/K=80 | `wsl.exe -d Ubuntu-24.04 -u root -- /tmp/moe_bmw16_s26_on/moe_topk_bmw16_experimental_party_node_e2e_test /tmp/moe_bmw16_s26_on/moe_topk_bmw16_experimental_party_node --tls-n1000`；Release、fresh materials | **本轮完整成功**：session `137976873088`，输入/种子 `0x2020100080`，T、双方 receiver 与双方 online party exit 0，TEST_ONLY controller 检查输出长度、原序、二值、weight K 和冻结 oracle。每方 sidecar `4,967,527,664` bytes，shell 各 `2,013,901` bytes；观测 TLS 发送 `9,941,055,454` bytes，T 报告 `offline_elapsed_us=612,464,080`、delivery `440,388,573` us。上述仅为单次同主机功能执行的真实程序字段，**不作为性能结果**。S25 首次超时复跑与本轮 fresh run 分开保留。 |

S26 首次 n=1000 尝试仍使用旧 600 s harness timeout：接收方先超时，T 的离线生成/连接随后失败，startup gate 未创建输入 shares 或启动在线协议。该次原始大日志在开始 fresh retry 前被覆盖；仅保留并登记控制台摘录 `e2e_tls_n1000_timeout_failure_capture.txt`（SHA-256 `376A026FD832DC3818266E4EF68017B5DD7186602BBE9BEDC1FB4E08B7329BF`），不把它称为完整 raw log。将 n≥1000 测试接收和在线超时提高到 1800 s 后，以上 fresh-material 复跑完成。失败运行和成功运行分列，不能合成一次尝试。

共同 tape 实际命令（39/39 PASS）：

```text
py -3 experiments/TEST_ONLY_BMW16_S12/run_common_tape_matrix.py --wsl-distro Ubuntu-24.04 --cpp-test /tmp/moe_bmw16_s26_test/moe_topk_bmw16_experimental_party_test --s4 experiments/TEST_ONLY_BMW16_S7_COMPOSITION/select4r_s4_source.py --out C:\Users\28641\.codex\artifacts\BMW16_S26_20261008\common_tape_final
```

S4 哈希验证对 CRLF 做字节规范化后与固定 Git blob SHA 对齐；修正前 Windows 工作树因 CRLF 被误报 S4 source hash mismatch。本轮脚本只更改哈希验证字节归一化，未改变所运行的 S4 程序。

小规模全 K 用例使用的原始固定输入由 E2E 源码 `--small-ranks` 生成；本轮复跑记录 `e2e_small_ranks_recheck.log` 的 SHA-256 为 `A7D6D0308D7D7332B2CA9046F4A821BEDCD412FC6DC47ACAD5E18AFCCB10A4A9`。测试程序退出 0；原始输出中部分 `node_e2e` 摘要行与相邻子进程输出拼接，因此本报告用程序退出码、n=2..8 全部 35 个 session 的 T 发材行、39 个成功配置各自的 P0/P1 成功行和 harness 内逐次 oracle 断言交叉核对，而不把摘要行数当作配置数。

小规模全 K 用例使用的原始固定输入由 E2E 源码 `--small-ranks` 生成；本轮复跑记录 `e2e_small_ranks_recheck.log` 的 SHA-256 为 `A7D6D0308D7D7332B2CA9046F4A821BEDCD412FC6DC47ACAD5E18AFCCB10A4A9`。测试程序退出 0；原始输出中部分 `node_e2e` 摘要行与相邻子进程输出拼接，因此本报告用程序退出码、n=2..8 全部 35 个 session 的 T 发材行、39 个成功配置各自的 P0/P1 成功行和 harness 内逐次 oracle 断言交叉核对，而不把摘要行数当作配置数。

## 5. 构建与复跑环境

- 工作树：`C:\Users\28641\.codex\worktrees\m6b-i-bmw16-s26\MoE_Top-k_Reproduction_Evaluation`。
- Linux：WSL Ubuntu 24.04，GCC 13.3，CMake 3.28.3，x86_64。OFF/ON/TEST_ONLY 构建各自使用独立目录 `/tmp/moe_bmw16_s26_off`、`..._on`、`..._test`。
- ON Release：实验开关 ON、TEST_ONLY failpoints OFF、DCF diagnostic counters OFF。TEST_ONLY 构建：实验开关 ON、测试 failpoints ON、DCF counters ON，仅用于差分与负例。
- 资源预检：运行前可用内存约 7.5 GB，`/tmp` 可用约 987 GB；n=1000 产生的所有材料留在 harness 临时目录，结束后清理。它是功能性 loopback 运行，不能作为 LAN/WAN 性能数据。

S25 38 路径清单与 S26 关键源文件基准/最终哈希、二进制哈希、证据日志 SHA-256 集中登记在仓库外 `C:\Users\28641\.codex\artifacts\BMW16_S26_20261008\sha256_index.txt`；密钥、bundle、证书私钥和真实 shares 未进入该报告或 Git。最终关键哈希如下：

| 对象 | SHA-256 |
|---|---|
| S26 party runtime 源 `VFSS/src/apps/moe_topk_bmw16_experimental_party_node.cpp` | `BB6D853ABF61647F26A9EA1AB16C9774AD7C3B277DC0267CE13CE6ECAF3A46A0` |
| S26 E2E harness 源 | `81A7BDD2FD1A26682A614CAFF8BC250F7763E41C15D99C3682C097481294167F` |
| common-tape Python harness 源 | `D1740971A449F309427CD3676A4B25CB4FD3B75743DB50BA4A6C319253E224AC` |
| Release party-node | `ba605cbb9970f0365320099b5a5e006cbb6b2fcb489d085d440bcce91a44ee1d` |
| Release E2E harness | `0780004c368cedd2503b87051ac572b3dcc7eceea2fbe82977d867c86481968a` |
| TEST_ONLY party-node（failpoints/counters ON） | `b025214d7498df6aa78989e5a62ea66341a9ba006eb6057bc6e1fa9f70f4e279` |
| common-tape matrix JSON | `DD2D4AC9D45E1E149B322E87EF37B9FF7CAE9A392A60CA4D2E4DBE600E840976` |
| cross-protocol targeted CTest log | `093D311E060D4D72A1A69EE26E2F6C020B49AC6A827AFB4662CE4B387E5BE453` |
| full OFF CTest log | `B1AE1D360C1FC6232A775EDE5176436B5AAFFE20B6FE5E80D8402FEE550F0515` |
| small-ranks recheck log | `A7D6D0308D7D7332B2CA9046F4A821BEDCD412FC6DC47ACAD5E18AFCCB10A4A9` |
| extended log | `B58445D2DE53F9394F9BD002B8B3A07E0949BD8AFB3D89B04E002B51B0C18834` |
| TLS fault matrix log | `F5B2166E5C989CBEE97C8DE5CBD25F771E5183BBE330E44F674613A1F1AC042C` |
| TLS n=1000 log | `3403A141BC074155B8AAD255EFEAF5BFFF8C316F1AB6417EEC839EBF2A64726F` |

## 8. 本轮修改文件

| 文件 | 修改 |
|---|---|
| `VFSS/src/apps/moe_topk_bmw16_experimental_party_node.cpp` | 修复 mask-share 发布异常回滚；成功前同步输出目录；增加仅 TEST_ONLY 可用的发布后 unlink/fsync 故障点。 |
| `VFSS/tests/moe_topk/experimental_bmw16_party_node_e2e_test.cpp` | 增加发布回滚负例并核验无 mask；将 n≥1000 TLS 接收/在线 timeout 调整到 1800 s。 |
| `experiments/TEST_ONLY_BMW16_S12/compare_common_tape.py` | Windows checkout 下对冻结 S4 Git blob 内容规范化 CRLF 后校验哈希；执行的 S4 内容不变。 |
| `PROJECT.md` | 加入 S26 当前状态注记，保留正式里程碑顺序及异聊天接收限制。 |
| `docs/IMPLEMENTATION_PLAN.md` | 同步 S26 EXPERIMENTAL 技术复核状态，不提前关闭 M6A→M6B 正式门。 |
| `docs/decisions/BMW16_S26_POST_CANDIDATE_REVIEW_DECISION_2026-10-09.md` | 记录可复现缺陷、修正范围和独立性边界。 |
| `docs/reproduction/BMW16_S26_POST_CANDIDATE_REVIEW_2026-10-09.md` | 本轮技术复核、执行矩阵、哈希与交接报告。 |

## 9. Draft PR 状态

提交 `597519f7bef7cfd05daaeedc36e6d8426beac9d5` 已推送到 `origin/codex/m6b-i-bmw16-s26`。本轮请求 GitHub MCP 创建 Draft PR 时，connector 返回 `connector ... is not connected`；浏览器创建页虽被打开，但获取页面状态连续超时。因此**Draft PR 未实际创建**，也没有把 compare/create URL 误报成 PR。候选分支可从以下 GitHub 页面创建独立 Draft PR：

`https://github.com/05T1925/MoE_Top-k_Reproduction_Evaluation/compare/main...codex/m6b-i-bmw16-s26?expand=1`

建议标题：`Add opt-in BMW16-derived Protocol I Select candidate`。创建时需保留本文中 EXPERIMENTAL 身份、未关闭的 DCF/联合视图安全门、正式性能 NOT_RUN，以及本执行聊天参与 S25、不能自称异聊天接收的说明。

## 6. 门禁状态

| 门 | 状态 | 本轮含义 |
|---|---|---|
| `EXPERIMENTAL_FUNCTIONAL_ACCEPTANCE` | `CONDITIONAL` | S26 修复后的有界 E2E / oracle / common-tape 证据通过范围内可接受；异聊天独立接收身份仍未满足。 |
| `DEFAULT_OFF_AND_BASELINE_ISOLATION` | `PASS` | default-off 不注册候选 target；基线 `VFSS-baseline/` 及主工作区未改。 |
| `MATERIAL_AND_TRANSPORT_CONTRACT` | `PASS (local test scope)` | 仅受测 WSL 本地独立 UID、fresh material、mTLS TCP 与错误合同。生产证书注册、远程部署、管理员/快照回滚不涵盖。 |
| `CROSS_PROTOCOL_REGRESSION` | `PASS (15/15 targeted)` | 指定旧 Protocol I/III / DCF / transport 目标通过；全仓构建受 `bitpack_test` 阻断，38 项 CTest Not Run。 |
| `SOURCE_TO_BINARY_PROVENANCE` | `PASS (external trace)` | 源 revision、构建配置、二进制与日志外部哈希；不声称二进制内嵌 revision。 |
| `DCF_SINGLE_KEY_PRIVACY` | `UNPROVEN` | S24 的实际压缩 DCF key / BGI15 字段对应断点继续有效；本轮未给当前完整 serialized key 函数隐私证明，也未发现攻击。 |
| `FULL_POOL_AND_SHUFFLE_VIEW` | `UNPROVEN` | 功能回归不能证明整池自适应 DCF view 或 forward/inverse shuffle/output joint simulation。 |
| `SECURE_ALIAS_READY` | `NO-GO` | 不创建 secure alias。 |
| `FORMAL_PERFORMANCE_READY` | `NO-GO / NOT_RUN` | 不运行正式 LAN/WAN 五次矩阵；n=1000 也仅功能 loopback。 |

## 7. 交接要求

Draft PR 对象是默认关闭的 EXPERIMENTAL 实现整理、mask share 发布回滚修正与复跑证据，不申请 secure/security/performance 接收。外部接收者应从 PR head 独立重建，核对本文最终日志与源/二进制哈希，检查修复路径，并另外决定是否接受异聊天接收资格。安全后续的首要门仍是 S24 指出的实际完整 serialized DCF party key 函数隐私桥接，其后才是 full-pool adaptive queries 与 shuffle/output joint view；正式性能门继续等待 V3→V4 顺序和统一九指标闭合。
