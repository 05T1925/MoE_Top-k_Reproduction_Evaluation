# MoE Top-K 详细实施计划

更新日期：2026-10-07

本文是 `PROJECT.md` 的执行版。`PROJECT.md` 定义项目范围、论文边界、统一语义和长期指标；本文将工作拆成可分配、可验证、可交接的阶段。

若本文与项目总纲冲突，以更新后的 `PROJECT.md` 和团队明确决定为准，并同步修正文档。不得仅在代码、分支名称或口头约定中形成新的协议和计量规则。

**2026-10-06 S14 有界研发附注：**S14 从 S13 `0eab2e3` 的隔离 worktree 继续复用 VFSS C++ party API，实施 bundle-wide one-shot burn 与进程内槽唯一性 guard、BMW16 shuffle-only 发材、n=128/K=2/8/80 的 common-tape 与独立进程 E2E，以及 n=64/128 的 1 warmup + 5 same-host loopback pilot。该研发由用户明确授权，提前开放的是 `EXPERIMENTAL` 研发和有界诊断，不表示 V3 完成或 V4 正式性能验收可以开始。完整 shuffle + correlated multi-key/adaptive DCF party-view simulator 和生产 T→party 私有认证交付仍未闭合/未实现；保持 NO-GO，不创建 secure alias。历史 S13 “n=128 未运行”陈述仍是 S13 截止时的准确记录，S14 的 n=128 数据只记在 [S14 决策](decisions/BMW16_S14_ONE_SHOT_MATERIAL_AND_N128_BOUNDED_PILOT_2026-10-06.md) 与 [S14 报告](reproduction/BMW16_S14_INDEPENDENT_ACCEPTANCE_VALIDATION_2026-10-06.md)。

**2026-10-07 S15 有界扩展附注：**S15 从 S14 报告提交 `1c92ca6` 独立重建并接收被测源码 `998726b`，复用同一 C++ EXPERIMENTAL 全链入口；将该实验 API/bundle 的有限域从 n≤128 扩至 n≤256，增加默认关闭的普通 DCF AES 扩展调用计数、独立日志/成本审计器、n=256 checked 容量/理想 abort 界与 common-tape/E2E/pilot 工具。对精确比较结果证明 SUCCESS soundness；理想均匀无放回采样下的实际有限参数 union bound 与计算随机源假设分开。完整 shuffle 与相关整池 DCF/adaptive view 的 simulator、生产 party 凭据/跨目录抗重放仍 NO-GO；状态保持 `PROJECT_DERIVED / EXPERIMENTAL`。V3/V4 与六方案验收没有提前完成，loopback 只属有界研究 pilot，不能并入正式性能表。具体复跑结果与外部证据哈希见 [S15 决策](decisions/BMW16_S15_SUCCESS_SOUNDNESS_ABORT_BOUND_AND_VIEW_GATE_2026-10-07.md)、[S15 验证报告](reproduction/BMW16_S15_INDEPENDENT_ACCEPTANCE_AND_N256_DIAGNOSTICS_2026-10-07.md) 和[证据索引](reproduction/evidence/BMW16_S15_EXTERNAL_EVIDENCE_INDEX_2026-10-07.md)。
**2026-10-07 S16 独立接收附注：**S16 对 S15 声明源码清单与 Git revision 做逐项审计：12 行中 7 个 C++ 源文件哈希不匹配，且无可达历史对象匹配；216 项 S15 原始归档自身校验通过。S16 因此区分“历史 S15 binary/source 绑定未核实”和“从 S15 声明 Git tree 新建的独立复跑”。后者 6/6 相关 CTest、61 行 T/P0/P1 E2E、n=256 K=2/8 S4 common-tape differential 及普通 DCF Eval 扩展计数审计通过；S15 历史 pilot 仅内部复算，不能作为源绑定性能证据。理想无放回抽样上界数字复核未变，修正了单调充分条件注释；实际 CSPRNG/PRF 计算优势未量化。计数 ON/OFF 固定带诊断观察约 28.27% median overhead，不是 V4 数据。n=1000 全池下界超过现有 cap，未运行。相关多 key 自适应 DCF+shuffle 联合 view simulator、生产凭据/交付与跨 store one-shot 仍 NO-GO；S16 未创建 secure alias，也未改变 V3→V4 正式验收顺序。详见 [S16 决策](decisions/BMW16_S16_INDEPENDENT_ACCEPTANCE_PROBABILITY_SECURITY_AND_METRICS_2026-10-07.md)、[报告](reproduction/BMW16_S16_INDEPENDENT_ACCEPTANCE_PROBABILITY_SECURITY_AND_METRICS_2026-10-07.md) 和[证据索引](reproduction/evidence/BMW16_S16_EXTERNAL_EVIDENCE_INDEX_2026-10-07.md)。

**2026-10-07 S18 用户授权的提前 EXPERIMENTAL 研发附注：**从 S17 `6f0ac1b` 隔离继续复用同一 VFSS C++ party API、稳定 Select 与原序 mask 路径；新增逐槽 KeyGen→分方 AEAD sidecar writer、紧凑 shell bundle 和 party 随机访问验证，使 T 不再先物化九池全部 uCMP/DCF keys、P0/P1 不再解密并常驻完整 key pool。固定 claim-root 的一次性合同沿用 S17，流式 API 与旧 n≤256/512MiB 内存 bundle 分开。S18 的 n≤256 与 n=1000 有界运行、外部记录及安全审查见 [S18 决策](decisions/BMW16_S18_STREAMED_KEYGEN_PARTY_READ_AND_SECURITY_REVIEW_2026-10-07.md) 和 [S18 验证报告](reproduction/BMW16_S18_STREAMED_KEYGEN_E2E_AND_SECURITY_REVIEW_2026-10-07.md)。此工作是用户明确授权的提前 `EXPERIMENTAL` 代码研发，不改写“前置条件”中的正式 M6A→M6B 次序，不表示 V3 完成、V4 正式 LAN/WAN 验收可开始或六方案比较完成。整池相关 DCF 自适应安全和 Protocol I shuffle 的联合 simulator 仍未闭合，无 secure alias。

当前状态：M2 已完成；三轮 C-INSTANTIATION、独立评审与 Theorem 4.1 逻辑通信匹配不改变 `AUTHOR_EXACT = NOT_PROVEN` 的证据边界。接收方复跑后 M5 已完成，M6A 为当前阶段。M5-H1 通过用户指定的数量级门槛，但 Theorem 4.2 精确逻辑成本仍相差 `254n` bits。F1 经 PR #25 合入；接收方在 9b3ce3747b1734602e3edf4c644ae1b6da52e8c1 通过 F2/G3 和 41/41 回归。dated strict-gate 与 H2 初始 FAIL 记录保留为历史。M5-FIX-F1 = COMPLETED；F2 / G3 = PASS。详见 [G3 接收签收](reviews/M5_TO_M6A_G3_RECEIVER_ACCEPTANCE_2026-09-25.md)。

Dealer-DPF Candidate B 已 **STOPPED / SUPERSEDED**。活动路线复用 Chase
OPV -> Share Translation -> Beneš -> Permute+Share -> two-pass
SecretSharedShuffle 作为 C1 conformance baseline；隔离的 correlated-parallel
three-round candidate 保持为准确标记的 C-INSTANTIATION。详见
[M2 Protocol I Chase redesign](decisions/M2_PROTOCOL_I_CHASE_SECRET_SHARED_SHUFFLE_REDESIGN_2026-09-21.md)。

本次修订取消 M4 CipherGPT 实施及性能任务，保留 M2、M3、M5 的编号和历史记录，将后续图升级分为 M6A AAV86、M6B BB90+DCF。

## 1. 当前结论与执行主线

### 1.1 已完成基础

| 里程碑 | 当前状态 | 保留边界 |
| --- | --- | --- |
| M0 | 仓库、来源和冻结基线已建立 | `VFSS-baseline/` 不修改 |
| M1/M1.1 | 统一语义、oracle、基础适配、metrics 和测试入口已完成 | 不重新定义 score、tie rule 和输出 |
| M2 | **COMPLETED / 已完成** | three-round C-INSTANTIATION、independent review PASS、Theorem 4.1 online logical communication match、Protocol I PR merged；`AUTHOR_EXACT = NOT_PROVEN` |
| M3 | 三轮模块化核心及 raw-score 五轮扩展已完成并冻结 | 作为 M5 的基础和对照 |
| M5 | 已完成 | F1 标准 raw-score-to-mask 入口及 G1/G2/G3 接收方复跑完成；保留 author-exact 与精确成本限制 |
| M6A | 进行中 | Protocol I、Protocol III 的 AAV86 升级与完整性能验收 |

当前 M2 工程实现标签为：

```text
m2_protocol_i_raw_score_input_modular_8round_mask_output
```

当前 M3 两个入口为：

```text
agarwal_protocol_iii_modular_3round
moe_topk_protocol_iii_raw_score_modular_5round
```

M2.16 完成的是 paper-exact 可行性与泄露审计，没有实现精确核心。M3 已在 `main@bb0d0e8` 完成整改；不得继续将 M3 写成尚待开始。

### 1.2 当前待完成目标

- Protocol I、Protocol III 各自的 AAV86 升级及完整性能验收。
- Protocol I BMW16-derived Select + DCF 与 Protocol III BB90+DCF 候选的安全组合、实现和完整性能验收。
- 六种目标方案的统一横向报告。

轮数口径固定为：

- Protocol I：Theorem 4.1 的三轮核心。
- Protocol III：Theorem 4.2 的两轮核心。
- raw-score 输入适配和原顺序 mask 输出适配另列，并计入端到端主结果。

不得将 Protocol I 目标写成“论文两轮”，也不得将核心轮数直接用作统一输入输出路径的总轮数。

### 1.3 新执行顺序

```text
M2：COMPLETED
  → M5：COMPLETED（F1/F2/G3 PASS；接收方复核 revision 9b3ce3747b1734602e3edf4c644ae1b6da52e8c1）
  → M6A 两种 AAV86 升级实现
  → M6A 完整性能验收
  → M6B Protocol I BMW16-derived Select+DCF / Protocol III BB90+DCF
  → M6B 完整性能验收
  → M7 六种方案统一报告
```

M3 作为已完成前置条件保留，不重新安排实现。M4 标记取消，不再作为 M5 的前置条件。

资料研究、设计、失败用例整理可以提前进行；依赖未冻结接口的实现和正式验收必须遵循上述顺序。

**2026-10-06 范围补充：**用户明确授权在 V3 结束前开展 S10 所定义的隔离 `EXPERIMENTAL_TEST_ONLY` 候选、raw-score adapter conformance 和有界 E2E，以解决 S9 的执行身份与正式输入接线问题。该授权仅提前开放研发/研究，不改变 V3→V4 正式验收依赖，不创建 secure API，不把 S10 的小规模 loopback 计量填入 V4 或六方案性能结果。详见 [S10 决策](decisions/BMW16_S10_RAW_SCORE_UID_ISOLATION_AND_FORMAL_ADAPTER_2026-10-06.md)。

**2026-10-06 S11 补充：**用户继续授权提前推进完整候选研发。S11 新增默认关闭的 VFSS C++ `EXPERIMENTAL` party API，已把正式 raw-score adapter、shuffle、两份四层 Select、membership、inverse shuffle 和 mask shares 接入可调用函数，并通过有限线程级 oracle E2E。该测试不是 T/P0/P1 独立 OS 进程；party-only AEAD bundle/耐久一次性领取及相关全池/实际 shuffle 联合视图 hybrid 未完成，故安全门仍 NO-GO。该研发授权不提前完成 V3/V4、不得启动或填充正式 LAN/WAN 矩阵。详见 [S11 决策](decisions/BMW16_S11_VFSS_EXPERIMENTAL_COMPOSITION_AND_SECURITY_GATE_2026-10-06.md) 与 [S11 验证报告](reproduction/BMW16_S11_CPP_EXPERIMENTAL_COMPOSITION_VALIDATION_2026-10-06.md)。

**2026-10-06 S12 补充：**用户继续授权在 V3 前做有界组合研发。S12 复用 S11 VFSS C++ party API，加入默认关闭的 `EXPERIMENTAL / TEST_ONLY` 三进程 T/P0/P1 runner、逐方 AES-GCM bundle、本地不同 UID 隔离与持久 one-shot claims；raw-score→shuffle→两任务四层 Select→membership→同置换逆路由→原序 mask shares 已在 52 个成功样本对照冻结 oracle，1 个注入算法 abort 双方一致且不发 mask。S4/C++ 使用外部共同随机 tape 的 n=2..8 全 K 轨迹 35 例通过。此实现仍缺生产认证材料传输与完整 shuffle+相关多 key FSS party-view simulator；不得升级 secure，不得开始 V4 正式 LAN/WAN 或声称 V3/V4/M7 已完成。详见 [S12 决策](decisions/BMW16_S12_THREE_PROCESS_MATERIAL_AND_PARTY_VIEW_GATE_2026-10-06.md)、[S12 报告](reproduction/BMW16_S12_IMPLEMENTATION_AND_VALIDATION_2026-10-06.md)。

**2026-10-07 S17 有界 party 接口与侧车补充：**S17 在隔离分支复用既有 VFSS C++ 全链，新增 default-OFF 的 T/party-node CLI、v3 bundle claim-root identity 与 Select/membership 随机访问密文侧车；party-node T/P0/P1 E2E 已扩到 n=256，现有 3proc 与 n=256 common-tape 差分结果仅属 EXPERIMENTAL 功能证据。T→party 仍是本地受限文件交付且 FD 由调用方提供，未实现生产认证网络。全池 adaptive DCF/shuffle 联合视图仍未过门，因此不创建 secure alias，不把该实现接入正式六方案完成项。

流式侧车只对已由现有 KeyGen 整体生成的 9·C(n,2) pair-key pool 做密文 random-access lazy read；它未降低 KeyGen 峰值内存，raw/shuffle 材料也未纳入侧车。n=1000 全池约 4.99 GB/party 的解析下界超 512 MiB cap，当前仍 n≤256，记 `PRECHECK_REJECTED / NOT_RUN`。未来真正顺序 KeyGen writer、私有交付与整池视图需单独验收。S17 是用户授权的有界接口/功能/安全研究工作，不改变 V3→V4 正式验收顺序，也不运行正式 LAN/WAN 矩阵。见 [S17 决策](decisions/BMW16_S17_INDEPENDENT_COMPOSITION_SECURITY_AND_STREAM_STORE_2026-10-07.md) 和[报告](reproduction/BMW16_S17_INDEPENDENT_ACCEPTANCE_AND_STREAM_STORE_2026-10-07.md)。

**2026-10-06 S13 补充：**独立从 S12 `646f71c` 重建并复跑 52 SUCCESS + 1 注入 abort、39 条 common-tape differential。修正异常分类/退出码合同与 S12 工作树哈希换行不可移植问题；当前 runner 能分别验证算法 abort、材料 abort、通信本地 abort、未预期异常 exit 70 和 mask 不存在。n=64 有界实验加入阶段/claim/Eval/网络计量并完成 n≤1e6 全池容量算术预检。S13 E2E 观察为 52 SUCCESS、1 注入算法 abort、1 篡改材料 abort、1 意外 API 参数异常、1 peer close；0 自然 probability abort。安全首个阻塞为相关节点 mask 的整池 DCF party view（含未用 key）与 shuffle 辅助状态的 H0→H1 联合模拟；生产 T 凭据/认证私有交付也未实现，故保持 `PROJECT_DERIVED / EXPERIMENTAL`，不升级 secure，不运行 n=128 或 V4 正式矩阵。详见 [S13 决策](decisions/BMW16_S13_RECEIVE_EXECUTION_EVIDENCE_SECURITY_CAPACITY_GATE_2026-10-06.md)、[S13 报告](reproduction/BMW16_S13_INDEPENDENT_ACCEPTANCE_VALIDATION_2026-10-06.md)、[归档索引](reproduction/evidence/BMW16_S12_S13_ARCHIVE_INDEX_2026-10-06.md)。

### 1.4 最终六种方案

1. Protocol I。
2. Protocol III。
3. Protocol I + AAV86。
4. Protocol III + AAV86。
5. Protocol I + BMW16-derived Select + DCF（PROJECT_DERIVED；S16 独立重建复跑的 opt-in EXPERIMENTAL VFSS C++ 三进程全链；S15 历史 C++ source identity 未接收；party-view/生产凭据发材仍 NO-GO；非正式性能数据）。
6. Protocol III + BB90+DCF。

已完成的 I 四轮核心和 III 三轮核心作为工程对照另列，不替代六种方案中的精确基线。

历史 `Direct Top-K` 原型不直接改名为 BB90+DCF。CipherGPT native、CipherGPT-style adapter 和 CryptoMoE 接入不属于本轮六种方案的交付范围。

### 1.5 文档同步边界

本次路线替代旧 `docs/decisions/ROADMAP_PRIORITY_2026-09-04.md` 中冲突的顺序与阶段门。

以下文档应同步：

- `PROJECT.md`
- `docs/IMPLEMENTATION_PLAN.md`
- `docs/TEAM_WORK_PLAN.md`
- `docs/M3_ONWARD_TEAM_WORK_PLAN.md`
- `README.md`
- 路线决策及配套计量规范

旧计划中的 M4→M5 前置关系不再执行。历史实现和实验记录保留，不将新目标反写为旧阶段已完成能力。

## 2. 全程不变的统一契约

### 2.1 输入和输出

- 单次调用包含 n 个 32 位二补码 signed fixed-point score 算术共享。
- 小数位数为 12，数值解释为量化整数除以 `2^12`。
- 两方 raw shares 满足 `raw = (x0+x1) mod 2^32`。
- 公开参数包含 n、K、位宽、payload 形状、party 拓扑和模式。
- 选择 score 最大的 K 个元素，同分按 original index 升序。
- 最高优先级 rank 为 0，有效范围为 `0..n-1`。
- 输出为原始输入顺序下长度 n 的秘密共享 Top-K bit-mask。
- 每位为 0/1，且恰好 K 位为 1。
- 不要求 Top-K 集合内部排序。
- selected payload 不能代替统一 mask。

统一接口：

```text
([z_1]^B, ..., [z_n]^B)
    <- TopK(([x_1]^A, ..., [x_n]^A), K)
```

元素与原始位置绑定、输入编码、shuffle、路由、逆映射、共享转换和 mask 生成均属于实际执行路径。主结果不能将这些步骤作为免费后处理扣除。

六种方案复用同一份输入语义、oracle 和输出契约。

### 2.2 安全与测试边界

- P2 为输入无关的离线材料提供方，完成分发后退出在线路径。
- secure runtime 不通过测试辅助逻辑重构 raw score、priority key、原顺序 rank、比较位、selected index 或最终 mask。
- 原始输入和中间值不得交给 Dealer 计算正确答案。
- 具体协议允许公开的 shuffled rank 或局部 rank 必须有单独的泄露说明，不能扩大为任意 rank 公开。
- oracle 重构只在隔离的 TEST_ONLY 路径发生。
- 材料必须绑定 session、party、参数和协议阶段，并按规定一次性消费。
- 不使用文件轮询、固定 sleep、模拟 shuffle 或在线补发材料隐瞒协议依赖。

新适配器按以下顺序验证：

```text
conformance
  → oracle differential
  → 独立进程 E2E
  → 消息、泄露和计量审计
```

### 2.3 测试矩阵

| n | K | AAV86 迭代 r |
| --- | --- | --- |
| 128、256 | 2、8 | 2、3、4、5 |
| `10^3`、`10^4`、`10^5`、`10^6` | 80 | 2、3、4、5 |

执行规则：

- 六种方案沿用同一 `(n,K)` 矩阵。
- AAV86 两种路线均覆盖 `r=2..5`。
- BB90 的迭代参数根据采用版本单独定义，不复用 AAV86 字段冒充已确定配置。
- 同一比较组使用同一输入、输入种子、score 解释、网络和 oracle。
- 输入种子与算法随机种子分别记录。
- 保留基础随机量化整数均匀范围 `[-32*2^12, 32*2^12]`，端点包含；其他分布单独标注。
- 正确性测试另覆盖重复值、全相等、负数、正负边界、`K=1`、`K=n`、非二次幂 n 和非法输入。
- 正式性能实验分别覆盖 LAN/WAN。
- 每个配置预热 1 次、正式运行 5 次，保留逐次结果并报告 median/min/max。
- 随机图每次运行的种子、边数、节点复杂度和性能结果关联保存。
- 无法运行的配置保留失败阶段、资源限制和原因，不缩小参数后冒充原配置。

基础协议的两次通信核验先覆盖小规模边界及 `(128,2/8)`、`(256,2/8)`，并使用可行规模检查增长趋势。它们不要求在进入后续阶段前强行完成全对全 `n=10^6` 实验。

完整长期矩阵及无法完成配置的记录在正式性能阶段处理。

### 2.4 必须输出的指标

| 字段 | 含义 |
| --- | --- |
| `offline_time_ms` | 全部预处理耗时，明确生成、序列化和分发边界 |
| `offline_material_total_bits` | 在线所需全部离线材料之和 |
| `online_time_ms` | 输入就绪至统一 mask 完成的在线耗时 |
| `online_comm_total_bits` | 在线各方实际发送字节折算后的总和 |
| `online_comm_per_party_bits` | total 除以在线方数量，主表通信字段 |
| `online_rounds` | 因果依赖决定的在线轮数 |
| `online_prg_calls_total` | 所有在线方长度倍增 PRG 调用总数 |
| `comparison_edges_total` | 实际执行的无序比较边总数 |
| `total_time_ms` | offline 与 online 时间之和 |

同时保留：

- 每方 `sent_bits`、`received_bits`；
- revision、实现标签、runtime、party topology；
- n、K、算法迭代参数；
- score、comparison、rank、payload 位宽与表示；
- logical/padded layout；
- 输入分布、输入种子、算法种子；
- CPU、内存、OS、编译器、flags、build type、线程数；
- 网络配置、带宽、RTT；
- 命令、预热次数、重复次数；
- correctness status、失败原因和原始结果定位信息。

total 只累加发送量，received 用于核验，不再次加入总量。

同一条比较边由两方分别计算，不因此记为两条边；同一位置对在不同步骤实际重复比较时，按实际次数计数。比较边数、DCF 调用数和 PRG 调用数分别记录。

未知指标使用 `NOT_MEASURED`，不能用零、估算或历史数字填充。完整性能阶段必须补齐适用计数；仅有字段而没有可信观测不算测量完成。

### 2.5 分阶段计量

每种方案至少区分：

1. raw-score 输入适配；
2. 论文核心或明确标记的组合核心；
3. 原顺序 mask 输出适配；
4. 测试控制、报告传输和 oracle 验证等辅助工作。

主比较使用完整安全路径；论文核验使用功能、参数和边界一致的核心数据。

阶段通信应能与总通信对账。并行或合并执行时按实际消息依赖计算总轮数，不机械相加，不把有依赖的两轮消息仅因放入同一函数而称为一轮。

历史计时边界保持原样。若旧记录包含 Dealer 启动、输入分发或报告收集，应继续注明；不能静默改写为纯核心时间。

## 3. 阶段门与验收规则

### 3.1 基础协议统一阶段门

M2 精确核心和 M5 精确核心均按以下顺序推进：

```text
G1：实现与正确性完成
  → G2：通信实测及差异解释完成
  → G3：接口和证据交接完成
```

| 阶段门 | 必须提供的证据 | 通过后的状态 |
| --- | --- | --- |
| G1 实现门 | conformance、oracle differential、独立进程 E2E、消息与泄露审计 | 可进入正式通信核验的候选 |
| G2 通信门 | 论文公式、实现消息推导、实际分方计数及差异解释 | 成本对应关系已核验 |
| G3 交接门 | 冻结 revision、接口契约、最小调用示例、材料及计量说明、接收方复跑记录 | 可作为下一阶段依赖 |

代码可以按依赖拆成多个可验证 PR 合并，但“代码已合并”不等于 G2/G3 已通过。

论文精确身份还要求全部相关论文前提成立；不能只凭 oracle 通过或轮数相同升级标签。

### 3.2 通信核验方法

每次核验形成三层对照：

```text
论文成本公式
  ↔ 实际参数和消息定义下的实现成本
  ↔ 独立进程执行得到的实际发送量
```

必须检查：

1. 比较功能是否一致：sorting、单个 order statistic、Top-K mask 不混用。
2. n、logical/padded 规模、输入域、rank 域和 payload 是否一致。
3. 使用了哪一条定理、脚注和优化。
4. total/per-party、bits/bytes、KB/KiB 是否一致。
5. 核心、输入适配、输出适配和封装是否分离。
6. 是否存在额外位宽、字节对齐、帧头、重复发送或不同材料表示。
7. 发送量与对端接收量是否能在同一统计边界下对应。
8. 随 n、K 增长的趋势是否符合实现消息结构。
9. 是否因遗漏必要步骤或改变泄露而得到较低通信量。

Protocol I 主要对照 Theorem 4.1。Protocol III 主要对照 Theorem 4.2，并注明是否采用其中脚注描述的共用掩码优化。

Table 2、Table 3 的估算参数和归一化方式需单独核对，不能把论文估算值写成本项目实测目标值。

### 3.3 通信门退出条件

- 原始分方和分阶段计数可复算。
- 论文公式与当前实现的对应关系明确。
- 核心及适配、封装成本分别说明。
- 关键差异已经定位并解释。
- 正确性、轮数和安全条件仍然成立。
- 接收方可按命令复跑核验配置。
- 未解决项有明确记录，不被“数量级接近”覆盖。

不要求 wire bytes 与理论 bit 数逐位相等，也不以任意误差百分比代替原因分析。

通信核验通过只支持成本一致性。数量级不符时，先定位实现或计量差异；不能直接判定论文错误。数量级相符时，也不能据此证明功能或安全性正确。

### 3.4 图升级阶段门

M6A、M6B 各自执行：

```text
算法与安全设计
  → 两种路线实现
  → 正确性、消息与材料审计
  → 计量能力验收
  → 完整矩阵性能实验
  → 报告与接口交接
```

M6A 完成两种路线的性能验收后，再进入 M6B 依赖实现及正式实验。

资料核对、明文算法 oracle 和不依赖未冻结接口的设计可以提前开展。

用户在 S10 后又逐项明确授权将 BMW16-derived Select 的隔离 EXPERIMENTAL 代码研发提前；S19 属于该授权下的复核、发布修复和有界功能验证，S20 增加离线 TLS 发材接口及独立 party 接收验证。此授权不改正式顺序：M6A、V3/V4 门禁与同口径正式性能矩阵状态不提前标记完成，也不把 S19/S20 冒烟数字写入正式比较表。

### 3.5 性能状态与失败配置

每个配置区分：

- 成功完成并取得全部适用指标；
- 正确性失败；
- 执行失败或超时；
- 资源不足；
- 指标未测或计数能力缺失；
- 参数不适用，并附算法依据。

资源无法承受的大规模点可按项目总纲记录原因和未测字段，阶段报告必须披露覆盖范围。此类记录不等于该点成功完成。

对于已经成功运行的配置，若 PRG、材料或通信等必需指标缺失，不能标为“全部指标完成”。平台性计量缺口应在正式性能验收前补齐。

不得静默丢弃失败随机种子、只保留最快结果或用重试掩盖正确性问题。

## 4. 里程碑任务与退出条件

### 4.1 M0：仓库与规范基线

状态：已完成。

保留交付物：

- 冻结提交 `993696e` 与标签 `vfss-baseline-2026-09-03`；
- 来源、论文版本、目录和忽略规则；
- 统一输出、测试矩阵、指标和证据层级；
- 远端仓库与文档入口。

历史验收以 `docs/M0_REVIEW.md` 为准。M0 时记录的 LICENSE 和参考资料分发事项保持其证据边界，不作为本次已解决事项。

不重新执行已完成的仓库初始化，不因本次改计划修改冻结树。

### 4.2 M1/M1.1：统一 oracle、数据和基础计量

状态：已完成并冻结。

保留：

- Q20.12 signed score 语义；
- 数值降序、同分 original index 升序；
- 基础输入分布与边界向量；
- 明文 stable-rank/Top-K oracle；
- DCF、CmpAgg conformance；
- 基础 MetricsRecord、provenance 和 CTest 入口。

历史验证包括：

```text
moe_topk_m1_oracle_test
moe_topk_m1_cmpagg_test
moe_topk_m1_metrics_test
moe_topk_m1_dcf_conformance_test
```

M1.1 在 Ubuntu 24.04.4、WSL2、x86_64 干净 Debug 构建中验证 revision：

```text
a2efe5e3d2d22bb3c031fb24dc3246c37d442fad
```

CTest 为 4/4 通过，详见：

`docs/M1_1_UBUNTU_HANDOFF.md`

后续只按实际需要补充计量和适配能力，不重新讨论已冻结语义，不把历史 `NOT_MEASURED` 改成已测。

### 4.3 M2：Protocol I 精确核心、通信核验与交接

状态：**COMPLETED / 已完成**。three-round C-INSTANTIATION implemented；independent three-round review PASS；online logical communication matches Theorem 4.1；Protocol I PR merged。`AUTHOR_EXACT = NOT_PROVEN` 是保留的证据边界，不阻止 M2 工程里程碑完成。以下阶段说明为已完成路径的历史技术记录。

主责：角色 A。角色 B 交叉评审并接收接口。

#### 输入

- M1 语义、oracle、测试输入与 metrics；
- 已完成的 M2 shuffle、CmpAgg、transport、package 和输入输出适配；
- Agarwal §2.4、§4.1、Theorem 4.1；
- Chase secret-shared shuffle；
- M2.15/M2.16 对 public masked-list 和泄露的审计记录。

#### M2 Chase 迁移阶段

1. **R0 migration/cleanup**：清理 Candidate-B-only 资产；冻结证据、group 与
   P0/P1/P2 knowledge contracts。
2. **R1 OPV conformance**：扩展 shape、position/value、freshness 和 process tests。
3. **R2 Share Translation**：验证 `Delta=b-pi(a)` 与 hiding boundary。
4. **R3 Beneš `(T,d)`**：验证 composition、disjointness 与 shape policy。
5. **R4 Permute+Share**：建立 clear oracle；区分 CHASE-DIRECT P0/P1
   preprocessing 与只提供 permutation-independent correlation 的 P2-assisted
   candidate。P2 同时知道 `pi0/pi1` 的 compiler 仅是 C-INSTANTIATION。
6. **R5 SecretSharedShuffle**：验证 two-pass forward shuffle，以及 P0/P1
   分别选择并保留 `pi0/pi1`、无单方获得 composed permutation 的 Chase view。
7. **R6 Agarwal adapter**：产生 same-`pi` secret shares 与 public
   `pi(key)+r`。conference version 是否禁止 inputless P2 在生成期知道完整
   `r` 为 `D-UNRESOLVED`；当前 C-INSTANTIATION 明确允许 P2 知道完整 `r`，
   knowledge-preserving 路径仍缺 distributed/blind r-bound GRank KeyGen。
8. **R7 integration**：接回 uCMP/DCF、CmpAgg、rank open 和 local routing；
   Protocol I shuffle 禁止 DPF routing。
9. **R8 E2E**：frozen-oracle differential 和独立 P2/P0/P1 process E2E。
10. **R9 accounting**：记录 causal rounds、online/offline bytes、OT/PRG/DCF
    和 wall-clock；未知值保持 `NOT_MEASURED`。

CHASE-DIRECT 中 P0/P1 分别选择 permutation，P2 不在其 party view 中。把
input-independent work 移入 P2 是项目 preprocessing C-INSTANTIATION；
P2 知道两方 permutation 或完整 `r` 不构成 author-exact 证据。

已验证的 serial Chase baseline transcript 为：

```text
offline direct: P0/P1 retain pi0/pi1 and run permutation-dependent OPV/ST
offline conservative candidate: P2 supplies only permutation-independent bases
offline r-private candidate: P0/P1 contribute r0/r1; blind GRank KeyGen BLOCKED
R1: P1(data owner) -> P0(pi0 owner), first PS message
R2: P0(data owner) -> P1(pi1 owner), depends on R1
R3: exchange q_b=z_b+r_b, reconstruct public y=pi(key)+r
R4: open shuffled rank shares; routing is local
```

隔离的 correlated-parallel C-INSTANTIATION 以同一个 `Pi` 的两组相关分解
并行处理 `x0/x1`：R1 双向得到 `Pi(x)` 的秘密分享，R2 双向得到公开
`Pi(x)+r`，本地执行 CmpAgg，R3 打开 rank shares。该候选已通过 conformance
和独立 P2/P0/P1 E2E；P2 生成期知道完整 `Pi/r`。因此实验性三轮候选已实现，
但 Agarwal author-exact transcript、严格 G1/G2/G3 与 dealer/full-`r` 语义仍
保持 **BLOCKED / D-UNRESOLVED**。

- three-round C-INSTANTIATION online logical communication: **MATCH** Agarwal
  Theorem 4.1 for identical `(n, ell', p)` parameters;
- benchmark evidence:
  [2026-09-24 online communication record](reproduction/M2_PROTOCOL_I_3ROUND_ONLINE_COMMUNICATION_UBUNTU_2026-09-24.md).

#### M2 通信阶段

已完成的通信核验：

1. 冻结被测 candidate revision 和实现标签。
2. 运行小规模边界与 `(128,2/8)`、`(256,2/8)` 核验。
3. 选择可行增长点检查通信趋势。
4. 对照 Theorem 4.1 及实际输入、payload 和 rank 表示。
5. 分别记录 raw adapter、shuffle/core、rank reveal、reverse mask 和封装成本。
6. 输出论文公式、实现推导和实际计数对照。
7. 定位并解释差异，必要时修正实现后重跑受影响配置。

#### M2 交接阶段

已完成的 M2→M5 handoff 向角色 B 提供：

- 可复跑的 revision 与构建配置；
- 公共接口、输入输出和 layout；
- score/rank 域及 payload 表示；
- 预处理材料格式、绑定和消费规则；
- transport/session/fingerprint 契约；
- 最小调用示例与错误语义；
- 分阶段计数说明；
- 正确性、轮数、泄露和通信核验报告；
- 与现有 M3 的兼容范围及不能直接复用的部分。

交接不要求 M5 使用 Protocol I 专用 shuffle。应区分可复用的公共部件和仅属于 Protocol I 的实现。

角色 B 在接收 revision 上复跑最小示例及代表性核验，记录接收结果。

#### 交付物

- Protocol I 精确三轮核心候选与统一 mask 路径；
- 独立进程运行入口；
- 测试和阶段审计；
- 通信核验报告；
- 公共接口交接记录。

M2 当前工程验收已完成；不得将其完成边界扩展为作者未公开实现的逐转录证明，`AUTHOR_EXACT = NOT_PROVEN`。

#### 退出条件

- 论文功能、同置换和材料条件成立；
- 核心三轮可由消息依赖复算；
- 原始输入顺序的 mask 与 oracle 一致；
- 没有新增未声明泄露或在线 Dealer；
- 通信计数可信且关键差异已解释；
- 交接方成功复跑并确认接口；
- 原工程基线和冻结语义未被覆盖。

### 4.4 M3：Protocol III 模块化三轮基线

状态：已完成，保留回归与维护。

实现规范见：

`docs/decisions/PROTOCOL_III_MODULAR_3ROUND_DESIGN.md`

保留的三轮路径：

```text
GRank
  → masked-rank DPF routing
  → secure combine
```

保留的 raw-score 五轮扩展：

```text
carry
  → sign
  → GRank
  → DPF routing
  → secure combine
```

已完成交付物：

- 两个不同输入边界的正式 Party executable；
- DPF、GRank、combine 与 raw-score 测试；
- 原顺序 XOR Top-K mask；
- 独立 Dealer/P0/P1 测试角色；
- 结构化 MetricsRecord 汇总；
- 三轮/五轮因果关系和 secure/test 边界记录。

后续维护要求：

- priority-key 三轮入口不能写成 raw-score 三轮入口；
- 不通过改名将 M3 升级为论文两轮；
- 新计量字段不得静默改变旧记录；
- M5 与 M3 使用同一 oracle 做差分；
- 单位 payload 的 mask 特化不能覆盖一般路由基线身份。

关闭证据见：

`docs/reproduction/M3_REVIEW_CLOSEOUT_UBUNTU_2026-09-10.md`

### 4.5 M4：CipherGPT 原生基线

状态：取消。

本轮不安排：

- CipherGPT 原生修复；
- original-index 或 mask 适配；
- CipherGPT-style VFSS adapter；
- CipherGPT 性能测试；
- CipherGPT 与六种方案的正式比较。

保留编号、历史资料与忽略规则。删除其作为 M5 前置条件的执行要求，不删除历史实验来源。

### 4.6 M5：Protocol III 精确两轮、通信核验与交接

主责：角色 B。角色 A 交叉评审。

状态：COMPLETED（接收方 F2/G3 PASS）。`protocol_iii_raw_score_mask_party` 是 M5-FIX-F1 提供的标准入口，输入为 signed Q20.12 份额，输出原输入顺序 XOR Top-K mask 份额；项目完整路径为四轮，通用 field Fselect/Fsort 核心仍为两轮。M5 在项目工程验收边界关闭，不改变 `AUTHOR_EXACT = NOT_PROVEN` 或 Theorem 4.2 精确成本不匹配的结论。

M5-B/C 的共享 CmpAgg 接口及三轮模块化路由已通过差分与独立进程回归。
M5-D 已增加独立的 `F_(2^127−1)` payload field、非零编码、field Beaver
乘法材料和 Protocol-III-only field-output DPF 包装层；决策与限制见
[2026-09-25 field payload decision](decisions/M5_PROTOCOL_III_FIELD_PAYLOAD_DECISION_2026-09-25.md)
及 [M5-D conformance](reproduction/M5_PROTOCOL_III_FIELD_PAYLOAD_CONFORMANCE_2026-09-24.md)。
当前 M3/M5-C `Z_(2^64)` 三轮 bit-mask runtime 保持原样。M5-E 已在独立
candidate 中组合 shared CmpAgg、M5-D field DPF 与非零 payload mask，
完成单 rank Fselect 的两轮因果状态机和差分/传输测试；见
[2026-09-25 two-round candidate decision](decisions/M5_PROTOCOL_III_TWO_ROUND_CANDIDATE_DECISION_2026-09-25.md)
及 [M5-E closeout](reproduction/M5_PROTOCOL_III_TWO_ROUND_FSELECT_CANDIDATE_2026-09-25.md)。
M5-F 已完成独立 fork+exec P2/P0/P1 process E2E：P2 离线分发完整 party-local
bundle 并在在线输入释放前退出；P0/P1 各自执行两个 framed exchange，过程事件
证明同轮 outbound 在 peer 消息读取前冻结，R2 后无协议发送。见
[M5-F process closeout](reproduction/M5_PROTOCOL_III_TWO_ROUND_PROCESS_E2E_2026-09-25.md)
及 [M5-F process decision](decisions/M5_PROTOCOL_III_TWO_ROUND_PROCESS_BOUNDARY_DECISION_2026-09-25.md)。
M5-G 复用同一 CmpAgg、field DPF keys 与两轮 R1/R2 transcript，在 R2 后通过
field FullEval 本地形成完整 rank-order field-record share vector；独立进程
P2/P0/P1 Fsort 差分与因果回归已通过。见
[M5-G decision](decisions/M5_PROTOCOL_III_FSORT_FULLEVAL_DECISION_2026-09-25.md)
及 [M5-G closeout](reproduction/M5_PROTOCOL_III_FSORT_FULLEVAL_2026-09-25.md)。
M5-H1 已完成 n=2–128 的独立进程 Fselect/Fsort 通信量测量，按用户指定的
数量级门槛通过；实现/论文逻辑位比约 1.42–1.44，精确成本式多
`254n` bits，且 shared uCMP 每边调用两次 DCF Eval。详见
[M5-H1 communication evidence](reproduction/M5_PROTOCOL_III_2ROUND_ONLINE_COMMUNICATION_UBUNTU_2026-09-25.md)。
M5-H2 独立评审最初因 F1/F2 关闭 FAIL。F1 经 PR #25 合入后，接收方在 main@9b3ce3747b1734602e3edf4c644ae1b6da52e8c1 通过 G3。H1 的数量级结果不等于 Theorem 4.2 精确成本匹配，也不代表 author-exact。
M5-FIX-F1 四轮 bit-mask 专用路径通过接收方差分、独立进程、成本与失败检查。详见 [F1 决策](decisions/M5_FIX_F1_SECURE_IO_ADAPTER_DECISION_2026-09-25.md)、[F1 证据](reproduction/M5_FIX_F1_SECURE_RAW_SCORE_TO_MASK_E2E_2026-09-25.md)和 G3 接收签收。该路径由两轮输入适配和两轮 CmpAgg/DPF 核心组成；不能通过删除 combine 宣称匹配 Theorem 4.2 通用 field 成本。通用 field Fselect/Fsort API 保持冻结，通用 ring-to-field 与 field-to-XOR 转换仍未实现。
原 general-field candidate 的入口为已分享的 field payload，Fselect 出口为单个 selected
record 份额，Fsort 出口为 rank-order record 份额向量。其通用 secure ring-to-field
输入转换和 field-output→原顺序 mask adapter 仍未实现；F1 已另行提供 bit-only
标准 mask 路径并完整计量。不能将完整 raw-score 路径称为两轮。非 2 幂 `n` 的 rank 域仍为项目
`Z_(2^rank_bits)` 实例化，不因 payload field 的加入而等同论文 `Z_n`。

#### 前置条件

- M3 已完成并保持稳定；
- M2 已完成的 CmpAgg/rank-share 契约及 handoff 资产可复用；
- M5 所需公共接口及其代数适用范围明确。

M5 的实现、通信核验与 F2/G3 接收方签收已完成。下列实现步骤保留为 M5 的技术记录；后续 M6A 必须基于已签收接口重新审计 AAV86 的自适应图、材料生成和泄露边界。

#### M5 实现阶段

1. 固定满足论文要求的域及表示。
2. 定义非零 payload、零值编码、乘法掩码和逆元失败语义。
3. 核对 VFSS DPF 输出与该表示的兼容性，不能把 `Z_(2^b)` 直接视为域。
4. 划分可复用的 M2 公共接口与必须新增的 M5 适配。
5. 实现 GRank 与 DPF routing 的跨阶段压缩。
6. 验证两轮论文核心，不遗漏输入或 mask 适配。
7. 与 M3 对同一输入、K 和种子进行差分。
8. 运行 conformance、独立进程 E2E、材料复用拒绝和传输异常测试。
9. 审计允许公开值、Dealer 行为和消息依赖。

仅因单位 payload 可简化计算而删掉 combine 轮，不能直接作为 Theorem 4.2 精确复现证据。

#### M5 通信阶段

G1 通过后：

1. 冻结 candidate revision 与标签。
2. 运行与 M2 核验功能可比的小规模及正式基线点。
3. 对照 Theorem 4.2，注明共用掩码优化是否采用。
4. 分解 GRank、压缩路由、表示转换和 mask 输出成本。
5. 比较 M3 与 M5 的通信、轮数及代数适配开销。
6. 核验单个 order statistic 与项目 Top-K mask 的区别。
7. 输出三层通信对照、差异解释与未解决项。

#### M5 交接阶段

G2 通过后，为 M6A 提供：

- 冻结的基线 revision；
- M2/M5 公共部件复用矩阵；
- 域、payload、DPF 与输入输出适配契约；
- 材料生成和一次性消费规则；
- 核心及端到端阶段计量；
- 正确性和通信核验报告；
- 图升级必须重新解决的假设；
- 最小调用示例与复跑结果。

不得将全对全材料生成器直接描述为已支持自适应比较图。

#### 交付物

- `agarwal_protocol_iii_exact_2round` 目标核心及统一 mask 路径；
- 域、非零编码、消息和泄露决策；
- M3/M5 差分及开销对照；
- 通信核验报告；
- M6A 接口交接记录。

#### 退出条件

- 论文代数条件满足；
- 两轮核心经消息审计和独立进程执行验证；
- 输出与冻结 oracle 一致；
- 输入、输出及表示转换成本完整记录；
- 通信差异已解释；
- 两条基础路线的交接可复跑；
- 只有上述条件通过后才更新精确身份。

### 4.7 M6A：AAV86 两种升级与完整性能验收

#### 前置条件

- M2、M5 精确核心与两次通信核验完成；
- 基础接口交接完成；
- M3 工程对照仍可运行。

#### 设计阶段

1. 固定 AAV86 算法及 CA 转换来源。
2. 明确图生成、pivot、bucket、局部 rank 和稳定同分语义。
3. 定义输入种子与算法随机种子。
4. 写明每轮公开值、图生成时机、材料和 Dealer 行为。
5. 解决输入无关、Dealer 在线静默的自适应 exact-edge 预处理。
6. 分别给出 Protocol I、Protocol III 组合协议与输出路径。
7. 审计 Protocol III 组合的代数条件和泄露，不能直接套用 shuffle-based compiler 的证明。

Protocol I+AAV86 对应构造的核心目标为 `2r+1`。Protocol III+AAV86 的 `2r` 是团队组合目标，必须独立推导与验证。

#### 实现阶段

分别交付：

```text
Protocol I + AAV86
Protocol III + AAV86
```

每种方案具有独立标签、入口和计量记录。共享算法部分只保留一份明确契约，不复制另一套 score 或 oracle。

不得使用在线 Dealer 原型、完整图预留或明文图测试冒充目标协议。不同模型的对照必须单独标记并完整计量。

#### 正确性与安全验收

- 明文图算法与稳定 Top-K oracle 一致；
- 安全图执行与明文算法差分一致；
- 重复值、全相等和非二次幂输入正确；
- 输出保持原始顺序且恰有 K 位为 1；
- 自适应材料生成、边绑定和跨轮消费可审计；
- 公开 bucket、局部 rank 和位置的泄露有明确说明；
- 两种组合的轮数独立验证。

#### 计量与性能阶段

1. 补齐可信 PRG 计数、每轮通信、离线材料和时间边界。
2. 记录每次运行实际 `e_A(n,r)`、`v_A(n,r)`。
3. 对 `r=2,3,4,5` 运行统一矩阵。
4. 分别完成 LAN/WAN 实验。
5. 每配置预热一次、正式五次，汇总 median/min/max。
6. 与对应全对全基线比较完整端到端成本。
7. 将比较量下降与额外轮数、材料、路由及适配成本一起报告。
8. 保存所有配置状态、失败原因和覆盖范围。

满足 Theorem 5.1 对应构造时，可将实测节点、边及通信与其理论项并列；其他组合使用明确标记的独立推导。

#### 交付物

- 两种 AAV86 升级实现；
- 图算法、预处理、泄露和轮数设计；
- conformance、差分和独立进程 E2E；
- 完整适用指标与逐次结果；
- LAN/WAN 性能报告；
- 未完成配置清单；
- 供 M6B 复用的接口与计量说明。

#### 退出条件

- 两种路线均完成实现与正确性、安全审计；
- 计量能力通过核验；
- 成功运行配置的全部适用指标齐全；
- 统一矩阵各配置均有成功或明确失败记录；
- 比较结论与实际覆盖范围一致；
- 报告经另一方复核；
- M6A 实现与性能验收完成后，才进入 M6B 依赖实现和正式实验。

### 4.8 M6B：Protocol I BMW16-derived Select + DCF / Protocol III BB90+DCF

#### 2026-10-06 Protocol I 候选更新

按用户明确范围，六方案中的 Protocol I 选择路线候选为 **Protocol I + BMW16-derived Select + DCF**（`PROJECT_DERIVED`），不登记为 BB90 算法或 BMW16 定理直接实现。本节的 Protocol III 路线仍按 BB90 研究目标执行。S7 集中式原型与报告保留历史身份；S8 已交付 T 离线发材、P0/P1 分进程的 `EXPERIMENTAL_TEST_ONLY` raw-score→原序 mask 组合，并验证有限样本。S8 联合 party-view 安全仍 `NO-GO`，未新增 secure runtime，完整性能矩阵 `NOT_RUN`。正式安全入口仍须等待独立安全门；M6A 与正式 V4 性能验收的既定依赖不因隔离实验授权而标记完成。见 [S7 决策/报告](docs/decisions/BMW16_S7_PROTOCOL_I_COMPOSITION_DECISION_2026-10-06.md) 与 [S8 决策/报告](docs/decisions/BMW16_S8_OFFLINE_THREE_PROCESS_COMPOSITION_DECISION_2026-10-06.md)。

S9 在隔离实验路径接入正式 `uint32_t` raw-score share 语义的 carry/sign-lift 端口，并独立重跑 S8 功能矩阵、raw-score oracle differential、n=64 非全量抽样样本及边审计。S9 复核发现同 OS token 可以读 peer share/package；其两世界 witness 以概率 1 区分；匿名公开 transcript 的 n≤5 枚举不覆盖本地 shuffle state 或共享 node-mask 相关的完整 DCF/uCMP pool。故 `OFFLINE_PRIVATE_MATERIAL` 与完整 `PARTY_VIEW_SECURITY` 继续 `NO-GO`，未建 secure API、未运行正式性能矩阵。该实验 raw adapter 不是正式 runtime adapter 接收。见 [S9 决策](decisions/BMW16_S9_RAW_ABI_AND_PRIVACY_GATE_2026-10-06.md) 与 [S9 报告](reproduction/BMW16_S9_INDEPENDENT_ACCEPTANCE_RAW_ABI_AND_VIEW_AUDIT_2026-10-06.md)。

S10 在不同 Linux UID 的本地 WSL 环境独立复跑 S9，并新增默认关闭的正式 raw-score party adapter 与 TEST_ONLY T/P0/P1 接线。11 个 n≤8 样本和一个 n64 样本通过 frozen C++ oracle；同 UID 文件读取夹具改为 UID ACL 双世界访问测试。root harness 本地拷贝材料、command-line metadata、无 AEAD/来源认证、公开 adaptive transcript 和相关全池 key 的联合模拟仍是明确安全缺口，因此只接受 `FUNCTIONAL_TEST_ONLY`，不建立 secure API；V3/V4 正式次序和 LAN/WAN 门不变。见 [S10 决策](decisions/BMW16_S10_RAW_SCORE_UID_ISOLATION_AND_FORMAL_ADAPTER_2026-10-06.md)、[验证报告](reproduction/BMW16_S10_INDEPENDENT_ACCEPTANCE_AND_EXECUTION_ISOLATION_2026-10-06.md) 与 [运行 summary](reproduction/evidence/BMW16_S10_RUN_SUMMARY.json)。

S11 在 S10 后新增 C++ `EXPERIMENTAL` 全链 party 函数与 T 的输入无关内存发材生成器，使用实际 uCMP/DCF 与 Protocol I shuffle state machine，party selection control 已从 S10 Python runner 移入 VFSS C++。固定 n=1/2/3/5/8/64 矩阵与强制 abort 通过 TEST_ONLY frozen oracle；n64 实际走非全量抽样分支。测试由一个进程中的两条 C++ party 线程执行，不能当作独立 OS process E2E。T→party 包未实现认证保密交付/耐久消费；实际 shuffle + 相关多 key FSS pool + adaptive transcript + local shuffle state + output shares 的 simulator 未证明。方案保持 `EXPERIMENTAL`，只记有界功能通过，不进入 secure API 或正式性能表。详见 [S11 决策](decisions/BMW16_S11_VFSS_EXPERIMENTAL_COMPOSITION_AND_SECURITY_GATE_2026-10-06.md) 和 [S11 验证报告](reproduction/BMW16_S11_CPP_EXPERIMENTAL_COMPOSITION_VALIDATION_2026-10-06.md)。

S12 在 S11 原 C++ party API 上完成本机三进程实验路径：T 使用公开参数预发 party-only AES-GCM bundles 并在 raw input shares 产生前退出；P0/P1 以 UID 22012/22013 运行，使用正式 raw adapter、真实 Protocol I shuffle/uCMP/DCF、C++ 四层控制、membership、逆路由和 mask shares。52/53 次 E2E 为成功并通过 oracle；失败的 1/53 是显式注入 abort，不观察到自然 abort。外部共同 tape 对 n=2..8 所有 K 共 35 条 C++/S4 控制轨迹通过。bundle 与 slot claim 通过本地测试负例，但 recipient credential provisioning 与跨主机认证交付未实现；实际 shuffle 与相关整池 FSS/DCF 自适应联合模拟仍 NO-GO。完整源码仍 opt-in 且明确 `EXPERIMENTAL`，无 secure API；此提前研发不改变 V3→V4 正式验收顺序，也不启动性能矩阵。见 [S12 决策](decisions/BMW16_S12_THREE_PROCESS_MATERIAL_AND_PARTY_VIEW_GATE_2026-10-06.md)、[S12 实施/验证报告](reproduction/BMW16_S12_IMPLEMENTATION_AND_VALIDATION_2026-10-06.md)。

S13 独立接收 S12 `646f71c`，修正 party API 对意外异常的宽泛吞错、transport/material 字符串分类和 runner 的 `return 0` abort 问题；结构化状态与 exit code 对齐，启动器验证退出码、双方报告、状态 scope 和 mask 文件。独立复建 S12 的 52 SUCCESS/1 注入 abort 与 39 tape matrix 均复现；本轮加双 bundle 篡改、错误 API 输入、peer close 负例。实验计量显示 n=64 单次约 8.95 s API 时间主要由 5,935 次持久 slot claim callback 的 fdatasync/目录 fsync 路径占用；FSS Eval 和网络交换均为毫秒级。本次另给 n=64..1e6 checked-arithmetic 全池下界，n=128 仍因当前硬 cap 和缺少完整验证未运行。复合联合安全第一个失败步骤是共享节点 mask 相关的完整 DCF key pool（含未用槽）、shuffle 辅助状态与自适应 Eval transcript 的 H0→H1 simulator 缺失；当前仍 NO-GO、EXPERIMENTAL、不开始 V4。见 [S13 决策](decisions/BMW16_S13_RECEIVE_EXECUTION_EVIDENCE_SECURITY_CAPACITY_GATE_2026-10-06.md) 与 [S13 报告](reproduction/BMW16_S13_INDEPENDENT_ACCEPTANCE_VALIDATION_2026-10-06.md)。

S14–S18 决策与验证细节见各自 dated 文档。S18 将原全池内存路径改为逐槽流式离线 KeyGen 与 sidecar 随机读取，并在其源码上完成 n=1000/K=80 路径；但多文件 no-clobber 发布及失败重试合同仍需后续修复。S19（2026-10-07）接收 S18 精确提交 `9e25411027a01bcf47b9c316e3c463c2f7c72127`，从其 tree 在隔离分支重建；将 bundle shell 发布改为 no-clobber 并按 inode 只清理本次拥有产物，增加绑定双方 shell manifest 的 paired-ready marker、要求 launcher 等待 T exit 0 后才启动 parties，以及相应普通失败/崩溃/既存文件/同 session 重试注入。TEST_ONLY 三进程通过 n=1/2/5/8/64/128/256 和 n=1000/K=80 冻结 oracle；S4/C++ common-tape 本轮 39 条 n≤8 和 2 条 n=256 轨迹通过。全部结果为本地 WSL/有限样本，非性能验收。当前候选仍 `PROJECT_DERIVED / EXPERIMENTAL`：相关阈值的整池、输出感知、自适应多 key DCF simulator（H1→H2）先缺，Protocol I shuffle/inverse/output share 联合 simulator 随后缺；候选匿名 transcript 泄露尚未由所有者批准，生产 party 凭据及跨主机私有认证交付也未实现。正式 V4 五次 LAN/WAN 矩阵与六方案排序 `NOT_RUN`。见 [S19 决策](decisions/BMW16_S19_PUBLICATION_RETRY_AND_PARTY_VIEW_DECISION_2026-10-07.md)、[验证报告](reproduction/BMW16_S19_INDEPENDENT_ACCEPTANCE_AND_PUBLICATION_FAULT_REPORT_2026-10-07.md) 与[外部证据索引](reproduction/evidence/BMW16_S19_EXTERNAL_EVIDENCE_INDEX_2026-10-07.md)。

S20（2026-10-07）接收 S19 精确提交 `617cd3cbbb7b5513c9f606ef5ceb7a73e7a96ef4`，在同一 VFSS party API 上增加 OpenSSL TLS 1.3 mutual authentication 的离线 T→P0/P1 package delivery，保持 Select、FSS key ABI 与 raw-score→mask 函数不变。T 用公开参数逐槽发材到 T 私有 spool，以分块 TLS 传输双方 AEAD sidecar 与 shell；接收端校验身份/参数/长度/digest、先暂存再 no-clobber 持久发布。默认构建不编译测试 failpoint；TEST_ONLY failpoint 必须显式打开且 BUILD_TESTING=ON。独立 UID loopback 验证覆盖 n=2/5/8/64/128/256 的 fresh-material 在线 oracle E2E，以及错凭据/元数据、分块、接收 fsync/发布、既存目标、T 部分提交崩溃。S20 n=1000 fresh TLS 路径试运行及失败/重跑状态见 [S20 验证报告](reproduction/BMW16_S20_INDEPENDENT_ACCEPTANCE_TLS_DELIVERY_AND_JOINT_VIEW_2026-10-07.md)。首次 n=1000 尝试暴露 30 秒 party 在线超时短于全链 Select 执行耗时，延长为有界 10 分钟后以 fresh bundle 复跑；所有 raw shares 只在 T exit 0 且两个 receiver exit 0 后由 TEST_ONLY 控制器创建。T 提交不具备跨端原子性：P0 已发布、P1 失败时会有 orphan，启动器必须以三个正常退出为条件并作废该 session，不能以 marker 代替。mTLS local test 不验证跨主机生产凭据配置；在线协议 fd 仍是调用方注入，S20 未实现 online transport supervisor。H1→H2 全池自适应多 key DCF view 与 H2→H3 shuffle/inverse 联合模拟仍 NO-GO；正式 V4/六方案矩阵未运行。见 [S20 决策](decisions/BMW16_S20_PRIVATE_TLS_DELIVERY_AND_JOINT_VIEW_GATE_2026-10-07.md)。

S21（2026-10-07）从 S20 精确提交 0ec88896fcc72440818bd3110dac482eced80e2c 复用唯一 C++ Select/party/material API。新增默认关闭的在线 TLS 1.3 mTLS 完整数据通道，使 ProtocolIFramedChannel 全帧经 SSL byte stream；S21 测试版新增隔离概率 abort failpoint，普通 BUILD_TESTING=OFF Release 无 failpoint。可信本地启动 gate 等待 T、P0/P1 receiver 全部 exit 0，校验双 package 后才执行创建输入 callback。独立不同 UID loopback E2E 覆盖 n=1 shortcut 与 n=2/3/5/8/64/128/256；S20 精确源码本轮也单独构建、39 条 common-tape 和代表 TLS E2E 复跑。普通 Protocol I transport/uCMP/score/shuffle、Protocol III、DCF 与 BMW16 party 定向 8 项 CTest 通过。在线通道和本地 gate 不是跨主机生产服务；生产证书运维/进程退出控制面仍待实现。AdaptiveAuxiliaryOutputMultiKeyDCF-Privacy 及 H2→H3 shuffle/output 联合模拟未闭合，保持 PROJECT_DERIVED / EXPERIMENTAL，无 secure alias、无正式 LAN/WAN/V4 矩阵。详细结果与哈希见 [S21 验证报告](reproduction/BMW16_S21_INDEPENDENT_ACCEPTANCE_ONLINE_TLS_AND_SECURITY_GATE_2026-10-07.md)。

#### 前置条件

M6A 两种路线的实现、性能验收及交接完成。

BB90 原文研究和明文算法设计可以提前进行，但不能以此跳过 M6A 的性能阶段。

#### 设计阶段

1. Protocol I 固定 S4/S9 选择候选版本、有限规模行为、失败语义与安全门；不得继承 BMW16 Theorem 8 的概率或比较常数。S8/S9 的三进程材料合同只在 `EXPERIMENTAL_TEST_ONLY` 范围运行，不能视作 secure 接收或正式性能入口。S9 的 raw input 移植 differential 不等于正式 raw adapter/party-view 接收。
2. Protocol III 固定 BB90 采用版本、适用范围、迭代参数及概率保证。
3. 明确两种算法各自从顺序统计量到本项目稳定 K-th 优先级的映射、成本界和失败事件。
4. 给出各自比较图/阶段到安全组合的转换依据。
5. 定义 selected stable key 的秘密共享表示和 DCF membership 方向。
6. 设计在线阈值与输入无关预生成 DCF 材料的衔接。
7. 分别设计 Protocol I、Protocol III 路线的完整阶段。
8. 推导消息依赖和总轮数，不预先套用 AAV86 或明文 Select 的四层公式。

#### 实现阶段

分别实现：

```text
Protocol I + BMW16-derived Select + DCF
Protocol III + BB90+DCF
```

完整功能为：

```text
选择算法得到稳定第 K 大阈值
  → DCF 生成成员指示共享
  → 必要的路由、逆映射和共享转换
  → 原顺序 Top-K mask
```

“投票”表示每个位置是否属于 Top-K 的秘密共享成员指示，不增加明文投票公开步骤。

仅以原始 score 与第 K 大 score 做 `>=` 比较无法保证重复值情况下恰选 K 个。必须沿用稳定同分语义，使阈值比较与冻结 oracle 完全一致。

不得将选出阈值本身写成完整 Top-K 协议，也不得将历史 Direct Top-K 原型仅改名后作为 BB90 实现。

#### 正确性与安全验收

- 两种选择算法各自与稳定顺序统计量 oracle 一致；
- DCF 成员选择与完整 Top-K oracle 一致；
- 覆盖随机、重复值、全相等、边界、非二次幂及 K 边界；
- 阈值、selected index 和最终 mask 不通过测试路径进入 secure transcript；
- 在线阈值相关预处理不引入未声明 Dealer 参与；
- 两种组合各自的材料、公开值和轮数经过审计。

#### 计量与性能阶段

分别计量：

1. Select 选择阶段（Protocol I 与 Protocol III 分别报告其来源标签）；
2. DCF 成员选择；
3. 输入、路由、逆映射和 mask 适配；
4. 完整端到端路径。

记录实际迭代数、节点和边计数、DCF/uCMP/PRG 调用、离线材料、每轮通信和时间。

在安全入口通过后，对统一 `(n,K)` 矩阵及各自固定参数进行 LAN/WAN 完整性能实验。遵循预热一次、正式五次及 median/min/max 规则。

与对应全对全和 AAV86 方案比较，不能只报告 BB90 图内部成本而省略最终 DCF 选择。

#### 交付物

- Protocol I BMW16-derived Select + DCF 与 Protocol III BB90+DCF 两种组合实现；
- 算法版本、阈值表示及预处理设计；
- 正确性、泄露和消息审计；
- 全部适用性能指标；
- 逐次结果、汇总报告和失败配置；
- 六种方案共同比较所需的接口与数据说明。

#### 退出条件

- 两种路线均完成完整 Top-K mask 路径；
- 稳定同分和恰好 K 个成员条件通过；
- 安全模型和概率保证明确；
- 成功配置全部适用指标齐全；
- 全矩阵配置状态和限制有记录；
- 阈值、DCF 和适配成本均计入；
- 报告经过交叉复核后进入 M7。

### 4.9 M7：六种方案统一性能报告

M7 汇总前述已完成结果，不把 M6A、M6B 的性能验收推迟到此阶段。

#### 主比较组

| 比较 | 目的 |
| --- | --- |
| I vs III | 比较全对全精确核心路线 |
| I vs I+AAV86 | 评估 I 路线的图排序升级 |
| III vs III+AAV86 | 评估 III 路线的图排序升级 |
| I vs I+BMW16-derived Select+DCF | 评估 I 路线的选择升级 |
| III vs III+BB90+DCF | 评估 III 路线的选择升级 |
| I+AAV86 vs I+BMW16-derived Select+DCF | 相同路线比较排序与选择候选 |
| III+AAV86 vs III+BB90+DCF | 相同路线比较两种升级算法 |
| I+AAV86 vs III+AAV86 | 相同图算法比较组合路线 |
| I+BMW16-derived Select+DCF vs III+BB90+DCF | 比较不同选择候选及组合路线 |

工程对照另列：

- I 四轮核心与三轮精确核心；
- III 三轮模块化核心与两轮精确核心；
- core、raw adapter、mask adapter 和封装成本；
- 必要且真实执行的 shuffle backend 专项对照。

CipherGPT 不进入本轮报告比较组。

#### 任务

1. 对齐六种方案的 revision、输入、环境、输出和计量边界。
2. 对实现或环境变化导致不可比的配置补跑。
3. 汇总全部长期矩阵配置及失败原因。
4. 展示 offline、online、total、per-party、轮数、PRG、边数和总时间。
5. 分开呈现理论、历史、当前实测和未测。
6. 报告性能拐点、资源限制及结论适用范围。
7. 保留两次基础通信核验的公式与差异解释。
8. 完成交叉审计和可复跑说明。

#### 退出条件

- 六种方案身份与实际完成情况明确；
- 比较功能、参数、环境和统计口径可比；
- 图表与数字可追溯到逐次记录；
- 不将不同安全模型放入无说明的统一速度排名；
- 未完成配置和缺失指标完整披露；
- 不以外推数据填补实测矩阵；
- 不因已写报告而把未实现或未验收方案标为完成。

CryptoMoE 在 M7 之后另行确定 eligibility、dummy、容量和允许公开的 transcript，不属于本轮退出条件。

## 5. 两人协作与接口所有权

### 5.1 角色 A：搭档

M2 已完成后的主责：

- 维护 M2 已完成的 Protocol I 资产与 handoff；
- 参与 M5 CmpAgg/rank-share 复用评审；
- M5 的交叉评审。

后续建议分工：

- M6A/M6B 公共比较材料、预处理及 Protocol I 路线；
- Protocol I 及其两种升级的性能运行；
- 材料生成、Dealer 和通信边界审计。

不再承担 M4 CipherGPT 任务。

### 5.2 角色 B：你

M5 当前主责：

- 总体计划、实施计划和分工文档修订；
- 推进 M5 域、编码、消息依赖及失败用例；
- 复用 M2 handoff；
- M5 精确两轮核心、通信核验与交接。

后续建议分工：

- M6A/M6B 的 Protocol III 路线；
- 统一输出与 Protocol III 适配；
- Protocol III 及其两种升级的性能运行；
- 结构化结果与跨协议对照。

M3 已完成，不重新安排为当前实现任务。

### 5.3 双方共同负责

- 公共图算法、oracle 和材料契约评审；
- 两次通信核验的交叉复跑；
- M6A/M6B 安全设计和阶段退出评审；
- 全部指标的计量口径；
- M7 汇总与结果审计。

图算法实现、公共计量及共享文件在每个阶段指定一名实际写入负责人，另一方评审；不同时维护两份不同实现语义。

详细到文件和 PR 的分工由更新后的 `docs/TEAM_WORK_PLAN.md`、`docs/M3_ONWARD_TEAM_WORK_PLAN.md` 承接。

### 5.4 共享资产

```text
VFSS/include/moe_topk/score_semantics.h
VFSS/include/moe_topk/topk_oracle.h
VFSS/include/moe_topk/metrics.h
docs/decisions/M1_SCORE_SEMANTICS.md
PROJECT.md
docs/IMPLEMENTATION_PLAN.md
docs/TEAM_WORK_PLAN.md
docs/M3_ONWARD_TEAM_WORK_PLAN.md
README.md
```

规则：

- score、tie rule、oracle 和输出语义保持冻结。
- 计量能力可按真实需求扩展，但同步说明影响，不静默改变旧字段。
- 同一共享文件同一区域不同时修改。
- 两条协议线不复制另一套 rank、输入生成或指标定义。
- 跨协议公共接口在真实调用需求出现后提取。
- 角色 A 不顺带修改 III 路由，角色 B 不顺带修改 I shuffle。
- 必要的公共变更独立成 PR，并运行受影响回归。
- 不修改 `VFSS-baseline/`。

### 5.5 交接记录要求

每次 G3 交接至少记录：

| 项目 | 内容 |
| --- | --- |
| 基准 | revision、分支、实现标签、构建配置 |
| 功能 | 输入、输出、公开参数、错误语义 |
| 表示 | score、rank、payload、logical/padded layout |
| 材料 | 生成方、时机、绑定、一次性消费 |
| 通信 | transport、session、阶段及消息顺序 |
| 计量 | core/adapter、total/per-party、计时边界 |
| 验证 | conformance、差分、E2E、轮数、通信报告 |
| 使用 | 最小调用示例与复跑命令 |
| 限制 | 已知缺口和不可直接复用的假设 |
| 接收 | 接收方复跑结果与日期 |

交接记录是可运行证据，不以“接口已经写好”的口头说明代替。

## 6. 分支与合并顺序

### 6.1 基本规则

- 从最新 main 建立短生命周期分支。
- 一个 PR 只处理一个明确实现步骤或治理变更。
- 不在一个 PR 中混合 Protocol I、Protocol III 和下一种升级算法的无关改动。
- 文档与设计可以提前合并，依赖 runtime 的验收服从阶段门。
- 原始密钥、日志、论文、构建产物和参考工程不进入普通 Git 历史。
- 计划中的目标名称不代表已有 executable 或已完成状态。

### 6.2 当前合并顺序

1. 合并本次范围、路线和分工文档。
2. 按依赖合并 M2 精确功能及相关测试。
3. 合并 M2 通信计量、核验报告和必要修正。
4. 完成 M2 G3 接口交接。
5. 合并 M5 所需域与适配，再合并精确核心和 E2E。
6. 合并 M5 通信核验及必要修正。
7. 完成 M5 G3 基础接口交接。
8. 合并 M6A 设计、必要公共接口和两种升级实现。
9. 完成 M6A 计量与完整性能报告。
10. 合并 M6B 设计、两种组合实现及验证。
11. 完成 M6B 完整性能报告。
12. 完成 M7 汇总和必要补跑。

每阶段可以拆多个 PR；前置阶段未通过时，不以“已经合并”绕过验收门。

M4 不再出现在合并顺序中。旧 M2→M3 交接作为已完成历史保留，不与当前 M2 精确核心→M5 交接混淆。

## 7. 每个合并请求的检查项

- 对应哪个里程碑、步骤和阶段门？
- 实现标签及证据层级是什么？
- 输入、输出、party 角色和允许公开值是否变化？
- 是否改变预处理、代数表示、材料绑定或消费方式？
- 核心与端到端轮数如何计算？
- 是否包含 raw-score 和 mask 适配成本？
- 新增或修改了哪些测试，实际命令和结果是什么？
- 通信是否保留 total、per-party 和分方计数？
- 适用阶段是否附通信核验或性能矩阵报告？
- 未测指标与失败配置是否如实记录？
- 是否影响另一条协议线或公共接口？
- 文档状态是否与 main/分支实际状态一致？
- `VFSS-baseline/` 是否保持不变？
- 是否避免提交参考工程、论文、密钥、日志和构建物？
- 是否运行相关回归及 `git diff --check`？

文档 PR 不声称重新执行历史测试。新性能结果必须有自己的 revision、输入、环境、命令和原始计数。

## 8. 立即下一步

### 8.1 当前 Chase migration 任务

1. 评审 R0 redesign、cleanup 和 party-knowledge diff。
2. R1--R3 复用现有 OPV/ST/Beneš，不重写第二套。
3. R4/R5 建 clear oracle 和 standalone forward-shuffle tests。
4. P2-assisted preprocessing 先证明与 direct Chase views 的 distribution
   equivalence；P2-knows-both-permutations 路径保持 C-INSTANTIATION。
5. R6 保留 P2-full-`r` 四轮路径时，只能作为 functional C-baseline；另行
   评估 distributed `r` 与 blind/distributed GRank KeyGen，不假定可行。
6. 保留 legacy M2/M3、`VFSS-baseline/` 和 Protocol III DPF。

### 8.2 Future strict gate

允许准确标记的 NON-EXACT Chase migration。strict G1、官方 G2/G3 和 M5
runtime 保持 gated；任何 C-baseline 都不能解除这些门。

### 8.3 双方立即共同确认

- 本轮目标是 I 三轮核心、III 两轮核心。
- I 四轮工程基线和 III 三轮工程基线继续保留。
- M2、M5 均执行“实现→通信核验→交接”。
- AAV86 两种路线完成完整性能验收后再推进 BB90+DCF。
- 六种方案共用语义和全部指标。
- CipherGPT 实施和性能任务已取消。
- 分支进度不能直接写成 main 已完成。
- 尚未测量的数据继续标记 `NOT_MEASURED`。

## 9. 历史证据索引

本节保留旧计划中的实现基础和验收事实。它们不是本次修改文档重新运行得到的结果，也不构成精确核心已经完成的声明。

### 9.1 M2.7：CmpAgg 三进程基础

实现标签：

```text
m2_priority_cmpagg_three_process_e2e
```

P2 为公开 canonical graph 生成 node-mask shares 和分方 uCMP/DCF 材料，发送后退出。测试控制器在离线屏障后提供 priority-key shares；重构与 oracle 检查只在测试层执行。

历史测试覆盖 `n=1,2,5,7,11`、K 边界、重复值、非二次幂、正负边界及 package/transport 错误矩阵。

记录的 Ubuntu 干净 Debug 构建为 11/11 CTest 通过。见：

`docs/reproduction/M2_CMPAGG_PROCESS_E2E_UBUNTU_2026-09-05.md`

### 9.2 M2.8–M2.14：工程组件与端到端路径

| 阶段 | 已有实现或证据 |
| --- | --- |
| M2.8 | EMP chosen OT、connected-fd adapter 和独立进程 conformance |
| M2.9 | GGM OPV 与 Share Translation |
| M2.10 | 单遍 Permute+Share |
| M2.11 | 两遍前向/逆向 shuffle roundtrip |
| M2.12 | priority-key 输入、shuffle、CmpAgg、受控 shuffled rank 与 reverse carrier 的小规模 E2E |
| M2.13 | 六轮模块化 mask 路径、离线屏障、有界帧、layout 和 rank-permutation 审计 |
| M2.14 | raw-score carry/lift/sign 适配后的八轮路径 |

主要决策：

- `docs/decisions/M2_CHOSEN_OT_DEPENDENCY.md`
- `docs/decisions/M2_OPV_SHARE_TRANSLATION.md`
- `docs/decisions/M2_PERMUTE_SHARE.md`
- `docs/decisions/M2_SECRET_SHARED_SHUFFLE.md`
- `docs/decisions/M2_PROTOCOL_I_SMALL_E2E.md`

主要复现记录：

- `docs/reproduction/M2_PERMUTE_SHARE_UBUNTU_2026-09-06.md`
- `docs/reproduction/M2_SECRET_SHARED_SHUFFLE_UBUNTU_2026-09-06.md`
- `docs/reproduction/M2_PROTOCOL_I_SMALL_E2E_UBUNTU_2026-09-06.md`

这些组件是后续复用基础，不自动证明精确 shuffle 功能或自适应图预处理已经实现。

### 9.3 M2.15/M2.16：精确核心差距审计

M2.15 确认当时的 PS API 无法表达论文需要的同置换 public masked shuffled list，因此保留四轮核心、八轮总路径。

M2.16 进一步审计 `pi(x)+r`、掩码秘密性和 GRank 关联，未新增精确原语。文档随 `f800f96` 合入 main。

三轮核心、七轮总路径在这些记录中仍为未实现候选。相关文件：

- `docs/decisions/M2_PROTOCOL_I_PAPER_CORE_ALIGNMENT.md`
- `docs/decisions/M2_PROTOCOL_I_PAPER_EXACT_3ROUND_DESIGN.md`
- `docs/decisions/M2_PROTOCOL_I_EXACT_LEAKAGE_AUDIT.md`
- `docs/reproduction/M2_PROTOCOL_I_PAPER_CORE_ALIGNMENT_UBUNTU_2026-09-06.md`
- `docs/reproduction/M2_PROTOCOL_I_PAPER_EXACT_3ROUND_UBUNTU_2026-09-06.md`

本次重新推进该目标，不反写这些历史结论。

### 9.4 M2 验证可靠性关闭

chosen-OT readable-hangup 和 modular E2E FD-lifecycle 修复已合入，未改变协议图。

记录的 Ubuntu 24.04.4 fresh 构建在 soft `RLIMIT_NOFILE=1024` 下：

- EMP-ON：19/19；
- EMP-OFF：13/13；
- `(128,2/8)`、`(256,2/8)` 及重复矩阵通过；
- 相关完整矩阵还在 4096 下通过。

这说明对应工程验证不依赖将 FD limit 提高到 4096，不表示论文精确条件或网络性能已验收。

见：

- `docs/reproduction/M2_CHOSEN_OT_POLLHUP_UBUNTU_2026-09-06.md`
- `docs/reproduction/M2_MODULAR_E2E_FD_LIFECYCLE_UBUNTU_2026-09-06.md`

### 9.5 M3 关闭记录

M3 在 `main@bb0d0e8` 完成整改，保留三轮 priority-key 和五轮 raw-score 两个入口。

已完成 logical-n GRank、DPF routing、secure combine、raw-score 安全入口、独立进程测试、两个正式 Party executable 和 MetricsRecord 汇总。

正式 Party 生成各自 report，TEST_ONLY 控制器汇总完整 MetricsRecord；不能写成每个 Party executable 都独立生成全局结果。

完整证据及 11-test 验证口径见：

`docs/reproduction/M3_REVIEW_CLOSEOUT_UBUNTU_2026-09-10.md`

历史计时包括已披露的 Dealer 生命周期、输入分发和 report 收集边界；网络 bandwidth/RTT 及完整在线 PRG 计数仍按原记录保留未测状态。

后续正式实验新增分阶段计量，不静默替换历史数字或证据来源。
