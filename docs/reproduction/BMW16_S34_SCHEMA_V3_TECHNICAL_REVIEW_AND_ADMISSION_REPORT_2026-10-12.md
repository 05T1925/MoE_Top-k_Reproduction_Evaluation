# M6B-I-BMW16-S34：schema-v3 技术复核、n=1000 准入与 V3/V4 前置报告

日期：2026-10-12（Asia/Shanghai）

## 1. 结论摘要

S33 实施者也参与 S34，因此本报告给出**技术复核**，不能称为独立接收。最终选定的 4 组三路线配对诊断（n=128/256、K=2/8）共 12 条 attempt，通过 schema-v3 JSON Schema、collector auditor、共同输入/oracle 身份和 fresh session/material 序号核验；原始 run record、manifest、历史 schema-v2 文件和哈希均未改写。

本轮发现并修正两个 TEST_ONLY collector 缺陷：n>1000 时 BMW 路线曾被错误标为 READY；campaign auditor 没有把 attempt 的输入身份、配置和 fresh-session/material serial 完整对账。另把 S33 报告中的“独立计量接收”更正为技术复核，并补入遗漏的早期身份失败记录。

| 门/事实 | S34 判定 | 证据范围 |
| --- | --- | --- |
| S26 功能接收 | **PASS（历史独立功能接收）** | 只表示默认关闭的实验候选通过 S26 功能审查 |
| S31 `conditional_secure_v2` 安全接收 | **CONDITIONAL_ACCEPTED（沿用）** | 沿用 S28–S31 的模型、假设、可信离线 T 和泄露函数 L；S34 未重开安全证明 |
| S32 计量交付 | **PASS（技术复核）** | S31→S32 源码差异、四条 schema-v2 原始记录及 manifest 重新审计 |
| S33 最终配对诊断 | **PASS（技术复核）** | 4 campaign / 12 attempt；全部为 DIAGNOSTIC，不是正式性能 |
| S33/S34 campaign inventory | **7 PASS / 15 FAIL（共 22）** | 7 个 schema-v3 合规；15 个早期探索/工程失败记录保留原样并排除于最终 12 条矩阵 |
| n=1000/K=80 A 全对全 | **PRECHECK_REJECTED** | 当前入口仅覆盖 n=128/256、K=2/8 |
| n=1000/K=80 B Protocol I+AAV86 | **PRECHECK_REJECTED** | r=2、D=1024；AAV86 E16 D≤256 硬门在 KeyGen 前拒绝 |
| n=1000/K=80 C BMW16-derived | **RESOURCE_REVIEW_REQUIRED / NOT_RUN** | conditional_secure_v2 形状界通过；RAM 低于 S31 建议值，峰值 RSS、网络和时限未测 |
| V3 | **OPEN** | Protocol I+AAV86 与 Protocol III+AAV86 的完整性能门未关闭 |
| 正式 V4 LAN/WAN | **NOT_ALLOWED / NOT_RUN** | 现行计划要求先关闭 V3；没有可信项目记录批准调整顺序 |
| PR #29 | **OPEN / NOT_APPROVED / NOT_MERGED** | PR 仍指向 S32 head；S33/S34 本地提交未推送 |

BMW16 路线名称继续使用 `Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED`。它是稳定 Top-K 的项目派生方案；不是 BB90 原算法复现，不继承 BB90 或 BMW16 Algorithm 7 的比较常数、轮数或概率定理。S31 条件安全结论不升级为无条件安全、恶意安全或部署安全。

## 2. 开工现场保全、身份与来源

### 2.1 开工时现场

S34 开始时没有在桌面主 checkout 或 S33/E16 worktree 执行 reset、clean、stash、archive、覆盖或删除。主 worktree 与 E16 bridge 的 branch、HEAD、Git 状态、tracked diff、staged diff 和未跟踪文件哈希已复制到仓库外：

`C:\Users\28641\.codex\evidence\m6b-s34-receipt-snapshot-20261011-01\`

| worktree | 开工身份 | 开工状态 | tracked working diff SHA-256 |
| --- | --- | --- | --- |
| S33 主 worktree | `codex/m6b-i-bmw16-s33-metrics`，HEAD `32cd7952cccdacc5a1a5b4fcca245f7acf594049` | 4 个 tracked modified、4 个 untracked；staged diff 为空 | `ff98dbab7af0b9d06e62e730a59aa9cf81f07c880ef9cb22b345ec25e3583a85` |
| E16 TEST_ONLY bridge | detached HEAD `267cb6924f061d376dbce7986fc0093b52ec2512` | 2 个 tracked modified；无 untracked | `78cee254c7de71a247bfb439b0d31c3d22434b3421416bd766346a6f68696e18` |

S33 主 worktree 开工时的 untracked SHA-256 清单位于 snapshot `primary/untracked-sha256.txt`。其中 schema-v3 为 `ad342ee442857ed0677dd4e13cd16af210e5ebf2dd950896e37e217e3d6e923a`；原 S33 collector 为 `c920b635ec2884b624aeceb7ab95a82ab133a4345b6dbe3d5b9544a7da69c7df`；原 S33 决策为 `eac35b8b53332d54c3b9f07323f46ae03ac11e5ebba63ce13dae1e6c49dfe5b7`；原 S33 报告为 `c1b7f3e385b58df1a5820d38e859c9b02100a78b0789464935b93376f9d12a22`。这些开工副本保持原状。

桌面主 checkout `C:\Users\28641\Desktop\MoE_Top-k_Reproduction_Evaluation` 未写入。`VFSS-baseline/`、`Papers/`、本地参考工程、密钥、证书、sidecar 和构建产物未进入本次 Git 差异；S33/S34 的原始大日志仍在仓库外。

### 2.2 代码身份

| 身份 | revision |
| --- | --- |
| S31 条件安全接收 | `a1a90cb2a568deb5e5078ddec10136ce9a743411` |
| S32 被测 runtime | `f2db098578b1cc096298ee39d990ff668d8dd6f7` |
| S32 最终报告 / S33 主 worktree 开工 HEAD | `32cd7952cccdacc5a1a5b4fcca245f7acf594049` |
| S33 时 E16 bridge source HEAD | `267cb6924f061d376dbce7986fc0093b52ec2512` |
| S33 E16 bridge TEST_ONLY 改动本地提交 | `d29db27a5e9a766e728f6929dfe72d05f6794d9d`，branch `codex/m6a-e16-s33-diagnostic-bridge` |
| PR #29 source head（远端） | `32cd7952cccdacc5a1a5b4fcca245f7acf594049` |
| `main` / `origin/main`（远端） | `c3926c68fd14f270faa8b55234311071947fa080` |

S31→S32 代码差异仍是 S33 审查对象；S34 只修改主 worktree 的 TEST_ONLY preflight/auditor、测试和治理文档。E16 bridge 的 2 个 TEST_ONLY harness 改动已在独立本地分支提交，未推送。没有 secure runtime、协议消息、泄露函数、材料 ABI 或输出语义变更。

## 3. S32 技术接收复核

### 3.1 原始 schema-v2 记录

使用原 `bmw16_run_record.py audit` 对仓库外 S32 四份记录重跑，4/4 PASS。记录与 manifest SHA-256 保持和 S32 报告相同：

| 配置 | record SHA-256 | evidence manifest SHA-256 |
| --- | --- | --- |
| n=8/K=3 | `82c04163458f0602a9712387acd84e56ba999615270ba20a1a2448f716accdb5` | `04e620cd8b6b72ebcfeb466aa4d9052c162a1b634dc2e66d1afa7ac7d780c4a9` |
| n=128/K=8 | `3495086a82608dbd11f92eece867cd9425dafcb0bb83a9d0a130248a25ef05f4` | `49d9e18672a1f87c028ac04a809366f214c42dea103a12af85b4650447116733` |
| n=256/K=80 | `25d1fcd73b7c2c75d44ce78a13d2f9d09f59abce9b8616c413075c8a4739e58c` | `7671c796794fdb38d56b282b76c020392a5eff71afbda3bccd4870d9df112f05` |
| n=1000/K=80 | `95c96f3998d811a9c2071d8695bffaf53fa99111a6a578251a3ebfc0bf34edd1` | `6fd32688c838d49aa6f37145ed781d6f55a62845c31920548328af38b3260f5a` |

审阅 S31→S32 的 8 个文件差异后，未发现消息、泄露、材料 ABI、一次性材料消费或 mask 语义变化。计数点到字段与审计恒等式沿用 S33 技术报告第 3.2、4 节；S32 的四份 JSON 保持 schema-v2 / DIAGNOSTIC 身份。

### 3.2 计数器 ON/OFF 固定 tape 原始证据

S33 的固定 TEST_ONLY tape 结果按原报告核验：3 次成功及 1 次 forced abort；移除时间和仅计数器字段后，ON/OFF 输出、计数、应用通信、边迹与 abort/no-mask 状态一致。计数器 ON 中位数 41.139 ms、OFF 35.682 ms，观察差 +15.29%；该数字只描述这 3 次本机 socketpair/进程内测试，不外推 LAN/WAN 或正式运行。

原始索引：`C:\Users\28641\.codex\evidence\m6b-s33-counter-calibration-fixed-tape\comparison_summary.json`，SHA-256 `d461c09f5fa29767d280b12991131eeab1a3cce775ab3ad615b2da3137c43368`。八个原始日志哈希仍由该 summary 引用；第一次未固定 OS `getrandom` 的探索尝试继续排除，不改写。

## 4. schema-v3 与 S33 campaign 证据复核

使用 schema 2020-12 validator 4.10.3 和 S34 增强后的 collector auditor 扫描原始证据根目录：

- `C:\Users\28641\.codex\evidence\m6b-s33-paired-diagnostics\`
- `C:\Users\28641\.codex\evidence\m6b-s33-validation\`

共 22 个 campaign、66 个 attempt、88 个 JSON 对象。7 个 campaign（campaign + 21 个 attempt，共 28 个对象）通过 schema-v3 和 auditor；15 个 campaign（其 45 个 attempt 和 15 个 campaign，共 60 个对象）未满足最终 schema-v3/auditor，原样保留，不纳入最终 12 条矩阵。S34 没有写回任何历史 attempt、campaign、manifest 或 hash。

### 4.1 最终选定的 4 组配对诊断

三条路线为 A Protocol I 全对全、B Protocol I+AAV86（`r=2`）、C `conditional_secure_v2`。各组的同一 signed Q20.12 输入、冻结 oracle、seed、输入 SHA、oracle SHA/FNV64 和 12 行九指标详见 S33 技术报告第 6 节。S34 对每个 attempt 的记录 SHA、manifest SHA 和 hash-bound `command.json` 重新读取；campaign attempt 的 n/K、input/oracle SHA、seed 与配置一致，每个 campaign 内 A/B/C 的 session/material serial 互不重复。12/12 为 SUCCESS，mask 为原序、二进制且恰 K 位；这些仍是单机 loopback **DIAGNOSTIC**。

| n/K | campaign id suffix | campaign SHA-256 |
| --- | --- | --- |
| 128/2 | `4aa945f3-f566-48dd-8bdf-2cb3d0747396` | `e7888cfb029b75c2a5235e96cb9dff917756de0f7eb27e6cb1e18c26c72522bc` |
| 128/8 | `5ba0a284-9f8e-4c86-a166-c163a4f7b57f` | `f7a8bbcbf511ea8bb5154454fb75be1f7172cc72bcf1730545a85ca71c02e4df` |
| 256/2 | `e510cf15-79c5-4fc8-aa6c-960b1ec48bf4` | `2f3edebca9e812c457672e98f819ac197e282efd0370cad7eaf6de5fcd21a8a1` |
| 256/8 | `1e8b02b6-de24-4555-a49b-70b3d68463c7` | `9e2fb69a8a7b1cccc487cbb9dcce60b888c07d0bace0496791e3482f613360d7` |

另有 3 个 schema-v3 合规 campaign：gate spot-check `fb42d177-1f1f-449c-bf49-c257155ce8de`（SHA `1923b4423c244922a102c1eeb7186ec9d042348e128f88abcaecf4f36bd2ad76`）；S33 最终输入身份 fixture 失败 `373096b7-4aa2-4253-94e8-dc534c50b9df`（SHA `808a6c597c9e81262b2ff61dec036e145d983f3c32e4ea48f78abbf6814cb6b4`）；S33 原报告遗漏索引的更早 fixture 失败 `388b64e5-56c0-4605-952d-4bc7963ef6d7`（SHA `a701675545ebb4ea5ed657b21ec684acbf5f35f214fb17cffaa68fb9c5842f4a`）。后两个是协议未启动的 `ENGINEERING_ERROR`，通过记录格式审计不等于协议成功。

### 4.2 排除的 15 个早期 campaign

下列 campaign 均保留原哈希和状态。12 个早期全成功批次不是最终选择的 schema-v3 配对结果：8 个早期 batch 的 3 条 attempt 均缺最终 null/`NOT_MEASURED` provenance，4 个早期 batch 缺共同 oracle FNV 身份。另 3 个 campaign 保留真实工程/材料失败，但使用早期不完整失败记录合同。它们不能被改标为最终 schema-v3 或正式结果。

| 类型 | campaign id | campaign SHA-256 |
| --- | --- | --- |
| 早期全成功，provenance 不全 | `1e8510c4-d61c-4ad2-beaf-78a5cc5c73d6` | `556e0c394117eaabbf6d26f3024772db93155b287cae1351dfcb0d318b16d603` |
| 早期全成功，provenance 不全 | `b72d4af8-aa49-4dcb-ba70-273d14b4721c` | `f29938e9ca79bc83b6b50857d5dba8ab467d07d99e0df972c229a48c02054164` |
| 早期全成功，缺共同 FNV | `de08d477-f27a-4bc2-9053-1564d31420e2` | `2aecf82daaee3d14065608631c1ea4b0755de41be37ee2f3b9601f57a8e69f88` |
| 早期全成功，provenance 不全 | `52eee62a-faf3-4789-8b2c-1ffd99da663e` | `2c44faea52cae17424ef66cd52466e743db23a2de839239d6d4a8b161b45a896` |
| 早期全成功，缺共同 FNV | `bf1a4c92-cdbe-40da-bca6-999a1ca8cc95` | `be3e0b30a423172624fdda7eb7e0961b43fc3e4ed6222001920bba349d706b16` |
| 早期全成功，provenance 不全 | `ebcf5e73-f7d9-4d8d-8312-4034ec965312` | `61e9e60dce924e3fb562dd541df586a5c77d4108cf603bda5518bea8e82f1913` |
| 早期全成功，provenance 不全 | `2f81e216-0085-42dc-bc48-d88daa2b990b` | `42bc20a8585fb42b8baa7dc376cc5c03c70cc66ee100a4a43d94a573452083b0` |
| 早期全成功，provenance 不全 | `4776f704-455e-4f0b-84ca-dbeb1d3c9c92` | `7de646c28f4345f0fe713ccb6f940e8569f30796f7284196a3964a58a7cbac8b` |
| 早期全成功，缺共同 FNV | `b493dc4b-4b2f-42e0-a06d-7f53b15cbf5b` | `509f68967148304489473ee9c0c553c7dd6d63af7b5996a2674d61595292dff2` |
| 早期全成功，缺共同 FNV | `742aefa8-aa18-4ff1-a9e6-7274c216a0b8` | `7c498ced4187f55674ba599abd8ac1afa0abfbe1ea48aa77d5628ed091d451a5` |
| 早期全成功，provenance 不全 | `7c8314bc-0210-4735-81a8-6d14a0a8be3c` | `0aa5b452564ecad7c6f5c336f723ec458a63b1f3904859256e70ddeee9035774` |
| 早期全成功，provenance 不全 | `938c9480-3791-4d23-ab00-e172191152c2` | `120c3f368a5290c2a242a287958cd974f9788aee9db20030dfa3fbe96a555b86` |
| 保留 AAV86 工程失败 | `18d7955b-d4c1-445c-a6c8-f9b1921d3486` | `c72a44b009de68d26601409c1e871c09c3889ce4e83fc26d18a176d8645ad587` |
| 保留 AAV86/BMW 材料/工程失败 | `deb7768f-d286-4246-a340-55345c379c80` | `b7e0c492b15bb99609061cf147925da360a4d1777e47ef890a801456a5857705` |
| 保留 BMW 工程失败 | `4dfbd555-cf53-49ad-8ad7-c9655baadffa` | `23909541aa85d814841259c3e1db469bb079546330176751b3b90443275e82b4` |

全量扫描日志 `audit-inventory-compact.txt` SHA-256 为 `67cd85e4764fc38268b29dc77b2934b99f698d3f9f5093aa0de3a869f600947e`；该日志列出 22 个 campaign 的 audit/schema 状态、15 个排除原因和全部 campaign hash。

## 5. S34 修复项与定向测试

### 5.1 修复项

1. **路线 preflight。** A 现在只对当前可执行的 `(128,2)/(128,8)/(256,2)/(256,8)` 形状报告 `RESOURCE_REVIEW_REQUIRED`；B 先检查 `D≤256`，再检查当前桥接形状并明确 `r=2`；C 只接受 `2≤n≤1000, 1≤K≤n`。形状适配不再等于资源准入。任何路线硬拒绝时，三路线 campaign 级 `campaign_admission` 为 `PRECHECK_REJECTED`，否则为 `RESOURCE_REVIEW_REQUIRED`。
2. **campaign identity auditor/sealer。** 对每条 attempt 与 campaign 核验 campaign id、n/K、input/oracle SHA-256、input seed；对 hash-bound `command.json` 核验共同输入身份与正数 session/material serial，并要求 campaign 内已启动路线的 serial 不重复。协议尚未启动的 identity fixture 错误不要求不存在的 route command，但仍保留 no-mask、失败原因和 NOT_MEASURED provenance。正式 `seal-campaign` 使用同一 identity audit。
3. **接收身份文档。** S33 参与者不能把同一工作称为独立接收。S33 报告改名并更正标题/状态，S33 决策、PROJECT、实施计划和 benchmark 计划同步说明；S26 历史独立功能接收和 S31 条件安全结论单独保留。

### 5.2 本轮新运行的测试

所有构建均在 WSL Ubuntu 24.04，构建目录在 `/tmp`。C++ 使用 GCC 13.3.0、CMake 3.28.3、OpenSSL 3.0.13，Release，`BUILD_TESTING=ON`。

| 测试 | 命令/构建目标 | 本轮结果 |
| --- | --- | --- |
| Collector unit tests | `python3 VFSS/tests/bench/test_protocol_i_unified_diagnostic.py -v` | **8/8 PASS**：shape gate、n=1000/n=10000 预检、配置身份错配、command identity 错配、重复 serial、协议未启动身份失败 |
| BMW16 conditional v2 conformance | `ctest --test-dir /tmp/m6b-s34-build --output-on-failure -R moe_topk_bmw16_conditional_secure_v2_conformance_test` | **1/1 PASS** |
| BMW16 frozen-oracle party test | `ctest --test-dir /tmp/m6b-s34-build --output-on-failure -R moe_topk_bmw16_experimental_party_test` | **1/1 PASS** |
| BMW16 independent-process E2E | `/tmp/m6b-s34-build/moe_topk_bmw16_experimental_party_node_e2e_test /tmp/m6b-s34-build/moe_topk_bmw16_experimental_party_node --conditional-secure-v2-paired-case 128 2 771339 2026101012813`（WSL root test harness） | **PASS**：T/P0/P1 为 distinct local UID；`SUCCESS_ORACLE_CHECKED`；原序 mask weight K；双方拒绝 durable-material replay 且 `mask=NONE` |
| E16 AAV86 conformance + oracle differential | `ctest --test-dir /tmp/m6a-s34-build --output-on-failure -I 1,2` | **2/2 PASS** |
| E16 all-pairs bridge test | `ctest --test-dir /tmp/m6a-s34-emp-build --output-on-failure -R moe_topk_m6a12_protocol_i_clique_benchmark_test` | **1/1 PASS** |
| S32 历史 schema-v2 auditor | `bmw16_run_record.py audit` 对原四份记录 | **4/4 PASS**，SHA 与原报告相同 |
| S33 schema-v3 全量扫描 | `audit_inventory_compact.py`（JSON Schema Draft 2020-12 + 当前 collector auditor） | **22 campaign：7 PASS / 15 FAIL**，失败记录均保留，不改哈希 |
| 全仓 CTest | 未执行 | **NOT_RUN**；以上均为定向测试/审计 |

第一次使用默认 WSL Ubuntu 20.04 时，S33 报告中的 `/tmp` 构建缓存不存在，CTest 未发现测试。后续显式切换到 Ubuntu 24.04，从两个被测 worktree 在仓库外新建构建并重新运行目标测试；不会把那次缓存路径错误记作测试结果。

### 5.3 本轮二进制身份

| 路线/构建 | binary SHA-256 | CMakeCache SHA-256 |
| --- | --- | --- |
| C v2 conformance | `e08c8d464a91211a49ee565083b8a7d878a1b5961beebe5e96537887c350cc69` | `5067693035c40d7b80d4926e7220a3a8cec979b7b1464d327ee829ca3e6f9784` |
| C party test | `f3d6206b3581bc052fde89f56cb430802bb414e7c8494c2f82d161833e4e8dda` | 同上 |
| C E2E harness | `f0dfeca2875dfb57a8ef9900a031496b0a0b7b135fb958c42582804a3728d529` | 同上 |
| C party-node | `08f1df9204a9593a8daf5163051d4ad18ff949846d86aa98df80ea8bba6c197e` | 同上 |
| AAV86 conformance | `3ca6d06d586d23c0edee56fc87259d61b9b52d594d7501765ff9efc76a12e56e` | `9cc66cacbc01bd1b906311e78fe5ad4f2ce018564aef8453ab351f4ffa552a66` |
| AAV86 differential | `d18859e462b7128ee7c2800728c80e8796a4d6fd4c6809fd778f5bad71c46388` | 同上 |
| A all-pairs E12 bridge test | `eb9539973e4d69450633ae88e382c5f1b723d213d8f486887811bb88aa78265c` | `a560c84ec1f453644bc18107ac75738df1df1f4f2b082802884735f5ebd2ada7` |

C 构建选项：Release、`BUILD_TESTING=ON`、BMW16 adapter ON、conditional v2 ON、v1 OFF、DCF PRG counters ON、failpoints OFF。E16 AAV86 build 为 Release、`BUILD_TESTING=ON`；全对全 build 额外启用既有 `MOE_TOPK_ENABLE_EMP_OT=ON` 和本地 EMP prefix。构建产物均在仓库外。

## 6. n=1000 与 n≥10000 分路线准入裁决

当前实测环境为 WSL2 Ubuntu 24.04、Linux `6.6.87.2-microsoft-standard-WSL2`、i9-13980HX、32 vCPU、总 RAM `8,122,392,576` bytes、可用 RAM `7,527,239,680` bytes、`/tmp` 可用磁盘 `985,908,383,744` bytes。S31 runbook 的 n=1000 规划值为 sidecar `4,967,527,664` bytes/party（约 9.94 GB 两方）；建议 RAM 至少 16 GiB，且要求租机后实测 RSS、T KeyGen 峰值和约 10 GB aggregate 网络传输。当前环境未满足这些资源证据条件。

| 路线 | n=1000/K=80 形状/参数 | n=1000 准入 | n≥10000/K=80 | 指标状态 |
| --- | --- | --- | --- | --- |
| A Protocol I all-pairs | 当前 E16 bridge 仅接收 n∈{128,256}、K∈{2,8}；没有该 n/K 的可执行入口 | `PRECHECK_REJECTED`，协议未启动 | 当前入口仍不接收；不生成材料 | n=1000、n=10000 九项全 `NOT_MEASURED` |
| B Protocol I+AAV86 | `r=2`；n=1000 padded D=1024 | `PRECHECK_REJECTED`，D>256 在 KeyGen 前硬拒绝 | D=16384，同样在 KeyGen 前拒绝 | 两规模九项全 `NOT_MEASURED` |
| C Protocol I + BMW16-derived Select + DCF | `conditional_secure_v2` 允许 2≤n≤1000、1≤K≤n；本 K=80 属参数边界内 | `RESOURCE_REVIEW_REQUIRED / NOT_RUN`；总 RAM 低于 16 GiB 建议，当前峰值 RSS、timeout、RTT/吞吐、网络流量均未测；未单独生成约 10 GB package | n=10000 超 n≤1000 wrapper；preflight `PRECHECK_REJECTED` | 两规模九项全 `NOT_MEASURED` |

n=1000 当前 `campaign_admission=PRECHECK_REJECTED`，因为 A/B 使三路线联合矩阵不成立；C 单路线也没有获得资源放行。n=10000 的 S31 checked layout 为 `540,395,955,164` bytes/party，aggregate `1,080,791,910,328` bytes，比当前可用 scratch 多 `94,883,526,584` bytes，且未计 20% 预留；本轮只读容量合同和已检查文件系统，不生成全池材料。

S34 preflight JSON 外部 SHA-256：

| 输入 | output SHA-256 |
| --- | --- |
| n=128/K=2 | `7e9f93cc533be0bee2f07953f0b795546f0dd56a33149a5f878fe7b41f6c43ce` |
| n=1000/K=80 | `9c75848a3b8731c75c31ddee0d74b106f833c1160ab9b6c994e3f370ff18adc8c` |
| n=10000/K=80 | `98ba359cda6fbfb3918b2cac1b0d2d37a993e06dc576b564f14a223d49f589a9` |

## 7. V3/V4 阶段门与 PR #29

### 7.1 正式性能前置门

`docs/BENCHMARK_VALIDATION_PLAN.md` 仍要求先完成 Protocol I+AAV86 与 Protocol III+AAV86 的 V3 完整性能验收，再进入 V4 LAN/WAN。S33/S34 schema、pairing diagnostics、E16 TEST_ONLY bridge、服务器 runbook 和 checked preflight 均是准备/诊断资产；都不能关闭 V3。S34 没有在可信项目记录中找到批准改变顺序的决策，也没有修改该顺序。正式 V4 尚不可启动，V4 1 warmup + 5 formal 的 LAN/WAN 记录为 `NOT_RUN`。

### 7.2 当前远端 PR 状态

| 独立状态 | 事实 |
| --- | --- |
| S26 独立功能接收 | 历史报告 PASS；只覆盖默认关闭的实验功能候选 |
| S31 条件安全接收 | `CONDITIONAL_ACCEPTED`，范围与假设保持不变 |
| S32 计量交付 | 在 PR #29 source head 中；S34 技术复核 PASS |
| S33/S34 本地提交 | **YES，本地提交；未推送** |
| PR #29 source head / source branch | `32cd7952cccdacc5a1a5b4fcca245f7acf594049` / `codex/m6b-i-bmw16-s26` |
| `main` / PR base | `c3926c68fd14f270faa8b55234311071947fa080` / `main` |
| PR 标题/正文 | 旧 S26 标题；正文仍是通用模板 |
| PR reviewer approval | **NO / NOT_APPROVED**；公开页面显示 Copilot 未能完成 review，原因是 requester's quota limit |
| PR state / merge | **OPEN / NOT_MERGED** |

公开 [PR #29 页面](https://github.com/05T1925/MoE_Top-k_Reproduction_Evaluation/pull/29)于 2026-10-12 重新核验；`git ls-remote origin` 的 source branch 与 main SHA 如上。GitHub 生成的 merge ref 不作为合并证据。未更新远端 PR metadata、未 push、未 merge。

## 8. 方案身份、安全边界与后续最短路径

准确名称是 **Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED**。目标是接收 signed Q20.12 score，遵守同分按 original index 的稳定语义，输出原输入顺序下恰有 K 位为 1 的秘密共享 mask。Select 的四个比较依赖层是子协议层；完整入口按实际 DAG 为 12 个 application stages。该项目派生候选不是 BB90 原算法复现，也不借用 BMW16 Algorithm 7 / BB90 的比较常数、轮数或概率保证。

安全状态沿用 S28–S31：trusted offline T 不与在线方合谋、最多一名半诚实在线方被腐化、私有认证通道、source-specific DCF/PRG 假设、OS CSPRNG、ROM sampler 熵条件和既有泄露函数 L。S34 没有改协议内容或这些条件；它不证明无条件/恶意安全，也不等于生产部署接收。

当前证据支持的下一步：

1. 按计划关闭 Protocol I+AAV86 和 Protocol III+AAV86 的 V3 正式性能门，并保存 machine-readable PASS。
2. 如需单独运行 BMW16-derived n=1000 诊断，先在满足资源建议的服务器测 peak RSS/T KeyGen、网络 RTT/吞吐/流量和时限，再重新执行 checked preflight；A/B 的形状或 D 硬门必须通过实现/容量合同变更及评审解决，不能绕过。
3. V3 关闭后再执行 V4 正式 LAN/WAN，每路线每配置 1 warmup + 5 formal；使用 schema-v3 `seal-campaign` 封存逐次记录和原始哈希。
4. PR #29 仍需实际 reviewer approval 和远端状态更新；当前本地提交不等于 PR 已更新或已合入。

## 9. 修改文件清单

### S33/S34 主 worktree 本地提交

| 文件 | 修改内容 |
| --- | --- |
| `PROJECT.md` | 纠正 S33 接收身份，加入 S34 campaign/n=1000/V3 状态 |
| `docs/IMPLEMENTATION_PLAN.md` | 将 S33 记为技术复核，记录 S34 findings 与资源裁决 |
| `docs/BENCHMARK_VALIDATION_PLAN.md` | 明确 V3 仍是 V4 前置，补充 S34 准入和 auditor 状态 |
| `docs/decisions/BMW16_S33_METRIC_RECEIPT_AND_PROTOCOL_I_DIAGNOSTIC_DECISION_2026-10-10.md` | 更正 S33 独立接收说法及 C 路线 n=1000 状态，附 S34 更正 |
| `docs/reproduction/BMW16_S33_TECHNICAL_METRIC_REVIEW_AND_DIAGNOSTIC_REPORT_2026-10-10.md` | 原 S33 报告改名并更正“独立”标签，加入 S34 复核增补 |
| `docs/reproduction/BMW16_S34_SCHEMA_V3_TECHNICAL_REVIEW_AND_ADMISSION_REPORT_2026-10-12.md` | 本详细报告与逐次证据索引 |
| `VFSS/tests/bench/protocol_i_unified_diagnostic.py` | 修正 route preflight、campaign identity/fresh-session auditor，并让 formal sealer 复用校验 |
| `VFSS/tests/bench/test_protocol_i_unified_diagnostic.py` | 新增 8 项 preflight/campaign identity 回归测试 |
| `VFSS/tests/bench/PROTOCOL_I_UNIFIED_RUN_RECORD_SCHEMA_3.json` | S33 schema-v3 原文件纳入提交；内容/哈希未改 |
| `VFSS/tests/moe_topk/experimental_bmw16_party_node_e2e_test.cpp` | S33 TEST_ONLY E2E 计量 harness；S34 未改其协议/安全实现 |

### 单独的 E16 TEST_ONLY bridge 本地提交 `d29db27a5e9a766e728f6929dfe72d05f6794d9d`

| 文件 | 修改内容 |
| --- | --- |
| `VFSS/tests/moe_topk/protocol_i_aav86_small_e2e_test.cpp` | 加入输入/份额夹具、online/入口 wall time 和 oracle digest 字段 |
| `VFSS/tests/moe_topk/protocol_i_e12_baseline_bench_test.cpp` | 加入输入生成、online/入口 wall time、oracle digest 与实际 uCMP 计数字段 |

新增复核原始输出均在仓库外 `C:\Users\28641\.codex\evidence\m6b-s34-review-20261011\`；S33 初始工作树快照在 `C:\Users\28641\.codex\evidence\m6b-s34-receipt-snapshot-20261011-01\`。原 S33 12 条配对逐次 hash 详见 S33 技术报告表；S34 的 `audit-inventory-compact.txt`、`audit-s32-records.txt`、三份 `preflight-ubuntu24-*.json`、Python/CTest/E2E logs 均记录了文件哈希。
