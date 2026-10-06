# BMW16 S15 独立接收与 n=256 诊断报告

日期：2026-10-07
身份：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL**
工作分支：`codex/m6b-i-bmw16-s15`
被测实现提交：`2c6937116fbbc170e7a254f4449a3a6411b0cb70`
报告提交：与被测提交分开记录；不得用报告提交替代被测实现 revision。

## 1. 接收范围和保护

从 S14 报告提交 `1c92ca6cefcfdfdb1e04912ca1a3ec573c58a269` 建立独立 worktree；S14 被测源码提交为 `998726bca161b08cac140d7430c17f5b1005d914`。起始 `main`/`origin/main` 为 `c3926c68fd14f270faa8b55234311071947fa080`。S4 Python Select 的原始 Git blob 为 `04ed6f8a352ab2277434344094471dff82f83e4b:experiments/TEST_ONLY_BMW16_DERIVED_SAMPLE_BRACKET_SELECT_4R/select4r.py`，SHA-256=`8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab`。

未修改桌面主工作区、`VFSS-baseline/`、AAV86/E20/E21、PR #28、论文和本地参考树。S14 的 64 项来源/原始证据清单在本机归档逐项验证通过；原清单 SHA-256=`8899b04952e8d6d287a7a74804f9bc1eaece02e52174c1254eb38cf3736360ef`。S14 自报结果没有被当作本轮复跑；复跑原件列在[外部证据索引](evidence/BMW16_S15_EXTERNAL_EVIDENCE_INDEX_2026-10-07.md)。

## 2. S14 接收结果与 one-shot 范围

| 接收门 | 本轮独立结论 | 复核内容 |
|---|---|---|
| `S14_FUNCTIONAL_ACCEPTANCE` | `PASS`（所复核配置） | 从 S14 源构建 Release；6 项相关 CTest 通过；n=128、K=2/8/80 三份 Python S4/C++ common-tape 中间 trace 相同；59 条 T/P0/P1 复跑记录通过 runner 合同与 oracle。 |
| `ONE_SHOT_ACCEPTANCE` | `PASS_WITH_LOCAL_STORE_SCOPE` | durable burn marker 在 party API/Eval 前写入、`fdatasync` marker、`fsync` 受保护目录；共用 claim root 的并发、fork、崩溃后重启/重复领取拒绝。S15 追加了反例：同一 bundle 复制到另一个空 claim root 后，新 store 接受 claim。故不声称跨目录、跨主机或抗回滚一次性。 |
| `METRIC_ACCEPTANCE` | `PARTIAL`，S15 补齐部分 DCF 计数 | S14 的 4,032 个 n=64 shuffle CmpAgg 槽由 shuffle-only bundle 移除；S15 计量 `unused_cmpagg=0`、`omitted_cmpagg=2·C(n,2)`。S15 新增普通 DCF 调用点计数；它仍不覆盖整个 FSS/shuffle PRG。 |

S14 独立三进程矩阵为 55 SUCCESS、1 注入算法 abort、1 材料 abort、1 意外异常、1 peer close；n=128 common tape 三例分别覆盖 K=2、8、80。S14 日志及 hash 与 S15 复跑分开登记。测试编排器以 root 建 fixture 并切换到 UID 22012/22013；这说明受限 party 进程文件隔离，不把 root 控制器权限计入任何一方隐私视图。

## 3. 实施变更

复用既有 S14 C++ 全链 API，没有重写 Select。将实验域 `n≤128` 扩为 `n≤256`，只影响 opt-in EXPERIMENTAL 入口和 bundle 验证。新增默认 OFF 的 `MOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS`：普通 DCF 计数 `keyGenDCF`/`evalDCF` 调用，以及 KeyGen `ecbEncFourBlocks`、Eval `ecbEncTwoBlocks` 扩展调用。扩展数是 AES 调用次数，不是 AES block 数，也不表示全库 PRG 调用。关闭选项时热路径不执行 atomic increment；开启时 `Bin=7` conformance fixture 实测 `1` KeyGen / `14` KeyGen 扩展、`2` Eval / `14` Eval 扩展。

E2E 审计器复算每方 slots `2p+2+9·C(n,2)`、KeyGen `2p+9·C(n,2)`、扩展数、`DCF Eval=2×uCMP party Eval`、在线 Party KeyGen=0、T Eval=0、双方收发守恒、claim 数和返回码。sampler PRF words 独立字段。四层 logical edge 与唯一材料槽不混为一项。

## 4. SUCCESS 正确性与算法中止概率

SUCCESS soundness 仅在严格全序、精确比较、正确原始输入适配器和稳定复合键成立时证明。将真实记录按 Select 升序记为 `x₁<…<xₙ`，令 `r=n−K+1`、`N=n+1`、`L=max(N−2r,0)`、`H=max(2r−N,0)`、`M=N+L+H=2·max(K,r)`、`h=M/2`。task 0 在低 sentinel 前放 L 个 low dummy，在全部真实记录后放 H 个 high dummy；task 1 在全部真实记录前放 L 个 low dummy，在 high sentinel 后放 H 个 high dummy。由此 `task0.Accept ∩ task1.Reject ∩ RealHandles={xᵣ}`。两份 task 都 `SUCCESS` 时，candidate 集合必须大小为 1；否则显式 invalid abort。

每个 median Partition 的 R1 样本全对比较给出样本内部精确 rank；R2 的 `c≤h≤cy` bracket 产生连续残余区间 U 和 `q=h−c`。q=0 是精确端点分支。q>0 时 R3 对 V 内部及 V×(U\V) 冻结比较图，获得 V 各自对 U 的精确 rank，含端点自身；虚拟 rank 0/u+1 只作为边界，不是元素。`i≤q<j` 导出 W 与 `qW`。R4 对 W 全对比较，reject 计数 `c+(i−1)+qW=h`；reject/accept 互斥并覆盖输入。空 U、越界 q、缺失非法 bracket、非唯一 sample rank、错误 W/R4 基数都是 `ABORT_ALGORITHM_INVALID`，不是概率失败或错答案成功。

稳定优先键是 signed Q20.12 score 降序、`original_index` 升序；dummy 属独立公开类别且不落入真实复合键范围。选中阈值后 membership 比较方向 `Rᵢ≥R_selected` 选择恰 K 个真实记录；同一秘密置换逆路由，取 XOR 位份额，给出原序 K-bit mask。因此抽样随机性只能造成显式 abort，不能在 Partition 报告 SUCCESS 时改变 kth 结果。证明不称 BMW16 Algorithm 7 的逐字实现，不继承 Theorem 8。

**理想均匀无放回抽样界**只分析当前 s/qL/qH/U_cap/v/W_cap 与取整。脚本按 M 分段检查上界：`p_abort≤2·(p_bracket+p_U+p_W)`，其中两 task 用 union bound，不要求彼此独立。保守端点数值：有效测试 n≤256 最大 `1.3105804606611235×10⁻¹¹`（M=512）；公式外推到 n≤10⁶ 最大 `5.065679989546104×10⁻⁸`（M=2,000,000）。这不是 CSPRNG 实测概率，计算安全需另假设 `getrandom` 与 SHA-256 counter PRF；它不包含实现错误、FSS 错误、材料/通信错误。n>256 在运行 API 被拒绝，没有声称相应规模已实现或运行。

## 5. 验证结果

环境：WSL2 Ubuntu 24.04，Linux x86_64；GCC 13.3.0，CMake 3.28.3，OpenSSL 3.0.13；Release，两个 opt-in 开关 ON。构建目录 `/tmp/bmw16-s15-rebuild`。本轮按 conformance → common-tape differential → 独立进程 E2E 顺序执行。

| 层 | 结果 |
|---|---|
| 相关 CTest | 6/6 通过：BMW16 Party、uCMP、transport、raw-score input、shuffle conformance、shuffle process E2E。全量 CTest 未作为本轮验收：临时目录只构建了相关 target，未构建的 36 个历史 target 被 CTest 标成 Not Run；该次不计入通过率。 |
| n=256 C++/S4 common tape | K=2、8 两例逐层一致（S4 sample/pivot/U/V/W/edge-plan/results/status/selected handle）；分别 seed 825600/825601，PASS。 |
| T/P0/P1 E2E | 61 条；57 SUCCESS 逐位通过冻结 C++ oracle，mask 长度 n、bit 值有效、weight=K；1 `TEST_ONLY_FORCED_ABORT_AFTER_SELECT`、1 材料篡改 abort、1 unexpected API error、1 peer close。自然抽样 abort 为 0。没有重试或排序 fallback。 |
| n=256 | K=2、8 各一例，分数向量含 INT32 极值/重复值路径；独立 oracle E2E 通过，材料包及 RSS 在当前资源限额内。 |
| 强制负例/one-shot | 强制算法 abort 无 mask；材料篡改、unexpected API、peer close 按结构化状态与退出码处理；party conformance 核对错收件密钥、错 party/session/width、截断、密文/manifest 篡改、重复 claim、fork/崩溃 claim、并发进程 slot claim和复制到新空 claim store。 |

同机 loopback pilot：每配置 1 warmup + 5 measured；4 个配置 n=64/K=8、n=128/K=80、n=256/K=2、n=256/K=8。以下是 5 次正式 attempt 的 median；各项 min/max 与逐次原件在外部 pilot JSON 中。墙钟是 Windows runner 包住 WSL 子进程的 elapsed time；在线/离线内部阶段值为 party/T 计时。所有 20 次正式样本均 SUCCESS，无自然 abort。

| n/K | wrapper wall (s) | online (ms) | T offline total (ms) | bundle/party (B) | peak RSS party / T (KiB) | logical edges | unique uCMP Eval | DCF Eval / party | sent=recv (B/party) | phases | sampler PRF words |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 64/8 | 0.4300 | 50.147 | 241.702 | 18,654,121 | 83,968 / 127,812 | 17,748 | 5,902 | 11,804 | 150,785 | 12 | 328 |
| 128/80 | 1.6761 | 335.687 | 942.565 | 76,701,577 | 320,000 / 523,448 | 25,852 | 18,167 | 36,334 | 223,809 | 12 | 371 |
| 256/2 | 9.4917 | 1,639.679 | 7,039.095 | 314,544,457 | 1,284,096 / 1,970,160 | 87,380 | 22,060 | 44,120 | 732,417 | 12 | 576 |
| 256/8 | 7.5424 | 1,666.679 | 4,455.517 | 314,544,457 | 1,284,096 / 1,970,160 | 84,750 | 24,051 | 48,102 | 711,377 | 12 | 570 |

成功分支的实际消息 DAG 计量为 `2 raw-score adapter phases + 2 forward-shuffle exchanges + 1 joint sampler-coin exchange + 4 Select edge/result exchanges + 2 inverse-shuffle exchanges + 1 status agreement = 12`。被强制/自然算法中止时 membership 与 inverse 不运行，计 10 个 phase；n=1 shortcut 不进入一般路径。四层 Select 只对应其中四个依赖交换，不能称 raw-score→mask 的完整安全协议四轮。

每配置全部 6 次 raw attempt（包括 warmup）、每轮逻辑/unique/dummy/repeat 边、R1–R4 与输入/输出阶段耗时、DCF KeyGen/Eval expansion 次数、包 plaintext/ciphertext/manifest 字节和每次峰值 RSS 均写在外部 JSON/日志。`sampler_prf_words` 不计入 DCF 扩展；DCF 计数也不代表全库 PRG。由于该测量构建开启了 atomic 计数器，pilot 用时含其额外开销；未测计数器 ON/OFF 的耗时差。因此这些数据仅用于工作量/容量诊断，不作性能排名；也不可与旧 revision、Protocol I 全对全或 I+AAV86 拼接，不能进入正式 V4 LAN/WAN 矩阵。

## 6. 容量与计量账本

全池预发槽数为 `2p+2+9·C(n,2)`，每方包下界由 checked uint64 算术按 `2p` raw uCMP、9 份 pair pool、manifest/tag 逐项求和。n=256：294,274 slots/party，容量下界 312,136,740 B/party；实测 sealed bundle 314,544,457 B/party，低于 512 MiB bundle cap。运行机 WSL 报告总 RAM 8.12 GB、available 7.57 GB，`/tmp` 可用 990.29 GB；n=256 pilot 实测 T 峰值 RSS 约 1.97 GiB、party 约 1.28 GiB。此为本机资源窗口，不外推部署配置。

| n | 槽/party（checked 推导） | 包字节下界/party | 运行状态 |
|---:|---:|---:|---|
| 64 | 18,274 | 18,494,340 | 有界运行 |
| 128 | 73,410 | 76,087,396 | 有界运行 |
| 256 | 294,274 | 312,136,740 | 本轮实测 bundle/E2E/pilot |
| 1,000 | 4,497,550 | 4,987,338,528 | `PRECHECK_REJECTED/API_CAP`，未生成包 |
| 10,000 | 449,987,770 | 542,225,036,988 | `PRECHECK_REJECTED/API_CAP`，未生成包 |
| 100,000 | 44,999,812,146 | 57,464,659,444,756 | `PRECHECK_REJECTED/API_CAP`，未生成包 |
| 1,000,000 | 4,499,997,597,154 | 6,070,495,802,256,900 | `PRECHECK_REJECTED/API_CAP`，未生成包 |

9 指标状态：逻辑比较、实际 DCF/uCMP Eval、预发/消费/未用槽、DCF AES 扩展计数、sampler PRF words、包长度/容量下界、阶段耗时、loopback 收发、phase 数和 T/P0/P1 RSS 有本机 pilot/E2E 记录；全库 PRG 总数、V4 LAN/WAN、跨方案可比总时间为 `NOT_MEASURED`。循环计数器不把 AES 调用折算为 AES blocks 或全部 PRG。

## 7. 安全结论与门禁

在理想私有认证通道、半诚实单方、可信且在线静默 T 的前提下，可以把已见 transcript 后的算法控制作为后处理模拟；但当前第一个失败 hybrid 是从独立 PRG streams 的 H1 转到完整 **auxiliary-input / correlated-threshold / multi-key party-view DCF pool** 的 H2：单 key API/隐私说明没有给出本地 shuffle 辅助状态、同节点 mask 相关阈值、整包未使用 key、公开 operand/bit 和自适应 query 的联合 simulator。接着还需要实际 shuffle 本地方 `sigma/tau/a/e/r_share`、消息、masked list、inverse output-share 的联合模拟。S15 没有证明不可能，也没有用 domain separation 或有限枚举代替证明。

材料 AEAD 的包身份/传输完整性与 DCF key 内数学关系是不同保障。root harness 预配本地 party key 不是生产 T 凭据注册/远程私有发材；本地 claim store marker 也不是跨 store 的 anti-rollback ledger。故不得建立 secure alias。

| 门 | 结论 |
|---|---|
| `FUNCTIONAL` | 有界样本 PASS；含独立 oracle 及 n=256 两例。 |
| `PROBABILITY_BOUND` | 理想均匀无放回 sampler 的项目推导界 PASS；CSPRNG/PRF 计算假设分离。 |
| `CONDITIONAL_SECURITY` | NO-GO：相关整池自适应 DCF party-view 及 shuffle/output-share hybrid 未闭合。 |
| `PRODUCTION_DELIVERY` | NO-GO：生产凭据建立、远程私有认证材料分发、跨目录 anti-replay 账本未实现。 |
| `PERFORMANCE_COMPARABILITY` | 仅 EXPERIMENTAL pilot；V4 正式矩阵未开始。 |

唯一最关键的下一构造：针对实际 `keyGenDCF/evalDCF` 与 Protocol I shuffle，给出并审查**带辅助 shuffle view、相关阈值向量、完整未用 key 池、公开自适应查询/打开 bits 的单方多 key DCF 模拟游戏**及其可归约优势；证明完成前维持 EXPERIMENTAL。此后另做 production party credential / T→party secure delivery 与全局 anti-replay。

## 8. 复跑与证据

相关命令：

```powershell
cmake --build /tmp/bmw16-s15-rebuild --target moe_topk_bmw16_experimental_3proc_test moe_topk_bmw16_experimental_party_test moe_topk_m2_ucmp_conformance_test moe_topk_m2_transport_conformance_test moe_topk_m2_score_input_conformance_test moe_topk_m2_parallel_shuffle_conformance_test moe_topk_m2_parallel_shuffle_process_e2e_test --parallel 4
ctest --test-dir /tmp/bmw16-s15-rebuild -R "moe_topk_bmw16|moe_topk_m2_(ucmp|score_input|transport|parallel_shuffle)" --output-on-failure
./moe_topk_bmw16_experimental_party_test
./moe_topk_bmw16_experimental_3proc_test
python experiments/TEST_ONLY_BMW16_S15/run_n256_common_tape.py --cpp-test /tmp/bmw16-s15-rebuild/moe_topk_bmw16_experimental_party_test --s4 <frozen-S4-source> --out <external-evidence-dir>
python experiments/TEST_ONLY_BMW16_S15/audit_e2e_log.py <external-e2e-stdout.log>
python experiments/TEST_ONLY_BMW16_S15/capacity_preflight.py
python experiments/TEST_ONLY_BMW16_S15/ideal_abort_bound.py
python experiments/TEST_ONLY_BMW16_S15/run_pilot.py --binary /tmp/bmw16-s15-rebuild/moe_topk_bmw16_experimental_3proc_test --out <new-external-dir>
```

外部结果在 `C:\Users\28641\.codex\artifacts\BMW16_S15_20261007\`，含原始分方 stdout/stderr、所有 pilot attempts、common tapes/traces、S14 独立复跑和总 SHA-256 清单。原始记录不入 Git。逐文件 hash、失败复跑/无效 harness 记录和外部目录清单见[证据索引](evidence/BMW16_S15_EXTERNAL_EVIDENCE_INDEX_2026-10-07.md)。
