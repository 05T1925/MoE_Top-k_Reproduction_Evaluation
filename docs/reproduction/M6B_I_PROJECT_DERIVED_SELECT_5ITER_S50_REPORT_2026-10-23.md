# M6B-I-PROJECT-SELECT-5ITER-S50 详细报告

日期：2026-10-23
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 1. 裁决摘要

本轮只裁决固定 S49 源码下、`1≤K≤N≤256`、单次新鲜 FTv3 package、理想 IID 发材 tape 且 `cap_success`、无 I/O/进程故障时，一方静态半诚实计算方的 GMW 成功转录前缀 `Kpre_3`。输出保持 signed Q20.12 score 降序、原始索引升序稳定同分、原序恰 K 个 1 的 Top-K XOR mask shares。S48 N≤1000 案例仅继承，本轮不重跑。

**构造结论：**在 S49 的 FTv3 两方语义材料联合律、S36 已复核的 H4 条件引理，以及 S36/S42 已记录的 `L_S29` 定义下，存在可执行的 N≤256 条件 simulator。FTv3 不拆分该输入域内任何 GMW AND layer；新增 layer/batch/count、phase、sequence、frame length 均由公开调用元组及 `L_S29` 中已批准的 active seed/outcome/endpoint/comparison-count/communication-length 确定。Beaver opening 和活动比较 bits 仍由 H4 条件采样，不加入泄露。

本构造经非作者只读复核后为 `PASS_WITH_SCOPE`。范围仅限理想 FTv3 IID 成功发行下的 N≤256 GMW pre-escrow transcript。它不关闭真实 RNG/DRBG、签名/材料发行桥、TLS、FTv3 escrow 后缀迁移、完整 party-view composition、故障前缀或发材中止合同门。

本轮未发现可复现的代码/合同缺陷；没有改运行时代码、没有构建或测试、没有性能测量，也未从局部前缀引理推出完整安全通过。

## 2. 来源身份及证据边界

按仓库 `AGENTS.md`，从本机最新 `main` `c3926c68fd14f270faa8b55234311071947fa080` 创建隔离分支 `codex/m6b-i-project-select5-s50`，再按顺序 fast-forward 到固定对象：S47 `feb5e6677568d649906f46d0e611d5d52349c07f` / tree `7c494b5cfcc67843410e683556a1792074d0ca1e`，S48 `e05b01d5c7e3899297b76a5e0fca212d38d24229` / tree `14cc78cb20080dd11326d1f9beb69279872e4aaa`，S49 `50bc223c19e24bdf052eacbd47be8df6a1d56d98` / tree `19f6febae92b8f95a15acf50c24002028723f7d0`。S49 不在 main；本轮没有从主工作区复制源码。S48 九项 canonical SHA-256 复算值 `e6dde491a1cf7696173a6690dcb65d6473b5f5b589702b84f013705eecd59e84` 与其 manifest 一致，且固定 S48 Git blobs 逐项匹配；S49 6 项 source snapshot canonical 是 `984d8f2dadec789d985f1d97ee7edb7920bc7ba1e3067c8cb75a8e39c3d60e00`。逐文件 raw SHA-256 和 Git blob SHA-1 在 S50 manifest。

桌面主工作区和历史 S47–S49 证据未改；`VFSS-baseline/` 无差异。本轮在新隔离 worktree 工作，初始状态干净。

四类证据分开：论文定义不证明本项目 Select5 是 BB90/BMW16；本地源码事实确定 FTv3 profile、sampling、AND layer/frame 与 transport schedule；项目派生推导为 S36 H4/S42 projection 到 S50 FTv3 schedule 的条件迁移；理想 IID tape、FTv3 cap success、可信 T/A 和成功执行是待验证模型假设，不是本轮实验结果。

固定 S47–S49 Git 链不含独立 S29 原始决策文件。因此 `L_S29` 采用 S36 §3.3 经审查定义及 S42 §3/gates 摘录；不从主工作区未跟踪文件补充、不扩大定义。此限制不妨碍按既有审查摘要裁决字段，但以后如要重审 S29 原始合同，需另固定该原文。

## 3. 冻结模型、权限与成功条件

完整公开调用元组记为 `P_3`：

```text
P=(protocol,session,invocation,N,K,route,circuit_hash,gate_schedule_hash,
   FTv3 profile,identities,recipient-key hashes,A identity,
   receipt-format version,public resource/state parameters).
```

FTv1 与 FTv3 的版本、sampler、package-format、schedule hash 和 receipt-format 字段不同，不能以含 FTv3 专属字段的 `P_3` 条件化 FTv1。比较语义材料律时使用共同语义参数 `P_sem=(protocol,session,invocation,N,K,route,circuit semantics,party roles,material dimensions)`；它决定材料形状，但不含 FT/sampler/package/receipt 版本字段或版本专属 schedule digest。两个完整 profile 写作 `P_1=(P_sem,profile_1)` 与 `P_3=(P_sem,profile_3)`。后文 `F3` 仍可读取本轮实际元组 `P_3`。

对 `b∈{0,1}`，`Kpre_3^b` 获得本方 `x_b,z_b,L_S29,pkg_b^3,static_b,P` 和已经产生的本方转录。理想 `F_T/v3` 只把本方签名 package 交给该方；同次 `F_TOPK` 只把本方 output share `z_b` 交给该方。Simulator 不得获 `pkg_(1-b)`、`z_(1-b)`、peer credential、A-only `ctx_A`、T 私钥、peer 随机带、A 内部状态或 reduction-only `J_b`，也没有无条件 `peer_deposit` oracle。

诚实对端由同次理想调用取得其自身 `x_(1-b),z_(1-b),pkg_(1-b)` 和凭据，按真实客户端产生交互；本方 simulator 不控制或伪造其 deposit。S50 的 `Kpre_3` 止于 output refresh 及本方 output share 形成，不包含 A 的 escrow submit/receipt/fetch suffix。

`cap_success` 是离线采样成功条件，不添加进 `L_S29`。S49 在理想 IID tape 下证明：条件于每个 bounded Fisher–Yates 位置都未耗尽重试上限，两方 semantic-material 联合投影等于 exact-uniform Fisher–Yates 发行 law。cap 失败只依赖发行随机带和公开 profile，不依赖输入、`z_b` 或 online transcript。S49 的 `δ_reject≤1023·2^-154<2^-144` 只适用于剥离版本/编码/签名后的语义材料投影；不适用于 full package/transcript。条件于成功时本轮 Kpre3 距离为 0；比较“有界发行”与“永不失败发行”时才另计 abort 质量。

## 4. 从 FTv3 材料 law 迁移 H4

S36 §4.1 的单门条件分布是核心：固定腐化方的完整本方 package、真实 wires 和全部此前 view，对未消费 gate 的全局 `A_g,B_g` 仍是独立均匀位。真实开值 `D=X⊕A_g`、`E=Y⊕B_g` 因此条件均匀。模拟器抽 `(D,E)←${0,1}²`，由本方 local triple/wires 得本方 partial `d_b,e_b`，peer partial 为 `(D⊕d_b,E⊕e_b)`，再按

```text
c_b ⊕ (D ∧ b_b) ⊕ (E ∧ a_b) ⊕ ([b=0] ∧ D ∧ E)
```

计算输出 share。它是给定 package 和历史后的联合 opening kernel，不是只证明 D/E 单变量边缘均匀。

FTv3 材料桥用两方联合等式：

```text
Law(π_sem(pkg_0^3,pkg_1^3) | P_sem,cap_success)
  = U_sem(P_sem)
  = Law(π_sem(pkg_0^1,pkg_1^1) | P_sem),
```

其中 `π_sem` 只留两方 Beaver triple shares 与 permutation shares；`U_sem(P_sem)` 是同一材料形状上的共同 exact-uniform 语义律。此写法避免把 FTv3 专属公开字段误当成 FTv1 的条件参数。等式由 S49 的 ideal IID 条件律继承。因为是完整两方联合 law，对任何正概率、有效的本方语义材料值 `m_b`，条件核满足

```text
Law(M_(1-b)^3 | M_b^3=m_b,P_sem,cap_success)
  = U_sem(M_(1-b) | M_b=m_b,P_sem)
  = Law(M_(1-b)^1 | M_b^1=m_b,P_sem).
```

这是由联合律分解得到的条件分布，不是从边缘律推出。FT/sampler/package-format 常量、哈希与确定性签名是完整版本化包的确定性编码/后处理，属于 `P_3` 与本方 package；本方 simulator 直接使用给定的 `pkg_b^3`，不需签名能力或 T 私钥，也不生成 peer 包。此语义材料等式不宣称完整 FTv1/FTv3 package-transcript 相等。

算法 schedule 在 N≤256 与旧 H4 相同：`sample_from_public_seed` 共用一个实现，FTv3 入口只把允许 population 上限从 256 提至 1024。给定相同公开 seed/session/fingerprint/iteration，N≤256 下返回完全相同的样本；后续 candidate、pivot、active edge 和分支只由该 seed 与以前公开的 outcomes 确定。不同 FTv3 schedule digest 是公开 profile 的固定值，不改 gate id 顺序。

最大 padded `m=256` 时 `index_bits=8,W=40`。所有可能的最大 AND layer 计数为：input generate `32m=8,192`，carry `m=256`，forward route `m²W=2,621,440`，compare generate `m(m−1)W/2=1,305,600`，borrow `m(m−1)/2=32,640`，inverse `m²=65,536`。均低于 batch cap `4,193,824`。更小 padded domains计数不增加，因此 FTv3 每个非空层只有 batch 0，count 精确等于该公开层长度；N≤256 不存在秘密相关的拆批边界。

定义 `F3(P_3,L)` 为把 S36 H4 抽样的 openings/local bits 确定性编码成 FTv3 消息的映射。则对每方、所有正概率且与 `P_3` 相容的固定条件：

```text
Law(Vpre_b^3 | x_b,z_b,L_S29,pkg_b^3,static_b,P_3,cap_success)
 = F3(P_3,L_S29)_* Law(Vpre_b^H4 | x_b,z_b,L_S29,π_sem(pkg_b^3),static_b,P_sem).
```

因而可定义 PPT simulator `Kpre_3^b = F3 ∘ H4_b`。右侧 H4 transcript 是 S36 的抽象旧语义 kernel；FTv1/FTv3 的版本专属字段分别由 `P_1/P_3` 的确定 profile 编码。该式**不**声称 FTv1/FTv3 package、签名或 wire bytes 逐值相等。非法/不相容泄露记录不在条件式范围内。

## 5. 新增可见字段与泄露映射

| 字段/状态 | 源码锚点与生成方式 | 秘密依赖 | 与既有参数/泄露关系及 sampler |
|---|---|---|---|
| FTv3/sampler/package version、schedule hash | `score_to_mask_v3_gmw.cpp:148-153,779-783,1216-1299`；profile 常量及哈希 | 无，公开定值 | 公开参数/本方签名 package 输入；simulator直接使用本方包，不扩展 L |
| READY、tag、phase | `score_to_mask_v3_party.cpp:133-158,575-580`；成功 READY 固定 `{1}` | 无 | phase由公开消息 DAG确定；READY/tag/长度属既有通信结构，直接固定编码 |
| D/E partial bits | `score_to_mask_v3_party.cpp:210-270`；局部 wire XOR 本方 triple | 有 | 完整 party view但非新增 leakage；H4 抽 uniform global D/E并按真实公式得到双方 partial |
| layer/batch/count | `score_to_mask_v3_party.cpp:210-226`；空层提前返回；非空层递增 ordinal、batch切片、count为剩余 gates 数 | N≤256无 | gate层大小/空层由公开 N、固定电路确定；每非空层 batch=0，count为公式长度 |
| Beaver payload/frame length | `score_to_mask_v3_party.cpp:69-86,220-226`；固定 inner prefix 104 bytes，body `12+ceil(2·count/8)`；transport length为 payload 长度 | bit payload有，长度无 | `L_S29`已批准 communication lengths；新长度是 P/L 的函数。固定 outer framing header另加，不把 TLS record 开销纳入 |
| phase/sequence/frame order | `score_to_mask_v3_party.cpp:133-158,160-195`; `protocol_i_transport.cpp:121-146,487-533` | 仅消息数依赖分支 | phase是 next message ordinal；sequence是该方向成功 frame 计数。active branch由 P、seed/outcome/endpoint 等 L 决定，故为确定后处理 |
| API阶段/帧/字节统计 | `score_to_mask_v3_party.cpp:830-863`；输出 `party_channel_*` counters | 无额外值依赖；统计量是成功应用 frames 的计数/长度 | 是本方 API telemetry，数值由同一 `P,L` message DAG 与固定 framing确定；不包含 TLS record overhead，不增加泄露 |
| sample seed/ACK | `score_to_mask_v3_party.cpp:160-195`; `project_derived_select5.cpp:369-442` | seed本身随机 | S29批准只披露活动轮次 seed；sim用给定seed，ACK/长度确定，未来未活动seed不提前生成 |
| iteration/substage/edge count、active compare outcome | `score_to_mask_v3_party.cpp:721-747`; `project_derived_select5.cpp:382-519` | active graph依赖此前结果；local bits依赖 wire shares | S29摘要含active endpoints/positions、pivot/outcome、comparison count；local bit由H4抽样，peer bit=公开 outcome XOR local bit |
| 本地 material-consumption bitmap/count | `score_to_mask_v3_party.cpp:208,229-235,341-343,782` | 当前执行状态 | 不是 wire 字段；由公开 gate order 与已重放的 trace确定。成功终点全门消费由代码检查，不增加泄露 |
| output-refresh帧、local `z_b` | `score_to_mask_v3_party.cpp:784-815`；P0 body长度N+5、P1 ACK长度1 | 内容相关，长度只依赖 N | H4固定 `z_b` 续接并生成消息内容；长度属既有 communication length，不新增 masked-value类别 |
| restart/retry/abort | 成功路径无此消息；package lease在 `575-577`消费 | 不适用 | 故障/重启在本定理外；不塞进 `L_S29`，也不声称 crash-prefix sampler |

对新增 `B3=(layer,batch,count,phase,sequence,frame-length,API frame/byte counters)`，存在确定函数 `f3` 使 `B3=f3(P,L_S29)`，因此 `H(B3|P,L_S29)=0`，对隐藏秘密 `S` 有 `I(S;B3|P,L_S29)=0`。这说明它不增加信息泄露；不是把 B3 新增到 L。D/E、local compare bits、output payload 本身并非 `f3(P,L)`，它们由 H4 条件分布生成。

`L_A`保持单独：A-only context、package-set/context digest、peer ciphertext/hash/length、PENDING/COMMITTED、到达顺序/时序、receipt与恢复状态都不交给本方 simulator或计入 `L_S29`。原始 TLS record/时序不属于 Kpre3。

## 6. P0/P1 sampler 与证明步骤

```text
Kpre_3^b(x_b,z_b,L,pkg_b^3,static_b,P):
  验证公开元组、FTv3 profile、本方 fresh package 与 L 的兼容性
  仅解析本方 triple/permutation shares；初始化 phase、sequence、消费位图
  以真实消息方向顺序生成 READY={1}
  对公开 gate schedule 中每个 AND layer，按 gate id 顺序：
    N≤256 时只生成 batch 0，count=公开层 gate 数
    每个未消费 gate 抽 fresh uniform D,E，计算 local d/e 与 peer partial
    按源码 Beaver 公式计算本方 output share；更新本地 consumed bitmap
    附加 layer/batch/count 并以 phase/sequence/length 确定性封装
  用 L 中已披露的活动 seed/outcome 重放 Select5 控制器
  对每个 active comparison：按 H4 local-bit 条件核抽本方 bits，
    peer bits := outcome XOR local bits，编码 iteration/substage/count
  沿用 H4 到 forward/inverse routing、membership 和固定 z_b 的 output refresh
  返回 FTv3 pre-escrow 本方完整 view 与本地状态
```

P0 和 P1 各自条件推导如下。每个门固定本方完整 package/history 后，未消费 global masks 仍独立均匀；模拟抽 D/E 与真实 joint opening 等分布。P0的局部 triple share 是 `(a0,b0,c0)` 并使用含 `D∧E` 的公式；P1使用其 XOR complement并不加该项，uniformity同样成立。两方消息先后顺序不同，但双方 tag/phase/sequence由同一确定消息 DAG给出。P0生成并公开的活动 seed是`L_S29`输入，P1使用同一seed及本方回显/ACK路径。两方均不需 peer material。

自适应阶段只由公开 seed和已泄露 outcome驱动：固定合法 `(P,L)` 后，candidate集合、sample/pivot、edge列表、是否继续/提前选出、消息数可逐步重放。对每个公开 outcome `o_i`，固定 H4 local bit `q_b,i` 后 peer bit只能是 `o_i⊕q_b,i`；不打开 inactive comparator table。output refresh沿用 S36固定理想输出share条件核：P0以`z_0`确定refresh值，P1通过本方membership share与`z_1`确定相应接收值。这里不证明真实 party RNG或加密。

## 7. 可复用结论与仍开放的组合门

可复用：S36 package-conditioned Beaver openings、活动比较 outcome的peer bit填充、自适应Select5重放、固定`z_b`下至输出share的H4续接。迁移条件为 S49 两方 semantic joint law、cap success、N≤256 schedule/seed算法等价、FTv3 profile由P及本方package持有、FTv3新增wire字段的确定性映射。

不自动迁移：S42 receipt/escrow suffix。FTv3 context format 2、profile签名字段、package hash/set binding、lease及服务parser需要另做 A-side 路由与状态机映射；诚实peer仍必须真实认证deposit。S42 receipt/fetch只可作为算法无关的候选引理，不能写成本轮FTv3后缀PASS。`FTV3_ESCROW_SUFFIX=OPEN`、完整`PARTY_VIEW_COMPOSITION=OPEN`。因此本轮不进一步声称 escrow/输出合成安全。

其他 OPEN/不适用：真实OpenSSL/party RNG有序流、熵源和reseed/lifecycle；T真实签名/材料生成桥；TLS/mTLS目标部署优势；RSA-OAEP/AES-GCM和hash绑定；A token RNG与commit/fetch组合；N=257..1000证明扩展；crash/abort prefix、A-only腐化、物理掉电耐久。人工 kill是故障注入；本轮未运行通信，自然业务I/O仍沿用`NOT_REPRODUCED`。

理想模型中 `Kpre_3`与真实条件transcript精确相等，条件优势为0；`F3`不增加优势项。真实系统的后续表达保留 `Adv_DRBG_T(Q_total,C_total;E)`、party RNG、`Adv_signature`、`Adv_hash_binding`、`Adv_recipient_encryption`、`Adv_TLS/auth`、A token RNG等独立符号；无部署参数时不合并或虚构安全位数。S49 δ只在上述semantic projection比较中单列。

## 8. 复核与运行状态

执行者参与S35–S49历史实现/推导/决策，不是本轮独立审查者。reviewer `/root/s40_independent_receiver` 此前只读审查S42、S48；未参与S47、S49或S50实现/推导。该 reviewer 独立检查了材料条件 law、N≤256 调度映射、P0/P1 对称性、字段泄露分类与 S49 δ 适用范围。首轮指出 FTv1/FTv3 版本专属参数被错误写成同一条件元组；本报告已改用共同 `P_sem` 并重新写出固定本方材料后的 peer 条件核。reviewer 复核修订后确认该类型问题解决，接受限定范围的 `Kpre_3`、P0/P1 与字段映射结论为 `PASS_WITH_SCOPE`，并确认 S42 escrow suffix 仍 `OPEN`。reviewer 未构建、未运行测试、未修改文件；这是一项密码学只读复核，不是独立功能接收。

| 门 | 当前状态 | 限定 |
|---|---|---|
| S49/S48来源身份 | `PASS_WITH_SCOPE` | fixed Git commits/blobs 与 manifest；详细值见S50 manifest |
| FTv3 semantic joint law | `INHERITED_PASS_WITH_SCOPE` | 理想 IID、cap success、semantic projection |
| `KPRE_3_N_LE_256` | `PASS_WITH_SCOPE` | 独立复核；ideal IID、cap success、N≤256、无故障 GMW pre-escrow prefix |
| 新 schedule 字段不新增泄露 | `PASS_WITH_SCOPE` | reviewer 确认 `B3=f3(P,L_S29)`；未增加 L |
| `S42_SUFFIX_TO_FTV3` | `OPEN` | escrow后缀未重证 |
| 完整 party-view / shuffle-output-escrow composition | `OPEN` | 非本轮范围 |
| 真实 DRBG / TLS / party RNG | `OPEN` | 部署假设和优势未闭合 |
| `LOW_PROBABILITY_ISSUANCE_ABORT_ACCEPTANCE` | `OPEN_CONFIRMATION` | 无新负责人确认 |
| Build/test | `NOT_RUN` | 文档/证明审查 |
| 自然业务 I/O | `NOT_REPRODUCED` | 本轮未运行通信，沿用S40/S48记录 |
| Formal LAN/WAN/RTT/AAV86/performance | `NOT_RUN` / `NOT_MEASURED` | 未启动 |
| N>256 proof | `NOT_IN_SCOPE` | 不外推S48 N≤1000功能收据 |
| secure alias | 未创建 | v3 default-OFF沿用 |

单独面向非实现负责人的确认问题（当前没有新确认记录）：

> 合法输入下，若离线材料准备因有界拒绝采样达到重试上限而以极低概率返回 `ISSUANCE_ABORT`，并需要换一个新 invocation 重新发材，是否可接受？

## 9. 最终声明边界和下一步

独立复核完成后，N≤256 ideal-IID 成功 GMW pre-escrow 前缀现标为`PASS_WITH_SCOPE`。下一步只审 FTv3 context/package-set 路由到真实 A 状态机的 suffix，再分别审真实DRBG、私有信道、签名/加密与完整party-view composition；不改变L或合同。

可以写：**“项目自有五迭代稳定 Select 已在 S48 列出的 N≤1000 用例中实现，并完成范围限定的稳定 Top-K mask oracle 检查。”** 必须注明 S48 `PASS_WITH_SCOPE`、案例非穷尽且不是安全/性能结论。阻止“完整安全论证完成”的事项仍包括目标RNG和私有信道桥、FTv3 escrow suffix、完整GMW/Select/shuffle/output/escrow组合、故障适用范围与低概率发材中止合同接受。

本轮仅本地隔离分支，不推送/合并/创建PR；不改`VFSS-baseline/`，不加入密钥、材料、证书、构建物或原始大日志。
