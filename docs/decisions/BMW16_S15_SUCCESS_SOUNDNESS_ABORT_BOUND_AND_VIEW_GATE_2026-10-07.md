# BMW16 S15：SUCCESS soundness、抽样中止界、整池视图与 n=256 门

日期：2026-10-07
身份：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL**

## 1. 基线与本轮身份

S15 从 S14 报告提交 `1c92ca6cefcfdfdb1e04912ca1a3ec573c58a269` 创建独立 worktree/分支；S14 被测源码提交是 `998726bca161b08cac140d7430c17f5b1005d914`。起始 `main`/`origin/main` 为 `c3926c68fd14f270faa8b55234311071947fa080`。S15 没有覆盖桌面主工作区、AAV86、E20/E21、PR #28、`VFSS-baseline/`、论文或参考树。

S4 Python 源码固定 SHA-256：`8644bf71bf8143a1a24f2a8e3b705306f1797bf2634f7cbd14c7ce9743fdfbab`。S14 的源码冻结表、64 项外部证据归档、独立重建、n=128 三份共同随机 tape 差分和 59 行独立 T/P0/P1 结果见本决策后附的 [S15 外部证据索引](../reproduction/evidence/BMW16_S15_EXTERNAL_EVIDENCE_INDEX_2026-10-07.md)。外部清单中的 64 项 S14 文件逐个 SHA-256 校验通过。S14 自报结果和 S15 独立复跑分别记录；后者没有改写 S14 历史提交。

S15 被测实现源码提交：`2c6937116fbbc170e7a254f4449a3a6411b0cb70`；独立验证/本决策文档随后单独提交。启用 DCF 计数器的 Release 二进制与原始输出 hash 见 S15 外部证据索引。

本轮复用既有 C++ 全链入口，不改写 Select。把实验输入硬上限从 128 提到 256；加入默认关闭的 FSS DCF 调用计数选项；增加 checked 容量/理想抽样概率计算、独立三进程日志审计器和 n=256 共同 tape/loopback 工具。计数器关闭时不执行 DCF 热路径 atomic increment。

## 2. S14 的独立接收与一次性语义边界

| 门 | S15 独立判定 | 证据边界 |
|---|---|---|
| `S14_FUNCTIONAL_ACCEPTANCE` | PASS（所复核的 S14 配置） | 从 S14 源独立 Release 重建；6 个 CTest 通过；n=128/K=2、8、80 的 Python S4/C++ 每层共同 tape trace 相同；S14 59 行 T/P0/P1 E2E 重跑通过，成功输出与冻结 oracle 一致。历史与本轮原件哈希见证据索引。 |
| `ONE_SHOT_ACCEPTANCE` | PASS，仅限共用受保护 claim-store | bundle burn marker 使用 `O_EXCL|O_NOFOLLOW`、写 marker、`fdatasync(marker)`、`fsync(directory)`；party runner 在调用 raw-score→mask API 前先读包、解封并持久 `claim_bundle`。同一受保护 claim root 的并发/重复领取、fork/崩溃后重启拒绝由 S14 测试覆盖。S15 新增复制同一 bundle 到另一空 claim root 后领取成功的明确反例，因此不能声称抗跨目录复制、回滚或独立 store 重放。 |
| `METRIC_ACCEPTANCE` | S14 原计量有边界；S15 补齐 DCF 调用点 | S14 的 4,032 个未使用 shuffle CmpAgg 槽已在 S14 的 shuffle-only 发材中移除；S15 核验 n=256 `omitted_cmpagg=65,280`、`unused_cmpagg=0`。S14 没有 DCF 内部 PRG 调用计数；S15 新增 opt-in 计数及独立恒等式复算。 |

本地 claim store 的精确合同是：同一受保护目录对一个 `session/party/manifest` 只能成功 burn 一次，marker 在任何 slot Eval 前持久落盘；之后崩溃、fork、重启或同 store 并发领取不得再次运行该 bundle。store 目录被整体复制或换到另一空 store 时，旧 marker 不随新的受保护账本存在；S15 复现了这个边界。跨机/部署级防复制需要一个 parties 共用、抗回滚的持久消费账本或等价硬件/远端服务，当前没有实现。测试控制器以 root 管理两方 fixture；不能把 root 的文件视图算入 P0/P1 的隐私证明。

## 3. SUCCESS soundness：成立的精确条件和证明

结论只针对 S4/S14 当前项目衍生规则、严格全序和正确的 noiseless 比较结果。它不是 BMW16 Algorithm 7 的逐字实现，也不调用 BMW16 Theorem 8。随机抽样可以令算法显式 abort；若 Partition 返回 `SUCCESS`，它不能因“抽样失败”而返回错分区。

### 3.1 严格顺序与两个哨兵

项目的优先级是 signed Q20.12 score 降序、同分 `original_index` 升序。Protocol I raw-score adapter 产生有界严格优先键；S4 Select 使用其反序键，所以 Select 的升序是“低分在前，同分较大 original index 在前”。该反序键是优先键在 `b=33+index_bits` 位环中的精确公开界减法，真实优先键范围小于 `2^(32+index_bits)`，不会回绕；score 与 original index 共同确定唯一真实键。最低/最高 dummy 使用独立公开类别，位于全部真实类之外，不会碰撞合法 `INT32_MIN/MAX`。

记真实记录按 Select 升序为 `x_1<…<x_n`，要求 Top-K 的边界下标为 `r=n-K+1`。令

```text
N=n+1
L=max(N-2r,0)
H=max(2r-N,0)
M=N+L+H=2·max(K,n-K+1)
h=M/2=r+L
```

两个 median 输入共用同一批真实匿名句柄：

- task 0 顺序为 `L 个 low dummy < sentinel_low < x_1…x_n < H 个 high dummy`。在前 h 项 Reject 中有 L 个 low dummy、low sentinel 和 `r−1` 个真实项，故 task0.Accept 中的首个真实项是 `x_r`。
- task 1 顺序为 `L 个 low dummy < x_1…x_n < sentinel_high < H 个 high dummy`。前 h 项 Reject 中有 L 个 low dummy 和 `r` 个真实项，故 task1.Reject 中最后一个真实项是 `x_r`。
- 交集 `task0.Accept ∩ task1.Reject ∩ RealHandles` 恰为 `{x_r}`。S4 实现还要求 `candidate.size()==1`，否则状态是 `ABORT_ALGORITHM_INVALID`，不能输出成功答案。

公式覆盖边界：`K=1` 时 `r=n`、`H=n−1`、`L=0`、`h=n`；`K=n` 时 `r=1`、`L=n−1`、`H=0`、`h=n`。当 `K<n/2` 时 `r` 侧的 high padding 补平 median；当 `K>n/2` 时 low padding 补平；奇偶 n 与 `K=n/2` 统一由 `N=n+1` 的公式取整，没有 parity 特例。`n=1,K=1` 是明确 shortcut，Party 直接输出 XOR shares `(1,0)`；它不运行 Select、uCMP 或 DCF，也不计入一般 Select 样本。

### 3.2 单个 median Partition 的成功证明

设一次 Partition 的输入是 M 个严格全序项，M 偶数，目标 Reject 数 `h=M/2`。对比结果准确时：

1. R1 对无放回样本 S 的全部无序对比较。每个样本项的 pairwise rank 因而精确且互异；若代码发现 rank 不唯一或 qL/qH 不合法，会标内部无效状态，不做排序补救。
2. R2 把所有输入（除 pivot 自身）分别与 R1 两 pivot 比较。记 `c=#items<x`、`cy=#items<y`。R2 只在 `c≤h≤cy` 时继续，并构造互不重叠的 `prefix={item<x}`、`suffix={item>y}` 和连续区间 `U=[x,y]`。于是 `|U|=cy−c+1≥2`，残余目标 `q=h−c` 满足 `0≤q<|U|`。`q=0` 直接输出 Reject=prefix、Accept=U∪suffix，基数分别 h 和 M−h。
3. 对 `q>0`，R3 无放回抽 V。边集合是 `C(V,2) ∪ (V×(U\V))`，故 V 每项相对于 U 的精确 1-based rank 都可从本轮前已冻结的比较结果还原。取 rank `i≤q` 的最近左 pivot x、rank `j>q` 的最近右 pivot y；缺侧用 0 或 `u+1` 虚拟边界。`W` 是二者之间的连续窗口，`qW=q−(i−1)`（i=0 时 qW=q），由 `i≤q<j` 得 `1≤qW≤|W|`。
4. R4 比较 W 内所有无序对。每项在 W 内的 rank 因此精确。Reject 为 prefix、低于 x 的 U 项及 W 中前 qW 项；Accept 为 suffix、高于 y 的 U 项及 W 的其余项。两者不交且覆盖全部 M 项，并且 `|Reject|=c+(i−1)+qW=h`、`|Accept|=M−h`。代码再核验 rank 集、基数、并集和互斥性；未通过即 invalid abort，不报告错 `SUCCESS`。

端点归属因此明确：R2 的 `q=0` 是精确 endpoint shortcut；R3/R4 的 x、y 自身包含在 W；虚拟边界不算输入元素、不计 Reject/Accept 的真实位数；U 空、q 越界、W 空、重复 rank 在上述有效前提下不可达，若运行时 guard 触发属算法不变量错误，不归为概率 abort。

### 3.3 membership、逆路由和原序 mask

选中的是 Select 升序第 r 项，即恰好按优先级位于 Top-K 边界的真实记录。membership 计算 `R_i ≥ R_selected`；自比较本地取 1，其他记录用 canonical pair 对应的严格比较份额及正确方向补数。因稳定真实键严格唯一，满足条件者恰为最高优先级 K 条记录。membership 位 shares 经同一前向 shuffle 的逆置换恢复原输入顺序；在 `Z/(2^b)` share 上取低位形成 XOR bit shares 后，双方异或重构每位等于 membership 位。因此只要 API `SUCCESS`，输出长度 n、bit-domain、weight K，并逐位等于冻结 stable Top-K oracle。

上述是数学 soundness 条件证明；S15 的 C++ 三进程 E2E 另在有限样本检查它，测试不能取代此证明。特别是 FSS/uCMP 返回错比较、错误参数/位宽、输入适配损坏不属于抽样“概率失败”；这些应分别成为 conformance/实现错误，不能用 ABORT_ALGORITHM_PROBABILITY 掩盖。

## 4. S4/S14 实际参数的理想抽样 abort 上界

本节只分析项目代码参数和理想均匀无放回抽样，不继承 BMW16 定理。每个 median 长度 M 使用：

```text
s      = min(M, ceil(sqrt(64M)))
qL/qH  = M/2, M/2+1                         if s=M
         max(1,floor((s+1)/2)-ceil(sqrt(16s))),
         min(s,floor((s+2)/2)+ceil(sqrt(16s))) otherwise
U_cap  = min(M, ceil(8 M^(3/4)))
v      = min(u, ceil(sqrt(64u²/M)))
W_cap  = 2 ceil(sqrt(4M)) + 1
```

所有 ceil 均对整数不等式作精确取整；S14 源把 epsilon 标记为不使用。抽样为无放回 partial Fisher–Yates；C++ 正常路径把 P0/P1 各 32-byte `getrandom` 贡献交换后 XOR，随后用 SHA-256 counter PRF words 和 rejection-bounded sampler；task/R1/R3 各有域。两方可见双方贡献及其 XOR，故这些采样决定属于公开 transcript。理想概率计算假定这些 words 是独立均匀随机；实际实现另依赖 OS CSPRNG 与 SHA-256 counter-stream PRF，计算优势未与统计概率相加。

对单次 Partition，严格 rank 和精确比较下：

- `s=M` 时 R1 bracket 不会失败；否则 M≥66、s≥65。计入 qL/qH 的整数取整后，两个超几何尾的偏差至少 `4√s−1/2`。运行域 M≤2,000,000 时，Hoeffding 指数 `2(4√s−1/2)^2/s` 大于 31，因此保守取 `p_bracket(M)≤2e^−31`。这避免把连续近似的端点误写成精确 `e^-32`。
- `M≤4096` 时 U_cap=M；否则令 x=M^(1/4)>8。若 |U| 超 cap，存在长度 c=U_cap 的窗口只含至多 `qH−qL+1` 个样本。该超几何均值至少 `64x`；a=ceil(4√s)≤4√s+1 且 s≤9√M，故 `qH−qL+1≤24x+4<32x≤μ/2`。Chernoff 下尾不超过 `exp(−μ/8)≤exp(−8M^(1/4))`，至多 M 个窗口 union 得 `p_U(M)≤M·exp(−8M^(1/4))`。
- `M≤64` 时 v=u，W 无概率超 cap；否则 `W>W_cap` 会在两个 V pivot 之间包含至少 `2ceil(2√M)≥4√M` 个未采样 V 的连续项。每个固定 gap 未命中概率不大于 `e^−32`，至多 M 个位置 union 得 `p_W(M)≤M·e^−32`。

一次 Partition 的上界为三项之和；两个 Select task 的 abort 用 union bound，不依赖两个任务的随机事件独立：

```text
P_abort(n,K) ≤ 2·(p_bracket(M)+p_U(M)+p_W(M))
M = 2·max(K,n−K+1)
```

S15 `ideal_abort_bound.py` 以整数 sqrt/fourth-root 取整复算，并对 M 的分段单调式检查端点。测试域 n≤256 的最大列为 `1.3105804606611235e−11`（M=512）；同参数形式延伸到 n≤10^6、所有 K 的保守最大列为 `5.065679989546104e−8`（M≤2,000,000），低于 10^-6 的研究目标。这是理想均匀无放回抽样界，不是本地 CSPRNG 的统计保证。运行时仍拒绝 n>256，不能声称 1000 或 10^6 路径已实现/测过。此界不包括比较器故障、熵源/PRF 计算优势、AEAD/通信错误或实现 bug；没有把它们伪装成算法概率。

## 5. 比较量与容量

两实例 median 长度 `M=2max(K,n−K+1)≤2n`。单 task：若 s=M（M≤64），R1 边数 `C(M,2)≤32M`；若 M>64，则 `s≤8√M+1`，故 `C(s,2)≤34M`。R2 不超过 `2(M−1)`；R3 为 `C(v,2)+v(u−v)≤vu=O(M)`，因 `v≤ceil(8u/√M)` 且 `u≤ceil(8M^(3/4))` 对大 M 线性，小 M 是有界范围；R4 不超过 `C(W_cap,2)=O(M)`。小 M≤4096 的 U cap 可为 M，但 M 的范围固定，仍可纳入渐近常数。两 task 合并仍为 `O(n)` 逻辑比较；任一概率 abort 均不重试、不排序、不追加比较。

离线池是全 pair 预发，成本仍是二次方。当前公式：`slots/party=2p+2+9C(n,2)`，其中 `p=next_power_of_two(max(n,2))`，9 项为两 task 的 8 个 Select pair pool 与 1 个 membership pool；shuffle-only 发材不含两份未 Eval 的 CmpAgg pool。每个 uCMP key 对每方一次 Eval 调用两次 DCF，故 `dcf_eval=2·ucmp_party_eval`。

n=256、index_bits=8、comparison_bits=41、C=32,640：

| 字段 | n=256/K=2 与 K=8 |
|---|---:|
| slots / party | 294,274 |
| DCF keygen calls / party-pair | 294,272 = 2p+9C |
| keygen node expansions / party-pair | 24,123,136 = 2·(2p·34+9C·41) |
| checked 包下界 / party | 312,136,740 B |
| 独立进程实测密文 bundle / party | 314,544,457 B |
| bundle 文件 cap | 536,870,912 B |
| T peak RSS | 约 1,970,160 KiB |
| party peak RSS | 约 1,284,096 KiB |

容量下界不是包实测；T/party RSS 是本机单次三进程实测，不外推部署机器。n=1000 的下界约 4.99 GB/party，n=10^6 约 6.07 PB/party；当前 API 限 n≤256，且包 cap 512 MiB，以上规模为 `PRECHECK_REJECTED/API_CAP`，无真实 bundle、时间或性能值。

## 6. DCF 调用计量的定义与检查

新增 `MOE_TOPK_ENABLE_FSS_DCF_PRG_COUNTERS` CMake 选项，默认 OFF。开启时，以 atomic counters 记录普通 DCF 路径的：`keyGenDCF` call 数、每个 KeyGen `ecbEncFourBlocks` 节点扩展调用数、`evalDCF` call 数、`traverseOneDCF` `ecbEncTwoBlocks` 节点扩展调用数。它们是 AES 扩展调用**次数**，不是 AES block 数，也不是 FSS 全库所有 PRG/AES 调用；shuffle 其它 PRG、cryptoTools 其它 AES 仍 `NOT_MEASURED`。`sampler_prf_words` 独立报告，不与 DCF 扩展计数相加。

T 在 bundle 生成前 reset 计数器，结束后采集；party 在调用完整 party API 前 reset，返回/异常均保存可用部分计数。计数器是进程级 atomic，不由阶段或线程清零；验证器在新 T/P0/P1 进程中执行，阻止跨运行混计。小型 `Bin=7` conformance fixture 核对 1 次 KeyGen、14 个 KeyGen node expansions、2 次 Eval、14 个 Eval node expansions。完整 E2E 审计器逐 case 复算 key 数、KeyGen 扩展次数、DCF Eval=`2×uCMP`、party 无 KeyGen、T 无 Eval，以及双方 send/recv 字节守恒；当 CMake 选项关闭时计数器返回 disabled/0 且审计器不把零当成实测值。

## 7. 联合单方视图：条件性 reduction 与第一个缺口

### 7.1 view 与理想泄露草案

对腐化 P0 或 P1，完整本地 view 至少包括：本方正式 raw score 加法份额及 raw adapter 中间 share；本方 forward/inverse shuffle `sigma/tau/a/e/r_share`、本地消息和输出 share；自己解封 bundle 全部材料（已用/未用、两 task 共用真实 handle 对上的 9 份真实 pair key、slot manifest/claim store/访问次序）；本方 DCF Eval share；打开的 `public_z`、采样 seed/样本、pivot、U/V/W、R1–R4 匿名图/边/比较 bit、选中匿名 handle、abort 与流量/相位；成功时自己的原序 XOR mask share。T 是可信非合谋且在线静默，离线只见 n/K/session/width 等公开配置，但可保留其所有材料状态。

待项目所有者确认的 leakage 草案是公开 `n,K,session,parameter widths/padding`、shuffle masked list `public_z`、两实例共用匿名真实 handle 的自适应比较图和打开 bits、算法控制状态/abort 类别、selected anonymous handle、材料槽访问/流量，以及双方各自输出 share。真实 original_index、score、rank、阈值和完整 mask 不应因此额外公开。TEST_ONLY harness/root 不属于此 view。如果目标隐私不允许打开匿名 pair 的 comparison bit，现有入口本身就不满足该泄露草案，不能靠模拟器重命名解决。

### 7.2 可审查 hybrid 尝试

- **H0 实际执行。** S14/S15 的 `ProtocolIUcmpMaterial` 以 `alpha=(mask_left−mask_right) mod 2^b` 进入 `keyGenDCF`；同一 forward shuffle 节点 mask 差对每个真实 pair 在两 task 的八个 Select pool与 membership pool各生成一次 key，9 份阈值相同但 KeyGen 随机流不同。party 解封全池，即使后续算法没查询的 key 也在 view 中。
- **H0→H1 PRG stream。** `seed_fss_once` 用 OS `getrandom` 给 `FSSConfig::prngs[tid]` seeds；`keyGenDCF` 通过 thread-local PRNG 顺序取 seed blocks，再在每层 AES 扩展。若这些流满足相应 AES/PRG 安全、所有 seed 有足够熵且不回滚，可把被消费的 stream 输出替换为独立均匀 blocks，优势至多 `Σ_tid Adv_PRG(Q_tid)`。这里 Q_tid 必须含每线程所有实际 PRNG 用量；S15 计数只覆盖 DCF AES 扩展点和 DCF 调用，不是 Q_tid 的完整测量。因此这是明确的条件步骤，不是已核验的数值安全界。S15 没有把每槽 domain separation 当作证明替代。
- **H1→H2 整池 DCF。** 若额外假设一个 `auxiliary-input, correlated-threshold, multi-key one-party DCF privacy` 定理：对完整阈值向量、与其联合生成且独立于 KeyGen coins 的 shuffle 辅助信息、全池含未用 key、公开相关 operands、每槽至多一次 adaptive Eval 输出 share 与已打开 bit leakage，整个本方 view 可由 leakage 模拟；并且单 key 版本可在该 aux-input 下对每次独立 KeyGen hybrid 使用，则 q=2p+9C 次混合损失至多 `q·ε_aux`（再加 H0→H1 PRG 项）。这一假设需从仓库 DCF 实现或其引用证明核实。仓库目前只有 `keyGenDCF`/`evalDCF` 的算法代码和单次 key API，没有该 aux-input/multi-key game、模拟器或优势定理；而阈值在 pair 间共享 node mask 差、9 池重复，公开查询依赖同一 masked list。**第一个未闭合 hybrid 是 H1→H2**。单 key privacy 即便成立，未注明 aux-input 条件也不能自动用于上述相关全池/公开自适应视图。S15 没有区分见证证明它不可能。
- **H2→H3 shuffle/transcript。** 固定全泄露 transcript 后，后续边、pivot、U/V/W、槽访问和 selected handle 是此前公开 bits、输入无关随机 coins 的后处理；在 transcript 被允许泄露且 H1→H2 simulator 成立时，该控制本身可继续模拟。但实际 Protocol I shuffle 需模拟本地方 `sigma/tau/a/e/r_share`、消息、`public_z`、含同置换 inverse 的输出 shares；只证 `tau0=pi∘sigma1^-1` / `tau1=pi∘sigma0^-1` 的局部置换边际不够。完整 shuffle-view hybrid 当前也 `NOT_PROVED`。
- **H3→H4 组合。** 若 key pool、shuffle、输出 share simulator 均成立，按先后组合两 task、membership、inverse、算法 abort，并在 peer fatal error 时只承诺存活方本地 abort；现代码只保证正常 algorithm status 在最终 frame 达成一致。材料/通信失败仍可能是 `LOCAL_ONLY`，不能标双方 AGREED_ABORT。

这定义了一个可审核的充分假设和优势形状，但 S15 没有声称当前仓库满足它。生产 T 身份注册、私有认证通道/远程 delivery 和跨机持久 anti-replay 也独立未完成；本机 AES-GCM bundle 与 UID 测试不解决这些部署条件。

## 8. 分项门禁

| 门 | 状态 | 理由 |
|---|---|---|
| `S14_FUNCTIONAL_ACCEPTANCE` | PASS（S14 被测域） | 独立 rebuild、CTest、S4/C++ n128 trace、59 行三进程；历史/本轮证据分栏。 |
| `ONE_SHOT_ACCEPTANCE` | PASS_WITH_LOCAL_STORE_SCOPE | burn-before-Eval；同 store 持久防重放；新空 store 接受已复制 bundle 的反例限制全局保证。 |
| `SUCCESS_SOUNDNESS` | PROVED_CONDITIONED_ON_EXACT_COMPARISONS | 对所有有效 n/K、稳定严格键，任意两个 Partition SUCCESS 产生唯一 rank r；membership/inverse 恰 K 位。比较器/适配器正确性单独靠 conformance/E2E。 |
| `PROBABILITY_BOUND` | PROVED_FOR_IDEAL_SAMPLER_PARAMETERS | 当前参数理想 abort union bound n≤10^6 小于 10^-6；计算随机生成假设单独列出，不继承 BMW16 定理。 |
| `N256_FUNCTIONAL` | PASS_FOR_K2_K8_TESTED_INPUTS | common tape 两例逐层相同；T/P0/P1 独立 UID E2E 两例通过 oracle，真实材料/RSS 在界内。 |
| `CONDITIONAL_SECURITY` | NO-GO / H1→H2_POOL_VIEW | 需要的 aux-input correlated-threshold multi-key DCF 定理与完整 shuffle/output-share simulator 未在实现/来源中找到；无“安全不可能”结论。 |
| `PRODUCTION_DELIVERY` | NO-GO | 本机 harness root 预配 party key；无 production T 认证注册/私有交付、抗回滚共享账本或异机密钥生命周期。 |
| `PERFORMANCE_COMPARABILITY` | EXPERIMENTAL_PILOT_ONLY | n64/128/256 1+5 same-host loopback；未与全对全 Protocol I 或 I+AAV86 同 revision/网络/计时边界配对，不能入 V4。 |

V3、V4 五次 LAN/WAN 矩阵和六方案结论继续未完成。下一项唯一最关键安全工作是对实际 `ProtocolIUcmpMaterial::keyGenDCF/evalDCF` 形式化并审计上述 **auxiliary-input correlated-threshold multi-key party-view game**，含完整相关未用 pool 与一轮自适应 query。若原语只支持单 key 标准隐私，就先更换/扩展原语证明接口；不要改写 Select 或只增加 domain separation 后宣布 secure。
