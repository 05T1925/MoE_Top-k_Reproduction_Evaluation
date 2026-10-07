# BMW16 S16 独立接收、概率复核、安全门与计量报告

日期：2026-10-07
实现身份：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL**
被测源码 revision：`a95d7cec8105da0c418a465d96f314830a873630`
本报告与决策文档 revision：另见最终 Git 记录；不得将报告 revision 当作被测 revision。

## 1. 基线、工作区与接收对象

本工作从 S15 报告 commit `b8e9fc622403ece033466b15c479643bc6ecfb74` 建立独立 worktree `C:\Users\28641\.codex\worktrees\m6b-i-bmw16-s16\MoE_Top-k_Reproduction_Evaluation`，分支 `codex/m6b-i-bmw16-s16`。建树前 S15 worktree clean；起始 `main`/`origin/main` 是 `c3926c68fd14f270faa8b55234311071947fa080`。本轮没有触碰桌面主工作区、`VFSS-baseline/`、`Papers/`、AAV86、E20/E21、PR #28 或本地参考工程。

按接收顺序检查 `PROJECT.md`、实施与基准计划、S14/S15 决策/报告/证据索引、Protocol I+BMW16 源码，以及 `protocol-reproduction` skill。S15 已接入的 VFSS C++ 全链 API 被复用；没有重写 Select 或另建 raw-score→mask 协议。S16 新增代码只有 DCF 计数输出与固定 TEST_ONLY tape 计量入口、独立证据审计脚本；未创建 secure alias。

### 1.1 S15 证据接收与源码身份差异

外部 S15 归档位于 `C:\Users\28641\.codex\artifacts\BMW16_S15_20261007\`。根清单列 216 个文件，S16 对所有 216 项执行 `sha256sum -c`，均匹配；归档 `SHA256SUMS.txt` SHA-256=`be7089865be7068f2101fb47ad5abbab9527935b2967cd22c997c32cdfcde9ea`。S14 的 64 项归档原哈希也核对通过，S14 清单哈希=`8899b04952e8d6d287a7a74804f9bc1eaece02e52174c1254eb38cf3736360ef`。

另将 S15 的 `S15_SOURCE_AND_BINARY_HASHES.txt` 逐行与其声明的源 revision `2c6937116fbbc170e7a254f4449a3a6411b0cb70` 的 Git blob 比较，并搜索所有可达历史：12 个源码路径中 5 个 Python 匹配；7 个 C++ 路径不匹配，且这 7 个清单哈希均不匹配任何可达 Git revision。CRLF/LF 转换不能解释差异。独立表在外部 `s15-source-identity-audit.json`，SHA-256=`bd3e7a3e59251741a865cb1f8a656d28c7377006ba5f0917f9fdc66b39817515`。

因此 S15 原始归档完整性是 PASS，S15 声明源码与历史二进制的对应关系为 **UNVERIFIED**。S16 没有猜测旧 binary 的实际源，也没有把 S15 的 1+5 pilot 认定为源绑定实测。S16 从声明 Git tree 独立重建并复跑的证据是新的，身份单独记录如下。

## 2. S16 被测构建与环境

- 操作系统：WSL2 Ubuntu 24.04 x86_64；GCC 13.3.0；CMake Release；OpenSSL 3.0.13。
- CMake：实验 Select adapter ON；完整测试构建的 `MOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=ON`。计时对照使用同一源码、Release、adapter ON，另建 counters OFF 版本。
- 构建目录：`/tmp/bmw16-s16-fresh-20261007`；计数器关闭构建：`/tmp/bmw16-s16-counters-off-20261007`。
- 三进程 TEST_ONLY binary SHA-256：`0800995d89fe26cd20e9c1d5c97c8dbbcfb7563f7bfcc9b0602dc711d507cfab`。
- Party/conformance binary（counters ON）SHA-256：`d6215e3385219ee56dd17dc36efe9e0ec91fed5dca7816b844e17cc9e4659d8b`。
- Party/conformance binary（counters OFF）SHA-256：`7145f03560900201ad86653f5877e5d8dc94c1a27d543e0ff26766e72abeb775`。
- S16 改动的 party-test Git 源码 blob，位于 `a95d7cec…`：SHA-256=`aa32b2e0794c995a1eb0cf55b6a4b4e06b6ee3da5c6f42488db78a06623f75cb`。算法库源仍从 S15 声明 Git tree 重建；改动的测试入口不作为协议源证据。

相关 target 按 conformance 顺序构建并执行，CTest 匹配集合 6/6 通过：BMW16 Party、uCMP、transport、raw-score input、shuffle conformance、shuffle process E2E。全量 CTest 因本次临时目录没有构建其余历史 targets 而未执行；不将 `Not Run` 计作通过或回归失败。原始测试日志 SHA-256=`735b25d61c3a8f48620171ef7a6f0e1f7fffc88bb404fccf8f3aecff6e8ab45a`。

## 3. 复跑与功能结果

### 3.1 S4 / C++ common-tape differential

使用冻结 S4 文件 `C:\Users\28641\.codex\artifacts\BMW16_S15_20261007\s4_select4r_frozen.py`，其 SHA-256=`8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab`。n=256，K=2/8，tape/input seeds 825600/825601；从样本、pivot、U/V/W、每轮边、比较结果、状态到 selected handle 的 trace 逐层相同。S16 最终 matrix SHA-256=`4aff390b75553b1b43424c01423169b9f203442008d55c6bd8fc03e5b6619df2`。S15 历史报告中的 39 条较小规模 common-tape 差分另按外部归档核验，未将其改写成本轮结果。

### 3.2 独立 T/P0/P1 进程 E2E

S16 新建相关目标后运行现有 C++ 三进程 harness，并由独立审计脚本读取原始 stdout、stderr、party 状态、退出码、输出存在性、oracle 结果和计数恒等式。61 条结果：

| 状态 | 数量 | 处理 |
|---|---:|---|
| `SUCCESS` | 57 | TEST_ONLY harness 重构并与冻结 oracle 对照；长度为 n、值为 bit、weight=K，逐位一致。 |
| `ABORT_ALGORITHM_PROBABILITY`（注入） | 1 | 双方一致 abort；不产生 mask。是强制注入，不是自然概率失败。 |
| `ABORT_MATERIAL` | 1 | 材料错误分类；无成功 mask。 |
| `ERROR_UNEXPECTED` | 1 | 意外异常保持非零错误退出语义，不会被伪装为正常 abort/success。 |
| `ABORT_COMMUNICATION` / peer close | 1 | 存活方记录本地通信 abort；对端断开时不声称双方已确认一致。 |

自然算法 probability abort 为 0/61；这只是观察。所有 61 行独立审计通过，无被过滤的失败种子。n=64/K=8 的代表行是 17,837 个逻辑比较调用、每方 11,338 个 DCF Eval；同一行 sampler PRF words 和扩展调用另有字段。原始 stdout SHA-256=`2bffe719384fd8d38202c30f7f875870209f203a25636536ecf8c8fc730eec33`，stderr=`f3f76b104f0bfee4353529c6c97534d68f57c0fc58bb527d0462f03bd8b8ee90`，独立 E2E audit JSON=`bf9a5ab4ab4bf19963f91650d678e70de28dd70fe58400ea328f62885590c838`。

S16 E2E 沿用测试 harness 的受限 party UID 运行 P0/P1；root 只负责 TEST_ONLY fixture、启动与结束 oracle 核对。日志核对未发现输入份额、DCF key、解封凭据或完整 mask 被写入原始 party 日志。该检查不是生产 OS 凭据/通道证明。

### 3.3 历史 S15 E2E 与 pilot 的接收差异

S16 也重算了 S15 外部 61 行 E2E audit，状态汇总 57 SUCCESS、1 注入概率 abort、1 材料错误、1 意外错误、1 peer close；原 stdout/stderr hash 匹配且独立统计可重算。该重算确认 S15 归档内数据互相一致，但由于 7 个 C++ 源码行未绑定到声明 revision，不能单独接受为“该 Git 源码所构建的历史二进制”的证据。类似地，S15 pilot 的 24 个 raw 文件哈希与 JSON summary 全部一致、4 配置 × 5 measured 全成功，但其源绑定仍受该问题影响，不进入性能结论。

## 4. SUCCESS 正确性及有限概率界

S16 独立复核并保持 S15 的 SUCCESS soundness 推理，公式、有限参数和分支细节见[决策文档第 3–4 节](../decisions/BMW16_S16_INDEPENDENT_ACCEPTANCE_PROBABILITY_SECURITY_AND_METRICS_2026-10-07.md)。关键结论：只要真实键是严格全序、uCMP 结果精确、S4 运行时输入/比较/参数正确，则任何被代码标为 `SUCCESS` 的两个 Partition 均有精确各 `h=M/2` 划分；两哨兵交集唯一为第 K 边界记录；membership 的 `Rᵢ≥R_selected` 与同置换逆路由得到恰 K 位 stable oracle mask。随机抽样只能进入明确失败/abort 分支；invariant guard 失败不能改叫 probability abort。

独立数学脚本覆盖所有可达偶数 M：n≤256 为 M=2..512 共 256 个值；解析公式域 n≤10⁶ 为 M=2..2,000,000 共 1,000,000 个值。结果分别为 `1.3105804606611235e-11` 与 `5.065679989546104e-8`。脚本哈希和输出见证据索引。bound 用两个 task 的 union bound，不假设 task 独立。n≤10⁶ 是对公式的检查，不代表当前 API 支持；实际 API 限 n≤256。

结论严格分三类：

1. `IDEAL_UNIFORM_NO_REPLACEMENT`：有条件数学上界，采用 S4 当前 s/qL/qH/U_cap/v/W_cap 整数取整。
2. `CSPRNG/PRF`：实际 stream 的计算安全假设独立；未量化优势，未与统计失败界混加。
3. `FINITE_OBSERVATION`：61 个复跑样本无自然 probability abort；不构成 `≤10^-6` 统计证明。

## 5. PRG 全链计数口径

S15/S16 新加的计数点位于普通 DCF `evalDCF` 的 `ecbEncTwoBlocks` node expansion。定义：

```text
online_prg_calls_total = P0、P1 两进程普通 DCF ecbEncTwoBlocks 调用次数之和
online_aes_blocks_total = 2 × online_prg_calls_total
sampler_prf_words_total = 双方 SHA-256 counter-sampler words 之和（独立字段）
```

计数分别 reset 于新建 T/party 进程内的相应阶段，party 在线 API 不执行 KeyGen；审计器对 61 例逐方复算。由源码路径搜索，raw adapter、shuffle、Select control、membership、inverse 没有其它同类在线 DCF AES 扩展调用；但此值**不包括**离线 DCF KeyGen、AES-GCM、系统 crypto 库内部无法由该 hook 观测的 AES 指令或其它通用 PRG API。因此名称虽沿用 `online_prg_calls_total`，报告将它解释为 DCF expansion，不宣称库级完整 PRG 指令数。源码审计与二进制计数范围未能提供一个统一库级 hook，完整库级计数 `NOT_MEASURED`。

S16 61 例复算 JSON SHA-256=`379e5d342924c09245a76864040023ca3c39fc58084e971390fcf1b2ae0ac72b`。S16 E2E 最大 n=64/K=8 案例有 881,804 次 DCF node expansion / 1,763,608 AES blocks / 656 sampler PRF words。对于 n=1 shortcut，不运行 DCF，数值 0 来自结构上无 DCF，而不是计数器观测到的任意全路径 PRG 为零。

| 字段 | 本轮状态 | 解释 |
|---|---|---|
| 四层 Select 逻辑比较、真实/dummy/重复边 | `MEASURED_PER_ATTEMPT` | 三进程输出与独立审计器复算；逻辑边不等于唯一规范 FSS key 槽。 |
| 唯一 uCMP 材料槽、预发/消费槽、DCF Eval | `MEASURED_PER_ATTEMPT` | 两 party Eval 和 offline dealer slot 分开，`DCF Eval=2×uCMP party Eval`。 |
| online DCF PRG expansion 与对应 AES block | `MEASURED_WITH_COUNTERS_ON` | 只计 evalDCF 的 `ecbEncTwoBlocks`；不称库级全 PRG。 |
| SHA-256 sampler PRF words | `MEASURED_SEPARATE` | 不与 DCF 调用次数或 AES blocks 相加。 |
| 因果消息 phase、每方 send/recv、各在线阶段/总时间、party RSS | `MEASURED_PER_E2E_ATTEMPT` | S16 C++ harness 逐次记录；属于 counters ON 的实验环境，不具有跨方案或 LAN/WAN 可比资格。 |
| T 发材阶段时间、bundle/manifest/AEAD 字节、T RSS | `MEASURED_PER_E2E_ATTEMPT` | 实验性 T 单进程发材计量，包长和 RAM 不互相替代。 |
| 库级全 PRG/AES、同 revision 全对全/I+AAV86 配对、正式 LAN/WAN 五次矩阵 | `NOT_MEASURED_OR_NOT_ACCEPTED` | S15 pilot 内部可复算但 source identity 未接收；S16 不补造可比性或全库计数。 |

示例仅作被测路径追溯：S16 fresh run 的 n=64/K=8/seed=1206，Select 逻辑比较 17,837、每方 DCF Eval 11,338、两方扩展调用总数 881,804，sampler words 328；成功路径 12 phases、每方 send=recv=151,497 B。单次 T 包 18,654,121 B/party，generation/seal/delivery/offline total 分别 105,010/95,574/65,804/266,391 μs，T RSS 127,812 KiB；party 在线时间 45,532/45,443 μs，party RSS 83,968 KiB。该计数器 ON 单次数据不是正式性能比较；ON/OFF overhead 另以固定 tape 诊断估计。

## 6. 计数器开销对照

S16 构建同源码 ON/OFF 二进制；计时入口仅在 TEST_ONLY，n=64、K=8、input seed `160608`、固定 score 向量 `((17i mod 23)−11)`、固定 `alternating` sampler tape、固定 TEST_ONLY entropy interposer，交替顺序，各 1 warmup + 5 次配对，每次生成 fresh material。计时只针对同进程两 party API thread + socketpair，不做 coins exchange。

ON/OFF 四轮边 hash、轮调用数、uCMP Eval、DCF Eval 与 tape 一致。ON critical online `min/median/max=43,727/44,971/47,045 μs`；OFF=`33,799/35,059/37,494 μs`；median 开销 `+28.2723409%`。这些原件在 `counter-overhead-accepted/`。先前曾运行但发现固定 sampler tape 未固定 shuffle 导致边图不一致的一组 probe，以及计时 harness 把 OFF 的 `NOT_MEASURED` 当数值比较的失败尝试，均留在外部归档内并排除出接受样本。被接收的最终结果是相同边图和计数的 paired probe。

此处所谓每次“fresh material”是每个独立进程重新执行发材，不从上次 attempt 复用内存对象；固定 `LD_PRELOAD` `getrandom` 测试 interposer 使每次生成可重复，不提供新的密码学熵，故只用于锁定相同 shuffle 图/工作量的测试计时，绝不能用于协议安全评估。

此开销数据不用于修正 S15 loopback 值，也不属于 LAN/WAN benchmark。未来如启用性能计量，应优先用 counters OFF 主测、同一 revision 的独立轻量事件审计或另报计数 build；各方案必须有共同定义。

## 7. 容量、对照资格和下一规模

沿用 S15 checked 全池合同 `slots/party=2p+2+9C(n,2)`，key length `57+24b`，比较宽度 `b=33+index_bits`。容量预检完整 JSON SHA-256=`af86090d5c1b1612298ffb9af8014b2dce65e2c420d5234ba16bbec170ff1048`，下表是解析下界，不是运行包长：

| n | slots / party | bundle lower bound / party | 本轮入口状态 |
|---:|---:|---:|---|
| 64 | 18,274 | 18,494,340 B | 已有有界 E2E；S16 复跑 n=64 |
| 128 | 73,410 | 76,087,396 B | S16 三进程矩阵 K=2/8/80 均有成功样本；包长下界非实测 |
| 256 | 294,274 | 312,136,740 B | S16 三进程矩阵 K=2/8 与 common-tape K=2/8 均运行；包长下界仍非实测 |
| 1,000 | 4,497,550 | 4,987,338,528 B ≈4.99 GB | 超 512 MiB cap；API cap 256；`PRECHECK_REJECTED`，未发材/运行 |
| 10,000 | 449,987,770 | 542,225,036,988 B | `PRECHECK_ONLY`，无运行 |
| 100,000 | 44,999,812,146 | 57,464,659,444,756 B | `PRECHECK_ONLY`，无运行 |
| 1,000,000 | 4,499,997,597,154 | 6,070,495,802,256,900 B ≈6.07 PB | `PRECHECK_ONLY`，无运行 |

n=1,000 全池容量不适合当前 API。E20 流式生成可能降低峰值 RAM 或中间文件常驻，但不能省掉已定义全池 key 槽、离线生成字节和一次性访问约束；它是需单独评审的新发材接口，不是当前实现的“解除限制”理由。

跨方案比较当前 `NOT_READY`：S16 未运行 Protocol I all-pairs 或 I+AAV86 的同 revision 配对测量；没有正式 LAN/WAN 校准矩阵；S15 pilot 源身份不合格用于精确复现接收。未测 online network bytes、正式 elapsed time、统一 offline package/time、V4 repeated runs、库级 PRG、正式 abort rate 均填 `NOT_MEASURED`。不得把 S15 loopback 摘要、S16 固定带开销诊断或公式容量填进正式性能表。

## 8. 复跑命令

以下命令反映 S16 实际工具和隔离路径。外部证据存于 `C:\Users\28641\.codex\artifacts\BMW16_S16_20261007\`；占位 `<external>` 代表在该目录新建且不与历史文件覆盖的子目录。

```powershell
# 相关 Release 构建（Linux/WSL build dir）
cmake -S VFSS -B /tmp/bmw16-s16-fresh-20261007 -DCMAKE_BUILD_TYPE=Release `
  -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON `
  -DMOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=ON
cmake --build /tmp/bmw16-s16-fresh-20261007 --parallel 4 --target `
  moe_topk_bmw16_experimental_3proc_test moe_topk_bmw16_experimental_party_test `
  moe_topk_m2_ucmp_conformance_test moe_topk_m2_transport_conformance_test `
  moe_topk_m2_score_input_conformance_test moe_topk_m2_parallel_shuffle_conformance_test `
  moe_topk_m2_parallel_shuffle_process_e2e_test
ctest --test-dir /tmp/bmw16-s16-fresh-20261007 -R `
  "moe_topk_bmw16|moe_topk_m2_(ucmp|score_input|transport|parallel_shuffle)" --output-on-failure

# counter-OFF 同源构建，然后用固定 TEST_ONLY entropy 作 ON/OFF 工作量匹配
cmake -S VFSS -B /tmp/bmw16-s16-counters-off-20261007 -DCMAKE_BUILD_TYPE=Release `
  -DMOE_TOPK_ENABLE_EXPERIMENTAL_BMW16_SELECT_ADAPTER=ON `
  -DMOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS=OFF
cmake --build /tmp/bmw16-s16-counters-off-20261007 --parallel 4 --target moe_topk_bmw16_experimental_party_test
python experiments/TEST_ONLY_BMW16_S16/counter_overhead.py `
  --counter-on /tmp/bmw16-s16-fresh-20261007/moe_topk_bmw16_experimental_party_test `
  --counter-off /tmp/bmw16-s16-counters-off-20261007/moe_topk_bmw16_experimental_party_test `
  --test-entropy-so /mnt/c/Users/28641/.codex/artifacts/BMW16_S16_20261007/fixed_getrandom_interposer.so `
  --out C:\Users\28641\.codex\artifacts\BMW16_S16_20261007\counter-overhead-rerun --repetitions 5

# 独立 T/P0/P1 测试 harness；把两个 fd 分别保留
wsl.exe -d Ubuntu-24.04 -u root -- bash -lc `
  'cd /tmp/bmw16-s16-fresh-20261007 && ./moe_topk_bmw16_experimental_3proc_test > <external>/s16-e2e.stdout.log 2> <external>/s16-e2e.stderr.log'
python experiments/TEST_ONLY_BMW16_S15/audit_e2e_log.py <external>/s16-e2e.stdout.log
python experiments/TEST_ONLY_BMW16_S16/audit_online_prg_e2e.py <external>/s16-e2e.stdout.log --out <external>/s16-online-prg-audit.json

# n=256 common tape 与公式/容量检查
python experiments/TEST_ONLY_BMW16_S15/run_n256_common_tape.py `
  --cpp-test /tmp/bmw16-s16-fresh-20261007/moe_topk_bmw16_experimental_party_test `
  --s4 C:\Users\28641\.codex\artifacts\BMW16_S15_20261007\s4_select4r_frozen.py `
  --out <external>/n256-common-tape
python experiments/TEST_ONLY_BMW16_S16/abort_bound_recheck.py > <external>/abort-bound-recheck.json
python experiments/TEST_ONLY_BMW16_S15/capacity_preflight.py > <external>/capacity-preflight.json

# S15 归档审计与来源身份复算
sha256sum -c C:\Users\28641\.codex\artifacts\BMW16_S15_20261007\SHA256SUMS.txt
python experiments/TEST_ONLY_BMW16_S16/audit_s15_source_identity.py `
  C:\Users\28641\.codex\artifacts\BMW16_S15_20261007\S15_SOURCE_AND_BINARY_HASHES.txt `
  --revision 2c6937116fbbc170e7a254f4449a3a6411b0cb70 --out <external>/s15-source-identity-audit.json
```

## 9. 最终门禁

| 项目 | 判定 |
|---|---|
| `FUNCTIONAL` | `PASS_FOR_S16_REBUILT_TESTED_DOMAIN`；历史 S15 C++ source/binary identity 未核实。 |
| `IDEAL_ABORT_BOUND` | `PASS_CONDITIONAL_IDEAL_UNIFORM_WITHOUT_REPLACEMENT`；实现随机源计算优势另列。 |
| `CONDITIONAL_SECURITY` | `NO_GO`：首缺口是相关阈值完整 DCF 多 key、辅助输入、自适应 opened-bit view 的 simulator；Protocol I shuffle/inverse 联合模拟待证。 |
| `PRIVATE_DELIVERY` | `NO_GO`：目前不是生产 T→party 凭据/认证私有交付。 |
| `ONE_SHOT_SCOPE` | `PASS_SHARED_PROTECTED_LOCAL_ROOT_ONLY`；跨 store/回滚 scope 未满足。 |
| `PERFORMANCE_COMPARABILITY` | `NO_GO/NOT_READY`：无同源全对全/I+AAV86 对照和正式 V4 LAN/WAN；S15 pilot 受源码身份影响。 |

不因功能测试通过将身份升级为 secure。V3/V4/M7 均未因本次 S16 工作而完成；正式 V4 矩阵保持未运行。
