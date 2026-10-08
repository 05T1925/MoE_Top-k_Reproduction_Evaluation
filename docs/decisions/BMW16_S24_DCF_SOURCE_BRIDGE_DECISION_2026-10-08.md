# BMW16 S24：VFSS DCF 到 BGI15 的源码桥接裁决

日期：2026-10-08<br>
起始提交：S23 文档提交 `7b781168161a488b156353acf1623100fe59b1f7`<br>
起始 `origin/main` / merge-base：`c3926c68fd14f270faa8b55234311071947fa080`<br>
分支：`codex/m6b-i-bmw16-s24`<br>
身份：**Protocol I + BMW16-derived Select + DCF / PROJECT_DERIVED / EXPERIMENTAL**

## 裁决

1. 源码 DCF 的函数语义可逐式解释为 `f^<_{a,g}(x)=g·[x<a]`，输出组是 `Z/(2^64)`。uCMP 在一份 DCF key 上做两次 Eval，再进行环内修正；功能 conformance 与安全仍是两类证据。
2. 本轮没有证明当前 `keyGenDCF/evalDCF` 完整 party-key 分布与 BGI15 Algorithm 5/6 相同。首个自然逐字段映射已被一个精确分布事件否定：BGI15 为 target child 独立均匀抽两份 seed correction；VFSS 只存一个 common `scw`，由互补的前态 control bits 门控，映射出的两份 correction 总有一份为零。该结果否定**直接变量对应**，不是对 VFSS serialized key 的攻击，也不排除另一重编码或 source-specific 证明。
3. `DCFCodeEquivalentToBGI15GenEval` 判 **NO-GO（不能把 BGI15 Theorem 6 直接贴到当前 ABI）**。当前 serialized key 的单 key 函数隐私为 **UNPROVEN**，不据此宣称实现不安全。
4. S23 的 `N_DCF·ε_DCF` 整池 hybrid 仅保留为条件推导：还需实际 ABI 的单 key函数隐私、根/节点随机流替换，以及相同许可泄露类上的自适应 transcript simulator。
5. 保留 opt-in VFSS 三进程功能路径；不改运行时代码和材料 ABI，不建 secure alias，不运行 n=1000 或正式性能矩阵。

## 论文来源与页码

Boyle–Gilboa–Ishai, *Function Secret Sharing*, EUROCRYPT 2015，官方 PDF：<https://www.iacr.org/archive/eurocrypt2015/90560300/90560300.pdf>。

| 对象 | PDF 页码（一基；印刷页） | 本报告依赖 |
| --- | --- | --- |
| Definition 2 | p.7；印刷 p.7 | 完整 party key 的函数隐私游戏；同域函数挑战 |
| Algorithm 5 `Gen_<` | p.19；印刷 p.19 | 比较 DCF 的初始化、PRG 分支、seed/control/value correction |
| Algorithm 6 `Eval_<` | pp.19–20；印刷 pp.19–20 | MSB-first 路径、状态校正与 `(-1)^β` 输出 |
| Proposition 1 / Theorem 6 | p.20；印刷 p.20 | 论文构造的正确性和两 key FSS 函数隐私 |

原文算法使用 `G:{0,1}^λ→{0,1}^{2λ+2log|G|+2}`。Definition 2/Theorem 6 针对论文所定义的 Gen/Eval；相同输出函数不自动转移完整 key 隐私。

## 源码功能递推与可证明等式

源码位置：`VFSS/ext/FSS/dcf.cpp`：`convert` 60–85、`traverseOneDCF` 87–126、`traversePathDCF` 130–148、`keyGenDCF` 152–284、`evalDCF` 294–335；uCMP 在 `VFSS/src/moe_topk/protocol_i_ucmp.cpp:5–15`；key wrapper 在 `VFSS/ext/FSS/include/FSS/keypack.h:8–22`。

对 `Bin=b,Bout=64,groupSize=1`，令 `q(z)` 为 `convert(64,1,z)` 读取 block 的首 8 字节并按目标 ABI 解释为 `uint64`。Group 是 `Z/(2^64)`。根 `kβ[0]` 的 AES seed 是 `kβ[0]&~3`，control 是 `lsb(kβ[0])`；`keyGenDCF:183–185` 强制根 control 互补。输入位按 `b−1−i` 读，MSB-first。

每层 `keep=a_i, lose=1−a_i`。两方分别以 seed `stateβ&~3` 将 AES counters `0,1,2,3` 加密为四 blocks；前两块是左右 child block，后两块是左右 value block（`dcf.cpp:188–225`）。seed block 高 126 位是下层 seed，bit 0 是 control，bit 1 被 `~3` 屏蔽；value 是所选 block 的前 64 bits。

`keyGenDCF:244–266` 的 correction 精确为：

```text
scw = (s0_lose XOR s1_lose) AND ~3
dL  = lsb(s0_L XOR s1_L) XOR keep XOR 1
dR  = lsb(s0_R XOR s1_R) XOR keep
CW  = scw OR (dL << 1) OR dR             // k0[i+1] == k1[i+1]
state'β = sβ_keep XOR (Tβ ? (scw XOR d_keep) : 0)
```

故 target-prefix control XOR 为 1、首次离开阈值前缀后 seed/control 收敛。这证明源码具有论文正确性证明中类似的状态不变量形状，尚不说明随机 key 分布等价。

值校正（`dcf.cpp:217–242`）中 `σ=(-1)^t1`，`v_alpha` 初始为 0：

```text
v_i = σ·(-v_alpha - v0_lose + v1_lose + [keep=1]·g) mod 2^64
v_alpha ← v_alpha - v1_keep + v0_keep + σ·v_i mod 2^64
```

Eval 每层两方重构贡献为 `v0_x−v1_x+(t0−t1)·v_i`。target prefix 上 `t0−t1=σ`，故 `v_alpha` 是累计 target-edge 输出。若首次失配发生在 `keep=1`，losing-left edge 的该式加上既有 `v_alpha` 后恰为 `g`；若 `keep=0`，恰为 0。首次失配后两方状态相同，后续层与叶项相消。若输入等于阈值，末端 `g0=q(state1&~3)-q(state0&~3)-v_alpha`，并按 `lsb(state1)` 取符号（`dcf.cpp:269–281`），使叶项恰抵消 `v_alpha`。因此重构输出是 `g·[x<a]`。这是源码代数功能证明，不是 key privacy 证明。

## 逐项对照表

| 内容 | BGI15 | VFSS | 等价边界 |
| --- | --- | --- | --- |
| 函数/输入 | `a,x∈{0,1}^b`；`g` iff `x<a` | 上述 MSB-first DCF recurrence | **函数语义吻合** |
| 根 | Alg.5 初始化多个分支的 S/T/V 状态 | 两个 random blocks；一方低位被改为另一方补位 | 可解释为互补控制，但 key tuple 字节/分布不相同 |
| 分支 PRG | 通用 `G` 输出两 seed、两 control、两组 G 值 | AES keyed by `state&~3`，四固定 counter blocks | 取 `λ=126, |G|=2^64` 时所需输出 `382` bits；要假设受限 126-bit AES-key 族构成安全 PRG |
| seed/control CW | Alg.5 按 target/lose 分支和 party 采样/约束 | 一个 common `scw` + 两 control CW bit，通过 `Tβ` 门控 | 不存在直接采样变量一一对应，见下节 |
| value CW | Alg.5 的分支/party value correction | common `v[i]`；Eval 加 `Tβ·v[i]` 并乘 party sign | 功能递推能证明；论文 `cv` 分布仍需重编码 |
| terminal output | Alg.6 V 状态累积后输出 | b 层后增加 `q(leaf)+T·g0` | 需要将末端项编码进 BGI V/CV；本轮没有构造此变换 |
| party sign / group | Alg.6 用 `(-1)^β`；加法 Abelian group | SERVER0 `+`、SERVER1 `−`；`uint64` 模加 | 输出解码对应 `Z/(2^64)`，不是 XOR |
| uCMP wrapper | 非 BGI15 | `α=(ml−mr) mod2^b`；`x=(zl−zr) mod2^b`，`y=x−2^(b−1)`；同 key Eval 两次；P1 对 `y≥half` 加 1；返回 P0=`1−ge`、P1=`−ge` | `PROJECT_DERIVED`；合法 mask/range 下重构 `[left<right]`，由 conformance 测试覆盖 |
| 参数/完整 key | 定理任意有限 Abelian `G` | b=34..53、Bout=64、groupSize=1、payload=1；`M2UC/version/party/b/64/1`、3 长度、`k[b+1]` blocks、`g[1]`、`v[b]` | 长度 `57+24b` bytes，即 873..1329；block 原始 memcpy，数值字段 big-endian；安全游戏必须覆盖完整 wrapper key |

`ProtocolIUcmpMaterial` 每个 comparator slot 是一次 `keyGenDCF` 产生的一对 party keys；每方在两个输入点 Eval 同一 key 两次，不是两份独立 DCF key。

## 第一个实际分布断点

BGI15 Alg.5 PDF p.19 line 7 对 target child 独立均匀抽取 `c_s^{0,target},c_s^{1,target}∈{0,1}^λ`。最直接映射到 VFSS 的 target-path seed correction 是：

```text
c_s^{β,target} = Tβ·scw,   T0 XOR T1=1.
```

因此事件 `c_s^{0,target}=0 OR c_s^{1,target}=0` 在 VFSS 变量映射下概率为 1；在 BGI 的两个独立均匀变量下概率为 `2^(1−λ)−2^(−2λ)`。toy `λ=4` 差值为 `225/256`。这是**该内部变量直接识别**的精确反例。VFSS pack 压缩存储 `scw` 而没有显式 `c_s` pair，故这不是对实际 serialized key 的区分攻击；要继续引用 Theorem 6，仍需给出另一分布保持重编码或当前格式的独立函数隐私证明。

BGI party key 初始 tuple 包含两分支 S/T/V，且每层含两份 `CW^β`；VFSS 格式是单 root + common packed CW + common `g/v`。字段名相似和重构正确均不能代替完整分布变换。

### 最小后续修正方向与材料代价

1. 优先尝试对完整 `ProtocolIUcmpPartyMaterial` 建 source-specific single-key function-privacy 定理，覆盖 `k/g/v` 全字段和自适应公开输出；或给出从该 key 到 BGI key 的可验证分布保持编码。
2. 若失败，独立实现 versioned BGI15 `Gen_<`/`Eval_<` pack、stream writer/store 与 uCMP adapter，不覆盖旧 ABI。对 `λ=126, log|G|=64`，Alg.5 每方初始字段为 `382` bits，每内部层两份 correction vectors 共 `764` bits；tight bit-packed 字段下界 `382+764(b−1)` bits/party，b=34/53 向上取整为 3,200/5,014 bytes，不含包元数据、manifest 或 AEAD，属于字段推导而非实测。当前为 873/1,329 bytes。需要新 key ABI、材料格式版本及全套定向 conformance/oracle/E2E。

## PRG 条件

- **节点 PRG**：需证明/假设 `G_A(seed)=截取 AES_{encode126(seed)}(0,1,2,3)` 得到 382-bit 有效字段是安全 PRG。AES key 由 126-bit seed 编码且其余两位固定，不可无说明地引用均匀 128-bit AES key 安全性；不报具体安全位数。
- **根随机流**：`seed_fss_once()` 以 OS `getrandom` 初始化 cryptoTools PRNG（每 stream 128-bit）；其 `SetSeed/refillBuffer` 使用 AES counter mode；每次 `keyGenDCF` 从调用线程取得两个 blocks，即 32 bytes。要把该流替换为独立均匀带，优势按 AES-CTR PRG 查询数记符号项。
- **源码计数**：KeyGen 每层两次 `ecbEncFourBlocks`，为 `2b` 调用/DCF pair；Eval 每层一次 `ecbEncTwoBlocks`，为 `b` 调用/DCF Eval。实际全路径 PRG/AES 计数 `NOT_MEASURED`；调用数、block 加密数、logical compare 与 key slots 不互换。

## 整池与 shuffle 状态

令 `C=n(n−1)/2,p=padded_n`。DCF KeyGen pair 调用数为 `N_DCF=2p+9C`；流式 Select/membership sidecar 只有 `9C` slots；`offline_material_slots_per_party=2p+2+9C` 另含 raw 与 forward/inverse shuffle 材料。跨 task/round/membership 使用新 key，同一 uCMP key 上的两个 Eval 属于一次比较槽。

只有在实际 key 达到单 key `ε_DCF` 函数隐私、随机流可替换、并且 simulator 能从腐化方输入/理想输出与候选泄露 L 生成同一 transcript 条件分布时，逐槽 hybrid 才可写成 `N_DCF·ε_DCF + Adv_PRG(Q)`。不能把真实执行得到的整段 L 当作未证明的模拟输入。当前 key 的 `ε_DCF` 尚未建立，所以整池门仍 `CONDITIONAL`。Protocol I forward/inverse shuffle、共享 handle、membership 和输出 share 的 `ProtocolIForwardInverseViewSimulator_L` 仍独立未证。

## 分门状态

| 门禁 | 状态 | 依据 |
| --- | --- | --- |
| `DCF_CODE_CORRESPONDENCE` | **NO-GO（定理转移被精确变量断点阻断）** | 函数等式成立；直接 correction-word 分布对应失败；没有替代重编码 |
| `SINGLE_KEY_PRIVACY_FOR_ACTUAL_SERIALIZED_KEY` | **NO-GO / UNPROVEN，不是攻击结论** | Definition 2 覆盖完整 key；当前无直接归约 |
| `ADAPTIVE_FULL_POOL_REDUCTION` | **CONDITIONAL** | 形式上可有 `N_DCF·ε`，当前 ε 未证，L simulator 与 shuffle 组合也未闭合 |
| `EXPERIMENTAL_FUNCTIONAL_HANDOFF` | **PASS（有界）** | 见 S24 验证报告和 handoff；仅功能接收，不作密码学结论 |
| `SECURE_ALIAS_READY` | **NO-GO** | DCF 单 key、整池/输出感知视图和 shuffle/output simulator 均未闭合 |

不得称 BB90 原算法、不得继承 BMW16 Theorem 8 的概率或比较常数、不得称四个 Select 层为完整安全协议四轮。
