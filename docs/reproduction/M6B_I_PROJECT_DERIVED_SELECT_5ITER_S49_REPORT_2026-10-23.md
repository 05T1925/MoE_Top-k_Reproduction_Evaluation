# M6B-I-PROJECT-SELECT-5ITER-S49 详细复核报告

日期：2026-10-23
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`
结论：理想 IID 下的 FTv3 语义材料成功条件律 `CONDITIONAL_PASS_REVIEWED`；S42→FTv3 的 party-view 迁移、目标部署 DRBG 与私有信道证明均 `OPEN`。本轮不构成完整安全通过。

## 1. 执行摘要

本轮以 S48 固定提交 `e05b01d5c7e3899297b76a5e0fca212d38d24229` 为精确对象，在隔离 worktree 只做 provenance、源码与证明复核。未重跑 S35/S40/S46/S48 功能矩阵；没有可复现的实现错误，因此没有改运行时代码，也未构建或运行测试。

答案分三层：

1. **理想 IID 发材材料层：可闭合，但只限语义材料投影。** FTv3 从独立随机字节产生 Beaver triples、置换 share 和 bounded Fisher–Yates 样本。接受位置的余数在条件成功时严格均匀，且与该位置耗用的拒绝次数独立；不同阶段使用不相交的 IID tape 片段。因而，条件于所有采样步骤未触及 cap，语义材料的联合分布与无限制、精确均匀 Fisher–Yates 材料相同。无界 cap 失败概率由 union bound 限制为 `δ_reject≤1023·2^-154<2^-144`。
2. **完整版本化 FTv3 理想发行 law：可以精确写成独立版本接口，但不是 FTv1 transcript 等价。** 固定 profile、包/上下文编码、用途域签名、摘要与数据字段都是语义材料和公开参数的确定性后处理。要与实现相符，理想功能必须显式纳入 `ISSUANCE_ABORT`、固定 profile 和 T 的 signing capability，并区分 party package 与 A-only context。版本化包有意不同，不能用 `δ_reject` 声称完整 F_T/v1 与 F_T/v3 package transcript 统计接近。
3. **从 S42 推出 FTv3 成功路径 party-view 模拟：尚未闭合。** S42 的 H4 条件 law 只经审查覆盖 ideal FTv1、`N≤256` 和旧应用 schedule。FTv3 使用新的签名 profile 与可见批次/phase/frame schedule；缺少适用于新 package/schedule 的本方 `Kpre_3` transcript sampler，及其新可见字段完全由批准 `L_S29` 和公开参数生成的证明。`N=257..1000` 还超出 S42 定理原有 N 范围。该结论是证明缺口，不是攻击或不可能性。

实际 OpenSSL `RAND_bytes` 全序联合流和真实 TLS 部署配置另有独立缺口。当前没有目标部署环境资料，不能把 S48 WSL2 构建环境的 OpenSSL 3.0.13 当成部署 profile。N=1000 的单次 Dealer 随机请求上界仍可从固定源码复算，但它只限制请求，不证明输出流伪随机或 IID。

## 2. 来源、工作区与精确对象

### 2.1 S48 起点

- 指定基线：S48 commit `e05b01d5c7e3899297b76a5e0fca212d38d24229`，tree `14cc78cb20080dd11326d1f9beb69279872e4aaa`，parent S47 `feb5e6677568d649906f46d0e611d5d52349c07f`。
- S48 原始 source/build/evidence manifest SHA-256：`62e7cbf05ef7946232e4e322a6fe8e0a3d3927d2fe600fdf98899efd9aefc9f8`。
- S48 九项 changed-source/delivery canonical raw SHA-256 复算为 `e6dde491a1cf7696173a6690dcb65d6473b5f5b589702b84f013705eecd59e84`，与记录一致。
- S48 commit 中九项的 Git blob SHA-1 均与 S48 manifest 相符。S49 当前 checkout 对其中三项治理文档（PROJECT、实施计划、参考清单）做了本轮状态追加，所以这三项当前字节/blob 不再与 S48 基线相同；其余六项仍属于 S48 原快照，其中三项当前 raw SHA 直接匹配，另三项仅在 CRLF→LF 后匹配记录 raw SHA。复核清单同时记录 S48 commit blob 和 S49 当前 blob，未把本轮预期变更误判为基线冲突。
- S47 manifest 原始 SHA-256：`3e4be30fdbd0c0463b39ad1ec8dcd1893a096f7bc92d35c6f86b850a936480ef`；19 项 `source_delta` 的 recorded-raw-SHA canonical 为 `5e70ff463f6e8593604fef320953e0df59ab2762bae1b39fb32c8a7fb1f70f7c`。按 S47 清单生成时使用的 culture-sensitive path sort 重算匹配该值；ordinal bytewise sort 得到另一摘要，因为 S47 没有显式指定 collation。19/19 Git blob 在指定 S47 commit 下匹配，源文件身份不依赖该排序约定。
- S47 源码增量的当前 checkout raw hash 差异只见于治理/报告类文本的行结束转换，以及 S48 已记录的两项 TEST_ONLY harness 与治理文件变化；runtime/CMake 的 14 项仍与 S47 blob 匹配。历史对象没有用当前主工作区内容替代。
- `VFSS-baseline/` 无差异；桌面主工作区和 S48/S47 原 worktree 未修改。本轮在新 worktree 建立分支 `codex/m6b-i-project-select5-s49`，开始状态为指定 S48 commit。

### 2.2 S47 来源链

S47 manifest 的 19 项是从其 `source_delta` 记录逐项对照 S47 commit 得到，而非从名称相近的当前文件推定。S48 对这 19 项的接收基线与 S48 自己的两项 TEST_ONLY 扩展区分。S48 manifest 记载 `runtime_source_changed=false`、`CMake_changed=false`、`TEST_ONLY_sources_changed=true`；S49 继承的是 S48 已接收的精确对象，不回到 S35 旧源码，也不从 main 拼装。

### 2.3 历史证据边界

S48 的 N≤1000 功能接收为 `PASS_WITH_SCOPE`：13 个已列出的多进程 oracle 案通过；这不是全 N/K、全输入证明，也不是安全/性能证据。本轮不重跑任何历史矩阵。S42 是本轮待迁移的证明前提；S42 本身的条件范围不能由 S48 的功能结果扩大。

## 3. 证据类别、功能与角色合同

### 3.1 四类证据

| 类别 | S49 使用内容 |
|---|---|
| 论文定义 | Protocol I 的相关基础；不推出本项目 Select5 是 BB90 算法，也不推出完整安全性。 |
| 本地源码事实 | FTv3 profile、包/上下文编码、签名、retry sampler、burn marker、随机请求调用点、TLS 上下文配置。 |
| 项目派生设计/证明 | S42 的 H4 party-visible success projection；S47 的 FTv3 profile/schedule；S49 在 IID tape 下的语义材料条件 law。 |
| 待验证假设 | 目标部署 RNG/熵源/DRBG 生命周期、真实条件成功分布、TLS 部署参数、FTv3 的 H4 版本适配和完整 transcript composition。 |

### 3.2 固定功能合同

双方输入为 signed Q20.12 score shares；重构后的分数按 signed 值降序，分数相同按原始索引升序稳定排序。输出为原始输入顺序下恰有 K 个 1 的 Top-K mask XOR shares。S48 确认的正常托管合同继续有效：P0/P1 各自在内存中短暂形成本方 share 并加密提交；A 只保存密文、不解密，收到两份密文后共同提交；之后双方分别取回本方 share；两 API 物理返回时刻可以不同。

角色为离线受信 T、双方计算方 P0/P1、在线条件受信 A。至多一方静态半诚实腐化；基线中 T/A 不与被腐化方合谋。任意 abort/crash-prefix 隐私、A-only 腐化、物理掉电耐久和本轮未固定的部署模型不纳入安全结论。人工进程终止只是故障注入；自然业务 I/O 故障沿用 `NOT_REPRODUCED`，S49 未重测。

## 4. S42 H4 的实际证明接口

S42 审查的 party-view 是离散应用/API transcript，不是原始 TLS 记录或完整隐藏世界。对腐化方 b，其条件输入包括本方输入/理想输出、批准泄露、本方签名 package、静态状态、公开参数和本方实际前缀。模拟器不获得对端 package/share/凭据、A context、T 私钥或 proof-only `J_b` 权限。诚实对端必须用自己的同次调用材料和输出按因果顺序提交，A 使用真实状态机；receipt token 由理想独立 A 随机带产生。S42 的投影 law 不要求 `Law(H_b|Q_b)` 本身相等。

S42 已复核结论严格限于：理想 `F_T/v1` 与 `F_TOPK`，一方静态半诚实，固定 route，`1≤K≤N≤256`，新鲜一-shot package，理想随机带，无 I/O/进程故障的成功应用/API transcript，以及一次从已 COMMITTED 状态开始的本方 authenticated fetch。其 Hybrid 0→1 需要的是特定旧 schedule 下的 H4 前缀 kernel `Kpre_b`。

S42 将 `L_S29` 与 `L_A` 分开。`L_S29` 未增列上下文、包集合摘要、peer 密文、PENDING/COMMITTED 服务内部状态或服务到达时序；批准的通信长度/活动选择字段仍按 S29/S42 原记录。A 的 context、包摘要、密文 hash/length、事务状态和顺序属于 A 侧视图。S49 不重新定义或扩大这些函数。

## 5. 理想 FTv3 的联合随机变量和生成律

### 5.1 同次调用绑定

固定公开调用元组 `P=(protocol, session, invocation, N, K, route, circuit_hash, gate_schedule_hash, party identities, recipient-key hashes, escrow identity, receipt_format_version, resource/state parameters, signer verification identity)`。源码以 package profile、session/invocation、N/K、身份/key fingerprints、circuit/schedule digests 和 context 的 package hashes/set digest 绑定调用。FT version、sampler version、package format 和 receipt-format 是互不替代的字段空间。

FTv3 固定 profile 为 `(ft_version=3, sampler_version=2, lambda_bits=128, retry_limit=7, package_format_version=6)`；context 格式为 2，协议版本 3，escrow `receipt_format_version=2`。FTv3 schedule hash 含批次容量/frame/phase/Select5 schedule。party package 使用 FTv3 专用签名用途域；A-only context 的签名覆盖版本 profile、P、两方 package SHA-256、package-set digest、状态参数和 receipt format。P0/P1 只取得各自 package；`ctx_A` 仅交 A；T 的私钥不交模拟器。

### 5.2 IID tape 下语义材料的精确分布

设 `m` 为 padded N，`G` 为 gate count。理想 T 使用一条 IID 均匀字节 tape，按源码次序切分：

1. `G` 个字节为 triple 随机带。每个 gate 的一个字节取 5 个独立低位，形成独立均匀的 `a,b,a0,b0,c0`；两方份额为 `(a0,a⊕a0)`、`(b0,b⊕b0)`、`(c0,(a∧b)⊕c0)`。所以每个 gate 的本地份额独立均匀，重构得到有效 Beaver triple，且不同 gate 独立。
2. `ceil(m²/8)` 字节为 permutation-share 带。其每个位是 P0 one-hot 矩阵 share 的独立均匀位；P1 是真实置换矩阵 XOR P0，故两份重构为同一置换矩阵。
3. Fisher–Yates 对 `i=m,m−1,…,2` 依次请求独立均匀 32-bit 字 `X_{i,j}`。令 `t_i=2^32 mod i`、`rho_i=t_i/2^32`；仅接受 `X≥t_i`，然后取 `X mod i`。固定最多 7 次，七次皆拒绝则本次发材中止。

对某一位置 i、尝试编号 j≤7 和任一结果 `y∈[0,i)`：

```text
Pr[J_i=j, Y_i=y] = rho_i^(j-1) * (1-rho_i)/i.
```

因此 `Y_i` 在尝试次数/该位置是否 cap 成功的条件下仍均匀；条件于所有位置都在 cap 内成功，整组 `Y_i` 的联合律仍是逐位置独立均匀的 Fisher–Yates 选择。triples 与 permutation shares 在拒绝字前已由不相交 IID tape 区段生成；自适应消耗 Fisher–Yates 字的停止点后续 tape 仍独立 IID。条件成功不会偏置前缀材料。故：

```text
Law(M_semantic_FTv3 | P, cap_success, ideal_IID_tape)
  = Law(M_semantic_exact_uniform_FisherYates | P).
```

这里 `M_semantic` 仅含两方 triple shares 与 permutation shares，剥除 profile/version/encoding/signature/package/context/receipt 字段。拒绝概率满足：

```text
rho_i <= 1023 / 2^32 < 2^-22
Pr[cap_abort] = 1 - Π_i(1-rho_i^7)
              <= Σ_i rho_i^7
              <= 1023 * 2^-154 < 2^-144 < 2^-128.
```

若把 cap-abort `⊥` 与永不失败的 semantic-material law 比较，TV 差距最多 `δ_reject`；若条件于成功，语义材料 law 精确相等。该统计项只适用这项明确定义的材料投影，不能直接套在跨版本 package 或签名转录上。

### 5.3 版本包、上下文和发材失败状态

在理想 T 签名 capability/key 固定、非随机的编码/哈希/sign 操作成功时，版本化 package 和 context 是 `M_semantic` 与 P 的确定性后处理。确定性后处理不增加前述语义材料分布距离，但 FTv3 transcript 本来就不是 FTv1 transcript：其固定 profile、签名域、package/context format、schedule hash 和 receipt field 均不同。不得把它写成“FTv1 完整 package law 等于 FTv3”。

`F_T/v3` 的 faithful ideal interface 应定义为：有效新调用先 burn 固定 `(session, invocation, profile)`；按 IID tape 生成 M；采样 cap 耗尽时仅向发材调用者给统一 `ISSUANCE_ABORT`，保持私有 burn 状态，不向 P0/P1 发布 package、向 A 发布 context 或创建 online lease；成功时按上述 profile 构造两份有签 package 和 A-only context，分别交付本方/A。模拟器只拿获准的本方 package，不取 peer package/context/T 私钥。重复同 invocation 不再发行。

真实源码在 sampler 之前创建并持久化 T 私有 marker；cap abort 时 marker 保留，包、上下文/签名与 party lease 尚未发布；随机材料 vector 已在内存形成且无显式清零证明。S48 的 abort 测试仅直接覆盖离线发材发布边界；在线 A/P0/P1 在该测试中未启动，在线 DB row 状态是 `NOT_RUN`。后续三个 durable-create 是依次进行，不能由该 cap-abort 证据推出任意写失败的跨文件原子性。

### 5.4 固定源码锚点

| 审查事实 | 源码锚点 |
|---|---|
| profile 字段、签名域、三份 durable create | `VFSS/src/moe_topk/score_to_mask_v3_gmw.cpp:58-64,840-1007` |
| 5-bit/gate triple 映射、独立 permutation-share bytes 与 Fisher–Yates 顺序 | `score_to_mask_v3_gmw.cpp:871-943` |
| 1 MiB `RAND_bytes` chunk 与错误即中止 | `score_to_mask_v3_gmw.cpp:280-286` |
| FTv3 invocation marker、重放拒绝、marker 持久化 | `score_to_mask_v3_gmw.cpp:472-542` |
| production 与 TEST_ONLY 都固定 retry cap=7 | `score_to_mask_v3_gmw.cpp:1126-1193`；常量位于 `VFSS/include/moe_topk/score_to_mask_v3_gmw.h:37` |
| bounded sampler、阈值与版本支持域 | `VFSS/src/moe_topk/score_to_mask_v3_bounded_sampler.h:7-29,48-63` |
| package/context profile、receipt-v2 与 schedule validation | `score_to_mask_v3_gmw.cpp:1216-1299,1331-1412` |
| TLS 1.3、peer verification 与 verify depth | `VFSS/src/moe_topk/protocol_i_transport.cpp:211-238` |
| Party/escrow pinned identity checks | `VFSS/src/moe_topk/score_to_mask_v3_party.cpp:388-390`; `score_to_mask_v3_escrow.cpp:577-579,685-687` |
| FTv3 的公开 batching、phase/frame profile 未继承旧泄露覆盖结论 | `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S47_FT_V3_SPEC_2026-10-23.md:56-83,146-179,236-243` |
| S42 H4 已通过的范围、Q_b、Hybrid 0→1 与 L_S29/L_A | `docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S42_REPORT_2026-10-22.md:13,37-43,65-88,115-165,169-188` |

这些行号固定在 S48 commit 的 runtime/document Git blobs；本轮未改 runtime。此锚点把代码事实与本报告的理想分布推导分开。
## 6. S42→FTv3 成功路径迁移裁决

### 6.1 成功后缀可复用的范围

receipt/v2 的 S42 证明从固定本方加密 share 与已经 COMMITTED/服务状态前提继续生成 token/response/fetch projection。S48 确认 escrow API 合同、token 字段与 A 事务顺序未改；FTv3 的 context 仍绑定 receipt-format 2。因而在新的 `Kpre_3` 已证明且固定本方 share envelope 的条件下，S42 的 A/T 信任边界和本方 receipt/fetch 投影是合理的复用候选。它不是本轮对 FTv3 端到端成功模拟的新 PASS，也不证明 A-only 或 crash-prefix 模型。

### 6.2 首个精确未闭合等式

对 `b∈{0,1}`，令 `Q_b^3=(x_b,z_b,L_S29,pkg_b^3,static_b,Vpre_b^3,C_b^3)`，其中包是 FTv3 自己的签名 package、前缀是 FTv3 新批处理协议实际消息。本轮已证明的材料式只给 `Law(M^3|cap_success)=Law(M^ideal)`；S42 的 H4 只给旧 FTv1/旧 schedule 的 `Kpre_b`。尚未证明以下本方 transcript 续接式：

```text
Law(Vpre_b^3 | x_b,z_b,L_S29,pkg_b^3,static_b,P,cap_success)
  = Kpre_b^3(x_b,z_b,L_S29,pkg_b^3,static_b,P),
```

其中 `Kpre_b^3` 必须在不取得 peer package/share/credential、A context、T 私钥或 J_b 的情况下模拟 READY、Beaver d/e openings、公开比较结果、活动选择 seed/边、layer/batch/count、phase/sequence、长度以及本方材料消费状态。S42 没有定义或审查这个 kernel。FTv3 specification 明确标出新增 batch boundaries、frame counts 和 phase pattern 是外部可见 schedule profile；它未主张这些新字段已由旧 `L_S29` 覆盖。缺少的可检验陈述为：每个新增可见 `(layer,batch,count,phase,length)` 都是 `P`、公开 gate schedule 与旧 `L_S29` 已授权字段的确定性函数，并且 H4 的条件局部 wire/material 分布在该 schedule 下仍可采样。若成立，需给新的 H4 推导；若不成立，不得自行将字段添加到 `L_S29`。

对 `N=257..1000`，`Kpre_b` 的输入域及 S42 定理直接不包含这些实例；必须证明扩域后的 gate/material 条件分布，而不能以 N=1000 E2E 功能正确代替。没有 toy 反例或攻击；这是未建立的条件 transcript kernel，不表示不存在 sampler。

### 6.3 party/A 权限分离

| 角色 | 成功路径允许信息 | 本方 party simulator 不可获值 |
|---|---|---|
| P_b / `Sim_b` | `x_b,z_b,L_S29`、公开设置、己方 signed package、自己的状态/前缀和协议真实可见 A 响应；可与诚实对端和 A 按实际协议因果交互。 | peer package/share/凭据、`ctx_A`、T 私钥/其他随机带、proof-only `J_b`、无条件成功 deposit oracle。 |
| P_(1-b) honest endpoint | 仅本方输入/理想输出、己方 package/凭据及公开设置；自己产生密文并认证 deposit。 | A 解密材料、被腐化方私密状态。 |
| A | 两次认证请求及加密密文、context/package-set/hash/长度、到达顺序/服务状态/token；只保存密文、不解密。 | 明文 shares/解密私钥。此为条件受信模型，不做 A-only simulator。 |
| T | 离线材料随机带、签名私钥、私有 burn marker。 | 在线输入、rank、协议计算状态。 |

`L_S29` 与 `L_A` 不合并。FTv3 版本元数据必须让 party 从本方包获知的常量或公开参数计算；A-only context 及 package-set/context hash 继续只在 A 视图。模拟成功后缀必须让 honest endpoint 的第二次 deposit 真实发生，再由 A 状态机进入 COMMITTED；不能伪造 `peer_deposit` 成功。

## 7. 随机请求上界与真实 DRBG 桥

### 7.1 固定源码中的单次发材上界

`random_bytes()` 使用全局 OpenSSL `RAND_bytes` 并把每次请求切分到最多 `1<<20` 字节；失败即抛出 `OpenSSL CSPRNG failed`。FTv3 的 `G` bytes、置换 share bytes 和每个 Fisher–Yates 32-bit 字均通过该包装器。N=1000、padded `m=1024` 时，从固定 FTv3 gate budget 得 `G=88,626,688`：

```text
Qmax = G + ceil(m²/8) + 4·7·(m−1)
     = 88,626,688 + 131,072 + 28,644
     = 88,786,404 bytes.
Cmax = ceil(G/2²⁰) + 1 + 7·(m−1)
     = 85 + 1 + 7,161
     = 7,247 RAND_bytes calls.
max request = 1,048,576 bytes.
```

这是单次 T/Dealer FTv3 发材路径的保守上界，且在每个 Fisher–Yates 步骤都达到 cap 前仍可能少用字。它不包括 party sample seed、party 输出 refresh、RSA-OAEP/AES-GCM envelope key/nonce、A receipt token 或 TLS 内部随机调用；也不含多 invocation 总 query 数或 DRBG lifetime 查询。故真实优势应参数化总聚合量 `Q_total,C_total`，而非拿一次 Qmax 代替部署生命周期预算。

### 7.2 所需真实随机游戏

在明确部署环境 `E` 与公开参数 P 下，定义真实对象为整个有序随机过程 `W_T=((len_j,bytes_j,status_j))_j`：含适应性请求边界/长度、成功或失败返回、cap abort 点及输出包/上下文/公开后处理。理想对象是同一调用规则下的 IID byte tape。隐藏 seed/reseed entropy 输入 S 应对 S 求平均；演化的 DRBG 内部 state 不是独立 entropy input。所需的是整个 joint stream 的计算不可区分界 `ε=Adv_DRBG_T(Q_total,C_total;E)`，不是每次 `RAND_bytes` 的边缘随机性。

若真实与理想完整随机游戏的**无条件统计总变差距离**为 `ε_TV`，理想 cap-success 概率 `p≥1−δ_reject`，则真实 success 概率至少 `p−ε_TV`，且在 `ε_TV<p` 时条件成功 transcript 的总变差距离满足 `TV(P_real|S,P_iid|S) ≤ 2ε_TV/(p−ε_TV) ≤ 2ε_TV/(1−δ_reject−ε_TV)`。这是统计距离条件化引理，不能把计算型 `Adv_DRBG_T` 直接代入 `ε_TV`；若只获得计算优势，须另给条件成功 DRBG 游戏或带成功事件的归约。RAND 返回失败状态与 cap 失败状态必须留在联合游戏输出中，不能静默条件掉失败。比较带 cap-abort 的版本化 FTv3 可把 cap abort 纳入理想接口；若改与永不失败理想语义材料 law 比较，另计 `δ_reject`。签名/封装/TLS 项分别列，不把这些项混成未经证明的单一数值。

### 7.3 目标部署证据缺项

本轮没有目标部署 profile 文件或可安全读取的部署资料。S48 manifest 的 Ubuntu/WSL2 build host OpenSSL 3.0.13/default-provider 是历史构建信息，不是目标部署证明。实际源代码使用默认全局 `RAND_bytes`，未显式传入 `OSSL_LIB_CTX`。需要只读、脱敏地提供：

- OS/容器镜像与构建包身份；OpenSSL/libcrypto 完整版本/build options；加载的 provider/module、property query、FIPS 状态；`OPENSSL_CONF`/`OPENSSL_MODULES` 与全局 context 配置。
- 熵源种类/配置、初始化及 reseed 策略/间隔、DRBG 类型/父子层级、RAND 错误如何传播；不提供 seed、entropy byte 或任何秘密。
- T 进程 fork/restart/并发线程模型、共享 RAND context、最大 invocation 数、生命周期内聚合 `Q_total,C_total`、并发 interleaving、重启与 reseed 边界。
- TLS 部署实际协商版本/密码套件/组/签名策略、证书验证与 pin/轮换流程、CA policy、session resumption/key update/provider 配置。证书和私钥文件不收集。

因此 `DRBG_DEPLOYMENT_PROFILE=OPEN/NOT_AVAILABLE`、`JOINT_RNG_STREAM_BRIDGE=OPEN`。不能以文档、`RAND_bytes` API 名称或单次功能测试关闭该门。

## 8. 私有信道和组合范围

源码将 TLS context 的 min/max protocol 固定为 TLS 1.3，加载 CA/证书/私钥，启用 `SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT` 并设 verify depth 4；party/escrow 还执行配置身份/pin 校验。该结论是本地源码配置，不证明目标部署的 openssl/provider/certificate policy，也未给 TLS transcript/leakage game 的优势。`PRIVATE_CHANNEL_BRIDGE=OPEN`。

不得由 S42 本方投影或 S48 功能 E2E 推出真实 FTv3 的 shuffle/output/escrow 完整组合。输入转换、Beaver 开值、活动比较结果、批次schedule、正/逆路由、输出刷新、输出加密、A pending/commit/token/fetch 需在 FTv3 `Kpre_3` 闭合后依因果顺序合成。OAEP/AES-GCM、T 签名、hash binding、A token RNG、TLS 均保留独立符号假设；无参数时不写安全位数。

## 9. 发材中止、功能收据与故障边界

- S48 的 `ISSUANCE_ABORT` cap 注入只证明离线 Dealer 路径：同 invocation 的 T 私有 marker 先创建并保留；在 cap 失败处未发布 party packages/context、未建立在线服务 lease/READY。测试没有启动 A/P0/P1，故不能说已观察“线上数据库零行”；材料临时 vector 清零未证明。
- S47 的后续 durable-create 是 package0、package1、context 依次完成；cap abort 位于这几个文件发布之前。不能由 cap 用例推出之后写失败/崩溃具有跨文件 rollback。
- 合法输入的低概率有界拒绝中止是否可接受没有正式确认记录，保持 `OPEN_CONFIRMATION`，独立于安全证明。
- S48 的 13/13 和其资源观察为 `INHERITED_PASS_WITH_SCOPE_NOT_RERUN`。N=1000 每方 RSS、frame/payload、exchange phase 是历史本机功能诊断；本轮不重复、不作为性能/RTT。
- 自然业务 I/O 故障 `NOT_REPRODUCED`（S49 未运行通信）；人工 kill/crash、任意故障前缀、物理掉电/A-only 均不扩展本轮定理。

## 10. 独立复核与本轮运行

执行者曾参与 S35–S48 的相关实现/证明/决策，不把自身检查称为独立接收。reviewer `/root/s49_crypto_independent_review` 未参与 S47/S48 实现，也不是 S49 证明作者或共同推导者；其此前只读审阅过 S40–S42。该 reviewer 在本轮先做基于固定源码的独立初审，作者完成报告初稿后又做最终只读复核；这两次意见属于审查，不隐去其对最终措辞的反馈。复核范围为 IID cap 条件等式与 δ 界、S42 H4 域与 FTv3 schedule 缺口、Qmax/Cmax、条件成功度量、TLS 源码事实和 abort 范围。reviewer 未构建、未运行测试、未修改文件。

本轮运行：提交/tree/parent/status 与 S47/S48 manifests、hash、Git blobs 和 S47/S48 canonical 身份检查；源码/证明静态审查；JSON 交叉检查和 `git diff --check` 已完成。
本轮未运行：`BUILD/TEST=NOT_RUN`、协议进程、oracle、随机源统计、通信/故障实验。S40/S48 的测试结果只作为其历史配置的继承证据。

## 11. 状态与可声明范围

可写：**“项目自有五迭代稳定 Select 已在报告列出的 N≤1000 测试用例中实现，并通过稳定原序 Top-K mask oracle 检查。”** 句子必须附 `S48 PASS_WITH_SCOPE` 与“非穷尽案例，非安全/性能结论”。不能写“全 N/K 正确”“完整协议安全”“BB90 复现”“BMW16 实现”或“性能通过”。

当前阻止完整安全论证的事项：S42 H4→FTv3 profile/schedule 的 `Kpre_3` 条件模拟、N=257..1000 的证明域扩展、目标真实 DRBG ordered-stream/entropy/lifecycle bridge、目标 TLS/auth channel bridge、以及 FTv3 shuffle/output/escrow 联合 composition；此外低概率 `ISSUANCE_ABORT` 产品合同确认仍独立 OPEN。S48 已接收功能不关闭这些门。

给非实现负责人的单一确认问题：

> 合法输入下，若离线材料准备因有界拒绝采样达到重试上限而以极低概率返回 `ISSUANCE_ABORT`，并需要换一个新 invocation 重新发材，是否可接受？

## 12. 下一步最短路径

1. 只读取得脱敏目标部署 RNG/TLS profile，确定 `E`、总查询量和生命周期边界；之后定理才可写 `Adv_DRBG_T(Q_total,C_total;E)`。
2. 先把 S47 固定 FTv3 schedule 与 S42 的 L_S29、Q_b、Kpre 字段逐项映射，证明新增 batch/phase/frame 字段是公开/已获准泄露的确定函数；再独立构造并复核 N≤256 的 `Kpre_3`，成功后才扩到 N≤1000。
3. 分开完成真实 signature/recipient-encryption/A-token/TLS 优势项和在线 party-view/escrow composition；不创建 secure alias。
4. 把低概率发材 abort 确认单独登记。完成上述安全/功能准入前，不启动正式 LAN/WAN、RTT 或 AAV86 性能矩阵。