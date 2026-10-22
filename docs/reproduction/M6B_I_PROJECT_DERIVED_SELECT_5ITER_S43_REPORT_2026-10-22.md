# M6B-I-PROJECT-SELECT-5ITER-S43：F_T/v1 到真实 Dealer 的联合分布桥接审查

日期：2026-10-22
路线：**Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL**
裁定：**理想均匀带下单次成功发材材料联合律 `PASS_WITH_SCOPE`；package/context 字节级耦合要求 receipt-format 参数一致；参数匹配后的真实离线 Dealer 首个概率断点是 OpenSSL `RAND_bytes` 联合输出到理想独立字节带的替换，整体桥 `CONDITIONAL / OPEN`。**

## 1. 结论摘要

本轮从 S42 已提交且哈希重核的文档快照出发，审查 S35 的原始材料规格、S39 固定 runtime 与 S40 的 TEST_ONLY 增量。S42 只有治理/证明文档提交；实际被审 Dealer/runtime 仍是 S39 固定提交 `8028c0fece69fe0c648b1e0f1e12ad7fca723f20`，其父链记录 S35 runtime `77f9684e1a50794004fd31f3e5ee7f193e9b12ec`。S40 manifest 记载 runtime 代码未变，只有 TEST_ONLY process E2E 驱动变化。

源码可支持一个**精确的条件结论**：若将一次成功 Dealer 调用中按顺序实际请求的 `RAND_bytes` 字节替换成独立均匀字节，则 Beaver triples、Fisher–Yates 排列与 permutation one-hot XOR shares 的联合分布与 S35 `F_T/v1` 材料生成律一致；package/context 的确定性签名后处理也在相同 tuple 和相同 receipt-format 参数下逐值耦合。S39 的 receipt-v2 opt-in 会令签名 context 中 `state.receipt_format_version` 从 1 变为 2；因此与 S35 固定 v1 context 的逐字节相等只适用于 receipt version=1，v2 模式要求理想接口显式采用相同状态参数。关键随机性原因是 triple tape、矩阵 share tape 和 shuffle draw 来自不同字节区间；代码没有把用于洗牌的随机值复用为 P0 permutation share。

从真实实现到这项理想化仍缺一个可量化的联合 RNG 假设。源码实际调用 OpenSSL `RAND_bytes`，检查失败并中止；仓库没有固定部署 entropy source/provider、跨线程/并发调用、进程 fork/restart/reseed 状态，也没有可引用的 `Adv_DRBG_T(q)` 界。因此本轮**不宣称**真实材料分布与独立均匀带相等，也不宣称此桥已被完整安全证明。OpenSSL 文档描述其 RAND API 为 CSPRNG/DRBG，并说明自动播种/重播种路径和熵源依赖；这些 API 文档不能代替本项目部署下的联合分布优势证明。[OpenSSL RAND_bytes](https://docs.openssl.org/3.0/man3/RAND_bytes/), [OpenSSL EVP_RAND](https://docs.openssl.org/3.0/man7/EVP_RAND/)

另一个边界是时间复杂度：`next_uniform` 使用正确的拒绝采样，但 `for(;;)` 没有重试上限。返回值条件分布无偏，期望抽样次数有界；实现因此是 expected-polynomial-time，而不是源码可证明的严格 worst-case PPT。若后续定理只接受严格 PPT，应在版本化采样器中限制重试次数并将拒绝失败概率显式加入功能/优势界。本轮没有因这一形式边界修改运行时代码。

没有发现可复现运行时缺陷；没有改 runtime，没有 fresh build/test，也没有重跑 S35/S40 功能矩阵。完整 GMW party-view composition、TLS/private-channel bridge、任意故障前缀和性能门仍分别 OPEN/NOT_RUN。此路线不是 BB90 原算法复现、BMW16 或 BB90+DCF 实现。

## 2. 固定对象、来源链与 S41 勘误

### 2.1 已核验身份

| 对象 | 精确身份 | S43 用途 |
|---|---|---|
| S35 runtime | `77f9684e1a50794004fd31f3e5ee7f193e9b12ec`；S35 manifest raw SHA-256 `62be2127ae4eb28eb2c6d5d5b1dc2a5d46d068d6a68a62f2c0835708abdc6c07`；85 项 canonical `af693eb61183a606c64a1f1b16ee5c3ad6e4b382ec5456e541c7915b8f95faa0` | 原始 `F_T/v1` 生成律与格式基线 |
| S39 runtime | `8028c0fece69fe0c648b1e0f1e12ad7fca723f20`；S39 manifest raw SHA-256 `af4643a816a2bc0e22772ce849d0daab082aa68161cc001777992d060c5b3615`；85 项 canonical `09444abd50c41357eb63e9050b7e39c3b2948588205d9d70f02561d7d8ec49d9` | 本轮真实发材/签名/package 实现对象 |
| S40 evidence commit | `b04618117b11cffe0e899947726ab1c66807ffdd`; S40 manifest Git blob SHA-256 `66b15c5f1354cb1a52812b721e4c456b9a67678bca06ca0209babd2fcdaf029b`; 85-file canonical `10c508b3d0d16a06f826bbdfd0ada97b9fd4b99c2abdcd2336cd5a5605b804f6` | Target-scale functional evidence on S39 runtime; inherited, not rerun |
| S41 文档提交 | `486026ba425a2d17ca15173d056db6eb309747d1` | S42 起始父级与 S41 decision/report/gates/manifest |
| S42 固定提交 | `72a23fc067b5bfbf10edf110eeff45ec1f0ed6bd`，tree `1d72a0942adff4a39253a73f4a12e2989559064e`，父级 S41 `486026…` | S43 的隔离工作起点；六项均为文档/证据改动 |
| S42 source manifest | raw SHA-256 `d6884503e9aff9997b8a0f5e59ec37b2e6a7ade50b1915d748b238ff051442ab`；85 项重算 canonical `2ea2896cce7dd284f3d317a7462dde528c188cc15cb32af16c744dad4e5e6837` | 85/85 源文件哈希及 5 项 artifact 哈希全部匹配 |

S42 source manifest 的 85 条 `sha256_current_s42_worktree` 均与当前固定快照逐字节重算相同；canonical 规则是 UTF-8 ordinal 路径排序、`path<TAB>小写 SHA-256<LF>`。`VFSS/src/moe_topk/score_to_mask_v3_gmw.cpp` 在 S39 Git blob 中的 SHA-256 为 `527440a7bb9fa0b08d37def3f1a471f357eb22238728c5b141fb20b7b4b9fc60`；相应 S42 checkout 原始字节 SHA-256 `8b27c1957b5b6d2a70d34d9edb6edbea77099c83b9218379911ee155616f6f26`。两种哈希差异来自 Windows checkout 换行；S42 manifest 的原始哈希重算与其记录一致，Git blob 身份由 commit 固定。

### 2.2 `OPEN_METADATA_DIVERGENCE` 的新增勘误

S41 manifest 原文保持不动。按已能复现 S40 canonical 的规则重算 S41 85 项 `sha256_s40_original_worktree` 得到 `10c508…804f6`，与 S40 记录一致；重算 S41 85 项 `sha256_s41_checkout` 得到 `e0bb4d2507a2cf2536c5d28d53c5c2c6aa19f927359482c1e236c5b19b5f8a69`，与 S41 汇总字段 `bfa8ba9c051ca69c43a22130fa9d1cefc8885cbed10284ed0506762667ab726c` 不一致。S41 没有记录产生 `bfa8…` 的另一种序列化或输入，故具体生成原因不可从现有证据还原；可确定原因仅为该汇总元数据无法从自身逐项值复现。详见 [S41 来源勘误](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S41_PROVENANCE_ERRATUM_2026-10-22.md)。S43 不把该 provenance 错误解释成 S39 代码哈希冲突。

### 2.3 工作区与参与关系

本轮使用隔离 worktree `C:\Users\28641\.codex\worktrees\m6b-i-s42-joint-simulator\MoE_Top-k_Reproduction_Evaluation`，起始 S42 commit 的工作树干净。用户主工作区、历史 S35–S42 worktree 与 `VFSS-baseline/` 未修改；不清理、不覆盖历史证据。执行者参与了 S35–S42 实现、审查、证明或决策，属于本轮作者；不能把自己的复核叫作独立复核。非作者 `/root/s39_crypto_review` 完成了本轮只读复核；其范围和条件性裁定见第 8 节及 source/evidence manifest。

## 3. 冻结的理想功能及证明目标

将一次成功发材调用的公开参数记为

```text
P = (protocol=v3, session, invocation, N, K,
     party TLS identity hashes, recipient RSA public-key hashes,
     escrow identity hash, circuit hash, gate-schedule hash,
     escrow state parameters (including receipt_format_version),
     fixed T verification key)
```

源码由调用方提供 `session`、`invocation`、`N/K` 和身份/公钥摘要；Dealer 不为这些字段自行抽随机值，也不接收分数、任一方输入 share、rank 或 mask。`route` 未作为独立 header 字段；固定路线由 circuit description 常量参与 circuit hash，gate 顺序由 gate-schedule description 参与 schedule hash。该结论限于固定路线，不能外推到共享同一包格式的其他 route。

理想 `F_T/v1(P)` 在 T 可信、离线且不与在线方合谋的模型下，使用独立均匀 tapes 生成；参数 `P` 中的 receipt-format 值必须与真实签名 context 一致：

1. 对 `g=gate_count` 个 gate 逐门生成相关 Beaver triples 及各自 XOR shares；
2. 对 `m=padded_n` 生成均匀排列 `π∈S_m` 和独立均匀矩阵 share `R∈{0,1}^{m²}`，输出 `R` 与 `P_π⊕R`；
3. 以固定 T 签名 key 按 package/context 不同 purpose domain 和格式版本签署两份 package 及 metadata-only escrow context；
4. 给两方各自一个未消费 one-shot lease 初态。

发材角色路由是：P0 只取得 P0 package 和自己的材料 share，P1 只取得 P1 package 和自己的材料 share；metadata-only `ctx_A` 只发给在线托管方 A，A 不取得 triple/permutation share；T 在离线生成中暂时持有全局关联材料和签名私钥，成功交付后退出。签名 oracle 是 F_T/setup 接口的一部分，party simulator 不获得 T 私钥。在线公开/私有 transcript 可能与各自 package 相关；`G3` 只用同一 package bytes 的耦合向下传递该关系，不声称单方 GMW transcript 已被证明可模拟。

该理想接口生成**一次单次调用的成功输出**。S35 规格没有把同一 tuple 的重复发材请求、幂等重试、发材中途进程重启/磁盘故障、全局 lease rollback/复制定义为 `F_T/v1` 状态转移。它们必须独立于单次成功路径讨论，不可以假定理想接口为 retry oracle。

## 4. 实际 S39 发材调用链和联合材料采样

The reviewed Dealer files are the S39 fixed Git objects `VFSS/src/moe_topk/score_to_mask_v3_gmw.cpp` and `VFSS/include/moe_topk/score_to_mask_v3_gmw.h`. Within these two files, the S35-to-S39 diff adds receipt-v2 support/validation; triple/permutation sampling, package header, material digest, signature body and context encoding logic have no diff. Opting into receipt/v2 does change the signed context's receipt-version field value from 1 to 2, while the encoding routine/layout is unchanged. This finding is limited to these two Dealer files: the overall S35-to-S39 `VFSS/` diff has seven tracked paths (CMake, the GMW header/source, escrow and party sources, and two tests), so it does not mean the whole runtime diff consists of only these files. Current checkout line numbers are navigation aids; Git object hashes in the manifest identify the source.

### 4.1 输入校验与随机字节

`score_to_mask_v3_dealer_issue_packages` 位于 `score_to_mask_v3_gmw.cpp:492–594`。它先计算资源预算并检查 `K∈[1,N]`、session/identity 非零、三个路径互异、Escrow 状态参数受支持，再加载 T Ed25519 私钥、导出 signer fingerprint 和确定性 circuit/schedule hash。`random_bytes` 在 `:234–241` 把大请求切成不超过 1 MiB 的块调用 `RAND_bytes`；每次返回码非 1 都抛错，不继续使用失败输出。

T 没有显式 seed 参数、不会从输入分数取种子，也没有项目自定义 DRBG。材料字节来自进程所用 OpenSSL RAND provider 的隐式播种与重播种状态。S35/S39 记录的构建 OpenSSL 为 3.0.13；S43 没有重新构建或现场检查部署 provider/entropy source。把真实带替换成理想带时还需假设初始 DRBG seed/entropy 状态在给定公开 tuple 与固定 T 签名密钥后独立于它们；代码本身没有建立此初始化独立性。

### 4.2 Beaver triples

`:519–538` 一次请求 `g` 个字节。对 gate `i` 的一个字节 `r_i`，只取低五位得到独立候选 bits `(a_i,b_i,a_i^0,b_i^0,c_i^0)`，令 `c_i=a_i∧b_i`，输出：

```text
a_i^1 = a_i ⊕ a_i^0
b_i^1 = b_i ⊕ b_i^0
c_i^1 = c_i ⊕ c_i^0
```

若 `r_i` 是均匀字节，则这五位为独立均匀 bits；剩余三位丢弃。对任一 gate，六个 share bits 的联合 law 为：

```text
Pr[(a0,a1,b0,b1,c0,c1)=v] = 2^-5  若 a=a0⊕a1, b=b0⊕b1, c=c0⊕c1=a∧b；
                              0  其他情况。
```

不同 gate 使用不同字节，故在理想 tape 下独立；`a,b` 均匀独立，三个 P0 share bits 均匀独立于 `(a,b)`，P1 share 按异或确定。该陈述是本地生成算法对 Beaver joint law 的代数推导，不借用通用 GMW 定理。

### 4.3 permutation 与 shares

`:539–544` 初始化 `permutation[i]=i`，然后对 `i=m,…,2` 执行 Fisher–Yates swap，索引由 `next_uniform(i)` 采样。`next_uniform` 在 `:254–262` 取新的 32-bit RAND 输出 `v`，阈值 `t=2^32 mod i`，拒绝 `v<t`，否则返回 `v mod i`。接受区间大小是 `2^32−t`，可被 `i` 整除，所以在每个 `i` 的独立均匀 32-bit 抽样下，返回值在 `[0,i−1]` 上严格均匀；Fisher–Yates 因而输出均匀 `π`。

矩阵 share tape 是 `:540–541` 单独填充的 `permutation_random`，长度 `ceil(m²/8)`；洗牌抽样发生于不同 RAND 调用，**并未复用此缓冲区**。`:545–553` 令每个 cell 的秘密位为 `P_π[row,original]=[row=π[original]]`，P0 share 为该独立 tape 的对应位，P1 share 为 `P_π⊕P0`。理想 tape 下精确联合式为：

```text
Pr[π,R0,R1] = 1/(m!·2^(m²))  当 R0∈{0,1}^(m²) 且 R1=P_π⊕R0；否则为 0。
```

triple tape、permutation share tape 和 shuffle draws 是同一个理想 IID byte stream 中互不重叠的区间，因此彼此独立。注意两种 padding：原始 `permutation_random` 最后一个 byte 中矩阵之外的高位也被采样，但随后丢弃；序列化 permutation-share byte 中未使用的 padding bits 则由零初始化并保持为零。理想格式只包含后者的固定零编码，不能把已丢弃的原始高位当作 package 输出或额外材料。

### 4.4 包、签名、上下文和材料摘要

receipt format 是 escrow state parameters 中的签名字段，不能与外层 context object 的 `format_version=1` 混淆。S35 只接受 `state.receipt_format_version=1`；S39 的显式 v2 构建接受 1 或 2。两者的 context 序列化函数未变，但选择 v2 时签名 body、context hash 和签名 bytes 会随字段值确定性改变。以下 exact-coupling 结论均要求 `F_T/v1` 与真实 Dealer 使用同一 receipt-format 参数；若既有 ideal interface 将它固定为 1，则 S39 v2 配置不属于 byte-for-byte 相同的 ideal output law，需要在理想接口的公开输入/参数中明确该状态值后再比较。该值是公开 setup 状态，不新增 `L_S29`。

`:555–594` 将各 party 的 packed triple bytes 与 permutation share bytes 连接，计算 material SHA-256；`:314–346` 按固定大端编码写入 package header：magic/version、Q20.12 `(32,12)`、session/invocation、N/K、party identity hash、recipient RSA public-key hash、A identity hash、party id、gate count/材料长度、circuit/gate-schedule hash、material hash 和 T signer fingerprint。route 由固定 circuit description（`:33–38`）间接绑定，gate id 由 schedule 与 count 绑定。

`:149–180` 对 `domain || NUL || format_version || canonical_body` 调用 Ed25519 签名。party package 使用 `.../party-package` domain，escrow context 使用 `.../escrow-context` domain；party package format version 是 4，context version 是 1。Ed25519 算法的签名 nonce 从 private key 前缀和消息摘要确定性产生，RFC 8032 给出具体签名算法；在固定 key、同一消息、符合 Ed25519 的 signer oracle 下，签名字节是同一个确定性后处理。该映射不需要把 T 私钥交给 party simulator。签名算法的真实性/抗伪造仍属于不同的恶意篡改证明义务。[RFC 8032 §5.1.6](https://www.rfc-editor.org/rfc/rfc8032.html#section-5.1.6)

`:575–590` 创建 metadata-only context：它包含同一 tuple、party/A identities、circuit/schedule、两份完整签名 package SHA-256、按 P0/P1 顺序计算的 `package_set_hash`、signer fingerprint 与 escrow 状态参数，不含 triples/permutation/input/output shares。context hash、材料 hash、package hashes 和签名均是对 coupled package bytes 的确定性后处理。S43 不依赖 SHA-256 输出“看似随机”隐藏材料；在两实验室字节相同的耦合下这些摘要逐字节相同。SHA-256 collision/second-preimage 与 Ed25519 EUF-CMA 只在额外的恶意篡改/冒充模型下作为绑定优势项，不用于本轮 honest issuer 输出分布相等。

## 5. 混合序列与条件优势

令 `W_T` 是一次 Dealer 成功调用实际由 `random_bytes` 交付的有序字节流，含 triple 请求、矩阵-share 请求和自适应长度的 shuffle rejection 请求。令 `G(P,W_T)` 表示上文所有材料生成、格式序列化、hash、签名和 context 生成。

| 游戏 | 操作 | 裁定/损失 |
|---|---|---|
| `G0` | 固定公开 tuple `P`、固定 T 签名密钥和固定部署环境；环境中每次 RAND 请求均成功返回，且成功条件不依赖抽样输出值；由 OpenSSL RAND 返回材料字节 | 源码事实；这里固定成功环境而不按随机输出值筛选成功样本 |
| `G1` | 保留同一成功调用图与所有确定性函数，将 RAND 返回的有序字节替换成 IID uniform bytes；假设初始 seed/entropy 状态在给定 `P` 和固定 T 签名密钥后独立 | 对该随机字节替换调用项目所部署 DRBG 的联合 indistinguishability 假设 `Adv_DRBG_T(q)`。若要把 RAND API 失败也纳入定理，须另建 abort/失败游戏；本 hybrid 不以输出相关事件条件化。OpenSSL API 文档和函数名不是此假设的证明 |
| `G2` | 将 `G1` 生成的完整双 party package、context、digest、signature、初始 lease 与 `F_T/v1(P)` 输出比对 | 在理想 tape、固定 T 签名 oracle/公钥、同一规范序列化、同一 receipt-format 参数以及相同 Fisher–Yates/材料算法时，分布**精确相等**；S35 v1 对比 S39 v2 时，receipt-format 状态字节是显式且预期的确定性差异；无 hash “随机性”推论 |
| `G3` | 将相同 packages 交入在线 party，计算公开/私有协议 transcript | 若固定相同 parties 输入 shares、peer 行为及其余 RNG/TLS tape，真实 Dealer 和 `F_T` 的 package bytes 相同会逐值传递；这只把 Dealer hybrid 的差异限制在 `Adv_DRBG_T(q)`，不能替代在线 GMW transcript sampler 或 TLS bridge |

**在 receipt-format 参数匹配后，首个未闭合概率式：**

```text
Law(W_T | P, fixed-environment RAND-success execution,
    lifecycle, concurrent RAND users)
    ≈ Law(U | P, fixed-environment RAND-success execution)
```

所缺不是 triple 公式，而是证明真实 OpenSSL DRBG 这段**多次、可能自适应长度、被材料哈希/签名及后续在线 transcript 观察的完整输出流**，在固定输入与部署状态条件下与均匀 tape 的计算距离上界。假设还须明确初始 seed/entropy 与固定公开 tuple、固定 T 签名密钥独立；“成功路径”指固定环境中 RAND 请求成功且成功判定不按随机输出选择样本，不是对一个可能依赖随机输出的成功事件做条件分布。需要具体 provider/entropy seeding、初始与重启状态、fork、安全 reseed、跨调用/线程并发、RAND 调用量上限和符号/量化优势界。OpenSSL 3.0 文档说明 `RAND_bytes` 使用 CSPRNG，自动 reseed 依赖所配置 seed source，失败时可进入错误状态；代码检查 API 错误，但项目未固定或审查部署 seed source。[OpenSSL RAND_bytes](https://docs.openssl.org/3.0/man3/RAND_bytes/), [OpenSSL EVP_RAND reseeding](https://docs.openssl.org/3.0/man7/EVP_RAND/)

当前 `next_uniform` 的重试次数没有固定上界。对 `i≤m≤256`，`t_i=2^32 mod i≤i−1≤255`，每次拒绝概率 `t_i/2^32`；故 expected draw count 为 `1/(1−t_i/2^32)`，总随机字节期望受 `g+ceil(m²/8)+4(m−1)/(1−(m−1)/2^32)` 上界控制。它仍有非零概率继续任意多轮，不能据此给 strict worst-case polynomial query bound。版本化修正候选：为每个 `i` 限制最多 `r` 次抽取；在 `N≤256` 下总拒绝概率可用

```text
δ_reject(r) ≤ Σ_(i=2..m) (t_i/2^32)^r ≤ (m−1)((m−1)/2^32)^r
```

上界，超过上限时明确发材失败；被接受的候选索引仍均匀。这个改动会新增发材失败分支，不改变泄露、材料格式、角色或输出，但需在新版本 F_T/Dealer 规格和测试中登记。本轮未实施。

## 6. 签名绑定、消费、拒绝和 restart 的边界

### 6.1 包/tuple/签名绑定

S39 package header 与 context 均绑定 session、invocation、N/K、party/A identity、recipient public key 摘要、circuit/schedule、party role 和材料/包集合摘要。`validate_package` (`score_to_mask_v3_gmw.cpp:596–641`) 对期待 tuple、party、budget、包长度、材料 digest、signer fingerprint 和 domain/versioned signature 做相等/验签；context 校验同样检查 magic/length/purpose/version 和签名。证据支持的是**代码绑定映射**，不是 T 私钥管理、验签公钥 provisioning、恶意 issuer 安全性或完整供应链证明。

### 6.2 单次 lease

party 调用先打开并验证 package，做预算、通道配置、recipient-key hash 检查，再在发送 READY 前调用 lease (`score_to_mask_v3_party.cpp:458–502`)。`score_to_mask_v3_consume_package_lease` (`score_to_mask_v3_gmw.cpp:765–808`) 在本地 SQLite 建 `consumed` 表，唯一键为 `package_hash`，同一记录还含 session、invocation、party；`BEGIN IMMEDIATE` 插入后 COMMIT。插入重复会拒绝；随后重读 package，如果 hash 变化则失败但 lease 保持 burn。这个成功/重复拒绝顺序可由源代码核验。

范围限制：此机制要求同一个未回滚 lease DB 持续使用。删除/回滚 DB、换新 lease dir、拷贝数据库 fork、重装恢复、同 tuple 下另行发一套不同 package（其 hash 不同）不受单一 package-hash 唯一键阻止。Dealer 的 `score_to_mask_v3_dealer_issue_packages` 没有跨请求的 invocation issuance ledger，也没有按 tuple 幂等重放已有 bytes 的路径。因此 S35 `F_T/v1` 的“单次调用未消费 lease 初态”可以映射到成功执行；同 tuple 多次发材/重试/rollback 的整体 one-shot 语义尚无 F_T 状态机定义，不能从当前实现或 WAL/FULL 推导掉电耐久性。

### 6.3 issuer 输出创建与 restart

`durable_create` (`score_to_mask_v3_gmw.cpp:287–311`) 使用 `O_EXCL`、权限 `0600`，写完单个文件后 `fsync` 文件并 `fsync` 父目录。发材函数按 P0 package、P1 package、context 的顺序分别调用 `durable_create` (`:591–593`)；这不是覆盖三个文件的原子事务，也没有 journal/rollback。若第二/第三次创建失败或进程中止，之前的文件可能留存；同路径重试会因 `O_EXCL` 拒绝，而换新路径可能产生同 tuple 的另一套新随机材料。没有故障注入或掉电试验，以上是源码读出的失败状态，不是本轮实测自然故障。

这些分支不在“单次完整成功发材”的 joint-law equality 内。若项目需要将 restart/retry 纳入 `F_T/v1`，最小后续版本化方案是定义 tuple-keyed issuance journal 和原子发布：先生成/签名整套文件到私有临时 bundle，文件与目录同步后以单一 manifest/目录原子切换发布；同 tuple 重试返回同一完整 package set，或明确失败且不泄露部分包。需额外定义 journal 损坏/回滚与跨设备复制。当前不做此变更，也不把当前逐文件 fsync 写成已证明掉电耐久。

### 6.4 随机性失败和其他 rejection

公开参数、路径别名、Escrow 状态、密钥格式错误会在生成/落盘前抛错；RAND 错误返回同样被检查并停止。签名/序列化出错发生在 `durable_create` 之前。文件创建/写入/同步阶段则可能留下前述部分结果。源码没有定义 F_T 错误/拒绝输出 law，也没有记录所有发材 rejection/restart 测试，所以 `F_T/v1` 的单次成功分布不能延伸到上述环境失败前缀。

## 7. 门禁状态与范围

完整机器可读结果见 [S43 gates](M6B_I_PROJECT_DERIVED_SELECT_5ITER_S43_GATES_2026-10-22.json)。

| Gate | 本轮状态 | 范围 |
|---|---|---|
| `S42_SOURCE_RECEIPT` | `PASS` | S42 85 项清单、5 个 artifact 与固定 Git 树已重核 |
| `F_T_TO_DEALER_JOINT_DISTRIBUTION` | `CONDITIONAL` | 理想 IID tape 下单次成功发材精确匹配；真实 RNG hybrid 未闭合 |
| `RNG_DRBG_BRIDGE` | `OPEN` | `Adv_DRBG_T(q)`、调用上限、部署 entropy/provider/lifecycle 尚未证明/量化 |
| `SIGNATURE_BINDING` | `PASS_WITH_SCOPE` | 固定 key 下且 receipt_format_version 参数相同，字段/域/格式与确定性签名后处理对应；恶意 authenticity 项分离 |
| `ONE_SHOT_CONSUMPTION` | `CONDITIONAL` | 同一未回滚 SQLite lease DB 内的单包消费映射；DB rollback/重复 tuple issuance 未闭合 |
| `PRIVATE_CHANNEL_BRIDGE` | `OPEN` | 本轮未重证 TLS/私有通道；沿用 S42 以外独立门 |
| `PARTY_VIEW_COMPOSITION` | `OPEN` | S42 仅复核应用/API 成功投影；未把真实 Dealer/RNG/TLS/GMW 完整联合论证组合为安全结论 |
| `CRASH_RELIABILITY_SCOPE` | `NOT_PROVED` | 任意 crash-prefix 不纳入定理；自然业务读写故障历史为 `NOT_REPRODUCED`；掉电/介质损坏未验证 |
| `SECURE_ALIAS_READY` | `NOT_READY` | 不创建 alias；v3 默认仍 OFF |
| `PERFORMANCE_READY` | `NOT_READY` | N>256 `NOT_ADMITTED`；正式 LAN/WAN/RTT 未运行/未测量 |

## 8. build、test、性能和独立复核

- 本轮 build/test：`NOT_RUN`。无代码改动、无具体可复现缺陷，不重跑 S35 195 或 S40 211 案，也不构建全仓。历史测试只作为继承证据并由 S35/S40 manifest 引用，不冒充本轮运行。
- 形式/概率核查：本轮按源码公式手工推导 Beaver 联合 law、Fisher–Yates rejection law 和 permutation share 联合式；没有编写或运行 toy enumeration、概率脚本或 fuzz 测试。
- 性能：`NOT_MEASURED`。没有启动 LAN/WAN、RTT 或正式 performance matrix。五个算法迭代、比较依赖层、应用 exchange phase、网络 RTT 不互相替代。
- 自然业务阶段读写错误：继承 S40 `NOT_REPRODUCED`；S43 未复跑，也没有把故障注入称为自然故障。
- 非作者复核：`/root/s39_crypto_review` 未参与本轮证明撰写，完成了上述范围的只读复核，未改文件、未构建、未运行测试。其裁定为理想 IID tape 下单次成功发材联合律 `PASS_WITH_SCOPE`；真实 OpenSSL DRBG bridge 仍 `OPEN/CONDITIONAL`。这只复核理想化局部联合律，不构成整个 `F_T_TO_DEALER_JOINT_DISTRIBUTION` 的 reviewed pass，更不构成完整协议安全通过。复核意见要求的 diff 范围、padding、receipt-version、seed 独立性和成功条件澄清已并入本报告；复核者未重新检查这些编辑，也未运行测试。
- 未验证/未量化：实际 entropy 质量、OpenSSL provider 配置、跨进程/线程调用模型、fork/reseed/并发、`Adv_DRBG_T(q)` 参数；恶意签名替换与 hash binding 优势；WAL/FULL 掉电耐久；失败 issuance 的自动恢复；TLS/private-channel 优势；全协议 party-view simulator。

## 9. 最小下一步

1. 已完成的非作者复核仅覆盖本轮局部联合 law；若后续实质性修改条件分布推导，应再次请求独立只读复核。
2. 若要把 `F_T_TO_DEALER_JOINT_DISTRIBUTION` 从条件提升为端到端桥，先固定部署的 OpenSSL build/provider/seed source 与 T 进程生命周期，并指定可证明的多次/并发 DRBG 安全定义和 query bound；必要时新版本引入 bounded rejection 及 `δ_reject`。不新增泄露、不改变输出、角色或材料 ABI。
3. 单独决定是否把发材重试/崩溃恢复定义成 `F_T/v2` 状态转移；若需要，再实现 tuple-keyed atomic bundle journal，并新增仅覆盖该状态机的故障回归。没有这项决策前，不把当前部分写入路径称为全局 one-shot/掉电耐久保证。

不得创建 `secure` alias，不改变路线名，不把 S43 的条件生成律称作完整安全 PASS 或论文 BB90 复现。
