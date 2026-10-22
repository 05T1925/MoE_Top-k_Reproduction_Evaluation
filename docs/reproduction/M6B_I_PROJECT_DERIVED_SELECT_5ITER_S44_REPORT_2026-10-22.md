# M6B-I-PROJECT-SELECT-5ITER-S44：Dealer 随机字节流桥与拒绝采样时间模型

日期：2026-10-22
路线：**Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL**
裁定：**理想 IID 字节带下的 S43 联合材料律经本轮非作者复核，保留 PASS_WITH_SCOPE；真实 OpenSSL 联合 RNG 桥为 OPEN；S39 的 next_uniform 仅能证明 expected-polynomial time，不能满足本路线现有的 strict-PPT 证明先例。**
运行时代码：**未修改**；构建/测试：**NOT_RUN**；正式性能：**NOT_RUN / NOT_MEASURED**。

## 1. 结论

本轮固定 S43 提交 a1b02a8133955786e0bb6b047f55b746af66acbf，沿 S39 固定 Dealer/runtime 提交 8028c0fece69fe0c648b1e0f1e12ad7fca723f20 审查。实际 Dealer 从 OpenSSL RAND_bytes 取得一条依赖调用顺序的字节带：先生成所有 Beaver triple 随机字节，再生成独立 permutation-share 字节，随后按 Fisher–Yates 位置逐次抽取 32-bit 值并拒绝越界前缀。理想 IID 字节带下，S43 的 triple/permutation 联合采样结论仍成立；本轮不重做该证明，只复核 S43 最终文字及其范围。

真实材料桥的首个概率步骤仍要求：在给定公开调用 tuple、T 签名密钥和固定部署环境后，整段有序 RAND 输出（包括多次 API 请求、可变长度拒绝尾部及并发/重播种影响）与理想 IID 字节流计算不可区分。仓库只记录过某次构建环境使用 OpenSSL 3.0.13，没有当前部署的 provider、seed source、entropy source、RAND 配置或 T 进程生命周期证据。因此不能从函数名或 OpenSSL 文档推出本项目的 Adv_DRBG_T，也不能给出具体数值优势。

拒绝采样方面，S39 固定源在每个排列位置使用 for(;;)，没有次数上限。对 N≤256，单次调用期望消耗有限且为多项式，但任何有限次数的重试上界都存在非零超限概率；故代码不满足标准 worst-case strict PPT。S38 的先例明确为归约中的 Fisher–Yates 设置每位置 λ 次上限并计入截断统计距离；本项目没有找到接受 expected-PPT 的一般决策。因此本轮不把 expected-PPT 当作已获批准模型，门禁保持 OPEN。最小候选是单独版本化的 F_T/v2 bounded-rejection profile，令每个位置最多 r(λ)=λ 次，超限时在任何 package 落盘前统一返回 issuance-abort，并将 δ_reject 写入功能和安全优势。该方案会改变 F_T 的失败分布及签名配置，需要另立版本，不在本轮实现。

这项裁定不是攻击、运行时缺陷或安全否定；它说明真实 OpenSSL bridge 仍缺部署假设与联合安全界，而当前无界实现无法在项目既有 strict-PPT 证明口径下获得有限 query bound。

## 2. 固定对象、来源复核与范围

### 2.1 提交和来源

- S44 起始 HEAD：a1b02a8133955786e0bb6b047f55b746af66acbf；父级 S42：72a23fc067b5bfbf10edf110eeff45ec1f0ed6bd；S43 tree：2b58e2a02ec9668cfbd9e6ba7419f76142f7f828。起始分支为 codex/m6b-i-project-select5-s43，起始工作树干净。
- S35 runtime：77f9684e1a50794004fd31f3e5ee7f193e9b12ec。S39 被测 Dealer/runtime：8028c0fece69fe0c648b1e0f1e12ad7fca723f20。S40 功能证据：b04618117b11cffe0e899947726ab1c66807ffdd。S39 是 S43 的祖先。
- S43 source/build/evidence manifest 原始 SHA-256：9f8064b90a9c52a45d3c7d3488deca075497544de342c6f45a30fb1f0ceee320。在 S44 entry、修改治理文件之前，S43 manifest 中七项 deliverable 的原始字节哈希均经核验；其中 decision、report、gates、S41 erratum 四项当前仍与记录相符。PROJECT.md、IMPLEMENTATION_PLAN.md、REFERENCE_MANIFEST.md 随 S44 增加治理记录后哈希改变，当前值另列于 S44 source/evidence manifest。
- 直接从 S39 Git tree 读取并复算 S43 manifest 中七个核心 Dealer/runtime source blob 的 SHA-256，7/7 匹配。关键文件 VFSS/src/moe_topk/score_to_mask_v3_gmw.cpp 为 527440a7bb9fa0b08d37def3f1a471f357eb22238728c5b141fb20b7b4b9fc60；其头文件 VFSS/include/moe_topk/score_to_mask_v3_gmw.h 为 48e4872201c25061a030d44d4427798c61bd29702c8cd8f332e06da837d1676f。S39 source manifest 的 85 项规范哈希仍为 09444abd50c41357eb63e9050b7e39c3b2948588205d9d70f02561d7d8ec49d9；运行时来源直接由 S39 commit/blob 固定，没有用当前 main 或同名文件替代。
- S41 provenance erratum 保留不动。另发现 S43 manifest 嵌入的 S42 85 项 source rows 自身 canonical SHA-256 可重算为 2ea2896cce7dd284f3d317a7462dde528c188cc15cb32af16c744dad4e5e6837，但其中 PROJECT.md、docs/IMPLEMENTATION_PLAN.md 两行不能从 S42 Git tree 的 blob 或普通 Windows CRLF checkout 重建；其余 83 行可按 S42 commit 重建。具体值为：PROJECT 记录 924c989e4475b3129d777859d270b6b228e20f294b834568e0522947a8f5b0d8，S42 Git blob 为 b1a146858cdf7d60bb0985ff32bc61d969f02d80028fd2a8559019ce6d088a65；IMPLEMENTATION_PLAN 记录 281724be8352d89767ebb5896eed70f4e5fda401ff231c771c64d6f8466cd343，S42 Git blob 为 e5700651ea981bbc3cf7705c9a3437220f0427e2b687a47d801b002f42b27d7b。现有记录不足以确定这两行差异的生成原因；S44 将它们标为独立的 OPEN_METADATA_DIVERGENCE，不改写 S42/S43 历史。它们是治理文档字节身份问题，不是 S39 Dealer 源码冲突；S39 Dealer 仍有直接 commit/blob 身份。故本轮 RNG 源码审查继续以精确 S39 Git 对象为准，未用这两行反推 S39 runtime。
- S43 reviewer /root/s39_crypto_review 的独立复核未覆盖意见并入后的最终文字。本轮由未参与 S43 推导的 /root/s43_final_text_review 只读检查了最终 S43 报告、decision、gates 和 manifest，确认理想 IID tape 的局部联合律仍仅为 PASS_WITH_SCOPE、真实 DRBG 桥 OPEN；未修改文件，未运行构建或测试。该复核允许本轮把理想 IID 联合律作为条件前提，但不提升为完整安全结论。

### 2.2 固定证明模型

对单次成功发材调用，令公开 setup 为：

~~~~text
P = (protocol, session, invocation, N, K, fixed route/circuit/gate schedule,
     party identities, recipient-key hashes, escrow identity/state including
     receipt_format_version, fixed T verification key vk_T)
~~~~

下文 `sk_T` 表示 T 持有的固定签名私钥；它不交给在线方，`vk_T` 才是公开 setup 的验证键。随机带假设必须覆盖二者与播种熵之间的相关性。

令 E 只表示部署配置与生命周期策略：OpenSSL/libcrypto build、provider、RAND 配置、libctx、配置的 seed/entropy source 类型、进程/线程/fork/restart/reseed/并发策略及 API 错误处理策略；E 不包含实际熵样本或初始 seed。令 S 表示隐藏的初始 seed/熵输入及之后各次 reseed 熵输入，不包括由这些输入和调用历史演化得到的 DRBG 内部状态或输出。给定 E 后，S 按部署熵/播种模型产生，并需满足与公开 tuple P、固定 T 签名私钥 sk_T 独立；例如需要 `Law(S | P, sk_T, E)=Law(S | E)`，或由明确的熵源定理推出对应条件。不能把实际取定的 seed/熵输入固定进条件，因为这会消去待证明的随机性。给定 S、E 和调用历史，演化的 DRBG state 再用于生成 W_T，即 Dealer 这次调用按实际顺序获得的全部 RAND 字节；若其他调用可并发交错，则须以 provider 的整个多调用状态转移及 Dealer 所取片段建模，不能擅自把 W_T 当作隔离的全局前缀。理想带 U 与 P、sk_T 和 E 独立，且各字节 IID uniform。

成功路径比较必须对隐藏 S/熵输入求平均，不能再条件于其实际实现值。若以“本次所有 RAND 请求均成功”条件化，还必须在所采用的 DRBG 假设中显式涵盖该条件事件及其对 W_T 分布的选择效应；不能默认 API 成功与输出随机带独立。另一种可审查定义是比较 `(返回字节、成功/失败状态)` 的完整联合 transcript，并在两边统一失败策略。RAND_bytes 错误、启动失败、进程中止、重启、并发竞争和落盘失败没有在本轮单独建模或验证。OpenSSL 对 RAND API 的 CSPRNG/DRBG 描述不能替代对隐藏 S 求平均后、针对 P、sk_T、E 的完整联合界。

### 2.3 条件 hybrid 与优势

令 G(P,W) 为源码从随机带计算三材料、两份 package、材料/包/context 摘要、签名以及之后确定性可见字段的算法。

1. G0 使用真实部署中、对隐藏 S 求平均后的 OpenSSL RAND 完整有序输出 transcript W_T。
2. G1 在同一自适应调用调度下用无限 IID uniform 字节带 U 回答请求；调用者从 U 中消费的总字节数 Q 由先前的输出（包括拒绝结果）决定。令 `T_U` 为按相同 API 分块/状态规则生成的调用者 transcript。需要部署适用的多调用 DRBG 联合不可区分假设，记作 `Adv_DRBG_T(Q,C;E)`，其中 Q 是总输出字节量、C 是 RAND 请求数；E 固定配置但不固定实现 seed/state。该假设覆盖对隐藏 S 的分布、成功条件化或状态返回策略、所有可观察的确定性后处理，以及并发输出的聚合视图，而不是只证明每次 RAND_bytes 的边缘均匀性。若 DRBG 定义不含成功条件化，则本式不能直接用于成功路径定理。
3. G1 到理想 F_T/v1：在 S43 的 ideal IID tape 单次成功发材联合律、签名 key/oracle、规范序列化和公开 receipt_format_version 匹配时精确耦合，增量为 0。签名、hash 与包字段是确定性后处理，不需假设 SHA-256 输出均匀；这也不证明恶意签名替换、hash binding、TLS 或完整 party view。
4. 对假设性的有界版本，局部完整视图结论应以匹配版本的理想功能为目标：
   `|Pr[D(View_real_v2)=1]−Pr[D(View_F_T/v2)=1]| ≤ Adv_DRBG_T(Q_max,C_max;E)`。
   这是计算不可区分界，不是统计距离；成功条件化只有在上一步所述 DRBG 游戏明确支持时适用。定义 `π_mat` 为解析 package 并只保留语义材料、丢弃 sampler/version 字段、签名与编码元数据的投影。对该投影，显式失败 `F_T/v2` 与无失败 `F_T/v1` 的语义输出距离至多 `δ_reject`；故若比较的是投影后的有界真实视图与 `F_T/v1` 语义材料，界为 `Adv_DRBG_T(Q_max,C_max;E)+δ_reject`。不同 version/profile 字段使两个版本的完整 package/signature 视图确定性可区分，不能声称全视图 TV≤δ。私有通道/后续 party-view 项必须另列，不能合并成 RNG 结论。当前 v1 源码下 Q、C 没有最坏上界，不能把期望 Q 直接代入严格固定 query 的 DRBG 定义；因此本轮不给当前 v1 一个有限 worst-case DRBG hybrid 界。

## 3. 真实 Dealer 调用链和字节消费

S39 源码行号是便于定位的行号；对象身份以 S39 commit 和 source manifest 为准。

| 阶段 | 源码位置 | 请求字节/次数 | 用途与关系 |
|---|---|---:|---|
| request helper | score_to_mask_v3_gmw.cpp:234–241 | 每次最多 2^20 字节，按请求分块；RAND_bytes 返回值非 1 即抛异常 | 不自行播种，不调用 RAND_poll/RAND_add，不手工 reseed |
| Beaver triples | :519–538 | g 字节，g 为 gate_count；大请求被拆成 ceil(g/2^20) 次 RAND_bytes | 每 gate 取低 5 bit 形成 (a,b,a0,b0,c0)，高 3 bit 丢弃；三组 share 编码不改变调用数 |
| permutation-share tape | :539–541 | ceil(m²/8) 字节，m=padded_n；N≤256 时只需一次 RAND_bytes | 独立缓冲区为 P0 的 permutation share；尾字节未用高 bit 被丢弃；后续序列化 padding 为固定 0 |
| Fisher–Yates | :254–262、:542–544 | 每个位置 i=m,…,2 每次尝试 4 字节、单独一次 RAND_bytes；拒绝则继续请求 | 32-bit v；t_i=2^32 mod i；v<t_i 拒绝，否则 v mod i |
| package/context 后处理 | :555–594 | Dealer 明文路径没有更多直接 RAND 请求 | hash、签名、包/context 序列化在内存中先完成；只有全部随机材料与包/context完成后才依次 durable_create |

这里 m 是至少 2 的最小 2 次幂且 m≥N，g 的精确式来自 score_to_mask_v3_gate_budget（:420–439）：

~~~~text
w = 32 + log2(m)
pairs = m(m-1)/2
g = 63m + m²w + pairs(2w-1) + m²
q_base = g + ceil(m²/8)
Q = q_base + 4 Σ(i=2..m) T_i
~~~~

T_i 是位置 i 的独立提议次数。令 t_i=2^32 mod i，ρ_i=t_i/2^32；在理想 IID 带下 T_i 为接受率 1−ρ_i 的几何变量，E[T_i]=1/(1−ρ_i)。不同位置使用流中不重叠的 32-bit 字，条件尾部仍独立；故 shuffle 输出均匀，且不与先前独立 matrix share tape 复用随机值。这沿用 S43 理想 law，经本轮非作者复核，不把单段边缘均匀外推成真实 DRBG 结论。

N=128、256 的字节预算（不是测量结果）：

| N | m | g / triple 随机字节 | permutation-share 随机字节 | Fisher–Yates 位置 | q_base | 理想带下 E[Q] |
|---:|---:|---:|---:|---:|---:|---:|
| 128 | 128 | 1,289,280 | 2,048 | 127 | 1,291,328 | 约 1,291,836.000003 |
| 256 | 256 | 5,281,664 | 8,192 | 255 | 5,289,856 | 约 5,290,876.000014 |

上表 expected Q 按 q_base+4Σ1/(1−ρ_i) 推导；不是 worst-case query bound。N=256 时 RAND_bytes 对 triple buffer 有 6 个分块请求、share buffer 1 次，再加 255 个或更多各自独立的 4-byte 请求。因此 API 请求数期望约 262.0000035，但最大请求数无界。Gate count 和 q 数是源码公式推导，不是新 build、bench 或运行记录。

S44 范围内 Dealer 源文件的直接 RAND_bytes 调用只有 helper 内一次；该 helper 的 Dealer callsites 是 triple、permutation-share 和 next_uniform。相同 receipt/v3 代码家族的其他随机性单独列出，以免误认为由本轮 Dealer tape 覆盖：score_to_mask_v3_party.cpp:169 为 party seed，:380–381 为 envelope key/nonce，:696 为 output refresh；score_to_mask_v3_escrow.cpp:252 为 A 的 receipt token。它们属于 party/A 随机带及既有 v2 token 引理，不是 T 的材料带。本仓库其他协议模块也有 RAND_bytes 调用，但不属于被固定 S39 Dealer 实现；本轮没有把它们纳入一个共同 DRBG 定理。

## 4. Provider、熵源、生命周期与错误路径

实际源代码只调用 RAND_bytes，未在 Dealer 内设置 OpenSSL provider、EVP_RAND、seed source、libctx、reseed interval、prediction resistance 或 fork handling，也未设置并发互斥。对所有应用层调用位置的检索没有发现 Dealer 旁路使用另一套材料 RNG。Dealer API 自身不声明禁止并发签发；线程调度和外部 OpenSSL RAND 使用由进程/部署环境决定。

S43 继承的构建记录写明 OpenSSL 3.0.13，但这只是历史 S35/S40 构建环境，S44 没有重新构建或读取部署实例；不代表部署版本。OpenSSL 3.0 官方文档描述默认 RAND API 与 public/private/primary DRBG 的关系、按 build/config 选择 seed source、自动 reseed 和错误状态。具体是否使用默认 RAND_OpenSSL、哪个 provider/seed source，以及进程中是否被替换/配置，当前无部署证据，状态 OPEN。[OpenSSL RAND_bytes](https://docs.openssl.org/3.0/man3/RAND_bytes/)、[RAND_get0_primary](https://docs.openssl.org/3.0/man3/RAND_get0_primary/)、[EVP_RAND](https://docs.openssl.org/3.0/man7/EVP_RAND/)、[EVP_RAND-SEED-SRC](https://docs.openssl.org/3.0/man7/EVP_RAND-SEED-SRC/)

首个缺失的部署定理仍是（其中分布对隐藏 S 求平均，不固定实际 seed/state）：

~~~~text
Law(T_real | P, sk_T, E, Suc_real)
  ≈_c Law(T_U | P, sk_T, E, Suc_IID)
~~~~

这里 `T_real` 含 RAND 返回字节、调用边界和状态；`Suc_real` 是成功发材事件，`Suc_IID` 在理想成功 API 中概率为 1。该条件分布优势必须对 S 中的初始/reseed 熵输入求平均，同时明确条件化选择效应；也可改用不条件化的 `(bytes,status)` 联合游戏并在两侧定义相同失败语义。要把这个式子变成当前部署的条件定理，需要：具体 OpenSSL/libcrypto build 与 provider 配置；seed source 和 OS entropy 可信性/初始化独立性；T 进程 fork、restart、reseed 与并发策略；RAND 请求错误与输出流的联合行为；同一 DRBG 状态下所有其他调用的聚合查询预算；以及严格定义的 Adv_DRBG_T(q) 假设。OpenSSL 文档称 API 由 CSPRNG 生成且需要成功播种，并建议检查返回值；这解释源码为什么检查错误，不提供这些项目部署条件的证明，也不导出一个本项目数值安全位数。

失败路径：若任一个 RAND_bytes chunk 返回非 1，random_bytes 抛出异常；Triple、permutation 和 shuffle 抽样都发生在 durable_create 之前，因此该类 RNG 错误不应先写出 package 文件。随机数组/材料向量在内存中的清零没有由本轮证明；失败后重试、同 tuple 再签发、进程重启后 RNG 状态、错误是否重试与发行幂等均未纳入 F_T/v1。后续 durable_create 阶段仍按 S43 记录顺序写 P0、P1、context，普通文件写入错误可能留下部分产物；这不是 RNG 失败的安全证明，也未在本轮注入验证。

## 5. 拒绝采样和证明时间模型裁决

对每个 i∈{2,…,m}，一次均匀 32-bit 字拒绝概率为 ρ_i=t_i/2^32；accepted 区间大小可被 i 整除，所以每个返回值严格均匀。源码 loop 直到接受，其期望提议次数为 1/(1−ρ_i)，但不设最大次数。N≤256 时 t_i≤255，因此：

~~~~text
E[Q] = q_base + 4 Σ(i=2..m) 1/(1−ρ_i)
     ≤ q_base + 4(m−1)/(1−(m−1)/2^32)
~~~~

这证明期望输出字节与循环工作量有界为多项式。它不证明任何有限 worst-case 上界：所有 i 上持续拒绝的概率虽极小但非零，算法在某些随机带上可以继续任意久。OpenSSL/OS entropy 初始化是否阻塞也未由这项抽样期望式覆盖。

项目口径：在当前可查到的 S36–S43 proof/governance 先例中，归约器被明确要求为统一 PPT/strict-PPT；S38 为 Fisher–Yates 专门采用每位置最多 λ 次重试并给出 ε_perm≤255·2^(−24λ)。未找到项目决策明确允许 expected-PPT 或 expected-query DRBG reductions。因此 S44 裁定：expected-PPT 结论可作为复杂度观察记录，但不能按现有证明标准准入；REJECTION_RUNTIME_BOUND 继续 OPEN，直到项目采用新的 expected-PPT 安全定义或实现版本化上限。

## 6. 有界版本的最小规格候选（未实现）

建议下一版本显式标记 sampler profile，例如 F_T/v2 + bounded-rejection-v1；令每个位置最多 r(λ)=λ 次独立提议。定义：

~~~~text
δ_reject(m,r) = 1 − ∏(i=2..m) (1−ρ_i^r)
             ≤ Σ(i=2..m) ρ_i^r
             ≤ (m−1)(255/2^32)^r
             < (m−1)·2^(−24r)
             ≤ 255·2^(−24r), for m≤256
~~~~

r=λ 时该项对安全参数 λ 可忽略。若任一位置 r 次皆拒绝，T 在随机材料生成阶段返回唯一的 ISSUANCE_ABORT，不输出任何 party package/context，也不创建 one-shot lease；该检查发生在当前实现所有 durable_create 之前。失败原因与位置不返回 party transcript，避免新增长度/位置字段。这个 abort 仅是预处理发行失败合同，不等同在线两方 crash-prefix privacy，也不声称 buffer zeroization。

令 `π_mat` 将两方 package 解析为语义材料输出，并丢弃 sampler/version 字段、签名、序列化和传输元数据。成功条件下每个 accepted residue 仍严格均匀，且“全部位置在 r 次内成功”事件与所有接受值独立，所以 `π_mat(F_T/v2)` 的条件成功 law 与 `F_T/v1` 的语义材料 law 完全相同。若 `F_T/v2` 显式包含该 abort，且 abort 与成功材料作为不同输出，则 `TV(π_mat(F_T/v2),π_mat(F_T/v1))=δ_reject`。该结论不适用于完整 package/context/signature transcript：版本/profile 字段和所签字节不同，完整视图可以被确定性区分。bounded 真实实现到匹配版本 `F_T/v2` 的全包 RNG hybrid 仅计 `Adv_DRBG_T(Q_max,C_max;E)`；只有投影到 `π_mat` 后，转接到 `F_T/v1` 才可再加统计项 `δ_reject`。不能把 δ 隐藏进 L_S29，必须作为明确的发材失败概率/功能结果。

在此候选中：

~~~~text
Q_max(m,r) = q_base + 4r(m−1)
C_max(m,r) = ceil(g/2^20) + 1 + r(m−1)
N=256: Q_max = 5,289,856 + 1,020r bytes
       C_max = 7 + 255r RAND_bytes calls
N=128: Q_max = 1,291,328 + 508r bytes
       C_max = 3 + 127r RAND_bytes calls
~~~~

包格式/签名需新增 sampler-profile/version 与 retry limit 的公开绑定字段，或以另一个规范化 signed setup version 绑定；v1 verifier 必须拒绝未知 profile，不能把它当成现有 v1 package。失败语义、调用方如何观测/重试、T 对重复同 tuple 的发行语义以及包/lease 版本兼容均需单独决策。此设计不改角色、在线消息或语义材料成功 law；它新增有限概率的公开预处理 abort，并改变完整 package/signature transcript，因此不声称新旧版本全视图接近，也没有静默改写 F_T/v1。没有修改 runtime，也没有测试该候选。

## 7. 独立复核、证据和所有门禁

本轮执行者参与 S35–S43 的实现/证明/决策，属于作者。/root/s43_final_text_review 未参与 S43 推导，先只读复核了 S43 最终文字，再对 S44 初稿做非作者复核；首轮指出 E 不应包含 realized seed/state，以及计算优势不可与统计距离混写。本报告已依此修正。审查者随后复核 bounded-rejection 的投影范围、期望调用数、S43 entry-time 哈希措辞及最终报告哈希，并确认这些修正；该审查者未改文件、未运行 build/test。审查范围只覆盖上述条件式/界限/来源措辞，不是全协议安全复核。

| 门禁 | 状态 | 范围 |
|---|---|---|
| S43_SOURCE_RECEIPT | PASS_WITH_SCOPE | S43 commit/manifest/artifacts 精确；S39 七个核心文件 direct blob hash 7/7 通过。S42 内嵌两份治理文档 source row 另记 OPEN_METADATA_DIVERGENCE，不是 Dealer source collision |
| DRBG_ASSUMPTION_AND_DEPLOYMENT_MODEL | OPEN | 真实部署 provider、seed/entropy source、state/reseed/fork/thread/concurrency 证据缺失 |
| JOINT_RNG_STREAM_BRIDGE | OPEN | 缺 Adv_DRBG_T(Q,C;E)，不得由 CSPRNG 标签或边缘均匀性推导 |
| REJECTION_RUNTIME_BOUND | OPEN | 源码为 expected-PPT only；没有 finite worst-case query bound，项目未批准 expected-PPT 定理 |
| FT_VERSION_COMPATIBILITY | OPEN_DESIGN_ONLY | F_T/v1 不变；F_T/v2 bounded-rejection 规格候选未获版本/接口审查且未实现 |
| ONE_SHOT_SCOPE | CONDITIONAL | 仅同一 package 与未回滚 lease DB 的已记成功路径；global rollback/reissue/restart 不证明 |
| PRIVATE_CHANNEL_BRIDGE | OPEN | 继承，不在本轮审查 |
| PARTY_VIEW_COMPOSITION | OPEN | Dealer RNG 局部桥不替代完整 GMW party-view simulator |
| SECURE_ALIAS_READY | NOT_READY | 无 secure alias；v3 default OFF |
| PERFORMANCE_READY | NOT_READY | 正式 LAN/WAN/RTT 未运行/测量；N>256 未准入 |

## 8. 验证、未运行项和下一步

- S44 只读源码/manifest/证明审查，无 runtime 或 test 代码改动。
- BUILD/TEST = NOT_RUN；未重跑 S35 195 案、S40 211 案；未运行 toy enumeration、随机源统计测试、构建或进程 E2E。
- 正式 LAN/WAN、RTT 和性能矩阵 = NOT_RUN / NOT_MEASURED；S35/S40 既有功能证据仅继承，不将本机时间或 phase 数写作网络性能。
- 自然业务 I/O 故障仍继承 S40 的 NOT_REPRODUCED；本轮未执行人工故障注入。崩溃前缀隐私、掉电耐久和内存清零未证明。
- N>256 = NOT_ADMITTED；无 secure alias；默认 OFF；VFSS-baseline 不变。
- 无未量化优势项被填数值。Adv_DRBG_T(Q,C;E)、签名/哈希 active-security、private-channel/TLS、party-view 组合均保持各自范围。
- 最近的可执行下一步：冻结部署 OpenSSL/libcrypto build 与 provider/seed source/线程和进程 lifecycle，选定允许 expected-PPT 还是 strict-PPT；若沿用现有 strict-PPT 先例，提交 F_T/v2 bounded-rejection 功能/包格式决策，再实现并由独立者做 sampler conformance 与包兼容回归。此后才能给出有界 q 下的 DRBG hybrid；此项仍不是完整协议安全 PASS。

路线继续标记为 PROJECT_DERIVED / EXPERIMENTAL，目标仍是原序稳定 Top-K mask；不是 BB90 或 BMW16 原算法复现。五次算法迭代、比较依赖层、在线 exchange phase 与 RTT 仍是不同指标。
