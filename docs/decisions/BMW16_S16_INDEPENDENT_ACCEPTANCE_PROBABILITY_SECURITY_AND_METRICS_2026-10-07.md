# BMW16 S16：S15 独立接收、概率界复核、联合视图与计量门

日期：2026-10-07
身份：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL**
本决策对应的复跑源码：`a95d7cec8105da0c418a465d96f314830a873630`。本文档和 S16 报告在其后提交；报告 revision 不代表被测源码 revision。

## 1. 接收对象与范围

S16 从 S15 报告提交 `b8e9fc622403ece033466b15c479643bc6ecfb74` 建隔离 worktree，分支为 `codex/m6b-i-bmw16-s16`。起始工作树干净；起点 `main`/`origin/main` 为 `c3926c68fd14f270faa8b55234311071947fa080`。桌面主工作区、`VFSS-baseline/`、论文和参考工程、AAV86/E20/E21 及 PR #28 未改动。

S15 声明被测源码 revision 为 `2c6937116fbbc170e7a254f4449a3a6411b0cb70`，报告 revision 为 `b8e9fc…`。S16 先校验了 S15 外部归档，再从其声明 Git revision 重建相关目标；S16 的新增普通 DCF 计数测试夹具位于源 revision `a95d7cec…`。不得把 S16 新构建二进制说成 S15 历史二进制，也不得把本轮结果倒填为 S15 当时测量。

### S15 来源身份审计发现

S15 外部归档有 216 项，`sha256sum -c` 全部通过，归档 `SHA256SUMS.txt` 的 SHA-256 为 `be7089865be7068f2101fb47ad5abbab9527935b2967cd22c997c32cdfcde9ea`。但外部 `S15_SOURCE_AND_BINARY_HASHES.txt` 的 12 个仓库源文件行与 Git 有以下关系：5 个 Python 文件与声明 revision 的 Git blob 一致；7 个 C++ 文件不一致。逐项排除 CRLF 后，对每个不匹配哈希搜索可达 Git 历史，也均无匹配 revision。独立审计 JSON 为 `s15-source-identity-audit.json`，SHA-256=`bd3e7a3e59251741a865cb1f8a656d28c7377006ba5f0917f9fdc66b39817515`。

不匹配的七个路径为 `VFSS/CMakeLists.txt`、`VFSS/ext/FSS/dcf.cpp`、`VFSS/ext/FSS/include/FSS/dcf.h`、`VFSS/src/moe_topk/experimental_bmw16_material_bundle.cpp`、`VFSS/src/moe_topk/experimental_bmw16_select_party.cpp`、`VFSS/tests/moe_topk/experimental_bmw16_3proc_test.cpp`、`VFSS/tests/moe_topk/experimental_bmw16_party_test.cpp`。例如 CMake 清单 hash `5c37d1c5…` 对应 Git blob SHA-256 `da876d54…`；DCF 实现清单 `47f91a08…` 对应 Git blob `d7943df8…`；完整 64 位哈希逐路径保存在上述 JSON。

该发现意味着**S15 历史 C++ 二进制与其声明 Git 源码的绑定未核实**。它不能确定旧二进制实际由哪些字节构建，不能据此断言旧二进制错误。S15 原始归档完整性仍成立；S16 从声明 Git tree 重建并复跑得到的是一个新的、可追溯证据链。S15 历史 pilot 的文件 hash 和统计汇总可内部复算，但因源绑定缺口，不接收为被测源码已验证的性能数据。

## 2. S15 独立接收门

| 门 | S16 判定 | 依据与边界 |
|---|---|---|
| `S15_FUNCTIONAL_ACCEPTANCE` | `PASS_FOR_S16_GIT_REBUILD_DOMAIN`；历史 S15 binary/source link `UNVERIFIED` | S16 从 Git tree 重建相关目标，6/6 相关 CTest 通过；61 条独立进程矩阵经状态/退出码/oracle 审计，其中 57 SUCCESS，失败状态各 1 条；S4/C++ n=256 两条共同 tape 轨迹相同。此结论不追认哈希不匹配的历史 binary。 |
| `PROBABILITY_ARGUMENT_ACCEPTANCE` | `PASS_CONDITIONAL_IDEAL_UNIFORM_WITHOUT_REPLACEMENT` | 独立重算 S15 固定整数参数的保守尾界和可达偶数 `M` 域；无 BMW16 Theorem 8 外推。OS 熵源与 SHA-256 counter sampler 的计算假设单列，不能换写为实现的统计失败率。 |
| `METRIC_ACCEPTANCE` | `PARTIAL` | S16 复算 61 例普通 DCF Eval 扩展数，sampler PRF words 独立；没有覆盖系统库中所有潜在 AES/PRG 指令，也无跨方案同源/同边界测量。S15 pilot 摘要仅内部核验且不能源绑定。 |

S15 报告所列 24 个 pilot 原始 stdout 文件哈希全部匹配；4 组配置的 1 warmup + 5 measured 汇总可从逐次记录重算，20 个正式样本全为 SUCCESS。它只说明 S15 归档内部一致，不消除上述 7 个源码身份差异，也不是 V4 认可数据。

## 3. SUCCESS soundness 复核

设 Select 升序真实键为 `x₁<…<xₙ`，要求 Top-K 边界真实键下标 `r=n−K+1`。S4 的两份 median 扩张采用：

```text
N=n+1; L=max(N−2r,0); H=max(2r−N,0)
M=N+L+H=2·max(K,r); h=M/2
```

task 0 的顺序为 `L 个 low dummy, sentinel_low, x₁…xₙ, H 个 high dummy`；task 1 为 `L 个 low dummy, x₁…xₙ, sentinel_high, H 个 high dummy`。若两份 Partition 都精确返回各自前 `h` 个 Reject，则 `task0.Accept ∩ task1.Reject ∩ RealHandles={xᵣ}`。该等式覆盖 `K=1`、`K=n`、奇偶 `n` 与中位边界；`n=1` 另走已标明的 shortcut。

对每个当前 noiseless Partition，R1 样本内部的全对比较给出精确样本 rank；R2 的两 pivot bracket 若不成立即 abort，若成立则 `prefix`、`U`、`suffix` 连续且不交，`q=h−|prefix|`。`q=0` 是精确边界分支。`q>0` 时 R3 的 `C(V,2) ∪ V×(U\V)` 比较确定每个样本项在 U 的精确 rank，端点含样本 pivot 自身；虚拟 rank `0`/`|U|+1` 不对应元素。由 `i≤q<j` 生成 W 与 `qW`。R4 全比较 W 后，拒绝数为 `|prefix|+(i−1)+qW=h`，且 Reject/Accept 互斥覆盖输入。代码中的空集合、越界 rank、重复样本 rank、错误基数等 guard 触发都不是概率“错成功”，而是算法不变量错误/abort。

两哨兵的交集因而唯一对应真实边界项。项目稳定键唯一；membership 方向 `Rᵢ≥R_selected`，恰好包含优先级不低于第 K 阈值的 K 条真实记录。沿同一个隐藏置换 inverse-route 后，XOR 位份额重构等于冻结 oracle。此是针对当前 S4 项目派生实现、精确比较及正确 ABI 的数学 soundness 条件，不是论文 Algorithm 7 逐字一致，也不是无条件运行正确性证明。S16 有限 differential/E2E 是实现证据，不替代该证明。

## 4. 理想抽样失败界：重新计算和界限

只使用 S4/S15 当前整数规则：

```text
s      = min(M, ceil(sqrt(64M)))
qL/qH  = M/2, M/2+1                         if s=M
         max(1,floor((s+1)/2)-ceil(sqrt(16s))),
         min(s,floor((s+2)/2)+ceil(sqrt(16s))) otherwise
U_cap  = min(M, ceil(8M^(3/4)))
v      = min(u, ceil(sqrt(64u²/M)))
W_cap  = 2ceil(sqrt(4M))+1
```

当前 sampler 是无放回 partial Fisher–Yates。理想模型取独立均匀无放回样本；实际双方 OS CSPRNG 合币及 SHA-256 counter stream 仅提供计算假设。固定种子/测试随机带不用于正式 party。

单 Partition 的保守预算：

- bracket：若 `s=M` 则失败概率为 0；否则取样 rank 偏离中点至少 `4√s−1/2`，Hoeffding 给 `2 exp(−2(4√s−1/2)²/s) < 2e⁻³¹`（实际适用区间内 `s≥65`）。
- U cap：`M≤4096` 时 `U_cap=M`，不可能超限；否则设 `x=M^(1/4)>8`。超 cap 导致长度 `U_cap` 窗口的样本落点至多 `qH−qL+1≤24x+4<32x≤μ/2`，其超几何下尾由 Chernoff 以 `exp(−μ/8)≤exp(−8x)` 控制；至多 M 个窗口 union，得 `M exp(−8M^(1/4))`。
- W cap：`M≤64` 时 `v=u` 或 cap 覆盖，失败为 0。否则一个超长未采样 gap 至少 `4√M`；固定 gap 未命中的概率至多 `e⁻³²`，对至多 M 个 gap union 得 `M e⁻³²`。

令 `p_b=0` 若 `s=M`，否则 `2e⁻³¹`；`p_U=0` 若 `U_cap=M`，否则 `M e⁻⁸ᴹ¹/⁴`；`p_W=0` 若 `M≤64` 或 `W_cap≥M`，否则 `M e⁻³²`。两份 Select task 的总失败用不要求任务独立的 union bound：

```text
P_abort(n,K) ≤ 2(p_b+p_U+p_W),  M=2·max(K,n−K+1).
```

S16 脚本用整数平方根/四次根取整覆盖所有 `M=2,4,…,512`（n≤256 可达域），最大 `1.3105804606611235e−11`，发生在 M=512；对公式域 M≤2,000,000（对应 n≤10⁶ 的 K 全域）最大 `5.065679989546104e−8`，在 M=2,000,000。M>4096 的合并尾项导数单调检查更正了 S15 checker 注释：充分条件是 `(2x−1)e^(−8x)<e^(−32)`，不是 `<e^(−31)`；`x>8` 时左侧递减且不超过 `15e^-64<e^-32`，故端点最大值不变。

该界仍是**理想独立均匀无放回抽样数学界**，不是有限运行观测，也没有把 CSPRNG/PRF 区分优势与统计概率直接相加。当前 API 仍限制 n≤256；n≤10⁶ 结果只是公式审计，不是可运行性声明。没有自然 abort 的 61 例不能证明 `≤10⁻⁶` 的实现级统计保证。

## 5. 整池安全 hybrid 与第一个缺口

拟议理想泄露明确包括公开的 `n,K,session,width/padding`、shuffle 后 public masked list、两个 task 共用的匿名真实 handle、R1–R4 自适应匿名图和打开 comparison bits、pivot/U/V/W、已用/未用槽访问与流量、selected anonymous handle、abort 类别。每方输出仅为其原序 XOR mask share。P0/P1 的实际联合本地 view 还含本方 raw share、shuffle 本地置换/辅助材料及消息、全部九个真实 pair-key pool（两个 task 的八个 Select pool + membership pool，含未用 key）、相关 node-mask 阈值、Eval shares、逆路由状态、输出 share。项目所有者尚须接受公开匿名图/bits 的泄露草案；本结论不假设它已批准。

可审查的 hybrid 顺序为：H0 实际；H0→H1 将各线程 PRG stream 替换为独立随机 tape；H1→H2 将完整 DCF key/evaluation-share 池换为受泄露模拟的理想多 key transcript；H2→H3 模拟 Protocol I shuffle 的本地辅助信息、消息、public_z 与同一置换 inverse 输出 share；H3→H4 将两份 Select、membership、abort 组合为泄露的后处理。

H0→H1 只有在每线程 seed 均高熵、不回滚，且审计出每线程所有 DCF KeyGen 随机用量 `Q_tid` 时才可计优势 `Σ_tid Adv_PRG(Q_tid)`。现有 S16 online 计数专门覆盖普通 DCF `evalDCF` 内 `ecbEncTwoBlocks` 扩展，不能提供 KeyGen 的完整 `Q_tid`，也不是库级 PRG 全计数，因此这里只是条件项。

**第一个未闭合步是 H1→H2。** 需要对实际 `keyGenDCF/evalDCF` 给出一方的 auxiliary-input、相关阈值、多 key 安全定义/模拟器：输入应容许本方 shuffle 辅助 view；整池包括未用 key；九个池的阈值因同一 node mask 差而相关；公开 operands 和前序公开 bits 可自适应决定后续 Eval；每个槽的 Eval share 以及公开合成 bit均属于 view。若公开 bit 在目标泄露中，则“己方 Eval share + bit 可推出对方 share”是该泄露的后处理，并非额外声称隐藏；但 simulator 仍须模拟己方 Eval shares 与完整 correlated key pool 的联合分布。仓库代码只有 keygen/eval API，不含上述 aux-input/multi-key 定理。不能从单 key 隐私自动推导，也不能宣称数学上不可能。

随后 H2→H3 还缺实际 Protocol I shuffle (`sigma/tau/a/e/r_share`、交换消息、public_z 和 inverse 输出 share) 与上述自适应 transcript 的联合 simulator。AES-GCM 只能认证 bundle 字节/AAD，不证明 key 的数学阈值关系或整个 party-view 隐私。

**唯一关键的下一安全构造**：针对精确 DCF keygen 分布提出并外部审查上述 H1→H2 多 key aux-input adaptive-view 引理；证明须给出每次 hybrid 的随机性条件、对 correlated alpha 的处理、未用 key 的替换和优势总和。若无法证明，下一最小改动是把公开 comparison bits/自适应图移入有固定访问面的安全控制构造，并重新计算新增在线轮数、材料及泄露；不得悄悄启用。

## 6. 发材与一次性范围

数学上的理想私有认证通道可以作为条件性安全证明前提单独讨论；S15/S16 实际本地 AES-GCM bundle 使用 root 测试 harness 预置双方凭据，不构成生产凭据注册或远程 T→party 私有认证交付。AAD/标签校验提供字节与外层身份字段完整性，可信 T 的正确 keygen 才负责 key 内部数学关系。

当前 burn marker 在单一受保护 claim root 内持久化，先于任一 slot Eval；相同 store 的重复/并发领取和领取后崩溃重启拒绝。把 bundle 复制到另一空 claim root 后可再次 claim，故 `ONE_SHOT_SCOPE=PASS_SHARED_PROTECTED_LOCAL_ROOT_ONLY`、跨 store/迁移/备份/回滚抗重放 `NO-GO`。部署合同必须明确一个 bundle identity 只能进入一个受保护且不可回滚的 claim root，或实现外部共享持久 anti-replay ledger；目前两者都未达到部署级验证。不能用测试 root 同时能读两方目录这一点论证 party 隐私。

## 7. PRG/成本口径、计数器开销和容量

计数器在实际 `evalDCF` → `ecbEncTwoBlocks` 节点展开处统计调用；每次调用内部处理两个 AES blocks，所以 `AES_blocks_total=2×online_prg_calls_total`。该计数仅是普通 DCF online expansion，不等于全部线上 AES 指令或系统库 PRG。S16 独立审计 61 行完整路径、每方按 DCF 调用参数复算扩展公式，检查 `DCF Eval=2×uCMP party Eval`；sampler SHA-256 PRF words 分列，离线 DCF KeyGen、AES-GCM 与未覆盖系统库指令不计入该字段。S16 样本 n=64/K=8 的最大一次运行记录为 881,804 DCF 扩展调用、1,763,608 对应 AES blocks、656 sampler PRF words；此数字是当前计数定义下的实测，不是全库 PRG 总量。

同一源 commit `a95d7cec…` 构建计数 ON/OFF 两个 party-test binary，以 n=64/K=8、固定输入与测试专用采样 tape、交替顺序、各 1 warmup + 5 次 fresh-material 配对。两 build 的四轮边 hash、逻辑轮计数、uCMP/DCF Eval 数和 tape 相同。critical online median：ON 44,971 μs，OFF 35,059 μs，差 `+28.2723%`。这是 in-process socketpair、无采样币交换、TEST_ONLY 固定 entropy 的 instrumentation 诊断，不是 LAN/WAN 性能数据；未来正式性能构建应计数关闭，并以同口径单独审计计数。

checked 全池公式沿用 `slots/party=2p+2+9·C(n,2)`，包下界对真实配置含 key、manifest 与固定 overhead。关键点：n=256 为 294,274 slots、312,136,740 B/party 解析下界；n=1000 为 4,497,550 slots、4,987,338,528 B（约 4.99 GB）下界，超过 512 MiB cap 且 API 当前上限 n≤256，拒绝而未运行；n=10,000 为 542,225,036,988 B，n=100,000 为 57,464,659,444,756 B，n=1,000,000 为 6,070,495,802,256,900 B。均为 checked 解析下界，非包长/时间实测。E20 流式发材可作为磁盘/常驻 RAM 的工程思路，但不减少二次方 key 数与总字节，也必须另证随机访问、view 和 one-shot。

## 8. 门禁结论

| 门 | 结论 |
|---|---|
| `FUNCTIONAL` | `PASS_FOR_REBUILT_TESTED_DOMAIN`；S15 历史 binary/source linkage 未核实。 |
| `IDEAL_ABORT_BOUND` | `PASS_CONDITIONAL_IDEAL_UNIFORM_WITHOUT_REPLACEMENT`；CSPRNG/PRF 计算优势未量化。 |
| `CONDITIONAL_SECURITY` | `NO_GO`；首缺口为 correlated-threshold full-pool adaptive multi-key DCF aux-input simulator；shuffle/inverse 组合也未闭合。 |
| `PRIVATE_DELIVERY` | `NO_GO`；理想通道是单独证明前提，生产 party credential/T→party delivery 未实现。 |
| `ONE_SHOT_SCOPE` | `PASS_SHARED_PROTECTED_LOCAL_ROOT_ONLY`; 跨 store/rollback 防重放 `NO_GO`。 |
| `PERFORMANCE_COMPARABILITY` | `NO_GO / NOT_READY`；S15 pilot 源身份不接收，S16 仅 TEST_ONLY counter overhead；无正式 LAN/WAN、无同 revision 全对全/I+AAV86 配对矩阵。 |

方案继续保持 `PROJECT_DERIVED / EXPERIMENTAL`。不创建 secure alias，不运行正式 V4 五次 LAN/WAN 矩阵，不计入六方案性能排名。实施计划的 V3→V4 顺序保持有效；S16 用户授权下的有界复核和工具工作不表示 V3/V4 完成。
