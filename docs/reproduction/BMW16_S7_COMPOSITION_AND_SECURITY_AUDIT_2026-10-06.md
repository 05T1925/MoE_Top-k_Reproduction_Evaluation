# BMW16 S7：组合原型、独立复核与安全门报告

日期：2026-10-06
方案名：**Protocol I + BMW16-derived Select + DCF**
身份：PROJECT_DERIVED；代码身份：TEST_ONLY_CENTRAL_OPEN
结论：**Select 与 raw-score→原序 mask 功能组合实际运行；Party-view 安全 NO-GO；没有 secure 入口，没有正式性能矩阵。**

## 1. 范围、基线和保护

本任务从 S6 审计起点继续。隔离分支 codex/m6b-i-bmw16-s7 从 main@c3926c68fd14f270faa8b55234311071947fa080 建立；S7 起始工作树干净，起点 origin/main 与 main 一致。S4 04ed6f8a352ab2277434344094471dff82f83e4b、S5 67c3f302a8564b68616f7d7247c8cee3415ace31、S6 c4f34e7e8f9fd954637573fe62b8d9b567362298 为只读历史证据。S5/S6 不包含 S4 程序；S7 从 S4 git object 选择性复制 select4r.py 到 TEST_ONLY 目录，并保留其来源 SHA-256，不假定源码已在基线分支。

E20 参考 revision 8986a40175f5f8eef56d765fef0cfa863e205985，功能实现来源 a8f0ac47f9b3ebc4276d1aa0445f178acc1ef56f；S6 记录其独立接收为 detached checkout 8986a401...，接收报告 SHA-256 0BB83D44A058CF5282B59A16724D7EB288048CFFDAEFB8997D8EE090B1353CA0，状态 PASS_WITH_EXPLICIT_LIMITS。该接收只覆盖 E20 明确范围。本轮没有迁移 E20 材料 runtime；其流式发材、分方存储和一次性领取经验不证明多轮自适应 BMW 图的联合安全。

Desktop 主工作区分支为 feat/m6a-performance-evaluation，HEAD 与 main 相同；当时已有 PROJECT.md、docs/IMPLEMENTATION_PLAN.md、docs/PAPERS.sha256、docs/REFERENCE_MANIFEST.md 修改，以及两个 decisions 文档、M6A inventory、siamjdiscrmath.pdf 等未跟踪资料。S7 使用独立 worktree，起点干净，没有覆盖或带入这些差异。VFSS、VFSS-baseline、AAV86/Protocol I 旧入口、E20/E21、PR #28、参考树和论文均未修改。只更新 S7 分支的项目范围文档，不把历史 Protocol I 泄露审计或 BB90 结果改写。

| 来源 | 文件身份 | 页数 / SHA-256 | 用途 |
|---|---|---:|---|
| BMW16 2016 论文 | Papers/1603.04941v1.pdf（本地只读） | 44 / F46F83CBA279F37E9E3AAA0D64B145FDFCFBC52C0C9C8B79BACEC1259CA9CA23 | 主要算法来源；S4 采用步骤及保证边界沿用 S4/S5 原页报告，本轮不扩张论文定理 |
| 辅助材料 | Papers/017.pdf（本地只读） | 249 / 3EAE3A93F12AB71EEA8C040167F21351FCF3FF54DC423985A04087E3E90CB15 | 相关材料交叉核对，不作独立证明 |
| S4 sample-bracket Select | S4 git object；源 SHA-256 8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab | — | PROJECT_DERIVED 运行来源，不称为原样 Algorithm 7 |

仓库原 M6A→M6B 顺序计划是历史的正式集成/性能门。本次用户明确授权 BMW16-derived Protocol I 研究原型，不把这项授权误写为 M6A 完成、正式性能可开始或旧 BB90 资料/结论被替换。

## 2. S6 三项技术复核

### 2.1 n=2 可见端点泄露：成立范围收窄

**原始记录标签可见时，S6 反例成立。** P0 两项本地加法份额在两执行中都为 (0,0)；两个真实世界的严格顺序分别是 0<1 与 1<0。若 transcript 公开“原始记录 0 与 1 的比较位”，该位分别为 1 和 0。对应理想 Top-1 masks 分别 [0,1]、[1,0]。对任一理想 mask，P0 输出 XOR 份额在四个 2-bit 向量上均以 1/4 概率出现，两个世界边际相同；但公开比较位支撑集不相交，故即使条件于相同 P0 输入份额与输出份额仍存在可区分事件。完整结果见 BMW16_S7_VIEW_AND_NAMESPACE_AUDIT.json。

对 P1 可对称构造：固定 P1 两项份额为 (0,0)，让 P0 两项份额随两个世界分别为 [0,1] 与 [1,0]。P1 的 corrupted local shares 和理想输出份额分布相同，公开原记录端点比较位仍翻转。因而 original-ID transcript 泄露反例对腐化 P0、P1 都成立；条件只针对端点能关联回真实记录。

**端点仅为隐藏均匀句柄时，不能沿用上述论证。** 独立审计固定无向查询图，枚举全部真实严格顺序、每一个句柄置换及所有边子集：n=2 单边两种顺序的 transcript 都是 {0:1,1:1}；n=3 遍历 8 个边子集，n=4 遍历 64 个边子集，每组在各秘密顺序下 transcript 计数相同。这个有限枚举纠正了“公开比较位必然直接泄露真实顺序”的过宽外推。

另有主线 `VFSS/tests/moe_topk/protocol_i_parallel_shuffle_conformance_test.cpp` 的 tiny shuffle-view 枚举：131,072 个分配下，分别检查腐化 P0/P1 固定自身输入份额时，其本地 shuffle view 对对方份额变化的不变性；S7 在 WSL 构建并运行该测试，exit 0。该结果只涉及小环上的 shuffle 组件 marginal view，不覆盖 BMW 自适应比较、两实例材料相关性或最终 DCF/mask 输出联合模拟。

**未证明完整匿名自适应协议安全。** 固定图边际不含两方本地份额/置换/shuffle 消息、两个实例共有真实句柄关系、相关 FSS key 与访问/未访问槽、按比较结果自适应选图、概率 abort 和 mask 输出份额。S7 原型还会直接把这些量中的若干项交给一个集中控制器。结果只收窄反例范围，不支持 PARTY_VIEW_SECURITY=PASS。

### 2.2 uCMP 方向与实际位宽：S6 位宽下界修正

本地 VFSS/src/moe_topk/protocol_i_ucmp.cpp 的严格比较语义是 c=[x<y]，两方 output share 相加按 Z/(2^64) 得到 0/1。对互异真实稳定键，反向比较 share 可逐方作 (1-c0 mod 2^64, -c1 mod 2^64)；成员 x>=threshold 同样是 1-[x<threshold]，并正确包含相等。反向严格小于与成员谓词的边界不同，不能混用。

S6 取 b=max(1,ceil(log2 n))，stable real/dummy code 最大值小于 2^(33+b)，故纯数值编码需要 33+b 位。但当前 uCMP 差值校正以 h=2^(Bin−1) 为输入最高位，实测安全非负输入区间需 <h。真实 FSS 反例：n=2/K=1，Bin=34，比较 0<8589934593 数学为真，实际重构位为 0；Bin=35 重构为 1。故当前 comparator 的项目候选宽度须为 34+b：n=1→35、128→41、256→42、1000→44。n=10^6 需要 54 位，当前 34..53 位 API 不支持。证据见 BMW16_S7_UCMP_KEY_CONFORMANCE.json。

实际测试编码：Q(s)=(uint32(s) XOR 0x80000000)，R(s,i)=(Q(s)<<b)+(n−1−i)。R 递增按 signed score 升序、同分 original_index 降序；按高优先级解释，即 score 降序、original_index 升序。Rmax=(2^32−1)2^b+(n−1)。低 dummy code 为 j∈[0,L)，低 sentinel 为 L，真实项为 L+1+R，高 sentinel 为 L+Rmax+2，高 dummy 为 L+Rmax+3+j。该类别偏移在 TEST_ONLY 中保证所有虚拟项与真实复合键严格分离，且 dummy 不计作真实 mask 位。没有实现这些整数运算的秘密共享版本。

实际 key 方向按规范化端点生成。S7 TEST_ONLY 只在同一因果轮、同一任务内部为同一无序边共享一份比较；反向逻辑调用仍计入算法调用数，通过 share complement 取结果。round/task/instance/membership 不共享一次性材料；跨轮重复边会取新材料。这没有表示输入无关离线材料覆盖已实现。

### 2.3 全局 D 容量：S6 估算过宽，但密钥池仍很大

对单个活跃 Select 域 M=2max(K,n−K+1)；双实例共享真实身份、各自拥有 dummy 时，句柄并集 H=2M−n，S6 padding D=next_power_of_two(H) 是全局地址 envelope。它不是每个 Select 比较图的必要活跃域。按两实例和四轮全有向边，加上未知阈值 membership 离线池，保守计数为：

    Select comparator slots = 8 M (M-1)
    Membership slots        = n (n-1)
    Total ordered slots      = 8 M (M-1) + n (n-1)

这是所有有向对预发、无跨阶段缓存的槽上界模型；只有 reverse complement 的材料/访问合同证明后才可改成无向池。n=1000/K=80 时 M=1842、H=2684、D=4096：全局 D 模型 135,183,960 槽，活跃 M 模型 28,127,976 槽；正确 Bin=44，每逻辑槽 97+24Bin=1153 bytes 的模型给出每方 32,431,556,328 bytes 序列化下界估计。此估计未生成、未落盘、未计时，且远超仓库现有 1,000,000-edge package cap。n=128/K=2 活跃池 530,352；n=256/K=2 为 2,142,000；n=10^6 因 uCMP 不支持 Bin=54，不给解析字节值。逐配置与三种 handle 合同在独立容量 JSON 中。

| n=1000,K=80 材料合同 | ordered 全池模型 | 若 reverse complement/key 访问合同另获证明 | 两实例交集/限制 |
|---|---:|---:|---|
| S6 全局 D=4096 envelope | 135,183,960 槽 | 67,591,980 无向槽 | H=2684 先 pad 到 D；D 不等于必要活跃地址域 |
| 每实例 M=1842 活跃命名域 | 28,127,976 槽 | 14,063,988 无向槽 | 两个实例需共享真实记录 opaque handle 或另建秘密 join |
| 两个不相交 M 命名域 | 28,127,976 槽 | 14,063,988 无向槽 | 没有现成的真实记录相交，必须秘密 join |
| 共享真实 handle、dummy 按实例分址 | 28,127,976 槽 | 14,063,988 无向槽 | H=2684；公开相交可能泄露真实 cohort；图/访问隐私仍未证明 |

以上均为解析预发槽模型，不是实际生成、写盘或测量结果。无向数是条件性缩减；S7 只验证同轮 TEST_ONLY share complement 代数，尚未证明安全访问轨迹与一次性材料合同。

全局 H→D、两个不相交 M 命名域、共享真实句柄且 dummy 地址按实例隔离三种合同分别影响两哨兵交集、跨实例秘密 join、可能的真实 cohort 暴露和 key-ID 绑定。没有一种能仅靠容量代数证明 shuffle privacy。S7 未选择 secure namespace 合同；D=4096 是保守全局 envelope，不是必要的每 Select 池大小。

## 3. 实际 TEST_ONLY 组合及功能证据

目录 experiments/TEST_ONLY_BMW16_S7_COMPOSITION/ 包含 S4 明文 Select adapter、实际 VFSS uCMP/DCF + shuffle 的 C++ 测试服务、两份并行 Select 与 membership/original scatter 组合器、冻结 C++ Top-K oracle differential、独立边清单审计、uCMP 编码/方向测试和匿名视图/容量枚举器。S4 代码副本保留原身份标签。环境为单个 Python 控制器和单个 C++ 服务进程；控制器持有 clear score、自己拆分输入 shares、发起 keygen/eval，打开比较 output shares 和 shuffle identity。它不是 T/P0/P1 三独立进程，也没有在线静默 T 或输入无关离线发材。

成功矩阵 10/10：

| 样本 | n/K | 覆盖 | 结果 |
|---|---:|---|---|
| singleton_min_k1 | 1/1 | INT32_MIN、singleton TEST_ONLY identity | oracle PASS |
| n2_extremes_k1/k2 | 2/1、2/2 | 两端 K、INT32 极值 | oracle PASS |
| n3_all_equal_k2 | 3/2 | 全同分、奇数规模 | oracle PASS |
| n5_mixed_k1/kn | 5/1、5/5 | K 两端、非二次幂、正负值 | oracle PASS |
| n7_repeated_k3 | 7/3 | 重复分数、稳定 index | oracle PASS |
| n8_mixed_k3 | 8/3 | 双哨兵交集、重复、INT32 边界 | oracle PASS |
| n8_all_equal_k4 | 8/4 | 全同分、中间 K | oracle PASS |
| n16_boundary_k8 | 16/8 | 较大 TEST_ONLY 值域、边界/重复 | oracle PASS |

所有输入与种子都保存在 JSON。该小规模下采样参数取全样本/残余全覆盖，故 10/10 只证明这些样本的程序行为，不是随机失败概率证据。初始位宽错误矩阵 BMW16_S7_INITIAL_WIDTH_FAILURE_MATRIX.json 全量保留：8/10 项以 COMPLETED_WRONG 结束且 NO_MASK，另 2 项成功；这不是论文随机失败路径，而是旧 33+b 位宽越过 uCMP 输入可用半区间的实现错误。修正为 34+b 后才得到上表 10/10。没有筛选失败种子或静默重试。

代表性实际组合输入为 [-2147483648,2147483647,0,-1,0,2147483647,-2147483648,7]，n=8/K=3，input seed=11、Select seed=42、FSS comparator seed=99、shuffle seed字段=77。实际 shuffle primitive 内用 OS getrandom(2)，shuffle_request_seed 只是报告标签，因此 shuffle 不是按该字段可复现。测试输出 selected original index 7；XOR shares P0=[0,0,0,1,0,1,1,1]、P1=[0,1,0,1,0,0,1,0]，异或得到 [0,1,0,0,0,1,0,1]，与冻结 C++ oracle 完全一致且恰 3 个 1。selected handle、shuffle 映射和候选 mask 在此诊断运行中公开。

独立边审计对 trace 中每一轮冻结 edge list 重新散列/计数，并检查轮计划只声明依赖前一轮结果：四个 Select comparison barrier 为 PASS。该 n=8 运行：

| 轮 | 逻辑调用 | 真实-真实 | dummy 相关 | 本轮重复 | 前轮重复 | 实际 uCMP 槽 | 双方 DCF Eval |
|---|---:|---:|---:|---:|---:|---:|---:|
| R1 | 132 | 56 | 76 | 28 | 0 | 132 | 528 |
| R2 | 44 | 28 | 16 | 10 | 44 | 42 | 168 |
| R3 | 2 | 2 | 0 | 0 | 2 | 2 | 8 |
| R4 | 2 | 2 | 0 | 0 | 2 | 2 | 8 |
| 合计 Select | 180 | 88 | 92 | 38 | 48 | 178 | 712 |

membership 另有 8 次逻辑查询、8 槽、32 次双边 DCF Eval。shuffle primitive 另生成 28 个 unused CmpAgg key slots、双方合计 key wire bytes 60,984；它们不是 Select/membership DCF Eval，也未被使用。在线/离线计时、secure bytes、PRG、abort 率、端到端轮数均 NOT_MEASURED。

### 候选安全接口（设计合同，尚未实现）

| 阶段 | 应有输入/输出 | 必须保持的性质 | S7 状态 |
|---|---|---|---|
| 输入 | P0/P1 各持有 signed Q20.12 score 的 Z/(2^32) 加法份额 | 不重构 score；原始 index 与记录绑定 | TEST_ONLY 控制器当前先见明文再拆 shares |
| stable key | 记录的秘密 score、秘密 original_index → R_i shares | Q(s)=(uint32(s) XOR 0x80000000)，R=(Q<<b)+(n−1−i)；测试宽度 34+b | 只做明文编码并用真实 uCMP conformance；无 secure lift/encoding |
| 共同置换与 Select | 同一秘密置换下的真实记录 + 实例私有 sentinel/dummy → 双哨兵 Select | 两实例交集绑定同一真实 handle；样本/pivot/U/V/W/边/材料 ID 的公开性须由理想泄露或模拟证明 | 集中式 shuffle 打开 handle；图和比较位直接可见 |
| K-th 绑定 | 两 Select 交集的唯一 record → selected stable-key shares | 不公开 selected original_index/handle/key；绑定到后续 DCF | 控制器持有明文 selected index/key |
| membership | 每条原记录 R_i shares 和 selected-key shares → 1-bit shares | 谓词按高优先级编码为 R_i≥R_selected，成功恰 K 位 | 真实 uCMP/DCF 调用，但阈值由控制器明文构造 |
| 逆路由/输出 | shuffled membership shares → original-order XOR mask shares | 复用同一秘密置换；不得打开 K-th、mask 或 index | 控制器用已打开 shuffle map scatter；TEST_ONLY oracle 后验核验 |
| abort | 随机失败/材料/通信错误 → 双方一致 ABORT(reason_class)，无 mask | 不重试、不降级、不返回全零 mask | 小程序只抑制失败 mask；没有分方 abort 协议 |

因此功能路径在 harness 中可跑通，但每个需要隐藏或绑定的值仍未满足正式 secure API 合同。

## 4. 泄露、因果图与 secure 阻塞

当前实际 TEST_ONLY 路径如下。此图描述 harness 执行，不是安全协议轮数图：

    测试控制器明文输入
      → 控制器随机拆分分数 shares
      → 集中调用仓库 shuffle 两方状态机（内部两轮；记录身份映射）
      → Select R1 生成整批边、调用实际 uCMP/DCF、打开比较位
      → R2 / R3 / R4 在前轮结果返回后冻结边，再调用比较器
      → 双哨兵两实例交集选 record，控制器知晓 stable key
      → 逐记录真实 uCMP/DCF membership（R_i ≥ R_selected）
      → 控制器按已打开 shuffle map scatter 得原序 shares
      → TEST_ONLY oracle 校验

未来安全入口尚需分别计数 raw-score→稳定键转换与 FSS 输入绑定、秘密 shuffle、四层 Select 自适应比较、selected-key DCF membership、同置换秘密逆路由、XOR mask 输出转换和 ABORT 一致协调。总在线轮数、材料、每方通信现为 NOT_DERIVED / NOT_MEASURED；不能写成四轮。

具体 secure blockers：

1. 没有联合模拟/混合证明，覆盖腐化方 local shares、shuffle state/message、共同真实 handle、两实例 transcript、adaptive graph、相关 FSS key 和读取/未读槽、abort、selected key 关联及输出 shares。
2. 没有对自适应且输入相关边的输入无关离线 key-pool 覆盖和隐私访问方案；现有全对有向池容量模型明显超过材料包限额，S6 的 D 全局池更大。不能用在线 Dealer 解决。
3. 同轮 reverse-share complement 的局部等式已验证，但跨轮/跨任务 key 不复用；没有兼容一次性领取、私有图访问和完整容量的方级材料 ID/协议。
4. 两份 Select 需要同一真实记录的隐藏身份绑定与两哨兵交集；dummy 命名、真实句柄关联与公开 cohort 泄露尚未闭合。
5. selected stable key 到每条记录 DCF key 的阈值绑定、成员 DCF 的输入无关离线参数化、shuffle 逆路由与 XOR 输出都没有 secure composition。
6. 实际 comparator 最大支持 53 位，n=10^6 当前稳定编码要 54 位；需扩展真实原语、包装、序列化及 conformance，或另证不碰撞的压缩编码。
7. ABORT 类别、双方一致中止/no-mask 合约和概率目标 ≤10^-6 的独立论证未实现。测试中 0 abort 不构成上界。

抽样概率口径也未迁移到密码学实现：当前 Select 使用 Python random.Random 的固定 seed；这用于可复跑 TEST_ONLY，不是密码学随机数。shuffle 内部使用 OS getrandom，但其 shuffle seed 字段只是标签。S7 没有生产 Secure PRG/抽样器，因此没有可与理想随机抽样失败概率相加的计算安全优势界，也没有证明总 ABORT≤10^-6。

因此未创建 secure API 或 VFSS 文件；下一阶段入口是先关闭第 1、2、4 项联合安全/材料覆盖引理，再做分方 DCF 绑定和逆路由。若视图证明给出匿名比较 transcript 必须隐藏的结论，需指定消息/隐藏图机制及其在线成本，不能靠边际均匀性替代证明。

## 5. 运行环境、命令和哈希

环境：Windows worktree；Python 3.13.7、NumPy 2.3.5；WSL Ubuntu 24.04.4 LTS、CMake 3.28.3、GCC 13.3.0；C++ 使用仓库 CMake/VFSS 目标；编译产物在 /tmp/m6b_i_bmw16_s7_build/，未进入 Git。

构建 TEST_ONLY C++ adapter：

    wsl -d Ubuntu-24.04 -- bash -lc "cmake -S '/mnt/c/Users/28641/.codex/worktrees/m6b-i-bmw16-s7/MoE_Top-k_Reproduction_Evaluation/experiments/TEST_ONLY_BMW16_S7_COMPOSITION' -B /tmp/m6b_i_bmw16_s7_build -DCMAKE_BUILD_TYPE=Release && cmake --build /tmp/m6b_i_bmw16_s7_build --target bmw16_s7_fss_testonly -j2"

矩阵和实际组合命令：

    python experiments/TEST_ONLY_BMW16_S7_COMPOSITION/run_validation.py --fss-backend wsl:/tmp/m6b_i_bmw16_s7_build/bmw16_s7_fss_testonly --output docs/reproduction/evidence/BMW16_S7_TEST_ONLY_MATRIX.json
    python experiments/TEST_ONLY_BMW16_S7_COMPOSITION/compose_testonly.py --scores-json experiments/TEST_ONLY_BMW16_S7_COMPOSITION/vectors/mixed_extremes_n8_k3.json --k 3 --input-seed 11 --algorithm-seed 42 --shuffle-seed 77 --fss-seed 99 --fss-backend wsl:/tmp/m6b_i_bmw16_s7_build/bmw16_s7_fss_testonly --trace --output docs/reproduction/evidence/BMW16_S7_E2E_N8_K3_SEED42.json
    python experiments/TEST_ONLY_BMW16_S7_COMPOSITION/audit_edges.py docs/reproduction/evidence/BMW16_S7_E2E_N8_K3_SEED42.json --output docs/reproduction/evidence/BMW16_S7_EDGE_AUDIT.json
    python experiments/TEST_ONLY_BMW16_S7_COMPOSITION/key_ucmp_conformance.py --fss-backend wsl:/tmp/m6b_i_bmw16_s7_build/bmw16_s7_fss_testonly --output docs/reproduction/evidence/BMW16_S7_UCMP_KEY_CONFORMANCE.json
    python experiments/TEST_ONLY_BMW16_S7_COMPOSITION/audit_s7.py --output docs/reproduction/evidence/BMW16_S7_VIEW_AND_NAMESPACE_AUDIT.json
    wsl -d Ubuntu-24.04 -- bash -lc "cmake --build /tmp/m6b_i_bmw16_s7_build --target moe_topk_m2_parallel_shuffle_conformance_test -j2 && /tmp/m6b_i_bmw16_s7_build/vfss/moe_topk_m2_parallel_shuffle_conformance_test"

本轮实际运行：最终矩阵 10/10 oracle PASS；保留的位宽修正前矩阵为 8/10 COMPLETED_WRONG、NO_MASK，2/10 SUCCESS；前述 8 条不能解释成算法论文允许的概率失败。key conformance 10 组、89 次实际 uCMP/DCF 比较、356 次双边 DCF Eval、86 次 reverse-share 转换；代表性 E2E oracle PASS；独立 edge audit PASS；匿名/容量审计完成。旧 mainline moe_topk_m2_parallel_shuffle_conformance_test 在 WSL 构建后运行成功（静默 exit 0），这只补充 shuffle 组件的有限测试证据。

TEST_ONLY C++ binary SHA-256：20c9751b0a85fd3ede6a4570ce3fb24273b7f23391bc2503899da729bae2d8f3。PDF、源码及结果哈希清单见 docs/reproduction/evidence/BMW16_S7_SHA256SUMS.txt。源码 revision 随本地提交固定；不推送/合并，不更新 PR。

## 6. 门禁结论

| 门 | 判定 | 依据/限制 |
|---|---|---|
| FUNCTIONAL_COMPOSITION | PASS_TEST_ONLY_ONLY | 10 个固定小样本成功；两个 Select→阈值→DCF membership→原序 XOR mask 的集中 harness 实跑；C++ frozen oracle 一致 |
| PARTY_VIEW_SECURITY | NO_GO / UNPROVEN | 有 original-ID 泄露反例；匿名固定图枚举只证明受限边际结果；完整联合模拟、私有访问和输出绑定欠缺 |
| SMALL_N_SECURE_RUNTIME | NOT_IMPLEMENTED | 不满足安全门；无 VFSS secure 新入口、无 T/P0/P1 独立进程运行 |
| FULL_PERFORMANCE | NOT_RUN | 无 LAN/WAN；计时、材料字节、通信、PRG、abort 和全端到端在线轮数均 NOT_MEASURED |

## 7. 修改范围

S7 新增 TEST_ONLY 源码、证据和本报告/决策，并最小更新项目范围、实施顺序注记、V4 状态、参考来源及论文哈希登记。没有改生产协议源代码。最终 git diff --check、禁止路径检查和提交 revision 由交付回复汇报。
