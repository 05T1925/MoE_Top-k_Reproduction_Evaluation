# M6B-I-PROJECT-SELECT-5ITER S54：FTv3 Dealer 随机流桥接审查

日期：2026-10-25
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

项目功能仍为 signed Q20.12 分数降序、原始索引升序稳定同分，并输出原序、恰有 K 个 1 的 Top-K XOR mask shares。该实现借鉴 BB90 研究思路，是项目派生的五迭代稳定 Select；不是 BB90 原算法复现、BMW16 或 BB90+DCF 实现。本报告只审查 FTv3 离线 Dealer 随机流与固定源码之间的联合分布桥，不把功能证据扩写成安全结论。

## 1. 结论与门禁

| 主张 | 状态 | 适用范围 |
|---|---|---|
| S53 固定对象与来源 | `PASS_WITH_SCOPE` | 精确 commit/tree/parent、manifest 及 Git blob；checkout raw 换行差异单独记录 |
| FTv3 源码有序随机请求账本 | `PASS_WITH_SCOPE` | 单个 FTv3 发材 invocation，显式 `RAND_bytes` 包装调用 |
| IID tape 下 Beaver/permutation 联合材料律 | `PASS_WITH_SCOPE` | 固定公开参数、固定 profile、发材确定性后处理成功；不含真实部署 RNG |
| cap-success 条件化 | `PASS_WITH_SCOPE` | 理想 IID 32-bit 字带；不受 cap-success 筛选偏置语义材料 |
| `δ_reject` | `PASS_WITH_SCOPE` | `P_sem` 语义材料投影，`δ≤1023·2^-154<2^-144`；不是全签名 transcript 界 |
| 真实随机流替换 | `CONDITIONAL` | 需部署方接受状态感知、适应性整段流假设 `Adv_DRBG_T(Q,C;E_T)` |
| 目标 provider/熵源/进程生命周期 | `OPEN/NOT_AVAILABLE` | 本轮没有目标部署 profile；本机历史构建信息不构成部署证明 |
| 实际 RAND 成功条件化 | `OPEN` | 无真实错误状态优势或调用成功概率界；不可免费条件化 |
| 同 registry one-shot/burn | `PASS_WITH_SCOPE` | 同一受限且未回滚 registry；不覆盖全局、多目录、跨版本或备份回滚 |
| S53 外生 SuccessFence 消息视图 | `PASS_WITH_SCOPE`（继承） | 仅理想成功调度，不重证 S50/S51 子引理 |
| 真实有限 timeout 成功视图 | `OPEN` | 成功筛选与本方前缀/隐藏状态非依赖关系未证明 |
| BUILD/TEST、性能 | `NOT_RUN` / `NOT_MEASURED` | 本轮静态源码/证明审查，无 runtime 改动 |

**回答首要问题：**在明确的 status-aware 联合 DRBG 假设下，FTv3 的随机流替换可以条件性成立，且确定性 package/context 编码、摘要和固定密钥签名后处理不放大该计算距离。当前无法把假设落到目标部署，因为实际 OpenSSL/libcrypto/provider、熵源/播种/重播种和 T 进程生命周期资料缺失。首个未闭合部署依赖是 `E_T` 的具体实例与成功/错误状态模型，故 `DRBG_DEPLOYMENT_PROFILE=OPEN`，`JOINT_RNG_STREAM_BRIDGE=CONDITIONAL`，没有实现级安全 PASS。

## 2. 固定来源、工作区和证据层级

本轮在新隔离 worktree、短分支 `codex/m6b-i-project-select5-s54-rng-bridge` 从 S53 commit 开始。初始 worktree clean；`origin/main=c3926c68fd14f270faa8b55234311071947fa080` 是该精确候选的祖先。S53 commit 为 `dde7137c8fd185d5725d88980688caacde16da5b`，tree 为 `589f12d813576ba0ba890d2ad6dbfbaa8792fa26`，parent 为 S52 `578ecadfe807748a3e2fada54ca11efe1b00d28c`；S52 parent 为 S51 `1cc1e5202b3a603b55aaae985704933d70db0794`。S47–S53 的其余父链列在新 source/evidence manifest。

S53 manifest `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S53_SOURCE_EVIDENCE_MANIFEST_2026-10-25.json` 的 canonical delivery digest `43d5254d72f41d449356a9d8a3b8538219b7435a9b960f209c9efabe0a50437e` 以记录的 raw SHA、ordinal path 排序和定义的 UTF-8 行格式重算一致；4 个 delivery file 的 Git blob 均匹配其记录。`core.autocrlf=true` 下 3 个文本/JSON 当前 checkout raw SHA 与 manifest 值不同，但 blob 一致；plan 的当前 raw/blob 与其登记值相同。这是可逆换行转换，不是候选源码替代。S53 manifest 本身当前 raw SHA 为 `2eedf262e4b6a89152434c7a05b66eaab33181d44ef4ced017aa0f1d806efceb`，Git blob `f00a301672d36d133261112ebbe0dbd288615cc2`。`VFSS-baseline/` 相对固定 HEAD 无差异，桌面主工作区未用于写入。

审查的核心源码均来自 S53 tree：

| 文件 | S53 Git blob SHA-1 | 当前 checkout SHA-256 |
|---|---|---|
| `VFSS/src/moe_topk/score_to_mask_v3_gmw.cpp` | `848afbe5ddd5a34a54ed06dabce6b806a72835c6` | `6870cf9f1fb92cae8dcfaa385c7beb17effefab04afbc0e010f382565b3afc9f` |
| `VFSS/src/moe_topk/score_to_mask_v3_bounded_sampler.h` | `04b148691f452f95e0a6cf9bc97ab9c29443b03c` | `458a47fbdde4bf0ee9575a6b60b628d8750f0732bd51c1ed46239bc00090912f` |
| `VFSS/include/moe_topk/score_to_mask_v3_gmw.h` | `3806eed03dde2231a5be791c65475a0c0f4a54d4` | `b55b2908efa97f9fdc6a85f337bee8ea30e0b76057ab88ce2bdab0647fbdecc1` |
| `VFSS/src/moe_topk/score_to_mask_v3_party.cpp` | `e5a5def38d7fb66a5423bd1389d8c525d31559ac` | `cf99f62a50145faa4b6dcce31f1e0237c2975647293191cc830b8397178531a8` |
| `VFSS/src/moe_topk/score_to_mask_v3_escrow.cpp` | `7c2f2fd6725694125eb40f2725fe11d079fd1cd3` | `f6aca56cfe156892e861649cc5513513e12e3328ae2c55ba357c61d1e28c755` |
| `VFSS/src/moe_topk/protocol_i_transport.cpp` | `05e8a2d6c1782a4f64b9184b6fec2b0a79879597` | `7018044af8475e1fc87ca0aeb80538685bfafe304ded67642faad6195beeb5b4` |

结论区分四类证据：论文定义只作研究背景，不在 S54 裁决论文等价；本地代码事实由固定 S53 Git blob 支持；项目派生证明是 FTv3 与 SuccessFence 下的有限条件结论；OpenSSL provider/熵源及部署生命周期是待验证假设。

## 3. FTv3 profile 与生成输出

固定 FTv3 profile 为 `ft_version=3`、`sampler_version=2`、`lambda_bits=128`、`retry_limit=7`、`package_format=6`。context 为 format 2；escrow `receipt_format_version=2` 是独立字段。源码头文件常量位于 `score_to_mask_v3_gmw.h:33-45`；profile 由 `score_to_mask_v3_gmw.cpp:1126-1144` 的显式 FTv3 入口建立。输入/输出、角色和已批准泄露没有变化。

设固定公开参数为 `P=(session,invocation,N,K,route,circuit,schedule,party/recipient/escrow identities,receipt/state parameters,profile,signer verification identity)`，令 `m` 为 padded N，`G` 为固定 gate count。FTv3 issuer 在 `score_to_mask_v3_gmw.cpp:840-874` 先校验参数、identity、签名密钥与 receipt profile；在第一次随机字节请求前运行 `before_material()` 登记私有 burn marker。`P` 和 T 的签名密钥固定且不由在线输入决定。

## 4. 有序随机请求账本与上界复算

随机源包装位于 `score_to_mask_v3_gmw.cpp:280-287`：请求以至多 `2^20` 字节切块调用全局 `RAND_bytes(out,chunk)`；返回值不为 1 就抛出 `OpenSSL CSPRNG failed`，没有重试或 fallback。实际代码未显式传递 `OSSL_LIB_CTX`，也未在此入口绑定 provider。采样器位于 `score_to_mask_v3_bounded_sampler.h:7-30`，profile 重试界计算位于 `:48-63`。

随机请求按以下顺序发生：

1. **Triple tape（`gmw.cpp:875-894`）：**`fill_random` 请求 `G` 个字节。每 gate 使用一个 byte 的低 5 bit 表示独立均匀 `(a,b,a0,b0,c0)`；高 3 bit 不用。party shares 为 `a0/a⊕a0`、`b0/b⊕b0`、`c0/(a∧b)⊕c0`。因此每 gate 约束 `c=a∧b`，share 联合律由同 gate 的 5 个理想独立位决定。
2. **Permutation-share tape（`gmw.cpp:895-897,934-943`）：**随后请求 `ceil(m²/8)` 字节。每一位是 P0 one-hot permutation-matrix share；P1 对应位为真实 permutation bit XOR P0 share。该 tape 与之前 triple tape、之后 sampler words 在 IID 模型中独立。
3. **Fisher–Yates words（`gmw.cpp:898-933`）：**对 `i=m,m−1,…,2`，每次最多取 7 个 32-bit word。令 `t_i=2^32 mod i`，拒绝 `X<t_i`，接受后输出 `X mod i` 并 swap。每个 word 经 `fill_random(4)`，因此每个尝试对应一条 4-byte `RAND_bytes` 调用。调用数量/停止点依赖前面的拒绝结果，但被拒绝的值和停止点不会写入 package/context。
4. **编码、摘要与签名（`gmw.cpp:186-227,547-585,944-1007`）：**源码显式随机包装器不再调用 `RAND_bytes`。固定版本/公开字段与语义材料产生 package header、material SHA-256、带用途域和 package format 6 的 Ed25519 签名；context format 2 带两方 package hash、package-set hash、receipt format 2 并再签名。它们是固定输入/key/provider 上的后处理。源代码没有显式从 T tape 取签名随机数；签名实现、key 管理和签名安全仍是独立假设/门禁。

对 `N=1000`，`m=1024`、`index_bits=10`、`W=42`、`pairs=1024·1023/2=523,776`。固定源码 gate 公式（`gmw.cpp:718-741`）为

```text
G = 63m + m²W + pairs(2W−1) + m²
  = 64,512 + 44,040,192 + 43,473,408 + 1,048,576
  = 88,626,688.
```

`m=1024` 的置换随机带为 `ceil(m²/8)=131,072` B。7 次/每位置的 Fisher–Yates 最坏消耗为 `4·7·(m−1)=28,644` B；故

```text
Qmax = G + ceil(m²/8) + 4·7·(m−1)
     = 88,626,688 + 131,072 + 28,644
     = 88,786,404 bytes.
Cmax = ceil(G/2²⁰) + 1 + 7·(m−1)
     = 85 + 1 + 7,161
     = 7,247 RAND_bytes calls.
max request = 2²⁰ = 1,048,576 bytes.
```

`Qmax/Cmax` 是固定源码中单个 Dealer issuance 显式 `RAND_bytes` 包装请求上界。它不含启动时另行生成的 session/invocation、调用其他 OpenSSL API、issuer 同进程其他使用者或其他并发 invocation；也不含 party 侧 output refresh/recipient envelope、A 的 token RNG、TLS 内部随机数。真实部署的优势参数必须把共享 DRBG 的总 invocation 汇总成 `(Q_total,C_total)`，而不是把一条会话上界当作进程寿命上界。

## 5. IID law、拒绝概率和 cap-success 条件化

在 IID byte tape 下，32-bit word 在每个 Fisher–Yates 步骤均匀。`ρ_i=(2^32 mod i)/2^32≤1023/2^32<2^-22`。接受区间 `[t_i,2^32)` 长度可被 `i` 整除，所以每个 residue 被接受的原像数相同。对固定位置 `i`、尝试数 `j≤7` 和 `y∈[0,i)`：

```text
Pr[J_i=j, Y_i=y] = ρ_i^(j−1)·(1−ρ_i)/i.
```

这直接给出 `Y_i` 独立于该位置成功尝试数且条件均匀；不同 Fisher–Yates 位置消耗 IID tape 的不相交段，停止规则的自适应消耗不改变后续未读 tape。Triple bytes 和 permutation-share bytes 先于拒绝 words 产生、互相独立。因此有：

```text
Law(M_semantic | P, cap_success, IID tape)
  = Law(M_semantic_FTv3 ideal | P),
```

其中 `M_semantic` 仅含两方 Beaver triple shares 和两方 permutation shares，不含 FT/sampler/package/context/receipt version、encoding、hash、signature、lease、签名上下文或后续网络 transcript。`cap_success` 不偏置该语义材料律。最坏 `m=1024` 有至多 1023 个采样位置，每位置 7 次都拒绝的概率为 `ρ_i^7`，所以

```text
δ_reject = 1−Π_(i=2..m)(1−ρ_i^7)
          ≤ Σ_(i=2..m)ρ_i^7
          ≤ 1023·2^-154
          = (1023/1024)·2^-144
          < 2^-144 < 2^-128.
```

实际 supported padded `m` 由 `N≤1000` 且 `m≤1024` 限制；因此该上界覆盖完整 FTv3 输入域的采样位置数。若比较“有 cap-abort 的 FTv3”与“永不失败理想语义材料”，`δ_reject` 只挂在上表定义的 `M_semantic` 投影。若理想 F_T/v3 本身也执行同一 cap 与统一发材中止，则这一 cap 语义不是额外 hybrid 差异。不得将 `δ_reject` 自动写成完整签名 package/context/party transcript 的总变差界，也不得把它与 DRBG 优势合并成未推导的单一安全位数。

## 6. 真实 DRBG 联合游戏与条件界

定义公开调用环境 `P` 与具体部署 `E_T`。隐藏初始播种与重播种熵记为 `S` 并对其联合分布求平均；DRBG 演化状态不是新鲜独立 entropy input。真实自适应调用流写为

```text
W_real = ((ℓ_j, status_j, bytes_j))_(j=1..C),
```

其中每个请求长度 `ℓ_j`、请求边界、返回状态、实际字节和终止/abort 位置都属于模型。若某次 `RAND_bytes` 失败，源码当次立即抛错；不许把失败返回从游戏里条件掉。Padded Fisher–Yates 使后续请求数依赖之前输出，故需要整个有序流的联合计算界，不能将每次调用的边缘均匀性相乘。

**所需假设 `A_DRBG_T(E_T)`：**在 fixed `(P,E_T)` 和实际 T 进程上下文下，任何 PPT distinguisher 对真实全局 `RAND_bytes` 自适应接口（含 status）的完整 issuer 输出，与同一 request machine、成功返回 IID bytes 的输出区分优势至多

```text
ε_T = Adv_DRBG_T(Qmax,Cmax; E_T, P).
```

假设必须将实际公开 `P`、隐藏 `S` 的分布、实际 provider 和错误状态纳入。固定签名私钥由两个 issuer 实验内部持有且不交给 distinguisher；distinguisher 只看到协议允许的 issuer 输出。若多次签发/其他调用共享同一 DRBG，替换上界使用 `Adv_DRBG_T(Q_total,C_total;E_T)`；若签名 provider 或其他 API 有内部随机调用而未被此 oracle 计入，必须另列随机项，不能塞进 `Qmax/Cmax`。

Hybrid：

- `H0`：实际目标环境 `E_T` 的全局 RAND 实例，固定 issuer 跑完一次完整发行，包括 request trace、success/abort、相关 package/context 和私有 registry 状态。
- `H1`：只把该 issuer 对 RAND wrapper 的整段自适应接口替换成 IID byte tape，保持同一请求长度规则、status 观察、固定 `(P,sk_T)`、签名/散列与文件顺序。`H0→H1` 优势为 `ε_T`（或者按前述约定使用成功流 DRBG 项和显式 API failure 项）。
- `H1→H2`：按源码采样器精确推出 FTv3 的 IID 生成律、cap abort 概率和成功条件化式；该步没有计算假设。对 cap-success 下 `M_semantic` 与理想 FTv3 的等式为精确等式。
- `H2→H3`：将固定版本 package/context、摘要和签名应用于语义材料。对随机带替换而言，这是同一有效算法的后处理，数据处理不增大 `ε_T`。此说法以固定签名 key、发行调用成功及相同确定性序列化/签名实现为条件，不证明签名不可伪造、SHA-256 碰撞抵抗、文件持久化、recipient encryption、TLS 或数据库安全。

### RAND 成功条件化

令 `S_RNG = {本次调用所有 RAND_bytes 调用均返回成功，且 bounded sampler 未 cap-abort}`。它只表示“随机/采样阶段成功”，**不表示签名、三个文件的持久化发布或整个 Dealer 调用成功**。理想 IID 字节调用无 API error，且 `Pr_I[S_RNG]=p_I≥1−δ_reject`。要求 `ε_T` 的游戏同时输出 `(S_RNG,V)`，而不是只比较条件于成功的字节边缘。事件测试给出 `Pr_R[S_RNG]≥p_I−ε_T≥1−δ_reject−ε_T`；对任意 PPT 条件 transcript distinguisher D，构造联合 distinguisher：`S_RNG` 成立时输出 D(V)，否则输出固定位 0。于是

```text
Adv( Law_R(V | S_RNG), Law_I(V | S_RNG) )
  ≤ 2ε_T / (p_I−ε_T)
  ≤ 2ε_T / (1−δ_reject−ε_T),
```

其中第二个较松界只在 `ε_T<1−δ_reject` 时分母为正；本界采用包含状态位和完整输出的联合计算优势 `ε_T`。若 DRBG 定理只覆盖所有调用成功后的字节、不覆盖 RAND error/status，则还须给出 `Pr_E[任一 RAND_bytes 失败]` 上界或独立错误状态游戏，并把它纳入联合优势后才能条件化。不得把通用 API 文档、build/test host OpenSSL 版本或单次随机测试当作此界。

若目标结论条件于**完整成功发材/party package 已发布**，还需令 `S_issue=S_RNG∧S_publish`，其中 `S_publish` 要求签名/序列化及三次有序 `durable_create` 均成功；在联合游戏覆盖该状态、且给出理想 `Pr_I[S_issue]≥p_min>ε_T` 时，才可用同一条件化引理（分母以 `p_min−ε_T` 替代）。本轮没有签名/provider 或文件写入成功概率下界，也没有将发布失败条件化成安全游戏，故“完整包成功发布条件下的联合视图”保持 `OPEN`。这不影响对包含失败状态的无条件整段发行输出所作的条件 DRBG 替换假设。以上两种条件界均不含 S53 真实 finite-timeout 的 `S_actual`；SuccessFence 是另一模型条件。

## 7. 版本/签名后处理、角色可见性与随机源边界

FTv3 package 格式 6 的 profile/schedule/身份/材料 hash 被 package 签名覆盖。party signature 用 FTv3 专用 purpose domain，domain-separated message 还带 BE32 package format；context format 2 单独绑定 FT profile、package format、`receipt_format_version=2`、session/invocation/N/K、identities、circuit/schedule、两方 package hash 与 package-set hash，再签名。`schedule_hash_ft_v3()` 包含 1 MiB frame、batch、phase 和 schedule 描述字段。版本不同的 package/context law 不是 FTv1 的完整转录 law；S54 只说明固定 FTv3 profile 下对随机流的后处理。

| 状态/字段 | 来源和去向 | party 视图 | 说明 |
|---|---|---|---|
| Triple/permutation share bytes | T 随机带→各自 package | 本方 package 可见；对方 package 不可见 | 是当前 RNG bridge 的主要输入 |
| Fisher–Yates permutation | bounded words→两方 matrix shares | party 只从本方 package 有其 share；完整 permutation 不公开给单方 | 与 share bytes 的联合律由 IID tape 描述 |
| Profile/circuit/schedule/hash | 固定参数、确定性 header/context hash | 本方 package 中仅有已签字段；A context 独立给 A | 不包含随机请求计数 |
| Ed25519 signatures | `EVP_DigestSign` 的固定 key/固定 message 处理 | party 看本方包签名；A 收 signed context | 源码未通过 `random_bytes` 显式消耗 T tape；算法/实现安全另列 |
| Burn marker | 确定性 session/invocation/profile hash key + fixed record | 仅 T 私有 registry | 是一次性本地状态，不是本方泄露 |
| `RAND_bytes` request count/status | T 局部调用及异常路径 | 不写入成功 package/context；发材调用者能观察统一成功/异常类别 | 本轮没有目标 provider 错误日志/计数 |
| A token、party sample/output refresh、AES key/nonce、TLS 随机数 | A/Party/transport 各自组件 | 依各角色源码 | 明确不纳入 T 的 `Qmax/Cmax`；对应 bridge 仍 OPEN |

`L_S29` 与 A-only `L_A` 不变。A context、两方 package-set digest 与 escrow 内部状态仍是 A-only；不将 retry count、DRBG 状态或新增 timeout 数据塞入 party leakage。协议代码生成的随机后续视图若要由 issuer 输出推出，必须在完整系统 hybrid 中另外接入各自随机源和私有信道项；S54 不单独关闭完整 party-view。

## 8. One-shot、burn 和发行失败的真实边界

`register_ft_v3_invocation_once`（`gmw.cpp:472-544`）在材料 vector 分配和随机采样前，以 domain-separated SHA256(session, invocation) 得到 `ft-v3-...` 文件名；目录必须为 T 所有、0700、禁止 symlink。文件使用 `O_CREAT|O_EXCL|O_NOFOLLOW`，record 包含 session/invocation/N/K、FT/sampler/lambda/retry/package format、circuit/schedule hash，并 fsync 文件和目录。固定目录且无外部回滚时，同一路径第二次同 invocation 被拒；这是源码/本地文件系统条件下的 profile-local one-shot 机制。

bounded sampler cap abort 发生于 `dealer_issue_packages_impl` 生成 package/context 和调用 `durable_create` 之前（`gmw.cpp:897-1007`），因此该**指定失败**留下 T 私有 marker，而不交付 party package/context/在线 lease/READY；材料 vector 可能已经形成，没有源码级清零保证。S48 的人工字节源用例观察过这一发材前缀，S54 不重跑该用例。

不可外推为全局不可回滚或多文件事务：FTv3 marker namespace 不让 FTv1/FTv2 issuer 自动识别该墓碑（小 N 的跨版本显式重发在其机制范围外）；换 registry 根路径、复制/回滚 backup 或跨文件系统耐久性没有证据。成功生成后 P0 package、P1 package、signed context 是 3 次顺序 `durable_create`（`gmw.cpp:1005-1007`）；若后续创建失败，先前文件可能已经存在；当前 `durable_create` 正在创建/写入/fsync 的文件本身也可能留下空、部分或完整 artifact，源码没有跨文件事务回滚或失败后 unlink。S54 未复现此类文件系统故障，故不判实现缺陷，也不声称对任意发材失败路径提供原子不可见。

## 9. 与 S50/S51/S53 证明及 timeout 模型的关系

- S49/S50 的 IID FTv3/`cap_success` 语义律可按上述确定性源码映射复用；它覆盖理想随机带，不覆盖真实 provider。
- S50 `Kpre_3` 是 N≤256 的 ideal FTv3 pre-escrow 条件 transcript 子引理；N=1000 只有 S47/S48/S52 有限功能收据，不是安全扩域。
- S51 `Kpost_3` 是 ideal A token RNG、因果 honest-peer deposit 和成功 escrow API 后缀范围；A token RNG、签名/recipient encryption/TLS 实际桥未关。
- S53 在外生 SuccessFence（所有既定合法消息由环境保证在 deadline 前处理）下继承 `Kpre_3+Kpost_3` 成功应用消息视图 `PASS_WITH_SCOPE`。S54 的 Dealer RNG 结论若部署假设成立，可作为把理想 FTv3 输入口映到实际 issuer 输出的**一个条件 hybrid**，但不是完整系统安全证明。
- 真实有限 timeout 的 `S_actual` 成功筛选仍需证明 `Law(Vpre_b | Q_b,cap_success,S_actual)≈Law(Vpre_b | Q_b,cap_success)` 或构造权限正确 sampler；S54 不提供网络/OS/TLS/DB 调度律，不将它替换成 SuccessFence，也不扩 `L_S29`。

因此仅可以报告：**在理想 IID 发材 law 下源码分布映射成立；在具体、状态感知的 DRBG 假设下，issuer 输出桥有条件成立。**不能报告“实际 OpenSSL 配置已证明与 IID 等价”或“整体安全通过”。

## 10. 独立复核、运行项和剩余门

执行者参与了 S45–S53 的相关交付/推导，故执行者不是非作者审查人。本轮新建的只读协作审查代理以 `fork_turns=none` 启动，未参与 S45–S54 的发材实现或推导；其只读复核本决策、报告、门禁及引用源码，复核范围为：(i) 拒绝采样条件联合律和 δ；(ii) `Qmax/Cmax` 来源与每次调用长度；(iii) status-aware DRBG 假设及 success-conditioned reduction；(iv) signature/context 确定性后处理边界；(v) marker/重放的精确源码范围；(vi) S53 SuccessFence 与真实 timeout 结论分离。reviewer 确认请求次序与 Qmax/Cmax、IID 联合律及 δ 投影、包含状态的条件界、确定性后处理边界和 SuccessFence 区分均准确；提出的两项精确修改（分母条件 `ε_T<1−δ_reject`、文件写入失败时当前 artifact 也可能残留）已落实，另将完整 `S_issue` 下界收紧为 `p_min>ε_T`。该复核只覆盖理论与源码映射，不含目标部署实例化或完整安全定理；reviewer 没有改文件、没有构建/运行测试。

本轮仅做固定源码/历史证据静态审查、概率推导、哈希及 manifest 复核。没有 runtime/test 改动或可复现代码缺陷；BUILD/TEST=`NOT_RUN`，S35 195、S40 211、S48 13、S52 15 案和 N=1000 功能矩阵均未重跑；正式网络/性能 `NOT_RUN/NOT_MEASURED`。

关闭具体部署门之前最小需要的脱敏证据：

1. 实际 Dealer 的可执行文件/容器身份、libcrypto 解析路径与完整版本/build options/hash；实际加载 provider/module、module hash、property query/FIPS 状态、`OSSL_LIB_CTX` 与 `OPENSSL_CONF`/`OPENSSL_MODULES` 配置（不提供密钥）。
2. 目标平台的熵源类别、首次播种与 reseed 策略/阈值、DRBG 类型/层级、错误状态传播；不得提供 seed、entropy bytes 或材料。
3. T 进程 threads/concurrent issuers、fork/exec、restart/reseed、VM/container snapshot/restore 和 registry backup/rollback 生命周期；所有共享 RNG 使用者的最大 `Q_total/C_total` 及单进程调用交错模型。
4. 项目/部署方可接受的状态感知自适应整流安全假设或可审查的 `Adv_DRBG_T(Q_total,C_total;E_T)` 归约；说明如何满足 `Law(S|P,sk_T,E_T)` 假设及成功事件下界。没有具体资料前保持 OPEN。
5. 低概率拒绝上限发材中止仍需项目负责人决定：合法输入下若离线发材以极低概率达到 bounded sampler 上限，本次 invocation 作废、保留 T 私有 burn marker 并要求新 invocation 重试，是否可接受？此问题不涉及已确认的正常 escrow/输出合同。

其余未关闭门保持独立：`A_TOKEN_RNG=OPEN`、`PRIVATE_CHANNEL_TLS_MTLS=OPEN`、`SIGNATURE_BINDING_SECURITY=OPEN`、`RECIPIENT_ENCRYPTION_HASH=OPEN`、`PARTY_VIEW_COMPOSITION=OPEN`、`ACTUAL_FINITE_TIMEOUT_VIEW=OPEN`、`N_GT_256_SECURITY=NOT_IN_SCOPE`、`SECURE_ALIAS_READY=NOT_READY`、`PERFORMANCE_READY=NOT_READY`。无 `secure` alias，无 push/merge。

Source/evidence manifest: `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S54_SOURCE_EVIDENCE_MANIFEST_2026-10-25.json`.
