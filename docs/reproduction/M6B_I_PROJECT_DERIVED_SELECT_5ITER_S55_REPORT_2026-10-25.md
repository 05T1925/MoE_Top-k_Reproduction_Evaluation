# M6B-I-PROJECT-SELECT-5ITER-S55 详细审查报告

日期：2026-10-25
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 1. 范围与结论摘要

本轮以 S54 固定源码核验 FTv3 Dealer 的随机请求、错误/中止、签名与文件发布路径，重新计算安全证明域和功能域的单次 Q/C 上界，并尝试将 S54 的条件优势界实例化到目标 T 部署。目标部署配置没有出现在当前仓库、可见部署说明或该隔离 checkout 的服务配置中。因此停止在“源码级账本 + 条件定理 + 部署资料缺失”：真实 `DRBG_DEPLOYMENT_PROFILE` 与 `JOINT_RNG_STREAM_BRIDGE` 保持 `OPEN`。

| 门 | 状态 | 本轮裁决 |
|---|---|---|
| S54 固定来源与源码身份 | `PASS_WITH_SCOPE` | S54 commit/tree/父链、manifest、交付 Git blobs 与 runtime blobs 已核对 |
| FTv3 IID semantic-material law | `PASS_WITH_SCOPE`（继承） | 仅理想 IID tape 与语义材料投影；不是目标 DRBG 结论 |
| 单次随机请求账本 | `PASS_WITH_SCOPE` | N≤256 与 N≤1000 两种 padded 上界已重新计算 |
| `Adv_DRBG_T` 条件桥 | `CONDITIONAL` | 必须覆盖整段自适应 API 请求/返回状态与发行输出 |
| 目标部署 profile | `OPEN / NOT_AVAILABLE` | 未找到可归属目标 T 的部署配置或部署方声明 |
| 聚合 `Q_total_API,C_total_API` | `OPEN` | 缺 T 进程/共享 RNG 使用者与生命周期调用上界；provider 内部计量另列 |
| `S_RNG` 成功条件 | `CONDITIONAL` | 只在包含 RAND 成功/错误状态的联合优势和目标熵模型下 |
| 完整 `S_issue` 成功条件 | `OPEN` | 缺 `Pr_I[S_issue]≥p_min>ε_T` 证据及完整发行状态游戏 |
| burn marker / 三文件发布边界 | `PASS_WITH_SCOPE` / `OPEN` | 仅同一未回滚 registry 的拒重放；三文件不是原子发布 |
| 非作者独立源码/证明复核 | `PASS_WITH_SCOPE` | 账本、条件式与发布边界复核完成；未构建/测试 |
| BUILD / TEST | `NOT_RUN` | 本轮无 runtime/test 改动，未运行构建或测试 |
| 正式性能、LAN/WAN、RTT、AAV86 | `NOT_RUN / NOT_MEASURED` | 不在本轮范围 |

## 2. 固定对象、来源与工作区保护

- 从 S54 commit `4695d70a4bb9a7f2bbce5c6fc7e163cb3f93d978` 建立全新隔离 worktree 和短期分支 `codex/m6b-i-project-select5-s55-drbg-deployment`。S54 tree=`8acd16e2ee891aed3be8f0021d8d1aaafd57ca1c`，parent=`dde7137c8fd185d5725d88980688caacde16da5b`。S54 manifest 记录的 `origin/main=c3926c68fd14f270faa8b55234311071947fa080` 是固定提交的祖先；本轮分支包含该固定候选，没有以桌面主工作区或近似快照替代。
- S54 manifest raw SHA-256=`73bad36464ed8192d8e6ee1518380c43c7d8249f1cbbc6345cfc5b20ee2957e8`，Git blob=`4454f8cdb54824785996f7a20174b1d0d02f80dd`。其 canonical delivery digest 以 path ordinal 排序、TAB 分隔、UTF-8/LF 行重算为 `3e3f5786e14ec60c5469baedaca83e360bcd0ddb7d80e4c878c4de3f8eae41a2`，与记录值一致；S54 四个 delivery Git blobs、六个固定 runtime Git blobs 均匹配固定 S54 commit。新 worktree 中三份 Markdown/计划文件 raw hash 与 S54 旧 manifest 的 raw hash 不同，差异来自 `core.autocrlf` 检出换行；Git blob 身份一致且为来源裁决依据。JSON raw hash 与旧登记相符。
- 本轮扩展固定源码审查了 Dealer、bounded sampler、公共头文件、party、escrow、传输与 CMake，以及 DCF/OPV/PermuteShare 等随机 API 调用源。每个文件的原始 SHA-256 与 S54 commit Git blob 均写入 S55 source/evidence manifest。
- 本轮只读检索了项目树里的部署/服务/容器/系统服务/OpenSSL 配置文件名与相关内容；没有目标 T 可执行文件、镜像 digest、服务 unit、deployment manifest、OpenSSL provider/config 或部署方 profile。`VFSS/CMakeLists.txt` 的 OpenSSL 1.1.1 是最低依赖声明，不是实际运行版本；S48 的 Ubuntu/WSL2 OpenSSL 3.0.13 与早期历史构建记录是构建证据，不是目标 T 环境。没有运行 `openssl version` 来推断目标配置，也没有读取或收集私钥、seed、熵、证书私密部分或生产凭据。
- `VFSS/` 是活动实现；没有修改 `VFSS-baseline/`。S45–S54 的决策、报告、门禁、cases 和来源记录仅作为历史依据读取；S48 N≤1000 功能收据不被扩展或重标。

## 3. 目标部署配置状态

| 配置字段 | 实际采集值 | 代码默认/源码事实 | 本轮状态 |
|---|---|---|---|
| T 服务可执行文件、容器/镜像身份 | 未找到 | 仓库提供的是 VFSS 静态库与实验 target；没有目标 T 的发布包/服务清单 | `OPEN / NOT_AVAILABLE` |
| OpenSSL/libcrypto 版本、构建来源、加载路径/hash | 未提供 | CMake 要求 OpenSSL ≥1.1.1；没有将此约束锁定为目标构建版本 | `OPEN / NOT_AVAILABLE` |
| Provider、模块、property query、FIPS、`OSSL_LIB_CTX` | 未提供 | FTv3 调用全局 `RAND_bytes`，该入口不显式传入 `OSSL_LIB_CTX` 或 provider | `OPEN / NOT_AVAILABLE` |
| `OPENSSL_CONF` / `OPENSSL_MODULES` | 未提供 | 仓库没有目标实例配置文件 | `OPEN / NOT_AVAILABLE` |
| OS 熵源、DRBG 类型/层级、首次 instantiate/reseed 策略 | 未提供 | 随机来源由 OpenSSL `RAND_bytes` 抽象；源码不固定 provider 的 seed/reseed 行为 | `OPEN / NOT_AVAILABLE` |
| 线程、worker、进程数、fork/restart/恢复/迁移 | 未提供 | 无目标进程生命周期/并发 profile | `OPEN / NOT_AVAILABLE` |
| FTv3、签名、TLS、同进程其他 RNG 使用者是否共享实例 | 未提供 | 多个仓内组件直接调用 `RAND_bytes`；不同系统/角色有独立 API，源码不能证明目标部署的进程边界和 context 共享关系 | `OPEN / NOT_AVAILABLE` |
| RAND 返回失败/错误队列/重试/新 invocation | 未提供目标运行日志 | FTv3 wrapper 遇到 `RAND_bytes != 1` 立即抛错；bounded rejection 达 cap 返回 `ISSUANCE_ABORT`，同一固定 registry marker 保留 | 代码事实 `PASS_WITH_SCOPE`；目标错误模型 `OPEN` |
| 签名器类型/provider 与文件系统/挂载选项 | 未提供 | 源码使用 OpenSSL EVP Ed25519 并调用三次顺序 `durable_create` | `OPEN / NOT_AVAILABLE` |

结论：没有可依据的目标 `E_T` 实例。S54 的条件优势不能用此工作站、历史构建信息、OpenSSL API 名称或功能测试代入。

## 4. FTv3 的有序随机请求和错误路径

固定源码锚点均按 S54 Git blobs：

- `VFSS/src/moe_topk/score_to_mask_v3_gmw.cpp:280-287`：`random_bytes` 将每个调用拆成最多 `1<<20` 字节块，逐块调用 `RAND_bytes`；任意块返回值不为 1 时立即抛出 `OpenSSL CSPRNG failed`。没有内部重试、fallback 或把失败值代入材料。前面已成功的块已消耗；异常时材料可能只在内存中部分形成。
- `score_to_mask_v3_gmw.cpp:718-741`：`m` 是从 2 起始的下一个 2 的幂，最大 1024。令 `w=32+ceil(log2(m))`、`pairs=m(m−1)/2`，

  ```text
  G(m)=63m + m²w + pairs·(2w−1) + m².
  ```

- `score_to_mask_v3_gmw.cpp:839-943`：在 marker 成功后先分配并填充 `G` 个 triple 随机字节；接着取得 `ceil(m²/8)` 个 permutation-share 字节；随后按 `i=m,…,2` 执行 Fisher–Yates，每个最多 7 次调用 4-byte `fill_random`。接受/拒绝结果决定后续 API 请求数，因此请求序列是自适应、变长的；达到 cap 时停在该位置并抛出 `ISSUANCE_ABORT`。
- 单次发材源层请求界：

  ```text
  Qmax(m) = G(m) + ceil(m²/8) + 4·7·(m−1)
  Cmax(m) = ceil(G(m)/2²⁰) + 1 + 7·(m−1)
  request_max = 2²⁰ bytes.
  ```

  permutation-share 请求对本支持域始终是一条请求；每个 FY word 独占一条 4-byte API 请求。`Qmax/Cmax` 是该调用对 `RAND_bytes` 的请求数/字节数上界；它不是成功返回的字节数、底层 provider 内部 DRBG generate/reseed 调用数或目标进程总量。

| 范围 | `m` | index bits / `w` | pairs | `G` bytes | permutation bytes | FY 最大 bytes/calls | `Qmax` | `Cmax` | 最大单请求 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| S50 安全证明域 N≤256 | 256 | 8 / 40 | 32,640 | 5,281,664 | 8,192 | 7,140 / 1,785 | **5,296,996 B** | **1,792** | 1,048,576 B |
| S48 已测功能域 N≤1000 | 1,024 | 10 / 42 | 523,776 | 88,626,688 | 131,072 | 28,644 / 7,161 | **88,786,404 B** | **7,247** | 1,048,576 B |

对于 N≤256 安全域，源码 retry cap 为 7；用 `rho_i≤255/2³²<2⁻²⁴` 得 IID tape 语义拒绝界 `δ≤255·2⁻¹⁶⁸<2⁻¹⁶⁰`。S54 使用较宽的 FTv3 全功能域界 `1023·2⁻¹⁵⁴<2⁻¹⁴⁴`；本轮结果与其一致，但在 N≤256 上可进一步收紧。两者仅适用于剥离 profile/version/encoding/hash/signature 后的语义材料投影与理想 IID tape，不是实际部署失败概率。

## 5. 部署级 Q/C 聚合及随机调用者

单次上界只足以参数化一个 FTv3 invocation。目标环境需给出每个共享 RNG context 的总请求量，例如：

```text
Q_total_API = Σ_i Q_FTv3(N_i) + Q_other_API
C_total_API = Σ_i C_FTv3(N_i) + C_other_API.
```

并发调用应以实际共享 context 的合并顺序/自适应调用模型计数；多 context/多进程需给各 context 的上界及可组合优势。不能仅用 `Qmax(N=1000)` 当作 T 进程或服务生命周期总查询量。底层 provider 的 `generate/reseed/entropy` 事件记作独立的 `Q_internal,C_internal`，与 RAND API 字节数/调用数不是同一计量单位，不能直接相加。S54 DRBG 游戏的 `Q,C` 必须明确采用 API 请求模型还是 provider 内部操作模型，并通过目标 `E_T` 说明两者的映射。

本轮在活动 `VFSS/` 中看到以下可能相关的直接 RNG 使用者：

| 调用点 | 源码可见请求 | 角色/限制 |
|---|---|---|
| FTv3 Dealer | 本节已计量的 G、置换 share、FY requests | T 目标路径；唯一已完整计量路径 |
| FTv3 party | 每次 Select5 seed 32 B；输出 envelope AES key 32 B 与 nonce 12 B；P0 output refresh `N` B | party 端源调用，按已确认角色应与离线 T/A 区分；不计入 T，除非部署实际共进程 |
| FTv3 escrow | receipt/v2 token 32 B/交易 | A 端源调用；不计入 T，除非部署共进程 |
| BCG21 DCF keygen | 两次各 16 B root 调用 | 其他活动 API；目标 T 调用图未知 |
| OPV | 每次 16 B seed 调用 | 其他活动 API；是否在 T 进程未知 |
| PermuteShare DO preprocess | 一条 `24·n` B 调用（`ProtocolIBlock192` 为 24 B） | 其他活动 API；最大实际 n/是否同进程未知 |
| Select5 与 ParallelShuffle 的辅助随机流 | 使用 OS `getrandom()`，不是显式 OpenSSL `RAND_bytes` | 不是本表的 libcrypto API bytes；仍需在部署级 OS entropy 生命周期中确认是否相关 |
| TLS/EVP/provider 内部 | 本仓源码未提供目标部署内部调用总量 | 作为 `Q_internal,C_internal` 和状态/熵边界单独计量；不与 RAND API bytes/call 直接相加 |

这些调用点不证明运行时共享了同一 DRBG；它们证明仅凭单个 FTv3 issuer 函数无法推出进程总 `Q/C`。若 T 按冻结角色离线发材后退出、party/A 分进程，该角色隔离会排除对应方调用；但仍需确认同一 T 进程中的其他 API、TLS/签名 provider 和重复/并发发材情况。

## 6. S54 条件优势与成功条件化复核

### 6.1 必须实例化的游戏

令公开 profile/调用参数为 `P`；目标环境为 `E_T`。真实有序随机交互记录至少包含每一 API 请求边界、长度、返回字节或失败状态、请求交错、终止位置与 issuer 可见输出。隐藏初始播种和 reseed 熵统记为 `S`，按部署的 `Law(S|P,sk_T,E_T)` 平均；DRBG 演化 state 不是新鲜独立熵。需要如下整段优势，而非每次调用的边缘伪随机性：

```text
ε_T = Adv_DRBG_T(Q_total_API, C_total_API; E_T, P)
```

这里的 `Q_total_API,C_total_API` 只计同一目标 RNG context 上的应用层 API 请求/字节；provider 内部 generate/reseed/entropy 事件必须由 `E_T` 的具体 DRBG 模型描述，或以另一个明确计量口径建模，不能把两层数值直接相加。该记号是条件假设接口，不表示本轮已知实际总量。

此游戏必须含自适应边界和长度、RAND 成功/错误返回、bounded sampler 的 cap-abort 位置，以及所声称要条件化的 issuer 输出/状态。若只比较“成功返回的 bytes”，不能把错误概率、失败筛选或中止时点从条件分布推导里删掉。若签名或文件发布结果未被该游戏输出/覆盖，需要单独加入其后处理/故障状态模型及优势项。

### 6.2 `S_RNG` 条件

令 `S_RNG` 表示“本次发行的每个 `RAND_bytes` 请求均成功且 Fisher–Yates 未达到 cap”。在理想 IID tape 下，随机 API 不失败，故 `p_I=Pr_I[S_RNG]≥1−δ`。若完整联合计算游戏对 `(S_RNG,V)` 给出优势 `ε_T`，则事件测试可得：

```text
p_R = Pr_R[S_RNG] ≥ p_I − ε_T ≥ 1 − δ − ε_T.
```

对任意条件 transcript distinguisher `D`，把 `(S_RNG,V)` 输入上的试验在失败时输出固定值，可得：

```text
Adv_cond(D) ≤ 2 ε_T/(p_I − ε_T)
            ≤ 2 ε_T/(1 − δ − ε_T),    ε_T < 1 − δ.
```

该式是计算不可区分界在**同一个状态感知、带输出联合游戏**下的条件化归约，不是 TV 距离陈述；它不实例化未知的 `ε_T`，也不自动覆盖签名、文件系统或真实超时的筛选事件。

### 6.3 `S_issue` 完整发材条件

`score_to_mask_v3_gmw.cpp:944-1007` 在语义材料生成后依次构造/签名 P0 包、P1 包、签名 A context，然后顺序创建 package0、package1、context 三个文件。令 `S_publish` 包括上述签名、序列化、三次 `durable_create` 全部成功，`S_issue=S_RNG∧S_publish`。若完整 joint game 能比较 `(S_issue,V_issue)`，并且目标理想发行环境中有独立可审查的

```text
Pr_I[S_issue] ≥ p_min > ε_T,
```

才有同型条件界 `2ε_T/(p_min−ε_T)`。当前没有目标文件系统/签名/provider 的错误模型或 `p_min`；因此完整成功 package/transcript 的条件分布是 `OPEN`。不能把历史 E2E 没失败作为概率下界。

### 6.4 被界约束的材料与不被界自动覆盖的字段

在固定公开 `P`、私钥/key provider、序列化/哈希/签名和发布状态处理相同且确定性的条件下，package profile/version、gate schedule、材料 hash、签名、context 中的 package hash/package-set hash 和 receipt-format 参数是语义材料及公开调用输入的确定性后处理，随机带替换的计算优势不会被确定性函数放大。它们不是“均匀随机字段”。

此数据处理结论不自动证明 Ed25519 实现的不可伪造性、key 来源、HSM/provider 等价性、开放环境下的 `RAND_bytes` 成功率、签名/序列化故障条件化或多文件发布成功概率。Ed25519 在固定 key/message 下由算法给出确定签名；代码确实经 EVP `DigestSign`，而目标 signer/provider 未固定，因此确定性输出映射是需保持相同实现/接口的条件前提，签名安全单独记账。

## 7. One-shot、签名与多文件发布的真实边界

- 参数、身份与 escrow state 验证后，Dealer 读取 signer key；`before_material()` 随后调用 FTv3 marker 注册。marker 以 domain-separated `(session,invocation)` 计算文件名，在 T 私有 0700 且 owner 匹配的目录中以 `O_EXCL` 创建；记录含 `n,k`、FT/sampler/retry/package profile、circuit hash 与 schedule hash。marker file 和目录均调用 `fsync`，之后才分配/取随机材料。
- 同一固定 registry 目录中的 FTv3 同 invocation 再发行会因 `EEXIST` 拒绝；即使 marker 写入或后续步骤失败，已创建 marker 保留，阻止该目录中重用调用。该结论依赖路径/文件系统没有被备份恢复、快照回滚或迁移到旧状态。不同 registry 目录、FTv2 的独立 domain、其他主机、恢复副本与外部并行 registry 没有全局防重保证。
- 输入/参数/私钥加载若在 marker 前失败，不创建该 invocation marker；marker 后 `RAND_bytes`、sampler、签名或发布失败会留下 burn 状态。拒绝采样失败时材料只在进程内形成（若已形成），没有源码保证的内存清零；package/context/线上 lease 还未发布。
- `sign_bytes` 用 EVP Ed25519；签名域、package format/profile、package 头、材料 hash 都经 signed message 绑定；A-only context 又绑定版本、双方 package hashes/set hash、identities、state 与 receipt format 并签名。哈希/签名输入确定性，但实际 signer key provider、密钥硬件/进程隔离和签名优势没有目标证据。
- `durable_create` 各自使用 `O_EXCL`、mode 0600、循环写入、文件 `fsync`、close 及父目录 `fsync`；顺序是 package0→package1→context。没有跨三文件事务/rollback。任一创建、write、fsync、close 或后续目录 fsync 错误都可能使当前文件及此前文件残留；断电、文件系统、块设备/虚拟化层实际耐久行为未验证。
- 因此：`ONE_SHOT_BURN_SAME_REGISTRY=PASS_WITH_SCOPE`；跨目录/跨版本全局不可重放、备份回滚防护、多文件原子发布、掉电耐久、内存擦除与目标 signer 安全均 `OPEN`。这一组系统/文件结论不由 DRBG 假设覆盖。

## 8. S54 成果与 S53/其他门的继承边界

- 可继承：S49/S50 对理想 IID FTv3 semantic-material law、cap-success 及 `δ_reject` 投影；S54 自身源码账本与 `S_RNG` 条件归约形式。它们不实例化目标 provider/entropy。
- S53 `SuccessFence` 仅是环境外生保证消息在 deadline 前成功处理的理想执行模型。它不能关闭真实有限 timeout/wall-clock 条件视图：`ACTUAL_FINITE_TIMEOUT_SUCCESS_VIEW=OPEN`。
- A token RNG、TLS/mTLS、recipient encryption/hash binding、party-view composition、shuffle/output/escrow 联合模拟仍是独立开放门。S48 N≤1000 有限功能案例不是安全扩域或全 N/K 证明。
- 论文定义、本地源码事实、项目派生条件推导、目标部署假设分别记账；本轮没有把项目代码行为写成 BB90 原算法结论。

## 9. 独立复核

执行者参与 S45–S54 的相关审查/推导，不能自称独立审查人。非作者 reviewer 应以新鲜无历史上下文只读复核：

1. N≤256 与 N≤1000 的 G/Q/C 重算及“API request”与 provider 内部调用边界；
2. `S_RNG`、`S_issue` 成功条件与计算优势条件化分母；
3. 是否仅凭本机/历史 build 信息推断目标 T 环境；
4. `RAND_bytes` 全局调用及其他同进程可能调用者的聚合表；
5. burn marker 的 namespace/registry 局限、签名后处理和三文件发布失败残留范围；
6. `OPEN` 与 `CONDITIONAL` 状态没有被功能、SuccessFence 或 `fsync` 夸大。

Reviewer 未参与 S45–S55 实现/推导，以无历史上下文独立复核交付和固定源码，并重新核算两种规模的请求/调用界及状态感知条件公式。其结论为 `PASS_WITH_SCOPE`：源码调用顺序、重试上限、Q/C 数值、单位分层、目标部署资料缺口、条件化边界与 burn/publication 范围均与证据一致；并指出报告摘要及优势式的 Q/C 记号应显式写成 API 层。该项已按建议修订。本次 reviewer 未构建、未运行测试、未修改文件；这是证明/源码只读复核，不是功能接收，也不升级部署级 DRBG 安全门。

## 10. 可执行的脱敏目标资料清单

请由 **T 部署负责人/SRE** 提供运行环境与生命周期，由 **构建/发布负责人** 提供可执行文件、libcrypto/provider 的实际加载身份，由 **密码服务/PKI 负责人** 提供签名器类型/公钥指纹和熵/DRBG 策略。所有材料需带采集时间、目标环境标识（可匿名化）、服务版本与哈希；缺值明确写 `UNKNOWN`。不要提交私钥、seed、entropy bytes、证书私密部分、生产凭据、原始随机输出或包含这些信息的 core dump。

1. **真实进程身份：**T service 的可执行文件/OCI image digest、构建 ID、OS/kernel/container 基线、运行 uid、进程数/worker 数；实际加载 `libcrypto` 文件的路径与 SHA-256（从进程映射采集，而不是仅用 PATH 中的 openssl CLI）。
2. **OpenSSL/provider：**OpenSSL 完整版本、构建选项/来源；实际激活 provider/module 名称、版本及文件 hash、property query/FIPS 状态；服务使用 default `OSSL_LIB_CTX` 还是自建 context；运行时应用 context 实际加载到的配置标识、`OPENSSL_CONF`/`OPENSSL_MODULES` 的脱敏路径与配置 hash。仅 `openssl version -a`/`openssl list -providers` 的单独 CLI 输出不能证明服务进程使用相同 context。
3. **DRBG 与熵：**实际 DRBG 类型/parent-child 层级、strength 与 instantiate/reseed 策略和阈值、OS 熵源类别和初始化依赖、provider 错误传播。提供策略/版本/计数，不提供 seed 或熵内容。
4. **进程生命周期：**是否 fork/exec、worker/thread 数、并行 issuance 数、重启/restart、VM/container snapshot/restore/migration、进程恢复与备份回滚策略；RNG context 如何在这些边界重建或 reseed。
5. **请求总量：**每一 RNG context 上最大 FTv3 invocation 数及各自 N profile；签名/TLS/其他 OpenSSL API、同进程其他 `RAND_bytes` 用户总量；给出 API 层 `Q_total_API,C_total_API` 的推导、调用交错模型与测量口径。Provider 内部 generate/reseed/entropy 计量使用另行定义的 `Q_internal,C_internal`，不得直接与 API 字节/调用数相加；说明 `E_T` 如何映射两层。可提供计数器/聚合日志，不提供随机字节。
6. **错误与发布状态：**`RAND_bytes` 成功/失败状态计数、OpenSSL error 类别（脱敏）、wrapper 异常映射、cap `ISSUANCE_ABORT` 计数、失败后的 invocation 重试规则；签名、序列化、三文件各阶段 create/write/fsync/close/directory-fsync 状态及发布结果。不得用“历史没有失败”充当概率下界。
7. **Signer / filesystem：**软件/HSM 签名接口、Ed25519/provider 身份与公钥 fingerprint（仅公钥）；registry 与 package 的文件系统类型、mount/durability/备份/rollback 方式、不同发布目录是否同一文件系统。不得提供私钥或证书私密内容。
8. **可核验证据：**由各负责人对脱敏配置快照和推导签名/审批；附文件 hash、采集者角色和日期。实际进程内诊断应只输出 provider/context 标识、libcrypto/module 文件 hash、调用计数和状态类别，不输出 secrets、seed、随机 bytes 或材料。

资料到齐后，先把目标 `E_T,P` 与 T 的共享 RNG context/总量固定，再请独立审查者对状态感知整流游戏实例化。若只能给 `Q_total,C_total` 而不能支持 entropy/reseed/provider/lifecycle 假设，`JOINT_RNG_STREAM_BRIDGE` 仍 OPEN。

## 11. 独立复核、执行与停止状态

- Reviewer 结果：新鲜上下文非作者对固定源码、来源身份、两种规模 Q/C 账本、API/provider 计量边界、`S_RNG`/`S_issue` 条件界、目标配置缺口及 marker/发布范围完成只读复核，结论 `PASS_WITH_SCOPE`。reviewer 未构建、未测试、未编辑文件；其建议已用于统一 API 与 provider 内部 Q/C 记号。该项不构成功能接收或目标部署安全接收。
- Runtime/tests 未改；BUILD/TEST=`NOT_RUN`；S35/S40/S48/S52 历史矩阵不重跑；本轮没有协议 E2E 或自然 I/O 故障实验。
- 低概率合法发材中止接受：`OPEN_CONFIRMATION`，无新增负责人决定。
- `ACTUAL_FINITE_TIMEOUT_SUCCESS_VIEW=OPEN`；A token RNG、签名/recipient encryption、TLS/mTLS、完整 party-view 组合、N>256 安全扩域仍 OPEN/NOT_IN_SCOPE。
- 正式 LAN/WAN、RTT、AAV86 与性能矩阵 `NOT_RUN/NOT_MEASURED`；未创建 `secure` alias、未推送、未合并；用户主工作区与历史 worktree 未修改。
