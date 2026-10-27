# M6B-I PROJECT_DERIVED_SELECT_5ITER S61 详细报告

日期：2026-10-27
路线：`Protocol I + PROJECT_DERIVED_SELECT_5ITER + GMW/Beaver-v3 / EXPERIMENTAL`
结论标签：项目派生功能/证明审查；不是 BB90、BMW16 或 BB90+DCF 原协议复现。

## 1. 执行摘要

本轮以 S60 精确快照为固定源码，对 `1≤K≤N≤256`、一方静态半诚实、理想 FTv3 材料且发材成功、理想 `F_TOPK` output-refresh share、理想 A token RNG、诚实 peer 因果执行和外生 source-event cut，构造了 `Kpre-prefix^{b,j}`。cut 在 output share 形成前不向模拟器给未来 `z_b`；share 形成后，只把该腐化 party 自己的 staged share 纳入其本地状态。此限定 online-prefix 子目标经本轮非作者理论复核后可标 `PASS_WITH_SCOPE`。

核心构造不是“把带有最终 `z_b` 的 S50 成功 transcript 截断”。对 output-refresh 之前任意计算前缀，使用 S36 已审条件等式：固定任何 `z_b` 都不改变此前计算 prefix 的 law；随后对 `z_b` 边缘化，得到不需要 `z_b` 的 sampler。另对 P1 的特殊边界单独处理：它已经收到 P0 的 output-refresh frame、但尚未执行本地 XOR 形成 `z_1` 时，模拟器从均匀分布采样 frame payload `u`，依据 P0 的 fresh ideal refresh share 得到相同 law，不拿未来 `z_1`。

本结论**不**覆盖 cap-abort/issuer launch 通知、协议验证错误、`ABORT`、`UNKNOWN`、fetch 失败、真实 timeout、partial frame 故障、T/A 实际随机源或部署故障。S50/S51/S53 的完整成功路径只能在理想 `SuccessFence` 下组合；本轮的 cut 选择是另一项外生理想调度条件，不能解释为实际进程被杀、网络超时或 crash-prefix 的安全证明。

## 2. 固定接收对象与来源

| 项 | 本轮核验值 |
|---|---|
| 固定 S60 commit | `07325b076dc31f83439d55d7a59ad2239e0d5fcf` |
| tree | `748a4d86bdfbaffc6d401e8d25f3de7a02ed90f2` |
| parent | `0f67aec45d767c4bf530b12e4d548a82d68b6800` |
| 当前隔离分支 | `codex/m6b-i-project-select5-s61` |
| S61 开始状态 | S60 tracked tree 精确；已有两份未跟踪 S61 草稿，开始前已读取并保留 |
| S60 canonical delivery SHA-256 | `9b6b35befdb2138ce15ee8750d154a5881e1f4aa3f6392dbbbe75e5d7efc1024` |
| 本轮源码变更 | 无 |

从 S60 source/evidence manifest 读取并以固定 Git blob 核验：

| 文件 | 用途 | S60 记录的 blob-content SHA-256 |
|---|---|---|
| `VFSS/src/moe_topk/score_to_mask_v3_party.cpp` | 在线 party DAG、Beaver layer、Select5、output refresh、submit/fetch | `5cace49daa40d9243c457b521232c7e2305e308276ce23221072d802b33bbce5` |
| `VFSS/src/moe_topk/score_to_mask_v3_gmw.cpp` | gate schedule/profile、材料和包验证、一次性材料接口 | `0819e11be6d7cef70f94491e40d9ff6bb932826164bf40311bbc10066dbf2c42` |
| VFSS/src/moe_topk/project_derived_select5.cpp | 五迭代候选更新及活动比较调用 | 28bdec5c3d5264a2ed3435658fd3409b001fe7f04fd1c40255265e7cacc018b9 |
| `VFSS/src/moe_topk/protocol_i_transport.cpp` | frame phase、sequence、完整帧 send/receive | `ca18f02820ae2270ea3f6c419e8a11de34d7577b24e599668fafef1cea4496e1` |
| `VFSS/src/moe_topk/score_to_mask_v3_escrow.cpp` | escrow TLS framed request/response 与服务状态机 | `e835c71b2565232f6044b4cdd0439d08bda632732d7a360a71949cf8f896addf` |
| `VFSS/include/moe_topk/score_to_mask_v3_gmw.h` | v3 批次上限、package/API 合同 | `cb9d185a474bf11f350e1aa46df47264d11b4a1cbf247fb2a81772a446a2134f` |
| VFSS/include/moe_topk/project_derived_select5.h | Select5 的公开域上限/接口 | 4a38df0d7c56a1ffbd3f0836c79ced23a95b1b2193e7c9005ba10432c73156ab |

证据层级分开处理：论文定义只作研究参照；本地源码事实由 S60 固定 Git blob 支持；`Kpre₃`、条件 law 和本轮 prefix sampler 是项目派生证明；真实 RNG、TLS/加密优势、部署故障模型和 owner 尚未确认的失败合同属于待证明/待决事项。没有从桌面主工作区或近似快照拼接代码。S61 开始时这两份 S61 Markdown 草稿已是未跟踪文件（decision raw SHA-256 15422e55eb277ae4ec41dc4bff92b754355f82a38a93fbb566b99faf25cfa7cc；report raw SHA-256 dfaccaa671848d62ff733397d35e41b7a5a743cfdd9bf9b24460433826032844），先读取后在其内容上修订；故固定 S60 提交干净与 S61 工作目录初始含未跟踪草稿分别记录。

## 3. 固定模型、视图和权限

对腐化 party `b∈{0,1}`，令固定条件元组

```text
Q_b = (x_b, public(τ), pkg_b^3, static_b, L_S29,
       ISSUED/cap_success, ideal F_TOPK interface).
```

`τ` 是同一次调用的完整公开绑定：`(session, invocation, N, K, route, circuit hash, gate-schedule hash, FT/package/context/receipt profile, party/A/T 身份与公开密钥标识, escrow state parameters)`。`ISSUED` 固定本轮从成功发材状态开始；FTv3 cap-abort、签名/文件发布故障不在此条件模型内。

理想输入与禁止输入：

| 对象 | 可用信息 |
|---|---|
| `Sim_b^{≤j}` | `x_b`、`public(τ)`、本方有效签名 package、静态本方凭据/状态、原样 `L_S29`、cut 标签 `j`，以及仅在本方输出形成事件之后的本方 staged `z_b`。它自己生成 cut 前完整本方局部 state/transcript。 |
| honest party `1-b` | 自己同次调用的输入、理想输出份额、本方 package、凭据和真实协议算法；在共同世界中按真实因果次序与 B/A 交互。 |
| A/ideal escrow | T 只给 A 的 metadata-only context、两方实际到达的密文/身份和自己的内部事务状态；理想 A token RNG。 |
| **不交给** `Sim_b` | peer input/output/package/credential、T 私钥或 Dealer tape、A context/DB 状态/peer ciphertext、测试 controller 的 DB/file/log 访问、未授权随机带、proof-only reduction oracle、未来 `z_b`（share formed 前）。 |

本方 view 可包含本方完整输入 share、package、局部 triple/permutation shares、wire shares、已发送/收到完整应用帧、局部 accumulator/已消费 gate 状态、本方输出 share（仅形成后）、自己的 envelope 和本方 API 已见状态。A 的 `PENDING/COMMITTED/ABORTED` 是服务内状态，不等于 party view；`COMMIT_STATUS_UNKNOWN` 是客户端 observation，不是 DB state。

`L_S29` 维持 S51 恢复的原始 M6B S29 六类来源：公开参数/协议绑定、允许的匿名活动端点/位置、pivot/比较 outcome、活动迭代的 sample seed、获准 masked values、比较数/通信长度/统一 abort 范围。S61 不增列字段；`layer/batch/count/phase/sequence/frame length` 在 N≤256 下是公开 tuple、固定 schedule 与已获准活动 `L_S29` 的确定函数。原始 TLS timing、真实 deadline、A-only context/state 和 escrow DB 不挪入 `L_S29`。

## 4. 从源码得到的有限事件 DAG

### 4.1 源码顺序

1. Party 先打开并验证本方 FTv3 package、参数和材料长度，检查 `N/K` 与内存预估；完成认证 peer channel 握手/证书 pinning，重置 frame counter，然后消费本方 package lease。对应 `score_to_mask_v3_party.cpp:522–594`、`score_to_mask_v3_gmw.cpp:1470+`。
2. `GmwParty::exchange`/`and_layer` 通过 `exchange_phase` 设定 `phase` 并按 party 顺序发送/接收。P0 先 send 后 receive；P1 先 receive 后 send。`and_layer` 在 exchange 前按 gate id 检查并标记一次消费，构造 packed D/E partial；收齐对方 frame 后 XOR 得打开值，再计算本地 Beaver output shares。来源 `score_to_mask_v3_party.cpp:133–158,210–323`。
3. 先进行 additive-share→Boolean share 的输入转换；随后 forward permutation route、全体 pair comparator 的 borrow recurrence；之后 Select5 每轮按控制状态决定 sample seed、活动比较 clique/star frame 与下一 candidate；最终第五轮 candidate clique 完成精确选择，形成 membership、inverse route，再检查所有 gate 均已消费。来源 `score_to_mask_v3_party.cpp:594–798`、`project_derived_select5.cpp:355–520`。
4. P0 生成本方刷新 share R，先形成 staged z0=R，再发 u=R XOR m0 并收 ACK。P1 的 exchange_phase 顺序为先收取/校验 P0 的完整刷新 frame，再发送固定 {1} ACK；该函数返回后调用者校验 body 并逐位计算 z1=u XOR m1。因而 P1 存在完整收到 u、尚未发送 ACK和ACK 已发送、尚未形成 z1两个不同 cut。来源 score_to_mask_v3_party.cpp:144–156,799–830。
5. 本方生成 recipient envelope 后，调用 submit callback；收到 COMMITTED receipt 后调用 authenticated fetch，核对 receipt、解密并比较本地 staged share，最终 API 才标成功。异常路径可产生 ABORT/RECOVERY_REQUIRED，但不属于本轮理想成功 cut。来源 `score_to_mask_v3_party.cpp:824–902`。
6. escrow 客户端每次 submit/fetch 单独建立 TLS connection：接收固定 READY frame，phase 2 发送 request frame，接收 response frame。服务 first deposit 可提交 PENDING；第二合法 deposit 才生成一次 token 并更新 COMMITTED。来源 `score_to_mask_v3_escrow.cpp:314–443,566–623,678–713,876–900`。

底层 `ProtocolIFramedChannel::send/receive` 对**完整应用 frame**设置 phase/检查 header；sequence 每方向递增。frame header 对材料绑定版本为 56 B，最大 application payload 为 1 MiB。partial read/write/TLS WANT_READ/WANT_WRITE/poll 的字节内循环不属于协议层单一 event cut；它们属于真实 transport/timeout 故障模型。来源 `protocol_i_transport.cpp:123–169,303–305,480–533`。

### 4.2 E_cut 的显式事件词汇与边界

为避免把任意 CPU 指令或 TLS 字节进度混入定理，定义固定公开调用 P、获准 L_S29 和一个成功执行事件图 D(P,L_S29,sigma)。sigma 是理想无故障消息调度的外生次序参数；它决定合法消息何时被处理，但不查看隐藏 wire/material、peer secret 或 A 数据库。J 是从下列有限 source-event 边界中外生指定的单个 cut 标签。sigma、J 是证明实验控制输入，不出现在 party view，也不并入 L_S29。若二者由真实 wall-clock、timeout、随机丢包或隐藏进度选择，模型不再是本轮模型。

本轮 cut universe 从成功取得并验证同次 package、通过本地 preflight/lease 检查且理想私有通道已建立之后开始；初始化、发材交付/启动通知边界由 S60 的 issuance-routing OPEN 覆盖，本轮不把它们算入 `E_cut`。`E_cut(P,L,sigma)` 只由以下边界生成；所有标签对应固定 S60 source call/return、完整 application frame 或明确定义的本地协议状态转换。`pre(e)`/`post(e)` 表示 source operation 前/后，不表示机器指令级中断：

| 事件词汇 | 产生位置与可观察内容 | cut 状态 |
|---|---|---|
| READY_PHASE_SET(1)、READY_BODY_READY | exchange_phase 设 phase、编码固定 {1} | 本地 phase、完整待发 body；无 peer frame |
| FRAME_BODY_READY(p,tag) | 固定协议 operation 完成本地 frame body 构造（含 output-refresh u 与 ACK body） | 公开 tag/长度及本地完整 body；body 内容按对应前缀核采样，不细分构造循环的指令点 |
| FRAME_SEND_COMPLETE(p,tag) | ProtocolIFramedChannel::send 完整返回 | 本方完整发送的 application frame 与方向 sequence |
| FRAME_RECEIVE_COMPLETE(p,tag) | receive 完整返回 | 完整 peer frame 已进入本地 buffer；尚未假设 caller 已 decode |
| FRAME_VALIDATED / EXCHANGE_RETURN | decode_context/tag/body 检查后，或 and_layer 解开 D/E 并计算输出后 | 本地 peer body、opened bits/wires、消费 bitmap、algorithm accumulator |
| SEED_RNG_RETURN / SEED_FRAME / ACK_FRAME / SEED_ACCEPTED | exchange_sample_seed 中 RAND 返回、seed/ack 完整帧与 digest 校验 | 仅实际已产生/收到的当前迭代 seed；不含未来 seed |
| AND_BODY_READY(layer,batch=0,count) / AND_RESULT_READY(layer) | 每个非空 and_layer 一批的完整局部 body/材料消费，及收帧后输出 shares | layer/batch/count、当前消费 bitmap 与 wires；N≤256 时每层至多一批 |
| COMPARE_BODY_READY(iter,substage) / COMPARE_RESULT_READY | compare_wave body 构造、完整 exchange、逐 bit XOR 得到公开 outcome | active edges、local/peer bits 与已产生 outcome |
| SELECT_STATE_UPDATED(iter) | Select5 每轮由已公开 seed/outcome 更新 sample/pivot/candidate/residual/selected state | 当前迭代结束状态；未来轮次消息尚未生成 |
| OUTPUT_SHARE_FORMED(b) | 完成本地 local_output 向量构造 | 只对腐化 party 的本地 state 记录 staged z_b；API 尚未返回 |
| OWN_ENVELOPE_READY、SUBMIT_ENTER | 本方加密完成并进入 submit callback | 本方密文可见；A context/DB 仍隐藏 |
| ESCROW_READY_RECEIVED(op)、ESCROW_REQUEST_SEND_COMPLETE(op)、ESCROW_RESPONSE_RECEIVE_COMPLETE(op)、ESCROW_RESPONSE_PARSED(op) | submit/fetch 各自 TLS connection 上 7-byte READY 与完整 framed request/response | 只含本方实际收到的 response；不含 peer ciphertext |
| FETCH_DECRYPT_CHECKED、API_RETURN_SHARE | fetch token 比较、recipient decrypt/share check、函数返回 | 分别区分已取回和 API 已返回 share |

op∈{submit,fetch}。P0/P1 的 exchange_phase 顺序固定为 P0 send→receive→decode，P1 receive→decode→send；exchange_sample_seed 同样是 P0 发 seed 收 ACK、P1 收 seed 发 ACK。`FRAME_BODY_READY` 只在固定协议 operation 的 body 参数/本地 buffer 完整形成时产生；不细分构造循环中的任意指令。Beaver 每层/批次局部 gate loop 被视为一个固定协议 operation；不把每个单独 gate 的内存写入当成可切点。

A 的 DB 内部 NO_TX/PENDING/COMMITTED/ABORTED 转移与 response send/receive 分开：它们是 D 内的隐藏服务 transition，用于联合状态核，但不是 party-visible cut 标签；J 可落在相邻 party event 之间，party view 只含截至该点实际收到的完整帧。数据库状态、A context 和测试 controller 读数均不交给 Sim_b。TCP/TLS record 内 partial byte、poll/WANT_READ/WANT_WRITE、EOF、超时和任意机器指令点不在 E_cut。

该定义封闭且有限：在 N≤256 下 phase 最多 89；每个非空 layer 每 phase 一批；Select5 至多 5 次状态更新、4 个 sample exchange、9 个 compare wave；output refresh 固定一次；submit/fetch 各一次调用、各自一组 READY/request/response 事件。所有 `FRAME_BODY_READY` 标签均由这些有界 operation 生成。给定固定 P,L,sigma 后所有循环和标签均有界；不声称给任意失败/真实网络执行定义 cut。
## 5. `N≤256` 的 phase/frame 上界

令 `m=next_power_of_two(N)`，`W=32+log₂(m)`（稳定 priority key 包含 signed score key 与原始 index bits）。FTv3 最大 batch 为 `4,193,824` gates。由 S50 固定 N≤256 schedule 的 layer 公式重新映射到本源码：

| layer 类型 | 每层 gate 数上界（m=256,W=40） | 每层 batch/frame |
|---|---:|---:|
| input generate | `32m=8,192` | 1 |
| carry recurrence | `m=256` | 每 recurrence layer 1，共31层 |
| forward route | `m²W=2,621,440` | 1 |
| pair comparator generate | `C(m,2)W=1,305,600` | 1 |
| pair borrow recurrence | `C(m,2)=32,640` | 每 recurrence layer 1，共 `W−1=39` 层 |
| inverse route | `m²=65,536` | 1 |

当 `m≥2`，AND layer frame 数 `32 + 1 + 1 + (W−1) + 1 = W+34`，最大为 74；`m=1` 无 pair compare，AND layer 为 34。N≤256 所有可能 batch 均不拆成多个包，因此 layer/batch/count/phase/frame length 不受秘密输入造成的 batch-cut 影响。

Select5最多活动四个采样轮次，每轮一个 sample/ACK phase；前四轮最多 2 个比较 wave/轮，第五轮最多 1 个最终 clique wave，即 `≤4` sample phases 与 `≤9` comparison phases。加 READY 1 phase、AND layers 最多74 phase、output refresh 1 phase：Protocol-I 最大 `1+74+4+9+1=89` logical phases。五个算法迭代并不等于 89 phases、89 RTT 或网络时延。

Beaver batch body `12+ceil(2·count/8)` B；inner v3 context prefix 104 B；transport 材料绑定 header 56 B。route 最大层 `count=2,621,440` 时：body `655,372` B、inner payload `655,476` B、含外层 header 的明文 frame `655,532` B，低于 1 MiB payload ceiling。比较 wave 最多 `C(256,2)=32,640` edges，body `4,086` B、inner `4,190` B、frame `4,246` B。seed ACK 最大 inner payload 137 B；READY 105 B；P0 output-refresh 在 N=256 时 inner payload 365 B，P1 ACK 105 B。TLS record overhead 不计入这些 Protocol-I frame 数字，亦不是 RTT/性能测量。

## 6. 逐 cut sampler / 条件分布

下面首式是完整视图的联合 law 记法：`z_b`（如已形成）由 ideal `F_TOPK` 在本地 share-formation event 产生，并作为之后的本地状态值进入联合分布；`Kpre_prefix` 包含该 ideal 输出边际。紧随其后的分支式明确区分 share 形成前的无 `z_b` 条件 law 与形成后的给定本方 `z_b` 条件 law。

```text
Law_real(V_b^{≤j}, state_b^{≤j}, share_formed_b |
         x_b, public(τ), pkg_b^3, static_b, L_S29,
         ISSUED/cap_success, SuccessFence, sigma, J=j)
 = Kpre_prefix^{b,j}(x_b, public(τ), pkg_b^3, static_b, L_S29).
```

`V_b^{≤j}` 只含截至该完整事件边界已在本方 process/transport 可见的字段；`state_b^{≤j}` 含本地 register class、当前 phase/sequence、已准备 body、已收 frame、本地 gate-consumption bitmap 与算法 accumulator。`J=j` 是固定/外生切点，不是 success-after-timeout 选择。

为避免把“未来输出 share”隐含放入早期条件，逐 cut 的目标分布分两类写作：

```text
j < OUTPUT_SHARE_FORMED:
  Law(V_b^{<=j}, state_b^{<=j}, share_formed_b=0 |
      x_b, public(tau), pkg_b^3, static_b, L_S29,
      ISSUED/cap_success, SuccessFence, sigma, J=j)

j >= OUTPUT_SHARE_FORMED:
  Law(V_b^{<=j}, state_b^{<=j}, share_formed_b=1 |
      x_b, public(tau), pkg_b^3, static_b, L_S29,
      ideal F_TOPK's own output z_b delivered at OUTPUT_SHARE_FORMED,
      ISSUED/cap_success, SuccessFence, sigma, J=j)
```

第二行的 `z_b` 是该 corrupted party 在本地 share-formation transition 才从 ideal output interface 收到的状态值；它既不交给诚实对端，也不成为 `L_S29`。若 cut 固定在 share-formation 之前，目标法完全不条件于 `z_b`。每个证明实验只执行一个由外生 `J` 指定的 cut；不要求一个已停止的 prefix sampler 在未知未来 `z_b` 下同时承诺之后各 cut 的联合续接。

### 6.1 `j` 在 online output-share formation 之前

对 READY 到 membership/inverse-route 的计算段，使用 S36 `H4_b` 的逐层 sampler：

1. 只解析腐化方的 `pkg_b^3` 与本方 wires/triple/permutation shares；按真实公开 gate ID 初始化本地 consumed bitmap、phase 与 sequence。
2. 对下一个未消费 Beaver gate，条件于固定本方完整 package、全部既有 view 和 wires，S36/S50 的条件联合 law 给出未使用全局 `A_g,B_g` 均匀独立；令 `D=X⊕A_g,E=Y⊕B_g`。抽均匀 `(D,E)∈{0,1}²`，本地 payload 是源码决定的 `d_b=x_b⊕a_b,e_b=y_b⊕b_b`；peer payload 写 `(D⊕d_b,E⊕e_b)`。依照 `c_b⊕(D∧b_b)⊕(E∧a_b)⊕([b=0]∧D∧E)` 生成本方 local gate output。FTv3 批量版本将这些位按 layer/batch/count 打包；`N≤256` 每层最多一个 batch。
3. 对 active `kTagOpenedCompare` wave，给定该 active outcome `o`（在兼容的 `L_S29` 中），按 S36/H4 的 local-bit 条件核生成本方 bit vector `q_b`，peer vector 是逐位 `o⊕q_b`；phase、iteration、substage、edge count、padding 和 frame length 都按源码确定性编码。收帧后 outcome、下一 active edge set、candidate/pivot/residual 更新照真实控制器计算。
4. sample seed/ACK 使用 `L_S29` 中对应活动轮次的 seed；ACK 为 `(iteration,SHA256(seed))` 的确定性后处理。未来未活动 seed/frame 不提前放入 `state_b^{≤j}`。完整 `L_S29` 可以是 simulator 输入，但它不授权把未发生消息放入 cut view。
5. 对每个源 event 边界，只运行到相应完整 send/receive、phase change、layer result 或 Select5 state update。send-only cut 保留自己待收前缀；receive-before-send 的 P1 cut 先接收再发送，符合源顺序。局部 wires/consume bitmap 在本方可见，不能转交给另一方。

不从单个 Beaver opening 的无条件均匀性推导整前缀。每一步的核条件于本方完整 package 和已模拟 history；未消费 gate 的 ideal triple masks 与此前使用门独立，因此归纳替换成立。活动比较 bit 是相同被允许 outcome 下的条件 kernel，活动图按 seed/outcome 重放。

S50 `Kpre₃` 使用固定 `z_b` 生成完整 pre-escrow transcript。它本身不能直接截断成无 `z_b` 的 law。本轮所需桥接是 S36 §4.3 的明确关系：在最终 output-refresh 之前，刷新随机量 `R` 尚未进入本方 wire/打开值；所以对每个兼容 `z_b`，计算 view `W_b` 具有同一 law：

```text
Law(W_b | x_b, z_b, pkg_b^3, L_S29, cap_success)
 = K_comp^b(x_b, pkg_b^3, L_S29),   for every compatible z_b.
```

由全概率边缘化，而不是凭边缘猜测：

```text
Law(W_b | x_b,pkg_b^3,L_S29,cap_success)
 = Σ_z Pr[z_b=z | x_b,pkg_b^3,L_S29,cap_success]
       Law(W_b | x_b,z_b=z,pkg_b^3,L_S29,cap_success)
 = K_comp^b.
```

对任何计算前缀投影 `π_j(W_b)` 仍成立。因此 output-share 前所有 READY、D/E opening、batch frame、seed、active compare、局部 Select5 控制和 inverse-route cut 均不需要也不取得未来 `z_b`。这里继承的是 S36/S50 的理想材料条件 law，不是实际部署 RNG bridge。

### 6.2 Output-refresh 事件边界（精确 P0/P1 次序）

P0：在 OUTPUT_SHARE_FORMED(0) 前，尚无 R/z0。ideal F_TOPK 刷新事件抽 R←U({0,1}^N)，将 z0=R 放入被腐化 P0 的 staged 本地状态；随后在 `FRAME_BODY_READY(0,output-refresh)` 构造 u=R XOR m0。cut 只有在 share 形成后才可向 Sim_0 提供 z0。给定 z0 与本方 m0，其后 frame body 是确定值；此前计算 transcript 则从 K_comp^0 采样，不需未来 z0。

P1：真实源码先在 `FRAME_BODY_READY(1,output-refresh-ack)` 构造本地请求 body {1} 并进入 exchange_phase，之后执行 receive()、decode/校验并将 P0 完整 frame body u 放入本地 buffer，接着才 send() ACK {1}。exchange_phase 返回后 caller 校验 ACK/body 长度并逐位执行 local_output[i]=u_i XOR m1_i。因此至少区分：
1. FRAME_RECEIVE_COMPLETE(output-refresh) 到 FRAME_SEND_COMPLETE(ACK) 之间：本地已收到完整 u，ACK body 在本地待发，share_formed_1=0；
2. ACK 已完整发送、exchange 返回到 OUTPUT_SHARE_FORMED(1) 之间：u 已本地可见，仍 share_formed_1=0；
3. OUTPUT_SHARE_FORMED(1) 之后：z1 已作为本地 staged share 存在。

前两个 cut 均不允许模拟器拿 z1。P1 的 ACK body 在 `FRAME_BODY_READY` 时固定为公开常量 `{1}`。因为 P0 的 ideal R 新鲜均匀且独立于此前 transcript 与 hidden m0，P1 收到的 refresh frame payload 条件 law 为：

Law(u | V_1^pre-refresh, L_S29, m0, P, cap_success, SuccessFence)
  = U({0,1}^N).

所以对这两个 cut，模拟器从均匀分布抽 u，构造真实格式完整 frame body（公开 tag、N 和长度照源编码）；在 sigma 指定的次序发送固定 ACK。它没有得到 peer share，也没有得到未来 z1。这不是单字段边缘替代：R 独立均匀意味着对每个固定 V_1^pre-refresh,m0,L 都有相同条件核。

在 OUTPUT_SHARE_FORMED(1) 之后，模拟器才可读本方 staged z1，依据本方已模拟的 m1 设置 u=z1 XOR m1，再沿真实 ACK/解码/本地状态函数生成后续前缀。此关系与 S36/S50 固定输出份额条件核一致。share_formed=1 只说明腐化方进程内存已有本方 share；它尚未表示加密提交、A COMMITTED、fetch 成功或 API 返回。
#### 6.2.1 每类 cut 的 Kpre-prefix 采样核

对固定 b、公开 P/L、source-event 调度 sigma 和 cut j，构造按 cut 类别分派的核：

    Kpre_prefix(b,j):
      若 j < OUTPUT_SHARE_FORMED:
          sample W_b <- K_comp^b(P,L_S29,pkg_b^3,static_b)
          return projection/state_b of W_b through event j
          （若 j 是 P1 refresh receive-complete/pre-ACK 或 ACK-complete/pre-XOR：
           additionally sample u <- U({0,1}^N)，按真实 phase/send 顺序推进到 j）
      若 j >= OUTPUT_SHARE_FORMED:
          sample W_b <- K_comp^b(P,L_S29,pkg_b^3,static_b)
          at OUTPUT_SHARE_FORMED invoke the ideal F_TOPK share source for this
          corrupted endpoint and record only its own staged z_b
          set the refresh frame from z_b and the already-sampled local mask share
          if j is after submit/fetch entry, continue with Kpost_prefix(b,j)

这里 K_comp 是 S36/S50 已复核的 ideal IID FTv3 cap-success 前缀核；切点 j 是 projection stop label，不是模拟器可以提前看到的未来消息。S36 给出的强条件关系是：

    for every compatible z:
      Law(W_b | x_b,z_b=z,pkg_b^3,static_b,P,L_S29,cap_success)
        = K_comp^b(x_b,pkg_b^3,static_b,P,L_S29).

于是对任何早于 share formation 的 source-event projection pi_j：

    Law(pi_j(W_b),state_b^j | x_b,pkg_b^3,static_b,P,L_S29,
         cap_success,SuccessFence,sigma,J=j)
      = pi_j # K_comp^b.

对 share formation 后的 cut，模拟器在 share-formation transition 才拿到 z_b，并沿固定-z 条件核采样 W_b；条件于 z_b 的 kernel 与 K_comp 相同，故 joint pair (W_b,z_b) 的 law 与理想刷新功能一致。P1 的 receive-before-XOR cut 另加前述均匀 u 条件核。该构造既不把未来 z_b 给早期 cut，也不将完整 Kpre_3 transcript 截断冒充条件模拟。

### 6.3 本地 share 已形成至 submit/fetch/API 的成功事件前缀

对 OUTPUT_SHARE_FORMED(b) 后 cut，Sim_b 可取得且只取得本方 staged z_b。本方本地 envelope C_b 由模拟器用自己的 recipient public key 与同格式随机算法生成；这是腐化 party 自己能看到的内容，不需要 peer ciphertext hybrid。之后采用 event-indexed causal kernel Kpost_prefix^{b,j}：模拟器按 sigma 从当前 prefix 继续执行真实 endpoint API 到 cut j 即停止，不先生成完整成功 transcript 再截断。

- 诚实 endpoint 在同一个理想联合世界中用自己的输入、z_(1-b)、pkg_(1-b)、凭据和公开元组独立生成第二份密文/认证 deposit。party simulator 不读取或代造这些对象。
- ideal A 内部持有 metadata-only ctx_A，按照源码协议处理真实到达的请求。首份有效 deposit 后可能为 PENDING；第二份有效 deposit 后才抽一次理想独立 token U、提交 COMMITTED 并保存 U。A 和诚实 endpoint 仅经真实状态机/认证接口因果交互。
- SuccessFence 明确保证该 scope 内各个合法消息/服务事件按 sigma 完成且不触发 deadline/error。若 cut 在腐化方 request send 后、response receive 前，party prefix 只有已发送的本方 frame 和当前本地 staged share；A 可能已经内部 PENDING/COMMITTED，但这些状态不进入 V_b。若 cut 在 COMMITTED response receive/parse 后，模拟器只收到自身 COMMITTED,tx,U。fetch request/response 同理；fetch 返回本方 C_b,U，之后才解密检查、API 返回本方 z_b。
- 理想 token law 在两份有效 deposit 验证且 ideal A RNG 成功的条件下为 Law(U | V_b^pre,C_b,ctx_A,C_(1-b),valid_deposits)=Uniform({0,1}^256)；U 只抽一次并在 submit、fetch 和重复 fetch 后处理里复用。隐藏 context/peer ciphertext/DB 状态不交给 Sim_b。这是理想 token RNG 的局部条件 law，不桥接真实 A RAND 或全系统错误状态。
- 每个 prefix 通过逐消息 transition kernel 生成：对方请求未到达时不虚构其消息；A response 未到达时不生成 party response；只有 second deposit 已在 sigma 中处理后才生成 COMMITTED；只有本方 fetch response 已收齐且本地检查成功后才产生 API_RETURN_SHARE。因此这是有因果状态的 prefix sampler，不是完整 Kpost_3 transcript 的截断替代。

对应事件核只覆盖理想成功执行；不覆盖 ABORTED、COMMIT_STATUS_UNKNOWN、receipt/fetch 错误、response 丢失、restart 或 timeout selection。S51 的 Kpost_3 完整成功后缀仅在 SuccessFence 下成立；本节逐切点扩展按状态机转移直接采样。本轮 reviewer 确认这项增量 prefix-kernel 推导可从 S51 理想状态机因果延伸，故 `Kpost-prefix` 为 `PASS_WITH_SCOPE`（继承 S51 ideal token/Kpost 引理）；reviewer 未重新认证 S51 token 引理、真实 escrow 源码 refinement、密码学组件或运行时行为。
## 7. P0/P1 分别模拟与对称性

| 类别 | P0 (`b=0`) | P1 (`b=1`) | 结论 |
|---|---|---|---|
| Beaver opening 帧 | 先发送本地 d/e，再接收 peer partial | 先接收 peer partial，再发本地 d/e | 同一条件 joint kernel；cut state/phase 顺序分别保持源码方向 |
| 比较 wave | 先发本地 q bits，再收 peer bits | 先收 peer bits，再发本地 q bits | 固定允许 outcome `o` 后，peer bits=`o⊕q_b`；不打开 inactive table |
| 活动 seed | P0 生成并发送 32-byte 活动 seed，收 ack digest | P1 收 seed、重算 digest、发 ack | `L_S29` 仅向模拟器提供允许活动 seed；未来未活动 seed不出现在 prefix state |
| output refresh | 先形成 z0=R，`FRAME_BODY_READY` 构造 u=z0 XOR m0，发送后收 P1 ACK {1} | 先构造固定 ACK body {1}，完整接收 P0 u，后发送 ACK，再由 caller XOR 形成 z1=u XOR m1 | P1 receive-complete 到 share-formed 的两个 cut 均用均匀 u sampler；只有形成后才可接收本方 staged z_b |
| escrow | 自己密文先后顺序可变；只见自己服务响应/取回 | 同左 | A context、peer ciphertext、DB 状态不交给 party simulator；honest endpoint独立按自己状态提交 |

两方不是按相同源码顺序同步发送；对称性指各自条件联合 law 能以该方实际 send-first/receive-first 顺序采样，不是把 P0 的消息先后复制给 P1。

## 8. 泄露字段及 cut 条件

| 字段/状态 | 源码来源 | 是否秘密依赖 | sampler/合同分类 |
|---|---|---|---|
| phase、sequence、tag、session/invocation、frame type/length | `exchange_phase`、`encode_context`、transport header | phase/sequence/length 由 schedule 决定；session 等为公开绑定 | 在固定公开元组、N≤256 和 `L_S29` active schedule 下确定；不是新增泄露 |
| layer ordinal、batch、count、consumed bitmap | FTv3 `and_layer` | gate 位内容秘密，N≤256 只有公开固定一批；本地 consumed 由调用序列决定 | count/shape 定值；bitmap 为本方完整 view，逐步从 gate ID 重建，不是对外 `L` 字段 |
| packed D/E partials 与 local gate output | Beaver frame | 是本方 secret state | 以固定本方材料/历史的 joint opening kernel采样，不从公开长度推出 |
| sample seed、candidate/active endpoints、pivot、opened outcome | party + Select5 callback | 活动路径由获准 outcome/seed控制 | 仅使用已批准 `L_S29` 条件值；future messages不进入 cut view |
| output refresh body `u` | party output refresh | 内容与本方 mask share相关 | share formed前分别按 H4/均匀 refresh kernel；形成后由 staged own share重建 |
| PENDING/COMMITTED/token/Ciphertext peer/hash/context | escrow server/DB | hidden service state/peer数据 | 不属于 party view或 `L_S29`；只有真实 response/fetch字段按 event 交付 |
| submit/fetch status, receipt, own fetch cipher | escrow client callback response | 与 A 状态相关 | 仅按理想 A 状态机/相应成功 event生成；不借 DB controller query |
| wall clock、timeout、TLS records、partial byte prefix | OS/TLS/transport wait loops | 可能受数据、调度及隐藏服务状态影响 | 本轮不建模；保持真实 finite timeout/transport view OPEN |

`E_cut=j` 本身是环境事件选择，不扩展 `L_S29`。如果把 `J` 允许依赖 secret-dependent activity 或真实耗时，必须另建调度联合核；本轮只把 event label 固定/外生地给定。

## 9. S50/S51/S53 可复用范围

- **S50 Kpre₃：**可复用其理想 FTv3 条件材料法、逐门 Beaver opening、获准 active comparison 条件 bit、Select5 adaptive trace 以及固定 `z_b` 的 output refresh。此处新增的是 prefix projection/不带未来 `z_b` 的边缘化等式及 P1 的 receive-before-share sampler，不能只说“直接截断”。
- **S51 Kpost₃：**可复用理想 token RNG、真实 honest endpoint causality、成功 COMMITTED receipt/fetch 的成功事件核；本轮另外按每个 A-service response event给出 success-only prefix sampler。不能迁移 ABORT、UNKNOWN、response 丢失、fetch 错误、服务重启或任何失败概率。
- **S53 SuccessFence：**只用来说明理想 timely-success 语句的既有范围。它不是本轮 cut selector，也不是实际 deadline保证；S61 没证明 `Law(Vpre|success under real timeouts)`。
- **均不推出：**真实 T/A DRBG、签名、RSA-OAEP/AES-GCM、TLS/mTLS 部署优势；N>256 安全扩域；abort-aware 的 unfiltered T_AB；故障恢复安全；正式性能/RTT。

## 10. 停止点和其余 OPEN

在限定 `ISSUED/cap_success` + 外生 event cut 的 online-prefix 子目标内，没有留下一个无法生成的 `D/E`、active edge、phase/length、完整 body-ready 或本方 staged state 变量；最关键的条件转换为前述 `W_b ⟂ z_b | x_b,pkg_b,L_S29`，以及 P1 输出 frame receive cut 的均匀 `u` 核。独立 reviewer 对其限定的增量步骤作了只读确认；S36/S50 Beaver 条件材料核与 S51 escrow/token 基础引理仍属继承前提，并非本轮重新证明。

从完整系统视角，范围外首个未闭合合同仍是 S60 记录的 `FTv3 ISSUANCE_ABORT → 两方 package/context delivery → 双方 start/notice` 联合核。即使该点有负责人合同，也仍需证明不同协议验证拒绝、ABORT、escrow UNKNOWN/fetch failures 的 cut law。理想 A 状态核不提供实际 SQLite/failure/refinement 证据。真实有限 timeout 还缺 scheduler/deployment duration kernel，不能用外生 cut sampler代替。

`F_TOPK-ISSUE-ESCROW/v1` 仍是未批准/未实现的 proof-only 候选。后续若要把本轮 success-only cuts 扩成 abort-aware 功能，至少需将本报告 `E_cut` 标签与 `share_formed_b`/当前完整帧计数加入理想状态转移；并新增 cap-abort 到同 invocation package/context/start/notice、协议验证拒绝、honest peer 不提交、A ABORT/UNKNOWN、response 不可达和 fetch/recovery 失败的逐方条件核。A 内部状态仍不得作为 party simulator 输入，`J` 不得进入 `L_S29`。这只列明待补接口状态与证明工作，不把候选视为已批准或已实现。

需要负责人确认的事项仅为 cap-abort 烧毁 invocation 后是否只允许新 invocation 重试、发材/启动失败通知及不可达时双方可见状态规则、以及隐私定理是否覆盖协议级 abort。N≤256 是本轮证明子目标的适用域，不由本轮决定为产品永久输入上限。历史对“算法随机失败数值上限”的延期不是 cap-abort 接受。

## 11. 执行、审查与交付

本轮只读源码/证明审查并写文档：runtime/tests/ABI/default switches/L 均未改；BUILD=`NOT_RUN`，TEST=`NOT_RUN`，不重跑旧矩阵，不做网络故障注入、LAN/WAN、RTT、AAV86 或性能实验。自然业务 I/O 本轮未测试，状态沿用 `NOT_REPRODUCED`。

非作者 `/root/s61_prefix_review` 明确确认未参与 S50–S61 推导。其只读复核独立重推了固定 `z_b` 下的前缀边缘化、P1 receive-before-share 的均匀 `u` 核、有限 `E_cut` 及其 exogenous `sigma/J` 条件，并确认 S51 理想 token/Kpost 成功状态机可按因果转移扩展到 event-prefix sampler。Reviewer 未重推 S36/S50 Beaver 材料条件核，也未重新认证 S51 token 引理或真实 escrow 源码 refinement。Reviewer 指出并纠正草稿原来把 P1 ACK 写在 receive 前的源码顺序错误；还要求明确 SuccessFence、J 不属于 party view、成功 setup 是 cut 起点，并补列 output-refresh `FRAME_BODY_READY`。本报告据此修订。Reviewer 未构建/测试/修改文件；理论复核不是功能接收、完整安全认证或部署验证。

S61 输出：本决策、详细报告、机器可读 gates、source/evidence manifest，以及 PROJECT/IMPLEMENTATION_PLAN 的当前状态摘要。逐项文件 SHA-256 和 canonical delivery 摘要见 manifest。`VFSS-baseline/` 不变；没有密钥、材料、证书、构建物或原始大日志进入仓库。未推送、合并或创建 PR。
