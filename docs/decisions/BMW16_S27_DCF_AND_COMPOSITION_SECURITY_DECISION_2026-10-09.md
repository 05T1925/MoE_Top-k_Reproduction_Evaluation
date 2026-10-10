# BMW16 S27：实际 DCF party-key 与联合视图安全裁决

日期：2026-10-09
身份：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL**
审查对象：S26 最终提交 `2818bce20f30719eaccf0bc5df586cce4fe78c84`；运行时修复提交 `ded433d636a3df929380bdce0ff22f4ce6121453`。

## 1. 决策摘要

本次没有发现针对完整 `M2UC v1` 序列化 party key 的区分攻击，也没有发现确定的运行时 DCF 错误。S24 发现的“把 VFSS common correction word 逐字段当成 BGI15 两份独立 correction”问题仍成立，但它只否定那条字段映射证明，不是完整 key 攻击。

新增一个源代码专属的理想扩展器证明：对 `groupSize=1` 的当前 `keyGenDCF/evalDCF`，若把隐藏方逐层扩展建模为安全的 126-bit→382-bit PRG，并把 cryptoTools AES-CTR 根流建模为安全 PRG，则单方得到的**完整序列化 key**可以由与阈值和 payload 无关的分布模拟。证明包含 root、每层完整 CW、所有 `v_i` 和终端 `g`，而不是只看两方输出和。

这个结果是**条件性来源专属证明**，不把 BGI15 Theorem 6 直接转移到 VFSS 字段。实际 AES 的 126-bit 限制子密钥族及 OS 随机源仍是明列的密码学假设，尚未由独立接收者复核。整池自适应 DCF view 和实际 shuffle/output 组合也给出逐项条件性模拟；采样的实际 SHA-256 counter 流仅在随机预言机模型有自然的理想化解释，标准模型中不声称 S16 的理想无放回统计界自动适用。

不改运行时代码，不建立 secure alias，不提升为正式性能候选。当前身份和默认关闭状态不变。

## 2. 基线、来源类别与证据

`origin/main` 为 `c3926c68fd14f270faa8b55234311071947fa080`，它是 S26 基线提交的祖先；S27 worktree 从用户指定的 S26 HEAD 建立，未改写历史。S26 尚未获得其要求的异会话独立签收；S27 本记录也不声称自己可替代另一接收者的独立安全评审。

证据分层如下：

| 类别 | 本文使用内容 |
|---|---|
| PAPER | BGI15 Definition 2 给出单方 key indistinguishability 游戏；Algorithm 5/6 与 Theorem 6 只证明论文自己的 Gen/Eval。论文没有直接证明本仓库的字段布局。 |
| SOURCE | 本仓库 `dcf.cpp`、`keypack.h`、uCMP、party、shuffle 与发材代码的实际字段和方程。 |
| PROJECT_DERIVED | 以下 126→382 扩展器 key-privacy 证明、整池 hybrid、shuffle 等式模拟、leakage 函数和组合接口。 |
| UNPROVEN / ASSUMPTION | AES-126 扩展器 PRG、AES-CTR 根流、OS 随机源、公开 transcript 的项目授权有效范围、跨阶段组合由独立评审接收。 |

论文对象为 Boyle–Gilboa–Ishai, *Function Secret Sharing*, EUROCRYPT 2015，Definition 2（PDF 页 7）、Algorithm 5/6（PDF 页 18–19）和 Theorem 6（PDF 页 20）。可从[作者论文 PDF](https://tzin.bgu.ac.il/~gilboan/publications/DPF-Extended.pdf)复核。Theorem 6 的结论依赖论文所定义的构造和 PRG，不因两构造实现同一 `β·[x<α]` 函数而自动转移。

本隔离 worktree 没有 `Papers/` 目录；未复制、移动或修改本地论文 PDF。

## 3. 实际参数、完整 party key 与函数

当前活动接口固定 `Bin=b∈[34,53]`、`Bout=64`、`groupSize=1`。KeyGen 的阈值是 `α∈Z/(2^b)`，payload 是 `β∈Z/(2^64)`；输入按 MSB-first 处理。`greaterThan=false` 的 `keyGenDCF` 与 `evalDCF` 两份输出满足：

\[
F_{α,β}(x)=β\,[x<α]\pmod {2^{64}}.
\]

P0、P1 各自收到 `M2UC v1`，其中包含公开头 `(party,b,64,1)`、数组长度 `(b+1,1,b)`、自己的 root block `k[0]`、两方相同的 `CW[1..b]`、同一 `g[0]` 和同一 `v[0..b−1]`。端点 alpha 和 payload 不作为明文字段序列化。单 key 游戏将整个字节串交给对手；`eval_strict_lt` 在相同 DCF key 上执行两次 Eval，并输出由两次 Eval 导出的一个 ring share。对手可对其 key 任意调用 Eval，视为拿到完整 key 后的确定性后处理。

对应源码：

| 源码 | 实际行为 |
|---|---|
| `VFSS/ext/FSS/dcf.cpp:152–185` | KeyGen 从 `FSSConfig::prngs[tid]` 取两个 root block；将 root control bit 设为互补；分别作为两 party 的 `k[0]`。 |
| `dcf.cpp:188–216` | threshold MSB-first；节点扩展输入为 `s & ~3`；AES 输入为 block `0,1,2,3`；提取 root control 和符号。 |
| `dcf.cpp:219–242` | 取左右 value 输出的低 64 位，生成一个 `v_i` 并更新 KeyGen 累计量。 |
| `dcf.cpp:244–266` | 每层生成一份共享 CW：高 126 seed correction bits 与两个 control correction bits；只沿目标 `keep` 子节点继续。 |
| `dcf.cpp:269–284` | 计算同一终端 `g` 并写入两份 key。 |
| `dcf.cpp:87–148,294–335` | Eval 用输入位选分支、应用 CW、逐层累计 `v_i`，最后加终端项；SERVER1 取负号。 |
| `VFSS/ext/FSS/include/FSS/keypack.h:8–23` | 完整 `DCFKeyPack` 字段为 `Bin,Bout,groupSize,k*,g*,v*`。 |
| `VFSS/src/moe_topk/protocol_i_ucmp.cpp:11–15` | `ProtocolIUcmpPartyMaterial` 复制全 key，M2UC v1 校验长度并序列化全部字段。 |
| `protocol_i_ucmp.cpp:13` | 同一个 key 以 `x=(zl−zr) mod 2^b`、`y=(x−2^(b−1)) mod 2^b` 执行两次 DCF Eval，再按 party 符号还原严格比较 share。 |

完整 wire ABI 顺序为 `"M2UC" || version=1 || party || Bin || Bout=64 || groupSize=1 || u64be(Bin+1) || u64be(1) || u64be(Bin) || raw16(k[0..Bin]) || u64be(g[0]) || u64be(v[0..Bin−1])`。三段长度均是 64-bit big-endian；`block` 是 16-byte 原始内存复制；`g/v` 是 64-bit big-endian。总长度 `57+24b` 字节；`b=34` 时 873 bytes，`b=53` 时 1,329 bytes。block 因此还依赖 host block 字节序/布局。M2UC 自身不包含 session、stage、task、round、edge 或 fingerprint；这些外部绑定由上层 slot manifest / bundle AAD 提供，且仍依赖可信 KeyGen 把函数参数与 slot 关联，AEAD 标签本身不证明该数学关系。当前接受范围不声称跨 ABI 可移植；头部和长度校验仅检查编码形状，不认证 DCF 的数学关系。

逐变量映射（`V` 与所有输出累加在 `Z/(2^64)` 中）：令 `s_{j,q}` 是 party `j` 的当前状态，`t_{j,q}=lsb(s_{j,q})`，阈值第 `i` 位按 MSB-first 记为 `a_i`。源码的四个 AES 输入/输出对应关系是

| 变量 | `dcf.cpp` 源码等式 | 含义 |
|---|---|---|
| 左右 child/value | `si[j][L]=AES_{s&~3}(0)`，`si[j][R]=AES_{s&~3}(1)`；`vi[j][L]=convert_64(AES(2))`，`vi[j][R]=convert_64(AES(3))` | `pt={0,1,2,3}`，`convert_64` 取 block 的前 8 个 ABI 字节；扩展只用 state 的高 126 位作为 AES key。 |
| 目标分支 | `keep=a_i`，`lose=keep xor 1` | `idx >> (Bin−1−i)`；两方 KeyGen 只沿 `keep` 继续。 |
| 共同 CW | `scw=(si[0][lose] xor si[1][lose]) & ~3`；`d_L=(lsb(si[0][L]) xor lsb(si[1][L])) xor keep xor 1`；`d_R=(lsb(si[0][R]) xor lsb(si[1][R])) xor keep`；`CW=(scw | d_L<<1 | d_R)` | 完整 128-bit CW；Eval 对选择位 `x_i` 读取 `d_L` 或 `d_R`。 |
| value word | `ε=(-1)^{t_{1,q}}`；`v_i=ε(−A_i−V_{0,lose}+V_{1,lose}+[keep=1]β)` | 该路径 `greaterThan=false`；代码以 `GroupElement` 无符号环加减实现模 `2^64`。 |
| accumulator / next state | `A_{i+1}=A_i−V_{1,keep}+V_{0,keep}+εv_i`；`s'_{j}=si[j][keep] xor (t_{j,q} ? (scw xor d_keep) : 0)` | `d_keep` 是该 CW 中与 `keep` 对应的控制校正位。 |
| terminal word | `g=s_{1,b}−s_{0,b}−A_b`；若 `lsb(s_{1,b})=1` 则 `g=−g` | 同一 `g` 序列化在两份 party key 中。完整 key 隐私中，`g` 的均匀性由最后隐藏的 `V_keep` 掩蔽 `A_b`，不是假定被 `~3` 清位后的 terminal seed 转成 64-bit 均匀值。 |
| Eval | 输入位 `x_i` 同样 MSB-first；`s' = AES_child(x_i) xor (t_previous ? (scw xor d_{x_i}) : 0)`；`v_share += sign_j·(V_{x_i}+t_previous·v_i)`；末端 `sign_1=−1, sign_0=+1` | `traversePathDCF`/`traverseOneDCF`，末端再加 `convert_64(s_final & ~3)+lsb(s_final)·g`。 |

此映射固定了源代码在 `pt={0,1,2,3}`、左右分支和 `CW` bit 位置上的具体含义，不把 `scw` 与 BGI15 的两份 target correction 混同。`keyGenDCF(..., greaterThan=false)` 的两方输出之和是 `D_α(t)=1[t<α]·β mod 2^64`。uCMP 对同一 key 两次求值：`x=(z_l−z_r) mod 2^b`、`y=(x−h) mod 2^b`，其中 `h=2^{b−1}`；源码合并式为 `1−D_α(y)+D_α(x)−1[y≥h]`。若 `z_i=A_i+m_i` 且 KeyGen 的 `α=m_l−m_r`，该式恢复 `1[A_l<A_r]` 的应用前提是 `A_l−A_r` 的有符号代表落在 `(-h,h)`，以免差值环绕造成歧义。raw adapter 的 32-bit 操作数差满足此界；Select priority 的最大值是 `2^{32+index_bits}−1 < h`，所以任意两真实 priority 的差满足此界，代码给所有真实元素加相同 `low_pad+1` 不改变差值。dummy 边由公开类别直接给出，不调用 uCMP。该 uCMP 转换和 DCF key privacy 是两个独立论证层次。

## 4. 当前源码的单 key 函数隐私引理

### 4.1 理想扩展器

令隐藏 seed 的有效状态为 `S∈{0,1}^126`，控制位为 `t∈{0,1}`。bit 1 被 `s&~3` 丢弃，不进入 AES key、控制位或 64-bit `convert` 输入。定义源代码实际需要的扩展器：

\[
G(S)=(S_L,t_L,S_R,t_R,V_L,V_R),
\]

其中两个子状态各为 126-bit seed 加 1-bit control，`V_L,V_R∈Z/(2^{64})`。因此相关输出长 382 bits。理想实验令每次新状态的这 382 bits 独立均匀；若种子重复，同一确定性扩展表项必须复用，并把重复事件单列为 collision 项。

考虑任意 `α`、`β` 和任一被腐化 party `c`。对手知道 `c` 的完整 root 和所有 serialized fields，也可以重算该方 AES。KeyGen 另一方的 root 高 126 位在给定已知 root/control 后均匀。归纳假设第 i 层隐藏的目标路径状态在此前可见字段条件下仍均匀。

设本层 `keep=α_i`、`lose=1−α_i`、`A_i=v_alpha`、`ε=(-1)^{t_1}`。源代码的 `greaterThan=false` 更新等价于：

\[
v_i=ε(-A_i-V_{0,lose}+V_{1,lose}+ [keep=1]β),
\]
\[
A_{i+1}=A_i-V_{1,keep}+V_{0,keep}+εv_i
       =(V_{0,keep}-V_{0,lose})-(V_{1,keep}-V_{1,lose})+[keep=1]β.
\]

给定腐化方在该节点的展开结果，隐藏方 `lose` seed 的高 126 位一次性掩蔽 CW 高位；两个隐藏子状态的控制位分别掩蔽 `d_L,d_R`；隐藏 `V_lose` 以系数 `±1` 掩蔽 `v_i`。三个来源在理想 `G` 中独立，所以本层完整 `CW_i∈{0,1}^{128}` 与 `v_i∈Z/(2^64)` 条件均匀，且与 `keep`、`β` 及历史 serialized prefix 无关。

隐藏 `keep` seed 是另一独立均匀子状态；对它按 control 是否触发 XOR correction，只做已知偏移，因此仍均匀并闭合下一层归纳。隐藏 `keep` value 进入 `A_{i+1}`，不进入当前 `v_i`。最后一层后，该隐藏 `keep` value 以系数 `±1` 进入 `g`，而不在其他 serialized field 中出现；所以终端 `g` 均匀并与 root、全部 CW 和全部 v 字段独立。这里不能用终端 seed scalar 是 64-bit uniform 作理由：`s&~3` 清掉两个低控制位，最终 `convert(64,1,...)` 并非从 64 个未受限 seed bits 取值。真正遮住 g 的是最后隐藏 `keep` value。

由此，理想 `G` 下任一 party 的完整序列化 key 分布为：固定公开 M2UC 头；一个均匀 128-bit root；`b` 个相互独立均匀 128-bit CW；`b` 个相互独立均匀 64-bit v；一个均匀 64-bit g。此分布不含 `α` 或 `β`。两方 key 共用 CW/v/g 并不影响单方游戏，但不能误称两份 key 相互独立。

### 4.2 计算假设与优势项

源码用 `AES_{s&~3}(0,1,2,3)`。准确的原语假设是：高 126-bit seed 经零低位嵌入 AES key 后，投影得到的 `G:{0,1}^{126}→{0,1}^{382}` 是对带有独立 auxiliary input 的安全 PRG。若用 PRP 表达，需**明确假设** AES 对 key family `K=encode(S||00)` 的 4-query restricted-key PRP 安全，再加 4 点 PRP/随机函数切换项；一般均匀 128-bit AES PRP 定义本身不会自动推出这个受限子族结论。4 点切换损失可用 `6/2^128` 上界，AES 优势保留符号，不报具体安全位数。

根随机另外由 `seed_fss_once` 从 `getrandom` 为 256 个 cryptoTools PRNG 分配种子；`PRNG::refillBuffer` 是 AES counter mode。要求 OS CSPRNG 与该 multi-stream AES-CTR 输出伪随机。对单 key 用两个 root block，对整池按线程 `q_t=2N_t` 个 block 记账。不能把该根流假设与节点 `G` 混为一个“FSS PRG”。

对一份单 key、宽度 b 的条件优势写为：

\[
Adv_{key}(A)\le Adv_{root}(2)+b\,Adv_G^{aux}+\binom{2b}{2}/2^{126}.
\]

最后一项是双方/各层有效 node-seed 碰撞的保守 birthday 项；更精确界可按实际状态数收紧。若只给单次 KeyGen 且无 collision conditioning，可将它保留为失败事件。此公式不声称具体 AES 或 OS 数值。

可审查的单 key 游戏顺序为：`H0` 是实际 cryptoTools root stream、实际 AES child expansion 和完整 M2UC 序列化；`H1` 用独立均匀 128-bit blocks 替换该 KeyGen 消耗的两个 root blocks，差异由 `Adv_root(2)` 界定；`H2,i` 按阈值路径从根到叶，依次把隐藏 party 的第 `i` 次 126→382 扩展替换为独立均匀 tape entry。每个 `H2,i−1 → H2,i` 的 reduction 将其 PRG 挑战输出直接填入该节点的 `si/vi`，再按真实的 `CW/v_i/next-state` 公式生成后续字节。挑战输入 seed 在此前替换之后均匀，且在该步扩展前独立于 reduction 的 auxiliary input；auxiliary 可包含固定 `(α,β)`、腐化方 root、完整 visible-party 状态、所有其他 key 的生成带和此前序列化 prefix。以 `G126` 的 auxiliary-input PRG 假设界定每步差异，累计至多 `b·Adv_G^aux`。`H3` 是理想扩展分布；排除跨层 seed 重复后，CW 与 v 的一次性 pad 推导给出完整 key 的独立均匀分布，重复事件加 birthday 项。任意 Eval、两次 uCMP Eval 与 keypack 解码都是该完整 key 上的确定性后处理。

`H2` 的归纳断言必须由接收者核对：固定阈值路径后，下一隐藏 keep state 的高 126 位是 raw keep seed 与此前确定偏移 `t_previous·scw` 的 XOR；raw keep seed 是当层理想扩展中的新独立坐标，因此条件于可见 prefix 仍均匀。隐藏 losing seed 不进入后续状态；隐藏 `V_lose` 以单位系数只进入当前 `v_i`。需要特别区分 control bit：CW 的 `d_L/d_R` 与腐化方可计算的 visible child control 共同暴露对应的 hidden child control；本证明不声称这些 1-bit control 保密。keep control 会随 keep child state 进入下一层，但它已可由当前 CW 条件化，AES 扩展的 key 只使用高 126 seed。终端 `g` 的遮蔽仍来自最后隐藏 `V_keep`，不是 control 或低位清零后的 seed。归纳的“一次性 pad”来自同一个 ideal expansion 输出内彼此独立的 seed/value 坐标，而不是假设 serialized correction words 本身独立。实际 PRG 替换后，上述理想坐标独立性成立；在 `H0` 中它只以 hybrid 优势成立。

### 4.3 论文对应边界

BGI15 Definition 2 是“完整单方 key”的比较游戏，适用于把本节的 `M2UC v1` 字节串作为挑战对象；任意 Eval 是 key 的后处理。BGI15 Theorem 6 只适用于论文 Alg. 5/6 的 Gen/Eval。S24 已给出 common `scw` 与论文独立 target correction 的直接变量对应会失败；本节绕开变量重命名，对 VFSS 自身递推证明，不宣称 VFSS 逐字段满足论文算法。

## 5. 整池、同一 key 双 Eval 与自适应图

令 `n` 为逻辑输入规模、`p=padded_n`、`C=n(n−1)/2`、`b=comparison_bits`。实际材料生成 `N_DCF=2p+9C` 次 uCMP KeyGen：`2p` raw carry/sign，两个 Select task 共 `2×4×C`，membership `C`。shuffle-only forward/inverse 不生成 DCF key。`offline_material_slots_per_party=2p+2+9C` 另含两个 shuffle slot，不能把 slot 数写成 DCF KeyGen 数。

若分方 RNG 流相互独立，且节点种子无跨 key 重复，则理想单方 key pool 是上述单 key simulator 的直积，与整个 correlated threshold vector 无关。阈值可以因共用 shuffle `r` 而相关：逐 key 引理对**任意固定向量**成立，其他 key 在混合中以已知实际阈值运行，因此该相关性不破坏 hybrid。未访问 key 同样包含在完整初始 party view 中并由 simulator 采样；它们不因“在线没读”而被忽略。

raw-score adapter 的 `2p` 个 carry/sign slot 也包含在这个 pool hybrid。其 carry/sign masked operand message 在 `L` 中；模拟器用腐化方 raw share、对应的本地方 key 和这些公开 operand 重算本地 `carry/sign` share，再按真实 adapter 等式得到本地 `lift/stable-key share` 和第二阶段消息。carry/sign bit 本身并未公开；这些结果只是本地方程后处理。因此 simulator 仍从 ideal `F_L` 自行获得公开 operand，而不从真实执行复制 transcript。

每个 uCMP slot 在 `ProtocolIUcmpPartyMaterial::eval_strict_lt` 中调用同一 party DCF key 两次；两次 Eval 不是两份独立 key。完整 key 已进入 view，所以 evaluator 的两次输出是本地方可由 key 和公开 operands 重算的确定性值。两方打开的比较 bit `c` 与本地 share `s` 给出的 peer share 必为 `c−s mod 2^64`，无需为 peer uCMP share 引入额外随机变量。下一轮图只由先前打开的 bit 和公开 sampler coin 决定；模拟器先从 ideal F 收到 L，再按 L 的 slot/order 执行。

整池 DCF 项的保守 hybrid 界（每个腐化方单独计算）为：

\[
Adv_{pool}\le Adv_{root}((q_t)_{t=0}^{255})+N b\,Adv_G^{aux}+\binom{2Nb}{2}/2^{126},
\quad N=2p+9C.
\]

其中每个活动 PRNG stream 的 `q_t` 至少是该线程实际消费的 128-bit root blocks；本候选新 T 进程串行生成时总计 `Σq_t=2N`，通常只由 `tid=0` 消费。若实际 T 在额外 OpenMP worker/更早 KeyGen 上消费过 PRNG，必须把它们计入 `q_t`，不能仍用本式较小参数。该界依赖独立 root stream 和上述 node PRG 假设；协议逐槽 fresh KeyGen、slot tuple 含 session/stage/task/round/canonical endpoints，轮内方向规范化/缓存不会把 key 跨轮或跨 task 复用。

模拟顺序避免循环假设：理想功能 `F_L` 读取 raw input，在内部生成理想随机 coin、hidden permutation 与 masks，运行项目算法，并**自己计算**要公开的 masked operands、匿名边/比较 bit、pivot/U/V/W、selected handle、key-ID/访问顺序、abort 与消息长度，形成 L。Simulator 只获得这份由 ideal `F_L` 产生的 L、腐化方 raw input share 和 ideal output share；不把真实执行的公开 transcript 原样交给 simulator。它先抽样完整 party key pool，再对 L 中的每个 operand 运行本地方程。对每个已打开比较 bit `c`，它将 peer Eval share 设为 `c−s`。这覆盖未使用 key、两 task 共用的匿名 handle 以及 membership slots。

## 6. 实际 shuffle/inverse 与输出 share 模拟

本节是项目 shuffle 构造的代数视图证明，不从 Protocol I 论文定理借安全结论。所有向量运算在各 record field 对应的环上进行。dealer 选择 `π,σ0,σ1`，令 `τ0=π∘σ1⁻¹`、`τ1=π∘σ0⁻¹`；随机 `a0,a1,h,r0,r1`，`e0=−τ0(a1)−h`、`e1=−τ1(a0)+h`。party b 发 `m_b=σ_b(x_b)+a_b`，再发 `q_b=τ_b(m_{1−b})+e_b+r_b`。

直接代入得到：

\[
q_0^{pre-r}=πx_1-h,\quad q_1^{pre-r}=πx_0+h,\quad
z=q_0+q_1=π(x_0+x_1)+r_0+r_1.
\]

inverse stage 使用独立的新掩码与 `π⁻¹`，对 membership additive shares `μ0,μ1` 得：

\[
o_0=π^{-1}μ_1-h',\quad o_1=π^{-1}μ_0+h',\quad
o_0+o_1=π^{-1}(μ_0+μ_1).
\]

对任一 party，条件于真实 `π`，其本地 `σ_b` 独立均匀；`τ_b` 因另一方独立均匀 σ 而独立均匀，所以该本地 `(σ_b,τ_b)` 的分布不依赖 π。forward 与 inverse 使用新的独立 σ 和 masks，同一 π 仍不在单方本地置换因子分布中暴露。`e_b` 由独立均匀 h 掩蔽，本地 `a_b,r_b` 独立均匀。

对应 simulator 对 corrupt P0/P1 均可。forward 阶段从相同局部分布抽本地 `σ/τ/a/e/r`，由腐化输入份额计算本方 `m`；对方 round‑1 消息 `m_peer` 均匀，因为对方的 `a_peer` 是独立均匀向量。由 `τ_b(m_peer)+e_b` 算本方中间 share 和 `q_b`，再置 peer round‑2 消息为 `z−q_b`。`e_b` 在本方其余材料及 `m_peer` 条件下均匀，因为 `h` 是新鲜均匀向量；peer `r_share` 同样使 `z` 成为理想功能生成的 masked disclosure。

inverse 阶段不能先独立抽 `e_b` 和 `m_peer` 再声称输出 share 条件分布正确，因为本方完整中间 share `w_b=τ_b(m_peer)+e_b` 的低位就是输出 XOR share。给定 ideal output share `y_b`，逐 record 采样如下：先均匀抽 `m_peer`；再均匀抽 `w_b`，其中 word0 的低 bit 固定为 `y_b`、其余 `comparison_bits−1` 位均匀，word1/word2 全宽均匀；最后设 `e_b=w_b−τ_b(m_peer)`，本方 round‑2 消息为 `q_b=w_b+r_b`，peer 消息为 `z−q_b`。真实分布中 `m_peer` 均匀，`e_b` 在该条件下均匀，且给定 `y_b` 后二者恰满足同一低位关系；反向构造因此复现真实 view 的条件分布，同时不需要 `π`、诚实方输入或完整明文 mask。`public_z` 的随机和由 peer `r_share` 掩蔽，条件于它时 peer round‑2 消息也由和式唯一决定。此条件分布是本组合证明中必须由独立接收者逐式核验的关键步骤。

两份 Select 共用同一 `π`；模拟器不公开 `π`，而使用 ideal F 自己生成的匿名 transcript 和 party output share。forward/inverse 的本地材料均须与对应的 round transcript 一起按上述条件分布生成，不能把完整理想 permutation 直接交给模拟器。

`public_masked` 的 inverse round2 向量当前在 party 内被构造，即使上层只取 `.shuffled_share`，两份 round2 消息仍使其可计算；因此将 forward 和 inverse 两个 `public_z` 都列入 L。每个向量带新鲜均匀 `r0+r1`，作为公开 masked value 处理。

以上 shuffle 模拟依赖 ideal F 中的随机置换与均匀 masks、实际 `getrandom`/Fisher–Yates rejection 采样的计算随机性，以及腐化方不与 T 合谋。它证明了本地置换因子边际加 transcript 的构造路线；独立接收者仍需逐项审查“条件于 output share”的 inverse transcript 反解。有限图枚举没有被当作此一般引理。

## 7. Leakage L、状态与异常边界

按用户本轮确认的项目泄露口径，拟定 `L` 包括：公开参数/session/channel；raw adapter 的 carry/sign masked operand 向量；forward 与 inverse public masked vectors；匿名端点、比较 bit、图轮次、sample/pivot/U/V/W 位置、selected anonymous handle；实际 material ID/slot/order、逻辑比较数、消息阶段/长度；两方统一的算法 abort code。共同 coin seed 由两方 contribution XOR 得到，party 可计算，故也属于实际公开随机 transcript。若 abort 前图已透露阶段/原因，L 同样包含那段 transcript；不声称失败类别在统计上完全不可区分。

L 不含 raw score、`original_index`、真实 index↔匿名 handle 对应、真实 rank、selected stable key、单方之外的完整输出 mask。每个 party view 另含自己的 raw input share、自己的整个材料 bundle/key pool（已用和未用）、shuffle local factors、self/peer frame 明文、每次本地 Eval share、最终本地 output share、退出/错误状态。T 有自己的全部离线随机量与材料并可信非合谋；T 不进入 online view。

正常成功与概率 abort 属于理想执行可模拟 transcript。材料损坏、TLS/peer 断连、OS 错误和不变量违反不放进统计 abort：它们是工程错误；只承诺可观察的 local/peer status，不在无 ACK 时宣称双方一致。party-node 的详细本地 reason 不作为对端字段；日志权限和部署管理员越权不在半诚实模型内。

## 8. 抽样保证

S16 的独立复核对全部可达偶数 M（n≤256 的公式域 M≤512）逐个检查既定整数参数，两个 Select task 用 union bound、无需独立。S16 报告的理想均匀无放回界 `1.3105804606611235e−11` 是**理想采样数学结果**，不是 SHA-256 counter 实现的标准模型概率证明，也不是 BMW16 Theorem 8 的数值。

实际 party 交换两个 32-byte `getrandom` contribution 并 XOR，再对 `(seed,task,domain,counter)` 做 SHA-256 并以 rejection sampling 得无放回样本。semi-honest 且至少一个贡献来自诚实 OS CSPRNG 时，xor seed 可建模为均匀；在随机预言机模型下，新的 domain/counter 输入哈希值均匀独立，S16 理想界可条件性套用。标准模型中 public seed 的裸 SHA-256 counter 不是带秘密 key 的 PRF；S16 数学式不能单独证明实现的全样本失败概率。有限自然失败样本数也不填补该缺口。S27 未改变 sampler，未运行自然失败压力矩阵。

## 9. 门禁裁决

| 门 | 状态 | 理由 |
|---|---|---|
| `DCF_SINGLE_KEY_PRIVACY` | `CONDITIONAL` | 完整 M2UC key 的 source-specific 理想 G 证明；计算桥依赖 restricted-key 126-bit PRG 与 AES-CTR root stream 假设，未独立接收。 |
| `ADAPTIVE_FULL_POOL_VIEW` | `CONDITIONAL` | 全池 vector hybrid 处理 correlated alpha、unused keys、同 slot 两次 Eval 和 adaptive L 查询；依赖单 key 引理、根随机流独立性与 collision 项。 |
| `SHUFFLE_OUTPUT_COMPOSITION` | `CONDITIONAL` | 给出 S27 shuffle local-factor 分布与 forward/inverse 等式 simulator；OS random masks、完整 L、输出 share 反解仍待外部复核。 |
| `SAMPLER_ABORT_GUARANTEE` | `CONDITIONAL` | 理想无放回界适用于 S16 参数；实际 SHA-256 counter 仅给 ROM 条件，标准模型为 UNPROVEN。 |
| `SECURE_ALIAS_READY` | `UNPROVEN` | 未有另一接收者审查此证明；无条件 DCF/整池/联合视图证据也未通过；不建 secure alias。 |
| `EXPERIMENTAL_FUNCTIONAL_REGRESSION` | `CONDITIONAL` | S26 runtime 原样保留；本窗口重建并运行定向 DCF/uCMP/shuffle/party 测试与 TEST_ONLY toy model，不重跑独立进程 E2E。历史 S26 E2E 不冒充 S27 新 E2E。 |

## 10. 需要独立接收者裁决的最小清单

1. 验证 `G126` 382-bit projection 及其 auxiliary-input PRG hybrid，尤其隐藏 keep value 对终端 `g` 的独立一次性掩蔽。
2. 验证 `cryptoTools::PRNG` 的 AES-CTR root call budget 与 per-thread `q_t`；在并发/预热环境下补齐任何先于 candidate KeyGen 的 FSS PRNG 消耗。
3. 复核 `N_DCF=2p+9C`、key IDs、每 key 两次 DCF Eval、未用 key 与完整 pool hybrid。
4. 对照真实 adapter frame 和 inverse party 内 `public_masked` 确认 L 字段无遗漏；逐方检查选中匿名 handle 与 local shuffle view、output XOR share 的联合分布。
5. 复核 getrandom、受限本地威胁模型、abort 可见阶段和随机预言机抽样界是否满足项目目标。
6. 通过后仍需另行完成 S26 独立功能接收、生产凭据/部署审查和正式 V4 性能门；S27 不关闭这些门。

若项目要求不接受 `AES-126` 受限子族假设，最小新实现方向是另立 `M2UC v2` DCF backend：以有版本的 `KDF(domain || 126-bit state)` 派生完整 128-bit AES key，定义并测试新 G 接口后再对照同一 source-specific lemma。该方案会改 keypack ABI、全部 DCF 材料、离线字节和旧/新识别；S27 没有暗中替换当前材料或把未实现版本称为修复。
