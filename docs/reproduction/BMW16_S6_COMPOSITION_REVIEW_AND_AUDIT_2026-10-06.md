# M6B-I-BMW16-S6：组合审计、TEST_ONLY 验证与交付记录

日期：2026-10-06
结果标签：`BMW16_DERIVED_SELECT_PROTOCOL_I_COMPOSITION_REVIEW_TEST_ONLY`
结论：数学键/计数审计 `PASS_WITH_EXPLICIT_LIMITS`；Protocol I 安全组合 `NO-GO_FOR_SECURE_IMPLEMENTATION`。
运行结论：本阶段**没有运行 S4 Select 程序或任何 secure Select**；运行了独立 Python 审计器和独立 C++ frozen-oracle key conformance。

## 1. 基线与文件身份

S6 worktree 为 `C:\Users\28641\.codex\worktrees\m6b-i-bmw16-s6\MoE_Top-k_Reproduction_Evaluation`，分支 `codex/m6b-i-bmw16-s6`，起始 HEAD `c3926c68fd14f270faa8b55234311071947fa080`。该值等于当时读取的 `origin/main`，并通过在线 `git ls-remote origin refs/heads/main` 核对。S6 worktree 在实现前干净。

桌面主工作区当前 branch `feat/m6a-performance-evaluation`，HEAD 同为 `c3926c68...`，存在预先已有 `PROJECT.md`、`docs/IMPLEMENTATION_PLAN.md`、`docs/PAPERS.sha256`、`docs/REFERENCE_MANIFEST.md` 修改，4 份未跟踪 M6A 文档与 `siamjdiscrmath.pdf`。S6 未复制、覆盖或修改桌面 worktree；本次改动只在独立 worktree。

| 来源 | 起点/身份核验 | 证据边界 |
|---|---|---|
| S4 | commit `04ed6f8a352ab2277434344094471dff82f83e4b`，worktree `codex/bmw16-s4` 干净；非 main 祖先 | `select4r.py` SHA-256 `8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab`。只读，未搬入 S6 |
| S5 | commit `67c3f302a8564b68616f7d7247c8cee3415ace31`，worktree `codex/m6b-i-bmw16-s5` 干净；非 main 祖先 | S5 已接收 S4 明文算法但给 secure composition `NO-GO`。S6 保留 S5 历史与 S1/S4 反例，不改写结论 |
| E20 | commit `8986a40175f5f8eef56d765fef0cfa863e205985`；最后 secure-code-changing commit `a8f0ac47f9b3ebc4276d1aa0445f178acc1ef56f` | 只审计离线输入无关发材、分方固定偏移存储、一次领取的可复用设计点；未复制 E20 代码或原始数据 |
| E20 receiver | detached checkout `8986a401...`，独立接收报告 SHA-256 `0BB83D44A058CF5282B59A16724D7EB288048CFFDAEFB8997D8EE090B1353CA0` | `PASS_WITH_EXPLICIT_LIMITS` 不证明 BMW16 自适应多 key 联合模拟；不据此过 S6 隐私门 |
| E21 | `93d96f6c9f9d5c758e5471ea38158f05f1c7dea4` | 未独立接收；不借用代码、性能或原始证据 |

主论文身份：`Papers/1603.04941v1.pdf`，44 页，SHA-256 `F46F83CBA279F37E9E3AAA0D64B145FDFCFBC52C0C8B79BACEC1259CA9CA23`；辅助核对 `Papers/017.pdf`，249 页，SHA-256 `3EAE3A93F12AB71EEA8AC040167F21351FCF3FF54DC423985A04087E3E90CB15`。页数由 Poppler `pdfinfo` 读取，hash 从桌面主工作区只读 PDF 计算，并与 S4/S5 来源记录比对。文件不在新 worktree 的 Git checkout，也未加入 Git；哈希行已登记到 `docs/PAPERS.sha256`。`017.pdf` 只作复述交叉核对，不当作独立证明。

S4/S5 的论文定位仍为：noiseless comparison model PDF p.7；Algorithm 5 PDF p.29 与证明 pp.28–29；Algorithm 7 PDF p.34；Theorem 8 pp.11、33–34；Appendix A Lemma 1 p.16。S6 的算法实现对象是 S4 `PROJECT_DERIVED sample-bracket Select`，并未声称逐字实现 Algorithm 5/7，也不使用 BMW16 Theorem 8 概率或比较常数。

## 2. S6 实际收紧的组合规则

### 2.1 两哨兵与 record handle

真实元素排序低到高为 `(signed_score ASC, original_index DESC)`，令 `r=n-K+1`、`M=2 max(r,K)`、`h=M/2`、`L=h-r`、`H=h-K`。两实例分别添加一个 low/high sentinel；各自含 `L` 个低 dummy、n 条真实记录、`H` 个高 dummy，且真实 `r` 位于 low-sentinel 实例的 `h+1` rank、high-sentinel 实例的 `h` rank。成功时 `Accept_low ∩ Reject_high` 恰为一条真实记录，即 stable Top-K 边界；S4 将非真实 dummy 过滤，集合基数不为 1 时不冒充成功。

S4 源码复用同一 `real_ids`，但为两个实例分别创建 pad 和 sentinel ID。于是两实例句柄并集为 `2M-n`，S6 capacity conservatively uses `D=next_power_of_two(2M-n)`；D 的多余地址只是 storage/material 覆盖域，不增加算法输入项。可用相同 pad ID 把句柄并集合并到 M+1，但这是 PROJECT_DERIVED 布局变化，本报告仅作对照，不用于主容量。

### 2.2 stable score key 与 DCF 边界方向

定义 `b=max(1,ceil(log2 n))`、`o=(raw_score_bits XOR 0x80000000)`，实数反向键 `R=(o<<b)+(n−1−original_index)`。其与主线 priority key 满足 `R=Rmax−P`，故它无碰撞地实现 S4 排序逆序；分数两端 INT32 极值无需特殊哨兵分数。再对低 dummy/sentinel/真实/高 sentinel/高 dummy 加区间偏移，最大数值小于 `2^(33+b)`，不依赖额外类别 tag。`Bin=33+b` 与 uCMP 34..53 兼容至 n=10^6；n=1 时 `b=1,Bin=34`。具体公式及 overflow inequality 见决策文档和独立脚本。

最终真实 membership 应用 `R_i≥R_selected`。低到高第 `r=n−K+1` 项作阈值，故恰 K 个真实元素满足该式；其等价形式是主线 priority key 的 `P_i≤P_selected`。`original_index` 是原始行号，必须随 secret record carry，不能换成 shuffle 后 slot。此处只完成数学表示和 TEST_ONLY conformance；raw share 的 sign-lift、加偏移、比较和 key 绑定均未进 secure runtime。

## 3. 边、Eval、槽位和最坏比较量

S4 两个 partition 实例共用 R1–R4 四个因果比较层。单实例上界 `R1≤42M,R2≤2M,R4≤25M`；当 `M≤4096` 时 `R3≤2048M`，更大 M 时 S4 `U` cap/V 规则给 `R3≤657M`。两实例逐轮相加，所以小 M 层预算为 `(84M,4M,4096M,50M)`，总比较调用 ≤`4234M≤8468n`；大 M 层预算为 `(84M,4M,1314M,50M)`，合计 ≤`1452M≤2904n`。这证明的是 S4 项目构造的线性明文逻辑调用上界，不是 BB90 常数。S6 不执行这些比较，所有实际比较/real-real/dummy/重复边数均标 `NOT_RUN`。

当前 `ProtocolIUcmpPartyMaterial::eval_strict_lt` 对一条 active logical edge 在每一方内部调用底层 `evalDCF` 两次。因此若 secure Select 用该封装，e 条逻辑边对应 P0 2e、P1 2e、双方便 4e DCF Eval；key slot 不等于 Eval 次数。R1/R2/R3/R4 及两个 instance 的 one-shot 材料必须带不同 stage/round/instance 域；同 pair 的跨轮重复仍要单独预处理/计数。S6 S4 运行边清单、实际相同轮重复、跨轮重复和 real/dummy分类均未产生。

离线 all-pairs 覆盖需先区分 ordered request 与无序关系。S4 R2 对每个 pivot 发出 `item→pivot`，若两个 pivot 都在 sample 中，pivot pair 会产生两个相反方向的逻辑调用。当前比较材料没有证明可从一个方向的共享结果局部导出反方向，也没有可复用 one-shot key 的缓存合同。因此不复用方向时，单个实例、单轮完整有序池为 `D(D−1)`；两实例四轮加 real membership 的有序保守全池为 `8*D*(D−1)+n*(n−1)`。S5 报告的 `8*C(M,2)+C(n,2)` 是 canonical 无序容量估计，S6 将其保留为条件优化，不称为方向安全保守数；S5 公式对应的方向安全修正为 `8*M*(M−1)+n*(n−1)`。`C(n,2)` 或 `n(n−1)` 是潜在比较地址覆盖，不等于实际 DCF Eval 数。隐藏 selected-star 访问仍需要 oblivious key retrieval/MUX；全池执行再 secret-mux 则二次在线比较。固定样本 R1 有序池 `s(s−1)`，R2 有序并集 `s(D−1)`；canonical R2 无序并集 `D*s−C(s+1,2)`。两实例 R3/R4 各按 full D ordered pool 预留。独立小域枚举 466 个 sample subsets 和 2,815 个 pivot-pair sets 验证了有序及无序集合公式。固定样本池仍是条件容量，不是已证明的安全地址方案。

每方字节用 S5 lower-bound 模型 `97+24Bin` 逐 logical material slot：当前 uCMP party key 序列化 `57+24Bin`，另计 16B endpoint mask share 和 24B edge envelope。未含 AEAD/header/index、盘块/缓存、T 临时峰值、生成速度和网络 framing。E20 的 `24Bin+24` 格式不相同，未用作换算。本阶段无真实 key generation、写盘或流式扫描，因此字节字段是解析预检而非实测。

关键 capacity 结果（详细 17 配置见 JSON）：

| 配置 | S5 报告无序 slots（条件） | S5 修正有序 slots | S6 D 全池有序 slots | S6 D 无序 slots（条件） | Bin S5→S6 |
|---|---:|---:|---:|---:|---:|
| n128 K2 | 265,176 | 530,352 | 2,109,312 | 1,054,656 | 43→40 |
| n128 K8 | 241,416 | 482,832 | 2,109,312 | 1,054,656 | 43→40 |
| n256 K2 | 1,071,000 | 2,142,000 | 8,445,696 | 4,222,848 | 44→41 |
| n256 K8 | 1,022,664 | 2,045,328 | 8,445,696 | 4,222,848 | 44→41 |
| n1000 K80 | 14,063,988 | 28,127,976 | 135,183,960 | 67,591,980 | 46→43 |
| n10^4 K1 | 1,649,915,000 | 3,299,830,000 | 8,689,662,448 | 4,344,831,224 | 50→47 |
| n10^5 K1 | 164,999,150,000 | 329,998,300,000 | 2,209,018,961,248 | 1,104,509,480,624 | 53→50 |
| n10^6 K1 | 16,499,991,500,000 | 32,999,983,000,000 | 141,737,453,800,896 | 70,868,726,900,448 | 56→53 |

按 `97+24Bin` 序列化 accounting 得到的每方字节下界如下。第一列容量是 S5 报告的无序估值；该列只有在反向共享比较能安全局部取补时才可作为单份材料。S5 有序列为本次修正后的方向安全基准，S6 有序列按 D 覆盖域；S6 固定样本列仍依赖固定样本地址不泄露的证明。所有值均是解析下界，实际 keygen/落盘未测。

| 配置 | S5 报告无序下界（条件） | S5 有序修正下界 | S6 D 全池有序下界 | S6 固定样本有序下界（条件） |
|---|---:|---:|---:|---:|
| n128 K2 | 299,383,704 B | 598,767,408 B | 2,229,542,784 B | 1,296,000,384 B |
| n128 K8 | 272,558,664 B | 545,117,328 B | 2,229,542,784 B | 1,291,161,438 B |
| n256 K2 | 1,234,863,000 B | 2,469,726,000 B | 9,129,797,376 B | 5,070,942,894 B |
| n256 K8 | 1,179,131,592 B | 2,358,263,184 B | 9,129,797,376 B | 5,064,967,126 B |
| n1000 K80 | 16,890,849,588 B | 33,781,699,176 B | 152,622,690,840 B | 80,322,506,296 B |
| n10^4 K1 | 2,139,939,755,000 B | 4,279,879,510,000 B | 10,644,836,498,800 B | 5,477,674,837,600 B |
| n10^5 K1 | 225,883,836,350,000 B | 451,767,672,700,000 B | 2,865,097,593,273,856 B | 1,443,933,012,554,176 B |
| n10^6 K1 | NOT_SUPPORTED (S5 Bin56) | NOT_SUPPORTED (S5 Bin56) | 194,038,574,253,426,624 B | 97,834,066,887,556,224 B |

S6 的容量结果不意味着这些槽已生成。主表按 direction-safe ordered slots 计：例如 official n=1000/K=80、D=4096 时，单实例单轮 `D(D−1)=16,773,120` 个有向槽，超过当前 mainline Protocol I package validation limit 1,000,000；原先提及的 `C(D,2)=8,386,560` 只是一个方向或经证明可复用后的无序关系数。n=10^6 时 D=4,194,304 还超过现有 Protocol I padded-domain API limit 1,048,576。E20 streaming 能改变传输/落盘形态，但不闭合活动地址隐私或联合模拟。

## 4. 安全组合门：确切泄露与尚缺接口

目标模型为离线可信非合谋 T、在线两方半诚实 P0/P1、T 在线静默。需要 T 在输入未知时为每个 party、stage、round、Select instance、ordered endpoint pair、comparator domain 和 material ID 预生成 one-shot materials。S6 没有把此模型视为已证明的 E20 继承性质。

公开 R1 sample handle、R1 compare bit、R2 pivot、U/V/W、active edge、材料 ID/文件偏移或 selected handle 会产生输入顺序相关 transcript。特别是公开比较位的最小反例：n=2,K=1，P0 两次执行的原始加法份额都相同（全零），P1 的份额分别重构 `[0,1]` 和 `[1,0]`；P0 本地输入/局部视图相同，公开严格比较结果翻转。该 transcript 无法只由 P0 的输入视图和隐藏 selected mask share 模拟。若边 endpoint 又可按 P0 本地 shuffle permutation 或材料 slot 对回源记录，泄露更直接。

要隐藏自适应图，需要 secret bit 上的 pivot selection、U/V/W 构造、定长 edge scheduler、oblivious gather/compact 与 key retrieval。读取所有 pair 可隐藏哪些边活跃，但无方向复用证明时会在每轮/实例运行 `D(D−1)` 个有序比较调用（无序关系数 `C(D,2)`），破坏线性 active online comparison；没有被允许的隐式全对全 fallback。E20 的 fixed-offset storage 让合法动态 pair 地址可能反映在 OS/store trace，单独成功接收 E20 的流式发材不构成 S6 联合视图 simulator。

完整安全证据必须对腐化 P0 与 P1 分别模拟 input shares/local permutation/inverse、shuffle 消息、两实例 shared real-handle linkage、T key shares/pool ID、未读取/读取 offset、比较 transcript、abort reason/time 和 output shares。当前只证明裸 key 数值和 pool 覆盖，没有上述模拟/混合归约。DCF binder 还需把秘密选出的 R key 和逐 record key 关联；若公开 selected index，直接泄露第 K 项。逆路由必须复用同一秘密 permutation；成功时 membership shares 重构为 oracle mask，abort 时两方一致结束且无任何 mask，不能回传全零。

## 5. 消息 DAG、概率与计量

| 因果顺序 | 工作项 | 已知/未定轮数 |
|---:|---|---|
| offline 0 | T 预生成输入无关的 Select/DCF/shuffle/inverse one-shot materials、分发后静默 | 实际 keygen、bytes、磁盘、PRG `NOT_MEASURED` |
| 1 | raw score share sign-lift + original-index-bound R key | 当前窄适配器存在，但该端到端接口轮数未由 S6 推导 |
| 2 | common hidden shuffle | 现有 forward share shuffle 为 2 rounds；shared two-instance handle composition未证 |
| 3–6 | Select R1→R2→R3→R4 | **4 个比较依赖层**；每层的消息/secret scheduler未实现 |
| 7 | two-sentinel one-hot intersection and threshold selection | 安全 mux/routing轮数未知 |
| 8 | threshold-to-real DCF membership | 星形逻辑边 n−1；隐藏地址/在线轮数未知；all-pair 扫描是二次 |
| 9 | inverse shuffle | 现有 reverse share shuffle为2 rounds；组合未证 |
| 10 | arithmetic bit share→XOR LSB shares + agreed abort/output | LSB转换可在规定加法域内本地做；双方 abort握手未知 |

因此 `select_comparison_layers=4`，而 `secure_raw_score_to_original_order_mask_online_rounds=NOT_DERIVED`。不能把四层称为 Protocol I secure entry 的完整四轮，也不把已存在的 forward/reverse rounds 简单相加冒充总轮数。

S4 对官方最大扩张 `M=1842` 的 ideal independent uniform sampling failure bound 为 `<4.50140648×10^-7`，只描述 PROJECT_DERIVED TEST_ONLY Select 的理想随机带。它不是 BMW16 Theorem 8。`10^-6` 工程 abort 预算最多只留下约 `5.49859352×10^-7` 给所有 PRG、shuffle 和抽样偏差，但这些分布误差没有定理/参数，不能假设统计独立后直接加成。S4 Python RNG 仅为可复跑测试；没有 secure sampling 或 CryptoPRG 的 S6 调用。理想正确组件成功时 `Pr[wrong|SUCCESS]=0` 是条件合同，不是当前 runtime 保证。

所有实际离线/在线时间、通信、实际 DCF Eval、PRG、abort rate、Select边计数均为 `NOT_MEASURED`/`NOT_RUN`。本阶段无正式 LAN/WAN 矩阵。

## 6. 运行、结果和哈希

环境：Windows 10.0.26200（PowerShell）、CPython 3.13.7、MinGW-w64 `g++ 8.1.0`。复跑命令（S6 worktree 根目录）：

```powershell
python -B experiments/TEST_ONLY_BMW16_S6_COMPOSITION_REVIEW/audit_capacity_and_keys.py `
  --output docs/reproduction/evidence/BMW16_S6_KEY_CAPACITY_AUDIT.json
g++ -std=c++17 -O2 -I VFSS/include `
  experiments/TEST_ONLY_BMW16_S6_COMPOSITION_REVIEW/oracle_key_conformance.cpp `
  -o C:/Users/28641/.codex/worktrees/m6b-i-bmw16-s6/bmw16_s6_oracle_key_conformance.exe
C:/Users/28641/.codex/worktrees/m6b-i-bmw16-s6/bmw16_s6_oracle_key_conformance.exe
```

原始终端结果：

```tex
label=BMW16_DERIVED_SELECT_PROTOCOL_I_COMPOSITION_REVIEW_TEST_ONLY
status=PASS_ARITHMETIC_AND_COVERAGE_CHECKS; NO_SECURE_COMPOSITION_CLAIM
key_cases={'exhaustive_ternary_vectors_n_le_6': 1092,
           'exhaustive_boundary_vectors_n_le_4': 780,
           'topk_kth_cases_checked': 8975}
coverage={'sample_subsets_exhausted': 466,
          'possible_pivot_pair_sets_exhausted': 2815}
configs=17
BMW16_S6_TEST_ONLY_ORACLE_KEY_CONFORMANCE PASS vectors=2787 kth_cases=19950
```

Python checker枚举 score alphabet `{-1,0,1}` 的 n≤6 所有输入，及 `{INT32_MIN,-1,0,1,INT32_MAX}` 的 n≤4 所有输入；遍历每个合法 K、反向 priority 关系、two-sentinel code strictness、stable Top-K threshold。另检查全相等、重复边界、K=1/n。C++ checker直接包括冻结 `VFSS/include/moe_topk/topk_oracle.h` 与 `protocol_i_priority_key.h`，以 INT32 位型、全相等和重复值验证 `(R_i≥R_selected)` 与 frozen `top_k_mask` 同一。

没有输入依赖 pivot 的 S4 Select运行，没有“成功率”样本可报告，没有失败 seed 可增删；历史 S4 报告保留其成功和失败记录。S6 验证也未生成任何真实 FSS key 或开网络进程。审计 JSON 记录 17 配置、checked capacity、每 stage package limit、S4 比较调用 upper bounds，以及所有 NOT_MEASURED 字段。

源码与证据哈希：

| 文件/对象 | SHA-256 |
|---|---|
| `audit_capacity_and_keys.py` | `B595A7E08EA5A1A574D18A8AB6A2D3E8FFCD0A4882E508126F0F4F1CEE2ED63E` |
| `oracle_key_conformance.cpp` | `30E1B1FC5EC1F4818B31EF878E6A3B884D9173FE9511B42BD05A84EB2A5F048F` |
| `BMW16_S6_KEY_CAPACITY_AUDIT.json`（最终运行） | `1A0468C4610295713397645B554B87935D4AFD05100081A893D4277F47FD7E9B` |
| C++ 临时测试二进制（运行后删除，不入 Git） | `604F9864C46FBBE8761A4D8F3D0A952597DF56DB1D2F26695CABA1DFF3934694` |

以上源码与 JSON hash 对应本报告所列的最终审计运行。临时二进制已用于 oracle conformance，记录 hash 后删除，不作为仓库 build artifact。

## 7. 文件清单与门禁

### 新增

- `experiments/TEST_ONLY_BMW16_S6_COMPOSITION_REVIEW/audit_capacity_and_keys.py`：独立键宽、双哨兵编码、R2 possible-pivot coverage、S4 linear-bound与材料容量核算器；不导入 S4/S5 实现，不接 secure runtime。
- `experiments/TEST_ONLY_BMW16_S6_COMPOSITION_REVIEW/oracle_key_conformance.cpp`：把 R 阈值方向直接对照仓库冻结 C++ oracle。
- `docs/decisions/BMW16_S6_PROTOCOL_I_SELECT_COMPOSITION_DECISION_2026-10-06.md`：规格/边界/安全组合 decision。
- `docs/reproduction/evidence/BMW16_S6_KEY_CAPACITY_AUDIT.json`：确定性机器结果。
- 本报告。

### 修改

- `PROJECT.md`：六方案 Protocol I 升级候选由 BB90+DCF 改为 PROJECT_DERIVED BMW16-derived Select+DCF，记录当前 NO-GO 和历史标签。
- `docs/IMPLEMENTATION_PLAN.md`：M6B 当前目标更新为 I BMW16-derived 组合门 + III BB90 路线；保留正式 V4 的 M6A 前置条件。
- `docs/BENCHMARK_VALIDATION_PLAN.md`：V4候选名、身份/门禁与未实现状态同步。
- `docs/PAPERS.sha256`：登记两份只读 PDF 的 SHA；PDF 本身未跟踪。

### 七项状态

| 检查项 | 判定 |
|---|---|
| S4 来源身份/PROJECT_DERIVED 边界 | `PASS_WITH_EXPLICIT_LIMITS` |
| 稳定键、dummy 类别及 uCMP 位宽算术 | `PASS_ARITHMETIC_AND_FROZEN_ORACLE_CONFORMANCE` |
| 离线 pair-pool 公式覆盖 | `PASS_SMALL_EXHAUSTIVE_SET_ENUMERATION; SECURE ACCESS UNPROVEN` |
| Select 四层线性逻辑比较界 | `S4_BOUND_REDERIVED; ACTUAL S6 SELECT COUNT NOT_RUN` |
| common shuffle + adaptive hidden graph + joint view | `NO-GO / UNPROVEN` |
| Protocol I + Select + DCF secure入口 | `NO-GO_FOR_SECURE_IMPLEMENTATION` |
| 原序 mask correctness/security and full online rounds | `NOT_IMPLEMENTED / NOT_DERIVED` |

## 8. 校验与未更改范围

提交前运行 `git diff --check`；逐项核对差异不包含 `VFSS/`、`VFSS-baseline/`、PDF、密钥、S4/S5/E20/E21 源码/参考树、本地参考工程、构建物或原始大日志。C++ exe 在 worktree 外的 S6 临时目录，记录 hash 后删除。提交在 `codex/m6b-i-bmw16-s6` 的独立本地 revision；不推送、不合并 main、不更新 PR #28。
