# BMW16 S28：实际 DCF 与 Protocol I 联合视图独立复核

日期：2026-10-09

身份：Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL
审查对象：S27 文档提交 8bbf7119470bbc37f6950a3b425255ed993eaa8d；被审 runtime 是 S26 最终源码 2818bce20f30719eaccf0bc5df586cce4fe78c84，运行时修复提交 ded433d636a3df929380bdce0ff22f4ce6121453。

## 决定

本接收者独立复核了 S27 的关键 DCF 变量关系、相关材料池 hybrid、shuffle/inverse 条件分布及采样器边界。没有发现完整序列化 key 的具体攻击或当前 runtime 的确定性缺陷。S27 的 source-specific 单 key 证明在理想扩展器模型下代数闭合；实际源码结论只能是有条件的，且必须明确采用受限 AES key 子族假设。

| 门 | 裁决 | 范围 |
|---|---|---|
| DCF_SINGLE_KEY_PRIVACY | CONDITIONAL | M2UC v1 完整 party key；依赖 AES 受限子族 126→382 PRG、cryptoTools AES-CTR 根流及 OS CSPRNG。没有将 BGI15 Theorem 6 直接转移到 VFSS 字段。 |
| ADAPTIVE_FULL_POOL_VIEW | CONDITIONAL | 固定输入与相关阈值向量下逐槽 hybrid；理想功能生成 L，模拟器再采样全池 key 并按 L 执行。 |
| SHUFFLE_OUTPUT_COMPOSITION | CONDITIONAL | 实际 forward/inverse 代数和给定输出 share 的反向采样闭合；依赖理想 T、独立均匀置换/mask、候选 L 和随机性合同。 |
| SAMPLER_ABORT_GUARANTEE | CONDITIONAL | S16 理想均匀无放回界仅在 ROM、至少一个诚实 OS 熵贡献时条件性套用；标准模型 SHA-256 counter 没有本项目已证明的分布界。 |
| FUNCTIONAL_REGRESSION | PASS（有界） | 本轮定向重建；39 项 common-tape differential 与 fresh-material mTLS 三进程 E2E 至 n=256 通过。n=1000 未在本轮运行。 |
| SECURE_ALIAS_READY | UNPROVEN | 条件假设及联合模拟未获另一独立安全接收者审核；不建 secure alias。 |

本轮未修改 party/runtime、DCF、uCMP、材料格式、shuffle、泄露、轮数或指标口径。仅新增隔离 TEST_ONLY 理想一层枚举器，并在 E2E 测试 harness 加入 T 子进程失败输出，便于诊断错误构建配置。方案始终标为 PROJECT_DERIVED / EXPERIMENTAL；四层 Select 只表示选择阶段的比较依赖。

## 接收基线与证据类型

工作从 S27 精确 HEAD 建立独立 worktree，分支 codex/m6b-i-bmw16-s28。起始提交 8bbf7119470bbc37f6950a3b425255ed993eaa8d 的父提交为 S26 最终源码 2818bce20f30719eaccf0bc5df586cce4fe78c84。S26 runtime 修复为 ded433d636a3df929380bdce0ff22f4ce6121453。main 与 origin/main 均为 c3926c68fd14f270faa8b55234311071947fa080；merge-base 相同。S27 是文档提交，没有改变被审 runtime。PR #28 不在本候选内，本 worktree 未改其分支或文件。

| 类别 | 本决策用法 |
|---|---|
| PAPER | BGI15 Definition 2、Algorithm 5/6、Theorem 6 只提供论文自身 DCF 的 key-privacy 定义与构造结论。 |
| SOURCE | 以当前 VFSS dcf.cpp、keypack.h、uCMP、材料生成、shuffle、party-node 的运算、字段和消息为准。 |
| PROJECT_DERIVED | 下文的理想扩展器归纳、pool hybrid、shuffle/output 模拟与 Select 组合。 |
| UNPROVEN / ASSUMPTION | AES 受限 key 子族 PRG、AES-CTR 根流、OS 熵、随机预言机采样、生产凭据部署和外部独立接收。 |

BGI15 来源可查 [IACR EUROCRYPT 2015 proceedings PDF](https://www.iacr.org/archive/eurocrypt2015/90560300/90560300.pdf) 和[作者完整版本](https://tzin.bgu.ac.il/~gilboan/publications/DPF-Extended.pdf)。Theorem 6 对应论文自己的 Gen/Eval，不直接证明 VFSS 压缩 key。S24 关于 common seed correction 不等同论文独立 target correction 的字段映射断点仍然成立；S28 没有撤回它。

## 实际 DCF key 和单方安全游戏

支持域：Bin=b∈[34,53]、Bout=64、groupSize=1。KeyGen 阈值 α∈Z/(2^b)，payload β∈Z/(2^64)。输入按最高位优先处理；两份 Eval 输出之和为 Dα(x)=β·1[x<α] mod 2^64。

M2UC v1 单方序列化对象包含：

- 9 字节头：M2UC、version=1、party、Bin、Bout=64、groupSize=1；
- 三个 64-bit big-endian 长度：Bin+1、1、Bin；
- Bin+1 个 16 字节 raw block k（该方 root 和各层 correction）；
- 一个 64-bit big-endian g；
- Bin 个 64-bit big-endian v。

长度为 57+24b 字节，b=34 时 873，b=53 时 1,329。block 是原始内存复制，完整 ABI 仍依赖 host block 布局/字节序；本审查不扩大其跨 ABI 承诺。session、task、round、endpoint 由外层 slot/AAD 绑定，而不是 M2UC 自身认证。完整 key 隐私游戏向单方对手提供此完整字节串和公开参数；对手任意 Eval、反序列化和本地运算均视为确定性后处理。

主要源码位置：

| 行为 | 源码 |
|---|---|
| 两根 block、互补 root control、MSB-first 目标位 | VFSS/ext/FSS/dcf.cpp:152–188 |
| 两方 seed/value 四分支扩展，输入点 0/1/2/3 | dcf.cpp:193–216 |
| v_i 和累计量 A_i 更新 | dcf.cpp:219–242 |
| shared correction 的高 126 seed 位和两个 control 位 | dcf.cpp:244–266 |
| 终端 g | dcf.cpp:269–284 |
| Eval 分支、逐层 v share、party sign 与终端项 | dcf.cpp:87–148, 294–335 |
| pack 字段 | VFSS/ext/FSS/include/FSS/keypack.h:8–23 |
| strict-less 两次 Eval、M2UC 序列化 | VFSS/src/moe_topk/protocol_i_ucmp.cpp:6,13–15 |

## DCF 源码专属理想扩展器证明

根生成取两个独立 block：k1[0]=s1，k0[0]=(s0 & ~1) XOR (lsb(s1) XOR 1)。所以两 party root control 互补，而任一 party 自己的完整 128-bit root block 边际均匀；root bit 1 也保留在 serialized block 中。令每个有效 DCF 节点状态由 126-bit seed S 和 control t 构成。源码将 s 按 s&~3 形成 AES key：高 126 位进入 AES，最低位参与状态转移，第二低位不进入 AES/control。四个固定 AES 输入是 0、1、2、3。输出 0/1 提供左右 child seed/control，输出 2/3 经 convert 截成两个 64-bit value。child block 的第二低位不进入后续 AES/control，归纳不依赖其隐藏。

理想 G126 将左右 child 的高 126 seed 位、低位 control，以及左右 64-bit value 看成一次性独立均匀坐标，总长 382 bit。一次性掩蔽来自同一理想 G 输出坐标独立，不是把 serialized correction 先验假设为独立。

固定阈值位 keep=a_i、lose=1−a_i。设累计量 A_i，epsilon=(-1)^t1。代码在 Z/(2^64) 中满足

\[
v_i=\epsilon(-A_i-V_{0,lose}+V_{1,lose}+[keep=1]\beta),
\]
\[
A_{i+1}=A_i-V_{1,keep}+V_{0,keep}+\epsilon v_i.
\]

两种腐化方分别是同一归纳的镜像情形：P0 corrupt 时隐藏扩展侧为 P1，v_i 中隐藏 losing value 的系数为 +epsilon；P1 corrupt 时隐藏侧为 P0，系数为 -epsilon。terminal g 中隐藏最后 keep value 的系数分别为 +1 与 -1。两者绝对值都是环单位。给定 corrupt party 的根、其每层展开和历史 serialized prefix：

1. 隐藏方 losing seed 高 126 位独立均匀，以 XOR 掩蔽 CW 高位。
2. 隐藏方左右 control 分别掩蔽 CW 两个 control 位。CW 打开后，另一侧 control 可由本方 control 与 CW 求出；本证明不声称 control 保密。
3. 隐藏方 losing value 以系数 +1 或 −1 进入 v_i，因此在 Z/(2^64) 中一次性均匀掩蔽 v_i。
4. 隐藏方 keep seed 是新独立坐标。下一层 raw seed 的高 126 位均匀；control/CW 导致的 correction 是已知 XOR 偏移，归纳继续。
5. 隐藏方 keep value 不进入当前 v_i，而进入 A_{i+1}，最后以单位系数进入终端 g。末尾的 g 掩蔽来自该 value 坐标，不是把低位清零的终端 seed 当作 64-bit 均匀值。

每层 corrupt party 的完整 128-bit CW 和 v_i 对任意 α、β 条件均匀；terminal g 同样均匀。故 G126 模型下完整单方 key 是固定头、均匀 root、b 个独立均匀 CW、b 个独立均匀 64-bit v 和一个均匀 64-bit g，分布不含 α、β。两方 key 共用 CW/v/g；本结论是单方隐私，不称两份 key 相互独立。

新增 TEST_ONLY 模型穷举极小域：2-bit node seed、一个 control、Z/2 values、一层、两种阈值/两种 payload、两方，并条件化不同 root seed。复核时发现最初 checker 忽略了 serialized root block 中既不进入 AES key、也不参与 control 的第二低位；该位仍属于完整 party key。已将它加入 tuple 并重跑：25,165,824 个 ideal tape case；每方 1,024 种 root/CW/v/g 投影 tuple，每种频数 6,144。此修正只补全 toy checker 的覆盖，不涉及 runtime。实验仅检查递推/计数，不能验证一般 Bin、AES 或密码学安全性。

### 理想到实际 AES 的条件

当前 AES key 受限为 encode(S||00)，S 有 126 bit。实际 computational bridge 需要此 restricted-key family 在四个固定输入上的 PRP/PRG 安全，而不是未经说明地使用均匀 128-bit key AES 的一般 PRP 声明。若采用 restricted-key PRP 假设，从 PRP 到随机函数的 4 点切换差额至多 6/2^128；PRP 优势保留符号 Adv_AES-restricted。另需 cryptoTools AES-CTR 根随机流 PRG 和 OS getrandom 假设。没有编造安全位数。

S24 字段映射失败不是完整 key 攻击。S28 是当前源代码递推的条件证明，不是 BGI15 Theorem 6 对 M2UC v1 的直接迁移。

## 整池、两次 Eval 和自适应访问

令 p=padded_n、C=n(n−1)/2。当前 DCF KeyGen 次数：

\[
N_{DCF}=2p+9C=2p+8C+C.
\]

2p 是 raw carry/sign；8C 是两个 Select task 各四层全池边；C 是 membership。shuffle-only forward/inverse 另有两个每方 slot，不是 DCF KeyGen。offline_material_slots_per_party=2p+2+9C。`n=1` 走 shortcut，不调用该材料生成器；本节公式表和全池混合仅对 `n≥2` 的一般路径计数。

T 逐槽新 KeyGen；本次源码审查未见 task/round/edge/membership 间 key reuse。每个 uCMP slot 对 x=(z_l−z_r) mod 2^b 和 y=(x−2^(b−1)) mod 2^b 做两次 DCF Eval；共用同 key，是完整 key 上的确定性后处理。双向请求和跨轮材料使用不能混为一谈。

hybrid 顺序为：

1. 固定完整输入、T randomness、共享 node masks、由此确定的相关阈值向量和理想协议 coins。单 key 归纳对任意固定 α、β 成立，所以可先条件化在相关 alpha 向量上。
2. 将根 AES-CTR 前缀逐活动流替换成均匀 block tape，差异记为 Adv_root((q_t)_t)。
3. 按 slot 顺序，把 corrupt party 完整 pool 中每把 key 隐藏一侧的 b 次 G126 换为独立随机 tape。归约可把固定 alpha/beta 向量与已有生成状态作为辅助参数；目标是指定函数 key 分布与参数无关均匀 key 的替换。
4. 完整 key pool 在线前已在 corrupt party view 中。未使用槽也在初始模拟器中直接采样，不可遗漏。未来的自适应边/slot 只按理想 L 已含的先前 transcript、coins 和 endpoint 顺序访问。
5. ideal functionality 读取私有输入并在内部生成 ideal randomness、masked operands、比较 bits、anonymous edges、pivot/U/V/W、selected handle、material ID/access order、abort stage 和消息长度，形成 L 后交给 simulator。Simulator 不以 real transcript 为输入。
6. 给定公开比较 bit c 与本地 Eval share s，peer share 按 c−s mod 2^64 构造；这与真实打开的加法 share 关系相同。覆盖同 key 两 Eval、未访问 key、两 task 共用 anonymous handle 和 membership slots。

逐 corrupt party 的保守界：

\[
Adv_{pool}\le Adv_{root}((q_t)_t)+N_{DCF}bAdv^{aux}_{G126}
 + {2N_{DCF}b\choose2}/2^{126}.
\]

G126 假设须覆盖固定参数及 hybrid prefix 作为 auxiliary input。最后一项排除 2N_DCF b 个 node-state 输入中的 seed 碰撞。q_t 是每条 AES-CTR stream 实际被 KeyGen 消费的 128-bit blocks；当前 T 单线程 KeyGen 时 q_0=2N_DCF，其余为 0。cryptoTools SetSeed 的 256-block buffer prefill 属于内部 AES 运算，不算被 KeyGen 消费的 blocks。此计数是源码布局推导，不是 S28 PRG 计数实测；未来并行 KeyGen 必须按每线程重算。

| n | p | C | N_DCF | q_0=2N_DCF |
|---:|---:|---:|---:|---:|
| 2 | 2 | 1 | 13 | 26 |
| 3 | 4 | 3 | 35 | 70 |
| 5 | 8 | 10 | 106 | 212 |
| 8 | 8 | 28 | 268 | 536 |
| 64 | 64 | 2,016 | 18,272 | 36,544 |
| 128 | 128 | 8,128 | 73,408 | 146,816 |
| 256 | 256 | 32,640 | 294,272 | 588,544 |

上述表格是公式派生，不是实测消耗、时间或性能结果。

## 泄露函数和完整 party view

候选 leakage L：session/config/channel；raw adapter 对外公开的 carry/sign masked operands；forward 与 inverse public_z/masked vectors；匿名比较端点和打开 bit；采样、pivot、U/V/W 的匿名位置；selected anonymous handle；slot/key ID、顺序、逻辑比较次数、协议 phase/frame 长度；两方 XOR 得到的 sampler seed；统一的算法 abort 状态及已公开 transcript 可推断出的 abort 阶段。

L 不含 raw score、original_index、真实 index↔anonymous handle mapping、真实 rank、selected stable key 或完整明文 mask。每方真实 view 另有本方 raw input share、完整已用和未用 key pool、本方 forward/inverse shuffle factors、自发/收到的所有 frames、本地 Eval shares、selected local output share、最终 XOR mask share 和本地状态。T 可信且在线静默、不与 P0/P1 合谋，仅持有公开离线配置。

比较 bit 已公开时 party 可推导 peer arithmetic Eval share。这是候选泄露的一部分，不自动构成安全证明。模拟器只能取得 ideal functionality 根据真实私有输入与 ideal coins 自己生成的 L；不得把 real transcript 原样塞给模拟器来证明自己可模拟。

材料、TLS、OS 与不变量错误是工程失败，不属于抽样概率 abort。没有 peer ACK 时仅报告 LOCAL_ONLY，不报告 PEER_AGREED。Abort 前公开图可以泄露失败阶段；不声称不同阶段在统计上不可区分。

## Forward/inverse shuffle 与输出 share

实际来源为 VFSS/src/moe_topk/protocol_i_parallel_shuffle.cpp:318–370（dealer）与 :453–515（party rounds）；BMW16 编排在 VFSS/src/moe_topk/experimental_bmw16_select_party.cpp:1016–1078（forward、coin、Select）及 :1082–1123（inverse、status）。dealer 取置换 π、独立均匀 σ0,σ1，令 τ0=π∘σ1⁻¹、τ1=π∘σ0⁻¹；再取独立均匀 a0,a1,h,r0,r1，设 e0=−τ0(a1)−h、e1=−τ1(a0)+h。party b 两轮消息为 m_b=σ_b(x_b)+a_b 和 q_b=τ_b(m_peer)+e_b+r_b。代入得：

\[
q_0-r_0=πx_1-h,\quad q_1-r_1=πx_0+h,\quad
z=q_0+q_1=π(x_0+x_1)+r_0+r_1.
\]

inverse 使用 fresh factors/masks 和 π⁻¹。条件于 corrupt party 最终 XOR output share y_b，逐记录模拟：均匀抽样 peer round1 frame m_peer；抽本方中间值 w_b，固定 word0 LSB=y_b，其余有效位均匀；反解 e_b=w_b−τ_b(m_peer)；取 fresh r_b，造本方 round2 q_b=w_b+r_b，peer frame 由公开和式 z−q_b 决定。真实关系是 w_b=τ_b(m_peer)+e_b，输出 share 是 LSB(w_b)。独立新鲜 h 使 e_b 条件均匀，peer a 使 m_peer 均匀，peer r 掩蔽 z。此方式联合模拟 output share 与 inverse messages；不需向 simulator 交付 π、诚实方输入或完整明文 mask。

两 Select task 共享同一匿名真实句柄/置换。Simulator 不获知 π，以 ideal functionality 自行生成 anonymous transcript 和 selected handle。该论证是项目 shuffle 的条件性代数模拟，不借 Protocol I 论文定理；要求实际独立随机 mask、置换、ideal T 与 L 字段完整。有限枚举只用于寻找反例，不能替代一般证明。

## 抽样保证

双方先各自从 getrandom 取得 32-byte contribution，公开交换后 XOR；随后对 (seed, task, domain, counter) 计算 SHA-256 并使用 rejection sampling 得到无放回样本，任务/域/计数隔离。S16 历史脚本逐字从原 revision 执行：理想均匀无放回模型，n≤256 的最大 union bound 为 1.3105804606611235e−11；公式域延展至 n≤10^6 为 5.065679989546104e−8。ROM 且至少一个诚实 OS 熵贡献时条件性使用该数学界。标准模型中公开 SHA-256 counter 不是 keyed PRF，本项目没有证明其样本流与独立均匀分布不可区分。零自然 abort 不等于概率证明；注入 abort 不计入自然频率。

## 本轮验证

环境：WSL Ubuntu 24.04、Linux 6.6.87.2-microsoft-standard-WSL2 x86_64、CMake 3.28.3、GCC 13.3.0、OpenSSL 3.0.13。构建产物在 /tmp/moe_bmw16_s28_review、/tmp/moe_bmw16_s28_test；原始结果在 C:\Users\28641\.codex\artifacts\BMW16_S28_20261009\run_01。

| 运行目标 | 本轮结果 | SHA-256 |
|---|---|---|
| moe_topk_m2_ucmp_conformance_test | exit 0 | 18cd538f1e33a0177e1fb4a95f14b0670e3e0ee7498adf8b2d79197cd58ea950 |
| moe_topk_m2_parallel_shuffle_conformance_test | exit 0 | 1d29afb194c62d9c23cd777ac9af09b9e8bc5831d167010def631d5953ac3692 |
| moe_topk_bmw16_experimental_party_test | exit 0；n=1 shortcut、n=5/8、n=64 样本和 abort negative | f2250060f8c30ae4cd34b97d72b638708491841ea2e7d3f82b1b78a0df134942 |
| ideal_dcf_one_level_check.py | 25,165,824 个 ideal tape case PASS；1024 种 tuple 各频数 6144；不是密码学证明 | 文件 SHA-256 1833336F50F4506F4E01B6CA46F5719950863D3B1473CC642E60B44A78E23CC7 |
| S4/C++ common-tape matrix | 39/39 PASS | matrix.json SHA-256 ED383DF5CB2B8D1C5B5A6601B95B8130A8C04931AC032DA0E6513A52CF824F86 |
| independent T/P0/P1 mTLS E2E | exit 0，至 n=256 | 成功日志 SHA-256 C1999EC049525B9787F7B54139277C63DF16D8362D3E4F83E337CCF33A15780C |

E2E 成功配置：n=1/K=1 shortcut；n=2/K=1；n=3/K=2；n=5/K=5；n=8/K=4；n=64/K=8；n=128/K=128；n=256/K=2。每项 fresh materials，TEST_ONLY oracle 检查原序、二值、weight=K。负例覆盖重领、错 party/session/stream、重排/截断/篡改 sidecar、已有 sidecar、T prepare/commit fault、receiver publish/fsync/silent、peer close/silent、强制算法 abort、工程 fault、missing TLS stream、最终状态不一致、mask publish failure；失败无 mask，断连无 ACK 仅算 LOCAL_ONLY。不同 UID 和 loopback TCP+mTLS 为同主机隔离试验，不是跨主机安全部署或性能结果。自然算法 abort 观察 0 次；注入 abort 单列。

首次 E2E 使用 failpoint-OFF Release node，harness 需要的 TEST_ONLY t-local-test-only 子命令不存在，child exit=64。输出 usage 证明是编译开关不匹配，尚未进入协议路径。独立 TEST_ONLY 构建后 E2E exit 0；此初次失败保留为配置诊断。

代表 binary：

| Binary | SHA-256 |
|---|---|
| Release party-node | 5d8033b29f6e8aa9b8cc2ccff0262564b342aab8f19b1be2d7f24baf5a89d52e |
| TEST_ONLY party-node | bea403fb8ee8e39460fde3542354e289af25e2c91193f4d4c0038a7cb0949b5c |
| TEST_ONLY E2E harness | acf4520d9a65911bd26b5deca8aa3ee277ffe8ce6bc4a2fab327ee089109260d |
| Party conformance | f2250060f8c30ae4cd34b97d72b638708491841ea2e7d3f82b1b78a0df134942 |
| uCMP conformance | 18cd538f1e33a0177e1fb4a95f14b0670e3e0ee7498adf8b2d79197cd58ea950 |
| Shuffle conformance | 1d29afb194c62d9c23cd777ac9af09b9e8bc5831d167010def631d5953ac3692 |

完整构建、执行命令与文件 hash 在决策文档和外部证据目录。只执行定向目标；full CTest NOT_RUN。由于 runtime 与材料代码未改，n=1000 本轮 NOT_RUN；S22/S26 的历史 n=1000 只归属原 revision。LAN/WAN、总时间、PRG 计数、九指标均 NOT_MEASURED。

## S28 文件改动

1. 新增 experiments/TEST_ONLY_BMW16_S28/ideal_dcf_one_level_check.py：独立 toy-domain 理想代数 checker，不进入 VFSS party/runtime。
2. 修改 VFSS/tests/moe_topk/experimental_bmw16_party_node_e2e_test.cpp：T 子进程失败时打印其隔离日志，以识别 OFF/ON 构建错配。
3. 新增本决策和 docs/reproduction/BMW16_S28_INDEPENDENT_SECURITY_REVIEW_2026-10-09.md。
4. 更新 PROJECT.md 和 docs/IMPLEMENTATION_PLAN.md 的当前状态；不改历史 dated 报告，不改变里程碑正式顺序。

git diff --check 与禁止路径检查须在提交前通过。测试密钥、certificate private keys、materials、binaries、paper 和 raw logs 不进入 Git。本轮不运行正式 LAN/WAN 性能、不创建 secure alias、不推送、不合并。

## 下一接收者最小核对表

1. 独立审核 G126 auxiliary-input 定义是否覆盖固定 α/β、前序 key prefix 与完整 view。
2. 检查 2N_DCF b 状态碰撞事件和各线程 q_t 计数是否保守覆盖真实 root/child seed。
3. 核对 ideal F 自行生成 L 的模拟方向和所有真实 frame/key ID 字段。
4. 检查 inverse output-share 条件采样及本方 output share 与此前 shuffle view 的联合条件分布。
5. 独立接收最终源码/二进制；本聊天不自签异会话接收。
6. 单独审查 ROM sampler 假设、生产凭据/部署与正式性能门。
