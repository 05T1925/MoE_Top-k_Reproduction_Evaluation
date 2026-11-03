# M6B-I PROJECT_DERIVED_SELECT_5ITER S63 报告

日期：2026-11-03
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`

## 1. 结论摘要

S62 的成功路径证明止于 `N≤256`，原因是原有 H4 schedule 尚未覆盖 FTv3 的多批 AND layer。S63 从固定代码重新推导 N=257..1000 的 gate layer、拆批、消息形状、phase、sequence、输出 share 形成时点和 Kpost 长度映射。对超过 256 的新增批次，完整 Beaver 开值/本地输出联合核可以沿 S50 的 H4 条件核逐批归纳；所有新增批次元数据均由公开 `N/K/profile` 和原有 `L_S29` 确定。经未参与本轮推导的非作者只读复核，S63 将**理想成功路径、应用/API view、外生成功调度与固定完整源码事件 cut**扩到 `1≤K≤N≤1000`，标 `PASS_WITH_SCOPE`。

此结论不表示 S62 的协议 abort-aware 模拟已经闭合，不表示运行时实现已获得密码学安全验收，也不由 S48/S52 功能案例推出。S62 的三项负责人合同决定仍为 `OPEN_CONFIRMATION`；真实超时、Dealer/A 的 DRBG、签名与文件发布、TLS/mTLS、收件人加密、SQLite 和整体实际部署组合继续各自 `OPEN`。

## 2. 固定对象与证据层级

固定被审对象是 S62 commit `c9c87799b0f27931249d5b82edce37a5f0f1814c`，tree `79bec327f312ed199e2ae4406781d52f5971436b`，parent `d843b45f29ab87d75520b28aac12c82608ec9678`。当前隔离分支为 `codex/m6b-i-project-select5-s63`；开工时工作树 clean。没有从 Desktop 主工作区、main 或相似提交替换源码。

S62 manifest 文件的 Git blob 为 `20baae7b34cce4efa2960c19e4d89498881905cc`，Git blob 内容 SHA-256 为 `f8c79634e2bb1159ed61fb86b56420f30a126ab02a7118095c0b0410c218351c`。重算 S62 五个交付文件的 canonical SHA-256 得到 `9ce84f044a1483b6a412a7f518fb64c8abc30ef616e5f9bdf2b12ffb8686e56c`，与 S62 manifest 一致。固定树中的 S62 manifest 记录 S61 的八个运行时代码来源；这些 Git blob 与 S62 固定树完全一致。S62 保留的 S61 历史 manifest 差异是两个 inherited-evidence 行将 `PROJECT.md` 和 `docs/IMPLEMENTATION_PLAN.md` 指向 S60 父版；S62 同时记录 S61 实际对象。此治理来源差异不涉及运行时代码，本轮不改写历史记录。

四类依据分开：

1. **论文定义**：研究背景和协议原语来源；本文不据此声称 Select5 是 BB90 原算法或任何论文算法复现。
2. **本地源码事实**：S62 固定 Git blob 中 FTv3 profile、AND batching、Select5 和 framing 行为。
3. **项目派生证明**：S50 H4 条件核、S51 Kpost 成功后缀、S53 SuccessFence、S61 source-event cut 依其明示条件经 S63 扩域。
4. **待验证假设**：理想材料/刷新/token RNG、诚实对端、非选择性 SuccessFence，以及真实运行时到理想接口的所有部署桥。

### 2.1 固定源码映射

下列 SHA-256 均针对固定 Git blob 字节；逐文件 Git blob ID 与完整证据身份列在 S63 source/evidence manifest。

| 固定源码 | Git blob SHA-256 | S63 用途 |
|---|---|---|
| `VFSS/include/moe_topk/score_to_mask_v3_gmw.h` | `cb9d185a474bf11f350e1aa46df47264d11b4a1cbf247fb2a81772a446a2134f` | FTv3 N/padded-N、门数、包长、batch cap、API 范围 |
| `VFSS/src/moe_topk/score_to_mask_v3_gmw.cpp` | `0819e11be6d7cef70f94491e40d9ff6bb932826164bf40311bbc10066dbf2c42` | gate 预算公式、profile/schedule hash |
| `VFSS/src/moe_topk/score_to_mask_v3_party.cpp` | `5cace49daa40d9243c457b521232c7e2305e308276ce23221072d802b33bbce5` | AND 批次、phase、D/E、Select5、输出和 escrow API 顺序 |
| `VFSS/src/moe_topk/protocol_i_transport.cpp` | `ca18f02820ae2270ea3f6c419e8a11de34d7577b24e599668fafef1cea4496e1` | payload 上限、wire header、单向 sequence |
| `VFSS/src/moe_topk/project_derived_select5.cpp` | `28bdec5c3d5264a2ed3435658fd3409b001fe7f04fd1c40255265e7cacc018b9` | 五次算法迭代、活动 seed/edge/outcome 驱动 |
| `VFSS/include/moe_topk/project_derived_select5.h` | `4a38df0d7c56a1ffbd3f0836c79ced23a95b1b2193e7c9005ba10432c73156ab` | FTv3 Select5 域和 API 声明 |
| `VFSS/src/moe_topk/score_to_mask_v3_escrow.cpp` | `e835c71b2565232f6044b4cdd0439d08bda632732d7a360a71949cf8f896addf` | Kpost 成功后缀对应的真实请求/提交/取回源码映射 |

## 3. 冻结的功能、角色与证明模型

项目功能仍为 signed Q20.12 分数降序、同分按原始 index 升序稳定排序；输出原始输入顺序、恰有 `K` 个 1 的 Top-K XOR mask shares。正常托管合同仍是双方各自加密本方 share，A 收到两份密文后共同提交，双方再分别取回本方 share；API 物理返回可不同。角色、输入、输出、`L_S29` 和默认 opt-in 边界均不变。

本次成功路径定理假设：

- 一方静态半诚实腐化；另一方是按自身同次调用输入、材料、输出份额和凭据真实因果执行的 honest endpoint；T/A 受信且不与腐化方合谋。
- 使用理想 FTv3 语义材料律，且条件于发材成功 `cap_success`；本方可见本方材料包，不得到对端 package/share/credential、A-only context/DB、T 私钥或 controller 权限。
- 理想 output-refresh share law 和理想 A 独立 token RNG；A 的 token 在双方有效 deposit 后抽一次并在响应/fetch 中复用。
- 对每个固定公开调用元组和与其相容的既有 `L_S29`，由外生 `SuccessFence` 保证合法成功路径事件按协议顺序完成；再由外生选择一个固定、完整源码事件 cut。cut 不由隐藏计算内容或真实运行耗时事后选择。
- `1≤K≤N≤1000`。`N>1000` 不在结论内。

这是项目派生的理想成功应用/API 视图结论。它不纳入 cap-abort、其他协议中止、实际墙钟 timeout 条件化、A-only 腐化、任意 partial-byte/TLS-record/CPU 指令 cut、崩溃/掉电或底层服务失败。

## 4. 源码 schedule 与资源推导（推导值，不是实测/性能）

FTv3 参数在 `score_to_mask_v3_gmw.h:35–44`：`MaxN=1000`、`MaxPaddedN=1024`、gate ceiling `100,000,000`、package ceiling `64 MiB`、最大 AND batch `B=4,193,824`。`score_to_mask_v3_gmw.cpp:718–741` 从 padded `m` 计算 `index_bits=ceil(log2(m))`、`W=32+index_bits`、`pairs=m(m−1)/2`，以及

```text
G(m,W) = 63m + m²W + C(m,2)(2W−1) + m².
```

头部、签名、`ceil(3G/8)` 个 triple share bytes 和 `ceil(m²/8)` 个 permutation share bytes 相加得到每方 package bytes。分批发生于同一逻辑 AND layer 内，按固定 gate-id 次序切成连续批；当前层输入向量在 layer 调用前已构造，前一批输出不会成为同一 layer 后续批的输入。

| N 区间 / padded m | W | Gate 总数 | AND layer/batch 分解 | AND batch frames | logical AND layer calls | 成功路径最大 phase |
|---|---:|---:|---|---:|---:|---:|
| N≤256 (worst-case m=256) | 40 | 5,281,664 | input 1；carry 31；route 1；compare-gen 1；borrow 39；inverse 1 | 74 | 74 | 89 |
| 257..512 / 512 | 41 | 21,638,400 | input 1；carry 31；route 3；compare-gen 2；borrow 40；inverse 1 | 78 | 75 | 93 |
| 513..1000 / 1024 | 42 | 88,626,688 | input 1；carry 31；route 11；compare-gen 6；borrow 41；inverse 1 | 91 | 76 | 106 |

N=1000 的逐层 sizes 与 batch count：

| 逻辑层 | 层门数 | batch 数及末批 | 依据 |
|---|---:|---|---|
| 输入转换/generate | 32,768 | 1 | `32m` |
| carry recurrence | 每层 1,024，共 31 层 | 每层 1，共 31 | 源码 bit 1..31，每层 `m` 个门 |
| forward route | 44,040,192 | 11：10×4,193,824 + 2,101,952 | `m²W` |
| pair compare-generate | 21,998,592 | 6：5×4,193,824 + 1,029,472 | `C(m,2)W` |
| pair borrow recurrence | 每层 523,776，共 41 层 | 每层 1，共 41 | 源码 bit 1..41，每层 `C(m,2)` |
| inverse route | 1,048,576 | 1 | `m²` |

因此 N=1000 有 76 个逻辑 AND layer 调用、91 个 AND batch `exchange_phase`。最大门层都低于 gate/package 配置上限；每方 FTv3 package 为 33,366,516 bytes，双包 66,733,032 bytes（由固定源码公式推算，不是当轮测量）。`N=257..512` 的包 profile 以 `m=512` 计算；在 `N=513` 时跳至 `m=1024`，相应 schedule 之后保持到 N=1000。

### 4.1 批次 frame/payload/phase/sequence

`score_to_mask_v3_party.cpp:210–270` 的批次 body 为三个 u32 字段 `(layer,batch,count)` 加 `ceil(2count/8)` 个 D/E bits；inner context 在 `:69–87` 加 104 bytes。对最大配置批次 `count=4,193,824`：

```text
packed D/E bytes = ceil(2·count/8) = 1,048,456
batch body       = 12 + packed = 1,048,468
inner payload    = 104 + body = 1,048,572 bytes
payload cap      = 1,048,576 bytes
material header  = 56 bytes (outside payload cap)
outer wire frame = 1,048,628 bytes, excluding TLS record overhead.
```

最大批次 inner payload 距 1 MiB 上限还差 4 bytes，故合法。`ProtocolIFramedChannel::send/receive` 检查的是 payload 长度，先独立写/读 56-byte material-bound header 再写/读 payload（`protocol_i_transport.cpp:26–30,480–527`）；不得把 wire header 算入 payload 上限，也不得把 TLS record overhead算进本报告中的应用 payload。

每个 batch exchange 分配一个 `phase`，`next_phase_` 检查上限 255；每个方向完成一次 frame 后递增其 u64 sequence。N=1000 的最大相位/消息 DAG 为：

```text
phase 1: READY
phases 2..91: input 1 + carry 31 + forward 11 + compare-gen 6 + borrow 41 = 90 batches
up to 4 sample-seed exchanges and 9 active comparison waves: phases 92..104 (when maximal)
phase 105: inverse batch (maximal path)
phase 106: output refresh
```

为了避免把阶段表的“骨干 91”与拆批总数混淆：固定骨干为 READY 加 inverse 之前的 90 个 batch；91 个 AND batch 总数另包含 1 个 inverse batch。更精确的动态相位式为：令 `s≤4` 是实际活动 seed exchange 数，`w≤9` 是活动比较 wave 数，则 inverse 在 `92+s+w`，output refresh 在 `93+s+w`；最坏 `s=4,w=9` 得 phase 105、106。早停时更少。每个方向每个 phase 最多一帧，所以帧数和 sequence 为该 phase 图的确定结果，最大序号为 105。

F_T/v3 schedule hash 在 `score_to_mask_v3_gmw.cpp:148–159` 绑定 batch gate cap、1 MiB payload profile、固定骨干 91、Select5 最多 13 phase、inverse 1、output 1；最大总 phase 106 小于 phase 编码上限 255。五次算法迭代、最多九个明文比较依赖 wave、应用 exchange phase 和网络 RTT 是四个不同量，S63 不把 phase 数或同机诊断写成 RTT/正式性能。

比较波的 body 是 `iteration/substage/count/packed local bits`。N=1000 下最大活动边数量至多 `C(1024,2)=523,776`，其 body 65,478 bytes，inner payload 65,582 bytes；真实 Select5 active edge 数由之前批准公开的 endpoint/pivot/outcome/seed trace 确定，inactive comparator 不打开。P0 output-refresh inner payload 为 1,109 bytes，P1 ACK inner payload 105 bytes；分别来自 N=1000 的公开长度。该值同样是源码推导，不是网络计量。

## 5. 逐批 Kpre 条件核

令 `P=(τ,N,K,m,W,FTv3 profile)`，固定 `b∈{0,1}`、本方输入 share `x_b`、本方已签名 package `pkg_b^3`、静态状态以及和 `P` 相容的批准泄露 `L=L_S29`。`L` 沿用 S29/S51/S61 六类字段：公开参数/协议绑定、允许公开的匿名活动端点/位置、pivot/outcome、活动 seed、批准 masked values、比较数和通信长度/统一 abort 类别。S63 不新增 leakage。

对每个 AND layer `ℓ`，源码生成的完整 gate vector
`G_ℓ=((id_i,X_i,Y_i))_{i=0}^{q_ℓ−1}` 在调用 `and_layer` 前已确定于本方局部 wires 和公开电路。FTv3 把它用公开 `B` 划为连续不相交区间
`I_{ℓ,r}=[rB,min((r+1)B,q_ℓ))`。批次字段不由当前 gate bits 选择。对任一尚未消费的 `id_i`，在 ideal FTv3 的联合 triple law 下，固定 `pkg_b^3` 和此前真实/已模拟 history 后，peer 补份的 Beaver masks 保持其 H4 条件联合分布；同一 batch 的 opening 向量应当作为联合量采样，而不是只从边缘均匀性推论。

对固定当前 gate inputs `(X_i,Y_i)`，全局 opening 为
`D_i=X_i⊕A_i`、`E_i=Y_i⊕B_i`。H4 对一整个不相交 batch 给出 `(D,E,party-local partials,local output shares)` 的条件 kernel。Simulator 按该 kernel 抽 opening 与本方/peer partials，并按真实 Beaver 输出公式重建本方局部输出；peer frame 只包含其局部 D/E partial。随后它执行确定性的层/批次编码和真实 P0/P1 发送次序。

关键归纳：同层 gate inputs 在开始 `and_layer` 前已经生成，且同层下一 batch 不读取上一 batch output；上一 batch 输出只写入对应 `outputs[offset+i]`。批次 gate IDs 不重叠，未消费 gates 与已消费批次材料独立。故在条件于固定本方 package、截至该 batch 的本方状态/view 和 `L` 时，可对 batch ordinal `r` 逐次应用 H4 kernel。归纳到所有 layer/batch 后，任意固定 batch source-event cut 的完整本方 prefix 具有真实条件联合 law；它不是拿给定最终 `z_b` 的成功 transcript 做后验截断。`consumed_` 位图和部分输出数组是本方局部状态，从固定 gate-id schedule 和已模拟 opening 逐项重建，不发送给 peer。

对于 S61 output-share 条件边界也按源顺序处理：P0 在创建并暂存本方 refresh share 后才构造、发送 `u=R⊕m_0`；在该 formation 事件之前模拟器不获得未来 `z_0`。P1 先收到 P0 的完整 refresh frame，再发 ACK `{1}`，最后在 caller 中计算 `z_1=u⊕m_1`。在 P1 收到 `u` 但 share 未形成的 cut，因理想新鲜 `R` 均匀，固定任意此前本地 view 和 `m_0` 都有 `Law(u|view,m_0)=U({0,1}^N)`，可直接采样 `u` 而不取未来 `z_1`。只有 share 真正形成后，才把腐化方自己的 staged `z_b` 作为其进程状态提供给该方 sampler。

### 5.1 字段—源码—泄露/采样映射

| 字段/状态 | 源码来源 | 对秘密的依赖 | 条件 sampler / `L_S29` 分类 |
|---|---|---|---|
| `layer,batch,count` | party `:210–226` | 无；batch 取 gate slice，count 为公开 layer 长度剩余数 | `f(P)` 的确定函数；`H(B|P,L)=0` |
| `phase,sequence,frame count` | `exchange_phase :133–157`；transport `:480–527` | Select 早停时依赖公开 active trace | `f(P,L)`；现有通信长度/比较数类别；不新增 L |
| inner/outer payload length | party `:69–87,210–226`；transport `:480–527` | 固定 AND 层只依赖 m；active frame 依赖已批准边数；output 依赖 N | 由 `P,L` 精确计算；TLS record/墙钟不纳入 |
| batch body 内 D/E bits | party `:227–267` | 含本方材料与 wires | 使用整批 H4 条件联合核，不以 shape/边缘模拟内容 |
| 本方 consumed gate IDs / batch partial outputs | party `:229–269` | 本方内部状态 | 由 `pkg_b`、已模拟 gate inputs、opening 和 cut 顺序重建；不向 peer 发送 |
| active seed、edge、pivot、outcome、compare count | party `:721–773`；Select5 `project_derived_select5.cpp:355–521` | outcome/active set 由此前隐藏比较后公开 | 按已有 `L_S29` 固定获准字段逐层重放；local compare bit按 H4 条件核填充，不开 inactive table |
| output refresh `u` 和 `z_b` | party `:799–830` | payload 及 shares 为秘密 | P1 formation 前用均匀 `u` 条件核；formation 后只交本方 staged `z_b` |
| submit/receipt/fetch/API 返回 | party `:831–902`；S51 Kpost | 成功 A 状态和 token | honest peer causal endpoint + ideal A + ideal token RNG + SuccessFence；A DB/context和 peer密文仍隐藏 |
| wall-clock、TLS record、partial byte | transport exact I/O；TLS wrapper | 可能与隐藏内容和调度相关 | 不属于此模型；真实 finite timeout/成功筛选门 OPEN |

对批次公开元数据 `B_N=(layer,batch,count,phase,sequence,frame-size,counters)`，固定 `(P,L)` 后 `H(B_N|P,L)=0`，因此 `I(S;B_N|P,L)=0`；这是一条确定性后处理证明，不是将 B_N 新增进 `L_S29`。随机/秘密 payload 内容仍由条件核生成。`L_A` 保持独立，A-only context、package-set hash、peer ciphertext/hash/到达状态均不交 party simulator。

## 6. 扩展的有限 event-cut 与完整成功后缀

定义 `E_cut^FTv3(N)` 为 S61 完整 source-event cut 词汇的参数化集合。固定 N 的 schedule 后，允许 cut 仅位于：成功的 package validation/preflight/lease/理想私有通道 setup 之后；READY 完整 body/frame；每个 gate layer 每个 `(layer,batch)` 的完整 body-ready、本方完整 send、peer 完整 receive/parse、opening 完成和 layer return；每个 active seed/ACK、active compare wave send/receive 与 Select 状态更新；P0 refresh share formation/body/send/receive，P1 refresh receive/ACK/send/XOR share formation；以及 S61 已列的 submit request、A-success response 到达/解析、fetch request/response 到达、解密核验/API share return。

`j∈E_cut^FTv3(N)` 是外生固定的单个 source-event 边界。cut 的标签可含公开 layer/batch/phase 位置，但不允许根据本方隐藏 wire、elapsed time 或真实 success/failure 结果选择。发送/接收均为完整应用 frame；不含 partial bytes、TLS record、任意 CPU 指令点、timeout、abort/服务恢复。active trace 的可用事件由固定 `L_S29` 决定；对一个固定合法 `(P,L,j)`，模拟器仅推进至 j，不提前输出后续消息或 future share。

每批内 P0 发送本方 frame 后再接收 peer frame；P1 先接收/校验，再发送本方 frame。cut prefix 保留相应方向的待发送/待接收状态，不将 P0 的先发顺序复制给 P1。批次 cut 的新增本地变量（`offset/batch/outputs` 已完成段/consume bitmap）在本方代码中均可从 `pkg_b^3`、`x_b`、此前模拟 wires和固定 j 重建。

Kpost 成功后缀只从本方 output share 已形成处开始。S51 的 ideal token 与 causal honest peer/A 状态机依赖 vector 长度 N 和 public key size 形成 envelope/请求长度，不依赖 Kpre 是否一层一批。`encrypt_share` envelope bytes 为 `42 + RSA_modulus_bytes + N`；在 S51 使用 RSA-2048 示例下 N=1000 时为 1,298 bytes，远低于 escrow ciphertext cap 2 MiB。A 的 receipt/v2 token 仍为 32 bytes，理想 RNG 条件分布与分享长度无关；双方成功 deposit 后 A 只回给每方其真实 receipt，并在各自 fetch 返回本方密文。此处复用的是 S51 的**理想成功** Kpost 条件核；真实 RSA/AES、TLS、A `RAND_bytes` 与存储安全仍不在定理中。

## 7. 逐步 hybrid、P0/P1 与优势界

给定 `b` 和固定兼容条件 `q_b=(x_b,z_b或未形成、P,L,pkg_b,static_b)`，按以下顺序组合：

1. **材料 law**：使用 S49 理想 IID F_T/v3 语义材料联合 law并条件于 `cap_success`；不把 cap abort 结果混入本定理。两方不独立生成 triples；沿 ideal joint setup 给每方其 package，party simulator 只看 `pkg_b`。
2. **AND schedule**：按 public gate IDs 对每层确定性拆批。对批次 r 固定历史 `H_r`，用完整 H4 conditional joint kernel 采样 `D/E`、本方 partial、peer message 和本方 local outputs。由于不相交未用 gate materials，替换前后条件核逐批相等；归纳覆盖 91 个最大 AND batches。
3. **Select5**：给定现有 L 的活动 seed/outcome/endpoint/count，按源码重算候选/active edges 和下一 phase；每个获准 active compare outcome 使用 S36/H4 local-bit 条件填充，peer bit为 `outcome⊕local_bit`。最多 4 seed、9 active waves；不揭示 inactive compare table。
4. **Output refresh/cut**：按 S61 的 P0/P1 源顺序。任何 formation 前 cut 不提供 `z_b`；P1 receive-pre-share 用均匀 `u` sampler。formation 后才将本方 staged share作为本方 state输入。
5. **Kpost**：诚实对端以自己的输入、输出、package和凭据真实提交；Sim_b只用自己数据形成、发送、取回自己的 envelope。ideal A隐藏内部 context/DB和peer ciphertext，双方有效 deposit成功后抽一次独立均匀 token并复用；SuccessFence固定成功调度，response/fetch/API消息按真实因果状态机生成。

每个替换在理想联合接口下均为精确条件分布相等，因此本轮扩域的理想模型条件统计距离为 0；不把这个“0”外推为实际运行优势。真实证明还须分别处理 `Adv_DRBG_T/A/P`、签名、哈希绑定、recipient RSA-OAEP/AES-GCM、私有通道/TLS、成功选择/实际超时及协议组合项；在目标部署和参数未给定时只保留符号，S63不实例化数值优势。

P0/P1 对称性不是消息时间顺序完全相同：P0 对每个 batch/wave 先 send 再 receive，P1 先 receive 再 send；相应 simulator 依各自源顺序输出同一 joint opening law。P0 的 refresh share 先形成再发送；P1 先收 frame、后 ACK、再形成 share。两方仅可接收自己的本方 package/output share，peer endpoint及A隐藏状态不进入本方 simulator。S63 reviewer 独立复核了该对称映射和 formation 时点。

## 8. S62 中止合同：并行裁决但不代为选择

固定仓库中没有新增负责人书面决定，以下仍 `OPEN_CONFIRMATION`：

1. FTv3 bounded sampler 到 cap 后 burn 本 invocation，是否接受仅用**新 invocation**重试；
2. 发材失败是否要求两方都不启动/都得到明确的一般失败；通知未送达时双方是否允许观察不同 timeout/UNKNOWN；
3. 隐私定理仅覆盖初始化与发材成功后的正常执行，还是纳入协议级 abort。

过去对项目算法随机失败数值上限的暂缓不构成 FTv3 cap-abort 的批准。已确认的正常托管流程、两方分别取回自身份额和人工 kill 属故障注入的含义不重开。三项不同候选范围如下：

| 分支 | 定理覆盖 | S63 状态 | 尚缺 |
|---|---|---|---|
| `T_success` | 发材成功、诚实双方执行、ideal A commit/fetch、SuccessFence 成功视图 | `PASS_WITH_SCOPE`，N≤1000 | 真实实现/部署桥与实际超时选择 |
| `T_abort` | 另含 cap-abort、issuer delivery/start/notice、在线协议拒绝和 A ABORT/UNKNOWN/recovery failure | `OPEN` | owner选择、同 invocation 联合功能、逐事件 sampler及生产 refinement |
| `T_deployment` | 再纳入 RAND/provider、signer、文件系统、SQLite、TLS/网络、机器故障与 wall-clock timeout | `OPEN` | 部署方模型与环境证据，不由 ideal T_abort自动覆盖 |

`PAIRED_GENERIC_ABORT` 仅是 S62 proof-only 候选：只有负责人确认它已在获准 `L_S29` 内，才能使用；未确认之前 S63 不生成统一 abort 投影。现有生产 Dealer issuer 与 party execute API 分开，没有实际共同 launch/notice barrier。TEST_ONLY E2E 控制器等待 Dealer 成功、检查两个 package 后启动双方，只能描述为测试控制流，不能拿来证明生产全/无启动。

即使将来确认 cap-abort 策略，仍不能将 S61 success-prefix 核用来模拟失败。若继续扩展 abort-aware 模型，首个在线联合条件核仍是第一份认证密文进入 A `PENDING` 后：诚实对端第二次 deposit 是否先于 wait expiry、A 内部转移、提交方真实收到/解析哪种 response 的联合核。`COMMIT_STATUS_UNKNOWN` 是 party observation而非 A DB 状态；本地 share形成、密文提交、share取回和 API 返回必须继续区分。S51 Kpost仅覆盖成功后缀。

## 9. 证据分栏、审查与运行项

| 主张 | 状态 | 证据/边界 |
|---|---|---|
| S62 固定来源 | `PASS` | commit/tree/parent、manifest canonical及运行时 blob 核验 |
| ideal FTv3 batched `Kpre`，`N≤1000` | `PASS_WITH_SCOPE` | 新的逐批条件联合 kernel + S50/S49 前提 + 独立只读 review |
| event-cut 扩展 `E_cut^FTv3(N)` | `PASS_WITH_SCOPE` | 仅完整 source-event边界、外生单 cut、成功 path；非任意指令/字节/故障 cut |
| Kpost success suffix，N≤1000 | `PASS_WITH_SCOPE` | S51理想 A/诚实 peer/独立 token RNG，长度/阶段映射；reviewer只读复核 |
| `Kpre+Kpost` ideal success app/API view，N≤1000 | `PASS_WITH_SCOPE` | SuccessFence、`cap_success`及上述模型，不含真实 timeout/deployment |
| S48/S52 N=1000 finite functionality | `INHERITED_PASS_WITH_SCOPE` | 案例结果继承且未重跑；不作安全证明 |
| 协议 abort、cap-abort delivery/start/notice | `OPEN_CONFIRMATION` / `OPEN` | S62无 owner decision，无生产 barrier |
| escrow ABORT/UNKNOWN/recovery failure | `OPEN` | 第一份 PENDING 后的到达/超时/响应联合核未定义 |
| 真实 finite timeout / wall-clock成功筛选 | `OPEN` | 未构造 scheduler law或条件 sampler；SuccessFence不替代 |
| T/A DRBG、签名/发布/FS、SQLite、TLS/mTLS、recipient encryption | `OPEN` | 需要分别做部署/密码桥，不给真实位数 |
| BUILD/TEST、历史矩阵、故障注入 | `NOT_RUN` | 本轮纯证明/来源审查；未复用历史矩阵作新测试 |
| 正式 LAN/WAN、RTT、AAV86 / 性能 | `NOT_RUN/NOT_MEASURED` | 未启动 |

执行者参与过 S50–S62 的证明/决策历史，不把自身工作称作独立复核。非作者 `/root/s63_independent_review` 未参与 S50–S63，独立检查固定源码 batch 数/phase/帧公式、H4 条件核能否逐批延拓、L字段映射、P0/P1顺序、share formation、Kpost尺寸映射和S62 owner abort边界。其未编辑、未构建、未测试；这不是功能接收或部署安全签核。

## 10. 未闭合安全门与下一步

1. 负责人先决定 cap-abort新 invocation重试、发行失败通知/启动规则和安全定理是否含协议级 abort；不得以已有 algorithm failure deferral替代。
2. 若选择 success-only 定理，本轮 N≤1000 ideal-app-view范围可在上述条件下引用；下一步为独立建真实 T/A 部署 profile与 DRBG/签名/发布/私有信道/recipient encryption/SQLite 证据，而非重复功能矩阵。
3. 若选择 abort-aware 定理，先版本化批准 `F_TOPK-ISSUE-ESCROW` 的同调用 issuer outcome与party projection；若通知不可达允许不对称观察，应定义联合 notice/start kernel且不得扩大 L。随后逐个构造 A 状态/response/UNKNOWN/fetch-recovery failure核，停在第一个不可模拟点。
4. 即使成功路径理想证明闭合，真实有限 timeout下 `Law(Vpre|Q,cap_success,Success_actual)` 仍需自己的部署调度模型或条件 sampler；不增加wall-clock至 `L_S29` 来绕开。

本轮未修改运行时代码或测试、未做构建/功能/故障/性能测试；没有新自然I/O观察（继承状态仍 `NOT_REPRODUCED`）。`VFSS-baseline/` 未修改，无 secure alias、push、merge 或 PR。文件及 Git blob 哈希见 S63 source/evidence manifest。

## 11. 来源复核命令与本轮执行

来源身份复核使用 `git rev-parse HEAD`、`git rev-parse "HEAD^{tree}"`、`git rev-parse HEAD^`，并通过固定 S62 commit 的 Git objects 读取 manifest 与源码 blob；没有从其他 worktree 或桌面 checkout 取源码。S63 manifest 生成时对固定源码、引用证据和 staged 交付文件计算 Git blob SHA-1、blob 内容 SHA-256 与字节数，并复算交付 canonical digest。

本轮还执行了 `python -m json.tool` 对 S63 gates 和 source/evidence manifest 做 JSON 解析，运行 manifest 自校验脚本核对 staged delivery hash、runtime source blob hash 与 canonical digest，并执行 `git diff --cached --check`。这些是来源/格式/差异校验，不是 runtime 测试。BUILD、TEST、schedule trace/conformance 与历史功能矩阵均 `NOT_RUN`。
