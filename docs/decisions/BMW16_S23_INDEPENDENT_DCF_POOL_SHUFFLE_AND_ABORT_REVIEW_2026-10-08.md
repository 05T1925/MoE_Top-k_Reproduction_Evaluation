# BMW16 S23：S22 的 DCF 整池、shuffle 与抽样界独立裁决

日期：2026-10-08
方案身份：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL**
被审源码：S22 `9210c11a7a877f8fe3687e0509c1722440cf63fe`。本决策由该提交的独立 worktree 产生，不回写 S22 历史文件。

## 决定

1. 维持 `EXPERIMENTAL`，不创建 `secure` alias，不进入正式 V4/LAN/WAN 矩阵或六方案速度排名。
2. 学姐/项目所有者对匿名 transcript 的许可来自用户在 S22/S23 任务中转述的答复。本文件将其记为项目批准的**候选泄露函数**，不记为密码学安全证明。
3. S22 的“`N·ε` 条件性多 key hybrid”具有可审查的证明骨架：对任意两组具有同一候选泄露 `L` 的固定输入视图，先条件化于不由 DCF KeyGen 随机带决定的 `L`，再逐槽替换完整 party key；替换槽两边必须给出同一已打开比较位。把其余完整 key（包括未用 key）放进单 key游戏的 challenge-independent state。若每个 DCF 槽确实满足 BGI15 的完整 party-key 函数隐私，且 T 的每槽 KeyGen 随机带可替换为独立随机 tape，则相关阈值本身不阻止标准 hybrid。挑战 key 上的本地 Eval 是完整 key 的后处理；在相同已打开 bit `b` 下，收到的 peer share 可由正确性等式 `b-s_c` 精确构造，不需声称它是独立均匀值。公开 bit 决定后续图时，只能作为已固定的 `L` 后处理；这不是允许挑战 bit 改变 transcript。该结论是**条件推导**，不等于当前 VFSS DCF 已满足 BGI15 定理或完整 shuffle 联合模拟已成立。
4. 当前第一个可定位的 DCF 缺口是：仓库 `keyGenDCF/evalDCF` 与 BGI15 Algorithm 5/6 的逐项构造等价尚未被证明。S22 直接把 BGI15 Theorem 6 的单 key界套给 VFSS 实现，缺少这一桥接引理和其 PRG 参数/优势映射。
5. Protocol I 的实际 forward/inverse shuffle 与两任务共享匿名 handle、membership、abort、原序输出 share 的完整模拟仍未闭合。shuffle roundtrip/conformance 只证明功能关系，不证明该联合视图可模拟；本轮没有得到反例，也不把“未找到反例”记作安全 PASS。
6. S16 的理想均匀无放回抽样 abort 界在固定整数规则域内复核通过。当前公开 XOR coin + SHA-256 domain/counter + rejection sampler 在**随机预言机模型**中实现该理想分布（排除 seed 预查询/跨会话碰撞事件）；对具体 SHA-256 的随机预言机差异没有已知项目级数值约简，故实现级数值保证保持条件性。

## 范围、角色与泄露函数

角色假设：T 可信、仅离线接收公开 `n/K/session/fingerprint/width` 等配置、在线输入出现前退出且不与 P0/P1 合谋；P0/P1 半诚实，只持有本方 raw-score 算术份额、party-only 材料、私有认证在线通道和本方最终 XOR mask share。目标理想功能在 SUCCESS 时输出原序且 weight 恰为 K 的 XOR shares；抽样失败双方一致 ABORT 且无 mask。这个功能身份不继承 BMW16 Theorem 8，也不代表 BB90 原算法。

本轮采用以下“已获项目许可公开”的候选泄露函数 `L`（许可来源为用户转述，不是独立安全结论）：

| 字段 | 产生阶段/消息 | 可见方 | 与秘密的关系 | 安全状态 |
|---|---|---|---|---|
| `n,K,session,fingerprint,width,stage/task/round` | 会话配置、frame/header、bundle manifest | T/P0/P1 | 公开输入 | 配置公开 |
| `public_z`/masked operand 向量 | forward shuffle 第二交换及其和 | P0/P1 | 由原始记录、隐藏置换和随机 mask 共同决定 | 许可公开；shuffle 联合模拟未证 |
| 匿名边端点、方向、round/task、实际 key/slot ID 与读取次序 | 每轮 frozen edge frame 和本地 sidecar `load_once` | P0/P1 | 端点为 shuffled handle；slot 编码 session/use/task/round/canonical handle pair | 许可公开；不得编码 original index/score |
| 每条比较的打开 bit、pivot、U/V/W、样本及图控制 | Select 每层比较结果交换后 | P0/P1 | 依赖真实稳定键与采样 coin | 许可公开；决定后续自适应控制 |
| 两个 Select 任务的共同真实 handle 关系、selected anonymous handle | 两任务交集及最终 selected frame | P0/P1 | 定位 shuffled record，但不包含原序映射 | 许可公开；共同置换相关性仍纳入联合 view |
| 比较次数、帧长、通信长度/相位 | transport counters / frames | P0/P1 | 受比较图和 abort 影响 | 许可公开 |
| 统一算法中止状态 | final agreement byte：0 SUCCESS、1 概率 abort、2 invalid | P0/P1 | 公开成功或中止；前序图可能提示阶段 | 许可公开；不能声称失败原因完全隐藏 |
| `local_reason`、工程堆栈 | 本地诊断 | 仅本方 | 可能揭示错误细节 | 不进入协议 status frame；不同步共享 |
| 原始 score、original_index→shuffle handle 映射、原序 rank、选中明文稳定键、完整 mask | 不应出现在线上普通消息 | 不应公开 | 秘密 | 禁止公开 |
| 本方输出 XOR share | party API 返回/本方文件 | 本方 | 单方随机份额 | 由理想功能输出，不等同完整明文 mask |

许可 L 仅列明可接受的信息量。它不证明 `view(P0)` 或 `view(P1)` 可由 `L` 与本方理想输入/输出模拟。真实 view 还包括完整本地 shuffle 因子/帧、整个本方已用及未用 sidecar key、每次 Eval share、收到的 peer share、slot claim/访问轨迹、两 task 共享的真实匿名 handle、inverse 因子与最终本地 output share。

## BGI15 与 VFSS DCF 的逐式对照

论文来源为 Boyle–Gilboa–Ishai, *Function Secret Sharing*, EUROCRYPT 2015。Definition 2（论文 PDF p.7）将安全游戏定义为：攻击者提交同域函数 `f0,f1` 和任意 state，挑战者对随机选择的函数生成 key，向腐化方给出其完整 party key；Theorem 6（PDF pp.19–20）覆盖 `f_{a,g}^<(x)=g` 当 `0≤x<a`、否则 0 的二方比较函数族。Algorithm 5/6 位于 PDF pp.18–19；论文说明相关证明细节因篇幅转至 full version。见 [BGI15 论文 PDF](https://www.iacr.org/archive/eurocrypt2015/90560300/90560300.pdf)。

| 对象 | BGI15 定义 | VFSS 实际代码 | 结论 |
|---|---|---|---|
| 输入域/阈值 | `x,a ∈ {0,1}^b`，`x<a` | `ProtocolIUcmpMaterial(b,ml,mr)` 令 `alpha=(ml-mr) mod 2^b`；uCMP 对 `x=(zl-zr) mod 2^b` 与 `y=x-2^(b-1)` 求值，`b∈[34,53]` | 函数域/输入宽度语义可逐源码追踪 |
| 输出组 | 任意有限阿贝尔加法组 G | `GroupElement=uint64_t`，`Bout=64,groupSize=1`，运算模 `2^64` | 可把目标组记为 `Z/(2^64)`；不是 bit/XOR 组 |
| DCF correctness | 两 party Eval shares 相加还原 `1[x<alpha]` | `evalDCF` 沿路径做 AES expansion，并按 SERVER1 取负；uCMP 两次 Eval 后作 `t2-t1`、wrap correction，P0 返回 `1-ge`、P1 返回 `-ge` | 源码方程与 uCMP conformance 支持功能对应；功能测试不证明 FSS 隐私 |
| 完整 party key | Definition 2 的 `k_i` | `DCFKeyPack=(Bin,Bout,groupSize,k[Bin+1],g[groupSize],v[Bin·groupSize])`；uCMP wire 序列含 party/bits/domain/group、长度、全部 k/g/v，`serialize()` 不只给 root seed | 若 FSS 定理适用，未使用的 key 也属于被保护的完整 key |
| Gen/Eval 构造 | BGI Algorithm 5/6 的特定 seed/control/correction-word 关系及一个 PRG 接口 | `dcf.cpp:152–284` 从 `prngs[tid].get<array<block,2>>()` 取两个 block；随后每层对两个当前 seed 调 `ecbEncFourBlocks` 并构造 k/g/v；`eval` 每层调 `ecbEncTwoBlocks`。uCMP wrapper 在 `protocol_i_ucmp.cpp:5–15` | 未找到可审计的逐变量映射，故不把 Theorem 6 直接归给此代码 |

最小未证桥接命题：`DCFCodeEquivalentToBGI15GenEval(b,Bout=64,group=1)`。证明须给出从 `dcf.cpp` 的两 root blocks、low-bit/control 编码、每层 AES 四块输出、correction words、最终 `g/v` 以及 SERVER0/1 符号，到 BGI Algorithm 5/6 变量的一一映射；再把固定输入宽度 `b=34..53`、组 `Z/(2^64)`、AES PRG 与实际 `cryptoTools::PRNG` 的优势条件接入定理。S23 未发现“两个 root blocks”本身导致错误的功能反例；该结构可能是等价优化或另一安全构造，但没有推导不能据此宣称相同证明。

## DCF 整池 hybrid：条件性推导及其边界

记 `C=binom(n,2)`、`p=padded_n`。当前实际 KeyGen 函数调用数是
\[
N_{DCF}=2p+9C,
\]
而每 party 在线 sidecar 实际拥有 `9C` 个 Select/membership key slots：两 task × 四 round × C，加 membership × C。`offline_material_slots_per_party=2p+2+9C` 中 `2p` 是 raw carry/sign DCF，`+2` 是 forward/inverse shuffle stage 材料计数，不是 DCF KeyGen。S22 把三种“slot”混称的描述应按此口径解释；源码本身公式一致。`expected_slot_count()` 为 `9C`；`generate_ucmp_slots()` 的 base 为 `2p+2`，之后严格生成 8 个比较池与 membership 池；shuffle 配置为 `include_cmpagg_material=false`，不生成 shuffle CmpAgg DCF。

在先假设每个 slot 使用独立均匀 KeyGen tape 且当前实现确实满足 BGI15 单 key 定义时，可这样构造 H1→H2：

1. 固定两个比较世界具有相同的腐化方输入/output share 和同一许可泄露 L；L 的整条自适应 transcript、匿名端点、两次 DCF 输入、公开 uCMP bit、选中的 handle、未用 slot 列表、frame 长度/次序全部在向 FSS challenger 提交函数对与 `state` 前固定。BGI15 Definition 2 允许 `state` 与候选函数关联，但它不能由挑战比特决定。不能把真实协议事后生成的 `L` 循环地塞入游戏：证明必须先对每个固定 `L` 证明它与两候选函数均一致、且 `L` 不依赖 KeyGen 随机带，再对 `L` 的理想分布取混合。若某候选阈值令已打开 bit 或后续图不同，该候选不属于同一 `L` 比较类。
2. 对第 j 个 slot，用两个被比较的输入世界各自的精确函数 `f0=f_{alpha_j,1}^<`、`f1=f_{alpha'_j,1}^<` 作挑战；不以任选的“代表阈值”代替真实混合分布。其他 slot 的函数、相关 node-mask 派生值、腐化方 shuffle 辅助信息和其它 party keys 全部放进同一个固定 state。若 `L` 不唯一决定目标阈值，归约必须对整个同 `L` 等价类的真实阈值混合证明，不能只挑一个与打开 bit 相容的值；若两候选在任何已打开 bit 上不同，这对世界不属于同一 `L` 类。挑战 bit 只选择挑战 party key，不改变该 state。
3. 一个 uCMP slot 由一次 `keyGenDCF` 产生的完整 party DCF key 支持；`eval_strict_lt` 在两个输入 `x,y` 上重复 Eval **同一个 key**，并在本地组合结果。两次 `evalDCF(x)`/`evalDCF(y)`、组合和后续 Eval 均为完整 key 的确定性后处理。peer share 在打开 bit `b` 后等于 `b-s_c`；若该 comparator 的公开 bit 在两个挑战函数下相同，模拟器据 L 和本方 Eval share 构造即可，不需要声称 peer Eval share 独立均匀。
4. 后续图只由此前公开 bit/样本结果决定；L 已含这条图。因此任意自适应访问次序可视为对全池 keys 和固定 L 的后处理。未使用 key没有 Eval，也仍包含在完整 pool hybrid 里。相同真实 edge 在不同 task/round 使用新 key；同一 slot 只访问一次。阈值向量满足 `alpha_ij+alpha_jk=alpha_ik` 的相关性不自动破坏 hybrid：它是 challenge state 的相关输入，而不是独立性结论的前提；但相邻 hybrid 的 state 必须严格相同。
5. 若单 key安全优势为 `epsilon_DCF(b,64)`，标准 N-hop game hybrid 的形式是 `N_DCF·epsilon_DCF`，再加根随机流与 tree PRG 替换优势。该表达式保持符号；本仓库没有实测/审计得到可代入的具体安全位数。

这是一条**条件性归约路线**，不是实际代码的安全 PASS。逐槽归约的关键不是假设阈值独立，而是证明每个相邻 hybrid 的两候选函数、所有其他 key/state 和已许可 `L` 在挑战比特两侧相同；真实比较 bit 由双方 Eval 重构，但由 DCF 正确性它不依赖 KeyGen 随机带。归约可从同一 `L` 中取 bit `b`，对挑战 key 本地 share `s_c` 计算收到的 peer share `b-s_c`，于是该二元消息是实际线上开值的精确表示。仍需形式化证明真实泄露分布可按固定 `L` 分解，并处理两 task、membership 与所有 slot 的具体阈值向量。若挑战两侧的任何已公开 comparator bit 不同，该步被 `L` 直接区分，不能套用。S22 未给出这些细节，也未给出 `dcf.cpp` 到 BGI15 的桥接；S23 给出的是候选归约检查表而不是定理。有限 transcript 枚举不代替这个构造。

随机带计账：`seed_fss_once()` 以 `getrandom` 初始化 256 个 `FSSConfig::prngs`，每个 stream 取 16 B；当前材料生成调用链串行，`keyGenDCF` 取 `omp_get_thread_num()` 的 stream，实际落到调用线程（本次 party-node/offline writer 是单线程 KeyGen）。每个 KeyGen pair 从该 stream 消耗 2 个 128-bit block，即 32 B；source-derived 根带用量为 `Q_root=2N_DCF` 个 block（`32N_DCF` B），当前材料生成函数调用数可精确由 `n,p` 求出。每个 KeyGen depth 对两个 seed 各做一次四块 AES expansion：计 `2bN_DCF` 个 `ecbEncFourBlocks` 调用、`8bN_DCF` 个 AES block encryptions。每个 `evalDCF` 有 b 次 `ecbEncTwoBlocks`、`2b` 个 AES block encryptions；每个 uCMP slot query 对每 party 调两次 DCF，故实际比较数 Q_cmp 对应 `4bQ_cmp` 个 AES block encryptions/party。计数含义不可与现有 `dcf_eval`、逻辑 comparator 数混用；本轮没有打开 DCF/AES 实际 block counter，运行级 `Q_cmp/Q_AES` 不在新日志中单列时记 `NOT_MEASURED`，只保留代码公式。AES/root-PRG 优势按 `Adv_AES/cryptoTools_PRG(Q_AES,Q_root)` 符号项记录。

## Protocol I shuffle 与输出组合

实际 dealer 在 `protocol_i_parallel_shuffle.cpp:318–379` 采样 π、σ0、σ1，设 `τ0=π∘σ1⁻¹`、`τ1=π∘σ0⁻¹`，`e0=−τ0(a1)−h`、`e1=−τ1(a0)+h`；party 状态机在 `:468–495` 执行第一、第二消息。代数上 P0 第二轮本地中间项为 `π(x1)−h`，P1 为 `π(x0)+h`，相加后
\[
public\_z = \pi(x_0+x_1)+r_0+r_1.
\]
BMW16 party path 再在 `experimental_bmw16_select_party.cpp` 用同一真实 handle pool 的两份 Select、membership records 和预生成的逆置换 shuffle；成功时只返回 inverse shuffle 本方 `shuffled_share.word0 & 1`，final agreement 前不公开完整 mask。上述是源码代数和功能语义，不是 joint-view simulator。

尚缺的精确引理为 `ProtocolIForwardInverseViewSimulator_L`：输入只能是腐化方 raw share、本方理想输出 share 与完整 L；输出需联合模拟该方 forward/inverse 的 σ/τ/a/e/r_share、本地 input/output share、两轮收发帧、public_z、shared-π 的两个 Select handles、membership local shares、selected handle、最终状态和本地 abort。尤其要证明在同一 π 与 π⁻¹ 下，逆路由后的本方输出 share 条件分布与理想输出 share 一致，且与该方 forward 局部 permutation/shuffle 状态的关联不额外泄露原序映射。当前 small-n 枚举只核对功能/边际分布，不证明此引理。S23 未构造同腐化输入、同 ideal output/L、不同诚实输入下的可区分见证；因此结论是 UNPROVEN，不是“不可能”。

## 抽样界与具体 SHA-256 口径

S16 的固定规则（理想 sampler 下）为：

```text
M = 2*max(K, n-K+1); n=1 单独 shortcut
s = min(M, ceil(sqrt(64*M)))
若 s=M: qL=M/2, qH=M/2+1
否则 qL=max(1, floor((s+1)/2)-ceil(sqrt(16*s)))
      qH=min(s, floor((s+2)/2)+ceil(sqrt(16*s)))
Ucap=min(M, ceil(8*M^(3/4)))
v=min(u, ceil(sqrt(64*u*u/M)))
Wcap=2*ceil(sqrt(4*M))+1
```

对单个 Partition：bracket 项在 `s<M` 时 `2e^-31` 否则 0；`U` cap 项在 `M>4096` 时 `M·e^(-8M^(1/4))` 否则 0；`W` cap 项在 `M>64` 且 cap<M 时 `M·e^-32` 否则 0。两 Select tasks 用 union bound，**不需要任务独立**：`P_abort≤2(p_b+p_U+p_W)`。S16 独立脚本用整数根取整扫过每个可达 even M：n≤256 最高 `1.3105804606611235e-11`（M=512）；M≤2,000,000 公式域最高 `5.065679989546104e-8`（M=2,000,000）。第二个数是公式外推，不是当前 API 支持或运行结果，也不是 BMW16 Theorem 8。

实际生产抽样：双方各用 OS `getrandom` 取 32 B、经 authenticated stream 交换并 XOR 成公开 seed；`SHA256(seed||task||domain||counter)` 提供 word，`bounded()` 用 rejection (`threshold=(-bound)%bound`) 拒绝有偏余数，partial Fisher–Yates 无放回选样。理想随机预言机下，所有不同 `(seed,task,domain,counter)` 输入输出独立均匀，rejection 余数精确均匀，故实现抽样分布等于 S16 理想前提；seed 是公开值，不能叫 secret-key PRF。对随机预言机输入提前查询、跨会话 256-bit seed 碰撞可显式加 `Q_H/2^256 + binom(q_session,2)/2^256` 量级的失败事件。对实际 SHA-256 与随机预言机的区分项没有本项目可引用的定量界，应记 `Adv_SHA256→RO(Q_H)=NOT_QUANTIFIED`；不能把它和统计 abort 概率无条件相加。结论选用任务允许的随机预言机模型方案，实际实现级概率仍为 CONDITIONAL。

## 本轮证据与门禁

本轮独立证据：S22 原提交 fresh build；S4/C++ common-tape 39/39；当前构建的不同 UID TLS independent-process E2E 含 n=2/3/5/8/64/128/256 成功、13 类离线交付失败、概率/工程 abort、TLS identity、缺 stream fail-closed、对端静默和 final disagreement；非 TLS 扩展 E2E 含 15 条 `node_e2e`，所有 SUCCESS oracle check 通过，材料无效样例拒绝。定向 CTest 8/8 通过。原始输出和 hash 在 S23 验证报告登记。没有改协议源码；n=1000 未在本轮重跑，引用 S22 原件仅作历史证据。完整 CTest、正式性能矩阵均未运行。

| 门禁 | 裁决 | 依据 |
|---|---|---|
| `DCF_IMPL_CORRESPONDENCE` | **UNPROVEN** | 函数语义和 ABI 可对照；代码到 BGI Gen/Eval 的等价与 AES/根 PRG优势映射缺引理 |
| `ADAPTIVE_FULL_POOL_VIEW` | **CONDITIONAL / UNPROVEN FOR THIS CODE** | 给出 `N·epsilon` 逐 key reduction 条件；shared-mask alpha 相关性可放入固定 game state，但需上项桥接；许可 L 必须保持相同 |
| `SHUFFLE_OUTPUT_COMPOSITION` | **NO-GO / UNPROVEN** | 缺 `ProtocolIForwardInverseViewSimulator_L`；未发现反例不构成 PASS |
| `SAMPLER_ABORT_BOUND` | **CONDITIONAL_RANDOM_ORACLE_MODEL** | 理想公式复核通过；SHA-256 concrete-to-RO 优势未量化 |
| `FUNCTIONAL_REGRESSION` | **PASS (bounded)** | 39 common-tape、TLS/non-TLS E2E、定向 CTest；不是密码学证明 |
| `SECURE_ALIAS_READY` | **NO-GO** | DCF 构造桥接与 shuffle/output simulator 尚未独立证明 |

下一步最小可执行证明任务：单独完成并审查 `DCFCodeEquivalentToBGI15GenEval`，从 `dcf.cpp:168–284` 和 `:294–335` 开始，给出每个状态/输出字节对照与 AES PRG 参数；用一份小型符号模型核验阈值函数、full party-key 形状与 Eval。它通过后，再把上面的 `N·epsilon` hybrid 形式化为完整 party-view simulator，并独立证明 `ProtocolIForwardInverseViewSimulator_L`。在两者通过前保留 EXPERIMENTAL。
