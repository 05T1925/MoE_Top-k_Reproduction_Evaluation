# M6B-I-PROJECT-SELECT-5ITER-S30 工作报告

日期：2026-10-20
路线：PROJECT_DERIVED_SELECT_5ITER（项目派生的五迭代 Select；不是 BB90/BMW16 原算法复现）
范围：冻结 S25 候选的成功输出 share law 与 crash 证据边界；对 Sim_b(x_b,z_b,pp,L) 做一次有停止条件的完整 score-conversion party-view sampler 尝试；提出版本化 score-to-mask-v3 规格与准入裁决。未改运行时代码，未运行功能矩阵或性能矩阵。

## 1. 裁决摘要

| 门 | S30 状态 | 结论 |
|---|---|---|
| 被审对象 | PASS | S25 manifest 的 74 个文件均匹配；canonical SHA-256 为 64f32993b12bcd8692ed089f5dcdb8ce1be9f181ff217b508b8b0470eb8e440d。 |
| S25 功能接收 | PASS_WITH_SCOPE（继承） | S27 对同一 manifest 快照完成限定范围独立接收。S30 没有复跑测试，也没有把该收据称作安全证明。 |
| 运行时 share law | PASS_WITH_SCOPE（从源码推导） | 在理想独立随机带模型下，两方 shares 均匀 XOR 重构为代码实际生成的 M_runtime。真实 OS RNG 联合优势未量化。 |
| 全输入稳定 Top-K 正确性 | NOT_PROVEN / 仅继承有限功能收据 | 将 M_runtime 等同于所有允许输入/随机带下的理想稳定 Top-K M，还需全域正确性证明；S27 的有限 E2E 收据不能替代。 |
| abort/crash 输出合同 | NEEDS_OWNER_DECISION | 源码/API 与 S25 crash 探针不支持 all-or-nothing 结论；报告保留 F_TOPK-PREFIX 与 F_TOPK-ATOMIC 两个精确候选，不代替负责人选择。 |
| P0/P1 完整 v2 party-view sampler | OPEN/NO_GO | 未能从 (x_b,z_b,pp,L) 生成 ready 前完整材料、score-conversion、后续消息及任意前缀的可审查同分布 sampler。这是证明缺口与未量化条件分布，不是区分器或不可能性证明。 |
| v2 模块组合 | OPEN/NO_GO | F_scoreconv/v2 把 Q′_b 给腐化方会扩大外层输出；隐藏它则仍需缺失的最终输出条件联合 sampler。 |
| v3 规格 | DESIGN_SPEC_ONLY | 给出以 GMW 半诚实 2PC 与一次性 Boolean Beaver preprocessing 为后端的版本化 API、材料 schema、DAG 和证明义务；该规格不等于已证明或已实现。 |
| 运行时准入 | NO_GO_PENDING_OWNER_AND_PROOF | 不修改 VFSS/。路线选为“转入 v3 设计规格”；实现前须冻结 abort 合同、确认 v3 的泄露/通信观测定义，并完成独立密码学证明与审查。 |

## 2. 身份、材料与证据边界

### 2.1 被审快照

被审对象是独立 S25 worktree：

C:\Users\28641\.codex\worktrees\m6b-project-select5-s25\MoE_Top-k_Reproduction_Evaluation

- S25 manifest：docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S25_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-19.json
- manifest SHA-256：c96450eb62b186e866291b6c4415441e0ef3216703612314542e1d2ee25372eb
- 文件数：74；每个 manifest 文件的本轮 SHA-256 都匹配。
- canonical SHA-256：64f32993b12bcd8692ed089f5dcdb8ce1be9f181ff217b508b8b0470eb8e440d
- 基础提交：c3926c68fd14f270faa8b55234311071947fa080；S25 候选未提交。不能只用基础提交替代候选。
- S25 相对 S24 只改了 3 个测试源文件，71 个文件相同；没有 S25 运行时代码改动。

核验按 manifest 写明的 canonicalization 执行：UTF-8、相对路径 StringComparer.Ordinal 排序，每行 path<TAB>lowercase_sha256<LF>，然后对串联字节计算 SHA-256。S25 functional receipt 只继承 S27：PASS_WITH_SCOPE。S27 接收者独立于 S19–S26；这证明的是记录范围内的构建、测试与 E2E 接收，不是完整密码学安全签核。

### 2.2 证据分类

| 类别 | S30 使用方式 |
|---|---|
| 论文定义 | 论文可支持其自身的 DCF/密码学原语结论；它们不定义本地 PROJECT_DERIVED_SELECT_5ITER 的五次迭代算法、share law、lease、abort 或完整 party view。没有把项目代码行为反写成论文结论。 |
| 本地运行时行为 | 以 S25 精确快照中的 C++ 调用链、材料生成和 S25 crash 测试为依据，列出代码实际生成/可见的状态。 |
| 项目派生定义 | F_TOPK-U、已批准泄露 L、Sim_b 输入和 v3 接口属于本项目的理想功能/架构合同。 |
| 待验证设想 | v2 完整联合 sampler、跨 RNG 联合优势、完整自适应池混合、v3 的 GMW transcript/declassification/abort 组合证明均未完成。 |

本轮阅读了 S25 worktree 的 PROJECT.md、docs/IMPLEMENTATION_PLAN.md、S25 决策/报告，S27/S28 接收 worktree 的报告/决策，以及主工作区的 S29 报告/决策。VFSS-baseline/、论文和本地参考工程保持只读。

## 3. 冻结外层理想功能与模拟器输入

### 3.1 成功语义

令 b∈{0,1}。输入是两方长度为 n 的 Q20.12 signed score shares x0,x1，重构为 raw[i]=(x0[i]+x1[i]) mod 2^32。公开参数 pp 含 n,K,padded_n,h,c、稳定同分规则、party/session/fingerprint 和协议/ABI 绑定。理想功能按 signed score 降序、原始 index 升序求唯一 Top-K mask M∈{0,1}^n，且 Σ_i M_i=K。

S25 成功输出的本地代码行为支持以下合同：

    z0 ← uniform {0,1}^n
    z1 = z0 XOR M
    P0 只得到 z0；P1 只得到 z1

因此逐方模拟器接口冻结为：

    Sim_b(input_share_b, prescribed_final_output_share_b, public_params, approved_L)

模拟器不获得对方输入、M、Q_b、Q′_b、rho_b、carry/sign 本地 share、DCF/uCMP key、Dealer tape、party package 明文或其他材料中间值。失败时，模拟器只可获得冻结理想功能明确规定的该方输出/状态；当前 abort 合同仍待负责人决策。

### 3.2 已批准泄露 L

沿用 S29 记录，不加入 Q′ 或其他字段：

- 公开参数及协议绑定；
- 获准公开的 shuffle endpoint/位置；
- pivot 与比较结果 bit；
- forward shuffle 完成后才按活动迭代公开的 sample seed，不提前披露未来 seed；
- 已批准的 masked values；
- comparison count、通信长度与统一 ABORT。

Q_b/Q′_b、rho、carry/sign 本地 share、DCF/uCMP key、Dealer tape、party-local package/manifest/lease、shuffle 本地状态及内部输出前缀属于 party view 的待模拟部分，不是 L。L 的模拟器也不能直接读真实材料或 Dealer tape。

### 3.3 输出前错误

无效公开参数、维度/取值越界或协议绑定不匹配，理想功能应在任何 mask share 输出前返回统一 ABORT。具体校验集合、错误与异常的映射及状态码尚未被冻结，因此只有上述抽象语义，不能声称 S25 所有错误路径均符合它。

## 4. 成功 share law：源码推导

关键源码均在 S25 精确快照中：

1. VFSS/src/moe_topk/protocol_i_parallel_shuffle.cpp:345-365 生成 h，并构造 e0 = -tau0(a1)-h、e1 = -tau1(a0)+h。
2. :504-522 计算每方 inverse-shuffle 本地记录 tau_b(peer_round1)+e_b。代入 round-1 记录后，word1 分别含 π(s1)-h 与 π(s0)+h；s0,s1 是 membership 产生的本地 shares，π 是本轮 inverse routing。
3. VFSS/src/moe_topk/protocol_i_project_derived_select5.cpp:2237-2246 将 DCF membership shares 放入 carrier 并执行 inverse shuffle。
4. :2249-2252 对前 logical_n 个原序记录取 word1 & 1 作为输出 share。
5. DCF membership shares 在 :2237-2240 的 P0/P1 公式下按协议规则重构成运行时实际选中 mask；inverse routing 恢复原输入顺序。源码推导不单独证明对所有输入/随机带均选出理想稳定 Top-K。

对 R_c=Z/2^cZ 中任意 64-bit 加减，最低位满足模 2 的加法/减法律。因此在理想随机带中，如果每个 h[i].word1 & 1 是独立均匀 bit，则：

    z0[i] = π(s1)[i].LSB XOR h[i].LSB
    z1[i] = π(s0)[i].LSB XOR h[i].LSB
    z0[i] XOR z1[i] = M_runtime[i]

每个 vector<uint8_t> 位置对应一个独立输出字节，字节值只为 0 或 1（不是 bit-packed）；给定 M_runtime，z0 在 {0,1}^n 上均匀，z1 每位为 z0 XOR M_runtime。每方的单方输出边缘均匀，双方 XOR 精确等于运行时实际 mask；没有一方获得完整运行时 mask。该 share law 与理想目标 F_TOPK-U 的 share law 形式一致；M_runtime=M（对所有允许输入/随机带均为稳定 Top-K）仍需另行证明。

适用边界：上述是源码代数推导加理想独立随机带模型下的精确 share law。S25 使用 OS getrandom（如 protocol_i_parallel_shuffle.cpp:50-65,84-85），而 DCF BCG21 key generation 使用 OpenSSL RAND_bytes；本地 API 名称不同不能证明联合独立或给出优势界。epsilon_rng_joint 仍未量化。故这是运行时 mask share law 的 PASS_WITH_SCOPE，不是全域 Top-K 正确性证明，也不是完整 view 模拟通过。

进一步，若完整前缀与隐藏 base 已能采样，可尝试将 inverse-shuffle 随机 pad h 的对应低位条件化到 prescribed z_b，并同步生成 e_b 及相关 package/转录状态。这只是最终输出层的候选 pad 步；e_b 还含对方 a-share，且完整条件联合 law 未证明。它不能生成前置完整 party view，也不能替代 v2 SimBase。

## 5. 中止/崩溃合同：源码事实与待决事项

### 5.1 调用顺序和本地 share 可见性

- Durable package 路径在 protocol_i_project_derived_select5.cpp:1917-1967 先 claim session/material；注入 crash step 1 位于 package claim/score conversion 之前。
- 成功计算先在 :2237-2252 产生本地 membership 与 inverse-shuffle output_share。
- 随后 :2254-2277 执行本地 .prepared、双方 PREPARE、.commit、双方 COMMIT、本地 .published，之后 API 才 return 本方 share。
- durable_output_phase 写的是本地阶段标记，不是 mask share 的远程原子发布。注释也承认崩溃可能留下本地 orphan share。
- E2E 子进程在 API 成功后才向结果 pipe 写 8-byte 长度，再写 mask bytes；crash 检查 protocol_i_project_derived_select5_three_process_e2e_test.cpp:762-786 只读取并验证长度头，未读取 payload。

### 5.2 六个已测 P0 crash 注入前缀

| 注入点 | 源码阶段 | P0 本地 output share | S25 探针记录 | 能支持的结论 |
|---|---|---|---|---|
| 1 | durable session claim 后，package claim 前 | 尚未进入 score conversion/最终输出计算 | P0 exit 81；P1 exit 2；两侧结果长度头 0 | 该测试时序没有观察到结果 share 发布；不涵盖并发的所有 crash 调度。 |
| 2 | output_share 已形成；写 .prepared 后、PREPARE 前 | 已在 P0 进程内存 | P0 exit 82；P1 exit 2；两侧长度头 0 | 没有证明 P0 从未持有本地 share；只测到无结果头。 |
| 3 | P0 收到 peer PREPARE 后 | 已在内存 | P0 exit 83；P1 exit 2；两侧长度头 0 | PREPARE 不是 API 输出；没有原子交付证明。 |
| 4 | P0 写 .commit 后、COMMIT exchange 前 | 已在内存 | P0 exit 84；P1 exit 2；两侧长度头 0 | COMMIT 前缀的已测结果；不能推广到任意调度。 |
| 5 | P0 收到 peer COMMIT 后 | 已在内存 | P0 exit 85；P1 exit 0；P1 子进程写出 8 字节长度前缀，编码的 payload length=n=7 | P1 API 返回并向子进程开始写结果；探针只读 8 字节长度前缀，没有读 payload。 |
| 6 | P0 写 .published 后 | 已在内存 | P0 exit 86；P1 exit 0；P1 子进程写出 8 字节长度前缀，编码的 payload length=n=7 | P1 可独自发布/持有本地 share；P0 崩溃，无证据称双方无 share。探针只读长度前缀，没有读 payload。 |

在步骤 2–6，P0 至少在自身地址空间已形成本地 share；步骤 5–6 的 S25 测试可观察到 P1 的输出长度头。上述探针谓词是 NO_COMPLETE_MASK，并非“无人得到本地 share”“原子无输出”或“all-or-nothing”。其他任意时序中 P1 在何时形成本地 share，不能从单次注入记录推定。

统一错误/ABORT 路径使用最终状态通道发送 0（:2284-2309；未进入最终状态阶段时 abort_best_effort 会尽力通知），但网络故障时双方是否都收到该字节并无保证。S25/S27 的 UNIFORM_ABORT_NO_MASK 只覆盖其记录的 case。

### 5.3 必须由负责人选择的候选合同

abort_contract=NEEDS_OWNER_DECISION。只保留以下两个选项：

1. F_TOPK-PREFIX：每方 API 若返回则理想输出该方 share，若在返回前终止则该方输出 ⊥；另一方允许已经返回并得到自己的 share。进程内部已计算但未返回的 share 属于 view。该合同与 API 前缀模型相符，但仍需补齐统一 ABORT、材料 claim/consume 和每个阶段的状态映射。
2. F_TOPK-ATOMIC：只有两方都原子完成 PUBLISH 才对两方交付 share，否则双方均为 ⊥。这比当前证据强；需要外部原子发布机制及新证明，不能由 .prepared/.commit/.published 标记或 NO_COMPLETE_MASK 推导。

本报告没有选择较弱合同来帮助模拟器通过。负责人决策前，不为任意 abort/crash 前缀声称完整理想功能安全。

## 6. 有停止条件的 v2 完整视图 sampler 尝试

### 6.1 被模拟 view 与停止规则

对腐化方 P_b，完整 view 包含：本方 x_b、公开参数/绑定、ready 前完整本方 package；rho、carry/sign 的本方 masks 与 DCF keys；manifest/digest/segment/ready/lease/claim 与 used/unused 地址；phase 4/5 frames、同一 DCF key 的两次 Eval、本地 carry/lift/sign/Q_b/Q′_b；q_b/key_records；forward shuffle 本地状态与消息；活动 Select 轮的自适应地址、pivot/seed/比较 bit、used/unused DCF 槽；membership、inverse shuffle、本地 output share 与最终状态交易。Dealer 完整 tape 不属于 party 输入，模拟器也不能读取真实 tape。

停止条件：从已知 x_b,z_b,pp,L 只推进到第一个没有可审查条件 sampler 的分布项；一旦需要未知输入或缺失的 joint conditional law，不假定均匀边缘、忽略 package/lease，或继续声称 N=128/256 的整池模拟。Dealer package 可用新随机带无条件重采样；真正的首个停止点是无法把该 unconditional package sampler 联到指定 L 及 carry/sign frames/Eval transcript 的条件联合 law。未尝试拼接后续全协议 sampler。

### 6.2 逐阶段真实/候选分布对照

| 阶段 | 真实 party 可见分布 | 候选 sampler 进展 | 裁决 |
|---|---|---|---|
| 理想接口 | 输入 x_b；成功输出 z_b；公开 pp,L。不给对方输入或内部状态。 | 固定 Sim_b(x_b,z_b,pp,L)，与 S29 一致。 | 接口冻结；不授权加入 Q′/rho。 |
| Dealer package / ready 前 | T 生成 carry/sign、rho、Select DCF 池、两组 shuffle metadata、membership DCF 池；M6B5/v4 单体 package 或 M6S2/v3 streamed slots 的 ready/digest/manifest/lease 由内容与绑定决定。 | 可用独立新随机带运行 package generator，得到无条件 package/ready/digest/lease 样本；但不能据此生成给定 x_b,z_b,pp,L 与后续线上 transcript 下的完整 view。rho_b 已在 ready package 中；不得把 rho 当成在线新抽取，也不得删除 digest/lease。 | 无条件 package law 可采样；尚未到第一处线上条件分布。 |
| Carry phase 4 / sign phase 5 | protocol_i_score_input.cpp:90-139 发送/收到 masked frames、对本方 DCF key 双次 Eval，形成 carry/lift/sign 与 Q_b。 | 首次无法从无条件 package law 转到 Law(Pkg_b, carry/sign DCF key+mask, frames, Eval outputs | x_b,z_b,pp,L)。S28/S29 的 restricted V_pre 候选只在其显式条件及 L_prefix 下可用于模块级分析；AES/DCF、跨 RNG、TLS 和完整 package 联合界未量化。 | 本轮首个停止点：缺少上述条件联合 sampler，是证明缺口/未量化依赖；没有区分器，也未证明 sampler 不存在。 |
| rho refresh / Q′_b | protocol_i_score_input.cpp:156-169 以 Q′_b=(Q_b+rho_b) mod 2^c 刷新；调用方 protocol_i_project_derived_select5.cpp:2022-2045 将完整 {Q′_b,0,0} 放入 key_records，再 forward shuffle。 | 若已有完整 V_pre sampler 且 rho 对其独立均匀，则可按条件式 rho_b=q′−Q_b(v) 补成受限联合样本；但本方 rho 是 ready 前 package 内容，且当前理想输出不给 q′。单变量均匀边缘或代数恒等式不足以证明 Law(package,rho,Q′ | x_b,z_b,pp,L)。 | 没有完整条件分布证明；不是攻击结论。 |
| q/key_records 与 forward shuffle | 本方完整 Q′_b record、shuffle metadata、round1/round2 frames，随后才开始活动 Select schedule。 | 固定前向 wire 的 S23/S28 条件推导不能给出 Q′、完整 package、shuffle 内部态和 L 的同分布联合 sampler。 | 未继续。 |
| adaptive Select、DCF 整池、membership、inverse/output/crash | 活动 seed 在 forward shuffle 后按轮发送；每轮选取数据依赖地址，key 允许双次 Eval；未用槽仍属于 party package/view。最终 membership 和 inverse shuffle产出 output share，再进入 PREPARE/COMMIT/PUBLISH。 | 由于上面的停止条件，本轮没有构造 used/unused slots、adaptive address、inverse shuffle 或 crash prefix 的 full sampler。h 的输出 pad 条件步不足以补齐它们。 | NOT_ATTEMPTED_AFTER_STOP_CONDITION。 |

### 6.3 Q′_b 可见性与材料时序

Q′_b 是真实运行中的 party-local 状态：carry/sign Eval 结束得到 Q_b，随后用已预载的本方 rho 做刷新；score conversion 返回后它被写成 full key record 并送入 forward shuffle。N≤128 的 v4 在 claim package 时装入 rho；N=256 的 streamed v3 在 ready store 中有 purpose=5 的 refresh slots，claim 后读入。故“代码实际可见”不等于“理想模拟器允许获得”；冻结接口仍只给 (x_b,z_b,pp,L)。

## 7. N=128 与 N=256 材料 schema 的停止后库存

以下为 S25 源码结构计数，用于说明未模拟的完整 view 大小；不是本轮隐私证明或复跑数据：

| 项 | N=128 / M6B5-v4 | N=256 / M6S2-v3 |
|---|---:|---:|
| Select DCF | 5·C(128,2)=40,640 | 5·C(256,2)=163,200 |
| shuffle metadata | 10N=1,280 | 10N=2,560 |
| carry/sign DCF | 2N=256 | 2N=512 |
| membership DCF | N=128 | N=256 |
| rho | N=128 个 package 字段 | N=256 个 purpose=5 slots |
| 合计逻辑材料项 | 42,432 | 166,784 |
| S25 实测布局（历史收据） | 44,986,628 B/party monolithic package | 两方 staged store 385,331,840 B；每方 192,665,920 B，manifest 每方 12,675,648 B、11 segments |

两种 schema 分开审查：v4 是单体结构化 package；M6S2/v3 按 session/party/purpose/端点/ABI 一次性 claim slot，N=256 refresh payload+manifest 布局增量是 21,504 B/party。当前 S30 没有读取、采样、重跑或安全组合任何已用/未用 key 槽；因此 N=128、N=256 的整池、逐轮自适应地址、shuffle/inverse、membership、输出与 crash gate 均为 NOT_ATTEMPTED_AFTER_STOP_CONDITION，不是 PASS。

## 8. v3 最小版本化规格与准入

### 8.1 目标外层功能和后端合同

版本名：score-to-mask-v3；路线名继续是 PROJECT_DERIVED_SELECT_5ITER。API 只接受本方原始 Q20.12 score share 与公开参数，输出只含本方最终 mask share；不增加外层泄露。

建议后端固定为两方半诚实 GMW 布尔电路协议，离线可信 T 为每个 AND gate 生成一次性 XOR-shared Beaver triples。电路复现项目派生的 score conversion、稳定 tie-break、五轮 Select、membership 与 inverse routing；没有 Q′_b 模块输出、没有 DCF party key、没有 carry/sign frame、没有 v2 rho。电路内部可有对每个 party 可见的随机 wire shares，但它们由 GMW simulator 覆盖，不是外层输出。Q′ 不再是 API 级完整键 share；即使电路语义有对应 wire，也不得由 API 导出。

精确 mask-share 输出按 F_TOPK-U 实现：P0 在安全计算中提供均匀私有随机 r，GMW 仅将 z0=r 输出给 P0、z1=r XOR M 输出给 P1。任一方不接收完整 M。

### 8.2 API、材料、版本绑定与迁移

候选 API：

    score_to_mask_v3_party(config_v3, input_share_b, gmw_channel, v3_preprocessing_reader)
        -> { local_mask_share, final_status }

- 新 schema/ABI：M6B7 / score-to-mask-v3；必须绑定 circuit digest、compiler/version、n,K,padded_n,h,c、score/tie ABI、session、party、gate/triple count 与输出/L profile。
- 材料只包含 and_gate_id -> (a_b,b_b,c_b) 的一次性 Boolean triple shares、随机 tape/domain metadata、manifest/ready/lease。T 以一个新随机 root 派生不同 domain tag 的 v3 tape；不再混用 OS getrandom 与 OpenSSL RAND_bytes 来暗示独立性。需要证明 domain-separated PRF/DRBG 联合伪随机界，并为 used/unused gates 统一生成本方视图。
- v2 M6B5/v4 和 M6S2/v3 的 DCF、rho、shuffle package/lease 不可作为 v3 输入或回滚材料；schema magic、protocol version、digest 和 runtime ABI 不兼容拒收。
- v2 保留为实验功能/回归对象，但不继承 v3 安全准入；v3 回滚只能回到明确标记为实验且 NO_GO 的 v2，不得经由 secure alias。
- v3 的 gate/triple 数、存储、消息字节、峰值内存均待电路编译/计量，当前 NOT_MEASURED。

### 8.3 消息 DAG、party-local 状态和一次性消费

1. Offline T：锁定 v3 circuit digest 和 gate schedule；为每个 AND gate 生成 Beaver triple，按党方 XOR share 输出独立材料文件；制作绑定 session/party/version 的 manifest。材料 ready 前校验全量长度、digest、无重复 ID；T 擦除 master seed 并退出，不与 party 合谋。
2. BIND/CLAIM：P0/P1 交换公开 pp、版本、circuit digest、manifest binding；不匹配统一 abort。两方分别一次性 claim 全部或流式 gate slots，写入不可回放 lease；未用 triples 仍是本方 view，simulator 必须采样。
3. INPUT：每方把 x_b 作为私有 GMW 输入；本方仅持有协议定义的 input wire shares。进行 32-bit modular score reconstruction/sign/carry 的电路运算；不发送原始 score 或 carry/sign local frames。
4. SELECT/DECLASSIFY：GMW 执行固定五轮 Select、stable index tie-break、membership 与 inverse routing。只在 S29 已批准时间点输出 L 中字段：masked values、shuffle endpoint/position、pivot、比较 bit、活动轮 sample seed、计数/长度；未来 seed 不提前公开。所有泄露值是电路规定 declassification output，其他中间 wire/share 不能打开。
5. OUTPUT/PUBLISH：以安全计算给 P0 输出 z0，给 P1 输出 z1；完成双方状态交易后 API 按冻结合同返回。输出 payload 的进程外发布与本地 .published 标记须明确区分。
6. ABORT/CRASH：任何材料、传输、证明绑定或协议状态错误只能走 ABORT，消耗/作废已 claim triples，禁止 replay。每一个 message prefix 都映射到负责人冻结的 F_TOPK-PREFIX 或 F_TOPK-ATOMIC；在负责人决定前只做方案设计，不作 runtime 实现。crash view 需要覆盖已输入 wire、已打开 masks、未使用 triple slots、partial declassification 和 orphan output share。

通信长度字段本身已在旧 L 内，但 v3 在线协议形状会改变。v3 必须证明长度是公开参数、固定 circuit schedule 与已批准活动状态的函数，不依赖额外秘密数据；若要改变长度泄露的语义，必须另行更新决策和理想功能，不能把“字段名没变”当作“泄露没变”。

### 8.4 证明义务与优势项

P0、P1 分别需要同一功能下独立 simulator/hybrid，并将本方输入、prescribed output share、公开参数及准确 L 作为唯一非内部模拟器输入：

1. 电路功能正确性：对全部允许输入和随机带证明固定 Q20.12 signed 解释、稳定同分、五轮选择、membership/inverse routing 与 Top-K oracle 完全一致。
2. output law：对两方逐一证明均匀 XOR-share law，且任何一方不能恢复完整 mask。
3. GMW 半诚实安全：固定具体 GMW/Beaver 实例及安全定义和仿真定理；证明所有已打开的 values 恰为 L，其他 wire shares、公开消息和 transcript 可由标准 simulator 生成。当前 trusted-T triple 方案没有 OT transcript。
4. 离线 preprocessing：证明每方完整本地 triple package（包括未用槽）、manifest、digest、ready、lease 和重启/abort 前缀可从输入接口采样；证明 T 与 P0/P1 的不合谋假设及跨 session one-shot 消费。
5. L 顺序组合：逐项证明 declassification 与 sample seed 的阶段/顺序符合已批准 L，无额外地址、未来随机数或 masked value 泄露。
6. crash/abort：对每个可见消息前缀证明与被选 ideal abort contract 一致，包括局部 share 已形成但未返回的阶段。
7. 可信 T 直接生成 Beaver triples 的当前候选不使用 OT；须证明 triple correctness、每方 package law 与一次性消费。优势项包括 epsilon_preprocessing、epsilon_PRG/DRBG、epsilon_rng_joint、epsilon_TLS 及 circuit/declassification hybrid 误差；若具体实现另外使用 OT，再计入 epsilon_OT。应给出参数 N、gate_count、query_count、time 后可审查的界。本轮没有量化这些项。

### 8.5 如何消除 S29 首个断点，及其限制

v3 设计不尝试对 v2 的 Law(package_b,Q_b,rho_b,Q′_b,frames | x_b,z_b,pp,L) 做代数拼接或假设独立。拟议改造会删除 v2 的 score-conversion DCF keys、预 ready rho 与 API 可导出的完整 Q′_b state；若后续实现固定具体 GMW 变体，并完成从 (x_b,z_b,pp,L) 出发的 GMW transcript、逐 gate triple/package、declassification、used/unused 材料与 crash-prefix simulator/hybrid 证明，则新协议证明可覆盖旧变量的条件分布缺口。该定理和完整 simulator 当前尚未实例化，因此 S30 不声称已经消断。

这是架构层的可审查消断点方案，不是现成运行时证明。若未来实现只是把 v2 API 融合/改名，继续在 party-local view 中形成 carry/sign DCF key/frame 或完整 Q′_b，或者不证明 GMW preprocessing/package simulator，则断点仍在，v3 不准入。

## 9. 路线比较与 S30 选择

| 路线 | 保持 v2 原功能/泄露 | 当前状态 | 风险与下一步 |
|---|---|---|---|
| 原 v2 proof-only | 可保持；不改运行时/输出/L | 仍缺完整条件 sampler，S30 一次有停止条件的尝试未闭合 | 可以未来继续研究，但继续写限定 V_pre 推导不会自动关闭 full package/output/crash view。 |
| 把 Q′_b 交给 scoreconv 子功能 | 不保持外层输出范围 | 拒绝作为原功能证明 | 组合功能泄露新增中间值；不准通过此路线制造 PASS。 |
| 版本化 score-to-mask-v3 | 目标合同保持 Top-K mask、均匀 XOR shares 和原 L；替换内部执行协议/材料 | S30 交付 DESIGN_SPEC_ONLY；未实现、未证明、未测量 | 最可执行的后续设计路线；固定 GMW simulator 与 triple package 规格。实现准入暂停于 abort 合同、通信长度/L保持性以及正式安全定理/独立审查。 |

**S30 最终路线决策：转入版本化 v3 设计规格，不以 v2 当前接口准入安全实现。v3 运行时代码本轮不准入；在负责人冻结 abort 合同并确认 v3 的通信长度/泄露语义后，再开始独立 v3 证明与实现审查。** 这不是 v2 安全不可能性结论，也不是攻击结论。

## 10. 独立审查与复跑记录

独立接收者 /root/s30_independent_receiver 未参与 S19–S29 的证明撰写、决策或实现。其重验 74/74 identity，并二审输出 mask share law 与 all-random-tape Top-K correctness 的边界、crash probe 只读长度头、v2 条件分布停止点、v3 GMW/preprocessing 假设和泄露边界。其提出的措辞修正已写回报告/gates/决策。独立只读审查不是密码学证明，也不替代组织/负责人签核；接收者已对修订版完成最终只读复核，确认指定边界准确且未发现新缺陷。未来 v3 密码学签核应由未参与 S19–S30 的审查者完成。

本轮实际执行：
- S25 identity 复核使用的 PowerShell 命令逻辑如下（逐 manifest path 校验，再按 manifest 规则重算 canonical）：
      $root = 'C:\Users\28641\.codex\worktrees\m6b-project-select5-s25\MoE_Top-k_Reproduction_Evaluation'
      $mf = Join-Path $root 'docs\reproduction\M6B_I_PROJECT_DERIVED_SELECT_5ITER_S25_SOURCE_BUILD_EVIDENCE_MANIFEST_2026-10-19.json'
      $m = Get-Content -LiteralPath $mf -Raw | ConvertFrom-Json
      $map = @{}; foreach ($f in $m.files) { $map[$f.path] = $f }
      $paths = [string[]]$map.Keys; [Array]::Sort($paths, [StringComparer]::Ordinal)
      $rows = foreach ($p in $paths) {
          $actual = (Get-FileHash -LiteralPath (Join-Path $root $p) -Algorithm SHA256).Hash.ToLowerInvariant()
          if ($actual -ne ([string]$map[$p].s25_sha256).ToLowerInvariant()) { throw "HASH_MISMATCH $p" }
          "$p$([char]9)$actual"
      }
      $canonical = ($rows -join [char]10) + [char]10
      $sha = [Security.Cryptography.SHA256]::Create()
      $canonical_sha256 = [BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($canonical))).Replace('-','').ToLowerInvariant()
      if ($canonical_sha256 -ne '64f32993b12bcd8692ed089f5dcdb8ce1be9f181ff217b508b8b0470eb8e440d') { throw "CANONICAL_MISMATCH" }
  命令结果：74 entries；0 mismatch；canonical SHA-256 与 manifest 声明相同。

- 源码只读定位/行号检查：protocol_i_parallel_shuffle.cpp、protocol_i_project_derived_select5.cpp、protocol_i_score_input.cpp、protocol_i_ucmp.cpp 与 S25 三进程 E2E 测试。
- 未运行：S25/S27 功能矩阵、构建、conformance、oracle differential、独立进程 E2E、密码学测试、完整池 simulator、正式 LAN/WAN 性能矩阵（均 NOT_RUN；性能为 NOT_MEASURED）。没有运行时更改，因此不为形式重跑历史矩阵。
- 未创建 secure alias；未改 VFSS/、VFSS-baseline/、论文、密钥、证书、材料包、构建物或无关测试。

## 11. 本轮修改文件

只新增 S30 治理/审查文档：

1. docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S30_REPORT_2026-10-20.md（本报告）
2. docs/reproduction/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S30_GATES_2026-10-20.json
3. docs/decisions/M6B_I_PROJECT_DERIVED_SELECT_5ITER_S30_DECISION_2026-10-20.md

没有修改活动或冻结运行时代码。
